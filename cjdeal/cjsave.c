

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>
#include "sys/reboot.h"
#include <wait.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "cjsave.h"


extern pthread_mutex_t mutex_savetask;

void getFRZtime(CLASS_6013 class6013, CLASS_6015 class6015,TS ts,INT8U *buf,INT32U seqsec)
{
	int i=0;
	INT32U daysec=0;
	TS ts_lastday,ts_lastmon;
	ts_lastmon = ts_lastday= ts;//重新赋值，减一天用于日冻结存储时标赋值
	fprintf(stderr,"\nclass6015.savetimeflag=%d\n",class6015.savetimeflag);
	switch(class6015.savetimeflag)
	{
//	case 1:
//		buf[0] = (ts.Year&0xff00)>>8;
//		buf[1] = ts.Year&0x00ff;
//		buf[2] = ts.Month;
//		buf[3] = ts.Day;
//		buf[4] = ts.Hour;
//		buf[5] = ts.Minute;
//		buf[6] = ts.Sec;
//		break;
	case 2://相对当日零点零分
		buf[0] = (ts.Year&0xff00)>>8;
		buf[1] = ts.Year&0x00ff;
		buf[2] = ts.Month;
		buf[3] = ts.Day;
		buf[4] = 0;
		buf[5] = 0;
		buf[6] = 0;
		break;
	case 3://相对上日33点59分
		tminc(&ts_lastday,day_units,-1);
		buf[0] = (ts_lastday.Year&0xff00)>>8;
		buf[1] = ts_lastday.Year&0x00ff;
		buf[2] = ts_lastday.Month;
		buf[3] = ts_lastday.Day;
		buf[4] = 23;
		buf[5] = 59;
		buf[6] = 0;
		break;
	case 4://相对上日零点零分
		tminc(&ts_lastday,day_units,-1);
		buf[0] = (ts_lastday.Year&0xff00)>>8;
		buf[1] = ts_lastday.Year&0x00ff;
		buf[2] = ts_lastday.Month;
		buf[3] = ts_lastday.Day;
		buf[4] = 0;
		buf[5] = 0;
		buf[6] = 0;
		break;
	case 5://相对当月1日零点零分
		buf[0] = (ts.Year&0xff00)>>8;
		buf[1] = ts.Year&0x00ff;
		buf[2] = ts.Month;
		buf[3] = 1;
		buf[4] = 0;
		buf[5] = 0;
		buf[6] = 0;
		break;
	case 7://相当上月末0点0分
		ts_lastmon.Day = 1;
		tminc(&ts_lastmon,day_units,-1);//上月末
		buf[0] = (ts_lastmon.Year&0xff00)>>8;
		buf[1] = ts_lastmon.Year&0x00ff;
		buf[2] = ts_lastmon.Month;
		buf[3] = ts_lastmon.Day;
		buf[4] = 0;
		buf[5] = 0;
		buf[6] = 0;
		break;
	default:
		fprintf(stderr,"\n%d:%d:%d daysec=%d\n",ts.Hour,ts.Minute,ts.Sec,daysec);
		daysec = ts.Hour*3600+ts.Minute*60+ts.Sec;
		daysec = daysec/seqsec;
		daysec = daysec*seqsec;
		fprintf(stderr,"\ndaysec=%d seqsec=%d\n",daysec,seqsec);
		buf[0] = (ts.Year&0xff00)>>8;
		buf[1] = ts.Year&0x00ff;
		buf[2] = ts.Month;
		buf[3] = ts.Day;
		buf[4] = daysec/3600;
		buf[5] = daysec%3600/60;
		buf[6] = daysec%60;
		break;
	}
	fprintf(stderr,"\nbuf:");
	for(i=0;i<7;i++)
		fprintf(stderr," %02x",buf[i]);
	fprintf(stderr,"\n");
}
void groupFIXEDdate(HEAD_UNIT *oad_unit)
{
	memset(oad_unit,0x00,sizeof(oad_unit));

	memset(&oad_unit[0].oad_m,0x00,sizeof(OAD));
	oad_unit[0].oad_r.OI = 0x6042;
	oad_unit[0].oad_r.attrindex = 0;
	oad_unit[0].oad_r.attflg = 2;
	oad_unit[0].len = 8;

	memset(&oad_unit[1].oad_m,0x00,sizeof(OAD));
	oad_unit[1].oad_r.OI = 0x6041;
	oad_unit[1].oad_r.attrindex = 0;
	oad_unit[1].oad_r.attflg = 2;
	oad_unit[1].len = 8;

	memset(&oad_unit[2].oad_m,0x00,sizeof(OAD));
	oad_unit[2].oad_r.OI = 0x6040;
	oad_unit[2].oad_r.attrindex = 0;
	oad_unit[2].oad_r.attflg = 2;
	oad_unit[2].len = 8;
}
void groupFIXEDevent(HEAD_UNIT *oad_unit,ROAD eve_oad)
{
	//{0x20,0x22,0x02,0x00},{0x20,0x1e,0x02,0x00},{0x20,0x20,0x02,0x00}
	//{0x0005,               0x0008,               0x0008};
	memset(oad_unit,0x00,sizeof(oad_unit));

	memcpy(&oad_unit[0].oad_m,&eve_oad.oad,sizeof(OAD));
	oad_unit[0].oad_r.OI = 0x2022;
	oad_unit[0].oad_r.attrindex = 0;
	oad_unit[0].oad_r.attflg = 2;
	oad_unit[0].len = 5;

	memcpy(&oad_unit[1].oad_m,&eve_oad.oad,sizeof(OAD));
	oad_unit[1].oad_r.OI = 0x201e;
	oad_unit[1].oad_r.attrindex = 0;
	oad_unit[1].oad_r.attflg = 2;
	oad_unit[1].len = 8;

	memcpy(&oad_unit[2].oad_m,&eve_oad.oad,sizeof(OAD));
	oad_unit[2].oad_r.OI = 0x2020;
	oad_unit[2].oad_r.attrindex = 0;
	oad_unit[2].oad_r.attflg = 2;
	oad_unit[2].len = 8;
}
void getOneUnit(HEAD_UNIT *headbuf,OAD oad_m,OAD oad_r,INT16U len)
{
	HEAD_UNIT	headunit={};

	memcpy(&headunit.oad_m,&oad_m,sizeof(OAD));
	memcpy(&headunit.oad_r,&oad_r,sizeof(OAD));
	headunit.len = len;
	memcpy(headbuf,&headunit,sizeof(HEAD_UNIT));
}
/*
 * 结构为6个字节长度+TSA(0x00+202a0200+2个字节长度)+3*时标+CSD
 * unitlen_z长度为此任务当日需要抄的全部数据长度，以此将一个测量点一天的数据放在一个地方
 */
