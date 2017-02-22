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
static CommBlock FirObject;
static CommBlock ComObject;
static CommBlock nets_comstat;
static CommBlock serv_comstat;

static INT32S timeoffset[50]; //终端精准校时 默认最近心跳时间总个数50次
static INT8U crrntimen      = 0;  //终端精准校时 当前接手心跳数
static INT8U timeoffsetflag = 0; //终端精准校时 开始标志

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

    //关闭打开的接口
    asyslog(LOG_INFO, "开始关闭红外通信接口(%d)", FirObject.phy_connect_fd);
    close(FirObject.phy_connect_fd);
    asyslog(LOG_INFO, "开始关闭维护通信接口(%d)", ComObject.phy_connect_fd);
    close(ComObject.phy_connect_fd);
    asyslog(LOG_INFO, "开始关闭终端对主站链接接口(%d)", nets_comstat.phy_connect_fd);
    close(nets_comstat.phy_connect_fd);
    asyslog(LOG_INFO, "开始关闭主站对终端链接接口(%d)", serv_comstat.phy_connect_fd);
    close(serv_comstat.phy_connect_fd);

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

INT8S GenericWrite(int fd, INT8U* buf, INT16U len) {
    int ret = anetWrite(fd, buf, (int)len);
    if (ret != len) {
        asyslog(LOG_WARNING, "报文发送失败(长度:%d,错误:%d)", len, errno);
    }
    bufsyslog(buf, "Send:", len, 0, BUFLEN);
    return ret;
}

void GenericRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask) {
    CommBlock* nst = (CommBlock*)clientData;

    //判断fd中有多少需要接收的数据
    int revcount = 0;
    ioctl(nst->phy_connect_fd, FIONREAD, &revcount);

    //关闭异常端口
    if (revcount == 0) {
        aeDeleteFileEvent(eventLoop, nst->phy_connect_fd, AE_READABLE);
        close(nst->phy_connect_fd);
        nst->phy_connect_fd = -1;
    }

    if (revcount > 0) {
        for (int j = 0; j < revcount; j++) {
            read(nst->phy_connect_fd, &nst->RecBuf[nst->RHead], 1);
            nst->RHead = (nst->RHead + 1) % BUFLEN;
        }
        bufsyslog(nst->RecBuf, "Recv:", nst->RHead, nst->RTail, BUFLEN);

        for (int k = 0; k < 3; k++) {
            int len = 0;
            for (int i = 0; i < 5; i++) {
                len = StateProcess(&nst->deal_step, &nst->rev_delay, 10, &nst->RTail, &nst->RHead, nst->RecBuf, nst->DealBuf);
                if (len > 0) {
                    break;
                }
            }

            if (len > 0) {
                int apduType = ProcessData(nst);
                switch (apduType) {
                    case LINK_RESPONSE:
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

void initComPara(CommBlock* compara) {
	CLASS_4001_4002_4003 c4001;
	readCoverClass(0x4001, 0, &c4001, sizeof(c4001), para_vari_save);
	asyslog("逻辑地址长度：%d\n", c4001.curstom_num);
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

    compara->p_send = GenericWrite;
}

void deal_terminal_timeoffset() {
    TS jzqtime;
    TSGet(&jzqtime); //集中器时间

    //判断是否到开始记录心跳时间
    if (JProgramInfo->t_timeoffset.type == 1 && JProgramInfo->t_timeoffset.totalnum > 0) {
        //有效总个数是否小于主站设置得最少有效个数
        INT8U lastn = JProgramInfo->t_timeoffset.totalnum - JProgramInfo->t_timeoffset.maxn - JProgramInfo->t_timeoffset.minn;
        if (lastn < JProgramInfo->t_timeoffset.lastnum || lastn == 0)
            return;
        if (crrntimen < JProgramInfo->t_timeoffset.totalnum)
            timeoffsetflag = 1; //开始标志 记录心跳
        else
            timeoffsetflag = 0;
        //判断是否需要计算相关偏差值  记录完毕 达到主站设置得最大记录数
        if (timeoffsetflag == 0 && crrntimen == JProgramInfo->t_timeoffset.totalnum) {
            getarryb2s(timeoffset, crrntimen);
            INT32S allk = 0;
            INT8U index = 0;
            for (index = JProgramInfo->t_timeoffset.maxn - 1; index < (crrntimen - JProgramInfo->t_timeoffset.minn); index++) {
                allk += timeoffset[index];
            }
            INT32S avg = allk / lastn;
            //如果平均偏差值大于等于主站设置得阀值进行精确校时
            if (abs(avg) >= JProgramInfo->t_timeoffset.timeoffset) {
                TSGet(&jzqtime); //集中器时间
                tminc(&jzqtime, sec_units, avg);
                DateTimeBCD DT;
                DT.year.data  = jzqtime.Year;
                DT.month.data = jzqtime.Month;
                DT.day.data   = jzqtime.Day;
                DT.hour.data  = jzqtime.Hour;
                DT.min.data   = jzqtime.Minute;
                DT.sec.data   = jzqtime.Sec;
                setsystime(DT);
                //产生对时事件
                Event_3114(DT, JProgramInfo);
            }
            memset(timeoffset, 0, 50);
            crrntimen      = 0;
            timeoffsetflag = 0;
        }
    }
}

void Getk(LINK_Response link) {
    TS T4;
    TSGet(&T4); //集中器接收时间
    TS T1;
    TS T2;
    TS T3;
    T1.Year   = link.request_time.year;
    T1.Month  = link.request_time.month;
    T1.Day    = link.request_time.day_of_month;
    T1.Hour   = link.request_time.hour;
    T1.Minute = link.request_time.minute;
    T1.Sec    = link.request_time.second;
    T1.Week   = link.request_time.day_of_week;

    T2.Year   = link.reached_time.year;
    T2.Month  = link.reached_time.month;
    T2.Day    = link.reached_time.day_of_month;
    T2.Hour   = link.reached_time.hour;
    T2.Minute = link.reached_time.minute;
    T2.Sec    = link.reached_time.second;
    T2.Week   = link.reached_time.day_of_week;

    T3.Year   = link.response_time.year;
    T3.Month  = link.response_time.month;
    T3.Day    = link.response_time.day_of_month;
    T3.Hour   = link.response_time.hour;
    T3.Minute = link.response_time.minute;
    T3.Sec    = link.response_time.second;
    T3.Week   = link.response_time.day_of_week;

    INT32S U              = difftime(tmtotime_t(T2), tmtotime_t(T1));
    INT32S V              = difftime(tmtotime_t(T4), tmtotime_t(T3));
    INT32S K              = (U - V) / 2;
    timeoffset[crrntimen] = K;
    crrntimen++;
    if (crrntimen == JProgramInfo->t_timeoffset.totalnum)
        timeoffsetflag = 0;
}

void NETRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask) {
    CommBlock* nst = (CommBlock*)clientData;

    //判断fd中有多少需要接收的数据
    int revcount = 0;
    ioctl(fd , FIONREAD, &revcount);

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
                len = StateProcess(&nst->deal_step, &nst->rev_delay, 10, &nst->RTail, &nst->RHead, nst->RecBuf, nst->DealBuf);
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
                        if (timeoffsetflag == 1) {
                            Getk(nst->linkResponse);
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

    printf("nst->phy_connect_fd %d\n", nst->phy_connect_fd);

    if (nst->phy_connect_fd <= 0) {
        initComPara(nst);
        nst->phy_connect_fd = anetTcpConnect(NULL, IPaddr, Class25.master.master[0].port);
        if (nst->phy_connect_fd > 0) {
            asyslog(LOG_INFO, "链接主站(主站地址:%s,结果:%d)", IPaddr, nst->phy_connect_fd);
            if (aeCreateFileEvent(ep, nst->phy_connect_fd, AE_READABLE, NETRead, nst) < 0) {
                close(nst->phy_connect_fd);
                nst->phy_connect_fd = -1;
            } else {
            	dealinit();
                anetTcpKeepAlive(NULL, nst->phy_connect_fd);
                SetOnline();
                asyslog(LOG_INFO, "与主站链路建立成功");
            }
        }
    } else {
        TS ts = {};
        TSGet(&ts);
        Comm_task(nst);
        deal_terminal_timeoffset();
    	deal_vsms();
    }

    /*
     * 检查红外与维护串口通信状况。
     */
    if (FirObject.phy_connect_fd < 0) {
        if ((FirObject.phy_connect_fd = OpenCom(3, 2400, (unsigned char*)"even", 1, 8)) <= 0) {
            asyslog(LOG_ERR, "红外串口打开失败");
        } else {
            if (aeCreateFileEvent(ep, FirObject.phy_connect_fd, AE_READABLE, GenericRead, &FirObject) < 0) {
                asyslog(LOG_ERR, "红外串口监听失败");
                close(FirObject.phy_connect_fd);
                FirObject.phy_connect_fd = -1;
            }
        }
    }

    if (ComObject.phy_connect_fd < 0) {
        if ((ComObject.phy_connect_fd = OpenCom(4, 9600, (unsigned char*)"even", 1, 8)) <= 0) {
            asyslog(LOG_ERR, "维护串口打开失败");
        } else {
            if (aeCreateFileEvent(ep, ComObject.phy_connect_fd, AE_READABLE, GenericRead, &ComObject) < 0) {
                asyslog(LOG_ERR, "维护串口监听失败");
                close(ComObject.phy_connect_fd);
                ComObject.phy_connect_fd = -1;
            }
        }
    }

    return 2000;
}

void CreateAptSer(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask) {
    CommBlock* nst = (CommBlock*)clientData;

    //如果已经接受主站连接，先关闭
    if (nst->phy_connect_fd > 0) {
        aeDeleteFileEvent(eventLoop, nst->phy_connect_fd, AE_READABLE);
        close(nst->phy_connect_fd);
        nst->phy_connect_fd = -1;
    }

    //如果成功建立主站连接，则创建IO事件；否则重新建立监听
    nst->phy_connect_fd = anetTcpAccept(NULL, fd, NULL, 0, NULL);
    if (nst->phy_connect_fd > 0) {
        asyslog(LOG_INFO, "建立主站反向链接(结果:%d)", nst->phy_connect_fd);
        if (aeCreateFileEvent(eventLoop, nst->phy_connect_fd, AE_READABLE, NETRead, nst) == -1) {
            aeDeleteFileEvent(eventLoop, nst->phy_connect_fd, AE_READABLE);
            close(fd);
        }
    } else {
        asyslog(LOG_WARNING, "监听端口出错，尝试重建监听");
        aeDeleteFileEvent(eventLoop, nst->phy_connect_fd, AE_READABLE);
        close(fd);
        int listen_port = anetTcpServer(NULL, 5555, "0.0.0.0", 1);
        if (listen_port > 0) {
            nst->phy_connect_fd = listen_port;
            aeCreateFileEvent(eventLoop, listen_port, AE_READABLE, CreateAptSer, nst);
        } else {
            asyslog(LOG_ERR, "监听端口出错，重新建立监听失败");
            abort();
        }
    }
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
    asyslog(LOG_INFO, "主站通信地址(1)为：%d.%d.%d.%d:%d", Class25.master.master[0].ip[1], Class25.master.master[0].ip[2],Class25.master.master[0].ip[3],Class25.master.master[0].ip[4],Class25.master.master[0].port);
    asyslog(LOG_INFO, "主站通信地址(2)为：%d.%d.%d.%d:%d", Class25.master.master[1].ip[1], Class25.master.master[1].ip[2],Class25.master.master[1].ip[3],Class25.master.master[1].ip[4],Class25.master.master[1].port);

    //检查运行参数
    if (argc <= 2) {
    	memset(IPaddr, 0, sizeof(IPaddr));
        snprintf(IPaddr, sizeof(IPaddr), "%d.%d.%d.%d", Class25.master.master[0].ip[1], Class25.master.master[0].ip[2],Class25.master.master[0].ip[3],Class25.master.master[0].ip[4]);
        asyslog(LOG_INFO, "确定：主站通信地址为：%s\n", IPaddr);
    }else{
        //解析通信参数
        memset(IPaddr, 0, sizeof(IPaddr));
        memcpy(IPaddr, argv[1], strlen(argv[1]));
        asyslog(LOG_INFO, "确定：主站通信地址为：%s\n", IPaddr);
    }

    //向cjmain报告启动
    JProgramInfo = OpenShMem("ProgramInfo", sizeof(ProgramInfo), NULL);
    memcpy(JProgramInfo->Projects[1].ProjectName, "cjcomm", sizeof("cjcomm"));
    JProgramInfo->Projects[1].ProjectID = getpid();

    //初始化通信对象参数
    initComPara(&FirObject);
    initComPara(&ComObject);
    initComPara(&nets_comstat);
    initComPara(&serv_comstat);
}

int main(int argc, char* argv[]) {
	printf("version 1012\n");
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

    //建立服务端侦听
    int listen_port = anetTcpServer(NULL, 5555, "0.0.0.0", 1);
    if (listen_port >= 0) {
        aeCreateFileEvent(ep, listen_port, AE_READABLE, CreateAptSer, &serv_comstat);
    }

    //建立网络连接（以太网、GPRS）维护事件
    aeCreateTimeEvent(ep, 1 * 1000, NETWorker, &nets_comstat, NULL);
    aeMain(ep);

    QuitProcess(&JProgramInfo->Projects[3]);
    return EXIT_SUCCESS;
}
