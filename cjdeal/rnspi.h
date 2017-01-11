/*
 * SPI.h
 *
 *  Created on: 2013-3-6
 *      Author: Administrator
 */

#ifndef RNSPI_H_
#define RNSPI_H_

#include "StdDataType.h"

//#define	DEV_RN_RST				"/dev/gpoJC_RST"
#define DEV_RN_RST "gpo8209_RST"
#define DEV_RN_IRQ "/dev/gpoJC_IRQ"

//错误列表
#define ERR_WRTBUF_OVERLEN -1 //写入缓冲长度过长
#define ERR_WAIT_TIMEOUT -2   //数据等待超时
//
#define BUFFLENMAX_SPI 128

INT32S spifp_rn8209; // RN8209打开spi句柄
/* 打开初始化SPI设备
 * spipath：spi设备全路径
 * 返回：     spi设备句柄
 * */
extern INT32S spi_init_r(INT32S fd, const char* spipath);

/*关闭SPI设备
 * */
extern INT32S spi_close_r(INT32S fd);

/*RN8209_spi 口读操作
 * 输入参数：spifd：设备句柄，spi:spi设备名，addr:读寄存器地址,
 * 返回：寄存器值
 */
extern INT32S rn_spi_read(int spifp, INT32U addr);

/*RN8209_spi 口写操作
 * 输入参数：spifd：设备句柄，addr:读寄存器地址,buf：寄存器值
 * 返回：spi写长度
 */
extern INT32S rn_spi_write(int spifp, INT32U addr, INT8U* buf);

/* spi  读操作
 * 输入参数：fd：设备句柄，cbuf:写值，clen：写长度，rbuf：读值,rlen:读长度
 * 返回：寄存器值
 */
extern int spi_read_r(int fd, INT8U* cbuf, int16_t clen, INT8U* rbuf, int rlen);

/* spi  写操作
 * 输入参数：fd：设备句柄，buf:写值，len：写长度
 * 返回：寄存器值
 */
extern int spi_write_r(int fd, INT8U* buf, int len);

/* 校验寄存器读出的数值是否正确
 * 输入参数：regvalue 寄存器的值，记录上次spi读出的数据
 * 返回：本次数据与上次数据相同，返回 0；失败返回 -1
 */
extern INT8S check_regvalue_rn8209(INT32S regvalue);

/* 换算RN8209计量寄存器值
 * 输入参数：reg：计量寄存器值
 * 返回值：实时电压采样值
 */
extern INT32S trans_regist_rn8209(INT32S reg);

/* 获取芯片ID，确定是否是RN8209
 * 输入参数：无
 * 输出参数：如果芯片是RN8209，返回 1；否则返回 0
 */
extern INT8S check_id_rn8209(void);

/* 初始化RN8209运行环境
 * 输入参数：进程ID
 * 输出参数：无
 */
extern void init_run_env_rn8209(INT32S pid);

#endif /* RNSPI_H_ */
