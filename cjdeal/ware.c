/*
 * ware.c
 * 谐波计算
 *
 *  Created on: 2013-5-23
 *      Author: Administrator
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>

#include "StdDataType.h"
#include "PublicFunction.h"
#include "Shmem.h"
#include "att7022e.h"
#include "spi.h"
#include "ware.h"

extern _RealData realdata;
#if (NUM_FFT == 64)
const INT32S sin_tab[64] = { 0, 3212, 6393, 9512, 12539, 15446, 18204, 20787,
		23170, 25329, 27245, 28898, 30273, 31356, 32137, 32609, 32767, 32609,
		32137, 31356, 30273, 28898, 27245, 25329, 23170, 20787, 18204, 15446,
		12539, 9512, 6393, 3212, 0, -3212, -6393, -9512, -12539, -15446, -18204,
		-20787, -23170, -25329, -27245, -28898, -30273, -31356, -32137, -32609,
		-32767, -32609, -32137, -31356, -30273, -28898, -27245, -25329, -23170,
		-20787, -18204, -15446, -12539, -9512, -6393, -3212 };

const INT32S cos_tab[64] = { 32767, 32609, 32137, 31356, 30273, 28898, 27245,
		25329, 23170, 20787, 18204, 15446, 12539, 9512, 6393, 3212, 0, -3212,
		-6393, -9512, -12539, -15446, -18204, -20787, -23170, -25329, -27245,
		-28898, -30273, -31356, -32137, -32609, -32767, -32609, -32137, -31356,
		-30273, -28898, -27245, -25329, -23170, -20787, -18204, -15446, -12539,
		-9512, -6393, -3212, 0, 3212, 6393, 9512, 12539, 15446, 18204, 20787,
		23170, 25329, 27245, 28898, 30273, 31356, 32137, 32609 };

const INT8U DXTable[64] = { 0, 32, 16, 48, 8, 40, 24, 56, 4, 36, 20, 52, 12, 44,
		28, 60, 2, 34, 18, 50, 10, 42, 26, 58, 6, 38, 22, 54, 14, 46, 30, 62,

		1, 33, 17, 49, 9, 41, 25, 57, 5, 37, 21, 53, 13, 45, 29, 61, 3, 35, 19,
		51, 11, 43, 27, 59, 7, 39, 23, 55, 15, 47, 31, 63 };
#endif
//0-21次谐波分析谐波增益系数值
static FP64 HarmGainCoef[22] =
		{ 1.00000000000000, 1.00000000000000, 1.00362187060665,
				1.00969162604172, 1.01825901332331, 1.02939520355364,
				1.04319331488342, 1.05977378492696, 1.07927443401769,
				1.10187021533519, 1.12776405100967, 1.15718387269867,
				1.19042304659018, 1.22777858879375, 1.26962990669109,
				1.31639900106378, 1.36856044411429, 1.42671649331348,
				1.49149037623292, 1.56363705732262, 1.64398947583697,
				1.73366802372421, };

/*
 *读谐波增益补偿系数
 *	"/nor/config/harmgain.cfg"文件中逗号前面谐波次数，逗号后面是谐波增益系数
 */
void read_harmgain_cfg() {
	FILE *fp = NULL;
	INT8U filename[64];
	INT8U i;
	int temp;		//谐波次数
	FP64 gainval;	//补偿值

	memset(filename, 0, sizeof(filename));
	sprintf((char *) filename, "%s/harmgain.cfg", _ACSDIR_);
	fp = fopen((char *) filename, "r");
	if (fp != NULL) {
		for (i = 0; i < 22; i++) {
			fscanf(fp, "%d,%lf", &temp, &gainval);
//			fprintf(stderr,"temp=%d,gainval=%d\n",temp,gainval);
			if (temp >= 0 && temp < 22)
				HarmGainCoef[temp] = gainval;
		}
		fclose(fp);
	} else {
		fprintf(stderr, "无谐波补偿值配置文件，选择默认配置\n");
	}
//	for(i=0;i<22;i++) {
//		fprintf(stderr,"交采谐波：次数：%d, 补偿值：%lf\n",i,HarmGainCoef[i]);
//	}
}

