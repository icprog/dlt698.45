/*
 * dealProtocol.c
 *
 *  Created on: 2013-3-20
 *      Author: Administrator
 */

#include <string.h>
#include "dealData.h"
#include "stdio.h"
#include "PublicFunction.h"


//返回值说明：
//-1：错误
//0：无数据单元
//其他：组成或解析的报文长度
/************************************** AFN=00 **************************************/
INT16S dealAFN00_F1(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	INT8U i;
	INT32U channelStatus;

	format3762->afn00_f1.CommandStatus = data[0] & 0x01;

	channelStatus = ((data[3]<<24) + (data[2]<<16) + (data[1]<<8) + data[0]) >> 1;
	for (i=0; i<31; i++)
	{
		format3762->afn00_f1.ChannelStatus[i] = (channelStatus>>i) & 0x01;
	}

	format3762->afn00_f1.WaitingTime = (data[5]<<8) + data[4];
	return 6;

//	INT8U i;
//	INT32U channelStatus;
//
//	format3762->afn00_f1.CommandStatus = data[0] & 0x01;
//
//	channelStatus = ((data[1]<<8) + data[0]) >> 1;
//	for (i=0; i<16; i++)
//	{
//		format3762->afn00_f1.ChannelStatus[i] = (channelStatus>>i) & 0x01;
//	}
//
//	format3762->afn00_f1.WaitingTime = (data[3]<<8) + data[2];
//	return 4;
}

INT16S dealAFN00_F2(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	format3762->afn00_f2.ErrStatus = data[0];
	return 1;
}

INT16S dealAFN00(FORMAT3762* format3762, INT8U dir, INT8U fn, INT8U* data)
{
	INT16S len;
	switch(fn)
	{
	case 1:
		len = dealAFN00_F1(format3762, dir, data);
		break;
	case 2:
		len = dealAFN00_F2(format3762, dir, data);
		break;
	default:
		len = -1;
		break;
	}
	return len;
}


/************************************** AFN=01 **************************************/
INT16S dealAFN01_F1(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return 0;
	}
	else
	{
		return -1;
	}
	return -1;
}

INT16S dealAFN01_F2(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return 0;
	}
	else
	{
		return -1;
	}
	return -1;
}

INT16S dealAFN01_F3(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return 0;
	}
	else
	{
		return -1;
	}
	return -1;
}

INT16S dealAFN01(FORMAT3762* format3762, INT8U dir, INT8U fn, INT8U* data)
{
	INT16S len;
	switch(fn)
	{
	case 1:
		len = dealAFN01_F1(format3762, dir, data);
		break;
	case 2:
		len = dealAFN01_F2(format3762, dir, data);
		break;
	case 3:
		len = dealAFN01_F3(format3762, dir, data);
		break;
	default:
		len = -1;
		break;
	}
	return len;
}



/************************************** AFN=02 **************************************/
INT16S dealAFN02_F1(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		data[0] = format3762->afn02_f1_down.Protocol;
		data[1] = format3762->afn02_f1_down.MsgLength;
		memcpy(&data[2], &format3762->afn02_f1_down.MsgContent[0], data[1]);

		return (2+data[1]);
	}
	else
	{
		format3762->afn02_f1_up.Protocol = data[0];
		format3762->afn02_f1_up.MsgLength = data[1];
		memcpy(&format3762->afn02_f1_up.MsgContent[0], &data[2], data[1]);

		return (2+data[1]);
	}
	return -1;
}

INT16S dealAFN02(FORMAT3762* format3762, INT8U dir, INT8U fn, INT8U* data)
{
	INT16S len;
	switch(fn)
	{
	case 1:
		len = dealAFN02_F1(format3762, dir, data);
		break;
	default:
		len = -1;
		break;
	}
	return len;
}



/************************************** AFN=03 **************************************/
INT16S dealAFN03_F1(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return 0;
	}
	else
	{
		memcpy(&format3762->afn03_f1_up.VendorCode, &data[0], 2);
		memcpy(&format3762->afn03_f1_up.ChipCode, &data[2], 2);
		INT32U year,month,day,version[2];
		bcd2int32u(&data[4], 1, inverted, &day);
		bcd2int32u(&data[5], 1, inverted, &month);
		bcd2int32u(&data[6], 1, inverted, &year);
		bcd2int32u(&data[7], 1, inverted, &version[0]);
		bcd2int32u(&data[8], 1, inverted, &version[1]);

		format3762->afn03_f1_up.VersionDay = day & 0xff;
		format3762->afn03_f1_up.VersionMonth = month & 0xff;
		format3762->afn03_f1_up.VersionYear = year & 0xff;
		format3762->afn03_f1_up.Version[0] = version[0] & 0xff;
		format3762->afn03_f1_up.Version[1] = version[1] & 0xff;

		return 9;
	}
	return -1;
}

INT16S dealAFN03_F2(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return 0;
	}
	else
	{
		format3762->afn03_f2_up.NoiseIntensity = data[0] & 0x0f;
		return 1;
	}
	return -1;
}

