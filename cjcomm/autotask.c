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
#include "ParaDef.h"
//停止日常上报检测标志
static int stopSign = 0;
//上报确认标志
static int conformSign = 0;
static int conformTimes = 0, bak_conformTimes = 0;
static int conformOverTime = 0, bak_conformOverTime = 0;
static int reportChoice = 0;
//是否还有更多报文标示
static int MoreContentSign = 0;
//时间任务序号
static int conformCheckId = 0;

//任务参数变更
static INT16S taskChangeSign = -1;
//对时标识
static INT16S timeChangeSign = -1;
//停止重复上报标志
static INT8U stopREtry = 0;

void init6013ListFrom6012File(ProgramInfo *JProgramInfo) {

	INT16U total_autotasknum = 0;

	INT8U result = 0;
	memset(&JProgramInfo->autotask, 0, sizeof(JProgramInfo->autotask));	//增加初始化
	INT16U tIndex = 0;
	OI_698 oi = 0x6013;
	CLASS_6013 class6013 = { };

	for (tIndex = 0; tIndex < 256; tIndex++) {
		if (readCoverClass(oi, tIndex, &class6013, sizeof(CLASS_6013),
				coll_para_save) == 1) {
			if (class6013.cjtype == rept) {
				init_autotask(total_autotasknum, class6013,
						JProgramInfo->autotask);
				total_autotasknum++;
			}
		}
	}
}

int ConformCheck(struct aeEventLoop* ep, long long id, void* clientData) {
	CommBlock* nst = (CommBlock*) clientData;

	fprintf(stderr, "conformSign = %d\n", conformSign);
	//在此检查上报报文是否得到确认
	if (conformSign == 1) {
		return AE_NOMORE;
	}

	asyslog(LOG_INFO, "上报未确认,重试(%d)", conformTimes);
	//第一次调用此函数，启动任务上报

	if (conformTimes == 1) {
//        stopSign    = 0;	//注释,改在最后一帧清除
		conformSign = 0;
		//强制确认数据帧，跳下一帧发送
		MoreContentSign = callAutoReport(TASK_FRAME_DATA, reportChoice, nst, 1);
		if (MoreContentSign >= 1) {
			conformTimes = bak_conformTimes;
			conformOverTime = bak_conformOverTime;
			asyslog(LOG_INFO, "强制跳下一帧，标识[%d],重发[%d],超时[%d秒]", MoreContentSign,
					conformTimes, conformOverTime);
		} else {
			stopSign = 0;
			asyslog(LOG_INFO, "无更多报文,stopSign=%d", stopSign);
			return AE_NOMORE;
		}
	} else {
		MoreContentSign = callAutoReport(TASK_FRAME_DATA, reportChoice, nst, 0);
		conformTimes--;
	}
//    conformTimes--;

	return conformOverTime * 1000;
}

/*
 * 通过cj report命令进行曲线数据的补送,
 * 准备好的数据放在REPORT_FRAME_DATA文件中,上送结束后,自动删除文件
 * */
int HandReportTask(CommBlock* nst,INT8U *saveover)
{
	int callret = 0;
	if ((access(REPORT_FRAME_DATA, F_OK) == 0) && (*saveover == 1)) {
		DEBUG_TIME_LINE("");
		sleep(3);
		asyslog(LOG_INFO, "发现手动补报曲线文件");
		callret = callAutoReport(REPORT_FRAME_DATA,
		REPROTNOTIFICATIONRECORDLIST, nst, 1);		//1:默认收到确认数据
		if (callret == 0) {	//上报结束
			asyslog(LOG_INFO, "补报结束,删除文件");
			callret = unlink(REPORT_FRAME_DATA);
			if (callret == 0) {
				*saveover = 0;
				asyslog(LOG_INFO, "补报文件删除成功");
			}
		}
	}
	return 0;
}

