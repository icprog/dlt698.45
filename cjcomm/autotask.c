#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "db.h"
#include "cjcomm.h"
#include "ParaDef.h"
//停止日常上报检测标
static int stopSign = 0;
//上报确认标志
static int conformSign = 0;
static int conformTimes = 0, bak_conformTimes = 0;
static int conformOverTime = 0, bak_conformOverTime = 0;
static int reportChoice = 0;
//是否还有更多报文标示
static int MoreContentSign = 0;
//时间任务序号
static long long conformCheckId = 0;

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
	int times = 0;

	if ((access(REPORT_FRAME_DATA, F_OK) == 0) && (*saveover == 1)) {
		times = 0;
		do{
			sleep(3);
			asyslog(LOG_INFO, "发现手动补报文件");
			callret = callAutoReport(REPORT_FRAME_DATA,
			REPROTNOTIFICATIONRECORDLIST, nst, 1);		//1:默认收到确认数据
			times++;
		}while(callret && times <= 1024);

		if (callret == 0) {	//上报结束
			asyslog(LOG_INFO, "补报结束,删除文件");
			callret = unlink(REPORT_FRAME_DATA);
			if (callret == 0) {
				*saveover = 0;
				asyslog(LOG_INFO, "补报文件删除成功");
			} else {
				asyslog(LOG_INFO, "补报文件删除失败");
			}
		}
	}
	return 0;
}

int getTaskInterval(int i, TI* taskInv)
{
	ProgramInfo * shmem = (ProgramInfo *)dbGet("program.info");
	taskFailInfo_s* tfs = (taskFailInfo_s*)dbGet("task_list");
	CLASS_6013 class6013={};

	if( NULL == taskInv	 || NULL == tfs || NULL == shmem)
		return 0;

	if (readCoverClass(0x6013, shmem->autotask[tfs->rptList[i][1].pos].ID, &class6013, sizeof(CLASS_6013),coll_para_save) == 1) {
		taskInv->units = class6013.interval.units;
		taskInv->interval = class6013.interval.interval;
	} else
		return 0;

	return 1;
}

void repairTime(int i, taskFailInfo_s* tfs)
{
	//检查需要补的时间是否过长
	TS tmp;

	if(NULL == tfs)
		return;

	memcpy(&tmp, &tfs->rptList[i][1].startTime, sizeof(TS));
	tminc(&tmp, hour_units, 24);
	if (TScompare(tmp, tfs->rptList[i][0].startTime) == 2) {
		memcpy(&tmp, &tfs->rptList[i][0].startTime, sizeof(TS));
		tminc(&tmp, hour_units, -24);
		memcpy(&tfs->rptList[i][1].startTime, &tmp, sizeof(TS));

		asyslog(LOG_INFO, "[%s][%d]修正任务<%d>的上次上报时间: %04d-%02d-%02d %02d-%02d-%02d",__FUNCTION__, __LINE__,
				tfs->rptList[i][1].taskId,
				tfs->rptList[i][1].startTime.Year, tfs->rptList[i][1].startTime.Month, tfs->rptList[i][1].startTime.Day,
				tfs->rptList[i][1].startTime.Hour, tfs->rptList[i][1].startTime.Minute, tfs->rptList[i][1].startTime.Sec);
		saveCoverClass(0x6099, 0, tfs, sizeof(taskFailInfo_s), para_vari_save);
		return;
	}
}

/*
 * 如果上一次上报成功时间只比当前成功上报时间早一个周期,
 * 则说明没有漏报的数据, 将上一次上报时间修正为当前上报
 * 时间
 * 0: 有漏报的数据
 * 1: 没有漏报的数据
 */
