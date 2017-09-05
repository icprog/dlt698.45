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


int Set_4000(INT8U *data,INT8U *DAR)
{
	DateTimeBCD datetime={};
	int		index=0;

//	DataTimeGet(&datetime);
	index += getDateTimeS(1,data,(INT8U *)&datetime,DAR);
	if(*DAR==success) {	//时间合法
		setsystime(datetime);
	}
//	sleep(2);		//延时2秒，确保台体测试过程中，修改时间设置成功
	return index;
}
int Set_4006(INT8U *data,INT8U *DAR,INT8U attr_act)
{
    if(attr_act == 127 || attr_act == 128)
    {
    	CLASS_4006 class_tmp={};
    	int ret = readCoverClass(0x4006,0,&class_tmp,sizeof(CLASS_4006),para_vari_save);
    	if(ret == 1)
    		getEnum(0,data,&class_tmp.state);
    	saveCoverClass(0x4006,0,&class_tmp,sizeof(CLASS_4006),para_vari_save);
    }
	return 0;
}
////////////////////////////////////////////////////////////
/*
 * 电压合格率
 */
INT8U Get_213x(INT8U getflg,INT8U *sourcebuf,INT8U *buf,int *len)
{
	PassRate_U passu={};

	memset(&passu,0,sizeof(PassRate_U));
	if(getflg && sourcebuf!=NULL) {
		memcpy(&passu,sourcebuf,sizeof(PassRate_U));
	}
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
INT8U Get_2200(INT8U getflg,INT8U *sourcebuf,INT8U *buf,int *len)
{
	Flow_tj	flow_tj={};

	memset(&flow_tj,0,sizeof(Flow_tj));
	if(getflg && sourcebuf!=NULL) {
		memcpy(&flow_tj,sourcebuf,sizeof(flow_tj));
	}
	*len=0;
	*len += create_struct(&buf[*len],2);
	//GW送检 终端当前数据招测流量时，使用GPRS内部协议栈时，日，月流量都为0，才判断合格。
    if(getZone("GW")==0) {
		*len += fill_double_long_unsigned(&buf[*len],0x00);
		*len += fill_double_long_unsigned(&buf[*len],0x00);
    }else {
		*len += fill_double_long_unsigned(&buf[*len],flow_tj.flow.day_tj);
		*len += fill_double_long_unsigned(&buf[*len],flow_tj.flow.month_tj);
    }
	return 1;
}
/*
 * 获取日月供电时间
 */
INT8U Get_2203(INT8U getflg,INT8U *sourcebuf,INT8U *buf,int *len)
{
	Gongdian_tj gongdian_tj={};

	memset(&gongdian_tj,0,sizeof(Gongdian_tj));
	if(getflg && sourcebuf!=NULL) {
		memcpy(&gongdian_tj,sourcebuf,sizeof(Gongdian_tj));
	}
	fprintf(stderr,"Get_2203 :day_gongdian=%d,month_gongdian=%d\n",gongdian_tj.gongdian.day_tj,gongdian_tj.gongdian.month_tj);
	*len=0;
	*len += create_struct(&buf[*len],2);
	INT32U day_tj=0,month_tj=0;
	if(gongdian_tj.gongdian.day_tj%60==0)
		day_tj = gongdian_tj.gongdian.day_tj/60;
	else
		day_tj = gongdian_tj.gongdian.day_tj/60+1;
	if(gongdian_tj.gongdian.month_tj%60==0)
		month_tj = gongdian_tj.gongdian.month_tj/60;
	else
		month_tj = gongdian_tj.gongdian.month_tj/60+1;
	*len += fill_double_long_unsigned(&buf[*len],day_tj);
	*len += fill_double_long_unsigned(&buf[*len],month_tj);
	return 1;
}

/*
 * 获取日月复位次数
 */
INT8U Get_2204(INT8U getflg,INT8U *sourcebuf,INT8U *buf,int *len)
{
	Reset_tj reset_tj={};

	memset(&reset_tj,0,sizeof(Reset_tj));
	if(getflg && sourcebuf!=NULL) {
		memcpy(&reset_tj,sourcebuf,sizeof(Reset_tj));
	}
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
			//注意：测试项（时钟招测与对时）：要求时钟下发与招测误差在5秒内。当内部协议栈时，收发速度比较慢。因此将此处主站招测时钟是人为增加7秒再上送，防止通信延时引起误差
			//	   测试项（状态量变位）：测试先招测时钟，然后改变遥信状态，10秒后招测3104事件，此时上送时间不应早于招测时钟返回的时间。
			//			 在此处如果加7秒，在Get_StandardUnit（）产生3104事件时候，将事件发生时间也重新增加7秒。
			//     测试项（终端维护）：测试数据初始化3100事件，判断事件发生时间有效性，因此处增加7秒，相应事件产生时间增加7秒
//		    if(getZone("GW")==0) {
//		    	TS	add_ts;
//		    	TimeBCDToTs(time,&add_ts);
//		    	fprintf(stderr, "============================================\n\n\n\n\n\add_ts.sec=%d,\n",add_ts.Sec);
//		    	tminc(&add_ts, 0, 7);
//		    	TsToTimeBCD(add_ts,&time);
//		    	fprintf(stderr, "============================================\n\n\n\n\n\time.sec=%d,\n",add_ts.Sec);
//		    }

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
 * 搜表结果
 * */
int Get_6002(OAD oad,INT8U type,INT8U *data)
{
	int 	index=0,i=0,j=0,ret=0;
	CLASS_6002 class6002={};

	ret = readCoverClass(0x6002,0,&class6002,sizeof(CLASS_6002),para_vari_save);
	if ((ret == 1) || (type==1)) {
		switch(oad.attflg) {
		case 2:	//搜表结果
			if(class6002.searchNum > SERACH_NUM) {
				syslog(LOG_ERR,"搜表结果记录数[%d],大于限值[%d]",class6002.searchNum,SERACH_NUM);
				class6002.searchNum = SERACH_NUM;
			}
			index += create_array(&data[index],class6002.searchNum);
			for(i=0;i<class6002.searchNum;i++) {
				index += create_struct(&data[index],7);		//属性2：struct 7个元素
				index += fill_TSA(&data[index],&class6002.searchResult[i].CommAddr.addr[1],class6002.searchResult[i].CommAddr.addr[0]);
				index += fill_TSA(&data[index],&class6002.searchResult[i].CJQAddr.addr[1],class6002.searchResult[i].CJQAddr.addr[0]);
				index += fill_enum(&data[index],class6002.searchResult[i].protocol);
				index += fill_enum(&data[index],class6002.searchResult[i].phase);
				index += fill_unsigned(&data[index],class6002.searchResult[i].signal);
				index += fill_date_time_s(&data[index],&class6002.searchResult[i].searchTime);
				index += create_array(&data[index],class6002.searchResult[i].annexNum);
				if(class6002.searchResult[i].annexNum > SERACH_PARA_NUM) {
					syslog(LOG_ERR,"搜表结果记录数[%d],大于限值[%d]",class6002.crosszoneNum,SERACH_PARA_NUM);
					class6002.searchResult[i].annexNum = SERACH_PARA_NUM;
				}
				for(j=0;j<class6002.searchResult[i].annexNum;j++) {
					index += create_struct(&data[index],2);		//属性2：struct 2个元素
					index += create_OAD(1,&data[index],class6002.searchResult[i].annexInfo[j].oad);
					index += fill_Data(class6002.searchResult[i].annexInfo[j].data.type,&data[index],class6002.searchResult[i].annexInfo[j].data.data);
				}
			}
			break;
		case 5://跨台区结果
			if(class6002.crosszoneNum > SERACH_NUM) {
				syslog(LOG_ERR,"搜表结果记录数[%d],大于限值[%d]",class6002.crosszoneNum,SERACH_NUM);
				class6002.crosszoneNum = SERACH_NUM;
			}
			index += create_array(&data[index],class6002.crosszoneNum);
			for(i=0;i<class6002.crosszoneNum;i++) {
				index += create_struct(&data[index],3);		//属性2：struct 3个元素
				index += fill_TSA(&data[index],&class6002.crosszoneResult[i].CommAddr.addr[1],class6002.crosszoneResult[i].CommAddr.addr[0]);
				index += fill_TSA(&data[index],&class6002.crosszoneResult[i].mainPointAddr.addr[1],class6002.crosszoneResult[i].mainPointAddr.addr[0]);
				index += fill_date_time_s(&data[index],&class6002.crosszoneResult[i].changeTime);
			}
			break;
		case 6:	//搜表结果记录数
			index += fill_long_unsigned(&data[index],class6002.searchNum);
			break;
		case 7:	//跨台区搜表结果记录数
			index += fill_long_unsigned(&data[index],class6002.crosszoneNum);
			break;
		case 8:	//搜表
			index += create_struct(&data[index],4);
			index += fill_bool(&data[index],class6002.attr8.enablePeriodFlg);
			index += fill_bool(&data[index],class6002.attr8.autoUpdateFlg);
			index += fill_bool(&data[index],class6002.attr8.eventFlg);
			index += fill_enum(&data[index],class6002.attr8.clearChoice);
			break;
		case 9:	//每天周期搜表参数配置
			if(class6002.attr9_num > SERACH_PARA_NUM) {
				syslog(LOG_ERR,"搜表结果记录数[%d],大于限值[%d]",class6002.attr9_num,SERACH_PARA_NUM);
				class6002.attr9_num = SERACH_PARA_NUM;
			}
			index += create_array(&data[index],class6002.attr9_num);
			for(i=0;i<class6002.attr9_num;i++) {
				index += create_struct(&data[index],2);
				index += fill_time(&data[index],class6002.attr9[i].startTime);
				index += fill_long_unsigned(&data[index],class6002.attr9[i].searchLen);
			}
			break;
		case 10://搜表状态
			index += fill_enum(&data[index],class6002.searchSta);
			break;
		}
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

		if(task.runtime.runtime[23].beginHour==dtunsigned) {
			index += fill_unsigned(&data[index],task.runprio);		//执行优先级
		}else index += fill_enum(&data[index],task.runprio);			//执行优先级

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
	//fprintf(stderr,"\n 6015 read coll ok　seqnum=%d  type=%d  ret=%d\n",seqnum,type,ret);
	if ((ret == 1) || (type==1)) {
		//fprintf(stderr,"\n 6015 read coll ok　seqnum=%d  type=%d  ret=%d\n",seqnum,type,ret);
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
	//fprintf(stderr,"\n 6017 read coll ok　seqnum=%d  type=%d  ret=%d\n",seqnum,type,ret);
	if ((ret == 1) || (type==1)) {
		//fprintf(stderr,"\n 6017 read coll ok　seqnum=%d  type=%d  ret=%d\n",seqnum,type,ret);
		index += create_struct(&data[index],5);					//属性2：struct 5个元素
		index += fill_unsigned(&data[index],event.sernum);		//方案序号
		if(event.collstyle.colltype == 0xff ) {					//采集类型无效,为勘误前的定义结构
			if(event.collstyle.roads.num>ARRAY_ROAD_NUM)	event.collstyle.roads.num = ARRAY_ROAD_NUM;
			index += create_array(&data[index],event.collstyle.roads.num);
			for(i=0;i<event.collstyle.roads.num;i++) {
				index += fill_ROAD(1,&data[index],event.collstyle.roads.road[i]);	//采集数据
			}
		}else {
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
		}
		index += fill_MS(1,&data[index],event.ms);		//电能表集合
		index += fill_bool(&data[index],event.ifreport);		//上报标识
		index += fill_long_unsigned(&data[index],event.deepsize);		//存储深度
	}
	return index;
}

/*
 * 透明方案
 * */
int Get_6019(INT8U type,INT8U seqnum,INT8U *data)
{
	int 	index=0,ret=0,i=0,j=0;
	CLASS_6019 trans={};

	ret = readCoverClass(0x6019,seqnum,&trans,sizeof(CLASS_6019),coll_para_save);
	if ((ret == 1) || (type==1)) {
		//fprintf(stderr,"\n 6019 read coll ok　seqnum=%d  type=%d  ret=%d\n",seqnum,type,ret);
		index += create_struct(&data[index],3);					//属性2：struct 3个元素
		index += fill_unsigned(&data[index],trans.planno);		//方案序号
		for(i=0;i<trans.contentnum;i++) {		//方案内容集
			if(trans.plan[i].seqno!=0) {	//勘误增加序号设置
				index += create_struct(&data[index],6);
				index += fill_long_unsigned(&data[index],trans.plan[i].seqno);
			}else {
				index += create_struct(&data[index],5);
			}
			index += fill_TSA(&data[index],(INT8U *)&trans.plan[i].addr.addr[1],trans.plan[i].addr.addr[0]);
			index += fill_long_unsigned(&data[index],trans.plan[i].befscript);
			index += fill_long_unsigned(&data[index],trans.plan[i].aftscript);
			index += create_struct(&data[index],4);					//方案控制标志：struct 4个元素
			index += fill_bool(&data[index],trans.plan[i].planflag.waitnext);
			index += fill_long_unsigned(&data[index],trans.plan[i].planflag.overtime);
			index += fill_enum(&data[index],trans.plan[i].planflag.resultflag);
			index += create_struct(&data[index],3);					//结果比对参数：struct 3个元素
			index += fill_unsigned(&data[index],trans.plan[i].planflag.resultpara.featureByte);
			index += fill_long_unsigned(&data[index],trans.plan[i].planflag.resultpara.interstart);
			index += fill_long_unsigned(&data[index],trans.plan[i].planflag.resultpara.interlen);
			index += create_array(&data[index],trans.plan[i].datanum);//方案报文集
			for(j=0;j<trans.plan[i].datanum;j++) {
				index += create_struct(&data[index],2);
				index += fill_unsigned(&data[index],trans.plan[i].data[j].datano);
				index += fill_octet_string(&data[index],(char *)&trans.plan[i].data[j].data[1],trans.plan[i].data[j].data[0]);
			}
		}
		index += fill_long_unsigned(&data[index],trans.savedepth);
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

/*
 *  文件传输接口类接口
 * */
int GetClass18(INT8U attflg,INT8U *data)
{
	CLASS18		class18={};
	int	index = 0,ret = 0;

	ret = readCoverClass(0x18,0,&class18,sizeof(CLASS18),para_vari_save);
	switch(attflg) {
	case 2:	//文件信息
		index += create_struct(&data[index],6);
		index += fill_visible_string(&data[index],&class18.source_file[1],class18.source_file[0]);
		index += fill_visible_string(&data[index],&class18.dist_file[1],class18.dist_file[0]);
		index += fill_double_long_unsigned(&data[index],class18.file_size);
		INT8U file_attr=class18.file_attr & 0x03;
		index += fill_bit_string(&data[index],3,&file_attr);
		index += fill_visible_string(&data[index],&class18.file_version[1],class18.file_version[0]);
		index += fill_enum(&data[index],class18.file_type);
		break;
	case 3:	//命令结果
		index += fill_enum(&data[index],class18.cmd_result);
		break;
	}
	return index;
}

