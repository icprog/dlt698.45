/*
 * calc.c
 *
 *  Created on: 2017-2-15
 *      Author: wzm
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>
#include "sys/reboot.h"
#include <wait.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "calc.h"
#include "cjdeal.h"
#include "ParaDef.h"
#include "AccessFun.h"
#include "Objectdef.h"

//#ifdef SPTF_III
//SumGroup_TYPE sumgroup[MAXNUM_SUMGROUP];
//#endif
POINT_CALC_TYPE point[MAXNUM_IMPORTANTUSR_CALC];
MaxDemand tjXuliang_acs;				//日冻结交采需量统计
MaxDemand tjXuliang_acs_m;				//月冻结交采需量统计
INT8U 	CalcPointNum;		//需要统计的最大个数
CLASS_4030 obj_offset={};
/*
 * 	山东要求：电压合格率统计，在停上电1分钟及停电期间，不进行电压合格统计
 * =1:满足上电1分钟要求，可以进行电压合格率统计
 * =0：掉电中，或上电不足一分钟，不进行电压合格率统计
 * */
int	getcalcvalid()
{
	static time_t	tingtime=0,starttime=0;
	static INT8U	firstflg=0;
	if(pwr_has_byVolt(JProgramInfo->ACSRealData.Available,
			          JProgramInfo->ACSRealData.Ua,
			          JProgramInfo->event_obj.Event3106_obj.poweroff_para_obj.screen_para_obj.happen_voltage_limit)==FALSE){
		if(firstflg==0)	{
			firstflg=1;
			tingtime = time(NULL);
		}
		return 0;
	}else {
		firstflg = 0;
		if(tingtime==0)	tingtime = time(NULL);
		starttime =  time(NULL);
		if(abs(starttime-tingtime) < 60) {
			return 0;
		}else {
			return 1;
		}
	}
}


/* 记录各测量点类型和数量,一号测量点可以是交采测试点,也可以下挂电表测量点
 * 测量点类型包括:暂时统计485测量点,交采测试点
 * */
INT8U write_calc_stru(POINT_CALC_TYPE *pcalc)
{
	pcalc[0].valid = FALSE;
	CLASS_6001 meter={};
	CLASS11		coll={};
	INT16U oi=0x6000;
	int	i=0,blknum=0,num=0;
	if(readInterClass(oi,&coll)==-1) return 0;
	blknum = getFileRecordNum(oi);
	if(blknum == -1) {
		fprintf(stderr,"未找到OI=%04x的相关信息配置内容！！！\n",oi);
		return 0;
	}else if(blknum == -2){
		fprintf(stderr,"采集档案表不是整数，检查文件完整性！！！\n");
		return 0;
	}
	for(i=0;i<blknum;i++){
		if(readParaClass(oi,&meter,i)==1) {
			//读交采和485表进行统计
           if(meter.basicinfo.port.OI == 0xF201 || meter.basicinfo.port.OI == 0xF208){
        	   pcalc[num].PointNo=meter.sernum;
        	   if(meter.basicinfo.port.OI == 0xF201)
        		   pcalc[num].Type=JIAOCAI_TYPE;
        	   else if(meter.basicinfo.port.OI == 0xF208)
        		   pcalc[num].Type=METER_485_TYPE;
        	   else
        		   pcalc[num].Type=METER_485_TYPE;
        	   pcalc[num].valid = TRUE;
        	   num++;
        	   if(num>=MAXNUM_IMPORTANTUSR_CALC)	{
					fprintf(stderr,"设置超过容量，考虑扩容,num=%d\n",num);
					return num;
				}
           }
		}
	}
	return num;
}
/*
 *根据系统参数确定测量点信息，
 *填充POINT_CALC_TYPE结构体数据，用于进行统计计算
 *pcalc[0]的位置始终放置交采数据及限值
 */
INT8U get_point_para(POINT_CALC_TYPE *pcalc)
{
	static INT8U lastchgoi6000;	//保存参数改变次数，0
	static INT8U first=1;
	INT8U	calcnum=0;

	if(first){
		first=0;
		lastchgoi6000 = JProgramInfo->oi_changed.oi6000;
		calcnum = write_calc_stru(pcalc);
	}
	if(lastchgoi6000!=JProgramInfo->oi_changed.oi6000){
		calcnum = write_calc_stru(pcalc);
		if(lastchgoi6000!=JProgramInfo->oi_changed.oi6000) {
			lastchgoi6000++;
			if(lastchgoi6000==0) lastchgoi6000=1;
		}
	}
	return calcnum;
}

