/*
 * autotask.c
 *
 *  Created on: Mar 1, 2017
 *      Author: admin
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "../include/Objectdef.h"
#include "../libBase/PublicFunction.h"
#include "../libAccess/AccessFun.h"
#include "dlt698def.h"
#include "dlt698.h"
#include "OIfunc.h"

extern INT8U securetype;
extern INT8U TmpDataBuf[MAXSIZ_FAM];
extern INT8U TmpDataBufList[MAXSIZ_FAM*2];
extern void FrameTail(INT8U *buf,int index,int hcsi);
extern int FrameHead(CSINFO *csinfo,INT8U *buf);
extern INT16S composeSecurityResponse(INT8U* SendApdu,INT16U Length);
//-------
//DateTimeBCD timet_bcd(time_t t)
//{
//	DateTimeBCD ts;
//    struct tm set;
//
//    localtime_r(&t, &set);
//    ts.year.data  = set.tm_year + 1900;
//    ts.month.data = set.tm_mon + 1;
//    ts.day.data   = set.tm_mday;
//    ts.hour.data  = set.tm_hour;
//    ts.min.data   = set.tm_min;
//    ts.sec.data   = set.tm_sec;
//    return ts;
//}
TS mylookback(time_t times,TI ti,INT8U n)//根据当前时间times 和间隔 ti，计算当前时间为基准的上 n 间隔值的时刻
{
	TS ts;
	return ts;
}
/*
 * datetime 开始时间
 * ti 间隔
 * ti_delay 延时－目前只处理了日冻结月冻结的延时　　其它的应该不会有任务延时
 */
