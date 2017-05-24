/*
 * dealProcess.c
 *
 *  Created on: 2014-4-4
 *      Author: Administrator
 */

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdlib.h>
#include <att7022e.h>
#include <sys/wait.h>
#include <math.h>
#include "dlt645.h"
#include "def645.h"
#include "Shmem.h"
#include "PublicFunction.h"

extern void InitACSCoef();
#define MsgSendOverTime 3
#define MC 6400

INT8U 	acs_check_end = 0;

pid_t pid;
INT32S comfd;        //通讯口打开返回句柄

INT8U RecvBuf[256];    //接收报文缓冲区
INT8U SendBuf[256];    //发送报文缓冲区
INT8U TempBuf[256];
INT8U RecvLen;    //接收缓冲区长度
INT8U SendLen;    //发送缓冲区长度


INT8U addr[6];//表地址，低在前，高在后
INT8U bcAddr_AA[6] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
INT8U bcAddr_99[6] = {0x99, 0x99, 0x99, 0x99, 0x99, 0x99};

double P[3], Q[3], U[3], I[3];//三相有功、无功、电压、电流

extern ProgramInfo *JProgramInfo;
extern mqd_t mqd_main;

extern INT16S ReceDataFrom485(INT32S fd, INT8U *str);

extern void SendDataTo485(INT32S fd, INT8U *sendbuf, INT16U sendlen);

int MIN(int x, int y) {
    return (x < y) ? x : y;
}

int MAX(int x, int y) {
    return (x > y) ? x : y;
}


//保留到10的bit次方位。如bit=-1，保留到十分位
double deal45(double x, INT8S bit) {
    x = x + 5 * pow(10, bit - 1);
    x = x * pow(10, 0 - bit);
    long t = (long) x;
    x = t * pow(10, bit);
    return x;
}

INT32S getIntBits(INT32U dint) {
    int ret = 0;
    do {
        dint /= 10;
        ret++;
    } while (dint > 0);
    return ret;
}

INT32S int32u2bcd(INT32U dint32, INT8U *bcd, ORDER order) {
    INT8U i;
    INT16U mod = 1;
    INT32S len = 0;
    len = getIntBits(dint32);
    if (len % 2 > 0)
        len++;
    if (order == positive) {
        for (i = 0; i < len / 2; i++) {
            mod = dint32 % 100;
            bcd[len / 2 - i - 1] = (mod / 10);
            bcd[len / 2 - i - 1] = (bcd[len / 2 - i - 1] << 4) + (mod % 10);
            dint32 = dint32 / 100;
        }
    } else if (order == inverted) {
        for (i = 0; i < len / 2; i++) {
            mod = dint32 % 100;
            bcd[i] = (mod / 10);
            bcd[i] = (bcd[i] << 4) + (mod % 10);
            dint32 = dint32 / 100;
        }
    } else
        return -1;
//	if (dint32 < 0) {
//		bcd[len/2 - 1] = bcd[len/2 - 1] | 0x80;
//	}
    return len / 2;
}

//反转buff
INT8S reversebuff(INT8U *buff, INT32U len, INT8U *invbuff) {
    if (buff == NULL)
        return -1;
    if (len == 0)
        return -2;
    if (invbuff == NULL)
        return -3;
    INT8U *buftmp = (INT8U *) malloc(len);
    memcpy(buftmp, buff, len);
    INT32U i = 0;
    for (i = 0; i < len; i++) {
        invbuff[i] = buftmp[len - i - 1];
    }
    free(buftmp);
    buftmp = NULL;
    return 0;
}
/*================================================================================================================
 该函数经过调试，消除小数部分最后一位有误差的现象(	FP64 a = 0.12;INT8U b = a*100;fprintf(stderr,"\n b:%d",b);-------->b==11)
 使用注意：
             1.小数部分精度最好不要超过小数点后8位。
             2. 使用的时候需要判断if( double2bcd() > 0)才可使用。如下使用方式是错误的：
                  FP64 doub =123.26141411;double2bcd(doub,bcd,1,4);因为整数部分123第三个参数应该传入2。
             3.
                 如果double2bcd参数dbl有可能是负数，则外部bcd的需要定义成bcd[ibytes+dbytes+1] 因为bcd[0]需要存放符号位0xff，
                 如果dbl只能是正数，则外部bcd的定义需要定义成bcd[ibytes+dbytes]即可。
  在调试函数都过程中发现的现象：
  1.
 =============================================================================================================*/

