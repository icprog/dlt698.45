#include <time.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "event.h"
#include "EventObject.h"
#include "PublicFunction.h"
#include "AccessFun.h"
#include "Shmem.h"
#include "Objectdef.h"
#include "ParaDef.h"
#include "../libMq/libmmq.h"
#include "basedef.h"


static TSA TSA_LIST[MAX_POINT_NUM];
static int TSA_NUMS;
//当前最新抄表数据
static Curr_Data curr_data[MAX_POINT_NUM];
//当前内存保存得数据个数
static INT16U currnum=0;
//当前事件参数变更状态
static OI_CHANGE oi_chg;
//MeterPower MeterPowerInfo[3+1];//台区公共表加０－３个电能表
MeterPower TermialPowerInfo;//终端停上电时间信息

static Other_Data other_data[50];
static INT8U other_curr_index=0;
/*
 * 说明：
 * 进程如调用该部分事件接口
 * 在根据事件采集方案 然后根据实际情况调用具体函数判断310B，310C，310D，310E，3105
 * 如抄表失败直接调用Event_310F，实际情况调用Event_3111，Event_3112,Event_311A，Event_311B，Event_311C
 * 实时调用Event_3106判断停上电,Event_3107,Event_3108，Event_3119
 * 698规约判断如是初始化命令调用Event_3100，Event_3114
 * 远程升级或维护进程变更软件版本号调用Event_3101
 * 状态量改变调用Event_3104
 * esam规约调用Event_3109
 * Event_310A设备故障事件可能在很多地方需要用到，直接调用，输入不同参数
 * GPRS通信调用Event_3110
 */
/*
 * 数据初始化 清空该部分内存
 */
void Reset_Eventpara(){
	memset(curr_data,0,sizeof(Curr_Data)*MAX_POINT_NUM*4);
}

/*
 * 更新当前最新得正向有功(只针对正向有功)
 */
INT8U Refresh_Data(TSA tsa,INT32U newdata,INT8U flag){
	int i=0;
	INT8U haveflag=0;
	TS currtime;
	TSGet(&currtime);
	for(i=0;i<currnum;i++){
       if(memcmp(&tsa,&curr_data[i].tsa,sizeof(TSA)) == 0 && curr_data[i].flag == flag){
           curr_data[i].data=newdata;
           memcpy(&curr_data[i].ts,&currtime,sizeof(TS));
           curr_data[i].flag = flag;
           haveflag=1;
    	   break;
       }
	}
	if(haveflag == 0){
		memcpy(&curr_data[currnum].tsa,&tsa,sizeof(TSA));
		curr_data[currnum].data=newdata;
		memcpy(&curr_data[currnum].ts,&currtime,sizeof(TS));
		curr_data[currnum].flag=flag;
		currnum++;
		if(currnum>=MAX_POINT_NUM)
			currnum=0;
	}
	return 1;
}

/*
 * 获取上次保存得正向有功
 */
INT8U Get_Mdata(TSA tsa,INT32U *olddata,TS *ts,INT8U flag){
	int i=0;
	INT8U haveflag=0;
	for(i=0;i<currnum;i++){
		if(memcmp(&tsa,&curr_data[i].tsa,sizeof(TSA)) == 0
				&& curr_data[i].flag == flag){
			*olddata=curr_data[i].data;
			memcpy(ts,&curr_data[i].ts,sizeof(TS));
			curr_data[i].flag = flag;
			haveflag=1;
			break;
		}
	}
	if(haveflag == 0)
		return 0;
	return 1;
}
/*
 * 寻找已知表号，并对这个产生未知电表事件n
 */
INT8U Event_FindTsa(TSA tsa) {
    for (int i = 0; i < TSA_NUMS; i++) {
        int find = 1;
        for (int j = 0; j < tsa.addr[0]; j++) {
            if (tsa.addr[j] != TSA_LIST[i].addr[j]) {
                find = 0;
                break;
            }
        }
        if (find == 1) {
            return 1;
        }
    }
    return 0;
}

/*
 * 根据参数读取事件记录文件
 * oi:事件oi eventno:0最新n某条 Getbuf空指针地址，动态分配 Getlen返回长度
 */
INT8U Get_Event(OAD oad,INT8U eventno,INT8U** Getbuf,int *Getlen,ProgramInfo* prginfo_event)
{
	int filesize=0;
	INT8U currno=0,_currno=0,maxno=0;
	 switch(oad.OI){
	   case 0x3100:
		   currno=prginfo_event->event_obj.Event3100_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event3100_obj.maxnum;
		   break;
	   case 0x3101:
		   currno=prginfo_event->event_obj.Event3101_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event3101_obj.maxnum;
		   break;
	   case 0x3104:
		   currno=prginfo_event->event_obj.Event3104_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event3104_obj.maxnum;
		   break;
	   case 0x3105:
		   currno=prginfo_event->event_obj.Event3105_obj.event_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event3105_obj.event_obj.maxnum;
		   break;
	   case 0x3106:
		   currno=prginfo_event->event_obj.Event3106_obj.event_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event3106_obj.event_obj.maxnum;
		   break;
	   case 0x3107:
		   currno=prginfo_event->event_obj.Event3107_obj.event_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event3107_obj.event_obj.maxnum;
		   break;
	   case 0x3108:
		   currno=prginfo_event->event_obj.Event3108_obj.event_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event3108_obj.event_obj.maxnum;
		   break;
	   case 0x3109:
		   currno=prginfo_event->event_obj.Event3109_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event3109_obj.maxnum;
		   break;
	   case 0x310A:
		   currno=prginfo_event->event_obj.Event310A_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event310A_obj.maxnum;
		   break;
	   case 0x310B:
		   currno=prginfo_event->event_obj.Event310B_obj.event_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event310B_obj.event_obj.maxnum;
		   break;
	   case 0x310C:
		   currno=prginfo_event->event_obj.Event310C_obj.event_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event310C_obj.event_obj.maxnum;
		   break;
	   case 0x310D:
		   currno=prginfo_event->event_obj.Event310D_obj.event_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event310D_obj.event_obj.maxnum;
		   break;
	   case 0x310E:
		   currno=prginfo_event->event_obj.Event310E_obj.event_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event310E_obj.event_obj.maxnum;
		   break;
	   case 0x310F:
		   currno=prginfo_event->event_obj.Event310F_obj.event_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event310F_obj.event_obj.maxnum;
		   break;
	   case 0x3110:
		   currno=prginfo_event->event_obj.Event3110_obj.event_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event3110_obj.event_obj.maxnum;
		   break;
	   case 0x3111:
		   currno=prginfo_event->event_obj.Event3111_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event3111_obj.maxnum;
		   break;
	   case 0x3112:
		   currno=prginfo_event->event_obj.Event3112_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event3112_obj.maxnum;
		   break;
	   case 0x311A:
		   currno=prginfo_event->event_obj.Event311A_obj.event_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event311A_obj.event_obj.maxnum;
		   break;
	   case 0x311B:
		   currno=prginfo_event->event_obj.Event311B_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event311B_obj.maxnum;
		   break;
	   case 0x311C:
		   currno=prginfo_event->event_obj.Event311C_obj.event_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event311C_obj.event_obj.maxnum;
		   break;
	   case 0x3114:
		   currno=prginfo_event->event_obj.Event3114_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event3114_obj.maxnum;
		   break;
	   case 0x3115:
		   currno=prginfo_event->event_obj.Event3115_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event3115_obj.maxnum;
		   break;
	   case 0x3116:
		   currno=prginfo_event->event_obj.Event3116_obj.event_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event3116_obj.event_obj.maxnum;
		   break;
	   case 0x3117:
		   currno=prginfo_event->event_obj.Event3117_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event3117_obj.maxnum;
		   break;
	   case 0x3118:
		   currno=prginfo_event->event_obj.Event3118_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event3118_obj.maxnum;
		   break;
	   case 0x3119:
		   currno=prginfo_event->event_obj.Event3119_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event3119_obj.maxnum;
		   break;
	   case 0x3200:
		   currno=prginfo_event->event_obj.Event3200_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event3200_obj.maxnum;
		   break;
	   case 0x3201:
		   currno=prginfo_event->event_obj.Event3201_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event3201_obj.maxnum;
		   break;
	   case 0x3202:
		   currno=prginfo_event->event_obj.Event3202_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event3202_obj.maxnum;
		   break;
	   case 0x3203:
		   currno=prginfo_event->event_obj.Event3203_obj.crrentnum;
		   maxno=prginfo_event->event_obj.Event3203_obj.maxnum;
		   break;
	}
	_currno=currno-(eventno-1);
	//maxno=0?15:maxno;
	if(maxno == 0)
		maxno=15;
	if(_currno<=0 || _currno>maxno)
		_currno = 1;
	//fprintf(stderr,"currno=%d,maxno=%d pno=%d\n",currno,maxno,prginfo_event->event_obj.Event310E_obj.event_obj.crrentnum);
	SaveFile_type savefiletype = event_record_save;
	switch(oad.attflg){
	     case 2:
	    	 savefiletype=event_record_save;
	    	 break;
	     case 7:
	    	 savefiletype=event_current_save;
	    	 break;
	}
	filesize = getClassFileLen(oad.OI,_currno,savefiletype);
	if(filesize<=0)  return 0;
	*Getlen=filesize;
	*Getbuf=(INT8U*)malloc(filesize);
	readCoverClass(oad.OI,_currno,*Getbuf,*Getlen,savefiletype);
	return 1;
}

/*
 * GETRECORD
 * event_no 上第n条
 * oi_array 要得到得OAD
 * oi_index 要得到得OAD数量
 * real_index 最终数据长度
 * num 数据块数量
 * record_para 数据参数
 * prginfo_event 共享内存
 */
INT8U Getevent_Record(INT8U event_no,OI_698 *oi_array,INT8U oi_index,INT8U *real_index,
		INT8U num,RESULT_RECORD *record_para,ProgramInfo* prginfo_event,INT8U *first){
	INT8U *Getbuf=NULL;//因为记录为变长，只能采用二级指针，动态分配
	int Getlen=0;//记录长度
	Get_Event(record_para->oad,event_no,(INT8U**)&Getbuf,&Getlen,prginfo_event);
	if(Getbuf!=NULL && Getlen>0){
		fprintf(stderr,"Getlen=%d \n",Getlen);
		 record_para->dar = 1;
		 if(record_para->data == NULL){
			 record_para->data=(INT8U*)malloc(Getlen*num+1);//先按最大长度分配
			 memset(&record_para->data[0],0,Getlen*num+1);
		 }
		 if(*first == 0){
			 *first=1;
			 record_para->data[0] =1;		//data
			 *real_index = 2;
		 }
		 record_para->data[1] +=1;		//SEQUENCE OF A-RecordRow
		 INT8U m=0;
		 INT8U T_index=0;
		 for(m=0;m<oi_index;m++){
			switch(oi_array[m]){
				case 0x2022://事件序号
					memcpy(&record_para->data[*real_index],&Getbuf[STANDARD_NO_INDEX],5);
					(*real_index) +=5;
					break;
				case 0x201E://事件发生时间
					memcpy(&record_para->data[*real_index],&Getbuf[STANDARD_HAPPENTIME_INDEX],8);
					(*real_index) +=8;
					break;
				case 0x2020://事件结束时间
					{
						if(Getbuf[STANDARD_ENDTIME_INDEX]==dtdatetimes){
							memcpy(&record_para->data[*real_index],&Getbuf[STANDARD_ENDTIME_INDEX],8);
							(*real_index) +=8;
							T_index = STANDARD_SOURCE_INDEX;
						}else if(Getbuf[STANDARD_ENDTIME_INDEX]==0){
							record_para->data[(*real_index)++]=0;
							T_index = 16;
						}
					}
					break;
				case 0x2024://事件发生源
				{
					INT8U len=0;
					switch(Getbuf[T_index]){
						case s_null:
							record_para->data[(*real_index)++]=0;
							len=0;
							break;
						case s_tsa:
							len=Getbuf[T_index+1]+1;
							break;
						case s_oad:
							len=5;
							break;
						case s_usigned:
							len=2;
							break;
						case s_enum:
							len=2;
							break;
						case s_oi:
							len=3;
							break;
					}
					if(len>0)
						memcpy(&record_para->data[*real_index],&Getbuf[T_index],len);
					 (*real_index) +=len;
					 T_index +=len;
				}
				break;
				case 0x3309://停上电属性标志
					memcpy(&record_para->data[*real_index],&Getbuf[T_index+20],3);
					(*real_index) +=3;
					break;
				case 0x2025://事件当前值
					memcpy(&record_para->data[*real_index],&Getbuf[0],Getlen);
					(*real_index) +=Getlen;
					break;
			}
		 }
		 if (Getbuf!=NULL)
			free(Getbuf);
	}else{
		record_para->data[(*real_index)++]=1;
		record_para->data[(*real_index)++]=0;
		record_para->dar = 0; //无数据
	}
	fprintf(stderr,"*rel_index=%d \n",*real_index);
	record_para->datalen =*real_index;//最终长度
	return 1;
}
/*
 * GETREQUESTRECORD selector9/10 获取事件记录中数据
 * record_para传入要得到的数据OAD prginfo_event 共享内存
 */
INT8U Getevent_Record_Selector(RESULT_RECORD *record_para,ProgramInfo* prginfo_event){
    if(record_para == NULL)
    	return 0;
	INT8U event_no=1; //9:上第n条记录 10:上N条
	OI_698 oi_array[50]={0};
	 INT8U oi_index=0; //召测得数据OI数量
	 INT8U real_index=0;//最终长度
	 INT8U i=0;
	 if(record_para->selectType == 9 || record_para->selectType == 10){
		 for(i=0;i<record_para->rcsd.csds.num;i++){
			//OAD
			if(record_para->rcsd.csds.csd[i].type == 0){
				if(record_para->rcsd.csds.csd[i].csd.oad.attflg == 2)
					oi_array[oi_index++]=record_para->rcsd.csds.csd[i].csd.oad.OI;
			}
			//ROAD
			else if(record_para->rcsd.csds.csd[i].type == 1){
				if(record_para->rcsd.csds.csd[i].csd.road.oad.attflg == 2)
					oi_array[oi_index++]=record_para->rcsd.csds.csd[i].csd.road.oad.OI;

				INT8U j=0;
				for(j=0;j<record_para->rcsd.csds.csd[i].csd.road.num;j++){
					if(record_para->rcsd.csds.csd[i].csd.road.oads[j].attflg == 2)
						oi_array[oi_index++]=record_para->rcsd.csds.csd[i].csd.road.oads[j].OI;
				}
			}
		 }
	 }else if(record_para->selectType == 2){
		 oi_array[oi_index++]=0x2022;
		 oi_array[oi_index++]=0x201e;
		 oi_array[oi_index++]=0x2020;
		 oi_array[oi_index++]=0x2024;
	 }
	 int j=0;
	 for(j=0;j<oi_index;j++)
		 fprintf(stderr,"j:%04x \n",oi_array[j]);
	 if(oi_index > 0){
		 INT8U first=0;
		 switch(record_para->selectType){
		     case 2:
			 case 9:
			 {
				event_no=record_para->select.selec9.recordn;
				fprintf(stderr,"event_no=%d \n",event_no);
				Getevent_Record(event_no,oi_array,oi_index,
						&real_index,1,record_para,prginfo_event,&first);
			 }
				 break;
			 case 10:
			 {
				 event_no=record_para->select.selec10.recordn;
				 for(;event_no>0;event_no--){
					 Getevent_Record(event_no,oi_array,oi_index,
							 &real_index,record_para->select.selec10.recordn,
							 record_para,prginfo_event,&first);
				 }
			 }
				 break;
		   }
	 }
	return 1;
}
/*
 * 事件需要上报
 */
