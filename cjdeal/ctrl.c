//
// Created by 周立海 on 2017/4/24.
//

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <Shmem.h>

#include "AccessFun.h"
#include "basedef.h"
#include "crtl_base.h"
#include "ctrl.h"
#include "pluse.h"
#include "assert.h"
#include "PublicFunction.h"
#include "event.h"

#include "stb.h"

extern ProgramInfo* JProgramInfo;
CtrlState* CtrlC;

static ctrlUN ctrlunit, ctrlunit_old;
static INT8U F206_state = 0;

int sumUpfindPulse(SumUpUnit* suu, PluseUnit * pu, int sum_i, int p_i, int* mmm) {
	int res = 0;
//	fprintf(stderr, "sumUpfindPulse sum_i\n");
//	if(sum_i == 0 && p_i == 0) {
//		fprintf(stderr, "sumUpfindPulse %d %d %d %d %d\n", suu->class23[0].allist[0].tsa.addr[0],suu->class23[0].allist[0].tsa.addr[1],suu->class23[0].allist[0].tsa.addr[2],suu->class23[0].allist[0].tsa.addr[3],suu->class23[0].allist[0].tsa.addr[4]);
//		fprintf(stderr, "sumUpfindPulse %d %d %d %d %d\n", pu->class12[0].addr[0],pu->class12[0].addr[1],pu->class12[0].addr[2],pu->class12[0].addr[3],pu->class12[0].addr[4]);
//		exit(0);
//	}
	for (int i = 0; i < 8; i++) {
		if (suu->class23[sum_i].allist[i].tsa.addr[0] == 0
				|| suu->class23[sum_i].allist[i].tsa.addr[0] > 17) {
			continue;
		}

		for (int a_i = 0; a_i < suu->class23[sum_i].allist[i].tsa.addr[0] - 1; a_i++) {
			if (suu->class23[sum_i].allist[i].tsa.addr[a_i + 2] != pu->class12[p_i].addr[a_i + 1]) {
				res = 0;
				break;
			}
			res = 1;
		}
		if (res == 1) {
			*mmm = i;
			break;
		}
	}
	return res;
}

void sumUpInitUnit(SumUpUnit* suu, ProgramInfo* JProgramInfo) {
	memset(suu, 0x00, sizeof(SumUpUnit));

	suu->class23 = JProgramInfo->class23;

	for (int i = 0; i < 8; i++) {
		suu->prev[i] = JProgramInfo->class23[i].DayPALL;
		for (int j = 0; j < 8; j++) {
			for (int k = 0; k < 5; k++) {
				suu->curP[i][j][k] = JProgramInfo->class23[i].allist[j].curP[k];
				suu->curQ[i][j][k] = JProgramInfo->class23[i].allist[j].curQ[k];
				suu->curNP[i][j][k] = JProgramInfo->class23[i].allist[j].curNP[k];
				suu->curNQ[i][j][k] = JProgramInfo->class23[i].allist[j].curNQ[k];
			}
		}
	}

	TS ts;
	TSGet(&ts);

	suu->old_day = ts.Day;
	suu->old_month = ts.Month;
}

//刷新总加组
void SumUpRefreshUnit(SumUpUnit* suu) {
	for (int sum_i = 0; sum_i < 8; sum_i++) {
		INT64U tmp_remains = 0;
		for (int al_i = 0; al_i < 8; al_i++) {
//			if(sum_i == 0){
//				fprintf(stderr, "8[%d]=========%lld %lld %lld %lld %lld\n", al_i, suu->curP[sum_i][al_i][0],suu->curP[sum_i][al_i][1],suu->curP[sum_i][al_i][2],suu->curP[sum_i][al_i][3],suu->curP[sum_i][al_i][4]);
//				fprintf(stderr, "6[%d]=========%lld %lld %lld %lld %lld\n", al_i, suu->class23[sum_i].allist[al_i].curP[0],suu->class23[sum_i].allist[al_i].curP[1],suu->class23[sum_i].allist[al_i].curP[2],suu->class23[sum_i].allist[al_i].curP[3],suu->class23[sum_i].allist[al_i].curP[4]);
//			}
			int cal_flag = (suu->class23[sum_i].allist[al_i].cal_flag == 0) ? 1 : -1;
			if (suu->class23[sum_i].allist[al_i].al_flag == 0) {
				for (int rate_i = 0; rate_i < 5; rate_i++) {
					INT64S tmp = suu->class23[sum_i].allist[al_i].curP[rate_i]
							- suu->curP[sum_i][al_i][rate_i];
					tmp_remains = (tmp <= 0) ? 0 : tmp;
					tmp_remains *= cal_flag;
					if (sum_i == 0 && al_i == 0) {
						fprintf(stderr, "tmp_remains %lld\n", tmp_remains);
					}
					suu->curP[sum_i][al_i][rate_i] = suu->class23[sum_i].allist[al_i].curP[rate_i];
					if (rate_i > 0) {
						suu->class23[sum_i].DayP[rate_i - 1] += tmp_remains;
						suu->class23[sum_i].MonthP[rate_i - 1] += tmp_remains;
					} else {
						suu->class23[sum_i].DayPALL += tmp_remains;
						suu->class23[sum_i].MonthPALL += tmp_remains;
					}
				}
				for (int rate_i = 0; rate_i < 5; rate_i++) {
					INT64U tmp = suu->class23[sum_i].allist[al_i].curQ[rate_i]
							- suu->curQ[sum_i][al_i][rate_i];
					tmp_remains = (tmp <= 0) ? 0 : tmp;
					tmp_remains *= cal_flag;
					suu->curQ[sum_i][al_i][rate_i] = suu->class23[sum_i].allist[al_i].curQ[rate_i];
					if (rate_i > 0) {
						suu->class23[sum_i].DayQ[rate_i - 1] += tmp_remains;
						suu->class23[sum_i].MonthQ[rate_i - 1] += tmp_remains;
					} else {
						suu->class23[sum_i].DayQALL += tmp_remains;
						suu->class23[sum_i].MonthQALL += tmp_remains;
					}
				}
			} else {
				for (int rate_i = 0; rate_i < 5; rate_i++) {
					INT64U tmp = suu->class23[sum_i].allist[al_i].curNP[rate_i]
							- suu->curNP[sum_i][al_i][rate_i];
					tmp_remains = (tmp <= 0) ? 0 : tmp;
					tmp_remains *= cal_flag;
					suu->curNP[sum_i][al_i][rate_i] =
							suu->class23[sum_i].allist[al_i].curNP[rate_i];
					if (rate_i > 0) {
						suu->class23[sum_i].DayP[rate_i - 1] += tmp_remains;
						suu->class23[sum_i].MonthP[rate_i - 1] += tmp_remains;
					} else {
						suu->class23[sum_i].DayPALL += tmp_remains;
						suu->class23[sum_i].MonthPALL += tmp_remains;
					}
				}

				for (int rate_i = 0; rate_i < 5; rate_i++) {
					INT64U tmp = suu->class23[sum_i].allist[al_i].curNQ[rate_i]
							- suu->curNQ[sum_i][al_i][rate_i];
					tmp_remains = (tmp <= 0) ? 0 : tmp;
					tmp_remains *= cal_flag;
					suu->curNQ[sum_i][al_i][rate_i] =
							suu->class23[sum_i].allist[al_i].curNQ[rate_i];
					if (rate_i > 0) {
						suu->class23[sum_i].DayQ[rate_i - 1] += tmp_remains;
						suu->class23[sum_i].MonthQ[rate_i - 1] += tmp_remains;
					} else {
						suu->class23[sum_i].DayQALL += tmp_remains;
						suu->class23[sum_i].MonthQALL += tmp_remains;
					}
				}
			}
		}
	}

	fprintf(stderr, "before 总加组功率%lld 电表 %lld  电表电量【%lld】%lld     %lld %lld %lld [%lld]\n", suu->class23[0].p,
			suu->curP[0][0][0],suu->class23[0].allist[0].curP[0],
			suu->class23[0].DayP[0], suu->class23[0].DayP[1], suu->class23[0].DayP[2],
			suu->class23[0].DayP[3], suu->class23[0].remains);
}

