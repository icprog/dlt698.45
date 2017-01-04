#ifndef V_GPRS_H_
#define V_GPRS_H_

#include "ae.h"
#include "msgr.h"
#include "mtypes.h"
#include "libgdw3761.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <mqueue.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <termios.h>
#include <sys/types.h>

//所有流程标志完成的标志
#define STEP_OK 99

// gprs模块通信波特率
#define MODEM_BAUND 115200

#define PowerOnStep 0
#define ModemSwtStep 10
#define ModemRstStep 20
#define AtCheckStep 30
#define MuxCheckStep 40
#define modemCheckStep 50
#define DailStep 60
#define ipCheckStep 70

#define FinallyStep 99
#define CleanStep 100
#define StandByStep 110
#define ShutDown 999

#define NET_GPRS 1
#define NET_CDMA2000 2

#define NODEBUFSIZE 1024

typedef struct node{
	int len;
	int id;
	INT8U buf[NODEBUFSIZE];
}SendNode;

struct mNetworker;

int findStr(char* Des, char* target, int len);

//完全重置网络模块，从新开始拨号各个流程
void gprsDestory(struct aeEventLoop* eventLoop, struct mNetworker* ao);

//终止拨号流程，清理拨号数据
void gprsShutDown(struct aeEventLoop* eventLoop, struct mNetworker* ao);

INT8S netWrite(INT8U* buf, INT16U len);
void netClientRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask);
void netMixRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask);
void serRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask);
void netAccept(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask);

int SendATCommand(char* buf, int len, int com);
void atCheckRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask);
void atModemRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask);
void atModemOnLine(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask);

int gpifun(char* devname);
int gpofun(char* devname, int data);

int OpenMuxCom(INT8U port, int baud, unsigned char* par, unsigned char stopb, INT8U bits);

void serialMuxShutDown(void);

int ModemConstruct(struct aeEventLoop* ep, long long id, void* clientData);
int ModemOnLine(struct aeEventLoop* ep, long long id, void* clientData);

INT8S netModemWrite(INT8U* buf, INT16U len);
void netModemRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask);

#endif
