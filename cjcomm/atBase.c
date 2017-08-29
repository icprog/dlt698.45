/*
 * atBase.c
 *
 *  Created on: 2017-7-20
 *      Author: zhoulihai
 */

#include <stdarg.h>

#include "db.h"
#include "cjcomm.h"
#include "atBase.h"
#include "helper.h"

static ATOBJ atObj;

int checkgprs_exist() {
	INT8S gprsid = 0;
	INT8S gprs_s0 = -1, gprs_s1 = -1, gprs_s2 = -1;

	gprsid = getSpiAnalogState() & 0x1f;
	if ((gprsid & 0x1f) == 0x1e) {
		asyslog(LOG_INFO, "有GPRS模块  %02x", gprsid);
		return 1;
	} else if (gprsid == -1) { // II型
		gprs_s0 = gpio_readbyte("DEV_GPRS_S0");
		gprs_s1 = gpio_readbyte("DEV_GPRS_S1");
		gprs_s2 = gpio_readbyte("DEV_GPRS_S2");
		if (gprs_s0 == 1 && gprs_s1 == 1 && gprs_s2 == 1) {
			asyslog(LOG_INFO, "无GPRS模块  %d, %d, %d", gprs_s0, gprs_s1, gprs_s2);
			return 0;
		} else {
			asyslog(LOG_INFO, "有GPRS模块  %d, %d, %d", gprs_s0, gprs_s1, gprs_s2);
			return 1;
		}
	}
	return 0;
}

int gpofun(char *devname, int data) {
	int fd = -1;
	if ((fd = open(devname, O_RDWR | O_NDELAY)) >= 0) {
		write(fd, &data, sizeof(int));
		close(fd);
		return 1;
	}
	return 0;
}

int OpenMuxCom(INT8U port, int baud, unsigned char *par, unsigned char stopb,
		INT8U bits) {
	int Com_Port = 0;
	struct termios old_termi, new_termi;
	int baud_lnx = 0;
	unsigned char tmp[128];
	memset(tmp, 0, 128);
	sprintf((char *) tmp, "/dev/mux%d", port);
	//    sprintf((char*)tmp, "/dev/ttyS%d", port);

	Com_Port = open((char *) tmp, O_RDWR | O_NOCTTY); /* 打开串口文件 */
	if (Com_Port < 0) {
		fprintf(stderr, "open the serial port fail! errno is: %d\n", errno);
		return -1; /*打开串口失败*/
	}
	if (tcgetattr(Com_Port, &old_termi) != 0) /*存储原来的设置*/
	{
		fprintf(stderr,
				"get the terminal parameter error when set baudrate! errno is: %d\n",
				errno);
		/*获取终端相关参数时出错*/
		return -1;
	}
	// printf("\n\r c_ispeed == %d old_termi  c_ospeed == %d",old_termi.c_ispeed, old_termi.c_ospeed);
	bzero(&new_termi, sizeof(new_termi)); /*将结构体清零*/
	new_termi.c_cflag |= (CLOCAL | CREAD); /*忽略调制解调器状态行，接收使能*/
	new_termi.c_lflag &= ~(ICANON | ECHO | ECHOE); /*选择为原始输入模式*/
	new_termi.c_oflag &= ~OPOST; /*选择为原始输出模式*/
	new_termi.c_cc[VTIME] = 2; /*设置超时时间为0.5 s*/
	new_termi.c_cc[VMIN] = 0; /*最少返回的字节数是 7*/
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

	if (strncmp((char *) par, "even", 4) == 0) //设置奇偶校验为偶校验
			{
		// new_termi.c_iflag |= (INPCK | ISTRIP);
		new_termi.c_cflag |= PARENB;
		new_termi.c_cflag &= ~PARODD;

	} else if (strncmp((char *) par, "odd", 3) == 0) //设置奇偶校验为奇校验
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

	tcflush(Com_Port, TCIOFLUSH); /* 刷新输入输出流 */
	if (tcsetattr(Com_Port, TCSANOW, &new_termi) != 0) /* 激活串口配置 */
	{
		fprintf(stderr, "Set serial port parameter error!\n"); // close(Com_Port);
		return 0;
	}
	return Com_Port;
}

