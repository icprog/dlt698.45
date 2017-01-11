/*
 * secure.c
 *
 *  Created on: 2017-1-11
 *      Author: gk
 */
#include <stdio.h>

#include "ParaDef.h"
#include "secure.h"
#include "StdDataType.h"
#include "../libEsam/esam.h"
/**********************************************************************
 *单位解析，解析安全传输中SID,MAC,RN
 *输入：type定义的数据类型（下发报文不含数据类型标示）0x01  SID  0x02 RN MAC
 *输出：source解析长度
 *限定：type=0x01时dest是SID类型结构体；
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
			memcpy(dest,source+1,source[0]);//RN随机数
		len=len+4+1+source[0];
	}
	else if(type == 0x02)//RN  OR MAC    octet-string类型
	{
		if(source[0]!=0)
			memcpy(dest,source+1,source[0]);
		len+=source[0];
	}
	else
		return -1;
	return len;
}
/**********************************************************************
 *安全传输中获取应用数据单元
 *输入：apdu完整的应用数据单元开头地址
 *输出：应用数据单元长度包括长度的1或2个字节
 **********************************************************************/
 INT16S secureGetAppDataUnit(INT8U* apdu)
 {
	 INT16U datalen=0;
		if(apdu[2]>7)//长度字节，如果小于8，肯定不正确（参照读取一个对象属性），后一个字节也是长度（8*256=2048）
		{
			datalen=apdu[2]+1;
		}
		else
		{
			datalen=(apdu[2]<<8)&0xff00+apdu[3]&0x00ff;
			if(datalen>BUFLEN) return -1;
			datalen+=2;
		}
		return datalen;
 }
 /**********************************************************************
  * 安全传输Esam校验
  *decryptData--明文信息 encryptData--密文数据集
  *输入：retData--esam验证后返回信息(需要在该函数外层开辟空间)
  *输出：retData长度
  **********************************************************************/
 INT16U secureEsamCheck(INT8U* apdu,INT16S appLen,INT8U* retData)
 {
	 INT8U appUnit=apdu[1];//明文或密文标识
	 INT16S len=0;
	if(apdu[2+appLen]==0x00)//SID_MAC数据验证码
	{
		SID_MAC sidmac;
		len = UnitParse(&apdu[2+appLen+1],sidmac,0x01);//解析SID部分
		if(len<=0) return -1;
		len = UnitParse(&apdu[2+appLen+1+len],sidmac.mac,0x02);//解析MAC部分
		if(len<=0) return -1;//
	}
	else if(apdu[2+appLen]==0x01)//RN随机数
	{

	}
	else if(apdu[2+appLen]==0x02)//RN_MAC随机数+数据MAC
	{

	}
	else if(apdu[2+appLen]==0x03)//安全标示SID
	{

	}
	else
		return -4;//错误的数据验证信息
 }