/*
 * 限值比较
 * 输入参数：value：采样值，ss_limit:上上限，s_limit：上限，x_limit：下限，xx_limit：下下限
 * 返回：越限类型
 * */
int cmp_limit(int value, int ss_limit,int s_limit,int x_limit,int xx_limit)
{
	if (value > ss_limit&&ss_limit!=0) 		    return SSHANG_XIAN;
	if ((value > s_limit&&s_limit!=0) && (value <= ss_limit&&ss_limit!=0)) return SHANG_XIAN;
	if ((value > x_limit&&x_limit!=0) && (value<= s_limit&&s_limit!=0))  return HEGE;
	if ((value > xx_limit&&xx_limit!=0)  && (value <= x_limit&&x_limit!=0))  return XIA_XIAN;
	if (value <  xx_limit) 						  return XXIA_XIAN;
	return 0;
}
/*计算单相当前状态的累计时间和总电压
 *参数：参数1，单相电压的属性结构体，参数2，当前时间，参数3，出参，当前状态的累计时间，参数4，出参，电压的累加值，参数5，当前电压值。
 *函数的计算结果，直接存储在参数2中
 * */
void voltageRate_deal(voltageRateProp* voltageRate,struct tm NowTime,INT16U *count,INT32U *U_Sum,INT32U voltage)
{
	INT32U count_min = 0;
	time_t tmp;
	if(voltageRate->Last_count == 0)
	{
		tmp = mktime(&NowTime)-60;
		//tmp = mktime(&NowTime);
		localtime_r(&tmp,&voltageRate->StartTime);//记录起始时间为上一分种

		voltageRate->Last_count = 1;
		(*count)++;
		(*U_Sum)+=voltage;
	}
	else
	{
		fprintf(stderr,"voltageRate_deal-mktime(&NowTime):%ld,mktime(&voltageRate->StartTime):%ld\r\n",
				mktime(&NowTime),mktime(&voltageRate->StartTime));

		//防止向前校表
		if(mktime(&NowTime) > mktime(&voltageRate->StartTime))
		{
			//count_min = (INT32U)(difftime(mktime(&NowTime),mktime(&voltageRate->StartTime))/60);

			count_min = (INT32U)(mktime(&NowTime)/60 - mktime(&voltageRate->StartTime)/60);
			fprintf(stderr,"count_min:%d,(*count):%d,voltageRate->Last_count:%d,voltage:%d\r\n\r\n",count_min,(*count),voltageRate->Last_count,voltage);
	//		printf("count_min:%d,voltageRate.Last_count:%d,*count:%d\r\n",count_min,voltageRate.Last_count,*count);
			//统计当前状态的累计时间
			(*count) += (count_min- voltageRate->Last_count);
			//累加单相的总电压
			(*U_Sum) += ((count_min- voltageRate->Last_count)*voltage);
			voltageRate->Last_count = count_min;
		}
		else
		{
			tmp = mktime(&NowTime)-120;

			localtime_r(&tmp,&voltageRate->StartTime);//记录起始时间为上一分种
		}
	}
}

/*电压合格率处理函数
 *参数：参数1，当前电压的状态，参数2，电压的统计属性，参数3，当前电压值
 * */
