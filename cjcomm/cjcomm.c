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

#include "option.h"
#include "PublicFunction.h"
#include "cjcomm.h"

#include "ObjectAction.h"
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
            StateProcess(&fir_comstat.deal_step, &fir_comstat.rev_delay, 10, &fir_comstat.RTail, &fir_comstat.RHead, fir_comstat.RecBuf, fir_comstat.DealBuf);
            if (len > 0) {
                int apduType = ProcessData(&fir_comstat);
                switch (apduType) {
                    case LINK_RESPONSE:
                    	fir_comstat.linkstate   = build_connection;
                    	fir_comstat.testcounter = 0;
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
            StateProcess(&com_comstat.deal_step, &com_comstat.rev_delay, 10, &com_comstat.RTail, &com_comstat.RHead, com_comstat.RecBuf, com_comstat.DealBuf);
            if (len > 0) {
                int apduType = ProcessData(&com_comstat);
                switch (apduType) {
                    case LINK_RESPONSE:
                    	com_comstat.linkstate   = build_connection;
                    	com_comstat.testcounter = 0;
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
        printf("NET RECV:\n");
        for (int j = 0; j < revcount; j++) {
            read(nst->phy_connect_fd, &nst->RecBuf[nst->RHead], 1);
            printf("%02x ", nst->RecBuf[nst->RHead]);
            nst->RHead = (nst->RHead + 1) % BUFLEN;
        }
        printf("\n");

        int len =
        StateProcess(&nst->deal_step, &nst->rev_delay, 10, &nst->RTail, &nst->RHead, nst->RecBuf, nst->DealBuf);
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
        Comm_task(nst);
    }

    return 200;
}

int gpofun(char* devname, int data) {
    int fd = -1;
    if ((fd = open(devname, O_RDWR | O_NDELAY)) >= 0) {
        write(fd, &data, sizeof(int));
        close(fd);
        return 1;
    }
    return 0;
}

int OpenMuxCom(INT8U port, int baud, unsigned char* par, unsigned char stopb, INT8U bits) {
    int Com_Port = 0;
    struct termios old_termi, new_termi;
    int baud_lnx = 0;
    unsigned char tmp[128];
    memset(tmp, 0, 128);
    sprintf((char*)tmp, "/dev/mux%d", port);

    Com_Port = open((char*)tmp, O_RDWR | O_NOCTTY); /* 打开串口文件 */
    if (Com_Port < 0) {
        fprintf(stderr, "open the serial port fail! errno is: %d\n", errno);
        return -1; /*打开串口失败*/
    }
    if (tcgetattr(Com_Port, &old_termi) != 0) /*存储原来的设置*/
    {
        fprintf(stderr, "get the terminal parameter error when set baudrate! errno is: %d\n", errno);
        /*获取终端相关参数时出错*/
        return -1;
    }
    // printf("\n\r c_ispeed == %d old_termi  c_ospeed == %d",old_termi.c_ispeed, old_termi.c_ospeed);
    bzero(&new_termi, sizeof(new_termi));          /*将结构体清零*/
    new_termi.c_cflag |= (CLOCAL | CREAD);         /*忽略调制解调器状态行，接收使能*/
    new_termi.c_lflag &= ~(ICANON | ECHO | ECHOE); /*选择为原始输入模式*/
    new_termi.c_oflag &= ~OPOST;                   /*选择为原始输出模式*/
    new_termi.c_cc[VTIME] = 2;                     /*设置超时时间为0.5 s*/
    new_termi.c_cc[VMIN]  = 0;                     /*最少返回的字节数是 7*/
    new_termi.c_cflag &= ~CSIZE;
    // new_termi.c_iflag &= ~INPCK;     /* Enable parity checking */
    new_termi.c_iflag &= ~ISTRIP;
    switch (baud) {
        case 1200:
            baud_lnx = B1200;
            break;
        case 2400:
            baud_lnx = B2400;
            break;
        case 4800:
            baud_lnx = B4800;
            break;
        case 9600:
            baud_lnx = B9600;
            break;
        case 19200:
            baud_lnx = B19200;
            break;
        case 38400:
            baud_lnx = B38400;
            break;
        case 57600:
            baud_lnx = B57600;
            break;
        case 115200:
            baud_lnx = B115200;
            break;
        default:
            baud_lnx = B9600;
            break;
    }

    switch (bits) {
        case 5:
            new_termi.c_cflag |= CS5;
            break;
        case 6:
            new_termi.c_cflag |= CS6;
            break;
        case 7:
            new_termi.c_cflag |= CS7;
            break;
        case 8:
            new_termi.c_cflag |= CS8;
            break;
        default:
            new_termi.c_cflag |= CS8;
            break;
    }

    if (strncmp((char*)par, "even", 4) == 0) //设置奇偶校验为偶校验
    {
        // new_termi.c_iflag |= (INPCK | ISTRIP);
        new_termi.c_cflag |= PARENB;
        new_termi.c_cflag &= ~PARODD;

    } else if (strncmp((char*)par, "odd", 3) == 0) //设置奇偶校验为奇校验
    {
        new_termi.c_cflag |= PARENB;
        new_termi.c_cflag |= PARODD;
        // new_termi.c_iflag |= (INPCK | ISTRIP);
    } else {
        new_termi.c_cflag &= ~PARENB; //设置奇偶校验为无校验
                                      // new_termi.c_iflag &=~ ISTRIP;
    }

    if (stopb == 1) //停止位
    {
        new_termi.c_cflag &= ~CSTOPB; //设置停止位为:一位停止位
    } else if (stopb == 2) {
        new_termi.c_cflag |= CSTOPB; //设置停止位为:二位停止位
    } else {
        new_termi.c_cflag &= ~CSTOPB; //设置停止位为:一位停止位
    }

    cfsetispeed(&new_termi, baud_lnx); /* 设置输入拨特率 */
    cfsetospeed(&new_termi, baud_lnx); /* 设置输出拨特率 */

    tcflush(Com_Port, TCIOFLUSH);                      /* 刷新输入输出流 */
    if (tcsetattr(Com_Port, TCSANOW, &new_termi) != 0) /* 激活串口配置 */
    {
        fprintf(stderr, "Set serial port parameter error!\n"); // close(Com_Port);
        return 0;
    }
    return Com_Port;
}

int SendATCommand(char* buf, int len, int com) {
    int res = write(com, buf, len);
    return (res < 0) ? 0 : res;
}
int RecieveFromComm(char* buf, int mlen, int com) {
    int len = read(com, buf, mlen);
    return (len < 0) ? 0 : len;
}

//查看拨号程序是否获取到ip地址
int tryifconfig() {
    int sock;
    struct sockaddr_in sin;
    struct ifreq ifr;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        return -1;
    }
    strncpy(ifr.ifr_name, "ppp0", IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;
    if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
        close(sock);
        return -1;
    }
    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
    if (sin.sin_addr.s_addr > 0) {
        rlog("[tryifconfig]获取到正确的IP地址%s\n", inet_ntoa(sin.sin_addr));
        close(sock);
        return 1;
    }
    close(sock);
    return 0;
}

void* ATWorker(void* args) {
    while (1) {
        rlog("[ATWorker]开始拨号流程。\n");

        system("pkill ftpget");
        system("ppp-off");
        system("pkill gsmMuxd");

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

        rlog("[ATWorker]打开串口复用。\n");
        system("mux.sh &");
        sleep(5);

        int sMux0 = OpenMuxCom(0, 115200, (unsigned char*)"none", 1, 8);
        int sMux1 = OpenMuxCom(1, 115200, (unsigned char*)"none", 1, 8);

        if (sMux0 < 0 || sMux1 < 0) {
            close(sMux0);
            close(sMux1);
            goto err;
        }

        for (int i = 0; i < 3; i++) {
            char Mrecvbuf[128];
            memset(Mrecvbuf, 0, 128);

            SendATCommand("\rAT\r", 4, sMux0);
            sleep(3);
            if (RecieveFromComm(Mrecvbuf, 128, sMux0) > 0) {
                if (strstr(Mrecvbuf, "OK") != NULL) {
                    break;
                }
            }
            if (i == 2) {
                goto err;
            }
        }

        for (int timeout = 0; timeout < 10; timeout++) {
            char Mrecvbuf[128];

            SendATCommand("\rAT$MYGMR\r", 10, sMux0);
            delay(1000);
            memset(Mrecvbuf, 0, 128);
            RecieveFromComm(Mrecvbuf, 128, sMux0);

            char INFO[6][32];
            if (sscanf(Mrecvbuf, "%*[^\n]\n%[^\n]\n%[^\n]\n%[^\n]\n%[^\n]\n%[^\n]\n%[^\n]", INFO[0], INFO[1], INFO[2], INFO[3],
                       INFO[4], INFO[5]) == 6) {
                break;
            }
        }

        for (int timeout = 0; timeout < 10; timeout++) {
            char Mrecvbuf[128];

            SendATCommand("\rAT$MYTYPE?\r", 12, sMux0);
            delay(1000);
            memset(Mrecvbuf, 0, 128);
            RecieveFromComm(Mrecvbuf, 128, sMux0);

            int k, l, m;
            if (sscanf(Mrecvbuf, "%*[^:]: %d,%d,%d", &k, &l, &m) == 3) {
                if ((l & 0x01) == 1) {
                    rlog("[ATWorker]远程通信单元类型为GPRS。\n");
                    break;
                }
                if ((l & 0x08) == 8) {
                    rlog("[ATWorker]远程通信单元类型为CDMA2000。\n");
                    break;
                }
            }
        }

        for (int timeout = 0; timeout < 10; timeout++) {
            char Mrecvbuf[128];

            SendATCommand("\rAT$MYCCID\r", 11, sMux0);
            delay(1000);
            memset(Mrecvbuf, 0, 128);
            RecieveFromComm(Mrecvbuf, 128, sMux0);
            char CCID[32];
            memset(CCID, 0, 32);
            if (sscanf(Mrecvbuf, "%*[^\"]\"%[0-9|A-Z|a-z]", CCID) == 1) {
                rlog("CCID: %s\n", CCID);
                break;
            }
        }

        for (int timeout = 0; timeout < 50; timeout++) {
            char Mrecvbuf[128];

            SendATCommand("\rAT+CSQ\r", 8, sMux0);
            delay(1000);
            memset(Mrecvbuf, 0, 128);
            RecieveFromComm(Mrecvbuf, 128, sMux0);

            int k, l;
            if (sscanf(Mrecvbuf, "%*[^:]: %d,%d", &k, &l) == 2) {
                rlog("GprsCSQ = %d,%d\n", k, l);
                if (k != 99) {
                    break;
                }
            }
        }

        for (int timeout = 0; timeout < 50; timeout++) {
            char Mrecvbuf[128];

            SendATCommand("\rAT+CREG?\r", 10, sMux0);
            delay(1000);
            memset(Mrecvbuf, 0, 128);
            RecieveFromComm(Mrecvbuf, 128, sMux0);

            int k, l;
            if (sscanf(Mrecvbuf, "%*[^:]: %d,%d", &k, &l) == 2) {
                rlog("GprsCREG = %d,%d\n", k, l);
                if (l == 1 || l == 5) {
                    break;
                }
            }
        }

        system("pppd call gprs &");

        for (int i = 0; i < 50; i++) {
            if (tryifconfig() == 1) {
                break;
            }
        }

        while (1) {
            delay(1000);
            printf("wait for error.\n");
        }

    err:
        continue;
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
    struct sigaction sa = {};

    if (access("/nand/mlog", F_OK) == -1) {
        system("mkdir /nand/mlog");
    }

    if (InitPro(argc, argv) == 0) {
        fprintf(stderr, "[main]进程 %s 参数错误\n", argv[0]);
        return EXIT_FAILURE;
    }
    Setsig(&sa, QuitProcess);

    CreateATWorker();
    CreateFirComWorker();

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
