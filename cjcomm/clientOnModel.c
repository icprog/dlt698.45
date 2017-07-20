#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdarg.h>
#include <pthread.h>

#include "PublicFunction.h"
#include "dlt698def.h"
#include "cjcomm.h"
#include "../include/Shmem.h"
#include "clientOnModel.h"
#include "basedef.h"

/*
 * 内部协议栈参数
 */
static CLASS25 Class25;
static CommBlock ClientForModelObject;
static long long ClientOnModel_Task_Id;
static MASTER_STATION_INFO NetIps[4];
static pthread_mutex_t locker;
static NetObject netObject;
//月流量统计
static int MonthTJ;

void showTime()
{
	TS ts;
	TSGet(&ts);
	fprintf(stderr, "时间%d-%d\n", ts.Minute, ts.Sec);
}

CommBlock *getComBlockForModel() {
    return &ClientForModelObject;
}

static int NetSend(int fd, INT8U *buf, INT16U len) {
    if (len > 2048) {
        asyslog(LOG_WARNING, "发送报文太长[2048-%d]", len);
        return 0;
    }
    pthread_mutex_lock(&locker);
    if (netObject.tail + 1 != netObject.head) {
        memcpy(netObject.send[netObject.tail].buf, buf, len);
        netObject.send[netObject.tail].len = len;
        asyslog(LOG_WARNING, "内部协议栈发送(长度:%d,头尾:%d-%d)", len, netObject.head, netObject.tail);
        netObject.tail++;
        netObject.tail %= 16;
    } else {
        asyslog(LOG_WARNING, "内部协议栈发送[缓冲区满](长度:%d,头尾:%d-%d)", len, netObject.head, netObject.tail);
    }
    pthread_mutex_unlock(&locker);
}

static int NetRecv(INT8U *buf) {
    pthread_mutex_lock(&locker);
    int len = netObject.recv.len;
    if (len < 1) {
        pthread_mutex_unlock(&locker);
        return 0;
    }
    MonthTJ += len;
    netObject.recv.len = 0;
    for (int i = 0; i < len; ++i) {
        buf[i] = netObject.recv.buf[i];
    }
    pthread_mutex_unlock(&locker);
    return len;
}

static int getNext(INT8U *buf) {
    if (netObject.head == netObject.tail) {
        return -1;
    }
    pthread_mutex_lock(&locker);
    int res = netObject.head;
    int len = netObject.send[res].len;

    for (int i = 0; i < netObject.send[res].len; ++i) {
        buf[i] = netObject.send[res].buf[i];
    }
    netObject.head++;
    netObject.head %= 16;
    pthread_mutex_unlock(&locker);
    return len;
}

static int putNext(INT8U *buf, INT16U len) {
    pthread_mutex_lock(&locker);
    MonthTJ += len;
    if (netObject.recv.len + len > 2048) {
        pthread_mutex_unlock(&locker);
        return -1;
    }

    for (int i = 0; i < len; ++i) {
        netObject.recv.buf[netObject.recv.len + i] = buf[i];
    }
    netObject.recv.len += len;
    pthread_mutex_unlock(&locker);
    return len;
}

static MASTER_STATION_INFO getNextGprsIpPort(CommBlock *commBlock) {
    static int index = 0;
    static int ChangeFlag = -1;
    //检查主站参数是否有变化
    if (ChangeFlag != ((ProgramInfo *) commBlock->shmem)->oi_changed.oi4500) {
        readCoverClass(0x4500, 0, (void *) &Class25, sizeof(CLASS25), para_vari_save);
        memcpy(&NetIps, &Class25.master.master, sizeof(NetIps));
        asyslog(LOG_WARNING, "检测到通信参数变化！刷新主站参数！");
        ChangeFlag = ((ProgramInfo *) commBlock->shmem)->oi_changed.oi4500;
        commBlock->Heartbeat = Class25.commconfig.heartBeat;
        readCoverClass(0xf101, 0, (void *) &ClientForModelObject.f101, sizeof(CLASS_F101), para_vari_save);
    }

    MASTER_STATION_INFO res;
    memset(&res, 0x00, sizeof(MASTER_STATION_INFO));
    snprintf((char *) res.ip, sizeof(res.ip), "%d.%d.%d.%d", NetIps[index].ip[1], NetIps[index].ip[2],
             NetIps[index].ip[3], NetIps[index].ip[4]);
    res.port = NetIps[index].port;
    index++;
    index %= 2;
    asyslog(LOG_INFO, "客户端[GPRS]尝试链接的IP地址：%s:%d", res.ip, res.port);
    return res;
}

