/*
 * readplc.c
 *
 *  Created on: 2017-1-4
 *      Author: wzm
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>
#include "sys/reboot.h"
#include <wait.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include "time.h"
#include <sys/time.h>
#include "lib3762.h"
#include "readplc.h"
#include "cjdeal.h"
#include "cjsave.h"
#include "dlt645.h"
#include "dlt698.h"
#include "dlt698def.h"

extern ProgramInfo* JProgramInfo;
extern int SaveOADData(INT8U taskid,OAD oad_m,OAD oad_r,INT8U *databuf,int datalen,TS ts_res);
extern INT16U data07Tobuff698(FORMAT07 Data07,INT8U* dataContent);
extern INT8S analyzeProtocol07(FORMAT07* format07, INT8U* recvBuf, const INT16U recvLen, BOOLEAN *nextFlag);
extern INT8S OADMap07DI(OI_698 roadOI,OAD sourceOAD, C601F_645* flag645);
extern void DbgPrintToFile1(INT8U comport,const char *format,...);
extern void DbPrt1(INT8U comport,char *prefix, char *buf, int len, char *suffix);
extern  INT16U data07Tobuff698(FORMAT07 Data07,INT8U* dataContent);
extern INT8U checkMeterType(MY_MS mst,INT8U usrType,TSA usrAddr);
extern mqd_t mqd_zb_task;
extern CJCOMM_PROXY cjcommProxy;
extern GUI_PROXY cjGuiProxy_plc;
extern Proxy_Msg* p_Proxy_Msg_Data;//液晶给抄表发送代理处理结构体，指向由guictrl.c配置的全局变量
extern TASK_CFG list6013[TASK6012_MAX];
//-----------------------------------------------------------------------------------------------------------------------------------------------------

typedef struct
{
	INT8U startIndex; 	//报文中的某数据的起始字节
	INT8U dataLen;	 	//数据长度（字节数）
	INT8U aunite;		//一个数据单元长度
	INT8U intbits;		//整数部分长度
	INT8U decbits;		//小数部分长度
	char name[30];
	INT8U Flg07[4];//对应07表实时数据标识
	OAD oad1;
	OAD oad2;
}MeterCurveDataType;
#define CURVENUM 29
MeterCurveDataType meterCurveData[CURVENUM]=
{
			{60, 4, 4, 6, 2, "正向有功总电能曲线",	 {0x00,0x01,0x00,0x00},{0x5002,0x02,0x00},{0x0010,0x02,0x00}},
			{64, 4, 4, 6, 2, "反向有功总电能曲线",	 {0x00,0x02,0x00,0x00},{0x5002,0x02,0x00},{0x0020,0x02,0x00}},
			{77, 4, 4, 6, 2, "一象限无功总电能曲线",{0x00,0x05,0x00,0x00},{0x5002,0x02,0x00},{0x0050,0x02,0x00}},
			{81, 4, 4, 6, 2, "二象限无功总电能曲线",{0x00,0x06,0x00,0x00},{0x5002,0x02,0x00},{0x0060,0x02,0x00}},
			{85, 4, 4, 6, 2, "三象限无功总电能曲线",{0x00,0x07,0x00,0x00},{0x5002,0x02,0x00},{0x0070,0x02,0x00}},
			{89, 4, 4, 6, 2, "四象限无功总电能曲线",{0x00,0x08,0x00,0x00},{0x5002,0x02,0x00},{0x0080,0x02,0x00}},
			{8,  6, 2, 3, 1, "当前电压",			 {0x02,0x01,0xff,0x00},{0x5002,0x02,0x00},{0x2000,0x02,0x00}},
			{14, 9, 3, 3, 3, "当前电流",			 {0x02,0x02,0xff,0x00},{0x5002,0x02,0x00},{0x2001,0x02,0x00}},
			{26, 12,3, 2, 4, "有功功率曲线",	 	 {0x02,0x03,0xff,0x00},{0x5002,0x02,0x00},{0x2004,0x02,0x00}},
			{51, 8, 2, 2, 1, "功率因数曲线",		 {0x02,0x06,0xff,0x00},{0x5002,0x02,0x00},{0x200a,0x02,0x00}}


//	{8,  2, 3, 1, "A相电压",			{0x01,0x01,0x10,0x06}},
//	{10, 2, 3, 1, "B相电压",			{0x02,0x01,0x10,0x06}},
//	{12, 2, 3, 1, "C相电压",			{0x03,0x01,0x10,0x06}},
//
//	{14, 3, 3, 3, "A相电流",			{0x01,0x02,0x10,0x06}},
//	{17, 3, 3, 3, "B相电流",			{0x02,0x02,0x10,0x06}},
//	{20, 3, 3, 3, "C相电流",			{0x03,0x02,0x10,0x06}},
//
//	{23, 2, 2, 2, "频率曲线",			{0xFF,0xFF,0xFF,0xFF}},
//
//	{26, 3, 2, 4, "总有功功率曲线",	{0x00,0x03,0x10,0x06}},
//	{29, 3, 2, 4, "A相有功功率曲线",	{0x01,0x03,0x10,0x06}},
//	{32, 3, 2, 4, "B相有功功率曲线",	{0x02,0x03,0x10,0x06}},
//	{35, 3, 2, 4, "C相有功功率曲线",	{0x03,0x03,0x10,0x06}},
//
//	{38, 3, 2, 4, "总无功功率曲线",	{0x00,0x04,0x10,0x06}},
//	{41, 3, 2, 4, "A相无功功率曲线",	{0x01,0x04,0x10,0x06}},
//	{44, 3, 2, 4, "B相无功功率曲线",	{0x02,0x04,0x10,0x06}},
//	{47, 3, 2, 4, "C相无功功率曲线",	{0x03,0x04,0x10,0x06}},
//
//	{51, 2, 3, 1, "总功率因数曲线",	{0x00,0x05,0x10,0x06}},
//	{53, 2, 3, 1, "A相功率因数曲线",	{0x01,0x05,0x10,0x06}},
//	{55, 2, 3, 1, "B相功率因数曲线",	{0x02,0x05,0x10,0x06}},
//	{57, 2, 3, 1, "C相功率因数曲线",	{0x03,0x05,0x10,0x06}},
//
//	{60, 4, 6, 2, "正向有功总电能曲线",{0x01,0x06,0x10,0x06}},
//	{64, 4, 6, 2, "反向有功总电能曲线",{0x02,0x06,0x10,0x06}},
//	{68, 4, 6, 2, "正向无功总电能曲线",{0x03,0x06,0x10,0x06}},
//	{72, 4, 6, 2, "反向无功总电能曲线",{0x04,0x06,0x10,0x06}},
//
//	{77, 4, 6, 2, "一象限无功总电能曲线",{0x01,0x07,0x10,0x06}},
//	{81, 4, 6, 2, "二象限无功总电能曲线",{0x02,0x07,0x10,0x06}},
//	{85, 4, 6, 2, "三象限无功总电能曲线",{0x03,0x07,0x10,0x06}},
//	{89, 4, 6, 2, "四象限无功总电能曲线",{0x04,0x07,0x10,0x06}},
//
//	{94, 3, 2, 4, "当前有功需量曲线",	{0xFF,0xFF,0xFF,0xFF}},
//	{97, 3, 2, 4, "当前无功需量曲线",	{0xFF,0xFF,0xFF,0xFF}},
};


void SendDataToCom(int fd, INT8U *sendbuf, INT16U sendlen)
{
	int i=0;
	ssize_t slen;
	slen = write(fd, sendbuf, sendlen);
	DbPrt1(31,"S:", (char *) sendbuf, slen, NULL);
	fprintf(stderr,"\nsend(%d)",slen);
	for(i=0;i<slen;i++)
		fprintf(stderr," %02x",sendbuf[i]);
}
int RecvDataFromCom(int fd,INT8U* buf,int* head)
{
	int len,i;
	INT8U TmpBuf[ZBBUFSIZE];
	memset(TmpBuf,0,ZBBUFSIZE);
	if (fd < 0 ) return 0;
	len = read(fd,TmpBuf,ZBBUFSIZE);
	if (len>0)
	{
		fprintf(stderr,"\nrecv(%d): ",len);
	}

	for(i=0;i<len;i++)
	{
		buf[*head]=TmpBuf[i];
		fprintf(stderr,"%02x ",TmpBuf[i]);
		*head = (*head + 1) % ZBBUFSIZE;
	}
	return len;
}

int StateProcessZb(unsigned char *str,INT8U* Buf )
{
	int i;
	INT16U tmptail=0;

	switch (rec_step)
	{
	case 0:
		while (RecvTail != RecvHead)
		{
			if(Buf[RecvTail] == 0x68)
			{
				rec_step = 1;
				break;
			}else {
				RecvTail = (RecvTail+1)%ZBBUFSIZE;
			}
		}
		break;
	case 1:
		if(((RecvHead - RecvTail+ZBBUFSIZE)%ZBBUFSIZE)>=3)
		{
			tmptail=RecvTail;
			tmptail = (tmptail+1)%ZBBUFSIZE;
			DataLen = Buf[tmptail];
			tmptail = (tmptail+1)%ZBBUFSIZE;
			DataLen |= (Buf[tmptail]<<8);
			if (DataLen !=0)
			{
				rec_step = 2;
				oldtime1 = time(NULL);
			}else
			{
				RecvTail = (RecvTail+1)%ZBBUFSIZE;
				rec_step = 0;
			}
			break;
		}
		break;
	case 2:
		if(((RecvHead - RecvTail+ZBBUFSIZE)%ZBBUFSIZE)>=DataLen)
		{
			if (Buf[(RecvTail+DataLen-1)%ZBBUFSIZE] == 0x16)
			{
				//fprintf(stderr,"\nReceiveFromCarr OK: len = %d\n",DataLen);
				for(i=0; i<DataLen; i++)
				{
					str[i] = Buf[RecvTail];
					RecvTail = (RecvTail+1)%ZBBUFSIZE;
				}
				rec_step = 0;
				DbPrt1(31,"R:", (char *) str, DataLen, NULL);
				return DataLen;
			}else {
				RecvTail = (RecvTail+1)%ZBBUFSIZE;
				rec_step = 0;
			}
		}else
		{
			newtime1 = time(NULL);
			if ((newtime1-oldtime1)> 2)
			{
				RecvTail = (RecvTail+1)%ZBBUFSIZE;
				rec_step = 0;
			}
		}
		break;
	default:
		break;
	}
	return (0);
}
/*
 * n: bit位  n(0..7)
 */
INT8U judgebit(INT8U Byte,int n)
{
	if (n <= 7)
	{
		INT8U tmp = (Byte >> n) & 0x01;
		if (tmp == 1)
			return 1;
	}
	return 0;
}

TS DateBCD2Ts(DateTimeBCD timebcd)
{
	TS ts;
	ts.Year = timebcd.year.data;
	ts.Month = timebcd.month.data;
	ts.Day = timebcd.day.data;
	ts.Hour = timebcd.hour.data;
	ts.Minute = timebcd.min.data;
	ts.Sec = timebcd.sec.data;
	ts.Week = 1;
	fprintf(stderr,"\n111111 ts.Year %d-%d-%d %d:%d:%d",ts.Year,ts.Month,ts.Day,ts.Hour,ts.Minute,ts.Sec);

	return ts;
}

