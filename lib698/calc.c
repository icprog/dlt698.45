/*
 * calc.c
 *
 *  Created on: 2017-2-24
 *      Author: wzm
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "AccessFun.h"
#include "EventObject.h"
#include "dlt698.h"
#include "dlt698def.h"
#include "secure.h"
#include "Esam.h"
#include "ParaDef.h"
#include "Shmem.h"
#include "PublicFunction.h"
#include "CalcObject.h"
/*
 * 硬件复位43000100 调用
 */
INT8U Reset_add()
{
	Reset_tj reset_tj={};
	TS 		newts={};
	int	  	len=0;

	memset(&reset_tj,0,sizeof(Reset_tj));
	TSGet(&newts);
	fprintf(stderr," Reset_add: %d-%d-%d\n",newts.Year,newts.Month,newts.Day);
	len = readVariData(0x2204,0,&reset_tj,sizeof(Reset_tj));
	if(len > 0) { 	//读取到数据
		fprintf(stderr,"read day_reset=%d,mon_reset=%d\n",reset_tj.reset.day_tj,reset_tj.reset.month_tj);
		fprintf(stderr,"当前时间:%d-%d  记录时间 %d-%d\n",newts.Month,newts.Day,reset_tj.ts.Month,reset_tj.ts.Day);
		//如果跨天 日供电清零
		if(reset_tj.ts.Day != newts.Day) {
			TSGet(&reset_tj.ts);
			reset_tj.reset.day_tj = 0;
		}
		//如果跨月 月供电清零
		if(reset_tj.ts.Month != newts.Month) {
			TSGet(&reset_tj.ts);
			reset_tj.reset.month_tj = 0;
		}
	}else {
		TSGet(&reset_tj.ts);
	}
	reset_tj.reset.day_tj++;
	reset_tj.reset.month_tj++;
	fprintf(stderr,"day_reset=%d,mon_reset=%d,TS=%d-%d\n",reset_tj.reset.day_tj,reset_tj.reset.month_tj,reset_tj.ts.Month,reset_tj.ts.Day);
    len = saveVariData(0x2204,0,&reset_tj,sizeof(Reset_tj));
    fprintf(stderr,"len=%d\n",len);
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

//	static INT8U first=0;
//	if(first==0) {
//		first=1;
//		reset_tj.reset.day_tj=1;
//		reset_tj.reset.month_tj =1;
//	}else {
//		reset_tj.reset.day_tj=1;
//		reset_tj.reset.month_tj =2;
//	}
	fprintf(stderr,"Get_2204 :reset day_tj=%d,month_tj=%d\n",reset_tj.reset.day_tj,reset_tj.reset.month_tj);
	*len=0;
	*len += create_struct(&buf[*len],2);
	*len += fill_long_unsigned(&buf[*len],reset_tj.reset.day_tj);
	*len += fill_long_unsigned(&buf[*len],reset_tj.reset.month_tj);
	return 1;
}

/*
 * 获取电压合格率 0x2131 2132 2133
 */
