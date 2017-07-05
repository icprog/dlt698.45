#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "cjcomm.h"
#include "mmq.h"
#include "../libMq/libmmq.h"
#include "../include/StdDataType.h"

//建立消息监听服务
static long long Mmq_Task_Id;
static struct mq_attr mmqAttr;
static mqd_t mmqd;
static EventBuf autoEventBuf[AUTO_EVENT_BUF_SIZE];

void initAutoEventBuf(EventBuf *eb) {
    memset(autoEventBuf, 0x00, sizeof(autoEventBuf));
    for (int i = 0; i < AUTO_EVENT_BUF_SIZE; ++i) {
        eb[i].header.cmd = -1;
    }
}

int putAutoEventBuf(EventBuf *eb, mmq_head *msg_head, void *buff) {
    for (int i = 0; i < AUTO_EVENT_BUF_SIZE; ++i) {
        if (eb[i].header.cmd == -1) {
            memcpy(&eb[i].header, msg_head, sizeof(mmq_head));
            memcpy(&eb[i].content, buff, MAXSIZ_PROXY_NET);
            eb[i].repeat_timeout = -1;
            saveCoverClass(0x4520, 0, autoEventBuf, sizeof(autoEventBuf), para_vari_save);
            return 1;
        }
    }
    return -1;
}

int getAutoEventBuf(EventBuf *eb, mmq_head *msg_head, void *buff) {
    /*
     * 当前有需要发送的事件数据
     */
//    for (int i = 0; i < AUTO_EVENT_BUF_SIZE; ++i) {
//        if (eb[i].repeat_timeout > 0 && eb[i].header.cmd != -1) {
//            return -1;
//        }
//    }

    /*
     * 获取需要再次发送的事件
     */
//    for (int i = 0; i < AUTO_EVENT_BUF_SIZE; ++i) {
//        if (eb[i].repeat_timeout == 0 && eb[i].header.cmd != -1) {
//            memcpy(msg_head, &eb[i].header, sizeof(mmq_head));
//            memcpy(buff, &eb[i].content, MAXSIZ_PROXY_NET);
//            eb[i].repeat_timeout = -1;
//            eb[i].header.cmd = -1;
//            return 1;
//        }
//    }
    /*
     * 获取首次发送的事件
     */
    for (int i = 0; i < AUTO_EVENT_BUF_SIZE; ++i) {
        if (eb[i].header.cmd != -1) {
            memcpy(msg_head, &eb[i].header, sizeof(mmq_head));
            memcpy(buff, &eb[i].content, MAXSIZ_PROXY_NET);
            eb[i].repeat_timeout = AUTO_EVENT_REPEAT_TIMEOUT;
            eb[i].header.cmd = -1;
            saveCoverClass(0x4520, 0, autoEventBuf, sizeof(autoEventBuf), para_vari_save);
            return 1;
        }
    }
    return -1;
}

void refreshAutoEventBuf(EventBuf *eb) {
    for (int i = 0; i < AUTO_EVENT_BUF_SIZE; ++i) {
        if (eb[i].header.cmd != -1 && eb[i].repeat_timeout > 0) {
            eb[i].repeat_timeout--;
            return;
        }
    }
    return;
}

void freeAutoEventBuf() {
	for (int i = 0; i < AUTO_EVENT_BUF_SIZE; ++i) {
		if (autoEventBuf[i].header.cmd != -1
				&& autoEventBuf[i].repeat_timeout != -1) {
			autoEventBuf[i].header.cmd = -1;
			autoEventBuf[i].repeat_timeout = -1;
			return;
		}
	}
	return;
}

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
    if (res <= 0) {
        return;
    }
    asyslog(LOG_INFO, "收到代理消息，返回(%d)，类型(%d)", res, headBuf.cmd);
    if (putAutoEventBuf(autoEventBuf, &headBuf, getBuf) == -1) {
        asyslog(LOG_ALERT, "警告：事件缓冲区已满,事件上送丢失！");
    }
    return;
}

void MmqSend(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask) {
    INT8U getBuf[MAXSIZ_PROXY_NET];
    mmq_head headBuf;
    CommBlock *nst = NULL;

//    refreshAutoEventBuf(autoEventBuf);
    if (getAutoEventBuf(autoEventBuf, &headBuf, getBuf) == -1) {
        return;
    }

    switch (GetOnlineType()) {
        case 1:
            nst = GetComBlockForGprs();
            break;
        case 2:
            nst = GetComBlockForNet();
            break;
        case 3:
            nst = getComBlockForModel();
            break;
    }
    asyslog(LOG_INFO, "发送代理消息，返回(%d)，类型(%d)", 0, headBuf.cmd);
    switch (headBuf.cmd) {
        case TERMINALPROXY_RESPONSE:
            ProxyListResponse((PROXY_GETLIST *) getBuf, nst);
            break;
        case TERMINALEVENT_REPORT:
            Report_Event(nst, getBuf, 2, 1);
            break;
        case METEREVENT_REPORT:
            callEventAutoReport(nst, getBuf, headBuf.bufsiz);
            break;
    }
}

/*
 * 模块维护循环
 */
int RegularMmq(struct aeEventLoop *ep, long long id, void *clientData) {
    //获取当前的上线通道
    if (GetOnlineType() == 0) {
        asyslog(LOG_WARNING, "当前无通道在线, 不开启消息监听");
        close(mmqd);
        mmqd = -1;
        return 5000;
    }

    if (mmqd < 0) {
        mmqd = mmq_open(PROXY_NET_MQ_NAME, &mmqAttr, O_RDONLY);
        if (mmqd <= 0) {
            asyslog(LOG_ERR, "消息队列监听建立失败，1分钟后重建");
            return 60 * 1000;
        }
    } else {
        MmqRead(ep, mmqd, NULL, 0);
        MmqSend(ep, mmqd, NULL, 0);
    }
    return 1000;
}

/*
 * 供外部使用的初始化函数，并开启维护循环
 */
int StartMmq(struct aeEventLoop *ep, long long id, void *clientData) {
    MmqInit();
    initAutoEventBuf(autoEventBuf);
    readCoverClass(0x4520, 0, autoEventBuf, sizeof(autoEventBuf), para_vari_save);
    Mmq_Task_Id = aeCreateTimeEvent(ep, 1000, RegularMmq, clientData, NULL);
    asyslog(LOG_INFO, "监听服务器时间事件注册完成(%lld)", Mmq_Task_Id);
    return 1;
}
