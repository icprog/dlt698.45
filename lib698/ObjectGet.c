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
extern INT8S (*pSendfun)(int fd,INT8U* sndbuf,INT16U sndlen);
extern int FrameHead(CSINFO *csinfo,INT8U *buf);
extern void FrameTail(INT8U *buf,int index,int hcsi);
extern int comfd;

typedef struct
{
	OAD oad;
	INT8U dar;		//错误信息
	INT8U *data;	//数据  上报时与 dar二选一
	INT16U datalen;	//数据长度
}RESULT_NORMAL;
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
	if (response.data!=NULL)
	{
		sendbuf[index++] = 1;//choice 1  ,Data有效
		memcpy(&sendbuf[index],response.data,response.datalen);
		index = index + response.datalen;
	}else
	{
		sendbuf[index++] = 0;//choice 0  ,DAR 有效 (数据访问可能的结果)
		sendbuf[index++] = 0x16;
		sendbuf[index++] = response.dar;
	}
	sendbuf[index++] = 0;
	sendbuf[index++] = 0;

	FrameTail(sendbuf,index,hcsi);

	if(pSendfun!=NULL)
		pSendfun(comfd,sendbuf,index+3);
	return (index+3);
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
int GetSecurePara(RESULT_NORMAL *response)
{
	INT8U *data=NULL;
	OAD oad;
	CLASS_F101 f101;
	oad = response->oad;
	data = response->data;
	readParaClass(0xf101,&f101,0);
	switch(oad.attflg )
	{
		case 2://安全模式选择
			data  = malloc(2);
			data[0] = 0x16;
			data[1] = 0x01;
			response->data = data;
			response->datalen = 2;
			break;
		case 3://安全模式参数array
			break;
	}
	return 0;
}
int getRequestRecord(INT8U *typestu,CSINFO *csinfo,INT8U *buf)
{
	RSD rsd;
	OAD oad={};
	INT8U rsdtype=0;
	//1,OAD
	oad.OI= (typestu[0]<<8) | typestu[1];
	oad.attflg = typestu[2];
	oad.attrindex = typestu[3];
	fprintf(stderr,"\n- getRequestRecord  OI = %04x  attrib=%d  index=%d",oad.OI,oad.attflg,oad.attrindex);

	//2,RSD
	rsdtype = typestu[4];
	switch(rsdtype)
	{
		case 0:
			//null
			break;
		case 1:
//			OAD oad1;
			break;
		case 2:
			break;
		case 3:
			break;
		case 4:
			break;
	}
	//3,RCSD
	return 1;
}
int doGetnormal(RESULT_NORMAL *response)
{
	INT16U oi = response->oad.OI;
	fprintf(stderr,"\ngetRequestNormal----------  oi =%04x  ",oi);
	switch(oi)
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
		case 0xF101:
			GetSecurePara(response);
			break;
	}
	return 0;
}
int getRequestNormal(OAD oad,INT8U *data,CSINFO *csinfo,INT8U *sendbuf)
{
	RESULT_NORMAL response;
	response.oad = oad;
	response.data = NULL;
	response.datalen = 0;
	doGetnormal(&response);
	BuildFrame_GetResponse(GET_REQUEST_NORMAL,csinfo,response,sendbuf);
	if (response.data!=NULL)
		free(response.data);
	return 1;
}
int getRequestNormalList(OAD oad,INT8U *data,CSINFO *csinfo,INT8U *sendbuf)
{
	return 1;
}
