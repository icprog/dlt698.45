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
#include "ParaDef.h"
#include "event.h"
#include "dlt698.h"

//typedef union { //control code
//	INT8U u8b; //convenient to set value to 0
//	struct { //only for little endian mathine!
//		INT8U bak :4; //备用
//		INT8U power_ctl :1; //当前功率下浮控
//		INT8U business_ctl :1; //营业报停控
//		INT8U work_ctl :1; //厂休控
//		INT8U time_ctl :1; //时段控
//	} pcstate;
//} PCAlarmState;
//
//typedef union { //control code
//	INT8U u8b; //convenient to set value to 0
//	struct { //only for little endian mathine!
//		INT8U bak :6; //备用
//		INT8U buy_elec_ctl :1; 	//购电控
//		INT8U month_elec_ctl :1; //月电控
//	} ecstate;
//} ECAlarmState;
//
//ECAlarmState ecAlarm;
//PCAlarmState pcAlarm;

/*
 * 根据QGDW1374.1-2013在参数设置，控制投入或解除及控制执行时应有音响（或者语音）告警通知用户
 * */
void buzzerCtrl()
{
	gpio_writebyte((INT8S *)DEV_ALARM_BUZZER,0x01);
	usleep(500000);
	gpio_writebyte((INT8S *)DEV_ALARM_BUZZER,0x0);
}

///////////////////////////////////////////////set
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
		buzzerCtrl();
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
	memcpy(&c8001,&shareAddr->ctrls.c8001,sizeof(CLASS_8001));
	switch(oad.attflg) {
	case 2:	//保电状态，只读
//		index += getEnum(1,data,&c8001.state);
//		if(index==0)  *DAR = type_mismatch;
		*DAR = no_wdblock_state;
		break;
	case 3:
		index += getLongUnsigned(data,(INT8U *)&c8001.noCommTime);
		if(index==0)  *DAR = type_mismatch;
		else asyslog(LOG_WARNING, "设置保电属性3(%d)", c8001.noCommTime);
		if(c8001.noCommTime !=0) {	//=0:表示不自动保电
			c8001.state = 2;		//自动保电
		}
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
		buzzerCtrl();
	}
	return index;
}

/*
 * 根据当前信息序号，查找需要添加的数组位置
 * */
INT8U findInfoIndex(CLASS_8003_8004 info,INT8U info_no)
{
	INT8U  infoindex = 0xff, i=0;
	fprintf(stderr,"info_no=%d\n",info_no);
	for(i=0;i<10;i++) {
		if(info.chinese_info[i].no==info_no) {
			infoindex = i;
			break;
		}
	}
	if(infoindex == 0xff) {	//未找到相同序号，查找空位置
		for(i=0;i<10;i++) {
			fprintf(stderr,"info[%d].no=%d\n",i,info.chinese_info[i].no);
			if(info.chinese_info[i].no==0xff) {
				infoindex = i;
				break;
			}
		}
	}
	fprintf(stderr,"findinfo  infoindex = %d\n",infoindex);
	return infoindex;
}
/*
 * 一般中文信息，重要中文信息
 * */
int class8003_8004_set(OAD oad, INT8U *data, INT8U *DAR) {
	CLASS_8003_8004 info={};
	INT8U	i=0,info_num=0;
	INT8U	index = 0;
	INT8U   infoindex = 0,info_no=0;
	int		ret = 0;

	ret = readCoverClass(oad.OI, 0, (void *) &info, sizeof(CLASS_8003_8004),para_vari_save);
	if(ret == -1) {
		for(i=0;i<10;i++) {
			info.chinese_info[i].no = 0xff;
		}
	}
	switch(oad.attflg) {
	case 2:	//
		index += getArray(&data[index],&info_num,DAR);
		fprintf(stderr,"info_num = %d\n",info_num);
		info_num = limitJudge("中文信息数组",10,info_num);
		index += getStructure(&data[index],NULL,DAR);
		index += getUnsigned(&data[index],&info_no,DAR);
		for(i=0;i<info_num;i++) {
			infoindex = findInfoIndex(info,info_no);
			if(infoindex!=0xFF) {
				info.chinese_info[infoindex].no = info_no;
				index += getDateTimeS(1,&data[index],(INT8U *)&info.chinese_info[infoindex].releaseData,DAR);
				index += getBool(&data[index],&info.chinese_info[infoindex].readflg,DAR);
				index += getVisibleString(1,201,&data[index],(INT8U *)info.chinese_info[infoindex].info,DAR);
			}
		}
		if(infoindex==0xff)  *DAR = type_mismatch;
		break;
	}
	if(*DAR == success) {
		*DAR = saveCoverClass(oad.OI, 0, (void *) &info, sizeof(CLASS_8003_8004),para_vari_save);
	}
	return index;
}

/*
 * 终端保安定值
 */
int class8100_set(OAD oad, INT8U *data, INT8U *DAR)
{
	CLASS_8100 c8100={};
	ProgramInfo *shareAddr = getShareAddr();
	INT8U	index = 0;

	memcpy(&c8100,&shareAddr->ctrls.c8100,sizeof(CLASS_8100));
	index += getLong64(&data[index],&c8100.v);
	shareAddr->ctrls.c8100.v = c8100.v;
	saveCoverClass(0x8100, 0, (void *) &c8100, sizeof(CLASS_8100),
			para_vari_save);
	asyslog(LOG_WARNING, "设置终端安保定值(%lld)", shareAddr->ctrls.c8100.v);
	buzzerCtrl();
	return index;
}

/*
 * 终端时控时段
 * */
int class8101_set(OAD oad, INT8U *data, INT8U *DAR) {
	ProgramInfo *shareAddr = getShareAddr();
	CLASS_8101 c8101={};
	int	index = 0;

	memset(&c8101, 0x00, sizeof(CLASS_8101));
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
		buzzerCtrl();
	}
	return index;
}

/*
 * 功控告警时间
 * */
int class8102_set(OAD oad, INT8U *data, INT8U *DAR)
{
	CLASS_8102 c8102={};
	int index = 0;
	int i = 0;
	ProgramInfo *shareAddr = getShareAddr();

	memset(&c8102, 0x00, sizeof(CLASS_8102));
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
		buzzerCtrl();
	}
	return index;
}

/*
 * 负荷控制对象接口类 属性3,4,5，控制投入状态
 * */
