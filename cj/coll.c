/*
 * coll.c
 *
 *  Created on: Jan 19, 2017
 *      Author: AVA
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "AccessFun.h"
#include "StdDataType.h"
#include "Objectdef.h"
#include "main.h"
#include "dlt698.h"
#include "PublicFunction.h"

typedef enum{
	coll_bps=1,
	coll_protocol,
	coll_wiretype,
	task_ti,
	task_cjtype,
	task_prio,
	task_status,
	task_runtime,
	coll_mode,
	ms_type,
	savetime_sel,
}OBJ_ENUM;

char *getenum(int type,int val)
{
	char name1[128]={};
	char *name=NULL;

	name = name1;
	memset(name1,0,sizeof(name1));
//	fprintf(stderr,"val=%d ,type=%d\n",val,type);
	switch(type) {
	case coll_bps:
		if(val==bps300)	strcpy(name,"300");
		if(val==bps600)	strcpy(name,"600");
		if(val==bps1200)	strcpy(name,"1200");
		if(val==bps2400)	strcpy(name,"2400");
		if(val==bps4800)	strcpy(name,"4800");
		if(val==bps7200)	strcpy(name,"7200");
		if(val==bps9600)	strcpy(name,"9600");
		if(val==bps19200)	strcpy(name,"19200");
		if(val==bps38400)	strcpy(name,"38400");
		if(val==bps57600)	strcpy(name,"57600");
		if(val==bps115200)	strcpy(name,"115200");
		if(val==autoa)		strcpy(name,"自适应");
		break;
	case coll_protocol:
		if(val==0)	strcpy(name,"未知");
		if(val==1)	strcpy(name,"DL/T645-1997");
		if(val==2)	strcpy(name,"DL/T645-2007");
		if(val==3)	strcpy(name,"DL/T698.45");
		if(val==4)	strcpy(name,"CJ/T18802004");
		break;
	case coll_wiretype:
		if(val==0)	strcpy(name,"未知");
		if(val==1)	strcpy(name,"单相");
		if(val==2)	strcpy(name,"三相三线");
		if(val==3)	strcpy(name,"三相四线");
		break;
	case task_ti:
		if(val==0)	strcpy(name,"秒");
		if(val==1)	strcpy(name,"分");
		if(val==2)	strcpy(name,"时");
		if(val==3)	strcpy(name,"日");
		if(val==4)	strcpy(name,"月");
		if(val==5)	strcpy(name,"年");
		break;
	case task_cjtype:
		if(val==1)	strcpy(name,"普通采集方案");
		if(val==2)	strcpy(name,"事件采集方案");
		if(val==3)	strcpy(name,"透明方案");
		if(val==4)	strcpy(name,"上报方案");
		if(val==5)	strcpy(name,"脚本方案");
		break;
	case task_prio:
		if(val==1)	strcpy(name,"首要");
		if(val==2)	strcpy(name,"必要");
		if(val==3)	strcpy(name,"需要");
		if(val==4)	strcpy(name,"可能");
		break;
	case task_status:
		if(val==1)	strcpy(name,"正常");
		if(val==2)	strcpy(name,"停用");
		break;
	case task_runtime:
		if(val==0)	strcpy(name,"前闭后开");
		if(val==1)	strcpy(name,"前开后闭");
		if(val==2)	strcpy(name,"前闭后闭");
		if(val==3)	strcpy(name,"前开后开");
		break;
	case coll_mode:
		if(val==0)	strcpy(name,"采集当前数据");
		if(val==1)	strcpy(name,"采集上第N次");
		if(val==2)	strcpy(name,"按冻结时标采集");
		if(val==3)	strcpy(name,"按时间间隔采集");
		if(val==4)	strcpy(name,"补抄");
		break;
	case ms_type:
		if(val==0)	strcpy(name,"无电能表");
		if(val==1)	strcpy(name,"全部用户地址");
		if(val==2)	strcpy(name,"一组用户类型");
		if(val==3)	strcpy(name,"一组用户地址");
		if(val==4)	strcpy(name,"一组配置序号");
		if(val==5)	strcpy(name,"一组用户类型区间");
		if(val==6)	strcpy(name,"一组用户地址区间");
		if(val==7)	strcpy(name,"一组配置序号区间");
		break;
	case savetime_sel:
		if(val==0)	strcpy(name,"未定义");
		if(val==1)	strcpy(name,"任务开始时间");
		if(val==2)	strcpy(name,"相对当日0点0分");
		if(val==3)	strcpy(name,"相对上日23点59分");
		if(val==4)	strcpy(name,"相对上日0点0分");
		if(val==5)	strcpy(name,"相对当月1日0点0分");
		if(val==6)	strcpy(name,"数据冻结时标");
		break;
	}
	return name;
}
/*
 * 采集档案配置表
 * */
