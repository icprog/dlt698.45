#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include "cjt188.h"

#define LIB_CJT1888_VER 0x0001

INT8U	sendSerial=0;
/***************************************
 * CJ/T188 报文完整性解析
 ***************************************/
int cj188_PreProcess(INT8U* step,INT32U* rev_delay,INT32U delay_num,INT32U* rev_tail,INT32U* rev_head,INT8U *RevBuf,INT8U* dealbuf)
{
	int i=0;
	int length=0;

	switch(*step){
		case 0:
			while(*rev_head!=*rev_tail) {
//				fprintf(stderr,"RevBuf[%d]=%02x\n",*rev_tail,RevBuf[*rev_tail]);
				if (RevBuf[*rev_tail]== 0x68) {
					*step = 1;
					fprintf(stderr,"\nstep0 ok");
					break;
				}else {
					*rev_tail = (*rev_tail + 1)% CJ188_MAXSIZE;
				}
			}
			break;
		case 1:
			if (((*rev_head - *rev_tail +CJ188_MAXSIZE)%CJ188_MAXSIZE)>10)
			{
				length = RevBuf[(*rev_tail+ 10)%CJ188_MAXSIZE];
//				fprintf(stderr,"length=%02x\n",length);
				if (length == 0){
					fprintf(stderr,"\nstep1 error1");
					*rev_tail = (*rev_tail + 1)% CJ188_MAXSIZE;
					*step = 0;
				}
				if((*rev_head-*rev_tail+CJ188_MAXSIZE)%CJ188_MAXSIZE >= (length+12)){

					int testi = (*rev_tail + 10 + length + 2 + CJ188_MAXSIZE)% CJ188_MAXSIZE;
					fprintf(stderr,"\nstep1.1 ok  buf[%d]=%02x  buf[%d]=%02x  length=%d",*rev_tail,RevBuf[*rev_tail],testi,RevBuf[testi],length);
					if(RevBuf[(*rev_tail + 10 + length + 2 + CJ188_MAXSIZE)% CJ188_MAXSIZE]== 0x16)
					{
						fprintf(stderr,"\nstep1.2 ok");
						if((RevBuf[(*rev_tail + 9 + CJ188_MAXSIZE)% CJ188_MAXSIZE] & 0x80) == 0x80){		//Control控制码D7=1:由主站发出应答帧
							fprintf(stderr,"\nMBUS:R(%d)=",length+13);
							*rev_delay = 0;
							for(i=0;i<(length+13);i++)	{
								dealbuf[i] = RevBuf[*rev_tail];
								fprintf(stderr,"%02x ",dealbuf[i]);
								*rev_tail = (*rev_tail + 1) % CJ188_MAXSIZE;
							}
							*step = 0;
							return (length+13);
						}else {
							*rev_tail = (*rev_tail + 10 + length + 2 + CJ188_MAXSIZE)% CJ188_MAXSIZE;
							*step = 0;
							fprintf(stderr,"\nstep1 error2");
							break;
						}
					}
					else
					{
						if (*rev_delay < delay_num)	{
							(*rev_delay)++;
							usleep(10*1000);
							break;
						}else {
							*rev_delay = 0;
							*rev_tail = (*rev_tail+1)% CJ188_MAXSIZE;
							*step = 0;
							fprintf(stderr,"\nstep1 error3");
						}
					}
				}else {
					if (*rev_delay < delay_num)	{
						(*rev_delay)++;
						usleep(10*1000);
						break;
					}else {
						*rev_delay = 0;
						*rev_tail = (*rev_tail +1 )% CJ188_MAXSIZE;
						*step = 0;
						fprintf(stderr,"\nstep1 error4");
					}
				}
			}
			break;
		default :
			break;
	}
	return 0;
}

/***************************************
 * CJ/T188 报文内容解析
 ***************************************/
