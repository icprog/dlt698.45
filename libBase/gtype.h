/*
 * gtype.h
 *
 *  Created on: Sep 11, 2015
 *      Author: fzh
 */

#ifndef GTYPE_H_
#define GTYPE_H_
#include "option.h"
#include "StdDataType.h"
typedef struct
{
	INT8U VendorCode[2];	//厂商代码（VendorCode[1~0]：TC=鼎信；ES=东软；LH=力合微；37=中瑞昊天）
	INT8U ChipCode[2];		//芯片代码
	INT8U VersionDay;		//版本日期-日
	INT8U VersionMonth;		//版本日期-月
	INT8U VersionYear;		//版本日期-年
	INT8U Version[2];		//版本
}ZB_Info;	//厂商代码和版本信息
typedef struct
{
	INT8U chg6000;		/*采集档案配置表属性变更*/
	INT8U chg6002;		/*搜表类属性变更*/
	INT8U chg6012;		/*任务配置表属性变更*/
	INT8U chg6014;		/*普通采集方案集属性变更*/
	INT8U chg6016;		/*事件采集方案集属性变更*/
	INT8U chg6018;		/*透明方案集属性变更*/
	INT8U chg601C;		/*上报方案集属性变更*/
	INT8U chg601E;		/*采集规则库属性变更*/
	INT8U chg6051;		/*实时监控采集方案集属性变更*/
}PARA_CHG;
typedef struct
{
	INT8U online_state;	/*gprs在线1 ，cdma在线2 ，FDD_LTE在线3 ，TD_LTE在线4 ，以太网在线5 ，正在拨号中6 */
	INT8U csq;			/*信号强度*/
	INT8U pppip[16];	/*无线IP*/
	INT8U sa[16];		/*集中器地址*/
	INT8U yx[4];		/*遥信状态*/
	ZB_Info zbinfo;		/*载波模块信息*/
	INT8U zbstatus;		/*载波抄表状态*/
	INT8U jiexian;		/*交采接线类型*/
	PARA_CHG parachg;	/*参数变更标识*/

}TerminalMemType;


#endif /* GTYPE_H_ */
