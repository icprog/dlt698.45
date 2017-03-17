/*
 * SecureMode.c
 *
 *  Created on: 2017-1-13
 *      Author: gk
 */
#include <stdio.h>
#include <string.h>

#include "dlt698def.h"
#include "../include/StdDataType.h"
#include "../include/Objectdef.h"
typedef struct
{
	SecureModel securePara;
	INT8U mask;//安全模式参数里有ZZ存在，此mask标记ZZ.mask=Z个数
}SecureUnit;
SecureUnit DefaultSecurePara[]={{{0x0FFF,0x8004},3},//当前电量
														{{0x1FFF,0x4000},3},//最大需量
														{{0x202C,0x8004},0},//当前钱包文件
														{{0x202F,0x4180},0},//电能表主动上报
														{{0x2FFF,0x4040},3},//变量
														{{0x3FFF,0x4110},3},//事件
														{{0x4000,0x8184},0},//日期时间
														{{0x4001,0x8104},0},//通信地址
														{{0x4002,0x8404},0},//表号
														{{0x4003,0x4400},0},//客户编号
														{{0x400A,0x4500},0},//两套分时费率切换时间
														{{0x400B,0x4500},0},//两套阶梯切换时间
														{{0x401C,0x4500},0},//电流互感器变比
														{{0x401D,0x4500},0},//电压互感器变比
														{{0x401E,0x4500},0},//金额限值
														{{0x4018,0x4000},0},//当前套费率电价
														{{0x4019,0x4500},0},//备用套费率电价
														{{0x401A,0x4000},0},//当前套阶梯参数
														{{0x401B,0x4500},0},//备用套阶梯参数
														{{0x4111,0x8104},0},//备案号
														{{0x4FFF,0x4110},3},//参变量
														{{0x5000,0x4140},2},//勘误新增项。支持广播瞬时冻结需要。
														{{0x50FF,0x4110},2},//冻结（勘误修改）
														{{0x60FF,0x8887},2},//采集监控
														{{0x70FF,0x8085},2},//集合
														{{0x80FF,0x4110},2},//控制
														{{0xF000,0x8110},0},//分帧传输
														{{0xF001,0x8220},0},//分块传输
														{{0xF002,0x8880},0},//扩展传输
														{{0xF100,0x8414},0},//ESAM
														{{0xF101,0x8124},0},//安全模式参数
														{{0xF2FF,0x8887},2},//输入输出接口设备
														{{0xFFFF,0x8887},2}//自定义
};
/*
 * 比较安全等级
 * 安全等级数值越小，等级越高
 * 输入： sclass终端存储对象等级，dclass主站下发报文安全等级
 * 输出：返回1，主站下发等级高于或等于终端存储等级，合格；返回0，低于终端存储等级，不合格，返回-1，发生错误
 */
INT8S SMode_CompareClass(INT8U sclass,INT8U dclass)
{
	if((sclass&dclass) == dclass || dclass !=0x00) return 1;//默认安全参数中，有的IO有2项，此处直接返回合格
	if(sclass ==0x00 || dclass==0x00) return -1;//默认安全参数里，有0x00项，代表不能操作，此处返回-1
	if((sclass&0x0F)<(dclass&0x0F)) return 0;
	else return 1;
}
//获取apdu类型对应的优先级，4bits
INT8S SMode_TypeGrade(INT16U classNo,INT8U apduType)
{
	INT8U ret=0x00;
	switch(apduType)
	{
	case GET_REQUEST:
		ret=(INT8U)(classNo>>12);
		break;
	case SET_REQUEST:
		ret=(INT8U)(classNo>>8);
		break;
	case ACTION_REQUEST:
		ret=(INT8U)(classNo>>4);
		break;
	case PROXY_REQUEST:
		ret=(INT8U)(classNo);
		break;
	default:
		break;
	}
	return ret&0x0F;//高4位清一下，似乎用不着清
}
//获取叠加掩码后oad数值  如：0x1fff 掩码3  返回0x1000
INT16U SMode_MaskDeal(OI_698 oi,INT8U mask)
{
	return (oi&(0xffff<<(4*mask)));
}
/****************************************************************************
 * 单个OAD获取安全模式参数安全等级  0x08   0x04  0x02  0x01   数值越小，安全等级越高
 * 输入：OADnum主站下发OAD之一。SMPara  F101属性3参数起始指针，paranum（属性3）个数
 * 输出：优先级    返回-1，代表没有找到，此时需要根据apduType判断默认优先级，返回0x00，该优先级代表没有权限，不能操作，返回否认帧
 * 说明：oad首先和显示安全模式参数比较，显示模式安全参数完整的oad，没有zz，只需要判断==
 * 之后如果没有找到显示安全参数，再和默认安全参数比较（显示安全模式参数为首选标准）
 *****************************************************************************/
