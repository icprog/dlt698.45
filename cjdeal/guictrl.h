/*
 * guictrl.h
 *
 *  Created on: 2017-1-4
 *      Author: wzm
 */
#ifndef GUICTRL_H_
#define GUICTRL_H_

pthread_attr_t guictrl_attr_t;
int thread_guictrl_id;        //液晶、控制（I型、专变）
pthread_t thread_guictrl;
extern void guictrl_proccess();
#endif /* EVENTCALC_H_ */
