//
// Created by 周立海 on 2017/4/21.
//

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>

#include "class12.h"
#include "class23.h"
#include "dlt698.h"
#include "AccessFun.h"
#include "PublicFunction.h"


int class23_act1(OI_698 oi) {
	int no = oi - 0x2301;
	ProgramInfo *shareAddr = getShareAddr();
	asyslog(LOG_WARNING, "清除所有配置单元(%04x)", no);
	memset(&shareAddr->class23[no], 0x00, sizeof(CLASS23));
	saveCoverClass(oi, 0, &shareAddr->class23[no], sizeof(CLASS23), para_vari_save);
	//数据为NULL，占用1个字节   23 01 01 00 00（NULL）
	return 1;
}

int class23_act3_4(OAD oad, INT8U* data, Action_result *act_ret)
{
	int  index = 0;
	AL_UNIT al_unit={};
	int no = oad.OI - 0x2301;
	INT8U	unitnum =0, i=0;

	ProgramInfo *shareAddr = getShareAddr();
	asyslog(LOG_WARNING, "添加一个配置单元(%04x)", no);

	if(oad.attflg == 4) {
		index += getArray(&data[index],&unitnum,&act_ret->DAR);
		unitnum = limitJudge("总加配置单元",MAX_AL_UNIT,unitnum);
	}else if(oad.attflg == 3) {
		unitnum = 1;
	}
	for(i=0;i<unitnum;i++) {
		index += getStructure(&data[index],NULL,&act_ret->DAR);
		index += getOctetstring(1,&data[index],(INT8U *)&al_unit.tsa,&act_ret->DAR);
		index += getEnum(1,&data[index],&al_unit.al_flag);
		index += getEnum(1,&data[index],&al_unit.cal_flag);
		if (shareAddr->class23[no].allist[i].tsa.addr[0] == 0x00 && act_ret->DAR == success) {
			memcpy(&shareAddr->class23[no].allist[i], &al_unit,	sizeof(AL_UNIT));
			asyslog(LOG_WARNING, "添加一个配置单元，地址(%d)", i);
		}
	}
	saveCoverClass(oad.OI, 0, &shareAddr->class23[no], sizeof(CLASS23), para_vari_save);
	act_ret->datalen = index;
	return 0;
//	if (data[0] != 0x02 || data[1] != 0x03 || data[2] != 0x55) {
//		return 0;
//	}
//
//	int tsa_len = data[3];
//	int data_index = 4;
//
//	if (tsa_len > 17) {
//		return 0;
//	}
//	al_unit.tsa.addr[0] = data[3];
//
//	for (int i = 0; i < tsa_len; ++i) {
//		al_unit.tsa.addr[1 + i] = data[data_index];
//		data_index++;
//	}
//
//	if (data[data_index] != 0x16 || data[data_index + 2] != 0x16) {
//		return 0;
//	}
//
//	al_unit.al_flag = data[data_index + 1];
//	al_unit.cal_flag = data[data_index + 3];
//
//	asyslog(LOG_WARNING, "添加一个配置单元(%d)", index);
//	ProgramInfo *shareAddr = getShareAddr();
//	for (int i = 0; i < MAX_AL_UNIT; i++) {
//		if (shareAddr->class23[index].allist[i].tsa.addr[0] == 0x00) {
//			memcpy(&shareAddr->class23[index].allist[i], &al_unit,
//					sizeof(AL_UNIT));
//			asyslog(LOG_WARNING, "添加一个配置单元，地址(%d)", i);
//			break;
//		}
//	}
//	return 0;
}

int class23_set_attr2(OI_698 oi,INT8U *data, INT8U *DAR, CLASS23 *memClass23)
{
	int  index = 0;
	CLASS23		class23[MAX_AL_UNIT]={};
	INT8U	unitnum = 0,i=0;
	int  no = oi - 0x2301;

	memcpy(class23,memClass23,sizeof(class23));
//	readCoverClass(oi, 0, &class23, sizeof(CLASS23), para_vari_save);
	index += getArray(&data[index],&unitnum,DAR);
	fprintf(stderr,"unitnum = %d  DAR=%d\n",unitnum,*DAR);
	unitnum = limitJudge("总加组配置单元",MAX_AL_UNIT,unitnum);
	for(i=0;i<unitnum;i++) {
		index += getStructure(&data[index],NULL,DAR);
		index += getOctetstring(1,&data[index],(INT8U *)&class23[no].allist[i].tsa,DAR);
		index += getEnum(1,&data[index],&class23[no].allist[i].al_flag);
		index += getEnum(1,&data[index],&class23[no].allist[i].cal_flag);
	}

	fprintf(stderr,"1111unitnum = %d  DAR=%d oi=%04x\n",unitnum,*DAR,oi);
	if(*DAR==success) {
		memcpy(memClass23,class23,sizeof(class23));
		saveCoverClass(oi, 0, &class23[no], sizeof(CLASS23), para_vari_save);
	}
	return index;
}