INT16S dealAFN03_F3(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		data[0] = format3762->afn03_f3_down.Point;
		data[1] = format3762->afn03_f3_down.PointNum;
		return 2;
	}
	else
	{
		INT8U i;

		format3762->afn03_f3_up.SlavePointNum = data[0];
		format3762->afn03_f3_up.SlavePointNum_Trans = data[1];

		for(i=0; i<format3762->afn03_f3_up.SlavePointNum_Trans; i++)
		{
			memcpy(&format3762->afn03_f3_up.SlavePoint[i].Addr[0], &data[2+i*8], 6);///bcd
			format3762->afn03_f3_up.SlavePoint[i].RepeaterLevel = data[8+i*8];
			format3762->afn03_f3_up.SlavePoint[i].InterceptQuality = data[8+i*8];
			format3762->afn03_f3_up.SlavePoint[i].InterceptCount = data[9+i*8];
		}
		return (2+8*data[1]);
	}
	return -1;
}

INT16S dealAFN03_F4(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return 0;
	}
	else
	{
		memcpy(&format3762->afn03_f4_up.MasterPointAddr[0], &data[0], 6);
		return 6;
	}
	return -1;
}

INT16S dealAFN03_F5(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return 0;
	}
	else
	{
		INT8U i;
		format3762->afn03_f5_up.RateNum = data[0] & 0x0f;
		format3762->afn03_f5_up.Channel = (data[0]>>4) & 0x03;
		format3762->afn03_f5_up.ReadMode = (data[0]>>6) & 0x03;
		format3762->afn03_f5_up.ChannelNum = data[1] & 0x0f;

		for (i=0; i<format3762->afn03_f5_up.RateNum; i++)
		{
			format3762->afn03_f5_up.Rate[i].Rate = ((data[3+i*2] & 0x7f)<<8) + data[2+i*2];
			format3762->afn03_f5_up.Rate[i].RateUnit = (data[3+i*2]>>7) & 0x01;
		}
		return (2+2*format3762->afn03_f5_up.RateNum);
	}
	return -1;
}

INT16S dealAFN03_F6(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		data[0] = format3762->afn03_f6_down.Duration;
		return 1;
	}
	else
	{
		format3762->afn03_f6_up.DisturbStatus = data[0];
		return 1;
	}
	return -1;
}

INT16S dealAFN03_F7(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return 0;
	}
	else
	{
		format3762->afn03_f7_up.OverTime = data[0];
		return 1;
	}
	return -1;
}

INT16S dealAFN03_F8(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return 0;
	}
	else
	{
		format3762->afn03_f8_up.WirelessChannel = data[0];
		format3762->afn03_f8_up.WirelessPower = data[1];
		return 2;
	}
	return -1;
}

INT16S dealAFN03_F9(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		data[0] = format3762->afn03_f9_down.Protocol;
		data[1] = format3762->afn03_f9_down.MsgLength;
		memcpy(&data[2], format3762->afn03_f9_down.MsgContent, data[1]);
		return (2+data[1]);
	}
	else
	{
		format3762->afn03_f9_up.DelayTime = (data[1]<<8) + data[0];
		format3762->afn03_f9_up.Protocol = data[2];
		format3762->afn03_f9_up.MsgLength = data[3];
		memcpy(format3762->afn03_f9_up.MsgContent, &data[4], data[3]);
		return (4+data[3]);
	}
	return -1;
}

INT16S dealAFN03_F10(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return 0;
	}
	else
	{
		INT8U i;
		INT32U year,month,day,version[2];

		format3762->afn03_f10_up.CommType = data[0] & 0x0f;
		format3762->afn03_f10_up.RouterManagement = (data[0]>>4) & 0x01;
		format3762->afn03_f10_up.SlavePointMode = (data[0]>>5) & 0x01;
		format3762->afn03_f10_up.ReadMode = (data[0]>>6) & 0x03;

		format3762->afn03_f10_up.TransParam = data[1] & 0x07;
		format3762->afn03_f10_up.ChangeMode = (data[1]>>3) & 0x03;
		format3762->afn03_f10_up.ConfirmMode = (data[1]>>5) & 0x01;
		format3762->afn03_f10_up.ExecuteMode = (data[1]>>6) & 0x03;

		format3762->afn03_f10_up.ChannelNum = data[2] & 0x1f;
		format3762->afn03_f10_up.PowerOffInfo = (data[2]>>5) & 0x07;
		format3762->afn03_f10_up.RateNum = data[3] & 0x0f;

		format3762->afn03_f10_up.MonitorOverTime = data[6];
		format3762->afn03_f10_up.BroadcastOverTime = (data[8]<<8) + data[7];
		format3762->afn03_f10_up.MsgMaxLen = (data[10]<<8) + data[9];
		format3762->afn03_f10_up.PackageMaxLen = (data[12]<<8) + data[11];
		format3762->afn03_f10_up.UpdateWaitingTime = data[13];
		memcpy(format3762->afn03_f10_up.MasterPointAddr, &data[14], 6);

//		int32u2bcd(data[15], &format3762->afn03_f10_up.MasterPointAddr[0], inverted);
//		int32u2bcd(data[16], &format3762->afn03_f10_up.MasterPointAddr[1], inverted);
//		int32u2bcd(data[17], &format3762->afn03_f10_up.MasterPointAddr[2], inverted);
//		int32u2bcd(data[18], &format3762->afn03_f10_up.MasterPointAddr[3], inverted);
//		int32u2bcd(data[19], &format3762->afn03_f10_up.MasterPointAddr[4], inverted);
//		int32u2bcd(data[20], &format3762->afn03_f10_up.MasterPointAddr[5], inverted);

		format3762->afn03_f10_up.SlavePointMaxNum = (data[21]<<8) + data[20];
		format3762->afn03_f10_up.CurrSlavePointNum = (data[23]<<8) + data[22];

		bcd2int32u(&data[24], 1, inverted, &day);
		bcd2int32u(&data[25], 1, inverted, &month);
		bcd2int32u(&data[26], 1, inverted, &year);
		format3762->afn03_f10_up.PublishDay = day & 0xff;
		format3762->afn03_f10_up.PublishMonth = month & 0xff;
		format3762->afn03_f10_up.PublishYear = year & 0xff;

		bcd2int32u(&data[27], 1, inverted, &day);
		bcd2int32u(&data[28], 1, inverted, &month);
		bcd2int32u(&data[29], 1, inverted, &year);
		format3762->afn03_f10_up.RecordDay = day & 0xff;
		format3762->afn03_f10_up.RecordMonth = month & 0xff;
		format3762->afn03_f10_up.RecordYear = year & 0xff;

		memcpy(&format3762->afn03_f10_up.ModuleInfo.VendorCode, &data[30], 2);
		memcpy(&format3762->afn03_f10_up.ModuleInfo.ChipCode, &data[32], 2);
		bcd2int32u(&data[34], 1, inverted, &day);
		bcd2int32u(&data[35], 1, inverted, &month);
		bcd2int32u(&data[36], 1, inverted, &year);
		bcd2int32u(&data[37], 1, inverted, &version[0]);
		bcd2int32u(&data[38], 1, inverted, &version[1]);
		format3762->afn03_f10_up.ModuleInfo.VersionDay = day & 0xff;
		format3762->afn03_f10_up.ModuleInfo.VersionMonth = month & 0xff;
		format3762->afn03_f10_up.ModuleInfo.VersionYear = year & 0xff;
		format3762->afn03_f10_up.ModuleInfo.Version[0] = version[0] & 0xff;
		format3762->afn03_f10_up.ModuleInfo.Version[1] = version[1] & 0xff;

		for (i=0; i<format3762->afn03_f10_up.RateNum; i++)
		{
			format3762->afn03_f10_up.Rate[i].Rate = ((data[40+i*2] & 0x7f)<<8) + data[49+i*2];
			format3762->afn03_f10_up.Rate[i].RateUnit = (data[40+i*2]>>7) & 0x01;
		}
		return (39+format3762->afn03_f10_up.RateNum*2);
	}
	return -1;
}

