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

//红外口监听、处理结构体
static CommBlock FirObject;
static long long Serial_Task_Id;

static int GlobBand[] = {300, 600, 1200, 2400, 4800, 7200, 9600, 19200, 38400, 57600, 115200};
static char *GlobCrc[] = {"none", "odd", "even"};
static int GlobData[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
static int GlobStop[] = {0, 1, 2};

/*
 *所有模块共享的写入函数，所有模块共享使用
 */
static int IfrWrite(int fd, INT8U *buf, INT16U len) {
    int ret = anetWrite(fd, buf, (int) len);
    if (ret != len) {
        asyslog(LOG_WARNING, "[红外]报文发送失败(长度:%d,错误:%d)", len, errno);
    }
    bufsyslog(buf, "红外发送:", len, 0, BUFLEN);
    return ret;
}

/*
 * 模块*内部*使用的初始化参数
 */
static void IfrInit(void) {
    asyslog(LOG_INFO, "初始化红外模块...");
    initComPara(&FirObject, IfrWrite);
}

/*
 * 红外、维护串口通行的读取函数，函数共享使用
 */
static void IfrRead(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask) {
    CommBlock *nst = (CommBlock *) clientData;

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
        bufsyslog(nst->RecBuf, "红外接收(100):", nst->RHead, nst->RTail, BUFLEN);
        for (int k = 0; k < 3; k++) {
            int len = 0;
            for (int i = 0; i < 5; i++) {
                len = StateProcess(nst, 100);
                if (len > 0) {
                    break;
                }
            }

            if (len > 0) {
                int apduType = ProcessData(nst);
                switch (apduType) {
                    case LINK_RESPONSE:
                        nst->linkstate = build_connection;
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
 * 为I型集中器的特殊处理，改为轮询红外口数据的方式
 */
static void IfrReadLoop(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask) {
    CommBlock *nst = (CommBlock *) clientData;

    //判断fd中有多少需要接收的数据
    int revcount = 0;
    ioctl(nst->phy_connect_fd, FIONREAD, &revcount);

    if (revcount > 0) {
        for (int j = 0; j < revcount; j++) {
            read(nst->phy_connect_fd, &nst->RecBuf[nst->RHead], 1);
            nst->RHead = (nst->RHead + 1) % BUFLEN;
        }
    }

    if (nst->RHead != nst->RTail) {
        bufsyslog(nst->RecBuf, "红外接收():", nst->RHead, nst->RTail, BUFLEN);
    }

    int len = StateProcess(nst, 10);
    if (len > 0) {
        int apduType = ProcessData(nst);
        switch (apduType) {
            case LINK_RESPONSE:
                nst->linkstate = build_connection;
                nst->testcounter = 0;
                break;
            default:
                break;
        }
    }
}

/*
 * 模块维护循环
 */
static int RegularIfr(struct aeEventLoop *ep, long long id, void *clientData) {
    CLASS_f202 oif202 = {};
    CommBlock *nst = (CommBlock *) clientData;

    if (nst->phy_connect_fd < 0) {
        if (readCoverClass(0xf202, 0, &oif202, sizeof(CLASS_f202), para_vari_save) > 0) {
            fprintf(stderr, "红外模块读取参数：波特率(%d)，校验方式(%s)，数据位(%d)，停止位(%d)\n", GlobBand[oif202.devpara.baud],
                    GlobCrc[oif202.devpara.verify], GlobData[oif202.devpara.databits],
                    GlobStop[oif202.devpara.stopbits]);
            nst->phy_connect_fd = OpenCom(3, GlobBand[oif202.devpara.baud],
                                          (unsigned char *) GlobCrc[oif202.devpara.verify],
                                          GlobStop[oif202.devpara.stopbits],
                                          GlobData[oif202.devpara.databits]);
        } else {
            nst->phy_connect_fd = OpenCom(3, 1200, (unsigned char *) "even",
                                          1, 8);
        }

        if (nst->phy_connect_fd <= 0) {
            asyslog(LOG_ERR, "红外串口打开失败");
        } else {
//            if (aeCreateFileEvent(ep, nst->phy_connect_fd, AE_READABLE, IfrRead, nst) < 0) {
//                asyslog(LOG_ERR, "红外串口监听失败");
//                close(nst->phy_connect_fd);
//                nst->phy_connect_fd = -1;
//                return 10 * 1000;
//            }
        }
    } else {
        // 为I型集中器的特殊处理
        IfrReadLoop(ep, 0, nst, 0);
    }
    return 1000;
}

/*
 * 供外部使用的初始化函数，并开启维护循环
 */
int StartIfr(struct aeEventLoop *ep, long long id, void *clientData) {
    IfrInit();

    Serial_Task_Id = aeCreateTimeEvent(ep, 1000, RegularIfr, &FirObject, NULL);
    asyslog(LOG_INFO, "红外串口时间事件注册完成(%lld)", Serial_Task_Id);
    return 1;
}

/*
 * 用于程序退出时调用
 */
void IfrDestory(void) {
    //关闭资源
    asyslog(LOG_INFO, "关闭红外通信接口(%d)", FirObject.phy_connect_fd);
    close(FirObject.phy_connect_fd);
    FirObject.phy_connect_fd = -1;
}
