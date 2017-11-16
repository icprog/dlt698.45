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
#include "PublicFunction.h"
#include "event.h"
#include "dlt698.h"
#include "dlt698def.h"
#include "OIfunc.h"
#include "OIsetfunc.h"
#include "class8.h"
#include "class12.h"
#include "class23.h"

extern int doReponse(int server,int reponse,CSINFO *csinfo,int datalen,INT8U *data,INT8U *buf);
extern int doGetnormal(INT8U seqOfNum,RESULT_NORMAL *response);
extern ProgramInfo *memp;
extern INT8U TmpDataBuf[MAXSIZ_FAM];
extern INT8U TmpDataBufList[MAXSIZ_FAM*2];
extern TimeTag	Response_timetag;		//响应的时间标签值

//INT8U prtstat(int flg)
//{
//	if (flg == 1) {
//		fprintf(stderr,"\n保存成功");
//		return success;
//	}else {
//		fprintf(stderr,"\n保存失败");
//		return refuse_rw;
//	}
//}

INT16U setclass18(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U index=0;
	CLASS18	class18={};

	memset(&class18,0,sizeof(CLASS18));
	readCoverClass(0x18,0,&class18,sizeof(CLASS18),para_vari_save);
	fprintf(stderr,"oad=%x attflg=%d\n",oad.OI,oad.attflg);
	switch(oad.attflg) {
	case 2://文件信息
		index += getStructure(&data[index],NULL,DAR);
		index += getVisibleString(1,VISIBLE_STRING_LEN,&data[index],(INT8U *)&class18.source_file,DAR);
		index += getVisibleString(1,VISIBLE_STRING_LEN,&data[index],(INT8U *)&class18.dist_file,DAR);
		index += getDouble(&data[index],(INT8U *)&class18.file_size);
		index += getBitString(1,&data[index],(INT8U *)&class18.file_attr);
		index += getVisibleString(1,VISIBLE_STRING_LEN,&data[index],(INT8U *)&class18.file_version,DAR);
		index += getEnum(1,&data[index],(INT8U *)&class18.file_type);
		fprintf(stderr,"source=%s,dist=%s\n,size-%d,attr=%d,version=%s,file_type=%d\n",
				&class18.source_file[1],&class18.dist_file[1],class18.file_size,class18.file_attr,&class18.file_version[1],class18.file_type);
		break;
	case 3://命令结果
		index += getEnum(1,&data[index],(INT8U *)&class18.cmd_result);
		break;
	}
	*DAR = saveCoverClass(0x18,0,&class18,sizeof(CLASS18),para_vari_save);
	return index;
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
	int 	i=0;
	INT16S	classlen=0;
	Class7_Object	class7={};
	INT8U	str[OCTET_STRING_LEN]={};
	int		index = 0;

	classlen = getEventClassLen(oad.OI);
	eventbuff = (INT8U *)malloc(classlen);
	if(eventbuff!=NULL) {
		memset(eventbuff,0,classlen);
	}
	readCoverClass(oad.OI,0,eventbuff,classlen,event_para_save);
//	fprintf(stderr,"\n设置前：clsslen=%d\n",classlen);
//	for(int i=0;i<classlen;i++) {
//		fprintf(stderr,"%02x ",eventbuff[i]);
//		if(i%16==0) fprintf(stderr," \n");
//	}
	memcpy(&class7,eventbuff,sizeof(Class7_Object));
	switch(oad.attflg) {
	case 1:	//逻辑名
		memset(str,0,sizeof(str));
		fprintf(stderr,"\n设置前:class7.logic_name = %s",class7.logic_name);
		index += getOctetstring(1,OCTET_STRING_LEN-1,&data[index],(INT8U *)&class7.logic_name[1],&class7.logic_name[0],DAR);
		fprintf(stderr,"\n设置后:class7.logic_name = %s",class7.logic_name);
		break;
	case 3:	//关联属性表
		index += getArray(&data[index],&class7.class7_oad.num,DAR);
		for(i=0;i<class7.class7_oad.num;i++) {
			index += getOAD(1,&data[index],&class7.class7_oad.oadarr[i],DAR);
		}
		fprintf(stderr,"\n设置:class7.关联属性表num=%d ",class7.class7_oad.num);
		for(i=0;i<class7.class7_oad.num;i++) {
			fprintf(stderr,"\n%04x-%02x%02x",class7.class7_oad.oadarr[i].OI,class7.class7_oad.oadarr[i].attflg,class7.class7_oad.oadarr[i].attrindex);
		}
		break;
	case 4:	//当前记录数
		index += getLongUnsigned(&data[index],(INT8U *)&class7.crrentnum);
		fprintf(stderr,"\n设置:class7.当前记录数 = %d",class7.crrentnum);
		break;
	case 5:	//最大记录数
		index += getLongUnsigned(&data[index],(INT8U *)&class7.maxnum);
		fprintf(stderr,"\n设置:class7.最大记录数 = %d",class7.maxnum);
		break;
	case 8: //上报标识
		index += getEnum(1,&data[index],(INT8U *)&class7.reportflag);
		fprintf(stderr,"\n设置:class7.上报标识 = %d",class7.reportflag);
		break;
	case 9: //有效标识
		index += getBool(&data[index],(INT8U *)&class7.enableflag,DAR);
		fprintf(stderr,"\n设置:class7.有效标识 = %d",class7.enableflag);
		break;
	}
	memcpy(eventbuff,&class7,sizeof(Class7_Object));
	*DAR = saveCoverClass(oad.OI,0,eventbuff,classlen,event_para_save);
	free(eventbuff);
	eventbuff=NULL;
	return index;
}

