#include "libgdw3761.h"
#include "handle.h"
#include "mtypes.h"

#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

void gprsShutDown(struct aeEventLoop* eventLoop, mNetworker* ao) {
	if (ao->originSerial > 0){
	    aeDeleteFileEvent(eventLoop, ao->originSerial, AE_READABLE);
	    close(ao->originSerial);
	    ao->originSerial = 0;
	}
    //清理拨号数据

	if (ao->sMux0 > 0){
	    aeDeleteFileEvent(eventLoop, ao->sMux0, AE_READABLE);
	    close(ao->sMux0);
	    ao->sMux0 = 0;
	}

	if (ao->sMux1 > 0){
	    aeDeleteFileEvent(eventLoop, ao->sMux1, AE_READABLE);
	    close(ao->sMux1);
	    ao->sMux1 = 0;
	}

    bzero(ao->atRecBuf, sizeof(ao->atRecBuf));

    ao->ConstructStep = ShutDown;
    serialMuxShutDown();
}

void gprsDestory(struct aeEventLoop* eventLoop, mNetworker* ao) {
    //停止客户端监听
	if (ao->fd > 0) {
		aeDeleteFileEvent(eventLoop, ao->fd, AE_READABLE);
		close(ao->fd);
		ao->fd = 0;
	}

	//停止相应服务器反向链接
	if (ao->listenPort > 0) {
		aeDeleteFileEvent(eventLoop, ao->listenPort, AE_READABLE);
		close(ao->listenPort);
		ao->listenPort = 0;
	}

	//停止服务端监听
	if (ao->serverPort > 0) {
		aeDeleteFileEvent(eventLoop, ao->serverPort, AE_READABLE);
		close(ao->serverPort);
		ao->serverPort = 0;
	}

    //状态机复位
    ao->step      = 0;
    ao->rev_delay = 0;
    ao->rev_tail  = 0;
    ao->rev_head  = 0;
    bzero(ao->NetRevBuf, sizeof(ao->NetRevBuf));
    bzero(ao->NetSendBuf, sizeof(ao->NetSendBuf));

	//清理拨号数据
	if (ao->originSerial > 0) {
		aeDeleteFileEvent(eventLoop, ao->originSerial, AE_READABLE);
		close(ao->originSerial);
		ao->originSerial = 0;
	}
	if (ao->sMux0 > 0) {
		aeDeleteFileEvent(eventLoop, ao->sMux0, AE_READABLE);
		close(ao->sMux0);
		ao->sMux0 = 0;
	}

	if (ao->sMux1 > 0) {
		aeDeleteFileEvent(eventLoop, ao->sMux1, AE_READABLE);
		close(ao->sMux1);
		ao->sMux1 = 0;
	}

    bzero(ao->atRecBuf, sizeof(ao->atRecBuf));

    ao->ConstructStep = PowerOnStep;
    ao->sendRetry = 0;
}

void gdw3761Dealer(int fd, int revcount, mNetworker* net){

    //调用read接口读取其中的数据
    int len, j = 0;
    for (j = 0; j < revcount; j++) {
        len = anetRead(fd, net->NetRevBuf + net->rev_head, 1);
        net->rev_head += len;
        net->rev_head %= FrameSize;
    }

	////////////////////////////////////////////
    char printbuf[2048];
    memset(printbuf, 0, 2048);
	int i = 0, count = 0;
	for (i = net->rev_tail; i != net->rev_head; (i++) % FrameSize) {
		printbuf[i] = net->NetRevBuf[i];
		count ++;
	}
    DbPrt1("Recv:", printbuf, count, NULL);
	////////////////////////////////////////////

	//调用3761状态机
	int k, deallen = 0;
	for (k = 0; k < 3; k++) {
		deallen = gdw3761_preprocess(&net->step, &net->rev_delay, 10,
				&net->rev_tail, &net->rev_head, net->NetRevBuf, net->DealBuf);
		if (deallen > 0) {
			break;
		}
	}

	if (deallen > 0) {
		net->address_A3 = net->DealBuf[11];
		gdw3761_setCallback(netWrite);
		int ret = gdw3761_parse(net->DealBuf, deallen, NULL);
		if (ret > 0) {
			fprintf(stderr, "[vMsgr]主站响应 [AFN %02x]\n", ret);
		}
	}
}

