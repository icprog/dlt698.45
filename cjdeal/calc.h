/*
 * calc.h
 *
 *  Created on: 2017-2-15
 *      Author: wzm
 */

#ifndef CALC_H_
#define CALC_H_

#include "ParaDef.h"
#include "StdDataType.h"

pthread_attr_t calc_attr_t;
int thread_calc_id;        //统计
pthread_t thread_calc;
extern void calc_proccess();
extern void terminalTaskFreeze(INT8U taskid,INT8U fanganid);


#endif /* CALC_H_ */
