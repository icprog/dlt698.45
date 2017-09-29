//lcd_prt.c 菜单功能实现
//#include "../include/stdafx.h"
#include "comm.h"
#include "lcd_menu.h"
#include "lcd_ctrl.h"
#include "lcdprt_fk.h"
#include "lcdprt_jzq.h"
#include "show_ctrl.h"
#define OFFSET_Y 3
#define TIMEOUT_SHORT 2
#pragma message("\n\n************************************\n SPTF_III__Compiling............\n************************************\n")
extern void jzq_id_edit();
extern void menu_set_nettx();
extern void menu_set_wlantx();
extern void menu_FactoryReset();
extern void menu_showclddata();
extern void menu_jzqsetmeter();
extern void menu_jzqaddmeter();
extern void menu_jzqdelmeter();
Menu menu_fk[]={
	//level,    name,   		fun, 				ispasswd			pthis,
{{level0,"  主菜单 ",		NULL, 				MENU_NOPASSWD},		NULL},
	{{level1,"1.实时数据", 	NULL, 				MENU_NOPASSWD},		NULL},
					{{level2,"1.当前功率",	menu_realP, 		MENU_NOPASSWD},		NULL},
					{{level2,"2.当前电量", 	menu_realE, 		MENU_NOPASSWD},		NULL},
					{{level2,"3.负荷曲线", 	menu_loadcurve, 	MENU_NOPASSWD},		NULL},
					{{level2,"4.开关状态", 	menu_yxstatus_fk, 	MENU_NOPASSWD},		NULL},
					{{level2,"5.功控记录", 	menu_gongctlrec, 	MENU_NOPASSWD},		NULL},
					{{level2,"6.电控记录", 	menu_dianctlrec, 	MENU_NOPASSWD},		NULL},
					{{level2,"7.遥控记录", 	menu_yaoctlrec, 	MENU_NOPASSWD},		NULL},
					{{level2,"8.失电记录", 	menu_shidianrec, 	MENU_NOPASSWD},		NULL},
					{{level2,"9.交流采样信息",menu_realjc, 		MENU_NOPASSWD},		NULL},
	{{level1,"2.参数定值", 	NULL, 				MENU_NOPASSWD},		NULL},
					{{level2,"1.时段控参数", 		menu_shiduanpara, 	MENU_NOPASSWD},		NULL},
					{{level2,"2.厂休控参数", 		menu_changxiupara, 	MENU_NOPASSWD},		NULL},
					{{level2,"3.报停控参数",		menu_baotingpara, 	MENU_NOPASSWD},		NULL},
					{{level2,"4.下浮控参数",		menu_xiafupara, 	MENU_NOPASSWD},		NULL},
					{{level2,"5.月电控参数", 		menu_yuedianpara, 	MENU_NOPASSWD},		NULL},
					{{level2,"6.KvKiKp", 		menu_kvkikp, 		MENU_NOPASSWD},		NULL},
					{{level2,"7.电能表参数", 		NULL, 				MENU_NOPASSWD},		NULL},
									{{level3,"1.修改测量点", 		menu_jzqsetmeter,	MENU_ISPASSWD},	NULL},
									{{level3,"2.添加测量点", 		menu_jzqaddmeter,	MENU_ISPASSWD},	NULL},
									{{level3,"3.删除测量点", 		menu_jzqdelmeter,	MENU_ISPASSWD},	NULL},
					{{level2,"8.配置参数", 		NULL, 				MENU_NOPASSWD},		NULL},
									{{level3,"1.终端编号", 	jzq_id_edit, 					MENU_ISPASSWD_EDITMODE},		NULL},
									{{level3,"2.通信通道", 	NULL, 	MENU_ISPASSWD_EDITMODE},		NULL},
													{{level4,"1.通信方式", 	NULL, 				MENU_NOPASSWD},		NULL},
																{{level5,"1.以太网通信方式",   menu_set_nettx,        MENU_NOPASSWD},     NULL},
																{{level5,"2.无线通信方式",     menu_set_wlantx,       MENU_NOPASSWD },    NULL},
									{{level3,"3.终端复位", 	NULL, 						MENU_ISPASSWD},		NULL},
													{{level4,"1.终端重启", 	menu_jzqreboot, 	MENU_ISPASSWD},		NULL},
													{{level4,"2.数据初始化", 	menu_initjzqdata, 	MENU_ISPASSWD},		NULL},
													{{level4,"3.参数初始化", 	menu_FactoryReset, 	MENU_ISPASSWD},		NULL},
									{{level3,"4.终端时间设置", menu_jzqtime, 				MENU_NOPASSWD},		NULL},
									{{level3,"5.界面密码设置", menu_setpasswd, 			MENU_NOPASSWD},		NULL},
									{{level3,"6.现场调试", 	NULL, 				MENU_NOPASSWD},		NULL},
													{{level4,"1.本地IP设置",	menu_termip, 		MENU_NOPASSWD},		NULL},
													{{level4,"2.GPRSIP查看",	menu_gprsip, 		MENU_NOPASSWD},		NULL},
													{{level4,"3.液晶对比度", 	menu_lcdcontrast, 	MENU_NOPASSWD},		NULL},
													{{level4,"4.时钟电池", 	menu_rtcpower, 		MENU_NOPASSWD},		NULL},
													{{level4,"5.GPRS模块信息",menu_gprs_info,		MENU_NOPASSWD},		NULL},
													{{level4,"6.交采芯片信息",menu_ac_info,		MENU_NOPASSWD},		NULL},
	{{level1,"3.控制状态", 	menu_control, 				MENU_NOPASSWD},		NULL},
	{{level1,"4.电能表示数", 	menu_showclddata, 			MENU_NOPASSWD},		NULL},
	{{level1,"5.中文信息", 	menu_zhongwen, 				MENU_NOPASSWD},		NULL},
	{{level1,"6.购电信息", 	menu_goudian, 				MENU_NOPASSWD},		NULL},
	{{level1,"7.终端信息", 	menu_jzqstatus, 			MENU_NOPASSWD},		NULL},
};//测量点数据显示


int getMenuSize_fk(){
	return sizeof(menu_fk)/sizeof(Menu);
}

char* getctrlround(char round, char *str){
	int i;
	char str_tmp[100];
	for(i=0;i<8;i++){
		if(round & (1<<i)){
			memset(str_tmp, 0, 100);
			sprintf(str_tmp, " %d", i+1);
			strcat(str,str_tmp);
		}
	}
	return str;
}

void menu_control_showstate(char *ctlname, INT8U state, Point pos){
	char str[100];
	memset(str, 0, 100);
	sprintf(str, "%s:%s", ctlname, state?"投入":"解除");
	gui_textshow(str, pos, LCD_NOREV);
}

void menu_control(){
	Rect rect;
	CLASS23			class23[8];			//总加组
	CLASS_8103 c8103; 					//时段功控
	CLASS_8104 c8104; 					//厂休控
	CLASS_8105 c8105; 					//营业报停控
	CLASS_8106 c8106; 					//功率下浮控
	CLASS_8107 c8107; 					//购电控
	CLASS_8108 c8108; 					//月电控

	int i = 0;
	for (i = 0; i < 8; ++i) {
		readCoverClass(0x2301 + i, 0, &class23[0],sizeof(CLASS23), para_vari_save);
	}

	readCoverClass(0x8103, 0, (void *) &c8103, sizeof(CLASS_8103),para_vari_save);
	readCoverClass(0x8104, 0, (void *) &c8104, sizeof(CLASS_8104),para_vari_save);

	readCoverClass(0x8105, 0, (void *) &c8105, sizeof(CLASS_8105),para_vari_save);
	readCoverClass(0x8106, 0, (void *) &c8106, sizeof(CLASS_8106),para_vari_save);
	readCoverClass(0x8107, 0, (void *) &c8107, sizeof(CLASS_8107),para_vari_save);
	readCoverClass(0x8108, 0, (void *) &c8108, sizeof(CLASS_8108),para_vari_save);

	char str[100], first_flg=0;
	Point pos;
	int zj_index=1;
	memset(str, 0, 100);
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		if(PressKey==ESC)
			break;
		switch(PressKey)
		{
		case LEFT:
		case UP:
			zj_index--;
			if(zj_index<=0)
				zj_index = MAXNUM_SUMGROUP;
			break;
		case RIGHT:
		case DOWN:
			zj_index++;
			if(zj_index>MAXNUM_SUMGROUP)
				zj_index = 1;
			break;
		case ESC:
			return;
		}

		gui_clrrect(rect_Client);
		gui_setpos(&pos, rect_Client.left+6*FONTSIZE, rect_Client.top+3*FONTSIZE);
		memset(str, 0, 100);

		sprintf(str, "总加组%d ", zj_index);
		gui_textshow(str, pos, LCD_NOREV);
		gui_setpos(&pos, rect_Client.left+6*FONTSIZE, rect_Client.top+6*FONTSIZE);
		memset(str, 0, 100);
		sprintf(str, "[OI:%04x]", 0x2300 + zj_index);
		gui_textshow(str, pos, LCD_NOREV);
		memcpy(&rect, &rect_Client, sizeof(Rect));
		rect = gui_getstrrect((unsigned char*)str, pos);//获得字符串区域
		gui_reverserect(gui_changerect(rect, 2));//反显按钮

		gui_setpos(&pos, rect_Client.left+1*FONTSIZE, rect_Client.top+10*FONTSIZE);//4
		menu_control_showstate((char*)"下浮：",c8106.enable.state & 0x01, pos);			//当前功率下浮控	没有控制方案集数组，默认参数下发在原来结构体数组 0
		pos.y += FONTSIZE*3;
		menu_control_showstate((char*)"报停：",c8105.enable[zj_index-1].state & 0x01, pos);//营业报停控
		pos.y += FONTSIZE*3;
		menu_control_showstate((char*)"厂休：",c8104.enable[zj_index-1].state & 0x01, pos);//营业报停控
		pos.y += FONTSIZE*3;

		gui_setpos(&pos, rect_Client.left+15*FONTSIZE, rect_Client.top+10*FONTSIZE);//4
		menu_control_showstate((char*)"时段：",c8103.enable[zj_index-1].state & 0x01, pos);	//时段功控
		pos.y += FONTSIZE*3;
		menu_control_showstate((char*)"购电：",c8107.enable[zj_index-1].state & 0x01 , pos);//购电控
		pos.y += FONTSIZE*3;
		menu_control_showstate((char*)"月电：",c8108.enable[zj_index-1].state & 0x01 , pos);//月电控
		pos.y += FONTSIZE*3;

		if(PressKey!=NOKEY || first_flg==0){
			first_flg = 1;
		}
		PressKey = NOKEY;
		delay(300);
	}
}

void fk_realE_showZJ(LcdDataItem *item,int itemNum,INT8U cldno,INT8U *surfix)
{
	int offset_y = 3;
	Point pos;
	TS curts;
	INT8U str[100];
	memset(str, 0, 100);
	FP64 dval=0;
	gui_clrrect(rect_Client);//清除客户显示区
	gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+3);
	sprintf((char *)str, "测量点%d", cldno);
	gui_textshow((char *)str, pos, LCD_NOREV);
	memset(str, 0, 100);
	gui_setpos(&pos, rect_Client.left, rect_Client.top+3*FONTSIZE-2);
	gui_textshow((char*)"当前正向有功电量示数", pos, LCD_NOREV);
	if(get_itemdata1(item,5, 117, &dval, 2)==1){
		sprintf((char*)str,"总% 12.2f %s",dval, surfix);
	}else
		sprintf((char*)str,"总     xxxx.xx %s",surfix);
	gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
	if(get_itemdata1(item,5, 118, &dval, 2)==1){
		sprintf((char*)str,"尖% 12.2f %s", dval, surfix);
	}else
		sprintf((char*)str,"尖     xxxx.xx %s", surfix);
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
	if(get_itemdata1(item,5, 119, &dval, 2)==1){
		sprintf((char*)str,"峰% 12.2f %s",dval, surfix);
	}else
		sprintf((char*)str,"峰     xxxx.xx %s", surfix);
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
	if(get_itemdata1(item,5, 120, &dval, 2)==1){
		sprintf((char*)str,"平% 12.2f %s",dval, surfix);
	}else
		sprintf((char*)str,"平     xxxx.xx %s", surfix);
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
	if(get_itemdata1(item,5, 121, &dval, 2)==1){
		sprintf((char*)str,"谷% 12.2f %s",dval, surfix);
	}else
		sprintf((char*)str,"谷     xxxx.xx %s", surfix);
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	TSGet(&curts);
	memset(str, 0, 100);
	if(curts.Year==0|| curts.Month==0||curts.Day==0)
		sprintf((char*)str,"抄表时间 00/00/00 00:00");
	else
	sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
			curts.Year-2000, curts.Month, curts.Day, curts.Hour, curts.Minute);
	gui_setpos(&pos, rect_Client.left+FONTSIZE, rect_Client.top+18*FONTSIZE);
	gui_textshow((char*)str, pos, LCD_NOREV);
	return;
}

