#include "ae.h"
#include "msgr.h"
#include "anet.h"
#include "../libhd/libhd.h"
#include "../include/gdm_para.h"
#include "../include/mmqdef.h"
#include "../include/gdm.h"
#include "../libmmq/libmmq.h"
#include "../include/shmdef.h"
#include "../lib3761/libgdw3761.h"
#include "handle.h"
#include <stdarg.h>
#include <mqueue.h>

//唯一的一个全局变量，包含本模块几乎所有的数据，设置为static属性保持，只有本文件内的函数可见。
static MSGR_ONLY_ONE aMsgr;

void DbgPrintToFile1(const char* format, ...) {
    // return;
    char str[50];
    time_t cur_time;
    struct tm cur_tm;
    FILE* fp = NULL;
    memset(str, 0, 50);
    cur_time = time(NULL);
    localtime_r(&cur_time, &cur_tm);
    sprintf(str, "\n[%04d-%02d-%02d %02d:%02d:%02d]", cur_tm.tm_year + 1900, cur_tm.tm_mon + 1, cur_tm.tm_mday, cur_tm.tm_hour,
            cur_tm.tm_min, cur_tm.tm_sec);
    va_list ap;
    if (!fp)
        fp = fopen("/nand/gprs.log", "a+");
    va_start(ap, format);
    vfprintf(fp, str, ap);
    if (fp)
        vfprintf(fp, format, ap);
    va_end(ap);
    fflush(fp);

    if (fp) {
        long int flen;
        fseek(fp, 0L, SEEK_END);
        flen = ftell(fp);
        if (flen > 300000) {
            fclose(fp);
            fp   = NULL;
            fp   = fopen("/nand/gprs.log", "w");
            flen = ftell(fp);
            fclose(fp);
            fp = fopen("/nand/gprs.log", "a+");
        }
        fclose(fp);
        fp = NULL;
    }
}


void myBCDtoASC1(char val, char dest[2]) {
    int i = 0;
    char c[2];
    c[0] = 0;
    c[1] = 0;
    c[0] = (val >> 4) & 0x0f;
    c[1] = val & 0x0f;
    for (i = 0; i < 2; i++) {
        // if(c[i]>=0 && c[i]<=9)
        if (c[i] <= 9)
            dest[i] = c[i] + '0';
        if (c[i] == 10)
            dest[i] = 'a';
        if (c[i] == 11)
            dest[i] = 'b';
        if (c[i] == 12)
            dest[i] = 'c';
        if (c[i] == 13)
            dest[i] = 'd';
        if (c[i] == 14)
            dest[i] = 'e';
        if (c[i] == 15)
            dest[i] = 'f';
    }
}

void DbPrt1(char* prefix, char* buf, int len, char* suffix) {
    // return ;
    char str[50], tmpbuf[2048], c[2], c1[2], c2[2];
    int i = 0;
    memset(c, 0, 2);
    memset(str, 0, 50);
    memset(tmpbuf, 0, 2048);

    int count  = 0;
    int prtlen = 0;
    int k      = 0;
    while (1) {
        memset(c, 0, 2);
        memset(str, 0, 50);
        memset(tmpbuf, 0, 2048);
        if (len <= 512) {
            prtlen = len;
        } else {
            if (k < len / 512) {
                k++;
                prtlen = 512;
            } else {
                prtlen = len % 512;
            }
        }
        if (prefix != NULL) {
            sprintf(str, "%s[%d] ", prefix, prtlen);
            strcat(tmpbuf, str);
        }
        for (i = 0; i < prtlen; i++) {
            memset(c, 0, 2);
            memset(c1, 0, 2);
            memset(c2, 0, 2);
            myBCDtoASC1(buf[i + count], c);

            c1[0] = c[0];
            c2[0] = c[1];
            strcat(tmpbuf, c1);
            strcat(tmpbuf, c2);
            strcat(tmpbuf, " ");
        }
        if (suffix != NULL)
            strcat(tmpbuf, suffix);
        DbgPrintToFile1(tmpbuf);
        count += prtlen;
        if (count >= len)
            break;
    }
}


