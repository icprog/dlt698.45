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

INT8S gpio_readbyte(char* devpath) {
    char data = 0;
    int fd    = -1;
    if ((fd = open((const char*)devpath, O_RDWR | O_NDELAY)) >=0) {
        read(fd, &data, sizeof(char));
        close(fd);
    } else {
    	syslog(LOG_ERR,"%s %s fd=%d\n",__func__,devpath,fd);
    	return -1;
    }
    return data;
}

INT32S gpio_readint(char* devpath) {
    char data = 0;
    int fd    = -1;
    if ((fd = open((const char*)devpath, O_RDWR | O_NDELAY)) >= 0) {
        read(fd, &data, sizeof(INT32S));
        close(fd);
    } else {
       	syslog(LOG_ERR,"%s %s fd=%d\n",__func__,devpath,fd);
    	return -1;
    }
    return data;
}

INT32S gpio_writebyte(char* devpath, INT8S data) {

    int fd = -1;
    if ((fd = open((const char*)devpath, O_RDWR | O_NDELAY)) >= 0) {
        write(fd, &data, sizeof(char));
        close(fd);
        return 1;
    } else {
       	syslog(LOG_ERR,"%s %s fd=%d\n",__func__,devpath,fd);
        return -1;
    }
    return 0;
}

INT32S gpio_writebytes(char* devpath, INT8S* vals, INT32S valnum) {
    int fd = -1;
    int i  = 0;
    fd     = open((const char*)devpath, O_RDWR | O_NDELAY);
    if (fd <= 0) {
       	syslog(LOG_ERR,"%s %s fd=%d\n",__func__,devpath,fd);
    	return -1;
    }
    for (i = 0; i < valnum; i++) {
        write(fd, &vals[i], sizeof(char));
        delay(10);
    }
//    delay(1000);
    close(fd);
    return 0;
}

//二型集中器没有电池只有电容，所以不能够读出底板是否有电，且二型集中器只有一相电压，停上电事件在硬件复位时不能产生，
//所以判断时，需要判断当前电压大于一个定值且小时参数时，产生事件(大于的定时暂定为10v交采已经将实时电压值乘以１０).
BOOLEAN pwr_has_byVolt(INT8U valid, INT32U volt, INT16U limit) {
    if ((valid == TRUE) && ((volt > 100 && volt < limit) || (volt < 30))) {
        return FALSE;
    }
    return TRUE; //上电
}

/*
 * 底板电源是否有电
 */
BOOLEAN pwr_has()
{
    INT32U state = 1;
    int fd       = -1;

    fd = open(DEV_MAINPOWER, O_RDWR | O_NDELAY);
    if (fd >= 0) {
        read(fd, &state, 1);
        close(fd);
    }else {
    	syslog(LOG_ERR,"%s %s fd=%d\n",__func__,DEV_MAINPOWER,fd);
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
BOOLEAN bettery_getV(FP32* clock_bt, FP32* tmnl_bt) {
    int fd                     = -1;
    unsigned int adc_result[2] = {};

    adc_result[0] = 0;
    adc_result[1] = 0;
    if ((fd = open(DEV_ADC0, O_RDWR | O_SYNC)) == -1) {
       	syslog(LOG_ERR,"%s %s fd=%d\n",__func__,DEV_ADC0,fd);
       	*clock_bt = 0;
       	*tmnl_bt = 0;
    	return FALSE;
    }
    gpio_writebyte(DEV_ADC_SWITCH, 0);
    sleep(1);
    read(fd, adc_result, 2 * sizeof(unsigned int));
    gpio_writebyte(DEV_ADC_SWITCH, 1);
    *clock_bt = adc_result[0] * 1.0 / 1023 * 6.6;
    *tmnl_bt  = adc_result[1] * 1.0 / 1023 * 6.6;
    close(fd);
    return TRUE;
}

BOOLEAN bettery_getV_II(FP32* clock_bt)
{
    int fd                     = -1;
    unsigned int adc_result = 0;
    if ((fd = open(DEV_ADC, O_RDWR | O_SYNC)) == -1) {
       	syslog(LOG_ERR,"%s %s fd=%d\n",__func__,DEV_ADC,fd);
       	*clock_bt = 0;
    	return FALSE;
    }
    read(fd, &adc_result, sizeof(unsigned int));
    *clock_bt = adc_result * 1.0 / 1023 * 6.6;
    close(fd);
    return TRUE;
}
