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
	newfd = OpenCom(port,baud,(unsigned char *)"even",1,8);

	return newfd;
}

/*
 * 根据6013任务配置单元去文件里查找对应的6015普通采集方案
 * 输入 st6013
 * 输出 st6015
*/
INT8U use6013find6015(INT16U taskID, CLASS_6015* st6015)
{
	INT8U result = 0;

	return result;

}


INT8U deal6015_698(CLASS_6015 st6015,BasicInfo6001 to6001)
{
	INT8U result = 0;
	INT8U sendbuff[BUFFSIZE];
	memset(sendbuff,0,BUFFSIZE);

	result = composeProtocol698_GetRequest(sendbuff,st6015,to6001.addr);

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
	comfd4851 = open_com_para_chg(obj6001.port,obj6001.baud,comfd4851);
	if(comfd4851<=0)
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
 * 从文件里读取LIST6001SIZE个测量点
 * */
INT8U readList6001FromFile(BasicInfo6001* list6001,INT16U groupIndex,int recordnum)
{
	INT16U		oi = 0x6001;
	INT8U result = 0;
	INT8U mIndex = 0;
	int endIndex;
	CLASS_6001	 meter={};
	if(((groupIndex+1)*LIST6001SIZE) > recordnum)
	{
		endIndex = recordnum;
	}
	else
	{
		endIndex = (groupIndex+1)*LIST6001SIZE;
	}


	for(mIndex = groupIndex*LIST6001SIZE;mIndex < endIndex;mIndex++)
	{
		if(readParaClass(oi,&meter,mIndex)==1)
		{
			if(meter.sernum!=0 && meter.sernum!=0xffff)
			{
				if(meter.basicinfo.port.OI == 0xF201)
				{
					fprintf(stderr,"\n序号:%d ",meter.sernum);

					list6001[mIndex%LIST6001SIZE].port = S4851;
					list6001[mIndex%LIST6001SIZE].baud = meter.basicinfo.baud;
					list6001[mIndex%LIST6001SIZE].protocol = meter.basicinfo.protocol;
					memcpy(&list6001[mIndex%LIST6001SIZE].addr,&meter.basicinfo.addr,sizeof(TSA));
					memcpy(&list6001[mIndex%LIST6001SIZE].addr,&meter.basicinfo.addr,sizeof(TSA));
				}
				else
				{
					fprintf(stderr,"非485测量点 %04X",meter.basicinfo.port.OI);
				}
			}
		}
	}

	return result;
}

/*
 * 处理一个普通采集方案
 * */
INT8U deal6015(CLASS_6015 st6015)
{
	INT8U result = 0;
	BasicInfo6001 list6001[LIST6001SIZE];
	int recordnum = 0;
	//全部电表
	if(st6015.ms.allmeter_null == 1)
	{
		INT16U		oi = 0x6001;
		recordnum = getFileRecordNum(oi);
		if(recordnum == -1)
		{
			fprintf(stderr,"未找到OI=%04x的相关信息配置内容！！！\n",6000);
			return result;
		}else if(recordnum == -2)
		{
			fprintf(stderr,"采集档案表不是整数，检查文件完整性！！！\n");
			return result;
		}

		/*
		 * 根据st6015.csd 和 list6001抄表
		 * */
		INT16U groupNum = (recordnum/LIST6001SIZE) + 1;
		INT16U groupindex;
		INT8U mpIndex;
		for(groupindex = 0;groupindex < groupNum;groupindex++)
		{
			memset(list6001,0,sizeof(list6001));
			result = readList6001FromFile(list6001,groupindex,recordnum);
			for(mpIndex = 0;mpIndex < LIST6001SIZE;mpIndex++)
			{
				deal6015_singlemeter(to6015,list6001[mpIndex]);
			}
		}


	}
	return result;
}

//void get_request(INT8U *cjtype,INT8U oad_num,INT8U piid,INT8U *oad,INT8U TP)
//{
//	int i=0;
//	INT16U sendindex=0;
//	INT8U sendbuf[512];
//
//	//报文头以后组 todo
//	sendbuf[sendindex++] = 0x05;
//	switch (cjtype[0])
//	{
//	case 0://实时数据
//		if(oad_num > 1)
//			sendbuf[sendindex++] = 0x01;//normal
//		else
//			sendbuf[sendindex++] = 0x02;//normallist
//		sendbuf[sendindex++] = (piid+1)%256;
//		if(oad_num > 1)
//			sendbuf[sendindex++] = oad_num;//piid后面为个数
//		for(i=0;i<oad_num;i++)
//		{
//			memcpy(&sendbuf[sendindex],&oad[i][0],4);//oad为二位数组
//			sendindex += 4;
//		}
//		break;
//	case 1://日冻结 采集上第N次
//		sendbuf[sendindex++] = 0x03;//record
//		sendbuf[sendindex++] = (piid+1)%256;
//		memcpy(&sendbuf[sendindex],&oad[i][0],4);//oad为二位数组,第0个为冻结类型
//		sendindex += 4;
//		sendbuf[sendindex++] = 0x01;//选择方法为RCSD
//		break;
//	default:break;
//	}
//	sendbuf[sendindex++] = TP;
//}

void init6013()
{
	list6013[0].basicInfo.taskID = 1;
	list6013[0].basicInfo.interval.interval = 1;
	list6013[0].basicInfo.interval.units = day_units;//一天
	list6013[0].basicInfo.cjtype = norm;
	list6013[0].basicInfo.sernum = 1;//方案编号
	list6013[0].basicInfo.startime.year.data = 2016;
	list6013[0].basicInfo.startime.month.data = 9;
	list6013[0].basicInfo.startime.day.data = 12;
	list6013[0].basicInfo.startime.hour.data = 0;
	list6013[0].basicInfo.startime.min.data = 2;
	list6013[0].basicInfo.startime.sec.data = 0;
	list6013[0].basicInfo.endtime.year.data = 2099;
	list6013[0].basicInfo.endtime.month.data = 9;
	list6013[0].basicInfo.endtime.day.data = 9;
	list6013[0].basicInfo.endtime.hour.data = 9;
	list6013[0].basicInfo.endtime.min.data = 9;
	list6013[0].basicInfo.endtime.sec.data = 9;
	list6013[0].basicInfo.delay.interval = 0;
	list6013[0].basicInfo.delay.units = sec_units;
	list6013[0].basicInfo.runprio = ness;
	list6013[0].basicInfo.state =  task_valid;
	list6013[0].basicInfo.befscript = 0;
	list6013[0].basicInfo.aftscript = 0;
	list6013[0].basicInfo.runtime.type = B_K;
	list6013[0].basicInfo.runtime.runtime[0] = 0;
	list6013[0].basicInfo.runtime.runtime[1] = 0;
	list6013[0].basicInfo.runtime.runtime[2] = 0x17;
	list6013[0].basicInfo.runtime.runtime[3] = 0x3b;
}
void init6015()
{
	to6015.sernum = 1;
	to6015.deepsize = 0x100;
	to6015.cjtype = 1;
	//to6015.data = ;//无采集内容
	to6015.csd[0].rcsd[0].oad.OI = 0x5004;
	to6015.csd[0].rcsd[0].oad.attflg = 2;
	to6015.csd[0].rcsd[0].oad.attrindex = 0;
	to6015.csd[0].rcsd[0].oad.OI = 0x2021;
	to6015.csd[0].rcsd[0].oad.attflg = 2;
	to6015.csd[0].rcsd[0].oad.attrindex = 0;
	to6015.csd[0].rcsd[1].oad.OI = 0x0010;
	to6015.csd[0].rcsd[1].oad.attflg = 2;
	to6015.csd[0].rcsd[1].oad.attrindex = 0;
	to6015.csd[0].rcsd[2].oad.OI = 0x0020;
	to6015.csd[0].rcsd[2].oad.attflg = 2;
	to6015.csd[0].rcsd[2].oad.attrindex = 0;
	to6015.ms.allmeter_null = 1;//所有电表
	to6015.savetimeflag = 4;
}


//时间在任务开始结束时间段内 0:任务开始 1：任务不执行
INT8U time_in_round(CLASS_6013 from6012_curr)
{
	struct tm tm_tmp;
	time_t start_sec=0,end_sec=0,curr_sec=0;
	if(from6012_curr.startime.year.data < 1900 || from6012_curr.startime.month.data < 1 ||
			from6012_curr.endtime.year.data < 1900 || from6012_curr.endtime.month.data < 1)
		return 1;//无效，任务不执行
	memset(&tm_tmp,0x00,sizeof(struct tm));
	tm_tmp.tm_year = from6012_curr.startime.year.data - 1900;
	tm_tmp.tm_mon = from6012_curr.startime.month.data - 1;
	tm_tmp.tm_wday = from6012_curr.startime.day.data;
	tm_tmp.tm_hour = from6012_curr.startime.hour.data;
	tm_tmp.tm_min = from6012_curr.startime.min.data;
	tm_tmp.tm_sec = from6012_curr.startime.sec.data;
	start_sec = mktime(&tm_tmp);
	memset(&tm_tmp,0x00,sizeof(struct tm));
	tm_tmp.tm_year = from6012_curr.endtime.year.data - 1900;
	tm_tmp.tm_mon = from6012_curr.endtime.month.data - 1;
	tm_tmp.tm_wday = from6012_curr.endtime.day.data;
	tm_tmp.tm_hour = from6012_curr.endtime.hour.data;
	tm_tmp.tm_min = from6012_curr.endtime.min.data;
	tm_tmp.tm_sec = from6012_curr.endtime.sec.data;
	end_sec = mktime(&tm_tmp);

	if(start_sec >= end_sec)
		return 1;

	curr_sec = time(NULL);
	if(curr_sec < start_sec || curr_sec > end_sec)//抄表时段外
		return 1;

	return 0;
}

/*
 * 过滤掉
 * 状态不对
 * 时段不符合
 * 要求的任务
 *
 * */
INT8U filterInvalidTask(INT16U taskIndex)
{
	TS ts_now;
	TSGet(&ts_now);

	INT16U min_start,min_end,now_min;//距离0点0分
	if(list6013[taskIndex].basicInfo.state == task_novalid)//任务无效
		return 0;

	if(time_in_round(list6013[taskIndex].basicInfo)==1)//不在抄表时段内
		return 0;

	now_min = ts_now.Hour * 60 + ts_now.Minute;
	min_start = list6013[taskIndex].basicInfo.runtime.runtime[0] * 60 + list6013[taskIndex].basicInfo.runtime.runtime[1];//前秒数
	min_end = list6013[taskIndex].basicInfo.runtime.runtime[2] * 60 + list6013[taskIndex].basicInfo.runtime.runtime[3];//后秒数

	if(min_start >= min_end)//当日抄表时段无效
		return 0;
	if((list6013[taskIndex].basicInfo.runtime.type & 0x01) == 0x01)//后闭
	{
		if(now_min > min_end)
			return 0;
	}
	else
	{
		if(now_min >= min_end)
			return 0;
	}

	if((list6013[taskIndex].basicInfo.runtime.type & 0x03) == 0x01)//前闭
	{
		if(now_min < min_start)
			return 0;
	}
	else
	{
		if(now_min <= min_start)
			return 0;
	}
	return 1;
}
/*
 * 根据抄表时间断和抄表间隔
 * 计算当前时间应该抄第几轮
 * runtime 现在默认就一个时段
 *
 * */
int getTaskReadRounds(INT16U taskIndex)
{
	int readRound = 0;
	TS ts_now;
	TSGet(&ts_now);

	switch(list6013[taskIndex].basicInfo.interval.units)//计算本抄表时段唯一标识值
	{
		case year_units:
			if(ts_now.Year != list6013[taskIndex].ts_last.Year)
			{
				readRound += 1;
			}
			break;
		case month_units:
			if(ts_now.Month != list6013[taskIndex].ts_last.Month)
			{
				readRound += 1;
			}
			break;
		case day_units:
			if(ts_now.Day != list6013[taskIndex].ts_last.Day)
			{
				readRound += 1;
			}
			break;
		case hour_units://小时开始，标识不是唯一，不同天会产生循环的数，因此需要加上年月日判断
			//小时与上一次抄表时间的day结合使用,每天重新生成唯一标识
			if(ts_now.Year != list6013[taskIndex].ts_last.Year || ts_now.Month != list6013[taskIndex].ts_last.Month || ts_now.Day != list6013[taskIndex].ts_last.Day)
			{
				readRound = 1;
				list6013[taskIndex].resetFlag = 1;
			}
			else
				readRound = 1 + abs(ts_now.Hour - list6013[taskIndex].basicInfo.runtime.runtime[0])/list6013[taskIndex].basicInfo.interval.interval;
			break;
		case minute_units:
			if(ts_now.Year != list6013[taskIndex].ts_last.Year || ts_now.Month != list6013[taskIndex].ts_last.Month || ts_now.Day != list6013[taskIndex].ts_last.Day)
			{
				readRound = 1;
				list6013[taskIndex].resetFlag = 1;
			}
			else
			{
				INT16U minSpan = abs((ts_now.Hour*60 + ts_now.Minute) - (list6013[taskIndex].basicInfo.runtime.runtime[0]*60 + list6013[taskIndex].basicInfo.runtime.runtime[1]));
				readRound =  1 + minSpan /list6013[taskIndex].basicInfo.interval.interval;
			}
			break;
		case sec_units:
			if(ts_now.Year != list6013[taskIndex].ts_last.Year || ts_now.Month != list6013[taskIndex].ts_last.Month || ts_now.Day != list6013[taskIndex].ts_last.Day)
			{
				readRound = 1;
				list6013[taskIndex].resetFlag = 1;
			}
			else
			{
				INT16U secSpan = abs((ts_now.Hour*3600 + ts_now.Minute*60 + ts_now.Sec) -
									(list6013[taskIndex].basicInfo.runtime.runtime[0]*3600
											+ list6013[taskIndex].basicInfo.runtime.runtime[1]*60));
				readRound =  1 + secSpan /list6013[taskIndex].basicInfo.interval.interval;
			}

			break;
		default:
			break;
	}

	return readRound;
}
/*
 * 比较当前时间应该先抄读哪一个任务
 * 返回
 * ：0-优先级一样
 * ：1-taskIndex1先执行
 * ：2-taskIndex2先执行
 * */
INT8U cmpTaskPrio(INT16U taskIndex1,INT16U taskIndex2)
{
	if(list6013[taskIndex1].basicInfo.interval.units > list6013[taskIndex2].basicInfo.interval.units)
	{
		return 2;
	}
	if(list6013[taskIndex1].basicInfo.interval.units < list6013[taskIndex2].basicInfo.interval.units)
	{
		return 1;
	}
	if((getTaskReadRounds(taskIndex1) - list6013[taskIndex1].last_round) >
	(getTaskReadRounds(taskIndex2) - list6013[taskIndex2].last_round))
	{
		return 1;
	}
	if((getTaskReadRounds(taskIndex1) - list6013[taskIndex1].last_round) <
	(getTaskReadRounds(taskIndex2) - list6013[taskIndex2].last_round))
	{
		return 2;
	}
	return 0;
}
INT16S getNextTastID()
{
	INT16S taskIndex = -1;
	INT16U tIndex = 0;


	for(tIndex=0;tIndex<TASK6012_MAX;tIndex++)
	{
		//过滤任务无效或者不再抄表时段内的
		if (filterInvalidTask(tIndex)==0)
		{
			continue;
		}
		//过滤没到抄表间隔的
		if((list6013[tIndex].last_round == getTaskReadRounds(tIndex))
			&&(list6013[tIndex].resetFlag!=1))
		{
			continue;
		}
		if(taskIndex == -1)
		{
			taskIndex = tIndex;
			continue;
		}
		else if(list6013[taskIndex].basicInfo.runprio < list6013[tIndex].basicInfo.runprio)
		{
			//判断优先级
			taskIndex = tIndex;
			continue;
		}
		else
		{
			if(cmpTaskPrio(taskIndex,tIndex) == 2)
			{
				taskIndex = tIndex;
				continue;
			}
		}

	}

	return list6013[taskIndex].basicInfo.taskID;
}
/*
 * 从文件里把所有的任务单元读上来
 * */
INT8U init6013ListFrom6012File()
{
	INT8U result = 0;
	memset(list6013,0x00,TASK6012_MAX*sizeof(TASK_CFG));
	//list6013  初始化上一次抄表时间  和 抄表轮次
	TS ts_now;
	TSGet(&ts_now);
	INT16U tIndex = 0;
	for(tIndex = 0;tIndex < TASK6012_MAX;tIndex++)
	{
		list6013[tIndex].ts_last.Year = ts_now.Year;
		list6013[tIndex].ts_last.Month = ts_now.Month;
		list6013[tIndex].ts_last.Day = ts_now.Day;
		list6013[tIndex].ts_last.Hour = ts_now.Hour;
		list6013[tIndex].ts_last.Minute = ts_now.Minute;
		list6013[tIndex].last_round = 0;
	}

	return result;
}
void read485_thread(void* i485port)
{
	INT8U port = *(INT8U*)i485port;
	comfd4851 = -1;
	INT8U ret = 0;
	INT16S taskID = -1;
	init6013ListFrom6012File();

	init6013();
	init6015();
	while(1)
	{

		taskID = getNextTastID();

		if(taskID > -1)
		{
			fprintf(stderr,"\n taskID = %d",taskID);
			CLASS_6035 result6035;//采集任务监控单元
			memset(&result6035,0,sizeof(CLASS_6035));
			result6035.taskID = taskID;
			result6035.taskState = IN_OPR;
			DataTimeGet(&result6035.starttime);


			ret = use6013find6015(taskID,&to6015);
			ret = deal6015(to6015);
			DataTimeGet(&result6035.endtime);
			result6035.taskState = AFTER_OPR;
		}
		else
		{
			fprintf(stderr,"\n 当前无任务可执行");
		}
		sleep(1);
	}

	pthread_detach(pthread_self());
	if(port==1)
	{
		pthread_exit(&thread_read4851);
	}

	if(port==2)
	{
		pthread_exit(&thread_read4852);
	}

	sleep(1);


}
void read485_proccess()
{
	INT8U i485port1 = 1;
	INT8U i485port2 = 2;
	pthread_attr_init(&read485_attr_t);
	pthread_attr_setstacksize(&read485_attr_t,2048*1024);
	pthread_attr_setdetachstate(&read485_attr_t,PTHREAD_CREATE_DETACHED);
	while ((thread_read4851_id=pthread_create(&thread_read4851, &read485_attr_t, (void*)read485_thread, &i485port1)) != 0)
	{
		sleep(1);
	}
	while ((thread_read4852_id=pthread_create(&thread_read4852, &read485_attr_t, (void*)read485_thread, &i485port2)) != 0)
	{
		sleep(1);
	}
}