#define wucha 0.0000001

INT32S double2bcd(FP64 dbl, INT8U *bcd, const INT32U ibytes, const INT32U dbytes) {
    if (bcd == NULL) {
        fprintf(stderr, "\n bcd is null!");
        return -1;
    }
    memset(bcd, 0, ibytes + dbytes);
    INT8U zs[7] = {0};
    INT8U xs[7] = {0};
    INT8U j = 0;
    //整数部分处理
    FP64 dbl_abs = 0;
    dbl_abs = fabs(dbl);
//	INT32U dint = (INT32U)dbl_abs;// dbl_abs=3.0   -------->INT32U dint = (INT32U)dbl_abs;---------->dint=2。使用下句解决
    INT32U dint = (INT32U)((float) dbl_abs);

    INT32S lint = getIntBits(dint);//整数部分位数
    if (lint % 2 != 0)
        lint++;//lint/2
    if (ibytes < lint / 2) {
        fprintf(stderr, "\n use error! the (lint/2)>ibytes!!!error!!!");
        return -2;
    }
    INT32S lenl = MAX(lint / 2, ibytes);
    INT32S lenl_min = MIN(lint / 2, ibytes);
    int32u2bcd(dint, zs, positive);
//	INT8U i = 0;
//	for(i=0;i<lint/2;i++)
//		fprintf(stderr,"\n zhengshu[%d]:%02x",i,zs[i]);
//	fprintf(stderr,"\n ------------------------------------------------");


    //小数部分处理
    FP64 dotp = (dbl_abs - dint);//对dotp=0的特殊处理
//	if(dotp == 0)
//		return (ibytes+dbytes);
    if ((-wucha <= dotp) && (dotp <= wucha)) {
        memcpy(&bcd[ibytes - lint / 2], &zs[0], lenl_min);//拷贝整数部分
        memcpy(&bcd[lenl], &xs[0], dbytes);//拷贝小数部分
        if (dbl < 0)
            bcd[0] = 0xff;
        return (ibytes + dbytes);
    }
    INT32U _dbytes = dbytes + 1;
    dotp = dotp + (1 / pow(10, _dbytes * 2));


//===============将下面#if 0 ~ #endif之间的代码注释，改为如下
    for (j = 0; j < _dbytes; j++) {
        dotp = dotp * 100;
        INT8U _dotp = (INT8U) dotp;
        int32u2bcd(_dotp, &xs[j], positive);
        dotp -= _dotp;
    }
#if 0
    INT32U xiaoshu =(INT32U)(dotp*pow(10,_dbytes*2));
//	if(xiaoshu > 1)//特殊处理
//		xiaoshu -= 1;
    INT32S linx = getIntBits(xiaoshu);//小数部分扩大之后位数
    if(linx%2!=0)
        linx ++;//linx/2
    int32u2bcd(xiaoshu, xs,positive);
//	for(i=0;i<linx/2;i++)
//	fprintf(stderr,"\n xiaoshu[%d]:%02x",i,xs[i]);
//	fprintf(stderr,"\n ------------------------------------------------");
#endif

    memcpy(&bcd[ibytes - lint / 2], &zs[0], lenl_min);//拷贝整数部分
    memcpy(&bcd[lenl], &xs[0], dbytes);//拷贝小数部分
    if (dbl < 0)
        bcd[0] = 0xff;
//	for(i=0;i<(ibytes+dbytes);i++)
//	fprintf(stderr,"\n bcd[%d]:%02x",i,bcd[i]);
//	fprintf(stderr,"\n ------------------------------------------------");
    return (ibytes + dbytes);
}

