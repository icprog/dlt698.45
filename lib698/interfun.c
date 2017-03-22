/*
 * interfun.c
 *
 *  Created on: Feb 27, 2017
 *      Author: ava
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "AccessFun.h"
#include "StdDataType.h"
#include "Objectdef.h"
#include "PublicFunction.h"
#include "dlt698.h"

extern ProgramInfo *memp;
//////////////////////////////////////////////////////////////////////
void printMS(MY_MS ms)
{
	int i=0,j=0;
	int ms_num=0;
	fprintf(stderr,"电能表集合：MS choice=%d\n",ms.mstype);
	switch(ms.mstype) {
	case 0:	fprintf(stderr,"无电能表");		break;
	case 1:	fprintf(stderr,"全部用户地址");	break;
	case 3:	//一组用户地址
		ms_num = (ms.ms.userAddr[0].addr[0]<<8)|ms.ms.userAddr[0].addr[1];
		fprintf(stderr,"一组用户地址：个数=%d\n ",ms_num);
		if(ms.ms.configSerial[0] > COLLCLASS_MAXNUM) fprintf(stderr,"配置序号 超过限值 %d ,error !!!!!!",COLLCLASS_MAXNUM);
		for(j=1;j<ms_num;j++) {
			for(i=0;i<(ms.ms.userAddr[j].addr[0]+1);i++) {
				fprintf(stderr,"%02x ",ms.ms.userAddr[j].addr[i]);
			}
		}
		break;
	case 4://一组配置序号
		fprintf(stderr,"一组配置序号：个数=%d\n ",ms.ms.configSerial[0]);
		if(ms.ms.configSerial[0] > COLLCLASS_MAXNUM) fprintf(stderr,"配置序号 超过限值 %d ,error !!!!!!",COLLCLASS_MAXNUM);
		for(i=0;i<ms.ms.configSerial[0];i++) {
			fprintf(stderr,"%d ",ms.ms.configSerial[i+1]);
		}
		break;
	}
}

void print_rsd(INT8U choice,RSD rsd)
{
	fprintf(stderr,"RSD:choice=%d\n",choice);
	switch(choice) {
	case 10:
		fprintf(stderr,"Select10为指定选取最新的 %d 条记录:\n",rsd.selec10.recordn);
		printMS(rsd.selec10.meters);
		break;
	}
}

void print_road(ROAD road)
{
	int w=0;

	asyslog(LOG_INFO,"ROAD:%04x-%02x%02x ",road.oad.OI,road.oad.attflg,road.oad.attrindex);
	fprintf(stderr,"ROAD:%04x-%02x%02x ",road.oad.OI,road.oad.attflg,road.oad.attrindex);
	if(road.num >= 16) {
		fprintf(stderr,"csd overvalue 16 error\n");
		return;
	}
	for(w=0;w<road.num;w++)
	{
		asyslog(LOG_INFO,"<关联OAD..%d>%04x-%02x%02x ",w,road.oads[w].OI,road.oads[w].attflg,road.oads[w].attrindex);
		fprintf(stderr,"<关联OAD..%d>%04x-%02x%02x ",w,road.oads[w].OI,road.oads[w].attflg,road.oads[w].attrindex);
	}
	fprintf(stderr,"\n");
}

void print_rcsd(CSD_ARRAYTYPE csds)
{
	int i=0;
	for(i=0; i<csds.num;i++)
	{
		if (csds.csd[i].type==0)
		{
			asyslog(LOG_INFO,"<%d>OAD%04x-%02x%02x ",i,csds.csd[i].csd.oad.OI,csds.csd[i].csd.oad.attflg,csds.csd[i].csd.oad.attrindex);
			fprintf(stderr,"<%d>OAD%04x-%02x%02x ",i,csds.csd[i].csd.oad.OI,csds.csd[i].csd.oad.attflg,csds.csd[i].csd.oad.attrindex);
		}else if (csds.csd[i].type==1)
		{
			asyslog(LOG_INFO,"<%d> ",i);
			fprintf(stderr,"<%d>",i);
			print_road(csds.csd[i].csd.road);
		}
	}
}

////////////////////////////////////////////////////////////////////
int getTItoSec(TI ti)
{
	int  sec = 0;
	switch(ti.units)
	{
		case 0://秒
			sec = ti.interval;
			break;
		case 1://分
			sec = ti.interval * 60;
			break;
		case 2://时
			sec =  ti.interval * 3600;
			break;
		case 3://日
			sec = ti.interval * 3600 *24;
			break;
		case 4://月
			break;
		case 5://年
			break;
		default:
			break;
	}
	fprintf(stderr,"get TI(%d-%d) sec=%d\n",ti.units,ti.interval,sec);
	return sec;
}
//////////////////////////////////////////////////////////////////////
int  create_OAD(INT8U *data,OAD oad)
{
	data[0] = ( oad.OI >> 8 ) & 0xff;
	data[1] = oad.OI & 0xff;
	data[2] = oad.attflg;
	data[3] = oad.attrindex;
	return 4;
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

int fill_bool(INT8U *data,INT8U value)
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

int fill_octet_string(INT8U *data,char *value,INT8U len)
{
//	if(len==0) {
//		data[0] = 0;
//		return 1;
//	}
	data[0] = dtoctetstring;
	if(len > OCTET_STRING_LEN)  {
		fprintf(stderr,"fill_octet_string len=%d 超过限值[%d]",len,OCTET_STRING_LEN);
		len = OCTET_STRING_LEN;
	}
	data[1] = len;
	memcpy(&data[2],value,len);
	return (len+2);
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

int fill_date_time_s(INT8U *data,DateTimeBCD *time)
{
	DateTimeBCD  init_datatimes={};

	memset(&init_datatimes,0xEE,sizeof(DateTimeBCD));
	if(memcmp(time,&init_datatimes,sizeof(DateTimeBCD))==0) {		//时间无效，上送NULL（0）
		data[0] = 0;
		return 1;
	}else {
		data[0] = dtdatetimes;
		time->year.data = time->year.data >>8 | time->year.data<<8;
		memcpy(&data[1],time,sizeof(DateTimeBCD));
		return (7+1);
	}
}

int fill_TI(INT8U *data,TI ti)
{
	data[0] = dtti;
	data[1] = ti.units;
	data[2] = (ti.interval>>8)&0xff;
	data[3] = ti.interval&0xff;
	return 4;
}

int fill_TSA(INT8U *data,INT8U *value,INT8U len)
{
	data[0] = dttsa;
	data[1] = len;
	memcpy(&data[2],value,len);
	return (len+2);
}

int fill_CSD(INT8U type,INT8U *data,MY_CSD csd)		//0x5b
{
	int 	num=0,i=0;
	int		index=0;

	if(type==1) {		//需要填充类型描述
		data[index++] = dtcsd;
	}
	data[index++] = csd.type;
	if(csd.type == 0) {	//oad
		index += create_OAD(&data[index],csd.csd.oad);
	}else if(csd.type == 1) {	//road
		index += create_OAD(&data[index],csd.csd.road.oad);
		num = csd.csd.road.num;
		data[index++] = num;
		for(i=0;i<num;i++)
		{
			index += create_OAD(&data[index],csd.csd.road.oads[i]);
		}
	}
	return index;
}

int fill_MS(INT8U *data,MY_MS myms)		//0x5C
{
	INT8U choicetype=0;

	data[0] = dtms;
	choicetype = myms.mstype;
	switch (choicetype)
	{
		case 0:
			data[1] = 0;//myms.ms.nometer_null;  //0表示 没有电表  1表示 全部电表
			return 2;
		case 1:
			data[1] = 1;//myms.ms.allmeter_null;  //0表示 没有电表  1表示 全部电表
			return 2;
		case 2:
			break;
		case 3:
			break;
		case 4:
			break;
	}
	return 0;
}

int fill_RCSD(INT8U type,INT8U *data,CSD_ARRAYTYPE csds)		//0x60
{
	int 	num=0,i=0;//,k=0;
	int		index=0;

	if(type==1) {		//需要填充类型描述
		data[index++] = dtrcsd;
	}
	num = csds.num;
	if(num==0) {		//OAD
		index += create_OAD(&data[index],csds.csd[0].csd.oad);
	}else {				//RCSD		SEQUENCE OF CSD
		data[index++] = num;
		for(i=0;i<num;i++)
		{
			index +=  fill_CSD(0,&data[index],csds.csd[i]);
//			data[index++] = csds.csd[i].type;	//第 i 个csd类型
//			fprintf(stderr,"num=%d type=%d\n",num,csds.csd[i].type);
//			fprintf(stderr,"oi=%04x_%02x_%02x\n",csds.csd[i].csd.oad.OI,csds.csd[i].csd.oad.attflg,csds.csd[i].csd.oad.attrindex);
//			if (csds.csd[i].type ==0)		//对象属性描述符 OAD
//			{
//				index += create_OAD(&data[index],csds.csd[i].csd.oad);
//			}else	{						//记录型对象属性描述符 [1] ROAD
//				index += create_OAD(&data[index],csds.csd[i].csd.road.oad);
//				for(k=0; k<csds.csd[i].csd.road.num; k++)
//				{
//					index += create_OAD(&data[index],csds.csd[i].csd.road.oads[k]);	//关联对象属性描述符  SEQUENCE OF OAD
//				}
//			}
		}
	}
	return index;
}

int fill_Data(INT8U *data,INT8U *value)
{
	INT8U type=0;
	fprintf(stderr,"value=%02x  %02x\n",value[0],value[1]);
	type = value[0];
	switch(type) {
	case dtunsigned:
		fill_unsigned(data,value[1]);
		return 2;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////
int getArray(INT8U *source,INT8U *dest)		//1
{
	dest[0] = source[1];
	return 2;//source[0] 0x1 (array type)   source[1] =num
}

int getStructure(INT8U *source,INT8U *dest)		//2
{
	if (dest!=NULL)
		dest[0] = source[1];
	return 2;//source[0] 0x2 (stru type)   source[1] =num
}

int getBool(INT8U *source,INT8U *dest)		//3
{
	dest[0] = source[1];
	return 2;//source[0] 0x3 (bool type)   source[1] =value
}

int getBitString(INT8U type,INT8U *source,INT8U *dest)   //4
{
	int  bits=0,bytes=0;
	if (type==1 || type==0)
	{
		bits = source[type];		//位串
		bytes = bits/8;
		if(bits%8) {
			bytes += 1;
		}
		memcpy(dest, &source[type+1],bytes);
		return (bytes + type + 1);		// 1:长度字节
	}
	return 0;
}

int getDouble(INT8U *source,INT8U *dest)	//5  and 6
{
	dest[0] = source[4];
	dest[1] = source[3];
	dest[2] = source[2];
	dest[3] = source[1];
	return 4;
}

/*
 *  type ==1 存在类型字节
 */