INT16S dealAFN03_F11(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		data[0] = format3762->afn03_f11_down.AFN;
		return 1;
	}
	else
	{
		format3762->afn03_f11_up.AFN = data[0];
		INT16U i;
		for (i=0; i<256; i++)
		{
			format3762->afn03_f11_up.FnSupport[i] = (data[1+i/8]>>(i%8)) & 0x01;
		}
		return 33;
	}
	return -1;
}

INT16S dealAFN03(FORMAT3762* format3762, INT8U dir, INT8U fn, INT8U* data)
{
	INT16S len;
	switch(fn)
	{
	case 1:
		len = dealAFN03_F1(format3762, dir, data);
		break;
	case 2:
		len = dealAFN03_F2(format3762, dir, data);
		break;
	case 3:
		len = dealAFN03_F3(format3762, dir, data);
		break;
	case 4:
		len = dealAFN03_F4(format3762, dir, data);
		break;
	case 5:
		len = dealAFN03_F5(format3762, dir, data);
		break;
	case 6:
		len = dealAFN03_F6(format3762, dir, data);
		break;
	case 7:
		len = dealAFN03_F7(format3762, dir, data);
		break;
	case 8:
		len = dealAFN03_F8(format3762, dir, data);
		break;
	case 9:
		len = dealAFN03_F9(format3762, dir, data);
		break;
	case 10:
		len = dealAFN03_F10(format3762, dir, data);
		break;
	case 11:
		len = dealAFN03_F11(format3762, dir, data);
		break;
	default:
		len = -1;
		break;
	}
	return len;
}




/************************************** AFN=04 **************************************/
INT16S dealAFN04_F1(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		data[0] = format3762->afn04_f1_down.Duration;
		return 1;
	}
	else
	{
		return -1;
	}
	return -1;
}

INT16S dealAFN04_F2(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return 0;
	}
	else
	{
		return -1;
	}
	return -1;
}

INT16S dealAFN04_F3(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		data[0] = format3762->afn04_f3_down.rate;
		memcpy(&data[1], format3762->afn04_f3_down.destAddr, 6);
		data[7] = format3762->afn04_f3_down.protocol;
		data[8] = format3762->afn04_f3_down.msgLen;
		memcpy(&data[9], format3762->afn04_f3_down.msgContent, format3762->afn04_f3_down.msgLen);
		return (9+data[8]);
	}
	else
	{
		return -1;
	}
	return -1;
}

INT16S dealAFN04(FORMAT3762* format3762, INT8U dir, INT8U fn, INT8U* data)
{
	INT16S len;
	switch(fn)
	{
	case 1:
		len = dealAFN04_F1(format3762, dir, data);
		break;
	case 2:
		len = dealAFN04_F2(format3762, dir, data);
		break;
	case 3:
		len = dealAFN04_F3(format3762, dir, data);
		break;
	default:
		len = -1;
		break;
	}
	return len;
}




/************************************** AFN=05 **************************************/
INT16S dealAFN05_F1(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		memcpy(&data[0], &format3762->afn05_f1_down.MasterPointAddr[0], 6);
		return 6;
	}
	else
	{
		return -1;
	}
	return -1;
}

