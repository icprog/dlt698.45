/*
 * gtype.h
 *
 *  Created on: Sep 11, 2015
 *      Author: fzh
 */

#ifndef GTYPE_H_
#define GTYPE_H_

#include "StdDataType.h"
#include "EventObject.h"
#include "ParaDef.h"

#pragma pack(1)				//结构体一个字节对齐

typedef struct {
	INT8U	oi3100;				//终端初始化事件
	INT8U	oi3101;				//终端版本变更事件
	INT8U	oi3104;				//终端状态量变位事件
	INT8U	oi3105;				//电能表时钟超差事件
	INT8U	oi3106;				//终端停/上电事件
	INT8U	oi3107;				//终端直流模拟量越上限事件
	INT8U	oi3108;				//终端直流模拟量越下限事件
	INT8U	oi3109;				//终端消息认证错误事件
	INT8U	oi310A;				//设备故障记录
	INT8U	oi310B;				//电能表示度下降事件
	INT8U	oi310C;				//电能量超差事件
	INT8U	oi310D;				//电能表飞走事件
	INT8U	oi310E;				//电能表停走事件
	INT8U	oi310F;				//终端抄表失败事件
	INT8U	oi3110;				//月通信流量超限事件
	INT8U	oi3111;				//发现未知电能表事件
	INT8U	oi3112;				//跨台区电能表事件
	INT8U	oi3114;				//终端对时事件
	INT8U	oi3115;				//遥控跳闸记录
	INT8U	oi3116;				//有功总电能量差动越限事件记录
	INT8U	oi3117;				//输出回路接入状态变位事件记录
	INT8U	oi3118;				//终端编程记录
	INT8U	oi3119;				//终端电流回路异常事
	INT8U	oi311A;				//电能表在网状态切换事件
	INT8U	oi311B;				//终端对电表校时记录
	INT8U	oi311C;				//电能表数据变更监控记录
	INT8U	oi3200;				//功控跳闸记录
	INT8U	oi3201;				//电控跳闸记录
	INT8U	oi3202;				//购电参数设置记录
	INT8U	oi3203;				//电控告警事件记录
////////////////////////////////////////////////////////
	INT8U	oi4016;				//当前套日时段表
////////////////////////////////////////////////////////
	INT8U   oiF203;             //开关量
	INT8U oi6000;		/*采集档案配置表属性变更*/
	INT8U oi6002;		/*搜表类属性变更*/
	INT8U oi6012;		/*任务配置表属性变更*/
	INT8U oi6014;		/*普通采集方案集属性变更*/
	INT8U oi6016;		/*事件采集方案集属性变更*/
	INT8U oi6018;		/*透明方案集属性变更*/
	INT8U oi601C;		/*上报方案集属性变更*/
	INT8U oi601E;		/*采集规则库属性变更*/
	INT8U oi6051;		/*实时监控采集方案集属性变更*/
	INT8U oi4030;       //电压合格率统计
}OI_CHANGE;

//交采系数
typedef struct {
//	INT16U	crc;					//CRC校验
	INT8U PhaseA[3];				//相角系数
	INT8U PhaseB[3];
	INT8U PhaseC[3];
	INT8U PhaseA1[3];				//小电流相角系数
	INT8U PhaseB1[3];
	INT8U PhaseC1[3];
	INT8U PhaseA0[3];				//分段电流相角系数
	INT8U PhaseB0[3];
	INT8U PhaseC0[3];
	INT8U UA[3];					//电压系数
	INT8U UB[3];
	INT8U UC[3];
	INT8U IA[3];					//电流系数
	INT8U IB[3];
	INT8U IC[3];
	INT8U I0[3];
	INT8U PA[3];					//有功系数
	INT8U PB[3];
	INT8U PC[3];
	INT8U UoffsetA[3];					//电压有效值offset校正
	INT8U UoffsetB[3];
	INT8U UoffsetC[3];
	INT8U IoffsetA[3];					//电流有效值offset校正
	INT8U IoffsetB[3];
	INT8U IoffsetC[3];
	INT8U Tpsoffset[3];				//温度
	INT32U HarmUCoef[3];			//谐波电压系数，校表过程中，将读取的基波值/220.0得到系数
	INT32U HarmICoef[3];			//谐波电流系数，校表过程中，将读取的基波值/1.5得到系数
	INT32U	WireType;			//接线方式，0x1200：三相三，0x0600：三相四
}ACCoe_SAVE;

