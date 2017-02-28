/*
 * ObjectAction.c
 *
 *  Created on: Nov 12, 2016
 *      Author: ava
 */

#include <string.h>
#include <stdio.h>
#include "ParaDef.h"
#include "AccessFun.h"
#include "StdDataType.h"
#include "dlt698def.h"
#include "dlt698.h"
#include "Objectdef.h"
#include "event.h"
#include "secure.h"
#include "basedef.h"

//4:fzhs
//3: liu
void get_BasicUnit(INT8U *source,INT16U *sourceindex,INT8U *dest,INT16U *destindex);
extern INT8U Reset_add();
extern void FrameTail(INT8U *buf,int index,int hcsi);
extern int FrameHead(CSINFO *csinfo,INT8U *buf);
extern INT8S (*pSendfun)(int fd,INT8U* sndbuf,INT16U sndlen);
extern int comfd;
extern ProgramInfo *memp;


INT16U getMytypeSize(INT8U first )
{
	if (first == 0xAA)
	{
		return (sizeof(DATA_TYPE));
	}
	if (first == 0x55)
	{
		return (sizeof(CSD_ARRAYTYPE));
	}
	if( first == 0xCC)
	{
		return (12);
	}
	if( first == 0x22)
	{
		return (sizeof(MASTER_STATION_INFO_LIST));
	}
	if( first == 0x88)
	{
		return (sizeof(TSA_ARRAYTYPE));
	}
	if ( first == 0x77)
		return (sizeof(ARRAY_ROAD));
	return 0 ;
}

int doReponse(int server,int reponse,CSINFO *csinfo,PIID piid,OAD oad,int dar,INT8U *data,INT8U *buf)
{
	int index=0, hcsi=0;

	csinfo->dir = 1;
	csinfo->prm = 0;

	index = FrameHead(csinfo,buf);
	hcsi = index;
	index = index + 2;
	buf[index] = server;
	index++;
	buf[index] = reponse;
	index++;
//	fprintf(stderr,"piid.data[%d]=%02x\n",index,piid.data);
	buf[index] = piid.data;
	index++;
	index += create_OAD(&buf[index],oad);
	buf[index] = dar;
	index++;
	if(data!=NULL) {
		memcpy(&buf[index],&data,sizeof(data));
		index = index + sizeof(data);
	}
	buf[index++] = 0;	//跟随上报信息域 	FollowReport
	buf[index++] = 0;	//时间标签		TimeTag
	FrameTail(buf,index,hcsi);

	if(pSendfun!=NULL)
		pSendfun(comfd,buf,index+3);
	return (index+3);
}
int getArrayNum(INT8U *source,INT8U *dest)
{
	dest[0] = source[1];
	return 2;//source[0] 0x1 (array type)   source[1] =num
}
int getUnsigned(INT8U *source,INT8U *dest)
{
	dest[0] = source[1];
	return 2;//source[0] 0x11(unsigned type)   source[1] =data
}
int getROAD(INT8U *source,ROAD *dest)
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
int getTI(INT8U *source,INT8U *dest)
{
	dest[0] = source[0];//单位
	dest[2] = source[1];//long unsigned数值
	dest[1] = source[2];//
	return 3;
}
int getBool(INT8U *source,INT8U *dest)
{
	dest[0] = source[1];
	return 2;//source[0] 0x1 (bool type)   source[1] =value
}

int getLongUnsigned(INT8U *source,INT8U *dest)
{
	dest[1] = source[1];
	dest[0] = source[2];
	return 3;
}
int getDateTimeBCD(INT8U *source,INT8U *dest)
{
	dest[1] = source[0];//年
	dest[0] = source[1];
	dest[2] = source[2];//月
	dest[3] = source[3];//日
	dest[4] = source[4];//时
	dest[5] = source[5];//分
	dest[6] = source[6];//秒
	return sizeof(DateTimeBCD);
}
int getMS(INT8U *source,INT8U *dest)
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
 * 解析选择方法类型 RSD
 */
