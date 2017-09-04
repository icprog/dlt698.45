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

Func_t func[] = { { "USB_test", USB_test, 1 },					//USB检测
									{ "softver_test", softver_test, 1 },		//软件版本和内核日期检测
									{ "esam_test", Esam_test, 1 },				//esam型号检测
									{ "485I_test", RS485I_test, 1 },				//485-1检测
									{ "485II_test", RS485II_test, 1 },			//485-2检测
									{ "RJ45_test", RJ45_test, 1 },				//RJ45检测
									{ "battery_test", battery_test, 1 },		//时钟电池电压检测
									{ "YX_test", YX_test, 1 }							//遥信检测
							};

pthread_t thread_key;
int thread_run;
pthread_attr_t attr_t;



void *lcd_key()
{
	INT8U Key_State = 0; //0：无按键 1：刚按下键 2：延时时间内仍有按键
	int keypress = 0, presskey_first = 0, presskey_qudou = 0,
			presskey_qudou_old = 0;
	int count = 0;
	DEBUG_TO_FILE(1, TEST_LOG_FILE, "\n lcd_key process = %d",
			(int )pthread_self());
	while (1) {
		if (thread_run == PTHREAD_STOP)
			break;
		delay(50);
		keypress = 0;
		presskey_qudou_old = presskey_qudou;
		presskey_qudou = getkey();
		switch (Key_State) {
		case 0:
			if (presskey_qudou != 0 && presskey_qudou_old == 0) {
				presskey_first = presskey_qudou;
				Key_State = 1;
			}
			break;
		case 1:
			if (presskey_qudou != 0) {
				count++;
				if (count >= 1) {
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
		if (keypress != 0) {
			PressKey = keypress;
			DEBUG_TO_FILE(1, TEST_LOG_FILE, "\n PressKey = %x", PressKey);
			gpio_writebyte((char*) "gpoLCD_LIGHT", 1);
		}
	}
	DEBUG_TO_FILE(1, TEST_LOG_FILE, "\n lcd_key pthread_exit = %d",
			(int )pthread_self());
	pthread_detach(pthread_self());
	pthread_exit((void*) 0);
}

void start_key()
{
	int thread_key_id = -1;
	thread_run = PTHREAD_RUN;
	pthread_attr_init(&attr_t);
	pthread_attr_setdetachstate(&attr_t, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&attr_t, 256000);
	thread_key_id = pthread_create(&thread_key, &attr_t, lcd_key, NULL); //按键
	if (thread_key_id != 0) {
		perror("\n thread_key created failed");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "\nLCD main process = %d", (int) pthread_self());
	delay(2000);
	pthread_detach(thread_key);
	return;
}

void QuitProcess(int signo)
{
	thread_run = PTHREAD_STOP;
	lcd_disp("clear", 0, 0);
	delay(2000);
	pthread_attr_destroy(&attr_t);
	//threads_wait();//pthread_join 等待所有的线程退出
	exit(0);
}

void readFuncCfg()
{
	int i = 0;
	char enable[] = "1";
	int funcCnt = 0;

	funcCnt = sizeof(func) / sizeof(Func_t);
	for (i = 0; i < funcCnt; i++) {
		if (readcfg(JZQTEST_PARA_NAME, func[i].name, enable) == 1) {
			func[i].enable = atoi(enable);
		}
	}
}

int main(int argc, char *argv[])
{
	int succflag = 0;
	int func_size = 0, i;
	char cmd[100] = { 0 };

	pid_t pids[128] = { 0 };
	if (prog_find_pid_by_name((INT8S *) argv[0], pids) > 1) {
		DEBUG_TO_FILE(1, "%s进程仍在运行,进程号[%d]，程序退出...", argv[0], pids[0]);
		return EXIT_SUCCESS;
	}

	gpio_writebyte((char *) DEV_LED_RUN, 0);
	sprintf(cmd, "rm %s", TEST_LOG_FILE);
	system(cmd);
	DEBUG_TO_FILE(1, TEST_LOG_FILE, "======================开始测试======================");
	ReadHzkBuff_12();
	openlight();
	start_key(); //启动按键线程

	lcd_disp("clear", 0, 0);
	func_size = sizeof(func) / sizeof(Func_t);
	readFuncCfg();
	ReadDeviceConfig(&g_cfg_para);
	for (i = 0; i < func_size; i++) {
		if (func[i].enable == 1)
			func[i].func();
		DEBUG_TO_FILE(1, TEST_LOG_FILE, "%s done", func[i].name);
	}

	DEBUG_TO_FILE(1, TEST_LOG_FILE, "======================测试结束======================");
	QuitProcess(0);
	return succflag;
}
