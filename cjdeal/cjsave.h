

#ifndef CJSAVE_H_
#define CJSAVE_H_
#include "Objectdef.h"
#include "PublicFunction.h"
#include "AccessFun.h"
#include "dlt698.h"
#include "filebase.h"

#define MET_RATE 4
typedef struct {

};

extern void SaveNorData(INT8U taskid,INT8U *databuf,int datalen);
extern void ReadNorData(TS ts,INT8U taskid,INT8U *tsa);

#endif /* CJSAVE_H_ */
