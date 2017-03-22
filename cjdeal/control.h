/*
 * control.h
 *
 *  Created on: 2017-3-8
 *      Author: xxh
 * 终端的控制功能主要分为功率定值控制、电量定值控制、保电/剔除、远方控制这四大类：
 * <功率定值控制>:功率定值闭环控制根据控制参数不同分为时段功控、厂休功控、营业报停功控和当前功率下浮控等
 * 			控制类型，控制的优先级由高到低是当前功率下浮控、营业报停功控、厂休功控、时段功控。若多种功率
 * 			控制类型同时投入，只执行优先级最高的功率控制类型，
 * 	  1、时段功控：
 * 	  		控制流程：
 * 	  		a、主站依次向专变采集终端下发功控时段、功率定值、定值浮动系数、告警时间、控制轮次等参数，
 * 	  		终端收到这些命令后设置相应参数。
 * 	  		b、
 * 	  		c、
 * 	  2、厂休功控：
 * 	  3、营业报停功控：
 * 	  4、当前功率下浮控：
 * <电能量定值控制>：主要包括月电控、购电量（费）控等类型
 *    1、月电控：
 *    2、购电控：
 * <保电和剔除>：
 * 终端收到主站下发的保电命令后，进入保电状态，自动解除原有控制状态，并在任何情况下均不执行跳闸命令。
 * 终端收到主站保电解除命令，恢复正常执行控制命令。
 * 在终端上电或与主站通信持续不能链接时，终端应自动进入保电状态，待终端与主站恢复通信链接后，终端自动恢复到断线前的控制状态。
 * 终端接收到主站下发的剔除投入命令后，除对时命令外，对其他任何广播命令或终端组地址控制命令均不响应。
 * 终端收到主站的剔除解除命令，恢复到正常通信状态。
 * <远方控制>：
 * 终端接收主站的跳闸控制命令后，按设定的告警延迟时间，限电时间和控制轮次动作输出继电器，控制相应被控负荷开关；同时终端应有
 * 音响（或语音）告警通知用户，并记录跳闸时间，跳闸论次，跳闸前功率，跳闸后2min功率等，显示屏应显示执行结果，终端接收到主站
 * 的允许合闸控制命令后，应有音响（或语音）和显示“控制解除”告警通知用户，允许用户合闸。
 */

#ifndef CONTROL_H_
#define CONTROL_H_
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include "../libBase/PublicFunction.h"
#include "../include/StdDataType.h"
#define MAXNUM_PARA 150			//最大参数设置为150项

#define PROGNAME_MAX      	8
#define LUNCI_MAX			2

#define TRUE_STATE			1
#define ZERO_STATE			0

#define HE					1
#define FEN					0
#define RED_LIGHT		TRUE_STATE
#define RED_CLOSE		ZERO_STATE
#define GREEN_LIGHT		TRUE_STATE
#define GREEN_CLOSE		ZERO_STATE

typedef enum {
	STATE1,STATE2,STATE3
}baodian_state;	/*保电状态     属性2（保电状态，只读）∷=enum{解除（0），保电（1），自动保电（2）}*/

/*全局变量声明*/
int serial_fd;
char login_flag;
int thread_ctrl;
pthread_t thread_ctrl_id;

