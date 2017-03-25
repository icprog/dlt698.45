#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "cjcomm.h"
#include "SendBuf.h"

static SendStru SendList[10];
static int head, tail;

int SendBufWrite(INT8U* buf, INT16U len) {
    if ((head + 1) % 10 == tail) {
        return -1;
    }

    memcpy(SendList[head].buf, buf, len);
    SendList[head].len = len;
    head += 1;
    head %= 10;

    return len;
}
int SendBufCheck(void) {
    return head != tail;
}
int SendBufClean(void) {
    head = 0;
    tail = 0;
    memset(SendList, 0x00, sizeof(SendList));
}
int SendBufSendNext(int fd, INT8S (*p_send)(int fd, INT8U* buf, INT16U len)) {
    if (head != tail) {
        tail += 1;
        tail %= 10;
        return p_send(fd, SendList[(tail - 1 + 10) % 10].buf, SendList[(tail - 1 + 10) % 10].len);
    }
    return 0;
}
