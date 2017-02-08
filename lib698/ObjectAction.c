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
#include "Objectdef.h"

extern void FrameTail(INT8U *buf,int index,int hcsi);
extern int FrameHead(CSINFO *csinfo,INT8U *buf);
extern INT8S (*pSendfun)(int fd,INT8U* sndbuf,INT16U sndlen);
extern int comfd;

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
	buf[index] = (oad.OI>>8) & 0xff;
	index++;
	buf[index] = oad.OI & 0xff;
	index++;
	buf[index] = oad.attflg;
	index++;
	buf[index] = oad.attrindex;
	index++;

	buf[index] = dar;
	index++;
	if(data!=NULL) {
		memcpy(&buf[index],&data,sizeof(data));
		index = index + sizeof(data);
	}
	FrameTail(buf,index,hcsi);

	if(pSendfun!=NULL)
		pSendfun(comfd,buf,index+3);
	return (index+3);
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
	dest_sumindex = getMytypeSize(dest[0]);
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
		case 0x12://long unsigned
			size = 2;
			dest[0]= source[2];
			dest[1]= source[1];
			fprintf(stderr,"\n		long %02x %02x",source[2],source[1]);
			if (dest_sumindex ==0)
				dest_sumindex = size;
			break;
		case 0x55://TSA
			size = source[1];
			memcpy(dest,&source[1],size+2);// 0 表示长度为 1字节    1表示 长度为2字节 ....  将TSA长度拷贝到地址缓存中
			if (dest_sumindex ==0)
				dest_sumindex = TSA_LEN;
			size = size + 1;
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
		case 0x16://enum
			size = 1;
			memcpy(dest,&source[1],size);
			fprintf(stderr,"\n		enum data=%d\n",dest[0]);
			if (dest_sumindex ==0)
				dest_sumindex = size;
			break;
		case 0x11://unsigned
			size=1;
			memcpy(dest,&source[1],size);
			if (dest_sumindex ==0)
				dest_sumindex = size;
			fprintf(stderr,"\n		unsigned %02x",source[1]);
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
		case 0x51://OAD
			size = 4;
			dest[0]= source[2];
			dest[1]= source[1];
			dest[2]= source[3];
			dest[3]= source[4];
			if (dest_sumindex ==0)
				dest_sumindex = size;
			break;
		case 0x09://octet-string
			size = source[1];
			memcpy(dest,&source[2],size);
			if (dest_sumindex ==0)
				dest_sumindex = OCTET_STRING_LEN;
			size = size + 1;
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
	int k=0;
	INT8U addnum = data[1];
	INT16U source_sumindex=0,source_index=0,dest_sumindex=0,dest_index=0;
	fprintf(stderr,"\nsizeof Event-fangAn=%d",sizeof(eventFangAn));
	fprintf(stderr,"\n添加个数 %d",addnum);
	for(k=0; k<addnum; k++)
	{
		memset(&eventFangAn,0,sizeof(eventFangAn));
		get_BasicUnit(&data[2]+source_sumindex,&source_index,(INT8U *)&eventFangAn.sernum,&dest_index);
		source_sumindex += source_index;
		dest_sumindex += dest_index;
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
//			ClearCjFangAn();
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

			deleteClass(0x6012,1);
			break;
		case 129://方法 129:Clear()
			fprintf(stderr,"\n清空采集任务配置表");
			clearClass(0x6012);
			break;
	}
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
			clearClass(6000);
			break;
	}
}
int doObjectAction(OAD oad,INT8U *data)
{
	INT16U oi = oad.OI;
	INT8U attr_act = oad.attflg;
	fprintf(stderr,"\n----------  oi =%04x",oi);
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
	}
	return success;	//DAR=0，成功	TODO：增加DAR各种错误判断
}
