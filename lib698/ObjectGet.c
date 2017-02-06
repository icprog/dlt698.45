/*
 * ObjectGet.c
 *
 *  Created on: Nov 12, 2016
 *      Author: ava
 */

#include <string.h>
#include <stdio.h>

#include "AccessFun.h"
#include "StdDataType.h"
#include "Objectdef.h"
void GetMeterInfo(INT16U attr_act,INT8U *data)
{
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
int getRequestNormal(OAD oad,INT8U *data)
{
	INT16U oi = oad.OI;
	INT8U attr = oad.attflg;
	fprintf(stderr,"\n----------  oi =%04x",oi);
	switch(oi)
	{
		case 0x6000:	//采集档案配置表
			GetMeterInfo(attr,data);
			break;
		case 0x6002:	//搜表
			break;
		case 0x6012:	//任务配置表
			TaskInfo(attr,data);
			break;
		case 0x6014:	//普通采集方案集
			CjiFangAnInfo(attr,data);
			break;
		case 0x6016:	//事件采集方案
			EventCjFangAnInfo(attr,data);
			break;
	}
	return 1;
}