void voltage_result_deal(int result ,DIANYA_TJ* CurrVoltage,INT32U voltage)
{
	struct tm NowTime;
	time_t time_of_day;
	time_t tmp;
	time_of_day = time(NULL);
	localtime_r(&time_of_day, &NowTime);

	if(CurrVoltage->U_Count == 0)
	{
		tmp = mktime(&NowTime)-60;
		//tmp = mktime(&NowTime);
		localtime_r(&tmp,&CurrVoltage->StartTime);//记录起始时间为上一分种
		CurrVoltage->U_Count++;
	}
	else
	{
		//防止向前校表
		if(mktime(&NowTime) > mktime(&CurrVoltage->StartTime))
		{
			CurrVoltage->U_Count +=(INT32U)(mktime(&NowTime)/60 - mktime(&CurrVoltage->StartTime)/60);
		}
		else
		{
			tmp = mktime(&NowTime)-120;
			localtime_r(&tmp,&CurrVoltage->StartTime);//记录起始时间为上一分种
		}
	}

	switch (result)
	{
		case HEGE:
			voltageRate_deal(&CurrVoltage->voltageRate_ok,NowTime,&CurrVoltage->ok_count,&CurrVoltage->U_Sum,voltage);
			CurrVoltage->voltageRate_S.Last_count = 0;
			CurrVoltage->voltageRate_SS.Last_count = 0;
			CurrVoltage->voltageRate_x.Last_count = 0;
			CurrVoltage->voltageRate_xx.Last_count = 0;
			break;
		case SHANG_XIAN:
			voltageRate_deal(&CurrVoltage->voltageRate_S,NowTime,&CurrVoltage->s_count,&CurrVoltage->U_Sum,voltage);
			CurrVoltage->voltageRate_ok.Last_count = 0;
			CurrVoltage->voltageRate_SS.Last_count = 0;
			CurrVoltage->voltageRate_x.Last_count = 0;
			CurrVoltage->voltageRate_xx.Last_count = 0;
			break;
		case SSHANG_XIAN:
			voltageRate_deal(&CurrVoltage->voltageRate_SS,NowTime,&CurrVoltage->ss_count,&CurrVoltage->U_Sum,voltage);
			CurrVoltage->voltageRate_S.Last_count = 0;
			CurrVoltage->voltageRate_ok.Last_count = 0;
			CurrVoltage->voltageRate_x.Last_count = 0;
			CurrVoltage->voltageRate_xx.Last_count = 0;
			break;
		case XIA_XIAN:
			voltageRate_deal(&CurrVoltage->voltageRate_x,NowTime,&CurrVoltage->x_count,&CurrVoltage->U_Sum,voltage);
			CurrVoltage->voltageRate_S.Last_count = 0;
			CurrVoltage->voltageRate_SS.Last_count = 0;
			CurrVoltage->voltageRate_ok.Last_count = 0;
			CurrVoltage->voltageRate_xx.Last_count = 0;
			break;
		case XXIA_XIAN:
			voltageRate_deal(&CurrVoltage->voltageRate_xx,NowTime,&CurrVoltage->xx_count,&CurrVoltage->U_Sum,voltage);
			CurrVoltage->voltageRate_S.Last_count = 0;
			CurrVoltage->voltageRate_SS.Last_count = 0;
			CurrVoltage->voltageRate_x.Last_count = 0;
			CurrVoltage->voltageRate_ok.Last_count = 0;
			break;
		default:
			CurrVoltage->U_Sum += voltage;
	}

	tmp = mktime(&NowTime);
	localtime_r(&tmp,&CurrVoltage->StartTime);

	return;
}
void cmp_max(int value,int *max_old,unsigned char *ttime)
{
	TS ts;
	if((0 == value)||( NULL == max_old ))
		return;

	if(value >= (*max_old))
	{
		TSGet(&ts);
		*max_old = value;
		if(ttime != NULL)
			ttime[2]=ts.Day; ttime[1]=ts.Hour; ttime[0]=ts.Minute;
	}
}
void cmp_min(int value,int *min_old,unsigned char *ttime)
{
	TS ts;

	if((0 == value)||( NULL == min_old ))
		return;

	if(value <= (*min_old)||(*min_old == 0))
	{
		TSGet(&ts);
		*min_old = value;

		if(ttime != NULL)
			ttime[2]=ts.Day; ttime[1]=ts.Hour; ttime[0]=ts.Minute;
	}
}
/*
 * 四舍五入函数，并将double
 * 参数１，为保留的小数位数,参数２，需要转换的double数字
*/
FP64 Round(int Decbits,FP64 x)
{
  return (int)(pow(10,Decbits)*x+0.5)/(FP64)pow(10,Decbits);
}
/*复位电压统计数据到共享内存*/
void CpPubdata_U(DIANYA_TJ source,Statistics_U * Dest)
{

	Dest->max = source.max;
	memcpy(Dest->max_time,source.max_time,3);
	Dest->min = source.min;
	memcpy(Dest->min_time,source.min_time,3);
	Dest->U_Avg = source.U_Avg;
	Dest->ok_Rate = Round(3,source.ok_Rate)*A25_COEF;
	Dest->ok_count = source.ok_count;
	Dest->s_Rate = Round(3,source.s_Rate)*A25_COEF;
	Dest->s_count = source.s_count;
	Dest->ss_count = source.ss_count;
	Dest->x_Rate = Round(3,source.x_Rate)*A25_COEF;
	Dest->x_count = source.x_count;
	Dest->xx_count = source.xx_count;
	Dest->U_Sum = source.U_Sum;
	Dest->U_Count = source.U_Count;
}

