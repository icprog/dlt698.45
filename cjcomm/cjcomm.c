#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>

#include "cjcomm.h"
#include "../include/Shmem.h"

//共享内存地址
static ProgramInfo *JProgramInfo = NULL;
static int ProgIndex = 0;
static int OnlineType; // 0:没在线 1:GPRS 2:以太网 3:内部协议栈连接主站成功
CLASS_4000 class_4000;

/*
 * 获取当前在线状态
 */
int GetOnlineType(void) {
    return OnlineType;
}

void SetOnlineType(int type) {
    if (JProgramInfo != NULL) {
        JProgramInfo->dev_info.jzq_login = type;
    }
    OnlineType = type;
    if (type == 0) {
        SetConnectStatus(0);
    } else {
        SetConnectStatus(1);
    }
}

void SetGprsStatus(int status) {
    if (JProgramInfo != NULL) {
        JProgramInfo->dev_info.gprs_status = status;
    }
}

void SetGprsCSQ(int csq) {
    if (JProgramInfo != NULL) {
        JProgramInfo->dev_info.Gprs_csq = csq;
    }
}

void SetWireLessType(int type) {
    if (JProgramInfo != NULL) {
        JProgramInfo->dev_info.wirelessType = type;
    }
}

void SetPPPDStatus(int status) {
    if (JProgramInfo != NULL) {
        JProgramInfo->dev_info.pppd_status = status;
    }
}

void SetConnectStatus(int status) {
    if (JProgramInfo != NULL) {
        JProgramInfo->dev_info.connect_ok = status;
    }
}

