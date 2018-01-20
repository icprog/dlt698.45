/*
 * lcdpoll.c 液晶轮寻
 * */
//#include "../include/stdafx.h"
#include "basedef.h"
#include "gui.h"
#include "mutils.h"
#include "show_ctrl.h"
#include "../include/att7022e.h"
#include "../libBase/PublicFunction.h"

//#ifdef CCTT_I
#include "lcdprt_jzq.h"
//#else
//#include "lcdprt_fk.h"
//#endif

#define MUL_OFFSET 1000

extern ProgramInfo* p_JProgramInfo;
extern INT8U g_chgOI4001 ;
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
//	INT8U str[100] = {0};
//	Point pos = {0};
//	int base_x=0.5, base_y=0.5;
//
//	gui_clrrect(rect_Client);
////显示IP
//	gui_setpos(&pos, rect_Client.left+base_x*FONTSIZE, rect_Client.top+base_y*FONTSIZE);
//	gui_textshow((char*)"主站IP:", pos, LCD_NOREV);
//	gui_setpos(&pos, rect_Client.left+(base_x+1)*FONTSIZE, rect_Client.top+(base_y+2)*FONTSIZE);
//
//	memcpy(str, ParaAll->f3.IP_MS, 16);
//	gui_textshow((char*)str, pos, LCD_NOREV);
////显示端口号
//	gui_setpos(&pos, rect_Client.left+base_x*FONTSIZE, rect_Client.top+(base_y+4)*FONTSIZE);
//	gui_textshow((char*)"端口号:", pos, LCD_NOREV);
//	gui_setpos(&pos, rect_Client.left+(base_x+8)*FONTSIZE, rect_Client.top+(base_y+4)*FONTSIZE);
//
//	sprintf((char*)str, "%d", (int)ParaAll->f3.Port_MS);
//	gui_textshow((char*)str, pos, LCD_NOREV);
////APN
//	gui_setpos(&pos, rect_Client.left+base_x*FONTSIZE, rect_Client.top+(base_y+6)*FONTSIZE);
//	gui_textshow((char*)"APN:", pos, LCD_NOREV);
//	gui_setpos(&pos, rect_Client.left+(base_x+1)*FONTSIZE, rect_Client.top+(base_y+8)*FONTSIZE);
//
//	memcpy(str, ParaAll->f3.APN, 16);
//	gui_textshow((char*)str, pos, LCD_NOREV);
////终端ID
//	gui_setpos(&pos, rect_Client.left+base_x*FONTSIZE, rect_Client.top+(base_y+10)*FONTSIZE);
//	gui_textshow((char*)"终端ID:", pos, LCD_NOREV);
//	gui_setpos(&pos, rect_Client.left+(base_x+8)*FONTSIZE, rect_Client.top+(base_y+10)*FONTSIZE);
//
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
}



void lcdpoll_showtotalDER(LcdDataItem *item, int size, INT8U show_flg)
{

	INT8U str[100] = {0};
	Point pos = {0};
	int base_x=0.5, base_y=0.5;

	INT8U groupIndex = 0;
	FP64 totalRer = 0.f;


	if (NULL == p_JProgramInfo)
		return;

	if (SPTF3 != p_JProgramInfo->cfg_para.device)
		return;

	gui_clrrect(rect_Client);
//显示IP
	gui_setpos(&pos, rect_Client.left+base_x*FONTSIZE, rect_Client.top+base_y*FONTSIZE);
	gui_textshow((char*)"总剩余电量:", pos, LCD_NOREV);
	gui_setpos(&pos, rect_Client.left+(base_x+1)*FONTSIZE, rect_Client.top+(base_y+2)*FONTSIZE);


	//TODO: 添加总加组剩余电量
	for(groupIndex = 0;groupIndex < MAXNUM_SUMGROUP;groupIndex++)
	{
//		totalRer += shmm_getpubdata()->data_calc_by1min[groupIndex-1].RER;	// FOR698
	}

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
//#ifdef SPTF_III
//#endif

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
	INT8U i = 0,len = 0;
	union{
		INT32U vval;
		INT8U vval_bin[4];
	}vval_int32;
	vval_int32.vval = 0;
	len = sizeof(vval_int32.vval_bin);
	item_index = finddataitem(item, size, did);
	if(item_index>=0){
		if(item[item_index].val[0]!=0xee && item[item_index].val[0]!=0xef){
			for(i=0;i < len;i++){
				vval_int32.vval_bin[len-i-1] = item[item_index].val[i];
			}
			*dval = vval_int32.vval%100*0.01+vval_int32.vval/100;
//			*dval = bcd2double((INT8U*)item[item_index].val,  sizeof(item[item_index].val), decimal, positive);
			flg = 1;
		}
	}
	return flg;
}

