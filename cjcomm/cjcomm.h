#ifndef CJCOMM_H_
#define CJCOMM_H_

#include <termios.h>
#include <errno.h>
#include <wait.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>


#include "ae.h"
#include "at.h"
#include "ae.h"
#include "dlt698.h"
#include "dlt698def.h"
#include "event.h"
#include "libmmq.h"
#include "AccessFun.h"
#include "StdDataType.h"
#include "PublicFunction.h"

void initComPara(CommBlock* compara);

void GenericRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask);
INT8S GenericWrite(int fd, INT8U* buf, INT16U len);

ProgramInfo* getShareMem(void);
void Comm_task(CommBlock* compara);
void clearcount(int index);
int GetOnlineType(void);
void ClientDestory(void);
int StartClient(struct aeEventLoop* ep, long long id, void* clientData);
void ClientRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask);

void MmqDestory(void);
int StartMmq(struct aeEventLoop* ep, long long id, void* clientData);
int StartVerifiTime(struct aeEventLoop* ep, long long id, void* clientData);

INT8U GetTimeOffsetFlag(void);
void Getk(LINK_Response link, ProgramInfo* JProgramInfo);

void ServerDestory(void);
int StartServer(struct aeEventLoop* ep, long long id, void* clientData);


void IfrDestory(void);
int StartIfr(struct aeEventLoop* ep, long long id, void* clientData);

void SerialDestory(void);
int StartSerial(struct aeEventLoop* ep, long long id, void* clientData);

#endif
