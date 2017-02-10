#ifndef EVENTOBJECT_H_
#define EVENTOBJECT_H_

#include "ParaDef.h"
#include "StdDataType.h"

#pragma pack(1)				//结构体一个字节对齐

/*
 * 特殊终端事件:Event3105_Object,Event3106_Object,Event3107_Object,Event3108_Object,
 *            Event310B_Object,Event310C_Object,Event310D_Object,Event310E_Object,
 *            Event310F_Object,Event3110_Object,Event3116_Object,Event311A_Object,
 *            Event311C_Object
 * 其他终端事件可按Class7_Object进行定义！
 */
//目前已确定终端判断30个事件，可根据以下数组结构轮寻读取
//const static OI_698  event_oi[EVENT_OI_MAXNUM] ={
//		 0x3100,0x3101,0x3104,0x3105,0x3106,0x3107,0x3108,0x3109,0x310A,0x310B,
//		 0x310C,0x310D,0x310E,0x310F,0x3110,0x3111,0x3112,0x3114,0x3115,0x3116,
//		 0x3117,0x3118,0x3119,0x311A,0x311B,0x311C,0x3200,0x3201,0x3202,0x3203
//};

//设备故障记录 事件发生源
typedef enum
{
    memory_err=0,    //终端主板内存故障（0）
    clock_err=1,     //时钟故障（1）
    comm_err=2,      //主板通信故障（2）
    c485_err=3,      //485抄表故障（3）
    show_err=4,      //显示板故障（4）
    plc_err=5        //载波通道异常（5）
}MachineError_type;

//终端电流回路异常事件 事件发生源
typedef enum
{
  shortcircuit=0,   //短路（0）
  opencircuit=1    //开路（1）
}CurrentError_type;

//累计时间及发生次数
typedef struct
{
	INT16U nums;        //事件发生次数
	INT16U times;       //事件累计时间
}Timesnums_Object;

//当前数值记录表
//typedef struct
//{
//	INT8U num;
//    INT8U source[TSA_LEN];
//    Timesnums_Object tims_nums;
//}Crrent_Object;

typedef struct
{
	INT8U	num;
	OAD     oadarr[CLASS7_OAD_NUM];
}Class7_OAD;

//class_7 事件类通用结构体
typedef struct
{
	OI_698 oi;           	//逻辑名
	INT16U crrentnum;    	//当前记录数
	INT16U maxnum;       	//最大记录数
	BOOLEAN reportflag;  	//上报标识 1 上报 0 不上报
	BOOLEAN enableflag;  	//有效标识 1 有效 0 无效
	Class7_OAD	class7_oad;  //关联对象属性表			//放结构体后面为扩展
	//Crrent_Object crrent_arr[5]; //当前值记录表
}Class7_Object;

//电能表时钟超差事件参数
typedef struct
{
  INT16U over_threshold; //判断阀值(单位 秒)
  INT8U task_no;         //关联采集任务号
}MeterTimeOver_Object;

//停电书记采集配置参数
typedef struct
{
  INT8U collect_flag;   //采集标志(bit0 置1有效，置0无效 bit1 置1随机选择测量点，置0只采集设置对测量点。)
  INT8U time_space;     //停电事件抄读时间间隔（单位 小时）
  INT8U time_threshold; //停电事件抄读时间限值（单位 分钟）
  TSA meter_tas[5];     //需要抄读停电事件电能表
}Collect_Para_Object;

//停电事件甄别限值参数
typedef struct
{
  INT16U mintime_space; //停电时间最小有效间隔（分钟）
  INT16U maxtime_space; //停电时间最大有效间隔（分钟）
  INT16U startstoptime_offset; //停电事件起止时间偏差限值（分钟）
  INT16U sectortime_offset;   //停电事件时间区段偏差限值（分钟）
  INT16U happen_voltage_limit; //停电发生电压限值(单位V 换算 -1)
  INT16U recover_voltage_limit;//停电恢复电压限值(单位V 换算 -1)
}Screen_Para_Object;

