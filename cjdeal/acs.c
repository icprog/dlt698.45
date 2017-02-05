/*
 * acs.c
 *
 *  Created on: Jan 25, 2017
 *      Author: ava
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "AccessFun.h"
#include "StdDataType.h"
#include "Objectdef.h"
#include "PublicFunction.h"
#include "att7022e.h"
#include "rn8209.h"
#include "spi.h"
#include "acs.h"
#include "ware.h"


ACCoe_SAVE  attCoef={};
ACEnergy_Sum	energysum={};		//电能寄存器值累计后的示值（四舍五入后数据）
INT32S 			spifp_rn8209=0; // RN8209打开spi句柄
INT32S 			spifp=0; 		// ATT7022打开spi句柄

static 	INT32U 	K_vrms = 8193;       // RN8209校表系数 748600
static 	INT32S 	device_id=-1;

extern sem_t * 		sem_check_fd;	//校表信号量

extern ProgramInfo* JProgramInfo;



//温度及补偿值对应公式
static INT16S AllGainVal[25][2]={
	{80,-180},{75,-165},{70,-150},{65,-130},{60,-110},{55,-90},{50,-70},{45,-50},//80'C,75'C,70'C,65'C,60'C,55'C,50'C,45'C
	{40,-30},{35,-10},{30,0},{25,0},{20,0},{15,8},{10,20},{5,37},//40'C,35'C,30'C,25'C,20'C,15'C,10'C,5'C
	{0,42},{-5,47},{-10,52},{-15,57},{-20,62},{-25,67},{-30,72},{-35,77},{-40,82}//0'C,-5'C,-10'C,-15'C,-20'C,-25'C,-30'C,-35'C
};

//高温Ua,Ub,Uc,Ia,Ib,Ic补偿系数
static FP32		HighTempCoef[6][2]={
		{30,1.000},{30,1.000},{30,1.000},
		{30,1.000},{30,1.000},{30,1.000}
};
//低温Ua,Ub,Uc,Ia,Ib,Ic补偿系数
static FP32		LowTempCoef[6][2]={
		{-60,1.000},{-60,1.000},{-60,1.000},
		{-60,1.000},{-60,1.000},{-60,1.000}
};

///共享内存定义:交采作为电表使用时，要进行设置
static INT8U	QFeatureWord1=0x05;		//无功组合特征字61
static INT8U	QFeatureWord2=0x50;		//无功组合特征字2

INT8U			thread_run_flag;//线程运行标记
INT32S			prog_pid;		//进程ID号
INT32S			spifp;			//ATT7022E打开spi句柄
_AverageStru    HZall;			//频率平均值计算缓冲区
_AverageStru    TempRegBuff;	//温度平均值计算缓冲区
_RealData		realdata;		//交采实时数据
_EnergyCurr		energycurr;		//电能量实时值
ACEnergy_Sum	energysum;		//电能寄存器值累计后的示值
_HarmonicD		HarmData;		//谐波数据
_WarePoint  	WarePoint;		//
ACCoe_SAVE	 	attCoef;		//校表系数
//Pname_DMCol_Strt 	*pDM;
sem_t * 		sem_check_fd;	//校表信号量
INT32U		chksum,oldchksum;
INT32U		chksum1,oldchksum1;
INT32S 		device_flag;

void check_reg_print(INT32S device_id);
void energysum_print();

/**************************************/
// 根据P总、Q总的方向判断当前象限（quadrant）值
/*************************************/
#define GETPHASE(Pt,Qt,Phase) {		\
		if(Pt>0 && Qt>0)	Phase=1;			\
		if(Pt<0 && Qt>0)	Phase=2;			\
		if(Pt<0 && Qt<0)	Phase=3;			\
		if(Pt>0 && Qt<0)	Phase=4;			\
}

/*
 *读高低温交采补偿系数
 *	"/nor/config/acgain.cfg"文件中逗号前面温度值，后面数据温度补偿值
 */
void read_tempgain_cfg()
{
	FILE *fp=NULL;
	INT8U filename[64];
	INT8U i;
	int temp,gainval;	//温度，补偿值

	memset(filename,0,sizeof(filename));
	sprintf((char *)filename,"%s/acgain.cfg",_ACSDIR_);
	fp = fopen((char *)filename,"r");
	if(fp != NULL)
	{
		for(i=0;i<25;i++) {
			fscanf(fp,"%d,%d",&temp,&gainval);
//			fprintf(stderr,"temp=%d,gainval=%d\n",temp,gainval);
			AllGainVal[i][0]=temp;
			AllGainVal[i][1]=gainval;
		}
		fclose(fp);
	}else {
		fprintf(stderr,"无温度补偿值配置文件，选择默认配置\n");
	}
	for(i=0;i<25;i++) {
		dbg_prt("交采：No:%d ,温度：%d, 补偿值：%d\n",i,AllGainVal[i][0],AllGainVal[i][1]);
	}
}

/*
 *读高低温交采补偿系数
 *	"/nor/config/tempcoef.cfg"
 *	文件每行配置：电压，温度，补偿系数，温度，补偿系数
 *	Ua,30,1.000,-60,1.000
 */
void read_tempcoef_cfg()
{
	FILE *fp=NULL;
	INT8U filename[64];
	INT8U i;
	char  type[8];
	INT8U  no=0;
	FP32 htemp,ltemp,hcoef,lcoef;	//温度，补偿值

	memset(filename,0,sizeof(filename));
	sprintf((char *)filename,"%s/tempcoef.cfg",_ACSDIR_);
	fp = fopen((char *)filename,"r");
	if(fp != NULL)
	{
		for(i=0;i<6;i++) {
			memset(type,0,sizeof(type));
			fscanf(fp,"%s",type);
			fscanf(fp,"%f,%f,%f,%f",&htemp,&hcoef,&ltemp,&lcoef);
	//		fprintf(stderr,"%s:%f=%f,%f=%f\n",type,htemp,hcoef,ltemp,lcoef);
			if(strcmp(type,"Ua")==0)	no=0;
			else if(strcmp(type,"Ub")==0)	no=1;
			else if(strcmp(type,"Uc")==0)	no=2;
			else if(strcmp(type,"Ia")==0)	no=3;
			else if(strcmp(type,"Ib")==0)	no=4;
			else if(strcmp(type,"Ic")==0)	no=5;
			if(hcoef==0) {hcoef=1.0;fprintf(stderr,"高温补偿系数文件设置0，修改为1");}
			if(lcoef==0) {lcoef=1.0;fprintf(stderr,"低温补偿系数文件设置0，修改为1");}
			HighTempCoef[no][0]=htemp;
			HighTempCoef[no][1]=hcoef;
			LowTempCoef[no][0]=ltemp;
			LowTempCoef[no][1]=lcoef;
		}
		fclose(fp);
	}else {
		fprintf(stderr,"无电压电流温度系数配置文件，默认乘系数1.0\n");
	}
	for(i=0;i<6;i++){
		dbg_prt("高温 no=%d,温度=%f（度）,补偿系数=%f\n",i,HighTempCoef[i][0],HighTempCoef[i][1]);
		dbg_prt("低温 no=%d,温度=%f（度）,补偿系数=%f\n",i,LowTempCoef[i][0],LowTempCoef[i][1]);
	}
}
/*
 * AllGainVal数组定义参照ATT7022EU应用笔记，全通道增益补偿说明
 * AllGainVal数组下标1：代表温度寄存器值，下标2：全通道增益补充寄存器值
 * 根据温度值得到AllGain增益补偿寄存器
 * 输入：tpsd：温度传感器寄存器值
 * */
INT32S get_allgain_reg(INT16S tpsd,INT32S *temp)
{
	static INT16S		oldtemp=100;
	int					i;
	static INT32S		allgain_val=1000;				//需要补偿的寄存器值

	*temp = tpsd;
	dbg_prt("tpsd=%d,*temp=%d,oldtemp=%d\n",tpsd,*temp,oldtemp);
	for(i=0;i<25;i++) {				//温度在15‘C-80’C ,tpsd 增长曲线
		if((tpsd>=AllGainVal[i+1][0]) && (tpsd<AllGainVal[i][0])) {
//			fprintf(stderr,"tpsd=%d,AllGain[%d]=%d,AllGain[%d]=%d\n",tpsd,i,AllGainVal[i][0],i+1,AllGainVal[i+1][0]);
			if(oldtemp!=tpsd) {
				allgain_val=AllGainVal[i][1];
				oldtemp = tpsd;
			}
			return allgain_val;
		}
	}
	if((tpsd>=AllGainVal[0][0]))		{
		if(oldtemp!=tpsd) {
			allgain_val=AllGainVal[0][1];
			oldtemp = *temp;
		}
		return allgain_val;
	}
	if((tpsd<=AllGainVal[24][0]))		{
		if(oldtemp!=tpsd) {
			allgain_val=AllGainVal[24][1];
			oldtemp = *temp;
		}
		return allgain_val;
	}
	return allgain_val;
}

/*
 * 根据Vref的温度校正，ATT7022E全通道增益校表寄存器
 * 返回值：目前温度值
 * */
