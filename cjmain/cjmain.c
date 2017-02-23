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
#include "AccessFun.h"
#include "cjmain.h"
#include <dirent.h>
#define PATH_MAX 256
int DevResetNum=0;
void Runled()
{
	gpio_writebyte((char*)DEV_LED_RUN, 1); //运行灯常亮
}

void Watchdog(int dogmin)//硬件看门狗
{
	int fd = -1;
	if((fd = open(DEV_WATCHDOG, O_RDWR | O_NDELAY)) == -1)
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
void shmm_destroy()
{
	if(JProgramInfo !=NULL)
	{
		munmap(JProgramInfo,sizeof(ProgramInfo));
		JProgramInfo = NULL;
		shm_unlink((const char*)"ProgramInfo");
	}
}

//程序入口函数-----------------------------------------------------------------------------------------------------------
//程序退出前处理，杀死其他所有进程 清楚共享内存
void ProjectMainExit(int signo)
{
	fprintf(stderr,"\ncjmain exit\n");
	close_named_sem(SEMNAME_SPI0_0);
	sem_unlink(SEMNAME_SPI0_0);
	close_named_sem(SEMNAME_PARA_SAVE);
	sem_unlink(SEMNAME_PARA_SAVE);
	shmm_destroy();
	exit(0);
	return ;
}

INT32S prog_find_pid_by_name(INT8S* ProcName, INT32S* foundpid)
{
	DIR             *dir;
	struct dirent   *d;
	int             pid, i;
	char            *s;
	int pnlen;
	i = 0;
	foundpid[0] = 0;
	pnlen = strlen((const char*)ProcName);
	/* Open the /proc directory. */
	dir = opendir("/proc");
	if (!dir)
	{
			printf("cannot open /proc");
			return -1;
	}
	/* Walk through the directory. */
	while ((d = readdir(dir)) != NULL) {
			char exe [PATH_MAX+1];
			char path[PATH_MAX+1];
			int len;
			int namelen;
			/* See if this is a process */
			if ((pid = atoi(d->d_name)) == 0)
				continue;
			snprintf(exe, sizeof(exe), "/proc/%s/exe", d->d_name);
			if ((len = readlink(exe, path, PATH_MAX)) < 0)
					continue;
			path[len] = '\0';
			/* Find ProcName */
			s = strrchr(path, '/');
			if(s == NULL) continue;
			s++;
			/* we don't need small name len */
			namelen = strlen(s);
			if(namelen < pnlen)     continue;
			if(!strncmp((const char*)ProcName, s, pnlen)) {
					/* to avoid subname like search proc tao but proc taolinke matched */
					if(s[pnlen] == ' ' || s[pnlen] == '\0') {
							foundpid[i] = pid;
							i++;
					}
			}
	}
	foundpid[i] = 0;
	closedir(dir);
	return  i;
}

/*
 * 初始化操作
 * */
void ProgInit()
{
	sem_t * sem_spi=NULL,*sem_parasave=NULL;	//SPI通信信号量
	int		val;

	//此设置决定集中器电池工作，并保证在下电情况下，长按向下按键唤醒功能
	gpio_writebyte(DEV_BAT_SWITCH,(INT8S)1);
	//信号量建立
	sem_spi = create_named_sem(SEMNAME_SPI0_0,1);							//TODO:放入vmain
	sem_getvalue(sem_spi, &val);
	sem_parasave = create_named_sem(SEMNAME_PARA_SAVE,1);
	sem_getvalue(sem_parasave, &val);
	fprintf(stderr,"process The sem is %d\n", val);
	InitClass4300();
	//初始化事件参数，调用文件
	//JProgramInfo
	readCoverClass(0x3100,0,&JProgramInfo->event_obj.Event3100_obj,sizeof(JProgramInfo->event_obj.Event3100_obj),event_para_save);
	readCoverClass(0x3101,0,&JProgramInfo->event_obj.Event3101_obj,sizeof(JProgramInfo->event_obj.Event3101_obj),event_para_save);
	readCoverClass(0x3104,0,&JProgramInfo->event_obj.Event3104_obj,sizeof(JProgramInfo->event_obj.Event3104_obj),event_para_save);
	readCoverClass(0x3105,0,&JProgramInfo->event_obj.Event3105_obj,sizeof(JProgramInfo->event_obj.Event3105_obj),event_para_save);
	readCoverClass(0x3106,0,&JProgramInfo->event_obj.Event3106_obj,sizeof(JProgramInfo->event_obj.Event3106_obj),event_para_save);
	readCoverClass(0x3107,0,&JProgramInfo->event_obj.Event3107_obj,sizeof(JProgramInfo->event_obj.Event3107_obj),event_para_save);
	readCoverClass(0x3108,0,&JProgramInfo->event_obj.Event3108_obj,sizeof(JProgramInfo->event_obj.Event3108_obj),event_para_save);
	readCoverClass(0x3109,0,&JProgramInfo->event_obj.Event3109_obj,sizeof(JProgramInfo->event_obj.Event3109_obj),event_para_save);
	readCoverClass(0x310A,0,&JProgramInfo->event_obj.Event310A_obj,sizeof(JProgramInfo->event_obj.Event310A_obj),event_para_save);
	readCoverClass(0x310B,0,&JProgramInfo->event_obj.Event310B_obj,sizeof(JProgramInfo->event_obj.Event310B_obj),event_para_save);
	readCoverClass(0x310C,0,&JProgramInfo->event_obj.Event310C_obj,sizeof(JProgramInfo->event_obj.Event310C_obj),event_para_save);
	readCoverClass(0x310D,0,&JProgramInfo->event_obj.Event310D_obj,sizeof(JProgramInfo->event_obj.Event310D_obj),event_para_save);
	readCoverClass(0x310E,0,&JProgramInfo->event_obj.Event310E_obj,sizeof(JProgramInfo->event_obj.Event310E_obj),event_para_save);
	readCoverClass(0x310F,0,&JProgramInfo->event_obj.Event310F_obj,sizeof(JProgramInfo->event_obj.Event310F_obj),event_para_save);
	readCoverClass(0x3110,0,&JProgramInfo->event_obj.Event3110_obj,sizeof(JProgramInfo->event_obj.Event3110_obj),event_para_save);
	readCoverClass(0x3111,0,&JProgramInfo->event_obj.Event3111_obj,sizeof(JProgramInfo->event_obj.Event3111_obj),event_para_save);
	readCoverClass(0x3112,0,&JProgramInfo->event_obj.Event3112_obj,sizeof(JProgramInfo->event_obj.Event3112_obj),event_para_save);
	readCoverClass(0x3114,0,&JProgramInfo->event_obj.Event3114_obj,sizeof(JProgramInfo->event_obj.Event3114_obj),event_para_save);
	readCoverClass(0x3115,0,&JProgramInfo->event_obj.Event3115_obj,sizeof(JProgramInfo->event_obj.Event3115_obj),event_para_save);
	readCoverClass(0x3116,0,&JProgramInfo->event_obj.Event3116_obj,sizeof(JProgramInfo->event_obj.Event3116_obj),event_para_save);
	readCoverClass(0x3117,0,&JProgramInfo->event_obj.Event3117_obj,sizeof(JProgramInfo->event_obj.Event3117_obj),event_para_save);
	readCoverClass(0x3118,0,&JProgramInfo->event_obj.Event3118_obj,sizeof(JProgramInfo->event_obj.Event3118_obj),event_para_save);
	readCoverClass(0x3119,0,&JProgramInfo->event_obj.Event3119_obj,sizeof(JProgramInfo->event_obj.Event3119_obj),event_para_save);
	readCoverClass(0x311A,0,&JProgramInfo->event_obj.Event311A_obj,sizeof(JProgramInfo->event_obj.Event311A_obj),event_para_save);
	readCoverClass(0x311B,0,&JProgramInfo->event_obj.Event311B_obj,sizeof(JProgramInfo->event_obj.Event311B_obj),event_para_save);
	readCoverClass(0x311C,0,&JProgramInfo->event_obj.Event311C_obj,sizeof(JProgramInfo->event_obj.Event311C_obj),event_para_save);
	readCoverClass(0x3200,0,&JProgramInfo->event_obj.Event3200_obj,sizeof(JProgramInfo->event_obj.Event3200_obj),event_para_save);
	readCoverClass(0x3201,0,&JProgramInfo->event_obj.Event3201_obj,sizeof(JProgramInfo->event_obj.Event3201_obj),event_para_save);
	readCoverClass(0x3202,0,&JProgramInfo->event_obj.Event3202_obj,sizeof(JProgramInfo->event_obj.Event3202_obj),event_para_save);
	readCoverClass(0x3203,0,&JProgramInfo->event_obj.Event3203_obj,sizeof(JProgramInfo->event_obj.Event3203_obj),event_para_save);
}
time_t ifDevReset()
{
	int i=0;
	if (DevResetNum !=JProgramInfo->oi_changed.reset)
	{
		fprintf(stderr,"\ncjmain判断需要设备复位");
		for(i=0;i<PROJECTCOUNT;i++)
		{
			if (JProgramInfo->Projects[i].ProjectID >0)
			{
				fprintf(stderr,"\nID=%d  %s  需要停止",JProgramInfo->Projects[i].ProjectID,JProgramInfo->Projects[i].ProjectName );
				JProgramInfo->Projects[i].ProjectState = NeedStop;
			}
		}
		DevResetNum = JProgramInfo->oi_changed.reset;
		return (time(NULL));
	}
	return 0;
}

#define MMQNAMEMAXLEN    	32		//消息队列名称长度
#define MAXSIZ_PROXY_485    256
#define MAXNUM_PROXY_485    25
#define MAXSIZ_PROXY_NET    256
#define MAXNUM_PROXY_NET    25

typedef enum
{
	 cjcomm,
	 cjdeal
}PROGS_ID;

typedef struct
{
	PROGS_ID pid;    //消息队列服务器端进程号
	INT8U    name[MMQNAMEMAXLEN]; //消息队列名称
	INT32U   maxsiz; //数据缓冲容量最大值
	INT32U   maxnum; //mq最大消息个数
}mmq_attribute;

const static mmq_attribute mmq_register[]=
{
	{cjcomm,PROXY_485_MQ_NAME,MAXSIZ_PROXY_NET,MAXNUM_PROXY_NET},
	{cjdeal,PROXY_NET_MQ_NAME,MAXSIZ_PROXY_485,MAXNUM_PROXY_485},
};
void createmq()
{
	struct mq_attr attr;

	mmq_create((INT8S*)PROXY_485_MQ_NAME,&attr,O_RDONLY);
	mmq_create((INT8S*)PROXY_NET_MQ_NAME,&attr,O_RDONLY);

}
int main(int argc, char *argv[])
{
	pid_t pids[128];
	int i=0;
	struct sigaction sa1;

	Setsig(&sa1,ProjectMainExit);
	JProgramInfo = (ProgramInfo*)CreateShMem("ProgramInfo",sizeof(ProgramInfo),NULL);
	if (prog_find_pid_by_name((INT8S*)argv[0], pids) > 1)
		return EXIT_SUCCESS;

	if (argc >= 2 )
	{
		if (strcmp("all",argv[1])==0)
			ReadSystemInfo();
	}
	fprintf(stderr,"\ncjmain run!");
	ProgInit();
	time_t resetStart=0;
	DevResetNum =JProgramInfo->oi_changed.reset;
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
				case NeedStop:
					fprintf(stderr,"\n进程:%s %s  PID=%d 停止",JProgramInfo->Projects[i].ProjectName,JProgramInfo->Projects[i].argv[0],JProgramInfo->Projects[i].ProjectID);
					if(ProjectKill(JProgramInfo->Projects[i])==1)
						JProgramInfo->Projects[i].ProjectState = NowRun;
					break;
			}
			JProgramInfo->Projects[i].WaitTimes++;
		}
		if (resetStart==0)
			resetStart = ifDevReset();
		else
		{
			fprintf(stderr,"\n...%ld",time(NULL));
			if (abs(time(NULL)-resetStart)>=5)
			{
				system("reboot");
			}
		}
   	}
	return EXIT_SUCCESS;//退出
}
