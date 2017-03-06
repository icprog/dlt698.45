/*
 * ObjectGet.c
 *
 *  Created on: Nov 12, 2016
 *      Author: ava
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "AccessFun.h"
#include "StdDataType.h"
#include "Objectdef.h"
#include "dlt698def.h"
#include "dlt698.h"
#include "PublicFunction.h"
#include "secure.h"
extern INT8S (*pSendfun)(int fd,INT8U* sndbuf,INT16U sndlen);
extern int FrameHead(CSINFO *csinfo,INT8U *buf);
extern void FrameTail(INT8U *buf,int index,int hcsi);
extern INT8U Get_Event(OAD oad,INT8U eventno,INT8U** Getbuf,int *Getlen,ProgramInfo* prginfo_event);
extern INT8U Getevent_Record_Selector(RESULT_RECORD *record_para,ProgramInfo* prginfo_event);
extern INT16S composeSecurityResponse(INT8U* SendApdu,INT16U Length);
extern int comfd;
extern INT8U TmpDataBuf[MAXSIZ_FAM];
extern INT8U TmpDataBufList[MAXSIZ_FAM*2];
extern INT8U securetype;
extern ProgramInfo *memp;
extern PIID piid_g;


int BuildFrame_GetResponseRecord(INT8U response_type,CSINFO *csinfo,RESULT_RECORD record,INT8U *sendbuf)
{
	int index=0, hcsi=0;
	csinfo->dir = 1;
	csinfo->prm = 0;
	index = FrameHead(csinfo,sendbuf);
	hcsi = index;
	index = index + 2;
	sendbuf[index++] = GET_RESPONSE;
	sendbuf[index++] = response_type;
	sendbuf[index++] = piid_g.data;	//	piid

	if (record.datalen > 0)
	{
//		sendbuf[index++] = 1;//choice 1  ,SEQUENCE OF A-RecordRow
		memcpy(&sendbuf[index],record.data,record.datalen);
		index = index + record.datalen;
	}else
	{
		sendbuf[index++] = 0;//choice 0  ,DAR 有效 (数据访问可能的结果)
		sendbuf[index++] = record.dar;
	}
	sendbuf[index++] = 0;
	sendbuf[index++] = 0;
	FrameTail(sendbuf,index,hcsi);
	if(pSendfun!=NULL)
		pSendfun(comfd,sendbuf,index+3);
	return (index+3);
}


int BuildFrame_GetResponse(INT8U response_type,CSINFO *csinfo,INT8U oadnum,RESULT_NORMAL response,INT8U *sendbuf)
{
	int apduplace =0;
	int index=0, hcsi=0;
	csinfo->dir = 1;
	csinfo->prm = 1;
	index = FrameHead(csinfo,sendbuf);
	hcsi = index;
	index = index + 2;

	apduplace = index;		//记录APDU 起始位置
	sendbuf[index++] = GET_RESPONSE;
	sendbuf[index++] = response_type;
	sendbuf[index++] = piid_g.data;	//	piid
	if (oadnum>0)
		sendbuf[index++] = oadnum;
	memcpy(&sendbuf[index],response.data,response.datalen);
	index = index + response.datalen;
	sendbuf[index++] = 0;	//跟随上报信息域 	FollowReport
	sendbuf[index++] = 0;	//时间标签		TimeTag
	if(securetype!=0)//安全等级类型不为0，代表是通过安全传输下发报文，上行报文需要以不低于请求的安全级别回复
	{
		apduplace += composeSecurityResponse(&sendbuf[apduplace],index-apduplace);
		index=apduplace;
	}
	FrameTail(sendbuf,index,hcsi);
	if(pSendfun!=NULL)
		pSendfun(comfd,sendbuf,index+3);
	return (index+3);
}


int GetMeterInfo(RESULT_NORMAL *response)
{
	int 	index=0;
	INT8U 	*data = NULL;
	OAD oad={};
	CLASS_6001	 meter={};
	int		i=0,blknum=0;

	oad = response->oad;
	data = response->data;

	memset(&meter,0,sizeof(CLASS_6001));
	blknum = getFileRecordNum(oad.OI);
	fprintf(stderr,"GetMeterInfo oad.oi=%04x blknum=%d\n",oad.OI,blknum);
	if(blknum<=1)	return 0;
	index += create_array(&data[index],blknum-1);		//个数-1：文件第一个块为空 电表总数
	for(i=0;i<blknum;i++)
	{
		index += Get_6001(i,&data[index]);
	}
	response->datalen = index;
	return 0;
}
int GetTaskInfo(RESULT_NORMAL *response)
{
	return 0;
}

int GetCjiFangAnInfo(RESULT_NORMAL *response)
{
	return 0;
}
int GetEventCjFangAnInfo(RESULT_NORMAL *response)
{
	return 0;
}

int GetYxPara(RESULT_NORMAL *response)
{
	int index=0;
	INT8U *data = NULL;
	OAD oad;
	CLASS_f203 objtmp;
	int	 chgflag=0;
	oad = response->oad;
	data = response->data;
	memset(&objtmp,0,sizeof(objtmp));
	readCoverClass(0xf203,0,&objtmp,sizeof(objtmp),para_vari_save);
	switch(oad.attflg )
	{
		case 4://配置参数
			index += create_struct(&data[index],2);
			index += fill_bit_string8(&data[index],objtmp.state4.StateAcessFlag);
			index += fill_bit_string8(&data[index],objtmp.state4.StatePropFlag);
			break;
		case 2://设备对象列表
			fprintf(stderr,"GetYxPara oi.att=%d\n",oad.attflg);
			objtmp.statearri.num = 4;
			index += create_array(&data[index],objtmp.statearri.num);
			for(int i=0;i<objtmp.statearri.num;i++)
			{
				index += create_struct(&data[index],2);
				index += fill_unsigned(&data[index],objtmp.statearri.stateunit[i].ST);
				index += fill_unsigned(&data[index],objtmp.statearri.stateunit[i].CD);
				if(objtmp.statearri.stateunit[i].CD) {
					objtmp.statearri.stateunit[i].CD = 0;
					chgflag = 1;
				}
			}
			if(chgflag) {	//遥信变位状态更改后，保存
				saveCoverClass(0xf203,0,&objtmp,sizeof(objtmp),para_vari_save);
			}
			fprintf(stderr,"index=%d\n",index);
			break;
	}
	response->datalen = index;
	return 0;
}
int GetEsamPara(RESULT_NORMAL *response)
{
	INT8U *data=NULL;
	OAD oad;
	oad = response->oad;
	data = response->data;
	response->datalen = getEsamAttribute(oad,data);
	if(response->datalen == 0)
		response->dar = 0x16;//esam验证失败
	return 0;
}
int GetSecurePara(RESULT_NORMAL *response)
{
	INT8U *data=NULL;
	OAD oad;
	CLASS_F101 f101;
	oad = response->oad;
	data = response->data;
//	readParaClass(0xf101,&f101,0);
	readCoverClass(0xf101,0,&f101,sizeof(f101),para_vari_save);
	switch(oad.attflg )
	{
		case 2://安全模式选择
			response->datalen = fill_enum(data,f101.active);;
			break;
		case 3://安全模式参数array
			break;
	}
	return 0;
}
int GetSysDateTime(RESULT_NORMAL *response)
{
	INT8U *data=NULL;
	OAD oad;
	DateTimeBCD time;
	system((const char*)"hwclock -s");
	oad = response->oad;
	data = response->data;
	DataTimeGet(&time);
	switch(oad.attflg )
	{
		case 2://安全模式选择
			response->datalen = fill_date_time_s(response->data,&time);
			break;
	}
	return 0;
}

int Get3105(RESULT_NORMAL *response)
{
	int index=0;
	OAD oad={};
	Event3105_Object	tmpobj={};
	INT8U *data = NULL;

	data = response->data;
	oad = response->oad;
	memset(&tmpobj,0,sizeof(Event3105_Object));
	readCoverClass(oad.OI,0,&tmpobj,sizeof(Event3105_Object),event_para_save);
	index += create_struct(&data[index],2);
	index += fill_long_unsigned(&data[index],tmpobj.mto_obj.over_threshold);
	index += fill_unsigned(&data[index],tmpobj.mto_obj.task_no);
	response->datalen = index;
	return 0;
}

int Get3106(RESULT_NORMAL *response)
{
	int index=0;
	OAD oad={};
	Event3106_Object	tmpobj={};
	INT8U *data = NULL;
	int	i=0;

	data = response->data;
	oad = response->oad;
	memset(&tmpobj,0,sizeof(Event3106_Object));
	readCoverClass(oad.OI,0,&tmpobj,sizeof(Event3106_Object),event_para_save);
	index += create_struct(&data[index],2);	//属性６　２个元素
	index += create_struct(&data[index],4);	//停电数据采集配置参数　４个元素
	index += fill_bit_string8(&data[index],tmpobj.poweroff_para_obj.collect_para_obj.collect_flag);
	index += fill_unsigned(&data[index],tmpobj.poweroff_para_obj.collect_para_obj.time_space);
	index += fill_unsigned(&data[index],tmpobj.poweroff_para_obj.collect_para_obj.time_threshold);
	index += create_array(&data[index],tmpobj.poweroff_para_obj.collect_para_obj.tsaarr.num);
	for(i=0;i<tmpobj.poweroff_para_obj.collect_para_obj.tsaarr.num;i++) {
		index += fill_TSA(&data[index],&tmpobj.poweroff_para_obj.collect_para_obj.tsaarr.meter_tas[i].addr[1],tmpobj.poweroff_para_obj.collect_para_obj.tsaarr.meter_tas[i].addr[0]);
	}
	index += create_struct(&data[index],6);	//停电事件甄别限值参数　６个元素
	index += fill_long_unsigned(&data[index],tmpobj.poweroff_para_obj.screen_para_obj.mintime_space);
	index += fill_long_unsigned(&data[index],tmpobj.poweroff_para_obj.screen_para_obj.maxtime_space);
	index += fill_long_unsigned(&data[index],tmpobj.poweroff_para_obj.screen_para_obj.startstoptime_offset);
	index += fill_long_unsigned(&data[index],tmpobj.poweroff_para_obj.screen_para_obj.sectortime_offset);
	index += fill_long_unsigned(&data[index],tmpobj.poweroff_para_obj.screen_para_obj.happen_voltage_limit);
	index += fill_long_unsigned(&data[index],tmpobj.poweroff_para_obj.screen_para_obj.recover_voltage_limit);
	response->datalen = index;
	return 0;
}

int Get310c(RESULT_NORMAL *response)
{
	int index=0;
	OAD oad={};
	Event310C_Object	tmpobj={};
	INT8U *data = NULL;

	data = response->data;
	oad = response->oad;
	memset(&tmpobj,0,sizeof(Event310C_Object));
	readCoverClass(oad.OI,0,&tmpobj,sizeof(Event310C_Object),event_para_save);
	index += create_struct(&data[index],2);	//属性６　２个元素
	index += fill_double_long_unsigned(&data[index],tmpobj.poweroffset_obj.power_offset);
	index += fill_unsigned(&data[index],tmpobj.poweroffset_obj.task_no);
	response->datalen = index;
	return 0;
}

int Get310d(RESULT_NORMAL *response)
{
	int index=0;
	OAD oad={};
	Event310D_Object	tmpobj={};
	INT8U *data = NULL;

	data = response->data;
	oad = response->oad;
	memset(&tmpobj,0,sizeof(Event310D_Object));
	readCoverClass(oad.OI,0,&tmpobj,sizeof(Event310D_Object),event_para_save);
	index += create_struct(&data[index],2);	//属性６　２个元素
	index += fill_double_long_unsigned(&data[index],tmpobj.poweroffset_obj.power_offset);
	index += fill_unsigned(&data[index],tmpobj.poweroffset_obj.task_no);
	response->datalen = index;
	return 0;
}

int Get310e(RESULT_NORMAL *response)
{
	int index=0;
	OAD oad={};
	Event310E_Object	tmpobj={};
	INT8U *data = NULL;

	data = response->data;
	oad = response->oad;
	memset(&tmpobj,0,sizeof(Event310E_Object));
	readCoverClass(oad.OI,0,&tmpobj,sizeof(Event310E_Object),event_para_save);
	index += create_struct(&data[index],2);
	index += fill_TI(&data[index],tmpobj.powerstoppara_obj.power_offset);
	index += fill_unsigned(&data[index],tmpobj.powerstoppara_obj.task_no);
	response->datalen = index;
	return 0;
}

int Get310f(RESULT_NORMAL *response)
{
	int index=0;
	OAD oad={};
	Event310F_Object	tmpobj={};
	INT8U *data = NULL;

	data = response->data;
	oad = response->oad;
	memset(&tmpobj,0,sizeof(Event310F_Object));
	readCoverClass(oad.OI,0,&tmpobj,sizeof(Event310F_Object),event_para_save);
	index += create_struct(&data[index],2);
	index += fill_unsigned(&data[index],tmpobj.collectfail_obj.retry_nums);
	index += fill_unsigned(&data[index],tmpobj.collectfail_obj.task_no);
	response->datalen = index;
	return 0;
}

int Get3110(RESULT_NORMAL *response)
{
	int index=0;
	OAD oad={};
	Event3110_Object	tmpobj={};
	INT8U *data = NULL;

	data = response->data;
	oad = response->oad;
	memset(&tmpobj,0,sizeof(Event3110_Object));
	readCoverClass(oad.OI,0,&tmpobj,sizeof(Event3110_Object),event_para_save);
	index += create_struct(&data[index],1);
	index += fill_double_long_unsigned(&data[index],tmpobj.Monthtrans_obj.month_offset);
	response->datalen = index;
	return 0;
}

int Get4001_4002_4003(RESULT_NORMAL *response)
{
	int i=0;
	OAD oad={};
	CLASS_4001_4002_4003	class_addr={};

	oad = response->oad;
	memset(&class_addr,0,sizeof(CLASS_4001_4002_4003));
	readCoverClass(oad.OI,0,&class_addr,sizeof(CLASS_4001_4002_4003),para_vari_save);
	switch(oad.attflg )
	{
		case 2:
			response->datalen =  class_addr.curstom_num[0]+1;//第一个字节是长度 + 内容
			fprintf(stderr,"\n读取 datalen=%d\n",response->datalen);
			for(i=0;i< response->datalen;i++)
				fprintf(stderr," %02x",class_addr.curstom_num[i]);
			fprintf(stderr,"\n");
			response->data[0] = 0x09;
			response->datalen += 1;//再加一个数据类型
			memcpy(&response->data[1] ,class_addr.curstom_num,response->datalen);
			break;
	}
	return 0;
}
int Get4004(RESULT_NORMAL *response)
{
	int index=0;
	INT8U *data = NULL;
	OAD oad;
	CLASS_4004	class_tmp={};
	data = response->data;
	oad = response->oad;
	memset(&class_tmp,0,sizeof(CLASS_4004));
	readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4004),para_vari_save);
	switch(oad.attflg )
	{
		case 2:
			index += create_struct(&data[index],3);//经度，纬度，高度
			index += create_struct(&data[index],4);//方位、度、分、秒
			index += fill_enum(&data[index],class_tmp.jing.fangwei);
			index += fill_unsigned(&data[index],class_tmp.jing.du);
			index += fill_unsigned(&data[index],class_tmp.jing.fen);
			index += fill_unsigned(&data[index],class_tmp.jing.miao);
			index += create_struct(&data[index],4);//方位、度、分、秒
			index += fill_enum(&data[index],class_tmp.wei.fangwei);
			index += fill_unsigned(&data[index],class_tmp.wei.du);
			index += fill_unsigned(&data[index],class_tmp.wei.fen);
			index += fill_unsigned(&data[index],class_tmp.wei.miao);
			index += fill_double_long_unsigned(&data[index],class_tmp.heigh);
			response->datalen = index;
			break;
	}
	return 0;
}
int Get4006(RESULT_NORMAL *response)
{
	int index=0;
	INT8U *data = NULL;
	OAD oad;
	CLASS_4006	class_tmp={};
	data = response->data;
	oad = response->oad;
	memset(&class_tmp,0,sizeof(CLASS_4006));
	readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4006),para_vari_save);
	class_tmp.clocksource = 1;//时钟芯片
	class_tmp.state = 0;//可用
	switch(oad.attflg )
	{
		case 2:
			index += create_struct(&data[index],2);//时钟源
			index += fill_enum(&data[index],class_tmp.clocksource);
			index += fill_enum(&data[index],class_tmp.state);
			response->datalen = index;
			break;
	}
	return 0;
}
int Get4007(RESULT_NORMAL *response)
{
	int index=0;
	INT8U *data = NULL;
	OAD oad;
	CLASS_4007	class_tmp={};
	data = response->data;
	oad = response->oad;
	memset(&class_tmp,0,sizeof(CLASS_4007));
	readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4007),para_vari_save);
	switch(oad.attflg )
	{
		case 2:
			index += create_struct(&data[index],7);
			index += fill_unsigned(&data[index],class_tmp.poweon_showtime);
			index += fill_long_unsigned(&data[index],class_tmp.lcdlight_time);
			index += fill_long_unsigned(&data[index],class_tmp.looklight_time);
			index += fill_long_unsigned(&data[index],class_tmp.poweron_maxtime);
			index += fill_long_unsigned(&data[index],class_tmp.poweroff_maxtime);
			index += fill_unsigned(&data[index],class_tmp.energydata_dec);
			index += fill_unsigned(&data[index],class_tmp.powerdata_dec);
			response->datalen = index;
			break;
	}
	return 0;
}
int Get4103(RESULT_NORMAL *response)
{
	int index=0;
	INT8U *data = NULL;
	OAD oad;
	CLASS_4103	class_tmp={};
	data = response->data;
	oad = response->oad;
	memset(&class_tmp,0,sizeof(CLASS_4103));
	readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4103),para_vari_save);
	switch(oad.attflg )
	{
		case 2:
			index += fill_visible_string(&data[index],&class_tmp.assetcode[1],class_tmp.assetcode[0]);
			response->datalen = index;
			break;
	}
	return 0;
}
int Get4204(RESULT_NORMAL *response)
{
	int index=0;
	INT8U *data = NULL;
	OAD oad;
	CLASS_4204	class_tmp={};
	data = response->data;
	oad = response->oad;
	memset(&class_tmp,0,sizeof(CLASS_4204));
	readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4204),para_vari_save);
	switch(oad.attflg )
	{
		case 2:
			index += create_struct(&data[index],2);
			index += fill_time(&data[index],class_tmp.startime);
			index += file_bool(&data[index],class_tmp.enable);
			response->datalen = index;
			break;
		case 3:
			index += create_struct(&data[index],3);
			index += fill_integer(&data[index],class_tmp.upleve);
			index += fill_time(&data[index],class_tmp.startime1);
			index += file_bool(&data[index],class_tmp.enable1);
			response->datalen = index;
			break;
	}
	return 0;
}
int Get4300(RESULT_NORMAL *response)
{
	int index=0;
	INT8U *data = NULL;
	OAD oad;
	CLASS19	class_tmp={};
	data = response->data;
	oad = response->oad;
	memset(&class_tmp,0,sizeof(CLASS19));
	readCoverClass(0x4300,0,&class_tmp,sizeof(CLASS19),para_vari_save);
	switch(oad.attflg )
	{
		case 3:
			index += create_struct(&data[index],6);
			index += fill_visible_string(&data[index],class_tmp.info.factoryCode,4);
			index += fill_visible_string(&data[index],class_tmp.info.softVer,4);
			index += fill_visible_string(&data[index],class_tmp.info.softDate,6);
			index += fill_visible_string(&data[index],class_tmp.info.hardVer,4);
			index += fill_visible_string(&data[index],class_tmp.info.hardDate,6);
			index += fill_visible_string(&data[index],class_tmp.info.factoryExpInfo,8);
			break;
		case 4:
			index +=fill_date_time_s(&data[index],&class_tmp.date_Product);
			break;
		case 7:
			index += file_bool(&data[index],class_tmp.follow_report);
			break;
		case 8:
			index += file_bool(&data[index],class_tmp.active_report);
			break;
		case 9:
			index += file_bool(&data[index],class_tmp.talk_master);
			break;
	}
	response->datalen = index;
	return 0;
}

void printrcsd(RCSD rcsd)
{
	int i=0;
	int k=0;
	for(i = 0; i<rcsd.csds.num;i++)
	{
		if (rcsd.csds.csd[i].type == 1)
		{
			fprintf(stderr,"\n");
			fprintf(stderr,"\nROAD     %04x %02x %02x",rcsd.csds.csd[i].csd.road.oad.OI,rcsd.csds.csd[i].csd.road.oad.attflg,rcsd.csds.csd[i].csd.road.oad.attrindex);
			for(k=0;k<rcsd.csds.csd[i].csd.road.num;k++)
				fprintf(stderr,"\n     		oad %04x %02x %02x",rcsd.csds.csd[i].csd.road.oads[k].OI,rcsd.csds.csd[i].csd.road.oads[k].attflg,rcsd.csds.csd[i].csd.road.oads[k].attrindex);
		}else
		{
			fprintf(stderr,"\nOAD     %04x %02x %02x",rcsd.csds.csd[i].csd.oad.OI,rcsd.csds.csd[i].csd.oad.attflg,rcsd.csds.csd[i].csd.oad.attrindex);
		}
	}
}
void printSel5(RESULT_RECORD record)
{
	fprintf(stderr,"\n%d年 %d月 %d日 %d时:%d分:%d秒",
					record.select.selec5.collect_save.year.data,record.select.selec5.collect_save.month.data,
					record.select.selec5.collect_save.day.data,record.select.selec5.collect_save.hour.data,
					record.select.selec5.collect_save.min.data,record.select.selec5.collect_save.sec.data);
	fprintf(stderr,"\nMS-TYPE %d  ",record.select.selec5.meters.mstype);
	printrcsd(record.rcsd);
}

void printSel7(RESULT_RECORD record)
{
	fprintf(stderr,"\n采集存储时间起始值：%d-%d-%d %d:%d:%d",
					record.select.selec7.collect_save_star.year.data,record.select.selec7.collect_save_star.month.data,
					record.select.selec7.collect_save_star.day.data,record.select.selec7.collect_save_star.hour.data,
					record.select.selec7.collect_save_star.min.data,record.select.selec7.collect_save_star.sec.data);
	fprintf(stderr,"\n采集存储时间结束值：%d-%d-%d %d:%d:%d",
					record.select.selec7.collect_save_finish.year.data,record.select.selec7.collect_save_finish.month.data,
					record.select.selec7.collect_save_finish.day.data,record.select.selec7.collect_save_finish.hour.data,
					record.select.selec7.collect_save_finish.min.data,record.select.selec7.collect_save_finish.sec.data);
	fprintf(stderr,"\n时间间隔TI 单位:%d[秒-0，分-1，时-2，日-3，月-4，年-5],间隔:%x",record.select.selec7.ti.units,record.select.selec7.ti.interval);
	fprintf(stderr,"\n电能表集合MS 类型：%d\n",record.select.selec7.meters.mstype);
	printrcsd(record.rcsd);
}

void printSel9(RESULT_RECORD record)
{
	fprintf(stderr,"\nSelector9:指定选取上第n次记录\n");
	fprintf(stderr,"\n选取上第%d次记录 ",record.select.selec9.recordn);
	fprintf(stderr,"\nRCSD个数：%d",record.rcsd.csds.num);
	printrcsd(record.rcsd);
}

void printrecord(RESULT_RECORD record)
{
	switch(record.selectType){
	case 1:
		fprintf(stderr,"\nOAD %04x %02x %02x",record.select.selec1.oad.OI,record.select.selec1.oad.attflg,record.select.selec1.oad.attrindex);
		fprintf(stderr,"\nData Type= %02x  Value=%d ",record.select.selec1.data.type,record.select.selec1.data.data[0]);
		break;
	case 5:
		printSel5(record);
		break;
	case 7:
		printSel7(record);
		break;
	case 9:
		printSel9(record);
		break;
	}
}
///
int getSel1_coll(RESULT_RECORD *record)
{
	int ret=0;
	int		index = 0;
	int		taskid=0;

//	record->data = data;
//	data[index++] = 1;		//1条记录     [1] SEQUENCE OF A-RecordRow
	switch(record->select.selec1.oad.OI)
	{
		case 0x6001:
		{
			if(record->select.selec1.data.type == dtlongunsigned) {
				taskid = (record->select.selec1.data.data[0]<<8 | record->select.selec1.data.data[1]);
			}
			fprintf(stderr,"\n  record for 6001  - taskid=%d\n",taskid);
			index += Get_6001(taskid,&record->data[index]);
			break;
		}
		case 0x6013:
			if(record->select.selec1.data.type == dtunsigned) {
				taskid = record->select.selec1.data.data[0];
			}
			fprintf(stderr,"\n  record for 6012  - taskid=%d\n",taskid);
			index += Get_6013(taskid,&record->data[index]);
			break;
		case 0x6015:
			if(record->select.selec1.data.type == dtunsigned) {
				taskid = record->select.selec1.data.data[0];
			}
			fprintf(stderr,"\n  record for 6014  - taskid=%d\n",taskid);
			index += Get_6015(taskid,&record->data[index]);
			break;
		case 0x6035:
		{
			if(record->select.selec1.data.type == dtunsigned) {
				taskid = record->select.selec1.data.data[0];
			}
			fprintf(stderr,"taskid=%d\n",taskid);
			index += Get_6035(taskid,&record->data[index]);
			break;
		}
		default:
			fprintf(stderr,"\nrecord switch default!");
	}
	if(index==0) {	//0条记录     [1] SEQUENCE OF A-RecordRow
		record->data[0] = 0;
	}
	record->datalen = index;
	fprintf(stderr,"\nrecord->datalen = %d",record->datalen);
	return ret;
}
/*
 * 选择方法1: 读取指定对象指定值
 * */
