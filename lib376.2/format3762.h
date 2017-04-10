/*
 * format.h
 *
 *  Created on: 2013-3-19
 *      Author: Administrator
 */

#ifndef FORMAT3762_H_
#define FORMAT3762_H_

#include "../include/StdDataType.h"

#define RepeaterLevelMax 15	//中继级别最大

/******************************* AFN=00 确认/否认 ***********************************/
typedef struct
{
	INT8U CommandStatus;		//命令状态
	INT8U ChannelStatus[31];	//信道状态
	INT16U WaitingTime;			//等待时间
}AFN00_F1;	//确认

typedef struct
{
	INT8U ErrStatus;		//错误状态字
}AFN00_F2;	//否认




/******************************* AFN=02 数据转发 ***********************************/
typedef struct
{
	INT8U Protocol;			//通信协议类型
	INT8U MsgLength;		//报文长度
	INT8U MsgContent[256];	//报文内容
}AFN02_F1_DOWN;	//转发通信协议数据帧

typedef struct
{
	INT8U Protocol;			//通信协议类型
	INT8U MsgLength;		//报文长度
	INT8U MsgContent[256];	//报文内容
}AFN02_F1_UP;	//转发通信协议数据帧




/******************************* AFN=03 查询数据 ***********************************/
typedef struct
{
	INT8U Point;		//开始节点指针
	INT8U PointNum;		//读取节点的数量
}AFN03_F3_DOWN;	//从节点侦听信息

typedef struct
{
	INT8U Duration;		//持续时间
}AFN03_F6_DOWN;	//主节点干扰状态

typedef struct
{
	INT8U Protocol;			//通信协议类型
	INT8U MsgLength;		//报文长度
	INT8U MsgContent[256];	//报文内容
}AFN03_F9_DOWN;	//通信延时相关广播通信时长

typedef struct
{
	INT8U AFN;		//AFN功能码
}AFN03_F11_DOWN;	//本地通信模块AFN索引

typedef struct
{
	INT8U VendorCode[2];	//厂商代码
	INT8U ChipCode[2];		//芯片代码
	INT8U VersionDay;		//版本日期-日
	INT8U VersionMonth;		//版本日期-月
	INT8U VersionYear;		//版本日期-年
	INT8U Version[2];		//版本
}AFN03_F1_UP;	//厂商代码和版本信息

typedef struct
{
	INT8U NoiseIntensity;	//噪声强度
}AFN03_F2_UP;	//噪声值

typedef struct
{
	INT8U Addr[6];			//从节点地址
	INT8U RepeaterLevel;	//中继级别
	INT8U InterceptQuality;	//侦听信号品质
	INT8U InterceptCount;	//侦听次数
}InterceptInfo;	//从节点侦听信息

typedef struct
{
	INT8U SlavePointNum;			//侦听到的从节点总数
	INT8U SlavePointNum_Trans;		//侦听到的本帧传输的从节点数量
	InterceptInfo SlavePoint[256];	//从节点侦听信息
}AFN03_F3_UP;	//从节点侦听信息

typedef struct
{
	INT8U MasterPointAddr[6];		//主节点地址
}AFN03_F4_UP;	//主节点地址

typedef struct
{
	INT16U Rate;	//通信速率
	INT8U RateUnit;	//速率单位标识
}CommunicationInfo;	//通信速率信息

typedef struct
{
	INT8U RateNum;				//速率数量
	INT8U Channel;				//主节点信道特征
	INT8U ReadMode;				//周期抄表模式
	INT8U ChannelNum;			//信道数量
	CommunicationInfo Rate[15];	//通信速率信息
}AFN03_F5_UP;	//主节点状态字和通信速率

typedef struct
{
	INT8U DisturbStatus;		//干扰状态
}AFN03_F6_UP;	//主节点干扰状态

typedef struct
{
	INT8U OverTime;		//最大超时时间
}AFN03_F7_UP;	//读取从节点监控最大超时时间

