/*
 * ware.h
 *
 *  Created on: 2013-5-25
 *      Author: liuhongli
 */

#ifndef WARE_H_
#define WARE_H_

#define NUM_FFT  64
#define EXP_FFT  6    //(2)6=64

#include "StdDataType.h"

typedef struct {
	INT32S Real;		//实部
	INT32S Imag;		//虚部
} Lisan;

typedef struct {
	INT32S Ua[80];
	INT32S Ub[80];
	INT32S Uc[80];

	INT32S Ia[80];
	INT32S Ib[80];
	INT32S Ic[80];
} _WarePoint;

typedef struct {
	FP64 Ua[20];		//2--19次U相电压谐波值[2]-[19]；0,1不用
	FP64 Ub[20];		//2--19次V相电压谐波值
	FP64 Uc[20];		//2--19次W相电压谐波值
	FP64 Ia[20];		//2--19次U相电流谐波值
	FP64 Ib[20];		//2--19次V相电流谐波值
	FP64 Ic[20];		//2--19次W相电流谐波值

	FP64 HR_Ua[20];		//2--19次U相电压谐波含有率Harmonic Rate
	FP64 HR_Ub[20];		//2--19次V相电压谐波含有率
	FP64 HR_Uc[20];		//2--19次W相电压谐波含有率
	FP64 HR_Ia[20];		//2--19次U相电流谐波含有率
	FP64 HR_Ib[20];		//2--19次V相电流谐波含有率
	FP64 HR_Ic[20];		//2--19次W相电流谐波含有率

	FP64 Thd_Ia;		//A相电流总畸变率
	FP64 Thd_Ib;		//B相电流总畸变率
	FP64 Thd_Ic;		//C相电流总畸变率
	FP64 Thd_Ua;		//A相电压总畸变率
	FP64 Thd_Ub;		//B相电压总畸变率
	FP64 Thd_Uc;		//C相电压总畸变率

	//下面日统计需要计算量
	FP64 Thd_Max_Ia;		//A相电流总畸变率日最大值
	FP64 Thd_Max_Ib;		//B相电流总畸变率
	FP64 Thd_Max_Ic;		//C相电流总畸变率
	FP64 Thd_Max_Ua;		//A相电压总畸变率
	FP64 Thd_Max_Ub;		//B相电压总畸变率
	FP64 Thd_Max_Uc;		//C相电压总畸变率
	//总畸变率日最大值及发生时间
	TS Thd_MaxTime_Ia;	//A相电流总畸变率日最大值发生时间
	TS Thd_MaxTime_Ib;	//B相电流
	TS Thd_MaxTime_Ic;	//C相电流
	TS Thd_MaxTime_Ua;	//A相电压
	TS Thd_MaxTime_Ub;	//B相电压
	TS Thd_MaxTime_Uc;	//C相电压

	FP64 HarmMax_Ia[20];  //U相电流谐波最大值
	FP64 HarmMax_Ib[20];  //V相电流谐波最大值
	FP64 HarmMax_Ic[20];  //W相电流谐波最大值
	TS HarmMaxTime_Ia[20];   //U相电流谐波最大发生时间
	TS HarmMaxTime_Ib[20];   //V相电流谐波最大发生时间
	TS HarmMaxTime_Ic[20];   //W相电流谐波最大发生时间

	FP64 HRMax_Ua[20];  			//	U相电电压谐波含有率最大值
	FP64 HRMax_Ub[20];
	FP64 HRMax_Uc[20];
	TS HRMaxTime_Ua[20];   //U相电压谐波含有率最大发生时间
	TS HRMaxTime_Ub[20];
	TS HRMaxTime_Uc[20];
} _HarmonicD;

void GetHarmonicCoef();
void ffttest(INT32S *pointbuf, INT16U *xb);
void calc_harm_value(_WarePoint ware);
extern void read_harmgain_cfg();
extern void read_ware_regist(TS ts, int fp, INT8U adjuststa);
extern void ware_process(TS ts, INT8U *askharm);
extern void fft_daoxu(INT32S *dataR);

#endif /* WARE_H_ */