void Collect6000(OI_698	oi)
{
	CLASS_6001	 meter={};
	CLASS11		coll={};
	int			i=0,j=0,blknum=0;
//	INT16U		oi = 0x6000;

	if(readInterClass(oi,&coll)==-1)  return;
	fprintf(stderr,"采集档案配置表CLASS_11--------------");
	fprintf(stderr,"逻辑名:%s    当前=%d     最大=%d\n",coll.logic_name,coll.curr_num,coll.max_num);

	blknum = getFileRecordNum(oi);
	if(blknum == -1) {
		fprintf(stderr,"未找到OI=%04x的相关信息配置内容！！！\n",oi);
		return;
	}else if(blknum == -2){
		fprintf(stderr,"采集档案表不是整数，检查文件完整性！！！\n");
		return;
	}
	fprintf(stderr,"采集档案配置单元文件记录个数：【%d】\n",blknum);
	fprintf(stderr,"基本信息:[1]通信地址  [2]波特率  [3]规约  [4]端口OAD  [5]通信密码  [6]费率个数  [7]用户类型  [8]接线方式  [9]额定电压  [10]额定电流 \n");
	fprintf(stderr,"扩展信息:[11]采集器地址 [12]资产号 [13]PT [14]CT\n");
	fprintf(stderr,"附属信息:[15]对象属性OAD  [16]属性值\n");
	for(i=0;i<blknum;i++) {
		if(readParaClass(oi,&meter,i)==1) {
			if(meter.sernum!=0 && meter.sernum!=0xffff) {
				fprintf(stderr,"\n序号:%d ",meter.sernum);
				fprintf(stderr,"[1]");
				if(meter.basicinfo.addr.addr[0]>TSA_LEN)   fprintf(stderr,"TSA 长度[%d]超过17个字节，错误！！！\n",meter.basicinfo.addr.addr[0]);
				for(j=0;j<meter.basicinfo.addr.addr[0];j++) {
					fprintf(stderr,"%02x",meter.basicinfo.addr.addr[j+1]);
//				fprintf(stderr,"[1]%02x%02x%02x%02x%02x%02x ",
//						meter.basicinfo.addr.addr[0],meter.basicinfo.addr.addr[1],meter.basicinfo.addr.addr[2],meter.basicinfo.addr.addr[3],
//						meter.basicinfo.addr.addr[4],meter.basicinfo.addr.addr[5]);
				}
				fprintf(stderr," [2]%s ",getenum(coll_bps,meter.basicinfo.baud));
				fprintf(stderr,"[3]%s ",getenum(coll_protocol,meter.basicinfo.protocol));
				fprintf(stderr,"[4]%04X_%02X%02X ",meter.basicinfo.port.OI,meter.basicinfo.port.attflg,meter.basicinfo.port.attrindex);
				fprintf(stderr,"[5]");
				for(j=0;j<meter.basicinfo.pwd[0];j++) {
					fprintf(stderr,"%02x",meter.basicinfo.pwd[j+1]);
				}
				fprintf(stderr,"[6]%d [7]%d ",meter.basicinfo.ratenum,meter.basicinfo.usrtype);
				fprintf(stderr,"[8]%s ",getenum(coll_wiretype,meter.basicinfo.connectype));
				fprintf(stderr,"[9]%d [10]%d ",meter.basicinfo.ratedU,meter.basicinfo.ratedI);
				fprintf(stderr,"[11]");
				for(j=0;j<meter.extinfo.cjq_addr.addr[0];j++) {
					fprintf(stderr,"%02x",meter.extinfo.cjq_addr.addr[j+1]);
				}
				fprintf(stderr," [12]");
				for(j=0;j<meter.extinfo.asset_code[0];j++) {
					fprintf(stderr,"%02x",meter.extinfo.asset_code[j+1]);
				}
				fprintf(stderr," [13]%d [14]%d",meter.extinfo.pt,meter.extinfo.ct);
				fprintf(stderr,"\n       [15]%04X_%02X%02X",meter.aninfo.oad.OI,meter.aninfo.oad.attflg,meter.aninfo.oad.attrindex);
			}
		}
	}
	fprintf(stderr,"\n");
}

