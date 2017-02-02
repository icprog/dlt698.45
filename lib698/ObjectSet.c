/*
 * ObjectSet.c
 *
 *  Created on: Nov 12, 2016
 *      Author: ava
 */

#include <string.h>
#include <stdio.h>

#include "AccessFun.h"
#include "StdDataType.h"
#include "Objectdef.h"
int setRequestNormal(OAD oad,INT8U *data)
{
	INT16U oi = oad.OI;
	INT8U attr = oad.attflg;
	fprintf(stderr,"\n----------  oi =%04x",oi);
	switch(oi)
	{
		case 0x6000:	//采集档案配置表
//			MeterInfo(attr_act,data);
			break;
		case 0x6002:	//搜表
			break;
		case 0x6012:	//任务配置表
//			TaskInfo(attr_act,data);
			break;
		case 0x6014:	//普通采集方案集
//			CjiFangAnInfo(attr_act,data);
			break;
		case 0x6016:	//事件采集方案
//			EventCjFangAnInfo(attr_act,data);
			break;
		case 0x310d:
	}
	return 1;
}
int setRequestNormalList(OAD oad,INT8U *data)
{
	int stepsize=1 , i=0 , bytes=0, objbytes=0;
	INT8U objectnum = Object[0];

	for(i=0 ; i< objectnum ; i++)
	{
		objbytes = setRequestNormal(Object+stepsize,csinfo,buf);
		if (objbytes >0)
			stepsize = stepsize + objbytes;
		else
			break;
	}
	return bytes;
}