int getOctetstring(INT8U type,INT8U *source,INT8U *tsa)   //9
{
	if (type==1 || type==0)
	{
		INT8U num = source[type];//字节数
		if(num>TSA_LEN) {		//todo: 定义 OCTET_STRING_LEN也会调用该函数
			fprintf(stderr,"长度越限[%d]  num=%d\n",TSA_LEN,num);
			num = TSA_LEN;
		}
		memcpy(tsa, &source[type],num+1);
		return (num + type + 1);	// 1:长度字节
	}
	return 0;
}

int getVisibleString(INT8U *source,INT8U *dest)	//0x10
{
	int	len=VISIBLE_STRING_LEN;
	if(source[0]<VISIBLE_STRING_LEN) {
		len = source[0]+1;			// source[0]表示类型，source[1]表示长度，字符串长度加 长度字节本身
	}else fprintf(stderr,"VisibleString over %d\n",VISIBLE_STRING_LEN);
	memcpy(&dest[0],&source[1],len);
	return len+1;
}

int getUnsigned(INT8U *source,INT8U *dest)	//0x11
{
	dest[0] = source[1];
	return 2;//source[0] 0x11(unsigned type)   source[1] =data
}

int getLongUnsigned(INT8U *source,INT8U *dest)	//0x12
{
	dest[1] = source[1];
	dest[0] = source[2];
	return 3;
}

