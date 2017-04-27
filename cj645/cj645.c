/*
 * v645.c
 *
 *  Created on: 2014-2-28
 *      Author: Administrator
 */


#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <stdlib.h>

extern INT32S comfd;
extern INT8U addr[6];//表地址，低在前，高在后
extern void dealProcess();

//读表地址
void getAddr()
{
	INT16U TmnlAddr = (shmm_getpara()->f89.TmnlAddr[0]<<8) + shmm_getpara()->f89.TmnlAddr[1];
	int32u2bcd(TmnlAddr, &addr[0], inverted);
	memset(&addr[2], 0, 4);
}

void initMmq()
{
	struct mq_attr attr_main;
	struct mq_attr attr_645_net;
	struct mq_attr attr_645_com;

	//打开4852消息队列
	mqd_645_net = mmq_open((INT8S *)S485_2_REV_NET_MQ,&attr_645_net,O_RDONLY);
	if(mqd_645_net<0)	fprintf(stderr,"S485_2_REV_NET_MQ:mq_open_ret=%d\n",mqd_645_net);

	mqd_645_com = mmq_open((INT8S *)S485_2_REV_COM_MQ,&attr_645_com,O_RDONLY);
	if(mqd_645_com<0)	fprintf(stderr,"S485_2_REV_COM_MQ:mq_open_ret=%d\n",mqd_645_com);

	//打开main消息队列
	mqd_main = mmq_open((INT8S *)COM_VMAIN_MQ,&attr_main,O_WRONLY);
	if(mqd_main<0)	fprintf(stderr,"MAIN_REV_MQ:mq_open_ret=%d\n",mqd_main);
}


//void setsig(struct sigaction *psa,void (*pfun)(int signo))
//{
//	if (psa!=NULL)
//	{
//		psa->sa_handler = pfun;
//		sigemptyset(&psa->sa_mask);
//		psa->sa_flags = 0;
//		sigaction(SIGTERM, psa,NULL);
//		sigaction(SIGSYS, psa,NULL);
//		sigaction(SIGPWR, psa,NULL);
//		sigaction(SIGKILL, psa,NULL);
//		sigaction(SIGQUIT, psa,NULL);
//		sigaction(SIGILL, psa,NULL);
//		sigaction(SIGINT, psa,NULL);
//		sigaction(SIGHUP, psa,NULL);
//		sigaction(SIGABRT, psa,NULL);
//		sigaction(SIGBUS, psa,NULL);
//		signal(SIGPIPE,SIG_IGN);
//	}
//}
//}

//处理现场
void QuitProcess(int signo)
{
	if(comfd!=0) com_close(comfd);
	shmm_unregister();
	fprintf(stderr, "\n\r cj645 quit xxx\n\r");
	exit(0);
}


//主程序
int main(int argc, char *argv[])
{
	fprintf(stderr, "\ncj645 start ....\n\r");
	INT8U comport;

#if (defined(CCTT_I)||defined(SPTF_III))
	if(argc==2)
	{
		comport=atoi(argv[1]);
	}
	else
	{
		comport=S4852;
	}
#elif (defined CCTT_II)
	if(argc==2)
	{
		comport=atoi(argv[1]);
	}
	else
	{
		comport=4;
	}
#endif
	fprintf(stderr,"open /dev/ttyS%d\n", comport);

	struct sigaction _sa;
//	setsig(&_sa,QuitProcess);
	sig_set(&_sa,QuitProcess);
	//打开共享内存
	if(shmm_register()<0)
	{
		fprintf(stderr, "打开共享内存失败\n");
		QuitProcess(0);
	 	return EXIT_FAILURE;
	}

//	if ((comfd=com_open(S4852, 2400, (INT8U *)"even", 1, 8))<1)
	if ((comfd=com_open(comport, 2400, (INT8U *)"even", 1, 8))<1)
	{
		fprintf(stderr, "OpenCom645 ERR!!! ........................\n");
	}
	getAddr();
	initMmq();
	dealProcess();

	sleep(1);

	QuitProcess(0);
 	return EXIT_SUCCESS;
}
