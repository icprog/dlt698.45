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

char name1[128]={};
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
void print6000(OI_698	oi)
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
				fprintf(stderr,"[1]%d-%d-",meter.basicinfo.addr.addr[0],meter.basicinfo.addr.addr[1]);
				if(meter.basicinfo.addr.addr[0]>TSA_LEN)   fprintf(stderr,"TSA 长度[%d]超过17个字节，错误！！！\n",meter.basicinfo.addr.addr[0]);
				for(j=0;j<(meter.basicinfo.addr.addr[1]+1);j++) {
					fprintf(stderr,"%02x",meter.basicinfo.addr.addr[j+2]);
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
				for(j=0;j<(meter.extinfo.cjq_addr.addr[1]+1);j++) {
					fprintf(stderr,"%02x",meter.extinfo.cjq_addr.addr[j+2]);
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

/*
 * 采集档案配置单元
 * */
void Collect6000(int argc, char *argv[])
{
	CLASS_6001	 meter={};
	int		ret = -1, pi=0, po=0;
	int		i=0;
	int 	tmp[50]={};
	int		seqno=0;

	if(strcmp("pro",argv[2])==0) {
		if(argc<5) {
			print6000(0x6000);
		}
	}
	if(strcmp("delete",argv[2])==0) {
		if(argc==5) {
			seqno = atoi(argv[4]);
			fprintf(stderr,"删除 采集档案配置序号： %d\n",seqno);
			if(readParaClass(0x6000,&meter,seqno)==1) {
				if(seqno == meter.sernum) {
					meter.sernum = 0;
					ret = saveParaClass(0x6000,&meter,seqno);
					if(ret==1)
						fprintf(stderr,"删除 序号 %d 成功, ret=%d\n",seqno,ret);
				}
			}
		}
	}
	if(strcmp("add",argv[2])==0) {
		if(argc<5) {
			fprintf(stderr,"\n添加一个采集档案配置单元：配置项0-12：\n[0]配置序号 \n");
			fprintf(stderr,"基本信息:[1]通信地址  [2]波特率  [3]规约  [4]端口OAD  [5]费率个数  [6]用户类型  [7]接线方式  [8]额定电压  [9]额定电流 \n");
			fprintf(stderr,"扩展信息:[10]采集器地址 [11]PT [12]CT\n");
			fprintf(stderr,"配置说明:\n【1】通信地址TSA，【10】采集器地址:第一个字节为TSA长度如： 05 12 34 56 78 9A\n");
			fprintf(stderr,"【2】波特率:300bps(0),600bps(1),1200bps(2),2400bps(3),4800bps(4),7200bps(5),9600bps(6),19200bps(7),38400bps(8),57600bps(9),115200bps(10),自适应(255)\n");
			fprintf(stderr,"【3】规约:未知(0),DL/T645-1997(1),DL/T645-2007(2),DL/T698.45(3),CJ/T188-2004(4)\n");
			fprintf(stderr,"【4】OAD:格式输入 04x-04x，如OAD=f2010201, 输入：f201-0201\n");
			fprintf(stderr,"【7】接线方式:未知(0),单相(1),三相三线(2),三相四线(3)\n");
			fprintf(stderr,"\n例如<配置序号1的内容>：\n");
			fprintf(stderr,"cj coll add 6000 序号 表地址 		波特率 规约 端口OAD 费率格式 用户类型 接线方式 额定电压 额定电流 采集器地址 PT CT\n");
			fprintf(stderr,"cj coll add 6000 1    06 18 00 03 35 15 52  3     2  f201-0201   4       1        2         220     15      0       2200 1500\n");
			fprintf(stderr,"cj coll add 6000 1  06 18 00 03 35 15 52  3 2 f201-0201 4 1 2 220 15 0 2200 1500\n");
		}else {
			memset(&meter,0,sizeof(CLASS_6001));
			pi = 4;
			po = 0;
			sscanf(argv[pi],"%d",&tmp[po]);
			meter.sernum = tmp[po];
			pi++;
			po++;
			fprintf(stderr,"sernum=%d ",meter.sernum);
			///TSA
			sscanf(argv[pi],"%d",&tmp[po]);
			meter.basicinfo.addr.addr[1]=tmp[po];	// addr[0] :array num  addr[1]: TSA_len(后面数据帧为tsa_len+1) addr[2]...addr[2+TSA_len+1]
			pi++;
			po++;
			for(i=0;i<(meter.basicinfo.addr.addr[1]);i++) {
				sscanf(argv[pi],"%02x",&tmp[po]);
				meter.basicinfo.addr.addr[2+i] = tmp[po];
				pi++;
				po++;
			}
			meter.basicinfo.addr.addr[0]=meter.basicinfo.addr.addr[1]+1;
			meter.basicinfo.addr.addr[1]=meter.basicinfo.addr.addr[1]-1;

			fprintf(stderr,"TSA=%d-%d ",meter.basicinfo.addr.addr[0],meter.basicinfo.addr.addr[1]);
			for(i=0;i<(meter.basicinfo.addr.addr[1]+1);i++) {
				fprintf(stderr,"%02x ",meter.basicinfo.addr.addr[i+2]);
			}

			//////
			sscanf(argv[pi],"%d",&tmp[po]);
			meter.basicinfo.baud = tmp[po];
			pi++;
			po++;
			fprintf(stderr,"\nbaud=%d ",meter.basicinfo.baud);

			sscanf(argv[pi],"%d",&tmp[po]);
			meter.basicinfo.protocol = tmp[po];
			pi++;
			po++;
			fprintf(stderr,"\nprotocol=%d ",meter.basicinfo.protocol);

			sscanf(argv[pi],"%04x-%04x",&tmp[po],&tmp[po+1]);
			meter.basicinfo.port.OI = tmp[po];
			meter.basicinfo.port.attflg = (tmp[po+1]>>8) & 0xff;
			meter.basicinfo.port.attrindex = tmp[po+1] & 0xff;
			pi++;
			po=po+2;
			fprintf(stderr,"\nOAD=%04x %02x%02x ",meter.basicinfo.port.OI,meter.basicinfo.port.attflg,meter.basicinfo.port.attrindex);
			memset(&meter.basicinfo.pwd,0,sizeof(meter.basicinfo.pwd));
			sscanf(argv[pi],"%d",&tmp[po]);
			meter.basicinfo.ratenum = tmp[po];
			pi++;
			po++;
			sscanf(argv[pi],"%d",&tmp[po]);
			meter.basicinfo.usrtype = tmp[po];
			pi++;
			po++;
			sscanf(argv[pi],"%d",&tmp[po]);
			meter.basicinfo.connectype = tmp[po];
			pi++;
			po++;
			sscanf(argv[pi],"%d",&tmp[po]);
			meter.basicinfo.ratedU = tmp[po];
			pi++;
			po++;
			sscanf(argv[pi],"%d",&tmp[po]);
			meter.basicinfo.ratedI = tmp[po];
			pi++;
			po++;
			sscanf(argv[pi],"%d",&tmp[po]);
			meter.extinfo.cjq_addr.addr[1]=tmp[po];	// addr[0] :array num  addr[1]: TSA_len(后面数据帧为tsa_len+1) addr[2]...addr[2+TSA_len+1]
			pi++;
			po++;
			////TSA
			for(i=0;i<(meter.extinfo.cjq_addr.addr[1]);i++) {
				sscanf(argv[pi],"%02x",&tmp[po]);
				meter.extinfo.cjq_addr.addr[2+i] = tmp[po];
				pi++;
				po++;
			}
			meter.extinfo.cjq_addr.addr[0]=meter.extinfo.cjq_addr.addr[1]+1;
			if(meter.extinfo.cjq_addr.addr[1]!=0) {
				meter.extinfo.cjq_addr.addr[1]=meter.extinfo.cjq_addr.addr[1]-1;
			}
			///////
			sscanf(argv[pi],"%d",&tmp[po]);
			meter.extinfo.pt = tmp[po];
			pi++;
			po++;
			sscanf(argv[pi],"%d",&tmp[po]);
			meter.extinfo.ct = tmp[po];
			pi++;
			po++;
			ret = saveParaClass(0x6000,&meter,meter.sernum);
			fprintf(stderr,"保存采集档案配置单元 序号 %d, ret=%d\n",meter.sernum,ret);
		}
	}
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
	fprintf(stderr,"运行时段:%d",class6013.runtime.num);
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
	if(class6015.csds.num >= MY_CSD_NUM) {
		fprintf(stderr,"csd overvalue MY_CSD_NUM error\n");
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

void print_6017(CLASS_6017 eventFangAn)
{
	INT8U j=0;

	fprintf(stderr,"\n事件采集方案：[1]方案编号 [2]采集事件数据ROAD [3]电能表集合MS [4]上报标识 [5]存储深度");

	fprintf(stderr,"\n[1]方案编号 ：%d ",eventFangAn.sernum);
	fprintf(stderr,"\n[2]ROAD{%d}\n",eventFangAn.roads.num);
	for(j=0;j<eventFangAn.roads.num;j++)
	{
		print_road(eventFangAn.roads.road[j]);
	}
	fprintf(stderr,"[3]");
	printMS(eventFangAn.ms);
	fprintf(stderr,"\n[4]%d  ",eventFangAn.ifreport);
	fprintf(stderr,"[5]%d\n",eventFangAn.deepsize);
}

//事件上报方案
void Event6017(int argc, char *argv[])
{
	int		ret = -1;
	int		i=0;
	int 	tmp[30]={};
	INT8U	taskid=0;
	OI_698	oi=0;
	CLASS_6017 eventFangAn={};

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
					memset(&eventFangAn,0,sizeof(CLASS_6017));
					if(readCoverClass(oi,taskid,&eventFangAn,sizeof(CLASS_6017),coll_para_save)== 1) {
						print_6017(eventFangAn);
					}else {
//						fprintf(stderr,"任务ID=%d 无任务配置单元",taskid);
					}
				}
			}else if(argc==5) {
				sscanf(argv[4],"%d",&tmp[0]);
				taskid = tmp[0];
				fprintf(stderr,"taskid=%d\n",taskid);
				memset(&eventFangAn,0,sizeof(CLASS_6017));
				if(readCoverClass(oi,taskid,&eventFangAn,sizeof(CLASS_6017),coll_para_save)==1) {
					print_6017(eventFangAn);
				}else {
					fprintf(stderr,"无任务配置单元");
				}
			}
		}
	}
}