int RecieveFromComm(char *buf, int mlen, int com) {
	int len = read(com, buf, mlen);

	if (len > 0) {
		INT8U atbuf[1024];
		memset(atbuf, 0x00, sizeof(atbuf));
		int atbufindex = 0;

		asyslog(LOG_INFO, "[AT]recv:\n");

		for (int i = 0; i < len; i++) {
			if (buf[i] >= 0x20 && buf[i] <= 0x7E) {
				atbuf[atbufindex++] = buf[i];
			}
		}
		asyslog(LOG_INFO, "%s", atbuf);
	}
	return (len < 0) ? 0 : len;
}

//查看拨号程序是否获取到ip地址
int tryifconfig(CLASS25 *class25) {
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
		int ips[4];
		memset(ips, 0x00, sizeof(ips));
		sscanf(inet_ntoa(sin.sin_addr), "%d.%d.%d.%d", &ips[0], &ips[1],
				&ips[2], &ips[3]);
		asyslog(LOG_INFO, "获取到正确的IP地址%d.%d.%d.%d\n", ips[0], ips[1], ips[2],
				ips[3]);
		class25->pppip[0] = 4;
		class25->pppip[0] = ips[0];
		class25->pppip[0] = ips[1];
		class25->pppip[0] = ips[2];
		class25->pppip[0] = ips[3];
		close(sock);
		return 1;
	}
	close(sock);
	return 0;
}

int AtSendCmd(ATOBJ *ao, INT8U * buf, INT32U len) {
	int res = write(ao->fd, buf, len);
	int i = 0;
	if (len > 0) {
		INT8U atbuf[1024];
		memset(atbuf, 0x00, sizeof(atbuf));
		int atbufindex = 0;

		asyslog(LOG_INFO, "[AT]send:");

		for (i = 0; i < len; i++) {
			if (buf[i] >= 0x20 && buf[i] <= 0x7E) {
				atbuf[atbufindex++] = buf[i];
			}
		}
		asyslog(LOG_INFO, "%s", atbuf);
	}
	return (res < 0) ? 0 : res;
}

int AtInitObjBlock(ATOBJ *ao) {
	memset(ao, 0x00, sizeof(ATOBJ));
}

ATOBJ *AtGet(void) {
	return &atObj;
}

int SendCommandGetOK(ATOBJ *ao, int retry, const char *fmt, ...) {
	va_list argp;
	va_start(argp, fmt);
	char cmd[128];
	memset(cmd, 0x00, sizeof(cmd));
	vsprintf(cmd, fmt, argp);
	va_end(argp);

	for (int timeout = 0; timeout < retry; timeout++) {
		char Mrecvbuf[128];
		AtSendCmd(ao, cmd, strlen(cmd));
		delay(500);
		memset(Mrecvbuf, 0, 128);
		RecieveFromComm(Mrecvbuf, 128, ao->fd);
		if (strstr(Mrecvbuf, "OK") != NULL) {
			return 1;
		}
	}
	return 0;
}