int class23_set_attr13(OI_698 oi,INT8U *data, INT8U *DAR, CLASS23 *memClass23)
{
	int  index = 0;
	CLASS23		class23[MAX_AL_UNIT]={};
	int  no = oi - 0x2301;

	memcpy(class23,memClass23,sizeof(class23));
//	readCoverClass(oi, 0, &class23, sizeof(CLASS23), para_vari_save);
	index += getUnsigned(&data[index],&class23[no].aveCircle,DAR);
	if(*DAR==success) {
		memcpy(memClass23,class23,sizeof(class23));
		saveCoverClass(oi, 0, &class23[no], sizeof(CLASS23), para_vari_save);
	}
	return index;
}

int class23_set_attr14_15(OAD oad,INT8U *data, INT8U *DAR, CLASS23 *memClass23)
{
	int  index = 0;
	CLASS23		class23[MAX_AL_UNIT]={};
	int  no = oad.OI - 0x2301;

	memcpy(class23,memClass23,sizeof(class23));
//	readCoverClass(oad.OI, 0, &class23, sizeof(CLASS23), para_vari_save);
	switch(oad.attflg){
	case 14:
		index += getBitString(1,&data[index],&class23[no].pConfig);
		if(*DAR==success) {
			memcpy(memClass23,class23,sizeof(class23));
			saveCoverClass(oad.OI, 0, &class23[no], sizeof(CLASS23), para_vari_save);
		}
		break;
	case 15:
		index += getBitString(1,&data[index],&class23[no].eConfig);
		if(*DAR==success) {
			memcpy(memClass23,class23,sizeof(class23));
			saveCoverClass(oad.OI, 0, &class23[no], sizeof(CLASS23), para_vari_save);
		}
		break;
	}
	return index;
}

//TODO: 设置某一项属性的处理
int class23_set_attr16(OAD oad,INT8U *data, INT8U *DAR, CLASS23 *memClass23)
{
	int  index = 0;
	CLASS23		class23[MAX_AL_UNIT]={};
	int  no = oad.OI - 0x2301;

	memcpy(class23,memClass23,sizeof(class23));
//	readCoverClass(oad.OI, 0, &class23, sizeof(CLASS23), para_vari_save);
	index += getStructure(&data[index],NULL,DAR);
	index += getUnsigned(&data[index],&class23[no].alConState.index,DAR);
	index += getBitString(1,&data[index],&class23[no].alConState.enable_flag);
	index += getBitString(1,&data[index],&class23[no].alConState.PCState);
	index += getBitString(1,&data[index],&class23[no].alConState.ECState);
	index += getBitString(1,&data[index],&class23[no].alConState.PTrunState);
	index += getBitString(1,&data[index],&class23[no].alConState.ETrunState);
	if(*DAR==success) {
		memcpy(memClass23,class23,sizeof(class23));
		saveCoverClass(oad.OI, 0, &class23[no], sizeof(CLASS23), para_vari_save);
	}
	return index;
}

int class23_set_attr17(OAD oad,INT8U *data, INT8U *DAR, CLASS23 *memClass23)
{
	int  index = 0;
	CLASS23		class23[MAX_AL_UNIT]={};
	int  no = oad.OI - 0x2301;

	memcpy(class23,memClass23,sizeof(class23));
//	readCoverClass(oad.OI, 0, &class23, sizeof(CLASS23), para_vari_save);
	index += getStructure(&data[index],NULL,DAR);
	index += getLong64(&data[index],&class23[no].alCtlState.v);
	index += getInteger(&data[index],&class23[no].alCtlState.Downc,DAR);
	index += getBitString(1,&data[index],&class23[no].alCtlState.OutputState);
	index += getBitString(1,&data[index],&class23[no].alCtlState.MonthOutputState);
	index += getBitString(1,&data[index],&class23[no].alCtlState.BuyOutputState);
	index += getBitString(1,&data[index],&class23[no].alCtlState.PCAlarmState);
	index += getBitString(1,&data[index],&class23[no].alCtlState.ECAlarmState);
	if(*DAR==success) {
		memcpy(memClass23,class23,sizeof(class23));
		saveCoverClass(oad.OI, 0, &class23[no], sizeof(CLASS23), para_vari_save);
	}
	return index;
}



