#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>
#include "sys/reboot.h"
#include <wait.h>
#include <errno.h>

#include "ParaDef.h"
#include "PublicFunction.h"
#include "cjmain.h"

void Runled()
{

}

void Watchdog(int dogmin)//硬件看门狗
{
	int fd = -1;
	if((fd = open("/dev/watchdog", O_RDWR | O_NDELAY)) == -1)
	{
		fprintf(stderr, "\n\r open /dev/watchdog error!!!");
		return;
	}
	if (dogmin<2 || dogmin>20)
		dogmin = 5;
	write(fd,&dogmin,sizeof(int));
	close(fd);
	return;
}
//检查外部程序是否正常运行,是否超时
void ProjectCheck(ProjectInfo *proinfo)
{
	if(strlen((char*)proinfo->ProjectName)<2) return;
	if(proinfo->ProjectState==NowRun)
	{
		if(proinfo->WaitTimes > PRO_WAIT_COUNT)
		{
			proinfo->ProjectState=NeedKill;
			proinfo->WaitTimes=0;
		}
	}
}

int LAPI_Fork2(void)
{
    pid_t pid;
    int status=0;
    if (!(pid = fork())) {
        switch (fork()) {
            case 0:
                return 0;
            case -1:
                _exit( errno ); /* assumes all errnos are <256 */
                break;
            default:
                _exit(0);//孙子的父亲离开，以便让儿子的父亲回收!!
                break;
        }
    }
    if (pid < 0 || waitpid(pid, &status, 0) < 0)//儿子的父亲回收
        return -1;

    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) == 0)
            return 1;
        else
            errno = WEXITSTATUS(status);
    } else
        errno = EINTR; /* well, interrupted */

    return -1;
}
int ProjectKill(ProjectInfo proinfo)
{
	if(proinfo.ProjectID>0)
	{
		fprintf(stderr,"\nkill %d",proinfo.ProjectID);
		kill(proinfo.ProjectID,SIGTERM);
		return 1;
	}
	return 0;
}
//激活外部程序
int ProjectExecute(ProjectInfo proinfo,int i)
{
	pid_t pid;
	char t1[20]={};
	pid = LAPI_Fork2();
	//创建一个新进程
	if(pid==-1)
	{
		perror( "\n\rfork failure" );
		return 1;
	}
	memset(t1,0,sizeof(t1));
	if(pid==0)//子进程代码运行部分
	{
		execlp((char*)proinfo.ProjectName,(char*)proinfo.ProjectName,proinfo.argv[0],proinfo.argv[1],proinfo.argv[2],proinfo.argv[3],NULL);
		exit(1);
		return 0;
	}
	else//父进程代码运行部分
	{
		//获取程序运行是的名称
		return 1;
	}
	return 1;
}

//读取系统配置文件
void ReadSystemInfo()
{
	char syssetfile[100]={};
	char ReadString[256]={},proname[PRONAMEMAXLEN]={},argv1[ARGVMAXLEN]={},argv2[ARGVMAXLEN]={},argv3[ARGVMAXLEN]={};
	int  i=0;
    FILE *fp=NULL;
    for(i=0;i<PROJECTCOUNT;i++)
	{
    	memset((void *)&JProgramInfo->Projects[i],0,sizeof(ProjectInfo));
	}
	sprintf((char*)syssetfile,"%s/systema.cfg",_CFGDIR_);

	fp = fopen(syssetfile,"r");
	if(fp!=NULL)
	{
		while( ! feof( fp ) )
		{
			memset((char *)ReadString,0,sizeof(ReadString));
			fgets( (char *)ReadString, sizeof( ReadString ), fp );
			if(strncmp(ReadString,"[config]",sizeof(ReadString))==0)
			{
				break;
			}
			if(strncmp(ReadString,"[end]",sizeof(ReadString))==0)
			{
				continue;
			}
			memset(proname,0,PRONAMEMAXLEN);
			memset(argv1,0,ARGVMAXLEN);
			memset(argv2,0,ARGVMAXLEN);
			memset(argv3,0,ARGVMAXLEN);
			if (sscanf(ReadString,"%d=%s %s %s %s",&i,proname,argv1,argv2,argv3)>0)
			{
				if (i>=PROJECTCOUNT || i<0)
					continue;
				if(strlen((char*)proname)<2)
					continue;

				memcpy(JProgramInfo->Projects[i].ProjectName,proname,PRONAMEMAXLEN);
				memcpy(JProgramInfo->Projects[i].argv[1],argv1,ARGVMAXLEN);
				memcpy(JProgramInfo->Projects[i].argv[2],argv2,ARGVMAXLEN);
				memcpy(JProgramInfo->Projects[i].argv[3],argv3,ARGVMAXLEN);
				sprintf((char *)JProgramInfo->Projects[i].argv[0],"%d",i);
				JProgramInfo->Projects[i].ProjectState = NeedStart;
				JProgramInfo->Projects[i].WaitTimes = 0;
			}
		}
		fclose(fp);
		fp=NULL;
	}
	fprintf(stderr,"\n\n\n");
}

//程序入口函数-----------------------------------------------------------------------------------------------------------
//程序退出前处理，杀死其他所有进程 清楚共享内存
void ProjectMainExit(int signo)
{
	exit(0);
	return ;
}
int main(int argc, char *argv[])
{
	int i=0;
	INT8U prostat=0;
	struct sigaction sa1;
	fprintf(stderr,"\ncjmain run!");
	Setsig(&sa1,ProjectMainExit);

	//此设置决定集中器在下电情况下，长按向下按键唤醒功能
	gpio_writebyte((INT8S*)DEV_BAT_SWITCH,(INT8S)1);

	JProgramInfo = (ProgramInfo*)CreateShMem("ProgramInfo",sizeof(ProgramInfo),NULL);
	ReadSystemInfo();
	while(1)
   	{
		sleep(1);
		Watchdog(5);
		Runled();
		for(i=0;i<PROJECTCOUNT;i++)
		{
			ProjectCheck(&JProgramInfo->Projects[i]);
			switch(JProgramInfo->Projects[i].ProjectState)
			{
				case NeedKill:
					fprintf(stderr,"\n进程:%s %s  PID=%d 异常！",JProgramInfo->Projects[i].ProjectName,JProgramInfo->Projects[i].argv[0],JProgramInfo->Projects[i].ProjectID);
					if(ProjectKill(JProgramInfo->Projects[i])==1)
						JProgramInfo->Projects[i].ProjectState = NeedStart;
					break;
				case NeedStart:
					if(	ProjectExecute(JProgramInfo->Projects[i],i)==0)
						return  EXIT_SUCCESS;
					else
						JProgramInfo->Projects[i].ProjectState = NowRun;
					break;
				case NowRun:
					break;
			}
			JProgramInfo->Projects[i].WaitTimes++;
		}
   	}
	return EXIT_SUCCESS;//退出
}