int SendCommandGetOK(int fd, int retry, const char *fmt, ...) {
    va_list argp;
    va_start(argp, fmt);
    char cmd[128];
    memset(cmd, 0x00, sizeof(cmd));
    vsprintf(cmd, fmt, argp);
    va_end(argp);

    for (int timeout = 0; timeout < retry; timeout++) {
        char Mrecvbuf[128];
        SendATCommand(cmd, strlen(cmd), fd);
        delay(800);
        memset(Mrecvbuf, 0, 128);
        RecieveFromComm(Mrecvbuf, 128, fd);
        if (strstr(Mrecvbuf, "OK") != 0) {
            return 1;
        }
    }
    return 0;
}

void resetModel() {
    asyslog(LOG_INFO, "重置模块状态...");
    gpofun("/dev/gpoGPRS_POWER", 0);
    sleep(8);
    gpofun("/dev/gpoGPRS_POWER", 1);
    gpofun("/dev/gpoGPRS_RST", 1);
    gpofun("/dev/gpoGPRS_SWITCH", 1);
    sleep(2);
    gpofun("/dev/gpoGPRS_RST", 0);
    sleep(1);
    gpofun("/dev/gpoGPRS_RST", 1);
    sleep(5);
    gpofun("/dev/gpoGPRS_SWITCH", 0);
    sleep(1);
    gpofun("/dev/gpoGPRS_SWITCH", 1);
    sleep(10);
}

int getNetType(int fd) {
    char Mrecvbuf[128];
    for (int timeout = 0; timeout < 10; timeout++) {
        SendATCommand("\rAT$MYTYPE?\r", 12, fd);
        delay(1000);
        memset(Mrecvbuf, 0, 128);
        RecieveFromComm(Mrecvbuf, 128, fd);

        int k, l, m;
        if (sscanf(Mrecvbuf, "%*[^:]: %d,%d,%d", &k, &l, &m) == 3) {
            if ((l & 0x01) == 1) {
                asyslog(LOG_INFO, "远程通信单元类型为GPRS。\n");
                break;
            }
            if ((l & 0x08) == 8) {
                asyslog(LOG_INFO, "远程通信单元类型为CDMA2000。\n");
                break;
            }
        }
    }
}

int getCIMIType(int fd) {
    char *cimiType[] = {
            "46003", "46011",
    };

    char Mrecvbuf[128];
    for (int timeout = 0; timeout < 5; timeout++) {
        SendATCommand("\rAT+CIMI\r", 9, fd);
        delay(1000);
        memset(Mrecvbuf, 0, 128);
        RecieveFromComm(Mrecvbuf, 128, fd);

        char cimi[64];
        memset(cimi, 0x00, sizeof(cimi));
        if (sscanf((char *) &Mrecvbuf[0], "%*[^0-9]%[0-9]", cimi) == 1) {
            asyslog(LOG_INFO, "CIMI = %s\n", cimi);
            for (int i = 0; i < 2; i++) {
                if (strncmp(cimiType[i], cimi, 5) == 0) {
                    return 3;
                }
            }
            return 1;
        }
    }
    return 0;
}

int getNetworkType(int fd) {
    for (int timeout = 0; timeout < 5; timeout++) {
        char Mrecvbuf[128];

        SendATCommand("\rAT+QNWINFO\r", 12, fd);
        delay(1000);
        memset(Mrecvbuf, 0, 128);
        RecieveFromComm(Mrecvbuf, 128, fd);
        if (strstr(Mrecvbuf, "TDD LTE") != NULL) {
            asyslog(LOG_INFO, "TDD LTE模式在线");
            SetWireLessType(2);
            break;
        }
        if (strstr(Mrecvbuf, "FDD LTE") != NULL) {
            asyslog(LOG_INFO, "FDD LTE模式在线");
            SetWireLessType(3);
            break;
        }
    }

}