/*
 * 通过交采ATT7022E采样值计算的电能示值。电能示值是一个累计值，上电继续累加，保证不丢失。
 * 电能示值通过vsave保存到文件SAVE_NAME_ACDATA（"acenergy.dat"）
 * 所有结构体的电能示值= 实际值*ENERGY_COEF（6400）。
 * 系数具体定义参照att7022e.h
 * */
typedef struct {
//	INT16U	crc;					//CRC校验
	INT32U	PosPa_All;			  	//正向A相有功总电能示值
	INT32U	PosPb_All;			  	//正向B相有功总电能示值
	INT32U	PosPc_All;				//正向C相有功总电能示值
	INT32U	PosPt_All;				//正向有功总电能示值
	INT32U	PosPt_Rate[MAXVAL_RATENUM];	//正向有功总四费率电能示值

	INT32U	NegPa_All;				//反向A相有功总电能示值
	INT32U	NegPb_All;				//反向B相有功总电能示值
	INT32U	NegPc_All;				//反向C相有功总电能示值
	INT32U	NegPt_All;				//反向有功总电能示值
	INT32U	NegPt_Rate[MAXVAL_RATENUM];	//反向有功总四费率电能示值

	INT32U	PosQa_All;				//正向A相无功总电能示值
	INT32U	PosQb_All;				//正向B相无功总电能示值
	INT32U	PosQc_All;				//正向C相无功总电能示值
	INT32U	PosQt_All;				//正向无功总电能示值
	INT32U	PosQt_Rate[MAXVAL_RATENUM];	//正向无功总四费率电能示值

	INT32U	NegQa_All;				//反向A相无功总电能示值
	INT32U	NegQb_All;				//反向B相无功总电能示值
	INT32U	NegQc_All;				//反向C相无功总电能示值
	INT32U	NegQt_All;				//反向无功总电能示值
	INT32U	NegQt_Rate[MAXVAL_RATENUM];	//反向无功总四费率电能示值

	INT32U	Q1_Qt_All;				//一象限Quadrant无功总电能示值
	INT32U	Q1_Qt_Rate[MAXVAL_RATENUM];	//一象限费率1-4无功总电能示值
	INT32U	Q2_Qt_All;				//二象限无功总电能示值
	INT32U	Q2_Qt_Rate[MAXVAL_RATENUM];	//二象限费率1-4无功总电能示值
	INT32U	Q3_Qt_All;				//三象限无功总电能示值
	INT32U	Q3_Qt_Rate[MAXVAL_RATENUM];	//三象限费率1-4无功总电能示值
	INT32U	Q4_Qt_All;				//四象限无功总电能示值
	INT32U	Q4_Qt_Rate[MAXVAL_RATENUM];	//四象限费率1-4无功总电能示值
}ACEnergy_Sum;

