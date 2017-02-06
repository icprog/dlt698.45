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
void prtstat(int flg)
{
	if (flg == 1)
		fprintf(stderr,"\n保存成功");
	else
		fprintf(stderr,"\n保存失败");
}
INT16U set310d(INT8U attflg,INT8U index,INT8U *data)
{
	INT16U source_index=0,dest_index=0;
	Event310D_Object tmp310d;
	INT32U value=0;
	int saveflg=0;
	if ( attflg == 6 )
	{
		readCoverClass(0x310d,0,&tmp310d,sizeof(tmp310d),event_para_save);
		fprintf(stderr,"\ntmp310d 阈值=%x",tmp310d.poweroffset_obj.power_offset);
		get_BasicUnit(data,&source_index,(INT8U *)&value,&dest_index);
		tmp310d.poweroffset_obj.power_offset = value;
		fprintf(stderr,"\n电能表飞走事件：属性6 阈值=%x",value);
		saveflg = saveCoverClass(0x310d,0,&tmp310d,sizeof(tmp310d),event_para_save);
		prtstat(saveflg);
	}
	return source_index;
}
INT16U set310c(INT8U attflg,INT8U index,INT8U *data)
{
	INT16U source_index=0,dest_index=0;
	Event310C_Object tmp310c;
	INT32U value;
	int saveflg=0;
	if ( attflg == 6 )
	{
		readCoverClass(0x310c,0,&tmp310c,sizeof(tmp310c),event_para_save);
		fprintf(stderr,"\ntmp310c 阈值=%x",tmp310c.poweroffset_obj.power_offset);
		get_BasicUnit(data,&source_index,(INT8U *)&value,&dest_index);
		tmp310c.poweroffset_obj.power_offset = value;
		fprintf(stderr,"\n电能量超差事件：属性6 阈值=%x",value);
		saveflg = saveCoverClass(0x310c,0,&tmp310c,sizeof(tmp310c),event_para_save);
		prtstat(saveflg);
	}
	return source_index;
}
INT16U set310e(INT8U attflg,INT8U index,INT8U *data)
{
	INT16U source_index=0,dest_index=0;
	Event310E_Object tmp310e;
	TI value;
	int saveflg=0;
	if ( attflg == 6 )
	{
		readCoverClass(0x310e,0,&tmp310e,sizeof(tmp310e),event_para_save);
		fprintf(stderr,"\ntmp310e 阈值=%d 单位=%d",tmp310e.powerstoppara_obj.power_offset.interval,tmp310e.powerstoppara_obj.power_offset.units);
		get_BasicUnit(data,&source_index,(INT8U *)&value,&dest_index);
		tmp310e.powerstoppara_obj.power_offset = value;
		fprintf(stderr,"\n电能表停走事件：属性6 阈值=%d 单位=%d",value.interval,value.units);
		saveflg = saveCoverClass(0x310e,0,&tmp310e,sizeof(tmp310e),event_para_save);
		prtstat(saveflg);
	}
	return source_index;
}
INT16U set310f(INT8U attflg,INT8U index,INT8U *data)
{
	INT16U source_index=0,dest_index=0;
	Event310F_Object tmp310f;
	INT8U value;
	int saveflg = 0;
	if ( attflg == 6 )
	{
		readCoverClass(0x310f,0,&tmp310f,sizeof(tmp310f),event_para_save);
		get_BasicUnit(data,&source_index,(INT8U *)&value,&dest_index);
		tmp310f.collectfail_obj.retry_nums = value;
		fprintf(stderr,"\n终端抄表失败事件：属性6 重试轮次=%d ",value);
		saveflg = saveCoverClass(0x310f,0,&tmp310f,sizeof(tmp310f),event_para_save);
		prtstat(saveflg);
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
	fprintf(stderr,"\n事件类对象属性设置");
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
	fprintf(stderr,"\n参变量类对象属性设置");
	switch(oi)
	{
		case 0x4000://日期时间
			set4000(attr,oad.attrindex,data);
			break;
		case 0x4001://通信地址
			break;
		case 0x4002://表号
			break;
		case 0x4003://客户编号
			break;
		case 0x4004://设备地理位置
			break;
		case 0x4005://组地址
			break;
		case 0x4006://时钟源
			break;
		case 0x4007://LCD参数
			break;
		case 0x4030://电压合格率参数
			break;
	}
}
void CollParaSet(OAD oad,INT8U *data)
{
	INT16U oi = oad.OI;
	INT8U attr = oad.attflg;
	fprintf(stderr,"\n采集监控类对象属性设置");
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
}
int setRequestNormal(INT8U *data,OAD oad,CSINFO *csinfo,INT8U *buf)
{
	INT16U oi = oad.OI;
	INT8U oihead = (oad.OI&0xF000) >>12;
	fprintf(stderr,"\n对象属性设置  【 %04x 】",oi);

	switch(oihead)
	{
		case 0x3:		//事件对象
			EventSetAttrib(oad,data);
			break;
		case 0x4:		//参变量对象
			EnvironmentValue(oad,data);
			break;
		case 0x6:		//采集监控类对象
			CollParaSet(oad,data);
			break;
	}
	return 1;
}
int setRequestNormalList(INT8U *data,OAD oad)
{
	return 0;
}

