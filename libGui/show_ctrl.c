/*
 * show_ctrl.c
 *
 *  Created on: 2017-3-7
 *      Author: prayer
 */
#include <string.h>
#include <time.h>
#include "comm.h"
#include "lcd_led.h"
#include "gui.h"
#include "show_ctrl.h"
#define MAX_INTERVAL 2//显示开关切换最大间隔时间 单位s
#define LED_EC_COUNT 10 //告警灯闪烁时长
volatile static time_t curts,oldts;
ProgramInfo* p_JProgramInfo;//共享内存指针，由ProgramInfo_register(ProgramInfo* JProgramInfo)初始化，在guictrl.c中调用
Proxy_Msg* p_Proxy_Msg_Data;//液晶给抄表发送代理处理结构体，指向由guictrl.c配置的全局变量
extern void lcd_showTopStatus();
extern void lcd_showBottomStatus(int zb_status, int gprs_status);
#ifdef JIANGSU
void lcd_Bottom_allshow()
{
	INT8U stepx=16;
	char str[3];
	Point icon_pos;
	memset(str,0,sizeof(str));
	memset(&icon_pos,0,sizeof(Point));
	icon_pos.x+=6;
	icon_pos.y=145;
	str[0]=0x0f;
	str[1]=0x10;
	gui_textshow_16(str, icon_pos, LCD_NOREV);//以太网小电脑
	icon_pos.x+=stepx;
	str[0]=0x28;
	str[1]=0x29;
	gui_textshow_16(str, icon_pos, LCD_NOREV);//维护口
	icon_pos.x+=stepx;
	str[0]=0x15;
	str[1]=0x16;
	gui_textshow_16(str, icon_pos, LCD_NOREV);//GPRS
	icon_pos.x+=stepx;
	str[0]=0x0a;
	str[1]=0x0b;
	gui_textshow_16(str, icon_pos, LCD_NOREV);//电话1
	icon_pos.x+=stepx;
	str[0]=0x0a;
	str[1]=0x0c;
	gui_textshow_16(str, icon_pos, LCD_NOREV);//电话2
	icon_pos.x+=stepx;
	str[0]=0x0a;
	str[1]=0x0d;
	gui_textshow_16(str, icon_pos, LCD_NOREV);//电话3
	icon_pos.x+=stepx;
	str[0]=0x0a;
	str[1]=0x0e;
	gui_textshow_16(str, icon_pos, LCD_NOREV);//电话4
	icon_pos.x+=stepx;
	str[0]=0x17;
	str[1]=0x18;
	gui_textshow_16(str, icon_pos, LCD_NOREV);//USB
	icon_pos.x+=stepx;
	str[0]=0x19;
	str[1]=0x1a;
	gui_textshow_16(str, icon_pos, LCD_NOREV);//存储卡
	set_time_show_flag(1);//TODO:new
}

void icon_work(Point icon_pos,char *str,INT8U *para_state,INT8U *para_delay)
{
	char char_icon[3];
	memset(str,0,sizeof(str));
	memset(char_icon,0,3);
	if(*para_state == 1)
	{
		(*para_delay)++;
		if((*para_delay)%2==0)//str空值，图案消失
		{
			char_icon[0]=' ';
			char_icon[1]=' ';
			gui_textshow_16(char_icon, icon_pos, LCD_NOREV);
			*para_delay=0;
			*para_state=0;
		}
		else
		{
			gui_textshow_16(str, icon_pos, LCD_NOREV);
		}
		set_time_show_flag(1);//TODO:new
	}
	else
		*para_delay=0;
}

