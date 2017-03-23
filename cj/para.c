/*
 * para.c
 *
 *  Created on: Jan 5, 2017
 *      Author: admin
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "AccessFun.h"
#include "StdDataType.h"
#include "Objectdef.h"
#include "main.h"

//typedef struct
//{
//	char name[OCTET_STRING_LEN];		//逻辑名
//	char devdesc[VISIBLE_STRING_LEN];		//设备描述符
//	VERINFO info;						//版本信息
//	DateTimeBCD date_Product;			//生产日期
//	OI_698 ois[10];						//子设备列表
//	char  protcol[OCTET_STRING_LEN];	//支持的规约列表
//	INT8U follow_report;				//是否允许跟随上报
//	INT8U active_report;				//是否允许主动上报
//	INT8U talk_master;					//是否允许与主站通话
//} CLASS19;					//设备管理接口类

void print4000()
{
	CLASS_4000 oi4000={};

	memset(&oi4000,0,sizeof(CLASS_4000));
	fprintf(stderr,"校时参数[4000]\n");
	readCoverClass(0x4000,0,&oi4000,sizeof(CLASS_4000),para_vari_save);
	fprintf(stderr,"校时模式：%d  [主站授时（0），终端精确校时（1），北斗/GPS（2），其他（255）]\n",oi4000.type);
	fprintf(stderr,"日期时间-精准校时模式：\n");
	fprintf(stderr,"最近心跳时间总个数：%d\n",oi4000.hearbeatnum);
	fprintf(stderr,"最大值剔除个数：%d\n",oi4000.tichu_max);
	fprintf(stderr,"最小值剔除个数：%d\n",oi4000.tichu_min);
	fprintf(stderr,"通讯延时阀值：%d\n",oi4000.delay);
	fprintf(stderr,"最小有效个数：%d\n",oi4000.num_min);
}
void print4204()
{
	CLASS_4204 oi4204={};

	memset(&oi4204,0,sizeof(CLASS_4204));
	fprintf(stderr,"广播校时参数[4204]\n");
	readCoverClass(0x4204,0,&oi4204,sizeof(CLASS_4204),para_vari_save);
	fprintf(stderr,"终端广播校时开始时间：%d:%d:%d\n",oi4204.startime[0],oi4204.startime[1],oi4204.startime[2]);
	fprintf(stderr,"是否启用：%d\n",oi4204.enable);

	fprintf(stderr,"单地址终端广播校时开始时间：%d:%d:%d\n",oi4204.startime1[0],oi4204.startime1[1],oi4204.startime1[2]);
	fprintf(stderr,"时钟误差阀值：%d\n",oi4204.upleve);
	fprintf(stderr,"是否启用：%d\n",oi4204.enable1);
}
void print4300()
{
	CLASS19  oi4300={};
	fprintf(stderr,"设备接口类[4300]\n");
	readCoverClass(0x4300,0,&oi4300,sizeof(CLASS19),para_vari_save);
	fprintf(stderr,"\n1.逻辑名：%s",oi4300.name);
	fprintf(stderr,"\n2.设备描述符：%s",oi4300.devdesc);
	fprintf(stderr,"\n3.版本信息:");
	fprintf(stderr,"\n	厂商代码 %c%c%c%c",oi4300.info.factoryCode[0],oi4300.info.factoryCode[1],oi4300.info.factoryCode[2],oi4300.info.factoryCode[3]);
	fprintf(stderr,"\n	软件版本 %c%c%c%c",oi4300.info.softVer[0],oi4300.info.softVer[1],oi4300.info.softVer[2],oi4300.info.softVer[3]);
	fprintf(stderr,"\n	软件版本日期 %c%c%c%c%c%c",oi4300.info.softDate[0],oi4300.info.softDate[1],oi4300.info.softDate[2],oi4300.info.softDate[3],oi4300.info.softDate[4],oi4300.info.softDate[5]);
	fprintf(stderr,"\n	硬件版本 %c%c%c%c",oi4300.info.hardVer[0],oi4300.info.hardVer[1],oi4300.info.hardVer[2],oi4300.info.hardVer[3]);
	fprintf(stderr,"\n	硬件版本日期 %c%c%c%c%c%c",oi4300.info.hardDate[0],oi4300.info.hardDate[1],oi4300.info.hardDate[2],oi4300.info.hardDate[3],oi4300.info.hardDate[4],oi4300.info.hardDate[5]);
	fprintf(stderr,"\n	规约列表 %s\n",oi4300.protcol);
	fprintf(stderr,"\n4.生产日期 %d-%d-%d %d:%d:%d",oi4300.date_Product.year.data,oi4300.date_Product.month.data,oi4300.date_Product.day.data,oi4300.date_Product.hour.data,oi4300.date_Product.min.data,oi4300.date_Product.sec.data);
	fprintf(stderr,"\n5.子设备列表");
	fprintf(stderr,"\n6.支持的规约列表:%s",oi4300.protcol);
	fprintf(stderr,"\n7.是否允许跟随上报:%d",oi4300.follow_report);
	fprintf(stderr,"\n8.是否允许主动上报:%d",oi4300.active_report);
	fprintf(stderr,"\n9.是否允许与主站通话:%d",oi4300.talk_master);

}

void para_process(int argc, char *argv[])
{
	int 	tmp=0;
	OI_698	oi=0;
	int 	method=0;

	if(argc==5) {	//para
		if(strcmp(argv[1],"para")==0) {
			if(strcmp(argv[2],"method")==0) {
				sscanf(argv[3],"%04x",&tmp);
				oi = tmp;
				sscanf(argv[4],"%d",&tmp);
				method = tmp;
				switch(oi) {
				case 0x4300:
					switch(method) {
					case 3:		//数据区初始化
					case 5:		//事件初始化
					case 6:		//需量初始化
						dataInit(method);
						break;
					}
					break;
				}
			}
		}
	}
	if(argc==4) {	//para
		if(strcmp(argv[2],"pro")==0) {
			sscanf(argv[3],"%04x",&tmp);
			oi = tmp;
			switch(oi) {
			case 0x4300:
				print4300();
				break;
			case 0x4000:
				print4000();
				break;
			case 0x4204:
				print4204();
				break;
			}
		}
	}
}

