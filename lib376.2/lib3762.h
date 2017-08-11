/*
 * j3762.h
 *
 *  Created on: 2013-3-19
 *      Author: Administrator
 */

#ifndef J3762_H_
#define J3762_H_

#include "../include/StdDataType.h"
#include "format3762.h"
#include "PublicFunction.h"
extern INT8S analyzeProtocol3762(FORMAT3762* format3762, INT8U* recvBuf, const INT16U recvLen);
extern INT8S composeProtocol3762(FORMAT3762* format3762, INT8U* sendBuf);
extern int AFN13_F1(FORMAT3762 *down,INT8U *sendBuf3762,INT8U* destAddr, INT8U protocol, INT8U delayFlag, INT8U* sendBuf645, INT8U sendLen645);
extern int AFN11_F1(FORMAT3762 *down,INT8U *sendBuf,INT8U *SlavePointAddr);
extern int AFN11_F2(FORMAT3762 *down,INT8U *sendBuf,INT8U *SlavePointAddr);
extern int AFN10_F1(FORMAT3762 *down,INT8U *sendBuf);
extern int AFN10_F2(FORMAT3762 *down,INT8U *sendBuf,INT16U index, INT8U num);
extern int AFN05_F1(FORMAT3762 *down,INT8U *sendBuf,INT8U *addr);
extern int AFN03_F10(FORMAT3762 *down,INT8U *sendBuf);
extern int AFN03_F9(FORMAT3762 *down,INT8U *sendBuf,INT8U protocol,INT8U msgLen,INT8U *msgContent);
extern int AFN05_F3(FORMAT3762 *down,INT8U moduleFlag, INT8U ctrl, INT8U* sendBuf645, INT8U sendLen645,INT8U *sendBuf);
extern int AFN12_F1(FORMAT3762 *down,INT8U *sendBuf);
extern int AFN14_F1(FORMAT3762 *down,FORMAT3762 *up,INT8U *sendBuf,INT8U* destAddr, INT8U readFlag, INT8U delayTime, INT8U msgLen, INT8U *msgContent);
extern int AFN00_F01(FORMAT3762 *down,INT8U *sendBuf);
extern int AFN12_F3(FORMAT3762 *down,INT8U *sendBuf);
extern int AFN12_F2(FORMAT3762 *down,INT8U *sendBuf);
extern int AFN10_F4(FORMAT3762 *down,INT8U *sendBuf);
extern int AFN11_F5(FORMAT3762 *down,INT8U *sendBuf,INT8U minute);
extern int AFN05_F3(FORMAT3762 *down,INT8U moduleFlag, INT8U ctrl, INT8U* sendBuf645, INT8U sendLen645,INT8U *sendBuf);
extern INT8S AFN03_F4(FORMAT3762 *down,INT8U *sendBuf);
extern INT8S AFN01_F2(FORMAT3762 *down,INT8U *sendBuf);
#endif /* J3762_H_ */
