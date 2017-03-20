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

/*
 * 本文件内存放客户端模式代码，专门处理客户端模式数据收发
 * 错误处理等
 * GPRS上线专用文件
 */

//以太网、GPRS、侦听服务端处理对象
static CommBlock ClientObject;
static long long Client_Task_Id;
static MASTER_STATION_INFO IpPool[4];

/*
 * 模块*内部*使用的初始化参数
 */
static void ClientInit(void) {
    SetOnlineType(0);
    asyslog(LOG_INFO, "初始化（客户端模式）模块...");
    initComPara(&ClientObject);
}

static void ClientRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask) {
    CommBlock* nst = (CommBlock*)clientData;

    //判断fd中有多少需要接收的数据
    int revcount = 0;
    ioctl(fd, FIONREAD, &revcount);

    //关闭异常端口
    if (revcount == 0) {
        asyslog(LOG_WARNING, "链接出现异常，关闭端口");
        aeDeleteFileEvent(eventLoop, fd, AE_READABLE);
        close(fd);
        nst->phy_connect_fd = -1;
    }

    if (revcount > 0) {
        for (int j = 0; j < revcount; j++) {
            read(fd, &nst->RecBuf[nst->RHead], 1);
            nst->RHead = (nst->RHead + 1) % BUFLEN;
        }
        bufsyslog(nst->RecBuf, "Recv:", nst->RHead, nst->RTail, BUFLEN);

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
                        if (GetTimeOffsetFlag() == 1) {
                            Getk(nst->linkResponse, nst->shmem);
                        }
                        nst->linkstate   = build_connection;
                        nst->testcounter = 0;
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

static MASTER_STATION_INFO getNextIpPort(void) {
    static int index = 0;
    MASTER_STATION_INFO res;
    memset(&res, 0x00, sizeof(MASTER_STATION_INFO));
    snprintf((char*)res.ip, sizeof(res.ip), "%d.%d.%d.%d", IpPool[index].ip[1], IpPool[index].ip[2], IpPool[index].ip[3], IpPool[index].ip[4]);
    res.port = IpPool[index].port;
    index++;
    index %= 2;
    asyslog(LOG_INFO, "尝试链接的IP地址：%s:%d", res.ip, res.port);
    return res;
}

int GetInterFaceIp(char* interface, char* ips) {
    int sock;
    struct sockaddr_in sin;
    struct ifreq ifr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        return -1;
    }
    strncpy(ifr.ifr_name, interface, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;
    if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
        close(sock);
        return -1;
    }
    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
    if (sin.sin_addr.s_addr > 0) {
        int ip[4];
        memset(ip, 0x00, sizeof(ip));
        sscanf(inet_ntoa(sin.sin_addr), "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
        snprintf(ips, 16, "%d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
        close(sock);
        return 1;
    }
    close(sock);
    return 0;
}

static int CertainConnect() {
    static int step = 0;
    static int fd   = 0;
    static char peerBuf[32];
    static char boundBuf[32];
    static int port = 0;

    if (step == 0) {
        MASTER_STATION_INFO ip_port = getNextIpPort();

        memset(boundBuf, 0x00, sizeof(boundBuf));
        if (GetInterFaceIp("ppp0", boundBuf) == 1) {
            fd = anetTcpNonBlockBindConnect(NULL, (char*)ip_port.ip, ip_port.port, boundBuf);
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

static int RegularClient(struct aeEventLoop* ep, long long id, void* clientData) {
    CommBlock* nst = (CommBlock*)clientData;
    clearcount(1);

    if (nst->phy_connect_fd <= 0 && GetOnlineType() == 0) {
        initComPara(nst);
        SetOnlineType(0);

        nst->phy_connect_fd = CertainConnect();
        if (nst->phy_connect_fd > 0) {
            //            asyslog(LOG_INFO, "链接主站(主站地址:%s,结果:%d)", IPaddr, nst->phy_connect_fd);
            if (aeCreateFileEvent(ep, nst->phy_connect_fd, AE_READABLE, ClientRead, nst) < 0) {
                close(nst->phy_connect_fd);
                nst->phy_connect_fd = -1;
            } else {
                dumpPeerStat(nst->phy_connect_fd, "客户端与主站链路建立成功");
                gpofun("/dev/gpoONLINE_LED", 1);
                SetOnlineType(1);
            }
        }
    } else {
        Comm_task(nst);
        EventAutoReport(nst);
        CalculateTransFlow(nst->shmem);
        //暂时忽略函数返回
        RegularAutoTask(ep, nst);
    }

    return 2000;
}

/*
 *所有模块共享的写入函数，所有模块共享使用
 */
static int ClientWrite(int fd, INT8U* buf, INT16U len) {
    int ret = anetWrite(fd, buf, (int)len);
    if (ret != len) {
        asyslog(LOG_WARNING, "[客户]报文发送失败(长度:%d,错误:%d)", len, errno);
    }
    bufsyslog(buf, "客户发送:", len, 0, BUFLEN);
    return ret;
}

/*
 * 供外部使用的初始化函数，并开启维护循环
 */
int StartClient(struct aeEventLoop* ep, long long id, void* clientData) {
    CLASS25* class25 = (CLASS25*)clientData;
    memcpy(&IpPool, &class25->master.master, sizeof(IpPool));
    asyslog(LOG_INFO, "主站通信地址(1)为：%d.%d.%d.%d:%d", class25->master.master[0].ip[1], class25->master.master[0].ip[2], class25->master.master[0].ip[3],
            class25->master.master[0].ip[4], class25->master.master[0].port);
    asyslog(LOG_INFO, "主站通信地址(2)为：%d.%d.%d.%d:%d", class25->master.master[1].ip[1], class25->master.master[1].ip[2], class25->master.master[1].ip[3],
            class25->master.master[1].ip[4], class25->master.master[1].port);

    //绑定本地发送函数
    ClientObject.p_send = ClientWrite;

    ClientInit();
    Client_Task_Id = aeCreateTimeEvent(ep, 1000, RegularClient, &ClientObject, NULL);

    asyslog(LOG_INFO, "客户端时间事件注册完成(%lld)", Client_Task_Id);
    StartMmq(ep, 0, &ClientObject);
    return 1;
}

/*
 * 用于程序退出时调用
 */
void ClientDestory(void) {
    //关闭资源
    asyslog(LOG_INFO, "开始关闭终端对主站链接接口(%d)", ClientObject.phy_connect_fd);
    close(ClientObject.phy_connect_fd);
    ClientObject.phy_connect_fd = -1;
}