INT16U EventSetAttrib(OAD oad,INT8U *data,INT8U *DAR)
{
	OI_698  oi = oad.OI;
	INT8U   attr = oad.attflg;
	INT16S	classlen=0;
	INT16U	data_index=0;

	fprintf(stderr,"\n事件类对象属性设置  oi=%04x\n",oi);
	classlen = getEventClassLen(oi);
	if(classlen == -1) {
		*DAR = obj_unexist;
		return *DAR;
	}
	switch(attr) {
	case 1:	//逻辑名
	case 3:	//关联属性表
	case 4:	//当前记录数
	case 5:	//最大记录数
	case 8: //上报标识
	case 9: //有效标识
		data_index = setClass7attr(oad,data,DAR);
		break;
	case 6:	//配置参数
		switch(oi) {
		case 0x300F:	//电能表电压逆相序事件
			data_index = set300F(oad,data,DAR);
			break;
		case 0x3105:	//电能表时钟超差事件
			data_index = set3105(oad,data,DAR);
			break;
		case 0x3106:	//终端停上电事件
			data_index = set3106(oad,data,DAR);
			break;
		case 0x310c:	//电能量超差事件阈值
			data_index = set310c(oad,data,DAR);
			break;
		case 0x310d:	//电能表飞走事件阈值
			data_index = set310d(oad,data,DAR);
			break;
		case 0x310e:	//电能表停走事件阈值
			data_index = set310e(oad,data,DAR);
			break;
		case 0x310F:	//终端抄表失败事件
			data_index = set310f(oad,data,DAR);
			break;
		case 0x3110:	//月通信流量超限事件阈值
			data_index = set3110(oad,data,DAR);
			break;
		case 0x311c:	//电能表数据变更监控记录
			data_index = set311c(oad,data,DAR);
			break;
		}
		break;
	}
	return data_index;
}