typedef struct
{
	INT8U WirelessChannel;	//无线信道组
	INT8U WirelessPower;	//无线主节点发射功率
}AFN03_F8_UP;	//查询无线通信参数

typedef struct
{
	INT16U DelayTime;		//广播通信延迟时间
	INT8U Protocol;			//通信协议类型
	INT8U MsgLength;		//报文长度
	INT8U MsgContent[256];	//报文内容
}AFN03_F9_UP;	//通信延时相关广播通信时长

typedef struct
{
	INT8U CommType;			//通信方式
	INT8U RouterManagement;	//路由管理方式
	INT8U SlavePointMode;	//从节点信息模式
	INT8U ReadMode;			//周期抄表模式
	INT8U TransParam;		//传输延时参数支持
	INT8U ChangeMode;		//失败节点切换发起方式
	INT8U ConfirmMode;		//广播命令确认方式
	INT8U ExecuteMode;		//广播命令信道执行方式
	INT8U ChannelNum;		//信道数量
	INT8U PowerOffInfo;		//低压电网掉电信息
	INT8U RateNum;			//速率数量
	INT8U MonitorOverTime;	//从节点监控最大超时时间
	INT16U BroadcastOverTime;//广播命令最大超时时间
	INT16U MsgMaxLen;		//最大支持的报文长度
	INT16U PackageMaxLen;	//文件传输支持的最大单个数据包长度
	INT8U UpdateWaitingTime;//升级操作等待时间
	INT8U MasterPointAddr[6];//主节点地址
	INT16U SlavePointMaxNum;//支持的最大从节点数量
	INT16U CurrSlavePointNum;//当前从节点数量
	INT8U PublishDay;		//通信模块使用的协议发布日期
	INT8U PublishMonth;		//通信模块使用的协议发布日期
	INT8U PublishYear;		//通信模块使用的协议发布日期
	INT8U RecordDay;		//通信模块使用的协议最后备案日期
	INT8U RecordMonth;		//通信模块使用的协议最后备案日期
	INT8U RecordYear;		//通信模块使用的协议最后备案日期
	AFN03_F1_UP ModuleInfo;	//通信模块厂商代码及版本信息
	CommunicationInfo Rate[15];	//通信速率信息
}AFN03_F10_UP;	//本地通信模块运行模式信息

typedef struct
{
	INT8U AFN;				//AFN功能码
	INT8U FnSupport[256];	//Fn等数据单元支持情况
}AFN03_F11_UP;	//本地通信模块报文支持信息


/******************************* AFN=04 链路接口检测 ***********************************/
typedef struct
{
	INT8U Duration;		//持续时间
}AFN04_F1_DOWN;	//发送测试

typedef struct
{
	INT8U rate;				//通信速率
	INT8U destAddr[6];		//目标地址
	INT8U protocol;			//协议类型
	INT8U msgLen;			//报文长度
	INT8U msgContent[256];	//报文内容
}AFN04_F3_DOWN;	//本地通信模块报文通信测试



/******************************* AFN=05 控制命令 ***********************************/
typedef struct
{
	INT8U MasterPointAddr[6];	//主节点地址
}AFN05_F1_DOWN;	//设置主节点地址

typedef struct
{
	INT8U EventReportFlag;	//事件上报状态标志
}AFN05_F2_DOWN;	//允许/禁止从节点上报

typedef struct
{
	INT8U ctrl;				//控制字
	INT8U MsgLength;		//报文长度
	INT8U MsgContent[256];	//报文内容
}AFN05_F3_DOWN;	//启动广播

typedef struct
{
	INT8U OverTime;		//最大超时时间
}AFN05_F4_DOWN;	//设置从节点监控最大超时时间

typedef struct
{
	INT8U WirelessChannel;	//无线信道组
	INT8U WirelessPower;	//无线主节点发射功率
}AFN05_F5_DOWN;	//设置无线通信参数