_HarmonicD HarmData;
_WarePoint WarePoint;
ACCoe_SAVE attCoef;		//校表系数

/*********************************/
//读出谐波采样点并且处理它
//********************************/
INT32S read_ware_point(INT32S fp) {
	INT32S readc = 0;

	readc = att_spi_read(fp, 0x7f, 3);
	if (readc > 32768)
		readc = readc - 65536;
	return readc;
}

//**************************************************/
// adjuststa:校表设置状态
// 使能电压\电流通道的谐波测量功能
// 0xa 电压通道 按照 ua ub uc的顺序放采样点
// 0xb 电流通道 按照 ia ib ic的顺序放采样点
//**************************************************/
void read_ware_regist(TS ts, int fp, INT8U adjuststa) {
	static TS lastts;
	static INT8U initflag = 1;
	INT8U i;
	INT8U temp[3];

	if (initflag) {
		lastts.Minute = ts.Minute;
		initflag = 0;
	} else if (lastts.Minute != ts.Minute || adjuststa == 1) {
		lastts.Minute = ts.Minute;
		//允许写操作
		temp[0] = 0;
		temp[1] = 0;
		temp[2] = 0x5a;
		att_spi_write(fp, Reg_Enable, 3, temp);
		//停止同步数据功能
		temp[0] = 0x00;
		temp[1] = 0x00;
		temp[2] = 0x00;
		att_spi_write(fp, Reg_SyncData, 3, temp);
		//启动自动同步数据功能
		temp[0] = 0x00;
		temp[1] = 0x00;
		temp[2] = 0x02;
		att_spi_write(fp, Reg_SyncData, 3, temp);
		usleep(100000);
//		fprintf(stderr,"80个点Ua电压数据");
		for (i = 0; i < 80; i++) {
			WarePoint.Ua[i] = read_ware_point(fp);
//			if(i%16==0) fprintf(stderr,"\n");
//			fprintf(stderr,"%d ",WarePoint.Ua[i]);
			WarePoint.Ub[i] = read_ware_point(fp);
			WarePoint.Uc[i] = read_ware_point(fp);
			WarePoint.Ia[i] = read_ware_point(fp);
			WarePoint.Ib[i] = read_ware_point(fp);
			WarePoint.Ic[i] = read_ware_point(fp);
			read_ware_point(fp);
		}
		temp[0] = 0;
		temp[1] = 0;
		temp[2] = 1;
		att_spi_write(fp, Reg_Enable, 3, temp); //不允许写操作
	}
}

/***********************************************************/
//fft算法。
//*入口参数：经过组合以后的采样点，出口参数：实部虚部
/***********************************************************/
void FFT_Test(INT32S *dataR, Lisan *some) {
	int k, L, j, b, p, i, temp;
	INT32S TR, TI;
	INT32S XRC, XRS, XIC, XIS, AXRI, SXRI;
	INT32S dataI[NUM_FFT];

	memset(dataI, 0, sizeof(dataI));
	for (L = 1; L <= EXP_FFT; ++L) { /* for(1) */
		b = 1;
		i = L - 1;
		if (i > 0)
			b = b << (i);
		for (j = 0; j <= b - 1; j++) { /* for(2) */
			p = 1;
			i = EXP_FFT - L;
			if (i > 0)
				p = p << (i);
			p = p * j;
			for (k = j; k < NUM_FFT; k += (b << 1)) { /* for(3) */

				TR = dataR[k];
				TI = dataI[k];
				temp = k + b;

				XRC = ((INT64S) dataR[temp] * cos_tab[p]) >> 15;
				XRS = ((INT64S) dataR[temp] * sin_tab[p]) >> 15;
				XIC = ((INT64S) dataI[temp] * cos_tab[p]) >> 15;
				XIS = ((INT64S) dataI[temp] * sin_tab[p]) >> 15;

				AXRI = XRC + XIS;
				SXRI = XRS - XIC;
				dataR[k] = TR + AXRI; //(XRC+XIS);
				dataI[k] = TI - SXRI; //(XRS-XIC);
				dataR[temp] = TR - AXRI; //(XRC+XIS);
				dataI[temp] = TI + SXRI; //(XRS-XIC);
			}
		}
	}
	for (i = 0; i < 20; i++) { /* 只需要20次以下的谐波进行分析 */
		some[i].Real = dataR[i];
		some[i].Imag = dataI[i];
	}
}

