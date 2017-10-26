#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>

#include "db.h"
#include "Shmem.h"
#include "cjcomm.h"
#include "atBase.h"
#include "special.h"

int cWriteWithCalc(int fd, INT8U *buf, INT16U len) {
	int old = (int) dbGet("calc.new") + len;
	dbSet("calc.new", old);
	cWrite(fd, buf, len);
}

int cWrite(int fd, INT8U *buf, INT16U len) {
	int ret = anetWrite(fd, buf, (int) len);
	if (ret != len) {
		asyslog(LOG_WARNING, "报文发送失败(长度:%d,错误:%d,端口:%d)", len, errno, fd);
	}
	bufsyslog(buf, "发送:", len, 0, BUFLEN);
	if (getZone("GW") == 0) {
		char prtpara[16];
		sprintf(prtpara,"[NET_%d]S:",fd);
		PacketBufToFile(1,prtpara, (char *) buf, len, NULL);
	}
	return ret;
}

void cReadWithCalc(struct aeEventLoop *ep, int fd, void *clientData, int mask) {
	int revcount = 0;
	ioctl(fd, FIONREAD, &revcount);
	if (revcount > 0) {
		int old = (int) dbGet("calc.new") + revcount;
		dbSet("calc.new", old);
	}
	cRead(ep, fd, clientData, mask);
}

void cReadWithoutCheck(struct aeEventLoop *ep, int fd, void *clientData, int mask) {
	CommBlock *nst = (CommBlock *) clientData;

	int revcount = 0;
	ioctl(fd, FIONREAD, &revcount);

	if(revcount <= 0){
		return;
	}

	TSGet(&nst->final_frame);
	for (int j = 0; j < revcount; j++) {
		read(fd, &nst->RecBuf[nst->RHead], 1);
		nst->RHead = (nst->RHead + 1) % BUFLEN;
	}
	bufsyslog(nst->RecBuf, "接收:", nst->RHead, nst->RTail, BUFLEN);
	if (getZone("GW") == 0) {
		int buflen = 0;
		buflen = (nst->RHead - nst->RTail + BUFLEN) % BUFLEN;
		char prtpara[16];
		sprintf(prtpara,"[NET_%d]R:",fd);
		PacketBufToFile(1,prtpara, (char *) &nst->RecBuf[nst->RTail],
				buflen, NULL);
	}
}

void cRead(struct aeEventLoop *ep, int fd, void *clientData, int mask) {
	CommBlock *nst = (CommBlock *) clientData;

	int revcount = 0;
	ioctl(fd, FIONREAD, &revcount);

	//关闭异常端口
	if (revcount <= 0) {
		asyslog(LOG_WARNING, "链接[%d]异常，关闭端口", errno);
		aeDeleteFileEvent(ep, fd, AE_READABLE);
		close(fd);
		dbSet("online.type", 0);
		nst->phy_connect_fd = -1;
	} else {
		TSGet(&nst->final_frame);
		for (int j = 0; j < revcount; j++) {
			read(fd, &nst->RecBuf[nst->RHead], 1);
			nst->RHead = (nst->RHead + 1) % BUFLEN;
		}
		bufsyslog(nst->RecBuf, "接收:", nst->RHead, nst->RTail, BUFLEN);
		if (getZone("GW") == 0) {
			int buflen = 0;
			buflen = (nst->RHead - nst->RTail + BUFLEN) % BUFLEN;
			char prtpara[16];
			sprintf(prtpara,"[NET_%d]R:",fd);
			PacketBufToFile(1,prtpara, (char *) &nst->RecBuf[nst->RTail],
					buflen, NULL);
		}
	}
}

void cProc(struct aeEventLoop *ep, CommBlock * nst) {
	int res = 0;
	do {
		res = StateProcess(nst, 5);
		if (nst->deal_step >= 3) {
			int apduType = ProcessData(nst);
			ConformAutoTask(ep, nst, apduType);
			switch (apduType) {
			case LINK_RESPONSE:
				gpofun("/dev/gpoONLINE_LED", 1);
				First_VerifiTime(nst->linkResponse, nst->shmem); //简单对时
				if (GetTimeOffsetFlag() == 1) {
					Getk_curr(nst->linkResponse, nst->shmem);
				}
				nst->linkstate = build_connection;
				nst->testcounter = 0;
				break;
			case 9:
				dbSet("proxy", nst);
				fprintf(stderr, "以太网收到代理消息，更新端口\n");
				break;
			default:
				break;
			}
		}
	} while (res == 1);
}

