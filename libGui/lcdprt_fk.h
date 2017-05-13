/*
 * lcdprt_fk.h
 *
 *  Created on: 2013-7-11
 *      Author: yd
 */

#ifndef LCDPRT_JZQ_H_
#define LCDPRT_JZQ_H_
#include "lcdprt.h"
//#include "gdm.h"
#define FKIII_METERNUM 8  //负控表所带的电表总数
#ifdef SPTF_III
void show_kvki();
void show_kp();
void menu_control();
void menu_realdata();
void menu_zhongwen();
void menu_goudian();
void menu_teminfo();
void menu_realP();
void menu_realE();
void menu_loadcurve();
void menu_switchstate();
void menu_gongctlrec();
void menu_dianctlrec();
void menu_yaoctlrec();
void menu_shidianrec();
void menu_realjc();
void menu_shiduanpara();
void menu_changxiupara();
void menu_baotingpara();
void menu_xiafupara();
void menu_yuedianpara();
void menu_kvkikp();
void menu_fksetmeter();
void menu_485func_change();
void addmeter();
void deletemeter(int pindex);
void menu_fksetid_10();
void menu_fksetid_16();
void menu_fkmasterip();
void menu_fkmasterip1();
void menu_fkaddmeter();
void menu_fkdelmeter();
//TODO:事件操作函数
//void gongctlrec(ERC *erc, int erc_index,int count);
//void yaoctlrec(ERC *erc, int erc_index,int count);
//void shidianrec(ERC *erc, int erc_index,int count);
//void dianctlrec(ERC *erc, int erc_index,int count);
void show_load_page(INT8U *filename, int zj_index, int point_begin, int point_num);
char* getctrlround(char round, char *str);
//int get_erc(int ercno, ERC *erc, int erc_num);

extern void menu_masterapn();
extern void menu_termip();
extern void menu_setpasswd();
extern void menu_lcdcontrast();
extern void showallmeter(void (*pfun)(int cldno));
extern void show_realdatabycld(int cldno);
extern void show_realdata(int cldno, LcdDataItem *item, int itemcount);
extern void menu_yxstatus();
extern void jzq_addr_edit();
extern void menu_jzqzddr10();
extern void menu_jzqzddr16();
extern void menu_settx();
extern void menu_jzqtelephone();
extern void menu_masterapn();
extern void menu_eth0para();
extern void menu_jzqtime();
extern void menu_lcdcontrast();
extern void menu_jzqstatus();
extern void menu_jzqreboot();
extern void menu_initjzqdata();
extern void menu_initjzqpara();
extern void menu_gprsip();
extern void menu_rtcpower();
extern void menu_zb_info();
extern void menu_gprs_info();
extern void menu_ac_info();
extern void setmeterpara(int pindex);
extern int getMemInfo();
extern int getNandInfo();
extern int getNorInfo();
extern int getCpuInfo();
extern int requestdata_485_ZB(int cldno, INT8U *mq_name, int arr_did[], int arr_size, LcdDataItem *item);
extern int requestdata_485_ZB_Block(int cldno, INT8U *mq_name, int msg_num,int arr_did, LcdDataItem *item);
extern int requestdata_485_ZB_Single(int cldno, INT8U *mq_name, int arr_did[], int arr_size, LcdDataItem *item);
extern int finddataitem(LcdDataItem *item, int size, int did);
#ifdef SHANGHAI
extern void showselectmeter(char *cldaddr,char *box_addr);
#endif
#ifdef JIANGSU
extern void show_realdatabycld_js(int pindex);
extern void setmeterpara_js(int pindex);
extern void menu_eth0para_js();
#endif
#ifdef HUBEI
void menu_realdata_DY();
void menu_realdata_DL();
void menu_realdata_X1();
void menu_realdata_X2();
void menu_realdata_X3();
void menu_realdata_X4();
void menu_realdata_P1();
void menu_realdata_P2();
void menu_realdata_OT();
void menu_realdata_ZJ();
void LunXunShowPage7(LcdDataItem *item, int size, INT8U show_flg);
void LunXunShowPage8(LcdDataItem *item, int size, INT8U show_flg);
void LunXunShowPage_X1_Q_All(LcdDataItem *item, int size, INT8U show_flg);
void LunXunShowPage_X2_Q_All(LcdDataItem *item, int size, INT8U show_flg);
void LunXunShowPage_X3_Q_All(LcdDataItem *item, int size, INT8U show_flg);
void LunXunShowPage_X4_Q_All(LcdDataItem *item, int size, INT8U show_flg);
void LunXunShowPage9(LcdDataItem *item, int size, INT8U show_flg);
void LunXunShowPage10(LcdDataItem *item, int size, INT8U show_flg);
extern void menu_Virpara();
extern void menu_login_stat();
extern void menu_manualsearch();
extern void menu_readmeterbycldno();
extern void menu_readmeterbycldaddr();
void menu_initjzqparaf3();
#endif
#endif
#endif /* LCDPRT_JZQ_H_ */