int get_BasicRSD(INT8U *source,INT8U *dest,INT8U *type)
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
			index = getDateTimeBCD(&source[1],(INT8U *)&select4.collect_star);
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
			index += getDateTimeBCD(&source[index],(INT8U *)&select6.collect_star);
			index += getDateTimeBCD(&source[index],(INT8U *)&select6.collect_finish);
			index += getTI(&source[index],(INT8U *)&select6.ti);
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
/*
 * 解析记录列选择 RCSD
 */
int get_BasicRCSD(INT8U *source,CSD_ARRAYTYPE *csds)
{
	INT8U oadtmp[4];
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

void get_BasicUnit(INT8U *source,INT16U *sourceindex,INT8U *dest,INT16U *destindex)
{
	INT8U choicetype;
	INT8U 	size=0;
	INT8U	i=0;
	INT8U	strnum = 0;
	INT16U  source_sumindex = 0,dest_sumindex=0,csdsize=0;

	INT8U 	type = source[0];

	fprintf(stderr,"\ntype = %02x  sourceindex=%d ",type,*sourceindex);
	A_FPRINTF("dest[0] :%02X\n", dest[0]);
	dest_sumindex = getMytypeSize(dest[0]);
	A_FPRINTF("dest[0] :%02X\n", dest[0]);
	if (dest_sumindex>0)
	{
		dest[0] = type;
		dest = dest + 1;
		fprintf(stderr,"\n遇到变长结构体 目标地址跳转 %d 字节",dest_sumindex);
	}
	switch (type)
	{
		case 0x00:
			dest[0] = 0;//Data类型 0x00为NULL
			break;
		case 0x01:	//array
			strnum = source[1];
			dest[0] = strnum;		//数组类型第一个字节为长度
			dest = dest + 1;
			fprintf(stderr,"\n数组个数-%d",strnum);
			size = 1;
			if (dest_sumindex>0)
			{
				csdsize = dest_sumindex;//csdsize 保存特殊类型数组尺寸
				dest_sumindex = 0;
			}
			break;
		case 0x02: //struct
			strnum = source[1];
			fprintf(stderr,"\n		结构体 %d  元素",strnum);
			size = 1;
			break;
		case 0x03: //bool
			dest[0] = source[1];
			fprintf(stderr,"\n		bool %d  元素",source[1]);
			size = 1;
			if (dest_sumindex ==0)
				dest_sumindex = 1;
			break;
		case 0x04: //bit-string
			size = 2;
			dest[0] = source[2];  // TODO: 此处默认8个bit   source[1] : 长度字节
			fprintf(stderr,"\n		bit-string %d ",source[2]);
			if (dest_sumindex ==0)
				dest_sumindex = 1;
			break;
		case 0x06: //double-long-unsigned
			size = 4;
			dest[0] = source[4];
			dest[1] = source[3];
			dest[2] = source[2];
			dest[3] = source[1];
			if (dest_sumindex ==0)
				dest_sumindex = size;
			break;
		case 0x09://octet-string
			size = source[1];
			memcpy(dest,&source[1],size+1);// memcpy(dest,&source[2],size); 第一个字节放置实际数据长度
			if (dest_sumindex ==0)
				dest_sumindex = OCTET_STRING_LEN;
			size = size + 1;
			break;
		case 0x0a:	//visible-string
			size = source[1];// 长度
			memcpy(dest,&source[1],size+1);
			if (dest_sumindex ==0)
				dest_sumindex = VISIBLE_STRING_LEN;
			size += 1;//加1 ：    1长度
			break;
		case 0x11://unsigned
			size=1;
			memcpy(dest,&source[1],size);
			if (dest_sumindex ==0)
				dest_sumindex = size;
			fprintf(stderr,"\n		unsigned %02x",source[1]);
			break;
		case 0x12://long unsigned
			size = 2;
			dest[0]= source[2];
			dest[1]= source[1];
			fprintf(stderr,"\n		long %02x %02x",source[2],source[1]);
			if (dest_sumindex ==0)
				dest_sumindex = size;
			break;
		case 0x16://enum
			size = 1;
			memcpy(dest,&source[1],size);
			fprintf(stderr,"\n		enum data=%d\n",dest[0]);
			if (dest_sumindex ==0)
				dest_sumindex = size;
			break;
		case 0x1b://time
			size = 3;
			dest[0] = source[1];
			dest[1] = source[2];
			dest[2] = source[3];
			fprintf(stderr,"\n		time %02x %02x %02x ",dest[0],dest[1],dest[2]);
			if (dest_sumindex ==0)
				dest_sumindex = size;
			break;
		case 0x1c://DateTimeBCD
			dest[1] = source[1];//年
			dest[0] = source[2];
			dest[2] = source[3];//月
			dest[3] = source[4];//日
			dest[4] = source[5];//时
			dest[5] = source[6];//分
			dest[6] = source[7];//秒
			size  = 7;
			if (dest_sumindex ==0)
				dest_sumindex = size;
			break;
		case 0x55://TSA
			size = source[1];
			memcpy(dest,&source[1],size+2);// 0 表示长度为 1字节    1表示 长度为2字节 ....  将TSA长度拷贝到地址缓存中
			if (dest_sumindex ==0)
				dest_sumindex = TSA_LEN;
			size = size + 1;
			fprintf(stderr,"TSA %d %02x %02x %02x %02x %02x %02x\n",dest[0],dest[1],dest[2],dest[3],dest[4],dest[5],dest[6]);
			break;
		case 0x5c://MS
			size = 1;
			choicetype = source[1];
			switch (choicetype)
			{
				case 0:
				case 1:
					dest[0] = source[1];  //0表示 没有电表  1表示 全部电表
					fprintf(stderr,"\n		MS:Choice =%02x ",source[1]);
					size = 1;
					break;
				case 2:
					break;
				case 3:
					break;
				case 4:
					break;
			}
			if (dest_sumindex ==0)
				dest_sumindex = sizeof(MY_MS);
			fprintf(stderr,"\n		目标地址跳转 %d 字节 ",dest_sumindex);
			break;
		case 0x51://OAD
			size = 4;
			dest[0]= source[2];
			dest[1]= source[1];
			dest[2]= source[3];
			dest[3]= source[4];
			if (dest_sumindex ==0)
				dest_sumindex = size;
			break;
		case 0x54://TI
			dest[0] = source[1];//单位
			dest[2] = source[2];//long unsigned数值
			dest[1] = source[3];
			size = 3;
			if (dest_sumindex ==0)
				dest_sumindex = 3;
			break;
    	case 0x5B://CSD
			choicetype = source[1];//01
			if (choicetype == 1)
			{//road
				dest[0] = choicetype;
				dest[1] = source[3];
				dest[2] = source[2];
				dest[3] = source[4];
				dest[4] = source[5];
				int numm = source[6];//SEQUENCE 0F OAD 数量
				dest[5] = (INT8U)numm;
				fprintf(stderr,"\nnumm=%d",numm);
				for(int k=0;k<numm;k++)
				{
					dest[6+k*4+0] = source[7+k*4+1];
					dest[6+k*4+1] = source[7+k*4+0];
					dest[6+k*4+2] = source[7+k*4+2];
					dest[6+k*4+3] = source[7+k*4+3];
				}
				size =1+ 4+ 1 + numm*4;// 1:choicetype  4:oad  1:num
			}else
			{//oad  6字节
				dest[0] = choicetype;
				dest[1] = source[3];
				dest[2] = source[2];
				dest[3] = source[4];
				dest[4] = source[5];
				size = 4+1;// 1： choicetype占用1个字节
				fprintf(stderr,"\n%02x %02x %02x %02x ",dest[1],dest[2],dest[3],dest[4]);
			}
			if (dest_sumindex ==0)
				dest_sumindex = sizeof(MY_CSD);
			break;
	}
	source_sumindex = size + 1;// 1：类型占用一个字节
	fprintf(stderr,"\n源缓冲区跳 %d字节 ",source_sumindex);

	for(i=0;i<strnum;i++)
	{
		fprintf(stderr,"\n----------i=%d  dest 向前移动 %d",i,dest_sumindex);
		get_BasicUnit(source+source_sumindex,sourceindex,dest+dest_sumindex,destindex);
		source_sumindex += *sourceindex;
		dest_sumindex += *destindex;
	}
	if (i == strnum)
	{
		if (csdsize>0)
		{
			fprintf(stderr,"\n循环结束 csdsize=%d  dest_sumindex=%d",csdsize,dest_sumindex);
			dest_sumindex = csdsize;
		}
	}
	*sourceindex = source_sumindex;
	*destindex = dest_sumindex;
}

void AddBatchMeterInfo(INT8U *data)
{
	CLASS_6001 meter={};
	int k=0,saveflg=0;
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
		fprintf(stderr,"\n-------------1  6001_len=%d, sernum=%d\n",sizeof(CLASS_6001),meter.sernum);
		if(meter.sernum==1)
			memcpy(meter.name,"1111111111111111",sizeof(meter.name));
		else  if(meter.sernum==2) memcpy(meter.name,"2222222222222222",sizeof(meter.name));
		fprintf(stderr,"\n-------------1  6001_len=%d, sernum=%d\n",sizeof(CLASS_6001),meter.sernum);
		saveflg = saveParaClass(0x6000,(unsigned char*)&meter,meter.sernum);
		if (saveflg==1)
			fprintf(stderr,"\n采集档案配置 %d 保存成功",meter.sernum);
		else
			fprintf(stderr,"\n采集档案配置 %d 保存失败",meter.sernum);
	}
}
void AddCjiFangAnInfo(INT8U *data)
{
	INT8U *buf;
	CLASS_6015 fangAn={};
	int k=0,saveflg=0;
	INT8U addnum = data[1];
	INT16U source_sumindex=0,source_index=0,dest_sumindex=0,dest_index=0;
	fprintf(stderr,"\nsizeof fangAn=%d",sizeof(fangAn));
	fprintf(stderr,"\n添加个数 %d",addnum);
	for(k=0; k<addnum; k++)
	{
		memset(&fangAn,0xee,sizeof(fangAn));
		fangAn.data.type = 0xAA;//标识data缓冲区
		fangAn.csds.flag = 0x55;//标识csd数组
		get_BasicUnit(&data[2]+source_sumindex,&source_index,(INT8U *)&fangAn.sernum,&dest_index);
		source_sumindex += source_index;
		dest_sumindex += dest_index;
		fprintf(stderr,"\n方案号 ：%d ",fangAn.sernum);
		fprintf(stderr,"\n存储深度 ：%d ",fangAn.deepsize);
		fprintf(stderr,"\n采集类型 ：%d ",fangAn.cjtype);
		fprintf(stderr,"\n采集内容(data) 类型：%02x  data=%d",fangAn.data.type,fangAn.data.data[0]);
		buf = (INT8U *)&fangAn.csds.flag;
		fprintf(stderr,"\ncsd:");
		INT8U type=0,w;
		for(int i=0; i<10;i++)
		{
			type = fangAn.csds.csd[i].type;
			if (type==0)
			{
				fprintf(stderr,"\nOAD");
				fprintf(stderr,"\n%04x %02x %02x",fangAn.csds.csd[i].csd.oad.OI,fangAn.csds.csd[i].csd.oad.attflg,fangAn.csds.csd[i].csd.oad.attrindex);
			}else if (type==1)
			{
				fprintf(stderr,"\nROAD");
				fprintf(stderr,"\n		OAD-%04x %02x %02x",fangAn.csds.csd[i].csd.road.oad.OI,fangAn.csds.csd[i].csd.road.oad.attflg,fangAn.csds.csd[i].csd.road.oad.attrindex);
				for(w=0;w<10;w++)
				{
					if (fangAn.csds.csd[i].csd.road.oads[w].OI!=0xeeee)
						fprintf(stderr,"\n		OAD-%04x %02x %02x",fangAn.csds.csd[i].csd.road.oads[w].OI,fangAn.csds.csd[i].csd.road.oads[w].attflg,fangAn.csds.csd[0].csd.road.oads[w].attrindex);
				}
			}
		}
		fprintf(stderr,"\n电能表集合MS ：类型 %d (0:无表   1:全部   2:一组用户   3:一组用户地址   4:一组配置序号   )",fangAn.mst.mstype);
		fprintf(stderr,"\n存储时标选择 ： %d (1:任务开始时间  2：相对当日0点0分  3:相对上日23点59分  4:相对上日0点0分  5:相对当月1日0点0分)",fangAn.savetimeflag);
		fprintf(stderr,"\n");

		saveflg = saveCoverClass(0x6015,fangAn.sernum,&fangAn,sizeof(fangAn),coll_para_save);
		if (saveflg==1)
			fprintf(stderr,"\n采集方案 %d 保存成功",fangAn.sernum);
		else
			fprintf(stderr,"\n采集方案 %d 保存失败",fangAn.sernum);
	}
}