void lcd_Bottom_bar()
{
	Point icon;
	memset(&icon,0,sizeof(Point));
	INT8U stepx=16;
	char str[3];
	memset(str,0,sizeof(str));

	static INT8U ethernet_delay=0;
	static INT8U weihu_delay=0;
	static INT8U gprs_delay=0;
	static INT8U telep1_delay=0;
	static INT8U telep2_delay=0;
	static INT8U telep3_delay=0;
	static INT8U telep4_delay=0;
	static INT8U usb_delay=0;
	static INT8U sdcard_delay=0;
	icon.x+=6;
	icon.y=145;
	lcd_Bottom_allshow();
	str[0]=0x0f;
	str[1]=0x10;
	icon_work(icon,str,&shmm_getdevstat()->Rev_flag[0],&ethernet_delay);
	icon.x+=stepx;
	str[0]=0x28;
	str[1]=0x29;
	icon_work(icon,str,&shmm_getdevstat()->Rev_flag[1],&weihu_delay);
	icon.x+=stepx;
	str[0]=0x15;
	str[1]=0x16;
	icon_work(icon,str,&shmm_getdevstat()->Rev_flag[2],&gprs_delay);
	icon.x+=stepx;
	str[0]=0x0a;
	str[1]=0x0b;
	icon_work(icon,str,&shmm_getdevstat()->Rev_flag[3],&telep1_delay);
	icon.x+=stepx;
	str[0]=0x0a;
	str[1]=0x0c;
	icon_work(icon,str,&shmm_getdevstat()->Rev_flag[4],&telep2_delay);
	icon.x+=stepx;
	str[0]=0x0a;
	str[1]=0x0d;
	icon_work(icon,str,&shmm_getdevstat()->Rev_flag[5],&telep3_delay);
	icon.x+=stepx;
	str[0]=0x0a;
	str[1]=0x0e;
	icon_work(icon,str,&shmm_getdevstat()->Rev_flag[6],&telep4_delay);
	icon.x+=stepx;
	str[0]=0x0a;
	str[1]=0x0e;
	icon_work(icon,str,&shmm_getdevstat()->Rev_flag[7],&usb_delay);
	icon.x+=stepx;
	str[0]=0x0a;
	str[1]=0x0e;
	icon_work(icon,str,&shmm_getdevstat()->Rev_flag[8],&sdcard_delay);
}
#endif

void ProgramInfo_register(ProgramInfo* JProgramInfo)
{
	p_JProgramInfo = JProgramInfo;
}

void Proxy_Msg_Data_register(Proxy_Msg* Proxy_Msg_Data)
{
	p_Proxy_Msg_Data = Proxy_Msg_Data;
}
/*
 *显示选择开关函数
 * */
INT8U switch_show_ret()
{
	INT8U ret = 0;
	curts = time(NULL);
	if((curts - oldts) < MAX_INTERVAL)//小于2s
	{
		ret = 1;
	}
	else if((curts - oldts) < MAX_INTERVAL*2)//大于等于2s小于4s
	{
		ret = 2;
	}
	else if((curts - oldts) >= MAX_INTERVAL*2)//大于等于4s
	{
		oldts = curts;
		ret = 1;
	}
	return ret;
}
/*
 * 状态运行灯控制
 * */
void lcd_showstatus()
{
//	INT8U ec1_old=0, ec2_old=0, Clock_Batt_Voltage_flg=0;
	static char alarmled_count=0;
	volatile char led_run_state=0, led_alarm_state=0;
//	char erc_flg=0;
//	TS curts, oldts;
//	tmget(&curts);
//	memcpy(&oldts, &curts, sizeof(TS));
//	dbg_prt("\n lcd_showled process = %d", (int)pthread_self());
//	ec1_old = shmm_getdevstat()->erc.EC1;
//	ec2_old = shmm_getdevstat()->erc.EC2;
//	while(1){
//		if(thread_run == PTHREAD_STOP){
//			gpio_writebyte((INT8S*)LED_RUN, LED_OFF);
//			gpio_writebyte((INT8S*)LED_ALARM, LED_OFF);
////			break;
//		}
//		tmget(&curts);
//		if(curts.Sec!=oldts.Sec){
//			memcpy(&oldts, &curts, sizeof(TS));
			//TODO:告警灯处理
//			if(shmm_getdevstat()->Clock_Batt_Voltage < MIN_BATTERY_VOL){//TODO:设备状态判断
//				Clock_Batt_Voltage_flg = 1;
//				alarmled_count = 0;
//			}else{
//				if(Clock_Batt_Voltage_flg == 1)
//					gpio_writebyte((INT8S*)LED_ALARM, 0);
//				Clock_Batt_Voltage_flg = 0;
//			}
//			if(shmm_getdevstat()->erc.EC1!=ec1_old){
//				alarmled_count = 0;
//				erc_flg = 1;
//				ec1_old = shmm_getdevstat()->erc.EC1;
//			}
//			if(shmm_getdevstat()->erc.EC2!=ec2_old){
//				alarmled_count = 0;
//				erc_flg = 1;
//				ec2_old = shmm_getdevstat()->erc.EC2;
//			}
//			if((erc_flg==1 && alarmled_count!=LED_EC_COUNT)||(Clock_Batt_Voltage_flg==1)){
//				led_alarm_state = (~led_alarm_state)&0x01;
//				gpio_writebyte((INT8S*)LED_ALARM, led_alarm_state);
//			}
//		if((Clock_Batt_Voltage_flg==1)){
//			led_alarm_state = (~led_alarm_state)&0x01;
//			gpio_writebyte((char*)LED_ALARM, led_alarm_state);
//		}
		alarmled_count++;
		if(alarmled_count>=LED_EC_COUNT){
			alarmled_count = LED_EC_COUNT;
//				erc_flg = 0;
			gpio_writebyte((char*)LED_ALARM, 0);
		}
			//TODO:运行灯处理
		if(p_JProgramInfo!=NULL)
		{
			if(p_JProgramInfo->dev_info.jzq_login==GPRS_COM||
					p_JProgramInfo->dev_info.jzq_login==NET_COM){
				led_run_state = (~led_run_state)&0x01;
				gpio_writebyte((char*)LED_RUN, led_run_state);
			}else
				gpio_writebyte((char*)LED_RUN, 1);
		}
//		}
//上状态条显示
		lcd_showTopStatus();
//		set_time_show_flag(1);//TODO:new
//		delay(600);
//	}
}
/*
 * 设备状态信息显示处理
 * */
