/*
 * j3762.c
 *
 *  Created on: 2013-3-20
 *      Author: Administrator
 */
#include <string.h>
#include <stdio.h>
#include "lib3762.h"
#include "dealData.h"

INT8U getCS3762(INT8U* buf, const INT16U len)
{
	INT8U i, cs=0;

	for (i=0; i<len; i++)
	{
		cs = (cs + buf[i])%256;
	}
	return cs;
}

INT8S isValid3762(INT8U* recvBuf, const INT16U recvLen)
{
	INT16U len;
	len = (recvBuf[2]<<8) + recvBuf[1];

	if ((recvBuf[0]==0x68) && (len==recvLen) && (recvBuf[recvLen-1]==0x16) &&
			recvBuf[recvLen-2]==getCS3762(&recvBuf[3], recvLen-5))
	{
		return 0;
	}
	return -1;
}

INT8U getFN(INT8U dt1, INT8U dt2)
{
	INT8U i,fn;
	for(i=0; i<8; i++)
	{
		if (((dt1>>i) & 0x01) == 1)
		{
			break;
		}
	}
	fn = dt2*8+i+1;
	return fn;
}

void setFN(INT8U* dt, INT8U fn)
{
	dt[0] = 1<<((fn-1)%8);
	dt[1] = (fn-1)/8;
}
INT8S AFN03_F4(FORMAT3762 *down,INT8U *sendBuf)
{//查询主节点地址 afn=03, f4
	INT8U sendLen;

	memset(&sendBuf[0], 0, 256);
	down->afn = 0x03;
	down->fn = 4;
	down->ctrl.PRM = 1;//启动站
	down->info_down.ChannelFlag = 0;//信道标识
	down->info_down.ModuleFlag = 0;//无地址域A
	down->info_down.Seq = down->info_down.Seq++;//序列号
	down->ctrl.ComType = 1;//窄带载波通信
	sendLen = composeProtocol3762(down, sendBuf);
	return sendLen;
}


int AFN11_F5(FORMAT3762 *down,INT8U *sendBuf,INT8U minute)
{//激活从节点主动注册

	INT8U sendLen;
	memset(&sendBuf[0], 0, 256);

	down->afn = 0x11;
	down->fn = 5;
	down->ctrl.PRM = 1;//启动站
	down->info_down.ChannelFlag = 0;//信道标识
	down->info_down.ModuleFlag = 0;//无地址域A
	down->info_down.Seq = down->info_down.Seq++;//序列号
	down->afn11_f5_down.Duration = minute;//持续时间
	down->ctrl.ComType = 1;//窄带载波通信
	sendLen = composeProtocol3762(down, sendBuf);

	return sendLen ;
}
int AFN03_F9(FORMAT3762 *down,INT8U *sendBuf,INT8U protocol,INT8U msgLen,INT8U *msgContent)
{
	//查询通信延时相关广播通信时长
	INT8U sendLen = 0;
	memset(&sendBuf[0], 0, 256);

	down->afn = 0x03;
	down->fn = 9;
	down->ctrl.PRM = 1;//启动站
	down->info_down.ChannelFlag = 0;//信道标识
	down->info_down.ModuleFlag = 0;//无地址域A
	down->info_down.Seq = down->info_down.Seq++;//序列号
	down->afn03_f9_down.Protocol = protocol;
	down->afn03_f9_down.MsgLength = msgLen;
	memcpy(down->afn03_f9_down.MsgContent, msgContent, msgLen);
	sendLen = composeProtocol3762(down, sendBuf);

	return sendLen;
}
int AFN12_F2(FORMAT3762 *down,INT8U *sendBuf)
{
	//暂停
	INT8U sendLen;
	memset(&sendBuf[0], 0, 256);

	down->afn = 0x12;
	down->fn = 2;
	down->ctrl.PRM = 1;//启动站
	down->info_down.ChannelFlag = 0;//信道标识
	down->info_down.ModuleFlag = 0;//无地址域A
	down->info_down.Seq = down->info_down.Seq++;//序列号
	down->ctrl.ComType = 1;//窄带载波通信
	sendLen = composeProtocol3762(down, sendBuf);
	return sendLen ;
}