int getEnum(INT8U type,INT8U *source,INT8U *enumvalue)	//16
{
	if (type==1 || type==0)
	{
		*enumvalue = source[type];
		return (1 + type);
	}
	return 0;
}

int getTime(INT8U type,INT8U *source,INT8U *dest) 	//0x1B
{
	if((type == 1) || (type == 0)) {
		dest[0] = source[type+0];//时
		dest[1] = source[type+1];//分
		dest[2] = source[type+2];//秒
		return (3+type);
	}
	return 0;
}
/*
 * type: =1 包含类型描述字节
 * 		　=0 不包含类型描述字节
 */
int getDateTimeS(INT8U type,INT8U *source,INT8U *dest)		//0x1C
{
	if((type == 1) || (type == 0)) {
		dest[1] = source[type+0];//年
		dest[0] = source[type+1];
		dest[2] = source[type+2];//月
		dest[3] = source[type+3];//日
		dest[4] = source[type+4];//时
		dest[5] = source[type+5];//分
		dest[6] = source[type+6];//秒
		return (7+type);
	}
	return 0;
}

int getOI(INT8U type,INT8U *source,OI_698 oi)		//0x50
{
	if((type == 1) || (type == 0)) {
		oi = source[type];
		oi = (oi<<8) + source[type+1];
		return (type+2);
	}
	return 0;
}