int getSelector1(RESULT_RECORD *record)
{
	int  ret=0;
	INT8U oihead = (record->oad.OI & 0xF000) >>12;

	switch(oihead) {
	case 6:			//采集监控类对象
		fprintf(stderr,"\n读取采集监控对象\n");
		getSel1_coll(record);
		break;
	}
	return ret;
}


int doGetrecord(OAD oad,INT8U *data,RESULT_RECORD *record)
{
	int 	source_index=0;		//getrecord 指针
	int		dest_index=0;		//getreponse 指针
	INT8U 	SelectorN =0;

//	memset(TmpDataBuf,0,sizeof(TmpDataBuf));
//	record->oad = oad;
//	record->data = TmpDataBuf;
//	record->datalen = 0;
	fprintf(stderr,"\nGetRequestRecord   oi=%x  %02x  %02x",record->oad.OI,record->oad.attflg,record->oad.attrindex);
	source_index = get_BasicRSD(0,&data[source_index],(INT8U *)&record->select,&record->selectType);
	fprintf(stderr,"\nRSD Select%d     data[%d] = %02x",record->selectType,source_index,data[source_index]);
	source_index +=get_BasicRCSD(0,&data[source_index],&record->rcsd.csds);
	//record.rcsd.csds.csd[i].csd.oad.OI
	SelectorN = record->selectType;
	fprintf(stderr,"\n- getRequestRecord SelectorN=%d OI = %04x  attrib=%d  index=%d",SelectorN,record->oad.OI,record->oad.attflg,record->oad.attrindex);
	printrecord(*record);
	dest_index += create_OAD(&record->data[dest_index],record->oad);
	switch(SelectorN) {
	case 1:		//指定对象指定值
		record->data[dest_index++] = 1;	//一行记录M列属性描述符 	RCSD
		record->data[dest_index++] = 0;	//OAD
		record->select.selec1.oad.attrindex = 0;		//上送属性下所有索引值
		dest_index += create_OAD(&record->data[dest_index],record->select.selec1.oad);
		record->data[dest_index++] = 1; //CHOICE  [1]  data
		record->data[dest_index++] = 1; //M = 1  Sequence  of A-RecordRow
		record->data = &TmpDataBuf[dest_index];		//修改record的数据帧的位置
		getSelector1(record);
		record->data = TmpDataBuf;				//data 指向回复报文帧头
		record->datalen += dest_index;			//数据长度+ResultRecord
	break;
	case 5:
	case 7:
		dest_index +=fill_RCSD(0,&record->data[dest_index],record->rcsd.csds);
		record->data = &TmpDataBuf[dest_index];
		getSelector(record->select, record->selectType,record->rcsd.csds,(INT8U *)record->data,(int *)&record->datalen);
		record->data = TmpDataBuf;				//data 指向回复报文帧头
		record->datalen += dest_index;			//数据长度+ResultRecord
//		fprintf(stderr,"\nreturn len =%d\n",index);
//		fprintf(stderr,"\n报文(%d)：",index);
//		for(i=0;i<index;i++)
//			fprintf(stderr," %02x",record->data[i]);
		break;
	case 9:		//指定读取上第n次记录
		dest_index +=fill_RCSD(0,&record->data[dest_index],record->rcsd.csds);
		record->data = &TmpDataBuf[dest_index];
		Getevent_Record_Selector(record,memp);
		record->data = TmpDataBuf;				//data 指向回复报文帧头
		record->datalen += dest_index;			//数据长度+ResultRecord
		break;
	}
	fprintf(stderr,"\n---doGetrecord end\n");
	return source_index;
}