/*总加组接口类（class_id=23）*/
typedef struct{
	TSA total_dizhi;				//	参与总加的分路通信地址  TSA，
	INT8U total_biaozhi;			//	总加标志	            enum{正向（0），反向（1）}，
	INT8U total_yusuanfubiaozhi;	//	运算符标志	            enum{加（0），减（1）}
}total_peizhidanyuan;	//总加配置单元
typedef struct{
	INT8U	shiduan_plannum;//时段控定值方案号	   unsigned，
	INT8U	shiduan_valid;//功控时段有效标志位  bit-string(SIZE(8))，
	INT8U	gongkong_state;//功控状态            PCState，bit0～bit7按顺序对位表示；置“1”：投入，置“0”：解除；bit0：时段控;bit1：厂休控;bit2：营业报停控;	bit3：当前功率下浮控;	bit4～bit7：备用
	INT8U	diankong_state;//电控状态            ECState，bit0～bit7按顺序对位表示；置“1”：投入，置“0”：解除；bit0：月电控;bit1：购电控;bit2～bit7：备用
	INT8U	gongkong_trunstate;//功控轮次状态        TrunState，bit0～bit7按顺序对位表示1～8轮次开关的受控状态；置“1”：受控，置“0”：不受控。
	INT8U	diankong_trunstate;//电控轮次状态        TrunState,bit0～bit7按顺序对位表示1～8轮次开关的受控状态；置“1”：受控，置“0”：不受控。
}total_kongzhi_setstate;//总加组控制设置状态∷=structure
typedef struct{
	INT64U	dangqian_gongkongzhi;//	当前功控定值             long64（单位：W 换算：-1），
	INT8U	gonglv_xiafu_xishu;//	当前功率下浮控浮动系数   integer（单位：%），
	INT8U	gongkong_tiaozha_state;//	功控跳闸输出状态         OutputState，
	INT8U	yuediankong_tiaozha_state;//	月电控跳闸输出状态       OutputState，
	INT8U	goudiankong_tiaozha_state;//	购电控跳闸输出状态       OutputState，
	INT8U	gongkong_alarmstate;//	功控越限告警状态         PCAlarmState，
	INT8U	diankong_alarmstate;//	电控越限告警状态         ECAlarmState
}dangqian_kongzhi_zhuangtai;//总加组当前控制状态
typedef struct{

}huansuan_danwei;	//单位换算
typedef struct{
	OI_698	logic_name;				//逻辑名		1
	total_peizhidanyuan	total_pzdy;//总加配置表	2
	INT64U	total_yougonggonglv;	//总加有功功率		3
	INT64U	total_wugonggonglv;		//	4．总加无功功率
	INT64U	total_huacha_pingjunyou;//	5．总加滑差时间内平均有功功率
	INT64U	total_huacha_pingjunwu;//	6．总加滑差时间内平均无功功率
	INT64U	total_riyougongdianliang;//	7．总加日有功电量
	INT64U	total_riwugongdianliang;//	8．总加日无功电量
	INT64U	total_yueyougongdianliang;//	9．总加月有功电量
	INT64U	total_yuewugongdianliang;//	10．总加月无功电量
	INT64U	total_shengyudianliang;//	11．总加剩余电量（费）
	INT64U	total_dongjiezhi;//	12．当前功率下浮控控后总加有功功率冻结值
	INT8U period;//总加组滑差时间周期	13
	INT8U gongkong_lunci;//总加组功控轮次配置	14
	INT8U diankong_lunci;//总加组电控轮次配置	15
	total_kongzhi_setstate total_setstate;//总加组控制设置状态	16
	dangqian_kongzhi_zhuangtai total_dqkzzt;//总加组当前控制状态	17
	huansuan_danwei	huansuan_dw;//换算及单位	18
}SUMGROUP_INTERFACE_CLASS_23;//总加组接口类	2301总加组1......2308总加组8

/*控制方面的结构体类型定义*/
typedef struct{
	int begin;		//开始\结束分钟数  30（分钟）一个计数单位
	int end;		//0表示0点    30表示0：30   60表示1点   90表示1点30    120表示2点    150 表示2点20  ...
//	int shiduanNo;	//时段编号1-8
//	int kongzhi;	//控制标识
//	int valid;		//是否投入

}GongKong_ShiDuan_8101;//终端功控时段
typedef struct{
	//控制方案∷=structure
	//{
	OI_698 sumgroup_object;
	INT8U kongzhi_touru_biaoshi;//时段功控投入标识    bit-string(SIZE(8))，
	INT8U fanganhao;//时段功控定值方案号  unsigned
}kongzhi_fangan;
typedef struct{
	INT8U time_interval_num;	//时段号
	INT64U time_interval_1_value;//时段1功控定值
	INT64U time_interval_2_value;//时段2功控定值
	INT64U time_interval_3_value;//时段3功控定值
	INT64U time_interval_4_value;//时段4功控定值
	INT64U time_interval_5_value;//时段5功控定值
	INT64U time_interval_6_value;//时段6功控定值
	INT64U time_interval_7_value;//时段7功控定值
	INT64U time_interval_8_value;//时段8功控定值
}PowerCtrlParam;
typedef struct{
	OI_698 sumgroup_object;		//总加组对象
	INT8U  plan_ide;	//方案标识    标识identification=ide
	PowerCtrlParam pcp1;	//第一套定值
	PowerCtrlParam pcp2;	//第二套定值
	PowerCtrlParam pcp3;	//第三套定值
	INT8S  coefficient;//时段功控定值浮动系数
}Time_Interval_8109;	//时段功控配置单元