INT8U Need_Report(OI_698 oi,INT8U eventno,ProgramInfo* prginfo_event){
	static INT8U lastchgoi4300=0;
	static INT8U first=1;
	static CLASS19 class19;
	fprintf(stderr,"cesi \\n");
	if(first){
		first=0;
		lastchgoi4300 = prginfo_event->oi_changed.oi4300;
		memset(&class19,0,sizeof(CLASS19));
		readCoverClass(0x4300,0,&class19,sizeof(class19),para_vari_save);
	}
	if(lastchgoi4300!=prginfo_event->oi_changed.oi4300){
		memset(&class19,0,sizeof(CLASS19));
		readCoverClass(0x4300,0,&class19,sizeof(class19),para_vari_save);
		if(lastchgoi4300!=prginfo_event->oi_changed.oi4300) {
			lastchgoi4300=prginfo_event->oi_changed.oi4300;
		}
	}
	fprintf(stderr,"libevent:active_report=%d talk_master=%d \n",class19.active_report,class19.talk_master);
	if(class19.active_report == 1){
		mqs_send((INT8S *)PROXY_NET_MQ_NAME,1,TERMINALEVENT_REPORT,(INT8U *)&oi,sizeof(OI_698));
	}
	return 1;
}

/*
 * 根据不同类型数据获取事件发生源
 */
INT8U Get_Source(INT8U *Source,Source_Typ S_type,
		INT8U *Data_type,INT8U *Len){
	switch(S_type){
		case s_null:
			*Data_type=0;
			*Len=0;
			break;
		case s_tsa:
			*Data_type=85;
		    *Len=(Source[0]+1);
			break;
		case s_oad:
			*Data_type=81;
			*Len=4;
			break;
		case s_usigned:
			*Data_type=17;
			*Len=1;
			break;
		case s_enum:
			*Data_type=22;
			*Len=1;
			break;
		case s_oi:
			*Data_type=80;
			*Len=2;
			break;
	}
	return 1;
}
/*
 * 当前记录值
 */
INT8U Get_CurrResult(INT8U *Rbuf,INT8U *Index,
		INT8U *Source,Source_Typ S_type,INT8U Num,INT32U Timedelay){
    Rbuf[(*Index)++]=dtstructure;//structure
    Rbuf[(*Index)++]=0x02;//数量
    //事件发生源
    INT8U datatype=0,sourcelen=0;
    Get_Source(Source,S_type,&datatype,&sourcelen);
    Rbuf[(*Index)++] = datatype;
//    if(datatype==s_tsa)
//    	Rbuf[(*Index)++] = sourcelen;
    if(sourcelen>0)
    	memcpy(&Rbuf[(*Index)],Source,sourcelen);
    (*Index)+=sourcelen;
	Rbuf[(*Index)++] = dtstructure;//structure
	Rbuf[(*Index)++] = 0x02;
	Rbuf[(*Index)++] = 0x06;
	Rbuf[(*Index)++] = (Num>>24)&0x000000ff;
	Rbuf[(*Index)++] = (Num>>16)&0x000000ff;
	Rbuf[(*Index)++] = (Num>>8)&0x000000ff;
	Rbuf[(*Index)++] = Num&0x000000ff;
	Rbuf[(*Index)++] = 0x06;
	Rbuf[(*Index)++] = (Timedelay>>24)&0x000000ff;
	Rbuf[(*Index)++] = (Timedelay>>16)&0x000000ff;
	Rbuf[(*Index)++] = (Timedelay>>8)&0x000000ff;
	Rbuf[(*Index)++] = Timedelay&0x000000ff;
	return 1;
}
/*
 * 标准数据单元接口
 * Save_buf记录单元 Index长度 Eventno事件记录序号 Source事件发生源 Source_Type事件发生源类型
 */
INT8U Get_StandardUnit(ProgramInfo* prginfo_event,OI_698 oi,INT8U *Rbuf,INT8U *Index,
		INT8U Eventno,INT8U *Source,Source_Typ S_type){
	//Struct
	Rbuf[(*Index)++] = dtstructure;//0
	//单元数量
	Rbuf[(*Index)++] = STANDARD_NUM;//1
	//事件记录序号
	Rbuf[(*Index)++] = dtdoublelongunsigned;//2
	INT32U En=(INT32U)Eventno;
	Rbuf[(*Index)++] = ((En>>24)&0x000000ff);//3
	Rbuf[(*Index)++] = ((En>>16)&0x000000ff);//4
	Rbuf[(*Index)++] = ((En>>8)&0x000000ff);//5
	Rbuf[(*Index)++] = En&0x000000ff;//6
	DateTimeBCD ntime;
	DataTimeGet(&ntime);

	//事件发生时间
	if(oi == 0x3106)
	{
		if(*Source==0){//停电
			Rbuf[(*Index)++] = dtdatetimes;//7
			Rbuf[(*Index)++] = ((ntime.year.data>>8)&0x00ff);//8
			Rbuf[(*Index)++] = ((ntime.year.data)&0x00ff);//9
			Rbuf[(*Index)++] = ntime.month.data;//10
			Rbuf[(*Index)++] = ntime.day.data;//11
			Rbuf[(*Index)++] = ntime.hour.data;//12
			Rbuf[(*Index)++] = ntime.min.data;//13
			Rbuf[(*Index)++] = ntime.sec.data;//14
		}else{
			Rbuf[(*Index)++] = dtdatetimes;//7
			Rbuf[(*Index)++] = (((TermialPowerInfo.PoweroffTime.tm_year+1900)>>8)&0x00ff);//8
			Rbuf[(*Index)++] = ((TermialPowerInfo.PoweroffTime.tm_year+1900)&0x00ff);//9
			Rbuf[(*Index)++] = TermialPowerInfo.PoweroffTime.tm_mon+1;//10
			Rbuf[(*Index)++] = TermialPowerInfo.PoweroffTime.tm_mday;//11
			Rbuf[(*Index)++] = TermialPowerInfo.PoweroffTime.tm_hour;//12
			Rbuf[(*Index)++] = TermialPowerInfo.PoweroffTime.tm_min;//13
			Rbuf[(*Index)++] = TermialPowerInfo.PoweroffTime.tm_sec;//14
		}
	}
	else
	{
		Rbuf[(*Index)++] = dtdatetimes;//7
		Rbuf[(*Index)++] = ((ntime.year.data>>8)&0x00ff);//8
		Rbuf[(*Index)++] = ((ntime.year.data)&0x00ff);//9
		Rbuf[(*Index)++] = ntime.month.data;//10
		Rbuf[(*Index)++] = ntime.day.data;//11
		Rbuf[(*Index)++] = ntime.hour.data;//12
		Rbuf[(*Index)++] = ntime.min.data;//13
		Rbuf[(*Index)++] = ntime.sec.data;//14
	}
	//事件结束时间
	if(oi==0x311C){
		Rbuf[(*Index)++] = dtdatetimes;//15
		memset(&Rbuf[*Index],DATA_FF,sizeof(ntime));//TODO
		(*Index)+=sizeof(ntime);//0
	}else if(oi==0x3105 || oi==0x310A ||
			oi==0x310B || oi==0x310C ||
			oi==0x310D || oi==0x310E){
		Rbuf[(*Index)++] = 0;//15无结束时间
	}else if(oi==0x3106){
		if(*Source==0){
			Rbuf[(*Index)++] = 0;//15无结束时间
		}
		else{
			Rbuf[(*Index)++] = dtdatetimes;//15
			Rbuf[(*Index)++] = (((TermialPowerInfo.PoweronTime.tm_year+1900)>>8)&0x00ff);//16
			Rbuf[(*Index)++] = ((TermialPowerInfo.PoweronTime.tm_year+1900)&0x00ff);//17
			Rbuf[(*Index)++] = TermialPowerInfo.PoweronTime.tm_mon+1;//18
			Rbuf[(*Index)++] = TermialPowerInfo.PoweronTime.tm_mday;//19
			Rbuf[(*Index)++] = TermialPowerInfo.PoweronTime.tm_hour;//20
			Rbuf[(*Index)++] = TermialPowerInfo.PoweronTime.tm_min;//21
			Rbuf[(*Index)++] = TermialPowerInfo.PoweronTime.tm_sec;//22
		}
	}
	else{
		Rbuf[(*Index)++] = dtdatetimes;//15
		Rbuf[(*Index)++] = ((ntime.year.data>>8)&0x00ff);//16
		Rbuf[(*Index)++] = ((ntime.year.data)&0x00ff);//17
		Rbuf[(*Index)++] = ntime.month.data;//18
		Rbuf[(*Index)++] = ntime.day.data;//19
		Rbuf[(*Index)++] = ntime.hour.data;//20
		Rbuf[(*Index)++] = ntime.min.data;//21
		Rbuf[(*Index)++] = ntime.sec.data;//22
	}
	//事件发生源
	INT8U datatype=0,sourcelen=0;
	Get_Source(Source,S_type,&datatype,&sourcelen);
	Rbuf[(*Index)++] = datatype;//23
	if(sourcelen>0)
		memcpy(&Rbuf[(*Index)],Source,sourcelen);
	(*Index)+=sourcelen;
	//事件上报状态
	Rbuf[(*Index)++] = dtarray;//array
	Rbuf[(*Index)++] = 0x02;//数量
	Rbuf[(*Index)++] = dtstructure;//struct
	Rbuf[(*Index)++] = 0x02;//数量
	Rbuf[(*Index)++] = 0x51;//OAD
	Rbuf[(*Index)++] = 0x45;//gprs
	Rbuf[(*Index)++] = 0x00;
	Rbuf[(*Index)++] = 0x00;
	Rbuf[(*Index)++] = 0x00;
	Rbuf[(*Index)++] = dtunsigned;//unsigned
	Rbuf[(*Index)++] = 0x00;
	Rbuf[(*Index)++] = dtstructure;//struct
	Rbuf[(*Index)++] = 0x02;//数量
	Rbuf[(*Index)++] = 0x51;//OAD
	Rbuf[(*Index)++] = 0x45;//以太网
	Rbuf[(*Index)++] = 0x10;
	Rbuf[(*Index)++] = 0x00;
	Rbuf[(*Index)++] = 0x00;
	Rbuf[(*Index)++] = dtunsigned;//unsigned
	Rbuf[(*Index)++] = 0x00;
	INT8U low_unit=oi&0x00ff;
	prginfo_event->dev_info.Cur_Ercno=((((low_unit>>4)&0x0f)*10)+(low_unit&0x0f));
    return 1;
}

/*
 * 每个事件记录数不超过设定得最大记录数，默认15个
 */
INT16U Getcurrno(INT16U currno,INT16U maxno){
	fprintf(stderr,"[event]currno=%d maxno=%d \n",currno,maxno);
	if(maxno == 0)
		maxno=15;
	fprintf(stderr,"maxno=%d \n",maxno);
	if(currno>maxno)
		return 1;
	return currno;
}
/*
 * 终端初始化事件1 可以698规约解析actionrequest 调用该接口，data为OAD
 */
INT8U Event_3100(INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	Reset_Eventpara();
	if(oi_chg.oi3100 != prginfo_event->oi_changed.oi3100){
		readCoverClass(0x3100,0,&prginfo_event->event_obj.Event3100_obj,sizeof(prginfo_event->event_obj.Event3100_obj),event_para_save);
		oi_chg.oi3100 = prginfo_event->oi_changed.oi3100;
	}

    if (prginfo_event->event_obj.Event3100_obj.enableflag == 0) {
        return 0;
    }
    //43 00 03 00
    //事件判定
    INT16U oi=(data[0]<<8)+data[1];
    INT8U action=data[2]&0b00011111;
    if(oi==0x4300 && action==3){
    	INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		prginfo_event->event_obj.Event3100_obj.crrentnum++;
		prginfo_event->event_obj.Event3100_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event3100_obj.crrentnum,prginfo_event->event_obj.Event3100_obj.maxnum);
		INT32U crrentnum = prginfo_event->event_obj.Event3100_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(prginfo_event,0x3100,Save_buf,&index,crrentnum,NULL,s_null);
		//无关联数据
		Save_buf[STANDARD_NUM_INDEX]+=0;
		//存储更改后得参数
		saveCoverClass(0x3100,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event3100_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3100,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};memset(Currbuf,0,50);
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
		saveCoverClass(0x3100,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(prginfo_event->event_obj.Event3100_obj.reportflag)
			Need_Report(0x3100,crrentnum,prginfo_event);
    }
    return 1;
}

/*
 * 终端版本变更事件 可直接调用，维护命令修改版本或者升级后可调用
 */
INT8U Event_3101(INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi3101 != prginfo_event->oi_changed.oi3101){
		readCoverClass(0x3101,0,&prginfo_event->event_obj.Event3101_obj,sizeof(prginfo_event->event_obj.Event3101_obj),event_para_save);
		oi_chg.oi3101 = prginfo_event->oi_changed.oi3101;
	}
    if (prginfo_event->event_obj.Event3101_obj.enableflag == 0) {
        return 0;
    }
    //事件判定
    if(1){
    	INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		prginfo_event->event_obj.Event3101_obj.crrentnum++;
		prginfo_event->event_obj.Event3101_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event3101_obj.crrentnum,prginfo_event->event_obj.Event3101_obj.maxnum);
		INT32U crrentnum = prginfo_event->event_obj.Event3101_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(prginfo_event,0x3101,Save_buf,&index,crrentnum,NULL,s_null);
		//取共享内存或文件
		//事件发生前软件版本号
		Save_buf[index++]=dtvisiblestring;//visible-string 4
		Save_buf[index++]=4;// 4
		Save_buf[index++]=0;
		Save_buf[index++]=0;
		Save_buf[index++]=0;
		Save_buf[index++]=0;
		//事件发生前软件版本号
		Save_buf[index++]=dtvisiblestring;//visible-string 4
		Save_buf[index++]=4;// 4
		Save_buf[index++]=0;
		Save_buf[index++]=0;
		Save_buf[index++]=0;
		Save_buf[index++]=0;
		Save_buf[STANDARD_NUM_INDEX]+=2;
		//存储更改后得参数
		saveCoverClass(0x3101,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event3101_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3101,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};memset(Currbuf,0,50);
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
		saveCoverClass(0x3101,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(prginfo_event->event_obj.Event3101_obj.reportflag)
			Need_Report(0x3101,crrentnum,prginfo_event);
    }
    return 1;
}

/*
 * 状态量变位事件 可直接调用 data为前后得ST CD，（1-4路）8个字节即可
 */
INT8U Event_3104(INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi3104 != prginfo_event->oi_changed.oi3104){
		readCoverClass(0x3104,0,&prginfo_event->event_obj.Event3104_obj,sizeof(prginfo_event->event_obj.Event3104_obj),event_para_save);
		oi_chg.oi3104 = prginfo_event->oi_changed.oi3104;
	}
	fprintf(stderr,"prginfo_event->event_obj.Event3104_obj.enableflag = %d \n",prginfo_event->event_obj.Event3104_obj.enableflag);
    if (prginfo_event->event_obj.Event3104_obj.enableflag == 0) {
        return 0;
    }
    //事件判定
    if(1){
    	INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		prginfo_event->event_obj.Event3104_obj.crrentnum++;
		prginfo_event->event_obj.Event3104_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event3104_obj.crrentnum,prginfo_event->event_obj.Event3104_obj.maxnum);
		INT32U crrentnum = prginfo_event->event_obj.Event3104_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(prginfo_event,0x3104,Save_buf,&index,crrentnum,NULL,s_null);
		//事件发生时间
		DateTimeBCD ntime;
		DataTimeGet(&ntime);
		Save_buf[index++] = dtdatetimes;