int getRequestRecord(OAD oad,INT8U *data,CSINFO *csinfo,INT8U *sendbuf)
{
	RESULT_RECORD record;

	memset(TmpDataBuf,0,sizeof(TmpDataBuf));
	record.oad = oad;
	record.data = TmpDataBuf;
	record.datalen = 0;
	doGetrecord(oad,data,&record);
	BuildFrame_GetResponseRecord(GET_REQUEST_RECORD,csinfo,record,sendbuf);

//	securetype = 0;		//清除安全等级标识
	return 1;
}


int getRequestRecordList(INT8U *data,CSINFO *csinfo,INT8U *sendbuf)
{
	RESULT_RECORD record={};
	OAD	oad={};
	int i=0;
	int recordnum = 0;
	int destindex=0;
	int sourceindex=0;

	memset(TmpDataBufList,0,sizeof(TmpDataBufList));
	recordnum = data[sourceindex++];
	fprintf(stderr,"getRequestRecordList  Result-record=%d \n ",recordnum);
	TmpDataBufList[destindex++] = recordnum;
	for(i=0;i<recordnum;i++) {
		memset(TmpDataBuf,0,sizeof(TmpDataBuf));
		record.data = TmpDataBuf;
		record.datalen = 0;
		sourceindex += getOAD(0,&data[sourceindex],&oad);
		record.oad = oad;
		sourceindex += doGetrecord(oad,&data[sourceindex],&record);
		memcpy(&TmpDataBufList[destindex],record.data,record.datalen);
		destindex += record.datalen;
	}
	record.data = TmpDataBufList;
	record.datalen = destindex;
	BuildFrame_GetResponseRecord(GET_REQUEST_RECORD,csinfo,record,sendbuf);
///	securetype = 0;		//清除安全等级标识
	return 1;
}




