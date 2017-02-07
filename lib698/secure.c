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
 *安全传输中获取应用数据单元长度
 *参见A-XDR编码规则(第一个字节最高bit位代表是否可变长度，0代表本身即长度字节，不超128,1代表剩余bit是长度域字节数，先取长度域再取长度)
 *输入：apdu完整的应用数据单元开头地址
 *输出：应用数据单元长度包括长度的1或2个字节或3个字节
 **********************************************************************/
 INT16U GetDataLength(INT8U* Data)
 {
	 INT16U datalen=0;
		if((Data[0] &0x80) != 0x80)//最高位代表长度域的属性
		{
			datalen=Data[0]+1;
		}
		else
		{
			if((Data[0] &0x7F) == 0x01)//长度域1个字节长度
				datalen=Data[1]+2;
			else if((Data[0] &0x7F) == 0x02)//长度域2个字节长度
				datalen=((INT16U)Data[1]<<8)+(INT16U)Data[2] +3 ;//
			else datalen=0;//长度域不会超过2个字节，超过的话此处做异常处理
		}
		if(datalen > BUFLEN) datalen=0;//超过上行最大长度，做异常处理
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
INT32S secureResponseData(INT8U* RN,INT8U* apdu)
{
	 INT32S fd=-1;
	 INT32S ret=0;
	 INT16S MACindex=0;//MAC所在位置可能因为应用数据单元长度不定而变化
	 fd = Esam_Init(fd,(INT8U*)DEV_SPI_PATH);
    if(fd<0) return -3;
    INT16S len = GetDataLength(&apdu[2]);

    if(len>255)
    	MACindex=2+2+len;
    else if(len>0 && len<256)
    	MACindex=2+1+len;
    else return -1;

    ret =Esam_DencryptReport( fd,  RN, &apdu[MACindex], &apdu[2], apdu);
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
	 INT16U tmplen=0;
	 INT16U appLen=0;
	 INT32S ret=0;
	 SID_MAC sidmac;
	 appLen = GetDataLength(&apdu[2]);
	 if(appLen<=0) return -100;

	if(apdu[2+appLen]==0x00 ||apdu[2+appLen]==0x03)//SID_MAC数据验证码
	{
		tmplen = UnitParse(&apdu[2+appLen+1],(INT8U*)&sidmac,0x01);//解析SID部分
		if(tmplen<=0) return -101;
		if(apdu[2+appLen]==0x00)
		{
			tmplen = UnitParse(&apdu[2+appLen+1+tmplen],sidmac.mac,0x02);//解析MAC部分
			if(tmplen<=0) return -102;//
		}
		ret = Esam_SIDTerminalCheck(fd,sidmac,&apdu[2],retData);
	}
	else
		return -101;
	if(apdu[2+appLen]==0x00)
		*secureType=0x04;//密文+mac等级
	if(apdu[2+appLen]==0x03)
		*secureType=0x03;//密文等级
	return ret;
 }
 /**********************************************************************
  *应用数据单元为明文情况时处理方案
  *当前理解：当前资料主要以明文+RN为主，为后续，兼容明文+RN_MAC情况。
  *输入：返回安全类别，此处类别总是02明文+MAC
  *mac值传入上层函数，暂时用不到
  *输出：retData长度
  **********************************************************************/
 INT32S secureDecryptDataDeal(INT32S fd,INT8U* apdu,INT8U* secureType,INT8U* MAC)
 {
	 INT32S ret=0;
	 INT16U appLen = GetDataLength(&apdu[2]);//计算应用数据单元长度
	 if(appLen<=0) return -100;

	 if(apdu[2+appLen]==0x01 || apdu[2+appLen]==0x02)// 只处理RN/RN_MAC情况
	 {
			 ret =  Esam_GetTerminalInfo(fd,&apdu[2+appLen+1],&apdu[2],MAC);//最后+1是数据验证信息标识
	 }
	 else
		 return -101;
		 *secureType=0x02;//明文+MAC等级
	 return ret;
 }