void mAgentCreate(struct mAgent* ag, mAgentWrite* write, aeFileProc* read) {
    ag->fd        = 0;
    ag->step      = 0;
    ag->rev_delay = 0;
    ag->rev_tail  = 0;
    ag->rev_head  = 0;
    bzero(ag->NetRevBuf, sizeof(ag->NetRevBuf));
    bzero(ag->NetSendBuf, sizeof(ag->NetSendBuf));

    ag->lastReadTime  = 0;
    ag->lastWriteTime = 0;
    ag->ReadCount     = 0;
    ag->WriteCount    = 0;
    ag->onlineState   = 0;
    ag->retryDelay    = 0;
    ag->write         = write;
    ag->read          = read;
}

void mAgentDestory(aeEventLoop* eventLoop, mAgent* ag) {
    //不再监视此端口的事件
    aeDeleteFileEvent(eventLoop, ag->fd, AE_READABLE);
    //关闭设备端口
    close(ag->fd);
    // fd设置为0，方便timer检测到，然后尝试重新创建设备
    ag->fd          = 0;
    ag->onlineState = 0;

    //状态机复位
    ag->step      = 0;
    ag->rev_delay = 0;
    ag->rev_tail  = 0;
    ag->rev_head  = 0;
    bzero(ag->NetRevBuf, sizeof(ag->NetRevBuf));
    bzero(ag->NetSendBuf, sizeof(ag->NetSendBuf));
}

void mNetCreate(struct mNetworker* ag, mAgentWrite* write, aeFileProc* read) {
    ag->fd        = 0;
    ag->step      = 0;
    ag->rev_delay = 0;
    ag->rev_tail  = 0;
    ag->rev_head  = 0;
    bzero(ag->NetRevBuf, sizeof(ag->NetRevBuf));
    bzero(ag->NetSendBuf, sizeof(ag->NetSendBuf));

    ag->lastReadTime  = 0;
    ag->lastWriteTime = 0;
    ag->ReadCount     = 0;
    ag->WriteCount    = 0;
    ag->onlineState   = 0;
    ag->retryDelay    = 0;
    ag->write         = write;
    ag->read          = read;

    ag->sendRetry = 0;
}

int msgrCreate(MSGR* msgr, int mode) {
    mAgentCreate(&msgr->ifr, ifrWrite, ifrRead);
    mAgentCreate(&msgr->com, comWrite, comRead);
    mNetCreate(&msgr->net, NULL, NULL);

    aMsgr.ifr.Baud  = 2400;
    aMsgr.ifr.Bits  = 8;
    aMsgr.ifr.Stopb = 1;
    memset(&aMsgr.ifr.Parity, "even", strlen("even"));

    aMsgr.com.Baud  = 9600;
    aMsgr.com.Bits  = 8;
    aMsgr.com.Stopb = 1;
    memset(&aMsgr.com.Parity, "even", strlen("even"));

    msgr->net.MainPort = shmm_getpara()->f3.Port_MS;
    memcpy((void*)&msgr->net.MainIp[0], (void*)&shmm_getpara()->f3.IP_MS[0], 16);
    msgr->net.BackPort = shmm_getpara()->f3.Port1_MS;
    memcpy(&msgr->net.BackIp[0], &shmm_getpara()->f3.IP1_MS[0], 16);
    memcpy(&msgr->net.APN[0], &shmm_getpara()->f3.APN[0], sizeof(msgr->net.APN));
    msgr->net.HeartBeat = shmm_getpara()->f1.HeartInterval * 60;
    if (msgr->net.HeartBeat > 900) {
        msgr->net.HeartBeat = 900;
    }

    if (mode == ONLINE_MODEM) {
        msgr->net.abilitychs = ONLINE_MODEM;
    } else {
        msgr->net.abilitychs = ONLINE_STACK;
    }

    fprintf(stderr, "[vmsgr]程序以%s方式运行。", mode == ONLINE_MODEM ? "内部协议栈" : "外部协议栈");

    msgr->net.modeType   = shmm_getpara()->f8.Ter_mod;
    msgr->net.listenPort = shmm_getpara()->f7.Port_Listen;
    msgr->net.workType   = (shmm_getpara()->f8.WorkMode & 0b10000000) >> 7;

    msgr->net.listenPort    = -1;
    msgr->net.serverPort    = -1;
    msgr->net.mlist         = listCreate();
    listSetFreeMethod(msgr->net.mlist, free);

    msgr->mmq.fd            = 0;
    msgr->net.ConstructStep = 0;

    return 1;
}

