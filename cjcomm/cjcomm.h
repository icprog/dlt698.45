#ifndef CJCOMM_H_
#define CJCOMM_H_

#include <termios.h>
#include <errno.h>
#include <wait.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include "StdDataType.h"
#include "PublicFunction.h"
#include "dlt698.h"
#include "ae.h"

void initComPara(CommBlock* compara);

void NETRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask);
void GenericRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask);
INT8S GenericWrite(int fd, INT8U* buf, INT16U len);


int getOnlineState(void);

void setCCID(INT8U CCID[]);
void setIMSI(INT8U IMSI[]);
void setSINSTR(INT16U SINSTR);
void setPPPIP(INT8U PPPIP[]);
void saveCurrClass25(void);

INT8U GetTimeOffsetFlag(void);
void Getk(LINK_Response link, ProgramInfo* JProgramInfo);

void ServerDestory(void);
int StartServer(struct aeEventLoop* ep, long long id, void* clientData);


void IfrDestory(void);
int StartIfr(struct aeEventLoop* ep, long long id, void* clientData);

void SerialDestory(void);
int StartSerial(struct aeEventLoop* ep, long long id, void* clientData);

#endif
