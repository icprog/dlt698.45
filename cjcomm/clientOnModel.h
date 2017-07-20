#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <pthread.h>

#include "PublicFunction.h"
#include "dlt698def.h"
#include "cjcomm.h"
#include "at.h"
#include "ae.h"
#include "../include/Shmem.h"

typedef struct {
    INT8U buf[4096];
    INT16U len;
} Block;

typedef struct {
    Block send[16];
    Block recv;
    INT16U head;
    INT16U tail;
} NetObject;


#define AT_FRAME_LEN 4096

typedef struct {
	INT32U fd;
	INT32U state;	//作为AT流程的流转标识
	INT32U RecvHead;
	INT32U RecvTail;
	INT32U SendHead;
	INT32U SendTail;

	INT8U recv[AT_FRAME_LEN];
	INT8U deal[AT_FRAME_LEN];
	INT8U send[AT_FRAME_LEN];
	INT32U NeedGet;	//需要读取数据
	INT32U CanSend;	//可以发送数据,数字为发送的字节数
} ATOBJ;

int AtInitObjBlock(ATOBJ *ao);
int AtProcessRecv(ATOBJ *ao);
int AtDealData(ATOBJ *ao);
int AtNeedRead(ATOBJ *ao);
int AtNeedSend(ATOBJ *ao);