int class13_attr3_4_5(ALSTATE *alstate,INT8U *data, INT8U *DAR)
{
	OI_698	oi=0;
	int 	i=0,index = 0,unit=0;
	INT8U	alnum = 0;
	ALSTATE tmpstate[MAX_AL_UNIT]={};

	memset(&tmpstate,0,sizeof(ALSTATE)*MAX_AL_UNIT);
	memcpy(tmpstate,alstate,sizeof(ALSTATE)*MAX_AL_UNIT);
	index += getArray(&data[index],&alnum,DAR);
	alnum = limitJudge("控制状态",MAX_AL_UNIT,alnum);
	for(i=0;i<alnum;i++) {
		index += getStructure(&data[index],NULL,DAR);
		index += getOI(1,&data[index],&oi);
		unit  = oi - 0x2301;
		unit = rangeJudge("总加组",unit,0,(MAXNUM_SUMGROUP-1));
		if(unit == -1) {
			*DAR = interface_uncomp;
		}
		tmpstate[unit].name = oi;
		if(data[index] == dtenum) {		//属性3，属性5 状态为enum 类型
			index += getEnum(1,&data[index],&tmpstate[unit].state);
		}else if(data[index] == dtbitstring) {	//属性4 状态为bitstring 类型
			index += getBitString(1,&data[index],&tmpstate[unit].state);
		}
	}
	if(*DAR == success) {
		memcpy(alstate,tmpstate,sizeof(ALSTATE)*MAX_AL_UNIT);
		for(i=0;i<MAX_AL_UNIT;i++) {
			if(alstate[i].name!=0) {
				asyslog(LOG_WARNING, "总加组【%d】OI=%04x,state=%d",i,alstate[i].name,alstate[i].state);
			}
		}
		buzzerCtrl();
	}
	return index;
}

/*
 * 时段功控
 * */
int class8103_set(OAD oad, INT8U *data, INT8U *DAR) {
	int index = 0;
	int i = 0;
	INT8U unit_num = 0;
	ProgramInfo *shareAddr = getShareAddr();

	switch(oad.attflg) {
	case 2:		//时段功控配置单元
		index += getArray(&data[index],&unit_num,DAR);
		fprintf(stderr,"unit_num = %d\n",unit_num);
		for(i=0;i<unit_num;i++) {
			index += class8103_act3(1,oad.attflg,&data[index],DAR);
		}
		break;
	case 3:
		index = class13_attr3_4_5(shareAddr->ctrls.c8103.enable,&data[index],DAR);
		if(*DAR == success) {
			saveCoverClass(oad.OI, 0, (void *) &shareAddr->ctrls.c8103, sizeof(CLASS_8103),	para_vari_save);
		}
		break;
	case 4:
		index = class13_attr3_4_5(shareAddr->ctrls.c8103.output,&data[index],DAR);
		if(*DAR == success) {
			saveCoverClass(oad.OI, 0, (void *) &shareAddr->ctrls.c8103, sizeof(CLASS_8103),	para_vari_save);
		}
		break;
	case 5:
		index = class13_attr3_4_5(shareAddr->ctrls.c8103.overflow,&data[index],DAR);
		if(*DAR == success) {
			saveCoverClass(oad.OI, 0, (void *) &shareAddr->ctrls.c8103, sizeof(CLASS_8103),	para_vari_save);
		}
		break;
	}
	return index;
}

/*
 * 厂休控
 * */
int class8104_set(OAD oad, INT8U *data, INT8U *DAR) {
	int index = 0;
	int i = 0;
	INT8U unit_num = 0;
	ProgramInfo *shareAddr = getShareAddr();

	switch(oad.attflg) {
	case 2:
		index += getArray(&data[index],&unit_num,DAR);
		fprintf(stderr,"unit_num = %d\n",unit_num);
		for(i=0;i<unit_num;i++) {
			index += class8104_act3(1,oad.attflg,&data[index],DAR);
		}
		break;
	case 3:
		index = class13_attr3_4_5(shareAddr->ctrls.c8104.enable,&data[index],DAR);
		if(*DAR == success) {
			saveCoverClass(oad.OI, 0, (void *) &shareAddr->ctrls.c8104, sizeof(CLASS_8104),	para_vari_save);
		}
		break;
	case 4:
		index = class13_attr3_4_5(shareAddr->ctrls.c8104.output,&data[index],DAR);
		if(*DAR == success) {
			saveCoverClass(oad.OI, 0, (void *) &shareAddr->ctrls.c8104, sizeof(CLASS_8104),	para_vari_save);
		}
		break;
	case 5:
		index = class13_attr3_4_5(shareAddr->ctrls.c8104.overflow,&data[index],DAR);
		if(*DAR == success) {
			saveCoverClass(oad.OI, 0, (void *) &shareAddr->ctrls.c8104, sizeof(CLASS_8104),	para_vari_save);
		}
		break;
	}
	return index;
}

/*
 * 营业报停控
 * */
int class8105_set(OAD oad, INT8U *data, INT8U *DAR) {
	int index = 0;
	int i = 0;
	INT8U unit_num = 0;
	ProgramInfo *shareAddr = getShareAddr();

	switch(oad.attflg) {
	case 2:
		index += getArray(&data[index],&unit_num,DAR);
		fprintf(stderr,"unit_num = %d\n",unit_num);
		for(i=0;i<unit_num;i++) {
			index += class8105_act3(1,oad.attflg,&data[index],DAR);
		}
		break;
	case 3:
		index = class13_attr3_4_5(shareAddr->ctrls.c8105.enable,&data[index],DAR);
		if(*DAR == success) {
			saveCoverClass(oad.OI, 0, (void *) &shareAddr->ctrls.c8105, sizeof(CLASS_8105),	para_vari_save);
		}
		break;
	case 4:
		index = class13_attr3_4_5(shareAddr->ctrls.c8105.output,&data[index],DAR);
		if(*DAR == success) {
			saveCoverClass(oad.OI, 0, (void *) &shareAddr->ctrls.c8105, sizeof(CLASS_8105),	para_vari_save);
		}
		break;
	case 5:
		index = class13_attr3_4_5(shareAddr->ctrls.c8105.overflow,&data[index],DAR);
		if(*DAR == success) {
			saveCoverClass(oad.OI, 0, (void *) &shareAddr->ctrls.c8105, sizeof(CLASS_8105),	para_vari_save);
		}
		break;
	}
	return index;
}

/*
 * 购电控
 * */
int class8107_set(OAD oad, INT8U *data, INT8U *DAR)
{
	int		index=0;
	INT8U	unit_num = 0 ,i = 0;
	ProgramInfo *shareAddr = getShareAddr();
	switch(oad.attflg) {
	case 2:		//增加
		index += getArray(&data[index],&unit_num,DAR);
		fprintf(stderr,"unit_num = %d\n",unit_num);
		for(i=0;i<unit_num;i++) {
			index += set_OI810c(oad.attflg,&data[index],shareAddr->ctrls.c8107.list,DAR);
		}
		if(*DAR == success) {
			saveCoverClass(oad.OI, 0, (void *) &shareAddr->ctrls.c8107, sizeof(CLASS_8107),	para_vari_save);
		}
		break;
	case 3:
		index = class13_attr3_4_5(shareAddr->ctrls.c8107.enable,&data[index],DAR);
		if(*DAR == success) {
			saveCoverClass(oad.OI, 0, (void *) &shareAddr->ctrls.c8107, sizeof(CLASS_8107),	para_vari_save);
		}
		break;
	case 4:
		index = class13_attr3_4_5(shareAddr->ctrls.c8107.output,&data[index],DAR);
		if(*DAR == success) {
			saveCoverClass(oad.OI, 0, (void *) &shareAddr->ctrls.c8107, sizeof(CLASS_8107),	para_vari_save);
		}
		break;
	case 5:
		index = class13_attr3_4_5(shareAddr->ctrls.c8107.overflow,&data[index],DAR);
		if(*DAR == success) {
			saveCoverClass(oad.OI, 0, (void *) &shareAddr->ctrls.c8107, sizeof(CLASS_8107),	para_vari_save);
		}
		break;
	}
	return index;
}