INT32S write_allgain_reg(INT32S fp,INT32S val)
{
	static INT32S  LastTemp=0;		//保存上一次温度值
	static INT8U   TempNum=0;		//记录温度值计数，当连续采样5次温度值没变，认为有效
	INT8U temp[3];
	INT32S	regval;

	regval = get_allgain_reg(val,&realdata.Temp);
	dbg_prt("温度值=%d，增益补偿值=%d(H:%04x),累计次数=%d\n",realdata.Temp,regval,regval,TempNum);
//	if(LastTemp==realdata.Temp) TempNum++;
//	else {
//		LastTemp = realdata.Temp;
//		TempNum = 0;
//	}
	if(LastTemp!=realdata.Temp){
		TempNum++;
	}else {
		TempNum = 0;
	}
//	fprintf(stderr,"LastTemp=%d,TempNum=%d\n",LastTemp,TempNum);
	if(TempNum>=5 && regval!=1000 && val!=0){	//温度连续5次没有变化并且温度值不在25-35度，进行增益补偿寄存器修改
		TempNum = 0;
		LastTemp = realdata.Temp;
//		fprintf(stderr,"\n\n温度补偿:regval=%d,val=%d,realdata.Temp=%d\n",regval,val,realdata.Temp);
//		syslog(LOG_NOTICE,"temp=%d,write AllGain=%04x\n",realdata.Temp,regval);
		temp[0] = 0;
		temp[1] = 0;
		temp[2] = 0x5a;
		att_spi_write(fp, Reg_Enable, 3, temp); //允许写操作

		temp[0] = 0;
		temp[1] = (regval >>8) & 0xff;
		temp[2] = regval & 0xff;
		att_spi_write(fp, w_AllGain, 3, temp); //增益补偿寄存器值

		temp[0] = 0;
		temp[1] = 0;
		temp[2] = 1;
		att_spi_write(fp, Reg_Enable, 3, temp); //不允许写操作

		chksum =  att_spi_read(spifp, r_ChkSum, 3);
		oldchksum =  att_spi_read(spifp, r_ChkSum, 3);
	}
	return regval;
}

void WriteRegInit(INT32S fp)
{
	INT8U temp[3];
//	INT32S	DeviceId;
//	INT32U	sum;

	temp[0] = 0;
	temp[1] = 0;
	temp[2] = 0;
	att_spi_write(fp, Reg_Clean, 3, temp); //清校表寄存器

	temp[0] = 0;
	temp[1] = 0xb9;
	temp[2] = 0xfe;//bit0=0：关闭第7路ADC
	att_spi_write(fp, w_ModeCfg, 3, temp); //模式配置

	temp[0] = 0;
	temp[1] = 0x3d;//XIEBO			开启基波/谐波计量
	temp[2] = 0xC4;//配置能量寄存器读后清零，bit7置1//系统推荐写入F8 04 ,三相四线使用代数和累加方式
	att_spi_write(fp, w_EMUCfg, 3, temp); //EMU单元配置

//三相三启动功率  Pstartup=INT[0.6*Ub*Ib*HFconst*EC*k%. *2^23/(2.592*10^10)]
	temp[0] = 0;
	temp[1] = 0x00;
	temp[2] = 0x0a;
	att_spi_write(fp, w_Pstart, 3, temp); //起动功率设置寄存器

	temp[0] = 0;
	temp[1] = 0x34;
	temp[2] = 0x37;
	att_spi_write(fp, w_ModuleCFG, 3, temp); //模拟模块使能寄存器

	temp[0] = 0;
	temp[1] = 0;
	temp[2] = 0;
	att_spi_write(fp, w_QPhscal, 3, temp); //无功相位校正寄存器

	temp[0] = 0x00;
	temp[1] = 0x01;
	temp[2] = 0x6D;//365
	att_spi_write(fp, w_Hfconst, 3, temp); //高频脉冲常数
	//HFConst=INT[25920000000*G*G*Vu*Vi/(EC*Un*Ib)]  G=1.163   EC=6400
	//Vu=0.22  Vi=0.1  Un=220   Ib=1.5

	if(device_flag == ATT_VER_ID) {
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
		temp[2] = 0x02;//0x0A;//VrefAuto_En=1,YModesel=0,QEnergySel=0
		att_spi_write(fp, w_EMCfg, 3, temp);  //新算法控制器  ,YModesel=1:新算法使用角度算法
	}else {
		//Iregion=INT[Ib*N*33%*2^5]=1.5*80*0.33*2^5=1267=0x4f3
		temp[0] = 0x00;
		temp[1] = 0x04;
		temp[2] = 0xf3;
		att_spi_write(fp, w_Iregion, 3, temp);  //相位补偿区域设置寄存器,1.5*33%=0.495A时进行相位补偿分段
	}
}

/*
 * 读校表寄存器的值
 * */
INT32U	read_check_wreg(INT32S fp,INT8U	addr)
{
	INT32U wreg_val=0;
	INT8U temp[3];
//	INT32U	i,reg[256];

	temp[0] = 0x00;
	temp[1] = 0x00;
	temp[2] = 0x5A;
	att_spi_write(fp, Reg_Read, 3, temp); //选择读取校表寄存器数据

//	for(i=0;i<0x63;i++) {
//		reg[i] = att_spi_read(fp,i,3);
//		fprintf(stderr,"reg[%d]=%04x\n",i,reg[i]);
//	}
	wreg_val = att_spi_read(fp,addr,3);

	temp[0] = 0;
	temp[1] = 0;
	temp[2] = 1;
	att_spi_write(fp, Reg_Read, 3, temp); //不允许读操作
	return wreg_val;
}

/*
 * 写ATT7022E校表寄存器*/
INT32U write_coef_reg(INT32S fp)
{
	INT8U temp[3];
	INT32U	sum;

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
	att_spi_write(fp, w_GainADC7, 3, attCoef.I0);

	att_spi_write(fp, w_PgainA, 3, attCoef.PA);
	att_spi_write(fp, w_QgainA, 3, attCoef.PA);
	att_spi_write(fp, w_SgainA, 3, attCoef.PA);

	att_spi_write(fp, w_PgainB, 3, attCoef.PB);
	att_spi_write(fp, w_QgainB, 3, attCoef.PB);
	att_spi_write(fp, w_SgainB, 3, attCoef.PB);

	att_spi_write(fp, w_PgainC, 3, attCoef.PC);
	att_spi_write(fp, w_QgainC, 3, attCoef.PC);
	att_spi_write(fp, w_SgainC, 3, attCoef.PC);

//	temp[0] = 0x00;
//	temp[1] = 0x00;
//	temp[2] = 0x7;
//	att_spi_write(fp, w_IaRmsoffse, 3, temp);	//Irms=0时，校正时改值计算大约=7
//	att_spi_write(fp, w_IbRmsoffse, 3, temp);
//	att_spi_write(fp, w_IcRmsoffse, 3, temp);
//	att_spi_write(fp, w_UaRmsoffse, 3, temp);
//	att_spi_write(fp, w_UbRmsoffse, 3, temp);
//	att_spi_write(fp, w_UcRmsoffse, 3, temp);
	if(device_flag==ATT_VER_ID) {
		att_spi_write(fp, w_PhSregApq0, 3, attCoef.PhaseA0);
		att_spi_write(fp, w_PhSregBpq0, 3, attCoef.PhaseB0);
		att_spi_write(fp, w_PhSregCpq0, 3, attCoef.PhaseC0);

		att_spi_write(fp, w_PhSregApq1, 3, attCoef.PhaseA);
		att_spi_write(fp, w_PhSregBpq1, 3, attCoef.PhaseB);
		att_spi_write(fp, w_PhSregCpq1, 3, attCoef.PhaseC);

		att_spi_write(fp, w_PhSregApq2, 3, attCoef.PhaseA1);
		att_spi_write(fp, w_PhSregBpq2, 3, attCoef.PhaseB1);
		att_spi_write(fp, w_PhSregCpq2, 3, attCoef.PhaseC1);
	}else {
		att_spi_write(fp, w_PhSregApq0, 3, attCoef.PhaseA);
		att_spi_write(fp, w_PhSregBpq0, 3, attCoef.PhaseB);
		att_spi_write(fp, w_PhSregCpq0, 3, attCoef.PhaseC);

		att_spi_write(fp, w_PhSregApq1, 3, attCoef.PhaseA1);
		att_spi_write(fp, w_PhSregBpq1, 3, attCoef.PhaseB1);
		att_spi_write(fp, w_PhSregCpq1, 3, attCoef.PhaseC1);
	}
	temp[0] = 0;
	temp[1] = 0;
	temp[2] = 1;
	att_spi_write(fp, Reg_Enable, 3, temp); //不允许写操作
	//---------读取计量芯片类型---------------------------------------------------
	sum = att_spi_read(spifp, r_ChkSum, 3);
	sum = sum & 0xffffff;

	chksum1 = att_spi_read(spifp, r_ChkSum1, 3);
	chksum1 = chksum1 & 0xffffff;
	oldchksum1 = chksum1;

	dbg_prt("\n\r 大电流相角 校表系数");
	dbg_prt("PhaseA=%02x_%02x_%02x",attCoef.PhaseA0[0],attCoef.PhaseA0[1],attCoef.PhaseA0[2]);
	dbg_prt("PhaseB=%02x_%02x_%02x",attCoef.PhaseB0[0],attCoef.PhaseB0[1],attCoef.PhaseB0[2]);
	dbg_prt("PhaseC=%02x_%02x_%02x",attCoef.PhaseC0[0],attCoef.PhaseC0[1],attCoef.PhaseC0[2]);
	dbg_prt("\n\r 相角 校表系数");
	dbg_prt("PhaseA=%02x_%02x_%02x",attCoef.PhaseA[0],attCoef.PhaseA[1],attCoef.PhaseA[2]);
	dbg_prt("PhaseB=%02x_%02x_%02x",attCoef.PhaseB[0],attCoef.PhaseB[1],attCoef.PhaseB[2]);
	dbg_prt("PhaseC=%02x_%02x_%02x",attCoef.PhaseC[0],attCoef.PhaseC[1],attCoef.PhaseC[2]);
	dbg_prt("\n\r 小电流相角 校表系数");
	dbg_prt("PhaseA=%02x_%02x_%02x",attCoef.PhaseA1[0],attCoef.PhaseA1[1],attCoef.PhaseA1[2]);
	dbg_prt("PhaseB=%02x_%02x_%02x",attCoef.PhaseB1[0],attCoef.PhaseB1[1],attCoef.PhaseB1[2]);
	dbg_prt("PhaseC=%02x_%02x_%02x",attCoef.PhaseC1[0],attCoef.PhaseC1[1],attCoef.PhaseC1[2]);
	dbg_prt("\n\r 电压 校表系数");
	dbg_prt("UA=%02x_%02x_%02x",attCoef.UA[0],attCoef.UA[1],attCoef.UA[2]);
	dbg_prt("UB=%02x_%02x_%02x",attCoef.UB[0],attCoef.UB[1],attCoef.UB[2]);
	dbg_prt("UC=%02x_%02x_%02x",attCoef.UC[0],attCoef.UC[1],attCoef.UC[2]);
	dbg_prt("\n\r 电流 校表系数");
	dbg_prt("IA=%02x_%02x_%02x",attCoef.IA[0],attCoef.IA[1],attCoef.IA[2]);
	dbg_prt("IB=%02x_%02x_%02x",attCoef.IB[0],attCoef.IB[1],attCoef.IB[2]);
	dbg_prt("IC=%02x_%02x_%02x",attCoef.IC[0],attCoef.IC[1],attCoef.IC[2]);
	dbg_prt("\n\r 功率 校表系数");
	dbg_prt("PA=%02x_%02x_%02x",attCoef.PA[0],attCoef.PA[1],attCoef.PA[2]);
	dbg_prt("PB=%02x_%02x_%02x",attCoef.PB[0],attCoef.PB[1],attCoef.PB[2]);
	dbg_prt("PC=%02x_%02x_%02x",attCoef.PC[0],attCoef.PC[1],attCoef.PC[2]);
	dbg_prt("\n\r 谐波电压系数");
	dbg_prt("Ua=%d, Ub=%d, Uc=%d",attCoef.HarmUCoef[0],attCoef.HarmUCoef[1],attCoef.HarmUCoef[2]);
	dbg_prt("\n\r 谐波电流系数");
	dbg_prt("Ia=%d, Ib=%d, Ic=%d",attCoef.HarmICoef[0],attCoef.HarmICoef[1],attCoef.HarmICoef[2]);
	return sum;
}

