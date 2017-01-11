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

static char
		*usage =
				"Usage: ./cj (维护功能)  "							\
					"[run options]\n"								\
					"help	 [help]\n\n"							\
					"\n--------------------采集监控类对象----------------------------\n"	\
					"cj para 6000 [采集档案配置表读取]	\n";

int main(int argc, char *argv[])
{
	if(argc<2) {
		fprintf(stderr,"%s",usage);
		return EXIT_SUCCESS;
	}
	if(strcmp("help",argv[1])==0) {
		fprintf(stderr,"%s",usage);
		return EXIT_SUCCESS;
	}
	if(strcmp("para",argv[1])==0)
	{
		if(argc>2) {
			para_process(argv[2]);
		}
		return EXIT_SUCCESS;
	}
	if(strcmp("test",argv[1])==0)
	{
		DataType  dt={};
		fprintf(stderr,"DataType len=%d\n",sizeof(dt));
		return EXIT_SUCCESS;
	}
	fprintf(stderr,"%s",usage);
	return EXIT_SUCCESS;
}
