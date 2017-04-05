/*
 * acs.c
 *
 *  Created on: Feb 3, 2017
 *      Author: ava
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <syslog.h>

#include "AccessFun.h"
#include "StdDataType.h"
#include "PublicFunction.h"
#include "Objectdef.h"
#include "Shmem.h"
#include "spi.h"
#include "att7022e.h"
#include "rn8209.h"
#include "main.h"
extern ProgramInfo* JProgramInfo ;
#define 	W_APQ0		8
#define 	W_APQ1		1
#define 	W_APQ2		7

#define EC			6400
#define N_CONST			80
//HFConst=INT[25920000000*G*G*Vu*Vi/(EC*Un*Ib)]  G=1.163   EC=6400  Vu=0.22  Vi=0.1  Un=220   Ib=1.5
//6A/1.5A-5mA   0.005*20k = 0.1
//Vu=0.22  Vi=0.002  Un=220   Ib=1		HFConst = 365.19363  INT[]=365
//#define HFconst		0x16D	//三相四

//HFConst=INT[25920000000*G*G*Vu*Vi/(EC*Un*Ib)]  G=1.163   EC=6400  154257789.312/1408000
//20A/1A-1mA   0.001*20k = 0.02
//Vu=0.22  Vi=0.02  Un=220   Ib=1		HFConst = 109.558089  INT[]=109
#define HFconst		365	//三相四   6d

//HFConst=INT[25920000000*G*G*Vu*Vi/(EC*Un*Ib)]  G=1.163   EC=6400
//20A/1A-1mA   0.001*30k = 0.03
//Vu=0.22  Vi=0.03  Un=220   Ib=1		HFConst = 164.3371335  INT[]=164
//#define HFconst		0xa4	//三相四

//2^23 = 8388608   10^10=10000000000
//PQCoef=(HFConst*EC*2^23)/(2.592*10^10)=(365*6400*2^23)/(2.592*10^10)   =1.959578829×10¹³/
//6A/1.5A 互感器
#define PQCoef		756.01035      //三相四
//20A/1A 互感器
//#define PQCoef		225.767474576		     //三相四

#define AV_COUNT 	10;			//RN8209计量芯片 校正电压采样次数

float		Preal[3];		//0:A相，1：B相，2：C相
float		Qreal[3];
float		Ureal[3];
float		Ireal[3];

INT32S		spifp=-1;					//ATT7022E打开spi句柄
ACCoe_SAVE	 attCoef={};				//校表系数
INT32U 		K_vrms=1;	        		//RN8209读取电压的系数
INT32S		VersionID=-1;		//att7022e 改版记录
INT32U		WireType;		//ATT7022e 接线方式，0x0600:三相四，0x1200：三相三

//交采系数
typedef struct {
	INT16U	crc;					//CRC校验
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
}ACCoe_SAVE_3761;

void realdataprint(_RealData realdata)
{
		fprintf(stderr,"电压(V):	UA=%.1f, UB=%.1f, UC=%.1f\n",
				(float)realdata.Ua/U_COEF,(float)realdata.Ub/U_COEF,(float)realdata.Uc/U_COEF);
		fprintf(stderr,"电流(A):	IA=%.3f, IB=%.3f, IC=%.3f, IL=%.3f\n",
				(float)realdata.Ia/I_COEF,(float)realdata.Ib/I_COEF,(float)realdata.Ic/I_COEF,(float)realdata.I0/I_COEF);
		fprintf(stderr,"有功(kW):	PT=%.1f, PA=%.1f, PB=%.1f, PC=%.1f\n",
				(float)realdata.Pt/P_COEF,(float)realdata.Pa/10,
				(float)realdata.Pb/P_COEF,(float)realdata.Pc/10);
		fprintf(stderr,"无功(kVar):	QZ=%.1f, QA=%.1f, QB=%.1f, QC=%.1f\n",
				(float)realdata.Qt/Q_COEF,(float)realdata.Qa/Q_COEF,
				(float)realdata.Qb/Q_COEF,(float)realdata.Qc/Q_COEF);
		fprintf(stderr,"视在(kW):	SZ=%.1f, SA=%.1f, SB=%.1f, SC=%.1f\n",
				(float)realdata.St/S_COEF,(float)realdata.Sa/S_COEF,
				(float)realdata.Sb/S_COEF,(float)realdata.Sc/S_COEF);
		fprintf(stderr,"功率因数:	  Cos=%.3f, CosA=%.3f, CosB=%.3f, CosC=%.3f\n",
				(float)realdata.Cos/COS_COEF,(float)realdata.CosA/COS_COEF,
				(float)realdata.CosB/COS_COEF,(float)realdata.CosC/COS_COEF);
		fprintf(stderr,"相角:	        Pga=%.1f, Pgb=%.1f, Pgc=%.1f\n",
				(float)realdata.Pga/PHASE_COEF,(float)realdata.Pgb/PHASE_COEF,
				(float)realdata.Pgc/PHASE_COEF);
		fprintf(stderr,"电压夹角:	YUaUb=%.1f, YUaUc=%.1f, YUbUc=%.1f\n",
				(float)realdata.YUaUb/ANGLE_COEF,(float)realdata.YUaUc/ANGLE_COEF,
				(float)realdata.YUbUc/ANGLE_COEF);
		fprintf(stderr,"频率:      Freq=%.2f\n",(float)realdata.Freq/FREQ_COEF);
		fprintf(stderr,"1min平均电压：UA=%.1f, UB=%.1f, UC=%.1f\n",
				(float)realdata.AvgUa/U_COEF,(float)realdata.AvgUb/U_COEF,
				(float)realdata.AvgUc/U_COEF);
		fprintf(stderr,"基波电压:	UA=%.1f, UB=%.1f, UC=%.1f\n",
				(float)realdata.LineUa/U_COEF,(float)realdata.LineUb/U_COEF,(float)realdata.LineUc/U_COEF);
		fprintf(stderr,"基波电流:	IA=%.3f, IB=%.3f, IC=%.3f\n",
				(float)realdata.LineIa/I_COEF,(float)realdata.LineIb/I_COEF,(float)realdata.LineIc/I_COEF);
		fprintf(stderr,"相序标识.功率方向:    SFlag=%02x   PFlag=%02x\n",realdata.SFlag,realdata.PFlag);
		fprintf(stderr,"温度:	  Temp=%d\n",realdata.Temp);
		fprintf(stderr,"\n\r");
}

void acs_energysum_print(ACEnergy_Sum energysum)
{
	fprintf(stderr,"总电能示值(kWh,kVarh):PosPt=%d, NegPt=%d, PosQt=%d, NegQt=%d\n",
			energysum.PosPt_All,energysum.NegPt_All,energysum.PosQt_All,energysum.NegQt_All);
	fprintf(stderr,"正向有功:Pa=%d, Pb=%d, Pc=%d\n",
			energysum.PosPa_All,energysum.PosPb_All,energysum.PosPc_All);
	fprintf(stderr,"反向有功:	Pa=%d, Pb=%d, Pc=%d\n",
			energysum.NegPa_All,energysum.NegPb_All,energysum.NegPc_All);
	fprintf(stderr,"正向无功:	Qa=%d, Qb=%d, Qc=%d\n",
			energysum.PosQa_All,energysum.PosQb_All,energysum.PosQc_All);
	fprintf(stderr,"反向无功:	Qa=%d, Qb=%d, Qc=%d\n",
			energysum.NegQa_All,energysum.NegQb_All,energysum.NegQc_All);
	fprintf(stderr,"正向有功费率:P1=%d, P2=%d, P3=%d, P4=%d\n",
			energysum.PosPt_Rate[0],energysum.PosPt_Rate[1],
			energysum.PosPt_Rate[2],energysum.PosPt_Rate[3]);
	fprintf(stderr,"反向有功费率:P1=%d, P2=%d, P3=%d, P4=%d\n\r",
			energysum.NegPt_Rate[0],energysum.NegPt_Rate[1],
			energysum.NegPt_Rate[2],energysum.NegPt_Rate[3]);
	fprintf(stderr,"正向无功费率:Q1=%d, Q2=%d, Q3=%d, Q4=%d\n\r",
			energysum.PosQt_Rate[0],energysum.PosQt_Rate[1],
			energysum.PosQt_Rate[2],energysum.PosQt_Rate[3]);
	fprintf(stderr,"反向无功费率:Q1=%d, Q2=%d, Q3=%d, Q4=%d\n\r",
			energysum.NegQt_Rate[0],energysum.NegQt_Rate[1],
			energysum.NegQt_Rate[2],energysum.NegQt_Rate[3]);
	fprintf(stderr,"象限1：总及四费率1:Qt=%d,Q1=%d,Q2=%d,Q3=%d,Q4=%d\n",
			energysum.Q1_Qt_All,energysum.Q1_Qt_Rate[0],energysum.Q1_Qt_Rate[1],
			energysum.Q1_Qt_Rate[2],energysum.Q1_Qt_Rate[3]);
	fprintf(stderr,"象限2：总及四费率2:Qt=%d,Q1=%d,Q2=%d,Q3=%d,Q4=%d\n",
			energysum.Q2_Qt_All,energysum.Q2_Qt_Rate[0],energysum.Q2_Qt_Rate[1],
			energysum.Q2_Qt_Rate[2],energysum.Q2_Qt_Rate[3]);
	fprintf(stderr,"象限3：总及四费率3:Qt=%d,Q1=%d,Q2=%d,Q3=%d,Q4=%d\n",
			energysum.Q3_Qt_All,energysum.Q3_Qt_Rate[0],energysum.Q3_Qt_Rate[1],
			energysum.Q3_Qt_Rate[2],energysum.Q3_Qt_Rate[3]);
	fprintf(stderr,"象限4：总及四费率4:Qt=%d,Q1=%d,Q2=%d,Q3=%d,Q4=%d\n",
			energysum.Q4_Qt_All,energysum.Q4_Qt_Rate[0],energysum.Q4_Qt_Rate[1],
			energysum.Q4_Qt_Rate[2],energysum.Q4_Qt_Rate[3]);
	fprintf(stderr,"\n\r");
}


/*ATT7022E校表寄存器值打印*/
void acs_regdata_print()
{
	int		val;
	INT8U temp[3],i;
	INT32S	REC[255];
	sem_t * sem_fd=NULL;

	sem_fd = open_named_sem(SEMNAME_SPI0_0);
	sem_wait(sem_fd);
	memset(REC,0,sizeof(REC));
	temp[0] = 0;
	temp[1] = 0;
	temp[2] = 0x5a;
	att_spi_write(spifp, Reg_Read, 3, temp); //校表允许读操作

	for (i = 0; i < 0x38; i++)
	{
		REC[i] = att_spi_read(spifp, i, 3);
		fprintf(stderr,"REC[%02x]=%x\n",i,REC[i]);
	}
	for (i = w_Iregion1; i <= w_PhSregCpq2; i++) {
		REC[i] = att_spi_read(spifp, i, 3);
		fprintf(stderr,"REC[%02x]=%x\n",i,REC[i]);
	}

	temp[0] = 0;
	temp[1] = 0;
	temp[2] = 1;
	att_spi_write(spifp, Reg_Read, 3, temp); //不允许读操作

	fprintf(stderr,"\n\r读7022E校表寄存器值");
	fprintf(stderr,"\n\r 相角 校表系数\n");
	fprintf(stderr,"w_PhSregApq0=%06x\n",REC[w_PhSregApq0]);
	fprintf(stderr,"w_PhSregBpq0=%06x\n",REC[w_PhSregBpq0]);
	fprintf(stderr,"w_PhSregCpq0=%06x\n",REC[w_PhSregCpq0]);
	fprintf(stderr,"w_PhSregApq1=%06x\n",REC[w_PhSregApq1]);
	fprintf(stderr,"w_PhSregBpq1=%06x\n",REC[w_PhSregBpq1]);
	fprintf(stderr,"w_PhSregCpq1=%06x\n",REC[w_PhSregCpq1]);
	fprintf(stderr,"w_PhSregApq2=%06x\n",REC[w_PhSregApq2]);
	fprintf(stderr,"w_PhSregBpq2=%06x\n",REC[w_PhSregBpq2]);
	fprintf(stderr,"w_PhSregCpq2=%06x\n",REC[w_PhSregCpq2]);
	fprintf(stderr,"\n\r 电压 校表系数\n");
	fprintf(stderr,"UA=%06x\n",REC[w_UgainA]);
	fprintf(stderr,"UB=%06x\n",REC[w_UgainB]);
	fprintf(stderr,"UC=%06x\n",REC[w_UgainC]);
	fprintf(stderr,"\n\r 电流 校表系数\n");
	fprintf(stderr,"IA=%06x\n",REC[w_IgainA]);
	fprintf(stderr,"IB=%06x\n",REC[w_IgainB]);
	fprintf(stderr,"IC=%06x\n",REC[w_IgainC]);
	fprintf(stderr,"\n\r 功率 校表系数\n");
	fprintf(stderr,"PA=%06x\n",REC[w_PgainA]);
	fprintf(stderr,"PB=%06x\n",REC[w_PgainB]);
	fprintf(stderr,"PC=%06x\n",REC[w_PgainC]);
	fprintf(stderr,"\n\r 电流offset 校表系数\n");
	fprintf(stderr,"IOffsetA=%06x\n",REC[w_IaRmsoffse]);
	fprintf(stderr,"IOffsetB=%06x\n",REC[w_IbRmsoffse]);
	fprintf(stderr,"IOffsetC=%06x\n",REC[w_IcRmsoffse]);
	fprintf(stderr,"\n\r 相位 校表系数\n");
	fprintf(stderr,"w_Iregion=%06x\n",REC[w_Iregion]);
	fprintf(stderr,"w_Iregion1=%06x\n",REC[w_Iregion1]);

	fprintf(stderr,"\n\r-------------coef.par 文件保存----------------------\n");
	fprintf(stderr,"sizeof(ACCoe_SAVE)=%d\n",sizeof(ACCoe_SAVE));
	readCoverClass(0,0,&attCoef,sizeof(ACCoe_SAVE),acs_coef_save);
	fprintf(stderr,"UA系数:%02x-%02x-%02x\n",attCoef.UA[0],attCoef.UA[1],attCoef.UA[2]);
	fprintf(stderr,"UB系数:%02x-%02x-%02x\n",attCoef.UB[0],attCoef.UB[1],attCoef.UB[2]);
	fprintf(stderr,"UC系数:%02x-%02x-%02x\n",attCoef.UC[0],attCoef.UC[1],attCoef.UC[2]);
	fprintf(stderr,"IA系数:%02x-%02x-%02x\n",attCoef.IA[0],attCoef.IA[1],attCoef.IA[2]);
	fprintf(stderr,"IB系数:%02x-%02x-%02x\n",attCoef.IB[0],attCoef.IB[1],attCoef.IB[2]);
	fprintf(stderr,"IC系数:%02x-%02x-%02x\n",attCoef.IC[0],attCoef.IC[1],attCoef.IC[2]);
	sem_post(sem_fd);
	sem_getvalue(sem_fd, &val);
	fprintf(stderr,"process <vd> The sem is %d\n", val);
	fprintf(stderr,"process <vd> close a semaphore\n");
	sem_close(sem_fd);
}

