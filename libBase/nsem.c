/*
 * nsem.c
 *
 *  Created on: 2017-1-6
 *      Author: gk
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "PublicFunction.h"
/*
 * 功能：在/dev/shm目录下创建信号量描述文件，如果已经存在同名的文件，则先删除，然后在创建。
 * 输入：
 * name：为命名信号量的名称。
 * flag：1 或者 0
 * 返回：如果信号量创建成功，则返回信号量句柄
 */
sem_t* nsem_creat(const char* name, int flag) {
    sem_t* fd;
    if (name != NULL) {
        sem_unlink(name);
        fd = sem_open(name, O_CREAT, O_RDWR, flag);
        if (fd != SEM_FAILED) {
            return fd;
        }
    }
    fprintf(stderr, "\ncreate sem %s failed:%s", name, strerror(errno));
    return NULL;
}
/*
 * 功能：打开一个命名信号量
 * 输入
 * name：命名信号量文件名
 * 返回
 * 成功：返回信号量句柄
 * 失败：返回空
 */
sem_t* nsem_open(const char* name) {
    sem_t* fd;
    if (name != NULL) {
        fd = sem_open(name, O_RDWR);
        if (fd != SEM_FAILED) {
            return fd;
        }
    }
    fprintf(stderr, "\nopen sem %s failed:%s", name, strerror(errno));
    return NULL;
}
void nsem_timedwait(sem_t* sem, int sec) {
    struct timespec tsspec;
    if (clock_gettime(CLOCK_REALTIME, &tsspec) == -1) {
        fprintf(stderr, "\nnsem_timedwait clock_gettime error");
    }
    tsspec.tv_sec += sec;
    sem_timedwait(sem, &tsspec);
}