INT8S bcd2int32u(INT8U *bcd, INT8U len, ORDER order, INT32U *dint) {
    INT32S i;
    if (bcd == NULL)
        return -1;
    if (len <= 0)
        return -2;
    *dint = 0;
    if (order == inverted) {
        for (i = len; i > 0; i--) {
            *dint = (*dint * 10) + ((bcd[i - 1] >> 4) & 0xf);
            *dint = (*dint * 10) + (bcd[i - 1] & 0xf);
        }
    } else if (order == positive) {
        for (i = 0; i < len; i++) {
            *dint = (*dint * 10) + ((bcd[i] >> 4) & 0xf);
            *dint = (*dint * 10) + (bcd[i] & 0xf);
        }
    } else
        return -3;
    return 0;
}


long pgpl_fork(void) {
    pid_t pid;
    INT32S status;

    if (!(pid = fork())) { //child run
        // fork first time, in child process
        switch (fork()) {
            case 0:
                return 0;
            case -1:
                _exit(errno); /* assumes all errnos are <256 */
                break;
            default:
                _exit(0);
                break;
        }
    }
    //parent

    if (pid < 0 || waitpid(pid, &status, 0) < 0)
        return -1;

    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) == 0)
//            return 1;
            return pid;
        else
            errno = WEXITSTATUS(status);
    } else
        errno = EINTR; /* well, interrupted */
    return -1;
}


//返回值：0-成功；1-失败
INT8S execACS(INT8U flag) {
    pid = pgpl_fork();
    if (pid == -1) {
        fprintf(stderr, "进程创建失败！！！\n");
        return 1;
    } else if (pid == 0)//子进程代码运行部分
    {
        char param[12][16];
        sprintf(param[0], "%f", P[0]);
        sprintf(param[1], "%f", P[1]);
        sprintf(param[2], "%f", P[2]);

        if (JProgramInfo->dev_info.WireType == 0x1200)//接线方式，0x1200：三相三，0x0600：三相四
        {
            fprintf(stderr, "三相三校表！！！\n");
            sprintf(param[3], "%f", Q[0]);
            sprintf(param[4], "-%f", Q[1]);
            sprintf(param[5], "-%f", Q[2]);
        } else {
            fprintf(stderr, "三相四校表！！！\n");
            sprintf(param[3], "%f", Q[0]);
            sprintf(param[4], "%f", Q[1]);
            sprintf(param[5], "%f", Q[2]);
        }
        fprintf(stderr, "Q=%s, %s, %s\n", param[3], param[4], param[5]);

        sprintf(param[6], "%f", U[0]);
        sprintf(param[7], "%f", U[1]);
        sprintf(param[8], "%f", U[2]);
        sprintf(param[9], "%f", I[0]);
        sprintf(param[10], "%f", I[1]);
        sprintf(param[11], "%f", I[2]);


        if (flag == 1)//3A
        {
            execlp("cj", "cj", "acs", "acreg", param[0], param[1], param[2], param[3], param[4], param[5],
                   param[6], param[7], param[8], param[9], param[10], param[11], NULL);
        } else if (flag == 2)//1.5A
        {
            execlp("cj", "cj", "acs", "acphase", param[0], param[1], param[2], param[3], param[4], param[5],
                   param[6], param[7], param[8], param[9], param[10], param[11], NULL);
        } else if (flag == 3)//0.15A
        {
            execlp("cj", "cj", "acs", "acphase0", param[0], param[1], param[2], param[3], param[4], param[5],
                   param[6], param[7], param[8], param[9], param[10], param[11], NULL);
        } else if (flag == 4)//0.3A
        {
            execlp("cj", "cj", "acs", "acphase", param[0], param[1], param[2], param[3], param[4], param[5],
                   param[6], param[7], param[8], param[9], param[10], param[11], NULL);
        } else if (flag == 5)//u
        {
            execlp("cj", "cj", "acs", "checku", param[6], NULL);

        }

        exit(1);
        return 0;
    } else if (pid > 0)//父进程代码运行部分
    {
        sleep(15);
        return 1;
    }
    return 1;
}

