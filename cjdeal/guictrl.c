/*
 * guictrl.c
 *
 *  Created on: 2017-1-4
 *      Author: wzm
 */
/*
新规范 液晶程序（第一次提交原始程序） V0.0
整个液晶的大体框架：
1、整个结构分为 ：1)控制线程来guictrl_thread()，主体逻辑函数lcd_ctl;
			   2)显示处理线程guishow_thread，主体逻辑为lcd_show。
2、其中液晶显示线程是以面向对象（借鉴控件的想法）的思路去显示页面的。
lcd_ctl接口延续之前376.1液晶按键控制逻辑，更新的是将功能-显示缓冲区写入驱动设备中，放入接口内;
lcd_show接口延续之前376.1液晶显示处理逻辑，更新的是将三个显示区域的显示处理放到同一个接口内处理;
注意：
   刷新的机制：方案1：定时检测LcdBuf是否有变化；
   	   	   	 方案2：要刷新的时刻置标志位;
   目前使用的是第二种，set_time_show_flag(1);置位刷新标志，现在刷新显示缓冲区放在来lcd_ctl内;
 */
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sys/reboot.h>
#include <wait.h>
#include <time.h>
#include <errno.h>
#include "guictrl.h"
//#ifdef CCTT_I
//#elif defined SPTF_III
//#include "lcdprt_fk.h"
//#endif

//#define MTRACE
extern Menu menu[];
MenuList *pmenulist_head;
pthread_t thread_key, thread_menu, thread_status, thread_lcm,thread_downstatus;//thread_send;
int thread_run;
Proxy_Msg Proxy_Msg_Data;

time_t offlinetime;
INT16U show_offtime;//液晶掉线显示时间

/*
软件方法是指编制一段时间大于100ms 的延时程序，在第一次检测到有键按下时， 
执行这段延时子程序使键的前沿抖动消失后再检测该键状态，如果该键仍保持闭合状态电 平，
则确认为该键已稳定按下，否则无键按下，从而消除了抖动的影响。
同理，在检测到按 键释放后，也同样要延迟一段时间，以消除后沿抖动，然后转入对该按键的处理。
*/
INT8U g_JZQ_TimeSetUp_flg;//是否设置时间，1,设置时间  0，没有设置时间。问题现象：如果设置时间，则进入轮显
extern int getkey();
char g_LcdPoll_Keypress;
extern void lcdpoll_show(ProgramInfo* jprograminfo);

//轮显
void lcd_poll(){
	sleep(1);
	lcdpoll_show(JProgramInfo);
}

//清屏
void clearAllScreen()
{
	gui_clrrect(rect_TopStatus);
	gui_clrrect(rect_BottomStatus);
	gui_clrrect(rect_Client);
	lcm_write();
}

void GuiQuitProcess() // TODO：线程退出处理函数
{
	thread_run = PTHREAD_STOP;
	clearAllScreen();
	deletemenu(pmenulist_head);
#ifdef MTRACE
	muntrace();
#endif
	lcm_close();
	gpio_writebyte((char*)"/dev/gpoLCD_LIGHT", 0);//背光
}

void init_gobal_variable(void)
{
//	thread_key=0; TODO:全局变量配置
//	thread_menu=0;
//	thread_status=0;
//	thread_lcm=0;
	g_LcdPoll_Flag = LCD_NOTPOLL; //TODO:是否进行轮显标志
//	struct sigaction sa_gui;
//	sa_gui.sa_handler = QuitProcess;
//	sigemptyset(&sa_gui.sa_mask);
//	sa_gui.sa_flags = 0;
//	sigaction(SIGTERM, &sa_gui, NULL);
//	sigaction(SIGSYS, &sa_gui, NULL);
//	sigaction(SIGPWR, &sa_gui, NULL);
//	sigaction(SIGKILL, &sa_gui, NULL);
//	sigaction(SIGQUIT, &sa_gui, NULL);
//	sigaction(SIGILL, &sa_gui, NULL);
//	sigaction(SIGINT, &sa_gui, NULL);
//	sigaction(SIGHUP, &sa_gui, NULL);
//	sigaction(SIGABRT, &sa_gui, NULL);
//	sigaction(SIGBUS, &sa_gui, NULL);
//	sig_set(&sa_gui,QuitProcess);
}
#ifdef FB_SIM
extern int initX11();
#endif

extern void initliangdu();
extern void initlunxian();

/*
 * 退出程序处理
 * */
void gui_thread_quit_deal()
{
	gpio_writebyte((char*)LED_RUN, LED_OFF);
	gpio_writebyte((char*)LED_ALARM, LED_OFF);
}
/*
 * 轮讯按键键值，控制显示，向终端显卡驱动写入显示缓冲区，控制液晶轮显标志;
 * */