int getOAD(INT8U type,INT8U *source,OAD *oad)		//0x51
{
	if((type == 1) || (type == 0)) {
		oad->OI = source[type];
		oad->OI = (oad->OI <<8) | source[type+1];
		oad->attflg = source[type+2];
		oad->attrindex = source[type+3];
		return (4+type);
	}
	return 0;
}

int getROAD(INT8U *source,ROAD *dest)		//0x52
{
	INT8U oadtmp[4]={};
	int i=0,oadnum=0,index=1;

	memset(oadtmp,0,4);
	oadtmp[0] = source[index+1];
	oadtmp[1] = source[index+0];
	oadtmp[2] = source[index+2];
	oadtmp[3] = source[index+3];
	memcpy(&dest->oad,oadtmp,4);//source[0] == ROAD type (0x52)
	index += 4;
	dest->num = source[index++];
	oadnum = dest->num;
	memset(oadtmp,0,4);
	for(i=0; i<oadnum;i++)
	{
		oadtmp[0] = source[index+1];
		oadtmp[1] = source[index+0];
		oadtmp[2] = source[index+2];
		oadtmp[3] = source[index+3];
		memcpy(&dest->oads[i],oadtmp,4);
		index +=4;
	}
	return index;
}

int getTI(INT8U type,INT8U *source,TI *ti)	//0x54
{
	if((type==1) || (type==0)) {
		ti->units = source[type];//单位
		ti->interval = source[type+1];	//long unsigned数值
		ti->interval = (ti->interval <<8) | source[type+2];//
		return (3+type);    //不能取sizeof(TI) 设计结构体对齐方式，返回值要处理规约数据内容
	}
	return 0;
}

int get_Data(INT8U *source,INT8U *dest)
{
	int type=0,i=0;
	type = source[0];
	dest[0] = type;
	switch(type){
	case dtunsigned:
		dest[1] = source[1];
		return 2;
	case dtlongunsigned:
		dest[1] = source[1];
		dest[2] = source[2];
		return 3;
	case dtfloat64:
	case dtlong64:
	case dtlong64unsigned:
		for(i=0 ; i<8; i++)
			dest[8-i] = source[i+1];	//dest[8] ,7 ,6 ,5, 4 ,3 ,2 ,1   dest[0]:type
		return 8 + 1;
	case dtenum:
		dest[1] = source[1];
		return 2;
	case dtfloat32:
		for(i=0 ; i<4; i++)
			dest[4-i] = source[i+1];	//dest[4] ,3 ,2 ,1   dest[0]:type
		return 4 + 1;
	case dtdatetime:
		dest[2] = source[1];//年
		dest[1] = source[2];
		dest[3] = source[3];//月
		dest[4] = source[4];//日
		dest[5] = source[5];//时
		dest[6] = source[6];//分
		dest[7] = source[7];//秒
		return 7 + 1;
	case dttsa:
		i = source[1];//长度
		memcpy(&dest[1],&source[1],i+1);
		return i+1;
	}
	return 0;
}
/*
 * 解析选择方法类型 RSD
 */
