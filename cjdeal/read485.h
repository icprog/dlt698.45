/*
 * read485.h
 *
 *  Created on: 2017-1-4
 *      Author: wzm
 */

#ifndef READ485_H_
#define READ485_H_
#include "Objectdef.h"
#include "PublicFunction.h"
#include "AccessFun.h"
#include "dlt698.h"
//一次从文件里读取10个6001--采集档案任务配置单元
#define LIST6001SIZE 10
#define TASK6012_MAX 256
#define BUFFSIZE 256
pthread_attr_t read485_attr_t;
int thread_read4851_id,thread_read4852_id;           //485、四表合一（I型、II型、专变）
pthread_t thread_read4851,thread_read4852;


typedef enum{
	PROTOCOL_UNKNOWN = 0,
	DLT_645_97 = 1,
	DLT_645_07 = 2,
	DLT_698 = 3,
	CJT_188 = 4,
}METER_PROTOCOL;
typedef struct
{
	TSA addr;			//通信地址
	INT8U baud;			//波特率
	METER_PROTOCOL protocol;		//规约类型
	INT8U port;			//端口
}BasicInfo6001;

typedef struct {
	INT8U resetFlag;
	INT8U run_flg;//本轮已抄读 二进制：00未执行 01已执行(由抄表程序置位) 论次变更时置0
	int last_round;//上一次抄表时段唯一标识值，判断某一时间论次是否变更
	TS ts_last;//上一次抄表时刻
	CLASS_6013 basicInfo;
}TASK_CFG;


TASK_CFG list6013[TASK6012_MAX];
CLASS_6015 to6015;
INT32S comfd4851;
INT32S comfd4852;
extern void read485_proccess();
#endif /* READ485_H_ */
