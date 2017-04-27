/*
 * key.c //按键处理
 *
 *  Created on: 2013-5-14
 *      Author: yd
 */


#include "comm.h"
#include "../libBase/PublicFunction.h"
int PressKey; //全局变量 按键值

//置位byte的第bit_offset位值为bit
void setbit(int *val, char bit, char bit_offset)
{
	if(bit_offset<0 || bit_offset>31){
		fprintf(stderr,"\n bit_offset > 31");
		return;
	}
	if(bit==1){//置1
		*val |=(1<<bit_offset);
	}else if(bit==0){//置0
		*val &=(~(1<<bit_offset));
   }//else
//		fprintf(stderr,"\n setbit bit != 1 && bit != 0");
	return;
}
#ifndef FB_SIM
int getkey()
{
	int keyval=0, keypress=0;
	keyval = gpio_readbyte((char*)"/dev/gpiKEY_L");//左
	setbit(&keypress, keyval, OFFSET_L);
	keyval = gpio_readbyte((char*)"/dev/gpiKEY_R");//右
	setbit(&keypress, keyval, OFFSET_R);
	keyval = gpio_readbyte((char*)"/dev/gpiKEY_U");//上
	setbit(&keypress, keyval, OFFSET_U);
	keyval = gpio_readbyte((char*)"/dev/gpiKEY_D");//下
	setbit(&keypress, keyval, OFFSET_D);
	keyval = gpio_readbyte((char*)"/dev/gpiKEY_ENT");//确定
	setbit(&keypress, keyval, OFFSET_O);
	keyval = gpio_readbyte((char*)"/dev/gpiKEY_ESC");//取消
	setbit(&keypress, keyval, OFFSET_E);
	return keypress;
}
#endif