void print6013(CLASS_6013 class6013)
{
	INT8U	i=0;
	fprintf(stderr,"【6013】任务配置单元: 任务ID--%d\n",class6013.taskID);
	fprintf(stderr,"       [1]%s-%d ",getenum(task_ti,class6013.interval.units),class6013.interval.interval);
	fprintf(stderr,"[2]%s  [3]%d   ",getenum(task_cjtype,class6013.cjtype),class6013.sernum);
	fprintf(stderr,"[4]%d-%d-%d %d:%d:%d ",class6013.startime.year.data,class6013.startime.month.data,class6013.startime.day.data,
			class6013.startime.hour.data,class6013.startime.min.data,class6013.startime.sec.data);
	fprintf(stderr,"[5]%d-%d-%d %d:%d:%d ",class6013.endtime.year.data,class6013.endtime.month.data,class6013.endtime.day.data,
			class6013.endtime.hour.data,class6013.endtime.min.data,class6013.endtime.sec.data);
	fprintf(stderr,"[6]%s-%d ",getenum(task_ti,class6013.delay.units),class6013.delay.interval);
	fprintf(stderr,"[7]%s  ",getenum(task_prio,class6013.runprio));
	fprintf(stderr,"[8]%s  [9]%d  [10]%d ",getenum(task_status,class6013.state),class6013.befscript,class6013.aftscript);
	fprintf(stderr,"[11]%s ",getenum(task_runtime,class6013.runtime.type));
	for(i=0;i<class6013.runtime.num;i++) {
		fprintf(stderr,"[%d:%d %d:%d] ",class6013.runtime.runtime[i].beginHour,class6013.runtime.runtime[i].beginMin,
									    class6013.runtime.runtime[i].endHour,class6013.runtime.runtime[i].endMin);
	}
	fprintf(stderr,"\n");
}

