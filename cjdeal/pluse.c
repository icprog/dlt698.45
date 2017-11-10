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

void getPluseCount(unsigned int *pulse) {
	int fd = open("/dev/pulse", O_RDWR);
	read(fd, pulse, sizeof(unsigned int) * 2);
	close(fd);
	fprintf(stderr, "刷新脉冲 %d-%d\n", pulse[0], pulse[1]);
}

int getNowZone() {
	TS ts;
	TSGet(&ts);
	fprintf(stderr, "VgetNowZone %d\n", ts.Hour);
	if (ts.Hour < 6) {
		return 0;
	}else if (ts.Hour < 12) {
		return 1;
	}else if (ts.Hour < 18) {
		return 2;
	}
	return 3;
}

//根据脉冲计算电量
void cacl_DD(unsigned int pulse, int index) {
	int con = JProgramInfo->class12[index].unit[0].k;
	if (con == 0) {
		return;
	}
	if((JProgramInfo->class12[index].pt * JProgramInfo->class12[index].ct) == 0)
	{
		return;
	}

	con = con/(JProgramInfo->class12[index].pt * JProgramInfo->class12[index].ct);

	int time_zone = getNowZone();
	switch (JProgramInfo->class12[index].unit[0].conf) {

	case 0:
		//正向有功 = 脉冲总数 * 1000/con;
		JProgramInfo->class12[index].val_pos_p[time_zone] += pulse * 10 * con;
		JProgramInfo->class12[index].day_pos_p[time_zone] += pulse * 10 * con;
		JProgramInfo->class12[index].mon_pos_p[time_zone] += pulse * 10 * con;
		fprintf(stderr, "实时正向有功 %d\n", JProgramInfo->class12[index].p);
		break;
	case 2:
		JProgramInfo->class12[index].val_nag_p[time_zone] += pulse * 10 * con;
		JProgramInfo->class12[index].day_nag_p[time_zone] += pulse * 10 * con;
		JProgramInfo->class12[index].mon_nag_p[time_zone] += pulse * 10 * con;
		fprintf(stderr, "实时反向有功 %d\n", JProgramInfo->class12[index].p);
		break;
	case 1:
		//正向无功 = 脉冲总数 * 100/con + 脉冲总数%10;
		JProgramInfo->class12[index].val_pos_q[time_zone] += pulse * 10 * con;
		JProgramInfo->class12[index].day_pos_q[time_zone] += pulse * 10 * con;
		JProgramInfo->class12[index].mon_pos_q[time_zone] += pulse * 10 * con;
		fprintf(stderr, "实时正向无功 %d\n", JProgramInfo->class12[index].p);
		break;

	case 3:
		//反向无功 = 脉冲总数 * 100/con + 脉冲总数%10;
		JProgramInfo->class12[index].val_nag_q[time_zone] += pulse * 10 * con;
		JProgramInfo->class12[index].day_nag_q[time_zone] += pulse * 10 * con;
		JProgramInfo->class12[index].mon_nag_q[time_zone] += pulse * 10 * con;
		fprintf(stderr, "实时反向无功 %d\n", JProgramInfo->class12[index].p);
		break;
	}
}

//计算周期内实时功率
void cacl_PQ(unsigned int pulse, int index) {
	INT64U con = JProgramInfo->class12[index].unit[0].k;
	if (con == 0) {
		return;
	}

	if((JProgramInfo->class12[index].pt * JProgramInfo->class12[index].ct) == 0)
	{
		return;
	}

	con = con / (JProgramInfo->class12[index].pt * JProgramInfo->class12[index].ct);

	switch (JProgramInfo->class12[index].unit[0].conf) {
	case 0:
	case 2:
		//瞬时有功功率 = 实时脉冲*3600*变比/60/con
		JProgramInfo->class12[index].p = (pulse*60*10) * con;  //扩大10倍
		fprintf(stderr, "实时功率 %d  -- [pulse=%d  脉冲常数=%d]\n", (int)JProgramInfo->class12[index].p,pulse,(int)con);
		break;
	case 1:
	case 3:
		//瞬时无功功率 = 实时脉冲*3600*变比/60/con
		JProgramInfo->class12[index].q =  (1000*pulse*60*10)/con;
		break;
	}

/*
//	double k = (JProgramInfo->class12[index].ct * JProgramInfo->class12[index].pt)
	double k = (1000)
			/ (double)(con*1.0);

	fprintf(stderr, "cacl_PQ in k = %f! index= %d!!!!++++%lld   %lld+++++++++\n", k, index, JProgramInfo->class12[index].ct, JProgramInfo->class12[index].pt);

	switch (JProgramInfo->class12[index].unit[0].conf) {
	case 0:
	case 2:
		//瞬时有功功率 = 实时脉冲*3600*变比/60/con
		JProgramInfo->class12[index].p = pulse * 600 * k;
		fprintf(stderr, "实时功率 %d\n", JProgramInfo->class12[index].p);
		break;
	case 1:
	case 3:
		//瞬时无功功率 = 实时脉冲*3600*变比/60/con
		JProgramInfo->class12[index].q = pulse * 600 * k;
		break;
	}
*/
}

void refreshPluse(int sec) {
	static int first = 1;
	static int pluseCountOld[2] = { 0, 0 };
	static int pluseCountPeriod[2] = { 0, 0 };

	unsigned int pluse[2] = { 0, 0 };

	getPluseCount(pluse);

	if (first == 1) {
		for (int i = 0; i < 2; i++) {
			pluseCountOld[i] = pluse[i];
			pluseCountPeriod[i] = pluse[i];
		}
		first = 0;
	}

	for (int i = 0; i < 2; i++) {
		if (pluse[i] > pluseCountOld[i]) {
			int val = pluse[i] - pluseCountOld[i];
			JProgramInfo->class12[i].pluse_count += val;
			cacl_DD(val, i);
			pluseCountOld[i] = pluse[i];
		}
	}

	static int step[2];
	static time_t last_time;

	for(int i = 0; i < 2; i++) {
		switch (step[i]) {
		case 0:
			if (pluse[i] >= pluseCountPeriod[i]) {
				last_time = time(NULL);
				step[i] = 1;
			}
			break;
		case 1:
			if(abs(time(NULL) - last_time) >= 60){
				int val = pluse[i] - pluseCountPeriod[i];
				cacl_PQ(val, i);
				pluseCountPeriod[i] = pluse[i];
				step[i] = 0;
			}
			break;
		default:
			step[i] = 0;
			break;
		}
	}


//	if (sec == 15) {
//		for (int i = 0; i < 2; i++) {
//			if (pluse[i] >= pluseCountPeriod[i]) {
//				int val = pluse[i] - pluseCountPeriod[i];
//				cacl_PQ(val, i);
//				pluseCountPeriod[i] = pluse[i];
//			}
//		}
//	}
}
