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

#pragma pack(1)				//结构体一个字节对齐
/////////////////////////////////////////////////////////////////////////
/********************************************************
 *				接口类公共属性结构
 ********************************************************/
typedef struct {
	INT8U logic_name[OCTET_STRING_LEN]; //逻辑名
	INT16U curr_num;					//当前元素个数
	INT16U max_num;					//最大元素个数
} CLASS11;			//集合接口类

typedef struct {
	INT8U logic_name[OCTET_STRING_LEN];			//逻辑名
	INT16U device_num;					 //设备对象数量
} CLASS22;			//输入输出设备接口类

typedef struct
{
	char factoryCode[4];	//厂商代码
	char softVer[4];		//软件版本号
	char softDate[6];		//软件版本日期
	char hardVer[4];		//硬件版本号
	char hardDate[6];		//硬件版本日期
	char factoryExpInfo[8];//厂家扩展信息
}VERINFO;

typedef struct
{
	char name[OCTET_STRING_LEN];		//逻辑名
	char devdesc[OCTET_STRING_LEN];		//设备描述符
	VERINFO info;						//版本信息
	DateTimeBCD date_Product;			//生产日期
	OI_698 ois[10];						//子设备列表
	char  protcol[OCTET_STRING_LEN];	//支持的规约列表
	INT8U follow_report;				//是否允许跟随上报
	INT8U active_report;				//是否允许主动上报
	INT8U talk_master;					//是否允许与主站通话
} CLASS19;					//设备管理接口类

typedef struct {
	INT8U workModel;					//工作模式 enum{混合模式(0),客户机模式(1),服务器模式(2)},
	INT8U onlineType;					//在线方式 enum{永久在线(0),被动激活(1)}
	INT8U connectType;					//连接方式 enum{TCP(0),UDP(1)}
	INT8U appConnectType;				//连接应用方式 enum{主备模式(0),多连接模式(1)}
	INT8U apn[OCTET_STRING_LEN];		//apn
	INT8U userName[OCTET_STRING_LEN];	//用户名称
	INT8U passWord[OCTET_STRING_LEN];	//密码
	INT8U proxyIp[OCTET_STRING_LEN];	//代理服务器地址
	INT16U proxyPort;					//代理端口
	INT8U timeoutRtry;					//超时时间，重发次数
	INT8U heartBeat;					//心跳周期秒
} COMM_CONFIG_1;
typedef struct {
	INT8U workModel;					//工作模式 enum{混合模式(0),客户机模式(1),服务器模式(2)},
	INT8U connectType;					//连接方式 enum{TCP(0),UDP(1)}
	INT8U appConnectType;				//连接应用方式 enum{主备模式(0),多连接模式(1)}
	INT16U listenPort[5];				//侦听端口列表
	INT8U proxyIp[OCTET_STRING_LEN];	//代理服务器地址
	INT16U proxyPort;					//代理端口
	INT8U timeoutRtry;					//超时时间，重发次数
	INT8U heartBeat;					//心跳周期秒
} COMM_CONFIG_2;
typedef struct {
	INT8U ip[20];						//主站 IP 192.168.000.001
	INT16U port;						//端口
} MASTER_STATION_INFO;
typedef struct {
	INT8U center[OCTET_STRING_LEN];		//短信中心号码
	INT8U master[4][OCTET_STRING_LEN];	//主站号码
	INT8U dest[4][OCTET_STRING_LEN];	//短信通知目的号码
} SMS_INFO;

typedef struct {
	INT8U name[OCTET_STRING_LEN];		//逻辑名
	COMM_CONFIG_1 commconfig;			//通信配置
	MASTER_STATION_INFO master[4];		//主站通信参数表
	SMS_INFO sms;						//短信通信参数表
	VERINFO info;						//版本信息
	INT8U protcol[10];					//支持的规约列表
	INT8U ccid[20];						//SIM卡CCID
	INT8U imsi[20];						//SIM卡IMSI
	INT16U signalStrength;				//信号强度
	INT8U pppip[20];					//拨号IP
} CLASS25;					//4500、4501公网通信模块1，2

typedef struct {
	INT8U ipConfigType;					//IP 配置方式 enum{DHCP(0),静态(1),PPPoE(2)}
	INT8U ip[OCTET_STRING_LEN];				//IP
	INT8U subnet_mask[OCTET_STRING_LEN];	//子网掩码
	INT8U gateway[OCTET_STRING_LEN];		//网关
	INT8U username_pppoe[OCTET_STRING_LEN];	//PPPOE用户名   sohu.com@yaxinli.com.cn
	INT8U password_pppoe[OCTET_STRING_LEN];	//PPPOE密码
} NETCONFIG;
typedef struct {
	INT8U name[OCTET_STRING_LEN];		//逻辑名
	COMM_CONFIG_2 commconfig;			//通信配置
	MASTER_STATION_INFO master[4];		//主站通信参数表
	NETCONFIG IP;						//终端IP
	INT8U mac[20];						//MAC地址
} CLASS26;						//以太网通信接口类

/////////////////////////////////////////////////////////////////////////////
/********************************************************
 *				 A.5 参变量类对象
 ********************************************************/
typedef struct {
	INT8U 	login_name[OCTET_STRING_LEN];		//逻辑名
	INT8U  	curstom_num[OCTET_STRING_LEN];	//客户编号
} CLASS_4001_4002_4003; 	//4001:通信地址，4002：表号，4003：客户编号