int AtPrepare(ATOBJ *ao) {
	static int count = 0;
	static int retry = 0;

	char Mrecvbuf[128];
	memset(Mrecvbuf, 0, 128);
	MASTER_STATION_INFO ip_port;
	CLASS25* c25 = (CLASS25*) dbGet("class25");

	switch (ao->state) {
	case 0:
		system("ppp-off");
		if (helperKill("gsmMuxd", 1) == -1) {
			return 5000;
		}
		if (ao->at_retry++ > 5) {
			asyslog(LOG_INFO, "<AT流程> 5次拨号失败，关断模块10分钟>>>>>>>>");
			gpofun("/dev/gpoGPRS_POWER", 0);
			ao->at_retry = 0;
			return 1000 * 60 * 5;
		}
		count = 0;
		retry = 0;
		gpofun("/dev/gpoCSQ_GREEN", 0);
		gpofun("/dev/gpoCSQ_RED", 0);
		gpofun("/dev/gpoONLINE_LED", 0);

		ao->CSQ = 0;
		ao->TYPE = 0;
		ao->PPPD = 0;
		ao->GPRS_STATE = 0;

		ao->state = 1;
		return 200;
	case 1:
		asyslog(LOG_INFO, "开始拨号>>>>>>>>");
		gpofun("/dev/gpoGPRS_POWER", 1);
		gpofun("/dev/gpoGPRS_RST", 1);
		gpofun("/dev/gpoGPRS_SWITCH", 1);
		ao->state = 2;
		return 3000;
	case 2:
		gpofun("/dev/gpoGPRS_SWITCH", 0);
		ao->state = 3;
		return 1000;
	case 3:
		gpofun("/dev/gpoGPRS_SWITCH", 1);
		ao->state = 4;
		return 1000;
	case 4:
		gpofun("/dev/gpoGPRS_RST", 0);
		ao->state = 5;
		return 1000;
	case 5:
		gpofun("/dev/gpoGPRS_RST", 1);
		ao->state = 6;
		return 10 * 1000;
	case 6:
		system("mux.sh&");
		ao->state = 7;
		return 10 * 1000;
	case 7:
		if(ao->fd > 0){
			close(ao->fd);
		}
		ao->fd = OpenMuxCom(0, 115200, (unsigned char *) "none", 1, 8); // 0
		if (ao->fd < 0) {
			close(ao->fd);
			asyslog(LOG_ERR, "串口复用异常");
			ao->state = 0;
			return 500;
		}
		ao->state = 8;
		return 100;
	case 8:
		if (retry > 5) {
			ao->state = 0;
			return 100;
		}
		AtSendCmd(ao, "at\r", 3);
		ao->state = 9;
		return 500;
	case 9:
		if (RecieveFromComm(Mrecvbuf, 128, ao->fd) > 0) {
			if (strstr(Mrecvbuf, "OK") != NULL) {
				retry = 0;
				ao->state = 10;
				return 500;
			}
		}
		retry++;
		ao->state = 8;
		return 1000;
	case 10:
		ao->GPRS_STATE = 1;
		if (retry > 5) {
			ao->state = 0;
			return 100;
		}
		AtSendCmd(ao, "\rAT$MYGMR\r", 10);
		ao->state = 11;
		return 500;
	case 11:
		RecieveFromComm(Mrecvbuf, 128, ao->fd);
		if (sscanf(Mrecvbuf,
				"%*[^\n]\n%[^\n]\n%[^\n]\n%[^\n]\n%[^\n]\n%[^\n]\n%[^\n]",
				ao->INFO[0], ao->INFO[1], ao->INFO[2], ao->INFO[3], ao->INFO[4],
				ao->INFO[5]) == 6) {
			retry = 0;
			ao->state = 12;
			return 500;
		}
		retry++;
		ao->state = 10;
		return 1000;
	case 12:
		if (retry > 15) {
			ao->state = 0;
			return 100;
		}
		AtSendCmd(ao, "\rAT$MYTYPE?\r", 12);
		ao->state = 13;
		return 500;
	case 13:
		RecieveFromComm(Mrecvbuf, 128, ao->fd);
		if (sscanf(Mrecvbuf, "%*[^:]: %d,%d%*[^]", &ao->Tmp, &ao->TYPE) == 2) {
			retry = 0;
			ao->state = 14;
			return 500;

		}
		retry++;
		ao->state = 12;
		return 1000;
	case 14:
		ao->GPRS_STATE = 2;
		if (retry > 5) {
			ao->state = 0;
			return 100;
		}
		AtSendCmd(ao, "\rAT+CSQ\r", 8);
		ao->state = 15;
		return 500;
	case 15:
		RecieveFromComm(Mrecvbuf, 128, ao->fd);
		if (sscanf(Mrecvbuf, "%*[^:]: %d,%*[^]", &ao->CSQ) == 1) {
			asyslog(LOG_INFO, "GprsCSQ = %d\n", ao->CSQ);
			if (ao->CSQ != 99) {
				retry = 0;
				ao->state = 16;
				return 500;
			}
		}
		retry++;
		ao->state = 14;
		return 1000;
	case 16:
		if (retry > 35) {
			ao->state = 0;
			return 100;
		}
		AtSendCmd(ao, "\rAT+CREG?\r", 10);
		ao->state = 17;
		return 500;
	case 17:
		RecieveFromComm(Mrecvbuf, 128, ao->fd);
		if (sscanf(Mrecvbuf, "%*[^:]: %d,%d", &ao->Tmp, &ao->REG_STATE) == 2) {
			asyslog(LOG_INFO, "GprsCREG = %d,%d", ao->Tmp, ao->REG_STATE);
			if (ao->REG_STATE == 1 || ao->REG_STATE == 5) {
				retry = 0;
				ao->state = 18;
				return 500;
			}
		}
		retry++;
		ao->state = 16;
		return 1000;
	case 18:
		ao->GPRS_STATE = 4;
		if (retry > 5) {
			ao->state = 0;
			return 100;
		}
		AtSendCmd(ao, "\rAT+CIMI\r", 9);
		ao->state = 19;
		return 500;
	case 19:
		RecieveFromComm(Mrecvbuf, 128, ao->fd);

		memset(ao->CIMI, 0x00, sizeof(ao->CIMI));
		if (sscanf((char *) &Mrecvbuf[0], "%*[^0-9]%[0-9]", ao->CIMI) == 1) {
			if (strncmp("46003", ao->CIMI, 5) == 0
					|| strncmp("46011", ao->CIMI, 5) == 0
					|| strncmp("46005", ao->CIMI, 5) == 0) {
				ao->TYPE = 1;
				ao->script = 1;
				retry = 0;
				ao->state = 20;
				return 500;
			}
			ao->TYPE = 0;
			ao->script = 0;
			retry = 0;
			ao->state = 20;
			return 500;
		}
		retry++;
		ao->state = 18;
		return 1000;
	case 20:
		if (retry > 5) {
			ao->state = 0;
			return 100;
		}
		AtSendCmd(ao, "\rAT+QNWINFO\r", 12);
		ao->state = 21;
		return 500;
		break;
	case 21:
		RecieveFromComm(Mrecvbuf, 128, ao->fd);
		if (strstr(Mrecvbuf, "TDD LTE") != NULL) {
			asyslog(LOG_INFO, "TDD LTE模式在线");
			ao->TYPE = 2;
		}
		if (strstr(Mrecvbuf, "FDD LTE") != NULL) {
			asyslog(LOG_INFO, "FDD LTE模式在线");
			ao->TYPE = 3;
		}
		retry = 0;
		if (((int) dbGet("gprs.type")) == 2) {
			ao->state = 50;
		} else {
			ao->state = 22;
		}
		return 500;
	case 22:
		close(ao->fd);
		if (ao->script == 1) {
			system("pppd call cdma2000 &");
		} else {
			system("pppd call gprs &");
		}
		retry = 0;
		ao->state = 23;
		return 100;
	case 23:
		if (retry > 50) {
			ao->state = 0;
			return 100;
		}
		if (helperCheckIp(&ao->PPP_IP[0]) == 1) {
			retry = 0;
			ao->PPPD = 1;
			ao->state = AT_FINISH_PREPARE;
		}
		retry++;
		return 1000;
	case 50:
		retry = 0;
		ao->state = 51;
		return 100;
	case 51:
		if (retry > 8) {
			ao->state = 0;
			return 100;
		}
		if (SendCommandGetOK(ao, 1, "\rAT$MYNETACT=1,0\r") == 1) {
			retry = 0;
			ao->state = 52;
			return 100;
		}
		retry++;
		return 500;
	case 52:
		if (retry > 8) {
			ao->state = 0;
			return 100;
		}
		if (SendCommandGetOK(ao, 1, "\rAT$MYNETCON=1,\"APN\",\"%s\"\r",
				&(((CLASS25*) dbGet("class25"))->commconfig.apn[1])) == 1) {
			retry = 0;
			ao->state = 53;
			return 100;
		}
		retry++;
		return 500;
	case 53:
		if (retry > 8) {
			ao->state = 0;
			return 100;
		}
		if (SendCommandGetOK(ao, 1, "\rAT$MYNETCON=1,\"USERPWD\",\"%s,%s\"\r",
				&(((CLASS25*) dbGet("class25"))->commconfig.userName[1]),
				&(((CLASS25*) dbGet("class25"))->commconfig.passWord[1]))
				== 1) {
			retry = 0;
			ao->state = 54;
			return 100;
		}
		retry++;
		return 500;
	case 54:
		if (retry > 8) {
			ao->state = 0;
			return 100;
		}
		if (SendCommandGetOK(ao, 1, "\rAT$MYNETURC=1\r") == 1) {
			retry = 0;
			ao->state = 55;
			return 100;
		}
		retry++;
		return 500;

	case 55:
		if (retry > 8) {
			ao->state = 0;
			return 100;
		}
		ip_port = helperGetNextGPRSIp();
		if (SendCommandGetOK(ao, 1, "\rAT$MYNETSRV=1,1,0,0,\"%s:%d\"\r",
				ip_port.ip, ip_port.port) == 1) {
			retry = 0;
			ao->state = 56;
			return 100;
		}
		retry++;
		return 500;
	case 56:
		if (retry > 10) {
			ao->state = 0;
			return 100;
		}
		if (SendCommandGetOK(ao, 1, "\rAT$MYNETACT=1,1\r") == 1) {
			retry = 0;
			ao->state = 57;
			return 100;
		}
		retry++;
		return 500;

	case 57:
		if (retry > 30) {
			ao->state = 0;
			return 100;
		}

		if (SendCommandGetOK(ao, 1, "\rAT$MYNETOPEN=1\r") == 1) {
			retry = 0;
			dbSet("online.type", 3);
			ao->PPPD = 1;
			ao->state = AT_FINISH_PREPARE;
			return 100;
		}
		retry++;
		return 1500;

	case AT_FINISH_PREPARE:
		if (retry > 20) {
			ao->state = 0;
			return 1000;
		}
		retry++;
		asyslog(LOG_INFO, "======%d", retry);
		ao->at_retry = 0;
		readCoverClass(0x4500, 0, c25, sizeof(CLASS25), para_vari_save);
		c25->signalStrength = ao->CSQ;
		//TODO:SIM卡号码simkard, CCID未有AT获取
		//一致性测试4500-10,ccid的第一个字节为长度
		memcpy(&c25->imsi[1],ao->imsi,sizeof(c25->imsi));
		c25->imsi[0] = strlen(ao->imsi);
		memcpy(&c25->ccid[1],ao->ccid,sizeof(c25->ccid));
		c25->ccid[0] = strlen(ao->ccid);
		memcpy(&c25->simkard[1],ao->CIMI,sizeof(c25->simkard));
		c25->simkard[0] = strlen(ao->CIMI);
		memcpy(&c25->pppip,ao->PPP_IP,sizeof(c25->pppip));
		saveCoverClass(0x4500, 0, c25, sizeof(CLASS25), para_vari_save);
		return 5 * 1000;
	}
	return 0;
}

