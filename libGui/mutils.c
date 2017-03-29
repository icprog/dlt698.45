/*
 * mutils.c
 *
 *  Created on: 2017-3-3
 *      Author: prayer
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "mutils.h"
void write_apn(char* apn) {
    FILE* fp;
    fp = fopen("/etc/ppp/gprs-connect-chat", "w");
    if (fp == NULL) {
        return;
    }
    fprintf(fp, "TIMEOUT        5\n");
    fprintf(fp, "ABORT        \'\\nBUSY\\r\'\n");
    fprintf(fp, "ABORT        \'\\nNO ANSWER\\r\'\n");
    fprintf(fp, "ABORT        \'\\nRINGING\\r\\n\\r\\nRINGING\\r\'\n");
    fprintf(fp, "ABORT        \'\\nNO CARRIER\\r\'\n");
    fprintf(fp, "\'\' \\rAT\n");
    fprintf(fp, "TIMEOUT        5\n");
    fprintf(fp, "\'OK-+++\\c-OK\'    ATH\n");
    fprintf(fp, "TIMEOUT        20\n");
    fprintf(fp, "OK        AT+CREG?\n");
    fprintf(fp, "TIMEOUT        10\n");
    fprintf(fp, "OK        AT+CGATT=1\n");
    fprintf(fp, "TIMEOUT        300\n");
    fprintf(fp, "OK        AT+CGATT?\n");
    fprintf(fp, "OK        AT+CFUN=1\n");
    fprintf(fp, "TIMEOUT        5\n");
    fprintf(fp, "OK        AT+CPIN?\n");
    fprintf(fp, "TIMEOUT        5\n");
    fprintf(fp, "OK        AT+CSQ\n");
    fprintf(fp, "TIMEOUT        5\n");
    fprintf(fp, "OK        AT+CGDCONT=1,\"IP\",\"%s\"\n", apn);
    fprintf(fp, "OK        ATDT*99***1#\n");
    fprintf(fp, "CONNECT \'\'\n");
    fclose(fp);
    fp = NULL;
}

void write_userpwd(unsigned char* user, unsigned char* pwd, unsigned char* apn) {
    FILE* fp = NULL;
    fp       = fopen("/etc/ppp/chap-secrets", "w");
    fprintf(fp, "\"%s\" * \"%s\" *", user, pwd);
    fclose(fp);

    fp = fopen("/etc/ppp/pap-secrets", "w");
    fprintf(fp, "\"%s\" * \"%s\" *", user, pwd);
    fclose(fp);

    fp = fopen("/etc/ppp/peers/cdma2000", "w");
    fprintf(fp, "/dev/mux0\n");
    fprintf(fp, "115200\n");
    fprintf(fp, "modem\n");
    fprintf(fp, "debug\n");
    fprintf(fp, "nodetach\n");
    fprintf(fp, "usepeerdns\n");
    fprintf(fp, "noipdefault\n");
    fprintf(fp, "defaultroute\n");
    fprintf(fp, "user \"%s\"\n", user);
    fprintf(fp, "0.0.0.0:0.0.0.0\n");
    fprintf(fp, "ipcp-accept-local\n");
    fprintf(fp, "connect 'chat -s -v -f /etc/ppp/cdma2000-connect-chat'\n");
    fprintf(fp, "disconnect \"chat -s -v -f /etc/ppp/gprs-disconnect-chat\"\n");
    fclose(fp);

    fp = fopen("/etc/ppp/cdma2000-connect-chat", "w");
    fprintf(fp, "TIMEOUT        5\n");
    fprintf(fp, "ABORT        \"DELAYED\"\n");
    fprintf(fp, "ABORT        \"BUSY\"\n");
    fprintf(fp, "ABORT        \"ERROR\"\n");
    fprintf(fp, "ABORT        \"NO DIALTONE\"\n");
    fprintf(fp, "ABORT        \"NO CARRIER\"\n");
    fprintf(fp, "'' AT\n");
    fprintf(fp, "TIMEOUT        5\n");
    fprintf(fp, "'OK-+++\c-OK' ATZ\n");
    fprintf(fp, "TIMEOUT        20\n");
    fprintf(fp, "OK        AT+CREG?\n");
    fprintf(fp, "TIMEOUT        10\n");
    fprintf(fp, "OK        AT$MYNETCON=0,\"USERPWD\",\"%s,%s\"\n", user, pwd);
    fprintf(fp, "TIMEOUT        10\n");
    fprintf(fp, "OK        AT$MYNETCON=0,\"APN\",\"%s\"\n", apn);
    fprintf(fp, "TIMEOUT        10\n");
    fprintf(fp, "OK        AT$MYNETCON=0,\"AUTH\",1\n");
    fprintf(fp, "TIMEOUT        10\n");
    fprintf(fp, "OK ATDT#777\n");
    fprintf(fp, "CONNECT ''\n");
    fclose(fp);
}

/*时间组合TmS结构
 * */