void rn8209_regdata_print()
{
	INT32U	coef;
	fprintf(stderr,"RN8209 校表系数值读取\n");
	readCoverClass(0,0,&attCoef,sizeof(ACCoe_SAVE),acs_coef_save);
	coef = (attCoef.UA[0]<<16)+(attCoef.UA[1]<<8)+(attCoef.UA[2]);
	fprintf(stderr,"UCoef=%x-%x-%x(0x%x-----%d)\n",attCoef.UA[0],attCoef.UA[1],attCoef.UA[2],coef,coef);
	syslog(LOG_NOTICE,"读取RN8209校表系数=%d\n",coef);
}

void WriteRegInit(INT32S fp)
{
	INT8U temp[3];

	temp[0] = 0;
	temp[1] = 0;
	temp[2] = 0;
	att_spi_write(fp, Reg_Clean, 3, temp); //清校表寄存器

	temp[0] = 0;
	temp[1] = 0xb9;
	temp[2] = 0xfe;
	att_spi_write(fp, w_ModeCfg, 3, temp); //模式配置

	temp[0] = 0;
	temp[1] = 0x3d;//XIEBO			开启基波/谐波计量
	temp[2] = 0x84;//配置能量寄存器读后清零，bit7置1//系统推荐写入F8 04
	att_spi_write(fp, w_EMUCfg, 3, temp); //EMU单元配置

	temp[0] = 0;
	temp[1] = 0x34;
	temp[2] = 0x37;
	att_spi_write(fp, w_ModuleCFG, 3, temp); //模拟模块使能寄存器

	temp[0] = 0;
	temp[1] = 0;
	temp[2] = 0;
	att_spi_write(fp, w_QPhscal, 3, temp); //无功相位校正寄存器

//	temp[0] = 0x00;
//	temp[1] = 0x00;
//	temp[2] = 0x0A;
	temp[0] = (HFconst>>16) & 0xff;
	temp[1] = (HFconst>>8) & 0xff;
	temp[2] = HFconst & 0xff;
	fprintf(stderr,"w_Hfconst = %x_%x_%x",temp[0],temp[1],temp[2]);
	att_spi_write(fp, w_Hfconst, 3, temp); //高频脉冲常数

	if(VersionID == ATT_VER_ID) {
		temp[0] = 0x00;
		temp[1] = 0x13;
		temp[2] = 0x80;
		att_spi_write(fp, w_Iregion, 3, temp);  //相位补偿区域设置寄存器,1.5*130%=2A时进行相位补偿分段

		//Iregion=INT[Ib*N*33%*2^5]=1.5*80*0.33*2^5=1267=0x4F3
		temp[0] = 0x00;
		temp[1] = 0x04;
		temp[2] = 0xf3;
		att_spi_write(fp, w_Iregion1, 3, temp);  //相位补偿区域设置寄存器,1.5*33%=0.495A时进行相位补偿分段

		temp[0] = 0x00;
		temp[1] = 0x00;
		temp[2] = 0x0A;//VrefAuto_En=1,YModesel=1,QEnergySel=0
		att_spi_write(fp, w_EMCfg, 3, temp);  //新算法控制器
	}else {
		//Iregion=INT[Ib*N*33%*2^5]=1.5*80*0.33*2^5=1267=0x4f3
		temp[0] = 0x00;
		temp[1] = 0x04;
		temp[2] = 0xf3;
		att_spi_write(fp, w_Iregion, 3, temp);  //相位补偿区域设置寄存器,1.5*33%=0.495A时进行相位补偿分段
	}
}

