/*
 * calc.h
 *
 *  Created on: 2017-2-15
 *      Author: wzm
 */

#ifndef CALC_H_
#define CALC_H_

#include "ParaDef.h"
#include "StdDataType.h"

#define MESGBUF_MAX			5120//2048
#define ZhongJiaPoint_Max   64	 //每总加组最大测量点数
#define SHANG_XIAN          1    //越上限
#define XIA_XIAN            2    //越下限
#define SSHANG_XIAN         3    //越上上限
#define XXIA_XIAN           4    //越下下限
#define HEGE 	            5    //合格

#define ALL_TYPE			1     //全部类型
#define JIAOCAI_TYPE		2	 //交采类型
#define METER_485_TYPE		3	 //485电表类型
#define METER_ZB_TYPE		4	 //载波电表类型
#define METER_PULSE_TYPE	5	 //脉冲电表类型
#define DAY					2	 //日统计类型 此处的定义是为了和日期的枚举类型保持一致
#define MONTH				1	 //月统计类型
#define GROUPDAY			3    //总加组日统计类型
#define GROUPMONTH  		4    //总加组月统计类型
#define GROUPCURVED		    5    //总加组曲线统计类型

#define TJ_ITEMCOUNT		350	 //全部统计测量点的统计数据条数
#define	EVENTNUM_MAX		256	 //事件数据结构体总个数
#define	EVENTBUF			100	 //事件数据缓冲区长度
#define RIPP				117		//dm.cfg文件ID对应
#define MAXNUM_IMPORTANTUSR_CALC MAXNUM_IMPORTANTUSR //重点用户数量最多２０个电表测量点＋１个交采测量点
#define DEMANDPERIOD        15 //需量周期，默认是15
#define DATASTARTPOSITION   4//bin类型数据格式拷贝的起始地址
#define CURVED_DATA_LEN  96//曲线数组长度

#define kw2w 1 //单位千瓦转换成瓦
#define kw2w_acs 1000 //交采单位千瓦转换成瓦
#define ENLARG_DIGIT_ENERGY 2 //电能量放大位数
#define ENLARG_DIGIT_P 1 //功率放大位数
//#endif
#define ENLARG_DIGIT_U 1 //电压放大位数
#define ENLARG_DIGIT_I 3 //电流放大位数

#define A23_INTBYT 2
#define A23_DEDBIT 4

#define A7_INTBYT 3
#define A7_DEDBIT 1

#define A13_INTBYT 4
#define A13_DEDBIT 4

#define A25_INTBYT 3
#define A25_DEDBIT 3

#define A5_INTBYT 3
#define A5_DEDBIT 1

#define A10_INTBYT 6
#define A10_DEDBIT 0

#define A2_INTBYT 4


#define A5_COEF 10 //A5格式在存储时的放大倍数

#define A25_COEF 1000 //A25格式在存储时的放大倍数
#define ERC14PATH "/nand/erc14.bat"
#define PUBDATA_BAK "/nand/para/pubdata.bak"
#define RAT_COEF 100   //百分比放大倍数

pthread_attr_t calc_attr_t;
int thread_calc_id;        //统计
pthread_t thread_calc;
extern void calc_proccess();
typedef struct
{
	INT16U id;
	INT8U type;
}DATA_ID_TYPE;

typedef struct
{
	unsigned char  Offset;					//当前存储需量纪录指针，0-15
	signed int Z_P_Value[DEMANDPERIOD];				//正向有功电能实时值
	signed int Z_Q_Value[DEMANDPERIOD];				//正向无功电能实时值
	signed int F_P_Value[DEMANDPERIOD];				//反向有功电能实时值
	signed int F_Q_Value[DEMANDPERIOD];				//反向无功电能实时值

	signed int Z_P_FL_Value[MAXVAL_RATENUM][DEMANDPERIOD];	//正向有功电能四费率实时值
	signed int Z_Q_FL_Value[MAXVAL_RATENUM][DEMANDPERIOD];	//正向无功电能四费率实时值
	signed int F_P_FL_Value[MAXVAL_RATENUM][DEMANDPERIOD];	//反向有功电能四费率实时值
	signed int F_Q_FL_Value[MAXVAL_RATENUM][DEMANDPERIOD];	//反向无功电能四费率实时值

	INT32U P_Value[MAXNUM_IMPORTANTUSR_CALC][DEMANDPERIOD]; //A相有功功率实时值
	INT32U Pa_Value[MAXNUM_IMPORTANTUSR_CALC][DEMANDPERIOD]; //A相有功功率实时值
	INT32U Pb_Value[MAXNUM_IMPORTANTUSR_CALC][DEMANDPERIOD];//B相有功功率实时值
	INT32U Pc_Value[MAXNUM_IMPORTANTUSR_CALC][DEMANDPERIOD];//C相有功功率实时值
}XuLiangCalc_TYPE;
typedef struct{
	INT32U value;
	INT8U Available;
}MYINT32UC;
typedef struct{
	INT32S value;
	INT8U Available;
}MYINT32S;

