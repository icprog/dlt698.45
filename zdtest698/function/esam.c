/*
 * 文件名：esam.c
 * 文件功能描述：安全认证
 * 创建标识：2012
 * 修改标识：
 * 修改描述：
 */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <pthread.h>
#include <linux/spi/spidev.h>
#include <math.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <semaphore.h>
#include <termios.h>
#include <errno.h>
#include "esam.h"

INT32S Comfd;

const int F_Table[16] = {0, 372, 558, 744, 1116, 1488, 1860, 0,
                           0, 512, 768, 1024, 1536, 2048, 0, 0};
const int D_Table[8] = {0, 1, 2, 4, 8, 16, 0, 0};

int gpio_read(char *devname)
{
	int data=0, fd=0;
	char devpath[100];
	sprintf(devpath, "%s", devname);
	if((fd = open(devpath, O_RDWR | O_NDELAY)) > 0)
	{
		read(fd,&data,sizeof(int));
		close(fd);
	}
	return data;
}

int gpio_write(char *devname, int data)
{
	int fd=0;
	char devpath[100];
	sprintf(devpath, "%s", devname);
	if((fd = open(devpath, O_RDWR | O_NDELAY)) > 0)
	{
		write(fd,&data,sizeof(int));
		close(fd);
		return 1;
	}
	return 0;
}

void esam_init()
{
	gpio_write("/dev/gpoESAM_RST",0);
	usleep(100000);
	gpio_write("/dev/gpoESAM_RST",1);
//	usleep(100000);
//	gpio_write("/dev/gpoGPRS_POWER",0);
	sleep(1);
//	gpio_write("/dev/gpoESAM_RST",0);
//	usleep(100000);
//	gpio_write("/dev/gpoESAM_RST",1);
//	sleep(3);
//	gpio_write("/dev/gpoESAM_CS", 1);
//	gpio_write("/dev/gpoATT_CS",1);
}

INT32S com_open(INT8U port,INT32S baud,INT8U *parity,INT8U stopb,INT8U bits)
{
	INT32S ComPort=0;
	struct termios old_termi,new_termi;
	int baud_lnx=0;
	unsigned char tmp[128];
	memset(tmp,0,128);

	sprintf((char *)tmp,"/dev/ttyS%d",port);
	ComPort = open((char *)tmp, O_RDWR);/* 打开串口文件 */
//	fprintf(stderr, "\n485I  tmp ==%s OpenCom485 fd =%d\n",tmp, ComPort);
	if( ComPort<0 )
	{
		fprintf(stderr,"open the serial port fail! errno is: %d\n", errno);
		return -1; /*打开串口失败*/
	}
	if ( tcgetattr( ComPort, &old_termi) != 0) /*存储原来的设置*/
	{
		printf("get the terminal parameter error when set baudrate! errno is: %d\n", errno);
		/*获取终端相关参数时出错*/
		return 0;
	}
   // printf("\n\r c_ispeed == %d old_termi  c_ospeed == %d",old_termi.c_ispeed, old_termi.c_ospeed);
	bzero(&new_termi,sizeof(new_termi));    				/*将结构体清零*/
	new_termi.c_cflag|= (CLOCAL|CREAD); 					/*忽略调制解调器状态行，接收使能*/
	new_termi.c_lflag&=~(ICANON|ECHO|ECHOE);				/*选择为原始输入模式*/
	new_termi.c_oflag&=~OPOST; 								/*选择为原始输出模式*/
	new_termi.c_cc[VTIME] = 1; 								/*设置超时时间为0.5 s*/
	new_termi.c_cc[VMIN] = 0;								/*最少返回的字节数是 7*/
	new_termi.c_cflag &= ~CSIZE;
	//new_termi.c_iflag &= ~INPCK;     /* Enable parity checking */
	new_termi.c_iflag &=~ ISTRIP;
	switch(baud)
	{
		case 1200:
			baud_lnx = B1200;
			break;
		case 2400:
			baud_lnx = B2400;
			break;
		case 4800:
			baud_lnx = B4800;
			break;
		case 9600:
			baud_lnx = B9600;
			break;
		case 19200:
			baud_lnx = B19200;
			break;
		case 38400:
			baud_lnx = B38400;
			break;
		case 57600:
			baud_lnx = B57600;
			break;
		case 115200:
			baud_lnx = B115200;
			break;
		default:
			baud_lnx = B9600;
			printf("\nSerial COM%d do not setup baud, default baud is 9600!!!", port);
			break;
	}

	switch( bits )
		{
			case 5:
				new_termi.c_cflag |= CS5;
				break;
			case 6:
				new_termi.c_cflag |= CS6;
				break;
			case 7:
				new_termi.c_cflag |= CS7;
				break;
			case 8:
				new_termi.c_cflag |= CS8;
				break;
			default:
				new_termi.c_cflag |= CS8;
				break;
		}

	if(strncmp((char *)parity,"even",4)==0)//设置奇偶校验为偶校验
	{
		//new_termi.c_iflag |= (INPCK | ISTRIP);
		new_termi.c_cflag |= PARENB;
		new_termi.c_cflag &= ~PARODD;

	}
	else if(strncmp((char *)parity,"odd",3)==0)  //设置奇偶校验为奇校验
	{
		new_termi.c_cflag |= PARENB;
		new_termi.c_cflag |= PARODD;
		//new_termi.c_iflag |= (INPCK | ISTRIP);
	}
	else
	{
		new_termi.c_cflag &= ~PARENB; 	//设置奇偶校验为无校验
		//new_termi.c_iflag &=~ ISTRIP;
	}


	if(stopb==1)//停止位
	{
		new_termi.c_cflag&= ~CSTOPB; //设置停止位为:一位停止位
	}
	else if(stopb==2)
	{
		new_termi.c_cflag |= CSTOPB; //设置停止位为:二位停止位
	}
	else
	{
		new_termi.c_cflag&= ~CSTOPB; //设置停止位为:一位停止位
	}

	cfsetispeed(&new_termi, baud_lnx); 							/* 设置输入拨特率 */
	cfsetospeed(&new_termi, baud_lnx); 							/* 设置输出拨特率 */

	tcflush(ComPort, TCIOFLUSH); 								/* 刷新输入输出流 */
	if(tcsetattr(ComPort,TCSANOW,&new_termi)!= 0)				/* 激活串口配置 */
	{
		printf("Set serial port parameter error!\n");
		return 0;
	}
	return ComPort;
}


