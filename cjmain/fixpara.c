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


void InitClass4300(CLASS19 *oi4300)
{
//	CLASS19	 oi4300={};
	int	 ret=0;
	ret = readCoverClass(0x4300,0,oi4300,sizeof(CLASS19),para_vari_save);
	if(ret!=1) {
		memset(oi4300,0,sizeof(CLASS19));
		strncpy((char *)&oi4300->name[1],"4300",sizeof(oi4300->name));
		strncpy((char *)&oi4300->info.factoryCode[1],"QDGK",sizeof(oi4300->info.factoryCode));
		strncpy((char *)&oi4300->info.softVer[1],"V1.1",sizeof(oi4300->info.softVer));
		strncpy((char *)&oi4300->info.softDate[1],"161102",sizeof(oi4300->info.softDate));
		strncpy((char *)&oi4300->info.hardVer[1],"1.00",sizeof(oi4300->info.hardVer));
		strncpy((char *)&oi4300->info.hardDate[1],"150628",sizeof(oi4300->info.hardDate));
		strncpy((char *)&oi4300->protcol[1],"DL/T 698.45",sizeof(oi4300->protcol));
		saveCoverClass(0x4300,0,oi4300,sizeof(CLASS19),para_init_save);
		fprintf(stderr,"\n厂商代码 %s",oi4300->info.factoryCode);
		fprintf(stderr,"\n软件版本 %s",oi4300->info.softVer);
		fprintf(stderr,"\n软件版本日期 %s",oi4300->info.softDate);
		fprintf(stderr,"\n硬件版本 %s",oi4300->info.hardVer);
		fprintf(stderr,"\n硬件版本日期 %s",oi4300->info.hardDate);
	}
}

