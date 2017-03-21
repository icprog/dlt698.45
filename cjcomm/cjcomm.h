#include <termios.h>
#include <errno.h>
#include <wait.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include "ae.h"
#include "at.h"
#include "anet.h"
#include "dlt698.h"
#include "dlt698def.h"
#include "event.h"
#include "libmmq.h"
#include "AccessFun.h"
#include "StdDataType.h"
#include "PublicFunction.h"

#ifndef CJCOMM_H_
#define CJCOMM_H_

//维护公共状态的函数
int Comm_task(CommBlock* compara);
void clearcount(int index);
int GetOnlineType(void);
void SetOnlineType(int type);
void refreshComPara(CommBlock* compara);

int StartVerifiTime(struct aeEventLoop* ep, long long id, void* clientData);

int GetInterFaceIp(char* interface, char* ips);

void initComPara(CommBlock* compara, INT8S (*p_send)(int fd, INT8U* buf, INT16U len));
void CalculateTransFlow(ProgramInfo* prginfo_event);
void EventAutoReport(CommBlock* nst);
void dumpPeerStat(int fd, char* info);

//以太网通信接口
int StartClientForNet(struct aeEventLoop* ep, long long id, void* clientData);
void ClientForNetDestory(void);

// GPRS网络通信接口
int StartClientForGprs(struct aeEventLoop* ep, long long id, void* clientData);
void ClientForGprsDestory(void);

//消息队列通信接口
void MmqDestory(void);
int StartMmq(struct aeEventLoop* ep, long long id, void* clientData);

//服务端通信接口
void ServerDestory(void);
int StartServer(struct aeEventLoop* ep, long long id, void* clientData);

//红外通信接口
void IfrDestory(void);
int StartIfr(struct aeEventLoop* ep, long long id, void* clientData);

//维护串口通信接口
void SerialDestory(void);
int StartSerial(struct aeEventLoop* ep, long long id, void* clientData);

//精确校时接口
INT8U GetTimeOffsetFlag(void);
void Getk(LINK_Response link, ProgramInfo* JProgramInfo);

//任务自动上送接口
void ConformAutoTask(struct aeEventLoop* ep, CommBlock* nst, int res);
void RegularAutoTask(struct aeEventLoop* ep, CommBlock* nst);

#endif
