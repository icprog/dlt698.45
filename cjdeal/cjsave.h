

#ifndef CJSAVE_H_
#define CJSAVE_H_
#include "Objectdef.h"
#include "PublicFunction.h"
#include "AccessFun.h"
#include "dlt698.h"
#include "filebase.h"


extern int SaveNorData(INT8U taskid,ROAD *road_eve,INT8U *databuf,int datalen);//,TS ts_cc);
extern void ReadNorData(TS ts,INT8U taskid,INT8U *tsa);

#endif /* CJSAVE_H_ */
