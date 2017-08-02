/*
 * show_ctrl.h
 *
 *  Created on: 2017-3-7
 *      Author: prayer
 */

#ifndef SHOW_CTRL_H_
#define SHOW_CTRL_H_
#include "../libAccess/AccessFun.h"
#include "../include/ParaDef.h"
#include "../include/Objectdef.h"
#include "../libBase/PublicFunction.h"
#include "../libMq/libmmq.h"
#include "../include/Shmem.h"
#define PROXY 1
#define PosPt_All_Id 117
#define PosPt_Rate1_Id 118
#define PosPt_Rate2_Id 119
#define PosPt_Rate3_Id 120
#define PosPt_Rate4_Id 121
#define DATA_INIT 1
#define EVENT_INIT 2
#define DEMAND_INIT 3
#define FACTORY_RESET 4

#define GUI_FIRST_RUN	1
#define GUI_NOT_FIRST_RUN	0

#define PARA_HAS_READ	1
#define PARA_NOT_READ	0


typedef enum
{
	unknown,Dl97,Dl07,Dl698,Cj188
}PROTOCOL;
typedef enum {
	YEAR,MONTH,DAY,HOUR,MIN,SEC
}TIMEUNIT;
//点抄结果
typedef struct
{
	TS tm_collect;//采集时间
	INT8U data_All[4];//总电能示值，以下为各费率电能示值
	INT8U Rate1_Data[4];
	INT8U Rate2_Data[4];
	INT8U Rate3_Data[4];
	INT8U Rate4_Data[4];
}RealDataInfo;

typedef struct
{
	INT8U done_flag;			 //完成标志
	TSA addr;                    //通信地址
    INT8U baud;                  //波特率
    INT8U protocol;              //规约类型
    OAD port;                    //端口
	OI_698 oi;                   //数据标识
    RealDataInfo realdata;       //请求结果,总及4费率
}Proxy_Msg;

extern ProgramInfo* p_JProgramInfo;

extern INT8U g_chgOI4500;
extern INT8U g_chgOI4300;

extern CLASS26 g_class26_oi4510;
extern CLASS25 g_class25_oi4500;
extern CLASS_4001_4002_4003 g_Class4001_4002_4003;
extern CLASS19 g_class19_oi4300;

extern void showmain();
extern void show_ctrl();
extern void lcd_showTopStatus();
extern void lcd_showBottomStatus(int zb_status, int gprs_status);
extern void ProgramInfo_register(ProgramInfo* JProgramInfo);
extern void Proxy_Msg_Data_register(Proxy_Msg* Proxy_Msg_Data);
extern void Init_GuiLib_variable();
#endif /* SHOW_CTRL_H_ */