INT16U EnvironmentValue(OAD oad,INT8U *data,INT8U *DAR)
{
	fprintf(stderr,"\n参变量类对象属性设置");
	INT16U	data_index=0;
	switch(oad.OI)
	{
		case 0x4000://日期时间
			data_index = set4000(oad,data,DAR);
			break;
		case 0x4001://通信地址
		case 0x4002://表号
		case 0x4003://客户编号
			data_index = set4001_4002_4003(oad,data,DAR);
			break;
		case 0x4004://设备地理位置
			data_index = set4004(oad,data,DAR);
			break;
		case 0x4005://组地址
			data_index = set4005(oad,data,DAR);
			break;
		case 0x4006://时钟源
			data_index = set4006(oad,data,DAR);
			break;
		case 0x4007://LCD参数
			data_index = set4007(oad,data,DAR);
			break;
		case 0x400C://时区时段数
			data_index = set400c(oad,data,DAR);
			break;
		case 0x4014://当前套时区表
			data_index = set4014(oad,data,DAR);
			break;
		case 0x4016://当前套日时段表
			data_index = set4016(oad,data,DAR);
			break;
		case 0x4024://剔除
			data_index = set4024(oad,data,DAR);
			break;
		case 0x4030://电压合格率参数
			data_index = set4030(oad,data,DAR);
			break;
		case 0x4103://资产管理编码
			data_index = set4103(oad,data,DAR);
			break;
		case 0x4202://级联通信参数
			data_index = set4202(oad,data,DAR);
			break;
		case 0x4204://终端广播校时
			data_index = set4204(oad,data,DAR);
			break;
		case 0x4300://电气设备
			data_index = set4300(oad,data,DAR);
			break;
		case 0x4400:
			data_index = set4400(oad,data,DAR);
			break;
		case 0x4500:
			data_index = set4500(oad,data,DAR);
			break;
		case 0x4510://以太网通信接口类
			data_index = set4510(oad,data,DAR);
			break;
		case 0x4018://当前套费率电价
			data_index = set4018(oad,data,DAR);
			break;
	}
	return data_index;
}

INT16U CollParaSet(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U	data_index=0;
	INT16U oi = oad.OI;
//	INT8U attr = oad.attflg;
	fprintf(stderr,"\n采集监控类对象属性设置");
	*DAR = refuse_rw;
	switch(oi)
	{
		case 0x6000:	//采集档案配置表
			break;
		case 0x6002:	//搜表
			set6002(oad,data,DAR);
			break;
		case 0x6012:	//任务配置表
			break;
		case 0x6014:	//普通采集方案集
			break;
		case 0x6016:	//事件采集方案
			break;
	}
	return data_index;
}

INT16U DeviceIoSetAttrib(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U	data_index=0;
	fprintf(stderr,"\n输入输出设备类对象属性设置");
	switch(oad.OI)
	{
		case 0xF000://文件分帧传输管理
		case 0xF001://文件分开传输管理
		case 0xF002://文件扩展传输管理
			data_index = setclass18(oad,data,DAR);
			break;
		case 0xF101:
			data_index = setf101(oad,data,DAR);
			break;
		case 0xF203:	//开关量输入
			data_index = setf203(oad,data,DAR);
			break;
		case 0xF206:	//告警输出
			data_index = setf206(oad,data,DAR);
			break;
	}
	return data_index;
}

INT16U ALSetAttrib(OAD oad, INT8U *data, INT8U *DAR) {
	INT16U data_index = 0;
	switch (oad.OI) {
		case 0x2301:
		case 0x2302:
		case 0x2303:
		case 0x2304:
		case 0x2305:
		case 0x2306:
		case 0x2307:
		case 0x2308:
			data_index = class23_set(oad, data, DAR);
			break;
		case 0x2401:
		case 0x2402:
//		case 0x2403:
//		case 0x2404:
//		case 0x2405:
//		case 0x2406:
//		case 0x2407:
//		case 0x2408:
			data_index = class12_set(oad, data, DAR);
			break;
		case 0x8000://遥控
			data_index = class8000_set(oad, data, DAR);
			break;
		case 0x8001://保电设置
			data_index = class8001_set(oad, data, DAR);
			break;
		case 0x8003://一般中文信息
		case 0x8004://重要中文信息
			data_index = class8003_8004_set(oad, data, DAR);
			break;
		case 0x8100://终端保安定值
			data_index = class8100_set(oad, data, DAR);
			break;
        case 0x8101:
            data_index = class8101_set(oad, data, DAR);
            break;
        case 0x8102:
            data_index = class8102_set(oad, data, DAR);
            break;
        case 0x8103:
            data_index = class8103_set(oad, data, DAR);
            break;
        case 0x8104:
            data_index = class8104_set(oad, data, DAR);
            break;
        case 0x8105:
            data_index = class8105_set(oad, data, DAR);
            break;
        case 0x8107:
        	data_index = class8107_set(oad, data, DAR);
            break;
        case 0x8108:
        	data_index = class8108_set(oad, data, DAR);
            break;
	}
	return data_index;
}

