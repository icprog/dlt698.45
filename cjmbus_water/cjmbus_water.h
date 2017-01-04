#ifndef CJMBUS_WATER_H_
#define CJMBUS_WATER_H_
#include <termios.h>
#include <errno.h>
#include <wait.h>
#include "PublicFunction.h"
#include "gtype.h"
#include "cjt188def.h"
#include "cjt188.h"

#define delay(A) usleep((A)*1000)
#define FRAMELEN 	1024
#define BUFLEN  	1024
typedef struct{
	INT32S	ComPort;	//串口号
	INT32S  Baud; 		//波特率
	INT8U   Bits;		//位数
	INT8U 	Parity[6];	//奇偶
	INT8U	Stopb;		//停止位
}COMPORTCFG;
typedef struct{
	INT16U	MpNo;		//测量点号
	INT8U	Addr[7];	//表地址
	INT8U	MpType;		//表种类
	INT8U	BigNo;		//大类号
	INT8U	LittleNo;	//小类号
	INT8U	Protocol;	//规约类型
	COMPORTCFG comcfg;	//串口配置
}MeterType;
typedef struct{
	INT8U	DI[4];		//数据项标识
	INT8U	DataType;	//数据类型
	INT8U	Status;		//抄读状态	0未抄表	1成功	2抄读失败的	3表计不支持的
	INT8U	TryCount;	//抄读次数
}DataType;
struct DataNode{
	MeterType Meter;	//表基本信息
	DataType Flag;		//数据项标识
	struct DataNode *Next;
};

ProgramInfo* JProgramInfo=NULL;
int ProIndex=0;
int Port;
INT8U	SendBuf[BUFLEN]={};			//发送数据
INT8U 	DealBuf[FRAMELEN]={};  		//保存接口函数处理长度
INT8U 	RecBuf[BUFLEN]={}; 			//接收数据
INT32U	 	RHead=0,RTail=0;		//接收报文头指针，尾指针
INT8U 	deal_step=0;				//数据接收状态机处理标记
int		chaobiao_step=0;			//抄表状态
INT32U	rev_delay=0;				//接收延时
INT8U	meter_serno=0;				//序列号

INT8U	OldType=0;
#endif
