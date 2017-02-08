#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>

#include "PublicFunction.h"

/*
 *     LOG_ERR        发生错误
 *     LOG_WARNING    警告信息
 *     LOG_INFO       系统变量、状态信息
 *     LOG_DEBUG      调试信息
 *
 */

void asyslog(int priority, const char* fmt, ...) {
    va_list argp;
    va_start(argp, fmt);
    vsyslog(priority, fmt, argp);
    vprintf(fmt, argp);
    va_end(argp);
}

void bufsyslog(const INT8U* buf, int head, int tail, int len) {
	int count = 0;
	char msg[4096];
	memset(msg, '\0', sizeof(msg));
	sprintf(msg, "RECV:");
	while(head != tail)
	{
		sprintf(msg + 4 + count * 3, " %02x", buf[tail]);
		tail = (tail + 1)%len;
		count++;
		if (count > 1024){
			break;
		}
	}
	syslog(LOG_INFO, "%s", buf);
}
