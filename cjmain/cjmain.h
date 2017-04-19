#ifndef CJMAIN_H_
#define CJMAIN_H_

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include "PublicFunction.h"

extern void InitClass4016();	//当前套日时段表
extern void InitClass4300();	//电气设备信息
extern void InitClass4500();	//公网通信模块1
extern void InitClass4510();	//以太网通信模块1
extern void InitClass6000();	//采集配置单元
extern void InitClassf203();	//开关量输入

#define MAX_MMQ_SIZE 32

#endif