void PrintTaskInfo2(TASK_INFO *task)
{
	int i=0,j=0,numindex=0;
	for(i=0;i<task->task_n;i++)
	{
		for(j=0;j<task->task_list[i].fangan.item_n;j++)
		{
			numindex++;
			switch(task->task_list[i].fangan.cjtype)
			{
			case 0:
				DbgPrintToFile1(31,"%02d| %04x-%02x%02x - %04x-%02x%02x .%02d任务 执行频率%d[%d] %d级 方案%d ，类型%d  采集当前数据 | %d-%d-%d %d:%d:%d OK=%d cov %02x%02x%02x%02x",
						numindex,
						task->task_list[i].fangan.items[j].oad1.OI,task->task_list[i].fangan.items[j].oad1.attflg,task->task_list[i].fangan.items[j].oad1.attrindex,
						task->task_list[i].fangan.items[j].oad2.OI,task->task_list[i].fangan.items[j].oad2.attflg,task->task_list[i].fangan.items[j].oad2.attrindex,
						task->task_list[i].taskId,
						task->task_list[i].ti.interval,task->task_list[i].ti.units,
						task->task_list[i].leve,
						task->task_list[i].fangan.No,
						task->task_list[i].fangan.type,
						task->task_list[i].begin.year.data,task->task_list[i].begin.month.data,task->task_list[i].begin.day.data,
						task->task_list[i].begin.hour.data,task->task_list[i].begin.min.data,task->task_list[i].begin.sec.data,
						task->task_list[i].fangan.items[j].sucessflg,
						task->task_list[i].fangan.items[j].item07[3],task->task_list[i].fangan.items[j].item07[2],
						task->task_list[i].fangan.items[j].item07[1],task->task_list[i].fangan.items[j].item07[0]
						);
				break;
			case 1:
				DbgPrintToFile1(31,"%02d| %04x-%02x%02x - %04x-%02x%02x .%02d任务 执行频率%d[%d] %d级 方案%d ，类型%d  采集上%d次 | %d-%d-%d %d:%d:%d OK=%d cov %02x%02x%02x%02x",
						numindex,
						task->task_list[i].fangan.items[j].oad1.OI,task->task_list[i].fangan.items[j].oad1.attflg,task->task_list[i].fangan.items[j].oad1.attrindex,
						task->task_list[i].fangan.items[j].oad2.OI,task->task_list[i].fangan.items[j].oad2.attflg,task->task_list[i].fangan.items[j].oad2.attrindex,
						task->task_list[i].taskId,
						task->task_list[i].ti.interval,task->task_list[i].ti.units,
						task->task_list[i].leve,
						task->task_list[i].fangan.No,task->task_list[i].fangan.type,
						task->task_list[i].fangan.N,
						task->task_list[i].begin.year.data,task->task_list[i].begin.month.data,task->task_list[i].begin.day.data,
						task->task_list[i].begin.hour.data,task->task_list[i].begin.min.data,task->task_list[i].begin.sec.data,
						task->task_list[i].fangan.items[j].sucessflg,
						task->task_list[i].fangan.items[j].item07[3],task->task_list[i].fangan.items[j].item07[2],
						task->task_list[i].fangan.items[j].item07[1],task->task_list[i].fangan.items[j].item07[0]
						);
				break;
			case 2:
				DbgPrintToFile1(31,"%02d| %04x-%02x%02x - %04x-%02x%02x .%02d任务 执行频率%d[%d] %d级 方案%d ，类型%d  按冻结时标采集 | %d-%d-%d %d:%d:%d OK=%d cov %02x%02x%02x%02x",
						numindex,
						task->task_list[i].fangan.items[j].oad1.OI,task->task_list[i].fangan.items[j].oad1.attflg,task->task_list[i].fangan.items[j].oad1.attrindex,
						task->task_list[i].fangan.items[j].oad2.OI,task->task_list[i].fangan.items[j].oad2.attflg,task->task_list[i].fangan.items[j].oad2.attrindex,
						task->task_list[i].taskId,
						task->task_list[i].ti.interval,task->task_list[i].ti.units,
						task->task_list[i].leve,
						task->task_list[i].fangan.No,task->task_list[i].fangan.type,
						task->task_list[i].begin.year.data,task->task_list[i].begin.month.data,task->task_list[i].begin.day.data,
						task->task_list[i].begin.hour.data,task->task_list[i].begin.min.data,task->task_list[i].begin.sec.data,
						task->task_list[i].fangan.items[j].sucessflg,
						task->task_list[i].fangan.items[j].item07[3],task->task_list[i].fangan.items[j].item07[2],
						task->task_list[i].fangan.items[j].item07[1],task->task_list[i].fangan.items[j].item07[0]
						);
				break;
			case 3:
				DbgPrintToFile1(31,"%02d| %04x-%02x%02x - %04x-%02x%02x .%02d任务 执行频率%d[%d] %d级 方案%d ，类型%d  间隔%d (%d) | %d-%d-%d %d:%d:%d OK=%d cov %02x%02x%02x%02x",
						numindex,
						task->task_list[i].fangan.items[j].oad1.OI,task->task_list[i].fangan.items[j].oad1.attflg,task->task_list[i].fangan.items[j].oad1.attrindex,
						task->task_list[i].fangan.items[j].oad2.OI,task->task_list[i].fangan.items[j].oad2.attflg,task->task_list[i].fangan.items[j].oad2.attrindex,
						task->task_list[i].taskId,
						task->task_list[i].ti.interval,task->task_list[i].ti.units,
						task->task_list[i].leve,
						task->task_list[i].fangan.No,task->task_list[i].fangan.type,
						task->task_list[i].fangan.ti.interval,
						task->task_list[i].fangan.ti.units,
						task->task_list[i].begin.year.data,task->task_list[i].begin.month.data,task->task_list[i].begin.day.data,
						task->task_list[i].begin.hour.data,task->task_list[i].begin.min.data,task->task_list[i].begin.sec.data,
						task->task_list[i].fangan.items[j].sucessflg,
						task->task_list[i].fangan.items[j].item07[3],task->task_list[i].fangan.items[j].item07[2],
						task->task_list[i].fangan.items[j].item07[1],task->task_list[i].fangan.items[j].item07[0]
						);
				break;
			default:
				DbgPrintToFile1(31,"%02d| %04x-%02x%02x - %04x-%02x%02x .%02d任务 执行频率%d[%d] %d级 方案%d ，类型%d 未知采集类型 | %d-%d-%d %d:%d:%d OK=%d cov %02x%02x%02x%02x",
						numindex,
						task->task_list[i].fangan.items[j].oad1.OI,task->task_list[i].fangan.items[j].oad1.attflg,task->task_list[i].fangan.items[j].oad1.attrindex,
						task->task_list[i].fangan.items[j].oad2.OI,task->task_list[i].fangan.items[j].oad2.attflg,task->task_list[i].fangan.items[j].oad2.attrindex,
						task->task_list[i].taskId,
						task->task_list[i].ti.interval,task->task_list[i].ti.units,
						task->task_list[i].leve,
						task->task_list[i].fangan.No,task->task_list[i].fangan.type,
						task->task_list[i].begin.year.data,task->task_list[i].begin.month.data,task->task_list[i].begin.day.data,
						task->task_list[i].begin.hour.data,task->task_list[i].begin.min.data,task->task_list[i].begin.sec.data,
						task->task_list[i].fangan.items[j].sucessflg,
						task->task_list[i].fangan.items[j].item07[3],task->task_list[i].fangan.items[j].item07[2],
						task->task_list[i].fangan.items[j].item07[1],task->task_list[i].fangan.items[j].item07[0]
						);
			}
		}
	}
}
void PrintTaskInfo(TASK_INFO *task,int taski)
{
	int i=0,j=0,numindex=0;
//	for(i=0;i<task->task_n;i++)
	{
		i = taski;
		for(j=0;j<task->task_list[i].fangan.item_n;j++)
		{
			numindex++;
			switch(task->task_list[i].fangan.cjtype)
			{
			case 0:
				DbgPrintToFile1(31,"%02d| %04x-%02x%02x - %04x-%02x%02x .%02d任务 执行频率%d[%d] %d级 方案%d ，类型%d  采集当前数据 | %d-%d-%d %d:%d:%d OK=%d cov %02x%02x%02x%02x",
						numindex,
						task->task_list[i].fangan.items[j].oad1.OI,task->task_list[i].fangan.items[j].oad1.attflg,task->task_list[i].fangan.items[j].oad1.attrindex,
						task->task_list[i].fangan.items[j].oad2.OI,task->task_list[i].fangan.items[j].oad2.attflg,task->task_list[i].fangan.items[j].oad2.attrindex,
						task->task_list[i].taskId,
						task->task_list[i].ti.interval,task->task_list[i].ti.units,
						task->task_list[i].leve,
						task->task_list[i].fangan.No,
						task->task_list[i].fangan.type,
						task->task_list[i].begin.year.data,task->task_list[i].begin.month.data,task->task_list[i].begin.day.data,
						task->task_list[i].begin.hour.data,task->task_list[i].begin.min.data,task->task_list[i].begin.sec.data,
						task->task_list[i].fangan.items[j].sucessflg,
						task->task_list[i].fangan.items[j].item07[3],task->task_list[i].fangan.items[j].item07[2],
						task->task_list[i].fangan.items[j].item07[1],task->task_list[i].fangan.items[j].item07[0]
						);
				break;
			case 1:
				DbgPrintToFile1(31,"%02d| %04x-%02x%02x - %04x-%02x%02x .%02d任务 执行频率%d[%d] %d级 方案%d ，类型%d  采集上%d次 | %d-%d-%d %d:%d:%d OK=%d cov %02x%02x%02x%02x",
						numindex,
						task->task_list[i].fangan.items[j].oad1.OI,task->task_list[i].fangan.items[j].oad1.attflg,task->task_list[i].fangan.items[j].oad1.attrindex,
						task->task_list[i].fangan.items[j].oad2.OI,task->task_list[i].fangan.items[j].oad2.attflg,task->task_list[i].fangan.items[j].oad2.attrindex,
						task->task_list[i].taskId,
						task->task_list[i].ti.interval,task->task_list[i].ti.units,
						task->task_list[i].leve,
						task->task_list[i].fangan.No,task->task_list[i].fangan.type,
						task->task_list[i].fangan.N,
						task->task_list[i].begin.year.data,task->task_list[i].begin.month.data,task->task_list[i].begin.day.data,
						task->task_list[i].begin.hour.data,task->task_list[i].begin.min.data,task->task_list[i].begin.sec.data,
						task->task_list[i].fangan.items[j].sucessflg,
						task->task_list[i].fangan.items[j].item07[3],task->task_list[i].fangan.items[j].item07[2],
						task->task_list[i].fangan.items[j].item07[1],task->task_list[i].fangan.items[j].item07[0]
						);
				break;
			case 2:
				DbgPrintToFile1(31,"%02d| %04x-%02x%02x - %04x-%02x%02x .%02d任务 执行频率%d[%d] %d级 方案%d ，类型%d  按冻结时标采集 | %d-%d-%d %d:%d:%d OK=%d cov %02x%02x%02x%02x",
						numindex,
						task->task_list[i].fangan.items[j].oad1.OI,task->task_list[i].fangan.items[j].oad1.attflg,task->task_list[i].fangan.items[j].oad1.attrindex,
						task->task_list[i].fangan.items[j].oad2.OI,task->task_list[i].fangan.items[j].oad2.attflg,task->task_list[i].fangan.items[j].oad2.attrindex,
						task->task_list[i].taskId,
						task->task_list[i].ti.interval,task->task_list[i].ti.units,
						task->task_list[i].leve,
						task->task_list[i].fangan.No,task->task_list[i].fangan.type,
						task->task_list[i].begin.year.data,task->task_list[i].begin.month.data,task->task_list[i].begin.day.data,
						task->task_list[i].begin.hour.data,task->task_list[i].begin.min.data,task->task_list[i].begin.sec.data,
						task->task_list[i].fangan.items[j].sucessflg,
						task->task_list[i].fangan.items[j].item07[3],task->task_list[i].fangan.items[j].item07[2],
						task->task_list[i].fangan.items[j].item07[1],task->task_list[i].fangan.items[j].item07[0]
						);
				break;
			case 3:
				DbgPrintToFile1(31,"%02d| %04x-%02x%02x - %04x-%02x%02x .%02d任务 执行频率%d[%d] %d级 方案%d ，类型%d  间隔%d (%d) | %d-%d-%d %d:%d:%d OK=%d cov %02x%02x%02x%02x",
						numindex,
						task->task_list[i].fangan.items[j].oad1.OI,task->task_list[i].fangan.items[j].oad1.attflg,task->task_list[i].fangan.items[j].oad1.attrindex,
						task->task_list[i].fangan.items[j].oad2.OI,task->task_list[i].fangan.items[j].oad2.attflg,task->task_list[i].fangan.items[j].oad2.attrindex,
						task->task_list[i].taskId,
						task->task_list[i].ti.interval,task->task_list[i].ti.units,
						task->task_list[i].leve,
						task->task_list[i].fangan.No,task->task_list[i].fangan.type,
						task->task_list[i].fangan.ti.interval,
						task->task_list[i].fangan.ti.units,
						task->task_list[i].begin.year.data,task->task_list[i].begin.month.data,task->task_list[i].begin.day.data,
						task->task_list[i].begin.hour.data,task->task_list[i].begin.min.data,task->task_list[i].begin.sec.data,
						task->task_list[i].fangan.items[j].sucessflg,
						task->task_list[i].fangan.items[j].item07[3],task->task_list[i].fangan.items[j].item07[2],
						task->task_list[i].fangan.items[j].item07[1],task->task_list[i].fangan.items[j].item07[0]
						);
				break;
			default:
				DbgPrintToFile1(31,"%02d| %04x-%02x%02x - %04x-%02x%02x .%02d任务 执行频率%d[%d] %d级 方案%d ，类型%d 未知采集类型 | %d-%d-%d %d:%d:%d OK=%d cov %02x%02x%02x%02x",
						numindex,
						task->task_list[i].fangan.items[j].oad1.OI,task->task_list[i].fangan.items[j].oad1.attflg,task->task_list[i].fangan.items[j].oad1.attrindex,
						task->task_list[i].fangan.items[j].oad2.OI,task->task_list[i].fangan.items[j].oad2.attflg,task->task_list[i].fangan.items[j].oad2.attrindex,
						task->task_list[i].taskId,
						task->task_list[i].ti.interval,task->task_list[i].ti.units,
						task->task_list[i].leve,
						task->task_list[i].fangan.No,task->task_list[i].fangan.type,
						task->task_list[i].begin.year.data,task->task_list[i].begin.month.data,task->task_list[i].begin.day.data,
						task->task_list[i].begin.hour.data,task->task_list[i].begin.min.data,task->task_list[i].begin.sec.data,
						task->task_list[i].fangan.items[j].sucessflg,
						task->task_list[i].fangan.items[j].item07[3],task->task_list[i].fangan.items[j].item07[2],
						task->task_list[i].fangan.items[j].item07[1],task->task_list[i].fangan.items[j].item07[0]
						);
			}
		}
	}
}
void getCjTypeData(CJ_FANGAN *fangAn,CLASS_6015 *Class6015)
{
	fangAn->cjtype = Class6015->cjtype;
	switch(fangAn->cjtype)
	{
		case 1:
			if (Class6015->data.type == dtunsigned)
				fangAn->N = Class6015->data.data[0];  //采集上第N次
			break;
		case 3:
			if (Class6015->data.type == dtti)
				memcpy((INT8U *)&fangAn->ti,&Class6015->data.data[0],sizeof(TI));
			break;
	}
}
int Array_OAD_Items(CJ_FANGAN *fangAn)
{
	int i=0,j=0,num=0, oadtype=0;
	CLASS_6015 Class6015;
	CLASS_6017 Class6017;

	if(fangAn->type == norm)
	{
		readCoverClass(0x6015, fangAn->No, (void *)&Class6015, sizeof(CLASS_6015), coll_para_save);
		for(i=0;i<Class6015.csds.num;i++)//全部CSD循环
		{
			oadtype = Class6015.csds.csd[i].type;
			getCjTypeData(fangAn,&Class6015);
			if(oadtype == 1)//ROAD
			{
				for(j=0;j<Class6015.csds.csd[i].csd.road.num;j++)
				{
					fangAn->items[num].oad1 = Class6015.csds.csd[i].csd.road.oad;
					fangAn->items[num].oad2 = Class6015.csds.csd[i].csd.road.oads[j];
					num++;
					if (num >= FANGAN_ITEM_MAX )
						return num;
				}
			}else			//OAD
			{
				fangAn->items[num].oad1.OI = 0;
				fangAn->items[num].oad2 = Class6015.csds.csd[i].csd.oad;
				num++;
				if (num >= FANGAN_ITEM_MAX )
					return num;
			}
		}
	}
	if(fangAn->type == events)
	{
		readCoverClass(0x6017, fangAn->No, (void *)&Class6017, sizeof(CLASS_6017), coll_para_save);
		for(i=0; i< Class6017.collstyle.roads.num; i++)
		{
			for(j=0;j<Class6017.collstyle.roads.road[i].num;j++)
			{
				fangAn->items[num].oad1 = Class6017.collstyle.roads.road[i].oad;
				fangAn->items[num].oad2 = Class6017.collstyle.roads.road[i].oads[j];
				num++;
				if (num >= FANGAN_ITEM_MAX )
					return num;
			}
		}
	}
	fangAn->item_n = num;
	return num;
}

int task_Refresh(TASK_UNIT *taskunit)
{
	DateTimeBCD ts;
	int i=0,t=0;
	INT8U id=0;
	id = taskunit->taskId;
	DbgPrintToFile1(31,"task_Refresh  重新初始化 任务%d ",id);
	for(i=0;i<TASK6012_MAX;i++)
	{
		if (list6013[i].basicInfo.taskID == id)
		{
			DbgPrintToFile1(31,"找到");
			taskunit->beginTime = calcnexttime(list6013[i].basicInfo.interval,\
					list6013[i].basicInfo.startime,list6013[i].basicInfo.delay);
			ts =  timet_bcd(taskunit->beginTime);
			taskunit->begin = ts;
			for(t=0;t<taskunit->fangan.item_n;t++)
				taskunit->fangan.items[t].sucessflg = 0;
		}
	}
	return t;
}

