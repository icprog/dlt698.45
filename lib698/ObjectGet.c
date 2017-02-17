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
#include "PublicFunction.h"
#include "secure.h"
extern INT8S (*pSendfun)(int fd,INT8U* sndbuf,INT16U sndlen);
extern int FrameHead(CSINFO *csinfo,INT8U *buf);
extern void FrameTail(INT8U *buf,int index,int hcsi);
extern INT8U Get_Event(OI_698 oi,INT8U eventno,INT8U** Getbuf,int *Getlen,ProgramInfo* prginfo_event);
extern INT8U Getevent_Record_Selector(RESULT_RECORD *record_para,ProgramInfo* prginfo_event);
extern void getoad(INT8U *data,OAD *oad);
extern int get_BasicRCSD(INT8U *source,CSD_ARRAYTYPE *csds);
extern int get_BasicRSD(INT8U *source,INT8U *dest,INT8U *type);
extern INT16S composeSecurityResponse(INT8U* SendApdu,INT16U Length);
extern int comfd;
extern INT8U TmpDataBuf[MAXSIZ_FAM];
extern INT8U TmpDataBufList[MAXSIZ_FAM*2];
extern INT8U securetype;
extern ProgramInfo *memp;

int BuildFrame_GetResponseRecord(INT8U response_type,CSINFO *csinfo,RESULT_RECORD record,INT8U *sendbuf)
{
	int index=0, hcsi=0,num=0,i=0,k=0;
	csinfo->dir = 1;
	csinfo->prm = 0;
	index = FrameHead(csinfo,sendbuf);
	hcsi = index;
	index = index + 2;
	sendbuf[index++] = GET_RESPONSE;
	sendbuf[index++] = response_type;
	sendbuf[index++] = 0;	//	piid
	sendbuf[index++] = (record.oad.OI>>8) & 0xff;
	sendbuf[index++] = record.oad.OI & 0xff;
	sendbuf[index++] = record.oad.attflg;
	sendbuf[index++] = record.oad.attrindex;
	num = record.rcsd.csds.num;
	sendbuf[index++] = num;
	for(i=0;i<num;i++)
	{
		sendbuf[index++] = record.rcsd.csds.csd[i].type;	//第 i 个csd类型
		if (record.rcsd.csds.csd[i].type ==0)
		{
			sendbuf[index++] = (record.rcsd.csds.csd[i].csd.oad.OI)>>8 &0xff ;
			sendbuf[index++] = record.rcsd.csds.csd[i].csd.oad.OI &0xff;
			sendbuf[index++] = record.rcsd.csds.csd[i].csd.oad.attflg;
			sendbuf[index++] = record.rcsd.csds.csd[i].csd.oad.attrindex;
		}else
		{
			sendbuf[index++] = (record.rcsd.csds.csd[i].csd.road.oad.OI>>8) &0xff ;
			sendbuf[index++] = record.rcsd.csds.csd[i].csd.road.oad.OI &0xff;
			sendbuf[index++] = record.rcsd.csds.csd[i].csd.road.oad.attflg;
			sendbuf[index++] = record.rcsd.csds.csd[i].csd.road.oad.attrindex;
			for(k=0; k<record.rcsd.csds.csd[i].csd.road.num; k++)
			{
				sendbuf[index++] = (record.rcsd.csds.csd[i].csd.road.oads[k].OI>>8) & 0xff;
				sendbuf[index++] = record.rcsd.csds.csd[i].csd.road.oads[k].OI & 0xff;
				sendbuf[index++] = record.rcsd.csds.csd[i].csd.road.oads[k].attflg;
				sendbuf[index++] = record.rcsd.csds.csd[i].csd.road.oads[k].attrindex;
			}
		}
	}

	if (record.datalen > 0)
	{
		sendbuf[index++] = 1;//choice 1  ,SEQUENCE OF A-RecordRow
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
	csinfo->prm = 0;
	index = FrameHead(csinfo,sendbuf);
	hcsi = index;
	index = index + 2;

	apduplace = index;		//记录APDU 起始位置
	sendbuf[index++] = GET_RESPONSE;
	sendbuf[index++] = response_type;
	sendbuf[index++] = 0;	//	piid
	if (oadnum>0)
		sendbuf[index++] = oadnum;
	memcpy(&sendbuf[index],response.data,response.datalen);
	index = index + response.datalen;
	sendbuf[index++] = 0;
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

int create_array(INT8U *data,INT8U numm)
{
	data[0] = dtarray;
	data[1] = numm;
	return 2;
}
int create_struct(INT8U *data,INT8U numm)
{
	data[0] = dtstructure;
	data[1] = numm;
	return 2;
}
int file_bool(INT8U *data,INT8U value)
{
	data[0] = dtbool;
	data[1] = value;
	return 2;
}
int fill_bit_string8(INT8U *data,INT8U bits)
{
	//TODO : 默认8bit ，不符合A-XDR规范
	data[0] = dtbitstring;
	data[1] = 0x08;
	data[2] = bits;
	return 3;
}

int fill_double_long_unsigned(INT8U *data,INT32U value)
{
	data[0] = dtdoublelongunsigned;
	data[1] = (value & 0xFF000000) >> 24 ;
	data[2] = (value & 0x00FF0000) >> 16 ;
	data[3] = (value & 0x0000FF00) >> 8 ;
	data[4] =  value & 0x000000FF;
	return 5;
}
int fill_visible_string(INT8U *data,char *value,INT8U len)
{
	data[0] = dtvisiblestring;
	data[1] = len;
	memcpy(&data[2],value,len);
	return (len+2);
}

int fill_integer(INT8U *data,INT8U value)
{
	data[0] = dtinteger;
	data[1] = value;
	return 2;
}
int fill_unsigned(INT8U *data,INT8U value)
{
	data[0] = dtunsigned;
	data[1] = value;
	return 2;
}

int fill_long_unsigned(INT8U *data,INT16U value)
{
	data[0] = dtlongunsigned;
	data[1] = (value & 0xFF00)>>8;
	data[2] = value & 0x00FF;
	return 3;
}
int fill_enum(INT8U *data,INT8U value)
{
	data[0] = dtenum;
	data[1] = value;
	return 2;
}
int fill_time(INT8U *data,INT8U *value)
{
	data[0] = dttime;
	memcpy(&data[1],&value[0],3);
	return 4;
}

int fill_DateTimeBCD(INT8U *data,DateTimeBCD *time)
{
	data[0] = dtdatetimes;
	time->year.data = time->year.data >>8 | time->year.data<<8;
	memcpy(&data[1],time,sizeof(DateTimeBCD));
	return (sizeof(DateTimeBCD)+1);
}

int GetMeterInfo(RESULT_NORMAL *response)
{
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

	oad = response->oad;
	data = response->data;
	DataTimeGet(&time);
	switch(oad.attflg )
	{
		case 2://安全模式选择
			response->datalen = fill_DateTimeBCD(response->data,&time);
			break;
	}
	return 0;
}

int Get4001_4002_4003(RESULT_NORMAL *response)
{
	int i;
	OAD oad;
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
			index += fill_visible_string(&data[index],class_tmp.assetcode,32);
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
			response->datalen = index;
			break;
		case 4:
			index += create_struct(&data[index],3);
//			index += fill_integer(&data[index],class_tmp.upleve);
//			index += fill_time(&data[index],class_tmp.startime1);
//			index += file_bool(&data[index],class_tmp.enable1);
			response->datalen = index;
			break;
	}
	return 0;
}

int GetEventRecord(RESULT_NORMAL *response)
{
	INT8U *data=NULL;
	INT16U datalen=0;

	if ( Get_Event(response->oad.OI,response->oad.attrindex,&data,(INT8U *)&datalen,memp) == 1 )
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
			case 0x310d:
				break;
			case 0x310c:
				break;
			case 0x310e:
				break;
			case 0x310F:
				break;
		}
		break;
	}
	return 0;
}

