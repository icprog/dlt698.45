#ifndef OPTION_H_
#define OPTION_H_

#include "StdDataType.h"
//typedef unsigned char  BOOLEAN;
//typedef unsigned char  INT8U;                    /* Unsigned  8 bit quantity                           */
//typedef signed   char  INT8S;                    /* Signed    8 bit quantity                           */
//typedef unsigned short INT16U;                   /* Unsigned 16 bit quantity                           */
//typedef signed   short INT16S;                   /* Signed   16 bit quantity                           */
//typedef unsigned int   INT32U;                   /* Unsigned 32 bit quantity                           */
//typedef signed   int   INT32S;                   /* Signed   32 bit quantity                           */
//typedef unsigned long long     	INT64U;          /* Unsigned 64 bit quantity   						   */
//typedef signed long long  		INT64S;          /* Unsigned 64 bit quantity                           */
//typedef float          FP32;                     /* Single precision floating point                    */
//typedef double         FP64;                     /* Double precision floating point                    */

#define ARGVMAXLEN			50		//参数最大长度
#define PRONAMEMAXLEN		50		//进程名称最大长度
#define	PROJECTCOUNT		10		//守护进程可以支持的最多进程数
#define ARGCMAX				4		//支持进程参数最大数

#define REALDATA_LIST_LENGTH 10
#define _CFGDIR_ 	"/nor/config"

typedef struct _name_attach {
} name_attach_t;
typedef enum{					//该子程序运行状态
	None=0,						//标志无程序运行
	NeedStart,					//该程序需要运行
	NeedKill,					//该程序需要停止运行
	NowRun,						//目前该程序正在运行
}ProjectState;					//程序运行状态

typedef struct
{
	INT16U Year;    //year;
    INT8U  Month;   //month;
    INT8U  Day;     //day;
    INT8U  Hour;    //hour;
    INT8U  Minute;  //minute;
    INT8U  Sec;     //second;
    INT8U  Week;
}TS;
typedef struct{
	INT8U ASK_Port;				//终端通信端口号
	INT8U ASK_Control;         	//通信控制字
	INT8U WaitTime;				//转发接受等待报文超时时间
	INT8U DelayTime;			//转发接受等待字节超时时间
	INT8U len[2];				//转发内容字节数
	INT8U ANSWER_Buf[256];		//长度是2个字节，空间有限先定义一个字节256的空间
}ZF_F1;

typedef struct{
	INT8U ASK_Port;				//终端通信端口号
	INT8U ASK_Leval;			//转发中继级数
	INT8U ASK_DataType;			//转发直接抄读的数据标识类型
	INT8U DataFlag[4];			//转发直接抄读的数据标识 //9.27
	INT8U len;					//转发内容字节数
	INT8U ASK_Buf[32][6];		//第n级转发中继地址 n<=32
	INT8U ANSWER_adress[6];     //转发目标地址
	INT8U ANSWER_endflag;       //转发结果标志
	INT8U ANSWER_Buf[256];		//长度是1个字节，转发直接抄读的数据内容
}ZF_F9;

typedef struct{
	INT8U ASK_Port;				//终端通信端口号
	INT8U ASK_Leval;			//转发中继级数
	INT8U len;					//转发直接遥控命令密码字节数
	INT8U ASK_Buf[32][6];		//n级转发中继地址 n<=32
	INT8U ANSWER_zhabiaozhi;    //遥控跳闸/允许合闸标志
	INT8U ANSWER_adress[6];     //转发目标地址
	INT8U ANSWER_endflag;       //转发结果标志
	INT8U ANSWER_Buf[256];		//长度是1个字节，转发直接遥控命令密码空间
}ZF_F10;

typedef struct{
	INT8U ASK_Port;				//终端通信端口号
	INT8U ASK_Leval;			//转发中继级数
	INT8U len;					//转发直接遥控命令密码字节数
	INT8U ASK_Buf[32][6];		//n级转发中继地址 n<=32
	INT8U ANSWER_songdian;     //遥控送电标志
	INT8U ANSWER_adress[6];    //转发目标地址
	INT8U ANSWER_endflag;      //转发结果标志
	INT8U ANSWER_Buf[256];		//长度是1个字节，转发直接遥控命令密码空间
}ZF_F11;

typedef union{
	unsigned char Buff[500];
	ZF_F1	F1;
	ZF_F1	F9;
	ZF_F1	F10;
	ZF_F1	F11;
}TRANSTYPE;
typedef struct {
	INT8U  type;					//1:透明转发【F1】 9:【F9】 10:【F10】 11:【F11】
	INT32S ticket;					//取数据时的凭据
	INT32S stat;					//数据状态标志 1:就绪  2:下发中... 3:完成
	TRANSTYPE realdata;
}Realdata;					//实时数据结构体
typedef struct {
	Realdata		RealdataList[REALDATA_LIST_LENGTH];
	INT16U			Ticket_g;					//实时数据请求凭据（累加计数）请求时加1
}RealdataReq;
typedef struct{
	INT32U		ProjectID;						//子程序PID  由系统自动分配的，杀死进程时用
	INT8U		ProjectName[PRONAMEMAXLEN];		//程序名称
	INT8U		ProjectRunName[PRONAMEMAXLEN];	//进程正式运行后名称
	INT8U		ProjectState;					//程序运行状态,chaobiao主程序使用,其他程序只能读
	INT8U		WaitTimes;						//记录子进程等待的时间 父进程每秒加一，子进程每循环清零，超出自定义该进程死亡秒数则标志该进程死亡，需要重起
	INT32U		Version;						//进程版本号
	INT8U		argv[ARGCMAX][ARGVMAXLEN];		//运行参数
}ProjectInfo; 	//子程序信息

typedef struct {
	ProjectInfo		Projects[PROJECTCOUNT];		//子程序信息
	RealdataReq		RealDatareq;				//实时数据请求缓存


}ProgramInfo; //程序信息结构

//顺序
typedef enum { positive,/*正序*/inverted //倒序
} ORDER;

#endif /* OPTION_H_ */
