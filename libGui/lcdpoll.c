/*
 * lcdpoll.c 液晶轮寻
 * */
//#include "../include/stdafx.h"
//#include "comm.h"
#include "gui.h"
#include "mutils.h"
#include "show_ctrl.h"
#include "../include/att7022e.h"
#include "../libBase/PublicFunction.h"
extern ProgramInfo* p_JProgramInfo;
//#include "att7022e.h"
#ifdef HUBEI
#include  "../lib3761/appendix.h"
#endif
#ifdef CCTT_I
#include "lcdprt_jzq.h"
#elif defined SPTF_III
#include "lcdprt_fk.h"
#else
#include "lcdprt_jzq.h"
#endif
#define MUL_OFFSET 1000
extern int read_filedata(char* FileName, int point, int did, INT8U flag, void *source);
extern void dataitem_showvalue(INT8U *filename, int cldno, char *idname, int dataid, int len, int pos_x, int pos_y);

//获得有效的数据项个数
int getjcelementcount(LcdDataItem *item)
{
	int i, count=0;
	for(i=0; i<JC_ITEM_COUNT; i++){
		if(item[i].dataflg_id==0)
			continue;
		count++;
	}
	return count;
}
//TODO:显示IP端口
void lcdpoll_showidapnip(LcdDataItem *item, int size, INT8U show_flg){
//	INT8U str[100];
//	Point pos;
//	int base_x=0.5, base_y=0.5;
//	memset(str, 0, 100);
//	gui_clrrect(rect_Client);
////显示IP
//	gui_setpos(&pos, rect_Client.left+base_x*FONTSIZE, rect_Client.top+base_y*FONTSIZE);
//	gui_textshow((char*)"主站IP:", pos, LCD_NOREV);
//	gui_setpos(&pos, rect_Client.left+(base_x+1)*FONTSIZE, rect_Client.top+(base_y+2)*FONTSIZE);
//	memset(str, 0, 100);
//	memcpy(str, ParaAll->f3.IP_MS, 16);
//	gui_textshow((char*)str, pos, LCD_NOREV);
////显示端口号
//	gui_setpos(&pos, rect_Client.left+base_x*FONTSIZE, rect_Client.top+(base_y+4)*FONTSIZE);
//	gui_textshow((char*)"端口号:", pos, LCD_NOREV);
//	gui_setpos(&pos, rect_Client.left+(base_x+8)*FONTSIZE, rect_Client.top+(base_y+4)*FONTSIZE);
//	memset(str, 0, 100);
//	sprintf((char*)str, "%d", (int)ParaAll->f3.Port_MS);
//	gui_textshow((char*)str, pos, LCD_NOREV);
////APN
//	gui_setpos(&pos, rect_Client.left+base_x*FONTSIZE, rect_Client.top+(base_y+6)*FONTSIZE);
//	gui_textshow((char*)"APN:", pos, LCD_NOREV);
//	gui_setpos(&pos, rect_Client.left+(base_x+1)*FONTSIZE, rect_Client.top+(base_y+8)*FONTSIZE);
//	memset(str, 0, 100);
//	memcpy(str, ParaAll->f3.APN, 16);
//	gui_textshow((char*)str, pos, LCD_NOREV);
////终端ID
//	gui_setpos(&pos, rect_Client.left+base_x*FONTSIZE, rect_Client.top+(base_y+10)*FONTSIZE);
//	gui_textshow((char*)"终端ID:", pos, LCD_NOREV);
//	gui_setpos(&pos, rect_Client.left+(base_x+8)*FONTSIZE, rect_Client.top+(base_y+10)*FONTSIZE);
//	memset(str, 0, 100);
//	sprintf((char*)str, "%02x%02x-%02x%02x", (int)ParaAll->f89.AreaNo[0],(int)ParaAll->f89.AreaNo[1],
//			(int)ParaAll->f89.TmnlAddr[0],(int)ParaAll->f89.TmnlAddr[1]);
//	gui_textshow((char*)str, pos, LCD_NOREV);
//	PressKey = NOKEY;
//	while(g_LcdPoll_Flag==LCD_NOTPOLL){
//		if(PressKey==ESC)
//			break;
//		PressKey = NOKEY;
//		delay(200);
//	}
//  set_time_show_flag(1);//TODO:new
}
#ifdef SPTF_III
void lcdpoll_showtotalDER(LcdDataItem *item, int size, INT8U show_flg)
{

	INT8U str[100];
	Point pos;
	int base_x=0.5, base_y=0.5;
	memset(str, 0, 100);
	gui_clrrect(rect_Client);
//显示IP
	gui_setpos(&pos, rect_Client.left+base_x*FONTSIZE, rect_Client.top+base_y*FONTSIZE);
	gui_textshow((char*)"总剩余电量:", pos, LCD_NOREV);
	gui_setpos(&pos, rect_Client.left+(base_x+1)*FONTSIZE, rect_Client.top+(base_y+2)*FONTSIZE);

	INT8U groupIndex;
	FP64 totalRer = 0.f;
	for(groupIndex = 0;groupIndex < MAXNUM_SUMGROUP;groupIndex++)
	{
		totalRer += shmm_getpubdata()->data_calc_by1min[groupIndex-1].RER;
	}
	memset(str, 0, 100);
	sprintf(str, "剩余电量:% 11.2lf kWh",totalRer);
	gui_textshow((char*)str, pos, LCD_NOREV);

	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		if(PressKey==ESC)
			break;
		PressKey = NOKEY;
		delay(200);
	}

}
#endif
int finddataitem(LcdDataItem *item, int size, int did){
	int ret_did=-1, i;
	if(size<=0)
		return -1;
	for(i=0; i<size; i++){
		if(item[i].dataflg_id==did){
			ret_did = i;
			break;
		}
	}
	return ret_did;
}
FP64 get_itemdata(LcdDataItem *item, int size, int did, int decimal){
	int item_index;
	FP64 dval=0;
	item_index = finddataitem(item, size, did);
	if(item_index>=0){
		if(item[item_index].val[0]!=0xee)
			dval = bcd2double((INT8U*)item[item_index].val, sizeof(item[item_index].val), decimal, positive);
	}else
		fprintf(stderr,"\nGUI get_itemdata-->item_index=%d",item_index);
//	fprintf(stderr,"\n did=%d   item_index=%d",did, item_index);
//	int i;
//	for(i=0; i<7; i++)
//		fprintf(stderr," %02x",item[item_index].val[i]);
//	fprintf(stderr," ==>%f",dval);
	return dval;
}

