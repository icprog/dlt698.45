/*
 * ObjectAction.h
 *
 *  Created on: Nov 12, 2016
 *      Author: ava
 */

#ifndef OBJECTACTION_H_
#define OBJECTACTION_H_
#include "ParaDef.h"
#include "StdDataType.h"
int doObjectAction();

#pragma pack(1)				//结构体一个字节对齐
/////////////////////////////////////////////////////////////////////////
/********************************************************
 *				接口类公共属性结构
 ********************************************************/
typedef struct//集合接口类
{
	INT8U 	logic_name[OCTET_STRING_LEN];//逻辑名
	INT16U 	curr_num;					//当前元素个数
	INT16U 	max_num;					//最大元素个数
}COLL_CLASS_11;

/////////////////////////////////////////////////////////////////////////
typedef struct
{
	TSA addr;			//通信地址
	INT8U baud;			//波特率
	INT8U protocol;		//规约类型
	OAD port;			//端口
	INT8U pwd[OCTET_STRING_LEN];	//通信密码
	INT8U ratenum;		//费率个数
	INT8U usrtype;		//用户类型
	INT8U connectype;	//接线方式
	INT16U ratedU;		//额定电压
	INT16U ratedI;		//额定电流
}BASIC_OBJECT;
typedef struct
{
	TSA cjq_addr;		//采集器地址
	INT8U asset_code[OCTET_STRING_LEN];		//资产号
	INT16U pt;
	INT16U ct;
}EXTEND_OBJECT;
typedef struct
{
	OAD oad;
	INT8U data[OCTET_STRING_LEN];
}ANNEX_OBJECT;

typedef struct
{
	INT8U name[OCTET_STRING_LEN];		//参数变量接口类逻辑名
	INT16U sernum;						//配置序号
	BASIC_OBJECT basicinfo;				//基本信息
	EXTEND_OBJECT extinfo;				//扩展信息
	ANNEX_OBJECT aninfo;				//附属信息
}CLASS_6001;//采集档案配置表对象
typedef struct
{
	RUN_TIME_TYPE type;                //运行时段类型
	INT8U runtime[4];                  //时段 0-3分别表示起始小时.分钟，结束小时.分钟
}TASK_RUN_TIME;

typedef struct
{
	INT8U 	hour;		//时
	INT8U	min;		//分
	INT8U	rateno;		//费率号
}Day_Period;

typedef struct
{
	Day_Period Period_Rate[MAX_PERIOD_RATE];
}CLASS_4016;

typedef struct
{
	INT8U taskID;		                //参数变量接口类逻辑名
	TI interval;						//执行频率
	INT16U deepsize;					//存储深度	// pdf中没有定义
	SCHM_TYPE  cjtype;					//方案类型
	INT8U sernum;						//方案序号
	DateTimeBCD startime;               //开始时间
	DateTimeBCD endtime;                //结束时间
	TI delay;						    //延时
	RUN_PRIO runprio;                   //执行优先级
	TASK_VALID state;                   //任务状态
	INT8U  befscript;                   //任务开始前脚本  //long unsigned
	INT8U  aftscript;                   //任务完成后脚本  //long unsigned
	TASK_RUN_TIME runtime;              //任务运行时段
}CLASS_6013;//任务配置单元

typedef struct
{
	INT8U name[OCTET_STRING_LEN];		//参数变量接口类逻辑名
	INT8U sernum;						//方案序号
	INT16U deepsize;					//存储深度
	INT8U  cjtype;						//采集类型
	DATA_TYPE data;						//采集内容
	INT8U csdtype;
	CSD  csd[10];						//记录列选择 array CSD,
	MS     ms;							//电能表集合
	INT8U  savetimeflag;				//存储时标选择 enum
}CLASS_6015;//普通采集方案

typedef struct
{
	INT8U name[OCTET_STRING_LEN];		//参数变量接口类逻辑名
	INT16U sernum;						//方案序号
	ROAD road[10];						//采集的事件数据
	MS  ms;								//采集类型
	INT8U ifreport;						//上报标识
	INT16U  deepsize;					//存储深度
}CLASS_6017;//事件采集方案

typedef struct
{
	INT8U taskID;		                //任务ID
	TASK_STATE taskState;				//任务执行状态
	DateTimeBCD starttime;                //任务结束结束时间
	DateTimeBCD endtime;                //任务结束结束时间
	INT16U totalMSNum;					//采集总数量
	INT16U successMSNum;				//采集成功数量
	INT16U sendMsgNum;					//发送报文数量
	INT16U rcvMsgNum;					//接受报文数量
}CLASS_6035;//采集任务监控单元

#endif /* OBJECTACTION_H_ */
