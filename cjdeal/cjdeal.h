#ifndef CJMAIN_H_
#define CJMAIN_H_

#include "PublicFunction.h"

extern ProgramInfo* JProgramInfo;

extern void DealState(ProgramInfo* prginfo);
extern void read_oif203_para();

#define TASK6012_MAX 256


typedef struct {
	INT8U run_flg;//累计需要抄读次数 抄读一次后置为0   到下一次抄读时间置为1
	time_t ts_next;//下一次抄表时刻
	CLASS_6013 basicInfo;
}TASK_CFG;
INT8U total_tasknum;
TASK_CFG list6013[TASK6012_MAX];


typedef enum
{
	para_no_chg =0,
	para_6000_chg =0x01,
	para_6012_chg =0x02,
	para_6014_chg =0x04
}PARA_CHG_TYPE;
INT8U para_ChangeType;


//6000测量点信息
#define MAX_METER_NUM_1_PORT 200

typedef struct
{
	INT16U meterSum;
	INT16U list6001[MAX_METER_NUM_1_PORT];
}INFO_6001_LIST;

INFO_6001_LIST info6000[2];//两路485


//任务调度
pthread_attr_t dispatchTask_attr_t;
int thread_dispatchTask_id;
pthread_t thread_dispatchTask;
extern INT8U is485OAD(OAD portOAD,INT8U port485);
#endif
