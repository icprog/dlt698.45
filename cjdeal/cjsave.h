

#ifndef CJSAVE_H_
#define CJSAVE_H_
#include "Objectdef.h"
#include "PublicFunction.h"
#include "AccessFun.h"
#include "dlt698.h"
#include "filebase.h"

#define MET_RATE 4
typedef struct{
	INT8U type;//0：oad 1：road
	OAD   oad;
	INT8U num[2];//长度或个数，类型为0，表示长度；类型为1，表示个数
}HEAD_UNIT;



#endif /* CJSAVE_H_ */
