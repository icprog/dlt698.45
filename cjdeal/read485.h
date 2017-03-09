

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
#define LIST6001SIZE 20
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


//698 OAD 和 645 07规约 数据标识对应关系



extern void read485_proccess();
mqd_t mqd_485_main;

INT32S comfd4851;
INT32S comfd4852;
INT8U i485port1;
INT8U i485port2;
//以下是测试用的假数据
#ifdef TESTDEF
#define TESTARRAYNUM 20
CLASS_601F testArray[TESTARRAYNUM];
#endif
#endif /* READ485_H_ */