int get_BasicRSD(INT8U type,INT8U *source,INT8U *dest,INT8U *seletype)		//0x5A
{
	INT16U source_sumindex=0,source_index=0,dest_index=0;
	int index = 0,i=0;
//	INT16U tmpbuf[COLLCLASS_MAXNUM];
	Selector4 select4;
	Selector6 select6;
	Selector9 select9;
	Selector10 select10;
	Selector1 select1;
	Selector2 select2;

	int	classtype=0;
	if(type == 1) {		//有RSD类型描述
		classtype = source[index++];
		fprintf(stderr,"classtype=%02x\n",classtype);
	}
	*seletype = source[index++];//选择方法
	fprintf(stderr,"\n\n----------seletype=%d\n",*seletype);
	switch(*seletype )
	{
		case 0:
			dest[0] = 0;
			index = 1;
			break;
		case 1:
			memset(&select1,0,sizeof(select1));
			index += getOAD(0,&source[index],&select1.oad);
			index += get_Data(&source[index],&select1.data.type);
			memcpy(dest,&select1,sizeof(select1));
			fprintf(stderr,"\n index = %d   select1 OI=%04x !!!!!!!!!!!\n",index,select1.oad.OI);
			break;
		case 2:
			memset(&select2,0,sizeof(select2));
			getOAD(0,&source[index],&select2.oad);
			select2.data_from.type = 0xAA;
//			get_BasicUnit(&source[index++]+source_sumindex,&source_index,(INT8U *)&select2.data_from,&dest_index);
			source_sumindex += source_index;
			select2.data_to.type = 0xAA;
//			get_BasicUnit(&source[index++]+source_sumindex,&source_index,(INT8U *)&select2.data_to,&dest_index);
			source_sumindex += source_index;
			select2.data_jiange.type = 0xAA;
//			get_BasicUnit(&source[index++]+source_sumindex,&source_index,(INT8U *)&select2.data_jiange,&dest_index);
			source_sumindex += source_index;
			memcpy(dest,&select2,sizeof(select2));
			index += source_sumindex;// + 4;
			break;
		case 3:

			break;
		case 4:
		case 5:
			index += getDateTimeS(0,&source[index],(INT8U *)&select4.collect_star);
			fprintf(stderr,"\n--- %02x %02x --",source[1+index],source[1+index+1]);
			index += getMS(0,&source[index],&select4.meters);
			memcpy(dest,&select4,sizeof(select4));
			break;
		case 6:
		case 7:
		case 8:
//			index++;	//type
			index += getDateTimeS(0,&source[index],(INT8U *)&select6.collect_star);
			index += getDateTimeS(0,&source[index],(INT8U *)&select6.collect_finish);
			index += getTI(0,&source[index],&select6.ti);
			index += getMS(0,&source[index],&select6.meters);
			memcpy(dest,&select6,sizeof(select6));
			break;
		case 9:
			select9.recordn = source[index];
			memcpy(dest,&select9,sizeof(select9));
			index = 2;
			break;
		case 10:
			select10.recordn = source[index++];
			index += getMS(0,&source[index],&select10.meters);
			printMS(select10.meters);
			memcpy(dest,&select10,sizeof(select10));
			break;
	}
	return index;
}

