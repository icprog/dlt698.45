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
	INT32U state;	//作为AT流程的流转标识
	INT32U RecvHead;
	INT32U RecvTail;
	INT32U SendHead;
	INT32U SendTail;

	INT8U recv[AT_FRAME_LEN];
	INT8U deal[AT_FRAME_LEN];
	INT8U send[AT_FRAME_LEN];
	INT32U NeedRead;	//需要读取数据
	INT32U NeedSend;	//可以发送数据,数字为发送的字节数
	INT32U Tmp;
	INT8S INFO[6][32];	//模块的厂家信息
	INT8U TYPE;			//在线类型
	INT8U CSQ;			//信号强度
	INT32S REG_STATE;	//注网状态
	INT8U CIMI[64];
	INT8U PPPD;			//pppd拨号成功
	INT8U GPRS_STATE;	//拨号状态
} ATOBJ;

ATOBJ *AtGet(void);

int AtInitObjBlock(ATOBJ *ao);
int AtProcessRecv(ATOBJ *ao);
int AtDealData(ATOBJ *ao);
int AtNeedRead(ATOBJ *ao);
int AtNeedSend(ATOBJ *ao);
int ATReadData(ATOBJ *ao);
int ATSendData(ATOBJ *ao);

int AtInPrepare(ATOBJ *ao);
int AtPrepareFinish(ATOBJ *ao);

int AtPrepare(ATOBJ *ao);
void AtDealer(ATOBJ *ao);

int ATUpdateStatus(ATOBJ *ao);

int AtSendCmd(ATOBJ *ao, INT8U * buf, INT32U len);


#endif /* ATBASE_H_ */
