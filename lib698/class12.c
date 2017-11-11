/*
 * class12.c
 *
 *  Created on: 2017-7-3
 *      Author: zhoulihai
 */
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>

#include "class12.h"
#include "PublicFunction.h"
#include "dlt698.h"
#include "dlt698def.h"
#include "AccessFun.h"
#include "../include/StdDataType.h"

//int class12_act3(int index, INT8U* data) {
//	if (data[0] != 0x02 || data[1] != 0x03) {
//		return 0;
//	}
//	if (data[2] != 0x51) {
//		return 0;
//	}
//
//	ProgramInfo *shareAddr = getShareAddr();
//	for (int i = 0; i < 12; i++) {
//		if (shareAddr->class12[index].unit[i].no.OI == 0x00) {
//			shareAddr->class12[index].unit[i].no.OI = data[3] * 256 + data[4];
//			asyslog(LOG_INFO, "添加脉冲计量配置单元 %04x",
//					shareAddr->class12[index].unit[i].no.OI);
//			shareAddr->class12[index].unit[i].no.attflg = data[5];
//			shareAddr->class12[index].unit[i].no.attrindex = data[6];
//
//			shareAddr->class12[index].unit[i].conf = data[8];
//			shareAddr->class12[index].unit[i].k = data[10] * 256 + data[11];
//			break;
//		}
//	}
//
//	for (int i = 0; i < 2; ++i) {
//		saveCoverClass(0x2401 + i, 0, &shareAddr->class12[i],
//				sizeof(CLASS12), para_vari_save);
//	}
//
//	return 0;
//}

/*
 * type =  ACTION_REQUEST  07
 * type =  SET_REQUEST  06
 */
int class12_act3_attr4(INT8U type,OI_698 oi,INT8U *data, INT8U *DAR, CLASS12 *memClass12)
{
	CLASS12	 class12[MAX_PULSE_NUM]={};
	int  index = 0;
	int  no = oi - 0x2401;
	int  i = 0;
	INT8U	unitnum=0;

	INT8U conf=0;
	OAD oad;
	INT32U k=0;
	int addflg=0;
	int firstzero=-1;

	fprintf(stderr,"\n=============  3   no=%d   oi=%04x",no,oi);

	memcpy(class12,memClass12,sizeof(class12));
//	readCoverClass(oi, 0, &class12, sizeof(CLASS12), para_vari_save);
	if(type == SET_REQUEST) {	//set

		index += getArray(&data[index],&unitnum,DAR);
		fprintf(stderr,"\n=============  1   unitnum=%d",unitnum);
		unitnum = limitJudge("脉冲配置单元",MAX_PULSE_UNIT,unitnum);
		fprintf(stderr,"\n=============  2   unitnum=%d",unitnum);
		for (i = 0; i < unitnum; i++) {
			index += getStructure(&data[index],NULL,DAR);
			index += getOAD(1,&data[index],&class12[no].unit[i].no,DAR);
			index += getEnum(1,&data[index],&class12[no].unit[i].conf);
			index += getLongUnsigned(&data[index],(INT8U *)&class12[no].unit[i].k);
		}
	}else if(type == ACTION_REQUEST){ 	//action

		index += getStructure(&data[index],NULL,DAR);
		index += getOAD(1,&data[index],&oad,DAR);
		index += getEnum(1,&data[index],&conf);
		index += getLongUnsigned(&data[index],(INT8U *)&k);
		addflg = 0;
		for (i = 0; i < MAX_PULSE_UNIT; i++)
		{
			fprintf(stderr,"\n===== 脉冲单元%d   oi=%04x-%02x%02x   k=%d\n",i,class12[no].unit[i].no.OI,class12[no].unit[i].no.attflg,class12[no].unit[i].no.attrindex,class12[no].unit[i].k);
			if (class12[no].unit[i].no.OI ==0 && firstzero==-1)
				firstzero = i;
			if(class12[no].unit[i].no.OI == oad.OI  &&
			   class12[no].unit[i].no.attflg == oad.attflg &&
			   class12[no].unit[i].no.attrindex == oad.attrindex)
			{	//添加一个新的控制单元
				fprintf(stderr,"\n==== 添加到 i=%d",i);
				class12[no].unit[i].conf = conf;
				class12[no].unit[i].k = k;
				class12[no].unit[i].no = oad;
				addflg = 1;
//				index += getStructure(&data[index],NULL,DAR);
//				index += getOAD(1,&data[index],&class12[no].unit[i].no,DAR);
//				index += getEnum(1,&data[index],&class12[no].unit[i].conf);
//				index += getLongUnsigned(&data[index],(INT8U *)&class12[no].unit[i].k);
				break;
			}
		}
		if (addflg==0)
		{
			if (firstzero<0 || firstzero>=MAX_PULSE_UNIT)
				firstzero = 0;
			class12[no].unit[firstzero].conf = conf;
			class12[no].unit[firstzero].k = k;
			class12[no].unit[firstzero].no = oad;
		}
	}
	if(*DAR == success) {

		memcpy(memClass12,class12,sizeof(class12));		//更新内存
		fprintf(stderr,"\n------------------==========------------ 脉冲1 （K）=%d  脉冲2（K）=%d ",memClass12->unit[0].k, memClass12->unit[1].k);
		saveCoverClass(oi, 0, &class12[no], sizeof(CLASS12), para_vari_save);
	}
	return index;
}