INT16S cj188_parse(cj188_Frame *cj188,INT8U* rcvbuff,INT32U rcvlen)
{
	INT16S ret = 0;

	ret=filter(rcvbuff,rcvlen);
	if(ret == 0) {
		cj188->MeterType = rcvbuff[1];
		memcpy(&cj188->Addr[0],&rcvbuff[2],7);
		cj188->Ctrl = rcvbuff[9];
		cj188->Length = rcvbuff[10]-3;
		memcpy(&cj188->DI[0],&rcvbuff[11],2);
		cj188->SER	= rcvbuff[13];
		if(cj188->Length > 0 ) {
			memcpy(&cj188->Data[0],&rcvbuff[14],cj188->Length);
			ret = cj188->Length;
		}
	}
	return ret;
}

/***************************************
 * CJ/T188 报文内容解析
 ***************************************/
INT16S cj188_parseData(INT8U type,INT8U di1,INT8U di0,INT8U *data)
{
	INT16S 	ret=0;
//	curr_Water_Gos 	*currData;

	switch(type) {
	case T_WATER_COOL:
	case T_WATER_HEAT:
	case T_WATER_MIDDLE:
	case T_GAS:
	case T_METER:
//		if(di1 == 0x90 && di0 == 0x1F)
//			cj188_WaterGos_CurrData(currData,data);
		break;
	case T_HOT_HEAT:
	case T_HOT_COOL:

		break;
	default:
		fprintf(stderr,"仪表类型不支持\n");
		ret = ERR_METERTYPE;
		break;
	}
	return ret;
}

/***************************************
 * CJ/T188 水\气表计量数据内容解析
 ***************************************/
INT16S cj188_WaterGos_CurrData(curr_Water_Gos *currData,INT8U *data)
{
	INT16S	index = 0;
	if(data == NULL || currData == NULL)
		return  ERR_RCVD_LOST;
	memcpy(currData->totalflow,&data[index],4);
	index = index + 4;
	currData->totalflow_unit = data[index];
	index = index + 1;
	memcpy(currData->dayflow,&data[index],4);
	index = index + 4;
	currData->dayflow_unit = data[index];
	index = index + 1;
	bcd2int32u(&data[index],2,positive,&currData->realtime.Year);
	index = index + 2;
	bcd2int32u(&data[index],1,positive,&currData->realtime.Month);
	index = index + 1;
	bcd2int32u(&data[index],1,positive,&currData->realtime.Day);
	index = index + 1;
	bcd2int32u(&data[index],1,positive,&currData->realtime.Hour);
	index = index + 1;
	bcd2int32u(&data[index],1,positive,&currData->realtime.Minute);
	index = index + 1;
	bcd2int32u(&data[index],1,positive,&currData->realtime.Second);
	index = index + 1;
	memcpy(currData->status,&data[index],2);
	index = index + 2;
	return index;
}

/***************************************
 * CJ/T188 热表计量数据内容解析
 ***************************************/
INT16S cj188_Hot_CurrData(curr_Hot *currData,INT8U *data)
{
	INT16S	index = 0;
	if(data == NULL || currData == NULL)
		return  ERR_RCVD_LOST;
	memcpy(currData->dayhot,&data[index],4);
	index = index + 4;
	currData->dayhot_unit = data[index];
	index = index + 1;
	memcpy(currData->currhot,&data[index],4);
	index = index + 4;
	currData->currhot_unit = data[index];
	index = index + 1;
	memcpy(currData->hotpower,&data[index],4);
	index = index + 4;
	currData->hotpower_unit = data[index];
	index = index + 1;
	memcpy(currData->flow,&data[index],4);
	index = index + 4;
	currData->flow_unit = data[index];
	index = index + 1;
	memcpy(currData->totalflow,&data[index],4);
	index = index + 4;
	currData->totalflow_unit = data[index];
	index = index + 1;
	memcpy(currData->servewatertemp,&data[index],3);
	index = index + 3;
	memcpy(currData->backwatertemp,&data[index],3);
	index = index + 3;
	memcpy(currData->worktime,&data[index],3);
	index = index + 3;
	bcd2int32u(&data[index],2,positive,&currData->realtime.Year);
	index = index + 2;
	bcd2int32u(&data[index],1,positive,&currData->realtime.Month);
	index = index + 1;
	bcd2int32u(&data[index],1,positive,&currData->realtime.Day);
	index = index + 1;
	bcd2int32u(&data[index],1,positive,&currData->realtime.Hour);
	index = index + 1;
	bcd2int32u(&data[index],1,positive,&currData->realtime.Minute);
	index = index + 1;
	bcd2int32u(&data[index],1,positive,&currData->realtime.Second);
	index = index + 1;
	memcpy(currData->status,&data[index],2);
	index = index + 2;
	return index;
}

