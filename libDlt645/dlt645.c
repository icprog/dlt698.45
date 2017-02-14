
#define LIB645_VER 	1
#include "dlt645.h"
#include <string.h>
#include <stdio.h>

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

	if (format07->Ctrl == 0x11)//读数据
	{
		sendBuf[0] = 0x68;
		memcpy(&sendBuf[1], format07->Addr, 6);//地址
		sendBuf[7] = 0x68;
		sendBuf[8] = format07->Ctrl;
		sendBuf[9] = 0x04;//长度
		memcpy(&sendBuf[10], format07->DI, 4);//数据标识

		for (i=10; i<14; i++)
		{
			sendBuf[i] += 0x33;
		}

		sendBuf[14] = getCS645(&sendBuf[0], 14);
		sendBuf[15] = 0x16;

		return 16;
	}
	if (format07->Ctrl == 0x12)//读后续数据
	{
		sendBuf[0] = 0x68;
		memcpy(&sendBuf[1], format07->Addr, 6);//地址
		sendBuf[7] = 0x68;
		sendBuf[8] = format07->Ctrl;
		sendBuf[9] = 0x05;//长度
		memcpy(&sendBuf[10], format07->DI, 4);//数据标识
		sendBuf[14] = format07->SEQ;

		for (i=10; i<15; i++)
		{
			sendBuf[i] += 0x33;
		}

		sendBuf[15] = getCS645(&sendBuf[0], 15);
		sendBuf[16] = 0x16;

		return 17;
	}

	return -1;
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
//			ret = 0;

			INT8U errData[256];//7.10 for shandong
			memset(errData, 0xff, 256);
			if (memcmp(errData, format07->Data, format07->Length-4) == 0)
				return -1;
			else
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
