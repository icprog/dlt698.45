#include "gprs.h"

int gpifun(char* devname) {
    int data = 0, fd = 0;
    if ((fd = open(devname, O_RDWR | O_NDELAY)) > 0) {
        read(fd, &data, sizeof(int));
        close(fd);
    }
    return data;
}

int gpofun(char* devname, int data) {
    int fd = -1;
    if ((fd = open(devname, O_RDWR | O_NDELAY)) >= 0) {
        write(fd, &data, sizeof(int));
        close(fd);
        return 1;
    }
    return 0;
}

int OpenMuxCom(INT8U port, int baud, unsigned char* par, unsigned char stopb, INT8U bits) {
    int Com_Port = 0;
    struct termios old_termi, new_termi;
    int baud_lnx = 0;
    unsigned char tmp[128];
    memset(tmp, 0, 128);
    // sprintf((char *)tmp,"/dev/ttyS%d",port);
    sprintf((char*)tmp, "/dev/mux%d", port);

    //	sprintf((char *)tmp,"%s%d",SERDEVNAME,port);
    Com_Port = open((char*)tmp, O_RDWR | O_NOCTTY); /* 打开串口文件 */
    if (Com_Port < 0) {
        fprintf(stderr, "open the serial port fail! errno is: %d\n", errno);
        return -1; /*打开串口失败*/
    }
    if (tcgetattr(Com_Port, &old_termi) != 0) /*存储原来的设置*/
    {
        fprintf(stderr, "get the terminal parameter error when set baudrate! errno is: %d\n", errno);
        /*获取终端相关参数时出错*/
        return -1;
    }
    // printf("\n\r c_ispeed == %d old_termi  c_ospeed == %d",old_termi.c_ispeed, old_termi.c_ospeed);
    bzero(&new_termi, sizeof(new_termi));          /*将结构体清零*/
    new_termi.c_cflag |= (CLOCAL | CREAD);         /*忽略调制解调器状态行，接收使能*/
    new_termi.c_lflag &= ~(ICANON | ECHO | ECHOE); /*选择为原始输入模式*/
    new_termi.c_oflag &= ~OPOST;                   /*选择为原始输出模式*/
    new_termi.c_cc[VTIME] = 2;                     /*设置超时时间为0.5 s*/
    new_termi.c_cc[VMIN]  = 0;                     /*最少返回的字节数是 7*/
    new_termi.c_cflag &= ~CSIZE;
    // new_termi.c_iflag &= ~INPCK;     /* Enable parity checking */
    new_termi.c_iflag &= ~ISTRIP;
    switch (baud) {
        case 1200:
            baud_lnx = B1200;
            break;
        case 2400:
            baud_lnx = B2400;
            break;
        case 4800:
            baud_lnx = B4800;
            break;
        case 9600:
            baud_lnx = B9600;
            break;
        case 19200:
            baud_lnx = B19200;
            break;
        case 38400:
            baud_lnx = B38400;
            break;
        case 57600:
            baud_lnx = B57600;
            break;
        case 115200:
            baud_lnx = B115200;
            break;
        default:
            baud_lnx = B9600;
            fprintf(stderr, "\nSerial COM%d do not setup baud, default baud is 9600!!!", port);
            break;
    }

    switch (bits) {
        case 5:
            new_termi.c_cflag |= CS5;
            break;
        case 6:
            new_termi.c_cflag |= CS6;
            break;
        case 7:
            new_termi.c_cflag |= CS7;
            break;
        case 8:
            new_termi.c_cflag |= CS8;
            break;
        default:
            new_termi.c_cflag |= CS8;
            break;
    }

    if (strncmp((char*)par, "even", 4) == 0) //设置奇偶校验为偶校验
    {
        // new_termi.c_iflag |= (INPCK | ISTRIP);
        new_termi.c_cflag |= PARENB;
        new_termi.c_cflag &= ~PARODD;

    } else if (strncmp((char*)par, "odd", 3) == 0) //设置奇偶校验为奇校验
    {
        new_termi.c_cflag |= PARENB;
        new_termi.c_cflag |= PARODD;
        // new_termi.c_iflag |= (INPCK | ISTRIP);
    } else {
        new_termi.c_cflag &= ~PARENB; //设置奇偶校验为无校验
                                      // new_termi.c_iflag &=~ ISTRIP;
    }

    if (stopb == 1) //停止位
    {
        new_termi.c_cflag &= ~CSTOPB; //设置停止位为:一位停止位
    } else if (stopb == 2) {
        new_termi.c_cflag |= CSTOPB; //设置停止位为:二位停止位
    } else {
        new_termi.c_cflag &= ~CSTOPB; //设置停止位为:一位停止位
    }

    cfsetispeed(&new_termi, baud_lnx); /* 设置输入拨特率 */
    cfsetospeed(&new_termi, baud_lnx); /* 设置输出拨特率 */

    tcflush(Com_Port, TCIOFLUSH);                      /* 刷新输入输出流 */
    if (tcsetattr(Com_Port, TCSANOW, &new_termi) != 0) /* 激活串口配置 */
    {
        fprintf(stderr, "Set serial port parameter error!\n"); // close(Com_Port);
        return 0;
    }
    return Com_Port;
}

