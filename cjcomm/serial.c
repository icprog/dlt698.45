#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ae.h"
#include "rlog.h"
#include "dlt698def.h"
#include "cjcomm.h"
#include "AccessFun.h"
#include "PublicFunction.h"

//维护串口监听、处理结构体
static CommBlock SerialObject;
static long long Serial_Task_Id;

/*
 * 模块*内部*使用的初始化参数
 */
void SerialInit(void) {
	asyslog(LOG_INFO, "初始化维护串口模块...");
	initComPara(&SerialObject);
}

/*
 * 用于程序退出时调用
 */
void SerialDestory(void) {
	//关闭资源
    asyslog(LOG_INFO, "关闭通信接口(%d)", SerialObject.phy_connect_fd);
    close(SerialObject.phy_connect_fd);
    SerialObject.phy_connect_fd = -1;
}

/*
 * 模块维护循环
 */
int RegularSerial(struct aeEventLoop* ep, long long id, void* clientData){
	CommBlock* nst = (CommBlock*)clientData;
    if (nst->phy_connect_fd < 0) {
        if ((nst->phy_connect_fd = OpenCom(4, 9600, (unsigned char*)"even", 1, 8)) <= 0) {
            asyslog(LOG_ERR, "维护串口打开失败");
        } else {
            if (aeCreateFileEvent(ep, nst->phy_connect_fd, AE_READABLE, GenericRead, nst) < 0) {
                asyslog(LOG_ERR, "维护串口监听失败");
                close(nst->phy_connect_fd);
                nst->phy_connect_fd = -1;
                return 10 * 1000;
            }
        }
    }
    return 1000;
}

/*
 * 供外部使用的初始化函数，并开启维护循环
 */
int StartSerial(struct aeEventLoop* ep, long long id, void* clientData){
	SerialInit();
	Serial_Task_Id = aeCreateTimeEvent(ep, 1000, RegularSerial, &SerialObject, NULL);
	asyslog(LOG_INFO, "维护串口时间事件注册完成(%lld)", Serial_Task_Id);
	return 1;
}

/*
 * 红外、维护串口通行的读取函数，函数共享使用
 */
void GenericRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask) {
    CommBlock* nst = (CommBlock*)clientData;

    //判断fd中有多少需要接收的数据
    int revcount = 0;
    ioctl(nst->phy_connect_fd, FIONREAD, &revcount);

    //关闭异常端口
    if (revcount == 0) {
        aeDeleteFileEvent(eventLoop, nst->phy_connect_fd, AE_READABLE);
        close(nst->phy_connect_fd);
        nst->phy_connect_fd = -1;
    }

    if (revcount > 0) {
        for (int j = 0; j < revcount; j++) {
            read(nst->phy_connect_fd, &nst->RecBuf[nst->RHead], 1);
            nst->RHead = (nst->RHead + 1) % BUFLEN;
        }
        bufsyslog(nst->RecBuf, "Recv:", nst->RHead, nst->RTail, BUFLEN);
        for (int k = 0; k < 3; k++) {
            int len = 0;
            for (int i = 0; i < 5; i++) {
                len = StateProcess(nst, 10);
                if (len > 0) {
                    break;
                }
            }

            if (len > 0) {
                int apduType = ProcessData(nst);
                switch (apduType) {
                    case LINK_RESPONSE:
                        nst->linkstate   = build_connection;
                        nst->testcounter = 0;
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

/*
 *所有模块共享的写入函数，所有模块共享使用
 */
INT8S GenericWrite(int fd, INT8U* buf, INT16U len) {
	asyslog(LOG_WARNING, "发送报文(长度:%d)", len);
    int ret = anetWrite(fd, buf, (int)len);
    if (ret != len) {
        asyslog(LOG_WARNING, "报文发送失败(长度:%d,错误:%d)", len, errno);
    }
    bufsyslog(buf, "Send:", len, 0, BUFLEN);
    return ret;
}

