/*
 * lcd_status.c 液晶上下状态条
 *
 *  Created on: 2013-5-14
 *      Author: Administrator
 */
//#include "../include/stdafx.h"
#include "basedef.h"
#include "gui.h"
#include "show_ctrl.h"

int g_curcldno;
#define GPRSCSQ_L1 8 //信号强度
#define GPRSCSQ_L2 16
#define GPRSCSQ_L3 24

#define GPRSCSQ_LEN1 3  //信号柱高度
#define GPRSCSQ_LEN2 5
#define GPRSCSQ_LEN3 8
#define GPRSCSQ_LEN4 11
INT8U flag = 0;
void topstatus_showCSQ(INT16U gprscsq)
{
	Point wire_pos, csq_pos;
	//显示天线
	memset(&wire_pos, 0, sizeof(Point));
	char str[2];
	memset(str, 0, 2);
	str[0] = 0x1c;
	gui_textshow_16(str, wire_pos, LCD_NOREV);
	//显示信号强度
	memset(&csq_pos, 0, sizeof(Point));
	csq_pos.x = wire_pos.x + FONTSIZE_8;
	csq_pos.y = FONTSIZE_8 * 2 - 2;
	if(gprscsq>0){
		gui_vline(csq_pos, csq_pos.y-GPRSCSQ_LEN1);
		csq_pos.x++;
		gui_vline(csq_pos, csq_pos.y-GPRSCSQ_LEN1);
		csq_pos.x++;
	}
	if(gprscsq>GPRSCSQ_L1){
		csq_pos.x++;
		gui_vline(csq_pos, csq_pos.y-GPRSCSQ_LEN2);
		csq_pos.x++;
		gui_vline(csq_pos, csq_pos.y-GPRSCSQ_LEN2);
		csq_pos.x++;
	}
	if(gprscsq>GPRSCSQ_L2){
		csq_pos.x++;
		gui_vline(csq_pos, csq_pos.y-GPRSCSQ_LEN3);
		csq_pos.x++;
		gui_vline(csq_pos, csq_pos.y-GPRSCSQ_LEN3);
		csq_pos.x++;
	}
	if(gprscsq>GPRSCSQ_L3){
		csq_pos.x++;
		gui_vline(csq_pos, csq_pos.y-GPRSCSQ_LEN4);
		csq_pos.x++;
		gui_vline(csq_pos, csq_pos.y-GPRSCSQ_LEN4);
	}
}
//显示通信方式 G L
void topstatus_showcommtype(INT8U online_type)
{
	Point pos;
	char str[3];
	Rect rect;
	memset(str, 0, 3);
	pos.x = FONTSIZE_8*2.5;
	pos.y = 0;
	gui_clrrect(getrect(pos, FONTSIZE_8*2, FONTSIZE_8*2));

	switch(online_type)
	{
		case GPRS_COM://外部协议栈上线
		case 3:  //内部协议栈上线
			if(getZone("GW")==0) {
				if(p_JProgramInfo->dev_info.wirelessType>=1 && p_JProgramInfo->dev_info.wirelessType<=4) {
					str[0] = 0x34;//4G
					str[1] = 0x47;
				}
			}else {
				if (p_JProgramInfo->dev_info.wirelessType==2)
				{
					str[0] = 0x05;//C
					str[1] = 0x06;
				}else if(p_JProgramInfo->dev_info.wirelessType==1)
				{
					str[0] = 0x01;//G
					str[1] = 0x02;
				}
				else if(p_JProgramInfo->dev_info.wirelessType==3 || p_JProgramInfo->dev_info.wirelessType==4)
				{
					str[0] = 0x34;//4G
					str[1] = 0x47;
				}
			}
			break;
		case NET_COM:
			str[0] = 0x07;//L
			str[1] = 0x08;
			break;
	}
	if(online_type==GPRS_COM||online_type==NET_COM || online_type == 3)
	{
		gui_textshow_16(str, pos, LCD_NOREV);
		rect = gui_changerect(getrect(pos, FONTSIZE_8*2, FONTSIZE_8*2), -2);
		rect.left -= 1;
		rect.top -= 1;
		gui_rectangle(rect);
	}
}

