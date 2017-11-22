//
// Created by 周立海 on 2017/4/24.
//

#ifndef INC_698_CRTL_BASE_H
#define INC_698_CRTL_BASE_H

#include "../include/StdDataType.h"

#define CTRL_HEAD 0x68
#define CTRL_END 0x16
#define CTRL_LEN 7

#define TRUE_STATE 1
#define ZERO_STATE 0

#define HE 1
#define FEN 0
#define RED_LIGHT TRUE_STATE
#define RED_CLOSE ZERO_STATE
#define GREEN_LIGHT TRUE_STATE
#define GREEN_CLOSE ZERO_STATE
#define VALID 0x80

#define SEND_CTRL_CMD 0xA3  //控制命令输出
#define RECE_CTRL_CMD 0x3A  //返回控制状态

#define SEND_CTRL_STAT 0xA4  //读取控制状态命令
#define RECE_CTRL_STAT 0x4A  //返回控制状态

#define SEND_CTRLHUILU_STAT 0xA5  //读取控制回路状态命令
#define RECE_CTRLHUILU_STAT 0x5A  //返回控制回路状态

int CheckModelState();
int OpenSerialPort();
int InitCtrlModel();

INT8U SetCtrl_CMD(int serial_fd, INT8U lun1_state, INT8U lun1_red,
		INT8U lun1_green, INT8U lun2_state, INT8U lun2_red, INT8U lun2_green,
		INT8U gongk_led, INT8U diank_led, INT8U alm_state, INT8U baodian_led);

int CtrlModelIsPlugIn();
int CtrlInitCommPort();
void CtrlInitModelState();
#endif
