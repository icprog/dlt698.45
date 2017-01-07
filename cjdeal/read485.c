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
#include "read485.h"


void read485_thread()
{
  while(1){

  }
  pthread_detach(pthread_self());
  pthread_exit(&thread_read485);
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
/*
 * 具体抄读6015中配置的数据项
 * 输入 st6013
 * 输出 st6015
*/
INT8U deal6015_698protocol(CLASS_6015 st6015)
{
	INT8U result = 0;

	return result;

}
void read485_proccess()
{
	pthread_attr_init(&read485_attr_t);
	pthread_attr_setstacksize(&read485_attr_t,2048*1024);
	pthread_attr_setdetachstate(&read485_attr_t,PTHREAD_CREATE_DETACHED);
	while ((thread_read485_id=pthread_create(&thread_read485, &read485_attr_t, (void*)read485_thread, NULL)) != 0)
	{


		CLASS_6013 from6013;
		CLASS_6015 to6015;
		memset(&to6015,0,sizeof(CLASS_6015));


		CLASS_6035 result6035;//采集任务监控单元
		memset(&result6035,0,sizeof(CLASS_6035));
		result6035.taskID = from6013.taskID;
		result6035.taskState = IN_OPR;
		//result6035.startime = ;
		//result6035.endtime = ;
		INT8U ret = 0;
		ret = use6013find6015(from6013,&to6015);
		ret = deal6015_698protocol(to6015);



		sleep(1);
	}
}