typedef struct
{
	MYINT32UC Va_i;//1分钟平均
	MYINT32UC Vb_i;
	MYINT32UC Vc_i;
	MYINT32UC Va; //瞬时值
	MYINT32UC Vb;
	MYINT32UC Vc;
	MYINT32S Ia;
	MYINT32S Ib;
	MYINT32S Ic;
	MYINT32S I0;
	MYINT32S Cos;//总功率因数
	MYINT32S Cosa;
	MYINT32S Cosb;
	MYINT32S Cosc;
	MYINT32UC CP;
	MYINT32UC CPa;
	MYINT32UC Pb;
	MYINT32UC Pc;

	MYINT32UC P_R[MAXVAL_RATENUM];//正向有功费率x有功功率
	MYINT32UC Q_R[MAXVAL_RATENUM];//正向无功费率x有功功率

	MYINT32UC F_P;//反向有功功率
	MYINT32UC F_P_R[MAXVAL_RATENUM];//反向有功费率x有功功率
	MYINT32UC F_Q;//反向无功功率
	MYINT32UC F_Q_R[MAXVAL_RATENUM];//反向无功费率x有功功率

	MYINT32UC Qa;
	MYINT32UC Qb;
	MYINT32UC Qc;
	MYINT32UC CQ;//正向无功功率
	MYINT32UC CPsz;//总视在功率
	MYINT32S PFlag; //有功、无功功率方向，正向为0，负相为1
	MYINT32S z_P_energy_all;				//当前正向有功总电能量
	MYINT32S z_P_energy[MAXVAL_RATENUM];	//当前正向有功电能量
	MYINT32S f_P_energy_all;				//当前反向有功总电能量
	MYINT32S f_P_energy[MAXVAL_RATENUM];	//当前反向有功电能量
	MYINT32S z_Q_energy_all;				//当前正向无功总电能量
	MYINT32S z_Q_energy[MAXVAL_RATENUM];	//当前正向无功电能量
	MYINT32S f_Q_energy_all;  			//当前反向无功总电能量
	MYINT32S f_Q_energy[MAXVAL_RATENUM];  //当前反向无功电能量

	INT16U ElectricMeterState_old[7];//电能表运行状态字
	INT16U ElectricMeterState[7];//电能表运行状态字
	MYINT32UC SFlag;		//交采相序异常标记
}REALDATA;


typedef struct{
	INT32U Value;					//限值
	INT32U ContinueTime;			//持续时间
	INT32U Reinstate;				//恢复系数
}LIMITVALUE_TYPE;					//F26 设置


typedef struct{
	//INT32U U_count;//一天当中相电压累加时间
	INT32U Last_count;//当前电压的上一次持续时间
	struct tm StartTime;//当前电压当前状态的起始时间
}voltageRateProp;//用于计算电压合格率的中间值


typedef struct{
	INT16U ss_count;					//相电压越上上限累计时间    (min)
	INT16U xx_count;					//相电压越下下限累计时间    (min)
	INT16U s_count;						//相电压越上限累计时间      (min)
	INT16U x_count;						//相电压越下限累计时间 	(min)
	INT16U ok_count;					//相电压合格限累计时间 	(min)
	int max;							//相电压最大值
	unsigned char max_time[3];			//相电压最大值发生时间
	int min;							//相电压最小值
	unsigned char min_time[3];			//相电压最小值发生时间
	INT32U U_Avg;//一天当中相电压的平均电压
	FP32 s_Rate;//电压上限率
	FP32 x_Rate;//电压下限率
	FP32 ok_Rate;//电压合格率

	INT32U U_Sum;//一天当中相电压累加和，用于计算平均电压
	INT32U U_Count;//一天当中相电压累加时间
	struct tm StartTime;//当前电压的起始时间
	voltageRateProp voltageRate_SS;//用于计算电压合格率的中间值,上上限
	voltageRateProp voltageRate_S;//用于计算电压合格率的中间值,上限
	voltageRateProp voltageRate_ok;//用于计算电压合格率的中间值,合格
	voltageRateProp voltageRate_x;//用于计算电压合格率的中间值,下限
	voltageRateProp voltageRate_xx;//用于计算电压合格率的中间值,下下限
}DIANYA_TJ;