int AtPrepareFinish(ATOBJ *ao) {
	return ao->state == AT_FINISH_PREPARE;
}

int checkModelStatus(char *buf, int len) {
	static int status = 0;
	static int merror = 0;

	if (len < 0) {
		return 0;
	}

	for (int i = 0; i < len - 5; ++i) {
		if (buf[i] == 'E' && buf[i + 1] == 'R' && buf[i + 2] == 'R'
				&& buf[i + 3] == 'O' && buf[i + 4] == 'R') {
			return -1;
		}
	}
	return 0;
}

int AtReadExactly(ATOBJ *ao, CommBlock *nst) {
	int chl = 0;
	int sum = 0;

	memset(ao->recv, 0, AT_FRAME_LEN);
	AtSendCmd(ao, "\rAT$MYNETREAD=1,1024\r", 21);
	delay(300);
	int resLen = RecieveFromComm(ao->recv, AT_FRAME_LEN, ao->fd);

	char *netReadPos = strstr(ao->recv, "MYNETREAD:");
	if (netReadPos == NULL) {
		printf("找不到MYNETREAD\n");
		return checkModelStatus(ao->recv, resLen);
	}

	if (sscanf(netReadPos, "%*[^:]: %d,%d", &chl, &sum) == 2) {
		printf("报文通道：长度(%d-%d)\n", chl, sum);
		if (sum == 0) {
			return 0;
		}
		int pos = helperReadPositionGet(sum);
		printf("==recv %d, at %d has %d [bytes]\n", resLen, 15 + pos, sum);
		int old = (int) dbGet("calc.new") + sum;
		dbSet("calc.new", old);
		for (int i = 0; i < sum; i++) {
			nst->RecBuf[nst->RHead] = netReadPos[pos + 15 + i];
			nst->RHead = (nst->RHead + 1) % BUFLEN;
			printf("%02x ", netReadPos[pos + 15 + i]);
		}
		return sum;
	} else {
		asyslog(LOG_INFO, "没有解析到报文\n");
	}
	return 0;
}

