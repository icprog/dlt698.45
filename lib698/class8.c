//
// Created by 周立海 on 2017/4/21.
//
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <math.h>

#include "AccessFun.h"
#include "class8.h"
#include "PublicFunction.h"
#include "event.h"
#include "dlt698.h"

typedef union { //control code
	INT8U u8b; //convenient to set value to 0
	struct { //only for little endian mathine!
		INT8U bak :4; //备用
		INT8U power_ctl :1; //当前功率下浮控
		INT8U business_ctl :1; //营业报停控
		INT8U work_ctl :1; //厂休控
		INT8U time_ctl :1; //时段控
	} pcstate;
} PCAlarmState;

typedef union { //control code
	INT8U u8b; //convenient to set value to 0
	struct { //only for little endian mathine!
		INT8U bak :6; //备用
		INT8U buy_elec_ctl :1; 	//购电控
		INT8U month_elec_ctl :1; //月电控
	} ecstate;
} ECAlarmState;

ECAlarmState ecAlarm;
PCAlarmState pcAlarm;

int class8000_act129(int index, int attr_act, INT8U *data,
		Action_result *act_ret) {

	ProgramInfo *shareAddr = getShareAddr();
	if (shareAddr->ctrls.c8001.state == 1)
	{
		shareAddr->ctrls.cf205.currentState = 0;
	}else
	{
		shareAddr->ctrls.cf205.currentState = 1;
	}

	if(shareAddr->ctrls.cf205.currentState == 0){
		return 0;
	}

	if(data[0] != 0x01 || data[1] != 0x01){
		return -1;
	}
	if(data[2] != 0x02 || data[3] != 0x04){
		return -1;
	}

	OI_698 oi = data[5] * 256 + data[6];
	int delay = data[10];
	int limit_time = data[12] * 256 + data[13];
	int auto_act = data[15];

	asyslog(LOG_WARNING, "遥控跳闸 %d - %d - %d\n", delay, limit_time, auto_act);

	shareAddr->ctrls.control[0] = 0xEEFFEFEF;
	shareAddr->ctrls.control[1] = 0xEEFFEFEF;
	shareAddr->ctrls.control[2] = 0xEEFFEFEF;

	shareAddr->ctrls.control_event = 1;
	return 0;
}

int class8000_act130(int index, int attr_act, INT8U *data,
		Action_result *act_ret) {

	ProgramInfo *shareAddr = getShareAddr();

	shareAddr->ctrls.cf205.currentState = 0;

	if(shareAddr->ctrls.cf205.currentState == 0){
		return 0;
	}

	if(data[0] != 0x01 || data[1] != 0x01){
		return -1;
	}
	if(data[2] != 0x02 || data[3] != 0x04){
		return -1;
	}

	OI_698 oi = data[5] * 256 + data[6];
	int allow_or_auto = data[10];

	asyslog(LOG_WARNING, "遥控合闸 %d\n", allow_or_auto);

	shareAddr->ctrls.control[0] = 0xCCAACACA;
	shareAddr->ctrls.control[1] = 0xCCAACACA;
	shareAddr->ctrls.control[2] = 0xCCAACACA;

	return 0;
}

int class8000_act_route(int index, int attr_act, INT8U *data,
		Action_result *act_ret) {
	switch (attr_act) {
	case 127:
//		class8000_act127(1, attr_act, data, act_ret);
		break;
	case 128:
//		class8000_act128(1, attr_act, data, act_ret);
		break;
	case 129:
		class8000_act129(1, attr_act, data, act_ret);
		break;
	case 130:
		class8000_act130(1, attr_act, data, act_ret);
		break;
	}
	return 1;
}

int class8001_act127(int index, int attr_act, INT8U *data,
		Action_result *act_ret) {
	asyslog(LOG_WARNING, "投入保电\n");
	CLASS_8001 c8001;
	memset(&c8001, 0x00, sizeof(CLASS_8001));
//	readCoverClass(0x8001, 0, (void *) &c8001, sizeof(CLASS_8001),
//			para_vari_save);
	ProgramInfo *shareAddr = getShareAddr();
	memcpy(&c8001,&shareAddr->ctrls.c8001,sizeof(CLASS_8001));
	c8001.state = 1;
	shareAddr->ctrls.c8001.state = c8001.state;
	saveCoverClass(0x8001, 0, (void *) &c8001, sizeof(CLASS_8001),
			para_vari_save);
	return 0;
}

int class8001_act128(int index, int attr_act, INT8U *data,
		Action_result *act_ret) {
	asyslog(LOG_WARNING, "解除保电\n");
	CLASS_8001 c8001;
	ProgramInfo *shareAddr = getShareAddr();

	memset(&c8001, 0x00, sizeof(CLASS_8001));
//	readCoverClass(0x8001, 0, (void *) &c8001, sizeof(CLASS_8001),
//			para_vari_save);
	memcpy(&c8001,&shareAddr->ctrls.c8001,sizeof(CLASS_8001));
	c8001.state = 0;
	shareAddr->ctrls.c8001.state = c8001.state;
	saveCoverClass(0x8001, 0, (void *) &c8001, sizeof(CLASS_8001),
			para_vari_save);
	return 0;
}

int class8001_act129(int index, int attr_act, INT8U *data,
		Action_result *act_ret) {
	asyslog(LOG_WARNING, "解除自动保电\n");
	CLASS_8001 c8001={};
	ProgramInfo *shareAddr = getShareAddr();

	memset(&c8001, 0x00, sizeof(CLASS_8001));
//	readCoverClass(0x8001, 0, (void *) &c8001, sizeof(CLASS_8001),
//			para_vari_save);
	memcpy(&c8001,&shareAddr->ctrls.c8001,sizeof(CLASS_8001));
	c8001.state = 2;
	shareAddr->ctrls.c8001.state = c8001.state;
	saveCoverClass(0x8001, 0, (void *) &c8001, sizeof(CLASS_8001),
			para_vari_save);
	return 0;
}

int class8001_act_route(int index, int attr_act, INT8U *data,
		Action_result *act_ret) {
	switch (attr_act) {
	case 127:
		class8001_act127(1, attr_act, data, act_ret);
		break;
	case 128:
		class8001_act128(1, attr_act, data, act_ret);
		break;
	case 129:
		class8001_act129(1, attr_act, data, act_ret);
		break;
	}
	return 1;
}

int class8002_act127(int index, int attr_act, INT8U *data,
		Action_result *act_ret) {
	asyslog(LOG_WARNING, "催费告警投入\n");
	CLASS_8002 c8002;
	ProgramInfo *shareAddr = getShareAddr();

//	readCoverClass(0x8002, 0, (void *) &c8002, sizeof(CLASS_8002),
//			para_vari_save);
	memcpy(&c8002,&shareAddr->ctrls.c8002,sizeof(CLASS_8002));
	c8002.state = 1;
	shareAddr->ctrls.c8002.state = c8002.state;
	saveCoverClass(0x8002, 0, (void *) &c8002, sizeof(CLASS_8002),
			para_vari_save);
	return 0;
}

int class8002_act128(int index, int attr_act, INT8U *data,
		Action_result *act_ret) {
	asyslog(LOG_WARNING, "催费告警退出\n");
	CLASS_8002 c8002;
	ProgramInfo *shareAddr = getShareAddr();
//	readCoverClass(0x8002, 0, (void *) &c8002, sizeof(CLASS_8002),
//			para_vari_save);
	memcpy(&c8002,&shareAddr->ctrls.c8002,sizeof(CLASS_8002));
	c8002.state = 0;
	shareAddr->ctrls.c8002.state = c8002.state;
	saveCoverClass(0x8002, 0, (void *) &c8002, sizeof(CLASS_8002),
			para_vari_save);
	return 0;
}

int class8002_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret) {
	switch (attr_act) {
	case 127:
		class8002_act127(1, attr_act, data, act_ret);
		break;
	case 128:
		class8002_act128(1, attr_act, data, act_ret);
		break;
	}
	return 1;
}


INT64U getLongValue(INT8U *data) {
	INT64U v = 0x00;
	for (int i = 0; i < 8; ++i) {
		v += data[i + 1];
		if (i + 1 == 8) {
			break;
		}
		v = v << 8;
	}
	return v;
}

/*
 * 遥控
 * */
