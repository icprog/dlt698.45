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
#include "EventObject.h"
extern void get_BasicUnit(INT8U *source,INT16U *sourceindex,INT8U *dest,INT16U *destindex);

INT16U set310d(INT8U attflg,INT8U index,INT8U *data)
{
	INT16U source_index=0,dest_index=0;
	Event310D_Object tmp310d;
	INT32U value=0;
	if ( attflg == 6 )
	{
		get_BasicUnit(data,&source_index,(INT8U *)&value,&dest_index);
		tmp310d.poweroffset_obj.power_offset = value;
		fprintf(stderr,"\n电能表飞走事件：属性6 阈值=%x",value);
	}
	return source_index;
}
INT16U set310c(INT8U attflg,INT8U index,INT8U *data)
{
	INT16U source_index=0,dest_index=0;
	Event310C_Object tmp310c;
	INT32U value;
	if ( attflg == 6 )
	{
		get_BasicUnit(data,&source_index,(INT8U *)&value,&dest_index);
		tmp310c.poweroffset_obj.power_offset = value;
		fprintf(stderr,"\n电能量超差事件：属性6 阈值=%x",value);
	}
	return source_index;
}
INT16U set310e(INT8U attflg,INT8U index,INT8U *data)
{
	INT16U source_index=0,dest_index=0;
	Event310E_Object tmp310e;
	TI value;
	if ( attflg == 6 )
	{
		get_BasicUnit(data,&source_index,(INT8U *)&value,&dest_index);
		tmp310e.powerstoppara_obj.power_offset = value;
		fprintf(stderr,"\n电能表停走事件：属性6 阈值=%d 单位=%d",value.interval,value.units);
	}
	return source_index;
}
INT16U set310f(INT8U attflg,INT8U index,INT8U *data)
{
	INT16U source_index=0,dest_index=0;
	Event310F_Object tmp310f;
	INT8U value;
	if ( attflg == 6 )
	{
		get_BasicUnit(data,&source_index,(INT8U *)&value,&dest_index);
		tmp310f.collectfail_obj.retry_nums = value;
		fprintf(stderr,"\n终端抄表失败事件：属性6 重试轮次=%d ",value);
	}
	return source_index;
}
INT16U set4000(INT8U attflg,INT8U index,INT8U *data)
{
	INT16U source_index=0,dest_index=0;
	DateTimeBCD datetime;
	if ( attflg == 2 )
	{
		get_BasicUnit(data,&source_index,(INT8U *)&datetime,&dest_index);
		fprintf(stderr,"\n终端对时：属性2  %d年-%d月-%d日 %d时:%d分:%d秒",datetime.year.data,datetime.month.data,datetime.day.data,datetime.hour.data,datetime.min.data,datetime.sec.data);
	}
	return source_index;
}
void EventSetAttrib(OAD oad,INT8U *data)
{
	INT16U oi = oad.OI;
	INT8U attr = oad.attflg;

	switch(oi)
	{
		case 0x310d:
			set310d(attr,oad.attrindex,data);
			break;
		case 0x310c:
			set310c(attr,oad.attrindex,data);
			break;
		case 0x310e:
			set310e(attr,oad.attrindex,data);
			break;
		case 0x310F:
			set310f(attr,oad.attrindex,data);
			break;
	}
}
void EnvironmentValue(OAD oad,INT8U *data)
{
	INT16U oi = oad.OI;
	INT8U attr = oad.attflg;

	switch(oi)
	{
		case 0x4000:
			set4000(attr,oad.attrindex,data);
			break;
	}
}
int setRequestNormal(INT8U *data,OAD oad,CSINFO *csinfo,INT8U *buf)
{
	INT16U oi = oad.OI;
	INT8U oihead = (oad.OI&0xF000) >>12;
	fprintf(stderr,"\n对象属性设置  oi =%04x",oi);
	switch(oi)
	{
		case 0x6000:	//采集档案配置表
			break;
		case 0x6002:	//搜表
			break;
		case 0x6012:	//任务配置表
			break;
		case 0x6014:	//普通采集方案集
			break;
		case 0x6016:	//事件采集方案
			break;
	}
	switch(oihead)
	{
		case 0x3:		//事件对象
			EventSetAttrib(oad,data);
			break;
		case 0x4:		//参变量对象
			EnvironmentValue(oad,data);
			break;
	}
	return 1;
}
int setRequestNormalList(INT8U *data,OAD oad)
{
	return 0;
}

