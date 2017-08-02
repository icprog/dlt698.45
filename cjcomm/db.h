/*
 * db.h
 *
 *  Created on: 2017-7-26
 *      Author: zhoulihai
 */

#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>

#include "Shmem.h"
#include "cjcomm.h"

#ifndef DB_H_
#define DB_H_

typedef struct {
	CommBlock ifr;
	CommBlock net;
	CommBlock gprs;
	CommBlock serial;

	CLASS_f201 cf201;
	CLASS_f202 cf202;

	CLASS25 c25;
	CLASS26 c26;

	ProgramInfo *JProgramInfo;
	int ProgIndex;
	int OnlineType;		//以太网、gprs
	int CalcNew;		//流量统计
	int GprsType;		//gprs、cmda、4G

	//消息相关
	INT8U retry_buf[MAXSIZ_PROXY_NET];
	mmq_head retry_head;
	int retry_count;
} DBStruct;

void * dbGet(char * name);
int dbSet(char * name, void*);
void dbInit(int index);

#endif /* DB_H_ */
