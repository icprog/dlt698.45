#ifndef JPublicFunctionH
#define JPublicFunctionH
#include "stdio.h"
#include "zlib.h"
#include "errno.h"
#include "time.h"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <sys/mman.h>
#include <termios.h>
#include "math.h"
#include "PublicFunction.h"

#include <linux/serial.h>
#include <sys/ioctl.h>

/* BCD码转int32u
 *参数：bcd为bcd码头指针，len为bcd码长度，order为positive正序/inverted反序，dint转换结果
 * 返回:0：成功；-1：asc为空；-2：en为0；-3：order有误
 * 例如:0x12 0x34 -> 1234
 * */

#define PIN_BASE		32
#define	 AT91_PIN_PC1	(PIN_BASE + 0x40 + 1)
#define 	AT91_PIN_PA7	(PIN_BASE + 0x00 + 7)

INT8S bcd2int32u(INT8U *bcd, INT8U len,ORDER order,INT32U* dint)
{
	int i=0;
	if(bcd == NULL)
		return -1;
	if(len<=0)
		return -2;
	*dint = 0;
	if(order == inverted)
	{
		for (i = len; i > 0; i--) {
			*dint = (*dint * 10) + ((bcd[i - 1] >> 4) & 0xf);
			*dint = (*dint * 10) + (bcd[i - 1] & 0xf);
		}
	}
	else if(order == positive)
	{
		for (i = 0; i < len; i++) {
			*dint = (*dint * 10) + ((bcd[i] >> 4) & 0xf);
			*dint = (*dint * 10) + (bcd[i] & 0xf);
		}
	}else
		return -3;
	return 0;
}

INT32S getIntBits(INT32U dint)
{
	int ret=0;
	do{
		dint /=10;
		ret++;
	}while(dint>0);
	return ret;
}

INT32S int32u2bcd(INT32U dint32, INT8U* bcd,ORDER order)
{
	INT8U i;
	INT16U mod = 1;
	INT32S len= 0;
	len = getIntBits(dint32);
	if(len%2 >0)
		len ++;
	if(order == positive)
	{
		for (i = 0; i < len/2; i++) {
			mod = dint32 % 100;
			bcd[len/2-i-1] = (mod / 10);
			bcd[len/2-i-1] = (bcd[len/2-i-1] << 4) + (mod % 10);
			dint32 = dint32 / 100;
		}
	}else if(order == inverted)
	{
		for (i = 0; i < len/2; i++) {
			mod = dint32 % 100;
			bcd[i] = (mod / 10);
			bcd[i] = (bcd[i] << 4) + (mod % 10);
			dint32 = dint32 / 100;
		}
	}
	else
		return -1;
	return len/2;
}

