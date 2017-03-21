#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "cjcomm.h"

//维护串口监听、处理结构体
static CommBlock ServerObject;
static long long Server_Task_Id;
static int listen_port;

/*
 *所有模块共享的写入函数，所有模块共享使用
 */
int ServerWrite(int fd, INT8U* buf, INT16U len) {
    int ret = anetWrite(fd, buf, (int)len);
    if (ret != len) {
        asyslog(LOG_WARNING, "[服务]报文发送失败(长度:%d,错误:%d)", len, errno);
    }
    bufsyslog(buf, "服务发送:", len, 0, BUFLEN);
    return ret;
}

/*
 * 模块*内部*使用的初始化参数
 */
void ServerInit(void) {
    asyslog(LOG_INFO, "初始化监听服务器模块...");
    listen_port = -1;
    initComPara(&ServerObject, ServerWrite);
}

void ServerRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask) {
    CommBlock* nst = (CommBlock*)clientData;

    //判断fd中有多少需要接收的数据
    int revcount = 0;
    ioctl(fd, FIONREAD, &revcount);

    //关闭异常端口
    if (revcount == 0) {
        asyslog(LOG_WARNING, "链接出现异常，关闭端口");
        aeDeleteFileEvent(eventLoop, fd, AE_READABLE);
        close(fd);
        nst->phy_connect_fd = -1;
    }

    if (revcount > 0) {
        for (int j = 0; j < revcount; j++) {
            read(fd, &nst->RecBuf[nst->RHead], 1);
            nst->RHead = (nst->RHead + 1) % BUFLEN;
        }
        bufsyslog(nst->RecBuf, "服务接收:", nst->RHead, nst->RTail, BUFLEN);

        for (int k = 0; k < 5; k++) {
            int len = 0;
            for (int i = 0; i < 5; i++) {
                len = StateProcess(nst, 10);
                if (len > 0) {
                    break;
                }
            }
            if (len <= 0) {
                break;
            }

            if (len > 0) {
                int apduType = ProcessData(nst);
                fprintf(stderr, "apduType=%d\n", apduType);
                ConformAutoTask(eventLoop, nst, apduType);
                switch (apduType) {
                    case LINK_RESPONSE:
                        if (GetTimeOffsetFlag() == 1) {
                            Getk(nst->linkResponse, nst->shmem);
                        }
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

void CreateAptSer(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask) {
    CommBlock* nst = (CommBlock*)clientData;
    char errmsg[128];
    memset(errmsg, 0x00, sizeof(errmsg));

    //如果已经接受主站连接，先关闭
    if (nst->phy_connect_fd > 0) {
        aeDeleteFileEvent(eventLoop, nst->phy_connect_fd, AE_READABLE);
        close(nst->phy_connect_fd);
        nst->phy_connect_fd = -1;
    }

    //如果成功建立主站连接，则创建IO事件；否则重新建立监听
    nst->phy_connect_fd = anetTcpAccept(errmsg, fd, NULL, 0, NULL);
    if (nst->phy_connect_fd > 0) {
        asyslog(LOG_INFO, "建立主站反向链接(结果:%d)", nst->phy_connect_fd);
        if (aeCreateFileEvent(eventLoop, nst->phy_connect_fd, AE_READABLE, ServerRead, nst) == -1) {
            aeDeleteFileEvent(eventLoop, nst->phy_connect_fd, AE_READABLE);
            close(fd);
        }
    } else {
        asyslog(LOG_WARNING, "监听端口出错,原因(%s)", errmsg);
        aeDeleteFileEvent(eventLoop, nst->phy_connect_fd, AE_READABLE);
        close(fd);
    }
}

/*
 * 模块维护循环
 */
int RegularServer(struct aeEventLoop* ep, long long id, void* clientData) {
    CommBlock* nst = (CommBlock*)clientData;
    char errmsg[128];
    memset(errmsg, 0x00, sizeof(errmsg));
    if (listen_port < 0) {
        listen_port = anetTcpServer(errmsg, 5555, "0.0.0.0", 1);
        if (listen_port >= 0) {
            aeCreateFileEvent(ep, listen_port, AE_READABLE, CreateAptSer, nst);
        } else {
            asyslog(LOG_ERR, "网络监听建立失败，1分钟后重建，出错原因(%s)", errmsg);
            return 60 * 1000;
        }
    }
    return 1000;
}

/*
 * 供外部使用的初始化函数，并开启维护循环
 */
int StartServer(struct aeEventLoop* ep, long long id, void* clientData) {
    ServerInit();
    Server_Task_Id = aeCreateTimeEvent(ep, 1000, RegularServer, &ServerObject, NULL);
    asyslog(LOG_INFO, "监听服务器时间事件注册完成(%lld)", Server_Task_Id);
    return 1;
}

/*
 * 用于程序退出时调用
 */
void ServerDestory(void) {
    //关闭资源
    asyslog(LOG_INFO, "关闭监听服务器(%d)", ServerObject.phy_connect_fd);
    close(ServerObject.phy_connect_fd);
    ServerObject.phy_connect_fd = -1;

    //关闭监听的端口
    close(listen_port);
    listen_port = -1;
}