time_t calcnexttime(TI ti,DateTimeBCD datetime,TI ti_delay)
{
	int jiange=0;
	time_t timestart=0,timenow=0,timetmp=0,timeret=0;
	TS ptm;
	if (ti.interval<=0)
		return 0;

	//asyslog(LOG_NOTICE,"任务开始时间 %04d-%02d-%02d %02d:%02d:%02d\n",datetime.year.data,datetime.month.data,datetime.day.data,datetime.hour.data,datetime.min.data,datetime.sec.data);
	ptm.Year = datetime.year.data;
	ptm.Month = datetime.month.data;
	ptm.Day = datetime.day.data;
	ptm.Hour = datetime.hour.data;
	ptm.Minute = datetime.min.data;
	ptm.Sec = datetime.sec.data;

	//加上延时
	if(ti_delay.interval > 0)
	{
		tminc(&ptm, ti_delay.units,ti_delay.interval);
	}

	timestart = tmtotime_t(ptm);//开始时间
	timenow = time(NULL);//当前时间
	jiange = TItoSec(ti);
	fprintf(stderr,"延迟　TI:%d-%d, jiange = %d 秒\n",ti.units,ti.interval,jiange);
	if (timenow >= timestart)
	{
		if(jiange > 0 )
		{
			timetmp = timenow - timestart;
			int intpart = timetmp / jiange;
			int rempart = timetmp % jiange;
			fprintf(stderr,"\n任务开始时间(%ld)  早于当前时间(%ld)  %d个间隔 余%d 秒",timestart,timenow,intpart,rempart);
//			if (rempart>0)	//修改去掉else的判断条件,当余数=0时,原代码进入else返回当前时间,重新发送曲线数据
//			{
				timeret = (intpart + 1) * jiange  + timestart ;
				fprintf(stderr,"\n计算下次开始时间 %ld ",timeret);
				return timeret ;
//			}else
//			{
//				fprintf(stderr,"\n任务在当前时间 %ld 执行",timenow);
//				return timenow;
//			}
		}
		else//间隔日 月 或者 年
		{
			TS tmNext;
			TSGet(&tmNext);
			fprintf(stderr,"\n\n ti.interval = %d",ti.interval);

			tminc(&tmNext, ti.units,1);

			if(ti.units != day_units)
			{
				tmNext.Day = datetime.day.data;
			}
			tmNext.Hour = datetime.hour.data;
			tmNext.Minute = datetime.min.data;
			tmNext.Sec = datetime.sec.data;

			//加上延时
			if(ti_delay.interval > 0)
			{
				tminc(&tmNext, ti_delay.units,ti_delay.interval);
			}

			timeret = tmtotime_t(tmNext);//开始时间
			asyslog(LOG_NOTICE,"\n\n********下次开始时间--------1 %d-%d-%d %d:%d:%d",tmNext.Year,tmNext.Month,tmNext.Day,tmNext.Hour,tmNext.Minute,tmNext.Sec);
			return timeret;
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
void init_autotask(INT16U taskIndex,CLASS_6013 class6013,AutoTaskStrap* list)
{
	struct tm  tmp_tm;
		list[taskIndex].ID = class6013.taskID;
		list[taskIndex].SerNo = class6013.sernum;
		list[taskIndex].nexttime = calcnexttime(class6013.interval,class6013.startime,class6013.delay);
		fprintf(stderr,"\ninit_autotask [ %d 任务 %d 方案 %d  下次时间 %ld ]",taskIndex,list[taskIndex].ID ,list[taskIndex].SerNo,list[taskIndex].nexttime );
		localtime_r(&list[taskIndex].nexttime,&tmp_tm);
		fprintf(stderr,"下次时间 %04d-%02d-%02d %02d:%02d:%02d\n",tmp_tm.tm_year+1900,tmp_tm.tm_mon+1,tmp_tm.tm_mday,tmp_tm.tm_hour,tmp_tm.tm_min,tmp_tm.tm_sec);
		asyslog(LOG_NOTICE,"任务索引【%d】,方案【%d】,下次时间【%04d-%02d-%02d %02d:%02d:%02d】",taskIndex,list[taskIndex].ID,tmp_tm.tm_year+1900,tmp_tm.tm_mon+1,tmp_tm.tm_mday,tmp_tm.tm_hour,tmp_tm.tm_min,tmp_tm.tm_sec);

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
 * 填充frmdata 文件，组织数据上报帧
 * 返回=1， 数据准备好, 返回 ReportNotificationList
 * 文件内容:
 * 2个字节：数据有效长度(先低后高)
 * 数据： SEQUENCE OF A-ResultNormal + OAD + GetResult + 实际数据
 * */
int fillTaskfrm(OAD oad,INT8U *data,int frmlen)
{
	FILE *fp = NULL;
	INT8U	frmdata[MAXSIZ_FAM]={};
	int		index = 2; //前两个字节保存有效长度

	fp = openFramefile(TASK_FRAME_DATA);
	if (fp==NULL)
		return 0;
	memset(frmdata,0,sizeof(frmdata));

	frmdata[index++] = 1;						//SEQUENCE OF A-ResultNormal
	index +=create_OAD(0,&frmdata[index],oad);	//OAD
	frmdata[index++] = 1;						//Data
	memcpy(&frmdata[index],data,frmlen);		//拷贝oad数据
	index += frmlen;
	fprintf(stderr,"index=%d\n",index);
	frmdata[0] = (index-2) & 0xff;
	frmdata[1] = ((index-2)>>8) & 0xff;
	saveOneFrame(frmdata,index,fp);
	fclose(fp);
	return REPORTNOTIFICATIONLIST;
}

/*
 * 根据OAD进行相应的数据组帧
 * */
int getTaskOadData(OAD taskoad)
{
	int		ret = 0;
	int  	datalen=0;
	INT8U	data[MAXSIZ_FAM]={};

	fprintf(stderr,"taskoad.OI=%04x\n",taskoad.OI);
	memset(data,0,sizeof(data));
	switch(taskoad.OI) {
	case 0x4000:
		datalen = Get_4000(taskoad,data);
		break;
	}

	ret = fillTaskfrm(taskoad,data,datalen);
	fprintf(stderr,"datalen=%d ret=%d\n",datalen,ret);
	return ret;
}

int GetReportData(CLASS_601D report)
{
	int  ret = 0;
	INT16U server_send_size=0;			//服务器发送帧最大尺寸

	fprintf(stderr,"report.reportdata.type=%d  report.reportdata.data.recorddata.selectType=%d\n",report.reportdata.type,report.reportdata.data.recorddata.selectType);
	if (report.reportdata.type==0)//OAD
	{
		ret = getTaskOadData(report.reportdata.data.oad);
	}else if(report.reportdata.type==1)//RecordData
	{
		if(AppVar_p==NULL) {
			server_send_size = FRAMELEN;
		}else server_send_size = AppVar_p->server_send_size;
		fprintf(stderr,"server_send_size = %d\n",server_send_size);
		ret = getSelector(report.reportdata.data.oad,
							report.reportdata.data.recorddata.rsd,
							report.reportdata.data.recorddata.selectType | 0x80,
							report.reportdata.data.recorddata.csds,NULL, NULL,server_send_size);
		fprintf(stderr,"GetReportData   ret=%d\n",ret);
		if(ret>=1) {	//查找到相应的数据
			ret = REPROTNOTIFICATIONRECORDLIST;	//
		}
	}
	return ret;
}

INT16U  composeAutoTask(AutoTaskStrap *list)
{
	int i=0, ret=0;
	time_t timenow = time(NULL);
	CLASS_6013 class6013={};
	CLASS_601D class601d={};
	DateTimeBCD timebcd={};
	if(list->nexttime==0)	return ret;		//防止无效重复读取文件

	timenow = time(NULL);

//	fprintf(stderr,"timenow=%ld  nexttime=%ld\n",timenow,list->nexttime);

	if(timenow >= list->nexttime)
	{
//		asyslog(LOG_INFO,"任务上报时间%ld, %ld\n", list->nexttime, timenow);
		timebcd = timet_bcd(timenow);
		asyslog(LOG_INFO,"[任务上报时间%ld, %ld]: %04d:%02d:%02d %02d:%02d:%02d\n",list->nexttime, timenow,
				timebcd.year.data,timebcd.month.data,timebcd.day.data,
				timebcd.hour.data,timebcd.min.data,timebcd.sec.data);

		if (readCoverClass(0x6013, list->ID, &class6013, sizeof(CLASS_6013),coll_para_save) == 1)
		{
//			asyslog(LOG_INFO,"\ni=%d 任务【 %d 】 	 开始执行   上报方案编号【 %d 】",i,list->ID,list->SerNo);
			if (readCoverClass(0x601D, list->SerNo, &class601d, sizeof(CLASS_601D),coll_para_save) == 1)
			{
//				asyslog(LOG_INFO,"方案编号601d:reportnum=%d",class601d.reportnum);
				print_rcsd(class601d.reportdata.data.recorddata.csds);
				list->ReportNum = class601d.maxreportnum;
				list->OverTime = TItoSec(class601d.timeout);
				asyslog(LOG_INFO,"任务【 %d 】上报方案编号【 %d 】重发=%d,超时=%d",list->ID,list->SerNo,list->ReportNum,list->OverTime);
				fprintf(stderr,"list->SerNo = %d\n",list->SerNo);
				ret = GetReportData(class601d);// =1,=2, 数据组织好了
				fprintf(stderr,"GetReportData=%d\n",ret);
			}
			list->nexttime = calcnexttime(class6013.interval,class6013.startime,class6013.delay);
//			asyslog(LOG_INFO, "再次计算任务上报时间%ld, %ld\n", list->nexttime, timenow);
			timebcd = timet_bcd(list->nexttime);
			asyslog(LOG_INFO,"[计算下次上报%ld, %ld]: %04d:%02d:%02d %02d:%02d:%02d\n",list->nexttime, timenow,
					timebcd.year.data,timebcd.month.data,timebcd.day.data,
					timebcd.hour.data,timebcd.min.data,timebcd.sec.data);

		}else
		{
//				fprintf(stderr,"\n任务参数丢失！");
		}
	}
	return ret;
}

/*
 * 通讯进程循环调用 callAutoReport
 *  reportChoice =1:REPORTNOTIFICATIONLIST   =2:REPROTNOTIFICATIONRECORDLIST
 *  ifecho ：  0 没收到确认，或第一次调用    1 收到确认
 *  返回    :  1  需要继续发送   0 发送完成
 */
int callAutoReport(char *filename,INT8U reportChoice,CommBlock* com, INT8U ifecho)
{
	if (com==NULL || filename == NULL)
		return 0;
	INT8U *sendbuf = com->SendBuf;
	static INT8U piid=0;
	static int nowoffset = 0;
	static int nextoffset = 0;
	static int sendcounter =0;
	int datalen = 0, j=0,index=0 ,hcsi=0;//,apduplace=0;
	INT8U apdu_buf[MAXSIZ_FAM]={};
	int	apdu_index=0;
	CSINFO csinfo={};

	memset(TmpDataBuf,0,sizeof(TmpDataBuf));  //长度 1600
	if (ifecho == 1 )//上一次给确认了或者发送计数大于上报次数限制,通信也置位，默认主站收到报文
	{
		nowoffset = nextoffset;
		sendcounter = 0;
	}
	sendcounter++;
	datalen = 0;
//	fprintf(stderr,"\n当前偏移位置 nowoffset = %d  ",nowoffset);
	nextoffset = readFrameDataFile(filename,nowoffset,TmpDataBuf,&datalen);
	fprintf(stderr,"\n读出 (%d)：",datalen);
	for(j=0; j<datalen; j++)
	{
		if (j%20==0)fprintf(stderr,"\n");
		fprintf(stderr,"%02x ",TmpDataBuf[j]);
	}
//	fprintf(stderr,"\n下帧偏移位置 nextoffset = %d ",nextoffset);
	asyslog(LOG_INFO,"当前偏移nowoffset = %d  下帧偏移nextoffset = %d ",nowoffset,nextoffset);
	if (nextoffset == 0)
	{
		fprintf(stderr,"\n发送完毕！");
		nowoffset = 0;
		sendcounter = 0;
		return 0;
	}
	piid++;
	com->report_piid[0] = piid;		//处理无应答重复上报判断报文帧，目前未考虑多通道在线情况
	index = 0;
	if (fillcsinfo(&csinfo,com->serveraddr,com->taskaddr)==0)
		return 0;
	index = FrameHead(&csinfo,sendbuf);
	hcsi = index;
	index = index + 2;
//	apduplace = index;		//记录APDU 起始位置
	apdu_buf[apdu_index++] = REPORT_NOTIFICATION;
	apdu_buf[apdu_index++] = reportChoice;//	REPROTNOTIFICATIONRECORDLIST;
	apdu_buf[apdu_index++] = piid;	//PIID

	memcpy(&apdu_buf[apdu_index],TmpDataBuf,datalen);//将读出的数据拷贝
	apdu_index +=datalen;
	apdu_buf[apdu_index++] = 0;	//FollowReport
	apdu_buf[apdu_index++] = 0; //TimeTag
	if(com->f101.active == 1){
		//加密
		INT16U seclen=composeAutoReport(apdu_buf,apdu_index);
		if(seclen>0){
			sendbuf[index++]=16; //SECURIGY-REQUEST
			sendbuf[index++]=0;  //明文应用数据单元
			memcpy(&sendbuf[index],apdu_buf,seclen);
			index +=seclen;
		}
	}else {		//明文未测试
		memcpy(&sendbuf[index],apdu_buf,apdu_index);
		index +=apdu_index;
	}
	FrameTail(sendbuf,index,hcsi);

	if(com->p_send!=NULL)
		com->p_send(com->phy_connect_fd,sendbuf,index+3);

	return 1;
}

/*
 * 电表事件调用接口
 *  返回 : piid   =-1，错误
 */
int callEventAutoReport(CommBlock* com,INT8U *eventbuf,int datalen)
{
	INT8U *sendbuf = com->SendBuf;
	static INT8U piid=0;
	int 	index=0,hcsi=0,apduplace=0;
	CSINFO csinfo={};

	if ((com==NULL) || (datalen==0))
		return -1;
	if (fillcsinfo(&csinfo,com->serveraddr,com->taskaddr)==0)
		return 0;
	piid++;
	com->report_piid[0] = piid;		//处理无应答重复上报判断报文帧，目前未考虑多通道在线情况
	index = 0;		
	index = FrameHead(&csinfo,sendbuf);
	hcsi = index;
	index = index + 2;
	apduplace = index;		//记录APDU 起始位置
	sendbuf[index++] = REPORT_NOTIFICATION;
	sendbuf[index++] = REPROTNOTIFICATIONRECORDLIST;
	sendbuf[index++] = piid;	//PIID
	sendbuf[index++] = 1;	//sequence of A-RecordRow ,事件上送默认每次只上送一个事件记录
	memcpy(&sendbuf[index],eventbuf,datalen);//将读出的数据拷贝
	index +=datalen;
	sendbuf[index++] = 0;
	sendbuf[index++] = 0;
	FrameTail(sendbuf,index,hcsi);
	if(com->p_send!=NULL)
		com->p_send(com->phy_connect_fd,sendbuf,index+3);  //+3:crc1,crc2,0x16
	return piid;
}