void print_601d(CLASS_601D	 reportplan)
{
	int j=0;
	fprintf(stderr,"\n[1]上报方案编号 [2]上报通道 [3]上报响应超时时间 [4]最大上报次数 [5]上报内容 {[5.1]类型(0:OAD,1:RecordData) [5.2]数据 [5.3]RSD}");
	fprintf(stderr,"\n[1]上报方案编号:%d \n",reportplan.reportnum);
	fprintf(stderr,"[2]OAD[%d] ",reportplan.chann_oad.num);
	for(j=0;j<reportplan.chann_oad.num;j++) {
		fprintf(stderr,"%04x-%02x%02x ",reportplan.chann_oad.oadarr[j].OI,reportplan.chann_oad.oadarr[j].attflg,reportplan.chann_oad.oadarr[j].attrindex);
	}
	fprintf(stderr," [3]TI %d-%d ",reportplan.timeout.units,reportplan.timeout.interval);
	fprintf(stderr," [4]%d ",reportplan.maxreportnum);
	fprintf(stderr," [5.1]%d ",reportplan.reportdata.type);
	if(reportplan.reportdata.type==0) {
		fprintf(stderr," [5.2]OAD:%04x-%02x%02x ",reportplan.reportdata.data.oad.OI,reportplan.reportdata.data.oad.attflg,reportplan.reportdata.data.oad.attrindex);
	}else {
		fprintf(stderr," [5.2]OAD:%04x-%02x%02x ",reportplan.reportdata.data.oad.OI,reportplan.reportdata.data.oad.attflg,reportplan.reportdata.data.oad.attrindex);
		print_rcsd(reportplan.reportdata.data.recorddata.csds);
		fprintf(stderr," [5.4]");
		print_rsd(reportplan.reportdata.data.recorddata.selectType,reportplan.reportdata.data.recorddata.rsd);
	}
	fprintf(stderr,"\n\n");
}