/*
 * 清校表系数*/
void clean_coef(void)
{
	INT8U temp[3];
	INT8U	num;

	fprintf(stderr,"\n\r清校表系数\n\r");
	temp[0] = 0;
	temp[1] = 0;
	temp[2] = 0x5a;
	num = att_spi_write(spifp, 0xc9, 3, temp); //允许写操作
//	fprintf(stderr,"num=%d\n",num);
	WriteRegInit(spifp);

	temp[0] = 0;
	temp[1] = 0;
	temp[2] = 0;

	att_spi_write(spifp, w_PhSregApq0, 3, temp);//相位校正
	att_spi_write(spifp, w_PhSregBpq0, 3, temp);
	att_spi_write(spifp, w_PhSregCpq0, 3, temp);
	att_spi_write(spifp, w_PhSregApq1, 3, temp);
	att_spi_write(spifp, w_PhSregBpq1, 3, temp);
	att_spi_write(spifp, w_PhSregCpq1, 3, temp);
	if(VersionID == ATT_VER_ID) {
		att_spi_write(spifp, w_PhSregApq2, 3, temp);
		att_spi_write(spifp, w_PhSregBpq2, 3, temp);
		att_spi_write(spifp, w_PhSregCpq2, 3, temp);
	}
	att_spi_write(spifp, w_UgainA, 3, temp);
	att_spi_write(spifp, w_UgainB, 3, temp);
	att_spi_write(spifp, w_UgainC, 3, temp);

	att_spi_write(spifp, w_IgainA, 3, temp);
	att_spi_write(spifp, w_IgainB, 3, temp);
	att_spi_write(spifp, w_IgainC, 3, temp);

	att_spi_write(spifp, w_PgainA, 3, temp);
	att_spi_write(spifp, w_QgainA, 3, temp);
	att_spi_write(spifp, w_SgainA, 3, temp);

	att_spi_write(spifp, w_PgainB, 3, temp);
	att_spi_write(spifp, w_QgainB, 3, temp);
	att_spi_write(spifp, w_SgainB, 3, temp);

	att_spi_write(spifp, w_PgainC, 3, temp);
	att_spi_write(spifp, w_QgainC, 3, temp);
	att_spi_write(spifp, w_SgainC, 3, temp);

//	temp[0] = 0;
//	temp[1] = 0;
//	temp[2] = 7;
//	att_spi_write(spifp, w_IaRmsoffse, 3, temp);//Ioffset
//	att_spi_write(spifp, w_IbRmsoffse, 3, temp);
//	att_spi_write(spifp, w_IcRmsoffse, 3, temp);

	temp[0] = 0;
	temp[1] = 0;
	temp[2] = 1;
	att_spi_write(spifp, 0xc9, 3, temp); //不允许写操作
}

void clean_phase(void)
{
	INT8U temp[3];

	fprintf(stderr,"\n\r清相位寄存器\n\r");
	temp[0] = 0;
	temp[1] = 0;
	temp[2] = 0x5a;
	att_spi_write(spifp, 0xc9, 3, temp); //允许写操作

	temp[0] = 0;
	temp[1] = 0;
	temp[2] = 0;
	att_spi_write(spifp, w_PhSregApq0, 3, temp);//相位校正
	att_spi_write(spifp, w_PhSregBpq0, 3, temp);
	att_spi_write(spifp, w_PhSregCpq0, 3, temp);
	att_spi_write(spifp, w_PhSregApq1, 3, temp);
	att_spi_write(spifp, w_PhSregBpq1, 3, temp);
	att_spi_write(spifp, w_PhSregCpq1, 3, temp);
	att_spi_write(spifp, w_PhSregApq2, 3, temp);
	att_spi_write(spifp, w_PhSregBpq2, 3, temp);
	att_spi_write(spifp, w_PhSregCpq2, 3, temp);

	temp[0] = 0;
	temp[1] = 0;
	temp[2] = 1;
	att_spi_write(spifp, 0xc9, 3, temp); //不允许写操作
}

/*写校表系数
 */
void write_coef_val(int pha,int type,INT8U *val)
{
	INT8U	i;
	switch(pha) {
	case 0:				//A相
		switch(type) {
		case W_APQ1:			//相角系数 //相位校正1  Iregion1<I<Iregion0
			for(i=0;i<3;i++)	attCoef.PhaseA[i] = val[i];
			break;
		case 2:			//有功系数
			for(i=0;i<3;i++)	attCoef.PA[i] = val[i];
			break;
		case 3:			//电压系数
			for(i=0;i<3;i++)	attCoef.UA[i] = val[i];
			break;
		case 4:			//电流系数
			for(i=0;i<3;i++)	attCoef.IA[i] = val[i];
			break;
		case 5:			//电压offset系数
			for(i=0;i<3;i++)	attCoef.UoffsetA[i] = val[i];
			break;
		case 6:			//电流offset系数
			for(i=0;i<3;i++)	attCoef.IoffsetA[i] = val[i];
			break;
		case W_APQ2:			//小电流相位校正系数  //相位校正2  I<Iregion1
			for(i=0;i<3;i++)	attCoef.PhaseA1[i] = val[i];
			break;
		case W_APQ0:			//大电流相位校正系数，att7022e-d支持//相位校正0 I>Iregion0
			for(i=0;i<3;i++)	attCoef.PhaseA0[i] = val[i];
			break;
		}
		break;
	case 1:				//B相
		switch(type) {
		case W_APQ1:			//相角系数
			for(i=0;i<3;i++)	attCoef.PhaseB[i] = val[i];
			break;
		case 2:			//有功系数
			for(i=0;i<3;i++)	attCoef.PB[i] = val[i];
			break;
		case 3:			//电压系数
			for(i=0;i<3;i++)	attCoef.UB[i] = val[i];
			break;
		case 4:			//电流系数
			for(i=0;i<3;i++)	attCoef.IB[i] = val[i];
			break;
		case 5:			//电压offset系数
			for(i=0;i<3;i++)	attCoef.UoffsetB[i] = val[i];
			break;
		case 6:			//电流offset系数
			for(i=0;i<3;i++)	attCoef.IoffsetB[i] = val[i];
			break;
		case W_APQ2:			//小电流相位校正系数
			for(i=0;i<3;i++)	attCoef.PhaseB1[i] = val[i];
			break;
		case W_APQ0:			//大电流相位校正系数，att7022e-d支持
			for(i=0;i<3;i++)	attCoef.PhaseB0[i] = val[i];
			break;
		}
		break;
	case 2:				//C相
		switch(type) {
		case W_APQ1:			//相角系数
			for(i=0;i<3;i++)	attCoef.PhaseC[i] = val[i];
			break;
		case 2:			//有功系数
			for(i=0;i<3;i++)	attCoef.PC[i] = val[i];
			break;
		case 3:			//电压系数
			for(i=0;i<3;i++)	attCoef.UC[i] = val[i];
			break;
		case 4:			//电流系数
			for(i=0;i<3;i++)	attCoef.IC[i] = val[i];
			break;
		case 5:			//电压offset系数
			for(i=0;i<3;i++)	attCoef.UoffsetC[i] = val[i];
			break;
		case 6:			//电流offset系数
			for(i=0;i<3;i++)	attCoef.IoffsetC[i] = val[i];
			break;
		case W_APQ2:			//小电流相位校正系数
			for(i=0;i<3;i++)	attCoef.PhaseC1[i] = val[i];
			break;
		case W_APQ0:			//大电流相位校正系数，att7022e-d支持
			for(i=0;i<3;i++)	attCoef.PhaseC0[i] = val[i];
			break;
		}
		break;
	}
}

/*读4次有功、无功、电压、电流寄存器值
 * 求平均值
 */