INT16S dealAFN05_F2(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		data[0] = format3762->afn05_f2_down.EventReportFlag;
		return 1;
	}
	else
	{
		return -1;
	}
	return -1;
}

INT16S dealAFN05_F3(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		INT8U i;
		data[0] = format3762->afn05_f3_down.ctrl;
		data[1] = format3762->afn05_f3_down.MsgLength;

		for(i=0; i<format3762->afn05_f3_down.MsgLength; i++)
		{
			memcpy(&data[2], &format3762->afn05_f3_down.MsgContent[0], data[1]);
		}
		return (2+data[1]);
	}
	else
	{
		return -1;
	}
	return -1;
}

INT16S dealAFN05_F4(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		data[0] = format3762->afn05_f4_down.OverTime;
		return 1;
	}
	else
	{
		return -1;
	}
	return -1;
}

INT16S dealAFN05_F5(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		data[0] = format3762->afn05_f5_down.WirelessChannel;
		data[1] = format3762->afn05_f5_down.WirelessPower;
		return 2;
	}
	else
	{
		return -1;
	}
	return -1;
}

INT16S dealAFN05(FORMAT3762* format3762, INT8U dir, INT8U fn, INT8U* data)
{
	INT16S len;
	switch(fn)
	{
	case 1:
		len = dealAFN05_F1(format3762, dir, data);
		break;
	case 2:
		len = dealAFN05_F2(format3762, dir, data);
		break;
	case 3:
		len = dealAFN05_F3(format3762, dir, data);
		break;
	case 4:
		len = dealAFN05_F4(format3762, dir, data);
		break;
	case 5:
		len = dealAFN05_F5(format3762, dir, data);
		break;
	default:
		len = -1;
		break;
	}
	return len;
}




/************************************** AFN=06 **************************************/
INT16S dealAFN06_F1(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return -1;
	}
	else
	{
		INT8U i;
		format3762->afn06_f1_up.Num = data[0];

		for (i=0; i<format3762->afn06_f1_up.Num; i++)
		{
			memcpy(&format3762->afn06_f1_up.SlavePoint[i].Addr[0], &data[1+9*i], 6);
			format3762->afn06_f1_up.SlavePoint[i].Protocol = data[7+9*i];
			format3762->afn06_f1_up.SlavePoint[i].Index = (data[9+9*i]<<8) + data[8+9*i];
		}
		return (1+9*data[0]);
	}
	return -1;
}

INT16S dealAFN06_F2(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return -1;
	}
	else
	{
		format3762->afn06_f2_up.SlavePointIndex = (data[1]<<8) + data[0];
		format3762->afn06_f2_up.Protocol = data[2];
		format3762->afn06_f2_up.TransTime = (data[4]<<8) + data[3];
		format3762->afn06_f2_up.MsgLength = data[5];
		memcpy(&format3762->afn06_f2_up.MsgContent, &data[6], data[5]);
		return (6+data[5]);
	}
	return -1;
}

INT16S dealAFN06_F3(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return -1;
	}
	else
	{
		format3762->afn06_f3_up.WorkChange = data[0];
		return 1;
	}
	return -1;
}

INT16S dealAFN06_F4(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return -1;
	}
	else
	{
		INT8U i;
		format3762->afn06_f4_up.Num = data[0];
		memcpy(&format3762->afn06_f4_up.SlavePoinAddr[0], &data[1], 6);
		format3762->afn06_f4_up.Protocol = data[7];
		format3762->afn06_f4_up.Index = (data[9]<<8) + data[8];
		format3762->afn06_f4_up.DeviceType = data[10];
		format3762->afn06_f4_up.SubPointNum = data[11];
		format3762->afn06_f4_up.SubPointNum_Trans = data[12];

		for(i=0;i<format3762->afn06_f4_up.SubPointNum_Trans;i++)
		{
			memcpy(&format3762->afn06_f4_up.SubPoint[i].Addr[0], &data[13+7*i], 6);
			format3762->afn06_f4_up.SubPoint[i].Protocol = data[19+7*i];
		}
		return (13+7*data[12]);
	}
	return -1;
}

INT16S dealAFN06_F5(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return -1;
	}
	else
	{
		int len=0;
		format3762->afn06_f5_up.DeviceType = data[0];
		format3762->afn06_f5_up.Protocol = data[1];
		format3762->afn06_f5_up.MsgLength = data[2];
		len = data[2];
		if (len > 256)
			len = 256;
		memcpy(&format3762->afn06_f5_up.MsgContent, &data[3], len);
		return (3+data[2]);
	}
	return -1;
}

INT16S dealAFN06_F6(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return -1;
	}
	else
	{
		INT8U i;
		format3762->afn06_f6_up.Num = data[0];
		for(i=0;i<format3762->afn06_f6_up.Num;i++)
		{
			memcpy(&format3762->afn06_f6_up.SlavePointAddr[0], &data[1+6*i], 6);
		}
		return (1+6*data[0]);
	}
	return -1;
}

INT16S dealAFN06(FORMAT3762* format3762, INT8U dir, INT8U fn, INT8U* data)
{
	INT16S len;
	switch(fn)
	{
	case 1:
		len = dealAFN06_F1(format3762, dir, data);
		break;
	case 2:
		len = dealAFN06_F2(format3762, dir, data);
		break;
	case 3:
		len = dealAFN06_F3(format3762, dir, data);
		break;
	case 4:
		len = dealAFN06_F4(format3762, dir, data);
		break;
	case 5:
		len = dealAFN06_F5(format3762, dir, data);
		break;
	case 6:
		len = dealAFN06_F6(format3762, dir, data);
		break;
	default:
		len = -1;
		break;
	}
	return len;
}