int getCSD(INT8U type,INT8U *source,MY_CSD* csd)		//0X5B
{
	int index = type;//是否存在类型字节
	if (type==0 || type==1)
	{
		csd->type = source[index++];
		if (csd->type==0)//OAD
		{
			getOAD(0,&source[index],&csd->csd.oad);
			index = index + sizeof(OAD);
			return index;
		}else if (csd->type==1)//ROAD
		{
			getOAD(0,&source[index],&csd->csd.road.oad);
			index = index + sizeof(OAD);
			csd->csd.road.num = source[index++];
			int k=0;
			for(k=0;k<csd->csd.road.num;k++)
			{
				getOAD(0,&source[index],&csd->csd.road.oads[k]);
				index = index + 4;
			}
			return index;
		}
	}
	return 0;
}
int getMS(INT8U type,INT8U *source,MY_MS *ms)		//0x5C
{
	INT8U choicetype=0;
	int	  seqnum = 0,seqlen = 0,i=0;
	if(type>1) {
		fprintf(stderr,"MS 类型标识不符 type=%d\n",type);
		return 0;
	}
	choicetype = source[type];//0
	fprintf(stderr,"MS:Choice = %d\n",choicetype);
	switch (choicetype)
	{
		case 0:
		case 1:
			ms->mstype = source[type];  //0表示 没有电表  1表示 全部电表	//区分MS类型，人工加入一个字节，报文中无此说明
			ms->mstype = source[type];
			return 1+type;
		case 2:
			break;
		case 3:
			type++;
			seqlen = source[type];	//sequence 的长度
			if(seqlen & 0x80) {		//长度两个字节
				seqnum = (source[type] << 8) | source[type+1];
				type += 2;
			}else {
				seqnum = seqlen;
				type += 1;
			}
			if(seqnum>COLLCLASS_MAXNUM) {
				fprintf(stderr,"sequence of num 大于容量 %d,无法处理！！！",COLLCLASS_MAXNUM);
				return 1+type;
			}
			ms->mstype = choicetype;
			ms->ms.userAddr[0].addr[0] = (seqnum>>8)&0xff;
			ms->ms.userAddr[0].addr[1] = seqnum & 0xff;
			fprintf(stderr,"seqnum=%d\n",seqnum);		//只测试了一个电表
			for(i=0;i<seqnum;i++) {
				memcpy(&ms->ms.userAddr[i+1].addr,&source[type],(source[type]+1));
				type = type+source[type]+1;
			}
			fprintf(stderr,"TSA len=%d\n",ms->ms.userAddr[1].addr[0]);
			for(i=0;i<(ms->ms.userAddr[1].addr[0]+1);i++) {
				fprintf(stderr,"%02x ",ms->ms.userAddr[1].addr[i]);
			}
			return type;
		case 4:	//一组配置序号  	[4] 	SEQUENCE OF long-unsigned
			type++;
			seqlen = source[type];	//sequence 的长度
			if(seqlen & 0x80) {		// 只考虑长度最多两个字节情况
				seqnum = (source[type] << 8) | source[type+1];
				type += 2;
			}else {
				seqnum = seqlen;
				type += 1;
			}
			fprintf(stderr,"type=%d\n",type);
			if(seqnum>COLLCLASS_MAXNUM) {
				fprintf(stderr,"sequence of num 大于容量 %d,无法处理！！！",COLLCLASS_MAXNUM);
				return 1+type;
			}
			ms->mstype = choicetype;
			ms->ms.configSerial[0] = seqnum;
			fprintf(stderr,"choicetype=%d,seqnum=%d\n",choicetype,seqnum);
			for(i=0;i<seqnum;i++) {
				ms->ms.configSerial[i+1] = (source[type]<<8)|source[type+1];
				type = type+2;
			}
			return type;
	}
	return 0;
}

/*
 * 解析记录列选择 RCSD
 */
int get_BasicRCSD(INT8U type,INT8U *source,CSD_ARRAYTYPE *csds)	//0x60
{
	INT8U oadtmp[4]={};
	int i=0,index=0,j=0;
	INT8U num=0,classtype=0;
	if(type == 1) {		//有RCSD类型描述
		classtype = source[index++];
		fprintf(stderr,"classtype=%d\n",classtype);
	}
	num = source[index++];
	fprintf(stderr,"get RCSD num=%d\n",num);
	csds->num = num;
	for(i=0;i<num ;i++)
	{
		csds->csd[i].type = source[index++];
		if (csds->csd[i].type  == 1)
		{//road
			oadtmp[0] = source[index+1];
			oadtmp[1] = source[index+0];
			oadtmp[2] = source[index+2];
			oadtmp[3] = source[index+3];
			memcpy(&csds->csd[i].csd.road.oad,oadtmp,4);
			index = index +4;
			csds->csd[i].csd.road.num = source[index++];
			for(j=0;j<csds->csd[i].csd.road.num;j++)
			{
				oadtmp[0] = source[index+1];
				oadtmp[1] = source[index+0];
				oadtmp[2] = source[index+2];
				oadtmp[3] = source[index+3];
				index = index +4;
				memcpy(&csds->csd[i].csd.road.oads[j],oadtmp,4);
			}
		}else
		{//oad  6字节
			oadtmp[0] = source[index+1];
			oadtmp[1] = source[index+0];
			oadtmp[2] = source[index+2];
			oadtmp[3] = source[index+3];
			index = index + 4;
			memcpy(&csds->csd[i].csd.road.oad,oadtmp,4);
		}
	}
	return index;
}

////////////////////////////////////////////////////////////
/*
 * 采集档案配置单元
 * */
