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
#include <stdarg.h>
#include <sys/stat.h>
#include "calc.h"
#include "cjdeal.h"
#include "cjsave.h"
#include "ParaDef.h"
#include "AccessFun.h"
#include "Objectdef.h"
#include "PublicFunction.h"
#include "event.h"
#include "EventObject.h"
#include "CalcObject.h"
#include "dlt698.h"

CLASS_4030 obj_offset={};
PassRate_U  passu_d[3],passu_m[3];
Gongdian_tj gongdian_tj;
Max_ptongji max_ptongji[MAXNUM_IMPORTANTUSR_CALC];
extern ProgramInfo* JProgramInfo;
extern INT8U poweroffon_state;
extern MeterPower MeterPowerInfo[POWEROFFON_NUM];


///*
// * 根据任务的CSD来判断是否是该任务数据
// * */
//int isOADByCSD(OAD freezeOAD,OAD relateOAD,CSD_ARRAYTYPE csds)
//{
////	static OI_698 VoltTj_OI[3]={0x2131,0x2132,0x2133};	//A相、B相、C相电压合格率
//	int i=0,j=0,k=0;
//	for(i=0;i<csds.num;i++)	{
//		if(csds.csd[i].type == 1) {	//ROAD
//			if(memcmp(&csds.csd[i].csd.road.oad,&freezeOAD,sizeof(OAD))==0) {
//				for(j=0;j<csds.csd[i].csd.road.num;j++)	{
//					if(memcmp(&csds.csd[i].csd.road.oad,&relateOAD,sizeof(OAD))==0) {
//						return 1;
//					}
//				}
//			}
//		}
//	}
//	return 0;
//}

/*
 * 根据任务号及采集方案的array CSD存储相关数据
 * */
int saveTerminalTaskData(INT8U taskid,TS savets,TSA tsa,CSD_ARRAYTYPE csds)
{
	INT8U	saveBuf[255]={};
	int		index=0,buflen=0;
	OAD 	freezeOAD={},relateOAD={};
	DateTimeBCD		freezetime={};
	INT8U	kk=0;
	int 	i=0,j=0;
	int		saveret=0;

	for(i=0;i<csds.num;i++)	{
		if(csds.csd[i].type == 1) {	//ROAD
			freezeOAD = csds.csd[i].csd.road.oad;
			for(j=0;j<csds.csd[i].csd.road.num;j++)	{
				index = 0;
				saveBuf[index++] = dttsa;	//TSA标识
				memcpy(&saveBuf[index],&tsa,sizeof(TSA));
				index += sizeof(TSA);
				relateOAD = csds.csd[i].csd.road.oads[j];
				asyslog(LOG_NOTICE,"freezeOAD =%04x relateOAD = %04x\n",freezeOAD.OI,relateOAD.OI);

				switch(relateOAD.OI) {
				case 0x2021:	//数据冻结时间
					TsToTimeBCD(savets,&freezetime);
					index += fill_date_time_s(&saveBuf[index],&freezetime);
					break;
				case 0x2131:
				case 0x2132:
				case 0x2133:
					kk = relateOAD.OI - 0x2131;
					asyslog(LOG_NOTICE,"kk = %d\n",kk);
					if(freezeOAD.OI==0x5004) {
						fill_variClass(relateOAD,1,(INT8U *)&passu_d[kk],&saveBuf[index],&buflen);
						index += buflen;
					}else if(freezeOAD.OI==0x5006) {
						fill_variClass(relateOAD,1,(INT8U *)&passu_m[kk],&saveBuf[index],&buflen);
						index += buflen;
					}
					break;
				default:
					fill_variClass(relateOAD,1,NULL,&saveBuf[index],&buflen);
					break;
				}
				asyslog(LOG_NOTICE,"saveOADData index = %d \n",index);
				saveret = SaveOADData(taskid,freezeOAD,relateOAD,saveBuf,index,savets);
			}
		}
	}
	return saveret;
}
/*
 * 按照任务进行冻结数据的存储
 * oi，attr		存储的OI及属性
 * savets		存储时标
 * savelen		需要存储数据长度
 * data			数据内容
 * */
