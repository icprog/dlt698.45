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
static int conformSign = 0; //正常上报确认标志
static int confirmSignAppend = 0; //补报确认标志

static int conformTimes = 0, bak_conformTimes = 0;

static int confirmTimesAppend = 0; //补报重复次数, 用于主站未确认时, 重发的次数, 每重发一次就递减1
static int bak_confirmTimesAppend = 0; //补报次数备份. 用于多帧发送时, 避免重新读取任务的重发次数

static int conformOverTime = 0, bak_conformOverTime = 0;

static int confirmOverTimeAppend = 0;//补报延时, 用于决定主站未确认时, 多久重发一次
static int bak_confirmOverTimeAppend = 0; //补报延时备份, 用于多帧发送时, 避免重新读取任务的超时时间

static int reportChoice = 0;

static int MoreContentSign = 0; //是否还有更多报文标示
static int MoreContentSignAppend = 0; //是否还有更多补报报文标示

static long long conformCheckId = 0; //时间任务序号
static long long conformCheckIdAppend = 0; //补报时间任务序号

//任务参数变更
static INT16S taskChangeSign = -1;
//对时标识
static INT16S timeChangeSign = -1;

//停止重复上报标志
static INT8U stopREtry = 0;

void init6013ListFrom6012File(ProgramInfo *JProgramInfo)
{
	INT16U total_autotasknum = 0;
	INT8U result = 0;
	memset(&JProgramInfo->autotask, 0, sizeof(JProgramInfo->autotask));	//增加初始化
	INT16U tIndex = 0;
	OI_698 oi = 0x6013;
	CLASS_6013 class6013 = { };

	for (tIndex = 0; tIndex < 256; tIndex++) {
		if (readCoverClass(oi, tIndex, &class6013, sizeof(CLASS_6013),
				coll_para_save) == 1) {
			if (class6013.cjtype == rept && class6013.state == 1) {
				init_autotask(total_autotasknum, class6013,
						JProgramInfo->autotask);
				total_autotasknum++;
			}
		}
	}
}

