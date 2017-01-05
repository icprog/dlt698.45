/*
 * ObjectAction.c
 *
 *  Created on: Nov 12, 2016
 *      Author: ava
 */

#include <string.h>
#include <stdio.h>
#include "ObjectAction.h"
#include "AccessFun.h"
#include "StdDataType.h"


void get_BasicUnit(INT8U *source,INT16U *sourceindex,INT8U *dest,INT16U *destindex)
{
	INT8U 	size=0;
	INT8U	i=0;
	INT8U	strnum = 0;
	INT16U  source_sumindex = 0,dest_sumindex=0;
	INT8U 	type = source[0];

//	fprintf(stderr,"\n\ntype = %02x  sourceindex=%d \n",type,*sourceindex);
	switch (type)
	{
		case 0x01:	//array
			strnum = source[1];
			size = 1;
			break;
		case 0x02: //struct
			strnum = source[1];
			size = 1;
			break;
		case 0x12://long unsigned
			size = 2;
			dest[0]= source[2];
			dest[1]= source[1];
			dest_sumindex = size;
			break;
		case 0x55://TSA
			size = source[1];
			memcpy(dest,&source[2],size);
			dest_sumindex = TSA_LEN;
			size = size + 1;
			break;
		case 0x16://enum
			size = 1;
			memcpy(dest,&source[1],size);
//			fprintf(stderr,"enum  dest=%d\n",dest[0]);
			dest_sumindex = size;
			break;
		case 0x11://unsigned
			size = 1;
			memcpy(dest,&source[1],size);
			dest_sumindex = size;
			break;
		case 0x51://OAD
			size = 4;
			dest[0]= source[2];
			dest[1]= source[1];
			dest[2]= source[3];
			dest[3]= source[4];
			dest_sumindex = size;
			break;
		case 0x09://octet-string
			size = source[1];
			memcpy(dest,&source[2],size);
			dest_sumindex = OCTET_STRING_LEN;
			size = size + 1;
			break;
	}
	source_sumindex = size + 1;

//	fprintf(stderr,"\nadd size=%d,source[0]=%02x ",size,source[0]);

	for(i=0;i<strnum;i++) {
//		fprintf(stderr,"\ni = %d ",i);
		get_BasicUnit(source+source_sumindex,sourceindex,dest+dest_sumindex,destindex);
		source_sumindex += *sourceindex;
		dest_sumindex += *destindex;
//		fprintf(stderr,"\n sourceindex == %d  source_sumindex = %d",*sourceindex,source_sumindex);
//		fprintf(stderr,"\n destindex == %d  dest_sumindex = %d",*destindex,dest_sumindex);
	}
	*sourceindex = source_sumindex;
	*destindex = dest_sumindex;
}