void fk_realE_showZJ_JC(INT8U cldno,INT8U *surfix)
{
	int offset_y = 3;
	Point pos;
	TS curts;
	INT8U str[100];
	memset(str, 0, 100);
	if(cldno != 1)
	{
		return;
	}
	gui_clrrect(rect_Client);//清除客户显示区
	gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+3);
	sprintf((char *)str, "测量点%d", cldno);
	gui_textshow((char *)str, pos, LCD_NOREV);
	memset(str, 0, 100);
	gui_setpos(&pos, rect_Client.left, rect_Client.top+3*FONTSIZE-2);
	gui_textshow((char*)"当前正向有功电量示数", pos, LCD_NOREV);
//	sprintf((char*)str,"总% 10.2f %s",shmm_getpubdata()->ac_energy.PosPt_All*1.0/(ENERGY_CONST*ENERGY_COEF), surfix);			//FOR698
	gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
//	sprintf((char*)str,"尖% 10.2f %s",shmm_getpubdata()->ac_energy.PosPt_Rate[0]*1.0/(ENERGY_CONST*ENERGY_COEF), surfix);
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
//	sprintf((char*)str,"峰% 10.2f %s",shmm_getpubdata()->ac_energy.PosPt_Rate[1]*1.0/(ENERGY_CONST*ENERGY_COEF), surfix);
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
//	sprintf((char*)str,"平% 10.2f %s",shmm_getpubdata()->ac_energy.PosPt_Rate[2]*1.0/(ENERGY_CONST*ENERGY_COEF), surfix);
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
//	sprintf((char*)str,"谷% 10.2f %s",shmm_getpubdata()->ac_energy.PosPt_Rate[3]*1.0/(ENERGY_CONST*ENERGY_COEF), surfix);
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	TSGet(&curts);
	memset(str, 0, 100);
	if(curts.Year==0|| curts.Month==0||curts.Day==0)
		sprintf((char*)str,"抄表时间 00/00/00 00:00");
	else
	sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
			curts.Year-2000, curts.Month, curts.Day, curts.Hour, curts.Minute);
	gui_setpos(&pos, rect_Client.left+FONTSIZE, rect_Client.top+18*FONTSIZE);
	gui_textshow((char*)str, pos, LCD_NOREV);
	return;
}


void fk_realQ_showZJ(LcdDataItem *item,int itemNum,INT8U cldno,INT8U *surfix)
{
	int offset_y = 3;
	Point pos;
	TS ts;
	INT8U str[100];
	memset(str, 0, 100);
	FP64 dval=0;
	int itemPos = -1;
	gui_clrrect(rect_Client);//清除客户显示区
	gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+3);
	sprintf((char *)str, "测量点%d", cldno);
	gui_textshow((char *)str, pos, LCD_NOREV);
	pos.y = rect_Client.top + 3;
	if(get_itemdata1(item,itemNum, 122, &dval, 2)==1){
		sprintf((char*)str,"正向无功 % 8.2f %s",dval, "kVarh");
	}else
		sprintf((char*)str,"正向无功    xxxx.xx %s","kVarh");
	pos.x = rect_Client.left;
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
	if(get_itemdata1(item,itemNum, 167, &dval, 2)==1){
		sprintf((char*)str,"月最大需量% 7.4f%s", dval, "kVar");
	}else
		sprintf((char*)str,"月最大需量   xx.xxxx %s", "kVar");
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
	itemPos = finddataitem(item,itemNum,172);
	if((itemPos != -1) && (item[itemPos].val[0] != 0xee)){
		ts.Minute = bcd2int((char*)&item[itemPos].val[0]);
		ts.Hour = bcd2int((char*)&item[itemPos].val[1]);
		ts.Day = bcd2int((char*)&item[itemPos].val[2]);
		ts.Month = bcd2int((char*)&item[itemPos].val[3]);
		sprintf((char*)str,"发生时间   %02d-%02d %02d:%02d", ts.Month, ts.Day, ts.Hour, ts.Minute);
	}else
		sprintf((char*)str,"发生时间   00-00 00:00");
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);

	if(get_itemdata1(item,itemNum, 142, &dval, 2)==1){
		sprintf((char*)str,"反向无功 % 8.2f %s",dval, "kVarh");
	}else
		sprintf((char*)str,"反向无功    xxxx.xx %s","kVarh");
	pos.x = rect_Client.left;
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
	if(get_itemdata1(item,itemNum, 187, &dval, 2)==1){
		sprintf((char*)str,"月最大需量% 7.4f%s", dval, "kVar");
	}else
		sprintf((char*)str,"月最大需量   xx.xxxx %s", "kVar");
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
	itemPos = finddataitem(item,itemNum,192);
	if((itemPos != -1) && (item[itemPos].val[0] != 0xee)){
		ts.Minute = bcd2int((char*)&item[itemPos].val[0]);
		ts.Hour = bcd2int((char*)&item[itemPos].val[1]);
		ts.Day = bcd2int((char*)&item[itemPos].val[2]);
		ts.Month = bcd2int((char*)&item[itemPos].val[3]);
		sprintf((char*)str,"发生时间   %02d-%02d %02d:%02d", ts.Month, ts.Day, ts.Hour, ts.Minute);
	}else
		sprintf((char*)str,"发生时间   00-00 00:00");
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);

//	memset(str, 0, 100);
//	sprintf((char*)str,"局编号 %s", ParaAll->f29s.f29[cldno-1].MeterDisplayNo);			//FOR698
//	gui_setpos(&pos, rect_Client.left, rect_Client.top+18*FONTSIZE+2);
//	gui_textshow((char*)str, pos, LCD_NOREV);
	return;
}


void fk_realQ_showZJ_JC(INT8U cldno)
{
	int offset_y = 3;
	Point pos;
	INT8U str[100];
	memset(str, 0, 100);
	if(cldno != 1)
	{
		return;
	}
	gui_clrrect(rect_Client);//清除客户显示区
	gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+3);
	sprintf((char *)str, "测量点%d", cldno);
	gui_textshow((char *)str, pos, LCD_NOREV);
	pos.y = rect_Client.top + 3;

//	sprintf((char*)str,"正向无功总 % 8.2f %s",shmm_getpubdata()->ac_energy.PosQt_All*1.0/(ENERGY_CONST*ENERGY_COEF), "kVarh");	//FOR698
	pos.x = rect_Client.left;
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);

//	sprintf((char*)str,"月最大需量% 7.4f%s", shmm_getpubdata()->jc_data.JC_ZPXL_Curr_Data, "kVar");
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
//	sprintf((char*)str,"发生时间   %02d-%02d %02d:%02d", shmm_getpubdata()->jc_data.JC_FPXL_Curr_Time[0],\
//			shmm_getpubdata()->jc_data.JC_FPXL_Curr_Time[1], shmm_getpubdata()->jc_data.JC_FPXL_Curr_Time[2],\
//			shmm_getpubdata()->jc_data.JC_FPXL_Curr_Time[3]);
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
//	sprintf((char*)str,"反向无功总 % 8.2f %s",shmm_getpubdata()->ac_energy.NegQt_All*1.0/(ENERGY_CONST*ENERGY_COEF), "kVarh");
	pos.x = rect_Client.left;
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
//	sprintf((char*)str,"月最大需量% 7.4f%s", shmm_getpubdata()->jc_data.JC_FQXL_Curr_Data, "kVar");
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
//	sprintf((char*)str,"发生时间   %02d-%02d %02d:%02d", shmm_getpubdata()->jc_data.JC_FQXL_Curr_Time[0], shmm_getpubdata()->jc_data.JC_FQXL_Curr_Time[1],\
//			shmm_getpubdata()->jc_data.JC_FQXL_Curr_Time[2], shmm_getpubdata()->jc_data.JC_FQXL_Curr_Time[3]);
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
//	memset(str, 0, 100);
//	sprintf((char*)str,"局编号 %s", ParaAll->f29s.f29[cldno-1].MeterDisplayNo);
//	gui_setpos(&pos, rect_Client.left, rect_Client.top+18*FONTSIZE+2);
//	gui_textshow((char*)str, pos, LCD_NOREV);
	return;
}

void fk_realV_showDY(LcdDataItem *item,int itemNum,INT8U cldno,INT8U *surfix)//当前电表电压显示
{
	int offset_y = 3;
		Point pos;
		TS curts;
		INT8U str[100];
		memset(str, 0, 100);
		FP64 dval=0;
		gui_clrrect(rect_Client);//清除客户显示区
		gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+3);
		sprintf((char *)str, "测量点%d", cldno);
		gui_textshow((char *)str, pos, LCD_NOREV);
		memset(str, 0, 100);
		if(get_itemdata1(item,3, 36, &dval, 2)==1){
			sprintf((char*)str,"A相% 12.2f %s",dval, surfix);
		}else
			sprintf((char*)str,"A相     xxxx.xx %s",surfix);
		gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		memset(str, 0, 100);
		if(get_itemdata1(item,3, 37, &dval, 2)==1){
			sprintf((char*)str,"B相% 12.2f %s", dval, surfix);
		}else
			sprintf((char*)str,"B相     xxxx.xx %s", surfix);
		pos.y += 2*FONTSIZE+offset_y;
		gui_textshow((char*)str, pos, LCD_NOREV);
		memset(str, 0, 100);
		if(get_itemdata1(item,3, 38, &dval, 2)==1){
			sprintf((char*)str,"C相% 12.2f %s",dval, surfix);
		}else
			sprintf((char*)str,"C相     xxxx.xx %s", surfix);
		pos.y += 2*FONTSIZE+offset_y;
		gui_textshow((char*)str, pos, LCD_NOREV);
		TSGet(&curts);
		memset(str, 0, 100);
		if(curts.Year==0|| curts.Month==0||curts.Day==0)
			sprintf((char*)str,"抄表时间 00/00/00 00:00");
		else
		sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
				curts.Year-2000, curts.Month, curts.Day, curts.Hour, curts.Minute);
		gui_setpos(&pos, rect_Client.left+FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
}

void fk_realC_showDL(LcdDataItem *item,int itemNum,INT8U cldno,INT8U *surfix)//当前电表电压显示
{
	int offset_y = 3;
	Point pos;
	TS curts;
	INT8U str[100];
	memset(str, 0, 100);
	FP64 dval=0;
	gui_clrrect(rect_Client);//清除客户显示区
	gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+3);
	sprintf((char *)str, "测量点%d", cldno);
	gui_textshow((char *)str, pos, LCD_NOREV);
	memset(str, 0, 100);
	if(get_itemdata1(item,3, 39, &dval, 2)==1){
		sprintf((char*)str,"A相% 12.2f %s",dval, surfix);
	}else
		sprintf((char*)str,"A相     xxxx.xx %s",surfix);
	gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
	if(get_itemdata1(item,3, 40, &dval, 2)==1){
		sprintf((char*)str,"B相% 12.2f %s", dval, surfix);
	}else
		sprintf((char*)str,"B相     xxxx.xx %s", surfix);
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
	if(get_itemdata1(item,3, 41, &dval, 2)==1){
		sprintf((char*)str,"C相% 12.2f %s",dval, surfix);
	}else
		sprintf((char*)str,"C相     xxxx.xx %s", surfix);
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	TSGet(&curts);
	memset(str, 0, 100);
	if(curts.Year==0|| curts.Month==0||curts.Day==0)
		sprintf((char*)str,"抄表时间 00/00/00 00:00");
	else
	sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
			curts.Year-2000, curts.Month, curts.Day, curts.Hour, curts.Minute);
	gui_setpos(&pos, rect_Client.left+FONTSIZE, rect_Client.top+18*FONTSIZE);
	gui_textshow((char*)str, pos, LCD_NOREV);
}

void fk_realX_showX(LcdDataItem *item,int itemNum,INT8U cldno,INT8U *surfix, int did)//当前电表电压显示
{
	Point pos;
	INT8U str[100];
	FP64 dval=0;
	gui_clrrect(rect_Client);//清除客户显示区
	gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"  无功电能示值", pos, LCD_NOREV);
	memset(str, 0, 100);
	if(get_itemdata1(item,5, did, &dval, 2)==1){
		sprintf((char*)str,"总% 14.2f kVArh",dval);
	}else
		sprintf((char*)str,"总       xxxx.xx %s",surfix);
	gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+5*FONTSIZE);
	gui_textshow((char*)str, pos, LCD_NOREV);
	if(get_itemdata1(item,5, did+1, &dval, 2)==1){
		sprintf((char*)str,"尖% 14.2f kVArh",dval);
	}else
		sprintf((char*)str,"尖       xxxx.xx %s",surfix);
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	if(get_itemdata1(item,5, did+2, &dval, 2)==1){
		sprintf((char*)str,"峰% 14.2f kVArh",dval);
	}else
		sprintf((char*)str,"峰       xxxx.xx %s",surfix);
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	if(get_itemdata1(item,5, did+3, &dval, 2)==1){
		sprintf((char*)str,"平% 14.2f kVArh",dval);
	}else
		sprintf((char*)str,"平       xxxx.xx %s",surfix);
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	if(get_itemdata1(item,5, did+4, &dval, 2)==1){
		sprintf((char*)str,"谷% 14.2f kVArh",dval);
	}else
		sprintf((char*)str,"谷       xxxx.xx %s",surfix);
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
}

