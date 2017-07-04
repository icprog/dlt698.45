/*
 * class12.c
 *
 *  Created on: 2017-7-3
 *      Author: zhoulihai
 */
#include <stdio.h>

#include "class12.h"
#include "PublicFunction.h"
#include "dlt698.h"
#include "../include/StdDataType.h"

int class12_act3(int index, INT8U* data) {
	if (data[0] != 0x02 || data[1] != 0x03) {
		return 0;
	}
	if (data[2] != 0x51) {
		return 0;
	}

	ProgramInfo *shareAddr = getShareAddr();
	for (int i = 0; i < 12; i++) {
		if (shareAddr->class12.unit[i].no.OI == 0x00) {
			shareAddr->class12.unit[i].no.OI = data[3] * 256 + data[4];
			asyslog(LOG_INFO, "添加脉冲计量配置单元 %04x",
					shareAddr->class12.unit[i].no.OI);
			shareAddr->class12.unit[i].no.attflg = data[5];
			shareAddr->class12.unit[i].no.attrindex = data[6];

			shareAddr->class12.unit[i].conf = data[8];
			shareAddr->class12.unit[i].k = data[10] * 256 + data[11];
			break;
		}
	}

	return 0;
}

int class12_act4(int index, INT8U* data) {
	return 0;
}

int class12_router(int index, int attr_act, INT8U *data, Action_result *act_ret) {
	switch (attr_act) {
	case 3:
		class12_act3(index, data);
		break;
	case 4:
		class12_act4(index, data);
		break;
	}
	return 0;
}

int class2401_set_attr_2(int index, OAD oad, INT8U *data, INT8U *DAR) {
	ProgramInfo *shareAddr = getShareAddr();
	if (data[0] != 0x09) {
		return 0;
	}
	if (data[1] > 255) {
		return 0;
	}

	for (int i = 0; i < data[1]; i++) {
		shareAddr->class12.addr[i] = data[2 + i];
	}

	return 0;
}

int class2401_set_attr_3(int index, OAD oad, INT8U *data, INT8U *DAR) {
	if (data[0] != 0x02) {
		return 0;
	}
	if (data[1] != 0x02) {
		return 0;
	}

	if (data[2] != 0x12 || data[5] != 0x12) {
		return 0;
	}
	int pt = data[3] * 255 + data[4];
	int ct = data[6] * 255 + data[7];
	asyslog(LOG_WARNING, "修改互感器倍率%d-%d", pt, ct);
	ProgramInfo *shareAddr = getShareAddr();
	shareAddr->class12.pt = pt;
	shareAddr->class12.ct = ct;
	return 0;
}

int class2401_set(int index, OAD oad, INT8U *data, INT8U *DAR) {

	switch (oad.attflg) {
	case 2:
		class2401_set_attr_2(0, oad, data, DAR);
		break;
	case 3:
		class2401_set_attr_3(0, oad, data, DAR);
		break;
	}

	return 0;
}