//显示时间
void topstatus_showtime()
{
	time_t ts;
	struct tm curtm;
	Point pos;
	char str_time[15];
	memset(str_time, 0, 15);
	ts = time(NULL);
	localtime_r(&ts, &curtm);
	sprintf(str_time, "%02d:%02d", curtm.tm_hour, curtm.tm_min);
	pos.x = LCM_X - FONTSIZE_8*5;
	pos.y = 0;
	gui_textshow_16(str_time, pos, LCD_NOREV);
}
//显示叹号  ErcFlg
void topstatus_showAlarm(int ercflg)
{
	Point pos;
	Rect rect;
	char str[3];
	static char ercshow_flg=0;
	time_t ts;
	static time_t oldts=0;
	gui_setpos(&pos,rect_TopStatus.left+FONTSIZE_8*5, rect_TopStatus.top);
	gui_setrect(&rect, pos.x, pos.y, pos.x+FONTSIZE_8*2+1, pos.y+FONTSIZE_8*2);
	gui_clrrect(rect);
	if(ercflg==0)
		return;
	if (ercflg >= 1) {
		ts = time(NULL);
		if(abs(ts-oldts)>=1){
			oldts = ts;
			ercshow_flg = (~ercshow_flg)&0x01;
		}
		if(ercshow_flg==1){
			memset(str, 0, 3);
			sprintf(str, "%02d", ercflg);
			gui_textshow_16(str, pos, LCD_NOREV);
		}else{
			memset(str, 0, 3);
			str[0] = 0x7b;
			str[1] = 0x7c;
			gui_textshow_16(str, pos, LCD_NOREV);
		}
	}
}
//显示测量点号
void topstatus_showcldno(int cldno)
{
	Point pos;
	char str[5];
	INT8U fontsize=0;
	fontsize = getFontSize();
	memset(&pos, 0, sizeof(Point));
	memset(str, 0, 5);
	sprintf(str, "%04d", cldno);
	//pos.x = FONTSIZE_8*8.75;
	pos.x = FONTSIZE_8*9.75;
	gui_textshow_16(str, pos, LCD_NOREV);

	//pos.x = FONTSIZE_8*7.5;
	pos.x = FONTSIZE_8*9.25;
	gui_vline(pos, FONTSIZE_8*2);
	//pos.x = LCM_X - FONTSIZE_8*6;
	pos.x = LCM_X - FONTSIZE_8*6;
	gui_vline(pos, FONTSIZE_8*2);
}
void topstatus_showEsamStatus()
{
	char str[3] = {0x00,0x00,0x00};
	Point pos;
	gui_setpos(&pos,rect_TopStatus.left+FONTSIZE_8*7.25, rect_TopStatus.top);
	if(p_JProgramInfo->dev_info.Esam_VersionStatus == 0)//如果芯片为测试密钥，显示小房子信息
	{
		str[0] = 0x0f;
		str[1] = 0x10;
		gui_textshow_16(str, pos, LCD_NOREV);
	}
}

//gprs模块信号显示，液晶上面部分显示
void lcd_showTopStatus()
{
	int ercno_curr=0;
	INT16U gprs_csq = 0;
	Point topline_pos;
	gui_clrrect(rect_TopStatus);
	if(p_JProgramInfo != NULL){
		if(p_JProgramInfo->dev_info.pppd_status == 1)
		{
			gprs_csq = p_JProgramInfo->dev_info.Gprs_csq;
		}
		if(p_JProgramInfo->dev_info.Cur_Ercno > 0)
		{
			ercno_curr = p_JProgramInfo->dev_info.Cur_Ercno;
		}
	}
	topstatus_showCSQ(gprs_csq);
	topstatus_showAlarm(ercno_curr);
	 topstatus_showEsamStatus();//显示esam状态信息
	topstatus_showcldno(g_curcldno);
	topstatus_showtime();

	topline_pos.x = rect_TopStatus.left;
	topline_pos.y = rect_TopStatus.bottom;
	gui_hline(topline_pos, LCM_X);
	if(p_JProgramInfo != NULL)
		topstatus_showcommtype(p_JProgramInfo->dev_info.jzq_login);
}

void lcd_showBottomStatus(int zb_status, int gprs_status)
{
	INT8U jzq_login_type = 0;
	time_t curtime=time(NULL);
	char str[50] = {0};
	Point pos = {0};
	gui_clrrect(rect_BottomStatus);
	pos.x = rect_BottomStatus.left;
	pos.y = rect_BottomStatus.top - 1;
	gui_hline(pos, LCM_X);
	pos.x = rect_BottomStatus.left + 3;
	pos.y = rect_BottomStatus.top + 1;
	memset(str, 0, 50);

	if (SPTF3 != p_JProgramInfo->cfg_para.device)
	{
		switch(zb_status)
		{
		case ZB_IDLE:
			sprintf(str, "%s", "载波:空闲中...");
			break;
		case ZB_INIT:
			sprintf(str, "%s", "载波:初始化中...");
			break;
		case ZB_METERREADING:
			sprintf(str, "%s", "载波:终端抄表中...");
			break;
		case ZB_SYNCMETER:
			sprintf(str, "%s", "载波:同步档案中...");
			break;
		case ZB_SEARCHMETER:
			sprintf(str, "%s", "载波:终端搜表中...");
			break;
		}
		if(curtime%6>=3)
			gui_textshow(str, pos, LCD_NOREV);
	}
	memset(str, 0, 50);

	switch(gprs_status)
	{
		case GPRS_MODEM_INIT:
			sprintf(str, "%s", "GPRS:初始化通信模块");
			break;
		case GPRS_CHECKMODEM:
			sprintf(str, "%s", "GPRS:检测AT命令成功");
			break;
		case GPRS_GETVER:
			sprintf(str, "%s", "GPRS:获取模块版本信息");
			break;
		case GPRS_SIMOK:
			sprintf(str, "%s", "GPRS:检测到SIM卡");
			break;
		case GPRS_CREGOK:
			sprintf(str, "%s%d", "GPRS:注册网络,信号强度为",p_JProgramInfo->dev_info.Gprs_csq);
			break;
		case GPRS_DIALING:
			sprintf(str, "%s", "GPRS:拨号中...");
			break;
		case GPRS_CONNECTING:
			sprintf(str, "%s", "GPRS:连接到主站...");
			break;
		case GPRS_ONLINE:
			sprintf(str, "%s", "GPRS:终端在线");
			break;
	}
	if(p_JProgramInfo != NULL) {
		jzq_login_type = p_JProgramInfo->dev_info.jzq_login;
	}
	if(curtime%6<3){
		if(jzq_login_type != NET_COM )
			gui_textshow(str, pos, LCD_NOREV);
		else if(jzq_login_type == NET_COM)
			gui_textshow((char*)"以太网:终端在线", pos, LCD_NOREV);
	}
}