void read_reg_val(int pha,	INT32S  *regP,INT32S  *regQ,INT32S  *regU,INT32S  *regI)
{
	static	int	ReadCount=4;
	INT32S		tmpsum[4];
	INT32S		i;

	memset(tmpsum,0,sizeof(tmpsum));
	switch(pha) {
	case 0:				//A相
		for(i=0;i<ReadCount;i++)
		{
			tmpsum[0]=tmpsum[0]+att_spi_read(spifp, r_Pa, 3);
			tmpsum[1]=tmpsum[1]+att_spi_read(spifp, r_Qa, 3);
			tmpsum[2]=tmpsum[2]+att_spi_read(spifp, r_UaRms, 3);
			tmpsum[3]=tmpsum[3]+att_spi_read(spifp, r_IaRms, 3);
			usleep(500000);
		}
		break;
	case 1:			//B相
		for(i=0;i<ReadCount;i++)
		{
			tmpsum[0]=tmpsum[0]+att_spi_read(spifp, r_Pb, 3);
			tmpsum[1]=tmpsum[1]+att_spi_read(spifp, r_Qb, 3);
			tmpsum[2]=tmpsum[2]+att_spi_read(spifp, r_UbRms, 3);
			tmpsum[3]=tmpsum[3]+att_spi_read(spifp, r_IbRms, 3);
			usleep(500000);
		}
		break;
	case 2:			//C相
		for(i=0;i<ReadCount;i++)
		{
			tmpsum[0]=tmpsum[0]+att_spi_read(spifp, r_Pc, 3);
			tmpsum[1]=tmpsum[1]+att_spi_read(spifp, r_Qc, 3);
			tmpsum[2]=tmpsum[2]+att_spi_read(spifp, r_UcRms, 3);
			tmpsum[3]=tmpsum[3]+att_spi_read(spifp, r_IcRms, 3);
			usleep(500000);
		}
		break;
	}
	*regP = tmpsum[0]/ReadCount;
	*regQ = tmpsum[1]/ReadCount;
	*regU = tmpsum[2]/ReadCount;
	*regI = tmpsum[3]/ReadCount;
}

/*较表命令
 * P、Q、U、I校正
 * */
void phase_check(int pha)
{
	INT32S		regval[4];			//0:P,1:Q,2:U,3:I
	float 		tempPhase=0,tempPgain=0,tempUgain=0,tempIgain=0;
	short 		JiaoCha,Pgain,Ugain,Igain;
	INT8U 		temp[3],mod;
	float 		temp1,temp2;

	read_reg_val(pha,&regval[0],&regval[1],&regval[2],&regval[3]);
	if(pha==0) fprintf(stderr,"\nA相 P、Q、U、I、Phase校正\n");
	if(pha==1) fprintf(stderr,"B相 P、Q、U、I、Phase校正\n");
	if(pha==2) fprintf(stderr,"C相 P、Q、U、I、Phase校正\n");
	fprintf(stderr,"\n\r P ==%x [%d]",regval[0],regval[0]);
	fprintf(stderr,"\n\r Q ==%x [%d]",regval[1],regval[1]);
	fprintf(stderr,"\n\r U ==%x [%d]",regval[2],regval[2]);
	fprintf(stderr,"\n\r I ==%x [%d]",regval[3],regval[3]);

	//P
    //计算测量功率值
	if (regval[0] > 8388608)//2^23
		regval[0] = regval[0] -16777216;//2^24
	regval[0] = regval[0]*1.0/PQCoef*100;

	//Q
	if (regval[1] > 8388608)//2^23
		regval[1] = regval[1] -16777216;//2^24
	regval[1] = (regval[1]*1.0/PQCoef)*1.0005*100;//1.0005为无功算法存在的增益
//	fprintf(stderr,"\n\r QA/756==%x [%d]",regval[1] ,regval[1] );

	//计算角差校正值(Preal*Q-P*Qreal)/(P*Preal+Q*Qreal);
	temp1=Preal[pha]*regval[1] - regval[0]*Qreal[pha];
	temp2=regval[0]*Preal[pha] + regval[1]*Qreal[pha];
	if(temp2!=0)
		tempPhase=temp1*1.0/temp2;
//	fprintf(stderr,"\n\r tempPhase[%d]==%f",pha,tempPhase);

	if (tempPhase>=0)
	{
		mod=(int)(tempPhase*32768*10)%10;//2^15=32768
//		fprintf(stderr,"\n\r mod_phase==%d",mod);
		if(mod>=5)
			JiaoCha = floor(tempPhase * 32768)+1;
		else
			JiaoCha = floor(tempPhase * 32768);
	}else
	{
		mod=(int)((65536 + tempPhase * 32768)*10)%10;
		if(mod>=5)
			JiaoCha = floor(65536 + tempPhase * 32768)+1;
		else
			JiaoCha = floor(65536 + tempPhase * 32768);
	}

	//校正角差后的功率值
	regval[0]=regval[0]+regval[1]*tempPhase;
	//功率增益校正值
	if(regval[0]!=0)
		tempPgain=Preal[pha]*100*1.0/regval[0]-1;
//	fprintf(stderr,"\n\r tempPgain[%d]==%f",pha,tempPgain);
	if (tempPgain>=0)
	{
		mod=(int)(tempPgain*32768*10)%10;
		if(mod>=5)
			Pgain = floor(tempPgain * 32768)+1;
		else
			Pgain = floor(tempPgain * 32768);
	}else
	{
		mod=(int)((65536 + tempPgain * 32768)*10)%10;
//		fprintf(stderr,"\n\r mod_P==%d",mod);
		if(mod>=5)
			Pgain = floor(65536 + tempPgain * 32768)+1;
		else
			Pgain = floor(65536 + tempPgain * 32768);
	}

	//电压、电流校正
	//计算测量电压电流有效值
	regval[2]=regval[2]*1.0/8192*U_COEF*10; 	//保住电压小数点后2位，*10保证精度。
	regval[3]=regval[3]*1.0/8192/N_CONST*I_COEF*10;	//保住电流小数点后3位，*10保证精度。
	fprintf(stderr,"regval_I = %d \n",regval[3]);
	//计算电压、电流有效值校正系数
	if(regval[2]!=0)
		tempUgain=Ureal[pha]*U_COEF*10*1.0/regval[2]-1;
	if (tempUgain>=0)
	{
		mod=(int)(tempUgain*32768*10)%10;
//		fprintf(stderr,"\n\r mod_U==%d",mod);
		if(mod>=5)
			Ugain = floor(tempUgain * 32768)+1;
		else
			Ugain = floor(tempUgain * 32768);
	}else
	{
		mod=(int)(65536 + tempUgain*32768*10)%10;
		if(mod>=5)
			Ugain = floor(65536 + tempUgain * 32768)+1;
		else
			Ugain = floor(65536 + tempUgain * 32768);
	}

	if(regval[3]!=0)
		tempIgain=Ireal[pha]*I_COEF*10*1.0/regval[3]-1;
	fprintf(stderr,"Ireal[pha]=%f,tempIgain=%f\n",Ireal[pha],tempIgain);

	if (tempIgain>=0)
	{
		mod=(int)(tempIgain*32768*10)%10;
		if(mod>=5)
			Igain = floor(tempIgain * 32768)+1;
		else
			Igain = floor(tempIgain * 32768);
	}else
	{
		mod=(int)((65536 + tempIgain * 32768)*10)%10;
		if(mod>=5)
			Igain = floor(65536 + tempIgain * 32768)+1;
		else
			Igain = floor(65536 + tempIgain * 32768);
	}
	fprintf(stderr,"Igain=%x\n",Igain);


	temp[0] = 0;
	temp[1] = (JiaoCha & 0xff00) >> 8;
	temp[2] = JiaoCha & 0xff;
	if(VersionID==1) 	write_coef_val(pha,W_APQ1,&temp[0]);		//写入P
	else if(VersionID==ATT_VER_ID) write_coef_val(pha,W_APQ0,&temp[0]);

	temp[0] = 0;
	temp[1] = (Pgain & 0xff00) >> 8;
	temp[2] = Pgain & 0xff;
	write_coef_val(pha,2,&temp[0]);

	temp[0] = 0;
	temp[1] = (Ugain & 0xff00) >> 8;
	temp[2] = Ugain & 0xff;
	write_coef_val(pha,3,&temp[0]);

	temp[0] = 0;
	temp[1] = (Igain & 0xff00) >> 8;
	temp[2] = Igain & 0xff;
	write_coef_val(pha,4,&temp[0]);

	if(pha==0){
		fprintf(stderr,"\n\r A相 校表系数\n");
		fprintf(stderr,"PhaseA0=%02x_%02x_%02x\n",attCoef.PhaseA0[0],attCoef.PhaseA0[1],attCoef.PhaseA0[2]);
		fprintf(stderr,"PhaseA=%02x_%02x_%02x\n",attCoef.PhaseA[0],attCoef.PhaseA[1],attCoef.PhaseA[2]);
		fprintf(stderr,"PhaseA1=%02x_%02x_%02x\n",attCoef.PhaseA1[0],attCoef.PhaseA1[1],attCoef.PhaseA1[2]);
		fprintf(stderr,"PA=%02x_%02x_%02x\n",attCoef.PA[0],attCoef.PA[1],attCoef.PA[2]);
		fprintf(stderr,"UA=%02x_%02x_%02x\n",attCoef.UA[0],attCoef.UA[1],attCoef.UA[2]);
		fprintf(stderr,"IA=%02x_%02x_%02x\n",attCoef.IA[0],attCoef.IA[1],attCoef.IA[2]);
		fprintf(stderr,"\n\r A相 校正结束\n");
	}
	if(pha==1) {
		fprintf(stderr,"\n\r B相 校表系数\n");
		fprintf(stderr,"PhaseB0=%02x_%02x_%02x\n",attCoef.PhaseB0[0],attCoef.PhaseB0[1],attCoef.PhaseB0[2]);
		fprintf(stderr,"PhaseB=%02x_%02x_%02x\n",attCoef.PhaseB[0],attCoef.PhaseB[1],attCoef.PhaseB[2]);
		fprintf(stderr,"PhaseB1=%02x_%02x_%02x\n",attCoef.PhaseB1[0],attCoef.PhaseB1[1],attCoef.PhaseB1[2]);
		fprintf(stderr,"PB=%02x_%02x_%02x\n",attCoef.PB[0],attCoef.PB[1],attCoef.PB[2]);
		fprintf(stderr,"UB=%02x_%02x_%02x\n",attCoef.UB[0],attCoef.UB[1],attCoef.UB[2]);
		fprintf(stderr,"IB=%02x_%02x_%02x\n",attCoef.IB[0],attCoef.IB[1],attCoef.IB[2]);
		fprintf(stderr,"\n\r B相 校正结束\n");
	}
	if(pha==2) {
		fprintf(stderr,"\n\r C相 校表系数\n");
		fprintf(stderr,"PhaseC0=%02x_%02x_%02x\n",attCoef.PhaseC0[0],attCoef.PhaseC0[1],attCoef.PhaseC0[2]);
		fprintf(stderr,"PhaseC=%02x_%02x_%02x\n",attCoef.PhaseC[0],attCoef.PhaseC[1],attCoef.PhaseC[2]);
		fprintf(stderr,"PhaseC1=%02x_%02x_%02x\n",attCoef.PhaseC1[0],attCoef.PhaseC1[1],attCoef.PhaseC1[2]);
		fprintf(stderr,"PC=%02x_%02x_%02x\n",attCoef.PC[0],attCoef.PC[1],attCoef.PC[2]);
		fprintf(stderr,"UC=%02x_%02x_%02x\n",attCoef.UC[0],attCoef.UC[1],attCoef.UC[2]);
		fprintf(stderr,"IC=%02x_%02x_%02x\n",attCoef.IC[0],attCoef.IC[1],attCoef.IC[2]);
		fprintf(stderr,"\n\r C相 校正结束\n");
	}
	return;
}