// 谐波平均值计算
// 输入：hz：频率采样数组，temphz：实时值
// 返回：频率值
INT32U ave_hz(INT32U *hz,INT32U temphz,INT8U Len)
{
	INT8U  i;
	INT32U SumHz;
	SumHz =0;
	//  j = sizeof(hz);
	for(i=0;i<Len;i++)
	SumHz = SumHz + hz[i];
	if(Len!=0)SumHz = SumHz/Len;
	if((abs(SumHz - temphz)) >10*FREQ_COEF)      		//两者相差10HZ
		return temphz;                    //返回瞬时的频率
	else
		return SumHz;                     //否则返回平均频率
}

// 温度平均值计算
// 输入：temp：温度寄存器采样数组，realtemp：实时值
// 返回：频率值
/// INT32U ave_temp(INT32U *temp,INT32U realtemp,)
// {
//	INT8U  i,j;
//	INT32U SumTemp;
//	SumTemp =0;
//	j = sizeof(temp);
//	for(i=0;i<j;i++)
//		SumTemp = SumTemp + temp[i];
//  SumTemp = SumTemp/j;
//	return SumTemp;                     //否则返回平均频率
// }
// 换算计量寄存器值
//输入参数：type：采样数据类型,para:计算倍数, reg：计量寄存器值  temp:温度值，用于温度补偿
//返回值    ：实时采样值
INT32S	trans_regist(INT8U	type,INT8U para,INT32S reg,INT32S temp)
{
//	INT32S		val=0;
	INT32S		tread=0;
	float			tmpval=0;
	#define N			80
	switch(type){
	case U:					//电压
		tread = reg * U_COEF / 8192;
		if(tread <= 10*U_COEF)  tread=0;		//特殊处理，当电压<10V,进行零飘处理，清除0
//		val = tread * (1.0*(1+(temp-15)*0.00008));
		break;
	case I:					//电流
//		fprintf(stderr,"Ireg=%d\n",reg);
		tmpval = (float)reg * I_COEF /8192/N;
		tread = (INT32S)tmpval;
//		val = tread * (1.0*(1+(temp-15)*0.00005));
		break;
	case P:					//有功	reg:24位数据,补码形式,	如果reg>2^23,则val=reg-2^24,	否则 val=reg*K
		if (reg > 8388608)				//2^23=8388608
			tread = reg -16777216;	//2^24=16777216
		else tread = reg;
		tmpval = tread*P_COEF/756;
		tread =  tmpval*para;
//		val = tread *(1.0*(1+(temp-15)*0.0001));
		break;
	case Q:					//无功
		if (reg > 8388608)
			tread = reg -16777216;
		else tread = reg;
		tmpval = tread*Q_COEF/756;
		tread =  tmpval*para;
//		val =  tread*(1.0*(1+(temp-15)*1.2247*0.0001));
		break;
	case S:					//视在功率总是大于或者等于0,所以符号位始终为0。
//		3-4
		tmpval = reg*S_COEF/756;
		tread = tmpval*para;
		break;
	case COS:				//功率因数
		if (reg > 8388608)
			tread = reg -16777216;
		else tread = reg;
		tmpval = (float)tread/8388608;
		tread =  tmpval*COS_COEF;
		break;
	case FREQ:			//频率
		tread = reg*FREQ_COEF/8192;
		if( (tread>56*FREQ_COEF)||(tread<44*FREQ_COEF) )  			 //45hz --- 55hz之间
			HZall.Buff[HZall.point]  = 50*FREQ_COEF;
		else  HZall.Buff[HZall.point]  = tread;
		tread = ave_hz(HZall.Buff,HZall.Buff[HZall.point],HZall.point);
//		val = tread;
		HZall.point = (HZall.point + 1) & 0xf;
		break;
	case PHASE:		//电压与电流相角
//		dbg_prt("phs=%d\n",reg);
		reg = reg & 0x1FFFFF;
		if (reg >= 1048576)						//2^20=1048576
			tread = (reg -16777216) |0xFFF00000;						//填符号位
		else tread = reg;
		tmpval = tread*180.0/1048576;
		tread = tmpval*PHASE_COEF;
//		val = tread;
		break;
	case ANGLE:		//电压夹角
		tmpval = reg*180.0/1048576;
		tread = tmpval*ANGLE_COEF;
//		val = tread;
		break;
	case TEMP:		//温度测试
		if(reg>128) 	tread=reg-256;
		else tread = reg;
		tread = 25-tread*0.726;
		dbg_prt("寄存器值=%d,温度值=%d\n",reg,tread);
		break;
	}
	return   tread;				//返回无温度补偿值
	//return	val;
}

/* 读ATT7022E芯片的计量寄存器
 * 换算采样数据
 * 返回：温度寄存器值，
 */