//读数据
void getData(FORMAT07 format07_down) {}

//广播校时
void broadCast(FORMAT07 format07) {
    fprintf(stderr, "\n广播校时 %d-%d-%d, %d:%d:%d\n", format07.Time[5] + 2000,
            format07.Time[4], format07.Time[3], format07.Time[2], format07.Time[1], format07.Time[0]);

    int rtc;
    struct tm _tm;
    struct timeval tv;
    _tm.tm_sec = format07.Time[0];
    _tm.tm_min = format07.Time[1];
    _tm.tm_hour = format07.Time[2];
    _tm.tm_mday = format07.Time[3];
    _tm.tm_mon = format07.Time[4] - 1;
    _tm.tm_year = format07.Time[5] + 2000 - 1900;
    _tm.tm_isdst = 0;
    tv.tv_sec = mktime(&_tm);
    tv.tv_usec = 0;
    settimeofday(&tv, (struct timezone *) 0);
    rtc = open(DEV_RTC, O_RDWR);
    ioctl(rtc, RTC_SET_TIME, &_tm);
    close(rtc);

    char cmd[10];
    sprintf(cmd, "%s", "date");
    system(cmd);
    fprintf(stderr, "\n\n");
}


//最大需量清零
void clearXL() {}


//电表清零
void clearAll() {}


//解析报文得到有功、无功、电压、电流值
void Msg2AcsValue(FORMAT07 format07) {
    INT32U tempdata[48];
    INT8U i;
    for (i = 0; i < 48; i++) {
        bcd2int32u(&format07.Data[i], 1, positive, &tempdata[i]);
    }

    P[0] = tempdata[3] * 100 + tempdata[2] + (double) tempdata[1] / 100 + (double) tempdata[0] / 10000;
    P[1] = tempdata[7] * 100 + tempdata[6] + (double) tempdata[5] / 100 + (double) tempdata[4] / 10000;
    P[2] = tempdata[11] * 100 + tempdata[10] + (double) tempdata[9] / 100 + (double) tempdata[8] / 10000;
    Q[0] = tempdata[15] * 100 + tempdata[14] + (double) tempdata[13] / 100 + (double) tempdata[12] / 10000;
    Q[1] = tempdata[19] * 100 + tempdata[18] + (double) tempdata[17] / 100 + (double) tempdata[16] / 10000;
    Q[2] = tempdata[23] * 100 + tempdata[22] + (double) tempdata[21] / 100 + (double) tempdata[20] / 10000;
    U[0] = tempdata[27] * 100 + tempdata[26] + (double) tempdata[25] / 100 + (double) tempdata[24] / 10000;
    U[1] = tempdata[31] * 100 + tempdata[30] + (double) tempdata[29] / 100 + (double) tempdata[28] / 10000;
    U[2] = tempdata[35] * 100 + tempdata[34] + (double) tempdata[33] / 100 + (double) tempdata[32] / 10000;
    I[0] = tempdata[39] * 100 + tempdata[38] + (double) tempdata[37] / 100 + (double) tempdata[36] / 10000;
    I[1] = tempdata[43] * 100 + tempdata[42] + (double) tempdata[41] / 100 + (double) tempdata[40] / 10000;
    I[2] = tempdata[47] * 100 + tempdata[46] + (double) tempdata[45] / 100 + (double) tempdata[44] / 10000;

    fprintf(stderr, "P=%0.4f, %0.4f, %0.4f\n", P[0], P[1], P[2]);
    fprintf(stderr, "Q=%0.4f, %0.4f, %0.4f\n", Q[0], Q[1], Q[2]);
    fprintf(stderr, "U=%0.4f, %0.4f, %0.4f\n", U[0], U[1], U[2]);
    fprintf(stderr, "I=%0.4f, %0.4f, %0.4f\n", I[0], I[1], I[2]);
}


