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
#include "basedef.h"
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

int savePulseTaskData(INT8U pluse_index,INT8U taskid,TS savets,TSA tsa,CSD_ARRAYTYPE csds)
{
	INT8U	saveBuf[255]={};
	int		index=0;
	INT16U	buflen=0;
	OAD 	freezeOAD={},relateOAD={};
	DateTimeBCD		freezetime={};
	int 	i=0,j=0;
	int		saveret=0;

	for(i=0;i<csds.num;i++)	{
		if(csds.csd[i].type == 0) {	//oad
			index = 0;
			saveBuf[index++] = dttsa;	//TSA标识
			memcpy(&saveBuf[index],&tsa,sizeof(TSA));
			index += sizeof(TSA);
//			asyslog(LOG_NOTICE,"save TSA_len=%d oad=%04x_%02x%02x\n",index,
//					csds.csd[i].csd.oad.OI,csds.csd[i].csd.oad.attflg,csds.csd[i].csd.oad.attrindex);
			fill_pulseEnergy(JProgramInfo->cfg_para.device,pluse_index,csds.csd[i].csd.oad,&saveBuf[index],&buflen);
			index += buflen;
			asyslog(LOG_NOTICE,"savePulseTaskData taskid = %d OAD=%04x_%02x%02x index=%d \n",taskid,csds.csd[i].csd.oad.OI,
					csds.csd[i].csd.oad.attflg,csds.csd[i].csd.oad.attrindex,index);
			memset(&freezeOAD,0,sizeof(OAD));
			saveret = SaveOADData(taskid,freezeOAD,csds.csd[i].csd.oad,saveBuf,index,savets);
		}
		if(csds.csd[i].type == 1) {	//ROAD
			freezeOAD = csds.csd[i].csd.road.oad;
			for(j=0;j<csds.csd[i].csd.road.num;j++)	{
				index = 0;
				saveBuf[index++] = dttsa;	//TSA标识
				memcpy(&saveBuf[index],&tsa,sizeof(TSA));
				index += sizeof(TSA);
				relateOAD = csds.csd[i].csd.road.oads[j];
				asyslog(LOG_NOTICE,"save TSA=%d freezeOAD=%04x_%02x%02x relateOAD=%04x_%02x%02x\n",index,
						freezeOAD.OI,freezeOAD.attflg,freezeOAD.attrindex,relateOAD.OI,relateOAD.attflg,relateOAD.attrindex);
				switch(relateOAD.OI) {
				case 0x2021:	//数据冻结时间
					TsToTimeBCD(savets,&freezetime);
					index += fill_date_time_s(&saveBuf[index],&freezetime);
					break;
				default:
					fill_pulseEnergy(JProgramInfo->cfg_para.device,pluse_index,relateOAD,&saveBuf[index],&buflen);
					index += buflen;
					break;
				}
				asyslog(LOG_NOTICE,"savePulseTaskData taskid = %d freezeOAD=%04x relateOAD=%04x index=%d\n",taskid,freezeOAD.OI,relateOAD.OI,index);
				saveret = SaveOADData(taskid,freezeOAD,relateOAD,saveBuf,index,savets);
			}
		}
	}
	return saveret;
}
/*
 * 根据任务号及采集方案的array CSD存储相关数据
 * */