INT8S netWrite(INT8U* buf, INT16U len) {
	static int id = 1;

	//留下余地
	if (len > NODEBUFSIZE - 24) {
		fprintf(stderr, "[vMsgr]发送消息长度超过%d,丢弃消息。\n", NODEBUFSIZE - 24);
		return -1;
	}

	mNetworker* g = getNetStruct();
	SendNode * msg = malloc(sizeof(SendNode));
	bzero(msg, sizeof(SendNode));
	msg->len = len;
	msg->id = id++;

	memcpy(msg->buf, buf, len);

	//队列不要存储过多报文。
	if (listLength(g->mlist) > 5){
		listDelNode(g->mlist, listFirst(g->mlist));
	}

	g->mlist = listAddNodeTail(g->mlist, msg);
}

void netAccept(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask) {
    mNetworker* mo = (mNetworker*)clientData;

    if (mo->serverPort > 0){
    	 aeDeleteFileEvent(eventLoop, mo->serverPort, AE_READABLE);
    	 close(mo->serverPort);
    	 mo->serverPort = -1;
    }
    mo->serverPort = anetTcpAccept(NULL, mo->listenPort, NULL, 0, NULL);
    if (mo->serverPort > 0) {
        fprintf(stderr, "[vmsgr] 建立主站反向链接。fd = %d\n", mo->serverPort);
        if (aeCreateFileEvent(eventLoop, mo->serverPort, AE_READABLE, serRead, mo) == -1){
        	close(mo->serverPort);
        	mo->serverPort = -1;
        }
    } else {
        //网络监听出现异常，重置整体模块
        gprsDestory(eventLoop, mo);
    }
}

void netMixRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask) {
    mNetworker* net = (mNetworker*)clientData;

    //判断fd中有多少需要接收的数据
    int revcount = 0;
    ioctl(fd, FIONREAD, &revcount);

    //关闭异常端口，如果没有服务模式在运行，就重置整体模块
    if (revcount == 0) {
        if (net->listenPort <= 0) {
            gprsDestory(eventLoop, net);
        }
        aeDeleteFileEvent(eventLoop, fd, AE_READABLE);
        close(fd);
        net->fd = 0;
        return;
    }

    gdw3761Dealer(fd, revcount, net);
}

void netClientRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask) {
    mNetworker* net = (mNetworker*)clientData;

    //判断fd中有多少需要接收的数据
    int revcount = 0;
    ioctl(fd, FIONREAD, &revcount);

    //关闭异常端口，重置整体模块
    if (revcount == 0) {
        gprsDestory(eventLoop, net);
        return;
    }

    gdw3761Dealer(fd, revcount, net);
}

void serRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask) {
    mNetworker* net = (mNetworker*)clientData;

    //判断fd中有多少需要接收的数据
    int revcount = 0;
    ioctl(fd, FIONREAD, &revcount);

    //关闭异常端口，如果没有服务模式在运行，就重置整体模块
    if (revcount == 0) {
        aeDeleteFileEvent(eventLoop, fd, AE_READABLE);
        close(fd);
        net->serverPort = 0;
    }

    gdw3761Dealer(fd, revcount, net);
}


void netModemRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask) {
    mNetworker* net = (mNetworker*)clientData;

    int position = 0, chnnal = 0, length = 0;
    bzero(net->atRecBuf, RES_LENGTH);
    int resLen         = read(fd, net->atRecBuf, RES_LENGTH);

    fprintf(stderr, "[vMsgr][netModemRead]\n%s=\n", net->atRecBuf);


    if(findStr(net->atRecBuf, "$MYNETWRITE:", resLen) != -1){
    	SendNode * msg = (SendNode *)listFirst(net->mlist)->value;

    	write(net->originSerial, msg->buf, msg->len);
    	listDelNode(net->mlist, listFirst(net->mlist));
    }

    if(findStr(net->atRecBuf, "$MYURCREAD:", resLen) != -1){
    	write(net->originSerial, "\rAT$MYNETREAD=1,1024\r", strlen("\rAT$MYNETREAD=1,1024\r"));
    }


    if((position = findStr(net->atRecBuf, "$MYNETREAD:", resLen)) != -1){
    	if(sscanf(&net->atRecBuf[position], "$MYNETREAD:%d,%d", &chnnal, &length) == 2){
    		fprintf(stderr, "[vMsgr] channal %d, get data %d\n", chnnal, length);
    		int i = 0, head = 0;
    		head = findStr(net->atRecBuf, "h", resLen);
    		for (i = 0; i < length; i ++){
    	        net->NetRevBuf[net->rev_head] = net->atRecBuf[head+i];
    	        net->rev_head += 1;
    	        net->rev_head %= FrameSize;
    		}
    	    gdw3761Dealer(0, 0, net);
    	}
    	else{
    		fprintf(stderr, "[vMsgr]无法解析AT报文。\n");
    	}
    }
}
