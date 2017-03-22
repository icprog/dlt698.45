#ifndef __SEND_BUF_H
#define __SEND_BUF_H

#include "dlt698def.h"
#include "StdDataType.h"

typedef struct {
    INT8U buf[BUFLEN];
    INT16U len;
} SendStru;

int SendBufWrite(INT8U* buf, INT16U len);
int SendBufCheck(void);
int SendBufClean(void);
int SendBufSendNext(int fd, INT8S (*p_send)(int fd, INT8U* buf, INT16U len));

#endif