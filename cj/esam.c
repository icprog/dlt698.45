/*
 * esam.c
 *
 *  Created on: Mar 30, 2017
 *      Author: lhl
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <semaphore.h>
#include <fcntl.h>
#include <linux/spi/spidev.h>

#include "StdDataType.h"
#include "Shmem.h"
#include "EventObject.h"
#include "PublicFunction.h"
#include "AccessFun.h"
#include "Objectdef.h"
#include "ParaDef.h"
#include "main.h"
#include "Esam.h"
#include "SPI.h"


extern ProgramInfo* JProgramInfo;
sem_t* sem_spi0_0;

int dumpstat_Test(char* name, int fd,uint32_t speed) {
    static uint8_t mode = 0;
    static uint8_t bits   = 8;

    if(speed==0) {
    	speed = 20000000;
    }else speed = speed*1000000;
    fprintf(stderr,"\nInit ESAM SPI %s, speed=%d\n",name,speed);

    mode = SPI_MODE_3;
   // mode |= SPI_CPHA;

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

int32_t SPI_Init_Test(int32_t fd, char* spipath,uint32_t speed) {
	if (fd != -1) {
		SPI_Close(fd);
	}

	fd = open((char*) spipath, O_RDWR);
	if (fd < 0)
		printf("[SPI ERROR] can't open  device %s\n", spipath);

	return dumpstat_Test((char*) spipath, fd,speed);
}


INT32S Esam_Init_Test(INT32S fd,uint32_t speed) {
    gpio_writebyte(DEV_ESAM_PWR, 0);
    usleep(50000);
//    gpio_writebyte(DEV_ATT_RST, 1);
//    usleep(2);
    gpio_writebyte(DEV_ESAM_CS, 1);
    return SPI_Init_Test(fd, ESAM_SPI_DEV,speed);
//    if(JProgramInfo->DevicePara[0]==2) {
//    	return SPI_Init_Test(fd, ESAM_SPI_DEV_II,speed);
//    }else {
////    	sem_spi0_0 = open_named_sem(SEMNAME_SPI0_0);
//    	return SPI_Init_Test(fd, ESAM_SPI_DEV,speed);
//    }
}

void Esam_Clear_Test(INT32S fd) {
//	if(JProgramInfo->DevicePara[0]!=2)  {
		close_named_sem(SEMNAME_SPI0_0);
//	}
    SPI_Close(fd);
}
/**********************************
 *向esam芯片写数据
 *输入：fd文件索引  Tbuf发送数据地址，Tlen发送长度
 *输出：无
 *说明：发送中时间间隔usleep严格限定，不要更改
 ***********************************/
void Esam_WriteToChip_Test(INT32S fd, INT8U* Tbuf, INT16U Tlen)
{
		struct spi_ioc_transfer	xfer[2];
		memset(xfer, 0,  sizeof xfer);
//		gpio_writebyte(DEV_ATT_CS,1);
//		usleep(5);
		gpio_writebyte(DEV_ESAM_CS,1);
		usleep(10);
		gpio_writebyte(DEV_ESAM_CS,0);
		usleep(20);
		xfer[0].tx_buf = (int)Tbuf;//发数据
		xfer[0].len =Tlen;
		ioctl(fd, SPI_IOC_MESSAGE(2), xfer);
		usleep(5);
		gpio_writebyte(DEV_ESAM_CS,1);
		usleep(10);
		gpio_writebyte(DEV_ESAM_CS,0);
		usleep(20);

		printf("\n Esam_WriteToChip:");
		int i=0;
		for( i=0;i<Tlen;i++)
			printf("%02X ",Tbuf[i]);
		printf("\n");
}
/**********************************
 *从esam芯片读取数据
 *输入：fd文件索引  Rbuf接收数据地址，Rlen接收数据长度
 *输出：无
 *说明：发送中时间间隔usleep严格限定，不要更改
 ***********************************/
void Esam_ReadFromChip_Test(INT32S fd, INT8U* Rbuf, INT8U Rlen)
{
			struct spi_ioc_transfer	xfer[2];
			memset(xfer, 0,  sizeof xfer);
			xfer[1].rx_buf = (int) Rbuf;
			xfer[1].len = Rlen;
			ioctl(fd, SPI_IOC_MESSAGE(2), xfer);
			usleep(50);//每次查询指令间隔时间在15us----100us之间，最大查询事件为20*50us==1s,外层循环最多3次，3s

//			printf("\n Esam_ReadFromChip:");
//			int i;
//			for( i=0;i<Rlen;i++)
//				printf("%02X ",Rbuf[i]);
//			printf("\n");
}

