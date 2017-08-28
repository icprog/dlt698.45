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
void printProxyDoThenGet(int tsa_num,DO_Then_GET *doget)
{
	int i=0,j=0,k=0;
	fprintf(stderr,".doTsaThenGet seqofTSA=%d\n",tsa_num);
	for(i=0;i<tsa_num;i++) {
		fprintf(stderr,"	..timeout=%d\n",doget[i].timeout);
		fprintf(stderr,"	..seqofTSA=%d\n",doget[i].num);
		for(j=0;j<doget[i].num;j++) {
			printTSA(doget[i].tsa);
			fprintf(stderr,"		...set_oad=%04x_%02x%02x\n",doget[i].setoads[j].oad_set.OI,doget[i].setoads[j].oad_set.attflg,doget[i].setoads[j].oad_set.attrindex);
			fprintf(stderr,"		...len data=%d\n",doget[i].setoads[j].len);
			fprintf(stderr,"		");
			for(k=0;k<doget[i].setoads[j].len;k++) {
				fprintf(stderr,"%02x ",doget[i].setoads[j].data[k]);
			}
			fprintf(stderr,"\n");
			fprintf(stderr,"...get_oad=%04x_%02x%02x\n",doget[i].setoads[j].oad_get.OI,doget[i].setoads[j].oad_get.attflg,doget[i].setoads[j].oad_get.attrindex);
			fprintf(stderr,"...delay=%d\n",doget[i].setoads[j].dealy);
		}
	}
}

void printDataTimeS(char *pro,DateTimeBCD datetimes)
{
	fprintf(stderr,"[%s]: %04d-%02d-%02d %02d:%02d:%02d\n",pro,datetimes.year.data,datetimes.month.data,datetimes.day.data,
			datetimes.hour.data,datetimes.min.data,datetimes.sec.data);
}

void printTI(char *pro,TI ti)
{
	fprintf(stderr,"[%s]:单位(%d)-间隔值(%d)  [秒:0,分:1,时:2,日:3,月:4,年:5]\n",pro,ti.units,ti.interval);
}

void printTSA(TSA tsa)
{
	int	j=0;
	fprintf(stderr,"%d-%d-",tsa.addr[0],tsa.addr[1]);
	if(tsa.addr[0]>TSA_LEN)   fprintf(stderr,"TSA 长度[%d]超过17个字节，错误！！！\n",tsa.addr[0]);
	for(j=0;j<(tsa.addr[1]+1);j++) {
		fprintf(stderr,"%02x",tsa.addr[j+2]);
	}
	fprintf(stderr,"\n");
}

void printMS(MY_MS ms)
{
	int i=0,j=0;
	int ms_num=0;
	int	seqOfLen=0;
	int	dtlen=0;

	fprintf(stderr,"电能表集合：MS choice=%d\n",ms.mstype);
	switch(ms.mstype) {
	case 0:	fprintf(stderr,"无电能表");		break;
	case 1:	fprintf(stderr,"全部用户地址");	break;
	case 2: //一组用户类型
		ms_num = (ms.ms.userType[0]<<8) | ms.ms.userType[1];
		fprintf(stderr,"一组用户类型：个数=%d\n 值=",ms_num);
		for(j=0;j<ms_num;j++) {
			fprintf(stderr,"%d ",ms.ms.userType[j+2]);
		}
		fprintf(stderr,"\n");
		break;
	case 3:	//一组用户地址
		ms_num = (ms.ms.userAddr[0].addr[0]<<8)|ms.ms.userAddr[0].addr[1];
		fprintf(stderr,"一组用户地址：个数=%d\n ",ms_num);
		if(ms.ms.configSerial[0] > COLLCLASS_MAXNUM) fprintf(stderr,"配置序号 超过限值 %d ,error !!!!!!",COLLCLASS_MAXNUM);
		for(j=0;j<ms_num;j++) {
			for(i=0;i<(ms.ms.userAddr[j+1].addr[0]+1);i++) {
				fprintf(stderr,"%02x ",ms.ms.userAddr[j+1].addr[i]);
			}
			fprintf(stderr,"\n");
		}
		break;
	case 4://一组配置序号
		fprintf(stderr,"一组配置序号：个数=%d\n ",ms.ms.configSerial[0]);
		if(ms.ms.configSerial[0] > COLLCLASS_MAXNUM) fprintf(stderr,"配置序号 超过限值 %d ,error !!!!!!",COLLCLASS_MAXNUM);
		for(i=0;i<ms.ms.configSerial[0];i++) {
			fprintf(stderr,"%d ",ms.ms.configSerial[i+1]);
		}
		fprintf(stderr,"\n");
		break;
	case 5://一组用户类型区间
	case 6://一组用户地址区间
	case 7://一组配置序号区间
		seqOfLen = 0;
		for(i=0;i<COLLCLASS_MAXNUM;i++) {
			if(ms.ms.type[i].type!=interface) {
				dtlen = getDataTypeLen(ms.ms.type[i].begin[0]);
				if(dtlen>0) {
					seqOfLen++;
					fprintf(stderr,"Region:单位[%d](前闭后开:0,前开后闭:1,前闭后闭:2,前闭后闭:3)\n",ms.ms.type[i].type);
					fprintf(stderr,"Region:[类型：%02x]起始值 ",ms.ms.type[i].begin[0]);
					for(j=0;j<(dtlen+1);j++) {
						fprintf(stderr,"%02x ",ms.ms.type[i].begin[j]);
					}
					fprintf(stderr,"\nRegion:[类型：%02x]结束值  ",ms.ms.type[i].end[0]);
				}
				dtlen = getDataTypeLen(ms.ms.type[i].end[0]);
				if(dtlen>0) {
					for(j=0;j<(dtlen+1);j++) {
						fprintf(stderr,"%02x ",ms.ms.type[i].end[j]);
					}
				}
			}
		}
		fprintf(stderr,"\n     一组用户类型区间：个数=%d\n ",seqOfLen);
		break;
	}
}

