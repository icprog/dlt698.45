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
#include "at.h"

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
    fprintf(stderr, "[AT]recv:\n");
    if (len > 0) {
        for (i = 0; i < len; i++)
            fprintf(stderr, "%c", buf[i]);
    }
    fprintf(stderr, "================\n");
    return (len < 0) ? 0 : len;
}

//查看拨号程序是否获取到ip地址
int tryifconfig(CLASS25* class25) {
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
        int ips[4];
        memset(ips, 0x00, sizeof(ips));
        sscanf(inet_ntoa(sin.sin_addr), "%d.%d.%d.%d", &ips[0], &ips[1], &ips[2], &ips[3]);
        asyslog(LOG_INFO, "获取到正确的IP地址%d.%d.%d.%d\n", ips[0], ips[1], ips[2], ips[3]);
        class25->pppip[0] = 4;
        class25->pppip[0] = ips[0];
        class25->pppip[0] = ips[1];
        class25->pppip[0] = ips[2];
        class25->pppip[0] = ips[3];
        close(sock);
        return 1;
    }
    close(sock);
    return 0;
}

void AT_POWOFF() {
    //关闭打开的服务
    asyslog(LOG_INFO, "开始关闭外部服务（gtpget、ppp-off、gsmMuxd）");
    system("ppp-off");
    absoluteKill("ftpget", 15);
    absoluteKill("gsmMuxd", 15);
    sleep(3);
    gpofun("/dev/gpoGPRS_POWER", 0);
}

// 可打印字符串转换为字节数据
// 如："C8329BFD0E01" --> {0xC8, 0x32, 0x9B, 0xFD, 0x0E, 0x01}
// pSrc: 源字符串指针
// pDst: 目标数据指针
// nSrcLength: 源字符串长度
// 返回: 目标数据长度
int gsmString2Bytes(const char* pSrc, unsigned char* pDst, int nSrcLength) {
    int i;
    //	fprintf(stderr,"gsmString2Bytes:nSrcLength=%d,pSrc=%s\n",nSrcLength,pSrc);
    for (i = 0; i < nSrcLength; i += 2) {
        // 输出高4位
        if (*pSrc >= '0' && *pSrc <= '9') {
            *pDst = (*pSrc - '0') << 4;
        } else {
            *pDst = (*pSrc - 'A' + 10) << 4;
        }
        //       fprintf(stderr,"pSrc=%c,pDsth=%02x\n",*pSrc,*pDst);
        pSrc++;

        // 输出低4位
        if (*pSrc >= '0' && *pSrc <= '9') {
            *pDst |= *pSrc - '0';
        } else {
            *pDst |= *pSrc - 'A' + 10;
        }
        //       fprintf(stderr,"pSrc=%d,pDstl=%02x\n",*pSrc,*pDst);
        pSrc++;
        pDst++;
    }
    // 返回目标数据长度
    return nSrcLength / 2;
}

// 两两颠倒的字符串转换为正常顺序的字符串
// 如："683158812764F8" --> "8613851872468"a
// pSrc: 源字符串指针
// pDst: 目标字符串指针
// nSrcLength: 源字符串长度
// 返回: 目标字符串长度
int gsmSerializeNumbers(const char* pSrc, char* pDst, int nSrcLength) {
    int nDstLength; // 目标字符串长度
                    //    char ch;          // 用于保存一个字符
    int i;

    // 复制串长度
    nDstLength = nSrcLength;

    //    fprintf(stderr,"pSrc=%s\n",pSrc);
    // 两两颠倒
    for (i = 0; i < nSrcLength; i++) {
        if (i % 2 == 0) {
            pDst[i + 1] = pSrc[i];
        } else {
            pDst[i - 1] = pSrc[i];
        }
    }
    //    fprintf(stderr,"pDst=%s\n",pDst);
    // 最后的字符是'F'吗？
    if (pDst[i - 1] == 'F') {
        //	    fprintf(stderr,"pDst[%d]=%c\n",i-1,pDst[i-1]);
        pDst[i - 1] = '\0'; // 输出字符串加个结束符
        nDstLength--;       // 目标字符串长度减1
    }
    // 返回目标字符串长度
    return nDstLength;
}