INT32U read_regist(INT32S fp,INT32S device_type)
{
	INT32S 	RRec[128];				//7022E计量参数寄存器数据（Read Only）
	int i;
	INT32S 	tmm;
	INT32S UabJ,UacJ,UbcJ;
	INT32S IaJ=0,IbJ=0,IcJ=0;

	RRec[r_ChkSum] = att_spi_read(fp, r_ChkSum, 3);	//校表数据校验和
	RRec[r_ChkSum] = RRec[r_ChkSum] & 0xffffff;
	chksum = RRec[r_ChkSum];
//	dbg_prt("Rec[ChkSum]=%x\n",RRec[r_ChkSum]);
	for (i = 0; i <= r_Freq; i++) {											//常规数据
		RRec[i] = att_spi_read(fp, i, 3);
	}
	for (i = r_YUaUb; i <=r_YUbUc; i++) {											//常规数据
		RRec[i] = att_spi_read(fp, i, 3);
	}
	for (i = r_LineUaRrms; i <= r_LineIcRrms; i++) { 		//基波电压电流有效值
		RRec[i] = att_spi_read(fp, i, 3);
	}
	RRec[r_SFlag] = att_spi_read(fp, r_SFlag, 3);
	RRec[r_PFlag] = att_spi_read(fp, r_PFlag, 3);
//	dbg_prt("r_SFlag=%03x,r_PFlag=%02x\n",RRec[r_SFlag],RRec[r_PFlag]);
	RRec[r_TPSD] = att_spi_read(fp, r_TPSD, 3);
	RRec[r_TPSD] = RRec[r_TPSD] & 0x00ff;					//温度传感器输出，用于温度补偿
	if(RRec[r_TPSD]>128)
		tmm=RRec[r_TPSD]-256;
	else
		tmm=RRec[r_TPSD];
	energycurr.EPa = att_spi_read(fp, r_Epa, 3);
	energycurr.EPb = att_spi_read(fp, r_Epb, 3);
	energycurr.EPc = att_spi_read(fp, r_Epc, 3);
	energycurr.EPt = att_spi_read(fp, r_Ept, 3);
	energycurr.EQa = att_spi_read(fp, r_Eqa, 3);
	energycurr.EQb = att_spi_read(fp, r_Eqb, 3);
	energycurr.EQc = att_spi_read(fp, r_Eqc, 3);
	energycurr.EQt = att_spi_read(fp, r_Eqt, 3);

//	fprintf(stderr,"r_TPSD=%d,tmm=%d\n",RRec[r_TPSD],tmm);
	realdata.Temp = trans_regist(TEMP,1,RRec[r_TPSD],tmm);
	realdata.Pa = trans_regist(P,1,RRec[r_Pa],tmm);
	realdata.Pb = trans_regist(P,1,RRec[r_Pb],tmm);
	realdata.Pc = trans_regist(P,1,RRec[r_Pc],tmm);
	realdata.Pt = trans_regist(P,2,RRec[r_Pt],tmm);
	realdata.Qa = trans_regist(Q,1,RRec[r_Qa],tmm);
	realdata.Qb = trans_regist(Q,1,RRec[r_Qb],tmm);
	realdata.Qc = trans_regist(Q,1,RRec[r_Qc],tmm);
	realdata.Qt = trans_regist(Q,2,RRec[r_Qt],tmm);
	realdata.Sa = trans_regist(S,1,RRec[r_Sa],tmm);
	realdata.Sb = trans_regist(S,1,RRec[r_Sb],tmm);
	realdata.Sc = trans_regist(S,1,RRec[r_Sc],tmm);
	realdata.St = trans_regist(S,2,RRec[r_St],tmm);
//	realdata.St = realdata.Sa + realdata.Sb + realdata.Sc;		//trans_regist(S,2,RRec[r_St],tmm);
	realdata.Ua = trans_regist(U,1,RRec[r_UaRms],tmm);
	realdata.Ub = trans_regist(U,1,RRec[r_UbRms],tmm);
	realdata.Uc = trans_regist(U,1,RRec[r_UcRms],tmm);
	realdata.Ia = trans_regist(I,1,RRec[r_IaRms],tmm);
	realdata.Ib = trans_regist(I,1,RRec[r_IbRms],tmm);
	realdata.Ic = trans_regist(I,1,RRec[r_IcRms],tmm);
	//送检为了补偿电压电流精度，人工置入系数进行补偿
	if(realdata.Temp>=HighTempCoef[0][0]) realdata.Ua=realdata.Ua*HighTempCoef[0][1];
	else if(realdata.Temp<=LowTempCoef[0][0]) realdata.Ua=realdata.Ua*LowTempCoef[0][1];
	if(realdata.Temp>=HighTempCoef[1][0]) realdata.Ub=realdata.Ub*HighTempCoef[1][1];
	else if(realdata.Temp<=LowTempCoef[1][0]) realdata.Ub=realdata.Ub*LowTempCoef[1][1];
	if(realdata.Temp>=HighTempCoef[2][0]) realdata.Uc=realdata.Uc*HighTempCoef[2][1];
	else if(realdata.Temp<=LowTempCoef[2][0]) realdata.Uc=realdata.Uc*LowTempCoef[2][1];
	if(realdata.Temp>=HighTempCoef[3][0]) realdata.Ia=realdata.Ia*HighTempCoef[3][1];
	else if(realdata.Temp<=LowTempCoef[3][0]) realdata.Ia=realdata.Ia*LowTempCoef[3][1];
	if(realdata.Temp>=HighTempCoef[4][0]) realdata.Ib=realdata.Ib*HighTempCoef[4][1];
	else if(realdata.Temp<=LowTempCoef[4][0]) realdata.Ib=realdata.Ib*LowTempCoef[4][1];
	if(realdata.Temp>=HighTempCoef[5][0]) realdata.Ic=realdata.Ic*HighTempCoef[5][1];
	else if(realdata.Temp<=LowTempCoef[5][0]) realdata.Ic=realdata.Ic*LowTempCoef[5][1];

	RRec[r_I0Rms]=0;	//2013年新规范无零序电流
	realdata.I0 = trans_regist(I,1,RRec[r_I0Rms],tmm);
	realdata.CosA = trans_regist(COS,1,RRec[r_Pfa],tmm);
	realdata.CosB = trans_regist(COS,1,RRec[r_Pfb],tmm);
	realdata.CosC = trans_regist(COS,1,RRec[r_Pfc],tmm);
	realdata.Cos = trans_regist(COS,1,RRec[r_Pft],tmm);
	if((realdata.Pa>=0) && (realdata.Pa<=10)) realdata.CosA = 1*COS_COEF;	//没有功率时，功率因数=1
	if((realdata.Pb>=0) && (realdata.Pb<=10)) realdata.CosB = 1*COS_COEF;
	if((realdata.Pc>=0) && (realdata.Pc<=10)) realdata.CosC = 1*COS_COEF;
	if((realdata.Pt>=0) && (realdata.Pt<=10)) realdata.Cos = 1*COS_COEF;
	realdata.Freq = trans_regist(FREQ,1,RRec[r_Freq],tmm);

	realdata.LineUa = trans_regist(U,1,RRec[r_LineUaRrms],tmm);
	realdata.LineUb = trans_regist(U,1,RRec[r_LineUbRrms],tmm);
	realdata.LineUc = trans_regist(U,1,RRec[r_LineUcRrms],tmm);
	realdata.LineIa = trans_regist(I,1,RRec[r_LineIaRrms],tmm);
	realdata.LineIb = trans_regist(I,1,RRec[r_LineIbRrms],tmm);
	realdata.LineIc = trans_regist(I,1,RRec[r_LineIcRrms],tmm);
	realdata.SFlag = RRec[r_SFlag]&0xfff;
	realdata.PFlag = RRec[r_PFlag]&0xff;

	UabJ = trans_regist(ANGLE,1,RRec[r_YUaUb],tmm);
	UacJ = trans_regist(ANGLE,1,RRec[r_YUaUc],tmm);
	UbcJ = trans_regist(ANGLE,1,RRec[r_YUbUc],tmm);
	IaJ = trans_regist(PHASE,1,RRec[r_Pga],tmm);
	IbJ = trans_regist(PHASE,1,RRec[r_Pgb],tmm);
	IcJ = trans_regist(PHASE,1,RRec[r_Pgc],tmm);

//	fprintf(stderr,"IJ=%d-%d-%d  ",IaJ,IbJ,IcJ);
	//电压相位角以UA为基准，逆时针方向计算，范围为0-360°
	//Ua电压相位角：0°
	//Ub电压相位角：Ua逆时针方向旋转到Ub的夹角
	//Uc电压相位角：Ua逆时针方向旋转到Uc的夹角
	realdata.YUaUb = 0;				//Ua为基准角度0
	realdata.YUaUc = 360*PHASE_COEF - UabJ;		//Ub表示Ua与Ub之间角度，逆时针
	realdata.YUbUc = 360*PHASE_COEF - UacJ ;	//Uc表示Ua与Uc之间角度，逆时针
	//电流夹角以UA为基准，逆时针方向计算，范围0-360
	//Ia电流相位角：Ua逆时针方向旋转到Ia的夹角
	//Ib电流相位角：Ua逆时针方向旋转到Ib的夹角
	//Ic电流相位角：Ua逆时针方向旋转到Ic的夹角
	IaJ = realdata.YUaUb - IaJ;		//Ia与Ua夹角，
	IbJ = realdata.YUaUc - IbJ;		//Ib与Ua夹角
	IcJ = realdata.YUbUc - IcJ;		//Ic与Ua夹角
	//采样到的负数转换到正值
	if(IaJ < 0) realdata.Pga = IaJ + 360*PHASE_COEF;
	else realdata.Pga = IaJ;
	if(IbJ < 0) realdata.Pgb = IbJ + 360*PHASE_COEF;
	else realdata.Pgb = IbJ;
	if(IcJ < 0) realdata.Pgc = IcJ + 360*PHASE_COEF;
	else realdata.Pgc = IcJ;

//	fprintf(stderr,"\nUJ=%d  %d  %d  realU=%d  %d  %d\n",UabJ,UacJ,UbcJ,realdata.YUaUb,realdata.YUaUc,realdata.YUbUc);
//	fprintf(stderr,"\nIJ=%d  %d  %d  realI=%d  %d  %d\n",IaJ,IbJ,IcJ,realdata.Pga,realdata.Pgb,realdata.Pgc);
//	dbg_prt("SFlag=%03x,PFlag=%02x，TPSD=%d\n",realdata.SFlag,realdata.PFlag,RRec[r_TPSD]);
//	TempRegBuff.Buff[TempRegBuff.point] = RRec[r_TPSD];
//	AvgTemp = ave_temp(TempRegBuff.Buff,TempRegBuff.Buff[HZall.point]);
//	TempRegBuff.point = (TempRegBuff.point + 1) & 0xf;
//	return AvgTemp;

	//fprintf(stderr,"tmm=%d\n",tmm);
	return realdata.Temp;//RRec[r_TPSD];
}


/*
 * 计算1分内的电压平均值
 * 输入参数：ts：当前时间
 * */