void CreateSaveHead(char *fname,CSD_ARRAYTYPE csds,HEADFIXED_INFO *taskhead_info,HEAD_UNIT *headoad_unit,INT32U seqnum,INT32U seqsec)
{
	INT16U len_tmp=0;
	int i=0,j=0;
	OAD oad_m;
	FILE *fp = NULL;
	memset(&oad_m,0,sizeof(OAD));


	if(csds.num == 0xee || csds.num == 0)
	{
		asyslog(LOG_WARNING, "cjsave :csds.num=%d,return!!!error!!!",csds.num);
		return;
	}
	taskhead_info->seqnum = seqnum;
	taskhead_info->seqsec = seqsec;

	//文件头的oad单元前三个oad固定，为3个时标
	groupFIXEDdate(headoad_unit);//组固定的三个时标
	taskhead_info->oadnum = 3;
	taskhead_info->reclen = 24;//3*8
	fprintf(stderr,"\n时标长度:%d\n",taskhead_info->reclen);//时标长度

	if(csds.num > MY_CSD_NUM)//超了
		csds.num = MY_CSD_NUM;
	for(i=0;i<csds.num;i++)
	{
		if(csds.csd[i].type == 0xee)
			break;
		if(csds.csd[i].type != 0 && csds.csd[i].type != 1)
			continue;
		if(csds.csd[i].type == 0)	//OAD
		{
			if(csds.csd[i].csd.oad.OI == 0x202a || csds.csd[i].csd.oad.OI == 0x6040 ||
					csds.csd[i].csd.oad.OI == 0x6041 || csds.csd[i].csd.oad.OI == 0x6042)
			{
				fprintf(stderr,"OAD=%04x_%02x%02x 存在固定头，不计算长度\n",csds.csd[i].csd.oad.OI,csds.csd[i].csd.oad.attflg,csds.csd[i].csd.oad.attrindex);
				continue;
			}
			 len_tmp = CalcOIDataLen(csds.csd[i].csd.oad);//多一个数据类型
			 taskhead_info->reclen += len_tmp;
			 getOneUnit(&headoad_unit[taskhead_info->oadnum],oad_m,csds.csd[i].csd.oad,len_tmp);
			 taskhead_info->oadnum++;
		}else if(csds.csd[i].type == 1)	//ROAD
		{
			if(csds.csd[i].csd.road.num == 0xee)
				continue;
			if(csds.csd[i].csd.road.num > ROAD_OADS_NUM)//超了
				csds.csd[i].csd.road.num = ROAD_OADS_NUM;
			for(j=0;j<csds.csd[i].csd.road.num;j++)
			{
				if(csds.csd[i].csd.road.oads[j].OI == 0xeeee)
					break;
				len_tmp = CalcOIDataLen(csds.csd[i].csd.road.oads[j]);//多一个数据类型
				taskhead_info->reclen += len_tmp;
				getOneUnit(&headoad_unit[taskhead_info->oadnum],csds.csd[i].csd.road.oad,csds.csd[i].csd.road.oads[j],len_tmp);
				taskhead_info->oadnum++;
			}
		}
	}
	fp = fopen(fname,"w+");
	fwrite(taskhead_info,sizeof(HEADFIXED_INFO),1,fp);
	fwrite(headoad_unit,sizeof(HEAD_UNIT),taskhead_info->oadnum,fp);
	if(fp != NULL)
		fclose(fp);
}
void CreateSaveEventHead(char *fname,ROAD eve_oad,HEADFIXED_INFO *taskhead_info,HEAD_UNIT *headoad_unit)
{
	INT16U len_tmp=0;
	int i=0;
	FILE *fp = NULL;
	if(eve_oad.num == 0xee || eve_oad.num == 0)
	{
		asyslog(LOG_WARNING, "cjsave eve:csds.num=%d,return!!!error!!!",eve_oad.num);
		return;
	}
	taskhead_info->seqnum = 1;//只存储一个
	taskhead_info->seqsec = 0;//秒数无效

	//文件头的oad单元前三个oad固定，为3个时标
	groupFIXEDevent(headoad_unit,eve_oad);//组固定的三个时标
	taskhead_info->oadnum = 3;
	taskhead_info->reclen = 21;//2*8+5
	fprintf(stderr,"\n时标长度:%d\n",taskhead_info->reclen);//时标长度

	if(eve_oad.num > ROAD_OADS_NUM)//超了
		eve_oad.num = ROAD_OADS_NUM;
	for(i=0;i<eve_oad.num;i++)
	{
		if(eve_oad.oads[i].OI == 0xeeee)
			break;
		if(eve_oad.oads[i].OI == 0x2020 || eve_oad.oads[i].OI == 0x2022 || eve_oad.oads[i].OI == 0x201e)
			continue;
		len_tmp = CalcOIDataLen(eve_oad.oads[i]);//多一个数据类型
		taskhead_info->reclen += len_tmp;
		getOneUnit(&headoad_unit[taskhead_info->oadnum],eve_oad.oad,eve_oad.oads[i],len_tmp);
		taskhead_info->oadnum++;
	}
	fp = fopen(fname,"w+");
	fwrite(taskhead_info,sizeof(HEADFIXED_INFO),1,fp);
	fwrite(headoad_unit,sizeof(HEAD_UNIT),taskhead_info->oadnum,fp);
	if(fp != NULL)
		fclose(fp);
}
void TStoDATEBCD(TS ts,INT8U *buf)
{
	buf[0] = (ts.Year&0xff00)>>8;
	buf[1] = ts.Year&0x00ff;
	buf[2] = ts.Month;
	buf[3] = ts.Day;
	buf[4] = ts.Hour;
	buf[5] = ts.Minute;
	buf[6] = ts.Sec;
}
void fillRECdata(OADDATA_SAVE *OADdata,INT8U OADnum,INT8U *databuf,HEADFIXED_INFO taskhead_info,HEAD_UNIT *headoad_unit,TS OADts,CLASS_6013 class6013,CLASS_6015 class6015)
{
	int i=0,j=0,mm=0;
	INT16U indexn = 0;

//	pos6042 = 0;//记录第一个位置为6042，存储时间
//	pos6041 = 1+sizeof(DateTimeBCD);//记录第二个位置为6041
//	pos6040 = 2*(1+sizeof(DateTimeBCD));//记录第三个位置为6041
	//组记录头
	fprintf(stderr,"\n---------------------------databuf[0]=%d class6015.savetimeflag=%d\n",databuf[0],class6015.savetimeflag);
	if(databuf[0] == 0)// && class6015.savetimeflag != 6)//采集开始时间为空 6表示冻结时标取抄表上来的6042
	{//lyl 修改，没有抄上时标，先自组一个时标
		databuf[0]=0x1c;
		getFRZtime(class6013,class6015,OADts,&databuf[1],taskhead_info.seqsec);
	}

	if(databuf[16] == 0)//采集成功时间为空
	{
		databuf[8]=0x1c;
		TStoDATEBCD(OADts,&databuf[9]);
	}
	if(databuf[16] == 0)//采集开始时间为空
	{
		databuf[16]=0x1c;
		TStoDATEBCD(OADts,&databuf[17]);
	}
	indexn = 24;
	fprintf(stderr,"\nindexn = %d\n",indexn);

	if(taskhead_info.oadnum>100)
		taskhead_info.oadnum=100;//最多存储100个oads
	for(i=0;i<taskhead_info.oadnum;i++)//开头三个时标
	{
//		if(headoad_unit[i].oad_r.OI == 0x6040 || headoad_unit[i].oad_r.OI == 0x6041 ||//抄表给三个时标
//				headoad_unit[i].oad_r.OI == 0x6042)
//			continue;
		fprintf(stderr,"\n---------%d:0x%04x-0x%04x\n",i,headoad_unit[i].oad_m.OI,headoad_unit[i].oad_r.OI);
		for(j=0;j<OADnum;j++)
		{
			fprintf(stderr,"\n%d:0x%04x-0x%04x\n",j,OADdata[j].oad_m.OI,OADdata[j].oad_r.OI);
			if(headoad_unit[i].oad_m.OI == OADdata[j].oad_m.OI &&
					headoad_unit[i].oad_r.OI == OADdata[j].oad_r.OI)
			{
				fprintf(stderr,"\nclass6015.savetimeflag == %d headoad_unit[i].oad_r.OI = %04x\n",
						class6015.savetimeflag,headoad_unit[i].oad_r.OI);
				if(class6015.savetimeflag == 6 && headoad_unit[i].oad_r.OI == 0x2021)
					memcpy(&databuf[0],OADdata[j].data,8);
				fprintf(stderr,"\nOADdata[%d].data(%d):",j,OADdata[j].datalen);
				for(mm=0;mm<OADdata[j].datalen;mm++)
					fprintf(stderr," %02x",OADdata[j].data[mm]);
				fprintf(stderr,"\n");

				memcpy(&databuf[indexn],OADdata[j].data,headoad_unit[i].len);

				if(headoad_unit[i].len != OADdata[j].datalen)
					fprintf(stderr,"\n长度不符，按照文件头长度存储 %d %d\n",headoad_unit[i].len,OADdata[j].datalen);
				break;
			}
		}
		indexn += headoad_unit[i].len;
	}
	fprintf(stderr,"\ndatabuf(%d):",indexn);
	for(mm=0;mm<indexn;mm++)
		fprintf(stderr," %02x",databuf[mm]);
	fprintf(stderr,"\n");
}
void fillEVEdata(OADDATA_SAVE *OADdata,INT8U OADnum,INT8U *databuf,HEADFIXED_INFO taskhead_info,HEAD_UNIT *headoad_unit)
{
	int i=0,j=0,mm=0;
	INT16U index = 0;

	if(taskhead_info.oadnum>100)
		taskhead_info.oadnum=100;//最多存储100个oads
	for(i=0;i<taskhead_info.oadnum;i++)
	{
		fprintf(stderr,"\n查找%d: 0x%04x-0x%04x\n",i,headoad_unit[i].oad_m.OI,headoad_unit[i].oad_r.OI);
		for(j=0;j<OADnum;j++)
		{
			fprintf(stderr,"\n%d:0x%04x-0x%04x\n",i,OADdata[i].oad_m.OI,OADdata[i].oad_r.OI);
			if((headoad_unit[i].oad_m.OI == OADdata[j].oad_m.OI/* ||
					OADdata[j].oad_r.OI == 0x2020 || OADdata[j].oad_r.OI == 0x2022 || OADdata[j].oad_r.OI == 0x201e*/) &&
					headoad_unit[i].oad_r.OI == OADdata[j].oad_r.OI)
			{
				fprintf(stderr,"\nOADdata[%d].data(%d):",j,OADdata[j].datalen);
				for(mm=0;mm<OADdata[j].datalen;mm++)
					fprintf(stderr," %02x",OADdata[j].data[mm]);
				fprintf(stderr,"\n");

				memcpy(&databuf[index],OADdata[j].data,headoad_unit[i].len);

				if(headoad_unit[i].len != OADdata[j].datalen)
					fprintf(stderr,"\n长度不符，按照文件头长度存储 %d %d\n",headoad_unit[i].len,OADdata[j].datalen);
				break;
			}
		}
		index += headoad_unit[i].len;
		fprintf(stderr,"\nindex=%d len=%d\n",index,headoad_unit[i].len);
	}
//	fprintf(stderr,"\ndatabuf(%d):",index);
//	for(mm=0;mm<index;mm++)
//		fprintf(stderr," %02x",databuf[mm]);
//	fprintf(stderr,"\n");
}
//INT8U get60136015info(INT8U taskid,CLASS_6015 *class6015,CLASS_6013 *class6013)
//{
//	int i=0;
//	memset(class6013,0,sizeof(CLASS_6013));
//	memset(class6015,0,sizeof(CLASS_6015));
//	if(readCoverClass(0x6013,taskid,class6013,sizeof(CLASS_6013),coll_para_save) == 1)
//	{
//		if(class6013->cjtype != 1 || class6013->state != 1)//
//			return 0;
//		if(readCoverClass(0x6015,class6013->sernum,class6015,sizeof(CLASS_6015),coll_para_save) == 1)
//		{
//			for(i=0;i<MY_CSD_NUM;i++)
//			{
//				switch(class6015->csds.csd[i].csd.road.oad.OI)//union
//				{
//				case 0x0000://
//				case 0x5000:
//				case 0x5002:
//				case 0x5003://
//					break;
//				case 0x5004://日冻结
//				case 0x5005://结算日
//					return 1;
//					break;
//				case 0x5006://月冻结
//					return 2;
//					break;
//				case 0x5007://年冻结
//					return 3;
//					break;
//				default:break;
//				}
//			}
//			return 4;//实时
//		}
//	}
//	else
//		return 0;
//	return 4;//实时或曲线
//}
int getTItoSec(TI ti)
{
	int  sec = 0;
	switch(ti.units)
	{
		case sec_units://秒
			sec = ti.interval;
			break;
		case minute_units://分
			sec = ti.interval * 60;
			break;
		case hour_units://时
			sec =  ti.interval * 3600;
			break;
		default:
			break;
	}
	fprintf(stderr,"get TI(%d-%d) sec=%d\n",ti.units,ti.interval,sec);
	return sec;
}
//INT32U getTASKruntimes(CLASS_6013 class6013,CLASS_6015 class6015,INT32U *seqsec)//计算任务每天抄读次数
//{
//	INT32U runtimes=0,seqsecond=0,taskdaysec=0;//频率秒数 任务一天的活跃秒数
//	TI ti_tmp;
//	fprintf(stderr,"\n任务执行区间end:%d-%d,start:%d-%d\n",
//			class6013.runtime.runtime[0].endHour,class6013.runtime.runtime[0].endMin,
//			class6013.runtime.runtime[0].beginHour,class6013.runtime.runtime[0].beginMin);
//	taskdaysec = ((class6013.runtime.runtime[0].endHour*60+class6013.runtime.runtime[0].endMin) -
//			(class6013.runtime.runtime[0].beginHour*60+class6013.runtime.runtime[0].beginMin))*60;
//	fprintf(stderr,"\n任务一天执行的秒数taskdaysec=%d\n",taskdaysec);
//	if(taskdaysec<=0)
//		return 0;//任务设置不合理
//	fprintf(stderr,"\n--------------cjtype=%d\n",class6015.cjtype);
//	switch(class6015.cjtype)
//	{
//	case 0://当前数据 存储次数由任务执行频率决定
//		fprintf(stderr,"\nunit:%d interval:%d\n",class6013.delay.units,class6013.delay.interval);
//		seqsecond = getTItoSec(class6013.interval);
//		if(seqsecond==0)
//			runtimes = 1;
//		else
//			runtimes = taskdaysec/seqsecond+1;
//		break;
//	case 1://采集上第n次
////		runtimes=class6015.data.data[1];//data[0]应该是类型？
////		break;
//	case 2://按冻结时标采集
//		runtimes = 1;
//		break;
//	case 3://按时标间隔采集
//		ti_tmp.interval = (class6015.data.data[1]<<8) + class6015.data.data[2];
//		ti_tmp.units = class6015.data.data[0];
//		seqsecond = getTItoSec(ti_tmp);
//		if(seqsecond==0)
//			runtimes = 1;
//		else
//			runtimes = taskdaysec/seqsecond+1;
//		break;
//	case 4://补抄
//		break;
//	default:
//		runtimes = 1;
//		break;
//	}
//	fprintf(stderr,"\taskdaysec=%d\n",taskdaysec);
//	if(seqsecond==0)
//		seqsecond = taskdaysec;
//	*seqsec = seqsecond;
//	fprintf(stderr,"\nseqsecond=%d,*seqsec=%d\n",seqsecond,*seqsec);
//	return runtimes;
//}
int SaveOADData(INT8U taskid,OAD oad_m,OAD oad_r,INT8U *databuf,int datalen,TS ts_res)
{
	TSA tsa;
	OADDATA_SAVE OADdata;
	fprintf(stderr,"\nSaveOADData databuf(%d)\n",datalen);
	PRTbuf(databuf,datalen);
	if(datalen <= TSA_LEN+1)
	{
		fprintf(stderr,"\nSaveOADData 长度错误!!!\n");
		return 0;
	}
	OADdata.oad_m = oad_m;
	OADdata.oad_r = oad_r;
	OADdata.datalen = datalen-TSA_LEN-1;
	memcpy(OADdata.data,&databuf[sizeof(TSA)+1],OADdata.datalen);
	memcpy(tsa.addr,&databuf[1],TSA_LEN);
	saveREADOADdata(taskid,tsa,&OADdata,1,ts_res);
	return OADdata.datalen;
}
void getCsdsInfo(INT8U *oadnum,INT32U *reclen,CSD_ARRAYTYPE csds)
{
	INT8U oad_num=0;
	INT32U rec_len=0,len_tmp=0;
	int i=0,j=0;
	if(csds.num > MY_CSD_NUM)//超了
		csds.num = MY_CSD_NUM;
	for(i=0;i<csds.num;i++)
	{
		if(csds.csd[i].type == 0xee)
			break;
		if(csds.csd[i].type != 0 && csds.csd[i].type != 1)
			continue;
		if(csds.csd[i].type == 0)	//OAD
		{
			if(csds.csd[i].csd.oad.OI == 0x202a || csds.csd[i].csd.oad.OI == 0x6040 ||
					csds.csd[i].csd.oad.OI == 0x6041 || csds.csd[i].csd.oad.OI == 0x6042)
			{
				fprintf(stderr,"OAD=%04x_%02x%02x 存在固定头，不计算长度\n",csds.csd[i].csd.oad.OI,csds.csd[i].csd.oad.attflg,csds.csd[i].csd.oad.attrindex);
				continue;
			}
			 len_tmp = CalcOIDataLen(csds.csd[i].csd.oad);//多一个数据类型
			 rec_len += len_tmp;
			 oad_num++;
		}else if(csds.csd[i].type == 1)	//ROAD
		{
			if(csds.csd[i].csd.road.num == 0xee)
				continue;
			if(csds.csd[i].csd.road.num > ROAD_OADS_NUM)//超了
				csds.csd[i].csd.road.num = ROAD_OADS_NUM;
			for(j=0;j<csds.csd[i].csd.road.num;j++)
			{
				if(csds.csd[i].csd.road.oads[j].OI == 0xeeee)
					break;
				len_tmp = CalcOIDataLen(csds.csd[i].csd.road.oads[j]);//多一个数据类型
				rec_len += len_tmp;
				oad_num++;
			}
		}
	}
	*oadnum=oad_num;
	*reclen=rec_len;
}
void saveREADOADdata(INT8U taskid,TSA tsa,OADDATA_SAVE *OADdata,INT8U OADnum,TS OADts)
{
	pthread_mutex_lock(&mutex_savetask);
#if 1
	DbgPrintToFile1(1,"\n##############taskid　＝　%d saveREADOADdata = %d　TSA=%02x %02x %02x %02x %02x %02x %02x %02x",
			taskid,OADnum,tsa.addr[0],tsa.addr[1],tsa.addr[2],tsa.addr[3],tsa.addr[4],tsa.addr[5],tsa.addr[6],tsa.addr[7]);
	fprintf(stderr,"\n##############saveREADOADdata = %d",OADnum);
	INT8U prtIndex = 0;
	for(prtIndex = 0;prtIndex < OADnum;prtIndex++)
	{
		fprintf(stderr,"\nOADdata[%d] oad_m = %04x oad_r =  %04x data[%d] =",
						prtIndex,OADdata[prtIndex].oad_m.OI,OADdata[prtIndex].oad_r.OI,OADdata[prtIndex].datalen);
		DbgPrintToFile1(1,"\nOADdata[%d] oad_m = %04x oad_r =  %04x data[%d] = %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
				prtIndex,OADdata[prtIndex].oad_m.OI,OADdata[prtIndex].oad_r.OI,OADdata[prtIndex].datalen,
				OADdata[prtIndex].data[0],OADdata[prtIndex].data[1],OADdata[prtIndex].data[2],OADdata[prtIndex].data[3]
				,OADdata[prtIndex].data[4],OADdata[prtIndex].data[5],OADdata[prtIndex].data[6],OADdata[prtIndex].data[7]
				,OADdata[prtIndex].data[8],OADdata[prtIndex].data[9],OADdata[prtIndex].data[10],OADdata[prtIndex].data[11]);
		INT8U prtIndex1 = 0;
		for(prtIndex1 = 0;prtIndex1 < OADdata[prtIndex].datalen;prtIndex1++)
		{
			fprintf(stderr,"%02x ", OADdata[prtIndex].data[prtIndex1]);
		}
	}
#endif
	INT8U frz_type=0,findtsa=0,oadnum=0,tsadata[18],data_buf[2048];
	INT16U curseq=0;
	INT32U blklen=0,headlen=0,reclen=0,seqsec=0,seqnum=0,oadsec=0;
	int i=0;
	long int savepos=0;

	FILE *fp = NULL;
	char fname[FILENAMELEN]={},cmd[FILENAMELEN]={};
	CLASS_6015	class6015={};
	CLASS_6013	class6013={};
	HEADFIXED_INFO taskhead_info;
	HEAD_UNIT headoad_unit[FILEOADMAXNUM];//任务最多设置100个有效oad(包括6040 6041 6042 不包括202a)，可扩展
	TS ts_oad,ts_file;
	ts_file = ts_oad = OADts;
	memset(&taskhead_info,0x00,sizeof(HEADFIXED_INFO));

	if((frz_type = get60136015info(taskid,&class6015,&class6013))==0)//获取任务参数
	{
		fprintf(stderr,"\n获取任务%d配置失败\n",taskid);
		pthread_mutex_unlock(&mutex_savetask);
		return;
	}
	fprintf(stderr,"\n任务%d存储类型为%d\n",taskid,frz_type);
	seqnum = getTASKruntimes(class6013,class6015,&seqsec);
	fprintf(stderr,"\n每天存储记录个数seqnum=%d,seqsec=%d\n",seqnum,seqsec);
	if(seqnum == 0)
	{
		pthread_mutex_unlock(&mutex_savetask);
		return;
	}

	oadsec = OADts.Hour*60*60+OADts.Minute*60+OADts.Sec;
	getFILEts(frz_type,&ts_file);
	getTaskFileName(taskid,ts_file,fname);
	fprintf(stderr,"\n文件存储时间%04d-%02d-%02d %02d:%02d:%02d  存储文件 %s\n",
			OADts.Year,OADts.Month,OADts.Day,OADts.Hour,OADts.Minute,OADts.Sec,fname);
	if(access(fname,F_OK)!=0)
	{
		fprintf(stderr,"\n创建文件头\n");
		CreateSaveHead(fname,class6015.csds,&taskhead_info,headoad_unit,seqnum,seqsec);//写文件头信息并返回
	}
	else
	{
		fp = fopen(fname,"r");//读格式
		fread(&taskhead_info,sizeof(HEADFIXED_INFO),1,fp);
		fread(headoad_unit,taskhead_info.oadnum*sizeof(HEAD_UNIT),1,fp);
		fclose(fp);
		getCsdsInfo(&oadnum,&reclen,class6015.csds);
		if(seqnum!=taskhead_info.seqnum || seqsec!=taskhead_info.seqsec
				|| oadnum+3!=taskhead_info.oadnum || reclen+24!=taskhead_info.reclen)//任务变更 todo 再加上oad不再这个方案里
		{
			asyslog(LOG_INFO,"删除原来任务%d文件，创建文件头 任务信息:%d %d %d %d 文件中信息:%d %d %d %d",taskid,
					seqnum,seqsec,oadnum+3,reclen+24,
					taskhead_info.seqnum,taskhead_info.seqsec,taskhead_info.oadnum,taskhead_info.reclen);
			fprintf(stderr,"\n删除原来任务，创建文件头\n");
			sprintf(cmd,"rm -r /nand/task/%02d",taskid);
			system(cmd);
			CreateSaveHead(fname,class6015.csds,&taskhead_info,headoad_unit,seqnum,seqsec);//写文件头信息并返回
		}
	}
	blklen = taskhead_info.reclen*taskhead_info.seqnum;
	headlen = sizeof(HEADFIXED_INFO)+taskhead_info.oadnum*sizeof(HEAD_UNIT);
	fprintf(stderr,"\nreclen=%d,seqnum=%d,seqsec=%d,blklen=%d,oadnum=%d\n",taskhead_info.reclen,seqnum,seqsec,blklen,taskhead_info.oadnum);

	fp = fopen(fname,"a+");//格式;//格式
	fseek(fp,headlen,SEEK_SET);//跳过文件头
	while(!feof(fp))//轮寻文件
	{
		fprintf(stderr,"\n---1--%d--%d--%d\n",sizeof(TSA)+1,sizeof(tsadata),ftell(fp));
		if(fread(tsadata,sizeof(TSA)+1,1,fp)==0)//0x55 + tsa
		{
			fprintf(stderr,"\n文件尾\n");
			break;
		}
		else
			fprintf(stderr,"\n---2\n");
		prtTSA(tsa);
		if(memcmp(&tsadata[1],tsa.addr,TSA_LEN)==0 && tsadata[0] == 0x55)//找到了存储结构的位置，一个存储结构可能含有unitnum个单元
		{
			savepos = ftell(fp);//位置为跳过最开头的tsa的位置，即符合tsa的第一条记录的位置
			findtsa = 1;
			break;
		}
		else
			fseek(fp,blklen,SEEK_CUR);//地址不匹配
	}
	memset(tsadata,0x00,sizeof(tsadata));
	memset(data_buf,0x00,sizeof(data_buf));
	fprintf(stderr,"\noadsec=%d seqsec=%d\n",oadsec,seqsec);
	if(seqnum == 1)
		curseq = 0;
	else {
		if(seqsec==0)
			curseq = 0;
		else
			curseq = oadsec/seqsec;//从0开始,计算要存的记录
	}

	fprintf(stderr,"\ncurseq=%d\n",curseq);
	if(findtsa == 0)//没找到，在文件尾添加这个测量点的空记录
	{
		fseek(fp,0,SEEK_END);//定位到文件尾
		tsadata[0]=0x55;
		memcpy(&tsadata[1],tsa.addr,sizeof(tsa.addr));
		fwrite(tsadata,sizeof(tsadata),1,fp);//记录最开始的tsa写入文件
		savepos = ftell(fp)+curseq*taskhead_info.reclen;//位置为跳过最开头的tsa的末尾位置加上要定位的前面的curseq条记录
		for(i=0;i<taskhead_info.seqnum;i++)
			fwrite(data_buf,taskhead_info.reclen,1,fp);//写入空记录
	}
	else//找到了匹配的TSA
	{
		savepos = ftell(fp)+curseq*taskhead_info.reclen;//位置为跳过最开头的tsa的末尾位置加上要定位的前面的curseq条记录
		fseek(fp,savepos,SEEK_SET);//跳到要覆盖的记录的位置
		fread(data_buf,taskhead_info.reclen,1,fp);
	}

	fillRECdata(OADdata,OADnum,data_buf,taskhead_info,headoad_unit,OADts,class6013,class6015);//把数据组好，放入缓存

	fprintf(stderr,"\nsavepos=%d data_buf(%d):",savepos,taskhead_info.reclen);
	for(i=0;i<taskhead_info.reclen;i++)
		fprintf(stderr," %02x",data_buf[i]);
	fprintf(stderr,"\n");

	if(fp != NULL)
		fclose(fp);
	fp = fopen(fname,"r+");//格式
	fseek(fp,savepos,SEEK_SET);
	fwrite(data_buf,taskhead_info.reclen,1,fp);//记录覆盖方式存入文件
	if(fp!=NULL)
		fclose(fp);

	pthread_mutex_unlock(&mutex_savetask);

}
INT8U geteveno(OADDATA_SAVE *OADdata,INT8U OADnum,INT8U *eveno)
{
	int i = 0;
	for(i=0;i<OADnum;i++)
	{
		if(OADdata[i].oad_r.OI == 0x2022)
		{
			memcpy(eveno,OADdata[i].data,5);
			return 1;
		}
	}
	return 0;
}
//事件存储
INT16U saveREADOADevent(ROAD eve_road,TSA tsa,OADDATA_SAVE *OADdata,INT8U OADnum,TS OADts)
{
#if 1
	prtTSA(tsa);
	DbgPrintToFile1(1,"\n##############saveREADOADevent = %d OI = %04x",OADnum,eve_road.oad.OI);
	fprintf(stderr,"\n##############saveREADOADevent = %d",OADnum);
	INT8U prtIndex = 0;
	for(prtIndex = 0;prtIndex < OADnum;prtIndex++)
	{
		fprintf(stderr,"\nOADdata[%d] oad_m = %04x oad_r =  %04x data[%d] =",
						prtIndex,OADdata[prtIndex].oad_m.OI,OADdata[prtIndex].oad_r.OI,OADdata[prtIndex].datalen);
		DbgPrintToFile1(1,"\nOADdata[%d] oad_m = %04x oad_r =  %04x data[%d] = %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
				prtIndex,OADdata[prtIndex].oad_m.OI,OADdata[prtIndex].oad_r.OI,OADdata[prtIndex].datalen,
				OADdata[prtIndex].data[0],OADdata[prtIndex].data[1],OADdata[prtIndex].data[2],OADdata[prtIndex].data[3]
				,OADdata[prtIndex].data[4],OADdata[prtIndex].data[5],OADdata[prtIndex].data[6],OADdata[prtIndex].data[7]
				,OADdata[prtIndex].data[8],OADdata[prtIndex].data[9],OADdata[prtIndex].data[10],OADdata[prtIndex].data[11]);
		INT8U prtIndex1 = 0;
		for(prtIndex1 = 0;prtIndex1 < OADdata[prtIndex].datalen;prtIndex1++)
		{
			fprintf(stderr,"%02x ", OADdata[prtIndex].data[prtIndex1]);
		}
	}
#endif
	INT8U evehappen=0,databuf[1024],tsadata[18],eveno[5],fir_save=0;
	INT32U blklen=0,headlen=0,indexn=0;
	int i = 0;
	long int savepos=0;
	HEAD_UNIT headoad_unit[FILEOADMAXNUM];//任务最多设置100个有效oad(包括2022 201e 2020 不包括202a)，可扩展
	HEADFIXED_INFO taskhead_info;
	FILE *fp = NULL;
	char fname[FILENAMELEN]={};
	getEveFileName(eve_road.oad.OI,fname);//创建eve文件

	if(geteveno(OADdata,OADnum,eveno)==0)//得到事件序号
	{
		DbgPrintToFile1(1,"没有事件序号!!!!");
		fprintf(stderr,"\n没有事件序号!!!!\n");
		return 0;
	}

	fp = fopen(fname,"r");
	if(fp == NULL)//文件没内容 组文件头，如果文件已存在，提取文件头信息
	{
		fprintf(stderr,"\n创建事件文件头\n");
		DbgPrintToFile1(1,"创建事件文件头");
		CreateSaveEventHead(fname,eve_road,&taskhead_info,headoad_unit);//写文件头信息并返回
		evehappen = 1;//第一次存储，产生事件
		savepos = sizeof(HEADFIXED_INFO)+taskhead_info.oadnum*sizeof(HEAD_UNIT);
		fir_save = 1;
		fprintf(stderr,"\n1第一次存储，产生事件  savepos=%d\n",savepos);
		DbgPrintToFile1(1,"1第一次存储，产生事件  savepos=%d",savepos);
	}
	else
	{
		fp = fopen(fname,"a+");//读格式
		fread(&taskhead_info,sizeof(HEADFIXED_INFO),1,fp);
		fread(headoad_unit,taskhead_info.oadnum*sizeof(HEAD_UNIT),1,fp);
		blklen = taskhead_info.reclen;
		headlen = sizeof(HEADFIXED_INFO)+taskhead_info.oadnum*sizeof(HEAD_UNIT);
		fprintf(stderr,"\neve:reclen=%d,blklen=%d,oadnum=%d,headlen=%d\n",taskhead_info.reclen,blklen,taskhead_info.oadnum,headlen);

		fseek(fp,headlen,SEEK_SET);//跳过文件头
		while(!feof(fp))//轮寻文件
		{
			fprintf(stderr,"\n---1--%d--%d--%d\n",sizeof(TSA)+1,sizeof(tsadata),ftell(fp));
			if(fread(tsadata,sizeof(TSA)+1,1,fp)==0)//0x55 + tsa
			{
				fprintf(stderr,"\n文件尾\n");
				break;
			}
			else
				fprintf(stderr,"\n---2\n");
			prtTSA(tsa);
			PRTbuf(tsadata,TSA_LEN+1);
			if(memcmp(&tsadata[1],tsa.addr,TSA_LEN)==0 && tsadata[0] == 0x55)//找到了存储结构的位置，一个存储结构可能含有unitnum个单元
			{
				savepos = ftell(fp);//位置为跳过最开头的tsa的位置，即符合tsa的第一条记录的位置
				fprintf(stderr,"\n找到了tsa savepos=%d\n",savepos);

				fread(databuf,blklen,1,fp);
				if(memcmp(&databuf,&eveno[18],5)==0)//比对事件记录序号是否相同，相同事件未发生，不保存
				{
					if(fp != NULL)
						fclose(fp);
					fprintf(stderr,"\n--事件OI%02x未发生，不存!!\n",eve_road.oad.OI);
					return 0;//事件未发生，不存
				}
				else
				{
					//发送消息通知主动上报
					evehappen = 1;
					fprintf(stderr,"\n--事件OI%02x发生!!\n",eve_road.oad.OI);
				}

				break;
			}
			else
				fseek(fp,blklen,SEEK_CUR);//地址不匹配
		}
		if(savepos == 0)//没找到记录
		{
			evehappen = 1;//第一次存储，产生事件
			fseek(fp,0,SEEK_END);
			savepos = ftell(fp);
			fir_save = 1;
			fprintf(stderr,"\n2第一次存储，产生事件  savepos=%d\n",savepos);
		}
	}
	memset(databuf,0x00,sizeof(databuf));
	if(fir_save == 1)
	{
		databuf[0] = 0x55;
		memcpy(&databuf[1],tsa.addr,TSA_LEN);
		indexn += TSA_LEN+1;
	}

	DbgPrintToFile1(1,"存储事件  OADnum=%d 存储信息:长度%d oad个数%d 存储个数%d 存储间隔%d",OADnum,
			taskhead_info.reclen,taskhead_info.oadnum,taskhead_info.seqnum,taskhead_info.seqsec);
	fillEVEdata(OADdata,OADnum,&databuf[indexn],taskhead_info,headoad_unit);
	indexn += taskhead_info.reclen;


//	for(i=0;i<taskhead_info.reclen+TSA_LEN+1;i++)
//		fprintf(stderr," %02x",databuf[i]);
//	fprintf(stderr,"\n");

	if(fp != NULL)
		fclose(fp);
	fp = fopen(fname,"r+");//格式
	fprintf(stderr,"\n存储savepos=%d len=%d\n",savepos,taskhead_info.reclen+TSA_LEN+1);
	fseek(fp,savepos,SEEK_SET);
	fwrite(databuf,indexn,1,fp);//记录覆盖方式存入文件
	if(fp != NULL)
		fclose(fp);
	return evehappen;
}