void fk_realP_showP(LcdDataItem *item,int itemNum,INT8U cldno,INT8U *surfix, int did)//当前电表电压显示
{
	Point pos;
	INT8U str[100];
	bzero(str,100);
	FP64 dval=0;
	gui_clrrect(rect_Client);//清除客户显示区
	gui_setpos(&pos, rect_Client.left+7*FONTSIZE, rect_Client.top+FONTSIZE);
	if(did == 24)
		gui_textshow((char *)"当前有功功率", pos, LCD_NOREV);
	if(did == 28)
		gui_textshow((char *)"当前无功功率", pos, LCD_NOREV);
	memset(str, 0, 100);
	if(get_itemdata1(item,4, did, &dval, 2)==1){
		sprintf((char*)str,"总% 15.4f %s",dval,surfix);
	}else
		sprintf((char*)str,"总       xxxx.xx %s",surfix);
	gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+5*FONTSIZE);
	gui_textshow((char*)str, pos, LCD_NOREV);
	bzero(str,100);
	if(get_itemdata1(item,4, did+1, &dval, 2)==1){
		sprintf((char*)str,"A相% 15.4f %s",dval,surfix);
	}else
		sprintf((char*)str,"A相       xxxx.xx %s",surfix);
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
	if(get_itemdata1(item,4, did+2, &dval, 2)==1){
		sprintf((char*)str,"B相% 15.4f %s",dval,surfix);
	}else
		sprintf((char*)str,"B相       xxxx.xx %s",surfix);
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	if(get_itemdata1(item,4, did+3, &dval, 2)==1){
		sprintf((char*)str,"C相% 15.4f %s",dval,surfix);
	}else
		sprintf((char*)str,"C相       xxxx.xx %s",surfix);
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
}

void fk_show_currdat(int cldno){
	int mqcount=0;
	char str[100];
	LcdDataItem item[100];//存储的所有数据项
	int zf_index=1,i, arr_did[6];//
	INT8U first_flg=0;
	memset(item, 0, 100*sizeof(LcdDataItem));
	memset(str,0,100);
	memset(arr_did, 0, 6*sizeof(int));
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){//如果未处于轮显状态
		switch(PressKey)
		{
		case LEFT:
			zf_index--;//显示前一屏内容
			if(zf_index<1)
				zf_index = 2;
			break;
		case RIGHT:
			zf_index++;//显示后一屏内容
			if(zf_index>2)
				zf_index = 1;
			break;
		case ESC:
			return;
		}
		if(PressKey!=NOKEY || first_flg==0)
		{//如果有按键或者
			for(i=0; i<5; i++)
			{//5个数据项一屏
				arr_did[i] = i+3+(zf_index-1)*5;
			}
			first_flg = 1;
			switch(zf_index)
			{
			case 1:
//				if(ParaAll->f10.para_mp[cldno-1].Port == PORT_JC)								//FOR698
//				{
//					fk_realE_showZJ_JC(cldno,(INT8U *)"kWh");
//				}
//				else
//				{
//					if(ParaAll->f10.para_mp[cldno-1].Port == PORT_485I)
//						mqcount = requestdata_485_ZB_Block(ParaAll->f10.para_mp[cldno-1].MPNo,(INT8U*)S485_1_REV_GUI_MQ,5, 3117,item);
//					else if(ParaAll->f10.para_mp[cldno-1].Port == PORT_485II)
//						mqcount = requestdata_485_ZB_Block(ParaAll->f10.para_mp[cldno-1].MPNo,(INT8U*)S485_2_REV_GUI_MQ,5, 3117,item);
//					else
						fk_realE_showZJ_JC(cldno,(INT8U *)"kWh");
//				}
				break;
			case 2:
//				if(ParaAll->f10.para_mp[cldno-1].Port == PORT_JC)								//FOR698
//				{
//					fk_realQ_showZJ_JC(cldno);
//				}
//				else
//				{
//					arr_did[0] = 122;
//					arr_did[1] = 167;
//					arr_did[2] = 172;
//					arr_did[3] = 142;
//					arr_did[4] = 187;
//					arr_did[5] = 192;
//					if(ParaAll->f10.para_mp[cldno-1].Port == PORT_ZB)
//						mqcount = requestdata_485_ZB_Single(ParaAll->f10.para_mp[cldno-1].MPNo, (INT8U*)PLC_REV_GUI_MQ, arr_did,6,item);
//					else if(ParaAll->f10.para_mp[cldno-1].Port == PORT_485I)
//					{
//						fprintf(stderr,"cldno = %d\n",cldno);
//						mqcount = requestdata_485_ZB_Single(ParaAll->f10.para_mp[cldno-1].MPNo,(INT8U*)S485_1_REV_GUI_MQ, arr_did,6,item);
//						fprintf(stderr,"mqcount = %d\n",mqcount);
//					}
//					else if(ParaAll->f10.para_mp[cldno-1].Port == PORT_485II)
//					mqcount = requestdata_485_ZB_Single(ParaAll->f10.para_mp[cldno-1].MPNo,(INT8U*)S485_2_REV_GUI_MQ, arr_did,6,item);
//					fk_realQ_showZJ(item,mqcount,cldno,(INT8U*)"kVArh");
//				}
				break;
			}
		}
		PressKey = NOKEY;
		delay(300);
	}
}

//flag==1,当前正向有功电量示数
//flag==2,三相电压
//flag==3,三相电流
void request_data(int cldno,int msg_num, int arr_did, int flag)
{
	LcdDataItem item[100];
	memset(item,0,100*sizeof(LcdDataItem));
	int mqcount=0;
	int arrid[6];
	INT8U first_flg=0;
	PressKey = NOKEY;
	memset(arrid, 0, 6*sizeof(int));
	while(g_LcdPoll_Flag==LCD_NOTPOLL){//如果未处于轮显状态
		switch(PressKey)
		{
			case ESC:
			return;
		}
		if(first_flg==0)
		{
//			first_flg = 1;
//			if(ParaAll->f10.para_mp[cldno-1].Port == PORT_JC)											//FOR698
//			{
//				gui_clrrect(rect_Client);//清除客户显示区
//				if(flag == 1){
//					fk_realE_showZJ_JC(cldno,(INT8U *)"kWh");
//				}
//				else if(flag == 2){
//					LunXunShowPage7(item,100,2);//用轮显电压显示即可
//				}
//				else if(flag == 3){
//					LunXunShowPage8(item,100,2);//用轮显电流显示即可
//				}
//				else if(flag == 4){
//					LunXunShowPage_X1_Q_All(item,100,2);//用轮显电流显示即可
//				}
//				else if(flag == 5){
//					LunXunShowPage_X2_Q_All(item,100,2);//用轮显电流显示即可
//				}
//				else if(flag == 6){
//					LunXunShowPage_X3_Q_All(item,100,2);//用轮显电流显示即可
//				}
//				else if(flag == 7){
//					LunXunShowPage_X4_Q_All(item,100,2);//用轮显电流显示即可
//				}
//				else if(flag == 8){
//					LunXunShowPage9(item,100,2);//用轮显电流显示即可
//				}
//				else if(flag == 9){
//					LunXunShowPage10(item,100,2);//用轮显电流显示即可
//				}
//				else if(flag == 10){
//					fk_realQ_showZJ_JC(cldno);
//				}
//				else
//					return;
//			}
//			else
//			{
//				if(flag == 10)
//				{
//					arrid[0] = 122;
//					arrid[1] = 167;
//					arrid[2] = 172;
//					arrid[3] = 142;
//					arrid[4] = 187;
//					arrid[5] = 192;
//					if(ParaAll->f10.para_mp[cldno-1].Port == PORT_ZB)
//						mqcount = requestdata_485_ZB_Single(ParaAll->f10.para_mp[cldno-1].MPNo, (INT8U*)PLC_REV_GUI_MQ, arrid,6,item);
//					else if(ParaAll->f10.para_mp[cldno-1].Port == PORT_485I)
//					{
//						fprintf(stderr,"cldno = %d\n",cldno);
//						mqcount = requestdata_485_ZB_Single(ParaAll->f10.para_mp[cldno-1].MPNo,(INT8U*)S485_1_REV_GUI_MQ, arrid,6,item);
//						fprintf(stderr,"mqcount = %d\n",mqcount);
//					}
//					else if(ParaAll->f10.para_mp[cldno-1].Port == PORT_485II)
//					mqcount = requestdata_485_ZB_Single(ParaAll->f10.para_mp[cldno-1].MPNo,(INT8U*)S485_2_REV_GUI_MQ, arrid,6,item);
//					fk_realQ_showZJ(item,mqcount,cldno,(INT8U*)"kVArh");
//					continue;
//				}
//				if(ParaAll->f10.para_mp[cldno-1].Port == PORT_ZB)
//					mqcount = requestdata_485_ZB_Block(ParaAll->f10.para_mp[cldno-1].MPNo, (INT8U*)PLC_REV_GUI_MQ,msg_num, arr_did,item);
//				else if(ParaAll->f10.para_mp[cldno-1].Port == PORT_485I)
//					mqcount = requestdata_485_ZB_Block(ParaAll->f10.para_mp[cldno-1].MPNo,(INT8U*)S485_1_REV_GUI_MQ,msg_num, arr_did,item);
//				else if(ParaAll->f10.para_mp[cldno-1].Port == PORT_485II)
//					mqcount = requestdata_485_ZB_Block(ParaAll->f10.para_mp[cldno-1].MPNo,(INT8U*)S485_2_REV_GUI_MQ,msg_num, arr_did,item);
//				else
//					return;
//				if(flag == 1){
//					fk_realE_showZJ(item,5,cldno,(INT8U*)"kWh");
//				}
//				else if(flag == 2){
//					fk_realV_showDY(item,3,cldno,(INT8U*)"V");
//				}
//				else if(flag == 3){
//					fk_realC_showDL(item,3,cldno,(INT8U*)"A");
//				}
//				else if(flag == 4){
//					fk_realX_showX(item,5,cldno,(INT8U*)"kvarh",127);
//				}
//				else if(flag == 5){
//					fk_realX_showX(item,5,cldno,(INT8U*)"kvarh",147);
//				}
//				else if(flag == 6){
//					fk_realX_showX(item,5,cldno,(INT8U*)"kvarh",152);
//				}
//				else if(flag == 7){
//					fk_realX_showX(item,5,cldno,(INT8U*)"kvarh",132);
//				}
//				else if(flag == 8){
//					fk_realP_showP(item,4,cldno,(INT8U*)"kW",24);
//				}
//				else if(flag == 9){
//					fk_realP_showP(item,4,cldno,(INT8U*)"kVar",28);
//				}
//				else
//					return;
//			}
		}
		PressKey = NOKEY;
		delay(300);
	}
}

void real_showZJ(int cldno)
{
	request_data(cldno,5,3117,1);
}
void realV_showDY(int cldno)
{
	request_data(cldno,3,3036,2);
}
void realC_showDL(int cldno)
{
	request_data(cldno,3,3039,3);
}
void realX_showX1(int cldno)
{
	request_data(cldno,5,3127,4);
}
void realX_showX2(int cldno)
{
	request_data(cldno,5,3147,5);
}
void realX_showX3(int cldno)
{
	request_data(cldno,5,3152,6);
}
void realX_showX4(int cldno)
{
	request_data(cldno,5,3132,7);
}
void realP_showP1(int cldno)
{
	request_data(cldno,4,3024,8);
}
void realP_showP2(int cldno)
{
	request_data(cldno,4,3028,9);
}
void realO_showOT(int cldno)
{
	request_data(cldno,4,3028,10);
}


void fk_show_realdatabycld(int pindex){
	int mqcount=0;
	LcdDataItem item[100];//存储的所有数据项
	if(pindex <=0)
		return;
	memset(item, 0, 100*sizeof(LcdDataItem));
	int arr_did[5]={117, 118, 119, 120, 121};
//	if(ParaAll->f10.para_mp[pindex-1].Port == PORT_ZB)
//		mqcount = requestdata_485_ZB(ParaAll->f10.para_mp[pindex-1].MPNo, (INT8U*)PLC_REV_GUI_MQ, arr_did, 1, item);	//FOR698
//	else if(ParaAll->f10.para_mp[pindex-1].Port == PORT_485I)
//		mqcount = requestdata_485_ZB(ParaAll->f10.para_mp[pindex-1].MPNo,(INT8U*)S485_1_REV_GUI_MQ, arr_did, 1, item);
//	else if(ParaAll->f10.para_mp[pindex-1].Port == PORT_485II)
//		mqcount = requestdata_485_ZB(ParaAll->f10.para_mp[pindex-1].MPNo,(INT8U*)S485_2_REV_GUI_MQ, arr_did, 1, item);
	show_realdata(pindex, item, mqcount);
}


void menu_realdata(){
	showallmeter(fk_show_currdat);
}

void menu_realdata_ZJ(){
	showallmeter(real_showZJ);
}
void menu_realdata_DY(){
	showallmeter(realV_showDY);
}
void menu_realdata_DL(){
	showallmeter(realC_showDL);
}
void menu_realdata_X1(){
	showallmeter(realX_showX1);
}
void menu_realdata_X2(){
	showallmeter(realX_showX2);
}
void menu_realdata_X3(){
	showallmeter(realX_showX3);
}
void menu_realdata_X4(){
	showallmeter(realX_showX4);
}
void menu_realdata_P1(){
	showallmeter(realP_showP1);
}
void menu_realdata_P2(){
	showallmeter(realP_showP2);
}
void menu_realdata_OT(){
	showallmeter(realO_showOT);
}