void repairTime(int i, taskFailInfo_s* tfs)
{
	//检查需要补的时间是否过长
	TS tmp;
	memcpy(&tmp, &tfs->rptList[i][1].startTime, sizeof(TS));
	tminc(&tmp, 2, 12);
	if (TScompare(tmp, tfs->rptList[i][0].startTime) == 2) {
		memcpy(&tmp, &tfs->rptList[i][0].startTime, sizeof(TS));
		tminc(&tmp, 2, -12);
		memcpy(&tfs->rptList[i][1].startTime, &tmp, sizeof(TS));
		saveCoverClass(0x6099, 0, tfs, sizeof(taskFailInfo_s), para_vari_save);
	}
}

void checkAndSendAppends(INT8U *saveOver) {
	TS failts;
	TS succts;
	taskFailInfo_s* tfs = (taskFailInfo_s*)dbGet("task_list");

	if ( NULL == tfs || NULL == saveOver) {
		return;
	}

	for (int i = 0; i < MAXNUM_AUTOTASK; i++) {
		//如果当前没有需要发送的任务，就检查一下有没有需要补报的任务
		if (tfs->rptList[i][0].sign != 0xAA
				|| tfs->rptList[i][1].sign != 0xAA) {
			continue;
		}
		//先修正一下补报时间，然后去最后一次上报成功时间和补报时间
		repairTime(i, tfs);
		memcpy(&succts, &tfs->rptList[i][0].startTime, sizeof(TS));
		memcpy(&failts, &tfs->rptList[i][1].startTime, sizeof(TS));

		//没有补报的时长是否超过15分钟
		tminc(&failts, minute_units, 15);

		int res = TScompare(failts, succts);
		if (res == 0 || res == 1) {
			continue;
		} else {
			//取补报起始时间
			memcpy(&failts, &tfs->rptList[i][1].startTime, sizeof(TS));
			//计算补报结束时间
			tminc(&tfs->rptList[i][1].startTime, 2, 1);
			asyslog(LOG_INFO, "检查到任务: <%d> 的数据需要补报, 时间: %04d-%02d-%02d %02d-%02d-%02d, %04d-%02d-%02d %02d-%02d-%02d",
					tfs->rptList[i][1].taskId,
					failts.Year, failts.Month, failts.Day, failts.Hour, failts.Minute, failts.Sec,
					tfs->rptList[i][1].startTime.Year, tfs->rptList[i][1].startTime.Month, tfs->rptList[i][1].startTime.Day,
					tfs->rptList[i][1].startTime.Hour, tfs->rptList[i][1].startTime.Minute, tfs->rptList[i][1].startTime.Sec);
			supplementRpt(failts, tfs->rptList[i][1].startTime,
					tfs->rptList[i][1].taskId, saveOver);
			saveCoverClass(0x6099, 0, tfs, sizeof(taskFailInfo_s), para_vari_save);
			break;
		}
	}
	stopREtry = 1;
}

