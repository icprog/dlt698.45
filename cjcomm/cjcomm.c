#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "cjcomm.h"

//共享内存地址
static ProgramInfo* JProgramInfo = NULL;
static int ProgIndex             = 0;
static int OnlineType; // 0:没在线 1:GPRS 2:以太网
CLASS_4000 class_4000;

/*
 * 获取当前在线状态
 */
int GetOnlineType(void) {
    return OnlineType;
}

void SetOnlineType(int type) {
    OnlineType = type;
}

void CalculateTransFlow(ProgramInfo* prginfo_event) {
    static Flow_tj c2200;
    //统计临时变量
    static long long rtx_bytes = 0;
    static long long rx_bytes  = 0;
    static long long tx_bytes  = 0;
    static int localMin        = 0;

    static int first_flag = 1;
    if (first_flag == 1) {
        first_flag = 0;
        memset(&c2200, 0x00, sizeof(c2200));
        // readVariData(0x2200, 0, &c2200, sizeof(c2200), para_vari_save);
        asyslog(LOG_INFO, "初始化月流量统计(%lld)", c2200.flow.month_tj);
    }

    TS ts = {};
    TSGet(&ts);
    if (localMin != ts.Minute) {
        localMin = ts.Minute;
    } else {
        return;
    }

    FILE* rfd = fopen("/sys/class/net/eth0/statistics/rx_bytes", "r");
    FILE* tfd = fopen("/sys/class/net/eth0/statistics/tx_bytes", "r");
    if (rfd == NULL || tfd == NULL) {
        asyslog(LOG_INFO, "未检测到端口(PPP0)打开");
        if(rfd != NULL) {
            fclose(rfd);
        }
        if(tfd != NULL) {
            fclose(tfd);
        }
        return;
    }

    fscanf(rfd, "%lld", &rx_bytes);
    fscanf(tfd, "%lld", &tx_bytes);

    fclose(rfd);
    fclose(tfd);

    //说明ppp0重新拨号了
    if (rtx_bytes > rx_bytes + tx_bytes) {
        rtx_bytes = 0;
    }

    if (ts.Minute % 2 == 0) {
        asyslog(LOG_INFO, "20分钟月流量统计，未统计流量%lld", (rx_bytes + tx_bytes) - rtx_bytes);
        c2200.flow.month_tj += (rx_bytes + tx_bytes) - rtx_bytes;
        // saveCoverClass(0x2200, 0, &c2200, sizeof(c2200), para_init_save);
        rtx_bytes = rx_bytes + tx_bytes;
        Event_3110(c2200.flow.month_tj, sizeof(c2200.flow), prginfo_event);
    }

    return;
}

void clearcount() {
    JProgramInfo->Projects[ProgIndex].WaitTimes = 0;
}