/*
 * 月电控
 * */
int class8108_set(OAD oad, INT8U *data, INT8U *DAR) {
	int index = 0;
	int i = 0;
	INT8U unit_num = 0;
	ProgramInfo *shareAddr = getShareAddr();
	switch(oad.attflg) {
	case 2:		//增加
		index += getArray(&data[index],&unit_num,DAR);
		fprintf(stderr,"unit_num = %d\n",unit_num);
		for(i=0;i<unit_num;i++) {
			index += class8108_act3(1,oad.attflg,&data[index],DAR);
		}
		break;
	case 3:
		index = class13_attr3_4_5(shareAddr->ctrls.c8108.enable,&data[index],DAR);
		if(*DAR == success) {
			saveCoverClass(oad.OI, 0, (void *) &shareAddr->ctrls.c8108, sizeof(CLASS_8108),	para_vari_save);
		}
		break;
	case 4:
		index = class13_attr3_4_5(shareAddr->ctrls.c8108.output,&data[index],DAR);
		if(*DAR == success) {
			saveCoverClass(oad.OI, 0, (void *) &shareAddr->ctrls.c8108, sizeof(CLASS_8108),	para_vari_save);
		}
		break;
	case 5:
		index = class13_attr3_4_5(shareAddr->ctrls.c8108.overflow,&data[index],DAR);
		if(*DAR == success) {
			saveCoverClass(oad.OI, 0, (void *) &shareAddr->ctrls.c8108, sizeof(CLASS_8108),	para_vari_save);
		}
	}
	return index;
}

///////////////////////////////////////////////action
int class8000_act127_128(int attr_act, INT8U *data,Action_result *act_ret)
{
	ProgramInfo *shareAddr = getShareAddr();
	if(attr_act == 127) {
		shareAddr->ctrls.c8000.alarmaction = 0x55;
	}if(attr_act == 128) {
		shareAddr->ctrls.c8000.alarmaction = 0;
	}
	asyslog(LOG_WARNING,"遥控方法=%d alarmaction=%02x",attr_act,shareAddr->ctrls.c8000.alarmaction);
	saveCoverClass(0x8000, 0, (void *) &shareAddr->ctrls.c8000, sizeof(CLASS_8000),para_vari_save);
	act_ret->datalen = 1;
	return 1;
}

//分闸
int class8000_act129(int attr_act, INT8U *data,Action_result *act_ret)
{
	CLASS_8000	c8000={};
	INT8U 	ctrlnum = 0, i=0;
	int		ret = 0,index = 0;
	ProgramInfo *shareAddr = getShareAddr();

	memcpy(&c8000,&shareAddr->ctrls.c8000,sizeof(CLASS_8000));
	index += getArray(&data[index],&ctrlnum,&act_ret->DAR);
	ret = rangeJudge("遥控继电器",ctrlnum,0,2);
	if(ret == -1) {
		act_ret->DAR = boundry_over;
		return 0;
	}
	fprintf(stderr,"ctrlnum = %d\n",ctrlnum);
	for(i=0;i<ctrlnum;i++) {
		index += getStructure(&data[index],NULL,&act_ret->DAR);
		index += getOAD(1,&data[index],&c8000.relay_oad[i],&act_ret->DAR);
		index += getUnsigned(&data[index],&c8000.alarmdelay[i],&act_ret->DAR);
		index += getLongUnsigned(&data[index],(INT8U *)&c8000.powerouttime[i]);
		index += getBool(&data[index],&c8000.autoclose[i],&act_ret->DAR);
		c8000.openclose[i] = 0x5555;
		asyslog(LOG_WARNING, "遥控跳闸第%d路[%04x] OAD=%04x_%02x%02x - %d - %d - %d\n",i,c8000.openclose[i],
				c8000.relay_oad[i].OI,c8000.relay_oad[i].attflg,c8000.relay_oad[i].attrindex,
				c8000.alarmdelay[i], c8000.powerouttime[i], c8000.autoclose[i]);
	}
	if(act_ret->DAR == success){
		memcpy(&shareAddr->ctrls.c8000,&c8000,sizeof(CLASS_8000));
		act_ret->DAR = saveCoverClass(0x8000, 0, (void *) &c8000, sizeof(CLASS_8000),para_vari_save);
		buzzerCtrl();
	}
	act_ret->datalen = index;
	return 0;
}

//合闸
int class8000_act130(int attr_act, INT8U *data, Action_result *act_ret) {
	CLASS_8000	c8000={};
	INT8U 	ctrlnum = 0, i=0;
	int		ret = 0,index = 0;
	ProgramInfo *shareAddr = getShareAddr();

	memcpy(&c8000,&shareAddr->ctrls.c8000,sizeof(CLASS_8000));
	index += getArray(&data[index],&ctrlnum,&act_ret->DAR);
	ret = rangeJudge("遥控继电器",ctrlnum,0,2);
	if(ret == -1) {
		act_ret->DAR = boundry_over;
		return 0;
	}
	for(i=0;i<ctrlnum;i++) {
		index += getStructure(&data[index],NULL,&act_ret->DAR);
		index += getOAD(1,&data[index],&c8000.relay_oad[i],&act_ret->DAR);
		index += getEnum(1,&data[index],&c8000.closecmd[i]);
		c8000.openclose[i] = 0xCCCC;
		asyslog(LOG_WARNING, "遥控合闸第%d路[%04x] OAD=%04x_%02x%02x - %d \n",i,c8000.openclose[i],
				c8000.relay_oad[i].OI,c8000.relay_oad[i].attflg,c8000.relay_oad[i].attrindex,c8000.closecmd[i]);
	}
	if(act_ret->DAR == success){
		memcpy(&shareAddr->ctrls.c8000,&c8000,sizeof(CLASS_8000));
		act_ret->DAR = saveCoverClass(0x8000, 0, (void *) &c8000, sizeof(CLASS_8000),para_vari_save);
		buzzerCtrl();
	}
	act_ret->datalen = index;
	return 0;
}

