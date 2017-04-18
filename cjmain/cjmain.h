#ifndef CJMAIN_H_
#define CJMAIN_H_

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include "PublicFunction.h"

extern void InitClass4300();	//电气设备信息
extern void InitClass4500();	//公网通信模块1
extern void InitClass4510();	//以太网通信模块1

#define MAX_MMQ_SIZE 32

#endif
