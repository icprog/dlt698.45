/*
 * lcdprt_jzq.h
 *
 *  Created on: 2013-7-11
 *      Author: yd
 */
#ifndef LCDPRT_JZQ_H_
#define LCDPRT_JZQ_H_
#include "lcdprt.h"
#include "../include/Objectdef.h"
//配置序号、表地址、端口、规约、通信速率、费率个数、接线方式、采集器地址
typedef struct{
	int iidnex;
	int mpno;
	char cldaddr[12];
}Gui_MP_t;

extern int g_curcldno;
extern INT8U g_JZQ_TimeSetUp_flg;
void addmeter();
void menu_showclddata();
void menu_showdaydata();
void menu_showmonthdata();
void menu_setmeterpara();
void menu_jzqtime();
void menu_setpasswd();
void menu_jzqaddr();
void menu_readmeterbycldno();
void menu_readmeterbycldaddr();
void menu_jzqstatus();
void menu_manualsearch();
void menu_termip();
void menu_gprsip();
void menu_lcdcontrast();
void menu_rtcpower();
void menu_impsoe();
void menu_norsoe();
void menu_zb_begin();
void menu_zb_stop();
void menu_zb_resume();
void menu_vifr_set();
void menu_rs232_set();
void menu_zb_info();
void menu_gprs_info();
void menu_ac_info();
void menu_485func_change();
void menu_readmeter_info();
void menu_heartcycle();
void menu_yxstatus();
void menu_yxstatus_js();
void menu_autosearch();
void menu_jzqreboot();
void menu_debug();
void menu_initjzqdata();
void menu_initjzqevent();
void menu_initjzqdemand();
//void menu_initjzqpara();
//void menu_initjzqpara();
void menu_pagesetup();
void menu_masterapn();
void menu_settx();
void menu_set_nettx();
void menu_set_wlantx();
void menu_netmaster();
void menu_wlanmaster();
void menu_eth0para();
void menu_jzqtelephone();
void jzq_id_edit();
void jzq_addr_edit();
//void menu_jzqzddr10();
//void menu_jzqzddr16();
void menu_jzqsetmeter();
void menu_jzqaddmeter();
void menu_jzqdelmeter();
void deletemeter(int pindex);
void gui_mp_free(CLASS_6001 *gui_mp);
void showallmeter(void (*pfun)(int cldno));
void menu_fail_meterinfo(INT16U *fail_mpindex,INT16U fail_num);
int gui_mp_select(Gui_MP_t **ppgui_mp,char *cldaddr);
int requestdata_ACS(int cmd, LcdDataItem *item);
int gui_mp_compose(CLASS_6001 **ppgui_mp);
extern int getMemInfo();
extern int getNandInfo();
extern int getNorInfo();
extern int getCpuInfo();
extern void menu_jzqtime_js();
extern void modify_lcd_contrast();
extern void colseMsg(mqd_t mqd);
extern INT8U getmpport(INT16U index);
extern int msgbox_jzqtimeset(char *s_jzqtime, int len);
extern int msgbox_inputjzqtime(char *s_jzqtime, int len);
extern void show_jcdata(INT8U itemcount,LcdDataItem *item);
extern void ShowCLDDataPage(LcdDataItem *item, int size, INT8U show_flg);
extern void ShowJCData(ProgramInfo* p_JProgramInfo,int PageNo, LcdDataItem *item, int size, int show_flg);
extern int sendMsg(mqd_t mqd, INT32U cmd, INT8S *sendbuf, INT32U bufsiz);
extern void menu_Virpara();
#endif /* LCDPRT_JZQ_H_ */
