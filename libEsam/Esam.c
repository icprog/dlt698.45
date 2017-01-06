/*
 * Esam.c
 *
 *  Created on: 2013-6-26
 *  Author: Administrator
 */

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <semaphore.h>
#include "../libBase/PublicFunction.h"
#include "../include/ParaDef.h"
#include "Esam.h"
#include "SPI.h"

sem_t* sem_spi0_0;
/*
 * TODO:传入该库中参数Data，长度可能大于255的，后面都必须跟参数len，2字节，确定不大于255的，传指针，第一个字节为长度
 * 主站下发报文，解析成字段后，依次传入该库，不要把子串再组合传入，目的是将业务层剥离开，库外不需要知道ESAM如何组串。
 */
// LRC 对T-ESAM指令流校验 函数
//返回校验结果
INT8U LRC(INT8U* buf, INT32U len) {
    if (buf == NULL || len <= 0)
        return -1;
    INT32U i  = 0;
    INT8U ret = buf[0];
    for (i = 0; i < len - 1; i++) {
        ret ^= buf[i + 1];
    }
    return ~ret;
}
//指定长度的两种数据异或后输出
INT16S AxorB(INT8U* abuf, INT8U* bbuf, INT8U* Rbuf, INT8U len) {
    INT8U i = 0;
    if (abuf == NULL || bbuf == NULL || Rbuf == NULL || len <= 0)
        return -1;
    for (i = 0; i < len; i++) {
        Rbuf[i] = abuf[i] ^ bbuf[i];
    }
    return 1;
}
//TODO:在vmain主进程中，需要有nsem_creat（）创建该信号量。
INT32S Esam_Init(INT32S fd, INT8U* spipath) {
    gpio_writebyte((INT8S*)DEV_ESAM_PWR, 0);
    usleep(50000);
    gpio_writebyte((INT8S*)DEV_ATT_RST, 1);
    usleep(2);
    gpio_writebyte((INT8S*)DEV_ESAM_CS, 1);
    sem_spi0_0 = nsem_open(SEMNAME_SPI0_0);
    return SPI_Init(fd, spipath);
}

