#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "cjcomm.h"

static INT32S timeoffset[50];    //终端精准校时 默认最近心跳时间总个数50次
static INT8U crrntimen      = 0; //终端精准校时 当前接手心跳数
static INT8U timeoffsetflag = 0; //终端精准校时 开始标志
extern CLASS_4000 class_4000;

static long long Veri_Time_Task_Id;

/*
 * 返回是否要记录精确对时心跳
 */
INT8U GetTimeOffsetFlag(void) {
    return timeoffsetflag;
}

/*
 * 根据心跳，获得K值
 */
INT32S Getk(LINK_Response link,ProgramInfo* JProgramInfo)
{
    TS T4;
    TSGet(&T4); //集中器接收时间
    TS T1;
    TS T2;
    TS T3;
    T1.Year   = link.request_time.year;
    T1.Month  = link.request_time.month;
    T1.Day    = link.request_time.day_of_month;
    T1.Hour   = link.request_time.hour;
    T1.Minute = link.request_time.minute;
    T1.Sec    = link.request_time.second;
    T1.Week   = link.request_time.day_of_week;

    T2.Year   = link.reached_time.year;
    T2.Month  = link.reached_time.month;
    T2.Day    = link.reached_time.day_of_month;
    T2.Hour   = link.reached_time.hour;
    T2.Minute = link.reached_time.minute;
    T2.Sec    = link.reached_time.second;
    T2.Week   = link.reached_time.day_of_week;

    T3.Year   = link.response_time.year;
    T3.Month  = link.response_time.month;
    T3.Day    = link.response_time.day_of_month;
    T3.Hour   = link.response_time.hour;
    T3.Minute = link.response_time.minute;
    T3.Sec    = link.response_time.second;
    T3.Week   = link.response_time.day_of_week;
    fprintf(stderr,"T2:%d-%d-%d %d:%d:%d \n",T2.Year,T2.Month,T2.Day,T2.Hour,T2.Minute,T2.Sec);
    fprintf(stderr,"T1:%d-%d-%d %d:%d:%d \n",T1.Year,T1.Month,T1.Day,T1.Hour,T1.Minute,T1.Sec);
    fprintf(stderr,"T4:%d-%d-%d %d:%d:%d \n",T4.Year,T4.Month,T4.Day,T4.Hour,T4.Minute,T4.Sec);
    fprintf(stderr,"T3:%d-%d-%d %d:%d:%d \n",T3.Year,T3.Month,T3.Day,T3.Hour,T3.Minute,T3.Sec);
    INT32S U              = difftime(tmtotime_t(T2), tmtotime_t(T1));
    INT32S V              = difftime(tmtotime_t(T4), tmtotime_t(T3));
    INT32S K              = (U - V) / 2;
    fprintf(stderr,"U=%d V=%d k=%d\n",U,V,K);
    return K;
}

/*
 * 进行对时并产生对时事件
 */
void Event_VerifiTime(ProgramInfo* JProgramInfo,INT32S avg){
	TS jzqtime;
	TSGet(&jzqtime); //集中器时间
	DateTimeBCD DT,DT_B;
	//对时前时间
	DT_B.year.data  = jzqtime.Year;
	DT_B.month.data = jzqtime.Month;
	DT_B.day.data   = jzqtime.Day;
	DT_B.hour.data  = jzqtime.Hour;
	DT_B.min.data   = jzqtime.Minute;
	DT_B.sec.data   = jzqtime.Sec;
	tminc(&jzqtime, sec_units, avg);
	//对时后时间
	DT.year.data  = jzqtime.Year;
	DT.month.data = jzqtime.Month;
	DT.day.data   = jzqtime.Day;
	DT.hour.data  = jzqtime.Hour;
	DT.min.data   = jzqtime.Minute;
	DT.sec.data   = jzqtime.Sec;
	setsystime(DT);
	//产生对时事件
	Event_3114(DT_B, JProgramInfo);
}