int class8000_set(OAD oad, INT8U *data, INT8U *DAR)
{
	CLASS_8000 c8000={};
	INT8U	index = 0;
	ProgramInfo *shareAddr = getShareAddr();

	memcpy(&c8000,&shareAddr->ctrls.c8000,sizeof(CLASS_8000));
	if(oad.attflg == 2) {	//配置参数
		index += getStructure(&data[index],NULL,DAR);
		index += getDouble(&data[index],(INT8U *)&c8000.limit);
		index += getLongUnsigned(&data[index],(INT8U *)&c8000.delaytime);
	}
	if(*DAR == success) {
		memcpy(&shareAddr->ctrls.c8000,&c8000,sizeof(CLASS_8000));
		*DAR = saveCoverClass(oad.OI, 0, (void *) &c8000, sizeof(CLASS_8000),para_vari_save);
	}
	return index;
}

/*
 * 保电设置
 * */
int class8001_set(OAD oad, INT8U *data, INT8U *DAR) {
	CLASS_8001 c8001={};
	INT8U	i=0;
	INT8U	index = 0;
	ProgramInfo *shareAddr = getShareAddr();

	memset(&c8001,0,sizeof(CLASS_8001));
//	readCoverClass(0x8001, 0, (void *) &c8001, sizeof(CLASS_8001),
//					para_vari_save);
	memcpy(&c8001,&shareAddr->ctrls.c8001,sizeof(CLASS_8001));
	switch(oad.attflg) {
	case 2:	//保电状态，只读
		index += getEnum(1,data,&c8001.state);
		if(index==0)  *DAR = type_mismatch;
		break;
	case 3:
		index += getLongUnsigned(data,(INT8U *)&c8001.noCommTime);
		if(index==0)  *DAR = type_mismatch;
		else asyslog(LOG_WARNING, "设置保电属性3(%d)", c8001.noCommTime);
		break;
	case 4:
		index += getLongUnsigned(data,(INT8U *)&c8001.autoTime);
		if(index==0)  *DAR = type_mismatch;
		else asyslog(LOG_WARNING, "设置保电属性4(%d)", c8001.autoTime);
		break;
	case 5:
		index += getArray(&data[index],&c8001.unit_count,DAR);
		fprintf(stderr,"unit_count = %d\n",c8001.unit_count);
		for(i=0;i<c8001.unit_count;i++) {
			index += getStructure(&data[index],NULL,DAR);
			index += getUnsigned(&data[index],&c8001.unit[i].autoTimeStart,DAR);
			index += getUnsigned(&data[index],&c8001.unit[i].autoTimeEnd,DAR);
			if(c8001.unit[i].autoTimeStart > c8001.unit[i].autoTimeEnd) {
				*DAR = type_mismatch;
			}
			fprintf(stderr,"autoTimeStart = %d  autoTimeEnd = %d\n",c8001.unit[i].autoTimeStart,c8001.unit[i].autoTimeEnd);
		}
		break;
	}
	if(*DAR == success) {
		memcpy(&shareAddr->ctrls.c8001,&c8001,sizeof(CLASS_8001));
		*DAR = saveCoverClass(0x8001, 0, (void *) &c8001, sizeof(CLASS_8001),
				para_vari_save);
	}
	return index;
}

//终端保安定值
int class8100_set(int index, OAD oad, INT8U *data, INT8U *DAR)
{
	CLASS_8100 c8100={};
	ProgramInfo *shareAddr = getShareAddr();

	if (data[0] != 0x14) {
		return 0;
	}

//	readCoverClass(0x8100, 0, (void *) &c8100, sizeof(CLASS_8100),
//			para_vari_save);
	memcpy(&c8100,&shareAddr->ctrls.c8100,sizeof(CLASS_8100));
	c8100.v = getLongValue(data);
	shareAddr->ctrls.c8100.v = c8100.v;
	saveCoverClass(0x8100, 0, (void *) &c8100, sizeof(CLASS_8100),
			para_vari_save);
	asyslog(LOG_WARNING, "设置终端安保定值(%lld)", c8100.v);

	return 0;
}

/*
 * 终端时控时段
 * */
int class8101_set(OAD oad, INT8U *data, INT8U *DAR) {
	ProgramInfo *shareAddr = getShareAddr();
	CLASS_8101 c8101={};
	int	index = 0;

	memset(&c8101, 0x00, sizeof(CLASS_8101));
//	readCoverClass(0x8101, 0, (void *) &c8101, sizeof(CLASS_8101),
//			para_vari_save);
	memcpy(&c8101,&shareAddr->ctrls.c8101,sizeof(CLASS_8101));
	index += getArray(&data[index],&c8101.time_num,DAR);
	c8101.time_num = limitJudge("功控时段",12,c8101.time_num);
	fprintf(stderr,"c8101.time_num = %d\n",c8101.time_num);
	for (int i = 0; i < c8101.time_num; i++) {
		index += getUnsigned(&data[index],&c8101.time[i],DAR);
		fprintf(stderr,"%02x\n", c8101.time[i]);
	}
	if(*DAR == success) {
		memcpy(&shareAddr->ctrls.c8101,&c8101,sizeof(CLASS_8101));
		saveCoverClass(0x8101, 0, (void *) &c8101, sizeof(CLASS_8101),
			para_vari_save);
	}
	return index;
}

/*
 * 功控告警时间
 * */
int class8102_set(OAD oad, INT8U *data, INT8U *DAR) {
	CLASS_8102 c8102={};
	int index = 0;
	int i = 0;
	ProgramInfo *shareAddr = getShareAddr();

	memset(&c8102, 0x00, sizeof(CLASS_8102));
//	readCoverClass(0x8102, 0, (void *) &c8102, sizeof(CLASS_8102),
//			para_vari_save);
	memcpy(&c8102,&shareAddr->ctrls.c8102,sizeof(CLASS_8102));
	index += getArray(&data[index],&c8102.time_num,DAR);
	c8102.time_num = limitJudge("功控告警时间",8,c8102.time_num);
	fprintf(stderr,"c8102.time_num = %d\n",c8102.time_num);
	for (i = 0; i < c8102.time_num; i++) {
		index += getUnsigned(&data[index],&c8102.time[i],DAR);
		fprintf(stderr,"%d\n", c8102.time[i]);
	}
	fprintf(stderr,"DAR = %d\n", *DAR);
	if(*DAR == success) {
		memcpy(&shareAddr->ctrls.c8102,&c8102,sizeof(CLASS_8102));
		saveCoverClass(0x8102, 0, (void *) &c8102, sizeof(CLASS_8102),
			para_vari_save);
	}
	return index;
}

/*
 * 功控告警时间
 * */
int class8103_set(OAD oad, INT8U *data, INT8U *DAR) {
//	CLASS_8103 c8103={};
	int index = 0;
	int i = 0;
	INT8U unit_num = 0;
//	ProgramInfo *shareAddr = getShareAddr();

	index += getArray(&data[index],&unit_num,DAR);
	fprintf(stderr,"unit_num = %d\n",unit_num);
	for(i=0;i<unit_num;i++) {
		index += class8103_act3(1,oad.attflg,&data[index],DAR);
	}
	return index;
}

/*
 * 厂休控
 * */
int class8104_set(OAD oad, INT8U *data, INT8U *DAR) {
//	CLASS_8104 c8104={};
	int index = 0;
	int i = 0;
	INT8U unit_num = 0;
//	ProgramInfo *shareAddr = getShareAddr();

	index += getArray(&data[index],&unit_num,DAR);
	fprintf(stderr,"unit_num = %d\n",unit_num);
	for(i=0;i<unit_num;i++) {
		index += class8104_act3(1,oad.attflg,&data[index],DAR);
	}
	return index;
}

/*
 * 营业报停控
 * */
int class8105_set(OAD oad, INT8U *data, INT8U *DAR) {
//	CLASS_8105 c8105={};
	int index = 0;
	int i = 0;
	INT8U unit_num = 0;
//	ProgramInfo *shareAddr = getShareAddr();

	index += getArray(&data[index],&unit_num,DAR);
	fprintf(stderr,"unit_num = %d\n",unit_num);
	for(i=0;i<unit_num;i++) {
		index += class8105_act3(1,oad.attflg,&data[index],DAR);
	}
	return index;
}

/*
 * 月电控
 * */
