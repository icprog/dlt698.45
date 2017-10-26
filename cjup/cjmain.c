#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>
#include <sys/reboot.h>
#include <wait.h>
#include <errno.h>
#include <mqueue.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

int gpio_writebyte(char* devpath, char data) {

    int fd = -1;
    if ((fd = open((const char*)devpath, O_RDWR | O_NDELAY)) >= 0) {
        write(fd, &data, sizeof(char));
        close(fd);
        return 1;
    }
    return 0;
}

void main(){
	int FB_Handle=-1;
	unsigned char LcdBuf[160*160];
	memset(LcdBuf, 0x00, 160*160);

	gpio_writebyte((char*)"/dev/gpoLCD_LIGHT", 1);//背光
	FB_Handle = open("/dev/fb0", O_RDWR | O_NDELAY);
	if(FB_Handle>=0)
			write(FB_Handle,LcdBuf,160*160);
	for (int i = 0; i < 10; i++){
		usleep(600 * 1000);
		memset(LcdBuf, 0xff, 16*(i+1) *160);
		write(FB_Handle,LcdBuf,160*160);
	}
	exit(0);
}
