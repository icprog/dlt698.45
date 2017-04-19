/*
 * fixpara.c
 *
 *  Created on: Feb 10, 2017
 *      Author: ava
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>
#include "sys/reboot.h"
#include <wait.h>
#include <errno.h>

#include "ParaDef.h"
#include "PublicFunction.h"
#include "AccessFun.h"
#include "Objectdef.h"

static VERINFO  	verinfo={"QDGK","ZJSJ","170323","1.00","150628",""};	//4300 版本信息
static DateTimeBCD 	product_date={{2016},{04},{6},{0},{0},{0}};		//4300 生产日期
static char		protcol[]="DL/T 698.45";			//4300 支持规约类型


void InitClass4300() 	 //电气设备信息
{
    CLASS19 oi4300 = {};
    int ret        = 0;

    memset(&oi4300, 0, sizeof(CLASS19));
	ret = readCoverClass(0x4300, 0, &oi4300, sizeof(CLASS19), para_vari_save);
	if((ret!=1) || (memcmp(&oi4300.info,&verinfo,sizeof(VERINFO))!=0)
			|| memcmp(&oi4300.date_Product,&product_date,sizeof(DateTimeBCD))!=0
			|| memcmp(&oi4300.protcol,protcol,sizeof(protcol))!=0) {
		fprintf(stderr,"\n初始化电气设备信息：4300\n");
		memcpy(&oi4300.info,&verinfo,sizeof(VERINFO));
		memcpy(&oi4300.date_Product,&product_date,sizeof(DateTimeBCD));
		memcpy(&oi4300.protcol,protcol,sizeof(oi4300.protcol));
		saveCoverClass(0x4300, 0, &oi4300, sizeof(CLASS19), para_vari_save);
	}
	fprintf(stderr, "\n厂商代码 %c%c%c%c", oi4300.info.factoryCode[0], oi4300.info.factoryCode[1], oi4300.info.factoryCode[2], oi4300.info.factoryCode[3]);
    fprintf(stderr, "\n软件版本 %c%c%c%c", oi4300.info.softVer[0], oi4300.info.softVer[1], oi4300.info.softVer[2], oi4300.info.softVer[3]);
    fprintf(stderr, "\n软件版本日期 %c%c%c%c%c%c", oi4300.info.softDate[0], oi4300.info.softDate[1], oi4300.info.softDate[2], oi4300.info.softDate[3],
            oi4300.info.softDate[4], oi4300.info.softDate[5]);
    fprintf(stderr, "\n硬件版本 %c%c%c%c", oi4300.info.hardVer[0], oi4300.info.hardVer[1], oi4300.info.hardVer[2], oi4300.info.hardVer[3]);
    fprintf(stderr, "\n硬件版本日期 %c%c%c%c%c%c", oi4300.info.hardDate[0], oi4300.info.hardDate[1], oi4300.info.hardDate[2], oi4300.info.hardDate[3],
            oi4300.info.hardDate[4], oi4300.info.hardDate[5]);
    fprintf(stderr, "\n规约列表 %s", oi4300.protcol);
    fprintf(stderr, "\n生产日期 %d-%d-%d\n", oi4300.date_Product.year.data,oi4300.date_Product.month.data,oi4300.date_Product.day.data);
}

void InitClass4500()	//公网通信模块1
{
	CLASS25	 oi4500 = {};
    int ret        = 0;
    memset(&oi4500, 0, sizeof(CLASS25));
    ret = readCoverClass(0x4500, 0, (void*)&oi4500, sizeof(CLASS25), para_vari_save);
    if(ret!=1) {
    	fprintf(stderr,"\n初始化公网通信模块1：4500\n");
    	oi4500.commconfig.workModel = 1;		//客户机模式
    	oi4500.commconfig.onlineType = 0;
    	oi4500.commconfig.connectType = 0;
    	oi4500.commconfig.listenPortnum = 1;
    	oi4500.commconfig.listenPort[0] = 0x5022;
    	oi4500.commconfig.heartBeat = 60;	//60s
    	oi4500.master.masternum = 1;
    	oi4500.master.master[0].ip[1] = 192;
    	oi4500.master.master[0].ip[2] = 168;
    	oi4500.master.master[0].ip[3] = 0;
    	oi4500.master.master[0].ip[4] = 179;
    	oi4500.master.master[0].port = 0x5022;
    	saveCoverClass(0x4500, 0, &oi4500, sizeof(CLASS25), para_vari_save);
    }
}

void InitClass4510()	//以太网通信模块1
{
	CLASS26	 oi4510 = {};
    int ret        = 0;
    memset(&oi4510, 0, sizeof(CLASS26));
    ret = readCoverClass(0x4510, 0, (void*)&oi4510, sizeof(CLASS26), para_vari_save);
    if(ret!=1) {
    	fprintf(stderr,"\n初始化以太网通信模块1：4510\n");
    	oi4510.commconfig.workModel = 1;		//客户机模式
    	oi4510.commconfig.connectType = 0;
    	oi4510.commconfig.listenPortnum = 1;
    	oi4510.commconfig.listenPort[0] = 0x5022;
    	oi4510.commconfig.heartBeat = 60;	//60s
    	oi4510.master.masternum = 1;
    	oi4510.master.master[0].ip[1] = 192;
    	oi4510.master.master[0].ip[2] = 168;
    	oi4510.master.master[0].ip[3] = 0;
    	oi4510.master.master[0].ip[4] = 179;
    	oi4510.master.master[0].port = 0x5022;
    	saveCoverClass(0x4510, 0, &oi4510, sizeof(CLASS26), para_vari_save);
    }
}
