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
extern void FrameTail(INT8U *buf,int index,int hcsi);
extern int FrameHead(CSINFO *csinfo,INT8U *buf);
//-------

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

	fprintf(stderr,"任务开始时间 %04d-%02d-%02d %02d:%02d:%02d\n",datetime.year.data,datetime.month.data,datetime.day.data,datetime.hour.data,datetime.min.data,datetime.sec.data);
	ptm.Year = datetime.year.data;
	ptm.Month = datetime.month.data;
	ptm.Day = datetime.day.data;
	ptm.Hour = datetime.hour.data;
	ptm.Minute = datetime.min.data;
	ptm.Sec = datetime.sec.data;
	timestart = tmtotime_t(ptm);//开始时间
	timenow = time(NULL);//当前时间
	jiange = getTItoSec(ti);
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
	struct tm tmp_tm;
	if(class6013.cjtype == rept)
	{
		list[index].ID = class6013.taskID;
		list[index].SerNo = class6013.sernum;
		list[index].nexttime = calcnexttime(class6013.interval,class6013.startime);
		fprintf(stderr,"\ninit_autotask [ %d 任务 %d 方案 %d  下次时间 %ld ]",index,list[index].ID ,list[index].SerNo,list[index].nexttime );
		localtime_r(&list[index].nexttime,&tmp_tm);
		fprintf(stderr,"下次时间 %04d-%02d-%02d %02d:%02d:%02d\n",tmp_tm.tm_year+1900,tmp_tm.tm_mon+1,tmp_tm.tm_mday,tmp_tm.tm_hour,tmp_tm.tm_min,tmp_tm.tm_sec);
		index++;
	}
}
int fillcsinfo(CSINFO *csinfo,INT8U *addr,INT8U clientaddr)
{
	int i=0;
	csinfo->dir = 1;		//服务器发出
	csinfo->prm = 0; 	//服务器发出
	csinfo->funcode = 3; //用户数据
	csinfo->sa_type = 0 ;//单地址
	csinfo->sa_length = addr[0] ;
	//服务器地址
	fprintf(stderr,"sa_length = %d \n",csinfo->sa_length);
	if(csinfo->sa_length<OCTET_STRING_LEN) {
		for(i=0;i<csinfo->sa_length;i++) {
			csinfo->sa[i] = addr[csinfo->sa_length-i];
		}
	}else {
		fprintf(stderr,"SA 长度超过定义长度，不合理！！！\n");
		return 0;
	}
	//客户端地址
	csinfo->ca = clientaddr;
	return 1;
}
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

/*
 *
 */
long int readFrameDataFile(char *filename,int offset,INT8U *buf,int *datalen)
{
	FILE *fp=NULL;
	int bytelen=0;

	fp = fopen(filename,"r");
	if (fp!=NULL && buf!=NULL)
	{
		fseek(fp,offset,0);		 			//定位到文件指定偏移位置
		if (fread(&bytelen,2,1,fp) <=0)	 	//读出数据报文长度
			return 0;
		if (fread(buf,bytelen,1,fp) <=0 ) 	//按数据报文长度，读出全部字节
			return 0;
		*datalen = bytelen;
		return (ftell(fp));		 			//返回当前偏移位置
	}
	return 0;
}

/*
 * 通讯进程循环调用 callAutoReport
 *
 *  ifecho ：  0 没收到确认，或第一次调用    1 收到确认
 *  返回    :  1  需要继续发送   0 发送完成
 */
int callAutoReport(CommBlock* com, INT8U ifecho)
{
	if (com==NULL)
		return 0;
	INT8U *sendbuf = com->SendBuf;
	static int nowoffset = 0;
	static int nextoffset = 0;
	static int sendcounter =0;
	int datalen = 0, j=0,index=0 ,hcsi=0,apduplace=0;
	CSINFO csinfo={};

	memset(TmpDataBuf,0,sizeof(TmpDataBuf));  //长度 1600
	if (ifecho == 1 )//上一次给确认了或者发送计数大于上报次数限制,通信也置位，默认主站收到报文
	{
		nowoffset = nextoffset;
		sendcounter = 0;
	}
	sendcounter++;
	datalen = 0;
	fprintf(stderr,"\n当前偏移位置 nowoffset = %d  ",nowoffset);
	nextoffset = readFrameDataFile("/nand/datafile",nowoffset,TmpDataBuf,&datalen);
	fprintf(stderr,"\n读出 (%d)：",datalen);
	for(j=0; j<datalen; j++)
	{
		if (j%20==0)fprintf(stderr,"\n");
		fprintf(stderr,"%02x ",TmpDataBuf[j]);
	}
	fprintf(stderr,"\n下帧偏移位置 nextoffset = %d ",nextoffset);
	if (nextoffset == 0)
	{
		fprintf(stderr,"\n发送完毕！");
		nowoffset = 0;
		sendcounter = 0;
		return 0;
	}

	index = 0;
	if (fillcsinfo(&csinfo,com->serveraddr,com->taskaddr)==0)
		return 0;
	index = FrameHead(&csinfo,sendbuf);
	hcsi = index;
	index = index + 2;
	apduplace = index;		//记录APDU 起始位置
	sendbuf[index++] = REPORT_NOTIFICATION;
	sendbuf[index++] = REPROTNOTIFICATIONRECORDLIST;
	sendbuf[index++] = 0;	//PIID

	memcpy(sendbuf,TmpDataBuf,datalen);//将读出的数据拷贝
	index +=datalen;

	sendbuf[index++] = 0;
	sendbuf[index++] = 0;
	FrameTail(sendbuf,index,hcsi);
	if(com->p_send!=NULL)
		com->p_send(com->phy_connect_fd,sendbuf,index+3);

	return 1;
}

int GetReportData(CLASS_601D report)
{
	int  ret = 0;
	if (report.reportdata.type==0)//OAD
	{

	}else if(report.reportdata.type==1)//RecordData
	{
		ret = getSelector(report.reportdata.data.oad,
							report.reportdata.data.recorddata.rsd,
							report.reportdata.data.recorddata.selectType,
							report.reportdata.data.recorddata.csds,NULL, NULL);
	}
	return ret;
}
INT16U  composeAutoTask(AutoTaskStrap *list)
{
	int i=0, ret=0;
	time_t timenow = time(NULL);
	CLASS_6013 class6013={};
	CLASS_601D class601d={};

	if(timenow >= list->nexttime)
	{
		if (readCoverClass(0x6013, list->ID, &class6013, sizeof(CLASS_6013),coll_para_save) == 1)
		{
			fprintf(stderr,"\ni=%d 任务【 %d 】 	 开始执行   上报方案编号【 %d 】",i,list->ID,list->SerNo);
			if (readCoverClass(0x601D, list->SerNo, &class601d, sizeof(CLASS_601D),coll_para_save) == 1)
			{
				list->ReportNum = class601d.reportnum;
				list->OverTime = getTItoSec(class601d.timeout);
				if (GetReportData(class601d) == 1)//数据组织好了
					ret = 2;
			}
			list->nexttime = calcnexttime(class6013.interval,class6013.startime);
		}else
		{
//				fprintf(stderr,"\n任务参数丢失！");
		}
	}
	return ret;
}