//换行问题 换行的时候有可能出现乱码
#define CHAENUM_PERLINE 24
void show_msg(char *text, Point text_pos){
	int index_str=0;
	char str[CHAENUM_PERLINE];
	Point pos;
	if(text==NULL)
		return;
	int len = 0;
	int i=0;
	gui_setpos(&pos, text_pos.x, text_pos.y);
	len = strlen(text);
	memset(str, 0, CHAENUM_PERLINE);
	while(i<=len){
		if(text[i]>0x80){
			if((index_str+2)<CHAENUM_PERLINE){
				memcpy(&str[index_str], &text[i], 2);
				index_str += 2;
				i += 2;
			}else{
				gui_textshow(str, pos, LCD_NOREV);
				index_str = 0;
				memset(str, 0, CHAENUM_PERLINE);
				memcpy(&str[index_str], &text[i], 2);
				index_str += 2;
				i += 2;
				pos.y += 2*FONTSIZE + 3;
			}
		}else if(text[i]<=0x80 && text[i]>0){
			if((index_str+1)<CHAENUM_PERLINE){
				memcpy(&str[index_str], &text[i], 1);
				index_str++;
				i++;
			}else{
				gui_textshow(str, pos, LCD_NOREV);
				index_str = 0;
				memset(str, 0, CHAENUM_PERLINE);
				memcpy(&str[index_str], &text[i], 1);
				index_str++;
				i++;
				pos.y += 2*FONTSIZE + 3;
			}
		}else{
			gui_textshow(str, pos, LCD_NOREV);
			break;
		}
	}
}

void menu_zhongwen(){
	char str[200], str_neirong[200];
	Point pos;
	memset(str, 0, 200);
	PressKey = NOKEY;
	gui_clrrect(rect_Client);
	memset(str_neirong, 0, 200);
//	memcpy(str_neirong, shmm_getdevstat()->controls.f32_cfg.data, shmm_getdevstat()->controls.f32_cfg.len);//没有主站的中文信息数据   //FOR698
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		if(PressKey==ESC)
			break;
		gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+2*FONTSIZE);
		memset(str, 0, 200);
		sprintf(str, "中文信息");
		gui_textshow(str, pos, LCD_NOREV);
		pos.x = rect_Client.left + FONTSIZE;
		pos.y += FONTSIZE*3;
		memset(str, 0, 200);
//		if(str[0]!=0)
//			sprintf(str, "种类: %s  编号: %d", shmm_getdevstat()->controls.f32_cfg.type?"重要信息":"普通信息",			//FOR698
//					shmm_getdevstat()->controls.f32_cfg.id);
//		else
			sprintf(str, "种类:           编号:  ");
		gui_textshow(str, pos, LCD_NOREV);
		pos.y += FONTSIZE*3;
		sprintf(str, "信息内容:");
		gui_textshow(str, pos, LCD_NOREV);

		pos.y += FONTSIZE*3;
		memset(str, 0, 200);
		if(str_neirong[0]!=0)
			show_msg(str_neirong, pos);
		else
			show_msg((char*)"无中文信息", pos);
		PressKey = NOKEY;
		delay(300);
	}
}

void menu_goudian(){
	LcdDataItem item[100];//存储的所有数据项
	memset(item, 0, 100*sizeof(LcdDataItem));
//读共享内存获取购电量
	char str[100], first_flg=0;
	Point pos;
	int zj_index=1,unite_index=1;
	CLASS_8107 c8107; //购电控

	readCoverClass(0x8107, 0, (void *) &c8107, sizeof(CLASS_8107),para_vari_save);
	memset(str, 0, 100);
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		switch(PressKey)
		{
		case LEFT:
		case UP:
			unite_index--;
			if(unite_index<=0)
				unite_index = MAXNUM_SUMGROUP;
			break;
		case RIGHT:
		case DOWN:
			unite_index++;
			if(unite_index>MAXNUM_SUMGROUP)
				unite_index = 1;
			break;
		case ESC:
			return;
		}
		int enable_i=0,ifenable=0;
		if(PressKey!=NOKEY || first_flg==0){
			first_flg = 1;

			for(enable_i=0;enable_i<8;enable_i++)
			{
				fprintf(stderr,"\nc8107[%d] index = %04x  ",enable_i,c8107.list[enable_i].index);
				fprintf(stderr,"\nc8107[%d] alarm = %d  ",enable_i,c8107.list[enable_i].alarm);
				fprintf(stderr,"\nc8107[%d] ctrl = %d  ",enable_i,c8107.list[enable_i].ctrl);
				fprintf(stderr,"\nc8107[%d] v = %d  ",enable_i,c8107.list[enable_i].v);
				fprintf(stderr,"\n\n\n");
			}

			for(enable_i=0;enable_i<8;enable_i++)
			{
				if (c8107.enable[enable_i].name == c8107.list[unite_index-1].index)
				{
					ifenable = c8107.enable[enable_i].state;
					break;
				}
			}
			gui_clrrect(rect_Client);
			gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+2*FONTSIZE);
			memset(str, 0, 100);
			sprintf(str,"配置单元%d [总加组%04x]", unite_index, c8107.list[unite_index-1].index);
			gui_textshow(str, pos, LCD_NOREV);
//			if (ifenable==0 )//该总加组没投入！
//			{
//				pos.x = rect_Client.left+5*FONTSIZE;
//				pos.y += 5*FONTSIZE;
//				memset(str, 0, 100);
//				sprintf(str,"未投入");
//				gui_textshow(str, pos, LCD_NOREV);
//				PressKey = NOKEY;
//				delay(300);
//				continue;
//			}
			gui_setpos(&pos, rect_Client.left+FONTSIZE, rect_Client.top+5*FONTSIZE);
			memset(str, 0, 100);
			sprintf(str, "购电单号:%d", c8107.list[unite_index-1].no);			//FOR698
			gui_textshow(str, pos, LCD_NOREV);
			memset(str, 0, 100);
			sprintf(str, "方式: %s", c8107.list[unite_index-1].add_refresh==1?"追加":"刷新");
			pos.y += FONTSIZE*2+3;
			gui_textshow(str, pos, LCD_NOREV);

			memset(str, 0, 100);
			sprintf(str, "购电量值: %d kWh", c8107.list[unite_index-1].v);
			pos.y += FONTSIZE*2+3;
			gui_textshow(str, pos, LCD_NOREV);

			memset(str, 0, 100);
			sprintf(str, "报警门限: %d kWh", c8107.list[unite_index-1].alarm);
			pos.y += FONTSIZE*2+3;
			gui_textshow(str, pos, LCD_NOREV);

			memset(str, 0, 100);
			sprintf(str, "跳闸门限: %d kWh", c8107.list[unite_index-1].ctrl);
			pos.y += FONTSIZE*2+3;
			gui_textshow(str, pos, LCD_NOREV);

			memset(str, 0, 100);
			sprintf(str, "购电控模式: %s", c8107.list[unite_index-1].type==1?"远程":"本地");
			pos.y += FONTSIZE*2+3;
			gui_textshow(str, pos, LCD_NOREV);
		}
		PressKey = NOKEY;
		delay(300);
	}
}

void menu_teminfo_showused(char* name, int used, Point pos){
	char str[50];
	memset(str, 0, 50);
	sprintf(str, "%s:%d%%", name,used);
	gui_textshow(str, pos, LCD_NOREV);
}

void menu_realP(){
	Rect rect;
	char str[100], first_flg=0;
	Point pos;
	int mc_index=1;
	int zj_index=1;
	memset(str, 0, 100);
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		switch(PressKey)
		{
		case LEFT:
		case UP:
			zj_index--;
			if(zj_index<=0)
				zj_index = MAXNUM_SUMGROUP;
			break;
		case RIGHT:
		case DOWN:
			zj_index++;
			if(zj_index>MAXNUM_SUMGROUP)
				zj_index = 1;
			break;
		case ESC:
			return;
		}
		if(PressKey!=NOKEY || first_flg==0){
			first_flg = 1;
			gui_clrrect(rect_Client);
			gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+2*FONTSIZE);
			memset(str, 0, 100);
			sprintf(str, "总加组%d [OI:%04x]", zj_index,0x2300 + zj_index);
			gui_textshow(str, pos, LCD_NOREV);

			memcpy(&rect, &rect_Client, sizeof(Rect));
			rect = gui_getstrrect((unsigned char*)str, pos);//获得字符串区域
			gui_reverserect(gui_changerect(rect, 2));//反显按钮

			memset(str, 0, 100);
			if (p_JProgramInfo->class23[0].p<1000)
				sprintf((char*)str,"有功功率:%d  W",(int)p_JProgramInfo->class23[zj_index-1].p);	//FOR698
			else
				sprintf((char*)str,"有功功率:%.2f kW",p_JProgramInfo->class23[zj_index-1].p*1.0/1000);	//FOR698
			pos.x = rect_Client.left + 4*FONTSIZE;
			pos.y += FONTSIZE*3;
			gui_textshow(str, pos, LCD_NOREV);

			memset(str, 0, 100);
			if (p_JProgramInfo->class23[0].q<1000)
				sprintf((char*)str,"无功功率:%d  Var",(int)p_JProgramInfo->class23[zj_index-1].q);
			else
				sprintf((char*)str,"无功功率:%.2f kVar",p_JProgramInfo->class23[zj_index-1].p*1.0/1000);	//FOR698
			pos.y += FONTSIZE*3;
			gui_textshow(str, pos, LCD_NOREV);

			//------------------------------------------------------------------
			pos.x = rect_Client.left+3*FONTSIZE;
			pos.y += FONTSIZE*4;
			mc_index = zj_index%2+1;
			sprintf(str, "脉冲计量%d [OI:%04x]", mc_index,0x2400 + mc_index);
			gui_textshow(str, pos, LCD_NOREV);

			memcpy(&rect, &rect_Client, sizeof(Rect));
			rect = gui_getstrrect((unsigned char*)str, pos);//获得字符串区域
			gui_reverserect(gui_changerect(rect, 2));//反显按钮

			pos.x = rect_Client.left + 4*FONTSIZE;
			memset(str, 0, 100);
			if (p_JProgramInfo->class12[mc_index].p<1000)
				sprintf((char*)str,"有功功率:%d  w",(int)p_JProgramInfo->class12[mc_index].p);
			else
				sprintf((char*)str,"有功功率:%.2f kVar",p_JProgramInfo->class12[mc_index].p*1.0/1000);	//FOR698
			pos.y += FONTSIZE*3;
			gui_textshow(str, pos, LCD_NOREV);

			if (p_JProgramInfo->class12[mc_index].q<1000)
				sprintf((char*)str,"无功功率:%d  Var",(int)p_JProgramInfo->class12[mc_index].q);
			else
				sprintf((char*)str,"无功功率:%.2f kVar",p_JProgramInfo->class12[mc_index].q*1.0/1000);	//FOR698
			pos.y += FONTSIZE*3;
			gui_textshow(str, pos, LCD_NOREV);


		}
		PressKey = NOKEY;
		delay(300);
	}
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++实时数据start+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/************************************************************************************************************************************************************
 --------------------------------------------------------------------实时数据---当前电量------------------------------------------------------------------------
 ***********************************************************************************************************************************************************/
void realE_showZJ(INT8U zj_index, int arr_data[], INT8U *surfix)
{
	int offset_y = 3;
	Point pos;
	TS curts;
	INT8U str[100];
	memset(str, 0, 100);
	sprintf((char*)str,"总% 12d %s",arr_data[0], surfix);
	gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+7*FONTSIZE);
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
	sprintf((char*)str,"尖% 12d %s", arr_data[1], surfix);
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
	sprintf((char*)str,"峰% 12d %s",arr_data[2], surfix);
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
	sprintf((char*)str,"平% 12d %s",arr_data[3], surfix);
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
	sprintf((char*)str,"谷% 12d %s",arr_data[4], surfix);
	pos.y += 2*FONTSIZE+offset_y;
	gui_textshow((char*)str, pos, LCD_NOREV);
//	TSGet(&curts);
//	memset(str, 0, 100);
//	if(curts.Year==0|| curts.Month==0||curts.Day==0)
//		sprintf((char*)str,"抄表时间 00/00/00 00:00");
//	else
//	sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
//			curts.Year-2000, curts.Month, curts.Day, curts.Hour, curts.Minute);
//	gui_setpos(&pos, rect_Client.left, rect_Client.top+18*FONTSIZE);
//	gui_textshow((char*)str, pos, LCD_NOREV);
	return;
}