/************************************** AFN=10 **************************************/
INT16S dealAFN10_F1(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return 0;
	}
	else
	{
		format3762->afn10_f1_up.Num = (data[1]<<8) + data[0];
		format3762->afn10_f1_up.SupportNum = (data[3]<<8) + data[2];
		return 4;
	}
	return -1;
}

INT16S dealAFN10_F2(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		data[0] = format3762->afn10_f2_down.Index & 0xff;
		data[1] = (format3762->afn10_f2_down.Index>>8) & 0xff;
		data[2] = format3762->afn10_f2_down.Num;
		return 3;
	}
	else
	{
		INT8U i;
		format3762->afn10_f2_up.Num = (data[1]<<8) + data[0];
		format3762->afn10_f2_up.ReplyNum = data[2];
		for (i=0;i<format3762->afn10_f2_up.ReplyNum;i++)
		{
			memcpy(&format3762->afn10_f2_up.SlavePoint[i].Addr[0], &data[3+8*i], 6);
			format3762->afn10_f2_up.SlavePoint[i].RepeaterLevel = data[9+8*i] & 0x0f;
			format3762->afn10_f2_up.SlavePoint[i].InterceptQuality = (data[9+8*i]>>4) & 0x0f;
			format3762->afn10_f2_up.SlavePoint[i].Phase = data[10+8*i] & 0x07;
			format3762->afn10_f2_up.SlavePoint[i].Protocol = (data[10+8*i]>>3) & 0x07;
		}
		return (3+8*data[2]);
	}
	return -1;
}

INT16S dealAFN10_F3(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		memcpy(&data[0], &format3762->afn10_f3_down.SlavePointAddr[0], 6);
		return 6;
	}
	else
	{
		INT8U i;
		format3762->afn10_f3_up.Num = data[0];
		for (i=0;i<format3762->afn10_f3_up.Num;i++)
		{
			memcpy(&format3762->afn10_f3_up.SlavePoint[i].Addr[0], &data[1+8*i], 6);
			format3762->afn10_f3_up.SlavePoint[i].RepeaterLevel = data[7+8*i] & 0x0f;
			format3762->afn10_f3_up.SlavePoint[i].InterceptQuality = (data[7+8*i]>>4) & 0x0f;
			format3762->afn10_f3_up.SlavePoint[i].Phase = data[8+8*i] & 0x07;
			format3762->afn10_f3_up.SlavePoint[i].Protocol = (data[8+8*i]>>3) & 0x07;
		}
		return (1+8*data[2]);
	}
	return -1;
}

INT16S dealAFN10_F4(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return 0;
	}
	else
	{
		format3762->afn10_f4_up.FinishFlag = data[0] & 0x01;
		format3762->afn10_f4_up.WorkFlag = (data[0]>>1) & 0x01;
		format3762->afn10_f4_up.ReportFlag = (data[0]>>2) & 0x01;
		format3762->afn10_f4_up.ErrCode = (data[0]>>4) & 0x0f;
		format3762->afn10_f4_up.Num = (data[2]<<8) + data[1];
		format3762->afn10_f4_up.FinishedNum = (data[4]<<8) + data[3];
		format3762->afn10_f4_up.RepeaterFinishedNum = (data[6]<<8) + data[5];
		format3762->afn10_f4_up.WorkStatus = data[7] & 0x01;
		format3762->afn10_f4_up.RegisterStatus = (data[7]>>1) & 0x01;
		format3762->afn10_f4_up.Rate = (data[9]<<8) + data[8];
		format3762->afn10_f4_up.RepeaterLevel_Phase1 = data[10];
		format3762->afn10_f4_up.RepeaterLevel_Phase2 = data[11];
		format3762->afn10_f4_up.RepeaterLevel_Phase3 = data[12];
		format3762->afn10_f4_up.WorkStep_Phase1 = data[13];
		format3762->afn10_f4_up.WorkStep_Phase2 = data[14];
		format3762->afn10_f4_up.WorkStep_Phase3 = data[15];
		return 16;
	}
	return -1;
}

INT16S dealAFN10_F5(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		data[0] = format3762->afn10_f5_down.Index & 0xff;
		data[1] = (format3762->afn10_f5_down.Index>>8) & 0xff;
		data[2] = format3762->afn10_f5_down.Num;
		return 3;
	}
	else
	{
		INT8U i;
		format3762->afn10_f5_up.Num = (data[1]<<8) + data[0];
		format3762->afn10_f5_up.ReplyNum = data[2];
		for (i=0;i<format3762->afn10_f5_up.ReplyNum;i++)
		{
			memcpy(&format3762->afn10_f5_up.SlavePoint[i].Addr[0], &data[3+8*i], 6);
			format3762->afn10_f5_up.SlavePoint[i].RepeaterLevel = data[9+8*i] & 0x0f;
			format3762->afn10_f5_up.SlavePoint[i].InterceptQuality = (data[9+8*i]>>4) & 0x0f;
			format3762->afn10_f5_up.SlavePoint[i].Phase = data[10+8*i] & 0x07;
			format3762->afn10_f5_up.SlavePoint[i].Protocol = (data[10+8*i]>>3) & 0x07;
		}
		return (3+8*data[2]);
	}
	return -1;
}