void RegularAutoTask(struct aeEventLoop* ep, CommBlock* nst) {

	ProgramInfo* shmem = (ProgramInfo*) nst->shmem;
	rptInfo_s* rptInfo = (rptInfo_s*) dbGet("curr_task");
	if (stopSign == 1) {
		return;
	}

	if (taskChangeSign != shmem->oi_changed.oi6012
			|| timeChangeSign != shmem->oi_changed.oi4000) {
		asyslog(LOG_INFO, "检查到6012参数变更，或者时间变化，重新计算任务时间.");
		init6013ListFrom6012File(shmem);
		taskChangeSign = shmem->oi_changed.oi6012;
		timeChangeSign = shmem->oi_changed.oi4000;
		stopSign = 0;		//清除上报标记
	}

	for (int i = 0; i < MAXNUM_AUTOTASK; i++) {
		//调用日常通信接口
		rptInfo->pos = i;
		int res = composeAutoTask(&shmem->autotask[i], rptInfo);
		if ((res == 1) || (res == 2)) {
			//第一次调用此函数，启动任务上报
			reportChoice = res;
			MoreContentSign = callAutoReport(TASK_FRAME_DATA, reportChoice, nst,
					0);
			//不再调用此函数标志
			stopSign = 1;
			//标示上报任务尚未获得确认
			conformSign = 0;
			if (shmem->autotask[i].ReportNum > 5) {
				conformTimes = 5;
				asyslog(LOG_INFO, "任务重复上报设置次数[%d]过大,设置默认上报5次",
						shmem->autotask[i].ReportNum);
			} else {
				conformTimes = shmem->autotask[i].ReportNum;
			}
			if (shmem->autotask[i].OverTime > 120) {
				conformOverTime = 120;
				asyslog(LOG_INFO, "任务重复上报超时时间[%d]秒,设置超时120秒",
						shmem->autotask[i].OverTime);
			} else {
				conformOverTime = shmem->autotask[i].OverTime;
			}
			bak_conformTimes = conformTimes;			//记录第一帧无应答后,下一帧重复发送次数
			bak_conformOverTime = conformOverTime;
			//注册时间事件，检查确认状态
			conformCheckId = aeCreateTimeEvent(ep, conformOverTime * 1000,
					ConformCheck, nst, NULL);
			asyslog(LOG_INFO, "检查到上报任务，初始化上报状态(次数=%d-时间=%d)、注册时间事件(%d)",
					conformTimes, conformOverTime, conformCheckId);
			break;
		}
	}
	//没有正常曲线上报，且没有补报曲线上报
	HandReportTask(nst, &shmem->cfg_para.extpara[0]);	//cj report命令手动补抄进行任务上送

	if (stopSign == 0 && stopREtry != 1) {
		checkAndSendAppends(&shmem->cfg_para.extpara[0]);
	}
}

void ConformAutoTask(struct aeEventLoop* ep, CommBlock* nst, int res) {
	if (res == REPORT_RESPONSE && stopSign == 1) {
		asyslog(LOG_INFO, "任务收到确认");
		//暂时不使用分帧重复发送  -
		//曲线分帧第一帧上送完 不清除标记,等待全部上送结束,注释stopSign=0
//        stopSign    = 0;
		conformSign = 1;
//        return;
		//有更多的报文
		MoreContentSign = callAutoReport(TASK_FRAME_DATA, reportChoice, nst, 1);
		if (MoreContentSign == 1) {
			asyslog(LOG_INFO, "发现更多的报文，注销检查函数，任务序号(%d)", conformCheckId);
			if (aeDeleteTimeEvent(ep, conformCheckId) == AE_OK) {//TODO:是否需要增加删除成功,重新注册,删除失败会有什么问题?
				conformSign = 0;		//清除确认标记,方便重新进入检查函数
				conformTimes = bak_conformTimes;
				conformOverTime = bak_conformOverTime;
				conformCheckId = aeCreateTimeEvent(ep, conformOverTime * 1000,
						ConformCheck, nst, NULL);
				asyslog(LOG_INFO, "重新注册，任务序号(%d),(次数=%d-时间=%d)", conformCheckId,
						conformTimes, conformOverTime);
			}
		} else {
			//这里成功上送了一次任务
			stopSign = 0;
			conformSign = 1;
			taskFailInfo_s* tfs = dbGet("task_list");
			rptInfo_s* rptInfo = (rptInfo_s*) dbGet("curr_task");
			int index = rptInfo->pos;
			rptInfo->sign = 0xAA;
			//更新最新的成功上报任务的时间
			memcpy(&tfs->rptList[index][0], rptInfo, sizeof(rptInfo_s));
			//如果1位置没有初始化，就在这里初始化一下。
			if (tfs->rptList[index][1].sign != 0xAA) {
				memcpy(&tfs->rptList[index][1], rptInfo, sizeof(rptInfo_s));
			}
			stopREtry = 0;
			saveCoverClass(0x6099, 0, tfs, sizeof(taskFailInfo_s), para_vari_save);
		}
	}
}
