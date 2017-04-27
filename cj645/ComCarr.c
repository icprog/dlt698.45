/*
 * ComCarr.c
 *
 *  Created on: 2014-4-3
 *      Author: Administrator
 */


/*
 * 485口接收处理,判断完整帧
 * 输出；*str：接收缓冲区
 * 返回：>0：完整报文；=0:接收长度为0；-1：乱码，无完整报文
 */
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "Shmem.h"
#include "StdDataType.h"
//INT16S ReceDataFrom485(INT32S fd, INT8U *str)
//{
//	INT8U 	TmprevBuf[256];//接收报文临时缓冲区
//	INT16U  len,rec_step,rec_head,rec_tail,DataLen,i,j;
//	if(fd<=2)
//		return -1;
//	memset(TmprevBuf,0,256);
//	rec_head=rec_tail= rec_step = DataLen =0;
//
//	for(j=0;j<5;j++)
//	{
//		usleep(100000);	//100ms
//		len = read(fd,TmprevBuf,256);
//		if(len>0){
//			fprintf(stderr, "len=%d, 11111111\n", len);
//			for(i=0;i<len;i++)
//			{
//				str[rec_head++]=TmprevBuf[i];
//			}
//
////			int i;
////			fprintf(stderr, "v645 RECV: ");
////			for(i=0; i<len; i++)
////			{
////				fprintf(stderr, "%02x ", TmprevBuf[i]);
////			}
////			fprintf(stderr, "\n");
//
////			INT8U str[50];
////			memset(str, 0, 50);
////			sprintf((char *)str, "485(%d)_R(%d):", S4852, len);
////			dbg_prtbuff((char *)str, TmprevBuf, len, "%02x", " ", "\n");
//		}
//		switch (rec_step) {
//		case 0:
//			if (rec_tail < rec_head)
//			{
//				for(i=rec_tail; i<rec_head; i++)
//				{
//					if(str[i] == 0x68)
//					{//ma:判断第一个字符是否为0x68
//						rec_step = 1;
//						rec_tail = i;
//						break;
//					}else
//						rec_tail++;
//				}
//			}
//			break;
//		case 1:
//			if((rec_head - rec_tail)>=9)
//			{
//				if(str[rec_tail]==0x68 && str[rec_tail+7]==0x68 )
//				{
//					DataLen=str[rec_tail+9];//获取报文数据块长度
//					rec_step = 2;
//					break;
//				}else
//					rec_tail++;
//			}
//			break;
//		case 2:
//			if((rec_head - rec_tail)>=(DataLen+2))
//			{
//				if (str[rec_tail+9 +DataLen+2] == 0x16) {
//					fprintf(stderr, "rec_head=%d, 222222\n", rec_head);
//					return rec_head;
//				}
//			}
//			break;
//		default:
//			break;
//		}
//	}
//	if (len>0)
//		return -1;
//	else
//		return 0;
//}



INT16S ReceDataFrom485(INT32S fd, INT8U *str)
{
	INT8U 	TmprevBuf[256];//接收报文临时缓冲区
	INT16U  len,rec_step,rec_head,rec_tail,DataLen,i,j;
	if(fd<=2)
		return -1;
	memset(TmprevBuf,0,256);
	rec_head=rec_tail= rec_step = DataLen =0;

	len = read(fd,TmprevBuf,256);
	if (len>0)
	{
		for(j=0;j<15;j++)
		{
			usleep(2000);	//20ms
			fprintf(stderr, "j=%d: ", j);//test
			for(i=0;i<len;i++)
			{
				str[rec_head++]=TmprevBuf[i];
				fprintf(stderr, "%02x ", TmprevBuf[i]);//test
			}
			fprintf(stderr, "\n");//test

			switch (rec_step)
			{
			case 0:
				if (rec_tail < rec_head)
				{
					for(i=rec_tail; i<rec_head; i++)
					{
						if(str[i] == 0x68)
						{//ma:判断第一个字符是否为0x68
							rec_step = 1;
							rec_tail = i;
							break;
						}else
							rec_tail++;
					}
				}
				break;
			case 1:
				if((rec_head - rec_tail)>=9)
				{
					if(str[rec_tail]==0x68 && str[rec_tail+7]==0x68 )
					{
						DataLen=str[rec_tail+9];//获取报文数据块长度
						rec_step = 2;
						break;
					}else
						rec_tail++;
				}
				break;
			case 2:
				if((rec_head - rec_tail)>=(DataLen+2))
				{
					if (str[rec_tail+9 +DataLen+2] == 0x16) {
						return rec_head;
					}
				}
				break;
			default:
				break;
			}
			len = read(fd,TmprevBuf,256);
		}
	}
	return 0;
}

/**
 * 485口发送
 */
void SendDataTo485(INT32S fd,INT8U *sendbuf,INT16U sendlen)
{
	int i;
	fprintf(stderr, "v645 SEND: ");
	for(i=0; i<sendlen; i++)
	{
		fprintf(stderr, "%02x ", sendbuf[i]);
	}
	fprintf(stderr, "\n\n\n");

	ssize_t slen;
	slen = write(fd,sendbuf,sendlen);
	if(slen<0)	fprintf(stderr, "slen=%d,send err!\n",slen);
}
