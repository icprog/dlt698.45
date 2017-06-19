/*
 * readplc.h
 *
 *  Created on: 2017-1-4
 *      Author: wzm
 */
#ifndef READPLC_H_
#define READPLC_H_
#include "StdDataType.h"
#include "PublicFunction.h"
#include "AccessFun.h"
#include "format3762.h"
#include "lib3762.h"

pthread_attr_t readplc_attr_t;
int thread_readplc_id;        //载波（I型）
pthread_t thread_readplc;
extern void readplc_proccess();
extern INT16S getTaskIndex(INT8U port);

#define NONE_PROCE	 		-1
#define DATE_CHANGE   		0
#define DATA_REAL     		1
#define METER_SEARCH  		2
#define TASK_PROCESS  		3
#define SLAVE_COMP    		4
#define INIT_MASTERADDR    	5

#define ALLOK 2
#define ZBBUFSIZE 512
#define BUFSIZE645 256
#define TASK6012_MAX 256
#define FANGAN_ITEM_MAX 64
#define DLT645_07  2
#define DLT698  3
#define CJT188  4
#define NUM_07DI_698OAD 100
//---------------------------------------------------------------

//typedef struct
//{
//	INT8U startIndex; 	//报文中的某数据的起始字节
//	INT8U dataLen;	 	//数据长度（字节数）
//	INT8U intbits;		//整数部分长度
//	INT8U decbits;		//小数部分长度
//	char name[30];
//	INT8U Flg07[4];//对应07表实时数据标识
//	OAD oad1;
//	OAD oad2;
//}MeterCurveDataType;//电表负荷记录
//typedef struct
//{
//	INT8U startIndex; 	//报文中的某数据的起始字节
//	INT8U dataLen;	 	//数据长度（字节数）
//}NDataType;//电表负荷记录

//int MCurveData[10];
//{
//						{8,  2, 3, 1, "A相电压",			{0x01,0x01,0x10,0x06}},
//						{10, 2, 3, 1, "B相电压",			{0x02,0x01,0x10,0x06}},
//						{12, 2, 3, 1, "C相电压",			{0x03,0x01,0x10,0x06}},
//
//						{14, 3, 3, 3, "A相电流",			{0x01,0x02,0x10,0x06}},
//						{17, 3, 3, 3, "B相电流",			{0x02,0x02,0x10,0x06}},
//						{20, 3, 3, 3, "C相电流",			{0x03,0x02,0x10,0x06}},
//
//						{23, 2, 2, 2, "频率曲线",			{0xFF,0xFF,0xFF,0xFF}},
//
//						{26, 3, 2, 4, "总有功功率曲线",	{0x00,0x03,0x10,0x06}},
//						{29, 3, 2, 4, "A相有功功率曲线",	{0x01,0x03,0x10,0x06}},
//						{32, 3, 2, 4, "B相有功功率曲线",	{0x02,0x03,0x10,0x06}},
//						{35, 3, 2, 4, "C相有功功率曲线",	{0x03,0x03,0x10,0x06}},
//
//						{38, 3, 2, 4, "总无功功率曲线",	{0x00,0x04,0x10,0x06}},
//						{41, 3, 2, 4, "A相无功功率曲线",	{0x01,0x04,0x10,0x06}},
//						{44, 3, 2, 4, "B相无功功率曲线",	{0x02,0x04,0x10,0x06}},
//						{47, 3, 2, 4, "C相无功功率曲线",	{0x03,0x04,0x10,0x06}},
//
//						{51, 2, 3, 1, "总功率因数曲线",	{0x00,0x05,0x10,0x06}},
//						{53, 2, 3, 1, "A相功率因数曲线",	{0x01,0x05,0x10,0x06}},
//						{55, 2, 3, 1, "B相功率因数曲线",	{0x02,0x05,0x10,0x06}},
//						{57, 2, 3, 1, "C相功率因数曲线",	{0x03,0x05,0x10,0x06}},
//
//						{60, 4, 6, 2, "正向有功总电能曲线",{0x01,0x06,0x10,0x06}},
//						{64, 4, 6, 2, "反向有功总电能曲线",{0x02,0x06,0x10,0x06}},
//						{68, 4, 6, 2, "正向无功总电能曲线",{0x03,0x06,0x10,0x06}},
//						{72, 4, 6, 2, "反向无功总电能曲线",{0x04,0x06,0x10,0x06}},
//
//						{77, 4, 6, 2, "一象限无功总电能曲线",{0x01,0x07,0x10,0x06}},
//						{81, 4, 6, 2, "二象限无功总电能曲线",{0x02,0x07,0x10,0x06}},
//						{85, 4, 6, 2, "三象限无功总电能曲线",{0x03,0x07,0x10,0x06}},
//						{89, 4, 6, 2, "四象限无功总电能曲线",{0x04,0x07,0x10,0x06}},
//
//						{94, 3, 2, 4, "当前有功需量曲线",	{0xFF,0xFF,0xFF,0xFF}},
//						{97, 3, 2, 4, "当前无功需量曲线",	{0xFF,0xFF,0xFF,0xFF}},
//};


CLASS_6015 fangAn6015[20];
TASK_INFO taskinfo;

typedef struct
{
	TS nowts;
	TS oldts;
	int initflag;
	int redo;			//0:无动作   1:重启抄读  2:恢复抄读
	int comfd;
	int state_bak;					//运行状态备份
	int state;						//当前运行状态
	INT8U taskno;					//当前任务编号
	DateTimeBCD endtime;			//任务结束时间
	CJ_FANGAN fangAn;				//当前采集方案
	time_t send_start_time;			//发送开始时间
	CLASS_6035 result6035;			//采集任务监测
	INT8U masteraddr[6];			//主节点地址
	INT8U sendbuf[ZBBUFSIZE];		//发送缓存
	INT8U recvbuf[ZBBUFSIZE];		//接收缓存
	INT8U dealbuf[ZBBUFSIZE];		//待处理数据缓存
	FORMAT3762 format_Down;
	FORMAT3762 format_Up;
}RUNTIME_PLC;
AFN03_F10_UP module_info;
struct Tsa_Node
{
	TSA tsa;
	int tsa_index;
	INT8U protocol;
	INT8U usrtype;
	INT8U flag[8];
	INT8U curr_i;
	INT8U readnum;
	struct Tsa_Node *next;
};
struct Tsa_Node *tsa_head;
struct Tsa_Node *tsa_zb_head;
int tsa_zb_count;
int tsa_count;
/////////////////////////////////////////
int RecvHead;
int RecvTail;
int DataLen;
int SlavePointNum;
INT8U ZBMasterAddr[6];
int rec_step;
time_t oldtime1;
time_t newtime1;
INT8U buf645[BUFSIZE645];
#endif /* EVENTCALC_H_ */