//任务配置单元
void Task6013(int argc, char *argv[])
{
	int		ret = -1, pi=0, po=0;
	int		i=0;
	int 	tmp[30]={};
	INT8U	taskid=0;
	OI_698	oi=0;
	CLASS_6013	class6013={};

	sscanf(argv[3],"%04x",&tmp[0]);
	oi = tmp[0];
	if(strcmp("clear",argv[2])==0) {
		ret = clearClass(oi);
		if(ret==-1) {
			fprintf(stderr,"清空出错=%d",ret);
		}
	}
	if(strcmp("delete",argv[2])==0) {
		if(argc==5) {
			sscanf(argv[4],"%d",&tmp[0]);
			taskid = tmp[0];
			if(deleteClass(oi,taskid)==1) {
				fprintf(stderr,"删除一个配置单元oi【%04x】【%d】成功",oi,taskid);
			}
		}else fprintf(stderr,"参数错误，查看cj help");
	}else {
		if(strcmp("pro",argv[2])==0) {
			if(argc<5) {
				fprintf(stderr,"[1]执行频率 [2]方案类型 [3]方案编号 [4]开始时间 [5]结束时间 [6]延时 [7]执行优先级 [8]状态 [9]开始前脚本id [10]开始后脚本id [11]运行时段【起始HH:MM 结束HH:MM】\n");
				for(i=0;i<=255;i++) {
//					sscanf(argv[4],"%04x",&tmp[0]);
					taskid = i;
	//				fprintf(stderr,"taskid=%d\n",taskid);
					memset(&class6013,0,sizeof(CLASS_6013));
					if(readCoverClass(oi,taskid,&class6013,sizeof(class6013),coll_para_save)==1) {
						print6013(class6013);
					}else {
//						fprintf(stderr,"任务ID=%d 无任务配置单元",taskid);
					}
				}
			}else if(argc==5) {
				sscanf(argv[4],"%04x",&tmp[0]);
				taskid = tmp[0];
				fprintf(stderr,"taskid=%d\n",taskid);
				memset(&class6013,0,sizeof(CLASS_6013));
				if(readCoverClass(oi,taskid,&class6013,sizeof(class6013),coll_para_save)==1) {
					print6013(class6013);
				}else {
					fprintf(stderr,"无任务配置单元");
				}
			}else {
				memset(&class6013,0,sizeof(CLASS_6013));
				pi = 4;
				po = 0;
				sscanf(argv[pi],"%d",&tmp[po]);
				class6013.taskID = tmp[po];
				pi++;
				po++;
				sscanf(argv[pi],"%d-%d",&tmp[po],&tmp[po+1]);
				class6013.interval.units = tmp[po];
				class6013.interval.interval = tmp[po+1];
				pi++;
				po=po+2;
				sscanf(argv[pi],"%d",&tmp[po]);
				class6013.cjtype = tmp[po];
				pi++;
				po++;
				sscanf(argv[pi],"%d",&tmp[po]);
				class6013.sernum = tmp[po];
				pi++;
				po++;
				sscanf(argv[pi],"%d-%d-%d",&tmp[po],&tmp[po+1],&tmp[po+2]);
				class6013.startime.year.data = tmp[po];
				class6013.startime.month.data = tmp[po+1];
				class6013.startime.day.data = tmp[po+2];
				pi++;
				po=po+3;
				sscanf(argv[pi],"%d:%d:%d",&tmp[po],&tmp[po+1],&tmp[po+2]);
				class6013.startime.hour.data = tmp[po];
				class6013.startime.min.data = tmp[po+1];
				class6013.startime.sec.data = tmp[po+2];
				pi++;
				po=po+3;
				sscanf(argv[pi],"%d-%d-%d",&tmp[po],&tmp[po+1],&tmp[po+2]);
				class6013.endtime.year.data = tmp[po];
				class6013.endtime.month.data = tmp[po+1];
				class6013.endtime.day.data = tmp[po+2];
				pi++;
				po=po+3;
				sscanf(argv[pi],"%d:%d:%d",&tmp[po],&tmp[po+1],&tmp[po+2]);
				class6013.endtime.hour.data = tmp[po];
				class6013.endtime.min.data = tmp[po+1];
				class6013.endtime.sec.data = tmp[po+2];
				pi++;
				po=po+3;
				sscanf(argv[pi],"%d-%d",&tmp[po],&tmp[po+1]);
				class6013.delay.units = tmp[po];
				class6013.delay.interval = tmp[po+1];
				pi++;
				po=po+2;
				sscanf(argv[pi],"%d",&tmp[po]);
				class6013.runprio = tmp[po];
				pi++;
				po++;
				sscanf(argv[pi],"%d",&tmp[po]);
				class6013.state = tmp[po];
				pi++;
				po++;
				sscanf(argv[pi],"%d",&tmp[po]);
				class6013.runtime.type = tmp[po];
				pi++;
				po++;
				sscanf(argv[pi],"%d:%d-%d:%d",&tmp[po],&tmp[po+1],&tmp[po+2],&tmp[po+3]);
				class6013.runtime.runtime[0].beginHour=tmp[po];
				class6013.runtime.runtime[0].beginMin=tmp[po+1];
				class6013.runtime.runtime[0].endHour=tmp[po+2];
				class6013.runtime.runtime[0].endMin=tmp[po+3];
				pi++;
				po=po+4;
				saveCoverClass(oi,class6013.taskID,&class6013,sizeof(CLASS_6013),coll_para_save);
			}
		}
	}
}