int saveTerminalTaskData(INT8U taskid,TS savets,TSA tsa,CSD_ARRAYTYPE csds)
{
	INT8U	saveBuf[255]={};
	int		index=0;
	INT16U	buflen=0;
	OAD 	freezeOAD={},relateOAD={};
	DateTimeBCD		freezetime={};
	int 	i=0,j=0;
	int		saveret=0;
	Volt_PassRate_tj	volt_tj={};

	for(i=0;i<csds.num;i++)	{
		if(csds.csd[i].type == 0) {	//oad
			index = 0;
			saveBuf[index++] = dttsa;	//TSA标识
			memcpy(&saveBuf[index],&tsa,sizeof(TSA));
			index += sizeof(TSA);
			fill_variClass(csds.csd[i].csd.oad,1,NULL,&saveBuf[index],&buflen,JProgramInfo);
			index += buflen;
			asyslog(LOG_NOTICE,"saveTerminalTaskData taskid = %d OAD=%04x index=%d\n",taskid,csds.csd[i].csd.oad.OI,index);
			memset(&freezeOAD,0,sizeof(OAD));
			saveret = SaveOADData(taskid,freezeOAD,csds.csd[i].csd.oad,saveBuf,index,savets);
		}
		if(csds.csd[i].type == 1) {	//ROAD
			freezeOAD = csds.csd[i].csd.road.oad;
			for(j=0;j<csds.csd[i].csd.road.num;j++)	{
				index = 0;
				saveBuf[index++] = dttsa;	//TSA标识
				memcpy(&saveBuf[index],&tsa,sizeof(TSA));
				index += sizeof(TSA);
				relateOAD = csds.csd[i].csd.road.oads[j];
				switch(relateOAD.OI) {
				case 0x2021:	//数据冻结时间
					TsToTimeBCD(savets,&freezetime);
					index += fill_date_time_s(&saveBuf[index],&freezetime);
					break;
				case 0x2131:
					readVariData(relateOAD.OI,0,&volt_tj,sizeof(Volt_PassRate_tj));
					if(freezeOAD.OI==0x5004) {
						fill_variClass(relateOAD,1,(INT8U *)&volt_tj.dayu_tj,&saveBuf[index],&buflen,JProgramInfo);
						index += buflen;
					}else if(freezeOAD.OI==0x5006) {
						fill_variClass(relateOAD,1,(INT8U *)&volt_tj.monthu_tj,&saveBuf[index],&buflen,JProgramInfo);
						index += buflen;
					}
					break;
				case 0x2132:
				case 0x2133:
					//山东II型集中器电压合格率要求B、C相上送NULL
					if(JProgramInfo->cfg_para.device != CCTT2) {
						readVariData(relateOAD.OI,0,&volt_tj,sizeof(Volt_PassRate_tj));
						if(freezeOAD.OI==0x5004) {
							fill_variClass(relateOAD,1,(INT8U *)&volt_tj.dayu_tj,&saveBuf[index],&buflen,JProgramInfo);
							index += buflen;
						}else if(freezeOAD.OI==0x5006) {
							fill_variClass(relateOAD,1,(INT8U *)&volt_tj.monthu_tj,&saveBuf[index],&buflen,JProgramInfo);
							index += buflen;
						}
					}
					break;
				default:
					fill_variClass(relateOAD,1,NULL,&saveBuf[index],&buflen,JProgramInfo);
					index += buflen;
					break;
				}
				asyslog(LOG_NOTICE,"saveTerminalTaskData taskid = %d freezeOAD=%04x relateOAD=%04x index=%d\n",taskid,freezeOAD.OI,relateOAD.OI,index);
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
void terminalTaskFreeze(INT8U taskid,INT8U fanganid)
{
	TS	savets;
	int tsa_num = 0;
	int	meterid=0;
	INT8S	pluseindex=0;
	CLASS_6015	class6015={};
//	CLASS_6013	class6013={};
	CLASS_6001 *tsa_group = NULL;

	TSGet(&savets);
	if (readCoverClass(0x6015, fanganid, &class6015, sizeof(CLASS_6015), coll_para_save)== 1)	{
		asyslog(LOG_NOTICE,"fanganid = %d\n",fanganid);
		//根据相关的方案的MS类型获取测量点信息
		tsa_num = getOI6001(class6015.mst,(INT8U **)&tsa_group);
		asyslog(LOG_NOTICE,"tsa_num = %d\n",tsa_num);
		for(meterid=0;meterid<tsa_num;meterid++) {
			if(tsa_group[meterid].basicinfo.port.OI == PORT_JC) {
				//满足交采测量点,查找到满足CSD的数据任务，进行相关任务数据存储
				saveTerminalTaskData(taskid,savets,tsa_group[meterid].basicinfo.addr,class6015.csds);
			}
			if(tsa_group[meterid].basicinfo.port.OI == PORT_PLUSE) {
				//满足脉冲输入设备，进行相关任务数据存储
				pluseindex = tsa_group[meterid].basicinfo.port.attrindex-1;
				pluseindex = rangeJudge("脉冲",pluseindex,0,(MAX_PULSE_NUM-1));
				if(pluseindex != -1) {
					savePulseTaskData(pluseindex,taskid,savets,tsa_group[meterid].basicinfo.addr,class6015.csds);
				}
			}
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
 * 三型专变：脉冲计量数据冻结文件
 * */
void PluseFreeze(INT8U device_type,CLASS12 *class12)
{
	if(device_type != SPTF3)	return;
	CLASS12_ENERGY	pluse_energy[2]={};
	INT8U	i=0;
	for(i=0;i<2;i++) {
		memcpy(&pluse_energy[i].val_pos_p,&class12[i].val_pos_p,sizeof(pluse_energy[i].val_pos_p));
		memcpy(&pluse_energy[i].val_pos_q,&class12[i].val_pos_q,sizeof(pluse_energy[i].val_pos_q));
		memcpy(&pluse_energy[i].val_nag_p,&class12[i].val_nag_p,sizeof(pluse_energy[i].val_nag_p));
		memcpy(&pluse_energy[i].val_nag_q,&class12[i].val_nag_q,sizeof(pluse_energy[i].val_nag_q));
	}
	saveVariData(PORT_PLUSE,0,(INT8U *)&pluse_energy,sizeof(pluse_energy));
}

/*
 * 终端统计数据及计量数据冻结
 * */
void TerminalFreeze()
{
	static TS 		newts,oldts;
	static INT8U 	first=1;
	int	j=0;
	Volt_PassRate_tj	volt_tj;

	if(first==1) {
		first = 0;
		TSGet(&oldts);
	}
	TSGet(&newts);
	if(newts.Day !=oldts.Day) {
//		PluseFreeze(JProgramInfo->cfg_para.device,JProgramInfo->class12);
		Save_TJ_Freeze(0,0x2203,0x0200,newts,sizeof(gongdian_tj.gongdian),(INT8U *)&gongdian_tj.gongdian);
		gongdian_tj.gongdian.day_tj = 0;
		for(j=0;j<3;j++) {
			memset(&volt_tj,0,sizeof(Volt_PassRate_tj));
			readVariData((0x2131+j),0,(INT8U *)&volt_tj,sizeof(Volt_PassRate_tj));
			memcpy(&volt_tj.dayu_tj,&passu_d[j],sizeof(PassRate_U));
			if(j==0) {
				syslog(LOG_NOTICE,"存储：A相电压-监测时间（日）=%d，（月）=%d\n",volt_tj.dayu_tj.monitorTime,volt_tj.monthu_tj.monitorTime);
			}
			saveVariData((0x2131+j),0,(INT8U *)&volt_tj,sizeof(Volt_PassRate_tj));
			Save_TJ_Freeze(0,(0x2131+j),0x0201,newts,sizeof(PassRate_U),(INT8U *)&passu_d[j]);
			memset(&passu_d[j],0,sizeof(PassRate_U));
		}
		gongdian_tj.gongdian.day_tj = 0;
		if(newts.Month !=oldts.Month && newts.Hour == 0) {
			Save_TJ_Freeze(1,0x2203,0x0200,newts,sizeof(gongdian_tj.gongdian),(INT8U *)&gongdian_tj.gongdian);
			for(j=0;j<3;j++) {
				memset(&volt_tj,0,sizeof(Volt_PassRate_tj));
				readVariData((0x2131+j),0,(INT8U *)&volt_tj,sizeof(Volt_PassRate_tj));
				memcpy(&volt_tj.monthu_tj,&passu_m[j],sizeof(PassRate_U));
				if(j==0) {
					syslog(LOG_NOTICE,"A相电压-监测时间（日）=%d，（月）=%d\n",volt_tj.dayu_tj.monitorTime,volt_tj.monthu_tj.monitorTime);
				}
				saveVariData((0x2131+j),0,(INT8U *)&volt_tj,sizeof(Volt_PassRate_tj));
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


