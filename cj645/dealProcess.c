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
#include "def645.h"

#define MsgSendOverTime 3
#define MC 6400

pid_t pid;
INT32S comfd;		//通讯口打开返回句柄

INT8U RecvBuf[256];	//接收报文缓冲区
INT8U SendBuf[256];	//发送报文缓冲区
INT8U TempBuf[256];
INT8U RecvLen;	//接收缓冲区长度
INT8U SendLen;	//发送缓冲区长度


INT8U addr[6];//表地址，低在前，高在后
INT8U bcAddr_AA[6] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
INT8U bcAddr_99[6] = {0x99, 0x99, 0x99, 0x99, 0x99, 0x99};

double P[3], Q[3], U[3], I[3];//三相有功、无功、电压、电流


extern mqd_t mqd_main;
extern INT16S ReceDataFrom485(INT32S fd, INT8U *str);
extern void SendDataTo485(INT32S fd,INT8U *sendbuf,INT16U sendlen);


//保留到10的bit次方位。如bit=-1，保留到十分位
double deal45(double x, INT8S bit)
{
	x = x + 5*pow(10, bit-1);
	x = x * pow(10, 0-bit);
	long t = (long)x;
	x = t * pow(10, bit);
	return x;
}

//发消息
//0:成功，其他:失败
INT8S sendMsg(mqd_t mqSendFd, INT8U pid, INT8U cmd, INT16U bufsiz, INT8S *sendBuf, INT8U prio)
{
	INT8S ret;
	mmq_head mqSendHead;

	mqSendHead.pid = pid;
	mqSendHead.cmd = cmd;
	mqSendHead.bufsiz = bufsiz;

	ret = mmq_put(mqSendFd, MsgSendOverTime, mqSendHead, sendBuf, prio);
	if (ret<0)
	{
		fprintf(stderr, "ERROR in sendMsg!!! ret=%d, %s\n", ret, strerror(errno));
	}
	else
	{
		fprintf(stderr, "-----------send to vmain!!!-----------\n");
	}
	return ret;
}

