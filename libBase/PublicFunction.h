/*
 * PublicFunction.h
 *
 *  Created on: Sep 11, 2015
 *      Author: fzh
 */

#ifndef PUBLICFUNCTION_H_
#define PUBLICFUNCTION_H_

#include <signal.h>
#include <semaphore.h>
#include "Shmem.h"
#include "StdDataType.h"
#include "ParaDef.h"


extern void TSGet(TS *ts);
extern INT8U TScompare(TS ts1,TS ts2);
extern time_t tmtotime_t(TS ptm);
extern void DataTimeGet(DateTimeBCD* ts);
INT8S tminc(TS* tmi,Time_Units unit,INT32S val);
extern void Setsig(struct sigaction *psa,void (*pfun)(ProjectInfo *proinfo));
/*
 * 共享内存操作函数
 * */
extern void* CreateShMem(char* shname,int memsize,void* pmem);
extern void* OpenShMem(char* shname,int memsize,void* pmem);
extern void  shmm_unregister(char* shname,int memsize);

/*
 * 串口操作函数
 * */
extern int OpenCom(int port,int baud,unsigned char *par,unsigned char stopb,unsigned char bits);
extern void CloseCom(int ComPort);

/*
 * 时间转换函数
 * */
extern void TSGet(TS *ts);
extern time_t tmtotime_t(TS ptm);
extern void DataTimeGet(DateTimeBCD* ts);


/* BCD码转int32u
 *参数：bcd为bcd码头指针，len为bcd码长度，order为positive正序/inverted反序，dint转换结果
 * 返回:0：成功；-1：asc为空；-2：en为0；-3：order有误
 * 例如:0x12 0x34 -> 1234
 * */
extern INT8S bcd2int32u(INT8U *bcd, INT8U len,ORDER order,INT32U* dint);
extern INT32S int32u2bcd(INT32U dint32, INT8U* bcd,ORDER order);
/*
 * gpio操作函数
 */
extern INT8S gpio_readbyte(char* devpath);
extern INT32S gpio_readint(char* devpath) ;
extern INT32S gpio_writebyte(char* devpath, INT8S data) ;
extern INT32S gpio_writebytes(char* devpath, INT8S* vals, INT32S valnum) ;

extern BOOLEAN pwr_has();
extern BOOLEAN pwr_has_byVolt(INT8U valid,INT32U volt,INT16U limit);
extern BOOLEAN bettery_getV(FP32* clock_bt,FP32* tmnl_bt);

extern INT8S getSpiAnalogState();
/*
 * 信号量操作函数
 * */
extern sem_t* nsem_open(const char* name);
extern sem_t* create_named_sem(const char* name, int flag);
extern sem_t* open_named_sem(const char* name);

#endif /* PUBLICFUNCTION_H_ */