int class8108_set(OAD oad, INT8U *data, INT8U *DAR) {
//	CLASS_8108 c8108={};
	int index = 0;
	int i = 0;
	INT8U unit_num = 0;
//	ProgramInfo *shareAddr = getShareAddr();

	index += getArray(&data[index],&unit_num,DAR);
	fprintf(stderr,"unit_num = %d\n",unit_num);
	for(i=0;i<unit_num;i++) {
		index += class8108_act3(1,oad.attflg,&data[index],DAR);
	}
	return index;
}

int get_PowerCtrlParam(INT8U *data,PowerCtrlParam *param, INT8U *DAR)
{
	int	ii = 0;

	ii += getStructure(&data[ii],NULL,DAR);
	ii += getBitString(1,&data[ii],&param->n);
	ii += getLong64(&data[ii],&param->t1);
	ii += getLong64(&data[ii],&param->t2);
	ii += getLong64(&data[ii],&param->t3);
	ii += getLong64(&data[ii],&param->t4);
	ii += getLong64(&data[ii],&param->t5);
	ii += getLong64(&data[ii],&param->t6);
	ii += getLong64(&data[ii],&param->t7);
	ii += getLong64(&data[ii],&param->t8);
	return ii;
}

/*
 * 时段功控
 * */
int class8103_act3(int index, int attr_act, INT8U *data, INT8U *DAR) {
	int	ii = 0;
	OI_698 oi = 0;
	int unit = 0;

	CLASS_8103 c8103={};
	ProgramInfo *shareAddr = getShareAddr();

//	readCoverClass(0x8103, 0, (void *) &c8103, sizeof(CLASS_8103),para_vari_save);
	memcpy(&c8103,&shareAddr->ctrls.c8103,sizeof(CLASS_8103));
	ii += getStructure(&data[ii],NULL,DAR);
	ii += getOI(1,&data[ii],&oi);
	unit  = oi - 0x2301;
	unit = rangeJudge("总加组",unit,0,(MAXNUM_SUMGROUP-1));
	c8103.list[unit].index = oi;
	asyslog(LOG_WARNING, "时段功控-添加控制单元[oi=%04x]",oi);
	ii += getBitString(1,&data[ii],&c8103.list[unit].sign);
	fprintf(stderr,"unit = %d,sign=%02x\n",unit,c8103.list[unit].sign);
	ii += get_PowerCtrlParam(&data[ii],&c8103.list[unit].v1,DAR);
	ii += get_PowerCtrlParam(&data[ii],&c8103.list[unit].v2,DAR);
	ii += get_PowerCtrlParam(&data[ii],&c8103.list[unit].v3,DAR);
	ii += getInteger(&data[ii],&c8103.list[unit].para,DAR);
	if(*DAR == success) {
		asyslog(LOG_WARNING, "时段功控-保存[unit=%d]",unit);
//		fprintf(stderr,"c8103 act 3  v2.n=%d t1 = %lld\n",c8103.list[unit].v2.n,c8103.list[unit].v2.t1);
		c8103.enable[unit].name = oi;
		c8103.output[unit].name = oi;
		c8103.overflow[unit].name = oi;
		memcpy(&shareAddr->ctrls.c8103, &c8103, sizeof(CLASS_8103));
		saveCoverClass(0x8103, 0, (void *) &c8103, sizeof(CLASS_8103),para_vari_save);
	}
	return ii;

//	if (data[0] != 0x02 || data[1] != 0x06) {
//		return 0;
//	}
//
//	if (data[2] != 0x50) {
//		return 0;
//	}
//
//	oi = data[3] * 256 + data[4];
//	unit  = oi - 0x2301;
//
//	c8103.list[unit].index = oi;//data[3] * 256 + data[4];
//	c8103.list[unit].sign = data[7];
//
//	if (data[8] != 0x02 || data[9] != 0x09) {
//		return 0;
//	}
//	fprintf(stderr,"oi=%04x unit=%d\n",oi,unit);
//
//	int ind = 12;
//	c8103.list[unit].v1.n = data[ind++];
//	c8103.list[unit].v1.t1 = getLongValue(&data[ind]);
//	ind =ind + 9;
//	c8103.list[unit].v1.t2 = getLongValue(&data[ind]);
//	ind =ind + 9;
//	c8103.list[unit].v1.t3 = getLongValue(&data[ind]);
//	ind =ind + 9;
//	c8103.list[unit].v1.t4 = getLongValue(&data[ind]);
//	ind =ind + 9;
//	c8103.list[unit].v1.t5 = getLongValue(&data[ind]);
//	ind =ind + 9;
//	c8103.list[unit].v1.t6 = getLongValue(&data[ind]);
//	ind =ind + 9;
//	c8103.list[unit].v1.t7 = getLongValue(&data[ind]);
//	ind =ind + 9;
//	c8103.list[unit].v1.t8 = getLongValue(&data[ind]);
//	ind =ind + 9;
//	fprintf(stderr,"111111=%d  %d  %d\n",ind,data[ind],data[ind+1]);
//	if (data[ind] != 0x02 || data[ind+1] != 0x09) {
//		return 0;
//	}
//	fprintf(stderr,"222222=%d  %d  %d\n",ind,data[ind],data[ind+1]);
//	ind = ind + 2;
//	c8103.list[unit].v2.n = data[ind++];
//	c8103.list[unit].v2.t1 = getLongValue(&data[ind]);
//	ind = ind + 9;
//	c8103.list[unit].v2.t2 = getLongValue(&data[ind]);
//	ind = ind + 9;
//	c8103.list[unit].v2.t3 = getLongValue(&data[ind]);
//	ind = ind + 9;
//	c8103.list[unit].v2.t4 = getLongValue(&data[ind]);
//	ind = ind + 9;
//	c8103.list[unit].v2.t5 = getLongValue(&data[ind]);
//	ind = ind + 9;
//	c8103.list[unit].v2.t6 = getLongValue(&data[ind]);
//	ind = ind + 9;
//	c8103.list[unit].v2.t7 = getLongValue(&data[ind]);
//	ind = ind + 9;
//	c8103.list[unit].v2.t8 = getLongValue(&data[ind]);
//	ind = ind + 9;
//	fprintf(stderr,"333333=%d  %d  %d\n",ind,data[ind],data[ind+1]);
//	if (data[ind] != 0x02 || data[ind+1] != 0x09) {
//		return 0;
//	}
//	fprintf(stderr,"444444=%d  %d  %d\n",ind,data[ind],data[ind+1]);
//	ind = ind +2;
//	c8103.list[unit].v2.n = data[ind++];
//	c8103.list[unit].v2.t1 = getLongValue(&data[ind]);
//	ind = ind + 9;
//	c8103.list[unit].v2.t2 = getLongValue(&data[ind]);
//	ind = ind + 9;
//	c8103.list[unit].v2.t3 = getLongValue(&data[ind]);
//	ind = ind + 9;
//	c8103.list[unit].v2.t4 = getLongValue(&data[ind]);
//	ind = ind + 9;
//	c8103.list[unit].v2.t5 = getLongValue(&data[ind]);
//	ind = ind + 9;
//	c8103.list[unit].v2.t6 = getLongValue(&data[ind]);
//	ind = ind + 9;
//	c8103.list[unit].v2.t7 = getLongValue(&data[ind]);
//	ind = ind + 9;
//	c8103.list[unit].v2.t8 = getLongValue(&data[ind]);
//	ind = ind + 10;
//	c8103.list[unit].para = data[ind];
//
//	fprintf(stderr,"v2.n=%d t1 = %lld\n",c8103.list[unit].v2.n,c8103.list[unit].v2.t1);
//	memcpy(shareAddr->ctrls.c8103.list, c8103.list, sizeof(c8103.list));
//	printf("c8103 act 3\n");
//	saveCoverClass(0x8103, 0, (void *) &c8103, sizeof(CLASS_8103),
//			para_vari_save);
//	return 0;
}


/*
 * 时段功控
 * */