void AddBatchMeterInfo(INT8U *data)
{
	CLASS_6001 meter={};
	int k=0;
	INT8U addnum = data[1];
	INT16U source_sumindex=0,source_index=0,dest_sumindex=0,dest_index=0;
//	fprintf(stderr,"\naddnum=%d",addnum);

//	fprintf(stderr,"\nCLASS_6001 BASIC_OBJECT=%d, EXTEND_OBJECT=%d",sizeof(BASIC_OBJECT),sizeof(EXTEND_OBJECT));

	for(k=0; k<addnum; k++)
	{
		memset(&meter,0,sizeof(meter));
		get_BasicUnit(&data[2]+source_sumindex,&source_index,(INT8U *)&meter.sernum,&dest_index);
		source_sumindex += source_index;
		dest_sumindex += dest_index;
		fprintf(stderr,"\n\nAddBatchMeterInfo  index = %d ,sumindex = %d",source_index,source_sumindex);
		fprintf(stderr,"\n........meter.sernum=%d,addr=%02x%02x%02x%02x%02x%02x,baud=%d,protocol=%d",meter.sernum,
				meter.basicinfo.addr.addr[0],meter.basicinfo.addr.addr[1],meter.basicinfo.addr.addr[2],meter.basicinfo.addr.addr[3],
				meter.basicinfo.addr.addr[4],meter.basicinfo.addr.addr[5],meter.basicinfo.baud,meter.basicinfo.protocol);
		fprintf(stderr,"\n........[OAD]OI=%04x,attflg=%d,attrindex=%d\n pwd=%02x%02x%02x%02x%02x%02x,ratenum=%d,usrtype=%d,connectype=%d,ratedU=%d,rateI=%d",
				meter.basicinfo.port.OI,meter.basicinfo.port.attflg,meter.basicinfo.port.attrindex,
				meter.basicinfo.pwd[0],meter.basicinfo.pwd[1],meter.basicinfo.pwd[2],
				meter.basicinfo.pwd[3],meter.basicinfo.pwd[4],meter.basicinfo.pwd[5],
				meter.basicinfo.ratenum,meter.basicinfo.usrtype,meter.basicinfo.connectype,meter.basicinfo.ratedU,meter.basicinfo.ratedI);
		fprintf(stderr,"\n........[ext]addr=%02x%02x%02x%02x%02x%02x,asset_code=%02x%02x%02x%02x%02x%02x,pt=%d ct=%d",
				meter.extinfo.cjq_addr.addr[0],meter.extinfo.cjq_addr.addr[1],meter.extinfo.cjq_addr.addr[2],
				meter.extinfo.cjq_addr.addr[3],meter.extinfo.cjq_addr.addr[4],meter.extinfo.cjq_addr.addr[5],
				meter.extinfo.asset_code[0],meter.extinfo.asset_code[1],meter.extinfo.asset_code[2],meter.extinfo.asset_code[3],
				meter.extinfo.asset_code[4],meter.extinfo.asset_code[5],meter.extinfo.pt,meter.extinfo.ct);
		//将meter添加到记录文件
//		extern unsigned short SaveMPara(int mtype,int id,unsigned char* data,int len);
		fprintf(stderr,"\n-------------1  6001_len=%d\n",sizeof(CLASS_6001));
		SaveMPara(0,6000,(unsigned char*)&meter,sizeof(CLASS_6001));
	}
}
void AddCjiFangAnInfo(INT8U *data)
{
	CLASS_6015 fangAn={};
	int k=0;
	INT8U addnum = data[1];
	INT16U source_sumindex=0,source_index=0,dest_sumindex=0,dest_index=0;

	for(k=0; k<addnum; k++)
	{
		memset(&fangAn,0,sizeof(fangAn));
		get_BasicUnit(&data[2]+source_sumindex,&source_index,(INT8U *)&fangAn.sernum,&dest_index);
		source_sumindex += source_index;
		dest_sumindex += dest_index;
		fprintf(stderr,"\n-------------1  6001_len=%d\n",sizeof(CLASS_6001));
//		SaveMPara(0,6000,(unsigned char*)&meter,sizeof(CLASS_6001));
	}
}

void CjiFangAnInfo(INT16U attr_act,INT8U *data)
{
	switch(attr_act)
	{
		case 2:	 //属性 2(配置表)∷=array 采集档案配置单元
			break;
		case 127://方法 127:Add (array 普通采集方案)
			AddCjiFangAnInfo(data);
			break;
		case 128://方法 128:Delete(array 方案编号)
			break;
		case 129://方法 129:Clear( )
			break;
		case 130://方法 130:Set_CSD(方案编号,array CSD)
			break;
	}
}
void MeterInfo(INT16U attr_act,INT8U *data)
{
	switch(attr_act)
	{
		case 2:	 //属性 2(配置表)∷=array 采集档案配置单元
			break;
		case 127://方法 127:Add (采集档案配置单元)
			AddBatchMeterInfo(data);
			break;
		case 128://方法 128:AddBatch(array 采集档案配置单元)
			AddBatchMeterInfo(data);
			break;
		case 129://方法 129:Update(配置序号,基本信息)
			break;
		case 130://方法 130:Update(配置序号,扩展信息,附属信息)
			break;
		case 131://方法 131:Delete(配置序号)
			break;
		case 132://方法 132:Delete(基本信息)
			break;
		case 133://方法 133:Delete(通信地址, 端口号)
			break;
		case 134://方法 134:Clear()
			break;
	}
}
int doObjectAction(OMD omd,INT8U *data)
{
	INT16U oi = omd.OI;
	INT8U attr_act = omd.method_tag;
	fprintf(stderr,"\n----------  oi =%04x",oi);
	switch(oi)
	{
		case 0x6000://采集档案配置表
			MeterInfo(attr_act,data);
			break;
		case 0x6002://搜表
			break;
		case 0x6012://任务配置表
			break;
		case 0x6014://普通采集方案集
			CjiFangAnInfo(attr_act,data);
	}
	return 1;
}