int AFN12_F3(FORMAT3762 *down,INT8U *sendBuf)
{//恢复抄表
	INT8U sendLen;
	memset(&sendBuf[0], 0, 256);

	down->afn = 0x12;
	down->fn = 3;
	down->ctrl.PRM = 1;//启动站
	down->info_down.ChannelFlag = 0;//信道标识
	down->info_down.ModuleFlag = 0;//无地址域A
	down->info_down.Seq = down->info_down.Seq++;//序列号
	down->ctrl.ComType = 1;//窄带载波通信
	sendLen = composeProtocol3762(down, sendBuf);
	return sendLen ;
}

int AFN13_F1(FORMAT3762 *down,INT8U *sendBuf3762,INT8U* destAddr, INT8U protocol, INT8U delayFlag, INT8U* sendBuf645, INT8U sendLen645)
{//监控从节点
	INT8U sendLen3762;
	memset(&sendBuf3762[0], 0, 256);

	//组3762报文
	down->afn = 0x13;
	down->fn = 1;
	down->ctrl.PRM = 1;//启动站
	down->info_down.ChannelFlag = 0;//信道标识
	down->info_down.ModuleFlag = 1;//有地址域A
	down->info_down.Seq = down->info_down.Seq++;//序列号
	down->ctrl.ComType = 1;//窄带载波通信
//	getMasterPointAddr(down->addr.SourceAddr);
	memcpy(down->addr.DestAddr, destAddr, 6);//目的地址
	down->afn13_f1_down.Protocol = 2;
	down->afn13_f1_down.DelayFlag = delayFlag;
	down->afn13_f1_down.SubPointNum = 0;
	down->afn13_f1_down.MsgLength = sendLen645;
	memcpy(down->afn13_f1_down.MsgContent, sendBuf645, sendLen645);
	sendLen3762 = composeProtocol3762(down, sendBuf3762);
	return sendLen3762 ;
}

int AFN14_F1(FORMAT3762 *down,FORMAT3762 *up,INT8U *sendBuf,INT8U* destAddr, INT8U readFlag, INT8U delayTime, INT8U msgLen, INT8U *msgContent)
{
	INT8U sendLen;
	memset(&sendBuf[0], 0, 256);

	down->afn = 0x14;
	down->fn = 1;
	down->ctrl.PRM = 0;//从动站
	down->info_down.ChannelFlag = up->info_up.ChannelFlag;//信道标识
	down->info_down.ModuleFlag = 1;//有地址域A
	down->info_down.Seq = up->info_up.Seq;//format3762_Down->info_down.Seq++;//序列号
	down->ctrl.ComType = 1;//窄带载波通信
//	down->addr.SourceAddr  在组帧前填充
	memcpy(down->addr.DestAddr, destAddr, 6);//目的地址

	down->afn14_f1_down.ReadFlag = readFlag;
	down->afn14_f1_down.DelayFlag = delayTime;
	down->afn14_f1_down.MsgLength = msgLen;
	memcpy(down->afn14_f1_down.MsgContent, msgContent, msgLen);
	down->afn14_f1_down.SubPointNum = 0;

	sendLen = composeProtocol3762(down, sendBuf);
	return sendLen;
}

int AFN12_F1(FORMAT3762 *down,INT8U *sendBuf)
{
	int sendlen=0;
	memset(&sendBuf[0], 0, 256);

	down->afn = 0x12;
	down->fn = 1;
	down->ctrl.PRM = 1;//启动站
	down->info_down.ChannelFlag = 0;//信道标识
	down->info_down.ModuleFlag = 0;//无地址域A
	down->info_down.Seq = down->info_down.Seq++;//序列号
	down->ctrl.ComType = 1;//窄带载波通信
	sendlen = composeProtocol3762(down, sendBuf);
	return sendlen ;
}