int regIntoNet(int fd) {
    char Mrecvbuf[128];
    for (int timeout = 0; timeout < 80; timeout++) {
        SendATCommand("\rAT+CREG?\r", 10, fd);
        delay(1000);
        memset(Mrecvbuf, 0, 128);
        RecieveFromComm(Mrecvbuf, 128, fd);

        int k, l;
        if (sscanf(Mrecvbuf, "%*[^:]: %d,%d", &k, &l) == 2) {
            asyslog(LOG_INFO, "GprsCREG = %d,%d\n", k, l);
            if (l == 1 || l == 5) {
                return 1;
            }
        }
    }
    return 0;
}

int myStrnstr(char *buf, int len) {
    for (int i = 0; i < len - 1; ++i) {
        if (buf[i] == 'O' && buf[i + 1] == 'K') {
            return i;
        }
    }
    return 0;
}

int checkModelStatus(char *buf, int len) {
    static int status = 0;
    static int merror = 0;

    if (len < 0) {
        int res = status;
        status = 0;
        return res;
    }

    for (int i = 0; i < len - 8; ++i) {
        if (buf[i] == 'U'
            && buf[i + 1] == 'R' && buf[i + 2] == 'C'
            && buf[i + 3] == 'R' && buf[i + 4] == 'E'
            && buf[i + 5] == 'A' && buf[i + 6] == 'D') {
            status = 1; //有数据读入
            return 0;
        }
    }

    for (int i = 0; i < len - 5; ++i) {
        if (buf[i] == 'E'
            && buf[i + 1] == 'R' && buf[i + 2] == 'R'
            && buf[i + 3] == 'O' && buf[i + 4] == 'R') {
            if (sscanf(buf, "%*[^:]: %d", &merror) == 1) {
                status = 99;//有错误
                return 0;
            }
            status = 99;
            merror = 99;
            return 0;
        }
    }
}


int modelSendExactly(int fd, int retry, int len, int buf) {
    int chl = 0;
    int sum = 0;
    int length = len;

    char recbuf[2048];
    char cmdBuf[2048];

    bufsyslog(buf, "客户端[内部协议栈]发送:", len, 0, BUFLEN);

    for (int timeout = 0; timeout < retry; timeout++) {
        memset(recbuf, 0x00, sizeof(recbuf));
        memset(cmdBuf, 0x00, sizeof(cmdBuf));

        sprintf(cmdBuf, "\rAT$MYNETWRITE=1,%d\r", length);
        SendATCommand(cmdBuf, strlen(cmdBuf), fd);

        delay(800);
        int recLen = RecieveFromComm(recbuf, sizeof(recbuf), fd);
        checkModelStatus(recbuf, recLen);

        if (sscanf(recbuf, "%*[^:]: %d,%d", &chl, &sum) == 2) {
            write(fd, buf, sum);
            for (int j = 0; j < 3; ++j) {
                delay(800);
                memset(recbuf, 0, sizeof(recbuf));
                int position = RecieveFromComm(recbuf, sizeof(recbuf), fd);
                checkModelStatus(recbuf, position);
                if (myStrnstr(recbuf, position) != 0) {
                    printf("~~~~~~~~~~~~~%d-%d-%d", length, sum, position);
                    length -= sum;
                    if (length == 0) {
                        return len;
                    }
                }
            }
        }
    }
    return 0;
}

int readPositionGet(int length) {
    int pos = 1;
    int tmp = length;
    for (int i = 0; i < 5; ++i) {
        tmp /= 10;
        if (tmp == 0) {
            break;
        }
        pos++;
    }
    return pos;
}