/******************************* AFN=06 主动上报 ***********************************/
typedef struct
{
	INT8U Addr[6];		//从节点地址
	INT8U Protocol;		//从节点通信协议类型
	INT16U Index;		//从节点序号
}SlavePointInfo;	//上报从节点信息

typedef struct
{
	INT8U Num;						//上报从节点的数量
	SlavePointInfo SlavePoint[256];	//上报从节点信息
}AFN06_F1_UP;	//上报从节点信息

typedef struct
{
	INT16U SlavePointIndex;	//从节点序号
	INT8U Protocol;			//通信协议类型 1:97 2:07
	INT16U TransTime;		//当前报文本地通信上行时间
	INT8U MsgLength;		//报文长度
	INT8U MsgContent[256];	//报文内容
}AFN06_F2_UP;	//上报从节点信息

typedef struct
{
	INT8U WorkChange;		//路由工作任务变动类型 1：抄表结束；2：搜表结束
}AFN06_F3_UP;	//上报路由工况变动信息

typedef struct
{
	INT8U Addr[6];		//下接从节点通信地址
	INT8U Protocol;		//下接从节点通信协议类型
}SubPointInfo;	//下接从节点信息

typedef struct
{
	INT8U Num;					//上报从节点的数量
	INT8U SlavePoinAddr[6];		//从节点1通信地址
	INT8U Protocol;				//从节点1通信协议类型
	INT16U Index;				//从节点1序号
	INT8U DeviceType;			//从节点1设备类型
	INT8U SubPointNum;			//从节点下接从节点数量
	INT8U SubPointNum_Trans;	//本报文传输的从节点1下接从节点数量
	SubPointInfo SubPoint[256];//下接从节点信息
}AFN06_F4_UP;	//上报从节点信息及设备类型

typedef struct
{
	INT16U DeviceType;		//从节点设备类型
	INT8U Protocol;			//通信协议类型
	INT8U MsgLength;		//报文长度
	INT8U MsgContent[256];	//报文内容
}AFN06_F5_UP;	//上报从节点事件

typedef struct
{
	INT8U Num;						//上报从节点数量
	INT8U SlavePointAddr[256][6];	//从节点地址
}AFN06_F6_UP;	//上报事件从节点的地址



/******************************* AFN=10 路由查询类 ***********************************/
typedef struct
{
	INT16U Index;	//从节点起始序号
	INT8U Num;		//从节点数量
}AFN10_F2_DOWN;	//从节点信息

typedef struct
{
	INT8U SlavePointAddr[6];	//从节点地址
}AFN10_F3_DOWN;	//从节点信息

typedef struct
{
	INT16U Index;	//从节点起始序号
	INT8U Num;		//从节点数量
}AFN10_F5_DOWN;	//未抄读成功的从节点信息

typedef struct
{
	INT16U Index;	//从节点起始序号
	INT8U Num;		//从节点数量
}AFN10_F6_DOWN;	//主动注册的从节点信息

typedef struct
{
	INT16U Num;			//从节点总数量
	INT16U SupportNum;	//路由支持的最大从节点数量
}AFN10_F1_UP;	//从节点数量

typedef struct
{
	INT8U Addr[6];			//地址
	INT8U RepeaterLevel;	//中继级别
	INT8U InterceptQuality;	//侦听信号品质
	INT8U Phase;			//相位
	INT8U Protocol;			//通信协议类型
}SlavePointInformation;

typedef struct
{
	INT16U Num;								//从节点总数量
	INT8U ReplyNum;							//本次应答的从节点数量
	SlavePointInformation SlavePoint[256];	//从节点信息
}AFN10_F2_UP;	//从节点信息

typedef struct
{
	INT8U Num;								//提供路由的从节点总数量
	SlavePointInformation SlavePoint[256];	//提供路由的从节点信息
}AFN10_F3_UP;	//指定从节点的上一级中继路由信息

