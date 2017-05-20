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
