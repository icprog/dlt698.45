#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ae.h"
#include "anet.h"
#include "dlt698def.h"
#include "cjcomm.h"
#include "AccessFun.h"
#include "PublicFunction.h"

static INT32S timeoffset[50];    //终端精准校时 默认最近心跳时间总个数50次
static INT8U crrntimen      = 0; //终端精准校时 当前接手心跳数
static INT8U timeoffsetflag = 0; //终端精准校时 开始标志

static long long Veri_Time_Task_Id;

INT8U GetTimeOffsetFlag(void) {
    return timeoffsetflag;
}

int VerifiTime(struct aeEventLoop* ep, long long id, void* clientData) {
    ProgramInfo* JProgramInfo = (ProgramInfo*)clientData;
    TS jzqtime;
    TSGet(&jzqtime); //集中器时间

    //判断是否到开始记录心跳时间
    if (JProgramInfo->t_timeoffset.type == 1 && JProgramInfo->t_timeoffset.totalnum > 0) {
        //有效总个数是否小于主站设置得最少有效个数
        INT8U lastn = JProgramInfo->t_timeoffset.totalnum - JProgramInfo->t_timeoffset.maxn - JProgramInfo->t_timeoffset.minn;
        if (lastn < JProgramInfo->t_timeoffset.lastnum || lastn == 0)
            return 0;
        if (crrntimen < JProgramInfo->t_timeoffset.totalnum)
            timeoffsetflag = 1; //开始标志 记录心跳
        else
            timeoffsetflag = 0;
        //判断是否需要计算相关偏差值  记录完毕 达到主站设置得最大记录数
        if (timeoffsetflag == 0 && crrntimen == JProgramInfo->t_timeoffset.totalnum) {
            getarryb2s(timeoffset, crrntimen);
            INT32S allk = 0;
            INT8U index = 0;
            for (index = JProgramInfo->t_timeoffset.maxn - 1; index < (crrntimen - JProgramInfo->t_timeoffset.minn); index++) {
                allk += timeoffset[index];
            }
            INT32S avg = allk / lastn;
            //如果平均偏差值大于等于主站设置得阀值进行精确校时
            if (abs(avg) >= JProgramInfo->t_timeoffset.timeoffset) {
                TSGet(&jzqtime); //集中器时间
                tminc(&jzqtime, sec_units, avg);
                DateTimeBCD DT;
                DT.year.data  = jzqtime.Year;
                DT.month.data = jzqtime.Month;
                DT.day.data   = jzqtime.Day;
                DT.hour.data  = jzqtime.Hour;
                DT.min.data   = jzqtime.Minute;
                DT.sec.data   = jzqtime.Sec;
                setsystime(DT);
                //产生对时事件
                Event_3114(DT, JProgramInfo);
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
    if (crrntimen == JProgramInfo->t_timeoffset.totalnum)
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