typedef struct
{
	INT8U FinishFlag;			//路由完成标志	1：路由学习完成	0：未完成
	INT8U WorkFlag;				//工作标志		1：正在工作		0：停止工作
	INT8U ReportFlag;			//上报事件标志
	INT8U ErrCode;				//纠错编码
	INT16U Num;					//从节点总数量
	INT16U FinishedNum;			//已抄从节点数量
	INT16U RepeaterFinishedNum;	//中继抄到的从节点数量
	INT8U WorkStatus;			//工作状态
	INT8U RegisterStatus;		//注册允许状态
	INT16U Rate;				//通信速率
	INT8U RepeaterLevel_Phase1;	//第1相中继级别
	INT8U RepeaterLevel_Phase2;	//第2相中继级别
	INT8U RepeaterLevel_Phase3;	//第3相中继级别
	INT8U WorkStep_Phase1;		//第1相工作步骤
	INT8U WorkStep_Phase2;		//第2相工作步骤
	INT8U WorkStep_Phase3;		//第3相工作步骤
}AFN10_F4_UP;	//路由运行状态

typedef struct
{
	INT16U Num;								//从节点总数量
	INT8U ReplyNum;							//本次应答的从节点数量
	SlavePointInformation SlavePoint[256];	//从节点信息
}AFN10_F5_UP;	//未抄读成功的从节点信息

typedef struct
{
	INT16U Num;								//从节点总数量
	INT8U ReplyNum;							//本次应答的从节点数量
	SlavePointInformation SlavePoint[256];	//从节点信息
}AFN10_F6_UP;	//主动注册的从节点信息




/******************************* AFN=11 路由设置类 ***********************************/
typedef struct
{
	INT8U Addr[6];		//从节点地址
	INT8U Protocol;		//从节点通信协议类型
}SlavePointInfo2;

typedef struct
{
	INT16U Num;							//从节点数量
	SlavePointInfo2 SlavePoint[256];	//从节点信息
}AFN11_F1_DOWN;	//添加从节点

typedef struct
{
	INT8U Num;						//从节点数量
	INT8U SlavePointAddr[256][6];	//从节点地址
}AFN11_F2_DOWN;	//删除从节点

typedef struct
{
	INT8U SlavePointAddr[6];	//从节点地址
	INT8U RepeaterLevel;		//中继级别
	INT8U RepeaterSlavePointAddr[RepeaterLevelMax][6];	//各级中继从节点地址
}AFN11_F3_DOWN;	//设置从节点固定中继路径

typedef struct
{
	INT8U WorkFlag;			//工作状态
	INT8U RegisterFlag;		//注册允许状态
	INT8U ErrCode;			//纠错编码
	INT16U Rate;			//通信速率
	INT8U RateUnit;			//速率单位标识
}AFN11_F4_DOWN;	//设置路由工作模式

typedef struct
{
	INT8U StartTimeSecond;	//开始时间
	INT8U StartTimeMinute;
	INT8U StartTimeHour;
	INT8U StartTimeDay;
	INT8U StartTimeMonth;
	INT8U StartTimeYear;
	INT16U Duration;		//持续时间
	INT8U RepeatCount;		//重发次数
	INT8U TimeSliceNum;		//随机等待时间片个数
}AFN11_F5_DOWN;	//激活从节点主动注册

typedef struct
{
	INT8U Num;						//从节点数量
	INT8U SlavePointAddr[20][6];	//从节点地址
}AFN11_F8_DOWN;	//节点请求队列预告



/******************************* AFN=13 路由数据转发类 ***********************************/
typedef struct
{
	INT8U Protocol;				//通信协议类型
	INT8U DelayFlag;			//通信延时相关性标志
	INT8U SubPointNum;			//从节点附属节点数量
	INT8U SubPointAddr[256][6];	//从节点附属节点地址
	INT8U MsgLength;			//报文长度
	INT8U MsgContent[256];		//报文内容
}AFN13_F1_DOWN;	//监控从节点