int class8103_act4(int index, int attr_act, INT8U *data, INT8U *DAR) {
	int	ii = 0;
	OI_698 oi = 0;
	int unit = 0;

	CLASS_8103 c8103={};
	ProgramInfo *shareAddr = getShareAddr();

//	readCoverClass(0x8103, 0, (void *) &c8103, sizeof(CLASS_8103),para_vari_save);
	memcpy(&c8103,&shareAddr->ctrls.c8103,sizeof(CLASS_8103));
	ii += getStructure(&data[ii],NULL,DAR);
	ii += getOI(1,&data[ii],&oi);
	unit  = oi - 0x2301;
	unit = rangeJudge("总加组",unit,0,(MAXNUM_SUMGROUP-1));
	if(unit < 0 || unit > 7) {

	}
	c8103.list[unit].index = oi;
	asyslog(LOG_WARNING, "时段功控-添加控制单元[oi=%04x]",oi);
	ii += getBitString(1,&data[ii],&c8103.list[unit].sign);
	fprintf(stderr,"unit = %d,sign=%02x\n",unit,c8103.list[unit].sign);
	ii += get_PowerCtrlParam(&data[ii],&c8103.list[unit].v1,DAR);
	ii += get_PowerCtrlParam(&data[ii],&c8103.list[unit].v2,DAR);
	ii += get_PowerCtrlParam(&data[ii],&c8103.list[unit].v3,DAR);
	ii += getInteger(&data[ii],&c8103.list[unit].para,DAR);
	if(*DAR == success) {
		asyslog(LOG_WARNING, "时段功控-保存[unit=%d]",unit);
//		fprintf(stderr,"c8103 act 3  v2.n=%d t1 = %lld\n",c8103.list[unit].v2.n,c8103.list[unit].v2.t1);
		memcpy(shareAddr->ctrls.c8103.list, c8103.list, sizeof(c8103.list));
		c8103.enable[unit].name = oi;
		c8103.output[unit].name = oi;
		c8103.overflow[unit].name = oi;
		saveCoverClass(0x8103, 0, (void *) &c8103, sizeof(CLASS_8103),para_vari_save);
	}
	return ii;
}


int class8103_act6(int index, int attr_act, INT8U *data, Action_result *act_ret) {
	int oi = 0x00;
	if (data[0] != 0x50) {
		return 0;
	}

	oi = data[1] * 256 + data[2];
	asyslog(LOG_WARNING, "时段功控-控制投入[%04x]", oi);

	ProgramInfo *shareAddr = getShareAddr();
	int sindex = oi - 0x2301;
	sindex = rangeJudge("总加组",sindex,0,(MAXNUM_SUMGROUP-1));
	if(sindex == -1) {
		act_ret->DAR = interface_uncomp;
		return 0;
	}
	CLASS_8103 c8103={};
	memset(&c8103, 0x00, sizeof(CLASS_8103));
//	readCoverClass(0x8103, 0, (void *) &c8103, sizeof(CLASS_8103),
//			para_vari_save);

	memcpy(&c8103,&shareAddr->ctrls.c8103,sizeof(CLASS_8103));
	fprintf(stderr,"\nindex=%04x sign=%d",shareAddr->ctrls.c8103.list[0].index,c8103.list[0].sign);
	fprintf(stderr,"\nenable   name=%04x state=%d",shareAddr->ctrls.c8103.enable[0].name,shareAddr->ctrls.c8103.enable[0].state);
	fprintf(stderr,"\noutput   name=%04x state=%d",shareAddr->ctrls.c8103.output[0].name,shareAddr->ctrls.c8103.output[0].state);
	fprintf(stderr,"\noverflow name=%04x state=%d",shareAddr->ctrls.c8103.overflow[0].name,shareAddr->ctrls.c8103.overflow[0].state);
	c8103.enable[sindex].state = 0x01;
	c8103.enable[sindex].name = oi;
	shareAddr->ctrls.c8103.enable[sindex].state = 0x01;
	shareAddr->ctrls.c8103.enable[sindex].name = oi;
	fprintf(stderr,"\noutput   name=%04x state=%d",c8103.output[0].name,c8103.output[0].state);
	fprintf(stderr,"\noverflow name=%04x state=%d",c8103.overflow[0].name,c8103.overflow[0].state);

	saveCoverClass(0x8103, 0, (void *) &c8103, sizeof(CLASS_8103),
			para_vari_save);

	return 0;
}

int class8103_act7(int index, int attr_act, INT8U *data, Action_result *act_ret) {
	int oi = 0x00;
	if (data[0] != 0x50) {
		return 0;
	}

	oi = data[1] * 256 + data[2];
	asyslog(LOG_WARNING, "时段功控-控制解除[%04x]", oi);

	ProgramInfo *shareAddr = getShareAddr();
	int sindex = oi - 0x2301;
	sindex = rangeJudge("总加组",sindex,0,(MAXNUM_SUMGROUP-1));
	if(sindex == -1) {
		act_ret->DAR = interface_uncomp;
		return 0;
	}
	CLASS_8103 c8103;
	memset(&c8103, 0x00, sizeof(CLASS_8103));
//	readCoverClass(0x8103, 0, (void *) &c8103, sizeof(CLASS_8103),
//			para_vari_save);
	memcpy(&c8103,&shareAddr->ctrls.c8103,sizeof(CLASS_8103));
	c8103.enable[sindex].state = 0x00;
	c8103.enable[sindex].name = oi;
	shareAddr->ctrls.c8103.enable[sindex].state = 0x00;
	shareAddr->ctrls.c8103.enable[sindex].name = oi;
	shareAddr->class23[0].alCtlState.OutputState = 0x00;
	shareAddr->class23[0].alCtlState.PCAlarmState = 0x00;

	saveCoverClass(0x8103, 0, (void *) &c8103, sizeof(CLASS_8103),
			para_vari_save);
	return 0;
}

int class8103_act127(int index, int attr_act, INT8U *data,
		Action_result *act_ret) {
	ProgramInfo *shareAddr = getShareAddr();
	int oi = 0x00;
	INT8U sign = 0x00;
	INT8U numb = 0x00;

	if (data[0] != 0x02 || data[1] != 0x02) {
		return 0;
	}

	oi = data[3] * 256 + data[4];

	if (data[5] != 0x02 || data[6] != 0x02) {
		return 0;
	}

	sign = data[9];
	numb = data[11];

	asyslog(LOG_WARNING, "控制方案切换[8103-127],%04x-%02d-%02d", oi, sign, numb);

	CLASS_8103 c8103;
	memset(&c8103, 0x00, sizeof(CLASS_8103));
//		readCoverClass(0x8103, 0, (void *) &c8103, sizeof(CLASS_8103),
//				para_vari_save);
	memcpy(&c8103,&shareAddr->ctrls.c8103,sizeof(CLASS_8103));
	c8103.sign = sign;
	c8103.numb = numb;
	memcpy(&shareAddr->ctrls.c8103,&c8103,sizeof(CLASS_8103));
	saveCoverClass(0x8103, 0, (void *) &c8103, sizeof(CLASS_8103),
				para_vari_save);

	return 0;
}

int class8103_act_route(int index, int attr_act, INT8U *data,
		Action_result *act_ret) {
	switch (attr_act) {
	case 3:
		act_ret->datalen = class8103_act3(1, attr_act, data, &act_ret->DAR);
		break;
	case 4:
		class8103_act4(1, attr_act, data, &act_ret->DAR);
		break;
	case 6:
		class8103_act6(1, attr_act, data, act_ret);
		break;
	case 7:
		class8103_act7(1, attr_act, data, act_ret);
		break;
	case 127:
		class8103_act127(1, attr_act, data, act_ret);
		break;
	}
	return 1;
}

int class8104_act3(int index, int attr_act, INT8U *data, INT8U *DAR) {
	asyslog(LOG_WARNING, "厂休功控-添加控制单元");
	CLASS_8104 c8104;
	int ii = 0;
	OI_698 oi = 0x00;
	int unit = 0;

	ii += getStructure(&data[ii],NULL,DAR);
	ii += getOI(1,&data[ii],&oi);

	memset(&c8104, 0x00, sizeof(CLASS_8104));

	asyslog(LOG_WARNING, "厂休功控-添加控制单元(OI=%04x)",oi);
	if(oi>=0x2301 && oi<=0x2308) {
		unit = oi-0x2301;
	}else {
		*DAR = obj_unexist;
		return 0;
	}
	fprintf(stderr, "unit = %d\n", unit);
//	readCoverClass(0x8104, 0, (void *) &c8104, sizeof(CLASS_8104),
//		para_vari_save);

	ProgramInfo *shareAddr = getShareAddr();
	memcpy(&c8104,&shareAddr->ctrls.c8104,sizeof(CLASS_8104));
	c8104.list[unit].index = data[3] * 256 + data[4];
	INT64U v = 0x00;
	for (int j = 0; j < 8; j++) {
		v += data[6 + j] * powl(256, 7 - j);
	}
	c8104.list[unit].v = v;
	c8104.list[unit].start.year.data = data[15] * 256 + data[16];
	c8104.list[unit].start.month.data = data[17];
	c8104.list[unit].start.day.data = data[18];
	c8104.list[unit].start.hour.data = data[19];
	c8104.list[unit].start.min.data = data[20];

	c8104.list[unit].sustain = data[23] * 256 + data[24];
	c8104.list[unit].noDay = data[27];

	c8104.enable[unit].name = oi;
	c8104.output[unit].name = oi;
	c8104.overflow[unit].name = oi;
	memcpy(&shareAddr->ctrls.c8104,&c8104,sizeof(CLASS_8104));
	saveCoverClass(0x8104, 0, (void *) &c8104, sizeof(CLASS_8104),
			para_vari_save);
//	readCoverClass(0x8104, 0, (void *) &shareAddr->ctrls.c8104.list[unit], sizeof(CLASS_8104),
//			para_vari_save);
	fprintf(stderr, "刷新参数 %lld %d %d\n", shareAddr->ctrls.c8104.list[unit].v, shareAddr->ctrls.c8104.list[unit].sustain, shareAddr->ctrls.c8104.list[unit].noDay);
	return 27;
}