//停上电事件配置参数
typedef struct
{
	Collect_Para_Object collect_para_obj;
	Screen_Para_Object screen_para_obj;
}PowerOff_Para_Object;

//终端直流模拟量越上限事件参数配置
typedef struct
{
  INT32S top_limit; //直流模拟量上限
}AnalogTop_Object;

//终端直流模拟量越下限事件参数配置
typedef struct
{
  INT32S bom_limit; //直流模拟量下限
}AnalogBom_Object;

//关联采集任务号
typedef struct
{
 INT8U task_no; //关联采集任务号
}Meter_Down_Object;

//电能表时钟超差事件
typedef struct
{
  Class7_Object event_obj;      //class7事件类
  MeterTimeOver_Object mto_obj; //电能表时钟超差事件参数
}Event3105_Object;

//停上电事件
typedef struct
{
	Class7_Object event_obj;      //class7事件类
	PowerOff_Para_Object poweroff_para_obj;
}Event3106_Object;

//终端直流模拟量越上限事件
typedef struct
{
	Class7_Object event_obj;      //class7事件类
	AnalogTop_Object analogtop_obj;
}Event3107_Object;

//终端直流模拟量越下限事件
typedef struct
{
	Class7_Object event_obj;      //class7事件类
	AnalogBom_Object analogbom_obj;
}Event3108_Object;

//电能表示度下降事件
typedef struct
{
	Class7_Object event_obj;      //class7事件类
	Meter_Down_Object meter_down_obj;
}Event310B_Object;

//电能量超差、飞走事件配置参数
typedef struct
{
  INT32U power_offset;  //阀值（单位% 无换算）
  INT8U  task_no; //关联采集任务号
}PowerOffset_Object;

//电能量超差事件
typedef struct
{
	Class7_Object event_obj;      //class7事件类
	PowerOffset_Object poweroffset_obj;
}Event310C_Object;

//电能量飞走事件
typedef struct
{
	Class7_Object event_obj;      //class7事件类
	PowerOffset_Object poweroffset_obj;
}Event310D_Object;

//电能量停走事件配置参数
typedef struct
{
  TI power_offset;  //阀值
  INT8U  task_no; //关联采集任务号
}PowerStopPara_Object;

//电能量停走事件
typedef struct
{
	Class7_Object event_obj;      //class7事件类
	PowerStopPara_Object powerstoppara_obj;
}Event310E_Object;

//终端抄表失败事件配置参数
typedef struct
{
  INT8U retry_nums; //重试论次
  INT8U task_no;     //关联采集任务号
}CollectFail_Object;

//终端抄表失败事件
typedef struct
{
	Class7_Object event_obj;      //class7事件类
	CollectFail_Object collectfail_obj;
}Event310F_Object;

//月通信流量超限事件配置参数
typedef struct
{
   INT32U month_offset;           //通信流量门限  double-long-unsigned（单位：byte）
}Monthtrans_Object;

//月通信流量超限事件
typedef struct
{
	Class7_Object event_obj;      //class7事件类
	Monthtrans_Object Monthtrans_obj;
}Event3110_Object;

//有功总电能量差动越限事件记录配置参数
typedef struct
{
	INT8U group_no;             //有功总电能量差动组序号
	OI_698 contrast_group;          //对比的总加组
	OI_698 consult_group;           //参照的总加组
	INT8U flag;             	//参与差动的电能量的时间区间及对比方法标志 bit-string（SIZE（8））
	                            //bit0～bit1编码表示电能量的时间跨度，取值范围0～2依次表示60分钟电量、30分钟电量、15分钟电量，其他值无效。
								//bit7表示对比方法标志，置“0”：相对对比，公式见公式（1）；置“1”：绝对对比，公式见公式（2）。
								//bit2～bit6备用。
								// ％  .....	(1)
								//.............	(2)
								//式中：
								//Q——对比的总加组总电能量；
								//q——参照的总加组总电能量。
    INT8S relative_offset;      //差动越限相对偏差值 integer（单位：%，换算：0）
    INT64S absolute_offset;     //差动越限绝对偏差值 long64（单位：kWh，换算：-4）
}PowChadongPara_Object;

