#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <wait.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <sys/reboot.h>
#include <bits/types.h>
#include <bits/sigaction.h>

#include "cjdeal.h"
#include "read485.h"
#include "readplc.h"
#include "guictrl.h"
#include "acs.h"
#include "event.h"
#include "calc.h"
#include "ParaDef.h"
#include "EventObject.h"

ProgramInfo* JProgramInfo=NULL;
int ProIndex=0;
INT8U poweroffon_state = 0; //停上电抄读标志 0无效，1抄读，2抄读完毕
MeterPower MeterPowerInfo[POWEROFFON_NUM]; //当poweroffon_state为1时，抄读其中ERC3106State=1得tsa得停上电时间，
                                           //赋值给结构体中得停上电时间，同时置VALID为1,全部抄完后，置poweroffon_state为2
/*********************************************************
 *程序入口函数-----------------------------------------------------------------------------------------------------------
 *程序退出前处理，杀死其他所有进程 清楚共享内存
 **********************************************************/
void QuitProcess(ProjectInfo *proinfo)
{
	close_named_sem(SEMNAME_SPI0_0);
	proinfo->ProjectID=0;
    fprintf(stderr,"\n退出：%s %d",proinfo->ProjectName,proinfo->ProjectID);
	exit(0);
}
/*******************************************************
 * 清死亡计数
 */
void clearcount(int index) {
    JProgramInfo->Projects[index].WaitTimes = 0;
}
/*********************************************************
 * 进程初始化
 *********************************************************/
int InitPro(ProgramInfo** prginfo, int argc, char *argv[])
{
	if (argc >= 2)
	{
		*prginfo = OpenShMem("ProgramInfo",sizeof(ProgramInfo),NULL);
		ProIndex = atoi(argv[1]);
		fprintf(stderr,"\n%s start",(*prginfo)->Projects[ProIndex].ProjectName);
		(*prginfo)->Projects[ProIndex].ProjectID=getpid();//保存当前进程的进程号
		return 1;
	}
	return 0;
}

/********************************************************
 * 载入档案、参数
 ********************************************************/
int InitPara()
{
	InitACSPara();
	read_oif203_para();		//开关量输入值读取
	return 0;
}

INT8U time_in_shiduan(TASK_RUN_TIME str_runtime) {
	TS ts_now;
	TSGet(&ts_now);

	INT16U min_start, min_end, now_min;	//距离0点0分
	now_min = ts_now.Hour * 60 + ts_now.Minute;
	INT8U timePartIndex = 0;
	for (timePartIndex = 0; timePartIndex < str_runtime.num; timePartIndex++)
	{
		min_start = str_runtime.runtime[timePartIndex].beginHour * 60
				+ str_runtime.runtime[timePartIndex].beginMin;
		min_end = str_runtime.runtime[timePartIndex].endHour * 60
				+ str_runtime.runtime[timePartIndex].endMin;
		if (min_start <= min_end) {
			if ((now_min > min_start) && (now_min < min_end)) {
				return 1;
			} else if (((str_runtime.type & 0x01) == 0x01)
					&& (now_min == min_end)) {
				return 1;
			} else if (((str_runtime.type & 0x03) == 0x01)
					&& (now_min == min_start)) {
				return 1;
			}
		}
	}
	return 0;
}
//时间在任务开始结束时间段内 0:任务开始 1：任务不执行
INT8U time_in_task(CLASS_6013 from6012_curr) {
	struct tm tm_start;
	struct tm tm_end;
	struct tm tm_curr;
	if (from6012_curr.startime.year.data < 1900
			|| from6012_curr.startime.month.data < 1
			|| from6012_curr.endtime.year.data < 1900
			|| from6012_curr.endtime.month.data < 1) {
		fprintf(stderr, "\n time_in_task - 1");
		return 1;	//无效，任务不执行
	}

	memset(&tm_start, 0x00, sizeof(struct tm));
	tm_start.tm_year = from6012_curr.startime.year.data - 1900;
	tm_start.tm_mon = from6012_curr.startime.month.data - 1;
	tm_start.tm_mday = from6012_curr.startime.day.data;
	tm_start.tm_hour = from6012_curr.startime.hour.data;
	tm_start.tm_min = from6012_curr.startime.min.data;
	tm_start.tm_sec = from6012_curr.startime.sec.data;

	memset(&tm_end, 0x00, sizeof(struct tm));
	tm_end.tm_year = from6012_curr.endtime.year.data - 1900;
	tm_end.tm_mon = from6012_curr.endtime.month.data - 1;
	tm_end.tm_mday = from6012_curr.endtime.day.data;
	tm_end.tm_hour = from6012_curr.endtime.hour.data;
	tm_end.tm_min = from6012_curr.endtime.min.data;
	tm_end.tm_sec = from6012_curr.endtime.sec.data;

	time_t curr_time_t = time(NULL );
	localtime_r(&curr_time_t, &tm_curr);
#if 0
	fprintf(stderr,"\n start year = %d mon = %d day = %d hour=%d  min=%d",
			tm_start.tm_year,tm_start.tm_mon,tm_start.tm_mday,tm_start.tm_hour,tm_start.tm_min);
	fprintf(stderr,"\n end year = %d mon = %d day = %d hour=%d  min=%d",
			tm_end.tm_year,tm_end.tm_mon,tm_end.tm_mday,tm_end.tm_hour,tm_end.tm_min);
	fprintf(stderr,"\n curr year = %d mon = %d day = %d hour=%d  min=%d",
			tm_curr.tm_year,tm_curr.tm_mon,tm_curr.tm_mday,tm_curr.tm_hour,tm_curr.tm_min);
#endif
	if ((tm_curr.tm_year >= tm_start.tm_year)
			&& (tm_curr.tm_year <= tm_end.tm_year)) {
		if (tm_start.tm_year == tm_end.tm_year) {
			tm_start.tm_year = 0;
			tm_end.tm_year = 0;
			tm_curr.tm_year = 0;
			time_t currsec = mktime(&tm_curr);
			time_t startsec = mktime(&tm_start);
			time_t endsec = mktime(&tm_end);
			if ((currsec >= startsec) && (currsec <= endsec)) {
				return 0;
			} else {
				return 1;
			}
		} else {
			return 0;
		}
	} else {
		return 1;
	}
	return 0;
}

