/*
 * interfun.c
 *
 *  Created on: Feb 27, 2017
 *      Author: ava
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "AccessFun.h"
#include "StdDataType.h"
#include "Objectdef.h"
#include "dlt698.h"

int  create_OAD(INT8U *data,OAD oad)
{
	data[0] = ( oad.OI >> 8 ) & 0xff;
	data[1] = oad.OI & 0xff;
	data[2] = oad.attflg;
	data[3] = oad.attrindex;
	return 4;
}

int create_array(INT8U *data,INT8U numm)
{
	data[0] = dtarray;
	data[1] = numm;
	return 2;
}
int create_struct(INT8U *data,INT8U numm)
{
	data[0] = dtstructure;
	data[1] = numm;
	return 2;
}
int file_bool(INT8U *data,INT8U value)
{
	data[0] = dtbool;
	data[1] = value;
	return 2;
}
int fill_bit_string8(INT8U *data,INT8U bits)
{
	//TODO : 默认8bit ，不符合A-XDR规范
	data[0] = dtbitstring;
	data[1] = 0x08;
	data[2] = bits;
	return 3;
}

int fill_double_long_unsigned(INT8U *data,INT32U value)
{
	data[0] = dtdoublelongunsigned;
	data[1] = (value & 0xFF000000) >> 24 ;
	data[2] = (value & 0x00FF0000) >> 16 ;
	data[3] = (value & 0x0000FF00) >> 8 ;
	data[4] =  value & 0x000000FF;
	return 5;
}

int fill_octet_string(INT8U *data,char *value,INT8U len)
{
	data[0] = dtoctetstring;
	data[1] = len;
	memcpy(&data[2],value,len);
	return (len+2);
}

int fill_visible_string(INT8U *data,char *value,INT8U len)
{
	data[0] = dtvisiblestring;
	data[1] = len;
	memcpy(&data[2],value,len);
	return (len+2);
}

int fill_integer(INT8U *data,INT8U value)
{
	data[0] = dtinteger;
	data[1] = value;
	return 2;
}
int fill_unsigned(INT8U *data,INT8U value)
{
	data[0] = dtunsigned;
	data[1] = value;
	return 2;
}

int fill_long_unsigned(INT8U *data,INT16U value)
{
	data[0] = dtlongunsigned;
	data[1] = (value & 0xFF00)>>8;
	data[2] = value & 0x00FF;
	return 3;
}
int fill_enum(INT8U *data,INT8U value)
{
	data[0] = dtenum;
	data[1] = value;
	return 2;
}
int fill_time(INT8U *data,INT8U *value)
{
	data[0] = dttime;
	memcpy(&data[1],&value[0],3);
	return 4;
}

int fill_DateTimeBCD(INT8U *data,DateTimeBCD *time)
{
	DateTimeBCD  init_datatimes={};

	memset(&init_datatimes,0xEE,sizeof(DateTimeBCD));
	if(memcmp(time,&init_datatimes,sizeof(DateTimeBCD))==0) {		//时间无效，上送NULL（0）
		data[0] = 0;
		return 1;
	}else {
		data[0] = dtdatetimes;
		time->year.data = time->year.data >>8 | time->year.data<<8;
		memcpy(&data[1],time,sizeof(DateTimeBCD));
		return (sizeof(DateTimeBCD)+1);
	}
}


int fill_TI(INT8U *data,TI ti)
{
	data[0] = dtti;
	data[1] = ti.units;
	data[2] = (ti.interval>>8)&0xff;
	data[3] = ti.interval&0xff;
	return 4;
}

int fill_TSA(INT8U *data,INT8U *value,INT8U len)
{
	data[0] = dttsa;
	data[1] = len;
	memcpy(&data[2],value,len);
	return (len+2);
}