int updateTime(int i, TI* taskInv)
{
	TS tmp = {0};
	taskFailInfo_s* tfs = (taskFailInfo_s*)dbGet("task_list");

	if ( NULL == tfs )
		return 1;

	memcpy(&tmp, &tfs->rptList[i][1].startTime, sizeof(TS));

	if(TScompare(tmp, tfs->rptList[i][0].startTime) == 1) {//如果上一次上报时间大于当前上报时间, 修正为当前上报时间
		memcpy(&tfs->rptList[i][1].startTime, &tfs->rptList[i][0].startTime, sizeof(TS));
		asyslog(LOG_INFO, "[%s][%d]修正任务<%d>的上次上报时间: %04d-%02d-%02d %02d-%02d-%02d",__FUNCTION__, __LINE__,
				tfs->rptList[i][1].taskId,
				tfs->rptList[i][1].startTime.Year, tfs->rptList[i][1].startTime.Month, tfs->rptList[i][1].startTime.Day,
				tfs->rptList[i][1].startTime.Hour, tfs->rptList[i][1].startTime.Minute, tfs->rptList[i][1].startTime.Sec);
		return 1;
	}

	tminc(&tmp, taskInv->units, taskInv->interval);

	if(TScompare(tmp, tfs->rptList[i][0].startTime) == 2)//如果有漏报的数据, 就不更新上一次上报时间
		return 0;


	//如果没有漏报的数据, 把上一次上报时间更新为当前上报时间
	memcpy(&tfs->rptList[i][1].startTime, &tfs->rptList[i][0].startTime, sizeof(TS));
	asyslog(LOG_INFO, "[%s][%d]修正任务<%d>的上次上报时间: %04d-%02d-%02d %02d-%02d-%02d",__FUNCTION__, __LINE__,
			tfs->rptList[i][1].taskId,
			tfs->rptList[i][1].startTime.Year, tfs->rptList[i][1].startTime.Month, tfs->rptList[i][1].startTime.Day,
			tfs->rptList[i][1].startTime.Hour, tfs->rptList[i][1].startTime.Minute, tfs->rptList[i][1].startTime.Sec);
	return 1;
}