int AFN11_F1(FORMAT3762 *down,INT8U *sendBuf,INT8U *SlavePointAddr)
{
	int sendlen=0;
	memset(&sendBuf[0], 0, 256);

	down->afn = 0x11;
	down->fn = 1;
	down->ctrl.PRM = 1;//启动站
	down->info_down.ChannelFlag = 0;//信道标识
	down->info_down.ModuleFlag = 0;//无地址域A
	down->info_down.Seq = down->info_down.Seq++;//序列号
	down->afn11_f1_down.Num = 1;
	memcpy(&down->afn11_f1_down.SlavePoint[0].Addr[0], SlavePointAddr, 6);
	down->afn11_f1_down.SlavePoint[0].Protocol = 2;//07表
	down->ctrl.ComType = 1;//窄带载波通信
	sendlen = composeProtocol3762(down, sendBuf);
	return sendlen;
}

int AFN11_F2(FORMAT3762 *down,INT8U *sendBuf,INT8U *SlavePointAddr)
{
	int sendlen=0;
	memset(&sendBuf[0], 0, 256);

	down->afn = 0x11;
	down->fn = 2;
	down->ctrl.PRM = 1;//启动站
	down->info_down.ChannelFlag = 0;//信道标识
	down->info_down.ModuleFlag = 0;//无地址域A
	down->info_down.Seq = down->info_down.Seq++;//序列号
	down->ctrl.ComType = 1;//窄带载波通信
	down->afn11_f2_down.Num = 1;
	memcpy(&down->afn11_f2_down.SlavePointAddr[0][0], &SlavePointAddr[0], 6);

	sendlen = composeProtocol3762(down, sendBuf);
	return sendlen;
}
int AFN10_F1(FORMAT3762 *down,INT8U *sendBuf)
{
	int sendlen=0;
	memset(&sendBuf[0], 0, 256);

	down->afn = 0x10;
	down->fn = 1;
	down->ctrl.PRM = 1;//启动站
	down->info_down.ChannelFlag = 0;//信道标识
	down->info_down.ModuleFlag = 0;//无地址域A
	down->info_down.Seq = down->info_down.Seq++;//序列号
	down->ctrl.ComType = 1;//窄带载波通信
	sendlen = composeProtocol3762(down, sendBuf);
	return sendlen;
}
int AFN10_F2(FORMAT3762 *down,INT8U *sendBuf,INT16U index, INT8U num)
{
	int sendlen=0;
	memset(&sendBuf[0], 0, 256);

	down->afn = 0x10;
	down->fn = 2;
	down->ctrl.PRM = 1;//启动站
	down->info_down.ChannelFlag = 0;//信道标识
	down->info_down.ModuleFlag = 0;//无地址域A
	down->info_down.Seq = down->info_down.Seq++;//序列号
	down->ctrl.ComType = 1;//窄带载波通信
	down->afn10_f2_down.Index = index;
	down->afn10_f2_down.Num = num;

	sendlen = composeProtocol3762(down, sendBuf);
	return sendlen;
}

int AFN05_F1(FORMAT3762 *down,INT8U *sendBuf,INT8U *addr)
{
	int sendlen=0;
	memset(&sendBuf[0], 0, 256);

	down->afn = 0x05;
	down->fn = 1;
	down->ctrl.PRM = 1;//启动站
	down->ctrl.ComType = 1;//窄带载波通信
	down->info_down.ChannelFlag = 0;//信道标识
	down->info_down.ModuleFlag = 0;//无地址域A
	down->info_down.Seq = down->info_down.Seq++;//序列号
	memcpy(down->afn05_f1_down.MasterPointAddr,addr,6);
	sendlen = composeProtocol3762(down, sendBuf);
	return sendlen ;
}
int AFN05_F3(FORMAT3762 *down,INT8U moduleFlag, INT8U ctrl, INT8U* sendBuf645, INT8U sendLen645,INT8U *sendBuf)
{//启动广播
	int sendLen;
	memset(&sendBuf[0], 0, 256);

	down->afn = 0x05;
	down->fn = 3;
	down->ctrl.PRM = 1;//启动站
	down->ctrl.ComType = 1;//窄带载波通信
	down->info_down.ChannelFlag = 0;//信道标识
	down->info_down.ModuleFlag = moduleFlag;//地址域A
	down->info_down.Seq = down->info_down.Seq++;//序列号

	if (moduleFlag == 1)//有地址域
	{
		memset(down->addr.DestAddr, 0x99, 6);//目的地址
	}
	down->afn05_f3_down.ctrl = ctrl;
	down->afn05_f3_down.MsgLength = sendLen645;
	memcpy(&down->afn05_f3_down.MsgContent, sendBuf645, sendLen645);

	sendLen = composeProtocol3762(down, sendBuf);
	return sendLen;
}