//		memcpy(&Save_buf[index], &ntime, sizeof(ntime));
//		index+=sizeof(ntime);
		Save_buf[index++] = ((ntime.year.data>>8)&0x00ff);
		Save_buf[index++] = ((ntime.year.data)&0x00ff);
		Save_buf[index++] = ntime.month.data;
		Save_buf[index++] = ntime.day.data;
		Save_buf[index++] = ntime.hour.data;
		Save_buf[index++] = ntime.min.data;
		Save_buf[index++] = ntime.sec.data;
		//第1路事件发生后
		Save_buf[index++]=dtstructure;//structure
		Save_buf[index++]=2;// 2
		Save_buf[index++]=dtunsigned;
		Save_buf[index++]=data[0];
		Save_buf[index++]=dtunsigned;
		Save_buf[index++]=data[1];
		//第2路事件发生后
		Save_buf[index++]=dtstructure;//structure
		Save_buf[index++]=2;// 2
		Save_buf[index++]=dtunsigned;
		Save_buf[index++]=data[2];
		Save_buf[index++]=dtunsigned;
		Save_buf[index++]=data[3];
		//第3路事件发生后
		Save_buf[index++]=dtstructure;//structure
		Save_buf[index++]=2;// 2
		Save_buf[index++]=dtunsigned;
		Save_buf[index++]=data[4];
		Save_buf[index++]=dtunsigned;
		Save_buf[index++]=data[5];
		//第4路事件发生后
		Save_buf[index++]=dtstructure;//structure
		Save_buf[index++]=2;// 2
		Save_buf[index++]=dtunsigned;
		Save_buf[index++]=data[6];
		Save_buf[index++]=dtunsigned;
		Save_buf[index++]=data[7];
		Save_buf[STANDARD_NUM_INDEX]+=5;
		//存储更改后得参数
		saveCoverClass(0x3104,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event3104_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3104,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};memset(Currbuf,0,50);
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
		saveCoverClass(0x3104,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(prginfo_event->event_obj.Event3104_obj.reportflag)
			Need_Report(0x3104,crrentnum,prginfo_event);
    }
    return 1;
}

/*
 * 电能表时钟超差事件 tsa事件发生源 电表时钟
 */
INT8U Event_3105(TSA tsa,INT8U taskno,INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi3105 != prginfo_event->oi_changed.oi3105){
		readCoverClass(0x3105,0,&prginfo_event->event_obj.Event3105_obj,sizeof(prginfo_event->event_obj.Event3105_obj),event_para_save);
		oi_chg.oi3105 = prginfo_event->oi_changed.oi3105;
	}
    if (prginfo_event->event_obj.Event3105_obj.event_obj.enableflag == 0) {
        return 0;
    }
//    if(prginfo_event->event_obj.Event3105_obj.mto_obj.task_no!=taskno)
//    	return 0;

    if(data==NULL)
    	return 0;
    TS jzqtime;
    TSGet(&jzqtime);//集中器时间
    TS metertime;
    metertime.Year=((data[0]<<8)+data[1]);//年
    metertime.Month=data[2];//月
    metertime.Day=data[3];
    metertime.Hour=data[4];
    metertime.Minute=data[5];
    metertime.Sec=data[6];
    fprintf(stderr,"jzqtime=%d-%d-%d %d:%d:%d \n",jzqtime.Year,jzqtime.Month,jzqtime.Day,jzqtime.Hour,jzqtime.Minute,jzqtime.Sec);
    fprintf(stderr,"metertime=%d-%d-%d %d:%d:%d \n",metertime.Year,metertime.Month,metertime.Day,metertime.Hour,metertime.Minute,metertime.Sec);
    int tcha=abs(difftime(tmtotime_t(metertime),tmtotime_t(jzqtime)));
    fprintf(stderr,"tcha=%d over_threshold=%d \n",tcha,prginfo_event->event_obj.Event3105_obj.mto_obj.over_threshold);
    //事件判定
    if (tcha>prginfo_event->event_obj.Event3105_obj.mto_obj.over_threshold
    		&& prginfo_event->event_obj.Event3105_obj.mto_obj.over_threshold>0) {
    	INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		//更新当前记录数
		prginfo_event->event_obj.Event3105_obj.event_obj.crrentnum++;
		prginfo_event->event_obj.Event3105_obj.event_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event3105_obj.event_obj.crrentnum,prginfo_event->event_obj.Event3105_obj.event_obj.maxnum);
		INT32U crrentnum = prginfo_event->event_obj.Event3105_obj.event_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(prginfo_event,0x3105,Save_buf,&index,crrentnum,(INT8U*)&tsa,s_tsa);
		//无属性3关联数据
		Save_buf[STANDARD_NUM_INDEX]+=0;
		//存储更改后得参数
		saveCoverClass(0x3105,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event3105_obj,sizeof(Event3105_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3105,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
        //存储当前记录值
		INT8U Currbuf[50]={};memset(Currbuf,0,50);
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,(INT8U*)&tsa,s_tsa,crrentnum,0);
		saveCoverClass(0x3105,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(prginfo_event->event_obj.Event3105_obj.event_obj.reportflag)
			Need_Report(0x3105,crrentnum,prginfo_event);
    }else
        return 0;
    return 1;
}


/*
 * 写停上电事件日志
 */
void ERC3106log(int type,INT32U Ua,struct tm logtm)
{
	char ERC3106log[100];
	memset((void*)ERC3106log,'\0',sizeof(ERC3106log));

	if (type==1)//上电
		sprintf(ERC3106log,"上电: time-%d.%d.%d-%d:%d:%d,Ua=%d\n",logtm.tm_year+1900,logtm.tm_mon+1,
				logtm.tm_mday,logtm.tm_hour,logtm.tm_min,
				logtm.tm_sec,Ua);
	else
		sprintf(ERC3106log,"停电: time-%d.%d.%d-%d:%d:%d,Ua=%d\n",logtm.tm_year+1900,logtm.tm_mon+1,
				logtm.tm_mday,logtm.tm_hour,logtm.tm_min,
				logtm.tm_sec,Ua);

	FILE *fp= NULL;
	fp = fopen("/nand/ERC3106.log","a+");
	if(fp > 0)
	{
		fseek(fp,0,SEEK_END);
		fwrite(ERC3106log,1,strlen(ERC3106log),fp);
		fclose(fp);
	}
}

/*
 * 写停上电事件临时文件
 */
INT8U filewrite(char *FileName, void *source, int size)
{
	FILE *fp = NULL;
//	int fd;
	INT8U res;
	int num = 0;
	fp = fopen((char*) FileName, "w");
	if (fp != NULL )
	{
		fseek(fp, 0, SEEK_SET);
		num = fwrite(source, size, 1, fp);
//		fprintf(stderr,"fwrite.num=%d,FileName=%s,size=%d\n",num,FileName,size);
		if (num == 1)
		{
			res = 1;
		} else
			res = 0;
		fclose(fp);
	} else
	{
		res = 0;
	}
	return res;
}

