//
// Created by 周立海 on 2017/4/24.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ctrl.h"
#include "crtl_base.h"
#include "../include/Shmem.h"
#include "../include/Objectdef.h"
#include "../libAccess/AccessFun.h"

extern ProgramInfo* JProgramInfo;

static struct {
	CLASS23 class23[8];

	CLASS_8100 c8100; //终端保安定值
	CLASS_8101 c8101; //终端功控时段
	CLASS_8102 c8102; //功控告警时间

	CLASS_8103 c8103; //时段功控
	CLASS_8104 c8104; //厂休控
	CLASS_8105 c8105; //营业报停控
	CLASS_8106 c8106; //功率下浮控

	CLASS_8107 c8107; //购电控
	CLASS_8108 c8108; //月电控
} CtrlC;

int ctrl_base_test() {
	printf("%d", CheckModelState());
	InitCtrlModel();
	int fd = OpenSerialPort();

	SetCtrl_CMD(fd, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);

	return 0;
}

void refreshSumUp() {

}

void CheckParaUpdate() {

}

int initAll() {
	//读取总加组数据
	memset(&CtrlC, 0x00, sizeof(CtrlC));
	for (int i = 0; i < 8; ++i) {
		readCoverClass(0x2301 + i, 0, &JProgramInfo->class23[0],
				sizeof(CLASS23), para_vari_save);
	}

	readCoverClass(0x8100, 0, &CtrlC.c8100, sizeof(CLASS_8100), para_vari_save);
	readCoverClass(0x8101, 0, &CtrlC.c8101, sizeof(CLASS_8101), para_vari_save);
	readCoverClass(0x8102, 0, &CtrlC.c8102, sizeof(CLASS_8102), para_vari_save);
	readCoverClass(0x8103, 0, &CtrlC.c8103, sizeof(CLASS_8103), para_vari_save);
	readCoverClass(0x8104, 0, &CtrlC.c8104, sizeof(CLASS_8104), para_vari_save);
	readCoverClass(0x8105, 0, &CtrlC.c8105, sizeof(CLASS_8105), para_vari_save);
	readCoverClass(0x8106, 0, &CtrlC.c8106, sizeof(CLASS_8106), para_vari_save);
	readCoverClass(0x8107, 0, &CtrlC.c8107, sizeof(CLASS_8107), para_vari_save);
	readCoverClass(0x8108, 0, &CtrlC.c8108, sizeof(CLASS_8108), para_vari_save);

	return 0;
}

int getBit(INT8U v, int index) {
	return (v >> (index)) & 0x01;
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
		if (CtrlC.c8103.enable[i].name == oi
				&& CtrlC.c8103.enable[i].state == 0) {
			return -1;
		}
	}

	//计算当前标准时段
	int offtime = (ts.Hour * 2 + (ts.Minute / 30)) / 6;

	//获取当前总加组，在当前时段下的配置参数
	for (int i = 0; i < MAX_AL_UNIT; i++) {
		if (CtrlC.c8103.list[i].index == oi) {
			//判断当前时段的开关是否打开
			if (getBit(CtrlC.c8103.sign, offtime) == 0x00) {
				return -1;
			}
			int index = CtrlC.c8103.numb;
			switch (index) {
			case 0:
				return CtrlC.c8103.list[i].v1.t1;
			case 1:
				return CtrlC.c8103.list[i].v2.t1;
			case 2:
				return CtrlC.c8103.list[i].v3.t1;
			default:
				return -1;
			}
		}
	}

	//没有找到总加组参数
	return -1;
}

int deal8103() {
	TS ts;
	TSGet(&ts);
	for (int i = 0; i < 2; i++) {
		int val = 0;
		if (CheckAllUnitEmpty(JProgramInfo->class23[i].allist)) {
			if (getCurrTimeValue(0x2301 + i, ts) != -1) {
				if (JProgramInfo->class23[i].DayP[0] > val) {
					//产生约负荷越限
					return 1;
				} else {
					//清除告警状态
					return 0;
				}
			}
		}
	}
	return 0;
}