/*
 * 电压合格率统计
 */
void voltage_calc(){
	static INT8U lastchgoi4030=0;
	static INT8U first=1;
	if(first){
		first=0;
		lastchgoi4030 = JProgramInfo->oi_changed.oi4030;
		readParaClass(0x4030,&obj_offset,0);
	}
	if(lastchgoi4030!=JProgramInfo->oi_changed.oi4030){
		readParaClass(0x4030,&obj_offset,0);
		if(lastchgoi4030!=JProgramInfo->oi_changed.oi4030) {
			lastchgoi4030++;
			if(lastchgoi4030==0) lastchgoi4030=1;
		}
	}
	int i=0,result;
	FP32 Rate = 0;
	TS tm;
	TSGet(&tm);
	for(i = 0;i< MAXNUM_IMPORTANTUSR_CALC ;i++)
	{
			INT32U pointno = point[i].PointNo-1;
			if(point[i].valid!=TRUE)
				continue;
			if(point[i].Realdata.Va.Available)
			{
			//计算电压是否合格
			result = cmp_limit(point[i].Realdata.Va.value,obj_offset.uUp_Kaohe,
					obj_offset.uUp,obj_offset.uDown,
					obj_offset.uDown_Kaohe);

//			voltage_result_deal(result,&point[i].Result.tjUa,point[i].Realdata.Va.value/U_COEF);
//			voltage_result_deal(result,&point[i].Result_m.tjUa,point[i].Realdata.Va.value/U_COEF);
			voltage_result_deal(result,&point[i].Result.tjUa,point[i].Realdata.Va.value);

			voltage_result_deal(result,&point[i].Result_m.tjUa,point[i].Realdata.Va.value);
			//计算电压最值及最值发生时间
			cmp_max(point[i].Realdata.Va.value,&point[i].Result.tjUa.max,point[i].Result.tjUa.max_time);
			cmp_max(point[i].Realdata.Va.value,&point[i].Result_m.tjUa.max,point[i].Result_m.tjUa.max_time);
			cmp_min(point[i].Realdata.Va.value,&point[i].Result.tjUa.min,point[i].Result.tjUa.min_time);
			cmp_min(point[i].Realdata.Va.value,&point[i].Result_m.tjUa.min,point[i].Result_m.tjUa.min_time);

			point[i].Result.tjUa.ok_count=point[i].Result.tjUa.U_Count-(point[i].Result.tjUa.x_count+point[i].Result.tjUa.xx_count
					+point[i].Result.tjUa.ss_count+point[i].Result.tjUa.s_count);
			point[i].Result.tjUa.U_Avg = (point[i].Result.tjUa.U_Sum/point[i].Result.tjUa.U_Count);//*U_COEF;
			//计算电压日越上限率，下限率，合格率
			Rate = (((FP32)point[i].Result.tjUa.ss_count+(FP32)point[i].Result.tjUa.s_count)/(FP32)point[i].Result.tjUa.U_Count)*RAT_COEF;
			point[i].Result.tjUa.s_Rate = Rate>100?100:Rate;
			Rate = (((FP32)point[i].Result.tjUa.xx_count+(FP32)point[i].Result.tjUa.x_count)/(FP32)point[i].Result.tjUa.U_Count)*RAT_COEF;
			point[i].Result.tjUa.x_Rate = Rate>100?100:Rate;
			point[i].Result.tjUa.ok_Rate = 100.00-point[i].Result.tjUa.x_Rate-point[i].Result.tjUa.s_Rate;
			point[i].Result_m.tjUa.ok_count=point[i].Result_m.tjUa.U_Count-(point[i].Result_m.tjUa.x_count+point[i].Result_m.tjUa.xx_count+
			point[i].Result_m.tjUa.ss_count+point[i].Result_m.tjUa.s_count);
			point[i].Result_m.tjUa.U_Avg = (point[i].Result_m.tjUa.U_Sum/point[i].Result_m.tjUa.U_Count);// *U_COEF;

			//计算电压月越上限率，下限率，合格率
			Rate = (((FP32)point[i].Result.tjUa.ss_count+(FP32)point[i].Result_m.tjUa.s_count)/(FP32)point[i].Result_m.tjUa.U_Count)*RAT_COEF;
			point[i].Result_m.tjUa.s_Rate = Rate>100?100:Rate;
			Rate = (((FP32)point[i].Result_m.tjUa.xx_count+(FP32)point[i].Result_m.tjUa.x_count)/(FP32)point[i].Result_m.tjUa.U_Count)*RAT_COEF;
			point[i].Result_m.tjUa.x_Rate = Rate>100?100:Rate;
			point[i].Result_m.tjUa.ok_Rate = 100.00-point[i].Result_m.tjUa.x_Rate-point[i].Result_m.tjUa.s_Rate;
			//统计数据给共享内存赋值
			CpPubdata_U(point[i].Result.tjUa,&JProgramInfo->StatisticsPoint[i].DayResu.tjUa);
			CpPubdata_U(point[i].Result_m.tjUa,&JProgramInfo->StatisticsPoint[i].MonthResu.tjUa);
			JProgramInfo->StatisticsPoint[i].PointNo = point[i].PointNo;

		}
		else
		{
			memset((INT8U*)&JProgramInfo->StatisticsPoint[i].DayResu.tjUa,0xee,sizeof(JProgramInfo->StatisticsPoint[i].DayResu.tjUa));
			memset((INT8U*)&JProgramInfo->StatisticsPoint[i].MonthResu.tjUa,0xee,sizeof(JProgramInfo->StatisticsPoint[i].MonthResu.tjUa));
		}

		//code 3
//#ifdef SPTF_III
//		if(shmm_getdevstat()->WireType == 0x0600)
//#endif
		{
			//chengxiaolong add 0629 三相四线才统计Ub
			if(point[i].Realdata.Vb.Available)
			{

				result = cmp_limit(point[i].Realdata.Vb.value,obj_offset.uUp_Kaohe,
						obj_offset.uUp,obj_offset.uDown,
						obj_offset.uDown_Kaohe);

	//			voltage_result_deal(result,&point[i].Result.tjUb,point[i].Realdata.Vb.value/U_COEF);
	//			voltage_result_deal(result,&point[i].Result_m.tjUb,point[i].Realdata.Vb.value/U_COEF);
				voltage_result_deal(result,&point[i].Result.tjUb,point[i].Realdata.Vb.value);
				voltage_result_deal(result,&point[i].Result_m.tjUb,point[i].Realdata.Vb.value);
				cmp_max(point[i].Realdata.Vb.value,&point[i].Result.tjUb.max,point[i].Result.tjUb.max_time);
				cmp_max(point[i].Realdata.Vb.value,&point[i].Result_m.tjUb.max,point[i].Result_m.tjUb.max_time);
				cmp_min(point[i].Realdata.Vb.value,&point[i].Result.tjUb.min,point[i].Result.tjUb.min_time);
				cmp_min(point[i].Realdata.Vb.value,&point[i].Result_m.tjUb.min,point[i].Result_m.tjUb.min_time);

				point[i].Result.tjUb.ok_count=point[i].Result.tjUb.U_Count-(point[i].Result.tjUb.x_count+point[i].Result.tjUb.xx_count+
											point[i].Result.tjUb.ss_count+point[i].Result.tjUb.s_count);
				point[i].Result.tjUb.U_Avg = (point[i].Result.tjUb.U_Sum/point[i].Result.tjUb.U_Count);//*U_COEF;

				//计算电压日越上限率，下限率，合格率
				Rate = (((FP32)point[i].Result.tjUb.ss_count+(FP32)point[i].Result.tjUb.s_count)/(FP32)point[i].Result.tjUb.U_Count)*RAT_COEF;
				point[i].Result.tjUb.s_Rate = Rate>100?100:Rate;
				Rate = (((FP32)point[i].Result.tjUb.xx_count+(FP32)point[i].Result.tjUb.x_count)/(FP32)point[i].Result.tjUb.U_Count)*RAT_COEF;
				point[i].Result.tjUb.x_Rate = Rate>100?100:Rate;
				point[i].Result.tjUb.ok_Rate = 100.00-point[i].Result.tjUb.x_Rate-point[i].Result.tjUb.s_Rate;
				point[i].Result_m.tjUb.ok_count=point[i].Result_m.tjUb.U_Count-(point[i].Result_m.tjUb.x_count+point[i].Result_m.tjUb.xx_count+
					point[i].Result_m.tjUb.ss_count+point[i].Result_m.tjUb.s_count);
				point[i].Result_m.tjUb.U_Avg =(point[i].Result_m.tjUb.U_Sum/point[i].Result_m.tjUb.U_Count);//*U_COEF;

				if(point[i].Result_m.tjUb.U_Count == 0)
				{
					fprintf(stderr,"point[i].Result_m.tjUb.U_Count == 0,pointNo = %d\r\n",point[i].PointNo);

				}
				//计算电压月越上限率，下限率，合格率
				Rate = (((FP32)point[i].Result_m.tjUb.ss_count+(FP32)point[i].Result_m.tjUb.s_count)/(FP32)point[i].Result_m.tjUb.U_Count)*RAT_COEF;
				point[i].Result_m.tjUb.s_Rate = Rate>100?100:Rate;
				Rate = (((FP32)point[i].Result_m.tjUb.xx_count+(FP32)point[i].Result_m.tjUb.x_count)/(FP32)point[i].Result_m.tjUb.U_Count)*RAT_COEF;
				point[i].Result_m.tjUb.x_Rate = Rate>100?100:Rate;
				point[i].Result_m.tjUb.ok_Rate = 100.00-point[i].Result_m.tjUb.x_Rate-point[i].Result_m.tjUb.s_Rate;

				JProgramInfo->StatisticsPoint[i].PointNo = point[i].PointNo;
				CpPubdata_U(point[i].Result.tjUb,&JProgramInfo->StatisticsPoint[i].DayResu.tjUb);
				CpPubdata_U(point[i].Result_m.tjUb,&JProgramInfo->StatisticsPoint[i].MonthResu.tjUb);

			}
			else
			{
				memset((INT8U*)&JProgramInfo->StatisticsPoint[i].DayResu.tjUb,0xee,sizeof(JProgramInfo->StatisticsPoint[i].DayResu.tjUb));
				memset((INT8U*)&JProgramInfo->StatisticsPoint[i].MonthResu.tjUb,0xee,sizeof(JProgramInfo->StatisticsPoint[i].MonthResu.tjUb));
			}
		}
		//code 2

		if(point[i].Realdata.Vc.Available)
		{
			result = cmp_limit(point[i].Realdata.Vc.value,obj_offset.uUp_Kaohe,
					obj_offset.uUp,obj_offset.uDown,
					obj_offset.uDown_Kaohe);

//			voltage_result_deal(result,&point[i].Result.tjUc,point[i].Realdata.Vc.value/U_COEF);
//			voltage_result_deal(result,&point[i].Result_m.tjUc,point[i].Realdata.Vc.value/U_COEF);
			voltage_result_deal(result,&point[i].Result.tjUc,point[i].Realdata.Vc.value);
			voltage_result_deal(result,&point[i].Result_m.tjUc,point[i].Realdata.Vc.value);
			cmp_max(point[i].Realdata.Vc.value,&point[i].Result.tjUc.max,point[i].Result.tjUc.max_time);
			cmp_min(point[i].Realdata.Vc.value,&point[i].Result.tjUc.min,point[i].Result.tjUc.min_time);

			cmp_max(point[i].Realdata.Vc.value,&point[i].Result_m.tjUc.max,point[i].Result_m.tjUc.max_time);
			cmp_min(point[i].Realdata.Vc.value,&point[i].Result_m.tjUc.min,point[i].Result_m.tjUc.min_time);

			point[i].Result.tjUc.ok_count=point[i].Result.tjUc.U_Count-(point[i].Result.tjUc.x_count+point[i].Result.tjUc.xx_count+
				point[i].Result.tjUc.ss_count+point[i].Result.tjUc.s_count);
			point[i].Result.tjUc.U_Avg = (point[i].Result.tjUc.U_Sum/point[i].Result.tjUc.U_Count);//*U_COEF;


			//计算电压日越上限率，下限率，合格率
			Rate = (((FP32)point[i].Result.tjUc.ss_count+(FP32)point[i].Result.tjUc.s_count)/(FP32)point[i].Result.tjUc.U_Count)*RAT_COEF;
			point[i].Result.tjUc.s_Rate = Rate>100?100:Rate;
			Rate = (((FP32)point[i].Result.tjUc.xx_count+(FP32)point[i].Result.tjUc.x_count)/(FP32)point[i].Result.tjUc.U_Count)*RAT_COEF;
			point[i].Result.tjUc.x_Rate = Rate>100?100:Rate;
			point[i].Result.tjUc.ok_Rate = 100.00-point[i].Result.tjUc.x_Rate-point[i].Result.tjUc.s_Rate;


			point[i].Result_m.tjUc.ok_count=point[i].Result_m.tjUc.U_Count-(point[i].Result_m.tjUc.x_count+point[i].Result_m.tjUc.xx_count+
				 point[i].Result_m.tjUc.ss_count+point[i].Result_m.tjUc.s_count);
			point[i].Result_m.tjUc.U_Avg = (point[i].Result_m.tjUc.U_Sum/point[i].Result_m.tjUc.U_Count);//*U_COEF;

			Rate = (((FP32)point[i].Result_m.tjUc.ss_count+(FP32)point[i].Result_m.tjUc.s_count)/(FP32)point[i].Result_m.tjUc.U_Count)*RAT_COEF;
			point[i].Result_m.tjUc.s_Rate = Rate>100?100:Rate;
			Rate = (((FP32)point[i].Result_m.tjUc.xx_count+(FP32)point[i].Result_m.tjUc.x_count)/(FP32)point[i].Result_m.tjUc.U_Count)*RAT_COEF;
			point[i].Result_m.tjUc.x_Rate = Rate>100?100:Rate;
			point[i].Result_m.tjUc.ok_Rate = 100.00-point[i].Result_m.tjUc.x_Rate-point[i].Result_m.tjUc.s_Rate;

			JProgramInfo->StatisticsPoint[i].PointNo = point[i].PointNo;
			CpPubdata_U(point[i].Result.tjUc,&JProgramInfo->StatisticsPoint[i].DayResu.tjUc);
			CpPubdata_U(point[i].Result_m.tjUc,&JProgramInfo->StatisticsPoint[i].MonthResu.tjUc);
		}
		else
		{
			memset((INT8U*)&JProgramInfo->StatisticsPoint[i].DayResu.tjUc,0xee,sizeof(JProgramInfo->StatisticsPoint[i].DayResu.tjUc));
			memset((INT8U*)&JProgramInfo->StatisticsPoint[i].MonthResu.tjUc,0xee,sizeof(JProgramInfo->StatisticsPoint[i].MonthResu.tjUc));
		}
	}
}

/*
 * 统计主线程
 */
void calc_thread()
{
	INT8U valid = 0;
    while(1){
		/*根据系统参数确定测量点信息，内容保存到point相关处*/
		get_point_para(&point[0]);
		#ifdef SHANDONG
			valid = getcalcvalid();
		#else
			valid = 1;
		#endif
	    if(valid == 1){
			voltage_calc();
			saveCoverClass(0x4030,0,&JProgramInfo->StatisticsPoint,sizeof(JProgramInfo->StatisticsPoint),calc_voltage_save);
	    }
	    usleep(100*1000);
  }
  pthread_detach(pthread_self());
  pthread_exit(&thread_calc);
}

/*
 * 统计主函数
 */
void calc_proccess()
{
	pthread_attr_init(&calc_attr_t);
	pthread_attr_setstacksize(&calc_attr_t,2048*1024);
	pthread_attr_setdetachstate(&calc_attr_t,PTHREAD_CREATE_DETACHED);
	while ((thread_calc_id=pthread_create(&thread_calc, &calc_attr_t, (void*)calc_thread, NULL)) != 0)
	{
		sleep(1);
	}
}