/*
 * 发送停上电事件记录函数
 * 参数:参数1,事件标记,属性是否正常,停电是否有效等,参数2,事件类型,0为停电事件,非0为上电事件
*/
void SendERC3106(INT8U flag,INT8U Erctype,ProgramInfo* prginfo_event)
{
	INT8U Save_buf[256];
	bzero(Save_buf, sizeof(Save_buf));
	prginfo_event->event_obj.Event3106_obj.event_obj.crrentnum++;
	prginfo_event->event_obj.Event3106_obj.event_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event3106_obj.event_obj.crrentnum,prginfo_event->event_obj.Event3106_obj.event_obj.maxnum);
	INT32U crrentnum = prginfo_event->event_obj.Event3106_obj.event_obj.crrentnum;
	INT8U index=0;
	//标准数据单元
	Get_StandardUnit(prginfo_event,0x3106,Save_buf,&index,crrentnum,(INT8U*)&Erctype,s_enum);
	//属性标志
	Save_buf[index++]=dtbitstring;//bit-string
	Save_buf[index++]=0x08;//len
	Save_buf[index++]=flag;
	Save_buf[STANDARD_NUM_INDEX]+=1;
	//存储更改后得参数
	int ret=saveCoverClass(0x3106,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event3106_obj,sizeof(Event3106_Object),event_para_save);
	fprintf(stderr,"ret=%d \n",ret);
	//存储记录集
	saveCoverClass(0x3106,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
	//存储当前记录值
	INT8U Currbuf[50]={};memset(Currbuf,0,50);
	INT8U Currindex=0;
	Get_CurrResult(Currbuf,&Currindex,(INT8U*)&Erctype,s_enum,crrentnum,0);
	saveCoverClass(0x3106,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
	//判断是否要上报
	if(prginfo_event->event_obj.Event3106_obj.event_obj.reportflag)
		Need_Report(0x3106,crrentnum,prginfo_event);
}

//判断终端与电能表的时间偏差
BOOLEAN MeterDiff(ProgramInfo* prginfo_event,MeterPower *MeterPowerInfo,INT8U *state)
{
	int i = 0,index=0;
	for(i = 0;i<POWEROFFON_NUM;i++)
	{
		if(MeterPowerInfo[i].Valid)
		{
			fprintf(stderr,"MeterPowerInfo[%d],PoweroffTime,year=%d,month=%d,day=%d,hour=%d,min=%d,sec=%d\n",
					i,MeterPowerInfo[i].PoweroffTime.tm_year+1900,MeterPowerInfo[i].PoweroffTime.tm_mon,MeterPowerInfo[i].PoweroffTime.tm_mday,
					MeterPowerInfo[i].PoweroffTime.tm_hour,MeterPowerInfo[i].PoweroffTime.tm_min,MeterPowerInfo[i].PoweroffTime.tm_sec);

			fprintf(stderr,"MeterPowerInfo[%d],PoweronTime,year=%d,month=%d,day=%d,hour=%d,min=%d,sec=%d\n",
								i,MeterPowerInfo[i].PoweronTime.tm_year+1900,MeterPowerInfo[i].PoweronTime.tm_mon,MeterPowerInfo[i].PoweronTime.tm_mday,
								MeterPowerInfo[i].PoweronTime.tm_hour,MeterPowerInfo[i].PoweronTime.tm_min,MeterPowerInfo[i].PoweronTime.tm_sec);
			fprintf(stderr,"TermialPowerInfo,PoweroffTime,year=%d,month=%d,day=%d,hour=%d,min=%d,sec=%d\n",
						TermialPowerInfo.PoweroffTime.tm_year+1900,TermialPowerInfo.PoweroffTime.tm_mon,TermialPowerInfo.PoweroffTime.tm_mday,
						TermialPowerInfo.PoweroffTime.tm_hour,TermialPowerInfo.PoweroffTime.tm_min,TermialPowerInfo.PoweroffTime.tm_sec);
			fprintf(stderr,"TermialPowerInfo,TermialPowerInfo.PoweronTime,year=%d,month=%d,day=%d,hour=%d,min=%d,sec=%d\n",
								TermialPowerInfo.PoweronTime.tm_year+1900,TermialPowerInfo.PoweronTime.tm_mon,TermialPowerInfo.PoweronTime.tm_mday,
								TermialPowerInfo.PoweronTime.tm_hour,TermialPowerInfo.PoweronTime.tm_min,TermialPowerInfo.PoweronTime.tm_sec);
			int Diff1_tmp1 = abs(difftime(mktime(&MeterPowerInfo[i].PoweroffTime),mktime(&TermialPowerInfo.PoweroffTime)));
			int Diff1_tmp2 = abs(difftime(mktime(&MeterPowerInfo[i].PoweronTime),mktime(&TermialPowerInfo.PoweronTime)));
			fprintf(stderr,"Diff1_tmp1 =%d,Diff1_tmp2 = %d,poweroff_on_offset=%d\r\n",Diff1_tmp1,Diff1_tmp2,prginfo_event->event_obj.Event3106_obj.poweroff_para_obj.screen_para_obj.startstoptime_offset);
			//判断停电事件　起止时间偏差限值的合法性
			int poweroffset = 0;
			poweroffset = prginfo_event->event_obj.Event3106_obj.poweroff_para_obj.screen_para_obj.startstoptime_offset;
#ifdef SHANDONG
			poweroffset = 100;
#endif
			if((abs(Diff1_tmp1)>(poweroffset*60))
			||(abs(Diff1_tmp2)>(poweroffset*60)))
			{
				TermialPowerInfo.Valid = POWER_OFF_INVALIDE;
				fprintf(stderr,"MeterDiff err1\r\n");
				filewrite(ERC3106PATH,&TermialPowerInfo,sizeof(TermialPowerInfo));
				return FALSE;
			}

			//判断停电事件时间区段偏差限值
			int tmp1 =abs(difftime(mktime(&MeterPowerInfo[i].PoweronTime),mktime(&MeterPowerInfo[i].PoweroffTime)));
			int tmp2 =abs(difftime(mktime(&TermialPowerInfo.PoweronTime),mktime(&TermialPowerInfo.PoweroffTime)));
            fprintf(stderr,"tmp1=%d tmp2=%d sectortime_offset=%d \n",tmp1,tmp2,
            		prginfo_event->event_obj.Event3106_obj.poweroff_para_obj.screen_para_obj.sectortime_offset*60);
			if(abs(tmp1-tmp2) > prginfo_event->event_obj.Event3106_obj.poweroff_para_obj.screen_para_obj.sectortime_offset*60)
			{
				fprintf(stderr,"MeterDiff err2\r\n");
				TermialPowerInfo.Valid = POWER_OFF_INVALIDE;
				filewrite(ERC3106PATH,&TermialPowerInfo,sizeof(TermialPowerInfo));
				return FALSE;
			}
		}
	}
		fprintf(stderr,"POWER_OFF_VALIDE\r\n");
		TermialPowerInfo.Valid = POWER_OFF_VALIDE;
		filewrite(ERC3106PATH,&TermialPowerInfo,sizeof(TermialPowerInfo));
		return TRUE;
}

INT8U fileread(char *FileName, void *source, INT32U size)
{
	FILE *fp = NULL;
	int num, ret = 0;
	fp = fopen((const char*)FileName, "r");
	if (fp != NULL )
	{
		num = fread(source, size, 1, fp);
		ret = 1;
		fclose(fp);
	}
	else
	{
		ret = 0;
		//fprintf(stderr, "%s read error\n\r", FileName);
	}
	return ret;
}
INT8U Set_Poweron(ProgramInfo* prginfo_event,time_t time_of_now,INT16U maxtime_space,
		INT16U mintime_space,INT8U *flag){
            fprintf(stderr,"注意：进来了... \n");
	        localtime_r((const time_t*)&time_of_now, (struct tm *)&TermialPowerInfo.PoweronTime);
			filewrite(ERC3106PATH,&TermialPowerInfo,sizeof(TermialPowerInfo));
			int interval = difftime(mktime(&TermialPowerInfo.PoweronTime),mktime(&TermialPowerInfo.PoweroffTime));
			ERC3106log(1,prginfo_event->ACSRealData.Ua,TermialPowerInfo.PoweronTime);//调试加入log

			//如果上电时间大于停电时间或者停上电时间间隔小于最小间隔或者大于最大间隔不产生下电事件
			if(mintime_space==0)
				mintime_space=1;
			if(maxtime_space==0)
				maxtime_space=4300;
			fprintf(stderr,"interval=%d mintime_space=%d maxtime_space=%d \n",interval,mintime_space*60,maxtime_space*60);
			if((interval > mintime_space*60)&&(interval < maxtime_space*60)) {
				fprintf(stderr,"上电时间满足参数：interval=%d\n",interval);
				*flag = 0b10000000;
			}
	return 1;
}
INT8U Get_meter_powoffon(ProgramInfo* prginfo_event,MeterPower *MeterPowerInfo,
		INT8U *state,time_t time_of_now,INT16U maxtime_space,
		INT16U mintime_space,INT8U *flag){

    INT8U collect_flag=prginfo_event->event_obj.Event3106_obj.poweroff_para_obj.collect_para_obj.collect_flag;
    INT8U collect_flag_1 = ((collect_flag>>7)&0x01);
    INT8U collect_flag_2 = ((collect_flag>>6)&0x01);
    static INT8U on_time = 0;
    if(collect_flag_1 == 1){
    	on_time++;
		if(on_time == 1){
			fprintf(stderr,"此时第一次进来 on_time=%d \n",on_time);
			Set_Poweron(prginfo_event,time_of_now,maxtime_space,mintime_space,flag);
			return 1;
		}else{
			if(on_time < 60){
				fprintf(stderr,"上电等待 on_time=%d \n",on_time);
				return 1;
			}
		}
    }else{
    	fprintf(stderr,"不需要采集得时候进来");
    	Set_Poweron(prginfo_event,time_of_now,maxtime_space,mintime_space,flag);
    }
    on_time = 0;
    TermialPowerInfo.ERC3106State = POWER_ON;
    fprintf(stderr,"\n 如果初步判断事件有效，置标志位，通知抄表停上电事件 \n");
	CLASS_6001	 meter={};
	CLASS11		coll={};
	int			i=0,j=0,blknum=0;
	INT16U		oi = 0x6000;

	if(readInterClass(oi,&coll)==-1)  return 0;
	fprintf(stderr,"采集档案配置表CLASS_11--------------");
	fprintf(stderr,"逻辑名:%s    当前=%d     最大=%d\n",coll.logic_name,coll.curr_num,coll.max_num);

	blknum = getFileRecordNum(oi);
	if(blknum == -1) {
		fprintf(stderr,"未找到OI=%04x的相关信息配置内容！！！\n",oi);
		return 0;
	}else if(blknum == -2){
		fprintf(stderr,"采集档案表不是整数，检查文件完整性！！！\n");
		return 0;
	}
	//如果有效
    if(collect_flag_1 == 1){
    	//随机
    	if(collect_flag_2 == 1){
    		fprintf(stderr,"随机选择测量点 \n");
    		 INT8U curr_n=0;
             for(i=0;i<blknum;i++){
            	 if(readParaClass(oi,&meter,i)==1){
            		 if(meter.basicinfo.port.OI == 0xF201){
            			 fprintf(stderr,"sernum=%d \n",meter.sernum);
						 MeterPowerInfo[curr_n].ERC3106State = 1;
						 MeterPowerInfo[curr_n].Valid = 0;
						 memcpy(&MeterPowerInfo[curr_n].tsa,&meter.basicinfo.addr,TSA_LEN);
						 curr_n++;
						 if(curr_n>2)
							 break;
            		 }
               }
             }
    	}else{
    		for(j=0;j<prginfo_event->event_obj.Event3106_obj.poweroff_para_obj.collect_para_obj.tsaarr.num;j++){
    			 MeterPowerInfo[j].ERC3106State = 1;
			     MeterPowerInfo[j].Valid = 0;
			     memcpy(&MeterPowerInfo[j].tsa,&prginfo_event->event_obj.Event3106_obj.poweroff_para_obj.collect_para_obj.tsaarr.meter_tas[j],TSA_LEN);
    		}

    	}
    	*state=1;
    	fprintf(stderr,"state=%d \n",*state);
    }else
    	return 0;
	return 1;
}
/*
 * 终端停/上电事件5-停电事件-放在交采模块
 */
INT8U Event_3106(ProgramInfo* prginfo_event,MeterPower *MeterPowerInfo,INT8U *state) {
	if(oi_chg.oi3106 != prginfo_event->oi_changed.oi3106){
	    readCoverClass(0x3106,0,&prginfo_event->event_obj.Event3106_obj,sizeof(prginfo_event->event_obj.Event3106_obj),event_para_save);
		oi_chg.oi3106 = prginfo_event->oi_changed.oi3106;
	}
	if (prginfo_event->event_obj.Event3106_obj.event_obj.enableflag == 0) {
		return 0;
	}
	BOOLEAN gpio_5V=pwr_has();
	time_t time_of_now;
	time_of_now = time(NULL);
	INT8U flag = 0;
	static INT8U off_time = 0;
	fileread(ERC3106PATH,&TermialPowerInfo,sizeof(TermialPowerInfo));
	INT16U poweroff_happen_vlim=prginfo_event->event_obj.Event3106_obj.poweroff_para_obj.screen_para_obj.happen_voltage_limit;
	INT16U recover_voltage_limit=prginfo_event->event_obj.Event3106_obj.poweroff_para_obj.screen_para_obj.recover_voltage_limit;
	INT16U mintime_space=prginfo_event->event_obj.Event3106_obj.poweroff_para_obj.screen_para_obj.mintime_space;
	INT16U maxtime_space=prginfo_event->event_obj.Event3106_obj.poweroff_para_obj.screen_para_obj.maxtime_space;
	if(*state == 2){
		MeterDiff(prginfo_event,MeterPowerInfo,state);
		*state=0;
	}
	INT8U off_flag=0,on_flag=0;
	//判断下电
	if(TermialPowerInfo.ERC3106State == POWER_START){
		if(prginfo_event->cfg_para.device == 2){//II型
			if(((prginfo_event->ACSRealData.Available==TRUE)
							&&(prginfo_event->ACSRealData.Ua>100
									&& prginfo_event->ACSRealData.Ua<poweroff_happen_vlim)) ||
					(prginfo_event->ACSRealData.Ua <=100))
				off_flag=1;
		}else{
			if((((prginfo_event->ACSRealData.Ua<poweroff_happen_vlim)&&(prginfo_event->ACSRealData.Ub<poweroff_happen_vlim)
									&&(prginfo_event->ACSRealData.Uc<poweroff_happen_vlim))&&((prginfo_event->ACSRealData.Ua|prginfo_event->ACSRealData.Ub|prginfo_event->ACSRealData.Uc)>0)&&gpio_5V)
									||((prginfo_event->ACSRealData.Available == TRUE&&prginfo_event->ACSRealData.Ua==0&&prginfo_event->ACSRealData.Ua<poweroff_happen_vlim)
											&&(prginfo_event->ACSRealData.Available == TRUE&&prginfo_event->ACSRealData.Ub==0&&prginfo_event->ACSRealData.Ub<poweroff_happen_vlim)
											&&(prginfo_event->ACSRealData.Available == TRUE&&prginfo_event->ACSRealData.Uc==0&&prginfo_event->ACSRealData.Uc<poweroff_happen_vlim)&&(!gpio_5V)))
				off_flag=1;
		}
        if(off_flag == 1)
		{
			off_time++;
			if(off_time <5)
				return 0;
			off_time=0;
			//电压低于限值，且底板有电，产生下电事件
			TermialPowerInfo.ERC3106State = POWER_OFF;
			flag = 0b10000000;
			localtime_r((const time_t*)&time_of_now, &TermialPowerInfo.PoweroffTime);
			ERC3106log(0,prginfo_event->ACSRealData.Ua,TermialPowerInfo.PoweroffTime);//调试加入log

			filewrite(ERC3106PATH,&TermialPowerInfo,sizeof(TermialPowerInfo));
			SendERC3106(flag,0,prginfo_event);
		}
	}else if(TermialPowerInfo.ERC3106State == POWER_OFF){
		if(prginfo_event->cfg_para.device == 2){//II型
			if((prginfo_event->ACSRealData.Available && prginfo_event->ACSRealData.Ua>recover_voltage_limit && recover_voltage_limit>0)
					||(prginfo_event->ACSRealData.Available && prginfo_event->ACSRealData.Ua>180))
				on_flag=1;
		}else{
			if((prginfo_event->ACSRealData.Available&&prginfo_event->ACSRealData.Ua>recover_voltage_limit)
			        			||(prginfo_event->ACSRealData.Available&&prginfo_event->ACSRealData.Ua>recover_voltage_limit)
			        				||(prginfo_event->ACSRealData.Available&&prginfo_event->ACSRealData.Ua>recover_voltage_limit))
				on_flag=1;
		}
		if(on_flag == 1)
		{
			if(Get_meter_powoffon(prginfo_event,MeterPowerInfo,state,time_of_now,
					maxtime_space,mintime_space,&flag) == 1){
				TermialPowerInfo.Valid = POWER_START;
				filewrite(ERC3106PATH,&TermialPowerInfo,sizeof(TermialPowerInfo));
				return 0;
			}
			TermialPowerInfo.ERC3106State = POWER_START;
			filewrite(ERC3106PATH,&TermialPowerInfo,sizeof(TermialPowerInfo));
			SendERC3106(flag,1,prginfo_event);
		}
	}else{
		int interval = difftime(mktime(&TermialPowerInfo.PoweronTime),mktime(&TermialPowerInfo.PoweroffTime));
		fprintf(stderr,"\nTermialPowerInfo.Valid=%d interval=%d mintime_space=%d maxtime_space=%d ",TermialPowerInfo.Valid,
				interval,mintime_space*60,maxtime_space*60);

		if(TermialPowerInfo.Valid == POWER_OFF_VALIDE)
		{
			//如果上电时间大于停电时间或者停上电时间间隔小于最小间隔或者大于最大间隔不产生下电事件
			if((interval > mintime_space*60)&&(interval < maxtime_space*60)){
//#define ZHEJIANG
#ifdef ZHEJIANG
				flag = 0b10000000;
				SendERC3106(flag,1,prginfo_event);
				sleep(3);
#endif
				flag = 0b11000000;
			}
			else
				flag = 0b01000000;
			//如果判断停电事件无效
			SendERC3106(flag,1,prginfo_event);
			TermialPowerInfo.ERC3106State = POWER_START;
			TermialPowerInfo.Valid = POWER_START;
			filewrite(ERC3106PATH,&TermialPowerInfo,sizeof(TermialPowerInfo));
		}
		else if(TermialPowerInfo.Valid == POWER_OFF_INVALIDE)
		{
			fprintf(stderr,"\nTermialPowerInfo.Valid=%d",TermialPowerInfo.Valid);
			//如果上电时间大于停电时间或者停上电时间间隔小于最小间隔或者大于最大间隔不产生下电事件
			if((interval > mintime_space*60)&&(interval < maxtime_space*60))
				flag = 0b10000000;
			else
				flag = 0;

			TermialPowerInfo.ERC3106State = POWER_START;
			TermialPowerInfo.Valid = POWER_START;
			//如果判断停电事件无效
			SendERC3106(flag,1,prginfo_event);
			filewrite(ERC3106PATH,&TermialPowerInfo,sizeof(TermialPowerInfo));
		}
		else
		{
			int interval_limit = prginfo_event-incompatible>event_obj.Event3106_obj.poweroff_para_obj.collect_para_obj.time_threshold;
			if(interval_limit==0)
				interval_limit = 5;
			//如果抄表超时还未抄回,直接上报无效上电事件
			if(difftime(time_of_now,mktime(&TermialPowerInfo.PoweronTime))>(interval_limit*60))
			{
				fprintf(stderr,"抄电表停上电时间超时未回,上报无效停电事件 \n");
				TermialPowerInfo.Valid = POWER_OFF_INVALIDE;
				filewrite(ERC3106PATH,&TermialPowerInfo,sizeof(TermialPowerInfo));
			}
		}
	}
    return 1;
}

/*
 * 分析交采数据，产生对应的配置事件。
 */
INT8U Event_AnalyseACS(INT8U* data,INT8U len) {
    return 1;
}

/*
 * 终端直流模拟量越上限事件6 data为直流模拟量 字节高到低
 */
INT8U Event_3107(INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi3107 != prginfo_event->oi_changed.oi3107){
		readCoverClass(0x3107,0,&prginfo_event->event_obj.Event3107_obj,sizeof(prginfo_event->event_obj.Event3107_obj),event_para_save);
		oi_chg.oi3107 = prginfo_event->oi_changed.oi3107;
	}
    if (prginfo_event->event_obj.Event3107_obj.event_obj.enableflag == 0) {
        return 0;
    }
    static INT8U flag=0;
    INT32S moniliang=(data[0]<<24)+(data[1]<<16)+(data[2]<<8)+data[3];
    INT32S offset=prginfo_event->event_obj.Event3107_obj.analogtop_obj.top_limit;
    if(moniliang>offset){
    	if(flag==0)
    	{
			flag=1;
			INT8U Save_buf[256];
			bzero(Save_buf, sizeof(Save_buf));
			//更新当前记录数
			prginfo_event->event_obj.Event3107_obj.event_obj.crrentnum++;
			prginfo_event->event_obj.Event3107_obj.event_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event3107_obj.event_obj.crrentnum,prginfo_event->event_obj.Event3107_obj.event_obj.maxnum);
			INT32U crrentnum = prginfo_event->event_obj.Event3107_obj.event_obj.crrentnum;
			INT8U index=0;
			//标准数据单元
			INT8U oad[4]={0xF2,0x04,0x02,0x00};
			Get_StandardUnit(prginfo_event,0x3107,Save_buf,&index,crrentnum,(INT8U*)oad,s_oad);
			//属性3无关联数据
			Save_buf[STANDARD_NUM_INDEX]+=0;
			//存储更改后得参数
			saveCoverClass(0x3107,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event3107_obj,sizeof(Event3107_Object),event_para_save);
			//存储记录集
			saveCoverClass(0x3107,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
			//存储当前记录值
			INT8U Currbuf[50]={};memset(Currbuf,0,50);
			INT8U Currindex=0;
			Get_CurrResult(Currbuf,&Currindex,(INT8U*)oad,s_oad,crrentnum,0);
			saveCoverClass(0x3107,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
			//判断是否要上报
			if(prginfo_event->event_obj.Event3107_obj.event_obj.reportflag)
				Need_Report(0x3107,crrentnum,prginfo_event);
    	}
    }else
    	flag=0;
    return 1;
}

/*
 * 终端直流模拟量越下限事件7 data为直流模拟量 字节高到低
 */
INT8U Event_3108(INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi3108 != prginfo_event->oi_changed.oi3108){
		readCoverClass(0x3108,0,&prginfo_event->event_obj.Event3108_obj,sizeof(prginfo_event->event_obj.Event3108_obj),event_para_save);
		oi_chg.oi3108 = prginfo_event->oi_changed.oi3108;
	}
    if (prginfo_event->event_obj.Event3108_obj.event_obj.enableflag == 0) {
        return 0;
    }
    static INT8U flag=0;
    INT32S moniliang=(data[0]<<24)+(data[1]<<16)+(data[2]<<8)+data[3];
	INT32S offset=prginfo_event->event_obj.Event3108_obj.analogbom_obj.bom_limit;
	if(moniliang<offset){
		if(flag==0){
			flag=1;
			INT8U Save_buf[256];
			bzero(Save_buf, sizeof(Save_buf));
			//更新当前记录数
			prginfo_event->event_obj.Event3108_obj.event_obj.crrentnum++;
			prginfo_event->event_obj.Event3108_obj.event_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event3108_obj.event_obj.crrentnum,prginfo_event->event_obj.Event3108_obj.event_obj.maxnum);
			INT32U crrentnum = prginfo_event->event_obj.Event3108_obj.event_obj.crrentnum;
			INT8U index=0;
			//标准数据单元
			INT8U oad[4]={0xF2,0x04,0x02,0x00};
			Get_StandardUnit(prginfo_event,0x3108,Save_buf,&index,crrentnum,(INT8U*)oad,s_oad);
			//属性3无关联数据
			Save_buf[STANDARD_NUM_INDEX]+=0;
			//存储更改后得参数
			saveCoverClass(0x3108,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event3108_obj,sizeof(Event3108_Object),event_para_save);
			//存储记录集
			saveCoverClass(0x3108,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
			//存储当前记录值
			INT8U Currbuf[50]={};memset(Currbuf,0,50);
			INT8U Currindex=0;
			Get_CurrResult(Currbuf,&Currindex,(INT8U*)oad,s_oad,crrentnum,0);
			saveCoverClass(0x3108,(INT16U)crreincompatiblentnum,(void *)Currbuf,(int)Currindex,event_current_save);
			//判断是否要上报
			if(prginfo_event->event_obj.Event3108_obj.event_obj.reportflag)
				Need_Report(0x3108,crrentnum,prginfo_event);
		}
	}else
		flag=0;
    return 1;
}

/*
 * 终端消息认证错误事件8 data事件发生前安全认证密码(不含数据类型) len为长度
 */
