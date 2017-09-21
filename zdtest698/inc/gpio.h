#ifndef	GPIO_H
#define	GPIO_H

/*
 * gpio操作函数
 */
extern char gpio_readbyte(char* devpath);
extern char gpio_readint(char* devpath, int* data);
extern int gpio_writebyte(char* devpath, char data);
extern int gpio_writebytes(char* devpath, char* vals, int valnum);

extern BOOLEAN pwr_has();
extern INT8U pwr_down_byVolt(INT8U valid, INT32U volt, INT16U limit);
extern INT8U pwr_on_byVolt(INT8U valid, INT32U volt, INT16U limit);
extern BOOLEAN bettery_getV(FP32* clock_bt, FP32* tmnl_bt);
extern BOOLEAN bettery_getV_II(FP32* clock_bt);
extern INT8S getSpiAnalogState();

#endif
