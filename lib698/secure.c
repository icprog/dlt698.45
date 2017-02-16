/*
 * secure.c
 *
 *  Created on: 2017-1-11
 *      Author: gk
 */
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "ParaDef.h"
#include "secure.h"
#include "../libEsam/Esam.h"
/**********************************************************************
 *安全传输中获取应用数据单元长度
 *参见A-XDR编码规则(第一个字节最高bit位代表是否可变长度，0代表本身即长度字节，不超128,1代表剩余bit是长度域字节数，先取长度域再取长度)
 *输入：apdu完整的应用数据单元开头地址
 *输出：应用数据单元长度包括长度的1或2个字节或3个字节
 **********************************************************************/
extern INT8U securetype;//dlt698.c定义
extern INT8U secureRN[20];
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
 //已测/但测试报文无法通过校验，应该是esam芯片内证书和报文证书不匹配
INT32S secureConnectRequest(SignatureSecurity* securityInfo ,SecurityData* RetInfo)
{
	 INT32S fd=-1;
	 fd = Esam_Init(fd,(INT8U*)ACS_SPI_DEV);
     if(fd<0) return -1;
     fprintf(stderr,"secureConnectRequest  securityInfo= %d   =%d  \n",securityInfo->encrypted_code2[0],securityInfo->signature[0]);
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
	 fd = Esam_Init(fd,(INT8U*)ACS_SPI_DEV);
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
INT32S UnitParse(INT8U* source,INT8U* dest,INT8U type)
{
	INT32S len=0;
	if(type==0x01)//SID
	{
		memcpy(dest,source,4);//标识double-long-unsigned
		dest+=4;
		source+=4;
		if(source[0]!=0)
			memcpy(dest,source,source[0]+1);//附加信息（包含一个字节长度）
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
 INT32S secureEncryptDataDeal(INT32S fd,INT8U* apdu,INT8U* retData)
 {
	 INT32S tmplen=0;
	 INT16U appLen=0;
	 INT32S ret=0;
	 SID_MAC sidmac;
	 appLen = GetDataLength(&apdu[2]);
	 if(appLen<=0) return -100;
	// fprintf(stderr,"applen = %d \n",appLen);
	if(apdu[2+appLen]==0x00 ||apdu[2+appLen]==0x03)//SID_MAC数据验证码
	{
		//fprintf(stderr,"sid_mac first byte = %02x,%02x\n",(INT8U)apdu[2+appLen],(INT8U)apdu[2+apdu+1]);
		tmplen = UnitParse(&apdu[2+appLen+1],(INT8U*)&sidmac,0x01);//解析SID部分
		if(tmplen<=0) return -101;
		if(apdu[2+appLen]==0x00)
		{
			tmplen = UnitParse(&apdu[2+appLen+1+tmplen],sidmac.mac,0x02);//解析MAC部分
			if(tmplen<=0) return -102;//
		}
		ret = Esam_SIDTerminalCheck(fd,sidmac,&apdu[2],retData);
		//fprintf(stderr,"secureEncryptDataDeal ret = %d",ret);
	}
	else
		return -101;
	if(apdu[2+appLen]==0x00)
		securetype=0x04;//密文+mac等级
	if(apdu[2+appLen]==0x03)
		securetype=0x03;//密文等级
	return ret;
 }
 /**********************************************************************
  *应用数据单元为明文情况时处理方案
  *当前理解：当前资料主要以明文+RN为主，为后续，兼容明文+RN_MAC情况。
  *输入：返回安全类别，此处类别总是02明文+MAC
  *mac值传入上层函数，暂时用不到
  *输出：retData长度
  **********************************************************************/
 INT32S secureDecryptDataDeal(INT8U* apdu)
 {
	 INT32S ret=0;
	 INT16U appLen = GetDataLength(&apdu[2]);//计算应用数据单元长度
	 if(appLen<=0) return -100;
	 fprintf(stderr,"secureDecryptDataDeal appLen = %d\n",appLen);
	 securetype=0x02;//明文+MAC等级
	 if(apdu[2+appLen]==0x01 || apdu[2+appLen]==0x02)// 只处理RN/RN_MAC情况
	 {
		 if(apdu[2+appLen+1]<=sizeof(secureRN))//随机数字符数小于secureRN大小(原则是16byte)
		 {
			 memcpy(secureRN,&apdu[2+appLen+2],apdu[2+appLen+1]);
			 ret=appLen;
		 }
			 //ret =  Esam_GetTerminalInfo(fd,&apdu[2+appLen+1],&apdu[2],MAC);//最后+1是数据验证信息标识
	 }
	 else if(apdu[2+appLen]==0x00 || apdu[2+appLen]==0x03)
	 {
		 ret=appLen;
	 }
		 return -101;
	 return ret;
 }
 //获取ESAM主站/终端证书   证书都是大于1000字节，此处按照大于1000,2个字节组织上送报文
 	//ccieFlag证书标识  0x0C主站证书   0x0A终端证书
 	INT16U getEsamCcie(INT8U ccieFlag,INT8U *retBuff)//&&已测
 	{
 		INT32S retLen=0;
 		INT32S fd=-1;
 		fd = Esam_Init(fd,(INT8U*)ACS_SPI_DEV);
 		//fprintf(stderr,"function getesamccie fd = %d\n",fd);
 		if(fd<0) return 0;
 		INT8U buff[2048];
 		//fprintf(stderr,"ccieFlag = %d\n",ccieFlag);
 		if(ccieFlag==0x0C)	//主站证书
 			retLen = Esam_GetTermiSingleInfo(fd,0x0C,buff);
 		else                        //终端证书
 			retLen=Esam_GetTermiSingleInfo(fd,0x0B,buff);//此处芯片手册和属性编号不一致
 		//fprintf(stderr,"function getesamccie retlen = %d\n",retLen);
 		if(retLen>10)   //10值为随意值，正常证书长度1500左右(正常状态下)
 		{
 			retBuff[0] = 0x09;//octet-string类型
 			retBuff[1] = 0x82;//可变长度，2个字节
 			retBuff[2] = (INT8U)((retLen>>8)&0x00ff);//长度字节高字节
 			retBuff[3] = (INT8U)(retLen&0x00ff);//长度字节低字节
 			memcpy(&retBuff[4],buff,retLen);
 			retLen+=4;
 		}
 		return retLen;
 	}

 //组织属性对象中的当前计数器，返回添加进入的字节数量
 //attrindex OAD内属性索引
 	//已测
 INT8U composeEsamCurrentCounter(INT8U attrindex,EsamInfo *esamInfo,INT8U *retBuff)
 {
	 INT8U retLen=0;
	 if(attrindex == 0x01)//单地址应用协商计数器
	 {
		 retBuff[0]=0x06;//double-long-unsigned
		 memcpy(&retBuff[1],esamInfo->CurrentCounter.SingleAddrCounter,4);
		 retLen=0x05;
	 }
	 else if(attrindex == 0x02)//主动上报计数器
	 {
		 retBuff[0]=0x06;//double-long-unsigned
		 memcpy(&retBuff[1],esamInfo->CurrentCounter.ReportCounter,4);
		 retLen=0x05;
	 }
	 else if(attrindex == 0x03)//应用广播通信序列号
	 {
		 retBuff[0]=0x06;//double-long-unsigned
		 memcpy(&retBuff[1],esamInfo->CurrentCounter.BroadCastSID,4);
		 retLen=0x05;
	 }
	 else//当前计数器整个信息域
	 {
		retBuff[0]=0x02;//struct
		retBuff[1]=0x03;//3个元素
		retBuff[2]=0x06;//double-long-unsigned
		memcpy(&retBuff[3],esamInfo->CurrentCounter.SingleAddrCounter,4);
		retBuff[7]=0x06;
		memcpy(&retBuff[8],esamInfo->CurrentCounter.ReportCounter,4);
		retBuff[12]=0x06;
		memcpy(&retBuff[13],esamInfo->CurrentCounter.BroadCastSID,4);
		retLen=2+3*5;
	 }
	 return retLen;
 }
 //ESAM证书版本
 INT8U composeEsamCcieVersion(INT8U attrindex,EsamInfo *esamInfo,INT8U *retBuff)
 {
	 INT8U retLen=0;
	 if(attrindex == 0x01)
	 {
		 retBuff[0]=0x09;//octet-string
		 retBuff[1]=0x01;//长度
		 retBuff[2]=esamInfo->CcieVersion.TerminalCcieVersion;
		 retLen=3;
	 }
	 else if(attrindex ==0x02)
	 {
		 retBuff[0]=0x09;//octet-string
		 retBuff[1]=0x01;//长度
		 retBuff[2]=esamInfo->CcieVersion.ServerCcieVersion;
		 retLen=3;
	 }
	 else
	 {
		 retBuff[0]=0x02;//struct
		 retBuff[1]=0x02;//3个元素
		 retBuff[2]=0x09;//octet-stirng
		 retBuff[3]=0x01;//长度
		 retBuff[4]=esamInfo->CcieVersion.TerminalCcieVersion;
		 retBuff[5]=0x09;//octet-stirng
		 retBuff[6]=0x01;//长度
		 retBuff[7]=esamInfo->CcieVersion.ServerCcieVersion;
		 retLen=8;
	 }
	 return retLen;
 }
 //ESAM属性获取
 //输入：单个oad，retBuff,返回字符串指针，将需要返回字符串写入，开头为长度
 //输出：写入retBuff长度
 //当属性为0的时候，返回错误，因为无法将主站和客户端的证书组报文返回去，也没这么用的
 //已测
INT16U getEsamAttribute(OAD oad,INT8U *retBuff)
{
	INT32S retLen=0;
	static struct timeval tv_store;//存储静态时间，用于list类型属性提取，不用多次esam访问
	static 	EsamInfo esamInfo;//
	INT8U attnum = oad.attflg&0x1F;
	if(attnum == 0x0C || attnum==0x0A)//主站/终端证书属性
	{
		retLen = getEsamCcie(attnum,retBuff);
		return retLen<=0 ? 0:retLen;
	}
	struct timeval tv_new;//静态存储时间
	gettimeofday(&tv_new, NULL);
	fprintf(stderr,"get esam attribute 00000\n");
	if(tv_store.tv_sec == 0 || (tv_new.tv_sec - tv_store.tv_sec)>=3)//第一次进入该函数，或有效时间超过3秒，重新esam访问
	{
		fprintf(stderr,"get esam attribute 11111\n");
		INT32S fd=-1;
		fd = Esam_Init(fd,(INT8U*)ACS_SPI_DEV);
		if(fd>0)
		{
			retLen = Esam_GetTermiInfo(fd,&esamInfo);
			if(retLen>0)
				memcpy(&tv_store,&tv_new,sizeof(tv_store));//更新存储时间
		}
		if(fd<=0 || retLen<0) //打开esam失败，返回DAR错误0x16
		{
			return 0;
		}
		if(fd>0)	Esam_Clear(fd);
	}
	//经过以上的过滤，处理3种情况(第一次进入，时间超时，证书，一下直接从esamInfo中拷贝属性信息)
	switch(attnum)
	{
		case 0x02:    //ESAM序列号
			retBuff[0] = 0x09;//octet-string
			retBuff[1]=0x08;//长度
			memcpy(&retBuff[2],esamInfo.EsamSID,8);
			retLen=2+8;
			break;
		case 0x03:   //ESAM版本号
			retBuff[0] = 0x09;//octet-string
			retBuff[1]=0x04;
			memcpy(&retBuff[2],esamInfo.EsamVID,4);
			retLen=2+4;
			break;
		case 0x04:    //对称秘钥版本
			retBuff[0] = 0x09;//octet-string
			retBuff[1] = 0x10;
			memcpy(&retBuff[2],esamInfo.SecretKeyVersion,16);
			retLen=2+16;
			break;
		case 0x05:    //会话时效门限
			retBuff[0]=0x06;//double-long-unsigned
			retBuff[1] = 0x04;
			memcpy(&retBuff[2],esamInfo.SessionTimeHold,4);
			retLen=2+4;
			break;
		case 0x06:   //会话时效剩余时间
			retBuff[0]=0x06;//double-long-unsigned
			retBuff[1] = 0x04;
			memcpy(&retBuff[2],esamInfo.SessionTimeLeft,4);
			retLen=2+4;
			break;
		case 0x07:    //当前计数器
			retLen = composeEsamCurrentCounter(oad.attrindex,&esamInfo,retBuff);
			break;
		case 0x08:      //证书版本
			retLen = composeEsamCcieVersion(oad.attrindex,&esamInfo,retBuff);
			break;
		case 0x09:     //终端证书序列号
			retBuff[0] = 0x09;//octet-string
			retBuff[1] = 0x10;
			memcpy(&retBuff[2],esamInfo.TerminalCcieSID,16);
			retLen=2+16;
			break;
		case 0x0B:    //主站证书序列号
			retBuff[0] = 0x09;//octet-string
			retBuff[1] = 0x10;
			memcpy(&retBuff[2],esamInfo.ServerCcieSID,16);
			retLen=2+16;
			break;
		case 0x0D:    //ESAM安全存储对象列表
			break;
		default:
			break;
	}
	return retLen;
}
//esam方法操作7，秘钥更新（02 02 09 82 00 C0 7F CA 75。。。。）
//输入：Data2为原始报文头，包括真个秘钥更新的结构体  02结构体 02 2个元素 09 octetstring 82 可变2个字节 00 C0长度字节 7F CA 75。。。。
//输出：
INT32S esamMethodKeyUpdate(INT8U *Data2)
{
	 INT16U secureLen=0;
	 SID_MAC sidmac;
	 INT32S tmplen=0;
	if(Data2[0]==0x02 && Data2[1]==0x02 && Data2[2]==0x09)//此处必须严格遵守字节值
	{
		secureLen = GetDataLength(&Data2[3]);//包含头部的长度字节数量
		if(secureLen <= 0)			return -1;
		INT32S fd=-1;
		fd = Esam_Init(fd,(INT8U*)ACS_SPI_DEV);
		if(fd>0)
		{
			tmplen = UnitParse(&Data2[2+secureLen+1],(INT8U *)&sidmac,0x01);//填充sidmac中sid部分
			if(tmplen<=0) return -2;
			tmplen = UnitParse(&Data2[2+secureLen+1+tmplen],sidmac.mac,0x02);//填充mac
			if(tmplen<=0) return -3;
			tmplen = Esam_SymKeyUpdate(fd,sidmac,&Data2[3]);//秘钥更新
			if(fd>0) Esam_Clear(fd);
			return tmplen;
		}
		else
			return -4;
	}
	else
		return -5;
}
//esam 方法操作8  证书更新----------//esam 方法操作9 设置协商时效门限
//方法8和9   统一一个方法解决
INT32S esamMethodCcieSession(INT8U *Data2)
{
	 INT16U secureLen=0;
	 SID sid;
	 INT32S tmplen=0;
	 if(Data2[0]==0x02 && Data2[1]==0x02 && Data2[2]==0x09)//此处必须严格遵守字节值
	{
		 secureLen = GetDataLength(&Data2[3]);//包含头部的长度字节数量
		if(secureLen <= 0)			return -1;
		INT32S fd=-1;
		fd = Esam_Init(fd,(INT8U*)ACS_SPI_DEV);
		if(fd>0)
		{
			tmplen = UnitParse(&Data2[2+secureLen+1],(INT8U *)&sid,0x01);//填充sidmac中sid部分
			if(tmplen<=0) return -2;
			tmplen = Esam_CcieSession(fd,sid,&Data2[3]);//证书更新///协商时效门限
			if(fd>0)  Esam_Clear(fd);
			return tmplen;
		}
		else return -3;
	}
	 else return -4;
}


