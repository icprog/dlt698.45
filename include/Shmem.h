/*
 * gtype.h
 *
 *  Created on: Sep 11, 2015
 *      Author: fzh
 */

#ifndef GTYPE_H_
#define GTYPE_H_

#include "StdDataType.h"
typedef struct
{
	INT8U VendorCode[2];	//厂商代码（VendorCode[1~0]：TC=鼎信；ES=东软；LH=力合微；37=中瑞昊天）
	INT8U ChipCode[2];		//芯片代码
	INT8U VersionDay;		//版本日期-日
	INT8U VersionMonth;		//版本日期-月
	INT8U VersionYear;		//版本日期-年
	INT8U Version[2];		//版本
}ZB_Info;	//厂商代码和版本信息
typedef struct
{
	INT8U chg6000;		/*采集档案配置表属性变更*/
	INT8U chg6002;		/*搜表类属性变更*/
	INT8U chg6012;		/*任务配置表属性变更*/
	INT8U chg6014;		/*普通采集方案集属性变更*/
	INT8U chg6016;		/*事件采集方案集属性变更*/
	INT8U chg6018;		/*透明方案集属性变更*/
	INT8U chg601C;		/*上报方案集属性变更*/
	INT8U chg601E;		/*采集规则库属性变更*/
	INT8U chg6051;		/*实时监控采集方案集属性变更*/
}PARA_CHG;
typedef struct
{
	INT8U online_state;	/*gprs在线1 ，cdma在线2 ，FDD_LTE在线3 ，TD_LTE在线4 ，以太网在线5 ，正在拨号中6 */
	INT8U csq;			/*信号强度*/
	INT8U pppip[16];	/*无线IP*/
	INT8U sa[16];		/*集中器地址*/
	INT8U yx[4];		/*遥信状态*/
	ZB_Info zbinfo;		/*载波模块信息*/
	INT8U zbstatus;		/*载波抄表状态*/
	INT8U jiexian;		/*交采接线类型*/
	PARA_CHG parachg;	/*参数变更标识*/
}TerminalMemType;

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

#endif /* GTYPE_H_ */
