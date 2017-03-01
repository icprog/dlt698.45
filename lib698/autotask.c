/*
 * autotask.c
 *
 *  Created on: Mar 1, 2017
 *      Author: admin
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../include/Objectdef.h"

/*
 * 初始化自动上报方案
 */
void init_autotask(CLASS_6013 class6013,AutoTaskStrap* list)
{
	static int index=0;

	if(class6013.cjtype == rept)
	{
		list[index].ID = class6013.taskID;
		list[index].SerNo = class6013.sernum;
//		list[index].nexttime = nexttime(ti,timebcd);
		index+=1;
	}
}