//电表明文合闸
int class8000_act131(int attr_act, INT8U *data, Action_result *act_ret) {
	CLASS_8000	c8000={};
	INT8U 	ctrlnum = 0, i=0;
	int		ret = 0,index = 0;
	ProgramInfo *shareAddr = getShareAddr();

	memcpy(&c8000,&shareAddr->ctrls.c8000,sizeof(CLASS_8000));
	index += getArray(&data[index],&ctrlnum,&act_ret->DAR);
	ret = rangeJudge("遥控继电器",ctrlnum,0,5);
	if(ret == -1) {
		act_ret->DAR = boundry_over;
		return 0;
	}
	for(i=0;i<ctrlnum;i++) {
		index += getStructure(&data[index],NULL,&act_ret->DAR);
		index += getOAD(1,&data[index],&c8000.meter_Ctrl[i].oad,&act_ret->DAR);
		index += getEnum(1,&data[index],&c8000.meter_Ctrl[i].closecmd);
		index += getVisibleString(1,VISIBLE_STRING_LEN,&data[index],c8000.meter_Ctrl[i].passwd,&act_ret->DAR);
		c8000.meter_Ctrl[i].actionflag = 0x55;
		asyslog(LOG_WARNING, "电表明文合闸第%d路 OAD=%04x_%02x%02x - %d -%s\n",i,
				c8000.meter_Ctrl[i].oad.OI,c8000.meter_Ctrl[i].oad.attflg,c8000.meter_Ctrl[i].oad.attrindex,
				c8000.meter_Ctrl[i].closecmd,c8000.meter_Ctrl[i].passwd);
	}
	if(act_ret->DAR == success){
		memcpy(&shareAddr->ctrls.c8000,&c8000,sizeof(CLASS_8000));
		act_ret->DAR = saveCoverClass(0x8000, 0, (void *) &c8000, sizeof(CLASS_8000),para_vari_save);
		buzzerCtrl();
	}
	act_ret->datalen = index;
	return 0;
}

int class8002_act127(int attr_act, INT8U *data,	Action_result *act_ret) {
	CLASS_8002 c8002={};
	int index = 0;
	ProgramInfo *shareAddr = getShareAddr();

	memcpy(&c8002,&shareAddr->ctrls.c8002,sizeof(CLASS_8002));
	index += getStructure(&data[index],NULL,&act_ret->DAR);
	index += getOctetstring(1,3,&data[index],(INT8U *)&c8002.alarmTime[1],(INT8U *)&c8002.alarmTime[0],&act_ret->DAR);
	index += getVisibleString(1,201,&data[index],(INT8U *)c8002.alarmInfo,&act_ret->DAR);
	c8002.state = 1;
	if(act_ret->DAR == success) {
		fprintf(stderr,"告警信息=%d[%s]\n",c8002.alarmInfo[0],&c8002.alarmInfo[1]);
		asyslog(LOG_WARNING, "催费告警投入 alarmTime=%d [%02x %02x %02x] \n",c8002.alarmTime[0],c8002.alarmTime[1],c8002.alarmTime[2],c8002.alarmTime[3]);
		memcpy(&shareAddr->ctrls.c8002,&c8002,sizeof(CLASS_8002));
		act_ret->DAR = saveCoverClass(0x8002, 0, (void *) &c8002, sizeof(CLASS_8002),
				para_vari_save);
		buzzerCtrl();
	}
	act_ret->datalen = index;
	return 0;
}

int class8002_act128(int attr_act, INT8U *data,Action_result *act_ret) {
	ProgramInfo *shareAddr = getShareAddr();
	shareAddr->ctrls.c8002.state = 0;
	asyslog(LOG_WARNING, "催费告警退出 state = %d\n",shareAddr->ctrls.c8002.state);
	act_ret->DAR = saveCoverClass(0x8002, 0, (void *) &shareAddr->ctrls.c8002, sizeof(CLASS_8002),para_vari_save);
	act_ret->datalen = 1;
	buzzerCtrl();
	return 0;
}


/*
 * 添加中文信息
 * */
int class8003_8004_act127(OI_698 OI, INT8U *data,	Action_result *act_ret)
{
	CLASS_8003_8004 info={};
	INT8U	 info_no=0,i=0,infoindex=0;
	int		index=0,ret=0;

	ret = readCoverClass(OI, 0, (void *) &info, sizeof(CLASS_8003_8004),para_vari_save);
	if(ret==-1) {	//文件不存在，初始化中文信息序号
		for(i=0;i<10;i++) {
			info.chinese_info[i].no = 0xFF;
		}
	}
	index += getStructure(&data[index],NULL,&act_ret->DAR);
	index += getUnsigned(&data[index],&info_no,&act_ret->DAR);

	infoindex = findInfoIndex(info,info_no);
	if(infoindex!=0xFF) {
		info.chinese_info[infoindex].no = info_no;
		index += getDateTimeS(1,&data[index],(INT8U *)&info.chinese_info[infoindex].releaseData,&act_ret->DAR);
		index += getVisibleString(1,201,&data[index],(INT8U *)&info.chinese_info[infoindex].info,&act_ret->DAR);
		if(act_ret->DAR == success) {
			act_ret->DAR = saveCoverClass(OI, 0, (void *) &info, sizeof(CLASS_8003_8004),para_vari_save);
		}
	}else {
		act_ret->DAR = boundry_over;
	}
	return 0;
}

/*
 * 删除信息
 * */
int class8003_8004_act128(OI_698 OI, INT8U *data,Action_result *act_ret) {
	CLASS_8003_8004 info={};
	INT8U	 info_no=0,i=0;
	int		index=0,ret = 0;

	ret = readCoverClass(OI, 0, (void *) &info, sizeof(CLASS_8003_8004),para_vari_save);
	if(ret==-1) {
		for(i=0;i<10;i++) {
			info.chinese_info[i].no=0xff;
		}
	}
	index += getUnsigned(&data[index],&info_no,&act_ret->DAR);
	for(i=0;i<10;i++) {
		if(info.chinese_info[i].no==info_no) {
			info.chinese_info[i].no = 0xFF;
			break;
		}
	}
	if(act_ret->DAR == success) {
		act_ret->DAR = saveCoverClass(OI, 0, (void *) &info, sizeof(CLASS_8003_8004),para_vari_save);
	}
	act_ret->datalen = index;
	return 0;
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

int class13_act4_delete(int *alunit,INT8U *data,Action_result *act_ret)
{
	int unit = 0;
	int	ii = 0;
	OI_698 oi = 0;

	ii += getOI(1,&data[ii],&oi);
	unit  = oi - 0x2301;
	unit = rangeJudge("总加组",unit,0,(MAXNUM_SUMGROUP-1));
	if(unit == -1) {
		act_ret->DAR = interface_uncomp;
		return 0;
	}
	*alunit = unit;
	act_ret->DAR = success;
	act_ret->datalen = ii;
	asyslog(LOG_WARNING, "删除控制单元[oi=%04x]",oi);
	return 1;
}

/*
 * 控制投入解除
 * */
int class13_act6_7(int attr_act, ALSTATE *enable,INT8U *data, Action_result *act_ret)
{
	OI_698 oi = 0x00;
	int ii = 0;

	ii += getOI(1,&data[ii],&oi);
	act_ret->datalen = ii;
	asyslog(LOG_WARNING, "控制[%04x] act=%d", oi,attr_act);
	int sindex = oi - 0x2301;
	sindex = rangeJudge("总加组",sindex,0,(MAXNUM_SUMGROUP-1));
	if(sindex == -1) {
		act_ret->DAR = interface_uncomp;
		return 0;
	}
	enable[sindex].name = oi;
	if(attr_act == 6) {			//控制投入
		enable[sindex].state = 1;
	}else if(attr_act == 7) {	//控制解除
		enable[sindex].state = 0;
	}
	buzzerCtrl();
	act_ret->DAR = success;
	return 0;
}
/*
 * 时段功控
 * */
int class8103_act3(int index, int attr_act, INT8U *data, INT8U *DAR)
{
	int	ii = 0;
	OI_698 oi = 0;
	int unit = 0;
	CLASS_8103 c8103={};
	ProgramInfo *shareAddr = getShareAddr();

	memcpy(&c8103,&shareAddr->ctrls.c8103,sizeof(CLASS_8103));
	ii += getStructure(&data[ii],NULL,DAR);
	ii += getOI(1,&data[ii],&oi);
	fprintf(stderr,"oi=%04x\n",oi);
	unit  = oi - 0x2301;
	fprintf(stderr,"unit=%04x\n",unit);
	unit = rangeJudge("总加组",unit,0,(MAXNUM_SUMGROUP-1));
	if(unit == -1) {
		*DAR = interface_uncomp;
	}
	fprintf(stderr,"DAR=%d\n",*DAR);
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
		buzzerCtrl();
	}
	return ii;
}