void SumUpRefreshPulseUnit(SumUpUnit* suu, PluseUnit * pu) {
	for (int s_i = 0; s_i < 8; s_i++) {
		//计算脉冲测量点在总加组中的数据
		for (int p_i = 0; p_i < 2; p_i++) {
//			fprintf(stderr, "SumUpRefreshPulseUnit [%d %d] %d %d\n", s_i, p_i, pu->class12[p_i].ct, pu->class12[p_i].pt);
			if (pu->class12[p_i].ct == 0 || pu->class12[p_i].pt == 0) {
				continue;
			}

			float con = pu->class12[p_i].ct * pu->class12[p_i].pt;

			int index = 0;
			if (sumUpfindPulse(suu, pu, s_i, p_i, &index) == 0) {
				continue;
			}
			fprintf(stderr, "找到总加组-脉冲配对 %d %d %d\n", s_i, p_i, index);
			suu->class23[s_i].p = (INT64S)(pu->class12[p_i].p * con);
			suu->class23[s_i].q = (INT64S)(pu->class12[p_i].q * con);

			suu->class23[s_i].allist[index].curP[0] = 0;
			suu->class23[s_i].allist[index].curQ[0] = 0;
			suu->class23[s_i].allist[index].curNP[0] = 0;
			suu->class23[s_i].allist[index].curNQ[0] = 0;

			for (int i = 1; i < 5; i++) {
				suu->class23[s_i].allist[index].curP[i] = (INT64U)(pu->class12[p_i].val_pos_p[i - 1] * con);
				suu->class23[s_i].allist[index].curQ[i] = (INT64U)(pu->class12[p_i].val_pos_q[i - 1] * con);
				suu->class23[s_i].allist[index].curNP[i] = (INT64U)(pu->class12[p_i].val_nag_p[i - 1] * con);
				suu->class23[s_i].allist[index].curNQ[i] = (INT64U)(pu->class12[p_i].val_nag_q[i - 1] * con);

				suu->class23[s_i].allist[index].curP[0] += pu->class12[p_i].val_pos_p[i - 1] * con;
				suu->class23[s_i].allist[index].curQ[0] += pu->class12[p_i].val_pos_q[i - 1] * con;
				suu->class23[s_i].allist[index].curNP[0] += pu->class12[p_i].val_nag_p[i - 1] * con;
				suu->class23[s_i].allist[index].curNQ[0] += pu->class12[p_i].val_nag_q[i - 1] * con;
			}
			if (s_i == 0) {
				printf("[1]%lld %lld %lld %lld %lld\n", suu->class23[s_i].allist[index].curP[0],
						suu->class23[s_i].allist[index].curP[1],
						suu->class23[s_i].allist[index].curP[2],
						suu->class23[s_i].allist[index].curP[3],
						suu->class23[s_i].allist[index].curP[4]);
			}
		}

	}
}

void sumUpCalcRemain(SumUpUnit* suu) {
	for (int s_i = 0; s_i < 8; s_i++) {
		//根据总加组的日有功电量计算计算剩余电量
		INT64U tmp = (suu->class23[s_i].DayPALL - suu->prev[s_i]);
		suu->prev[s_i] = suu->class23[s_i].DayPALL;
		suu->class23[s_i].remains -= tmp;
		if (suu->class23[s_i].remains < 0) {
			suu->class23[s_i].remains = 0;
		}
	}

	//跨日、月清零
	TS ts;
	TSGet(&ts);

	if (suu->old_day != ts.Day) {
		suu->old_day = ts.Day;
		for (int i = 0; i < 8; i++) {
			int size = sizeof(INT64S) * MAXVAL_RATENUM;
			memset(suu->class23[i].DayP, 0x00, size);
			memset(suu->class23[i].DayQ, 0x00, size);
			suu->class23[i].DayPALL = 0;
			suu->class23[i].DayQALL = 0;
			suu->prev[i] = 0;
		}
	}

	if (suu->old_month != ts.Month) {
		suu->old_month = ts.Month;
		for (int i = 0; i < 8; i++) {
			int size = sizeof(INT64S) * MAXVAL_RATENUM;
			memset(suu->class23[i].MonthP, 0x00, size);
			memset(suu->class23[i].MonthQ, 0x00, size);
			suu->class23[i].MonthPALL = 0;
			suu->class23[i].MonthQALL = 0;
			suu->prev[i] = 0;
		}
	}
	fprintf(stderr, "总加组功率%lld 电量(%lld)%lld %lld %lld %lld - (%lld)%lld %lld %lld %lld [%lld]\n",
			suu->class23[0].p, suu->class23[0].DayPALL, suu->class23[0].DayP[0],
			suu->class23[0].DayP[1], suu->class23[0].DayP[2], suu->class23[0].DayP[3],
			suu->class23[0].MonthPALL, suu->class23[0].MonthP[0], suu->class23[0].MonthP[1],
			suu->class23[0].MonthP[2], suu->class23[0].MonthP[3], suu->class23[0].remains);
}