INT8S SMode_OADGetClass(OAD oad,SecureModel* SMPara,INT16U paraNum,INT8U apduType)
{
	INT16U i=0;
	INT8U masktmp=0;
	OI_698 classtmp=0x0000;

	if(paraNum!=0)//如果显示安全模式参数有设置
	{
		for(i=0;i<paraNum;i++)
		{
			if(oad.OI == (SMPara+i)->oi)
			{
				return SMode_TypeGrade((SMPara+i)->model,apduType);
			}
		}
	}
	for(i=0;i<(sizeof(DefaultSecurePara)/sizeof(SecureUnit));i++)//计算defaultSecurePara元素个数，查找符合的oad安全等级
	{
		if(SMode_MaskDeal(oad.OI,DefaultSecurePara[i].mask)==SMode_MaskDeal(DefaultSecurePara[i].securePara.oi,DefaultSecurePara[i].mask))
		{
			if(DefaultSecurePara[i].mask==0x00)//mask为0，找到完全匹配的默认安全模式参数，
				return SMode_TypeGrade(DefaultSecurePara[i].securePara.model,apduType);//返回安全等级
			else
			{
				if(masktmp==0)//初次找到‘类似’符合要求的oad，记录掩码和oi，继续寻找(好理解的说)
				{
					masktmp=DefaultSecurePara[i].mask;
					classtmp=DefaultSecurePara[i].securePara.model;
				}
				else                        //再次找到‘类似’符合要求的oad，和已存掩码比较，存储掩码值小的oi和掩码
				{
					if(masktmp>DefaultSecurePara[i].mask)
					{
						masktmp=DefaultSecurePara[i].mask;
						classtmp=DefaultSecurePara[i].securePara.model;

					}
				}
			}
		}
	}
	if(masktmp!=0 && classtmp!=0)
		return SMode_TypeGrade(classtmp,apduType);
	else
		return -1;
}
/*获取多个oad在单个apdu类型下的最高安全等级    oad可为1
 * 对应宣贯材料中，如果客户段通过一个APDU同时访问多个对象，当对象的安全级别不一致的时候，一起种最高安全级别为准
 * 特殊情况：数据读取时安全模式参数中没有明确要求，均采用明文+MAC方式
*                     参数设置如果在安全模式参数中没有明确要求，均采用密文+MAC方式设置
*                     其他（操作/代理）材料中未找到说明，此处采用密文加MAC处理！！！！
*                     0x01 密文+MAC    0x02密文      0x04明文+MAC     0x08明文      0x00无操作权限
*/
INT8S SMode_OADListGetClass(OAD *oad,INT16U oadNum,SecureModel* SMPara,INT16U paraNum,INT8U apduType)
{
	INT8S classtmp =0x0F;
	INT16U i=0;
	if(apduType !=GET_REQUEST && apduType!= SET_REQUEST && apduType != ACTION_REQUEST && apduType != PROXY_REQUEST)
		return -1;
	for(i=0;i<oadNum;i++)//可能多个oad中存在无安全模式参数的oad，此种情况忽略该oad
	{
		INT8S tmp = SMode_OADGetClass(*(oad+i),SMPara,paraNum,apduType);
		if(tmp!=-1)//找到对应的安全模式
		{
			if(tmp<classtmp)//寻找优先级最高，即数值最小的，如果存在优先级数值为0x00，无操作权限
			{
				classtmp = tmp;
			}
		}
	}
	if(classtmp ==0x0F)// 在显示安全模式和默认安全模式下，都没有获取到安全等级
	{
		if(apduType==GET_REQUEST)
			classtmp = 0x04;
		else if(apduType==SET_REQUEST)//
			classtmp = 0x01;
		else                             //资料无约定，此处暂定密文+MAC
			classtmp = 0x01;
	}
	return classtmp;
}