void menu_realE(){
	char str[100];
	Point pos;
	int zf_index=1, zj_index=1, arr_data[5];//
	INT8U first_flg=0;
	memset(str,0,100);
	memset(arr_data, 0, 5*sizeof(int));
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){//如果未处于轮显状态
		switch(PressKey)
		{
		case UP://按“上”方向键总价组号减1
			zj_index--;
			if(zj_index<=0)
				zj_index = MAXNUM_SUMGROUP;
			break;
		case DOWN://按“下”方向键总加组号加1
			zj_index++;
			if(zj_index>MAXNUM_SUMGROUP)
				zj_index = 1;
			break;
		/*
		 * 一个总加组数据分4屏显示，由“左”“右”方向键控制
		 * */
		case LEFT://
			zf_index--;//显示前一屏内容
			if(zf_index<1)
				zf_index = 4;
			break;
		case RIGHT:
			zf_index++;//显示后一屏内容
			if(zf_index>4)
				zf_index = 1;
			break;
		case ESC:
			return;
		}
		if(PressKey!=NOKEY || first_flg==0){//如果有按键或者
			first_flg = 1;
			gui_clrrect(rect_Client);//清除客户显示区
			gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+3);
			memset(str, 0, 100);
			sprintf(str, "总加组%d [OI:%04x]", zj_index,0x2300 + zj_index);
			gui_textshow(str, pos, LCD_NOREV);
			pos.x = rect_Client.left+ 4*FONTSIZE;
			pos.y = rect_Client.top + 4*FONTSIZE-4;
			memset(str, 0, 100);
			switch(zf_index){
			case 1:
				arr_data[0] = (int)p_JProgramInfo->class23[zj_index-1].DayPALL;
				arr_data[1] = (int)p_JProgramInfo->class23[zj_index-1].DayP[0];
				arr_data[2] = (int)p_JProgramInfo->class23[zj_index-1].DayP[1];
				arr_data[3] = (int)p_JProgramInfo->class23[zj_index-1].DayP[2];
				arr_data[4] = (int)p_JProgramInfo->class23[zj_index-1].DayP[3];
				gui_textshow((char*)"当日有功总电能量", pos, LCD_NOREV);
				realE_showZJ(zj_index, arr_data, (INT8U*)"kWh");
				break;
			case 2:
				arr_data[0] = (int)p_JProgramInfo->class23[zj_index-1].DayQALL;
				arr_data[1] = (int)p_JProgramInfo->class23[zj_index-1].DayQ[0];
				arr_data[2] = (int)p_JProgramInfo->class23[zj_index-1].DayQ[1];
				arr_data[3] = (int)p_JProgramInfo->class23[zj_index-1].DayQ[2];
				arr_data[4] = (int)p_JProgramInfo->class23[zj_index-1].DayQ[3];
				gui_textshow((char*)"当日无功总电能量", pos, LCD_NOREV);
				realE_showZJ(zj_index, arr_data, (INT8U*)"kVArh");
				break;
			case 3:
				arr_data[0] = (int)p_JProgramInfo->class23[zj_index-1].MonthPALL;
				arr_data[1] = (int)p_JProgramInfo->class23[zj_index-1].MonthP[0];
				arr_data[2] = (int)p_JProgramInfo->class23[zj_index-1].MonthP[1];
				arr_data[3] = (int)p_JProgramInfo->class23[zj_index-1].MonthP[2];
				arr_data[4] = (int)p_JProgramInfo->class23[zj_index-1].MonthP[3];
				gui_textshow((char*)"当月有功总电能量", pos, LCD_NOREV);
				realE_showZJ(zj_index, arr_data, (INT8U*)"kWh");
				break;
			case 4:
				arr_data[0] = (int)p_JProgramInfo->class23[zj_index-1].MonthQALL;
				arr_data[1] = (int)p_JProgramInfo->class23[zj_index-1].MonthQ[0];
				arr_data[2] = (int)p_JProgramInfo->class23[zj_index-1].MonthQ[1];
				arr_data[3] = (int)p_JProgramInfo->class23[zj_index-1].MonthQ[2];
				arr_data[4] = (int)p_JProgramInfo->class23[zj_index-1].MonthQ[3];
				gui_textshow((char*)"当月无功总电能量", pos, LCD_NOREV);
				realE_showZJ(zj_index, arr_data, (INT8U*)"kVArh");
			}
		}
		PressKey = NOKEY;
		delay(300);
	}
}

 //-------------------实时数据----负荷曲线------------------------
#define POINT_TOTAL 96 //每个总加组96个点
#define POINT_PERPAGE 6 //每页显示6个点
//LcdDataItem item[100];//存储的所有数据项

void show_load_page(INT8U *filename, int zj_index, int point_begin, int point_num)
{
//	CurveData_SAVE curvedata;
	double dval=0;
	int i;
	Point pos;
	char str[100];
	gui_setpos(&pos, rect_Client.left+5*FONTSIZE, rect_Client.top+FONTSIZE);
	memset(str, 0, 100);
	sprintf(str, "当前负荷曲线 总加组%d", zj_index);
	gui_textshow(str, pos, LCD_NOREV);
	pos.x = rect_Client.left ;
	pos.y += 3*FONTSIZE;
	memset(str, 0, 100);
	sprintf(str, "序号  时标        数据");
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
	for(i=point_begin-1; i<point_begin-1 +point_num; i++){
		pos.x = rect_Client.left + FONTSIZE;
		pos.y += 2*FONTSIZE + 3;
		//总加组有功功率曲线数据
//		if(read_filedata_curve((char*)filename, zj_index, 1272, (i*15)/60, (i*15)%60, (void*)&curvedata)==1){		//FOR698
//			dval = bcd2double((INT8U*)curvedata.data, sizeof(curvedata.data), 2, positive);
//			sprintf(str,"%02d   %02d:%02d % 10.2f kW",i+1,(i*15)/60,(i*15)%60,dval);
//		}else
			sprintf(str,"%02d   %02d:%02d     xxxx.xx kW",i+1,(i*15)/60,(i*15)%60);
		gui_textshow((char*)str, pos, LCD_NOREV);
	}
}

void menu_loadcurve()
{
	TS ts;
//	tmget(&ts);
	TSGet(&ts);
	int zj_index=1, zf_index=1;
	INT8U first_flg=0;
	LcdDataItem item[MAXNUM_SUMGROUP][POINT_TOTAL];
	memset(item, 0, POINT_TOTAL*MAXNUM_SUMGROUP*sizeof(LcdDataItem));
	char filename[100];
	memset(filename, 0, 100);
	sprintf(filename, "/nand/curvedata/%04d%02d%02d.dat", ts.Year, ts.Month, ts.Day);//初始化文件名

	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		switch(PressKey)
		{
		case UP:
			zj_index--;
			if(zj_index<=0)
				zj_index = MAXNUM_SUMGROUP;
			break;
		case DOWN:
			zj_index++;
			if(zj_index>MAXNUM_SUMGROUP)
				zj_index = 1;
			break;
		case LEFT:
			zf_index--;
			if(zf_index<1)
				zf_index = POINT_TOTAL/POINT_PERPAGE;
			break;
		case RIGHT:
			zf_index++;
			if(zf_index>POINT_TOTAL/POINT_PERPAGE)
				zf_index = 1;
			break;
		case ESC:
			return;
		}
		if(PressKey!=NOKEY || first_flg==0){
			first_flg = 1;
			gui_clrrect(rect_Client);
			show_load_page((INT8U*)filename, zj_index, (zf_index-1)*POINT_PERPAGE+1, POINT_PERPAGE);
		}
		PressKey = NOKEY;
		delay(300);
	}
}

//-----实时数据-------功控记录--------
int showtime_rec(INT8U *buf, int index, char *str){
	INT32U year=0,month=0,day=0,hour=0,minute=0;
	bcd2int32u(&buf[0], 1, positive, &minute);
	bcd2int32u(&buf[1], 1, positive, &hour);
	bcd2int32u(&buf[2], 1, positive, &day);
	bcd2int32u(&buf[3], 1, positive, &month);
	bcd2int32u(&buf[4], 1, positive, &year);
	if(year!=0 && month!=0 && day!=0 && hour!=0&&minute!=0)
		year += 2000;
	sprintf(str, "%04d-%02d-%02d %02d:%02d", year,month,day,hour,minute);
	return index+5;
}

//获得指定ercno的事件
//int get_erc(int ercno, ERC *erc, int erc_num){
//	int i, count=0;
//	if(erc_num<=0)
//		return 0;

//	for(i=shmm_getdevstat()->erc.EC1; i>=0; i--){									// FOR698
//		if(shmm_getdevstat()->erc.ImpEvent[i].Buff[0]==ercno){
//			memcpy(&erc[count], &shmm_getdevstat()->erc.ImpEvent[i], sizeof(ERC));
//			count = (count+1)%erc_num;
//		}
//	}
//	for(i=255; i>shmm_getdevstat()->erc.EC1; i--){
//		if(shmm_getdevstat()->erc.ImpEvent[i].Buff[0]==ercno){
//			memcpy(&erc[count], &shmm_getdevstat()->erc.ImpEvent[i], sizeof(ERC));
//			count = (count+1)%erc_num;
//		}
//	}
//	for(i=shmm_getdevstat()->erc.EC2; i>=0; i--){
//		if(shmm_getdevstat()->erc.NorEvent[i].Buff[0]==ercno){
//			memcpy(&erc[count], &shmm_getdevstat()->erc.NorEvent[i], sizeof(ERC));
//			count = (count+1)%erc_num;
//		}
//	}
//	fprintf(stderr,"count1 = %d\n",count);
//	for(i=255; i>shmm_getdevstat()->erc.EC2; i--){
//		if(shmm_getdevstat()->erc.NorEvent[i].Buff[0]==ercno){
//			memcpy(&erc[count], &shmm_getdevstat()->erc.NorEvent[i], sizeof(ERC));
//			count = (count+1)%erc_num;
//		}
//	}
//	return count;
//}
/*
void gongctlrec(ERC *erc, int erc_index,int count){							//FOR698
	INT8U zjNo = 1;
	short tmp = 0;
	char str[100];
	Point pos;
	INT8U year=0, month=0, day=0, hour=0, minute=0;
	memset(str, 0, 100);
	gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+FONTSIZE);
	sprintf(str, "功控记录 (%d)", erc_index+1);
	gui_textshow(str, pos, LCD_NOREV);

	if(count != 0)
	{
		pos.x = rect_Client.left;
		pos.y += 2*FONTSIZE+2;
		memset(str, 0, 100);
		zjNo = (erc->Err6.ZongJIaNo&0x3f) + 1;
		sprintf(str, "总 加 组:    %d", zjNo);
		gui_textshow(str, pos, LCD_NOREV);
		pos.y += FONTSIZE*2+3;
		memset(str, 0, 100);
		if((erc->Err6.KongLeiBie&0x01)==0x01)
			sprintf(str, "功控类别:   时段控");
		else if((erc->Err6.KongLeiBie&0x02)==0x02)
			sprintf(str, "功控类别:   厂休控");
		else if((erc->Err6.KongLeiBie&0x04)==0x04)
			sprintf(str, "功控类别:   营业报停控");
		else if((erc->Err6.KongLeiBie&0x08)==0x08)
			sprintf(str, "功控类别:   功率下浮控");
		else
			sprintf(str, "功控类别:   未知类别");
		gui_textshow(str, pos, LCD_NOREV);
		pos.y += FONTSIZE*2+3;
		memset(str, 0, 100);
		lcd_A15toDate((INT8U*)&erc->Err6.Occur_Time,&year,&month,&day,&hour,&minute);
		sprintf(str, "跳闸时间:   %02d-%02d-%02d %02d:%02d", year, month, day, hour, minute);
		gui_textshow(str, pos, LCD_NOREV);
		pos.y += FONTSIZE*2+3;
		memset(str, 0, 100);
		sprintf(str, "跳闸轮次: ");
		getctrlround(erc->Err6.LunCi, str);
		gui_textshow(str, pos, LCD_NOREV);
		pos.y += FONTSIZE*2+3;
		memset(str, 0, 100);
		tmp = erc->Err6.P_DingZhi[0]|((erc->Err6.P_DingZhi[1]&0x0f)<<8);
		sprintf(str, "功率定值:   % 10.2f kW", lcd_A02toDouble(tmp)/1000);
		gui_textshow(str, pos, LCD_NOREV);
		pos.y += FONTSIZE*2+3;
		memset(str, 0, 100);
		tmp = erc->Err6.P_Old[0]|((erc->Err6.P_Old[1]&0x0f)<<8);
		sprintf(str, "跳闸前功率: % 10.2f kW", lcd_A02toDouble(tmp)/1000);
		gui_textshow(str, pos, LCD_NOREV);
		pos.y += FONTSIZE*2+3;
		memset(str, 0, 100);
		tmp = erc->Err6.Delay_2_P[0]|((erc->Err6.Delay_2_P[1]&0x0f)<<8);
		sprintf(str, "2分钟后功率:% 10.2f kW", lcd_A02toDouble(tmp)/1000);
		gui_textshow(str, pos, LCD_NOREV);
	}
	else
	{
		pos.x = rect_Client.left+10*FONTSIZE;
		pos.y += FONTSIZE*4+3;
		memset(str, 0, 100);
		gui_textshow("无功控发生", pos, LCD_NOREV);
	}
}
*/