void fft_daoxu(INT32S *dataR) {
	INT8U a, b, i;
	INT32S Temp;
	for (i = 1; i < NUM_FFT; i++) {
		a = i;
		b = DXTable[i];
		if (b > a) {
			Temp = dataR[a];
			dataR[a] = dataR[b];
			dataR[b] = Temp;
		}
	}
}

//ffttest  总函数
//入口参数：离散采样点
//出口参数：谐波
void ffttest(INT32S *pointbuf, INT16U *xb) {
	INT8U i;
	INT64U tempasd;
	INT32S temp1, temp2;
	INT32S chazhibuf[64];   //64点fft
	Lisan allsome[20];
	TS ts;

	TSGet(&ts);
//	chazhilage(chazhibuf,pointbuf);
	memcpy(chazhibuf, pointbuf, sizeof(chazhibuf));

	fft_daoxu(chazhibuf);
	FFT_Test(chazhibuf, allsome);

	for (i = 0; i < 20; i++) {
		temp1 = (allsome[i].Real) / 10;
		temp2 = (allsome[i].Imag) / 10;
		tempasd = (INT64U) (temp1 * temp1 + temp2 * temp2);
		xb[i] = sqrt(tempasd);
	}
}

/*
 * 计算谐波值及含有率
 * 输入参数：ware：谐波采样值，
 */