int cProcForShanDongAutoReport(struct aeEventLoop *ep, CommBlock * nst) {
	int res = 0;
	do {
		res = StateProcess(nst, 5);
		if (nst->deal_step >= 3) {
			int apduType = ProcessData(nst);
			//5分钟之内可以通过485自动上送数据
			dbSet("485auto", 60 * 5);
			ConformAutoTask(ep, nst, apduType);
			switch (apduType) {
			case LINK_RESPONSE:
				gpofun("/dev/gpoONLINE_LED", 1);
				First_VerifiTime(nst->linkResponse, nst->shmem); //简单对时
				if (GetTimeOffsetFlag() == 1) {
					Getk_curr(nst->linkResponse, nst->shmem);
				}
				nst->linkstate = build_connection;
				nst->testcounter = 0;
				break;
			case 9:
				dbSet("proxy", nst);
				fprintf(stderr, "串口收到代理消息，更新端口\n");
				break;
			default:
				break;
			}
		}
	} while (res == 1);
}

void QuitProcess(int sig) {
	if (helperKill("gsmMuxd", 18) == -1) {
		asyslog(LOG_WARNING, "未能彻底结束gsmMuxd进程...");
	}
	ProgramInfo *info = (ProgramInfo *) dbGet("program.info");
	info->dev_info.Gprs_csq = 0;
	info->dev_info.gprs_status = 0;
	info->dev_info.wirelessType = 0;
	info->dev_info.pppd_status = 0;
	info->dev_info.connect_ok = 0;
	info->dev_info.jzq_login = 0;
	asyslog(LOG_INFO, "通信模块退出,收到信号类型(%d),在线状态", sig, info->dev_info.jzq_login);
	shmm_unregister("ProgramInfo", sizeof(ProgramInfo));
	exit(0);
}

void WriteLinkRequest(INT8U link_type, INT16U heartbeat, LINK_Request *link_req) {

	struct timeval tpnow;
	gettimeofday(&tpnow, NULL);

	int msec = (float)tpnow.tv_usec/1000;//毫秒

	TS ts = { };
	TSGet(&ts);
	link_req->type = link_type;
	link_req->piid_acd.data = 0;
	link_req->time.year = ts.Year;//((ts.Year << 8) & 0xff00) | ((ts.Year >> 8) & 0xff); // apdu 先高后低
	link_req->time.month = ts.Month;
	link_req->time.day_of_month = ts.Day;
	link_req->time.day_of_week = ts.Week;
	link_req->time.hour = ts.Hour;
	link_req->time.minute = ts.Minute;
	link_req->time.second = ts.Sec;
	link_req->time.milliseconds = msec;
	link_req->heartbeat = heartbeat;//((heartbeat << 8) & 0xff00)| ((heartbeat >> 8) & 0xff);

}

int Comm_task(CommBlock *compara) {
	INT16U heartbeat = (compara->Heartbeat == 0) ? 300 : compara->Heartbeat;
	if (abs(time(NULL) - compara->lasttime) < heartbeat) {
		return 0;
	}
	compara->lasttime = time(NULL);

	if (compara->testcounter > 2) {
		return -1;
	}

	if (compara->linkstate == close_connection) {
		WriteLinkRequest(build_connection, heartbeat, &compara->link_request);
		int len = Link_Request(compara->link_request, compara->serveraddr,
				compara->SendBuf);
		compara->p_send(compara->phy_connect_fd, compara->SendBuf, len);
	} else {
		WriteLinkRequest(heart_beat, heartbeat, &compara->link_request);
		int len = Link_Request(compara->link_request, compara->serveraddr,
				compara->SendBuf);
		compara->p_send(compara->phy_connect_fd, compara->SendBuf, len);
	}
	compara->testcounter++;
	return 0;
}

void refreshComPara(CommBlock *compara) {
	compara->phy_connect_fd = -1;
	compara->testcounter = 0;
	compara->linkstate = close_connection;
	memset(compara->RecBuf, 0, sizeof(compara->RecBuf));
	memset(compara->SendBuf, 0, sizeof(compara->SendBuf));
	memset(compara->DealBuf, 0, sizeof(compara->DealBuf));
	compara->RHead = 0;
	compara->RTail = 0;
	compara->deal_step = 0;
	compara->rev_delay = 20;
	compara->shmem = dbGet("program.info");
	compara->lasttime = 0;
}

