/*
 * function_test.h
 *
 *  Created on: 2014-2-26
 *      Author: yd
 */

#ifndef FUNCTION_TEST_H_
#define FUNCTION_TEST_H_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include "PublicFunction.h"
#include "StdDataType.h"
#include "ParaDef.h"
#include "basedef.h"

extern int PressKey;

extern int u2g(char *inbuf, int inlen, char *outbuf, int outlen);
extern int ReceiveFrom485_test(unsigned char *str, int ComPort);

extern ConfigPara	g_cfg_para;
extern void Esam_test();
extern void USB_test();
extern void RS485I_test();
extern void RS485II_test();
extern void ZB_test();
extern void softver_test();
extern void RJ45_test();
extern void YX_test();
extern void battery_test();

#endif /* FUNCTION_TEST_H_ */
