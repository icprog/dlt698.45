/*
 * main.c
 *
 *  Created on: Jan 5, 2017
 *      Author: ava
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include <mqueue.h>
#include <semaphore.h>
#include <termios.h>

#include "StdDataType.h"
#include "para.h"
#include "event.h"

static char
		*usage_para =
				"Usage: ./cj (维护功能)  "							\
					"[run options]\n"								\
					"help	 [help]\n\n"							\
					"\n--------------------采集监控类对象----------------------------\n"	\
					"cj para 6000 [采集档案配置表读取]	\n"					\
					;
static char
		*usage_event =
					"--------------------事件类对象----------------------------\n"	\
					"[读取事件属性]  cj event att <oi> :例如：读取采集终端初始化事件属性 cj event att 3100 	\n"					\
					"[Class7]  cj event att <oi> 当前记录数 最大记录数 上报标识 有效标识 关联对象个数 关联对象OAD[1-10]	\n"	\
					"[设置采集终端初始化事件] cj event att 3100 1 16 1 1 0 \n"	\
					"[设置终端状态量变位事件] cj event att 3104 1 16 1 1 5 201E-4200 F203-4201 F203-4202 F203-4203 F203-4204 F203-4205\n"		\
					;

int main(int argc, char *argv[])
{
	if(argc<2) {
		fprintf(stderr,"%s",usage_para);
		fprintf(stderr,"%s",usage_event);
		return EXIT_SUCCESS;
	}
	if(strcmp("help",argv[1])==0) {
		fprintf(stderr,"%s",usage_para);
		fprintf(stderr,"%s",usage_event);
		return EXIT_SUCCESS;
	}
	if(strcmp("para",argv[1])==0)
	{
		if(argc>2) {
			para_process(argv[2]);
		}
		return EXIT_SUCCESS;
	}
	if(strcmp("event",argv[1])==0)
	{
		fprintf(stderr,"%s",usage_event);
		event_process(argc,argv);
		return EXIT_SUCCESS;
	}
	if(strcmp("test",argv[1])==0)
	{
		DataType  dt={};
		fprintf(stderr,"DataType len=%d\n",sizeof(dt));
		return EXIT_SUCCESS;
	}
	fprintf(stderr,"%s",usage_para);
	fprintf(stderr,"%s",usage_event);
	return EXIT_SUCCESS;
}