INT8U Get_Voltagehegelv(INT8U *buf,INT8U *len,TSA tsa,OAD oad){
	StatisticsPointProp StatisticsPoint[MAXNUM_IMPORTANTUSR];
	INT8U i=0;

	*len=0;
	for(i=0;i<MAXNUM_IMPORTANTUSR;i++){
	   if(readVariData(oad.OI,i,&StatisticsPoint[i],sizeof(StatisticsPointProp))) {
		   if(memcmp(&StatisticsPoint[i].tsa,&tsa,sizeof(TSA)) == 0){
			   if(oad.OI==0x2131 && oad.attflg==2){
					*len += create_struct(&buf[*len],2);
					*len += create_struct(&buf[*len],5);
					*len += fill_double_long_unsigned(&buf[*len],StatisticsPoint[i].DayResu.tjUa.U_Count);
					*len += fill_long_unsigned(&buf[*len],StatisticsPoint[i].DayResu.tjUa.ok_Rate);
					*len += fill_long_unsigned(&buf[*len],100-StatisticsPoint[i].DayResu.tjUa.ok_Rate);	//电压超限率
					*len += fill_long_unsigned(&buf[*len],StatisticsPoint[i].DayResu.tjUa.s_count);
					*len += fill_double_long_unsigned(&buf[*len],StatisticsPoint[i].DayResu.tjUa.x_count);
					*len += create_struct(&buf[*len],5);
					*len += fill_double_long_unsigned(&buf[*len],StatisticsPoint[i].MonthResu.tjUa.U_Count);
					*len += fill_long_unsigned(&buf[*len],StatisticsPoint[i].MonthResu.tjUa.ok_Rate);
					*len += fill_long_unsigned(&buf[*len],100-StatisticsPoint[i].MonthResu.tjUa.ok_Rate);	//电压超限率
					*len += fill_long_unsigned(&buf[*len],StatisticsPoint[i].MonthResu.tjUa.s_count);
					*len += fill_double_long_unsigned(&buf[*len],StatisticsPoint[i].MonthResu.tjUa.x_count);
			   }else if(oad.OI==0x2132 && oad.attflg==2){
				   	*len += create_struct(&buf[*len],2);
					*len += create_struct(&buf[*len],5);
					*len += fill_double_long_unsigned(&buf[*len],StatisticsPoint[i].DayResu.tjUb.U_Count);
					*len += fill_long_unsigned(&buf[*len],StatisticsPoint[i].DayResu.tjUb.ok_Rate);
					*len += fill_long_unsigned(&buf[*len],100-StatisticsPoint[i].DayResu.tjUb.ok_Rate);	//电压超限率
					*len += fill_long_unsigned(&buf[*len],StatisticsPoint[i].DayResu.tjUb.s_count);
					*len += fill_double_long_unsigned(&buf[*len],StatisticsPoint[i].DayResu.tjUb.x_count);
					*len += create_struct(&buf[*len],5);
					*len += fill_double_long_unsigned(&buf[*len],StatisticsPoint[i].MonthResu.tjUb.U_Count);
					*len += fill_long_unsigned(&buf[*len],StatisticsPoint[i].MonthResu.tjUb.ok_Rate);
					*len += fill_long_unsigned(&buf[*len],100-StatisticsPoint[i].MonthResu.tjUb.ok_Rate);	//电压超限率
					*len += fill_long_unsigned(&buf[*len],StatisticsPoint[i].MonthResu.tjUb.s_count);
					*len += fill_double_long_unsigned(&buf[*len],StatisticsPoint[i].MonthResu.tjUb.x_count);
			   }else if(oad.OI==0x2133 && oad.attflg==2){
				   	*len += create_struct(&buf[*len],2);
					*len += create_struct(&buf[*len],5);
					*len += fill_double_long_unsigned(&buf[*len],StatisticsPoint[i].DayResu.tjUc.U_Count);
					*len += fill_long_unsigned(&buf[*len],StatisticsPoint[i].DayResu.tjUc.ok_Rate);
					*len += fill_long_unsigned(&buf[*len],100-StatisticsPoint[i].DayResu.tjUc.ok_Rate);	//电压超限率
					*len += fill_long_unsigned(&buf[*len],StatisticsPoint[i].DayResu.tjUc.s_count);
					*len += fill_double_long_unsigned(&buf[*len],StatisticsPoint[i].DayResu.tjUc.x_count);
					*len += create_struct(&buf[*len],5);
					*len += fill_double_long_unsigned(&buf[*len],StatisticsPoint[i].MonthResu.tjUc.U_Count);
					*len += fill_long_unsigned(&buf[*len],StatisticsPoint[i].MonthResu.tjUc.ok_Rate);
					*len += fill_long_unsigned(&buf[*len],100-StatisticsPoint[i].MonthResu.tjUc.ok_Rate);	//电压超限率
					*len += fill_long_unsigned(&buf[*len],StatisticsPoint[i].MonthResu.tjUc.s_count);
					*len += fill_double_long_unsigned(&buf[*len],StatisticsPoint[i].MonthResu.tjUc.x_count);
			   }
		   }
	   }
	   break;
	}
	return 1;
}

/*
 * 获取最大功率及发生时间 2140 2141
 */
INT8U Get_Maxp(INT8U *buf,INT8U *len,TSA tsa,OAD oad){
	Max_ptongji max_ptongji[MAXNUM_IMPORTANTUSR_CALC];
	*len=0;
    INT8U i=0;
	for(i=0;i<MAXNUM_IMPORTANTUSR;i++){
	   if(readVariData(0x2140,i,&max_ptongji[i],sizeof(Max_ptongji))) {
		   if(memcmp(&max_ptongji[i].tsa,&tsa,sizeof(TSA)) == 0){
			   if(oad.OI==0x2140 && oad.attflg==2){
				   *len += create_struct(&buf[*len],2);
				   *len += fill_double_long_unsigned(&buf[*len],max_ptongji[i].mp.d_max);
				   fill_date_time_s(&buf[*len],&max_ptongji[i].mp.d_ts);
			   }else if(oad.OI==0x2141 && oad.attflg==2){
				   *len += create_struct(&buf[*len],2);
				   *len += fill_double_long_unsigned(&buf[*len],max_ptongji[i].mp.m_max);
				   fill_date_time_s(&buf[*len],&max_ptongji[i].mp.m_ts);
			   }
			   break;
		   }
	   }
	}
	return 1;
}