int class8104_act6(int index, int attr_act, INT8U *data, Action_result *act_ret) {
	int oi = 0x00;
	if (data[0] != 0x50) {
		return 0;
	}

	oi = data[1] * 256 + data[2];
	asyslog(LOG_WARNING, "厂休功控-控制投入[%04x]", oi);

	ProgramInfo *shareAddr = getShareAddr();
	int sindex = oi - 0x2301;
	sindex = rangeJudge("总加组",sindex,0,(MAXNUM_SUMGROUP-1));
	if(sindex == -1) {
		act_ret->DAR = interface_uncomp;
		return 0;
	}
	CLASS_8104 c8104;
	memset(&c8104, 0x00, sizeof(CLASS_8104));
//	readCoverClass(0x8104, 0, (void *) &c8104, sizeof(CLASS_8104),
//			para_vari_save);
	memcpy(&c8104,&shareAddr->ctrls.c8104,sizeof(CLASS_8104));
	c8104.enable[sindex].state = 0x01;
	c8104.enable[sindex].name = oi;
	shareAddr->ctrls.c8104.enable[sindex].state = 0x01;
	shareAddr->ctrls.c8104.enable[sindex].name = oi;

	saveCoverClass(0x8104, 0, (void *) &c8104, sizeof(CLASS_8104),
			para_vari_save);
	return 0;
}

int class8104_act7(int index, int attr_act, INT8U *data, Action_result *act_ret) {
	int oi = 0x00;
	if (data[0] != 0x50) {
		return 0;
	}

	oi = data[1] * 256 + data[2];
	asyslog(LOG_WARNING, "厂休功控-控制解除[%04x]", oi);

	ProgramInfo *shareAddr = getShareAddr();
	int sindex = oi - 0x2301;
	sindex = rangeJudge("总加组",sindex,0,(MAXNUM_SUMGROUP-1));
	if(sindex == -1) {
		act_ret->DAR = interface_uncomp;
		return 0;
	}
	CLASS_8104 c8104;
	memset(&c8104, 0x00, sizeof(CLASS_8104));
//	readCoverClass(0x8104, 0, (void *) &c8104, sizeof(CLASS_8104),
//			para_vari_save);
	memcpy(&c8104,&shareAddr->ctrls.c8104,sizeof(CLASS_8104));
	c8104.enable[sindex].state = 0x00;
	shareAddr->ctrls.c8104.enable[sindex].state = 0x00;
	shareAddr->class23[0].alCtlState.OutputState = 0x00;
	shareAddr->class23[0].alCtlState.PCAlarmState = 0x00;
	saveCoverClass(0x8104, 0, (void *) &c8104, sizeof(CLASS_8104),
			para_vari_save);
	return 0;
}

int class8104_act_route(int index, int attr_act, INT8U *data,
		Action_result *act_ret) {
	switch (attr_act) {
	case 3:
		act_ret->datalen = class8104_act3(1, attr_act, data, &act_ret->DAR);
		break;
	case 6:
		class8104_act6(1, attr_act, data, act_ret);
		break;
	case 7:
		class8104_act7(1, attr_act, data, act_ret);
		break;
	}
	return 1;
}

int class8105_act3(int index, int attr_act, INT8U *data, INT8U *DAR) {
	OI_698 oi = 0x00;
	int unit = 0;
	int	ii = 0;
	CLASS_8105 c8105={};

	ProgramInfo *shareAddr = getShareAddr();
	ii += getStructure(&data[ii],NULL,DAR);
	ii += getOI(1,&data[ii],&oi);
	asyslog(LOG_WARNING, "营业报停-添加控制单元(OI=%04x)",oi);

	if(oi>=0x2301 && oi<=0x2308) {
		unit = oi-0x2301;
	}else {
		*DAR = obj_unexist;
		return 0;
	}
	memset(&c8105, 0x00, sizeof(CLASS_8105));

//	readCoverClass(0x8105, 0, (void *) &c8105, sizeof(CLASS_8105),
//			para_vari_save);
	memcpy(&c8105,&shareAddr->ctrls.c8105,sizeof(CLASS_8105));
	c8105.list[unit].index = oi;
	ii += getDateTimeS(1,&data[ii],(INT8U *)&c8105.list[unit].start,DAR);
	ii += getDateTimeS(1,&data[ii],(INT8U *)&c8105.list[unit].end,DAR);
	ii += getLong64(&data[ii],&c8105.list[unit].v);
	fprintf(stderr,"c8105.v = %lld\n",c8105.list[unit].v);
	printDataTimeS("报停起始时间",c8105.list[unit].start);
	printDataTimeS("报停结束时间",c8105.list[unit].end);

	if(*DAR == success) {
		c8105.enable[unit].name = oi;
		c8105.output[unit].name = oi;
		c8105.overflow[unit].name = oi;
		fprintf(stderr,"enable[%d].name = %04x\n",unit,c8105.enable[unit].name);
		memcpy(&shareAddr->ctrls.c8105,&c8105,sizeof(CLASS_8105));
		saveCoverClass(0x8105, 0, (void *) &c8105, sizeof(CLASS_8105),
			para_vari_save);
	}
	return ii;
}

int class8105_act6(int index, int attr_act, INT8U *data, Action_result *act_ret) {
	int oi = 0x00;
	if (data[0] != 0x50) {
		act_ret->DAR =type_mismatch;
		act_ret->datalen = 0;
		return 0;
	}

	oi = data[1] * 256 + data[2];
	asyslog(LOG_WARNING, "营业报停-控制投入[%04x]", oi);

	ProgramInfo *shareAddr = getShareAddr();
	int sindex = oi - 0x2301;
	sindex = rangeJudge("总加组",sindex,0,(MAXNUM_SUMGROUP-1));
	if(sindex == -1) {
		act_ret->DAR =obj_unexist;
		act_ret->datalen = 0;
		return 0;
	}

	CLASS_8105 c8105={};
	memset(&c8105, 0x00, sizeof(CLASS_8105));
//	readCoverClass(0x8105, 0, (void *) &c8105, sizeof(CLASS_8105),
//			para_vari_save);
	memcpy(&c8105,&shareAddr->ctrls.c8105,sizeof(CLASS_8105));
	c8105.enable[sindex].state = 0x01;
	c8105.enable[sindex].name = 0x01;
	shareAddr->ctrls.c8105.enable[sindex].state = 0x01;
	shareAddr->ctrls.c8105.enable[sindex].name = 0x01;
	saveCoverClass(0x8105, 0, (void *) &c8105, sizeof(CLASS_8105),
			para_vari_save);
	return 0;
}