INT16U setRequestNormal(INT8U *data,OAD oad,INT8U *DAR,CSINFO *csinfo,INT8U *buf)
{
	INT8U oihead = (oad.OI&0xF000) >>12;
	INT16U	data_index=0;

	fprintf(stderr,"\n对象属性设置  【 %04x 】",oad.OI);

	if(Response_timetag.effect==0) {
		*DAR = timetag_invalid;
		return data_index;
	}
	switch(oihead)
	{
		case 0x2:
			data_index = ALSetAttrib(oad,data,DAR);
			break;
		case 0x3:		//事件对象
			data_index = EventSetAttrib(oad,data,DAR);
			break;
		case 0x4:		//参变量对象
			data_index = EnvironmentValue(oad,data,DAR);
			break;
		case 0x6:		//采集监控类对象
			data_index = CollParaSet(oad,data,DAR);
			break;
		case 0x08:
			data_index = ALSetAttrib(oad,data,DAR);
			break;
		case 0xf:		//输入输出设备类对象 + ESAM接口类对象
			data_index = DeviceIoSetAttrib(oad,data,DAR);
			break;
		default:
			data_index = get_Data(data,NULL);	//一致性测试中，为了查找解析后续的OAD的内容
			fprintf(stderr,"oad.oi=%04x,data_index=%d\n",oad.OI,data_index);
			*DAR = obj_undefine;
			break;
	}
	if(*DAR==success) {		//参数文件更改，通知进程
		setOIChange(oad.OI);
	}
	return data_index;
}

int setRequestNormalList(INT8U *data,CSINFO *csinfo,INT8U *buf)
{
	INT8U DAR=success;
	OAD  oad={};
	OAD  event_oad[5]={};
	INT8U oadnum = 0,event_oadnum=0;
	int i=0,listindex=0;
	int sourceindex=0;		//源数据的索引

	oadnum = data[sourceindex++];
	fprintf(stderr,"\nsetRequestNormalList!! OAD_NUM=%d\n",oadnum);
	memset(TmpDataBufList,0,sizeof(TmpDataBufList));
	TmpDataBufList[listindex++] = oadnum;
	memset(&event_oad,0,sizeof(event_oad));
	for(i=0;i<oadnum;i++)
	{
		sourceindex += getOAD(0,&data[sourceindex],&oad,NULL);
//
//		if(Response_timetag.effect) {
//			memset(TmpDataBuf,0,sizeof(TmpDataBuf));
//			getOAD(0,&apdu[3],&oad);
//			data = &apdu[7];					//Data
//			setRequestNormal(data,oad,&DAR,NULL,buf);
//		}else {
//			DAR = timetag_invalid;
//		}

		if(oad.OI!=0x4300 && oad.OI!=0x4000 && ((oad.OI>>12)==0x04)) {
			memcpy(&event_oad[event_oadnum],&oad,sizeof(OAD));
			event_oadnum++;
		}
		DAR = success;		//每一个OAD的设置初始值设置为成功
		sourceindex += setRequestNormal(&data[sourceindex],oad,&DAR,NULL,buf);
		listindex += create_OAD(0,&TmpDataBufList[listindex],oad);
		TmpDataBufList[listindex++] = (INT8U)DAR;
	}
	doReponse(SET_RESPONSE,SET_REQUEST_NORMAL_LIST,csinfo,listindex,TmpDataBufList,buf);
	//此处处理防止在设置后未上送应答帧而直接上送事件报文
//	for(i=0;i<event_oadnum;i++) {
//		Get698_event(event_oad[i],memp);
//	}
	if(Response_timetag.effect==1)  {		//有效 产生事件
		Get698_3118_moreoad(event_oad,event_oadnum,memp);
	}
	return 0;
}


