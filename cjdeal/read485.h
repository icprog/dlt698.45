

#ifndef READ485_H_
#define READ485_H_
#include "Objectdef.h"
#include "PublicFunction.h"
#include "AccessFun.h"
#include "dlt698.h"
#include "dlt645.h"
#include "event.h"
#include "cjsave.h"
#include "libmmq.h"

//一次从文件里读取10个6001--采集档案任务配置单元
#define LIST6001SIZE 10
#define TASK6012_MAX 256
#define BUFFSIZE 256
#define DATA_CONTENT_LEN 500



pthread_attr_t read485_attr_t;
int thread_read4851_id,thread_read4852_id;           //485、四表合一（I型、II型、专变）
pthread_t thread_read4851,thread_read4852;

extern INT16U CalcOIDataLen(OI_698 oi,INT8U attr_flg);

typedef enum{
	PROTOCOL_UNKNOWN = 0,
	DLT_645_97 = 1,
	DLT_645_07 = 2,
	DLT_698 = 3,
	CJT_188 = 4,
}METER_PROTOCOL;
typedef struct
{
	INT16U sernum;
	TSA addr;			//通信地址
	INT8U baud;			//波特率
	METER_PROTOCOL protocol;		//规约类型
	INT8U port;			//端口
}BasicInfo6001;

typedef struct {
	INT8U run_flg;//累计需要抄读次数 抄读一次后置为0   到下一次抄读时间置为1
	TS ts_next;//下一次抄表时刻
	CLASS_6013 basicInfo;
}TASK_CFG;

//698 OAD 和 645 07规约 数据标识对应关系



extern void read485_proccess();
mqd_t mqd_485_main;
TASK_CFG list6013[TASK6012_MAX];
INT32S comfd4851;
INT32S comfd4852;
//以下是测试用的假数据
#ifdef TESTDEF
#define TESTARRAYNUM 20
CLASS_601F testArray[TESTARRAYNUM];
#endif
#endif /* READ485_H_ */
