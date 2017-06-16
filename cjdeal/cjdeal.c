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
#include "dlt698def.h"
#include "basedef.h"
#include "ctrl.h"

extern INT32S 			spifp_rn8209;
extern INT32S 			spifp;

extern INT8S use6013find6015or6017(INT8U cjType,INT16U fanganID,TI interval6013,CLASS_6015* st6015);
extern INT8U checkMeterType(MY_MS mst,INT8U usrType,TSA usrAddr);

ProgramInfo* JProgramInfo=NULL;
int ProIndex=0;
INT8U poweroffon_state = 0; //停上电抄读标志 0无效，1抄读，2抄读完毕
MeterPower MeterPowerInfo[POWEROFFON_NUM]; //当poweroffon_state为1时，抄读其中ERC3106State=1得tsa得停上电时间，
                                           //赋值给结构体中得停上电时间，同时置VALID为1,全部抄完后，置poweroffon_state为2
/*********************************************************
 *程序入口函数-----------------------------------------------------------------------------------------------------------
 *程序退出前处理，杀死其他所有进程 清楚共享内存
 **********************************************************/
void QuitProcess()
{
	spi_close(spifp);
	spi_close(spifp_rn8209);
	close_named_sem(SEMNAME_SPI0_0);
    shmm_unregister("ProgramInfo", sizeof(ProgramInfo));
	read485QuitProcess();
	//proinfo->ProjectID=0;
    //fprintf(stderr,"\n退出：%s %d",proinfo->ProjectName,proinfo->ProjectID);
	fprintf(stderr,"\n cjdeal 退出");
	exit(0);
}
/*******************************************************
 * 清死亡计数
 */
void clearcount(int index) {
//	fprintf(stderr,"\n  cjdeal pid=%d  JProgramInfo->Projects[%d].WaitTimes = %d    ",JProgramInfo->Projects[index].ProjectID,index,JProgramInfo->Projects[index].WaitTimes);
//	fprintf(stderr,"\n  cjdeal prog name = %s\n",JProgramInfo->Projects[index].ProjectName);
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
		if(*prginfo!=NULL) {
			fprintf(stderr,"\n%s start",(*prginfo)->Projects[ProIndex].ProjectName);
			(*prginfo)->Projects[ProIndex].ProjectID=getpid();//保存当前进程的进程号
			fprintf(stderr,"ProjectID[%d]=%d\n",ProIndex,(*prginfo)->Projects[ProIndex].ProjectID);
		}
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
	return 0;
}

