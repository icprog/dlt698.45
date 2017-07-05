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
#include "PublicFunction.h"
#include "event.h"
#include "EventObject.h"
#include "CalcObject.h"

CLASS_4030 obj_offset={};
PassRate_U  passu_d[3],passu_m[3];
Gongdian_tj gongdian_tj;
Max_ptongji max_ptongji[MAXNUM_IMPORTANTUSR_CALC];
extern ProgramInfo* JProgramInfo;
extern INT8U poweroffon_state;
extern MeterPower MeterPowerInfo[POWEROFFON_NUM];

void Vol_Rate_Tj(PassRate_U *passu_d_tmp,PassRate_U *passu_m_tmp,INT32U voltage,int per_min)
{
	if(JProgramInfo->ACSRealData.Available){
		//日
		passu_d_tmp->monitorTime +=per_min;
		if(voltage>=obj_offset.uUp && obj_offset.uUp>0){
			passu_d_tmp->upLimitTime +=per_min;
			passu_m_tmp->upLimitTime +=per_min;
		}
		if(voltage<=obj_offset.uDown && obj_offset.uDown>0){
			passu_d_tmp->downLimitTime +=per_min;
			passu_m_tmp->downLimitTime +=per_min;
		}
		INT32U d_over=passu_d_tmp->upLimitTime+passu_d_tmp->downLimitTime;
		FP32 d_over_per=(FP32)d_over/(FP32)passu_d_tmp->monitorTime;
		FP32 d_hege_per=1-d_over_per;
		passu_d_tmp->overRate=d_over_per*100*100;
		passu_d_tmp->passRate=d_hege_per*100*100;
//		fprintf(stderr,"passu_d_tmp:monitorTime:%d downLimitTime:%d overRate:%d passRate:%d upLimitTime:%d\n",
//				passu_d_tmp->monitorTime,
//				passu_d_tmp->downLimitTime,passu_d_tmp->overRate,
//				passu_d_tmp->passRate,passu_d_tmp->upLimitTime);

        //月
		passu_m_tmp->monitorTime +=per_min;
		INT32U m_over=passu_m_tmp->upLimitTime+passu_m_tmp->downLimitTime;
		FP32 m_over_per=(FP32)m_over/(FP32)passu_d_tmp->monitorTime;
		FP32 m_hege_per=1-m_over_per;
		passu_m_tmp->overRate=m_over_per*100*100;
		passu_m_tmp->passRate=m_hege_per*100*100;
//		fprintf(stderr,"passu_m_tmp:monitorTime:%d downLimitTime:%d overRate:%d passRate:%d upLimitTime:%d\n",passu_m_tmp->monitorTime,
//						passu_m_tmp->downLimitTime,passu_m_tmp->overRate,
//						passu_m_tmp->passRate,passu_m_tmp->upLimitTime);
	}
}


