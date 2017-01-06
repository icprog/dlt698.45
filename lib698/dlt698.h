

#ifndef DLT698_H_
#define DLT698_H_
#include "StdDataType.h"

extern int doObjectAction(OMD omd,INT8U *data);
extern int StateProcess(int* step,int* rev_delay,int delay_num, int* rev_tail,int* rev_head,unsigned char *NetRevBuf,unsigned char* dealbuf);
extern int ProcessData(CommBlock *com);
extern int Link_Request(LINK_Request request,INT8U *addr,INT8U *buf);
#endif
