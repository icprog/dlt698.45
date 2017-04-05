/*
 * spi.c
 *
 *  Created on: 2013-5-27
 *      Author: liuhongli
 */
//
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/spi/spidev.h>
#include <semaphore.h>

#include "att7022e.h"
#include "rn8209.h"
#include "spi.h"

#ifdef NUC972
#define SPI_TX_DUAL             0x100
#define SPI_TX_QUAD             0x200
#define SPI_RX_DUAL             0x400
#define SPI_RX_QUAD             0x800

static void dumpstat(const char *name, int fd)
{
	static uint32_t mode;
	static uint8_t bits = 8;
	static uint32_t speed = 10000000;//2000000;//400000;
	int ret;
	unsigned char lsb=!SPI_LSB_FIRST;

//	mode |= SPI_CPHA;
#ifdef NUC972
	mode = SPI_MODE_1 | SPI_TX_DUAL | SPI_RX_DUAL;
#else
	mode = SPI_MODE_1;//SPI_MODE_1;
#endif

	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		fprintf(stderr,"can't set spi mode\n");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		fprintf(stderr,"can't get spi mode\n");

	if(ioctl(fd, SPI_IOC_WR_LSB_FIRST, &lsb)<0)
	{
			perror("bit order can't set");
	}

	if (ioctl(fd, SPI_IOC_RD_LSB_FIRST, &lsb) < 0)
	{
		perror("SPI rd_lsb_fist");
	}


	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		fprintf(stderr,"can't set bits per word\n");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		fprintf(stderr,"can't get bits per word\n");

	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		fprintf(stderr,"can't set max speed hz\n");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		fprintf(stderr,"can't get max speed hz\n");
}

#else
/*
 * spi设备工作模式设置
 * */
