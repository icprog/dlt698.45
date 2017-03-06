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
 * datetime 开始时间
 * ti 间隔
 */
time_t calcnexttime(TI ti,DateTimeBCD datetime)
{
	int spacenum=0;
	int jiange=0;
	time_t timestart=0,timenow=0,timetmp=0,timeret=0;
	TS ptm;
	ptm.Year = datetime.year.data;
	ptm.Month = datetime.month.data;
	ptm.Day = datetime.day.data;
	ptm.Hour = datetime.hour.data;
	ptm.Minute = datetime.min.data;
	ptm.Sec = datetime.sec.data;
	timestart = tmtotime_t(ptm);//开始时间
	timenow = time(NULL);//当前时间
	switch(ti.units)
	{
		case 0://秒
			jiange = ti.interval;
			break;
		case 1://分
			jiange = ti.interval * 60;
			break;
		case 2://时
			jiange =  ti.interval * 3600;
			break;
		case 3://日
			jiange = ti.interval * 3600 *24;
			break;
		case 4://月
			break;
		case 5://年
			break;
		default :
			break;
	}
	if (timenow > timestart)
	{
//		timetmp = timenow - timestart;
//		spacenum = timetmp / jiange;
//		timeret = timestart + spacenum* jiange;

		if (timeret >= timenow)
		{
			return timeret;
		}else
		{
			return (timeret + jiange);
		}
	}else
		return timestart;
}

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
