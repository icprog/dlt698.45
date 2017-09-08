/*
 * show_ctrl.c
 *
 *  Created on: 2017-3-7
 *      Author: prayer
 */
#include <string.h>
#include <time.h>
#include "comm.h"
#include "gui.h"
#include "show_ctrl.h"
#include "ParaDef.h"
#include "basedef.h"

#define MAX_INTERVAL 1//显示开关切换最大间隔时间 单位s
#define LED_EC_COUNT 10 //告警灯闪烁时长
volatile static time_t curts,oldts;

/*
 * 共享内存指针
 * 由ProgramInfo_register(ProgramInfo* JProgramInfo)初始化
 * 在guictrl.c中调用
*/
ProgramInfo* p_JProgramInfo = NULL;

INT8U g_chgOI4001;
INT8U g_chgOI4300;//oi4300参数变更标记记录
INT8U g_chgOI4500;//oi4500参数变更标记记录
INT8U g_chgOI4510;

CLASS_4001_4002_4003 g_Class4001_4002_4003;
CLASS19 g_class19_oi4300;
CLASS25 g_class25_oi4500;
CLASS26 g_class26_oi4510;

Proxy_Msg* p_Proxy_Msg_Data;//液晶给抄表发送代理处理结构体，指向由guictrl.c配置的全局变量
extern void lcd_showTopStatus();
extern void lcd_showBottomStatus(int zb_status, int gprs_status);

void ProgramInfo_register(ProgramInfo* JProgramInfo)
{
	p_JProgramInfo = JProgramInfo;
}

void Proxy_Msg_Data_register(Proxy_Msg* Proxy_Msg_Data)
{
	p_Proxy_Msg_Data = Proxy_Msg_Data;
}
//cjdeal gui线程开头，调用该函数初始化库内全局变量
//先从文件中读取全局变量，失败后，全局变量归零
//将涉及到参数读取的项的oi_changed全部赋值成当前值。
void Init_GuiLib_variable()
{
	if (readCoverClass(0x4001, 0, (void*)&g_Class4001_4002_4003, sizeof(CLASS_4001_4002_4003), para_vari_save)<=0)//读取失败
	{
		memset(&g_Class4001_4002_4003,0,sizeof(CLASS_4001_4002_4003));
		printf("\nlibgui:read para  0x4001 err!!\n");
	}

	if(readCoverClass(0x4300, 0, (void*)&g_class19_oi4300, sizeof(CLASS19), para_vari_save)<=0)
	{
		memset(&g_class19_oi4300,0,sizeof(CLASS19));
		printf("\nlibgui:read para  0x4300 err!!\n");
	}
	if(readCoverClass(0x4500, 0, (void*)&g_class25_oi4500, sizeof(CLASS25), para_vari_save)<=0)
	{
		memset(&g_class25_oi4500,0,sizeof(g_class25_oi4500));
		printf("\nlibgui:read para  0x4500 err!!\n");
	}
	if(readCoverClass(0x4510, 0, (void*)&g_class26_oi4510,sizeof(CLASS26), para_vari_save)<=0)
	{
		memset(&g_class26_oi4510,0,sizeof(CLASS26));
		printf("\nlibgui:read para  0x4510 err!!\n");
	}
	if(p_JProgramInfo!=NULL	)
	{
		g_chgOI4001 = p_JProgramInfo->oi_changed.oi4001;
		g_chgOI4300 = p_JProgramInfo->oi_changed.oi4300;
		g_chgOI4500 = p_JProgramInfo->oi_changed.oi4500;
		g_chgOI4510 = p_JProgramInfo->oi_changed.oi4510;
	}
}
/*
 *显示选择开关函数
 * */
//INT8U switch_show_ret()
//{
//	INT8U ret = 0;
//	curts = time(NULL);
//	if((curts - oldts) < MAX_INTERVAL)//小于2s
//	{
//		ret = 1;
//	}
//	else if((curts - oldts) < MAX_INTERVAL*2)//大于等于2s小于4s
//	{
//		ret = 2;
//	}
//	else if((curts - oldts) >= MAX_INTERVAL*2)//大于等于4s
//	{
//		oldts = curts;
//		ret = 1;
//	}
//	return ret;
//}
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
			gpio_writebyte((char*)DEV_LED_ALARM, 0);
		}
//			//TODO:运行灯处理
//		if(p_JProgramInfo!=NULL)
//		{
//			if(p_JProgramInfo->dev_info.jzq_login==GPRS_COM||
//					p_JProgramInfo->dev_info.jzq_login==NET_COM ||p_JProgramInfo->dev_info.jzq_login == 3 ){  //3 内部协议栈上线
//				led_run_state = (~led_run_state)&0x01;
//				gpio_writebyte((char*)DEV_LED_RUN, led_run_state);
//			}else
//				gpio_writebyte((char*)DEV_LED_RUN, 1);
//		}
//上状态条显示
		lcd_showTopStatus();
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
			if(p_JProgramInfo->dev_info.jzq_login==1 || p_JProgramInfo->dev_info.jzq_login==3)//1/GPRS外部协议栈上线2/内部协议栈上线
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

	if(p_JProgramInfo->cfg_para.device  == CCTT1) {
		sprintf((char*)str, "低压集抄集中器");
	}else if(p_JProgramInfo->cfg_para.device  == SPTF3) {
		sprintf((char*)str, "专变III型终端");
	}
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
		lcd_showdownstatus();//显示下方状态栏
		lcd_showstatus();//显示状态灯和上方状态栏
}