int ConformCheck(struct aeEventLoop* ep, long long id, void* clientData)
{
	CommBlock* nst = (CommBlock*) clientData;

	fprintf(stderr, "[%s()][%d]conformSign = %d\n", __FUNCTION__, __LINE__,
			conformSign);
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
 * 补报重发
 */
int repeatRptAppend(struct aeEventLoop* ep, long long id, void* clientData)
{
	CommBlock* nst = (CommBlock*) clientData;

	asyslog(LOG_INFO, "[%s()][%d]confirmSignAppend = %d\n", __FUNCTION__,
	__LINE__, confirmSignAppend);
	//在此检查上报报文是否得到确认
	if ((confirmSignAppend == 1) || (dbGet("online.type") == 0)) {
		return AE_NOMORE;
	}

	asyslog(LOG_INFO, "补报未确认,重试(%d)", confirmTimesAppend);
	//第一次调用此函数，启动任务上报

	if (confirmTimesAppend == 1) {
		confirmSignAppend = 0;
		//强制确认数据帧，跳下一帧发送
		MoreContentSignAppend = callAutoReport(REPORT_FRAME_DATA,
		REPROTNOTIFICATIONRECORDLIST, nst, 1);
		if (MoreContentSignAppend >= 1) {
			confirmTimesAppend = bak_confirmTimesAppend;
			confirmOverTimeAppend = bak_confirmOverTimeAppend;
			asyslog(LOG_INFO, "[%s()][%d]上一帧未确认, 强制跳下一帧，标识[%d],重发[%d],超时[%d秒]",
					__FUNCTION__, __LINE__, MoreContentSign, confirmTimesAppend,
					confirmOverTimeAppend);
		} else {
			stopREtry = 0;
			asyslog(LOG_INFO, "[%s()][%d]无更多补报的报文,stopREtry=%d", __FUNCTION__,
			__LINE__, stopREtry);
			return AE_NOMORE;
		}
	} else {
		MoreContentSignAppend = callAutoReport(REPORT_FRAME_DATA,
		REPROTNOTIFICATIONRECORDLIST, nst, 0);
		confirmTimesAppend--;
	}

	return confirmOverTimeAppend * 1000;
}

int getTaskInterval(int i, TI* taskInv)
{
	ProgramInfo * shmem = (ProgramInfo *) dbGet("program.info");
	taskFailInfo_s* tfs = (taskFailInfo_s*) dbGet("task_list");
	CLASS_6013 class6013 = { };

	if ( NULL == taskInv || NULL == tfs || NULL == shmem)
		return 0;

	if (readCoverClass(0x6013, shmem->autotask[tfs->rptList[i][1].pos].ID,
			&class6013, sizeof(CLASS_6013), coll_para_save) == 1) {
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

	if (NULL == tfs)
		return;

	memcpy(&tmp, &tfs->rptList[i][1].startTime, sizeof(TS));
	tminc(&tmp, hour_units, 24);
	if (TScompare(tmp, tfs->rptList[i][0].startTime) == 2) {
		memcpy(&tmp, &tfs->rptList[i][0].startTime, sizeof(TS));
		tminc(&tmp, hour_units, -24);
		memcpy(&tfs->rptList[i][1].startTime, &tmp, sizeof(TS));

		asyslog(LOG_INFO,
				"[%s][%d]修正任务<%d>的上次上报时间: %04d-%02d-%02d %02d-%02d-%02d",
				__FUNCTION__, __LINE__, tfs->rptList[i][1].taskId,
				tfs->rptList[i][1].startTime.Year,
				tfs->rptList[i][1].startTime.Month,
				tfs->rptList[i][1].startTime.Day,
				tfs->rptList[i][1].startTime.Hour,
				tfs->rptList[i][1].startTime.Minute,
				tfs->rptList[i][1].startTime.Sec);
		saveCoverClass(0x6099, 0, tfs, sizeof(taskFailInfo_s), para_vari_save);
		return;
	}
}

/*
 * 如果上一次上报成功时间只比当前成功上报时间早一个周期,
 * 则说明没有漏报的数据, 将上一次上报时间修正为当前上报
 * 时间.
 * 如果上一次上报成功时间比当前成功上报时间早,且超过
 * 一个周期, 则说明有漏报的时间点.
 * @return: 0-有漏报的数据,
 *          1-没有漏报的数据
 */
int updateTime(int i, TI* taskInv)
{
	TS tmp = { 0 };
	taskFailInfo_s* tfs = (taskFailInfo_s*) dbGet("task_list");

	if ( NULL == tfs)
		return 1;

	tmp = tfs->rptList[i][1].startTime;
	tminc(&tmp, taskInv->units, taskInv->interval);

	if (TScompare(tmp, tfs->rptList[i][0].startTime) == 2)//如果有漏报的数据, 就不更新上一次上报时间
		return 0;

	/*
	 *    如果上一次上报时间大于当前上报时间, 或者上一次上报时间增加一个周期后,
	 * 比当前上报时间晚, 则把上一次上报时间更新为当前上报时间
	 */
	tfs->rptList[i][1].startTime = tfs->rptList[i][0].startTime;
	tfs->rptList[i][1].endTime = tfs->rptList[i][0].startTime;

	return 1;
}


/*
 * 检查当前上报任务是否有漏点的.
 * 检查依据: 每个任务有两个对应结构: rptInfo_s, 保存了当前任务
 * 在nst->shmem.autotask 中的索引号, 任务成功上报的时间点,
 * 且第1个rptInfo_s保存的是正常上报的时间点, 第2个rptInfo_s
 * 保存的是补报成功的时间点. 如果第2个时间点比第1个时间点早1个
 * 任务执行周期, 则说明当前任务没有漏点. 如果第2个时间点比第1个
 * 时间点早, 且多于1个任务执行周期, 则说明当前任务有漏点,
 * 需要对当前任务进行补报.
 */
void checkAndSendAppends(struct aeEventLoop* ep, CommBlock* nst)
{
	CLASS_6013 class6013 = { 0 };
	CLASS_601D class601d = { 0 };
	TS failts = { 0 };
	TS succts = { 0 };
	taskFailInfo_s* tfs = (taskFailInfo_s*) dbGet("task_list");
	TI taskInv = { 0 };
	rptInfo_s* rptInfo = (rptInfo_s*) dbGet("curr_retry_task");	//当前补报的任务信息, 用于跟确认帧处理函数交换数据
	ProgramInfo* shmem = (ProgramInfo*) nst->shmem;
	int i = 0;

	if ( NULL == tfs || NULL == rptInfo || NULL == shmem) {
		return;
	}

	if (stopSign != 0 || stopREtry != 0) {	//stopSign == 1, 说明有正常上报的任务在处理;
											//stopREtry == 1, 说明有补报的过程在处理.
		return;
	}

	for (i = 0; i < MAXNUM_AUTOTASK; i++) {
		//如果当前没有需要发送的任务，就检查一下有没有需要补报的任务
		if (tfs->rptList[i][0].sign != 0xAA
				|| tfs->rptList[i][1].sign != 0xAA) {
			continue;
		}

		if(tfs->rptList[i][0].taskId != tfs->rptList[i][1].taskId
				||i != tfs->rptList[i][1].pos) {//todo: 方案索引或者方案编号不一致, 说明任务被重新调整过. 是否要记住调整之前的补报信息?

			asyslog(LOG_INFO, "[%s()][%d]任务: <%d, %02d, %02d> 的方案编号不一致, 丢弃以前的补报信息", __FUNCTION__,
					__LINE__, i, tfs->rptList[i][0].taskId, tfs->rptList[i][1].taskId);

			tfs->rptList[i][1].pos = i;
			tfs->rptList[i][1].taskId = tfs->rptList[i][0].taskId;
			tfs->rptList[i][1].startTime = tfs->rptList[i][0].startTime;
			tfs->rptList[i][1].endTime = tfs->rptList[i][1].startTime;
			continue;
		}

		if (getTaskInterval(i, &taskInv) == 0)
			continue;

		if (updateTime(i, &taskInv) == 1)
			continue;

		//先修正一下补报时间，然后去最后一次上报成功时间和补报时间
		repairTime(i, tfs);
		succts = tfs->rptList[i][0].startTime;
		failts = tfs->rptList[i][1].startTime;

		tminc(&failts, taskInv.units, taskInv.interval);

//		asyslog(LOG_INFO, "[%s()][%d]任务: <%d> 的执行频率: %d-%02d", __FUNCTION__,
		__LINE__, tfs->rptList[i][1].taskId, taskInv.units, taskInv.interval);

		int res = TScompare(failts, succts);
		if (res == 2) {									//没有补报的时长超过一个执行周期, 则开始补报
			//计算补报结束时间
			memcpy(&failts, &tfs->rptList[i][1].startTime, sizeof(TS));

			//如果任务执行周期大于1小时, 取1个任务周期; 否则取1小时
			if (TItoSec(taskInv) < 3600) {
				tminc(&failts, hour_units, 1);
			} else {
				tminc(&failts, taskInv.units, taskInv.interval);
			}

			res = TScompare(failts, succts);
			if (res == 1) {			//如果结束时间晚于当前上报时间, 修正为当前上报时间
				asyslog(LOG_INFO, "[%s()][%d]任务: <%d> , 修正结束时间", __FUNCTION__,
				__LINE__, tfs->rptList[i][1].taskId);
				memcpy(&failts, &succts, sizeof(TS));
			}

			if (failts.Day > tfs->rptList[i][1].startTime.Day) {//supplementRpt不支持跨日,
																//如果跨日, 开始时间前进一个周期, 结束时间=开始时间
				tminc(&tfs->rptList[i][1].startTime, taskInv.units,
						taskInv.interval);
				failts = tfs->rptList[i][1].startTime;
			}

			supplementRpt(tfs->rptList[i][1].startTime, failts,
					tfs->rptList[i][1].taskId, &shmem->cfg_para.extpara[0]);

			if(shmem->cfg_para.extpara[0] == 0) {
				continue;
			} else {
				asyslog(LOG_INFO,
									"[%s()][%d]检查到任务: <%d, %d, %d> 的数据需要补报, 时间: %04d-%02d-%02d %02d-%02d-%02d, %04d-%02d-%02d %02d-%02d-%02d",
									__FUNCTION__, __LINE__, i, tfs->rptList[i][0].taskId, tfs->rptList[i][1].taskId,
									tfs->rptList[i][1].startTime.Year,
									tfs->rptList[i][1].startTime.Month,
									tfs->rptList[i][1].startTime.Day,
									tfs->rptList[i][1].startTime.Hour,
									tfs->rptList[i][1].startTime.Minute,
									tfs->rptList[i][1].startTime.Sec, failts.Year, failts.Month,
									failts.Day, failts.Hour, failts.Minute, failts.Sec);
			}

			MoreContentSignAppend = callAutoReport(REPORT_FRAME_DATA,
			REPROTNOTIFICATIONRECORDLIST, nst, 0);						//发第一帧

			//更新当前补报任务信息
			tfs->rptList[i][1].endTime = failts;
			memcpy(rptInfo, &tfs->rptList[i][1], sizeof(rptInfo_s));
			//不再调用此函数标志
			stopREtry = 1;
			//标示补报任务尚未获得确认
			confirmSignAppend = 0;

			if (readCoverClass(0x6013, shmem->autotask[i].ID, &class6013,
					sizeof(CLASS_6013), coll_para_save) == 1) {
				if (readCoverClass(0x601D, shmem->autotask[i].SerNo, &class601d,
						sizeof(CLASS_601D), coll_para_save) == 1) {
					shmem->autotask[i].ReportNum = class601d.maxreportnum;
					shmem->autotask[i].OverTime = TItoSec(class601d.timeout);
				}
			} else {
				asyslog(LOG_INFO, "[%s()][%d]任务参数丢失！", __FUNCTION__, __LINE__);
			}

			//ReportNum的类型可能会改为有符号类型, 所以考虑小于等于0的异常情况
			if (shmem->autotask[i].ReportNum > 5 || shmem->autotask[i].ReportNum <= 0) {
				confirmTimesAppend = 5;
				asyslog(LOG_INFO, "[%s()][%d]任务重复上报设置次数[%d]过大,设置默认上报5次",
						__FUNCTION__, __LINE__, shmem->autotask[i].ReportNum);
			} else {
				confirmTimesAppend = shmem->autotask[i].ReportNum;
			}

			//OverTime的类型可能会改为有符号类型, 所以考虑小于等于0的异常情况
			if (shmem->autotask[i].OverTime > 120 || shmem->autotask[i].OverTime <= 0) {
				confirmOverTimeAppend = 120;
				asyslog(LOG_INFO, "[%s()][%d]任务重复上报超时时间[%d]秒,设置超时120秒",
						__FUNCTION__, __LINE__, shmem->autotask[i].OverTime);
			} else {
				confirmOverTimeAppend = shmem->autotask[i].OverTime;
			}

			bak_confirmTimesAppend = confirmTimesAppend;
			bak_confirmOverTimeAppend = confirmOverTimeAppend;

			//注册时间事件，检查确认状态
			conformCheckIdAppend = aeCreateTimeEvent(ep,
					confirmOverTimeAppend * 1000, repeatRptAppend, nst, NULL);
			asyslog(LOG_INFO,
					"[%s()][%d]检查到补报任务, 初始化补报状态(次数=%d-时间=%d), 注册时间事件(%d)",
					__FUNCTION__, __LINE__, confirmTimesAppend,
					confirmOverTimeAppend, conformCheckIdAppend);
			break;
		}
	}
}

void RegularAutoTask(struct aeEventLoop* ep, CommBlock* nst)
{
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
				asyslog(LOG_INFO, "[%s()][%d]任务重复上报设置次数[%d]过大,设置默认上报5次",
						__FUNCTION__, __LINE__, shmem->autotask[i].ReportNum);
			} else {
				conformTimes = shmem->autotask[i].ReportNum;
			}

			if (shmem->autotask[i].OverTime > 120) {
				conformOverTime = 120;
				asyslog(LOG_INFO, "[%s()][%d]任务重复上报超时时间[%d]秒,设置超时120秒",
						__FUNCTION__, __LINE__, shmem->autotask[i].OverTime);
			} else {
				conformOverTime = shmem->autotask[i].OverTime;
			}

			bak_conformTimes = conformTimes;			//记录第一帧无应答后,下一帧重复发送次数
			bak_conformOverTime = conformOverTime;
			//注册时间事件，检查确认状态
			conformCheckId = aeCreateTimeEvent(ep, conformOverTime * 1000,
					ConformCheck, nst, NULL);
			asyslog(LOG_INFO,
					"[%s()][%d]检查到上报任务，初始化上报状态(次数=%d-时间=%d)、注册时间事件(%d)",
					__FUNCTION__, __LINE__, conformTimes, conformOverTime,
					conformCheckId);
			break;
		}
	}

	checkAndSendAppends(ep, nst);
}

