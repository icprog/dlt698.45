
#define LIB645_VER 	1
#include "dlt645.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../libBase/PublicFunction.h"

INT8U getCS645(INT8U* buf, const INT16U len)
{
	INT16U i, cs=0;

	for (i=0; i<len; i++)
	{
		cs = (cs + buf[i])%256;
	}
	return cs;
}

INT8S isValid645(INT8U* recvBuf, const INT16U recvLen)
{
	if ((recvBuf[0]==0x68) && (recvBuf[7]==0x68) && (recvBuf[recvLen-1]==0x16) &&
			recvBuf[recvLen-2]==getCS645(&recvBuf[0], recvLen-2))
	{
		return 0;
	}
	return -1;
//	return 0;
}

INT16U getFECount(INT8U* recvBuf, const INT16U recvLen)//得到待解析报文中前导符FE的个数
{
	INT16U i, count=0;
	for (i=0; i<recvLen; i++)
	{
		if (recvBuf[i] == 0x68)
		{
			count = i;
			break;
		}
	}
	return count;
}

INT16U getFFCount(INT8U* recvBuf, const INT16U recvLen)//得到待解析报文中后缀FF的个数（江苏II型新联的电表）
{
	INT16U i, count=0;
	for (i=recvLen-1; i>0; i--)
	{
		if (recvBuf[i]!=0x16)
		{
			count++;
		}
		else
		{
			break;
		}
	}
	return count;
}


//07报文组合入口函数

INT16S composeProtocol07(FORMAT07* format07, INT8U* sendBuf)
{
	INT16U i;
	INT8U len;
	INT16S sendlen = 0;
	INT8U addrBuff[6] = {0};
	INT8U tmpbuf[512] = {0};

	fprintf(stderr,"composeProtocol07 format07->Addr = %02x%02x%02x%02x%02x%02x",
			format07->Addr[0],format07->Addr[1],format07->Addr[2],format07->Addr[3],format07->Addr[4],format07->Addr[5]);
	reversebuff(format07->Addr,6,addrBuff);

	DEBUG_TIME_LINE("composeProtocol07 ctrl = %02X", format07->Ctrl);

	switch (format07->Ctrl) {
	case 0x11: //读数据
		sendBuf[0] = 0x68;
		memcpy(&sendBuf[1], addrBuff, 6); //地址
		sendBuf[7] = 0x68;
		sendBuf[8] = format07->Ctrl;
		sendBuf[9] = 0x04; //长度
		memcpy(&sendBuf[10], format07->DI, 4); //数据标识

		for (i = 10; i < 14; i++) {
			sendBuf[i] += 0x33;
		}

		sendBuf[14] = getCS645(&sendBuf[0], 14);
		sendBuf[15] = 0x16;

		sendlen = 16;
		break;
	case 0x12: //读后续数据
		sendBuf[0] = 0x68;
		memcpy(&sendBuf[1], addrBuff, 6); //地址
		sendBuf[7] = 0x68;
		sendBuf[8] = format07->Ctrl;
		sendBuf[9] = 0x05; //长度
		memcpy(&sendBuf[10], format07->DI, 4); //数据标识
		sendBuf[14] = format07->SEQ;

		for (i = 10; i < 15; i++) {
			sendBuf[i] += 0x33;
		}

		sendBuf[15] = getCS645(&sendBuf[0], 15);
		sendBuf[16] = 0x16;

		sendlen = 17;
		break;
	case 0x14: //  台体校表扩展
		len = format07->Length;
		sendBuf[0] = 0x68;
		memcpy(&sendBuf[1], format07->Addr, 6); //地址
		sendBuf[7] = 0x68;
		sendBuf[8] = format07->Ctrl;
		sendBuf[9] = len; //长度
		memcpy(&sendBuf[10], format07->DI, 4); //数据标识
		memcpy(&sendBuf[14], format07->Data, len - 4);
		for (i = 10; i < 10 + len; i++) {
			sendBuf[i] += 0x33;
		}
		sendBuf[10 + len] = getCS645(&sendBuf[0], 10 + len);
		sendBuf[11 + len] = 0x16;

		sendlen = (12 + len);
		break;
	case 0x13: //启动搜表（鼎信用）
		sendBuf[0] = 0x68;
		memset(&sendBuf[1], 0xAA, 6); //地址
		sendBuf[7] = 0x68;
		sendBuf[8] = format07->Ctrl;
		sendBuf[9] = 0x02; //长度
		memcpy(&sendBuf[10], format07->SearchTime, 2); //搜表时长

		for (i = 10; i < 12; i++) {
			sendBuf[i] += 0x33;
		}

		sendBuf[12] = getCS645(&sendBuf[0], 16);
		sendBuf[13] = 0x16;

		sendlen = 14;
		break;
	case 0x08: //广播校时
		sendBuf[0] = 0x68;
		memcpy(&sendBuf[1], addrBuff, 6); //地址
		sendBuf[7] = 0x68;
		sendBuf[8] = format07->Ctrl;
		sendBuf[9] = 0x06; //长度
		int32u2bcd(format07->Time[0], &sendBuf[10], inverted); //校时时间
		int32u2bcd(format07->Time[1], &sendBuf[11], inverted);
		int32u2bcd(format07->Time[2], &sendBuf[12], inverted);
		int32u2bcd(format07->Time[3], &sendBuf[13], inverted);
		int32u2bcd(format07->Time[4], &sendBuf[14], inverted);
		int32u2bcd(format07->Time[5] % 100, &sendBuf[15], inverted);

		for (i = 10; i < 16; i++) {
			sendBuf[i] += 0x33;
		}

		sendBuf[16] = getCS645(&sendBuf[0], 16);
		sendBuf[17] = 0x16;

		sendlen = 18;
		break;
	case 0xff: //读负荷曲线
		sendBuf[0] = 0x68;
		memcpy(&sendBuf[1], addrBuff, 6); //地址
		sendBuf[7] = 0x68;
		sendBuf[8] = 0x11;
		sendBuf[9] = 10; //长度
		memcpy(&sendBuf[10], format07->DI, 4); //数据标识
		sendBuf[14] = format07->sections;
		sendBuf[15] = format07->startMinute;
		sendBuf[16] = format07->startHour;
		sendBuf[17] = format07->startDay;
		sendBuf[18] = format07->startMonth;
		sendBuf[19] = format07->startYear;

		for (i = 10; i < 20; i++) {
			sendBuf[i] += 0x33;
		}

		sendBuf[20] = getCS645(&sendBuf[0], 20);
		sendBuf[21] = 0x16;

		sendlen = 22;
		break;
	default:
		sendlen = -1;
		break;
	}

	if (getZone("GW")!=0)
	{
		DEBUG_TIME_LINE("sendlen: %d", sendlen);
		if(sendlen > 0) {//dlt645-07协议明确要求, 在前面加4个0xFE, 以唤醒总线设备
			tmpbuf[0] = 0xfe;
			tmpbuf[1] = 0xfe;
			tmpbuf[2] = 0xfe;
			tmpbuf[3] = 0xfe;
			memcpy(&tmpbuf[4], sendBuf, sendlen);
			sendlen += 4;
			memcpy(sendBuf,tmpbuf,sendlen);
		}
	}

	return sendlen;
}