int Get_6001(INT8U seqnum,INT8U *data)
{
	int 	index=0;
	CLASS_6001 meter={};

	if(readParaClass(0x6000,&meter,seqnum)==1) {
		fprintf(stderr,"\n 6000 read meter ok");
		index += create_struct(&data[index],4);		//属性2：struct 四个元素
		index += fill_long_unsigned(&data[index],meter.sernum);		//配置序号
		index += create_struct(&data[index],10);					//基本信息:10个元素
		index += fill_TSA(&data[index],(INT8U *)&meter.basicinfo.addr.addr[1],meter.basicinfo.addr.addr[0]);		//TSA
		index += fill_enum(&data[index],meter.basicinfo.baud);			//波特率
		index += fill_enum(&data[index],meter.basicinfo.protocol);		//规约类型
		data[index++] = dtoad;
		index += create_OAD(&data[index],meter.basicinfo.port);		//端口
		index += fill_octet_string(&data[index],(char *)&meter.basicinfo.pwd[1],meter.basicinfo.pwd[0]);		//通信密码
		index += fill_unsigned(&data[index],meter.basicinfo.ratenum);		//费率个数
		index += fill_unsigned(&data[index],meter.basicinfo.usrtype);		//用户类型
		index += fill_enum(&data[index],meter.basicinfo.connectype);		//接线方式
		index += fill_long_unsigned(&data[index],meter.basicinfo.ratedU);		//额定电压
		index += fill_long_unsigned(&data[index],meter.basicinfo.ratedI);		//额定电流
		index += create_struct(&data[index],4);					//扩展信息:4个元素
		index += fill_TSA(&data[index],(INT8U *)&meter.extinfo.cjq_addr.addr[1],meter.extinfo.cjq_addr.addr[0]);		//TSA
		index += fill_octet_string(&data[index],(char *)&meter.extinfo.asset_code[1],meter.extinfo.asset_code[0]);	//资产号
		index += fill_long_unsigned(&data[index],meter.extinfo.pt);		//PT
		index += fill_long_unsigned(&data[index],meter.extinfo.ct);		//CT
		index += create_array(&data[index],0);					//附属信息:0个元素
	}
	return index;
}

/*
 * 任务配置单元
 * */
int Get_6013(INT8U taskid,INT8U *data)
{
	int 	index=0,i=0;
	CLASS_6013 task={};

	if (readCoverClass(0x6013,taskid,&task,sizeof(CLASS_6013),coll_para_save)) {
		fprintf(stderr,"\n 6013 read meter ok");
		index += create_struct(&data[index],12);		//属性2：struct 12个元素
		index += fill_unsigned(&data[index],task.taskID);		//配置序号
		index += fill_TI(&data[index],task.interval);			//执行频率
		index += fill_enum(&data[index],task.cjtype);			//方案类型
		index += fill_unsigned(&data[index],task.sernum);			//方案序号
		index += fill_date_time_s(&data[index],&task.startime);		//开始时间
		index += fill_date_time_s(&data[index],&task.endtime);		//结束时间
		index += fill_TI(&data[index],task.delay);				//延时
		index += fill_enum(&data[index],task.runprio);			//执行优先级
		index += fill_enum(&data[index],task.state);			//任务状态
		index += fill_long_unsigned(&data[index],task.befscript); //任务开始前脚本
		index += fill_long_unsigned(&data[index],task.aftscript); //任务完成后脚本
		index += create_struct(&data[index],2);					//任务运行时段:2个元素
		index += fill_enum(&data[index],task.runtime.type);
		index += create_array(&data[index],task.runtime.num);	  //时段表
		for(i=0;i<task.runtime.num;i++) {
			index += create_struct(&data[index],4);
			index += fill_unsigned(&data[index],task.runtime.runtime[i].beginHour);
			index += fill_unsigned(&data[index],task.runtime.runtime[i].beginMin);
			index += fill_unsigned(&data[index],task.runtime.runtime[i].endHour);
			index += fill_unsigned(&data[index],task.runtime.runtime[i].endMin);
		}
	}
	return index;
}

/*
 * 普通采集方案
 * */