INT8U Event_3109(INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi3109 != prginfo_event->oi_changed.oi3109){
		readCoverClass(0x3109,0,&prginfo_event->event_obj.Event3109_obj,sizeof(prginfo_event->event_obj.Event3109_obj),event_para_save);
		oi_chg.oi3109 = prginfo_event->oi_changed.oi3109;
	}
    if (prginfo_event->event_obj.Event3109_obj.enableflag == 0) {
        return 0;
    }
    if(1){
    	INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		//更新当前记录数
		prginfo_event->event_obj.Event3109_obj.crrentnum++;
		prginfo_event->event_obj.Event3109_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event3109_obj.crrentnum,prginfo_event->event_obj.Event3109_obj.maxnum);
		INT32U crrentnum = prginfo_event->event_obj.Event3109_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(prginfo_event,0x3109,Save_buf,&index,crrentnum,NULL,s_null);
		//事件发生前安全认证密码
		Save_buf[index++]=dtvisiblestring;//visable-string
		Save_buf[index++]=len;
		memcpy(&Save_buf[index],data,len);
		index+=len;
		Save_buf[STANDARD_NUM_INDEX]+=1;
		//存储更改后得参数
		saveCoverClass(0x3109,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event3109_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3109,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};memset(Currbuf,0,50);
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
		saveCoverClass(0x3109,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(prginfo_event->event_obj.Event3109_obj.reportflag)
			Need_Report(0x3109,crrentnum,prginfo_event);
    }
    return 1;
}

/*
 * 设备故障事件 errtype：0,1,2,3,4,5
 */
INT8U Event_310A(MachineError_type errtype,ProgramInfo* prginfo_event) {
	if(oi_chg.oi310A != prginfo_event->oi_changed.oi310A){
		readCoverClass(0x310A,0,&prginfo_event->event_obj.Event310A_obj,sizeof(prginfo_event->event_obj.Event310A_obj),event_para_save);
		oi_chg.oi310A = prginfo_event->oi_changed.oi310A;
	}
    if (prginfo_event->event_obj.Event310A_obj.enableflag == 0) {
        return 0;
    }
    INT8U Source=errtype;
    INT8U Save_buf[256];
    bzero(Save_buf, sizeof(Save_buf));
    //更新当前记录数
    prginfo_event->event_obj.Event310A_obj.crrentnum++;
    prginfo_event->event_obj.Event310A_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event310A_obj.crrentnum,prginfo_event->event_obj.Event310A_obj.maxnum);
    INT32U crrentnum = prginfo_event->event_obj.Event310A_obj.crrentnum;
    INT8U index=0;
	//标准数据单元
	Get_StandardUnit(prginfo_event,0x310A,Save_buf,&index,crrentnum,(INT8U*)&Source,s_enum);
	//无属性3关联数据
	Save_buf[STANDARD_NUM_INDEX]+=0;
	//存储更改后得参数
	saveCoverClass(0x310A,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event310A_obj,sizeof(Class7_Object),event_para_save);
	//存储记录集
	saveCoverClass(0x310A,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
	//存储当前记录值
	INT8U Currbuf[50]={};memset(Currbuf,0,50);
	INT8U Currindex=0;
	Get_CurrResult(Currbuf,&Currindex,(INT8U*)&Source,s_enum,crrentnum,0);
	saveCoverClass(0x310A,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
	//判断是否要上报
	if(prginfo_event->event_obj.Event310A_obj.reportflag)
		Need_Report(0x310A,crrentnum,prginfo_event);
    return 1;
}

/*
 * 电能表示度下降事件10 前台两次电能值对比是否超过设定值
 */
INT8U Event_310B(TSA tsa, INT8U taskno,INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi310B != prginfo_event->oi_changed.oi310B){
		readCoverClass(0x310B,0,&prginfo_event->event_obj.Event310B_obj,sizeof(prginfo_event->event_obj.Event310B_obj),event_para_save);
		oi_chg.oi310B = prginfo_event->oi_changed.oi310B;
	}
	fprintf(stderr,"[310B]taskno=%d len=%d meter_down_obj.task_no=%d \n",taskno,len,prginfo_event->event_obj.Event310B_obj.meter_down_obj.task_no);
	fprintf(stderr,"[310B]prginfo_event->event_obj.Event310B_obj.event_obj.enableflag=%d \n",prginfo_event->event_obj.Event310B_obj.event_obj.enableflag);
    if (prginfo_event->event_obj.Event310B_obj.event_obj.enableflag == 0) {
        return 0;
    }
//    if(prginfo_event->event_obj.Event310B_obj.meter_down_obj.task_no!=taskno)
//    	return 0;

    if(data==NULL)
    	return 0;
    INT32U newdata=(data[0]<<24)+(data[1]<<16)+(data[2]<<8)+data[3];
    INT32U olddata=0;
    TS ts;
    if(Get_Mdata(tsa,&olddata,&ts,3) == 0){
    	Refresh_Data(tsa,newdata,3);//更新数据
    	return 0;
    }
    fprintf(stderr,"[310B]olddata=%d newdata=%d \n",olddata,newdata);
	if(olddata>newdata){
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		prginfo_event->event_obj.Event310B_obj.event_obj.crrentnum++;
		prginfo_event->event_obj.Event310B_obj.event_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event310B_obj.event_obj.crrentnum,prginfo_event->event_obj.Event310B_obj.event_obj.maxnum);
		INT32U crrentnum = prginfo_event->event_obj.Event310B_obj.event_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(prginfo_event,0x310B,Save_buf,&index,crrentnum,(INT8U*)&tsa,s_tsa);
		//属性3有关联数据
		Save_buf[index++]=dtdoublelongunsigned;//double-long-unsigned
		Save_buf[index++]=(olddata>>24)&0x000000ff;
		Save_buf[index++]=(olddata>>16)&0x000000ff;
		Save_buf[index++]=(olddata>>8)&0x000000ff;
		Save_buf[index++]=olddata&0x000000ff;
		Save_buf[index++]=dtdoublelongunsigned;//double-long-unsigned
		Save_buf[index++]=(newdata>>24)&0x000000ff;
		Save_buf[index++]=(newdata>>16)&0x000000ff;
		Save_buf[index++]=(newdata>>8)&0x000000ff;
		Save_buf[index++]=newdata&0x000000ff;
		Save_buf[STANDARD_NUM_INDEX]+=2;

		//存储更改后得参数
		saveCoverClass(0x310B,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event310B_obj,sizeof(Event310B_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x310B,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};memset(Currbuf,0,50);
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,(INT8U*)&tsa,s_tsa,crrentnum,0);
		saveCoverClass(0x310B,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(prginfo_event->event_obj.Event310B_obj.event_obj.reportflag)
			Need_Report(0x310B,crrentnum,prginfo_event);
	}
    //更新数据
	if(olddata!=newdata)
		Refresh_Data(tsa,newdata,3);
    return 1;
}

/*
 * 电能量超差事件11 前台两次电能值以及测量点额定电压、电流
 */