#define OFFSET_Y 3
void ShowCLDDataPage(LcdDataItem *item, int size, INT8U show_flg)
{
	Point pos = {0};
	TS curts = {0};
	INT8U str[100] = {0};
	INT8U chg_str[50] = {0};

	gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"当前正向有功电能示值", pos, LCD_NOREV);

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
		if (p_JProgramInfo->oi_changed.oi4001 != g_chgOI4001) {
			g_chgOI4001 = p_JProgramInfo->oi_changed.oi4001;
			readCoverClass(0x4001, 0, (void*)&g_Class4001_4002_4003, sizeof(CLASS_4001_4002_4003), para_vari_save);
		}

		bcd2str(&g_Class4001_4002_4003.curstom_num[1],(INT8U*)chg_str,g_Class4001_4002_4003.curstom_num[0],sizeof(str),positive);
		sprintf((char*)str,"终端地址 %s",chg_str);
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
	}else if(show_flg==2){
		TSGet(&curts);

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

void LunXunShowPage1(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){
	Point pos = {0};
	TS curts = {0};
	INT8U str[100] = {0};
	INT8U chg_str[50] = {0};

	gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+1*FONTSIZE);
	gui_textshow((char *)"当前正向有功电能示值", pos, LCD_NOREV);

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
		if (p_JProgramInfo->oi_changed.oi4001 != g_chgOI4001) {
			g_chgOI4001 = p_JProgramInfo->oi_changed.oi4001;
			readCoverClass(0x4001, 0, (void*)&g_Class4001_4002_4003, sizeof(CLASS_4001_4002_4003), para_vari_save);
		}

		bcd2str(&g_Class4001_4002_4003.curstom_num[1],(INT8U*)chg_str,g_Class4001_4002_4003.curstom_num[0],sizeof(str),positive);
		sprintf((char*)str,"终端地址 %s",chg_str);
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
	}else if(show_flg==2){
		TSGet(&curts);

		if(curts.Year==0|| curts.Month==0||curts.Day==0)
			sprintf((char*)str,"抄表时间 00/00/00 00:00");
		else
			sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
				curts.Year-2000, curts.Month, curts.Day, \
				curts.Hour, curts.Minute);
		gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
	}
	return;
}

