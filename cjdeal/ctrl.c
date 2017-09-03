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

//刷新总加组
void refreshSumUp() {
	static int old_day;
	static int old_month;

	TS ts;
	TSGet(&ts);

	JProgramInfo->class23[0].p = JProgramInfo->class12[0].p;
	JProgramInfo->class23[0].q = JProgramInfo->class12[0].q;

	for (int i = 0; i < 4; i++) {
		JProgramInfo->class23[0].DayP[i] =
				JProgramInfo->class12[0].day_pos_p[i];
		JProgramInfo->class23[0].DayQ[i] =
				JProgramInfo->class12[0].day_pos_q[i];
		JProgramInfo->class23[0].MonthP[i] =
				JProgramInfo->class12[0].mon_pos_p[i];
		JProgramInfo->class23[0].MonthQ[i] =
				JProgramInfo->class12[0].mon_pos_q[i];
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

	//读取总加组数据
	CtrlC = &JProgramInfo->ctrls;
	memset(CtrlC, 0x00, sizeof(CtrlState));
	for (int i = 0; i < 8; ++i) {
		memset(&JProgramInfo->class23[i], 0x00, sizeof(CLASS23));
		readCoverClass(0x2301 + i, 0, &JProgramInfo->class23[i],
				sizeof(CLASS23), para_vari_save);
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
	fprintf("时段功控计算时段 index1 %d index2 %d last %d index %d\n", time_index1,
			time_index2, time_num, index);
	switch (index) {
	case 0:
		switch (time_num) {
		case 1:
			return CtrlC->c8103.list[line].v1.t1;
		case 2:
			return CtrlC->c8103.list[line].v1.t2;
		case 3:
			return CtrlC->c8103.list[line].v1.t3;
		case 4:
			return CtrlC->c8103.list[line].v1.t4;
		case 5:
			return CtrlC->c8103.list[line].v1.t5;
		case 6:
			return CtrlC->c8103.list[line].v1.t6;
		case 7:
			return CtrlC->c8103.list[line].v1.t7;
		case 8:
			return CtrlC->c8103.list[line].v1.t8;
		default:
			return -1;
		}

	case 1:
		switch (time_num) {
		case 1:
			return CtrlC->c8103.list[line].v2.t1;
		case 2:
			return CtrlC->c8103.list[line].v2.t2;
		case 3:
			return CtrlC->c8103.list[line].v2.t3;
		case 4:
			return CtrlC->c8103.list[line].v2.t4;
		case 5:
			return CtrlC->c8103.list[line].v2.t5;
		case 6:
			return CtrlC->c8103.list[line].v2.t6;
		case 7:
			return CtrlC->c8103.list[line].v2.t7;
		case 8:
			return CtrlC->c8103.list[line].v2.t8;
		default:
			return -1;
		}
	case 2:
		switch (time_num) {
		case 1:
			return CtrlC->c8103.list[line].v3.t1;
		case 2:
			return CtrlC->c8103.list[line].v3.t2;
		case 3:
			return CtrlC->c8103.list[line].v3.t3;
		case 4:
			return CtrlC->c8103.list[line].v3.t4;
		case 5:
			return CtrlC->c8103.list[line].v3.t5;
		case 6:
			return CtrlC->c8103.list[line].v3.t6;
		case 7:
			return CtrlC->c8103.list[line].v3.t7;
		case 8:
			return CtrlC->c8103.list[line].v3.t8;
		default:
			return -1;
		}
	default:
		return CtrlC->c8103.list[line].v1.t1;
	}
}

int deal8103() {
	static int step = 0;
	static int count = 0;

	for (int i = 0; i < 8; i++) {
		if (JProgramInfo->ctrls.c8103.enable[i].state == 0) {
			step = 0;
			count = 0;
			return 0;
		}

		INT64U val = getCurrTimeValue(i);
		fprintf(stderr, "时段功控限值(%lld Compare %lld) index=%d\n", val,
				JProgramInfo->class23[i].p, i);

		if (val < JProgramInfo->class23[i].p) {
			fprintf(stderr, "进入时段功控时间，判断功率%lld\n", JProgramInfo->class23[i].p);
			switch (step) {
			case 0:
				JProgramInfo->class23[i].alCtlState.OutputState |= 0;
				JProgramInfo->class23[i].alCtlState.PCAlarmState |= 128;
				fprintf(stderr, "时段功控，告警！！！！！！！！！！！！！");
				fprintf(stderr, "功控告警时间 %d\n", CtrlC->c8102.time[0] * 60);
				if (count * 5 > (CtrlC->c8102.time[0]) * 60) {
					fprintf(stderr, "时段功控，一轮跳闸！！！！！！！！！！！！！");
					JProgramInfo->class23[i].alCtlState.OutputState = 128;
					JProgramInfo->class23[i].alCtlState.PCAlarmState = 0;
					step = 1;
				}
				count += 1;
				break;
			case 1:
				fprintf(stderr, "时段功控，二轮告警！！！！！！！！！！！！！");
				fprintf(stderr, "功控告警时间 %d\n", CtrlC->c8102.time[1] * 60);
				if (count * 5 > (CtrlC->c8102.time[1]) * 60) {
					fprintf(stderr, "时段功控，二轮跳闸！！！！！！！！！！！！！");
					JProgramInfo->class23[i].alCtlState.OutputState |= 192;
					JProgramInfo->class23[i].alCtlState.PCAlarmState |= 0;
					step = 2;
				}
				count += 1;
				break;
			case 2:
				JProgramInfo->class23[i].alCtlState.OutputState |= 192;
				JProgramInfo->class23[i].alCtlState.PCAlarmState |= 0;
				count = 0;
				break;
			}
		} else {
			step = 0;
			count = 0;
		}
	}
	return 0;
}

//计算当前时间是否在厂休时段范围内
INT64U getIsInTime(int line) {
	TS ts;
	TSGet(&ts);

	if (getBit(CtrlC->c8104.list[line].noDay, ts.Week) == 0) {
		fprintf(stderr, "不是限电日！！！！！！！！！！～～～～～～～～～～～\n");
		return 0;
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
	if (TScompare(start, ts) == 1) {
		CtrlC->c8104.list[line].v;
	} else {
		return -1;
		fprintf(stderr, "不在厂休范围内！！！！！！！！！！～～～～～～～～～～～\n");
	}
}

int deal8104() {
	static int step = 0;
	static int count = 0;

	for (int i = 0; i < 1; i++) {
		if (JProgramInfo->ctrls.c8104.enable[i].state == 0) {
			step = 0;
			count = 0;
			return 0;
		}

		INT64U val = getIsInTime(i);
		fprintf(stderr, "厂休控限值(%lld Compare %lld) index=%d\n", val,
				JProgramInfo->class23[i].p, i);
		if (val <= JProgramInfo->class23[i].p) {
			fprintf(stderr, "进入厂休控时间，判断功率%lld\n", JProgramInfo->class23[i].p);
			switch (step) {
			case 0:
				JProgramInfo->class23[i].alCtlState.OutputState |= 0;
				JProgramInfo->class23[i].alCtlState.PCAlarmState |= 64;
				fprintf(stderr, "厂休控，告警！！！！！！！！！！！！！");
				fprintf(stderr, "功控告警时间 %d\n", CtrlC->c8102.time[0] * 60);
				if (count * 5 > (CtrlC->c8102.time[0]) * 60) {
					fprintf(stderr, "厂休控，一轮跳闸！！！！！！！！！！！！！");
					JProgramInfo->class23[i].alCtlState.OutputState = 128;
					JProgramInfo->class23[i].alCtlState.PCAlarmState = 0;
					step = 1;
				}
				count += 1;
				break;
			case 1:
				fprintf(stderr, "厂休控，二轮告警！！！！！！！！！！！！！");
				fprintf(stderr, "功控告警时间 %d\n", CtrlC->c8102.time[1] * 60);
				if (count * 5 > (CtrlC->c8102.time[1]) * 60) {
					fprintf(stderr, "厂休控，二轮跳闸！！！！！！！！！！！！！");
					JProgramInfo->class23[i].alCtlState.OutputState |= 192;
					JProgramInfo->class23[i].alCtlState.PCAlarmState |= 0;
					step = 2;
				}
				count += 1;
				break;
			case 2:
				JProgramInfo->class23[i].alCtlState.OutputState |= 192;
				JProgramInfo->class23[i].alCtlState.PCAlarmState |= 0;
				count = 0;
				break;
			}
		} else {
			step = 0;
			count = 0;
		}
	}
	return 0;
}

//计算当前时间是否在厂休时段范围内
INT64U getIsStop(int line) {
	TS ts;
	TSGet(&ts);

	if (TScompare(ts, CtrlC->c8105.list[line].start) == 1
			&& TScompare(ts, CtrlC->c8105.list[line].end) == 2) {
		return CtrlC->c8105.list[line].v;
	}
	return -1;
}

int deal8105() {
	static int step = 0;
	static int count = 0;

	for (int i = 0; i < 1; i++) {
		if (JProgramInfo->ctrls.c8105.enable[i].state == 0) {
			step = 0;
			count = 0;
			return 0;
		}

		INT64U val = getIsStop(i);
		fprintf(stderr, "营业报停控限值(%lld Compare %lld) index=%d\n", val,
				JProgramInfo->class23[i].p, i);
		if (val <= JProgramInfo->class23[i].p) {
			fprintf(stderr, "进入营业报停控时间，判断功率%lld\n", JProgramInfo->class23[i].p);
			switch (step) {
			case 0:
				JProgramInfo->class23[i].alCtlState.OutputState |= 0;
				JProgramInfo->class23[i].alCtlState.PCAlarmState |= 32;
				fprintf(stderr, "营业报停控，告警！！！！！！！！！！！！！");
				fprintf(stderr, "功控告警时间 %d\n", CtrlC->c8102.time[0] * 60);
				if (count * 5 > (CtrlC->c8102.time[0]) * 60) {
					fprintf(stderr, "营业报停控，一轮跳闸！！！！！！！！！！！！！");
					JProgramInfo->class23[i].alCtlState.OutputState = 128;
					JProgramInfo->class23[i].alCtlState.PCAlarmState = 0;
					step = 1;
				}
				count += 1;
				break;
			case 1:
				fprintf(stderr, "营业报停控，二轮告警！！！！！！！！！！！！！");
				fprintf(stderr, "功控告警时间 %d\n", CtrlC->c8102.time[1] * 60);
				if (count * 5 > (CtrlC->c8102.time[1]) * 60) {
					fprintf(stderr, "营业报停控，二轮跳闸！！！！！！！！！！！！！");
					JProgramInfo->class23[i].alCtlState.OutputState |= 192;
					JProgramInfo->class23[i].alCtlState.PCAlarmState |= 0;
					step = 2;
				}
				count += 1;
				break;
			case 2:
				JProgramInfo->class23[i].alCtlState.OutputState |= 192;
				JProgramInfo->class23[i].alCtlState.PCAlarmState |= 0;
				count = 0;
				break;
			}
		} else {
			step = 0;
			count = 0;
		}

	}
	return 0;
}

//计算当前时间是否在功率下浮控范围内
INT64U getIsInDown(int line) {
	TS start;
	TSGet(&start);

	tminc(&start, 1, CtrlC->c8104.list[line].sustain);
	if (TScompare(start, start) == 1) {
		return JProgramInfo->class23[line].p
				* ((CtrlC->c8106.list.down_xishu + 100) / 100.0);
	} else {
		return -1;
		fprintf(stderr, "不在功率下浮控控制时间内！！！！！！！！！！～～～～～～～～～～～\n");
	}
}

int deal8106() {
	TS ts;
	TSGet(&ts);
	static int step = 0;
	static int count = 0;
	static int freeze_count = 0;

	if (JProgramInfo->ctrls.c8106.enable.state == 0) {
		step = 0;
		count = 0;
		freeze_count = 0;
		return 0;
	}

	//冻结时间
	if (freeze_count * 5 < JProgramInfo->ctrls.c8106.list.down_freeze * 60) {
		return 0;
	}

	INT64U val = getIsInDown(1);
	int i = JProgramInfo->ctrls.c8106.index - 0x2301;
	if (i > 7 || i < 0)
	{
		i = 0;
	}

	fprintf(stderr, "功率下浮控限值(%lld Compare %lld)\n", val,
			JProgramInfo->class23[i].p);
	if (val <= JProgramInfo->class23[i].p) {
		fprintf(stderr, "进入功率下浮控时间，判断功率%lld\n", JProgramInfo->class23[i].p);
		switch (step) {
		case 0:
			JProgramInfo->class23[i].alCtlState.OutputState |= 0;
			JProgramInfo->class23[i].alCtlState.PCAlarmState |= 16;
			fprintf(stderr, "功率下浮控，告警！！！！！！！！！！！！！");
			fprintf(stderr, "功控告警时间 %d\n", CtrlC->c8102.time[0] * 60);
			if (count * 5 > (CtrlC->c8102.time[0]) * 60) {
				fprintf(stderr, "功率下浮控，一轮跳闸！！！！！！！！！！！！！");
				JProgramInfo->class23[i].alCtlState.OutputState = 128;
				JProgramInfo->class23[i].alCtlState.PCAlarmState = 0;
				step = 1;
			}
			count += 1;
			break;
		case 1:
			fprintf(stderr, "功率下浮控，二轮告警！！！！！！！！！！！！！");
			fprintf(stderr, "功控告警时间 %d\n", CtrlC->c8102.time[1] * 60);
			if (count * 5 > (CtrlC->c8102.time[1]) * 60) {
				fprintf(stderr, "功率下浮控，二轮跳闸！！！！！！！！！！！！！");
				JProgramInfo->class23[i].alCtlState.OutputState |= 192;
				JProgramInfo->class23[i].alCtlState.PCAlarmState |= 0;
				step = 2;
			}
			count += 1;
			break;
		case 2:
			JProgramInfo->class23[i].alCtlState.OutputState |= 192;
			JProgramInfo->class23[i].alCtlState.PCAlarmState |= 0;
			count = 0;
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
			JProgramInfo->class23[i].alCtlState.OutputState = 0;
			JProgramInfo->class23[i].alCtlState.ECAlarmState = 0;
			JProgramInfo->class23[i].alCtlState.BuyOutputState = 0;
			return 0;
		}

		INT64U val = CtrlC->c8107.list[0].ctrl;
		INT64U warn = CtrlC->c8107.list[0].alarm;
		fprintf(stderr, "购电控限制[%lld] [%lld]\n", val, warn);

		if (val >= 0) {
			fprintf(stderr, "购电判断值[%lld]\n", JProgramInfo->class23[i].remains);

			if (JProgramInfo->class23[i].remains <= val) {
				fprintf(stderr, "购电控跳闸！！！！！！！！！！！！！！！！！！\n", val);
				JProgramInfo->class23[i].alCtlState.OutputState = 192;
				JProgramInfo->class23[i].alCtlState.ECAlarmState = 0;
				JProgramInfo->class23[i].alCtlState.BuyOutputState = 192;
				return 2;
			}

			if (JProgramInfo->class23[i].remains <= warn) {
				fprintf(stderr, "购电控告警！！！！！！！！！！！！！！！！！！\n", val);
				JProgramInfo->class23[i].alCtlState.ECAlarmState = 64;
				JProgramInfo->class23[i].alCtlState.OutputState = 0;
				JProgramInfo->class23[i].alCtlState.BuyOutputState = 0;
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
			JProgramInfo->class23[i].alCtlState.OutputState = 0;
			JProgramInfo->class23[i].alCtlState.MonthOutputState = 0;
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
				JProgramInfo->class23[i].alCtlState.OutputState = 192;
				JProgramInfo->class23[i].alCtlState.MonthOutputState = 192;
				JProgramInfo->class23[i].alCtlState.ECAlarmState = 0;
				return 2;
			}

			if (JProgramInfo->class23[i].MonthPALL * 100 > e * val) {
				fprintf(stderr, "月电控告警！！！！！！！！！！！！！！！！！！\n", val);
				JProgramInfo->class23[i].alCtlState.ECAlarmState = 128;
				JProgramInfo->class23[i].alCtlState.OutputState = 0;
				JProgramInfo->class23[i].alCtlState.MonthOutputState = 0;
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

//统计输出与告警状态
//	sumUpCtrl();
//
//	//汇总所有总加组的状态
//	getFinalCtrl();
}

int ctrlMain(void * arg) {

	int secOld = 0;
	int ctrlflg = 0;
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

			//一分钟计算一次控制逻辑
			if (secOld % 5 == 0) {

				//检查参数更新
				//			CheckParaUpdate();

				//处理控制逻辑
				dealCtrl();
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