int SendATCommand(char* buf, int len, int com) {
    return write(com, buf, len);
}

//停止ftp监听，停止串口复用
void serialMuxShutDown(void) {
    system("pkill ftpget");
    system("ppp-off");
    system("pkill gsmMuxd");
}

//查看拨号程序是否获取到ip地址
int tryifconfig() {
    int sock;
    struct sockaddr_in sin;
    struct ifreq ifr;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        return -1;
    }
    strncpy(ifr.ifr_name, "ppp0", IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;
    if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
        close(sock);
        return -1;
    }
    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
    if (sin.sin_addr.s_addr > 0) {
        fprintf(stderr, "[vMsgr][tryifconfig]获取到正确的IP地址%s\n", inet_ntoa(sin.sin_addr));
        close(sock);
        return 1;
    }
    close(sock);
    return 0;
}

int ModemConstruct(struct aeEventLoop* ep, long long id, void* clientData) {
    mNetworker* go = (mNetworker*)clientData;

    int fd_state = 0;

    if (go->ConstructStep < 99){
    	fprintf(stderr, "[vMsgr]拨号流程-步骤 %d\n", go->ConstructStep);
    }

    switch (go->ConstructStep) {
        //模块上电
        case PowerOnStep:
            serialMuxShutDown();
        case 1:
            gpofun("/dev/gpoGPRS_POWER", 1);
            go->ConstructStep = 2;
            return 3 * 1000;
        case 2:
            gpofun("/dev/gpoGPRS_POWER", 0);
            go->ConstructStep = 3;
            return 3 * 1000;
        case 3:
            gpofun("/dev/gpoGPRS_POWER", 1);
            go->ConstructStep = ModemSwtStep;
            return 3 * 1000;

        //模块开关打开
        case ModemSwtStep:
        case 11:
            gpofun("/dev/gpoGPRS_SWITCH", 1);
            go->ConstructStep = 12;
            return 100;
        case 12:
            gpofun("/dev/gpoGPRS_SWITCH", 0);
            go->ConstructStep = 13;
            return 1 * 1000;
        case 13:
            gpofun("/dev/gpoGPRS_SWITCH", 1);
            go->ConstructStep = ModemRstStep;
            return 500;

        //模块状态重置
        case ModemRstStep:
        case 21:
            gpofun("/dev/gpoGPRS_RST", 1);
            go->ConstructStep = 22;
            return 1 * 1000;
        case 22:
            gpofun("/dev/gpoGPRS_RST", 0);
            go->ConstructStep = 23;
            return 1 * 1000;
        case 23:
            gpofun("/dev/gpoGPRS_RST", 1);
            go->ConstructStep = AtCheckStep;
            return 1 * 1000;

        case AtCheckStep:
        case 31:
            go->originSerial = com_open(0, MODEM_BAUND, (unsigned char*)"none", 1, 8);
            if (go->originSerial < 0) {
            	fprintf(stderr, "[vMsgr]AT检查失败(端口创建失败)。\n");
                //模块通信串口打开失败，2秒之后模块重启
                go->ConstructStep = PowerOnStep;
                return 2 * 1000;
            } else {
                //建立文件事件，监听串口回复AT命令
                aeCreateFileEvent(ep, go->originSerial, AE_READABLE, atCheckRead, go);
                SendATCommand("AT\r", strlen("AT\r"), go->originSerial);
                //允许重试
                go->retry         = 5;
                go->ConstructStep = 32;
                return 3 * 1000;
            }
        case 32:
            if (go->retry > 0) {
                go->retry--;
                fprintf(stderr, "[vMsgr] 再次发送AT检查命令。\n");
                SendATCommand("AT\r", strlen("AT\r"), go->originSerial);
                return 3 * 1000;
            } else {
            	fprintf(stderr, "[vMsgr] AT检查失败(未检测到AT-OK)。\n");
                close(go->originSerial);
                //模块at测试失败，2秒之后模块重启
                go->ConstructStep = PowerOnStep;
                aeDeleteFileEvent(ep, go->originSerial, AE_READABLE);
                close(go->originSerial);
                return 2 * 1000;
            }
        case MuxCheckStep:
        case 41:
            system("mux.sh &");
            go->retry         = 3;
            go->ConstructStep = 42;
            return 2 * 1000;
        case 42:
            go->sMux0 = OpenMuxCom(0, MODEM_BAUND, (unsigned char*)"none", 1, 8);
            go->sMux1 = OpenMuxCom(1, MODEM_BAUND, (unsigned char*)"none", 1, 8);

            if (go->sMux0 < 0 || go->sMux1 < 0) {
                close(go->sMux0);
                close(go->sMux1);

                //串口复用失败,关闭复用串口，取消复用进程，4秒后重新复用
                serialMuxShutDown();

                if (go->retry > 0) {
                    go->retry--;

                    go->ConstructStep = MuxCheckStep;
                    return 4 * 1000;
                } else {
                    //模块重启
                    go->ConstructStep = PowerOnStep;
                    return 2 * 1000;
                }
            } else {
                go->ConstructStep = modemCheckStep;
                return 10;
            }
        case modemCheckStep:
        case 51:
            aeCreateFileEvent(ep, go->sMux0, AE_READABLE, atModemRead, go);
            SendATCommand("AT$MYGMR\r", strlen("AT$MYGMR\r"), go->sMux0);
            // 30秒内必须拨号成功，否则重启模块
            go->retry         = 90;
            go->ConstructStep = 52;
            return 1000;
        case 52:
            if (go->retry > 0) {
                go->retry--;
                return 1000;
            } else {
                //模块重启
                close(go->sMux0);
                close(go->sMux1);
                aeDeleteFileEvent(ep, go->sMux0, AE_READABLE);
                go->ConstructStep = PowerOnStep;
                return 2 * 1000;
            }

        case DailStep:
        case 61:
            //拨号前需要关闭串口
            close(go->sMux0);
            if (go->netType == NET_GPRS) {
                system("pppd call gprs &");
            } else {
                system("pppd call cdma2000 &");
            }
            go->retry         = 20;
            go->ConstructStep = ipCheckStep;
            return 5 * 1000;

        case ipCheckStep:
        case 71:
            if (tryifconfig() == 1) {
                go->ConstructStep = FinallyStep;
                return 1 * 1000;
            } else {
                if (go->retry > 0) {
                    go->retry--;
                    return 1 * 1000;
                } else {
                    //模块重启
                    close(go->sMux0);
                    close(go->sMux1);
                    aeDeleteFileEvent(ep, go->sMux0, AE_READABLE);
                    go->ConstructStep = PowerOnStep;
                    return 2 * 1000;
                }
            }

        case FinallyStep:
        	if(tryifconfig() != 1){
        		//拨号连接丢失IP，模块重启
        		fprintf(stderr, "[vMsgr]拨号连接丢失IP，模块重启\n");
        		gprsDestory(ep, go);
        		return 1 * 1000;
        	}
           return 5 * 60 * 1000;
        case CleanStep:
            gpofun("/dev/gpoGPRS_POWER", 0);
            go->ConstructStep = StandByStep;
            return 2 * 1000;
        case StandByStep:
            return 5 * 1000;
        case ShutDown:
        	return 5 * 1000;
    }
}

