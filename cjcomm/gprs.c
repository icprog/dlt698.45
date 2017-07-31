#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "db.h"
#include "Shmem.h"
#include "atBase.h"
#include "helper.h"
#include "cjcomm.h"

/*
 * 本文件内存放客户端模式代码，专门处理客户端模式数据收发
 * 错误处理等
 * GPRS上线专用文件
 */

int CertainConnectForGprs(char *interface, CommBlock *commBlock) {
	static int step = 0;
	static int fd = 0;
	static char peerBuf[32];
	static char boundBuf[32];
	static int port = 0;

	if (step == 0) {
		MASTER_STATION_INFO ip_port = helperGetNextGPRSIp();

		memset(boundBuf, 0x00, sizeof(boundBuf));
		if (helperGetInterFaceIp(interface, boundBuf) == 1) {
			fd = anetTcpNonBlockBindConnect(NULL, (char *) ip_port.ip,
					ip_port.port, boundBuf);
			if (fd > 0) {
				step = 1;
			}
		}
		return -1;
	} else if (step < 8) {
		memset(peerBuf, 0x00, sizeof(peerBuf));
		if (0 == anetPeerToString(fd, peerBuf, sizeof(peerBuf), &port)) {
			step = 0;
			return fd;
		} else {
			step++;
			return -1;
		}
	} else {
		close(fd);
		step = 0;
	}
}

static int RegularModel(struct aeEventLoop *ep, long long id, void *clientData) {
	CommBlock *nst = (CommBlock *) clientData;
	nst->p_send = AtWriteToBuf;

	if (!AtPrepareFinish(AtGet())) {
		refreshComPara(nst);
		return 2000;
	}

	int res = AtReadExactly(AtGet(), nst);

	if (res == -1 || Comm_task(nst) == -1) {
		dbSet("oneline.type", 0);
	}

	if (res > 0) {
		TSGet(&nst->final_frame);
	}

	cProc(ep, nst);
	while (AtSendExactly(AtGet()) > 0) {
		continue;
	}

	return 500;
}

static int RegularGprs(struct aeEventLoop *ep, long long id, void *clientData) {
	CommBlock *nst = (CommBlock *) clientData;

	if (!AtPrepareFinish(AtGet())) {
		return 2000;
	}

	if (nst->phy_connect_fd <= 0) {
		if (dbGet("oneline.type") != 0) {
			return 2000;
		}
		refreshComPara(nst);
		nst->phy_connect_fd = CertainConnectForGprs("ppp0", nst);
		if (nst->phy_connect_fd > 0) {
			aeCreateFileEvent(ep, nst->phy_connect_fd, AE_READABLE, cRead, nst);
			helperPeerStat(nst->phy_connect_fd, "客户端[GPRS]与主站链路建立成功");
			gpofun("/dev/gpoONLINE_LED", 1);
			dbSet("online.type", 1);
		} else {
			return 2000;
		}
	} else {
		if (Comm_task(nst) == -1) {
			asyslog(LOG_WARNING, "客户端[GPRS]链接心跳超时，关闭端口");
			aeDeleteFileEvent(ep, nst->phy_connect_fd, AE_READABLE);
			close(nst->phy_connect_fd);
			nst->phy_connect_fd = -1;
			dbSet("oneline.type", 0);
		}
		cProc(ep, nst);
//		RegularAutoTask(ep, nst);
	}

	return 500;
}

/*
 * 供外部使用的初始化函数，并开启维护循环
 */
int StartClientForGprs(struct aeEventLoop *ep, long long id, void *clientData) {
	int sid = aeCreateTimeEvent(ep, 1000,
			dbGet("gprs.type") == 2 ? RegularModel : RegularGprs,
			dbGet("block.gprs"), NULL);
	asyslog(LOG_INFO, "客户端[GPRS]时间事件注册完成(%lld)", sid);
	return 1;
}