/*
 * 过滤掉
 * 状态不对
 * 时段不符合
 * 要求的任务
 *
 * */
INT8U filterInvalidTask(INT16U taskIndex) {

	if (list6013[taskIndex].basicInfo.taskID == 0) {
		fprintf(stderr, "\n filterInvalidTask - 1");
		return 0;
	}
	if (list6013[taskIndex].basicInfo.state == task_novalid)	//任务无效
			{
		fprintf(stderr, "\n filterInvalidTask - 2");
		return 0;
	}
	if (time_in_task(list6013[taskIndex].basicInfo) == 1)	//不在任务执行时段内
	{
		fprintf(stderr, "\n filterInvalidTask - 3");
		return 0;
	}
	if (time_in_shiduan(list6013[taskIndex].basicInfo.runtime) == 1)	//在抄表时段内
	{
		return 1;
	}
	return 0;
}

/*
 * 比较当前时间应该先抄读哪一个任务
 * 比较权重 优先级 >  采集类型（年>月>日>分） > run_flg
 * 返回
 * ：0-优先级一样
 * ：1-taskIndex1先执行
 * ：2-taskIndex2先执行
 * */
INT8U cmpTaskPrio(INT16U taskIndex1, INT16U taskIndex2) {

	if (list6013[taskIndex1].basicInfo.runprio
			> list6013[taskIndex2].basicInfo.runprio) {
		return 1;
	} else if (list6013[taskIndex1].basicInfo.runprio
			< list6013[taskIndex2].basicInfo.runprio) {
		return 2;
	} else if (list6013[taskIndex1].basicInfo.interval.units
			> list6013[taskIndex2].basicInfo.interval.units) {
		return 1;
	} else if (list6013[taskIndex1].basicInfo.interval.units
			< list6013[taskIndex2].basicInfo.interval.units) {
		return 2;
	} else if (list6013[taskIndex1].run_flg > list6013[taskIndex2].run_flg) {
		return 1;
	} else if (list6013[taskIndex1].run_flg < list6013[taskIndex2].run_flg) {
		return 2;
	}
	return 0;
}
//查找下一个执行的任务
INT16S getNextTastIndexIndex() {
	INT16S taskIndex = -1;
	INT16U tIndex = 0;

	for (tIndex = 0; tIndex < total_tasknum; tIndex++)
	{
		if (list6013[tIndex].basicInfo.taskID == 0) {
			continue;
		}
		fprintf(stderr, "\n ---------list6013[%d].basicInfo.taskID = %d ",
				tIndex, list6013[tIndex].basicInfo.taskID);
		//run_flg > 0说明应该抄读还没有抄
		if (list6013[tIndex].run_flg > 0) {
			fprintf(stderr, "\n  getNextTastIndexIndex-2222");
			list6013[tIndex].run_flg++;
		} else {
			//过滤任务无效或者不再抄表时段内的
			if (filterInvalidTask(tIndex) == 0) {
				fprintf(stderr, "\n  getNextTastIndexIndex-3333");
				continue;
			}

			time_t timenow = time(NULL);
			if(timenow >= list6013[tIndex].ts_next)
			{
				list6013[tIndex].run_flg = 1;
				fprintf(stderr, "\n  getNextTastIndexIndex-4444");
			}
			else
			{
				continue;
			}
		}

		if (taskIndex == -1)
		{
			if(list6013[tIndex].run_flg > 0)
			{
				fprintf(stderr, "\n  getNextTastIndexIndex-5555");
				taskIndex = tIndex;
			}
			continue;
		}

		if (cmpTaskPrio(taskIndex, tIndex) == 2) {
			fprintf(stderr, "\n  getNextTastIndexIndex-6666");
			taskIndex = tIndex;
			continue;
		}
	}
	return taskIndex;
}