int modelReadExactly(int fd, int retry) {
    int chl = 0;
    int sum = 0;

    char Mrecvbuf[2048];
    for (int timeout = 0; timeout < retry; timeout++) {
        memset(Mrecvbuf, 0, 2048);
        SendATCommand("\rAT$MYNETREAD=1,1024\r", strlen("\rAT$MYNETREAD=1,1024\r"), fd);
        delay(800);
        int resLen = RecieveFromComm(Mrecvbuf, 2048, fd);
        checkModelStatus(Mrecvbuf, resLen);

        char *netReadPos = strstr(Mrecvbuf, "MYNETREAD:");
        if (netReadPos == NULL) {
            printf("找不到MYNETREAD\n");
            continue;
        }

        if (sscanf(netReadPos, "%*[^:]: %d,%d", &chl, &sum) == 2) {
            printf("报文通道：长度(%d-%d)\n", chl, sum);
            if (sum == 0) { break; }
            int pos = readPositionGet(sum);
            printf("============recv %d, at %d has %d [bytes] (%c %c %c)\n", resLen, 15 + pos, sum,
                   netReadPos + 12 + pos, netReadPos + 13 + pos, netReadPos + 14 + pos);
            putNext(netReadPos + 15 + pos, sum);
            break;
        } else {
            printf("没有解析到报文\n");
        }
    }
}

int checkRecv(int fd, int retry) {
    char Mrecvbuf[128];
    for (int timeout = 0; timeout < retry; timeout++) {
        memset(Mrecvbuf, 0, 128);
        RecieveFromComm(Mrecvbuf, 128, fd);
        if (strstr(Mrecvbuf, "MYURCREAD") != 0) {
            return 1;
        }
    }
    return 0;
}