int get_itemdata1(LcdDataItem *item, int size, int did, FP64 *dval, int decimal){
	int item_index, flg=0;
	item_index = finddataitem(item, size, did);
	if(item_index>=0){
		if(item[item_index].val[0]!=0xee && item[item_index].val[0]!=0xef){
			*dval = bcd2double((INT8U*)item[item_index].val,  sizeof(item[item_index].val), decimal, positive);
			flg = 1;
		}
	}
//	fprintf(stderr,"\n did=%d   item_index=%d",did, item_index);
//	int i;
//	for(i=0; i<7; i++)
//		fprintf(stderr," %02x",item[item_index].val[i]);
//	fprintf(stderr," ====>%f",*dval);
	return flg;
}

#define OFFSET_Y 3
void ShowCLDDataPage(LcdDataItem *item, int size, INT8U show_flg)
{
	Point pos;
	TS curts;
	CLASS_4001_4002_4003 Class_id;
//	int temp1, temp2, temp3, temp4;
	INT8U str[100];
	INT8U chg_str[50];
	memset(&Class_id,0,sizeof(CLASS_4001_4002_4003));
	gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"当前正向有功电能示值", pos, LCD_NOREV);
	memset(str, 0, 100);
	FP64 dval=0;
	if(get_itemdata1(item, size, 117, &dval, 2)==1)
		sprintf((char*)str,"正向有功总% 10.2f kWh",dval);
	else
		sprintf((char*)str,"正向有功总 xxxx.xx kWh");
	gui_setpos(&pos, rect_Client.left+FONTSIZE, rect_Client.top+5*FONTSIZE);
	gui_textshow((char*)str, pos, LCD_NOREV);
	if(get_itemdata1(item, size, 118, &dval, 2)==1)
		sprintf((char*)str,"正向有功尖% 10.2f kWh",dval);
	else
		sprintf((char*)str,"正向有功尖 xxxx.xx kWh");
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	if(get_itemdata1(item, size, 119, &dval, 2)==1)
		sprintf((char*)str,"正向有功峰% 10.2f kWh",dval);
	else
		sprintf((char*)str,"正向有功峰 xxxx.xx kWh");
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	if(get_itemdata1(item, size, 120, &dval, 2)==1)
		sprintf((char*)str,"正向有功平% 10.2f kWh",dval);
	else
		sprintf((char*)str,"正向有功平 xxxx.xx kWh");
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	if(get_itemdata1(item, size, 121, &dval, 2)==1)
		sprintf((char*)str,"正向有功谷% 10.2f kWh",dval);
	else
		sprintf((char*)str,"正向有功谷 xxxx.xx kWh");
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	if(show_flg==1){
		//地址类型、逻辑地址、终端地址
		memset(chg_str,0,sizeof(chg_str));
		memset(str, 0, 100);
		readCoverClass(0x4001, 0, (void*)&Class_id, sizeof(CLASS_4001_4002_4003), para_vari_save);
		bcd2str(&Class_id.curstom_num[1],(INT8U*)chg_str,Class_id.curstom_num[0],sizeof(str),positive);
		sprintf((char*)str,"终端地址 %s",chg_str);
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}else if(show_flg==2){
		TSGet(&curts);
		memset(str, 0, 100);
		if(curts.Year==0|| curts.Month==0||curts.Day==0)
			sprintf((char*)str,"抄表时间 00/00/00 00:00");
		else
		sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
				curts.Year-2000, curts.Month, curts.Day, curts.Hour, curts.Minute);
		gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}
	return;
}

