#ifndef CJMAIN_H_
#define CJMAIN_H_

#include "PublicFunction.h"
#include "libmmq.h"
#include "show_ctrl.h"
#include <semaphore.h>

extern ProgramInfo* JProgramInfo;

extern void DealState(ProgramInfo* prginfo);
extern void read_oif203_para();
extern INT32S spi_close(INT32S fd);
extern INT8U is485OAD(OAD portOAD, INT8U port485);
//采集任务最大数量
#define TASK6012_CAIJI 10

typedef struct {
	INT8U run_flg; //累计需要抄读次数 抄读一次后置为0   到下一次抄读时间置为1
	time_t ts_next; //下一次抄表时刻
	CLASS_6013 basicInfo;
	CLASS_6035 Info6035;
} TASK_CFG;
INT8U total_tasknum;
TASK_CFG list6013[TASK6012_CAIJI];

typedef enum {
	para_no_chg = 0,
	para_6000_chg = 0x01,
	para_6012_chg = 0x02,
	para_6014_chg = 0x04,
	para_4000_chg = 0x08,
	para_4204_chg = 0x10,

} PARA_CHG_TYPE;
INT8U para_ChangeType;

//6000测量点信息
#define MAX_METER_NUM_1_PORT 200
//需要补抄的任务个数
#define MAX_REPLENISH_TASK_NUM 5
typedef struct {
	INT16U meterSum; //此端口上的测量点数量
	INT16U list6001[MAX_METER_NUM_1_PORT]; //测量点序号
} INFO_6001_LIST;

INFO_6001_LIST info6000[2]; //两路485
INT8U isNeed4852; //0-4852维护口　1-4852抄表口

/*补抄相关结构体*/
INT8U isReplenishOver[4]; //四个时间点补抄标志
typedef struct {
	INT8U taskID;
	INFO_6001_LIST list6001[2]; //测量点序号-两路485
	INT8U isSuccess[2][MAX_METER_NUM_1_PORT]; //此任务此测量点是否抄读成功
} Replenish_TaskInfoUnit;

typedef struct {
	INT8U tasknum; //有多少个任务需要补抄
	Replenish_TaskInfoUnit unitReplenish[MAX_REPLENISH_TASK_NUM]; //需要补抄的任务详细信息
} Replenish_TaskInfo;

Replenish_TaskInfo infoReplenish;

INT8U para_change485[2]; //参数变更后置1  485 1 2线程清空队列中剩余未执行的任务ID后置0
INT8U isAllowReport;
CLASS_4204 broadcase4204;
INT8U flagDay_4204[2]; //标识当天是否已经广播校时
//任务调度
pthread_attr_t dispatchTask_attr_t;
int thread_dispatchTask_id;
pthread_t thread_dispatchTask;

mqd_t mqd_485_main; //接受点抄的消息队列
pthread_mutex_t mutex;
pthread_mutex_t mutex_savetask;
typedef union {
	INT8U u8b; //方便变量初始化
	struct {
		INT8U proxyIdle :1; //代理操作是否空闲, 0-空闲, 1-被占用
		INT8U reser :1; //保留
		INT8U rs485_1_Active :1; //485-1代理执行标记 0-空闲 ，1-执行中
		INT8U rs485_2_Active :1; //485-2代理执行标记 0-空闲 ，1-执行中
		INT8U plcNeed :1; //需要使用载波模块标记, 0-不需要, 1-需要
		INT8U plcReady :1; //载波模块本次代理操作完成, 0-未就绪, 1-就绪
		INT8U rs485Need :1; //需要使用rs485标记, 0-不需要, 1-需要
		INT8U rs485Ready :1; //rs485本次代理操作完成, 0-未就绪, 1-就绪
	} devUse;
} proxyFlag_u;
proxyFlag_u proxyInUse; //判断本次代理操作是否完成
int proxyTimeOut; //
//保存代理召测信息的结构体
typedef struct {
	INT8U isInUse; //是否被占用 如果被占用 出现新的消息舍弃 第一位: 485 1 第二位:485 2 第三位:plc
	PROXY_GETLIST strProxyList;
} CJCOMM_PROXY;
CJCOMM_PROXY cjcommProxy;
CJCOMM_PROXY cjcommProxy_plc;

//保存液晶点抄信息的结构体
typedef struct {
	INT8U isInUse; //是否被占用 如果被占用 出现新的消息舍弃
	Proxy_Msg strProxyMsg;
} GUI_PROXY;
GUI_PROXY cjguiProxy;
GUI_PROXY cjGuiProxy_plc;

void printinfoReplenish(INT8U);
INT8U get6001ObjByTSA(TSA addr, CLASS_6001* targetMeter);
INT8U increase6035Value(INT8U taskID,INT8U type);
INT8U isTimerSame(INT8S index, INT8U* timeData);
#endif
