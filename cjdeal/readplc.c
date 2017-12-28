/*
 * readplc.c
 *
 *  Created on: 2017-1-4
 *      Author: wzm
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <termios.h>
#include <sys/mman.h>
#include <sys/reboot.h>
#include <sys/time.h>
#include "lib3762.h"
#include "readplc.h"
#include "cjdeal.h"
#include "cjsave.h"
#include "dlt645.h"
#include "dlt698.h"
#include "dlt698def.h"

static OAD	OAD_PORT_ZB={0xF209,0x02,0x01};
extern ProgramInfo* JProgramInfo;
extern int SaveOADData(INT8U taskid,OAD oad_m,OAD oad_r,INT8U *databuf,int datalen,TS ts_res);
extern INT16U data07Tobuff698(FORMAT07 Data07,INT8U* dataContent);
extern INT16U data97Tobuff698(FORMAT97 Data97,INT8U* dataContent);
extern INT8U increase6035Value(INT8U taskID,INT8U type);
extern INT8S OADMap07DI(OI_698 roadOI,OAD sourceOAD, C601F_645* flag645);
extern void DbgPrintToFile1(INT8U comport,const char *format,...);
extern void DbPrt1(INT8U comport,char *prefix, char *buf, int len, char *suffix);
extern INT8U checkMeterType(MY_MS mst,INT8U usrType,TSA usrAddr);
extern INT16S composeProtocol698_GetRequest(INT8U* sendBuf, CLASS_6015 obj6015,TSA meterAddr);

//extern INT8U getSaveTime(DateTimeBCD* saveTime,INT8U cjType,INT8U saveTimeFlag,DATA_TYPE curvedata);
//extern INT16U compose6012Buff(DateTimeBCD startTime,DateTimeBCD saveTime,TSA meterAddr,INT16U dataLen,INT8U* dataContent, INT8U port485);
extern mqd_t mqd_zb_task;
extern CLASS_4204	broadcase4204;

extern GUI_PROXY cjGuiProxy_plc;
extern Proxy_Msg* p_Proxy_Msg_Data;//液晶给抄表发送代理处理结构体，指向由guictrl.c配置的全局变量
extern TASK_CFG list6013[TASK6012_CAIJI];
extern INT8U analyzeProtocol698(INT8U* Rcvbuf, INT8U* resultCount, INT16S recvLen,
		INT8U* apduDataStartIndex, INT16S* dataLen) ;
extern INT8U deal698RequestResponse(INT8U getResponseType,INT8U csdNum,INT8U* apdudata,OADDATA_SAVE* oadListContent,INT16U* apdudataLen);
extern INT8U parseSingleROADDataHead(INT8U* oadData,OADDATA_SAVE* oadListContent,INT8U* rcvCSDnum,INT8U* recordNum);
extern INT16U parseSingleROADDataBody(INT8U* oadData,OADDATA_SAVE* oadListContent,INT8U rcvCSDnum);
int task_Refresh(TASK_UNIT *taskunit);

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
int findFromPools_ByTsa(INT8U *addr )
{
	int i = 0;
	for(i=0;i<4;i++)
	{
		if (memcmp(addr,plcPools.pool[i].tsa.addr,8) == 0 )
		{
			return i;
		}
	}
	return -1;
}

int findFromPools(INT8U *addr ,INT8U *itemflag)
{
	int i = 0;
	for(i=0;i<4;i++)
	{
		if (memcmp(addr,plcPools.pool[i].tsa.addr,8) == 0 )
		{
			if (memcmp(itemflag,plcPools.pool[i].item.item07,4) == 0) //抄读项 与 回码数据项相同
			{
				return i;
			}
		}
	}
	return -1;
}
void myadd2Pools(int ti,int ii)
{
	int i=0;
	i = plcPools.point;

	plcPools.pool[i].task_i = ti;
	plcPools.pool[i].item_i = ii;
	plcPools.pool[i].taskID = taskinfo.task_list[ti].taskId;
	plcPools.pool[i].fangAn = taskinfo.task_list[ti].fangan.No;
	memcpy(&plcPools.pool[i].item,&taskinfo.task_list[ti].fangan.items[ii],sizeof(DATA_ITEM));
	memcpy(&plcPools.pool[i].tsa,&taskinfo.tsa,sizeof(TSA));

	plcPools.point = (plcPools.point + 1)%4;
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
void JugeLastTime_SetZero(TASK_INFO *tasklist)
{
	DateTimeBCD ts;
	time_t nowt = time(NULL);
	int jianGe_sec=0,zhouqi_sec;
	int i=0 , j=0,num=0,k=0;
	num = tasklist->task_n;

	for(i=0;i<num;i++)
	{
		zhouqi_sec = TItoSec(tasklist->task_list[i].ti);//一个周期的秒数
		jianGe_sec = abs(nowt - tasklist->task_list[i].beginTime);//当前时刻距离开始时刻的秒数
		k = jianGe_sec/zhouqi_sec ;
		if (k>0)//计算上一个任务开始时刻
		{
			DbgPrintToFile1(31,"任务%d 计算下一次开始时间，标示清零(周期秒%d   间隔总秒数%d  k=%d)",tasklist->task_list[i].taskId,zhouqi_sec,jianGe_sec,k);
			tasklist->task_list[i].beginTime += k * zhouqi_sec;
			ts =  timet_bcd(tasklist->task_list[i].beginTime );
			tasklist->task_list[i].begin = ts;
			for(j=0;j<tasklist->task_list[i].fangan.item_n;j++)
				tasklist->task_list[i].fangan.items[j].sucessflg = 0;
		}
	}
}
DateTimeBCD ChgSucessFlg(TASK_INFO *taskinfo_p,DATA_ITEM item,INT8U usrtype,INT8U protocol,INT8U sucessflg)
{
	DateTimeBCD timebcd;
	int i=0,j=0,tnum=0,fnum=0,fangAnIndex=0,needflg=0;
	tnum = taskinfo_p->task_n;
	memset(&timebcd,0,sizeof(timebcd));
	for(i=0;i<tnum;i++)
	{
		fangAnIndex = findFangAnIndex(taskinfo_p->task_list[i].fangan.No);//查被抄电表当前任务的采集方案编号，在6015中的索引
		if (fangAnIndex >=0 )
		{
			needflg = checkMeterType(fangAn6015[fangAnIndex].mst, usrtype ,taskinfo_p->tsa);//查被抄电表的用户类型 是否满足6015中的用户类型条件
		}
		if (needflg)
		{
			fnum = taskinfo_p->task_list[i].fangan.item_n;
			for(j=0; j<fnum; j++)
			{
				if ((protocol==DLT_645_97 && memcmp(taskinfo_p->task_list[i].fangan.items[j].item97,item.item97,2) == 0) ||
					(protocol==DLT_645_07 && memcmp(taskinfo_p->task_list[i].fangan.items[j].item07,item.item07,4) == 0))
				{
					taskinfo_p->task_list[i].fangan.items[j].sucessflg = sucessflg;
#ifdef CHECK5004RATE
					if((sucessflg==2)&&(taskinfo_p->task_list[i].fangan.items[j].oad1.OI==0x5004))
					{
						success5004Num++;
					}
#endif
					taskinfo_p->now_itemi = j;
					taskinfo_p->now_taski = i;
					timebcd = taskinfo_p->task_list[i].fangan.items[j].savetime;
					DbgPrintToFile(31,"更新-%02x%02x%02x%02x%02x%02x%02x%02x[ti=%d ii=%d] [%02x%02x%02x%02x ,  %04x-%02x%02x]",
							taskinfo_p->tsa.addr[0],taskinfo_p->tsa.addr[1],taskinfo_p->tsa.addr[2],taskinfo_p->tsa.addr[3],
							taskinfo_p->tsa.addr[4],taskinfo_p->tsa.addr[5],taskinfo_p->tsa.addr[6],taskinfo_p->tsa.addr[7],i,j,
							taskinfo_p->task_list[i].fangan.items[j].item07[0],taskinfo_p->task_list[i].fangan.items[j].item07[1],
							taskinfo_p->task_list[i].fangan.items[j].item07[2],taskinfo_p->task_list[i].fangan.items[j].item07[3],
							taskinfo_p->task_list[i].fangan.items[j].oad1.OI,taskinfo_p->task_list[i].fangan.items[j].oad1.attflg,taskinfo_p->task_list[i].fangan.items[j].oad1.attrindex,
							taskinfo_p->task_list[i].fangan.items[j].oad2.OI,taskinfo_p->task_list[i].fangan.items[j].oad2.attflg,taskinfo_p->task_list[i].fangan.items[j].oad2.attrindex);
//					JugeNexTime_SetZero(&taskinfo_p->task_list[i]);
				}
			}
		}
	}
	return timebcd;
}
int calcTime_t(time_t nowt,time_t time, int uplimit)
{
	if (abs(nowt  - time) > uplimit)
		return 1;
	return 0;
}
INT8U getProtocol698Flag(int Factory)
{
	INT8U protocol698Flg = DLT_698;
	switch(Factory)
	{
		case DX_factory:
			protocol698Flg = 0;
			break;
	}
	return protocol698Flg;
}
void setFactoryVar(INT8U *factory)
{
	if (module_info.ModuleInfo.VendorCode[1]=='T' && module_info.ModuleInfo.VendorCode[0]=='C')
	{
		*factory = DX_factory;
	}else
		*factory = OTHER_factory;
}
void SendDataToCom(int fd, INT8U *sendbuf, INT16U sendlen)
{
//	int i=0;
	ssize_t slen;
	slen = write(fd, sendbuf, sendlen);
	DbPrt1(31,"S:", (char *) sendbuf, slen, NULL);
//	fprintf(stderr,"\nsend(%d)",slen);
//	for(i=0;i<slen;i++)
//		fprintf(stderr," %02x",sendbuf[i]);
	if(getZone("GW")==0) {
		PacketBufToFile(0,"[ZB]S:",(char *) sendbuf, slen, NULL);
	}
}
int RecvDataFromCom(int fd,INT8U* buf,int* head)
{
	int len,i;
	INT8U TmpBuf[ZBBUFSIZE];
	memset(TmpBuf,0,ZBBUFSIZE);
	if (fd < 0 ) return 0;
	len = read(fd,TmpBuf,ZBBUFSIZE);
//	if (len>0)
//	{
//		fprintf(stderr,"\nrecv(%d): ",len);
//	}

	for(i=0;i<len;i++)
	{
		buf[*head]=TmpBuf[i];
//		fprintf(stderr,"%02x ",TmpBuf[i]);
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
				if(getZone("GW")==0) {
					PacketBufToFile(0,"[ZB]R:",(char *) str, DataLen, NULL);
				}
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
	for(i=0;i<TASK6012_CAIJI;i++)
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
//int task_dateFlgItem(TASK_UNIT *taskunit)
//{
//	DateTimeBCD beginBCD;
//	beginBCD.year =
//	taskunit[0].taskId = 0;
//	taskunit[0].leve = 0;
//	taskunit[0].beginTime = calcnexttime(list6013[i].basicInfo.interval,list6013[i].basicInfo.startime,list6013[i].basicInfo.delay);//list6013[i].ts_next;
//	taskunit[0].endTime = tmtotime_t( DateBCD2Ts(list6013[i].basicInfo.endtime ));
//	ts =   timet_bcd(taskunit[0].beginTime);
//	taskunit[0].begin = ts;
//	taskunit[0].end = list6013[i].basicInfo.endtime;
//	type = list6013[i].basicInfo.cjtype;
//	serNo = list6013[i].basicInfo.sernum;//方案序号
//	memset(&taskunit[t].fangan,0,sizeof(CJ_FANGAN));
//	taskunit[0].fangan.type = type;
//	taskunit[0].fangan.No = serNo;
//	taskunit[0].ti = list6013[i].basicInfo.interval;
//	taskunit[0].fangan.item_n = Array_OAD_Items(&taskunit[t].fangan);
//
//	return 1;
//}
int task_leve(INT8U leve,TASK_UNIT *taskunit)
{
	DateTimeBCD ts;
	int i=0,t=0;
	INT8U type=0 ,serNo=0;
	TS tsNow;
	TSGet(&tsNow);

	for(i=0;i<TASK6012_CAIJI;i++)
	{
		if (list6013[i].basicInfo.cjtype==events)
			continue;
		if (list6013[i].basicInfo.runprio == leve && list6013[i].basicInfo.taskID>0)
		{
			taskunit[t].taskId = list6013[i].basicInfo.taskID;
			taskunit[t].leve = list6013[i].basicInfo.runprio;
			taskunit[t].beginTime = calcnexttime(list6013[i].basicInfo.interval,list6013[i].basicInfo.startime,list6013[i].basicInfo.delay);//list6013[i].ts_next;
#if 1//这部分代码是为了新安装的集中器能抄日冻结
			if((list6013[i].basicInfo.interval.units == day_units)&&(tsNow.Hour<22))
			{
					DbgPrintToFile1(31,"任务=%d 22点以前重启就强制抄读日冻结",taskunit[t].taskId);
					taskunit[t].beginTime -= 86400;
			}
#endif
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
	for(i=0;i<TASK6012_CAIJI;i++)
	{
		if (list6013[i].basicInfo.cjtype == norm)//普通采集任务
		{
			readCoverClass(0x6015, list6013[i].basicInfo.sernum, (void *)&fangAn6015p[j], sizeof(CLASS_6015), coll_para_save);
			DbgPrintToFile1(31,"fangAn6015p[%d].sernum = %d  fangAn6015p[%d].mst = %d ",
					j,fangAn6015p[j].sernum,j,fangAn6015p[j].mst.mstype);
			j++;
			if(j>=FANGAN6015_MAX)
				break;
		}
	}
}
int initSearchMeter(CLASS_6002 *class6002)
{
	if(readCoverClass(0x6002,0,class6002,sizeof(CLASS_6002),para_vari_save)==1)
	{
		fprintf(stderr,"搜表参数读取成功\n");
	}else {
		fprintf(stderr,"搜表参数文件不存在\n");
	}
	return 1;
}

int initTaskData(TASK_INFO *task)
{
	int num =0;
	if (task==NULL)
		return 0;
	memset(task,0,sizeof(TASK_INFO));
	memset(fangAn6015,0,sizeof(fangAn6015));
	memset(&plcPools,0,sizeof(plcPools));
	task_init6015(fangAn6015);
	/*初始化一个日冻结时标任务*/

	num += task_leve(0,&task->task_list[num]);
	num += task_leve(1,&task->task_list[num]);
	num += task_leve(2,&task->task_list[num]);
	num += task_leve(3,&task->task_list[num]);
	num += task_leve(4,&task->task_list[num]);
	task->task_n = num;
	memcpy(&taskinfo_bak,task,sizeof(TASK_INFO));//初始化一个备份
	return 1;
}
void init5004Num(int tsa_index,INT8U usrType,TSA usrAddr)
{
	static INT16S lastMeterusrtype = -1;
	static INT8U t5004count = 0;
	int taskIndex = 0,itemIndex = 0,tasknum = 0;
	INT8U tcount = 0,scount = 0,isHasRecord = 1;

	TASK_INFO taskinfoFile;
	if (readParaClass(0x8888, &taskinfoFile, tsa_index) != 1 )////读取序号为 tsa_index 的任务记录到内存变量 taskinfo 返回 1 成功   0 失败
	{
		memcpy(&taskinfoFile,&taskinfo_bak,sizeof(taskinfo));
		isHasRecord = 0;
	}

	if((lastMeterusrtype != usrType)||(isHasRecord==1))
	{
		lastMeterusrtype = usrType;
		tasknum = taskinfo_bak.task_n;
		for(taskIndex=0;taskIndex<tasknum;taskIndex++)
		{
			//判断是否需要抄读
			int fangAnIndex = findFangAnIndex(taskinfoFile.task_list[taskIndex].fangan.No);//查被抄电表当前任务的采集方案编号，在6015中的索引
			if (fangAnIndex >=0 )
			{
				INT8U needflg = checkMeterType(fangAn6015[fangAnIndex].mst, usrType ,usrAddr);//查被抄电表的用户类型 是否满足6015中的用户类型条件
				if(needflg == 1)
				{
					for(itemIndex = 0; itemIndex<taskinfoFile.task_list[taskIndex].fangan.item_n; itemIndex++)
					{
						if (taskinfoFile.task_list[taskIndex].fangan.items[itemIndex].oad1.OI == 0x5004)
						{
							tcount++;
							if((isHasRecord==1)&&(taskinfoFile.task_list[taskIndex].fangan.items[itemIndex].sucessflg==2))
							{
								scount++;
							}
						}
					}
				}

			}
		}
		t5004count = tcount;
	}
	success5004Num += scount;
	totoal5004Num += t5004count;
}

int initTsaList(struct Tsa_Node **head)
{
	success5004Num = 0;
	totoal5004Num = 0;;
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

#ifdef CHECK5004RATE
					init5004Num(tmp->tsa_index,tmp->usrtype,tmp->tsa);
#endif
				}
			}
		}
	}
	if (p!=NULL)
	{
		p->next = NULL;
	}

	asyslog(LOG_INFO,"totoal5004Num = %d success5004Num=%d",totoal5004Num,success5004Num);
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
	struct Tsa_Node *tmp = NULL;
	tmp = head;
	while(tmp!=NULL)
	{
		DbgPrintToFile1(31,"Protocol = %d  curr_i=%d   usertype=%02x  tsa: %02x%02x%02x%02x%02x%02x%02x%02x",tmp->protocol,tmp->curr_i,tmp->usrtype,
				tmp->tsa.addr[0],tmp->tsa.addr[1],tmp->tsa.addr[2],tmp->tsa.addr[3],tmp->tsa.addr[4],tmp->tsa.addr[5],tmp->tsa.addr[6],tmp->tsa.addr[7]);
		tmp = tmp->next;
	}
}
//TSA getNextTsa(struct Tsa_Node **p)
//{
//	TSA tsatmp;
//	memset(&tsatmp,0,sizeof(TSA));
//	if (*p!=NULL)
//	{
//		tsatmp = (*p)->tsa;
//		*p = (*p)->next;
//	}
//	return tsatmp;
//}
struct Tsa_Node* getNextTsa(struct Tsa_Node **p)
{
	struct Tsa_Node* node=NULL;

	if (*p!=NULL)
	{
		node = *p;
		*p = (*p)->next;
	}
	return node;
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
			if(p->protocol == desnode->protocol)
			{
				return 1;
			}
//			desnode->readnum = p->readnum;
//			desnode->protocol = p->protocol;
//			desnode->usrtype = p->usrtype;
//			return 1;
		}
//		else
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
//void addTsaList(struct Tsa_Node **head,INT8U *addr)
void addTsaList(struct Tsa_Node **head,SlavePointInformation *pointinfo)
{
	struct Tsa_Node *tmp = *head;
	struct Tsa_Node *new;
	tsa_zb_count = 0;
	if (*head==NULL)
	{
		*head = (struct Tsa_Node *)malloc(sizeof(struct Tsa_Node));
		(*head)->tsa.addr[0] = 7;
		(*head)->tsa.addr[1] = 5;
		(*head)->tsa.addr[2] = pointinfo->Addr[5];
		(*head)->tsa.addr[3] = pointinfo->Addr[4];
		(*head)->tsa.addr[4] = pointinfo->Addr[3];
		(*head)->tsa.addr[5] = pointinfo->Addr[2];
		(*head)->tsa.addr[6] = pointinfo->Addr[1];
		(*head)->tsa.addr[7] = pointinfo->Addr[0];
		(*head)->protocol =  pointinfo->Protocol;
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
		new->tsa.addr[2] = pointinfo->Addr[5];
		new->tsa.addr[3] = pointinfo->Addr[4];
		new->tsa.addr[4] = pointinfo->Addr[3];
		new->tsa.addr[5] = pointinfo->Addr[2];
		new->tsa.addr[6] = pointinfo->Addr[1];
		new->tsa.addr[7] = pointinfo->Addr[0];
		new->protocol = pointinfo->Protocol;
		new->next = NULL;
		tmp->next = new;
		tsa_zb_count++;
	}
	return;
}
void printModelinfo(AFN03_F10_UP info)
{
	DbgPrintToFile1(31,"硬件复位");
	DbgPrintToFile(31,"\n\n--------------------------------\n\n厂商信息:%c%c%c%c \nDate:%d-%d-%d \nVersion:%d%d",
			info.ModuleInfo.VendorCode[1],
			info.ModuleInfo.VendorCode[0],
			info.ModuleInfo.ChipCode[1],
			info.ModuleInfo.ChipCode[0],
			info.ModuleInfo.VersionYear,
			info.ModuleInfo.VersionMonth,
			info.ModuleInfo.VersionDay,
			info.ModuleInfo.Version[1],
			info.ModuleInfo.Version[0]);
	DbgPrintToFile(31,"\n主节点地址:%02x%02x%02x%02x%02x%02x",
			info.MasterPointAddr[5],
			info.MasterPointAddr[4],
			info.MasterPointAddr[3],
			info.MasterPointAddr[2],
			info.MasterPointAddr[1],
			info.MasterPointAddr[0]);
	DbgPrintToFile(31,"\nMonitorOverTime=%d 秒", info.MonitorOverTime);
	DbgPrintToFile(31,"\nReadMode=%02x\n--------------------------------\n\n", info.ReadMode);

}
void clearvar(RUNTIME_PLC *runtime_p)
{
	memset(runtime_p->recvbuf,0, ZBBUFSIZE);
	memset(runtime_p->dealbuf,0, ZBBUFSIZE);
	runtime_p->send_start_time = 0;
	memset(&runtime_p->format_Up,0,sizeof(runtime_p->format_Up));
}

