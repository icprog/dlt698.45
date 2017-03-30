#ifndef LCM_H_
#define LCM_H_
#include "comm.h"
/*
 * Ioctl definitions
 */

/* Use 'L' as magic number, means LCD */
#define LCD_IOC_MAGIC  'L'
/* Please use a different 8-bit number in your code */

#define LCD_IOCRESET    		_IO(LCD_IOC_MAGIC, 0)

#define LCD_IOC_UPDATE 			_IOW(LCD_IOC_MAGIC,  1,int)
#define LCD_IOC_BACKLIGHT		_IOW(LCD_IOC_MAGIC,  2,int)
#define LCD_IOC_AC_POWER		_IOW(LCD_IOC_MAGIC,  3,int)
#define LCD_IOC_CONTRAST		_IOW(LCD_IOC_MAGIC,  4,int)
#define LCD_IOC_STAT			_IOR(LCD_IOC_MAGIC,  5,int)
#define LCD_IOC_MAXNR 5


//lcd contrast: 0x0400-0x43f  0x0500-0x053f
#define YJ_BOTTOMLIMIT1 	0x041A
#define YJ_TOPLIMIT1 		0x043F
#define YJ_DEFAULTLIMIT1 	0x0436

#define YJ_BOTTOMLIMIT2 	0x0500
#define YJ_TOPLIMIT2 		0x053F
#define YJ_DEFAULTLIMIT2 	0x0514

extern int lcm_open();
extern void lcm_close();
extern void lcm_write();
extern void lcm_ac_power();
extern void modify_lcd_contrast();
#endif
