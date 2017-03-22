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

//停止日常上报检测标志
static int stopSign = 0;
//上报确认标志
static int conformSign     = 0;
static int conformTimes    = 0;
static int conformOverTime = 0;
//是否还有更多报文标示
static int MoreContentSign = 0;
//时间任务序号
static int conformCheckId = 0;

int ConformCheck(struct aeEventLoop* ep, long long id, void* clientData) {
    CommBlock* nst = (CommBlock*)clientData;

    //在此检查上报报文是否得到确认
    if (conformSign == 1) {
        return AE_NOMORE;
    }

    asyslog(LOG_INFO, "上报信息未得到确认,重试(%d)", conformTimes);
    //第一次调用此函数，启动任务上报
    MoreContentSign = callAutoReport(nst, 0);

    if (conformTimes == 0) {
        stopSign    = 0;
        conformSign = 0;
        //强制确认数据帧，跳下一帧发送
        MoreContentSign = callAutoReport(nst, 1);
        return AE_NOMORE;
    }
    conformTimes--;

    return conformOverTime * 1000;
}

void RegularAutoTask(struct aeEventLoop* ep, CommBlock* nst) {
    ProgramInfo* shmem = (ProgramInfo*)nst->shmem;
    if (stopSign == 1) {
        return;
    }

    for (int i = 0; i < MAXNUM_AUTOTASK; i++) {
        //调用日常通信接口
        int res = composeAutoTask(&shmem->autotask[i]);
        if (res == 2) {
            //第一次调用此函数，启动任务上报
            MoreContentSign = callAutoReport(nst, 0);
            //不再调用此函数标志
            stopSign = 1;
            //标示上报任务尚未获得确认
            conformSign     = 0;
            conformTimes    = shmem->autotask[i].ReportNum;
            conformOverTime = shmem->autotask[i].OverTime;
            //注册时间事件，检查确认状态
//            conformCheckId = aeCreateTimeEvent(ep, conformOverTime * 1000, ConformCheck, nst, NULL);
            asyslog(LOG_INFO, "检查到上报任务，初始化上报状态(次数=%d-时间=%d)、注册时间事件(%d)", conformTimes, conformOverTime, conformCheckId);
            break;
        }
    }
}

void ConformAutoTask(struct aeEventLoop* ep, CommBlock* nst, int res) {
    if (res == 8 || stopSign == 1) {
        //有更多的报文
        MoreContentSign = callAutoReport(nst, 1);
        if (MoreContentSign == 1) {
            asyslog(LOG_INFO, "发现更多的报文，注销之前的时间检查函数，任务序号(%d)", conformCheckId);
            aeDeleteTimeEvent(ep, conformCheckId);
            conformCheckId = aeCreateTimeEvent(ep, conformOverTime * 1000, ConformCheck, nst, NULL);
            asyslog(LOG_INFO, "重新注册时间事件，任务序号(%d)", conformCheckId);
        } else {
            stopSign    = 0;
            conformSign = 1;
        }
    }
}