void LunXunShowPage2(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){
	Point pos = {0};
	TS curts = {0};
	INT8U str[100] = {0};
	INT8U chg_str[50] = {0};

	gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"当前正向无功电能示值", pos, LCD_NOREV);

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


		if (p_JProgramInfo->oi_changed.oi4001 != g_chgOI4001) {
			g_chgOI4001 = p_JProgramInfo->oi_changed.oi4001;
			readCoverClass(0x4001, 0, (void*)&g_Class4001_4002_4003,sizeof(CLASS_4001_4002_4003), para_vari_save);
		}

		bcd2str(&g_Class4001_4002_4003.curstom_num[1],(INT8U*)chg_str,g_Class4001_4002_4003.curstom_num[0],sizeof(str),positive);
		sprintf((char*)str,"终端地址 %s",chg_str);
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
	}
	else if(show_flg==2){
		TSGet(&curts);

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
void LunXunShowPage3(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){
	Point pos = {0};
	TS curts = {0};
	INT8U str[100] = {0};
	INT8U chg_str[50] = {0};

	gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"当前反向有功电能示值", pos, LCD_NOREV);

	sprintf((char*)str,"反向有功总% 10.2f kWh",jprograminfo->ACSEnergy.NegPt_All*1.0/(ENERGY_CONST*ENERGY_COEF));
	pos.x = rect_Client.left + FONTSIZE/2;
	pos.y = rect_Client.top + 5*FONTSIZE;
	gui_textshow((char*)str, pos, LCD_NOREV);
	sprintf((char*)str,"反向有功尖% 10.2f kWh",jprograminfo->ACSEnergy.NegPt_Rate[0]*1.0/(ENERGY_CONST*ENERGY_COEF));
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	sprintf((char*)str,"反向有功峰% 10.2f kWh",jprograminfo->ACSEnergy.NegPt_Rate[1]*1.0/(ENERGY_CONST*ENERGY_COEF));
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	sprintf((char*)str,"反向有功平% 10.2f kWh",jprograminfo->ACSEnergy.NegPt_Rate[2]*1.0/(ENERGY_CONST*ENERGY_COEF));
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	sprintf((char*)str,"反向有功谷% 10.2f kWh",jprograminfo->ACSEnergy.NegPt_Rate[3]*1.0/(ENERGY_CONST*ENERGY_COEF));
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	if(show_flg==1){
		//地址类型、逻辑地址、终端地址
		if (p_JProgramInfo->oi_changed.oi4001 != g_chgOI4001) {
			g_chgOI4001 = p_JProgramInfo->oi_changed.oi4001;
			readCoverClass(0x4001, 0, (void*)&g_Class4001_4002_4003,sizeof(CLASS_4001_4002_4003), para_vari_save);
		}

		bcd2str(&g_Class4001_4002_4003.curstom_num[1],(INT8U*)chg_str,g_Class4001_4002_4003.curstom_num[0],sizeof(str),positive);
		sprintf((char*)str,"终端地址 %s",chg_str);
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
	}
	else if(show_flg==2){
		TSGet(&curts);

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
void LunXunShowPage4(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){
	Point pos = {0};
	TS curts = {0};
	INT8U str[100] = {0};
	INT8U chg_str[50] = {0};

	gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"当前反向无功电能示值", pos, LCD_NOREV);

	sprintf((char*)str,"反向无功总% 10.2f kVArh",jprograminfo->ACSEnergy.NegQt_All*1.0/(ENERGY_CONST*ENERGY_COEF));
	pos.x = rect_Client.left + FONTSIZE/2;
	pos.y = rect_Client.top + 5*FONTSIZE;
	gui_textshow((char*)str, pos, LCD_NOREV);
	sprintf((char*)str,"反向无功尖% 10.2f kVArh",jprograminfo->ACSEnergy.NegQt_Rate[0]*1.0/(ENERGY_CONST*ENERGY_COEF));
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	sprintf((char*)str,"反向无功峰% 10.2f kVArh",jprograminfo->ACSEnergy.NegQt_Rate[1]*1.0/(ENERGY_CONST*ENERGY_COEF));
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	sprintf((char*)str,"反向无功平% 10.2f kVArh",jprograminfo->ACSEnergy.NegQt_Rate[2]*1.0/(ENERGY_CONST*ENERGY_COEF));
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	sprintf((char*)str,"反向无功谷% 10.2f kVArh",jprograminfo->ACSEnergy.NegQt_Rate[3]*1.0/(ENERGY_CONST*ENERGY_COEF));
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	if(show_flg==1){
		//地址类型、逻辑地址、终端地址
		if (p_JProgramInfo->oi_changed.oi4001 != g_chgOI4001) {
			g_chgOI4001 = p_JProgramInfo->oi_changed.oi4001;
			readCoverClass(0x4001, 0, (void*)&g_Class4001_4002_4003,sizeof(CLASS_4001_4002_4003), para_vari_save);
		}

		bcd2str(&g_Class4001_4002_4003.curstom_num[1],(INT8U*)chg_str,g_Class4001_4002_4003.curstom_num[0],sizeof(str),positive);
		sprintf((char*)str,"终端地址 %s",chg_str);
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
	}
	else if(show_flg==2){
		TSGet(&curts);

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
void LunXunShowPage_X1_Q_All(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){
		Point pos = {0};
		TS curts = {0};
		INT8U str[100] = {0};
		INT8U chg_str[50] = {0};

		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+FONTSIZE);
		gui_textshow((char *)"一象限无功电能示值", pos, LCD_NOREV);
		memset(str, 0, 100);

		sprintf((char*)str,"总% 14.2f kVArh",jprograminfo->ACSEnergy.Q1_Qt_All*1.0/(ENERGY_CONST*ENERGY_COEF));
		pos.x = rect_Client.left + FONTSIZE/2;
		pos.y = rect_Client.top + 5*FONTSIZE;
		gui_textshow((char*)str, pos, LCD_NOREV);
		sprintf((char*)str,"尖% 14.2f kVArh",jprograminfo->ACSEnergy.Q1_Qt_Rate[0]*1.0/(ENERGY_CONST*ENERGY_COEF));
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
		sprintf((char*)str,"峰% 14.2f kVArh",jprograminfo->ACSEnergy.Q1_Qt_Rate[1]*1.0/(ENERGY_CONST*ENERGY_COEF));
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
		sprintf((char*)str,"平% 14.2f kVArh",jprograminfo->ACSEnergy.Q1_Qt_Rate[2]*1.0/(ENERGY_CONST*ENERGY_COEF));
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
		sprintf((char*)str,"谷% 14.2f kVArh",jprograminfo->ACSEnergy.Q1_Qt_Rate[3]*1.0/(ENERGY_CONST*ENERGY_COEF));
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
		if(show_flg==1){
			//地址类型、逻辑地址、终端地址
			if (p_JProgramInfo->oi_changed.oi4001 != g_chgOI4001) {
				g_chgOI4001 = p_JProgramInfo->oi_changed.oi4001;
				readCoverClass(0x4001, 0, (void*)&g_Class4001_4002_4003,sizeof(CLASS_4001_4002_4003), para_vari_save);
			}

			bcd2str(&g_Class4001_4002_4003.curstom_num[1],(INT8U*)chg_str,g_Class4001_4002_4003.curstom_num[0],sizeof(str),positive);
			sprintf((char*)str,"终端地址 %s",chg_str);
			gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
			gui_textshow((char*)str, pos, LCD_NOREV);
		}
		else if(show_flg==2){
			TSGet(&curts);

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
void LunXunShowPage_X2_Q_All(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){
			Point pos = {0};
			TS curts = {0};
			INT8U str[100] = {0};
			INT8U chg_str[50] = {0};

			gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+FONTSIZE);
			gui_textshow((char *)"二象限无功电能示值", pos, LCD_NOREV);
			memset(str, 0, 100);

			sprintf((char*)str,"总% 14.2f kVArh",jprograminfo->ACSEnergy.Q2_Qt_All*1.0/(ENERGY_CONST*ENERGY_COEF));
			pos.x = rect_Client.left + FONTSIZE/2;
			pos.y = rect_Client.top + 5*FONTSIZE;
			gui_textshow((char*)str, pos, LCD_NOREV);
			sprintf((char*)str,"尖% 14.2f kVArh",jprograminfo->ACSEnergy.Q2_Qt_Rate[0]*1.0/(ENERGY_CONST*ENERGY_COEF));
			pos.y += 2*FONTSIZE+OFFSET_Y;
			gui_textshow((char*)str, pos, LCD_NOREV);
			sprintf((char*)str,"峰% 14.2f kVArh",jprograminfo->ACSEnergy.Q2_Qt_Rate[1]*1.0/(ENERGY_CONST*ENERGY_COEF));
			pos.y += 2*FONTSIZE+OFFSET_Y;
			gui_textshow((char*)str, pos, LCD_NOREV);
			sprintf((char*)str,"平% 14.2f kVArh",jprograminfo->ACSEnergy.Q2_Qt_Rate[2]*1.0/(ENERGY_CONST*ENERGY_COEF));
			pos.y += 2*FONTSIZE+OFFSET_Y;
			gui_textshow((char*)str, pos, LCD_NOREV);
			sprintf((char*)str,"谷% 14.2f kVArh",jprograminfo->ACSEnergy.Q2_Qt_Rate[3]*1.0/(ENERGY_CONST*ENERGY_COEF));
			pos.y += 2*FONTSIZE+OFFSET_Y;
			gui_textshow((char*)str, pos, LCD_NOREV);
			if(show_flg==1){
				//地址类型、逻辑地址、终端地址
				if (p_JProgramInfo->oi_changed.oi4001 != g_chgOI4001) {
					g_chgOI4001 = p_JProgramInfo->oi_changed.oi4001;
					readCoverClass(0x4001, 0, (void*)&g_Class4001_4002_4003,sizeof(CLASS_4001_4002_4003), para_vari_save);
				}

				bcd2str(&g_Class4001_4002_4003.curstom_num[1],(INT8U*)chg_str,g_Class4001_4002_4003.curstom_num[0],sizeof(str),positive);
				sprintf((char*)str,"终端地址 %s",chg_str);
				gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
				gui_textshow((char*)str, pos, LCD_NOREV);
			}
			else if(show_flg==2){
				TSGet(&curts);

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
void LunXunShowPage_X3_Q_All(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){
	Point pos = {0};
		TS curts = {0};
		INT8U str[100] = {0};
		INT8U chg_str[50] = {0};

		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+FONTSIZE);
		gui_textshow((char *)"三象限无功电能示值", pos, LCD_NOREV);
		memset(str, 0, 100);

		sprintf((char*)str,"总% 14.2f kVArh",jprograminfo->ACSEnergy.Q3_Qt_All*1.0/(ENERGY_CONST*ENERGY_COEF));
		pos.x = rect_Client.left + FONTSIZE/2;
		pos.y = rect_Client.top + 5*FONTSIZE;
		gui_textshow((char*)str, pos, LCD_NOREV);
		sprintf((char*)str,"尖% 14.2f kVArh",jprograminfo->ACSEnergy.Q3_Qt_Rate[0]*1.0/(ENERGY_CONST*ENERGY_COEF));
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
		sprintf((char*)str,"峰% 14.2f kVArh",jprograminfo->ACSEnergy.Q3_Qt_Rate[1]*1.0/(ENERGY_CONST*ENERGY_COEF));
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
		sprintf((char*)str,"平% 14.2f kVArh",jprograminfo->ACSEnergy.Q3_Qt_Rate[2]*1.0/(ENERGY_CONST*ENERGY_COEF));
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
		sprintf((char*)str,"谷% 14.2f kVArh",jprograminfo->ACSEnergy.Q3_Qt_Rate[3]*1.0/(ENERGY_CONST*ENERGY_COEF));
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
		if(show_flg==1){
			//地址类型、逻辑地址、终端地址
			if (p_JProgramInfo->oi_changed.oi4001 != g_chgOI4001) {
				g_chgOI4001 = p_JProgramInfo->oi_changed.oi4001;
				readCoverClass(0x4001, 0, (void*)&g_Class4001_4002_4003,sizeof(CLASS_4001_4002_4003), para_vari_save);
			}

			bcd2str(&g_Class4001_4002_4003.curstom_num[1],(INT8U*)chg_str,g_Class4001_4002_4003.curstom_num[0],sizeof(str),positive);
			sprintf((char*)str,"终端地址 %s",chg_str);
			gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
			gui_textshow((char*)str, pos, LCD_NOREV);
		}
		else if(show_flg==2){
			TSGet(&curts);

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
void LunXunShowPage_X4_Q_All(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){
	Point pos = {0};
				TS curts = {0};
				INT8U str[100] = {0};
				INT8U chg_str[50] = {0};

				gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+FONTSIZE);
				gui_textshow((char *)"四象限无功电能示值", pos, LCD_NOREV);
				memset(str, 0, 100);

				sprintf((char*)str,"总% 14.2f kVArh",jprograminfo->ACSEnergy.Q4_Qt_All*1.0/(ENERGY_CONST*ENERGY_COEF));
				pos.x = rect_Client.left + FONTSIZE/2;
				pos.y = rect_Client.top + 5*FONTSIZE;
				gui_textshow((char*)str, pos, LCD_NOREV);
				sprintf((char*)str,"尖% 14.2f kVArh",jprograminfo->ACSEnergy.Q4_Qt_Rate[0]*1.0/(ENERGY_CONST*ENERGY_COEF));
				pos.y += 2*FONTSIZE+OFFSET_Y;
				gui_textshow((char*)str, pos, LCD_NOREV);
				sprintf((char*)str,"峰% 14.2f kVArh",jprograminfo->ACSEnergy.Q4_Qt_Rate[1]*1.0/(ENERGY_CONST*ENERGY_COEF));
				pos.y += 2*FONTSIZE+OFFSET_Y;
				gui_textshow((char*)str, pos, LCD_NOREV);
				sprintf((char*)str,"平% 14.2f kVArh",jprograminfo->ACSEnergy.Q4_Qt_Rate[2]*1.0/(ENERGY_CONST*ENERGY_COEF));
				pos.y += 2*FONTSIZE+OFFSET_Y;
				gui_textshow((char*)str, pos, LCD_NOREV);
				sprintf((char*)str,"谷% 14.2f kVArh",jprograminfo->ACSEnergy.Q4_Qt_Rate[3]*1.0/(ENERGY_CONST*ENERGY_COEF));
				pos.y += 2*FONTSIZE+OFFSET_Y;
				gui_textshow((char*)str, pos, LCD_NOREV);
				if(show_flg==1){
					//地址类型、逻辑地址、终端地址
					if (p_JProgramInfo->oi_changed.oi4001 != g_chgOI4001) {
						g_chgOI4001 = p_JProgramInfo->oi_changed.oi4001;
						readCoverClass(0x4001, 0, (void*)&g_Class4001_4002_4003,sizeof(CLASS_4001_4002_4003), para_vari_save);
					}

					bcd2str(&g_Class4001_4002_4003.curstom_num[1],(INT8U*)chg_str,g_Class4001_4002_4003.curstom_num[0],sizeof(str),positive);
					sprintf((char*)str,"终端地址 %s",chg_str);
					gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
					gui_textshow((char*)str, pos, LCD_NOREV);
				}
				else if(show_flg==2){
					TSGet(&curts);

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
void LunXunShowPage_S(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg)
{
		Point pos;
		TS curts = {0};
		INT8U str[100];
		INT8U chg_str[50] = {0};

		gui_setpos(&pos, rect_Client.left+8*FONTSIZE, rect_Client.top+FONTSIZE);
		gui_textshow((char *)"当前视在功率", pos, LCD_NOREV);
		pos.x = rect_Client.left + 4*FONTSIZE;
		pos.y += 2*FONTSIZE+OFFSET_Y;
		memset(str, 0, 100);

		sprintf((char*)str,"总 % 12.4f kVA",jprograminfo->ACSRealData.St*1.0/(S_COEF*ENERGY_COEF*10));
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
		memset(str, 0, 100);
		if(jprograminfo->dev_info.WireType != 0x1200 && jprograminfo->dev_info.WireType != 0x0600)//0x1200 三相三线接法 0x0600 三相四线接法
		{
			bzero(str,100);
			sprintf((char*)str,"未知的接线方式");
			gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
			gui_textshow((char*)str, pos, LCD_NOREV);
			return;
		}
		bzero(str,100);

		sprintf((char*)str,"A相% 12.4f kVA",jprograminfo->ACSRealData.Sa*1.0/(S_COEF*ENERGY_COEF*10));
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
		memset(str, 0, 100);
		if(jprograminfo->dev_info.WireType == 0x0600)
		{
			sprintf((char*)str,"B相% 12.4f kVA",jprograminfo->ACSRealData.Sb*1.0/(S_COEF*ENERGY_COEF*10));
			pos.y += 2*FONTSIZE+OFFSET_Y;
			gui_textshow((char*)str, pos, LCD_NOREV);
			memset(str, 0, 100);
		}
		sprintf((char*)str,"C相% 12.4f kVA",jprograminfo->ACSRealData.Sc*1.0/(S_COEF*ENERGY_COEF*10));
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);

		if(show_flg==1){
			//地址类型、逻辑地址、终端地址
			if (p_JProgramInfo->oi_changed.oi4001 != g_chgOI4001) {
				g_chgOI4001 = p_JProgramInfo->oi_changed.oi4001;
				readCoverClass(0x4001, 0, (void*)&g_Class4001_4002_4003,sizeof(CLASS_4001_4002_4003), para_vari_save);
			}

			bcd2str(&g_Class4001_4002_4003.curstom_num[1],(INT8U*)chg_str,g_Class4001_4002_4003.curstom_num[0],sizeof(str),positive);
			sprintf((char*)str,"终端地址 %s",chg_str);
			gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
			gui_textshow((char*)str, pos, LCD_NOREV);
		}
		else if(show_flg==2){
			TSGet(&curts);

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
//void LunXunShowPage5(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){
//{
//
//}
void LunXunShowPage7(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){

	Point pos = {0};
	TS curts = {0};
	INT8U str[100] = {0};
	INT8U chg_str[50] = {0};

	bzero(str,100);
	gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"当前电压", pos, LCD_NOREV);

	if(jprograminfo->dev_info.WireType != 0x1200 && jprograminfo->dev_info.WireType != 0x0600)//0x1200 三相三线接法 0x0600 三相四线接法
	{
		bzero(str,100);
		sprintf((char*)str,"未知的接线方式");
		gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		return;
	}
	bzero(str,100);
	if(jprograminfo->dev_info.WireType == 0x0600)
	{
		sprintf((char*)str,"A相电压% 9.3f V",jprograminfo->ACSRealData.Ua*1.0/U_COEF);
		gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		sprintf((char*)str,"B相电压% 9.3f V",jprograminfo->ACSRealData.Ub*1.0/U_COEF);
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
		sprintf((char*)str,"C相电压% 9.3f V",jprograminfo->ACSRealData.Uc*1.0/U_COEF);
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
	}
	else if(jprograminfo->dev_info.WireType == 0x1200)
	{
		sprintf((char*)str,"    Uab% 9.3f V",jprograminfo->ACSRealData.Ua*1.0/U_COEF);
		gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		sprintf((char*)str,"    Ubc% 9.3f V",jprograminfo->ACSRealData.Uc*1.0/U_COEF);
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
	}

	if(show_flg==1){
		//地址类型、逻辑地址、终端地址

		if (p_JProgramInfo->oi_changed.oi4001 != g_chgOI4001) {
			g_chgOI4001 = p_JProgramInfo->oi_changed.oi4001;
			readCoverClass(0x4001, 0, (void*)&g_Class4001_4002_4003, sizeof(CLASS_4001_4002_4003), para_vari_save);
		}
		bcd2str(&g_Class4001_4002_4003.curstom_num[1],(INT8U*)chg_str,g_Class4001_4002_4003.curstom_num[0],sizeof(str),positive);
		sprintf((char*)str,"终端地址 %s",chg_str);
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
	}else if(show_flg==2){
		TSGet(&curts);

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

void LunXunShowPage8(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){

	Point pos = {0};
	TS curts = {0};
	INT8U str[100] = {0};
	INT8U chg_str[50] = {0};

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
		return;
	}
	bzero(str,100);
	sprintf((char*)str,"A相电流% 9.3f A",jprograminfo->ACSRealData.Ia*1.0/I_COEF);
	gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
	gui_textshow((char*)str, pos, LCD_NOREV);

	sprintf((char*)str,"B相电流% 9.3f A",jprograminfo->ACSRealData.Ib*1.0/I_COEF);
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);

	sprintf((char*)str,"C相电流% 9.3f A",jprograminfo->ACSRealData.Ic*1.0/I_COEF);
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	if(show_flg==1){
		//地址类型、逻辑地址、终端地址

		if (p_JProgramInfo->oi_changed.oi4001 != g_chgOI4001) {
			g_chgOI4001 = p_JProgramInfo->oi_changed.oi4001;
			readCoverClass(0x4001, 0, (void*)&g_Class4001_4002_4003,sizeof(CLASS_4001_4002_4003), para_vari_save);
		}
		bcd2str(&g_Class4001_4002_4003.curstom_num[1],(INT8U*)chg_str,g_Class4001_4002_4003.curstom_num[0],sizeof(str),positive);
		sprintf((char*)str,"终端地址 %s",chg_str);
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
	}else if(show_flg==2){
		TSGet(&curts);

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

void LunXunShowPage9(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){

	Point pos = {0};
	TS curts = {0};
	INT8U str[100] = {0};
	INT8U chg_str[50] = {0};

	bzero(str,100);
	gui_setpos(&pos, rect_Client.left+7*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"当前有功功率", pos, LCD_NOREV);

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
		return;
	}
	bzero(str,100);
	sprintf((char*)str,"A相%15.4f kW",jprograminfo->ACSRealData.Pa*1.0/(P_COEF*1000));
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	if(jprograminfo->dev_info.WireType == 0x0600)
		{
			sprintf((char*)str,"B相%15.4f kW",jprograminfo->ACSRealData.Pb*1.0/(P_COEF*1000));
			pos.y += 2*FONTSIZE+OFFSET_Y;
			gui_textshow((char*)str, pos, LCD_NOREV);
		}
		sprintf((char*)str,"C相%15.4f kW",jprograminfo->ACSRealData.Pc*1.0/(P_COEF*1000));
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
	if(show_flg==1){
		//地址类型、逻辑地址、终端地址

		if (p_JProgramInfo->oi_changed.oi4001 != g_chgOI4001) {
			g_chgOI4001 = p_JProgramInfo->oi_changed.oi4001;
			readCoverClass(0x4001, 0, (void*)&g_Class4001_4002_4003, sizeof(CLASS_4001_4002_4003), para_vari_save);
		}
		bcd2str(&g_Class4001_4002_4003.curstom_num[1],(INT8U*)chg_str,g_Class4001_4002_4003.curstom_num[0],sizeof(str),positive);
		sprintf((char*)str,"终端地址 %s",chg_str);
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
	}else if(show_flg==2){
		TSGet(&curts);

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
void LunXunShowPage10(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){
	Point pos = {0};
	TS curts = {0};
	INT8U str[100] = {0};
	INT8U chg_str[50] = {0};

	bzero(str,100);
	gui_setpos(&pos, rect_Client.left+7*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"当前无功功率", pos, LCD_NOREV);

	sprintf((char*)str,"总 % 12.4f kVar",jprograminfo->ACSRealData.Qt*1.0/(Q_COEF*1000));
	gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+5*FONTSIZE);
	gui_textshow((char*)str, pos, LCD_NOREV);
	//fprintf(stderr,"接线方式：%02x",shmm_getdevstat()->WireType);
	if(jprograminfo->dev_info.WireType != 0x1200 && jprograminfo->dev_info.WireType != 0x0600)
	{
		bzero(str,100);
		sprintf((char*)str,"未知的接线方式");
		gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		return;
	}
	bzero(str,100);
	sprintf((char*)str,"A相%12.4f kVar",jprograminfo->ACSRealData.Qt*1.0/(Q_COEF*1000));
	pos.y += 2*FONTSIZE+OFFSET_Y;
	gui_textshow((char*)str, pos, LCD_NOREV);
	if(jprograminfo->dev_info.WireType == 0x0600)
		{
			sprintf((char*)str,"B相%12.4f kVar",jprograminfo->ACSRealData.Qt*1.0/(Q_COEF*1000));
			pos.y += 2*FONTSIZE+OFFSET_Y;
			gui_textshow((char*)str, pos, LCD_NOREV);
		}
		sprintf((char*)str,"C相%12.4f kVar",jprograminfo->ACSRealData.Qt*1.0/(Q_COEF*1000));
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
	if(show_flg==1){
		//地址类型、逻辑地址、终端地址

		if (p_JProgramInfo->oi_changed.oi4001 != g_chgOI4001) {
			g_chgOI4001 = p_JProgramInfo->oi_changed.oi4001;
			readCoverClass(0x4001, 0, (void*)&g_Class4001_4002_4003, sizeof(CLASS_4001_4002_4003), para_vari_save);
		}
		bcd2str(&g_Class4001_4002_4003.curstom_num[1],(INT8U*)chg_str,g_Class4001_4002_4003.curstom_num[0],sizeof(str),positive);
		sprintf((char*)str,"终端地址 %s",chg_str);
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
	}else if(show_flg==2){
		TSGet(&curts);

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

void LunXunShowPage11(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg){
		Point pos = {0};
		TS curts = {0};
		INT8U str[100] = {0};
		INT8U chg_str[50] = {0};

		bzero(str,100);
		gui_setpos(&pos, rect_Client.left+7*FONTSIZE, rect_Client.top+FONTSIZE);
		gui_textshow((char *)"当前功率因数", pos, LCD_NOREV);

		sprintf((char*)str,"总功率因数 % 9.4f ",jprograminfo->ACSRealData.Cos*1.0/COS_COEF);
		gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+5*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
		//fprintf(stderr,"接线方式：%02x",shmm_getdevstat()->WireType);
		if(jprograminfo->dev_info.WireType != 0x1200 && jprograminfo->dev_info.WireType != 0x0600)
		{
			bzero(str,100);
			sprintf((char*)str,"未知的接线方式");
			gui_setpos(&pos, rect_Client.left+4*FONTSIZE, rect_Client.top+5*FONTSIZE);
			gui_textshow((char*)str, pos, LCD_NOREV);
			return;
		}
		bzero(str,100);
		sprintf((char*)str,"A相功率因数%9.4f ",jprograminfo->ACSRealData.CosA*1.0/COS_COEF);
		pos.y += 2*FONTSIZE+OFFSET_Y;
		gui_textshow((char*)str, pos, LCD_NOREV);
		if(jprograminfo->dev_info.WireType == 0x0600)
			{
				sprintf((char*)str,"B相功率因数%9.4f ",jprograminfo->ACSRealData.CosB*1.0/COS_COEF);
				pos.y += 2*FONTSIZE+OFFSET_Y;
				gui_textshow((char*)str, pos, LCD_NOREV);
			}
			sprintf((char*)str,"C相功率因数%9.4f ",jprograminfo->ACSRealData.CosC*1.0/COS_COEF);
			pos.y += 2*FONTSIZE+OFFSET_Y;
			gui_textshow((char*)str, pos, LCD_NOREV);
		if(show_flg==1){
			//地址类型、逻辑地址、终端地址

			if (p_JProgramInfo->oi_changed.oi4001 != g_chgOI4001) {
				g_chgOI4001 = p_JProgramInfo->oi_changed.oi4001;
				readCoverClass(0x4001, 0, (void*)&g_Class4001_4002_4003, sizeof(CLASS_4001_4002_4003), para_vari_save);
			}
			bcd2str(&g_Class4001_4002_4003.curstom_num[1],(INT8U*)chg_str,g_Class4001_4002_4003.curstom_num[0],sizeof(str),positive);
			sprintf((char*)str,"终端地址 %s",chg_str);
			gui_setpos(&pos, rect_Client.left+3*FONTSIZE, rect_Client.top+18*FONTSIZE);
			gui_textshow((char*)str, pos, LCD_NOREV);
		}else if(show_flg==2){
			TSGet(&curts);

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

void ShowJCData(ProgramInfo* p_JProgramInfo,int PageNo, LcdDataItem *item, int size, int show_flg)
{
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
		LunXunShowPage_X1_Q_All(p_JProgramInfo,item, size, show_flg);//一象限无功电能示值
		break;
	case 5:
		LunXunShowPage_X2_Q_All(p_JProgramInfo,item, size, show_flg);//二象限无功电能示值
		break;
	case 6:
		LunXunShowPage_X3_Q_All(p_JProgramInfo,item, size, show_flg);//三象限无功电能示值
		break;
	case 7:
		LunXunShowPage_X4_Q_All(p_JProgramInfo,item, size, show_flg);//四象限无功电能示值
		break;
	case 8:
		LunXunShowPage_S(p_JProgramInfo,item, size, show_flg);//视在功率
		break;
	case 9:
		LunXunShowPage7(p_JProgramInfo,item, size, show_flg);//当前电压
		break;
	case 10:
		LunXunShowPage8(p_JProgramInfo,item, size, show_flg);//当前电流
		break;
	case 11:
		LunXunShowPage9(p_JProgramInfo,item, size, show_flg);//当前有功功率
		break;
	case 12:
		LunXunShowPage10(p_JProgramInfo,item, size, show_flg);//当前无功功率
		break;
	case 13:
		LunXunShowPage11(p_JProgramInfo,item, size, show_flg);//当前功率因数
		break;
//	case 14:
//		LunXunShowPage5(item, size, show_flg);//当前有功最大需量
//		break;
	default:
		break;
	}
}

LunXian_t LunXian[LunPageNum]={
		{"当前正向有功", 	LunXunShowPage1, 			1},//0
		{"当前反向有功", 	LunXunShowPage3, 			1},//1
		{"当前正向无功",	LunXunShowPage2, 			1},//2
		{"当前反向无功", 	LunXunShowPage4, 			1},//3
		{"一象限无功", 	LunXunShowPage_X1_Q_All, 	1},//4
		{"二象限无功", 	LunXunShowPage_X2_Q_All, 	1},//5
		{"三象限无功", 	LunXunShowPage_X3_Q_All, 	1},//6
		{"四象限无功", 	LunXunShowPage_X4_Q_All, 	1},//7
		{"视在功率", 		LunXunShowPage_S, 			1},//8
		{"当前电压", 		LunXunShowPage7, 			1},//9
		{"当前电流", 		LunXunShowPage8, 			1},//10
		{"当前有功功率", 	LunXunShowPage9, 			1},//11
		{"当前无功功率",	LunXunShowPage10, 			1},//12
		{"当前功率因数", 	LunXunShowPage11, 			1},//13
		//{"当前有功最大需量",LunXunShowPage5, 			1},//14//目前烟台关于需量代码没完成20170810
		//{"通信参数",		lcdpoll_showidapnip, 		1}//15
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
			show_ctrl();//显示控制非客户区//TODO:new//轮显时更新上下状态栏
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