INT8U time_in_shiduan(TASK_RUN_TIME str_runtime,TI interval) {
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
			if ((now_min > min_start) && (now_min < min_end))
			{
				return 1;
			}
			else if ((str_runtime.type  == 0)&& (now_min == min_start))
			{
				return 1;
			}
			else if ((str_runtime.type  == 1)&& (now_min == min_end))
			{
				return 1;
			}
			else if ((str_runtime.type  == 2)&& ((now_min == min_end)||(now_min == min_start)))
			{
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
	if (time_in_shiduan(list6013[taskIndex].basicInfo.runtime,list6013[taskIndex].basicInfo.interval) == 1)	//在抄表时段内
	{
		return 1;
	}
	return 0;
}

/*
 * 比较当前时间应该先抄读哪一个任务
 * 比较权重 优先级 >  采集类型（年>月>日>分） >方案类型 > run_flg
 * 返回
 * ：0-优先级一样
 * ：1-taskIndex1先执行
 * ：2-taskIndex2先执行
 * */
INT8U cmpTaskPrio(INT16U taskIndex1, INT16U taskIndex2) {

	if (list6013[taskIndex1].basicInfo.runprio > list6013[taskIndex2].basicInfo.runprio)
	{
		return 2;
	}
	else if (list6013[taskIndex1].basicInfo.runprio < list6013[taskIndex2].basicInfo.runprio)
	{
		return 1;
	}
	else if (list6013[taskIndex1].basicInfo.interval.units> list6013[taskIndex2].basicInfo.interval.units)
	{
		return 1;
	}
	else if (list6013[taskIndex1].basicInfo.interval.units < list6013[taskIndex2].basicInfo.interval.units)
	{
		return 2;
	}
	else if(list6013[taskIndex1].basicInfo.cjtype > list6013[taskIndex2].basicInfo.cjtype)
	{
		return 1;
	}
	else if(list6013[taskIndex1].basicInfo.cjtype < list6013[taskIndex2].basicInfo.cjtype)
	{
		return 2;
	}
	else if (list6013[taskIndex1].run_flg > list6013[taskIndex2].run_flg)
	{
		return 1;
	}
	else if (list6013[taskIndex1].run_flg < list6013[taskIndex2].run_flg)
	{
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
	//	fprintf(stderr, "\n ---------list6013[%d].basicInfo.taskID = %d ",
	//			tIndex, list6013[tIndex].basicInfo.taskID);
		//run_flg > 0说明应该抄读还没有抄
		if (list6013[tIndex].run_flg > 0) {
	//		fprintf(stderr, "\n  getNextTastIndexIndex-2222");
			list6013[tIndex].run_flg++;
		} else {
			//过滤任务无效或者不再抄表时段内的
			if (filterInvalidTask(tIndex) == 0) {
	//			fprintf(stderr, "\n  getNextTastIndexIndex-3333");
				continue;
			}

			time_t timenow = time(NULL);
	//		fprintf(stderr, "\n timenow = %d ts_next = %d",timenow, list6013[tIndex].ts_next);
			if(timenow >= list6013[tIndex].ts_next)
			{
				list6013[tIndex].run_flg = 1;
	//		fprintf(stderr, "\n  getNextTastIndexIndex-4444");
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
			//	fprintf(stderr, "\n  getNextTastIndexIndex-5555");
				taskIndex = tIndex;
			}
			continue;
		}

		if (cmpTaskPrio(taskIndex, tIndex) == 2) {
		//	fprintf(stderr, "\n  getNextTastIndexIndex-6666");
			taskIndex = tIndex;
			continue;
		}
	}
	return taskIndex;
}
/*
 * 判断portOAD是否属于485 port485 口
 * */
INT8U is485OAD(OAD portOAD,INT8U port485)
{
	fprintf(stderr,"\n portOAD.OI = %04x portOAD.attflg = %d  portOAD.attrindex = %d port485 = %d \ n"
			,portOAD.OI,portOAD.attflg,portOAD.attrindex,port485);
	if ((portOAD.OI != 0xF201) || (portOAD.attflg != 0x02)
			|| (portOAD.attrindex != port485)) {
		return 0;
	}
	return 1;
}
/*
 * 读取table6000 填充info6000  此结构体保存了每一个485口上有那些测量点
 * 抄表是根据此结构体读取测量点信息
 * */
INT8S init6000InfoFrom6000FIle()
{

	memset(&info6000,0,2*sizeof(INFO_6001_LIST));
	INT8U tIndex = 0;
	for(tIndex = 0;tIndex < infoReplenish.tasknum;tIndex++)
	{
		memset(infoReplenish.unitReplenish[tIndex].list6001,0,2*sizeof(INFO_6001_LIST));
		memset(infoReplenish.unitReplenish[tIndex].isSuccess,0,2*MAX_REPLENISH_TASK_NUM);
	}
	INT8S result = -1;
	INT16U meterIndex = 0;
	CLASS_6001 meter = { };
	INT32U index = 0;
	int recordnum = 0;
	INT16U oi = 0x6000;

	recordnum = getFileRecordNum(oi);
	if (recordnum == -1) {
		fprintf(stderr, "未找到OI=%04x的相关信息配置内容！！！\n", 6000);
		return result;
	} else if (recordnum == -2) {
		fprintf(stderr, "采集档案表不是整数，检查文件完整性！！！\n");
		return result;
	}
//	fprintf(stderr, "\n init6000InfoFrom6000FIle recordnum = %d ", recordnum);
	/*
	 * 根据st6015.csd 和 list6001抄表
	 * */

	for(index=0;index<recordnum;index++)
	{
		if(readParaClass(oi,&meter,index)==1)
		{
			if(meter.sernum!=0 && meter.sernum!=0xffff)
			{

				if(is485OAD(meter.basicinfo.port,1) == 1)
				{
					meterIndex = info6000[0].meterSum;
					info6000[0].list6001[meterIndex] = meter.sernum;
					info6000[0].meterSum++;
					if(info6000[0].meterSum > MAX_METER_NUM_1_PORT)
					{
					    asyslog(LOG_WARNING, "485 1测量点超数量");
						return result;
					}
				}
				if(is485OAD(meter.basicinfo.port,2) == 1)
				{
					meterIndex = info6000[1].meterSum;
					info6000[1].list6001[meterIndex] = meter.sernum;
					info6000[1].meterSum++;
					if(info6000[0].meterSum > MAX_METER_NUM_1_PORT)
					{
					    asyslog(LOG_WARNING, "485 2测量点超数量");
						return result;
					}
				}
			}
		}
	}
//	fprintf(stderr,"485 1口测量点数量 = %d   485 2口测量点数量 = %d",info6000[0].meterSum,info6000[1].meterSum);

	for(tIndex = 0;tIndex < infoReplenish.tasknum;tIndex++)
	{
		memcpy(&infoReplenish.unitReplenish[tIndex].list6001[0],&info6000[0],sizeof(INFO_6001_LIST));
		memcpy(&infoReplenish.unitReplenish[tIndex].list6001[1],&info6000[1],sizeof(INFO_6001_LIST));
	}
	return result;
}
INT8S saveClass6035(CLASS_6035* class6035)
{
	INT8U isFind = 0;
	INT8S ret = -1;
	int recordNum = getFileRecordNum(0x6035);
	CLASS_6035 file6035;
	memset(&file6035,0,sizeof(CLASS_6035));
	INT16U i;
	for(i=0;i<=recordNum;i++)
	{
		if(readCoverClass(0x6035,i,&file6035,sizeof(CLASS_6035),coll_para_save)== 1)
		{
			if(file6035.taskID == class6035->taskID)
			{
				isFind = 1;
				break;
			}
		}
	}
	if(isFind)
	{
		memcpy(&class6035->starttime,&file6035.starttime,sizeof(DateTimeBCD));
		class6035->totalMSNum += file6035.totalMSNum;
		class6035->successMSNum += file6035.successMSNum;
		class6035->sendMsgNum += file6035.sendMsgNum;
		class6035->rcvMsgNum += file6035.rcvMsgNum;
	}

	saveCoverClass(0x6035, class6035->taskID, class6035,
			sizeof(CLASS_6035), coll_para_save);


	return ret;
}
//集中器调时间　需要重新处理任务开始时间：如果是向前对时需要重新计算任务下一次开始时间，向后对时就不用了
INT8U deal6013_onPara4000changed()
{
	fprintf(stderr,"\ndeal6013_onPara4000changed--------------------start\n");
	INT8U ret = 1;
	INT16U tIndex;
	time_t time_now;
	time_now = time(NULL);//当前时间
	//普通任务
	for (tIndex = 0; tIndex < total_tasknum; tIndex++)
	{
		if((list6013[tIndex].basicInfo.taskID > 0)&&(time_now < list6013[tIndex].ts_next))
		{
			fprintf(stderr,"\n12313123123123\n");
			list6013[tIndex].ts_next  =
					calcnexttime(list6013[tIndex].basicInfo.interval,list6013[tIndex].basicInfo.startime,list6013[tIndex].basicInfo.delay);
		}
	}
	fprintf(stderr,"\ndeal6013_onPara4000changed--------------------end\n");


	return ret;
}

/*
 * 从文件里把所有的任务单元读上来
 * */
INT8U init6013ListFrom6012File() {
	INT16U tIndex = 0;

	for(tIndex = 0;tIndex < infoReplenish.tasknum;tIndex++)
	{
		infoReplenish.unitReplenish[tIndex].taskID = 0;
		memset(infoReplenish.unitReplenish[tIndex].isSuccess,0,2*MAX_REPLENISH_TASK_NUM);
	}
	infoReplenish.tasknum = 0;

	total_tasknum = 0;
	//list6013  初始化下一次抄表时间
	TS ts_now;
	TSGet(&ts_now);

	fprintf(stderr, "\n \n-------------init6013ListFrom6012File---------------start\n");
	INT8U result = 0;
	memset(list6013, 0, TASK6012_MAX * sizeof(TASK_CFG));


	OI_698 oi = 0x6013;
	CLASS_6013 class6013 = { };

	for (tIndex = 0; tIndex < TASK6012_MAX; tIndex++) {
		if (readCoverClass(oi, tIndex, &class6013, sizeof(CLASS_6013),
				coll_para_save) == 1) {
			//print6013(list6013[tIndex]);
			if(class6013.cjtype != rept)
			{
				memcpy(&list6013[total_tasknum].basicInfo, &class6013, sizeof(CLASS_6013));

				TS taskStartTime;
				TimeBCDToTs(list6013[total_tasknum].basicInfo.startime,&taskStartTime);
				INT8U timeCmp = TScompare(ts_now,taskStartTime);
#if 0
				asyslog(LOG_NOTICE,"当前时间 %04d-%02d-%02d %02d:%02d:%02d\n",
						ts_now.Year,ts_now.Month,ts_now.Day,ts_now.Hour,
						ts_now.Minute,ts_now.Sec);

				asyslog(LOG_NOTICE,"任务开始时间 %04d-%02d-%02d %02d:%02d:%02d\n",
						taskStartTime.Year,taskStartTime.Month,taskStartTime.Day,taskStartTime.Hour,
						taskStartTime.Minute,taskStartTime.Sec);
#endif
				//把需要补抄的任务放进infoReplenish
				CLASS_6015 st6015;
				memset(&st6015,0,sizeof(CLASS_6015));
				if (readCoverClass(0x6015, list6013[total_tasknum].basicInfo.sernum,&st6015, sizeof(CLASS_6015), coll_para_save)== 1)
				{
					if(st6015.csds.csd[0].csd.road.oad.OI == 0x5004)
					{
						infoReplenish.unitReplenish[infoReplenish.tasknum++].taskID = list6013[total_tasknum].basicInfo.taskID;
					}
				}
#if 1
				if(timeCmp < 2)
				{
					list6013[total_tasknum].ts_next  = tmtotime_t(ts_now);
				}
				else
#endif
				{
					list6013[total_tasknum].ts_next  =
									calcnexttime(list6013[total_tasknum].basicInfo.interval,list6013[total_tasknum].basicInfo.startime,list6013[total_tasknum].basicInfo.delay);
				}

				//TODO
				total_tasknum++;

				//任务初始化新建6035
				CLASS_6035 result6035;	//采集任务监控单元
				memset(&result6035,0,sizeof(CLASS_6035));
				result6035.taskState = BEFORE_OPR;
				result6035.taskID = class6013.taskID;
				saveClass6035(&result6035);

			}
		}
	}
	fprintf(stderr, "\n \n-------------init6013ListFrom6012File---------------start\n");
	return result;
}
INT8U getParaChangeType()
{
	INT8U ret = para_no_chg;
	static INT8U lastchgoi6000=0;
	static INT8U lastchgoi6012=0;
	static INT8U lastchgoi6014=0;

	static INT8U lastchgoi4000=0;
	static INT8U lastchgoi4204=0;
	static INT8U lastchgoi4300=0;
	static INT8U first=1;
	if(first)
	{
		first=0;
		lastchgoi6000 = JProgramInfo->oi_changed.oi6000;
		lastchgoi6012 = JProgramInfo->oi_changed.oi6012;
		lastchgoi6014 = JProgramInfo->oi_changed.oi6014;
		lastchgoi4204 = JProgramInfo->oi_changed.oi4204;
		lastchgoi4300 = JProgramInfo->oi_changed.oi4300;
		CLASS19 class19;
		memset(&class19,0,sizeof(CLASS19));
		readCoverClass(0x4300,0,&class19,sizeof(class19),para_vari_save);
		isAllowReport = class19.active_report;
		return ret;
	}
	if(lastchgoi4300 != JProgramInfo->oi_changed.oi4300)
	{
		lastchgoi4300 = JProgramInfo->oi_changed.oi4300;
		fprintf(stderr,"\n 测量点参数4300变更");
		CLASS19 class19;
		memset(&class19,0,sizeof(CLASS19));
		readCoverClass(0x4300,0,&class19,sizeof(class19),para_vari_save);
		isAllowReport = class19.active_report;
	}
	if(lastchgoi6000 != JProgramInfo->oi_changed.oi6000)
	{
		ret = ret|para_6000_chg;
		fprintf(stderr,"\n 测量点参数6000变更");
		lastchgoi6000 = JProgramInfo->oi_changed.oi6000;
	}
	if(lastchgoi6012 != JProgramInfo->oi_changed.oi6012)
	{
		ret = ret|para_6012_chg;
		fprintf(stderr,"\n 任务参数6012变更");
		lastchgoi6012= JProgramInfo->oi_changed.oi6012;
	}
	if(lastchgoi6014 != JProgramInfo->oi_changed.oi6014)
	{
		ret = ret|para_6014_chg;
		fprintf(stderr,"\n 采集方案参数6014变更");
		lastchgoi6014= JProgramInfo->oi_changed.oi6014;
	}

	if(lastchgoi4000 != JProgramInfo->oi_changed.oi4000)
	{
		ret = ret|para_4000_chg;
		fprintf(stderr,"\n 时间参数4000变更");
		lastchgoi4000= JProgramInfo->oi_changed.oi4000;
	}

	if(lastchgoi4204 != JProgramInfo->oi_changed.oi4204)
	{
		ret = ret|para_4204_chg;
		fprintf(stderr,"\n 终端广播校时参数4204变更");
		lastchgoi4204= JProgramInfo->oi_changed.oi4204;
	}

	return ret;
}
/*
 * 读取终端广播校时参数
 * */
INT8S init4204Info()
{
	INT8S ret = -1;
	memset(&broadcase4204,0,sizeof(CLASS_4204));
	if(readCoverClass(0x4204,0,&broadcase4204,sizeof(CLASS_4204),para_vari_save)==1)
	{
		flagDay_4204[0] = 1;
		flagDay_4204[1] = 1;
	}
	return ret;
}
INT8U findfake6001(INT8U tIndex,CLASS_6001* meter)
{
	INT8S ret = -1;
	INT16U meterIndex = 0;
	CLASS_6015 to6015;	//采集方案集
	memset(&to6015, 0, sizeof(CLASS_6015));
	memset(meter,0,sizeof(CLASS_6001));

	ret = use6013find6015or6017(list6013[tIndex].basicInfo.cjtype,list6013[tIndex].basicInfo.sernum,list6013[tIndex].basicInfo.interval,&to6015);
	if(ret == 1)
	{
		INT8U portIndex = 0;
		for(portIndex = 0;portIndex < 2;portIndex++)
		{
			for (meterIndex = 0; meterIndex < info6000[portIndex].meterSum; meterIndex++)
			{
				if (readParaClass(0x6000, meter, info6000[portIndex].list6001[meterIndex]) == 1)
				{
					if (meter->sernum != 0 && meter->sernum != 0xffff)
					{
						if (checkMeterType(to6015.mst, meter->basicinfo.usrtype,meter->basicinfo.addr))
						{
							fprintf(stderr,"\n 找到任务对应测量点　%d portIndex = %d meterIndex = %d",meter->sernum,portIndex,meterIndex);
							return 1;
						}
					}
				}
			}
		}
	}
	return ret;
}
INT8U createFakeTaskFileHead()
{
	CSD_ARRAYTYPE csds;
	char	fname[FILENAMELEN]={};

	INT8U taskinfoflg=0;
	TASKSET_INFO tasknor_info;
	INT16U headlen=0,unitlen=0,unitnum=0,runtime=0;//runtime执行次数

	INT8U dataContent[DATA_CONTENT_LEN];
	memset(dataContent,0,DATA_CONTENT_LEN);
	TS ts_cc;
	TSGet(&ts_cc);
	DateTimeBCD startTime;
	DataTimeGet(&startTime);

	CLASS_6001 meter;
	memset(&meter,0,sizeof(CLASS_6001));
	INT8U tIndex;
	for (tIndex = 0; tIndex < total_tasknum; tIndex++)
	{
		if ((list6013[tIndex].basicInfo.cjtype == norm)&&(list6013[tIndex].basicInfo.interval.units < day_units))
		{
			if(findfake6001(tIndex,&meter)==-1)
			{
				continue;
			}

			memset(dataContent,0,DATA_CONTENT_LEN);
			taskinfoflg=0;
			memset(&tasknor_info,0,sizeof(TASKSET_INFO));
			memset(&csds,0x00,sizeof(ROAD));

			if((taskinfoflg = ReadTaskInfo(list6013[tIndex].basicInfo.taskID,&tasknor_info))==0)
			{
				continue;
			}
			runtime = tasknor_info.runtime;
			memcpy(&csds,&tasknor_info.csds,sizeof(CSD_ARRAYTYPE));

			if(taskinfoflg == 2)//月冻结
			{
				ts_cc.Day = 0;
				ts_cc.Hour = 0;
				ts_cc.Minute = 0;
				ts_cc.Sec = 0;
				asyslog(LOG_WARNING, "月冻结存储:%d",ts_cc.Month);
			}

			getTaskFileName(list6013[tIndex].basicInfo.taskID,ts_cc,fname);
			CreateSaveHead(fname,NULL,csds,&headlen,&unitlen,&unitnum,runtime,1);//写文件头信息并返回
			INT16U index = 0;
			dataContent[index++] = dttsa;
			memcpy(&dataContent[index],meter.basicinfo.addr.addr,sizeof(TSA));//采集通信地址
			index += sizeof(TSA);
			index += fill_date_time_s(&dataContent[index],&startTime);
			index += fill_date_time_s(&dataContent[index],&startTime);
			index += fill_date_time_s(&dataContent[index],&startTime);

			SaveNorData(list6013[tIndex].basicInfo.taskID,NULL,dataContent,unitlen/runtime,ts_cc);
		}
	}
	return 1;
}
void timeProcess()
{
	static TS lastTime;
	static INT8U firstFlag = 1;

	TS nowTime;
	TSGet(&nowTime);

	if(firstFlag)
	{
		lastTime.Year = nowTime.Year;
		lastTime.Month = nowTime.Month;
		lastTime.Day = nowTime.Day;
		lastTime.Hour = nowTime.Hour;
		lastTime.Minute = nowTime.Minute;
		lastTime.Sec = nowTime.Sec;
		firstFlag = 0;
	}
	else
	{
		//跨天处理
		if(lastTime.Day != nowTime.Day)
		{
			asyslog(LOG_WARNING,"集中器跨天");
			flagDay_4204[0] = 1;
			flagDay_4204[1] = 1;

			lastTime.Day = nowTime.Day;

			isReplenishOver[0] = 1;
			isReplenishOver[1] = 1;
			isReplenishOver[2] = 1;
			isReplenishOver[3] = 1;


			para_change485[0] = 1;
			para_change485[1] = 1;



			INT8U taskIndex = 0;
			for(taskIndex = 0;taskIndex < infoReplenish.tasknum;taskIndex++)
			{
				memset(infoReplenish.unitReplenish[taskIndex].isSuccess,0,2*MAX_METER_NUM_1_PORT);
			}
			createFakeTaskFileHead();
		}
	}
}

INT8S dealMsgProcess()
{
	INT8S result = 0;
	if((cjcommProxy.isInUse != 0) ||(cjguiProxy.isInUse != 0))
	{
		fprintf(stderr,"\n ％％％％％％％％％％％％cjcommProxy.isInUse = %d cjguiProxy.isInUse = %d\n",cjcommProxy.isInUse,cjguiProxy.isInUse);
		return 	result;
	}

	INT8U  rev_485_buf[2048];
	INT32S ret;

	mmq_head mq_h;
	ret = mmq_get(mqd_485_main, 1, &mq_h, rev_485_buf);

	if (ret>0)
	{
		switch(mq_h.cmd)
		{
			case ProxyGetResponseList://代理
			{

				if(mq_h.pid == cjdeal)
				{
					fprintf(stderr, "\n收到代理召测\n");
					if(cjcommProxy.isInUse == 0)
					{
						memcpy(&cjcommProxy.strProxyList,rev_485_buf,sizeof(PROXY_GETLIST));
						cjcommProxy.isInUse = 3;
					}
					else
					{
						fprintf(stderr,"上一个代理召测没处理完");
					}

				}
				if(mq_h.pid == cjgui)
				{
					fprintf(stderr, "\n收到液晶点抄-----------------------------------23232323\n");
					if(cjguiProxy.isInUse == 0)
					{
						memcpy(&cjguiProxy.strProxyMsg,rev_485_buf,sizeof(Proxy_Msg));
						cjguiProxy.isInUse = 3;
					}
					else
					{
						fprintf(stderr,"上一个液晶点抄没处理完");
					}

				}

				readState = 0;
			}
			break;
			default:
			{
				asyslog(LOG_WARNING,"485收到未知消息  cmd=%d!!!---------------", mq_h.cmd);
			}

		}

	}
	return result;
}
void replenish_tmp()
{
	TS nowTime;
	TSGet(&nowTime);
	INT16U nowMin = nowTime.Hour*60 + nowTime.Minute;
	INT8U tmpIndex = 0;
	for(tmpIndex = 0;tmpIndex < 4;tmpIndex++)
	{
		if((isReplenishOver[tmpIndex] == 1)&&(nowMin >= replenishTime[tmpIndex]))
		{
			asyslog(LOG_WARNING,"第%d次补抄　时间%d分 补抄任务数量=%d",tmpIndex,replenishTime[tmpIndex],infoReplenish.tasknum);
			INT8U tIndex = 0;
			for(tIndex = 0;tIndex < infoReplenish.tasknum;tIndex++)
			{
				INT8U findIndex;
				for (findIndex = 0; findIndex < total_tasknum; findIndex++)
				{
#if 0
					fprintf(stderr,"\nlist6013[%d].basicInfo.taskID = %d infoReplenish.unitReplenish[%d].taskID = %d\n",
							findIndex,tIndex,
							list6013[findIndex].basicInfo.taskID,infoReplenish.unitReplenish[tIndex].taskID);
#endif
					if (list6013[findIndex].basicInfo.taskID == infoReplenish.unitReplenish[tIndex].taskID)
					{
						asyslog(LOG_WARNING,"发送补抄任务ID tIndex = %d　",tIndex);
						INT8S ret = mqs_send((INT8S *)TASKID_485_2_MQ_NAME,cjdeal,1,(INT8U *)&findIndex,sizeof(INT16S));
						ret = mqs_send((INT8S *)TASKID_485_1_MQ_NAME,cjdeal,1,(INT8U *)&findIndex,sizeof(INT16S));
					}
				}
			}
			isReplenishOver[tmpIndex] = 0;
		}
	}

}
void dispatch_thread()
{
	//运行调度任务进程
//	fprintf(stderr,"\ndispatch_thread start \n");
	memset(isReplenishOver,0,4);
	replenishTime[0] = 30;
	replenishTime[1] = 60;
	replenishTime[2] = 90;
	replenishTime[3] = 120;
	while(1)
	{
		timeProcess();
		if(mqd_485_main >= 0)
		{
			dealMsgProcess();
		}
		para_ChangeType = getParaChangeType();

		if(para_ChangeType&para_6000_chg)
		{
			para_change485[0] = 1;
			para_change485[1] = 1;
			init6000InfoFrom6000FIle();
#if 0
			filewrite(REPLENISHFILEPATH,&infoReplenish,sizeof(Replenish_TaskInfo));
#endif
		}
		if(para_ChangeType&para_4000_chg)
		{
			deal6013_onPara4000changed();
		}

		if(para_ChangeType&para_6012_chg)
		{
			para_change485[0] = 1;
			para_change485[1] = 1;
			init6013ListFrom6012File();
#if 0
			filewrite(REPLENISHFILEPATH,&infoReplenish,sizeof(Replenish_TaskInfo));
#endif
			system("rm -rf /nand/para/6035");
		}
		if(para_ChangeType&para_4204_chg)
		{
			init4204Info();
		}
		if(para_change485[0]||para_change485[1])
		{
			fprintf(stderr,"参数变更等待 485线程处理无效线程");
			sleep(1);
			continue;
		}
		INT16S tastIndex = -1;//读取所有任务文件
		tastIndex = getNextTastIndexIndex();
		sleep(2);
		if (tastIndex > -1)
		{
#if 0
			DbgPrintToFile1(port,"dispatch_thread　taskIndex = %d 任务开始",taskIndex);
#endif
			//计算下一次抄读此任务的时间;
			list6013[tastIndex].ts_next = calcnexttime(list6013[tastIndex].basicInfo.interval,list6013[tastIndex].basicInfo.startime,list6013[tastIndex].basicInfo.delay);

			INT8S ret = mqs_send((INT8S *)TASKID_485_2_MQ_NAME,cjdeal,1,(INT8U *)&tastIndex,sizeof(INT16S));
			fprintf(stderr,"\n 向485 2线程发送任务ID = %d \n",ret);
			ret = mqs_send((INT8S *)TASKID_485_1_MQ_NAME,cjdeal,1,(INT8U *)&tastIndex,sizeof(INT16S));
			fprintf(stderr,"\n 向485 1线程发送任务ID = %d \n",ret);
			ret = mqs_send((INT8S *)TASKID_plc_MQ_NAME,cjdeal,1,(INT8U *)&tastIndex,sizeof(INT16S));
			//TODO
			list6013[tastIndex].run_flg = 0;

		}
		else
		{
			//补抄
			replenish_tmp();
		}

		sleep(1);
	}
	  pthread_detach(pthread_self());
	  pthread_exit(&thread_dispatchTask);
}
void dispatchTask_proccess()
{
	//读取所有任务文件		TODO：参数下发后需要更新内存值
	init6013ListFrom6012File();
	init6000InfoFrom6000FIle();
#if 0
	fileread(REPLENISHFILEPATH,&infoReplenish,sizeof(Replenish_TaskInfo));
#endif
	init4204Info();
#ifdef TESTDEF
	fprintf(stderr,"\n补抄内容:\n");
	INT8U tIndex = 0;
	for(tIndex = 0;tIndex < infoReplenish.tasknum;tIndex++)
	{
		fprintf(stderr,"\n任务taskID = %d--------------\n",infoReplenish.unitReplenish[tIndex].taskID);
		INT16U mpIndex = 0;

		for(mpIndex = 0;mpIndex < infoReplenish.unitReplenish[tIndex].list6001[0].meterSum;mpIndex++)
		{
			fprintf(stderr,"\n测量点＝%d  isSuccess = %d",
					infoReplenish.unitReplenish[tIndex].list6001[0].list6001[mpIndex],
					infoReplenish.unitReplenish[tIndex].isSuccess[0][mpIndex]);
		}
		for(mpIndex = 0;mpIndex < infoReplenish.unitReplenish[tIndex].list6001[1].meterSum;mpIndex++)
		{
			fprintf(stderr,"\n测量点＝%d  isSuccess = %d",
					infoReplenish.unitReplenish[tIndex].list6001[1].list6001[mpIndex],
					infoReplenish.unitReplenish[tIndex].isSuccess[1][mpIndex]);
		}
	}
#endif
	para_change485[0] = 0;
	para_change485[1] = 0;

	struct mq_attr attr_485_main;
	mqd_485_main = mmq_open((INT8S *)PROXY_485_MQ_NAME,&attr_485_main,O_RDONLY);

	memset(&cjcommProxy,0,sizeof(CJCOMM_PROXY));
	memset(&cjguiProxy,0,sizeof(GUI_PROXY));

	pthread_attr_init(&dispatchTask_attr_t);
	pthread_attr_setstacksize(&dispatchTask_attr_t, 2048 * 1024);
	pthread_attr_setdetachstate(&dispatchTask_attr_t, PTHREAD_CREATE_DETACHED);

	while ((thread_dispatchTask_id = pthread_create(&thread_dispatchTask,&dispatchTask_attr_t, (void*) dispatch_thread,NULL)) != 0)
	{
		sleep(1);
	}
}

/*********************************************************
 * 主进程
 *********************************************************/
int main(int argc, char *argv[])
{
//	printf("a\n");
	//return ctrl_base_test();
	int del_day = 0,del_min = 0;
	TS ts;

	pid_t pids[128];
    struct sigaction sa = {};
    Setsig(&sa, QuitProcess);

    if (prog_find_pid_by_name((INT8S*)argv[0], pids) > 1)
		return EXIT_SUCCESS;

	fprintf(stderr,"\n[cjdeal]:cjdeal run!");
	if(InitPro(&JProgramInfo,argc,argv)==0){
		syslog(LOG_ERR,"进程 %s 参数错误",argv[0]);
		return EXIT_FAILURE;
	}

	asyslog(LOG_INFO,"进程 %s PID = %d",JProgramInfo->Projects[1].ProjectName,JProgramInfo->Projects[1].ProjectID);
	asyslog(LOG_INFO,"进程 %s PID = %d",JProgramInfo->Projects[2].ProjectName,JProgramInfo->Projects[2].ProjectID);
	//载入档案、参数
	InitPara();
	//任务调度进程
	dispatchTask_proccess();
	//485、四表合一
	read485_proccess();
	//统计计算 电压合格率 停电事件等
	calc_proccess();
	if(JProgramInfo->cfg_para.device == CCTT1)
	{
		//载波
		readplc_proccess();
	}
	if(JProgramInfo->cfg_para.device != CCTT2)
	{
		//液晶、控制
		guictrl_proccess();
	}
	//交采
	acs_process();

	while(1)
   	{
	    struct timeval start={}, end={};
	    long  interval=0;
		gettimeofday(&start, NULL);
		TSGet(&ts);
		if (ts.Hour==15 && ts.Minute==5 && del_day != ts.Day && del_min != ts.Minute)
		{
			deloutofdatafile();
			del_day = ts.Day;
			del_min = 0;
			asyslog(LOG_INFO,"判断删除过期文件");
		}
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