//07报文解析入口函数
INT8S analyzeProtocol07(FORMAT07* format07, INT8U* recvBuf, const INT16U recvLen, BOOLEAN *nextFlag)
{
	INT16U i, count, count2;
	INT8S ret = 0;
	count = getFECount(recvBuf, recvLen);//得到待解析报文中前导符FE的个数
	count2 = getFFCount(recvBuf, recvLen);//得到待解析报文中后缀FF的个数（江苏II型新联的电表）

	if (isValid645(&recvBuf[count], recvLen-count-count2) == 0)	//校验通过
	{
		format07->SEQ = 0;
		memcpy(&format07->Addr[0], &recvBuf[count+1], 6);
		format07->Ctrl = recvBuf[count+8];
		format07->Length = recvBuf[count+9];
		if (format07->Ctrl & 0x20)//控制码D5=1，表示有后续帧
		{
			*nextFlag = TRUE;
		}
		else
		{
			*nextFlag = FALSE;
		}

		for (i=count+10; i<count+10+format07->Length; i++)//数据域-33H处理
		{
			recvBuf[i] -= 0x33;
		}

		if ((format07->Ctrl == 0x91) || (format07->Ctrl == 0xB1))//正常应答
		{
			memcpy(format07->DI, &recvBuf[count+10], 4);
			memcpy(format07->Data, &recvBuf[count+14], format07->Length-4);

			return 0;
		}
		else if ((format07->Ctrl == 0x92) || (format07->Ctrl == 0xB2))//正常应答读后续帧
		{
			memcpy(format07->DI, &recvBuf[count+10], 4);
			memcpy(format07->Data, &recvBuf[count+14], format07->Length-5);
			format07->SEQ = recvBuf[count+10+format07->Length-1];
			ret = 0;
		}
		else if (format07->Ctrl == 0x11)//读数据
		{
			memcpy(format07->DI, &recvBuf[count+10], 4);
			ret = 4;
		}
		else if (format07->Ctrl == 0x13)//读表地址
		{
			INT8U tmpAddr[6] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
			if (memcmp(&recvBuf[count+1], tmpAddr, 6)==0)
			{
				ret = 5;
			}
		}
		else if (format07->Ctrl == 0x08)//广播校时
		{
			INT32U Time[6];
			for(i=0; i<6; i++)
			{
				Time[i] = format07->Time[i];
			}
			bcd2int32u(&recvBuf[count+10], 1, inverted, &Time[0]);
			bcd2int32u(&recvBuf[count+11], 1, inverted, &Time[1]);
			bcd2int32u(&recvBuf[count+12], 1, inverted, &Time[2]);
			bcd2int32u(&recvBuf[count+13], 1, inverted, &Time[3]);
			bcd2int32u(&recvBuf[count+14], 1, inverted, &Time[4]);
			bcd2int32u(&recvBuf[count+15], 1, inverted, &Time[5]);

			for(i=0; i<6; i++)
			{
				format07->Time[i] = Time[i];
			}
			ret = 1;
		}else if (format07->Ctrl == 0x14)//写数据（校表-自定义）
		{
			memcpy(format07->DI, &recvBuf[count+10], 4);
			memcpy(format07->Data, &recvBuf[count+14], format07->Length-4);
			ret = 8;
		}
		else if ((format07->Ctrl==0xD1) || (format07->Ctrl==0xD2))//异常应答
		{
			format07->Err = recvBuf[count+10];
			fprintf(stderr,"Err=%d\n", format07->Err);
			if (format07->Err == 0x02)//电表异常应答，无该数据项
			{
				ret = -1;
			}
			else//电表异常应答，未知错误
			{
				ret = -2;
			}
		}
		else//其他功能
		{
			ret = -3;
		}
	}
	else//校验错误
	{
		ret = -4;
	}
	return ret;
}