int getIsInTime(OI_698 oi, TS ts) {

	//检查当前总加组的开关是否打开
	for (int i = 0; i < MAX_AL_UNIT; i++) {
		if (CtrlC.c8104.enable[i].name == oi
				&& CtrlC.c8104.enable[i].state == 0) {
			return -1;
		}
	}

	//计算当前时间是否在厂休时段范围内
	TS start;
	TSGet(&start);

	for (int i = 0; i < MAX_AL_UNIT; i++) {
		if (CtrlC.c8104.list[i].index == oi) {
			//判断是否是限电日
			if (getBit(CtrlC.c8104.list[i].noDay, ts.Week) == 0) {
				return -1;
			}

			//判断是否在厂休时间内，并返回定值
			if (CtrlC.c8104.list[i].start.year.data != 0xFFFF) {
				TimeBCDToTs(CtrlC.c8104.list[i].start, &start);
				tminc(&start, 1, CtrlC.c8104.list[i].sustain);
				if (TScompare(start, ts) == 1) {
					return CtrlC.c8104.list[i].v;
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
	for (int i = 0; i < 2; i++) {
		int val = 0;
		if (CheckAllUnitEmpty(JProgramInfo->class23[i].allist)) {
			val = getIsInTime(0x2301 + i, ts);
		}

		if (val != -1) {
			if (JProgramInfo->class23[i].DayP[0] > val) {
				//产生约负荷越限
				return 1;
			} else {
				//清除告警状态
				return 0;
			}
		}
	}
	return 0;
}

int getIsInStop(OI_698 oi, TS ts) {

	//检查当前总加组的开关是否打开
	for (int i = 0; i < MAX_AL_UNIT; i++) {
		if (CtrlC.c8104.enable[i].name == oi
				&& CtrlC.c8105.enable[i].state == 0) {
			return -1;
		}
	}

	TS start, end;
	for (int i = 0; i < MAX_AL_UNIT; i++) {
		if (CtrlC.c8105.list[i].index == oi) {
			//判断当前时间是否在营业报停配置时段内
			TimeBCDToTs(CtrlC.c8105.list[i].start, &start);
			TimeBCDToTs(CtrlC.c8105.list[i].end, &end);

			if (TScompare(ts, start) == 1 && TScompare(end, ts) == 1) {
				return CtrlC.c8105.list[i].v;
			} else {
				return -1;
			}
		}
	}
}

int deal8105() {
	TS ts;
	TSGet(&ts);
	for (int i = 0; i < 2; i++) {
		int val = 0;
		if (CheckAllUnitEmpty(JProgramInfo->class23[i].allist)) {
			val = getIsInStop(0x2301 + i, ts);
		}

		if (val != -1) {
			if (JProgramInfo->class23[i].DayP[0] > val) {
				//产生约负荷越限
				return 1;
			} else {
				//清除告警状态
				return 0;
			}
		}
	}
	return 0;
}

int deal8106() {
	return 0;
}

int deal8107() {
	//购电控不受购电配置单元的影响，购电配置单元作为总加组剩余电量刷新的依据

	for (int i = 0; i < 2; i++) {
		if (JProgramInfo->class23[i].remains <= 0) {
			return 1;
		}
	}
	return 0;
}

int getMonthValue(OI_698 oi) {
	for (int i = 0; i < MAX_AL_UNIT; i++) {
		if (CtrlC.c8108.list[i].index == oi) {
			return CtrlC.c8108.list[i].v;
		}
	}
	return -1;
}

int deal8108() {
	for (int i = 0; i < 2; i++) {
		int val = 0;
		if (CheckAllUnitEmpty(JProgramInfo->class23[i].allist)) {
			val = getMonthValue(0x2301 + i);
		}

		if (val != -1) {
			if (JProgramInfo->class23[i].MonthP[0] > val) {
				return 1;
			}
		}
	}
	return 0;
}

void dealControl() {
	//直接跳闸，必须检测
	deal8107();
	deal8108();

	//检测控制有优先级，当高优先级条件产生时，忽略低优先级的配置

	if (deal8106() != 0) {
		return;
	}
	if (deal8105() != 0) {
		return;
	}
	if (deal8104() != 0) {
		return;
	}
	if (deal8103() != 0) {
		return;
	}

}

int ctrlMain() {

	//初始化参数,搭建8个总加组数据，读取功控、电控参数
	initAll();

	while (1) {
		//更新总加组数据
		refreshSumUp();

		//检查参数更新
		CheckParaUpdate();

		//处理控制逻辑
		dealControl();
	}
}
