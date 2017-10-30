#include <termios.h>
#include <errno.h>
#include <wait.h>

#ifndef CJCOMM_H_
#define CJCOMM_H_

#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include "ae.h"
#include "anet.h"
#include "dlt698.h"
#include "dlt698def.h"
#include "event.h"
#include "libmmq.h"
#include "AccessFun.h"
#include "StdDataType.h"
#include "PublicFunction.h"

int cWrite(INT8U name, int fd, INT8U *buf, INT16U len);
int cWriteWithCalc(INT8U name, int fd, INT8U *buf, INT16U len);

void cRead(struct aeEventLoop *ep, int fd, void *clientData, int mask);
void cReadWithCalc(struct aeEventLoop *ep, int fd, void *clientData, int mask);
void cReadWithoutCheck(struct aeEventLoop *ep, int fd, void *clientData, int mask);
void cProc(struct aeEventLoop *ep, CommBlock * nst);

int Comm_task(CommBlock *compara);
void refreshComPara(CommBlock *compara);
int StartVerifiTime(struct aeEventLoop *ep, long long id, void *clientData);

void initComPara(CommBlock *compara,
		INT32S (*p_send)(INT8U name, int fd, INT8U *buf, INT16U len));

int StartClientForNet(struct aeEventLoop *ep, long long id, void *clientData);
int StartClientForGprs(struct aeEventLoop *ep, long long id, void *clientData);
int StartMmq(struct aeEventLoop *ep, long long id, void *clientData);
int StartIfr(struct aeEventLoop *ep, long long id, void *clientData);
int StartSerial(struct aeEventLoop *ep, long long id, void *clientData);

//精确校时接口
extern INT8U GetTimeOffsetFlag(void);

extern void Getk_curr(LINK_Response link, ProgramInfo *JProgramInfo);

extern void First_VerifiTime(LINK_Response linkResponse,
		ProgramInfo *JProgramInfo);

//任务自动上送接口
void ConformAutoTask(struct aeEventLoop *ep, CommBlock *nst, int res);

void RegularAutoTask(struct aeEventLoop *ep, CommBlock *nst);

int StartPowerOff(struct aeEventLoop *ep, long long id, void *clientData);

#endif
