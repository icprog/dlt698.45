/*
 * main.h
 *
 *  Created on: Feb 10, 2017
 *      Author: lhl
 */

#ifndef MAIN_H_
#define MAIN_H_


extern void acs_process(int argc, char *argv[]);
extern void event_process(int argc, char *argv[]);
/*
 * 采集监控类文件的读取*/
extern void coll_process(int argc, char *argv[]);
/*
 * 参变量类*/
extern void para_process(int argc, char *argv[]);

#endif /* MAIN_H_ */