int GetVariable(RESULT_NORMAL *response)
{
	int	  	len=0;
	INT8U	databuf[VARI_LEN]={};
	INT8U *data = NULL;
	int index=0;

	data = response->data;
	memset(&databuf,0,sizeof(databuf));
	len = readVariData(response->oad.OI,0,&databuf,VARI_LEN);
	if(len>0) {
		switch(response->oad.OI)
		{
			case 0x2200:	//通信流量
				Get_2200(response->oad.OI,databuf,data,&index);
				break;
			case 0x2203:	//供电时间
				Get_2203(response->oad.OI,databuf,data,&index);
				break;
			case 0x2204:	//复位次数
				Get_2204(response->oad.OI,databuf,data,&index);
				break;
		}
		response->datalen = index;
		fprintf(stderr,"datalen=%d \n",response->datalen);
	}else if(len==0){
		response->datalen = 0;	//无数据
	}else {
		response->datalen = 0;	//无数据
		fprintf(stderr,"\n读取的OI=%04x ,不在变量类对象文件%s中，请从其他文件获取!!!\n",response->oad.OI,VARI_DATA);
	}
	return 1;
}

int GetEventRecord(RESULT_NORMAL *response)
{
	INT8U *data=NULL;
	int datalen=0;

	if ( Get_Event(response->oad,response->oad.attrindex,&data,&datalen,memp) == 1 )
	{
		if (datalen > 512 || data==NULL)
		{
			fprintf(stderr,"\n获取事件数据Get_Event函数异常! [datalen=%d  data=%p]",datalen,data);
			if (data!=NULL)
				free(data);
			return 0;
		}
		memcpy(response->data,data,datalen);
		response->datalen = datalen;
		if (data!=NULL)
			free(data);
		return 1;
	}
	response->datalen = 0;
	response->dar = other_err1;
	fprintf(stderr,"\n获取事件数据Get_Event函数返回 0  [datalen=%d  data=%p]",datalen,data);
	if (data!=NULL)
		free(data);
	return 1;
}