void *ModelWorker(void *args) {
    CLASS25 *class25 = (CLASS25 *) args;
    int sMux0 = -1;
    int com = 0;    //I型\III型GPRS打开串口0,II型打开串口5

    ProgramInfo *memp = ClientForModelObject.shmem;
    if (memp->cfg_para.device == CCTT2)
        com = 5;

    while (1) {
        gpofun("/dev/gpoCSQ_GREEN", 0);
        gpofun("/dev/gpoCSQ_RED", 0);
        gpofun("/dev/gpoONLINE_LED", 0);

        SetGprsStatus(0);
        SetGprsCSQ(0);
        SetWireLessType(0);
        SetPPPDStatus(0);

        resetModel();

        if (GetOnlineType() != 0) { goto wait; }

        if ((sMux0 = OpenCom(com, 115200, (unsigned char *) "none", 1, 8)) < 0) { goto err; }
        if (SendCommandGetOK(sMux0, 5, "\rat\r") == 0) { goto err; }
        SetGprsStatus(1);

        ////////////////////获取信息////////////////////
        for (int timeout = 0; timeout < 10; timeout++) {
            char Mrecvbuf[128];

            SendATCommand("\rAT$MYGMR\r", 10, sMux0);
            delay(1000);
            memset(Mrecvbuf, 0, 128);
            RecieveFromComm(Mrecvbuf, 128, sMux0);

            char INFO[6][32];
            if (sscanf(Mrecvbuf, "%*[^\n]\n%[^\n]\n%[^\n]\n%[^\n]\n%[^\n]\n%[^\n]\n%[^\n]", INFO[0], INFO[1], INFO[2],
                       INFO[3], INFO[4], INFO[5]) == 6) {
                break;
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
                asyslog(LOG_INFO, "CCID: %s\n", CCID);
                memcpy(class25->ccid, CCID, sizeof(32));
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
                asyslog(LOG_INFO, "GprsCSQ = %d,%d\n", k, l);
                if (k != 99) {
                    class25->signalStrength = k;
                    SetGprsCSQ(k);
                    if (k > 20) {
                        gpofun("/dev/gpoCSQ_GREEN", 1);
                    } else if (k > 10) {
                        gpofun("/dev/gpoCSQ_GREEN", 1);
                        gpofun("/dev/gpoCSQ_RED", 1);
                    } else {
                        gpofun("/dev/gpoCSQ_RED", 1);
                    }
                    break;
                }
            }
        }

        CLASS25 class25_temp;
        readCoverClass(0x4500, 0, &class25_temp, sizeof(CLASS25), para_vari_save);
        fprintf(stderr, "刷新4500数据（1）");
        memcpy(class25_temp.ccid, class25->ccid, sizeof(32));
        class25_temp.signalStrength = class25->signalStrength;
        SetGprsStatus(2);
        saveCoverClass(0x4500, 0, &class25_temp, sizeof(CLASS25), para_vari_save);
        ////////////////////获取信息////////////////////


        if (GetOnlineType() != 0) { goto wait; }
        if (regIntoNet(sMux0) == 0) { goto err; }

        SetGprsStatus(3);

        MASTER_STATION_INFO m = getNextGprsIpPort(&ClientForModelObject);
        switch (getCIMIType(sMux0)) {
            case 1:
                SendCommandGetOK(sMux0, 5, "\rAT$MYNETACT=1,0\r");
                if (SendCommandGetOK(sMux0, 5, "\rAT$MYNETCON=1,\"APN\",\"%s\"\r",
                                     &class25->commconfig.apn[1]) == 0) { goto err; }
                if (SendCommandGetOK(sMux0, 5, "\rAT$MYNETCON=1,\"USERPWD\",\"%s,%s\"\r",
                                     &class25->commconfig.userName[1],
                                     &class25->commconfig.passWord[1]) == 0) { goto err; }
                if (SendCommandGetOK(sMux0, 5, "\rAT$MYNETURC=1\r") == 0) { goto err; }

                if (SendCommandGetOK(sMux0, 5, "\rAT$MYNETSRV=1,1,0,0,\"%s:%d\"\r", m.ip, m.port) == 0) { goto err; }
                if (SendCommandGetOK(sMux0, 8, "\rAT$MYNETACT=1,1\r") == 0) { goto err; }
                if (SendCommandGetOK(sMux0, 20, "\rAT$MYNETOPEN=1\r") == 0) { goto err; }
                SetOnlineType(3);
                SetGprsStatus(4);
                SetWireLessType(1);
                SetPPPDStatus(1);
                getNetworkType(sMux0);
                gpofun("/dev/gpoONLINE_LED", 1);
                break;
            case 3:
                SendCommandGetOK(sMux0, 5, "\rAT$MYNETACT=1,0\r");
                if (SendCommandGetOK(sMux0, 5, "\rAT$MYNETCON=1,\"APN\",\"%s\"\r",
                                     &class25->commconfig.apn[1]) == 0) { goto err; }
                if (SendCommandGetOK(sMux0, 5, "\rAT$MYNETCON=1,\"USERPWD\",\"%s,%s\"\r",
                                     &class25->commconfig.userName[1],
                                     &class25->commconfig.passWord[1]) == 0) { goto err; }
                if (SendCommandGetOK(sMux0, 5, "\rAT$MYNETURC=1\r") == 0) { goto err; }

                if (SendCommandGetOK(sMux0, 5, "\rAT$MYNETSRV=1,1,0,0,\"%s:%d\"\r", m.ip, m.port) == 0) { goto err; }
                if (SendCommandGetOK(sMux0, 8, "\rAT$MYNETACT=1,1\r") == 0) { goto err; }
                if (SendCommandGetOK(sMux0, 20, "\rAT$MYNETOPEN=1\r") == 0) { goto err; }
                SetOnlineType(3);
                SetGprsStatus(4);
                SetWireLessType(1);
                SetPPPDStatus(1);
                getNetworkType(sMux0);
                gpofun("/dev/gpoONLINE_LED", 1);
                break;
            default:
                goto err;
        }

        wait:
        while (1) {
            sleep(1);
            if (GetOnlineType() == 0) { goto err; }

            INT8U sendBuf[2048];
            memset(sendBuf, 0x00, sizeof(sendBuf));
            int readySendLen = getNext(sendBuf);

            if (readySendLen != -1) {
//            	showTime();
                modelSendExactly(sMux0, 5, readySendLen, sendBuf);
            }

            modelReadExactly(sMux0, 3);

            switch (checkModelStatus(NULL, -1)) {
                case 1:
                    sleep(1);
                    modelReadExactly(sMux0, 3);
                    break;
                case 99:
                    asyslog(LOG_ERR, "内部协议栈连接出错!");
                    goto err;
                    break;

            }
        }

        err:
        sleep(1);
        close(sMux0);
        SetOnlineType(0);
        continue;
    }
}

void CreateOnModel(void *clientdata) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_mutex_init(&locker, NULL);

    pthread_t temp_key;
    pthread_create(&temp_key, &attr, ModelWorker, clientdata);
}