// 7-bit解码
// pSrc: 源编码串指针
// pDst: 目标字符串指针
// nSrcLength: 源编码串长度
// 返回: 目标字符串长度
int gsmDecode7bit(const unsigned char* pSrc, char* pDst, int nSrcLength) {
    int nSrc;            // 源字符串的计数值
    int nDst;            // 目标解码串的计数值
    int nByte;           // 当前正在处理的组内字节的序号，范围是0-6
    unsigned char nLeft; // 上一字节残余的数据

    // 计数值初始化
    nSrc = 0;
    nDst = 0;

    // 组内字节序号和残余数据初始化
    nByte = 0;
    nLeft = 0;

    // 将源数据每7个字节分为一组，解压缩成8个字节
    // 循环该处理过程，直至源数据被处理完
    // 如果分组不到7字节，也能正确处理
    while (nSrc < nSrcLength) {
        //    	fprintf(stderr,"pSrc=%02x\n",*pSrc);
        // 将源字节右边部分与残余数据相加，去掉最高位，得到一个目标解码字节
        *pDst = ((*pSrc << nByte) | nLeft) & 0x7f;
        //        fprintf(stderr,"pDst=%02x\n",*pDst);
        // 将该字节剩下的左边部分，作为残余数据保存起来
        nLeft = *pSrc >> (7 - nByte);
        // 修改目标串的指针和计数值
        pDst++;
        nDst++;
        // 修改字节计数值
        nByte++;
        // 到了一组的最后一个字节
        if (nByte == 7) {
            // 额外得到一个目标解码字节
            *pDst = nLeft;
            // 修改目标串的指针和计数值
            pDst++;
            nDst++;
            // 组内字节序号和残余数据初始化
            nByte = 0;
            nLeft = 0;
        }
        // 修改源串的指针和计数值
        pSrc++;
        nSrc++;
    }
    *pDst = '\0';
    // 返回目标串长度
    return nDst;
}

// PDU解码，用于接收、阅读短消息
// pSrc: 源PDU串指针
// pDst: 目标PDU参数指针
// 返回: 用户信息串长度
int gsmDecodePdu(const char* pSrc, SM_PARAM* pDst) {
    int nDstLength = 0;     // 目标PDU串长度
    unsigned char tmp;      // 内部用的临时字节变量
    unsigned char buf[512]; // 内部用的缓冲区

    // SMSC地址信息段
    gsmString2Bytes(pSrc, &tmp, 2);
    asyslog(LOG_DEBUG, "SMSC地址信息段长度=%d", tmp);

    tmp = (tmp - 1) * 2;
    asyslog(LOG_DEBUG, "SMSC号码串长度=%d", tmp);
    pSrc += 4;                                 // 指针后移
    gsmSerializeNumbers(pSrc, pDst->SCA, tmp); // 转换SMSC号码到目标PDU串
    pSrc += tmp;                               // 指针后移
    // TPDU段基本参数、回复地址等
    gsmString2Bytes(pSrc, &tmp, 2); // 取基本参数
    pSrc += 2;                      // 指针后移
    // 包含回复地址，取回复地址信息
    gsmString2Bytes(pSrc, &tmp, 2); // 取长度
    if (tmp & 1)
        tmp += 1;                              // 调整奇偶性
    pSrc += 4;                                 // 指针后移
    gsmSerializeNumbers(pSrc, pDst->TPA, tmp); // 取TP-RA号码
                                               //        fprintf(stderr,"TPA=%s\n",pDst->TPA);
    pSrc += tmp;                               // 指针后移

    // TPDU段协议标识、编码方式、用户信息等
    gsmString2Bytes(pSrc, (unsigned char*)&pDst->TP_PID, 2); // 取协议标识(TP-PID)
    asyslog(LOG_DEBUG, "TP_PID=%d", pDst->TP_PID);
    pSrc += 2;                                               // 指针后移
    gsmString2Bytes(pSrc, (unsigned char*)&pDst->TP_DCS, 2); // 取编码方式(TP-DCS)
    pDst->TP_DCS = pDst->TP_DCS & 0x0c;
    asyslog(LOG_DEBUG, "TP_DCS=%x,接收号码：%s", pDst->TP_DCS, pDst->TPA);
    pSrc += 2;                                    // 指针后移
    gsmSerializeNumbers(pSrc, pDst->TP_SCTS, 14); // 服务时间戳字符串(TP_SCTS)
    pSrc += 14;                                   // 指针后移
    gsmString2Bytes(pSrc, &tmp, 2);               // 用户信息长度(TP-UDL)
    pSrc += 2;                                    // 指针后移

    if (pDst->TP_DCS == GSM_7BIT) {
        // 7-bit解码
        nDstLength = gsmString2Bytes(pSrc, buf, tmp & 7 ? (int)tmp * 7 / 4 + 2 : (int)tmp * 7 / 4); // 格式转换
        if (nDstLength > 161) {
            asyslog(LOG_DEBUG, "7-Bit:错误短信长度>161,返回");
            return 0;
        }
        gsmDecode7bit(buf, pDst->TP_UD, nDstLength); // 转换到TP-DU
        asyslog(LOG_DEBUG, "7-Bit解码数据长度=%d", nDstLength);
        asyslog(LOG_DEBUG, "解码后的数据%s", pDst->TP_UD);
        nDstLength = tmp;
    } else if (pDst->TP_DCS == GSM_UCS2) {
        fprintf(stderr, "UCS2编码格式,不处理");
    } else {
        // 8-bit解码
        nDstLength = gsmString2Bytes(pSrc, buf, tmp * 2); // 格式转换
        asyslog(LOG_DEBUG, "nDstLength=%d", nDstLength);
        if (nDstLength > 161) {
            asyslog(LOG_DEBUG, "8-Bit:错误短信长度>161,返回");
            return 0;
        }
        nDstLength = memcpy(buf, pDst->TP_UD, nDstLength); // 转换到TP-DU
        asyslog(LOG_DEBUG, "8-Bit解码数据长度=%d", nDstLength);
    }

    // 返回目标字符串长度
    return nDstLength;
}

