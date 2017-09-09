/*
 ============================================================================
 Name        : zdtest698.c
 Author      : s_baoshan
 Version     :
 Copyright   : free to copy, modify, distribute
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/syslog.h>
#include <sys/stat.h>
#include "pthread.h"
#include "zdtest698.h"
#include "function_test.h"

extern int readcfg(char *filename, char *name, char *ret);
extern void ReadHzkBuff_12();
extern int getkey();
Func_t func[]={
	{"USB", 	USB_test,		1},
	{"softver", softver_test,	1},
	{"esam", 	Esam_test,		1},
	{"485I", 	RS485I_test,	1},
	{"485II",	RS485II_test,	1},
	{"RJ45",	RJ45_test,		1},
	{"YX",		YX_test,		1}
};

pthread_t thread_key;
int thread_run;
pthread_attr_t attr_t;
int PressKey;
GUI_Point_t Gui_Point;

int func_config(){
	char buf[10];
	int enable=0, i, func_size;
	memset(buf, 0, 10);
	system("pwd");

	if(access(JZQTEST_PARA_NAME,0)!=0){
		SdPrint("%s no file!!!", JZQTEST_PARA_NAME);
		return 0;
	}
	func_size = sizeof(func)/sizeof(Func_t);

	return 1;
}

void initmem(){
//	JProgramInfo = OpenShMem("ProgramInfo",sizeof(ProgramInfo),NULL);
}

INT8U is_find_jLcdTask;
void find_jLcdTask_id(){
	int i;
	INT8U flg=0;
	for(i=0; i<PROJECTCOUNT; i++){
		if(memcmp((char*)JProgramInfo->Projects[i].ProjectName,"jLcdTask", strlen("jLcdTask"))==0)
		{
			ProjectNo = i;
			SdPrint("\n jLcdTask ProjectNo = %d", ProjectNo);
			flg = 1;
			break;
		}
	}
	is_find_jLcdTask = flg;
	return;
}

void ClearWaitTimes_test(INT8U ProjectID,ProgramInfo* JProgramInfo)
{
	if(is_find_jLcdTask==1)
		JProgramInfo->Projects[ProjectID].WaitTimes = 0;
}

void *lcd_key()
{
	INT8U Key_State=0;//0：无按键 1：刚按下键 2：延时时间内仍有按键
	int keypress=0, presskey_first=0, presskey_qudou=0, presskey_qudou_old=0;
	int count=0;
	SdPrint("\n lcd_key process = %d", (int)pthread_self());
	while(1){
		if(thread_run == PTHREAD_STOP)
			break;
		delay(50);
		keypress = 0;
		presskey_qudou_old = presskey_qudou;
		presskey_qudou = getkey();
		switch(Key_State){
		case 0:
			if(presskey_qudou!=0 && presskey_qudou_old==0){
				presskey_first = presskey_qudou;
				Key_State = 1;
			}
			break;
		case 1:
			if(presskey_qudou!=0){
				count++;
				if(count>=1){
					Key_State = 2;
					count = 0;
				}
			}
			break;
		case 2:
			keypress = presskey_first;
			Key_State = 0;
			break;
		}
		if(keypress!=0){
			PressKey = keypress;
			SdPrint("\n PressKey = %x", PressKey);
			//PrintKey(PressKey);
			gpio_write((char*)"gpoLCD_LIGHT", 1);
		}
	}
	SdPrint("\n lcd_key pthread_exit = %d", (int)pthread_self());
	pthread_detach(pthread_self());
	pthread_exit((void*)0);
}

void start_key(){
	int thread_key_id;
	thread_run = PTHREAD_RUN;
	pthread_attr_init(&attr_t);
	pthread_attr_setdetachstate(&attr_t,PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&attr_t,256000);
	thread_key_id = pthread_create(&thread_key, &attr_t, lcd_key, NULL);//按键
	if(thread_key_id!=0) {
		perror("\n thread_key created failed");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr,"\nLCD main process = %d", (int)pthread_self());
	delay(2000);
	pthread_detach(thread_key);
	return;
}

void QuitProcess(int signo)
{
	thread_run = PTHREAD_STOP;
	delay(2000);
	pthread_attr_destroy(&attr_t);
	//threads_wait();//pthread_join 等待所有的线程退出
   	exit(0);
}

int main(int argc,char *argv[])
{
	int	succflag=0;
	int func_size=0, i;

	//先查找集中器现在有没有运行698程序

//	system("cj dog");
//	system("cj stop");
//	system("cjmain&");
	memset(&Gui_Point, 0, sizeof(GUI_Point_t));
	initmem();
	SdPrint("\n\n");
	SdPrint("\n======================开始测试======================");
//	find_jLcdTask_id();
	ReadHzkBuff_12();
	openlight();
	start_key();//启动按键线程
	if(func_config()==0)//配置所测试的项目
		return 0;
	lcd_disp("clear", 0, 0);
	lcd_disp("开始测试", Gui_Point.x+4*FONTSIZE, Gui_Point.y);
	func_size = sizeof(func)/sizeof(Func_t);
	SdPrint("\n func_size=%d", func_size);
	for(i=0; i<func_size; i++){
		if(func[i].enable==1)
			func[i].func();
		ClearWaitTimes_test(ProjectNo,JProgramInfo);
	}

	SdPrint("\n======================测试结束======================");
	SdPrint("\n\n");
//	system("cjmain all 2> /dev/shm/null &");
	QuitProcess(0);
	return succflag;
}