#if 0
void Save_Vol_Rate(INT8U flag,DateTimeBCD datetime)
{
   if(flag == 0){
      FreezeObject obj_5004;
      memset(&obj_5004,0,sizeof(FreezeObject));
      readCoverClass(0x5004,0,&obj_5004,sizeof(FreezeObject),para_vari_save);
      INT8U i=0,j=0;
      OAD oad;
      for(j=0;j<3;j++){
    	 oad.OI = 0x2131+j;
		 oad.attflg = 0x02;
		 oad.attrindex = 0x01;
    	  for(i=0;i<obj_5004.RelateNum;i++){
			  if(memcmp(&oad,&obj_5004.RelateObj[i].oad,sizeof(OAD))==0){
				  int ret=saveFreezeRecord(0x5004,oad,datetime,sizeof(PassRate_U),(INT8U *)&passu_d[j]);
//				  fprintf(stderr,"ret=%d oad=%04x %02x %02x  \n",ret,oad.OI,oad.attflg,oad.attrindex);
//				  fprintf(stderr,"passu_d[%d]:%d %d %d %d %d \n",j,passu_d[j].monitorTime,passu_d[j].downLimitTime,passu_d[j].overRate,passu_d[j].passRate,passu_d[j].upLimitTime);
				  memset(&passu_d[j],0,sizeof(PassRate_U));
			  }
			}
      }
   }else{
	   FreezeObject obj_5006;
		 memset(&obj_5006,0,sizeof(FreezeObject));
		 readCoverClass(0x5006,0,&obj_5006,sizeof(FreezeObject),para_vari_save);
		 INT8U i=0,j=0;
		 OAD oad;
		 for(j=0;j<3;j++){
		 oad.OI = 0x2131+j;
		 oad.attflg = 0x02;
		 oad.attrindex = 0x02;
		  for(i=0;i<obj_5006.RelateNum;i++){
			  if(memcmp(&oad,&obj_5006.RelateObj[i].oad,sizeof(OAD))==0){
				 int ret=saveFreezeRecord(0x5006,oad,datetime,sizeof(PassRate_U),(INT8U *)&passu_m[j]);
//				  fprintf(stderr,"ret=%d oad=%04x %02x %02x \n",ret,oad.OI,oad.attflg,oad.attrindex);
//				  fprintf(stderr,"passu_m[%d]:%d %d %d %d %d \n",j,passu_m[j].monitorTime,passu_m[j].downLimitTime,passu_m[j].overRate,passu_m[j].passRate,passu_m[j].upLimitTime);
				  memset(&passu_m[j],0,sizeof(PassRate_U));
			  }
			}
		 }
   }
}
#endif
void Calc_Tj()
{
	static time_t	currtime=0,nexttime=0;
	static INT8U 	first=1;
	static INT8U lastchgoi4030=0;
	int		j=0;
	TS 		newts;

	TSGet(&newts);
	if(first==1) {
		first = 0;
		currtime = time(NULL);
		nexttime = time(NULL);
		memset(&gongdian_tj,0,sizeof(Gongdian_tj));
		readVariData(0x2203,0,&gongdian_tj,sizeof(Gongdian_tj));
//		fprintf(stderr,"初始化: 2203:时间: %d-%d-%d %d:%d:%d\n",gongdian_tj.ts.Year,gongdian_tj.ts.Month,gongdian_tj.ts.Day,gongdian_tj.ts.Hour,gongdian_tj.ts.Minute,gongdian_tj.ts.Sec);
//		fprintf(stderr,"初始化: 日供电时间=%d   月供电时间=%d\n",gongdian_tj.gongdian.day_tj,gongdian_tj.gongdian.month_tj);
		lastchgoi4030 = JProgramInfo->oi_changed.oi4030;
	    readCoverClass(0x4030,0,&obj_offset,sizeof(obj_offset),para_vari_save);
//	    fprintf(stderr,"4030参数初始化:udown=%d udownkaohe=%d up=%d upkaohe=%d \n",obj_offset.uDown,obj_offset.uDown_Kaohe,obj_offset.uUp,obj_offset.uUp_Kaohe);
        memset(passu_d,0,sizeof(passu_d));
        memset(passu_m,0,sizeof(passu_m));
	}
	if(lastchgoi4030!=JProgramInfo->oi_changed.oi4030){
		readCoverClass(0x4030,0,&obj_offset,sizeof(obj_offset),para_vari_save);
			lastchgoi4030=JProgramInfo->oi_changed.oi4030;
//			fprintf(stderr,"4030参数变更:udown=%d udownkaohe=%d up=%d upkaohe=%d \n",obj_offset.uDown,obj_offset.uDown_Kaohe,obj_offset.uUp,obj_offset.uUp_Kaohe);
	}
	currtime = time(NULL);
	int tcha=abs(currtime - nexttime);
	if(tcha >= 60) {
		int per_min=tcha/60;
		nexttime = currtime;
		gongdian_tj.gongdian.day_tj +=1;
		gongdian_tj.gongdian.month_tj +=1;
		Vol_Rate_Tj(&passu_d[0],&passu_m[0],JProgramInfo->ACSRealData.Ua,per_min);
		Vol_Rate_Tj(&passu_d[1],&passu_m[1],JProgramInfo->ACSRealData.Ub,per_min);
		Vol_Rate_Tj(&passu_d[2],&passu_m[2],JProgramInfo->ACSRealData.Uc,per_min);
		//存储供电时间
//		fprintf(stderr,"2203:时间: %d-%d-%d %d:%d:%d\n",gongdian_tj.ts.Year,gongdian_tj.ts.Month,gongdian_tj.ts.Day,gongdian_tj.ts.Hour,gongdian_tj.ts.Minute,gongdian_tj.ts.Sec);
//		fprintf(stderr," 日供电时间=%d\n 月供电时间=%d\n",gongdian_tj.gongdian.day_tj,gongdian_tj.gongdian.month_tj);
		saveVariData(0x2203,0,&gongdian_tj,sizeof(Gongdian_tj));	//TODO：现场运行 是否需要 1分钟保存一次
	}

	if(newts.Day != gongdian_tj.ts.Day) {
//		fprintf(stderr,"2203:newts: %d-%d-%d %d:%d:%d\n",newts.Year,newts.Month,newts.Day,newts.Hour,newts.Minute,newts.Sec);
//		fprintf(stderr,"跨日	:时间: %d-%d-%d %d:%d:%d\n",gongdian_tj.ts.Year,gongdian_tj.ts.Month,gongdian_tj.ts.Day,gongdian_tj.ts.Hour,gongdian_tj.ts.Minute,gongdian_tj.ts.Sec);
		gongdian_tj.gongdian.day_tj = 0;
		//TSGet(&gongdian_tj.ts);
//		Save_Vol_Rate(0,datetime);
		Save_TJ_Freeze(0,0x2203,0x0200,newts,sizeof(gongdian_tj.gongdian),(INT8U *)&gongdian_tj.gongdian);
		for(j=0;j<3;j++) {
			Save_TJ_Freeze(0,(0x2131+j),0x0201,newts,sizeof(PassRate_U),(INT8U *)&passu_d[j]);
			memset(&passu_d[j],0,sizeof(PassRate_U));
		}
		if(newts.Month != gongdian_tj.ts.Month && newts.Hour == 0) {
//			gongdian_tj.gongdian.month_tj = 0;
			fprintf(stderr,"跨月");
//			Save_Vol_Rate(1,datetime);
			Save_TJ_Freeze(1,0x2203,0x0200,newts,sizeof(gongdian_tj.gongdian),(INT8U *)&gongdian_tj.gongdian);
			for(j=0;j<3;j++) {
				Save_TJ_Freeze(1,(0x2131+j),0x0202,newts,sizeof(PassRate_U),(INT8U *)&passu_m[j]);
				memset(&passu_m[j],0,sizeof(PassRate_U));
			}
		}
		TSGet(&gongdian_tj.ts);
	}
}
/*
 * 统计主线程
 */