/*******************************************************
 * ESAM数据读写函数
 * 输入：Wbuf:ESAM指令流指针，为完整帧:0x55 CLA INS P1 P2 Len1 Len2 DATA LRC1
 * 		Wlen:ESAM指令流长度；
 * 输出：Rbuf：ESAM返回数据指针，SW1 SW2 Len1 Len2 DATA LRC2;
 * 		 Rlen:ESAM返回数据长度，上述接收数据完整帧的长度；
 *      函数返回：
 *		ESAM执行成功后函数返回，接收的ESAM返回完整帧的长度；
 *		Result：返回负数，代表发生错误，正数代表接收字节，正确；
 *		其它：SW1SW2状态错误字，错误状态见“Esam.h”
 *	说明：函数对于读写数据的长度有最大限制，为BUFFLENMAX_SPI
 *	           读取终端证书，共1499个字节，不能一次读取所有数据，每次读取200，否则失败。
 *******************************************************/
INT32S Esam_WriteThenRead_Test(INT32S fd, INT8U* Tbuf, INT16U Tlen, INT8U* Rbuf){
	if(fd < 0)  return ERR_ESAM_SPI_OPENERR;
	if(Tlen > BUFFLENMAX_SPI)  return ERR_ESAM_WRTBUF_OVERLEN;
	if(Tlen==0|| Tbuf==NULL ||Rbuf==NULL ) return ERR_ESAM_TRANSPARA_ERR;
	if(Tbuf[Tlen-1]!= LRC(Tbuf+1,Tlen-2))  return ERR_ESAM_SENDDATA_LRCERR;//发送前校验LRC
	INT8U index;
	INT16S Result = ERR_ESAM_UNKNOWN;
    INT8U rx[BUFFLENMAX_SPI];

//    if(JProgramInfo->DevicePara[0]!=2)
//    	sem_wait(sem_spi0_0);
	for(index=0;index<1;index++)//只做6次异常处理，每次若出异常，时间会很长，秒级
	{
		memset(rx,0x00,BUFFLENMAX_SPI);
		Esam_WriteToChip_Test(fd,Tbuf,Tlen);//向片中发送数据
		INT8U i=0;
		do{
			memset(rx,0x00,1);
			Esam_ReadFromChip_Test(fd,rx,1);//读取1个字符
			i++;
			if(i>=50) break;
		}while(rx[0]!=MARK_ESAM);

		if(rx[0]==MARK_ESAM)
		{
			memset(rx,0x00,4);
			Esam_ReadFromChip_Test(fd,rx,4);//读取SW1 SW2 Len1 Len2
		}
		else  //未取回0x55字节数据，重新开始Esam_WriteToChip，向片内写数据
		{
			Result=ERR_ESAM_WAIT_TIMEOUT;
			usleep(50);
			continue;
		}
		if((rx[0] == 0x90)&&(rx[1] == 0x00))//接收数据状态判断
		{
			INT32S length =((0xff & rx[3])|(0xff00 & (rx[2]<<8)));
			//fprintf(stderr,"Esam_WriteThenRead length = %d\n",length);
			if(length>=0 && length<=2000)
			{
				for(i=0;i<(length/200)+1;i++)//读取DATA LRC2  每次不能读的太多，200一个间隔
				{
					if(i==length/200)
						Esam_ReadFromChip_Test(fd,rx+RevHeadLen_ESAM+i*200,(length+1)%200);
					else
						Esam_ReadFromChip_Test(fd,rx+RevHeadLen_ESAM+i*200,200);
				}
			}
			else
			{
				Result=ERR_ESAM_RETDATA_OVERLEN;
				usleep(50);
				continue;
			}

			if(rx[RevHeadLen_ESAM+length] == LRC(rx,RevHeadLen_ESAM+length))//LRC校验
			{
				Result = RevHeadLen_ESAM+length+1;//返回数据长度
				memcpy(Rbuf,rx,Result);
				break;
			}
			else//LRC校验错误
			{
				Result = ERR_ESAM_REVDATA_LRCERR;
				usleep(50);
				continue;
			}
		}
		else
		{
			Result = Esam_ErrMessageCheck(rx);
		}
	}
	gpio_writebyte(DEV_ESAM_CS,1);
//    if(JProgramInfo->DevicePara[0]!=2)
//    	sem_post(sem_spi0_0);
	return Result;
}