void calc_minute_ave_u(TS ts,INT32U ua,INT32U ub,INT32U uc,INT32U *avgua,INT32U *avgub,INT32U *avguc)
{
	static TS			lastts;
	static INT8U		initflag=1,wflag=1;
	static INT8U		wcount=0,calc_num;
	static INT8U		i;
	static	INT32U	TmpUa[60],TmpUb[60],TmpUc[60];
	INT32U				SumUa=0,SumUb=0,SumUc=0;

	if(initflag) {
		lastts.Sec = ts.Sec;
		initflag = 0;
		SumUa = 0;
		SumUb = 0;
		SumUc = 0;
		//dbg_prt("\nenter up");
	}
	if(ts.Sec!=lastts.Sec) {		//1sec采样一次
		//dbg_prt("\nenter down");
		lastts.Sec = ts.Sec;
		if(wflag) {						//第一次填充1分钟电压缓冲区
			TmpUa[wcount] = ua;
			TmpUb[wcount] = ub;
			TmpUc[wcount] = uc;
			wcount = (wcount+1)%60;if(initflag) {
				lastts.Sec = ts.Sec;
				initflag = 0;
				SumUa = 0;
				SumUb = 0;
				SumUc = 0;
			}
			if(wcount==0)		{
				wflag=0;
				calc_num = 60;
			}else calc_num = wcount;
		}else {
			TmpUa[wcount] = ua;
			TmpUb[wcount] = ub;
			TmpUc[wcount] = uc;
			wcount = (wcount+1)%60;
			calc_num = 60;
		}

//		dbg_prt("calc_num=%d\n",calc_num);
		for(i=0;i<calc_num;i++) {
			SumUa += TmpUa[i];
			SumUb += TmpUb[i];
			SumUc += TmpUc[i];
		}
		*avgua = SumUa/calc_num;
		*avgub = SumUb/calc_num;
		*avguc = SumUc/calc_num;
	}
}

/*
*1分钟计算统计一次电能示值
*根据ATT7022E——0x3d寄存器有功无功方向，判断当前电能量方向及数值
*输入参数：
*EPa:A相有功电能量		EPb:B相有功电能量  		EPc:C相有功电能量		EPt:合相有功电能量
*EQa:A相无功电能量		EQb:B相无功电能量  		EQc:C相无功电能量		EQt:合相无功电能量
*PFlag:有功、无功功率方向寄存器
*			  Bit00 :=1,表示A相有功功率反向，=0， 正向
*			  Bit01 :=1,表示B相有功功率反向，=0， 正向
*			  Bit02 :=1,表示C相有功功率反向，=0， 正向
*			  Bit03 :=1,表示合相有功功率反向，=0，正向
*			  Bit04 :=1,表示A相无功功率反向，=0， 正向
*			  Bit05 :=1,表示B相无功功率反向，=0， 正向
*			  Bit06 :=1,表示C相无功功率反向，=0， 正向
*			  Bit07 :=1,表示合相无功功率反向，=0，正向
*/
void calc_energy(TS nowts,INT32U PFlag,_EnergyCurr *ed)
{
	if(PFlag&0x01) //Bit00=1:A相反相有功电能量
	{
		ed->NegPa= ed->EPa;
		ed->PosPa = 0;
	}else	{
		ed->PosPa = ed->EPa;
		ed->NegPa= 0;
	}
	if(PFlag&0x02)//Bit01=1:B相反相有功电能量
	{
		ed->NegPb= ed->EPb;
		ed->PosPb = 0;
	}else {
		ed->PosPb = ed->EPb;
		ed->NegPb= 0;
	}
	if(PFlag&0x04) //Bit02=1:C相反相有功电能量
	{
		ed->NegPc= ed->EPc;
		ed->PosPc = 0;
	}else {
		ed->PosPc = ed->EPc;
		ed->NegPc= 0;
	}
	if(PFlag&0x08)  //Bit03=1:反向合相有功电能
	{
		ed->NegPt= ed->EPt;
		ed->PosPt = 0;
	}else {
		ed->PosPt = ed->EPt;
		ed->NegPt= 0;
	}
	if(PFlag&0x10) //Bit04=1:反向A相无功电能
	{
		ed->NegQa= ed->EQa;
		ed->PosQa = 0;
	}else {
		ed->PosQa = ed->EQa;
		ed->NegQa= 0;
	}
	if(PFlag&0x20)//Bit05=1:反向B相无功电能
	{
		ed->NegQb= ed->EQb;
		ed->PosQb = 0;
	}else {
		ed->PosQb = ed->EQb;
		ed->NegQb= 0;
	}
	if(PFlag&0x40)//Bit06=1:反向C相无功电能
	{
		ed->NegQc= ed->EQc;
		ed->PosQc = 0;
	}else {
		ed->PosQc = ed->EQc;
		ed->NegQc= 0;
	}
	if(PFlag&0x80) //Bit07=1:反向合相无功电能
	{
		ed->NegQt= ed->EQt;
		ed->PosQt = 0;
	}else {
		ed->PosQt = ed->EQt;
		ed->NegQt= 0;
	}
	dbg_prt("\ncalc_energy\n");
	dbg_prt("REC Ep:%d,%d,%d,%d\n",ed->EPt,ed->EPa,ed->EPb,ed->EPc);
	dbg_prt("REC Eq:%d,%d,%d,%d\n",ed->EQt,ed->EQa,ed->EQb,ed->EQc);
	dbg_prt("PFlag=%02X\n",PFlag);
	dbg_prt("PosP:%d,%d,%d,%d\n",ed->PosPt,ed->PosPa,ed->PosPb,ed->PosPc);
	dbg_prt("NegP:%d,%d,%d,%d\n",ed->NegPt,ed->NegPa,ed->NegPb,ed->NegPc);
	dbg_prt("PosQ:%d,%d,%d,%d\n",ed->PosQt,ed->PosQa,ed->PosQb,ed->PosQc);
	dbg_prt("NegQ:%d,%d,%d,%d\n",ed->NegQt,ed->NegQa,ed->NegQb,ed->NegQc);
}

/*
 * 根据主站F21终端电能量费率时段和费率数设置
 * 读取当前的费率数
 */
//TODO:按照4016 当前套日时段表获取
INT8U get_rates_no(TS ts_t,CLASS_4016 class4016)
{
	INT8U	i=0;
	for(i=0;i<MAX_PERIOD_RATE;i++) {

	}
}

/**************************************/
//根据当前费率值，计算累加的电能示值
/*************************************/
void sum_energy(TS nowts,INT8U nRate,_EnergyCurr ed,ACEnergy_Sum *es)
{
	dbg_prt("当前费率数：%d\n",nRate);
	es->PosPa_All = es->PosPa_All + ed.PosPa;
	es->PosPb_All = es->PosPb_All + ed.PosPb;
	es->PosPc_All = es->PosPc_All + ed.PosPc;
	es->PosPt_All = es->PosPt_All + ed.PosPt;
	es->PosPt_Rate[nRate] = es->PosPt_Rate[nRate] + ed.PosPt;
//		shmm_getpubdata()->ac_energy.PosPt_All=es->PosPa_All;
//		shmm_getpubdata()->ac_energy.PosPt_Rate[nRate]=es->PosPt_Rate[nRate];

	es->NegPa_All = es->NegPa_All + ed.NegPa;
	es->NegPb_All = es->NegPb_All + ed.NegPb;
	es->NegPc_All = es->NegPc_All + ed.NegPc;
	es->NegPt_All = es->NegPt_All + ed.NegPt;
	es->NegPt_Rate[nRate] = es->NegPt_Rate[nRate] + ed.NegPt;

	es->PosQa_All = es->PosQa_All + ed.PosQa;
	es->PosQb_All = es->PosQb_All + ed.PosQb;
	es->PosQc_All = es->PosQc_All + ed.PosQc;

	es->NegQa_All = es->NegQa_All + ed.NegQa;
	es->NegQb_All = es->NegQb_All + ed.NegQb;
	es->NegQc_All = es->NegQc_All + ed.NegQc;

	dbg_prt("总：PosPt=%d, NegPt=%d, PosQt=%d, NegQt=%d\n",
			es->PosPt_All,es->NegPt_All,es->PosQt_All,es->NegQt_All);
	dbg_prt("正向有功:	Pa=%d, Pb=%d, Pc=%d\n\r",
			es->PosPa_All,es->PosPb_All,es->PosPc_All);
	dbg_prt("反向有功:	Pa=%d, Pb=%d, Pc=%d\n\r",
			es->NegPa_All,es->NegPb_All,es->NegPc_All);
	dbg_prt("正向无功:	Qa=%d, Qb=%d, Qc=%d\n\r",
			es->PosQa_All,es->PosQb_All,es->PosQc_All);
	dbg_prt("反向无功:	Qa=%d, Qb=%d, Qc=%d\n\r",
			es->NegQa_All,es->NegQb_All,es->NegQc_All);
}