/*较表命令
 * P、Q、U、I校正
 * */
void phase_iregion_check(int pha,INT8U type)
{

	//PQCoef=(HFConst*EC*2^23)/(2.592*10^10)=(365*6400*2^23)/(2.592*10^10)
	INT32S		regval[4];			//0:P,1:Q,2:U,3:I
	float 		tempPhase=0;
	short 		JiaoCha;
	INT8U 		temp[3],mod;
	float 		temp1,temp2;

	read_reg_val(pha,&regval[0],&regval[1],&regval[2],&regval[3]);
	if(pha==0) fprintf(stderr,"A相 P、Q、U、I、Phase校正\n");
	if(pha==1) fprintf(stderr,"B相 P、Q、U、I、Phase校正\n");
	if(pha==2) fprintf(stderr,"C相 P、Q、U、I、Phase校正\n");
	fprintf(stderr,"\n\r P ==%x [%d]",regval[0],regval[0]);
	fprintf(stderr,"\n\r Q ==%x [%d]",regval[1],regval[1]);
	fprintf(stderr,"\n\r U ==%x [%d]",regval[2],regval[2]);
	fprintf(stderr,"\n\r I ==%x [%d]",regval[3],regval[3]);

	//P
    //计算测量功率值
	if (regval[0] > 8388608)//2^23
		regval[0] = regval[0] -16777216;//2^24
	regval[0] = regval[0]*1.0/PQCoef*100;

	//Q
	if (regval[1] > 8388608)//2^23
		regval[1] = regval[1] -16777216;//2^24
	regval[1] = (regval[1]*1.0/PQCoef)*1.0005*100;//1.0005为无功算法存在的增益
//	fprintf(stderr,"\n\r QA/756==%x [%d]",regval[1] ,regval[1] );

	//计算角差校正值(Preal*Q-P*Qreal)/(P*Preal+Q*Qreal);
	temp1=Preal[pha]*regval[1] - regval[0]*Qreal[pha];
	temp2=regval[0]*Preal[pha] + regval[1]*Qreal[pha];
	if(temp2!=0)
		tempPhase=temp1*1.0/temp2;
//	fprintf(stderr,"\n\r tempPhase[%d]==%f",pha,tempPhase);

	if (tempPhase>=0)
	{
		mod=(int)(tempPhase*32768*10)%10;//2^15=32768
//		fprintf(stderr,"\n\r mod_phase==%d",mod);
		if(mod>=5)
			JiaoCha = floor(tempPhase * 32768)+1;
		else
			JiaoCha = floor(tempPhase * 32768);
	}else
	{
		mod=(int)((65536 + tempPhase * 32768)*10)%10;
		if(mod>=5)
			JiaoCha = floor(65536 + tempPhase * 32768)+1;
		else
			JiaoCha = floor(65536 + tempPhase * 32768);
	}

	temp[0] = 0;
	temp[1] = (JiaoCha & 0xff00) >> 8;
	temp[2] = JiaoCha & 0xff;
//	if(VersionID==0) 	write_coef_val(pha,type,&temp[0]);		//写入P
//	else if(VersionID==ATT_VER_ID) write_coef_val(pha,type,&temp[0]);
	write_coef_val(pha,type,&temp[0]);

	if(pha==0){
		fprintf(stderr,"\n\r A相 校表系数\n");
		fprintf(stderr,"PhaseA0=%02x_%02x_%02x\n",attCoef.PhaseA0[0],attCoef.PhaseA0[1],attCoef.PhaseA0[2]);
		fprintf(stderr,"PhaseA=%02x_%02x_%02x\n",attCoef.PhaseA[0],attCoef.PhaseA[1],attCoef.PhaseA[2]);
		fprintf(stderr,"PhaseA1=%02x_%02x_%02x\n",attCoef.PhaseA1[0],attCoef.PhaseA1[1],attCoef.PhaseA1[2]);
		fprintf(stderr,"PA=%02x_%02x_%02x\n",attCoef.PA[0],attCoef.PA[1],attCoef.PA[2]);
		fprintf(stderr,"UA=%02x_%02x_%02x\n",attCoef.UA[0],attCoef.UA[1],attCoef.UA[2]);
		fprintf(stderr,"IA=%02x_%02x_%02x\n",attCoef.IA[0],attCoef.IA[1],attCoef.IA[2]);
		fprintf(stderr,"\n\r A相 校正结束\n");
	}
	if(pha==1) {
		fprintf(stderr,"\n\r B相 校表系数\n");
		fprintf(stderr,"PhaseB0=%02x_%02x_%02x\n",attCoef.PhaseB0[0],attCoef.PhaseB0[1],attCoef.PhaseB0[2]);
		fprintf(stderr,"PhaseB=%02x_%02x_%02x\n",attCoef.PhaseB[0],attCoef.PhaseB[1],attCoef.PhaseB[2]);
		fprintf(stderr,"PhaseB1=%02x_%02x_%02x\n",attCoef.PhaseB1[0],attCoef.PhaseB1[1],attCoef.PhaseB1[2]);
		fprintf(stderr,"PB=%02x_%02x_%02x\n",attCoef.PB[0],attCoef.PB[1],attCoef.PB[2]);
		fprintf(stderr,"UB=%02x_%02x_%02x\n",attCoef.UB[0],attCoef.UB[1],attCoef.UB[2]);
		fprintf(stderr,"IB=%02x_%02x_%02x\n",attCoef.IB[0],attCoef.IB[1],attCoef.IB[2]);
		fprintf(stderr,"\n\r B相 校正结束\n");
	}
	if(pha==2) {
		fprintf(stderr,"\n\r C相 校表系数\n");
		fprintf(stderr,"PhaseC0=%02x_%02x_%02x\n",attCoef.PhaseC0[0],attCoef.PhaseC0[1],attCoef.PhaseC0[2]);
		fprintf(stderr,"PhaseC=%02x_%02x_%02x\n",attCoef.PhaseC[0],attCoef.PhaseC[1],attCoef.PhaseC[2]);
		fprintf(stderr,"PhaseC1=%02x_%02x_%02x\n",attCoef.PhaseC1[0],attCoef.PhaseC1[1],attCoef.PhaseC1[2]);
		fprintf(stderr,"PC=%02x_%02x_%02x\n",attCoef.PC[0],attCoef.PC[1],attCoef.PC[2]);
		fprintf(stderr,"UC=%02x_%02x_%02x\n",attCoef.UC[0],attCoef.UC[1],attCoef.UC[2]);
		fprintf(stderr,"IC=%02x_%02x_%02x\n",attCoef.IC[0],attCoef.IC[1],attCoef.IC[2]);
		fprintf(stderr,"\n\r C相 校正结束\n");
	}
	return;
}