typedef struct {
	INT8U hour;						//时
	INT8U min;						//分
	INT8U rateno;						//费率号
} Day_Period;

typedef struct {
	INT8U num;
	Day_Period Period_Rate[MAX_PERIOD_RATE];
} CLASS_4016;
///////////////////////////////////////////////////////////////////////////////
/********************************************************
 *				A.7　采集监控类对象
 ********************************************************/

typedef struct {
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
} BASIC_OBJECT;
typedef struct {
	TSA cjq_addr;		//采集器地址
	INT8U asset_code[OCTET_STRING_LEN];		//资产号
	INT16U pt;
	INT16U ct;
} EXTEND_OBJECT;
typedef struct {
	OAD oad;
	INT8U data[OCTET_STRING_LEN];
} ANNEX_OBJECT;

typedef struct {
	INT8U name[OCTET_STRING_LEN];		//参数变量接口类逻辑名
	INT16U sernum;						//配置序号
	BASIC_OBJECT basicinfo;				//基本信息
	EXTEND_OBJECT extinfo;				//扩展信息
	ANNEX_OBJECT aninfo;				//附属信息
} CLASS_6001;				//采集档案配置表对象

typedef struct {
	INT8U beginHour;
	INT8U beginMin;
	INT8U endHour;
	INT8U endMin;
} TIMEPART;				//时段

typedef struct {
	INT8U type;   			            //运行时段类型
	INT8U num;
	TIMEPART runtime[24];       	    //时段表 0-3分别表示起始小时.分钟，结束小时.分钟
} TASK_RUN_TIME;

typedef struct {
	INT8U taskID;		                //参数变量接口类逻辑名
	TI interval;						//执行频率
	INT8U cjtype;						//方案类型
	INT8U sernum;						//方案序号
	DateTimeBCD startime;               //开始时间
	DateTimeBCD endtime;                //结束时间
	TI delay;						    //延时
	INT8U runprio;	                    //执行优先级
	INT8U state;    	                //任务状态
	INT16U befscript;                  //任务开始前脚本  //long unsigned
	INT16U aftscript;                  //任务完成后脚本  //long unsigned
	TASK_RUN_TIME runtime;              //任务运行时段
} CLASS_6013;              //任务配置单元

typedef struct {
	INT8U name[OCTET_STRING_LEN];		//参数变量接口类逻辑名
	INT8U sernum;						//方案序号
	INT16U deepsize;					//存储深度
	INT8U cjtype;						//采集类型
	DATA_TYPE data;						//采集内容
	CSD_ARRAYTYPE csds;					//记录列选择 array CSD,
	MY_MS mst;							//电能表集合
	INT8U savetimeflag;				//存储时标选择 enum
} CLASS_6015;				//普通采集方案

typedef struct {
//	INT8U name[OCTET_STRING_LEN];		//参数变量接口类逻辑名
	INT8U sernum;						//方案序号
	ROAD road[10];						//采集的事件数据
	MS ms;								//采集类型
	INT8U ifreport;						//上报标识
	INT16U deepsize;					//存储深度
} CLASS_6017;					//事件采集方案

typedef struct {
	INT8U taskID;		                //任务ID
	TASK_STATE taskState;				//任务执行状态
	DateTimeBCD starttime;              //任务结束结束时间
	DateTimeBCD endtime;                //任务结束结束时间
	INT16U totalMSNum;					//采集总数量
	INT16U successMSNum;				//采集成功数量
	INT16U sendMsgNum;					//发送报文数量
	INT16U rcvMsgNum;					//接受报文数量
} CLASS_6035;					//采集任务监控单元
///////////////////////////////////////////////////////////////////////////////
/********************************************************
 *				A.12　输入输出设备类对象
 ********************************************************/
typedef struct
{
	OI_698 oi;			//对象标识
	INT16U model;		//安全模式
}SecureModel;

typedef struct
{
	INT8U 		active;			//属性2：安全模式选择（0：不启用安全模式参数，1：启用安全模式）
	INT8U		modelnum;		//安全模式参数总个数
	SecureModel modelpara[255]; //属性3：显示安全模式参数
}CLASS_F101;//安全模式参数

typedef struct {
	INT8U ST;			//状态ST  0：“分”状态；1：“合”状态
	INT8U CD;			//变位CD  0：自前次遥信传送后无状态变化；1：自前次遥信传送后至少有一次状态变化。
} StateUnit;
typedef struct {
	INT8U num;
	StateUnit stateunit[STATE_MAXNUM];
} StateUnitArray;

typedef struct {
	INT8U StateAcessFlag;//bit-string(SIZE（8） bit0～bit7按顺序对位表示第1～8路状态量输入，置“1”：接入，置“0”：未接入。
	INT8U StatePropFlag;//bit-string(SIZE（8） bit0～bit7按顺序对位表示第1～8路状态量输入，置“1”常开触点。置“0”：常闭触点。
} StateAtti4;

typedef struct {
	CLASS22 class22;		//接口类IC
	StateUnitArray statearri;		//开关量单元属性2
	StateAtti4 state4;			//开关量属性
} CLASS_f203;			//开关量输入

typedef struct
{
	INT8U DI_1[4];
	INT8U DI_2[4];
}C601F_07Flag;

typedef struct {
	CSD flag698;
	C601F_07Flag flag07;
} CLASS_601F;			//开关量输入
#endif /* OBJECTACTION_H_ */