void ConformAutoTask(struct aeEventLoop* ep, CommBlock* nst, int res)
{
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
			TI taskInv = { };

			//这里成功上送了一次任务
			stopSign = 0;
			stopREtry = 0;
			conformSign = 1;

			rptInfo->sign = 0xAA;
			//更新最新的成功上报任务的时间
			memcpy(&tfs->rptList[i][0], rptInfo, sizeof(rptInfo_s));

			if (tfs->rptList[i][1].sign != 0xAA) {//如果上一次上报时间的sign没有初始化，就在这里初始化一下。
				memcpy(&tfs->rptList[i][1], rptInfo, sizeof(rptInfo_s));
			} else if (getTaskInterval(i, &taskInv) == 1) {	//如果已经初始化上次上报时间的sign, 那么更新上次上报时间
				updateTime(i, &taskInv);
			}

		   if(saveCoverClass(0x6099, 0, tfs, sizeof(taskFailInfo_s), para_vari_save) != 0) {
				  asyslog(LOG_INFO, "[%s][%d]save 6099 failed!!", __FUNCTION__, __LINE__);
		   }
		}
	} else if (res == REPORT_RESPONSE && stopSign == 0 && stopREtry == 1) {	//更新当前补报成功时间
		asyslog(LOG_INFO, "[%s()][%d]收到补报确认报文", __FUNCTION__, __LINE__);

		confirmSignAppend = 1;
		MoreContentSignAppend = callAutoReport(REPORT_FRAME_DATA, REPROTNOTIFICATIONRECORDLIST,
				nst, 1);
		if (MoreContentSignAppend == 1) {
			asyslog(LOG_INFO, "[%s()][%d]发现更多的报文，注销检查函数，任务序号(%d)", __FUNCTION__,
					__LINE__, conformCheckIdAppend);
			if (aeDeleteTimeEvent(ep, conformCheckIdAppend) == AE_OK) {	//TODO:是否需要增加删除成功,重新注册,删除失败会有什么问题?
				confirmSignAppend = 0;		//清除确认标记,方便重新进入检查函数
				confirmTimesAppend = bak_confirmTimesAppend;
				confirmOverTimeAppend = bak_confirmOverTimeAppend;
				conformCheckIdAppend = aeCreateTimeEvent(ep,
						confirmOverTimeAppend * 1000, repeatRptAppend, nst,
						NULL);
				asyslog(LOG_INFO, "[%s()][%d]重新注册, 任务序号(%d), (次数=%d-时间=%d)",
						__FUNCTION__, __LINE__, conformCheckIdAppend,
						confirmTimesAppend, confirmOverTimeAppend);
			}
		} else {
			taskFailInfo_s* tfs = dbGet("task_list");
			rptInfo_s* rptInfo = (rptInfo_s*) dbGet("curr_retry_task");
			int i = rptInfo->pos;

			//这里成功上送了一次任务
			stopREtry = 0;
			confirmSignAppend = 1;

			//更新当前补报任务的开始时间
			tfs->rptList[i][1].startTime = tfs->rptList[i][1].endTime;

			stopREtry = 0;
			asyslog(LOG_INFO, "补报结束,删除文件");
			if (unlink(REPORT_FRAME_DATA) == 0) {
				ProgramInfo* shmem = (ProgramInfo*) nst->shmem;
				shmem->cfg_para.extpara[0] = 0;
				asyslog(LOG_INFO, "补报文件删除成功");
			} else {
				asyslog(LOG_INFO, "补报文件删除失败");
			}

			if (aeDeleteTimeEvent(ep, conformCheckIdAppend) == AE_OK) {
			        asyslog(LOG_INFO, "[%s()][%d]删除重发成功",
			                           __FUNCTION__, __LINE__);
			}

			saveCoverClass(0x6099, 0, tfs, sizeof(taskFailInfo_s),
					para_vari_save);
		}
	}
}