/***************************************
 * CJ/T188 水\气表计量历史记录数据解析(上1月-上12月)
 ***************************************/
INT16S cj188_WaterGos_MonData(INT8U di1,INT8U di0,month_Water_Gos *monData,INT8U *data)
{
	INT16S	index = 0;
	INT8S	mon = 0;


	if(data == NULL || monData == NULL)
		return  ERR_RCVD_LOST;
	mon = getMonIndex(di1,di0);
	if(mon < 0 || mon > 11) return ERR_MONTH;

	memcpy(monData->HisData[mon].dayflow,&data[index],4);
	index = index + 4;
	monData->HisData[mon].dayflow_unit = data[index];
	index = index + 1;
	return index;
}

/***************************************
 * CJ/T188 水\气表计量历史记录数据解析(上1月-上12月)
 ***************************************/
INT16S cj188_Hot_MonData(INT8U di1,INT8U di0,month_Hot *monData,INT8U *data)
{
	INT16S	index = 0;
	INT8S	mon = 0;


	if(data == NULL || monData == NULL)
		return  ERR_RCVD_LOST;
	mon = getMonIndex(di1,di0);
	if(mon < 0 || mon > 11) return ERR_MONTH;

	memcpy(monData->HisData[mon].dayhot,&data[index],4);
	index = index + 4;
	monData->HisData[mon].dayhot_unit = data[index];
	index = index + 1;
	return index;
}

/***************************************
 * CJ/T188 读价格表
 ***************************************/
INT16S cj188_PriceTable(price_Table *price,INT8U *data)
{
	INT16S	index = 0;
	memcpy(price->price1,&data[index],3);
	index = index + 3;
	memcpy(price->amount1,&data[index],3);
	index = index + 3;
	memcpy(price->price2,&data[index],3);
	index = index + 3;
	memcpy(price->amount2,&data[index],3);
	index = index + 3;
	memcpy(price->price3,&data[index],3);
	index = index + 3;
	return index;
}

/***************************************
 * CJ/T188 读购入金额
 ***************************************/
INT16S cj188_BuyMoney(buy_Money *money,INT8U *data)
{
	INT16S	index = 0;

	money->serial = data[index];
	index = index + 1;
	memcpy(money->thismoney,&data[index],4);
	index = index + 4;
	memcpy(money->growmoney,&data[index],4);
	index = index + 4;
	memcpy(money->restmoney,&data[index],4);
	index = index + 4;
	memcpy(money->status,&data[index],2);
	index = index + 2;
	return index;
}

/********************************************************
 * CJ/T188 组织请求报文明码帧,返回组织的报文帧
 * 注意: 1.主站发送的序号SER,在每次通讯前,按照模256加1运算后产生
 * 		2.发送帧信息之前,应先发送2~4个字节FEH
 *******************************************************/