typedef struct
{
	INT16U TransTime;			//本地通信上行时长
	INT8U Protocol;				//通信协议类型
	INT8U MsgLength;			//报文长度
	INT8U MsgContent[256];		//报文内容
}AFN13_F1_UP;	//监控从节点




/******************************* AFN=14 路由数据抄读类 ***********************************/
typedef struct
{
	INT8U ReadFlag;				//抄读标志
	INT8U DelayFlag;			//通信延时相关性标志
	INT8U MsgLength;			//报文长度
	INT8U MsgContent[256];		//报文内容
	INT8U SubPointNum;			//从节点附属节点数量
	INT8U SubPointAddr[256][6];	//从节点附属节点地址
}AFN14_F1_DOWN;	//路由请求抄读内容

typedef struct
{
	INT8U CurrentTimeSecond;	//当前时间
	INT8U CurrentTimeMinute;
	INT8U CurrentTimeHour;
	INT8U CurrentTimeDay;
	INT8U CurrentTimeMonth;
	INT8U CurrentTimeYear;
}AFN14_F2_DOWN;	//路由请求集中器时钟

typedef struct
{
	INT8U MsgLength;		//数据长度
	INT8U MsgContent[256];	//修正数据内容
}AFN14_F3_DOWN;	//请求依通信延时修正通信数据

typedef struct
{
	INT8U Phase;				//通信相位
	INT8U SlavePointAddr[6];	//从节点地址
	INT16U Index;				//从节点序号
}AFN14_F1_UP;	//路由请求抄读内容

typedef struct
{
	INT8U SlavePointAddr[6];	//通信相位
	INT16U DelayTime;			//预计延迟时间
	INT8U MsgLength;			//数据长度
	INT8U MsgContent[256];		//修正数据内容
}AFN14_F3_UP;	//请求依通信延时修正通信数据



/******************************* AFN=15 文件传输 ***********************************/
typedef struct
{
	INT8U FileFlag;			//文件标识
	INT8U FileProperty;		//文件属性
	INT8U FileOrder;		//文件指令
	INT16U SectionCount;	//总段数
	INT32U SectionFlag;		//第i段标识
	INT16U DataLength;		//第i段数据长度
	INT8U Data[65536];		//文件数据
}AFN15_F1_DOWN;	//文件传输方式1

typedef struct
{
	INT16U SectionFlag;		//收到当前段标识
}AFN15_F1_UP;	//文件传输方式1




/******************************* 帧格式 ***********************************/
typedef struct
{
	INT8U ComType;	//通信方式
	INT8U PRM;		//0：从动站；1：主动站
	INT8U DIR;		//0：下行；1：上行
}CtrlDomain;	//控制域

typedef struct
{
	INT8U RouterFlag;		//路由标识
	INT8U SubPointFlag;		//附属节点标识
	INT8U ModuleFlag;		//通信模块标识
	INT8U ClashCheck;		//冲突检测
	INT8U RepeaterLevel;	//中继级别
	INT8U ChannelFlag;		//信道标识
	INT8U ErrCode;			//纠错编码标识
	INT8U ReplyBytes;		//预计应答字节数
	INT16U Rate;			//通信速率
	INT8U RateUnit;			//速率单位
	INT8U Seq;				//报文序列号
}InfoDomain_DOWN;	//信息域 - 下行

typedef struct
{
	INT8U RouterFlag;		//路由标识
	INT8U ModuleFlag;		//通信模块标识
	INT8U RepeaterLevel;	//中继级别
	INT8U ChannelFlag;		//信道标识

	INT8U PhaseFlag;		//实测相线标识
	INT8U ChannelFeature;	//电能表通道特征

	INT8U CommandQuality;	//末级命令信号品质
	INT8U ReplyQuality;		//末级应答信号品质
	INT8U EventFlag;		//事件标识
	INT8U Seq;				//报文序列号
}InfoDomain_UP;	//信息域 - 上行

