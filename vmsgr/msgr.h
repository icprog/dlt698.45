#ifndef V_MSGER_H_
#define V_MSGER_H_

#include "ae.h"
#include "gprs.h"
#include "adlist.h"
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
#include <errno.h>

#define FrameSize 8192

/*
 * 将所有使用3761规约的通信口抽象画为agent
 */
struct mAgent;

typedef void mAgentWrite(INT8U* buf, INT16U len);

typedef struct mAgent {
    int fd; //当前端口

    //串口参数
    unsigned int Baud;        //波特率
    unsigned char Bits;       //位数
    unsigned char Parity[12]; //奇偶
    unsigned char Stopb;      //停止位

    // 3761状态机调用所需要的参数
    int step;
    int rev_delay;
    int rev_tail;
    int rev_head;
    INT8U NetRevBuf[FrameSize];
    INT8U DealBuf[FrameSize];
    INT8U NetSendBuf[FrameSize];

    long long lastReadTime;  // 最后一次读取时间
    long long lastWriteTime; // 最后一次写入时间
    long long ReadCount;     // 读取总数
    long long WriteCount;    // 写入总数
    int onlineState;         // 在线标志位
    int retryDelay;          // fd打开失败之后的重试时间（秒）

    mAgentWrite* write;
    aeFileProc* read;

} mAgent;

// at命令缓冲区长度
#define RES_LENGTH 2048

#define NET_ONLINE 1
#define GPRS_ONLINE 2

#define ONLINE_MODEM 1 //内部协议栈（非透明传输）
#define ONLINE_STACK 2 //外部协议栈

#define TX_MIX_MODE 0    //混合模式
#define TX_CLIENT_MODE 1 //客户端模式

typedef struct mNetworker {
    int fd;           //网络主站通信
    int listenPort;   //监听服务地址
    int serverPort;   //服务器(主站)链接
    int originSerial; //模块通信的fd，没有经过串口复用
    int sMux0;        //正常AT通信
    int sMux1;        //控制模块LED灯

    INT8U atRecBuf[RES_LENGTH]; //模块at通信缓冲区
    INT8U atComBuf[RES_LENGTH];
    list* mlist;                //内部协议栈发送缓冲区

    INT8U address_A3; //组地址A3

    //网络参数
    char MainIp[30], BackIp[30], APN[16];
    int MainPort, BackPort, HeartBeat;

    // 3761状态机调用所需要的参数
    int step;
    int rev_delay;
    int rev_tail;
    int rev_head;
    INT8U NetRevBuf[FrameSize];
    INT8U DealBuf[FrameSize];
    INT8U NetSendBuf[FrameSize];

    long long lastReadTime;  // 最后一次读取时间
    long long lastWriteTime; // 最后一次写入时间
    long long ReadCount;     // 读取总数
    long long WriteCount;    // 写入总数
    int onlineState;         // 在线标志位
    int retryDelay;          // fd打开失败之后的重试时间（秒）

    mAgentWrite* write;
    aeFileProc* read;

    //内部协议栈与外部协议栈标志
    INT8U abilitychs;

    // tcp、udp标志
    INT8U netFamily;

    // gprs、cdma2000类型标志
    INT8U netType;

    //通信模式（客户机、服务器、混合模式）
    int modeType;

    //通信模式(TCP、UDP)
    int workType;

    //拨号流程所需要的步骤信息
    INT16U ConstructStep;

    //拨号流程允许的重试次数，随用随设定
    INT8U retry;

    int sendRetry;

} mNetworker;

typedef struct ammq {
    int fd;
    struct mq_attr attr;
    RealDataInfo* allrealinfo;
    TransDataInfo* allrtransinfo;
} mmqAgent;

typedef struct msgr {
    mAgent ifr;     // 红外接口处理
    mAgent com;     // 维护接口处理
    mNetworker net; // 以太网络接口处理
    mmqAgent mmq;   //内部消息队列
} MSGR_ONLY_ONE;

typedef MSGR_ONLY_ONE MSGR;

/* Prototypes */
void mAgentCreate(mAgent* ag, mAgentWrite* write, aeFileProc* read);
void mAgentDestory(struct aeEventLoop* eventLoop, mAgent* ag);

int msgrCreate(MSGR* msgr, int mode);
void msgrDestory(struct aeEventLoop* ep);

mAgent* getIfrStruct(void);
mAgent* getComStruct(void);
mNetworker* getNetStruct(void);
mmqAgent* getMmqStruct(void);

void mNetCreate(mNetworker* networker, mAgentWrite* write, aeFileProc* read);

void DbPrt1(char* prefix, char* buf, int len, char* suffix);

#endif