int ModemOnLine(struct aeEventLoop* ep, long long id, void* clientData) {
    mNetworker* go = (mNetworker*)clientData;

    if (go->ConstructStep < 99){
    	fprintf(stderr, "[vMsgr]拨号流程-步骤 %d\n", go->ConstructStep);
    }

    switch (go->ConstructStep) {
        case PowerOnStep:
            serialMuxShutDown();
        case 1:
            gpofun("/dev/gpoGPRS_POWER", 1);
            go->ConstructStep = 2;
            return 3 * 1000;
        case 2:
            gpofun("/dev/gpoGPRS_POWER", 0);
            go->ConstructStep = 3;
            return 3 * 1000;
        case 3:
            gpofun("/dev/gpoGPRS_POWER", 1);
            go->ConstructStep = ModemSwtStep;
            return 3 * 1000;

        case ModemSwtStep:
        case 11:
            gpofun("/dev/gpoGPRS_SWITCH", 1);
            go->ConstructStep = 12;
            return 100;
        case 12:
            gpofun("/dev/gpoGPRS_SWITCH", 0);
            go->ConstructStep = 13;
            return 1 * 1000;
        case 13:
            gpofun("/dev/gpoGPRS_SWITCH", 1);
            go->ConstructStep = ModemRstStep;
            return 500;

        case ModemRstStep:
        case 21:
            gpofun("/dev/gpoGPRS_RST", 1);
            go->ConstructStep = 22;
            return 1 * 1000;
        case 22:
            gpofun("/dev/gpoGPRS_RST", 0);
            go->ConstructStep = 23;
            return 1 * 1000;
        case 23:
            gpofun("/dev/gpoGPRS_RST", 1);
            go->ConstructStep = AtCheckStep;
            return 1 * 1000;

        case AtCheckStep:
        case 31:
            go->originSerial = com_open(0, MODEM_BAUND, (unsigned char*)"none", 1, 8);

            if (go->originSerial <= 0) {
                fprintf(stderr, "[vMsgr] AT检查失败(端口创建失败)。\n");
                gprsDestory(ep, go);
                go->ConstructStep = PowerOnStep;
                return 2 * 1000;
            } else {
            	printf("Check =====\n");
                //建立文件事件，监听串口回复AT命令
                aeCreateFileEvent(ep, go->originSerial, AE_READABLE, atCheckRead, go);

                while(1)
                {
                	char buf[1024];
                	memset(buf, 0, 1024);
                	printf("\n>>>");
                	gets(buf);
                	if(buf[0] == 'q'){
                		break;
                	}
                	buf[strlen(buf)] = '\r';
                	buf[strlen(buf)] = '\n';
                	printf("\n=(%d)\n%s\n=\n", strlen(buf), buf);
                	SendATCommand("\r", 1, go->originSerial);
                	SendATCommand(buf, strlen(buf), go->originSerial);
                	sleep(10);
                	memset(go->atRecBuf, 0, RES_LENGTH);
                    int resLen = read(go->originSerial, go->atRecBuf, RES_LENGTH);

                    ///////////////////////////////////////////
                    fprintf(stderr, "[atCheckRead(%d)]\n%s=\n", resLen, go->atRecBuf);

                }
                gpofun("/dev/gpoGPRS_POWER", 0);
                exit(1);

                go->retry         = 90;
                go->ConstructStep = 32;
                return 10 * 1000;
            }
        case 32:
            if (go->retry > 0) {
                go->retry--;
                //fprintf(stderr, "[vMsgr] 再次发送AT检查命令。\n");
                //SendATCommand("AT\r", strlen("AT\r"), go->originSerial);
                //SendATCommand("at+qnvfr=\"safe/scrub/master_info.bin\"\r", strlen("at+qnvfr=\"safe/scrub/master_info.bin\"\r"), go->originSerial);
                //SendATCommand("AT+CFUN=4\r", strlen("AT+CFUN=4\r"), go->originSerial);
                return 10 * 1000;
            } else {
            	fprintf(stderr, "[vMsgr] AT检查失败(未检测到AT-OK)。\n");
                gprsDestory(ep, go);
                go->ConstructStep = PowerOnStep;
                return 2 * 1000;
            }
        case MuxCheckStep:
        case 51:
            aeCreateFileEvent(ep, go->originSerial, AE_READABLE, atModemOnLine, go);
            SendATCommand("AT$MYGMR\r", strlen("AT$MYGMR\r"), go->originSerial);
            go->retry         = 90;
            go->ConstructStep = 52;
            return 1000;
        case 52:
            if (go->retry > 0) {
                go->retry--;
                return 1000;
            } else {
                gprsDestory(ep, go);
                go->ConstructStep = PowerOnStep;
                return 2 * 1000;
            }

        case FinallyStep:
            aeCreateFileEvent(ep, go->originSerial, AE_READABLE, netModemRead, go);
            gdw3761_setCallback(netWrite);
            gdw3761_link(1);
            go->ConstructStep = StandByStep;
            return 1 * 1000;
        case CleanStep:
            gpofun("/dev/gpoGPRS_POWER", 0);
            go->ConstructStep = StandByStep;
            return 2 * 1000;
        case StandByStep:
            return 5 * 1000;
        case ShutDown:
        	return 5 * 1000;
    }
}

void atCheckRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask) {
    mNetworker* go = (mNetworker*)clientData;
    int resLen     = 0;
    resLen         = read(go->originSerial, go->atRecBuf, RES_LENGTH);
    if (resLen <= 0) {
        goto clean;
    }

    ///////////////////////////////////////////
    fprintf(stderr, "[atCheckRead]\n%s=\n", go->atRecBuf);
    ///////////////////////////////////////////

    int i = 0;
    for (i = 1; i < resLen; i++) {
        if (go->atRecBuf[i - 1] == 'O' && go->atRecBuf[i] == 'K') {
            //检查完毕，设定ATOK状态
        	SendATCommand("at+qnvfw=\"safe/scrub/master_info.bin\",0100000073637275624D2D490000000000000000FFFFFFFFFFFFFF000000000000000000000000000000000000000000000000000000000000000000\r", strlen("at+qnvfw=\"safe/scrub/master_info.bin\",0100000073637275624D2D490000000000000000FFFFFFFFFFFFFF000000000000000000000000000000000000000000000000000000000000000000\r"), go->originSerial);
        	//sleep(5);
        	//SendATCommand("at+qnvfW=\"safe/scrub/master_cfg.bin\",0100000073637275624D2D4300BD510000069825803A090000000000\r", strlen("at+qnvfW=\"safe/scrub/master_cfg.bin\",0100000073637275624D2D4300BD510000069825803A090000000000\r"), go->originSerial);
            go->step = 40;
        	goto clean;
        }
    }
    //返回的值中没有找到OK字符，返回等待下一个AT信息到来
    return;

clean:
    aeDeleteFileEvent(eventLoop, go->originSerial, AE_READABLE);
}

