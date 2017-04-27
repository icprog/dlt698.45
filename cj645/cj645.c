/*
 * v645.c
 *
 *  Created on: 2014-2-28
 *      Author: Administrator
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include "StdDataType.h"
#include "Shmem.h"
#include "PublicFunction.h"

extern INT32S comfd;
extern INT8U addr[6];//表地址，低在前，高在后
extern void dealProcess();

ProgramInfo* JProgramInfo=NULL;

int ProIndex=0;

//处理现场
void QuitProcess()
{
	if(comfd>0) close(comfd);
    shmm_unregister("ProgramInfo", sizeof(ProgramInfo));
	fprintf(stderr, "\n\r cj645 quit xxx\n\r");
	exit(0);
}

int InitPro(ProgramInfo** prginfo, int argc, char *argv[])
{
	if (argc >= 2)
	{
		*prginfo = OpenShMem("ProgramInfo",sizeof(ProgramInfo),NULL);
		ProIndex = atoi(argv[1]);
		fprintf(stderr,"\n%s start",(*prginfo)->Projects[ProIndex].ProjectName);
		(*prginfo)->Projects[ProIndex].ProjectID=getpid();//保存当前进程的进程号
		fprintf(stderr,"ProjectID[%d]=%d\n",ProIndex,(*prginfo)->Projects[ProIndex].ProjectID);
		return 1;
	}
	return 0;
}

//主程序
int main(int argc, char *argv[])
{
	INT8U comport=2;
	struct sigaction sa;

	if(InitPro(&JProgramInfo,argc,argv)==0){
		fprintf(stderr,"进程 %s 参数错误",argv[0]);
		return EXIT_FAILURE;
	}
	fprintf(stderr, "\ncj645 start ....\n\r");
	if(JProgramInfo->cfg_para.device == 2) {	//II型集中器
		comport = 2;
	}else  comport = 4;
	fprintf(stderr,"open /dev/ttyS%d\n", comport);

	Setsig(&sa, QuitProcess);
	if ((comfd=OpenCom(comport, 2400, (INT8U *)"even", 1, 8))<1)
	{
		fprintf(stderr, "OpenCom645 ERR!!! ........................\n");
	}
	dealProcess();

	sleep(1);

	QuitProcess(0);
 	return EXIT_SUCCESS;
}