int setThenGetRequestNormalList(INT8U *data,CSINFO *csinfo,INT8U *buf)
{
	INT8U DAR=success;
	OAD  oad={};
	OAD  event_oad[5]={};
	INT8U seqofNum = 0,event_oadnum=0;
	INT8U get_delay = 0;		//延时读取时间
	int i=0,listindex=0;
	int sourceindex=0;		//源数据的索引
	RESULT_NORMAL response={};

	seqofNum = data[sourceindex++];
	fprintf(stderr,"\nsetRequestNormalList!! OAD_NUM=%d\n",seqofNum);
	memset(TmpDataBufList,0,sizeof(TmpDataBufList));
	TmpDataBufList[listindex++] = seqofNum;
	memset(&event_oad,0,sizeof(event_oad));
	for(i=0;i<seqofNum;i++)
	{
		sourceindex += getOAD(0,&data[sourceindex],&oad,NULL);		//一个设置的对象属性   OAD
		if(oad.OI==0x4300 && oad.OI==0x4000 && ((oad.OI>>12)==0x04)) {
			memcpy(&event_oad[event_oadnum],&oad,sizeof(OAD));
			event_oadnum++;
		}

		fprintf(stderr,"set 1 sourceindex=%d\n",sourceindex);
		sourceindex += setRequestNormal(&data[sourceindex],oad,&DAR,NULL,buf);	//设置对象数据     Data

		fprintf(stderr,"set end sourceindex=%d\n",sourceindex);

		listindex += create_OAD(0,&TmpDataBufList[listindex],oad);
		TmpDataBufList[listindex++] = (INT8U)DAR;

		sourceindex += getOAD(0,&data[sourceindex],&oad,NULL);		//一个读取的对象属性   OAD
		get_delay = data[sourceindex];						//延时读取时间
		sourceindex++;
		//get_delay = 0, 	//按照ProtocolConformance协商结果
		if(get_delay>=0 && get_delay<=5) 	sleep(get_delay);
		memset(TmpDataBuf,0,sizeof(TmpDataBuf));
		memcpy(&response.oad,&oad,sizeof(response.oad));
		response.datalen = 0;
		response.data = TmpDataBuf + 5;		//4 + 1             oad（4字节） + choice(1字节)
		doGetnormal(0,&response);
		listindex += create_OAD(0,&TmpDataBufList[listindex],oad);
		if(response.datalen>0) {
			TmpDataBufList[listindex++] = 1;		//Get-Result∷=CHOICE  Data		国网软件需要，但是报文解析软件不认
			memcpy(&TmpDataBufList[listindex],response.data,response.datalen);
			listindex = listindex + response.datalen;
		}else {
			TmpDataBufList[listindex++] = 0;//错误
			TmpDataBufList[listindex++] = response.dar;
		}
	}
	doReponse(SET_RESPONSE,SET_THENGET_REQUEST_NORMAL_LIST,csinfo,listindex,TmpDataBufList,buf);
	//此处处理防止在设置后未上送应答帧而直接上送事件报文
//	for(i=0;i<event_oadnum;i++) {
//		Get698_event(event_oad[i],memp);
//	}
	if(Response_timetag.effect==1) {
		Get698_3118_moreoad(event_oad,event_oadnum,memp);
	}
	return 0;
}