//有功总电能量差动越限事件
typedef struct
{
	Class7_Object event_obj;      //class7事件类
	PowChadongPara_Object powchadongpara_obj[20];
}Event3116_Object;

//电能表在网状态切换事件配置参数
typedef struct
{
	INT16U outtime_offset;     //判定延时时间long-unsigned（单位：s，换算：0）
}OuttimePara_Object;

//电能表在网状态切换事件
typedef struct
{
	Class7_Object event_obj;      //class7事件类
	OuttimePara_Object outtimepara_obj;
}Event311A_Object;

//电能表数据变更监控记录
typedef struct
{
	Class7_Object event_obj;      //class7事件类
	Meter_Down_Object task_para;  //关联任务号
}Event311C_Object;

//通道上报状态
typedef struct
{
   OAD channel_oad;
   INT8U report_flag;              //上报状态：
								   //bit0:事件发生上报标识，0—未上报，1—已上报；
								   //bit1:事件发生上报确认标识，0—未确认，1—已确认；
								   //bit2:事件结束（恢复）上报标识，0—未上报，1—已上报；
								   //bit3:事件结束（恢复）上报确认标识，0—未确认，1—已确认。
}Channel_Object;

//通用事件记录单元
typedef struct
{
	INT32U id;
	DateTimeBCD begin;
	DateTimeBCD end;
	INT8U source[32];			//事件发生源
	Channel_Object report;		//上报状态
	OAD others[8];				//关联属性
}E3301_Object;

typedef struct{
	INT32U value;
	INT8U Available;
}MYINT32U ;

typedef struct{
	MYINT32U Voltage[3];
	MYINT32U Current[3];
	MYINT32U Power[3];
}EVENTREALDATA;

/*以下为整个事件属性结构体，存储该结构体*/
typedef struct
{
	Class7_Object Event3100_obj;    //终端初始化事件1
	Class7_Object Event3101_obj;    //终端版本变更事件2
	Class7_Object Event3104_obj;    //终端状态量变位事件3
	Event3105_Object Event3105_obj; //电能表时钟超差事件4
	Event3106_Object Event3106_obj; //终端停/上电事件5
	Event3107_Object Event3107_obj; //终端直流模拟量越上限事件6
	Event3108_Object Event3108_obj; //终端直流模拟量越下限事件7
	Class7_Object Event3109_obj;    //终端消息认证错误事件8
	Class7_Object Event310A_obj;    //设备故障记录9
	Event310B_Object Event310B_obj; //电能表示度下降事件10
	Event310C_Object Event310C_obj; //电能量超差事件11
	Event310D_Object Event310D_obj; //电能表飞走事件12
	Event310E_Object Event310E_obj; //电能表停走事件13
	Event310F_Object Event310F_obj; //终端抄表失败事件14
	Event3110_Object Event3110_obj; //月通信流量超限事件15
	Class7_Object Event3111_obj;    //发现未知电能表事件16
	Class7_Object Event3112_obj;    //跨台区电能表事件17
	Class7_Object Event3114_obj;    //终端对时事件18
	Class7_Object Event3115_obj;    //遥控跳闸记录19
	Event3116_Object Event3116_obj; //有功总电能量差动越限事件记录20
	Class7_Object Event3117_obj;    //输出回路接入状态变位事件记录21
	Class7_Object Event3118_obj;    //终端编程记录22
	Class7_Object Event3119_obj;    //终端电流回路异常事件23
	Event311A_Object Event311A_obj; //电能表在网状态切换事件24
	Class7_Object Event311B_obj;    //终端对电表校时记录25
	Event311C_Object Event311C_obj; //电能表数据变更监控记录26
	Class7_Object Event3200_obj;    //功控跳闸记录27
	Class7_Object Event3201_obj;    //电控跳闸记录28
	Class7_Object Event3202_obj;    //购电参数设置记录29
	Class7_Object Event3203_obj;    //电控告警事件记录30
}TerminalEvent_Object;