void print6015(CLASS_6015 class6015)
{
	INT8U type=0,w=0,i=0;

	fprintf(stderr,"[1]方案编号 [2]存储深度 [3]采集类型 [4]采集内容 [5]OAD-ROAD [6]MS [7]存储时标\n");
	fprintf(stderr,"[6015]普通采集方案:[1]方案号: %d  \n",class6015.sernum);
	fprintf(stderr,"     [2]%d  [3]%s ",class6015.deepsize,getenum(coll_mode,class6015.cjtype));
	switch(class6015.cjtype) {
	case 0: // NULL
		fprintf(stderr,"[4]%02x ",class6015.data.data[0]);
		break;
	case 1:	//unsigned
		fprintf(stderr,"[4]%02x ",class6015.data.data[0]);
		break;
	case 2:// NULL
		fprintf(stderr,"[4]%02x ",class6015.data.data[0]);
		break;
	case 3://TI
		fprintf(stderr,"[4]%s-%d ",getenum(task_ti,class6015.data.data[0]),((class6015.data.data[2]<<8)|class6015.data.data[1]));
		break;
	case 4://RetryMetering
		fprintf(stderr,"[4]%s-%d %d\n",getenum(task_ti,class6015.data.data[0]),((class6015.data.data[2]<<8)|class6015.data.data[1]),
									((class6015.data.data[4]<<8)|class6015.data.data[3]));
		break;
	}
	if(class6015.csds.num >= 10) {
		fprintf(stderr,"csd overvalue 10 error\n");
		return;
	}
	fprintf(stderr,"[5]");
	for(i=0; i<class6015.csds.num;i++)
	{
		type = class6015.csds.csd[i].type;
		if (type==0)
		{
			fprintf(stderr,"<%d>OAD%04x-%02x%02x ",i,class6015.csds.csd[i].csd.oad.OI,class6015.csds.csd[i].csd.oad.attflg,class6015.csds.csd[i].csd.oad.attrindex);
		}else if (type==1)
		{
			fprintf(stderr,"<%d>ROAD%04x-%02x%02x ",i,
					class6015.csds.csd[i].csd.road.oad.OI,class6015.csds.csd[i].csd.road.oad.attflg,class6015.csds.csd[i].csd.road.oad.attrindex);
			if(class6015.csds.csd[i].csd.road.num >= 16) {
				fprintf(stderr,"csd overvalue 16 error\n");
				return;
			}
//			fprintf(stderr,"csds.num=%d\n",class6015.csds.num);
			for(w=0;w<class6015.csds.csd[i].csd.road.num;w++)
			{
				fprintf(stderr,"<..%d>%04x-%02x%02x ",w,
						class6015.csds.csd[i].csd.road.oads[w].OI,class6015.csds.csd[i].csd.road.oads[w].attflg,class6015.csds.csd[i].csd.road.oads[w].attrindex);
			}
		}
	}
	fprintf(stderr,"[6]%s ",getenum(ms_type,class6015.mst.mstype));
	fprintf(stderr,"[7]%s ",getenum(savetime_sel,class6015.savetimeflag));
	fprintf(stderr,"\n");

}