int GetClass7attr(RESULT_NORMAL *response)
{
	INT8U *data = NULL;
	OAD oad={};
	Class7_Object	class7={};
	int index=0;
	data = response->data;
	oad = response->oad;
	memset(&class7,sizeof(Class7_Object),0);
	readCoverClass(oad.OI,0,&class7,sizeof(Class7_Object),event_para_save);
	switch(oad.attflg) {
	case 1:	//逻辑名
		break;
	case 3:	//关联属性表
		break;
	case 4:	//当前记录数
		break;
	case 5:	//最大记录数
		break;
	case 8: //上报标识
		break;
	case 9: //有效标识
		index += file_bool(&data[0],class7.enableflag);
		break;
	}
	response->datalen = index;
	return 0;
}

int GetEventInfo(RESULT_NORMAL *response)
{
	fprintf(stderr,"GetEventInfo OI=%x,attflg=%d,attrindex=%d \n",response->oad.OI,response->oad.attflg,response->oad.attrindex);
	switch(response->oad.attflg) {
	case 2:		//事件记录表
		GetEventRecord(response);
		break;
	case 1:	//逻辑名
	case 3:	//关联属性表
	case 4:	//当前记录数
	case 5:	//最大记录数
	case 8: //上报标识
	case 9: //有效标识
		GetClass7attr(response);
		break;
	case 6:	//配置参数
		switch(response->oad.OI) {
			case 0x3105:	//电能表时间超差事件
				Get3105(response);
				break;
			case 0x3106:	//停上电事件
				Get3106(response);
				break;
			case 0x310c:	//电能量超差事件阈值
				Get310c(response);
				break;
			case 0x310d:	//电能表飞走事件阈值
				Get310d(response);
				break;
			case 0x310e:	//电能表停走事件阈值
				Get310e(response);
				break;
			case 0x310f:	//终端抄表失败事件
				Get310f(response);
				break;
			case 0x3110:	//月通信流量超限事件阈值
				Get3110(response);
				break;
		}
		break;
	case 7:
		GetEventRecord(response);
		break;
	}
	return 0;
}

