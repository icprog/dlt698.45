#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>
#include "sys/reboot.h"
#include <wait.h>
#include <errno.h>
#include "cjdeal.h"
#include "read485.h"
#include "eventcalc.h"
#include "stateacs.h"
#include "readplc.h"
#include "guictrl.h"

/*********************************************************
 *程序入口函数-----------------------------------------------------------------------------------------------------------
 *程序退出前处理，杀死其他所有进程 清楚共享内存
 **********************************************************/
void QuitProcess(ProjectInfo *proinfo)
{
	proinfo->ProjectID=0;
    fprintf(stderr,"\n退出：%s %d",proinfo->ProjectName,proinfo->ProjectID);
	exit(0);
}
/*********************************************************
 * 进程初始化
 *********************************************************/
int InitPro(ProgramInfo** prginfo, int argc, char *argv[])
{
	if (argc >= 2)
	{
		*prginfo = OpenShMem("ProgramInfo",sizeof(ProgramInfo),NULL);
		ProIndex = atoi(argv[1]);
		fprintf(stderr,"\n%s start",(*prginfo)->Projects[ProIndex].ProjectName);
		(*prginfo)->Projects[ProIndex].ProjectID=getpid();//保存当前进程的进程号
		return 1;
	}
	return 0;
}
/********************************************************
 * 载入档案、参数
 ********************************************************/
int InitPara()
{
  //
	return 0;
}
/*********************************************************
 * 主进程
 *********************************************************/
int main(int argc, char *argv[])
{
	struct sigaction sa1;
	fprintf(stderr,"\n[cjdeal]:cjdeal run!");
	if(InitPro(&JProgramInfo,argc,argv)==0){
		fprintf(stderr,"进程 %s 参数错误",argv[0]);
		return EXIT_FAILURE;
	}
	Setsig(&sa1,QuitProcess);
	//载入档案、参数
	InitPara();
	//485、四表合一
	read485_proccess();
	//事件、统计
	eventcalc_proccess();
	//状态量、交采、短信等其他功能
	stateacs_proccess();
	//载波
	readplc_proccess();
	//液晶、控制
	guictrl_proccess();
	while(1)
   	{
		//检查参数变量是否有变更
        if(1)//如果有变更
        	InitPara();
		sleep(1);
   	}
	return EXIT_SUCCESS;//退出
}