int doGetrecord(RESULT_RECORD *record)
{
	INT8U SelectorN = record->selectType;
	fprintf(stderr,"\n- getRequestRecord  OI = %04x  attrib=%d  index=%d",record->oad.OI,record->oad.attflg,record->oad.attrindex);
	int datalen=0;

	switch(SelectorN) {
	case 5:
	case 7:
//		getSelector(record->select,SelectorN,record->rcsd,(INT8U *)&record->data,datalen);
		break;
	case 9:		//指定读取上第n次记录
		Getevent_Record_Selector(record,memp);
		break;
	}
	return 1;
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

void printSel9(RESULT_RECORD record)
{
	fprintf(stderr,"\nSelector9:指定选取上第n次记录\n");
	fprintf(stderr,"\n选取上第%d次记录 ",record.select.selec9.recordn);
	fprintf(stderr,"\nRCSD个数：%d",record.rcsd.csds.num);
	printrcsd(record.rcsd);
}

int getRequestRecord(OAD oad,INT8U *data,CSINFO *csinfo,INT8U *sendbuf)
{
	RESULT_RECORD record;

	int index=0;
	memset(TmpDataBuf,0,sizeof(TmpDataBuf));
	record.oad = oad;
	record.data = TmpDataBuf;
	record.datalen = 0;
	fprintf(stderr,"\nGetRequestRecord   oi=%x  %02x  %02x",record.oad.OI,record.oad.attflg,record.oad.attrindex);
	index = get_BasicRSD(&data[index],(INT8U *)&record.select,&record.selectType);
	fprintf(stderr,"\nRSD Select%d  ",record.selectType);
	index +=get_BasicRCSD(&data[index],&record.rcsd.csds);
	if (record.selectType == 1)
	{
		fprintf(stderr,"\nOAD %04x %02x %02x",record.select.selec1.oad.OI,record.select.selec1.oad.attflg,record.select.selec1.oad.attrindex);
		fprintf(stderr,"\nData Type= %02x  Value=%d ",record.select.selec1.data.type,record.select.selec1.data.data[0]);
	}
	if(record.selectType==5)
	{
		printSel5(record);
	}
	if(record.selectType==9)
	{
		printSel9(record);
	}
	doGetrecord(&record);
	BuildFrame_GetResponseRecord(GET_REQUEST_RECORD,csinfo,record,sendbuf);
//	securetype = 0;		//清除安全等级标识
	return 1;
}
int GetEnvironmentValue(RESULT_NORMAL *response)
{
	switch(response->oad.OI)
	{
		case 0x4000:
			GetSysDateTime(response);
			break;
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
		case 3:			//事件类对象读取
			GetEventInfo(response);
			break;
		case 6:			//采集监控类对象
			GetCollPara(response);
			break;
		case 4:			//参变量类对象
			GetEnvironmentValue(response);
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
		getoad(oadtmp, &response.oad);
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
			TmpDataBufList[listindex + 4] = 0;//错误
			TmpDataBufList[listindex + 5] = response.dar;  //  0-3(oad)   4(choice)  5(dar)
			listindex = listindex + 6;
		}
	}
	response.data = TmpDataBufList;
	response.datalen = listindex;
	BuildFrame_GetResponse(GET_REQUEST_NORMAL_LIST,csinfo,oadnum,response,sendbuf);
	return 1;
}
