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
static int reportChoice = 0;
//是否还有更多报文标示
static int MoreContentSign = 0;
//时间任务序号
static int conformCheckId = 0;

//任务参数变更
static INT16S taskChangeSign = -1;
//对时标识
static INT16S timeChangeSign = -1;


void init6013ListFrom6012File(ProgramInfo *JProgramInfo) {

	INT16U total_autotasknum = 0;

	INT8U result = 0;
	memset(&JProgramInfo->autotask,0,sizeof(JProgramInfo->autotask));		//增加初始化
	INT16U tIndex = 0;
	OI_698 oi = 0x6013;
	CLASS_6013 class6013 = { };

	for (tIndex = 0; tIndex < 256; tIndex++) {
		if (readCoverClass(oi, tIndex, &class6013, sizeof(CLASS_6013),
				coll_para_save) == 1) {
			if(class6013.cjtype == rept)
			{
				init_autotask(total_autotasknum,class6013,JProgramInfo->autotask);
				total_autotasknum++;
			}

		}
	}
}


int ConformCheck(struct aeEventLoop* ep, long long id, void* clientData) {
    CommBlock* nst = (CommBlock*)clientData;

    //在此检查上报报文是否得到确认
    if (conformSign == 1) {
        return AE_NOMORE;
    }

    asyslog(LOG_INFO, "上报信息未得到确认,重试(%d)", conformTimes);
    //第一次调用此函数，启动任务上报


    if (conformTimes == 1) {
        stopSign    = 0;
        conformSign = 0;
        //强制确认数据帧，跳下一帧发送
        MoreContentSign = callAutoReport(reportChoice,nst, 1);
        asyslog(LOG_INFO, "强制跳下一帧，更多报文标识[%d]", MoreContentSign);
        return AE_NOMORE;
    }
    else{
    	MoreContentSign = callAutoReport(reportChoice,nst, 0);
    }
    conformTimes--;

    return conformOverTime * 1000;
}


void RegularAutoTask(struct aeEventLoop* ep, CommBlock* nst) {

    ProgramInfo* shmem = (ProgramInfo*)nst->shmem;
    if (stopSign == 1) {
        return;
    }

    if(taskChangeSign != shmem->oi_changed.oi6012 || timeChangeSign != shmem->oi_changed.oi4000){
    	asyslog(LOG_INFO, "检查到6012参数变更，或者时间变化，重新计算任务时间.");
    	init6013ListFrom6012File(shmem);
    	taskChangeSign = shmem->oi_changed.oi6012;
    	timeChangeSign = shmem->oi_changed.oi4000;
    }

    for (int i = 0; i < MAXNUM_AUTOTASK; i++) {
        //调用日常通信接口
        int res = composeAutoTask(&shmem->autotask[i]);

        if ((res == 1) || (res == 2)) {
            //第一次调用此函数，启动任务上报
        	reportChoice = res;
            MoreContentSign = callAutoReport(reportChoice,nst, 0);
            //不再调用此函数标志
            stopSign = 1;
            //标示上报任务尚未获得确认
            conformSign     = 0;
            conformTimes    = shmem->autotask[i].ReportNum;
            conformOverTime = shmem->autotask[i].OverTime;
            //注册时间事件，检查确认状态
            conformCheckId = aeCreateTimeEvent(ep, conformOverTime * 1000, ConformCheck, nst, NULL);
            asyslog(LOG_INFO, "检查到上报任务，初始化上报状态(次数=%d-时间=%d)、注册时间事件(%d)", conformTimes, conformOverTime, conformCheckId);
            break;
        }
    }
}

void ConformAutoTask(struct aeEventLoop* ep, CommBlock* nst, int res) {
    if (res == REPORT_RESPONSE && stopSign == 1) {
    	asyslog(LOG_INFO, "任务收到确认报文，发送状态置０");
        //暂时不使用分帧重复发送
        stopSign    = 0;
        conformSign = 1;

//        return;
        //有更多的报文
        MoreContentSign = callAutoReport(reportChoice,nst, 1);
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