int findStr(char* Des, char* target, int len) {
    if (strlen(target) > len) {
        return -1;
    }
    int i = 0;
    for (i = 0; i < len - strlen(target); i++) {
        if (0 == strncmp(&Des[i], target, strlen(target))) {
            return i;
        }
    }
    return -1;
}

void atModemRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask) {
    mNetworker* go = (mNetworker*)clientData;

    int i        = 0;
    int resLen   = 0;
    int position = 0;
    bzero(go->atRecBuf, RES_LENGTH);
    resLen = read(go->sMux0, go->atRecBuf, RES_LENGTH);

    if (resLen <= 0) {
        aeDeleteFileEvent(eventLoop, go->sMux0, AE_READABLE);
    }
    fprintf(stderr, "[vMsgr][atModemRead]\n%s=\n", go->atRecBuf);

    for (i = 0; i < resLen; i++) {
        if (go->atRecBuf[i] == 'E' && go->atRecBuf[i + 1] == 'R' && go->atRecBuf[i + 2] == 'R' && go->atRecBuf[i + 3] == 'O' &&
            go->atRecBuf[i + 4] == 'R' && go->atRecBuf[i + 5] == ':') {
        	fprintf(stderr, "[vMsgr][AT-ERROR] error = %d\n", go->atRecBuf[i + 6]);
            return;
        }

        if (findStr(go->atRecBuf, "$MYGMR", resLen) != -1) {
            SendATCommand("AT$MYTYPE?\r", strlen("AT$MYTYPE?\r"), go->sMux0);
            return;
        }

        if (findStr(go->atRecBuf, "$MYTYPE:", resLen) != -1) {
            int k, l, m;
            if (sscanf(&go->atRecBuf[i], "$MYTYPE:%d,%d,%d", &k, &l, &m) == 3) {
                if ((l & 0x01) == 1) {
                    go->netType = NET_GPRS;
                }
                if ((l & 0x08) == 8) {
                    go->netType = NET_CDMA2000;
                }

                SendATCommand("AT$MYCCID\r", strlen("AT$MYCCID\r"), go->sMux0);
                return;
            }
        }

        if (findStr(go->atRecBuf, "$MYCCID:", resLen) != -1) {
            SendATCommand("AT+CIMI\r", strlen("AT+CIMI\r"), go->sMux0);
            return;
        }

        if (findStr(go->atRecBuf, "+CIMI", resLen) != -1) {
            SendATCommand("AT+CSQ\r", strlen("AT+CSQ\r"), go->sMux0);
            return;
        }

        if ((position = findStr(go->atRecBuf, "+CSQ:", resLen)) != -1) {
            int k, l;
            if (sscanf(&go->atRecBuf[position], "+CSQ:%d,%d", &k, &l) == 2) {
                if (k != 99) {
                    SendATCommand("AT+CREG?\r", strlen("AT+CREG?\r"), go->sMux0);
                    return;
                }
            }
            SendATCommand("AT+CSQ\r", strlen("AT+CSQ\r"), go->sMux0);
            return;
        }

        if ((position = findStr(go->atRecBuf, "+CREG:", resLen)) != -1) {
            int k, l;
            if (sscanf(&go->atRecBuf[position], "+CREG:%d,%d", &k, &l) == 2) {
                if (l == 1 || l == 5) {
                    aeDeleteFileEvent(eventLoop, go->sMux0, AE_READABLE);
                    close(go->sMux0);
                    // AT正确，进入下一拨号流程
                    go->ConstructStep = DailStep;
                } else {
                    //发生错误，重新发送
                    SendATCommand("AT+CREG?\r", strlen("AT+CREG?\r"), go->sMux0);
                }
            }
            return;
        }
    }
}