INT32S Esam_GetTermiSingleInfo_Test(INT32S fd, INT8U type, INT8U* Rbuf) {
    INT8U GetInfo_ESAM[]    = { 0x55, 0x80, 0x36, 0x00, 0xFF, 0x00, 0x00,0x00 };
    if(type!=0x0B && type!=0x0C)
    	return -1;
    INT32S Result=0;
    INT8U tmp[BUFFLENMAX_SPI];
    memset(tmp,0,BUFFLENMAX_SPI);
    GetInfo_ESAM[4] = type;
    GetInfo_ESAM[7] = LRC(&GetInfo_ESAM[1], 6);
    //int i;
//    for(i=0;i<8;i++)
//    	fprintf(stderr,"%02x ",GetInfo_ESAM[i]);
//    fprintf(stderr,"\n");
    Result = Esam_WriteThenRead_Test(fd, (INT8U*)GetInfo_ESAM, 8, tmp);
//    if(Result>0 && Result<BUFFLENMAX_SPI) //大于BUFFLENMAX_SPI错误，此处做比较
//    {
//    	memcpy(Rbuf,&tmp[4],Result-5);
//    	return Result-5;
//    }
   return Result;
}

/**********************************
 *T-ESAM指令：批量获取终端信息
 *输出：*Rbuf：接收终端信息缓冲区指针：
 *			ESAM序列号（8Byte）+ ESAM 版本号（4B）+对称密钥版本（16B） + 主站证书版本号(1B) +终端证书版本号(1B) +
 *			会话时效门限（4B）+会话时效剩余时间（4B）+单地址应用协商计数器（4B）+主动上报计数器（4B）+应用广播通信序列号（4B）+
 *			终端证书序列号(16B) +主站证书序列号（16B）
 *函数返回：1、为正数是为终端信息数据长度		  2、负数：代表相应错误，见：Esam.h中，ESAM ERR ARRAY定义
 *************************************************************/
INT32S Esam_GetTermiInfo_Test(INT32S fd, EsamInfo* esamInfo) {
    INT8U GetSerialNum_ESAM[] = { 0x55, 0x80, 0x36, 0x00, 0xFF, 0x00, 0x00, 0x00};
    GetSerialNum_ESAM[7] = LRC(&GetSerialNum_ESAM[1], 6);

    INT32S Result=0;
    INT32S index=4;
    INT8U Rbuf[BUFFLENMAX_SPI];
	if((Result = Esam_WriteThenRead_Test(fd, (INT8U*)GetSerialNum_ESAM, 8, Rbuf)) > 0){
		memcpy(esamInfo->EsamSID,&Rbuf[index],8);//ESAM序列号
		index+=8;
		memcpy(esamInfo->EsamVID,&Rbuf[index],4); //ESAM版本号
		index+=4;
		memcpy(esamInfo->SecretKeyVersion,&Rbuf[index],16); //对称密钥版本
		index+=16;
		esamInfo->CcieVersion.ServerCcieVersion=Rbuf[index++]; //主站证书版本
		esamInfo->CcieVersion.TerminalCcieVersion=Rbuf[index++]; //终端证书版本
		memcpy(esamInfo->SessionTimeHold,&Rbuf[index],4);//会话时效门限
		index+=4;
		memcpy(esamInfo->SessionTimeLeft,&Rbuf[index],4);//会话剩余时间
		index+=4;
		memcpy(esamInfo->CurrentCounter.SingleAddrCounter,&Rbuf[index],4);//单地址应用协商计数器
		index+=4;
		memcpy(esamInfo->CurrentCounter.ReportCounter,&Rbuf[index],4);//主动上报计数器
		index+=4;
		memcpy(esamInfo->CurrentCounter.BroadCastSID,&Rbuf[index],4);//应用广播通信序列号
		index+=4;
		memcpy(esamInfo->TerminalCcieSID,&Rbuf[index],16); //终端证书序列号
		index+=16;
		memcpy(esamInfo->ServerCcieSID,&Rbuf[index],16);//主站证书序列号
	}
    return Result;
}

void writeLog(uint32_t speed,INT16U num,INT16S test1,INT16S test2)
{
	FILE *fp=NULL;
	char cmd[512]={};
	static  int err1=0,err2=0;
	static  uint32_t lastspeed=0;
	if(speed != lastspeed) {
		lastspeed = speed;
		err1 = 0;
		err2 = 0;
	}
	fp = fopen("/nand/esam.log","a+");
	if(fp!=NULL) {
		memset(cmd,0,sizeof(cmd));
		if(test1<0) err1++;
		if(test2<0) err2++;
		sprintf(cmd,"SPI速率:%d M,发送帧:%d ,主站证书返回:%d,  终端信息返回:%d,  证书错误:%d,  终端信息错误:%d\r\n",speed,num,test1,test2,err1,err2);
		fwrite(cmd,sizeof(cmd),1,fp);
		fclose(fp);
	}
}