int class23_get_2(OAD oad, INT8U index, INT8U *buf, int *len)
{
	ProgramInfo *shareAddr = getShareAddr();
	INT8U	unit = 0;
	INT8U	i=0;
	//总加组配置单元个数：有TSA认为有效
	unit = 0;
	for(i=0;i<MAX_AL_UNIT;i++) {
		if(shareAddr->class23[index].allist[i].tsa.addr[0]!=0) {
			unit++;
		}else break;
	}
	fprintf(stderr,"总加组配置单元个数 = %d \n",unit);
	*len = 0;
	fprintf(stderr,"attrindex = %d \n",oad.attrindex);
	if(oad.attrindex == 0) {
		if(unit) {
			*len += create_array(&buf[*len],unit);
			for(i=0;i<unit;i++) {
				*len += create_struct(&buf[*len],3);
				*len += fill_TSA(&buf[*len],&shareAddr->class23[index].allist[i].tsa.addr[1],shareAddr->class23[index].allist[i].tsa.addr[0]);
				*len += fill_enum(&buf[*len],shareAddr->class23[index].allist[i].al_flag);
				*len += fill_enum(&buf[*len],shareAddr->class23[index].allist[i].cal_flag);
			}
		}else {
			buf[*len] = 0;		//无配置单元，上送0
			*len = 1;
		}
	}
	fprintf(stderr," len = %d\n",*len);
	return 1;
}

int class23_get_long64(OAD oad, INT64S val, INT8U *buf, int *len)
{
	*len = 0;
	if(oad.attrindex == 0) {
		*len += fill_long64(&buf[*len], val);
	}
	return 1;
}

int class23_get_unsigned(OAD oad, INT8U val, INT8U *buf, int *len)
{
	*len = 0;
	if(oad.attrindex == 0) {
		*len += fill_unsigned(&buf[*len], val);
	}
	return 1;
}

int class23_get_bitstring(OAD oad, INT8U val, INT8U *buf, int *len)
{
	*len = 0;
	if(oad.attrindex == 0) {
		*len += fill_bit_string(&buf[*len],8,&val);
	}
	return 1;
}

int class23_get_7_8_9_10(OAD oad, INT64S energy_all,INT64S *energy,INT8U *buf, int *len){
	INT64S total_energy[MAXVAL_RATENUM + 1];
	INT8S	unit=0,i=0;

	*len = 0;
	total_energy[0] = 0;
	///TODO: 总电量分相需要累加，不能使用内存energy_all
	for (i = 0; i < MAXVAL_RATENUM; i++){
		total_energy[0] += energy[i];
	}
//	total_energy[0] = energy_all;
	for (i = 0; i < MAXVAL_RATENUM; i++){
		total_energy[i+1] = energy[i];
	}
	fprintf(stderr, "class23_get_7 %lld oad=%04x_%02x_%02x \n", total_energy[0],oad.OI,oad.attflg,oad.attrindex);

	if (oad.attrindex == 0) {
		unit = MAXVAL_RATENUM + 1;	//总及n个费率
		*len = 0;
		*len += create_array(&buf[*len],unit);
		for(i=0;i<unit;i++) {
			*len += fill_long64(&buf[*len], total_energy[i]);
		}
	}else {
		unit = oad.attrindex - 1;
		unit = rangeJudge("电能量",unit,0,(MAXVAL_RATENUM-1));
		if(unit != -1) {
			*len = 0;
			*len += fill_long64(&buf[*len], total_energy[unit]);
		}else {
			buf[*len++] = 0;		//NULL
		}
		return 1;
	}
	return 0;
}

int class23_get_16(OAD oad, ALCONSTATE alConState, INT8U *buf, int *len) {
	*len = 0;
	*len += create_struct(&buf[*len],6);
	*len += fill_unsigned(&buf[*len],alConState.index);
	*len += fill_bit_string(&buf[*len],8,&alConState.enable_flag);
	*len += fill_bit_string(&buf[*len],8,&alConState.PCState);
	*len += fill_bit_string(&buf[*len],8,&alConState.ECState);
	*len += fill_bit_string(&buf[*len],8,&alConState.PTrunState);
	*len += fill_bit_string(&buf[*len],8,&alConState.ETrunState);
	return 1;
}

int class23_get_17(OAD oad, ALCTLSTATE alCtlState, INT8U *buf, int *len) {
	*len = 0;
	*len += create_struct(&buf[*len],7);
	*len += fill_long64(&buf[*len],alCtlState.v);
	*len += fill_integer(&buf[*len],alCtlState.Downc);
	*len += fill_bit_string(&buf[*len],8,&alCtlState.OutputState);
	*len += fill_bit_string(&buf[*len],8,&alCtlState.MonthOutputState);
	*len += fill_bit_string(&buf[*len],8,&alCtlState.BuyOutputState);
	*len += fill_bit_string(&buf[*len],8,&alCtlState.PCAlarmState);
	*len += fill_bit_string(&buf[*len],8,&alCtlState.ECAlarmState);
	return 1;
}