void atModemOnLine(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask) {
    mNetworker* go = (mNetworker*)clientData;

    int i        = 0;
    int resLen   = 0;
    int position = 0;
    bzero(go->atRecBuf, RES_LENGTH);
    usleep(100 * 1000);
    resLen = read(fd, go->atRecBuf, RES_LENGTH);

    if (resLen <= 0) {
        aeDeleteFileEvent(eventLoop, fd, AE_READABLE);
    }
    fprintf(stderr, "[vMsgr][atModemOnLine]\n%s=\n", go->atRecBuf);

    for (i = 0; i < resLen; i++) {
        if (go->atRecBuf[i] == 'E' && go->atRecBuf[i + 1] == 'R' && go->atRecBuf[i + 2] == 'R' && go->atRecBuf[i + 3] == 'O' &&
            go->atRecBuf[i + 4] == 'R' && go->atRecBuf[i + 5] == ':') {
        	fprintf(stderr, "[vMsgr][AT-ERROR] error = %d\n", go->atRecBuf[i + 6]);
            return;
        }

        if (findStr(go->atRecBuf, "$MYGMR", resLen) != -1) {
            SendATCommand("AT$MYTYPE?\r", strlen("AT$MYTYPE?\r"), fd);
            return;
        }

        if ((position = findStr(go->atRecBuf, "$MYTYPE:", resLen)) != -1) {
            int k, l, m;
            if (sscanf(&go->atRecBuf[i + 22], "%d,%x,%d", &k, &l, &m) == 3) {
                if ((l & 0x01) == 1) {
                    go->netType = NET_GPRS;
                }
                if ((l & 0x08) == 8) {
                    go->netType = NET_CDMA2000;
                }
                SendATCommand("AT$MYCCID\r", strlen("AT$MYCCID\r"), fd);
                return;
            } else {
                SendATCommand("AT$MYTYPE?\r", strlen("AT$MYTYPE?\r"), fd);
                return;
            }
        }

        if (findStr(go->atRecBuf, "$MYCCID:", resLen) != -1) {
            SendATCommand("AT+CIMI\r", strlen("AT+CIMI\r"), fd);
            return;
        }

        if (findStr(go->atRecBuf, "+CIMI", resLen) != -1) {
            SendATCommand("AT+CSQ\r", strlen("AT+CSQ\r"), fd);
            return;
        }

        if ((position = findStr(go->atRecBuf, "+CSQ:", resLen)) != -1) {
            int k, l;
            if (sscanf(&go->atRecBuf[position], "+CSQ: %d,%d", &k, &l) == 2) {
                if (k != 99) {
                    SendATCommand("AT+CREG?\r", strlen("AT+CREG?\r"), fd);
                    return;
                }
            }
            SendATCommand("AT+CSQ\r", strlen("AT+CSQ\r"), fd);
            return;
        }

        if ((position = findStr(go->atRecBuf, "+CREG:", resLen)) != -1) {
			int k, l;
			if (sscanf(&go->atRecBuf[position], "+CREG: %d,%d", &k, &l) == 2) {
				int len = SendATCommand("\rAT$MYNETCON=0,contype,GPRS\r",
						strlen("\rAT$MYNETCON=0,contype,GPRS\r"), fd);
			} else {
				SendATCommand("AT+CREG?\r", strlen("AT+CREG?\r"), fd);
			}
			return;
        }

        if ((position = findStr(go->atRecBuf, "AT$MYNETCON=0,contype,GPRS\r", resLen)) != -1) {
            if (findStr(go->atRecBuf, "ERROR", resLen) != -1) {
                SendATCommand("AT$MYNETCON=0,\"APN\",\"cmnet\"\r", strlen("AT$MYNETCON=0,\"APN\",\"cmnet\"\r"), fd);
            } else {
                SendATCommand("AT$MYNETCON=0,contype,GPRS\r", strlen("AT$MYNETCON=0,contype,GPRS\r"), fd);
            }
            return;
        }

        if ((position = findStr(go->atRecBuf, "AT$MYNETCON=0,\"APN\"", resLen)) != -1) {
            if (findStr(go->atRecBuf, "OK", resLen) != -1) {
                SendATCommand("AT$MYNETCON=0,\"USERPWD\",\"None,None\"\r", strlen("AT$MYNETCON=0,\"USERPWD\",\"None,None\"\r"), fd);
            } else {
                SendATCommand("AT$MYNETCON=0,\"APN\",\"cmnet\"\r", strlen("AT$MYNETCON=0,\"APN\",\"cmnet\"\r"), fd);
            }
            return;
        }
        if ((position = findStr(go->atRecBuf, "AT$MYNETCON=0,\"USERPWD\"", resLen)) != -1) {
            if (findStr(go->atRecBuf, "OK", resLen) != -1) {
                SendATCommand("AT$MYNETACT=0,1\r", strlen("AT$MYNETACT=0,1\r"), fd);
                sleep(5);
            } else {
                SendATCommand("AT$MYNETCON=0,\"USERPWD\",\"None,None\"\r", strlen("AT$MYNETCON=0,\"USERPWD\",\"None,None\"\r"), fd);
            }
            return;
        }

        if ((position = findStr(go->atRecBuf, "AT$MYNETACT=0,", resLen)) != -1) {
            if (findStr(go->atRecBuf, "OK", resLen) != -1) {
                SendATCommand("AT$MYNETSRV=0,1,0,0,\"59.111.100.243:7361\"\r",
                              strlen("AT$MYNETSRV=0,1,0,0,\"59.111.100.243:7361\"\r"), fd);
            } else {
                SendATCommand("AT$MYNETACT=0,1\r", strlen("AT$MYNETACT=0,1\r"), fd);
            }
            return;
        }

        if ((position = findStr(go->atRecBuf, "AT$MYNETSRV=0,1,0,0", resLen)) != -1) {
            if (findStr(go->atRecBuf, "OK", resLen) != -1) {
                SendATCommand("\rAT$MYNETOPEN=1\r", strlen("\rAT$MYNETOPEN=1\r"), fd);
            } else {
                SendATCommand("AT$MYNETSRV=0,1,0,0,\"119.180.24.156:7361\"\r",
                              strlen("AT$MYNETSRV=0,1,0,0,\"119.180.24.156:7361\"\r"), fd);
            }
            return;
        }

        if ((position = findStr(go->atRecBuf, "MYNETOPEN:", resLen)) != -1) {
            if (findStr(go->atRecBuf, "OK", resLen) != -1) {
                go->ConstructStep = FinallyStep;
                aeDeleteFileEvent(eventLoop, go->originSerial, AE_READABLE);
            } else {
                SendATCommand("\rAT$MYNETOPEN=1\r", strlen("\rAT$MYNETOPEN=1\r"), fd);
            }
            return;
        }
    }
}
