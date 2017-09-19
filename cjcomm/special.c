/*
 * special.c
 *
 *  Created on: 2017-7-27
 *      Author: zhoulihai
 */

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
#include "cjcomm.h"
#include "basedef.h"

void specialCheckF101Change() {
	static int ChangeFlag = 0;
	ProgramInfo *info = (ProgramInfo *) dbGet("program.info");
	if (ChangeFlag != info->oi_changed.oiF101) {
		asyslog(LOG_WARNING, "检测到安全参数变化！刷新安全参数！");
		readCoverClass(0xf101, 0, &(((CommBlock*) dbGet("block.net"))->f101),
				sizeof(CLASS_F101), para_vari_save);
		readCoverClass(0xf101, 0, &(((CommBlock*) dbGet("block.gprs"))->f101),
				sizeof(CLASS_F101), para_vari_save);
		ChangeFlag = info->oi_changed.oiF101;
	}
}

void specialClear() {
	ProgramInfo *info = (ProgramInfo *) dbGet("program.info");
	int index = (int) dbGet("prog.index");
	info->Projects[index].WaitTimes = 0;
}

void specialCheck4510Change() {
	static int ChangeFlag = 0;
	ProgramInfo *info = (ProgramInfo *) dbGet("program.info");
	CLASS26* c26 = (CLASS26*) dbGet("class26");

	if (ChangeFlag != info->oi_changed.oi4510) {
		asyslog(LOG_WARNING, "检测到以太网通信参数变化！刷新主站参数！");
		readCoverClass(0x4510, 0, c26, sizeof(CLASS26), para_vari_save);
		((CommBlock*) dbGet("block.net"))->Heartbeat =
				c26->commconfig.heartBeat;
		ChangeFlag = info->oi_changed.oi4510;
	}
}

void specialCheck4500Change() {
	static int ChangeFlag = 0;
	ProgramInfo *info = (ProgramInfo *) dbGet("program.info");
	CLASS25* c25 = (CLASS25*) dbGet("class25");

	if (ChangeFlag != info->oi_changed.oi4500) {
		asyslog(LOG_WARNING, "检测到GPRS通信参数变化！刷新主站参数！");
		readCoverClass(0x4500, 0, c25, sizeof(CLASS25), para_vari_save);
		((CommBlock*) dbGet("block.gprs"))->Heartbeat =
				c25->commconfig.heartBeat;
		ChangeFlag = info->oi_changed.oi4500;
	}
}

void specialTransFlow() {
	static TS ts = { };
	static TS old = { };
	ProgramInfo *info = (ProgramInfo *) dbGet("program.info");

	if (old.Year == 0) {
		TSGet(&old);
	}

	TSGet(&ts);
	if (old.Sec != ts.Sec) {
		old.Sec = ts.Sec;
	} else {
		return;
	}

	int val = dbGet("calc.new");
	dbSet("calc.new", 0);

	Day_Mon_TJ * Tj_info = &info->dev_info.realTimeC2200.flow;

	Tj_info->day_tj += val;
	Tj_info->month_tj += val;

	Event_3110(Tj_info->month_tj, sizeof(Day_Mon_TJ), info);

	if (old.Minute != ts.Minute && ts.Minute % 2 == 0) {
		old.Minute = ts.Minute;
		if (old.Day != ts.Day) {
			asyslog(LOG_INFO, "检测到夸日,清零前数据(%d)", Tj_info->day_tj);
			Save_TJ_Freeze(0x2200, 0x0200, 0, ts, sizeof(Flow_tj),
					(INT8U *) &info->dev_info.realTimeC2200);
			Tj_info->day_tj = 0;
			old.Day = ts.Day;
		}
		if (old.Month != ts.Month) {
			asyslog(LOG_INFO, "检测到夸月,清零前数据(%d)", Tj_info->month_tj);
			Save_TJ_Freeze(0x2200, 0x0200, 1, ts, sizeof(Flow_tj),
					(INT8U *) &info->dev_info.realTimeC2200);
			Tj_info->month_tj = 0;
			old.Month = ts.Month;
		}
		saveVariData(0x2200, 0, &info->dev_info.realTimeC2200, sizeof(Flow_tj));
	}

	return;
}

int specialPowState(ProgramInfo *JProgramInfo) {
	int off_flag = 0;

	//II型
	if (JProgramInfo->cfg_para.device == CCTT2) {
		off_flag =
				pwr_down_byVolt(JProgramInfo->ACSRealData.Available,
						JProgramInfo->ACSRealData.Ua,
						JProgramInfo->event_obj.Event3106_obj.poweroff_para_obj.screen_para_obj.happen_voltage_limit);
	} else {
		BOOLEAN gpio_5V = pwr_has();
		if ((JProgramInfo->ACSRealData.Ua < 100)
				&& (JProgramInfo->ACSRealData.Ub < 100)
				&& (JProgramInfo->ACSRealData.Uc < 100) && (!gpio_5V)) {
			off_flag = 1;
		}
	}
	return off_flag;
}

int specialCheckPow() {
	static int count = 0;
	ProgramInfo *info = (ProgramInfo *) dbGet("program.info");

	if (specialPowState(info) == 1) {
		count++;
	} else {
		count = 0;
	}

	if (count > 120) {
		while (specialPowState(info) == 1) {
			sleep(2);
			asyslog(LOG_INFO, "检测到设备掉电一分钟，停止所有通信...");
		}
		asyslog(LOG_INFO, "检测到复电，继续通信...");
		count = 0;
	}
}

void SpecialCheck4001Change() {
	static int change_flag = 0;

	ProgramInfo *info = (ProgramInfo *) dbGet("program.info");
	if (change_flag != info->oi_changed.oi4001) {
		asyslog(LOG_INFO, "检测到4001参数变化，更新commblock...");
		change_flag = info->oi_changed.oi4001;
		CommBlock * tmp = 0;
		CLASS_4001_4002_4003 c4001;

		memset(&c4001, 0x00, sizeof(c4001));
		readCoverClass(0x4001, 0, &c4001, sizeof(c4001), para_vari_save);

		tmp = (CommBlock *) dbGet("block.ifr");
		memcpy(tmp->serveraddr, c4001.curstom_num, 16);

		tmp = (CommBlock *) dbGet("block.net");
		memcpy(tmp->serveraddr, c4001.curstom_num, 16);

		tmp = (CommBlock *) dbGet("block.gprs");
		memcpy(tmp->serveraddr, c4001.curstom_num, 16);

		tmp = (CommBlock *) dbGet("block.serial");
		memcpy(tmp->serveraddr, c4001.curstom_num, 16);
		asyslog(LOG_INFO, "更新commblock完毕...");
	}
}

int SpecialRegular(struct aeEventLoop *ep, long long id, void *clientData) {
	int shandong = (int) clientData;

	specialClear();
	SpecialCheck4001Change();
	specialCheckF101Change();
	specialCheck4500Change();
	specialCheck4510Change();
	specialTransFlow();
	ATUpdateStatus(AtGet());
	if (shandong == 1) {
		specialCheckPow();
	}
	return 500;
}

/*
 * 供外部使用的初始化函数，并开启维护循环
 */
int StartSecial(struct aeEventLoop *ep, long long id, void *clientData) {
	int shandong = 0;
	if (getZone("ShanDong") == 0) {
		shandong = 1;
	}

	int sid = aeCreateTimeEvent(ep, 1000, SpecialRegular, shandong, NULL);
	return 1;
}
