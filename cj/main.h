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
/*
 * 文件传输类对象 ESAM接口类对象 输入输出设备类对象 显示类对象
 * */
void inoutdev_process(int argc, char *argv[]);
#endif /* MAIN_H_ */