/*
 * 从文件里把所有的任务单元读上来
 * */
INT8U init6013ListFrom6012File() {

	total_tasknum = 0;

	//list6013  初始化下一次抄表时间
	TS ts_now;
	TSGet(&ts_now);

	fprintf(stderr, "\n -------------init6013ListFrom6012File---------------");
	INT8U result = 0;
	memset(list6013, 0, TASK6012_MAX * sizeof(TASK_CFG));
	INT16U tIndex = 0;
	OI_698 oi = 0x6013;
	CLASS_6013 class6013 = { };
	for (tIndex = 0; tIndex < TASK6012_MAX; tIndex++) {
		if (readCoverClass(oi, tIndex, &class6013, sizeof(CLASS_6013),
				coll_para_save) == 1) {

			//print6013(list6013[tIndex]);
			if(class6013.cjtype == rept)
			{
				init_autotask(class6013,JProgramInfo->autotask);
			}
			else
			{
				memcpy(&list6013[total_tasknum].basicInfo, &class6013, sizeof(CLASS_6013));
				time_t timenow = time(NULL);
				list6013[total_tasknum].ts_next  = timenow;

				total_tasknum++;
			}


		}
	}

	return result;
}

void dispatch_thread()
{
	//运行调度任务进程
	fprintf(stderr,"\ndispatch_thread start \n");


	while(1)
	{
		INT16S tastIndex = -1;//读取所有任务文件
		tastIndex = getNextTastIndexIndex();

		if (tastIndex > -1)
		{
			fprintf(stderr, "\n\n\n\n*************任务开始执行 ************ tastIndexIndex = %d taskID = %d*****************\n",
					tastIndex, list6013[tastIndex].basicInfo.taskID);
			//计算下一次抄读此任务的时间;
			list6013[tastIndex].ts_next = calcnexttime(list6013[tastIndex].basicInfo.interval,list6013[tastIndex].basicInfo.startime);

			INT8S ret = mqs_send((INT8S *)TASKID_485_2_MQ_NAME,1,1,(INT8U *)&tastIndex,sizeof(INT16S));
			fprintf(stderr,"\n 向485 2线程发送任务ID = %d \n",ret);
			ret = mqs_send((INT8S *)TASKID_485_1_MQ_NAME,1,1,(INT8U *)&tastIndex,sizeof(INT16S));
			fprintf(stderr,"\n 向485 1线程发送任务ID = %d \n",ret);
			//TODO
			list6013[tastIndex].run_flg = 0;

		}
		else
		{
			//fprintf(stderr,"\n当前无任务执行\n");
		}

		sleep(5);
	}


}
void dispatchTask_proccess()
{
	//读取所有任务文件		TODO：参数下发后需要更新内存值
	init6013ListFrom6012File();

	pthread_attr_init(&dispatchTask_attr_t);
	pthread_attr_setstacksize(&dispatchTask_attr_t, 2048 * 1024);
	pthread_attr_setdetachstate(&dispatchTask_attr_t, PTHREAD_CREATE_DETACHED);

	while ((thread_read4851_id = pthread_create(&thread_dispatchTask,&dispatchTask_attr_t, (void*) dispatch_thread,NULL)) != 0)
	{
		sleep(1);
	}
}

/*********************************************************
 * 主进程
 *********************************************************/
int main(int argc, char *argv[])
{
	pid_t pids[128];
    struct sigaction sa = {};
    Setsig(&sa, QuitProcess);

    if (prog_find_pid_by_name((INT8S*)argv[0], pids) > 1)
		return EXIT_SUCCESS;

	fprintf(stderr,"\n[cjdeal]:cjdeal run!");
	if(InitPro(&JProgramInfo,argc,argv)==0){
		fprintf(stderr,"进程 %s 参数错误",argv[0]);
		return EXIT_FAILURE;
	}


	//载入档案、参数
	InitPara();
	//任务调度进程
	dispatchTask_proccess();
	//485、四表合一
	read485_proccess();
	//统计计算 电压合格率 停电事件等
	calc_proccess();
	//载波
	//readplc_proccess();
	//液晶、控制
	//guictrl_proccess();
	//交采
	acs_process();

	while(1)
   	{
	    struct timeval start={}, end={};
	    long  interval=0;
		gettimeofday(&start, NULL);
		DealState(JProgramInfo);
		gettimeofday(&end, NULL);
		interval = 1000000*(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec);
	    if(interval>=1000000)
	    	fprintf(stderr,"deal main interval = %f(ms)\n", interval/1000.0);
		usleep(10 * 1000);
		clearcount(ProIndex);
   	}
	close_named_sem(SEMNAME_SPI0_0);
	return EXIT_SUCCESS;//退出
}
