/*
 * autotask.c
 *
 *  Created on: Mar 1, 2017
 *      Author: admin
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../include/Objectdef.h"
#include "../libBase/PublicFunction.h"
#include "../libAccess/AccessFun.h"
#include "dlt698def.h"
extern INT8U TmpDataBuf[MAXSIZ_FAM];
extern INT8U TmpDataBufList[MAXSIZ_FAM*2];
/*
 * datetime 开始时间
 * ti 间隔
 */
time_t calcnexttime(TI ti,DateTimeBCD datetime)
{
	int jiange=0;
	time_t timestart=0,timenow=0,timetmp=0,timeret=0;
	TS ptm;
	if (ti.interval<=0)
		return 0;
	ptm.Year = datetime.year.data;
	ptm.Month = datetime.month.data;
	ptm.Day = datetime.day.data;
	ptm.Hour = datetime.hour.data;
	ptm.Minute = datetime.min.data;
	ptm.Sec = datetime.sec.data;
	timestart = tmtotime_t(ptm);//开始时间
	timenow = time(NULL);//当前时间
	switch(ti.units)
	{
		case 0://秒
			jiange = ti.interval;
			break;
		case 1://分
			jiange = ti.interval * 60;
			break;
		case 2://时
			jiange =  ti.interval * 3600;
			break;
		case 3://日
			jiange = ti.interval * 3600 *24;
			break;
		case 4://月
			break;
		case 5://年
			break;
		default :
			break;
	}
	if (timenow > timestart)
	{
		timetmp = timenow - timestart;
		int intpart = timetmp / jiange;
		int rempart = timetmp % jiange;
		fprintf(stderr,"\n任务开始时间(%ld)  早于当前时间(%ld)  %d个间隔 余%d 秒",timestart,timenow,intpart,rempart);
		if (rempart>0)
		{
			timeret = (intpart + 1) * jiange  + timestart ;
			fprintf(stderr,"\n计算下次开始时间 %ld ",timeret);
			return timeret ;
		}
		else
		{
			fprintf(stderr,"\n任务在当前时间 %ld 执行",timenow);
			return timenow;
		}
	}else
	{
		fprintf(stderr,"\n任务开始时间(%ld)  晚于当前时间(%ld)  将于开始时间执行",timestart,timenow);
		return timestart;
	}
}

/*
 * 初始化自动上报方案
 */
void init_autotask(CLASS_6013 class6013,AutoTaskStrap* list)
{
	static int index=0;

	if(class6013.cjtype == rept)
	{
		list[index].ID = class6013.taskID;
		list[index].SerNo = class6013.sernum;
		list[index].nexttime = calcnexttime(class6013.interval,class6013.startime);
		index++;
	}
}
int selector10getdata(TSA tsa,INT8U num,CSD_ARRAYTYPE *csds, INT8U *databuf)
{

	return 0;
}

/*按 selector10 中 上n条记录   和    MS .TSA 去查询数据
 *
 *  REPORT_NOTIFICATION
 *  REPROTNOTIFICATIONRECORDLIST
 *	PIID
 *	SEQUENCE OF A-ReportRecord
 *		num:  0		A-ReportRecord  某一个电表（TSA）的 ROAD或 OAD 的记录
 *			  		{
 *
 *			  		}
 *			  1		A-ReportRecord
 *			  		{
 *			  			如果没有数据  DAR 0
 *			  		}
 *			  2		A-ReportRecord
 *			  		{
 *			  			...
 *			  		}
 *
 */
//根据type填充tsas ; 返回TS 的数量
int getTsas(INT8U type,TSA *tsas)
{
	return 0;
}
int doAutoReport(CLASS_601D report,CommBlock* com)
{
	if (com==NULL)
		return 0;
	INT8U *sendbuf = com->SendBuf;
	TSA tsa[255]={};

	fprintf(stderr,"\ndo AutoReport!!!");
	CSINFO csinfo;
	CSD_ARRAYTYPE *csds;
	INT8U mstype=0;
	int tsanum = 0 , i=0,index=0 ,recordnum=0;

	if (report.reportdata.type==0)//OAD
	{

	}else if(report.reportdata.type==1)//RecordData
	{
		csds = &report.reportdata.data.recorddata.csds;							// 方案中 rcsd
		recordnum = report.reportdata.data.recorddata.rsd.selec10.recordn; 		// 上 n 条记录
		mstype = report.reportdata.data.recorddata.rsd.selec10.meters.mstype; 	// 方案中 MS的类型
		tsanum = getTsas(mstype,tsa);
		for(i=0; i<tsanum ; i++)
		{
			memset(TmpDataBuf,0,sizeof(TmpDataBuf));
			selector10getdata(tsa[i], recordnum, csds, TmpDataBuf);

//			csinfo.dir = 1;
//			csinfo.prm = 1;
//			index = FrameHead(&csinfo,sendbuf);
//			hcsi = index;
//			index = index + 2;
//
//			apduplace = index;		//记录APDU 起始位置
//			sendbuf[index++] = REPORT_NOTIFICATION;
//			sendbuf[index++] = REPROTNOTIFICATIONRECORDLIST;
//			sendbuf[index++] = 0;//PIID

			if (report.reportdata.type==1)//RecordData
			{

			}else if(report.reportdata.type==0)//OAD
			{

			}else
				return 0;

			sendbuf[index++] = 0;
			sendbuf[index++] = 0;
//			FrameTail(sendbuf,index,hcsi);
			if(com->p_send!=NULL)
				com->p_send(com->phy_connect_fd,sendbuf,index+3);

		}
	}else
	{
		return 0;
	}
	return 1;
}
INT16U  composeAutoTask(AutoTaskStrap* list ,CommBlock* com)
{
	int i=0;
	time_t timenow = time(NULL);
	for(i=0; i< MAXNUM_AUTOTASK ;i++)
	{
		if(timenow >= list[i].nexttime)
		{
			CLASS_6013 class6013;
			if (readCoverClass(0x6013, list[i].ID, &class6013, sizeof(CLASS_6013),coll_para_save) == 1)
			{
				fprintf(stderr,"\ni=%d 任务【 %d 】 	 开始执行   上报方案编号【 %d 】",i,list[i].ID,list[i].SerNo);
				CLASS_601D class601d;
				if (readCoverClass(0x601D, list[i].SerNo, &class601d, sizeof(CLASS_601D),coll_para_save) == 1)
					doAutoReport(class601d,com);
				list[i].nexttime = calcnexttime(class6013.interval,class6013.startime);
				return 1;
			}else
			{
//				fprintf(stderr,"\n任务参数丢失！");
			}
		}
	}
	return 0;
}
