//
// Created by 周立海 on 2017/4/24.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>

#include "ctrl.h"
#include "crtl_base.h"
#include "pluse.h"
#include "Shmem.h"
#include "Objectdef.h"
#include "AccessFun.h"
#include "basedef.h"

extern INT8U get6001ObjByTSA(TSA addr, CLASS_6001* targetMeter);
extern ProgramInfo* JProgramInfo;
CtrlState * CtrlC;

typedef union { //control code
	INT16U u16b; //convenient to set value to 0
	struct { //only for little endian mathine!
		INT8U bak :6; //备用
		INT8U lun1_state :1; //轮次1-状态
		INT8U lun1_red :1; //轮次1-红灯
		INT8U lun1_green :1; //轮次1-绿灯
		INT8U lun2_state :1; //轮次2-状态
		INT8U lun2_red :1; //轮次2-红灯
		INT8U lun2_green :1; //轮次2-绿灯
		INT8U gongk_led :1; //功控灯
		INT8U diank_led :1; //电控灯
		INT8U alm_state :1; //告警状态
		INT8U baodian_led :1; //报警灯
	} ctrl;
} ctrlUN;

static ctrlUN ctrlunit, ctrlunit_old;

int ctrl_base_test() {
	printf("%d", CheckModelState());
	InitCtrlModel();
	int fd = OpenSerialPort();

	SetCtrl_CMD(fd, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);

	return 0;
}

int findPinSum(int sum_i, int p_i)
{
	if(sum_i == 0 && p_i == 0)
	{
		return 1;
	}

	for(int i = 0; i < 8; i++){
		int n = JProgramInfo->class23[sum_i].allist[i].tsa.addr[0];
		if (n == 0)
			continue;
		for(int a_i = 0; a_i < n; a_i++){
			JProgramInfo->class23[sum_i].allist[i].tsa.addr[a_i] != JProgramInfo->class12[p_i].addr[a_i];
			return 0;
		}
		fprintf(stderr, "++++++++++++++++++++++查找电表表号 %d\n", n);
		return 1;
	}
}

//刷新总加组
void refreshSumUp() {
	static int old_day;
	static int old_month;
	static int first_flag = 1;

	TS ts;
	TSGet(&ts);

	if(first_flag == 1) {
		first_flag = 0;
		old_day = ts.Day;
		old_month = ts.Month;
	}

	for (int sum_i = 0; sum_i < 8; sum_i++)
	{
		for(int p_i = 0; p_i < 2; p_i++)
		{
			if(findPinSum(sum_i, p_i) == 0){
				continue;
			}
			JProgramInfo->class23[sum_i].p = JProgramInfo->class12[p_i].p;
			JProgramInfo->class23[sum_i].q = JProgramInfo->class12[p_i].q;

			for (int i = 0; i < 4; i++) {
				JProgramInfo->class23[sum_i].DayP[i] =
						JProgramInfo->class12[p_i].day_pos_p[i];
				JProgramInfo->class23[sum_i].DayQ[i] =
						JProgramInfo->class12[p_i].day_pos_q[i];
				JProgramInfo->class23[sum_i].MonthP[i] =
						JProgramInfo->class12[p_i].mon_pos_p[i];
				JProgramInfo->class23[sum_i].MonthQ[i] =
						JProgramInfo->class12[p_i].mon_pos_q[i];
			}
		}
	}

	fprintf(stderr, "总加组功率%d 电量%lld %lld %lld %lld\n",
			JProgramInfo->class23[0].p, JProgramInfo->class23[0].DayP[0],
			JProgramInfo->class23[0].DayP[1], JProgramInfo->class23[0].DayP[2],
			JProgramInfo->class23[0].DayP[3]);

	if (old_day != ts.Day) {
		old_day = ts.Day;
		for (int i = 0; i < 2; i++) {
			memset(&JProgramInfo->class12[i].day_nag_p[0], 0x00,
					sizeof(JProgramInfo->class12[i].day_nag_p));
			memset(&JProgramInfo->class12[i].day_nag_q[0], 0x00,
					sizeof(JProgramInfo->class12[i].day_nag_q));
			memset(&JProgramInfo->class12[i].day_pos_p[0], 0x00,
					sizeof(JProgramInfo->class12[i].day_pos_p));
			memset(&JProgramInfo->class12[i].day_pos_q[0], 0x00,
					sizeof(JProgramInfo->class12[i].day_pos_q));
		}
	}

	if (old_month != ts.Month) {
		old_month = ts.Month;
		for (int i = 0; i < 2; i++) {
			memset(&JProgramInfo->class12[i].mon_nag_p[0], 0x00,
					sizeof(JProgramInfo->class12[i].mon_nag_p));
			memset(&JProgramInfo->class12[i].mon_nag_q[0], 0x00,
					sizeof(JProgramInfo->class12[i].mon_nag_q));
			memset(&JProgramInfo->class12[i].mon_pos_p[0], 0x00,
					sizeof(JProgramInfo->class12[i].mon_pos_p));
			memset(&JProgramInfo->class12[i].mon_pos_q[0], 0x00,
					sizeof(JProgramInfo->class12[i].mon_pos_q));
		}
	}

}

