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
	static int old_month;

	TS ts;
	TSGet(&ts);
	int total = 0;

	for (int i = 0; i < 4; i++) {
		total += JProgramInfo->class23[0].DayP[i];
	}

	fprintf(stderr, "total = %d\n", total);

	JProgramInfo->class23[0].p = JProgramInfo->class12[0].p;
	JProgramInfo->class23[0].q = JProgramInfo->class12[0].q;

	for (int i = 0; i < 4; i++) {
		fprintf(stderr, "分项%d\n", JProgramInfo->class12[0].day_pos_p[i]);
		JProgramInfo->class23[0].DayP[i] =
				JProgramInfo->class12[0].day_pos_p[i];
		JProgramInfo->class23[0].DayQ[i] =
				JProgramInfo->class12[0].day_pos_q[i];
		JProgramInfo->class23[0].MonthP[i] =
				JProgramInfo->class12[0].mon_pos_p[i];
		JProgramInfo->class23[0].MonthQ[i] =
				JProgramInfo->class12[0].mon_pos_q[i];
	}

	fprintf(stderr, "总加组 功率%d 电量%lld %lld %lld %lld\n",
			JProgramInfo->class23[0].p, JProgramInfo->class23[0].DayP[0],
			JProgramInfo->class23[0].DayP[1], JProgramInfo->class23[0].DayP[2],
			JProgramInfo->class23[0].DayP[3]);

	if (ts.Hour == 0 && ts.Minute == 0 && ts.Sec == 0) {
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
INT8U initFreezeDataFormFile() {
	fprintf(stderr,
			"\n\n\ninitFreezeDataFormFileinitFreezeDataFormFileinitFreezeDataFormFileinitFreezeDataFormFile");
	INT8U ret = 0;

	INT8U meterIndex = 0;
	INT8U groupIndex = 0;
	OAD oad_m[2];
	oad_m[0].OI = 0x5004;
	oad_m[0].attflg = 2;
	oad_m[0].attrindex = 0;
	oad_m[1].OI = 0x5006;
	oad_m[1].attflg = 2;
	oad_m[1].attrindex = 0;
	OAD oad_r[2];
	oad_r[0].OI = 0x0010;
	oad_r[0].attflg = 0x02;
	oad_r[0].attrindex = 0;
	oad_r[1].OI = 0x0020;
	oad_r[1].attflg = 0x02;
	oad_r[1].attrindex = 0;

	for (groupIndex = 0; groupIndex < 8; groupIndex++) {
		for (meterIndex = 0; meterIndex < MAX_AL_UNIT; meterIndex++) {
			if (JProgramInfo->class23[groupIndex].allist[meterIndex].tsa.addr[0]
					== 0)
				break;
			CLASS_6001 meter;
			INT8U ret = get6001ObjByTSA(
					JProgramInfo->class23[groupIndex].allist[meterIndex].tsa,
					&meter);
			if (ret == 1) {
				INT8U resultbuf[256];
				TS tsnow;
				TSGet(&tsnow);
				INT8U dataIndex = 0;
				for (dataIndex = 0; dataIndex < 4; dataIndex++) {
					memset(resultbuf, 0, 256);
					INT16U datalen = GetOADData(oad_m[dataIndex / 2],
							oad_r[dataIndex % 2], tsnow, meter, resultbuf);
					if (datalen > 3) {
						if (resultbuf[3] == (MAXVAL_RATENUM + 1)) {
							INT8U databuf[25];
							memcpy(databuf, &resultbuf[4], 25);
							INT8U rateIndex = 0;
							for (rateIndex = 0; rateIndex < MAXVAL_RATENUM + 1;
									rateIndex++) {
								if (rateIndex * 5 == 0x06) {
									INT32U dianliang = (databuf[rateIndex * 5
											+ 1] << 24)
											+ (databuf[rateIndex * 5 + 2] << 16)
											+ (databuf[rateIndex * 5 + 3] << 8)
											+ databuf[rateIndex * 5 + 4];
									JProgramInfo->class23[groupIndex].allist[meterIndex].freeze[dataIndex][rateIndex] =
											dianliang;
									fprintf(stderr,
											"\n dataIndex = %d rateIndex = %d value = %d",
											dataIndex, rateIndex, dianliang);
								} else {
									break;
								}

							}
						}
					}
				}

			}

		}
	}

	return ret;
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
		readCoverClass(0x2301 + i, i, &JProgramInfo->class23[i],
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

//根据当前时间返回
int getCurrTimeValue(OI_698 oi, TS ts) {
	//检查当前总加组的开关是否打开
	for (int i = 0; i < MAX_AL_UNIT; i++) {
		if (CtrlC->c8103.enable[i].name == oi
				&& CtrlC->c8103.enable[i].state == 0) {
			return -1;
		}
	}

	//计算当前标准时段
	int offtime = (ts.Hour * 2 + (ts.Minute / 30)) / 6;

	//获取当前总加组，在当前时段下的配置参数
	for (int i = 0; i < MAX_AL_UNIT; i++) {
		if (CtrlC->c8103.list[i].index == oi) {
			//判断当前时段的开关是否打开
			if (getBit(CtrlC->c8103.sign, offtime) == 0x00) {
				return -1;
			}
			int index = CtrlC->c8103.numb;
			switch (index) {
			case 0:
				return CtrlC->c8103.list[i].v1.t1;
			case 1:
				return CtrlC->c8103.list[i].v2.t1;
			case 2:
				return CtrlC->c8103.list[i].v3.t1;
			default:
				return -1;
			}
		}
	}

	//没有找到总加组参数
	return -1;
}

//更新各种控制的告警和轮次跳闸的状态
void updateState(ALSTATE *warn, ALSTATE* op, OI_698 name) {
	//找到对应的总价组
	int index = -1;
	for (int i = 0; i < MAX_AL_UNIT; i++) {
		if (warn[i].name == name) {
			index = i;
			break;
		}
	}
	if (index == -1) {
		return;
	}

	//第一轮告警
	if (getBit(warn[index].state, 0) == 0x00) {
		setBit(warn[index].state, 0);
		return;
	}

	//第一轮跳闸
	if (getBit(op[index].state, 0) == 0x00) {
		setBit(op[index].state, 0);
		return;
	}

	//第二轮告警
	if (getBit(warn[index].state, 1) == 0x00) {
		setBit(warn[index].state, 1);
		return;
	}

	//第二轮跳闸
	if (getBit(op[index].state, 1) == 0x00) {
		setBit(op[index].state, 1);
		return;
	}
}

int deal8103() {

	TS ts;
	TSGet(&ts);
	static int step = 0;
	static int count = 0;

	fprintf(stderr, "deal8103(%lld)\n", CtrlC->c8103.list[0].v1.t1);
	for (int i = 0; i < 1; i++) {
//		if (!CheckAllUnitEmpty(JProgramInfo->class23[i].allist)) {
//			continue;
//		}

		fprintf(stderr, "8103 index = %d\n", i);

		if (JProgramInfo->ctrls.c8103.enable[i].state == 0) {
			step = 0;
			count = 0;
			return 0;
		}

		INT64U val = CtrlC->c8103.list[0].v1.t1;
		fprintf(stderr, "时段功控限值(%lld)\n", val);

//		if (getCurrTimeValue(0x2301 + i, ts) != -1) {
		if (1) {
			fprintf(stderr, "进入时段功控时间，判断功率%lld\n", JProgramInfo->class23[i].p);
			if (val < JProgramInfo->class23[i].p) {
				switch (step) {
				case 0:
					JProgramInfo->class23[i].alCtlState.OutputState = 0;
					JProgramInfo->class23[i].alCtlState.PCAlarmState = 32;
					fprintf(stderr, "时段功控，一轮告警！！！！！！！！！！！！！");
					fprintf(stderr, "功控告警时间 %d\n", CtrlC->c8102.time[0] * 60);
					if (count * 5 > (CtrlC->c8102.time[0]) * 60) {
						JProgramInfo->class23[i].alCtlState.OutputState = 128;
						JProgramInfo->class23[i].alCtlState.PCAlarmState = 0;
						step = 1;
						count = 0;
						fprintf(stderr, "时段功控，一轮跳闸！！！！！！！！！！！！！");
					}
					count += 1;
					break;
				case 1:
					JProgramInfo->class23[i].alCtlState.OutputState = 0;
					JProgramInfo->class23[i].alCtlState.PCAlarmState = 16;

					fprintf(stderr, "时段功控，二轮告警！！！！！！！！！！！！！");
					if (count * 5 > (CtrlC->c8102.time[1]) * 60) {
						JProgramInfo->class23[i].alCtlState.OutputState = 192;
						JProgramInfo->class23[i].alCtlState.PCAlarmState = 0;
						step = 2;
						count = 0;
						fprintf(stderr, "时段功控，二轮跳闸！！！！！！！！！！！！！");
					}
					count += 1;
					break;
				case 2:
					break;
				}
			} else {
				step = 0;
				count = 0;
			}
		}

	}
	return 0;
}

int getIsInTime(OI_698 oi, TS ts) {

	//检查当前总加组的开关是否打开
	for (int i = 0; i < MAX_AL_UNIT; i++) {
		if (CtrlC->c8104.enable[i].name == oi
				&& CtrlC->c8104.enable[i].state == 0) {
			return -1;
		}
	}

	//计算当前时间是否在厂休时段范围内
	TS start;
	TSGet(&start);

	for (int i = 0; i < MAX_AL_UNIT; i++) {
		if (CtrlC->c8104.list[i].index == oi) {
			//判断是否是限电日
			if (getBit(CtrlC->c8104.list[i].noDay, ts.Week) == 0) {
				return -1;
			}

			//判断是否在厂休时间内，并返回定值
			if (CtrlC->c8104.list[i].start.year.data != 0xFFFF) {
				TimeBCDToTs(CtrlC->c8104.list[i].start, &start);
				tminc(&start, 1, CtrlC->c8104.list[i].sustain);
				if (TScompare(start, ts) == 1) {
					return CtrlC->c8104.list[i].v;
				} else {
					return -1;
				}
			}
		}
	}
}

int deal8104() {

	TS ts;
	TSGet(&ts);
	static int step = 0;
	static int count = 0;

	fprintf(stderr, "deal8105(%lld)\n", CtrlC->c8104.list[0].v);
	for (int i = 0; i < 1; i++) {
//		if (!CheckAllUnitEmpty(JProgramInfo->class23[i].allist)) {
//			continue;
//		}

		fprintf(stderr, "8104 index = %d\n", i);

		if (JProgramInfo->ctrls.c8104.enable[i].state == 0) {
			step = 0;
			count = 0;
			return 0;
		}

		INT64U val = CtrlC->c8104.list[0].v;
		fprintf(stderr, "厂休控限值(%lld)\n", val);

//		if (getIsInTime(0x2301 + i, ts)) {
		if (1) {
			fprintf(stderr, "进入厂休控时间，判断功率%lld\n", JProgramInfo->class23[i].p);
			if (val < JProgramInfo->class23[i].p) {
				switch (step) {
				case 0:
					JProgramInfo->class23[i].alCtlState.OutputState = 0;
					JProgramInfo->class23[i].alCtlState.PCAlarmState = 32;
					fprintf(stderr, "厂休控，一轮告警！！！！！！！！！！！！！");
					fprintf(stderr, "功控告警时间 %d\n", CtrlC->c8102.time[0] * 60);
					if (count * 5 > (CtrlC->c8102.time[0]) * 60) {
						JProgramInfo->class23[i].alCtlState.OutputState = 128;
						JProgramInfo->class23[i].alCtlState.PCAlarmState = 0;
						step = 1;
						count = 0;
						fprintf(stderr, "厂休控，一轮跳闸！！！！！！！！！！！！！");
					}
					count += 1;
					break;
				case 1:
					JProgramInfo->class23[i].alCtlState.OutputState = 0;
					JProgramInfo->class23[i].alCtlState.PCAlarmState = 16;

					fprintf(stderr, "厂休控，二轮告警！！！！！！！！！！！！！");
					if (count * 5 > (CtrlC->c8102.time[1]) * 60) {
						JProgramInfo->class23[i].alCtlState.OutputState = 192;
						JProgramInfo->class23[i].alCtlState.PCAlarmState = 0;
						step = 2;
						count = 0;
						fprintf(stderr, "厂休控，二轮跳闸！！！！！！！！！！！！！");
					}
					count += 1;
					break;
				case 2:
					break;
				}
			} else {
				step = 0;
				count = 0;
			}
		}

	}
	return 0;
}

long long getIsInStop(OI_698 oi, TS ts) {

	//检查当前总加组的开关是否打开
	for (int i = 0; i < MAX_AL_UNIT; i++) {
		if (CtrlC->c8104.enable[i].name == oi
				&& CtrlC->c8105.enable[i].state == 0) {
			return -1;
		}
	}

	TS start, end;
	for (int i = 0; i < MAX_AL_UNIT; i++) {
		if (CtrlC->c8105.list[i].index == oi) {
			//判断当前时间是否在营业报停配置时段内
			TimeBCDToTs(CtrlC->c8105.list[i].start, &start);
			TimeBCDToTs(CtrlC->c8105.list[i].end, &end);
			fprintf(stderr, "营业报停找配置单元(%d)\n", i);

			if (TScompare(ts, start) == 1 && TScompare(end, ts) == 1) {
				return CtrlC->c8105.list[i].v;
			} else {
				return -1;
			}
		}
	}
}

int deal8105() {
	TS ts;
	TSGet(&ts);
	static int step = 0;
	static int count = 0;

	fprintf(stderr, "deal8105(%lld)\n", CtrlC->c8105.list[0].v);
	for (int i = 0; i < 1; i++) {
		if (!CheckAllUnitEmpty(JProgramInfo->class23[i].allist)) {
			continue;
		}

		fprintf(stderr, "8105 index = %d\n", i);

		if (JProgramInfo->ctrls.c8105.enable[i].state == 0) {
			step = 0;
			count = 0;
			return 0;
		}

		INT64U val = CtrlC->c8105.list[0].v;
		fprintf(stderr, "营业报停限值(%lld)\n", val);

		if (TScompare(ts, CtrlC->c8105.list[0].start) == 1
				&& TScompare(ts, CtrlC->c8105.list[0].end) == 2) {
//		if (1) {
			fprintf(stderr, "进入营业报停时间，判断功率%lld\n", JProgramInfo->class23[i].p);
			if (val <= JProgramInfo->class23[i].p) {
				switch (step) {
				case 0:
					JProgramInfo->class23[i].alCtlState.OutputState = 0;
					JProgramInfo->class23[i].alCtlState.PCAlarmState = 32;
					fprintf(stderr, "营业报停控，一轮告警！！！！！！！！！！！！！");
					fprintf(stderr, "功控告警时间 %d\n", CtrlC->c8102.time[0] * 60);
					if (count * 5 > (CtrlC->c8102.time[0]) * 60) {
						JProgramInfo->class23[i].alCtlState.OutputState = 128;
						JProgramInfo->class23[i].alCtlState.PCAlarmState = 0;
						step = 1;
					}
					count += 1;
					break;
				case 1:
					JProgramInfo->class23[i].alCtlState.OutputState = 128;
					JProgramInfo->class23[i].alCtlState.PCAlarmState = 0;
					count = 0;
					fprintf(stderr, "营业报停控，一轮跳闸！！！！！！！！！！！！！");
					step = 2;
					break;
				case 2:
					JProgramInfo->class23[i].alCtlState.OutputState = 0;
					JProgramInfo->class23[i].alCtlState.PCAlarmState = 16;
					fprintf(stderr, "营业报停控，二轮告警！！！！！！！！！！！！！");
					fprintf(stderr, "功控告警时间 %d\n", CtrlC->c8102.time[1] * 60);
					if (count * 5 > (CtrlC->c8102.time[1]) * 60) {
						JProgramInfo->class23[i].alCtlState.OutputState = 192;
						JProgramInfo->class23[i].alCtlState.PCAlarmState = 0;
						step = 3;
					}
					count += 1;
					break;
				case 3:
					JProgramInfo->class23[i].alCtlState.OutputState = 192;
					JProgramInfo->class23[i].alCtlState.PCAlarmState = 0;
					count = 0;
					fprintf(stderr, "营业报停控，二轮跳闸！！！！！！！！！！！！！");
					break;
				}
			} else {
				step = 0;
				count = 0;
			}
		}

	}
	return 0;
}

int deal8106() {

	//购电控不受购电配置单元的影响，购电配置单元作为总加组剩余电量刷新的依据
	fprintf(stderr, "deal8106(%d)\n", CtrlC->c8106.list[0].down_freeze);
	for (int i = 0; i < 1; i++) {
		if (!CheckAllUnitEmpty(JProgramInfo->class23[i].allist)) {
			continue;
		}
		fprintf(stderr, "8106 index = %d\n", i);

		if (JProgramInfo->ctrls.c8106.enable[i].state == 0) {
			return 0;
		}
	}
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

INT64U getMonthValue(int i) {
	return CtrlC->c8108.list[i].v;
}

INT8U getMonthWarn(int i) {
	return CtrlC->c8108.list[i].para;
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

		INT64U val = getMonthValue(i);
		INT8U warn = getMonthWarn(i);
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

//汇总总加组状态，产生每个总加组独立的控制结果
void sumUpCtrl() {
	for (int i = 0; i < 8; i++) {
		//时段功控状态汇总
		for (int i = 0; i < MAX_AL_UNIT; i++) {
			if (CtrlC->c8103.overflow[i].name == 0x2301 + i) {
				JProgramInfo->class23[i].alCtlState.PCAlarmState |=
						CtrlC->c8103.overflow[i].state;
			}

			if (CtrlC->c8103.output[i].name == 0x2301 + i) {
				JProgramInfo->class23[i].alCtlState.OutputState |=
						CtrlC->c8103.output[i].state;
			}
		}

		//厂休控状态汇总
		for (int i = 0; i < MAX_AL_UNIT; i++) {
			if (CtrlC->c8104.overflow[i].name == 0x2301 + i) {
				JProgramInfo->class23[i].alCtlState.PCAlarmState |=
						CtrlC->c8104.overflow[i].state;
			}

			if (CtrlC->c8104.output[i].name == 0x2301 + i) {
				JProgramInfo->class23[i].alCtlState.OutputState |=
						CtrlC->c8104.output[i].state;
			}
		}

		//营业报停控状态汇总
		for (int i = 0; i < MAX_AL_UNIT; i++) {
			if (CtrlC->c8105.overflow[i].name == 0x2301 + i) {
				JProgramInfo->class23[i].alCtlState.PCAlarmState |=
						CtrlC->c8105.overflow[i].state;
			}

			if (CtrlC->c8105.output[i].name == 0x2301 + i) {
				JProgramInfo->class23[i].alCtlState.OutputState |=
						CtrlC->c8105.output[i].state;
			}
		}

		//功率下浮控状态汇总
		for (int i = 0; i < MAX_AL_UNIT; i++) {
			if (CtrlC->c8106.overflow[i].name == 0x2301 + i) {
				JProgramInfo->class23[i].alCtlState.PCAlarmState |=
						CtrlC->c8106.overflow[i].state;
			}

			if (CtrlC->c8106.output[i].name == 0x2301 + i) {
				JProgramInfo->class23[i].alCtlState.OutputState |=
						CtrlC->c8106.output[i].state;
			}
		}

		//购电控状态汇总
		for (int i = 0; i < MAX_AL_UNIT; i++) {
			if (CtrlC->c8107.overflow[i].name == 0x2301 + i) {
				JProgramInfo->class23[i].alCtlState.ECAlarmState |=
						CtrlC->c8107.overflow[i].state;
			}

			if (CtrlC->c8107.output[i].name == 0x2301 + i) {
				JProgramInfo->class23[i].alCtlState.BuyOutputState |=
						CtrlC->c8107.output[i].state;
				JProgramInfo->class23[i].alCtlState.OutputState |=
						CtrlC->c8107.output[i].state;
			}
		}

		//月电控状态汇总
		for (int i = 0; i < MAX_AL_UNIT; i++) {
			if (CtrlC->c8108.overflow[i].name == 0x2301 + i) {
				JProgramInfo->class23[i].alCtlState.ECAlarmState |=
						CtrlC->c8108.overflow[i].state;
			}

			if (CtrlC->c8108.output[i].name == 0x2301 + i) {
				JProgramInfo->class23[i].alCtlState.MonthOutputState |=
						CtrlC->c8108.output[i].state;
				JProgramInfo->class23[i].alCtlState.OutputState |=
						CtrlC->c8108.output[i].state;
			}
		}
	}
}

static struct {
	INT8U alarm;
	INT8U output1;
	INT8U output2;
	INT8U ECs;
	INT8U PCs;
	INT8U light1;
	INT8U light2;
} finalCtrl;

void getFinalCtrl() {
	for (int i = 0; i < 8; i++) {
		if (CheckAllUnitEmpty(JProgramInfo->class23[i].allist)) {

			//更新告警状态
			finalCtrl.alarm |= getBit(
					JProgramInfo->class23[i].alCtlState.ECAlarmState, 0);
			finalCtrl.alarm |= getBit(
					JProgramInfo->class23[i].alCtlState.PCAlarmState, 0);
			finalCtrl.alarm |= getBit(
					JProgramInfo->class23[i].alCtlState.ECAlarmState, 1);
			finalCtrl.alarm |= getBit(
					JProgramInfo->class23[i].alCtlState.PCAlarmState, 1);

			//更新一轮跳闸状态
			finalCtrl.output1 |= getBit(
					JProgramInfo->class23[i].alCtlState.OutputState, 0);
			finalCtrl.output1 |= getBit(
					JProgramInfo->class23[i].alCtlState.OutputState, 1);

			//更新二轮跳闸状态
			finalCtrl.output2 |= getBit(
					JProgramInfo->class23[i].alCtlState.OutputState, 0);
			finalCtrl.output2 |= getBit(
					JProgramInfo->class23[i].alCtlState.OutputState, 1);

			//更新功控指示灯
			finalCtrl.ECs += JProgramInfo->class23[i].alCtlState.PCAlarmState;

			//更新电控指示灯
			finalCtrl.ECs += JProgramInfo->class23[i].alCtlState.ECAlarmState;
		}
	}

	//正规化参数
	finalCtrl.ECs = (finalCtrl.ECs == 0) ? 0x00 : 0x01;
	finalCtrl.PCs = (finalCtrl.PCs == 0) ? 0x00 : 0x01;

	finalCtrl.light1 = (finalCtrl.output1 == 0) ? 0x00 : 0x01;
	finalCtrl.light2 = (finalCtrl.output2 == 0) ? 0x00 : 0x01;

	//做出控制
	int fd = OpenSerialPort();
	SetCtrl_CMD(fd, finalCtrl.output1, finalCtrl.light1, ~finalCtrl.light1,
			finalCtrl.output2, finalCtrl.light2, ~finalCtrl.light2,
			finalCtrl.PCs, finalCtrl.ECs, finalCtrl.alarm, 1);
	fclose(fd);

}

void dealCtrl() {
	//直接跳闸，必须检测
	deal8107();
	deal8108();

	//检测控制有优先级，当高优先级条件产生时，忽略低优先级的配置
//	JProgramInfo->class23[0].p = 100;
//	JProgramInfo->ctrls.c8105.enable[0].state = 1;
//	CtrlC->c8102.time[0] = 1;
//	CtrlC->c8102.time[1] = 1;

	deal8105();
//	if (deal8106() != 0) {
//		;
//	} else if (deal8105() != 0) {
//		;
//	}
//	} else if (deal8104() != 0) {
//		;
//	} else if (deal8103() != 0) {
//		;
//	}
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
	initFreezeDataFormFile();
	while (1) {
		TS now;
		TSGet(&now);

		//一秒钟刷新一次脉冲数据
		if (secOld != now.Sec) {
			fprintf(stderr, "ctrlMain in!!!!++++%lld   %lld+++++++++\n", JProgramInfo->class12[0].ct, JProgramInfo->class12[0].pt);

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
		secOld = now.Sec;
		usleep(200 * 1000);
	}
}
