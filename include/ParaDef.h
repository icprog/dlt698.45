/*
 * ParaDef.h
 *
 *  Created on: Jan 6, 2017
 *      Author: lhl
 */

#ifndef PARADEF_H_
#define PARADEF_H_


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

#define _CFGDIR_ 			"/nor/config"
///////////////////////////////////////////////////////////////
/*
 * 	终端类相关容量及参数定义
 * */
#define MAX_POINT_NUM 			1200

///////////////////////////////////////////////////////////////
/*
 * 	DL/T698.45		规约结构限值
 * */
#define TSA_LEN					17
#define OCTET_STRING_LEN		16
#define COLLCLASS_MAXNUM		1024		//定义集合类最大元素个数

////////////////////////////////////////////////////////////////

/*
 * 	GPIO硬件接口
 * */
#ifdef CCTT_II
 #define DEV_SPI_PATH   "/dev/spi1.0"
#else
  #define DEV_SPI_PATH   "/dev/spi0.0"
#endif
#define DEV_ESAM_RST   "/dev/gpoESAM_RST"
#define DEV_ESAM_CS    "/dev/gpoESAM_CS"
#define DEV_ESAM_PWR   "/dev/gpoESAM_PWR"
#define DEV_ATT_RST    "/dev/gpoATT_RST"

////////////////////////////////////////////////////////////////
#define delay(A) usleep((A)*1000)

/*
 * 	互斥信号量
 * */

#define SEMNAME_SPI0_0 "sem_spi0_0" //专变、I型集中器交采和esam的spi通信互斥信号量

////////////////////////////////////////////////////////////////
/*
 * 	串口定义
 * */
#define S4851   1
#define S4852   2
#define S4853   3

////////////////////////////////////////////////////////////////
#endif /* PARADEF_H_ */
