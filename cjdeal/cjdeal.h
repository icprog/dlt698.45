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

//任务调度
pthread_attr_t dispatchTask_attr_t;
int thread_dispatchTask_id;
pthread_t thread_dispatchTask;

#endif