void AddEventCjiFangAnInfo(INT8U *data)
{
	CLASS_6017 eventFangAn={};
	int i=0,k=0,saveflg=0;
	INT8U addnum = data[1];
	INT8U roadnum=0;
	int index=0;
	INT16U source_sumindex=0,source_index=0,dest_sumindex=0,dest_index=0;

	data += 4;

	fprintf(stderr,"\n添加个数 %d",addnum);
	for(k=0; k<addnum; k++)
	{
		memset(&eventFangAn,0,sizeof(eventFangAn));
		index += getUnsigned(&data[index],(INT8U *)&eventFangAn.sernum);
		index += getArrayNum(&data[index],(INT8U *)&eventFangAn.roads.num);
		for(i=0;i<eventFangAn.roads.num;i++)
			index += getROAD(&data[index],&eventFangAn.roads.road[i]);
		index += 1;//getMS没解释类型字节
		index += getMS(&data[index],&eventFangAn.ms.mstype);
		index += getBool(&data[index],&eventFangAn.ifreport);
		index += getLongUnsigned(&data[index],(INT8U *)&eventFangAn.deepsize);

		fprintf(stderr,"\n第 %d 个事件方案  ID=%d   (%d 个ROAD)",k,eventFangAn.sernum,eventFangAn.roads.num);
		int j=0,w=0;
		for(j=0;j<eventFangAn.roads.num;j++)
		{
			fprintf(stderr,"\nROAD%d",j);
			fprintf(stderr,"\n[oad %x %02x %02x]",eventFangAn.roads.road[j].oad.OI,eventFangAn.roads.road[j].oad.attflg,eventFangAn.roads.road[j].oad.attrindex);
			for(w=0;w<eventFangAn.roads.road[j].num;w++)
			{
				fprintf(stderr,"\n[%x %02x %02x]",eventFangAn.roads.road[j].oads[w].OI,eventFangAn.roads.road[j].oads[w].attflg,eventFangAn.roads.road[j].oads[w].attrindex);
			}
		}
		fprintf(stderr,"\nMStype = %d  data=%d",eventFangAn.ms.mstype,eventFangAn.ms.ms.allmeter_null);
		fprintf(stderr,"\n上报标识 = %d ",eventFangAn.ifreport);
		fprintf(stderr,"\n存储深度 = %d\n",eventFangAn.deepsize);

		saveflg = saveCoverClass(0x6017,eventFangAn.sernum,&eventFangAn,sizeof(eventFangAn),coll_para_save);
		if (saveflg==1)
			fprintf(stderr,"\n采集方案 %d 保存成功",eventFangAn.sernum);
		else
			fprintf(stderr,"\n采集方案 %d 保存失败",eventFangAn.sernum);

	}
}
void AddTaskInfo(INT8U *data)
{
	CLASS_6013 task={};
	int k=0,saveflg=0;
	INT8U addnum = data[1];
	INT16U source_sumindex=0,source_index=0,dest_sumindex=0,dest_index=0;
	fprintf(stderr,"\nsizeof task=%d",sizeof(task));
	fprintf(stderr,"\n添加个数 %d",addnum);
	for(k=0; k<addnum; k++)
	{
		memset(&task,0,sizeof(task));
		fprintf(stderr,"\n---------------------------------------进入解析\n");
		get_BasicUnit(&data[2]+source_sumindex,&source_index,(INT8U *)&task.taskID,&dest_index);
		fprintf(stderr,"\n---------------------------------------解析 第%d次\n",k);
		source_sumindex += source_index;
		dest_sumindex += dest_index;

		fprintf(stderr,"\n任务 ID=%d",task.taskID);
		fprintf(stderr,"\n执行频率 单位=%d   value=%d",task.interval.units,task.interval.interval);
		fprintf(stderr,"\n方案类型 =%d",task.cjtype);
		fprintf(stderr,"\n方案序号 =%d",task.sernum);
		fprintf(stderr,"\n开始时间 =%d年 %d月 %d日 %d时 %d分 %d秒 ",task.startime.year.data,task.startime.month.data,task.startime.day.data,task.startime.hour.data,task.startime.min.data,task.startime.sec.data);
		fprintf(stderr,"\n结束时间 =%d年 %d月 %d日 %d时 %d分 %d秒 ",task.endtime.year.data,task.endtime.month.data,task.endtime.day.data,task.endtime.hour.data,task.endtime.min.data,task.endtime.sec.data);
		fprintf(stderr,"\n优先级别 =%d",task.runprio);
		fprintf(stderr,"\n任务状态 =%d",task.state);
		fprintf(stderr,"\n运行时段类型 =%02x",task.runtime.type);
		fprintf(stderr,"\n开始  %d时 %d分  ",task.runtime.runtime[0].beginHour,task.runtime.runtime[0].beginMin);
		fprintf(stderr,"\n结束  %d时 %d分  ",task.runtime.runtime[0].endHour,task.runtime.runtime[0].endMin);

		saveflg = saveCoverClass(0x6013,task.taskID,&task,sizeof(task),coll_para_save);
		if (saveflg==1)
			fprintf(stderr,"\n采集任务 %d 保存成功",task.sernum);
		else
			fprintf(stderr,"\n采集任务 %d 保存失败",task.sernum);

	}
}
void Set_CSD(INT8U *data)
{
	CSD    csd;
	int k=0;
	INT8U num = data[1];
	INT16U source_sumindex=0,source_index=0,dest_sumindex=0,dest_index=0;

	for(k=0; k<num; k++)
	{
		memset(&csd,0,sizeof(csd));
		get_BasicUnit(&data[2]+source_sumindex,&source_index,(INT8U *)&csd,&dest_index);
		source_sumindex += source_index;
		dest_sumindex += dest_index;
//		SetCjFangAnCSD(CSD,k);
	}
}
void CjiFangAnInfo(INT16U attr_act,INT8U *data)
{
	switch(attr_act)
	{
		case 127:	//方法 127:Add (array 普通采集方案)
			fprintf(stderr,"\n添加普通采集方案");
			AddCjiFangAnInfo(data);
			break;
		case 128:	//方法 128:Delete(array 方案编号)
//			DeleteCjFangAn(data[1]);
			break;
		case 129:	//方法 129:Clear( )
			fprintf(stderr,"\n清空普通采集方案");
			clearClass(0x6015);			//普通采集方案放置在6015目录下
			break;
		case 130:	//方法 130:Set_CSD(方案编号,array CSD)
			Set_CSD(data);
			break;
	}
}
void EventCjFangAnInfo(INT16U attr_act,INT8U *data)
{
	switch(attr_act)
	{
		case 127:	//方法 127:Add(array 事件采集方案)
			fprintf(stderr,"\n添加事件采集方案");
			AddEventCjiFangAnInfo(data);
			break;
		case 128:	//方法 128:Delete(array 方案编号)
	//		DeleteEventCjFangAn(data[1]);
			break;
		case 129:	//方法 129:Clear( )
			fprintf(stderr,"\n清空事件采集方案");
			clearClass(0x6016);
			break;
		case 130:	//方法 130:Set_CSD(方案编号,array CSD)
	//		UpdateReportFlag(data);
			break;
	}
}
void TaskInfo(INT16U attr_act,INT8U *data)
{
	switch(attr_act)
	{
		case 127://方法 127:Add (任务配置单元)
			AddTaskInfo(data);
			break;
		case 128://方法 128:Delete(array任务 ID )

			deleteClass(0x6013,1);
			break;
		case 129://方法 129:Clear()
			fprintf(stderr,"\n清空采集任务配置表");
			clearClass(0x6013);		//任务配置单元存放在/nand/para/6013目录
			break;
	}
}
void TerminalInfo(INT16U attr_act,INT8U *data)
{
	switch(attr_act)
	{
		case 1://设备复位
			memp->oi_changed.reset++;
			Reset_add();
			fprintf(stderr,"\n4300 设备复位！");
			break;
		case 3://数据初始化
		case 5://事件初始化
		case 6://需量初始化
			dataInit(attr_act);
			Event_3100(NULL,0,memp);//初始化，产生事件
			fprintf(stderr,"\n终端数据初始化!");
			break;
	}
}
void FileTransMothod(INT16U attr_act,INT8U *data)
{
	INT8U name[128];
	INT8U sub_name[128];
	INT8U version[128];
	INT8U path[256];

	memset(name, 0x00, sizeof(name));
	memset(sub_name, 0x00, sizeof(sub_name));
	memset(version, 0x00, sizeof(version));
	memset(path, 0, sizeof(path));
	INT32U file_length = 0;
	INT16U block_length = 0;
	INT8U crc = 0;
	INT16U block_start = 0;

	switch(attr_act)
	{
		case 7://启动传输
			//开始解析固定的信息
			if (data[2] == 0x02 && data[3] == 0x05)
			{
				int data_index = 4;
				if(data[data_index] != 0x0a)
				{
					fprintf(stderr,"未能找到文件名\n");
					goto err;
				}
				data_index++;
				if(data[data_index] >= 128)
				{
					fprintf(stderr,"文件名过长\n");
					goto err;
				}
				memcpy(name, &data[data_index + 1], data[data_index]);
				data_index += data[data_index] + 1;

				if(data[data_index] != 0x0a)
				{
					fprintf(stderr,"未能找到文件扩展名\n");
					goto err;
				}
				data_index++;
				if(data[data_index] >= 128)
				{
					fprintf(stderr,"文件扩展名过长\n");
					goto err;
				}
				memcpy(sub_name, &data[data_index + 1], data[data_index]);
				data_index += data[data_index]+1;

				if(data[data_index] != 0x06)
				{
					fprintf(stderr,"未能找到文件长度\n");
					goto err;
				}
				data_index++;
				file_length += data[data_index++];
				file_length <<= 8;
				file_length += data[data_index++];
				file_length <<= 8;
				file_length += data[data_index++];
				file_length <<= 8;
				file_length += data[data_index++];
				data_index+=3;

				if(data[data_index] != 0x0a)
				{
					fprintf(stderr,"未能找到文件版本信息\n");
					goto err;
				}
				data_index++;
				if(data[data_index] >= 128)
				{
					fprintf(stderr,"文件版本信息过长\n");
					goto err;
				}
				memcpy(version, &data[data_index + 1], data[data_index]);
				data_index += data[data_index]+1;

				if(data[data_index] != 0x12)
				{
					fprintf(stderr,"未能找到文件传输块大小\n");
					goto err;
				}
				data_index++;
				block_length += data[data_index++];
				block_length <<= 8;
				block_length += data[data_index++];

				data_index+=2;
				if(data[data_index] != 0x16 || data[data_index+1] != 0x00)
				{
					fprintf(stderr,"无法找到文件校验方式或者文件校验方式不是CRC{%d}\n",data[data_index+1]);
					goto err;
				}
				data_index+=2;
				if(data[data_index] !=0x09)
				{
					fprintf(stderr,"无法找到文件校验的crc\n");
					goto err;
				}
				data_index++;
				crc = data[data_index];
			}
			else
			{
				goto err;
			}

			snprintf(path,sizeof(path), "/nand/UpFiles/u%s.%s.%s", name, sub_name, version);

			fprintf(stderr,"启动传输 文件名:%s,文件长度:%d,文件校验%02x,块长度%d\n", path, file_length, crc, block_length);
			createFile(path, file_length, crc, block_length);
			break;
		case 8://写文件
			if(data[0] == 0x02 && data[1] == 0x02)
			{
				int data_index = 2;
				INT16U block_index = 0;
				INT32U block_sub_len = 0;
				if(data[data_index] != 0x12)
				{
					fprintf(stderr,"未能找到分段序号\n");
					goto err;
				}
				data_index++;
				block_index += data[data_index++];
				block_index <<= 8;
				block_index += data[data_index++];

				if(data[data_index] != 0x09)
				{
					fprintf(stderr,"未能找到分段长度\n");
					goto err;
				}
				data_index++;
				if(data[data_index] > 128)
				{
					int length_len = data[data_index] - 128;
					if(length_len > 2){
						fprintf(stderr,"分片过长\n");
						goto err;
					}
					data_index++;
					for(int i = 0; i < length_len; i++)
					{
						block_sub_len+= data[data_index++];
						block_sub_len <<= 8;
					}
					block_sub_len >>= 8;
				}
				else
				{
					block_sub_len = data[data_index++];
				}
				block_start = data_index;
				fprintf(stderr,"写入文件 分段序号%d,分段长度%d\n", block_index, block_sub_len);
				appendFile(block_index, block_sub_len, &data[block_start]);
			}
			else
			{
				goto err;
			}
			break;
		case 9://读文件
			fprintf(stderr,"读取文件\n");
			break;
	}

err:
	return;
}
void MeterInfo(INT16U attr_act,INT8U *data)
{
	switch(attr_act)
	{
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
			//delClassBySeq(NULL,2);
			break;
		case 132://方法 132:Delete(基本信息)
			break;
		case 133://方法 133:Delete(通信地址, 端口号)
			break;
		case 134://方法 134:Clear()
			fprintf(stderr,"\n清空采集档案配置表");
			clearClass(0x6000);
			break;
	}
}
int EventMothod(OAD oad,INT8U *data)
{
	fprintf(stderr,"\n事件对象方法操作");
	switch(oad.attflg)
	{
		case 1://复位
			fprintf(stderr,"\n复位");
			clearClass(oad.OI);
			break;
	}
	return 0;
}
//esam 698处理函数返回0，正常，可以组上行帧。返回负数，异常，组错误帧，同意用0x16
void EsamMothod(INT16U attr_act,INT8U *data)
{
	INT32S ret=-1;
	switch(attr_act)
		{
			case 7://秘钥更新
				ret = esamMethodKeyUpdate(data);
				break;
			case 8://证书更新
			case 9://设置协商时效
				ret = esamMethodCcieSession(data);
				break;
			default:
				ret = -1;
				break;
		}
}
int doObjectAction(OAD oad,INT8U *data)
{
	INT16U oi = oad.OI;
	INT8U attr_act = oad.attflg;
	INT8U oihead = (oi & 0xF000) >>12;
	fprintf(stderr,"\n----------  oi =%04x",oi);
	switch(oihead) {
	case 3:			//事件类对象方法操作
		EventMothod(oad,data);
		break;
	}
	switch(oi)
	{
		case 0x6000:	//采集档案配置表
			MeterInfo(attr_act,data);
			break;
		case 0x6002:	//搜表
			break;
		case 0x6012:	//任务配置表
			TaskInfo(attr_act,data);
			break;
		case 0x6014:	//普通采集方案集
			CjiFangAnInfo(attr_act,data);
			break;
		case 0x6016:	//事件采集方案
			EventCjFangAnInfo(attr_act,data);
			break;
		case 0x4300:	//终端对象
			TerminalInfo(attr_act,data);
			break;
		case 0xF001: //文件传输
			FileTransMothod(attr_act,data);
			break;
		case 0xF100:
			EsamMothod(attr_act,data);
			break;
	}
	return success;	//DAR=0，成功	TODO：增加DAR各种错误判断
}
