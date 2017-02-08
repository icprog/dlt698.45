

#ifndef DLT645_H_
#define DLT645_H_

#include "StdDataType.h"

typedef struct
{
	INT8U Addr[6];			//表地址
	INT8U Ctrl;				//控制码
	INT16U Length;			//数据域长度
	INT8U DI[4];			//数据项标识
	INT8U Data[1024];		//数据内容
	INT8U SEQ;				//帧序号
	INT8U Err;
}FORMAT07;
//extern int ProcessData(unsigned char* Rcvbuf,int Rcvlen,unsigned char* addr);

#endif
