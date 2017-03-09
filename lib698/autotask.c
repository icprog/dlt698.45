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

/*
 * 根据ms.type填充tsas ; 返回TS 的数量
 * 注意调用后，释放**tsas的内存
 */
int getTsas(MY_MS ms,INT8U **tsas)
{
	int  tsa_num = 0;
	int	 record_num = 0;
	int	 tsa_len = 0;
	int	 i=0,j=0;
	CLASS_6001	 meter={};

	if(ms.mstype == 0) { //无电能表
		tsa_num = 0;
		return tsa_num;
	}
	record_num = getFileRecordNum(0x6000);
	*tsas = malloc(record_num*sizeof(TSA));
	fprintf(stderr," tsas  p=%p record_num=%d",*tsas,record_num);
	tsa_num = 0;
	for(i=0;i<record_num;i++) {
		if(readParaClass(0x6000,&meter,i)==1) {
			if(meter.sernum!=0 && meter.sernum!=0xffff) {
				switch(ms.mstype) {
				case 1:	//全部用户地址
					fprintf(stderr,"\nTSA: %d-",meter.basicinfo.addr.addr[0]);
					for(j=0;j<meter.basicinfo.addr.addr[0];j++) {
						fprintf(stderr,"-%02x",meter.basicinfo.addr.addr[j+1]);
					}
					memcpy(*tsas+(tsa_num*sizeof(TSA)),&meter.basicinfo.addr,sizeof(TSA));
					tsa_num++;
					break;
				case 2:	//一组用户类型
					for(j=0;j<ms.ms.userType[0];j++) {
						if(meter.basicinfo.usrtype == ms.ms.userType[j+1]) {
							memcpy(*tsas+(tsa_num*sizeof(TSA)),&meter.basicinfo.addr,sizeof(TSA));
							tsa_num++;
							break;
						}
					}
					break;
				case 3:	//一组用户地址
					tsa_len = (ms.ms.userAddr[0].addr[0]<<8) | ms.ms.userAddr[0].addr[1];
					for(j=0;j<tsa_len;j++) {
						if(memcmp(&ms.ms.userAddr[j+1],&meter.basicinfo.addr,sizeof(TSA))==0) {  //TODO:TSA下发的地址是否按照00：长度，01：TSA长度格式
							memcpy(*tsas+(tsa_num*sizeof(TSA)),&meter.basicinfo.addr,sizeof(TSA));
							tsa_num++;
							break;
						}
					}
					break;
				case 4:	//一组配置序号
					for(j=0;j<ms.ms.configSerial[0];j++) {
						if(meter.sernum == ms.ms.configSerial[j+1]) {
							memcpy(*tsas+(tsa_num*sizeof(TSA)),&meter.basicinfo.addr,sizeof(TSA));
							tsa_num++;
							break;
						}
					}
					break;
				case 5://一组用户类型区间

					break;
				case 6://一组用户地址区间

					break;
				case 7://一组配置序号区间

					break;
				}
			}
		}
	}
	fprintf(stderr,"\nms.mstype = %d,tsa_num = %d",ms.mstype,tsa_num);
	return tsa_num;
}

int doAutoReport(CLASS_601D report,CommBlock* com)
{
	if (com==NULL)
		return 0;
	INT8U *sendbuf = com->SendBuf;
	TSA *tsa=NULL;

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
		tsanum = getTsas(report.reportdata.data.recorddata.rsd.selec10.meters,(INT8U **)&tsa);
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
		if(tsa!=NULL) {
			free(tsa);
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