/*
 * Class7_Object Event3100_obj;    //终端初始化事件1
 * Class7_Object Event3109_obj;    //终端消息认证错误事件8
 * Class7_Object Event310A_obj;    //设备故障记录9
 * Event3110_Object Event3110_obj; //月通信流量超限事件15
 * Class7_Object Event3114_obj;    //终端对时事件18
 * Class7_Object Event3202_obj;    //购电参数设置记录29
 *
 */

/*
 * Event3105_Object Event3105_obj; //电能表时钟超差事件4
 *
 * Class7_Object Event310A_obj;    //设备故障记录9
 * Event310B_Object Event310B_obj; //电能表示度下降事件10
 * Event310C_Object Event310C_obj; //电能量超差事件11
 * Event310D_Object Event310D_obj; //电能表飞走事件12
 * Event310E_Object Event310E_obj; //电能表停走事件13
 * Event310F_Object Event310F_obj; //终端抄表失败事件14
 * Class7_Object Event3111_obj;    //发现未知电能表事件16
 * Class7_Object Event3112_obj;    //跨台区电能表事件17
 * Event311A_Object Event311A_obj; //电能表在网状态切换事件24
 * Class7_Object Event311B_obj;    //终端对电表校时记录25
 * Event311C_Object Event311C_obj; //电能表数据变更监控记录26
 */

/*
 * Event3106_Object Event3106_obj; //终端停/上电事件5
 * Event3107_Object Event3107_obj; //终端直流模拟量越上限事件6
 * Event3108_Object Event3108_obj; //终端直流模拟量越下限事件7
 * Class7_Object Event310A_obj;    //设备故障记录9
 * Class7_Object Event3119_obj;    //终端电流回路异常事件23
 */

typedef struct
{
	OI_698 		oi;  			//对象标识OI
	INT16U		classlen;	   //事件参数类长度
}EVENT_CLASS_INFO;

//目前已确定终端判断30个事件，可根据以下数组结构轮寻读取
const static EVENT_CLASS_INFO  event_class_len[] ={
		{0x3100,sizeof(Class7_Object)},		{0x3101,sizeof(Class7_Object)},		{0x3104,sizeof(Class7_Object)},		{0x3105,sizeof(Event3105_Object)},	{0x3106,sizeof(Event3106_Object)},
		{0x3107,sizeof(Event3107_Object)},	{0x3108,sizeof(Event3108_Object)},	{0x3109,sizeof(Class7_Object)},		{0x310A,sizeof(Class7_Object)},		{0x310B,sizeof(Event310B_Object)},
		{0x310C,sizeof(Event310C_Object)},	{0x310D,sizeof(Event310D_Object)},	{0x310E,sizeof(Event310E_Object)},	{0x310F,sizeof(Event310F_Object)},	{0x3110,sizeof(Event3110_Object)},
		{0x3111,sizeof(Class7_Object)},		{0x3112,sizeof(Class7_Object)},		{0x3114,sizeof(Class7_Object)},		{0x3115,sizeof(Class7_Object)},		{0x3116,sizeof(Event3116_Object)},
		{0x3117,sizeof(Class7_Object)},		{0x3118,sizeof(Class7_Object)},		{0x3119,sizeof(Class7_Object)},		{0x311A,sizeof(Event311A_Object)},	{0x311B,sizeof(Class7_Object)},
		{0x311C,sizeof(Event311C_Object)},	{0x3200,sizeof(Class7_Object)},		{0x3201,sizeof(Class7_Object)},		{0x3202,sizeof(Class7_Object)},		{0x3203,sizeof(Class7_Object)},
};

#endif
