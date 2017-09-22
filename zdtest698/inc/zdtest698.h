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
#include "basedef.h"
#include "gpio.h"
#include "lcd_test.h"
#include "function_test.h"

extern void ReadHzkBuff();
extern int readcfg(char *filename, char *name, char *ret);
extern void ReadHzkBuff_12();

extern void lcd_disp(char *str, int x, int y);
extern void openlight();
extern void closelight();
#endif /* RUNAPP_H_ */
