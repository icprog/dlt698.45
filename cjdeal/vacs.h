/*
 * vacs.h
 *
 *  Created on: 2013-5-4
 *      Author: Administrator
 */

#ifndef VACS_H_
#define VACS_H_

#include <semaphore.h>
#include "StdDataType.h"

#define U 1     //电压
#define I 2     //电流
#define P 3     //有功
#define Q 4     //无功
#define S 5     //视在
#define COS 6   //功率因数
#define FREQ 7  //频率
#define PHASE 8 //电压与电流相角
#define ANGLE 9 //电压夹角
#define TEMP 10 //温度计算

#define REALDATALEN 8192
#define REALDFNUM 128    //主站实抄数据
#define MAXVAL_RATENUM 4 //支持的最大费率数
#define U_COEF 10        //电压扩大系数

typedef struct {
    INT32S Pa; //有功功率
    INT32S Pb;
    INT32S Pc;
    INT32S Pt;

    INT32S Qa; //无功功率
    INT32S Qb;
    INT32S Qc;
    INT32S Qt;

    INT32S Sa; //视在功率
    INT32S Sb;
    INT32S Sc;
    INT32S St;

    INT32U AvgUa; // 1分钟平均电压
    INT32U AvgUb;
    INT32U AvgUc;

    INT32U Ua; //电压
    INT32U Ub;
    INT32U Uc;

    INT32U Ia; //电流
    INT32U Ib;
    INT32U Ic;
    INT32U I0;

    INT32S CosA; //功率因数
    INT32S CosB;
    INT32S CosC;
    INT32S Cos;

    INT32S Freq; //频率

    INT32S Pga; // A相电流与电压夹角
    INT32S Pgb; // B相电流与电压夹角
    INT32S Pgc; // C相电流与电压夹角

    INT32S YUaUb; //电压夹角
    INT32S YUaUc;
    INT32S YUbUc;

    INT32U LineUa; //基波电压有效值
    INT32U LineUb;
    INT32U LineUc;

    INT32U LineIa; //基波电流有效值
    INT32U LineIb;
    INT32U LineIc;

    INT32U PFlag; //有功、无功功率方向，正向为0，负相为1,
    INT32S Temp;  //温度传感器值
    INT32U SFlag; //存放断相、相序、SIG标志状态,第三位为1电压相序错,第四位为1电流相序错,
} _RealData;

typedef struct {
    //以下定义存放：ATT7022E寄存器值
    INT32U EPt; //合相有功电能
    INT32U EPa;
    INT32U EPb;
    INT32U EPc;

    INT32U EQt; //合相无功电能
    INT32U EQa;
    INT32U EQb;
    INT32U EQc;
    //以下定义存放：根据有功、无功功率方向计算正向、反向电能示值
    INT32U PosPa; //正相有功分项电能示值，根据
    INT32U PosPb;
    INT32U PosPc;
    INT32U PosPt;

    INT32U PosQa; //正相无功分项电能示值
    INT32U PosQb;
    INT32U PosQc;
    INT32U PosQt; //组合无功1

    INT32U NegPa; //反相有功分项电能示值
    INT32U NegPb;
    INT32U NegPc;
    INT32U NegPt;

    INT32U NegQa; //反相无功分项电能示值
    INT32U NegQb;
    INT32U NegQc;
    INT32U NegQt;
} _EnergyCurr;

typedef struct {
    INT8U point;
    INT32U Buff[16];
} _AverageStru;

//交采系数
typedef struct {
    INT16U crc;      // CRC校验
    INT8U PhaseA[3]; //相角系数
    INT8U PhaseB[3];
    INT8U PhaseC[3];
    INT8U PhaseA1[3]; //小电流相角系数
    INT8U PhaseB1[3];
    INT8U PhaseC1[3];
    INT8U PhaseA0[3]; //分段电流相角系数
    INT8U PhaseB0[3];
    INT8U PhaseC0[3];
    INT8U UA[3]; //电压系数
    INT8U UB[3];
    INT8U UC[3];
    INT8U IA[3]; //电流系数
    INT8U IB[3];
    INT8U IC[3];
    INT8U I0[3];
    INT8U PA[3]; //有功系数
    INT8U PB[3];
    INT8U PC[3];
    INT8U UoffsetA[3]; //电压有效值offset校正
    INT8U UoffsetB[3];
    INT8U UoffsetC[3];
    INT8U IoffsetA[3]; //电流有效值offset校正
    INT8U IoffsetB[3];
    INT8U IoffsetC[3];
    INT8U Tpsoffset[3];  //温度
    INT32U HarmUCoef[3]; //谐波电压系数，校表过程中，将读取的基波值/220.0得到系数
    INT32U HarmICoef[3]; //谐波电流系数，校表过程中，将读取的基波值/1.5得到系数
    INT32U WireType;     //接线方式，0x1200：三相三，0x0600：三相四
} ACCoe_SAVE;

/*
 * 通过交采ATT7022E采样值计算的电能示值。电能示值是一个累计值，上电继续累加，保证不丢失。
 * 电能示值通过vsave保存到文件SAVE_NAME_ACDATA（"/nand/acdata/acdata.dat"）
 * 所有结构体的电能示值= 实际值*ENERGY_COEF。
 * 系数具体定义参照att7022e.h
 * */
typedef struct {
    INT16U crc;                        // CRC校验
    INT32U PosPa_All;                  //正向A相有功总电能示值
    INT32U PosPb_All;                  //正向B相有功总电能示值
    INT32U PosPc_All;                  //正向C相有功总电能示值
    INT32U PosPt_All;                  //正向有功总电能示值
    INT32U PosPt_Rate[MAXVAL_RATENUM]; //正向有功总四费率电能示值

    INT32U NegPa_All;                  //反向A相有功总电能示值
    INT32U NegPb_All;                  //反向B相有功总电能示值
    INT32U NegPc_All;                  //反向C相有功总电能示值
    INT32U NegPt_All;                  //反向有功总电能示值
    INT32U NegPt_Rate[MAXVAL_RATENUM]; //反向有功总四费率电能示值

    INT32U PosQa_All;                  //正向A相无功总电能示值
    INT32U PosQb_All;                  //正向B相无功总电能示值
    INT32U PosQc_All;                  //正向C相无功总电能示值
    INT32U PosQt_All;                  //正向无功总电能示值
    INT32U PosQt_Rate[MAXVAL_RATENUM]; //正向无功总四费率电能示值

    INT32U NegQa_All;                  //反向A相无功总电能示值
    INT32U NegQb_All;                  //反向B相无功总电能示值
    INT32U NegQc_All;                  //反向C相无功总电能示值
    INT32U NegQt_All;                  //反向无功总电能示值
    INT32U NegQt_Rate[MAXVAL_RATENUM]; //反向无功总四费率电能示值

    INT32U Q1_Qt_All;                  //一象限Quadrant无功总电能示值
    INT32U Q1_Qt_Rate[MAXVAL_RATENUM]; //一象限费率1-4无功总电能示值
    INT32U Q2_Qt_All;                  //二象限无功总电能示值
    INT32U Q2_Qt_Rate[MAXVAL_RATENUM]; //二象限费率1-4无功总电能示值
    INT32U Q3_Qt_All;                  //三象限无功总电能示值
    INT32U Q3_Qt_Rate[MAXVAL_RATENUM]; //三象限费率1-4无功总电能示值
    INT32U Q4_Qt_All;                  //四象限无功总电能示值
    INT32U Q4_Qt_Rate[MAXVAL_RATENUM]; //四象限费率1-4无功总电能示值
} ACEnergy_Sum;

#endif /* VACS_H_ */
