#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "cjcomm.h"
#include "../libMq/libmmq.h"
#include "../include/StdDataType.h"

//建立消息监听服务
static long long Mmq_Task_Id;
static struct mq_attr mmqAttr;
static mqd_t mmqd;

/*
 * 模块*内部*使用的初始化参数
 */
void MmqInit(void) {
    asyslog(LOG_INFO, "初始化消息监听模块...");
    mmqAttr.mq_maxmsg = MAXNUM_PROXY_NET;
    mmqAttr.mq_msgsize = MAXSIZ_PROXY_NET;
    mmqd = -1;
}

/*
 * 用于程序退出时调用
 */
void MmqDestory(void) {
    //关闭资源
    asyslog(LOG_INFO, "关闭消息监听服务(%d)", mmqd);
    mmq_close(mmqd);
    mmqd = -1;
}

void MmqRead(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask) {
    INT8U getBuf[MAXSIZ_PROXY_NET];
    mmq_head headBuf;
    int res = mmq_get(fd, 1, &headBuf, getBuf);
    asyslog(LOG_INFO, "收到代理消息，返回(%d)，类型(%d)", res, headBuf.cmd);
    if (res <= 0) {
        close(fd);
        mmqd = -1;
        return;
    }

    //获取当前的上线通道
    if (GetOnlineType() == 0) {
        asyslog(LOG_WARNING, "当前无通道在线，收到代理消息[%d]", headBuf.cmd);
        return;
    }

    CommBlock *nst = NULL;

    switch (GetOnlineType()) {
        case 1:
            GetComBlockForGprs();
            break;
        case 2:
            GetComBlockForNet();
            break;
        case 3:
            getComBlockForModel();
            break;
    }
    
    switch (headBuf.cmd) {
        case TERMINALPROXY_RESPONSE:
            ProxyListResponse((PROXY_GETLIST *) getBuf, nst);
            break;
        case TERMINALEVENT_REPORT :
            Report_Event(nst, getBuf, 2, 1);
            break;
        case METEREVENT_REPORT:
            callEventAutoReport(nst, getBuf, headBuf.bufsiz);
            break;
    }
    return;
}

/*
 * 模块维护循环
 */
int RegularMmq(struct aeEventLoop *ep, long long id, void *clientData) {
    if (mmqd < 0) {
        mmqd = mmq_open(PROXY_NET_MQ_NAME, &mmqAttr, O_RDONLY);
        if (mmqd >= 0) {
            aeCreateFileEvent(ep, mmqd, AE_READABLE, MmqRead, clientData);
        } else {
            asyslog(LOG_ERR, "消息队列监听建立失败，1分钟后重建");
            return 60 * 1000;
        }
    }
    return 1000;
}

/*
 * 供外部使用的初始化函数，并开启维护循环
 */
int StartMmq(struct aeEventLoop *ep, long long id, void *clientData) {
    MmqInit();
    Mmq_Task_Id = aeCreateTimeEvent(ep, 1000, RegularMmq, clientData, NULL);
    asyslog(LOG_INFO, "监听服务器时间事件注册完成(%lld)", Mmq_Task_Id);
    return 1;
}