int class8103_act127(INT8U *data,Action_result *act_ret)
{
	CLASS_8103 c8103={};
	ProgramInfo *shareAddr = getShareAddr();
	OI_698  oi = 0x00;
	int unit = 0;
	int  ii=0;

	memcpy(&c8103,&shareAddr->ctrls.c8103,sizeof(CLASS_8103));
	ii += getStructure(&data[ii],NULL,&act_ret->DAR);
	ii += getOI(1,&data[ii],&oi);
	unit  = oi - 0x2301;
	unit = rangeJudge("总加组",unit,0,(MAXNUM_SUMGROUP-1));
	if(unit == -1) {
		act_ret->DAR = interface_uncomp;
	}
	c8103.plan[unit].index = oi;
	ii += getStructure(&data[ii],NULL,&act_ret->DAR);
	ii += getBitString(1,&data[ii],&c8103.plan[unit].sign);
	ii += getUnsigned(&data[ii],&c8103.plan[unit].numb,&act_ret->DAR);
	if(act_ret->DAR == success) {
		asyslog(LOG_WARNING, "控制方案切换[8103-127],%04x-%02x-%02d", c8103.plan[unit].index, c8103.plan[unit].sign, c8103.plan[unit].numb);
		memcpy(&shareAddr->ctrls.c8103,&c8103,sizeof(CLASS_8103));
		saveCoverClass(0x8103, 0, (void *) &c8103, sizeof(CLASS_8103),para_vari_save);
		buzzerCtrl();
	}
	act_ret->datalen = ii;
	return 0;
}

int class8104_act3(int index, int attr_act, INT8U *data, INT8U *DAR)
{
	CLASS_8104 c8104={};
	int ii = 0;
	OI_698 oi = 0x00;
	int unit = 0;
	ProgramInfo *shareAddr = getShareAddr();

	memset(&c8104, 0x00, sizeof(CLASS_8104));
	memcpy(&c8104,&shareAddr->ctrls.c8104,sizeof(CLASS_8104));

	ii += getStructure(&data[ii],NULL,DAR);
	ii += getOI(1,&data[ii],&oi);
	asyslog(LOG_WARNING, "厂休功控-添加控制单元(OI=%04x)",oi);
	unit = oi-0x2301;
	unit = rangeJudge("总加组",unit,0,(MAXNUM_SUMGROUP-1));
	if(unit == -1) {
		*DAR = interface_uncomp;
	}
	c8104.list[unit].index = oi;
	ii += getLong64(&data[ii],&c8104.list[unit].v);
	fprintf(stderr,"v = %lld\n",c8104.list[unit].v);
	ii += getDateTimeS(1,&data[ii],(INT8U *)&c8104.list[unit].start,DAR);
	ii += getLongUnsigned(&data[ii],(INT8U *)&c8104.list[unit].sustain);
	fprintf(stderr,"sustain = %d\n",c8104.list[unit].sustain);
	ii += getBitString(1,&data[ii],&c8104.list[unit].noDay);
	fprintf(stderr,"noDay = %d\n",c8104.list[unit].noDay);
	if(*DAR == success) {
		c8104.enable[unit].name = oi;
		c8104.output[unit].name = oi;
		c8104.overflow[unit].name = oi;
		memcpy(&shareAddr->ctrls.c8104,&c8104,sizeof(CLASS_8104));
		saveCoverClass(0x8104, 0, (void *) &c8104, sizeof(CLASS_8104),para_vari_save);
		fprintf(stderr, "刷新参数 %lld %d %d\n", shareAddr->ctrls.c8104.list[unit].v, shareAddr->ctrls.c8104.list[unit].sustain,shareAddr->ctrls.c8104.list[unit].noDay);
		buzzerCtrl();
	}
	return ii;
}

int class8105_act3(int index, int attr_act, INT8U *data, INT8U *DAR) {
	OI_698 oi = 0x00;
	int unit = 0;
	int	ii = 0;
	CLASS_8105 c8105={};
	ProgramInfo *shareAddr = getShareAddr();

	memset(&c8105, 0x00, sizeof(CLASS_8105));
	memcpy(&c8105,&shareAddr->ctrls.c8105,sizeof(CLASS_8105));
	ii += getStructure(&data[ii],NULL,DAR);
	ii += getOI(1,&data[ii],&oi);
	asyslog(LOG_WARNING, "营业报停-添加控制单元(OI=%04x)",oi);
	unit = oi-0x2301;
	unit = rangeJudge("总加组",unit,0,(MAXNUM_SUMGROUP-1));
	if(unit == -1) {
		*DAR = interface_uncomp;
	}
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
		saveCoverClass(0x8105, 0, (void *) &c8105, sizeof(CLASS_8105),para_vari_save);
		buzzerCtrl();
	}
	return ii;
}

/*
 * 修改购电控配置单元
 * */
int set_8106_attr6_7(INT8U service,INT8U *data,int *sum_index,ALSTATE *alstate,INT8U *DAR)
{
	int 		index = 0;
	OI_698		oi=0;
	ALSTATE		tmp_alstate={};

	index += getOI(1,&data[index],&oi);
	*sum_index = oi - 0x2301;
	*sum_index = rangeJudge("总加组",*sum_index,0,(MAXNUM_SUMGROUP-1));
	if(*sum_index == -1)  *DAR = obj_unexist;
	asyslog(LOG_WARNING, "控制[%04x] act=%d", oi,service);
	switch(service) {
	case 6:	//投入
//		if(alstate->name == oi) {
//			tmp_alstate.name = oi;
//			tmp_alstate.state = 1;
//		}
		*DAR = obj_undefine;
		break;
	case 7:	//解除
		fprintf(stderr,"alstate.name = %04x oi=%04x\n",alstate->name,oi);
//		if(alstate->name == oi) {		//国网台体测试，未下发功率下浮控投入，先发解除，原程序应答错误，台体不合格。此处去掉判断
			tmp_alstate.name = oi;
			tmp_alstate.state = 0;
//		}else {
//			*DAR = obj_unexist;
//		}
		break;
	}
	if(*DAR == success) {
		memcpy(alstate,&tmp_alstate,sizeof(ALSTATE));
		buzzerCtrl();
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
	case 127: //投入
		if(attr_act == 127) {
			c8106.enable.state = 0x01;
			c8106.enable.name = c8106.index;
			c8106.output.name = c8106.index;
			c8106.overflow.name = c8106.index;
		}
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
		memset(&c8106.list, 0x00, sizeof(DOWN_CTRL));
		break;
	}
	if(*DAR==success) {
		//暂时考虑投入状态、输出状态、告警状态不清除
//		c8106.enable.state = 0;
//		c8106.enable.name = c8106.index;
//		c8106.output.name = c8106.index;
//		c8106.overflow.name = c8106.index;
		fprintf(stderr,"c8106.enable.name = %04x\n",c8106.enable.name);
		memcpy(shmc8106,&c8106,sizeof(CLASS_8106));
		buzzerCtrl();
	}
	return ii;
}


