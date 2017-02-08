#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "ae.h"
#include "anet.h"
#include "rlog.h"
#include "at.h"
#include "assert.h"

#include "PublicFunction.h"
#include "dlt698def.h"
#include "cjcomm.h"

ProgramInfo* JProgramInfo = NULL;
static char IPaddr[24];
//以太网、GPRS、侦听服务端处理对象
static CommBlock FirObject;
static CommBlock ComObject;
static CommBlock nets_comstat;
static CommBlock serv_comstat;

void clearcount(int index) {
    JProgramInfo->Projects[index].WaitTimes = 0;
}

void QuitProcess(int sig) {
    asyslog(LOG_INFO, "通信模块退出,收到信号类型(%d)", sig);

    //关闭打开的服务
    asyslog(LOG_INFO, "开始关闭外部服务（gtpget、ppp-off、gsmMuxd）");
    system("pkill ftpget");
    system("ppp-off");
    system("pkill gsmMuxd");

    //关闭打开的接口
    asyslog(LOG_INFO, "开始关闭红外通信接口(%d)",FirObject.phy_connect_fd);
    close(FirObject.phy_connect_fd);
    asyslog(LOG_INFO, "开始关闭维护通信接口(%d)",ComObject.phy_connect_fd);
    close(ComObject.phy_connect_fd);
    asyslog(LOG_INFO, "开始关闭终端对主站链接接口(%d)",nets_comstat.phy_connect_fd);
    close(nets_comstat.phy_connect_fd);
    asyslog(LOG_INFO, "开始关闭主站对终端链接接口(%d)" ,serv_comstat.phy_connect_fd);
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
        if (compara->phy_connect_fd < 0 || compara->testcounter >= 2) {
            fprintf(stderr, "phy_connect_fd = %d ,compara->testcounter = %d\n", compara->phy_connect_fd, compara->testcounter);
//            initComPara(compara);
            return;
        } else if (compara->phy_connect_fd >= 0 && compara->linkstate == close_connection) //物理通道建立完成后，如果请求状态为close，则需要建立连接
        {
            WriteLinkRequest(build_connection, heartbeat, &compara->link_request);
            fprintf(stderr, "link %d-%0d-%d %d:%d:%d", compara->link_request.time.year, compara->link_request.time.month,
                    compara->link_request.time.day_of_month, compara->link_request.time.hour, compara->link_request.time.minute,
                    compara->link_request.time.second);
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
    int j = 0;
    fprintf(stderr, "Generic Send:\n");
    for (j = 0; j < len; j++) {
        fprintf(stderr, "%02x ", buf[j]);
    }
    printf("\n");
    return anetWrite(fd, buf, (int)len);
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
            printf("%02x ", nst->RecBuf[nst->RHead]);
            nst->RHead = (nst->RHead + 1) % BUFLEN;
        }

        int len = 0;
        for (int i = 0; i < 5; i++) {
            len = StateProcess(&nst->deal_step, &nst->rev_delay, 10, &nst->RTail, &nst->RHead, nst->RecBuf, nst->DealBuf);
            printf("len = %d\n", len);
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

void initComPara(CommBlock* compara) {
    INT8U addr[16] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    memcpy(compara->serveraddr, addr, 16);
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

    compara->p_send = GenericWrite;
}

void NETRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask) {
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
        bufsyslog(nst->RecBuf, nst->RHead, nst->RTail, revcount);

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


int NETWorker(struct aeEventLoop* ep, long long id, void* clientData) {
    CommBlock* nst = (CommBlock*)clientData;

    if (nst->phy_connect_fd <= 0) {
        initComPara(nst);
        fprintf(stderr, "IPaddr:%s\n", IPaddr);
        nst->phy_connect_fd = anetTcpConnect(NULL, IPaddr, 5022);
        fprintf(stderr, "[NETWorke1r]Connect Server(%d)[%s-%d]\n", nst->phy_connect_fd, __FILE__, __LINE__);
        if (nst->phy_connect_fd > 0) {
            fprintf(stderr, "[NETWorker]Connect Server(%d)\n", nst->phy_connect_fd);
            if (aeCreateFileEvent(ep, nst->phy_connect_fd, AE_READABLE, NETRead, nst) < 0) {
                close(nst->phy_connect_fd);
                nst->phy_connect_fd = -1;
            }
        }
    } else {
        TS ts = {};
        TSGet(&ts);
        Comm_task(nst);
    }

    return 200;
    /*
     * 检查红外与维护串口通信状况。
     */
    if(FirObject.phy_connect_fd < 0){
		if ((FirObject.phy_connect_fd = OpenCom(3, 2400, (unsigned char*)"none", 1, 8)) <= 0) {
			asyslog(LOG_ERR, "红外口打开失败");
		}
		else
		{
			if (aeCreateFileEvent(ep, FirObject.phy_connect_fd, AE_READABLE, GenericRead, &FirObject) < 0) {
				asyslog(LOG_ERR, "红外串口监听失败 %d", FirObject.phy_connect_fd);
				close(FirObject.phy_connect_fd);
				FirObject.phy_connect_fd = -1;
			}
		}
    }

    if(ComObject.phy_connect_fd < 0){
		if ((ComObject.phy_connect_fd = OpenCom(4, 2400, (unsigned char*)"none", 1, 8)) <= 0) {
			asyslog(LOG_ERR, "维护串口打开失败");
		}
		else
		{
			if (aeCreateFileEvent(ep, FirObject.phy_connect_fd, AE_READABLE, GenericRead, &ComObject) < 0) {
				asyslog(LOG_ERR, "维护串口监听失败 %d", FirObject.phy_connect_fd);
				close(FirObject.phy_connect_fd);
				FirObject.phy_connect_fd = -1;
			}
		}
    }

    return 200;
}

void CreateATWorker(void) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_t temp_key;
    pthread_create(&temp_key, &attr, ATWorker, NULL);
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
        fprintf(stderr, "[vmsgr] 建立主站反向链接。fd = %d\n", nst->phy_connect_fd);
        if (aeCreateFileEvent(eventLoop, nst->phy_connect_fd, AE_READABLE, NETRead, nst) == -1) {
            close(fd);
        }
    } else {
        int listen_port = anetTcpServer(NULL, 5555, "0.0.0.0", 1);
        aeCreateFileEvent(eventLoop, listen_port, AE_READABLE, CreateAptSer, nst);
    }
}

/*********************************************************
 * 进程初始化
 *********************************************************/
void enviromentCheck(int argc, char* argv[])
{
	//检查运行参数
	if(argc < 2){
		asyslog(LOG_ERR, "未指定通信参数");
		exit(1);
	}

	//解析通信参数
    memset(IPaddr, 0, sizeof(IPaddr));
    memcpy(IPaddr, argv[1], strlen(argv[1]));
    asyslog("主站通信地址为：%s\n", IPaddr);

    //向cjmain报告启动
    JProgramInfo = OpenShMem("ProgramInfo", sizeof(ProgramInfo), NULL);
    memcpy(JProgramInfo->Projects[3].ProjectName, "cjcomm", sizeof("cjcomm"));
    JProgramInfo->Projects[3].ProjectID = getpid();

    struct sigaction sa = {};
    Setsig(&sa, QuitProcess);


    //初始化通信对象参数
    initComPara(&FirObject);
    initComPara(&ComObject);
    initComPara(&nets_comstat);
    initComPara(&serv_comstat);
}

int main(int argc, char* argv[]) {

	//daemon(0,0);

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