//普通采集方案
void Task6015(int argc, char *argv[])
{
	int		ret = -1;
	int		i=0;
	int 	tmp[30]={};
	INT8U	taskid=0;
	OI_698	oi=0;
	CLASS_6015	class6015={};

	sscanf(argv[3],"%04x",&tmp[0]);
	oi = tmp[0];
	if(strcmp("clear",argv[2])==0) {
		ret = clearClass(oi);
		if(ret==-1) {
			fprintf(stderr,"清空出错=%d",ret);
		}
	}
	if(strcmp("delete",argv[2])==0) {
		if(argc==5) {
			sscanf(argv[4],"%d",&tmp[0]);
			taskid = tmp[0];
			if(deleteClass(oi,taskid)==1) {
				fprintf(stderr,"删除一个配置单元oi【%04x】【%d】成功",oi,taskid);
			}
		}else fprintf(stderr,"参数错误，查看cj help");
	}else {
		if(strcmp("pro",argv[2])==0) {
			if(argc<5) {
//				fprintf(stderr,"[1]方案编号 [2]存储深度 [3]采集类型 [4]采集内容 [5]OAD-ROAD [6]MS [7]存储时标\n");
				for(i=0;i<=255;i++) {
					taskid = i;
					memset(&class6015,0,sizeof(CLASS_6015));
					if(readCoverClass(oi,taskid,&class6015,sizeof(CLASS_6015),coll_para_save)== 1) {
						print6015(class6015);
					}else {
//						fprintf(stderr,"任务ID=%d 无任务配置单元",taskid);
					}
				}
			}else if(argc==5) {
				sscanf(argv[4],"%04x",&tmp[0]);
				taskid = tmp[0];
				fprintf(stderr,"taskid=%d\n",taskid);
				memset(&class6015,0,sizeof(CLASS_6015));
				if(readCoverClass(oi,taskid,&class6015,sizeof(class6015),coll_para_save)==1) {
					print6015(class6015);
				}else {
					fprintf(stderr,"无任务配置单元");
				}
			}
		}
	}
}

void print6035(CLASS_6035 class6035)
{
	fprintf(stderr,"[6035]采集任务监控单元 \n");
	fprintf(stderr,"[1]任务ID [2]执行状态<0:未执行 1:执行中 2:已执行> [3]任务执行开始时间 [4]任务执行结束时间 [5]采集总数量 [6]采集成功数量 [7]已发送报文条数 [8]已接收报文条数 \n");
	fprintf(stderr,"[1]%d [2]%d ",class6035.taskID,class6035.taskState);
	fprintf(stderr,"[3]%d-%d-%d %02d:%02d:%02d ",class6035.starttime.year.data,class6035.starttime.month.data,class6035.starttime.day.data,
			class6035.starttime.hour.data,class6035.starttime.min.data,class6035.starttime.sec.data);
	fprintf(stderr,"[4]%d-%d-%d %02d:%02d:%02d ",class6035.endtime.year.data,class6035.endtime.month.data,class6035.endtime.day.data,
			class6035.endtime.hour.data,class6035.endtime.min.data,class6035.endtime.sec.data);
	fprintf(stderr,"[5]%d [6]%d [7]%d [8]%d\n",class6035.totalMSNum,class6035.successMSNum,class6035.sendMsgNum,class6035.rcvMsgNum);
}
//采集任务监控
void Task6035(int argc, char *argv[])
{
	int		ret = -1;
	int		i=0;
	int 	tmp[30]={};
	INT8U	taskid=0;
	OI_698	oi=0;
	CLASS_6035	class6035={};

	sscanf(argv[3],"%04x",&tmp[0]);
	oi = tmp[0];
	if(strcmp("clear",argv[2])==0) {
		ret = clearClass(oi);
		if(ret==-1) {
			fprintf(stderr,"清空出错=%d",ret);
		}
	}
	if(strcmp("delete",argv[2])==0) {
		if(argc==5) {
			sscanf(argv[4],"%d",&tmp[0]);
			taskid = tmp[0];
			if(deleteClass(oi,taskid)==1) {
				fprintf(stderr,"删除一个配置单元oi【%04x】【%d】成功",oi,taskid);
			}
		}else fprintf(stderr,"参数错误，查看cj help");
	}else {
		if(strcmp("pro",argv[2])==0) {
			if(argc<5) {
				for(i=0;i<=255;i++) {
					taskid = i;
					memset(&class6035,0,sizeof(CLASS_6035));
					if(readCoverClass(oi,taskid,&class6035,sizeof(CLASS_6035),coll_para_save)== 1) {
						print6035(class6035);
					}else {
//						fprintf(stderr,"任务ID=%d 无任务配置单元",taskid);
					}
				}
			}else if(argc==5) {
				sscanf(argv[4],"%04x",&tmp[0]);
				taskid = tmp[0];
				fprintf(stderr,"taskid=%d\n",taskid);
				memset(&class6035,0,sizeof(CLASS_6035));
				if(readCoverClass(oi,taskid,&class6035,sizeof(class6035),coll_para_save)==1) {
					print6035(class6035);
				}else {
					fprintf(stderr,"无任务配置单元");
				}
			}
		}
	}
}