//int task_leve(INT8U leve,TASK_UNIT *taskunit,INT8U usrType,TSA usrAddr)
int task_leve(INT8U leve,TASK_UNIT *taskunit)
{
	DateTimeBCD ts;
	int i=0,t=0;
	INT8U type=0 ,serNo=0;

	for(i=0;i<TASK6012_MAX;i++)
	{
		if (list6013[i].basicInfo.runprio == leve && list6013[i].basicInfo.taskID>0)
		{
			taskunit[t].taskId = list6013[i].basicInfo.taskID;
			taskunit[t].leve = list6013[i].basicInfo.runprio;
			taskunit[t].beginTime = calcnexttime(list6013[i].basicInfo.interval,list6013[i].basicInfo.startime,list6013[i].basicInfo.delay);//list6013[i].ts_next;
			taskunit[t].endTime = tmtotime_t( DateBCD2Ts(list6013[i].basicInfo.endtime ));
			ts =   timet_bcd(taskunit[t].beginTime);
			taskunit[t].begin = ts;
			taskunit[t].end = list6013[i].basicInfo.endtime;
			type = list6013[i].basicInfo.cjtype;
			serNo = list6013[i].basicInfo.sernum;//方案序号
			memset(&taskunit[t].fangan,0,sizeof(CJ_FANGAN));
			taskunit[t].fangan.type = type;
			taskunit[t].fangan.No = serNo;
			taskunit[t].ti = list6013[i].basicInfo.interval;
			taskunit[t].fangan.item_n = Array_OAD_Items(&taskunit[t].fangan);
			t++;
		}
	}
	return t;
}
/*初始化 全部 普通采集方案6015数组  （CLASS_6015 task6015[20] 为请求抄读时提供参数支持）*/
void task_init6015(CLASS_6015 *fangAn6015p)
{
	int i=0,j=0;
	for(i=0;i<TASK6012_MAX;i++)
	{
		if (list6013[i].basicInfo.cjtype == norm)//普通采集任务
		{

			readCoverClass(0x6015, list6013[i].basicInfo.sernum, (void *)&fangAn6015p[j], sizeof(CLASS_6015), coll_para_save);
			DbgPrintToFile1(31,"fangAn6015p[%d].sernum = %d  fangAn6015p[%d].mst = %d ",
					j,fangAn6015p[j].sernum,j,fangAn6015p[j].mst.mstype);
			j++;
			if(j>=20)
				break;
		}
	}
}
int initTaskData(TASK_INFO *task)
{
	int num =0;
	if (task==NULL)
		return 0;
	memset(task,0,sizeof(TASK_INFO));
	memset(fangAn6015,0,sizeof(fangAn6015));

	task_init6015(fangAn6015);
	num += task_leve(0,&task->task_list[num]);
	num += task_leve(1,&task->task_list[num]);
	num += task_leve(2,&task->task_list[num]);
	num += task_leve(3,&task->task_list[num]);
	num += task_leve(4,&task->task_list[num]);
	task->task_n = num;
	return 1;
}
int initTsaList(struct Tsa_Node **head)
{
	int i=0, record_num=0 ,n=0;
	CLASS_6001	 meter={};
	struct Tsa_Node *p=NULL;
	struct Tsa_Node *tmp=NULL;

	record_num = getFileRecordNum(0x6000);
	for(i=0;i<record_num;i++)
	{
		if(readParaClass(0x6000,&meter,i)==1)
		{
			if (meter.sernum!=0 && meter.sernum!=0xffff && meter.basicinfo.port.OI==0xf209)
			{
				tmp = (struct Tsa_Node *)malloc(sizeof(struct Tsa_Node));
				memcpy(&tmp->tsa , &meter.basicinfo.addr,sizeof(TSA));
				tmp->protocol = meter.basicinfo.protocol;
				tmp->usrtype = meter.basicinfo.usrtype;
				tmp->readnum = 0;
				tmp->tsa_index = meter.sernum;
				memset(tmp->flag,0,sizeof(tmp->flag));
				if (tmp!=NULL)
				{
					if (n==0)
					{
						p  = tmp;
						*head = p;
					}else
					{
						p->next = tmp;
						p = p->next;
					}
					n++;
				}
			}
		}
	}
	if (p!=NULL)
	{
		p->next = NULL;
	}
	return n;
}

void reset_ZB()
{
	gpio_writebyte((char *)"/dev/gpoZAIBO_RST", 0);
//	usleep(500*1000);
	sleep (1);
//	sleep(5);
	gpio_writebyte((char *)"/dev/gpoZAIBO_RST", 1);
	sleep(10);
}
void tsa_print(struct Tsa_Node *head,int num)
{
	int j = 0,i=0;
	struct Tsa_Node *tmp = NULL;
	tmp = head;
	fprintf(stderr,"\ntmp = %p",tmp);
	while(tmp!=NULL)
	{
		fprintf(stderr,"\nTSA%d: %d-",i,tmp->tsa.addr[0]);
		for(j=0;j<tmp->tsa.addr[0];j++)
		{
			fprintf(stderr,"-%02x",tmp->tsa.addr[j+1]);
		}
		fprintf(stderr,"\nFLAG: %02x %02x %02x %02x %02x %02x %02x %02x\n",tmp->flag[0],tmp->flag[1],tmp->flag[2],tmp->flag[3],tmp->flag[4],tmp->flag[5],tmp->flag[6],tmp->flag[7]);
		fprintf(stderr,"\nProtocol = %d  curr_i=%d   usertype=%02x\n\n",tmp->protocol,tmp->curr_i,tmp->usrtype);
		tmp = tmp->next;
		i++;
	}
}
TSA getNextTsa(struct Tsa_Node **p)
{
	TSA tsatmp;
	memset(&tsatmp,0,sizeof(TSA));
	if (*p!=NULL)
	{
		tsatmp = (*p)->tsa;
		*p = (*p)->next;
	}
	return tsatmp;
}
//在档案中查找指定TSA
int findTsaInList(struct Tsa_Node *head,struct Tsa_Node *desnode)
{
	struct Tsa_Node *p=NULL;
	p = head;
	while(p!=NULL && desnode!=NULL)
	{
		if(memcmp(&(p->tsa.addr[2]),&(desnode->tsa.addr[2]), desnode->tsa.addr[1]+1)==0)
		{
			desnode->readnum = p->readnum;
			desnode->protocol = p->protocol;
			desnode->usrtype = p->usrtype;
			return 1;
		}
		else
			p = p->next;
	}
	return 0;
}
struct Tsa_Node *getNodeByTSA(struct Tsa_Node *head,TSA tsa)
{
	struct Tsa_Node *p=NULL;
	p = head;
	while(p!=NULL )
	{
		if(memcmp(&(p->tsa.addr[2]),&tsa.addr[2], tsa.addr[1]+1)==0)
		{
			return p;
		}
		else
			p = p->next;
	}
	return NULL;
}
void freeList(struct Tsa_Node *head)
{
	struct Tsa_Node *tmp;
	while(head!=NULL)
	{
		tmp = head;
		head = head->next;
		free(tmp);
	}
	tmp = NULL;
	return;
}
/**********************************************************************
 * 将表地址添加到链表尾   addr为标准6字节表地址 非TSA格式，赋值时将TSA格式补充完整
 *********************************************************************/