/*
 * 写ATT7022E校表角差寄存器*/
void write_Jiaocoef_reg(INT32S fp,INT8U type)
{
	INT8U temp[3];

	temp[0] = 0;
	temp[1] = 0;
	temp[2] = 0x5a;
	att_spi_write(fp, Reg_Enable, 3, temp); //允许写操作

	if(VersionID == ATT_VER_ID) {
		switch(type) {
		case W_APQ0:
			//3A的相位校正
			att_spi_write(fp, w_PhSregApq0, 3, attCoef.PhaseA0);
			att_spi_write(fp, w_PhSregBpq0, 3, attCoef.PhaseB0);
			att_spi_write(fp, w_PhSregCpq0, 3, attCoef.PhaseC0);
			break;
		case W_APQ1:
			//1.5A的相位校正
			att_spi_write(fp, w_PhSregApq1, 3, attCoef.PhaseA);
			att_spi_write(fp, w_PhSregBpq1, 3, attCoef.PhaseB);
			att_spi_write(fp, w_PhSregCpq1, 3, attCoef.PhaseC);
			break;
		case W_APQ2:
			//0.5A的相位校正
			att_spi_write(fp, w_PhSregApq2, 3, attCoef.PhaseA1);
			att_spi_write(fp, w_PhSregBpq2, 3, attCoef.PhaseB1);
			att_spi_write(fp, w_PhSregCpq2, 3, attCoef.PhaseC1);
			break;
		}
	}else {
		fprintf(stderr,"VersionID=%x\n",VersionID);

		switch(type) {
		case W_APQ1:
			//1.5A的相位校正
			att_spi_write(fp, w_PhSregApq0, 3, attCoef.PhaseA);
			att_spi_write(fp, w_PhSregBpq0, 3, attCoef.PhaseB);
			att_spi_write(fp, w_PhSregCpq0, 3, attCoef.PhaseC);
			break;
		case W_APQ2:
			//0.5A的相位校正
			fprintf(stderr,"PhaseA1=%02x%02x%02x\n", attCoef.PhaseA1[0], attCoef.PhaseA1[1], attCoef.PhaseA1[2]);
			fprintf(stderr,"PhaseB1=%02x%02x%02x\n", attCoef.PhaseB1[0], attCoef.PhaseB1[1], attCoef.PhaseB1[2]);
			fprintf(stderr,"PhaseC1=%02x%02x%02x\n", attCoef.PhaseC1[0], attCoef.PhaseC1[1], attCoef.PhaseC1[2]);
			att_spi_write(fp, w_PhSregApq1, 3, attCoef.PhaseA1);
			att_spi_write(fp, w_PhSregBpq1, 3, attCoef.PhaseB1);
			att_spi_write(fp, w_PhSregCpq1, 3, attCoef.PhaseC1);
			break;
		}
	}

	temp[0] = 0;
	temp[1] = 0;
	temp[2] = 1;
	att_spi_write(fp, Reg_Enable, 3, temp); //允许写操作
}

/*
 * 写ATT7022E校表寄存器*/
void write_coef_reg(INT32S fp)
{
	INT8U temp[3];

	temp[0] = 0;
	temp[1] = 0;
	temp[2] = 0x5a;
	att_spi_write(fp, Reg_Enable, 3, temp); //允许写操作

	WriteRegInit(fp);

	att_spi_write(fp, w_UgainA, 3, attCoef.UA);
	att_spi_write(fp, w_UgainB, 3, attCoef.UB);
	att_spi_write(fp, w_UgainC, 3, attCoef.UC);

	att_spi_write(fp, w_IgainA, 3, attCoef.IA);
	att_spi_write(fp, w_IgainB, 3, attCoef.IB);
	att_spi_write(fp, w_IgainC, 3, attCoef.IC);

	att_spi_write(fp, w_PgainA, 3, attCoef.PA);
	att_spi_write(fp, w_QgainA, 3, attCoef.PA);
	att_spi_write(fp, w_SgainA, 3, attCoef.PA);

	att_spi_write(fp, w_PgainB, 3, attCoef.PB);
	att_spi_write(fp, w_QgainB, 3, attCoef.PB);
	att_spi_write(fp, w_SgainB, 3, attCoef.PB);

	att_spi_write(fp, w_PgainC, 3, attCoef.PC);
	att_spi_write(fp, w_QgainC, 3, attCoef.PC);
	att_spi_write(fp, w_SgainC, 3, attCoef.PC);

	att_spi_write(fp, w_IaRmsoffse, 3, attCoef.IoffsetA);
	att_spi_write(fp, w_IbRmsoffse, 3, attCoef.IoffsetB);
	att_spi_write(fp, w_IcRmsoffse, 3, attCoef.IoffsetC);

	if(VersionID == ATT_VER_ID) {
		fprintf(stderr,"acreg new VersionID=%x\n",VersionID);
		//相位校正0 I>Iregion0
		att_spi_write(fp, w_PhSregApq0, 3, attCoef.PhaseA0);
		att_spi_write(fp, w_PhSregBpq0, 3, attCoef.PhaseB0);
		att_spi_write(fp, w_PhSregCpq0, 3, attCoef.PhaseC0);
		//相位校正1  Iregion1<I<Iregion0
		att_spi_write(fp, w_PhSregApq1, 3, attCoef.PhaseA);
		att_spi_write(fp, w_PhSregBpq1, 3, attCoef.PhaseB);
		att_spi_write(fp, w_PhSregCpq1, 3, attCoef.PhaseC);
		//相位校正2  I<Iregion1
		att_spi_write(fp, w_PhSregApq2, 3, attCoef.PhaseA1);
		att_spi_write(fp, w_PhSregBpq2, 3, attCoef.PhaseB1);
		att_spi_write(fp, w_PhSregCpq2, 3, attCoef.PhaseC1);
	}else {
		fprintf(stderr,"acreg VersionID=%x\n",VersionID);
		fprintf(stderr,"PhaseA=%02x_%02x_%02x\n",attCoef.PhaseA[0],attCoef.PhaseA[1],attCoef.PhaseA[2]);
		fprintf(stderr,"PhaseB=%02x_%02x_%02x\n",attCoef.PhaseB[0],attCoef.PhaseB[1],attCoef.PhaseB[2]);
		fprintf(stderr,"PhaseC=%02x_%02x_%02x\n",attCoef.PhaseC[0],attCoef.PhaseC[1],attCoef.PhaseC[2]);

		//相位校正
		att_spi_write(fp, w_PhSregApq0, 3, attCoef.PhaseA);
		att_spi_write(fp, w_PhSregBpq0, 3, attCoef.PhaseB);
		att_spi_write(fp, w_PhSregCpq0, 3, attCoef.PhaseC);

		fprintf(stderr,"PhaseA1=%02x_%02x_%02x\n",attCoef.PhaseA1[0],attCoef.PhaseA1[1],attCoef.PhaseA1[2]);
		fprintf(stderr,"PhaseB1=%02x_%02x_%02x\n",attCoef.PhaseB1[0],attCoef.PhaseB1[1],attCoef.PhaseB1[2]);
		fprintf(stderr,"PhaseC1=%02x_%02x_%02x\n",attCoef.PhaseC1[0],attCoef.PhaseC1[1],attCoef.PhaseC1[2]);

		//相位校正
		att_spi_write(fp, w_PhSregApq1, 3, attCoef.PhaseA1);
		att_spi_write(fp, w_PhSregBpq1, 3, attCoef.PhaseB1);
		att_spi_write(fp, w_PhSregCpq1, 3, attCoef.PhaseC1);
	}
	temp[0] = 0;
	temp[1] = 0;
	temp[2] = 1;
	att_spi_write(fp, Reg_Enable, 3, temp); //不允许写操作

//		fprintf(stderr,"\n\r 电压有效值offset 校正系数\n");
//		fprintf(stderr,"UoffsetA=%02x_%02x_%02x\n",attCoef.UoffsetA[0],attCoef.UoffsetA[1],attCoef.UoffsetA[2]);
//		fprintf(stderr,"UoffsetB=%02x_%02x_%02x\n",attCoef.UoffsetB[0],attCoef.UoffsetB[1],attCoef.UoffsetB[2]);
//		fprintf(stderr,"UoffsetC=%02x_%02x_%02x\n",attCoef.UoffsetC[0],attCoef.UoffsetC[1],attCoef.UoffsetC[2]);
	fprintf(stderr,"\n\r 电流有效值offset 校正系数\n");
	fprintf(stderr,"IoffsetA=%02x_%02x_%02x\n",attCoef.IoffsetA[0],attCoef.IoffsetA[1],attCoef.IoffsetA[2]);
	fprintf(stderr,"IoffsetB=%02x_%02x_%02x\n",attCoef.IoffsetB[0],attCoef.IoffsetB[1],attCoef.IoffsetB[2]);
	fprintf(stderr,"IoffsetC=%02x_%02x_%02x\n",attCoef.IoffsetC[0],attCoef.IoffsetC[1],attCoef.IoffsetC[2]);
	fprintf(stderr,"\n\r 相角 校表系数\n");
	fprintf(stderr,"PhaseA=%02x_%02x_%02x\n",attCoef.PhaseA[0],attCoef.PhaseA[1],attCoef.PhaseA[2]);
	fprintf(stderr,"PhaseB=%02x_%02x_%02x\n",attCoef.PhaseB[0],attCoef.PhaseB[1],attCoef.PhaseB[2]);
	fprintf(stderr,"PhaseC=%02x_%02x_%02x\n",attCoef.PhaseC[0],attCoef.PhaseC[1],attCoef.PhaseC[2]);
	if(VersionID == ATT_VER_ID) {
		fprintf(stderr,"\n\r 大电流相角 校表系数\n");
		fprintf(stderr,"PhaseA0=%02x_%02x_%02x\n",attCoef.PhaseA0[0],attCoef.PhaseA0[1],attCoef.PhaseA0[2]);
		fprintf(stderr,"PhaseB0=%02x_%02x_%02x\n",attCoef.PhaseB0[0],attCoef.PhaseB0[1],attCoef.PhaseB0[2]);
		fprintf(stderr,"PhaseC0=%02x_%02x_%02x\n",attCoef.PhaseC0[0],attCoef.PhaseC0[1],attCoef.PhaseC0[2]);
	}
	fprintf(stderr,"\n\r 小电流相角 校表系数\n");
	fprintf(stderr,"PhaseA1=%02x_%02x_%02x\n",attCoef.PhaseA1[0],attCoef.PhaseA1[1],attCoef.PhaseA1[2]);
	fprintf(stderr,"PhaseB1=%02x_%02x_%02x\n",attCoef.PhaseB1[0],attCoef.PhaseB1[1],attCoef.PhaseB1[2]);
	fprintf(stderr,"PhaseC1=%02x_%02x_%02x\n",attCoef.PhaseC1[0],attCoef.PhaseC1[1],attCoef.PhaseC1[2]);

	fprintf(stderr,"\n\r 电压 校表系数\n");
	fprintf(stderr,"UA=%02x_%02x_%02x\n",attCoef.UA[0],attCoef.UA[1],attCoef.UA[2]);
	fprintf(stderr,"UB=%02x_%02x_%02x\n",attCoef.UB[0],attCoef.UB[1],attCoef.UB[2]);
	fprintf(stderr,"UC=%02x_%02x_%02x\n",attCoef.UC[0],attCoef.UC[1],attCoef.UC[2]);
	fprintf(stderr,"\n\r 电流 校表系数\n");
	fprintf(stderr,"IA=%02x_%02x_%02x\n",attCoef.IA[0],attCoef.IA[1],attCoef.IA[2]);
	fprintf(stderr,"IB=%02x_%02x_%02x\n",attCoef.IB[0],attCoef.IB[1],attCoef.IB[2]);
	fprintf(stderr,"IC=%02x_%02x_%02x\n",attCoef.IC[0],attCoef.IC[1],attCoef.IC[2]);
	fprintf(stderr,"\n\r 功率 校表系数\n");
	fprintf(stderr,"PA=%02x_%02x_%02x\n",attCoef.PA[0],attCoef.PA[1],attCoef.PA[2]);
	fprintf(stderr,"PB=%02x_%02x_%02x\n",attCoef.PB[0],attCoef.PB[1],attCoef.PB[2]);
	fprintf(stderr,"PC=%02x_%02x_%02x\n",attCoef.PC[0],attCoef.PC[1],attCoef.PC[2]);
}