void print_rsd(INT8U choice,RSD rsd)
{
	fprintf(stderr,"RSD:choice=%d\n",choice);
	switch(choice) {
	case 8:
		printDataTimeS("采集成功时间起始值",rsd.selec8.collect_succ_star);
		printDataTimeS("采集成功时间结束值",rsd.selec8.collect_succ_finish);
		printTI("上报响应超时时间",rsd.selec8.ti);
		printMS(rsd.selec8.meters);
		break;
	case 10:
		fprintf(stderr,"Select10为指定选取最新的 %d 条记录:\n",rsd.selec10.recordn);
		printMS(rsd.selec10.meters);
		break;
	}
}

void print_road(ROAD road)
{
	int w=0;

//	asyslog(LOG_INFO,"ROAD:%04x-%02x%02x ",road.oad.OI,road.oad.attflg,road.oad.attrindex);
	fprintf(stderr,"ROAD:%04x-%02x%02x ",road.oad.OI,road.oad.attflg,road.oad.attrindex);
	if(road.num >= ROAD_OADS_NUM) {
		fprintf(stderr,"csd overvalue 16 error\n");
		return;
	}
	for(w=0;w<road.num;w++)
	{
		//asyslog(LOG_INFO,"<关联OAD..%d>%04x-%02x%02x ",w,road.oads[w].OI,road.oads[w].attflg,road.oads[w].attrindex);
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
//			asyslog(LOG_INFO,"<%d>OAD%04x-%02x%02x ",i,csds.csd[i].csd.oad.OI,csds.csd[i].csd.oad.attflg,csds.csd[i].csd.oad.attrindex);
			fprintf(stderr,"<%d>OAD%04x-%02x%02x ",i,csds.csd[i].csd.oad.OI,csds.csd[i].csd.oad.attflg,csds.csd[i].csd.oad.attrindex);
		}else if (csds.csd[i].type==1)
		{
//			asyslog(LOG_INFO,"<%d> ",i);
			fprintf(stderr,"<%d>",i);
			print_road(csds.csd[i].csd.road);
		}
	}
}

int create_array(INT8U *data,INT8U numm)	//0x01
{
	//fprintf(stderr,"numm =%d \n",numm);
	data[0] = dtarray;
	data[1] = numm;
	return 2;
}

int create_struct(INT8U *data,INT8U numm)	//0x02
{
	data[0] = dtstructure;
	data[1] = numm;
	return 2;
}

int fill_bool(INT8U *data,INT8U value)		//0x03
{
	data[0] = dtbool;
	data[1] = value;
	return 2;
}

int fill_bit_string(INT8U *data,INT8U size,INT8U *bits)		//0x04
{
	//TODO : 默认8bit ，不符合A-XDR规范
	if(size>=0 && size<=8){
		size = 8;
		syslog(LOG_ERR,"fill_bit_string size=%d, error",size);
	}
	data[0] = dtbitstring;
	data[1] = size;
	INT8U num=size/8;
	memcpy(&data[2],bits,num);
	return 2+num;
}

int fill_double_long(INT8U *data,INT32S value)		//0x05
{
	data[0] = dtdoublelong;
	data[1] = (value & 0xFF000000) >> 24 ;
	data[2] = (value & 0x00FF0000) >> 16 ;
	data[3] = (value & 0x0000FF00) >> 8 ;
	data[4] =  value & 0x000000FF;
	return 5;
}

int fill_double_long_unsigned(INT8U *data,INT32U value)		//0x06
{
	data[0] = dtdoublelongunsigned;
	data[1] = (value & 0xFF000000) >> 24 ;
	data[2] = (value & 0x00FF0000) >> 16 ;
	data[3] = (value & 0x0000FF00) >> 8 ;
	data[4] =  value & 0x000000FF;
	return 5;
}

int fill_double_long64(INT8U *data,INT64U value)		//0x14
{
	data[0] = dtlong64;
	data[1] = (value & 0xFF00000000000000) >> 56;
	data[2] = (value & 0x00FF000000000000) >> 48;
	data[3] = (value & 0x0000FF0000000000) >> 40;
	data[4] = (value & 0x000000FF00000000 )>> 32;
	data[1] = (value & 0x00000000FF000000) >> 24;
	data[2] = (value & 0x00FF000000FF0000) >> 16;
	data[3] = (value & 0x0000FF000000FF00) >> 8;
	data[4] = value & 0x00000000000000FF;
	return 9;
}


int fill_octet_string(INT8U *data,char *value,INT8U len)	//0x09
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

int fill_visible_string(INT8U *data,char *value,INT8U len)	//0x0a
{
	data[0] = dtvisiblestring;
	data[1] = len;
	memcpy(&data[2],value,len);
	return (len+2);
}

int fill_integer(INT8U *data,INT8U value)		//0x0f
{
	data[0] = dtinteger;
	data[1] = value;
	return 2;
}

int fill_long(INT8U *data,INT16U value)		//0x10
{
	data[0] = dtlong;
	data[1] = (value & 0xff00)>>8;
	data[2] = (value & 0xff);
	return 3;
}

int fill_unsigned(INT8U *data,INT8U value)		//0x11
{
	data[0] = dtunsigned;
	data[1] = value;
	return 2;
}

int fill_long_unsigned(INT8U *data,INT16U value)		//0x12
{
	data[0] = dtlongunsigned;
	data[1] = (value & 0xFF00)>>8;
	data[2] = value & 0x00FF;
	return 3;
}
int fill_enum(INT8U *data,INT8U value)		//0x16
{
	data[0] = dtenum;
	data[1] = value;
	return 2;
}
int fill_time(INT8U *data,INT8U *value)			//0x1b
{
	data[0] = dttime;
	memcpy(&data[1],&value[0],3);
	return 4;
}

