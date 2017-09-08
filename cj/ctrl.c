/*
 * ctrl.c
 *
 *  Created on: Sep 7, 2017
 *      Author: lhl
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "AccessFun.h"
#include "StdDataType.h"
#include "Objectdef.h"
#include "main.h"
#include "dlt698.h"
#include "dlt645.h"
#include "PublicFunction.h"
#include "crtl_base.h"

extern ProgramInfo* JProgramInfo ;

typedef union {//control code
	INT16U u16b;//convenient to set value to 0
	struct {//only for little endian mathine!
		INT8U bak	: 6;	//备用
		INT8U lun1_state: 1;//轮次1-状态
		INT8U lun1_red	: 1;//轮次1-红灯
		INT8U lun1_green: 1;//轮次1-绿灯
		INT8U lun2_state: 1;//轮次2-状态
		INT8U lun2_red	: 1;//轮次2-红灯
		INT8U lun2_green: 1;//轮次2-绿灯
		INT8U gongk_led		: 1;//功控灯
		INT8U diank_led		: 1;//电控灯
		INT8U alm_state		: 1;//告警状态
		INT8U baodian_led	: 1;//报警灯
	} ctrl;
} ctrlUN;
void ctrlTest(int argc, char *argv[])
{
	int roundno = 0;
	int cmd = 0;
   	JProgramInfo = OpenShMem("ProgramInfo", sizeof(ProgramInfo), NULL);
	if(argc < 5){
		fprintf(stderr, "参数不足\n");
	}else {
		roundno = atoi(argv[3]);
		cmd = atoi(argv[4]);
	}
	fprintf(stderr,"轮次【%d】 状态[%d]\n",roundno,cmd);
	if(roundno == 1 && cmd==0)  {
		fprintf(stderr, "一轮 分闸\n");
		JProgramInfo->ctrls.control[0] = 0xEEFFEFEF;
		JProgramInfo->ctrls.control[1] = 0xEEFFEFEF;
		JProgramInfo->ctrls.control[2] = 0xEEFFEFEF;
	}else if(roundno == 1 && cmd == 1) {
		fprintf(stderr, "一轮 合闸\n");
		JProgramInfo->ctrls.control[0] = 0xCCAACACA;
		JProgramInfo->ctrls.control[1] = 0xCCAACACA;
		JProgramInfo->ctrls.control[2] = 0xCCAACACA;
	}else if(roundno == 2 && cmd==0)  {
		fprintf(stderr, "二轮 分闸\n");
		JProgramInfo->ctrls.control[0] = 0x55552525;
		JProgramInfo->ctrls.control[1] = 0x55552525;
		JProgramInfo->ctrls.control[2] = 0x55552525;
	}else if(roundno == 2 && cmd == 1) {
		fprintf(stderr, "二轮 合闸\n");
		JProgramInfo->ctrls.control[0] = 0xCCCC2C2C;
		JProgramInfo->ctrls.control[1] = 0xCCCC2C2C;
		JProgramInfo->ctrls.control[2] = 0xCCCC2C2C;
	}else {
		fprintf(stderr, "非法参数\n");
	}
	shmm_unregister("ProgramInfo", sizeof(ProgramInfo));
}

void ctrl_comm(ctrlUN 	ctrlunit)
{
	CheckModelState();
	InitCtrlModel();
	int fd = OpenSerialPort();
	SetCtrl_CMD(fd, ctrlunit.ctrl.lun1_state, ctrlunit.ctrl.lun1_red, ctrlunit.ctrl.lun1_green,
				ctrlunit.ctrl.lun2_state, ctrlunit.ctrl.lun2_red, ctrlunit.ctrl.lun2_green,
				ctrlunit.ctrl.gongk_led, ctrlunit.ctrl.diank_led, ctrlunit.ctrl.alm_state, ctrlunit.ctrl.baodian_led);
	close(fd);
}
void ctrlType(int argc, char *argv[])
{
	int	type=0;
	ctrlUN 	ctrlunit;
	if(argc == 4) {
		type = atoi(argv[3]);
		if(type == 1) {	//功控
			ctrlunit.ctrl.lun1_state = 1;
			ctrlunit.ctrl.lun1_red = 0;
			ctrlunit.ctrl.lun1_green = 1;
			ctrlunit.ctrl.lun2_state = 1;
			ctrlunit.ctrl.lun2_red = 0;
			ctrlunit.ctrl.lun2_green = 1;
			ctrlunit.ctrl.gongk_led = 1;
			ctrlunit.ctrl.diank_led = 0;
			ctrlunit.ctrl.baodian_led = 0;
			ctrlunit.ctrl.alm_state = 0;
		}else  if(type == 2){ //电控
			ctrlunit.ctrl.lun1_state = 1;
			ctrlunit.ctrl.lun1_red = 0;
			ctrlunit.ctrl.lun1_green = 1;
			ctrlunit.ctrl.lun2_state = 1;
			ctrlunit.ctrl.lun2_red = 0;
			ctrlunit.ctrl.lun2_green = 1;
			ctrlunit.ctrl.gongk_led = 0;
			ctrlunit.ctrl.diank_led = 1;
			ctrlunit.ctrl.baodian_led = 0;
			ctrlunit.ctrl.alm_state = 0;
		}
		if(type==1 || type==2) {
			ctrl_comm(ctrlunit);
		}
	}
}

void ctrlAlarm(int argc, char *argv[])
{
	int	type=0;
	ctrlUN 	ctrlunit;

	if(argc == 4) {
		type = atoi(argv[3]);
		if(type == 1) {	//告警投入
			ctrlunit.ctrl.lun1_state = 1;
			ctrlunit.ctrl.lun1_red = 0;
			ctrlunit.ctrl.lun1_green = 1;
			ctrlunit.ctrl.lun2_state = 1;
			ctrlunit.ctrl.lun2_red = 0;
			ctrlunit.ctrl.lun2_green = 1;
			ctrlunit.ctrl.gongk_led = 0;
			ctrlunit.ctrl.diank_led = 0;
			ctrlunit.ctrl.baodian_led = 0;
			ctrlunit.ctrl.alm_state = 1;
		}else  if(type == 0){ //告警解除
			ctrlunit.ctrl.lun1_state = 1;
			ctrlunit.ctrl.lun1_red = 0;
			ctrlunit.ctrl.lun1_green = 1;
			ctrlunit.ctrl.lun2_state = 1;
			ctrlunit.ctrl.lun2_red = 0;
			ctrlunit.ctrl.lun2_green = 1;
			ctrlunit.ctrl.gongk_led = 0;
			ctrlunit.ctrl.diank_led = 0;
			ctrlunit.ctrl.baodian_led = 0;
			ctrlunit.ctrl.alm_state = 1;
		}
		if(type==1 || type==0) {
			ctrl_comm(ctrlunit);
		}
	}
}

void ctrlKeepelec(int argc, char *argv[])
{
	int	type=0;
	ctrlUN 	ctrlunit;

	if(argc == 4) {
		type = atoi(argv[3]);
		if(type == 1) {	//保电投入
			ctrlunit.ctrl.lun1_state = 1;
			ctrlunit.ctrl.lun1_red = 0;
			ctrlunit.ctrl.lun1_green = 1;
			ctrlunit.ctrl.lun2_state = 1;
			ctrlunit.ctrl.lun2_red = 0;
			ctrlunit.ctrl.lun2_green = 1;
			ctrlunit.ctrl.gongk_led = 0;
			ctrlunit.ctrl.diank_led = 0;
			ctrlunit.ctrl.baodian_led = 1;
			ctrlunit.ctrl.alm_state = 0;
		}else  if(type == 0){ //保电解除
			ctrlunit.ctrl.lun1_state = 1;
			ctrlunit.ctrl.lun1_red = 0;
			ctrlunit.ctrl.lun1_green = 1;
			ctrlunit.ctrl.lun2_state = 1;
			ctrlunit.ctrl.lun2_red = 0;
			ctrlunit.ctrl.lun2_green = 1;
			ctrlunit.ctrl.gongk_led = 0;
			ctrlunit.ctrl.diank_led = 0;
			ctrlunit.ctrl.baodian_led = 0;
			ctrlunit.ctrl.alm_state = 0;
		}
		if(type==1 || type==0) {
			ctrl_comm(ctrlunit);
		}
	}
}


void breezeTest(int argc, char *argv[])
{
	fprintf(stderr,"【蜂鸣器】蜂鸣投入 cj buzzer 1 <蜂鸣投入>  cj buzzer 0 <蜂鸣解除>\n");
	if(argc == 3) {
		if(strcmp("0",argv[2])==0) {
			gpio_writebyte((INT8S *)DEV_ALARM_BUZZER,0x00);
		}else if(strcmp("1",argv[2])==0){
			gpio_writebyte((INT8S *)DEV_ALARM_BUZZER,0x01);
		}
	}
}

void ctrl_process(int argc, char *argv[])
{
	ctrlUN 	ctrlunit;
	if(argc>2) {
		if(strcmp(argv[1],"ctrl")==0) {
			if(strcmp(argv[2],"round")==0) {
				ctrlTest(argc,argv);
			}else if(strcmp(argv[2],"type")==0) {
				ctrlType(argc,argv);
			}else if(strcmp(argv[2],"alarm")==0) {
				ctrlAlarm(argc,argv);
			}else if(strcmp(argv[2],"keepelec")==0) {
				ctrlKeepelec(argc,argv);
			}else if(strcmp(argv[2],"clear")==0) {
				ctrlunit.u16b = 0;
				ctrl_comm(ctrlunit);
			}
		}
	}
}