/*
 * 删除脉冲输入单元
 * */
int class12_act4(OI_698 oi,INT8U *data, INT8U *DAR, CLASS12 *memClass12)
{
	CLASS12	 class12[MAX_PULSE_NUM]={};
	int  index = 0;
	int  no = oi - 0x2401;
	int  i = 0;
	OAD	 port_oad={};

	memcpy(class12,memClass12,sizeof(class12));
//	readCoverClass(oi, 0, &class12, sizeof(CLASS12), para_vari_save);
	index += getOAD(1,&data[index],&port_oad,DAR);
	syslog(LOG_NOTICE,"删除脉冲单元[%04x_%02x%02x]",port_oad.OI,port_oad.attflg,port_oad.attrindex);
	if(*DAR == success) {
		for (i = 0; i < MAX_PULSE_UNIT; i++) {
			if(memcmp(&port_oad,&class12[no].unit[i].no.OI,sizeof(OAD))==0) {
				memset(&class12[no].unit[i],0,sizeof(PULSEUNIT));
				memcpy(memClass12,class12,sizeof(class12));
				saveCoverClass(oi, 0, &class12[no], sizeof(CLASS12), para_vari_save);
				break;
			}
		}
	}// TODO：未找到要删除的脉冲输入单元的端口号是否返回正确？
	return index;
}

int class12_router(OAD oad, INT8U *data, int *datalen, INT8U *DAR)
{
	int index = oad.OI - 0x2401;
	index = rangeJudge("脉冲",index,0,(MAX_PULSE_NUM-1));
	if(index == -1) {
		*datalen = 0;
		*DAR = obj_unexist;
		return 0;
	}
	ProgramInfo *shareAddr = getShareAddr();
	switch (oad.attflg) {
	case 3:
		*datalen = class12_act3_attr4(ACTION_REQUEST,oad.OI,data,DAR,shareAddr->class12);
		break;
	case 4:
		*datalen = class12_act4(oad.OI,data,DAR,shareAddr->class12);
		break;
	}
	return 0;
}


int class12_set_attr2(OI_698 oi,INT8U *data, INT8U *DAR, CLASS12 *memClass12)
{
	CLASS12	 class12[MAX_PULSE_NUM]={};
	int  index = 0;
	int  no = oi - 0x2401;

	memcpy(class12,memClass12,sizeof(class12));		//从内存或者，防止内存未及时更新到文件中
//	readCoverClass(oi, 0, &class12, sizeof(CLASS12), para_vari_save);
	if(data[index] == dtoctetstring) {
		index += getOctetstring(1,TSA_LEN-1,&data[index],(INT8U *)&class12[no].addr[1],&class12[no].addr[0],DAR);
	}else {
		index += getTSA(1,&data[index],(INT8U *)&class12[no].addr[0],DAR);
	}
	if(*DAR == success) {
		memcpy(memClass12,class12,sizeof(class12));
		saveCoverClass(oi, 0, &class12[no], sizeof(CLASS12), para_vari_save);
	}
	return index;
}

