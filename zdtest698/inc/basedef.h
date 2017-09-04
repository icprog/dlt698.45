#ifndef	BASE_DEF_H
#define	BASE_DEF_H

#define OK  		1
#define ERROR  		0
#define UNKNOWN  	-1

#define S4851   		1
#define S4852   		2
#define S4853   		3
#define SER_ZB			5

/*
 * 由宏定义判断终端类型
 * 现在已改为读取/nor/config/device.cfg
 * 来决定终端的类型
 */
#define CCTT_I //I型集中器
//#define CCTT_II//II型集中器
//#define SPTF_III//III型专变

#define CCTT1	1//与配置文件约定的I型集中器编号
#define CCTT2	2//与配置文件约定的II型集中器编号
#define SPTF3	3//与配置文件约定的III型专变编号

#define JZQTEST_PARA_NAME 				"/dos/cjgwn/zdtest698/zdtest698_para.cfg"
#define TEST_LOG_FILE					"/nand/utest.log"


typedef enum {
	STATE1 = 1,
	STATE2 = 2,
	STATE3 = 3,
	STATE4 = 4,
	PLUSE1 = 5,
	PLUSE2 = 6
} DEV_STATE_PULSE;


/* Use 'L' as magic number, means LCD */
#define LCD_IOC_MAGIC  'L'
/* Please use a different 8-bit number in your code */
#define _USERDIR_ 							"/nand/bin"
#define LCD_IOCRESET    				_IO(LCD_IOC_MAGIC, 0)
#define LCD_IOC_UPDATE 			_IOW(LCD_IOC_MAGIC,  1,int)
#define LCD_IOC_BACKLIGHT		_IOW(LCD_IOC_MAGIC,  2,int)
#define LCD_IOC_AC_POWER		_IOW(LCD_IOC_MAGIC,  3,int)
#define LCD_IOC_CONTRAST		_IOW(LCD_IOC_MAGIC,  4,int)
#define LCD_IOC_STAT					_IOR(LCD_IOC_MAGIC,  5,int)
#define LCD_IOC_MAXNR 				5

//按键定义-
#define OFFSET_U		0//上
#define OFFSET_D		1//下
#define OFFSET_L		2//左
#define OFFSET_R		3//右
#define OFFSET_O		4//确定
#define OFFSET_E		5//取消
#define OFFSET_C		6
#define OFFSET_K		7//编程键

#define NOKEY				0
#define UP						(1<<OFFSET_U)
#define DOWN					(1<<OFFSET_D)
#define LEFT					(1<<OFFSET_L)
#define RIGHT					(1<<OFFSET_R)
#define KEY_OK				(1<<OFFSET_O)
#define KEY_ESC			(1<<OFFSET_E)
#define CANCEL				(1<<OFFSET_C)
#define PROGKEY			(1<<OFFSET_K)

#define LCM_X    160
#define LCM_Y    160

#define FONTSIZE 12
#define ROWSIZE 12


//判断线程是否在运行
#define PTHREAD_RUN 	1
#define PTHREAD_STOP 	2

//液晶点坐标
typedef struct Point {
	int x;
	int y;
}Point_s;

typedef void (*Func)();

typedef struct {
	char name[50];
	Func func;
	INT8U enable;
} Func_t;

#endif
