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
#define _CFGDIR_ 				"/nor/config"
#define _ACSDIR_				"/nor/acs"
#define TASK_FRAME_DATA			"/nand/frmdata"		//任务分帧的数据文件
#define PARA_FRAME_DATA			"/nand/frmpara"		//参数类分帧的数据文件
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


#define MMQNAMEMAXLEN    	32		//消息队列名称长度
#define MAXSIZ_PROXY_485    2048
#define MAXNUM_PROXY_485    25
#define MAXSIZ_PROXY_NET    2048
#define MAXNUM_PROXY_NET    25
#define MAXSIZ_TASKID_QUEUE    256
#define MAXNUM_TASKID_QUEUE    25
#define MAXNUM_AUTOTASK		20					//采用上报方案的任务最大数

#define MAXSIZ_FAM	1600						//
///////////////////////////////////////////////////////////////
/*
 * 	终端类相关容量及参数定义
 * */
#define MAX_POINT_NUM 			1200
#define MAXNUM_IMPORTANTUSR     21 		    // 重点用户最大个数,20个重点用户+１交采
#define MAXNUM_SUMGROUP			8		    //总加组配置数量
#define MIN_BATTERY_VOL 		2.2  		//电池失压判断数值
#define MIN_BATTWORK_VOL 		3.3  		//电池电压工作最小数值，掉电情况下低于该值，不进行数据存储
#define MAXVAL_RATENUM			4	     	//支持的最大费率数

///////////////////////////////////////////////////////////////
/*
 * 	DL/T698.45		规约结构限值
 * */

#define MAX_APDU_SIZE			1024		//协商的APDU的最大尺寸
#define FRAME_SIZE				900			//分帧的长度，分帧组帧未考虑到链路层的数据长度，此处小于MAX_APDU_SIZE
#define MET_RATE                4           //电表费率应该根据电表配置来，此处调试使用
#define TSA_LEN					17
#define OCTET_STRING_LEN		16
#define VISIBLE_STRING_LEN		40
#define COLLCLASS_MAXNUM		1024		//定义集合类最大元素个数

#define	REPORT_CHANN_OAD_NUM	10			//上报方案 array OAD最大个数
#define MY_CSD_NUM				20			//my_csd数组最大各数
#define ROAD_OADS_NUM           16          //ROAD结构体里oads的最大个数
#define CLASS7_OAD_NUM			10			//关联对象属性表
#define MAX_PERIOD_RATE   		48      	//支持的最到终端费率时段数

#define	STATE_MAXNUM			8			//开关量单元最大个数

#define EVENT_OI_MAXNUM			30			//终端需要判断事件

#define	VARI_LEN				64			//变量类对象每个oi占用的位置空间
//一个CSD 对应07数据项个数 数组
#define DI07_NUM_601F 			10

#define POWEROFFON_NUM          10          //停上电事件需要抄读得测量点最大数

////////////////////////////////////////////////////////////////

/*
 * 	GPIO硬件接口
 * */

//TODO:根据交采芯片决定ESAM打开那个设备，不用CCTT_II区分
#define ESAM_SPI_DEV_II		   "/dev/spi1.0"
#define ESAM_SPI_DEV 			"/dev/spi0.0"

#define	ACS_SPI_DEV		"/dev/spi0.0"				//计量芯片使用的spi设备

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

#define DEV_WATCHDOG   			"/dev/watchdog"
#define DEV_LED_ALARM   		"/dev/gpoALARM"
#define DEV_LED_RUN     		"/dev/gpoRUN_LED"
#define DEV_LED_ONLINE  		"/dev/gpoONLINE_LED"
#define DEV_LED_CSQ_RED 		"/dev/gpoCSQ_RED"
#define DEV_LED_CSQ_GREEN 		"/dev/gpoCSQ_GREEN"
#define DEV_LED_RMT_RED      	"/dev/gpoREMOTE_RED"
#define DEV_LED_RMT_GRN   		"/dev/gpoREMOTE_GREEN"

////////*********************************************************
//消息
#define PROXY_485_MQ_NAME			"/proxy_485_mq"		//485抄表过程接收代理请求消息队列
#define PROXY_NET_MQ_NAME			"/proxy_net_mq"		//通讯进程代理应答接收消息队列

#define TASKID_485_1_MQ_NAME		"/taskid_485_1_mq"		//485 1抄表 任务ID 队列
#define	TASKID_485_2_MQ_NAME		"/taskid_485_2_mq"		//485 2抄表 任务ID 队列
#define TASKID_plc_MQ_NAME			"/taskid_plc_mq"		//载波 抄表 任务ID 队列
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