typedef struct{
	INT32S ss_count;					//相电流越上上限累计时间   (min)
	INT32S s_count;					//相电流越上限累计时间	 (min)
	int max;							//相电流最大值
	unsigned char max_time[3];			//相电流最大值发生时间
}DIANLIU_TJ;

typedef struct{
	int max;							//功率最大值
	unsigned char max_time[3];			//功率最大值发生时间
	int min;
	unsigned char min_time[3];			//功率最小值发生时间
	int zero_count;						//功率为零时间		(min)
}GONGLV_TJ;

//记录不平衡度值
typedef struct{

	INT16U I_UNBALANCE_Count;//电流不平衡度越限日累计时间
	INT16U V_UNBALANCE_Count;//电压不平衡度越限日累计时间

	INT16S I_UNBALANCE_Max;//电流不平衡最大值
	TS I_UNBALANCE_MaxTime;//电流不平衡最大值发生时间

	INT16U V_UNBALANCE_Max;//电压不平衡最大值
	TS V_UNBALANCE_MaxTime;//电压不平衡最大值发生时间
}UNBALANCE_PROPERTY;

//最大需量
typedef struct
{
	INT32S Z_P_X_All;//正向有功总最大需量
	INT32S Z_P_X_F[MAXVAL_RATENUM];//费率1正向有功最大需量

	INT32S F_P_X_All;//反向有功总最大需量
	INT32S F_P_X_F[MAXVAL_RATENUM];//费率1反向有功最大需量

	INT32S Z_Q_X_All;//正向无功总最大需量
	INT32S Z_Q_X_F[MAXVAL_RATENUM];//费率1正向无功最大需量

	INT32S F_Q_X_All;//反向无功总最大需量
	INT32S F_Q_X_F[MAXVAL_RATENUM];//费率1反向无功最大需量

//	INT32S P_X_All;//三相有功总最大需量
//	INT32S Pa_X_All;//A相有功总最大需量
//	INT32S Pb_X_All;//B相有功总最大需量
//	INT32S Pc_X_All;//C相有功总最大需量

	INT8U Time_Z_P_X_All[4];// 正向有功总最大需量发生时间
	INT8U Time_Z_P_X_F[MAXVAL_RATENUM][4];//费率1正向有功最大需量发生时间

	INT8U Time_F_P_X_All[4];// 反向有功总最大需量发生时间
	INT8U Time_F_P_X_F[MAXVAL_RATENUM][4];//费率1反向有功最大需量发生时间

	INT8U Time_Z_Q_X_All[4];//正向无功总最大需量发生时间//分 时 日 月
	INT8U Time_Z_Q_X_F[MAXVAL_RATENUM][4];//费率1正向无功最大需量发生时间

	INT8U Time_F_Q_X_All[4];//反向无功总最大需量发生时间
	INT8U Time_F_Q_X_F[MAXVAL_RATENUM][4];//费率1反向无功最大需量发生时间

//	INT8U Time_P_X_All[4];//三相有功总最大需量发生时间
//	INT8U Time_Pa_X_All[4];//A相有功总最大需量发生时间
//	INT8U Time_Pb_X_All[4];//B相有功总最大需量发生时间
//	INT8U Time_Pc_X_All[4];//C相有功总最大需量发生时间

}MaxDemand;

typedef struct
{
	INT32S P_X_All;//三相有功总最大需量
	INT32S Pa_X_All;//A相有功总最大需量
	INT32S Pb_X_All;//B相有功总最大需量
	INT32S Pc_X_All;//C相有功总最大需量

	INT8U Time_P_X_All[4];//三相有功总最大需量发生时间
	INT8U Time_Pa_X_All[4];//A相有功总最大需量发生时间
	INT8U Time_Pb_X_All[4];//B相有功总最大需量发生时间
	INT8U Time_Pc_X_All[4];//C相有功总最大需量发生时间
}MaxDemand_phase;



