/*
 * gpio.c
 *
 *  Created on: 2017-1-6
 *      Author: gk
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>

#include "PublicFunction.h"
#include "StdDataType.h"
#include "ParaDef.h"
#include "att7022e.h"
#include "basedef.h"

char gpio_readbyte(char* devpath)
{
    char data = 0;
    int fd    = -1;
    if ((fd = open((const char*)devpath, O_RDWR | O_NDELAY)) >=0) {
        read(fd, &data, sizeof(char));
        close(fd);
    } else {
    	DEBUG_TIME_LINE("open %s failed", devpath);
    	return -1;
    }
    return data;
}

char gpio_readint(char* devpath, int* pDdata)
{
	int fd = -1;

	if(pDdata == NULL)
		return -1;

	*pDdata = 0;
	if ((fd = open((const char*) devpath, O_RDWR | O_NDELAY)) >= 0) {
		read(fd, pDdata, sizeof(int));
		close(fd);
	} else {
		DEBUG_TIME_LINE("open %s failed", devpath);
		return -1;
	}
	return 1;
}

int gpio_writebyte(char* devpath, INT8S data)
{
	int fd = -1;
	if ((fd = open((const char*) devpath, O_RDWR | O_NDELAY)) >= 0) {
		write(fd, &data, sizeof(char));
		close(fd);
		return 1;
	} else {
		DEBUG_TIME_LINE("open %s failed", devpath);
		return -1;
	}
	return 0;
}

int gpio_writebytes(char* devpath, char* vals, int valnum)
{
	int fd = -1;
	int i = 0;
	fd = open((const char*) devpath, O_RDWR | O_NDELAY);
	if (fd <= 0) {
		DEBUG_TIME_LINE("open %s failed", devpath);
		return -1;
	}
	for (i = 0; i < valnum; i++) {
		write(fd, &vals[i], sizeof(char));
		delay(10);
	}
	close(fd);
	return 0;
}

/*
 * 二型集中器没有电池只有电容，所以不能够读出底板是否有电，且二型集中器只有一相电压，停上电事件在硬件复位时不能产生，
 * 所以判断时，需要判断当前电压大于一个定值且小时参数时，产生事件(大于的定时暂定为100v交采已经将实时电压值乘以１０).
 * 正常的顺序: 0< VOL_DOWN_THR < limit < VOL_ON_THR
 */
INT8U pwr_down_byVolt(INT8U valid, INT32U volt, INT16U limit)
{
	if (0 == limit)
		limit = VOL_DOWN_THR;

	if ((TRUE == valid) && (volt < limit)) {
		syslog(LOG_NOTICE, "volt=%d valid=%d limit=%d\n", volt, valid, limit);
		return 1; //断电
	}
	return 0; //未断电
}

INT8U pwr_on_byVolt(INT8U valid, INT32U volt, INT16U limit)
{
	if (0 == limit)
		limit = VOL_ON_THR;

	if ((valid == TRUE) && (volt > limit))
		return 1; //上电

	return 0; //未上电
}

/*
 * 底板电源是否有电
 * 返回 TRUE: 有电   FALSE:掉电
 */
BOOLEAN pwr_has()
{
	INT32U state = 1;
	int fd = -1;

	fd = open(DEV_MAINPOWER, O_RDWR | O_NDELAY);
	if (fd >= 0) {
		read(fd, &state, 1);
		close(fd);
	} else {
		syslog(LOG_ERR, "%s %s fd=%d\n", __func__, DEV_MAINPOWER, fd);
	}
	if ((state & 0x01) == 1) {
		return TRUE;
	} else {
		return FALSE;
	}
}

/*
 * 读取 dev/adc0 设备两个（unsigned int）类型的数据。
 * 第一个数据clock_bt 保存的时钟电池电压（与原/dev/adc设备读取值一致），
 * 第二个数据tmnl_bt 保存的设备电池电压。
 *    将 /dev/gpioADC_SWITCH=0（终端工作放电模式）
 *    将 /dev/gpioADC_SWITCH=1（终端工作充电模式）
 *
 */
BOOLEAN bettery_getV(FP32* clock_bt, FP32* tmnl_bt)
{
	int fd = -1;
	unsigned int adc_result[2] = { };

	adc_result[0] = 0;
	adc_result[1] = 0;
	if ((fd = open(DEV_ADC0, O_RDWR | O_SYNC)) == -1) {
		syslog(LOG_ERR, "%s %s fd=%d\n", __func__, DEV_ADC0, fd);
		*clock_bt = 0;
		*tmnl_bt = 0;
		return FALSE;
	}
	gpio_writebyte(DEV_ADC_SWITCH, 0);
	sleep(1);
	read(fd, adc_result, 2 * sizeof(unsigned int));
	gpio_writebyte(DEV_ADC_SWITCH, 1);
	*clock_bt = adc_result[0] * 1.0 / 1023 * 6.6;
	*tmnl_bt = adc_result[1] * 1.0 / 1023 * 6.6;
	close(fd);
	return TRUE;
}

BOOLEAN bettery_getV_II(FP32* clock_bt)
{
	int fd = -1;
	unsigned int adc_result = 0;
	if ((fd = open(DEV_ADC, O_RDWR | O_SYNC)) == -1) {
		syslog(LOG_ERR, "%s %s fd=%d\n", __func__, DEV_ADC, fd);
		*clock_bt = 0;
		return FALSE;
	}
	read(fd, &adc_result, sizeof(unsigned int));
	*clock_bt = adc_result * 1.0 / 1023 * 6.6;
	close(fd);
	return TRUE;
}

/////////////////////////////////////////////////////
/*
 * 并转串时钟输出74HC165
 * 正常返回  模拟状态 ，低5位为GPRS_ID, 第6位门节点状态
 * =-1：		无此设备，为II型集中器
 * */
INT8S getSpiAnalogState()
{
	unsigned char ret = 0;
	int i = 0;
	char  tmpid[8] = {0};

	if (gpio_writebyte(DEV_SPI_CS, 1) == -1) {
		return -1;
	}
	usleep(50);
	gpio_writebyte(DEV_SPI_CS, 0);
	usleep(50);
	gpio_writebyte(DEV_SPI_CS, 1);
	gpio_writebyte(DEV_SPI_CLK, 0);
	for (i = 0; i < 8; i++) {
		usleep(50);
		gpio_writebyte(DEV_SPI_CLK, 1);
		usleep(50);
		tmpid[i] = gpio_readbyte(DEV_SPI_MISO);
		usleep(50);
		gpio_writebyte(DEV_SPI_CLK, 0);
	}
	if (tmpid[6] == 1) // GPRS_STAT0
		ret |= 1 << 0;
	if (tmpid[4] == 1) // GPRS_STAT1
		ret |= 1 << 1;
	if (tmpid[5] == 1) // GPRS_STAT2
		ret |= 1 << 2;
	if (tmpid[1] == 1) // GPRS_STAT3
		ret |= 1 << 3;
	if (tmpid[3] == 1) // GPRS_STAT4
		ret |= 1 << 4;

	if (tmpid[2] == 1) // MEN node  门节点
		ret |= 1 << 5;
	return ret;
}