/*
*根据当前象限值进行四象限的无功电能示值累加
*组合无功1、2计算
*组合无功电能1、2根据1--4象限无功总电能以及特征字进行矢量和
*组合无功特征字1初始化为0x05（一二象限相加），组合无功特征字2初始化为0x50（三四象限相加）
*无功组合方式 1、2 特征字:
*	Bit7 		Bit6 		Bit5 		Bit4 		Bit3 		Bit2 		Bit1  		Bit0
*	IV 象限 		IV 象限 		III 象限 	III 象限 	II 象限 		II 象限 		I 象限 		I 象限
* (0 不减, 减) (0 不加, 加) (0 不减, 减) (0 不加, 加) (0 不减, 减) (0 不加, 加) (0 不减, 减) (0 不加, 加)
*/
void sum_reactive_energy(TS nowts,INT8U nRate,INT32U PFlag,INT8U QWord1,INT8U QWord2,INT32U EngPosQt,INT32U EngNegQt,ACEnergy_Sum *es)
{
	INT8U	flag[8],i;
	INT8U 	NowPhaseVal=0;	//当前象限值

	//		GETPHASE(Pt,Qt,phaseVal);			//得到当前象限值
	//根据PFlag寄存器判断Qt，Pt方向
	for(i=0;i<8;i++) {
		flag[i] = (PFlag>>i)&0x01;
	}
	if(flag[3]==0 && flag[7]==0) {		//一象限值,Pt>0,Qt>0
		es->Q1_Qt_All = es->Q1_Qt_All + EngPosQt;
		es->Q1_Qt_Rate[nRate] = es->Q1_Qt_Rate[nRate] + EngPosQt;
		NowPhaseVal = 1;
	}
	if(flag[3]==1 && flag[7]==0) {		//二象限值,Pt<0,Qt>0
		es->Q2_Qt_All = es->Q2_Qt_All + EngPosQt;
		es->Q2_Qt_Rate[nRate] = es->Q2_Qt_Rate[nRate] + EngPosQt;
		NowPhaseVal = 2;
	}
	if(flag[3]==1 && flag[7]==1) {		//三象限值,Pt<0,Qt<0
		es->Q3_Qt_All = es->Q3_Qt_All + EngNegQt;
		es->Q3_Qt_Rate[nRate] = es->Q3_Qt_Rate[nRate] + EngNegQt;
		NowPhaseVal = 3;
	}
	if(flag[3]==0 && flag[7]==1) {		//四象限值,Pt>0,Qt<0
		es->Q4_Qt_All = es->Q4_Qt_All + EngNegQt;
		es->Q4_Qt_Rate[nRate] = es->Q4_Qt_Rate[nRate] + EngNegQt;
		NowPhaseVal = 4;
	}
	dbg_prt("PFlag=%02x,flag[3]=%d,flag[7]=%d\n",PFlag,flag[3],flag[7]);
	dbg_prt("Q1=%d,Q2=%d,Q3=%d,Q4=%d\n",es->Q1_Qt_All,es->Q2_Qt_All,es->Q3_Qt_All,es->Q4_Qt_All);
	//无功组合特征字1：正向无功
	if((NowPhaseVal==1) || (NowPhaseVal==2)) {
		if((QWord1 & 0x01) || ((QWord1>>2)&0x01)) {   		//I.II象限(Qt>0)加
			es->PosQt_All = es->PosQt_All+EngPosQt;
			es->PosQt_Rate[nRate] = es->PosQt_Rate[nRate]+EngPosQt;
		}
		if(((QWord1>>1) & 0x01) || ((QWord1>>3)&0x01)) {   //I.II象限(Qt>0)减
			es->PosQt_All = es->PosQt_All-EngPosQt;
			es->PosQt_Rate[nRate] = es->PosQt_Rate[nRate]-EngPosQt;
		}
	}
	if((NowPhaseVal==3) || (NowPhaseVal==4)) {
		if(((QWord1>>4) & 0x01) || ((QWord1>>6)&0x01)) {   //III.IIII象限(Qt<0)加
			es->PosQt_All = es->PosQt_All+EngNegQt;
			es->PosQt_Rate[nRate] = es->PosQt_Rate[nRate]+EngNegQt;
		}
		if(((QWord1>>5) & 0x01) || ((QWord1>>7)&0x01)) {   //III.IIII象限(Qt<0)减
			es->PosQt_All = es->PosQt_All-EngNegQt;
			es->PosQt_Rate[nRate] = es->PosQt_Rate[nRate]-EngNegQt;
		}
	}
	if((NowPhaseVal==1) || (NowPhaseVal==2)) {
		//无功组合特征字2：反向无功
		if((QWord2 & 0x01) || ((QWord2>>2)&0x01)) {   		//I.II象限(Qt>0)加
			es->NegQt_All = es->NegQt_All+EngPosQt;
			es->NegQt_Rate[nRate] = es->NegQt_Rate[nRate]+EngPosQt;
		}
		if(((QWord2>>1) & 0x01) || ((QWord2>>3)&0x01)) {   //I.II象限(Qt>0)减
			es->NegQt_All = es->NegQt_All-EngPosQt;
			es->NegQt_Rate[nRate] = es->NegQt_Rate[nRate]-EngPosQt;
		}
	}
	if((NowPhaseVal==3) || (NowPhaseVal==4)) {
		if(((QWord2>>4) & 0x01) || ((QWord2>>6)&0x01)) {   //III.IIII象限(Qt<0)加
			es->NegQt_All = es->NegQt_All+EngNegQt;
			es->NegQt_Rate[nRate] = es->NegQt_Rate[nRate]+EngNegQt;
		}
		if(((QWord2>>5) & 0x01) || ((QWord2>>7)&0x01)) {   //III.IIII象限(Qt<0)减
			es->NegQt_All = es->NegQt_All-EngNegQt;
			es->NegQt_Rate[nRate] = es->NegQt_Rate[nRate]-EngNegQt;
		}
	}
}

// 读ATT7022E芯片的电能计量寄存器
// 换算采样数据
void read_engergy_regist(TS ts,INT32S fp)
{
	INT8U  nRate = 0;
	CLASS_4016	class4016;

	nRate = get_rates_no(ts,class4016);	//得到当前费率数
	/////////////TODO:
	nRate = 4;
	calc_energy(ts,realdata.PFlag,&energycurr);	//计算电能示值
	sum_energy(ts,nRate,energycurr,&energysum);
	sum_reactive_energy(ts,nRate,realdata.PFlag,QFeatureWord1,QFeatureWord2,energycurr.PosQt,energycurr.NegQt,&energysum);
}

void InitACSCoef()
{
	int	readret=0;
	int i=0;
	// 读ATT7022E芯片的校表系数
	readret = readCoverClass(0,0,&attCoef,sizeof(ACCoe_SAVE),acs_coef_save);
	if(readret!=1){
		syslog(LOG_NOTICE,"校表系数文件 coef.par 丢失，请校表！！！\n");
		fprintf(stderr,"校表系数文件 coef.par 丢失，请校表！！！\n");
	}
	for(i=0;i<3;i++) {
		if(attCoef.HarmUCoef[i]==0) attCoef.HarmUCoef[i]=1;
		if(attCoef.HarmICoef[i]==0) attCoef.HarmICoef[i]=1;
	}
}

void InitACSEnergy()
{
	int	readret=0;

	// 读电能示值累加值数据
	memset(&energysum,0,sizeof(ACEnergy_Sum));
	readret = readCoverClass(0,0,&energysum,sizeof(ACEnergy_Sum),acs_energy_save);
	if(readret==1) {		//读取文件成功
		energysum_print(energysum);
	}else {				//TODO:失败是否考虑读取日冻结数据作为基准值累计？清数据区？
		fprintf(stderr,"电量累计值文件 energy.dat 丢失 readret=%d\n",readret);
		syslog(LOG_NOTICE,"电量累计值文件 energy.dat 丢失\n");
	}
}

/*
 * 读取计量芯片类型
 * =0x820900  	RN8209
 * =1 		  	ATT7022E
 * =0x7022E0:	ATT7022E-D芯片
 * */
INT32S  InitACSChip()
{
	INT32S 	device_id = ATT_VER_ID;
	int		i=0;
	//获取芯片ID，确定芯片类型
    spifp_rn8209 = spi_init(spifp_rn8209, ACS_SPI_DEV, 400000);
    //RN8209(spi max 1.2M) spi speed=400K
   	for(i=0;i<3;i++) {
   		device_id = rn_spi_read(spifp_rn8209, DeviceID);
   		fprintf(stderr,"device_id=%x\n",device_id);
   		if(device_id == RN8209_VER_ID)	{
   			K_vrms = ((INT32U)attCoef.UA[0])<<16;
   			K_vrms += ((INT32U)attCoef.UA[1])<<8;
   			K_vrms += attCoef.UA[2];
   			fprintf(stderr,"读取到的校表系数 = %d\n", K_vrms);
   			return device_id;
   		}
   		usleep(500);
   	}
   	//ATT7022E
   	spifp = spi_init(spifp,ACS_SPI_DEV,2000000);		//ATT7022E(spi max 10M) spi speed = 2M
   	for(i=0;i<3;i++) {
   		device_id = att_spi_read(spifp, r_ChipID, 3);
   		if(device_id != 0xffffff)	break;
   		sleep(1);
   	}
   	fprintf(stderr,"device_id=%x\n",device_id);
	if (device_id==0) {
		device_id = 1;				//ATT7022E 旧版ID=0,共享内存=1：ATT7022E
	}else 	if(device_id==0xffffff)  {
		syslog(LOG_NOTICE,"vacs 读id失败，默认=%x\n",ATT_VER_ID);
	}
	for(i=0;i<3;i++) {
		attCoef.WireType = read_check_wreg(spifp,w_FailVoltage);
		if((attCoef.WireType != 0xffffff) && (attCoef.WireType != 0)) break;
		sleep(1);
	}
	if((attCoef.WireType == 0xffffff)||(attCoef.WireType == 0)) {
		attCoef.WireType = 0x0600;		//接线方式，0x1200：三相三，0x0600：三相四
		syslog(LOG_NOTICE,"vacs 读接线方式失败，默认=%x(三相四)\n",attCoef.WireType);
	}
	check_reg_print(device_id);
	read_harmgain_cfg();
	if(device_id!=ATT_VER_ID) {
		read_tempgain_cfg();
		read_tempcoef_cfg();
	}
	chksum = write_coef_reg(spifp);
	oldchksum = chksum;
	return (device_id);
}


