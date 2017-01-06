/*
 * SPI.c
 *
 *  Created on: 2013-3-6
 *      Author: Administrator
 */
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
//#include "../libhd/libhd.h"
#include "SPI.h"

int SPI_Close(int fd) {
    close(fd);
    return 1;
}

static int dumpstat(const char* name, int fd) {
    static uint8_t mode;
    static uint8_t bits   = 8;
    static uint32_t speed = 8000000;

    mode |= SPI_MODE_3;

    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) == -1){
    	printf("[SPI ERROR] can't set spi mode");
    	return -1;
    }

    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1){
    	printf("[SPI ERROR] can't set bits per word");
    	return -1;
    }

    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1){
    	printf("[SPI ERROR] can't set max speed");
    	return -1;
    }

    return fd;
}

int32_t SPI_Init(int32_t fd, u_char* spipath) {
	if (fd != -1) {
		SPI_Close(fd);
	}

	fd = open((char*) spipath, O_RDWR);
	if (fd < 0)
		printf("[SPI ERROR] can't open  device %s\n", spipath);

	return dumpstat((char*) spipath, fd);
}
