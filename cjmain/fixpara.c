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


void InitClass4300() {   //电气设备信息
    CLASS19 oi4300 = {};
    int ret        = 0;

    memset(&oi4300, 0, sizeof(CLASS19));
	ret = readCoverClass(0x4300, 0, &oi4300, sizeof(CLASS19), para_vari_save);
	if((ret!=1) || (memcmp(&oi4300.info,&verinfo,sizeof(VERINFO))!=0)
			|| memcmp(&oi4300.date_Product,&product_date,sizeof(DateTimeBCD))!=0
			|| memcmp(&oi4300.protcol,protcol,sizeof(oi4300.protcol))!=0) {
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
    fprintf(stderr, "\n生产日期 %d-%d-%d", oi4300.date_Product.year.data,oi4300.date_Product.month.data,oi4300.date_Product.day.data);
}