void calc_harm_value(_WarePoint ware) {
	int i = 0;
	INT16U TempUa[20];   //2--19次U相电压谐波值
	INT16U TempUb[20];   //2--19次V相电压谐波值
	INT16U TempUc[20];   //2--19次W相电压谐波值
	INT16U TempIa[20];   //2--19次U相电流谐波值
	INT16U TempIb[20];   //2--19次V相电流谐波值
	INT16U TempIc[20];   //2--19次W相电流谐波值

	//计算2-19次谐波电压
	ffttest(ware.Ua, TempUa);
	ffttest(ware.Ub, TempUb);
	ffttest(ware.Uc, TempUc);
	ffttest(ware.Ia, TempIa);
	ffttest(ware.Ib, TempIb);
	ffttest(ware.Ic, TempIc);

	//1次值为基波值
	for (i = 1; i < 20; i++) {   	//计算谐波总含有率及各次谐波含有率和谐波值
		if (abs(TempUa[1]) > 60) {
			HarmData.Ua[i] = (TempUa[i] * HarmGainCoef[i] / TempUa[1]
					* realdata.LineUa) / U_COEF;
			HarmData.HR_Ua[i] = HarmData.Ua[i] / HarmData.Ua[1];
		} else {
			HarmData.HR_Ua[i] = 0;
			HarmData.Ua[i] = 0;
		}
		if (abs(TempUb[1]) > 60) {
			HarmData.Ub[i] = (TempUb[i] * HarmGainCoef[i] / TempUb[1]
					* realdata.LineUb) / U_COEF;
			HarmData.HR_Ub[i] = HarmData.Ub[i] / HarmData.Ub[1];
		} else {
			HarmData.HR_Ub[i] = 0;
			HarmData.Ub[i] = 0;
		}
		if (abs(TempUc[1]) > 60) {
			HarmData.Uc[i] = (TempUc[i] * HarmGainCoef[i] / TempUc[1]
					* realdata.LineUc) / U_COEF;
			HarmData.HR_Uc[i] = HarmData.Uc[i] / HarmData.Uc[1];
		} else {
			HarmData.HR_Uc[i] = 0;
			HarmData.Uc[i] = 0;
		}
		if (abs(TempIa[1] > 10)) {
			if (attCoef.HarmICoef[0] != 0) {
				HarmData.Ia[i] = (TempIa[i] * HarmGainCoef[i] / TempIa[1]
						* realdata.LineIa) / I_COEF;
				HarmData.HR_Ia[i] = HarmData.Ia[i] / HarmData.Ia[1];
			}
		} else {
			HarmData.HR_Ia[i] = 0;
			HarmData.Ia[i] = 0;
		}
		if (abs(TempIb[1] > 10)) {
			HarmData.Ib[i] = (TempIb[i] * HarmGainCoef[i] / TempIb[1]
					* realdata.LineIb) / I_COEF;
			HarmData.HR_Ib[i] = HarmData.Ib[i] / HarmData.Ib[1];
		} else {
			HarmData.HR_Ib[i] = 0;
			HarmData.Ib[i] = 0;
		}
		if (abs(TempIc[1] > 10)) {
			HarmData.Ic[i] = (TempIc[i] * HarmGainCoef[i] / TempIc[1]
					* realdata.LineIc) / I_COEF;
			HarmData.HR_Ic[i] = HarmData.Ic[i] / HarmData.Ic[1];
		} else {
			HarmData.HR_Ic[i] = 0;
			HarmData.Ic[i] = 0;
		}
	}
//	fprintf(stderr,"20次谐波值");
//	for (i = 0; i < 20; i++) {   	//计算谐波总含有率及各次谐波含有率和谐波值
//		fprintf(stderr,"A相电压 %d次 谐波值 = %d ，有效值 = %f ,含有率计算 = %f",i,TempUa[i],HarmData.Ua[i],HarmData.HR_Ua[i]);
//	}
//	for (i = 0; i < 20; i++) {
//		fprintf(stderr,"A相电流 %d次 谐波值 = %d ，有效值 = %f ,含有率计算 = %f",i,TempIa[i],HarmData.Ia[i],HarmData.HR_Ia[i]);
//	}
}

/*
 * 计算谐波值及含有率
 * 输入参数：ware：谐波采样值，
 * 			Ua，Ub，Uc：基波电压值
 * 			Ia，Ib，Ic：基波电流值
 */