int class8105_act7(int index, int attr_act, INT8U *data, Action_result *act_ret) {
	int oi = 0x00;
	if (data[0] != 0x50) {
		act_ret->DAR =type_mismatch;
		act_ret->datalen = 0;
		return 0;
	}
	oi = data[1] * 256 + data[2];
	asyslog(LOG_WARNING, "营业报停-控制解除[%04x]", oi);

	ProgramInfo *shareAddr = getShareAddr();
	int sindex = oi - 0x2301;
	sindex = rangeJudge("总加组",sindex,0,(MAXNUM_SUMGROUP-1));
	if(sindex == -1) {
		act_ret->DAR =boundry_over;
		act_ret->datalen = 0;
		return 0;
	}

	CLASS_8105 c8105={};
	memset(&c8105, 0x00, sizeof(CLASS_8105));
//	readCoverClass(0x8105, 0, (void *) &c8105, sizeof(CLASS_8105),
//			para_vari_save);
	memcpy(&c8105,&shareAddr->ctrls.c8105,sizeof(CLASS_8105));
	c8105.enable[sindex].state = 0x00;
	shareAddr->ctrls.c8105.enable[sindex].state = 0x00;
	shareAddr->class23[0].alCtlState.OutputState = 0x00;
	shareAddr->class23[0].alCtlState.PCAlarmState = 0x00;
	saveCoverClass(0x8105, 0, (void *) &c8105, sizeof(CLASS_8105),
			para_vari_save);
	return 0;
}

int class8105_act_route(int index, int attr_act, INT8U *data,
		Action_result *act_ret) {
	switch (attr_act) {
	case 3:
		act_ret->datalen = class8105_act3(1, attr_act, data, &act_ret->DAR);
		break;
	case 6:
		class8105_act6(1, attr_act, data, act_ret);
		break;
	case 7:
		class8105_act7(1, attr_act, data, act_ret);
		break;
	}
	return 1;
}

/*
 * 修改购电控配置单元
 * */
int set_class13_att3(INT8U service,INT8U *data,int *sum_index,ALSTATE *alstate,INT8U *DAR)
{
	int 		index = 0;
	OI_698		oi=0;
	ALSTATE		tmp_alstate={};

	switch(service) {
	case 3: //set 属性3
		index += getStructure(&data[index],NULL,DAR);
		index += getOI(1,&data[index],&oi);
		tmp_alstate.name = oi;
		index += getEnum(1,&data[index],&tmp_alstate.state);
		*sum_index = oi - 0x2301;
		*sum_index = rangeJudge("总加组",*sum_index,0,(MAXNUM_SUMGROUP-1));
		if(*sum_index == -1)  *DAR = obj_unexist;
		break;
	case 6:	//投入
		index += getOI(1,&data[index],&oi);
		*sum_index = oi - 0x2301;
		*sum_index = rangeJudge("总加组",*sum_index,0,(MAXNUM_SUMGROUP-1));
		if(*sum_index == -1)  *DAR = obj_unexist;
		asyslog(LOG_WARNING, "购电-控制投入[%04x]", oi);
		tmp_alstate.name = oi;
		tmp_alstate.state = 1;
		break;
	case 7:	//解除
		index += getOI(1,&data[index],&oi);
		*sum_index = oi - 0x2301;
		*sum_index = rangeJudge("总加组",*sum_index,0,(MAXNUM_SUMGROUP-1));
		if(*sum_index == -1)  *DAR = obj_unexist;
		asyslog(LOG_WARNING, "购电-控制解除[%04x]", oi);
		tmp_alstate.name = 0;
		tmp_alstate.state = 0;
		break;
	}
	if(*DAR == success) {
		memcpy(alstate,&tmp_alstate,sizeof(ALSTATE));
	}
	return index;
}

int class8106_unit(int attr_act, INT8U *data, CLASS_8106 *shmc8106, INT8U *DAR)
{
	int	oi_tmp = 0;
	int	ii = 0;
	CLASS_8106 c8106={};

	ii += getStructure(&data[ii],NULL,DAR);
	ii += getOI(1,&data[ii],&c8106.index);
	asyslog(LOG_WARNING, "功率下浮-控制投入[%04x]", c8106.index);
	oi_tmp = rangeJudge("总加组",c8106.index,0x2301,0x2308);
	if(oi_tmp == -1)  *DAR = obj_unexist;
	switch(attr_act) {
	case 3:	//添加
	case 5: //更新
	case 127: //
		if(attr_act == 127) c8106.enable.state = 0x01;
		ii += getStructure(&data[ii],NULL,DAR);
		ii += getUnsigned(&data[ii],&c8106.list.down_huacha,DAR);
		ii += getInteger(&data[ii],&c8106.list.down_xishu,DAR);
		ii += getUnsigned(&data[ii],&c8106.list.down_freeze,DAR);
		ii += getUnsigned(&data[ii],&c8106.list.down_ctrl_time,DAR);
		ii += getUnsigned(&data[ii],&c8106.list.t1,DAR);
		ii += getUnsigned(&data[ii],&c8106.list.t2,DAR);
		ii += getUnsigned(&data[ii],&c8106.list.t3,DAR);
		ii += getUnsigned(&data[ii],&c8106.list.t4,DAR);
		break;
	case 4:	//删除
		memset(&c8106, 0x00, sizeof(CLASS_8106));
		break;
	}
	if(*DAR==success) {
		c8106.enable.name = c8106.index;
		c8106.output.name = c8106.index;
		c8106.overflow.name = c8106.index;
		fprintf(stderr,"c8106.enable.name = %04x\n",c8106.enable.name);
		memcpy(shmc8106,&c8106,sizeof(CLASS_8106));
	}
	return ii;
}

int class8106_act_route(OAD oad, INT8U *data, Action_result *act_ret)
{
	fprintf(stderr, "class8106_act_route  class8106_act_route %d\n", oad.attflg);
	int sum_index = 0;
	ProgramInfo *shareAddr = getShareAddr();
	switch (oad.attflg) {
	case 3://添加
	case 4://删除
	case 5://更新
		act_ret->datalen = class8106_unit(oad.attflg, data, &shareAddr->ctrls.c8106, &act_ret->DAR);
		break;
	case 6:	//控制投入
		act_ret->datalen = set_class13_att3(oad.attflg,data,&sum_index,&shareAddr->ctrls.c8106.enable,&act_ret->DAR);
		break;
	case 7: //控制解除
		act_ret->datalen = set_class13_att3(oad.attflg,data,&sum_index,&shareAddr->ctrls.c8106.enable,&act_ret->DAR);
		if(act_ret->DAR == success && (sum_index>=0 && sum_index<= MAX_AL_UNIT)) {
			pcAlarm.u8b = shareAddr->class23[sum_index].alCtlState.PCAlarmState;
			pcAlarm.pcstate.power_ctl = 0;
			shareAddr->class23[sum_index].alCtlState.PCAlarmState = pcAlarm.u8b;
			shareAddr->class23[sum_index].alCtlState.OutputState = 0;
		}
		break;
	case 127://投入（总加组对象，控制方案）
		act_ret->datalen = class8106_unit(oad.attflg, data, &shareAddr->ctrls.c8106, &act_ret->DAR);
		break;
	}
	asyslog(LOG_WARNING, "class8106 DAR=%d   index=%04x", act_ret->DAR ,shareAddr->ctrls.c8106.index);
	if(act_ret->DAR == success) {
		asyslog(LOG_WARNING, "class8106 index=%04x    OI=%04x",shareAddr->ctrls.c8106.index,oad.OI);
		saveCoverClass(oad.OI, 0, (void *) &shareAddr->ctrls.c8106, sizeof(CLASS_8106),para_vari_save);
	}
	return 1;
}

//int class8107_act3(int index, int attr_act, INT8U *data, Action_result *act_ret) {
//	INT16U oi = 0x00;
//	int id = 0x00;
//	INT8U sign = 0x00;
//	INT8U type = 0x00;
//	INT64U val = 0x00;
//	INT64U war_thr = 0x00;
//	INT64U ctl_thr = 0x00;
//	INT8U mode = 0x00;
//
//	ProgramInfo *shareAddr = getShareAddr();
//
//	if (data[0] != 0x02 || data[1] != 0x08) {
//		return 0;
//	}
//
//	oi = data[3] * 256 + data[4];
//	INT16U oi_b = data[4] * 256 + data[3];
//	int sindex = oi - 0x2301;
//	id = data[6] * 256 * 256 * 256 + data[7] * 256 * 256 + data[8] * 256
//			+ data[9];
//	shareAddr->ctrls.c8107.list[sindex].no = id;
//
//	sign = data[11];
//	shareAddr->ctrls.c8107.list[sindex].add_refresh = sign;
//
//	type = data[13];
//	shareAddr->ctrls.c8107.list[sindex].type = type;
//
//	val = getLongValue(&data[14]);
//	shareAddr->ctrls.c8107.list[sindex].v = val/100;
//	war_thr = getLongValue(&data[23]);
//	shareAddr->ctrls.c8107.list[sindex].alarm = war_thr/100;
//	ctl_thr = getLongValue(&data[32]);
//	shareAddr->ctrls.c8107.list[sindex].ctrl = ctl_thr/100;
//	mode = data[42];
//	shareAddr->ctrls.c8107.list[sindex].mode = mode;
//
//	shareAddr->class23[sindex].remains += shareAddr->ctrls.c8107.list[sindex].v;
//
//
//	Event_3202(&oi_b,2, getShareAddr());
//	return 0;
//}


