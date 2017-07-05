//
// Created by 周立海 on 2017/4/21.
//

#include <stdio.h>

#include "class23.h"
#include "dlt698.h"
#include "PublicFunction.h"

int class23_selector(int index, int attr_act, INT8U *data,
		Action_result *act_ret) {
	switch (attr_act) {
	case 1:
		class23_act1(index);
		break;
	case 3:
		class23_act3(index, data);
		break;
	}
	return 0;
}

int class23_act1(int index) {
	ProgramInfo *shareAddr = getShareAddr();
	asyslog(LOG_WARNING, "清除所有配置单元(%d)", index);
	memset(&shareAddr->class23[index], 0x00, sizeof(CLASS23));
	return 0;
}

int class23_act3(int index, INT8U* data) {
	AL_UNIT al_unit;
	if (data[0] != 0x02 || data[1] != 0x03 || data[2] != 0x55) {
		return 0;
	}

	int tsa_len = data[3];
	int data_index = 4;

	if (tsa_len > 17) {
		return 0;
	}
	al_unit.tsa.addr[0] = tsa_len + 1;
	al_unit.tsa.addr[1] = tsa_len - 1;

	for (int i = 0; i < tsa_len; ++i) {
		al_unit.tsa.addr[2 + i] = data[data_index];
		data_index++;
	}

	if (data[data_index] != 0x16 || data[data_index + 2] != 0x16) {
		return 0;
	}

	al_unit.al_flag = data[data_index + 1];
	al_unit.cal_flag = data[data_index + 3];

	asyslog(LOG_WARNING, "添加一个配置单元(%d)", index);
	ProgramInfo *shareAddr = getShareAddr();
	for (int i = 0; i < MAX_AL_UNIT; i++) {
		if (shareAddr->class23[index].allist[i].tsa.addr[0] != 0x00) {
			memcpy(&shareAddr->class23[index].allist[i], &al_unit,
					sizeof(AL_UNIT));
			break;
		}
	}

	return 0;
}

int class23_set(int index, OAD oad, INT8U *data, INT8U *DAR) {
	ProgramInfo *shareAddr = getShareAddr();
	asyslog(LOG_WARNING, "修改总加组属性(%d)", oad.attflg);

	switch (oad.attflg) {
	case 13:
		if (data[0] != 0x17) {
			return 0;
		}
		shareAddr->class23[index].aveCircle = data[1];
		break;
	case 14:
		if (data[0] != 0x04) {
			return 0;
		}
		shareAddr->class23[index].pConfig = data[1];
		break;
	case 15:
		if (data[0] != 0x04) {
			return 0;
		}
		shareAddr->class23[index].eConfig = data[1];
		break;
	}
	return 0;
}

int class23_get_17(OI_698 oi, INT8U *sourcebuf, INT8U *buf, int *len) {
	ProgramInfo *shareAddr = getShareAddr();
	int index = -1;
	for (int i = 0; i < 8; i++) {
		if (oi == 0x2301 + i) {
			index = i;
		}
	}
	if (index == -1) {
		return 0;
	}

	*len = 0;
	*len += create_struct(&buf[*len],7);
	*len += fill_double_long64(&buf[*len],shareAddr->class23[index].alCtlState.v);
	*len += fill_integer(&buf[*len],shareAddr->class23[index].alCtlState.Downc);
	*len += fill_bit_string(&buf[*len],8,shareAddr->class23[index].alCtlState.Downc);
	*len += fill_bit_string(&buf[*len],8,shareAddr->class23[index].alCtlState.OutputState);
	*len += fill_bit_string(&buf[*len],8,shareAddr->class23[index].alCtlState.MonthOutputState);
	*len += fill_bit_string(&buf[*len],8,shareAddr->class23[index].alCtlState.BuyOutputState);
	*len += fill_bit_string(&buf[*len],8,shareAddr->class23[index].alCtlState.PCAlarmState);
	*len += fill_bit_string(&buf[*len],8,shareAddr->class23[index].alCtlState.ECAlarmState);

	return 1;
}

int class23_get(OAD oad, INT8U *sourcebuf, INT8U *buf, int *len) {
	ProgramInfo *shareAddr = getShareAddr();
	asyslog(LOG_WARNING, "召唤总加组属性(%d)", oad.attflg);

	switch (oad.attflg) {
	case 17:
		return class23_get_17(oad.OI, sourcebuf, buf, len);

	}

}

