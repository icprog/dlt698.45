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
 * GPRS上线专用文件
 */

static CLASS25 Class25;
static CommBlock ClientForGprsObject;
static long long ClientForGprs_Task_Id;
static MASTER_STATION_INFO NetIps[4];

CommBlock *GetComBlockForGprs() {
    return &ClientForGprsObject;
}

/*
 *所有模块共享的写入函数，所有模块共享使用
 */
int ClientForGprsWrite(int fd, INT8U *buf, INT16U len) {
    gpofun("gpoREMOTE_GREEN", 1);
    int ret = anetWrite(fd, buf, (int) len);
    if (ret != len) {
        asyslog(LOG_WARNING, "客户端[GPRS]报文发送失败(长度:%d,错误:%d-%d)", len, errno, fd);
    }
    bufsyslog(buf, "客户端[GPRS]发送:", len, 0, BUFLEN);
    gpofun("gpoREMOTE_GREEN", 0);
    return ret;
}

/*
 *所有模块共享的写入函数，所有模块共享使用
 */
int MixForGprsWrite(int fd, INT8U *buf, INT16U len) {
    return SendBufWrite(buf, len);
}

static void ClientForGprsRead(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask) {
    CommBlock *nst = (CommBlock *) clientData;

    //判断fd中有多少需要接收的数据
    int revcount = 0;
    ioctl(fd, FIONREAD, &revcount);

    //关闭异常端口
    if (revcount == 0) {
        asyslog(LOG_WARNING, "客户端[GPRS]链接出现异常[%d]，关闭端口", errno);
        aeDeleteFileEvent(eventLoop, fd, AE_READABLE);
        close(fd);
        nst->phy_connect_fd = -1;
        SetOnlineType(0);
    }

    if (revcount > 0) {
        gpofun("gpoREMOTE_RED", 1);
        for (int j = 0; j < revcount; j++) {
            read(fd, &nst->RecBuf[nst->RHead], 1);
            nst->RHead = (nst->RHead + 1) % BUFLEN;
        }
        bufsyslog(nst->RecBuf, "客户端[GPRS]接收:", nst->RHead, nst->RTail, BUFLEN);
        gpofun("gpoREMOTE_RED", 0);
    }
}

static MASTER_STATION_INFO getNextGprsIpPort(CommBlock *commBlock) {
    static int index = 0;
    static int ChangeFlag = 0;
    //检查主站参数是否有变化
    if (ChangeFlag != ((ProgramInfo *) commBlock->shmem)->oi_changed.oi4500) {
        readCoverClass(0x4500, 0, (void *) &Class25, sizeof(CLASS25), para_vari_save);
        memcpy(&NetIps, &Class25.master.master, sizeof(NetIps));
        asyslog(LOG_WARNING, "检测到GPRS通信参数变化！刷新主站参数！");
        ChangeFlag = ((ProgramInfo *) commBlock->shmem)->oi_changed.oi4500;
        commBlock->Heartbeat = Class25.commconfig.heartBeat;
        readCoverClass(0xf101, 0, (void *) &ClientForGprsObject.f101, sizeof(CLASS_F101), para_vari_save);
    }

    MASTER_STATION_INFO res;
    memset(&res, 0x00, sizeof(MASTER_STATION_INFO));
    snprintf((char *) res.ip, sizeof(res.ip), "%d.%d.%d.%d", NetIps[index].ip[1], NetIps[index].ip[2],
             NetIps[index].ip[3], NetIps[index].ip[4]);
    res.port = NetIps[index].port;
    index++;
    index %= 2;
    asyslog(LOG_INFO, "客户端[GPRS]尝试链接的IP地址：%s:%d", res.ip, res.port);
    return res;
}