#ifdef SHANGHAI
//费率为2时，118，119为平和谷。为4时，119，121为平和谷。尖和峰直接用X表示。
void ShowCLDDataPage_SH(LcdDataItem *item, int size, INT8U show_flg,INT8U RateNum)
{
	Point pos;
	TmS curts;
	int temp1, temp2, temp3, temp4;
	INT8U str[100];
	gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"当前正向有功电能示值", pos, LCD_NOREV);
	memset(str, 0, 100);
	FP64 dval=0;
	if(get_itemdata1(item, size, 117, &dval, 2)==1)
		sprintf((char*)str,"正向有功总% 10.2f kWh",dval);
	else
		sprintf((char*)str,"正向有功总 xxxxxx.xx kWh");
	gui_setpos(&pos, rect_Client.left+FONTSIZE, rect_Client.top+5*FONTSIZE);
	gui_textshow((char*)str, pos, LCD_NOREV);
	if(RateNum == 2)
	{
		if(get_itemdata1(item, size, 118, &dval, 2)==1)
			sprintf((char*)str,"正向有功平% 10.2f kWh",dval);
		else
			sprintf((char*)str,"正向有功平 xxxxxx.xx kWh");
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
		if(get_itemdata1(item, size, 119, &dval, 2)==1)
			sprintf((char*)str,"正向有功谷% 10.2f kWh",dval);
		else
			sprintf((char*)str,"正向有功谷 xxxxxx.xx kWh");
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
	}
	else
	{
		sprintf((char*)str,"正向有功尖 xxxxxx.xx kWh");
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
		sprintf((char*)str,"正向有功峰 xxxxxx.xx kWh");
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
		if(get_itemdata1(item, size, 119, &dval, 2)==1)
			sprintf((char*)str,"正向有功平% 10.2f kWh",dval);
		else
			sprintf((char*)str,"正向有功平 xxxxxx.xx kWh");
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
		if(get_itemdata1(item, size, 121, &dval, 2)==1)
			sprintf((char*)str,"正向有功谷% 10.2f kWh",dval);
		else
			sprintf((char*)str,"正向有功谷 xxxxxx.xx kWh");
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
	}

	if(show_flg==1){
		memset(str, 0, 100);
		temp1 = ParaAll->f89.AreaNo[0];
		temp2 = ParaAll->f89.AreaNo[1];
		temp3 = ParaAll->f89.TmnlAddr[0];
		temp4 = ParaAll->f89.TmnlAddr[1];
		sprintf((char*)str,"终端地址 %02x%02x-%05d",temp1,temp2,((temp3<<8)+temp4));
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
	}else if(show_flg==2){
		tmget(&curts);
		memset(str, 0, 100);
		if(curts.Year==0|| curts.Month==0||curts.Day==0)
			sprintf((char*)str,"抄表时间 00/00/00 00:00");
		else
		sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
				curts.Year-2000, curts.Month, curts.Day, curts.Hour, curts.Minute);
		gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
	}
	return;
}
#endif

void LunXunShowPage1(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){
	Point pos;
	TS curts;
	CLASS_4001_4002_4003 Class_id;
//	int temp1, temp2, temp3, temp4;
	INT8U str[100];
	INT8U chg_str[50];
	memset(&Class_id,0,sizeof(CLASS_4001_4002_4003));
	gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+1*FONTSIZE);
	gui_textshow((char *)"当前正向有功电能示值", pos, LCD_NOREV);
	memset(str, 0, 100);
	sprintf((char*)str,"正向有功总% 10.2f kWh",jprograminfo->ACSEnergy.PosPt_All*1.0/(ENERGY_CONST*ENERGY_COEF));
	gui_setpos(&pos, rect_Client.left+FONTSIZE, rect_Client.top+5*FONTSIZE);
	gui_textshow((char*)str, pos, LCD_NOREV);
	sprintf((char*)str,"正向有功尖% 10.2f kWh",jprograminfo->ACSEnergy.PosPt_Rate[0]*1.0/(ENERGY_CONST*ENERGY_COEF));
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	sprintf((char*)str,"正向有功峰% 10.2f kWh",jprograminfo->ACSEnergy.PosPt_Rate[1]*1.0/(ENERGY_CONST*ENERGY_COEF));
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	sprintf((char*)str,"正向有功平% 10.2f kWh",jprograminfo->ACSEnergy.PosPt_Rate[2]*1.0/(ENERGY_CONST*ENERGY_COEF));
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	sprintf((char*)str,"正向有功谷% 10.2f kWh",jprograminfo->ACSEnergy.PosPt_Rate[3]*1.0/(ENERGY_CONST*ENERGY_COEF));
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	if(show_flg==1){
		//地址类型、逻辑地址、终端地址
		memset(chg_str,0,sizeof(chg_str));
		memset(str, 0, 100);
		readCoverClass(0x4001, 0, (void*)&Class_id, sizeof(CLASS_4001_4002_4003), para_vari_save);
		bcd2str(&Class_id.curstom_num[1],(INT8U*)chg_str,Class_id.curstom_num[0],sizeof(str),positive);
		sprintf((char*)str,"终端地址 %s",chg_str);
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}else if(show_flg==2){
		TSGet(&curts);
		memset(str, 0, 100);
		if(curts.Year==0|| curts.Month==0||curts.Day==0)
			sprintf((char*)str,"抄表时间 00/00/00 00:00");
		else
		sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
				curts.Year-2000, curts.Month, curts.Day, curts.Hour, curts.Minute);
		gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}
	return;
}