//功控记录
void menu_gongctlrec(){
/*
	int erc_index=0, erc_count=0;
	INT8U first_flg=0;
	ERC erc[256];
	memset(erc, 0, 256*sizeof(ERC));
	erc_count = get_erc(6, erc, 256);
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){//如果未处于轮显状态
		switch(PressKey)
		{//一个总加组数据分4屏显示，由“左”“右”方向键控制
		case UP:
		case LEFT://
			if(erc_count>0){
				erc_index--;//显示前一屏内容
				if(erc_index<0)
					erc_index = erc_count-1;
			}else
				erc_index = 0;
			break;
		case DOWN:
		case RIGHT:
			if(erc_count>0){
				erc_index++;//显示后一屏内容
				if(erc_index>=erc_count)
					erc_index = 0;
			}else
				erc_index = 0;
			break;
		case ESC:
			return;
		}
		if(PressKey!=NOKEY || first_flg==0){
			first_flg = 1;
			gui_clrrect(rect_Client);
			gongctlrec(&erc[erc_index], erc_index,erc_count);
		}
		PressKey = NOKEY;
		delay(300);
	}
	*/
}
/*
//---------------实时数据------电控记录-----------------
void dianctlrec(ERC *erc, int erc_index,int count){
	int fix_value=0,c_suffix=0;
	char str[100];
	Point pos;
	INT8U year=0, month=0, day=0, hour=0, minute=0;
	memset(str, 0, 100);
	gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+FONTSIZE);
	sprintf(str, "电控记录 (%d)", erc_index+1);
	gui_textshow(str, pos, LCD_NOREV);

	if(count != 0)
	{
		pos.x = rect_Client.left;
		pos.y += 2*FONTSIZE+2;
		memset(str, 0, 100);
		sprintf(str, "总 加 组:    %d", (erc->Err7.ZongJIaNo&0x3f) + 1);
		gui_textshow(str, pos, LCD_NOREV);
		pos.y += FONTSIZE*2+3;
		memset(str, 0, 100);
		fprintf(stderr,"err7 = %c\n",erc->Err7.KongLeiBie);
		if((erc->Err7.KongLeiBie&0x01)==0x01)
			sprintf(str, "电控类别:   月电控");
		else if((erc->Err7.KongLeiBie&0x02)==0x02)
			sprintf(str, "电控类别:   购电控");
		else
			sprintf(str, "电控类别:   未知类别");
		gui_textshow(str, pos, LCD_NOREV);
		pos.y += FONTSIZE*2+3;
		memset(str, 0, 100);
		lcd_A15toDate((INT8U*)&erc->Err7.Occur_Time,&year,&month,&day,&hour,&minute);
		sprintf(str, "跳闸时间:   %02d-%02d-%02d %02d:%02d", year, month, day, hour, minute);
		gui_textshow(str, pos, LCD_NOREV);
		pos.y += FONTSIZE*2+3;
		memset(str, 0, 100);
		sprintf(str, "跳闸轮次: ");
		getctrlround(erc->Err7.LunCi, str);
		gui_textshow(str, pos, LCD_NOREV);
		pos.y += FONTSIZE*2+3;
		memset(str, 0, 100);
		fix_value = lcd_A03toint_c(erc->Err7.DD_DingZhi, &c_suffix);
		sprintf(str, "电能量定值:   % 8d %s", fix_value, c_suffix?"MWh":"kWh");
		gui_textshow(str, pos, LCD_NOREV);
		memset(str, 0, 100);
		fix_value = lcd_A03toint_c(erc->Err7.DD_Old, &c_suffix);
		sprintf(str, "跳闸时电能量: % 8d %s", fix_value, c_suffix?"MWh":"kWh");
		pos.y += FONTSIZE*2+3;
		gui_textshow(str, pos, LCD_NOREV);
	}
	else{
		pos.x = rect_Client.left+10*FONTSIZE;
		pos.y += FONTSIZE*4+3;
		memset(str, 0, 100);
		gui_textshow("无电控发生", pos, LCD_NOREV);
	}
}
*/

void menu_dianctlrec(){

//	int erc_index=0, erc_count=0;									//FOR698
//	INT8U first_flg=0;
//	ERC erc[256];
//	memset(erc, 0, 256*sizeof(ERC));
//	erc_count = get_erc(7, erc, 256);
//	PressKey = NOKEY;
//	while(g_LcdPoll_Flag==LCD_NOTPOLL){//如果未处于轮显状态
//		switch(PressKey)
//		{
//		//一个总加组数据分4屏显示，由“左”“右”方向键控制
//		case UP:
//		case LEFT://
//			if(erc_count>0){
//				erc_index--;//显示前一屏内容
//				if(erc_index<0)
//					erc_index = erc_count-1;
//			}else
//				erc_index = 0;
//			break;
//		case DOWN:
//		case RIGHT:
//			if(erc_count>0){
//				erc_index++;//显示后一屏内容
//				if(erc_index>=erc_count)
//					erc_index = 0;
//			}else
//				erc_index = 0;
//			break;
//		case ESC:
//			return;
//		}
//		if(PressKey!=NOKEY || first_flg==0){
//			first_flg = 1;
//			gui_clrrect(rect_Client);
//			dianctlrec(&erc[erc_index],erc_index,erc_count);
//		}
//		PressKey = NOKEY;
//		delay(300);
//	}
}

/*************************************************************************end*******************************************************************************/

/***********************************************************************************************************************************************************
----------------------------------------------------------------实时数据-----------遥控记录---------------------------------------------------------------------
 * *********************************************************************************************************************************************************/
//void yaoctlrec(ERC *erc, int erc_index,int count){
//	short tmp = 0;
//	char str[100];
//	Point pos;
//	INT8U year=0, month=0, day=0, hour=0, minute=0;
//	memset(str, 0, 100);
//	gui_setpos(&pos, rect_Client.left+8*FONTSIZE, rect_Client.top+FONTSIZE);							//FOR698
//	sprintf(str, "遥控记录 (%d)", erc_index+1);
//	gui_textshow((char*)str, pos, LCD_NOREV);
//	if(count != 0)
//	{
//		pos.x = rect_Client.left;
//		pos.y += 3*FONTSIZE;
//		memset(str, 0, 100);
//		lcd_A15toDate((INT8U*)&erc->Err5.Occur_Time,&year,&month,&day,&hour,&minute);
//		sprintf(str, "跳闸时间:   %02d-%02d-%02d %02d:%02d", year, month, day, hour, minute);
//		gui_textshow(str, pos, LCD_NOREV);
//		pos.y += FONTSIZE*2+3;
//		memset(str, 0, 100);
//		sprintf(str, "跳闸轮次: ");
//		getctrlround(erc->Err5.LunCi, str);
//		gui_textshow(str, pos, LCD_NOREV);
//		pos.y += FONTSIZE*2+3;
//		memset(str, 0, 100);
//		tmp = erc->Err5.P[0]|(erc->Err5.P[1]<<8);
//		sprintf(str, "跳闸时功率: % 10.2f kW", lcd_A02toDouble(tmp));
//		gui_textshow(str, pos, LCD_NOREV);
//		pos.y += FONTSIZE*2+3;
//		memset(str, 0, 100);
//		tmp = erc->Err5.Delay_2_P[0]|(erc->Err5.Delay_2_P[1]<<8);
//		sprintf(str, "2分钟后功率:% 10.2f kW", lcd_A02toDouble(tmp));
//		gui_textshow(str, pos, LCD_NOREV);
//	}else{
//		pos.x = rect_Client.left+10*FONTSIZE;
//		pos.y += FONTSIZE*4+3;
//		memset(str, 0, 100);
//		gui_textshow("无遥控记录", pos, LCD_NOREV);
//	}
//}

void menu_yaoctlrec(){
	Class7_Object event3115;	//遥控跳闸记录

//	int erc_index=0, erc_count=0;
//	INT8U first_flg=0;
//	ERC erc[256];
//	memset(erc, 0, 256*sizeof(ERC));
//	erc_count = get_erc(5, erc, 256);
//	PressKey = NOKEY;
//	while(g_LcdPoll_Flag==LCD_NOTPOLL){//如果未处于轮显状态									//FOR698
//		switch(PressKey)
//		{
//		//一个总加组数据分4屏显示，由“左”“右”方向键控制
//		case UP:
//		case LEFT://
//			if(erc_count>0){
//				erc_index--;//显示前一屏内容
//				if(erc_index<0)
//					erc_index = erc_count-1;
//			}else
//				erc_index = 0;
//			break;
//		case DOWN:
//		case RIGHT:
//			if(erc_count>0){
//				erc_index++;//显示后一屏内容
//				if(erc_index>=erc_count)
//					erc_index = 0;
//			}else
//				erc_index = 0;
//			break;
//		case ESC:
//			return;
//		}
//		if(PressKey!=NOKEY || first_flg==0){
//			first_flg = 1;
//			gui_clrrect(rect_Client);
//			yaoctlrec(&erc[erc_index], erc_index,erc_count);
//		}
//		PressKey = NOKEY;
//		delay(300);
//	}

}

/**********************************************************************end**********************************************************************************/

/***********************************************************************************************************************************************************
----------------------------------------------------------------实时数据-----失电记录--------------------------------------------------------------------------
 * *********************************************************************************************************************************************************/
/*
void shidianrec(ERC *erc, int erc_index,int count){

	char str[100];
	INT8U time[5];
	Point pos;
	memset(str, 0, 100);
	memset(time, 0, 5);
	gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+2*FONTSIZE);					//FOR698
	memset(str, 0, 100);
	sprintf(str, "失电记录 (%d)",erc_index+1);
	gui_textshow(str, pos, LCD_NOREV);
	if(count != 0)
	{
		pos.x = rect_Client.left;
		pos.y += FONTSIZE*4;
		memset(str, 0, 100);
		if(erc->Err14.Ting_Time.BCD05 == 0xee)//如果停电是ee，显示上电时间
		{
			memset(time, 0, 5);
			lcd_A15toDate((INT8U*)&erc->Err14.Shang_Time,&time[4],&time[3],&time[2],&time[1],&time[0]);
			sprintf(str, "上电时间:   %02d-%02d-%02d %02d:%02d", time[4],time[3],time[2],time[1],time[0]);
			gui_textshow(str, pos, LCD_NOREV);
		}
		else
		{
			memset(time, 0, 5);
			lcd_A15toDate((INT8U*)&erc->Err14.Ting_Time,&time[4],&time[3],&time[2],&time[1],&time[0]);
			sprintf(str, "停电时间:   %02d-%02d-%02d %02d:%02d", time[4],time[3],time[2],time[1],time[0]);
			gui_textshow(str, pos, LCD_NOREV);
		}
		if(erc->Err14.Shang_Time.BCD05 == 0xee)///否则如果上电时间是ee，显示停电时间
		{
			memset(time, 0, 5);
			lcd_A15toDate((INT8U*)&erc->Err14.Ting_Time,&time[4],&time[3],&time[2],&time[1],&time[0]);
			sprintf(str, "停电时间:   %02d-%02d-%02d %02d:%02d", time[4],time[3],time[2],time[1],time[0]);
			gui_textshow(str, pos, LCD_NOREV);
		}
		else
		{
			memset(time, 0, 5);
			lcd_A15toDate((INT8U*)&erc->Err14.Shang_Time,&time[4],&time[3],&time[2],&time[1],&time[0]);
			sprintf(str, "上电时间:   %02d-%02d-%02d %02d:%02d", time[4],time[3],time[2],time[1],time[0]);
			gui_textshow(str, pos, LCD_NOREV);
		}
	}
	else{
		pos.x = rect_Client.left+8*FONTSIZE;
		pos.y += FONTSIZE*4+3;
		memset(str, 0, 100);
		gui_textshow("无停上电发生",pos,LCD_NOREV);
	}
}
*/

void menu_shidianrec(){
//	int erc_index=0, erc_count=0;
//	INT8U first_flg=0;
//	ERC erc[256];
//	memset(erc, 0, 256*sizeof(ERC));
//	erc_count = get_erc(14, erc, 256);
//	PressKey = NOKEY;
//	while(g_LcdPoll_Flag==LCD_NOTPOLL){//如果未处于轮显状态						//FOR698
//		switch(PressKey)
//		{
//		//一个总加组数据分4屏显示，由“左”“右”方向键控制
//		case UP:
//		case LEFT://
//			if(erc_count>0){
//				erc_index--;//显示前一屏内容
//				if(erc_index<0)
//					erc_index = erc_count-1;
//			}else
//				erc_index = 0;
//			break;
//		case DOWN:
//		case RIGHT:
//			if(erc_count>0){
//				erc_index++;//显示后一屏内容
//				if(erc_index>=erc_count)
//					erc_index = 0;
//			}else
//				erc_index = 0;
//			break;
//		case ESC:
//			return;
//		}
//		if(PressKey!=NOKEY || first_flg==0){
//			first_flg = 1;
//			gui_clrrect(rect_Client);
////			dbg_prt("\n erc_count=%d  erc_index=%d",erc_count, erc_index);
//			shidianrec(&erc[erc_index], erc_index, erc_count);
//		}
//		PressKey = NOKEY;
//		delay(300);
//	}
}

/****************************************************************************end****************************************************************************/