int myStrnstr(char *buf, int len) {
	for (int i = 0; i < len - 1; ++i) {
		if (buf[i] == 'O' && buf[i + 1] == 'K') {
			return i;
		}
	}
	return 0;
}

int AtSendExactly(ATOBJ *ao) {
	if (ao->SendLen <= 0) {
		return 0;
	}

	int chl = 0;
	int sum = 0;

	for (int timeout = 0; timeout < 3; timeout++) {
		char cmdBuf[2048];
		memset(cmdBuf, 0x00, sizeof(cmdBuf));
		sprintf(cmdBuf, "\rAT$MYNETWRITE=1,%d\r", ao->SendLen);
		AtSendCmd(ao, cmdBuf, strlen(cmdBuf));

		delay(500);
		int recLen = RecieveFromComm(ao->recv, sizeof(ao->recv), ao->fd);
		if (sscanf(ao->recv, "%*[^:]: %d,%d", &chl, &sum) == 2) {
			write(ao->fd, ao->send, sum);
			for (int j = 0; j < 3; ++j) {
				delay(500);
				memset(ao->recv, 0, sizeof(ao->recv));
				int position = RecieveFromComm(ao->recv, sizeof(ao->recv),
						ao->fd);
				if (myStrnstr(ao->recv, position) != 0) {
					printf("~~~~~~~~~~~~~%d-%d-%d", ao->SendLen, sum, position);
					for (int i = 0; i < ao->SendLen; i++) {
						ao->send[i] = ao->send[sum + i];
						printf("%02x ", ao->send[i]);
					}
					ao->SendLen -= sum;
					int old = (int) dbGet("calc.new") + sum;
					dbSet("calc.new", old);
					printf("&&&&&&&&&%d\n", ao->SendLen);
					if (ao->SendLen == 0) {
						goto SEND_END;
					}

				}
			}
		}
	}

	SEND_END: printf("#####return len = %d\n", ao->SendLen);
	return ao->SendLen;
}

int AtWriteToBuf(int fd, INT8U *buf, INT16U len) {
	for (int i = 0; i < len; i++) {
		AtGet()->send[AtGet()->SendLen + i] = buf[i];
	}
	AtGet()->SendLen += len;
}

int ATUpdateStatus(ATOBJ *ao) {
	ProgramInfo *info = (ProgramInfo *) dbGet("program.info");
	info->dev_info.Gprs_csq = ao->CSQ;
	info->dev_info.gprs_status = ao->GPRS_STATE;
	info->dev_info.wirelessType = ao->TYPE;
	info->dev_info.pppd_status = ao->PPPD;
	info->dev_info.connect_ok = (dbGet("online.type") != 0) ? 1 : 0;
	info->dev_info.jzq_login = dbGet("online.type");
	gpofun("/dev/gpoONLINE_LED", (dbGet("online.type") != 0) ? 1 : 0);
}

int AtGetSendLen(ATOBJ *ao) {
	return ao->SendLen;
}