int initAll() {

//控制状态初始化
	ctrlunit.u16b = 0;
	ctrlunit_old.u16b = 0;

	fprintf(stderr, "==============================ctrl init============================\n");

//读取总加组数据
	CtrlC = &JProgramInfo->ctrls;
	CtrlC->control_event = 0;
	memset(CtrlC, 0x00, sizeof(CtrlState));

	for (int i = 0; i < 8; ++i) {
		memset(&JProgramInfo->class23[i], 0x00, sizeof(CLASS23));
		readCoverClass(0x2301 + i, 0, &JProgramInfo->class23[i], sizeof(CLASS23), para_vari_save);
		JProgramInfo->class23[i].alCtlState.OutputState = 0;
		JProgramInfo->class23[i].alCtlState.BuyOutputState = 0;
		JProgramInfo->class23[i].alCtlState.MonthOutputState = 0;
		JProgramInfo->class23[i].alCtlState.PCAlarmState = 0;
		JProgramInfo->class23[i].alCtlState.ECAlarmState = 0;
	}

	for (int i = 0; i < 2; ++i) {
		memset(&JProgramInfo->class12[i], 0x00, sizeof(CLASS12));
		readCoverClass(0x2401 + i, 0, &JProgramInfo->class12[i], sizeof(CLASS12), para_vari_save);
		fprintf(stderr, "%d, %d, %d\n\n\n\n\n", JProgramInfo->class12[i].pt, JProgramInfo->class12[i].ct, JProgramInfo->class12[i].pluse_count);
	}
	sleep(5);

	readCoverClass(0x8100, 0, &CtrlC->c8100, sizeof(CLASS_8100), para_vari_save);
	readCoverClass(0x8101, 0, &CtrlC->c8101, sizeof(CLASS_8101), para_vari_save);
	readCoverClass(0x8102, 0, &CtrlC->c8102, sizeof(CLASS_8102), para_vari_save);
	readCoverClass(0x8103, 0, &CtrlC->c8103, sizeof(CLASS_8103), para_vari_save);

	int i = 0;
	for (i = 0; i < MAX_AL_UNIT; i++) {
		fprintf(stderr, "\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		fprintf(stderr, "\n-------i=%d------------\n", i);
		fprintf(stderr, "OI = %04x\n", CtrlC->c8103.list[i].index);
		fprintf(stderr, "sign = %02x\n", CtrlC->c8103.list[i].sign);
		fprintf(stderr, "V1 = %d %lld %lld %lld %lld %lld %lld %lld %lld \n",
				CtrlC->c8103.list[i].v1.n, CtrlC->c8103.list[i].v1.t1, CtrlC->c8103.list[i].v1.t2,
				CtrlC->c8103.list[i].v1.t3, CtrlC->c8103.list[i].v1.t4, CtrlC->c8103.list[i].v1.t5,
				CtrlC->c8103.list[i].v1.t6, CtrlC->c8103.list[i].v1.t7, CtrlC->c8103.list[i].v1.t8);
		fprintf(stderr, "para = %d\n", CtrlC->c8103.list[i].para);
	}

	readCoverClass(0x8104, 0, &CtrlC->c8104, sizeof(CLASS_8104), para_vari_save);
	readCoverClass(0x8105, 0, &CtrlC->c8105, sizeof(CLASS_8105), para_vari_save);
	readCoverClass(0x8106, 0, &CtrlC->c8106, sizeof(CLASS_8106), para_vari_save);
	readCoverClass(0x8107, 0, &CtrlC->c8107, sizeof(CLASS_8107), para_vari_save);
	readCoverClass(0x8108, 0, &CtrlC->c8108, sizeof(CLASS_8108), para_vari_save);

	fprintf(stderr, "==============================ctrl init end============================\n");


	readCoverClass(0xf205, 0, &CtrlC->cf205, sizeof(CLASS_F205), para_vari_save);
	return 0;
}

int CheckAllUnitEmpty(AL_UNIT au[]) {
	for (int i = 0; i < MAX_AL_UNIT; i++) {
		//总加组不为空
		if (au[i].tsa.addr[0] != 0x00) {
			return 1;
		}
	}
//总加组为空
	return 0;
}

//根据当前时段
INT64S getCurrTimeValue(int line) {
	TS ts;
	TSGet(&ts);

//计算当前标准时段
	int offtime = (ts.Hour * 2 + (ts.Minute / 30));
	int time_index1 = offtime / 4;
	int time_index2 = offtime % 4;

	int curr_type = 0;
	int time_num = 0;
	for (int i = 0; i < offtime; i++) {
		int inner_index1 = i / 4;
		int inner_index2 = i % 4;
		int inner_type = stb_getbit8(CtrlC->c8101.time[inner_index1], inner_index2 * 2)
				+ stb_getbit8(CtrlC->c8101.time[inner_index1], inner_index2 * 2 + 1) * 2;
		if (curr_type != inner_type && inner_type != 0) {
			curr_type = inner_type;
			time_num++;
		}
	}

	int index = CtrlC->c8103.plan[line].numb;
	fprintf(stderr, "时段功控计算时段 index1 %d index2 %d last %d index %d\n", time_index1, time_index2,
			time_num, index);
	switch (index) {
	case 0:
		switch (time_num) {
		case 0:
			return CtrlC->c8103.list[line].v1.t1;
		case 1:
			return CtrlC->c8103.list[line].v1.t2;
		case 2:
			return CtrlC->c8103.list[line].v1.t3;
		case 3:
			return CtrlC->c8103.list[line].v1.t4;
		case 4:
			return CtrlC->c8103.list[line].v1.t5;
		case 5:
			return CtrlC->c8103.list[line].v1.t6;
		case 6:
			return CtrlC->c8103.list[line].v1.t7;
		case 7:
			return CtrlC->c8103.list[line].v1.t8;
		default:
			return -1;
		}
		break;
	case 1:
		switch (time_num) {
		case 0:
			return CtrlC->c8103.list[line].v2.t1;
		case 1:
			return CtrlC->c8103.list[line].v2.t2;
		case 2:
			return CtrlC->c8103.list[line].v2.t3;
		case 3:
			return CtrlC->c8103.list[line].v2.t4;
		case 4:
			return CtrlC->c8103.list[line].v2.t5;
		case 5:
			return CtrlC->c8103.list[line].v2.t6;
		case 6:
			return CtrlC->c8103.list[line].v2.t7;
		case 7:
			return CtrlC->c8103.list[line].v2.t8;
		default:
			return -1;
		}
		break;
	case 2:
		switch (time_num) {
		case 0:
			return CtrlC->c8103.list[line].v3.t1;
		case 1:
			return CtrlC->c8103.list[line].v3.t2;
		case 2:
			return CtrlC->c8103.list[line].v3.t3;
		case 3:
			return CtrlC->c8103.list[line].v3.t4;
		case 4:
			return CtrlC->c8103.list[line].v3.t5;
		case 5:
			return CtrlC->c8103.list[line].v3.t6;
		case 6:
			return CtrlC->c8103.list[line].v3.t7;
		case 7:
			return CtrlC->c8103.list[line].v3.t8;
		default:
			return -1;
		}
		break;
	default:
		return CtrlC->c8103.list[line].v1.t1;
	}
}

int deal8103() {
	static int step[8];
	static int count[8];

	for (int i = 0; i < 8; i++) {
		if (JProgramInfo->ctrls.c8103.enable[i].state == 0 || JProgramInfo->ctrls.c8103.list[i].index == 0x00) {
			step[i] = 0;
			count[i] = 0;
			JProgramInfo->ctrls.c8103.output[i].state = 0;
			JProgramInfo->ctrls.c8103.overflow[i].state = 0;
			JProgramInfo->class23[i].alConState.PCState &= ~128;
			continue;
		} else {
			//更新总加组状态
//        	fprintf(stderr, "deal8103\n\n\n\n\n89898989~~~~~~~~~~~~~~%d\n", JProgramInfo->class23[i].alConState.PCState);
			JProgramInfo->class23[i].alConState.PCState |= 128;
		}

		INT64S val = getCurrTimeValue(i);

		if(val < JProgramInfo->ctrls.c8100.v)
		{
			val = JProgramInfo->ctrls.c8100.v;
		}
		fprintf(stderr, "时段功控限值(%lld Compare %lld) index=%d\n", val, JProgramInfo->class23[i].p, i);

		if (val <= JProgramInfo->class23[i].p) {
			fprintf(stderr, "进入时段功控时间，判断功率%lld\n", JProgramInfo->class23[i].p);
			switch (step[i]) {
			case 0:
				JProgramInfo->ctrls.c8103.output[i].state = 0;
				JProgramInfo->ctrls.c8103.overflow[i].state = 1;
				fprintf(stderr, "时段功控，告警！！！！！！！！！！！！！功控告警时间 %d %d\n", CtrlC->c8102.time[0] * 60,
						count[i] * 5);
				if (count[i] * 5 >= (CtrlC->c8102.time[0]) * 60) {
					JProgramInfo->ctrls.c8103.output[i].state = 128;
					JProgramInfo->ctrls.c8103.overflow[i].state = 0;
					count[i] = 0;
					step[i] = 1;
					fprintf(stderr, "时段功控，一轮跳闸[%d]！！！！！！！！！！！！！",
							JProgramInfo->class23[i].alCtlState.OutputState);
				}
				count[i] += 1;
				break;
			case 1:
				fprintf(stderr, "时段功控，二轮告警[%d]！！！！！！！！！！！！！",
						JProgramInfo->class23[i].alCtlState.OutputState);
				fprintf(stderr, "功控告警时间 %d\n", CtrlC->c8102.time[1] * 60);
				JProgramInfo->ctrls.c8103.output[i].state = 128;
				JProgramInfo->ctrls.c8103.overflow[i].state = 1;
				if (count[i] * 5 >= (CtrlC->c8102.time[1]) * 60) {
					fprintf(stderr, "时段功控，二轮跳闸！！！！！！！！！！！！！");
					JProgramInfo->ctrls.c8103.output[i].state = 192;
					JProgramInfo->ctrls.c8103.overflow[i].state = 1;
					step[i] = 2;
				}
				count[i] += 1;
				break;
			case 2:
				JProgramInfo->ctrls.c8103.output[i].state = 192;
				JProgramInfo->ctrls.c8103.overflow[i].state = 1;
				break;
			default:
				step[i] = 0;
				break;
			}
		} else {
			JProgramInfo->ctrls.c8103.output[i].state = 0;
			JProgramInfo->ctrls.c8103.overflow[i].state = 0;
			step[i] = 0;
			count[i] = 0;
		}
	}
	return 0;
}

//计算当前时间是否在厂休时段范围内
INT64S getIsInTime(int line) {
	TS ts;
	TSGet(&ts);
//	fprintf(stderr, "~~~~~~~~~~~~~刷新参数 %lld %04x %02x %d\n",  JProgramInfo->ctrls.c8104.list[0].v, JProgramInfo->ctrls.c8104.list[0].sustain, JProgramInfo->ctrls.c8104.list[0].noDay, line);
	if (stb_getbit8(0x7F, ts.Week) == 0) {
		fprintf(stderr, "不是限电日！！！！！！！！！！（%d）～～～～～～～～～～～\n", CtrlC->c8104.list[line].noDay);
		return -1;
	}

//判断是否在厂休时间内，并返回定值
	TS start;
	start.Year = ts.Year;
	start.Month = ts.Month;
	start.Day = ts.Day;
	start.Hour = CtrlC->c8104.list[line].start.hour.data;
	start.Minute = CtrlC->c8104.list[line].start.min.data;
	start.Sec = 0x00;

	tminc(&start, minute_units, CtrlC->c8104.list[line].sustain);

//	fprintf(stderr, "SSSSSSSSSSSSSSSSs%04x %04x %04x %04x %04x %04x", start.Year, start.Month, start.Day, start.Hour, start.Minute, start.Sec);

	if (TScompare(start, ts) == 1) {
		fprintf(stderr, "范围内！！！！！！！！！！～～～～～～～～～～～\n");
		return CtrlC->c8104.list[line].v;
	} else {
		fprintf(stderr, "不在厂休范围内！！！！！！！！！！～～～～～～～～～～～\n");
		return -1;
	}
}

int deal8104() {
	static int step[8];
	static int count[8];

	for (int i = 0; i < 1; i++) {
		if (JProgramInfo->ctrls.c8104.enable[i].state == 0 || JProgramInfo->ctrls.c8104.list[i].index == 0x00) {
			step[i] = 0;
			count[i] = 0;
			JProgramInfo->ctrls.c8104.output[i].state = 0;
			JProgramInfo->ctrls.c8104.overflow[i].state = 0;
			JProgramInfo->class23[i].alConState.PCState &= ~64;
			continue;
		} else {
			//更新总加组状态
			JProgramInfo->class23[i].alConState.PCState |= 64;
		}

		INT64S val = getIsInTime(i);
		if(val < JProgramInfo->ctrls.c8100.v)
		{
			val = JProgramInfo->ctrls.c8100.v;
		}
		fprintf(stderr, "厂休控限值(%lld Compare %lld) index=%d\n", val, JProgramInfo->class23[i].p, i);
		if (val <= JProgramInfo->class23[i].p) {
			//		if(1){
			fprintf(stderr, "进入厂休控时间，判断功率%lld\n", JProgramInfo->class23[i].p);
			switch (step[i]) {
			case 0:
				JProgramInfo->ctrls.c8104.output[i].state = 0;
				JProgramInfo->ctrls.c8104.overflow[i].state = 1;
				fprintf(stderr, "厂休控，告警！！！！！！！！！！！！！功控告警时间 %d %d\n", CtrlC->c8102.time[0] * 60,
						count[i] * 5);
				if (count[i] * 5 >= (CtrlC->c8102.time[0]) * 60) {
					JProgramInfo->ctrls.c8104.output[i].state = 128;
					JProgramInfo->ctrls.c8104.overflow[i].state = 0;
					count[i] = 0;
					step[i] = 1;
					fprintf(stderr, "厂休控，一轮跳闸[%d]！！！！！！！！！！！！！",
							JProgramInfo->class23[i].alCtlState.OutputState);
				}
				count[i] += 1;
				break;
			case 1:
				fprintf(stderr, "厂休控，二轮告警[%d]！！！！！！！！！！！！！",
						JProgramInfo->class23[i].alCtlState.OutputState);
				fprintf(stderr, "功控告警时间 %d\n", CtrlC->c8102.time[1] * 60);
				JProgramInfo->ctrls.c8104.output[i].state = 128;
				JProgramInfo->ctrls.c8104.overflow[i].state = 1;
				if (count[i] * 5 >= (CtrlC->c8102.time[1]) * 60) {
					fprintf(stderr, "厂休控，二轮跳闸！！！！！！！！！！！！！");
					JProgramInfo->ctrls.c8104.output[i].state = 192;
					JProgramInfo->ctrls.c8104.overflow[i].state = 1;
					step[i] = 2;
				}
				count[i] += 1;
				break;
			case 2:
				JProgramInfo->ctrls.c8104.output[i].state = 192;
				JProgramInfo->ctrls.c8104.overflow[i].state = 1;
				break;
			default:
				step[i] = 0;
				break;
			}
		} else {
			step[i] = 0;
			count[i] = 0;
			JProgramInfo->ctrls.c8104.output[i].state = 0;
			JProgramInfo->ctrls.c8104.overflow[i].state = 0;
		}
	}
	return 0;
}

void TransBCDToTS(DateTimeBCD bcd, TS *ts)
{
    ts->Year = bcd.year.data;
    ts->Month = bcd.month.data;
    ts->Day = bcd.day.data;
    ts->Hour = bcd.hour.data;
    ts->Minute = bcd.min.data;
    ts->Sec = bcd.sec.data;
}

//计算当前时间是否在厂休时段范围内
INT64S getIsStop(int line) {
	TS ts;
	TSGet(&ts);

//	fprintf(stderr, "@@@@ %d %d %d %d %d %d\n", CtrlC->c8105.list[line].start.year, CtrlC->c8105.list[line].start.month, CtrlC->c8105.list[line].start.day, CtrlC->c8105.list[line].start.hour, CtrlC->c8105.list[line].start.min, 0);
//	fprintf(stderr, "@@@@ %d %d %d %d %d %d\n", CtrlC->c8105.list[line].end.year, CtrlC->c8105.list[line].end.month, CtrlC->c8105.list[line].end.day, CtrlC->c8105.list[line].end.hour, CtrlC->c8105.list[line].end.min, 0);

    TS start;
    TS end;

	TransBCDToTS(CtrlC->c8105.list[line].start, &start);
	TransBCDToTS(CtrlC->c8105.list[line].end, &end);

	fprintf(stderr, "@@@@ %d %d\n", TScompare(ts, start),
			TScompare(ts, end));
	if (TScompare(ts, start) == 1
			&& TScompare(ts, end) == 2) {
		fprintf(stderr, "范围内！！！！！！！！！！～～～～～～～～～～～\n");
		return CtrlC->c8105.list[line].v;
	}
	fprintf(stderr, "不在营业报停段范围内！！！！！！！！！！～～～～～～～～～～～\n");
	return -1;
}

int deal8105() {
	static int step[8];
	static int count[8];

	for (int i = 0; i < 8; i++) {
		if (JProgramInfo->ctrls.c8105.enable[i].state == 0 || JProgramInfo->ctrls.c8105.list[i].index == 0x00) {
			step[i] = 0;
			count[i] = 0;
			JProgramInfo->ctrls.c8105.output[i].state = 0;
			JProgramInfo->ctrls.c8105.overflow[i].state = 0;
			JProgramInfo->class23[i].alConState.PCState &= ~32;
			continue;
		} else {
			//更新总加组状态
			JProgramInfo->class23[i].alConState.PCState |= 32;
		}

		INT64S val = getIsStop(i);
		if(val < JProgramInfo->ctrls.c8100.v)
		{
			val = JProgramInfo->ctrls.c8100.v;
		}
		fprintf(stderr, "营业报停控限值(%lld Compare %lld) index=%d\n", val, JProgramInfo->class23[i].p,
				i);
		if (val <= JProgramInfo->class23[i].p) {
			fprintf(stderr, "进入营业报停控时间，判断功率%lld\n", JProgramInfo->class23[i].p);
			switch (step[i]) {
				case 0:
					JProgramInfo->ctrls.c8105.output[i].state = 0;
					JProgramInfo->ctrls.c8105.overflow[i].state = 1;
					fprintf(stderr, "营业报停控，告警！！！！！！！！！！！！！功控告警时间 %d %d\n", CtrlC->c8102.time[0] * 60,
							count[i] * 5);
					if (count[i] * 5 >= (CtrlC->c8102.time[0]) * 60) {
						JProgramInfo->ctrls.c8105.output[i].state = 128;
						JProgramInfo->ctrls.c8105.overflow[i].state = 0;
						count[i] = 0;
						step[i] = 1;
						fprintf(stderr, "营业报停控，一轮跳闸[%d]！！！！！！！！！！！！！",
								JProgramInfo->class23[i].alCtlState.OutputState);
					}
					count[i] += 1;
					break;
				case 1:
					fprintf(stderr, "营业报停控，二轮告警[%d]！！！！！！！！！！！！！",
							JProgramInfo->class23[i].alCtlState.OutputState);
					fprintf(stderr, "功控告警时间 %d\n", CtrlC->c8102.time[1] * 60);
					JProgramInfo->ctrls.c8105.output[i].state = 128;
					JProgramInfo->ctrls.c8105.overflow[i].state = 1;
					if (count[i] * 5 >= (CtrlC->c8102.time[1]) * 60) {
						fprintf(stderr, "营业报停控，二轮跳闸！！！！！！！！！！！！！");
						JProgramInfo->ctrls.c8105.output[i].state = 192;
						JProgramInfo->ctrls.c8105.overflow[i].state = 1;
						step[i] = 2;
					}
					count[i] += 1;
					break;
				case 2:
					JProgramInfo->ctrls.c8105.output[i].state = 192;
					JProgramInfo->ctrls.c8105.overflow[i].state = 1;
					break;
				default:
					step[i] = 0;
			}
		} else {
			JProgramInfo->ctrls.c8105.output[i].state = 0;
			JProgramInfo->ctrls.c8105.overflow[i].state = 0;
			step[i] = 0;
			count[i] = 0;
		}
	}
	return 0;
}

//计算当前时间是否在功率下浮控范围内
INT64S getIsInDown(TS start, int line) {
	TS ts;
	TSGet(&ts);
	fprintf(stderr, "@@@@ %d %d %d %d %d %d\n", start.Year, start.Month, start.Day, start.Hour,
			start.Minute, 0);
	tminc(&start, 1, CtrlC->c8106.list.down_ctrl_time * 30);
//		fprintf(stderr, "@@@@ %d %d %d %d %d %d\n",start.Year, start.Month, start.Day, start.Hour, start.Minute, 0);
//		fprintf(stderr, "@@@@ %d %d %d %d %d %d\n", ts.Year, ts.Month, ts.Day, ts.Hour, ts.Minute, 0);

//	fprintf(stderr, "控制时间内！！！！！！！！！！[%d][%d]～～～～～～～～～～～\n",TScompare(ts, start), CtrlC->c8106.list.down_ctrl_time);
	if (TScompare(ts, start) == 2) {
		fprintf(stderr, "控制时间内！！！！！！！！！！[%g]～～～～～～～～～～～\n",
				JProgramInfo->class23[line].p * ((CtrlC->c8106.list.down_xishu + 100) / 100.0));
		return (INT64S)JProgramInfo->class23[line].p * ((CtrlC->c8106.list.down_xishu + 100) / 100);
	} else {
		fprintf(stderr, "不在功率下浮控控制时间内！！！！！！！！！！～～～～～～～～～～～\n");
		return -1;
	}
}

int deal8106() {
    TS ts;
    static TS start;
    TSGet(&ts);
    static int step = 0;
    static int count = 0;
    static int freeze_count = 0;
    static INT64S val;

    if (JProgramInfo->ctrls.c8106.enable.state == 0) {
        step = 0;
        count = 0;
        freeze_count = 0;
        TSGet(&start);
        JProgramInfo->ctrls.c8106.output.state = 0;
        JProgramInfo->ctrls.c8106.overflow.state = 0;
        int i = JProgramInfo->ctrls.c8106.index - 0x2301;
        if (i > 7 || i < 0) {
            i = 0;
        }
        JProgramInfo->class23[i].alConState.PCState &= ~16;
        val = 0;
        return 0;
    }

//冻结时间
    fprintf(stderr, "功率下浮控冻结时间(%d)\n", JProgramInfo->ctrls.c8106.list.down_freeze);
    if (freeze_count * 5 < JProgramInfo->ctrls.c8106.list.down_freeze * 60) {
        freeze_count++;
        return 0;
    }

    TSGet(&start);

    int i = JProgramInfo->ctrls.c8106.index - 0x2301;
    if (i > 7 || i < 0) {
        i = 0;
    }
//更新总加组状态
    JProgramInfo->class23[i].alConState.PCState |= 16;

    if (val <= 0) {
        val = getIsInDown(start, i);
        fprintf(stderr, "更新功率下浮(%lld)\n", val);
    }

    fprintf(stderr, "功率下浮控限值(%lld Compare %lld)\n", val, JProgramInfo->class23[i].p);
    if (val < JProgramInfo->ctrls.c8100.v) {
        val = JProgramInfo->ctrls.c8100.v;
    }
    if (val <= JProgramInfo->class23[i].p) {
        fprintf(stderr, "进入功率下浮控时间，判断功率%lld\n", JProgramInfo->class23[i].p);
        switch (step) {
            case 0:
                JProgramInfo->ctrls.c8106.output.state = 0;
                JProgramInfo->ctrls.c8106.overflow.state = 1;
                fprintf(stderr, "功率下浮控，告警！！！！！！！！！！！！！功控告警时间 %d %d\n", CtrlC->c8102.time[0] * 60,
                        count * 5);
                if (count * 5 >= (CtrlC->c8102.time[0]) * 60) {
                    JProgramInfo->ctrls.c8106.output.state = 128;
                    JProgramInfo->ctrls.c8106.overflow.state = 0;
                    count = 0;
                    step = 1;
                    fprintf(stderr, "功率下浮控，一轮跳闸[%d]！！！！！！！！！！！！！",
                            JProgramInfo->class23[i].alCtlState.OutputState);
                }
                count += 1;
                break;
            case 1:
                fprintf(stderr, "功率下浮控，二轮告警[%d]！！！！！！！！！！！！！",
                        JProgramInfo->class23[i].alCtlState.OutputState);
                fprintf(stderr, "功控告警时间 %d\n", CtrlC->c8102.time[1] * 60);
                JProgramInfo->ctrls.c8106.output.state = 128;
                JProgramInfo->ctrls.c8106.overflow.state = 1;
                if (count * 5 >= (CtrlC->c8102.time[1]) * 60) {
                    fprintf(stderr, "功率下浮控，二轮跳闸！！！！！！！！！！！！！");
                    JProgramInfo->ctrls.c8106.output.state = 192;
                    JProgramInfo->ctrls.c8106.overflow.state = 1;
                    step = 2;
                }
                count += 1;
                break;
            case 2:
                JProgramInfo->ctrls.c8106.output.state = 192;
                JProgramInfo->ctrls.c8106.overflow.state = 1;
                break;
            default:
                step = 0;
        }
    } else {
        JProgramInfo->ctrls.c8106.output.state = 0;
        JProgramInfo->ctrls.c8106.overflow.state = 0;
        step = 0;
        count = 0;
    }
    return 0;
}

int deal8107() {
//购电控不受购电配置单元的影响，购电配置单元作为总加组剩余电量刷新的依据

	fprintf(stderr, "deal8107(%lld)\n", CtrlC->c8107.list[0].ctrl);
	for (int i = 0; i < 1; i++) {
		if (JProgramInfo->ctrls.c8107.enable[i].state == 0 || JProgramInfo->ctrls.c8107.list[i].index == 0x00) {
			JProgramInfo->ctrls.c8107.output[i].state = 0;
			JProgramInfo->ctrls.c8107.overflow[i].state = 0;
			JProgramInfo->class23[i].alConState.ECState &= ~128;
			return 0;
		}

		//更新总加组状态
		fprintf(stderr, "deal8107\n\n\n\n\n8989898999999~~~~~~~~~~~~~~%d\n",
				JProgramInfo->class23[i].alConState.ECState);

		JProgramInfo->class23[i].alConState.ECState |= 128;

		if (!CheckAllUnitEmpty(JProgramInfo->class23[i].allist)) {
			continue;
		}
		fprintf(stderr, "8107 index = %d\n", i);

		INT64S val = CtrlC->c8107.list[0].ctrl;
		INT64S warn = CtrlC->c8107.list[0].alarm;
		fprintf(stderr, "购电控限制[%lld] [%lld]\n", val, warn);

		if (val >= 0) {

			INT64S mmm = JProgramInfo->class23[i].remains;
			if (mmm < 0) {
				mmm = 0;
			}
			fprintf(stderr, "购电判断值[%lld](%lld)\n", JProgramInfo->class23[i].remains, mmm);

			if (mmm <= val) {
				fprintf(stderr, "购电控跳闸 ！！！！！！！！！！！！！！！！！！\n");
				JProgramInfo->ctrls.c8107.output[i].state = 192;
				JProgramInfo->ctrls.c8107.overflow[i].state = 0;
				return 2;
			} else if (mmm <= warn) {
				fprintf(stderr, "购电控告警！！！！！！！！！！！！！！！！！！\n");
				JProgramInfo->ctrls.c8107.output[i].state = 0;
				JProgramInfo->ctrls.c8107.overflow[i].state = 1;
				return 1;
			}
			else{
				JProgramInfo->ctrls.c8107.output[i].state = 0;
				JProgramInfo->ctrls.c8107.overflow[i].state = 0;
			}
		}
	}
	return 0;
}

int deal8108() {
	fprintf(stderr, "deal8108(%lld)\n", CtrlC->c8108.list[0].v);
	for (int i = 0; i < 1; i++) {
		if (JProgramInfo->ctrls.c8108.enable[i].state == 0 || JProgramInfo->ctrls.c8103.list[i].index == 0x00) {
			JProgramInfo->ctrls.c8108.output[i].state = 0;
			JProgramInfo->ctrls.c8108.overflow[i].state = 0;
			JProgramInfo->class23[i].alConState.ECState &= ~64;
			return 0;
		}
		//更新总加组状态
		JProgramInfo->class23[i].alConState.ECState |= 64;

		if (!CheckAllUnitEmpty(JProgramInfo->class23[i].allist)) {
			continue;
		}
		fprintf(stderr, "8108 index = %d\n", i);

		INT64S val = CtrlC->c8108.list[i].v;
		INT8U warn = CtrlC->c8108.list[i].para;
		fprintf(stderr, "月电控限制%lld\n", val);

		long long total = 0;
		for (int i = 0; i < MAXVAL_RATENUM; i++) {
			total += JProgramInfo->class23[i].MonthP[i];
		}

		if (val != -1) {
            double e = warn / 100.0;

			fprintf(stderr, "月电控值%lld [%f][e=%f warn=%d]\n", JProgramInfo->class23[i].MonthPALL, e * val,e,warn);

			if (JProgramInfo->class23[i].MonthPALL > val) {
				fprintf(stderr, "月电控跳闸！！！！！！！！！！！！！！！！！！\n");
				JProgramInfo->ctrls.c8108.output[i].state = 192;
				JProgramInfo->ctrls.c8108.overflow[i].state = 0;
				return 2;
			}

			if (JProgramInfo->class23[i].MonthPALL > e * val) {
				fprintf(stderr, "月电控告警！！！！！！！！！！！！！！！！！！\n");
				JProgramInfo->ctrls.c8108.output[i].state = 0;
				JProgramInfo->ctrls.c8108.overflow[i].state = 1;
				return 1;
			}
		}
	}
	return 0;
}

void dealCtrl() {

	//如果保电不跳闸
	if(CtrlC->c8001.state == 1)
	{
		//保电
		return;
	}else if(CtrlC->c8001.state == 2)
	{
		//zidong
	}

//直接跳闸，必须检测
	deal8107();
	deal8108();

//检测控制有优先级，当高优先级条件产生时，忽略低优先级的配置

	deal8106();
	deal8105();
	deal8104();
	deal8103();
}

/*
 * 初始化参数
 * */
void CheckInitPara() {
	if (JProgramInfo->oi_changed.ctrlinit == 0x55) {
		JProgramInfo->oi_changed.ctrlinit = 0x00;
		system("rm -rf /nand/para/230*");
		system("rm -rf /nand/para/240*");
		system("rm -rf /nand/para/80*");
		sleep(3);
		syslog(LOG_NOTICE,"恢复出厂设置 重新载入参数！！！！！！！！");
		initAll();
	}
}

int SaveAll(void* arg) {
	TS now;
	int secOld = 0;
	int sign = 0;
	int old_sign[10];

	for(int i = 0; i < 10; i++)
	{
		old_sign[i] = 0;
	}

	int run = 1;
	while (run) {
		TSGet(&now);
		//一秒钟刷新一次
		if (secOld == now.Sec) {
			usleep(500 * 1000);
			continue;
		} else {
			secOld = now.Sec;
		}
		CheckInitPara();

		if (secOld % 47 == 0) {
			for (int i = 0; i < 8; ++i) {
				sign = stb_crc32((unsigned char *)&JProgramInfo->class23[i], sizeof(CLASS23));
				if(sign != old_sign[i]){
					old_sign[i] = sign;
					saveCoverClass(0x2301 + i, 0, &JProgramInfo->class23[i], sizeof(CLASS23),
						para_vari_save);
				}
			}
			sign = stb_crc32((unsigned char *)&JProgramInfo->class12[0], sizeof(CLASS12));
			if(sign != old_sign[8]){
				old_sign[8] = sign;
				saveCoverClass(0x2401, 0, &JProgramInfo->class12[0], sizeof(CLASS12),
						para_vari_save);
			}
			sign = stb_crc32((unsigned char *)&JProgramInfo->class12[1], sizeof(CLASS12));
			if(sign != old_sign[9]){
				old_sign[9] = sign;
				saveCoverClass(0x2402, 0, &JProgramInfo->class12[1], sizeof(CLASS12),
						para_vari_save);
			}
		}
	}
    return (void*)0;
}

void CheckCtrlControl() {
	static int count = 0;
	if (CtrlC->control_event == 1) {
		fprintf(stderr, "遥控跳闸事件");
		if (count++ > 120) {
			INT16U oi = 0x052f;
			Event_3115(&oi, 2, JProgramInfo);
			count = 0;
			CtrlC->control_event = 0;
		}
	}
}

void PackCtrlSituation() {
	INT8U out = 0;
	INT8U green_led = 0;

	INT8U pc = 0;
	INT8U ec = 0;
	INT8U al = 0;

	for (int i = 0; i < 8; ++i) {
		out |= JProgramInfo->class23[i].alCtlState.OutputState;
		out |= JProgramInfo->class23[i].alCtlState.BuyOutputState;
		out |= JProgramInfo->class23[i].alCtlState.MonthOutputState;

		al |= JProgramInfo->class23[i].alCtlState.PCAlarmState;
		al |= JProgramInfo->class23[i].alCtlState.ECAlarmState;
	}

	ctrlunit.ctrl.lun1_state = out >> 7;
	ctrlunit.ctrl.lun2_state = (out & ~(INT8U)128) >> 6;

	for (int i = 0; i < 8; i++) {
		green_led |= JProgramInfo->class23[i].pConfig;
		green_led |= JProgramInfo->class23[i].eConfig;
	}
	fprintf(stderr, "green_led %d\n", green_led);
	if (green_led == 192 || green_led == 128) {
		ctrlunit.ctrl.lun1_green = 1;
	} else {
		ctrlunit.ctrl.lun1_green = 0;
	}

	if (green_led == 192 || green_led == 64) {
		ctrlunit.ctrl.lun2_green = 1;
	} else {
		ctrlunit.ctrl.lun2_green = 0;
	}

	for (int j = 0; j < 8; j++) {
		pc |= JProgramInfo->ctrls.c8103.enable[j].state;
		pc |= JProgramInfo->ctrls.c8104.enable[j].state;
		pc |= JProgramInfo->ctrls.c8105.enable[j].state;
		pc |= JProgramInfo->ctrls.c8106.enable.state;
	}

	for (int j = 0; j < 8; j++) {
		ec |= JProgramInfo->ctrls.c8107.enable[j].state;
		ec |= JProgramInfo->ctrls.c8108.enable[j].state;
	}

	ctrlunit.ctrl.gongk_led = (INT8U)((pc == 0) ? 0 : 1);
	ctrlunit.ctrl.diank_led = (INT8U)((ec == 0) ? 0 : 1);

	ctrlunit.ctrl.lun1_red = (INT8U)((out == 128 || out == 192) ? 1 : 0);
	ctrlunit.ctrl.lun2_red = (INT8U)((out == 192) ? 1 : 0);

	if (ctrlunit.ctrl.lun1_red == 1) {
		ctrlunit.ctrl.lun1_green = 0;
	}

	if (ctrlunit.ctrl.lun2_red == 1) {
		ctrlunit.ctrl.lun2_green = 0;
	}

	if (JProgramInfo->ctrls.c8001.state == 1) {
		ctrlunit.ctrl.baodian_led = 1;
	} else {
		ctrlunit.ctrl.baodian_led = 0;
	}

	ctrlunit.ctrl.alm_state = (INT8U)((al == 0) ? 0 : 1);

	//如果保电不跳闸
	if(CtrlC->c8001.state == 1)
	{
		//保电
		return;
	}else if(CtrlC->c8001.state == 2)
	{
		//zidong
	}

	//汇总遥控命令
	static int old_state1 = 0;
	static int old_state2 = 0;

	fprintf(stderr, "CtrlC->c8000.openclose[0] == %04x\n", CtrlC->c8000.openclose[0]);
	if (CtrlC->c8000.openclose[0] == 0x5555)
	{
		fprintf(stderr, "CtrlC->c8000.openclose[0] == 0x5555\n");
		ctrlunit.ctrl.lun1_state = 1;
	}
	if (CtrlC->c8000.openclose[0] == 0xCCCC)
	{
		fprintf(stderr, "CtrlC->c8000.openclose[0] == 0xCCCC\n");
		ctrlunit.ctrl.lun1_state = 0;
	}

	if (CtrlC->c8000.openclose[1] == 0x5555)
	{
		ctrlunit.ctrl.lun2_state = 1;
	}
	if (CtrlC->c8000.openclose[1] == 0xCCCC)
	{
		ctrlunit.ctrl.lun2_state = 0;
	}

	if (CtrlC->c8000.openclose[0] != old_state1 || CtrlC->c8000.openclose[1] != old_state2)
	{
		old_state1 = CtrlC->c8000.openclose[0];
		old_state2 = CtrlC->c8000.openclose[1];
		CtrlC->control_event = 1;
	}

	static int cf205_s1 = 0;
	static int cf205_s2 = 0;

	//置F205状态
	CtrlC->cf205.unit[0].currentState = ctrlunit.ctrl.lun1_state;
	CtrlC->cf205.unit[1].currentState = ctrlunit.ctrl.lun2_state;

	if(CtrlC->cf205.unit[0].currentState != cf205_s1 || CtrlC->cf205.unit[1].currentState != cf205_s2)
	{
		cf205_s1 = CtrlC->cf205.unit[0].currentState;
		cf205_s2 = CtrlC->cf205.unit[1].currentState;
		saveCoverClass(0xf205, 0, &CtrlC->cf205, sizeof(CLASS_F205),
						para_vari_save);
	}

	fprintf(stderr, "ctrlunit.ctrl.lun1_state = %d %d\n", ctrlunit.ctrl.lun1_state, CtrlC->cf205.unit[0].currentState);
	fprintf(stderr, "ctrlunit.ctrl.lun2_state = %d %d\n", ctrlunit.ctrl.lun2_state, CtrlC->cf205.unit[1].currentState);


	//置8000继电器告警状态
	if (CtrlC->cf205.unit[0].currentState == 1){
		CtrlC->c8000.cmdstate = stb_setbit8(CtrlC->c8000.cmdstate, 7);
	}
	else{
		CtrlC->c8000.cmdstate = 0;
	}
	if (CtrlC->cf205.unit[1].currentState == 1){
		CtrlC->c8000.cmdstate = stb_setbit8(CtrlC->c8000.cmdstate, 6);
	}
	else {
		CtrlC->c8000.cmdstate = 0;
	}

	if (ctrlunit.ctrl.alm_state == 0)
	{
		ctrlunit.ctrl.alm_state = 1;
	}else{
		ctrlunit.ctrl.alm_state = 0;
	}


	fprintf(stderr, "遥控模块最后汇总[%d %d %d] %d %d %d %d\n", ctrlunit.ctrl.alm_state,
			ctrlunit.ctrl.gongk_led, ctrlunit.ctrl.diank_led, ctrlunit.ctrl.lun1_red,
			ctrlunit.ctrl.lun2_red, ctrlunit.ctrl.lun1_green, ctrlunit.ctrl.lun1_green);
	fprintf(stderr, " %d %d\n\n~~~~~~~~~~~~~~\n", (ctrlunit.u16b & 0xffff),
			(ctrlunit_old.u16b & 0xffff));
}

void HandlerCtrl() {
	if (JProgramInfo->ctrls.control[0] == 0xEEFFEFEF && JProgramInfo->ctrls.control[1] == 0xEEFFEFEF
			&& JProgramInfo->ctrls.control[2] == 0xEEFFEFEF) { //分闸
		ctrlunit.ctrl.lun1_state = 0;
		ctrlunit.ctrl.lun1_red = 1;
		ctrlunit.ctrl.lun1_green = 0;
	} else if (JProgramInfo->ctrls.control[0] == 0xCCAACACA
			&& JProgramInfo->ctrls.control[1] == 0xCCAACACA
			&& JProgramInfo->ctrls.control[2] == 0xCCAACACA) { //合闸
		ctrlunit.ctrl.lun1_state = 1;
		ctrlunit.ctrl.lun1_red = 0;
		ctrlunit.ctrl.lun1_green = 1;
	} else if (JProgramInfo->ctrls.control[0] == 0x55552525
			&& JProgramInfo->ctrls.control[1] == 0x55552525
			&& JProgramInfo->ctrls.control[2] == 0x55552525) { //分闸
		ctrlunit.ctrl.lun2_state = 0;
		ctrlunit.ctrl.lun2_red = 1;
		ctrlunit.ctrl.lun2_green = 0;
	} else if (JProgramInfo->ctrls.control[0] == 0xCCCC2C2C
			&& JProgramInfo->ctrls.control[1] == 0xCCCC2C2C
			&& JProgramInfo->ctrls.control[2] == 0xCCCC2C2C) { //合闸
		ctrlunit.ctrl.lun2_state = 1;
		ctrlunit.ctrl.lun2_red = 0;
		ctrlunit.ctrl.lun2_green = 1;
	}
	memset(&JProgramInfo->ctrls.control, 0, sizeof(JProgramInfo->ctrls.control));
}

void DoActuallyCtrl() {
	fprintf(stderr, " %d %d\n\n~~~~~~~~~~~~~~\n", (ctrlunit.u16b & 0x3ff),
			(ctrlunit_old.u16b & 0x3ff));
	if ((ctrlunit.u16b & 0x3ff) ^ (ctrlunit_old.u16b & 0x3ff)) {
		ctrlunit_old.u16b = ctrlunit.u16b;
		asyslog(LOG_NOTICE, "接收到控制命令：控制状态【%04x】 原状态【%04x】", ctrlunit.u16b, ctrlunit_old.u16b);
		InitCtrlModel();
		int fd = OpenSerialPort();
		SetCtrl_CMD(fd, ctrlunit.ctrl.lun1_state, ctrlunit.ctrl.lun1_red, ctrlunit.ctrl.lun1_green,
				ctrlunit.ctrl.lun2_state, ctrlunit.ctrl.lun2_red, ctrlunit.ctrl.lun2_green,
				ctrlunit.ctrl.gongk_led, ctrlunit.ctrl.diank_led, ctrlunit.ctrl.alm_state,
				ctrlunit.ctrl.baodian_led);
		close(fd);
	}
}
int gpofun(char *devname, int data) {
	int fd;
	if ((fd = open(devname, O_RDWR | O_NDELAY)) >= 0) {
		write(fd, &data, sizeof(int));
		close(fd);
		return 1;
	}
	return 0;
}

void CtrlStateSumUp() {
	for (int i = 0; i < 8; i++) {
		JProgramInfo->class23[i].alCtlState.OutputState = 0;
		JProgramInfo->class23[i].alCtlState.BuyOutputState = 0;
		JProgramInfo->class23[i].alCtlState.MonthOutputState = 0;
		JProgramInfo->class23[i].alCtlState.PCAlarmState = 0;
		JProgramInfo->class23[i].alCtlState.ECAlarmState = 0;

		if (JProgramInfo->ctrls.c8103.overflow[i].state == 1) {
			JProgramInfo->class23[i].alCtlState.PCAlarmState = stb_setbit8(
					JProgramInfo->class23[i].alCtlState.PCAlarmState, 7);
		}
		if (JProgramInfo->ctrls.c8104.overflow[i].state == 1) {
			JProgramInfo->class23[i].alCtlState.PCAlarmState = stb_setbit8(
					JProgramInfo->class23[i].alCtlState.PCAlarmState, 6);
		}
		if (JProgramInfo->ctrls.c8105.overflow[i].state == 1) {
			JProgramInfo->class23[i].alCtlState.PCAlarmState = stb_setbit8(
					JProgramInfo->class23[i].alCtlState.PCAlarmState, 5);
		}
		if (JProgramInfo->ctrls.c8106.overflow.state == 1) {
			JProgramInfo->class23[i].alCtlState.PCAlarmState = stb_setbit8(
					JProgramInfo->class23[i].alCtlState.PCAlarmState, 4);
		}

		if (JProgramInfo->ctrls.c8107.overflow[i].state == 1) {
			JProgramInfo->class23[i].alCtlState.ECAlarmState = stb_setbit8(
					JProgramInfo->class23[i].alCtlState.ECAlarmState, 6);
		}
		if (JProgramInfo->ctrls.c8108.overflow[i].state == 1) {
			JProgramInfo->class23[i].alCtlState.ECAlarmState = stb_setbit8(
					JProgramInfo->class23[i].alCtlState.ECAlarmState, 7);
		}


		if (JProgramInfo->ctrls.c8103.enable[i].state == 1) {
			JProgramInfo->class23[i].alConState.PCState = stb_setbit8(
					JProgramInfo->class23[i].alConState.PCState, 7);
		}
		if (JProgramInfo->ctrls.c8104.enable[i].state == 1) {
			JProgramInfo->class23[i].alConState.PCState = stb_setbit8(
					JProgramInfo->class23[i].alConState.PCState, 6);
		}
		if (JProgramInfo->ctrls.c8105.enable[i].state == 1) {
			JProgramInfo->class23[i].alConState.PCState = stb_setbit8(
					JProgramInfo->class23[i].alConState.PCState, 5);
		}
		if (JProgramInfo->ctrls.c8106.enable.state == 1) {
			JProgramInfo->class23[i].alConState.PCState = stb_setbit8(
					JProgramInfo->class23[i].alConState.PCState, 4);
		}

		if (JProgramInfo->ctrls.c8107.enable[i].state == 1) {
			JProgramInfo->class23[i].alConState.ECState = stb_setbit8(
					JProgramInfo->class23[i].alConState.ECState, 6);
		}
		if (JProgramInfo->ctrls.c8108.enable[i].state == 1) {
			JProgramInfo->class23[i].alConState.ECState = stb_setbit8(
					JProgramInfo->class23[i].alConState.ECState, 7);
		}

		JProgramInfo->class23[i].alCtlState.BuyOutputState |=
				JProgramInfo->ctrls.c8107.output[i].state;
		JProgramInfo->class23[i].alCtlState.MonthOutputState |=
				JProgramInfo->ctrls.c8108.output[i].state;

		JProgramInfo->class23[i].alCtlState.OutputState |=
				JProgramInfo->ctrls.c8103.output[i].state;
		JProgramInfo->class23[i].alCtlState.OutputState |=
				JProgramInfo->ctrls.c8104.output[i].state;
		JProgramInfo->class23[i].alCtlState.OutputState |=
				JProgramInfo->ctrls.c8105.output[i].state;
		JProgramInfo->class23[i].alCtlState.OutputState |= JProgramInfo->ctrls.c8106.output.state;
	}

	fprintf(stderr, "=======================%d=%d==\n", JProgramInfo->ctrls.c8103.overflow[0].state,
			JProgramInfo->class23[0].alCtlState.PCAlarmState);

	INT8U F206_tmp = 0;

//汇总F206
	for (int sum_i = 0; sum_i < 8; sum_i++) {
		F206_tmp |= JProgramInfo->class23[sum_i].alCtlState.ECAlarmState;
		F206_tmp |= JProgramInfo->class23[sum_i].alCtlState.PCAlarmState;
	}

	if (F206_state != F206_tmp) {
		fprintf(stderr, "告警状态F206变更!!! %d\n", F206_state);
		F206_state = F206_tmp;
		CLASS_f206 f206 = { };
		memset(&f206, 0, sizeof(CLASS_f206));
		readCoverClass(0xf206, 0, &f206, sizeof(CLASS_f206), para_vari_save);
		if (F206_state != 0) {
			f206.alarm_state[0] = 1;
			f206.state_num = 1;
		} else {
			f206.alarm_state[0] = 0;
			f206.state_num = 1;
		}

		saveCoverClass(0xf206, 0, &f206, sizeof(CLASS_f206), para_vari_save);
		fprintf(stderr, "告警状态F206变更!!! %d\n", F206_state);
	}
}

void ShaningLED_F206() {
	static int step = 0;
	static int count = 0;
	if (F206_state != 0) {
		if (step == 0) {
			gpofun("/dev/gpoALARM", 1);
			step = 1;
		} else {
			gpofun("/dev/gpoALARM", 0);
			step = 0;
		}
		if (count < 60) {
			gpofun("/dev/gpoBUZZER", 1);
			count ++;
		}else{
			gpofun("/dev/gpoBUZZER", 0);
		}
	} else {
		gpofun("/dev/gpoALARM", 0);
		gpofun("/dev/gpoBUZZER", 0);
		count  = 0;
	}
}

int ctrlMain(void* arg) {

	int secOld = 0;
	PluseUnit pu;
	SumUpUnit suu;

//初始化参数,搭建8个总加组数据，读取功控、电控参数
	initAll();
	pluseInitUnit(&pu, JProgramInfo);
	sumUpInitUnit(&suu, JProgramInfo);

	if (0) {
		for (int i = 0; i < 8; i++) {
			JProgramInfo->class23[i].allist[0].al_flag = 0;
			JProgramInfo->class23[i].allist[0].cal_flag = 0;
			JProgramInfo->class23[i].allist[0].curP[0] = 100;
			JProgramInfo->class23[i].allist[0].curP[1] = 20;
			JProgramInfo->class23[i].allist[0].curP[2] = 30;
			JProgramInfo->class23[i].allist[0].curP[3] = 20;
			JProgramInfo->class23[i].allist[0].curP[4] = 30;
		}

		SumUpRefreshUnit(&suu);
		for (int i = 0; i < 8; i++) {
			assert(JProgramInfo->class23[i].DayPALL == 10000);
			assert(JProgramInfo->class23[i].DayP[0] == 2000);
			assert(JProgramInfo->class23[i].DayP[1] == 3000);
			assert(JProgramInfo->class23[i].DayP[2] == 2000);
			assert(JProgramInfo->class23[i].DayP[3] == 3000);
		}

		for (int i = 0; i < 8; i++) {
			JProgramInfo->class23[i].allist[0].al_flag = 0;
			JProgramInfo->class23[i].allist[0].cal_flag = 1;
			JProgramInfo->class23[i].allist[0].curP[0] = 200;
			JProgramInfo->class23[i].allist[0].curP[1] = 40;
			JProgramInfo->class23[i].allist[0].curP[2] = 60;
			JProgramInfo->class23[i].allist[0].curP[3] = 40;
			JProgramInfo->class23[i].allist[0].curP[4] = 60;
		}

		SumUpRefreshUnit(&suu);
		for (int i = 0; i < 8; i++) {
			assert(JProgramInfo->class23[i].DayPALL == 0);
			assert(JProgramInfo->class23[i].DayP[0] == 0);
			assert(JProgramInfo->class23[i].DayP[1] == 0);
			assert(JProgramInfo->class23[i].DayP[2] == 0);
			assert(JProgramInfo->class23[i].DayP[3] == 0);
		}

		for (int i = 0; i < 8; i++) {
			JProgramInfo->class23[i].allist[0].al_flag = 1;
			JProgramInfo->class23[i].allist[0].cal_flag = 0;
			JProgramInfo->class23[i].allist[0].curNP[0] = 100;
			JProgramInfo->class23[i].allist[0].curNP[1] = 20;
			JProgramInfo->class23[i].allist[0].curNP[2] = 30;
			JProgramInfo->class23[i].allist[0].curNP[3] = 20;
			JProgramInfo->class23[i].allist[0].curNP[4] = 30;
		}

		SumUpRefreshUnit(&suu);
		for (int i = 0; i < 8; i++) {
			assert(JProgramInfo->class23[i].DayPALL == 10000);
			assert(JProgramInfo->class23[i].DayP[0] == 2000);
			assert(JProgramInfo->class23[i].DayP[1] == 3000);
			assert(JProgramInfo->class23[i].DayP[2] == 2000);
			assert(JProgramInfo->class23[i].DayP[3] == 3000);
		}

		for (int i = 0; i < 8; i++) {
			JProgramInfo->class23[i].allist[0].al_flag = 1;
			JProgramInfo->class23[i].allist[0].cal_flag = 1;
			JProgramInfo->class23[i].allist[0].curNP[0] = 200;
			JProgramInfo->class23[i].allist[0].curNP[1] = 40;
			JProgramInfo->class23[i].allist[0].curNP[2] = 60;
			JProgramInfo->class23[i].allist[0].curNP[3] = 40;
			JProgramInfo->class23[i].allist[0].curNP[4] = 60;
		}

		SumUpRefreshUnit(&suu);
		for (int i = 0; i < 8; i++) {
			assert(JProgramInfo->class23[i].DayPALL == 0);
			assert(JProgramInfo->class23[i].DayP[0] == 0);
			assert(JProgramInfo->class23[i].DayP[1] == 0);
			assert(JProgramInfo->class23[i].DayP[2] == 0);
			assert(JProgramInfo->class23[i].DayP[3] == 0);
		}

		memset(suu.curP, 0x00, sizeof(suu.curP));
		memset(suu.class23, 0x00, sizeof(CLASS23) * 8);

		JProgramInfo->class12[0].addr[0] = 0x05;
		JProgramInfo->class12[0].addr[1] = 0x01;
		JProgramInfo->class12[0].addr[2] = 0x02;
		JProgramInfo->class12[0].addr[3] = 0x03;
		JProgramInfo->class12[0].addr[4] = 0x04;
		JProgramInfo->class12[0].addr[5] = 0x05;

		JProgramInfo->class12[1].addr[0] = 0x05;
		JProgramInfo->class12[1].addr[1] = 0x05;
		JProgramInfo->class12[1].addr[2] = 0x04;
		JProgramInfo->class12[1].addr[3] = 0x03;
		JProgramInfo->class12[1].addr[4] = 0x02;
		JProgramInfo->class12[1].addr[5] = 0x01;

		JProgramInfo->class12[0].day_pos_p[0] = 20000;
		JProgramInfo->class12[0].day_pos_p[1] = 20000;
		JProgramInfo->class12[0].day_pos_p[2] = 20000;
		JProgramInfo->class12[0].day_pos_p[3] = 20000;

		JProgramInfo->class12[0].p = 12345;
		JProgramInfo->class12[0].q = 123450;

		JProgramInfo->class12[1].day_pos_p[0] = 10000;
		JProgramInfo->class12[1].day_pos_p[1] = 10000;
		JProgramInfo->class12[1].day_pos_p[2] = 10000;
		JProgramInfo->class12[1].day_pos_p[3] = 10000;

		JProgramInfo->class12[1].p = 54321;
		JProgramInfo->class12[1].p = 543210;

		int index = 0;
		for (int sum_i = 0; sum_i < 8; sum_i++) {
			JProgramInfo->class23[sum_i].allist[7 - sum_i].tsa.addr[0] = 0x06;
			JProgramInfo->class23[sum_i].allist[7 - sum_i].tsa.addr[1] = 0x05;
			JProgramInfo->class23[sum_i].allist[7 - sum_i].tsa.addr[2] = 0x05;
			JProgramInfo->class23[sum_i].allist[7 - sum_i].tsa.addr[3] = 0x04;
			JProgramInfo->class23[sum_i].allist[7 - sum_i].tsa.addr[4] = 0x03;
			JProgramInfo->class23[sum_i].allist[7 - sum_i].tsa.addr[5] = 0x02;
			JProgramInfo->class23[sum_i].allist[7 - sum_i].tsa.addr[6] = 0x01;

		}
		for (int sum_i = 0; sum_i < 8; sum_i++) {
			for (int p_i = 0; p_i < 2; p_i++) {
				if (sumUpfindPulse(&suu, &pu, sum_i, p_i, &index) == 1) {
					assert(p_i == 1);
					assert(index == 7 - sum_i);
				}
			}
		}

		SumUpRefreshPulseUnit(&suu, &pu);
		SumUpRefreshUnit(&suu);

		for (int i = 0; i < 8; i++) {
			fprintf(stderr, "1=========%lld %lld %lld %lld %lld\n",
					JProgramInfo->class23[i].DayPALL, JProgramInfo->class23[i].DayP[0],
					JProgramInfo->class23[i].DayP[1], JProgramInfo->class23[i].DayP[2],
					JProgramInfo->class23[i].DayP[3]);
			fprintf(stderr, "==========%lld %lld %lld %lld %lld\n",
					JProgramInfo->class23[i].MonthPALL, JProgramInfo->class23[i].MonthP[0],
					JProgramInfo->class23[i].MonthP[1], JProgramInfo->class23[i].MonthP[2],
					JProgramInfo->class23[i].MonthP[3]);
		}

		sumUpCalcRemain(&suu);

		printf("finish!\n");

		exit(0);
	}

	int run = 1;

	while (run) {
		TS now;
		TSGet(&now);

		//一秒钟刷新一次
		if (secOld == now.Sec) {
			usleep(200 * 1000);
			continue;
		} else {
			secOld = now.Sec;
		}

		pluseRefreshUnit(&pu);
		SumUpRefreshPulseUnit(&suu, &pu);
		SumUpRefreshUnit(&suu);
		sumUpCalcRemain(&suu);

		if (secOld % 5 == 0) {
			dealCtrl();
		}

		CtrlStateSumUp();
		PackCtrlSituation();
		ShaningLED_F206();
		DoActuallyCtrl();

		HandlerCtrl();
		CheckCtrlControl();
	}
	return (void*)0;
}