INT8U Event_310C(TSA tsa, INT8U taskno,INT8U* data,INT8U len,ProgramInfo* prginfo_event,CLASS_6001 meter)
{
	if(oi_chg.oi310C != prginfo_event->oi_changed.oi310C){
		readCoverClass(0x310C,0,&prginfo_event->event_obj.Event310C_obj,sizeof(prginfo_event->event_obj.Event310C_obj),event_para_save);
		oi_chg.oi310C = prginfo_event->oi_changed.oi310C;
	}
    if (prginfo_event->event_obj.Event310C_obj.event_obj.enableflag == 0) {
        return 0;
    }
//    if(prginfo_event->event_obj.Event310C_obj.poweroffset_obj.task_no!=taskno)
//       	return 0;
    if(data==NULL)
    	return 0;
    INT32U newdata=(data[0]<<24)+(data[1]<<16)+(data[2]<<8)+data[3];
	INT32U olddata=0;
	TS ts;
	if(Get_Mdata(tsa,&olddata,&ts,1) == 0){
		Refresh_Data(tsa,newdata,1);//更新数据
		return 0;
	}
	/*===============TODO根据共享内存或者直接读取文件 获取该表参数*/
   // CLASS_6001 meter={};
    INT32U power_offset=prginfo_event->event_obj.Event310C_obj.poweroffset_obj.power_offset;//超差值
    INT16U ratedU=meter.basicinfo.ratedU; //额定电压
    INT16U ratedI=meter.basicinfo.ratedI; //额定电流
    INT8U connectype=meter.basicinfo.connectype;//接线方式
    /*===============TODO可能需要根据下发得任务获取抄表间隔*/
    INT16U Intertime=5; //抄表间隔
    if(meter.basicinfo.port.OI == 0xF209)
    	Intertime *=60;//载波表
    else
    	Intertime =5;//485
    FP32 Em=0;
    switch(connectype){
		case 1://单相
			Em=((ratedU*0.1)*(ratedI*0.1)/(60*60))*(Intertime*60);
			break;
		case 2://三相三线
			Em=((ratedU*0.1)*(ratedI*0.1)*1.732/(60*60))*(Intertime*60);
			break;
		case 3://三相四线
			Em=((ratedU*0.1)*(ratedI*0.1)*3/(60*60))*(Intertime*60);
			break;
    }
    fprintf(stderr,"[310C]newdata=%d olddata=%d \n",newdata,olddata);
    if ((newdata>olddata) && (olddata>0))
	 {
    	//fprintf(stderr,"[310C](newdata-olddata)*1000=%d power_offset*Em=%d power_offset=%d Em=%d\n",(newdata-olddata)*1000,power_offset*Em,power_offset,Em);
		 //kwh转换得扩大1000倍
		 if (((newdata-olddata)*1000>power_offset*Em) && (power_offset>0)){
			INT8U Save_buf[256];
			bzero(Save_buf, sizeof(Save_buf));
			prginfo_event->event_obj.Event310C_obj.event_obj.crrentnum++;
			prginfo_event->event_obj.Event310C_obj.event_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event310C_obj.event_obj.crrentnum,prginfo_event->event_obj.Event310C_obj.event_obj.maxnum);
			INT32U crrentnum = prginfo_event->event_obj.Event310C_obj.event_obj.crrentnum;
			INT8U index=0;
			//标准数据单元
			Get_StandardUnit(prginfo_event,0x310C,Save_buf,&index,crrentnum,(INT8U*)&tsa,s_tsa);
			//属性3有关联数据
			Save_buf[index++]=dtdoublelongunsigned;//double-long-unsigned
			Save_buf[index++]=(olddata>>24)&0x000000ff;
			Save_buf[index++]=(olddata>>16)&0x000000ff;
			Save_buf[index++]=(olddata>>8)&0x000000ff;
			Save_buf[index++]=olddata&0x000000ff;
			Save_buf[index++]=dtdoublelongunsigned;//double-long-unsigned
			Save_buf[index++]=(newdata>>24)&0x000000ff;
			Save_buf[index++]=(newdata>>16)&0x000000ff;
			Save_buf[index++]=(newdata>>8)&0x000000ff;
			Save_buf[index++]=newdata&0x000000ff;
			Save_buf[STANDARD_NUM_INDEX]+=2;
			//存储更改后得参数
			saveCoverClass(0x310C,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event310C_obj,sizeof(Event310C_Object),event_para_save);
			//存储记录集
			saveCoverClass(0x310C,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
			//存储当前记录值
			INT8U Currbuf[50]={};memset(Currbuf,0,50);
			INT8U Currindex=0;
			Get_CurrResult(Currbuf,&Currindex,(INT8U*)&tsa,s_tsa,crrentnum,0);
			saveCoverClass(0x310C,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
			//判断是否要上报
			if(prginfo_event->event_obj.Event310C_obj.event_obj.reportflag)
				Need_Report(0x310C,crrentnum,prginfo_event);
		 }
	 }
    //更新数据
    if(olddata!=newdata)
    	Refresh_Data(tsa,newdata,1);

    return 1;
}

/*
 * 电能表飞走事件12 前台两次电能值以及测量点额定电压、电流
 */
INT8U Event_310D(TSA tsa, INT8U taskno,INT8U* data,INT8U len,ProgramInfo* prginfo_event,CLASS_6001 meter) {
	if(oi_chg.oi310D != prginfo_event->oi_changed.oi310D){
		readCoverClass(0x310D,0,&prginfo_event->event_obj.Event310D_obj,sizeof(prginfo_event->event_obj.Event310D_obj),event_para_save);
		oi_chg.oi310D = prginfo_event->oi_changed.oi310D;
	}
	if (prginfo_event->event_obj.Event310D_obj.event_obj.enableflag == 0) {
	        return 0;
	}
//	 if(prginfo_event->event_obj.Event310D_obj.poweroffset_obj.task_no!=taskno)
//		 return 0;

	if(data==NULL)
		return 0;
	INT32U newdata=(data[0]<<24)+(data[1]<<16)+(data[2]<<8)+data[3];
	INT32U olddata=0;
	TS ts;
	if(Get_Mdata(tsa,&olddata,&ts,2) == 0){
		Refresh_Data(tsa,newdata,2);//更新数据
		return 0;
	}
	/*===============TODO根据共享内存或者直接读取文件 获取该表参数*/
	//CLASS_6001 meter={};
	INT32U power_offset=prginfo_event->event_obj.Event310D_obj.poweroffset_obj.power_offset;//超差值
	INT16U ratedU=meter.basicinfo.ratedU; //额定电压
	INT16U ratedI=meter.basicinfo.ratedI; //额定电流
	INT8U connectype=meter.basicinfo.connectype;//接线方式
	/*===============TODO可能需要根据下发得任务获取抄表间隔*/
	INT16U Intertime=5; //抄表间隔
	if(meter.basicinfo.port.OI == 0xF209)
		Intertime *=60;//载波表
	else
		Intertime =5;//485
	FP32 Em=0;
	switch(connectype){
		case 1://单相
			Em=((ratedU*0.1)*(ratedI*0.1)/(60*60))*(Intertime*60);
			break;
		case 2://三相三线
			Em=((ratedU*0.1)*(ratedI*0.1)*1.732/(60*60))*(Intertime*60);
			break;
		case 3://三相四线
			Em=((ratedU*0.1)*(ratedI*0.1)*3/(60*60))*(Intertime*60);
			break;
	}
	fprintf(stderr,"[310D]newdata=%d olddata=%d \n",newdata,olddata);
	if ((newdata>olddata) && (olddata>0))
	 {
		//fprintf(stderr,"[310D](newdata-olddata)*1000=%d power_offset*Em=%d power_offset=%d Em=%d\n",(newdata-olddata)*1000,power_offset*Em,power_offset,Em);
		 //kwh转换得扩大1000倍
		 if (((newdata-olddata)*1000>power_offset*Em) && (power_offset>0)){
			INT8U Save_buf[256];
			bzero(Save_buf, sizeof(Save_buf));
			prginfo_event->event_obj.Event310D_obj.event_obj.crrentnum++;
			prginfo_event->event_obj.Event310D_obj.event_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event310D_obj.event_obj.crrentnum,prginfo_event->event_obj.Event310D_obj.event_obj.maxnum);
			INT32U crrentnum = prginfo_event->event_obj.Event310D_obj.event_obj.crrentnum;
			INT8U index=0;
			//标准数据单元
			Get_StandardUnit(prginfo_event,0x310D,Save_buf,&index,crrentnum,(INT8U*)&tsa,s_tsa);
			//属性3有关联数据
			Save_buf[index++]=dtdoublelongunsigned;//double-long-unsigned
			Save_buf[index++]=(olddata>>24)&0x000000ff;
			Save_buf[index++]=(olddata>>16)&0x000000ff;
			Save_buf[index++]=(olddata>>8)&0x000000ff;
			Save_buf[index++]=olddata&0x000000ff;
			Save_buf[index++]=dtdoublelongunsigned;//double-long-unsigned
			Save_buf[index++]=(newdata>>24)&0x000000ff;
			Save_buf[index++]=(newdata>>16)&0x000000ff;
			Save_buf[index++]=(newdata>>8)&0x000000ff;
			Save_buf[index++]=newdata&0x000000ff;
			Save_buf[STANDARD_NUM_INDEX]+=2;
			//存储更改后得参数
			saveCoverClass(0x310D,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event310D_obj,sizeof(Event310D_Object),event_para_save);
			//存储记录集
			saveCoverClass(0x310D,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
			//存储当前记录值
			INT8U Currbuf[50]={};memset(Currbuf,0,50);
			INT8U Currindex=0;
			Get_CurrResult(Currbuf,&Currindex,(INT8U*)&tsa,s_tsa,crrentnum,0);
			saveCoverClass(0x310D,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
			//判断是否要上报
			if(prginfo_event->event_obj.Event310D_obj.event_obj.reportflag)
				Need_Report(0x310D,crrentnum,prginfo_event);
		 }
	 }
	//更新数据
	if(olddata!=newdata)
		Refresh_Data(tsa,newdata,2);

	return 1;
}

/*
 * 电能表停走事件 前台两次电能值是否相同以及时间差是否超过设定值
 */
INT8U Event_310E(TSA tsa, INT8U taskno,INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi310E != prginfo_event->oi_changed.oi310E){
		readCoverClass(0x310E,0,&prginfo_event->event_obj.Event310E_obj,sizeof(prginfo_event->event_obj.Event310E_obj),event_para_save);
		oi_chg.oi310E = prginfo_event->oi_changed.oi310E;
	}
    if (prginfo_event->event_obj.Event310E_obj.event_obj.enableflag == 0) {
        return 0;
    }
//    if(prginfo_event->event_obj.Event310E_obj.powerstoppara_obj.task_no!=taskno)
//       	return 0;
    INT32U newdata=(data[0]<<24)+(data[1]<<16)+(data[2]<<8)+data[3];
	INT32U olddata=0;
	TS ts;
	if(Get_Mdata(tsa,&olddata,&ts,0) == 0){
		Refresh_Data(tsa,newdata,0);//更新数据
		return 0;
	}
	if(olddata == newdata){
		//若正向有功值相同
		TS currtime;
		TSGet(&currtime);
		int tcha=abs(difftime(tmtotime_t(ts),tmtotime_t(currtime)));
		fprintf(stderr,"ts:%d-%d-%d %d:%d:%d \n",ts.Year,ts.Month,ts.Day,ts.Hour,ts.Minute,ts.Sec);
		fprintf(stderr,"currtime:%d-%d-%d %d:%d:%d \n",currtime.Year,currtime.Month,currtime.Day,currtime.Hour,currtime.Minute,currtime.Sec);
		INT32U offset=0;
		switch(prginfo_event->event_obj.Event310E_obj.powerstoppara_obj.power_offset.units){
			case 0:
				offset=prginfo_event->event_obj.Event310E_obj.powerstoppara_obj.power_offset.interval;
				break;
			case 1:
				offset=prginfo_event->event_obj.Event310E_obj.powerstoppara_obj.power_offset.interval*60;
				break;
			case 2:
				offset=prginfo_event->event_obj.Event310E_obj.powerstoppara_obj.power_offset.interval*60*60;
				break;
			case 3:
				offset=prginfo_event->event_obj.Event310E_obj.powerstoppara_obj.power_offset.interval*24*60*60;
				break;
			case 4:
				offset=prginfo_event->event_obj.Event310E_obj.powerstoppara_obj.power_offset.interval*30*24*60*60;
				break;
			case 5:
				offset=prginfo_event->event_obj.Event310E_obj.powerstoppara_obj.power_offset.interval*365*24*60*60;
				break;
			}
		fprintf(stderr,"[310E]tcha=%d offset=%d units=%d \n",tcha,offset,prginfo_event->event_obj.Event310E_obj.powerstoppara_obj.power_offset.units);
		if(tcha>offset && offset>0){
			INT8U Save_buf[256];
			bzero(Save_buf, sizeof(Save_buf));
			prginfo_event->event_obj.Event310E_obj.event_obj.crrentnum++;
			prginfo_event->event_obj.Event310E_obj.event_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event310E_obj.event_obj.crrentnum,prginfo_event->event_obj.Event310E_obj.event_obj.maxnum);
		    INT32U crrentnum = prginfo_event->event_obj.Event310E_obj.event_obj.crrentnum;
			INT8U index=0;
			//标准数据单元
			Get_StandardUnit(prginfo_event,0x310E,Save_buf,&index,crrentnum,(INT8U*)&tsa,s_tsa);
			//属性3有关联数据
			Save_buf[index++]=dtdoublelongunsigned;//double-long-unsigned
			Save_buf[index++]=(olddata>>24)&0x000000ff;
			Save_buf[index++]=(olddata>>16)&0x000000ff;
			Save_buf[index++]=(olddata>>8)&0x000000ff;
			Save_buf[index++]=olddata&0x000000ff;
			Save_buf[STANDARD_NUM_INDEX]+=1;
			//存储更改后得参数
			saveCoverClass(0x310E,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event310E_obj,sizeof(Event310E_Object),event_para_save);
			//存储记录集
			saveCoverClass(0x310E,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
			//存储当前记录值
			INT8U Currbuf[50]={};memset(Currbuf,0,50);
			INT8U Currindex=0;
			Get_CurrResult(Currbuf,&Currindex,(INT8U*)&tsa,s_tsa,crrentnum,0);
			saveCoverClass(0x310E,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
			//判断是否要上报
			if(prginfo_event->event_obj.Event310E_obj.event_obj.reportflag)
				Need_Report(0x310E,crrentnum,prginfo_event);
		}
	}else
		Refresh_Data(tsa,newdata,0);//更新数据

    return 1;
}

/*
 * 抄表失败事件 抄表可自行判断是否抄表失败，可直接调用该接口
 */
INT8U Event_310F(TSA tsa, INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi310F != prginfo_event->oi_changed.oi310F){
		readCoverClass(0x310F,0,&prginfo_event->event_obj.Event310F_obj,sizeof(prginfo_event->event_obj.Event310F_obj),event_para_save);
		oi_chg.oi310F = prginfo_event->oi_changed.oi310F;
	}
    if (prginfo_event->event_obj.Event310F_obj.event_obj.enableflag == 0) {
        return 0;
    }
    INT8U Save_buf[256];
	bzero(Save_buf, sizeof(Save_buf));
	prginfo_event->event_obj.Event310F_obj.event_obj.crrentnum++;
	prginfo_event->event_obj.Event310F_obj.event_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event310F_obj.event_obj.crrentnum,prginfo_event->event_obj.Event310F_obj.event_obj.maxnum);
	INT32U crrentnum = prginfo_event->event_obj.Event310F_obj.event_obj.crrentnum;
	INT8U index=0;
	//标准数据单元
	Get_StandardUnit(prginfo_event,0x310F,Save_buf,&index,crrentnum,(INT8U*)&tsa,s_tsa);
	//属性3有关联数据
	//最近一次抄表成功时间
	Save_buf[index++]=dtdatetimes;//datetime-s
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	//最近一次正向有功
	Save_buf[index++]=dtdoublelongunsigned;//double-long-unsigned
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	//最近一次正向有功
	Save_buf[index++]=dtdoublelong;//double-long
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	Save_buf[STANDARD_NUM_INDEX]+=3;
	//存储更改后得参数
	saveCoverClass(0x310F,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event310F_obj,sizeof(Event310F_Object),event_para_save);
	//存储记录集
	saveCoverClass(0x310F,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
	//存储当前记录值
	INT8U Currbuf[50]={};memset(Currbuf,0,50);
	INT8U Currindex=0;
	Get_CurrResult(Currbuf,&Currindex,(INT8U*)&tsa,s_tsa,crrentnum,0);
	saveCoverClass(0x310F,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
	//判断是否要上报
	if(prginfo_event->event_obj.Event310F_obj.event_obj.reportflag)
		Need_Report(0x310F,crrentnum,prginfo_event);
    return 1;
}

/*
 * 月通信流量超限事件 data为当月已经发生流量 字节由高到低
 */
INT8U Event_3110(INT32U data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi3110 != prginfo_event->oi_changed.oi3110){
		readCoverClass(0x3110,0,&prginfo_event->event_obj.Event3110_obj,sizeof(prginfo_event->event_obj.Event3110_obj),event_para_save);
		oi_chg.oi3110 = prginfo_event->oi_changed.oi3110;
	}
    if (prginfo_event->event_obj.Event3110_obj.event_obj.enableflag == 0) {
        return 0;
    }
    static INT8U flag=0;
    INT32U offset=prginfo_event->event_obj.Event3110_obj.Monthtrans_obj.month_offset;
    //通信处判断还是这里判断 TODO
    if(data>offset){
    	if(flag==0){
    	    flag=1;
			INT8U Save_buf[256];
			bzero(Save_buf, sizeof(Save_buf));
			prginfo_event->event_obj.Event3110_obj.event_obj.crrentnum++;
			prginfo_event->event_obj.Event3110_obj.event_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event3110_obj.event_obj.crrentnum,prginfo_event->event_obj.Event3110_obj.event_obj.maxnum);
			INT32U crrentnum = prginfo_event->event_obj.Event3110_obj.event_obj.crrentnum;
			INT8U index=0;
			//标准数据单元
			Get_StandardUnit(prginfo_event,0x3110,Save_buf,&index,crrentnum,NULL,s_null);
			//属性3有关联数据
			//事件发生后已发生通信流量 //22004202
			Save_buf[index++]=dtdoublelongunsigned;//double-long-unsiged
			Save_buf[index++]=(data>>24)&0x000000ff;
			Save_buf[index++]=(data>>16)&0x000000ff;
			Save_buf[index++]=(data>>8)&0x000000ff;
			Save_buf[index++]=data&0x000000ff;
			//月通信流量门限 //31100601
			Save_buf[index++]=dtdoublelongunsigned;//double-long-unsigned
			Save_buf[index++]=(offset>>24)&0x000000ff;
			Save_buf[index++]=(offset>>16)&0x000000ff;
			Save_buf[index++]=(offset>>8)&0x000000ff;
			Save_buf[index++]=offset&0x000000ff;
			Save_buf[STANDARD_NUM_INDEX]+=2;
			//存储更改后得参数
			saveCoverClass(0x3110,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event3110_obj,sizeof(Event3110_Object),event_para_save);
			//存储记录集
			saveCoverClass(0x3110,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
			//存储当前记录值
			INT8U Currbuf[50]={};memset(Currbuf,0,50);
			INT8U Currindex=0;
			Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
			saveCoverClass(0x3110,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
			//判断是否要上报
			if(prginfo_event->event_obj.Event3110_obj.event_obj.reportflag)
				Need_Report(0x3110,crrentnum,prginfo_event);
			return 1;
    	}
    }else
    	flag=0;
    return 1;
}

/*
 * 发现未知电能表事件 抄表搜表可以判断出表信息，直接可调用该接口，默认data为整个电能表信息
 */
INT8U Event_3111(TSA tsa, INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi3111 != prginfo_event->oi_changed.oi3111){
		readCoverClass(0x3111,0,&prginfo_event->event_obj.Event3111_obj,sizeof(prginfo_event->event_obj.Event3111_obj),event_para_save);
		oi_chg.oi3111 = prginfo_event->oi_changed.oi3111;
	}
	if (prginfo_event->event_obj.Event3111_obj.enableflag == 0) {
	        return 0;
	    }
	if(data== NULL)
		return 0;
	INT8U Save_buf[256];
	bzero(Save_buf, sizeof(Save_buf));
	prginfo_event->event_obj.Event3111_obj.crrentnum++;
	prginfo_event->event_obj.Event3111_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event3111_obj.crrentnum,prginfo_event->event_obj.Event3111_obj.maxnum);
	INT32U crrentnum = prginfo_event->event_obj.Event3111_obj.crrentnum;
	INT8U index=0;
	//标准数据单元
	Get_StandardUnit(prginfo_event,0x3111,Save_buf,&index,crrentnum,NULL,s_null);
	//搜表结果集
	Save_buf[index++]=dtarray;//array
	Save_buf[index++]=1;//默认搜到一个就产生事件
	Save_buf[index++]=2;//structure
	Save_buf[index++]=7;//元素数量
	//默认data是搜表记录结果结构
	//通信地址 TSA
	Save_buf[index++]=dttsa;//TSA
	INT8U l1=data[0];
	memcpy(&Save_buf[index],&data[0],l1+1);
	index+=l1+1;
	//所属采集器地址 TSA
	Save_buf[index++]=dttsa;//TSA
	INT8U l2=data[l1+1];
	memcpy(&Save_buf[index],&data[l1+1],l2+1);
	index+=l2+1;
	//规约类型  enum
	Save_buf[index++]=dtenum;
	Save_buf[index++]=data[l1+1+l2+1];
	//相位 enum{未知（0），A（1），B（2），C（3）}
	Save_buf[index++]=dtenum;
	Save_buf[index++]=data[l1+1+l2+1+1];
	//信号品质unsigned，
	Save_buf[index++]=dtunsigned;
	Save_buf[index++]=data[l1+1+l2+1+2];
    //搜到的时间 date_time_s
	Save_buf[index++]=dtdatetimes;
	memcpy(&Save_buf[index],&data[l1+1+l2+1+3],7);
	index+=7;
	//搜到的附加信息  array附加信息
	Save_buf[index++]=dtarray;//array
	Save_buf[index++]=0;//数量
	Save_buf[STANDARD_NUM_INDEX]+=1;
	//存储更改后得参数
	saveCoverClass(0x3111,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event3111_obj,sizeof(Class7_Object),event_para_save);
	//存储记录集
	saveCoverClass(0x3111,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
	//存储当前记录值
	INT8U Currbuf[50]={};memset(Currbuf,0,50);
	INT8U Currindex=0;
	Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
	saveCoverClass(0x3111,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
	//判断是否要上报
	if(prginfo_event->event_obj.Event3111_obj.reportflag)
		Need_Report(0x3111,crrentnum,prginfo_event);
    return 1;
}

/*
 * 跨台区电能表事件17 抄表搜表可以判断出表信息，直接可调用该接口，默认data为整个垮台区电能表信息
 */
INT8U Event_3112(TSA tsa, INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi3112 != prginfo_event->oi_changed.oi3112){
		readCoverClass(0x3112,0,&prginfo_event->event_obj.Event3112_obj,sizeof(prginfo_event->event_obj.Event3112_obj),event_para_save);
		oi_chg.oi3112 = prginfo_event->oi_changed.oi3112;
	}
	if (prginfo_event->event_obj.Event3112_obj.enableflag == 0)
		return 0;
	if(data== NULL)
		return 0;
	INT8U Save_buf[256];
	bzero(Save_buf, sizeof(Save_buf));
	prginfo_event->event_obj.Event3112_obj.crrentnum++;
	prginfo_event->event_obj.Event3112_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event3112_obj.crrentnum,prginfo_event->event_obj.Event3112_obj.maxnum);
	INT32U crrentnum = prginfo_event->event_obj.Event3112_obj.crrentnum;
	INT8U index=0;
	//标准数据单元
	Get_StandardUnit(prginfo_event,0x3112,Save_buf,&index,crrentnum,NULL,s_null);
	//结果集
	Save_buf[index++]=dtarray;//array
	Save_buf[index++]=1;//默认搜到一个就产生事件
	Save_buf[index++]=2;//structure
	Save_buf[index++]=3;//元素数量
	//默认data是搜表记录结果结构
    //通信地址 TSA
	Save_buf[index++]=dttsa;//TSA
	INT8U l1=data[0];
	memcpy(&Save_buf[index],&data[0],l1+1);
	index+=l1+1;
	//所属采集器地址 TSA
	Save_buf[index++]=dttsa;//TSA
	INT8U l2=data[l1+1];
	memcpy(&Save_buf[index],&data[l1+1],l2+1);
	index+=l2+1;
	//变更时间 date_time_s
	Save_buf[index++]=dtdatetimes;
	memcpy(&Save_buf[index],&data[l1+1+l2+1],7);
	index+=7;
	Save_buf[STANDARD_NUM_INDEX]+=1;
	//存储更改后得参数
	saveCoverClass(0x3112,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event3112_obj,sizeof(Class7_Object),event_para_save);
	//存储记录集
	saveCoverClass(0x3112,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
	//存储当前记录值
	INT8U Currbuf[50]={};memset(Currbuf,0,50);
	INT8U Currindex=0;
	Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
	saveCoverClass(0x3112,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
	//判断是否要上报
	if(prginfo_event->event_obj.Event3112_obj.reportflag)
		Need_Report(0x3112,crrentnum,prginfo_event);
    return 1;
}

/*
 * 电能表在网状态切换事件24 怎么判断TODO？ data为电能表地址TSA及在网状态bool
 */
INT8U Event_311A(TSA tsa, INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi311A != prginfo_event->oi_changed.oi311A){
		readCoverClass(0x311A,0,&prginfo_event->event_obj.Event311A_obj,sizeof(prginfo_event->event_obj.Event311A_obj),event_para_save);
		oi_chg.oi311A = prginfo_event->oi_changed.oi311A;
	}
	if (prginfo_event->event_obj.Event311A_obj.event_obj.enableflag == 0)
		return 0;
	if(data== NULL)
		return 0;
	//怎么判定？
	//INT16U time_delay=prginfo_event->event_obj.Event311A_obj.outtimepara_obj.outtime_offset;
	//如果发生
	if(1){
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		prginfo_event->event_obj.Event311A_obj.event_obj.crrentnum++;
		prginfo_event->event_obj.Event311A_obj.event_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event311A_obj.event_obj.crrentnum,prginfo_event->event_obj.Event311A_obj.event_obj.maxnum);
		INT32U crrentnum = prginfo_event->event_obj.Event311A_obj.event_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(prginfo_event,0x311A,Save_buf,&index,crrentnum,NULL,s_null);
		//状态变迁事件
		Save_buf[index++]=dtarray;//array
		Save_buf[index++]=1;//默认一个状态变迁事件
		Save_buf[index++]=2;//structure
		Save_buf[index++]=2;//元素数量
		//默认data是记录结果结构
		//电能表地址 TSA
		Save_buf[index++]=dttsa;//TSA
		INT8U l1=data[0];
		memcpy(&Save_buf[index],&data[0],l1+1);
		index+=l1+1;
		//在网状态 bool
		Save_buf[index++]=dtbool;//bool
		Save_buf[index++]=data[l1+1];
		Save_buf[STANDARD_NUM_INDEX]+=1;
		//存储更改后得参数
		saveCoverClass(0x311A,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event311A_obj,sizeof(Event311A_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x311A,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};memset(Currbuf,0,50);
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
		saveCoverClass(0x311A,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(prginfo_event->event_obj.Event311A_obj.event_obj.reportflag)
			Need_Report(0x311A,crrentnum,prginfo_event);
	}
    return 1;
}

/*
 * 终端对电表校时记录 抄表是否可以自行判断是较时？可直接调用该接口 data为较时前电表时间及误差
 */
INT8U Event_311B(TSA tsa, INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi311B != prginfo_event->oi_changed.oi311B){
		readCoverClass(0x311B,0,&prginfo_event->event_obj.Event311B_obj,sizeof(prginfo_event->event_obj.Event311B_obj),event_para_save);
		oi_chg.oi311B = prginfo_event->oi_changed.oi311B;
	}
	if (prginfo_event->event_obj.Event311B_obj.enableflag == 0)
		return 0;
	if(data== NULL)
		return 0;
    if(1){
    	INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		prginfo_event->event_obj.Event311B_obj.crrentnum++;
		prginfo_event->event_obj.Event311B_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event311B_obj.crrentnum,prginfo_event->event_obj.Event311B_obj.maxnum);
		INT32U crrentnum = prginfo_event->event_obj.Event311B_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(prginfo_event,0x311B,Save_buf,&index,crrentnum,(INT8U*)&tsa,s_tsa);
		//校时前时钟    date_time_s
		Save_buf[index++]=dtdatetimes;
		memcpy(&Save_buf[index],data,7);
		index+=7;
		//时钟误差      integer（单位：秒，无换算)
		Save_buf[index++]=dtinteger;
		Save_buf[index++]=data[7];
		Save_buf[STANDARD_NUM_INDEX]+=2;
		//存储更改后得参数
		saveCoverClass(0x311B,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event311B_obj,sizeof(Class7_Object),1);
		//存储记录集
		saveCoverClass(0x311B,(INT16U)crrentnum,(void *)Save_buf,(int)index,2);
		//存储当前记录值
		INT8U Currbuf[50]={};memset(Currbuf,0,50);
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,(INT8U*)&tsa,s_tsa,crrentnum,0);
		saveCoverClass(0x311B,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,3);
		//判断是否要上报
		if(prginfo_event->event_obj.Event311B_obj.reportflag)
			Need_Report(0x311B,crrentnum,prginfo_event);
    }
    return 1;
}

