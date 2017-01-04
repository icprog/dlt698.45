#include "../lib3761/libgdw3761.h"
#include "handle.h"
#include "mtypes.h"
#include "../include/erc.h"

#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

INT8S comWrite(INT8U* buf, INT16U len) {
    mAgent* ao = getComStruct();
    write(ao->fd, buf, len);
}

void comRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask) {
    mAgent* agentCom = (mAgent*)clientData;

    int revcount = 0;
    ioctl(agentCom->fd, FIONREAD, &revcount);

    //设备异常，关闭端口
    if (revcount == 0) {
        mAgentDestory(eventLoop, agentCom);
    }

    int len, j = 0;
    for (j = 0; j < revcount; j++) {
        len = read(agentCom->fd, agentCom->NetRevBuf + agentCom->rev_head, 1);
        agentCom->rev_head += len;
        agentCom->rev_head %= FrameSize;
    }

    int deallen = gdw3761_preprocess(&agentCom->step, &agentCom->rev_delay, 100, &agentCom->rev_tail, &agentCom->rev_head,
                                     agentCom->NetRevBuf, agentCom->DealBuf);

    if (deallen > 0) {
        gdw3761_setCallback(ifrWrite);
        int ret = gdw3761_parse(agentCom->DealBuf, deallen, NULL);
        if (ret > 0) {
        	fprintf(stderr,"[vMsgr][Com]主站响应 [AFN %02x]\n", ret);
        }
    }
}