void msgrDestory(struct aeEventLoop* ep) {
    //关闭串口复用进程
    serialMuxShutDown();

    //注销所有设备端口
    mAgentDestory(ep, &aMsgr.ifr);
    mAgentDestory(ep, &aMsgr.com);

    //关闭消息队列
    mmq_close((mqd_t)aMsgr.mmq.fd);
}

mAgent* getIfrStruct(void) {
    return &aMsgr.ifr;
}

mAgent* getComStruct(void) {
    return &aMsgr.com;
}

mNetworker* getNetStruct(void) {
    return &aMsgr.net;
}

mmqAgent* getMmqStruct(void) {
    return &aMsgr.mmq;
}

void mCheckIfrPort(struct aeEventLoop* ep, mAgent* ao) {
    if (ao->fd > 0) {
        return;
    }
    ao->fd = com_open(3, ao->Baud, ao->Parity, ao->Stopb, ao->Bits);
    if (ao->fd > 0) {
        aeCreateFileEvent(ep, ao->fd, AE_READABLE, ifrRead, ao);
    }
}

void mCheckComPort(struct aeEventLoop* ep, mAgent* ao) {
    if (ao->fd > 0) {
        return;
    }
    ao->fd = com_open(1, ao->Baud, ao->Parity, ao->Stopb, ao->Bits);
    if (ao->fd > 0) {
        aeCreateFileEvent(ep, ao->fd, AE_READABLE, comRead, ao);
    }
}

void mCheckNetPort(struct aeEventLoop* ep, mNetworker* ao) {
    //用于主备切换的标志位置
    static unsigned int flag = 0;

    fprintf(stderr, "[vMsgr][mCheckNetPort]ao->fd = %d, ao->listenPort = %d\n", ao->fd, ao->listenPort);

    //混合模式
    if (ao->modeType == TX_MIX_MODE) {
    	//集中器主动上送连接
		if (listFirst(ao->mlist) != NULL && ao->fd <= 0) {
		    if (flag++ % 2 == 0) {
		        ao->fd = anetTcpConnect(NULL, ao->MainIp, ao->MainPort);
		    } else {
		        ao->fd = anetTcpConnect(NULL, ao->BackIp, ao->BackPort);
		    }
		    if (ao->fd > 0){
		    	gdw3761_setCallback(netWrite);
				int res = aeCreateFileEvent(ep, ao->fd, AE_READABLE, netMixRead, ao);
				if (res < 0){
					close(ao->fd);
					ao->fd = -1;
				}
		    }
		}

		// 混合模式需要建立监听服务，等待主站链接
		if (ao->listenPort <= 0) {
			ao->listenPort = anetTcpServer(NULL, 5100, "0.0.0.0", 2);
			if (ao->listenPort <= 0){
				fprintf(stderr, "[vMsgr]监听服务绑定失败，无法正确建立监听。\n");
				return;
			}
			fprintf(stderr, "[vmsgr] 建立端口监听。fd = %d\n", ao->listenPort);
			aeCreateFileEvent(ep, ao->listenPort, AE_READABLE, netAccept, ao);
		}
    }else{
    	//客户端模式
        if (ao->fd <= 0) {
			if (flag++ % 2 == 0) {
				ao->fd = anetTcpConnect(NULL, ao->MainIp, ao->MainPort);
			} else {
				ao->fd = anetTcpConnect(NULL, ao->BackIp, ao->BackPort);
			}
			if (ao->fd > 0){
			    gdw3761_setCallback(netWrite);

				int res = aeCreateFileEvent(ep, ao->fd, AE_READABLE, netClientRead, ao);
				fprintf(stderr, "[vMsgr]aeCreateFileEvent result = %d\n", res);
				if (res < 0){
					close(ao->fd);
					ao->fd = -1;
				}else{
					while(listFirst(ao->mlist) != NULL){
						listDelNode(ao->mlist, listFirst(ao->mlist));
					}
					gdw3761_link(1);
				}
			}else{
				ao->fd = -1;
			}
        }
    }

    //以太网链接成功，停止gprs拨号过程
    if (ao->fd > 0){
		if (ao->ConstructStep != FinallyStep && ao->ConstructStep != ipCheckStep) {
			if (ao->ConstructStep != ShutDown){
				gprsShutDown(ep, ao);
			}
			shmm_getdevstat()->jzq_login = NET_COM;
		} else {
			shmm_getdevstat()->jzq_login = GPRS_COM;
		}
    }
}