INT16S dealAFN10_F6(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		data[0] = format3762->afn10_f6_down.Index & 0xff;
		data[1] = (format3762->afn10_f6_down.Index>>8) & 0xff;
		data[2] = format3762->afn10_f6_down.Num;
		return 3;
	}
	else
	{
		INT8U i;
		format3762->afn10_f6_up.Num = (data[1]<<8) + data[0];
		format3762->afn10_f6_up.ReplyNum = data[2];
		for (i=0;i<format3762->afn10_f6_up.ReplyNum;i++)
		{
			memcpy(&format3762->afn10_f6_up.SlavePoint[i].Addr[0], &data[3+8*i], 6);
			format3762->afn10_f6_up.SlavePoint[i].RepeaterLevel = data[9+8*i] & 0x0f;
			format3762->afn10_f6_up.SlavePoint[i].InterceptQuality = (data[9+8*i]>>4) & 0x0f;
			format3762->afn10_f6_up.SlavePoint[i].Phase = data[10+8*i] & 0x07;
			format3762->afn10_f6_up.SlavePoint[i].Protocol = (data[10+8*i]>>3) & 0x07;
		}
		return (3+8*data[2]);
	}
	return -1;
}

INT16S dealAFN10(FORMAT3762* format3762, INT8U dir, INT8U fn, INT8U* data)
{
	INT16S len;
	switch(fn)
	{
	case 1:
		len = dealAFN10_F1(format3762, dir, data);
		break;
	case 2:
		len = dealAFN10_F2(format3762, dir, data);
		break;
	case 3:
		len = dealAFN10_F3(format3762, dir, data);
		break;
	case 4:
		len = dealAFN10_F4(format3762, dir, data);
		break;
	case 5:
		len = dealAFN10_F5(format3762, dir, data);
		break;
	case 6:
		len = dealAFN10_F6(format3762, dir, data);
		break;
	default:
		len = -1;
		break;
	}
	return len;
}



/************************************** AFN=11 **************************************/
INT16S dealAFN11_F1(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		INT8U i;
		data[0] = format3762->afn11_f1_down.Num;
		for(i=0;i<format3762->afn11_f1_down.Num;i++)
		{
			memcpy(&data[1+7*i], &format3762->afn11_f1_down.SlavePoint[i].Addr[0], 6);
			data[7+7*i] = format3762->afn11_f1_down.SlavePoint[i].Protocol;
		}
		return (1+7*data[0]);
	}
	else
	{
		return -1;
	}
	return -1;
}

INT16S dealAFN11_F2(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		INT8U i;
		data[0] = format3762->afn11_f2_down.Num;
		for(i=0;i<format3762->afn11_f2_down.Num;i++)
		{
			memcpy(&data[1+6*i], &format3762->afn11_f2_down.SlavePointAddr[i][0], 6);
		}
		return (1+6*data[0]);
	}
	else
	{
		return -1;
	}
	return -1;
}

INT16S dealAFN11_F3(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		INT8U i;
		memcpy(&data[0], &format3762->afn11_f3_down.SlavePointAddr[0], 6);
		data[6] = format3762->afn11_f3_down.RepeaterLevel;
		for(i=0;i<format3762->afn11_f3_down.RepeaterLevel;i++)
		{
			memcpy(&data[7+6*i], &format3762->afn11_f3_down.RepeaterSlavePointAddr[i][0], 6);
		}
		return (7+6*data[6]);
	}
	else
	{
		return -1;
	}
	return -1;
}

INT16S dealAFN11_F4(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		data[0] = (format3762->afn11_f4_down.ErrCode<<4) +
					(format3762->afn11_f4_down.RegisterFlag<<1) +
					format3762->afn11_f4_down.WorkFlag;
		data[1] = format3762->afn11_f4_down.Rate & 0xff;
		data[2] = (format3762->afn11_f4_down.RateUnit<<8) + ((format3762->afn11_f4_down.Rate>>8) & 0x7f);
		return 3;
	}
	else
	{
		return -1;
	}
	return -1;
}

INT16S dealAFN11_F5(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		data[0] = format3762->afn11_f5_down.StartTimeSecond;
		data[1] = format3762->afn11_f5_down.StartTimeMinute;
		data[2] = format3762->afn11_f5_down.StartTimeHour;
		data[3] = format3762->afn11_f5_down.StartTimeDay;
		data[4] = format3762->afn11_f5_down.StartTimeMonth;
		data[5] = format3762->afn11_f5_down.StartTimeYear;
		data[6] = format3762->afn11_f5_down.Duration & 0xff;
		data[7] = (format3762->afn11_f5_down.Duration>>8) & 0xff;
		data[8] = format3762->afn11_f5_down.RepeatCount;
		data[9] = format3762->afn11_f5_down.TimeSliceNum;
		return 10;
	}
	else
	{
		return -1;
	}
	return -1;
}

INT16S dealAFN11_F6(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return 0;
	}
	else
	{
		return -1;
	}
	return -1;
}

INT16S dealAFN11_F8(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		INT8U i;
		data[0] = format3762->afn11_f8_down.Num;
		for(i=0;i<format3762->afn11_f8_down.Num;i++)
		{
			memcpy(&data[1+6*i], &format3762->afn11_f8_down.SlavePointAddr[i][0], 6);
		}
		return (1+6*data[0]);
	}
	else
	{
		return -1;
	}
	return -1;
}

