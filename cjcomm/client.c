#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "cjcomm.h"

/*
 * 本文件内存放客户端模式代码，专门处理客户端模式数据收发
 * 错误处理等
 */

//以太网、GPRS、侦听服务端处理对象
static CommBlock ClientObject;
static long long Client_Task_Id;
static MASTER_STATION_INFO IpPool[4];
static int OnlineType; // 0:没在线 1:GPRS 2:以太网

/*
 * 模块*内部*使用的初始化参数
 */
void ClientInit(void) {
    OnlineType = 0;
    asyslog(LOG_INFO, "初始化（客户端模式）模块...");
    initComPara(&ClientObject);
}

/*
 * 获取当前在线状态
 */
int GetOnlineType(void) {
    return OnlineType;
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

void ClientRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask) {
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
                fprintf(stderr,"apduType=%d\n",apduType);
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

MASTER_STATION_INFO getNextIpPort(void) {
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

int RegularClient(struct aeEventLoop* ep, long long id, void* clientData) {
    CommBlock* nst = (CommBlock*)clientData;
    clearcount(1);

    if (nst->phy_connect_fd <= 0) {
        char errmsg[256];
        memset(errmsg, 0x00, sizeof(errmsg));
        initComPara(nst);
        OnlineType                  = 0;
        MASTER_STATION_INFO ip_port = getNextIpPort();
        nst->phy_connect_fd         = anetTcpConnect(errmsg, (char*)ip_port.ip, ip_port.port);
        if (nst->phy_connect_fd > 0) {
            //            asyslog(LOG_INFO, "链接主站(主站地址:%s,结果:%d)", IPaddr, nst->phy_connect_fd);
            if (aeCreateFileEvent(ep, nst->phy_connect_fd, AE_READABLE, ClientRead, nst) < 0) {
                close(nst->phy_connect_fd);
                nst->phy_connect_fd = -1;
            } else {
                anetTcpKeepAlive(NULL, nst->phy_connect_fd);
                asyslog(LOG_INFO, "与主站链路建立成功");
                gpofun("/dev/gpoONLINE_LED", 1);
                OnlineType = 1;
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
 * 供外部使用的初始化函数，并开启维护循环
 */
int StartClient(struct aeEventLoop* ep, long long id, void* clientData) {
    CLASS25* class25 = (CLASS25*)clientData;
    memcpy(&IpPool, &class25->master.master, sizeof(IpPool));
    asyslog(LOG_INFO, "主站通信地址(1)为：%d.%d.%d.%d:%d", class25->master.master[0].ip[1], class25->master.master[0].ip[2], class25->master.master[0].ip[3],
            class25->master.master[0].ip[4], class25->master.master[0].port);
    asyslog(LOG_INFO, "主站通信地址(2)为：%d.%d.%d.%d:%d", class25->master.master[1].ip[1], class25->master.master[1].ip[2], class25->master.master[1].ip[3],
            class25->master.master[1].ip[4], class25->master.master[1].port);

    ClientInit();
    Client_Task_Id = aeCreateTimeEvent(ep, 1000, RegularClient, &ClientObject, NULL);

    asyslog(LOG_INFO, "监听服务器时间事件注册完成(%lld)", Client_Task_Id);
    StartMmq(ep, 0, &ClientObject);
    return 1;
}
