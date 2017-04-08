

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

#include "show_ctrl.h"
#define BUFFSIZE 512
#define DATA_CONTENT_LEN 500
#define NUM_07DI_698OAD 100

#define DF07_BYTES  4
#define DF07_INFO_BYTES  50
#define MAXLEN_1LINE  100
#define CLASS_601F_CFG_FILE "/nor/config/07DI_698OAD.cfg"

#define PARA_CHANGE_RETVALUE  -1

#define MAX_RETRY_NUM 1 //抄表失败重试次数

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
//OAD+数据
typedef struct
{
	INT8U oad[4];
	INT8U datalen;
	INT8U data[50];
}OAD_DATA;


extern void read485_proccess();

mqd_t mqd_485_1_task;
mqd_t mqd_485_2_task;
struct mq_attr attr_485_main;
struct mq_attr attr_485_1_task;
struct mq_attr attr_485_2_task;

INT32S comfd485[2];
INT8U i485port1;
INT8U readState;//是否正在处理实时消息
INT8U i485port2;

//698 OAD 和 645 07规约 数据标识对应关系
INT8U map07DI_698OAD_NUM;
CLASS_601F map07DI_698OAD[NUM_07DI_698OAD];

#endif /* READ485_H_ */