void coll_process(int argc, char *argv[])
{
	int 	tmp=0;
	OI_698	oi=0;

	if(argc>=2) {	//coll pro 6000
		if(strcmp(argv[1],"coll")==0) {
			sscanf(argv[3],"%04x",&tmp);
			oi = tmp;
			switch(oi) {
			case 0x6000:
				Collect6000(oi);
				break;
			case 0x6013:
				Task6013(argc,argv);
				break;
			case 0x6015:
				Task6015(argc,argv);
				break;
			case 0x6035:
				Task6035(argc,argv);
				break;
			}
		}
	}
}

void cjframe(int argc, char *argv[])
{
	INT8U tempbuf[512];
	int len=0,i=0,tmp=0;
	fprintf(stderr,"\nargc =%d",argc);
	if (argc>3)
	{
		len = argc - 2;//cj test 05 02 23 34 54 67
		if (len>512)
			return;
		memset(tempbuf,0,512);
		for(i=0;i<len;i++)
		{
			sscanf(argv[i+2],"%02x",&tmp);
			tempbuf[i] = (INT8U)tmp;
		}
		testframe(tempbuf,len);
	}
}
/*
 * 读取文件头长度和块数据长度
 */
void ReadFileHeadLen(FILE *fp,INT16U *headlen,INT16U *blocklen)
{
	INT16U headlength=0,blocklength=0;
	fread(&headlength,2,1,fp);
	*headlen = ((headlength>>8)+((headlength&0xff)<<8));
	fread(&blocklength,2,1,fp);
	*blocklen = ((blocklength>>8)+((blocklength&0xff)<<8));
}
typedef struct{
	INT8U type;//0：oad 1：road
	OAD   oad;
	INT8U num[2];//长度或个数，类型为0，表示长度；类型为1，表示个数
}HEAD_UNIT;
void ReadFileHead(FILE *fp,INT16U headlen,INT16U unitlen,INT16U unitnum,INT8U *headbuf)
{
	HEAD_UNIT *headunit = NULL;
//	INT16U unitlen = 0,unitnum = 0;
//	INT16U headlen=0;
	int i=0;
	fread(headbuf,headlen,1,fp);
	headunit = malloc(headlen-4);
	memcpy(headunit,&headbuf[4],headlen-4);
	for(i=0;i<unitnum;i++)
	{
		fprintf(stderr,"\ntype:");
		fprintf(stderr,"%02x",headunit[i].type);
		fprintf(stderr,"oad:");
		fprintf(stderr,"%04x%02x%02x",headunit[i].oad.OI,headunit[i].oad.attflg,headunit[i].oad.attrindex);
		fprintf(stderr," num:");
		fprintf(stderr,"%02x%02x",headunit[i].num[0],headunit[i].num[1]);
	}
	fprintf(stderr,"\n");
	free(headunit);
}
/*
 * 读取抄表数据，读取某个测量点某任务某天一整块数据，放在内存里，根据需要提取数据,并根据csd
 */