void tmass(TS* ts,INT16U year,INT8U mon,INT8U day,INT8U hour,INT8U min,INT8U sec)
{
	if(year < 1900)
		ts->Year = year +2000;
	else
		ts->Year = year;
	ts->Month = mon;
	ts->Day = day;
	ts->Hour = hour;
	ts->Minute = min;
	ts->Sec = sec;
}

INT8S bcd2str(INT8U* bcd,INT8U* str,INT8U bcd_len,INT8U str_size,ORDER order)
{
	INT8U i,j,len;
	if(order == positive)
	{
		for(i = 0,j=0;i<bcd_len; i++)
		{
			if(j >= str_size)
			{
				break;
			}
			sprintf((char*)&str[j],"%02x",bcd[i]);
			j+=2;
		}
	}
	else if(order == inverted)
	{
		len = (bcd_len>str_size)?(str_size - 1):(bcd_len - 1);
		for(i = len,j = 0; i >= 0; i--)
		{
			sprintf((char*)&str[i],"%02x",bcd[j]);
			j+=2;
		}
	}
	return 1;
}

INT8S str2bcd(INT8U* str,INT8U* bcd,INT8U bcd_len)
{
	INT8U i,j;
	INT8U len = 0;
	INT8U* s_point = NULL;
	INT8U odd_flag = 0;
	len = strlen((char*)str);
	s_point = malloc(len);
	if(s_point == NULL)
	{
		return -1;
	}
	memset(s_point,0,len);

	for(i = 0;i<len;i++)
	{
		if(str[i]<='9' && str[i]>='0')
		{
			s_point[i] = str[i] - '0';
		}
		else if(str[i]<='f' && str[i]>='a')
		{
			s_point[i] = str[i] - 'W';
		}
		else if(str[i]<='F' && str[i] >= 'A')
		{
			s_point[i] = str[i] - '7';
		}
	}
//	len = i - 1;//实际转换的长度
	odd_flag = (len%2)?1:0;
	if(bcd_len < len/2)
	{
		if(s_point!=NULL)
		{
			free(s_point);
		}
		return -1;
	}
	else if(odd_flag)
	{
		if(bcd_len == len/2)
		{
			if(s_point!=NULL)
			{
				free(s_point);
			}
			return -2;
		}
	}
	for(i = 0,j=0;j<len/2;j++)
	{
		bcd[j] = s_point[i] << 4 | s_point[i+1];
		i+=2;
	}
	if(1 == odd_flag)
	{
		bcd[len/2] = s_point[len - 1]<<4;
	}
	if(s_point!=NULL)
	{
		free(s_point);
	}
	return 1;
}

//反转buff
INT8S reversebuff(INT8U* buff,INT32U len,INT8U* invbuff)
{
	if(buff==NULL)
		return -1;
	if(len == 0)
		return -2;
	if(invbuff == NULL)
		return -3;
	INT8U* buftmp =(INT8U*)malloc(len);
	memcpy(buftmp,buff,len);
	INT32U i=0;
	for(i=0; i < len; i++)
	{
		invbuff[i] = buftmp[len-i-1];
	}
	free(buftmp);
	buftmp = NULL;
	return 0;
}