//检查参数更新
void CheckParaUpdate() {

}

int initAll() {

	//控制状态初始化
	ctrlunit.u16b = 0;
	ctrlunit_old.u16b = 0;

	fprintf(stderr, "==============================ctrl init============================\n");

	//读取总加组数据
	CtrlC = &JProgramInfo->ctrls;
	CtrlC->control_event = 0;
	memset(&JProgramInfo->class12[0], 0x00, sizeof(CLASS12) * 2);
	memset(CtrlC, 0x00, sizeof(CtrlState));

	for (int i = 0; i < 8; ++i) {
		memset(&JProgramInfo->class23[i], 0x00, sizeof(CLASS23));
		readCoverClass(0x2301 + i, 0, &JProgramInfo->class23[i],
				sizeof(CLASS23), para_vari_save);
		JProgramInfo->class23[i].alCtlState.OutputState = 0;
		JProgramInfo->class23[i].alCtlState.BuyOutputState = 0;
		JProgramInfo->class23[i].alCtlState.MonthOutputState = 0;
		JProgramInfo->class23[i].alCtlState.PCAlarmState = 0;
		JProgramInfo->class23[i].alCtlState.ECAlarmState = 0;
	}


	for (int i = 0; i < 2; ++i) {
		memset(&JProgramInfo->class12[i], 0x00, sizeof(CLASS12));
		readCoverClass(0x2401 + i, 0, &JProgramInfo->class12[i],
				sizeof(CLASS12), para_vari_save);
	}

	readCoverClass(0x8100, 0, &CtrlC->c8100, sizeof(CLASS_8100),
			para_vari_save);
	readCoverClass(0x8101, 0, &CtrlC->c8101, sizeof(CLASS_8101),
			para_vari_save);
	readCoverClass(0x8102, 0, &CtrlC->c8102, sizeof(CLASS_8102),
			para_vari_save);
	readCoverClass(0x8103, 0, &CtrlC->c8103, sizeof(CLASS_8103),
			para_vari_save);
	readCoverClass(0x8104, 0, &CtrlC->c8104, sizeof(CLASS_8104),
			para_vari_save);
	readCoverClass(0x8105, 0, &CtrlC->c8105, sizeof(CLASS_8105),
			para_vari_save);
	readCoverClass(0x8106, 0, &CtrlC->c8106, sizeof(CLASS_8106),
			para_vari_save);
	readCoverClass(0x8107, 0, &CtrlC->c8107, sizeof(CLASS_8107),
			para_vari_save);
	readCoverClass(0x8108, 0, &CtrlC->c8108, sizeof(CLASS_8108),
			para_vari_save);


	fprintf(stderr, "==============================ctrl init end============================\n");

	return 0;
}

int getBit(INT8U v, int index) {
	return (v >> (index)) & 0x01;
}

