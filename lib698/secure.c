/*
 * secure.c
 *
 *  Created on: 2017-1-11
 *      Author: gk
 */
#include <stdio.h>
#include <string.h>

#include "ParaDef.h"
#include "secure.h"
#include "../libEsam/Esam.h"
/**********************************************************************
 *安全传输中获取应用数据单元
 *输入：apdu完整的应用数据单元开头地址
 *输出：应用数据单元长度包括长度的1或2个字节
 **********************************************************************/
 INT16S GetDataLength(INT8U* Data)
 {
	 INT16U datalen=0;
		if(Data[0]>7)//长度字节，如果小于8，肯定不正确（参照读取一个对象属性），后一个字节也是长度（8*256=2048）
		{
			datalen=Data[0]+1;
		}
		else
		{
			datalen=((Data[0]<<8)&0xff00)+(Data[1]&0x00ff);
			if(datalen>BUFLEN) return -1;
			datalen+=2;
		}
		return datalen;
 }
 /**********************************************************************
  *建立应用链接（）
  *输入：SignatureSecurity为主站下发解析得到；SecurityData为上行报文结构数据，需填充
  *输出：返回正值正确，否则失败
  **********************************************************************/
INT32S secureConnectRequest(SignatureSecurity* securityInfo ,SecurityData* RetInfo)
{
	 INT32S fd=-1;
	 fd = Esam_Init(fd,(INT8U*)DEV_SPI_PATH);
     if(fd<0) return -3;
     INT32S ret= Esam_CreateConnect( fd,  securityInfo , RetInfo);
     Esam_Clear(fd);
     return ret;
}
/**********************************************************************
*处理主动上报，主站回复报文同esam交互部分
 **********************************************************************/
INT32S secureResponseData(INT8U* RN,INT8U* apdu,INT8U* retData)
{
	 INT32S fd=-1;
	 INT32S ret=0;
	 INT16S MACindex=0;//MAC所在位置可能因为应用数据单元长度不定而变化
	 fd = Esam_Init(fd,(INT8U*)DEV_SPI_PATH);
    if(fd<0) return -3;
    INT16S len = GetDataLength(apdu[2]);

    if(len>255)
    	MACindex=2+2+len;
    else if(len>0 && len<256)
    	MACindex=2+1+len;
    else return -1;

    ret =Esam_DencryptReport( fd,  RN, &apdu[MACindex], &apdu[2],  retData);
     Esam_Clear(fd);
     return ret;
}
/**********************************************************************
 *单位解析，解析安全传输中SID,MAC,RN
 *输入：type定义的数据类型（下发报文不含数据类型标示）0x01  SID  0x02 RN MAC
 *输出：source解析长度
 *说明：type=0x01时dest是SID类型结构体；RN/MAC/SID中随机数，开头一个字节都是长度
 **********************************************************************/
INT8S UnitParse(INT8U* source,INT8U* dest,INT8U type)
{
	INT8S len=0;
	if(type==0x01)//SID
	{
		memcpy(dest,source,4);//标识double-long-unsigned
		dest+=4;
		source+=4;
		if(source[0]!=0)
			memcpy(dest,source,source[0]+1);//RN随机数（包含一个字节长度）
		len=len+4+1+source[0];
	}
	else if(type == 0x02)//RN  OR MAC    octet-string类型（包含一个字节长度）
	{
		if(source[0]!=0)
			memcpy(dest,source,source[0]+1);
		len+=source[0]+1;
	}
	else
		return -1;
	return len;
}

 /**********************************************************************
  *应用数据单元为密文情况时处理方案（ 698解析应用数据单元和数据验证信息.访问模型参见4.1.3.1）
  *当前理解：密文+SID为  《密文》等级   密文+SID_MAC为《密文+MAC》等级，密文情况下不存在RN/RN_MAC情况
  *因为密文+RN在安全芯片手册中找不到解密方法
  *输入：需返回的secureType安全类别，01明文，02明文+MAC 03密文  04密文+MAC
  *输出：retData长度
  **********************************************************************/
 INT32S secureEncryptDataDeal(INT32S fd,INT8U* secureType,INT8U* apdu,INT8U* retData)
 {
	 INT8U appUnit=apdu[1];//明文或密文标识
	 INT16S tmplen=0;
	 INT16S appLen=0;
	 INT32S ret;
	 appLen = GetDataLength(&apdu[2]);
	if(apdu[2+appLen]==0x00 || apdu[2+appLen]==0x03)//SID_MAC数据验证码
	{
		if(appUnit == 0x00 && apdu[2+appLen]==0x03) return 100;//明文+SID应用场景不存在，查看报文
		SID_MAC sidmac;
		tmplen = UnitParse(&apdu[2+appLen+1],(INT8U*)&sidmac,0x01);//解析SID部分
		if(tmplen<=0) return -101;
		if(appUnit == 0x01)
			tmplen = UnitParse(&apdu[2+appLen+1+tmplen],sidmac.mac,0x02);//解析MAC部分
		if(tmplen<=0) return -102;//
		ret = Esam_SIDTerminalCheck(fd,sidmac,&apdu[2],retData);
	}
	else if(apdu[2+appLen]==0x01)//RN随机数
	{
		INT8U RN[128];
		tmplen=UnitParse(&apdu[2+appLen+1],RN,0x02);
		if(tmplen<=0) return -103;
	}
	else if(apdu[2+appLen]==0x02)//RN_MAC随机数+数据MAC
	{
		INT8U RN[128];
		INT8U MAC[10];
		tmplen=UnitParse(&apdu[2+appLen+1],RN,0x02);//RN
		if(tmplen<=0) return -104;
		tmplen=UnitParse(&apdu[2+appLen+1+tmplen],MAC,0x02);//mac
		if(tmplen<=0) return -105;
	}
	else
		return -107;//错误的数据验证信息
	return ret;
 }
 /**********************************************************************
  *应用数据单元为明文情况时处理方案
  *当前理解：当前资料主要以明文+RN为主，为后续，兼容明文+RN_MAC情况。
  *输入：不用返回安全类别，此处类别总是02明文+MAC
  *输出：retData长度
  **********************************************************************/
 INT32S secureDecryptDataDeal(INT32S fd,INT8U* apdu,INT8U* retData)
 {
	 return 1;
 }