void initComPara(CommBlock *compara,
		INT32S (*p_send)(int fd, INT8U *buf, INT16U len)) {
	CLASS_4001_4002_4003 c4001;
	memset(&c4001, 0x00, sizeof(c4001));
	readCoverClass(0x4001, 0, &c4001, sizeof(c4001), para_vari_save);
	memcpy(compara->serveraddr, c4001.curstom_num, 16);

	compara->phy_connect_fd = -1;
	compara->testcounter = 0;
	compara->linkstate = close_connection;
	memset(compara->RecBuf, 0, sizeof(compara->RecBuf));
	memset(compara->SendBuf, 0, sizeof(compara->SendBuf));
	memset(compara->DealBuf, 0, sizeof(compara->DealBuf));
	compara->RHead = 0;
	compara->RTail = 0;
	compara->deal_step = 0;
	compara->rev_delay = 20;
	compara->shmem = dbGet("program.info");
	compara->p_send = p_send;
	compara->lasttime = 0;

	CLASS19 Class19 = { };
	memset(&Class19, 0, sizeof(CLASS19));
	if (readCoverClass(0x4300, 0, &Class19, sizeof(CLASS19), para_vari_save)) {
		memcpy(&compara->myAppVar.server_factory_version, &Class19.info,
				sizeof(FactoryVersion));
	}
	for (int i = 0; i < 2; i++) {
		compara->myAppVar.FunctionConformance[i] = 0xff;
	}
	for (int i = 0; i < 5; i++) {
		compara->myAppVar.ProtocolConformance[i] = 0xff;
	}
	compara->myAppVar.server_deal_maxApdu = FRAMELEN;
	compara->myAppVar.server_recv_size = FRAMELEN;
	compara->myAppVar.server_send_size = FRAMELEN; //台体测试终端主动上报时一帧数据超过1024个字节，并且需要一帧上送，此处最大2048
	compara->myAppVar.server_recv_maxWindow = 1;
	compara->myAppVar.expect_connect_timeout = 56400;

	readCoverClass(0xf101, 0, &compara->f101, sizeof(CLASS_F101),
			para_vari_save);
}

void commEnvCheck(int argc, char *argv[]) {
	pid_t pids[32];

	if (prog_find_pid_by_name((INT8S *) argv[0], pids) > 1) {
		asyslog(LOG_ERR, "检查到重复进程[号：%d]，程序退出...", pids[0]);
		exit(0);
	}

	if (argc < 2) {
		asyslog(LOG_ERR, "参数不足，程序退出...", pids[0]);
		exit(0);
	}

	//绑定信号处理了函数
	struct sigaction sa = { };
	Setsig(&sa, QuitProcess);
}

int doAt(struct aeEventLoop *ep, long long id, void *clientData) {
	ATOBJ *ao = (ATOBJ *) clientData;
	if ((int) dbGet("online.type") != 0) {
		return 2000;
	}
	return AtPrepare(ao);
}

int main(int argc, char *argv[]) {
	commEnvCheck(argc, argv);
	dbInit(atoi(argv[1]));

	if (argc > 2 && atoi(argv[2]) == 2) {
		dbSet("gprs.type", 2);
	}

	AtInitObjBlock(AtGet());

	//开启网络IO事件处理框架
	aeEventLoop *ep;
	ep = aeCreateEventLoop(128);
	if (ep == NULL) {
		asyslog(LOG_ERR, "事件处理框架创建失败，程序终止。\n");
		exit(0);
	}

	StartClientForNet(ep, 0, NULL);
	StartClientForGprs(ep, 0, NULL);
	StartSecial(ep, 0, NULL);
	aeCreateTimeEvent(ep, 1000, doAt, AtGet(), NULL);

	StartVerifiTime(ep, 0, dbGet("program.info"));
	StartMmq(ep, 0, NULL);
	StartIfr(ep, 0, NULL);
	StartSerial(ep, 0, NULL);

	if((int)dbGet("4852open") == 1 && getZone("HuNan") == 0) {
		StartSerial_hn(ep, 0, NULL);
	}

	aeMain(ep);
	return 0;
}