int class23_get(OAD oad, INT8U *sourcebuf, INT8U *buf, int *len) {
	asyslog(LOG_WARNING, "召唤总加组属性(%d)", oad.attflg);
	ProgramInfo *shareAddr = getShareAddr();
	int index = oad.OI - 0x2301;

	index = rangeJudge("总加组",index,0,(MAXNUM_SUMGROUP-1));
	if(index == -1)  return 0;		//返回值len为？？

	switch (oad.attflg) {
	case 2:
		return class23_get_2(oad, index, buf, len);
	case 3:
		return class23_get_long64(oad, shareAddr->class23[index].p, buf, len);
	case 4:
		return class23_get_long64(oad, shareAddr->class23[index].q, buf, len);
	case 5:
		return class23_get_long64(oad, shareAddr->class23[index].TaveP, buf, len);
	case 6:
		return class23_get_long64(oad, shareAddr->class23[index].TaveQ, buf, len);
	case 7:
		return class23_get_7_8_9_10(oad, shareAddr->class23[index].DayPALL, shareAddr->class23[index].DayP, buf, len);
	case 8:
		return class23_get_7_8_9_10(oad, shareAddr->class23[index].DayQALL, shareAddr->class23[index].DayQ, buf, len);
	case 9:
		return class23_get_7_8_9_10(oad, shareAddr->class23[index].MonthPALL, shareAddr->class23[index].MonthP, buf, len);
	case 10:
		return class23_get_7_8_9_10(oad, shareAddr->class23[index].MonthQALL, shareAddr->class23[index].MonthQ, buf, len);
	case 11:
		return class23_get_long64(oad, shareAddr->class23[index].remains, buf, len);
	case 12:
		return class23_get_long64(oad, shareAddr->class23[index].DownFreeze, buf, len);
	case 13:
		return class23_get_unsigned(oad, shareAddr->class23[index].aveCircle, buf, len);
	case 14:
		return class23_get_bitstring(oad, shareAddr->class23[index].pConfig, buf, len);
	case 15:
		return class23_get_bitstring(oad, shareAddr->class23[index].eConfig, buf, len);
	case 16:	//总加组控制设置状态
		return class23_get_16(oad, shareAddr->class23[index].alConState, buf, len);
	case 17:	//总加组当前控制状态
		return class23_get_17(oad, shareAddr->class23[index].alCtlState, buf, len);
	case 18:
		return get_Scaler_Unit(oad, shareAddr->class23[index].su, buf, len);
	}
	return 1;
}

int class23_set(OAD oad, INT8U *data, INT8U *DAR)
{
	asyslog(LOG_WARNING, "修改总加组(%04x)属性(%d)", oad.OI,oad.attflg);
	ProgramInfo *shareAddr = getShareAddr();
	int index = oad.OI - 0x2301;
	index = rangeJudge("总加组",index,0,(MAXNUM_SUMGROUP-1));
	if(index == -1)  {
		*DAR = obj_unexist;
		return 0;		//返回值len为？？
	}

	switch (oad.attflg) {
	case 2:
		return class23_set_attr2(oad.OI,data,DAR,shareAddr->class23);
	case 13:
		return class23_set_attr13(oad.OI,data,DAR,shareAddr->class23);
//		if (data[0] != 0x17) {
//			return 0;
//		}
//		shareAddr->class23[index].aveCircle = data[1];
		break;
	case 14:
		return class23_set_attr14_15(oad,data,DAR,shareAddr->class23);
	case 15:
		return class23_set_attr14_15(oad,data,DAR,shareAddr->class23);
	case 16:
		return class23_set_attr16(oad,data,DAR,shareAddr->class23);
	case 17:
		return class23_set_attr17(oad,data,DAR,shareAddr->class23);
	case 18://单位及换算
		break;
	}
	return 0;
}

int class23_selector(OAD oad, INT8U *data, Action_result *act_ret)
{
	int index = oad.OI - 0x2301;
	index = rangeJudge("总加组",index,0,(MAXNUM_SUMGROUP-1));
	if(index == -1)  {
		act_ret->DAR = obj_unexist;
		return 0;
	}
	switch (oad.attflg) {
	case 1:		//清空总加组
		act_ret->datalen = class23_act1(oad.OI);
		break;
	case 3:		//添加一个总加配置单元
		class23_act3_4(oad, data, act_ret);
		break;
	case 4:		//批量添加总加配置单元
		class23_act3_4(oad, data, act_ret);
		break;
	}
	return 0;
}
