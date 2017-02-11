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
extern INT8S (*pSendfun)(int fd,INT8U* sndbuf,INT16U sndlen);
extern int FrameHead(CSINFO *csinfo,INT8U *buf);
extern void FrameTail(INT8U *buf,int index,int hcsi);
extern INT8U Get_Event(OI_698 oi,INT8U eventno,INT8U** Getbuf,int *Getlen,ProgramInfo* prginfo_event);
extern int get_BasicRSD(INT8U *source,INT8U *dest,INT8U *type);
extern int get_BasicRCSD(INT8U *source,INT8U *dest);
extern int comfd;
extern INT8U TmpDataBuf[MAXSIZ_FAM];
extern ProgramInfo *memp;

typedef struct
{
	OAD oad;
	INT8U dar;		//错误信息
	INT8U *data;	//数据  上报时与 dar二选一
	INT16U datalen;	//数据长度
}RESULT_NORMAL;
typedef struct
{
	OAD oad;
	RCSD rcsd;
	INT8U *data;	//数据  上报时与 dar二选一
	INT16U datalen;	//数据长度
	INT8U selectType;//选择类型
	RSD   select;	 //选择方法实例
}RESULT_RECORD;

int BuildFrame_GetResponse(INT8U response_type,CSINFO *csinfo,RESULT_NORMAL response,INT8U *sendbuf)
{
	int index=0, hcsi=0;
	csinfo->dir = 1;
	csinfo->prm = 0;
	index = FrameHead(csinfo,sendbuf);
	hcsi = index;
	index = index + 2;
	sendbuf[index++] = GET_RESPONSE;
	sendbuf[index++] = response_type;
	sendbuf[index++] = 0;	//	piid
	sendbuf[index++] = (response.oad.OI>>8) & 0xff;
	sendbuf[index++] = response.oad.OI & 0xff;
	sendbuf[index++] = response.oad.attflg;
	sendbuf[index++] = response.oad.attrindex;
	if (response.datalen > 0)
	{
		sendbuf[index++] = 1;//choice 1  ,Data有效
		memcpy(&sendbuf[index],response.data,response.datalen);
		index = index + response.datalen;
	}else
	{
		sendbuf[index++] = 0;//choice 0  ,DAR 有效 (数据访问可能的结果)
//		sendbuf[index++] = 0x16;
		sendbuf[index++] = response.dar;
	}
	sendbuf[index++] = 0;
	sendbuf[index++] = 0;
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
int fill_bit_string8(INT8U *data,INT8U bits)
{
	//TODO : 默认8bit ，不符合A-XDR规范
	data[0] = dtbitstring;
	data[1] = 0x08;
	data[2] = bits;
	return 3;
}

int fill_visible_string(INT8U *data,INT8U numm,char *source)
{
	data[0] = dtvisiblestring;
	data[1] = numm;
	memcpy(&data[2],source,numm);
	return (2+numm);
}

int fill_unsigned(INT8U *data,INT8U value)
{
	data[0] = dtunsigned;
	data[1] = value;
	return 2;
}

int fill_enum(INT8U *data,INT8U value)
{
	data[0] = dtenum;
	data[1] = value;
	return 2;
}

int fill_DateTimeBCD(INT8U *data,DateTimeBCD *time)
{
	data[0] = dtdatetimes;
	time->year.data = time->year.data >>8 | time->year.data<<8;
	memcpy(&data[1],time,sizeof(DateTimeBCD));
	return (sizeof(DateTimeBCD)+1);
}

int fill_Verion(INT8U *data,VERINFO info)
{
	INT8U index = 0;
	index += fill_visible_string(&data[index],sizeof(info.factoryCode),info.factoryCode);
	index += fill_visible_string(&data[index],sizeof(info.softVer),info.softVer);
	index += fill_visible_string(&data[index],sizeof(info.softDate),info.softDate);
	index += fill_visible_string(&data[index],sizeof(info.hardVer),info.hardVer);
	index += fill_visible_string(&data[index],sizeof(info.hardDate),info.hardDate);
	index += fill_visible_string(&data[index],sizeof(info.factoryExpInfo),info.factoryExpInfo);
	return index;
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
int GetSecurePara(RESULT_NORMAL *response)
{
	INT8U 	*data=NULL;
	OAD 	oad;
	CLASS_F101 f101;
	oad = response->oad;
	data = response->data;
	readCoverClass(0xf101,0,&f101,sizeof(f101),para_vari_save);
	switch(oad.attflg )
	{
		case 2://安全模式选择
			response->datalen = fill_enum(&data[0],(f101.active)&0x01);
			break;
		case 3://安全模式参数array

			break;
	}
	return 0;
}


int GetDevice(RESULT_NORMAL *response)
{
	INT8U *data=NULL;
	CLASS19	 oi4300={};
	int		index=0;
	data = response->data;
	memset(&oi4300,0,sizeof(CLASS19));
	readCoverClass(0x4300,0,&oi4300,sizeof(CLASS19),para_vari_save);
	switch(response->oad.attflg) {
	case 3:	//版本信息
		index += create_struct(&data[index],6);
		index += fill_Verion(&data[index],oi4300.info);
		response->datalen = index;
		break;
	case 6://支持规约列表
		index += create_array(&data[index],1);
		index += fill_visible_string(&data[index],strlen(oi4300.protcol),oi4300.protcol);
		response->datalen = index;
		break;
	}
	return 0;
}

int GetNetInter(RESULT_NORMAL *response)
{
	INT8U *data=NULL;
	CLASS25	 netinfo={};
	int		index=0;
	data = response->data;
	memset(&netinfo,0,sizeof(CLASS25));
	readCoverClass(response->oad.OI,0,&netinfo,sizeof(CLASS25),para_vari_save);
	switch(response->oad.attflg) {
	case 5:	//版本信息
		index += create_struct(&data[index],6);
		index += fill_Verion(&data[index],netinfo.info);
		response->datalen = index;
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

int GetEventInfo(RESULT_NORMAL *response)
{
	INT8U *data=NULL;
	int datalen=0;
	if ( Get_Event(response->oad.OI,response->oad.attrindex,&data,(int *)&datalen,memp) == 1 )
	{
		fprintf(stderr,"datalen=%d\n",datalen);
		if (datalen > 512 || data==NULL)
		{
			fprintf(stderr,"\n获取事件数据Get_Event函数异常! [datalen=%d  data=%p]",datalen,data);
			if (data!=NULL)
				free(data);
			return 0;
		}
		memcpy(response->data,data,datalen);
		response->datalen = datalen;
		response->dar = success;
		if (data!=NULL)
			free(data);
		return 1;
	}
	response->datalen = 0;
	response->dar = obj_unexist;
	fprintf(stderr,"\n获取事件数据Get_Event函数返回 0  [datalen=%d  data=%p]",datalen,data);
	if (data!=NULL)
		free(data);

	return 0;
}

int GetParaVarInfo(RESULT_NORMAL *response)
{
	switch(response->oad.OI) {
	case 0x4000:	//日期时间
		GetSysDateTime(response);
		break;
	case 0x4300:	//电气设备
		GetDevice(response);
		break;
	case 0x4500:	//公网通信模块1
	case 0x4501:	//公网通信模块2
		GetNetInter(response);
		break;
	}
	return 0;
}

int GetCollInfo(RESULT_NORMAL *response)
{
	switch(response->oad.OI) {
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
	return 0;
}

int GetDevInfo(RESULT_NORMAL *response)
{
	switch(response->oad.OI) {
		case 0xF101:
			GetSecurePara(response);
			break;
		case 0xF203:
			GetYxPara(response);
			break;
	}
	return 0;
}

int doGetrecord(RESULT_RECORD *record)
{
	RSD rsd={};
	INT8U SelectorN = record->selectType;
	fprintf(stderr,"\n- getRequestRecord  OI = %04x  attrib=%d  index=%d",record->oad.OI,record->oad.attflg,record->oad.attrindex);

	switch(SelectorN)
	{
		case 0:
			//null
			break;
		case 1://Selector1
			break;
		case 2://Selector2
			break;
		case 3://Selector3
			break;
		case 4://Selector4
			break;
		case 5://Selector5
			break;
	}
	//3,RCSD
	return 1;
}
int doGetnormal(RESULT_NORMAL *response)
{
	INT16U oi = response->oad.OI;

	fprintf(stderr,"\ngetRequestNormal----------  oi =%04x  ",oi);

	INT8U oihead = (oi & 0xF000) >>12;
	switch(oihead) {
	case 3:			//事件类对象读取
		GetEventInfo(response);
		break;
	case 4:			//参变量类对象
		GetParaVarInfo(response);
		break;
	case 6:			//采集监控类对象
		GetCollInfo(response);
		break;
	case 0x0F:		//文件传输，ESAM，输入输出设备
		GetDevInfo(response);
		break;
	}
	return 1;
}
int getRequestNormal(OAD oad,INT8U *data,CSINFO *csinfo,INT8U *sendbuf)
{
	RESULT_NORMAL response;

	memset(TmpDataBuf,0,sizeof(TmpDataBuf));
	response.oad = oad;
	response.data = TmpDataBuf;
	response.datalen = 0;
	doGetnormal(&response);
	BuildFrame_GetResponse(GET_REQUEST_NORMAL,csinfo,response,sendbuf);
	return 1;
}
int getRequestRecord(OAD oad,INT8U *data,CSINFO *csinfo,INT8U *sendbuf)
{
	RESULT_RECORD record;
	int index=0;
	memset(TmpDataBuf,0,sizeof(TmpDataBuf));
	record.oad = oad;
	record.data = TmpDataBuf;
	record.datalen = 0;
	index = get_BasicRSD(data[index],(INT8U *)&record.select,&record.selectType);
	index +=get_BasicRCSD(data[index],(INT8U *)&record.rcsd);
	doGetrecord(&record);
//	BuildFrame_GetResponse(GET_REQUEST_RECORD,csinfo,record,sendbuf);
	return 1;
}

int getRequestNormalList(OAD oad,INT8U *data,CSINFO *csinfo,INT8U *sendbuf)
{
	return 1;
}
