#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "db.h"
#include "mmq.h"
#include "cjcomm.h"
#include "libmmq.h"
#include "StdDataType.h"

//建立消息监听服务
static mqd_t mmqd;

int RetryTask(struct aeEventLoop* ep, long long id, void* clientData) {
	int count = (int) dbGet("mmq.retry_count") + 1;
	dbSet("mmq.retry_count", count);
	CommBlock *nst = NULL;
	if (count < 60) {
		switch ((int) dbGet("online.type")) {
		case 1:
			nst = dbGet("block.gprs");
			break;
		case 2:
			nst = dbGet("block.net");
			break;
		case 3:
			nst = dbGet("block.gprs");
			break;
		}
		if (nst == NULL) {
			return AE_NOMORE;
		}
		if (nst->response_piid[0] != 0
				&& nst->response_piid[0] == nst->report_piid[0]) {
			return AE_NOMORE;
		}else{
			return 1000;
		}
	}
	callNotificationReport(nst, dbGet("mmq.retry_buf"),
			((mmq_head*) dbGet("mmq.retry_head"))->dataOAD,
			((mmq_head*) dbGet("mmq.retry_head"))->bufsiz);
	return 0;
}

void MmqReadandSend(struct aeEventLoop *ep, int fd, void *clientData, int mask) {
	INT8U getBuf[MAXSIZ_PROXY_NET];
	mmq_head headBuf;
	int res = mmq_get(fd, 1, &headBuf, getBuf);
	if (res <= 0) {
		return;
	}

	CommBlock *nst = NULL;
	switch ((int) dbGet("online.type")) {
	case 1:
		nst = dbGet("block.gprs");
		break;
	case 2:
		nst = dbGet("block.net");
		break;
	case 3:
		nst = dbGet("block.gprs");
		break;
	}

	asyslog(LOG_INFO, "发送代理消息，返回(%d)，类型(%d)", 0, headBuf.cmd);
	switch (headBuf.cmd) {
	case TERMINALPROXY_RESPONSE:
		ProxyListResponse((PROXY_GETLIST *) getBuf, nst);
		break;
	case TERMINALEVENT_REPORT:
		Report_Event(nst, getBuf, 2);
		break;
	case METEREVENT_REPORT:
		callEventAutoReport(nst, getBuf, headBuf.bufsiz);
		break;
	case NOTIFICATIONTRANS_PEPORT:
		callNotificationReport(nst, getBuf, headBuf.dataOAD, headBuf.bufsiz);
		dbSet("mmq.retry_count", 0);
		memcpy(dbGet("mmq.retry_buf"), getBuf, sizeof(getBuf));
		memcpy(dbGet("mmq.retry_head"), &headBuf, sizeof(headBuf));
		int sid = aeCreateTimeEvent(ep, 1000, RetryTask, clientData, NULL);
		asyslog(LOG_INFO, "建立重复上送监听，等待主站确认(%lld)", sid);
		break;
	}
	return;
}

/*
 * 模块维护循环
 */
int RegularMmq(struct aeEventLoop *ep, long long id, void *clientData) {
	//获取当前的上线通道
	if (dbGet("online.type") == 0) {
		asyslog(LOG_WARNING, "<代理消息、事件主动上报>等待通道在线...");
		return 8000;
	}

	if (mmqd <= 0) {
		struct mq_attr mmqAttr;
		mmqAttr.mq_maxmsg = MAXNUM_PROXY_NET;
		mmqAttr.mq_msgsize = MAXSIZ_PROXY_NET;
		mmqd = mmq_open(PROXY_NET_MQ_NAME, &mmqAttr, O_RDONLY);
		if (mmqd <= 0) {
			asyslog(LOG_ERR, "消息队列监听建立失败，1分钟后重建");
			return 60 * 1000;
		}
	} else {
		MmqReadandSend(ep, mmqd, NULL, 0);
	}
	return 1000;
}

/*
 * 供外部使用的初始化函数，并开启维护循环
 */
int StartMmq(struct aeEventLoop *ep, long long id, void *clientData) {
	mmqd = -1;
	int sid = aeCreateTimeEvent(ep, 1000, RegularMmq, clientData, NULL);
	asyslog(LOG_INFO, "监听服务器时间事件注册完成(%lld)", sid);
	return 1;
}