void LunXunShowPage2(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){
	Point pos;
	TS curts;
	CLASS_4001_4002_4003 Class_id;
//	int temp1, temp2, temp3, temp4;
	INT8U str[100];
	INT8U chg_str[50];
	memset(&Class_id,0,sizeof(CLASS_4001_4002_4003));
	gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"当前正向无功电能示值", pos, LCD_NOREV);
	memset(str, 0, 100);
	sprintf((char*)str,"正向无功总% 10.2f kVArh",jprograminfo->ACSEnergy.PosQt_All*1.0/(ENERGY_CONST*ENERGY_COEF));
	pos.x = rect_Client.left + FONTSIZE/2;
	pos.y = rect_Client.top + 5*FONTSIZE;
	gui_textshow((char*)str, pos, LCD_NOREV);
	sprintf((char*)str,"正向无功尖% 10.2f kVArh",jprograminfo->ACSEnergy.PosQt_Rate[0]*1.0/(ENERGY_CONST*ENERGY_COEF));
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	sprintf((char*)str,"正向无功峰% 10.2f kVArh",jprograminfo->ACSEnergy.PosQt_Rate[1]*1.0/(ENERGY_CONST*ENERGY_COEF));
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	sprintf((char*)str,"正向无功平% 10.2f kVArh",jprograminfo->ACSEnergy.PosQt_Rate[2]*1.0/(ENERGY_CONST*ENERGY_COEF));
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	sprintf((char*)str,"正向无功谷% 10.2f kVArh",jprograminfo->ACSEnergy.PosQt_Rate[3]*1.0/(ENERGY_CONST*ENERGY_COEF));
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	if(show_flg==1){
		//地址类型、逻辑地址、终端地址
		memset(chg_str,0,sizeof(chg_str));
		memset(str, 0, 100);
		readCoverClass(0x4001, 0, (void*)&Class_id, sizeof(CLASS_4001_4002_4003), para_vari_save);
		bcd2str(&Class_id.curstom_num[1],(INT8U*)chg_str,Class_id.curstom_num[0],sizeof(str),positive);
		sprintf((char*)str,"终端地址 %s",chg_str);
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}
	else if(show_flg==2){
		TSGet(&curts);
		memset(str, 0, 100);
		if(curts.Year==0|| curts.Month==0||curts.Day==0)
			sprintf((char*)str,"抄表时间 00/00/00 00:00");
		else
		sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
				curts.Year-2000, curts.Month, curts.Day, curts.Hour, curts.Minute);
		gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}
	return;
}
void LunXunShowPage3(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){

	Point pos;
	TS curts;
	CLASS_4001_4002_4003 Class_id;
//	int temp1, temp2, temp3, temp4;
	INT8U str[100];
	INT8U chg_str[50];
	memset(&Class_id,0,sizeof(CLASS_4001_4002_4003));
	bzero(str,100);
	gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"当前电压", pos, LCD_NOREV);
	memset(str, 0, 100);
	if(jprograminfo->dev_info.WireType != 0x1200 && jprograminfo->dev_info.WireType != 0x0600)//0x1200 三相三线接法 0x0600 三相四线接法
	{
		bzero(str,100);
		sprintf((char*)str,"未知的接线方式");
		gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
		return;
	}
	bzero(str,100);
	if(jprograminfo->dev_info.WireType == 0x0600)
	{
#ifdef JIANGSU
		sprintf((char*)str,"A相电压% 9.1f V",jprograminfo->ACSRealData.Ua*1.0/U_COEF);
#else
		sprintf((char*)str,"A相电压% 9.3f V",jprograminfo->ACSRealData.Ua*1.0/U_COEF);
#endif
		gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
	}
	else if(jprograminfo->dev_info.WireType == 0x1200)
	{
#ifdef JIANGSU
		sprintf((char*)str,"    Uab% 9.1f V",jprograminfo->ACSRealData.Ua*1.0/U_COEF);
#else
		sprintf((char*)str,"    Uab% 9.3f V",jprograminfo->ACSRealData.Ua*1.0/U_COEF);
#endif
		gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
	}

	if(show_flg==1){
		//地址类型、逻辑地址、终端地址
		memset(chg_str,0,sizeof(chg_str));
		memset(str, 0, 100);
		readCoverClass(0x4001, 0, (void*)&Class_id, sizeof(CLASS_4001_4002_4003), para_vari_save);
		bcd2str(&Class_id.curstom_num[1],(INT8U*)chg_str,Class_id.curstom_num[0],sizeof(str),positive);
		sprintf((char*)str,"终端地址 %s",chg_str);
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}else if(show_flg==2){
		TSGet(&curts);
		memset(str, 0, 100);
		if(curts.Year==0|| curts.Month==0||curts.Day==0)
			sprintf((char*)str,"抄表时间 00/00/00 00:00");
		else
		sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
				curts.Year-2000, curts.Month, curts.Day, curts.Hour, curts.Minute);
		gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}
	return;
}
void LunXunShowPage4(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){

	Point pos;
	TS curts;
	CLASS_4001_4002_4003 Class_id;
//	int temp1, temp2, temp3, temp4;
	INT8U str[100];
	INT8U chg_str[50];
	memset(&Class_id,0,sizeof(CLASS_4001_4002_4003));
	bzero(str,100);
	gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"当前电压", pos, LCD_NOREV);
	memset(str, 0, 100);
	//fprintf(stderr,"接线方式：%02x",shmm_getdevstat()->WireType);
	if(jprograminfo->dev_info.WireType != 0x1200 && jprograminfo->dev_info.WireType != 0x0600)
	{
		bzero(str,100);
		sprintf((char*)str,"未知的接线方式");
		gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
		return;
	}
	memset(str, 0, 100);
	if(jprograminfo->dev_info.WireType == 0x0600)
	{
#ifdef JIANGSU
		sprintf((char*)str,"B相电压% 9.1f V",jprograminfo->ACSRealData.Ub*1.0/U_COEF);
#else
		sprintf((char*)str,"B相电压% 9.3f V",jprograminfo->ACSRealData.Ub*1.0/U_COEF);
#endif
		gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
	}
	else if(jprograminfo->dev_info.WireType == 0x1200)
	{
#ifdef JIANGSU
		sprintf((char*)str,"    Ubc% 9.1f V",jprograminfo->ACSRealData.Uc*1.0/U_COEF);
#else
		sprintf((char*)str,"    Ubc% 9.3f V",jprograminfo->ACSRealData.Uc*1.0/U_COEF);
#endif
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
	}
	if(show_flg==1){
		//地址类型、逻辑地址、终端地址
		memset(chg_str,0,sizeof(chg_str));
		memset(str, 0, 100);
		readCoverClass(0x4001, 0, (void*)&Class_id, sizeof(CLASS_4001_4002_4003), para_vari_save);
		bcd2str(&Class_id.curstom_num[1],(INT8U*)chg_str,Class_id.curstom_num[0],sizeof(str),positive);
		sprintf((char*)str,"终端地址 %s",chg_str);
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}else if(show_flg==2){
		TSGet(&curts);
		memset(str, 0, 100);
		if(curts.Year==0|| curts.Month==0||curts.Day==0)
			sprintf((char*)str,"抄表时间 00/00/00 00:00");
		else
		sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
				curts.Year-2000, curts.Month, curts.Day, curts.Hour, curts.Minute);
		gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}
	return;
}
void LunXunShowPage5(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){

	Point pos;
	TS curts;
	CLASS_4001_4002_4003 Class_id;
//	int temp1, temp2, temp3, temp4;
	INT8U str[100];
	INT8U chg_str[50];
	memset(&Class_id,0,sizeof(CLASS_4001_4002_4003));
	bzero(str,100);
	gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"当前电压", pos, LCD_NOREV);
	memset(str, 0, 100);
	//fprintf(stderr,"接线方式：%02x",shmm_getdevstat()->WireType);
	if(jprograminfo->dev_info.WireType != 0x1200 && jprograminfo->dev_info.WireType != 0x0600)
	{
		bzero(str,100);
		sprintf((char*)str,"未知的接线方式");
		gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
		return;
	}
	memset(str, 0, 100);
	if(jprograminfo->dev_info.WireType == 0x0600)
	{
#ifdef JIANGSU
		sprintf((char*)str,"C相电压% 9.1f V",jprograminfo->ACSRealData.Uc*1.0/U_COEF);
#else
		sprintf((char*)str,"C相电压% 9.3f V",jprograminfo->ACSRealData.Uc*1.0/U_COEF);
#endif
		gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
	}
	if(show_flg==1){
		//地址类型、逻辑地址、终端地址
		memset(chg_str,0,sizeof(chg_str));
		memset(str, 0, 100);
		readCoverClass(0x4001, 0, (void*)&Class_id, sizeof(CLASS_4001_4002_4003), para_vari_save);
		bcd2str(&Class_id.curstom_num[1],(INT8U*)chg_str,Class_id.curstom_num[0],sizeof(str),positive);
		sprintf((char*)str,"终端地址 %s",chg_str);
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}else if(show_flg==2){
		TSGet(&curts);
		memset(str, 0, 100);
		if(curts.Year==0|| curts.Month==0||curts.Day==0)
			sprintf((char*)str,"抄表时间 00/00/00 00:00");
		else
		sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
				curts.Year-2000, curts.Month, curts.Day, curts.Hour, curts.Minute);
		gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}
	return;
}
void LunXunShowPage6(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){

	Point pos;
	TS curts;
	CLASS_4001_4002_4003 Class_id;
//	int temp1, temp2, temp3, temp4;
	INT8U str[100];
	INT8U chg_str[50];
	memset(&Class_id,0,sizeof(CLASS_4001_4002_4003));
	gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"当前电流", pos, LCD_NOREV);
	bzero(str,100);
	//fprintf(stderr,"接线方式：%02x",shmm_getdevstat()->WireType);
	if(jprograminfo->dev_info.WireType != 0x1200 && jprograminfo->dev_info.WireType != 0x0600)
	{
		bzero(str,100);
		sprintf((char*)str,"未知的接线方式");
		gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
		return;
	}
	bzero(str,100);

	sprintf((char*)str,"A相电流% 9.3f A",jprograminfo->ACSRealData.Ia*1.0/I_COEF);
	gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
	gui_textshow((char*)str, pos, LCD_NOREV);
	if(show_flg==1){
		//地址类型、逻辑地址、终端地址
		memset(chg_str,0,sizeof(chg_str));
		memset(str, 0, 100);
		readCoverClass(0x4001, 0, (void*)&Class_id, sizeof(CLASS_4001_4002_4003), para_vari_save);
		bcd2str(&Class_id.curstom_num[1],(INT8U*)chg_str,Class_id.curstom_num[0],sizeof(str),positive);
		sprintf((char*)str,"终端地址 %s",chg_str);
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}else if(show_flg==2){
		TSGet(&curts);
		memset(str, 0, 100);
		if(curts.Year==0|| curts.Month==0||curts.Day==0)
			sprintf((char*)str,"抄表时间 00/00/00 00:00");
		else
		sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
				curts.Year-2000, curts.Month, curts.Day, curts.Hour, curts.Minute);
		gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}
	return;
}
void LunXunShowPage7(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){

	Point pos;
	TS curts;
	CLASS_4001_4002_4003 Class_id;
//	int temp1, temp2, temp3, temp4;
	INT8U str[100];
	INT8U chg_str[50];
	memset(&Class_id,0,sizeof(CLASS_4001_4002_4003));
	gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"当前电流", pos, LCD_NOREV);
	bzero(str,100);
	//fprintf(stderr,"接线方式：%02x",shmm_getdevstat()->WireType);
	if(jprograminfo->dev_info.WireType != 0x1200 && jprograminfo->dev_info.WireType != 0x0600)
	{
		bzero(str,100);
		sprintf((char*)str,"未知的接线方式");
		gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
		return;
	}
	bzero(str,100);
	if(jprograminfo->dev_info.WireType == 0x0600)
	{
		sprintf((char*)str,"B相电流% 9.3f A",jprograminfo->ACSRealData.Ib*1.0/I_COEF);
		gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
	}
	if(show_flg==1){
		//地址类型、逻辑地址、终端地址
		memset(chg_str,0,sizeof(chg_str));
		memset(str, 0, 100);
		readCoverClass(0x4001, 0, (void*)&Class_id, sizeof(CLASS_4001_4002_4003), para_vari_save);
		bcd2str(&Class_id.curstom_num[1],(INT8U*)chg_str,Class_id.curstom_num[0],sizeof(str),positive);
		sprintf((char*)str,"终端地址 %s",chg_str);
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}else if(show_flg==2){
		TSGet(&curts);
		memset(str, 0, 100);
		if(curts.Year==0|| curts.Month==0||curts.Day==0)
			sprintf((char*)str,"抄表时间 00/00/00 00:00");
		else
		sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
				curts.Year-2000, curts.Month, curts.Day, curts.Hour, curts.Minute);
		gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}
	return;
}
void LunXunShowPage8(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){

	Point pos;
	TS curts;
	CLASS_4001_4002_4003 Class_id;
//	int temp1, temp2, temp3, temp4;
	INT8U str[100];
	INT8U chg_str[50];
	memset(&Class_id,0,sizeof(CLASS_4001_4002_4003));
	gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"当前电流", pos, LCD_NOREV);
	bzero(str,100);
	//fprintf(stderr,"接线方式：%02x",shmm_getdevstat()->WireType);
	if(jprograminfo->dev_info.WireType != 0x1200 && jprograminfo->dev_info.WireType != 0x0600)
	{
		bzero(str,100);
		sprintf((char*)str,"未知的接线方式");
		gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
		return;
	}
	memset(str, 0, 100);
	sprintf((char*)str,"C相电流% 9.3f A",jprograminfo->ACSRealData.Ic*1.0/I_COEF);
	gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);;
	gui_textshow((char*)str, pos, LCD_NOREV);
	if(show_flg==1){
		//地址类型、逻辑地址、终端地址
		memset(chg_str,0,sizeof(chg_str));
		memset(str, 0, 100);
		readCoverClass(0x4001, 0, (void*)&Class_id, sizeof(CLASS_4001_4002_4003), para_vari_save);
		bcd2str(&Class_id.curstom_num[1],(INT8U*)chg_str,Class_id.curstom_num[0],sizeof(str),positive);
		sprintf((char*)str,"终端地址 %s",chg_str);
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}else if(show_flg==2){
		TSGet(&curts);
		memset(str, 0, 100);
		if(curts.Year==0|| curts.Month==0||curts.Day==0)
			sprintf((char*)str,"抄表时间 00/00/00 00:00");
		else
		sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
				curts.Year-2000, curts.Month, curts.Day, curts.Hour, curts.Minute);
		gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}
	return;
}
void LunXunShowPage9(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){

	Point pos;
	TS curts;
	CLASS_4001_4002_4003 Class_id;
//	int temp1, temp2, temp3, temp4;
	INT8U str[100];
	INT8U chg_str[50];
	memset(&Class_id,0,sizeof(CLASS_4001_4002_4003));
	bzero(str,100);
	gui_setpos(&pos, rect_Client.left+7*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"当前有功功率", pos, LCD_NOREV);
	memset(str, 0, 100);
	sprintf((char*)str,"总 % 15.4f kW",jprograminfo->ACSRealData.Pt*1.0/(P_COEF*1000));
	gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+5*FONTSIZE);
	gui_textshow((char*)str, pos, LCD_NOREV);
	//fprintf(stderr,"接线方式：%02x",shmm_getdevstat()->WireType);
	if(jprograminfo->dev_info.WireType != 0x1200 && jprograminfo->dev_info.WireType != 0x0600)
	{
		bzero(str,100);
		sprintf((char*)str,"未知的接线方式");
		gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
		return;
	}
	bzero(str,100);
	sprintf((char*)str,"A相%15.4f kW",jprograminfo->ACSRealData.Pa*1.0/(P_COEF*1000));
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	if(show_flg==1){
		//地址类型、逻辑地址、终端地址
		memset(chg_str,0,sizeof(chg_str));
		memset(str, 0, 100);
		readCoverClass(0x4001, 0, (void*)&Class_id, sizeof(CLASS_4001_4002_4003), para_vari_save);
		bcd2str(&Class_id.curstom_num[1],(INT8U*)chg_str,Class_id.curstom_num[0],sizeof(str),positive);
		sprintf((char*)str,"终端地址 %s",chg_str);
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}else if(show_flg==2){
		TSGet(&curts);
		memset(str, 0, 100);
		if(curts.Year==0|| curts.Month==0||curts.Day==0)
			sprintf((char*)str,"抄表时间 00/00/00 00:00");
		else
		sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
				curts.Year-2000, curts.Month, curts.Day, curts.Hour, curts.Minute);
		gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}
	return;
}
void LunXunShowPage10(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){

	Point pos;
	TS curts;
	CLASS_4001_4002_4003 Class_id;
//	int temp1, temp2, temp3, temp4;
	INT8U str[100];
	INT8U chg_str[50];
	memset(&Class_id,0,sizeof(CLASS_4001_4002_4003));
	bzero(str,100);
	gui_setpos(&pos, rect_Client.left+7*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"当前有功功率", pos, LCD_NOREV);
	memset(str, 0, 100);
	sprintf((char*)str,"总 % 15.4f kW",jprograminfo->ACSRealData.Pt*1.0/(P_COEF*1000));
	gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+5*FONTSIZE);
	gui_textshow((char*)str, pos, LCD_NOREV);
	//fprintf(stderr,"接线方式：%02x",shmm_getdevstat()->WireType);
	if(jprograminfo->dev_info.WireType != 0x1200 && jprograminfo->dev_info.WireType != 0x0600)
	{
		bzero(str,100);
		sprintf((char*)str,"未知的接线方式");
		gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
		return;
	}
	memset(str, 0, 100);
	if(jprograminfo->dev_info.WireType == 0x0600)
	{
		sprintf((char*)str,"B相%15.4f kW",jprograminfo->ACSRealData.Pb*1.0/(P_COEF*1000));
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);;
	}
	if(show_flg==1){
		//地址类型、逻辑地址、终端地址
		memset(chg_str,0,sizeof(chg_str));
		memset(str, 0, 100);
		readCoverClass(0x4001, 0, (void*)&Class_id, sizeof(CLASS_4001_4002_4003), para_vari_save);
		bcd2str(&Class_id.curstom_num[1],(INT8U*)chg_str,Class_id.curstom_num[0],sizeof(str),positive);
		sprintf((char*)str,"终端地址 %s",chg_str);
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}else if(show_flg==2){
		TSGet(&curts);
		memset(str, 0, 100);
		if(curts.Year==0|| curts.Month==0||curts.Day==0)
			sprintf((char*)str,"抄表时间 00/00/00 00:00");
		else
		sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
				curts.Year-2000, curts.Month, curts.Day, curts.Hour, curts.Minute);
		gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}
	return;
}
void LunXunShowPage11(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){

	Point pos;
	TS curts;
	CLASS_4001_4002_4003 Class_id;
//	int temp1, temp2, temp3, temp4;
	INT8U str[100];
	INT8U chg_str[50];
	memset(&Class_id,0,sizeof(CLASS_4001_4002_4003));
	bzero(str,100);
	gui_setpos(&pos, rect_Client.left+7*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"当前有功功率", pos, LCD_NOREV);
	memset(str, 0, 100);
	sprintf((char*)str,"总 % 15.4f kW",jprograminfo->ACSRealData.Pt*1.0/(P_COEF*1000));
	gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+5*FONTSIZE);
	gui_textshow((char*)str, pos, LCD_NOREV);
	//fprintf(stderr,"接线方式：%02x",shmm_getdevstat()->WireType);
	if(jprograminfo->dev_info.WireType != 0x1200 && jprograminfo->dev_info.WireType != 0x0600)
	{
		bzero(str,100);
		sprintf((char*)str,"未知的接线方式");
		gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
		return;
	}
	memset(str, 0, 100);
	sprintf((char*)str,"C相%15.4f kW",jprograminfo->ACSRealData.Pc*1.0/(P_COEF*1000));
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	if(show_flg==1){
		//地址类型、逻辑地址、终端地址
		memset(chg_str,0,sizeof(chg_str));
		memset(str, 0, 100);
		readCoverClass(0x4001, 0, (void*)&Class_id, sizeof(CLASS_4001_4002_4003), para_vari_save);
		bcd2str(&Class_id.curstom_num[1],(INT8U*)chg_str,Class_id.curstom_num[0],sizeof(str),positive);
		sprintf((char*)str,"终端地址 %s",chg_str);
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}else if(show_flg==2){
		TSGet(&curts);
		memset(str, 0, 100);
		if(curts.Year==0|| curts.Month==0||curts.Day==0)
			sprintf((char*)str,"抄表时间 00/00/00 00:00");
		else
		sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
				curts.Year-2000, curts.Month, curts.Day, curts.Hour, curts.Minute);
		gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		set_time_show_flag(1);
	}
	return;
}