//有功、无功、电压、电流值转成报文
void AcsValue2Msg(FORMAT07 format07_down) {
    INT8U tempdata[4];
    FORMAT07 format07_up;
    memcpy(format07_up.Addr, addr, 6);
    format07_up.Ctrl = 0x91;
    format07_up.Length = 0x34;

    double tmpValue;
    tmpValue = deal45((double) JProgramInfo->ACSRealData.Pa / P_COEF, -1);
    double2bcd(tmpValue, &tempdata[0], 2, 2);
    reversebuff(tempdata, 4, &format07_up.Data[0]);

    tmpValue = deal45((double) JProgramInfo->ACSRealData.Pb / P_COEF, -1);
    double2bcd(tmpValue, &tempdata[0], 2, 2);
    reversebuff(tempdata, 4, &format07_up.Data[4]);

    tmpValue = deal45((double) JProgramInfo->ACSRealData.Pc / P_COEF, -1);
    double2bcd(tmpValue, &tempdata[0], 2, 2);
    reversebuff(tempdata, 4, &format07_up.Data[8]);

    tmpValue = deal45((double) JProgramInfo->ACSRealData.Qa / Q_COEF, -1);
    double2bcd(tmpValue, &tempdata[0], 2, 2);
    reversebuff(tempdata, 4, &format07_up.Data[12]);

    tmpValue = deal45((double) JProgramInfo->ACSRealData.Qb / Q_COEF, -1);
    double2bcd(tmpValue, &tempdata[0], 2, 2);
    reversebuff(tempdata, 4, &format07_up.Data[16]);

    tmpValue = deal45((double) JProgramInfo->ACSRealData.Qc / Q_COEF, -1);
    double2bcd(tmpValue, &tempdata[0], 2, 2);
    reversebuff(tempdata, 4, &format07_up.Data[20]);

    tmpValue = deal45((double) JProgramInfo->ACSRealData.Ua / U_COEF, -1);
    double2bcd(tmpValue, &tempdata[0], 2, 2);
    reversebuff(tempdata, 4, &format07_up.Data[24]);

    tmpValue = deal45((double) JProgramInfo->ACSRealData.Ub / U_COEF, -1);
    double2bcd(tmpValue, &tempdata[0], 2, 2);
    reversebuff(tempdata, 4, &format07_up.Data[28]);

    tmpValue = deal45((double) JProgramInfo->ACSRealData.Uc / U_COEF, -1);
    double2bcd(tmpValue, &tempdata[0], 2, 2);
    reversebuff(tempdata, 4, &format07_up.Data[32]);

    tmpValue = deal45((double) JProgramInfo->ACSRealData.Ia / I_COEF, -2);
    double2bcd(tmpValue, &tempdata[0], 2, 2);
    reversebuff(tempdata, 4, &format07_up.Data[36]);

    tmpValue = deal45((double) JProgramInfo->ACSRealData.Ib / I_COEF, -2);
    double2bcd(tmpValue, &tempdata[0], 2, 2);
    reversebuff(tempdata, 4, &format07_up.Data[40]);

    tmpValue = deal45((double) JProgramInfo->ACSRealData.Ic / I_COEF, -2);
    double2bcd(tmpValue, &tempdata[0], 2, 2);
    reversebuff(tempdata, 4, &format07_up.Data[44]);

    SendLen = composeProtocol07(&format07_up, SendBuf);
    SendDataTo485(comfd, SendBuf, SendLen);
}


