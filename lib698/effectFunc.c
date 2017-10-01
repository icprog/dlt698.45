/*
 * effectFunc.c
 *
 *  Created on: Jun 30, 2017
 *      Author: admin
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "AccessFun.h"
#include "StdDataType.h"
#include "Objectdef.h"
#include "PublicFunction.h"
#include "dlt698.h"

/*
 * 日期合法性判定
 * */
INT8U check_date(int year, int month, int day, int hour, int min, int sec)
{
	struct tm tm_new;

	//0xFF:表示时间无效，控制下发参数（0x8105）的起始、结束时间会有FF数据下发
	if(year==0xFFFF || month==0xFF || day==0xFF || hour==0xFF || min==0xFF || sec==0xFF) {
		return success;
	}
	if (year < 1900 || month <= 0 || month > 12 || day <= 0 || day > 31
			|| hour < 0 || hour > 23 || min < 0 || min>59 || sec < 0 || sec > 59)	{
//		syslog(LOG_ERR,"时间不合法: %d-%d-%d %d:%d:%d",year,month,day,hour,min,sec);
//		fprintf(stderr,"时间不合法: %d-%d-%d %d:%d:%d",year,month,day,hour,min,sec);
		return dblock_invalid;
	}
	tm_new.tm_year = year - 1900;
	tm_new.tm_mon = month - 1;
	tm_new.tm_mday = day;
	tm_new.tm_hour = hour;
	tm_new.tm_min = min;
	tm_new.tm_sec = sec;
	time_t time_new = mktime(&tm_new);
	localtime_r(&time_new, &tm_new);
	if (tm_new.tm_year != (year - 1900) || tm_new.tm_mon != (month - 1) || tm_new.tm_mday != day) {
//		fprintf(stderr,"时间不合法: %d-%d-%d %d:%d:%d",year,month,day,hour,min,sec);
//		syslog(LOG_ERR,"时间不合法: %d-%d-%d %d:%d:%d",year,month,day,hour,min,sec);
		return dblock_invalid;
	}else	{
		return success;
	}
}

/*
 * 根据下发的TimeTag值，判断有效性
 * 时效性判断规则：
 * 在时间标签中允许传输延时时间大于零的前提下，如果接收方的当前时间与时间标签中的开始发送时间之间的时差大于时间标签中的允许传输延时时间，则放弃处理；反之，则处理。
 * 因报文解析处理麻烦，改用从报文尾向前查找的方法。所以在判断条件上依次判断各个属性值的有效性，防止处理错误。
 * */
void isTimeTagEffect(TimeTag timetag,TimeTag *rec_timetag)
{
	time_t	nowtime_t = 0,tagtime_t = 0;
//	TS		tmpts;
	INT32U	interval_s = 0;		//TI间隔转换到秒数
	int		ret = 0;

	memset(rec_timetag,0,sizeof(TimeTag));
	rec_timetag->effect = 1;	//默认时间标签合法

	fprintf(stderr,"\ntimeTag:flag=%d\n",timetag.flag);
	printDataTimeS("time",timetag.sendTimeTag);
	printTI("tag",timetag.ti);
	if(timetag.flag==1) {	//时间标签有效
		ret = check_date(timetag.sendTimeTag.year.data,timetag.sendTimeTag.month.data,timetag.sendTimeTag.day.data,
				timetag.sendTimeTag.hour.data,timetag.sendTimeTag.min.data,timetag.sendTimeTag.sec.data);
		if(ret == success) {
			if(timetag.ti.units >=0 && timetag.ti.units <=5) {		//TI 间隔时间单位满足条件
				interval_s = TItoSec(timetag.ti);
				fprintf(stderr,"interval_s = %d\n",interval_s);
				nowtime_t = time(NULL);
//				TimeBCDToTs(timetag.sendTimeTag,&tmpts);
//				tagtime_t = tmtotime_t(tmpts);
				tagtime_t = TimeBCDTotime_t(timetag.sendTimeTag);
				fprintf(stderr,"nowtime=%ld, rece_tag=%ld, sub=%d interval_s=%d\n",nowtime_t,tagtime_t,(int)(nowtime_t-tagtime_t),interval_s);
				//国网台体测试，下发时间后，再设置参数的timetag比集中器时间慢1秒.此处用绝对值进行比较,防止判断时间标签无效
				//一致性测试：延时时间=0，应正确响应
				if((abs(nowtime_t-tagtime_t)<= interval_s) || (interval_s==0)) {
					rec_timetag->effect = 1;
				}else {
					rec_timetag->effect = 0;
				}
				rec_timetag->flag = 1;
				memcpy(&rec_timetag->ti,&timetag.ti,sizeof(timetag.ti));
				//时间标签不进行更新，响应帧上送客户端的时间标签
				//rec_timetag->sendTimeTag = timet_bcd(nowtime_t);
				memcpy(&rec_timetag->sendTimeTag,&timetag.sendTimeTag,sizeof(DateTimeBCD));
			}
		}else rec_timetag->flag = 0;
	}
	fprintf(stderr,"Response:时间标签有效性：%d\n",rec_timetag->effect);
	fprintf(stderr,"rec_timetag:flag=%d\n",rec_timetag->flag);
	printDataTimeS("time",rec_timetag->sendTimeTag);
	printTI("tag",rec_timetag->ti);
}