void lcd_ctl()
{
	INT8U Key_State=0;//0：无按键 1：刚按下键 2：延时时间内仍有按键
	time_t Time_PressKey, curtime; //最后一次按键时刻
	int keypress=0, presskey_first=0, presskey_qudou=0, presskey_qudou_old=0, counter=0;
	curtime = Time_PressKey = time(NULL);
	int count=0;
	g_LcdPoll_Keypress = 0;
	g_JZQ_TimeSetUp_flg = 0;
	while(1)
	{
		if(thread_run == PTHREAD_STOP)
			break;
		if(ret_show_time_flag())
		{
			set_time_show_flag(0);
			lcm_write();
		}
		usleep(50*1000);
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

		//集中器在断电后90秒关机，如果此时有按键被按，计时归零。
		//下面这段代码，就是当底板断电的情况下，如果有按键被按
		//就给vmain发消息，vmain将计时归零
			//TODO:断电按键检测
//			if(pwr_has() == FALSE)
//			{
//				mqd_t mqd;
//				INT8U sendBuf[2];
//				memset(sendBuf, 0, 2);
//TODO:给vmain发消息控制断电背光时间 cjmain创建消息队列
//				mqd =  createMsg((INT8S*)COM_VMAIN_MQ, O_WRONLY);
//				if(mqd>=0)
//				{
//					if(sendMsg(mqd, PRESSKEY, (INT8S*)sendBuf, 1)<0){
//						colseMsg(mqd);
//						fprintf(stderr, "\n gprs_REV_MQ:mq_open_ret=%d  error!!!",mqd);
//					}
//					fprintf(stderr,"Press Key send msg to vmain\r\n");
//				}
//				colseMsg(mqd);
//			}
			//PrintKey(PressKey);
			Time_PressKey = time(NULL);
			if(g_LcdPoll_Flag==LCD_INPOLL){
				g_LcdPoll_Keypress  = 1;
			}
			g_LcdPoll_Flag = LCD_NOTPOLL;
			gpio_writebyte((char*)"/dev/gpoLCD_LIGHT", 1);
		}
		counter = (counter+1)%10000;
		if(counter%50==0)
			curtime = time(NULL);
		if(g_JZQ_TimeSetUp_flg==1)
			Time_PressKey = curtime;
		if(abs(curtime-Time_PressKey)>POLLTIME && g_LcdPoll_Flag==LCD_NOTPOLL){
			g_LcdPoll_Flag = LCD_INPOLL;
			gpio_writebyte((char*)"/dev/gpoLCD_LIGHT", 0);
		}
	}
}
//非轮显显示函数接口
void lcd_not_poll()
{
	ShowItself(pmenulist_head->node.child);
}

/*
 * 液晶显示初始化函数：
 * 1.液晶主界面显示;
 * 2.初始化菜单项链表;
 * */
void gui_show_init()
{
	int menu_count=0;
	showmain();
	set_time_show_flag(1);//配置刷新标志
	menu_count = getMenuSize();
	pmenulist_head = ComposeDList(menu, menu_count);//构建菜单项链表，菜单项链表的构建和显示是分离的，这是一种设计思想
}

/*
 *液晶控制线程，控制和显示分开
 * */
void* guictrl_thread()
{
	lcd_ctl();
	thread_run = PTHREAD_STOP;
	gui_thread_quit_deal();//TODO:清空显示器
//	fprintf(stderr,"\n\n 液晶线程退出！！！");
//	pthread_detach(pthread_self());
	pthread_exit(&thread_guishow);
}
/*
 *液晶显示线程，控制和显示分开
 * */
void* guishow_thread()
{
	gui_show_init();
//	fprintf(stderr,"\n菜单项链表初始化成功!!!\n");
	while(PTHREAD_RUN == thread_run)
	{
		if((LCD_INPOLL==g_LcdPoll_Flag))//开始轮显
		{
			lcd_poll();
//			fprintf(stderr,"\n开始轮显！！！\n");
		}
		else if((LCD_NOTPOLL==g_LcdPoll_Flag))//非轮显显示
		{
			lcd_not_poll();
		}
		usleep(100*1000);
	}
	pthread_exit(&thread_guictrl);
}

void guictrl_proccess()
{
	fprintf(stderr, "\n CJGUI compile time:%s %s", __DATE__,__TIME__);
	if(lcm_open()<0){
		fprintf(stderr,"\n\n open fb0 fail!!!!! return");
		return;
	}
	ProgramInfo_register(JProgramInfo);//注册共享内存
	Proxy_Msg_Data_register(&Proxy_Msg_Data);//注册消息结构体指针
	lcm_ac_power();
	thread_run = PTHREAD_RUN;
	init_gobal_variable();//初始化全局变量
	initmenu();//初始化液晶菜单
	ReadHzkBuff();//读字库
	ReadHzkBuff_12();
	initliangdu();//初始化液晶亮度
	initlunxian();//读配置文件
	gpio_writebyte((char*)"/dev/gpoLCD_LIGHT", 1);//背光
	setFontSize(12);//设置字体
	pthread_attr_init(&guictrl_attr_t);
	pthread_attr_setstacksize(&guictrl_attr_t,2048*1024);
	pthread_attr_setdetachstate(&guictrl_attr_t,PTHREAD_CREATE_DETACHED);
	while ((thread_guictrl_id=pthread_create(&thread_guictrl, &guictrl_attr_t, (void*)guictrl_thread, NULL)) != 0)
	{
		sleep(1);
	}
	pthread_attr_init(&guishow_attr_t);
	pthread_attr_setstacksize(&guishow_attr_t,2048*1024);
	pthread_attr_setdetachstate(&guishow_attr_t,PTHREAD_CREATE_DETACHED);
	while((thread_guishow_id=pthread_create(&thread_guishow,&guishow_attr_t,(void*)guishow_thread,NULL)) != 0)
	{
		sleep(1);
	}
}