/***********************************************************************************************************************************************************
--------------------------------------------------------------------实时数据--------交流采样信息----------------------------------------------------------------
 * *********************************************************************************************************************************************************/
void menu_realjc(){
	int mqcount=0;
	LcdDataItem item[100];//存储的所有数据项
	memset(item, 0, 100*sizeof(LcdDataItem));
	int cldno=1;
	show_realdata(cldno, item, mqcount);
}

struct shiduanpara_ts{
	TS ts1;
	TS ts2;
};

int initshiduan(const INT8U period_powerctrl[][4],struct shiduanpara_ts *sd_ts){
	int i, j, sdts_num=0;
	INT8U tmp=128;
	for(i=0; i<12; i++){
		for(j=0;j<4;j++){
			if(period_powerctrl[i][j]!=tmp){
				tmp = period_powerctrl[i][j];
				if(period_powerctrl[i][j]==1||period_powerctrl[i][j]==2)
				{
					sd_ts[sdts_num].ts1.Hour = i*2 + j/2;
					sd_ts[sdts_num].ts1.Minute = (j%2)*30;
					if(j==0 || j==2)
						sd_ts[sdts_num].ts2.Hour = sd_ts[sdts_num].ts1.Hour;
					else
						sd_ts[sdts_num].ts2.Hour = sd_ts[sdts_num].ts1.Hour+1;
					sd_ts[sdts_num].ts2.Minute = ((j+1)%2)*30;
					sdts_num++;
				}
			}
		}
	}
	return sdts_num;
}

/**********************************************************************end**********************************************************************************/
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++实时数据end++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++参数定值start++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/************************************************************************************************************************************************************
--------------------------------------------------------------参数定值-----------时段控参数----------------------------------------------------------------------
************************************************************************************************************************************************************/
//时段控
void menu_shiduanpara(){
	CLASS_8103 c8103;
	Point pos;
	char str[100];
	INT8U  first_flg=0;
	int unite_index=1, dingzhi=1;
	struct shiduanpara_ts sd_ts[48];
	memset(sd_ts, 0, 48*sizeof(struct shiduanpara_ts));
	memset(&c8103,0,sizeof(CLASS_8103));
	readCoverClass(0x8103, 0, &c8103, sizeof(CLASS_8103),para_vari_save);

	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		if(PressKey==ESC)
			break;
		switch(PressKey)
		{
		case LEFT:
			//3套定值
			dingzhi++;
			dingzhi = dingzhi % 3;
			if (dingzhi==0)
				dingzhi = 3;
			break;
		case UP:
			unite_index--;
			if(unite_index<=0)
				unite_index = MAXNUM_SUMGROUP;
			break;
		case RIGHT:
			//3套定值
			dingzhi--;
			dingzhi = dingzhi % 3;
			if (dingzhi==0)
				dingzhi = 3;
			break;
		case DOWN:
			unite_index++;
			if(unite_index>MAXNUM_SUMGROUP)
				unite_index = 1;
			break;
		case ESC:
			return;
		}
		int enable_i=0,ifenable=0;
		if(PressKey!=NOKEY || first_flg==0){
			first_flg = 1;
			for(enable_i=0;enable_i<8;enable_i++)
			{
				if (c8103.enable[enable_i].name == c8103.list[unite_index-1].index)
				{
					ifenable = c8103.enable[enable_i].state;
					break;
				}
			}
			gui_clrrect(rect_Client);
			gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+FONTSIZE);
			memset(str, 0, 100);
			sprintf(str,"配置单元%d [总加组%04x]", unite_index, c8103.list[unite_index-1].index);
			gui_textshow(str, pos, LCD_NOREV);

			if (ifenable==0 )//该总加组没投入！
			{
				pos.x = rect_Client.left+5*FONTSIZE;
				pos.y += 5*FONTSIZE;
				memset(str, 0, 100);
				sprintf(str,"未投入");
				gui_textshow(str, pos, LCD_NOREV);
				PressKey = NOKEY;
				delay(300);
				continue;
			}
			pos.y += 3*FONTSIZE;
			memset(str, 0, 100);
			sprintf(str,"方案标识 %02x  浮动系数 %d",c8103.list[unite_index-1].sign,c8103.list[unite_index-1].para);
			gui_textshow(str, pos, LCD_NOREV);

			pos.y += 3*FONTSIZE;
			memset(str, 0, 100);
			sprintf(str,"第%d套定值",dingzhi);
			gui_textshow(str, pos, LCD_NOREV);
			pos.y += 3*FONTSIZE;
			memset(str, 0, 100);

			switch(dingzhi)
			{
				case 1://第一套定值
					sprintf(str,"时段1:%05d 时段2:%05d",(int)c8103.list[unite_index-1].v1.t1,(int)c8103.list[unite_index-1].v1.t2);
					gui_textshow(str, pos, LCD_NOREV);
					pos.y += 3*FONTSIZE;
					memset(str, 0, 100);
					sprintf(str,"时段3:%05d 时段4:%05d",(int)c8103.list[unite_index-1].v1.t3,(int)c8103.list[unite_index-1].v1.t4);
					gui_textshow(str, pos, LCD_NOREV);
					pos.y += 3*FONTSIZE;
					memset(str, 0, 100);
					sprintf(str,"时段5:%05d 时段6:%05d",(int)c8103.list[unite_index-1].v1.t5,(int)c8103.list[unite_index-1].v1.t6);
					gui_textshow(str, pos, LCD_NOREV);
					pos.y += 3*FONTSIZE;
					memset(str, 0, 100);
					sprintf(str,"时段7:%05d 时段8:%05d",(int)c8103.list[unite_index-1].v1.t7,(int)c8103.list[unite_index-1].v1.t8);
					gui_textshow(str, pos, LCD_NOREV);
					break;
				case 2://第二套定值
					sprintf(str,"时段1:%05d 时段2:%05d",(int)c8103.list[unite_index-1].v2.t1,(int)c8103.list[unite_index-1].v2.t2);
					gui_textshow(str, pos, LCD_NOREV);
					pos.y += 3*FONTSIZE;
					memset(str, 0, 100);
					sprintf(str,"时段3:%05d 时段4:%05d",(int)c8103.list[unite_index-1].v2.t3,(int)c8103.list[unite_index-1].v2.t4);
					gui_textshow(str, pos, LCD_NOREV);
					pos.y += 3*FONTSIZE;
					memset(str, 0, 100);
					sprintf(str,"时段5:%05d 时段6:%05d",(int)c8103.list[unite_index-1].v2.t5,(int)c8103.list[unite_index-1].v2.t6);
					gui_textshow(str, pos, LCD_NOREV);
					pos.y += 3*FONTSIZE;
					memset(str, 0, 100);
					sprintf(str,"时段7:%05d 时段8:%05d",(int)c8103.list[unite_index-1].v2.t7,(int)c8103.list[unite_index-1].v2.t8);
					gui_textshow(str, pos, LCD_NOREV);
					break;
				case 3://第三套定值
					sprintf(str,"时段1:%05d 时段2:%05d",(int)c8103.list[unite_index-1].v3.t1,(int)c8103.list[unite_index-1].v3.t2);
					gui_textshow(str, pos, LCD_NOREV);
					pos.y += 3*FONTSIZE;
					memset(str, 0, 100);
					sprintf(str,"时段3:%05d 时段4:%05d",(int)c8103.list[unite_index-1].v3.t3,(int)c8103.list[unite_index-1].v3.t4);
					gui_textshow(str, pos, LCD_NOREV);
					pos.y += 3*FONTSIZE;
					memset(str, 0, 100);
					sprintf(str,"时段5:%05d 时段6:%05d",(int)c8103.list[unite_index-1].v3.t5,(int)c8103.list[unite_index-1].v3.t6);
					gui_textshow(str, pos, LCD_NOREV);
					pos.y += 3*FONTSIZE;
					memset(str, 0, 100);
					sprintf(str,"时段7:%05d 时段8:%05d",(int)c8103.list[unite_index-1].v3.t7,(int)c8103.list[unite_index-1].v3.t8);
					gui_textshow(str, pos, LCD_NOREV);
					break;
			}
		}
		PressKey = NOKEY;
		delay(300);
	}
}

/************************************************************************************************************************************************************
--------------------------------------------------------------参数定值-----------厂休控参数----------------------------------------------------------------------
************************************************************************************************************************************************************/
//厂休控
void menu_changxiupara(){
	char week[7][3]={"一","二","三","四","五","六","日"};
	int unite_index=1, dingzhi=1;
	Point pos;
	CLASS_8104 c8104; //厂休控
	readCoverClass(0x8104, 0, (void *) &c8104, sizeof(CLASS_8104),para_vari_save);

	char str[100], hour=0, minute=0;
	INT8U first_flg=0;
	int zj_index=1, i;
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		if(PressKey==ESC)
			break;
		switch(PressKey)
		{
		case LEFT:
		case UP:
			unite_index--;
			if(unite_index<=0)
				unite_index = MAXNUM_SUMGROUP;
			break;
		case RIGHT:
		case DOWN:
			unite_index++;
			if(unite_index>MAXNUM_SUMGROUP)
				unite_index = 1;
			break;
		case ESC:
			return;
		}

		int enable_i=0,ifenable=0;
		if(PressKey!=NOKEY || first_flg==0){
			first_flg = 1;
			for(enable_i=0;enable_i<8;enable_i++)
			{
				fprintf(stderr,"\n  ------===== %d  ====-------- %04x ",enable_i,c8104.enable[enable_i].name );
				if (c8104.enable[enable_i].name == c8104.list[unite_index-1].index)
				{
					fprintf(stderr,"\n  ------===== %d  ====--------state %d ",enable_i,c8104.enable[enable_i].state );
					ifenable = c8104.enable[enable_i].state;
					break;
				}
				fprintf(stderr,"\n\n");
			}
			gui_clrrect(rect_Client);
			gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+FONTSIZE);
			memset(str, 0, 100);
			sprintf(str,"配置单元%d [总加组%04x]", unite_index, c8104.list[unite_index-1].index);
			gui_textshow(str, pos, LCD_NOREV);
			if (ifenable==0 )//该总加组没投入！
			{
				pos.x = rect_Client.left+5*FONTSIZE;
				pos.y += 5*FONTSIZE;
				memset(str, 0, 100);
				sprintf(str,"未投入");
				gui_textshow(str, pos, LCD_NOREV);
				PressKey = NOKEY;
				delay(300);
				continue;
			}

			pos.y += 3*FONTSIZE;
			memset(str, 0, 100);
			if (c8104.list[unite_index-1].v>0)
				sprintf(str, "厂休控定值 %04d W", c8104.list[unite_index-1].v);
			else
				sprintf(str, "厂休控定值 错误 ");
			gui_textshow(str, pos, LCD_NOREV);
			pos.y += 3*FONTSIZE;
			memset(str, 0, 100);
			sprintf(str, "起始时间:");
			gui_textshow(str, pos, LCD_NOREV);

			pos.y += 3*FONTSIZE;
			memset(str, 0, 100);
			if (c8104.list[unite_index-1].start.year.data==0xff || c8104.list[unite_index-1].start.month.data==0xff || c8104.list[unite_index-1].start.day.data==0xff )
			{
				sprintf(str, "日期无效 时间:%02d:%02d",
						c8104.list[unite_index-1].start.hour.data,
						c8104.list[unite_index-1].start.min.data);
			}else
			{
				sprintf(str, "%04d-%02d-%2d %02d:%02d",
						c8104.list[unite_index-1].start.year.data,
						c8104.list[unite_index-1].start.month.data,
						c8104.list[unite_index-1].start.day.data,
						c8104.list[unite_index-1].start.hour.data,
						c8104.list[unite_index-1].start.min.data);
			}
			gui_textshow(str, pos, LCD_NOREV);

			pos.y += 3*FONTSIZE;
			memset(str, 0, 100);
			sprintf(str, "延续时间: %d 分钟",c8104.list[unite_index-1].sustain);
			gui_textshow(str, pos, LCD_NOREV);

			pos.y += 3*FONTSIZE;
			memset(str, 0, 100);
			char week_str[30];
			memset(week_str,0,30);
			for(i=0;i<8;i++)
			{
				if(c8104.list[unite_index-1].noDay & (1<<i))
				{
					strcat(week_str,week[i]);
				}
			}
			sprintf(str, "厂休日:%s",week_str);
			gui_textshow(str, pos, LCD_NOREV);
		}
		PressKey = NOKEY;
		delay(300);
	}
}