void lcd_showdownstatus()
{
	int zb_status = ZB_IDLE;
	int gprs_status = GPRS_CHECKMODEM;
	if(p_JProgramInfo!=NULL){
		if(p_JProgramInfo->dev_info.gprs_status==1)
			gprs_status = GPRS_CHECKMODEM;//AT OK
		else if(p_JProgramInfo->dev_info.gprs_status==2)
			gprs_status = GPRS_GETVER;//GWR OK
		else if(p_JProgramInfo->dev_info.gprs_status==3)
			gprs_status = GPRS_SIMOK;//CPIN OK
		else if(p_JProgramInfo->dev_info.gprs_status==4){
			gprs_status = GPRS_CREGOK;//CREG OK
			if(p_JProgramInfo->dev_info.jzq_login==1)
				gprs_status = GPRS_ONLINE;
			else if(p_JProgramInfo->dev_info.connect_ok==1)
				gprs_status = GPRS_CONNECTING;
			else if(p_JProgramInfo->dev_info.pppd_status==1)
				gprs_status = GPRS_DIALING;
		}else if(p_JProgramInfo->dev_info.gprs_status==0)
			gprs_status = GPRS_MODEM_INIT;//init
		zb_status = p_JProgramInfo->dev_info.PLC_status;
	}
	lcd_showBottomStatus(zb_status, gprs_status);
}
//显示主界面
void showmain()
{
	time_t ts;
	struct tm curtm;
	Point pos;
	INT8U str[50];
	gui_clrrect(rect_Client);
	gui_setpos(&pos, rect_Client.left+FONTSIZE*5, rect_Client.top+FONTSIZE*4);
	memset(str, 0, 50);
	int fontsize=getFontSize();
	setFontSize(16);
//#ifdef CCTT_I
//#ifdef CCTT_I
	sprintf((char*)str, "低压集抄集中器");
//#else
//	sprintf((char*)str, "专变III型终端");
//#endif
	gui_textshow((char*)str, pos, LCD_NOREV);
	gui_setpos(&pos, rect_Client.left+7*FONTSIZE, rect_Client.top+8*FONTSIZE);
	ts = time(NULL);
	memset(str, 0, 50);
	localtime_r(&ts, &curtm);
	sprintf((char*)str, "%04d-%02d-%02d",curtm.tm_year+1900,curtm.tm_mon+1,curtm.tm_mday);
	gui_textshow((char*)str, pos, LCD_NOREV);
	gui_setpos(&pos, rect_Client.left+8*FONTSIZE, rect_Client.top+12*FONTSIZE);
	memset(str, 0, 50);
	sprintf((char*)str, "%02d:%02d:%02d", curtm.tm_hour,curtm.tm_min,curtm.tm_sec);
	gui_textshow((char*)str, pos, LCD_NOREV);
	setFontSize(fontsize);
	sleep(1);
}
//
void show_ctrl()
{
	switch(switch_show_ret())
	{
	case 1:
		lcd_showdownstatus();//显示液晶设备状态
		break;
	case 2:
		lcd_showstatus();//显示状态灯
		break;
	default:
		break;
	}
}