INT16S cj188_ComposeFrame(cj188_Frame cj188,cj188_Para cj188_para,INT8U* sendbuff)
{
	INT8U	datalen=0;

	sendbuff[0] = 0xFE;
	sendbuff[1] = 0xFE;
	sendbuff[2] = 0xFE;
	sendbuff[3] = 0xFE;



	sendbuff[4] = 0x68;
	sendbuff[5] = cj188.MeterType;
	memcpy(&sendbuff[6],cj188.Addr,7);		//地址
	sendbuff[13] = cj188.Ctrl;
	sendbuff[15] = cj188.DI[0];
	sendbuff[16] = cj188.DI[1];
	sendbuff[17] = cj188.SER; //sendSerial;
	datalen = dataFrame(cj188,cj188_para,&sendbuff[18]);
	sendbuff[14] = datalen+3;
	sendbuff[18+datalen] = checkSum(&sendbuff[4],14+datalen);
	sendbuff[19+datalen] = 0x16;
	return (datalen+16+4);




//	sendbuff[0] = 0x68;
//	sendbuff[1] = cj188.MeterType;
//	memcpy(&sendbuff[2],cj188.Addr,7);		//地址
//	sendbuff[9] = cj188.Ctrl;
//	sendbuff[11] = cj188.DI[0];
//	sendbuff[12] = cj188.DI[1];
//	sendbuff[13] = cj188.SER; //sendSerial;
//	datalen = dataFrame(cj188,cj188_para,&sendbuff[14]);
//	sendbuff[10] = datalen+3;
//	sendbuff[14+datalen] = checkSum(&sendbuff[0],14+datalen);
//	sendbuff[15+datalen] = 0x16;
//	return (datalen+16);
}

/********************************************************
 * CJ/T188 根据DI的值判断历史记录数据的月份,返回0-11月
 *******************************************************/
INT8S	getMonIndex(INT8U di1,INT8U	di0)
{
	INT8S	index=0;
	INT32U	di = 0;

	di = (di0 << 8) + di1;
	if(di >= 0xD120 && di <= 0xD12B) {
		index =  di - 0xD120;
	}else  index = ERR_MONTH;
	return index;
}

/********************************************************
 * CJ/T188 根据不同的Ctrl码,完成数据帧的报文组织
 *******************************************************/
INT16S dataFrame(cj188_Frame cj188,cj188_Para cj188_para,INT8U* databuf)
{
	INT8U	len = 0;
	INT16U	DI=0;
	DI = (cj188.DI[0] << 8) | cj188.DI[1];
	switch(cj188.Ctrl) {
	case READ_DATA:
	case READ_KEY_VER:
	case READ_ADDR:
		len = 0;
		break;
	case WRITE_DATA:
		switch(DI) {
		case 0xA010:	//写价格表
			len = compose_DI10(cj188_para.di10,databuf);
			break;
		case 0xA011:	//写结算日
			len = compose_DI11(cj188_para.di11,databuf);
			break;
		case 0xA012:	//写抄表日
			len = compose_DI12(cj188_para.di12,databuf);
			break;
		case 0xA013:	//写购入金额
			len = compose_DI13(cj188_para.di13,databuf);
			break;
		case 0xA014:	//写新密钥
			len = compose_DI14(cj188_para.di14,databuf);
			break;
		case 0xA015:	//写标准时间
			len = compose_DI15(cj188_para.di15,databuf);
			break;
		case 0xA017:	//写阀门控制
			len = compose_DI17(cj188_para.di17,databuf);
			break;
		case 0xA019:	//出厂重启
			len = 0;
			break;
		}
		break;
	case WRITE_ADDR:
		len = compose_DI18(cj188_para.di18,databuf);
		break;
	case WRITE_SYNC_DATA:
		len = compose_DI16(cj188_para.di16,databuf);
		break;
	}
	return len;
}

/***************************************
 * CJ/T188 主站写价格表报文组织
 * 返回值: 数据段长度
 ***************************************/
INT8U compose_DI10(DI_10 di10,INT8U *buf)
{
	INT8U	index=0;
	memcpy(&buf[index],&di10.Bcd_Price1[0],3);
	index = index+3;
	memcpy(&buf[index],&di10.Bcd_Amount1[0],3);
	index = index+3;
	memcpy(&buf[index],&di10.Bcd_Price2[0],3);
	index = index+3;
	memcpy(&buf[index],&di10.Bcd_Amount2[0],3);
	index = index+3;
	memcpy(&buf[index],&di10.Bcd_Price3[0],3);
	index = index+3;
	buf[index] = di10.Bcd_Date;
	index++;
	return index;
}

/***************************************
 * CJ/T188 主站写结算日
 * 返回值: 数据段长度
 ***************************************/