INT16S dealAFN11(FORMAT3762* format3762, INT8U dir, INT8U fn, INT8U* data)
{
	INT16S len;
	switch(fn)
	{
	case 1:
		len = dealAFN11_F1(format3762, dir, data);
		break;
	case 2:
		len = dealAFN11_F2(format3762, dir, data);
		break;
	case 3:
		len = dealAFN11_F3(format3762, dir, data);
		break;
	case 4:
		len = dealAFN11_F4(format3762, dir, data);
		break;
	case 5:
		len = dealAFN11_F5(format3762, dir, data);
		break;
	case 6:
		len = dealAFN11_F6(format3762, dir, data);
		break;
	case 8:
		len = dealAFN11_F8(format3762, dir, data);
		break;
	default:
		len = -1;
		break;
	}
	return len;
}



/************************************** AFN=12 **************************************/
INT16S dealAFN12_F1(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return 0;
	}
	else
	{
		return -1;
	}
	return -1;
}

INT16S dealAFN12_F2(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return 0;
	}
	else
	{
		return -1;
	}
	return -1;
}

INT16S dealAFN12_F3(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		return 0;
	}
	else
	{
		return -1;
	}
	return -1;
}

INT16S dealAFN12(FORMAT3762* format3762, INT8U dir, INT8U fn, INT8U* data)
{
	INT16S len;
	switch(fn)
	{
	case 1:
		len = dealAFN12_F1(format3762, dir, data);
		break;
	case 2:
		len = dealAFN12_F2(format3762, dir, data);
		break;
	case 3:
		len = dealAFN12_F3(format3762, dir, data);
		break;
	default:
		len = -1;
		break;
	}
	return len;
}



/************************************** AFN=13 **************************************/
INT16S dealAFN13_F1(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		INT8U i;
		data[0] = format3762->afn13_f1_down.Protocol;
		data[1] = format3762->afn13_f1_down.DelayFlag;
		data[2] = format3762->afn13_f1_down.SubPointNum;

		for(i=0;i<format3762->afn13_f1_down.SubPointNum;i++)
		{
			memcpy(&data[3+6*i], &format3762->afn13_f1_down.SubPointAddr[i][0], 6);
		}
		data[3+6*i] = format3762->afn13_f1_down.MsgLength;
		memcpy(&data[4+6*i], &format3762->afn13_f1_down.MsgContent[0], data[3+6*i]);

		return (4+6*data[2]+data[3+6*i]);
	}
	else
	{
		format3762->afn13_f1_up.TransTime = (data[1]<<8) + data[0];
		format3762->afn13_f1_up.Protocol = data[2];
		format3762->afn13_f1_up.MsgLength = data[3];
		memcpy(&format3762->afn13_f1_up.MsgContent[0], &data[4], data[3]);
		return (4+data[3]);
	}
	return -1;
}

INT16S dealAFN13(FORMAT3762* format3762, INT8U dir, INT8U fn, INT8U* data)
{
	INT16S len;
	switch(fn)
	{
	case 1:
		len = dealAFN13_F1(format3762, dir, data);
		break;
	default:
		len = -1;
	}
	return len;
}




/************************************** AFN=14 **************************************/
INT16S dealAFN14_F1(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		INT8U i, len;
		data[0] = format3762->afn14_f1_down.ReadFlag;
		data[1] = format3762->afn14_f1_down.DelayFlag;
		data[2] = len = format3762->afn14_f1_down.MsgLength;
		memcpy(&data[3], &format3762->afn14_f1_down.MsgContent[0], data[2]);
		data[3+len] = format3762->afn14_f1_down.SubPointNum;

		for(i=0; i<data[3+len]; i++)
		{
			memcpy(&data[4+len+6*i], &format3762->afn14_f1_down.SubPointAddr[i][0], 6);
		}
		return (4+data[2]+6*data[3+len]);
	}
	else
	{
		format3762->afn14_f1_up.Phase = data[0];
		memcpy(&format3762->afn14_f1_up.SlavePointAddr[0], &data[1], 6);
		format3762->afn14_f1_up.Index = (data[8]<<8) + data[7];
		return 9;
	}
	return -1;
}

INT16S dealAFN14_F2(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		int32u2bcd(format3762->afn14_f2_down.CurrentTimeSecond, &data[0], inverted);
		int32u2bcd(format3762->afn14_f2_down.CurrentTimeMinute, &data[1], inverted);
		int32u2bcd(format3762->afn14_f2_down.CurrentTimeHour, &data[2], inverted);
		int32u2bcd(format3762->afn14_f2_down.CurrentTimeDay, &data[3], inverted);
		int32u2bcd(format3762->afn14_f2_down.CurrentTimeMonth, &data[4], inverted);
		int32u2bcd(format3762->afn14_f2_down.CurrentTimeYear, &data[5], inverted);
		return 6;
	}
	else
	{
		return 0;
	}
	return -1;
}

INT16S dealAFN14_F3(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		data[0] = format3762->afn14_f3_down.MsgLength;
		memcpy(&data[1], &format3762->afn14_f3_down.MsgContent[0], data[0]);
		return (1+data[0]);
	}
	else
	{
		memcpy(format3762->afn14_f3_up.SlavePointAddr, &data[0], 6);
		format3762->afn14_f3_up.DelayTime = (data[7]<<8) + data[6];
		format3762->afn14_f3_up.MsgLength = data[8];
		memcpy(format3762->afn14_f3_up.MsgContent, &data[9], data[8]);
		return (9+data[8]);
	}
	return -1;
}