/*
 * 交采系数校正过程
 * */
void acs_regclean()
{
	int		val;
	char  	fname[64];
	sem_t * sem_fd=NULL;

	sem_fd = open_named_sem(SEMNAME_SPI0_0);
	sem_wait(sem_fd);
	clean_coef();
	sleep(1);
	memset(&attCoef,0,sizeof(ACCoe_SAVE));
	write_coef_reg(spifp);

	sem_post(sem_fd);
	sem_getvalue(sem_fd, &val);
	fprintf(stderr,"process <vd> The sem is %d\n", val);
	fprintf(stderr,"process <vd> close a semaphore\n");
	sem_close(sem_fd);
}

/*
 * ATT7022E-d芯片温度Toffset校正
 * */
void get_temp_val(INT32S fp)
{
	INT32S	TempREC;
	INT8U 	temp[3];

	TempREC = att_spi_read(spifp, r_TPSD, 3);
	fprintf(stderr,"温度寄存器TempREC=%x",TempREC);

	attCoef.Tpsoffset[0] = (TempREC>>16)&0xff;
	attCoef.Tpsoffset[1] = (TempREC>>8)&0xff;
	attCoef.Tpsoffset[2] = TempREC&0xff;

	temp[0] = 0;
	temp[1] = 0;
	temp[2] = 0x5a;
	att_spi_write(fp, Reg_Enable, 3, temp); //允许写操作

	att_spi_write(fp, w_TPSoffset, 3, attCoef.Tpsoffset); //写Toffset寄存器

	temp[0] = 0;
	temp[1] = 0;
	temp[2] = 1;
	att_spi_write(fp, Reg_Enable, 3, temp); //不允许写操作
}
/*
 * 交采系数校正过程
 * */
void acs_check_coef()
{
	int		val;
	sem_t * sem_check_fd=NULL;

	sem_check_fd = open_named_sem(SEMNAME_SPI0_0);
	sem_wait(sem_check_fd);
	memset(&attCoef,0,sizeof(attCoef));
	clean_coef();
	sleep(3);
	phase_check(0);
	sleep(1);
	if(WireType==0x0600) {	//三相四线校正B相
		phase_check(1);
		sleep(1);
	}
	phase_check(2);
	sleep(1);
	//温度校正
	if(VersionID == ATT_VER_ID) {
		fprintf(stderr,"温度校正\n");
		get_temp_val(spifp);
	}
	write_coef_reg(spifp);
	fprintf(stderr,"----------ACCoe_SAVE=%d\n",sizeof(ACCoe_SAVE));
	saveCoverClass(0,0,&attCoef,sizeof(ACCoe_SAVE),acs_coef_save);
	///////////14.1.10
//	fprintf(stderr,"等待谐波矫正\n");
//	sleep(8);
//	GetHarmonicCoef(spifp,Ureal[0],Ireal[0]);
	//////////////
	sem_post(sem_check_fd);
	sem_getvalue(sem_check_fd, &val);
	fprintf(stderr,"process <vd> The sem is %d\n", val);
	fprintf(stderr,"process <vd> close a semaphore\n");
	sem_close(sem_check_fd);
}

/*
 * 交采分段相角系数校正过程
 * */
void acs_check_phase_coef(INT8U type)
{
	int		val;
	sem_t * sem_fd=NULL;

	sem_fd = open_named_sem(SEMNAME_SPI0_0);
	sem_wait(sem_fd);
	readCoverClass(0,0,&attCoef,sizeof(ACCoe_SAVE),acs_coef_save);
	clean_coef();
	sleep(3);
	phase_iregion_check(0,type);
	sleep(1);
	if(WireType==0x0600) {	//三相四线校正B相
		phase_iregion_check(1,type);
		sleep(1);
	}
	phase_iregion_check(2,type);
	sleep(1);
	write_Jiaocoef_reg(spifp,type);
	saveCoverClass(0,0,&attCoef,sizeof(ACCoe_SAVE),acs_coef_save);
	sem_post(sem_fd);
	sem_getvalue(sem_fd, &val);
	fprintf(stderr,"process <vd> The sem is %d\n", val);
	fprintf(stderr,"process <vd> close a semaphore\n");
	sem_close(sem_fd);
}