void ShowJCData(ProgramInfo* p_JProgramInfo,int PageNo, LcdDataItem *item, int size, int show_flg)
{
	fprintf(stderr,"\n^^^^gui:JCpage==%d^^^^^\n",PageNo);
	switch(PageNo)
	{
	case 0:
		LunXunShowPage1(p_JProgramInfo,item, size, show_flg);//当前正向有功电能示值
		break;
	case 1:
		LunXunShowPage2(p_JProgramInfo,item, size, show_flg);//当前正向无功电能示值
		break;
	case 2:
		LunXunShowPage3(p_JProgramInfo,item, size, show_flg);//当前反向有功电能示值
		break;
	case 3:
		LunXunShowPage4(p_JProgramInfo,item, size, show_flg);//当前反向无功电能示值
		break;
	case 4:
		LunXunShowPage5(p_JProgramInfo,item, size, show_flg);//一象限无功电能示值
		break;
	case 5:
		LunXunShowPage6(p_JProgramInfo,item, size, show_flg);//二象限无功电能示值
		break;
	case 6:
		LunXunShowPage7(p_JProgramInfo,item, size, show_flg);//三象限无功电能示值
		break;
	case 7:
		LunXunShowPage8(p_JProgramInfo,item, size, show_flg);//四象限无功电能示值
		break;
	case 8:
		LunXunShowPage9(p_JProgramInfo,item, size, show_flg);//视在功率
		break;
	case 9:
		LunXunShowPage10(p_JProgramInfo,item, size, show_flg);//当前电压
		break;
	case 10:
		LunXunShowPage11(p_JProgramInfo,item, size, show_flg);//当前电流
		break;
	default:
		break;
	}
}