INT16S dealAFN14(FORMAT3762* format3762, INT8U dir, INT8U fn, INT8U* data)
{
	INT16S len;
	switch(fn)
	{
	case 1:
		len = dealAFN14_F1(format3762, dir, data);
		break;
	case 2:
		len = dealAFN14_F2(format3762, dir, data);
		break;
	case 3:
		len = dealAFN14_F3(format3762, dir, data);
		break;
	default:
		len = -1;
	}
	return len;
}



/************************************** AFN=15 **************************************/
INT16S dealAFN15_F1(FORMAT3762* format3762, INT8U dir, INT8U* data)
{
	if (dir == DOWN)
	{
		data[0] = format3762->afn15_f1_down.FileFlag;
		data[1] = format3762->afn15_f1_down.FileProperty;
		data[2] = format3762->afn15_f1_down.FileOrder;
		data[3] = format3762->afn15_f1_down.SectionCount & 0xff;
		data[4] = (format3762->afn15_f1_down.SectionCount>>8) & 0xff;
		data[5] = format3762->afn15_f1_down.SectionFlag & 0xff;
		data[6] = (format3762->afn15_f1_down.SectionFlag>>8) & 0xff;
		data[7] = (format3762->afn15_f1_down.SectionFlag>>16) & 0xff;
		data[8] = (format3762->afn15_f1_down.SectionFlag>>24) & 0xff;
		data[9] = format3762->afn15_f1_down.DataLength & 0xff;
		data[10] = (format3762->afn15_f1_down.DataLength>>8) & 0xff;
		memcpy(&data[11], &format3762->afn15_f1_down.Data[0], format3762->afn15_f1_down.DataLength);
		return (11+format3762->afn15_f1_down.DataLength);
	}
	else
	{
		format3762->afn15_f1_up.SectionFlag = (data[3]<<24) + (data[2]<<16) + (data[1]<<8) + data[0];
		return 4;
	}
	return -1;
}

INT16S dealAFN15(FORMAT3762* format3762, INT8U dir, INT8U fn, INT8U* data)
{
	INT16S len;
	switch(fn)
	{
	case 1:
		len = dealAFN15_F1(format3762, dir, data);
		break;
	default:
		len = -1;
		break;
	}
	return len;
}


//解析应用数据区
//data：数据单元头指针
INT16S analyzeData(FORMAT3762* format3762, INT8U dir, INT8U afn, INT8U fn, INT8U* data)
{
	INT16S len;
	switch(afn)
	{
	case 0x00:
		len = dealAFN00(format3762, dir, fn, data);
		break;
	case 0x01:
		len = dealAFN01(format3762, dir, fn, data);
		break;
	case 0x02:
		len = dealAFN02(format3762, dir, fn, data);
		break;
	case 0x03:
		len = dealAFN03(format3762, dir, fn, data);
		break;
	case 0x04:
		len = dealAFN04(format3762, dir, fn, data);
		break;
	case 0x05:
		len = dealAFN05(format3762, dir, fn, data);
		break;
	case 0x06:
		len = dealAFN06(format3762, dir, fn, data);
		break;
	case 0x10:
		len = dealAFN10(format3762, dir, fn, data);
		break;
	case 0x11:
		len = dealAFN11(format3762, dir, fn, data);
		break;
	case 0x12:
		len = dealAFN12(format3762, dir, fn, data);
		break;
	case 0x13:
		len = dealAFN13(format3762, dir, fn, data);
		break;
	case 0x14:
		len = dealAFN14(format3762, dir, fn, data);
		break;
	case 0x15:
		len = dealAFN15(format3762, dir, fn, data);
		break;
	case 0xF0://内部调试，不需要解析
		len = -2;
		break;
	default:
		len = -1;
		break;
	}
	return len;
}


//组合应用数据区
//data：数据单元头指针
INT16S composeData(FORMAT3762* format3762, INT8U dir, INT8U afn, INT8U fn, INT8U* data)
{
	INT16S len;
	switch(afn)
	{
	case 0x00:
		len = dealAFN00(format3762, dir, fn, data);
		break;
	case 0x01:
		len = dealAFN01(format3762, dir, fn, data);
		break;
	case 0x02:
		len = dealAFN02(format3762, dir, fn, data);
		break;
	case 0x03:
		len = dealAFN03(format3762, dir, fn, data);
		break;
	case 0x04:
		len = dealAFN04(format3762, dir, fn, data);
		break;
	case 0x05:
		len = dealAFN05(format3762, dir, fn, data);
		break;
	case 0x06:
		len = dealAFN06(format3762, dir, fn, data);
		break;
	case 0x10:
		len = dealAFN10(format3762, dir, fn, data);
		break;
	case 0x11:
		len = dealAFN11(format3762, dir, fn, data);
		break;
	case 0x12:
		len = dealAFN12(format3762, dir, fn, data);
		break;
	case 0x13:
		len = dealAFN13(format3762, dir, fn, data);
		break;
	case 0x14:
		len = dealAFN14(format3762, dir, fn, data);
		break;
	case 0x15:
		len = dealAFN15(format3762, dir, fn, data);
		break;
	default:
		len = -1;
		break;
	}
	return len;
}
