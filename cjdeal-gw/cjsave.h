
#ifndef CJSAVE_H_
#define CJSAVE_H_
#include "Objectdef.h"
#include "PublicFunction.h"
#include "AccessFun.h"
#include "dlt698.h"
#include "filebase.h"

extern void saveREADOADdata(INT8U taskid, TSA tsa, OADDATA_SAVE *OADdata,
		INT8U OADnum, TS OADts);
extern INT16U saveREADOADevent(ROAD eve_road, TSA tsa, OADDATA_SAVE *OADdata,
		INT8U OADnum, TS OADts);
#endif /* CJSAVE_H_ */
