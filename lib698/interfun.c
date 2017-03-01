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
#include "dlt698.h"

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

int fill_octet_string(INT8U *data,char *value,INT8U len)
{
	data[0] = dtoctetstring;
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
		return (sizeof(DateTimeBCD)+1);
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

///////////////////////////////////////////////////////////////////////////
int getArray(INT8U *source,INT8U *dest)		//1
{
	dest[0] = source[1];
	return 2;//source[0] 0x1 (array type)   source[1] =num
}

int getStructure(INT8U *source,INT8U *dest)		//2
{
	dest[0] = source[1];
	return 2;//source[0] 0x2 (stru type)   source[1] =num
}

int getBool(INT8U *source,INT8U *dest)		//3
{
	dest[0] = source[1];
	return 2;//source[0] 0x3 (bool type)   source[1] =value
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

/*
 * type: =1 包含类型描述字节
 * 		　=0 不包含类型描述字节
 */
int getDateTimeS(INT8U type,INT8U *source,INT8U *dest)		//0x1C
{
	if(type==1) {
		dest[1] = source[1];//年
		dest[0] = source[2];
		dest[2] = source[3];//月
		dest[3] = source[4];//日
		dest[4] = source[5];//时
		dest[5] = source[6];//分
		dest[6] = source[7];//秒
		return (sizeof(DateTimeBCD)+1);
	}else {
		dest[1] = source[0];//年
		dest[0] = source[1];
		dest[2] = source[2];//月
		dest[3] = source[3];//日
		dest[4] = source[4];//时
		dest[5] = source[5];//分
		dest[6] = source[6];//秒
		return sizeof(DateTimeBCD);
	}
}

int getOAD(INT8U type,INT8U *source,OAD *oad)		//0x51
{
	if(type==1) {
		oad->OI = source[1];
		oad->OI = (oad->OI <<8) | source[2];
		oad->attflg = source[3];
		oad->attrindex = source[4];
		return (sizeof(OAD)+1);
	}else {
		oad->OI = source[0];
		oad->OI = (oad->OI <<8) | source[1];
		oad->attflg = source[2];
		oad->attrindex = source[3];
		return (sizeof(OAD));
	}
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
	if(type==1) {
		ti->units = source[1];//单位
		ti->interval = source[2];	//long unsigned数值
		ti->interval = (ti->interval <<8) | source[3];//
		return (sizeof(TI)+1);
	}else {
		ti->units = source[0];//单位
		ti->interval = source[1];	//long unsigned数值
		ti->interval = (ti->interval <<8) | source[2];//
		return (sizeof(TI));
	}
}

/*
 * 解析选择方法类型 RSD
 */
int get_BasicRSD(INT8U *source,INT8U *dest,INT8U *type)		//0x5A
{
	INT16U source_sumindex=0,source_index=0,dest_index=0;
	int index = 0;
	INT8U tmpbuf[2];
	Selector4 select4;
	Selector6 select6;
	Selector9 select9;
	Selector10 select10;
	Selector1 select1;
	Selector2 select2;

	*type = source[0];//选择方法
	switch(*type )
	{
		case 0:
			dest[0] = 0;
			index = 1;
			break;
		case 1:
			memset(&select1,0,sizeof(select1));
			select1.oad.OI= (source[1]<<8) | source[2];
			select1.oad.attflg = source[3];
			select1.oad.attrindex = source[4];
			select1.data.type = 0xAA;
			get_BasicUnit(&source[5]+source_sumindex,&source_index,(INT8U *)&select1.data,&dest_index);
			source_sumindex += source_index;
			memcpy(dest,&select1,sizeof(select1));
			index = source_sumindex + 4 + 1;//4:oad  1:type   source_sumindex:解析data的内容长度
			fprintf(stderr,"\n index = %d    !!!!!!!!!!!\n",index);
			break;
		case 2:
			memset(&select2,0,sizeof(select2));
			select2.oad.OI= (source[1]<<8) | source[2];
			select2.oad.attflg = source[3];
			select2.oad.attrindex = source[4];
			select2.data_from.type = 0xAA;
			get_BasicUnit(&source[5]+source_sumindex,&source_index,(INT8U *)&select2.data_from,&dest_index);
			source_sumindex += source_index;
			select2.data_to.type = 0xAA;
			get_BasicUnit(&source[5]+source_sumindex,&source_index,(INT8U *)&select2.data_to,&dest_index);
			source_sumindex += source_index;
			select2.data_jiange.type = 0xAA;
			get_BasicUnit(&source[5]+source_sumindex,&source_index,(INT8U *)&select2.data_jiange,&dest_index);
			source_sumindex += source_index;
			memcpy(dest,&select2,sizeof(select2));
			index = source_sumindex + 4;
			break;
		case 3:
			break;
		case 4:
		case 5:
			index = getDateTimeS(0,&source[1],(INT8U *)&select4.collect_star);
			fprintf(stderr,"\n--- %02x %02x --",source[1+index],source[1+index+1]);
			source[index] = 0x5c;//报文中没有MS的类型字节，自己添加一个
			get_BasicUnit(&source[index],&source_index,(INT8U *)&select4.meters,&dest_index);

			source_sumindex += source_index;
			index += source_sumindex;
			memcpy(dest,&select4,sizeof(select4));
			break;
		case 6:
		case 7:
		case 8:
			index++;	//type
			index += getDateTimeS(0,&source[index],(INT8U *)&select6.collect_star);
			index += getDateTimeS(0,&source[index],(INT8U *)&select6.collect_finish);
			index += getTI(0,&source[index],&select6.ti);
			index += getMS(&source[18],&select6.meters.mstype);
			memcpy(dest,&select6,sizeof(select6));
			break;
		case 9:
			select9.recordn = source[1];
			memcpy(dest,&select9,sizeof(select9));
			index = 2;
			break;
		case 10:
			select10.recordn = source[0];
			get_BasicUnit(&source[1],&source_index,(INT8U *)&select10.meters.mstype,&dest_index);
			index = source_index + sizeof(DateTimeBCD)+ sizeof(DateTimeBCD)+ sizeof(TI);
			memcpy(dest,&select10,sizeof(select10));
			break;
	}
	return index;
}

int getMS(INT8U *source,INT8U *dest)		//0x5C
{
	INT8U choicetype=0;
	choicetype = source[0];
	switch (choicetype)
	{
		case 0:
		case 1:
			dest[0] = source[0];  //0表示 没有电表  1表示 全部电表
			fprintf(stderr,"\n		MS:Choice =%02x ",source[0]);
			return 1;
		case 2:
			break;
		case 3:
			break;
		case 4:
			break;
	}
	return 0;
}

/*
 * 解析记录列选择 RCSD
 */
int get_BasicRCSD(INT8U *source,CSD_ARRAYTYPE *csds)	//0x60
{
	INT8U oadtmp[4]={};
	int i=0,index=0,j=0;
	INT8U num=0;
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

int Get_6000(INT8U seqnum,INT8U *data)
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