void calc_harm_val(_WarePoint ware, INT32U Ua, INT32U Ub, INT32U Uc, INT32U Ia,
		INT32U Ib, INT32U Ic) {
	int i = 0;
	INT16U TempUa[20];   	//2--19次U相电压谐波值
	INT16U TempUb[20];   	//2--19次V相电压谐波值
	INT16U TempUc[20];   	//2--19次W相电压谐波值
	INT16U TempIa[20];   	//2--19次U相电流谐波值
	INT16U TempIb[20];   	//2--19次V相电流谐波值
	INT16U TempIc[20];   	//2--19次W相电流谐波值

	//计算2-19次谐波电压
	ffttest(ware.Ua, TempUa);
	ffttest(ware.Ub, TempUb);
	ffttest(ware.Uc, TempUc);
	ffttest(ware.Ia, TempIa);
	ffttest(ware.Ib, TempIb);
	ffttest(ware.Ic, TempIc);

	for (i = 0; i < 20; i++) {   	//计算谐波总含有率及各次谐波含有率和谐波值
		if (abs(TempUa[1]) > 60) {
			HarmData.HR_Ua[i] = TempUa[i] * 100.0 / TempUa[1];
			HarmData.Ua[i] = TempUa[i] * 1.0 / TempUa[1] * Ua / U_COEF;
		} else {
			HarmData.HR_Ua[i] = 0;
			HarmData.Ua[i] = 0;
		}
		if (abs(TempUb[1]) > 60) {
			HarmData.HR_Ub[i] = TempUb[i] * 100.0 / TempUb[1];
			HarmData.Ub[i] = TempUb[i] * 1.0 / TempUb[1] * Ub / U_COEF;
		} else {
			HarmData.HR_Ub[i] = 0;
			HarmData.Ub[i] = 0;
		}
		if (abs(TempUc[1]) > 60) {
			HarmData.HR_Uc[i] = TempUc[i] * 100.0 / TempUc[1];
			HarmData.Uc[i] = TempUc[i] * 1.0 / TempUc[1] * Uc / U_COEF;
		} else {
			HarmData.HR_Uc[i] = 0;
			HarmData.Uc[i] = 0;
		}
		if (abs(TempIa[1] > 10)) {
			HarmData.HR_Ia[i] = TempIa[i] * 100.0 / TempIa[1];
			HarmData.Ia[i] = TempIa[i] * 1.0 / TempIa[1] * Ia / I_COEF;
		} else {
			HarmData.HR_Ia[i] = 0;
			HarmData.Ia[i] = 0;
		}
		if (abs(TempIb[1] > 10)) {
			HarmData.HR_Ib[i] = TempIb[i] * 100.0 / TempIb[1];
			HarmData.Ib[i] = TempIb[i] * 1.0 / TempIb[1] * Ib / I_COEF;
		} else {
			HarmData.HR_Ib[i] = 0;
			HarmData.Ib[i] = 0;
		}
		if (abs(TempIc[1] > 10)) {
			HarmData.HR_Ic[i] = TempIc[i] * 100.0 / TempIc[1];
			HarmData.Ic[i] = TempIc[i] * 1.0 / TempIc[1] * Ic / I_COEF;
		} else {
			HarmData.HR_Ic[i] = 0;
			HarmData.Ic[i] = 0;
		}
	}
//	fprintf(stderr,"20次谐波值");
//	for (i = 0; i < 20; i++) {   	//计算谐波总含有率及各次谐波含有率和谐波值
//		fprintf(stderr,"A相电压 %d次 谐波值 = %f ，含有率计算 = %f",i,(double)TempUa[i]/116.5,HarmData.HR_Ua[i]);
//	}
//	for (i = 0; i < 20; i++) {
//		fprintf(stderr,"A相电流 %d次 谐波值 = %f ，含有率计算 = %f",i,(double)TempIa[i]/7732,HarmData.HR_Ia[i]);
//	}
}

//计算谐波总畸变率
void calc_harm_thd() {
	INT8U i;
	float TempThd[6];
	for (i = 0; i < 6; i++)
		TempThd[i] = 0;
	for (i = 2; i < 20; i++) {	//总畸变率=各次谐波含有率的平方和的开方
		TempThd[0] = TempThd[0] + HarmData.HR_Ua[i] * HarmData.HR_Ua[i];
		TempThd[1] = TempThd[1] + HarmData.HR_Ub[i] * HarmData.HR_Ub[i];
		TempThd[2] = TempThd[2] + HarmData.HR_Uc[i] * HarmData.HR_Uc[i];
		TempThd[3] = TempThd[3] + HarmData.HR_Ia[i] * HarmData.HR_Ia[i];
		TempThd[4] = TempThd[4] + HarmData.HR_Ib[i] * HarmData.HR_Ib[i];
		TempThd[5] = TempThd[5] + HarmData.HR_Ic[i] * HarmData.HR_Ic[i];
	}
	HarmData.Thd_Ua = sqrt(TempThd[0]);	//计算A相电压总畸变率
	HarmData.Thd_Ub = sqrt(TempThd[1]);	//计算B相电压总畸变率
	HarmData.Thd_Uc = sqrt(TempThd[2]);	//计算C相电压总畸变率
	HarmData.Thd_Ia = sqrt(TempThd[3]);		//计算A相电流总畸变率
	HarmData.Thd_Ib = sqrt(TempThd[4]);		//计算B相电流总畸变率
	HarmData.Thd_Ic = sqrt(TempThd[5]);		//计算C相电流总畸变率
	fprintf(stderr, "A相电压总畸变率=%f\n", HarmData.Thd_Ua);
	fprintf(stderr, "A相电流总畸变率=%f\n", HarmData.Thd_Ia);
}