//对时：flag=1，日期；flag=2，时间
void setTime(INT8U flag, FORMAT07 format07) {
    int rtc;
    struct tm _tm;
    struct timeval tv;

    TS tm;
    TSGet(&tm);

    if (flag == 1) {
        INT32U year, month, day;
        bcd2int32u(&format07.Data[11], 1, inverted, &year);
        bcd2int32u(&format07.Data[10], 1, inverted, &month);
        bcd2int32u(&format07.Data[9], 1, inverted, &day);

        fprintf(stderr, "\n对时-日期： %d-%d-%d\n", year + 2000, month, day);
        _tm.tm_sec = tm.Sec;
        _tm.tm_min = tm.Minute;
        _tm.tm_hour = tm.Hour;
        _tm.tm_mday = day;
        _tm.tm_mon = month - 1;
        _tm.tm_year = year + 2000 - 1900;
    } else if (flag == 2) {
        INT32U hour, minute, sec;
        bcd2int32u(&format07.Data[10], 1, inverted, &hour);
        bcd2int32u(&format07.Data[9], 1, inverted, &minute);
        bcd2int32u(&format07.Data[8], 1, inverted, &sec);

        fprintf(stderr, "\n对时-时间：%d:%d:%d\n", hour, minute, sec);
        _tm.tm_sec = sec;
        _tm.tm_min = minute;
        _tm.tm_hour = hour;
        _tm.tm_mday = tm.Day;
        _tm.tm_mon = tm.Month - 1;
        _tm.tm_year = tm.Year - 1900;

        fprintf(stderr, "year=%d, mon=%d\n", _tm.tm_year, _tm.tm_mon);
    }

    _tm.tm_isdst = 0;
    tv.tv_sec = mktime(&_tm);
    tv.tv_usec = 0;
    settimeofday(&tv, (struct timezone *) 0);
    rtc = open(DEV_RTC, O_RDWR);
    ioctl(rtc, RTC_SET_TIME, &_tm);
    close(rtc);

    char cmd[10];
    sprintf(cmd, "%s", "date");
    system(cmd);
    fprintf(stderr, "\n\n");

    SendBuf[0] = SendBuf[7] = 0x68;
    memcpy(&SendBuf[1], addr, 6);
    SendBuf[8] = 0x94;
    SendBuf[9] = 0x00;
    SendBuf[10] = getCS645(SendBuf, 10);
    SendBuf[11] = 0x16;
    //SendDataTo485(comfd, SendBuf, 12);
}


