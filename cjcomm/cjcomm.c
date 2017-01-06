#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

#include "ae.h"
#include "anet.h"
#include "rlog.h"

#include "option.h"
#include "PublicFunction.h"
#include "cjcomm.h"

ProgramInfo* JProgramInfo = NULL;

void clearcount(int index) {
    JProgramInfo->Projects[index].WaitTimes = 0;
}

void QuitProcess(ProjectInfo* proinfo) {
    rlog("EXIT：%s %d\n", "cjcomm", JProgramInfo->Projects[3].ProjectID);
    exit(0);
}
/*********************************************************
 * 进程初始化
 *********************************************************/
int InitPro(int argc, char* argv[]) {
    if (argc >= 2) {
        JProgramInfo = OpenShMem("ProgramInfo", sizeof(ProgramInfo), NULL);
        memcpy(JProgramInfo->Projects[3].ProjectName, "cjcomm", sizeof("cjcomm"));
        JProgramInfo->Projects[3].ProjectID = getpid();
        return 1;
    }
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
            initComPara(compara);
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

INT8S ComWrite(int fd, INT8U* buf, INT16U len) {
    return write(fd, buf, len);
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

    compara->p_send   = ComWrite;
    compara->p_recv   = NULL;
    compara->p_connet = NULL;
}

void* FirComWorker(void* args) {
    CommBlock fir_comstat;
    CommBlock com_comstat;

    fir_comstat.p_send = ComWrite;
    com_comstat.p_send = ComWrite;

    initComPara(&fir_comstat);
    initComPara(&com_comstat);

    if ((fir_comstat.phy_connect_fd = OpenCom(3, 2400, (unsigned char*)"none", 1, 8)) <= 0) {
        rlog("[FirComWorker] Open Fir FAILED.\n");
        return NULL;
    }

    if ((com_comstat.phy_connect_fd = OpenCom(4, 2400, (unsigned char*)"none", 1, 8)) <= 0) {
        rlog("[FirComWorker] Open Com FAILED.\n");
        return NULL;
    }

    while (1) {
        sleep(1);
        int revcount = 0;
        ioctl(fir_comstat.phy_connect_fd, FIONREAD, &revcount);
        if (revcount > 0) {
            for (int j = 0; j < revcount; j++) {
                read(fir_comstat.phy_connect_fd, &fir_comstat.RecBuf[fir_comstat.RHead], 1);
                fir_comstat.RHead = (fir_comstat.RHead + 1) % BUFLEN;
            }

            int len =
            StateProcess(&comstat.deal_step, &comstat.rev_delay, 10, &comstat.RTail, &comstat.RHead, comstat.RecBuf, comstat.DealBuf);
            if (len > 0) {
                int apduType = ProcessData(&comstat);
                switch (apduType) {
                    case LINK_RESPONSE:
                        comstat.linkstate   = build_connection;
                        comstat.testcounter = 0;
                        break;
                    default:
                        break;
                }
            }
        }
        revcount = 0;
        ioctl(com_comstat.phy_connect_fd, FIONREAD, &revcount);

        if (revcount > 0) {
            for (int j = 0; j < revcount; j++) {
                read(com_comstat.phy_connect_fd, &com_comstat.RecBuf[com_comstat.RHead], 1);
                com_comstat.RHead = (com_comstat.RHead + 1) % BUFLEN;
            }

            int len =
            StateProcess(&comstat.deal_step, &comstat.rev_delay, 10, &comstat.RTail, &comstat.RHead, comstat.RecBuf, comstat.DealBuf);
            if (len > 0) {
                int apduType = ProcessData(&comstat);
                switch (apduType) {
                    case LINK_RESPONSE:
                        comstat.linkstate   = build_connection;
                        comstat.testcounter = 0;
                        break;
                    default:
                        break;
                }
            }
        }
    }

    return NULL;
}

void CreateFirComWorker(void) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_t temp_key;
    pthread_create(&temp_key, &attr, FirComWorker, NULL);
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
            printf("%02x\n", nst->RecBuf[nst->RHead]);
            nst->RHead = (nst->RHead + 1) % BUFLEN;
        }

        int len =
        StateProcess(&comstat.deal_step, &comstat.rev_delay, 10, &comstat.RTail, &comstat.RHead, comstat.RecBuf, comstat.DealBuf);
        if (len > 0) {
            int apduType = ProcessData(&comstat);
            switch (apduType) {
                case LINK_RESPONSE:
                    comstat.linkstate   = build_connection;
                    comstat.testcounter = 0;
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
        nst->phy_connect_fd = anetTcpConnect(NULL, "192.168.0.159", 5022);
        if (nst->phy_connect_fd > 0) {
            rlog("[NETWorker]Connect Server(%d)\n", nst->phy_connect_fd);
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
}

int GPRSWorker(struct aeEventLoop* ep, long long id, void* clientData) {
    CommBlock* nst = (CommBlock*)clientData;

    if (nst->phy_connect_fd <= 0) {
        initComPara(nst);
        nst->phy_connect_fd = anetTcpConnect(NULL, "192.168.0.159", 5022);
    } else {
        TS ts = {};
        TSGet(&ts);
        Comm_task(&comstat);
    }

    return 200;
}

void* ATWorker(void* args) {
    gpofun("/dev/gpoGPRS_POWER", 1);
    sleep(3);
    gpofun("/dev/gpoGPRS_POWER", 0);
    sleep(3);
    gpofun("/dev/gpoGPRS_POWER", 1);
    sleep(3);

    gpofun("/dev/gpoGPRS_SWITCH", 1);
    usleep(200 * 1000);
    gpofun("/dev/gpoGPRS_SWITCH", 0);
    sleep(1);
    gpofun("/dev/gpoGPRS_SWITCH", 1);
    usleep(500 * 1000);

    gpofun("/dev/gpoGPRS_RST", 1);
    sleep(1);
    gpofun("/dev/gpoGPRS_RST", 0);
    sleep(1);
    gpofun("/dev/gpoGPRS_RST", 1);
    sleep(1);

    int fd = OpenCom(3, 2400, (unsigned char*)"none", 1, 8);
    if (fd < 0) {
        fprintf(stderr, "[vMsgr]AT检查失败(端口创建失败)。\n");
    }
    return NULL;
}

void CreateATWorker(void) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_t temp_key;
    pthread_create(&temp_key, &attr, ATWorker, NULL);
}

int main(int argc, char* argv[]) {
    INT8U apduType      = 0;
    TS ts               = {};
    struct sigaction sa = {};

    if (access("/nand/mlog", F_OK) == -1) {
        system("mkdir /nand/mlog");
    }

    if (InitPro(argc, argv) == 0) {
        fprintf(stderr, "进程 %s 参数错误\n", argv[0]);
        return EXIT_FAILURE;
    }
    Setsig(&sa, QuitProcess);

    CreateFirComWorker();
    initComPara(&comstat);

    aeEventLoop* ep;
    ep = aeCreateEventLoop(128);
    if (ep == NULL) {
        rlog("[main]事件循环创建失败，程序终止。\n");
    }

    CommBlock net_comstat;
    CommBlock gprs_comstat;

    net_comstat.p_send  = anetWrite;
    gprs_comstat.p_send = anetWrite;

    aeCreateTimeEvent(ep, 1 * 1000, NETWorker, &net_comstat, NULL);
    //    aeCreateTimeEvent(ep, 1 * 1000, GPRSWorker, &net_comstat, NULL);
    aeMain(ep);

    QuitProcess(&JProgramInfo->Projects[3]);
    return EXIT_SUCCESS;
}