//功率及电能示值
typedef struct{
	INT32U z_Psz_energy_all;
	INT32U z_Psz_energy[MAXVAL_RATENUM];//实时正向有功总电能示值
	INT32U f_Psz_energy_all;
	INT32U f_Psz_energy[MAXVAL_RATENUM];//实时反向有功总电能示值
	INT32U z_Qsz_energy_all;
	INT32U z_Qsz_energy[MAXVAL_RATENUM];//实时正向无功总电能示值
	INT32U f_Qsz_energy_all;
	INT32U f_Qsz_energy[MAXVAL_RATENUM];//实时反向无功总电能示值
	INT8U  Valid;
}ENERGY_NUM;

//功率及电能示值
typedef struct{
	MYINT32UC z_Psz_energy_all;
	MYINT32UC z_Psz_energy[MAXVAL_RATENUM];//实时正向有功总电能示值
	MYINT32UC f_Psz_energy_all;
	MYINT32UC f_Psz_energy[MAXVAL_RATENUM];//实时反向有功总电能示值
	MYINT32UC z_Qsz_energy_all;
	MYINT32UC z_Qsz_energy[MAXVAL_RATENUM];//实时正向无功总电能示值
	MYINT32UC f_Qsz_energy_all;
	MYINT32UC f_Qsz_energy[MAXVAL_RATENUM];//实时反向无功总电能示值
	INT8U  Valid;
}ENERGY_NUM_VALID;

//负载率
typedef struct{
	FP32 LoadRateMax;//负载率最大值
	INT8U max_time[3];//负载率最大值发生时间
	FP32 LoadRateMin;//负载率最小值
	INT8U min_time[3];//负载率最小值发生时间
}LoadRate_PROPERTY;

typedef struct{
	INT32S z_Psz_energy_all;
	INT32S f_Psz_energy_all;
	INT32S z_Qsz_energy_all;
	INT32S f_Qsz_energy_all;
}CURVEDDATA_PROPERTY;//曲线数据的属性

typedef struct{
	DIANYA_TJ tjUa;						//A相电压统计								二类数据F27
	DIANYA_TJ tjUb;						//B相电压统计
	DIANYA_TJ tjUc;						//C相电压统计
	//--------------------------------- 											二类数据F29
	DIANLIU_TJ tjIa;					//A相电流统计
	DIANLIU_TJ tjIb;					//B相电流统计
	DIANLIU_TJ tjIc;					//C相电流统计
	DIANLIU_TJ tjI0;					//零序电流统计    零序电流只判断越上限
	//---------------------------------
	short int cos_qduan1;				//功率因数区段1累计时间     						 二类数据F43
	short int cos_qduan2;				//功率因数区段2累计时间
	short int cos_qduan3;				//功率因数区段3累计时间
	//---------------------------------
	GONGLV_TJ tjP;						//总功率统计									二类数据F25
	GONGLV_TJ tjPa;						//A相功率统计
	GONGLV_TJ tjPb;						//B相功率统计
	GONGLV_TJ tjPc;						//C相功率统计
	//---------------------------------
	DIANLIU_TJ tjSzP;					//视在功率越限统计(借用电流统计数据结构)		二类数据30
//	GONGLV_TJ  tjXuliang;				//交采需量统计
//	MaxDemand tjXuliang;				//交采需量统计
	MaxDemand_phase tjXuliang;//三相需量统计
//	ENERGY_NUM tjEnergy;			//测量点能量统计   二类数据
	ENERGY_NUM_VALID tjEnergy_last;		//测量点前一天能量记录   二类数据
//	ENERGY_NUM_VALID tjEnergy_lastlast;	//测量点前两天能量记录   二类数据
	UNBALANCE_PROPERTY UNBALANCE_value; //不平衡度
	LoadRate_PROPERTY LoadRate;
//	PHASE_PROPERTY PHASE_prop_last;//测量点前一次断相记录   二类数据
//	OldXuLiangSet tjXuliang_Old;
}TONGJI_RESULT;//数据统计项

typedef struct{
	int PointNo;    	  		//测量点号
	int Type;			  		//测量点类型               2交采类型    3电表485  4电表载波
	REALDATA  Realdata;   		//测量点当前数据
	TONGJI_RESULT Result; 		//日统计值
	TONGJI_RESULT Result_m; 	//月统计值
	int valid;					//有效标识
}POINT_CALC_TYPE;
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
#endif /* CALC_H_ */