INT32S asc2bcd(INT8U* asc, INT32U len, INT8U* bcd,ORDER order) {
	INT32U i,  k;
	if(asc == NULL)
		return -1;
	if(len == 0)
		return -2;
	if(len %2 != 0 )
		return -3;
	if(order != positive && order != inverted)
			return -4;
	INT8U* ascb = (INT8U*)malloc(len);
	if(ascb == NULL)
		return -5;
	memcpy(ascb,asc,len);
	for (i = 0; i < len; i++) {
		if (ascb[i] <= '9' && ascb[i] >= '0')
			ascb[i] = ascb[i] - '0';
		else if(ascb[i] <= 'f' && ascb[i] >= 'a')
		{
			ascb[i] = ascb[i] - 'W';
		}
		else if(ascb[i] <= 'F' && ascb[i] >= 'A')
		{
			ascb[i] = ascb[i] - '7';
		}
	}
	for (i = 0, k = 0; i < len / 2; i++) {
		bcd[i] = (ascb[k] << 4) | ascb[k + 1];
		k++;
		k++;
	}
	if(order == inverted)
		reversebuff(bcd,len/2,bcd);
	if(ascb != NULL)
	{
		free(ascb);
		ascb = NULL;
	}
	return len/2;
}

INT8S bcd2int32u(INT8U *bcd, INT8U len,ORDER order,INT32U* dint)
{
	INT32S i;
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

FP64 bcd2double(INT8U *bcd, INT8U len,INT16U decbytes,ORDER order)
{
	if(bcd == NULL || len <=0)
		return -1;
	INT32S dint=0; //整数部分
	INT32S ddec=0; //小数部分
	FP64 ret = 0;
	if(decbytes > 0)
	{
		if(bcd[0] == 0xff) //负数
		{
			bcd2int32u(&bcd[1],len-decbytes-1,order,(INT32U*)&dint);
			bcd2int32u(&bcd[len-decbytes],decbytes,order,(INT32U*)&ddec);
			ret += (~dint+1);
			ret += (~ddec+1)/pow(10,decbytes*2);
		}
		else
		{
			bcd2int32u(bcd,len-decbytes,order,(INT32U*)&dint);
			bcd2int32u(&bcd[len-decbytes],decbytes,order,(INT32U*)&ddec);
			ret = ddec;
			ret = dint+ ret/pow(10,decbytes*2);
		}
	}
	else {
		if(bcd[0] == 0xff)
		{
			bcd2int32u(&bcd[1],len-1,positive,(INT32U*)&dint);
			dint=(~dint+1);
		}
		else
			bcd2int32u(bcd,len,positive,(INT32U*)&dint);
		ret = dint;
	}
	return ret;
}

INT32S bcd2asc(INT8U* bcd, INT32U len, INT8U* asc,ORDER order) {
	INT32U j, k;
	if(bcd == NULL)
		return -1;
	if(len == 0)
		return -2;
	if(order != positive && order != inverted)
			return -3;
	INT8U* bcd_inv=  (INT8U*)malloc(len);
	memcpy(bcd_inv,bcd,len);
	if(order == inverted)
	{
		reversebuff(bcd,len,bcd_inv);
	}
	memset(asc, 0, len*2);
	for (j = 0, k = 0; j < len; j++) {
		if (bcd != NULL) {
			INT8S A = bcd_inv[j] >> 4;
			INT8S B = bcd_inv[j] & 0x0F;
			if (A < 10)
				asc[k] = A+'0';
			else
				asc[k] = A+ '7';
			if (B < 10)
				asc[k+1] = B + '0';
			else
				asc[k+1] = B +'7';
			k++;
			k++;
		}
	}
	if(bcd_inv != NULL)
	{
		free(bcd_inv);
		bcd_inv = NULL;
	}
	return len*2;
}

INT16S getDayFilePath(INT16U MpNo,INT16U year,INT8U month,INT8U day,INT8U* fpath,INT16U path_len)
{
	INT8U is4IN1mp = 0;
	INT8U path[256];
	INT16S len = 0;
	INT16U groupno=0;
	if(path == NULL)
		return -1;
	if(year==0 || month == 0 || day==0)
		return -2;
	memset(path,0,256);

	if(strstr((char*)fpath,"4IN1") !=0)
	{
		fprintf(stderr,"\n\ngetDayFilePath is 4IN1 file\n\n");

		is4IN1mp = 1;
	}
	groupno = MpNo /MAXMPNUM_DATAFILE;
	len = strlen((const char*)path);
	if(path_len < len)
		return -3;
	memset(fpath,0,path_len);
	memcpy(fpath,path,len);
	return len;
}
