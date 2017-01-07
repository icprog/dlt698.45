/*
 * read485.h
 *
 *  Created on: 2017-1-4
 *      Author: wzm
 */

#ifndef READ485_H_
#define READ485_H_
#include "Objectdef.h"
pthread_attr_t read485_attr_t;
int thread_read485_id;           //485、四表合一（I型、II型、专变）
pthread_t thread_read485;

extern void read485_proccess();

#endif /* READ485_H_ */
