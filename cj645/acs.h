/*
 * acs.h
 *
 *  Created on: Jan 25, 2017
 *      Author: ava
 */

#ifndef ACS_H_
#define ACS_H_

#include "StdDataType.h"
#include <stdint.h>

#define dbg_prt(fmt,...) do	\
{	\
	{ } \
}while(0)

/*
 * #define dbg_prt(fmt,...) do \
	{ \
		{ \
			fprintf(stderr,fmt, ## __VA_ARGS__);\
			fprintf(stderr,"\n");\
		}\
}while(0)
*/

#define U			1			//电压
#define I			2			//电流
#define P			3			//有功
#define Q			4			//无功
#define S			5			//视在
#define COS			6			//功率因数
#define FREQ		7			//频率
#define PHASE		8			//电压与电流相角
#define ANGLE		9			//电压夹角
#define TEMP		10			//温度计算



typedef struct {
	//以下定义存放：ATT7022E寄存器值
	INT32U	EPt;	//合相有功电能
	INT32U	EPa;
	INT32U	EPb;
	INT32U	EPc;

	INT32U	EQt;	//合相无功电能
	INT32U	EQa;
	INT32U	EQb;
	INT32U	EQc;
	//以下定义存放：根据有功、无功功率方向计算正向、反向电能示值
	INT32U	PosPa;//正相有功分项电能示值，根据
	INT32U	PosPb;
	INT32U	PosPc;
	INT32U	PosPt;

	INT32U	PosQa;//正相无功分项电能示值
	INT32U	PosQb;
	INT32U	PosQc;
	INT32U	PosQt;//组合无功1

	INT32U	NegPa;//反相有功分项电能示值
	INT32U	NegPb;
	INT32U	NegPc;
	INT32U	NegPt;

	INT32U	NegQa;//反相无功分项电能示值
	INT32U	NegQb;
	INT32U	NegQc;
	INT32U	NegQt;
}_EnergyCurr;

typedef struct {
    INT8U  	point;
    INT32U Buff[16];
}_AverageStru;

extern void InitACSPara();
extern void acs_process();
extern void dumpstat(const char *name, int fd,uint32_t speed);
#endif /* ACS_H_ */
