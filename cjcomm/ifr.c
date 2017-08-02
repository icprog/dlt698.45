#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "db.h"
#include "helper.h"
#include "cjcomm.h"

static int RegularIfr(struct aeEventLoop *ep, long long id, void *clientData) {
	COMDCB *com = &(((CLASS_f202*) dbGet("f202"))->devpara);
	CommBlock *nst = (CommBlock *) clientData;

	if (nst->phy_connect_fd <= 0) {
		nst->phy_connect_fd = helperComOpen(3, com->baud, com->verify,
				com->stopbits, com->databits);
		if (nst->phy_connect_fd > 0) {
			aeCreateFileEvent(ep, nst->phy_connect_fd, AE_READABLE, cRead,
					nst);
		}
	} else {
		cProc(ep, nst);
	}
	return 500;
}

/*
 * 供外部使用的初始化函数，并开启维护循环
 */
int StartIfr(struct aeEventLoop *ep, long long id, void *clientData) {
	int sid = aeCreateTimeEvent(ep, 1000, RegularIfr, dbGet("block.ifr"), NULL);
	asyslog(LOG_INFO, "红外串口时间事件注册完成(%lld)", sid);
	return 1;
}
