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

#include "ae.h"
#include "anet.h"
#include "assert.h"
#include "at.h"
#include "rlog.h"

#include "AccessFun.h"
#include "PublicFunction.h"
#include "cjcomm.h"
#include "dlt698def.h"
#include "event.h"

//共享内存地址
static ProgramInfo* JProgramInfo = NULL;

static char IPaddr[24];
//以太网、GPRS、侦听服务端处理对象
static CommBlock nets_comstat;


static CLASS25 Class25;

void saveCurrClass25(void) {
    saveCoverClass(0x4500, 0, (void*)&Class25, sizeof(CLASS25), para_init_save);
}

void setCCID(INT8U CCID[]) {
    memcpy(Class25.ccid, CCID, 20);
}

void setIMSI(INT8U IMSI[]) {
    memcpy(Class25.imsi, IMSI, 20);
}

void setSINSTR(INT16U SINSTR) {
    Class25.signalStrength = SINSTR;
}

void setPPPIP(INT8U PPPIP[]) {
    memcpy(Class25.pppip, PPPIP, 20);
}

void clearcount(int index) {
    JProgramInfo->Projects[index].WaitTimes = 0;
}

void QuitProcess(int sig) {
    asyslog(LOG_INFO, "通信模块退出,收到信号类型(%d)", sig);
    IfrDestory();
    SerialDestory();
    ServerDestory();

    //关闭打开的接口
    asyslog(LOG_INFO, "开始关闭终端对主站链接接口(%d)", nets_comstat.phy_connect_fd);
    close(nets_comstat.phy_connect_fd);

    //关闭AT模块
    asyslog(LOG_INFO, "关闭AT模块电源");
    AT_POWOFF();

    //等待处理完成
    asyslog(LOG_INFO, "等待(2秒)清理工作完成……");
    sleep(2);

    exit(0);
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

void Comm_task(CommBlock* compara) {
    int sendlen           = 0;
    static time_t oldtime = 0;
    TS ts                 = {};
    INT16U heartbeat      = 60;

    time_t newtime = time(NULL);
    if (abs(newtime - oldtime) > heartbeat) {
        TSGet(&ts);
        oldtime = newtime;
        if (compara->testcounter >= 2) {
            close(compara->phy_connect_fd);
            compara->phy_connect_fd = -1;
            SetOffline();
            AT_POWOFF();
            compara->testcounter = 0;
            return;
        } else if (compara->linkstate == close_connection) //物理通道建立完成后，如果请求状态为close，则需要建立连接
        {
            WriteLinkRequest(build_connection, heartbeat, &compara->link_request);
            asyslog(LOG_INFO, "建立链接 %02d-%02d-%02d %d:%d:%d\n", compara->link_request.time.year,
                    compara->link_request.time.month, compara->link_request.time.day_of_month, compara->link_request.time.hour,
                    compara->link_request.time.minute, compara->link_request.time.second);
            sendlen = Link_Request(compara->link_request, compara->serveraddr, compara->SendBuf);
        } else {
            WriteLinkRequest(heart_beat, heartbeat, &compara->link_request);
            sendlen = Link_Request(compara->link_request, compara->serveraddr, compara->SendBuf);
        }
        compara->p_send(compara->phy_connect_fd, compara->SendBuf, sendlen);
        compara->testcounter++;
    }
}


void initComPara(CommBlock* compara) {
    int ret = 0, i = 0;
    CLASS19 oi4300    = {};
    CLASS_F101 oif101 = {};
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
    compara->p_send    = GenericWrite;

    memset(&oi4300, 0, sizeof(CLASS19));
    ret = readCoverClass(0x4300, 0, &oi4300, sizeof(CLASS19), para_vari_save);
    if (ret)
        memcpy(&compara->myAppVar.server_factory_version, &oi4300.info, sizeof(FactoryVersion));
    for (i                                       = 0; i < 2; i++)
        compara->myAppVar.FunctionConformance[i] = 0xff;
    for (i                                       = 0; i < 5; i++)
        compara->myAppVar.ProtocolConformance[i] = 0xff;
    compara->myAppVar.server_deal_maxApdu        = 1024;
    compara->myAppVar.server_recv_size           = 1024;
    compara->myAppVar.server_send_size           = 1024;
    compara->myAppVar.server_recv_maxWindow      = 1;
    compara->myAppVar.expect_connect_timeout     = 56400;
    //--------------------
    memset(&oif101, 0, sizeof(CLASS_F101));
    ret = readCoverClass(0xf101, 0, &oif101, sizeof(CLASS_F101), para_vari_save);
    memcpy(&compara->f101, &oif101, sizeof(CLASS_F101));
}


void NETRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask) {
    CommBlock* nst = (CommBlock*)clientData;

    //判断fd中有多少需要接收的数据
    int revcount = 0;
    ioctl(fd, FIONREAD, &revcount);

    //关闭异常端口
    if (revcount == 0) {
        asyslog(LOG_WARNING, "链接出现异常，关闭端口、取消监听");
        aeDeleteFileEvent(eventLoop, fd, AE_READABLE);
        close(fd);
        nst->phy_connect_fd = -1;
        SetOffline();
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
                switch (apduType) {
                    case LINK_RESPONSE:
                        if (GetTimeOffsetFlag() == 1) {
                            Getk(nst->linkResponse, JProgramInfo);
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

int NETWorker(struct aeEventLoop* ep, long long id, void* clientData) {
    CommBlock* nst = (CommBlock*)clientData;
    clearcount(1);

    if (nst->phy_connect_fd <= 0) {
        char errmsg[256];
        memset(errmsg, 0x00, sizeof(errmsg));
        initComPara(nst);
        nst->phy_connect_fd = anetTcpConnect(errmsg, IPaddr, Class25.master.master[0].port);
        if (nst->phy_connect_fd > 0) {
//            asyslog(LOG_INFO, "链接主站(主站地址:%s,结果:%d)", IPaddr, nst->phy_connect_fd);
            if (aeCreateFileEvent(ep, nst->phy_connect_fd, AE_READABLE, NETRead, nst) < 0) {
                close(nst->phy_connect_fd);
                nst->phy_connect_fd = -1;
            } else {
                anetTcpKeepAlive(NULL, nst->phy_connect_fd);
                SetOnline();
                asyslog(LOG_INFO, "与主站链路建立成功");
                gpofun("/dev/gpoONLINE_LED", 1);
            }
        } else {
//            asyslog(LOG_WARNING, "主站链接失败，(%d)[%s]", nst->phy_connect_fd, errmsg);
        }
    } else {
        TS ts = {};
        TSGet(&ts);
        Comm_task(nst);
    }

    return 2000;
}

/*********************************************************
 * 进程初始化
 *********************************************************/
void enviromentCheck(int argc, char* argv[]) {
    //绑定信号处理了函数
    struct sigaction sa = {};
    Setsig(&sa, QuitProcess);

    //读取设备参数
    readCoverClass(0x4500, 0, (void*)&Class25, sizeof(CLASS25), para_vari_save);
    asyslog(LOG_INFO, "工作模式 enum{混合模式(0),客户机模式(1),服务器模式(2)}：%d", Class25.commconfig.workModel);
    asyslog(LOG_INFO, "在线方式 enum{永久在线(0),被动激活(1)}：%d", Class25.commconfig.onlineType);
    asyslog(LOG_INFO, "连接方式 enum{TCP(0),UDP(1)}：%d", Class25.commconfig.connectType);
    asyslog(LOG_INFO, "连接应用方式 enum{主备模式(0),多连接模式(1)}：%d", Class25.commconfig.appConnectType);
    asyslog(LOG_INFO, "侦听端口列表：%d", Class25.commconfig.listenPort[0]);
    asyslog(LOG_INFO, "超时时间，重发次数：%02x", Class25.commconfig.timeoutRtry);
    asyslog(LOG_INFO, "心跳周期秒：%d", Class25.commconfig.heartBeat);
    asyslog(LOG_INFO, "主站通信地址(1)为：%d.%d.%d.%d:%d", Class25.master.master[0].ip[1], Class25.master.master[0].ip[2],
            Class25.master.master[0].ip[3], Class25.master.master[0].ip[4], Class25.master.master[0].port);
    asyslog(LOG_INFO, "主站通信地址(2)为：%d.%d.%d.%d:%d", Class25.master.master[1].ip[1], Class25.master.master[1].ip[2],
            Class25.master.master[1].ip[3], Class25.master.master[1].ip[4], Class25.master.master[1].port);

    //检查运行参数
    memset(IPaddr, 0, sizeof(IPaddr));
    snprintf(IPaddr, sizeof(IPaddr), "%d.%d.%d.%d", Class25.master.master[0].ip[1], Class25.master.master[0].ip[2],
             Class25.master.master[0].ip[3], Class25.master.master[0].ip[4]);
    asyslog(LOG_INFO, "确定：主站通信地址为：%s\n", IPaddr);

    //向cjmain报告启动
    JProgramInfo = OpenShMem("ProgramInfo", sizeof(ProgramInfo), NULL);
    memcpy(JProgramInfo->Projects[1].ProjectName, "cjcomm", sizeof("cjcomm"));
    JProgramInfo->Projects[1].ProjectID = getpid();

    //初始化通信对象参数
    initComPara(&nets_comstat);
}



int main(int argc, char* argv[]) {
    printf("version 1015\n");
    pid_t pids[128];
    if (prog_find_pid_by_name((INT8S*)argv[0], pids) > 1)
        return EXIT_SUCCESS;
    // daemon(0,0);

    enviromentCheck(argc, argv);
    //开始通信模块维护、红外与维护串口线程
    CreateATWorker();
    //开启网络IO事件处理框架
    aeEventLoop* ep;
    ep = aeCreateEventLoop(128);
    if (ep == NULL) {
        asyslog(LOG_ERR, "事件循环创建失败，程序终止。\n");
        exit(0);
    }

    StartIfr(ep, 0, NULL);
    StartSerial(ep, 0, NULL);
    StartServer(ep, 0, NULL);
    StartVerifiTime(ep, 0, JProgramInfo);
    StartMmq(ep, 0, &nets_comstat);


    //建立网络连接（以太网、GPRS）维护事件
    aeCreateTimeEvent(ep, 1 * 1000, NETWorker, &nets_comstat, NULL);
    aeMain(ep);

    QuitProcess(&JProgramInfo->Projects[1]);
    return EXIT_SUCCESS;
}