/*
 * GPRS上线，判断是否要进行简单对时
 */
void First_VerifiTime(LINK_Response linkResponse, ProgramInfo* JProgramInfo){
	static INT8U first_flag=1;
	if(first_flag){
		first_flag=0;
		INT32S avg=Getk(linkResponse,JProgramInfo);
		fprintf(stderr,"gprs上线，进行简单对时.....\n");
		readCoverClass(0x4000,0,&class_4000,sizeof(CLASS_4000),para_vari_save);
		fprintf(stderr,"判断是否要进行简单对时，avg=%d delay=d \n",avg,class_4000.delay);
		if(abs(avg)>=class_4000.delay){
			Event_VerifiTime(JProgramInfo,avg);
		}
	}
}

/*
 * 精确对时
 */
int VerifiTime(struct aeEventLoop* ep, long long id, void* clientData) {
    ProgramInfo* JProgramInfo = (ProgramInfo*)clientData;

    static INT8U oi4000_flag=0;
    static INT8U first =0;
   // fprintf(stderr,"精确对时 参数 type=%d hearbeatnum=%d \n",class_4000.type,class_4000.hearbeatnum);
    if((oi4000_flag != JProgramInfo->oi_changed.oi4000) || first==0){
    	first=1;
    	readCoverClass(0x4000,0,&class_4000,sizeof(CLASS_4000),para_vari_save);
    	oi4000_flag = JProgramInfo->oi_changed.oi4000;
    }
    //判断是否到开始记录心跳时间
    if (class_4000.type == 1 && class_4000.hearbeatnum > 0) {
        //有效总个数是否小于主站设置得最少有效个数
        INT8U lastn = class_4000.hearbeatnum - class_4000.tichu_max - class_4000.tichu_min;
        if (lastn < class_4000.num_min || lastn == 0)
            return 0;
        if (crrntimen < class_4000.hearbeatnum)
            timeoffsetflag = 1; //开始标志 记录心跳
        else
            timeoffsetflag = 0;
        //判断是否需要计算相关偏差值  记录完毕 达到主站设置得最大记录数
        if (timeoffsetflag == 0 && crrntimen == class_4000.hearbeatnum) {
            getarryb2s(timeoffset, crrntimen);
            INT32S allk = 0;
            INT8U index = 0;
            for (index = class_4000.tichu_max; index < (crrntimen - class_4000.tichu_min); index++) {
                allk += timeoffset[index];
                fprintf(stderr,"index=%d timeoffset=%d \n",index,timeoffset[index]);
            }
            fprintf(stderr,"allk=%d lastn=%d \n",allk,lastn);
            INT32S avg = allk / lastn;
            fprintf(stderr,"avg=%d delay=%d .... \n",avg, class_4000.delay);
            //如果平均偏差值大于等于主站设置得阀值进行精确校时
            if (abs(avg) >= class_4000.delay) {
            	Event_VerifiTime(JProgramInfo,avg);
            }
            memset(timeoffset, 0, 50);
            crrntimen      = 0;
            timeoffsetflag = 0;
        }
    }
    return 500;
}



/*
 * 获得k值并记录相关心跳次数
 */
void Getk_curr(LINK_Response link, ProgramInfo* JProgramInfo){
	INT32S avg=Getk(link,JProgramInfo);
	timeoffset[crrntimen] = avg;
	crrntimen++;
	if (crrntimen == class_4000.hearbeatnum)
		timeoffsetflag = 0;
}

/*
 * 供外部使用的初始化函数，并开启维护循环
 */
int StartVerifiTime(struct aeEventLoop* ep, long long id, void* clientData) {
    Veri_Time_Task_Id = aeCreateTimeEvent(ep, 1000, VerifiTime, clientData, NULL);
    asyslog(LOG_INFO, "精确校时事件注册完成(%lld)", Veri_Time_Task_Id);
    return 1;
}