/*
 * 电能表数据变更监控记录 抄表可自行判断，直接调用该函数。
 */
INT8U Event_311C(TSA tsa, INT8U taskno,OAD oad,INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi311C != prginfo_event->oi_changed.oi311C){
		readCoverClass(0x311C,0,&prginfo_event->event_obj.Event311C_obj,sizeof(prginfo_event->event_obj.Event311C_obj),event_para_save);
		oi_chg.oi311C = prginfo_event->oi_changed.oi311C;
	}
	if (prginfo_event->event_obj.Event311C_obj.event_obj.enableflag == 0)
		return 0;
	if(prginfo_event->event_obj.Event311C_obj.task_para.task_no!=taskno)
		return 0;

	if(data== NULL)
		return 0;

	INT8U i=0,happen_flag=0,have_flag=0;
	INT8U *oldata;
	for(i=0;i<50;i++){
		if(memcmp(&other_data[i].tsa,&tsa,sizeof(TSA))==0
				&& memcmp(&other_data[i].oad,&oad,sizeof(OAD))==0){
			have_flag=1;
            if(memcmp(data,other_data[i].data,len)!=0){
            	happen_flag=1;
            	oldata = other_data[i].data;
            	memcpy(other_data[i].data,data,len);
            }
            break;
		}
	}
	if(have_flag == 0){
		memcpy(&other_data[other_curr_index].tsa,&tsa,sizeof(TSA));
		memcpy(&other_data[other_curr_index].oad,&oad,4);
		memcpy(other_data[other_curr_index].data,data,len);
		other_curr_index++;
		if(other_curr_index>=50)
			other_curr_index = 0;
	}
	if(happen_flag == 1){
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		prginfo_event->event_obj.Event311C_obj.event_obj.crrentnum++;
		prginfo_event->event_obj.Event311C_obj.event_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event311C_obj.event_obj.crrentnum,prginfo_event->event_obj.Event311C_obj.event_obj.maxnum);
		INT32U crrentnum = prginfo_event->event_obj.Event311C_obj.event_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(prginfo_event,0x311C,Save_buf,&index,crrentnum,(INT8U*)&tsa,s_tsa);
		//CSD
		Save_buf[index++] = dtcsd;//CSD
		Save_buf[index++] = 0; //OAD
		memcpy(&Save_buf[index],&oad,4);
		index +=4;
		memcpy(&Save_buf[index],oldata,len);//旧数据
		index +=len;
		memcpy(&Save_buf[index],data,len);//新数据
		index +=len;
		Save_buf[STANDARD_NUM_INDEX]+=3;
		//存储更改后得参数
		saveCoverClass(0x311C,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event311C_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x311C,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};memset(Currbuf,0,50);
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,(INT8U*)&tsa,s_tsa,crrentnum,0);
		saveCoverClass(0x311C,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(prginfo_event->event_obj.Event311C_obj.event_obj.reportflag)
			Need_Report(0x311C,crrentnum,prginfo_event);
	}
    return 1;
}

/*
 * 终端对时事件 此接口在698规约库调用，data为对时前时间 date-time-s格式 7个字节
 */
INT8U Event_3114(DateTimeBCD data,ProgramInfo* prginfo_event) {
	if(oi_chg.oi3114 != prginfo_event->oi_changed.oi3114){
		readCoverClass(0x3114,0,&prginfo_event->event_obj.Event3114_obj,sizeof(prginfo_event->event_obj.Event3114_obj),event_para_save);
		oi_chg.oi3114 = prginfo_event->oi_changed.oi3114;
	}
	fprintf(stderr,"libevent:3114enableflag=%d \n",prginfo_event->event_obj.Event3114_obj.enableflag);
    if (prginfo_event->event_obj.Event3114_obj.enableflag == 0) {
        return 0;
    }

    if(1){
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		prginfo_event->event_obj.Event3114_obj.crrentnum++;
		prginfo_event->event_obj.Event3114_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event3114_obj.crrentnum,prginfo_event->event_obj.Event3114_obj.maxnum);
		INT32U crrentnum = prginfo_event->event_obj.Event3114_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(prginfo_event,0x3114,Save_buf,&index,crrentnum,NULL,s_null);
		//事件发生前对时时间
		Save_buf[index++]=dtdatetimes;
		Save_buf[index++] = ((data.year.data>>8)&0x00ff);
		Save_buf[index++] = ((data.year.data)&0x00ff);
		Save_buf[index++] = data.month.data;
		Save_buf[index++] = data.day.data;
		Save_buf[index++] = data.hour.data;
		Save_buf[index++] = data.min.data;
		Save_buf[index++] = data.sec.data;
		//事件发生后对时时间
		DateTimeBCD ntime;
		DataTimeGet(&ntime);
		Save_buf[index++]=dtdatetimes;
		Save_buf[index++] = ((ntime.year.data>>8)&0x00ff);
		Save_buf[index++] = ((ntime.year.data)&0x00ff);
		Save_buf[index++] = ntime.month.data;
		Save_buf[index++] = ntime.day.data;
		Save_buf[index++] = ntime.hour.data;
		Save_buf[index++] = ntime.min.data;
		Save_buf[index++] = ntime.sec.data;
		Save_buf[STANDARD_NUM_INDEX]+=2;
		//存储更改后得参数
		saveCoverClass(0x3114,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event3114_obj,sizeof(Class7_Object),1);
		//存储记录集
		saveCoverClass(0x3114,(INT16U)crrentnum,(void *)Save_buf,(int)index,2);
		//存储当前记录值
		INT8U Currbuf[50]={};memset(Currbuf,0,50);
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
		saveCoverClass(0x3114,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,3);
		//判断是否要上报
		if(prginfo_event->event_obj.Event3114_obj.reportflag)
			Need_Report(0x3114,crrentnum,prginfo_event);
    }
    return 1;
}

/*
 * 遥控跳闸记录
 */
INT8U Event_3115(INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi3115 != prginfo_event->oi_changed.oi3115){
		readCoverClass(0x3115,0,&prginfo_event->event_obj.Event3115_obj,sizeof(prginfo_event->event_obj.Event3115_obj),event_para_save);
		oi_chg.oi3115 = prginfo_event->oi_changed.oi3115;
	}
    if (prginfo_event->event_obj.Event3115_obj.enableflag == 0) {
        return 0;
    }
    return 1;
}

/*
 * 有功总电能量差动越限事件记录
 */
INT8U Event_3116(INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi3116 != prginfo_event->oi_changed.oi3116){
		readCoverClass(0x3116,0,&prginfo_event->event_obj.Event3116_obj,sizeof(prginfo_event->event_obj.Event3116_obj),event_para_save);
		oi_chg.oi3116 = prginfo_event->oi_changed.oi3116;
	}
    if (prginfo_event->event_obj.Event3116_obj.event_obj.enableflag == 0) {
        return 0;
    }
    return 1;
}

/*
 * 输出回路接入状态变位事件记录
 */
INT8U Event_3117(INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi3117 != prginfo_event->oi_changed.oi3117){
		readCoverClass(0x3117,0,&prginfo_event->event_obj.Event3117_obj,sizeof(prginfo_event->event_obj.Event3117_obj),event_para_save);
		oi_chg.oi3117 = prginfo_event->oi_changed.oi3117;
	}
    if (prginfo_event->event_obj.Event3117_obj.enableflag == 0) {
        return 0;
    }

    if(1){
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		prginfo_event->event_obj.Event3117_obj.crrentnum++;
		prginfo_event->event_obj.Event3117_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event3117_obj.crrentnum,prginfo_event->event_obj.Event3117_obj.maxnum);
		INT32U crrentnum = prginfo_event->event_obj.Event3117_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(prginfo_event,0x3117,Save_buf,&index,crrentnum,NULL,s_null);
		//无属性3关联数据
		Save_buf[STANDARD_NUM_INDEX]+=0;
		//存储更改后得参数
		saveCoverClass(0x3117,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event3117_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3117,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};memset(Currbuf,0,50);
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
		saveCoverClass(0x3117,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(prginfo_event->event_obj.Event3117_obj.reportflag)
			Need_Report(0x3117,crrentnum,prginfo_event);
    }
    return 1;
}

/*
 * 终端编程记录 data为多个OAD集合，第一个字节为数量，后面是多个4个字节得OAD
 */
INT8U Event_3118(INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi3118 != prginfo_event->oi_changed.oi3118){
		readCoverClass(0x3118,0,&prginfo_event->event_obj.Event3118_obj,sizeof(prginfo_event->event_obj.Event3118_obj),event_para_save);
		oi_chg.oi3118 = prginfo_event->oi_changed.oi3118;
	}
    if (prginfo_event->event_obj.Event3118_obj.enableflag == 0) {
        return 0;
    }

    if(1){
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		prginfo_event->event_obj.Event3118_obj.crrentnum++;
		prginfo_event->event_obj.Event3118_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event3118_obj.crrentnum,prginfo_event->event_obj.Event3118_obj.maxnum);
		INT32U crrentnum = prginfo_event->event_obj.Event3118_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(prginfo_event,0x3118,Save_buf,&index,crrentnum,NULL,s_null);
		//array OAD
	    Save_buf[index++]=dtarray;//array
	    Save_buf[index++]=data[0];//数量
	    int i=0;
	    for(i=0;i<data[0];i++){
	    	Save_buf[index++]=dtoad;//OAD
	    	memcpy(&Save_buf[index],&data[1+i*4],4);
	    	index+=4;
	    }
		Save_buf[STANDARD_NUM_INDEX]+=1;
		//存储更改后得参数
		saveCoverClass(0x3118,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event3118_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3118,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};memset(Currbuf,0,50);
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
		saveCoverClass(0x3118,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(prginfo_event->event_obj.Event3118_obj.reportflag)
			Need_Report(0x3118,crrentnum,prginfo_event);
    }
    return 1;
}

/*
 * 电能表开盖事件
 */
INT8U Event_301B(INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi301B != prginfo_event->oi_changed.oi301B){
		readCoverClass(0x301B,0,&prginfo_event->event_obj.Event301B_obj,sizeof(prginfo_event->event_obj.Event301B_obj),event_para_save);
		oi_chg.oi301B = prginfo_event->oi_changed.oi301B;
	}
    if (prginfo_event->event_obj.Event301B_obj.enableflag == 0) {
        return 0;
    }

    if(1){
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		prginfo_event->event_obj.Event301B_obj.crrentnum++;
		prginfo_event->event_obj.Event301B_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event301B_obj.crrentnum,prginfo_event->event_obj.Event301B_obj.maxnum);
		INT32U crrentnum = prginfo_event->event_obj.Event301B_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(prginfo_event,0x301B,Save_buf,&index,crrentnum,NULL,s_null);

		//存储更改后得参数
		saveCoverClass(0x301B,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event301B_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x301B,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};memset(Currbuf,0,50);
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
		saveCoverClass(0x301B,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(prginfo_event->event_obj.Event301B_obj.reportflag)
			Need_Report(0x301B,crrentnum,prginfo_event);
    }
    return 1;
}