int GetEnvironmentValue(RESULT_NORMAL *response)
{
	switch(response->oad.OI)
	{
		case 0x4000:
			GetSysDateTime(response);
			break;
		case 0x4001:
		case 0x4002:
		case 0x4003:
			Get4001_4002_4003(response);
			break;
		case 0x4004:
			Get4004(response);
			break;
		case 0x4006:
			Get4006(response);
			break;
		case 0x4007:
			Get4007(response);
			break;
		case 0x4103:
			Get4103(response);
			break;
		case 0x4204:
			Get4204(response);
			break;
		case 0x4300:
			Get4300(response);
			break;
	}
	return 1;
}
int GetCollPara(RESULT_NORMAL *response)
{
	switch(response->oad.OI)
	{
		case 0x6000:	//采集档案配置表
			GetMeterInfo(response);
			break;
		case 0x6002:	//搜表
			break;
		case 0x6012:	//任务配置表
			GetTaskInfo(response);
			break;
		case 0x6014:	//普通采集方案集
			GetCjiFangAnInfo(response);
			break;
		case 0x6016:	//事件采集方案
			GetEventCjFangAnInfo(response);
			break;
	}
	return 1;
}
int GetDeviceIo(RESULT_NORMAL *response)
{
	switch(response->oad.OI)
	{
		case 0xF100:
			GetEsamPara(response);
			break;
		case 0xF101:
			GetSecurePara(response);
			break;
		case 0xF203:
			GetYxPara(response);
			break;
	}
	return 1;
}
int doGetnormal(RESULT_NORMAL *response)
{
	INT16U oi = response->oad.OI;
	INT8U oihead = (oi & 0xF000) >>12;

	fprintf(stderr,"\ngetRequestNormal----------  oi =%04x  \n",oi);
	switch(oihead) {
		case 2:			//变量类对象
			GetVariable(response);
			break;
		case 3:			//事件类对象读取
			GetEventInfo(response);
			break;
		case 4:			//参变量类对象
			GetEnvironmentValue(response);
			break;
		case 6:			//采集监控类对象
			GetCollPara(response);
			break;
		case 0xF:		//文件类/esam类/设备类
			GetDeviceIo(response);
			break;
	}
	return 0;
}
int getRequestNormal(OAD oad,INT8U *data,CSINFO *csinfo,INT8U *sendbuf)
{
	RESULT_NORMAL response;
	INT8U oadtmp[4]={};

	oadtmp[0] = (oad.OI>>8) & 0xff;
	oadtmp[1] = oad.OI & 0xff;
	oadtmp[2] = oad.attflg;
	oadtmp[3] = oad.attrindex;
	memset(TmpDataBuf,0,sizeof(TmpDataBuf));
	memcpy(TmpDataBuf,oadtmp,4);

	response.oad = oad;
	response.data = TmpDataBuf+5; //4 + 1             oad（4字节） + choice(1字节)
	response.datalen = 0;
	doGetnormal(&response);
	if (response.datalen>0)
	{
		TmpDataBuf[4] = 1;//数据
		response.datalen += 5;  // datalen + oad(4) + choice(1)
	}else
	{
		TmpDataBuf[4] = 0;//错误
		TmpDataBuf[5] = response.dar;
		response.datalen += 6;  // datalen + oad(4) + choice(1) + dar
	}
	response.data = TmpDataBuf;

	BuildFrame_GetResponse(GET_REQUEST_NORMAL,csinfo,0,response,sendbuf);
	return 1;
}

