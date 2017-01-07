#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "PublicFunction.h"

void rlog(const char* fmt, ...) {
    char path[64];
    TS ts = {};
    TSGet(&ts);

    memset(path, 0, sizeof(path));
    sprintf(path, "/nand/mlog/M%02d-%02d-%02d.log", (ts.Year + 1900) % 100, ts.Month, ts.Day);
    FILE* fd = fopen(path, "a+");
    va_list argp;
    va_start(argp, fmt);
    fprintf(fd, "[%02d-%02d-%02d]:", ts.Hour, ts.Minute, ts.Sec);
    vfprintf(fd, fmt, argp); /* 将va_list传递给子函数 */
    va_end(argp);
    fclose(fd);
}

void file_sys_test(){
	char path[64];
	char content[512];
	memset(path, 0, sizeof(path));
	for(int i = 0; i < 1024; i++){
		sprintf(path, "/nand/test/%d", i);
		FILE* fd = fopen(path, "a+");
		fwrite(content, 512, 1, fd);
		fclose(fd);
	}
}