typedef struct
{
	INT8U SourceAddr[6];	//源地址
	INT8U RepeaterAddr[15][6];	//中继地址
	INT8U DestAddr[6];		//目的地址
}AddrDomain;	//地址域	InfoDomain.PointFlag=0时，无地址域



typedef struct
{
	INT16U length;
	CtrlDomain ctrl;
	InfoDomain_DOWN info_down;
	InfoDomain_UP info_up;
	AddrDomain addr;

	INT8U afn;
	INT8U dt1;
	INT8U dt2;
	INT16U fn;

	AFN00_F1 afn00_f1;
	AFN00_F2 afn00_f2;

	AFN02_F1_DOWN afn02_f1_down;
	AFN02_F1_UP afn02_f1_up;

	AFN03_F3_DOWN afn03_f3_down;
	AFN03_F6_DOWN afn03_f6_down;
	AFN03_F9_DOWN afn03_f9_down;
	AFN03_F11_DOWN afn03_f11_down;
	AFN03_F1_UP afn03_f1_up;
	AFN03_F2_UP afn03_f2_up;
	AFN03_F3_UP afn03_f3_up;
	AFN03_F4_UP afn03_f4_up;
	AFN03_F5_UP afn03_f5_up;
	AFN03_F6_UP afn03_f6_up;
	AFN03_F7_UP afn03_f7_up;
	AFN03_F8_UP afn03_f8_up;
	AFN03_F9_UP afn03_f9_up;
	AFN03_F10_UP afn03_f10_up;
	AFN03_F11_UP afn03_f11_up;

	AFN04_F1_DOWN afn04_f1_down;
	AFN04_F3_DOWN afn04_f3_down;

	AFN05_F1_DOWN afn05_f1_down;
	AFN05_F2_DOWN afn05_f2_down;
	AFN05_F3_DOWN afn05_f3_down;
	AFN05_F4_DOWN afn05_f4_down;
	AFN05_F5_DOWN afn05_f5_down;

	AFN06_F1_UP afn06_f1_up;
	AFN06_F2_UP afn06_f2_up;
	AFN06_F3_UP afn06_f3_up;
	AFN06_F4_UP afn06_f4_up;
	AFN06_F5_UP afn06_f5_up;
	AFN06_F6_UP afn06_f6_up;

	AFN10_F2_DOWN afn10_f2_down;
	AFN10_F3_DOWN afn10_f3_down;
	AFN10_F5_DOWN afn10_f5_down;
	AFN10_F6_DOWN afn10_f6_down;
	AFN10_F1_UP afn10_f1_up;
	AFN10_F2_UP afn10_f2_up;
	AFN10_F3_UP afn10_f3_up;
	AFN10_F4_UP afn10_f4_up;
	AFN10_F5_UP afn10_f5_up;
	AFN10_F6_UP afn10_f6_up;

	AFN11_F1_DOWN afn11_f1_down;
	AFN11_F2_DOWN afn11_f2_down;
	AFN11_F3_DOWN afn11_f3_down;
	AFN11_F4_DOWN afn11_f4_down;
	AFN11_F5_DOWN afn11_f5_down;
	AFN11_F8_DOWN afn11_f8_down;

	AFN13_F1_DOWN afn13_f1_down;
	AFN13_F1_UP afn13_f1_up;

	AFN14_F1_DOWN afn14_f1_down;
	AFN14_F2_DOWN afn14_f2_down;
	AFN14_F3_DOWN afn14_f3_down;
	AFN14_F1_UP afn14_f1_up;
	AFN14_F3_UP afn14_f3_up;

	AFN15_F1_DOWN afn15_f1_down;
	AFN15_F1_UP afn15_f1_up;

	INT8U cs;
}FORMAT3762;	//376.2帧结构


#endif /* FORMAT_H_ */