void QuitProcess(int sig) {
    asyslog(LOG_INFO, "通信模块退出,收到信号类型(%d)", sig);
    IfrDestory();
    SerialDestory();
    ServerDestory();
    MmqDestory();
    ClientForGprsDestory();
    ClientForNetDestory();

    //关闭AT模块
    asyslog(LOG_INFO, "关闭AT模块电源");
    AT_POWOFF();
    asyslog(LOG_INFO, "通信模块退出完成...");
    exit(0);
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

void WriteLinkRequest(INT8U link_type, INT16U heartbeat, LINK_Request* link_req) {
    TS ts = {};
    TSGet(&ts);
    link_req->type              = link_type;
    link_req->piid_acd.data     = 0;
    link_req->time.year         = ((ts.Year << 8) & 0xff00) | ((ts.Year >> 8) & 0xff); // apdu 先高后低
    link_req->time.month        = ts.Month;
    link_req->time.day_of_month = ts.Day;
    link_req->time.day_of_week  = ts.Week;
    link_req->time.hour         = ts.Hour;
    link_req->time.minute       = ts.Minute;
    link_req->time.second       = ts.Sec;
    link_req->time.milliseconds = 0;
    link_req->heartbeat         = ((heartbeat << 8) & 0xff00) | ((heartbeat >> 8) & 0xff);
}

int Comm_task(CommBlock* compara) {
    INT16U heartbeat = 60;

    if (abs(time(NULL) - compara->lasttime) < heartbeat) {
        return 0;
    }
    compara->lasttime = time(NULL);

    if (compara->testcounter >= 2) {
        return -1;
    }

    if (compara->linkstate == close_connection) //物理通道建立完成后，如果请求状态为close，则需要建立连接
    {
        WriteLinkRequest(build_connection, heartbeat, &compara->link_request);
        int sendlen = Link_Request(compara->link_request, compara->serveraddr, compara->SendBuf);
        compara->p_send(compara->phy_connect_fd, compara->SendBuf, sendlen);
    } else {
        WriteLinkRequest(heart_beat, heartbeat, &compara->link_request);
        int sendlen = Link_Request(compara->link_request, compara->serveraddr, compara->SendBuf);
        compara->p_send(compara->phy_connect_fd, compara->SendBuf, sendlen);
    }
    compara->testcounter++;
    return 0;
}

void refreshComPara(CommBlock* compara) {
    compara->phy_connect_fd = -1;
    compara->testcounter    = 0;
    compara->linkstate      = close_connection;
    memset(compara->RecBuf, 0, sizeof(compara->RecBuf));
    memset(compara->SendBuf, 0, sizeof(compara->SendBuf));
    memset(compara->DealBuf, 0, sizeof(compara->DealBuf));
    compara->RHead     = 0;
    compara->RTail     = 0;
    compara->deal_step = 0;
    compara->rev_delay = 20;
    compara->shmem     = JProgramInfo;
    compara->lasttime  = 0;
}

void initComPara(CommBlock* compara, INT8S (*p_send)(int fd, INT8U* buf, INT16U len)) {
    CLASS_4001_4002_4003 c4001;
    memset(&c4001, 0x00, sizeof(c4001));
    readCoverClass(0x4001, 0, &c4001, sizeof(c4001), para_vari_save);
    memcpy(compara->serveraddr, c4001.curstom_num, 16);

    compara->phy_connect_fd = -1;
    compara->testcounter    = 0;
    compara->linkstate      = close_connection;
    memset(compara->RecBuf, 0, sizeof(compara->RecBuf));
    memset(compara->SendBuf, 0, sizeof(compara->SendBuf));
    memset(compara->DealBuf, 0, sizeof(compara->DealBuf));
    compara->RHead     = 0;
    compara->RTail     = 0;
    compara->deal_step = 0;
    compara->rev_delay = 20;
    compara->shmem     = JProgramInfo;
    compara->p_send    = p_send;
    compara->lasttime  = 0;

    CLASS19 Class19 = {};
    memset(&Class19, 0, sizeof(CLASS19));
    if (readCoverClass(0x4300, 0, &Class19, sizeof(CLASS19), para_vari_save)) {
        memcpy(&compara->myAppVar.server_factory_version, &Class19.info, sizeof(FactoryVersion));
    }
    for (int i = 0; i < 2; i++) {
        compara->myAppVar.FunctionConformance[i] = 0xff;
    }
    for (int i = 0; i < 5; i++) {
        compara->myAppVar.ProtocolConformance[i] = 0xff;
    }
    compara->myAppVar.server_deal_maxApdu    = 1024;
    compara->myAppVar.server_recv_size       = 1024;
    compara->myAppVar.server_send_size       = 1024;
    compara->myAppVar.server_recv_maxWindow  = 1;
    compara->myAppVar.expect_connect_timeout = 56400;

    readCoverClass(0xf101, 0, &compara->f101, sizeof(CLASS_F101), para_vari_save);
}

void dumpPeerStat(int fd, char* info) {
    int peerBuf[128];
    int port = 0;

    memset(peerBuf, 0x00, sizeof(peerBuf));
    anetTcpKeepAlive(NULL, fd);
    anetPeerToString(fd, peerBuf, sizeof(peerBuf), &port);
    asyslog(LOG_INFO, "[%s%s]:%d\n", info, peerBuf, port);
}

/*********************************************************
 * 进程初始化
 *********************************************************/
void enviromentCheck(int argc, char* argv[]) {
    pid_t pids[128];
    if (prog_find_pid_by_name((INT8S*)argv[0], pids) > 1) {
        asyslog(LOG_ERR, "CJCOMM进程仍在运行,进程号[%d]，程序退出...", pids[0]);
        exit(0);
    }

    //绑定信号处理了函数
    struct sigaction sa = {};
    Setsig(&sa, QuitProcess);

    //向cjmain报告启动
    ProgIndex    = atoi(argv[1]);
    JProgramInfo = OpenShMem("ProgramInfo", sizeof(ProgramInfo), NULL);
    memcpy(JProgramInfo->Projects[ProgIndex].ProjectName, "cjcomm", sizeof("cjcomm"));
    JProgramInfo->Projects[ProgIndex].ProjectID = getpid();
}

int main(int argc, char* argv[]) {
    printf("version 1019\n");
    memset(&class_4000, 0, sizeof(CLASS_4000));
    enviromentCheck(argc, argv);
    SetOnlineType(0);

    //开启网络IO事件处理框架
    aeEventLoop* ep;
    ep = aeCreateEventLoop(128);
    if (ep == NULL) {
        asyslog(LOG_ERR, "事件处理框架创建失败，程序终止。\n");
        exit(0);
    }

    // StartIfr(ep, 0, NULL);
    // StartSerial(ep, 0, NULL);
    StartServer(ep, 0, NULL);
    StartVerifiTime(ep, 0, JProgramInfo);
    StartClientForGprs(ep, 0, NULL);
    StartClientForNet(ep, 0, NULL);
    StartMmq(ep, 0, NULL);

    aeMain(ep);

    //退出信号
    QuitProcess(99);
    return EXIT_SUCCESS;
}