/*
 * 修改购电控配置单元
 * */
int set_OI810c(INT8U service,INT8U *data,BUY_CTRL *oi810c,INT8U *DAR)
{
	int			sum_index = 0;
	int 		index = 0;
	INT8U		stru_num = 0,i=0;
	BUY_CTRL	tmp_oi810c={};

	ProgramInfo *shareAddr = getShareAddr();
	index += getStructure(&data[index],&stru_num,DAR);
//	if(stru_num != 8)	*DAR = interface_uncomp;
	index += getOI(1,&data[index],&tmp_oi810c.index);
	INT16U oi_b = (tmp_oi810c.index<<8) | ((tmp_oi810c.index>>8));
	sum_index = tmp_oi810c.index - 0x2301;
	sum_index = rangeJudge("总加组",sum_index,0,(MAXNUM_SUMGROUP-1));
	if(sum_index == -1) {
		*DAR = interface_uncomp;
	}
	switch(service) {
	case 2:	//set 属性2
	case 3:	//添加
	case 5: //更新
		index += getDouble(&data[index],(INT8U *)&tmp_oi810c.no);
		if(data[index]==dtenum) {
			index += getEnum(1,&data[index],&tmp_oi810c.add_refresh);
		}else if(data[index]==dtunsigned) {
			index += getUnsigned(&data[index],&tmp_oi810c.add_refresh,DAR);
		}
		if(data[index]==dtenum) {	//勘误增加购电类型属性
			index += getEnum(1,&data[index],&tmp_oi810c.type);
		}
		index += getLong64(&data[index],&tmp_oi810c.v);
		index += getLong64(&data[index],&tmp_oi810c.alarm);
		index += getLong64(&data[index],&tmp_oi810c.ctrl);
		index += getEnum(1,&data[index],&tmp_oi810c.mode);
		for(i=0;i<MAX_AL_UNIT;i++) {
//			asyslog(LOG_WARNING,"i = %x sum_index=%d 单号 %ld, 原单号 %ld\n",i,sum_index,tmp_oi810c.no,shareAddr->ctrls.c8107.list[i].no);
			if(tmp_oi810c.no == shareAddr->ctrls.c8107.list[i].no) { //购电单号相同，应返回错误值无效
				*DAR = recharge_reuse;
				break;
			}
		}
		break;
	}
	if(*DAR == success) {
		asyslog(LOG_WARNING, "购电-控制单元【act=%d】[%04x-%d-%d-%d-%lld-%lld-%lld-%d]",service, tmp_oi810c.index, tmp_oi810c.no,
				tmp_oi810c.add_refresh, tmp_oi810c.type, tmp_oi810c.v, tmp_oi810c.alarm, tmp_oi810c.ctrl, tmp_oi810c.mode);
		shareAddr->ctrls.c8107.enable[sum_index].name = tmp_oi810c.index;
		shareAddr->ctrls.c8107.output[sum_index].name = tmp_oi810c.index;
		shareAddr->ctrls.c8107.overflow[sum_index].name = tmp_oi810c.index;
		fprintf(stderr,"enable[%d] = %04x\n",sum_index,shareAddr->ctrls.c8107.enable[sum_index].name);
		memcpy(&oi810c[sum_index],&tmp_oi810c,sizeof(BUY_CTRL));
		if(service == 3 || service == 5) {
			if(tmp_oi810c.add_refresh == 0) {		//
				shareAddr->class23[sum_index].remains += shareAddr->ctrls.c8107.list[sum_index].v;
				asyslog(LOG_WARNING,"追加  remains = %d\n",oi_b,shareAddr->class23[sum_index].remains);
			}else if(tmp_oi810c.add_refresh == 1) {	//
				shareAddr->class23[sum_index].remains = shareAddr->ctrls.c8107.list[sum_index].v;
				asyslog(LOG_WARNING,"添加  remains = %d\n",oi_b,shareAddr->class23[sum_index].remains);
			}
			asyslog(LOG_WARNING,"Event_3202事件 oi_b=%04x  remains = %d\n",oi_b,shareAddr->class23[sum_index].remains);
			Event_3202((INT8U *)&oi_b,2, getShareAddr());
		}
		buzzerCtrl();
	}
	return index;
}

int class8108_act3(int index, int attr_act, INT8U *data, INT8U *DAR)
{
	OI_698 oi = 0x00;
	int ii = 0;
	int unit = 0;
	CLASS_8108 c8108={};
	ProgramInfo *shareAddr = getShareAddr();

	memcpy(&c8108,&shareAddr->ctrls.c8108,sizeof(CLASS_8108));
	ii += getStructure(&data[ii],NULL,DAR);
	ii += getOI(1,&data[ii],&oi);
	unit = oi - 0x2301;
	unit = rangeJudge("总加组",unit,0,(MAXNUM_SUMGROUP-1));
	if(unit == -1) {
		*DAR = interface_uncomp;
	}
	c8108.list[unit].index = oi;
	ii += getLong64(&data[ii],&c8108.list[unit].v);
	ii += getUnsigned(&data[ii],&c8108.list[unit].para,DAR);
	ii += getInteger(&data[ii],&c8108.list[unit].flex,DAR);

	if(*DAR == success) {
		c8108.enable[unit].name = oi;
		c8108.output[unit].name = oi;
		c8108.overflow[unit].name = oi;
		asyslog(LOG_WARNING, "月电-添加控制单元[%d][%04x-%lld-%d-%d]",unit,c8108.list[unit].index,c8108.list[unit].v,c8108.list[unit].para,c8108.list[unit].flex);
		memcpy(&shareAddr->ctrls.c8108,&c8108,sizeof(CLASS_8108));
		saveCoverClass(0x8108, 0, (void *) &c8108, sizeof(CLASS_8108),para_vari_save);
		buzzerCtrl();
	}
	return ii;
}


/*
 * 遥控
 * */
int class8000_act_route(int index, int attr_act, INT8U *data,
		Action_result *act_ret) {
	switch (attr_act) {
	case 127:
	case 128:
		class8000_act127_128(attr_act, data, act_ret);
		break;
	case 129:
		class8000_act129(attr_act, data, act_ret);
		break;
	case 130:
		class8000_act130(attr_act, data, act_ret);
		break;
	case 131:
		class8000_act131(attr_act, data, act_ret);
		break;
	}
	return 1;
}