int fill_date_time_s(INT8U *data,DateTimeBCD *time)		//0x1c
{
	DateTimeBCD  init_datatimes={};
	int		index=0;
	memset(&init_datatimes,0xEE,sizeof(DateTimeBCD));
	if(memcmp(time,&init_datatimes,sizeof(DateTimeBCD))==0) {		//时间无效，上送NULL（0）
		data[index++] = 0;
		return 1;
	}else {
		data[index++] = dtdatetimes;
		data[index++] = (time->year.data>>8)&0xff;
		data[index++] = time->year.data & 0xff;
		data[index++] = time->month.data;
		data[index++] = time->day.data;
		data[index++] = time->hour.data;
		data[index++] = time->min.data;
		data[index++] = time->sec.data;
		return index;
	}
}


int  create_OAD(INT8U type,INT8U *data,OAD oad)		//0x51
{
	int index=0;
	if(type)
		data[index++] = dtoad;
	data[index++] = ( oad.OI >> 8 ) & 0xff;
	data[index++] = oad.OI & 0xff;
	data[index++] = oad.attflg;
	data[index++] = oad.attrindex;
	return index;
}
int fill_OI(INT8U *data,INT8U value)
{
	data[0] = dtoi;
	data[1] = ( value >> 8 ) & 0xff;
	data[2] = value & 0xff;
	return 3;
}
int fill_ROAD(INT8U type,INT8U *data,ROAD road)			//0x52
{
	int 	index=0,i=0;
	if(type)
		data[index++] = dtroad;
	index += create_OAD(0,&data[index],road.oad);
	if(road.num>ROAD_OADS_NUM)	road.num = ROAD_OADS_NUM;
	data[index++] = road.num;
	for(i=0;i<road.num;i++) {
		index += create_OAD(0,&data[index],road.oads[i]);
	}
	return index;
}

int fill_TI(INT8U *data,TI ti)			//0x54
{
	data[0] = dtti;
	data[1] = ti.units;
	data[2] = (ti.interval>>8)&0xff;
	data[3] = ti.interval&0xff;
	return 4;
}

int fill_TSA(INT8U *data,INT8U *value,INT8U len) 	//0x55
{
	data[0] = dttsa;
	data[1] = len;
	memcpy(&data[2],value,len);
	return (len+2);
}

int fill_RSD(INT8U choice,INT8U *data,RSD rsd)			//0x5A
{
	int 	index=0;
	data[index++] = dtrsd;
	data[index++] = choice;
	switch(choice) {
	case 10:
		fprintf(stderr,"  -----------rsd.selec10.recordn=%d\n",rsd.selec10.recordn);
		//index += fill_unsigned(&data[index],rsd.selec10.recordn);
		data[index++] = rsd.selec10.recordn;
		index += fill_MS(0,&data[index],rsd.selec10.meters);
		break;
	}
	return index;
}

int fill_CSD(INT8U type,INT8U *data,MY_CSD csd)		//0x5b
{
	int 	num=0,i=0;
	int		index=0;

	if(type==1) {		//需要填充类型描述
		data[index++] = dtcsd;
	}
	data[index++] = csd.type;
	fprintf(stderr,"csd.type = %d\n",csd.type);
	if(csd.type == 0) {	//oad
		index += create_OAD(0,&data[index],csd.csd.oad);
	}else if(csd.type == 1) {	//road
		index += create_OAD(0,&data[index],csd.csd.road.oad);
		num = csd.csd.road.num;
		data[index++] = num;
		for(i=0;i<num;i++)
		{
			index += create_OAD(0,&data[index],csd.csd.road.oads[i]);
		}
	}
	return index;
}

/*填充TS类型的seqofLen
 * */
INT8U fill_SeqofLen(int seqlen,INT8U *msbuf)
{
	INT8U index=0;
	if(seqlen <= 0xff) {
		msbuf[index++] = seqlen;
	}else {
		msbuf[index++] = 0x82;	//0x80:长度区存在，0x02:长度２个字节
		msbuf[index++] = (seqlen >>8) & 0xff;
		msbuf[index++] = seqlen & 0xff;
	}
	return index;
}

int fill_MS(INT8U type,INT8U *data,MY_MS myms)		//0x5C
{
	INT8U 	choicetype=0;
	int 	index=0,seqof_index=0;
	int		i=0;
	int		seqof_len = 0;

	if(type)
		data[index++] = dtms;
	choicetype = myms.mstype;
	data[index++] = choicetype;
	fprintf(stderr,"fill_MS type = %d \n",choicetype);

	switch (choicetype)
	{
		case 0://0表示 没有电表  1表示 全部电表   //测试过
		case 1:
			break;
		case 2://一组用户类型   //测试过
			seqof_len = (myms.ms.userType[0]<<8) + myms.ms.userType[1];
			index += fill_SeqofLen(seqof_len,&data[index]);
			for(i=0;i<seqof_len;i++) {
				data[index++] = myms.ms.userType[2+i];		//不需要类型描述
			}
			break;
		case 3://一组用户地址
			seqof_len = (myms.ms.userAddr[0].addr[0]<<8) + myms.ms.userAddr[0].addr[1];
			index += fill_SeqofLen(seqof_len,&data[index]);
			for(i=0;i<seqof_len;i++) {
				memcpy(&data[index],&myms.ms.userAddr[i+1].addr[0],myms.ms.userAddr[i+1].addr[0]);
				index += myms.ms.userAddr[i+1].addr[0];
			}
			break;
		case 4:	//一组配置序号
			seqof_len = myms.ms.configSerial[0];
			index += fill_SeqofLen(seqof_len,&data[index]);
			for(i=0;i<seqof_len;i++) {
				data[index++] = (myms.ms.configSerial[i+1]>>8) & 0xff;		//TODO:是否包含类型
				data[index++] = myms.ms.configSerial[i+1] & 0xff;
				//index += fill_long_unsigned(&data[index],myms.ms.configSerial[i+1]);
			}
			break;
		case 5:	//一组用户类型区间		//ms联合体，一起处理 ，case5 湖南测试过
		case 6:	//一组用户地址区间
		case 7:	//一组配置序号区间
			seqof_len = 0;
			seqof_index = index;	//记录seqof位置
			index++;
			for(i=0;i<COLLCLASS_MAXNUM;i++) {
				if(myms.ms.type[i].type!=interface) {
					if(myms.ms.type[i].begin[0]!=0 && myms.ms.type[i].end[0]!=0) {
						seqof_len++;
						data[index++] = myms.ms.type[i].type;
						index += fill_Data(myms.ms.type[i].begin[0],&data[index],&myms.ms.type[i].begin[1]);
						index += fill_Data(myms.ms.type[i].end[0],&data[index],&myms.ms.type[i].end[1]);
					}
				}else break;
			}
			data[seqof_index] = seqof_len;
			break;
	}
	return index;
}

