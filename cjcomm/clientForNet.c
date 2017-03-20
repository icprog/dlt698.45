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

/*
 * 模块*内部*使用的初始化参数
 */
void ClientInit(void) {
    SetOnlineType(0);
    asyslog(LOG_INFO, "初始化（客户端模式）模块...");
    initComPara(&ClientObject);
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

static int CertainConnect() {
    static int step = 0;
    static int fd   = 0;
    static char peerBuf[32];
    static int port = 0;

    if (step == 0) {
        MASTER_STATION_INFO ip_port = getNextIpPort();

        fd = anetTcpNonBlockBindConnect(NULL, (char*)ip_port.ip, ip_port.port, "192.168.0.4");
        if (fd > 0) {
            step = 1;
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

int RegularClient(struct aeEventLoop* ep, long long id, void* clientData) {
    CommBlock* nst = (CommBlock*)clientData;
    clearcount(1);

    if (nst->phy_connect_fd <= 0) {
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
 * 供外部使用的初始化函数，并开启维护循环
 */
int StartClientForNet(struct aeEventLoop* ep, long long id, void* clientData) {
    CLASS26* class26 = (CLASS26*)clientData;
    memcpy(&IpPool, &class26->master.master, sizeof(IpPool));
    asyslog(LOG_INFO, "主站通信地址(1)为：%d.%d.%d.%d:%d", class26->master.master[0].ip[1], class26->master.master[0].ip[2], class26->master.master[0].ip[3],
            class26->master.master[0].ip[4], class26->master.master[0].port);
    asyslog(LOG_INFO, "主站通信地址(2)为：%d.%d.%d.%d:%d", class26->master.master[1].ip[1], class26->master.master[1].ip[2], class26->master.master[1].ip[3],
            class26->master.master[1].ip[4], class26->master.master[1].port);

    ClientInit();
    Client_Task_Id = aeCreateTimeEvent(ep, 1000, RegularClient, &ClientObject, NULL);

    asyslog(LOG_INFO, "客户端时间事件注册完成(%lld)", Client_Task_Id);
    StartMmq(ep, 0, &ClientObject);
    return 1;
}