void mCheckMmqPort(struct aeEventLoop* ep, mmqAgent* mo) {
    if (mo->fd > 0) {
        return;
    }
    mo->fd = mmq_open((INT8S*)NET_REV_MQ, &mo->attr, O_RDONLY);
    if (mo->fd > 0) {
        aeCreateFileEvent(ep, mo->fd, AE_READABLE, mmqRead, mo);
    }
}

INT8S gpio_readbyte(INT8S* devpath) {
    char data = 0;
    int fd    = 0;
    if ((fd = open((const char*)devpath, O_RDWR | O_NDELAY)) > 0) {
        read(fd, &data, sizeof(char));
        close(fd);
    } else
        return -1;
    return data;
}

int checkgprs_I_exist() {
    int gprsid = shmm_getdevstat()->GPRS_ID & 0x1f;
    if (gprsid == 0x1e) {
        return 1;
    }
    return 0;
}

int checkgprs_II_exist() {
    INT8S gprs_s0 = gpio_readbyte((INT8S*)"/dev/gpiGPRS_S0");
    INT8S gprs_s1 = gpio_readbyte((INT8S*)"/dev/gpiGPRS_S1");
    INT8S gprs_s2 = gpio_readbyte((INT8S*)"/dev/gpiGPRS_S2");
    if (gprs_s0 == 1 && gprs_s1 == 1 && gprs_s2 == 1) {
        return 0;
    }
    return 1;
}

int checkPower_I() {
    return pwr_has();
}

void checkModenSend(struct aeEventLoop* ep, mNetworker* ao){
	static unsigned int step = 0;
    static int last_id = 0;
    static int sendRetry = 0;

	if (ao->ConstructStep == StandByStep && step++ % 5 == 0) {
		if (listFirst(ao->mlist) != NULL) {
			SendNode * msg = (SendNode *)listFirst(ao->mlist)->value;
			fprintf(stderr, "[checkModenSend]SEND ID IS %d\n", msg->id);
			if(last_id == msg->id){
				if(sendRetry++ > 12){
					sendRetry = 0;
					gprsDestory(ep, ao);
				}
			}
			else{
				last_id = msg->id;
				sendRetry = 0;
			}

			bzero(ao->atComBuf, RES_LENGTH);
            sprintf(ao->atComBuf, "\rAT$MYNETWRITE=1,%d\r", msg->len);
            SendATCommand(ao->atComBuf, strlen(ao->atComBuf), ao->originSerial);
        }
    }
	else{
		last_id = 0;
	}
}

/*******************************************************
 * 函数说明：检查376.1规约PRM位是否为主动上送
 * 函数返回值：
 * *****************************************************/
int CheckPRM(INT8U* buf) {
    return ((buf[6] & 0x40) >> 6);
}

void checkStackSend(struct aeEventLoop* ep, mNetworker* ao) {
	int ret = 1;
	static int flag = 0;
	if (listFirst(ao->mlist) == NULL) {
		return;
	}

	SendNode * msg = (SendNode *) listFirst(ao->mlist)->value;
	fprintf(stderr, "[vMsgr][外部协议栈]Send ID %d [%d]\n", msg->id, msg->len);

	if (ao->modeType == TX_MIX_MODE) {
		if (CheckPRM(msg->buf) == 1) {
			if(ao->fd > 0){
				ret = anetWrite(ao->fd, msg->buf, msg->len);
				DbPrt1("[vMsgr][客户端发送]", msg->buf, msg->len, NULL);
				listDelNode(ao->mlist, listFirst(ao->mlist));
			}
		} else {
			if(ao->serverPort > 0){
				ret = anetWrite(ao->serverPort, msg->buf, msg->len);
				DbPrt1("[vMsgr][服务端发送]", msg->buf, msg->len, NULL);
				listDelNode(ao->mlist, listFirst(ao->mlist));
			}
		}
	} else {
		if(ao->fd > 0){
		ret = anetWrite(ao->fd, msg->buf, msg->len);
		if(ret == -1){
			fprintf(stderr, "[vMsgr]报文发送失败，网络重启\n");
			gprsDestory(ep, ao);
		}
	    DbPrt1("[vMsgr][客户端发送]", msg->buf, msg->len, NULL);
		listDelNode(ao->mlist, listFirst(ao->mlist));
		}
	}
}

