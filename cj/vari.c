/*
 * vari.c
 *
 *  Created on: Apr 5, 2017
 *      Author: ava
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "AccessFun.h"
#include "StdDataType.h"
#include "Objectdef.h"
#include "main.h"
#include "dlt698.h"
#include "dlt645.h"
#include "PublicFunction.h"

void print213x(OI_698  oi)
{
	int	  	len=0;
	Volt_PassRate_tj	volt_tj={};
	memset(&volt_tj,0,sizeof(Volt_PassRate_tj));
	len = readVariData(oi,0,&volt_tj,sizeof(Volt_PassRate_tj));
	fprintf(stderr," OI=%04x\n",oi);
	fprintf(stderr," 当日电压合格率\n");
	fprintf(stderr," 	电压监测时间（分钟）：%d\n",volt_tj.dayu_tj.monitorTime);
	fprintf(stderr," 	电压合格率：%d\n",volt_tj.dayu_tj.passRate);
	fprintf(stderr," 	电压超限率：%d\n",volt_tj.dayu_tj.overRate);
	fprintf(stderr," 	电压超上限时间（分钟）：%d\n",volt_tj.dayu_tj.upLimitTime);
	fprintf(stderr," 	电压超下限时间（分钟）：%d\n",volt_tj.dayu_tj.downLimitTime);
	fprintf(stderr," 当月电压合格率\n");
	fprintf(stderr," 	电压监测时间（分钟）：%d\n",volt_tj.monthu_tj.monitorTime);
	fprintf(stderr," 	电压合格率：%d\n",volt_tj.monthu_tj.passRate);
	fprintf(stderr," 	电压超限率：%d\n",volt_tj.monthu_tj.overRate);
	fprintf(stderr," 	电压超上限时间（分钟）：%d\n",volt_tj.monthu_tj.upLimitTime);
	fprintf(stderr," 	电压超下限时间（分钟）：%d\n",volt_tj.monthu_tj.downLimitTime);
}

void print2203()
{
	Gongdian_tj gongdian_tj={};
	int	  	len=0;
	INT32U day_tj=0,month_tj=0;

	memset(&gongdian_tj,0,sizeof(Gongdian_tj));
	len = readVariData(0x2203,0,&gongdian_tj,sizeof(Gongdian_tj));	//TODO: 是否需要 1分钟保存一次
	fprintf(stderr,"2203: len=%d 时间: %d-%d-%d %d:%d:%d\n",len,gongdian_tj.ts.Year,gongdian_tj.ts.Month,gongdian_tj.ts.Day,gongdian_tj.ts.Hour,gongdian_tj.ts.Minute,gongdian_tj.ts.Sec);
	if(gongdian_tj.gongdian.day_tj%60==0)
		day_tj = gongdian_tj.gongdian.day_tj/60;
	else
		day_tj = gongdian_tj.gongdian.day_tj/60+1;
	if(gongdian_tj.gongdian.month_tj%60==0)
		month_tj = gongdian_tj.gongdian.month_tj/60;
	else
		month_tj = gongdian_tj.gongdian.month_tj/60+1;
	fprintf(stderr," 日供电时间=%d（分）\n 月供电时间=%d（分）\n",day_tj,month_tj);
}

void print2204()
{

	Reset_tj reset_tj={};
	int	  	len=0;

	memset(&reset_tj,0,sizeof(Reset_tj));
	len = readVariData(0x2204,0,&reset_tj,sizeof(Reset_tj));
	fprintf(stderr,"2204:len=%d 复位时间: %d-%d-%d %d:%d:%d\n",len,reset_tj.ts.Year,reset_tj.ts.Month,reset_tj.ts.Day,reset_tj.ts.Hour,reset_tj.ts.Minute,reset_tj.ts.Sec);
	fprintf(stderr,"日复位累计次数=%d,月复位累计次数=%d\n",reset_tj.reset.day_tj,reset_tj.reset.month_tj);
}

void print2200()
{
	Flow_tj realTimeC2200;
	int	  	len=0;

	memset(&realTimeC2200,0,sizeof(Flow_tj));
	len = readVariData(0x2200,0,&realTimeC2200,sizeof(Flow_tj));
	fprintf(stderr,"当日通信流量=%d,当月通信流量=%d\n",realTimeC2200.flow.day_tj,realTimeC2200.flow.month_tj);
}
void vari_process(int argc, char *argv[])
{
	int 	tmp=0;
	OI_698	oi=0;

	if(argc==3) {	//vari
		sscanf(argv[2],"%04x",&tmp);
		oi = tmp;
		switch(oi) {
		case 0x2131:
		case 0x2132:
		case 0x2133:
			print213x(oi);
			break;
		case 0x2203:
			print2203();
			break;
		case 0x2204:
			print2204();
			break;
		case 0x2200:
       		print2200();
			break;
		}
	}
}