void saveClassF209(AFN03_F1_UP module_info)
{
	CLASS_f209	f209={};
	int  year=0;

	readCoverClass(0xf209,0,&f209,sizeof(CLASS_f209),para_vari_save);
	memcpy(f209.para.devdesc,"3762",4);
	f209.para.devpara.baud = bps9600;
	f209.para.devpara.databits = d8;
	f209.para.devpara.verify = even;
	f209.para.devpara.stopbits = stop1;
	f209.para.version.factoryCode[1] = module_info.VendorCode[1];
	f209.para.version.factoryCode[0] = module_info.VendorCode[0];
	f209.para.version.chipCode[1] = module_info.ChipCode[1];
	f209.para.version.chipCode[0] = module_info.ChipCode[0];
	year = 2000 + module_info.VersionYear;
	f209.para.version.softDate[0] = (year >>8) & 0xff;
	f209.para.version.softDate[1] = year & 0xff;
	f209.para.version.softDate[2] = module_info.VersionMonth;
	f209.para.version.softDate[3] = module_info.VersionDay;
	f209.para.version.softDate[4] = 0;		//day_of_week
	f209.para.version.softVer = module_info.Version[1]*100 + module_info.Version[0];
	saveCoverClass(0xf209,0,&f209,sizeof(CLASS_f209),para_vari_save);
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
			DbgPrintToFile(31,"硬件复位...");
			freeList(tsa_head);
			freeList(tsa_zb_head);
			tsa_head = NULL;
			tsa_zb_head = NULL;
			tsa_count = initTsaList(&tsa_head);
//			tsa_print(tsa_head,tsa_count);

			if (runtime_p->comfd >0)
				CloseCom( runtime_p->comfd );
			runtime_p->comfd = OpenCom(5, 9600,(unsigned char*)"even",1,8);// 5 载波路由串口 ttyS5   SER_ZB   //呼唤性模拟测试接485-1 test  1
			DbgPrintToFile1(31,"comfd=%d",runtime_p->comfd);
			runtime_p->initflag = 0;
			clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
			reset_ZB();

			fprintf(stderr,"\n-----------tsacount=%d",tsa_count);
			if (tsa_count <= 0)
			{
				DbgPrintToFile1(31,"无载波测量点,路由参数初始化");
				step_init = 0;
//				sendlen = AFN01_F2(&runtime_p->format_Down,runtime_p->sendbuf);
//				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				sleep(5);
				return NONE_PROCE;
			}
			runtime_p->send_start_time = nowtime ;
			step_init = 1;
			read_num = 0;
			break;

		case 1://读取载波信息
			if ((abs(nowtime  - runtime_p->send_start_time) > 60) &&
				runtime_p->format_Up.afn != 0x03 && runtime_p->format_Up.fn!= 10)
			{
				DbgPrintToFile1(31,"读取载波信息");
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
				runtime_p->send_start_time = nowtime ;
				sendlen = AFN03_F10(&runtime_p->format_Down,runtime_p->sendbuf);//查询载波模块信息
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				read_num ++;
			}else if (runtime_p->format_Up.afn == 0x03 && runtime_p->format_Up.fn == 10)
			{//返回载波信息
				fprintf(stderr,"\n返回载波信息");
				memcpy(&module_info,&runtime_p->format_Up.afn03_f10_up,sizeof(module_info));
				DbgPrintToFile1(31,"SlavePointMode = %02x ",runtime_p->format_Up.afn03_f10_up.SlavePointMode);
				printModelinfo(module_info);
				setFactoryVar(&RouterFactory);	/*设置路由厂家标示变量*/
				step_init = 0;
				if(getZone("GW")==0) {	//国网送检模拟测试，将来可取消
					if(JProgramInfo->dev_info.PLC_ModeTest==1) {
						runtime_p->modeFlag = 1;
						DbgPrintToFile1(31,"测试修改集中器主导");
					}else  if(JProgramInfo->dev_info.PLC_ModeTest==2) {
						runtime_p->modeFlag = 0;
						DbgPrintToFile1(31,"测试修改路由主导");
					}else JProgramInfo->dev_info.PLC_ModeTest = 0;
				}else JProgramInfo->dev_info.PLC_ModeTest = 0;
				if(JProgramInfo->dev_info.PLC_ModeTest==0) {
					if (module_info.ReadMode ==1)
					{
						runtime_p->modeFlag = 1;
						DbgPrintToFile1(31,"集中器主导");
					}else
					{
						runtime_p->modeFlag = 0;
						DbgPrintToFile1(31,"路由主导");
					}
				}
				//存储F209载波／微功率无线接口的本地通信模块单元信息
				saveClassF209(module_info.ModuleInfo);
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零

				return ZB_MODE;
//				return INIT_MASTERADDR;
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
	  CLASS_4001_4002_4003 classtmp = {};
	static INT8U masteraddr[6] = {0x00,0x00,0x00,0x00,0x00,0x00};
	int sendlen=0, addrlen=0,i=0;
	static int step_MasterAddr = 0;
	time_t nowtime = time(NULL);
	switch(step_MasterAddr )
	{
		case 0://查询主节点地址
			if ((abs(nowtime  - runtime_p->send_start_time) > 20) )
			{
				sendlen = AFN03_F4(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				DbgPrintToFile1(31,"查询主节点地址");
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
				runtime_p->send_start_time = nowtime ;
			}else if(runtime_p->format_Up.afn == 0x03 && runtime_p->format_Up.fn == 4)
			{
				readCoverClass(0x4001, 0, &classtmp, sizeof(CLASS_4001_4002_4003), para_vari_save);
				memcpy(masteraddr,runtime_p->format_Up.afn03_f4_up.MasterPointAddr,6);
				DbgPrintToFile1(31,"载波模块主节点 ：%02x%02x%02x%02x%02x%02x ",masteraddr[0],masteraddr[1],masteraddr[2],masteraddr[3],masteraddr[4],masteraddr[5]);
				DbgPrintToFile1(31,"终端逻辑地址:   %02x%02x%02x%02x%02x%02x ",classtmp.curstom_num[1],classtmp.curstom_num[2],classtmp.curstom_num[3],
																			 classtmp.curstom_num[4],classtmp.curstom_num[5],classtmp.curstom_num[6]);
				addrlen = classtmp.curstom_num[0];
				if(addrlen > 6){
					addrlen = 6;
				}
				for(i=0;i<addrlen;i++)
				{
					if(masteraddr[i] != classtmp.curstom_num[i+1])
					{
						step_MasterAddr = 1;
						memcpy(masteraddr,&classtmp.curstom_num[1],6);
						DbgPrintToFile1(31,"需要设置主节点地址 : %02x%02x%02x%02x%02x%02x ",masteraddr[0],masteraddr[1],masteraddr[2],masteraddr[3],masteraddr[4],masteraddr[5]);
						break;
					}
				}
				memcpy(runtime_p->masteraddr,masteraddr,6);//不管需要还是不需要设置主节点，都要用正确的地址给runtime_p->masteraddr赋值
				if (step_MasterAddr == 0)
				{
					DbgPrintToFile1(31,"不需要设置主节点地址");
					clearvar(runtime_p);
					return SLAVE_COMP;
				}
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
			}
			break;
		case 1:
			if (abs(nowtime  - runtime_p->send_start_time) > 20)
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
	static int step_cmpslave = 0, workflg=0, retryflag=0;
	static unsigned int slavenum = 0;
	static int index=0;
	static struct Tsa_Node *currtsa;//=tsa_zb_head;
//	struct Tsa_Node nodetmp;
	struct Tsa_Node *nodetmp;
	int i=0, sendlen=0, findflg=0;
	INT8U addrtmp[6]={};
	time_t nowtime = time(NULL);
	INT8U protocoltmp =0;
	if (module_info.SlavePointMode == 0)
	{
		DbgPrintToFile1(31,"不需要下发从节点信息，无路由管理");
		return TASK_PROCESS;
	}
	switch(step_cmpslave)
	{
		case 0://读取载波从节点数量
			if (abs(nowtime  - runtime_p->send_start_time) > 20 && workflg==0)
			{
				DbgPrintToFile1(31,"暂停抄表5");
				workflg = 1;
				retryflag = 0;
				sendlen = AFN12_F2(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
				runtime_p->send_start_time = nowtime ;
			}else if(runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1 && workflg==1)
			{//确认
				DbgPrintToFile1(31,"收到确认");
				sendlen = AFN10_F1(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				DbgPrintToFile1(31,"读取节点数量");
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime ;
			}else if(runtime_p->format_Up.afn == 0x10 && runtime_p->format_Up.fn == 1 && workflg==1)
			{
				slavenum = runtime_p->format_Up.afn10_f1_up.Num ;
				DbgPrintToFile1(31,"载波模块中从节点 %d 个",slavenum);
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
				step_cmpslave = 1;
				index = 1;
				workflg = 0;
			}else if(abs(nowtime  - runtime_p->send_start_time) > 100 && workflg==1)
			{
				DbgPrintToFile1(31,"读取载波从节点超时");
				workflg = 0;
			}
			break;
		case 1://读取全部载波从节点
			if ((abs(nowtime  - runtime_p->send_start_time) > 20) &&
				runtime_p->format_Up.afn != 0x10 && runtime_p->format_Up.fn!= 2)
			{
				DbgPrintToFile1(31,"读从节点信息 index =%d ",index);
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
					currtsa = tsa_head;	//读取完成 ,开始第 3 步
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
//					addTsaList(&tsa_zb_head,runtime_p->format_Up.afn10_f2_up.SlavePoint[i].Addr);
					addTsaList(&tsa_zb_head,&runtime_p->format_Up.afn10_f2_up.SlavePoint[i]);
				}
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
				if (slavenum<=0  || replyn==0)
				{
					DbgPrintToFile1(31,"载波：读取结束 读%d 个  实际 %d 个",index,slavenum);
					//tsa_print(tsa_zb_head,slavenum);
					step_cmpslave = 2;//读取结束
					currtsa = tsa_zb_head;
				}
			}
			break;
		case 2://删除多余节点
			if (abs(nowtime  - runtime_p->send_start_time) > 10)
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
						nodetmp = getNextTsa(&currtsa); //从载波模块的档案中取出一个 tsa
						findflg = findTsaInList(tsa_head,nodetmp);
						if (findflg==0)
						{
							DbgPrintToFile1(31,"节点  tsatmp (%d): %02x %02x %02x %02x %02x %02x %02x %02x  删除",findflg,nodetmp->tsa.addr[0],nodetmp->tsa.addr[1],nodetmp->tsa.addr[2],nodetmp->tsa.addr[3],nodetmp->tsa.addr[4],nodetmp->tsa.addr[5],nodetmp->tsa.addr[6],nodetmp->tsa.addr[7]);
							addrtmp[5] = nodetmp->tsa.addr[2];
							addrtmp[4] = nodetmp->tsa.addr[3];
							addrtmp[3] = nodetmp->tsa.addr[4];
							addrtmp[2] = nodetmp->tsa.addr[5];
							addrtmp[1] = nodetmp->tsa.addr[6];
							addrtmp[0] = nodetmp->tsa.addr[7];
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
						if(retryflag==0)
						{
							retryflag = 1;
							step_cmpslave = 1;
							index = 1;
							slavenum = tsa_count;
							freeList(tsa_zb_head);
							tsa_zb_head = NULL;
							clearvar(runtime_p);
							DbgPrintToFile1(31,"重读一次从节点信息");
							break;
						}
						else{
							step_cmpslave = 0;
							clearvar(runtime_p);
							return TASK_PROCESS;
						}
					}else
					{
						nodetmp = getNextTsa(&currtsa);
						findflg = findTsaInList(tsa_zb_head,nodetmp);
						if (findflg == 0)
						{
							DbgPrintToFile1(31,"111nodetmp.protocol = %d 节点  tsatmp (%d): %02x %02x %02x %02x %02x %02x %02x %02x  添加",
									nodetmp->protocol,findflg,nodetmp->tsa.addr[0],nodetmp->tsa.addr[1],nodetmp->tsa.addr[2],nodetmp->tsa.addr[3],nodetmp->tsa.addr[4],nodetmp->tsa.addr[5],nodetmp->tsa.addr[6],nodetmp->tsa.addr[7]);
							addrtmp[5] = nodetmp->tsa.addr[2];
							addrtmp[4] = nodetmp->tsa.addr[3];
							addrtmp[3] = nodetmp->tsa.addr[4];
							addrtmp[2] = nodetmp->tsa.addr[5];
							addrtmp[1] = nodetmp->tsa.addr[6];
							addrtmp[0] = nodetmp->tsa.addr[7];
							protocoltmp = nodetmp->protocol;
							if(nodetmp->protocol== DLT_698)
								protocoltmp = getProtocol698Flag(RouterFactory);
							sendlen = AFN11_F1(&runtime_p->format_Down,runtime_p->sendbuf, addrtmp,protocoltmp);//&nodetmp.tsa.addr[2]);//在载波模块中添加一个TSA
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
	}
	else if ( len == 1)
	{
		flag = 0;
		len = 0;
		sleep(5);
	}else if (len==-1)
	{
		flag = 1;
		len  = 0;
	}
	else
	{
		flag = 0;
		len = 0;
	}
	memcpy(runtime_p->format_Down.addr.SourceAddr,runtime_p->masteraddr,6);
#if 0
	sendlen = AFN14_F1(&runtime_p->format_Down,&runtime_p->format_Up,runtime_p->sendbuf,runtime_p->format_Up.afn14_f1_up.SlavePointAddr, flag, 0, len, framebuf);
#else
	sendlen = AFN14_F1(&runtime_p->format_Down,&runtime_p->format_Up,runtime_p->sendbuf,\
			          runtime_p->format_Up.afn14_f1_up.SlavePointAddr, \
			          flag, 0, len, framebuf);
#endif
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
int Format97(FORMAT97 *Data97,OAD oad1,OAD oad2,TSA tsa)
{
	INT8U startIndex =0;
	int find_07item = 0;
	C601F_645 obj601F_97Flag;

	memset(Data97, 0, sizeof(FORMAT97));
	obj601F_97Flag.protocol = DLT_645_97;

	find_07item = OADMap07DI(oad1.OI,oad2,&obj601F_97Flag) ;
//	DbgPrintToFile1(31,"find_07item=%d   %04x %04x    07Flg %02x%02x%02x%02x",find_07item,oad1.OI,oad2.OI,
//			obj601F_07Flag.DI._07.DI_1[0][3],obj601F_07Flag.DI._07.DI_1[0][2],obj601F_07Flag.DI._07.DI_1[0][1],obj601F_07Flag.DI._07.DI_1[0][0]);
	if (find_07item == 1)
	{
		Data97->Ctrl = CTRL_Read_97;
		if(tsa.addr[1] > 5)
		{
			tsa.addr[1] = 5;
			fprintf(stderr,"request698_07Data 电表地址长度大于6");
		}
		startIndex = 5 - tsa.addr[1];
		memcpy(&Data97->Addr[startIndex], &tsa.addr[2], (tsa.addr[1]+1));
//		memcpy(Data07->DI, &obj601F_07Flag.DI_1[0], 4);
		memcpy(Data97->DI, &obj601F_97Flag.DI._07.DI_1[0], 2);

		return 1;
	}
	return 0;
}
int Format07(FORMAT07 *Data07,OAD oad1,OAD oad2,TSA tsa)
{
	INT8U startIndex =0;
	int find_07item = 0;
	C601F_645 obj601F_07Flag;

	memset(Data07, 0, sizeof(FORMAT07));
	obj601F_07Flag.protocol = DLT_645_07;

	find_07item = OADMap07DI(oad1.OI,oad2,&obj601F_07Flag) ;
//	DbgPrintToFile1(31,"find_07item=%d   %04x %04x    07Flg %02x%02x%02x%02x",find_07item,oad1.OI,oad2.OI,
//			obj601F_07Flag.DI._07.DI_1[0][3],obj601F_07Flag.DI._07.DI_1[0][2],obj601F_07Flag.DI._07.DI_1[0][1],obj601F_07Flag.DI._07.DI_1[0][0]);


	fprintf(stderr,"\n-------    1         find_07item=%d",find_07item);

	if (find_07item == 1)
	{
		Data07->Ctrl = CTRL_Read_07;
		if(tsa.addr[1] > 5)
		{
			tsa.addr[1] = 5;
			fprintf(stderr,"request698_07Data 电表地址长度大于6");
		}
		startIndex = 5 - tsa.addr[1];
		memcpy(&Data07->Addr[startIndex], &tsa.addr[2], (tsa.addr[1]+1));
//		memcpy(Data07->DI, &obj601F_07Flag.DI_1[0], 4);
		memcpy(Data07->DI, &obj601F_07Flag.DI._07.DI_1[0], 4);
		fprintf(stderr,"\n-------    2         DI %02x %02x %02x %02x",Data07->DI[0],Data07->DI[1],Data07->DI[2],Data07->DI[3]);
		return 1;
	}
	return 0;
}
int compose6015_698(CLASS_6015 st6015, CLASS_6001 to6001,CLASS_6035* st6035,INT8U *sendbuff)
{
	int sendLen = 0;
	sendLen = composeProtocol698_GetRequest(sendbuff, st6015,to6001.basicinfo.addr);

	return sendLen;
}
int buildProxyFrame(RUNTIME_PLC *runtime_p,struct Tsa_Node *desnode,OAD oad1,OAD oad2)//Proxy_Msg pMsg)
{
	CLASS_6035 st6035 = {};
	CLASS_6015 st6015;
	CLASS_6001 meter = {};

	int sendlen = 0;
	INT8U type = 0;
	FORMAT07 Data07;
	FORMAT97 Data97;
	OAD requestOAD1;
	OAD requestOAD2;
	INT8U addrtmp[6];
	type = desnode->protocol;
	switch (type)
	{
	case DLT_645_97:
			requestOAD1.OI = oad1.OI;//0
			requestOAD2.OI = oad2.OI;//pMsg.oi
			requestOAD2.attflg = oad2.attflg;//0x02
			requestOAD2.attrindex = oad2.attrindex;//0x00
			Format97(&Data97,requestOAD1,requestOAD2,desnode->tsa);
			memset(buf645,0,BUFSIZE645);
			sendlen = composeProtocol97(&Data97, buf645);
			DbgPrintToFile1(31,"sendlen=%d",sendlen);
			DbPrt1(31,"645:", (char *) buf645, sendlen, NULL);
			if(getZone("GW")==0) {
				PacketBufToFile(0,"[ZB_PROXY]S:",(char *) buf645, sendlen, NULL);
			}
			if (sendlen>0)
			{
				memcpy(runtime_p->format_Down.addr.SourceAddr,runtime_p->masteraddr,6);
				addrtmp[5] = desnode->tsa.addr[2];
				addrtmp[4] = desnode->tsa.addr[3];
				addrtmp[3] = desnode->tsa.addr[4];
				addrtmp[2] = desnode->tsa.addr[5];
				addrtmp[1] = desnode->tsa.addr[6];
				addrtmp[0] = desnode->tsa.addr[7];
				return (AFN13_F1(&runtime_p->format_Down,runtime_p->sendbuf,addrtmp, DLT_645_97, 0, buf645, sendlen));
			}
			break;
		case DLT_645_07:
			requestOAD1.OI = oad1.OI;//0
			requestOAD2.OI = oad2.OI;//pMsg.oi
			requestOAD2.attflg = oad2.attflg;//0x02
			requestOAD2.attrindex = oad2.attrindex;//0x00
			Format07(&Data07,requestOAD1,requestOAD2,desnode->tsa);
			memset(buf645,0,BUFSIZE645);
			sendlen = composeProtocol07(&Data07, buf645);
			DbgPrintToFile1(31,"sendlen=%d",sendlen);
			DbPrt1(31,"645:", (char *) buf645, sendlen, NULL);
			if(getZone("GW")==0) {
				PacketBufToFile(0,"[ZB_PROXY]S:",(char *) buf645, sendlen, NULL);
			}
			if (sendlen>0)
			{
				memcpy(runtime_p->format_Down.addr.SourceAddr,runtime_p->masteraddr,6);
				addrtmp[5] = desnode->tsa.addr[2];
				addrtmp[4] = desnode->tsa.addr[3];
				addrtmp[3] = desnode->tsa.addr[4];
				addrtmp[2] = desnode->tsa.addr[5];
				addrtmp[1] = desnode->tsa.addr[6];
				addrtmp[0] = desnode->tsa.addr[7];
				return (AFN13_F1(&runtime_p->format_Down,runtime_p->sendbuf,addrtmp, DLT_645_07, 0, buf645, sendlen));
			}
			break;
		case DLT_698:
			memcpy(&meter.basicinfo.addr,&desnode->tsa,sizeof(TSA));
			memset(&st6015,0,sizeof(CLASS_6015));
			st6015.cjtype = TYPE_NULL;
			st6015.csds.num = 1;
			st6015.csds.csd[0].type = 0;
			st6015.csds.csd[0].csd.oad.OI = oad2.OI;
			st6015.csds.csd[0].csd.oad.attflg = 0x02;
			st6015.csds.csd[0].csd.oad.attrindex = 0x00;
			sendlen = compose6015_698(st6015,meter,&st6035,buf645);
			if (sendlen>0)
			{
				memcpy(runtime_p->format_Down.addr.SourceAddr,runtime_p->masteraddr,6);
				addrtmp[5] = desnode->tsa.addr[2];
				addrtmp[4] = desnode->tsa.addr[3];
				addrtmp[3] = desnode->tsa.addr[4];
				addrtmp[2] = desnode->tsa.addr[5];
				addrtmp[1] = desnode->tsa.addr[6];
				addrtmp[0] = desnode->tsa.addr[7];
//				return (AFN13_F1(&runtime_p->format_Down,runtime_p->sendbuf,addrtmp, getProtocol698Flag(RouterFactory), 0, buf645, sendlen));/*698表规约类型鼎信0，其它厂商3*/
				return (AFN13_F1(&runtime_p->format_Down,runtime_p->sendbuf,addrtmp, 0, 0, buf645, sendlen));
			}
			break;
	}

	return 0;
}
int saveProxyData(FORMAT3762 format_3762_Up,struct Tsa_Node *nodetmp)
{
	INT8U buf645[255];
	int len645=0;
	INT16S datalen=0;
	INT8U apduDataStartIndex = 0;
	INT8U csdNum = 0;
	INT8U nextFlag=0;
	INT8U getResponseType = 0;
	FORMAT07 frame07;
	FORMAT97 frame97;
	INT8U dataContent[50];
	memset(dataContent,0,sizeof(dataContent));
	CLASS_6001 meter = {};
//	CSD_ARRAYTYPE csds;
	memcpy(&meter.basicinfo.addr,&nodetmp->tsa,sizeof(TSA));

	if (format_3762_Up.afn13_f1_up.MsgLength > 0)
	{
		len645 = format_3762_Up.afn13_f1_up.MsgLength;
		memcpy(buf645, format_3762_Up.afn13_f1_up.MsgContent, len645);
		switch (nodetmp->protocol )
		{
		case DLT_645_97:
			if (analyzeProtocol97(&frame97, buf645, len645, &nextFlag) == 0)
			{
				len645 = data97Tobuff698(frame97,dataContent) ;
			}
			break;
			case DLT_645_07:
				if (analyzeProtocol07(&frame07, buf645, len645, &nextFlag) == 0)
				{
					len645 = data07Tobuff698(frame07,dataContent) ;
//					if(data07Tobuff698(frame07,dataContent) > 0)
//					{
//						TSGet(&p_Proxy_Msg_Data->realdata.tm_collect);
//						memcpy(p_Proxy_Msg_Data->realdata.data_All,&dataContent[3],4);
//						memcpy(p_Proxy_Msg_Data->realdata.Rate1_Data,&dataContent[8],4);
//						memcpy(p_Proxy_Msg_Data->realdata.Rate2_Data,&dataContent[13],4);
//						memcpy(p_Proxy_Msg_Data->realdata.Rate3_Data,&dataContent[18],4);
//						memcpy(p_Proxy_Msg_Data->realdata.Rate4_Data,&dataContent[23],4);
//					}
//					else
//					{
//						memset(&p_Proxy_Msg_Data->realdata,0xee,sizeof(RealDataInfo));
//					}
//					p_Proxy_Msg_Data->done_flag = 1;
				}
				break;
			case DLT_698:
				datalen = len645;
				getResponseType = analyzeProtocol698(buf645,&csdNum,len645,&apduDataStartIndex,&datalen);
				if (getResponseType >0 )
				{
					OADDATA_SAVE oadListContent[ROAD_OADS_NUM]={};
					memset(oadListContent,0,ROAD_OADS_NUM*sizeof(OADDATA_SAVE));
					INT16U apdudatalen = 0;
//					len645  = deal698RequestResponse(0,getResponseType,csdNum,&buf645[apduDataStartIndex],dataContent,csds,meter, 0,0);
					len645  = deal698RequestResponse(getResponseType,csdNum,&buf645[apduDataStartIndex],oadListContent,&apdudatalen);
				}
				break;
		}
		if (len645>0)
		{
			TSGet(&p_Proxy_Msg_Data->realdata.tm_collect);
			memcpy(p_Proxy_Msg_Data->realdata.data_All,&dataContent[3],4);
			memcpy(p_Proxy_Msg_Data->realdata.Rate1_Data,&dataContent[8],4);
			memcpy(p_Proxy_Msg_Data->realdata.Rate2_Data,&dataContent[13],4);
			memcpy(p_Proxy_Msg_Data->realdata.Rate3_Data,&dataContent[18],4);
			memcpy(p_Proxy_Msg_Data->realdata.Rate4_Data,&dataContent[23],4);
		}else
			memset(&p_Proxy_Msg_Data->realdata,0xee,sizeof(RealDataInfo));
		p_Proxy_Msg_Data->done_flag = 1;

	}
	return 0;
}

DATA_ITEM checkMeterData(TASK_INFO *meterinfo,int *taski,int *itemi,INT8U usrtype)
{
	int i=0,j=0,needflg=0;
	time_t nowt = time(NULL);
	DATA_ITEM item;
	int fangAnIndex = 0;
	memset(&item,0,sizeof(DATA_ITEM));
//	DbgPrintToFile1(31,"检查  %02x%02x%02x%02x%02x%02x%02x%02x  index=%d 任务数 %d   数据相个数 %d",
//			meterinfo->tsa.addr[0],meterinfo->tsa.addr[1],meterinfo->tsa.addr[2],
//			meterinfo->tsa.addr[3],meterinfo->tsa.addr[4],meterinfo->tsa.addr[5],
//			meterinfo->tsa.addr[6],meterinfo->tsa.addr[7],
//			meterinfo->tsa_index,meterinfo->task_n,meterinfo->task_list[i].fangan.item_n);


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
//				 task_Refresh(&taskinfo.task_list[i] );
			}
		}
	}



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
				 for(j = 0; j<meterinfo->task_list[i].fangan.item_n; j++)
				 {
					 if ( meterinfo->task_list[i].fangan.items[j].sucessflg==0)
					 {
						 item.oad1 = meterinfo->task_list[i].fangan.items[j].oad1;
						 item.oad2 = meterinfo->task_list[i].fangan.items[j].oad2;
						 *taski = i;
						 *itemi = j;
						 DbgPrintToFile1(31,"常规任务,满足抄读条件数据项");

#ifdef CHECK5004RATE
						FP32 s5004rate = success5004Num/(totoal5004Num*1.0);
						DbgPrintToFile1(31,"s5004rate = %f success5004Num = %d totoal5004Num = %d",s5004rate,success5004Num,totoal5004Num);
						TS tsNow;
						TSGet(&tsNow);
						if((s5004rate<99.6)&&(tsNow.Hour<=9)&&(item.oad1.OI!=0x5004))
						{
							DbgPrintToFile1(31,"success5004Num = %d totoal5004Num = %d 抄表率不高切表",success5004Num,totoal5004Num);
							memset(&item,0,sizeof(DATA_ITEM));
						}
#endif

						 return item;	//存在常规任务，满足抄读条件数据项
					 }
				 }
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
		case DLT_645_07:
			Format07(&Data07,item.oad1,item.oad2,desnode->tsa);
			DbgPrintToFile1(31,"当前抄读 【OAD1 %04x-%02x %02x    OAD2 %04x-%02x %02x】%02x%02x%02x%02x ",
						item.oad1.OI,item.oad1.attflg,item.oad1.attrindex,item.oad2.OI,item.oad2.attflg,item.oad2.attrindex,
						Data07.DI[3],Data07.DI[2],Data07.DI[1],Data07.DI[0]);
			sendlen = composeProtocol07(&Data07, buf);
			if (sendlen>0)
				memcpy(item07,Data07.DI,4);// 保存07规约数据项
			break;
		case DLT_698:
//			sendlen = composeProtocol698_GetRequest(buf, st6015, desnode->tsa);
			break;
	}
	return sendlen;
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
		case DLT_645_07:
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
		case DLT_698:
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
int Seek_6015(CLASS_6015 *st6015,CJ_FANGAN fangAn)
{
	int i=0;
	for(i=0; i<FANGAN6015_MAX ;i++)
	{
		if (fangAn6015[i].sernum == fangAn.No)
		{
			memcpy(st6015, &fangAn6015[i], sizeof(CLASS_6015 ));
			return 1;
		}
	}
	return 0;
}
int do_5004_type( int taski, int itemi ,INT8U *buf, struct Tsa_Node *desnode, DATA_ITEM  tmpitem)
{
	INT8U item97[2]={0,0} ,item07[4]={0,0,0,0} ,type = 0;
	time_t getTheTime;
	DateTimeBCD timebcd;
	int sendlen = 0;
	CLASS_6015 st6015;
	FORMAT97 Data97;
	FORMAT07 Data07;

	if (desnode == NULL)
		return 0;

	getTheTime = taskinfo.task_list[taski].beginTime ;
	timebcd =   timet_bcd(getTheTime);
//	timebcd.hour.data = 0;
//	timebcd.min.data = 0;

	type = desnode->protocol;
	switch(type)
	{
		case DLT_645_97:
		Format97(&Data97,tmpitem.oad1,tmpitem.oad2,desnode->tsa);
		DbgPrintToFile1(31,"当前抄读 【OAD1 %04x-%02x %02x    OAD2 %04x-%02x %02x】%02x%02x",
				tmpitem.oad1.OI,tmpitem.oad1.attflg,tmpitem.oad1.attrindex,tmpitem.oad2.OI,tmpitem.oad2.attflg,tmpitem.oad2.attrindex,
							Data97.DI[1],Data97.DI[0]);
		sendlen = composeProtocol97(&Data97, buf);
		if (sendlen>0)
			memcpy(item97,Data97.DI,2);// 保存07规约数据项
		break;
		case DLT_645_07:

			Format07(&Data07,tmpitem.oad1,tmpitem.oad2,desnode->tsa);
			DbgPrintToFile1(31,"当前抄读 【OAD1 %04x-%02x %02x    OAD2 %04x-%02x %02x】%02x%02x%02x%02x ",
					tmpitem.oad1.OI,tmpitem.oad1.attflg,tmpitem.oad1.attrindex,tmpitem.oad2.OI,tmpitem.oad2.attflg,tmpitem.oad2.attrindex,
						Data07.DI[3],Data07.DI[2],Data07.DI[1],Data07.DI[0]);
#ifdef CHECKSSTAMP
			if(taskinfo.freezeStamp == 0)
			{
				DbgPrintToFile1(31,"抄读冻结时标");
				Data07.DI[0] = 0x01;
				Data07.DI[1] = 0x00;
				Data07.DI[2] = 0x06;
				Data07.DI[3] = 0x05;
			}
			if(taskinfo.freezeStamp == 1)
			{
				DbgPrintToFile1(31,"冻结时标不对此方案不抄了");
				for(int i=0;i<taskinfo.task_list[taski].fangan.item_n;i++) {
							taskinfo.task_list[taski].fangan.items[i].sucessflg = 1;}
				return 0;
			}

#endif
			sendlen = composeProtocol07(&Data07, buf);

#ifdef CHECKSSTAMP
			if(taskinfo.freezeStamp == 0)
				return sendlen;
#endif
			if (sendlen>0)
				memcpy(item07,Data07.DI,4);// 保存07规约数据项
			break;
		case DLT_698:
			memset(&st6015,0,sizeof(CLASS_6015));
			Seek_6015(&st6015,taskinfo.task_list[taski].fangan);
			sendlen = composeProtocol698_GetRequest_RN(buf, st6015, desnode->tsa);
			break;
	}
//	sendlen = createMeterFrame(desnode, tmpitem, buf, item07);
	taskinfo.task_list[taski].fangan.items[itemi].item97[0] = item97[0];
	taskinfo.task_list[taski].fangan.items[itemi].item97[1] = item97[1];

	taskinfo.task_list[taski].fangan.items[itemi].item07[0] = item07[0];
	taskinfo.task_list[taski].fangan.items[itemi].item07[1] = item07[1];
	taskinfo.task_list[taski].fangan.items[itemi].item07[2] = item07[2];
	taskinfo.task_list[taski].fangan.items[itemi].item07[3] = item07[3];
	taskinfo.task_list[taski].fangan.items[itemi].savetime = timebcd;
	taskinfo.now_taski = taski;
	taskinfo.now_itemi = itemi;
	taskinfo.task_list[taski].fangan.items[itemi].sucessflg = 1;
	taskinfo.task_list[taski].fangan.item_i = itemi;
	if(type == DLT_698) {		//698抄读成功后,全部数据项置成功标志
		for(int i=0;i<taskinfo.task_list[taski].fangan.item_n;i++) {
			taskinfo.task_list[taski].fangan.items[i].sucessflg = 1;
		}
	}
	myadd2Pools(taski,itemi);
	PrintTaskInfo(&taskinfo,taski);
	if(sendlen<0)
		sendlen = 0;
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
//	task_Refresh(&taskinfo.task_list[taski] );
	if(sendlen<0)
		sendlen = 0;
	return sendlen;
}

/*int do_other_type( int taski, int itemi ,INT8U *buf, struct Tsa_Node *desnode, DATA_ITEM  tmpitem)
{
	INT8U item97[2]={0,0} ,item07[4]={0,0,0,0} ,type = 0;
	int sendlen = 1;
	CLASS_6015 st6015;
	FORMAT07 Data07;
	FORMAT97 Data97;
	if (desnode == NULL)
		return 0;

	type = desnode->protocol;
	DbgPrintToFile1(31,"抄读规约 protocol=%d ",type);

	switch(type)
	{
	case DLT_645_97:

		Format97(&Data97,tmpitem.oad1,tmpitem.oad2,desnode->tsa);
		DbgPrintToFile1(31,"当前抄读 【OAD1 %04x-%02x %02x    OAD2 %04x-%02x %02x】%02x%02x%02x%02x ",
				tmpitem.oad1.OI,tmpitem.oad1.attflg,tmpitem.oad1.attrindex,tmpitem.oad2.OI,tmpitem.oad2.attflg,tmpitem.oad2.attrindex,
							Data97.DI[1],Data97.DI[0]);
		sendlen = composeProtocol97(&Data97, buf);
		if (sendlen>0)
			memcpy(item97,Data97.DI,2);// 保存07规约数据项
		break;
		case DLT_645_07:
			Format07(&Data07,tmpitem.oad1,tmpitem.oad2,desnode->tsa);
			DbgPrintToFile1(31,"当前抄读 【OAD1 %04x-%02x %02x    OAD2 %04x-%02x %02x】%02x%02x%02x%02x ",
					tmpitem.oad1.OI,tmpitem.oad1.attflg,tmpitem.oad1.attrindex,tmpitem.oad2.OI,tmpitem.oad2.attflg,tmpitem.oad2.attrindex,Data07.DI[3],Data07.DI[2],Data07.DI[1],Data07.DI[0]);
			sendlen = composeProtocol07(&Data07, buf);
			if (sendlen>0)
				memcpy(item07,Data07.DI,4);// 保存07规约数据项
			break;
		case DLT_698:
			memset(&st6015,0,sizeof(CLASS_6015));
			if (Seek_6015(&st6015,taskinfo.task_list[taski].fangan)==1)
//				sendlen = composeProtocol698_GetRequest_RN(buf, st6015, desnode->tsa);
				sendlen = composeProtocol698_GetRequest(buf, st6015, desnode->tsa);
			break;
	}
//	sendlen = createMeterFrame(desnode, tmpitem, buf, item07);

//	updateFlags();
	memcpy(taskinfo.task_list[taski].fangan.items[itemi].item97,item97,2);
	memcpy(taskinfo.task_list[taski].fangan.items[itemi].item07,item07,4);
	taskinfo.task_list[taski].fangan.items[itemi].savetime = taskinfo.task_list[taski].begin;
	taskinfo.now_taski = taski;
	taskinfo.now_itemi = itemi;
	taskinfo.task_list[taski].fangan.items[itemi].sucessflg = 1;
	taskinfo.task_list[taski].fangan.item_i = itemi;
	PrintTaskInfo(&taskinfo,taski);
	return sendlen;
}
*/

int do_other_type( int taski, int itemi ,INT8U *buf, struct Tsa_Node *desnode, DATA_ITEM  tmpitem)
{
	INT8U item97[2]={0,0} ,item07[4]={0,0,0,0} ,type = 0;
	int sendlen = 0;
	CLASS_6015 st6015;
	FORMAT07 Data07;
	FORMAT97 Data97;
	if (desnode == NULL)
		return 1;
	type = desnode->protocol;
	DbgPrintToFile1(31,"抄读规约 protocol=%d ",type);
	switch(type) {
	case DLT_645_97:
		Format97(&Data97,tmpitem.oad1,tmpitem.oad2,desnode->tsa);
		DbgPrintToFile1(31,"当前抄读 【OAD1 %04x-%02x %02x    OAD2 %04x-%02x %02x】%02x%02x%02x%02x ",
				tmpitem.oad1.OI,tmpitem.oad1.attflg,tmpitem.oad1.attrindex,tmpitem.oad2.OI,tmpitem.oad2.attflg,tmpitem.oad2.attrindex,
				Data97.DI[1],Data97.DI[0]);
		sendlen = composeProtocol97(&Data97, buf);
		if (sendlen>0)   memcpy(item97,Data97.DI,2);// 保存07规约数据项
		break;
		case DLT_645_07:
			Format07(&Data07,tmpitem.oad1,tmpitem.oad2,desnode->tsa);
			DbgPrintToFile1(31,"当前抄读 【OAD1 %04x-%02x %02x    OAD2 %04x-%02x %02x】%02x%02x%02x%02x ",
					tmpitem.oad1.OI,tmpitem.oad1.attflg,tmpitem.oad1.attrindex,tmpitem.oad2.OI,tmpitem.oad2.attflg,tmpitem.oad2.attrindex,
					Data07.DI[3],Data07.DI[2],Data07.DI[1],Data07.DI[0]);
			sendlen = composeProtocol07(&Data07, buf);
			if (sendlen>0)    memcpy(item07,Data07.DI,4);// 保存07规约数据项
			break;
		case DLT_698:
			memset(&st6015,0,sizeof(CLASS_6015));
			if (Seek_6015(&st6015,taskinfo.task_list[taski].fangan)==1) {
				if (st6015.cjtype == TYPE_INTERVAL)   {
					//曲线点开始时间
					TS ts_start;
					time_t  endtime;
					TSGet(&ts_start);

					if(taskinfo.task_list[taski].ti.units == minute_units)
					{
						INT16U minInterVal = taskinfo.task_list[taski].ti.interval;
						ts_start.Minute = ts_start.Minute/minInterVal*minInterVal;
					}
					if(taskinfo.task_list[taski].ti.units == hour_units)
					{
						INT8U hourInterVal = taskinfo.task_list[taski].ti.interval;
						ts_start.Minute = 0;
						ts_start.Hour = ts_start.Hour/hourInterVal*hourInterVal;
					}

					st6015.data.data[CURVE_INFO_STARTINDEX+8] = 0x1c;
					st6015.data.data[CURVE_INFO_STARTINDEX+9] = (ts_start.Year>>8)&0x00ff;
					st6015.data.data[CURVE_INFO_STARTINDEX+10] = ts_start.Year&0x00ff;
					st6015.data.data[CURVE_INFO_STARTINDEX+11] = ts_start.Month;
					st6015.data.data[CURVE_INFO_STARTINDEX+12] = ts_start.Day;
					st6015.data.data[CURVE_INFO_STARTINDEX+13] = ts_start.Hour;
					st6015.data.data[CURVE_INFO_STARTINDEX+14] = ts_start.Minute;
					st6015.data.data[CURVE_INFO_STARTINDEX+15] = 0;

					//曲线点结束时间
					TS ts_end;
//					ts_end = ts_start - 任务执行间隔；
					endtime = tmtotime_t(ts_start);
					endtime = endtime - TItoSec(taskinfo.task_list[taski].ti);	//15*60
					time_tToTS(endtime,&ts_end);
					st6015.data.data[CURVE_INFO_STARTINDEX] = 0x1c;
					st6015.data.data[CURVE_INFO_STARTINDEX+1] = (ts_end.Year>>8)&0x00ff;
					st6015.data.data[CURVE_INFO_STARTINDEX+2] = ts_end.Year&0x00ff;
					st6015.data.data[CURVE_INFO_STARTINDEX+3] = ts_end.Month;
					st6015.data.data[CURVE_INFO_STARTINDEX+4] = ts_end.Day;
					st6015.data.data[CURVE_INFO_STARTINDEX+5] = ts_end.Hour;
					st6015.data.data[CURVE_INFO_STARTINDEX+6] = ts_end.Minute;
					st6015.data.data[CURVE_INFO_STARTINDEX+7] = 0;

#if 0 //抄曲线的时候强制加上2021
					INT8U csdIndex = 0;
					for(csdIndex = 0;csdIndex < st6015.csds.num;csdIndex++)
					{
						if(st6015.csds.csd[csdIndex].type == 1)
						{
							ROAD sourceRoad;
							memcpy(&sourceRoad,&st6015.csds.csd[csdIndex].csd.road,sizeof(ROAD));

							st6015.csds.csd[csdIndex].csd.road.oads[0].OI = 0x2021;
							st6015.csds.csd[csdIndex].csd.road.oads[0].attflg = 0x02;
							st6015.csds.csd[csdIndex].csd.road.oads[0].attrindex = 0x00;
							st6015.csds.csd[csdIndex].csd.road.num = 1;
							INT8U oadIndex = 0;
							for(oadIndex = 0;oadIndex < sourceRoad.num;oadIndex++)
							{
								if(sourceRoad.oads[oadIndex].OI == 0x2021)
								{
									continue;
								}
								else
								{
									st6015.csds.csd[csdIndex].csd.road.oads[st6015.csds.csd[csdIndex].csd.road.num].OI = sourceRoad.oads[oadIndex].OI;
									st6015.csds.csd[csdIndex].csd.road.oads[st6015.csds.csd[csdIndex].csd.road.num].attflg = sourceRoad.oads[oadIndex].attflg;
									st6015.csds.csd[csdIndex].csd.road.oads[st6015.csds.csd[csdIndex].csd.road.num].attrindex = sourceRoad.oads[oadIndex].attrindex;
									st6015.csds.csd[csdIndex].csd.road.num++;
								}
							}
						}
					}
#endif
					DbgPrintToFile1(31,"任务ID:%d 方案ID:%d 执行频率 %d[%d] 抄读开始时间【%04d-%02d-%02d %02d:%02d:%02d】结束时间【%04d-%02d-%02d %02d:%02d:%02d】",
							taskinfo.task_list[taski].taskId,taskinfo.task_list[taski].fangan.No,taskinfo.task_list[taski].ti.units,
							taskinfo.task_list[taski].ti.interval,ts_end.Year,ts_end.Month,ts_end.Day,ts_end.Hour,ts_end.Minute,ts_end.Sec,
							ts_start.Year,ts_start.Month,ts_start.Day,ts_start.Hour,ts_start.Minute,ts_start.Sec
					);
				}
			    sendlen = composeProtocol698_GetRequest_RN(buf, st6015, desnode->tsa);
			}
			break;
	}
	// sendlen = createMeterFrame(desnode, tmpitem, buf, item07);
	// updateFlags();
	if(sendlen) {		//抄读成功返回再置标志
		memcpy(taskinfo.task_list[taski].fangan.items[itemi].item97,item97,2);
		memcpy(taskinfo.task_list[taski].fangan.items[itemi].item07,item07,4);
		taskinfo.task_list[taski].fangan.items[itemi].savetime = taskinfo.task_list[taski].begin;
		taskinfo.now_taski = taski;
		taskinfo.now_itemi = itemi;
		taskinfo.task_list[taski].fangan.items[itemi].sucessflg = 1;
		taskinfo.task_list[taski].fangan.item_i = itemi;

		if(type == DLT_698) {		//698抄读成功后,全部数据项置成功标志
			DbgPrintToFile1(31,"698电表 taski=%d  item_num=%d",taski,taskinfo.task_list[taski].fangan.item_n);
			for(int i=0;i<taskinfo.task_list[taski].fangan.item_n;i++) {
				taskinfo.task_list[taski].fangan.items[i].sucessflg = 1;
			}
		}
		myadd2Pools(taski,itemi);
	}
	if(sendlen<0)
		sendlen = 0;
	PrintTaskInfo(&taskinfo,taski);
	return sendlen;
}

/*
 * 假如载波模块请求的测量点TSA与内存中的TSA不同, 则需要更新数据项的抄读状态.
 * 如果当前TSA的某个任务的所有数据的状态全不为0, 则表示这个任务的所有数据项都被操作完毕,
 * 除日冻结任务需要补抄外, 其他任务暂时不做补抄处理, 更新下一次任务执行时间,并将各抄读项状态置0.
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
					 flg5004Task = 1;
					 switch (meterinfo->task_list[i].fangan.items[j].sucessflg) {
					 case 2:
						 opt5004Count++;
						 break;
					 case 1://有未抄读成功项
						 opt5004Count++;
						 needRetry = 1;
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
			 {	//日冻结的全部数据项都抄过，但是存在抄读失败的数据
				 meterinfo->task_list[i].tryAgain = 0x55;
				 DbgPrintToFile1(31,"TSA:%02x%02x%02x%02x%02x%02x%02x%02x 此日冻结任务 %d 需要补抄",
						 meterinfo->tsa.addr[0],meterinfo->tsa.addr[1],
						 meterinfo->tsa.addr[2],meterinfo->tsa.addr[3],
						 meterinfo->tsa.addr[4],meterinfo->tsa.addr[5],
						 meterinfo->tsa.addr[6],meterinfo->tsa.addr[7],
						 meterinfo->task_list[i].taskId);
				 //meterinfo->freezeStamp = 0;
			 }

//			 if(flgZeroCnt == 0 && flg5004Task==0) {//flg5004Task==0, 说明当前任务不是日冻结任务   并且成功标志都不是0
//				task_Refresh(&taskinfo.task_list[i]);
//				for(j = 0; j<meterinfo->task_list[i].fangan.item_n; j++) {
//					meterinfo->task_list[i].fangan.items[j].sucessflg = 0;
//				}
//			 }

		}//end if 此任务时间已经到了
	}
}
int ProcessMeter(INT8U *buf,struct Tsa_Node *desnode)
{
	DATA_ITEM  tmpitem;
	int sendlen=0,taski=0, itemi=0 ;//返回 tmpitem指示的具体任务索引 ，itemi指示的具体数据项索引

	if (memcmp(taskinfo.tsa.addr,desnode->tsa.addr,TSA_LEN)!=0 )//内存的TSA 和请求的TSA 比对失败
	{
		if (ifTsaValid(taskinfo.tsa)==1)//判断为有效TSA
		{
			//相关信息存储前，日冻结任务特殊判断，
			//1、日冻结任务全部数据项都抄成功需要更新任务下次执行时间，将成功标识置 0
			//2、日冻结任务部分数据项未成功，标识置 0 ，下次执行时间不变
			//再次请求该表时，如果存在部分未成功数据时，任务可以再次执行，并重新抄读标识为 0的
			chkTsaTask(&taskinfo);
			saveParaClass(0x8888, &taskinfo,taskinfo.tsa_index);
		}
		if (readParaClass(0x8888, &taskinfo, desnode->tsa_index) != 1 )////读取序号为 tsa_index 的任务记录到内存变量 taskinfo 返回 1 成功   0 失败
		{
			memcpy(&taskinfo,&taskinfo_bak,sizeof(taskinfo));
			taskinfo.tsa = desnode->tsa;
			taskinfo.tsa_index = desnode->tsa_index;
			taskinfo.freezeStamp = 0;
			DbgPrintToFile1(31,"第一次请求，用备份结构体初始化该表抄读状态");
		}
	}
	tmpitem = checkMeterData(&taskinfo,&taski,&itemi,desnode->usrtype);	//根据任务的时间计划，查找一个适合抄读的数据项
	if (tmpitem.oad1.OI !=0 || tmpitem.oad2.OI !=0 )
	{	//组织抄读报文
		DbgPrintToFile1(31,"抄读OAD : %04x-%02x%02x  %04x-%02x%02x [taski=%d  itemi=%d]",tmpitem.oad1.OI,tmpitem.oad1.attflg,tmpitem.oad1.attrindex,
				tmpitem.oad2.OI,tmpitem.oad2.attflg,tmpitem.oad2.attrindex,taski,itemi);
		if (tmpitem.oad1.OI == 0x5002)
		{
//			sendlen = do_5002_type( taski, itemi , buf, desnode, tmpitem);//负荷记录

			tmpitem.oad1.OI = 0;
			sendlen = do_other_type( taski, itemi , buf, desnode, tmpitem);//其它数据
		}else if (tmpitem.oad1.OI == 0x5004)
		{
			sendlen = do_5004_type( taski, itemi , buf, desnode, tmpitem);//日冻结
		}
		else
		{
			sendlen = do_other_type( taski, itemi , buf, desnode, tmpitem);//其它数据
		}

		if(getZone("GW")==1)
		{
			//6035发送报文数量+1
			increase6035Value(taskinfo.task_list[taski].taskId,0);
		}

	}else
	{
		sendlen = 0;
	}
	return sendlen;
}
//int ProcessMeter_byJzq(INT8U *buf,INT8U *addrtmp,struct Tsa_Node *nodetmp)
void* ProcessMeter_byJzq(INT8U *buf,INT8U *addrtmp,int *len)//struct Tsa_Node *nodetmp)
{
	struct Tsa_Node *nodetmp;
	DATA_ITEM  tmpitem;
	int ret=0, sendlen=0,taski=0, itemi=0;//返回 tmpitem指示的具体任务索引 ，itemi指示的具体数据项索引
	//检查内存表信息是否合法
	*len = 0;
	if (ifTsaValid(taskinfo.tsa) == 0)//判断为无效TSA
	{
		ret = readParaClass(0x8888, &taskinfo, tsa_head->tsa_index) ;//读取序号为 tsa_index 的任务记录到内存变量 taskinfo
		if (ret != 1 )// 返回 1 成功   0 失败
		{
			memcpy(&taskinfo,&taskinfo_bak,sizeof(taskinfo));
			taskinfo.tsa =  tsa_head->tsa;
			taskinfo.tsa_index = tsa_head->tsa_index;
			taskinfo.freezeStamp = 0;
			DbgPrintToFile1(31,"第一次请求，用备份结构体初始化该表抄读状态");
		}
	}

	nodetmp = NULL;
	nodetmp = getNodeByTSA(tsa_head,taskinfo.tsa);
	if (nodetmp!=NULL)
	fprintf(stderr,"\nkaishi nodetmp TSA(index=%d  type=%d)： %02x%02x%02x%02x%02x%02x%02x%02x",nodetmp->tsa_index,nodetmp->usrtype,
			nodetmp->tsa.addr[0],nodetmp->tsa.addr[1],nodetmp->tsa.addr[2],nodetmp->tsa.addr[3]
			,nodetmp->tsa.addr[4],nodetmp->tsa.addr[5],nodetmp->tsa.addr[6],nodetmp->tsa.addr[7]);

	while(nodetmp != NULL)
	{
		JugeLastTime_SetZero(&taskinfo);//计算测量点所有任务上一次开始时刻
		tmpitem = checkMeterData(&taskinfo,&taski,&itemi,nodetmp->usrtype);	//根据任务的时间计划，查找一个适合抄读的数据项
		if (tmpitem.oad1.OI !=0 || tmpitem.oad2.OI !=0 )
		{	//组织抄读报文
			if (tmpitem.oad1.OI == 0x5002)
			{
				//sendlen = do_5002_type( taski, itemi , buf, desnode, tmpitem);//负荷记录
				tmpitem.oad1.OI = 0;
				sendlen = do_other_type( taski, itemi , buf, nodetmp, tmpitem);//其它数据

			}else if (tmpitem.oad1.OI == 0x5004)
				sendlen = do_5004_type( taski, itemi , buf, nodetmp, tmpitem);//日冻结
			else
				sendlen = do_other_type( taski, itemi , buf, nodetmp, tmpitem);//其它数据

			if(nodetmp->tsa.addr[1] > 5)
			{
				nodetmp->tsa.addr[1] = 5;
				fprintf(stderr,"request698_07Data 电表地址长度大于6");
			}

			int startIndex = 5 - nodetmp->tsa.addr[1];
			memcpy(&addrtmp[startIndex], &nodetmp->tsa.addr[2], (nodetmp->tsa.addr[1]+1));

			DbgPrintToFile1(31,"TSA[ %02x-%02x-%02x%02x%02x%02x%02x%02x ] ",\
					nodetmp->tsa.addr[0],nodetmp->tsa.addr[1],nodetmp->tsa.addr[2],nodetmp->tsa.addr[3],\
					nodetmp->tsa.addr[4],nodetmp->tsa.addr[5],nodetmp->tsa.addr[6],nodetmp->tsa.addr[7]);//TODO 抄表组织报文
			DbgPrintToFile1(31,"任务 %d ， 第 %d 项 【OAD1 %04x-%02x %02x    OAD2 %04x-%02x %02x】sendlen=%d  nodetmp=%p",taski,itemi,
					tmpitem.oad1.OI,tmpitem.oad1.attflg,tmpitem.oad1.attrindex,
					tmpitem.oad2.OI,tmpitem.oad2.attflg,tmpitem.oad2.attrindex,sendlen,nodetmp);
			*len = sendlen;
			DbgPrintToFile1(31,"有数据抄读，刷新任务内存状态");

			if(getZone("GW")==1)
			{
				//6035发送报文数量+1
				increase6035Value(taskinfo.task_list[taski].taskId,0);
			}

//			chkTsaTask(&taskinfo);
			return nodetmp;
		}else
		{
			chkTsaTask(&taskinfo);
			saveParaClass(0x8888, &taskinfo,taskinfo.tsa_index);
			nodetmp = nodetmp->next;
			if (nodetmp!=NULL)
			fprintf(stderr,"\n nodetmp TSA(index=%d  type=%d)： %02x%02x%02x%02x%02x%02x%02x%02x",nodetmp->tsa_index,nodetmp->usrtype,
					nodetmp->tsa.addr[0],nodetmp->tsa.addr[1],nodetmp->tsa.addr[2],nodetmp->tsa.addr[3]
					,nodetmp->tsa.addr[4],nodetmp->tsa.addr[5],nodetmp->tsa.addr[6],nodetmp->tsa.addr[7]);
			if (nodetmp!=NULL)
			{
				ret = readParaClass(0x8888, &taskinfo, nodetmp->tsa_index) ;//读取序号为 tsa_index 的任务记录到内存变量 taskinfo
				if (ret != 1 )// 返回 1 成功   0 失败
				{
					fprintf(stderr,"\n读取失败");
					memcpy(&taskinfo,&taskinfo_bak,sizeof(taskinfo));
					taskinfo.tsa = nodetmp->tsa;
					taskinfo.tsa_index = nodetmp->tsa_index;
				}
				fprintf(stderr,"\n读成功 tsa_index=%d  ",taskinfo.tsa_index);
			}
		}
	}
	if (nodetmp == NULL )// 返回 1 成功   0 失败
	{
		memset(&taskinfo.tsa,0,sizeof(TSA));
	}
	return NULL;
}
//int buildMeterFrame(INT8U *buf,struct Tsa_Node *desnode,CJ_FANGAN fangAn)
//{
//	INT8U type = 0;
//	FORMAT07 Data07;
//	int sendlen = 0 ,itemindex = -1 ,readcounter = 0;
//
//	if (desnode == NULL)
//		return 0;
//
//	readcounter = desnode->readnum;
//	type = desnode->protocol;
//	itemindex = findFirstZeroFlg(desnode, fangAn.item_n);//fangAn.item_n);
//	memset(buf,0,BUFSIZE645);
//	if (itemindex >= 0 && readcounter < 2)//全部数据最多抄读2次
//	{
//		DbgPrintToFile1(31,"当前抄读第 %d 数据项 curr_i指到 %d 【OAD1 %04x-%02x %02x    OAD2 %04x-%02x %02x】第 %d 次抄读",itemindex,desnode->curr_i,
//				fangAn.items[itemindex].oad1.OI,fangAn.items[itemindex].oad1.attflg,fangAn.items[itemindex].oad1.attrindex,
//				fangAn.items[itemindex].oad2.OI,fangAn.items[itemindex].oad2.attflg,fangAn.items[itemindex].oad2.attrindex,desnode->readnum+1);
//		switch (type)
//		{
//			case DLT_645_07:
//				Format07(&Data07,fangAn.items[itemindex].oad1,fangAn.items[itemindex].oad2,desnode->tsa);
//				sendlen = composeProtocol07(&Data07, buf);
//				if (sendlen>0)
//				{
////					DbPrt1(31,"645:", (char *) buf, sendlen, NULL);
//					return sendlen;
//				}
//				break;
//			case DLT_698:
//				return 20;
//		}
//	}
//	return 0;
//}
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
void saveTaskData_NormalData_1(INT8U protocol,int taskid,POOLTYPE poolone,FORMAT97 *frame97,FORMAT07 *frame07,TS ts)
{
	INT8U dataContent[50]={},alldata[100]={};
	int len698=0;

	DbgPrintToFile1(31,"收到普通数据回码");
	if(protocol == DLT_645_97)
	{
		len698 = data97Tobuff698(*frame97,dataContent);
	}
	else
	{
		len698 = data07Tobuff698(*frame07,dataContent);
	}

	if(len698 > 0)
	{
		alldata[0] = 0x55;
//		memcpy(&alldata[1],taskinfo.tsa.addr,17);
		memcpy(&alldata[1],poolone.tsa.addr,17);
		memcpy(&alldata[18],dataContent,len698);
		len698 = len698 + 18;
//		DbPrt1(31,"存储:", (char *) alldata, len698, NULL);
		SaveOADData(taskid,poolone.item.oad1,poolone.item.oad2,alldata,len698,ts);
	}
}
void saveTaskData_NormalData(INT8U protocol,TASK_INFO *tskinfo,FORMAT97 *frame97,FORMAT07 *frame07,TS ts)
{
	INT8U dataContent[50]={},alldata[100]={};
	int ti=0,ii=0,len698=0;
	ti = tskinfo->now_taski;
	ii = tskinfo->now_itemi;

	DbgPrintToFile1(31,"收到普通数据回码");
	if(protocol == DLT_645_97)
	{
		len698 = data97Tobuff698(*frame97,dataContent);
	}
	else
	{
		len698 = data07Tobuff698(*frame07,dataContent);
	}

	if(len698 > 0)
	{
		alldata[0] = 0x55;
		memcpy(&alldata[1],tskinfo->tsa.addr,17);
		memcpy(&alldata[18],dataContent,len698);
		len698 = len698 + 18;
		DbgPrintToFile1(31,"存储任务[%d][%04d-%02d-%02d %02d:%02d:%02d]",tskinfo->task_list[ti].taskId,ts.Year,ts.Month,ts.Day,ts.Hour,ts.Minute,ts.Sec);
//		DbPrt1(31,"存储:", (char *) alldata, len698, NULL);
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

//	DbPrt1(31,"curve:", (char *) frame07->Data, frame07->Length, NULL);
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
//						DbPrt1(31,"存储:", (char *) alldata, len698, NULL);
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
void doSave(INT8U protocol,FORMAT97 frame97,FORMAT07 frame07)
{
	struct Tsa_Node *nodetmp;
	DateTimeBCD timebcd;
	DATA_ITEM item;
	TSA tsatmp;
	TS ts;
	INT16U savetaskID = 0;
	if(protocol == DLT_645_97)
	{
		Addr_TSA(frame97.Addr,&tsatmp);
	}
	else
	{
		Addr_TSA(frame07.Addr,&tsatmp);
	}
	memcpy(item.item07,frame07.DI,4);
	memcpy(item.item97,frame97.DI,2);
	nodetmp = getNodeByTSA(tsa_head,tsatmp);
	if ((nodetmp==NULL ) || (protocol!=DLT_645_07 && protocol!=DLT_645_97 ))
		return;
	DbgPrintToFile1(31,"上数=%02x%02x%02x%02x%02x%02x%02x%02x [%d]规约%d ",
			tsatmp.addr[0],tsatmp.addr[1],tsatmp.addr[2],tsatmp.addr[3],
			tsatmp.addr[4],tsatmp.addr[5],tsatmp.addr[6],tsatmp.addr[7],nodetmp->tsa_index,protocol);


	if (memcmp(taskinfo.tsa.addr,tsatmp.addr,8) == 0 )//上数TSA就在内存中
	{
#ifdef CHECKSSTAMP
		if((protocol==DLT_645_07)&&(frame07.DI[3]==0x05)&&(frame07.DI[2]==0x06)&&(frame07.DI[1]==0x00)&&(frame07.DI[0]==0x01))
		{
			INT8U stampbuf[50] = {0};
			data07Tobuff698(frame07,stampbuf);
			DbgPrintToFile1(31,"冻结时标frame07.Data = %02x %02x %02x %02x %02x %02x dataContent = %02x %02x %02x %02x %02x %02x",
					frame07.Data[0],frame07.Data[1],frame07.Data[2],frame07.Data[3],frame07.Data[4],frame07.Data[5],
					stampbuf[0],stampbuf[1],stampbuf[2],stampbuf[3],stampbuf[4],stampbuf[5]);
			if(isTimerSame(0,stampbuf))
			{
				taskinfo.freezeStamp = 2;
				DbgPrintToFile1(31,"冻结时标正确");
			}
			else
			{
				taskinfo.freezeStamp = 0;
				DbgPrintToFile1(31,"冻结时标错误");
			}
			return;
		}
#endif

		timebcd = ChgSucessFlg(&taskinfo,item,nodetmp->usrtype,protocol,2);
		DbgPrintToFile1(31,"TSA在内存中 %d-%d-%d %d:%d tsknum=%d",timebcd.year.data,timebcd.month.data,timebcd.day.data,timebcd.hour.data,timebcd.min.data,taskinfo.task_n);

		if (timebcd.year.data!=0 && timebcd.month.data!=0 && timebcd.day.data!=0)
		{
			TimeBCDToTs(timebcd,&ts);//ts 为数据存储时间
			saveTaskData_NormalData(protocol,&taskinfo,&frame97,&frame07,ts);
		}
		savetaskID = taskinfo.task_list[taskinfo.now_taski].taskId;
	}else
	{
		if (readParaClass(0x8888, &taskinfo_tmp, nodetmp->tsa_index) == 1 )
		{
#ifdef CHECKSSTAMP

		if((protocol==DLT_645_07)&&(frame07.DI[3]==0x05)&&(frame07.DI[2]==0x06)&&(frame07.DI[1]==0x00)&&(frame07.DI[0]==0x01))
		{
			INT8U stampbuf[50] = {0};
			data07Tobuff698(frame07,stampbuf);
			DbgPrintToFile1(31,"冻结时标frame07.Data = %02x %02x %02x %02x %02x %02x dataContent = %02x %02x %02x %02x %02x %02x",
					frame07.Data[0],frame07.Data[1],frame07.Data[2],frame07.Data[3],frame07.Data[4],frame07.Data[5],
					stampbuf[0],stampbuf[1],stampbuf[2],stampbuf[3],stampbuf[4],stampbuf[5]);
			if(isTimerSame(0,stampbuf))
			{
				taskinfo_tmp.freezeStamp = 2;
				DbgPrintToFile1(31,"冻结时标正确");
			}
			else
			{
				taskinfo_tmp.freezeStamp = 0;
				DbgPrintToFile1(31,"冻结时标错误");
			}
			saveParaClass(0x8888, &taskinfo_tmp,taskinfo_tmp.tsa_index);
			return;
		}
#endif

			timebcd = ChgSucessFlg(&taskinfo_tmp,item,nodetmp->usrtype,protocol,2);
			DbgPrintToFile1(31,"TSA不在内存中 %d-%d-%d %d:%d",timebcd.year.data,timebcd.month.data,timebcd.day.data,timebcd.hour.data,timebcd.min.data);
			if (timebcd.year.data!=0 && timebcd.month.data!=0 && timebcd.day.data!=0)
			{
				TimeBCDToTs(timebcd,&ts);//ts 为数据存储时间
				saveTaskData_NormalData(protocol,&taskinfo_tmp,&frame97,&frame07,ts);
				saveParaClass(0x8888, &taskinfo_tmp,taskinfo_tmp.tsa_index);
			}
			savetaskID = taskinfo_tmp.task_list[taskinfo_tmp.now_taski].taskId;
		}
	}

	if((getZone("GW")==1)&&(savetaskID>0))
	{
		increase6035Value(savetaskID,1);
	}

}
INT8U ChgSucessFlg_698(TSA tsaMeter,INT8U taskid)
{
	INT8U ret = 0;
	int i = 0,j = 0;
		if (memcmp(taskinfo.tsa.addr,tsaMeter.addr,8) == 0 )//上数TSA就在内存中
		{
			int tnum = taskinfo.task_n;
			for(i=0;i<tnum;i++)
			{
				if(taskinfo.task_list[i].taskId == taskid)
				{
					for(j=0; j<taskinfo.task_list[i].fangan.item_n; j++)
					{
						taskinfo.task_list[i].fangan.items[j].sucessflg = 2;
					}
				}
			}
			saveParaClass(0x8888, &taskinfo,taskinfo.tsa_index);
			DbgPrintToFile1(31,"1---存储698表标识 tsaMeter = %02x %02x %02x %02x %02x %02x %02x %02x ",
					tsaMeter.addr[0],tsaMeter.addr[1],tsaMeter.addr[2],tsaMeter.addr[3],
					tsaMeter.addr[4],tsaMeter.addr[5],tsaMeter.addr[6],tsaMeter.addr[7]);
		}
		else
		{
			struct Tsa_Node *nodetmp = NULL;
			nodetmp = getNodeByTSA(tsa_head,tsaMeter);

			DbgPrintToFile1(31,"2---存储698表标识 tsaMeter = %02x %02x %02x %02x %02x %02x %02x %02x ",
					tsaMeter.addr[0],tsaMeter.addr[1],tsaMeter.addr[2],tsaMeter.addr[3],
					tsaMeter.addr[4],tsaMeter.addr[5],tsaMeter.addr[6],tsaMeter.addr[7]);

			DbgPrintToFile1(31,"2---nodetmp表 tsa = %02x %02x %02x %02x %02x %02x %02x %02x --(%d)",
					nodetmp->tsa.addr[0],nodetmp->tsa.addr[1],nodetmp->tsa.addr[2],nodetmp->tsa.addr[3],
					nodetmp->tsa.addr[4],nodetmp->tsa.addr[5],nodetmp->tsa.addr[6],nodetmp->tsa.addr[7],nodetmp->tsa_index);


			if (nodetmp!=NULL)
			{
				if (readParaClass(0x8888, &taskinfo_tmp, nodetmp->tsa_index) == 1 )
				{
					int tnum = taskinfo_tmp.task_n;
					for(i=0;i<tnum;i++)
					{
						if(taskinfo_tmp.task_list[i].taskId == taskid)
						{
							for(j=0; j<taskinfo_tmp.task_list[i].fangan.item_n; j++)
							{
								taskinfo_tmp.task_list[i].fangan.items[j].sucessflg = 2;
							}
						}
					}
					saveParaClass(0x8888, &taskinfo_tmp,taskinfo_tmp.tsa_index);
					DbgPrintToFile1(31,"2---存储698表标识 tsaMeter = %02x %02x %02x %02x %02x %02x %02x %02x ",
							tsaMeter.addr[0],tsaMeter.addr[1],tsaMeter.addr[2],tsaMeter.addr[3],
							tsaMeter.addr[4],tsaMeter.addr[5],tsaMeter.addr[6],tsaMeter.addr[7]);
				}
			}

		}
	return ret;
}
INT8U doSave_698(INT8U* buf645,int len645)
{

	INT8U ret = 0,csdNum = 0,taskid = 0,csdsIndex = 0;
	INT16S dataLen = len645;
	INT8U apduDataStartIndex = 0;

	INT16U count = getFECount(buf645, len645); //得到待解析报文中前导符FE的个数
	TSA tsaMeter;
	memset(&tsaMeter,0,sizeof(TSA));
	Addr_TSA(&buf645[5+count],&tsaMeter);


	INT8U getResponseType = analyzeProtocol698_RN(buf645,&csdNum,len645,&apduDataStartIndex,&dataLen);
	if(getResponseType == 0)
	{
		DbgPrintToFile1(31,"analyzeProtocol698_RN 失败");
		return ret;
	}

#ifdef TESTDEF
	fprintf(stderr,"buf645 [%d] = \n",len645);
	INT16U prtIndex =0;
	for(prtIndex = 0;prtIndex < len645;prtIndex++)
	{
		fprintf(stderr,"%02x ",buf645[prtIndex]);
		if((prtIndex+1)%20 ==0)
		{
			fprintf(stderr,"\n");
		}
	}
#endif

	OADDATA_SAVE oadListContent[ROAD_OADS_NUM]={};
	memset(oadListContent,0,ROAD_OADS_NUM*sizeof(OADDATA_SAVE));

	INT16U apdudatalen = 0;
	ROAD_ITEM item_road;
	memset(&item_road,0,sizeof(item_road));
	CSD_ARRAYTYPE csds = {0};
	CLASS_6001 tsa_group;
	CLASS_6015 class6015;
	memset(&tsa_group,0,sizeof(CLASS_6001));
	memset(&class6015,0,sizeof(CLASS_6015));
	OI_698 oi = 0x6015;

	if(get6001ObjByTSA(tsaMeter,&tsa_group) == 0)
	{
		DbgPrintToFile1(31,"测点不在6000里");
		return ret;
	}


	DbgPrintToFile1(31,"getResponseType = %d csdNum = %d",getResponseType,csdNum);
	if(getResponseType < GET_REQUEST_RECORD)
	{

		INT8U datacount = deal698RequestResponse(getResponseType,csdNum,&buf645[apduDataStartIndex],oadListContent,&apdudatalen);
		csds.num = datacount;
		for(csdsIndex = 0;csdsIndex < datacount;csdsIndex++)
		{
			csds.csd[csdsIndex].type = 0;
			memcpy(&csds.csd[csdsIndex].csd.oad,&oadListContent[csdsIndex].oad_r,sizeof(OAD));
			DbgPrintToFile1(31,"---oadListContent[%d].oad_r = %04x",csdsIndex,oadListContent[csdsIndex].oad_r.OI);
		}
#if 0
		DbgPrintToFile1(31,"csds.num = %d",csds.num);
		for(csdsIndex = 0;csdsIndex < datacount;csdsIndex++)
		{
			DbgPrintToFile1(31,"---csds[%d] type = %d OI = %04x",csdsIndex,csds.csd[csdsIndex].type,csds.csd[csdsIndex].csd.oad.OI);
		}
#endif
		extendcsds(csds,&item_road);
#if 0
		INT16U tmpindex = 0;
		for(tmpindex = 0;tmpindex < item_road.oadmr_num;tmpindex++)
		{
			DbgPrintToFile1(31,"---item_road taskid = %d oad_m = %04x oad_r = %04x",
					item_road.oad[tmpindex].taskid,item_road.oad[tmpindex].oad_m.OI,taskid,item_road.oad[tmpindex].oad_r.OI);
		}
#endif
		if((taskid = GetTaskidFromCSDs(item_road,&tsa_group)) == 0) {
			DbgPrintToFile1(31,"---########GetTaskData_error: taskid=%d\n",taskid);
				return ret;
		}
		if(datacount > 0)
		{
			TS ts_cc;
			TSGet(&ts_cc);
			saveREADOADdata(taskid,tsaMeter,oadListContent,datacount,ts_cc);
			ChgSucessFlg_698(tsaMeter,taskid);
		}
		else
		{
			DbgPrintToFile1(1,"\n收到回复报文但是没有数据taskid　＝　%d TSA=%02x %02x %02x %02x %02x %02x %02x %02x",
						taskid,tsaMeter.addr[0],tsaMeter.addr[1],tsaMeter.addr[2],tsaMeter.addr[3],
						tsaMeter.addr[4],tsaMeter.addr[5],tsaMeter.addr[6],tsaMeter.addr[7]);
		}

	}
	else
	{
		INT8U rcvCSDnum = 0,recordNum = 0;
		INT16U oaddataLen = 0;

		oaddataLen = parseSingleROADDataHead(&buf645[apduDataStartIndex],oadListContent,&rcvCSDnum,&recordNum);
		apduDataStartIndex += oaddataLen;

		INT8U ishas2021 = 0;
		csds.num = 1;
		csds.csd[0].type = 1;
		memcpy(&csds.csd[0].csd.road.oad,&oadListContent[0].oad_m,sizeof(OAD));
		csds.csd[0].csd.road.num = rcvCSDnum;
		for(csdsIndex = 0;csdsIndex < rcvCSDnum;csdsIndex++)
		{
			if(oadListContent[csdsIndex].oad_r.OI == 0x2021)
			{
				ishas2021 = 1;
				continue;
			}
			memcpy(&csds.csd[0].csd.road.oads[csdsIndex],&oadListContent[csdsIndex].oad_r,sizeof(OAD));
			DbgPrintToFile1(31,"oadListContent[%d].oad_r = %04x",csdsIndex,oadListContent[csdsIndex].oad_r.OI);
		}
		if(ishas2021 == 1)
		{
			csds.csd[0].csd.road.num -= 1;
		}
#if 0
		DbgPrintToFile1(31,"csds.num = %d csds.csd[0].csd.road.num = %d",csds.num,csds.csd[0].csd.road.num);
		for(csdsIndex = 0;csdsIndex < rcvCSDnum;csdsIndex++)
		{
			DbgPrintToFile1(31,"csds[%d] m_OI=%04x OI=%04x",csdsIndex,csds.csd[0].csd.road.oad.OI,csds.csd[0].csd.road.oads[csdsIndex].OI);
		}
#endif
		extendcsds(csds,&item_road);
#if 0
		INT16U tmpindex = 0;
		for(tmpindex = 0;tmpindex < item_road.oadmr_num;tmpindex++)
		{
			DbgPrintToFile1(31,"item_road taskid = %d oad_m = %04x oad_r = %04x",
					item_road.oad[tmpindex].taskid,item_road.oad[tmpindex].oad_m.OI,taskid,item_road.oad[tmpindex].oad_r.OI);
		}
#endif
		if((taskid = GetTaskidFromCSDs(item_road,&tsa_group)) == 0) {
			DbgPrintToFile1(31,"########GetTaskData_error: taskid=%d\n",taskid);
				return ret;
		}
		DbgPrintToFile1(31,"rcvCSDnum = %d recordNum = %d taskid = %d",rcvCSDnum,recordNum,taskid);
		INT16U taskIndex = 0;
		for(taskIndex=0;taskIndex<TASK6012_CAIJI;taskIndex++)
		{
			if (list6013[taskIndex].basicInfo.taskID == taskid)
			{
				if (readCoverClass(oi, list6013[taskIndex].basicInfo.sernum, &class6015, sizeof(CLASS_6015), coll_para_save)== 1)
				{
					if(rcvCSDnum > 0 && recordNum > 0)
					{
						INT8U recordIndex = 0;
						TS freezeTimeStamp;
						TSGet(&freezeTimeStamp);

						if(class6015.cjtype == TYPE_INTERVAL)
						{
							freezeTimeStamp.Sec = 0;

							INT16U minInterVal = 0;//冻结时标间隔-分钟
							if(list6013[taskIndex].basicInfo.interval.units == minute_units)
							{
								minInterVal = list6013[taskIndex].basicInfo.interval.interval;
								freezeTimeStamp.Minute = freezeTimeStamp.Minute/minInterVal*minInterVal;
							}
							if(list6013[taskIndex].basicInfo.interval.units == hour_units)
							{
								INT8U hourInterVal = list6013[taskIndex].basicInfo.interval.interval;
								freezeTimeStamp.Minute = 0;
								freezeTimeStamp.Hour = freezeTimeStamp.Hour/hourInterVal*hourInterVal;
							}

						}


						for(recordIndex = 0;recordIndex < recordNum;recordIndex++)
						{
							oaddataLen = parseSingleROADDataBody(&buf645[apduDataStartIndex],oadListContent,rcvCSDnum);
							apduDataStartIndex += oaddataLen;
							if((oadListContent[0].oad_r.OI==0x2021)&&(oadListContent[0].data[0] == 0x1c))
							{
								freezeTimeStamp.Year = oadListContent[0].data[1];
								freezeTimeStamp.Year = freezeTimeStamp.Year<<8;
								freezeTimeStamp.Year += oadListContent[0].data[2];
								freezeTimeStamp.Month = oadListContent[0].data[3];
								freezeTimeStamp.Day = oadListContent[0].data[4];
								freezeTimeStamp.Hour = oadListContent[0].data[5];
								freezeTimeStamp.Minute = oadListContent[0].data[6];
								freezeTimeStamp.Sec = oadListContent[0].data[7];
								freezeTimeStamp.Week = 0;
							}
							saveREADOADdata(taskid,tsaMeter,oadListContent,rcvCSDnum,freezeTimeStamp);
							if(class6015.cjtype == TYPE_INTERVAL)
							{
								//DbgPrintToFile1(31,"曲线时间点:%d-%d-%d %d:%d",freezeTimeStamp.Year,freezeTimeStamp.Month,freezeTimeStamp.Day,freezeTimeStamp.Hour,freezeTimeStamp.Minute);
								INT32S bactm = 0-((class6015.data.data[1]<<8)+class6015.data.data[2]);
								tminc(&freezeTimeStamp,class6015.data.data[0],bactm);
							}

						}
						if((recordNum > 0)&&(rcvCSDnum > 0))
						{
							ChgSucessFlg_698(tsaMeter,taskid);
						}
						else
						{
							DbgPrintToFile1(1,"\n收到回复报文但是没有数据taskid　＝　%d TSA=%02x %02x %02x %02x %02x %02x %02x %02x",
										taskid,tsaMeter.addr[0],tsaMeter.addr[1],tsaMeter.addr[2],tsaMeter.addr[3],
										tsaMeter.addr[4],tsaMeter.addr[5],tsaMeter.addr[6],tsaMeter.addr[7]);
						}
					}
				}
			}
		}

	}

	if((getZone("GW")==1)&&(taskid>0))
	{
		increase6035Value(taskid,1);
	}

#if 0
	//力合微的模块,07表AFN14_F1请求抄读时,返回AFN06_F2上报抄读数据的规约类型为0,此处用698解析报文失败后,用07规约解析存储
	else  if (analyzeProtocol07(&frame07, buf645, len645, &nextFlag) == 0)
	{
		DbgPrintToFile1(31,"存储规约类型%d",format_3762_Up.afn06_f2_up.Protocol);
		doSave(DLT_645_07,frame97,frame07);
	}
#endif
	return ret;
}
int SaveTaskData(FORMAT3762 format_3762_Up,INT8U taskid,INT8U fananNo)
{
	int len645=0;
	INT8U nextFlag=0,  buf645[1024]={0};
	FORMAT07 frame07 = {};
	FORMAT97 frame97 = {};

	DbgPrintToFile1(31,"存储规约类型%d",format_3762_Up.afn06_f2_up.Protocol);
	if (RouterFactory == DX_factory)//鼎信路由载波
	{
		if (format_3762_Up.afn06_f2_up.Protocol==0)
		{
			format_3762_Up.afn06_f2_up.Protocol  = DLT_698;
			DbgPrintToFile1(31,"鼎信载波，存储规约改为类型%d",format_3762_Up.afn06_f2_up.Protocol);
		}
	}
	if (format_3762_Up.afn06_f2_up.MsgLength > 0)
	{
		len645 = format_3762_Up.afn06_f2_up.MsgLength;
		memcpy(buf645, format_3762_Up.afn06_f2_up.MsgContent, len645);
		INT8U prtIndex = 0;
		for(prtIndex = 0;prtIndex < len645;prtIndex++)
		{
			fprintf(stderr,"%02x ",buf645[prtIndex]);
		}
		if(format_3762_Up.afn06_f2_up.Protocol == DLT_645_97)
		{
			if (analyzeProtocol97(&frame97, buf645, len645, &nextFlag) == 0)
			{
				doSave(DLT_645_97,frame97,frame07);
			}
		}
		else if((format_3762_Up.afn06_f2_up.Protocol == DLT_698)||(format_3762_Up.afn06_f2_up.Protocol == PROTOCOL_UNKNOWN))
		{
			doSave_698(buf645,len645);
		}
		else
		{
			if (analyzeProtocol07(&frame07, buf645, len645, &nextFlag) == 0)
			{
				doSave(DLT_645_07,frame97,frame07);
			}
		}
	}
	if (format_3762_Up.afn13_f1_up.MsgLength > 0)
	{
		len645 = format_3762_Up.afn13_f1_up.MsgLength;
		memcpy(buf645, format_3762_Up.afn13_f1_up.MsgContent, len645);
		if(format_3762_Up.afn13_f1_up.Protocol == DLT_645_97)
		{
			if (analyzeProtocol97(&frame97, buf645, len645, &nextFlag) == 0)
			{
				doSave(DLT_645_97,frame97,frame07);
			}
		}
		else
		{
			if (analyzeProtocol07(&frame07, buf645, len645, &nextFlag) == 0)
			{
				doSave(DLT_645_07,frame97,frame07);
			}
		}
	}
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
			if ( abs(nowtime - runtime_p->send_start_time) > 20)
			{
				DbgPrintToFile1(31,"\n重启抄表");
				clearvar(runtime_p);
				runtime_p->redo = 0;
				runtime_p->send_start_time = nowtime ;
				sendlen = AFN12_F1(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
			}else if ((runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1)||
					(abs( nowtime - runtime_p->send_start_time) > 10))
			{//确认  //或者抄过10秒未收到确认
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime ;
				step_cj = 2;
				beginwork = 0;
			}
			break;
		case 1://恢复抄读
			if (abs( nowtime - runtime_p->send_start_time) > 20)
			{
				DbgPrintToFile1(31,"\n恢复抄表");
				runtime_p->redo = 0;
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime ;
				sendlen = AFN12_F3(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
			}else if ((runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1) ||
					  (abs( nowtime - runtime_p->send_start_time) > 10))
			{//确认  //或者超过10秒未收到确认
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime ;
				step_cj = 2;
				beginwork = 0;
			}
			break;
		case 2://开始抄表
			if (runtime_p->format_Up.afn == 0x14 && runtime_p->format_Up.fn == 1  && inWaitFlag ==0)//收到请求抄读命令
			{
//				inWaitFlag = 1;
				beginwork = 1;//收到第一个请求抄读开始
				Addr_TSA(runtime_p->format_Up.afn14_f1_up.SlavePointAddr,&tsatmp);
				DbgPrintToFile1(31,"\n 请求地址 [ %02x-%02x-%02x%02x%02x%02x%02x%02x ] ",\
						tsatmp.addr[0],tsatmp.addr[1],tsatmp.addr[2],tsatmp.addr[3],\
						tsatmp.addr[4],tsatmp.addr[5],tsatmp.addr[6],tsatmp.addr[7]);
//				seqtmp = runtime_p->format_Up.info_up.Seq;//请求抄读帧SEQ
				sendlen = 0;
				nodetmp = NULL;
				nodetmp = getNodeByTSA(tsa_head,tsatmp);
				if( nodetmp != NULL )
				{
					sendlen = ProcessMeter(buf645,nodetmp);
					runtime_p->taskno = taskinfo.task_list[taskinfo.now_taski].taskId;
					runtime_p->fangAn.No = taskinfo.task_list[taskinfo.now_taski].fangan.No;
				}else
				{
					DbgPrintToFile1(31,"请求抄读电表不在集中器内");
					sendlen = -1;
				}
				flag= Echo_Frame( runtime_p,buf645,sendlen);//内部根据sendlen判断抄表 / 切表
				if (flag==0 || flag == 1)
					inWaitFlag = 0;
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime;
			}else if ( runtime_p->format_Up.afn == 0x06 && runtime_p->format_Up.fn == 2 )//收到返回抄表数据
			{
				inWaitFlag = 0;
				sendlen = AFN00_F01( &runtime_p->format_Up,runtime_p->sendbuf );//确认
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				fprintf(stderr,"存储 runtime_p->taskno = %d runtime_p->fangAn.No = %d",runtime_p->taskno,runtime_p->fangAn.No);
				SaveTaskData(runtime_p->format_Up, runtime_p->taskno, runtime_p->fangAn.No);
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime;
			}
			else if( abs(nowtime - runtime_p->send_start_time) > 100)
			{
				DbgPrintToFile1(31,"抄表过程，通讯超时,重启抄表");
				step_cj = 0;
			}

			JugeLastTime_SetZero(&taskinfo);//计算测量点所有任务上一次开始时刻

			break;
	}
	return TASK_PROCESS;
}

INT8U getTransCmdAddrProto(INT8U* cmdbuf, INT8U* addrtmp, INT8U* proto,INT8U len)
{
	if(NULL == cmdbuf || NULL == addrtmp || NULL == proto) {
		return 0;
	}
	int i=0;
	TSA tsatmp;
	*proto = 0;
	for(i=0;i<len;i++)
	{
		if (cmdbuf[i] == 0x68 && cmdbuf[i+7]==0x68)
		{
			memcpy(addrtmp, &cmdbuf[i+1], 6);
			Addr_TSA(addrtmp,&tsatmp);
			struct Tsa_Node *nodetmp=NULL;
			nodetmp = getNodeByTSA(tsa_head,tsatmp) ;
			if (nodetmp != NULL)
			{
				*proto = nodetmp->protocol;//dlt645-07 or 97
				DbgPrintToFile1(31,"透传TSA proto=%d",*proto);
				return 1;
			}
		}
	}
	INT8U Af = 0;
	for(i=0;i<len;i++)
	{
		if (cmdbuf[i] == 0x68)
		{
			Af = cmdbuf[i+4] & 0x0F;
			memcpy(addrtmp, &cmdbuf[i+5], Af+1);
			*proto = 0;//698.45
			DbgPrintToFile1(31,"透传 proto=%d",*proto);
			return 1;
		}
	}

	return 0;
}

INT8U Proxy_GetRequestList(RUNTIME_PLC *runtime_p,CJCOMM_PROXY *proxy,int* beginwork,time_t nowtime)
{
	static int obj_index = 0;
	int len645 =0,sendlen = 0 ;
	INT16U singleLen = 0 ,timeout = 20;
	INT8U tmpbuf[256]={} , nextFlag=0;
	struct Tsa_Node *nodetmp=NULL;
	FORMAT07 frame07;
	OAD oad1,oad2;

	//代理一个服务器的超时时间
	timeout = proxy->strProxyList.proxy_obj.objs[obj_index].onetimeout;
    if(timeout==0){
    	timeout = (proxy->strProxyList.timeout  > 0) ? proxy->strProxyList.timeout: 20;  //整个代理请求的超时时间
    }
	if (*beginwork==0 && proxy->isInUse==1)
	{
		DbgPrintToFile1(31,"Proxy_GetRequestList obj_index【 %d 】",obj_index);
		if(obj_index != proxy->strProxyList.num)
		{
			clearvar(runtime_p);
			nodetmp = getNodeByTSA(tsa_head,proxy->strProxyList.proxy_obj.objs[obj_index].tsa);
			if (nodetmp != NULL)
			{
				*beginwork = 1;
				oad1.OI = 0;
				oad2 = proxy->strProxyList.proxy_obj.objs[obj_index].oads[0];
				sendlen = buildProxyFrame(runtime_p,nodetmp,oad1,oad2);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				runtime_p->send_start_time = nowtime;
				DbgPrintToFile1(31,"发送代理 %d  timeout=%d",obj_index,proxy->strProxyList.proxy_obj.objs[obj_index].onetimeout);
			}else
			{
				obj_index = (obj_index+1)%proxy->strProxyList.num;//obj_index++;
				DbgPrintToFile1(31,"代理1 发现未知TSA ");
			}
		}
		else
		{
			DbgPrintToFile1(31,"代理1  obj_index=%d  allnum=%d",obj_index,proxy->strProxyList.num);
			*beginwork = 0;
			proxy->isInUse = 0;
			obj_index = 0;
//		代理１发送１次之后，再进入obj_index　＝　proxy->strProxyList.num就直接退出了
			return 5;	//进入100秒等待
		}
	}else if ((runtime_p->format_Up.afn == 0x13 && runtime_p->format_Up.fn == 1 ) && *beginwork==1)
	{
		DbgPrintToFile1(31,"代理1 收到数据");
		singleLen = 0;
		if (runtime_p->format_Up.afn13_f1_up.MsgLength > 0)
		{
			len645 = runtime_p->format_Up.afn13_f1_up.MsgLength;
			memcpy(buf645, runtime_p->format_Up.afn13_f1_up.MsgContent, len645);
			DbgPrintToFile1(31,"代理1 收到数据 报文长度=%d",runtime_p->format_Up.afn13_f1_up.MsgLength);
			if (analyzeProtocol07(&frame07, buf645, len645, &nextFlag) == 0)
			{
				DbgPrintToFile1(31,"代理1 收到数据 分析报文成功");
				pthread_mutex_lock(&mutex);
				memset(tmpbuf,0,sizeof(tmpbuf));
				singleLen = data07Tobuff698(frame07,tmpbuf);
				DbgPrintToFile1(31,"代理1 收到数据   to 698 len=%d",singleLen);
				if(singleLen > 0)
				{
					int dataindex = proxy->strProxyList.datalen;
					int addrlen = proxy->strProxyList.proxy_obj.objs[obj_index].tsa.addr[0]+1;
					memcpy(&proxy->strProxyList.data[dataindex],&proxy->strProxyList.proxy_obj.objs[obj_index].tsa.addr[0],addrlen);
					dataindex += addrlen;
					proxy->strProxyList.data[dataindex++] = proxy->strProxyList.proxy_obj.objs[obj_index].num;
					dataindex += create_OAD(0,&proxy->strProxyList.data[dataindex],proxy->strProxyList.proxy_obj.objs[obj_index].oads[0]);
					proxy->strProxyList.data[dataindex++] = 1;//data
					proxy->strProxyList.proxy_obj.objs[obj_index].dar = proxy_success;
					memcpy(&proxy->strProxyList.data[dataindex],tmpbuf,singleLen);
					dataindex += singleLen;
					proxy->strProxyList.datalen += dataindex;
				}
				pthread_mutex_unlock(&mutex);
			}else
			{
				DbgPrintToFile1(31,"代理1 收到数据 分析报文失败");
			}
		}
		if(singleLen==0)
			proxy->strProxyList.proxy_obj.objs[obj_index].dar = request_overtime;
		*beginwork = 0;
		obj_index = (obj_index+1)%proxy->strProxyList.num;//obj_index++;
		runtime_p->send_start_time = nowtime;
		DbgPrintToFile1(31,"代理1 完成一次代理 obj_index=%d num=%d",obj_index,proxy->strProxyList.num);
		proxyInUse.devUse.plcReady = 1;
		proxy->isInUse = 0;
		return 5;//进入100秒等待
	} else if ((abs(nowtime - runtime_p->send_start_time) > timeout) && *beginwork==1) {
		pthread_mutex_lock(&mutex);
		proxyInUse.devUse.plcReady = 1;
		proxy->isInUse = 0;
		proxy->strProxyList.datalen = 0;
		proxy->strProxyList.proxy_obj.transcmd.dar = request_overtime;
		pthread_mutex_unlock(&mutex);
		*beginwork = 0;
		clearvar(runtime_p);
		obj_index = (obj_index+1)%proxy->strProxyList.num;//obj_index++;
		runtime_p->send_start_time = nowtime;
		DbgPrintToFile1(31,"单次超时 obj_index=%d num=%d",obj_index,proxy->strProxyList.num);
		return 5;  //
	}else if(proxyInUse.devUse.plcNeed == 0 && *beginwork == 1) {
		DbgPrintToFile1(31,"总超时判断取消等待");
		clearvar(runtime_p);
		*beginwork = 0;
		obj_index = 0;
		pthread_mutex_lock(&mutex);
		proxyInUse.devUse.plcReady = 1;
		proxy->isInUse = 0;
		proxy->strProxyList.datalen = 0;
		proxy->strProxyList.proxy_obj.transcmd.dar = request_overtime;
		pthread_mutex_unlock(&mutex);
		return 5;	//进入100秒等待
	}
	return 2;
}
int JugeTransType(INT8U *buf,INT8U len,INT8U *buf_645)
{
	FORMAT3762 formatup;
	FORMAT07 frame07;
	BOOLEAN NEXTflag;
	int len07=0;
	int transType = 0;
	INT8U tmp3762[256];
	int ret=0;
	memset(tmp3762,0,256);
	memset(&broadtime,0,sizeof(broadtime));
	memset(buf_645,0,BUFSIZE645);
	memcpy(tmp3762,buf,len);

//	DbPrt1(31,"透传的报文:", (char *) tmp3762, len, NULL);
	transType = simpleAnaly3762(&formatup,tmp3762,len);
	if (transType==1)/*透传的是376.2报文*/
	{
		DbgPrintToFile1(31,"376.2【 afn=%02x   fn=%d 】",formatup.afn,formatup.fn);
		if ( (formatup.afn==0x05 && formatup.fn==3 ))
		{
			len07 = buf[14];
			memcpy(buf_645,&buf[15],len07);
			broadtime.len = len07;
			memcpy(broadtime.buf,buf_645,len07);
//			DbPrt1(31,"645:", (char *) buf_645, len07, NULL);
//			DbPrt1(31,"645:", (char *) &buf[14], 19, NULL);
			ret = analyzeProtocol07(&frame07, buf_645, len07, &NEXTflag);
			if ( ret == 1)
			{
				if (frame07.Ctrl==0x08)
				{
					broadtime.is = 1;
					broadtime.broadCastTime.Year = frame07.Time[5]+2000;
					broadtime.broadCastTime.Month = frame07.Time[4];
					broadtime.broadCastTime.Day = frame07.Time[3];
					broadtime.broadCastTime.Hour = frame07.Time[2];
					broadtime.broadCastTime.Minute = frame07.Time[1];
					broadtime.broadCastTime.Sec = frame07.Time[0];
					DbgPrintToFile1(31,"广播对时时间  %d-%d-%d %d:%d:%d ",broadtime.broadCastTime.Year,broadtime.broadCastTime.Month,
							broadtime.broadCastTime.Day,broadtime.broadCastTime.Hour,broadtime.broadCastTime.Minute,broadtime.broadCastTime.Sec);
					DbPrt1(31,"645对时报文:", (char *) &broadtime.buf, broadtime.len, NULL);
					return 1; /*376.2 AFN=05 fn=f3 启动广播*/
				}
			}
		}
		return 2;/*其它376.2报文*/
	}else
	{
		memcpy(buf645,tmp3762,len);
		ret = analyzeProtocol07(&frame07, buf645, len, &NEXTflag);
		if ( ret == 1)
		{
			DbgPrintToFile1(31,"透传的是645报文 控制码%02x",frame07.Ctrl);
			if (frame07.Ctrl==0x08)
			{
				broadtime.is = 1;
				broadtime.broadCastTime.Year = frame07.Time[5]+2000;
				broadtime.broadCastTime.Month = frame07.Time[4];
				broadtime.broadCastTime.Day = frame07.Time[3];
				broadtime.broadCastTime.Hour = frame07.Time[2];
				broadtime.broadCastTime.Minute = frame07.Time[1];
				broadtime.broadCastTime.Sec = frame07.Time[0];
				broadtime.len = len;
				memcpy(broadtime.buf,tmp3762,len);
				DbgPrintToFile1(31,"广播对时时间  %d-%d-%d %d:%d:%d ",broadtime.broadCastTime.Year,broadtime.broadCastTime.Month,
						broadtime.broadCastTime.Day,broadtime.broadCastTime.Hour,broadtime.broadCastTime.Minute,broadtime.broadCastTime.Sec);
				DbPrt1(31,"645对时报文:", (char *) &broadtime.buf, broadtime.len, NULL);
				return 1; /*376.2 AFN=05 fn=f3 启动广播*/
			}
			return 0;/*透传的是645报文*/
		}else
		{
			INT8U addr[6]={};
			memcpy(buf645,tmp3762,len);
			if (simpleProtocol698(buf645,len,addr)==1)/*透传的是645报文*/
			{
				return 0;/*透传的是645报文*/
			}
		}
	}
	return 3;/*透传的是其它报文*/
}
INT8U F209_TransRequest(RUNTIME_PLC *runtime_p,CJCOMM_PROXY *proxy,int* beginwork,time_t nowtime)
{
	INT8U addrtmp[10] = {0};//645报文中的目标地址
	int sendlen = 0;
	INT8U proto = 0;
	INT16U timeout = 20;
	INT8U datalen =0;
	int transType = 0;

	timeout = proxy->strProxyList.proxy_obj.f209Trans.overTime ;
	if (timeout>60 || timeout<=0)
		timeout = 20;

//	timeout = (proxy->strProxyList.proxy_obj.transcmd.revtimeout > 0) ? proxy->strProxyList.proxy_obj.transcmd.revtimeout: 20;

	if (*beginwork==0 && cjcommProxy_plc.isInUse==1) {//发送点抄
		*beginwork = 1;
		clearvar(runtime_p);
		datalen = cjcommProxy_plc.strProxyList.proxy_obj.f209Trans.buflen;
		transType = JugeTransType(cjcommProxy_plc.strProxyList.proxy_obj.f209Trans.transBuf,datalen,buf645);
		if (transType == 1)
		{
			DbgPrintToFile1(31,"代理内容为广播对时，需要切换到对时流程");
			//判断代理内容如果是广播对时，需要切换到对时流程
			cjcommProxy_plc.strProxyList.proxy_obj.f209Trans.dar = success;
			memset(cjcommProxy_plc.strProxyList.data,0,sizeof(cjcommProxy_plc.strProxyList.data));
			cjcommProxy_plc.strProxyList.data[0] = 0;
			cjcommProxy_plc.strProxyList.datalen = 1;
			proxyInUse.devUse.plcReady = 1;
			cjcommProxy_plc.isInUse = 0;
			clearvar(runtime_p);
			*beginwork = 0;
			return BROADCAST;
		}else if (transType == 2)
		{
			DbgPrintToFile1(31,"透传的是其它376.2报文");
			SendDataToCom(runtime_p->comfd, cjcommProxy_plc.strProxyList.proxy_obj.f209Trans.transBuf, datalen);
		}else
		{
			DbgPrintToFile1(31,"透传的是645报文");
			getTransCmdAddrProto(cjcommProxy_plc.strProxyList.proxy_obj.f209Trans.transBuf, addrtmp, &proto,datalen);
			memcpy(runtime_p->format_Down.addr.SourceAddr, runtime_p->masteraddr, 6);
			sendlen = AFN13_F1(&runtime_p->format_Down,runtime_p->sendbuf, addrtmp, proto, 0, \
					cjcommProxy_plc.strProxyList.proxy_obj.f209Trans.transBuf, datalen);
			SendDataToCom(runtime_p->comfd, runtime_p->sendbuf, sendlen );
		}
		DbgPrintToFile1(31,"发送 载波口 转发 （127方法)  timeout=%d",proxy->strProxyList.proxy_obj.f209Trans.overTime);
		runtime_p->send_start_time = nowtime;
	} else if ((runtime_p->format_Up.afn == 0x13 && runtime_p->format_Up.fn == 1 ) && *beginwork==1) {
		//收到应答数据，或超时10秒，
		pthread_mutex_lock(&mutex);
		if(runtime_p->format_Up.afn13_f1_up.MsgLength > 0) {

			datalen = (INT8U)runtime_p->format_Up.afn13_f1_up.MsgLength;
			cjcommProxy_plc.strProxyList.proxy_obj.f209Trans.dar = success;
			cjcommProxy_plc.strProxyList.data[0] = 0x09;	//octet-string
			cjcommProxy_plc.strProxyList.data[1] = datalen;
			memcpy(&cjcommProxy_plc.strProxyList.data[2],runtime_p->format_Up.afn13_f1_up.MsgContent,datalen);
			cjcommProxy_plc.strProxyList.datalen = datalen+2;

			DbgPrintToFile1(31,"返回数据长度 13-1 up.length=%d      [%02x  %02x  %02x  %02x   ]",runtime_p->format_Up.length,
					runtime_p->dealbuf[0],runtime_p->dealbuf[1],runtime_p->dealbuf[2],runtime_p->dealbuf[3]);

		} else {
			cjcommProxy_plc.strProxyList.proxy_obj.f209Trans.dar = request_overtime;
			cjcommProxy_plc.strProxyList.datalen = 0;
		}
		runtime_p->send_start_time = nowtime;
		memset(&runtime_p->format_Up, 0, sizeof(runtime_p->format_Up));
		proxyInUse.devUse.plcReady = 1;
		cjcommProxy_plc.isInUse = 0;
		*beginwork = 0;
		pthread_mutex_unlock(&mutex);
		DbgPrintToFile1(31,"收到127点抄返回数据 DAR=%d ",cjcommProxy_plc.strProxyList.proxy_obj.f209Trans.dar);
		return 5;//进入100秒等待
	}else if ( runtime_p->format_Up.afn<=0x15 && runtime_p->format_Up.fn!=0 &&  *beginwork==1)//国网需要将收到报文直接返回主站
	{
		pthread_mutex_lock(&mutex);
			cjcommProxy_plc.strProxyList.proxy_obj.f209Trans.dar = success;
			cjcommProxy_plc.strProxyList.data[0] = 0x09;	//octet-string
			datalen = runtime_p->format_Up.length;
			cjcommProxy_plc.strProxyList.data[1] = datalen;
			memcpy(&cjcommProxy_plc.strProxyList.data[2],&runtime_p->dealbuf,datalen);
			cjcommProxy_plc.strProxyList.datalen = datalen + 2;
			proxyInUse.devUse.plcReady = 1;
			cjcommProxy_plc.isInUse = 0;
		pthread_mutex_unlock(&mutex);
		*beginwork = 0;
		runtime_p->send_start_time = nowtime;
		memset(&runtime_p->format_Up, 0, sizeof(runtime_p->format_Up));
		DbgPrintToFile1(31,"收到127返回点抄数据  datalen=%d",cjcommProxy_plc.strProxyList.datalen);
		return 5;//进入100秒等待
	}
	else if ((abs(nowtime - runtime_p->send_start_time) > timeout) && *beginwork==1) {
		//代理超时后, 放弃本次操作, 上报超时应答
		pthread_mutex_lock(&mutex);
		cjcommProxy_plc.strProxyList.proxy_obj.transcmd.dar = request_overtime;
		cjcommProxy_plc.strProxyList.data[0] = 0;	//数据为　NULL
		cjcommProxy_plc.strProxyList.datalen = 1;
		*beginwork = 0;
		proxyInUse.devUse.plcReady = 1;
		cjcommProxy_plc.isInUse = 0;
		runtime_p->send_start_time = nowtime;
		pthread_mutex_unlock(&mutex);
		DbgPrintToFile1(31,"单次127点抄超时 timeout=%d",timeout);
		return 5;//进入100秒等待
	}else if(proxyInUse.devUse.plcNeed == 0 && *beginwork == 1)
	{
		*beginwork = 0;
		cjcommProxy_plc.isInUse = 0;
		clearvar(runtime_p);
		runtime_p->send_start_time = nowtime;
		DbgPrintToFile1(31,"总超时判断取消等待");
		return 5;//进入100秒等待
	}
	return 3;
}

INT8U Proxy_TransCommandRequest(RUNTIME_PLC *runtime_p,CJCOMM_PROXY *proxy,int* beginwork,time_t nowtime)
{
	INT8U addrtmp[10] = {0};//645报文中的目标地址
	int sendlen = 0;
	INT8U proto = 0;
	INT16U timeout = 20;
	INT8U datalen =0;
	INT16U	dindex = 0;
	int transType = 0;

	timeout = proxy->strProxyList.proxy_obj.transcmd.revtimeout ;
	if(timeout>60 || timeout<=0)
		timeout = 20;

	if (*beginwork==0 && cjcommProxy_plc.isInUse==1) {//发送点抄
		*beginwork = 1;
		clearvar(runtime_p);
		transType = JugeTransType(cjcommProxy_plc.strProxyList.proxy_obj.transcmd.cmdbuf,cjcommProxy_plc.strProxyList.proxy_obj.transcmd.cmdlen,buf645);
		if (transType == 1)
		{
			DbgPrintToFile1(31,"代理内容为广播对时，需要切换到对时流程");
			//判断代理内容如果是广播对时，需要切换到对时流程
			memset(cjcommProxy_plc.strProxyList.data,0,sizeof(cjcommProxy_plc.strProxyList.data));
			cjcommProxy_plc.strProxyList.proxy_obj.transcmd.dar = success;
			cjcommProxy_plc.strProxyList.data[0] = 1;//trans_result :有数据
			cjcommProxy_plc.strProxyList.data[1] = 0;//数据NULL
			cjcommProxy_plc.strProxyList.datalen = 2;
			proxyInUse.devUse.plcReady = 1;
			cjcommProxy_plc.isInUse = 0;
			clearvar(runtime_p);
			*beginwork = 0;
			return BROADCAST;
		}else if (transType == 2)
		{
			DbgPrintToFile1(31,"透传的是其它376.2报文");
			SendDataToCom(runtime_p->comfd, cjcommProxy_plc.strProxyList.proxy_obj.transcmd.cmdbuf, cjcommProxy_plc.strProxyList.proxy_obj.transcmd.cmdlen);
		}else if (transType == 3)
		{
			DbgPrintToFile1(31,"透传的是其它报文");
			getTransCmdAddrProto(cjcommProxy_plc.strProxyList.proxy_obj.transcmd.cmdbuf, addrtmp, &proto,cjcommProxy_plc.strProxyList.proxy_obj.transcmd.cmdlen);
			memcpy(runtime_p->format_Down.addr.SourceAddr, runtime_p->masteraddr, 6);
			sendlen = AFN13_F1(&runtime_p->format_Down,runtime_p->sendbuf, addrtmp, 0, 0,
					cjcommProxy_plc.strProxyList.proxy_obj.transcmd.cmdbuf, cjcommProxy_plc.strProxyList.proxy_obj.transcmd.cmdlen);
			SendDataToCom(runtime_p->comfd, runtime_p->sendbuf, sendlen );
		}else {
			DbgPrintToFile1(31,"透传的是645报文");
			getTransCmdAddrProto(cjcommProxy_plc.strProxyList.proxy_obj.transcmd.cmdbuf, addrtmp, &proto,cjcommProxy_plc.strProxyList.proxy_obj.transcmd.cmdlen);
			memcpy(runtime_p->format_Down.addr.SourceAddr, runtime_p->masteraddr, 6);
			sendlen = AFN13_F1(&runtime_p->format_Down,runtime_p->sendbuf, addrtmp, proto, 0,
					cjcommProxy_plc.strProxyList.proxy_obj.transcmd.cmdbuf, cjcommProxy_plc.strProxyList.proxy_obj.transcmd.cmdlen);
			SendDataToCom(runtime_p->comfd, runtime_p->sendbuf, sendlen );
		}
		DbgPrintToFile1(31,"发送 plc 代理 command ");
		runtime_p->send_start_time = nowtime;
	} else if ((runtime_p->format_Up.afn == 0x13 && runtime_p->format_Up.fn == 1 ) && *beginwork==1) {
		pthread_mutex_lock(&mutex);
		if(runtime_p->format_Up.afn13_f1_up.MsgLength > 0) {
			INT16U tIndex = 0;
			INT16U starttIndex = 0;
			for(tIndex = 0;tIndex < runtime_p->format_Up.afn13_f1_up.MsgLength;tIndex++) {//去掉前导符
				if(runtime_p->format_Up.afn13_f1_up.MsgContent[tIndex]!=0x68 &&
				   runtime_p->format_Up.afn13_f1_up.MsgContent[tIndex + 7]!=0x68 ) {
					continue;
				} else {
					starttIndex = tIndex;
					break;
				}
			}
			cjcommProxy_plc.strProxyList.proxy_obj.transcmd.dar = success;
			dindex = 0;
			cjcommProxy_plc.strProxyList.data[dindex++] = 1;

			if(getZone("GW")==0) {
				datalen = runtime_p->format_Up.length;
				cjcommProxy_plc.strProxyList.data[dindex++] = datalen;
				memcpy(&cjcommProxy_plc.strProxyList.data[2],runtime_p->dealbuf,datalen);
				dindex += datalen;
			}else
			{
				datalen = runtime_p->format_Up.afn13_f1_up.MsgLength - starttIndex;
				dindex += fillStringLen(&cjcommProxy_plc.strProxyList.data[dindex],datalen);
				memcpy(&cjcommProxy_plc.strProxyList.data[dindex],&runtime_p->format_Up.afn13_f1_up.MsgContent[starttIndex],datalen);
				dindex = dindex + datalen;
				DEBUG_BUFF(runtime_p->format_Up.afn13_f1_up.MsgContent, datalen);
			}
			cjcommProxy_plc.strProxyList.datalen = dindex;

		} else {
			cjcommProxy_plc.strProxyList.proxy_obj.transcmd.dar = request_overtime;
			cjcommProxy_plc.strProxyList.datalen = 0;
		}
		clearvar(runtime_p);
		proxyInUse.devUse.plcReady = 1;
		cjcommProxy_plc.isInUse = 0;
		*beginwork = 0;
		pthread_mutex_unlock(&mutex);
		runtime_p->send_start_time = nowtime;
		DbgPrintToFile1(31,"收到代理7点抄数据");
		return 5;//进入100秒等待
	}
	else if ( runtime_p->format_Up.afn!=0 && runtime_p->format_Up.fn!=0 &&  *beginwork==1)//国网需要将收到报文直接返回主站
	{
		pthread_mutex_lock(&mutex);
			cjcommProxy_plc.strProxyList.proxy_obj.transcmd.dar = success;
			cjcommProxy_plc.strProxyList.data[0] = 1;
			datalen = runtime_p->format_Up.length;
			cjcommProxy_plc.strProxyList.data[1] = datalen;
			memcpy(&cjcommProxy_plc.strProxyList.data[2],&runtime_p->dealbuf,datalen);
			cjcommProxy_plc.strProxyList.datalen = datalen + 2;
			proxyInUse.devUse.plcReady = 1;
			cjcommProxy_plc.isInUse = 0;
		pthread_mutex_unlock(&mutex);
		*beginwork = 0;
		clearvar(runtime_p);
		runtime_p->send_start_time = nowtime;
		DbgPrintToFile1(31,"收到点抄数据");
		return 5;//进入100秒等待
	}
	else if ((abs(nowtime - runtime_p->send_start_time) > timeout) && *beginwork==1) {
		//代理超时后, 放弃本次操作, 上报超时应答
		pthread_mutex_lock(&mutex);
		cjcommProxy_plc.isInUse = 0;
		proxyInUse.devUse.plcReady = 1;
		cjcommProxy_plc.strProxyList.datalen = 0;
		cjcommProxy_plc.strProxyList.proxy_obj.transcmd.dar = request_overtime;
		pthread_mutex_unlock(&mutex);
		*beginwork = 0;
		clearvar(runtime_p);
		runtime_p->send_start_time = nowtime;
		DbgPrintToFile1(31,"单次点抄超时");
		return 5;//进入100秒等待
	}else if(proxyInUse.devUse.plcNeed == 0 && *beginwork == 1)
	{
		DbgPrintToFile1(31,"总超时判断取消等待");
		clearvar(runtime_p);
		*beginwork = 0;
		runtime_p->send_start_time = nowtime;
		cjcommProxy_plc.isInUse = 0;
		proxyInUse.devUse.plcReady = 1;
		cjcommProxy_plc.strProxyList.datalen = 0;
		cjcommProxy_plc.strProxyList.proxy_obj.transcmd.dar = request_overtime;
		return 5;//进入100秒等待
	}
	return 2;
}
INT8U Proxy_Gui(RUNTIME_PLC *runtime_p,GUI_PROXY *proxy,int* beginwork,time_t nowtime)
{
	static struct Tsa_Node *nodetmp=NULL;
	int sendlen=0;
	if (*beginwork==0  && proxy->strProxyMsg.port.OI == 0xf209 && proxy->isInUse ==1 )
	{//发送点抄
		DbgPrintToFile1(31,"dealGuiRead 处理液晶点抄 :%02x%02x%02x%02x%02x%02x%02x%02x 波特率=%d protocol=%d 端口号=%04x%02x%02x 规约类型=%d 数据标识=%04x"
				,proxy->strProxyMsg.addr.addr[0],proxy->strProxyMsg.addr.addr[1],proxy->strProxyMsg.addr.addr[2],proxy->strProxyMsg.addr.addr[3]
				,proxy->strProxyMsg.addr.addr[4],proxy->strProxyMsg.addr.addr[5],proxy->strProxyMsg.addr.addr[6],proxy->strProxyMsg.addr.addr[7]
				,proxy->strProxyMsg.baud,proxy->strProxyMsg.protocol,proxy->strProxyMsg.port.OI,proxy->strProxyMsg.port.attflg,proxy->strProxyMsg.port.attrindex
				,proxy->strProxyMsg.protocol,proxy->strProxyMsg.oi);
		*beginwork = 1;
		nodetmp = NULL;
		nodetmp = getNodeByTSA(tsa_head,proxy->strProxyMsg.addr) ;
		clearvar(runtime_p);
		if( nodetmp != NULL )
		{
			DbgPrintToFile1(31,"发送点抄报文");
			memcpy(runtime_p->format_Down.addr.SourceAddr,runtime_p->masteraddr,6);
			OAD oad1,oad2;
			oad1.OI = 0;
			oad2.OI = proxy->strProxyMsg.oi;
			oad2.attflg = 0x02;
			oad2.attrindex = 0;
			sendlen = buildProxyFrame(runtime_p,nodetmp,oad1,oad2);
			SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
		}
		runtime_p->send_start_time = nowtime;
	}
	else if ((runtime_p->format_Up.afn == 0x13 && runtime_p->format_Up.fn == 1 ))
	{//收到应答数据，或超时10秒，
		proxy->isInUse = 0;
		*beginwork = 0;
		saveProxyData(runtime_p->format_Up,nodetmp);
		memset(&runtime_p->format_Up,0,sizeof(runtime_p->format_Up));
		runtime_p->send_start_time = nowtime;
		DbgPrintToFile1(31,"收到点抄数据");
		return 5;//进入100秒等待
	}else if ((abs(nowtime - runtime_p->send_start_time) > 30  ) && *beginwork==1)
	{
		proxy->isInUse = 0;
		*beginwork = 0;
		runtime_p->send_start_time = nowtime;
		DbgPrintToFile1(31,"单次点抄超时");
		return 5;//进入100秒等待
	}
	return 1;
}
int doProxy(RUNTIME_PLC *runtime_p)
{
	static int step_cj = 0, beginwork=0;
	int sendlen=0;
	time_t nowtime = time(NULL);
	switch( step_cj )
	{
		case 0://暂停抄表
			if (abs( nowtime - runtime_p->send_start_time )> 20)
			{
				DbgPrintToFile1(31,"暂停抄表4");
				DEBUG_TIME_LINE("暂停抄表");
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime ;
				sendlen = AFN12_F2(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
			}else if(runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1)
			{//确认
				clearvar(runtime_p);
				DEBUG_TIME_LINE("暂停抄表已确认");
				step_cj = 5;
				runtime_p->send_start_time = nowtime;
			}
			break;
		case 1://开始监控载波从节点
			step_cj = Proxy_Gui(runtime_p, &cjGuiProxy_plc, &beginwork, nowtime);
			break;
		case 2://处理主站代理
			DbgPrintToFile1(31,"处理主站代理 类型=%d",cjcommProxy_plc.strProxyList.proxytype);
			switch(cjcommProxy_plc.strProxyList.proxytype) {
				case ProxyGetRequestList:
					step_cj = Proxy_GetRequestList(runtime_p, &cjcommProxy_plc, &beginwork, nowtime);
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
					step_cj = Proxy_TransCommandRequest(runtime_p, &cjcommProxy_plc, &beginwork, nowtime);
					break;
				default:
					proxyInUse.devUse.plcReady = 1;
					cjcommProxy_plc.isInUse = 0;
					step_cj = 4;
			}
			break;
		case 3://F209——127 载波端口数据转发
			step_cj = F209_TransRequest(runtime_p, &cjcommProxy_plc, &beginwork, nowtime);
			break;
		case 5://AFN13-1 等待状态（超时统一处理默认是100秒)
			beginwork = 0;
			if(cjcommProxy_plc.isInUse == 1)
			{
				if(cjcommProxy_plc.strProxyList.proxytype == F209TransCommandAction)
				{
					DbgPrintToFile1(31,"进入F209透传>>>");
					step_cj = 3;
					break;
				}else
				{
					DbgPrintToFile1(31,"进入主站代理>>>");
					step_cj = 2;
					break;
				}
			} else if (cjGuiProxy_plc.isInUse == 1)
			{
				DbgPrintToFile1(31,"进入液晶点抄>>>");
				step_cj = 1;
				break;
			}
			int outtime = abs( nowtime - runtime_p->send_start_time) ;
			if (outtime%5==0)
				DbgPrintToFile1(31,"计时%d",outtime);
			if(outtime > 100  || getZone("GW")==0)
			{
				//最后一次代理操作后100秒, 才恢复抄读
				DbgPrintToFile1(31,"100秒超时");
				clearvar(runtime_p);
				step_cj = 4;
			}
			break;
		case 4://恢复抄表
			if (runtime_p->state_bak == TASK_PROCESS )
			{
				if(  runtime_p->modeFlag==1)
				{
					clearvar(runtime_p);
					step_cj = 0;
					beginwork = 0;
					DbgPrintToFile1(31,"集中器主导返回到状态%d",runtime_p->state_bak);
					return(runtime_p->state_bak);
				}
				if (abs( nowtime - runtime_p->send_start_time) > 20)
				{
					DbgPrintToFile1(31,"恢复抄表 runtime_p->modeFlag=%d",runtime_p->modeFlag);
					clearvar(runtime_p);
					runtime_p->send_start_time = nowtime ;
					sendlen = AFN12_F3(&runtime_p->format_Down,runtime_p->sendbuf);
					SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
					memset(&runtime_p->format_Up,0,sizeof(runtime_p->format_Up));
				}else if((runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1) ||
						(abs( nowtime - runtime_p->send_start_time) > 10))
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
		default :
			clearvar(runtime_p);
			step_cj = 0;
			beginwork = 0;
			return BROADCAST;
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
	int sendlen=0, searchlen=0;

	time_t nowtime = time(NULL);
	if (runtime_p->nowts.Hour==23 || (nowtime-beginSearchTime)>10800)
	{
		if(step_cj==0)
			DbgPrintToFile1(31,"23点结束搜表，返回状态 %d",runtime_p->state_bak);
		step_cj = 0;
		beginwork = 0;
		clearvar(runtime_p);
		return(runtime_p->state_bak);
	}

	switch( step_cj )
	{
		case 0://暂停抄读
			if ( abs(nowtime - runtime_p->send_start_time) > 20)
			{
				DbgPrintToFile1(31,"暂停抄表1");
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
			if ( abs(nowtime - runtime_p->send_start_time) > 20  && beginwork==0)
			{
				DbgPrintToFile1(31,"启动广播");
				clearvar(runtime_p);
				memcpy(runtime_p->format_Down.addr.SourceAddr,runtime_p->masteraddr,6);
				sendlen = startSearch(&runtime_p->format_Down,runtime_p->sendbuf);
				DbgPrintToFile1(31," sendlen=%d",sendlen);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				runtime_p->send_start_time = nowtime ;
				beginwork = 1;
			}
//			else if (runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1 && beginwork ==0)//收到确认
//			{
//				DbgPrintToFile1(31,"收到确认");
//				runtime_p->send_start_time = nowtime ;
//				DEBUG_TIME_LINE("\nruntime_p->send_start_time = %ld",runtime_p->send_start_time);
//				beginwork = 1;
//			}
			else if (beginwork == 1)
			{
				DEBUG_TIME_LINE("\nruntime_p->send_start_time = %ld   nowtime=%ld",runtime_p->send_start_time,nowtime);
				if (abs(nowtime - runtime_p->send_start_time) > 30 )
				{
					DbgPrintToFile1(31,"等待到时间");
					clearvar(runtime_p);
					step_cj = 2;
				}else
				{
					if ((abs(nowtime-runtime_p->send_start_time)) % 10 == 0)
					{
						DbgPrintToFile1(31,"等待120秒... (%ld)",nowtime-runtime_p->send_start_time);
						sleep(1);
					}
				}
			}
			break;
		case 2://激活从节点注册
			if ( abs(nowtime - runtime_p->send_start_time) > 20)
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
			if (search_i < SERACH_PARA_NUM)
				searchlen = search6002.attr9[search_i].searchLen ;
			else
				searchlen = search6002.startSearchLen;
			if(searchlen ==0)
				searchlen = 10;

			if ( abs(nowtime - runtime_p->send_start_time) < (searchlen *60) )
			{
				if ((runtime_p->format_Up.afn == 0x06 && runtime_p->format_Up.fn == 4)||
					(runtime_p->format_Up.afn == 0x06 && runtime_p->format_Up.fn == 1))
				{
					saveSerchMeter(runtime_p->format_Up);
					sendlen = AFN00_F01( &runtime_p->format_Up,runtime_p->sendbuf );//确认
					clearvar(runtime_p);
					SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
					clearvar(runtime_p);
					runtime_p->send_start_time = nowtime;
				}
				if ((nowtime-runtime_p->send_start_time) % 10 == 0)
				{
					DbgPrintToFile1(31,"等待 %d 秒... (%ld)",searchlen*60,nowtime-runtime_p->send_start_time);
					sleep(1);
				}
			}else
			{
				DbgPrintToFile1(31,"搜表等待延时结束");
				beginwork =0;
				step_cj = 4;
				clearvar(runtime_p);
//				return(runtime_p->state_bak);
			}
			break;
		case 4://判断是否结束
			if ( abs(nowtime - runtime_p->send_start_time) > 20 && beginwork==0)
			{
				sendlen = AFN10_F4(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				runtime_p->send_start_time = nowtime;
				beginwork = 1;
			}else if ( runtime_p->format_Up.afn == 0x10 && runtime_p->format_Up.fn == 4 &&  beginwork ==1)
			{
				if ((runtime_p->format_Up.afn10_f4_up.FinishFlag==1) && (runtime_p->format_Up.afn10_f4_up.WorkFlag==0))
				{
					DbgPrintToFile1(31,"搜表完成!!");
					beginwork =0;
					step_cj = 4;
//					return(runtime_p->state_bak);
					return (TASK_PROCESS);
				}
				DbgPrintToFile1(31,"搜表未完成，继续等待");
				step_cj = 3;
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime;
			}else if (abs(nowtime - runtime_p->send_start_time) > 20 && beginwork==1)
			{
				beginwork =0;
				DbgPrintToFile1(31,"超时");
				step_cj = 3;
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime;
			}
			break;
	}
	return METER_SEARCH;
}

int MyTimeJuge(INT8U *timestr)
{
	TS broadcastTime;
	TS nowts1;

	TSGet(&nowts1);
	TSGet(&broadcastTime);
	broadcastTime.Year = nowts1.Year;
	broadcastTime.Month = nowts1.Month;
	broadcastTime.Day = nowts1.Day;
	broadcastTime.Hour = timestr[0];
	broadcastTime.Minute = timestr[1];
	broadcastTime.Sec = timestr[2];

	time_t time1 = tmtotime_t(broadcastTime);
	time_t time2 = tmtotime_t(nowts1);
	time_t time3 = time2 - time1;

	if (abs(time3) < 2)
	{
		DbgPrintToFile1(31,"time3=%d",time3);
		return 1;
	}
	return 0;
}
int dateJudge(TS *old ,TS *new)
{
//	if(old->Day!=new->Day || old->Year!=new->Year || old->Month!=new->Month)
//	{
//		memcpy(old,new,sizeof(TS));
//		return 1;
//	}
	if ((new->Hour==23 && new->Minute>=55 )   && (old->Day!=new->Day))
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
		runtime_p->deallen = datalen;
		analyzeProtocol3762(&runtime_p->format_Up,runtime_p->dealbuf,datalen);
		fprintf(stderr,"\nafn=%02x   fn=%d 返回",runtime_p->format_Up.afn ,runtime_p->format_Up.fn);
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

void delplcrecord()
{
	sync();
	sleep(1);
	unlink("/nand/para/plcrecord.par");
	unlink("/nand/para/plcrecord.bak");
	sleep(5);
	sync();
	if(access("/nand/para/plcrecord.par",F_OK)==0)
	{
		system("rm -f /nand/para/plcrecord.par  /nand/para/plcrecord.bak");
	}
	if(access("/nand/para/plcrecord.par",F_OK)==0)
	{
		DbgPrintToFile1(31,"初始化任务参数失败 plcrecord.par");
		unlink("/nand/para/plcrecord.par");
	}
	if(access("/nand/para/plcrecord.bak",F_OK)==0)
	{
		DbgPrintToFile1(31,"初始化任务参数失败 plcrecord.bak");
		unlink("/nand/para/plcrecord.bak");
	}
	sync();
}
int stateJuge(int nowdstate,MY_PARA_COUNTER *mypara_p,RUNTIME_PLC *runtime_p,int *startFlg)
{
	int state = nowdstate;
	int i=0;
	int dateChg = 0;
	int pointChg = 0;

	if ((runtime_p->format_Up.afn==0x06) && (runtime_p->format_Up.fn==5))//AFN= 06 FN= F5  路由上报从节点事件
	{
#if 1
		if(runtime_p->state != AUTO_REPORT) {
			runtime_p->state_bak = runtime_p->state;
			DbgPrintToFile1(31,"载波主动上报  备份状态 %d",runtime_p->state_bak);
		}
		state = AUTO_REPORT;
		runtime_p->state = state;
		return state;
#endif
	}
	//-----------------------------------------------------------------------------------
	dateChg = dateJudge(&runtime_p->oldts,&runtime_p->nowts);
	if (JProgramInfo->oi_changed.oi6000 != mypara_p->my6000 )
	{
		DbgPrintToFile1(31,"档案参数变更1");
		mypara_p->my6000  = JProgramInfo->oi_changed.oi6000;
//		pointChg = 1;
//		initTaskData(&taskinfo);
		memcpy(&taskinfo,&taskinfo_bak,sizeof(taskinfo));
		delplcrecord();
		freeList(tsa_head);
		freeList(tsa_zb_head);
		tsa_head = NULL;
		tsa_zb_head = NULL;
		tsa_count = initTsaList(&tsa_head);
		tsa_print(tsa_head,tsa_count);

		runtime_p->state_bak = runtime_p->state;
		if (runtime_p->modeFlag >1 || runtime_p->modeFlag<0)
			state = DATE_CHANGE;
		else
			state = SLAVE_COMP;
		runtime_p->state = state;
		runtime_p->redo = 1;  //初始化之后需要重启抄读
		return state;
	}
	if ( dateChg == 1 ||  *startFlg == 1 )
	{
		DbgPrintToFile1(31,"初始化路由");
		*startFlg = 0;
		runtime_p->initflag = 1;
		runtime_p->state_bak = runtime_p->state;
		state = DATE_CHANGE;
		broadFlag_ts.Day  = 0;
		runtime_p->state = state;
		runtime_p->redo = 1;  //初始化之后需要重启抄读

		if(pointChg==1 || dateChg == 1 || access("/nand/para/plcrecord.par",F_OK) != 0)
		{//测量点参数变更  或 记录文件不存在  或   日期变更需要重新初始化任务
			DbgPrintToFile1(31,"初始化默认任务参数，清除抄表记录");
			initTaskData(&taskinfo);
			delplcrecord();
			PrintTaskInfo2(&taskinfo);
		}
		return state;
	}
	if (JProgramInfo->oi_changed.oi6012 != mypara.my6012)
	{
		//任务变更
		sleep(10);//需要与全局任务数组保持同步更新
		initTaskData(&taskinfo);
		delplcrecord();
		DbgPrintToFile1(31,"任务重新初始化");
		PrintTaskInfo2(&taskinfo);
		runtime_p->redo = 1;  //初始化之后需要重启抄读
		mypara.my6012 = JProgramInfo->oi_changed.oi6012 ;
	}

	if ((runtime_p->nowts.Hour==23 && runtime_p->nowts.Minute==59) || (runtime_p->nowts.Hour==0 && runtime_p->nowts.Minute==0))
		return state;  //23点59分--0点0分之间不进行任务判断（准备跨日初始化）

	//---------------------------------------------------------------------------------------------------------------------------
	if (JProgramInfo->oi_changed.oi6002 != mypara.my6002 )
	{
		mypara.my6002 = JProgramInfo->oi_changed.oi6002 ;
		initSearchMeter(&search6002);//重新读取搜表参数
		if(search6002.startSearchFlg == 1)
		{
			runtime_p->state_bak = runtime_p->state;
			runtime_p->state = METER_SEARCH;
			runtime_p->redo = 2;  //搜表后需要恢复抄读
			search6002.startSearchFlg = 0;			//启动立即搜表
			search_i = 0xff;
			saveCoverClass(0x6002,0,&search6002,sizeof(CLASS_6002),para_vari_save);
			runtime_p->redo = 2;  //搜表后需要恢复抄读
			beginSearchTime = time(NULL);
			DbgPrintToFile1(31,"立即启动搜表 时长=%d 分钟",search6002.startSearchLen);
			return METER_SEARCH;
		}
	}
	for(i=0; i<search6002.attr9_num;i++)
	{
		if (search6002.attr8.enablePeriodFlg==1 && MyTimeJuge(search6002.attr9[i].startTime)==1 )
		{
			sleep(3);
			runtime_p->state_bak = runtime_p->state;
			runtime_p->state = METER_SEARCH;
			clearvar(runtime_p);
			search_i = i;
			runtime_p->redo = 2;  //搜表后需要恢复抄读
			beginSearchTime = time(NULL);
			DbgPrintToFile1(31,"%d-%d-%d 点启动搜表",search6002.attr9[i].startTime[0],search6002.attr9[i].startTime[1],search6002.attr9[i].startTime[2]);
			return METER_SEARCH;
		}
	}
	//-------------------------------------------------------------------------------------------------------------------------
	if (cjGuiProxy_plc.isInUse ==1 && cjGuiProxy_plc.strProxyMsg.port.OI == 0xf209 && state!=DATE_CHANGE && state!=DATA_REAL)
	{	//出现液晶点抄载波表标识，并且不在初始化和点抄状态
		DbgPrintToFile1(31,"载波收到点抄消息 需要处理 %04x ",cjguiProxy.strProxyMsg.port.OI);
		runtime_p->state_bak = runtime_p->state;
		runtime_p->state = DATA_REAL;
		clearvar(runtime_p);
		runtime_p->redo = 2;	//点抄后需要恢复抄读
		return DATA_REAL;
	}

//	DbgPrintToFile1(31,"plcNeed: %d, isInUse: %d, plcReady: %d  state:%d",proxyInUse.devUse.plcNeed, cjcommProxy_plc.isInUse, proxyInUse.devUse.plcReady,state);

	if (proxyInUse.devUse.plcNeed == 1 && \
			cjcommProxy_plc.isInUse ==1 && \
			state!=DATE_CHANGE && \
			state!=DATA_REAL)
	{	//出现代理标识，并且不在初始化和点抄状态
		DbgPrintToFile1(31,"载波收到点代理请求, plcNeed: %d, plcReady: %d",proxyInUse.devUse.plcNeed, proxyInUse.devUse.plcReady);
		runtime_p->state_bak = runtime_p->state;
		runtime_p->state = DATA_REAL;
		clearvar(runtime_p);
//		runtime_p->redo = 2;	//点抄后需要恢复抄读
		fprintf(stderr,"\n载波收到 F209 点抄");
		return DATA_REAL;
	}

	if (broadcase4204.enable==1 &&
		MyTimeJuge(broadcase4204.startime)==1 &&
		broadFlag_ts.Day != runtime_p->nowts.Day )/*广播对时开始的条件 1：4204开启有效  2：到广播对时时间  3：当日未进行过对时 */
	{
		runtime_p->state_bak = runtime_p->state;
		runtime_p->redo = 2;	//广播后需要恢复抄读
		clearvar(runtime_p);
		broadFlag_ts.Day  = runtime_p->nowts.Day;
		return BROADCAST;
	}

	if (state == NONE_PROCE && taskinfo.task_n>0 && tsa_count > 0)
	{
		state = TASK_PROCESS;
		runtime_p->state = TASK_PROCESS;
	}

	return state;
}


int saveF13_F1Data(FORMAT3762 format_3762_Up)
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

int doTask_by_jzq(RUNTIME_PLC *runtime_p)
{
	static int step_cj = 0, beginwork=0;
	static int inWaitFlag = 0 ;
	int sendlen=0;
	struct Tsa_Node *nodetmp=NULL;
	INT8U addrtmp[6]={};
	time_t nowtime = time(NULL);

	switch( step_cj )
	{
		case 0://暂停抄表
			if (abs( nowtime - runtime_p->send_start_time) > 20)
			{
				DbgPrintToFile1(31,"\n暂停抄表2");
				clearvar(runtime_p);
				runtime_p->redo = 0;
				runtime_p->send_start_time = nowtime ;
				sendlen = AFN12_F2(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
			}else if(runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1)
			{//确认
				clearvar(runtime_p);
				runtime_p->send_start_time = 0;
				step_cj = 1;
				inWaitFlag = 0;
				beginwork = 0;
			}
			break;
		case 1://开始抄表
			if ( inWaitFlag==0)
			{
				nodetmp = (struct Tsa_Node *)ProcessMeter_byJzq(buf645,addrtmp,&sendlen );//下发 AFN_13_F1 找到一块需要抄读的表，抄读
				if (sendlen>0 && nodetmp!=NULL)
				{
					DbPrt1(31,"TS:", (char *) buf645, sendlen, NULL);
#if 1
					addrtmp[5] = nodetmp->tsa.addr[2];
					addrtmp[4] = nodetmp->tsa.addr[3];
					addrtmp[3] = nodetmp->tsa.addr[4];
					addrtmp[2] = nodetmp->tsa.addr[5];
					addrtmp[1] = nodetmp->tsa.addr[6];
					addrtmp[0] = nodetmp->tsa.addr[7];
#endif
					memcpy(runtime_p->format_Down.addr.SourceAddr, runtime_p->masteraddr, 6);
					sendlen = AFN13_F1(&runtime_p->format_Down,runtime_p->sendbuf,addrtmp, nodetmp->protocol, 0, buf645, sendlen);
					DbgPrintToFile1(31,"sendlen=%d  protocol=%d",sendlen,nodetmp->protocol);
					SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
					runtime_p->send_start_time = nowtime;
				}
				inWaitFlag = 1;
			}else if( runtime_p->format_Up.afn == 0x13 && runtime_p->format_Up.fn == 1 && inWaitFlag==1 )
			{
				DbgPrintToFile1(31,"集中器主导流程_收数据");
				saveF13_F1Data(runtime_p->format_Up);
				SaveTaskData(runtime_p->format_Up, runtime_p->taskno, runtime_p->fangAn.No);
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime;
				inWaitFlag = 0;
			}else if ((abs(nowtime - runtime_p->send_start_time) > 20 ) && inWaitFlag==1 )
			{
				DbgPrintToFile1(31,"超时");
				inWaitFlag = 0;
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime;
			}

			break;
	}
	return TASK_PROCESS;
}
int stateword_process(INT8U *buf645,INT8U len645,INT8U *addr)
{
	int ret=0;
	INT8U tmpDI[4] = {0x01, 0x15, 0x00, 0x04};//主动上报状态字
	INT8U tmpData[12]={};
	memset(tmpData, 0, 12);
	FORMAT07 format07_up;
	BOOLEAN nextFlag=0;
	INT8S ret645 = analyzeProtocol07(&format07_up, buf645, len645, &nextFlag);
	DbgPrintToFile1(31,"解析上报645报文 ret645=%d   DI= %02x %02x  %02x %02x ",ret645,format07_up.DI[0],format07_up.DI[1],format07_up.DI[2],format07_up.DI[3]);

	if ((ret645==0) && (memcmp(tmpDI, format07_up.DI, 4)==0))//正常应答
	{
		memcpy(addr,format07_up.Addr,6);
		if (format07_up.SEQ!=0)//有帧序号
		{
			memcpy(autoReportWords, format07_up.Data, format07_up.Length-5);//数据域多了一个字节帧序号
		}else{
			memcpy(autoReportWords, format07_up.Data, format07_up.Length-4);//
		}
		DbPrt1(31,"状态字 :", (char *) autoReportWords, 12, NULL);
		if (memcmp(autoReportWords, tmpData, 12) != 0)
		{
			ret = 1;
		}else
			ret = 2;
	}
	return ret;
}
int resetAutoEvent(INT8U *addr,INT8U* sendBuf645)
{
	FORMAT07 format07_down;
	int m=0, n=0;
	INT8U sendLen645 = 0;
	INT8U tmpData[12]={};
	memset(tmpData, 0, sizeof(tmpData));

	format07_down.Ctrl = 0x14;//写数据
	format07_down.Length = 4+8+12;
	memcpy((void *)format07_down.Addr, addr, sizeof(format07_down.Addr));

	format07_down.DI[0] = 0x03;
	format07_down.DI[1] = 0x15;
	format07_down.DI[2] = 0x00;
	format07_down.DI[3] = 0x04;
	memset(format07_down.Data, 0, sizeof(format07_down.Data));

	format07_down.Data[0] = 0x02;
	format07_down.Data[1] = 0x00;
	format07_down.Data[2] = 0x00;
	format07_down.Data[3] = 0x00;

	format07_down.Data[4] = 0x78;
	format07_down.Data[5] = 0x56;
	format07_down.Data[6] = 0x34;
	format07_down.Data[7] = 0x12;

	for (m=0; m<12; m++)
	{
		for (n=0; n<8; n++)
		{
			if (autoReportWordInfo[m*8+n].valid == 1)//当前事件需要抄读
			{
				tmpData[m] |= (1<<n);//先给需要复位的事件对应位置1，最后再按位取反
			}
		}
		format07_down.Data[8+m] = ~tmpData[m];
	}
	memset(autoReportWordInfo,0,sizeof(autoReportWordInfo));
	sendLen645 = composeProtocol07(&format07_down, sendBuf645);
	return sendLen645;
}
void getAutoEvent()
{
	int i, j;
	autoEventCounter = 0;
	autoEventTimes = 0;
	for (i=0; i<12; i++)
	{
		for (j=0; j<8; j++)
		{
			if (autoReportWords[i] & (1<<j))
			{
				int index = i*8 + j;
				autoReportWordInfo[index].valid = 1;
				autoReportWordInfo[index].count = autoReportWords[13+autoEventCounter];//状态和次数之间用0xAA间隔
				autoEventTimes += autoReportWordInfo[index].count;
				autoEventCounter++;
			}
		}
	}
}

int getOneEvent(INT8U *addr,INT8U *buf)
{
	int k=0 , m=0 ,  sendLen645=0 ;
	int lens = sizeof(autoEventInfo_meter)/sizeof(AutoEventInfo_Meter);
	INT8U dataFlag[4]={};
	BOOLEAN nextFlag=0;
	FORMAT07 Data07;

	for (k=0; k<96; k++)
	{
		if (autoReportWordInfo[k].valid == 1 && autoReportWordInfo[k].ok==0  && autoReportWordInfo[k].count<=10 )//第 k 事件需要抄读
		{
			for (m=0; m<lens; m++)
			{
				if (autoEventInfo_meter[m].index == (k)  &&
					autoEventInfo_meter[m].num   == autoReportWordInfo[k].counter)//找到对应的数据标识，需要抄读
				{
					autoReportWordInfo[k].counter++;
					DbgPrintToFile1(31,"事件名称 %s",autoEventInfo_meter[m].name);
					dataFlag[0] = autoEventInfo_meter[m].dataFlag[3];
					dataFlag[1] = autoEventInfo_meter[m].dataFlag[2];
					dataFlag[2] = autoEventInfo_meter[m].dataFlag[1];
					dataFlag[3] = autoEventInfo_meter[m].dataFlag[0];
					nextFlag=0;
					memset(buf,0,BUFSIZE645);
					Data07.Ctrl = 0x11;//读数据
					//掉用composeProtocol07函数内部将地址高低反了，此处直接调用地址，回导致地址错误。此处先地址反了
					reversebuff(addr,6,Data07.Addr);
					//memcpy(Data07.Addr, addr, 6);
					memcpy(Data07.DI, dataFlag, 4);
					sendLen645 = composeProtocol07(&Data07, buf);
					DbgPrintToFile1(31,"Addr %02x%02x%02x%02x%02x%02x FLAG07 %02x%02x%02x%02x   sendLen645=%d  ctrl=%02x",
							Data07.Addr[0],Data07.Addr[1],Data07.Addr[2],Data07.Addr[3],Data07.Addr[4],Data07.Addr[5],
							dataFlag[0],dataFlag[1],dataFlag[2],dataFlag[3],sendLen645,Data07.Ctrl);
					return sendLen645;
				}
			}

		}
	}
	return 0;
}
void addMeterEvent(int *msindex, INT8U* buf645,int len645)
{
	autoEvent_Save[*msindex].len = len645;
	memcpy(autoEvent_Save[*msindex].data, buf645, len645);
	DbgPrintToFile1(31,"事件【 %d 】",*msindex);
	DbPrt1(31,"内容：",(char *) autoEvent_Save[*msindex].data, len645, NULL);
	(*msindex)++;
}
INT8U readStateWord(INT8U *addr,INT8U *buf)
{
	INT8U sendLen645=0;
	INT8U tmpWords[4] = {0x01, 0x15, 0x00, 0x04};//主动上报状态字
	FORMAT07 format07_down;
	format07_down.Ctrl = 0x11;//读数据
	reversebuff(addr,6,format07_down.Addr);
//	memcpy(format07_down.Addr, addr, sizeof(format07_down.Addr));
	memcpy(format07_down.DI, tmpWords, 4);
	memset(buf,0,BUFSIZE645);
	sendLen645 = composeProtocol07(&format07_down, buf);

	return sendLen645;
}
int doAutoReport(RUNTIME_PLC *runtime_p)
{
	static int retry =0;
	static INT8U autoReportAddr[6];
	static int step_cj = 0, beginwork=0,msg_index=0;
	int sendlen=0 ,i=0, ret=0;
	time_t nowtime = time(NULL);
	INT8U	transData[512];
	int		transLen=0;

	fprintf(stderr,"step_cj=%d,beginwork=%d  runtime_p->state_bak=%d\n",step_cj,beginwork,runtime_p->state_bak);
	switch( step_cj )
	{
		case 0://确认主动上报
			memset(autoReportAddr,0,6);
			DbgPrintToFile1(31,"确认主动上报");
			memset(buf645,0,BUFSIZE645);
			memcpy(buf645,runtime_p->format_Up.afn06_f5_up.MsgContent,runtime_p->format_Up.afn06_f5_up.MsgLength);
			if (stateword_process(runtime_p->format_Up.afn06_f5_up.MsgContent,runtime_p->format_Up.afn06_f5_up.MsgLength,autoReportAddr)==1)
			{
				getAutoEvent();//根据状态字，表识事件及事件发生次数
				addMeterEvent(&msg_index,buf645,runtime_p->format_Up.afn06_f5_up.MsgLength);//保存状态字到事件缓存
				step_cj = 1;
			}else{
				step_cj = 0;
				clearvar(runtime_p);
				beginwork = 0;
				return(runtime_p->state_bak);
			}
			sendlen = AFN00_F01( &runtime_p->format_Up,runtime_p->sendbuf );//确认
			SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
			clearvar(runtime_p);
			beginwork = 0;
			break;
		case 1://抄读指定事件
			if ( abs(nowtime - runtime_p->send_start_time) > 20 && beginwork==0)
			{
				DbgPrintToFile1(31,"暂停抄表3");
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime ;
				sendlen = AFN12_F2(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
			}else if(runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1 && beginwork==0)
			{//确认
				beginwork = 1;
				clearvar(runtime_p);
			}
			if (beginwork == 1)
			{
				if ( abs(nowtime - runtime_p->send_start_time )> 10)
				{
					clearvar(runtime_p);
					sendlen = getOneEvent(autoReportAddr,	buf645);
					if (sendlen > 0)
					{
						memcpy(runtime_p->format_Down.addr.SourceAddr, runtime_p->masteraddr, 6);
						sendlen = AFN13_F1(&runtime_p->format_Down,runtime_p->sendbuf,autoReportAddr, 2, 0, buf645, sendlen);
						SendDataToCom(runtime_p->comfd, runtime_p->sendbuf, sendlen );
						runtime_p->send_start_time = nowtime ;
					}else{
						DbgPrintToFile1(31,"无事件抄读，开始复位状态字");
						beginwork = 0;
						step_cj = 2;
					}
				}else if ((runtime_p->format_Up.afn == 0x13 && runtime_p->format_Up.fn == 1 ))
				{
					addMeterEvent(&msg_index,runtime_p->format_Up.afn13_f1_up.MsgContent,runtime_p->format_Up.afn13_f1_up.MsgLength);//保存事件
					if(msg_index >= 30 )
					{
						DbgPrintToFile1(31,"事件记录抄过30条，开始复位状态字");
						beginwork = 0;
						step_cj = 2;
					}
					clearvar(runtime_p);
				}else if ((runtime_p->format_Up.afn == 0x06 && runtime_p->format_Up.fn == 5 ))
				{
					DbgPrintToFile1(31,"再次收到主动上报事件，上报流程恢复");
					memset(autoReportAddr,0,6);
					memset(autoEvent_Save,0,sizeof(autoEvent_Save));//暂存的事件
					memset(autoReportWordInfo,0,sizeof(autoReportWordInfo));//12字节主动上报状态字对应的每个事件是否发生及次数
					memset(autoReportWords,0,sizeof(autoReportWords));//电表主动上报状态字
					msg_index = 0;
					beginwork = 0;
					step_cj = 0;
				}
			}
			break;
		case 2://复位状态字
			if (abs( nowtime - runtime_p->send_start_time )> 20)
			{
				retry++;
				if (retry > 5)
					step_cj = 3;
				DbgPrintToFile1(31,"复位状态字");
				memset(buf645,0,BUFSIZE645);
				sendlen = resetAutoEvent(autoReportAddr,buf645);
				memcpy(runtime_p->format_Down.addr.SourceAddr, runtime_p->masteraddr, 6);
				sendlen = AFN13_F1(&runtime_p->format_Down,runtime_p->sendbuf,autoReportAddr, 2, 0, buf645, sendlen);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf, sendlen );
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime ;
			}else if ((runtime_p->format_Up.afn == 0x13 && runtime_p->format_Up.fn == 1 ))//收到应答 AFN13 F1
			{
				memset(buf645,0,BUFSIZE645);
				memcpy(buf645,runtime_p->format_Up.afn13_f1_up.MsgContent,runtime_p->format_Up.afn13_f1_up.MsgLength);
				ret = stateword_process(buf645,runtime_p->format_Up.afn13_f1_up.MsgLength,autoReportAddr);
				if (ret == 1)		//收到不为空的状态字
				{
					DbgPrintToFile1(31,"收到非空状态字，需要处理");
					addMeterEvent(&msg_index,runtime_p->format_Up.afn13_f1_up.MsgContent,runtime_p->format_Up.afn13_f1_up.MsgLength);//保存事件
					getAutoEvent();
					clearvar(runtime_p);
					beginwork = 1;	//可以直接进行状态字处理，不需要暂停抄读
					step_cj = 1;
				}else if(ret == 2)	//收到空的状态字
				{
					DbgPrintToFile1(31,"收到全为0状态字");
					clearvar(runtime_p);
					step_cj = 3;
				}else {				//收到抄读状态字应答报文
					DbgPrintToFile1(31,"收到应答报文，重读状态字");
					memset(buf645,0,BUFSIZE645);
					sendlen = readStateWord(autoReportAddr,buf645);
					memcpy(runtime_p->format_Down.addr.SourceAddr, runtime_p->masteraddr, 6);
					sendlen = AFN13_F1(&runtime_p->format_Down,runtime_p->sendbuf,autoReportAddr, 2, 0, buf645, sendlen);
					SendDataToCom(runtime_p->comfd, runtime_p->sendbuf, sendlen );
					clearvar(runtime_p);
					runtime_p->send_start_time = nowtime ;
				}
			}
			break;
		case 3://发消息到cjcomm
			DbgPrintToFile1(31,"");
			DbgPrintToFile1(31,"给主站发消息, 事件缓存数组 元素个数 【 %d 】",msg_index);
			transLen = 0;
			transData[transLen++] = msg_index;		//sequence of octet-string
			for(i=0;i<msg_index;i++)
			{
				if(autoEvent_Save[i].len>0 && autoEvent_Save[i].len<=0x7f) {
					transData[transLen++] = autoEvent_Save[i].len;
					memcpy(&transData[transLen],autoEvent_Save[i].data,autoEvent_Save[i].len);
					transLen += autoEvent_Save[i].len;
				}else {	//octet-string 长度超过127，长度字节最多两个字节表示（因为类型决定长度不会超过255）
					transData[transLen++] = 0x82;	//0x80:表示长度为多个字节，0x02:表示长度为2个字节
					transData[transLen++] = (autoEvent_Save[i].len >>8) & 0xff;
					transData[transLen++] = autoEvent_Save[i].len & 0xff;
					memcpy(&transData[transLen],autoEvent_Save[i].data,autoEvent_Save[i].len);
					transLen += autoEvent_Save[i].len;
				}
				DbgPrintToFile1(31,"【 %02d 】 |  datalen=%02d  【 %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x 】",
						i,autoEvent_Save[i].len,
						autoEvent_Save[i].data[0],autoEvent_Save[i].data[1],autoEvent_Save[i].data[2],
						autoEvent_Save[i].data[3],autoEvent_Save[i].data[4],autoEvent_Save[i].data[5],
						autoEvent_Save[i].data[6],autoEvent_Save[i].data[7],autoEvent_Save[i].data[8],
						autoEvent_Save[i].data[9],autoEvent_Save[i].data[10],autoEvent_Save[i].data[11],
						autoEvent_Save[i].data[12],autoEvent_Save[i].data[13],autoEvent_Save[i].data[14],
						autoEvent_Save[i].data[15],autoEvent_Save[i].data[16],autoEvent_Save[i].data[17],
						autoEvent_Save[i].data[18],autoEvent_Save[i].data[19],autoEvent_Save[i].data[20],
						autoEvent_Save[i].data[21],autoEvent_Save[i].data[22],autoEvent_Save[i].data[23],
						autoEvent_Save[i].data[24],autoEvent_Save[i].data[25],autoEvent_Save[i].data[26]);
			}
			ret = mqs_send((INT8S *)PROXY_NET_MQ_NAME,cjcomm,NOTIFICATIONTRANS_PEPORT,OAD_PORT_ZB,(INT8U *)&transData,transLen);
			memset(autoEvent_Save,0,sizeof(autoEvent_Save));//暂存的事件
			memset(autoReportWordInfo,0,sizeof(autoReportWordInfo));//12字节主动上报状态字对应的每个事件是否发生及次数
			memset(autoReportWords,0,sizeof(autoReportWords));//电表主动上报状态字
			autoEventCounter = 0;//96个事件中，发生的事件个数
			msg_index = 0;
			beginwork = 0;
			retry = 0;
			step_cj = 0;

			return(runtime_p->state_bak);
	}
	return AUTO_REPORT;
}
int broadcast_07(INT8U *buf,int delays,INT8U adr)
{
	DateTimeBCD  ts;
	FORMAT07 frame07;
	time_t nowtime ;
	int sendlen = 0;
	if (broadtime.is !=1)
	{
		nowtime = time(NULL) ;
		nowtime += delays;
		ts = timet_bcd(nowtime);
		frame07.Ctrl = 0x08;//广播校时
		memset(&frame07.Addr, adr, 6);//地址
		frame07.Time[0] = ts.sec.data;
		frame07.Time[1] = ts.min.data;
		frame07.Time[2] = ts.hour.data;
		frame07.Time[3] = ts.day.data;
		frame07.Time[4] = ts.month.data;
		frame07.Time[5] = ts.year.data%100;
		sendlen = composeProtocol07(&frame07, buf);
	}else
	{
		nowtime = tmtotime_t(broadtime.broadCastTime);
		nowtime = nowtime + delays;
		ts = timet_bcd(nowtime);
		frame07.Ctrl = 0x08;//广播校时
		memset(&frame07.Addr, adr, 6);//地址
		frame07.Time[0] = ts.sec.data;
		frame07.Time[1] = ts.min.data;
		frame07.Time[2] = ts.hour.data;
		frame07.Time[3] = ts.day.data;
		frame07.Time[4] = ts.month.data;
		frame07.Time[5] = ts.year.data%100;
		sendlen = composeProtocol07(&frame07, buf);

	}
	return sendlen;
}
int doBroadCast(RUNTIME_PLC *runtime_p)
{
	static int step_cj = 0, workflg=0  ,dealytime=0;
	int sendlen=0;
	time_t nowtime = time(NULL);

	switch( step_cj )
	{
		case 0://暂停抄表
			if (broadtime.is==1)
			{
				step_cj = 1;
				clearvar(runtime_p);
				break;
			}
			if (abs( nowtime - runtime_p->send_start_time )> 20)
			{
				DbgPrintToFile1(31,"广播对时_暂停抄表");
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime ;
				sendlen = AFN12_F2(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
			}else if(runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1)
			{//确认
				clearvar(runtime_p);
				workflg = 0;
				step_cj = 1;
			}
			break;
		case 1://查询通信延时相关广播通信时长
			if ( abs(nowtime - runtime_p->send_start_time) > 20)
			{
				dealytime = 0;
				memset(buf645,0,BUFSIZE645);
				sendlen = broadcast_07(buf645,0,0x98);
				if (broadtime.is==1)
					sendlen = AFN03_F9(&runtime_p->format_Down,runtime_p->sendbuf,0,broadtime.len,broadtime.buf);
				else
					sendlen = AFN03_F9(&runtime_p->format_Down,runtime_p->sendbuf,0,sendlen,buf645);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				DbgPrintToFile1(31,"广播对时_查询广播通信时长");
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime ;
			}else if (runtime_p->format_Up.afn == 0x12 )
			{
				sendlen = AFN00_F01( &runtime_p->format_Up,runtime_p->sendbuf );//确认
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				clearvar(runtime_p);
				DbgPrintToFile1(31,"广播对时_收到AFN=12类报文，进行确认");
				runtime_p->send_start_time = nowtime ;
			}else if (runtime_p->format_Up.afn == 0x03 && runtime_p->format_Up.fn==9 )
			{
				DbgPrintToFile1(31,"收到广播对时_查询通信时长回复");
				step_cj = 2;
				dealytime = runtime_p->format_Up.afn03_f9_up.DelayTime;
				clearvar(runtime_p);
			}
			break;
		case 2://
			if ( abs(nowtime - runtime_p->send_start_time) > 20 && workflg==0)
			{
				workflg = 1;
				memset(buf645,0,BUFSIZE645);
				sendlen = broadcast_07(buf645,dealytime ,0x99);
				sendlen = AFN05_F3(&runtime_p->format_Down,0,0x02,buf645,sendlen,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime ;
				DbgPrintToFile1(31,"广播对时_下发广播对时");
			}else if((runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1))
			{//确认
				DbgPrintToFile1(31,"收到广播确认 延时 %d s",runtime_p->format_Up.afn00_f1.WaitingTime);
				if (runtime_p->format_Up.afn00_f1.WaitingTime < 100 )
				{
					DbgPrintToFile1(31,"延时 %d s",runtime_p->format_Up.afn00_f1.WaitingTime);
					sleep(runtime_p->format_Up.afn00_f1.WaitingTime );
					DbgPrintToFile1(31,"延时时间到");
				}
				clearvar(runtime_p);
				step_cj = 0;
				workflg = 0;
				runtime_p->redo = 2;  //广播后恢复抄表
				return(runtime_p->state_bak);
			}else if((abs(nowtime - runtime_p->send_start_time > 20) && workflg==1) )
			{
				DbgPrintToFile1(31,"广播超时");
				clearvar(runtime_p);
				step_cj = 0;
				workflg = 0;
				runtime_p->redo = 2;  //广播后恢复抄表
				return(runtime_p->state_bak);
			}
			break;
	}
	return BROADCAST;
}
//载波查询模块工作模式和修改工作模式报文
//INT8U  searchMode[15]={0x68,0x0f,0x00,0x47,0x00,0x00,0xFF,0x00,0x00,0x0b,0x02,0x40,0x01,0x94,0x16};
//INT8U  setMode[16]={0x68,0x10,0x00,0x47,0x00,0x00,0xFF,0x00,0x00,0x0d,0x01,0x40,0x01,0x66,0xFb,0x16};
int ESRT_Mode_Chg(RUNTIME_PLC *runtime_p)
{
	//东软载波模块,切换路由工作模式
//	static INT8U chgMode[16]={0x68,0x10,0x00,0x47,0x00,0x00,0xFF,0x00,0x00,0x00,0x01,0x40,0x01,0x76,0xFE,0x16};
	static INT8U readMode[15]={0x68,0x0f,0x00,0x47,0x00,0x00,0xFF,0x00,0x00,0x02,0x02,0x40,0x01,0x8b,0x16};
	static INT8U count = 0;

	if(getZone("GW")!=0) {  //非国网送检
		if(module_info.ModuleInfo.VendorCode[1]=='E' && module_info.ModuleInfo.VendorCode[0]=='S') {
			time_t nowtime = time(NULL);
			if ((abs(nowtime  - runtime_p->send_start_time) > 20) )
			{
				count++;
				if(count>3) {
					count = 0;
					clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
					DbgPrintToFile1(31,"读取路由模式切换结束");
					return INIT_MASTERADDR;
				}
				SendDataToCom(runtime_p->comfd, readMode,15);
				DbgPrintToFile1(31,"东软载波模块[ESRT]。查询路由模式");
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
				runtime_p->send_start_time = nowtime ;
			}else if(runtime_p->format_Up.afn == 0x02 && runtime_p->format_Up.fn == 15)
			{
//				DbgPrintToFile1(31,"afn = %02x fn = %d \n",runtime_p->format_Up.afn,runtime_p->format_Up.fn);
				clearvar(runtime_p);
				return INIT_MASTERADDR;
			}
			return ZB_MODE;
		}
	}
	return INIT_MASTERADDR;
}


void readplc_thread()
{
	int startFlg = 1;
	int state = DATE_CHANGE;
	RUNTIME_PLC runtimevar;
	memset(&runtimevar,0,sizeof(RUNTIME_PLC));
	mypara.my6000 = JProgramInfo->oi_changed.oi6000 ;
	mypara.my6012 = JProgramInfo->oi_changed.oi6012 ;
	mypara.my6002 = JProgramInfo->oi_changed.oi6002 ;
	mypara.myf209 = JProgramInfo->oi_changed.oiF209 ;
	RecvHead = 0;
	RecvTail = 0;
	search_i = 0;
	RouterFactory = 0;
	initSearchMeter(&search6002);
	initTaskData(&taskinfo);
	PrintTaskInfo2(&taskinfo);
	DbgPrintToFile(31,"载波线程开始...111");
	runtimevar.format_Down.info_down.ReplyBytes = 0x28;

	DbgPrintToFile1(31,"1-fangAn6015[%d].sernum = %d  fangAn6015[%d].mst.mstype = %d ",
			0,fangAn6015[0].sernum,0,fangAn6015[0].mst.mstype);
	TSGet(&runtimevar.nowts);
	runtimevar.modeFlag = -1;//初始化为-1
	memset(&runtimevar.oldts,0,sizeof(&runtimevar.oldts));
	while(1)
	{
		usleep(50000);
		/********************************
		 * 	   状态实时判断
		********************************/
		TSGet(&runtimevar.nowts);
		state = stateJuge(state, &mypara,&runtimevar,&startFlg);

		/********************************
		 * 	   状态流程处理
		********************************/
		switch(state)
		{
			case DATE_CHANGE :
				state = doInit(&runtimevar);					//初始化 		 （ 1、硬件复位 2、模块版本信息查询  ）
				break;
			case ZB_MODE ://如果东软载波模块(ESRT)，且版本为(4331)进行路由模式切换到第４代。此时透传的AFN13-F1的规约类型一致填写:00(透明传输)
				state = ESRT_Mode_Chg(&runtimevar);
//				state = INIT_MASTERADDR;
				break;
			case INIT_MASTERADDR :
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
				if (runtimevar.modeFlag==1)
					state = doTask_by_jzq(&runtimevar);			//按任务抄表	  (集中器主导 1、根据方案类型和编号号确定抄表报文  )
				else
					state = doTask(&runtimevar);				//按任务抄表	  ( 1、根据方案类型和编号号确定抄表报文  )
				break;
			case AUTO_REPORT:
				state = doAutoReport(&runtimevar);				//载波主动上报处理
				break;
			case BROADCAST:
				state = doBroadCast(&runtimevar);				//广播对时
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
