/*
 * atBase.h
 *
 *  Created on: 2017-7-20
 *      Author: zhoulihai
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "StdDataType.h"
#include "PublicFunction.h"

#ifndef ATBASE_H_
#define ATBASE_H_

#define AT_FRAME_LEN 4096
#define AT_FINISH_PREPARE 99

typedef struct {
	INT32U fd;
	INT32U state; //作为AT流程的流转标识

	INT8U at_retry;

	INT8S recv[AT_FRAME_LEN];
	INT8S send[AT_FRAME_LEN];
	INT32U SendLen; //可以发送数据,数字为发送的字节数

	INT32U Tmp;
	INT8S INFO[6][32]; //模块的厂家信息
	INT32S TYPE; //在线类型
	INT32S CSQ; //信号强度
	INT32S REG_STATE; //注网状态
	INT8U CIMI[64];
    INT8U ccid[VISIBLE_STRING_LEN];        // SIM卡CCID
    INT8U imsi[VISIBLE_STRING_LEN];        // SIM卡IMSI
	INT8U PPPD; //pppd拨号成功
	INT8U GPRS_STATE; //拨号状态
	INT8U PPP_IP[OCTET_STRING_LEN];//拨号IP
	INT8U script;	//使用的拨号脚本
} ATOBJ;

ATOBJ *AtGet(void);

int AtInitObjBlock(ATOBJ *ao);
int AtPrepareFinish(ATOBJ *ao);

int AtPrepare(ATOBJ *ao);
int ATUpdateStatus(ATOBJ *ao);

int AtSendCmd(ATOBJ *ao, INT8U * buf, INT32U len);
int AtWriteToBuf(int fd, INT8U *buf, INT16U len);
int AtReadExactly(ATOBJ *ao, CommBlock *nst);
int AtSendExactly(ATOBJ *ao);
int AtGetSendLen(ATOBJ *ao);

#endif /* ATBASE_H_ */