void Esam_Clear(INT32S fd) {
    sem_close(sem_spi0_0);
    sem_spi0_0 = NULL;
    SPI_Close(fd);
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
INT32S Esam_WriteThenRead(INT32S fd, INT8U* Tbuf, INT8U Tlen, INT8U* Rbuf){
	if(fd < 0)  return ERR_ESAM_SPI_OPENERR;
	if(Tlen > BUFFLENMAX_SPI)  return ERR_ESAM_WRTBUF_OVERLEN;
	if(Tlen==0|| Tbuf==NULL ||Rbuf==NULL ) return ERR_ESAM_TRANSPARA_ERR;
	if(Tbuf[Tlen-1]!= LRC(Tbuf+1,Tlen-2))  return ERR_ESAM_SENDDATA_LRCERR;//发送前校验LRC
	INT8U index;
	INT16S Result = ERR_ESAM_UNKNOWN;
    INT8U rx[BUFFLENMAX_SPI];

	sem_wait(sem_spi0_0);

	for(index=0;index<6;index++)//只做3次异常处理，每次若出异常，时间会很长，秒级
	{
		memset(rx,0x00,BUFFLENMAX_SPI);
		Esam_WriteToChip(fd,Tbuf,Tlen);//向片中发送数据

		INT8U i=0;
		do{
			memset(rx,0x00,1);
			Esam_ReadFromChip(fd,rx,1);//读取1个字符
			i++;
			if(i>=20) break;
		}while(rx[0]!=MARK_ESAM);

		if(rx[0]==MARK_ESAM)//TODO:此处可以根据测试成功率，增加读取次数
		{
			memset(rx,0x00,4);
			Esam_ReadFromChip(fd,rx,4);//读取SW1 SW2 Len1 Len2
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
			if(length>=0 && length<=2000)
			{
				for(i=0;i<(length/200)+1;i++)//读取DATA LRC2  每次不能读的太多，200一个间隔
				{
					if(i==length/200)
						Esam_ReadFromChip(fd,rx+RevHeadLen_ESAM+i*200,(length+1)%200);
					else
						Esam_ReadFromChip(fd,rx+RevHeadLen_ESAM+i*200,200);
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
	gpio_writebyte((INT8S*)DEV_ESAM_CS,1);
	sem_post(sem_spi0_0);
	return Result;
}
/**********************************
 *向esam芯片写数据
 *输入：fd文件索引  Tbuf发送数据地址，Tlen发送长度
 *输出：无
 *说明：发送中时间间隔usleep严格限定，不要更改
 ***********************************/
void Esam_WriteToChip(INT32S fd, INT8U* Tbuf, INT8U Tlen)
{
		struct spi_ioc_transfer	xfer[2];
		memset(xfer, 0,  sizeof xfer);
		usleep(5);
		gpio_writebyte((INT8S*)DEV_ESAM_CS,1);
		usleep(10);
		gpio_writebyte((INT8S*)DEV_ESAM_CS,0);
		usleep(20);
		xfer[0].tx_buf = (int)Tbuf;//发数据
		xfer[0].len =Tlen;
		ioctl(fd, SPI_IOC_MESSAGE(2), xfer);
		usleep(5);
		gpio_writebyte((INT8S*)DEV_ESAM_CS,1);
		usleep(10);
		gpio_writebyte((INT8S*)DEV_ESAM_CS,0);
		usleep(20);

		printf("\n Esam_WriteToChip:");
		int i;
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
void Esam_ReadFromChip(INT32S fd, INT8U* Rbuf, INT8U Rlen)
{
			struct spi_ioc_transfer	xfer[2];
			memset(xfer, 0,  sizeof xfer);
			xfer[1].rx_buf = (int) Rbuf;
			xfer[1].len = Rlen;
			ioctl(fd, SPI_IOC_MESSAGE(2), xfer);
			usleep(50);//每次查询指令间隔时间在15us----100us之间，最大查询事件为20*50us==1s,外层循环最多3次，3s

			printf("\n Esam_ReadFromChip:");
			int i;
			for( i=0;i<Rlen;i++)
				printf("%02X ",Rbuf[i]);
			printf("\n");
}
/**********************************
 *当接收到的SW1 SW2 是异常数据的时候，解析出何种异常
 *输入： Rbuf接收数据地址
 *输出：异常编号
 *说明：无
 ***********************************/
INT16S Esam_ErrMessageCheck(INT8U *RBuf)
{
	INT16S Result = ERR_ESAM_UNKNOWN;
	if((RBuf[0] == 0x00)&&(RBuf[1] == 0x00))
	{
		Result = ERR_ESAM_SW1SW2_ALLZERO;
	}
	else
	{
		Result = (((RBuf[0]<<8)&0xff00) | (RBuf[1]&0x00ff));
		if((0x9E20 <= (INT16U)Result)&&(0x9E3F >= (INT16U)Result))
		{
			Result &= 0xfff0;
		}
		INT8U i;
		for( i=0;i<ERRNUM_SW1SW2_ESAM;i++)
		{
			if(Result == SW1SW2[i])
			{
				Result = ERR_ESAM_CERTIorSWIT_Fail-i;
				break;
			}
		}
		if(i >= ERRNUM_SW1SW2_ESAM)
		{
			Result = ERR_ESAM_UNKNOWN;
		}
	}
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
INT32S Esam_GetTermiInfo(INT32S fd, EsamInfo* esamInfo) {
    INT8U GetSerialNum_ESAM[] = { 0x55, 0x80, 0x36, 0x00, 0xFF, 0x00, 0x00, 0x00};
    GetSerialNum_ESAM[7] = LRC(&GetSerialNum_ESAM[1], 6);

    INT32S Result=0;
    INT32S index=4;
    INT8U Rbuf[BUFFLENMAX_SPI];
	if((Result = Esam_WriteThenRead(fd, (INT8U*)GetSerialNum_ESAM, 8, Rbuf)) > 0){
		memcpy(esamInfo->EsamSID,&Rbuf[index],8);//ESAM序列号
		index+=8;
		memcpy(esamInfo->EsamVID,&Rbuf[index],4); //ESAM版本号
		index+=4;
		memcpy(esamInfo->SecretKeyVersion,&Rbuf[index],16); //对称密钥版本
		index+=16;
		esamInfo->CcieVersion.ServerCcieVersion=Rbuf[index++]; //主站证书版本
		esamInfo->CcieVersion.TerminalCcieVersion=Rbuf[index++]; //终端证书版本
		esamInfo->SessionTimeHold=CharToINT32U(&Rbuf[index]);//会话时效门限
		index+=4;
		esamInfo->SessionTimeLeft=CharToINT32U(&Rbuf[index]);//会话剩余时间
		index+=4;
		esamInfo->CurrentCounter.SingleAddrCounter =CharToINT32U(&Rbuf[index]);//单地址应用协商计数器
		index+=4;
		esamInfo->CurrentCounter.ReportCounter=CharToINT32U(&Rbuf[index]);//主动上报计数器
		index+=4;
		esamInfo->CurrentCounter.BroadCastSID=CharToINT32U(&Rbuf[index]);//应用广播通信序列号
		index+=4;
		memcpy(esamInfo->TerminalCcieSID,&Rbuf[index],16); //终端证书序列号
		index+=16;
		memcpy(esamInfo->ServerCcieSID,&Rbuf[index],16);//主站证书序列号
	}
    return Result;
}
INT32U CharToINT32U(INT8U *Buf)
{
	INT32U ret=0;
	ret=(Buf[0]<<24)&0xff000000;
	ret|=(Buf[1]<<16)&0x00ff0000;
	ret|=(Buf[2]<<8)&0x0000ff00;
	ret|=Buf[3]&0x000000ff;
	return ret;
}
/**********************************
 *T-ESAM指令：单项获取终端信息
 *发送：803600P20000
 *返回：9000+LEN+ Data2
 *发送报文中P2含义:
 *			02：ESAM 序列号(8B)、
			03：ESAM 版本号（4B） 、
			04：对称密钥版本（16B）、
			05：证书版本（主站证书版本号(1B)+ 终端证书版本号(1B) ）、
			06：会话时效门限（4B） 、
			07：会话时效剩余时间（4B） 、
			08：当前计数器（ASCTR：单地址应用协商计数器（4B）、ARCTR：主动上报计数器（4B）、AGSEQ：应用广播通信序列号（4B） ）
			09：终端证书序列号(16B)、
			0A：主站证书序列号（16B）
			0B：终端证书
			0C：主站证书
 *函数返回：1、为正数是为终端信息数据长度
 *                   2、Rbuf为主站或终端证书(已去除头部（4字节）和尾部（1字节校验）)
 *		  2、负数：代表相应错误，见：Esam.h中，ESAM ERR ARRAY定义
 *说明，本函数只负责超读0B，0C，证书1499字节报文。其他信息通过批量函数获取即可
 *************************************************************/
INT32S Esam_GetTermiSingleInfo(INT32S fd, INT8U type, INT8U* Rbuf) {
    INT8U GetInfo_ESAM[]    = { 0x55, 0x80, 0x36, 0x00, 0xFF, 0x00, 0x00,0x00 };
    if(type!=0x0B && type!=0x0C)
    	return -1;
    INT32S Result=0;
    INT8U tmp[BUFFLENMAX_SPI];
    memset(tmp,0,BUFFLENMAX_SPI);

    GetInfo_ESAM[4] = type;
    GetInfo_ESAM[7] = LRC(&GetInfo_ESAM[1], 6);
    Result = Esam_WriteThenRead(fd, (INT8U*)GetInfo_ESAM, 8, tmp);
    if(Result>0 && Result<BUFFLENMAX_SPI) //大于BUFFLENMAX_SPI错误，此处做比较
    {
    	memcpy(Rbuf,&tmp[4],Result-5);
    	return Result-5;
    }
   return Result;
}
/**********************************
 *建立应用链接（会话秘钥协商）
 *主站下发connect-request(connectmechanisminfo)数字签名3，ucOutSessionInit提取密文2和ucOutSign客户机签名2传入该函数
 *返回：Data（data）为connect-response连接响应对象服务器随机数和服务器签名信息（48+ucSessionData+m+ucSign）
 *函数返回：1、为正数是为信息总长度
 *		  2、负数：代表相应错误，见：Esam.h中，ESAM ERR ARRAY定义
 *函数说明:ucOutSessionInit和ucOutSign第一字节是数量
 *函数说明:返回的ucSessionData固定48字节，ucSign长度为length-48
 *************************************************************/
INT32S Esam_CreateConnect(INT32S fd, SignatureSecurity securityInfo ,SecurityData* RetInfo) {
	if(sizeof(securityInfo.signature)<=securityInfo.signature[0] || sizeof(securityInfo.encrypted_code2)<=securityInfo.encrypted_code2[0])
		return ERR_ESAM_TRANSPARA_ERR;//校验第一个字节长度是否小与数组长度
	 INT32S Result=0;
	 INT8U tmp[BUFFLENMAX_SPI];
	 memset(tmp,0,BUFFLENMAX_SPI);
	 INT16U len=0;
	 INT8U GetInfo_ESAM[BUFFLENMAX_SPI]={0x55,0x80,0x02,0x00,0x00};
	 len+=5;
	 INT16U datalen=securityInfo.signature[0]+securityInfo.encrypted_code2[0];
	 GetInfo_ESAM[5]=(INT8U)((datalen>>8)&0x00ff);
	 GetInfo_ESAM[6]=(INT8U)(datalen&0x00ff);
	 len+=2;
	 memcpy(&GetInfo_ESAM[7],&securityInfo.encrypted_code2[1],securityInfo.encrypted_code2[0]);
	 len+=securityInfo.encrypted_code2[0];
	 memcpy(&GetInfo_ESAM[securityInfo.signature[0]+7],&securityInfo.signature[1],securityInfo.signature[0]);
	 len+=securityInfo.signature[0];
	 GetInfo_ESAM[len]=LRC(&GetInfo_ESAM[1],len-1);

	 Result = Esam_WriteThenRead(fd, (INT8U*)GetInfo_ESAM, len, tmp);

	 if(Result>0 && Result<BUFFLENMAX_SPI) //大于BUFFLENMAX_SPI错误，此处做比较
	{
		 memcpy(RetInfo->server_rn,&tmp[4],48);//48byte服务器随机数
		 memcpy(RetInfo->server_signInfo,&tmp[52],Result-53);  //53=4+1+48
		return Result-5;
	}
   return Result;
}
/**********************************
 *5.3  安全传输数据处理（主站到终端）第四步中，将主站下发信息解密
 *SID或SID+MAC，发送：4字节安全标识+附加数据(AttachData)+Data(+MAC)
 *输入：Data前两个字节为数据长度（必须为2个字节！！不够补零）(附加数据其实就是信息长度)
 *返回：Rbuf(Data)已去掉帧头帧尾
 *函数返回：1、为正数是为终端信息数据长度		  2、负数：代表相应错误，见：Esam.h中，ESAM ERR ARRAY定义
 *************************************************************/
INT32S Esam_SIDTerminalCheck(INT32S fd, SID_MAC SidMac,INT8U* Data, INT8U* Rbuf) {
	if(sizeof(SidMac.sid.addition)<=SidMac.sid.addition[0])	return ERR_ESAM_TRANSPARA_ERR;
	INT32S Result=0;
	INT16U len=0;
	INT8U tmp[BUFFLENMAX_SPI];
	 memset(tmp,0,BUFFLENMAX_SPI);
	 INT8U GetInfo_ESAM[BUFFLENMAX_SPI];
	 GetInfo_ESAM[0]=0x55;
	 len+=1;
	 memcpy(&GetInfo_ESAM[len],SidMac.sid.sig,4);//4字节安全标示
	 len+=4;
	 memcpy(&GetInfo_ESAM[len],&SidMac.sid.addition[1],SidMac.sid.addition[0]);//附加数据
	 len+=SidMac.sid.addition[0];
	 INT16U datalen=(0xff &Data[1])|(0xff00 & (Data[0]<<8));
	 memcpy(&GetInfo_ESAM[len],&Data[2],datalen);//密文应用数据单元
	 len+=datalen;
	 if(SidMac.mac[0]!=0x00)
	 {
		 if(sizeof(SidMac.mac)<=SidMac.mac[0])  return ERR_ESAM_TRANSPARA_ERR;
		 memcpy(&GetInfo_ESAM[len],&SidMac.mac[1],SidMac.mac[0]);//如果MAC有数据，拷贝
		 len+=SidMac.mac[0];
	 }
	 GetInfo_ESAM[len]=LRC(&GetInfo_ESAM[1],len-1);//获取LRC校验值
	 len+=1;
	 Result = Esam_WriteThenRead(fd, (INT8U*)GetInfo_ESAM, len, tmp);

	 if(Result>0 && Result<BUFFLENMAX_SPI) //大于BUFFLENMAX_SPI错误，此处做比较
	{
		 memcpy(Rbuf,&tmp[4],Result-5);
		return Result-5;
	}
	return Result;
}
/**********************************
 *5.3 安全传输数据处理（主站到终端）第七步，将终端上送报文加密
 *发送：801C00+P2+Lc+Data3
 *返回：9000+LEN+ Data4
 *P2:明文+MAC 方式：11
 *密文：96
 *密文+MAC：97
 *输入：P2(0x11,0x96,0x97),Data3数据返回帧的明文Data3[0-1]为字符串长度，后跟具体字符
 *输出：Esam返回DATA4需要根据P2type，查看芯片具体回复啥数据，再组上行报文
 *函数返回：1、为正数是为终端信息数据长度		  2、负数：代表相应错误，见：Esam.h中，ESAM ERR ARRAY定义
 *************************************************************/
INT32S Esam_SIDResponseCheck(INT32S fd, INT8U P2type, INT8U* Data3, INT8U* Rbuf) {
	 INT16U datalen=(0xff &Data3[1])|(0xff00 & (Data3[0]<<8));
	if(datalen==0) return ERR_ESAM_TRANSPARA_ERR;
	if(P2type!=0x11 || P2type!=0x96 || P2type!=0x97) return ERR_ESAM_TRANSPARA_ERR;
	INT32S Result=0;
	INT8U tmp[BUFFLENMAX_SPI];
	memset(tmp,0,BUFFLENMAX_SPI);
	 INT8U GetInfo_ESAM[BUFFLENMAX_SPI]={0x55,0x80,0x1C,0x00,P2type};//P2加入
	 GetInfo_ESAM[5]=Data3[0];// 长度
	 GetInfo_ESAM[6]=Data3[1];
	 memcpy(&GetInfo_ESAM[7],&Data3[2],datalen);
	 GetInfo_ESAM[7+datalen]=LRC(&GetInfo_ESAM[1],datalen+6);

	 Result = Esam_WriteThenRead(fd, (INT8U*)GetInfo_ESAM,datalen+8, tmp);
	 if(Result>0 && Result<BUFFLENMAX_SPI) //大于BUFFLENMAX_SPI错误，此处做比较
	{
		 memcpy(Rbuf,&tmp[4],Result-5);
		return Result-5;
	}
	return Result;
}
/**********************************
 *5.4.1安全传输数据处理  读取（抄读终端）应用场景：主站下发明文+RN，响应时需要MAC由此处获取
 *发送：800E4002+LC+Data1
 *返回：9000+0004+MAC
 *输入Data1(RN+PlainData)PlainData为响应的PADU(头2byte为长度)，RN为主站请求帧中附带的随机数(设定小与255)
 *函数返回：1、为正数是为终端信息数据长度		  2、负数：代表相应错误，见：Esam.h中，ESAM ERR ARRAY定义
 *************************************************************/
INT32S Esam_GetTerminalInfo(INT32S fd, INT8U *RN,INT8U* Data1,INT8U* Rbuf) {
	 INT16U datalen=(0xff &Data1[1])|(0xff00 & (Data1[0]<<8));
	 if(datalen<=0 || RN[0]<=0) return ERR_ESAM_TRANSPARA_ERR;
	INT32S Result=0;
	INT16U len=0;
	INT8U tmp[BUFFLENMAX_SPI];
	memset(tmp,0,BUFFLENMAX_SPI);
	INT8U GetInfo_ESAM[BUFFLENMAX_SPI]={0x55,0x80,0x0E,0x40,0x02};
	len+=5;
	 GetInfo_ESAM[5]=(INT8U)(((datalen+RN[0])>>8)&0x00ff);
	 GetInfo_ESAM[6]=(INT8U)((datalen+RN[0])&0x00ff);
	len+=2;
	memcpy(&GetInfo_ESAM[7],&RN[1],RN[0]);
	len+=RN[0];
	memcpy(&GetInfo_ESAM,&Data1[2],datalen);
	len+=datalen;
	GetInfo_ESAM[len] = LRC(&GetInfo_ESAM[1],len-1);

	Result = Esam_WriteThenRead(fd, (INT8U*)GetInfo_ESAM, len, tmp);
	if(Result>0 && Result<BUFFLENMAX_SPI) //大于BUFFLENMAX_SPI错误，此处做比较
	{
		 memcpy(Rbuf,&tmp[4],Result-5);
		return Result-5;
	}
	return Result;
}
/**********************************
 *安全传输数据处理（终端对称密钥更新）对称秘钥版本低时，更新秘钥
 *根据5.3中第4步获取Data2
 *终端判断安全标识为812E0000
 *发送：安全标识+附加数据 AttachData+Endata1+MAC
 *返回：9000+0000
 *输入5.3中第4步获取Data2，解析获取发送所需数据
 *函数返回：1、为正数是为终端信息数据长度	  2、负数：代表相应错误，见：Esam.h中，ESAM ERR ARRAY定义
 *************************************************************/
INT32S Esam_SymKeyUpdate(INT32S fd, INT8U* Data2, INT8U* Rbuf) {
//TODO:Data2中包含的数据具体啥样，等链接加密机后在更改
	if(Data2[0]==0) return ERR_ESAM_TRANSPARA_ERR;
	//INT32S Result=0;
	INT8U GetInfo_ESAM[BUFFLENMAX_SPI]={0x55};
	memcpy(&GetInfo_ESAM[1],&Data2[1],Data2[0]);
	GetInfo_ESAM[1+Data2[0]]=LRC(&GetInfo_ESAM[1],Data2[0]);

	return Esam_WriteThenRead(fd, (INT8U*)GetInfo_ESAM,Data2[0]+2,Rbuf);
}
/**********************************
 *安全传输数据处理（证书更新）与终端对称密钥更新类似
 *终端判断安全标识为81300203
 *发送：安全标识+附加数
 *据 AttachData+Endata1
 *返回：9000+0000
 *函数返回：1、为正数是为终端信息数据长度   负数：代表相应错误，见：Esam.h中，ESAM ERR ARRAY定义
 *************************************************************/
INT32S Esam_CcieUpdate(INT32S fd, INT8U* Data2, INT8U* Rbuf) {
	//TODO:Data2中包含的数据具体啥样，等链接加密机后在更改
	if(Data2[0]==0) return ERR_ESAM_TRANSPARA_ERR;
	//INT32S Result=0;
	INT8U GetInfo_ESAM[BUFFLENMAX_SPI]={0x55};
	memcpy(&GetInfo_ESAM[1],&Data2[1],Data2[0]);
	GetInfo_ESAM[1+Data2[0]]=LRC(&GetInfo_ESAM[1],Data2[0]);

	return Esam_WriteThenRead(fd, (INT8U*)GetInfo_ESAM,Data2[0]+2,Rbuf);
}
/**********************************
 *安全传输数据处理（更新会话时效门限）
 *终端判断安全标识为81340105
 *发送：安全标识+附加数
 *据 AttachData++Data1
 *返回：9000+0000
 *函数返回：1、为正数是为终端信息数据长度  负数：代表相应错误，见：Esam.h中，ESAM ERR ARRAY定义
 *************************************************************/
INT32S Esam_SessionTime(INT32S fd, INT8U* Data2, INT8U* Rbuf) {
	//TODO:Data2中包含的数据具体啥样，等链接加密机后在更改
	if(Data2[0]==0) return ERR_ESAM_TRANSPARA_ERR;
	//INT32S Result=0;
	INT8U GetInfo_ESAM[BUFFLENMAX_SPI]={0x55};
	memcpy(&GetInfo_ESAM[1],&Data2[1],Data2[0]);
	GetInfo_ESAM[1+Data2[0]]=LRC(&GetInfo_ESAM[1],Data2[0]);

	return Esam_WriteThenRead(fd, (INT8U*)GetInfo_ESAM,Data2[0]+2,Rbuf);
}
/**********************************
 *安全传输数据处理（终端主动上报）
 *发送：80140103+LC+Data1
 *返回：9000+LEN+Data2+4字节MAC1
 *LC: Data1 长度，2 字节
 *Data1：明文数据
 *Data2：12 字节随机数
 *RN_MAC：包含 12 字节随机数Data2 和 4 字节 MAC
 *函数输入：Data1明文
 *函数输出：1、为正数是为终端信息数据长度  负数：代表相应错误，见：Esam.h中，ESAM ERR ARRAY定义
 *函数说明：加密主动上报和读取/设置一样，在读取设置报文之外加壳（报文+安全标识）
 *************************************************************/
INT32S Esam_ReportEncrypt(INT32S fd, INT8U* Data1, INT8U* RN,INT8U* MAC) {
	 INT16U datalen=(0xff &Data1[1])|(0xff00 & (Data1[0]<<8));
	 if(datalen<=0) return ERR_ESAM_TRANSPARA_ERR;
	 INT8U tmp[BUFFLENMAX_SPI];
	memset(tmp,0,BUFFLENMAX_SPI);
	INT32S Result=0;
	INT8U GetInfo_ESAM[BUFFLENMAX_SPI]={0x55,0x80,0x14,0x01,0x03};
	GetInfo_ESAM[5]=Data1[0];
	GetInfo_ESAM[6]=Data1[1];
	memcpy(&GetInfo_ESAM[1],&Data1[2],datalen);
	GetInfo_ESAM[7+datalen]=LRC(&GetInfo_ESAM[1],datalen+6);

	Result = Esam_WriteThenRead(fd, (INT8U*)GetInfo_ESAM, datalen+8, tmp);
	if(Result>0 && Result<BUFFLENMAX_SPI) //大于BUFFLENMAX_SPI错误，此处做比较
	{
		 memcpy(RN,&tmp[4],12);
		 memcpy(MAC,&tmp[16],4);//TODO:此处需要查看MAC长度和整个报文长度，芯片手册此处不详细
		return Result;
	}
	return Result;
}
/**********************************
 *安全传输数据处理（主动上报下行报文解密）
 *发送：安全标识+附加数据 AttachData+ Data3+MAC2
 *返回：9000+Len+Data4
 *输入：猜想：SID:安全标识加附加数据，
 *函数输出：1、为正数是为终端信息数据长度  负数：代表相应错误，见：Esam.h中，ESAM ERR ARRAY定义
 *函数说明：此处宣贯资料和芯片手册存在差别，需要拿正式报文做比较。宣贯资料：将终端上报随机数/响应帧中明文/MAC下发安全芯片
 *芯片手册：安全标识+附加数据 AttachData+ Data3+MAC2，暂时用安全芯片为标准
 *************************************************************/
INT32S Esam_DencryptReport(INT32S fd, SID_MAC SidMac,INT8U* Data3, INT8U* Rbuf) {
	//Data3前2字节为长度
	if(sizeof(SidMac.sid.addition)<=SidMac.sid.addition[0])	return ERR_ESAM_TRANSPARA_ERR;
	 if(sizeof(SidMac.mac)<=SidMac.mac[0])  return ERR_ESAM_TRANSPARA_ERR;
	 INT16U datalen=(0xff &Data3[1])|(0xff00 & (Data3[0]<<8));
	 if(datalen<=0) return ERR_ESAM_TRANSPARA_ERR;
		INT32S Result=0;
		INT16U len=0;
		INT8U tmp[BUFFLENMAX_SPI];
		memset(tmp,0,BUFFLENMAX_SPI);
		 INT8U GetInfo_ESAM[BUFFLENMAX_SPI];
		 GetInfo_ESAM[0]=0x55;
		 len+=1;
		 memcpy(&GetInfo_ESAM[len],SidMac.sid.sig,4);//4字节安全标示
		 len+=4;
		 memcpy(&GetInfo_ESAM[len],&SidMac.sid.addition[1],SidMac.sid.addition[0]);//附加数据
		 len+=SidMac.sid.addition[0];
		 memcpy(&GetInfo_ESAM[len],&Data3[2],datalen);//密文应用数据单元
		 len+=datalen;
		 memcpy(&GetInfo_ESAM[len],&SidMac.mac[1],SidMac.mac[0]);//如果MAC有数据，拷贝
		 len+=SidMac.mac[0];
		 GetInfo_ESAM[len]=LRC(&GetInfo_ESAM[1],len-1);//获取LRC校验值
		 len+=1;

		 Result = Esam_WriteThenRead(fd, (INT8U*)GetInfo_ESAM, len, tmp);
		 if(Result>0 && Result<BUFFLENMAX_SPI) //大于BUFFLENMAX_SPI错误，此处做比较
		{
			 memcpy(Rbuf,&tmp[4],Result-5);
			return Result-5;
		}
		return Result;
}
/**********************************
 *终端抄读电表获取随机数
 *发送：800400100000+LRC
 *返回：9000+LEN+Rand
 *************************************************************/
INT32S Esam_GetRN(INT32S fd,  INT8U* Rbuf)  {
	INT32S Result=0;
	INT8U tmp[BUFFLENMAX_SPI];
	memset(tmp,0,BUFFLENMAX_SPI);
	INT8U GetInfo_ESAM[8]={0x55,0x80,0x04,0x00,0x10,0x00,0x00,0x00};
	GetInfo_ESAM[7]=LRC(&GetInfo_ESAM[1],6);
	Result = Esam_WriteThenRead(fd, (INT8U*)GetInfo_ESAM, 8, tmp);
	if(Result>0 && Result<BUFFLENMAX_SPI) //大于BUFFLENMAX_SPI错误，此处做比较
	{
		memcpy(Rbuf,&tmp[4],Result-5);
		return Result-5;
	}
	return Result;
}
/**********************************
 *电表上报数据解密明文+随机数+MAC
 *发送：800E4887+LC+电表表号+RN+Data2+MAC
 *返回：9000+0000
 *电表上报数据解密密文+随机数
 *发送：800C4807+LC+电表表号+ RN +Data2
 *返回：9000+Len+Data3
 **电表上报数据解密密文+随机数+MAC
 *发送：80124807+LC+电表表号+RN+Data2+MAC
 *返回：9000+Len+Data3
 *************************************************************/
INT32S Esam_EmeterDataDencrypt(INT32S fd, INT8U* MeterNo, INT8U* Rbuf) {
	INT8U GetInfo_ESAM[BUFFLENMAX_SPI];

}