void* CreateShMem(char* shname,int memsize,void* pmem)
{
	int fd=0;
	void *memptr=NULL;

	fd = shm_open(shname, O_RDWR | O_CREAT, 0777);
	if (fd == -1) {
		fprintf(stderr, "\nERROR: Create share %s memory failed:%s!\n", shname,strerror(errno));
		return NULL;
	}
	if (ftruncate(fd, memsize) == -1) {
		fprintf(stderr, "ltrunc %s: %s\n", shname,strerror(errno));
		return NULL;
	}
	memptr = mmap(0, memsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (memptr == MAP_FAILED) {
		fprintf(stderr, "mmap %s failed: %s\n", shname,strerror(errno));
		return NULL;
	}
	close(fd);
	return memptr;
}
void* OpenShMem(char* shname,int memsize,void* pmem)
{
	int fd=-1;
	fd = shm_open(shname, O_RDWR, 0777);
	if (fd == -1) {
		fprintf(stderr, "\nERROR: Open share memory %s failed:%s!\n", shname,strerror(errno));
		return NULL;
	}
	pmem = mmap(0, memsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd,0);
	if (pmem == MAP_FAILED) {
		fprintf(stderr, "mmap %s failed: %s\n",shname, strerror(errno));
		return NULL;
	}
	close(fd);
	return pmem;

}

void Setsig(struct sigaction *psa,void (*pfun)(ProjectInfo *proinfo))
{
	if (psa!=NULL)
	{
		psa->sa_handler = pfun;//主程序退出时杀死所有子进程，清空共享内存
		sigemptyset(&psa->sa_mask);//用来将参数 信号集初始化并清空。
		psa->sa_flags = 0;
		sigaction(SIGTERM, psa,NULL);
		sigaction(SIGSYS, psa,NULL);
		sigaction(SIGPWR, psa,NULL);
		sigaction(SIGKILL,psa,NULL);
		sigaction(SIGQUIT, psa,NULL);
		sigaction(SIGILL, psa,NULL);
		sigaction(SIGINT, psa,NULL);
		sigaction(SIGHUP, psa,NULL);
		sigaction(SIGABRT, psa,NULL);
		sigaction(SIGBUS, psa,NULL);
	}
}

int OpenCom(int port,int baud,unsigned char *par,unsigned char stopb,unsigned char bits)
{

	///lhl
	#define RTS485			0x542D
	#define TIOCGRS485      0x542E
	#define TIOCSRS485      0x542F

	struct serial_rs485 rs485conf;
	int 	rs485gpio;


	int ComPort=0;
	struct termios old_termi={},new_termi={};
	int baud_lnx=0;
	unsigned char tmp[20]={};
	memset(tmp,0,sizeof(tmp));
    sprintf((char *)tmp,"/dev/ttyS%d",port);
    ComPort = open((char *)tmp, O_RDWR|O_NOCTTY);/* 打开串口文件 */
    if( ComPort<0 )
    {
    	printf("open the serial port fail! errno is: %d\n", errno);
    	return 0; /*打开串口失败*/
    }
    if ( tcgetattr( ComPort, &old_termi) != 0) /*存储原来的设置*/
    {
    	printf("get the terminal parameter error when set baudrate! errno is: %d\n", errno);
    	/*获取终端相关参数时出错*/
    	return 0;
    }
    bzero(&new_termi,sizeof(new_termi));    				/*将结构体清零*/
    new_termi.c_cflag|= (CLOCAL|CREAD); 					/*忽略调制解调器状态行，接收使能*/
    new_termi.c_lflag&=~(ICANON|ECHO|ECHOE);				/*选择为原始输入模式*/
    new_termi.c_oflag&=~OPOST; 								/*选择为原始输出模式*/
    new_termi.c_cc[VTIME] = 5; 								/*设置超时时间为0.5 s*/
    new_termi.c_cc[VMIN] = 0;								/*最少返回的字节数是 7*/
    new_termi.c_cflag &= ~CSIZE;
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
    		baud_lnx = B2400;
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

    if(strncmp((char *)par,"even",4)==0)//设置奇偶校验为偶校验
    {
    	new_termi.c_cflag |= PARENB;
    	new_termi.c_cflag &= ~PARODD;
    }
    else if(strncmp((char *)par,"odd",3)==0)  //设置奇偶校验为奇校验
	{
		new_termi.c_cflag |= PARENB;
		new_termi.c_cflag |= PARODD;
	}else
	{
    	new_termi.c_cflag &= ~PARENB; 	//设置奇偶校验为无校验
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

	if ((port==S4851)||(port==S4852))
	{
	 //lhl
		/* Enable RS485 mode: */
		memset(&rs485conf,0,sizeof(rs485conf));
		rs485conf.flags |= SER_RS485_ENABLED;

		/* Set logical level for RTS pin equal to 1 when sending: */
	 //	rs485conf.flags |= SER_RS485_RTS_ON_SEND;
	 //	/* or, set logical level for RTS pin equal to 0 when sending: */
		rs485conf.flags &= ~(SER_RS485_RTS_ON_SEND);

		/* Set logical level for RTS pin equal to 1 after sending: */
		rs485conf.flags |= SER_RS485_RTS_AFTER_SEND;
		/* or, set logical level for RTS pin equal to 0 after sending: */
	 //	rs485conf.flags &= ~(SER_RS485_RTS_AFTER_SEND);

		if (ioctl (ComPort, TIOCSRS485, &rs485conf) < 0) {
			fprintf(stderr,"ioctl TIOCSRS485 error\n");
		}

		if (port==S4851)
			rs485gpio=AT91_PIN_PC1;		//上海485I：AT91_PIN_PC1， 485II：AT91_PIN_PA7 ，这里要根据不同的口进行不同的设置，需修改
		else if (port==S4852)
			rs485gpio=AT91_PIN_PA7;
		else
			rs485gpio=AT91_PIN_PC1;
		if (ioctl (ComPort, RTS485, &rs485gpio) < 0) {
			/* Error handling. See errno. */
			fprintf(stderr,"ioctl RTS485 error\n");
		}
		fprintf(stderr,"rs485gpio=%d,ComPort=%d\n",rs485gpio,ComPort);
	}





    return ComPort;
}
//关闭串口
void CloseCom(int ComPort)
{
	close(ComPort);
}
//获取时间
void TSGet(TS *ts)
{
	struct tm tmp_tm;
	time_t time_of_day;
	time_of_day = time(NULL);
	localtime_r(&time_of_day, &tmp_tm);
	ts->Year = tmp_tm.tm_year + 1900;
	ts->Month = tmp_tm.tm_mon + 1;
	ts->Day = tmp_tm.tm_mday;
	ts->Hour = tmp_tm.tm_hour;
	ts->Minute = tmp_tm.tm_min;
	ts->Sec = tmp_tm.tm_sec;
	ts->Week = tmp_tm.tm_wday;
}

/****************************************************************
 * 实时数据请求缓冲数组初始化
 */
void InitRealdataReq(RealdataReq *req)
{
	int i=0;
	req->Ticket_g = 0;
	for(i=0; i<REALDATA_LIST_LENGTH ;i++)
	{
		req->RealdataList[i].type = 0;
		req->RealdataList[i].ticket = -1;
		req->RealdataList[i].stat = -1;
	}
}
/******************************************************************
 * 实时数据请求进入缓冲数组
 * 返回： -1缓冲区满   非负整数: 本次进入缓冲数组实时数据请求的唯一标识
 * 数组元素中 ticket: 请求标识
 */
int SetRealdataReq(RealdataReq* req,TRANSTYPE* data)
{
	int i=0;
	INT16U t=0;
	for(i=0 ; i<REALDATA_LIST_LENGTH ; i++)/*遍历实时数据缓冲数组*/
	{
		if(req->RealdataList[i].stat == -1)
		{
			memcpy(&req->RealdataList[i].realdata,data,sizeof(TRANSTYPE));
			req->RealdataList[i].ticket = req->Ticket_g++;
			req->RealdataList[i].stat = 0;
			t = req->RealdataList[i].ticket ;
			return t;
		}
	}
    return -1;
}
/***************************************************************
 * 读取指定端口实时数据请求
 * 返回：1:读取成功  0:失败
 */
int GetRealdataReq(RealdataReq* req,TRANSTYPE* data,INT8U port)
{
	int i=0;
	for(i=0 ; i<REALDATA_LIST_LENGTH ; i++)/*遍历实时数据缓冲数组*/
	{
		if (req->RealdataList[i].stat == 1 && req->RealdataList[i].realdata.Buff[0]==port)
		{
			memcpy(data,&req->RealdataList[i].realdata,sizeof(TRANSTYPE));
			req->RealdataList[i].stat = 2;
			return 1;
		}
	}
	return 0;
}
/******************************************************************
 * 实时数据请求 处理结果更新到请求数组
 * 数组元素中 ticket: 请求标识
 */
void UpdateRealReq_data(RealdataReq* req,TRANSTYPE* data,INT16U ticket)
{
	int i=0;
	for(i=0 ; i<REALDATA_LIST_LENGTH ; i++)/*遍历实时数据缓冲数组*/
	{
		if (req->RealdataList[i].ticket == ticket )
		{
			memcpy(data,&req->RealdataList[i].realdata,sizeof(TRANSTYPE));
			req->RealdataList[i].stat = 3;
			break;
		}
	}
}
/******************************************************************
 * 获取请求的实时数据
 * 返回： -1:无此标识   非负整数: 返回list[i].stat 数据状态标志 1:就绪  2:下发中... 3:完成
 * 数组元素中 ticket: 本次查询实时数据请求的唯一标识
 */
int GetRealdataReq_data(RealdataReq* req,TRANSTYPE* data,INT16U ticket)
{
	int i=0;
	INT32S stat=-1;
	for(i=0 ; i<REALDATA_LIST_LENGTH ; i++)/*遍历实时数据缓冲数组*/
	{
		if (req->RealdataList[i].ticket == ticket )
		{
			stat = req->RealdataList[i].stat;
			if (stat == 3)
			{
				memcpy(data,&req->RealdataList[i].realdata,sizeof(TRANSTYPE));
				req->RealdataList[i].stat = -1;
				req->RealdataList[i].ticket = -1;
				req->RealdataList[i].type = 0;
			}
			break;
		}
	}
    return stat;
}


#endif /*JPublicFunctionH*/
