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
#include "Objectdef.h"

//测量点、事件参数
static TSA TSA_LIST[MAX_POINT_NUM];
static int TSA_NUMS;
//事件参数（读文件）
static TerminalEvent_Object event_object;
//当前最新抄表数据
static Curr_Data curr_data[MAX_POINT_NUM];
//当前内存保存得数据个数
static INT16U currnum=0;

/*
 * 说明：
 * 进程如调用该部分事件接口，需先调用Event_Init初始化事件参数结构体和测量点信息
 * 在根据事件采集方案 然后根据实际情况调用具体函数Event_AnalyseData,判断310B，310C，310D，310E，3105
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
 * 初始化参数
 */
INT8U Event_Init() {
    //初始化事件参数，调用文件
	readCoverClass(0x3100,0,&event_object.Event3100_obj,sizeof(event_object.Event3100_obj),event_para_save);
	readCoverClass(0x3101,0,&event_object.Event3101_obj,sizeof(event_object.Event3101_obj),event_para_save);
	readCoverClass(0x3104,0,&event_object.Event3104_obj,sizeof(event_object.Event3104_obj),event_para_save);
	readCoverClass(0x3105,0,&event_object.Event3105_obj,sizeof(event_object.Event3105_obj),event_para_save);
	readCoverClass(0x3106,0,&event_object.Event3106_obj,sizeof(event_object.Event3106_obj),event_para_save);
	readCoverClass(0x3107,0,&event_object.Event3107_obj,sizeof(event_object.Event3107_obj),event_para_save);
	readCoverClass(0x3108,0,&event_object.Event3108_obj,sizeof(event_object.Event3108_obj),event_para_save);
	readCoverClass(0x3109,0,&event_object.Event3109_obj,sizeof(event_object.Event3109_obj),event_para_save);
	readCoverClass(0x310A,0,&event_object.Event310A_obj,sizeof(event_object.Event310A_obj),event_para_save);
	readCoverClass(0x310B,0,&event_object.Event310B_obj,sizeof(event_object.Event310B_obj),event_para_save);
	readCoverClass(0x310C,0,&event_object.Event310C_obj,sizeof(event_object.Event310C_obj),event_para_save);
	readCoverClass(0x310D,0,&event_object.Event310D_obj,sizeof(event_object.Event310D_obj),event_para_save);
	readCoverClass(0x310E,0,&event_object.Event310E_obj,sizeof(event_object.Event310E_obj),event_para_save);
	readCoverClass(0x310F,0,&event_object.Event310F_obj,sizeof(event_object.Event310F_obj),event_para_save);
	readCoverClass(0x3110,0,&event_object.Event3110_obj,sizeof(event_object.Event3110_obj),event_para_save);
	readCoverClass(0x3111,0,&event_object.Event3111_obj,sizeof(event_object.Event3111_obj),event_para_save);
	readCoverClass(0x3112,0,&event_object.Event3112_obj,sizeof(event_object.Event3112_obj),event_para_save);
	readCoverClass(0x3114,0,&event_object.Event3114_obj,sizeof(event_object.Event3114_obj),event_para_save);
	readCoverClass(0x3115,0,&event_object.Event3115_obj,sizeof(event_object.Event3115_obj),event_para_save);
	readCoverClass(0x3116,0,&event_object.Event3116_obj,sizeof(event_object.Event3116_obj),event_para_save);
	readCoverClass(0x3117,0,&event_object.Event3117_obj,sizeof(event_object.Event3117_obj),event_para_save);
	readCoverClass(0x3118,0,&event_object.Event3118_obj,sizeof(event_object.Event3118_obj),event_para_save);
	readCoverClass(0x3119,0,&event_object.Event3119_obj,sizeof(event_object.Event3119_obj),event_para_save);
	readCoverClass(0x311A,0,&event_object.Event311A_obj,sizeof(event_object.Event311A_obj),event_para_save);
	readCoverClass(0x311B,0,&event_object.Event311B_obj,sizeof(event_object.Event311B_obj),event_para_save);
	readCoverClass(0x311C,0,&event_object.Event311C_obj,sizeof(event_object.Event311C_obj),event_para_save);
	readCoverClass(0x3200,0,&event_object.Event3200_obj,sizeof(event_object.Event3200_obj),event_para_save);
	readCoverClass(0x3201,0,&event_object.Event3201_obj,sizeof(event_object.Event3201_obj),event_para_save);
	readCoverClass(0x3202,0,&event_object.Event3202_obj,sizeof(event_object.Event3202_obj),event_para_save);
	readCoverClass(0x3203,0,&event_object.Event3203_obj,sizeof(event_object.Event3203_obj),event_para_save);
	//测量点信息
	TSA_NUMS=getFileRecordNum(0x6000);
	if(TSA_NUMS>MAX_POINT_NUM)
		TSA_NUMS=MAX_POINT_NUM;
	CLASS_6001	 meter={};
	int i=0;
	for(i=0;i<TSA_NUMS;i++) {
		if(readParaClass(0x6000,&meter,i)==1) {
			TSA_LIST[i]=meter.basicinfo.addr;
		}
	}
    return 1;
}

/*
 * 分析抄表存储的报文，输入任务id和抄读对象地址。
 * buf抄表数据 buf长度 id任务号 tsa表地址
 * 抄表抄读到正向有功、电表时钟调用该函数，buf（OAD+数据类型+数据），主要针对310B，310C，310D，310E，3105
 * 其他事件按事件情况调用，可能buf参数不同
 */
INT8U Event_AnalyseData(INT8U* buf, INT8U len,INT32U id, TSA tsa) {

    //存储事件产生所需要的判据
    INT8U data[64];
    bzero(data, sizeof(data));
    //找正向有功数据
    if (Event_FindOAD(buf, 0x0010, data)) {
        Event_310B(tsa, data,4); //电能表示度下降事件10
        Event_310C(tsa, data,4); //电能量超差事件11
        Event_310D(tsa, data,4); //电能表飞走事件12
        Event_310E(tsa, data,4); //电能表停走事件13
    }

    //找电表校时数据
    if (Event_FindOAD(buf, 0x4000, data)) {
        Event_3105(tsa, data,7); //电能表时钟超差事件4
    }
    return 1;
}

/*
 * 输入报文和想要找到的OAD，将结果数据放入data中，返回1为成功，返回0为失败。
 */
INT8U Event_FindOAD(INT8U* buf, OI_698 oad, INT8U* data) {
    //OAD 00 10 02 00
	//DATA 01
	//double-long-unsigned 06
	//数据:00 00 00 00
	INT16U curroad=((buf[0]<<8)+buf[1]);
	INT8U len=0;
	if(curroad == oad){
		switch(buf[5]){
		case 0x06:
			len=4;
			break;
		case 28:
			len=7;
			break;
		}
		memcpy(data,&buf[6],len);
	}else
		return 0;
    return 1;
}

/*
 * 更新当前最新得正向有功(只针对正向有功)
 */
INT8U Refresh_Data(TSA tsa,INT32U newdata,INT8U flag){
	int i=0;
	INT8U haveflag=0;
	for(i=0;i<currnum;i++){
       if(memcmp(&tsa,&curr_data[i].tsa,sizeof(TSA)) == 0){
           curr_data[i].data=newdata;
           //判断停走，需要更新时间
           if(flag == 1){
        	   TS currtime;
        	   TSGet(&currtime);
        	   memcpy(&curr_data[i].ts,&currtime,sizeof(TS));
           }
           haveflag=1;
    	   break;
       }
	}
	if(haveflag == 0){
		memcpy(&curr_data[currnum].tsa,&tsa,sizeof(TSA));
		curr_data[currnum].data=newdata;
		currnum++;
	}
	return 1;
}

/*
 * 获取上次保存得正向有功
 */
