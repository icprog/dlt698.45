/*
 * lcd_led.h
 *
 *  Created on: 2013-9-17
 *      Author: yd
 */

#ifndef LCD_LED_H_
#define LCD_LED_H_
#include "comm.h"
//#define SPI_GPIO_IOC_MAGIC  'S'
//#define SPI_GPIO_IOCRESET    _IO(SPI_GPIO_IOC_MAGIC, 0)
//#define SPI_GPIO_IOC_SET_OUTPUT 	_IOW(SPI_GPIO_IOC_MAGIC,  1,int)
//#define SPI_GPIO_IOC_CLR_OUTPUT  	_IOW(SPI_GPIO_IOC_MAGIC,  2,int)


#define LED_RUN 	"/dev/gpoRUN_LED"
#define LED_ALARM 	"/dev/gpoALARM"

#define LED_ON 	1
#define LED_OFF 0
extern void SpiLed(int led, char state);
#endif /* LCD_LED_H_ */
