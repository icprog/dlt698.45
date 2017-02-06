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
	}
	return name;
}
/*
 * 采集档案配置表
 * */
void Collect6000(OI_698	oi)
{
	CLASS_6001	 meter={};
	COLL_CLASS_11	coll={};
	int			i=0,blknum=0;
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
				fprintf(stderr,"[1]%02x%02x%02x%02x%02x%02x ",
						meter.basicinfo.addr.addr[0],meter.basicinfo.addr.addr[1],meter.basicinfo.addr.addr[2],meter.basicinfo.addr.addr[3],
						meter.basicinfo.addr.addr[4],meter.basicinfo.addr.addr[5]);
				fprintf(stderr,"[2]%s ",getenum(coll_bps,meter.basicinfo.baud));
				fprintf(stderr,"[3]%s ",getenum(coll_protocol,meter.basicinfo.protocol));
				fprintf(stderr,"[4]%04X_%02X%02X ",meter.basicinfo.port.OI,meter.basicinfo.port.attflg,meter.basicinfo.port.attrindex);
				fprintf(stderr,"[5]%02x%02x%02x%02x%02x%02x ",meter.basicinfo.pwd[0],meter.basicinfo.pwd[1],meter.basicinfo.pwd[2],
												meter.basicinfo.pwd[3],meter.basicinfo.pwd[4],meter.basicinfo.pwd[5]);
				fprintf(stderr,"[6]%d [7]%d ",meter.basicinfo.ratenum,meter.basicinfo.usrtype);
				fprintf(stderr,"[8]%s ",getenum(coll_wiretype,meter.basicinfo.connectype));
				fprintf(stderr,"[9]%d [10]%d ",meter.basicinfo.ratedU,meter.basicinfo.ratedI);
				fprintf(stderr,"\n       [11]%02x%02x%02x%02x%02x%02x ",
						meter.extinfo.cjq_addr.addr[0],meter.extinfo.cjq_addr.addr[1],meter.extinfo.cjq_addr.addr[2],
						meter.extinfo.cjq_addr.addr[3],meter.extinfo.cjq_addr.addr[4],meter.extinfo.cjq_addr.addr[5]);
				fprintf(stderr,"[12]%02x%02x%02x%02x%02x%02x ",
						meter.extinfo.asset_code[0],meter.extinfo.asset_code[1],meter.extinfo.asset_code[2],meter.extinfo.asset_code[3],
						meter.extinfo.asset_code[4],meter.extinfo.asset_code[5]);
				fprintf(stderr,"[13]%d [14]%d",meter.extinfo.pt,meter.extinfo.ct);
				fprintf(stderr,"\n       [15]%04X_%02X%02X",meter.aninfo.oad.OI,meter.aninfo.oad.attflg,meter.aninfo.oad.attrindex);
			}
		}
	}
	fprintf(stderr,"\n");
}

void print6013(CLASS_6013 class6013)
{
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
	fprintf(stderr,"[11]%s [%d:%d %d:%d] ",getenum(task_runtime,class6013.runtime.type),class6013.runtime.runtime[0].beginHour,
			class6013.runtime.runtime[0].beginMin,class6013.runtime.runtime[0].endHour,class6013.runtime.runtime[0].endMin);
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
#include <unistd.h>
void print6015(CLASS_6015 class6015)
{
	fprintf(stderr,"[1]方案编号 [2]存储深度 [3]采集类型 [4]采集内容 [5]OAD-ROAD [6]MS [7]存储时标\n");
	fprintf(stderr,"普通采集方案: \n[1]方案编号--%d",class6015.sernum);
	fprintf(stderr,"\n存储深度：%d \n采集类型：%s ",class6015.deepsize,getenum(coll_mode,class6015.cjtype));
	fprintf(stderr,"\n采集内容：type-%02x value-",class6015.data.type);
	usleep(10 * 1000);
	switch(class6015.cjtype) {
	case 0: // NULL
		fprintf(stderr,"%02x (NULL)",class6015.data.data[0]);
		break;
	case 1:	//unsigned
		fprintf(stderr,"%02x (unsigned)",class6015.data.data[0]);
		break;
	case 2:// NULL
		fprintf(stderr,"%02x (NULL)",class6015.data.data[0]);
		break;
	case 3://TI
		fprintf(stderr,"%d-%s (TI)",(class6015.data.data[2]|class6015.data.data[1]),getenum(task_ti,class6015.data.data[0]));
		break;
	case 4://RetryMetering
		fprintf(stderr,"[4]%s-%d %d(Retry)",getenum(task_ti,class6015.data.data[0]),(class6015.data.data[2]|class6015.data.data[1]),
									(class6015.data.data[4]|class6015.data.data[3]));
		break;
	}
//	fprintf(stderr,"\n%04x %02x %02x",class6015.csd[0].road.oad.OI,class6015.csd[0].road.oad.attflg,class6015.csd[0].road.oad.attrindex);
//	fprintf(stderr,"\n%04x %02x %02x ",class6015.csd[0].road.oads[0].OI,class6015.csd[0].road.oads[0].attflg,class6015.csd[0].road.oads[0].attrindex);
//	fprintf(stderr,"\n%04x %02x %02x ",class6015.csd[0].road.oads[1].OI,class6015.csd[0].road.oads[1].attflg,class6015.csd[0].road.oads[1].attrindex);
//	fprintf(stderr,"\n%04x %02x %02x ",class6015.csd[0].road.oads[2].OI,class6015.csd[0].road.oads[2].attflg,class6015.csd[0].road.oads[2].attrindex);
//	fprintf(stderr,"\n%04x %02x %02x ",class6015.csd[0].road.oads[3].OI,class6015.csd[0].road.oads[3].attflg,class6015.csd[0].road.oads[3].attrindex);




//	if(class6015.csd.type==0) fprintf(stderr,"[5]OAD");
//	else if(class6015.csd.type==1) fprintf(stderr,"[5]ROAD");
//	fprintf(stderr,"%04x-%02x%02x ",class6015.csd[0].rcsd.oad.OI,class6015.csd[0].rcsd.oad.attflg,class6015.csd[0].rcsd.oad.attrindex);
}

//普通采集方案
void Task6015(int argc, char *argv[])
{
	int		ret = -1, pi=0, po=0;
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
					if(readCoverClass(oi,taskid,&class6015,sizeof(CLASS_6015),coll_para_save)== -1) {
//						fprintf(stderr,"任务ID=%d 无任务配置单元",taskid);
					}else {
						print6015(class6015);
					}
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
			}
		}
	}
}

