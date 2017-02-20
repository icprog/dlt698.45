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
#include "main.h"

static char
		*usage_para =
					"\n--------------------参变量类对象----------------------------\n"	\
					"[电气设备] "
					"		 【数据初始化】cj para method 4300 3		\n"					\
					"-------------------------------------------------------\n\n"	\
					;

static char
		*usage_inoutdev =
					"\n-------------------A.12　输入输出设备类对象----------------------------\n"	\
					"\n---------文件传输类对象 	ESAM接口类对象 	输入输出设备类对象 	显示类对象--------\n"	\
					"【开关量输入】cj dev pro f203 				\n"					\
					"[初始化通信参数]  cj dev init <oi> :	例如：初始化通信参数  cj dev init 4500 	\n" \
					"【安全模式参数读取】cj dev pro f101 			\n"					\
					"【安全模式参数设置,0:不启用，1:启用】cj dev set f101 <0/1>		\n"	\
					"-------------------------------------------------------\n\n"	\
					;

static char
		*usage_coll =
					"\n--------------------采集监控类对象----------------------------\n"	\
					"[清除配置]cj coll clear <oi>	\n"					\
					"[删除一个配置单元]cj coll delete <oi> <id>  	id=【1..255】	\n"		\
					"[采集档案配置表读取]cj coll pro 6000	\n"					\
					"[任务配置单元] cj coll pro 6013 <任务号> [读取任务配置单元]\n" 		\
					"			  cj coll pro 6013 任务ID 执行频率 方案类型 方案编号 开始时间 结束时间 延时 执行优先级 状态 运行时段 起始小时:起始分钟 结束小时:结束分钟\n"	\
					"             cj coll pro 6013 1 1-5 1 1 2016-11-11 0:0:0 2099-9-9 9:9:9 1-2 2 1 0 0:0-23:59\n" 		\
					"[普通采集方案] cj coll pro 6015 <采集方案号>\n" 		\
					"[采集任务监控] cj coll pro 6035 <采集方案号>\n" 		\
					"-------------------------------------------------------\n\n"	\
					;
static char
		*usage_event =
					"--------------------事件类对象----------------------------\n"	\
					"[初始化事件参数]  cj event init <oi> :例如：初始化采集终端初始化事件  cj event init 0x3100/0全部 	\n"
					"[复位事件]  cj event reset <oi> :例如：复位采集终端初始化事件  cj event reset 0x3100 	\n"					\
					"[读取事件属性] cj event pro <oi> :例如：读取采集终端初始化事件属性 cj event pro 0x3100 	\n"					\
					"[设置Class7]  cj event pro <oi> 当前记录数 最大记录数 上报标识 有效标识 关联对象个数 关联对象OAD[1-10]	\n"	\
					"	[设置采集终端初始化事件] cj event pro 3100 1 16 1 1 0 \n"	\
					"	[设置终端状态量变位事件] cj event pro 3104 1 16 1 1 5 201E-4200 F203-4201 F203-4202 F203-4203 F203-4204 F203-4205\n"		\
					"[读取事件记录] cj event record <oi> 0（所有）/n（记录n）:例如：读取采集终端初始化事件记录 cj event record 0x3100 0（所有）/1(记录1)"
					"-------------------------------------------------------\n\n"	\
					;