void addTsaList(struct Tsa_Node **head,INT8U *addr)
{
	struct Tsa_Node *tmp = *head;
	struct Tsa_Node *new;
	tsa_zb_count = 0;
	if (*head==NULL)
	{
		*head = (struct Tsa_Node *)malloc(sizeof(struct Tsa_Node));
		(*head)->tsa.addr[0] = 7;
		(*head)->tsa.addr[1] = 5;
//		memcpy(&(*head)->tsa.addr[2] , addr,6);
		(*head)->tsa.addr[2] = addr[5];
		(*head)->tsa.addr[3] = addr[4];
		(*head)->tsa.addr[4] = addr[3];
		(*head)->tsa.addr[5] = addr[2];
		(*head)->tsa.addr[6] = addr[1];
		(*head)->tsa.addr[7] = addr[0];

		(*head)->next = NULL;
		tsa_zb_count = 1;
	}else
	{
		while(tmp->next != NULL)
		{
			tmp = tmp->next;
			tsa_zb_count++;
		}
		new = (struct Tsa_Node *)malloc(sizeof(struct Tsa_Node));
		new->tsa.addr[0] = 7;
		new->tsa.addr[1] = 5;
//		memcpy(&(new->tsa.addr[2]) , addr,6);//TODO: TSA长度未赋值
		new->tsa.addr[2] = addr[5];
		new->tsa.addr[3] = addr[4];
		new->tsa.addr[4] = addr[3];
		new->tsa.addr[5] = addr[2];
		new->tsa.addr[6] = addr[1];
		new->tsa.addr[7] = addr[0];
		new->next = NULL;
		tmp->next = new;
		tsa_zb_count++;
	}
	return;
}
void printModelinfo(AFN03_F10_UP info)
{
	DbgPrintToFile1(31,"硬件复位");
	DbgPrintToFile1(31,"\n\n--------------------------------\n\n厂商信息:%c%c%c%c \nDate:%d-%d-%d \nVersion:%d%d",
			info.ModuleInfo.VendorCode[1],
			info.ModuleInfo.VendorCode[0],
			info.ModuleInfo.ChipCode[1],
			info.ModuleInfo.ChipCode[0],
			info.ModuleInfo.VersionYear,
			info.ModuleInfo.VersionMonth,
			info.ModuleInfo.VersionDay,
			info.ModuleInfo.Version[1],
			info.ModuleInfo.Version[0]);
	DbgPrintToFile1(31,"\n主节点地址:%02x%02x%02x%02x%02x%02x",
			info.MasterPointAddr[5],
			info.MasterPointAddr[4],
			info.MasterPointAddr[3],
			info.MasterPointAddr[2],
			info.MasterPointAddr[1],
			info.MasterPointAddr[0]);
	DbgPrintToFile1(31,"\nMonitorOverTime=%d 秒", info.MonitorOverTime);
	DbgPrintToFile1(31,"\nReadMode=%02x\n--------------------------------\n\n", info.ReadMode);

}
void clearvar(RUNTIME_PLC *runtime_p)
{
	memset(runtime_p->recvbuf,0, ZBBUFSIZE);
	memset(runtime_p->dealbuf,0, ZBBUFSIZE);
	runtime_p->send_start_time = 0;
	memset(&runtime_p->format_Up,0,sizeof(runtime_p->format_Up));
}
int doInit(RUNTIME_PLC *runtime_p)
{
	static int step_init = 0;
	static int read_num = 0;
	int sendlen=0;
	time_t nowtime = time(NULL);
	if (runtime_p->initflag == 1)
		step_init = 0;
	switch(step_init )
	{
		case 0://初始化
			DbgPrintToFile1(31,"硬件复位...");
			freeList(tsa_head);
			freeList(tsa_zb_head);
			tsa_head = NULL;
			tsa_zb_head = NULL;
			tsa_count = initTsaList(&tsa_head);
			tsa_print(tsa_head,tsa_count);

			if (runtime_p->comfd >0)
				CloseCom( runtime_p->comfd );

			runtime_p->comfd = OpenCom(5, 9600,(unsigned char*)"even",1,8);
			DbgPrintToFile1(31,"comfd=%d",runtime_p->comfd);
			runtime_p->initflag = 0;
			clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
			reset_ZB();

			fprintf(stderr,"\n-----------tsacount=%d",tsa_count);
			if (tsa_count <= 0)
			{
				DbgPrintToFile1(31,"无载波测量点");
				step_init = 0;
				return NONE_PROCE;
			}
			runtime_p->send_start_time = nowtime ;
			step_init = 1;
			read_num = 0;
			break;

		case 1://读取载波信息
			fprintf(stderr,"\n读取信息状态 nowtime = %ld  runtime_p->send_start_time =%ld",nowtime,runtime_p->send_start_time );
			fprintf(stderr,"\nruntime_p->format_Up.afn= %02x  runtime_p->format_Up.fn=%d",runtime_p->format_Up.afn,runtime_p->format_Up.fn);
			DbgPrintToFile1(31,"读取载波信息");
			if ((nowtime  - runtime_p->send_start_time > 60) &&
				runtime_p->format_Up.afn != 0x03 && runtime_p->format_Up.fn!= 10)
			{
				fprintf(stderr,"\n读取载波信息");
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
				runtime_p->send_start_time = nowtime ;
				sendlen = AFN03_F10(&runtime_p->format_Down,runtime_p->sendbuf);//查询载波模块信息
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				read_num ++;
			}else if (runtime_p->format_Up.afn == 0x03 && runtime_p->format_Up.fn == 10)
			{//返回载波信息
				fprintf(stderr,"\n返回载波信息");
				memcpy(&module_info,&runtime_p->format_Up.afn03_f10_up,sizeof(module_info));
				printModelinfo(module_info);
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
				step_init = 0;
				sleep(1);
				return INIT_MASTERADDR;
			}
			//else if  (runtime_p->send_start_time !=0 && (nowtime  - runtime_p->send_start_time)>10)
			else if (read_num>=3)
			{//超时
				fprintf(stderr,"\n读取载波信息超时");
				DbgPrintToFile1(31,"读取载波信息超时");
				step_init = 0;
			}
			break;
	}
	return DATE_CHANGE;
}
int doSetMasterAddr(RUNTIME_PLC *runtime_p)
{
	INT8U masteraddr[6] = {0x01,0x02,0x03,0x04,0x05,0x06};
	int sendlen=0;
	static int step_MasterAddr = 0;
	time_t nowtime = time(NULL);
	switch(step_MasterAddr )
	{
		case 0://查询主节点地址
			if ((nowtime  - runtime_p->send_start_time > 20) &&
				runtime_p->format_Up.afn != 0x03 && runtime_p->format_Up.fn!= 4)
			{
				sendlen = AFN03_F4(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				DbgPrintToFile1(31,"查询主节点地址");
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
				runtime_p->send_start_time = nowtime ;
			}else if(runtime_p->format_Up.afn == 0x03 && runtime_p->format_Up.fn == 4)
			{//返回从节点数量
				DbgPrintToFile1(31,"载波模块主节点 ：%02x%02x%02x%02x%02x%02x ",\
						runtime_p->format_Up.afn03_f4_up.MasterPointAddr[0],
						runtime_p->format_Up.afn03_f4_up.MasterPointAddr[1],
						runtime_p->format_Up.afn03_f4_up.MasterPointAddr[2],
						runtime_p->format_Up.afn03_f4_up.MasterPointAddr[3],
						runtime_p->format_Up.afn03_f4_up.MasterPointAddr[4],
						runtime_p->format_Up.afn03_f4_up.MasterPointAddr[5]);
				step_MasterAddr = 1;
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
			}
			break;
		case 1:
			if (nowtime  - runtime_p->send_start_time > 20)
			{
				DbgPrintToFile1(31,"\n设置主节点地址 ");
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
				memcpy(runtime_p->masteraddr,masteraddr,6);
				sendlen = AFN05_F1(&runtime_p->format_Down,runtime_p->sendbuf,masteraddr);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				runtime_p->send_start_time = nowtime;
			}else if (runtime_p->format_Up.afn==0x00 && (runtime_p->format_Up.fn==1))
			{
				clearvar(runtime_p);
				DbgPrintToFile1(31,"\n设置主节点完成");

				return SLAVE_COMP;
			}
			break;
	}
	return INIT_MASTERADDR ;
}
int doCompSlaveMeter(RUNTIME_PLC *runtime_p)
{
	static int step_cmpslave = 0;
	static unsigned int slavenum = 0;
	static int index=0;
	static struct Tsa_Node *currtsa;//=tsa_zb_head;
	struct Tsa_Node nodetmp;
	int i=0, sendlen=0, findflg=0;
	INT8U addrtmp[6]={};
	time_t nowtime = time(NULL);
	switch(step_cmpslave)
	{
		case 0://读取载波从节点数量
			if ((nowtime  - runtime_p->send_start_time > 20) &&
				runtime_p->format_Up.afn != 0x10 && runtime_p->format_Up.fn!= 1)
			{
				sendlen = AFN12_F2(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				sleep(2);
				DbgPrintToFile1(31,"读从节点数量");
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
				runtime_p->send_start_time = nowtime ;
				sendlen = AFN10_F1(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
			}else if(runtime_p->format_Up.afn == 0x10 && runtime_p->format_Up.fn == 1)
			{//返回从节点数量
				slavenum = runtime_p->format_Up.afn10_f1_up.Num ;
				DbgPrintToFile1(31,"载波模块中从节点 %d 个",slavenum);
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
				step_cmpslave = 1;
				index = 1;
			}
			break;

		case 1://读取全部载波从节点
			if ((nowtime  - runtime_p->send_start_time > 20) &&
				runtime_p->format_Up.afn != 0x10 && runtime_p->format_Up.fn!= 2)
			{
				DbgPrintToFile1(31,"读从节点信息 %d ",index);
				if (slavenum > 10)
				{
					sendlen = AFN10_F2(&runtime_p->format_Down,runtime_p->sendbuf,index,10);
				}
				else if(slavenum > 0)
				{
					sendlen = AFN10_F2(&runtime_p->format_Down,runtime_p->sendbuf,index,slavenum);
				}else
				{
					step_cmpslave = 3;
					clearvar(runtime_p);
					currtsa = tsa_head;	//删除完成 ,开始第 3 步
					break;
				}

				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime;
			}else if (runtime_p->format_Up.afn == 0x10 && runtime_p->format_Up.fn == 2)
			{
				int replyn = runtime_p->format_Up.afn10_f2_up.ReplyNum;
				DbgPrintToFile1(31,"返回从节点数量 %d ",replyn);
				slavenum -= replyn;
				index += replyn;
				for(i=0; i<replyn; i++)
				{
					addTsaList(&tsa_zb_head,runtime_p->format_Up.afn10_f2_up.SlavePoint[i].Addr);
				}
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
				if (slavenum==0)
				{
					DbgPrintToFile1(31,"读取结束 读%d 个  实际 %d 个",index,slavenum);
					tsa_print(tsa_zb_head,slavenum);
					step_cmpslave = 2;//读取结束
					currtsa = tsa_zb_head;
				}
			}
			break;

		case 2://删除多余节点
			if (nowtime  - runtime_p->send_start_time > 10)
			{
				for(;;)
				{
					if (currtsa == NULL)
					{
						DbgPrintToFile1(31,"删除判断完成!");
						step_cmpslave = 3;
						clearvar(runtime_p);
						currtsa = tsa_head;	//删除完成 ,开始第 3 步
						break;
					}else
					{
						nodetmp.tsa = getNextTsa(&currtsa); //从载波模块的档案中取出一个 tsa
						findflg = findTsaInList(tsa_head,&nodetmp);
						if (findflg==0)
						{
							DbgPrintToFile1(31,"节点  tsatmp (%d): %02x %02x %02x %02x %02x %02x %02x %02x  删除",findflg,nodetmp.tsa.addr[0],nodetmp.tsa.addr[1],nodetmp.tsa.addr[2],nodetmp.tsa.addr[3],nodetmp.tsa.addr[4],nodetmp.tsa.addr[5],nodetmp.tsa.addr[6],nodetmp.tsa.addr[7]);
							addrtmp[5] = nodetmp.tsa.addr[2];
							addrtmp[4] = nodetmp.tsa.addr[3];
							addrtmp[3] = nodetmp.tsa.addr[4];
							addrtmp[2] = nodetmp.tsa.addr[5];
							addrtmp[1] = nodetmp.tsa.addr[6];
							addrtmp[0] = nodetmp.tsa.addr[7];
							sendlen = AFN11_F2(&runtime_p->format_Down,runtime_p->sendbuf, addrtmp);//&nodetmp.tsa.addr[2]);//在载波模块中删除一个表地址，addr[0]=7 addr[1]=5固定
							SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
							runtime_p->send_start_time = nowtime;
							break;
						}
					}
				}
			}else if (runtime_p->format_Up.afn==0x00 && (runtime_p->format_Up.fn==1))
			{
				clearvar(runtime_p);
			}
			break;

		case 3://添加节点
			if (nowtime  - runtime_p->send_start_time > 10)
			{
				for(;;)
				{
					if (currtsa == NULL)
					{
						DbgPrintToFile1(31,"添加判断完成");
						step_cmpslave = 0;
						clearvar(runtime_p);
						return TASK_PROCESS;
					}else
					{
						nodetmp.tsa = getNextTsa(&currtsa);	//从档案中取一个tsa
						findflg = findTsaInList(tsa_zb_head,&nodetmp);
						if (findflg == 0)
						{
							DbgPrintToFile1(31,"节点  tsatmp (%d): %02x %02x %02x %02x %02x %02x %02x %02x  添加",findflg,nodetmp.tsa.addr[0],nodetmp.tsa.addr[1],nodetmp.tsa.addr[2],nodetmp.tsa.addr[3],nodetmp.tsa.addr[4],nodetmp.tsa.addr[5],nodetmp.tsa.addr[6],nodetmp.tsa.addr[7]);
							addrtmp[5] = nodetmp.tsa.addr[2];
							addrtmp[4] = nodetmp.tsa.addr[3];
							addrtmp[3] = nodetmp.tsa.addr[4];
							addrtmp[2] = nodetmp.tsa.addr[5];
							addrtmp[1] = nodetmp.tsa.addr[6];
							addrtmp[0] = nodetmp.tsa.addr[7];
							sendlen = AFN11_F1(&runtime_p->format_Down,runtime_p->sendbuf, addrtmp);//&nodetmp.tsa.addr[2]);//在载波模块中添加一个TSA
							SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
							runtime_p->send_start_time = nowtime;
							break;
						}
					}
				}
			}else if (runtime_p->format_Up.afn==0x00 && (runtime_p->format_Up.fn==1))
			{
				clearvar(runtime_p);
			}
	}
	return SLAVE_COMP ;
}
void Addr_TSA(INT8U *addr,TSA *tsa)
{
	tsa->addr[0] = 7;
	tsa->addr[1] = 5;
	tsa->addr[2] = addr[5];
	tsa->addr[3] = addr[4];
	tsa->addr[4] = addr[3];
	tsa->addr[5] = addr[2];
	tsa->addr[6] = addr[1];
	tsa->addr[7] = addr[0];


//	memcpy(&tsa->addr[2],addr,6);
}
int Echo_Frame(RUNTIME_PLC *runtime_p,INT8U *framebuf,int len)
{
	int sendlen = 0,flag = 0;
	if ( len > 1 )
	{
		flag = 2;
		DEBUG_TIME_LINE("\n可以抄读！");
	}
	else if ( len == 1)
	{
		flag = 0;
		DEBUG_TIME_LINE("\n失败切表！");//失败切表
		sleep(2);
	}else
	{
		flag = 1;
		DEBUG_TIME_LINE("\n抄读成功！");//成功切表
	}
	memcpy(runtime_p->format_Down.addr.SourceAddr,runtime_p->masteraddr,6);
	sendlen = AFN14_F1(&runtime_p->format_Down,&runtime_p->format_Up,runtime_p->sendbuf,\
			          runtime_p->format_Up.afn14_f1_up.SlavePointAddr, \
			          flag, 0, len, framebuf);
	SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
	clearvar(runtime_p);
	return flag;
}
/***********************************************************************
 *  从index指向的bit开始，找到第一个为0的位 返回该位标示的数据项索引
 *  然后，index指针向前移动1
 *  desnode位当前电表信息节点地址，itemnum：当前采集方案数据项总数
 *  返回值： -1  标识bit返回到第0位
 **********************************************************************/
int findFirstZeroFlg(struct Tsa_Node *desnode,int itemnum)
{

	int byten =0, biti =0 ,index=0;
	do
	{
		index = desnode->curr_i;			//当前需要抄读的数据项索引  62   7, >>6
		byten = index / 8;
		biti = index % 8;

		desnode->curr_i = (desnode->curr_i + 1) % itemnum;
		if (desnode->curr_i==0)
			desnode->readnum++;
		if (((desnode->flag[byten] >> biti) & 0x01) == 0 )
		{
			fprintf(stderr,"\n第 %d 数据项 ,需要抄",index);
			return index;
		}
	}while(index!=itemnum-1);
	return -1;
}

int Format07(FORMAT07 *Data07,OAD oad1,OAD oad2,TSA tsa)
{
	INT8U startIndex =0;
	int find_07item = 0;
	C601F_645 obj601F_07Flag;

	memset(Data07, 0, sizeof(FORMAT07));
	obj601F_07Flag.protocol = 2;
	find_07item = OADMap07DI(oad1.OI,oad2,&obj601F_07Flag) ;
	DbgPrintToFile1(31,"find_07item=%d   %04x %04x    07Flg %02x%02x%02x%02x",find_07item,oad1.OI,oad2.OI,
			obj601F_07Flag.DI._07.DI_1[0][3],obj601F_07Flag.DI._07.DI_1[0][2],obj601F_07Flag.DI._07.DI_1[0][1],obj601F_07Flag.DI._07.DI_1[0][0]);
	if (find_07item == 1)
	{
		Data07->Ctrl = 0x11;
		startIndex = 5 - tsa.addr[1];
		memcpy(&Data07->Addr[startIndex], &tsa.addr[2], (tsa.addr[1]+1));
//		memcpy(Data07->DI, &obj601F_07Flag.DI_1[0], 4);
		memcpy(Data07->DI, &obj601F_07Flag.DI._07.DI_1[0], 4);

		return 1;
	}
	return 0;
}
int buildProxyFrame(RUNTIME_PLC *runtime_p,struct Tsa_Node *desnode,Proxy_Msg pMsg)
{
	int sendlen = 0;
	INT8U type = 0;
	FORMAT07 Data07;
	OAD requestOAD1;
	OAD requestOAD2;
	INT8U addrtmp[6];
	type = desnode->protocol;
	switch (type)
	{
		case DLT645_07:
			requestOAD1.OI = 0;
			requestOAD2.OI = pMsg.oi;
			requestOAD2.attflg = 0x02;
			requestOAD2.attrindex = 0x00;

			Format07(&Data07,requestOAD1,requestOAD2,desnode->tsa);
			memset(buf645,0,BUFSIZE645);
			sendlen = composeProtocol07(&Data07, buf645);
			DbgPrintToFile1(31,"sendlen=%d",sendlen);
			DbPrt1(31,"645:", (char *) buf645, sendlen, NULL);
			if (sendlen>0)
			{
				memcpy(runtime_p->format_Down.addr.SourceAddr,runtime_p->masteraddr,6);
				addrtmp[5] = desnode->tsa.addr[2];
				addrtmp[4] = desnode->tsa.addr[3];
				addrtmp[3] = desnode->tsa.addr[4];
				addrtmp[2] = desnode->tsa.addr[5];
				addrtmp[1] = desnode->tsa.addr[6];
				addrtmp[0] = desnode->tsa.addr[7];
				return (AFN13_F1(&runtime_p->format_Down,runtime_p->sendbuf,addrtmp, 2, 0, buf645, sendlen));
			}
			break;
		case DLT698:
			return 20;
	}

	return 0;
}
int saveProxyData(FORMAT3762 format_3762_Up)
{
	INT8U buf645[255];
	int len645=0;
	INT8U nextFlag=0;
	FORMAT07 frame07;
	INT8U dataContent[50];
	memset(dataContent,0,sizeof(dataContent));

	if (format_3762_Up.afn13_f1_up.MsgLength > 0)
	{
		len645 = format_3762_Up.afn13_f1_up.MsgLength;
		memcpy(buf645, format_3762_Up.afn13_f1_up.MsgContent, len645);
		if (analyzeProtocol07(&frame07, buf645, len645, &nextFlag) == 0)
		{
			if(data07Tobuff698(frame07,dataContent) > 0)
			{
				TSGet(&p_Proxy_Msg_Data->realdata.tm_collect);
				memcpy(p_Proxy_Msg_Data->realdata.data_All,&dataContent[3],4);
				memcpy(p_Proxy_Msg_Data->realdata.Rate1_Data,&dataContent[8],4);
				memcpy(p_Proxy_Msg_Data->realdata.Rate2_Data,&dataContent[13],4);
				memcpy(p_Proxy_Msg_Data->realdata.Rate3_Data,&dataContent[18],4);
				memcpy(p_Proxy_Msg_Data->realdata.Rate4_Data,&dataContent[23],4);
			}
			else
			{
				memset(&p_Proxy_Msg_Data->realdata,0xee,sizeof(RealDataInfo));
			}
			p_Proxy_Msg_Data->done_flag = 1;
		}
	}
	return 0;
}
int findFangAnIndex(int code)
{
	int i=0;
	for(i=0;i<20;i++)
	{
		if (fangAn6015[i].sernum == code)
		{
			return i;
		}
	}
	return -1;
}


DATA_ITEM checkMeterData(TASK_INFO *meterinfo,int *taski,int *itemi,INT8U usrtype)
{
	int i=0,j=0,needflg=0;
	time_t nowt = time(NULL);
	DATA_ITEM item;
	int fangAnIndex = 0;
	memset(&item,0,sizeof(DATA_ITEM));
	DbgPrintToFile1(31,"检查  %02x%02x%02x%02x%02x%02x%02x%02x  index=%d 任务数 %d   数据相个数 %d",
			meterinfo->tsa.addr[0],meterinfo->tsa.addr[1],meterinfo->tsa.addr[2],
			meterinfo->tsa.addr[3],meterinfo->tsa.addr[4],meterinfo->tsa.addr[5],
			meterinfo->tsa.addr[6],meterinfo->tsa.addr[7],
			meterinfo->tsa_index,meterinfo->task_n,meterinfo->task_list[i].fangan.item_n);

	DEBUG_TIME_LINE("检查  %02x%02x%02x%02x%02x%02x%02x%02x  index=%d 任务数 %d   数据相个数 %d",
			meterinfo->tsa.addr[0],meterinfo->tsa.addr[1],meterinfo->tsa.addr[2],
			meterinfo->tsa.addr[3],meterinfo->tsa.addr[4],meterinfo->tsa.addr[5],
			meterinfo->tsa.addr[6],meterinfo->tsa.addr[7],
			meterinfo->tsa_index,meterinfo->task_n,meterinfo->task_list[i].fangan.item_n);
	//正常任务判断
	for(i=0; i< meterinfo->task_n; i++)
	{
		if (nowt >= meterinfo->task_list[i].beginTime && meterinfo->task_list[i].tryAgain != 0x55 )
		{
			//判断是否需要抄读
			fangAnIndex = findFangAnIndex(meterinfo->task_list[i].fangan.No);//查被抄电表当前任务的采集方案编号，在6015中的索引
			if (fangAnIndex >=0 )
			{
				needflg = checkMeterType(fangAn6015[fangAnIndex].mst, usrtype ,meterinfo->tsa);//查被抄电表的用户类型 是否满足6015中的用户类型条件
			}
			if (needflg == 1)
			{
				 DEBUG_TIME_LINE("meterinfo->task_list[i].fangan.items[j].sucessflg: %d", meterinfo->task_list[i].fangan.items[j].sucessflg);
				 for(j = 0; j<meterinfo->task_list[i].fangan.item_n; j++)
				 {
					 DEBUG_TIME_LINE("meterinfo->task_list[i].fangan.items[j].sucessflg: %d", meterinfo->task_list[i].fangan.items[j].sucessflg);
					 if ( meterinfo->task_list[i].fangan.items[j].sucessflg==0)
					 {
						 item.oad1 = meterinfo->task_list[i].fangan.items[j].oad1;
						 item.oad2 = meterinfo->task_list[i].fangan.items[j].oad2;
						 *taski = i;
						 *itemi = j;
						 DbgPrintToFile1(31,"常规任务,满足抄读条件数据项");
						 return item;	//存在常规任务，满足抄读条件数据项
					 }
				 }
				 DEBUG_TIME_LINE("current has no item to read");
			}
		}
	}
	//需要补抄的任务
	//-----------------------------------------------------------
	for(i=0; i< meterinfo->task_n; i++)
	{
		if (nowt >= meterinfo->task_list[i].beginTime && meterinfo->task_list[i].tryAgain == 0x55 )
		{
			//判断是否需要抄读
			fangAnIndex = findFangAnIndex(meterinfo->task_list[i].fangan.No);//查被抄电表当前任务的采集方案编号，在6015中的索引
			if (fangAnIndex >=0 )
			{
				needflg = checkMeterType(fangAn6015[fangAnIndex].mst, usrtype ,meterinfo->tsa);//查被抄电表的用户类型 是否满足6015中的用户类型条件
			}
			if (needflg == 1)
			{
				 for(j = 0; j<meterinfo->task_list[i].fangan.item_n; j++)
				 {
					 if ( meterinfo->task_list[i].fangan.items[j].sucessflg==0)
					 {
						 item.oad1 = meterinfo->task_list[i].fangan.items[j].oad1;
						 item.oad2 = meterinfo->task_list[i].fangan.items[j].oad2;
						 *taski = i;
						 *itemi = j;
						 DbgPrintToFile1(31,"补抄数据,满足抄读条件");
						 return item;
					 }
				 }
				 DEBUG_TIME_LINE("no item need reRead");
				 meterinfo->task_list[i].tryAgain = 0;//已经无补抄数据，清除补抄标识
				 task_Refresh(&taskinfo.task_list[i] );
			}
		}
	}

	return item;
}

int createMeterFrame(struct Tsa_Node *desnode,DATA_ITEM item,INT8U *buf,INT8U *item07)
{
	INT8U type = 0;
	FORMAT07 Data07;
	int sendlen = 0 ;

	if (desnode == NULL)
		return 0;
	type = desnode->protocol;
	memset(buf,0,BUFSIZE645);
	switch (type)
	{
		case DLT645_07:
			Format07(&Data07,item.oad1,item.oad2,desnode->tsa);
			DbgPrintToFile1(31,"当前抄读 【OAD1 %04x-%02x %02x    OAD2 %04x-%02x %02x】%02x%02x%02x%02x ",
						item.oad1.OI,item.oad1.attflg,item.oad1.attrindex,item.oad2.OI,item.oad2.attflg,item.oad2.attrindex,
						Data07.DI[3],Data07.DI[2],Data07.DI[1],Data07.DI[0]);

			sendlen = composeProtocol07(&Data07, buf);
			if (sendlen>0)
			{
				memcpy(item07,Data07.DI,4);// 保存07规约数据项
//					DbPrt1(31,"645:", (char *) buf, sendlen, NULL);
				return sendlen;
			}
			break;
		case DLT698:
			return 20;
	}
	return 0;
}
int createMeterFrame_Curve(struct Tsa_Node *desnode,DATA_ITEM item,INT8U *buf,INT8U *item07,DateTimeBCD timebcd)
{
	INT8U type = 0;
	FORMAT07 Data07;
	INT8U CurveFlg[4] = {0x01, 0x00, 0x00, 0x06};//给定时间记录块
	int sendlen = 0 ;
	INT8U startIndex =0;
	if (desnode == NULL)
		return 0;
	type = desnode->protocol;
	memset(buf,0,BUFSIZE645);
	switch (type)
	{
		case DLT645_07:
			Data07.Ctrl = 0xff;
			startIndex = 5 - desnode->tsa.addr[1];
			memcpy(&Data07.Addr[startIndex], &desnode->tsa.addr[2], (desnode->tsa.addr[1]+1));
			memcpy(Data07.DI, &CurveFlg, 4);
			Data07.sections = 1;
			int32u2bcd(timebcd.year.data - 2000, &Data07.startYear,positive);
			int32u2bcd(timebcd.month.data,&Data07.startMonth,positive);
			int32u2bcd(timebcd.day.data,&Data07.startDay,positive);
			int32u2bcd(timebcd.hour.data,&Data07.startHour,positive);
			int32u2bcd(timebcd.min.data,&Data07.startMinute,positive);
			DbgPrintToFile1(31,"BCD %02x-%02x-%02x %02x:%02x",Data07.startYear,Data07.startMonth,Data07.startDay,Data07.startHour,Data07.startMinute);
			sendlen = composeProtocol07(&Data07, buf);

			if (sendlen>0)
			{
				memcpy(item07,Data07.DI,4);// 保存07规约数据项
				DbgPrintToFile1(31,"读负荷曲线 【OAD1 %04x-%02x %02x    OAD2 %04x-%02x %02x】%02x%02x%02x%02x ",
							item.oad1.OI,item.oad1.attflg,item.oad1.attrindex,item.oad2.OI,item.oad2.attflg,item.oad2.attrindex,
							Data07.DI[3],Data07.DI[2],Data07.DI[1],Data07.DI[0]);
				return sendlen;
			}
			break;
		case DLT698:
			return 20;
	}
	return 0;
}

int ifTsaValid(TSA tsa)
{
	if(tsa.addr[0]!=0 || tsa.addr[1]!=0 || tsa.addr[2]!=0|| tsa.addr[3]!=0|| tsa.addr[4]!=0|| tsa.addr[5]!=0
			|| tsa.addr[6]!=0|| tsa.addr[7]!=0|| tsa.addr[8]!=0|| tsa.addr[9]!=0|| tsa.addr[10]!=0|| tsa.addr[11]!=0
			|| tsa.addr[12]!=0|| tsa.addr[13]!=0|| tsa.addr[14]!=0|| tsa.addr[15]!=0|| tsa.addr[16]!=0)
	{
		return 1;
	}
	return 0;
}
void zeroitemflag(TASK_INFO *task)
{
	int t=0,j=0;
	for(t=0;t<task->task_n;t++)
	{
		for(j=0;j<task->task_list[t].fangan.item_n;j++)
		{
			task->task_list[t].fangan.items[j].sucessflg = 0;
		}
	}
}

int do_5004_type( int taski, int itemi ,INT8U *buf, struct Tsa_Node *desnode, DATA_ITEM  tmpitem)
{
	time_t getTheTime;
	DateTimeBCD timebcd;
	INT8U item07[4]={0,0,0,0};
	int i=0,sendlen = 0;

	getTheTime = taskinfo.task_list[taski].beginTime ;
	timebcd =   timet_bcd(getTheTime);
	timebcd.hour.data = 0;
	timebcd.min.data = 0;
	sendlen = createMeterFrame(desnode, tmpitem, buf, item07);
	taskinfo.task_list[taski].fangan.items[itemi].item07[0] = item07[0];
	taskinfo.task_list[taski].fangan.items[itemi].item07[1] = item07[1];
	taskinfo.task_list[taski].fangan.items[itemi].item07[2] = item07[2];
	taskinfo.task_list[taski].fangan.items[itemi].item07[3] = item07[3];
	taskinfo.task_list[taski].fangan.items[itemi].savetime = timebcd;

	taskinfo.now_taski = taski;
	taskinfo.now_itemi = itemi;
	taskinfo.task_list[taski].fangan.items[itemi].sucessflg = 1;
	taskinfo.task_list[taski].fangan.item_i = itemi;
	PrintTaskInfo(&taskinfo,taski);

//	if (itemi == taskinfo.task_list[taski].fangan.item_n-1)//最后一个
//	{
//		DbgPrintToFile1(31,"重新初始化 任务%d 开始时间",taskinfo.task_list[taski].taskId);
//		task_Refresh(&taskinfo.task_list[taski]);
//	}
	return sendlen;
}
int do_5002_type( int taski, int itemi ,INT8U *buf, struct Tsa_Node *desnode, DATA_ITEM  tmpitem)
{
	time_t getTheTime;
	DateTimeBCD timebcd;
	INT8U item07[4]={0,0,0,0};
	int i=0,sendlen = 0;

	switch(taskinfo.task_list[taski].fangan.cjtype)
	{
		case 3://按时间间隔抄表
			getTheTime = taskinfo.task_list[taski].beginTime - (taskinfo.task_list[taski].fangan.ti.interval * 60);
			timebcd =   timet_bcd(getTheTime);
			break;
		default:
			getTheTime = taskinfo.task_list[taski].beginTime ;
			timebcd =   timet_bcd(getTheTime);
			break;
	}
	DbgPrintToFile1(31,"begin=%ld   get=%ld   曲线时标%d-%d-%d %d:%d ",taskinfo.task_list[taski].beginTime ,getTheTime,timebcd.year.data,timebcd.month.data,timebcd.day.data,timebcd.hour.data,timebcd.min.data);
	sendlen = createMeterFrame_Curve(desnode, tmpitem, buf, item07, timebcd);
	for(i=0;i<taskinfo.task_list[taski].fangan.item_n;i++)
	{
		taskinfo.task_list[taski].fangan.items[i].sucessflg = 1;
		taskinfo.task_list[taski].fangan.items[i].item07[0] = item07[0];
		taskinfo.task_list[taski].fangan.items[i].item07[1] = item07[1];
		taskinfo.task_list[taski].fangan.items[i].item07[2] = item07[2];
		taskinfo.task_list[taski].fangan.items[i].item07[3] = item07[3];
		taskinfo.task_list[taski].fangan.items[i].savetime = timebcd;
	}
	taskinfo.now_taski = taski;
	taskinfo.now_itemi = itemi;
	PrintTaskInfo(&taskinfo,taski);
//	DbgPrintToFile1(31,"重新初始化 任务%d 开始时间",taskinfo.task_list[taski].taskId);
	task_Refresh(&taskinfo.task_list[taski]);

	return sendlen;
}
int do_other_type( int taski, int itemi ,INT8U *buf, struct Tsa_Node *desnode, DATA_ITEM  tmpitem)
{
	time_t getTheTime;
	DateTimeBCD timebcd;
	INT8U item07[4]={0,0,0,0};
	int i=0,sendlen = 0;

	DEBUG_TIME_LINE("createMeterFrame start");
	sendlen = createMeterFrame(desnode, tmpitem, buf, item07);
	DEBUG_TIME_LINE("createMeterFrame end");

	DEBUG_TIME_LINE("PrintTaskInfo2 start");
	taskinfo.task_list[taski].fangan.items[itemi].item07[0] = item07[0];
	taskinfo.task_list[taski].fangan.items[itemi].item07[1] = item07[1];
	taskinfo.task_list[taski].fangan.items[itemi].item07[2] = item07[2];
	taskinfo.task_list[taski].fangan.items[itemi].item07[3] = item07[3];
	taskinfo.task_list[taski].fangan.items[itemi].savetime = taskinfo.task_list[taski].begin;
	taskinfo.now_taski = taski;
	taskinfo.now_itemi = itemi;
	taskinfo.task_list[taski].fangan.items[itemi].sucessflg = 1;
	taskinfo.task_list[taski].fangan.item_i = itemi;
	PrintTaskInfo2(&taskinfo);
	DEBUG_TIME_LINE("itemi: %d, total: %d", itemi, \
			taskinfo.task_list[taski].fangan.item_n-1);
//	if (itemi == taskinfo.task_list[taski].fangan.item_n-1)//最后一个
//	{
//		DEBUG_TIME_LINE("");
//		DbgPrintToFile1(31,"重新初始化 任务%d 开始时间",taskinfo.task_list[taski].taskId);
//		task_Refresh(&taskinfo.task_list[taski]);
//	}

	return sendlen;
}


/*
 * 假如载波模块请求的测量点TSA与
 * 内存中的TSA不同, 则需要更新数据
 * 项的抄读状态.
 * 如果当前TSA的某个任务的所有数据
 * 的状态全不为0, 则表示这个任务的
 * 所有数据项都被操作完毕, 除日冻结
 * 任务需要补抄外, 其他任务暂时不做
 * 补抄处理, 更新下一次任务执行时间,
 * 并将各抄读项状态置0.
 */
void chkTsaTask(TASK_INFO *meterinfo)
{
	int opt5004Count=0;//日冻结已抄读数据项数目
	int	needRetry=0;//日冻结需要补抄标志

	int flgZeroCnt = 0;//除日冻结任务的其他任务, 抄读状态为0的数据项数目

	int tasknum = meterinfo->task_n;
	int i=0,j=0,flg5004Task=-1;
	time_t nowt = time(NULL);
	for(i=0;i<tasknum;i++)
	{
		 opt5004Count = 0;
		 needRetry = 0;
		 flg5004Task = 0;
		 flgZeroCnt = 0;

		if ( nowt >= meterinfo->task_list[i].beginTime)//此任务时间已经到了
		{
			 for(j = 0; j<meterinfo->task_list[i].fangan.item_n; j++)
			 {
				 if (meterinfo->task_list[i].fangan.items[j].oad1.OI == 0x5004)
				 {//如果是日冻结任务, 检查已抄读的数据项数目

					 flg5004Task = i;
					 switch (meterinfo->task_list[i].fangan.items[j].sucessflg) {
					 case 2:
						 opt5004Count++;
						 break;
					 case 1://有未抄读成功项
						 opt5004Count++;
						 needRetry = 1;//存在抄读过但未成功数据
						 meterinfo->task_list[i].fangan.items[j].sucessflg = 0;
						 break;
					 case 0:
						 break;
					 default:
						 break;
					 }
				 }
				 else
				 {//如果不是日冻结任务, 则检查未操作的数据项数目, 如果没有未操作数据项,
				  //则清零数据项的状态标志, 更新下一次执行时间
					 switch (meterinfo->task_list[i].fangan.items[j].sucessflg) {
					 case 0:
						 flgZeroCnt++;
						 break;
					 default:
						 break;
					 }
				 }
			 }

			 if (opt5004Count == meterinfo->task_list[i].fangan.item_n && needRetry==1)
			 {//日冻结的全部数据项都抄过，但是存在抄读失败的数据
				 meterinfo->task_list[i].tryAgain = 0x55;
				 DbgPrintToFile1(31,"TSA:%02x%02x%02x%02x%02x%02x%02x%02x 此日冻结任务 %d 需要补抄",
						 meterinfo->tsa.addr[0],meterinfo->tsa.addr[1],
						 meterinfo->tsa.addr[2],meterinfo->tsa.addr[3],
						 meterinfo->tsa.addr[4],meterinfo->tsa.addr[5],
						 meterinfo->tsa.addr[6],meterinfo->tsa.addr[7],
						 meterinfo->task_list[i].taskId);
			 }

			 if(flgZeroCnt == 0 && flg5004Task==0) {//flg5004Task==0, 说明当前任务不是日冻结任务
				task_Refresh(&taskinfo.task_list[i]);
				for(j = 0; j<meterinfo->task_list[i].fangan.item_n; j++) {
					meterinfo->task_list[i].fangan.items[j].sucessflg = 0;
				}
			 }
		}//end if 此任务时间已经到了
	}
}


int ProcessMeter(INT8U *buf,struct Tsa_Node *desnode)
{	DATA_ITEM  tmpitem;
	int sendlen=0,taski=0, itemi=0;//返回 tmpitem指示的具体任务索引 ，itemi指示的具体数据项索引

	//记录单元信息是不是这次要抄的电表的  Y：继续，N：读取本表记录信息到内存，继续
	DbgPrintToFile1(31,"内存   【%02x-%02x-%02x%02x%02x%02x%02x%02x】 index=%d",
			taskinfo.tsa.addr[0],taskinfo.tsa.addr[1],taskinfo.tsa.addr[2],
			taskinfo.tsa.addr[3],taskinfo.tsa.addr[4],taskinfo.tsa.addr[5],
			taskinfo.tsa.addr[6],taskinfo.tsa.addr[7],taskinfo.tsa_index);

	if (memcmp(taskinfo.tsa.addr,desnode->tsa.addr,TSA_LEN)!=0 )//内存的TSA 和请求的TSA 比对失败
	{
		if (ifTsaValid(taskinfo.tsa)==1)//判断为有效TSA
		{
			//相关信息存储前，日冻结任务特殊判断，
			//1、日冻结任务全部数据项都抄成功需要更新任务下次执行时间，将成功标识置 0
			//2、部分未成功，标识置 0 ，下次执行时间不变
			//再次请求该表时，如果存在部分未成功数据时，任务可以再次执行，并重新抄读标识为 0的
			chkTsaTask(&taskinfo);
			saveParaClass(0x8888, &taskinfo,taskinfo.tsa_index);
		}

		int ret = readParaClass(0x8888, &taskinfo, desnode->tsa_index) ;//读取序号为 tsa_index 的任务记录到内存变量 taskinfo
//		DbgPrintToFile1(31,"读序号=%d TSA",desnode->tsa_index);
		if (ret != 1 )// 返回 1 成功   0 失败
		{
			taskinfo.tsa = desnode->tsa;
			taskinfo.tsa_index = desnode->tsa_index;
			zeroitemflag(&taskinfo);
//			DbgPrintToFile1(31,"读失败");
		}else
		{
//			DbgPrintToFile1(31,"读成功  %02x-%02x-%02x%02x%02x%02x%02x%02x  index=%d",
//						taskinfo.tsa.addr[0],taskinfo.tsa.addr[1],taskinfo.tsa.addr[2],
//						taskinfo.tsa.addr[3],taskinfo.tsa.addr[4],taskinfo.tsa.addr[5],
//						taskinfo.tsa.addr[6],taskinfo.tsa.addr[7],taskinfo.tsa_index);
		}
	}
	tmpitem = checkMeterData(&taskinfo,&taski,&itemi,desnode->usrtype);	//根据任务的时间计划，查找一个适合抄读的数据项
	DEBUG_TIME_LINE("tmpitem.oad1.OI %04X, tmpitem.oad2.OI  %04X", tmpitem.oad1.OI, tmpitem.oad2.OI);
	if (tmpitem.oad1.OI !=0 || tmpitem.oad2.OI !=0 )
	{	//组织抄读报文
		if (tmpitem.oad1.OI == 0x5002)
		{
//			sendlen = do_5002_type( taski, itemi , buf, desnode, tmpitem);//负荷记录

			DEBUG_TIME_LINE("0x5002 start");
			tmpitem.oad1.OI = 0;
			DEBUG_TIME_LINE("tmpitem.oad1.OI %04X, tmpitem.oad2.OI  %04X", tmpitem.oad1.OI, tmpitem.oad2.OI);
			sendlen = do_other_type( taski, itemi , buf, desnode, tmpitem);//其它数据
			DEBUG_TIME_LINE("0x5002 end");

		}else if (tmpitem.oad1.OI == 0x5004)
		{
			DEBUG_TIME_LINE("");
			sendlen = do_5004_type( taski, itemi , buf, desnode, tmpitem);//日冻结
		}
		else
		{
			sendlen = do_other_type( taski, itemi , buf, desnode, tmpitem);//其它数据
			DEBUG_TIME_LINE("");
		}

	}else
	{
		//更新6035抄读成功数量
		CLASS_6035 result6035;	//采集任务监控单元
		get6035ByTaskID(taskinfo.task_list[taski].taskId,&result6035);
		result6035.successMSNum = getTaskDataTsaNum(result6035.taskID);
		saveClass6035(&result6035);

		DbgPrintToFile1(31,"切表");
		sendlen = 1;
		DEBUG_TIME_LINE("");
	}
	return sendlen;
}

int buildMeterFrame(INT8U *buf,struct Tsa_Node *desnode,CJ_FANGAN fangAn)
{
	INT8U type = 0;
	FORMAT07 Data07;
	int sendlen = 0 ,itemindex = -1 ,readcounter = 0;

	if (desnode == NULL)
		return 0;

	readcounter = desnode->readnum;
	type = desnode->protocol;
	itemindex = findFirstZeroFlg(desnode, fangAn.item_n);//fangAn.item_n);
	memset(buf,0,BUFSIZE645);
	if (itemindex >= 0 && readcounter < 2)//全部数据最多抄读2次
	{
		DbgPrintToFile1(31,"当前抄读第 %d 数据项 curr_i指到 %d 【OAD1 %04x-%02x %02x    OAD2 %04x-%02x %02x】第 %d 次抄读",itemindex,desnode->curr_i,
				fangAn.items[itemindex].oad1.OI,fangAn.items[itemindex].oad1.attflg,fangAn.items[itemindex].oad1.attrindex,
				fangAn.items[itemindex].oad2.OI,fangAn.items[itemindex].oad2.attflg,fangAn.items[itemindex].oad2.attrindex,desnode->readnum+1);
		switch (type)
		{
			case DLT645_07:
				Format07(&Data07,fangAn.items[itemindex].oad1,fangAn.items[itemindex].oad2,desnode->tsa);
				sendlen = composeProtocol07(&Data07, buf);
				if (sendlen>0)
				{
//					DbPrt1(31,"645:", (char *) buf, sendlen, NULL);
					return sendlen;
				}
				break;
			case DLT698:
				return 20;
		}
	}
	return 0;
}
void addTimeLable(TASK_INFO *tskinfo,int taski,int itemi)
{
	INT8U index = tskinfo->task_list[taski].fangan.item_i;//当前抄到方案数据项第几数据项目
	if (index == 0)
	{
		//保存开始时间
	}else if (index == tskinfo->task_list[taski].fangan.item_n)
	{
		//保存完成时间
	}
}
void saveTaskData_NormalData(TASK_INFO *tskinfo,FORMAT07 *frame07,TS ts)
{
	INT8U dataContent[50]={},alldata[100]={};
	int ti=0,ii=0,len698=0;
	ti = tskinfo->now_taski;
	ii = tskinfo->now_itemi;

	DbgPrintToFile1(31,"收到普通数据回码");

	len698 = data07Tobuff698(*frame07,dataContent);
	if(len698 > 0)
	{
		alldata[0] = 0x55;
		memcpy(&alldata[1],taskinfo.tsa.addr,17);
		memcpy(&alldata[18],dataContent,len698);
		len698 = len698 + 18;
		DbPrt1(31,"存储:", (char *) alldata, len698, NULL);
		SaveOADData(tskinfo->task_list[ti].taskId,tskinfo->task_list[ti].fangan.items[ii].oad1,tskinfo->task_list[ti].fangan.items[ii].oad2,alldata,len698,ts);
	}

}
void saveTaskData_MeterCurve(TASK_INFO *tskinfo,FORMAT07 *frame07,TS ts)
{
	int i=0,ti=0,ii=0,n=0,len698=0;
//	C601F_645 obj601F_07Flag;
//	obj601F_07Flag.protocol = 2;
	MeterCurveDataType result;
	INT8U dataContent[50]={};
	INT8U alldata[100]={};
	FORMAT07 myframe07;
	INT8U errCode[2] = {0xE0, 0xE0};//错误的起始码

	DbPrt1(31,"curve:", (char *) frame07->Data, frame07->Length, NULL);
	if (memcmp(&frame07->Data[0], errCode, 2) == 0)
		return;

	DbgPrintToFile1(31,"收到负荷曲线回码");
	ti = tskinfo->now_taski;
	ii = tskinfo->now_itemi;
	for(n=0;n<tskinfo->task_list[ti].fangan.item_n; n++)
	{
		if(tskinfo->task_list[ti].fangan.items[n].oad1.OI == 0x5002)
		{
			for(i=0;i<CURVENUM;i++)
			{
				if (tskinfo->task_list[ti].fangan.items[n].oad2.OI == meterCurveData[i].oad2.OI )
				{
					memcpy(&result, &meterCurveData[i], sizeof(MeterCurveDataType));
					memset(&myframe07,0,sizeof(myframe07));
					myframe07.DI[0] = result.Flg07[3];
					myframe07.DI[1] = result.Flg07[2];
					myframe07.DI[2] = result.Flg07[1];
					myframe07.DI[3] = result.Flg07[0];

					myframe07.Length = 4 + result.dataLen;//数据长度+4
					memcpy(myframe07.Data,&frame07->Data[result.startIndex],result.dataLen);
					DbgPrintToFile1(31,"i =  %d   frame07  %02x %02x %02x %02x ",i,myframe07.DI[0],myframe07.DI[1],myframe07.DI[2],myframe07.DI[3]);
					len698 = data07Tobuff698(myframe07,dataContent);
					if(len698 > 0)
					{
						alldata[0] = 0x55;
						memcpy(&alldata[1],tskinfo->tsa.addr,17);
						memcpy(&alldata[18],dataContent,len698);
						len698 = len698 + 18;
						DbPrt1(31,"存储:", (char *) alldata, len698, NULL);
						SaveOADData(tskinfo->task_list[ti].taskId,
								tskinfo->task_list[ti].fangan.items[n].oad1,
								taskinfo.task_list[ti].fangan.items[n].oad2,
								alldata,len698,
								ts);
						sleep(1);
					}

				}
			}
		}
	}
	return ;
}
int SaveTaskData(FORMAT3762 format_3762_Up,INT8U taskid)
{
	int len645=0, taski=0 ,itemi=0 ;
	INT8U nextFlag=0, dataContent[50]={}, buf645[255]={} ,taskFlag[4]={0,0,0,0} ,CurveFlg[4]={0x01, 0x00, 0x00, 0x06};
	TSA tsatmp;
	FORMAT07 frame07;
	TS ts;

	memset(dataContent,0,sizeof(dataContent));
	if (format_3762_Up.afn06_f2_up.MsgLength > 0)
	{
		len645 = format_3762_Up.afn06_f2_up.MsgLength;
		memcpy(buf645, format_3762_Up.afn06_f2_up.MsgContent, len645);
		if (analyzeProtocol07(&frame07, buf645, len645, &nextFlag) == 0)
		{
			Addr_TSA(frame07.Addr,&tsatmp);
			DbgPrintToFile1(31,"内存- %02x%02x%02x%02x%02x%02x%02x%02x%02x  index=%d",
					taskinfo.tsa.addr[0],taskinfo.tsa.addr[1],taskinfo.tsa.addr[2],
					taskinfo.tsa.addr[3],taskinfo.tsa.addr[4],taskinfo.tsa.addr[5],
					taskinfo.tsa.addr[6],taskinfo.tsa.addr[7],taskinfo.tsa.addr[8],taskinfo.tsa_index);
			DbgPrintToFile1(31,"上数- %02x%02x%02x%02x%02x%02x%02x%02x%02x",
					tsatmp.addr[0],tsatmp.addr[1],tsatmp.addr[2],
					tsatmp.addr[3],tsatmp.addr[4],tsatmp.addr[5],
					tsatmp.addr[6],tsatmp.addr[7],tsatmp.addr[8]);
			if (memcmp(taskinfo.tsa.addr,tsatmp.addr,8) == 0 )
			{//是当前抄读TSA 数据
				taski = taskinfo.now_taski;
				itemi = taskinfo.now_itemi;
//				TimeBCDToTs(taskinfo.task_list[taski].begin,&ts);
				TimeBCDToTs(taskinfo.task_list[taski].fangan.items[itemi].savetime,&ts);//ts 为数据存储时间

				memcpy(taskFlag,taskinfo.task_list[taski].fangan.items[itemi].item07,4);
				DbgPrintToFile1(31,"抄读数据项 %02x%02x%02x%02x",taskFlag[0],taskFlag[1],taskFlag[2],taskFlag[3]);
				DbgPrintToFile1(31,"回码数据项 %02x%02x%02x%02x",frame07.DI[0],frame07.DI[1],frame07.DI[2],frame07.DI[3]);
				DbgPrintToFile1(31,"存储时间 %d -%d -%d  %d:%d:%d",ts.Year,ts.Month,ts.Day,ts.Hour,ts.Minute,ts.Sec);
				if (memcmp(taskFlag,frame07.DI,4) == 0) //抄读项 与 回码数据项相同
				{
					taskinfo.task_list[taski].fangan.items[itemi].sucessflg = 2;
					if(memcmp(CurveFlg,taskFlag,4) == 0)//负荷曲线数据项
					{
						saveTaskData_MeterCurve(&taskinfo,&frame07,ts);
					}else								//其它数据项
					{
						saveTaskData_NormalData(&taskinfo,&frame07,ts);
					}
				}
			}
		}
	}
//	{
//		nodetmp = getNodeByTSA(tsa_head,tsatmp);
//		readParaClass(0x9999, &tmpmter, nodetmp->tsa_index);// 返回 0 成功   1 失败
//	}
	return 1;
}

int saveSerchMeter(FORMAT3762 format_3762_Up)
{
	if (format_3762_Up.afn06_f4_up.DeviceType == 1)
	{
		DbgPrintToFile1(31,"\n搜到设备-电表");
	}
	else
	{
		DbgPrintToFile1(31,"\n搜到设备-采集器");
	}
	return 1;
}

int checkAllData(struct Tsa_Node* head,struct Tsa_Node desnode)
{
	return 1;
}
int doTask(RUNTIME_PLC *runtime_p)
{
	static int step_cj = 0, beginwork=0;
	static int inWaitFlag = 0;
	struct Tsa_Node *nodetmp;

	TSA tsatmp;
	int sendlen=0, flag=0;

	time_t nowtime = time(NULL);
	if (runtime_p->redo == 1)
	{
		fprintf(stderr,"\n--------redo 重启抄表");
		step_cj = 0;
	}
	else if (runtime_p->redo == 2)
	{
		fprintf(stderr,"\n--------redo 恢复抄表");
		step_cj = 1;
	}

	switch( step_cj )
	{
		case 0://重启抄表
			if ( nowtime - runtime_p->send_start_time > 20)
			{
				DEBUG_TIME_LINE("\n--------do 重启抄表");
				DbgPrintToFile1(31,"\n重启抄表");
				clearvar(runtime_p);
				runtime_p->redo = 0;
				runtime_p->send_start_time = nowtime ;
				sendlen = AFN12_F1(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
			}else if(runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1)
			{//确认
				DEBUG_TIME_LINE("\n-------- 确认--------");
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime ;
				step_cj = 2;
				beginwork = 0;
			}
			break;
		case 1://恢复抄读
			if ( nowtime - runtime_p->send_start_time > 20)
			{
				DEBUG_TIME_LINE("\n--------do 恢复抄表");
				DbgPrintToFile1(31,"\n恢复抄表");
				runtime_p->redo = 0;
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime ;
				sendlen = AFN12_F3(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
			}else if(runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1)
			{//确认
				DEBUG_TIME_LINE("\n--------恢复抄表确认");
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime ;
				step_cj = 2;
				beginwork = 0;
			}
			break;
		case 2://开始抄表
			if (runtime_p->format_Up.afn == 0x14 && runtime_p->format_Up.fn == 1  && inWaitFlag ==0)//收到请求抄读命令
			{
				inWaitFlag = 1;
				beginwork = 1;//收到第一个请求抄读开始
				Addr_TSA(runtime_p->format_Up.afn14_f1_up.SlavePointAddr,&tsatmp);
				DbgPrintToFile1(31,"\n 请求地址 [ %02x-%02x-%02x%02x%02x%02x%02x%02x ] ",\
						tsatmp.addr[0],tsatmp.addr[1],tsatmp.addr[2],tsatmp.addr[3],\
						tsatmp.addr[4],tsatmp.addr[5],tsatmp.addr[6],tsatmp.addr[7]);//TODO 抄表组织报文

				DEBUG_TIME_LINE("请求地址 [ %02x-%02x-%02x %02x %02x %02x %02x %02x ] ",\
						tsatmp.addr[0],tsatmp.addr[1],tsatmp.addr[2],tsatmp.addr[3],\
						tsatmp.addr[4],tsatmp.addr[5],tsatmp.addr[6],tsatmp.addr[7]);

				sendlen = 0;
				nodetmp = NULL;
				nodetmp = getNodeByTSA(tsa_head,tsatmp);
				DEBUG_TIME_LINE("nodetmp: %p", nodetmp);
				if( nodetmp != NULL )
				{
					sendlen = ProcessMeter(buf645,nodetmp);
					//6035发送报文数量+1
					CLASS_6035 result6035;	//采集任务监控单元
					get6035ByTaskID(taskinfo.task_list[taskinfo.now_taski].taskId,&result6035);
					result6035.sendMsgNum++;
					saveClass6035(&result6035);
				}
				DbgPrintToFile1(31,"up channel = %02x",runtime_p->format_Up.info_up.ChannelFlag);
				flag= Echo_Frame( runtime_p,buf645,sendlen);//内部根据sendlen判断抄表 / 切表
				if (flag==0 || flag == 1)
					inWaitFlag = 0;
				runtime_p->send_start_time = nowtime;
			}else if ( runtime_p->format_Up.afn == 0x06 && runtime_p->format_Up.fn == 2 )//收到返回抄表数据
			{
				DbgPrintToFile1(31,"收数据");
				DEBUG_TIME_LINE("\n--------收数据");
				inWaitFlag = 0;
				sendlen = AFN00_F01( &runtime_p->format_Down,runtime_p->sendbuf );//确认
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );

				DEBUG_TIME_LINE("SaveTaskData-------start------------>");
				SaveTaskData(runtime_p->format_Up, runtime_p->taskno);
				DEBUG_TIME_LINE("<------------SaveTaskData-------end");
				clearvar(runtime_p);
				DbgPrintToFile1(31,"afn=%02x  fn=%02x",runtime_p->format_Up.afn,runtime_p->format_Up.fn );
				runtime_p->send_start_time = nowtime;
				//6035发送报文数量+1
				CLASS_6035 result6035;	//采集任务监控单元
				get6035ByTaskID(taskinfo.task_list[taskinfo.now_taski].taskId,&result6035);
				result6035.rcvMsgNum++;
				saveClass6035(&result6035);

			}else if( (nowtime - runtime_p->send_start_time > 10)  && inWaitFlag== 1)//等待超时,忽略超时过程中的请求抄读
			{
				DEBUG_TIME_LINE("超时");
				DbgPrintToFile1(31,"超时");
				inWaitFlag = 0;
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime;
			}else if( nowtime - runtime_p->send_start_time > 90)
			{
				DbgPrintToFile1(31,"抄表过程，通讯超时,重启抄表");
				DEBUG_TIME_LINE("抄表过程，通讯超时,重启抄表");
				step_cj = 0;
			}
			break;
	}
	return TASK_PROCESS;
}

INT8U getTransCmdAddrProto(INT8U* cmdbuf, INT8U* addrtmp, INT8U* proto)
{
	if(NULL == cmdbuf || NULL == addrtmp || NULL == proto) {
		return 0;
	}

	memcpy(addrtmp, cmdbuf+1, 6);
	*proto = 2;//dlt645-07

	return 1;
}

//响应主站代理操作
INT8U doClientProxyRequest(RUNTIME_PLC *runtime_p, int* beginwork, int* step_cj){
	int sendlen=0;
	struct Tsa_Node *nodetmp = NULL;
	INT8U addrtmp[10] = {0};//645报文中的目标地址
	INT8U proto = 0;
	time_t nowtime = time(NULL);
	INT16U timeout = 20;

	//TODO: 将状态机置于switch外层才符合逻辑, 否则外部的变量对状态机影响太大

	switch(cjcommProxy_plc.strProxyList.proxytype) {
	case ProxyGetRequestList:
		break;
	case ProxyGetRequestRecord:
		break;
	case ProxySetRequestList:
		break;
	case ProxySetThenGetRequestList:
		break;
	case ProxyActionRequestList:
		break;
	case ProxyActionThenGetRequestList:
		break;
	case ProxyTransCommandRequest:
		timeout = (cjcommProxy_plc.strProxyList.transcmd.revtimeout > 0) ?  \
										cjcommProxy_plc.strProxyList.transcmd.revtimeout : 20;

		DEBUG_TIME_LINE("cjcommProxy_plc.strProxyList.transcmd.revtimeout: %d; timeout: %d",\
						 cjcommProxy_plc.strProxyList.transcmd.revtimeout, timeout);

		if (*beginwork==0 && cjcommProxy_plc.isInUse==1) {//发送点抄
			*beginwork = 1;
			clearvar(runtime_p);

			getTransCmdAddrProto(cjcommProxy_plc.strProxyList.transcmd.cmdbuf, addrtmp, &proto);
			memcpy(runtime_p->format_Down.addr.SourceAddr, runtime_p->masteraddr, 6);

			sendlen = AFN13_F1(&runtime_p->format_Down,runtime_p->sendbuf, addrtmp, 2, 0, \
					cjcommProxy_plc.strProxyList.transcmd.cmdbuf, cjcommProxy_plc.strProxyList.transcmd.cmdlen);
			SendDataToCom(runtime_p->comfd, runtime_p->sendbuf, sendlen );
			DEBUG_BUFF(runtime_p->sendbuf, sendlen);
			runtime_p->send_start_time = nowtime;
			DEBUG_TIME_LINE("");
		} else if ((runtime_p->format_Up.afn == 0x13 && runtime_p->format_Up.fn == 1 ))	{//收到应答数据，或超时10秒，
			DEBUG_TIME_LINE("");
			cjcommProxy_plc.isInUse = 0;
			*beginwork = 0;

			if(runtime_p->format_Up.afn13_f1_up.MsgLength > 0) {
				INT16U tIndex = 0;
				INT16U starttIndex = 0;
				for(tIndex = 0;tIndex < runtime_p->format_Up.afn13_f1_up.MsgLength;tIndex++) {//去掉前导符
					if(runtime_p->format_Up.afn13_f1_up.MsgContent[tIndex]!=0x68) {
						continue;
					} else {
						starttIndex = tIndex;
						break;
					}
				}

				INT8U datalen = runtime_p->format_Up.afn13_f1_up.MsgLength - starttIndex;

				OADtoBuff(cjcommProxy_plc.strProxyList.transcmd.oad,cjcommProxy_plc.strProxyList.data);
				cjcommProxy_plc.strProxyList.data[4] = 1;
				cjcommProxy_plc.strProxyList.data[5] = datalen;

				memcpy(&cjcommProxy_plc.strProxyList.data[6],\
						&runtime_p->format_Up.afn13_f1_up.MsgContent[starttIndex],\
						datalen);
				DEBUG_BUFF(runtime_p->format_Up.afn13_f1_up.MsgContent, datalen);
				cjcommProxy_plc.strProxyList.datalen = datalen + 6;
				mqs_send((INT8S *)PROXY_NET_MQ_NAME,1,TERMINALPROXY_RESPONSE,\
						(INT8U *)&cjcommProxy_plc.strProxyList,sizeof(PROXY_GETLIST));
			} else {
				OADtoBuff(cjcommProxy_plc.strProxyList.transcmd.oad,cjcommProxy_plc.strProxyList.data);
				cjcommProxy_plc.strProxyList.data[4] = 0;
				cjcommProxy_plc.strProxyList.datalen = 5;
				mqs_send((INT8S *)PROXY_NET_MQ_NAME,1,\
						ProxySetResponseList,\
						(INT8U *)&cjcommProxy_plc.strProxyList,\
						sizeof(PROXY_GETLIST));
			}

			memset(&runtime_p->format_Up, 0, sizeof(runtime_p->format_Up));
			proxyInUse.devUse.plcReady = 1;
			DbgPrintToFile1(31,"收到点抄数据");
		} else if (((nowtime - runtime_p->send_start_time) > timeout) \
				&& *beginwork==1) {//代理超时后, 放弃本次操作, 上报超时应答

			DbgPrintToFile1(31,"单次点抄超时");
			cjcommProxy_plc.isInUse = 0;

			OADtoBuff(cjcommProxy_plc.strProxyList.transcmd.oad,cjcommProxy_plc.strProxyList.data);
			cjcommProxy_plc.strProxyList.data[4] = 0;
			cjcommProxy_plc.strProxyList.datalen = 5;
			mqs_send((INT8S *)PROXY_NET_MQ_NAME,1,\
					ProxySetResponseList,\
					(INT8U *)&cjcommProxy_plc.strProxyList,\
					sizeof(PROXY_GETLIST));

			proxyInUse.devUse.plcReady = 1;
			clearvar(runtime_p);
			DEBUG_TIME_LINE("");
		} else if( nowtime - runtime_p->send_start_time > 100  ) {//最后一次代理操作后100秒, 才恢复抄读
			DbgPrintToFile1(31,"100秒超时");
			clearvar(runtime_p);
			*beginwork = 0;
			*step_cj = 3;
			clearvar(runtime_p);
			DEBUG_TIME_LINE("");
		}
		break;
	default:
		break;
	}

	return 1;
}

int doProxy(RUNTIME_PLC *runtime_p)
{
	static int step_cj = 0, beginwork=0;
	static struct Tsa_Node *currtsa;
	struct Tsa_Node *nodetmp;
	int sendlen=0;

	time_t nowtime = time(NULL);
	switch( step_cj )
	{
		case 0://暂停抄表
			if ( nowtime - runtime_p->send_start_time > 20)
			{
				DbgPrintToFile1(31,"暂停抄表");
				DEBUG_TIME_LINE("暂停抄表");
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime ;
				sendlen = AFN12_F2(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
			}else if(runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1)
			{//确认
				clearvar(runtime_p);
				DEBUG_TIME_LINE("暂停抄表已确认");
				if(cjcommProxy_plc.isInUse == 1) {
					DEBUG_TIME_LINE("进入主站代理");
					step_cj = 2;
				} else if (cjGuiProxy_plc.isInUse == 1) {
					DEBUG_TIME_LINE("进入液晶点抄");
					step_cj = 1;
				}

				beginwork = 0;
			}
			break;
		case 1://开始监控载波从节点
			if (beginwork==0  && cjGuiProxy_plc.strProxyMsg.port.OI == 0xf209 && cjGuiProxy_plc.isInUse==1 )
			{//发送点抄
				DbgPrintToFile1(31,"dealGuiRead 处理液晶点抄 :%02x%02x%02x%02x%02x%02x%02x%02x 波特率=%d protocol=%d 端口号=%04x%02x%02x 规约类型=%d 数据标识=%04x"
						,cjGuiProxy_plc.strProxyMsg.addr.addr[0],cjGuiProxy_plc.strProxyMsg.addr.addr[1],cjGuiProxy_plc.strProxyMsg.addr.addr[2],cjGuiProxy_plc.strProxyMsg.addr.addr[3]
						,cjGuiProxy_plc.strProxyMsg.addr.addr[4],cjGuiProxy_plc.strProxyMsg.addr.addr[5],cjGuiProxy_plc.strProxyMsg.addr.addr[6],cjGuiProxy_plc.strProxyMsg.addr.addr[7]
						,cjGuiProxy_plc.strProxyMsg.baud,cjGuiProxy_plc.strProxyMsg.protocol,cjGuiProxy_plc.strProxyMsg.port.OI,cjGuiProxy_plc.strProxyMsg.port.attflg,cjGuiProxy_plc.strProxyMsg.port.attrindex
						,cjGuiProxy_plc.strProxyMsg.protocol,cjGuiProxy_plc.strProxyMsg.oi);
				beginwork = 1;
				nodetmp = NULL;
				nodetmp = getNodeByTSA(tsa_head,cjGuiProxy_plc.strProxyMsg.addr) ;
				clearvar(runtime_p);
				if( nodetmp != NULL )
				{
					DbgPrintToFile1(31,"发送点抄报文");
					memcpy(runtime_p->format_Down.addr.SourceAddr,runtime_p->masteraddr,6);
					sendlen = buildProxyFrame(runtime_p,nodetmp,cjGuiProxy_plc.strProxyMsg);
					SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				}
				runtime_p->send_start_time = nowtime;
			}
			else if ((runtime_p->format_Up.afn == 0x13 && runtime_p->format_Up.fn == 1 ))
			{//收到应答数据，或超时10秒，
				cjGuiProxy_plc.isInUse = 0;
				beginwork = 0;
				saveProxyData(runtime_p->format_Up);
				memset(&runtime_p->format_Up,0,sizeof(runtime_p->format_Up));
				DbgPrintToFile1(31,"收到点抄数据");
			}else if ((nowtime - runtime_p->send_start_time > 20  ) && beginwork==1)
			{
				DbgPrintToFile1(31,"单次点抄超时");
				cjGuiProxy_plc.isInUse = 0;
				beginwork = 0;
			}
			else if( nowtime - runtime_p->send_start_time > 100  )
			{//100秒等待
				DbgPrintToFile1(31,"100秒超时");
				clearvar(runtime_p);
				beginwork = 0;
				step_cj = 3;
			}
			break;
		case 2://处理主站代理
			doClientProxyRequest(runtime_p, &beginwork, &step_cj);
			break;
		case 3://恢复抄表
			if (runtime_p->state_bak == TASK_PROCESS )
			{
				if ( nowtime - runtime_p->send_start_time > 20)
				{
					DbgPrintToFile1(31,"恢复抄表");
					clearvar(runtime_p);
					runtime_p->send_start_time = nowtime ;
					sendlen = AFN12_F3(&runtime_p->format_Down,runtime_p->sendbuf);
					SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
					memset(&runtime_p->format_Up,0,sizeof(runtime_p->format_Up));
				}else if(runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1)
				{
					clearvar(runtime_p);
					step_cj = 0;
					beginwork = 0;
					DbgPrintToFile1(31,"返回到状态%d",runtime_p->state_bak);
					return(runtime_p->state_bak);
				}
			}else
			{
				clearvar(runtime_p);
				step_cj = 0;
				beginwork = 0;
				DbgPrintToFile1(31,"返回到状态%d",runtime_p->state_bak);
				return(runtime_p->state_bak);
			}
			break;
	}
	return DATA_REAL;
}
int startSearch(FORMAT3762 *down,INT8U *sendbuf)
{
	FORMAT07 format07_Down;
	INT8U ModuleFlag=0, Ctrl=0x02,searchTime=10, sendLen645=0;
	format07_Down.Ctrl = 0x13;//启动搜表
	format07_Down.SearchTime[0] = searchTime%256;
	format07_Down.SearchTime[1] = searchTime/256;
	memset(buf645,0,BUFSIZE645);
	sendLen645 = composeProtocol07(&format07_Down, buf645);
	int len = AFN05_F3(down, ModuleFlag, Ctrl, buf645, sendLen645,sendbuf);
	return (len);
}
int doSerch(RUNTIME_PLC *runtime_p)
{
	static int step_cj = 0, beginwork=0;
	static struct Tsa_Node *currtsa;
	struct Tsa_Node *nodetmp;
	TSA tsatmp;
	int sendlen=0;

	time_t nowtime = time(NULL);
	if (runtime_p->nowts.Hour==23)
	{
		step_cj = 0;
		beginwork = 0;
		clearvar(runtime_p);
		DbgPrintToFile1(31,"23点结束搜表，返回状态 %d",runtime_p->state_bak);
	}

	switch( step_cj )
	{
		case 0://暂停抄读
			if ( nowtime - runtime_p->send_start_time > 20)
			{
				DbgPrintToFile1(31,"暂停抄表");
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime ;
				sendlen = AFN12_F2(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
			}else if(runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1)
			{//确认
				clearvar(runtime_p);
				step_cj = 1;
			}
			break;
		case 1://启动广播
			if ( nowtime - runtime_p->send_start_time > 20  && beginwork==0)
			{
				DbgPrintToFile1(31,"启动广播");
				clearvar(runtime_p);
				memcpy(runtime_p->format_Down.addr.SourceAddr,runtime_p->masteraddr,6);
				sendlen = startSearch(&runtime_p->format_Down,runtime_p->sendbuf);
				DbgPrintToFile1(31," sendlen=%d",sendlen);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				runtime_p->send_start_time = nowtime ;
			}else if (runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1 && beginwork==0)//收到确认
			{
				DbgPrintToFile1(31,"收到确认");
				runtime_p->send_start_time = nowtime ;
				DEBUG_TIME_LINE("\nruntime_p->send_start_time = %ld",runtime_p->send_start_time);
				beginwork = 1;
			}else if (beginwork == 1)
			{
				DEBUG_TIME_LINE("\nruntime_p->send_start_time = %ld   nowtime=%ld",runtime_p->send_start_time,nowtime);
				if (nowtime - runtime_p->send_start_time >120 )
				{
					DbgPrintToFile1(31,"等待到时间");
					clearvar(runtime_p);
					step_cj = 2;
				}else
				{
					if ((nowtime-runtime_p->send_start_time) % 10 == 0)
					{
						DbgPrintToFile1(31,"等待120秒... (%ld)",nowtime-runtime_p->send_start_time);
						sleep(1);
					}
				}
			}
			break;
		case 2://激活从节点注册
			if ( nowtime - runtime_p->send_start_time > 20)
			{
				DbgPrintToFile1(31,"激活从节点注册",nowtime);
				beginwork = 0;
				clearvar(runtime_p);
				sendlen = AFN11_F5(&runtime_p->format_Down,runtime_p->sendbuf, 20);//minute
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				runtime_p->send_start_time = nowtime ;
			}else if (runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1)//收到确认
			{
				DbgPrintToFile1(31,"激活确认，进入等待从节点主动注册...",nowtime);
				beginwork = 1;
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime ;
				step_cj = 3;
			}
			break;
		case 3://等待注册
			if ( nowtime - runtime_p->send_start_time < 5 *60)
			{
				if (runtime_p->format_Up.afn == 0x06 && runtime_p->format_Up.fn == 4)
				{
					saveSerchMeter(runtime_p->format_Up);
					sendlen = AFN00_F01( &runtime_p->format_Down,runtime_p->sendbuf );//确认
					SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				}
				if ((nowtime-runtime_p->send_start_time) % 100 == 0)
				{
					DbgPrintToFile1(31,"等待1200秒... (%ld)",nowtime-runtime_p->send_start_time);
					sleep(1);
				}
			}else
			{
				DbgPrintToFile1(31,"搜表结束");
				beginwork =0;
				step_cj = 0;
				clearvar(runtime_p);
				return(runtime_p->state_bak);
			}
			break;
	}
	return METER_SEARCH;
}

int dateJudge(TS *old ,TS *new)
{
	if(old->Day!=new->Day || old->Year!=new->Year || old->Month!=new->Month)
	{
		memcpy(old,new,sizeof(TS));
		return 1;
	}
	else
		return 0;
}
void dealData(int state,RUNTIME_PLC *runtime_p)
{
	int datalen = 0;
	RecvDataFromCom(runtime_p->comfd,runtime_p->recvbuf,&RecvHead);
	datalen = StateProcessZb(runtime_p->dealbuf,runtime_p->recvbuf);
	if (datalen>0)
	{
		tcflush(runtime_p->comfd,TCIOFLUSH);
		analyzeProtocol3762(&runtime_p->format_Up,runtime_p->dealbuf,datalen);
//		fprintf(stderr,"\nafn=%02x   fn=%d 返回",runtime_p->format_Up.afn ,runtime_p->format_Up.fn);
	}
	return;
}
void initlist(struct Tsa_Node *head)
{
	struct Tsa_Node *tmp ;
	while(head!=NULL)
	{
		tmp = head;
		head = head->next;
		tmp->curr_i = 0;
		tmp->readnum = 0;
		memset(tmp->flag ,0 ,8);
	}
	return;
}

int stateJuge(int nowdstate,INT8U* my6000_p,RUNTIME_PLC *runtime_p)
{
	int state = nowdstate;

	if ( dateJudge(&runtime_p->oldts,&runtime_p->nowts) == 1 || JProgramInfo->oi_changed.oi6000 != *my6000_p)
	{
		DbgPrintToFile1(31,"\n状态切换到初始化");
		runtime_p->initflag = 1;
		runtime_p->state_bak = runtime_p->state;
		state = DATE_CHANGE;
		runtime_p->state = state;
		runtime_p->redo = 1;  //初始化之后需要重启抄读
		*my6000_p = JProgramInfo->oi_changed.oi6000;
		return state;
	}

	if ((runtime_p->nowts.Hour==23 && runtime_p->nowts.Minute==59) || (runtime_p->nowts.Hour==0 && runtime_p->nowts.Minute==0))
		return state;  //23点59分--0点0分之间不进行任务判断（准备跨日初始化）

	if ((runtime_p->nowts.Hour==20 && runtime_p->nowts.Minute==0) &&
		 state!=METER_SEARCH && state!=DATE_CHANGE && state!=SLAVE_COMP  && state!=INIT_MASTERADDR )
	{
		DbgPrintToFile1(31,"\n20点启动搜表");
		runtime_p->state_bak = runtime_p->state;
		clearvar(runtime_p);
		runtime_p->redo = 2;  //搜表后需要恢复抄读
		return METER_SEARCH;
	}

	//-------------------------------------------------------------------------------------------------------------------------
	//if ( judgebit(cjguiProxy.isInUse ,2) && cjguiProxy.strProxyMsg.port.OI == 0xf209 &&
	//	 state!=DATE_CHANGE && state!=DATA_REAL)
	if (cjGuiProxy_plc.isInUse ==1 && cjGuiProxy_plc.strProxyMsg.port.OI == 0xf209 && state!=DATE_CHANGE && state!=DATA_REAL)
	{	//出现液晶点抄载波表标识，并且不在初始化和点抄状态
		DbgPrintToFile1(31,"\n载波收到点抄消息 需要处理 %04x ",cjguiProxy.strProxyMsg.port.OI);
		runtime_p->state_bak = runtime_p->state;
		runtime_p->state = DATA_REAL;
		clearvar(runtime_p);
		runtime_p->redo = 2;	//点抄后需要恢复抄读
		return DATA_REAL;
	}

	if (proxyInUse.devUse.plcNeed == 1 && \
			cjcommProxy_plc.isInUse ==1 && \
			state!=DATE_CHANGE && \
			state!=DATA_REAL)
	{	//出现代理标识，并且不在初始化和点抄状态
		DbgPrintToFile1(31,"\n载波收到点代理请求, plcNeed: %d, plcReady: %d",\
				proxyInUse.devUse.plcNeed, proxyInUse.devUse.plcReady);
		runtime_p->state_bak = runtime_p->state;
		runtime_p->state = DATA_REAL;
		clearvar(runtime_p);
		runtime_p->redo = 2;	//点抄后需要恢复抄读
		return DATA_REAL;
	}
	if (state == NONE_PROCE && taskinfo.task_n>0)
	{
		state = TASK_PROCESS;
		runtime_p->state = TASK_PROCESS;
	}

	return state;
}
void readplc_thread()
{
	INT8U my6000=0;
	int state = DATE_CHANGE;
	RUNTIME_PLC runtimevar;
	memset(&runtimevar,0,sizeof(RUNTIME_PLC));
	my6000 = JProgramInfo->oi_changed.oi6000 ;
	RecvHead = 0;
	RecvTail = 0;

	initTaskData(&taskinfo);


	system("rm /nand/para/plcrecord.par  /nand/para/plcrecord.bak");
	DbgPrintToFile1(31,"2-fangAn6015[%d].sernum = %d  fangAn6015[%d].mst.mstype = %d ",
			0,fangAn6015[0].sernum,0,fangAn6015[0].mst.mstype);
	PrintTaskInfo2(&taskinfo);
	DbgPrintToFile1(31,"载波线程开始");
	runtimevar.format_Down.info_down.ReplyBytes = 0x28;

	DbgPrintToFile1(31,"1-fangAn6015[%d].sernum = %d  fangAn6015[%d].mst.mstype = %d ",
			0,fangAn6015[0].sernum,0,fangAn6015[0].mst.mstype);

	while(1)
	{
		usleep(50000);
		/********************************
		 * 	   状态判断
		********************************/
		TSGet(&runtimevar.nowts);
		state = stateJuge(state, &my6000,&runtimevar);

		/********************************
		 * 	   状态流程处理
		********************************/
		switch(state)
		{
			case DATE_CHANGE :
				state = doInit(&runtimevar);					//初始化 		 （ 1、硬件复位 2、模块版本信息查询  ）
				break;
			case INIT_MASTERADDR:
				state = doSetMasterAddr(&runtimevar);			//设置主节点地址 ( 1、主节点地址设置  )
				break;
			case SLAVE_COMP :
				state = doCompSlaveMeter(&runtimevar);			//从节点比对    ( 1、测量点比对  )
				break;
			case DATA_REAL :
				state = doProxy(&runtimevar);					//代理		  ( 1、发送代理抄读报文 2、根据超时限定主动退出代理state  ->> oldstate)
				break;
			case METER_SEARCH :
				state = doSerch(&runtimevar);					//搜表		  ( 1、启动搜表 2、根据超时限定主动退出搜表state )
				break;
			case TASK_PROCESS :
				state = doTask(&runtimevar);					//按任务抄表	  ( 1、根据方案类型和编号号确定抄表报文  )
				break;
			default :
				runtimevar.state = NONE_PROCE;
				sleep(1);
				break;
		}
		runtimevar.state  = state;
		/********************************
		 * 	   接收报文，并处理
		********************************/
		dealData(state,&runtimevar);

	}
	freeList(tsa_head);
	freeList(tsa_zb_head);

	pthread_detach(pthread_self());
	pthread_exit(&thread_readplc);
}

void readplc_proccess()
{
	struct mq_attr attr_zb_task;
	mqd_zb_task = mmq_open((INT8S *)TASKID_plc_MQ_NAME,&attr_zb_task,O_RDONLY);

	pthread_attr_init(&readplc_attr_t);
	pthread_attr_setstacksize(&readplc_attr_t,2048*1024);
	pthread_attr_setdetachstate(&readplc_attr_t,PTHREAD_CREATE_DETACHED);
	while ((thread_readplc_id=pthread_create(&thread_readplc, &readplc_attr_t, (void*)readplc_thread, NULL)) != 0)
	{
		sleep(1);
	}
}