typedef struct {
	INT32S  Pa ;	//有功功率
	INT32S  Pb ;
	INT32S  Pc ;
	INT32S  Pt ;

	INT32S  Qa ;	//无功功率
	INT32S  Qb ;
	INT32S  Qc ;
	INT32S  Qt ;

	INT32S  Sa ;	//视在功率
	INT32S  Sb ;
	INT32S  Sc ;
	INT32S  St ;

	INT32U  AvgUa ;	//1分钟平均电压
	INT32U  AvgUb ;
	INT32U  AvgUc ;

	INT32U  Ua ;	//电压
	INT32U  Ub ;
	INT32U  Uc ;

	INT32U  Ia ;	//电流
	INT32U  Ib ;
	INT32U  Ic ;
	INT32U  I0 ;

	INT32S  CosA;	//功率因数
	INT32S  CosB;
	INT32S  CosC;
	INT32S  Cos;

	INT32S  Freq; //频率

	INT32S  Pga; //A相电流与电压夹角
	INT32S  Pgb;//B相电流与电压夹角
	INT32S  Pgc;//C相电流与电压夹角

	INT32S 	YUaUb; //电压夹角	      Uab/Ua相位角
	INT32S  YUaUc;			    //Ub相位角
	INT32S  YUbUc;				//Ucb/Uc相位角
	INT32U LineUa;//基波电压有效值
	INT32U LineUb;
	INT32U LineUc;

	INT32U LineIa;//基波电流有效值
	INT32U LineIb;
	INT32U LineIc;

	INT32U	PFlag;				//有功、无功功率方向，正向为0，负相为1,
	INT32S	Temp;				//温度传感器值
	INT32U	SFlag;				//存放断相、相序、SIG标志状态,第三位为1电压相序错,第四位为1电流相序错,
	INT8U Available;            //交采开机后是否已经采到有效数据
}_RealData;

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
typedef struct{
	DateTimeBCD dt;  //时间
	INT8U type;      //校时标志
	INT8U totalnum;  //最近心跳时间总个数
	INT8U maxn;      //最大值剔除个数
	INT8U minn;      //最小值剔除个数
	INT8U timeoffset;//通讯延时数值 秒
	INT8U lastnum;   //最少有效个数
}Terminal_timeoffset;
typedef struct
{
	FP32 RPS;      //当前总加有功功率  1
	FP32 PQS;		//当前总加无功功率
	FP64 DESP;   	//当日总加有功总电能
	FP64 DESP_m1;
	FP64 DESP_m2;
	FP64 DESP_m3;
	FP64 DESP_m4;
	FP64 DESQ;    //当日总加无功总电能
	FP64 DESQ_m1;
	FP64 DESQ_m2;
	FP64 DESQ_m3;
	FP64 DESQ_m4;
	FP64 MESP;    //当月总加有功总电能
	FP64 MESP_m1;
	FP64 MESP_m2;
	FP64 MESP_m3;
	FP64 MESP_m4;
	FP64 MESQ;	//当月总加无功总电能
	FP64 MESQ_m1;
	FP64 MESQ_m2;
	FP64 MESQ_m3;
	FP64 MESQ_m4;
	FP64   RER;		//剩余电量
	FP64   DESP_before1min;//一分钟前的电能示值
	FP64   RER_beforectrl;//控前电量(液晶显示用)
}DATA_CALC_BY1MIN;  //一分钟一统计的数据
typedef struct
{
	FP64 	JC_ZPXL_Curr_Data;//当前正向有功最大需量
	INT8U 	JC_ZPXL_Curr_Time[4];//发生时间  字节[0]:分钟 [1]:小时 [2]:日 [3]月
	FP64 	JC_ZPXL_Curr_Data_FL[4];//当前正向有功费率最大需量
	INT8U 	JC_ZPXL_Curr_Time_FL[4][4];//发生时间  字节[0]:分钟 [1]:小时 [2]:日 [3]月

	FP64 	JC_FPXL_Curr_Data;//当前反向有功最大需量
	INT8U 	JC_FPXL_Curr_Time[4];//发生时间  字节[0]:分钟 [1]:小时 [2]:日 [3]月
	FP64 	JC_FPXL_Curr_Data_FL[4];//当前反向有功最大需量
	INT8U 	JC_FPXL_Curr_Time_FL[4][4];//发生时间  字节[0]:分钟 [1]:小时 [2]:日 [3]月

	FP64 	JC_ZQXL_Curr_Data;//当前正向无功最大需量
	INT8U 	JC_ZQXL_Curr_Time[4];//发生时间  字节[0]:分钟 [1]:小时 [2]:日 [3]月
	FP64 	JC_ZQXL_Curr_Data_FL[4];//当前正向无功最大需量
	INT8U 	JC_ZQXL_Curr_Time_FL[4][4];//发生时间  字节[0]:分钟 [1]:小时 [2]:日 [3]月

	FP64 	JC_FQXL_Curr_Data;//当前反向无功最大需量
	INT8U 	JC_FQXL_Curr_Time[4];//发生时间  字节[0]:分钟 [1]:小时 [2]:日 [3]月
	FP64 	JC_FQXL_Curr_Data_FL[4];//当前反向无功最大需量
	INT8U 	JC_FQXL_Curr_Time_FL[4][4];//发生时间  字节[0]:分钟 [1]:小时 [2]:日 [3]月
}PUB_JC_Data;//存放交采临时数据
typedef struct{
	INT32U s_Rate;//电压上限率
	INT32U x_Rate;//电压下限率
	INT32U ok_Rate;//电压合格率
	INT16U ss_count;					//相电压越上上限累计时间    (min)
	INT16U xx_count;					//相电压越下下限累计时间    (min)
	INT16U s_count;						//相电压越上限累计时间      (min)
	INT16U x_count;						//相电压越下限累计时间 	(min)
	INT16U ok_count;					//相电压合格限累计时间 	(min)
	INT16U max;							//相电压最大值
	INT16U min;							//相电压最小值
	INT16U U_Avg;//一天当中相电压的平均电压
	INT8U max_time[3];			//相电压最大值发生时间
	INT8U min_time[3];			//相电压最小值发生时间

	INT32U U_Sum;//一天当中相电压累加和，用于计算平均电压
	INT32U U_Count;//一天当中相电压累加时间

	INT8U tmp[2];//字节对齐位
}Statistics_U;//电压统计结果

