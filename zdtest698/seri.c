/*
 * seri.c
 *
 *  Created on: 2014-3-1
 *      Author: yd
 */
#include "stdio.h"
#include "errno.h"
#include "time.h"
//#include <hw/inout.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <termios.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <stdarg.h>
#include "PublicFunction.h"

//接收字符串    //9.24
int ReceiveFrom485_test(unsigned char *str, int ComPort)
{
	INT8U TmpBuf[256];
	int len, rec_step;
	int rec_head, rec_tail;
	int DataLen, i;
	int j;

	memset(TmpBuf, 0, 256);
	rec_head = rec_tail = rec_step = DataLen = 0;

	delay(500);
	for (j = 0; j < 15; j++) {
		len = read(ComPort, TmpBuf, 255); //200
		DEBUG_TIME_LINE("len: %d", len);
		if (j == 0 && len == 0)
			return 0;

		if (len > 0)
			for (i = 0; i < len; i++)
				str[rec_head++] = TmpBuf[i];

		switch (rec_step) {
		case 0:
			if (rec_tail < rec_head) {
				for (i = rec_tail; i < rec_head; i++) {
					if (str[i] == 0x68) {
						rec_step = 1;
						rec_tail = i;
						break;
					} else
						rec_tail++;
				}
			}
			break;
		case 1:
			if ((rec_head - rec_tail) >= 9) {
				if (str[rec_tail] == 0x68 && str[rec_tail + 7] == 0x68) {
					DataLen = str[rec_tail + 9]; //获取报文数据块长度
					rec_step = 2;
					break;
				} else
					rec_tail++;
			}
			break;
		case 2:
			if ((rec_head - rec_tail) >= (DataLen + 2)) {
				if (str[rec_tail + 9 + DataLen + 2] == 0x16)
					return rec_head;
			}
			break;
		default:
			break;
		}
	}
	return (0);
}