INT16S TermiSingleInfo_Test(uint32_t speed)
{
	INT32S fp_spi = -1;
	INT16S retlen=0;
	INT8U	buf[2048]={};

    fp_spi = Esam_Init_Test(fp_spi,speed);
	fprintf(stderr,"Esam_Init fp_spi=%d\n",fp_spi);
	if(fp_spi < 0)
		fprintf(stderr,"\nesam device cannot open\n");
	retlen = 0;
	retlen = Esam_GetTermiSingleInfo_Test(fp_spi,0x0b,buf);
    fprintf(stderr,"Esam_GetTermiSingleInfo result = %d\n",retlen);
    if(retlen>=0) fprintf(stderr,"主站证书 OK\n");
    switch(retlen) {
    case ERR_ESAM_WRTBUF_OVERLEN:
    	fprintf(stderr,"写入缓冲长度过长\n");
    	break;
    case ERR_ESAM_WAIT_TIMEOUT:
    	fprintf(stderr,"没有检测到0x55\n");
    	break;
    case ERR_ESAM_SW1SW2_ALLZERO:
    	fprintf(stderr,"接收数据状态字全零异常\n");
    	break;
    case ERR_ESAM_RETDATA_OVERLEN:
    	fprintf(stderr,"ESAM返回的LEN代表的长度数据过长\n");
    	break;
    case ERR_ESAM_SENDDATA_LRCERR:
    	fprintf(stderr,"发送数据LRC校验错误\n");
    	break;
    case ERR_ESAM_REVDATA_LRCERR:
    	fprintf(stderr,"接收数据LRC校验错误\n");
    	break;
    case ERR_ESAM_SPI_OPENERR:
    	fprintf(stderr,"ESAM SPI设备打开失败\n");
    	break;
    case ERR_ESAM_TRANSPARA_ERR:
    	fprintf(stderr,"传入参数错误\n");
    	break;
    }
	Esam_Clear_Test(fp_spi);
    return retlen;
}

INT16S TermiInfo_Test(uint32_t speed)
{
	INT32S fp_spi = -1;
	INT16S retlen=0,i=0;
	INT8U	buf[2048]={};
	EsamInfo	esamInfo;

    fp_spi = Esam_Init_Test(fp_spi,speed);
//	fprintf(stderr,"Esam_Init fp_spi=%d\n",fp_spi);
	if(fp_spi < 0)
		fprintf(stderr,"\nesam device cannot open\n");
	retlen = Esam_GetTermiInfo_Test(fp_spi,&esamInfo);
	if(retlen > 0)
	{
		fprintf(stderr,"\n esam 信息如下:\n");
		fprintf(stderr," 芯片序列号  版本号  对称密钥版本 密钥版本 终端证书序列号 主站证书序列号\n");
		for(i = 0;i<8;i++) //1.芯片序列号8字节
		{
			fprintf(stderr,"%02x",esamInfo.EsamSID[i]);
		}
		fprintf(stderr,"  ");
		for(i = 0;i<4;i++) //2.ESAM版本号
		{
			fprintf(stderr,"%02x",esamInfo.EsamVID[i]);
		}
		fprintf(stderr,"  ");
		for(i = 0;i<16;i++)//3.对称密钥版本16字节
		{
			fprintf(stderr,"%02x",esamInfo.SecretKeyVersion[i]);
		}
		fprintf(stderr,"  ");
		for(i = 0;i<16;i++)//
		{
			fprintf(stderr,"%02x",esamInfo.TerminalCcieSID[i]);
		}
		fprintf(stderr,"  ");
		for(i = 0;i<16;i++)//
		{
			fprintf(stderr,"%02x",esamInfo.ServerCcieSID[i]);
		}
		fprintf(stderr,"  ");
	}
    if(retlen<0)   fprintf(stderr,"未获取到esam信息\n");
	Esam_Clear_Test(fp_spi);
    return retlen;
}

void EsamTest(int argc, char* argv[])
{
	INT16S test1=0,test2=0;
	INT32S fp_spi = -1;
	INT16U i,j,testnum = 1;
	uint32_t	speed=20,maxspeed=20;

	if(argc==3) {
		speed = atoi(argv[2]);
		maxspeed = speed;
	}else if(argc==4) {
		speed = atoi(argv[2]);
		maxspeed = speed;
		testnum = atoi(argv[3]);
	}
	else if(argc==5) {
		speed = atoi(argv[2]);
		maxspeed = atoi(argv[3]);
		testnum = atoi(argv[4]);
	}
	JProgramInfo = OpenShMem("ProgramInfo", sizeof(ProgramInfo), NULL);
	fprintf(stderr,"JProgramInfo =%p",JProgramInfo);
	for(i=speed;i<=maxspeed;i++) {
		for(j=0;j<testnum;j++){
			test2 = TermiInfo_Test(i);
			sleep(1);
			test1 = TermiSingleInfo_Test(i);
			sleep(2);
//			writeLog(i,j,test1,test2);
		}
	}
	shmm_unregister("ProgramInfo", sizeof(ProgramInfo));
}


