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
	ret = readCoverClass(0x4300,0,&oi4300,sizeof(CLASS19),para_vari_save);
	if(ret!=1) {
		memset(&oi4300,0,sizeof(CLASS19));
		strncpy(oi4300.name,"4300",sizeof(oi4300.name));
		strncpy(oi4300.info.factoryCode,"QDGK",sizeof(oi4300.info.factoryCode));
		strncpy(oi4300.info.softVer,"V1.1",sizeof(oi4300.info.softVer));
		strncpy(oi4300.info.softDate,"161102",sizeof(oi4300.info.softDate));
		strncpy(oi4300.info.hardVer,"1.00",sizeof(oi4300.info.hardVer));
		strncpy(oi4300.info.hardDate,"150628",sizeof(oi4300.info.hardDate));
		strncpy(oi4300.protcol,"DL/T 698.45",sizeof(oi4300.protcol));
		saveCoverClass(0x4300,0,&oi4300,sizeof(CLASS19),para_init_save);
	}
}