/*
 * 交采实时数据显示
 * */
void realdataprint(TS	nowts)
{
	static INT8U oldsec=0;

	if(oldsec == nowts.Sec) return;
	oldsec = nowts.Sec;
	dbg_prt("电压:	UA=%.1f, UB=%.1f, UC=%.1f",
			(float)realdata.Ua/U_COEF,(float)realdata.Ub/U_COEF,(float)realdata.Uc/U_COEF);
	dbg_prt("电流:	IA=%.3f, IB=%.3f, IC=%.3f, IL=%.3f",
			(float)realdata.Ia/I_COEF,(float)realdata.Ib/I_COEF,(float)realdata.Ic/I_COEF,(float)realdata.I0/I_COEF);
	dbg_prt("有功:	PT=%.1f, PA=%.1f, PB=%.1f, PC=%.1f",
			(float)realdata.Pt/P_COEF,(float)realdata.Pa/10,
			(float)realdata.Pb/P_COEF,(float)realdata.Pc/10);
	dbg_prt("无功:	QZ=%.1f, QA=%.1f, QB=%.1f, QC=%.1f",
			(float)realdata.Qt/Q_COEF,(float)realdata.Qa/Q_COEF,
			(float)realdata.Qb/Q_COEF,(float)realdata.Qc/Q_COEF);
	dbg_prt("视在:	SZ=%.1f, SA=%.1f, SB=%.1f, SC=%.1f",
			(float)realdata.St/S_COEF,(float)realdata.Sa/S_COEF,
			(float)realdata.Sb/S_COEF,(float)realdata.Sc/S_COEF);
	dbg_prt("功率因数:	Cos=%.3f, CosA=%.3f, CosB=%.3f, CosC=%.3f",
			(float)realdata.Cos/COS_COEF,(float)realdata.CosA/COS_COEF,
			(float)realdata.CosB/COS_COEF,(float)realdata.CosC/COS_COEF);
	dbg_prt("相角:	Pga=%.1f, Pgb=%.1f, Pgc=%.1f",
			(float)realdata.Pga/PHASE_COEF,(float)realdata.Pgb/PHASE_COEF,
			(float)realdata.Pgc/PHASE_COEF);
	dbg_prt("电压夹角:	YUaUb=%.1f, YUaUc=%.1f, YUbUc=%.1f",
			(float)realdata.YUaUb/ANGLE_COEF,(float)realdata.YUaUc/ANGLE_COEF,
			(float)realdata.YUbUc/ANGLE_COEF);
	dbg_prt("频率:Freq=%.2f",(float)realdata.Freq/FREQ_COEF);
	dbg_prt("1min平均电压:	UA=%.1f, UB=%.1f, UC=%.1f",
			(float)realdata.AvgUa/U_COEF,(float)realdata.AvgUb/U_COEF,
			(float)realdata.AvgUc/U_COEF);
	dbg_prt("相序标识:SFlag=%02x",realdata.SFlag);
	dbg_prt("温度:Temp=%d",realdata.Temp);
	dbg_prt("基波电压:	UA=%.1f, UB=%.1f, UC=%.1f",
			(float)realdata.LineUa/U_COEF,(float)realdata.LineUb/U_COEF,(float)realdata.LineUc/U_COEF);
	dbg_prt("基波电流:	IA=%.3f, IB=%.3f, IC=%.3f",
			(float)realdata.LineIa/I_COEF,(float)realdata.LineIb/I_COEF,(float)realdata.LineIc/I_COEF);
	dbg_prt("\n\r");
}
/*
 * 校表寄存器数据显示SAVE_NAME_ACDATA
 * */
void check_reg_print(INT32S device_id)
{
	if(device_id==ATT_VER_ID){
		dbg_prt("\n\r 大电流相角 校表系数");
		dbg_prt("PhaseA=%02x_%02x_%02x",attCoef.PhaseA0[0],attCoef.PhaseA0[1],attCoef.PhaseA0[2]);
		dbg_prt("PhaseB=%02x_%02x_%02x",attCoef.PhaseB0[0],attCoef.PhaseB0[1],attCoef.PhaseB0[2]);
		dbg_prt("PhaseC=%02x_%02x_%02x",attCoef.PhaseC0[0],attCoef.PhaseC0[1],attCoef.PhaseC0[2]);
	}else {
		dbg_prt("\n\r 相角 校表系数\n");
		dbg_prt("PhaseA=%02x_%02x_%02x",attCoef.PhaseA[0],attCoef.PhaseA[1],attCoef.PhaseA[2]);
		dbg_prt("PhaseB=%02x_%02x_%02x",attCoef.PhaseB[0],attCoef.PhaseB[1],attCoef.PhaseB[2]);
		dbg_prt("PhaseC=%02x_%02x_%02x",attCoef.PhaseC[0],attCoef.PhaseC[1],attCoef.PhaseC[2]);
		dbg_prt("\n\r 小电流相角 校表系数");
		dbg_prt("PhaseA=%02x_%02x_%02x",attCoef.PhaseA1[0],attCoef.PhaseA1[1],attCoef.PhaseA1[2]);
		dbg_prt("PhaseB=%02x_%02x_%02x",attCoef.PhaseB1[0],attCoef.PhaseB1[1],attCoef.PhaseB1[2]);
		dbg_prt("PhaseC=%02x_%02x_%02x",attCoef.PhaseC1[0],attCoef.PhaseC1[1],attCoef.PhaseC1[2]);
		dbg_prt("\n\r 电压 校表系数");
		dbg_prt("UA=%02x_%02x_%02x",attCoef.UA[0],attCoef.UA[1],attCoef.UA[2]);
		dbg_prt("UB=%02x_%02x_%02x",attCoef.UB[0],attCoef.UB[1],attCoef.UB[2]);
		dbg_prt("UC=%02x_%02x_%02x",attCoef.UC[0],attCoef.UC[1],attCoef.UC[2]);
		dbg_prt("\n\r 电流 校表系数");
		dbg_prt("IA=%02x_%02x_%02x",attCoef.IA[0],attCoef.IA[1],attCoef.IA[2]);
		dbg_prt("IB=%02x_%02x_%02x",attCoef.IB[0],attCoef.IB[1],attCoef.IB[2]);
		dbg_prt("IC=%02x_%02x_%02x",attCoef.IC[0],attCoef.IC[1],attCoef.IC[2]);
		dbg_prt("\n\r 功率 校表系数");
		dbg_prt("PA=%02x_%02x_%02x",attCoef.PA[0],attCoef.PA[1],attCoef.PA[2]);
		dbg_prt("PB=%02x_%02x_%02x",attCoef.PB[0],attCoef.PB[1],attCoef.PB[2]);
		dbg_prt("PC=%02x_%02x_%02x",attCoef.PC[0],attCoef.PC[1],attCoef.PC[2]);
		dbg_prt("\n\r 谐波电压系数");
		dbg_prt("Ua=%d, Ub=%d, Uc=%d",attCoef.HarmUCoef[0],attCoef.HarmUCoef[1],attCoef.HarmUCoef[2]);
		dbg_prt("\n\r 谐波电流系数");
		dbg_prt("Ia=%d, Ib=%d, Ic=%d",attCoef.HarmICoef[0],attCoef.HarmICoef[1],attCoef.HarmICoef[2]);
	}
}

