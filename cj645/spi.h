/*
 * SPI.h
 *
 *  Created on: 2013-3-6
 *      Author: Administrator
 */

#ifndef SPI_H_
#define SPI_H_

#include <semaphore.h>
#include <stdint.h>
#include "StdDataType.h"


//错误列表
#define  ERR_WRTBUF_OVERLEN   -1 		//写入缓冲长度过长
#define  ERR_WAIT_TIMEOUT     -2 		//数据等待超时
//
#define  BUFFLENMAX_SPI_ACS       128

/* 打开初始化SPI设备
 * spipath：spi设备全路径
 * 返回：     spi设备句柄
 * */
extern INT32S spi_init(INT32S fd,const char * spipath,uint32_t speed);

/*关闭SPI设备
 * */
extern INT32S spi_close(INT32S fd);

/*ATT7022E_spi 口读操作
 * 输入参数：spifd：设备句柄，spi:spi设备名，addr:读寄存器地址,len:读取长度,
 * 返回：寄存器值
 */
extern INT32S att_spi_read(int spifp,INT32U addr, INT32U len);

/*ATT7022E_spi 口写操作
 * 输入参数：spifd：设备句柄，addr:读寄存器地址,len:读取长度,buf：寄存器值
 * 返回：spi写长度
 */
extern INT32S att_spi_write(int spifp, INT32U addr, INT32U len, INT8U *buf);

/* spi  读操作
 * 输入参数：fd：设备句柄，cbuf:写值，clen：写长度，rbuf：读值,rlen:读长度
 * 返回：寄存器值
 */
extern int spi_read(int fd,INT8U *cbuf, int16_t clen, INT8U *rbuf, int rlen);

/* spi  写操作
 * 输入参数：fd：设备句柄，buf:写值，len：写长度
 * 返回：寄存器值
 */
extern int spi_write(int fd, INT8U *buf, int len);

/*RN8209_spi 口读操作
 * 输入参数：spifd：设备句柄，spi:spi设备名，addr:读寄存器地址,
 * 返回：寄存器值
 */
INT32S rn_spi_read(int spifp, INT32U addr);

/*RN8209_spi 口写操作
 * 输入参数：spifd：设备句柄，addr:读寄存器地址,buf：寄存器值
 * 返回：spi写长度
 */
INT32S rn_spi_write(int spifp, INT32U addr, INT8U* buf);

#endif /* SPI_H_ */