typedef struct{
	Statistics_U tjUa;						//A相电压统计								二类数据F27
	Statistics_U tjUb;						//B相电压统计
	Statistics_U tjUc;						//C相电压统计
}StatisticsInfo;//三相电压统计结果

typedef struct{
	StatisticsInfo DayResu; //日统计电压结果
	StatisticsInfo MonthResu;//月统计电压结果
	INT32U PointNo; //测量点
}StatisticsPointProp;//测量点统计结果
//电能量值 41
typedef struct{
	INT16U pointno;
	BOOLEAN Valid;
	INT32S z_Psz_energy_all;
	INT32S z_Psz_energy[MAXVAL_RATENUM];//实时正向有功总电能  41  45
	INT32S f_Psz_energy_all;
	INT32S f_Psz_energy[MAXVAL_RATENUM];//实时反向有功总电能  43  47
	INT32S z_Qsz_energy_all;
	INT32S z_Qsz_energy[MAXVAL_RATENUM];//实时正向无功总电能 42  46
	INT32S f_Qsz_energy_all;
	INT32S f_Qsz_energy[MAXVAL_RATENUM];//实时反向无功总电能  44  48
}ENERGY_PROPERTY_SET;
typedef struct {
	INT32U 			ac_chip_type; 		//==0x820900:	RN8029芯片，III型集中器	//==1： ATT7022D-E芯片 	//==0x7022E0:	ATT7022E-D芯片
	INT32U			WireType;			//接线方式，0x1200：三相三，0x0600：三相四
	_RealData		ACSRealData;		//计量芯片实时数据
	ACEnergy_Sum	ACSEnergy;			//计量芯片电能量数据
	ProjectInfo		Projects[PROJECTCOUNT];	//子程序信息
	RealdataReq		RealDatareq;			//实时数据请求缓存
	OI_CHANGE		oi_changed;				//相应的OI参数修改变化值，结构体相应的OI值从1-255设置参数后循环累加
	TerminalEvent_Object event_obj;         //事件参数结构体
	FactoryVersion  version;				//终端版本信息
	Terminal_timeoffset t_timeoffset;    	//终端精准校时参数
	DATA_CALC_BY1MIN data_calc_by1min[MAXNUM_SUMGROUP];//一分钟一统计的数据
	PUB_JC_Data jc_data;                               //存放交采临时数据
	StatisticsPointProp StatisticsPoint[MAXNUM_IMPORTANTUSR];//测量点日月电压统计结果
	ENERGY_PROPERTY_SET ENERGY_PROPERTY_DAY[MAXNUM_IMPORTANTUSR]; //日电能量
	ENERGY_PROPERTY_SET ENERGY_PROPERTY_MONTH[MAXNUM_IMPORTANTUSR];//月电能量
}ProgramInfo; //程序信息结构

#endif /* GTYPE_H_ */
