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
#include "dlt698def.h"
#include "cjcomm.h"
#include "AccessFun.h"
#include "PublicFunction.h"

//红外口监听、处理结构体
static CommBlock FirObject;
static long long Serial_Task_Id;

/*
 * 模块*内部*使用的初始化参数
 */
void IfrInit(void) {
    asyslog(LOG_INFO, "初始化红外模块...");
    initComPara(&FirObject);
}

/*
 * 用于程序退出时调用
 */
void IfrDestory(void) {
    //关闭资源
    asyslog(LOG_INFO, "关闭红外通信接口(%d)", FirObject.phy_connect_fd);
    close(FirObject.phy_connect_fd);
    FirObject.phy_connect_fd = -1;
}

/*
 * 模块维护循环
 */
int RegularIfr(struct aeEventLoop* ep, long long id, void* clientData) {
    CommBlock* nst = (CommBlock*)clientData;
    if (nst->phy_connect_fd < 0) {
        if ((nst->phy_connect_fd = OpenCom(3, 2400, (unsigned char*)"even", 1, 8)) <= 0) {
            asyslog(LOG_ERR, "红外串口打开失败");
        } else {
            if (aeCreateFileEvent(ep, nst->phy_connect_fd, AE_READABLE, GenericRead, nst) < 0) {
                asyslog(LOG_ERR, "红外串口监听失败");
                close(nst->phy_connect_fd);
                nst->phy_connect_fd = -1;
                return 10 * 1000;
            }
        }
    }
    return 1000;
}

/*
 * 供外部使用的初始化函数，并开启维护循环
 */
int StartIfr(struct aeEventLoop* ep, long long id, void* clientData) {
    IfrInit();
    Serial_Task_Id = aeCreateTimeEvent(ep, 1000, RegularIfr, &FirObject, NULL);
    asyslog(LOG_INFO, "红外串口时间事件注册完成(%lld)", Serial_Task_Id);
    return 1;
}
