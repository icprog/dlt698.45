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
/*
 * 硬件复位43000100 调用
 */
INT8U Reset_add(){
	Reset_tj reset_tj;
	memset(&reset_tj,0,sizeof(Reset_tj));
	TS newts;
	TSGet(&newts);
	if(readCoverClass(0x2204,0,&reset_tj,sizeof(Reset_tj),calc_voltage_save)==1)
	{
		//如果跨天 日供电清零
		if(reset_tj.ts.Day != newts.Day)
			reset_tj.day_reset = 0;
		//如果跨月 月供电清零
		if(reset_tj.ts.Month != newts.Month)
			reset_tj.month_reset = 0;
	}
	reset_tj.day_reset++;
	reset_tj.month_reset++;
    memcpy(&reset_tj.ts,&newts,sizeof(TS));
	saveCoverClass(0x2204,0,&reset_tj,sizeof(Reset_tj),calc_voltage_save);
	return 1;
}

/*
 * 获取日月供电时间
 */
INT8U Get_2203(INT8U *buf,INT8U *len){
	Gongdian_tj gongdian_tj;
	*len=0;
	buf[(*len)++]=dtstructure;//structure
	buf[(*len)++]=2; //数量
	if(readCoverClass(0x2203,0,&gongdian_tj,sizeof(Gongdian_tj),calc_voltage_save)==1)
	{
       buf[(*len)++]=dtdoublelongunsigned;//doublelongunsigned
       buf[(*len)++]=gongdian_tj.day_gongdian&0x000000ff;
       buf[(*len)++]=((gongdian_tj.day_gongdian>>8)&0x000000ff);
       buf[(*len)++]=((gongdian_tj.day_gongdian>>16)&0x000000ff);
       buf[(*len)++]=((gongdian_tj.day_gongdian>>24)&0x000000ff);
       buf[(*len)++]=dtdoublelongunsigned;//doublelongunsigned
	   buf[(*len)++]=gongdian_tj.month_gongdian&0x000000ff;
	   buf[(*len)++]=((gongdian_tj.month_gongdian>>8)&0x000000ff);
	   buf[(*len)++]=((gongdian_tj.month_gongdian>>16)&0x000000ff);
	   buf[(*len)++]=((gongdian_tj.month_gongdian>>24)&0x000000ff);
	}else
	{
	   buf[(*len)++]=dtdoublelongunsigned;//doublelongunsigned
	   buf[(*len)++]=0;
	   buf[(*len)++]=0;
	   buf[(*len)++]=0;
	   buf[(*len)++]=0;
	   buf[(*len)++]=dtdoublelongunsigned;//doublelongunsigned
	   buf[(*len)++]=0;
	   buf[(*len)++]=0;
	   buf[(*len)++]=0;
	   buf[(*len)++]=0;
	}
	return 1;
}

/*
 * 获取日月复位次数
 */
INT8U Get_2204(INT8U *buf,INT8U *len){
	Reset_tj reset_tj;
	*len=0;
	buf[(*len)++]=dtstructure;//structure
	buf[(*len)++]=2; //数量
	if(readCoverClass(0x2203,0,&reset_tj,sizeof(Reset_tj),calc_voltage_save)==1)
	{
       buf[(*len)++]=dtlongunsigned;//longunsigned
       buf[(*len)++]=reset_tj.day_reset&0x00ff;
       buf[(*len)++]=((reset_tj.day_reset>>8)&0x00ff);
       buf[(*len)++]=dtlongunsigned;//longunsigned
	   buf[(*len)++]=reset_tj.month_reset&0x00ff;
	   buf[(*len)++]=((reset_tj.month_reset>>8)&0x00ff);
	}else
	{
	   buf[(*len)++]=dtlongunsigned;//longunsigned
	   buf[(*len)++]=0;
	   buf[(*len)++]=0;
	   buf[(*len)++]=dtlongunsigned;//longunsigned
	   buf[(*len)++]=0;
	   buf[(*len)++]=0;
	}
	return 1;
}