int fill_RCSD(INT8U type,INT8U *data,CSD_ARRAYTYPE csds)		//0x60
{
	int 	num=0,i=0;//,k=0;
	int		index=0;

	if(type==1) {		//需要填充类型描述
		data[index++] = dtrcsd;
	}
	num = csds.num;
	DEBUG_TIME_LINE("csds.num=%d\n",csds.num);
	if(num==0) {		//OAD  RCSD=0,即sequence of数据个数=0，表示不选择，国网台体测试终端编程事件时，读取上1次编程事件记录，RCSD=0，应答帧oad不应在此处填写
//		index += create_OAD(0,&data[index],csds.csd[0].csd.oad);
	}else {				//RCSD		SEQUENCE OF CSD
//		fprintf(stderr,"RCSD num = %d\n",num);
		data[index++] = num;
		for(i=0;i<num;i++)
		{
			index += fill_CSD(0,&data[index],csds.csd[i]);
//			fprintf(stderr,"index[%d]=%d\n",i,index);
		}
	}
//	fprintf(stderr,"index=%d\n",index);
	return index;
}

int fill_Data(INT8U type,INT8U *data,INT8U *value)
{
	int	 index = 0;
	INT16U  tmpval = 0;
	switch(type) {
	case dtnull://采集当前数据	//按冻结时标采集
		data[index++] = value[0];
		break;
	case dtunsigned:			//采集上第N次
		index += fill_unsigned(data,value[0]);
		break;
	case dtlongunsigned:	//
		tmpval = (value[0]<<8) | value[1];
		index += fill_long_unsigned(data,tmpval);
		break;
	case dtti:
		data[index++] = dtti;
		memcpy(&data[index],&value[0],3);	//需测试
		index += 3;
		break;
	case dttsa:		//需测试
		index += fill_TSA(&data[index],&value[1],value[1]);
		break;
	case dtstructure:
		index += create_struct(&data[index],2);
		data[index++] = dtti;
		memcpy(&data[index],&value[0],3);		//TI
		index += 3;
		index += fill_long_unsigned(&data[index],(INT16U)value[3]);
		break;
	}
	return index;
}

///////////////////////////////////////////////////////////////////////////
int getArray(INT8U *source,INT8U *dest,INT8U *DAR)		//1
{
	if(source[0]==dtarray) {
		dest[0] = source[1];
		return 2;//source[0] 0x1 (array type)   source[1] =num
	}else{
		*DAR = type_mismatch;
		return 0;
	}
}

int getStructure(INT8U *source,INT8U *dest,INT8U *DAR)		//2
{
	if(source[0]==dtstructure) {
		if (dest!=NULL)
			dest[0] = source[1];
		return 2;//source[0] 0x2 (stru type)   source[1] =num
	}else {
		int	data_len=0;
		data_len = get_Data(source,NULL);		//错误类型，为了setnormalList查找到下一个oad位置
		*DAR=type_mismatch;
		return data_len;
	}
}

int getBool(INT8U *source,INT8U *dest,INT8U *DAR)		//3
{
	if(source[0]==dtbool) {
		dest[0] = source[1];
		return 2;//source[0] 0x3 (bool type)   source[1] =value
	}else {
		*DAR=type_mismatch;
		return 0;
	}
}

int getBitString(INT8U type,INT8U *source,INT8U *dest)   //4
{
	int  bits=0,bytes=0;
	if ((type==1 && source[0]==dtbitstring) || (type==0))
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
	if(source[0] == dtdoublelong || source[0] == dtdoublelongunsigned) {
		dest[0] = source[4];
		dest[1] = source[3];
		dest[2] = source[2];
		dest[3] = source[1];
		return 5;
	}else return 0;
}

/*
 *  type ==1 存在类型字节
 */
int getOctetstring(INT8U type,INT8U *source,INT8U *tsa,INT8U *DAR)   //9  and  0x55
{
	if ((type==1 && (source[0]==dtoctetstring || source[0]==dttsa)) || type==0)
	{
		INT8U num = source[type];//字节数
		if(num>TSA_LEN) {		//todo: 定义 OCTET_STRING_LEN也会调用该函数
			asyslog(LOG_ERR,"Octetstring 长度越限[%d]  num=%d\n",TSA_LEN,num);
			num = TSA_LEN;
		}
		memcpy(tsa, &source[type],num+1);
		return (num + type + 1);	// 1:长度字节
	}else{
		*DAR = type_mismatch;
		return 0;
	}
	return 0;
}

int getVisibleString(INT8U *source,INT8U *dest,INT8U *DAR)	//0x0A
{
	if(source[0] == dtvisiblestring) {
		int	len=VISIBLE_STRING_LEN-1;
		if(source[1]<VISIBLE_STRING_LEN) {
			len = source[1]+1;			// source[0]表示类型，source[1]表示长度，字符串长度加 长度字节本身
		}else {
			asyslog(LOG_ERR,"VisibleString (%d) over %d\n",(source[1]+1),VISIBLE_STRING_LEN);
		}
		memcpy(&dest[0],&source[1],len);
		return (len+1);			//+1:类型
	}else{
		*DAR=type_mismatch;
		return 0;
	}
}