//任务上报方案
void Report601d(int argc, char *argv[])
{
	int		ret = -1;
	int		i=0;
	int 	tmp[30]={};
	INT8U	taskid=0;
	OI_698	oi=0;
	CLASS_601D	 reportplan={};

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
					memset(&reportplan,0,sizeof(CLASS_601D));
					if(readCoverClass(oi,taskid,&reportplan,sizeof(CLASS_601D),coll_para_save)== 1) {
						print_601d(reportplan);
					}else {
//						fprintf(stderr,"任务ID=%d 无任务配置单元",taskid);
					}
				}
			}else if(argc==5) {
				sscanf(argv[4],"%d",&tmp[0]);
				taskid = tmp[0];
				fprintf(stderr,"taskid=%d\n",taskid);
				memset(&reportplan,0,sizeof(CLASS_601D));
				if(readCoverClass(oi,taskid,&reportplan,sizeof(CLASS_601D),coll_para_save)==1) {
					print_601d(reportplan);
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
				Collect6000(argc,argv);
				break;
			case 0x6013:
				Task6013(argc,argv);
				break;
			case 0x6015:
				Task6015(argc,argv);
				break;
			case 0x6017:
				Event6017(argc,argv);
				break;
			case 0x601d:
				Report601d(argc,argv);
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
		fprintf(stderr,"\n文件里的TSA：");
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
int buf_int(INT8U  *buf)
{
	int value=0;
	value = buf[0];
	value = (value<<8) + buf[1];
	return value;
}
int buf_int2(INT8U  *buf)
{
	int value=0;
	value = buf[1];
	value = (value<<8) + buf[0];
	return value;
}
int readfile_int(FILE *fp)
{
	INT8U buf[2]={};
	int value=0;
	if (fp!=NULL)
	{
		if(fread(buf,2,1,fp)>0)
		{
			//value = buf[0];
			//value = (value<<8) + buf[1];
			value = buf_int(buf);
		}
	}
	return value;
}

int getOADf(INT8U type,INT8U *source,OAD *oad)		//0x51
{
	if((type == 1) || (type == 0)) {
		oad->OI = source[type+1];
		oad->OI = (oad->OI <<8) | source[type];
		oad->attflg = source[type+2];
		oad->attrindex = source[type+3];
		return (4+type);
	}
	return 0;
}

typedef struct{
	OAD   oad_m;
	OAD   oad_r;
	INT16U len;
}HEAD_UNIT0;
int findtsa(FILE *fp,int *TSA_D,int A_TSAblock)
{
	INT8U tmp=0,buf[20]={};
	int begitoffset =0 ;
	int k = 0,i=1;
	int findok = 1;

	for(;;)
	{
		findok = 1;
		begitoffset = ftell(fp);
		if (fread(&tmp,1,1,fp)<=0)
		{
			findok = 0;
			break;
		}
		if(tmp!=0X55)
		{
			findok = 0;
			break;
		}
//		fprintf(stderr,"\n标识%02x",tmp);
		fread(&tmp,1,1,fp);
//		fprintf(stderr,"  长度%d",tmp);
		memset(buf,0,20);
		fread(&buf,tmp,1,fp);

		fprintf(stderr,"\n TSA%d:",i++);
		for(k=0;k<tmp;k++)
		{
			fprintf(stderr," %02x",buf[k]);
		}
		for(k=0;k<tmp;k++)
		{
			if(buf[k]!=TSA_D[k])
				findok = 0;
		}
		if (findok==0)
		{
			fseek(fp,begitoffset+A_TSAblock,0);
		}else
		{
			fseek(fp,begitoffset,0);
			break;
		}
	}
	return findok;
}
void record_prt(int recordnum,int indexn,HEAD_UNIT0 *length,FILE *fp)
{
	int	nonullflag=0;
	int k=0,i=0,j=0;
	int	nullbuf[50]={};
	INT8U buf[50]={};

	memset(&nullbuf,0,sizeof(nullbuf));
	for(k=0;k<recordnum;k++)
	{
		nonullflag=0;
		for(i=0;i<indexn;i++)
		{
			memset(buf,0,50);
			if (fread(buf,length[i].len,1,fp)>0)
			{
			}else
				break;
			if((length[i].oad_r.OI==0x6040) || (length[i].oad_r.OI==0x6041) || (length[i].oad_r.OI==0x6042)) {
				if(memcmp(buf,nullbuf,sizeof(length[i].len))!=0) {		//存在数据
					nonullflag=1;
				}
			}
			if(nonullflag) {
				fprintf(stderr,"\n%04x . %04x  %02d字节     |",length[i].oad_m.OI,length[i].oad_r.OI,length[i].len);
				for(j=0;j<length[i].len;j++)
					fprintf(stderr,"%02x ",buf[j]);
				switch(length[i].oad_r.OI) {
				case 0x6040:
					fprintf(stderr," 采集启动时标: %04d-%02d-%02d %02d:%02d:%02d",(buf[1]<<8 | buf[2]),buf[3],buf[4],buf[5],buf[6],buf[7]);
					break;
				case 0x6041:
					fprintf(stderr," 采集成功时标: %04d-%02d-%02d %02d:%02d:%02d",(buf[1]<<8 | buf[2]),buf[3],buf[4],buf[5],buf[6],buf[7]);
					break;
				case 0x6042:
					fprintf(stderr," 采集存储时标: %04d-%02d-%02d %02d:%02d:%02d",(buf[1]<<8 | buf[2]),buf[3],buf[4],buf[5],buf[6],buf[7]);
					break;
				}
			}
		}
		if(nonullflag) {
			fprintf(stderr,"\n记录%d",k);
		}
	}
	return ;
}
int head_prt(int unitnum,HEAD_UNIT0 *length,int *indexn,FILE *fp)
{
	INT8U buf[50]={};
	int A_record=0,i=0,j=0;
	OAD oad;

	for(i=0;i<unitnum  ;i++)
	{
		memset(buf,0,50);
		fread(buf,10,1,fp);
		getOADf(0,&buf[0],&oad);
		memcpy(&length[i].oad_m,&oad,sizeof(oad));
		fprintf(stderr,"\n【%02d】  %04x-%02x-%02x   ",i,oad.OI,oad.attflg,oad.attrindex);
		getOADf(0,&buf[4],&oad);
		memcpy(&length[i].oad_r,&oad,sizeof(oad));
		fprintf(stderr,  "%04x-%02x-%02x   ",oad.OI,oad.attflg,oad.attrindex);
		length[i].len = buf_int2(&buf[8]);
		fprintf(stderr," %02d 字节        |   ",length[i].len);
		(*indexn)++;
		for(j=0;j<10;j++)
			fprintf(stderr,"%02x ",buf[j]);
		if (i==3)
			fprintf(stderr,"\n");
		A_record += length[i].len;
	}
	return A_record ;
}

void analyTaskData(int argc, char* argv[])
{
	int TSA_D[20]={};
	char *filename= argv[2];
	FILE *fp=NULL;
	int i=0, indexn=0,A_record=0,A_TSAblock=0;
	HEAD_UNIT0 length[20];
	int tsanum=0 , head_len=0,recordnum =0,haveTsa =0 , unitnum=0;

	if (filename!=NULL)
	{
		if(argc>3)
		{
			tsanum = argc -3;
			haveTsa = tsanum;   //人工输入的TSA目标字节数
			fprintf(stderr,"\n\n\n【 目标地址: ");
			for(i=0;i<tsanum;i++)
			{
				sscanf(argv[i+3],"%02x",&TSA_D[i]);
				fprintf(stderr,"%02x ",TSA_D[i]);
			}
			fprintf(stderr,"】");
		}

		fp = fopen(filename,"r");
		if(fp!=NULL)
		{
			fprintf(stderr,"\n\n\n--------------------------------------------------------");
			head_len = readfile_int(fp);
			fprintf(stderr,"\n文件头长度 %d (字节)",head_len);

			A_TSAblock = readfile_int(fp);
			memset(&length,0,sizeof(length));
			unitnum = (head_len )/sizeof(HEAD_UNIT0);

			//打印文件头结构
			A_record = head_prt(unitnum,length,&indexn,fp);

			recordnum = A_TSAblock/A_record;
			fprintf(stderr,"\n\n\n TSA块 %d（字节）  每记录 %d （字节）  共 %d 条记录\n",A_TSAblock,A_record,recordnum);

			if (findtsa(fp,TSA_D,A_TSAblock)==1)
			{
				fprintf(stderr,"\n\n\n>>>>查找到相关的TSA数据\n");
				//打印TSA的全部记录
				record_prt(recordnum,indexn,length,fp);
			}
			else
				fprintf(stderr,"\n\n\n>>>>未查找到相关的TSA数据\n\n\n");
			fclose(fp);
		}
	}
	return ;
}
