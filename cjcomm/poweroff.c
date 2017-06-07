//
// Created by 周立海 on 2017/6/2.
//

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
#include "../include/basedef.h"

static long long Power_Off_Task_Id;

int getPowerOffState(ProgramInfo *JProgramInfo) {
    int off_flag = 0;

    //II型
    if (JProgramInfo->cfg_para.device == CCTT2) {
    	off_flag = pwr_has_byVolt(JProgramInfo->ACSRealData.Available,JProgramInfo->ACSRealData.Ua,1300);
    } else {
        BOOLEAN gpio_5V = pwr_has();
        if ((JProgramInfo->ACSRealData.Ua < 100) && (JProgramInfo->ACSRealData.Ub < 100) &&
            (JProgramInfo->ACSRealData.Uc < 100) && (!gpio_5V)) {
            off_flag = 1;
        }
    }
    return off_flag;
}


int CheckPowerOff(struct aeEventLoop *ep, long long id, void *clientData) {
    static int count = 0;

    if (getPowerOffState(clientData) == 1) {
        count++;
    } else {
        count = 0;
    }

    if (count > 60) {
        while (getPowerOffState(clientData) == 1) {
            sleep(2);
            asyslog(LOG_INFO, "检测到设备掉电一分钟，停止所有通信...");
        }
        asyslog(LOG_INFO, "检测到复电，继续通信...");
        count = 0;
    }

    return 1000;
}

/*
 * 开启停电检测事件
 */
int StartPowerOff(struct aeEventLoop *ep, long long id, void *clientData) {
    Power_Off_Task_Id = aeCreateTimeEvent(ep, 1000, CheckPowerOff, clientData, NULL);
    asyslog(LOG_INFO, "停电检测事件注册完成(%lld)", Power_Off_Task_Id);
    return 1;
}
