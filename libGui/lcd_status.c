/*
 * lcd_status.c 液晶上下状态条
 *
 *  Created on: 2013-5-14
 *      Author: Administrator
 */
//#include "../include/stdafx.h"
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
#ifdef JIANGSU
	static INT8U csq_state=0;
//	fprintf(stderr,"jzq_login = %d \n",shmm_getdevstat()->jzq_login);
	if(shmm_getdevstat()->jzq_login == GPRS_COM)
	{
		str[0]=0x1c;
		gui_textshow_16(str, wire_pos, LCD_NOREV);
		csq_state=0;
	}
	else if(shmm_getdevstat()->jzq_login == 0)
	{
		csq_state++;
		if(csq_state%2==0)
		{
			str[0]=0x1c;
			gui_textshow_16(str, wire_pos, LCD_NOREV);
		}
		else
		{
			str[0]=' ';
			gui_textshow_16(str, wire_pos, LCD_NOREV);
		}
	}
	else
	{
		str[0]=' ';
		gui_textshow_16(str, wire_pos, LCD_NOREV);
	}
#else
	str[0] = 0x1c;
	gui_textshow_16(str, wire_pos, LCD_NOREV);
#endif
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
	set_time_show_flag(1);//TODO:new
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
		case GPRS_COM:
			if (p_JProgramInfo->dev_info.wirelessType==2)
			{
				str[0] = 0x05;//C
				str[1] = 0x06;
			}else
			{
				str[0] = 0x01;//G
				str[1] = 0x02;
			}
			break;
		case NET_COM:
			str[0] = 0x07;//L
			str[1] = 0x08;
			break;
	}
#ifdef ZHEJIANG
//	fprintf(stderr,"\n!!!!!!!!!!!!!!update_state:%d",shmm_getdevstat()->update_state);
	if(shmm_getdevstat()->update_state == 1)
	{
		str[0] = 0x05;//C
		str[1] = 0x06;
	}
#endif
	if(online_type==GPRS_COM||online_type==NET_COM)
	{
		gui_textshow_16(str, pos, LCD_NOREV);
		rect = gui_changerect(getrect(pos, FONTSIZE_8*2, FONTSIZE_8*2), -2);
		rect.left -= 1;
		rect.top -= 1;
		gui_rectangle(rect);
		set_time_show_flag(1);//TODO:new
	}
#if (defined(JIBEI)||defined(MENGDONG))
	else if(online_type == 0)
	{
		str[0] = 0x01;//G
		str[1] = 0x02;
		rect = gui_changerect(getrect(pos, FONTSIZE_8*2, FONTSIZE_8*2), -2);
		if(0 == flag)
		{
			gui_textshow_16(str, pos, LCD_NOREV);//显示大G
			rect.left -= 1;
			rect.top -= 1;
			gui_rectangle(rect);
			flag = 1;
		}
		else
		{
			rect.bottom += 1;
			rect.right += 1;
			gui_clrrect(rect);
			flag = 0;
		}
		//usleep(600*1000);
	}
#endif
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
#if (defined(JIBEI)||defined(JIANGSU))
		sprintf(str_time, "%02d:%02d:%02d", curtm.tm_hour, curtm.tm_min ,curtm.tm_sec);
		pos.x = LCM_X - FONTSIZE_8*5 -15;
		pos.y = 3;
		gui_textshow(str_time, pos, LCD_NOREV);
#else
		sprintf(str_time, "%02d:%02d", curtm.tm_hour, curtm.tm_min);
		pos.x = LCM_X - FONTSIZE_8*5;
		pos.y = 0;
		gui_textshow_16(str_time, pos, LCD_NOREV);
#endif
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
		set_time_show_flag(1);//TODO:new
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
	pos.x = FONTSIZE_8*8.75;
#if (defined(JIBEI)||defined(JIANGSU))
	pos.y = 3;
	setFontSize(12);
	gui_textshow(str, pos, LCD_NOREV);
	setFontSize(fontsize);
#else
	gui_textshow_16(str, pos, LCD_NOREV);
#endif

#ifndef JIANGSU //江苏测量点画边框
	pos.x = FONTSIZE_8*7.5;
	gui_vline(pos, FONTSIZE_8*2);
#if (defined(JIBEI))
	pos.x = LCM_X - FONTSIZE_8*6 -15;
#else
	pos.x = LCM_X - FONTSIZE_8*6;
#endif
	gui_vline(pos, FONTSIZE_8*2);
#else
	Rect rect;
	rect.bottom = FONTSIZE_8*1.75;
	rect.top = 3;
	rect.left = FONTSIZE_8*8;
	rect.right = LCM_X - FONTSIZE_8*6 -15;
	gui_rectangle(rect);
#endif
	set_time_show_flag(1);
}

//-------------------------------------------------------------------------------------
//接口 GprsCSQ  gprs_ok ErcFlg CLDNo
#ifndef FB_SIM
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

	topstatus_showcldno(g_curcldno);
	topstatus_showtime();

	topline_pos.x = rect_TopStatus.left;
	topline_pos.y = rect_TopStatus.bottom;
	gui_hline(topline_pos, LCM_X);
	set_time_show_flag(1);//TODO:new