//校表
void setACS(FORMAT07 format07) {
    char cmd[100];
    memset(cmd, 0, 100);
    int i;

    fprintf(stderr, "ac_chip_type=%x\n", JProgramInfo->dev_info.ac_chip_type);
    switch (JProgramInfo->dev_info.ac_chip_type) {
        case 1://==1： ATT7022D-E芯片
            for (i = 0; i < sizeof(dataFlag_2d) / sizeof(dataFlagInfo); i++) {
                if (memcmp(format07.DI, dataFlag_2d[i].dataflag, 4) == 0) {
                    switch (dataFlag_2d[i].index) {
                        case 1://3A校表
                        {
                            fprintf(stderr, "3A校表......\n");
                            Msg2AcsValue(format07);
                            execACS(1);//校表
                            break;
                        }
                        case 2://1.5A校表
                        {
                            fprintf(stderr, "1.5A校表......\n");
                            Msg2AcsValue(format07);
                            execACS(2);//校表
                            break;
                        }
                        case 3://0.15A校表
                        {
                            fprintf(stderr, "0.15A校表......\n");
                            Msg2AcsValue(format07);
                            execACS(3);//校表
                            break;
                        }
                        case 4://0.3A校表
                        {
                            fprintf(stderr, "0.3A校表......\n");
                            Msg2AcsValue(format07);
                            execACS(4);//校表
                            break;
                        }
                    }
                    fprintf(stderr, "校表结束!!!\n");
                    system("reboot");
                    break;
                }
            }
            break;
        case 0x7022E0:    //ATT7022E-D芯片
            fprintf(stderr, "ATT7022E-D芯片　校表\n");
            for (i = 0; i < sizeof(dataFlag_3d) / sizeof(dataFlagInfo); i++) {
                if (memcmp(format07.DI, dataFlag_3d[i].dataflag, 4) == 0) {
                    switch (dataFlag_3d[i].index) {
                        case 1://3A校表
                        {
                            fprintf(stderr, "3A校表......\n");
                            Msg2AcsValue(format07);
                            execACS(1);//校表
                            break;
                        }
                        case 2://1.5A校表
                        {
                            fprintf(stderr, "1.5A校表......\n");
                            Msg2AcsValue(format07);
                            execACS(2);//校表
                            break;
                        }
                        case 3://0.15A校表
                        {
                            fprintf(stderr, "0.15A校表......\n");
                            Msg2AcsValue(format07);
                            execACS(3);//校表
                            break;
                        }
                        case 4://0.3A校表
                        {
                            fprintf(stderr, "0.3A校表......\n");
                            Msg2AcsValue(format07);
                            execACS(4);//校表
                            break;
                        }
                    }
                    fprintf(stderr, "校表结束!!!\n");
                    sleep(3);
                    system("reboot");
                    break;
                }
            }

            break;
        case 0x820900:    //RN8029芯片，II型集中器
            fprintf(stderr, "II型集中器校表\n");
            for (i = 0; i < sizeof(dataFlag_4d) / sizeof(dataFlagInfo); i++) {
                if (memcmp(format07.DI, dataFlag_4d[i].dataflag, 4) == 0) {
                    switch (dataFlag_4d[i].index) {
                        case 1://电压校正
                        {
                            fprintf(stderr, "电压校表......\n");
                            Msg2AcsValue(format07);
                            execACS(5);//校表
                            fprintf(stderr, "校表结束!!!\n");
                            sleep(2);
                            acs_check_end = 1;
                            InitACSPara();
  //                          InitACSCoef();			//重新读取参数，并不重新启动，控制闪灯来判断
                            sleep(3);
//                            system("reboot");
                            break;
                        }
                        case 2://电压读取
                        {
                            fprintf(stderr, "电压读取返回......\n");
                            INT8U bcd[4];
                            FP64 value = (double) JProgramInfo->ACSRealData.Ua;
                            fprintf(stderr, "value=%f\n", value);
                            double2bcd(value, bcd, 2, 1);
                            fprintf(stderr, "bcd=%02x_%02x_%02x_%02x\n", bcd[0], bcd[1], bcd[2], bcd[3]);
                            reversebuff(bcd, 3, format07.Data);
                            fprintf(stderr, "Data=%02x_%02x_%02x\n", format07.Data[0], format07.Data[1],
                                    format07.Data[2]);
                            format07.Length = 4 + 3;
                            SendLen = composeProtocol07(&format07, SendBuf);
                            SendDataTo485(comfd, SendBuf, SendLen);
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                }
            }
            break;
    }
}


//ret：1-广播校时；4-读数据；5-读表地址；6-最大需量清零；7-电表清零；8-校表
void dealProcess() {
    BOOLEAN nextFlag;
    int readstate = 1,ledstep=0;
    while (1) {

        for (int j = 0; j < 5; ++j) {
            JProgramInfo->Projects[j].WaitTimes = 0;
        }
        ///yx 检测
        readstate = gpio_readbyte("/dev/gpiYX1");
        if(readstate==0) {
        	switch(ledstep%2) {
        	case 0:
        		gpio_writebyte("/dev/gpoREMOTE_RED", 1);
        		usleep(250 * 1000);
        		gpio_writebyte("/dev/gpoREMOTE_GREEN", 1);
        		usleep(250 * 1000);
        		break;
        	case 1:
        		gpio_writebyte("/dev/gpoREMOTE_GREEN", 0);
        		usleep(250 * 1000);
        		gpio_writebyte("/dev/gpoREMOTE_RED", 0);
        		usleep(250 * 1000);
        		break;
        	}
        	ledstep++;
        }
        fprintf(stderr,"acs_check_end=%d\n",acs_check_end);
        if(acs_check_end==1) {	//校表结束，进行闪灯

    		gpio_writebyte("/dev/gpoREMOTE_GREEN", 1);
    		usleep(250 * 1000);
    		gpio_writebyte("/dev/gpoREMOTE_GREEN", 0);
        }
        gpio_writebyte("/dev/gpoRUN_LED", 1);
        gpio_writebyte("/dev/gpoALARM", 1);
        usleep(250 * 1000);
        gpio_writebyte("/dev/gpoRUN_LED", 0);
        gpio_writebyte("/dev/gpoALARM", 0);
        usleep(250 * 1000);
        gpio_writebyte("/dev/gpoRUN_LED", 1);
        gpio_writebyte("/dev/gpoALARM", 1);
        usleep(50 * 1000);

        RecvLen = ReceDataFrom485(comfd, RecvBuf);
        if (RecvLen > 0) {
            fprintf(stderr, "v645 RECV: ");
            int i;
            for (i = 0; i < RecvLen; i++) {
                fprintf(stderr, "%02x ", RecvBuf[i]);
            }
            fprintf(stderr, "end\n");

            FORMAT07 format07_down;
            //返回值：1-广播校时；4-读数据；5-读表地址；6-最大需量清零；7-电表清零；8-写数据（校表等）
            INT8S ret = analyzeProtocol07(&format07_down, RecvBuf, RecvLen, &nextFlag);

            fprintf(stderr, "ret1=%d\n", ret);
            if (ret == 1)//广播校时
            {
                broadCast(format07_down);//ok
            } else if (ret == 4)//读数据
            {}
            else if (ret == 5)//读表地址
            {}
            else if (ret == 6)//最大需量清零
            {}
            else if (ret == 7)//电表清零
            {}
            else if (ret == 8)//校表
            {
                //34 34 33 37
            	fprintf(stderr,"format07_down.DI=%02x_%02x_%02x_%02x\n",format07_down.DI[3],format07_down.DI[2],format07_down.DI[1],format07_down.DI[0]);
                if ((format07_down.DI[3] == 0x04) && (format07_down.DI[2] == 0x00) &&
                    (format07_down.DI[1] == 0x01) && (format07_down.DI[0] == 0x01))//对时-日期
                {
                    setTime(1, format07_down);
                } else if ((format07_down.DI[3] == 0x04) && (format07_down.DI[2] == 0x00) &&
                           (format07_down.DI[1] == 0x01) && (format07_down.DI[0] == 0x02))//对时-时间
                {
                    setTime(2, format07_down);
                } else if ((format07_down.DI[3] == 0x05) && (format07_down.DI[2] == 0x00) &&
                           (format07_down.DI[1] == 0x00) && (format07_down.DI[0] == 0x07))//电压校正
                {
                    setACS(format07_down);
                } else if ((format07_down.DI[3] == 0x05) && (format07_down.DI[2] == 0x00) &&
                           (format07_down.DI[1] == 0x00) && (format07_down.DI[0] == 0x08))//电压校正
                {
                    setACS(format07_down);
                }
            } else if (ret == -4)//校验错误
            {
                fprintf(stderr, "校验错误!!!\n");
            } else if (ret == -5)//密码错
            {
                FORMAT07 format07_up;
                memcpy(format07_up.Addr, addr, 6);

                if (format07_down.Ctrl == 0x19)//最大需量清零
                {
                    format07_up.Ctrl = 0xD9;
                } else if (format07_down.Ctrl == 0x1A)//电表清零
                {
                    format07_up.Ctrl = 0xDA;
                }
                format07_up.Err = 0x04;//密码错
                SendLen = composeProtocol07(&format07_up, SendBuf);
                SendDataTo485(comfd, SendBuf, SendLen);
            } else {
                fprintf(stderr, "未知功能!!!\n");
            }
        }
    }
}
