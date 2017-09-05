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
#include "basedef.h"
/*
 * 硬件复位43000100 调用
 */
INT8U Reset_add()
{
	Reset_tj reset_tj={};
	TS 		newts={};
	int	  	len=0;
	OAD		oad={};
	DateTimeBCD datetime={};

	memset(&reset_tj,0,sizeof(Reset_tj));
	TSGet(&newts);
	fprintf(stderr," Reset_add: %d-%d-%d\n",newts.Year,newts.Month,newts.Day);
	len = readVariData(0x2204,0,&reset_tj,sizeof(Reset_tj));
	if(len > 0) { 	//读取到数据
		fprintf(stderr,"read day_reset=%d,mon_reset=%d\n",reset_tj.reset.day_tj,reset_tj.reset.month_tj);
		fprintf(stderr,"当前时间:%d-%d  记录时间 %d-%d\n",newts.Month,newts.Day,reset_tj.ts.Month,reset_tj.ts.Day);
		//如果跨天 日供电清零
		if(reset_tj.ts.Day != newts.Day) {
			Save_TJ_Freeze(0x2204,0x0200,0,newts,sizeof(reset_tj.reset),(INT8U *)&reset_tj.reset);
			TSGet(&reset_tj.ts);
			reset_tj.reset.day_tj = 0;
		}
		//如果跨月 月供电清零
		if(reset_tj.ts.Month != newts.Month) {
			Save_TJ_Freeze(0x2204,0x0200,0,newts,sizeof(reset_tj.reset),(INT8U *)&reset_tj.reset);
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

INT8U fillVacsData(INT8U structnum,INT8U attindex,INT8U datatype,INT32U data1,INT32U data2,INT32U data3,INT32U data4,INT8U *responseData)
{
	INT32U 	data[4]={};
	INT8U	index=0,i=0;

	if(structnum>4) {
		fprintf(stderr,"填充数据结构【%d】大于有效限定值【4】!!!",structnum);
		structnum = 4;
	}
	if (attindex>4) {
		fprintf(stderr,"属性索引值【%d】大于有效限定值【4】!!!",attindex);
		attindex = 4;
	}
	if(structnum>=1) {
		index += create_array(&responseData[index],structnum);
	}
	memset(data,0,sizeof(data));
	switch(attindex) {
	case 0:		//全部属性
		data[0] = data1;
		data[1] = data2;
		data[2] = data3;
		data[3] = data4;
	break;
	case 1:	data[0] = data1;	break;
	case 2:	data[0] = data2;	break;
	case 3:	data[0] = data3;	break;
	case 4:	data[0] = data4;	break;
	}
	for(i=0;i<structnum;i++) {
		switch(datatype){
		case dtlongunsigned:
			index += fill_long_unsigned(&responseData[index],data[i]);
			break;
		case dtdoublelong:
			index += fill_double_long(&responseData[index],data[i]);
			break;
		case dtlong:
			index += fill_long(&responseData[index],data[i]);
			break;
		}
	}
	return index;
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

