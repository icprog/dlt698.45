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
CLASS_4000 class_4000;

static long long Veri_Time_Task_Id;

INT8U GetTimeOffsetFlag(void) {
    return timeoffsetflag;
}

int VerifiTime(struct aeEventLoop* ep, long long id, void* clientData) {
    ProgramInfo* JProgramInfo = (ProgramInfo*)clientData;
    TS jzqtime;
    TSGet(&jzqtime); //集中器时间
    static INT8U oi4000_flag=0;

    if(oi4000_flag != JProgramInfo->oi_changed.oi4000){
    	readCoverClass(0x4000,0,&class_4000,sizeof(CLASS_4000),para_vari_save);
    	oi4000_flag = JProgramInfo->oi_changed.oi4000;
    }
    //判断是否到开始记录心跳时间
    if (class_4000.type == 1 && class_4000.hearbeatnum > 0) {
    	fprintf(stderr,"开始精确对时.... \n");
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
            for (index = class_4000.tichu_max; index < (crrntimen - class_4000.tichu_min + 1); index++) {
                allk += timeoffset[index];
            }
            INT32S avg = allk / lastn;
            fprintf(stderr,"avg=%d delay=%d .... \n",avg, class_4000.delay);
            //如果平均偏差值大于等于主站设置得阀值进行精确校时
            if (abs(avg) >= class_4000.delay) {
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
            memset(timeoffset, 0, 50);
            crrntimen      = 0;
            timeoffsetflag = 0;
        }
    }
    return 500;
}

void Getk(LINK_Response link, ProgramInfo* JProgramInfo) {
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

    INT32S U              = difftime(tmtotime_t(T2), tmtotime_t(T1));
    INT32S V              = difftime(tmtotime_t(T4), tmtotime_t(T3));
    INT32S K              = (U - V) / 2;
    timeoffset[crrntimen] = K;
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
