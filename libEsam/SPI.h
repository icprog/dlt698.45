/*
 * SPI.h
 *
 *  Created on: 2013-3-6
 *      Author: Administrator
 */

#ifndef SPI_H_
#define SPI_H_

#include <sys/types.h>
#include <stdint.h>

//#define DEBUG


#define ERR_WRTBUF_OVERLEN -1 //写入缓冲长度过长
#define ERR_WAIT_TIMEOUT -2   //数据等待超时

#define BUFFLENMAX_SPI 2048
#define BUFFLENMAX_SPI_ESAM 2048

extern int32_t SPI_Init(int32_t fd, u_char* spipath);
extern int32_t SPI_Close(int fd);

#endif /* SPI_H_ */
