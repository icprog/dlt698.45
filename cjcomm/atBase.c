/*
 * atBase.c
 *
 *  Created on: 2017-7-20
 *      Author: zhoulihai
 */

#include "atBase.h"

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

int AtProcessRecv(ATOBJ *ao) {
	if ((ao->RecvHead - ao->RecvTail + AT_FRAME_LEN) % AT_FRAME_LEN < 6) {
		return 0;
	}

	for (int i = ao->RecvTail; i != ao->RecvHead; i = (i + 1) % AT_FRAME_LEN) {
		if (ao->recv[i + 0] != '\r') {
			continue;
		}
		if (ao->recv[i + 1] != '\n') {
			continue;
		}
		if (ao->recv[i + 2] != 'O') {
			continue;
		}
		if (ao->recv[i + 3] != 'K') {
			continue;
		}
		if (ao->recv[i + 4] != '\r') {
			continue;
		}
		if (ao->recv[i + 5] != '\n') {
			continue;
		}

		int index = 0;
		do {
			ao->deal[index++] = ao->recv[ao->RecvTail];
			ao->recv[ao->RecvTail] = 0x00;
			ao->RecvTail = (ao->RecvTail + 1) % AT_FRAME_LEN;
		} while (ao->RecvTail != (i + 5));
		return 1;
	}
}

int AtInPrepare(ATOBJ *ao) {
	return ao->state != AT_FINISH_PREPARE;
}

int AtPrepare(ATOBJ *ao) {
	static int count = 0;
	static int retry = 0;

	char Mrecvbuf[128];
	memset(Mrecvbuf, 0, 128);

	switch (ao->state) {
	case 0:
		count = 0;
		retry = 0;
		gpofun("/dev/gpoCSQ_GREEN", 0);
		gpofun("/dev/gpoCSQ_RED", 0);
		gpofun("/dev/gpoONLINE_LED", 0);

		SetGprsStatus(0);
		SetGprsCSQ(0);
		SetWireLessType(0);
		SetPPPDStatus(0);
		ao->state = 1;
		return 200;
	case 1:
		asyslog(LOG_INFO, "重新开始拨号");
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
		system("mux.sh");
		ao->state = 7;
		return 1000;
	case 7:
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
		}
		AtSendCmd(ao, "at\r", 3);
		ao->state = 9;
		return 500;
	case 9:
		if (RecieveFromComm(Mrecvbuf, 128, ao->fd) > 0) {
			if (strstr(Mrecvbuf, "OK") != NULL) {
				retry = 0;
				SetGprsStatus(1);
				ao->state = 10;
				return 500;
			}
		}
		retry++;
		ao->state = 8;
		return 1000;
	case 10:
		if (retry > 5) {
			ao->state = 0;
		}
		SendATCommand("\rAT$MYGMR\r", 10, ao->fd);
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
		if (retry > 5) {
			ao->state = 0;
		}
		SendATCommand("\rAT$MYTYPE?\r", 12, ao->fd);
		ao->state = 13;
		return 500;
	case 13:
		if (sscanf(Mrecvbuf, "%*[^:]: %*[^],%d,%*[^]", &ao->TYPE) == 1) {
			if ((l & 0x01) == 1) {
				asyslog(LOG_INFO, "远程通信单元类型为GPRS。\n");
				retry = 0;
				ao->state = 14;
				return 500;
			}
			if ((l & 0x08) == 8) {
				asyslog(LOG_INFO, "远程通信单元类型为CDMA2000。\n");
				retry = 0;
				ao->state = 14;
				return 500;
			}
		}
		retry++;
		ao->state = 12;
		return 1000;
	case 14:
		SetGprsStatus(2);
		SendATCommand("\rAT+CSQ\r", 8, ao->fd);
		break;
	case 15:
		break;
	case 16:
		break;
	case 17:
		break;
	case 18:
		break;
	case 19:
		break;
	case 20:
		break;
	case 21:
		break;
	case 22:
		break;
	case 23:
		break;
	case 24:
		break;
	case 25:
		break;
	case 26:
		break;
	case 27:
		break;
	}
}

void AtPrepareFinish(ATOBJ *ao) {
	ao->state = AT_FINISH_PREPARE;
}

void AtDealer(ATOBJ *ao) {
	ao->state = AT_FINISH_PREPARE;
}

int AtDealData(ATOBJ *ao) {
	if (AtInPrepare(ao)) {
		AtPrepare(ao);
	} else {
		AtDealer(ao);
	}
}

int AtNeedRead(ATOBJ *ao) {
	return ao->NeedRead;
}

int AtNeedSend(ATOBJ *ao);

int ATReadData(ATOBJ *ao);

int ATSendData(ATOBJ *ao);