void checkSms(int port) {
    if (port < 0) {
        return;
    }
    for (int timeout = 0; timeout < 50; timeout++) {
        char Mrecvbuf[512];

        SendATCommand("\rAT+CMGL=0\r", 11, port);
        delay(3000);
        memset(Mrecvbuf, 0, 512);
        RecieveFromComm(Mrecvbuf, 512, port);

        char cimi[256];
        memset(cimi, 0x00, sizeof(cimi));
        char* position = strstr(Mrecvbuf, "0891683108");
        if (position != Mrecvbuf && position != NULL) {
            if (sscanf(position, "%[0-9|A-F]", cimi) == 1) {
                asyslog(LOG_INFO, "CMGR = %s\n", cimi);
                SM_PARAM smpara;
                int len = gsmDecodePdu(position, &smpara);
                fprintf(stderr, "数据：R(%d)---\n", len);
                for (int i = 0; i < len; i++) {
                    fprintf(stderr, "%02x ", smpara.TP_UD[i]);
                }
                fprintf(stderr, "\n---\n");
                break;
            }
        } else {
            asyslog(LOG_NOTICE, "无短信内容，等待下次检查...");
            break;
        }
    }
}

int absoluteKill(char* name, int timeout) {
    pid_t pids[128];
    for (int i = 0; i < timeout; i++) {
        memset(pids, 0x00, sizeof(pids));
        if (prog_find_pid_by_name(name, pids) >= 1) {
            char command[64];
            memset(command, 0x00, sizeof(command));
            sprintf(command, "kill %d", pids[0]);
            system(command);
            asyslog(LOG_INFO, "正在停止进程[%s],进程号[%d],使用的终止命令[%s]", name, pids[0], command);
        } else {
            return 0;
        }
        sleep(1);
    }
    return -1;
}