typedef struct{

//	gongkong_shiduan_8101 gk_shiduan[48];//
//	time_interval_8109 ti_8109;
}CONTROL_SHIDUAN_8103;
typedef struct{
	OI_698 sumgroup_object;		//总加组对象//	总加组对象    OI，
	INT64U	changkong_dingzhi;//	厂休控定值    long64（单位：W，换算：-1），
	DateTimeBCD_S	xiandian_begin_time;//	限电起始时间  date_time_s（年=FFFFH，月=FFH，日=FFH），
	INT16U	xiandian_yanxu_time;//	限电延续时间  long-unsigned（单位：分钟），
	INT8U	xiandianri;//	每周限电日    bit-string(SIZE(8))
}ChangXiu_ConfigUnit_810A;
typedef struct{

}CONTROL_CHANGXIU_8104;	//厂休控
typedef struct{
	OI_698 sumgroup_object;		//	总加组对象      OI，
	DateTimeBCD_S	baoting_begin_time;//	报停起始时间    date_time_s（时=FFH，分=FFH），
	DateTimeBCD_S	baoting_end_time;//	报停结束时间    date_time_s（时=FFH，分=FFH），
	INT64U	baotingkong_gonglv_dingzhi;//	报停控功率定值  long64（单位：W，换算：-1）
}YingYe_ConfigUnit_810B;
typedef struct{

}CONTROL_YINGYE_8105;	//营业报停控制

typedef struct{
	INT8U	huacha_shijian;	//	当前功率下浮控定值滑差时间    unsigned（单位：分钟），
	INT8S	fudongxishu;	//	当前功率下浮控定值浮动系数    integer（单位：%），
	INT8U	dongjie_yanshi;//	控后总加有功功率冻结延时时间  unsigned（单位：分钟），
	INT8U	kongzhi_shijian;//	当前功率下浮控的控制时间      unsigned（单位：0.5小时），
	INT8U	gaojing_shijian_1;//	当前功率下浮控第1轮告警时间  unsigned（单位：分钟），
	INT8U	gaojing_shijian_2;//	当前功率下浮控第2轮告警时间  unsigned（单位：分钟），
	INT8U	gaojing_shijian_3;//	当前功率下浮控第3轮告警时间  unsigned（单位：分钟），
	INT8U	gaojing_shijian_4;//	当前功率下浮控第4轮告警时间  unsigned（单位：分钟）
}CONTROL_XIAFU_8106;
typedef struct{
	OI_698 sumgroup_object;		//		总加组对象      OI，
	INT64U	yuedianliangkong_dingzhi;//		月电量控定值    long64（单位：kWh，换算：-4），
	INT8U	baojingmen_xishu;//		报警门限值系数  unsigned（单位：%），
	INT8S	yuedianliangkong_fudong;//		月电量控定值浮动系数  integer（单位：%）
}YueDian_ConfigUnit_810D;
typedef struct{

}CONTROL_YUEDIAN_8107;
typedef enum{
	zhuijia,shuaxin
}ZuiJia_ShuaXin_BiaoShi;
typedef enum{
	dianliang,dianfei
}GouDian_LeiXing;
typedef enum{
	bendi,yuancheng
}GouDianKong_MoShi;
typedef struct{
	OI_698 sumgroup_object;		//	总加组对象      OI，
	INT32U	goudian_danhao;//	购电单号        double-long-unsigned，
	ZuiJia_ShuaXin_BiaoShi	zuihui_biaoshi;//	追加/刷新标识   enum{追加（0），刷新（1）}，
	GouDian_LeiXing goudian_leixing;//	购电类型        enum{电量（0），电费（1）}，
	INT64U	goudianliang_zhi;//	购电量（费）值  long64（单位：kWh/元， 换算：-4），
	INT64U	baojing_menxianzhi;//	报警门限值      long64（单位：kWh/元，换算：-4），
	INT64U	tiaozha_menxianzhi;//	跳闸门限值      long64（单位：kWh/元，换算：-4）
	GouDianKong_MoShi	goudiankong_moshi;//	购电控模式      enum{本地模式（0），远程模式（1）}
}GouDian_ConfigUnnit_810C;
typedef struct{

}CONTROL_GOUDIAN_8108;

