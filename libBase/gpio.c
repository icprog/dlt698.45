/*
 * gpio.c
 *
 *  Created on: 2017-1-6
 *      Author: gk
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include "PublicFunction.h"
INT8S gpio_readbyte(INT8S* devpath) {
    char data = 0;
    int fd    = 0;
    if ((fd = open((const char*)devpath, O_RDWR | O_NDELAY)) > 0) {
        read(fd, &data, sizeof(char));
        close(fd);
    } else
        return -1;
    return data;
}

INT32S gpio_readint(INT8S* devpath) {
    char data = 0;
    int fd    = 0;
    if ((fd = open((const char*)devpath, O_RDWR | O_NDELAY)) > 0) {
        read(fd, &data, sizeof(INT32S));
        close(fd);
    } else
        return -1;
    return data;
}

INT32S gpio_writebyte(INT8S* devpath, INT8S data) {
    int fd = 0;
    if ((fd = open((const char*)devpath, O_RDWR | O_NDELAY)) > 0) {
        write(fd, &data, sizeof(char));
        close(fd);
        return 1;
    } else
        return -1;
    return 0;
}
INT32S gpio_writebytes(INT8S* devpath, INT8S* vals, INT32S valnum) {
    int fd = -1;
    int i  = 0;
    fd     = open((const char*)devpath, O_RDWR | O_NDELAY);
    if (fd < 0)
        return -1;
    for (i = 0; i < valnum; i++) {
        write(fd, &vals[i], sizeof(char));
        delay(10);
    }
    delay(1000);
    close(fd);
    return 0;
}