void calc_thread()
{
	time_t	currtime=0,nexttime=0;
	INT8U valid = 0;
	TS oldts,newts;
	static INT8U	init_chg=0;
	TSGet(&oldts);
	TSGet(&newts);
	currtime = time(NULL);
	nexttime = time(NULL);

	init_chg = JProgramInfo->oi_changed.init;
	while(1){
    	if(JProgramInfo->oi_changed.init != init_chg) {
    		init_chg = JProgramInfo->oi_changed.init;
    		TSGet(&gongdian_tj.ts);
    		gongdian_tj.gongdian.day_tj = 0;
    		gongdian_tj.gongdian.month_tj = 0;
    	}
    	//供电时间、电压合格率统计
    	Calc_Tj();
    	valid++;
    	TSGet(&newts);
		//判断停上电
		if(valid>5)//等待5秒
			Event_3106(JProgramInfo,MeterPowerInfo,&poweroffon_state);
	    usleep(1000*1000);
  }
  pthread_detach(pthread_self());
  pthread_exit(&thread_calc);
}

INT8U Init_Para(){

	return 1;
}
/*
 * 统计主函数
 */
void calc_proccess()
{
	Init_Para();
	pthread_attr_init(&calc_attr_t);
	pthread_attr_setstacksize(&calc_attr_t,2048*1024);
	pthread_attr_setdetachstate(&calc_attr_t,PTHREAD_CREATE_DETACHED);
	while ((thread_calc_id=pthread_create(&thread_calc, &calc_attr_t, (void*)calc_thread, NULL)) != 0)
	{
		sleep(1);
	}
}