/*
 * 此函数中：维护各个端口的状态、检查硬件、检查参数、发送心跳、自动上送信息等周期性工作
 */
int RegularCheck(struct aeEventLoop* ep, long long id, void* clientData) {
	// 1 设备正常 2 设备异常
	static int deviceState = 1;
	static int offline = 0;
    MSGR* m = (MSGR*)clientData;

    /*
     * 检查GPRS模块状态
     */
	if (deviceState == 1) {
		if (checkgprs_I_exist() != 1 ) {
			fprintf(stderr, "[vMsgr]检测到设备异常（未能检测到模块）\n");
			deviceState = 2;
			m->net.ConstructStep = CleanStep;
			gprsDestory(ep, &m->net);
		}
	}
	if (deviceState == 2) {
		if (checkgprs_I_exist() == 1) {
			fprintf(stderr, "[vMsgr]检测到设备恢复（检测到模块）\n");
			deviceState = 1;
			m->net.ConstructStep = PowerOnStep;
		}
	}

	/*
	 *	检查离线时间
	 */
	if (m->net.onlineState == 0){
		offline ++;
		//测试5分钟,现场5小时
		if (offline > 5 * 60 * 60){
			//清零，防止重复发送
			offline = 0;
			INT8U Fn = 1;
			fprintf(stderr, "[vMsgr]离线时间超时，发送重启命令\n");
			reset_tomsg(SAVE_REBOOT,&Fn,1);
		}
	}
	else{
		offline = 0;
	}

    mCheckIfrPort(ep, &m->ifr);
	mCheckComPort(ep, &m->com);
	mCheckMmqPort(ep, &m->mmq);
	mCheckNetPort(ep, &m->net);

//	if (m->net.abilitychs == ONLINE_MODEM) {
//	checkModenSend(ep, &m->net);

	checkStackSend(ep, &m->net);



    return 1 * 1000;
}

void QuitProcess() {
	fprintf(stderr, "[vMsgr]exit...\n");
    exit(0);
}

int main(int argc, char* argv[]) {
    // 查找重复的程序进程
    pid_t pids[128];
    int n = prog_find_pid_by_name((INT8S*)argv[0], pids);
    if (n > 1) {
        fprintf(stderr, "[vMsgr]检测到vmsgr程序有多个副本在运行，程序退出。\n");
        exit(0);
    }

    // 全局共享内存注册
    if (shmm_register() != 0) {
        fprintf(stderr, "[vMsgr]在vmsgr程序中注册内存失败，程序退出。\n");
        exit(0);
    }

    // 注册程序退出回调函数
    struct sigaction _sa;
    sig_set(&_sa, QuitProcess);

    // 初始化程序计数、函数回调、基本链接参数
    if (msgrCreate(&aMsgr, atoi(argv[1])) != 1) {
        fprintf(stderr, "[vMsgr]在vmsgr程序中初始化参数失败，程序退出。\n");
        exit(0);
    }

    gdw3761_register(vnet, netWrite, setpara_tomsg, sendacs_tomsg, read4851_tomsg, read4852_tomsg, readplc_tomsg, readcalc_tomsg,
                     reset_tomsg, setctrl_tomsg, vevent_tomsg);

    aeEventLoop* ep;
    ep = aeCreateEventLoop(128);
    if (ep == NULL) {
    	fprintf(stderr, "[vMsgr]事件循环创建失败，程序终止。\n");
    }

    if (aMsgr.net.abilitychs == ONLINE_STACK) {
        aeCreateTimeEvent(ep, 1 * 1000, ModemConstruct, &aMsgr.net, NULL);
    } else {
        aeCreateTimeEvent(ep, 1 * 1000, ModemOnLine, &aMsgr.net, NULL);
    }

    aeCreateTimeEvent(ep, 1 * 1000, RegularCheck, &aMsgr, NULL);
    aeMain(ep);
}
