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


void InitClass4300()
{
	CLASS19	 oi4300={};
	int	 ret=0;
	memset(&oi4300,0,sizeof(CLASS19));
	ret = readCoverClass(0x4300,0,&oi4300,sizeof(CLASS19),para_vari_save);
	if(ret!=1) {
		strncpy(oi4300.name,"4300",sizeof(oi4300.name));
		strncpy(oi4300.info.factoryCode,"QDGK",sizeof(oi4300.info.factoryCode));
		strncpy(oi4300.info.softVer,"ZJSJ",sizeof(oi4300.info.softVer));
		strncpy(oi4300.info.softDate,"170223",sizeof(oi4300.info.softDate));
		strncpy(oi4300.info.hardVer,"1.00",sizeof(oi4300.info.hardVer));
		strncpy(oi4300.info.hardDate,"150628",sizeof(oi4300.info.hardDate));
		strncpy(oi4300.protcol,"DL/T 698.45",sizeof(oi4300.protcol));
		oi4300.date_Product.year.data = 2017;
		oi4300.date_Product.month.data = 02;
		oi4300.date_Product.day.data = 23;
		saveCoverClass(0x4300,0,&oi4300,sizeof(CLASS19),para_init_save);
	}
	fprintf(stderr,"\n厂商代码 %c%c%c%c",oi4300.info.factoryCode[0],oi4300.info.factoryCode[1],oi4300.info.factoryCode[2],oi4300.info.factoryCode[3]);
	fprintf(stderr,"\n软件版本 %c%c%c%c",oi4300.info.softVer[0],oi4300.info.softVer[1],oi4300.info.softVer[2],oi4300.info.softVer[3]);
	fprintf(stderr,"\n软件版本日期 %c%c%c%c%c%c",oi4300.info.softDate[0],oi4300.info.softDate[1],oi4300.info.softDate[2],oi4300.info.softDate[3],oi4300.info.softDate[4],oi4300.info.softDate[5]);
	fprintf(stderr,"\n硬件版本 %c%c%c%c",oi4300.info.hardVer[0],oi4300.info.hardVer[1],oi4300.info.hardVer[2],oi4300.info.hardVer[3]);
	fprintf(stderr,"\n硬件版本日期 %c%c%c%c%c%c",oi4300.info.hardDate[0],oi4300.info.hardDate[1],oi4300.info.hardDate[2],oi4300.info.hardDate[3],oi4300.info.hardDate[4],oi4300.info.hardDate[5]);
	fprintf(stderr,"\n规约列表 %s\n",oi4300.protcol);
}

