
#ifndef ZDTEST_698_H_
#define ZDTEST_698_H_

#include "unistd.h"
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include "PublicFunction.h"
#include "StdDataType.h"
#include "Shmem.h"
#include "ParaDef.h"


#define    UPDATE_FILE		"/nand/update/update.tar"
#define    GZ_FILE			"/nand/update/user.tar.gz"
#define    TAR_FILE		"/nand/update/user.tar"
#define	MD5_FILES		"/nand/update/md5files"
#define 	MD5				"/nand/update/md5"
#define 	DIR_UPDATE		"/nand/update/"
#define 	DIR_BIN			"nand/bin"
#define 	DIR_LIB			"nor/lib"
#define 	DIR_CONFIG		"nor/config"

#define PrtTestName(str) {\
	fprintf(stderr, "\n***********" #str "***********\n");\
	syslog(LOG_NOTICE,"*********** %s ***********", #str);\
	}while(0);

#define SdPrint(...) {fprintf(stderr, __VA_ARGS__);\
	syslog(LOG_NOTICE,__VA_ARGS__);}while(0);


ProgramInfo* JProgramInfo;		//程序信息结构
unsigned char ProjectNo;

//液晶点坐标
typedef struct {
	int x;
	int y;
}GUI_Point_t;
#define FONTSIZE 12
#define ROWSIZE 12
typedef void (*Func)();
typedef struct {
	char name[50];
	Func func;
	char enable;
}Func_t;


//按键定义-
#define OFFSET_U	0
#define OFFSET_D	1
#define OFFSET_L	2
#define OFFSET_R	3
#define OFFSET_O	4
#define OFFSET_E	5
#define OFFSET_C	6
#define OFFSET_K	7//编程键

#define NOKEY		0
#define UP			(1<<OFFSET_U)
#define DOWN		(1<<OFFSET_D)
#define LEFT		(1<<OFFSET_L)
#define RIGHT		(1<<OFFSET_R)
#define OK1		(1<<OFFSET_O)
#define ESC1		(1<<OFFSET_E)
#define CANCEL		(1<<OFFSET_C)
#define PROGKEY	(1<<OFFSET_K)

//判断线程是否在运行
#define PTHREAD_RUN 	1
#define PTHREAD_STOP 	2

extern void ReadHzkBuff();
extern void lcd_disp(char *str, int x, int y);
extern void openlight();
extern void closelight();
extern void ClearWaitTimes_test(INT8U ProjectID,ProgramInfo* JProgramInfo);
#endif /* RUNAPP_H_ */