/*
 * 终端电流回路异常事件23,II型集中器没有电流，暂时不处理,type为0,1 短路、开路
 */
INT8U Event_3119(INT8U type, INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi3119 != prginfo_event->oi_changed.oi3119){
		readCoverClass(0x3119,0,&prginfo_event->event_obj.Event3119_obj,sizeof(prginfo_event->event_obj.Event3119_obj),event_para_save);
		oi_chg.oi3119 = prginfo_event->oi_changed.oi3119;
	}
    if (prginfo_event->event_obj.Event3119_obj.enableflag == 0) {
        return 0;
    }
    if(1){
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		prginfo_event->event_obj.Event3119_obj.crrentnum++;
		prginfo_event->event_obj.Event3119_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event3119_obj.crrentnum,prginfo_event->event_obj.Event3119_obj.maxnum);
		INT32U crrentnum = prginfo_event->event_obj.Event3119_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(prginfo_event,0x3119,Save_buf,&index,crrentnum,(INT8U*)&type,s_enum);
		//属性3无关联数据
		Save_buf[STANDARD_NUM_INDEX]+=0;
		//存储更改后得参数
		saveCoverClass(0x3119,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event3119_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3119,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};memset(Currbuf,0,50);
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,(INT8U*)&type,s_enum,crrentnum,0);
		saveCoverClass(0x3119,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(prginfo_event->event_obj.Event3119_obj.reportflag)
			Need_Report(0x3119,crrentnum,prginfo_event);
    }
    return 1;
}

/*
 * 功控跳闸记录 data为事件源OI+事件发生后2分钟功率long64+控制对象OI+跳闸轮次bit-string(SIZE(8))+功控定值long64+跳闸前总有加有功功率23012300
 */
INT8U Event_3200(INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi3200 != prginfo_event->oi_changed.oi3200){
		readCoverClass(0x3200,0,&prginfo_event->event_obj.Event3200_obj,sizeof(prginfo_event->event_obj.Event3200_obj),event_para_save);
		oi_chg.oi3200 = prginfo_event->oi_changed.oi3200;
	}
    if (prginfo_event->event_obj.Event3200_obj.enableflag == 0) {
        return 0;
    }

    if(1){
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		prginfo_event->event_obj.Event3200_obj.crrentnum++;
		prginfo_event->event_obj.Event3200_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event3200_obj.crrentnum,prginfo_event->event_obj.Event3200_obj.maxnum);
		INT32U crrentnum = prginfo_event->event_obj.Event3200_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(prginfo_event,0x3200,Save_buf,&index,crrentnum,(INT8U*)data,s_oi);
		//事件发生后2分钟功率long64
	    Save_buf[index++]=dtlong64;//long64
	    //data[0].data[1]为OI
        memcpy(&Save_buf[index],&data[2],8);
        index+=8;
        //控制对象OI
        Save_buf[index++]=dtoi;//OI
        memcpy(&Save_buf[index],&data[2+8],2);
        index+=2;
        //跳闸轮次bit-string(SIZE(8))
        Save_buf[index++]=dtbitstring;//BIT-STRING
        Save_buf[index++]=0x08;
        Save_buf[index++]=data[2+8+2];
        //功控定值long64
        Save_buf[index++]=dtlong64;//long64
		memcpy(&Save_buf[index],&data[2+8+2+1],8);
		index+=8;
		//跳闸前总有加有功功率23012300
		Save_buf[index++]=dtlong64;//long64
		memcpy(&Save_buf[index],&data[2+8+2+1+8],8);
		index+=8;
		Save_buf[STANDARD_NUM_INDEX]+=5;
		//存储更改后得参数
		saveCoverClass(0x3200,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event3200_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3200,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};memset(Currbuf,0,50);
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,(INT8U*)data,s_oi,crrentnum,0);
		saveCoverClass(0x3200,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(prginfo_event->event_obj.Event3200_obj.reportflag)
			Need_Report(0x3200,crrentnum,prginfo_event);
    }
    return 1;
}

/*
 * 电控跳闸记录 data为事件源OI+控制对象OI+跳闸轮次bit-string(SIZE(8))+电控定值long64+跳闸发生时总有加有功电量23014900array
 */
INT8U Event_3201(INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi3201 != prginfo_event->oi_changed.oi3201){
		readCoverClass(0x3201,0,&prginfo_event->event_obj.Event3201_obj,sizeof(prginfo_event->event_obj.Event3201_obj),event_para_save);
		oi_chg.oi3201 = prginfo_event->oi_changed.oi3201;
	}
    if (prginfo_event->event_obj.Event3201_obj.enableflag == 0) {
        return 0;
    }

    if(1){
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		prginfo_event->event_obj.Event3201_obj.crrentnum++;
		prginfo_event->event_obj.Event3201_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event3201_obj.crrentnum,prginfo_event->event_obj.Event3201_obj.maxnum);
		INT32U crrentnum = prginfo_event->event_obj.Event3201_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(prginfo_event,0x3201,Save_buf,&index,crrentnum,(INT8U*)data,s_oi);
        //控制对象OI
        Save_buf[index++]=dtoi;//OI
        memcpy(&Save_buf[index],&data[2],2);
        index+=2;
        //跳闸轮次bit-string(SIZE(8))
        Save_buf[index++]=dtbitstring;//BIT-STRING
        Save_buf[index++]=0x08;
        Save_buf[index++]=data[2+2];
        //电控定值long64
        Save_buf[index++]=dtlong64;//long64
		memcpy(&Save_buf[index],&data[2+2+1],8);
		index+=8;
		//跳闸发生时总有加有功电量23014900array
		Save_buf[index++]=dtarray;//array 考虑用array列出总加组
		Save_buf[index++]=0;//数量
		Save_buf[STANDARD_NUM_INDEX]+=4;
		//存储更改后得参数
		saveCoverClass(0x3201,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event3201_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3201,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};memset(Currbuf,0,50);
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,(INT8U*)data,s_oi,crrentnum,0);
		saveCoverClass(0x3201,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(prginfo_event->event_obj.Event3201_obj.reportflag)
			Need_Report(0x3201,crrentnum,prginfo_event);
    }
    return 1;
}

/*
 * 购电参数设置记录29
 */
INT8U Event_3202(INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi3202 != prginfo_event->oi_changed.oi3202){
		readCoverClass(0x3202,0,&prginfo_event->event_obj.Event3202_obj,sizeof(prginfo_event->event_obj.Event3202_obj),event_para_save);
		oi_chg.oi3202 = prginfo_event->oi_changed.oi3202;
	}
    //暂时不使用
	if (prginfo_event->event_obj.Event3202_obj.enableflag == 0) {
		return 0;
	}
	//oi:8107 or 810c
	if(1){
		INT8U oi[2]={0x81,0x07};
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		prginfo_event->event_obj.Event3202_obj.crrentnum++;
		prginfo_event->event_obj.Event3202_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event3202_obj.crrentnum,prginfo_event->event_obj.Event3202_obj.maxnum);
		INT32U crrentnum = prginfo_event->event_obj.Event3202_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(prginfo_event,0x3202,Save_buf,&index,crrentnum,(INT8U*)oi,s_oi);
		//属性3无关联数据
		Save_buf[STANDARD_NUM_INDEX]+=0;
		//存储更改后得参数
		saveCoverClass(0x3202,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event3202_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3202,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};memset(Currbuf,0,50);
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,(INT8U*)oi,s_oi,crrentnum,0);
		saveCoverClass(0x3202,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(prginfo_event->event_obj.Event3202_obj.reportflag)
			Need_Report(0x3202,crrentnum,prginfo_event);
	 }
    return 1;
}

/*
 * 电控告警事件记录  data为事件源OI+控制对象OI+电控定值long64
 */
INT8U Event_3203(INT8U* data,INT8U len,ProgramInfo* prginfo_event) {
	if(oi_chg.oi3203 != prginfo_event->oi_changed.oi3203){
		readCoverClass(0x3203,0,&prginfo_event->event_obj.Event3203_obj,sizeof(prginfo_event->event_obj.Event3203_obj),event_para_save);
		oi_chg.oi3203 = prginfo_event->oi_changed.oi3203;
	}
    //暂时不使用
	 if (prginfo_event->event_obj.Event3203_obj.enableflag == 0) {
	        return 0;
	}

	if(1){
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		prginfo_event->event_obj.Event3203_obj.crrentnum++;
		prginfo_event->event_obj.Event3203_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event3203_obj.crrentnum,prginfo_event->event_obj.Event3203_obj.maxnum);
		INT32U crrentnum = prginfo_event->event_obj.Event3203_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(prginfo_event,0x3203,Save_buf,&index,crrentnum,(INT8U*)data,s_oi);
		//控制对象OI
		Save_buf[index++]=dtoi;//OI
		memcpy(&Save_buf[index],&data[2],2);
		index+=2;
		//电控定值long64
		Save_buf[index++]=dtlong64;//long64
		memcpy(&Save_buf[index],&data[2+2],8);
		index+=8;
		Save_buf[STANDARD_NUM_INDEX]+=2;
		//存储更改后得参数
		saveCoverClass(0x3203,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event3203_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3203,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};memset(Currbuf,0,50);
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,(INT8U*)data,s_oi,crrentnum,0);
		saveCoverClass(0x3203,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(prginfo_event->event_obj.Event3203_obj.reportflag)
			Need_Report(0x3203,crrentnum,prginfo_event);
	}
    return 1;
}

/*
 * 电压相序异常 可以698规约解析actionrequest 调用该接口，data为OAD
 */
INT8U Event_300F(ProgramInfo* prginfo_event) {
	if(oi_chg.oi300F != prginfo_event->oi_changed.oi300F){
		readCoverClass(0x300F,0,&prginfo_event->event_obj.Event300F_obj,sizeof(prginfo_event->event_obj.Event300F_obj),event_para_save);
		oi_chg.oi300F = prginfo_event->oi_changed.oi300F;
	}
   // fprintf(stderr,"[300F] enableflag=%d \n",prginfo_event->event_obj.Event300F_obj.event_obj.enableflag);
    if (prginfo_event->event_obj.Event300F_obj.event_obj.enableflag == 0) {
        return 0;
    }
    //事件判定
    INT8U offset=prginfo_event->event_obj.Event300F_obj.offset;
    static TS starttime,nowtime;
    static INT8U first=0,happenflag=0;
    //fprintf(stderr,"[300F]Sflag=%02x \n",prginfo_event->ACSRealData.SFlag);
    if(((prginfo_event->ACSRealData.SFlag>>3)&0x01)>0){
    	if(first == 0){
    		TSGet(&starttime);
    		first = 1;
    	}
    	TSGet(&nowtime);
		int tcha=abs(difftime(tmtotime_t(nowtime),tmtotime_t(starttime)));

		if(tcha>offset && offset>0){
			if(happenflag == 1)
				return 1;
			happenflag=1;
			INT8U Save_buf[256];
			bzero(Save_buf, sizeof(Save_buf));
			prginfo_event->event_obj.Event300F_obj.event_obj.crrentnum++;
			prginfo_event->event_obj.Event300F_obj.event_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event300F_obj.event_obj.crrentnum,prginfo_event->event_obj.Event300F_obj.event_obj.maxnum);
			INT32U crrentnum = prginfo_event->event_obj.Event300F_obj.event_obj.crrentnum;
			INT8U index=0;
			//标准数据单元
			Get_StandardUnit(prginfo_event,0x300F,Save_buf,&index,crrentnum,NULL,s_null);
			//无关联数据
			Save_buf[STANDARD_NUM_INDEX]+=0;
			//存储更改后得参数
			saveCoverClass(0x300F,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event300F_obj,sizeof(Class7_Object),event_para_save);
			//存储记录集
			saveCoverClass(0x300F,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
			//存储当前记录值
			INT8U Currbuf[50]={};memset(Currbuf,0,50);
			INT8U Currindex=0;
			Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
			saveCoverClass(0x300F,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
			//判断是否要上报
			if(prginfo_event->event_obj.Event300F_obj.event_obj.reportflag)
				Need_Report(0x300F,crrentnum,prginfo_event);
		}
    }else{
    	first = 0;
    	happenflag=0;
    }
    return 1;
}

/*
 * 电流相序异常 可以698规约解析actionrequest 调用该接口，data为OAD
 */
INT8U Event_3010(ProgramInfo* prginfo_event) {
	if(oi_chg.oi3010 != prginfo_event->oi_changed.oi3010){
		readCoverClass(0x3010,0,&prginfo_event->event_obj.Event3010_obj,sizeof(prginfo_event->event_obj.Event3010_obj),event_para_save);
		oi_chg.oi3010 = prginfo_event->oi_changed.oi3010;
	}

    if (prginfo_event->event_obj.Event3010_obj.event_obj.enableflag == 0) {
        return 0;
    }
    //事件判定
    INT8U offset=prginfo_event->event_obj.Event3010_obj.offset;
    static TS starttime,nowtime;
    static INT8U first=0,happenflag=0;
    fprintf(stderr,"[3010]Sflag=%02x \n",prginfo_event->ACSRealData.SFlag);
    if(((prginfo_event->ACSRealData.SFlag>>4)&0x01)>0){
    	if(first == 0){
    		TSGet(&starttime);
    		first = 1;
    	}
    	TSGet(&nowtime);
		int tcha=abs(difftime(tmtotime_t(nowtime),tmtotime_t(starttime)));

		if(tcha>offset && offset>0){
			if(happenflag == 1)
				return 1;
			happenflag=1;
			INT8U Save_buf[256];
			bzero(Save_buf, sizeof(Save_buf));
			prginfo_event->event_obj.Event3010_obj.event_obj.crrentnum++;
			prginfo_event->event_obj.Event3010_obj.event_obj.crrentnum=Getcurrno(prginfo_event->event_obj.Event3010_obj.event_obj.crrentnum,prginfo_event->event_obj.Event3010_obj.event_obj.maxnum);
			INT32U crrentnum = prginfo_event->event_obj.Event3010_obj.event_obj.crrentnum;
			INT8U index=0;
			//标准数据单元
			Get_StandardUnit(prginfo_event,0x3010,Save_buf,&index,crrentnum,NULL,s_null);
			//无关联数据
			Save_buf[STANDARD_NUM_INDEX]+=0;
			//存储更改后得参数
			saveCoverClass(0x3010,(INT16U)crrentnum,(void *)&prginfo_event->event_obj.Event3010_obj,sizeof(Class7_Object),event_para_save);
			//存储记录集
			saveCoverClass(0x3010,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
			//存储当前记录值
			INT8U Currbuf[50]={};memset(Currbuf,0,50);
			INT8U Currindex=0;
			Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
			saveCoverClass(0x3010,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
			//判断是否要上报
			if(prginfo_event->event_obj.Event3010_obj.event_obj.reportflag)
				Need_Report(0x3010,crrentnum,prginfo_event);
		}
    }else{
    	first = 0;
    	happenflag=0;
    }
    return 1;
}

/*
 * 698guiyue规约库判断初始化事件、终端对时事件
 */
void  Get698_event(OAD oad,ProgramInfo* prginfo_event)
{
    if(oad.OI == 0x4300 && (oad.attflg == 3 || oad.attflg == 5 || oad.attflg == 6)){

    	Event_3100(NULL,0,prginfo_event);
    	prginfo_event->event_obj.Event3106_obj.event_obj.crrentnum = 0;//停上电
    	saveCoverClass(0x3106,0,(void *)&prginfo_event->event_obj.Event3106_obj,sizeof(Event3106_Object),event_para_save);
        memset(curr_data,0,MAX_POINT_NUM*4);//curr_data[MAX_POINT_NUM*4];
    }else if(oad.OI == 0x4000 && oad.attflg == 2){
    	DateTimeBCD datetime;
    	DataTimeGet(&datetime);
    	Event_3114(datetime,prginfo_event);
    }
}
