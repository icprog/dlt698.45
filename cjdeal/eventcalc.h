/*
 * eventcalc.h
 *
 *  Created on: 2017-1-4
 *      Author: wzm
 */
#ifndef EVENTCALC_H_
#define EVENTCALC_H_

pthread_attr_t eventcalc_attr_t;
int thread_eventcalc_id;        //事件、统计（I型、II型、专变）
pthread_t thread_eventcalc;
extern void eventcalc_proccess();
#endif /* EVENTCALC_H_ */
