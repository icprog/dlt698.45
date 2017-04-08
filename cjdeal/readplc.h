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

CLASS_6013 taskunite[TASK6012_MAX];
typedef struct
{
	OAD oad1;	//非关联 oad1.OI=0
	OAD oad2;	//数据项
}DATA_ITEM;
typedef union
{
	INT8U type;						//方案类型
	CLASS_6015 to6015;				//普通采集方案
	CLASS_6017 to6017;				//事件采集方案
	DATA_ITEM items[FANGAN_ITEM_MAX ];			//数据项数组
	INT8U item_n;					//数据项总数 < FANGAN_ITEM_MAX
}CJ_FANGAN;
typedef struct
{
	TS nowts;
	TS oldts;
	int initflag;
	int comfd;
	int state_bak;					//运行状态备份
	int tasktype;					//当前任务类型
	INT8U taskno;					//当前任务编号
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