int CertainConnectForGprs(char *interface, CommBlock *commBlock) {
    static int step = 0;
    static int fd = 0;
    static char peerBuf[32];
    static char boundBuf[32];
    static int port = 0;

    if (step == 0) {
        MASTER_STATION_INFO ip_port = getNextGprsIpPort(commBlock);

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

void check_F101_changed_Gprs(CommBlock *commBlock) {
    static int ChangeFlag = 0;
    if (ChangeFlag != ((ProgramInfo *) commBlock->shmem)->oi_changed.oiF101) {
        ChangeFlag = ((ProgramInfo *) commBlock->shmem)->oi_changed.oiF101;
        asyslog(LOG_WARNING, "检测到安全参数变化！刷新安全参数！");
        readCoverClass(0xf101, 0, (void *) &commBlock->f101, sizeof(CLASS_F101), para_vari_save);
    }
}

static int RegularClientForGprs(struct aeEventLoop *ep, long long id, void *clientData) {
    CommBlock *nst = (CommBlock *) clientData;
    clearcount();

    if (nst->phy_connect_fd <= 0) {
        if (GetOnlineType() != 0) {
            return 2000;
        }
        refreshComPara(nst);

        nst->phy_connect_fd = CertainConnectForGprs("ppp0", nst);
        if (nst->phy_connect_fd > 0) {
            if (aeCreateFileEvent(ep, nst->phy_connect_fd, AE_READABLE, ClientForGprsRead, nst) < 0) {
                close(nst->phy_connect_fd);
                nst->phy_connect_fd = -1;
            } else {
                dumpPeerStat(nst->phy_connect_fd, "客户端[GPRS]与主站链路建立成功");
                gpofun("/dev/gpoONLINE_LED", 1);
                SetOnlineType(1);
            }
        }else{
        	return 1000;
        }
    } else {
        if (Comm_task(nst) == -1) {
            asyslog(LOG_WARNING, "客户端[GPRS]链接心跳超时，关闭端口");
            AT_POWOFF();
            aeDeleteFileEvent(ep, nst->phy_connect_fd, AE_READABLE);
            close(nst->phy_connect_fd);
            nst->phy_connect_fd = -1;
            SetOnlineType(0);
        }

		int res = 0;
		do {
			res = StateProcess(nst, 5);
			if (nst->deal_step >= 3) {
				int apduType = ProcessData(nst);
				ConformAutoTask(ep, nst, apduType);
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
		} while (res == 1);


        check_F101_changed_Gprs(nst);
        CalculateTransFlow(nst->shmem);
        //暂时忽略函数返回
        RegularAutoTask(ep, nst);
    }

    return 100;
}

static int RegularMixForGprs(struct aeEventLoop *ep, long long id, void *clientData) {
    CommBlock *nst = (CommBlock *) clientData;
    clearcount();

    if (GetOnlineType() == 0x02) {
        //以太网上线
        return 2000;
    }

    if (GetOnlineType() == 0x00) {
        SendBufClean();
        refreshComPara(nst);
        //在这里拨号上线，并发送登录报文
        nst->phy_connect_fd = CertainConnectForGprs("ppp0", nst);
        if (nst->phy_connect_fd > 0) {
            dumpPeerStat(nst->phy_connect_fd, "混合模式[GPRS]与主站链路建立成功");
            gpofun("/dev/gpoONLINE_LED", 1);
            SetOnlineType(1);
            Comm_task(nst);
        }
    } else {
        //如果已经登录，则循环任务，等待发送
        if (Comm_task(nst) == -1) {
            asyslog(LOG_WARNING, "混合模式[GPRS]链接心跳超时，关闭端口");
            AT_POWOFF();
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
                nst->phy_connect_fd = CertainConnectForGprs("ppp0", nst);
            } else {
                SendBufSendNext(nst->phy_connect_fd, ClientForGprsWrite);
            }
        }
    }

    return 1000;
}

/*
 * 模块*内部*使用的初始化参数
 */
static void ClientForGprsInit(void) {
    asyslog(LOG_INFO, "\n\n>>>======初始化（客户端[GPRS]模式）模块======<<<");

    //读取设备参数
    readCoverClass(0x4500, 0, (void *) &Class25, sizeof(CLASS25), para_vari_save);
    asyslog(LOG_INFO, "工作模式 enum{混合模式(0),客户机模式(1),服务器模式(2)}：%d", Class25.commconfig.workModel);
    asyslog(LOG_INFO, "在线方式 enum{永久在线(0),被动激活(1)}：%d", Class25.commconfig.onlineType);
    asyslog(LOG_INFO, "连接方式 enum{TCP(0),UDP(1)}：%d", Class25.commconfig.connectType);
    asyslog(LOG_INFO, "连接应用方式 enum{主备模式(0),多连接模式(1)}：%d", Class25.commconfig.appConnectType);
    asyslog(LOG_INFO, "侦听端口列表：%d", Class25.commconfig.listenPort[0]);
    asyslog(LOG_INFO, "超时时间，重发次数：%02x", Class25.commconfig.timeoutRtry);
    asyslog(LOG_INFO, "心跳周期秒：%d", Class25.commconfig.heartBeat);
    memcpy(&NetIps, &Class25.master.master, sizeof(NetIps));
    asyslog(LOG_INFO, "主站通信地址(1)为：%d.%d.%d.%d:%d", NetIps[0].ip[1], NetIps[0].ip[2], NetIps[0].ip[3], NetIps[0].ip[4],
            NetIps[0].port);
    asyslog(LOG_INFO, "主站通信地址(2)为：%d.%d.%d.%d:%d", NetIps[1].ip[1], NetIps[1].ip[2], NetIps[1].ip[3], NetIps[1].ip[4],
            NetIps[1].port);

    if (Class25.commconfig.workModel == 0x01) {
        initComPara(&ClientForGprsObject, ClientForGprsWrite);
    } else {
        initComPara(&ClientForGprsObject, MixForGprsWrite);
    }
    ClientForGprsObject.Heartbeat = Class25.commconfig.heartBeat;
    readCoverClass(0xf101, 0, (void *) &ClientForGprsObject.f101, sizeof(CLASS_F101), para_vari_save);

    asyslog(LOG_INFO, ">>>======初始化（客户端[GPRS]模式）结束======<<<");
}

/*
 * 供外部使用的初始化函数，并开启维护循环
 */
int StartClientForGprs(struct aeEventLoop *ep, long long id, void *clientData) {
    ClientForGprsInit();
    CreateATWorker(&Class25);

    if (Class25.commconfig.workModel == 0x01) {
        ClientForGprs_Task_Id = aeCreateTimeEvent(ep, 1000, RegularClientForGprs, &ClientForGprsObject, NULL);
    } else {
        ClientForGprs_Task_Id = aeCreateTimeEvent(ep, 1000, RegularMixForGprs, &ClientForGprsObject, NULL);
    }
    asyslog(LOG_INFO, "客户端[GPRS]时间事件注册完成(%lld)", ClientForGprs_Task_Id);

    return 1;
}

/*
 * 用于程序退出时调用
 */
void ClientForGprsDestory(void) {
    asyslog(LOG_INFO, "开始关闭客户端[GPRS]接口(%d)", ClientForGprsObject.phy_connect_fd);
    close(ClientForGprsObject.phy_connect_fd);
    ClientForGprsObject.phy_connect_fd = -1;
}