void energysum_print(ACEnergy_Sum energysum_tmp)
{
	fprintf(stderr,"总电能示值：PosPt=%d, NegPt=%d, PosQt=%d, NegQt=%d\n",
			energysum_tmp.PosPt_All,energysum_tmp.NegPt_All,energysum_tmp.PosQt_All,energysum_tmp.NegQt_All);
	dbg_prt("总电能示值(4舍5入)：PosPt=%d, NegPt=%d, PosQt=%d, NegQt=%d\n",
			energysum.PosPt_All,energysum.NegPt_All,energysum.PosQt_All,energysum.NegQt_All);
	dbg_prt("正向有功:(4舍5入)	Pa=%d, Pb=%d, Pc=%d\n",
			energysum.PosPa_All,energysum.PosPb_All,energysum.PosPc_All);
	dbg_prt("正向有功:	Pa=%d, Pb=%d, Pc=%d\n",
			energysum_tmp.PosPa_All,energysum_tmp.PosPb_All,energysum_tmp.PosPc_All);
	dbg_prt("反向有功:	Pa=%d, Pb=%d, Pc=%d\n",
			energysum_tmp.NegPa_All,energysum_tmp.NegPb_All,energysum_tmp.NegPc_All);
	dbg_prt("正向无功:	Qa=%d, Qb=%d, Qc=%d\n",
			energysum_tmp.PosQa_All,energysum_tmp.PosQb_All,energysum_tmp.PosQc_All);
	dbg_prt("反向无功:	Qa=%d, Qb=%d, Qc=%d\n",
			energysum_tmp.NegQa_All,energysum_tmp.NegQb_All,energysum_tmp.NegQc_All);
	dbg_prt("正向有功费率: P1=%d, P2=%d, P3=%d, P4=%d\n",
			energysum_tmp.PosPt_Rate[0],energysum_tmp.PosPt_Rate[1],
			energysum_tmp.PosPt_Rate[2],energysum_tmp.PosPt_Rate[3]);
	dbg_prt("反向有功费率:	P1=%d, P2=%d, P3=%d, P4=%d\n\r",
			energysum_tmp.NegPt_Rate[0],energysum_tmp.NegPt_Rate[1],
			energysum_tmp.NegPt_Rate[2],energysum_tmp.NegPt_Rate[3]);
	dbg_prt("正向无功费率:	Q1=%d, Q2=%d, Q3=%d, Q4=%d\n\r",
			energysum_tmp.PosQt_Rate[0],energysum_tmp.PosQt_Rate[1],
			energysum_tmp.PosQt_Rate[2],energysum_tmp.PosQt_Rate[3]);
	dbg_prt("反向无功费率:	Q1=%d, Q2=%d, Q3=%d, Q4=%d\n\r",
			energysum.NegQt_Rate[0],energysum.NegQt_Rate[1],
			energysum.NegQt_Rate[2],energysum.NegQt_Rate[3]);
	dbg_prt("象限1：总及四费率1:Qt=%d,Q1=%d,Q2=%d,Q3=%d,Q4=%d\n",
			energysum_tmp.Q1_Qt_All,energysum_tmp.Q1_Qt_Rate[0],energysum_tmp.Q1_Qt_Rate[1],
			energysum_tmp.Q1_Qt_Rate[2],energysum_tmp.Q1_Qt_Rate[3]);
	dbg_prt("象限2：总及四费率2:Qt=%d,Q1=%d,Q2=%d,Q3=%d,Q4=%d\n",
			energysum_tmp.Q2_Qt_All,energysum_tmp.Q2_Qt_Rate[0],energysum_tmp.Q2_Qt_Rate[1],
			energysum_tmp.Q2_Qt_Rate[2],energysum_tmp.Q2_Qt_Rate[3]);
	dbg_prt("象限3：总及四费率3:Qt=%d,Q1=%d,Q2=%d,Q3=%d,Q4=%d\n",
			energysum_tmp.Q3_Qt_All,energysum_tmp.Q3_Qt_Rate[0],energysum_tmp.Q3_Qt_Rate[1],
			energysum_tmp.Q3_Qt_Rate[2],energysum_tmp.Q3_Qt_Rate[3]);
	dbg_prt("象限4：总及四费率4:Qt=%d,Q1=%d,Q2=%d,Q3=%d,Q4=%d\n",
			energysum_tmp.Q4_Qt_All,energysum_tmp.Q4_Qt_Rate[0],energysum_tmp.Q4_Qt_Rate[1],
			energysum_tmp.Q4_Qt_Rate[2],energysum_tmp.Q4_Qt_Rate[3]);
}

///////////////////////////////////////////////////////////////////////////////////////////////

//校验寄存器读出的数值是否正确
INT8S check_regvalue_rn8209(INT32S regvalue)
{
    INT32S RRec[128]; // RN8209计量参数寄存器数据

    RRec[SYS_RData >> 4] = rn_spi_read(spifp_rn8209, SYS_RData);
    //	dbg_prt( "校验寄存器值为： %d \n", RRec[SYS_RData>>4]);
    if (regvalue == RRec[SYS_RData >> 4]) {
        return 0;
    } else
        return -1;
}

//换算RN8209计量寄存器值
//输入参数：reg：计量寄存器值
//返回值    ：实时采样值
INT32S trans_regist_rn8209(INT32S reg) {
    INT32S tread = 0;
    if (K_vrms) {
        tread = ((FP64)reg * U_COEF) / K_vrms;
    }
    return tread;
}


void DealRN8209(void)
{
    INT32S RRec[128]; // RN8209计量参数寄存器数据
    INT32S val          = 0;
    static int time_old = 0;
    int time_now        = time(NULL);

    if (time_old == time_now) {
        return;
    }
    time_old = time_now;
    sem_wait(sem_check_fd);

    RRec[U_RMS >> 4] = rn_spi_read(spifp_rn8209, U_RMS); //电压通道有效值
    if ((check_regvalue_rn8209(RRec[U_RMS >> 4]) == 0)) {
        val = RRec[U_RMS >> 4];
    }
    sem_post(sem_check_fd);
    realdata.Ua = (realdata.Ua + (trans_regist_rn8209(val))) / 2; //转换，获取电压当前值
    fprintf(stderr, "当前电压值为： %d %d\n", realdata.Ua,trans_regist_rn8209(val));
}
/////////////////////////////////////////////////////////////

/*
 * 初始化交采相关参数、电能量读取、系数读取
 * */
void InitACSPara()
{
	int			val=0;

	//信号量建立
	sem_check_fd = create_named_sem(SEMNAME_SPI0_0,1);							//TODO:放入vmain
	sem_getvalue(sem_check_fd, &val);
	dbg_prt("process The sem is %d\n", val);

	InitACSCoef();			//读交采数据
	InitACSEnergy();		//电能量初值
	device_id = InitACSChip();		//初始化芯片类型
	JProgramInfo->ac_chip_type = device_id;
	JProgramInfo->WireType = attCoef.WireType;
	fprintf(stderr,"计量芯片版本：%06X, 接线方式=%X(0600:三相四，1200：三相三)\n",JProgramInfo->ac_chip_type,JProgramInfo->WireType);
	syslog(LOG_NOTICE,"计量芯片版本：%06X, 接线方式=%X(0600:三相四，1200：三相三)\n",JProgramInfo->ac_chip_type,JProgramInfo->WireType);
}

void DealATT7022(void)
{
	TS	nowts,oldts;
	INT32S	tpsd_reg;//,tempval=0;

	TSGet(&nowts);
	TSGet(&oldts);
	tpsd_reg = read_regist(spifp,device_flag);
	read_engergy_regist(nowts,spifp);		//1分钟采集一次电能量
//	read_ware_regist(nowts,spifp,vdAskHarmFlag);//1分钟或vd进程请求采集一次谐波采样值
//	tempval = write_allgain_reg(spifp,tpsd_reg);//根据温度寄存器值进行AllGain寄存器增益补偿
	if((chksum != oldchksum) || (chksum1 != oldchksum1))  {//校验和有变化，重新写计量参数寄存器
		sleep(2);
		// 读ATT7022E芯片的校表系数
		readCoverClass(0,0,&attCoef,sizeof(ACCoe_SAVE),acs_coef_save);
		chksum = write_coef_reg(spifp);
		fprintf(stderr,"ChkSum(%d) old(%d) ChkSum1(%d) old1(%d) have change,rewrite_coef_reg\n",chksum,oldchksum,chksum1,oldchksum1);
		syslog(LOG_NOTICE,"ChkSum(%d) old(%d) ChkSum1(%d) old1(%d) have change,rewrite_coef_reg\n",chksum,oldchksum,chksum1,oldchksum1);
		oldchksum = chksum;
	}
	calc_minute_ave_u(nowts,realdata.Ua,realdata.Ub,realdata.Uc,&realdata.AvgUa,&realdata.AvgUb,&realdata.AvgUc);
	//暂时去掉谐波处理过程。
	//ware_process(nowts,&vdAskHarmFlag);				//谐波值计算及统计过程
	realdataprint(nowts);
}

/*
 * 5分钟保存一次电量值
 * 断电情况下保存电量
 * */
void ACSEnergySave(ACEnergy_Sum energysum_tmp)
{
	TS	ts={};
	static INT8U oldmin=0;
	INT8S  saveflag = 0;
	FP32 bett[2]={};

	TSGet(&ts);
	if(ts.Minute%5==0 && ts.Minute!=oldmin) {
		oldmin = ts.Minute;
		if(pwr_has() == TRUE) {
			saveflag = 1;
		}
	}
	if(pwr_has() == FALSE) {
		sleep(2);
		if(bettery_getV(&bett[0],&bett[1]) == TRUE) {
			if(bett[1] >= MIN_BATTWORK_VOL) {
				saveflag = 2;
				syslog(LOG_NOTICE,"底板电源已关闭，电池电压=%f V,保存电能示值",bett[1]);
			}else {
				syslog(LOG_NOTICE,"底板电源已关闭，电池电压过低=%f V,不保存电量！！！",bett[1]);
			}
		}
	}
	if(saveflag) {
		saveCoverClass(0,0,&energysum_tmp,sizeof(ACEnergy_Sum),acs_energy_save);
	}
}
/*
 * 交采计量芯片的处理过程
 * */
void DealACS()
{
	switch(device_id) {
	case RN8209_VER_ID:
		DealRN8209();
		break;
	case 1:
	case ATT_VER_ID:
		DealATT7022();		//处理ATT7022E数据
		ACSEnergySave(energysum);	//电量的存储	//TODO :底板掉电情况下，保证不控制gprs的poweron/off管脚
		break;
	}
	//拷贝实时数据和电能量数据到pubdata共享内存结构体中。为了液晶的轮显数据
	memcpy(&JProgramInfo->ACSRealData,&realdata,sizeof(_RealData));
	memcpy(&JProgramInfo->ACSEnergy,&energysum,sizeof(ACEnergy_Sum));
}