void CalculateTransFlowModel(ProgramInfo *prginfo_event) {
    //统计临时变量
    static int rtx_bytes = 0;
    static int rx_bytes = 0;
    static int tx_bytes = 0;
    static int localMin = 0;
    static int localDay = 0;
    static int localMonth = 0;
    static int localSec = 0;

    static int first_flag = 1;

    TS ts = {};
    TSGet(&ts);

    if (first_flag == 1) {
        first_flag = 0;
        memset(&prginfo_event->dev_info.realTimeC2200, 0x00, sizeof(Flow_tj));
        readVariData(0x2200, 0, &prginfo_event->dev_info.realTimeC2200, sizeof(Flow_tj));
        asyslog(LOG_INFO, "初始化月流量统计(%d)", prginfo_event->dev_info.realTimeC2200.flow.month_tj);

        localMin = ts.Minute;
        localDay = ts.Day;
        localMonth = ts.Month;
        localSec = ts.Sec;
    }


    if (localSec != ts.Sec) {
        localSec = ts.Sec;
    } else {
        return;
    }

    pthread_mutex_lock(&locker);
    prginfo_event->dev_info.realTimeC2200.flow.day_tj += MonthTJ;
    prginfo_event->dev_info.realTimeC2200.flow.month_tj += MonthTJ;
    MonthTJ = 0;
    pthread_mutex_unlock(&locker);

    Event_3110(prginfo_event->dev_info.realTimeC2200.flow.month_tj, sizeof(prginfo_event->dev_info.realTimeC2200.flow),
               prginfo_event);

    if (localMin != ts.Minute && ts.Minute % 2 == 0) {
        localMin = ts.Minute;
//        asyslog(LOG_INFO, "2分钟月流量统计，未统计流量%d", (rx_bytes + tx_bytes) - rtx_bytes);//一直是0,上面已经赋值rtx_bytes
        //跨日月流量分别清零
        if (localDay != ts.Day) {
            asyslog(LOG_INFO, "检测到夸日，流量统计清零，清零前数据(%d)", prginfo_event->dev_info.realTimeC2200.flow.day_tj);
            Save_TJ_Freeze(0x2200,0x0200,0,ts,sizeof(Flow_tj),(INT8U *)&prginfo_event->dev_info.realTimeC2200);
            prginfo_event->dev_info.realTimeC2200.flow.day_tj = 0;
            localDay = ts.Day;
        }
        if (localMonth != ts.Month) {
            asyslog(LOG_INFO, "检测到夸月，流量统计清零，清零前数据(%d)", prginfo_event->dev_info.realTimeC2200.flow.month_tj);
            Save_TJ_Freeze(0x2200,0x0200,1,ts,sizeof(Flow_tj),(INT8U *)&prginfo_event->dev_info.realTimeC2200);
            prginfo_event->dev_info.realTimeC2200.flow.month_tj = 0;
            localMonth = ts.Month;
        }
        saveVariData(0x2200, 0, &prginfo_event->dev_info.realTimeC2200, sizeof(Flow_tj));
    }

    return;
}

static int RegularClientOnModel(struct aeEventLoop *ep, long long id, void *clientData) {
    CommBlock *nst = (CommBlock *) clientData;
    ProgramInfo* prginfo_event = (ProgramInfo*)nst->shmem;

    if (GetOnlineType() != 3) {
        refreshComPara(nst);
        return 1000;
    }


    INT8U recvBuf[2048];
    memset(recvBuf, 0x00, sizeof(recvBuf));
//    showTime();
    int revcount = NetRecv(recvBuf);
    if (revcount > 0) {
    	TSGet(&nst->final_frame);
        for (int j = 0; j < revcount; j++) {
            nst->RecBuf[nst->RHead] = recvBuf[j];
            nst->RHead = (nst->RHead + 1) % BUFLEN;
        }

        bufsyslog(nst->RecBuf, "客户端[GPRS]接收:", nst->RHead, nst->RTail, BUFLEN);
    }

    if (Comm_task(nst) == -1) {
        asyslog(LOG_WARNING, "内部协议栈[GPRS]链接心跳超时，关闭端口");
        SetOnlineType(0);
    }

	int res = 0;
	do {
		res = StateProcess(nst, 5);
		if (nst->deal_step >= 3) {
			int apduType = ProcessData(nst);
			ConformAutoTask(ep, nst, apduType);
			switch (apduType) {
			case LINK_RESPONSE:
				First_VerifiTime(nst->linkResponse, nst->shmem); //简单对时
				if (GetTimeOffsetFlag() == 1) {
					Getk_curr(nst->linkResponse, nst->shmem);
				}
				nst->linkstate = build_connection;
				nst->testcounter = 0;
				break;
			default:
				break;
			}
		}
	} while (res == 1);



    //判断流量越限事件
//    prginfo_event->dev_info.realTimeC2200.flow.month_tj += MonthTJ;
//    fprintf(stderr, "流量越限事件 %d-%d\n", MonthTJ, prginfo_event->dev_info.realTimeC2200.flow.month_tj);
//    MonthTJ = 0;
//    Event_3110(prginfo_event->dev_info.realTimeC2200.flow.month_tj, sizeof(prginfo_event->dev_info.realTimeC2200.flow),
//                   prginfo_event);

    check_F101_changed_Gprs(nst);
    CalculateTransFlowModel(nst->shmem);
    //暂时忽略函数返回
    RegularAutoTask(ep, nst);

    return 100;
}