INT8U compose_DI11(DI_11 di11,INT8U *buf)
{
	INT8U	index=0;
	buf[index] = di11.Bcd_Date;
	index++;
	return index;
}

/***************************************
 * CJ/T188 主站写抄表日
 * 返回值: 数据段长度
 ***************************************/
INT8U compose_DI12(DI_12 di12,INT8U *buf)
{
	INT8U	index=0;
	buf[index] = di12.Bcd_Date;
	index++;
	return index;
}

/***************************************
 * CJ/T188 主站写购入金额
 * 返回值: 数据段长度
 ***************************************/
INT8U compose_DI13(DI_13 di13,INT8U *buf)
{
	INT8U	index=0;
	buf[index] = di13.Hex_Serial;
	index++;
	memcpy(&buf[index],di13.Bcd_Price,4);
	index = index+4;
	return index;
}

/***************************************
 * CJ/T188 主站写新密钥
 * 返回值: 数据段长度
 ***************************************/
INT8U compose_DI14(DI_14 di14,INT8U *buf)
{
	INT8U	index=0;
	buf[index] = di14.Hex_Key_Ver;
	index++;
	memcpy(&buf[index],di14.Bcd_Key,8);
	index = index+8;
	return index;
}

/***************************************
 * CJ/T188 主站写标准时间
 * 返回值: 数据段长度
 ***************************************/
INT8U compose_DI15(DI_15 di15,INT8U *buf)
{
	INT8U	index=0;
	memcpy(&buf[index],di15.Bcd_Date,7);
	index = index+7;
	return index;
}

/***************************************
 * CJ/T188 主站写阀门控制
 * 返回值: 数据段长度
 ***************************************/
INT8U compose_DI17(DI_17 di17,INT8U *buf)
{
	INT8U	index=0;
	memcpy(&buf[index],di17.Hex_Status,2);
	index = index+2;
	return index;
}

/***************************************
 * CJ/T188 主站写地址
 * 调试阶段明码传输,收到出厂启动命令后不再响应
 * 返回值: 数据段长度
 ***************************************/
INT8U compose_DI18(DI_18 di18,INT8U *buf)
{
	INT8U	index=0;
	memcpy(&buf[index],di18.Bcd_Addr,7);
	index = index+7;
	return index;
}

/***************************************
 * CJ/T188 主站写机电同步数据,
 * 调试阶段明码传输,收到出厂启动命令后不再响应
 * 返回值: 数据段长度
 ***************************************/
INT8U compose_DI16(DI_16 di16,INT8U *buf)
{
	INT8U	index=0;
	memcpy(&buf[index],di16.Bcd_TotalFlow,5);
	index = index+5;
	return index;
}
/***************************************
 * CJ/T188 报文校验\帧完整过滤
 ***************************************/
INT16S filter(INT8U* rcvbuff,INT16U rcvlen)
{
	INT8U 	checkcode = 0;
	INT8U	length = 0;
	if(rcvlen >=13)	{
		if(rcvbuff[0] == 0x68)	{
			length = rcvbuff[10]+13;
			if(rcvlen >= length) {
				if(rcvbuff[length-1] == 0x16) {
					checkcode = checkSum(&rcvbuff[0],length-2);
//					fprintf(stderr,"\ncheck=%02x,revbuff[%d]=%02x\n",checkcode,length-2,rcvbuff[length-2]);
					if(checkcode == rcvbuff[length-2])
						return 0;
					else  return ERR_SUM_ERROR;   // 校验失败
				}else return ERR_LOST_0x16;
			}else return ERR_RCVD_LOST;
		}else return ERR_LOST_0x68;
	}else	return ERR_RCVD_LOST;
}


/***************************************
 * CJ/T188 帧校验和
 ***************************************/
INT8U checkSum(INT8U* buff,INT32U len)
{
	INT32U i=0;
	INT8U ret = 0;
	if(buff == NULL)
		return -1;
	if(len == 0)
		return -2;
	for (i = 0; i < len; i++) {
		ret = ret + buff[i];
	}
	return ret;
}