/*
 * 判断枚举类型数据的合法性，
 * 该函数能成功判断的前提枚举类型的值必须是联系的。
 * 输入条件： start :枚举类型的开始值， end: 枚举类型的结束值, other:其他值
 * 返回值：success：合法，dblock_invalid：非法
 * */
INT8U	getEnumValid(INT16U value,INT16U start,INT16U end,INT16U other)
{
	int i=0;
  	for(i=start;i<=end;i++) {
  		if(value == i) {
  			return success;
  		}
  	}
  	if(value == other) {
  		return success;
  	}
  	syslog(LOG_ERR,"枚举越限value=%d,start=%d,end=%d,other=%d",value,start,end,other);
	return dblock_invalid;
}

INT8U	getCOMDCBValid(COMDCB comdcb)
{
	INT8U DAR=success;
	DAR = getEnumValid(comdcb.baud,bps300,bps115200,autoa);
	if(DAR!=success)  return dblock_invalid;
	DAR = getEnumValid(comdcb.verify,none,even,none);
	if(DAR!=success)  return dblock_invalid;
	DAR = getEnumValid(comdcb.databits,d5,d8,d5);
	if(DAR!=success)  return dblock_invalid;
	DAR = getEnumValid(comdcb.stopbits,stop1,stop2,stop1);
	if(DAR!=success)  return dblock_invalid;
	DAR = getEnumValid(comdcb.flow,no,soft,no);
	if(DAR!=success)  return dblock_invalid;
	return success;
}
/*
 * 判断设置得OAD端口是否正确？
 */
INT8U getPortValid(OAD oad)
{
    if(oad.OI == 0xF200 ||
    		oad.OI == 0xF201 ||
    		oad.OI == 0xF202 ||
    		oad.OI == 0xF209)
    	return success;
	return type_mismatch;
}
/*
 * selector2 选择条件的合法性判断，起始时间>=结束时间，返回0，否则返回1
 * */
INT8U DataTimeCmp(DateTimeBCD startdt,DateTimeBCD enddt)
{
	time_t t1=0,t2=0;
	t1 = TimeBCDTotime_t(startdt);
	t2 = TimeBCDTotime_t(enddt);
	if(t1>=t2)  return 0;
	else return 1;
}

/*
 *  限值判断 ,注意限值为int型，不能超过0xffffffff
 *  		超过限值 返回设定限值
 *  		否则    返回实际值
 * */
int limitJudge(char *desc,int limit,int val)
{
	if(val>limit) {
		syslog(LOG_ERR,"%s[%d] ,大于设定值[%d],异常！！",desc,val,limit);
		return limit;
	}else
		return val;
}

/*
 *  范围值判断 注意：只比对int型数据
 *  满足范围：返回实际值
 *  否则：返回-1
 * */
int rangeJudge(char *desc,int val,int min,int max)
{
	if(val>=min && val<=max) {
		return val;
	}else {
		syslog(LOG_ERR,"%s,值[%d] 超限[%d-%d],异常！！",desc,val,min,max);
		return -1;
	}
}