int getRequestNormalList(INT8U *data,CSINFO *csinfo,INT8U *sendbuf)
{
	RESULT_NORMAL response;
	INT8U oadtmp[4]={};
	INT8U oadnum = data[0];
	int i=0,listindex=0;
	fprintf(stderr,"\nGetNormalList!! OAD_NUM=%d",oadnum);
	memset(TmpDataBufList,0,sizeof(TmpDataBufList));
	for(i=0;i<oadnum;i++)
	{
		memset(TmpDataBuf,0,sizeof(TmpDataBuf));
		memcpy(oadtmp,&data[1 + i*4],4);
		response.dar = 0;
		getOAD(0,oadtmp, &response.oad);
		response.datalen = 0;
		response.data = TmpDataBuf + 5;
		fprintf(stderr,"\n【%d】OI = %x  %02x  %02x",i,response.oad.OI,response.oad.attflg,response.oad.attrindex);
		doGetnormal(&response);

		memcpy(&TmpDataBufList[listindex + 0],oadtmp,4);
		if (response.datalen>0)
		{
			TmpDataBufList[listindex + 4] = 1;//数据
			memcpy(&TmpDataBufList[listindex + 5],response.data,response.datalen);
			listindex = listindex + 5 + response.datalen;
		}
		else
		{
//			TmpDataBufList[listindex + 4] = 0;//错误
			TmpDataBufList[listindex + 4] = response.dar;  //  0-3(oad)   4(choice)  5(dar)
			listindex = listindex + 5;
		}
	}
	response.data = TmpDataBufList;
	response.datalen = listindex;
	BuildFrame_GetResponse(GET_REQUEST_NORMAL_LIST,csinfo,oadnum,response,sendbuf);
	return 1;
}