INT8S AFN01_F2(FORMAT3762 *down,INT8U *sendBuf)
{//参数区初始化
	INT8U sendLen ;

	memset(&sendBuf[0], 0, 256);

	down->afn = 0x01;
	down->fn = 2;
	down->ctrl.PRM = 1;//启动站
	down->info_down.ChannelFlag = 0;//信道标识
	down->info_down.ModuleFlag = 0;//无地址域A
	down->info_down.Seq = down->info_down.Seq++;//序列号

	sendLen = composeProtocol3762(down, sendBuf);
	return sendLen;
}

int AFN03_F10(FORMAT3762 *down,INT8U *sendBuf)
{
	int sendlen=0;
	memset(&sendBuf[0], 0, 256);

	down->afn = 0x03;
	down->fn = 10;
	down->ctrl.PRM = 1;//启动站
	down->info_down.ChannelFlag = 0;//信道标识
	down->info_down.ModuleFlag = 0;//无地址域A
	down->info_down.Seq = down->info_down.Seq++;//序列号
	down->ctrl.ComType = 1;//窄带载波通信
	sendlen= composeProtocol3762(down, sendBuf);
	return sendlen;
}
int AFN10_F4(FORMAT3762 *down,INT8U *sendBuf)
{
	int sendlen = 0;
	memset(&sendBuf[0], 0, 256);
	down->afn = 0x10;
	down->fn = 4;
	down->ctrl.PRM = 1;//启动站
	down->info_down.ChannelFlag = 0;//信道标识
	down->info_down.ModuleFlag = 0;//无地址域A
	down->info_down.Seq = down->info_down.Seq++;//序列号
	sendlen = composeProtocol3762(down, sendBuf);
	return sendlen;
}
int AFN00_F01(FORMAT3762 *up,INT8U *sendBuf)
{
	FORMAT3762 down;
	int sendlen=0;
	memset(&sendBuf[0], 0, 256);

	down.afn = 0x00;
	down.fn = 1;
	down.ctrl.PRM = 0;//从动站
	down.info_down.ChannelFlag = up->info_up.ChannelFlag;//信道标识
	down.info_down.ModuleFlag = 0;//无地址域A
	down.info_down.Seq = up->info_up.Seq; //up->info_down.Seq;//序列号
	memset(&down.afn00_f1.CommandStatus, 0, 16);//从动站
	down.ctrl.ComType = 1;//窄带载波通信
	sendlen = composeProtocol3762(&down, sendBuf);
	return sendlen;
}