typedef struct{

}CONTROL_YAOKONG_8000;
typedef struct{
	INT8U begintime;	//起始时间（时）
	INT8U endtime;		//结束时间（时）  unsigned
}Auto_baodian_shiduan;//自动保电时段
typedef struct{
	baodian_state bd_s;//保电状态
	INT8U NoCommunication_TheMasterStation_Time; //允许与主站连续无通信时间//允许与主站最大无通信时长（分钟），0表示不自动保电
	INT16U shangdian_auto_baodian_shichang;//上电自动保电时长（分钟），0表示上电不自动保电
	Auto_baodian_shiduan auto_baodian;//自动保电时段
}CONTROL_BAODIAN_8001;
typedef struct{

}CUIFEI_8002;
typedef struct{

}COMMON_INFO_8003;	//一般中文信息
typedef struct{

}IMPORTANT_INFO_8004;	//重要中文信息
typedef struct{

}ALARM_STAT;
typedef struct{

	CONTROL_SHIDUAN_8103 	ShiDuankong;				//时段
	CONTROL_CHANGXIU_8104 	Changxiukong;				//厂休
	CONTROL_YINGYE_8105		Yingyekong;					//营业
	CONTROL_XIAFU_8106 		Xiafukong;					//下浮
	CONTROL_YUEDIAN_8107 	Yuediankong;				//月电
	CONTROL_GOUDIAN_8108 	Goudiankong;				//购电

	CONTROL_YAOKONG_8000 	Yaokong;					//遥控
	CONTROL_BAODIAN_8001 	Baodian;					//保电
	CUIFEI_8002				Cuifei;						//催费
	INT64U baoan_value_8100;							//保安定值
	GongKong_ShiDuan_8101	gk_shiduan_8101;			//	8101 终端功控时段;
	INT8U PowerCtrl_Alarm_Time;							//	8102 功控告警时间;
	Time_Interval_8109		time_interval_8109;			//	8109 时段功控配置单元;
	ChangXiu_ConfigUnit_810A	cx_configunit_810a;		//	810A 厂休控配置单元;
	YingYe_ConfigUnit_810B	yy_configunit_810b;			//	810B 营业报停控配置单元;
	GouDian_ConfigUnnit_810C	gd_configunit_810c;		//	810C 购电控配置单元;
	YueDian_ConfigUnit_810D		yd_configunit_810d;		//	810D 月电控配置单元;
	OI_698	ctr_object_810e;							//	810E 控制对象;OI
	INT8U	tiaozha_lunci;								//	810F 跳闸轮次;bit-string(SIZE(8))
	INT64U	diankong_dingzhi;							//	8110 电控定值;单位：kWh，换算-4

	COMMON_INFO_8003	com_info_8003;					//	8003 一般中文信息;
	IMPORTANT_INFO_8004	imp_info_8004;					//	8004 重要中文信息;

//	ALARM_STAT			AlarmStat[MAXNUM_SUMGROUP];			//告警状态
//	INT8U               TiChu;								//剔除
//	INT8U               ControlLED[2]; //第一字节控制用,0bit购电 1bit月电 2bit下浮 3bit营业 4bit厂休 5bit时段 6bit遥控 7bit催费
}CTR_OBJECT_CLASS;

typedef struct{
	INT8U lun1_state;
	INT8U lun1_red;
	INT8U lun1_green;
	INT8U lun2_state;
	INT8U lun2_red;
	INT8U lun2_green;
	INT8U gongk_led ;
	INT8U diank_led;
	INT8U alm_state ;
	INT8U baodian_led;
}CONTROL_VAR;


INT8U FnCounter[MAXNUM_PARA];//0无效   1-255设置参数次数循环累加计数


extern ProgramInfo* JProgramInfo;
#endif /* CONTROL_H_ */