//97报文解析入口函数
INT8S analyzeProtocol97(FORMAT97* format97, INT8U* recvBuf, const INT16U recvLen, BOOLEAN *nextFlag)
{
	INT16U i, count, count2;
	INT8S ret = 0;
	count = getFECount(recvBuf, recvLen);//得到待解析报文中前导符FE的个数
	count2 = getFFCount(recvBuf, recvLen);//得到待解析报文中后缀FF的个数（江苏II型新联的电表）

	if (isValid645(&recvBuf[count], recvLen-count-count2) == 0)	//校验通过
	{
		format97->SEQ = 0;
		memcpy(&format97->Addr[0], &recvBuf[count+1], 6);
		format97->Ctrl = recvBuf[count+8];
		format97->Length = recvBuf[count+9];

		if (format97->Ctrl & 0x20)//控制码D5=1，表示有后续帧
		{
			*nextFlag = TRUE;
		}
		else
		{
			*nextFlag = FALSE;
		}

		for (i=count+10; i<count+10+format97->Length; i++)//数据域-33H处理
		{
			recvBuf[i] -= 0x33;
		}

		if ((format97->Ctrl == 0x81) || (format97->Ctrl == 0xA1))//正常应答
		{
			memcpy(format97->DI, &recvBuf[count+10], 2);
			memcpy(format97->Data, &recvBuf[count+12], format97->Length);
			ret = 0;
		}
		else if ((format97->Ctrl == 0x82) || (format97->Ctrl == 0xA2))//正常应答读后续帧
		{
			memcpy(format97->DI, &recvBuf[count+10], 2);
			memcpy(format97->Data, &recvBuf[count+14], format97->Length-3);
			format97->SEQ = recvBuf[count+10+format97->Length-1];
			ret = 0;
		}
		else if (format97->Ctrl == 0x08)//广播校时
		{
			memcpy(format97->Time, &recvBuf[count+10], 6);
			ret = 1;
		}
		else if (format97->Ctrl == 0xC1)//异常应答
		{
			format97->Err = recvBuf[count+10];
			if (format97->Err == 0x02)//电表异常应答，无该数据项
			{
				ret = -1;
			}
			else//电表异常应答，未知错误
			{
				ret = -2;
			}
		}
		else//其他功能
		{
			ret = -3;
		}
	}
	else//校验错误
	{
		ret = -4;
	}
	return ret;
}


//97报文组合入口函数
INT16S composeProtocol97(FORMAT97* format97, INT8U* sendBuf)
{
	INT8U i;
	INT8U addrBuff[6] = {0};
	fprintf(stderr,"composeProtocol07 format07->Addr = %02x%02x%02x%02x%02x%02x",
			format97->Addr[0],format97->Addr[1],format97->Addr[2],format97->Addr[3],format97->Addr[4],format97->Addr[5]);
	reversebuff(format97->Addr,6,addrBuff);

	sendBuf[0] = 0x68;
	memcpy(&sendBuf[1], addrBuff, 6);//地址
	sendBuf[7] = 0x68;
	sendBuf[8] = format97->Ctrl;//控制码
	sendBuf[9] = 0x02;//长度
	memcpy(&sendBuf[10], format97->DI, 2);//数据标识

	for (i=10; i<12; i++)//数据域+33H处理
	{
		sendBuf[i] += 0x33;
	}

	sendBuf[12] = getCS645(&sendBuf[0], 12);
	sendBuf[13] = 0x16;

	return 14;
}