int class12_set_attr3(OI_698 oi,INT8U *data, INT8U *DAR, CLASS12 *memClass12)
{
	CLASS12	 class12[MAX_PULSE_NUM]={};
	int  index = 0;
	int  no = oi - 0x2401;

	memcpy(class12,memClass12,sizeof(class12));
//	readCoverClass(oi, 0, &class12, sizeof(CLASS12), para_vari_save);
	index += getStructure(&data[index],NULL,DAR);
	index += getLongUnsigned(&data[index],(INT8U *)&class12[no].pt);
	index += getLongUnsigned(&data[index],(INT8U *)&class12[no].ct);
	if(*DAR == success) {
		memcpy(memClass12,class12,sizeof(class12));
		saveCoverClass(oi, 0, &class12[no], sizeof(CLASS12), para_vari_save);
	}
	return index;
}

//int class2401_set_attr_3(int index, OAD oad, INT8U *data, INT8U *DAR) {
//	if (data[0] != 0x02) {
//		return 0;
//	}
//	if (data[1] != 0x02) {
//		return 0;
//	}
//
//	if (data[2] != 0x12 || data[5] != 0x12) {
//		return 0;
//	}
//	int pt = data[3] * 255 + data[4];
//	int ct = data[6] * 255 + data[7];
//	asyslog(LOG_WARNING, "修改互感器倍率%d-%d", pt, ct);
//	ProgramInfo *shareAddr = getShareAddr();
//	shareAddr->class12[index].pt = pt;
//	shareAddr->class12[index].ct = ct;
//
//	for (int i = 0; i < 2; ++i) {
//		saveCoverClass(0x2401 + i, 0, &shareAddr->class12[i],
//				sizeof(CLASS12), para_vari_save);
//	}
//	return 0;
//}

int class12_set(OAD oad, INT8U *data, INT8U *DAR)
{
	asyslog(LOG_WARNING, "设置OI=%04x,属性 %02x ", oad.OI,oad.attflg);
	int index = oad.OI - 0x2401;
	index = rangeJudge("脉冲",index,0,(MAX_PULSE_NUM-1));
	if(index == -1) {
		*DAR = obj_unexist;
		return 0;
	}
	ProgramInfo *shareAddr = getShareAddr();
	int datalen=0;
	switch (oad.attflg) {
	case 2:
		datalen = class12_set_attr2(oad.OI, data, DAR, shareAddr->class12);
		break;
	case 3:
		datalen = class12_set_attr3(oad.OI, data, DAR, shareAddr->class12);
		break;
	case 4:
		datalen = class12_act3_attr4(SET_REQUEST,oad.OI,data,DAR,shareAddr->class12);
		break;
	case 19:	//单位换算
		break;
	}

	return datalen;
}

int class12_get_3(OI_698 oi, INT32U pt, INT32U ct, INT8U *buf, int *len)
{
	*len = 0;
	*len += create_struct(&buf[*len],2);
	*len += fill_long_unsigned(&buf[*len],pt);
	*len += fill_long_unsigned(&buf[*len],ct);
	return 1;
}

int class12_get_4(OI_698 oi, PULSEUNIT *pluseunit, INT8U *buf, int *len)
{
	INT8U	unitnum = 0,i=0;
	for(i=0;i<MAX_PULSE_UNIT;i++) {
		if(pluseunit[i].no.OI != 0)  unitnum++;
		else break;
	}
	*len = 0;
	if(unitnum) {
		*len += create_array(&buf[*len],unitnum);
		for(i=0;i<unitnum;i++) {
			*len += create_struct(&buf[*len],3);
			*len += create_OAD(1,&buf[*len],pluseunit[i].no);
			*len += fill_enum(&buf[*len],pluseunit[i].conf);
			*len += fill_long_unsigned(&buf[*len],pluseunit[i].k);
		}
	}else {
		buf[*len] = 0;		//无配置单元，上送0
		*len = 1;
	}
	return 1;
}

int class12_get_5_6(OI_698 oi, INT32S val, INT8U *buf, int *len) {
	*len = 0;
	*len += fill_double_long(&buf[*len], val);
	return 1;
}