LunXian_t LunXian[LunPageNum]={
		{"当前有功总电能量示值", 	LunXunShowPage1, 			1},//0
		{"当前无功总电能量示值", 	LunXunShowPage2, 			1},//1
		{"A相电压",	LunXunShowPage3, 			1},//2
		{"B相电压", 	LunXunShowPage4, 			1},//3
		{"C相电压", 	LunXunShowPage5, 	1},//4
		{"A相电流", 	LunXunShowPage6, 	1},//5
		{"B相电流", 	LunXunShowPage7, 	1},//6
		{"C相电流", 	LunXunShowPage8, 	1},//7
		{"A相有功功率", 		LunXunShowPage9, 			1},//8
		{"B相有功功率", 		LunXunShowPage10, 			1},//9
		{"C相有功功率", 		LunXunShowPage11, 			1},//10
};



void initlunxian(){//yd
	int  i;
	INT8U lcd_lunxian_flg[4];
	memset(lcd_lunxian_flg, 0, 4);
	char ssetup[10];
	memset(ssetup, 0, 10);
	if(readcfg((char*)"/nor/config/lcd.cfg", (char*)"LCD_LUNXIAN_SETUP", ssetup)==0)
		memset(lcd_lunxian_flg, 0xff, 4);
	else
		asc2bcd((INT8U*)ssetup, 8, (INT8U*)lcd_lunxian_flg, positive);
	for(i=0; i<LunPageNum; i++)
		LunXian[i].isshow_flg = ((lcd_lunxian_flg[i/8]>>(i%8))&0x01);
}

//轮显交采数据
void lcdpoll_show(ProgramInfo* jprograminfo)
{
	int i;
	static int pageno=0;
	time_t curtime, oldtime;
	Point page_pos;
	LcdDataItem item[JC_ITEM_COUNT];//存储的所有数据项
	memset(&page_pos, 0, sizeof(Point));
	oldtime = curtime = time(NULL);
	for(i=pageno; i<LunPageNum; i++){
		if(LunXian[i].isshow_flg==0)
			continue;
		else{
			gui_clrrect(rect_Client);
			LunXian[i].pfun(jprograminfo,item, JC_ITEM_COUNT, 1);
			pageno = i;
			while(g_LcdPoll_Flag==LCD_INPOLL){
				curtime = time(NULL);
				if(abs(curtime-oldtime)>=LCDPOLL_INTERNAL)
					break;
				delay(1000);
			}
			break;
		}
	}
	pageno = (pageno+1)%LunPageNum;
}
