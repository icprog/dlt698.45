/*
 * readplc.h
 *
 *  Created on: 2017-1-4
 *      Author: wzm
 */
#ifndef READPLC_H_
#define READPLC_H_

pthread_attr_t readplc_attr_t;
int thread_readplc_id;        //载波（I型）
pthread_t thread_readplc;
extern void readplc_proccess();
#endif /* EVENTCALC_H_ */
