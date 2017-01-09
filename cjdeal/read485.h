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


//一次从文件里读取10个6001--采集档案任务配置单元
#define LIST6001SIZE 10


pthread_attr_t read485_attr_t;
int thread_read485_id;           //485、四表合一（I型、II型、专变）
pthread_t thread_read485;

extern void read485_proccess();


CLASS_6013 from6013;
CLASS_6015 to6015;


typedef enum{
	DLT_645_07 = 0,
	DLT_698 = 1,
	CJT_188 = 2,
}METER_PROTOCOL;
typedef struct
{
	TSA addr;			//通信地址
	INT8U baud;			//波特率
	METER_PROTOCOL protocol;		//规约类型
	INT8U port;			//端口
}BasicInfo6001;

#endif /* READ485_H_ */
