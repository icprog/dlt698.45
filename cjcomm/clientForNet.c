#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "cjcomm.h"
#include "../include/Shmem.h"

/*
 * 本文件内存放客户端模式代码，专门处理客户端模式数据收发
 * 错误处理等
 * 以太网上线专用文件
 */

static CLASS26 Class26;
static CommBlock ClientForNetObject;
static long long ClientForNet_Task_Id;
static MASTER_STATION_INFO IpPool[4];

CommBlock *GetComBlockForNet() {
    return &ClientForNetObject;
}

/*
 *所有模块共享的写入函数，所有模块共享使用
 */
int ClientForNetWrite(int fd, INT8U *buf, INT16U len) {
    int ret = anetWrite(fd, buf, (int) len);
    if (ret != len) {
        asyslog(LOG_WARNING, "客户端[以太网]报文发送失败(长度:%d,错误:%d-%d)", len, errno, fd);
    }
    bufsyslog(buf, "客户端[以太网]发送:", len, 0, BUFLEN);
    return ret;
}

/*
 *所有模块共享的写入函数，所有模块共享使用
 */
int MixForNetWrite(int fd, INT8U *buf, INT16U len) {
    return SendBufWrite(buf, len);
}

void ClientForNetRead(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask) {
    CommBlock *nst = (CommBlock *) clientData;

    //判断fd中有多少需要接收的数据
    int revcount = 0;
    ioctl(fd, FIONREAD, &revcount);

    //关闭异常端口
    if (revcount == 0) {
        asyslog(LOG_WARNING, "客户端[以太网]链接出现异常[%d]，关闭端口", errno);
        aeDeleteFileEvent(eventLoop, fd, AE_READABLE);
        close(fd);
        nst->phy_connect_fd = -1;
        SetOnlineType(0);
    }

    if (revcount > 0) {
        for (int j = 0; j < revcount; j++) {
            read(fd, &nst->RecBuf[nst->RHead], 1);
            nst->RHead = (nst->RHead + 1) % BUFLEN;
        }
        bufsyslog(nst->RecBuf, "客户端[以太网]接收:", nst->RHead, nst->RTail, BUFLEN);

        for (int k = 0; k < 5; k++) {
            int len = 0;
            for (int i = 0; i < 5; i++) {
                len = StateProcess(nst, 10);
                if (len > 0) {
                    break;
                }
            }
            if (len <= 0) {
                break;
            }

            if (len > 0) {
                int apduType = ProcessData(nst);
                fprintf(stderr, "apduType=%d\n", apduType);
                ConformAutoTask(eventLoop, nst, apduType);
                switch (apduType) {
                    case LINK_RESPONSE:
                        First_VerifiTime(nst->linkResponse, nst->shmem); //简单对时
                        if (GetTimeOffsetFlag() == 1) {
                            Getk_curr(nst->linkResponse, nst->shmem);
                        }
                        nst->linkstate = build_connection;
                        nst->testcounter = 0;
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

MASTER_STATION_INFO getNextNetIpPort(CommBlock *commBlock) {
    static int index = 0;
    static int ChangeFlag = 0;
    //检查主站参数是否有变化
    if (ChangeFlag != ((ProgramInfo *) commBlock->shmem)->oi_changed.oi4500) {
        memset(&Class26, 0, sizeof(CLASS26));
        readCoverClass(0x4510, 0, (void *) &Class26, sizeof(CLASS26), para_vari_save);
        memcpy(&IpPool, &Class26.master.master, sizeof(IpPool));
        asyslog(LOG_WARNING, "检测到通信参数变化！刷新主站参数！");
        ChangeFlag = ((ProgramInfo *) commBlock->shmem)->oi_changed.oi4500;
        commBlock->Heartbeat = Class26.commconfig.heartBeat;
    }
    MASTER_STATION_INFO res;
    memset(&res, 0x00, sizeof(MASTER_STATION_INFO));
    snprintf((char *) res.ip, sizeof(res.ip), "%d.%d.%d.%d", IpPool[index].ip[1], IpPool[index].ip[2],
             IpPool[index].ip[3], IpPool[index].ip[4]);
    res.port = IpPool[index].port;
    index++;
    index %= 2;
    asyslog(LOG_INFO, "客户端[以太网]尝试链接的IP地址：%s:%d", res.ip, res.port);
    return res;
}

int CertainConnectForNet(char *interface, CommBlock *commBlock) {
    static int step = 0;
    static int fd = 0;
    static char peerBuf[32];
    static char boundBuf[32];
    static int port = 0;

    if (step == 0) {
        MASTER_STATION_INFO ip_port = getNextNetIpPort(commBlock);

        memset(boundBuf, 0x00, sizeof(boundBuf));
        if (GetInterFaceIp(interface, boundBuf) == 1) {
            fd = anetTcpNonBlockBindConnect(NULL, (char *) ip_port.ip, ip_port.port, boundBuf);
            if (fd > 0) {
                step = 1;
            }
        }
        return -1;
    } else if (step < 8) {
        memset(peerBuf, 0x00, sizeof(peerBuf));
        if (0 == anetPeerToString(fd, peerBuf, sizeof(peerBuf), &port)) {
            step = 0;
            return fd;
        } else {
            step++;
            return -1;
        }
    } else {
        close(fd);
        step = 0;
    }
}

void check_F101_changed_Net(CommBlock *commBlock) {
    static int ChangeFlag = 0;
    if (ChangeFlag != ((ProgramInfo *) commBlock->shmem)->oi_changed.oiF101) {
        ChangeFlag = ((ProgramInfo *) commBlock->shmem)->oi_changed.oiF101;
        asyslog(LOG_WARNING, "检测到安全参数变化！刷新安全参数！");
        readCoverClass(0xf101, 0, (void *) &commBlock->f101, sizeof(CLASS_F101), para_vari_save);
    }
}

int RegularClientForNet(struct aeEventLoop *ep, long long id, void *clientData) {
    CommBlock *nst = (CommBlock *) clientData;
    clearcount();

    if (nst->phy_connect_fd <= 0) {
        if (GetOnlineType() != 0) {
            return 2000;
        }
        refreshComPara(nst);

        nst->phy_connect_fd = CertainConnectForNet("eth0", nst);
        if (nst->phy_connect_fd > 0) {
            if (aeCreateFileEvent(ep, nst->phy_connect_fd, AE_READABLE, ClientForNetRead, nst) < 0) {
                close(nst->phy_connect_fd);
                nst->phy_connect_fd = -1;
            } else {
                dumpPeerStat(nst->phy_connect_fd, "客户端[以太网]与主站链路建立成功");
                gpofun("/dev/gpoONLINE_LED", 1);
                SetOnlineType(2);
            }
        }
    } else {
        if (Comm_task(nst) == -1) {
            asyslog(LOG_WARNING, "客户端[以太网]链接心跳超时，关闭端口");
            aeDeleteFileEvent(ep, nst->phy_connect_fd, AE_READABLE);
            close(nst->phy_connect_fd);
            nst->phy_connect_fd = -1;
            SetOnlineType(0);
        }

        check_F101_changed_Net(nst);
        CalculateTransFlow(nst->shmem);
        //暂时忽略函数返回
        RegularAutoTask(ep, nst);
    }

    return 2000;
}

static int RegularMixForNet(struct aeEventLoop *ep, long long id, void *clientData) {
    CommBlock *nst = (CommBlock *) clientData;
    clearcount();
    if (GetOnlineType() == 0x01) {
        // GPRS上线
        return 2000;
    }

    if (GetOnlineType() == 0x00) {
        SendBufClean();
        refreshComPara(nst);
        //在这里拨号上线，并发送登录报文
        nst->phy_connect_fd = CertainConnectForNet("eth0", nst);
        if (nst->phy_connect_fd > 0) {
            dumpPeerStat(nst->phy_connect_fd, "混合模式[以太网]与主站链路建立成功");
            gpofun("/dev/gpoONLINE_LED", 1);
            SetOnlineType(2);
            Comm_task(nst);
        }
    } else {
        //如果已经登录，则循环任务，等待发送
        if (Comm_task(nst) == -1) {
            asyslog(LOG_WARNING, "混合模式[以太网]链接心跳超时，关闭端口");
            close(nst->phy_connect_fd);
            nst->phy_connect_fd = -1;
            SetOnlineType(0);
        }

        CalculateTransFlow(nst->shmem);
        //暂时忽略函数返回
        RegularAutoTask(ep, nst);

        //检查是否有报文需要发送
        if (SendBufCheck()) {
            if (nst->phy_connect_fd < 0) {
                nst->phy_connect_fd = CertainConnectForNet("eth0", nst);
            } else {
                SendBufSendNext(nst->phy_connect_fd, ClientForNetWrite);
            }
        }
    }

    return 2000;
}

/*
 * 模块*内部*使用的初始化参数
 */
void ClientForNetInit(void) {
    SetOnlineType(0);
    asyslog(LOG_INFO, "\n\n>>>======初始化（客户端[以太网]模式）模块======<<<");

    //读取设备参数
    memset(&Class26, 0, sizeof(CLASS26));
    readCoverClass(0x4510, 0, (void *) &Class26, sizeof(CLASS26), para_vari_save);
    asyslog(LOG_INFO, "工作模式 enum{混合模式(0),客户机模式(1),服务器模式(2)}：%d", Class26.commconfig.workModel);
    asyslog(LOG_INFO, "连接方式 enum{TCP(0),UDP(1)}：%d", Class26.commconfig.connectType);
    asyslog(LOG_INFO, "连接应用方式 enum{主备模式(0),多连接模式(1)}：%d", Class26.commconfig.appConnectType);
    asyslog(LOG_INFO, "侦听端口列表：%d", Class26.commconfig.listenPort[0]);
    asyslog(LOG_INFO, "超时时间，重发次数：%02x", Class26.commconfig.timeoutRtry);
    asyslog(LOG_INFO, "心跳周期秒：%d", Class26.commconfig.heartBeat);
    memcpy(&IpPool, &Class26.master.master, sizeof(IpPool));
    asyslog(LOG_INFO, "主站通信地址(1)为：%d.%d.%d.%d:%d", IpPool[0].ip[1], IpPool[0].ip[2], IpPool[0].ip[3], IpPool[0].ip[4],
            IpPool[0].port);
    asyslog(LOG_INFO, "主站通信地址(2)为：%d.%d.%d.%d:%d", IpPool[1].ip[1], IpPool[1].ip[2], IpPool[1].ip[3], IpPool[1].ip[4],
            IpPool[1].port);

    if (Class26.commconfig.workModel == 0x01) {
        initComPara(&ClientForNetObject, ClientForNetWrite);
    } else {
        initComPara(&ClientForNetObject, MixForNetWrite);
    }

    ClientForNetObject.Heartbeat = Class26.commconfig.heartBeat;
    readCoverClass(0xf101, 0, (void *) &ClientForNetObject.f101, sizeof(CLASS_F101), para_vari_save);

    asyslog(LOG_INFO, ">>>======初始化（客户端[以太网]模式）结束======<<<");
}

/*
 * 供外部使用的初始化函数，并开启维护循环
 */
int StartClientForNet(struct aeEventLoop *ep, long long id, void *clientData) {
    ClientForNetInit();
    if (Class26.commconfig.workModel == 0x01) {
        ClientForNet_Task_Id = aeCreateTimeEvent(ep, 1000, RegularClientForNet, &ClientForNetObject, NULL);
    } else {
        ClientForNet_Task_Id = aeCreateTimeEvent(ep, 1000, RegularMixForNet, &ClientForNetObject, NULL);
    }
    asyslog(LOG_INFO, "客户端[以太网]时间事件注册完成(%lld)", ClientForNet_Task_Id);

    return 1;
}

/*
 * 用于程序退出时调用
 */
void ClientForNetDestory(void) {
    asyslog(LOG_INFO, "开始关闭客户端[以太网]接口(%d)", ClientForNetObject.phy_connect_fd);
    close(ClientForNetObject.phy_connect_fd);
    ClientForNetObject.phy_connect_fd = -1;
}