//校验寄存器读出的数值是否正确
INT8S check_regvalue_rn8209(INT32S regvalue)
{
	INT32S 	RRec[128];							//RN8209计量参数寄存器数据

	RRec[SYS_RData>>4] = rn_spi_read(spifp, SYS_RData);
//	usleep(500000);
	fprintf(stderr, "校验寄存器值为： %d \n", RRec[SYS_RData>>4]);
	if(regvalue == RRec[SYS_RData>>4]){
		return 0;
	}else
		return -1;
}

/*
 * RN8209 交采电压系数校正过程
 * */
void phase_check_rn8209(FP64 u)
{
	INT32U vrms = 0;
	INT32S regvalue = 0;
	FP64 cur_value;
	INT8U i;
	INT8U temp[3];

	i = AV_COUNT;

	cur_value = u;	//当前电压值
	fprintf(stderr, "校表中...... \n");
	while(i--)
	{
		regvalue = rn_spi_read(spifp, U_RMS);//电压通道有效值
		usleep(400*1000);//3.4Hz 更新
		if((check_regvalue_rn8209(regvalue) == 0)){
			vrms += regvalue;
		}else{
			i++;
			continue;
		}
	}
	vrms /= AV_COUNT;
//	vrms *= REG_COUNT;	//电压寄存器的值扩大REG_COUNT倍
	K_vrms = floor((FP64)vrms / cur_value);
	fprintf(stderr, "校正系数：K_vrms = %d \n", K_vrms);
//	if(K_vrms > 740000 && K_vrms < 756000)
//	{
//		fprintf(stderr, "校表结束！！！ \n");
//	}else{
//		fprintf(stderr, "校表失败，请重新校表！！！ \n");
//		return;
//	}

	temp[0] = (K_vrms & 0xff0000) >> 16;
	temp[1] = (K_vrms & 0xff00) >> 8;
	temp[2] = K_vrms & 0xff;
	memset(&attCoef, 0, sizeof(ACCoe_SAVE));
	write_coef_val( 0, 3, &temp[0]);
	saveCoverClass(0,0,&attCoef,sizeof(ACCoe_SAVE),acs_coef_save);
}

void acs_phase(int argc, char *argv[])
{
	INT8U	type=0;
	spifp = spi_init(spifp, ACS_SPI_DEV);
	if(argc==3){
		fprintf(stderr,"\n\r输入标准源的有功、无功、电压、电流值，具体参数察看帮助(vd help)！\n");
	}else{
		sscanf(argv[3],"%f",&Preal[0]);
		sscanf(argv[4],"%f",&Preal[1]);
		sscanf(argv[5],"%f",&Preal[2]);
		sscanf(argv[6],"%f",&Qreal[0]);
		sscanf(argv[7],"%f",&Qreal[1]);
		sscanf(argv[8],"%f",&Qreal[2]);
		sscanf(argv[9],"%f",&Ureal[0]);
		sscanf(argv[10],"%f",&Ureal[1]);
		sscanf(argv[11],"%f",&Ureal[2]);
		sscanf(argv[12],"%f",&Ireal[0]);
		sscanf(argv[13],"%f",&Ireal[1]);
		sscanf(argv[14],"%f",&Ireal[2]);
		fprintf(stderr,"输入参考值：Pa=%4.2f,Pb=%4.2f,Pc=%4.2f, Qa=%4.2f,Qb=%4.2f,Qc=%4.2f\nUa=%3.2f,Ub=%3.2f,Uc=%3.2f,Ia=%2.2f,Ib=%2.2f,Ic=%2.2f\n",
				Preal[0],Preal[1],Preal[2],Qreal[0],Qreal[1],Qreal[2],
				Ureal[0],Ureal[1],Ureal[2],Ireal[0],Ireal[1],Ireal[2]);

		syslog(LOG_NOTICE,"开始相角校正，%s\n",argv[2]);
		if(strcmp("acphase",argv[2])==0) {
			if(VersionID==1) type = W_APQ2;
			else if(VersionID==ATT_VER_ID)	type = W_APQ1;
		}else if(strcmp("acphase0",argv[2])==0) {
			fprintf(stderr,"ATT7022E-d型芯片支持功能\n");
			if(VersionID==ATT_VER_ID)	type = W_APQ2;
			else{
				spi_close(spifp);
				return;
			}
		}
		acs_check_phase_coef(type);
	}
	spi_close(spifp);
}

void acs_regist(int argc, char *argv[])
{
	spifp = spi_init(spifp, ACS_SPI_DEV);
	if(argc==3){
		fprintf(stderr,"\n\r输入标准源的有功、无功、电压、电流值，具体参数察看帮助(vd help)！\n");
	}else{
		syslog(LOG_NOTICE,"开始计量校正\n");
		sscanf(argv[3],"%f",&Preal[0]);
		sscanf(argv[4],"%f",&Preal[1]);
		sscanf(argv[5],"%f",&Preal[2]);
		sscanf(argv[6],"%f",&Qreal[0]);
		sscanf(argv[7],"%f",&Qreal[1]);
		sscanf(argv[8],"%f",&Qreal[2]);
		sscanf(argv[9],"%f",&Ureal[0]);
		sscanf(argv[10],"%f",&Ureal[1]);
		sscanf(argv[11],"%f",&Ureal[2]);
		sscanf(argv[12],"%f",&Ireal[0]);
		sscanf(argv[13],"%f",&Ireal[1]);
		sscanf(argv[14],"%f",&Ireal[2]);
		fprintf(stderr,"输入参考值：Pa=%4.2f,Pb=%4.2f,Pc=%4.2f, Qa=%4.2f,Qb=%4.2f,Qc=%4.2f\nUa=%3.2f,Ub=%3.2f,Uc=%3.2f,Ia=%2.2f,Ib=%2.2f,Ic=%2.2f\n",
				Preal[0],Preal[1],Preal[2],Qreal[0],Qreal[1],Qreal[2],
				Ureal[0],Ureal[1],Ureal[2],Ireal[0],Ireal[1],Ireal[2]);
		acs_check_coef();
//		acs_regdata_print();
		//acs_check_coef1(); //衍生方案1校正
	}
	spi_close(spifp);
}

void acs_process(int argc, char *argv[])
{
	FP64 	Ureal_rn8209=0;

    JProgramInfo = OpenShMem("ProgramInfo", sizeof(ProgramInfo), NULL);

	VersionID = JProgramInfo->dev_info.ac_chip_type;
	WireType = JProgramInfo->dev_info.WireType;
	fprintf(stderr,"\n计量芯片版本：%06X, 接线方式=%X(0600:三相四，1200：三相三)\n",VersionID,WireType);
    if(argc>=3) {	//
		if(strcmp(argv[1],"acs")==0) {
			if(strcmp(argv[2],"acreg")==0) {
				acs_regist(argc,argv);
			}
			if((strcmp(argv[2],"acphase")==0) || (strcmp(argv[2],"acphase0")==0)) {
				acs_phase(argc,argv);
			}
			if(strcmp("acregclean",argv[2])==0){
				char inputcmd='N';
				fprintf(stderr,"小心操作！该命令是清除校表参数，确定清除输入‘Y’，取消输入‘N’！\n");
				scanf("%c",&inputcmd);
		//		fprintf(stderr,"inputcmd = %c\n",inputcmd);
				if(inputcmd=='Y') {
					fprintf(stderr,"校表系数清除\n");
					spifp = spi_init(spifp, ACS_SPI_DEV);
					acs_regclean();
					spi_close(spifp);
				}
			}
			if(strcmp("acregdata",argv[2])==0){
				spifp = spi_init(spifp, ACS_SPI_DEV);
				acs_regdata_print();
				spi_close(spifp);
			}
			if(strcmp("acdata",argv[2])==0){
				for(;;){
					realdataprint(JProgramInfo->ACSRealData);
					sleep(1);
				}
			}
			if(strcmp("ace",argv[2])==0){
				for(;;){
					acs_energysum_print(JProgramInfo->ACSEnergy);
					sleep(1);
				}
			}
			if(strcmp("acrndata",argv[2])==0){
				spifp = spi_init_r(spifp, ACS_SPI_DEV);
				rn8209_regdata_print();
				spi_close(spifp);
			}
			if(strcmp("checku",argv[2])==0)//u
			{
				spifp = spi_init_r(spifp,ACS_SPI_DEV);
				if(argc==3){
					fprintf(stderr, "\n\r输入标准源的电压值，带两位小数位，如：220.00。具体参数察看帮助(cj help)！\n");
				}else{
					sscanf(argv[3], "%lf", &Ureal_rn8209);
					fprintf(stderr, "输入参考值：U=%3.2f \n", Ureal_rn8209);
					phase_check_rn8209(Ureal_rn8209);
				}
				spi_close(spifp);
			}
		}
	}
    shmm_unregister("ProgramInfo", sizeof(ProgramInfo));
}