//int class8107_act5(int index, int attr_act, INT8U *data, Action_result *act_ret) {
//	INT16U oi = 0x00;
//	int id = 0x00;
//	INT8U sign = 0x00;
//	INT8U type = 0x00;
//	INT64U val = 0x00;
//	INT64U war_thr = 0x00;
//	INT64U ctl_thr = 0x00;
//	INT8U mode = 0x00;
//
//	ProgramInfo *shareAddr = getShareAddr();
//
//	if (data[0] != 0x02 || data[1] != 0x08) {
//		return 0;
//	}
//
//	oi = data[3] * 256 + data[4];
//	INT16U oi_b = data[4] * 256 + data[3];
//	int sindex = oi - 0x2301;
//	id = data[6] * 256 * 256 * 256 + data[7] * 256 * 256 + data[8] * 256
//			+ data[9];
//	shareAddr->ctrls.c8107.list[sindex].no = id;
//
//	sign = data[11];
//	shareAddr->ctrls.c8107.list[sindex].add_refresh = sign;
//
//	type = data[13];
//	shareAddr->ctrls.c8107.list[sindex].type = type;
//
//	val = getLongValue(&data[14]);
//	shareAddr->ctrls.c8107.list[sindex].v = val;
//	war_thr = getLongValue(&data[23]);
//	shareAddr->ctrls.c8107.list[sindex].alarm = war_thr;
//	ctl_thr = getLongValue(&data[32]);
//	shareAddr->ctrls.c8107.list[sindex].ctrl = ctl_thr;
//	mode = data[42];
//	shareAddr->ctrls.c8107.list[sindex].mode = mode;
//	asyslog(LOG_WARNING, "购电-更新控制单元[%04x-%d-%d-%d-%lld-%lld-%lld-%d]", oi, id,
//			sign, type, val, war_thr, ctl_thr, mode);
//
//	Event_3202(&oi_b,2, getShareAddr());
//	return 0;
//}
//
//int class8107_act6(int index, int attr_act, INT8U *data, Action_result *act_ret) {
//	int oi = 0x00;
//	if (data[0] != 0x50) {
//		act_ret->DAR =type_mismatch;
//		act_ret->datalen = 0;
//		return 0;
//	}
//
//	ProgramInfo *shareAddr = getShareAddr();
//
//	oi = data[1] * 256 + data[2];
//	int sindex = oi - 0x2301;
//	asyslog(LOG_WARNING, "购电-控制投入[%04x]", oi);
//
//	CLASS_8107 c8107={};
//	memset(&c8107, 0x00, sizeof(CLASS_8107));
//	readCoverClass(0x8107, 0, (void *) &c8107, sizeof(CLASS_8107),
//			para_vari_save);
//
//	c8107.enable[sindex].state = 0x01;
//	c8107.enable[sindex].name = 0x01;
//	shareAddr->ctrls.c8107.enable[sindex].state = 0x01;
//	shareAddr->ctrls.c8107.enable[sindex].name = 0x01;
//
//	saveCoverClass(0x8107, 0, (void *) &c8107, sizeof(CLASS_8107),
//			para_vari_save);
//	return 0;
//}

//int class8107_act7(int index, int attr_act, INT8U *data, Action_result *act_ret) {
//	int oi = 0x00;
//	if (data[0] != 0x50) {
//		act_ret->DAR =type_mismatch;
//		act_ret->datalen = 0;
//		return 0;
//	}
//
//	ProgramInfo *shareAddr = getShareAddr();
//
//	oi = data[1] * 256 + data[2];
//	int sindex = oi - 0x2301;
//	asyslog(LOG_WARNING, "购电-控制解除[%04x]", oi);
//
//	CLASS_8107 c8107={};
//	memset(&c8107, 0x00, sizeof(CLASS_8107));
//	readCoverClass(0x8107, 0, (void *) &c8107, sizeof(CLASS_8107),
//			para_vari_save);
//
//	c8107.enable[sindex].state = 0x00;
//	c8107.enable[sindex].name = 0x00;
//	shareAddr->ctrls.c8107.enable[sindex].state = 0x00;
//	shareAddr->ctrls.c8107.enable[sindex].name = 0x00;
//	shareAddr->class23[sindex].alCtlState.OutputState = 0x00;
//	shareAddr->class23[sindex].alCtlState.BuyOutputState = 0x00;
//	shareAddr->class23[sindex].alCtlState.ECAlarmState = 0x00;
//
//	saveCoverClass(0x8107, 0, (void *) &c8107, sizeof(CLASS_8107),
//			para_vari_save);
//	return 0;
//}

/*
 * 修改购电控配置单元
 * */
int set_OI810c(INT8U service,INT8U *data,BUY_CTRL *oi810c,INT8U *DAR)
{
	int			sum_index = 0;
	int 		index = 0;
	INT8U		stru_num = 0;
	BUY_CTRL	tmp_oi810c={};

	ProgramInfo *shareAddr = getShareAddr();
	index += getStructure(&data[index],&stru_num,DAR);
	if(stru_num != 8)	*DAR = interface_uncomp;
	index += getOI(1,&data[index],&tmp_oi810c.index);
	INT16U oi_b = (tmp_oi810c.index<<8) | ((tmp_oi810c.index>>8));
	sum_index = tmp_oi810c.index - 0x2301;
	sum_index = rangeJudge("总加组",sum_index,0,(MAXNUM_SUMGROUP-1));
	if(sum_index == -1) {
		*DAR = obj_unexist;
		return 0;
	}
	switch(service) {
	case 2:	//set 属性2
	case 3:	//添加
	case 5: //更新
		index += getDouble(&data[index],(INT8U *)&tmp_oi810c.no);
		index += getEnum(1,&data[index],&tmp_oi810c.add_refresh);
		index += getEnum(1,&data[index],&tmp_oi810c.type);
		index += getLong64(&data[index],&tmp_oi810c.v);
		index += getLong64(&data[index],&tmp_oi810c.alarm);
		index += getLong64(&data[index],&tmp_oi810c.ctrl);
		index += getEnum(1,&data[index],&tmp_oi810c.mode);
		break;
	case 4:	//删除
		memset(&tmp_oi810c, 0x00, sizeof(BUY_CTRL));
		break;
	}
	if(*DAR == success) {
		asyslog(LOG_WARNING, "购电-控制单元【act=%d】[%04x-%d-%d-%d-%lld-%lld-%lld-%d]",service, tmp_oi810c.index, tmp_oi810c.no,
				tmp_oi810c.add_refresh, tmp_oi810c.type, tmp_oi810c.v, tmp_oi810c.alarm, tmp_oi810c.ctrl, tmp_oi810c.mode);
		shareAddr->ctrls.c8107.enable[sum_index].name = tmp_oi810c.index;
		shareAddr->ctrls.c8107.output[sum_index].name = tmp_oi810c.index;
		shareAddr->ctrls.c8107.overflow[sum_index].name = tmp_oi810c.index;

		fprintf(stderr,"enable[%d] = %04x\n",shareAddr->ctrls.c8107.enable[sum_index].name);
		memcpy(&oi810c[sum_index],&tmp_oi810c,sizeof(BUY_CTRL));
		if(service == 3 || service == 5) {
			shareAddr->class23[sum_index].remains += shareAddr->ctrls.c8107.list[sum_index].v;
			asyslog(LOG_WARNING,"Event_3202事件 oi_b=%04x\n",oi_b);
			Event_3202((INT8U *)&oi_b,2, getShareAddr());
		}
	}
	return index;
}

