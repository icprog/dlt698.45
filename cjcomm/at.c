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

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "PublicFunction.h"
#include "dlt698def.h"
#include "cjcomm.h"
#include "rlog.h"
#include "vsms.h"


static int NeedDoAt = 1;
static int ledPort = -1;

void SetOnline(void){
	asyslog(LOG_NOTICE, "设置不需要AT拨号");
	NeedDoAt = 0;
}

void SetOffline(void){
	asyslog(LOG_NOTICE, "设置需要AT拨号");
	NeedDoAt = 1;
}

int checkgprs_exist() {
    INT8S gprsid  = 0;
    INT8S gprs_s0 = -1, gprs_s1 = -1, gprs_s2 = -1;

    gprsid = getSpiAnalogState() & 0x1f;
    if ((gprsid & 0x1f) == 0x1e) {
        asyslog(LOG_INFO, "有GPRS模块  %02x", gprsid);
        return 1;
    } else if (gprsid == -1) { // II型
        gprs_s0 = gpio_readbyte("DEV_GPRS_S0");
        gprs_s1 = gpio_readbyte("DEV_GPRS_S1");
        gprs_s2 = gpio_readbyte("DEV_GPRS_S2");
        if (gprs_s0 == 1 && gprs_s1 == 1 && gprs_s2 == 1) {
            asyslog(LOG_INFO, "无GPRS模块  %d, %d, %d", gprs_s0, gprs_s1, gprs_s2);
            return 0;
        } else {
            asyslog(LOG_INFO, "有GPRS模块  %d, %d, %d", gprs_s0, gprs_s1, gprs_s2);
            return 1;
        }
    }
    return 0;
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
    //    sprintf((char*)tmp, "/dev/ttyS%d", port);

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
    int i   = 0;
    if (len > 0) {
        for (i = 0; i < len; i++)
            fprintf(stderr, "%c", buf[i]);
    }
    return (res < 0) ? 0 : res;
}

int RecieveFromComm(char* buf, int mlen, int com) {
    int len = read(com, buf, mlen);
    int i   = 0;
    if (len > 0) {
        for (i = 0; i < len; i++)
            fprintf(stderr, "%c", buf[i]);
    }
    return (len < 0) ? 0 : len;
}


/************************************************************
 * 函数功能：控制无线模块net灯
 * *********************************************************/
int ATMYSOCKETLED(unsigned char step, int ComPort) {
    int timeout = 0, i, scid, Len;
    char scidstr[50];
    memset(scidstr, 0, 50);
    ComPort = ledPort;
    fprintf(stderr, "AT MYSOCKETLED %d\n", ComPort);
    do {
        scid = 0;
        timeout++;
        switch (step) {
            case 1:
                fprintf(stderr, "步骤1：受限的网路服务；无SIM卡或需输入PIN码；正在搜索网络；正在进行用户鉴权等\n");
                SendATCommand("AT$MYSOCKETLED=0\r", 17, ComPort);
                break;
            case 2:
                fprintf(stderr, "步骤2：模块处于待机状态\n");
                SendATCommand("AT$MYSOCKETLED=1\r", 17, ComPort);
                break;
            case 3:
                fprintf(stderr, "步骤3：PDP激活状态，并已获取IP地址\n");
                SendATCommand("AT$MYSOCKETLED=0\r", 17, ComPort);
                break;
            case 4:
                fprintf(stderr, "步骤4：Socket链接已建立\n");
                SendATCommand("AT$MYSOCKETLED=1\r", 17, ComPort);
                break;
            default:
                break;
        }
        delay(5000);
        INT8U Mrecvbuf[RES_LENGTH];
        memset(Mrecvbuf, 0, RES_LENGTH);
        Len = RecieveFromComm(Mrecvbuf, RES_LENGTH, ComPort);
        if (Len > 0) {
            for (i = 0; i < Len; i++) {
                fprintf(stderr, "%c", Mrecvbuf[i]);
            }
            fprintf(stderr, "\n");
        }

        for (i = 0; i < Len; i++) {
            if ((Mrecvbuf[i] == 'O') && (Mrecvbuf[i + 1] == 'K')) {
                scid = 1;
            }
        }
        if (scid == 1)
            break;

    } while (timeout < 5);
    if (timeout >= 5)
        return 0;

    return 1;
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
        asyslog(LOG_INFO, "获取到正确的IP地址%s\n", inet_ntoa(sin.sin_addr));
//                setPPPIP(inet_ntoa(sin.sin_addr));
        close(sock);
        return 1;
    }
    close(sock);
    return 0;
}

void AT_POWOFF() {
    //关闭打开的服务
    asyslog(LOG_INFO, "开始关闭外部服务（gtpget、ppp-off、gsmMuxd）");
    system("pkill ftpget");
    system("ppp-off");
    system("pkill gsmMuxd");
    sleep(3);
    gpofun("/dev/gpoGPRS_POWER", 0);
}

void* ATWorker(void* args) {
    while (1) {
        if (NeedDoAt == 0) {
            goto wait;
        }

        system("pkill ftpget");
        system("ppp-off");
        system("pkill gsmMuxd");
        sleep(3);

        gpofun("/dev/gpoGPRS_POWER", 0);
        sleep(4);
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

        asyslog(LOG_INFO, "打开串口复用模块");
        system("mux.sh &");
        sleep(15);

        int sMux0 = OpenMuxCom(0, 115200, (unsigned char*)"none", 1, 8); // 0
        int sMux1 = OpenMuxCom(1, 115200, (unsigned char*)"none", 1, 8);
        ledPort = sMux1;
        if (sMux0 < 0 || sMux1 < 0) {
            close(sMux0);
            close(sMux1);
            fprintf(stderr, "\n打开串口复用错误！\n");
            goto err;
        }

        for (int i = 0; i < 5; i++) {
            char Mrecvbuf[128];
            memset(Mrecvbuf, 0, 128);

            SendATCommand("at\r", 3, sMux0);
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
                    asyslog(LOG_INFO, "远程通信单元类型为GPRS。\n");
                    break;
                }
                if ((l & 0x08) == 8) {
                    asyslog(LOG_INFO, "远程通信单元类型为CDMA2000。\n");
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
                asyslog(LOG_INFO, "CCID: %s\n", CCID);
                setCCID(CCID);
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
                    setSINSTR(k);
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
                asyslog(LOG_INFO, "GprsCREG = %d,%d\n", k, l);
                if (l == 1 || l == 5) {
                    break;
                }
            }
        }
        close(sMux0);

        system("pppd call gprs &");

        for (int i = 0; i < 50; i++) {
            sleep(1);
            if (tryifconfig() == 1) {
                //拨号成功，存储参数，以备召唤
                saveCurrClass25();
                break;
            }
        }
        printf("sMux1 %d\n", sMux1);
        setPort(sMux1);
        ATMYSOCKETLED(4, -1);

    wait:
//    	deal_vsms(sMux1);
        //等待在线状态为“否”，重新拨号
        while (1) {
//        	RecePro(0);
            usleep(200);
            if (NeedDoAt == 1) {
                break;
            }
        }

    err:
        //拨号出错，重试
    	sleep(1);
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
