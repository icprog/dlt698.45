/*
 * read485.c
 *
 *  Created on: 2017-1-4
 *      Author: wzm
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>
#include "sys/reboot.h"
#include <wait.h>
#include <errno.h>
#include <string.h>
#include "read485.h"

/*
 * 根据测量点串口参数是否改变
 * 返回值：<=0：串口打开失败
 * 		  >0： 串口打开句柄
 * */
INT32S open_com_para_chg(INT8U port,INT32U baud,INT32S oldcomfd)
{
	INT32S newfd = 0;
	static INT8U lastport = 0;
	static INT32U lastbaud = 0;

	if ((lastbaud==baud) &&((lastport==port)))
	{
		return oldcomfd;
	}
	else
	{

	}
	if(oldcomfd>0)
	{
		CloseCom(oldcomfd);
		sleep(1);
	}
	newfd = OpenCom(port,baud,(char *)"even",1,8);

	return newfd;
}

/*
 * 根据6013任务配置单元去文件里查找对应的6015普通采集方案
 * 输入 st6013
 * 输出 st6015
*/
INT8U use6013find6015(CLASS_6013 st6013, CLASS_6015* st6015)
{
	INT8U result = 0;

	return result;

}


INT8U deal6015_698(CLASS_6015 st6015,BasicInfo6001 to6001)
{

	INT8U result = 0;

	return result;
}
INT8U deal6015_07(CLASS_6015 st6015,BasicInfo6001 to6001)
{
	INT8U result = 0;

	return result;
}
/*
 * 抄读1个测量点
*/
INT8U deal6015_singlemeter(CLASS_6015 st6015,BasicInfo6001 obj6001)
{
	INT8U ret = 0;
	//打开串口
	INT32S comfd = open_com_para_chg(obj6001.port,obj6001.baud,S4851);
	if(comfd<=0)
	{
		fprintf(stderr,"打开串口失败\n");
		return ret;
	}
	switch(obj6001.protocol)
	{
		case DLT_645_07:
			ret = deal6015_07(to6015,obj6001);
			break;
		default:
			ret = deal6015_698(to6015,obj6001);
	}
	return ret;
}
/*
 * 根据6013任务配置单元去文件里查找对应的6015普通采集方案
 * 输入 st6013
 * 输出 st6015
*/
INT8U readList6001FromFile(BasicInfo6001* list6001)
{
	INT8U result = 0;

	return result;
}

INT8U deal6015(CLASS_6015 st6015)
{
	INT8U result = 0;
	INT8U ret = 0;
	//全部电表
	if(st6015.ms.allmeter_null == 1)
	{
		/*
		 * 从文件里读取LIST6001SIZE个测量点
		 * */
		BasicInfo6001 list6001[LIST6001SIZE];
		memset(list6001,0,sizeof(list6001));
		ret = readList6001FromFile(list6001);
		/*
		 * 根据st6015.csd 和 list6001抄表
		 * */

		INT8U index;
		for(index = 0;index < LIST6001SIZE;index++)
		{
			deal6015_singlemeter(to6015,list6001[index]);
		}
	}
	return result;
}

void init6013()
{
	from6013.taskID = 1;
	from6013.interval.interval = 1;
	from6013.interval.units = day_units;//一天
	from6013.cjtype = norm;
	from6013.sernum = 1;//方案编号
	from6013.startime.year.data = 2016;
	from6013.startime.month.data = 9;
	from6013.startime.day.data = 12;
	from6013.startime.hour.data = 0;
	from6013.startime.min.data = 2;
	from6013.startime.sec.data = 0;
	from6013.endtime.year.data = 2099;
	from6013.endtime.month.data = 9;
	from6013.endtime.day.data = 9;
	from6013.endtime.hour.data = 9;
	from6013.endtime.min.data = 9;
	from6013.endtime.sec.data = 9;
	from6013.delay.interval = 0;
	from6013.delay.units = sec_units;
	from6013.runprio = ness;
	from6013.state = valid;
	from6013.befscript = 0;
	from6013.aftscript = 0;
	from6013.runtime.type = B_K;
	from6013.runtime.runtime[0] = 0;
	from6013.runtime.runtime[1] = 0;
	from6013.runtime.runtime[2] = 0x17;
	from6013.runtime.runtime[3] = 0x3b;
}
void init6015()
{
	to6015.sernum = 1;
	to6015.deepsize = 0x100;
	to6015.cjtype = 1;
	//to6015.data = ;//无采集内容
	to6015.csd[0].road.oad.OI = 0x5004;
	to6015.csd[0].road.oad.attflg = 2;
	to6015.csd[0].road.oad.attrindex = 0;
	to6015.csd[0].road.oads[0].OI = 0x2021;
	to6015.csd[0].road.oads[0].attflg = 2;
	to6015.csd[0].road.oads[0].attrindex = 0;
	to6015.csd[0].road.oads[1].OI = 0x0010;
	to6015.csd[0].road.oads[1].attflg = 2;
	to6015.csd[0].road.oads[1].attrindex = 0;
	to6015.csd[0].road.oads[2].OI = 0x0020;
	to6015.csd[0].road.oads[2].attflg = 2;
	to6015.csd[0].road.oads[2].attrindex = 0;
	to6015.ms.allmeter_null = 1;//所有电表
	to6015.savetimeflag = 4;
}

void read485_thread()
{
  while(1){

		init6013();
		init6015();

		CLASS_6035 result6035;//采集任务监控单元
		memset(&result6035,0,sizeof(CLASS_6035));
		result6035.taskID = from6013.taskID;
		result6035.taskState = IN_OPR;
		//result6035.startime = ;
		//result6035.endtime = ;
		INT8U ret = 0;
		CLASS_6001 to6001;
		ret = use6013find6015(from6013,&to6015);
		ret = deal6015(to6015);



	//		memset(&to6015,0,sizeof(CLASS_6015));
	//		INT8U ret = use6013find6015(from6013,&to6015);
  }
  pthread_detach(pthread_self());
  pthread_exit(&thread_read485);
}
void read485_proccess()
{
	pthread_attr_init(&read485_attr_t);
	pthread_attr_setstacksize(&read485_attr_t,2048*1024);
	pthread_attr_setdetachstate(&read485_attr_t,PTHREAD_CREATE_DETACHED);
	while ((thread_read485_id=pthread_create(&thread_read485, &read485_attr_t, (void*)read485_thread, NULL)) != 0)
	{
		sleep(1);
	}
}