int getUnsigned(INT8U *source,INT8U *dest,INT8U *DAR)	//0x11
{
	if(source[0] == dtunsigned || source[0] == dtinteger) {
		dest[0] = source[1];
		return 2;//source[0] 0x11(unsigned type)   source[1] =data
	}else {
		*DAR = type_mismatch;
		return 0;
	}
}

int getLongUnsigned(INT8U *source,INT8U *dest)	//0x12
{
	if(source[0] == dtlongunsigned) {
		dest[1] = source[1];
		dest[0] = source[2];
		return 3;
	}
	return 0;
}

int getEnum(INT8U type,INT8U *source,INT8U *enumvalue)	//0x16
{
	if ((type==1 && source[0]==dtenum) || (type==0))
	{
		*enumvalue = source[type];
		return (1 + type);
	}
	return 0;
}

int getTime(INT8U type,INT8U *source,INT8U *dest,INT8U *DAR) 	//0x1B
{
	if((type == 1 && source[0] == dttime) || (type == 0)) {
		if(source[type+0]>23 || source[type+1]>59 || source[type+2]>59){
			*DAR = boundry_over;
			return 0;
		}
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
int getDateTimeS(INT8U type,INT8U *source,INT8U *dest,INT8U *DAR)		//0x1C
{
	int  data_len = 0;
	if((type == 1 && source[0]==dtdatetimes) || (type == 0)) {
		*DAR = check_date((source[type+0]<<8)+source[type+1],source[type+2],source[type+3],source[type+4],source[type+5],source[type+6]);
//		fprintf(stderr,"DAR=%d getDateTimeS  %02x_%02x_%02x_%02x_%02x_%02x_%02x\n",*DAR,source[type+0],source[type+1],source[type+2],source[type+3],
//		                                                                     source[type+4],source[type+5],source[type+6]);
		dest[1] = source[type+0];//年
		dest[0] = source[type+1];
		dest[2] = source[type+2];//月
		dest[3] = source[type+3];//日
		dest[4] = source[type+4];//时
		dest[5] = source[type+5];//分
		dest[6] = source[type+6];//秒
		return (7+type);
	}
	if(type == 1 && source[0]!=dtdatetimes) {
		data_len = get_Data(source,dest);		//错误类型，为了setnormalList查找到下一个oad位置
		fprintf(stderr,"data_len = %d\n",data_len);
		*DAR = type_mismatch;
		return data_len;
	}
	return 0;
}

int getOI(INT8U type,INT8U *source,OI_698 oi)		//0x50
{
	if((type == 1 && source[0]==dtoi) || (type == 0)) {
		oi = source[type];
		oi = (oi<<8) + source[type+1];
		return (type+2);
	}
	return 0;
}

int getOAD(INT8U type,INT8U *source,OAD *oad,INT8U *DAR)		//0x51
{
	if((type == 1 && source[0]==dtoad) || (type == 0)) {
		oad->OI = source[type];
		oad->OI = (oad->OI <<8) | source[type+1];
		oad->attflg = source[type+2];
		oad->attrindex = source[type+3];
		return (4+type);
	}else{
		if(DAR!=NULL)	*DAR=type_mismatch;
		return 0;
	}
	return 0;
}

int getROAD(INT8U *source,ROAD *dest)		//0x52
{
	INT8U oadtmp[4]={};
	int i=0,oadnum=0,index=1;

	if(source[0]==dtroad) {
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
	return 0;
}

int getTI(INT8U type,INT8U *source,TI *ti)	//0x54
{
	if((type==1 && source[0]==dtti) || (type==0)) {
		ti->units = source[type];//单位
		ti->interval = source[type+1];	//long unsigned数值
		ti->interval = (ti->interval <<8) | source[type+2];//
		return (3+type);    //不能取sizeof(TI) 设计结构体对齐方式，返回值要处理规约数据内容
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
	RSD		rsd={};
	INT8U	DAR=success;

	int	classtype=0;
	if(type == 1) {		//有RSD类型描述
		classtype = source[index++];
		fprintf(stderr,"classtype=%02x\n",classtype);
	}
	*seletype = source[index++];//选择方法
	fprintf(stderr,"\n\n----------seletype=%d\n",*seletype);
	switch(*seletype)
	{
		case 0:
			dest[0] = 0;
			index = 1;
			break;
		case 1:
			memset(&rsd.selec1,0,sizeof(rsd.selec1));
			index += getOAD(0,&source[index],&rsd.selec1.oad,&DAR);
			index += get_Data(&source[index],&rsd.selec1.data.type);
			memcpy(dest,&rsd.selec1,sizeof(rsd.selec1));
			fprintf(stderr,"\n index = %d   select1 OI=%04x  select1.data.type=%d !!!!\n",index,rsd.selec1.oad.OI,rsd.selec1.data.type);
			break;
		case 2:
			memset(&rsd.selec2,0,sizeof(rsd.selec2));
			index += getOAD(0,&source[index],&rsd.selec2.oad,&DAR);
			index += get_Data(&source[index],&rsd.selec2.data_from.type);
			index += get_Data(&source[index],&rsd.selec2.data_to.type);
			index += get_Data(&source[index],&rsd.selec2.data_jiange.type);
			memcpy(dest,&rsd.selec2,sizeof(rsd.selec2));
			fprintf(stderr," select2 OAD=%04x-%02x-%02x\n",rsd.selec2.oad.OI,rsd.selec2.oad.attflg,rsd.selec2.oad.attrindex);
			fprintf(stderr," 起始值 type=%02x data=%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",rsd.selec2.data_from.type,
					rsd.selec2.data_from.data[0],rsd.selec2.data_from.data[1],rsd.selec2.data_from.data[2],rsd.selec2.data_from.data[3],
					rsd.selec2.data_from.data[4],rsd.selec2.data_from.data[5],rsd.selec2.data_from.data[6]);
			fprintf(stderr," 结束值 type=%02x data=%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",rsd.selec2.data_to.type,
					rsd.selec2.data_to.data[0],rsd.selec2.data_to.data[1],rsd.selec2.data_to.data[2],rsd.selec2.data_to.data[3],
					rsd.selec2.data_to.data[4],rsd.selec2.data_to.data[5],rsd.selec2.data_to.data[6]);
			fprintf(stderr," 数据间隔 type=%02x data=%02x-%02x-%02x\n",rsd.selec2.data_jiange.type,
					rsd.selec2.data_jiange.data[0],rsd.selec2.data_jiange.data[1],rsd.selec2.data_jiange.data[2]);
			break;
		case 3:
			memset(&rsd.selec3,0,sizeof(rsd.selec3));
			rsd.selec3.sel2_num = source[index++];
			if(rsd.selec3.sel2_num > SELECTOR3_NUM) {
				syslog(LOG_ERR,"设置的SEL3的个数[%d]大于限值[%d]\n",rsd.selec3.sel2_num,SELECTOR3_NUM);
				rsd.selec3.sel2_num = SELECTOR3_NUM;
			}
			fprintf(stderr,"sel2_num=%d\n",rsd.selec3.sel2_num);
			for(i=0;i<rsd.selec3.sel2_num;i++) {
				index += getOAD(0,&source[index],&rsd.selec3.selectors[i].oad,&DAR);
				index += get_Data(&source[index],&rsd.selec3.selectors[i].data_from.type);
				index += get_Data(&source[index],&rsd.selec3.selectors[i].data_to.type);
				index += get_Data(&source[index],&rsd.selec3.selectors[i].data_jiange.type);
				fprintf(stderr," select3 OAD=%04x-%02x-%02x\n",rsd.selec3.selectors[i].oad.OI,rsd.selec3.selectors[i].oad.attflg,rsd.selec3.selectors[i].oad.attrindex);
				fprintf(stderr," 起始值 type=%02x data=%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",rsd.selec3.selectors[i].data_from.type,
						rsd.selec3.selectors[i].data_from.data[0],rsd.selec3.selectors[i].data_from.data[1],rsd.selec3.selectors[i].data_from.data[2],
						rsd.selec3.selectors[i].data_from.data[3],
						rsd.selec3.selectors[i].data_from.data[4],rsd.selec3.selectors[i].data_from.data[5],rsd.selec3.selectors[i].data_from.data[6]);
				fprintf(stderr," 结束值 type=%02x data=%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",rsd.selec3.selectors[i].data_to.type,
						rsd.selec3.selectors[i].data_to.data[0],rsd.selec3.selectors[i].data_to.data[1],rsd.selec3.selectors[i].data_to.data[2],
						rsd.selec3.selectors[i].data_to.data[3],
						rsd.selec3.selectors[i].data_to.data[4],rsd.selec3.selectors[i].data_to.data[5],rsd.selec3.selectors[i].data_to.data[6]);
				fprintf(stderr," 数据间隔 type=%02x data=%02x-%02x-%02x\n",rsd.selec3.selectors[i].data_jiange.type,
						rsd.selec3.selectors[i].data_jiange.data[0],rsd.selec3.selectors[i].data_jiange.data[1],rsd.selec3.selectors[i].data_jiange.data[2]);

			}
			memcpy(dest,&rsd.selec3,sizeof(rsd.selec3));
			break;
		case 4:
		case 5:
			index += getDateTimeS(0,&source[index],(INT8U *)&rsd.selec4.collect_star,&DAR);
			fprintf(stderr,"\n--- %02x %02x --",source[1+index],source[1+index+1]);
			printDataTimeS("采集启动/存储时间",rsd.selec4.collect_star);
			index += getMS(0,&source[index],&rsd.selec4.meters);
			memcpy(dest,&rsd.selec4,sizeof(rsd.selec4));
			break;
		case 6:
		case 7:
		case 8:
//			index++;	//type
			index += getDateTimeS(0,&source[index],(INT8U *)&rsd.selec6.collect_star,&DAR);
			index += getDateTimeS(0,&source[index],(INT8U *)&rsd.selec6.collect_finish,&DAR);
			index += getTI(0,&source[index],&rsd.selec6.ti);
			index += getMS(0,&source[index],&rsd.selec6.meters);
			memcpy(dest,&rsd.selec6,sizeof(rsd.selec6));
			break;
		case 9:
			rsd.selec9.recordn = source[index];
			memcpy(dest,&rsd.selec9,sizeof(rsd.selec9));
			index = 2;
			break;
		case 10:
			rsd.selec10.recordn = source[index++];
			index += getMS(0,&source[index],&rsd.selec10.meters);
			printMS(rsd.selec10.meters);
			memcpy(dest,&rsd.selec10,sizeof(rsd.selec10));
			break;
	}
//	fprintf(stderr,"return index = %d\n",index);
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
			getOAD(0,&source[index],&csd->csd.oad,NULL);
			index = index + sizeof(OAD);
			return index;
		}else if (csd->type==1)//ROAD
		{
			getOAD(0,&source[index],&csd->csd.road.oad,NULL);
			index = index + sizeof(OAD);
			csd->csd.road.num = source[index++];
			int k=0;
			for(k=0;k<csd->csd.road.num;k++)
			{
				getOAD(0,&source[index],&csd->csd.road.oads[k],NULL);
				index = index + 4;
			}
			return index;
		}
	}
	return 0;
}

/*
 * 获取sequenceOf len,目前只考虑长度为两个字节
 * seqOfLen：　获取长度，
 * 返回：　source缓冲区的位置
 * */
INT8U getSeqofLen(INT8U *source,int *seqOfLen)
{
	INT8U 	index = 0;
	INT8U	seqofFlg = 0;

	seqofFlg = source[0];	//sequence 的长度
	if(seqofFlg & 0x80) {
		if((seqofFlg & 0x7f)==2) {		//只考虑长度两个字节，未测试
			*seqOfLen = (source[1] << 8) | source[2];
		}
		index = 3;
	}else {
		*seqOfLen = seqofFlg;
		index = 1;
	}
	return index;
}

int getMS(INT8U type,INT8U *source,MY_MS *ms)		//0x5C
{
	INT8U 	choicetype=0;
	int	  	seqlen = 0,i=0;
	int		msindex = 0;
	int 	index = type;
	if(type>1) {		//是否存在类型字节
		fprintf(stderr,"MS 类型标识不符 type=%d\n",type);
		return 0;
	}
	choicetype = source[index++];//0
	ms->mstype = choicetype;		//区分MS类型，加入一个字节
	fprintf(stderr,"MS:Choice = %d\n",choicetype);
	if(choicetype>=2 && choicetype<=7) {
		index += getSeqofLen(&source[index],&seqlen);
		if(seqlen>COLLCLASS_MAXNUM) {
			fprintf(stderr,"sequence of len=%d 大于容量 %d,无法处理！！！",seqlen,COLLCLASS_MAXNUM);
			return index;
		}
		fprintf(stderr,"seqlen=%d\n",seqlen);		//只测试了一个电表
	}
	switch (choicetype)
	{
	case 0:
	case 1:
		break;
	case 2://一组用户类型
		msindex = 0;
		ms->ms.userType[msindex++] = (seqlen>>8)&0xff;
		ms->ms.userType[msindex++] = seqlen & 0xff;
		for(i=0;i<seqlen;i++) {
			ms->ms.userType[msindex++] = source[index++];
			if(index>=(MAXSIZ_FAM-48)) {
				syslog(LOG_ERR,"MS 数据类型填充数据[%d]超限[%d],不予处理",index,MAXSIZ_FAM);
				break;
			}
		}
		break;
	case 3://一组用户地址
		msindex = 0;
		ms->ms.userAddr[msindex].addr[0] = (seqlen>>8)&0xff;
		ms->ms.userAddr[msindex].addr[1] = seqlen & 0xff;
		msindex++;
		for(i=0;i<seqlen;i++) {
			index += getOctetstring(0,&source[index],(INT8U *)&ms->ms.userAddr[msindex++].addr,NULL);
			if(index>=(MAXSIZ_FAM-48)) {
				syslog(LOG_ERR,"MS 数据类型填充数据[%d]超限[%d],不予处理",index,MAXSIZ_FAM);
				break;
			}
		}

		fprintf(stderr,"seqlen = %d TSA len=%d\n",seqlen,ms->ms.userAddr[1].addr[0]);
		for(i=0;i<(ms->ms.userAddr[1].addr[0]+1);i++) {
			fprintf(stderr,"%02x ",ms->ms.userAddr[1].addr[i]);
		}
		break;
	case 4:	//一组配置序号  	[4] 	SEQUENCE OF long-unsigned
		msindex = 0;
		ms->ms.configSerial[msindex++] = seqlen;
		for(i=0;i<seqlen;i++) {
			ms->ms.configSerial[msindex++] = (source[index]<<8)|source[index+1];
			index = index+2;
			if(index>=(MAXSIZ_FAM-48)) {
				syslog(LOG_ERR,"MS 数据类型填充数据[%d]超限[%d],不予处理",index,MAXSIZ_FAM);
				break;
			}
		}
		break;
	case 5:	//一组用户类型区间 [5] SEQUENCE OF Region //类型5:6015设置及读取湖南测试通过
	case 6: //一组用户地址区间
	case 7://一组配置序号区间
		msindex = 0;
		for(i=0;i<COLLCLASS_MAXNUM;i++) {
			ms->ms.type[i].type = interface;//初始化interface, 用来获取有效的区间长度，Region_Type type;//type = interface 无效
		}
		for(i=0;i<seqlen;i++) {
			ms->ms.type[msindex].type = source[index++];
			index += get_Data(&source[index],ms->ms.type[msindex].begin);
			index += get_Data(&source[index],ms->ms.type[msindex].end);
			msindex++;
			if(index>=(MAXSIZ_FAM-48)) {
				syslog(LOG_ERR,"MS 数据类型填充数据[%d]超限[%d],不予处理",index,MAXSIZ_FAM);
				break;
			}
		}
		break;
	}
	fprintf(stderr,"get MS index= %d\n",index);
	return index;
}

int getCOMDCB(INT8U type, INT8U* source, COMDCB* comdcb,INT8U *DAR)		//0x5F
{
	INT8U tmpDAR=success;
	if((type==1 && source[0]==dtcomdcb) || (type==0)) {
		comdcb->baud = source[type];
		comdcb->verify = source[type+1];
		comdcb->databits = source[type+2];
		comdcb->stopbits = source[type+3];
		comdcb->flow = source[type+4];
		tmpDAR = getCOMDCBValid(*comdcb);
		if(tmpDAR != success) *DAR = tmpDAR;
		return (5+type);
	}
	if(type == 1 && source[0]!=dtcomdcb) {
		int	data_len=0;
		fprintf(stderr,"source=%02x\n",source[0]);
		data_len = get_Data(source,NULL);		//错误类型，为了setnormalList查找到下一个oad位置
		fprintf(stderr,"error data_len = %d\n",data_len);
		*DAR = type_mismatch;
		return data_len;
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
	if(num>=MY_CSD_NUM) {
		fprintf(stderr,"????????????????????? rcsd 数量[%d]超过限值[%d],异常\n",num,MY_CSD_NUM);
		num = MY_CSD_NUM;
	}
	csds->num = num;
	for(i=0;i<num;i++)
	{
		csds->csd[i].type = source[index++];
//		fprintf(stderr,"type = %d\n",csds->csd[i].type);
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
		}else 	{//oad  6字节
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


/*
 * 根据数据类型返回相应的数据长度
 * */
int getDataTypeLen(int dt)
{
	switch(dt) {
	case dtnull: 			return 0;
	case dtbool: 			return 1;
	case dtdoublelong:  	return 4;
	case dtdoublelongunsigned: return 4;
	case dtinteger:			return 1;
	case dtlong:			return 2;
	case dtunsigned:		return 1;
	case dtlongunsigned: 	return 2;
	case dtlong64:			return 8;
	case dtlong64unsigned: 	return 8;
	case dtenum:		 	return 1;
	case dtfloat32:			return 4;
	case dtfloat64:			return 8;
	case dtdatetime:		return 10;
	case dtdate:			return 5;
	case dttime:			return 3;
	case dtdatetimes:		return 7;
	case dtoi:				return 2;
	case dtoad:				return 4;
	case dtomd:				return 4;
	case dtti:				return 3;
	default:
		syslog(LOG_NOTICE,"未处理数据类型");
		return -1;
	}
}

/*
 * 返回的Data数据
 * [0]:数据类型
 * [1-n]：实际数据
 * */
int get_Data(INT8U *source,INT8U *dest)
{
	int dttype=0,dtlen=0,i=0;
	int index=0;
	int	arraynum = 0;

	dttype = source[0];
	fprintf(stderr,"get_Data type=%02x\n",dttype);
	dtlen = getDataTypeLen(dttype);
	if(dtlen>=0) {
		if(dest!=NULL) {
			dest[0] = dttype;
			memcpy(&dest[1],&source[1],dtlen);
		}
		return (dtlen+1);  //+1:dttype
	}else {
		if(dttype == dtarray) {		//一致性测试 GET_11处理,为了寻找正确的RCSD，将Selector1异常数据处理结束
			index++;
//			fprintf(stderr,"array num = %d\n",source[index]);
			arraynum = source[index];
			index++;
			for(i=0;i<arraynum;i++) {
				dttype = source[index];
				dtlen = getDataTypeLen(dttype);
				index = index + dtlen + 1;
//				fprintf(stderr,"dtlen = %d  dttype=%d index=%d\n",dtlen,dttype,index);
			}
		}
		if(dest!=NULL) {
			dest[0] = 255;
		}
		fprintf(stderr,"未知数据长度 dtlen = %d\n",dtlen);
		return index;
	}
//	switch(dttype){
//	case dtunsigned:
//		dest[1] = source[1];
//		return 2;
//	case dtlongunsigned:
//		dest[1] = source[1];		//高低位
//		dest[2] = source[2];
//		return 3;
//	case dtfloat64:
//	case dtlong64:
//	case dtlong64unsigned:
//		for(i=0 ; i<8; i++)
//			dest[8-i] = source[i+1];	//dest[8] ,7 ,6 ,5, 4 ,3 ,2 ,1   dest[0]:type
//		return 8 + 1;
//	case dtenum:
//		dest[1] = source[1];
//		return 2;
//	case dtfloat32:
//		for(i=0 ; i<4; i++)
//			dest[4-i] = source[i+1];	//dest[4] ,3 ,2 ,1   dest[0]:type
//		return 4 + 1;
//	case dtdatetime:
//		dest[1] = source[1];//年
//		dest[2] = source[2];
//		dest[3] = source[3];//月
//		dest[4] = source[4];//day_of_month
//		dest[5] = source[5];//day_of_week
//		dest[6] = source[6];//时
//		dest[7] = source[7];//分
//		dest[8] = source[8];//秒
//		dest[9] = source[9];//毫秒
//		dest[10] = source[10];//
//		return 10 + 1;
//	case dtdatetimes:
//		dest[1] = source[1];//年
//		dest[2] = source[2];
//		dest[3] = source[3];//月
//		dest[4] = source[4];//日
//		dest[5] = source[5];//时
//		dest[6] = source[6];//分
//		dest[7] = source[7];//秒
//		return 7 + 1;
//		break;
//	case dtti:
//		memcpy(&dest[1],&source[1],3);
//		return 3 + 1;
//		break;
//	case dttsa:
//		i = source[1];//长度
//		memcpy(&dest[1],&source[1],i+1);
//		return i+1;
//	default:
//		fprintf(stderr,"未处理的数据类型\n");
//		break;
//	}
//	return 0;
}


/*参数文件修改，改变共享内存的标记值，通知相关进程，参数有改变
 * */
void setOIChange(OI_698 oi)
{
	switch(oi) {
	case 0x300F:	memp->oi_changed.oi300F++;	break;
	case 0x3010:	memp->oi_changed.oi3010++;	break;
	case 0x301B:	memp->oi_changed.oi301B++;	break;
	case 0x3100:  	memp->oi_changed.oi3100++; 	break;
	case 0x3101:  	memp->oi_changed.oi3101++; 	break;
	case 0x3104:	memp->oi_changed.oi3104++; 	break;
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

	case 0x4000:	memp->oi_changed.oi4000++;	break;
	case 0x4001:	memp->oi_changed.oi4001++;	break;
	case 0x4016:	memp->oi_changed.oi4016++;	break;
	case 0x4030:	memp->oi_changed.oi4030++;	break;
	case 0x4204:	memp->oi_changed.oi4204++;	break;
	case 0x4300:	memp->oi_changed.oi4300++;  break;
	case 0x4500:	memp->oi_changed.oi4500++;  break;
	case 0x4510:	memp->oi_changed.oi4510++;  break;

	case 0x6000:	memp->oi_changed.oi6000++;  break;
	case 0x6002:	memp->oi_changed.oi6002++;  break;
	case 0x6012:	memp->oi_changed.oi6012++;  break;
	case 0x6014:	memp->oi_changed.oi6014++;  break;
	case 0x6016:	memp->oi_changed.oi6016++;  break;
	case 0x6018:	memp->oi_changed.oi6018++;  break;
	case 0x601C:	memp->oi_changed.oi601C++;  break;
	case 0x601E:	memp->oi_changed.oi601E++;  break;
	case 0x6051:	memp->oi_changed.oi6051++;  break;

	case 0xf203:
		memp->oi_changed.oiF203++;
		fprintf(stderr,"memp->oi_changed.oiF203=%d\n",memp->oi_changed.oiF203);
		break;
	case 0xf101:	memp->oi_changed.oiF101++;  break;
	case 0xf209:	memp->oi_changed.oiF209++;	break;
	}
}