void CalculateTransFlow(ProgramInfo *prginfo_event) {
    static Flow_tj c2200;

    //统计临时变量
    static int rtx_bytes = 0;
    static int rx_bytes = 0;
    static int tx_bytes = 0;
    static int localMin = 0;
    static int localDay = 0;
    static int localMonth = 0;

    static int first_flag = 1;

    TS ts = {};
    TSGet(&ts);

    if (first_flag == 1) {
        first_flag = 0;
        memset(&c2200, 0x00, sizeof(c2200));
        readVariData(0x2200, 0, &c2200, sizeof(c2200));
        asyslog(LOG_INFO, "初始化月流量统计(%d)", c2200.flow.month_tj);

        localMin = ts.Minute;
        localDay = ts.Day;
        localMonth = ts.Month;
    }


    if (localMin != ts.Minute) {
        localMin = ts.Minute;
    } else {
        return;
    }

    FILE *rfd = fopen("/proc/net/dev", "r");
    if (rfd == NULL) {
        asyslog(LOG_INFO, "流量统计文件不存在.");
        return;
    }
    char buf[128];
    int index = 0;

    for (index = 0; index < 8; ++index) {
        memset(buf, 0x00, sizeof(buf));
        fgets(buf, sizeof(buf), rfd);
        if (strstr(buf, "ppp0") > 0) {
            sscanf(buf, "%*[^:]:%d%*d%*d%*d%*d%*d%*d%*d%d", &rx_bytes, &tx_bytes);
            break;
        }
    }
    fclose(rfd);

    if (index >= 8) {
        return;
    }

    if (rtx_bytes > rx_bytes + tx_bytes) {
        rtx_bytes = 0;
    }

    if (ts.Minute % 2 == 0) {
        asyslog(LOG_INFO, "20分钟月流量统计，未统计流量%d", (rx_bytes + tx_bytes) - rtx_bytes);
        //跨日月流量分别清零
        if (localDay != ts.Day) {
            asyslog(LOG_INFO, "检测到夸日，流量统计清零，清零前数据(%d)", c2200.flow.day_tj);
            c2200.flow.day_tj = 0;
            localDay = ts.Day;
        }

        if (localMonth != ts.Month) {
            asyslog(LOG_INFO, "检测到夸月，流量统计清零，清零前数据(%d)", c2200.flow.month_tj);
            c2200.flow.month_tj = 0;
            localMonth = ts.Month;
        }
        c2200.flow.day_tj += (rx_bytes + tx_bytes) - rtx_bytes;
        c2200.flow.month_tj += (rx_bytes + tx_bytes) - rtx_bytes;

        saveVariData(0x2200, 0, &c2200, sizeof(c2200));
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
    shmm_unregister("ProgramInfo", sizeof(ProgramInfo));
    //关闭AT模块
    asyslog(LOG_INFO, "关闭AT模块电源");
    AT_POWOFF();
    asyslog(LOG_INFO, "通信模块退出完成...");
    exit(0);
}

int GetInterFaceIp(char *interface, char *ips) {
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

void WriteLinkRequest(INT8U link_type, INT16U heartbeat, LINK_Request *link_req) {
    TS ts = {};
    TSGet(&ts);
    link_req->type = link_type;
    link_req->piid_acd.data = 0;
    link_req->time.year = ((ts.Year << 8) & 0xff00) | ((ts.Year >> 8) & 0xff); // apdu 先高后低
    link_req->time.month = ts.Month;
    link_req->time.day_of_month = ts.Day;
    link_req->time.day_of_week = ts.Week;
    link_req->time.hour = ts.Hour;
    link_req->time.minute = ts.Minute;
    link_req->time.second = ts.Sec;
    link_req->time.milliseconds = 0;
    link_req->heartbeat = ((heartbeat << 8) & 0xff00) | ((heartbeat >> 8) & 0xff);
}

int Comm_task(CommBlock *compara) {
    INT16U heartbeat = (compara->Heartbeat == 0) ? 300 : compara->Heartbeat;

    if (abs(time(NULL) - compara->lasttime) < heartbeat) {
        return 0;
    }

    compara->lasttime = time(NULL);

    if (compara->testcounter > 3) {
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

void refreshComPara(CommBlock *compara) {
    compara->phy_connect_fd = -1;
    compara->testcounter = 0;
    compara->linkstate = close_connection;
    memset(compara->RecBuf, 0, sizeof(compara->RecBuf));
    memset(compara->SendBuf, 0, sizeof(compara->SendBuf));
    memset(compara->DealBuf, 0, sizeof(compara->DealBuf));
    compara->RHead = 0;
    compara->RTail = 0;
    compara->deal_step = 0;
    compara->rev_delay = 20;
    compara->shmem = JProgramInfo;
    compara->lasttime = 0;
}

void initComPara(CommBlock *compara, INT8S (*p_send)(int fd, INT8U *buf, INT16U len)) {
    CLASS_4001_4002_4003 c4001;
    memset(&c4001, 0x00, sizeof(c4001));
    readCoverClass(0x4001, 0, &c4001, sizeof(c4001), para_vari_save);
    memcpy(compara->serveraddr, c4001.curstom_num, 16);

    compara->phy_connect_fd = -1;
    compara->testcounter = 0;
    compara->linkstate = close_connection;
    memset(compara->RecBuf, 0, sizeof(compara->RecBuf));
    memset(compara->SendBuf, 0, sizeof(compara->SendBuf));
    memset(compara->DealBuf, 0, sizeof(compara->DealBuf));
    compara->RHead = 0;
    compara->RTail = 0;
    compara->deal_step = 0;
    compara->rev_delay = 20;
    compara->shmem = JProgramInfo;
    compara->p_send = p_send;
    compara->lasttime = 0;

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
    compara->myAppVar.server_deal_maxApdu = FRAMELEN;
    compara->myAppVar.server_recv_size = FRAMELEN;
    compara->myAppVar.server_send_size = FRAMELEN;
    compara->myAppVar.server_recv_maxWindow = 1;
    compara->myAppVar.expect_connect_timeout = 56400;

    readCoverClass(0xf101, 0, &compara->f101, sizeof(CLASS_F101), para_vari_save);
}

void dumpPeerStat(int fd, char *info) {
    int peerBuf[128];
    int port = 0;

    memset(peerBuf, 0x00, sizeof(peerBuf));
    anetTcpKeepAlive(NULL, fd);
    anetPeerToString(fd, peerBuf, sizeof(peerBuf), &port);
    asyslog(LOG_INFO, "[%s%s]:%d\n", info, peerBuf, port);
}

int netWatch(struct aeEventLoop *ep, long long id, void *clientData) {
    static int deadline = 0;
//    printf("deadline = %d\n", deadline);
    if(GetOnlineType() == 0){
        deadline ++;
    } else{
        deadline = 0;
    }

    if(deadline > 2 * 60 * 60){
        system("date >> /nand/reboot.log");
        sleep(1);
        system("reboot");
    }

    return 1000;
}


void createWatch(struct aeEventLoop *ep) {
    int code = aeCreateTimeEvent(ep, 1000, netWatch, NULL, NULL);
    asyslog(LOG_ERR, "注册在线监听程序[%d]\n", code);
}

/*********************************************************
 * 进程初始化
 *********************************************************/
void enviromentCheck(int argc, char *argv[]) {
    pid_t pids[128];
    if (prog_find_pid_by_name((INT8S *) argv[0], pids) > 1) {
        asyslog(LOG_ERR, "CJCOMM进程仍在运行,进程号[%d]，程序退出...", pids[0]);
        exit(0);
    }

    //绑定信号处理了函数
    struct sigaction sa = {};
    Setsig(&sa, QuitProcess);

    //向cjmain报告启动
    ProgIndex = atoi(argv[1]);
    JProgramInfo = OpenShMem("ProgramInfo", sizeof(ProgramInfo), NULL);
    memcpy(JProgramInfo->Projects[ProgIndex].ProjectName, "cjcomm", sizeof("cjcomm"));
    JProgramInfo->Projects[ProgIndex].ProjectID = getpid();
}

int main(int argc, char *argv[]) {
    printf("version 1019\n");

    memset(&class_4000, 0, sizeof(CLASS_4000));
    enviromentCheck(argc, argv);
    SetOnlineType(0);

    //开启网络IO事件处理框架
    aeEventLoop *ep;
    ep = aeCreateEventLoop(128);
    if (ep == NULL) {
        asyslog(LOG_ERR, "事件处理框架创建失败，程序终止。\n");
        exit(0);
    }

    StartIfr(ep, 0, NULL);
    StartSerial(ep, 0, NULL);

    StartServer(ep, 0, NULL);
    StartVerifiTime(ep, 0, JProgramInfo);
    StartClientForGprs(ep, 0, NULL);
    StartClientForNet(ep, 0, NULL);

//    StartClientOnModel(ep, 0, NULL);
    StartMmq(ep, 0, NULL);
    createWatch(ep);

    aeMain(ep);

    //退出信号
    QuitProcess(99);
    return EXIT_SUCCESS;
}
