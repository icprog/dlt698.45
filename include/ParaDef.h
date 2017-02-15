/*
 * ParaDef.h
 *
 *  Created on: Jan 6, 2017
 *      Author: lhl
 */

#ifndef PARADEF_H_
#define PARADEF_H_

#define delay(A) usleep((A)*1000)
//////////////////////////////////////////////////////////////
#define _CFGDIR_ 			"/nor/config"
#define _ACSDIR_			"/nand/acs"
///////////////////////////////////////////////////////////////
/*
 * 	进程间通讯相关限值
 * */

#define ARGVMAXLEN			50					//参数最大长度
#define PRONAMEMAXLEN		50					//进程名称最大长度
#define	PROJECTCOUNT		10					//守护进程可以支持的最多进程数
#define ARGCMAX				4					//支持进程参数最大数
#define FRAMELEN 			2048
#define BUFLEN  			2048						//上行通道发送接收数组长度
#define REALDATA_LIST_LENGTH 	10				//实时数据请求缓存
#define PRO_WAIT_COUNT     		60

///////////////////////////////////////////////////////////////
/*
 * 	终端类相关容量及参数定义
 * */
#define MAX_POINT_NUM 			1200
#define MIN_BATTERY_VOL 		2.2  		//电池失压判断数值
#define MIN_BATTWORK_VOL 		3.3  		//电池电压工作最小数值，掉电情况下低于该值，不进行数据存储

///////////////////////////////////////////////////////////////
/*
 * 	DL/T698.45		规约结构限值
 * */
#define TSA_LEN					17
#define OCTET_STRING_LEN		16
#define COLLCLASS_MAXNUM		1024		//定义集合类最大元素个数

#define CLASS7_OAD_NUM			10			//关联对象属性表
#define MAX_PERIOD_RATE   		48      	//支持的最到终端费率时段数

#define	STATE_MAXNUM			8			//开关量单元最大个数

#define EVENT_OI_MAXNUM			30			//终端需要判断事件
////////////////////////////////////////////////////////////////

/*
 * 	GPIO硬件接口
 * */

//TODO:根据交采芯片决定ESAM打开那个设备，不用CCTT_II区分
#ifdef CCTT_II
 #define DEV_SPI_PATH   "/dev/spidev1.0"
#else
  #define DEV_SPI_PATH   "/dev/spi0.0"
#endif

#define	ACS_SPI_DEV		"/dev/spidev1.0"		//计量芯片使用的spi设备

//Esam与ATT7022E共用数据线,复位信号，各自独立片选，CS=0，可读写，
//因此不能同时读写ESAM与ATT7022E，必须互斥操作。

#define DEV_ESAM_RST   	"/dev/gpoESAM_RST"
#define DEV_ESAM_CS    	"/dev/gpoESAM_CS"
#define DEV_ESAM_PWR   	"/dev/gpoESAM_PWR"

#define	DEV_ATT_RST		"/dev/gpoATT_RST"
#define	DEV_ATT_CS		"/dev/gpoATT_CS"

//II型RN8209控制gpio，目前程序中未用
#define DEV_RN_RST 		"/dev/gpo8209_RST"
#define DEV_RN_CS 		"/dev/gpo8209_CS"
////////////////////////////////////////////////////////////////

#define DEV_BAT_SWITCH "/dev/gpoBAT_SWITCH"		//=1，电池工作
#define DEV_MAINPOWER  "/dev/gpiV5FROUNT_TST"  //底板电源：1上电0失电
#define DEV_ADC_SWITCH "/dev/gpioADC_SWITCH"   //=0（终端工作放电模式）=1（终端工作充电模式）

//I型集中器并转串模拟输出: GPRS_ID状态 与 1路门节点
#define DEV_SPI_CS		"/dev/gpoSPI_CS"		//并转串(74HC165)芯片选择
#define	DEV_SPI_CLK		"/dev/gpoSPI_CLK"		//并转串(74HC165)时钟输出
#define	DEV_SPI_MISO	"/dev/gpiSPI_MISO"		//并转串(74HC165)数据输入

//II型集中器GPRS状态
#define DEV_GPRS_S0		"/dev/gpiGPRS_S0"		//GPRS状态
#define DEV_GPRS_S1		"/dev/gpiGPRS_S1"		//GPRS状态
#define DEV_GPRS_S2		"/dev/gpiGPRS_S2"		//GPRS状态

#define DEV_STATE1  	"/dev/gpiYX1"
#define DEV_STATE2  	"/dev/gpiYX2"
#define DEV_STATE3  	"/dev/gpiYX3"
#define DEV_STATE4  	"/dev/gpiYX4"
#define DEV_PULSE  		"/dev/pulse"  			//脉冲1

#define DEV_ADC        "/dev/adc0"
/////////////////////////////////////////////////////////////////

/*
 * 	互斥信号量
 * */

#define SEMNAME_SPI0_0 		"sem_spi0_0" //专变、I型集中器交采和esam的spi通信互斥信号量
#define	SEMNAME_PARA_SAVE	"sem_parasave"			//参数文件存储
////////////////////////////////////////////////////////////////

/*
 * 	交采计量
 * */
#define MAXVAL_RATENUM			4		//支持的最大费率数
#define MAXVAL_HARMONICWAVE     19       //支持的谐波检测最高谐波次数

////////////////////////////////////////////////////////////////
/*
 * 	串口定义
 * */
#define S4851   1
#define S4852   2
#define S4853   3

////////////////////////////////////////////////////////////////
#endif /* PARADEF_H_ */