/************************************************************************************************************************************************************
--------------------------------------------------------------参数定值-----------报停控参数----------------------------------------------------------------------
************************************************************************************************************************************************************/
//报停控
void menu_baotingpara(){
	Point pos;
	int unite_index=1;
	char str[100], year[2], month[2], day[2];
	INT8U  first_flg=0;
	int zj_index=1;
	CLASS_8105 c8105; //营业报停控
	readCoverClass(0x8105, 0, (void *) &c8105, sizeof(CLASS_8105),para_vari_save);

	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		if(PressKey==ESC)
			break;
		switch(PressKey)
		{
		case LEFT:
		case UP:
			unite_index--;
			if(unite_index<=0)
				unite_index = MAXNUM_SUMGROUP;
			break;
		case RIGHT:
		case DOWN:
			unite_index++;
			if(unite_index>MAXNUM_SUMGROUP)
				unite_index = 1;
			break;
		case ESC:
			return;
		}
		if(PressKey!=NOKEY || first_flg==0){
			int enable_i=0,ifenable=0;
			first_flg = 1;
			gui_clrrect(rect_Client);
			gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+FONTSIZE);
			memset(str, 0, 100);
			sprintf(str,"配置单元%d [总加组%04x]", unite_index, c8105.list[unite_index-1].index);
			gui_textshow(str, pos, LCD_NOREV);

			for(enable_i=0;enable_i<8;enable_i++)
			{
				if (c8105.enable[enable_i].name == c8105.list[unite_index-1].index)
				{
					ifenable = c8105.enable[enable_i].state;
					break;
				}
			}
			if (ifenable==0 )//该总加组没投入！
			{
				pos.x = rect_Client.left+5*FONTSIZE;
				pos.y += 5*FONTSIZE;
				memset(str, 0, 100);
				sprintf(str,"未投入");
				gui_textshow(str, pos, LCD_NOREV);
				PressKey = NOKEY;
				delay(300);
				continue;
			}
			pos.y += 3*FONTSIZE;
			memset(str, 0, 100);
			sprintf(str, "起始时间:");
			gui_textshow(str, pos, LCD_NOREV);

			pos.y += 3*FONTSIZE;
			memset(str, 0, 100);
			sprintf(str, "%04d-%02d-%2d 02%d:02%d",
					c8105.list[unite_index-1].start.year.data,
					c8105.list[unite_index-1].start.month.data,
					c8105.list[unite_index-1].start.day.data,
					c8105.list[unite_index-1].start.hour.data,
					c8105.list[unite_index-1].start.min.data);
			gui_textshow(str, pos, LCD_NOREV);

			pos.y += 3*FONTSIZE;
			memset(str, 0, 100);
			sprintf(str, "结束时间:");
			gui_textshow(str, pos, LCD_NOREV);

			pos.y += 3*FONTSIZE;
			memset(str, 0, 100);
			sprintf(str, "%04d-%02d-%2d 02%d:02%d",
					c8105.list[unite_index-1].end.year.data,
					c8105.list[unite_index-1].end.month.data,
					c8105.list[unite_index-1].end.day.data,
					c8105.list[unite_index-1].end.hour.data,
					c8105.list[unite_index-1].end.min.data);
			gui_textshow(str, pos, LCD_NOREV);


			pos.y += 3*FONTSIZE;
			memset(str, 0, 100);
			if (c8105.list[unite_index-1].v>0)
				sprintf(str, "定值 %04d kW", c8105.list[unite_index-1].v);
			else
				sprintf(str, "定值 错误 ");
			gui_textshow(str, pos, LCD_NOREV);
		}
		PressKey = NOKEY;
		delay(300);
	}
}

/************************************************************************************************************************************************************
--------------------------------------------------------------参数定值-----------下浮控参数----------------------------------------------------------------------
************************************************************************************************************************************************************/
//下浮控
void menu_xiafupara(){
	//获取下浮控参数
	CLASS_8106 c8106;

	double fixvalue;
	int ctrl_flg;
	Point pos;
	char str[100];
	int zj_index=1, first_flg=0;
	readCoverClass(0x8106, 0, (void *) &c8106, sizeof(CLASS_8106),para_vari_save);

	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		if(PressKey==ESC)
			break;
		switch(PressKey)
		{
		case LEFT:
		case UP:
		case RIGHT:
		case DOWN:
			break;
		case ESC:
			return;
		}
		if(PressKey!=NOKEY || first_flg==0){
			first_flg = 1;

			gui_clrrect(rect_Client);
			gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+FONTSIZE);
			memset(str, 0, 100);
			sprintf(str, "总加组 [%04x]", c8106.index);
			gui_textshow(str, pos, LCD_NOREV);
			pos.x = rect_Client.left;
			pos.y += 2*FONTSIZE+3;
			memset(str, 0, 100);
			sprintf(str, "定值滑差时间: %d 分钟",c8106.list.down_huacha);
			gui_textshow(str, pos, LCD_NOREV);

			pos.y += 2*FONTSIZE+1;
			memset(str, 0, 100);
			sprintf(str, "定值浮动系数: %d %",c8106.list.down_xishu);
			gui_textshow(str, pos, LCD_NOREV);

			pos.y += 2*FONTSIZE+1;
			memset(str, 0, 100);
			sprintf(str, "控后冻结延时: %d 分钟",c8106.list.down_freeze);
			gui_textshow(str, pos, LCD_NOREV);

			pos.y += 2*FONTSIZE+1;
			memset(str, 0, 100);
			float khh = c8106.list.down_ctrl_time*0.5;
			sprintf(str, "控制时间: %.2f 小时",khh);
			gui_textshow(str, pos, LCD_NOREV);

			pos.y += 2*FONTSIZE+1;
			memset(str, 0, 100);
			sprintf(str, "第一轮告警时间: %02d 分钟",c8106.list.t1);
			gui_textshow(str, pos, LCD_NOREV);
			pos.y += 2*FONTSIZE+1;
			memset(str, 0, 100);
			sprintf(str, "第二轮告警时间: %02d 分钟",c8106.list.t2);
			gui_textshow(str, pos, LCD_NOREV);
			pos.y += 2*FONTSIZE+1;
			memset(str, 0, 100);
			sprintf(str, "第三轮告警时间: %02d 分钟",c8106.list.t3);
			gui_textshow(str, pos, LCD_NOREV);
			pos.y += 2*FONTSIZE+1;
			memset(str, 0, 100);
			sprintf(str, "第四轮告警时间: %02d 分钟",c8106.list.t4);
			gui_textshow(str, pos, LCD_NOREV);
		}
		PressKey = NOKEY;
		delay(300);
	}
}

/************************************************************************************************************************************************************
--------------------------------------------------------------参数定值-----------月电控参数----------------------------------------------------------------------
************************************************************************************************************************************************************/
//月电控
void menu_yuedianpara(){
	int unite_index=1;
	CLASS_8108 c8108;

	int suffix=0, fix_value=0;
	Point pos;
	char str[100];
	int zj_index=1, first_flg=0;

	readCoverClass(0x8108, 0, (void *) &c8108, sizeof(CLASS_8108),para_vari_save);

	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		switch(PressKey)
		{
		case LEFT:
		case UP:
			unite_index--;
			if(unite_index<=0)
				unite_index = MAXNUM_SUMGROUP;
			break;
		case RIGHT:
		case DOWN:
			unite_index++;
			if(unite_index>MAXNUM_SUMGROUP)
				unite_index = 1;
			break;
		case ESC:
			return;
		}
		if(PressKey!=NOKEY || first_flg==0){

			int enable_i=0,ifenable=0;
			first_flg = 1;
			gui_clrrect(rect_Client);
			gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+FONTSIZE);
			memset(str, 0, 100);
			sprintf(str,"配置单元%d [总加组%04x]", unite_index, c8108.list[unite_index-1].index);
			gui_textshow(str, pos, LCD_NOREV);

			for(enable_i=0;enable_i<8;enable_i++)
			{
				if (c8108.enable[enable_i].name == c8108.list[unite_index-1].index)
				{
					ifenable = c8108.enable[enable_i].state;
					break;
				}
			}
			if (ifenable==0 )//该总加组没投入！
			{
				pos.x = rect_Client.left+5*FONTSIZE;
				pos.y += 5*FONTSIZE;
				memset(str, 0, 100);
				sprintf(str,"未投入");
				gui_textshow(str, pos, LCD_NOREV);
				PressKey = NOKEY;
				delay(300);
				continue;
			}
			pos.y += 3*FONTSIZE;
			memset(str, 0, 100);
			sprintf(str, "电控定值: %d kW",c8108.list[unite_index-1].v);
			gui_textshow(str, pos, LCD_NOREV);

			pos.y += 3*FONTSIZE;
			memset(str, 0, 100);
			sprintf(str, "报警门限系数: %d",c8108.list[unite_index-1].para);
			gui_textshow(str, pos, LCD_NOREV);

			pos.y += 3*FONTSIZE;
			memset(str, 0, 100);
			sprintf(str, "定值浮动系数:%d",c8108.list[unite_index-1].flex);
			gui_textshow(str, pos, LCD_NOREV);
		}
		PressKey = NOKEY;
		delay(300);
	}
}

/************************************************************************************************************************************************************
--------------------------------------------------------------参数定值-----------KvKiKp------------------------------------------------------------------------
************************************************************************************************************************************************************/
void show_kvki(){
	int i;
	Point pos;
	char str[100];
	PressKey = NOKEY;
	gui_clrrect(rect_Client);
	gui_setpos(&pos, rect_Client.left+9*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char*)"KvKi参数", pos, LCD_NOREV);
	memset(str, 0, 100);
	pos.x = rect_Client.left + 3*FONTSIZE;
	pos.y += 2*FONTSIZE+2;
	sprintf(str, "总加组号    Kv    Ki");
	gui_textshow(str, pos, LCD_NOREV);
	for(i=0;i<MAXNUM_SUMGROUP;i++){
		memset(str, 0, 100);
		pos.y += 2*FONTSIZE;
//		sprintf(str, "    %d      %04d  %04d",i+1,
//				ParaAll->f25s.f25[i].MulPow_USeneor,
//				ParaAll->f25s.f25[i].MulPow_ISensor);						//FOR698
		gui_textshow(str, pos, LCD_NOREV);
	}
}

void show_kp(){

	int i;
	Point pos;
	char str[100];
	gui_clrrect(rect_Client);
	gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+2*FONTSIZE);
	gui_textshow((char*)"Kp参数", pos, LCD_NOREV);
	memset(str, 0, 100);
	pos.x = rect_Client.left + 6*FONTSIZE;
	pos.y += 3*FONTSIZE;
	sprintf(str, (char*)"脉冲路数    Kp");
//	for(i=0;i< ROADNUM_PULSE;i++){
//		memset(str, 0, 100);
//		pos.x = rect_Client.left + 9*FONTSIZE;
//		pos.y += 3*FONTSIZE;
//		sprintf(str, "%d       %04d", i, ParaAll->f11.f11[i].MeterConstant);						//FOR698
//		gui_textshow(str, pos, LCD_NOREV);
//	}

}

//Kv Ki Kp
void menu_kvkikp(){
	int pageno=0, first_flg=0;
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		switch(PressKey)
		{
		case LEFT:
		case UP:
			pageno--;
			if(pageno<0)
				pageno = 1;
			break;
		case RIGHT:
		case DOWN:
			pageno++;
			if(pageno>1)
				pageno = 0;
			break;
		case ESC:
			return;
		}
		if(PressKey!=NOKEY||first_flg==0){
			first_flg = 1;
			if(pageno==0)
				show_kvki();
			else
				show_kp();
		}
		PressKey = NOKEY;
		delay(300);
	}
}

/************************************************************************************************************************************************************
--------------------------------------------------------------参数定值-----------电能表参数----------------------------------------------------------------------
************************************************************************************************************************************************************/
void menu_fksetmeter(){
	showallmeter(setmeterpara);
}
void menu_fkaddmeter(){
	addmeter();
}
void menu_fkdelmeter(){
	showallmeter(deletemeter);
}


void menu_fkmasterip(){									//FOR698
//	char ipport[20];
//	INT8U str[6];
//	para_F3 p_f3;
//	memset(ipport, 0, 20);
//	if(msgbox_masterip(ipport, 17)==ACK){
//		memcpy(&p_f3.IP_MS[0], &ipport[0], 3);
//		p_f3.IP_MS[3] = '.';
//		memcpy(&p_f3.IP_MS[4], &ipport[3], 3);
//		p_f3.IP_MS[7] = '.';
//		memcpy(&p_f3.IP_MS[8], &ipport[6], 3);
//		p_f3.IP_MS[11] = '.';
//		memcpy(&p_f3.IP_MS[12], &ipport[9], 3);
//		memset(str, 0, 6);
//		memcpy(str, &ipport[12],4);
//		p_f3.Port_MS = atoi((char*)str);
//		setpara(&p_f3, 0, 3, sizeof(para_F3));
//	}
}

void menu_fkmasterip1(){									//FOR698
//	char ipport[20];
//	INT8U str[6];
//	para_F3 p_f3;
//	memset(ipport, 0, 20);
//	if(msgbox_masterip(ipport, 17)==ACK){
//		memcpy(&p_f3.IP1_MS[0], &ipport[0], 3);
//		p_f3.IP_MS[3] = '.';
//		memcpy(&p_f3.IP1_MS[4], &ipport[3], 3);
//		p_f3.IP_MS[7] = '.';
//		memcpy(&p_f3.IP1_MS[8], &ipport[6], 3);
//		p_f3.IP_MS[11] = '.';
//		memcpy(&p_f3.IP1_MS[12], &ipport[9], 3);
//		memset(str, 0, 6);
//		memcpy(str, &ipport[12],4);
//		p_f3.Port1_MS = atoi((char*)str);
//		setpara(&p_f3, 0, 3, sizeof(para_F3));
//	}
}
//#ifdef SPTF_III
//#endif