int class8107_act_route(OAD oad, INT8U *data,Action_result *act_ret) {
	int sum_index = 0;
	ProgramInfo *shareAddr = getShareAddr();
	switch (oad.attflg) {
	case 3:
	case 4:
	case 5:
		act_ret->datalen = set_OI810c(oad.attflg,data,shareAddr->ctrls.c8107.list,&act_ret->DAR);
		break;
	case 6:	//控制投入
		act_ret->datalen = set_class13_att3(oad.attflg,data,&sum_index,shareAddr->ctrls.c8107.enable,&act_ret->DAR);
		break;
	case 7: //控制解除
		act_ret->datalen = set_class13_att3(oad.attflg,data,&sum_index,shareAddr->ctrls.c8107.enable,&act_ret->DAR);
		fprintf(stderr,"\n *********  oi=%04x   sum_index=%d    dar=%d  ************* \n",oad.OI,sum_index,act_ret->DAR);

		if(act_ret->DAR == success && (sum_index>=0 && sum_index<= MAX_AL_UNIT)) {
			ecAlarm.u8b = shareAddr->class23[sum_index].alCtlState.ECAlarmState;
			ecAlarm.ecstate.buy_elec_ctl = 0;
			shareAddr->class23[sum_index].alCtlState.OutputState = 0x00;
			shareAddr->class23[sum_index].alCtlState.ECAlarmState = 0x00;
			shareAddr->class23[sum_index].alCtlState.BuyOutputState = 0x00;
			fprintf(stderr,"\n ********** sum[%d] alarmstate=%02x   outstate=%02x butout=%03x",sum_index,
					shareAddr->class23[sum_index].alCtlState.ECAlarmState,
					shareAddr->class23[sum_index].alCtlState.OutputState,
					shareAddr->class23[sum_index].alCtlState.BuyOutputState );
		}
		break;
	}
	if(act_ret->DAR == success) {
		saveCoverClass(0x8107, 0, (void *) &shareAddr->ctrls.c8107, sizeof(CLASS_8107),para_vari_save);
	}
	return 1;
}

int class8107_set(OAD oad, INT8U *data, INT8U *DAR)
{
	int		index=0;
	INT8U	unit_num = 0 ,i = 0;
	int		sum_index = 0;
	ProgramInfo *shareAddr = getShareAddr();
	switch(oad.attflg) {
	case 2:		//增加
		index += getArray(&data[index],&unit_num,DAR);
		fprintf(stderr,"unit_num = %d\n",unit_num);
		for(i=0;i<unit_num;i++) {
			index += set_OI810c(oad.attflg,&data[index],shareAddr->ctrls.c8107.list,DAR);
		}
		break;
	case 3:
		index += getArray(&data[index],&unit_num,DAR);
		fprintf(stderr,"unit_num = %d\n",unit_num);
		for(i=0;i<unit_num;i++) {
			index += set_class13_att3(oad.attflg,&data[index],&sum_index,shareAddr->ctrls.c8107.enable,DAR);
		}
		break;
	}
	if(*DAR == success) {
		saveCoverClass(oad.OI, 0, (void *) &shareAddr->ctrls.c8107, sizeof(CLASS_8107),para_vari_save);
	}
	return index;
}

int class8108_act3(int index, int attr_act, INT8U *data, INT8U *DAR) {
	int oi = 0x00;
	INT64U v = 0x00;
	INT8U th = 0x00;
	INT8U fl = 0x00;
	CLASS_8108 c8108;
//	readCoverClass(0x8108, 0, (void *) &c8108, sizeof(CLASS_8108),
//			para_vari_save);

	ProgramInfo *shareAddr = getShareAddr();
	memcpy(&c8108,&shareAddr->ctrls.c8108,sizeof(CLASS_8108));
	if (data[0] != 0x02 || data[1] != 0x04) {
		return 0;
	}

	oi = data[3] * 256 + data[4];
	index = oi - 0x2301;
	index = rangeJudge("总加组",index,0,(MAXNUM_SUMGROUP-1));
	if(index == -1) {
		*DAR = obj_unexist;
		return 0;
	}

	shareAddr->ctrls.c8108.list[index].v = getLongValue(&data[5]);
	v = getLongValue(&data[5]);
	shareAddr->ctrls.c8108.list[index].para = data[15];
	th = data[15];
	shareAddr->ctrls.c8108.list[index].flex = data[17];
	fl = data[17];

	c8108.list[index].flex = shareAddr->ctrls.c8108.list[index].flex ;
	c8108.list[index].para = shareAddr->ctrls.c8108.list[index].para ;
	c8108.list[index].v = shareAddr->ctrls.c8108.list[index].v ;
	c8108.list[index].index = oi;

	c8108.enable[index].name = oi;
	c8108.output[index].name = oi;
	c8108.overflow[index].name = oi;
	asyslog(LOG_WARNING, "月电-添加控制单元[%04x-%lld-%d-%d]", oi, v, th, fl);

	saveCoverClass(0x8108, 0, (void *) &c8108, sizeof(CLASS_8108),
			para_vari_save);
	return 17;
}

int class8108_act6(int index, int attr_act, INT8U *data, Action_result *act_ret) {
	int oi = 0x00;
	if (data[0] != 0x50) {
		act_ret->DAR =type_mismatch;
		act_ret->datalen = 0;
		return 0;
	}

	ProgramInfo *shareAddr = getShareAddr();

	oi = data[1] * 256 + data[2];
	int sindex = oi - 0x2301;
	sindex = rangeJudge("总加组",sindex,0,(MAXNUM_SUMGROUP-1));
	if(sindex == -1) {
		act_ret->DAR = obj_unexist;
		return 0;
	}

	asyslog(LOG_WARNING, "月电-控制投入[%04x]", oi);

	CLASS_8108 c8108;
	memset(&c8108, 0x00, sizeof(CLASS_8108));
//	readCoverClass(0x8108, 0, (void *) &c8108, sizeof(CLASS_8108),
//			para_vari_save);
	memcpy(&c8108,&shareAddr->ctrls.c8108,sizeof(CLASS_8108));
	shareAddr->ctrls.c8108.enable[sindex].state = 0x01;
	shareAddr->ctrls.c8108.enable[sindex].name = 0x01;
	c8108.enable[sindex].state = 0x01;
	c8108.enable[sindex].name = 0x01;
	saveCoverClass(0x8108, 0, (void *) &c8108, sizeof(CLASS_8108),
			para_vari_save);

	return 0;
}

int class8108_act7(int index, int attr_act, INT8U *data, Action_result *act_ret) {
	int oi = 0x00;
	if (data[0] != 0x50) {
		act_ret->DAR =type_mismatch;
		act_ret->datalen = 0;
		return 0;
	}

	ProgramInfo *shareAddr = getShareAddr();

	oi = data[1] * 256 + data[2];
	int sindex = oi - 0x2301;
	sindex = rangeJudge("总加组",sindex,0,(MAXNUM_SUMGROUP-1));
	if(sindex == -1) {
		act_ret->DAR = obj_unexist;
		return 0;
	}
	asyslog(LOG_WARNING, "月电-控制解除[%04x]", oi);

	CLASS_8108 c8108={};
	memset(&c8108, 0x00, sizeof(CLASS_8108));
//	readCoverClass(0x8108, 0, (void *) &c8108, sizeof(CLASS_8108),
//			para_vari_save);
	memcpy(&c8108,&shareAddr->ctrls.c8108,sizeof(CLASS_8108));
	c8108.enable[sindex].state = 0x00;
	shareAddr->ctrls.c8108.enable[sindex].state = 0x00;
	c8108.enable[sindex].name = 0x00;
	shareAddr->ctrls.c8108.enable[sindex].name = 0x00;
	//月电控
//	ecAlarm.ecstate = shareAddr->class23[sindex].alCtlState.MonthOutputState;
//	ecAlarm.ecstate.month_elec_ctl = 0;
	shareAddr->class23[sindex].alCtlState.OutputState = 0x00;
	shareAddr->class23[sindex].alCtlState.MonthOutputState = 0x00;
	shareAddr->class23[sindex].alCtlState.ECAlarmState = 0x00;

	saveCoverClass(0x8108, 0, (void *) &c8108, sizeof(CLASS_8108),
			para_vari_save);

	return 0;
}

int class8108_act_route(int index, int attr_act, INT8U *data,
		Action_result *act_ret) {
	switch (attr_act) {
	case 3:
		act_ret->datalen = class8108_act3(1, attr_act, data, act_ret->DAR);
		break;
	case 6:
		class8108_act6(1, attr_act, data, act_ret);
		break;
	case 7:
		class8108_act7(1, attr_act, data, act_ret);
		break;
	}
	return 0;
}
