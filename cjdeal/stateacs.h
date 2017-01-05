/*
 * stateacs.h
 *
 *  Created on: 2017-1-4
 *      Author: wzm
 */
#ifndef STATEACS_H_
#define STATEACS_H_

pthread_attr_t stateacs_attr_t;
int thread_stateacs_id;        //状态量、交采、短信等其他功能（I型、II型、专变）
pthread_t thread_stateacs;
extern void stateacs_proccess();
#endif /* EVENTCALC_H_ */