int Get_6015(INT8U seqnum,INT8U *data)
{
	int 	index=0,i=0;
	CLASS_6015 coll={};

	if (readCoverClass(0x6015,seqnum,&coll,sizeof(CLASS_6015),coll_para_save)) {
		fprintf(stderr,"\n 6015 read coll ok");
		index += create_struct(&data[index],6);		//属性2：struct 6个元素
		index += fill_unsigned(&data[index],coll.sernum);		//方案序号
		index += fill_long_unsigned(&data[index],coll.deepsize);	//存储深度
		index += create_struct(&data[index],2);		//属性2：struct 2个元素

		fprintf(stderr,"coll.cjtype = %d\n",coll.cjtype);
		index += fill_unsigned(&data[index],coll.cjtype);		//采集类型
		data[index++] = coll.data.data[0];
		index += fill_Data(&data[index],&coll.data.type);		//数据
		if(coll.csds.num > MY_CSD_NUM) {
			coll.csds.num = MY_CSD_NUM;
			fprintf(stderr,"采集档案记录列选择大于限值 %d\n",coll.csds.num );
		}
		fprintf(stderr,"采集档案记录列: array=%d\n",coll.csds.num);
		index += create_array(&data[index],coll.csds.num);
		for(i=0;i<coll.csds.num;i++) {
			index += fill_CSD(1,&data[index],coll.csds.csd[i]);
		}
		index += fill_MS(&data[index],coll.mst);		//电能表集合MS
		index += fill_enum(&data[index],coll.savetimeflag);		//存储时标选择
	}
	return index;
}

/*
 * 采集任务监控单元
 * */
int Get_6035(INT8U taskid,INT8U *data)
{
	int 	index=0;
	CLASS_6035	classoi={};

	if (readCoverClass(0x6035,taskid,&classoi,sizeof(CLASS_6035),coll_para_save))
	{
		index += create_struct(&data[index],8);
		index += fill_unsigned(&data[index],classoi.taskID);
		index += fill_enum(&data[index],classoi.taskState);
		index += fill_date_time_s(&data[index],&classoi.starttime);
		index += fill_date_time_s(&data[index],&classoi.endtime);
		index += fill_long_unsigned(&data[index],classoi.totalMSNum);
		index += fill_long_unsigned(&data[index],classoi.successMSNum);
		index += fill_long_unsigned(&data[index],classoi.sendMsgNum);
		index += fill_long_unsigned(&data[index],classoi.rcvMsgNum);
	}
	return index;
}

/*参数文件修改，改变共享内存的标记值，通知相关进程，参数有改变
 * */
void setOIChange(OI_698 oi)
{
	switch(oi) {
	case 0x3100:  	memp->oi_changed.oi3100++; 	break;
	case 0x3101:  	memp->oi_changed.oi3101++; 	break;
	case 0x3014:	memp->oi_changed.oi3104++; 	break;
	case 0x3105:	memp->oi_changed.oi3105++; 	break;
	case 0x3106:	memp->oi_changed.oi3106++; 	break;
	case 0x3107:	memp->oi_changed.oi3107++; 	break;
	case 0x3108:	memp->oi_changed.oi3108++; 	break;
	case 0x3109: 	memp->oi_changed.oi3109++; 	break;
	case 0x310A:	memp->oi_changed.oi310A++; 	break;
	case 0x310B:	memp->oi_changed.oi310B++; 	break;
	case 0x310C:	memp->oi_changed.oi310C++; 	break;
	case 0x310D:	memp->oi_changed.oi310D++; 	break;
	case 0x310E:	memp->oi_changed.oi310E++; 	break;
	case 0x310F:	memp->oi_changed.oi310F++; 	break;
	case 0x3110:	memp->oi_changed.oi3110++;	break;
	case 0x3111:	memp->oi_changed.oi3111++;	break;
	case 0x3112:	memp->oi_changed.oi3112++;  break;
	case 0x3114:	memp->oi_changed.oi3114++; 	break;
	case 0x3115:	memp->oi_changed.oi3115++; 	break;
	case 0x3116:	memp->oi_changed.oi3116++;	break;
	case 0x3117:	memp->oi_changed.oi3117++;	break;
	case 0x3118:	memp->oi_changed.oi3118++;	break;
	case 0x3119:	memp->oi_changed.oi3119++;	break;
	case 0x311A:	memp->oi_changed.oi311A++;	break;
	case 0x311B:	memp->oi_changed.oi311B++;	break;
	case 0x311C:	memp->oi_changed.oi311C++;	break;
	case 0x3200:	memp->oi_changed.oi3200++;	break;
	case 0x3201:	memp->oi_changed.oi3201++;	break;
	case 0x3202:	memp->oi_changed.oi3202++;	break;
	case 0x3203:	memp->oi_changed.oi3203++;	break;
	case 0x4016:	memp->oi_changed.oi4016++;	break;
	case 0x4300:	memp->oi_changed.oi4300++;  break;
	case 0xf203:	memp->oi_changed.oiF203++;  break;
	}
}

