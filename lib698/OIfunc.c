/*
 * OIfunc.c
 *
 *  Created on: Jun 3, 2017
 *      Author: ava
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "AccessFun.h"
#include "PublicFunction.h"
#include "OIfunc.h"
#include "dlt698.h"

////////////////////////////////////////////////////////////
/*
 * 电压合格率
 */
INT8U Get_213x(OAD oad,INT8U *sourcebuf,INT8U *buf,int *len)
{
	PassRate_U passu={};

	memcpy(&passu,sourcebuf,sizeof(PassRate_U));
	*len=0;
	*len += create_struct(&buf[*len],5);
	*len += fill_double_long_unsigned(&buf[*len],passu.monitorTime);
	*len += fill_long_unsigned(&buf[*len],passu.passRate);
	*len += fill_long_unsigned(&buf[*len],passu.overRate);
	*len += fill_double_long_unsigned(&buf[*len],passu.upLimitTime);
	*len += fill_double_long_unsigned(&buf[*len],passu.downLimitTime);
	return 1;
}
/*
 * 通信流量
 */
INT8U Get_2200(OI_698 oi,INT8U *sourcebuf,INT8U *buf,int *len)
{
	Flow_tj	flow_tj={};

	memcpy(&flow_tj,sourcebuf,sizeof(flow_tj));
	*len=0;
	*len += create_struct(&buf[*len],2);
	*len += fill_double_long_unsigned(&buf[*len],flow_tj.flow.day_tj);
	*len += fill_double_long_unsigned(&buf[*len],flow_tj.flow.month_tj);
	return 1;
}
/*
 * 获取日月供电时间
 */
INT8U Get_2203(OI_698 oi,INT8U *sourcebuf,INT8U *buf,int *len)
{
	Gongdian_tj gongdian_tj={};
	memcpy(&gongdian_tj,sourcebuf,sizeof(Gongdian_tj));

	fprintf(stderr,"Get_2203 :day_gongdian=%d,month_gongdian=%d\n",gongdian_tj.gongdian.day_tj,gongdian_tj.gongdian.month_tj);
	*len=0;
	*len += create_struct(&buf[*len],2);
	*len += fill_double_long_unsigned(&buf[*len],gongdian_tj.gongdian.day_tj);
	*len += fill_double_long_unsigned(&buf[*len],gongdian_tj.gongdian.month_tj);
	return 1;
}

/*
 * 获取日月复位次数
 */
INT8U Get_2204(OI_698 oi,INT8U *sourcebuf,INT8U *buf,int *len)
{
	Reset_tj reset_tj={};
	memcpy(&reset_tj,sourcebuf,sizeof(Reset_tj));
	fprintf(stderr,"Get_2204 :reset day_tj=%d,month_tj=%d\n",reset_tj.reset.day_tj,reset_tj.reset.month_tj);
	*len=0;
	*len += create_struct(&buf[*len],2);
	*len += fill_long_unsigned(&buf[*len],reset_tj.reset.day_tj);
	*len += fill_long_unsigned(&buf[*len],reset_tj.reset.month_tj);
	return 1;
}

////////////////////////////////////////////////////////////
/*
 * 日期时间
 * */
int Get_4000(OAD oad,INT8U *data)
{
	DateTimeBCD time={};
	CLASS_4000	class_tmp={};
	int index=0;

	switch(oad.attflg )
	{
		case 2://安全模式选择
			system((const char*)"hwclock -s");
			DataTimeGet(&time);
			index += fill_date_time_s(&data[index],&time);
			break;
		case 3://校时模式
			readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4000),para_vari_save);
			index += fill_enum(&data[index],class_tmp.type);
			break;
		case 4://精准校时模式
			readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4000),para_vari_save);
			index += create_struct(&data[index],5);
			index += fill_unsigned(&data[index],class_tmp.hearbeatnum);
			index += fill_unsigned(&data[index],class_tmp.tichu_max);
			index += fill_unsigned(&data[index],class_tmp.tichu_min);
			index += fill_unsigned(&data[index],class_tmp.delay);
			index += fill_unsigned(&data[index],class_tmp.num_min);
			break;
	}
	return index;
}

////////////////////////////////////////////////////////////
/*
 * 采集档案配置单元
 * type = 1: 获取一个配置单元长度，不判断数据有效性，为了分帧计算
 *      = 0: 获取配置单元数据
 * */