INT8U Get_Mdata(TSA tsa,INT32U *olddata,TS *ts){
	int i=0;
	INT8U haveflag=0;
	for(i=0;i<currnum;i++){
		if(memcmp(&tsa,&curr_data[i].tsa,sizeof(TSA)) == 0){
			*olddata=curr_data[i].data;
			memcpy(ts,&curr_data[i].ts,sizeof(TS));
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
 */
INT8U Get_Event(OI_698 oi,INT8U eventno,INT8U** Getbuf,INT8U *Getlen)
{
	int filesize=0;
	filesize = getClassFileLen(oi,eventno,event_record_save);
	if(filesize<=0)  return 0;
	*Getlen=filesize;
	*Getbuf=(INT8U*)malloc(filesize);
	readCoverClass(oi,eventno,*Getbuf,*Getlen,event_record_save);
	return 1;
}
/*
 * 事件需要上报
 */
INT8U Need_Report(OI_698 oi,INT8U eventno){
	INT8U Sbuf[200];
	INT8U index=0;
    //报文链路层报文头部
    //此处根据698协议
    //从文件读取记录
	INT8U *Getbuf=NULL;//因为记录为变长，只能采用二级指针，动态分配
	INT8U Getlen=0;//记录长度
	Get_Event(oi,eventno,(INT8U**)&Getbuf,&Getlen);
	if(Getbuf!=NULL){
		memcpy(&Sbuf[index],Getbuf,Getlen);
		index+=Getlen;
		free(Getbuf);
	}
	//报文链路层报文尾部
	//调用发送函数
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
    Rbuf[(*Index)++]=0x02;//structure
    Rbuf[(*Index)++]=0x02;//数量
    //事件发生源
    INT8U datatype=0,sourcelen=0;
    Get_Source(Source,S_type,&datatype,&sourcelen);
    Rbuf[(*Index)++] = datatype;
    if(sourcelen>0)
    	memcpy(&Rbuf[(*Index)],Source,sourcelen);
    (*Index)+=sourcelen;
	Rbuf[(*Index)++] = 0x02;//structure
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
INT8U Get_StandardUnit(OI_698 oi,INT8U *Rbuf,INT8U *Index,
		INT8U Eventno,INT8U *Source,Source_Typ S_type){
	//Struct
	Rbuf[(*Index)++] = 0x02;
	//单元数量
	Rbuf[(*Index)++] = STANDARD_NUM;
	//事件记录序号
	Rbuf[(*Index)++] = 0x06;
	//memcpy(&Rbuf[*Index], &Eventno, sizeof(INT32U));
	INT32U En=(INT32U)Eventno;
	Rbuf[(*Index)++] = (Eventno>>32)&0x000000ff;
	Rbuf[(*Index)++] = (Eventno>>16)&0x000000ff;
	Rbuf[(*Index)++] = (Eventno>>8)&0x000000ff;
	Rbuf[(*Index)++] = Eventno&0x000000ff;
	//(*Index)+=sizeof(INT32U);
	DateTimeBCD ntime;
	DataTimeGet(&ntime);

	//事件发生时间
	Rbuf[(*Index)++] = 0x1C;
	//memcpy(&Rbuf[*Index], &ntime, sizeof(ntime));
	//(*Index)+=sizeof(ntime);
	Rbuf[(*Index)++] = (ntime.year>>8)&0x00ff;
	Rbuf[(*Index)++] = (ntime.year)&0x00ff;
	Rbuf[(*Index)++] = ntime.month;
	Rbuf[(*Index)++] = ntime.day;
	Rbuf[(*Index)++] = ntime.hour;
	Rbuf[(*Index)++] = ntime.min;
	Rbuf[(*Index)++] = ntime.sec;
	//事件结束时间
	Rbuf[(*Index)++] = 0x1C;
	if(oi==0x311C){
		memset(&Rbuf[*Index],DATA_FF,sizeof(ntime));//TODO
		(*Index)+=sizeof(ntime);
	}
	else{
		Rbuf[(*Index)++] = (ntime.year>>8)&0x00ff;
		Rbuf[(*Index)++] = (ntime.year)&0x00ff;
		Rbuf[(*Index)++] = ntime.month;
		Rbuf[(*Index)++] = ntime.day;
		Rbuf[(*Index)++] = ntime.hour;
		Rbuf[(*Index)++] = ntime.min;
		Rbuf[(*Index)++] = ntime.sec;
	}
	//事件发生源
	INT8U datatype=0,sourcelen=0;
	Get_Source(Source,S_type,&datatype,&sourcelen);
	Rbuf[(*Index)++] = datatype;
	if(sourcelen>0)
		memcpy(&Rbuf[(*Index)],Source,sourcelen);
	(*Index)+=sourcelen;
	//事件上报状态
	Rbuf[(*Index)++] = 0x01;//array
	Rbuf[(*Index)++] = 0x02;//数量
	Rbuf[(*Index)++] = 0x02;//struct
	Rbuf[(*Index)++] = 0x02;//数量
	Rbuf[(*Index)++] = 0x51;//OAD
	Rbuf[(*Index)++] = 0x45;//gprs
	Rbuf[(*Index)++] = 0x00;
	Rbuf[(*Index)++] = 0x00;
	Rbuf[(*Index)++] = 0x00;
	Rbuf[(*Index)++] = 0x16;//unsigned
	Rbuf[(*Index)++] = 0x00;
	Rbuf[(*Index)++] = 0x45;//以太网
	Rbuf[(*Index)++] = 0x10;
	Rbuf[(*Index)++] = 0x00;
	Rbuf[(*Index)++] = 0x00;
	Rbuf[(*Index)++] = 0x16;//unsigned
	Rbuf[(*Index)++] = 0x00;
    return 1;
}

/*
 * 每个事件记录数不超过设定得最大记录数，默认15个
 */
INT8U Getcurrno(INT16U *currno,INT16U maxno){
	if(*currno>maxno)
		*currno=1;
	return 1;
}
/*
 * 终端初始化事件1 可以698规约解析actionrequest 调用该接口，data为OAD
 */
INT8U Event_3100(INT8U* data,INT8U len) {
    if (event_object.Event3100_obj.enableflag == 0) {
        return 0;
    }
    //43 00 03 00
    //事件判定
    INT16U oi=(data[0]<<8)+data[1];
    INT8U action=data[2]&0b00011111;
    if(oi==0x4300 && action==3){
    	INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		event_object.Event3100_obj.crrentnum++;
		Getcurrno(&event_object.Event3100_obj.crrentnum,event_object.Event3100_obj.maxnum);
		INT32U crrentnum = event_object.Event3100_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(0x3100,Save_buf,&index,crrentnum,NULL,s_null);
		//无关联数据
		Save_buf[STANDARD_NUM_INDEX]+=0;
		//存储更改后得参数
		saveCoverClass(0x3100,(INT16U)crrentnum,(void *)&event_object.Event3100_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3100,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
		saveCoverClass(0x3100,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(event_object.Event3100_obj.reportflag)
			Need_Report(0x3100,crrentnum);
    }
    return 1;
}

/*
 * 终端版本变更事件 可直接调用，维护命令修改版本或者升级后可调用
 */
INT8U Event_3101(INT8U* data,INT8U len) {
    if (event_object.Event3101_obj.enableflag == 0) {
        return 0;
    }
    //事件判定
    if(1){
    	INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		event_object.Event3101_obj.crrentnum++;
		Getcurrno(&event_object.Event3101_obj.crrentnum,event_object.Event3101_obj.maxnum);
		INT32U crrentnum = event_object.Event3101_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(0x3101,Save_buf,&index,crrentnum,NULL,s_null);
		//取共享内存或文件
		//事件发生前软件版本号
		Save_buf[index++]=10;//visible-string 4
		Save_buf[index++]=4;// 4
		Save_buf[index++]=0;
		Save_buf[index++]=0;
		Save_buf[index++]=0;
		Save_buf[index++]=0;
		//事件发生前软件版本号
		Save_buf[index++]=10;//visible-string 4
		Save_buf[index++]=4;// 4
		Save_buf[index++]=0;
		Save_buf[index++]=0;
		Save_buf[index++]=0;
		Save_buf[index++]=0;
		Save_buf[STANDARD_NUM_INDEX]+=2;
		//存储更改后得参数
		saveCoverClass(0x3101,(INT16U)crrentnum,(void *)&event_object.Event3101_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3101,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
		saveCoverClass(0x3101,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(event_object.Event3101_obj.reportflag)
			Need_Report(0x3101,crrentnum);
    }
    return 1;
}

/*
 * 状态量变位事件 可直接调用 data为前后得ST CD，（1-4路）8个字节即可
 */
INT8U Event_3104(INT8U* data,INT8U len) {
    if (event_object.Event3104_obj.enableflag == 0) {
        return 0;
    }
    //事件判定
    if(1){
    	INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		event_object.Event3104_obj.crrentnum++;
		Getcurrno(&event_object.Event3104_obj.crrentnum,event_object.Event3104_obj.maxnum);
		INT32U crrentnum = event_object.Event3104_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(0x3104,Save_buf,&index,crrentnum,NULL,s_null);
		//事件发生时间
		DateTimeBCD ntime;
		DataTimeGet(&ntime);
		Save_buf[index++] = 28;
		memcpy(&Save_buf[index], &ntime, sizeof(ntime));
		index+=sizeof(ntime);
		//第1路事件发生后
		Save_buf[index++]=2;//structure
		Save_buf[index++]=2;// 2
		Save_buf[index++]=17;
		Save_buf[index++]=data[0];
		Save_buf[index++]=17;
		Save_buf[index++]=data[1];
		//第2路事件发生后
		Save_buf[index++]=2;//structure
		Save_buf[index++]=2;// 2
		Save_buf[index++]=17;
		Save_buf[index++]=data[2];
		Save_buf[index++]=17;
		Save_buf[index++]=data[3];
		//第3路事件发生后
		Save_buf[index++]=2;//structure
		Save_buf[index++]=2;// 2
		Save_buf[index++]=17;
		Save_buf[index++]=data[4];
		Save_buf[index++]=17;
		Save_buf[index++]=data[5];
		//第4路事件发生后
		Save_buf[index++]=2;//structure
		Save_buf[index++]=2;// 2
		Save_buf[index++]=17;
		Save_buf[index++]=data[6];
		Save_buf[index++]=17;
		Save_buf[index++]=data[7];
		Save_buf[STANDARD_NUM_INDEX]+=5;
		//存储更改后得参数
		saveCoverClass(0x3104,(INT16U)crrentnum,(void *)&event_object.Event3104_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3104,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
		saveCoverClass(0x3104,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(event_object.Event3104_obj.reportflag)
			Need_Report(0x3104,crrentnum);
    }
    return 1;
}

/*
 * 电能表时钟超差事件 tsa事件发生源 电表时钟
 */
INT8U Event_3105(TSA tsa, INT8U* data,INT8U len) {
    if (event_object.Event3105_obj.event_obj.enableflag == 0) {
        return 0;
    }
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
    int tcha=abs(difftime(tmtotime_t(metertime),tmtotime_t(jzqtime)));
    //事件判定
    if (tcha>event_object.Event3105_obj.mto_obj.over_threshold
    		&& event_object.Event3105_obj.mto_obj.over_threshold>0) {
    	INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		//更新当前记录数
		event_object.Event3105_obj.event_obj.crrentnum++;
		Getcurrno(&event_object.Event3105_obj.event_obj.crrentnum,event_object.Event3105_obj.event_obj.maxnum);
		INT32U crrentnum = event_object.Event3105_obj.event_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(0x3105,Save_buf,&index,crrentnum,(INT8U*)&tsa,s_tsa);
		//无属性3关联数据
		Save_buf[STANDARD_NUM_INDEX]+=0;
		//存储更改后得参数
		saveCoverClass(0x3105,(INT16U)crrentnum,(void *)&event_object.Event3105_obj,sizeof(Event3105_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3105,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
        //存储当前记录值
		INT8U Currbuf[50]={};
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,(INT8U*)&tsa,s_tsa,crrentnum,0);
		saveCoverClass(0x3105,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(event_object.Event3105_obj.event_obj.reportflag)
			Need_Report(0x3105,crrentnum);
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
void SendERC3106(INT8U flag,INT8U Erctype)
{
	INT8U Save_buf[256];
	bzero(Save_buf, sizeof(Save_buf));
	event_object.Event3106_obj.event_obj.crrentnum++;
	Getcurrno(&event_object.Event3106_obj.event_obj.crrentnum,event_object.Event3106_obj.event_obj.maxnum);
	INT32U crrentnum = event_object.Event3106_obj.event_obj.crrentnum;
	INT8U index=0;
	//标准数据单元
	Get_StandardUnit(0x3106,Save_buf,&index,crrentnum,(INT8U*)&Erctype,s_enum);
	//属性标志
	Save_buf[index++]=4;//bit-string
	Save_buf[index++]=flag;
	Save_buf[STANDARD_NUM_INDEX]+=1;
	//存储更改后得参数
	saveCoverClass(0x3106,(INT16U)crrentnum,(void *)&event_object.Event3106_obj,sizeof(Event3106_Object),event_para_save);
	//存储记录集
	saveCoverClass(0x3106,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
	//存储当前记录值
	INT8U Currbuf[50]={};
	INT8U Currindex=0;
	Get_CurrResult(Currbuf,&Currindex,(INT8U*)&Erctype,s_enum,crrentnum,0);
	saveCoverClass(0x3106,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
	//判断是否要上报
	if(event_object.Event3106_obj.event_obj.reportflag)
		Need_Report(0x3106,crrentnum);
}

/*
 * 告知485发送获取测量点的停上电时间消息，考虑共享内存
 */
BOOLEAN SendMeterReadindMsg()
{
	//抄表可根据参数进行抄表
    //设置一个共享内存参数，告知抄表要进行抄表，抄表可自行判断要抄表得电表
	return TRUE;
}

//判断终端与电能表的时间偏差
BOOLEAN MeterDiff()
{
	int i = 0;
//	fprintf(stderr,"MeterDiff In,RecPointNum=%d,MsgNum=%d!!\r\n");
	for(i = 0;i<4;i++)
	{
		if(MeterPowerInfo[i].Valid)
		{
			//RecPointNum++;
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
			//fprintf(stderr,"Diff1_tmp1 =%d,Diff1_tmp2 = %d,poweroff_on_offset=%d\r\n",Diff1_tmp1,Diff1_tmp2,shmm_getpara()->f98.poweroff_on_offset);
			//判断停电事件　起止时间偏差限值的合法性
			int poweroffset = 0;
			poweroffset = event_object.Event3106_obj.poweroff_para_obj.screen_para_obj.startstoptime_offset;
#ifdef SHANDONG
			poweroffset = 100;
#endif
			if((abs(difftime(mktime(&MeterPowerInfo[i].PoweroffTime),mktime(&TermialPowerInfo.PoweroffTime)))>(poweroffset*60))
			||(abs(difftime(mktime(&MeterPowerInfo[i].PoweronTime),mktime(&TermialPowerInfo.PoweronTime)))>(poweroffset*60)))
			{

				TermialPowerInfo.Valid = POWER_OFF_INVALIDE;
				fprintf(stderr,"MeterDiff err1\r\n");
				filewrite(ERC3106PATH,&TermialPowerInfo,sizeof(TermialPowerInfo));
				return FALSE;
			}

			//判断停电事件时间区段偏差限值
			int tmp1 =abs(difftime(mktime(&MeterPowerInfo[i].PoweronTime),mktime(&MeterPowerInfo[i].PoweroffTime)));
			int tmp2 =abs(difftime(mktime(&TermialPowerInfo.PoweronTime),mktime(&TermialPowerInfo.PoweroffTime)));

			if(abs(tmp1-tmp2) > event_object.Event3106_obj.poweroff_para_obj.screen_para_obj.sectortime_offset*60)
			{
				fprintf(stderr,"MeterDiff err2\r\n");
				TermialPowerInfo.Valid = POWER_OFF_INVALIDE;
				filewrite(ERC3106PATH,&TermialPowerInfo,sizeof(TermialPowerInfo));
				return FALSE;
			}
		}
	}
	//if(MsgNum <= (RecPointNum*2))
	{
		fprintf(stderr,"POWER_OFF_VALIDE\r\n");
		TermialPowerInfo.Valid = POWER_OFF_VALIDE;
	    filewrite(ERC3106PATH,&TermialPowerInfo,sizeof(TermialPowerInfo));
	}
	//fprintf(stderr,"MsgNum=%d, RecPointNum=%d\n",MsgNum,RecPointNum);
	return TRUE;
}

INT8U fileread(char *FileName, void *source, INT32U size)
{
	FILE *fp = NULL;
	int num, ret = 0;

	//fprintf(stderr,"read FileName=%s\n",FileName);
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
		fprintf(stderr, "%s read error\n\r", FileName);
	}
	return ret;
}
/*
 * 终端停/上电事件5-停电事件-放在交采模块
 */
INT8U Event_3106(EVENTREALDATA Realdata) {
	if (event_object.Event3106_obj.event_obj.enableflag == 0) {
		return 0;
	}
	MeterDiff();
	BOOLEAN gpio_5V=pwr_has();
	time_t time_of_now;
	time_of_now = time(NULL);
	INT8U flag = 0;
	static INT8U off_time = 0;
	fileread(ERC3106PATH,&TermialPowerInfo,sizeof(TermialPowerInfo));
	INT16U poweroff_happen_vlim=event_object.Event3106_obj.poweroff_para_obj.screen_para_obj.happen_voltage_limit;
	INT16U recover_voltage_limit=event_object.Event3106_obj.poweroff_para_obj.screen_para_obj.recover_voltage_limit;
	INT16U mintime_space=event_object.Event3106_obj.poweroff_para_obj.screen_para_obj.mintime_space;
	INT16U maxtime_space=event_object.Event3106_obj.poweroff_para_obj.screen_para_obj.maxtime_space;
	//判断下电
	if(TermialPowerInfo.ERC3106State == POWER_START){
		if(Realdata.Voltage[0].value == 0)
				off_time ++;
			else
				off_time = 0;
		//二型集中器没有电池只有电容，所以不能够读出底板是否有电，且二型集中器只有一相电压，停上电事件在硬件复位时不能产生，
		//所以判断时，需要判断当前电压大于一个定值且小时参数时，产生事件(大于的定时暂定为10v交采已经将实时电压值乘以１０).
		if((Realdata.Voltage[0].Available==TRUE)
				&&((Realdata.Voltage[0].value>100&&Realdata.Voltage[0].value<poweroff_happen_vlim)
				||(Realdata.Voltage[0].value == 0 && off_time>5)))
	    //一型集中器
//		if((((Realdata.U[0].value<poweroff_happen_vlim)&&(Realdata.U[1].value<poweroff_happen_vlim)
//						&&(Realdata.U[2].value<poweroff_happen_vlim))&&((Realdata.U[0].value|Realdata.U[1].value|Realdata.U[2].value)>0)&&gpio_5V)
//						||((Realdata.U[0].Available == TRUE&&Realdata.U[0].value==0&&Realdata.U[0].value<poweroff_happen_vlim)
//								&&(Realdata.U[1].Available == TRUE&&Realdata.U[1].value==0&&Realdata.U[1].value<poweroff_happen_vlim)
//								&&(Realdata.U[2].Available == TRUE&&Realdata.U[2].value==0&&Realdata.U[2].value<poweroff_happen_vlim)&&(!gpio_5V)))

		{
			off_time = 0;
			//电压低于限值，且底板有电，产生下电事件
			TermialPowerInfo.ERC3106State = POWER_OFF;
			localtime_r((const time_t*)&time_of_now, &TermialPowerInfo.PoweroffTime);
			ERC3106log(0,Realdata.Voltage[0].value,TermialPowerInfo.PoweroffTime);//调试加入log

			filewrite(ERC3106PATH,&TermialPowerInfo,sizeof(TermialPowerInfo));
			SendERC3106(flag,0);
		}
	}else if(TermialPowerInfo.ERC3106State == POWER_OFF){
		//II型
		if((Realdata.Voltage[0].Available&&Realdata.Voltage[0].value>recover_voltage_limit))
        //I型
		//if((Realdata.U[0].Available&&Realdata.U[0].value>recover_voltage_limit)
        //			||(Realdata.U[1].Available&&Realdata.U[1].value>recover_voltage_limit)
        //				||(Realdata.U[2].Available&&Realdata.U[2].value>recover_voltage_limit))
		{
			fprintf(stderr,"停电后上电\r\n");
			TermialPowerInfo.ERC3106State = POWER_ON;
			localtime_r((const time_t*)&time_of_now, &TermialPowerInfo.PoweronTime);

			filewrite(ERC3106PATH,&TermialPowerInfo,sizeof(TermialPowerInfo));
			int interval = difftime(mktime(&TermialPowerInfo.PoweronTime),mktime(&TermialPowerInfo.PoweroffTime));
			ERC3106log(1,Realdata.Voltage[0].value,TermialPowerInfo.PoweronTime);//调试加入log

			//如果上电时间大于停电时间或者停上电时间间隔小于最小间隔或者大于最大间隔不产生下电事件
			if((interval > mintime_space*60)&&(interval < maxtime_space*60)) {
				fprintf(stderr,"上电时间满足f98参数：interval=%d\n",interval);
				flag = 0x01;
			}

			//如果初步判断事件有效，向485发送抄表消息,待判断事件有效性之后，再发送消息
			if(SendMeterReadindMsg())
			{
				fprintf(stderr,"SendMeterReadindMsg ok\n");
				TermialPowerInfo.Valid = POWER_START;
				filewrite(ERC3106PATH,&TermialPowerInfo,sizeof(TermialPowerInfo));
				return 0;
			}
			TermialPowerInfo.ERC3106State = POWER_START;
			filewrite(ERC3106PATH,&TermialPowerInfo,sizeof(TermialPowerInfo));
			SendERC3106(flag,1);
		}
	}else{
		int interval = difftime(mktime(&TermialPowerInfo.PoweronTime),mktime(&TermialPowerInfo.PoweroffTime));
		//fprintf(stderr,"TermialPowerInfo.Valid =%d\r\n",TermialPowerInfo.Valid);
		if(TermialPowerInfo.Valid == POWER_OFF_VALIDE)
		{
			fprintf(stderr,"\nTermialPowerInfo.Valid=%d",TermialPowerInfo.Valid);
			//如果上电时间大于停电时间或者停上电时间间隔小于最小间隔或者大于最大间隔不产生下电事件
			if((interval > mintime_space*60)&&(interval < maxtime_space*60))
				flag = 0x3;
			else
				flag = 0x2;
			//如果判断停电事件无效
			SendERC3106(flag,1);
			TermialPowerInfo.ERC3106State = POWER_START;
			TermialPowerInfo.Valid = POWER_START;
			filewrite(ERC3106PATH,&TermialPowerInfo,sizeof(TermialPowerInfo));
		}
		else if(TermialPowerInfo.Valid == POWER_OFF_INVALIDE)
		{
			fprintf(stderr,"\nTermialPowerInfo.Valid=%d",TermialPowerInfo.Valid);
			//如果上电时间大于停电时间或者停上电时间间隔小于最小间隔或者大于最大间隔不产生下电事件
			if((interval > mintime_space*60)&&(interval < maxtime_space*60))
				flag = 0x1;
			else
				flag = 0;

			TermialPowerInfo.ERC3106State = POWER_START;
			TermialPowerInfo.Valid = POWER_START;
			//如果判断停电事件无效
			SendERC3106(flag,1);
			filewrite(ERC3106PATH,&TermialPowerInfo,sizeof(TermialPowerInfo));
		}
		else
		{
			int interval_limit = event_object.Event3106_obj.poweroff_para_obj.collect_para_obj.time_threshold;
			if(interval_limit==0)
				interval_limit = 5;
			//如果抄表超时还未抄回,直接上报无效上电事件
			if(difftime(time_of_now,mktime(&TermialPowerInfo.PoweronTime))>(interval_limit*60))
			{
				//fprintf(stderr,"抄电表停上电时间超时未回,上报无效停电事件,interval=%d,shmm_getpara()->f97.poweroff_limit=%d!!!\r\n",interval,shmm_getpara()->f97.poweroff_limit);
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
INT8U Event_3107(INT8U* data,INT8U len) {
    if (event_object.Event3107_obj.event_obj.enableflag == 0) {
        return 0;
    }
    INT32S moniliang=(data[0]<<24)+(data[1]<<16)+(data[2]<<8)+data[3];
    INT32S offset=event_object.Event3107_obj.analogtop_obj.top_limit;
    if(moniliang>offset){
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		//更新当前记录数
		event_object.Event3107_obj.event_obj.crrentnum++;
		Getcurrno(&event_object.Event3107_obj.event_obj.crrentnum,event_object.Event3107_obj.event_obj.maxnum);
		INT32U crrentnum = event_object.Event3107_obj.event_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		INT8U oad[4]={0xF2,0x04,0x02,0x00};
		Get_StandardUnit(0x3107,Save_buf,&index,crrentnum,(INT8U*)oad,s_oad);
		//属性3无关联数据
		Save_buf[STANDARD_NUM_INDEX]+=0;
		//存储更改后得参数
		saveCoverClass(0x3107,(INT16U)crrentnum,(void *)&event_object.Event3107_obj,sizeof(Event3107_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3107,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,(INT8U*)oad,s_oad,crrentnum,0);
		saveCoverClass(0x3107,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(event_object.Event3107_obj.event_obj.reportflag)
			Need_Report(0x3107,crrentnum);
    }
    return 1;
}

/*
 * 终端直流模拟量越下限事件7 data为直流模拟量 字节高到低
 */
INT8U Event_3108(INT8U* data,INT8U len) {
    if (event_object.Event3108_obj.event_obj.enableflag == 0) {
        return 0;
    }

    INT32S moniliang=(data[0]<<24)+(data[1]<<16)+(data[2]<<8)+data[3];
	INT32S offset=event_object.Event3108_obj.analogbom_obj.bom_limit;
	if(moniliang<offset){
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		//更新当前记录数
		event_object.Event3108_obj.event_obj.crrentnum++;
		Getcurrno(&event_object.Event3108_obj.event_obj.crrentnum,event_object.Event3108_obj.event_obj.maxnum);
		INT32U crrentnum = event_object.Event3108_obj.event_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		INT8U oad[4]={0xF2,0x04,0x02,0x00};
		Get_StandardUnit(0x3108,Save_buf,&index,crrentnum,(INT8U*)oad,s_oad);
		//属性3无关联数据
		Save_buf[STANDARD_NUM_INDEX]+=0;
		//存储更改后得参数
		saveCoverClass(0x3108,(INT16U)crrentnum,(void *)&event_object.Event3108_obj,sizeof(Event3108_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3108,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,(INT8U*)oad,s_oad,crrentnum,0);
		saveCoverClass(0x3108,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(event_object.Event3108_obj.event_obj.reportflag)
			Need_Report(0x3108,crrentnum);
	}
    return 1;
}

/*
 * 终端消息认证错误事件8 data事件发生前安全认证密码(不含数据类型) len为长度
 */
INT8U Event_3109(INT8U* data,INT8U len) {
    if (event_object.Event3109_obj.enableflag == 0) {
        return 0;
    }
    if(1){
    	INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		//更新当前记录数
		event_object.Event3109_obj.crrentnum++;
		Getcurrno(&event_object.Event3109_obj.crrentnum,event_object.Event3109_obj.maxnum);
		INT32U crrentnum = event_object.Event3109_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(0x3109,Save_buf,&index,crrentnum,NULL,s_null);
		//事件发生前安全认证密码
		Save_buf[index++]=10;//visable-string
		memcpy(&Save_buf[index],data,len);
		index+=len;
		Save_buf[STANDARD_NUM_INDEX]+=1;
		//存储更改后得参数
		saveCoverClass(0x3109,(INT16U)crrentnum,(void *)&event_object.Event3109_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3109,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
		saveCoverClass(0x3109,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(event_object.Event3109_obj.reportflag)
			Need_Report(0x3109,crrentnum);
    }
    return 1;
}

/*
 * 设备故障事件 errtype：0,1,2,3,4,5
 */
INT8U Event_310A(MachineError_type errtype) {
    if (event_object.Event310A_obj.enableflag == 0) {
        return 0;
    }
    INT8U Source=0;
    switch(errtype){
       case memory_err://终端主板内存故障
            Source=0;
            break;
       case clock_err://时钟故障
		    Source=1;
		    break;
       case comm_err://主板通信故障
		    Source=2;
		    break;
       case c485_err://485抄表故障
		    Source=3;
		    break;
	   case show_err://显示板故障
		    Source=4;
		    break;
	   case plc_err://载波通道异常
		    Source=5;
		    break;
    }
    INT8U Save_buf[256];
    bzero(Save_buf, sizeof(Save_buf));
    //更新当前记录数
    event_object.Event310A_obj.crrentnum++;
    Getcurrno(&event_object.Event310A_obj.crrentnum,event_object.Event310A_obj.maxnum);
    INT32U crrentnum = event_object.Event310A_obj.crrentnum;
    INT8U index=0;
	//标准数据单元
	Get_StandardUnit(0x310A,Save_buf,&index,crrentnum,(INT8U*)&Source,s_enum);
	//无属性3关联数据
	Save_buf[STANDARD_NUM_INDEX]+=0;
	//存储更改后得参数
	saveCoverClass(0x310A,(INT16U)crrentnum,(void *)&event_object.Event310A_obj,sizeof(Class7_Object),event_para_save);
	//存储记录集
	saveCoverClass(0x310A,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
	//存储当前记录值
	INT8U Currbuf[50]={};
	INT8U Currindex=0;
	Get_CurrResult(Currbuf,&Currindex,(INT8U*)&Source,s_enum,crrentnum,0);
	saveCoverClass(0x310A,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
	//判断是否要上报
	if(event_object.Event310A_obj.reportflag)
		Need_Report(0x310A,crrentnum);
    return 1;
}

/*
 * 电能表示度下降事件10 前台两次电能值对比是否超过设定值
 */
INT8U Event_310B(TSA tsa, INT8U* data,INT8U len) {
    if (event_object.Event310B_obj.event_obj.enableflag == 0) {
        return 0;
    }
    if(data==NULL)
    	return 0;
    INT32U newdata=(data[0]<<24)+(data[1]<<16)+(data[2]<<8)+data[3];
    INT32U olddata=0;
    TS ts;
    if(Get_Mdata(tsa,&olddata,&ts) == 1){
    	if(olddata>newdata){
    		INT8U Save_buf[256];
			bzero(Save_buf, sizeof(Save_buf));
			event_object.Event310B_obj.event_obj.crrentnum++;
			Getcurrno(&event_object.Event310B_obj.event_obj.crrentnum,event_object.Event310B_obj.event_obj.maxnum);
			INT32U crrentnum = event_object.Event310B_obj.event_obj.crrentnum;
			INT8U index=0;
			//标准数据单元
			Get_StandardUnit(0x310B,Save_buf,&index,crrentnum,(INT8U*)&tsa,s_tsa);
			//属性3有关联数据
			Save_buf[index++]=6;//double-long-unsigned
			Save_buf[index++]=(olddata>>24)&0x000000ff;
			Save_buf[index++]=(olddata>>16)&0x000000ff;
			Save_buf[index++]=(olddata>>8)&0x000000ff;
			Save_buf[index++]=olddata&0x000000ff;
			Save_buf[index++]=6;//double-long-unsigned
			Save_buf[index++]=(newdata>>24)&0x000000ff;
			Save_buf[index++]=(newdata>>16)&0x000000ff;
			Save_buf[index++]=(newdata>>8)&0x000000ff;
			Save_buf[index++]=newdata&0x000000ff;
			Save_buf[STANDARD_NUM_INDEX]+=2;

			//存储更改后得参数
			saveCoverClass(0x310B,(INT16U)crrentnum,(void *)&event_object.Event310B_obj,sizeof(Event310B_Object),event_para_save);
			//存储记录集
			saveCoverClass(0x310B,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
			//存储当前记录值
			INT8U Currbuf[50]={};
			INT8U Currindex=0;
			Get_CurrResult(Currbuf,&Currindex,(INT8U*)&tsa,s_tsa,crrentnum,0);
			saveCoverClass(0x310B,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
			//判断是否要上报
			if(event_object.Event310B_obj.event_obj.reportflag)
				Need_Report(0x310B,crrentnum);
    	}
    }
    //更新数据
    Refresh_Data(tsa,newdata,0);
    return 1;
}

/*
 * 电能量超差事件11 前台两次电能值以及测量点额定电压、电流
 */
INT8U Event_310C(TSA tsa, INT8U* data,INT8U len) {
    if (event_object.Event310C_obj.event_obj.enableflag == 0) {
        return 0;
    }
    if(data==NULL)
    	return 0;
    INT32U newdata=(data[0]<<24)+(data[1]<<16)+(data[2]<<8)+data[3];
	INT32U olddata=0;
	TS ts;
	if(Get_Mdata(tsa,&olddata,&ts) == 0)
		return 0;
	/*===============TODO根据共享内存或者直接读取文件 获取该表参数*/
    CLASS_6001 meter={};
    INT32U power_offset=event_object.Event310C_obj.poweroffset_obj.power_offset;//超差值
    INT16U ratedU=meter.basicinfo.ratedU; //额定电压
    INT16U ratedI=meter.basicinfo.ratedI; //额定电流
    INT8U connectype=meter.basicinfo.connectype;//接线方式
    /*===============TODO可能需要根据下发得任务获取抄表间隔*/
    INT16U Intertime=15; //抄表间隔
    if(meter.basicinfo.port.OI == 0xF209)
    	Intertime *=60;//载波表
    else
    	Intertime =15;//485
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
    if ((newdata>olddata) && (olddata>0))
	 {
		 //kwh转换得扩大1000倍
		 if (((newdata-olddata)*1000>power_offset*Em) && (power_offset>0)){
			INT8U Save_buf[256];
			bzero(Save_buf, sizeof(Save_buf));
			event_object.Event310C_obj.event_obj.crrentnum++;
			Getcurrno(&event_object.Event310C_obj.event_obj.crrentnum,event_object.Event310C_obj.event_obj.maxnum);
			INT32U crrentnum = event_object.Event310C_obj.event_obj.crrentnum;
			INT8U index=0;
			//标准数据单元
			Get_StandardUnit(0x310C,Save_buf,&index,crrentnum,(INT8U*)&tsa,s_tsa);
			//属性3有关联数据
			Save_buf[index++]=6;//double-long-unsigned
			Save_buf[index++]=(olddata>>24)&0x000000ff;
			Save_buf[index++]=(olddata>>16)&0x000000ff;
			Save_buf[index++]=(olddata>>8)&0x000000ff;
			Save_buf[index++]=olddata&0x000000ff;
			Save_buf[index++]=6;//double-long-unsigned
			Save_buf[index++]=(newdata>>24)&0x000000ff;
			Save_buf[index++]=(newdata>>16)&0x000000ff;
			Save_buf[index++]=(newdata>>8)&0x000000ff;
			Save_buf[index++]=newdata&0x000000ff;
			Save_buf[STANDARD_NUM_INDEX]+=2;
			//存储更改后得参数
			saveCoverClass(0x310C,(INT16U)crrentnum,(void *)&event_object.Event310C_obj,sizeof(Event310C_Object),event_para_save);
			//存储记录集
			saveCoverClass(0x310C,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
			//存储当前记录值
			INT8U Currbuf[50]={};
			INT8U Currindex=0;
			Get_CurrResult(Currbuf,&Currindex,(INT8U*)&tsa,s_tsa,crrentnum,0);
			saveCoverClass(0x310C,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
			//判断是否要上报
			if(event_object.Event310C_obj.event_obj.reportflag)
				Need_Report(0x310C,crrentnum);
		 }
	 }
    //更新数据
    Refresh_Data(tsa,newdata,0);
    return 1;
}

/*
 * 电能表飞走事件12 前台两次电能值以及测量点额定电压、电流
 */
INT8U Event_310D(TSA tsa, INT8U* data,INT8U len) {
	if (event_object.Event310D_obj.event_obj.enableflag == 0) {
	        return 0;
	}
	if(data==NULL)
		return 0;
	INT32U newdata=(data[0]<<24)+(data[1]<<16)+(data[2]<<8)+data[3];
	INT32U olddata=0;
	TS ts;
	if(Get_Mdata(tsa,&olddata,&ts) == 0)
		return 0;
	/*===============TODO根据共享内存或者直接读取文件 获取该表参数*/
	CLASS_6001 meter={};
	INT32U power_offset=event_object.Event310D_obj.poweroffset_obj.power_offset;//超差值
	INT16U ratedU=meter.basicinfo.ratedU; //额定电压
	INT16U ratedI=meter.basicinfo.ratedI; //额定电流
	INT8U connectype=meter.basicinfo.connectype;//接线方式
	/*===============TODO可能需要根据下发得任务获取抄表间隔*/
	INT16U Intertime=15; //抄表间隔
	if(meter.basicinfo.port.OI == 0xF209)
		Intertime *=60;//载波表
	else
		Intertime =15;//485
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
	if ((newdata>olddata) && (olddata>0))
	 {
		 //kwh转换得扩大1000倍
		 if (((newdata-olddata)*1000>power_offset*Em) && (power_offset>0)){
			INT8U Save_buf[256];
			bzero(Save_buf, sizeof(Save_buf));
			event_object.Event310D_obj.event_obj.crrentnum++;
			Getcurrno(&event_object.Event310D_obj.event_obj.crrentnum,event_object.Event310D_obj.event_obj.maxnum);
			INT32U crrentnum = event_object.Event310D_obj.event_obj.crrentnum;
			INT8U index=0;
			//标准数据单元
			Get_StandardUnit(0x310D,Save_buf,&index,crrentnum,(INT8U*)&tsa,s_tsa);
			//属性3有关联数据
			Save_buf[index++]=6;//double-long-unsigned
			Save_buf[index++]=(olddata>>24)&0x000000ff;
			Save_buf[index++]=(olddata>>16)&0x000000ff;
			Save_buf[index++]=(olddata>>8)&0x000000ff;
			Save_buf[index++]=olddata&0x000000ff;
			Save_buf[index++]=6;//double-long-unsigned
			Save_buf[index++]=(newdata>>24)&0x000000ff;
			Save_buf[index++]=(newdata>>16)&0x000000ff;
			Save_buf[index++]=(newdata>>8)&0x000000ff;
			Save_buf[index++]=newdata&0x000000ff;
			Save_buf[STANDARD_NUM_INDEX]+=2;
			//存储更改后得参数
			saveCoverClass(0x310D,(INT16U)crrentnum,(void *)&event_object.Event310D_obj,sizeof(Event310D_Object),event_para_save);
			//存储记录集
			saveCoverClass(0x310D,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
			//存储当前记录值
			INT8U Currbuf[50]={};
			INT8U Currindex=0;
			Get_CurrResult(Currbuf,&Currindex,(INT8U*)&tsa,s_tsa,crrentnum,0);
			saveCoverClass(0x310D,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
			//判断是否要上报
			if(event_object.Event310D_obj.event_obj.reportflag)
				Need_Report(0x310D,crrentnum);
		 }
	 }
	//更新数据
	Refresh_Data(tsa,newdata,0);
	return 1;
}

/*
 * 电能表停走事件 前台两次电能值是否相同以及时间差是否超过设定值
 */
INT8U Event_310E(TSA tsa, INT8U* data,INT8U len) {
    if (event_object.Event310E_obj.event_obj.enableflag == 0) {
        return 0;
    }
    INT32U newdata=(data[0]<<24)+(data[1]<<16)+(data[2]<<8)+data[3];
	INT32U olddata=0;
	TS ts;
	if(Get_Mdata(tsa,&olddata,&ts) == 0)
		return 0;
	if(olddata == newdata){
		//若正向有功值相同
		TS currtime;
		TSGet(&currtime);
		int tcha=abs(difftime(tmtotime_t(ts),tmtotime_t(currtime)));
		INT32U offset=0;
		switch(event_object.Event310E_obj.powerstoppara_obj.power_offset.units){
			case 0:
				offset=event_object.Event310E_obj.powerstoppara_obj.power_offset.interval;
				break;
			case 1:
				offset=event_object.Event310E_obj.powerstoppara_obj.power_offset.interval*60;
				break;
			case 2:
				offset=event_object.Event310E_obj.powerstoppara_obj.power_offset.interval*60*60;
				break;
			case 3:
				offset=event_object.Event310E_obj.powerstoppara_obj.power_offset.interval*24*60*60;
				break;
			case 4:
				offset=event_object.Event310E_obj.powerstoppara_obj.power_offset.interval*30*24*60*60;
				break;
			case 5:
				offset=event_object.Event310E_obj.powerstoppara_obj.power_offset.interval*365*24*60*60;
				break;
			}
		if(tcha>offset && offset>0){
			INT8U Save_buf[256];
			bzero(Save_buf, sizeof(Save_buf));
			event_object.Event310E_obj.event_obj.crrentnum++;
			Getcurrno(&event_object.Event310E_obj.event_obj.crrentnum,event_object.Event310E_obj.event_obj.maxnum);
			INT32U crrentnum = event_object.Event310E_obj.event_obj.crrentnum;
			INT8U index=0;
			//标准数据单元
			Get_StandardUnit(0x310E,Save_buf,&index,crrentnum,(INT8U*)&tsa,s_tsa);
			//属性3有关联数据
			Save_buf[index++]=6;//double-long-unsigned
			Save_buf[index++]=(olddata>>24)&0x000000ff;
			Save_buf[index++]=(olddata>>16)&0x000000ff;
			Save_buf[index++]=(olddata>>8)&0x000000ff;
			Save_buf[index++]=olddata&0x000000ff;
			Save_buf[STANDARD_NUM_INDEX]+=1;
			//存储更改后得参数
			saveCoverClass(0x310E,(INT16U)crrentnum,(void *)&event_object.Event310E_obj,sizeof(Event310E_Object),event_para_save);
			//存储记录集
			saveCoverClass(0x310E,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
			//存储当前记录值
			INT8U Currbuf[50]={};
			INT8U Currindex=0;
			Get_CurrResult(Currbuf,&Currindex,(INT8U*)&tsa,s_tsa,crrentnum,0);
			saveCoverClass(0x310E,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
			//判断是否要上报
			if(event_object.Event310E_obj.event_obj.reportflag)
				Need_Report(0x310E,crrentnum);
		}
	}else
		Refresh_Data(tsa,newdata,1);//更新数据
    return 1;
}

/*
 * 抄表失败事件 抄表可自行判断是否抄表失败，可直接调用该接口
 */
INT8U Event_310F(TSA tsa, INT8U* data,INT8U len) {
    if (event_object.Event310F_obj.event_obj.enableflag == 0) {
        return 0;
    }
    INT8U Save_buf[256];
	bzero(Save_buf, sizeof(Save_buf));
	event_object.Event310F_obj.event_obj.crrentnum++;
	Getcurrno(&event_object.Event310F_obj.event_obj.crrentnum,event_object.Event310F_obj.event_obj.maxnum);
	INT32U crrentnum = event_object.Event310F_obj.event_obj.crrentnum;
	INT8U index=0;
	//标准数据单元
	Get_StandardUnit(0x310F,Save_buf,&index,crrentnum,(INT8U*)&tsa,s_tsa);
	//属性3有关联数据
	//最近一次抄表成功时间
	Save_buf[index++]=28;//datetime-s
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	//最近一次正向有功
	Save_buf[index++]=6;//double-long-unsigned
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	//最近一次正向有功
	Save_buf[index++]=5;//double-long
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	Save_buf[index++]=0;
	Save_buf[STANDARD_NUM_INDEX]+=3;
	//存储更改后得参数
	saveCoverClass(0x310F,(INT16U)crrentnum,(void *)&event_object.Event310F_obj,sizeof(Event310F_Object),event_para_save);
	//存储记录集
	saveCoverClass(0x310F,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
	//存储当前记录值
	INT8U Currbuf[50]={};
	INT8U Currindex=0;
	Get_CurrResult(Currbuf,&Currindex,(INT8U*)&tsa,s_tsa,crrentnum,0);
	saveCoverClass(0x310F,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
	//判断是否要上报
	if(event_object.Event310F_obj.event_obj.reportflag)
		Need_Report(0x310F,crrentnum);
    return 1;
}

/*
 * 月通信流量超限事件 data为当月已经发生流量 字节由高到低
 */
INT8U Event_3110(INT8U* data,INT8U len) {
    if (event_object.Event3110_obj.event_obj.enableflag == 0) {
        return 0;
    }
    INT32U monthbytes=(data[0]<<24)+(data[1]<<16)+(data[2]<<8)+data[3];
    INT32U offset=event_object.Event3110_obj.Monthtrans_obj.month_offset;
    //通信处判断还是这里判断 TODO
    if(monthbytes>offset){
    	INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		event_object.Event3110_obj.event_obj.crrentnum++;
		Getcurrno(&event_object.Event3110_obj.event_obj.crrentnum,event_object.Event3110_obj.event_obj.maxnum);
		INT32U crrentnum = event_object.Event3110_obj.event_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(0x3110,Save_buf,&index,crrentnum,NULL,s_null);
		//属性3有关联数据
		//事件发生后已发生通信流量 //22004202
		Save_buf[index++]=6;//double-long-unsiged
		Save_buf[index++]=data[0];
		Save_buf[index++]=data[1];
		Save_buf[index++]=data[2];
		Save_buf[index++]=data[3];
		//月通信流量门限 //31100601
		Save_buf[index++]=6;//double-long-unsigned
		Save_buf[index++]=(offset>>24)&0x000000ff;
		Save_buf[index++]=(offset>>16)&0x000000ff;
		Save_buf[index++]=(offset>>8)&0x000000ff;
		Save_buf[index++]=offset&0x000000ff;
		Save_buf[STANDARD_NUM_INDEX]+=2;
		//存储更改后得参数
		saveCoverClass(0x3110,(INT16U)crrentnum,(void *)&event_object.Event3110_obj,sizeof(Event3110_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3110,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
		saveCoverClass(0x3110,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(event_object.Event3110_obj.event_obj.reportflag)
			Need_Report(0x3110,crrentnum);
		return 1;
    }
    return 1;
}

/*
 * 发现未知电能表事件 抄表搜表可以判断出表信息，直接可调用该接口，默认data为整个电能表信息
 */
INT8U Event_3111(TSA tsa, INT8U* data,INT8U len) {
	if (event_object.Event3111_obj.enableflag == 0) {
	        return 0;
	    }
	if(data== NULL)
		return 0;
	INT8U Save_buf[256];
	bzero(Save_buf, sizeof(Save_buf));
	event_object.Event3111_obj.crrentnum++;
	Getcurrno(&event_object.Event3111_obj.crrentnum,event_object.Event3111_obj.maxnum);
	INT32U crrentnum = event_object.Event3111_obj.crrentnum;
	INT8U index=0;
	//标准数据单元
	Get_StandardUnit(0x3111,Save_buf,&index,crrentnum,NULL,s_null);
	//搜表结果集
	Save_buf[index++]=01;//array
	Save_buf[index++]=1;//默认搜到一个就产生事件
	Save_buf[index++]=2;//structure
	Save_buf[index++]=7;//元素数量
	//默认data是搜表记录结果结构
	//通信地址 TSA
	Save_buf[index++]=85;//TSA
	INT8U l1=data[0];
	memcpy(&Save_buf[index],&data[0],l1+1);
	index+=l1+1;
	//所属采集器地址 TSA
	Save_buf[index++]=85;//TSA
	INT8U l2=data[l1+1];
	memcpy(&Save_buf[index],&data[l1+1],l2+1);
	index+=l2+1;
	//规约类型  enum
	Save_buf[index++]=22;
	Save_buf[index++]=data[l1+1+l2+1];
	//相位 enum{未知（0），A（1），B（2），C（3）}
	Save_buf[index++]=22;
	Save_buf[index++]=data[l1+1+l2+1+1];
	//信号品质unsigned，
	Save_buf[index++]=17;
	Save_buf[index++]=data[l1+1+l2+1+2];
    //搜到的时间 date_time_s
	Save_buf[index++]=28;
	memcpy(&Save_buf[index],&data[l1+1+l2+1+3],7);
	index+=7;
	//搜到的附加信息  array附加信息
	Save_buf[index++]=1;//array
	Save_buf[index++]=0;//数量
	Save_buf[STANDARD_NUM_INDEX]+=1;
	//存储更改后得参数
	saveCoverClass(0x3111,(INT16U)crrentnum,(void *)&event_object.Event3111_obj,sizeof(Class7_Object),event_para_save);
	//存储记录集
	saveCoverClass(0x3111,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
	//存储当前记录值
	INT8U Currbuf[50]={};
	INT8U Currindex=0;
	Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
	saveCoverClass(0x3111,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
	//判断是否要上报
	if(event_object.Event3111_obj.reportflag)
		Need_Report(0x3111,crrentnum);
    return 1;
}

/*
 * 跨台区电能表事件17 抄表搜表可以判断出表信息，直接可调用该接口，默认data为整个垮台区电能表信息
 */
INT8U Event_3112(TSA tsa, INT8U* data,INT8U len) {
	if (event_object.Event3112_obj.enableflag == 0)
		return 0;
	if(data== NULL)
		return 0;
	INT8U Save_buf[256];
	bzero(Save_buf, sizeof(Save_buf));
	event_object.Event3112_obj.crrentnum++;
	Getcurrno(&event_object.Event3112_obj.crrentnum,event_object.Event3112_obj.maxnum);
	INT32U crrentnum = event_object.Event3112_obj.crrentnum;
	INT8U index=0;
	//标准数据单元
	Get_StandardUnit(0x3112,Save_buf,&index,crrentnum,NULL,s_null);
	//结果集
	Save_buf[index++]=01;//array
	Save_buf[index++]=1;//默认搜到一个就产生事件
	Save_buf[index++]=2;//structure
	Save_buf[index++]=3;//元素数量
	//默认data是搜表记录结果结构
    //通信地址 TSA
	Save_buf[index++]=85;//TSA
	INT8U l1=data[0];
	memcpy(&Save_buf[index],&data[0],l1+1);
	index+=l1+1;
	//所属采集器地址 TSA
	Save_buf[index++]=85;//TSA
	INT8U l2=data[l1+1];
	memcpy(&Save_buf[index],&data[l1+1],l2+1);
	index+=l2+1;
	//变更时间 date_time_s
	Save_buf[index++]=28;
	memcpy(&Save_buf[index],&data[l1+1+l2+1],7);
	index+=7;
	Save_buf[STANDARD_NUM_INDEX]+=1;
	//存储更改后得参数
	saveCoverClass(0x3112,(INT16U)crrentnum,(void *)&event_object.Event3112_obj,sizeof(Class7_Object),event_para_save);
	//存储记录集
	saveCoverClass(0x3112,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
	//存储当前记录值
	INT8U Currbuf[50]={};
	INT8U Currindex=0;
	Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
	saveCoverClass(0x3112,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
	//判断是否要上报
	if(event_object.Event3112_obj.reportflag)
		Need_Report(0x3112,crrentnum);
    return 1;
}

/*
 * 电能表在网状态切换事件24 怎么判断TODO？ data为电能表地址TSA及在网状态bool
 */
INT8U Event_311A(TSA tsa, INT8U* data,INT8U len) {
	if (event_object.Event311A_obj.event_obj.enableflag == 0)
		return 0;
	if(data== NULL)
		return 0;
	//怎么判定？
	INT16U time_delay=event_object.Event311A_obj.outtimepara_obj.outtime_offset;
	//如果发生
	if(1){
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		event_object.Event311A_obj.event_obj.crrentnum++;
		Getcurrno(&event_object.Event311A_obj.event_obj.crrentnum,event_object.Event311A_obj.event_obj.maxnum);
		INT32U crrentnum = event_object.Event311A_obj.event_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(0x311A,Save_buf,&index,crrentnum,NULL,s_null);
		//状态变迁事件
		Save_buf[index++]=01;//array
		Save_buf[index++]=1;//默认一个状态变迁事件
		Save_buf[index++]=2;//structure
		Save_buf[index++]=2;//元素数量
		//默认data是记录结果结构
		//电能表地址 TSA
		Save_buf[index++]=85;//TSA
		INT8U l1=data[0];
		memcpy(&Save_buf[index],&data[0],l1+1);
		index+=l1+1;
		//在网状态 bool
		Save_buf[index++]=3;//bool
		Save_buf[index++]=data[l1+1];
		Save_buf[STANDARD_NUM_INDEX]+=1;
		//存储更改后得参数
		saveCoverClass(0x311A,(INT16U)crrentnum,(void *)&event_object.Event311A_obj,sizeof(Event311A_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x311A,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
		saveCoverClass(0x311A,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(event_object.Event311A_obj.event_obj.reportflag)
			Need_Report(0x311A,crrentnum);
	}
    return 1;
}

/*
 * 终端对电表校时记录 抄表是否可以自行判断是较时？可直接调用该接口 data为较时前电表时间及误差
 */
INT8U Event_311B(TSA tsa, INT8U* data,INT8U len) {
	if (event_object.Event311B_obj.enableflag == 0)
		return 0;
	if(data== NULL)
		return 0;
    if(1){
    	INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		event_object.Event311B_obj.crrentnum++;
		Getcurrno(&event_object.Event311B_obj.crrentnum,event_object.Event311B_obj.maxnum);
		INT32U crrentnum = event_object.Event311B_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(0x311B,Save_buf,&index,crrentnum,(INT8U*)&tsa,s_tsa);
		//校时前时钟    date_time_s
		Save_buf[index++]=28;
		memcpy(&Save_buf[index],data,7);
		index+=7;
		//时钟误差      integer（单位：秒，无换算）
		Save_buf[index++]=15;
		Save_buf[index++]=data[7];
		Save_buf[STANDARD_NUM_INDEX]+=2;
		//存储更改后得参数
		saveCoverClass(0x311B,(INT16U)crrentnum,(void *)&event_object.Event311B_obj,sizeof(Class7_Object),1);
		//存储记录集
		saveCoverClass(0x311B,(INT16U)crrentnum,(void *)Save_buf,(int)index,2);
		//存储当前记录值
		INT8U Currbuf[50]={};
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,(INT8U*)&tsa,s_tsa,crrentnum,0);
		saveCoverClass(0x311B,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,3);
		//判断是否要上报
		if(event_object.Event311B_obj.reportflag)
			Need_Report(0x311B,crrentnum);
    }
    return 1;
}

/*
 * 电能表数据变更监控记录 抄表可自行判断，直接调用该函数。
 */
INT8U Event_311C(TSA tsa, INT8U* data,INT8U len) {
	if (event_object.Event311C_obj.event_obj.enableflag == 0)
		return 0;
	if(data== NULL)
		return 0;
	if(1){
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		event_object.Event311C_obj.event_obj.crrentnum++;
		Getcurrno(&event_object.Event311C_obj.event_obj.crrentnum,event_object.Event311C_obj.event_obj.maxnum);
		INT32U crrentnum = event_object.Event311C_obj.event_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(0x311C,Save_buf,&index,crrentnum,(INT8U*)&tsa,s_tsa);
		//监控数据对象  CSD 前台data，抄表直接组好，包括数据类型，因为是变长，只能这样处理，直接拷贝。
		memcpy(&Save_buf[index],data,len);
		index+=len;
		Save_buf[STANDARD_NUM_INDEX]+=3;
		//存储更改后得参数
		saveCoverClass(0x311C,(INT16U)crrentnum,(void *)&event_object.Event311C_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x311C,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,(INT8U*)&tsa,s_tsa,crrentnum,0);
		saveCoverClass(0x311C,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(event_object.Event311C_obj.event_obj.reportflag)
			Need_Report(0x311C,crrentnum);
	}
    return 1;
}

/*
 * 终端对时事件 此接口在698规约库调用，data为对时前时间 date-time-s格式 7个字节
 */
INT8U Event_3114(INT8U* data,INT8U len) {
    if (event_object.Event3114_obj.enableflag == 0) {
        return 0;
    }

    if(1){
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		event_object.Event3114_obj.crrentnum++;
		Getcurrno(&event_object.Event3114_obj.crrentnum,event_object.Event3114_obj.maxnum);
		INT32U crrentnum = event_object.Event3114_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(0x3114,Save_buf,&index,crrentnum,NULL,s_null);
		//事件发生前对时时间
		Save_buf[index++]=28;
		memcpy(&Save_buf[index],data,len);
		index+=len;
		//事件发生后对时时间
		DateTimeBCD ntime;
		DataTimeGet(&ntime);
		Save_buf[index++]=28;
		memcpy(&Save_buf[index],&ntime,7);
		index+=7;
		Save_buf[STANDARD_NUM_INDEX]+=2;
		//存储更改后得参数
		saveCoverClass(0x3114,(INT16U)crrentnum,(void *)&event_object.Event3114_obj,sizeof(Class7_Object),1);
		//存储记录集
		saveCoverClass(0x3114,(INT16U)crrentnum,(void *)Save_buf,(int)index,2);
		//存储当前记录值
		INT8U Currbuf[50]={};
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
		saveCoverClass(0x3114,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,3);
		//判断是否要上报
		if(event_object.Event3114_obj.reportflag)
			Need_Report(0x3114,crrentnum);
    }
    return 1;
}

/*
 * 输出回路接入状态变位事件记录
 */
INT8U Event_3117(INT8U* data,INT8U len) {
    if (event_object.Event3117_obj.enableflag == 0) {
        return 0;
    }

    if(1){
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		event_object.Event3117_obj.crrentnum++;
		Getcurrno(&event_object.Event3117_obj.crrentnum,event_object.Event3117_obj.maxnum);
		INT32U crrentnum = event_object.Event3117_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(0x3117,Save_buf,&index,crrentnum,NULL,s_null);
		//无属性3关联数据
		Save_buf[STANDARD_NUM_INDEX]+=0;
		//存储更改后得参数
		saveCoverClass(0x3117,(INT16U)crrentnum,(void *)&event_object.Event3117_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3117,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
		saveCoverClass(0x3117,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(event_object.Event3117_obj.reportflag)
			Need_Report(0x3117,crrentnum);
    }
    return 1;
}

/*
 * 终端编程记录 data为多个OAD集合，第一个字节为数量，后面是多个4个字节得OAD
 */
INT8U Event_3118(INT8U* data,INT8U len) {
    if (event_object.Event3118_obj.enableflag == 0) {
        return 0;
    }

    if(1){
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		event_object.Event3118_obj.crrentnum++;
		Getcurrno(&event_object.Event3118_obj.crrentnum,event_object.Event3118_obj.maxnum);
		INT32U crrentnum = event_object.Event3118_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(0x3118,Save_buf,&index,crrentnum,NULL,s_null);
		//array OAD
	    Save_buf[index++]=1;//array
	    Save_buf[index++]=data[0];//数量
	    int i=0;
	    for(i=0;i<data[0];i++){
	    	Save_buf[index++]=81;//OAD
	    	memcpy(&Save_buf[index],&data[1+i*4],4);
	    	index+=4;
	    }
		Save_buf[STANDARD_NUM_INDEX]+=1;
		//存储更改后得参数
		saveCoverClass(0x3118,(INT16U)crrentnum,(void *)&event_object.Event3118_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3118,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,NULL,s_null,crrentnum,0);
		saveCoverClass(0x3118,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(event_object.Event3118_obj.reportflag)
			Need_Report(0x3118,crrentnum);
    }
    return 1;
}

/*
 * 终端电流回路异常事件23,II型集中器没有电流，暂时不处理,type为0,1 短路、开路
 */
INT8U Event_3119(INT8U type, INT8U* data,INT8U len) {
    if (event_object.Event3119_obj.enableflag == 0) {
        return 0;
    }
    if(1){
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		event_object.Event3119_obj.crrentnum++;
		Getcurrno(&event_object.Event3119_obj.crrentnum,event_object.Event3119_obj.maxnum);
		INT32U crrentnum = event_object.Event3119_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(0x3119,Save_buf,&index,crrentnum,(INT8U*)&type,s_enum);
		//属性3无关联数据
		Save_buf[STANDARD_NUM_INDEX]+=0;
		//存储更改后得参数
		saveCoverClass(0x3119,(INT16U)crrentnum,(void *)&event_object.Event3119_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3119,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,(INT8U*)&type,s_enum,crrentnum,0);
		saveCoverClass(0x3119,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(event_object.Event3119_obj.reportflag)
			Need_Report(0x3119,crrentnum);
    }
    return 1;
}

/*
 * 功控跳闸记录 data为事件源OI+事件发生后2分钟功率long64+控制对象OI+跳闸轮次bit-string(SIZE(8))+功控定值long64+跳闸前总有加有功功率23012300
 */
INT8U Event_3200(INT8U* data,INT8U len) {
    if (event_object.Event3200_obj.enableflag == 0) {
        return 0;
    }

    if(1){
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		event_object.Event3200_obj.crrentnum++;
		Getcurrno(&event_object.Event3200_obj.crrentnum,event_object.Event3200_obj.maxnum);
		INT32U crrentnum = event_object.Event3200_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(0x3200,Save_buf,&index,crrentnum,(INT8U*)data,s_oi);
		//事件发生后2分钟功率long64
	    Save_buf[index++]=20;//long64
	    //data[0].data[1]为OI
        memcpy(&Save_buf[index],&data[2],8);
        index+=8;
        //控制对象OI
        Save_buf[index++]=80;//OI
        memcpy(&Save_buf[index],&data[2+8],2);
        index+=2;
        //跳闸轮次bit-string(SIZE(8))
        Save_buf[index++]=4;//BIT-STRING
        Save_buf[index++]=data[2+8+2];
        //功控定值long64
        Save_buf[index++]=20;//long64
		memcpy(&Save_buf[index],&data[2+8+2+1],8);
		index+=8;
		//跳闸前总有加有功功率23012300
		Save_buf[index++]=20;//long64
		memcpy(&Save_buf[index],&data[2+8+2+1+8],8);
		index+=8;
		Save_buf[STANDARD_NUM_INDEX]+=5;
		//存储更改后得参数
		saveCoverClass(0x3200,(INT16U)crrentnum,(void *)&event_object.Event3200_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3200,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,(INT8U*)data,s_oi,crrentnum,0);
		saveCoverClass(0x3200,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(event_object.Event3200_obj.reportflag)
			Need_Report(0x3200,crrentnum);
    }
    return 1;
}

/*
 * 电控跳闸记录 data为事件源OI+控制对象OI+跳闸轮次bit-string(SIZE(8))+电控定值long64+跳闸发生时总有加有功电量23014900array
 */
INT8U Event_3201(INT8U* data,INT8U len) {
    if (event_object.Event3201_obj.enableflag == 0) {
        return 0;
    }

    if(1){
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		event_object.Event3201_obj.crrentnum++;
		Getcurrno(&event_object.Event3201_obj.crrentnum,event_object.Event3201_obj.maxnum);
		INT32U crrentnum = event_object.Event3201_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(0x3201,Save_buf,&index,crrentnum,(INT8U*)data,s_oi);
        //控制对象OI
        Save_buf[index++]=80;//OI
        memcpy(&Save_buf[index],&data[2],2);
        index+=2;
        //跳闸轮次bit-string(SIZE(8))
        Save_buf[index++]=4;//BIT-STRING
        Save_buf[index++]=data[2+2];
        //电控定值long64
        Save_buf[index++]=20;//long64
		memcpy(&Save_buf[index],&data[2+2+1],8);
		index+=8;
		//跳闸发生时总有加有功电量23014900array
		Save_buf[index++]=20;//array
		Save_buf[index++]=0;//数量
		Save_buf[STANDARD_NUM_INDEX]+=4;
		//存储更改后得参数
		saveCoverClass(0x3201,(INT16U)crrentnum,(void *)&event_object.Event3201_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3201,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,(INT8U*)data,s_oi,crrentnum,0);
		saveCoverClass(0x3201,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(event_object.Event3201_obj.reportflag)
			Need_Report(0x3201,crrentnum);
    }
    return 1;
}

/*
 * 购电参数设置记录29
 */
INT8U Event_3202(INT8U* data,INT8U len) {
    //暂时不使用
	if (event_object.Event3202_obj.enableflag == 0) {
		return 0;
	}
	//oi:8107 or 810c
	if(1){
		INT8U oi[2]={0x81,0x07};
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		event_object.Event3202_obj.crrentnum++;
		Getcurrno(&event_object.Event3202_obj.crrentnum,event_object.Event3202_obj.maxnum);
		INT32U crrentnum = event_object.Event3202_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(0x3202,Save_buf,&index,crrentnum,(INT8U*)oi,s_oi);
		//属性3无关联数据
		Save_buf[STANDARD_NUM_INDEX]+=0;
		//存储更改后得参数
		saveCoverClass(0x3202,(INT16U)crrentnum,(void *)&event_object.Event3202_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3202,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,(INT8U*)oi,s_oi,crrentnum,0);
		saveCoverClass(0x3202,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(event_object.Event3202_obj.reportflag)
			Need_Report(0x3202,crrentnum);
	 }
    return 1;
}

/*
 * 电控告警事件记录  data为事件源OI+控制对象OI+电控定值long64
 */
INT8U Event_3203(INT8U* data,INT8U len) {
    //暂时不使用
	 if (event_object.Event3203_obj.enableflag == 0) {
	        return 0;
	}

	if(1){
		INT8U Save_buf[256];
		bzero(Save_buf, sizeof(Save_buf));
		event_object.Event3203_obj.crrentnum++;
		Getcurrno(&event_object.Event3203_obj.crrentnum,event_object.Event3203_obj.maxnum);
		INT32U crrentnum = event_object.Event3203_obj.crrentnum;
		INT8U index=0;
		//标准数据单元
		Get_StandardUnit(0x3203,Save_buf,&index,crrentnum,(INT8U*)data,s_oi);
		//控制对象OI
		Save_buf[index++]=80;//OI
		memcpy(&Save_buf[index],&data[2],2);
		index+=2;
		//电控定值long64
		Save_buf[index++]=20;//long64
		memcpy(&Save_buf[index],&data[2+2],8);
		index+=8;
		Save_buf[STANDARD_NUM_INDEX]+=2;
		//存储更改后得参数
		saveCoverClass(0x3203,(INT16U)crrentnum,(void *)&event_object.Event3203_obj,sizeof(Class7_Object),event_para_save);
		//存储记录集
		saveCoverClass(0x3203,(INT16U)crrentnum,(void *)Save_buf,(int)index,event_record_save);
		//存储当前记录值
		INT8U Currbuf[50]={};
		INT8U Currindex=0;
		Get_CurrResult(Currbuf,&Currindex,(INT8U*)data,s_oi,crrentnum,0);
		saveCoverClass(0x3203,(INT16U)crrentnum,(void *)Currbuf,(int)Currindex,event_current_save);
		//判断是否要上报
		if(event_object.Event3203_obj.reportflag)
			Need_Report(0x3203,crrentnum);
	}
    return 1;
}