/*
 * 保电
 * */
int class8001_act_route(int index, int attr_act, INT8U *data,Action_result *act_ret)
{
	ProgramInfo *shareAddr = getShareAddr();
	switch(attr_act) {
	case 127:
		shareAddr->ctrls.c8001.state = 1; //保电投入
		break;
	case 128://解除保电
	case 129://自动保电 //属性3值 !=0  进入自动保电
		shareAddr->ctrls.c8001.state = 0; //解除保电
		break;
	}
	asyslog(LOG_WARNING, "投入保电 state=%d\n",shareAddr->ctrls.c8001.state);
	act_ret->DAR = saveCoverClass(0x8001, 0, (void *) &shareAddr->ctrls.c8001, sizeof(CLASS_8001),para_vari_save);
	act_ret->datalen = 1;
	buzzerCtrl();
	return 1;
}

/*
 * 催费告警
 * */
int class8002_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret) {
	switch (attr_act) {
	case 127:
		class8002_act127(attr_act, data, act_ret);
		break;
	case 128:
		class8002_act128(attr_act, data, act_ret);
		break;
	}
	return 1;
}

/*
 * 一般中文信息/重要中文信息
 * */
int class8003_8004_act_route(OAD oad, INT8U *data, Action_result *act_ret)
{
	switch (oad.attflg) {
	case 127:
		class8003_8004_act127(oad.OI, data, act_ret);
		break;
	case 128:
		class8003_8004_act128(oad.OI, data, act_ret);
		break;
	}
	return 1;
}

/*
 * 时段功控
 * */
int class8103_act_route(int index, int attr_act, INT8U *data,Action_result *act_ret)
{
	ProgramInfo *shareAddr = getShareAddr();
	ALSTATE enable[MAX_AL_UNIT]={};
	int	  unit=0;

	switch (attr_act) {
	case 3://添加控制单元
	case 5://更新控制单元
		act_ret->datalen = class8103_act3(1, attr_act, data, &act_ret->DAR);
		break;
	case 4://删除控制单元
		class13_act4_delete(&unit,data,act_ret);
		if(act_ret->DAR == success) {
			memset(&shareAddr->ctrls.c8103.list[unit], 0x00, sizeof(TIME_CTRL));
			//暂时考虑投入状态、输出状态、告警状态不清除
//			memset(&shareAddr->ctrls.c8103.enable[unit],0,sizeof(ALSTATE));
//			shareAddr->ctrls.c8103.output[unit].name = 0;
//			shareAddr->ctrls.c8103.overflow[unit].name = 0;
			saveCoverClass(0x8103, 0, (void *) &shareAddr->ctrls.c8103, sizeof(CLASS_8103),para_vari_save);
		}
		break;
	case 6://控制投入
	case 7://控制解除
		memcpy(&enable,&shareAddr->ctrls.c8103.enable,sizeof(ALSTATE)*MAX_AL_UNIT);
		class13_act6_7(attr_act,enable,data,act_ret);
		if(act_ret->DAR == success) {
			memcpy(&shareAddr->ctrls.c8103.enable,&enable, sizeof(ALSTATE)*MAX_AL_UNIT);
			saveCoverClass(0x8103, 0, (void *) &shareAddr->ctrls.c8103, sizeof(CLASS_8103),para_vari_save);
		}
		break;
	case 127://时段功控方案切换
		class8103_act127(data, act_ret);
		break;
	}
	return 1;
}
/*
 * 厂休控
 * */
int class8104_act_route(int index, int attr_act, INT8U *data,Action_result *act_ret)
{
	ProgramInfo *shareAddr = getShareAddr();
	ALSTATE enable[MAX_AL_UNIT]={};
	int	  unit=0;
	switch (attr_act) {
	case 3://添加控制单元
	case 5://更新控制单元
		act_ret->datalen = class8104_act3(1, attr_act, data, &act_ret->DAR);
		break;
	case 4://删除控制单元
		class13_act4_delete(&unit,data,act_ret);
		if(act_ret->DAR == success) {
			memset(&shareAddr->ctrls.c8104.list[unit], 0x00, sizeof(FACT_CTRL));
			//暂时考虑投入状态、输出状态、告警状态不清除
//			memset(&shareAddr->ctrls.c8104.enable[unit],0,sizeof(ALSTATE));
//			shareAddr->ctrls.c8104.output[unit].name = 0;
//			shareAddr->ctrls.c8104.overflow[unit].name = 0;
			saveCoverClass(0x8104, 0, (void *) &shareAddr->ctrls.c8104, sizeof(CLASS_8104),para_vari_save);
		}
		break;
	case 6://控制投入
	case 7://控制解除
		memcpy(&enable,&shareAddr->ctrls.c8104.enable,sizeof(ALSTATE)*MAX_AL_UNIT);
		class13_act6_7(attr_act,enable,data,act_ret);
		if(act_ret->DAR == success) {
			memcpy(&shareAddr->ctrls.c8104.enable,&enable, sizeof(ALSTATE)*MAX_AL_UNIT);
			saveCoverClass(0x8104, 0, (void *) &shareAddr->ctrls.c8104, sizeof(CLASS_8104),para_vari_save);
		}
		break;
	}
	return 1;
}

/*
 * 营业报停控
 * */
int class8105_act_route(int index, int attr_act, INT8U *data,Action_result *act_ret) {
	ProgramInfo *shareAddr = getShareAddr();
	ALSTATE enable[MAX_AL_UNIT]={};
	int	  unit=0;

	switch (attr_act) {
	case 3://添加控制单元
	case 5://更新控制单元
		act_ret->datalen = class8105_act3(1, attr_act, data, &act_ret->DAR);
		break;
	case 4://删除控制单元
		class13_act4_delete(&unit,data,act_ret);
		if(act_ret->DAR == success) {
			memset(&shareAddr->ctrls.c8105.list[unit], 0x00, sizeof(STOP_CTRL));
			//暂时考虑投入状态、输出状态、告警状态不清除
//			memset(&shareAddr->ctrls.c8105.enable[unit],0,sizeof(ALSTATE));
//			shareAddr->ctrls.c8105.output[unit].name = 0;
//			shareAddr->ctrls.c8105.overflow[unit].name = 0;
			saveCoverClass(0x8105, 0, (void *) &shareAddr->ctrls.c8105, sizeof(CLASS_8105),para_vari_save);
		}
		break;
	case 6://控制投入
	case 7://控制解除
		memcpy(&enable,&shareAddr->ctrls.c8105.enable,sizeof(ALSTATE)*MAX_AL_UNIT);
		class13_act6_7(attr_act,enable,data,act_ret);
		if(act_ret->DAR == success) {
			memcpy(&shareAddr->ctrls.c8105.enable,&enable, sizeof(ALSTATE)*MAX_AL_UNIT);
			saveCoverClass(0x8105, 0, (void *) &shareAddr->ctrls.c8105, sizeof(CLASS_8105),para_vari_save);
		}
		break;
	}
	return 1;
}
/*
 * 当前功率下浮控
 * */
