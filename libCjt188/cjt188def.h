/*
 * cj188def.h
 *
 *  Created on: Sep 25, 2015
 *      Author: lhl
 */

#ifndef CJT188DEF_H_
#define CJT188DEF_H_

#define CJ188_MAXSIZE 	1024

/****错误码定义*****************/
#define ERR_RCVD_LOST       -1  		//接收报文不完整
#define ERR_LOST_0x68     	-2	        //报文开头0x68丢失
#define ERR_LOST_0x16		-3			//报文结束0x16丢失
#define	ERR_SUM_ERROR		-4			//总加和校验失败
#define ERR_METERTYPE		-5			//仪表类型不支持
#define ERR_MONTH			-6			//错误历史月

#define T_WATER_COOL	0x10		//冷水水表
#define T_WATER_HEAT	0x11		//生活热水水表
#define T_WATER_DRINK	0x12		//直饮水水表
#define T_WATER_MIDDLE	0x13		//中水水表
#define T_HOT_HEAT		0x20		//热量表(计热表)
#define T_HOT_COOL		0x21		//热量表(计冷表)
#define T_GAS			0x30		//燃气表
#define T_METER			0x40		//电度表

#define READ_DATA			0x01		//读数据				//加密传输
#define READ_KEY_VER		0x09		//读密钥版本号			//明码传输
#define	READ_ADDR			0x03		//读地址				//单机通讯,明码传输

#define WRITE_DATA			0x04		//写数据				//加密传输
#define WRITE_ADDR			0x15		//写地址				//调试阶段明码传输,收到出厂启用命令后不再响应
#define	WRITE_SYNC_DATA		0x16		//写机电同步数据		//调试阶段明码传输,收到出厂启用命令后不再响应


typedef struct
{
	INT8U	MeterType;		//仪表类型
	INT8U 	Addr[7];		//表地址
	INT8U 	Ctrl;			//控制码
	INT8U 	Length;			//数据域长度
	INT8U 	DI[2];			//数据项标识
	INT8U 	SER;			//帧序号
	INT8U 	Data[256];		//数据内容
}cj188_Frame;

//写价格表
typedef struct
{
	INT8U	Bcd_Price1[3];		//价格1,(元/单位用量)
	INT8U 	Bcd_Amount1[3];		//用量1
	INT8U	Bcd_Price2[3];		//价格2,(元/单位用量)
	INT8U 	Bcd_Amount2[3];		//用量2
	INT8U	Bcd_Price3[3];		//价格3,(元/单位用量)
	INT8U	Bcd_Date;			//启用日期
}DI_10;

//写结算日
typedef struct
{
	INT8U	Bcd_Date;			//结算日期
}DI_11;

//写抄表日
typedef struct
{
	INT8U	Bcd_Date;			//抄表日期
}DI_12;

//写购入金额
typedef struct
{
	INT8U	Hex_Serial;			//购买序号
	INT8U	Bcd_Price[4];		//购买金额 XXXXXX.XX元
}DI_13;

//写新密钥
typedef struct
{
	INT8U	Hex_Key_Ver;		//新密钥版本号
	INT8U	Bcd_Key[8];			//新密钥
}DI_14;

//写标准时间
typedef struct
{
	INT8U	Bcd_Date[7];		//实时时间 YYYYMMDDhhmmss
}DI_15;

//写阀门控制
typedef struct
{
	INT8U	Hex_Status[2];		//阀门状态ST
}DI_17;

//写地址
typedef struct
{
	INT8U	Bcd_Addr[7];		//地址A0-A6
}DI_18;

//写机电同步数据
typedef struct
{
	INT8U	Bcd_TotalFlow[5];		//当前累计流量
}DI_16;

//参数数据
typedef struct
{
	DI_10	di10;
	DI_11	di11;
	DI_12	di12;
	DI_13	di13;
	DI_14	di14;
	DI_15	di15;
	DI_17	di17;
	DI_18	di18;
	DI_16	di16;
}cj188_Para;

typedef struct
{
	INT32U	Year;
	INT32U	Month;
	INT32U	Day;
	INT32U	Hour;
	INT32U	Minute;
	INT32U	Second;
}RealTime;

//19+2(DI)+1(SER) = 16H
typedef struct
{
	INT8U	totalflow[4];			//当前累积流量
	INT8U	totalflow_unit;			//当前累积流量	 单位
	INT8U	dayflow[4];				//结算日累积流量
	INT8U	dayflow_unit;			//结算日累积流量 单位
	RealTime	realtime;			//实时时间
	INT8U	status[2];				//状态ST
}curr_Water_Gos;

//len= 43+2(DI)+1(SER) = 46_2EH
typedef struct
{
	INT8U	dayhot[4];				//结算日热量
	INT8U	dayhot_unit;			//结算日热量 单位
	INT8U	currhot[4];				//当前热量
	INT8U	currhot_unit;			//当前热量 单位
	INT8U	hotpower[4];			//热功率
	INT8U	hotpower_unit;			//热功率 单位
	INT8U	flow[4];				//流量
	INT8U	flow_unit;				//流量 单位
	INT8U	totalflow[4];			//累积流量
	INT8U	totalflow_unit;			//累积流量 单位
	INT8U	servewatertemp[3];		//供水温度
	INT8U	backwatertemp[3];		//回水温度
	INT8U	worktime[3];			//累积工作时间
	RealTime	realtime;			//实时时间
	INT8U	status[2];				//状态ST
}curr_Hot;

typedef struct{
	INT8U	dayflow[4];				//结算日累积流量
	INT8U	dayflow_unit;			//结算日累积流量 单位
}watergos_Data;

typedef struct{
	watergos_Data	HisData[12];		//上12个月计量数据
}month_Water_Gos;

typedef struct{
	INT8U	dayhot[4];				//结算日累积热量
	INT8U	dayhot_unit;			//结算日累积热量 单位
}hot_Data;

typedef struct{
	hot_Data	HisData[12];		//上12个月计量数据
}month_Hot;

//价格表
typedef struct{
	INT8U	price1[3];				//价格1,(元/单位用量)
	INT8U 	amount1[3];				//用量1
	INT8U	price2[3];				//价格2,(元/单位用量)
	INT8U 	amount2[3];				//用量2
	INT8U	price3[3];				//价格3,(元/单位用量)
}price_Table;

//结算日/抄表日
typedef struct{
	INT8U	account;				//结算日
	INT8U	readmeter;				//抄表日
}meter_Day;

//购入金额
typedef	struct{
	INT8U	serial;					//购入序号
	INT8U	thismoney[4];			//本次购入金额
	INT8U	growmoney[4];			//累计购入金额
	INT8U	restmoney[4];			//剩余金额
	INT8U	status[2];				//状态ST
}buy_Money;

#endif /* CJT188DEF_H_ */