void terminalTaskFreeze(TS savets)
{
	int tsa_num = 0;
	int	tsaid=0;
	INT8U	fanganid=0,taskid=0;
	CLASS_6015	class6015={};
	CLASS_6013	class6013={};
	CLASS_6001 *tsa_group = NULL;

	for(fanganid=0;fanganid<255;fanganid++) {
		if (readCoverClass(0x6015, fanganid, &class6015, sizeof(CLASS_6015), coll_para_save)== 1)	{
			asyslog(LOG_NOTICE,"fanganid = %d\n",fanganid);
			//任务中有相关的电压合格率csds的配置
//			if(isOADByCSD(freezeOAD,relateOAD,class6015.csds)==1) {
				//根据相关的方案的MS类型获取测量点信息
				tsa_num = getOI6001(class6015.mst,(INT8U **)&tsa_group);
				asyslog(LOG_NOTICE,"tsa_num = %d\n",tsa_num);
				for(tsaid=0;tsaid<tsa_num;tsaid++) {
					if(tsa_group[tsaid].basicinfo.port.OI == PORT_JC) {
						//满足交采测量点的通过方案号来获取任务号
						for(taskid=0;taskid<255;taskid++) {
							if (readCoverClass(0x6013, taskid, &class6013, sizeof(CLASS_6013), coll_para_save)== 1)	{
								if(class6013.sernum == fanganid) {
									//查找到满足CSD的数据任务，进行相关任务数据存储
									saveTerminalTaskData(taskid,savets,tsa_group[tsaid].basicinfo.addr,class6015.csds);
								}
							}
						}
					}
				}
//			}
		}
	}
	if(tsa_group != NULL) {
		tsa_group = NULL;
		free(tsa_group);
	}
}
/*
 * flag 0:日 1：月
 * sign 0:清零 1 增加
 */
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


        //月
			passu_m_tmp->monitorTime +=per_min;
			INT32U m_over=passu_m_tmp->upLimitTime+passu_m_tmp->downLimitTime;
			FP32 m_over_per=(FP32)m_over/(FP32)passu_d_tmp->monitorTime;
			FP32 m_hege_per=1-m_over_per;
			passu_m_tmp->overRate=m_over_per*100*100;
			passu_m_tmp->passRate=m_hege_per*100*100;

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
	static INT8U lastchgoi4030=0,addnum=0,lastreset=0;

	addnum++;

	if(first==1) {
		first = 0;
		currtime = time(NULL);
		nexttime = time(NULL);
		memset(&gongdian_tj,0,sizeof(Gongdian_tj));
		readVariData(0x2203,0,&gongdian_tj,sizeof(Gongdian_tj));

		syslog(LOG_NOTICE,"firset read day_tj=%d,mon_tj=%d\n",gongdian_tj.gongdian.day_tj,gongdian_tj.gongdian.month_tj);

		lastchgoi4030 = JProgramInfo->oi_changed.oi4030;
	    readCoverClass(0x4030,0,&obj_offset,sizeof(obj_offset),para_vari_save);
        memset(passu_d,0,sizeof(passu_d));
        memset(passu_m,0,sizeof(passu_m));
	}
	if(lastchgoi4030!=JProgramInfo->oi_changed.oi4030){
		readCoverClass(0x4030,0,&obj_offset,sizeof(obj_offset),para_vari_save);
			lastchgoi4030=JProgramInfo->oi_changed.oi4030;
	}
	currtime = time(NULL);
	int tcha=abs(currtime - nexttime);
	if(tcha >= 60) {
		int per_min=tcha/60;
		nexttime = currtime;
		Vol_Rate_Tj(&passu_d[0],&passu_m[0],JProgramInfo->ACSRealData.Ua,per_min);
		Vol_Rate_Tj(&passu_d[1],&passu_m[1],JProgramInfo->ACSRealData.Ub,per_min);
		Vol_Rate_Tj(&passu_d[2],&passu_m[2],JProgramInfo->ACSRealData.Uc,per_min);
	}
	gongdian_tj.gongdian.day_tj +=1;
	gongdian_tj.gongdian.month_tj +=1;
	if(addnum >= 5 || lastreset!=JProgramInfo->oi_changed.reset) {
//		syslog(LOG_NOTICE,"addnum=%d lastreset=%d reset=%d day_tj=%d,mon_tj=%d\n",addnum,lastreset,JProgramInfo->oi_changed.reset,gongdian_tj.gongdian.day_tj,gongdian_tj.gongdian.month_tj);
		addnum=0;
		lastreset=JProgramInfo->oi_changed.reset;
		saveVariData(0x2203,0,&gongdian_tj,sizeof(Gongdian_tj));	//TODO：
	}
//	syslog(LOG_NOTICE,"day_tj=%d,mon_tj=%d\n",gongdian_tj.gongdian.day_tj,gongdian_tj.gongdian.month_tj);
}

/*
 * 终端统计数据及计量数据冻结
 * */
void TerminalFreeze()
{
	static TS 		newts,oldts;
	static INT8U 	first=1;
	int	j=0;

	if(first==1) {
		first = 0;
		TSGet(&oldts);
	}
	TSGet(&newts);
	if(newts.Day !=oldts.Day) {
		Save_TJ_Freeze(0,0x2203,0x0200,newts,sizeof(gongdian_tj.gongdian),(INT8U *)&gongdian_tj.gongdian);
		gongdian_tj.gongdian.day_tj = 0;
		Save_Task_Freeze(newts);
		for(j=0;j<3;j++) {
			Save_TJ_Freeze(0,(0x2131+j),0x0201,newts,sizeof(PassRate_U),(INT8U *)&passu_d[j]);
			memset(&passu_d[j],0,sizeof(PassRate_U));
		}
		gongdian_tj.gongdian.day_tj = 0;
		if(newts.Month !=oldts.Month && newts.Hour == 0) {
			Save_Task_Freeze(newts);
			Save_TJ_Freeze(1,0x2203,0x0200,newts,sizeof(gongdian_tj.gongdian),(INT8U *)&gongdian_tj.gongdian);
			for(j=0;j<3;j++) {
				Save_TJ_Freeze(1,(0x2131+j),0x0202,newts,sizeof(PassRate_U),(INT8U *)&passu_m[j]);
				memset(&passu_m[j],0,sizeof(PassRate_U));
			}
			gongdian_tj.gongdian.month_tj = 0;
		}else{
			gongdian_tj.gongdian.month_tj +=1;
		}
		TSGet(&oldts);
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
    	//计量数据及统计数据冻结存储
    	TerminalFreeze();
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


