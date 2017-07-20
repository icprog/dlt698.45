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
	return ao->state != 99;
}

void AtPrepare(ATOBJ *ao) {

	switch (ao->state) {
	case 0:
		break;
	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	case 4:
		break;
	case 5:
		break;
	}
}

void AtPrepareFinish(ATOBJ *ao) {
	ao->state = 99;
}

void AtDealer(ATOBJ *ao) {
	ao->state = 99;
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