/*
 * 模块*内部*使用的初始化参数
 */
static void ClientOnModelInit(void) {
    asyslog(LOG_INFO, "\n\n>>>======初始化（内部协议栈[GPRS]模式）模块======<<<");

    //读取设备参数
    readCoverClass(0x4500, 0, (void *) &Class25, sizeof(CLASS25), para_vari_save);
    asyslog(LOG_INFO, "工作模式 enum{混合模式(0),客户机模式(1),服务器模式(2)}：%d", Class25.commconfig.workModel);
    asyslog(LOG_INFO, "在线方式 enum{永久在线(0),被动激活(1)}：%d", Class25.commconfig.onlineType);
    asyslog(LOG_INFO, "连接方式 enum{TCP(0),UDP(1)}：%d", Class25.commconfig.connectType);
    asyslog(LOG_INFO, "连接应用方式 enum{主备模式(0),多连接模式(1)}：%d", Class25.commconfig.appConnectType);
    asyslog(LOG_INFO, "侦听端口列表：%d", Class25.commconfig.listenPort[0]);
    asyslog(LOG_INFO, "超时时间，重发次数：%02x", Class25.commconfig.timeoutRtry);
    asyslog(LOG_INFO, "心跳周期秒：%d", Class25.commconfig.heartBeat);
    memcpy(&NetIps, &Class25.master.master, sizeof(NetIps));
    asyslog(LOG_INFO, "主站通信地址(1)为：%d.%d.%d.%d:%d", NetIps[0].ip[1], NetIps[0].ip[2], NetIps[0].ip[3],
            NetIps[0].ip[4],
            NetIps[0].port);
    asyslog(LOG_INFO, "主站通信地址(2)为：%d.%d.%d.%d:%d", NetIps[1].ip[1], NetIps[1].ip[2], NetIps[1].ip[3],
            NetIps[1].ip[4],
            NetIps[1].port);

    initComPara(&ClientForModelObject, NetSend);
    ClientForModelObject.Heartbeat = Class25.commconfig.heartBeat;
    readCoverClass(0xf101, 0, (void *) &ClientForModelObject.f101, sizeof(CLASS_F101), para_vari_save);

    asyslog(LOG_INFO, ">>>======初始化（内部协议栈[GPRS]模式）结束======<<<");
}


/*
 * 供外部使用的初始化函数，并开启维护循环
 */
int StartClientOnModel(struct aeEventLoop *ep, long long id, void *clientData) {
    ClientOnModelInit();
    CreateOnModel(&Class25);
    ClientOnModel_Task_Id = aeCreateTimeEvent(ep, 1000, RegularClientOnModel, &ClientForModelObject, NULL);
    asyslog(LOG_INFO, "内部协议栈[GPRS]时间事件注册完成(%lld)", ClientOnModel_Task_Id);
    MonthTJ = 0;

    return 1;
}

/*
 * 用于程序退出时调用
 */
void ClientOnModelDestory(void) {
    asyslog(LOG_INFO, "开始关闭内部协议栈[GPRS]接口(%d)", ClientForModelObject.phy_connect_fd);
    close(ClientForModelObject.phy_connect_fd);
    ClientForModelObject.phy_connect_fd = -1;
}