int Get_6001(INT8U type,INT16U seqnum,INT8U *data)
{
	int 	index=0,ret=0;
	CLASS_6001 meter={};

	ret = readParaClass(0x6000,&meter,seqnum);
	if((ret==1)||(type==1)) {
		if(type==0) {
			if(meter.sernum==0 || meter.sernum==0xffff)  return index;
		}
		fprintf(stderr,"\n 6000 read meter %d ok",seqnum);
		index += create_struct(&data[index],4);		//属性2：struct 四个元素
		index += fill_long_unsigned(&data[index],meter.sernum);		//配置序号
		index += create_struct(&data[index],10);					//基本信息:10个元素
		index += fill_TSA(&data[index],(INT8U *)&meter.basicinfo.addr.addr[1],meter.basicinfo.addr.addr[0]);		//TSA
		index += fill_enum(&data[index],meter.basicinfo.baud);			//波特率
		index += fill_enum(&data[index],meter.basicinfo.protocol);		//规约类型

		index += create_OAD(1,&data[index],meter.basicinfo.port);		//端口
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
int Get_6013(INT8U type,INT8U taskid,INT8U *data)
{
	int 	index=0,i=0,ret=0;
	CLASS_6013 task={};

	ret = readCoverClass(0x6013,taskid,&task,sizeof(CLASS_6013),coll_para_save);
	if ((ret == 1) || (type==1)) {
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
int Get_6015(INT8U type,INT8U seqnum,INT8U *data)
{
	int 	index=0,i=0,ret=0;
	CLASS_6015 coll={};

	ret = readCoverClass(0x6015,seqnum,&coll,sizeof(CLASS_6015),coll_para_save);
	fprintf(stderr,"\n 6015 read coll ok　seqnum=%d  type=%d  ret=%d\n",seqnum,type,ret);
	if ((ret == 1) || (type==1)) {
		fprintf(stderr,"\n 6015 read coll ok　seqnum=%d  type=%d  ret=%d\n",seqnum,type,ret);
		index += create_struct(&data[index],6);		//属性2：struct 6个元素
		index += fill_unsigned(&data[index],coll.sernum);		//方案序号
		index += fill_long_unsigned(&data[index],coll.deepsize);	//存储深度
		index += create_struct(&data[index],2);		//属性2：struct 2个元素

		fprintf(stderr,"coll.cjtype = %d\n",coll.cjtype);
		index += fill_unsigned(&data[index],coll.cjtype);		//采集类型
//		data[index++] = coll.data.data[0];
		index += fill_Data(coll.data.type,&data[index],coll.data.data);		//数据
		if(coll.csds.num > MY_CSD_NUM) {
			coll.csds.num = MY_CSD_NUM;
			fprintf(stderr,"采集档案记录列选择大于限值 %d\n",coll.csds.num );
		}
		fprintf(stderr,"采集档案记录列: array=%d\n",coll.csds.num);
		index += create_array(&data[index],coll.csds.num);
		for(i=0;i<coll.csds.num;i++) {
			index += fill_CSD(1,&data[index],coll.csds.csd[i]);
		}
		index += fill_MS(1,&data[index],coll.mst);		//电能表集合MS
		index += fill_enum(&data[index],coll.savetimeflag);		//存储时标选择
	}
	return index;
}

/*
 * 事件采集方案
 * */
int Get_6017(INT8U type,INT8U seqnum,INT8U *data)
{
	int 	index=0,ret=0,i=0;
	CLASS_6017 event={};

	ret = readCoverClass(0x6017,seqnum,&event,sizeof(CLASS_6017),coll_para_save);
	fprintf(stderr,"\n 6017 read coll ok　seqnum=%d  type=%d  ret=%d\n",seqnum,type,ret);
	if ((ret == 1) || (type==1)) {
		fprintf(stderr,"\n 6017 read coll ok　seqnum=%d  type=%d  ret=%d\n",seqnum,type,ret);
		index += create_struct(&data[index],5);					//属性2：struct 5个元素
		index += fill_unsigned(&data[index],event.sernum);		//方案序号
		index += create_struct(&data[index],2);					//属性2：struct 2个元素
		index += fill_unsigned(&data[index],event.collstyle.colltype);		//采集类型
		switch(event.collstyle.colltype) {
		case 0://周期采集事件数据
		case 2://根据通知采集指定事件数据
			if(event.collstyle.roads.num>ARRAY_ROAD_NUM)	event.collstyle.roads.num = ARRAY_ROAD_NUM;
			index += create_array(&data[index],event.collstyle.roads.num);
			for(i=0;i<event.collstyle.roads.num;i++) {
				index += fill_ROAD(1,&data[index],event.collstyle.roads.road[i]);	//采集数据
			}
			break;
		case 1://NULL,根据通知采集所有事件数据
			data[index++]=0;
			break;
		}
		index += fill_MS(1,&data[index],event.ms);		//电能表集合
		index += fill_bool(&data[index],event.ifreport);		//上报标识
		index += fill_long_unsigned(&data[index],event.deepsize);		//存储深度
	}
	return index;
}
/*
 * 上报方案
 * */
int Get_601D(INT8U type,INT8U seqnum,INT8U *data)
{
	int 	index=0,i=0,ret=0;
	CLASS_601D plan={};

	ret = readCoverClass(0x601D,seqnum,&plan,sizeof(CLASS_601D),coll_para_save);
	if ((ret == 1) || (type==1)) {
		fprintf(stderr,"\n 601d read report plan ok");
		index += create_struct(&data[index],5);		//属性2：struct 5个元素
		index += fill_unsigned(&data[index],plan.reportnum);		//方案序号
		index += create_array(&data[index],plan.chann_oad.num);
		for(i=0;i<plan.chann_oad.num;i++) {
			index += create_OAD(1,&data[index],plan.chann_oad.oadarr[i]);		//上报通道
		}
		index += fill_TI(&data[index],plan.timeout);				//上报响应超时时间
		index += fill_unsigned(&data[index],plan.maxreportnum);		//最大上报次数
		index += create_struct(&data[index],2);						//上报内容：struct 2个元素
		index += fill_unsigned(&data[index],plan.reportdata.type);	//类型
		if(plan.reportdata.type==0) {			//oad
			index += create_OAD(1,&data[index],plan.reportdata.data.oad);
		}else if(plan.reportdata.type==1){		//RecordData
			index += create_struct(&data[index],3);						//上报内容：struct 3个元素
			index += create_OAD(1,&data[index],plan.reportdata.data.recorddata.oad);
			index += fill_RCSD(1,&data[index],plan.reportdata.data.recorddata.csds);
			index += fill_RSD(plan.reportdata.data.recorddata.selectType,&data[index],plan.reportdata.data.recorddata.rsd);
		}
	}
	return index;
}
/*
 * 采集任务监控单元
 * */
int Get_6035(INT8U type,INT8U taskid,INT8U *data)
{
	int 	index=0,ret=0;
	CLASS_6035	classoi={};

	ret = readCoverClass(0x6035,taskid,&classoi,sizeof(CLASS_6035),coll_para_save);
	if ((ret == 1) || (type==1)) {
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