void com_close(INT32S fd)
{
	if(fd >0)
		close(fd);
}

void esam_write(int fd, INT8U *buf, INT8U len)
{
	int i;
	INT8U sbuf[512];

	memset(sbuf, 0, 512);
	for(i=0; i<len; i++)
		sbuf[i] = buf[i];
	fprintf(stderr,"\nS(%d)",4);
	for(i=0;i<len;i++) {
		fprintf(stderr,"%02X ",sbuf[i]);
	}
	write(fd,sbuf,len);
}


//----------------------------------------------------------------------------//
void esam_read()
{
	INT8U 	tmpbuf[MYBUFLEN];
	int 	len,i;

	memset(tmpbuf, 0,MYBUFLEN);

	while(1)
	{
		len = read(Comfd,tmpbuf,MYBUFLEN);
		if(len>0) {
			fprintf(stderr,"\nR(%d)",len);
			for(i=0;i<len;i++) {
				fprintf(stderr,"%02X ",tmpbuf[i]);
			}
		}
	}
}


//主程序////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int esam_main(int argc, char *argv[])
{
	int len=0,i;
	int ret=0;
	INT8U Esam_Send[MYBUFLEN]={0x00,0x84,0x00,0x00,0x08};
	INT8U Esam_Recv[MYBUFLEN];
	INT8U PTSConfirmStatus = 1;
	INT8U T_0;
	int port=4;

	if(argc>2) {
		port = atoi(argv[1]);
	}
	fprintf(stderr, "\n开始测试 ESAM 端口 %d", port);
	Comfd = com_open(port,9600,(INT8U *)"even",1,8);
	fprintf(stderr,"open ttyS%d\n",port);
	if(Comfd==-1) {
		fprintf(stderr,"open /dev/ttyS%d failed\n",port);
		return EXIT_SUCCESS;
	}
	sleep(1);
	fprintf(stderr,"esam init\n");
	esam_init();

	//esam_write(Comfd,Esam_Send,count);
	memset(Esam_Recv, 0, MYBUFLEN);
	//esam_read();
	len = read(Comfd,Esam_Recv,MYBUFLEN);
	if(len>0) {
		fprintf(stderr,"\nR(%d)",len);
		for(i=0;i<len;i++) {
			fprintf(stderr,"%02X ",Esam_Recv[i]);
		}
	}else
		return 0;


	if (Esam_Recv[0] == 0x3b) {
		if ((Esam_Recv[1] & 0x10) == 0x10) {
			if ((Esam_Recv[2] & 0x11) != 0x11) {
				Esam_Send[0] = 0xff;
				Esam_Send[1] = 0x10;
				Esam_Send[2] = Esam_Recv[2];
				Esam_Send[3] = Esam_Send[0] ^ Esam_Send[1] ^ Esam_Send[2];
				fprintf(stderr,"\nS(%d)",4);
				for(i=0;i<4;i++) {
					fprintf(stderr,"%02X ",Esam_Send[i]);
				}
				write(Comfd,Esam_Send,4);
				sleep(2);
				memset(Esam_Recv,0,MYBUFLEN);
				len = read(Comfd,Esam_Recv,MYBUFLEN);
				if(len>0) {
					fprintf(stderr,"\nR(%d)",len);
					for(i=0;i<len;i++) {
						fprintf(stderr,"%02X ",Esam_Recv[i]);
					}
				}
				//判断PTSConfirmStatus...
				if(PTSConfirmStatus==1) {
					T_0 = Esam_Recv[2];
					com_close(Comfd);
					sleep(1);
					Comfd = -1;
					Comfd = com_open(port,57600,(INT8U *)"even",1,8);
					fprintf(stderr,"\nnew open Comfd=%d\n",Comfd);
					sleep(3);
					Esam_Send[0] = 0x00;
					Esam_Send[1] = 0x84;
					Esam_Send[2] = 0x00;
					Esam_Send[3] = 0x00;
					Esam_Send[4] = 0x08;
					fprintf(stderr,"\nS(%d)",5);
					for(i=0;i<5;i++) {
						fprintf(stderr,"%02X ",Esam_Send[i]);
					}
					write(Comfd,Esam_Send,5);
					sleep(3);
					memset(Esam_Recv, 0, MYBUFLEN);
					len = 0;
					{
						len = read(Comfd,Esam_Recv,MYBUFLEN);
						fprintf(stderr,"read len=%d\n",len);
						if(len>0) {
							fprintf(stderr,"\nR(%d)",len);
							for(i=0;i<len;i++) {
								fprintf(stderr,"%02X ",Esam_Recv[i]);
							}
						}
						for(i=0; i<len; i++)
						{
							if(Esam_Recv[i]==0x90 && Esam_Recv[i+1]==0x00){
								fprintf(stderr,"        ESAM OK\n");
								ret = 1;
							}
						}
					}
				}
			}
		}
	}
	com_close(Comfd);
	return ret;
}