//计算谐波最大值及发生时间
//TODO:跨日清楚数据
void calc_harm_max() {
	INT8U i;
	for (i = 2; i < 20; i++) {
		if (HarmData.Ia[i] > HarmData.HarmMax_Ia[i]) {	//U相2--19次电流谐波最大值及发生时间
			HarmData.HarmMax_Ia[i] = HarmData.Ia[i];
			TSGet(&HarmData.HarmMaxTime_Ia[i]);
		}
		if (HarmData.Ib[i] > HarmData.HarmMax_Ib[i]) {
			HarmData.HarmMax_Ib[i] = HarmData.Ib[i];
			TSGet(&HarmData.HarmMaxTime_Ib[i]);
		}
		if (HarmData.Ic[i] > HarmData.HarmMax_Ic[i]) {
			HarmData.HarmMax_Ic[i] = HarmData.Ic[i];
			TSGet(&HarmData.HarmMaxTime_Ic[i]);
		}
		//A\B\C相2--19次谐波电压含有率最大值
		if (HarmData.HR_Ua[i] > HarmData.HRMax_Ua[i]) {
			HarmData.HRMax_Ua[i] = HarmData.HR_Ua[i];
			TSGet(&HarmData.HRMaxTime_Ua[i]);
		}
		if (HarmData.HR_Ub[i] > HarmData.HRMax_Ub[i]) {
			HarmData.HRMax_Ub[i] = HarmData.HR_Ub[i];
			TSGet(&HarmData.HRMaxTime_Ub[i]);
		}
		if (HarmData.HR_Uc[i] > HarmData.HRMax_Uc[i]) {
			HarmData.HRMax_Uc[i] = HarmData.HR_Uc[i];
			TSGet(&HarmData.HRMaxTime_Uc[i]);
		}
	}
	//A\B\C相谐波总畸变电流最大值
}

/**
 * 跨日清除谐波最大值
 */
void clean_harm_max() {
	INT8U i;
	for (i = 2; i < 20; i++) {
		HarmData.HarmMax_Ia[i] = 0;
		HarmData.HarmMax_Ib[i] = 0;
		HarmData.HarmMax_Ic[i] = 0;
		HarmData.HRMax_Ua[i] = 0;
		HarmData.HRMax_Ub[i] = 0;
		HarmData.HRMax_Uc[i] = 0;
		TSGet(&HarmData.HarmMaxTime_Ia[i]);
		TSGet(&HarmData.HarmMaxTime_Ib[i]);
		TSGet(&HarmData.HarmMaxTime_Ic[i]);
		TSGet(&HarmData.HRMaxTime_Ua[i]);
		TSGet(&HarmData.HRMaxTime_Ub[i]);
		TSGet(&HarmData.HRMaxTime_Uc[i]);
	}
}
/**
 * 谐波采样处理过程
 */
void ware_process(TS ts, INT8U *askharm) {
	static INT8U initflag = 1;
	static TS lastts;
	if (initflag) {
		lastts.Minute = ts.Minute;
		initflag = 0;
	}
	if (ts.Day != lastts.Day) {
		clean_harm_max();	//跨日清除谐波最大值
	}
	if (ts.Minute != lastts.Minute || *askharm == 1) {		//1sec采样一次
		*askharm = 0;
		//fprintf(stderr,"\nenter down");
		lastts.Minute = ts.Minute;
//		calc_harm_val(WarePoint,realdata.LineUa,realdata.LineUb,realdata.LineUc,realdata.LineIa,realdata.LineIb,realdata.LineIc);
		calc_harm_value(WarePoint);		//计算谐波有效值及含有率
		calc_harm_thd();	//计算谐波总畸变率
		calc_harm_max();	//计算谐波最大值
	}
}