void ReadNorData(TS ts,INT8U taskid,INT8U *tsa)
{
	FILE *fp  = NULL;
	INT16U headlen=0,unitlen=0,unitnum=0,readlen=0;
	INT8U *databuf_tmp=NULL;
	INT8U *headbuf=NULL;
	int i=0,j=0;
	int savepos=0;
	TS ts_now;
	TSGet(&ts_now);
	char	fname[128]={};
	TASKSET_INFO tasknor_info;
	taskid=1;
	if(ReadTaskInfo(taskid,&tasknor_info)!=1)
		return;
	getTaskFileName(taskid,ts_now,fname);
	fp = fopen(fname,"r");
	if(fp == NULL)//文件没内容 组文件头，如果文件已存在，提取文件头信息
	{
		fprintf(stderr,"\nopen file %s fail\n",fname);
		return;
	}
	ReadFileHeadLen(fp,&headlen,&unitlen);
	headbuf = (INT8U *)malloc(headlen);
	unitnum = (headlen-4)/sizeof(HEAD_UNIT);
	ReadFileHead(fp,headlen,unitlen,unitnum,headbuf);
	databuf_tmp = malloc(unitlen);
	fseek(fp,headlen,SEEK_SET);//跳过文件头
	while(!feof(fp))//找存储结构位置
	{
		//00 00 00 00 00 00 00 00 07 05 00 00 00 00 00 01
		//00 00 00 00 00 00 00 00 00 07 05 00 00 00 00 00 01
		readlen = fread(databuf_tmp,unitlen,1,fp);
		fprintf(stderr,"\n----读出来的长度readlen=%d,块大小unitlen=%d\n",readlen,unitlen);
		if(readlen == 0)
			break;
		fprintf(stderr,"\n");
		fprintf(stderr,"\n传进来的TSA：");
		for(i=0;i<TSA_LEN;i++)
			fprintf(stderr," %02x",tsa[i]);
		for(i=0;i<TSA_LEN;i++)
			fprintf(stderr," %02x",databuf_tmp[i+1]);
		if(memcmp(&databuf_tmp[1],&tsa[0],17)==0)//找到了存储结构的位置，一个存储结构可能含有unitnum个单元
		{
			savepos=ftell(fp)-unitlen;//对应的放到对应的位置
			break;
		}
	}
	fprintf(stderr,"\n----(pos:%d):\n",savepos);
	if(savepos==0)//没找到的不打印
		return;
	fprintf(stderr,"\n要求的tsa:");
	for(j=0;j<17;j++)
		fprintf(stderr,"%02x ",tsa[j]);
	fprintf(stderr,"\n文件中的数据(pos:%d):",savepos);
	fseek(fp,savepos,SEEK_SET);//
	fread(databuf_tmp,unitlen,1,fp);
//		for(j=0;j<unitlen;j++)
//		{
//			fprintf(stderr,"%02x ",databuf_tmp[j]);
//		}
	fprintf(stderr,"\n---存储单元数：%d 每个单元长度:%d\n",tasknor_info.runtime,unitlen/tasknor_info.runtime);
	fprintf(stderr,"\n文件%s存储的数据(%d)：",fname,unitlen);
	fprintf(stderr,"\n");

	for(i=0;i<unitlen;i++)
	{
		if(i%(unitlen/tasknor_info.runtime) == 0)
			fprintf(stderr,"\n%d:",i/(unitlen/tasknor_info.runtime));

		fprintf(stderr," %02x",databuf_tmp[i]);
	}

}
void cjread(int argc, char *argv[])
{
	INT8U tsa[17];
	INT8U taskid=0;
	TS ts_now;
	int len=0,i=0,tmp=0;
	fprintf(stderr,"\nargc =%d",argc);
	if (argc>3)
	{
		len = argc - 3;//cj test 05 02 23 34 54 67
		sscanf(argv[2],"%d",&tmp);
		if(tmp >= 256)
			return;
		if (len>18)
			return;
		taskid = tmp;
		memset(tsa,0,17);
		for(i=0;i<len;i++)
		{
			sscanf(argv[i+3],"%02x",&tmp);
			tsa[i] = (INT8U)tmp;
		}
		TSGet(&ts_now);
		ReadNorData(ts_now,taskid,tsa);
	}
}