#ifdef JIANGSU
	if(shmm_getdevstat()->jzq_login == GPRS_COM)
	{
		char str[3];
		memset(str,0,sizeof(str));
		Point pos;
//		shmm_getdevstat()->GPRS_MODEM_MYTYPE=1;
//		shmm_getdevstat()->GPRS_TXmodeType=3;
		pos.x = FONTSIZE_8*3;
		pos.y = 0;
		if(shmm_getdevstat()->Rev_flag[9]==1)//GPRS
			str[0]=0x1e;
		else if(shmm_getdevstat()->Rev_flag[9] == 0)//其他为空，当前江苏无CDMA，gprs进程中没置位，此处空
			str[0]=0x20;
		if(shmm_getdevstat()->Rev_flag[10]==1)//此处需确定图标一一对应关系
		{
			str[1]=0x7e;
			gui_textshow_16(str, pos, LCD_NOREV);
		}
		else if(shmm_getdevstat()->Rev_flag[10]==2)
		{
			str[1]=0x7f;
			gui_textshow_16(str, pos, LCD_NOREV);
		}
		else
		{
			str[0]=0x20;
			gui_textshow_16(str, pos, LCD_NOREV);
		}
	}
	else if(shmm_getdevstat()->jzq_login==NET_COM || shmm_getdevstat()->jzq_login==SER_COM)
	{
		topstatus_showcommtype(NET_COM);
	}
#else
	if(p_JProgramInfo != NULL)
		topstatus_showcommtype(p_JProgramInfo->dev_info.jzq_login);
#endif

}
#else
void lcd_showTopStatus()
{
	Point topline_pos;
	topstatus_showCSQ(16);
	topstatus_showcommtype(2);
	topstatus_showAlarm(31);
	topstatus_showcldno(1234);
	topstatus_showtime();
	topline_pos.x = rect_TopStatus.left;
	topline_pos.y = rect_TopStatus.bottom;
	gui_hline(topline_pos, LCM_X);
}
#endif

void lcd_showBottomStatus(int zb_status, int gprs_status)
{
//#ifdef CCTT_I
	INT8U jzq_login_type = 0;
	time_t curtime=time(NULL);
	char str[50];
	Point pos;
	gui_clrrect(rect_BottomStatus);
	pos.x = rect_BottomStatus.left;
	pos.y = rect_BottomStatus.top - 1;
	gui_hline(pos, LCM_X);
	pos.x = rect_BottomStatus.left + 3;
	pos.y = rect_BottomStatus.top + 1;
	memset(str, 0, 50);
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
	if(curtime%2==0)
		gui_textshow(str, pos, LCD_NOREV);
	set_time_show_flag(1);
	memset(str, 0, 50);
#ifdef JIANGXI_CDMA
	switch(gprs_status)
		{
		case GPRS_MODEM_INIT:
			sprintf(str, "%s", "CDMA:初始化通信模块");
			break;
		case GPRS_CHECKMODEM:
			sprintf(str, "%s", "CDMA:检测AT命令成功");
			break;
		case GPRS_GETVER:
			sprintf(str, "%s", "CDMA:获取模块版本信息");
			break;
		case GPRS_SIMOK:
			sprintf(str, "%s", "CDMA:检测到SIM卡");
			break;
		case GPRS_CREGOK:
			sprintf(str, "%s%d", "CDMA:注册网络,信号强度为",shmm_getdevstat()->Gprs_csq);
			break;
		case GPRS_DIALING:
			sprintf(str, "%s", "CDMA:拨号中...");
			break;
		case GPRS_CONNECTING:
			sprintf(str, "%s", "CDMA:连接到主站...");
			break;
		case GPRS_ONLINE:
			sprintf(str, "%s", "CDMA:终端在线");
			break;
		}
#else
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
		sprintf(str, "%s%d", "GPRS:注册网络,信号强度为",20);//shmm_getdevstat()->Gprs_csq);
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
#endif
//	sleep(1);
	if(p_JProgramInfo != NULL)
	{
		jzq_login_type = p_JProgramInfo->dev_info.jzq_login;
	}
	if(curtime%2==1){
		if(jzq_login_type != NET_COM )
			gui_textshow(str, pos, LCD_NOREV);
		else if(jzq_login_type == NET_COM)
			gui_textshow((char*)"以太网:终端在线", pos, LCD_NOREV);
	}
	delay(1000);
//#endif
//#if defined(SPTF_III) || defined(CCTT_I)
#ifdef SPTF_III
	time_t curtime=time(NULL);
	char str[50];
	Point pos;
	pos.x = rect_BottomStatus.left;
	pos.y = rect_BottomStatus.top - 1;
	gui_hline(pos, LCM_X);
	pos.x = rect_BottomStatus.left + 3;
	pos.y = rect_BottomStatus.top + 1;
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
		sprintf(str, "%s%d", "GPRS:注册网络,信号强度为",shmm_getdevstat()->Gprs_csq);
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
	if(curtime%2==1){
		gui_clrrect(rect_BottomStatus);
		if(shmm_getdevstat()->jzq_login!=NET_COM)
			gui_textshow(str, pos, LCD_NOREV);
		else
			gui_textshow((char*)"以太网:终端在线", pos, LCD_NOREV);
	}
	delay(1000);
#endif
	set_time_show_flag(1);
}