int class8106_act_route(OAD oad, INT8U *data, Action_result *act_ret)
{
	fprintf(stderr, "class8106_act_route  class8106_act_route %d\n", oad.attflg);
	int sum_index = 0;
	ProgramInfo *shareAddr = getShareAddr();
	ALSTATE enable={};

	switch (oad.attflg) {
	case 3://添加
	case 4://删除
	case 5://更新
	case 127://投入（总加组对象，控制方案）
		act_ret->datalen = class8106_unit(oad.attflg, data, &shareAddr->ctrls.c8106, &act_ret->DAR);
		break;
	case 6:	//控制投入
	case 7: //控制解除
		act_ret->datalen = set_8106_attr6_7(oad.attflg,data,&sum_index,&shareAddr->ctrls.c8106.enable,&act_ret->DAR);
		if(act_ret->DAR == success){
			memcpy(&shareAddr->ctrls.c8106.enable,&enable,sizeof(ALSTATE));
		}
//			pcAlarm.u8b = shareAddr->class23[sum_index].alCtlState.PCAlarmState;
//			pcAlarm.pcstate.power_ctl = 0;
//			shareAddr->class23[sum_index].alCtlState.PCAlarmState = pcAlarm.u8b;
//			shareAddr->class23[sum_index].alCtlState.OutputState = 0;
		break;
	}
	asyslog(LOG_WARNING, "class8106 DAR=%d   index=%04x", act_ret->DAR ,shareAddr->ctrls.c8106.index);
	if(act_ret->DAR == success) {
		asyslog(LOG_WARNING, "class8106 index=%04x    OI=%04x",shareAddr->ctrls.c8106.index,oad.OI);
		saveCoverClass(oad.OI, 0, (void *) &shareAddr->ctrls.c8106, sizeof(CLASS_8106),para_vari_save);
	}
	return 1;
}

/*
 * 购电控
 * */
int class8107_act_route(OAD oad, INT8U *data,Action_result *act_ret)
{
	ProgramInfo *shareAddr = getShareAddr();
	ALSTATE enable[MAX_AL_UNIT]={};
	int	unit=0;

	switch (oad.attflg) {
	case 3:
	case 5:
		act_ret->datalen = set_OI810c(oad.attflg,data,shareAddr->ctrls.c8107.list,&act_ret->DAR);
		break;
	case 4://删除控制单元
		class13_act4_delete(&unit,data,act_ret);
		if(act_ret->DAR == success) {
			memset(&shareAddr->ctrls.c8107.list[unit], 0x00, sizeof(MONTH_CTRL));
			//暂时考虑投入状态、输出状态、告警状态不清除
//			memset(&shareAddr->ctrls.c8107.enable[unit],0,sizeof(ALSTATE));
//			shareAddr->ctrls.c8107.output[unit].name = 0;
//			shareAddr->ctrls.c8107.overflow[unit].name = 0;
			saveCoverClass(oad.OI, 0, (void *) &shareAddr->ctrls.c8107, sizeof(CLASS_8107),para_vari_save);
		}
		break;
	case 6://控制投入
	case 7://控制解除
		memcpy(&enable,&shareAddr->ctrls.c8107.enable,sizeof(ALSTATE)*MAX_AL_UNIT);
		class13_act6_7(oad.attflg,enable,data,act_ret);
		if(act_ret->DAR == success) {
			memcpy(&shareAddr->ctrls.c8107.enable,&enable, sizeof(ALSTATE)*MAX_AL_UNIT);
			saveCoverClass(0x8107, 0, (void *) &shareAddr->ctrls.c8107, sizeof(CLASS_8107),para_vari_save);
		}
		break;
//	case 7: //控制解除
//		act_ret->datalen = set_class13_att367(oad.attflg,data,&sum_index,shareAddr->ctrls.c8107.enable,&act_ret->DAR);
//		fprintf(stderr,"\n *********  oi=%04x   sum_index=%d    dar=%d  ************* \n",oad.OI,sum_index,act_ret->DAR);
//
//		if(act_ret->DAR == success && (sum_index>=0 && sum_index<= MAX_AL_UNIT)) {
//			ecAlarm.u8b = shareAddr->class23[sum_index].alCtlState.ECAlarmState;
//			ecAlarm.ecstate.buy_elec_ctl = 0;
//			shareAddr->class23[sum_index].alCtlState.OutputState = 0x00;
//			shareAddr->class23[sum_index].alCtlState.ECAlarmState = 0x00;
//			shareAddr->class23[sum_index].alCtlState.BuyOutputState = 0x00;
//			fprintf(stderr,"\n ********** sum[%d] alarmstate=%02x   outstate=%02x butout=%03x",sum_index,
//					shareAddr->class23[sum_index].alCtlState.ECAlarmState,
//					shareAddr->class23[sum_index].alCtlState.OutputState,
//					shareAddr->class23[sum_index].alCtlState.BuyOutputState );
//		}
//		break;
	}
	if(act_ret->DAR == success) {
		saveCoverClass(0x8107, 0, (void *) &shareAddr->ctrls.c8107, sizeof(CLASS_8107),para_vari_save);
	}
	return 1;
}
/*
 * 月电控
 * */
int class8108_act_route(int index, int attr_act, INT8U *data,Action_result *act_ret)
{
	ProgramInfo *shareAddr = getShareAddr();
	ALSTATE enable[MAX_AL_UNIT]={};
	int	unit=0;
	switch (attr_act) {
	case 3:
	case 5://更新控制单元
		act_ret->datalen = class8108_act3(1, attr_act, data, &act_ret->DAR);
		break;
	case 4://删除控制单元
		class13_act4_delete(&unit,data,act_ret);
		if(act_ret->DAR == success) {
			memset(&shareAddr->ctrls.c8108.list[unit], 0x00, sizeof(MONTH_CTRL));
			//暂时考虑投入状态、输出状态、告警状态不清除
//			memset(&shareAddr->ctrls.c8108.enable[unit],0,sizeof(ALSTATE));
//			shareAddr->ctrls.c8108.output[unit].name = 0;
//			shareAddr->ctrls.c8108.overflow[unit].name = 0;
			saveCoverClass(0x8108, 0, (void *) &shareAddr->ctrls.c8108, sizeof(CLASS_8108),para_vari_save);
		}
		break;
	case 6://控制投入
	case 7://控制解除
		memcpy(&enable,&shareAddr->ctrls.c8108.enable,sizeof(ALSTATE)*MAX_AL_UNIT);
		class13_act6_7(attr_act,enable,data,act_ret);
		if(act_ret->DAR == success) {
			memcpy(&shareAddr->ctrls.c8108.enable,&enable, sizeof(ALSTATE)*MAX_AL_UNIT);
			saveCoverClass(0x8108, 0, (void *) &shareAddr->ctrls.c8108, sizeof(CLASS_8108),para_vari_save);
		}
		break;
	}
	return 0;
}
