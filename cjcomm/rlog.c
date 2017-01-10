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

void file_sys_test_w(){
	char path[64];
	char content[512];
	memset(path, 0, sizeof(path));
	for(int j = 0; j < 512; j ++){
		for(int i = 0; i < 128; i++){
			sprintf(path, "/nand/3000/%03d/%04d", j, i);
			FILE* fd = fopen(path, "a+");
			int size = rand();
			fwrite(content, size%256 + 64, 1, fd);
			fclose(fd);
		}
	}
}

void file_sys_test_r(){
	char path[64];
	char content[512];
	memset(path, 0, sizeof(path));
	srand(time(NULL));
	for(int i = 0; i < 1024; i++){
		int m = rand()%512;
		int n = rand()%128;
		sprintf(path, "/nand/3000/%03d/%04d", m, n);
		FILE* fd = fopen(path, "r");
		if (fd != NULL){
			int res = fread(content, 1, 512, fd);
			printf("File: %s\t%03d\n", path, res);
			fclose(fd);
		}
	}
}

void file_sys_test_evn(){
	char path[64];
	memset(path, 0, sizeof(path));
	for(int i = 0; i < 512; i++){
		sprintf(path, "mkdir /nand/3000/%03d",i);
		system(path);
	}
}
