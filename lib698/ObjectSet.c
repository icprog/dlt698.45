/*
 * ObjectSet.c
 *
 *  Created on: Nov 12, 2016
 *      Author: ava
 */

#include <string.h>
#include <stdio.h>
#include <malloc.h>

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

INT16U set310d(OI_698 oi,INT8U attflg,INT8U *data,INT8U *DAR)
{
	Event310D_Object tmp310d;
	INT16U  source_index=0,dest_index=0;
	int 	saveflg=0;

	saveflg = readCoverClass(oi,0,&tmp310d,sizeof(Event310D_Object),event_para_save);
	fprintf(stderr,"\n[310d]电能表飞走事件 阈值=%d 任务号=%d\n",tmp310d.poweroffset_obj.power_offset,tmp310d.poweroffset_obj.task_no);

	switch(attflg) {
	case 6:
		get_BasicUnit(data,&source_index,(INT8U *)&tmp310d.poweroffset_obj,&dest_index);
		fprintf(stderr,"\n：属性6 阈值=%d 任务号=%d",tmp310d.poweroffset_obj.power_offset,tmp310d.poweroffset_obj.task_no);
		saveflg = saveCoverClass(oi,0,&tmp310d,sizeof(Event310D_Object),event_para_save);
		break;
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

///////////////////////////////////////////////////////////////////////////////
/*
 * 根据oi参数，查找相应的class_info的结构体数据
 * */
INT16S getEventClassLen(OI_698 oi)
{
	INT16U i=0;

	for(i=0; i < sizeof(event_class_len)/sizeof(EVENT_CLASS_INFO);i++)
	{
		if(event_class_len[i].oi == oi) {
			return event_class_len[i].classlen;
		}
	}
	fprintf(stderr,"未找到OI=%04x的相关结构长度！！！\n",oi);
	return -1;
}


INT16U setClass7attr(OAD oad,INT8U *data,INT8U *DAR)
{
	INT8U*	eventbuff=NULL;
	int 	saveflg=0;
	INT16S	classlen=0;
	INT16U 	source_index=0,dest_index=0;
	Class7_Object	class7={};
	INT8U	str[OCTET_STRING_LEN]={};

	classlen = getEventClassLen(oad.OI);
	eventbuff = (INT8U *)malloc(classlen);
	if(eventbuff!=NULL) {
		memset(eventbuff,0,classlen);
	}
	saveflg = readCoverClass(oad.OI,0,eventbuff,classlen,event_para_save);
//	fprintf(stderr,"\n设置前：clsslen=%d\n",classlen);
//	for(int i=0;i<classlen;i++) {
//		fprintf(stderr,"%02x ",eventbuff[i]);
//		if(i%16==0) fprintf(stderr," \n");
//	}
	memcpy(&class7,eventbuff,sizeof(Class7_Object));
	switch(oad.attflg) {
	case 1:	//逻辑名
		fprintf(stderr,"\n设置前:class7.oi = %04x",class7.oi);
		fprintf(stderr,"data=%02x %02x %02x %02x %02x\n",data[0],data[1],data[2],data[3],data[4]);
		get_BasicUnit(data,&source_index,(INT8U *)&str[0],&dest_index);
		fprintf(stderr,"str=%02x %02x %02x %02x %02x\n",str[0],str[1],str[2],str[3],str[4]);
		class7.oi = (str[0]<<8) | str[1];
		fprintf(stderr,"\n设置后:class7.oi = %04x",class7.oi);
		break;
	case 3:	//关联属性表
		get_BasicUnit(data,&source_index,(INT8U *)&class7.oadarr,&dest_index);
		fprintf(stderr,"\n设置:class7.关联属性表 = %04x-%02x%02x",class7.oadarr[0].OI,class7.oadarr[0].attflg,class7.oadarr[0].attrindex);
		break;
	case 4:	//当前记录数
		get_BasicUnit(data,&source_index,(INT8U *)&class7.crrentnum,&dest_index);
		fprintf(stderr,"\n设置:class7.当前记录数 = %d",class7.crrentnum);
		break;
	case 5:	//最大记录数
		get_BasicUnit(data,&source_index,(INT8U *)&class7.maxnum,&dest_index);
		fprintf(stderr,"\n设置:class7.最大记录数 = %d",class7.maxnum);
		break;
	case 8: //上报标识
		get_BasicUnit(data,&source_index,(INT8U *)&class7.reportflag,&dest_index);
		fprintf(stderr,"\n设置:class7.上报标识 = %d",class7.reportflag);
		break;
	case 9: //有效标识
		get_BasicUnit(data,&source_index,(INT8U *)&class7.enableflag,&dest_index);
		fprintf(stderr,"\n设置:class7.有效标识 = %d",class7.enableflag);
		break;
	}
	memcpy(eventbuff,&class7,sizeof(Class7_Object));
	saveflg = saveCoverClass(oad.OI,0,eventbuff,classlen,event_para_save);
	prtstat(saveflg);
	free(eventbuff);
	eventbuff=NULL;
	*DAR = success;
	return source_index;
}

INT8U EventSetAttrib(OAD oad,INT8U *data)
{
	INT8U	DAR=success;
	OI_698  oi = oad.OI;
	INT8U   attr = oad.attflg;
	INT16S	classlen=0;

	fprintf(stderr,"\n事件类对象属性设置");
	classlen = getEventClassLen(oi);
	if(classlen == -1) {
		DAR = obj_unexist;
		return DAR;
	}
	switch(attr) {
	case 1:	//逻辑名
	case 3:	//关联属性表
	case 4:	//当前记录数
	case 5:	//最大记录数
	case 8: //上报标识
	case 9: //有效标识
		setClass7attr(oad,data,&DAR);
		break;
	case 6:	//配置参数
		switch(oi) {
			case 0x310d:
				set310d(oi,attr,data,&DAR);
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
		break;
	}
	return success;
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
	return success;
}
int setRequestNormalList(INT8U *data,OAD oad)
{
	return 0;
}

