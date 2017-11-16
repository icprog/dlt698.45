/*
 * pulse.c
 *
 *  Created on: Aug 16, 2017
 *      Author: z
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "pluse.h"
#include "Shmem.h"
#include "Objectdef.h"
#include "AccessFun.h"

extern ProgramInfo* JProgramInfo;

void pluseGetCount(int *pulse) {
	unsigned int buf[2];
	int fd = open("/dev/pulse", O_RDWR);
	if (fd > 0) {
		read(fd, buf, sizeof(unsigned int) * 2);
		pulse[0] = buf[0];
		pulse[1] = buf[1];
	}
	close(fd);
}

int pluseGetTimeZone() {
	TS ts;
	TSGet(&ts);
	fprintf(stderr, "VgetNowZone %d\n", ts.Hour);
	if (ts.Hour < 6) {
		return 0;
	} else if (ts.Hour < 12) {
		return 1;
	} else if (ts.Hour < 18) {
		return 2;
	}
	return 3;
}

//根据脉冲计算电量
void pluseCalcDD(PluseUnit * pu) {
	for (int index; index < 2; index++) {
		//检查变化
		int pulse = pu->pNow[index] - pu->pDD[index];
		if (pulse > 0) {
			pu->pDD[index] = pu->pNow[index];
			pu->class12[index].pluse_count += pulse;
		} else {
			continue;
		}

		//检查参数
		if (pu->class12[index].unit[index].k == 0) {
			continue;
		}

		float con = pu->class12[index].unit[index].k;
		con = (pulse * 10000.0) / con;

		int time_zone = pluseGetTimeZone();

		switch (pu->class12[index].unit[index].conf) {
		case 0:
			pu->class12[index].val_pos_p[time_zone] += con;
			pu->class12[index].day_pos_p[time_zone] += con;
			pu->class12[index].mon_pos_p[time_zone] += con;
			fprintf(stderr, "[CTRL]实时正向有功 %d\n",
					pu->class12[index].val_pos_p[time_zone]);
			break;
		case 2:
			pu->class12[index].val_nag_p[time_zone] += con;
			pu->class12[index].day_nag_p[time_zone] += con;
			pu->class12[index].mon_nag_p[time_zone] += con;
			fprintf(stderr, "[CTRL]实时反向有功 %d\n",
					pu->class12[index].val_nag_p[time_zone]);
			break;
		case 1:
			pu->class12[index].val_pos_q[time_zone] += con;
			pu->class12[index].day_pos_q[time_zone] += con;
			pu->class12[index].mon_pos_q[time_zone] += con;
			fprintf(stderr, "[CTRL]实时正向无功 %d\n",
					pu->class12[index].val_pos_q[time_zone]);
			break;
		case 3:
			pu->class12[index].val_nag_q[time_zone] += con;
			pu->class12[index].day_nag_q[time_zone] += con;
			pu->class12[index].mon_nag_q[time_zone] += con;
			fprintf(stderr, "[CTRL]实时反向无功 %d\n",
					pu->class12[index].val_nag_q[time_zone]);
			break;
		}
	}
}

//计算周期内实时功率
int pluseCalcPQ(PluseUnit * pu, int pulse, int index) {
	//检查参数
	if (pu->class12[index].unit[index].k == 0) {
		return 0;
	}

	float con = pu->class12[index].unit[index].k;
	con = (pulse * 60.0 * 10000.0) / con;

	switch (pu->class12[index].unit[index].conf) {
	case 0:
	case 2:
		JProgramInfo->class12[index].p = con;
		fprintf(stderr, "[CTRL]实时有功功率 %d\n\n\n\n\n\n\n\n\n\n\n",
				pu->class12[index].p);
		break;
	case 1:
	case 3:
		JProgramInfo->class12[index].q = con;
		fprintf(stderr, "[CTRL]实时无功功率 %d\n\n\n\n\n\n\n\n\n\n\n",
				pu->class12[index].q);
		break;
	}
}

int pluseInitUnit(PluseUnit * pu, ProgramInfo* JProgramInfo) {
	pluseGetCount(&pu->pNow[0]);

	for (int i = 0; i < 2; i++) {
		pu->step[i] = 0;
		pu->pDD[i] = pu->pNow[i];
		pu->pPQ[i] = pu->pNow[i];
	}

	pu->class12 = &JProgramInfo->class12[0];
	pu->class12[0].pluse_count = pu->pNow[0];
	pu->class12[1].pluse_count = pu->pNow[1];

	TS ts;
	TSGet(&ts);

	pu->old_day = ts.Day;
	pu->old_month = ts.Month;
}

void pluseRefreshUnit(PluseUnit * pu) {
	pluseGetCount(pu->pNow);
	fprintf(stderr, "[CTRL]脉冲计数%d-%d\n", pu->pNow[0], pu->pNow[1]);
	pluseCalcDD(pu);

	for (int i = 0; i < 2; i++) {
		switch (pu->step[i]) {
		case 0:
			if (pu->pNow[i] >= pu->pPQ[i]) {
				pu->last_time = time(NULL);
				pu->step[i] = 1;
			}
			break;
		case 1:
			if (abs(time(NULL) - pu->last_time) >= 59) {
				pluseCalcPQ(pu, pu->pNow[i] - pu->pPQ[i], i);
				pu->pPQ[i] = pu->pNow[i];
				pu->step[i] = 0;
			}
			break;
		default:
			pu->step[i] = 0;
			break;
		}
	}

	//跨日、月清零
	TS ts;
	TSGet(&ts);

	if (pu->old_day != ts.Day) {
		pu->old_day = ts.Day;
		for (int i = 0; i < 2; i++) {
			int size = sizeof(INT32U) * MAXVAL_RATENUM;
			memset(pu->class12[i].day_nag_p, 0x00, size);
			memset(pu->class12[i].day_nag_q, 0x00, size);
			memset(pu->class12[i].day_pos_p, 0x00, size);
			memset(pu->class12[i].day_pos_q, 0x00, size);
		}
	}

	if (pu->old_month != ts.Month) {
		pu->old_month = ts.Month;
		for (int i = 0; i < 2; i++) {
			int size = sizeof(INT32U) * MAXVAL_RATENUM;
			memset(pu->class12[i].mon_nag_p, 0x00, size);
			memset(pu->class12[i].mon_nag_q, 0x00, size);
			memset(pu->class12[i].mon_pos_p, 0x00, size);
			memset(pu->class12[i].mon_pos_q, 0x00, size);
		}
	}
}