void* ATWorker(void* args) {
    CLASS25* class25 = (CLASS25*)args;
    int sMux0        = -1;
    int sMux1        = -1;

    while (1) {
        if (GetOnlineType() != 0) {
            goto wait;
        }

        /*
         * 清除外部程序
         */
        asyslog(LOG_INFO, "清除外部程序...");
        system("ppp-off");
        absoluteKill("ftpget", 15);
        absoluteKill("gsmMuxd", 15);

        gpofun("/dev/gpoCSQ_GREEN", 0);
        gpofun("/dev/gpoCSQ_RED", 0);

        if (GetOnlineType() != 0) {
            goto wait;
        }

        /*
         * 重置模块状态
         */
        asyslog(LOG_INFO, "重置模块状态...");
        gpofun("/dev/gpoGPRS_POWER", 0);
        sleep(5);
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

        if (GetOnlineType() != 0) {
            goto wait;
        }

        /*
         * 处理AT参数，等待注册状态。
         */
        asyslog(LOG_INFO, "打开串口复用模块");
        system("mux.sh &");
        sleep(10);

        sMux0 = OpenMuxCom(0, 115200, (unsigned char*)"none", 1, 8); // 0
        sMux1 = OpenMuxCom(1, 115200, (unsigned char*)"none", 1, 8);
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
            if (sscanf(Mrecvbuf, "%*[^\n]\n%[^\n]\n%[^\n]\n%[^\n]\n%[^\n]\n%[^\n]\n%[^\n]", INFO[0], INFO[1], INFO[2], INFO[3], INFO[4], INFO[5]) == 6) {
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

        int reg_ok = 0;
        for (int timeout = 0; timeout < 80; timeout++) {
            char Mrecvbuf[128];

            SendATCommand("\rAT+CREG?\r", 10, sMux0);
            delay(1000);
            memset(Mrecvbuf, 0, 128);
            RecieveFromComm(Mrecvbuf, 128, sMux0);

            int k, l;
            if (sscanf(Mrecvbuf, "%*[^:]: %d,%d", &k, &l) == 2) {
                asyslog(LOG_INFO, "GprsCREG = %d,%d\n", k, l);
                if (l == 1 || l == 5) {
                    reg_ok = 1;
                    break;
                }
            }
        }
        if (reg_ok == 0) {
            asyslog(LOG_INFO, "注册网络失败");
            goto err;
        }

        char* cimiType[] = {
            "46003", "46011",
        };

        int callType = 1;

        for (int timeout = 0; timeout < 50; timeout++) {
            char Mrecvbuf[128];

            SendATCommand("\rAT+CIMI\r", 9, sMux0);
            delay(1000);
            memset(Mrecvbuf, 0, 128);
            RecieveFromComm(Mrecvbuf, 128, sMux0);

            char cimi[64];
            memset(cimi, 0x00, sizeof(cimi));
            if (sscanf((char*)&Mrecvbuf[0], "%*[^0-9]%[0-9]", cimi) == 1) {
                asyslog(LOG_INFO, "CIMI = %s\n", cimi);
                for (int i = 0; i < 2; i++) {
                    if (strncmp(cimiType[i], cimi, 5) == 0) {
                        callType = 3;
                        break;
                    }
                }
                break;
            }
        }
        close(sMux0);

        if (GetOnlineType() != 0) {
            goto wait;
        }

        /*
         * 运行拨号脚本
         */
        if (callType == 1) {
            asyslog(LOG_INFO, "拨号类型：GPRS\n");
            system("pppd call gprs &");
        } else {
            asyslog(LOG_INFO, "拨号类型：CDMA2000\n");
            system("pppd call cdma2000 &");
        }

        for (int i = 0; i < 50; i++) {
            sleep(1);
            if (tryifconfig(class25) == 1) {
                //拨号成功，存储参数，以备召唤
                saveCoverClass(0x4500, 0, class25, sizeof(CLASS25), para_init_save);
                break;
            }
        }
        sleep(20);

    wait:
        //等待在线状态为“否”，重新拨号
        while (1) {
            static int step = 0;
            if (step % 60 == 0) {
                checkSms(sMux1);
                step = 0;
            }
            step++;
            sleep(1);
            if (GetOnlineType() == 0) {
                goto err;
            }
        }

    err:
        sleep(1);
        continue;
    }

    return NULL;
}

void CreateATWorker(void* clientdata) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_t temp_key;
    pthread_create(&temp_key, &attr, ATWorker, clientdata);
}