//解析报文入口函数
INT8S analyzeProtocol3762(FORMAT3762* format3762, INT8U* recvBuf, const INT16U recvLen)
{
	INT8U index=10;
	INT16S dataLen;

	if (isValid3762(&recvBuf[0], recvLen) == 0)	//校验通过
	{
		format3762->length = (recvBuf[2]<<8) + recvBuf[1];
		format3762->ctrl.ComType = recvBuf[3] & 0x3f;
		format3762->ctrl.PRM = (recvBuf[3]>>6) & 0x01;
		format3762->ctrl.DIR = (recvBuf[3]>>7) & 0x01;

		format3762->info_up.RouterFlag = recvBuf[4] & 0x01;
		format3762->info_up.ModuleFlag = (recvBuf[4]>>2) & 0x01;
		format3762->info_up.RepeaterLevel = (recvBuf[4]>>4) & 0x0f;
		format3762->info_up.ChannelFlag = recvBuf[5] & 0x0f;
		format3762->info_up.PhaseFlag = recvBuf[6] & 0x0f;
		format3762->info_up.ChannelFeature = (recvBuf[6]>>4) & 0x0f;
		format3762->info_up.CommandQuality = recvBuf[7] & 0x0f;
		format3762->info_up.ReplyQuality = (recvBuf[7]>>4) & 0x0f;
		format3762->info_up.EventFlag = recvBuf[8] & 0x01;
		format3762->info_up.Seq = recvBuf[9];

		if (format3762->info_up.ModuleFlag == 1)	//有地址域A
		{
			memcpy(&format3762->addr.SourceAddr[0], &recvBuf[index], 6);
			index += 6;

//			for (i=0; i<format3762->info_up.RepeaterLevel; i++)
//			{
//				memcpy(&format3762->addr.RepeaterAddr[i][0], &recvBuf[index], 6);
//				index += 6;
//			}

			memcpy(&format3762->addr.DestAddr[0], &recvBuf[index], 6);
			index += 6;
		}

		format3762->afn = recvBuf[index++];
		format3762->dt1 = recvBuf[index++];
		format3762->dt2 = recvBuf[index++];
		format3762->fn = getFN(format3762->dt1, format3762->dt2);

		dataLen = analyzeData(format3762, format3762->ctrl.DIR, format3762->afn, format3762->fn, &recvBuf[index]);

		if (dataLen==-1)
		{
			fprintf(stderr, "非法376.2报文，不识别功能项！");
			return -1;
		}
		else if (dataLen==-2)//内部调试
		{
			fprintf(stderr, "内部调试，由各厂家定义，无需解析！");
		}
		else
		{
			if ((index+dataLen+2) != recvLen)
			{
				fprintf(stderr, "Length Error! analyzeLen:%d != recvLen:%d\n", index+dataLen+2, recvLen);
				return -1;
			}
			else
			{
			//	fprintf(stderr, "analyzeProtocol Correct!\n");
			}
		}
	}
	else
	{
		fprintf(stderr, "非法376.2报文，校验不通过！\n");
		return -1;
	}
	return 0;
}

//组合报文入口函数
INT8S composeProtocol3762(FORMAT3762* format3762, INT8U* sendBuf)
{
	INT16U cs, dataLen, sendIndex = 0;
	sendBuf[sendIndex++] = 0x68;
	sendBuf[sendIndex++] = 0x00;//长度
	sendBuf[sendIndex++] = 0x00;//长度
	sendBuf[sendIndex++] = (format3762->ctrl.DIR<<7) + (format3762->ctrl.PRM<<6) + format3762->ctrl.ComType;//控制域

	sendBuf[sendIndex++] = (format3762->info_down.RepeaterLevel<<4) + (format3762->info_down.ClashCheck<<3) +
			(format3762->info_down.ModuleFlag<<2) + (format3762->info_down.SubPointFlag<<1) + format3762->info_down.RouterFlag;//信息域
	sendBuf[sendIndex++] = (format3762->info_down.ErrCode<<4) + format3762->info_down.ChannelFlag;
	sendBuf[sendIndex++] = format3762->info_down.ReplyBytes;//预计应答字节数
	sendBuf[sendIndex++] = format3762->info_down.Rate & 0xff;
	sendBuf[sendIndex++] = (format3762->info_down.RateUnit<<7) + ((format3762->info_down.Rate>>8) & 0x3f);
	sendBuf[sendIndex++] = format3762->info_down.Seq;//报文序列号

	//地址域
	if (format3762->info_down.ModuleFlag == 1)
	{
		memcpy(&sendBuf[sendIndex], &format3762->addr.SourceAddr[0], 6);
		sendIndex+=6;

		INT8U i;
		for(i=0; i<format3762->info_down.RepeaterLevel; i++)
		{
			memcpy(&sendBuf[sendIndex], &format3762->addr.RepeaterAddr[0], 6);
			sendIndex+=6;
		}

		memcpy(&sendBuf[sendIndex], &format3762->addr.DestAddr[0], 6);
		sendIndex+=6;
	}

	sendBuf[sendIndex++] = format3762->afn;
	setFN(&sendBuf[sendIndex], format3762->fn);//数据单元标识
	sendIndex += 2;

	dataLen = composeData(format3762, format3762->ctrl.DIR, format3762->afn, format3762->fn, &sendBuf[sendIndex]);
	sendIndex += dataLen;

	cs = getCS3762(&sendBuf[3], sendIndex-3);
	sendBuf[sendIndex++] = cs;
	sendBuf[sendIndex++] = 0x16;
	sendBuf[1] = sendIndex & 0xff;
	sendBuf[2] = (sendIndex>>8) & 0xff;

	return sendIndex;
}
