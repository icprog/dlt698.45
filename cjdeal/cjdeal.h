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
	para_6014_chg =0x04,
	para_4000_chg =0x08,
	para_4204_chg =0x10,

}PARA_CHG_TYPE;
INT8U para_ChangeType;


//6000测量点信息
#define MAX_METER_NUM_1_PORT 200

typedef struct
{
	INT16U meterSum;//此端口上的测量点数量
	INT16U list6001[MAX_METER_NUM_1_PORT];//测量点序号
}INFO_6001_LIST;

INFO_6001_LIST info6000[2];//两路485

INT8U para_change485[2];//参数变更后置1  485 1 2线程清空队列中剩余未执行的任务ID后置0

CLASS_4204	broadcase4204;
INT8U flagDay_4204[2];//标识当天是否已经广播校时
//任务调度
pthread_attr_t dispatchTask_attr_t;
int thread_dispatchTask_id;
pthread_t thread_dispatchTask;
extern INT8U is485OAD(OAD portOAD,INT8U port485);
#endif