static char
		*usage_acs =
			"--------------------终端交采计量校表及维护命令----------------------------\n"	\
			"acs acreg   <Pa Pb Pc Qa Qb Qc Ua Ub Uc Ia Ib Ic >   [同时校正三相系数，输入值为单相标准值]\n\n" 			\
			"		 [三相四交采校表:准备工作：标准源输入220V,3A,角度=60“C(0.5L 感性)]\n" 	\
			"         例如输入：cj acs acreg 330.00 330.00 330.00 572.00 572.00 572.00 220.0 220.0 220.0 3 3 3\n"   			\
			"         [参数输入标准源显示值，可输入浮点数。]\n"					\
			"		<Pa 0 Pc Qa 0 Qc Uab 0 Uca Ia 0 Ic >\n"						\
			"		 [三相三交采校表:准备工作：标准源输入100V,3A,角度=1“C(1L 感性)]\n" 	\
			"         例如输入：cj acs acreg 259.8076 0 259.8076 150 0 -150 100 0 100 3 0 -3\n"   			\
			"acs acphase   <Pa Pb Pc Qa Qb Qc Ua Ub Uc Ia Ib Ic >    [小电流相位校正：同时校正三相系数，输入值为单相标准值]\n\n"		\
			"		 [ATT7022E 交采校表:准备工作：标准源输入220V,0.3A,角度=60“C(0.5L 感性)]\n"	\
			"         例如输入：cj acs acphase 165.00 165.00 165.00 285.79 285.79 285.79 220.0 220.0 220.0 220.0 1.5 1.5 1.5\n"				\
			"         [参数输入标准源显示值，可输入浮点数。]\n"			\
			"		 [ATT7022E-D 交采校表:准备工作：标准源输入220V,1.5A,角度=60“C(0.5L 感性)]\n"	\
			"         例如输入：cj acs acphase 165 165 165 286 286 286 220 220 220 1.5 1.5 1.5\n"				\
			"         [参数输入标准源显示值，可输入浮点数。]\n"			\
			"		[三相三交采校表:准备工作：标准源输入100V,1.5A,角度=1“C(1L 感性)]\n" 	\
			"         例如输入：cj acs acphase 129.9 0 129.9 75 0 -75 100 0 100 1.5 0 1.5\n"   			\
			"acs acphase0  <Pa Pb Pc Qa Qb Qc Ua Ub Uc Ia Ib Ic >  [(7022E-d型芯片支持)电流相位校正：同时校正三相系数，输入值为单相标准值]\n\n"		\
			"		 [交采校表:准备工作：标准源输入220V,0.15A,角度=60“C(0.5L 感性)]\n"	\
			"         例如输入：cj acs acphase0 16.5 16.5 16.5 28.6 28.6 28.6 220 220 220 0.15 0.15 0.15\n"				\
			"         [参数输入标准源显示值，可输入浮点数。]\n"						\
			"		[三相三交采校表:准备工作：标准源输入100V,0.3A,角度=1“C(1L 感性)]\n" 	\
			"         例如输入：cj acs acphase0 25.9 0 25.9 15 0 -15 100 0 100 0.3 0 0.3\n"   						\
			"acs acregclean [清除（交采）校表系数]\n" 									\
			"acs acregdata	[读校表系数]\n" 								\
			"acs acdata  [打印测量点1（交采）实时数据]\n" 								\
			"acs ace  	 [测量点1（交采）电能示值数据]\n" 								\
			"acs acrndata	[读RN8209校表系数]\n" 								\
			"acs checku <U> [rn8209交采电压校正：输入标准源显示值]\n" 		\
			"        例如输入：cj acs checku 220.00 \n"   \
			"        [参数输入标准源显示值，可输入浮点数。]\n"			\
			"-------------------------------------------------------\n\n"	\
		;

void prthelp()
{
	fprintf(stderr,"Usage: ./cj (维护功能)  ");
	fprintf(stderr,"help	 [help] ");
	fprintf(stderr,"%s",usage_acs);
	fprintf(stderr,"%s",usage_para);
	fprintf(stderr,"%s",usage_event);
	fprintf(stderr,"%s",usage_coll);
	fprintf(stderr,"%s",usage_inoutdev);

}
void dog_feed()
{
	INT32S fd = -1;
	INT32S tm = 888888;
	system("pkill cjmain");
	sleep(1);
	if((fd = open(DEV_WATCHDOG, O_RDWR | O_NDELAY)) == -1)
	{
		fprintf(stderr, "\n\r open /dev/watchdog error!!!");
		return ;
	}
	write(fd,&tm,sizeof(int));
	close(fd);
	system("pkill cjcomm");
	system("pkill cjdeal");
	system("pkill gsmMuxd");
}
int main(int argc, char *argv[])
{
	usleep(10);
	if(argc<2) {
		prthelp();
		return EXIT_SUCCESS;
	}
	if (strcmp("ip",argv[1])==0) {
		SetIPort(argc,argv);
		return EXIT_SUCCESS;
	}
	if (strcmp("apn",argv[1])==0) {
		SetApn(argc,argv);
		return EXIT_SUCCESS;
	}

	if (strcmp("dog",argv[1])==0 || strcmp("stop",argv[1])==0) {
		dog_feed();
		return EXIT_SUCCESS;
	}
	if(strcmp("help",argv[1])==0) {
		prthelp();
		return EXIT_SUCCESS;
	}
	if(strcmp("event",argv[1])==0)
	{
		fprintf(stderr,"%s",usage_event);
		event_process(argc,argv);
		return EXIT_SUCCESS;
	}
	if(strcmp("para",argv[1])==0)
	{
		fprintf(stderr,"%s",usage_para);
		para_process(argc,argv);
		return EXIT_SUCCESS;
	}
	if(strcmp("coll",argv[1])==0)
	{
		fprintf(stderr,"%s",usage_coll);
		coll_process(argc,argv);
		return EXIT_SUCCESS;
	}
	if(strcmp("dev",argv[1])==0)
	{
		fprintf(stderr,"%s",usage_inoutdev);
		inoutdev_process(argc,argv);
		return EXIT_SUCCESS;
	}
	if(strcmp("acs",argv[1])==0)
	{
		fprintf(stderr,"%s",usage_acs);
		acs_process(argc,argv);
		return EXIT_SUCCESS;
	}
	if (strcmp("test",argv[1])==0)
	{
		fprintf(stderr,"\n自组报文\n");
		cjframe(argc,argv);
		return EXIT_SUCCESS;
	}
	prthelp();
	return EXIT_SUCCESS;
}