static void dumpstat(const char *name, int fd)
{
	static uint8_t mode;
	static uint8_t bits = 8;
	static uint32_t speed = 2000000;//16000;
	int ret;

	mode |= SPI_CPHA;			//交采SPI原工作模式
//	mode = SPI_MODE_3;

	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		fprintf(stderr,"can't set spi mode");//pabort

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		fprintf(stderr,"can't get spi mode");

	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		fprintf(stderr,"can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		fprintf(stderr,"can't get bits per word");

	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		fprintf(stderr,"can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		fprintf(stderr,"can't get max speed hz");
}
#endif

static void dumpstat_r(const char *name, int fd)
{
	static uint8_t mode;
	static uint8_t bits = 8;
	static uint32_t speed = 400000;
	int ret;

	mode = SPI_MODE_1;
//	mode = SPI_MODE_2;

	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		fprintf(stderr,"can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		fprintf(stderr,"can't get spi mode");

	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		fprintf(stderr,"can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		fprintf(stderr,"can't get bits per word");

	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		fprintf(stderr,"can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		fprintf(stderr,"can't get max speed hz");
}


INT32S spi_close(INT32S fd)
{
	close(fd);
	return 1;
}

INT32S spi_init(INT32S fd,const char * spipath)
{
	if (fd != -1) {
		spi_close(fd);
	}
	fd = open((char*)spipath, O_RDWR);
	if (fd < 0){
		syslog(LOG_NOTICE,"打开SPI设备(%s)错误\n",spipath);
		fprintf(stderr,"can't open  device %s\n",spipath);		//pabort
	}
	dumpstat((char*)spipath,fd);
	return fd;
}

INT32S spi_init_r(INT32S fd,const char * spipath)
{
	if (fd != -1) {
		spi_close(fd);
	}
	fd = open((char*)spipath, O_RDWR);
	if (fd < 0){
		syslog(LOG_NOTICE,"打开SPI设备(%s)错误\n",spipath);
		fprintf(stderr,"can't open  device %s\n",spipath);		//pabort
	}
	dumpstat_r((char*)spipath,fd);
	return fd;
}

#ifdef NUC972
INT32S att_spi_write(int spifp, INT32U addr, INT32U len, INT8U *buf)
{
	INT32U 	i=0;
	int 	cnt = 0;
	INT8U	cmd[BUFFLENMAX_SPI_ACS];
	int		wlen = 0;

//	sem_wait(sem_check_fd);
	if((addr>=Reg_DataBuf) && (addr<=Reg_Reset)){
		cmd[cnt++] = addr | 0xC0; //写特殊命令
	} else
		cmd[cnt++] = addr | 0x80; //写更新校表数据
	for (i = 0; i < len; i++) {
		cmd[cnt++] = buf[i];
	}
	wlen = len+1;
	gpio_writebyte((INT8S*)DEV_ESAM_CS,1);
	gpio_writebyte((INT8S*)DEV_ATT_CS, 1);
	gpio_writebyte((INT8S*)DEV_ATT_CS, 0);
	if(write(spifp, cmd, wlen)!=wlen)
	{
	    fprintf(stderr,"TESAM send data format failed!\r\n");
	}
	gpio_writebyte((INT8S*)DEV_ATT_CS, 1);
//	sem_post(sem_check_fd);
	return wlen;
}

INT32S att_spi_read(int spifp,INT32U addr, INT32U len)
{
	INT8U	cmd[BUFFLENMAX_SPI_ACS];
	INT8U	buf[BUFFLENMAX_SPI_ACS];
	INT32U i;
	INT32S rec;
	int cnt = 0;

//	sem_wait(sem_check_fd);
	cmd[cnt++] = addr & 0x7f;
//	usleep(20); //CS high more than 10us
	gpio_writebyte((INT8S*)DEV_ESAM_CS,1);
	gpio_writebyte((INT8S*)DEV_ATT_CS, 1);
	gpio_writebyte((INT8S*)DEV_ATT_CS, 0);
	usleep(80); //delay more than 50us
	if(write(spifp, cmd, 1)!=1)
	{
	    printf("TESAM send data format failed!\r\n");
	    gpio_writebyte((INT8S*)DEV_ATT_CS, 1);
//	    sem_post(sem_check_fd);
		return -1;
	}
//	usleep(10);

	if(read(spifp, buf, len)!= len) {
		fprintf(stderr,"TESAM returning data: Common area failed!\r\n");
	}

	gpio_writebyte((INT8S*)DEV_ATT_CS, 1);

	rec = 0;
	for (i = 0; i < len; i++) {
		rec = (rec << 8) | buf[i];
//		fprintf(stderr,"%02x ",buf[i]);
	}
//	sem_post(sem_check_fd);
	return rec;
}

#else
int spi_read(int fd, INT8U *cbuf, int16_t clen, INT8U *rbuf, int rlen)
{
	unsigned char rx[512],i;
	struct spi_ioc_transfer	xfer[2];
	unsigned char tx[512];

	memset(tx,0x00,512);
	memset(rx,0x00,512);
	memset(xfer, 0,  sizeof xfer);

	for(i=0;i<clen;i++)
		tx[i]=cbuf[i];
	xfer[0].tx_buf = (int)tx;
	xfer[0].len = clen;

	xfer[1].rx_buf = (int) rx;
	xfer[1].len = rlen;

	gpio_writebyte((INT8S*)DEV_ESAM_CS,1);
	gpio_writebyte((INT8S*)DEV_ATT_CS, 1);
	gpio_writebyte((INT8S*)DEV_ATT_CS, 0);
	ioctl(fd, SPI_IOC_MESSAGE(2), xfer);
	gpio_writebyte((INT8S*)DEV_ATT_CS, 1);
	for(i=0; i<rlen; i++)
		rbuf[i]=rx[i];
	return 1;
}

int spi_write(int fd, INT8U *buf, int len)
{
	INT8U	tx[32],rx[32];
	INT8U	i;
	struct spi_ioc_transfer	xfer[2];

	memset(tx,0,32);
	memset(rx,0,32);
	memset(xfer, 0, sizeof xfer);

	if(len>8)  	return ERR_WRTBUF_OVERLEN;
	for(i=0;i<len;i++)
		tx[i]=buf[i];
	xfer[0].tx_buf = (int)tx;
	xfer[0].len = len;
	//xfer[0].delay_usecs=10;

	xfer[1].rx_buf = (int) rx;
	xfer[1].len = 0;
	//xfer[1].delay_usecs=10;

	gpio_writebyte((INT8S*)DEV_ESAM_CS,1);
	gpio_writebyte((INT8S*)DEV_ATT_CS, 1);
	gpio_writebyte((INT8S*)DEV_ATT_CS, 0);
	ioctl(fd, SPI_IOC_MESSAGE(2), xfer);
	gpio_writebyte((INT8S*)DEV_ATT_CS, 1);
	return len;
}

INT32S att_spi_read(int spifp,INT32U addr, INT32U len)
{
	INT8U	cmd[BUFFLENMAX_SPI_ACS];
	INT8U	buf[BUFFLENMAX_SPI_ACS];
	INT32U i;
	INT32S rec;
	int cnt = 0;
//	struct timespec tsspec;

//	if (clock_gettime(CLOCK_REALTIME, &tsspec)==-1)
//		fprintf(stderr,"clock_gettime error\n\r");
//	tsspec.tv_sec += 2;
//	sem_wait(sem_check_fd);
	cmd[cnt++] = addr & 0x7f;
	spi_read(spifp, cmd, cnt, buf, len);

	rec = 0;
	for (i = 0; i < len; i++) {
//		fprintf(stderr,"\n\ncxl buf[%d] = %d\n\n",i,buf[i]);
		rec = (rec << 8) | buf[i];
	}
//	sem_post(sem_check_fd);
	return rec;
}

INT32S att_spi_write(int spifp, INT32U addr, INT32U len, INT8U *buf)
{
	INT32U i;
	int 			cnt = 0;
	INT32S num;
	INT8U	cmd[BUFFLENMAX_SPI_ACS];
//	struct timespec tsspec;

//	if (clock_gettime(CLOCK_REALTIME, &tsspec)==-1)
//		fprintf(stderr,"clock_gettime error\n\r");
//	tsspec.tv_sec += 2;
//	sem_wait(sem_check_fd);
	if((addr>=Reg_DataBuf) && (addr<=Reg_Reset)){
		cmd[cnt++] = addr | 0xC0; //写特殊命令
	} else
		cmd[cnt++] = addr | 0x80; //写更新校表数据
	for (i = 0; i < len; i++) {
		cmd[cnt++] = buf[i];
	}
	num = spi_write(spifp, cmd, cnt);
//	sem_post(sem_check_fd);
	return num;
}

#endif


/*
 * RN8209计量芯片
 * */
///////////////////////////////////////////////////////////////////////////////
int spi_read_r(int fd, INT8U *cbuf, int16_t clen, INT8U *rbuf, int rlen)
{
	unsigned char rx[512],i;
	struct spi_ioc_transfer	xfer[2];
	unsigned char tx[512];

	memset(tx,0x00,512);
	memset(rx,0x00,512);
	memset(xfer, 0,  sizeof xfer);

	for(i=0;i<clen;i++)
		tx[i]=cbuf[i];

	xfer[0].tx_buf = (int)tx;//读取的地址
	//fprintf(stderr,"发送的数据 = %d\n",(int)tx);
	xfer[0].len = clen;
//	fprintf(stderr,"发送的长度 = %d\n",(int)clen);
	xfer[1].rx_buf = (int)rx;
	xfer[1].len = rlen;
	ioctl(fd, SPI_IOC_MESSAGE(2), xfer);
	for(i=0; i<rlen; i++){
		rbuf[i]=rx[i];
		//fprintf(stderr,"rbuf[%d] = %02x ",i,rbuf[i]);
	}
	return 1;
}

int spi_write_r(int fd, INT8U *buf, int len)
{
	INT8U	tx[32],rx[32];
	INT8U	i;
	struct spi_ioc_transfer	xfer[2];

	memset(tx,0,32);
	memset(rx,0,32);
	memset(xfer, 0, sizeof xfer);

	if(len>8)  	return ERR_WRTBUF_OVERLEN;
	for(i=0;i<len;i++)
		tx[i]=buf[i];
	xfer[0].tx_buf = (int)tx;
	xfer[0].len = len;

	xfer[1].rx_buf = (int) rx;
	xfer[1].len = 0;

	ioctl(fd, SPI_IOC_MESSAGE(2), xfer);

	return len;
}

INT32S rn_spi_read(int spifp,INT32U addr)
{
	INT8U	cmd[BUFFLENMAX_SPI_ACS];
	INT8U	buf[BUFFLENMAX_SPI_ACS];
	INT32U i = 0, len_rn8209 = 0;
	INT32S rec;
	int cnt = 0;

	len_rn8209 = addr & 0x0fu;
	cmd[cnt++] = addr>>4 & 0x7f;
	spi_read_r(spifp, cmd, cnt, buf, len_rn8209);

	rec = 0;
	for (i = 0; i < len_rn8209; i++) {
		rec = (rec << 8) | buf[i];
		//fprintf(stderr,"buf[%d] = %02x ",i,buf[i]);
	}

	return rec;
}

INT32S rn_spi_write(int spifp, INT32U addr, INT8U *buf)
{
	INT32U i = 0, len_rn8209 = 0;
	int cnt = 0;
	INT32S num;
	INT8U	cmd[BUFFLENMAX_SPI_ACS];

	len_rn8209 = addr & 0x0fu;
	if(addr == CMD_REG){
		cmd[cnt++] = 0xEA; //写特殊命令
	} else
		cmd[cnt++] = addr>>4 | 0x80;
	for (i = 0; i < len_rn8209; i++) {//写更新校表数据
		cmd[cnt++] = buf[i];
	}
	num = spi_write_r(spifp, cmd, cnt);

	return num;
}

///////////////////////////////////////////////////////////////////////////////