long pgpl_fork(void)
{
    pid_t pid;
    INT32S status;

    if (!(pid = fork())) { //child run
        // fork first time, in child process
        switch (fork()) {
            case 0:
                return 0;
            case -1:
                _exit( errno ); /* assumes all errnos are <256 */
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
INT8S execACS(INT8U flag)
{
	pid = pgpl_fork();
	if(pid == -1)
	{
		fprintf(stderr, "进程创建失败！！！\n");
		return 1;
	}
	else if(pid==0)//子进程代码运行部分
	{
		char param[12][16];
		sprintf(param[0], "%f", P[0]);
		sprintf(param[1], "%f", P[1]);
		sprintf(param[2], "%f", P[2]);

		if (shmm_getdevstat()->WireType == 0x1200)//接线方式，0x1200：三相三，0x0600：三相四
		{
			fprintf(stderr, "三相三校表！！！\n");
			sprintf(param[3], "%f", Q[0]);
			sprintf(param[4], "-%f", Q[1]);
			sprintf(param[5], "-%f", Q[2]);
		}
		else
		{
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
			execlp("vd", "vd", "acreg", param[0], param[1], param[2], param[3], param[4], param[5],
					param[6], param[7], param[8], param[9], param[10], param[11], NULL);
		}
		else if (flag == 2)//1.5A
		{
			execlp("vd", "vd", "acphase", param[0], param[1], param[2], param[3], param[4], param[5],
					param[6], param[7], param[8], param[9], param[10], param[11], NULL);
		}
		else if (flag == 3)//0.15A
		{
			execlp("vd", "vd", "acphase0", param[0], param[1], param[2], param[3], param[4], param[5],
					param[6], param[7], param[8], param[9], param[10], param[11], NULL);
		}
		else if (flag == 4)//0.3A
		{
			execlp("vd", "vd", "acphase", param[0], param[1], param[2], param[3], param[4], param[5],
					param[6], param[7], param[8], param[9], param[10], param[11], NULL);
		}
		else if (flag == 5)//u
		{
			execlp("vd", "vd", "checku", param[6], NULL);

		}

		exit(1);
		return 0;
	}
	else if(pid >0)//父进程代码运行部分
	{
		sleep(15);
		return 1;
	}
	return 1;
}

//读数据
void getData(FORMAT07 format07_down)
{
	int i;
	FORMAT07 format07_up;
	memcpy(format07_up.Addr, addr, 6);
	memcpy(format07_up.DI, format07_down.DI, 4);
	format07_up.Ctrl = 0x91;//正常应答

	for (i=0; i<sizeof(dataFlag)/sizeof(dataFlagInfo); i++)
	{
		if (memcmp(format07_down.DI, dataFlag[i].dataflag, 4)==0)
		{
			format07_up.Ctrl = 0x91;//正常应答
			switch (dataFlag[i].index)
			{
				case 1://正向有功总
				{
					INT8U bcd[4];
					FP64 value = (double)shmm_getpubdata()->ac_energy.PosPt_All/MC;
					double2bcd(value, bcd, 3, 1);
					reversebuff(bcd, 4, format07_up.Data);
					format07_up.Length = 4+4;
					break;
				}
				case 2://正向有功1费率
				{
					INT8U bcd[4];
					FP64 value = (double)shmm_getpubdata()->ac_energy.PosPt_Rate[0]/MC;
					double2bcd(value, bcd, 3, 1);
					reversebuff(bcd, 4, format07_up.Data);
					format07_up.Length = 4+4;
					break;
				}
				case 3://正向有功2费率
				{
					INT8U bcd[4];
					FP64 value = (double)shmm_getpubdata()->ac_energy.PosPt_Rate[1]/MC;
					double2bcd(value, bcd, 3, 1);
					reversebuff(bcd, 4, format07_up.Data);
					format07_up.Length = 4+4;
					break;
				}
				case 4://正向有功3费率
				{
					INT8U bcd[4];
					FP64 value = (double)shmm_getpubdata()->ac_energy.PosPt_Rate[2]/MC;
					double2bcd(value, bcd, 3, 1);
					reversebuff(bcd, 4, format07_up.Data);
					format07_up.Length = 4+4;
					break;
				}
				case 5://正向有功4费率
				{
					INT8U bcd[4];
					FP64 value = (double)shmm_getpubdata()->ac_energy.PosPt_Rate[3]/MC;
					double2bcd(value, bcd, 3, 1);
					reversebuff(bcd, 4, format07_up.Data);
					format07_up.Length = 4+4;
					break;
				}
				case 6://正向有功数据块
				{
					INT8U bcd[5][4];
					FP64 value[5];
					value[0] = (double)shmm_getpubdata()->ac_energy.PosPt_All/MC;
					value[1] = (double)shmm_getpubdata()->ac_energy.PosPt_Rate[0]/MC;
					value[2] = (double)shmm_getpubdata()->ac_energy.PosPt_Rate[1]/MC;
					value[3] = (double)shmm_getpubdata()->ac_energy.PosPt_Rate[2]/MC;
					value[4] = (double)shmm_getpubdata()->ac_energy.PosPt_Rate[3]/MC;

					double2bcd(value[0], bcd[0], 3, 1);
					double2bcd(value[1], bcd[1], 3, 1);
					double2bcd(value[2], bcd[2], 3, 1);
					double2bcd(value[3], bcd[3], 3, 1);
					double2bcd(value[4], bcd[4], 3, 1);

					reversebuff(bcd[0], 4, &format07_up.Data[0]);
					reversebuff(bcd[1], 4, &format07_up.Data[4]);
					reversebuff(bcd[2], 4, &format07_up.Data[8]);
					reversebuff(bcd[3], 4, &format07_up.Data[12]);
					reversebuff(bcd[4], 4, &format07_up.Data[16]);

					format07_up.Length = 4+20;
					break;
				}
				case 7://反向有功总
				{
					INT8U bcd[4];
					FP64 value = (double)shmm_getpubdata()->ac_energy.NegPt_All/MC;
					double2bcd(value, bcd, 3, 1);
					reversebuff(bcd, 4, format07_up.Data);
					format07_up.Length = 4+4;
					break;
				}
				case 8://反向有功1费率
				{
					INT8U bcd[4];
					FP64 value = (double)shmm_getpubdata()->ac_energy.NegPt_Rate[0]/MC;
					double2bcd(value, bcd, 3, 1);
					reversebuff(bcd, 4, format07_up.Data);
					format07_up.Length = 4+4;
					break;
				}
				case 9://反向有功2费率
				{
					INT8U bcd[4];
					FP64 value = (double)shmm_getpubdata()->ac_energy.NegPt_Rate[1]/MC;
					double2bcd(value, bcd, 3, 1);
					reversebuff(bcd, 4, format07_up.Data);
					format07_up.Length = 4+4;
					break;
				}
				case 10://反向有功3费率
				{
					INT8U bcd[4];
					FP64 value = (double)shmm_getpubdata()->ac_energy.NegPt_Rate[2]/MC;
					double2bcd(value, bcd, 3, 1);
					reversebuff(bcd, 4, format07_up.Data);
					format07_up.Length = 4+4;
					break;
				}
				case 11://反向有功4费率
				{
					INT8U bcd[4];
					FP64 value = (double)shmm_getpubdata()->ac_energy.NegPt_Rate[3]/MC;
					double2bcd(value, bcd, 3, 1);
					reversebuff(bcd, 4, format07_up.Data);
					format07_up.Length = 4+4;
					break;
				}
				case 12://反向有功数据块
				{
					INT8U bcd[5][4];
					FP64 value[5];
					value[0] = (double)shmm_getpubdata()->ac_energy.NegPt_All/MC;
					value[1] = (double)shmm_getpubdata()->ac_energy.NegPt_Rate[0]/MC;
					value[2] = (double)shmm_getpubdata()->ac_energy.NegPt_Rate[1]/MC;
					value[3] = (double)shmm_getpubdata()->ac_energy.NegPt_Rate[2]/MC;
					value[4] = (double)shmm_getpubdata()->ac_energy.NegPt_Rate[3]/MC;

					double2bcd(value[0], bcd[0], 3, 1);
					double2bcd(value[1], bcd[1], 3, 1);
					double2bcd(value[2], bcd[2], 3, 1);
					double2bcd(value[3], bcd[3], 3, 1);
					double2bcd(value[4], bcd[4], 3, 1);

					reversebuff(bcd[0], 4, &format07_up.Data[0]);
					reversebuff(bcd[1], 4, &format07_up.Data[4]);
					reversebuff(bcd[2], 4, &format07_up.Data[8]);
					reversebuff(bcd[3], 4, &format07_up.Data[12]);
					reversebuff(bcd[4], 4, &format07_up.Data[16]);

					format07_up.Length = 4+20;
					break;
				}
				case 13://年时区表数
				{
					format07_up.Length = 4+1;
					format07_up.Data[0] = 1;
					break;
				}
				case 14://日时段表数
				{
					format07_up.Length = 4+1;
					format07_up.Data[0] = 1;
					break;
				}
				case 15://日时段数
				{
					format07_up.Length = 4+1;
					format07_up.Data[0] = 1;
					break;
				}
				case 16://时区表数据
				{
					INT8U i;
					for (i=0; i<14; i++)
					{
						int32u2bcd((INT32U)shiquInfo[i].no, &format07_up.Data[0+i*3], positive);
						int32u2bcd((INT32U)shiquInfo[i].BeginD.day, &format07_up.Data[1+i*3], positive);
						int32u2bcd((INT32U)shiquInfo[i].BeginD.month, &format07_up.Data[2+i*3], positive);
					}
					format07_up.Length = 4+3*14;
					break;
				}
				case 17://时段表数据
				case 18:
				case 19:
				case 20:
				case 21:
				case 22:
				case 23:
				case 24:
				{
					INT8U i;
					for (i=0; i<14; i++)
					{
						int32u2bcd((INT32U)shiduanInfo[i].no, &format07_up.Data[0+i*3], positive);
						int32u2bcd((INT32U)shiduanInfo[i].BeginT.minute, &format07_up.Data[1+i*3], positive);
						int32u2bcd((INT32U)shiduanInfo[i].BeginT.hour, &format07_up.Data[2+i*3], positive);
					}
					format07_up.Length = 4+3*14;
					break;
				}
				case 25://电压数据
				{
					INT8U i;
					for (i=0; i<14; i++)
					{
						int32u2bcd((INT32U)shiquInfo[i].no, &format07_up.Data[0+i*3], positive);
						int32u2bcd((INT32U)shiquInfo[i].BeginD.day, &format07_up.Data[1+i*3], positive);
						int32u2bcd((INT32U)shiquInfo[i].BeginD.month, &format07_up.Data[2+i*3], positive);
					}
					format07_up.Length = 4+3*14;
					break;
				}

			}
			SendLen = composeProtocol07(&format07_up, SendBuf);
			SendDataTo485(comfd, SendBuf, SendLen);
			return;
		}
	}
	format07_up.Ctrl = 0xD1;//异常应答
	format07_up.Err = 0x02;//无请求数据
	SendLen = composeProtocol07(&format07_up, SendBuf);
	SendDataTo485(comfd, SendBuf, SendLen);
}

//广播校时
void broadCast(FORMAT07 format07)
{
	fprintf(stderr, "\n广播校时 %d-%d-%d, %d:%d:%d\n", format07.Time[5]+2000,
			format07.Time[4], format07.Time[3], format07.Time[2], format07.Time[1], format07.Time[0]);

	int rtc;
	struct tm _tm;
	struct timeval tv;
	_tm.tm_sec = format07.Time[0];
	_tm.tm_min = format07.Time[1];
	_tm.tm_hour = format07.Time[2];
	_tm.tm_mday = format07.Time[3];
	_tm.tm_mon = format07.Time[4]-1;
	_tm.tm_year = format07.Time[5]+2000-1900;
	_tm.tm_isdst = 0;
	tv.tv_sec = mktime(&_tm);
	tv.tv_usec = 0;
	settimeofday(&tv, (struct timezone *)0);
	rtc = open(DEV_RTC,O_RDWR);
	ioctl(rtc, RTC_SET_TIME, &_tm);
	close(rtc);

	char cmd[10];
	sprintf(cmd, "%s", "date");
	system(cmd);
	fprintf(stderr, "\n\n");
}

//应答表地址
void readAddr()
{
	FORMAT07 format07_up;
	memcpy(format07_up.Addr, addr, 6);
	format07_up.Ctrl = 0x93;
	SendLen = composeProtocol07(&format07_up, SendBuf);
	SendDataTo485(comfd, SendBuf, SendLen);
	fprintf(stderr, "\n\n");
}


//最大需量清零
void clearXL()
{
	fprintf(stderr, "最大需量清零.......\n");

//	INT8U buf[2]={0x04, 0x00};
//	sendMsg(mqd_main, vs485, SAVE_REBOOT, 2, (INT8S *)&buf, 0);

//	SendBuf[0] = SendBuf[7] = 0x68;
//	memcpy(&SendBuf[1], addr, 6);
//	SendBuf[8] = 0x99;//正常应答
//	SendBuf[9] = 0x00;
//	SendBuf[10] = getCS645(SendBuf, 10);
//	SendBuf[11] = 0x16;
//	SendDataTo485(comfd, SendBuf, 12);

	FORMAT07 format07;
	memcpy(format07.Addr, addr, 6);
	format07.Ctrl = 0x99;//正常应答
	SendLen = composeProtocol07(&format07, SendBuf);
	SendDataTo485(comfd, SendBuf, SendLen);
}


//电表清零
void clearAll()
{
	fprintf(stderr, "电表清零.......\n");

	INT8U buf[2]={0x04, 0x00};
	sendMsg(mqd_main, vs485, SAVE_REBOOT, 2, (INT8S *)&buf, 0);

//	SendBuf[0] = SendBuf[7] = 0x68;
//	memcpy(&SendBuf[1], addr, 6);
//	SendBuf[8] = 0x9A;//正常应答
//	SendBuf[9] = 0x00;
//	SendBuf[10] = getCS645(SendBuf, 10);
//	SendBuf[11] = 0x16;
//	SendDataTo485(comfd, SendBuf, 12);

	FORMAT07 format07;
	memcpy(format07.Addr, addr, 6);
	format07.Ctrl = 0x9A;//正常应答
	SendLen = composeProtocol07(&format07, SendBuf);
	SendDataTo485(comfd, SendBuf, SendLen);
}


//解析报文得到有功、无功、电压、电流值
void Msg2AcsValue(FORMAT07 format07)
{
	INT32U tempdata[48];
	INT8U i;
	for(i=0; i<48; i++)
	{
		bcd2int32u(&format07.Data[i], 1, positive, &tempdata[i]);
	}

	P[0] = tempdata[3]*100 + tempdata[2] + (double)tempdata[1]/100 + (double)tempdata[0]/10000;
	P[1] = tempdata[7]*100 + tempdata[6] + (double)tempdata[5]/100 + (double)tempdata[4]/10000;
	P[2] = tempdata[11]*100 + tempdata[10] + (double)tempdata[9]/100 + (double)tempdata[8]/10000;
	Q[0] = tempdata[15]*100 + tempdata[14] + (double)tempdata[13]/100 + (double)tempdata[12]/10000;
	Q[1] = tempdata[19]*100 + tempdata[18] + (double)tempdata[17]/100 + (double)tempdata[16]/10000;
	Q[2] = tempdata[23]*100 + tempdata[22] + (double)tempdata[21]/100 + (double)tempdata[20]/10000;
	U[0] = tempdata[27]*100 + tempdata[26] + (double)tempdata[25]/100 + (double)tempdata[24]/10000;
	U[1] = tempdata[31]*100 + tempdata[30] + (double)tempdata[29]/100 + (double)tempdata[28]/10000;
	U[2] = tempdata[35]*100 + tempdata[34] + (double)tempdata[33]/100 + (double)tempdata[32]/10000;
	I[0] = tempdata[39]*100 + tempdata[38] + (double)tempdata[37]/100 + (double)tempdata[36]/10000;
	I[1] = tempdata[43]*100 + tempdata[42] + (double)tempdata[41]/100 + (double)tempdata[40]/10000;
	I[2] = tempdata[47]*100 + tempdata[46] + (double)tempdata[45]/100 + (double)tempdata[44]/10000;

	fprintf(stderr, "P=%0.4f, %0.4f, %0.4f\n", P[0], P[1], P[2]);
	fprintf(stderr, "Q=%0.4f, %0.4f, %0.4f\n", Q[0], Q[1], Q[2]);
	fprintf(stderr, "U=%0.4f, %0.4f, %0.4f\n", U[0], U[1], U[2]);
	fprintf(stderr, "I=%0.4f, %0.4f, %0.4f\n", I[0], I[1], I[2]);
}


//有功、无功、电压、电流值转成报文
void AcsValue2Msg(FORMAT07 format07_down)
{
	INT8U tempdata[4];
	FORMAT07 format07_up;
	memcpy(format07_up.Addr, addr, 6);
	format07_up.Ctrl = 0x91;
	format07_up.Length = 0x34;

	double tmpValue;
	tmpValue = deal45((double)shmm_getpubdata()->ac_realdata.Pa/P_COEF, -1);
	double2bcd(tmpValue, &tempdata[0], 2, 2);
	reversebuff(tempdata, 4, &format07_up.Data[0]);

	tmpValue = deal45((double)shmm_getpubdata()->ac_realdata.Pb/P_COEF, -1);
	double2bcd(tmpValue, &tempdata[0], 2, 2);
	reversebuff(tempdata, 4, &format07_up.Data[4]);

	tmpValue = deal45((double)shmm_getpubdata()->ac_realdata.Pc/P_COEF, -1);
	double2bcd(tmpValue, &tempdata[0], 2, 2);
	reversebuff(tempdata, 4, &format07_up.Data[8]);

	tmpValue = deal45((double)shmm_getpubdata()->ac_realdata.Qa/Q_COEF, -1);
	double2bcd(tmpValue, &tempdata[0], 2, 2);
	reversebuff(tempdata, 4, &format07_up.Data[12]);

	tmpValue = deal45((double)shmm_getpubdata()->ac_realdata.Qb/Q_COEF, -1);
	double2bcd(tmpValue, &tempdata[0], 2, 2);
	reversebuff(tempdata, 4, &format07_up.Data[16]);

	tmpValue = deal45((double)shmm_getpubdata()->ac_realdata.Qc/Q_COEF, -1);
	double2bcd(tmpValue, &tempdata[0], 2, 2);
	reversebuff(tempdata, 4, &format07_up.Data[20]);

	tmpValue = deal45((double)shmm_getpubdata()->ac_realdata.Ua/U_COEF, -1);
	double2bcd(tmpValue, &tempdata[0], 2, 2);
	reversebuff(tempdata, 4, &format07_up.Data[24]);

	tmpValue = deal45((double)shmm_getpubdata()->ac_realdata.Ub/U_COEF, -1);
	double2bcd(tmpValue, &tempdata[0], 2, 2);
	reversebuff(tempdata, 4, &format07_up.Data[28]);

	tmpValue = deal45((double)shmm_getpubdata()->ac_realdata.Uc/U_COEF, -1);
	double2bcd(tmpValue, &tempdata[0], 2, 2);
	reversebuff(tempdata, 4, &format07_up.Data[32]);

	tmpValue = deal45((double)shmm_getpubdata()->ac_realdata.Ia/I_COEF, -2);
	double2bcd(tmpValue, &tempdata[0], 2, 2);
	reversebuff(tempdata, 4, &format07_up.Data[36]);

	tmpValue = deal45((double)shmm_getpubdata()->ac_realdata.Ib/I_COEF, -2);
	double2bcd(tmpValue, &tempdata[0], 2, 2);
	reversebuff(tempdata, 4, &format07_up.Data[40]);

	tmpValue = deal45((double)shmm_getpubdata()->ac_realdata.Ic/I_COEF, -2);
	double2bcd(tmpValue, &tempdata[0], 2, 2);
	reversebuff(tempdata, 4, &format07_up.Data[44]);

	SendLen = composeProtocol07(&format07_up, SendBuf);
	SendDataTo485(comfd, SendBuf, SendLen);
}


//对时：flag=1，日期；flag=2，时间
void setTime(INT8U flag, FORMAT07 format07)
{
	int rtc;
	struct tm _tm;
	struct timeval tv;

	TmS tm;
	tmget(&tm);

	if (flag == 1)
	{
		INT32U year, month, day;
		bcd2int32u(&format07.Data[11], 1, inverted, &year);
		bcd2int32u(&format07.Data[10], 1, inverted, &month);
		bcd2int32u(&format07.Data[9], 1, inverted, &day);

		fprintf(stderr, "\n对时-日期： %d-%d-%d\n", year+2000, month, day);
		_tm.tm_sec = tm.Sec;
		_tm.tm_min = tm.Minute;
		_tm.tm_hour = tm.Hour;
		_tm.tm_mday = day;
		_tm.tm_mon = month-1;
		_tm.tm_year = year+2000-1900;
	}
	else if (flag == 2)
	{
		INT32U hour, minute, sec;
		bcd2int32u(&format07.Data[10], 1, inverted, &hour);
		bcd2int32u(&format07.Data[9], 1, inverted, &minute);
		bcd2int32u(&format07.Data[8], 1, inverted, &sec);

		fprintf(stderr, "\n对时-时间：%d:%d:%d\n", hour, minute, sec);
		_tm.tm_sec = sec;
		_tm.tm_min = minute;
		_tm.tm_hour = hour;
		_tm.tm_mday = tm.Day;
		_tm.tm_mon = tm.Month-1;
		_tm.tm_year = tm.Year-1900;

		fprintf(stderr, "year=%d, mon=%d\n", _tm.tm_year, _tm.tm_mon);
	}

	_tm.tm_isdst = 0;
	tv.tv_sec = mktime(&_tm);
	tv.tv_usec = 0;
	settimeofday(&tv, (struct timezone *)0);
	rtc = open(DEV_RTC,O_RDWR);
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
void setACS(FORMAT07 format07)
{
	char cmd[100];
	memset(cmd, 0, 100);
	int i;

	fprintf(stderr, "shmm_getdevstat()->ac_chip_type=%x\n", shmm_getdevstat()->ac_chip_type);
	if (shmm_getdevstat()->ac_chip_type == 1)//2d
	{
		for (i=0; i<sizeof(dataFlag_2d)/sizeof(dataFlagInfo); i++)
		{
			if (memcmp(format07.DI, dataFlag_2d[i].dataflag, 4)==0)
			{
				switch (dataFlag_2d[i].index)
				{
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
				//发送数据
	//			AcsValue2Msg(format07);
				fprintf(stderr, "校表结束!!!\n");
				system("reboot");
				break;
			}
		}
	}
	else if (shmm_getdevstat()->ac_chip_type == ATT_VER_ID)//3d
	{
		fprintf(stderr, "333333333333\n");
		for (i=0; i<sizeof(dataFlag_3d)/sizeof(dataFlagInfo); i++)
		{
			if (memcmp(format07.DI, dataFlag_3d[i].dataflag, 4)==0)
			{
				switch (dataFlag_3d[i].index)
				{
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
				//发送数据
	//			AcsValue2Msg(format07);
				fprintf(stderr, "校表结束!!!\n");
				sleep(3);
				system("reboot");
				break;
			}
		}
	}
	else{// 校正电压
		fprintf(stderr, "444444444\n");
		for (i=0; i<sizeof(dataFlag_4d)/sizeof(dataFlagInfo); i++)
		{
			if (memcmp(format07.DI, dataFlag_4d[i].dataflag, 4)==0)
			{
				switch (dataFlag_4d[i].index)
				{
					case 1://电压校正
					{
						fprintf(stderr, "电压校表......\n");
						Msg2AcsValue(format07);
						execACS(5);//校表
						fprintf(stderr, "校表结束!!!\n");
						sleep(3);
						system("reboot");
						break;
					}
					case 2://电压读取
					{
						fprintf(stderr, "电压读取返回......\n");
						INT8U bcd[4];
						FP64 value = (double)shmm_getpubdata()->ac_realdata.Ua;
						fprintf(stderr,"value=%f\n",value);
						double2bcd(value, bcd, 2, 1);
						fprintf(stderr,"bcd=%02x_%02x_%02x_%02x\n",bcd[0],bcd[1],bcd[2],bcd[3]);
						reversebuff(bcd, 3, format07.Data);
						fprintf(stderr,"Data=%02x_%02x_%02x\n",format07.Data[0],format07.Data[1],format07.Data[2]);
						format07.Length = 4+3;
						SendLen = composeProtocol07(&format07, SendBuf);
						SendDataTo485(comfd, SendBuf, SendLen);
						break;
					}
					default: break;
				}
				break;
			}
		}
	}
}


//ret：1-广播校时；4-读数据；5-读表地址；6-最大需量清零；7-电表清零；8-校表
void dealProcess()
{
	BOOLEAN nextFlag;
	while (1)
	{
		RecvLen = ReceDataFrom485(comfd, RecvBuf);
		if(RecvLen>0)
		{
			fprintf(stderr, "v645 RECV: ");
			int i;
			for (i=0; i<RecvLen; i++)
			{
				fprintf(stderr, "%02x ", RecvBuf[i]);
			}
			fprintf(stderr, "\n");

			FORMAT07 format07_down;
			//返回值：1-广播校时；4-读数据；5-读表地址；6-最大需量清零；7-电表清零；8-写数据（校表等）
			INT8S ret = analyzeProtocol07(&format07_down, RecvBuf, RecvLen, &nextFlag);

			fprintf(stderr, "ret=%d\n", ret);
			if (ret == 1)//广播校时
			{
				broadCast(format07_down);//ok
			}
			else if (ret == 4)//读数据
			{
				if (memcmp(addr, format07_down.Addr, 6)==0)//表地址匹配
				{
					getData(format07_down);//ok
				}
				else
				{
					fprintf(stderr, "读数据错误，表地址不匹配！！！\n");
				}
			}
			else if (ret == 5)//读表地址
			{
				readAddr();//ok
			}
			else if (ret == 6)//最大需量清零
			{
				if (memcmp(addr, format07_down.Addr, 6)==0)//表地址匹配
				{
					clearXL();
				}
				else
				{
					fprintf(stderr, "最大需量清零错误，表地址不匹配！！！\n");
				}
			}
			else if (ret == 7)//电表清零
			{
				if ((memcmp(addr, format07_down.Addr, 6)==0) ||//标准
					(memcmp(bcAddr_AA, format07_down.Addr, 6)==0) ||//自定义
					(memcmp(bcAddr_99, format07_down.Addr, 6)==0)//自定义
					)//表地址匹配
				{
					clearAll();//ok
				}
				else
				{
					fprintf(stderr, "电表清零错误，表地址不匹配！！！\n");
				}
			}
			else if (ret == 8)//校表
			{
//				if ((memcmp(addr, format07_down.Addr, 6)==0) ||//标准
//					(memcmp(bcAddr_AA, format07_down.Addr, 6)==0) ||//自定义
//					(memcmp(bcAddr_99, format07_down.Addr, 6)==0)//自定义
//					)//表地址匹配
//				{
//					setACS(format07_down);
//				}
//				else
//				{
//					fprintf(stderr, "校表错误，表地址不匹配！！！\n");
//				}
				//34 34 33 37
				if ((format07_down.DI[3]==0x04) && (format07_down.DI[2]==0x00) &&
					(format07_down.DI[1]==0x01) && (format07_down.DI[0]==0x01))//对时-日期
				{
					setTime(1, format07_down);
				}
				else if ((format07_down.DI[3]==0x04) && (format07_down.DI[2]==0x00) &&
						(format07_down.DI[1]==0x01) && (format07_down.DI[0]==0x02))//对时-时间
				{
					setTime(2, format07_down);
				}
				else if ((format07_down.DI[3]==0x05) && (format07_down.DI[2]==0x00) &&
						(format07_down.DI[1]==0x00) && (format07_down.DI[0]==0x07))//电压校正
				{
					setACS(format07_down);
				}
				else if ((format07_down.DI[3]==0x05) && (format07_down.DI[2]==0x00) &&
						(format07_down.DI[1]==0x00) && (format07_down.DI[0]==0x08))//电压校正
				{
					setACS(format07_down);
				}
			}
			else if (ret == -4)//校验错误
			{
				fprintf(stderr, "校验错误!!!\n");
			}
			else if (ret == -5)//密码错
			{
				FORMAT07 format07_up;
				memcpy(format07_up.Addr, addr, 6);

				if (format07_down.Ctrl == 0x19)//最大需量清零
				{
					format07_up.Ctrl = 0xD9;
				}
				else if (format07_down.Ctrl == 0x1A)//电表清零
				{
					format07_up.Ctrl = 0xDA;
				}
				format07_up.Err = 0x04;//密码错
				SendLen = composeProtocol07(&format07_up, SendBuf);
				SendDataTo485(comfd, SendBuf, SendLen);
			}
			else
			{
				fprintf(stderr, "未知功能!!!\n");
			}
		}
		sleep(1);
	}
}
