#ifndef COMM_H_
#define COMM_H_
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <linux/reboot.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <mqueue.h>
#include <errno.h>
#include <sys/stat.h>
#include <math.h>
#include "sys/reboot.h"
//按键定义-
#define OFFSET_U	0
#define OFFSET_D	1
#define OFFSET_L	2
#define OFFSET_R	3
#define OFFSET_O	4
#define OFFSET_E	5
#define OFFSET_C	6

#define NOKEY	0
#define UP		(1<<OFFSET_U)
#define DOWN	(1<<OFFSET_D)
#define LEFT	(1<<OFFSET_L)
#define RIGHT	(1<<OFFSET_R)
#define OK		(1<<OFFSET_O)
#define ESC		(1<<OFFSET_E)
#define CANCEL	(1<<OFFSET_C)

//asc码表
#define ACK		0x06
#define CAN 	0x18

#define BEFORE	1
#define MIDDLE	2
#define AFTER	3

#define YES		1
#define NO		0

#define FOCUS	1
#define NOFOCUS	2

//#define delay(A) usleep((A)*1000)

extern int PressKey; //全局变量 按键值

#define _USERDIR_ 	"/nand/bin"

#define LED_ON 	1
#define LED_OFF 0

//液晶长宽像素
#define LCM_X	160//液晶长
#define LCM_Y	160//液晶宽
#define FONTSIZE    6  //每个字节的像素大小
#define FONTSIZE_8 	8

extern unsigned char LcdBuf[LCM_X*LCM_Y]; //液晶显存

extern  volatile char g_LcdPoll_Flag; //液晶是否在轮显中
#define LCD_INPOLL  	1  //在轮显中
#define LCD_NOTPOLL 	2  //不在轮显中

//液晶反选标志
#define LCD_REV 	1   //反显
#define LCD_NOREV 	0	//不反显
//菜单是否需要密码
#define MENU_ISPASSWD_EDITMODE 2  //需要密码 并且需要查看/编辑模式
#define MENU_ISPASSWD 1  //需要密码 并且需要查看/编辑模式
#define MENU_NOPASSWD 0	//不需要密码

//msgbox类型
#define BOXTYPE_EDIT		1 //有编辑框的
#define BOXTYPE_LABEL		2	//无编辑框的
#define BOXTYPE_TIME		3	//时间框
#define BOXTYPE_CHANGPASSWD	4	//修改密码
#define BOXTYPE_JZQDIZHI_10	5	//更改集中器地址
#define BOXTYPE_JZQDIZHI_16	6	//更改集中器地址
#define BOXTYPE_COMBOX		7	//用于搜表
#define BOXTYPE_IP			8	//用于搜表
#define BOXTYPE_HEART		9	//用于心跳

#define FRAME		1  //有边框
#define NOFRAME		0 //无边框

#define RETFLG		1  //此控件返回的值返回到msgbox
#define NORETFLG	0 //无返回

//控件类型
#define CTRL_EDIT 			1
#define CTRL_BUTTON_OK 		2
#define CTRL_BUTTON_CANCEL 	3
#define CTRL_COMBOX 		4
#define CTRL_COUNTER 		5
#define CTRL_EDITIP 		6
#define CTRL_EDITTIME 		7

//编辑框模式
#define SHOWMODE	1  	//显示模式
#define EDITMODE	2	//编辑模式
#define NORMALMODE	3	//退出模式

#define PASSWD_OK  1
#define PASSWD_ERR  0
#define PASSWD_ESC 3

//#define FB_SIM 1  //x86下的调试

//载波状态    0、空闲  1、载波初始化 	2、载波正在抄表中 	3、正在同步档案	4、正在搜表中
//gprs状态   1、检测gprs模块	2、gprs拨号中	3、正在连接主站  	4、终端在线
#define ZB_IDLE 			0
#define ZB_INIT 			1
#define ZB_METERREADING 	2
#define ZB_SYNCMETER	 	3
#define ZB_SEARCHMETER	 	4

#define GPRS_MODEM_INIT		0
#define GPRS_CHECKMODEM 	1
#define GPRS_GETVER			2
#define GPRS_SIMOK			3
#define GPRS_CREGOK			4

#define GPRS_DIALING 		5
#define GPRS_CONNECTING	 	6
#define GPRS_ONLINE		 	7

//通信方式
#define GPRS_COM	    1
#define NET_COM			2
#define SER_COM			3
//#define COMMTYPE_GPRS 	1  //GPRS通信
//#define COMMTYPE_LAN 	2	//RJ45通信
//#define COMMTYPE_COM 	3	//串口通信
#define COMMTYPE_SMS 	4	//短消息通信
#define COMMTYPE_CDMA 	5	//CDMA通信
#define COMMTYPE_WAN 	6	//无线电台通信
//#define DEBUG_X86 1
#ifdef DEBUG_X86
#define ParaAll shmm_getpara()
#else
//const para_all const* ParaAll;       //共享内存
#endif
//key.c
extern int gpio_write(char *devname, int data);
extern int gpio_read(char *devname);
extern int g_curcldno;
//判断线程是否在运行
#define PTHREAD_RUN 	1
#define PTHREAD_STOP 	2

#define PORT_ZB  	0xF209
#define PORT_485  	0xF201
#define PORT_JC		0xF208

#define PROTOCOL_97 1
#define PROTOCOL_07 30
#define PROTOCOL_JC 2
//键盘类型
#define KEYBOARD_ASC 1
#define KEYBOARD_DEC 2
#define KEYBOARD_HEX 3

//液晶轮显
#define LCDPOLL_INTERNAL 8//国网要求8秒

#define POLLTIME_SPTF_III	60//III型专变轮显超时时间
#define POLLTIME_I_II		45//I, II型集中器轮显超时时间


#endif