void checkAndSendAppends(CommBlock* nst, INT8U *saveOver) {
	TS failts = {0};
	TS succts = {0};
	taskFailInfo_s* tfs = (taskFailInfo_s*)dbGet("task_list");
	TI taskInv = {0};
	rptInfo_s* rptInfo = (rptInfo_s*) dbGet("curr_retry_task");
	int times = 0;

	if ( NULL == tfs || NULL == saveOver ) {
		return;
	}

	for (int i = 0; i < MAXNUM_AUTOTASK; i++) {
		//如果当前没有需要发送的任务，就检查一下有没有需要补报的任务
		if (tfs->rptList[i][0].sign != 0xAA
				|| tfs->rptList[i][1].sign != 0xAA) {
			continue;
		}

		if(getTaskInterval(i, &taskInv) == 0)
			continue;

		if(updateTime(i, &taskInv) == 1)
			continue;

		memcpy(&rptInfo->sign, &tfs->rptList[i][1].sign, sizeof(rptInfo_s));
		rptInfo->pos = i;
		//先修正一下补报时间，然后去最后一次上报成功时间和补报时间
		repairTime(i, tfs);
		memcpy(&succts, &tfs->rptList[i][0].startTime, sizeof(TS));
		memcpy(&failts, &tfs->rptList[i][1].startTime, sizeof(TS));
		asyslog(LOG_INFO, "检查到任务: <%d> 的上报时间: %04d-%02d-%02d %02d-%02d-%02d, %04d-%02d-%02d %02d-%02d-%02d",
							tfs->rptList[i][1].taskId,
							succts.Year, succts.Month, succts.Day, succts.Hour, succts.Minute, succts.Sec,
							failts.Year, failts.Month, failts.Day, failts.Hour, failts.Minute, failts.Sec);

		tminc(&failts, taskInv.units, taskInv.interval);
		asyslog(LOG_INFO, "检查到任务: <%d> 的执行频率: %d-%02d",
				tfs->rptList[i][1].taskId, taskInv.units, taskInv.interval);

		int res = TScompare(failts, succts);
		if (res == 2 ) {//没有补报的时长超过一个执行周期, 则开始补报
			do {
			//取补报起始时间
			memcpy(&failts, &tfs->rptList[i][1].startTime, sizeof(TS));
			times = 0;
				//计算补报结束时间
//				if(TItoSec(taskInv) < 3600) {
//					tminc(&tfs->rptList[i][1].startTime, hour_units, 1);
//				} else {
					tminc(&tfs->rptList[i][1].startTime, taskInv.units, taskInv.interval);
//				}
					asyslog(LOG_INFO, "[%s()][%d]任务<%d>的taskInv: %d-%d, class6013.interval: %d-%d",__FUNCTION__, __LINE__,
									tfs->rptList[i][1].taskId, taskInv.units, taskInv.interval);
				res = TScompare(tfs->rptList[i][1].startTime, tfs->rptList[i][0].startTime);
				asyslog(LOG_INFO, "[%s][%d]修正任务<%d>的上次上报时间: %04d-%02d-%02d %02d-%02d-%02d",__FUNCTION__, __LINE__,
													tfs->rptList[i][1].taskId,
													tfs->rptList[i][1].startTime.Year, tfs->rptList[i][1].startTime.Month, tfs->rptList[i][1].startTime.Day,
													tfs->rptList[i][1].startTime.Hour, tfs->rptList[i][1].startTime.Minute, tfs->rptList[i][1].startTime.Sec);
				if(res == 1) {//如果结束时间晚于当前上报时间, 修正为当前上报时间
					asyslog(LOG_INFO, "任务: <%d> , 修正结束时间", tfs->rptList[i][1].taskId);
					memcpy(&tfs->rptList[i][1].startTime, &tfs->rptList[i][0].startTime, sizeof(TS));
					asyslog(LOG_INFO, "[%s][%d]修正任务<%d>的上次上报时间: %04d-%02d-%02d %02d-%02d-%02d",__FUNCTION__, __LINE__,
														tfs->rptList[i][1].taskId,
														tfs->rptList[i][1].startTime.Year, tfs->rptList[i][1].startTime.Month, tfs->rptList[i][1].startTime.Day,
														tfs->rptList[i][1].startTime.Hour, tfs->rptList[i][1].startTime.Minute, tfs->rptList[i][1].startTime.Sec);
				}
				res = TScompare(tfs->rptList[i][1].startTime, tfs->rptList[i][0].startTime);
				asyslog(LOG_INFO, "检查到任务: <%d> 的数据需要补报, 时间: %04d-%02d-%02d %02d-%02d-%02d, %04d-%02d-%02d %02d-%02d-%02d",
						tfs->rptList[i][1].taskId,
						failts.Year, failts.Month, failts.Day, failts.Hour, failts.Minute, failts.Sec,
						tfs->rptList[i][1].startTime.Year, tfs->rptList[i][1].startTime.Month, tfs->rptList[i][1].startTime.Day,
						tfs->rptList[i][1].startTime.Hour, tfs->rptList[i][1].startTime.Minute, tfs->rptList[i][1].startTime.Sec);
				supplementRpt(failts, tfs->rptList[i][1].startTime,
						tfs->rptList[i][1].taskId, saveOver);
				asyslog(LOG_INFO, "saveOver: %d", *saveOver);
				HandReportTask(nst, saveOver);	//cj report命令手动补抄进行任务上送
				saveCoverClass(0x6099, 0, tfs, sizeof(taskFailInfo_s), para_vari_save);
				times++;
				sleep(2);
			}while (res == 2 && stopSign == 0 && stopREtry != 1 && times < 228);//228=96*3, 即96点负荷曲线3天的存储深度
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

	if (stopSign == 0 && stopREtry != 1) {//没有正常曲线上报，且没有补报曲线上报
		checkAndSendAppends(nst, &shmem->cfg_para.extpara[0]);
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
			taskFailInfo_s* tfs = dbGet("task_list");
			rptInfo_s* rptInfo = (rptInfo_s*) dbGet("curr_task");
			int i = rptInfo->pos;
			TI taskInv = {};

			//这里成功上送了一次任务
			stopSign = 0;
			conformSign = 1;

			rptInfo->sign = 0xAA;
			//更新最新的成功上报任务的时间
			memcpy(&tfs->rptList[i][0], rptInfo, sizeof(rptInfo_s));
			if (getTaskInterval(i, &taskInv) == 1)
				updateTime(i, &taskInv);

			//如果上一次上报时间的sign没有初始化，就在这里初始化一下。
			if (tfs->rptList[i][1].sign != 0xAA) {
				memcpy(&tfs->rptList[i][1], rptInfo, sizeof(rptInfo_s));
				asyslog(LOG_INFO, "[%s][%d]修正任务<%d>的上次上报时间: %04d-%02d-%02d %02d-%02d-%02d",__FUNCTION__, __LINE__,
								tfs->rptList[i][1].taskId,
								tfs->rptList[i][1].startTime.Year, tfs->rptList[i][1].startTime.Month, tfs->rptList[i][1].startTime.Day,
								tfs->rptList[i][1].startTime.Hour, tfs->rptList[i][1].startTime.Minute, tfs->rptList[i][1].startTime.Sec);
			}
			stopREtry = 0;
			saveCoverClass(0x6099, 0, tfs, sizeof(taskFailInfo_s), para_vari_save);
		}
	} else if (res == REPORT_RESPONSE && stopSign == 0 && stopREtry != 1 ) {//更新当前补报成功时间
		rptInfo_s* rptInfo = (rptInfo_s*) dbGet("curr_retry_task");
		asyslog(LOG_INFO, "收到补报确认报文");
	}
}
