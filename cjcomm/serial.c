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

int RegularSerial(struct aeEventLoop *ep, long long id, void *clientData) {
	COMDCB *com = &(((CLASS_f201*) dbGet("f201"))->devpara);
	CommBlock *nst = (CommBlock *) clientData;

	if (nst->phy_connect_fd <= 0) {
		nst->phy_connect_fd = helperComOpen9600(4, com->baud, com->verify,
				com->stopbits, com->databits);
		if (nst->phy_connect_fd > 0) {
			aeCreateFileEvent(ep, nst->phy_connect_fd, AE_READABLE, cRead,
					nst);
		}
	} else {
		cProcForShanDongAutoReport(ep, nst);
	}
	return 500;
}

/*
 * 供外部使用的初始化函数，并开启维护循环
 */
int StartSerial(struct aeEventLoop *ep, long long id, void *clientData) {
	int sid = aeCreateTimeEvent(ep, 1000, RegularSerial, dbGet("block.serial"),
			NULL);
	asyslog(LOG_INFO, "维护串口时间事件注册完成(%lld)", sid);
	return 1;
}