int setBit(INT8U v, int index) {
	return (0x01 << (index)) | v;
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
INT64U getCurrTimeValue(int line) {
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
		int inner_type = getBit(CtrlC->c8101.time[inner_index1],
				inner_index2 * 2)
				+ getBit(CtrlC->c8101.time[inner_index1], inner_index2 * 2 + 1)
						* 2;
		if (curr_type != inner_type && inner_type != 0) {
			curr_type = inner_type;
			time_num++;
		}
	}
	int index = CtrlC->c8103.numb;
	fprintf(stderr, "时段功控计算时段 index1 %d index2 %d last %d index %d\n",
			time_index1, time_index2, time_num, index);
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
	default:
		return CtrlC->c8103.list[line].v1.t1;
	}
}

int deal8103() {
	static int step[8];
	static int count[8];

	for (int i = 0; i < 8; i++) {
		if (JProgramInfo->ctrls.c8103.enable[i].state == 0) {
			step[i] = 0;
			count[i] = 0;
			continue;
		}

		INT64U val = getCurrTimeValue(i);
		fprintf(stderr, "时段功控限值(%lld Compare %lld) index=%d\n", val,
				JProgramInfo->class23[i].p, i);

		if (val < JProgramInfo->class23[i].p) {
			fprintf(stderr, "进入时段功控时间，判断功率%lld\n", JProgramInfo->class23[i].p);
			switch (step[i]) {
			case 0:
				JProgramInfo->class23[i].alCtlState.OutputState |= 0;
				JProgramInfo->class23[i].alCtlState.PCAlarmState |= 128;
				fprintf(stderr, "时段功控，告警！！！！！！！！！！！！！功控告警时间 %d %d\n",
						CtrlC->c8102.time[0] * 60, count[i] * 5);
				if (count[i] * 5 >= (CtrlC->c8102.time[0]) * 60) {
					JProgramInfo->class23[i].alCtlState.OutputState |= 128;
					JProgramInfo->class23[i].alCtlState.PCAlarmState &= ~128;
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
				JProgramInfo->class23[i].alCtlState.OutputState |= 128;
				JProgramInfo->class23[i].alCtlState.PCAlarmState &= ~128;
				if (count[i] * 5 >= (CtrlC->c8102.time[1]) * 60) {
					fprintf(stderr, "时段功控，二轮跳闸！！！！！！！！！！！！！");
					JProgramInfo->class23[i].alCtlState.OutputState |= 192;
					JProgramInfo->class23[i].alCtlState.PCAlarmState &= ~128;
					step[i] = 2;
				}
				count[i] += 1;
				break;
			case 2:
				JProgramInfo->class23[i].alCtlState.OutputState |= 192;
				JProgramInfo->class23[i].alCtlState.PCAlarmState &= ~128;
				break;
			}
		} else {
			step[i] = 0;
			count[i] = 0;
		}
	}
	return 0;
}

//计算当前时间是否在厂休时段范围内
INT64U getIsInTime(int line) {
	TS ts;
	TSGet(&ts);
//	fprintf(stderr, "~~~~~~~~~~~~~刷新参数 %lld %04x %02x %d\n",  JProgramInfo->ctrls.c8104.list[0].v, JProgramInfo->ctrls.c8104.list[0].sustain, JProgramInfo->ctrls.c8104.list[0].noDay, line);
	if (getBit(0x7F, ts.Week) == 0) {
		fprintf(stderr, "不是限电日！！！！！！！！！！（%d）～～～～～～～～～～～\n",
				CtrlC->c8104.list[line].noDay);
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

	tminc(&start, 1, CtrlC->c8104.list[line].sustain);

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
	static int bar1[128];
	static int step[8];
	static int count[8];
	static int bar2[128];

	for (int i = 0; i < 1; i++) {
		if (JProgramInfo->ctrls.c8104.enable[i].state == 0) {
			step[i] = 0;
			count[i] = 0;
			continue;
		}

		INT64U val = getIsInTime(i);
		fprintf(stderr, "厂休控限值(%lld Compare %lld) index=%d\n", val,
				JProgramInfo->class23[i].p, i);
		if (val < JProgramInfo->class23[i].p) {
//		if(1){
			fprintf(stderr, "进入厂休控时间，判断功率%lld\n", JProgramInfo->class23[i].p);
			switch (step[i]) {
			case 0:
				JProgramInfo->class23[i].alCtlState.OutputState |= 0;
				JProgramInfo->class23[i].alCtlState.PCAlarmState |= 64;
				fprintf(stderr, "厂休控，告警！！！！！！！！！！！！！功控告警时间 %d %d\n",
						CtrlC->c8102.time[0] * 60, count[i] * 5);
				if (count[i] * 5 >= (CtrlC->c8102.time[0]) * 60) {
					JProgramInfo->class23[i].alCtlState.OutputState |= 128;
					JProgramInfo->class23[i].alCtlState.PCAlarmState &= ~64;
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
				JProgramInfo->class23[i].alCtlState.OutputState |= 128;
				JProgramInfo->class23[i].alCtlState.PCAlarmState &= ~64;
				if (count[i] * 5 >= (CtrlC->c8102.time[1]) * 60) {
					fprintf(stderr, "厂休控，二轮跳闸！！！！！！！！！！！！！");
					JProgramInfo->class23[i].alCtlState.OutputState |= 192;
					JProgramInfo->class23[i].alCtlState.PCAlarmState &= ~64;
					step[i] = 2;
				}
				count[i] += 1;
				break;
			case 2:
				JProgramInfo->class23[i].alCtlState.OutputState |= 192;
				JProgramInfo->class23[i].alCtlState.PCAlarmState &= ~64;
				break;
			}
		} else {
			step[i] = 0;
			count[i] = 0;
		}
	}
	return 0;
}

//计算当前时间是否在厂休时段范围内
INT64U getIsStop(int line) {
	TS ts;
	TSGet(&ts);

//	fprintf(stderr, "@@@@ %d %d %d %d %d %d\n", CtrlC->c8105.list[line].start.year, CtrlC->c8105.list[line].start.month, CtrlC->c8105.list[line].start.day, CtrlC->c8105.list[line].start.hour, CtrlC->c8105.list[line].start.min, 0);
//	fprintf(stderr, "@@@@ %d %d %d %d %d %d\n", CtrlC->c8105.list[line].end.year, CtrlC->c8105.list[line].end.month, CtrlC->c8105.list[line].end.day, CtrlC->c8105.list[line].end.hour, CtrlC->c8105.list[line].end.min, 0);

	fprintf(stderr, "@@@@ %d %d\n",
			TScompare(ts, CtrlC->c8105.list[line].start),
			TScompare(ts, CtrlC->c8105.list[line].end));
	if (TScompare(ts, CtrlC->c8105.list[line].start) == 1
			&& TScompare(ts, CtrlC->c8105.list[line].end) == 2) {
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
		if (JProgramInfo->ctrls.c8105.enable[i].state == 0) {
			step[i] = 0;
			count[i] = 0;
			continue;
		}

		INT64U val = getIsStop(i);
		fprintf(stderr, "营业报停控限值(%lld Compare %lld) index=%d\n", val,
				JProgramInfo->class23[i].p, i);
		if (val <= JProgramInfo->class23[i].p) {
			fprintf(stderr, "进入营业报停控时间，判断功率%lld\n", JProgramInfo->class23[i].p);
			switch (step[i]) {
			case 0:
				JProgramInfo->class23[i].alCtlState.OutputState |= 0;
				JProgramInfo->class23[i].alCtlState.PCAlarmState |= 32;
				fprintf(stderr, "营业报停控，告警！！！！！！！！！！！！！功控告警时间 %d %d\n",
						CtrlC->c8102.time[0] * 60, count[i] * 5);
				if (count[i] * 5 >= (CtrlC->c8102.time[0]) * 60) {
					JProgramInfo->class23[i].alCtlState.OutputState |= 128;
					JProgramInfo->class23[i].alCtlState.PCAlarmState &= ~32;
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
				JProgramInfo->class23[i].alCtlState.OutputState |= 128;
				JProgramInfo->class23[i].alCtlState.PCAlarmState &= ~32;
				if (count[i] * 5 >= (CtrlC->c8102.time[1]) * 60) {
					fprintf(stderr, "营业报停控，二轮跳闸！！！！！！！！！！！！！");
					JProgramInfo->class23[i].alCtlState.OutputState |= 192;
					JProgramInfo->class23[i].alCtlState.PCAlarmState &= ~32;
					step[i] = 2;
				}
				count[i] += 1;
				break;
			case 2:
				JProgramInfo->class23[i].alCtlState.OutputState |= 192;
				JProgramInfo->class23[i].alCtlState.PCAlarmState &= ~32;
				break;
			}
		} else {
			step[i] = 0;
			count[i] = 0;
		}

	}
	return 0;
}

//计算当前时间是否在功率下浮控范围内
INT64U getIsInDown(TS start, int line) {
	TS ts;
	TSGet(&ts);
	fprintf(stderr, "@@@@ %d %d %d %d %d %d\n", start.Year, start.Month,
			start.Day, start.Hour, start.Minute, 0);
	tminc(&start, 1, CtrlC->c8106.list.down_ctrl_time * 30);
//		fprintf(stderr, "@@@@ %d %d %d %d %d %d\n",start.Year, start.Month, start.Day, start.Hour, start.Minute, 0);
//		fprintf(stderr, "@@@@ %d %d %d %d %d %d\n", ts.Year, ts.Month, ts.Day, ts.Hour, ts.Minute, 0);

//	fprintf(stderr, "控制时间内！！！！！！！！！！[%d][%d]～～～～～～～～～～～\n",TScompare(ts, start), CtrlC->c8106.list.down_ctrl_time);
	if (TScompare(ts, start) == 2) {
		fprintf(stderr, "控制时间内！！！！！！！！！！[%lld]～～～～～～～～～～～\n",
				JProgramInfo->class23[line].p
						* ((CtrlC->c8106.list.down_xishu + 100) / 100.0));
		return JProgramInfo->class23[line].p
				* ((CtrlC->c8106.list.down_xishu + 100) / 100.0);
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
	static INT64U val;

	if (JProgramInfo->ctrls.c8106.enable.state == 0) {
		step = 0;
		count = 0;
		freeze_count = 0;
		TSGet(&start);
		val = 0;
		return 0;
	}

	//冻结时间
	fprintf(stderr, "功率下浮控冻结时间(%d)\n",
			JProgramInfo->ctrls.c8106.list.down_freeze);
	if (freeze_count * 5 < JProgramInfo->ctrls.c8106.list.down_freeze * 60) {
		freeze_count++;
		return 0;
	}

	TSGet(&start);

	int i = JProgramInfo->ctrls.c8106.index - 0x2301;
	if (i > 7 || i < 0) {
		i = 0;
	}

	if (val <= 0) {
		val = getIsInDown(start, i);
		fprintf(stderr, "更新功率下浮(%lld)\n", val);
	}

	fprintf(stderr, "功率下浮控限值(%lld Compare %lld)\n", val,
			JProgramInfo->class23[i].p);
	if (val <= JProgramInfo->class23[i].p) {
		fprintf(stderr, "进入功率下浮控时间，判断功率%lld\n", JProgramInfo->class23[i].p);
		switch (step) {
		case 0:
			JProgramInfo->class23[i].alCtlState.OutputState |= 0;
			JProgramInfo->class23[i].alCtlState.PCAlarmState |= 16;
			fprintf(stderr, "功率下浮控，告警！！！！！！！！！！！！！功控告警时间 %d %d\n",
					CtrlC->c8102.time[0] * 60, count * 5);
			if (count * 5 >= (CtrlC->c8102.time[0]) * 60) {
				JProgramInfo->class23[i].alCtlState.OutputState |= 128;
				JProgramInfo->class23[i].alCtlState.PCAlarmState &= ~16;
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
			JProgramInfo->class23[i].alCtlState.OutputState |= 128;
			JProgramInfo->class23[i].alCtlState.PCAlarmState &= ~16;
			if (count * 5 >= (CtrlC->c8102.time[1]) * 60) {
				fprintf(stderr, "功率下浮控，二轮跳闸！！！！！！！！！！！！！");
				JProgramInfo->class23[i].alCtlState.OutputState |= 192;
				JProgramInfo->class23[i].alCtlState.PCAlarmState &= ~16;
				step = 2;
			}
			count += 1;
			break;
		case 2:
			JProgramInfo->class23[i].alCtlState.OutputState |= 192;
			JProgramInfo->class23[i].alCtlState.PCAlarmState &= ~16;
			break;
		}
	} else {
		step = 0;
		count = 0;
	}

	return 0;
}

int deal8107() {
//购电控不受购电配置单元的影响，购电配置单元作为总加组剩余电量刷新的依据

	fprintf(stderr, "deal8107(%lld)\n", CtrlC->c8107.list[0].ctrl);
	for (int i = 0; i < 1; i++) {
		if (!CheckAllUnitEmpty(JProgramInfo->class23[i].allist)) {
			continue;
		}
		fprintf(stderr, "8107 index = %d\n", i);

		if (JProgramInfo->ctrls.c8107.enable[i].state == 0) {
			return 0;
		}

		INT64U val = CtrlC->c8107.list[0].ctrl;
		INT64U warn = CtrlC->c8107.list[0].alarm;
		fprintf(stderr, "购电控限制[%lld] [%lld]\n", val, warn);

		if (val >= 0) {
			fprintf(stderr, "购电判断值[%lld]\n", JProgramInfo->class23[i].remains);

			if (JProgramInfo->class23[i].remains <= val) {
				fprintf(stderr, "购电控跳闸 ！！！！！！！！！！！！！！！！！！\n");
				JProgramInfo->class23[i].alCtlState.OutputState |= 192;
				JProgramInfo->class23[i].alCtlState.ECAlarmState |= 0;
				JProgramInfo->class23[i].alCtlState.BuyOutputState |= 192;
				return 2;
			}

			if (JProgramInfo->class23[i].remains <= warn) {
				fprintf(stderr, "购电控告警！！！！！！！！！！！！！！！！！！\n");
				JProgramInfo->class23[i].alCtlState.ECAlarmState |= 64;
				JProgramInfo->class23[i].alCtlState.OutputState |= 0;
				JProgramInfo->class23[i].alCtlState.BuyOutputState |= 0;
				return 1;
			}
		}
	}
	return 0;
}

int deal8108() {
	fprintf(stderr, "deal8108(%lld)\n", CtrlC->c8108.list[0].v);
	for (int i = 0; i < 1; i++) {
		if (!CheckAllUnitEmpty(JProgramInfo->class23[i].allist)) {
			continue;
		}
		fprintf(stderr, "8108 index = %d\n", i);

		if (JProgramInfo->ctrls.c8108.enable[i].state == 0) {
			return 0;
		}

		INT64U val = CtrlC->c8108.list[i].v;
		INT8U warn = CtrlC->c8108.list[i].para;
		fprintf(stderr, "月电控限制%lld\n", val);

		long long total = 0;
		for (int i = 0; i < MAXVAL_RATENUM; i++) {
			total += JProgramInfo->class23[i].MonthP[i];
		}

		if (val != -1) {
			float e = warn / 100.0;

			fprintf(stderr, "月电控值%lld [%f]\n",
					JProgramInfo->class23[i].MonthPALL * 100, e * val);

			if (JProgramInfo->class23[i].MonthPALL * 100 > val) {
				fprintf(stderr, "月电控跳闸！！！！！！！！！！！！！！！！！！\n", val);
				JProgramInfo->class23[i].alCtlState.OutputState |= 192;
				JProgramInfo->class23[i].alCtlState.MonthOutputState |= 192;
				JProgramInfo->class23[i].alCtlState.ECAlarmState = 0;
				return 2;
			}

			if (JProgramInfo->class23[i].MonthPALL * 100 > e * val) {
				fprintf(stderr, "月电控告警！！！！！！！！！！！！！！！！！！\n", val);
				JProgramInfo->class23[i].alCtlState.ECAlarmState |= 128;
				JProgramInfo->class23[i].alCtlState.OutputState |= 0;
				JProgramInfo->class23[i].alCtlState.MonthOutputState |= 0;
				return 1;
			}
		}
	}
	return 0;
}

void dealCtrl() {
	//直接跳闸，必须检测
	deal8107();
	deal8108();

	//检测控制有优先级，当高优先级条件产生时，忽略低优先级的配置

	deal8106();
	deal8105();
	deal8104();
	deal8103();
}

int ctrlMain(void * arg) {

	int secOld = 0;
	int ctrlflg = 0;
	int count = 0;
//初始化参数,搭建8个总加组数据，读取功控、电控参数
	initAll();

	while (1) {
		TS now;
		TSGet(&now);

		//一秒钟刷新一次脉冲数据
		if (secOld != now.Sec) {

			refreshPluse(secOld);
			//更新总加组数据
			refreshSumUp();
			secOld = now.Sec;

			fprintf(stderr, "++++++++++++++++++++++查找电表表号 %d\n", JProgramInfo->ctrls.c8102.time[0]);

			//一分钟计算一次控制逻辑
			if (secOld % 5 == 0) {

				//检查参数更新
				//			CheckParaUpdate();

				//处理控制逻辑
				dealCtrl();
			}
			if (secOld % 57 == 0) {
				for (int i = 0; i < 8; ++i) {
					saveCoverClass(0x2301 + i, 0, &JProgramInfo->class23[i],
							sizeof(CLASS23), para_vari_save);
				}
			}
			if (secOld % 51 == 0) {
				for (int i = 0; i < 2; ++i) {
					saveCoverClass(0x2401 + i, 0, &JProgramInfo->class12[i],
							sizeof(CLASS12), para_vari_save);
				}
			}

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

		if (JProgramInfo->ctrls.control[0] == 0xEEFFEFEF
				&& JProgramInfo->ctrls.control[1] == 0xEEFFEFEF
				&& JProgramInfo->ctrls.control[2] == 0xEEFFEFEF) { //分闸
			ctrlunit.ctrl.lun1_state = 0;
			ctrlunit.ctrl.lun1_red = 1;
			ctrlunit.ctrl.lun1_green = 0;
			ctrlflg = 1;
		} else if (JProgramInfo->ctrls.control[0] == 0xCCAACACA
				&& JProgramInfo->ctrls.control[1] == 0xCCAACACA
				&& JProgramInfo->ctrls.control[2] == 0xCCAACACA) { //合闸
			ctrlunit.ctrl.lun1_state = 1;
			ctrlunit.ctrl.lun1_red = 0;
			ctrlunit.ctrl.lun1_green = 1;
			ctrlflg = 1;
		} else if (JProgramInfo->ctrls.control[0] == 0x55552525
				&& JProgramInfo->ctrls.control[1] == 0x55552525
				&& JProgramInfo->ctrls.control[2] == 0x55552525) { //分闸
			ctrlunit.ctrl.lun2_state = 0;
			ctrlunit.ctrl.lun2_red = 1;
			ctrlunit.ctrl.lun2_green = 0;
			ctrlflg = 1;
		} else if (JProgramInfo->ctrls.control[0] == 0xCCCC2C2C
				&& JProgramInfo->ctrls.control[1] == 0xCCCC2C2C
				&& JProgramInfo->ctrls.control[2] == 0xCCCC2C2C) { //合闸
			ctrlunit.ctrl.lun2_state = 1;
			ctrlunit.ctrl.lun2_red = 0;
			ctrlunit.ctrl.lun2_green = 1;
			ctrlflg = 1;
		} else
			ctrlflg = 0;
		if (ctrlflg == 1) {
			memset(&JProgramInfo->ctrls.control, 0,
					sizeof(JProgramInfo->ctrls.control));
			asyslog(LOG_NOTICE, "接收到控制命令：控制状态【%04x】 原状态【%04x】", ctrlunit.u16b,
					ctrlunit_old.u16b);
			if ((ctrlunit.u16b & 0x3ff) ^ (ctrlunit_old.u16b & 0x3ff)) {
				ctrlunit_old.u16b = ctrlunit.u16b;
				printf("%d", CheckModelState());
				InitCtrlModel();
				int fd = OpenSerialPort();
				SetCtrl_CMD(fd, ctrlunit.ctrl.lun1_state,
						ctrlunit.ctrl.lun1_red, ctrlunit.ctrl.lun1_green,
						ctrlunit.ctrl.lun2_state, ctrlunit.ctrl.lun2_red,
						ctrlunit.ctrl.lun2_green, ctrlunit.ctrl.gongk_led,
						ctrlunit.ctrl.diank_led, ctrlunit.ctrl.alm_state,
						ctrlunit.ctrl.baodian_led);
				close(fd);
			}
		}

		usleep(300 * 1000);
	}
}