int class12_get_double_long_unsigned(OAD oad, INT32U *val, INT8U *buf, int *len) {
	INT32U total[MAXVAL_RATENUM + 1] = {};
	INT8S	unit=0;

	fprintf(stderr, "class12_get_7-18\n");

	int i = 0;
	for (i = 0; i < MAXVAL_RATENUM; i++){
		total[0] += val[i];
	}
	for (i = 0; i < MAXVAL_RATENUM; i++){
		total[i+1] = val[i];
	}

	if (oad.attrindex == 0) {
		unit = MAXVAL_RATENUM + 1;	//总及n个费率
		*len = 0;
		*len += create_array(&buf[*len],unit);
		for(i=0;i<unit;i++) {
			*len += fill_double_long_unsigned(&buf[*len], total[i]);
		}
	}else {
		unit = oad.attrindex - 1;
		unit = rangeJudge("脉冲电量",unit,0,MAXVAL_RATENUM);
		if(unit != -1) {
			*len = 0;
			*len += fill_double_long_unsigned(&buf[*len], total[unit]);
		}else {
			buf[*len++] = 0;		//NULL
		}
	}
	return 1;
}

int get_Scaler_Unit(OAD oad,Scaler_Unit *su, INT8U *buf, int *len)
{
	INT8U	i=0;
	INT8U	unitnum=0;

	if(oad.OI>=0x2301 && oad.OI<=0x2308) {
		unitnum = 10;
	}else if(oad.OI>=0x2401 && oad.OI<=0x2408) {
		unitnum = 14;
	}
	*len = 0;
	*len += create_struct(&buf[*len],unitnum);
	for(i=0;i<unitnum;i++) {
		*len += fill_Scaler_Unit(&buf[*len],su[i]);
	}
	return 1;
}

int class12_get(OAD oad, INT8U *sourcebuf, INT8U *buf, int *len){
	asyslog(LOG_WARNING, "召唤脉冲属性(%d)", oad.attflg);
	ProgramInfo *shareAddr = getShareAddr();
	int index = oad.OI - 0x2401;

	index = rangeJudge("脉冲",index,0,(MAX_PULSE_NUM-1));
	if(index == -1) return 0;

	switch (oad.attflg) {
	case 2:
		*len = fill_TSA(&buf[*len],&shareAddr->class12[index].addr[1],shareAddr->class12[index].addr[0]);
		return 1;
	case 3:
		return class12_get_3(oad.OI, shareAddr->class12[index].pt, shareAddr->class12[index].ct, buf, len);
	case 4:
		return class12_get_4(oad.OI, shareAddr->class12[index].unit, buf, len);
	case 5:
		return class12_get_5_6(oad.OI, shareAddr->class12[index].p, buf, len);
	case 6:
		return class12_get_5_6(oad.OI, shareAddr->class12[index].q, buf, len);
	case 7:
		return class12_get_double_long_unsigned(oad, shareAddr->class12[index].day_pos_p, buf, len);
	case 8:
		return class12_get_double_long_unsigned(oad, shareAddr->class12[index].mon_pos_p, buf, len);
	case 9:
		return class12_get_double_long_unsigned(oad, shareAddr->class12[index].day_nag_p, buf, len);
	case 10:
		return class12_get_double_long_unsigned(oad, shareAddr->class12[index].mon_nag_p, buf, len);
	case 11:
		return class12_get_double_long_unsigned(oad, shareAddr->class12[index].day_pos_q, buf, len);
	case 12:
		return class12_get_double_long_unsigned(oad, shareAddr->class12[index].mon_pos_q, buf, len);
	case 13:
		return class12_get_double_long_unsigned(oad, shareAddr->class12[index].day_nag_q, buf, len);
	case 14:
		return class12_get_double_long_unsigned(oad, shareAddr->class12[index].mon_nag_q, buf, len);
	case 15:
		return class12_get_double_long_unsigned(oad, shareAddr->class12[index].val_pos_p, buf, len);
	case 16:
		return class12_get_double_long_unsigned(oad, shareAddr->class12[index].val_pos_q, buf, len);
	case 17:
		return class12_get_double_long_unsigned(oad, shareAddr->class12[index].val_nag_p, buf, len);
	case 18:
		return class12_get_double_long_unsigned(oad, shareAddr->class12[index].val_nag_q, buf, len);
	case 19:
		return get_Scaler_Unit(oad, shareAddr->class12[index].su, buf, len);
	}
	return 1;
}
