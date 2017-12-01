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
#include "dlt645.h"
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
CLASS_6015 fangAn6015[20];

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
		if(val==0)	strcpy(name,"0");
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
		if(val==7)	strcpy(name,"相对上月月末0点0分");
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
				fprintf(stderr," [2]%s(%d) ",getenum(coll_bps,meter.basicinfo.baud),meter.basicinfo.baud);
				fprintf(stderr,"[3]%s ",getenum(coll_protocol,meter.basicinfo.protocol));
				fprintf(stderr,"[4]%04X_%02X%02X ",meter.basicinfo.port.OI,meter.basicinfo.port.attflg,meter.basicinfo.port.attrindex);
				fprintf(stderr,"[5]");
				for(j=0;j<meter.basicinfo.pwd[0];j++) {
					fprintf(stderr,"%02x",meter.basicinfo.pwd[j+1]);
				}
				fprintf(stderr,"[6]%d [7]%02x ",meter.basicinfo.ratenum,meter.basicinfo.usrtype);
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
				fprintf(stderr," [15]%04X_%02X%02X",meter.aninfo.oad.OI,meter.aninfo.oad.attflg,meter.aninfo.oad.attrindex);
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
			        setOIChange_CJ(0x6000);
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
			fprintf(stderr,"【4】OAD:格式输入 04x-04x，如OAD=f2010201, 输入：f201-0201(485-I) f201-0202(485-II)\n");
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
			meter.basicinfo.pwd[0] = 1;

			sscanf(argv[pi],"%d",&tmp[po]);
			meter.basicinfo.ratenum = tmp[po];
			pi++;
			po++;
			sscanf(argv[pi],"%x",&tmp[po]);
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
			memset(&meter.extinfo.asset_code,0,sizeof(meter.extinfo.asset_code));
			meter.extinfo.asset_code[0] = 1;

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
	        setOIChange_CJ(0x6000);
		}
	}
}

void print6002(CLASS_6002 class6002)
{
	int	i=0,j=0;
	fprintf(stderr,"方法127：实时启动搜表（搜表时长:单位：分钟）  %d\n",class6002.startSearchLen);
	fprintf(stderr,"属性10:(空闲(0),搜表中(1))  %d\n",class6002.searchSta);
	fprintf(stderr,"属性6:(所有搜表结果记录数)  %d\n",class6002.searchNum);
	fprintf(stderr,"属性7:(跨台区搜表结果记录数)  %d\n",class6002.crosszoneNum);
	fprintf(stderr,"属性8:\n");
	fprintf(stderr,"	是否启用每天周期搜表: %d\n",class6002.attr8.enablePeriodFlg);
	fprintf(stderr,"	自动更新采集档案: %d\n",class6002.attr8.autoUpdateFlg);
	fprintf(stderr,"	是否产生搜表相关事件: %d\n",class6002.attr8.eventFlg);
	fprintf(stderr,"	清空搜表结果选项[不清空(0)，每天周期搜表前清空(1),每次搜表前清空(2)]: %d\n",class6002.attr8.clearChoice);
	fprintf(stderr,"属性9(每天周期搜表参数配置): 个数  %d\n",class6002.attr9_num);
	for(i=0;i<class6002.attr9_num;i++) {
		fprintf(stderr,"	开始时间  %d:%d:%d\n",class6002.attr9[i].startTime[0],class6002.attr9[i].startTime[1],class6002.attr9[i].startTime[2]);
		fprintf(stderr,"	搜表时长（min）  %d\n",class6002.attr9[i].searchLen);
	}
	fprintf(stderr,"属性2(所有搜表结果)\n");
	for(i=0;i<class6002.searchNum;i++) {
		fprintf(stderr,"	通信地址");
		printTSA(class6002.searchResult[i].CommAddr);
		fprintf(stderr,"	所属采集器地址");
		printTSA(class6002.searchResult[i].CJQAddr);
		fprintf(stderr,"	规约类型 %d\n",class6002.searchResult[i].protocol);
		fprintf(stderr,"	相位 %d\n",class6002.searchResult[i].phase);
		fprintf(stderr,"	信号品质 %d\n",class6002.searchResult[i].signal);
		printDataTimeS("	搜到时间",class6002.searchResult[i].searchTime);
		fprintf(stderr,"	搜到附加信息 %d\n",class6002.searchResult[i].annexNum);
		for(j=0;j<class6002.searchResult[i].annexNum;j++) {
			fprintf(stderr,"	对象描述 %04x-%02x%02x\n",class6002.searchResult[i].annexInfo[j].oad.OI,
					class6002.searchResult[i].annexInfo[j].oad.attflg,class6002.searchResult[i].annexInfo[j].oad.attrindex);
			fprintf(stderr,"	属性值 类型[%d]\n",class6002.searchResult[i].annexInfo[j].data.type);
		}
	}
	fprintf(stderr,"属性5(跨台区搜表结果)\n");
	for(i=0;i<class6002.crosszoneNum;i++) {
		fprintf(stderr,"	通信地址");
		printTSA(class6002.crosszoneResult[i].CommAddr);
		fprintf(stderr,"	所属采集器地址");
		printTSA(class6002.crosszoneResult[i].mainPointAddr);
		printDataTimeS("	变更时间",class6002.crosszoneResult[i].changeTime);
	}
}

//搜表方案
void Search6002(int argc, char *argv[])
{
	CLASS_6002	class6002={};

	if(strcmp("pro",argv[2])==0) {
		if(argc<5) {
			if(readCoverClass(0x6002,0,&class6002,sizeof(CLASS_6002),para_vari_save)==1) {
				print6002(class6002);
			}else {
				fprintf(stderr,"搜表参数文件不存在\n");
			}
		}
	}
	if(strcmp("enable",argv[2])==0) {
		if(argc==5) {
			if(readCoverClass(0x6002,0,&class6002,sizeof(CLASS_6002),para_vari_save)==1) {

//				print6002(class6002);
			}else {
				fprintf(stderr,"搜表参数文件不存在\n");
			}
		}
	}
}

void print6013(CLASS_6013 class6013)
{
	INT8U	i=0;
	fprintf(stderr,"\n\n[1]执行频率 [2]方案类型 [3]方案编号 [4]开始时间 [5]结束时间 [6]延时 [7]执行优先级 [8]状态 [9]开始前脚本id [10]开始后脚本id [11]运行时段【起始HH:MM 结束HH:MM】\n");
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
		        setOIChange_CJ(0x6013);
			}
		}
	}
}

void print6015(CLASS_6015 class6015)
{
	INT8U type=0,w=0,i=0;

	fprintf(stderr,"\n\n[1]方案编号 [2]存储深度 [3]采集类型 [4]采集内容 [5]OAD-ROAD [6]MS [7]存储时标\n");
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
		fprintf(stderr,"[4]%s-%d ",getenum(task_ti,class6015.data.data[0]),((class6015.data.data[1]<<8)|class6015.data.data[2]));
		break;
	case 4://RetryMetering
		fprintf(stderr,"[4]%s-%d %d\n",getenum(task_ti,class6015.data.data[0]),((class6015.data.data[1]<<8)|class6015.data.data[2]),
									((class6015.data.data[3]<<8)|class6015.data.data[4]));
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
	printMS(class6015.mst);
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
	}
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
	if(strcmp("add",argv[2])==0) {

	}
}

void print_6017(CLASS_6017 eventFangAn)
{
	INT8U j=0;

	fprintf(stderr,"\n\n事件采集方案：[1]方案编号 采集方式{[2] 采集类型 [3]采集事件数据ROAD} [4]电能表集合MS [5]上报标识 [6]存储深度");

	fprintf(stderr,"\n[1]方案编号 ：%d ",eventFangAn.sernum);
	fprintf(stderr,"\n[2]采集类型 ：%d ",eventFangAn.collstyle.colltype);
	fprintf(stderr,"\n[3]ROAD{%d}\n",eventFangAn.collstyle.roads.num);
	for(j=0;j<eventFangAn.collstyle.roads.num;j++)
	{
		print_road(eventFangAn.collstyle.roads.road[j]);
	}
	fprintf(stderr,"[4]");
	printMS(eventFangAn.ms);
	fprintf(stderr,"\n[5]%d  ",eventFangAn.ifreport);
	fprintf(stderr,"[6]%d\n",eventFangAn.deepsize);
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
				sscanf(argv[4],"%d",&tmp[0]);taskid = tmp[0];
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

void print6019(CLASS_6019 TransFangAn)
{
	int  i=0,j=0;
	fprintf(stderr,"\n\n透明方案：[1]方案编号 [2]采集内容集{[2.1序号 [2.1]通信地址 [2.3]开始前脚本id [2.4]开始后脚本id [2.5]方案控制标志}\n");
	fprintf(stderr,"[2.5.1]等待后续报文 [2.5.2]等待报文超时时间（秒） [2.5.3]结果比对标识 [2.5.5]结果比对参数\n");
	fprintf(stderr,"[2.5.5.1]特征字节 [2.5.5.2]截取开始 [2.5.5.3]截取长度\n");
	fprintf(stderr,"[2.6]方案报文集 [2.6.1]报文序号 [2.6.2]报文内容\n");
	fprintf(stderr,"[3]存储深度\n");
	fprintf(stderr,"[1]%d [2]%d",TransFangAn.planno,TransFangAn.contentnum);
	for(i=0;i<TransFangAn.contentnum;i++) {
		fprintf(stderr,"[2.1]%d [2.2]",TransFangAn.plan[i].seqno);
		printTSA(TransFangAn.plan[i].addr);
		fprintf(stderr,"[2.3]%d [2.4]%d\n",TransFangAn.plan[i].befscript,TransFangAn.plan[i].aftscript);
		fprintf(stderr,"[2.5.1]%d [2.5.2]%d [2.5.3]%d\n",TransFangAn.plan[i].planflag.waitnext,TransFangAn.plan[i].planflag.overtime,TransFangAn.plan[i].planflag.resultflag);
		fprintf(stderr,"[2.5.5.1]%d [2.5.5.2]%d [2.5.5.3]%d\n",TransFangAn.plan[i].planflag.resultpara.featureByte,TransFangAn.plan[i].planflag.resultpara.interstart,TransFangAn.plan[i].planflag.resultpara.interlen);
		fprintf(stderr,"[2.6]%d ",TransFangAn.plan[i].datanum);
		for(j=0;j<TransFangAn.plan[i].datanum;j++) {
			fprintf(stderr,"[2.6.1]%d ",TransFangAn.plan[i].data[j].datano);
		}
	}
}

void Trans6019(int argc, char *argv[])
{
	INT8U	i=0;
	INT8U	planno = 0;
	int		tmp[2]={};
	CLASS_6019 TransFangAn={};

	if(strcmp("pro",argv[2])==0) {
		if(argc<5) {
//				fprintf(stderr,"[1]方案编号 [2]存储深度 [3]采集类型 [4]采集内容 [5]OAD-ROAD [6]MS [7]存储时标\n");
			for(i=0;i<=255;i++) {
				planno = i;
				memset(&TransFangAn,0,sizeof(CLASS_6019));
				if(readCoverClass(0x6019,planno,&TransFangAn,sizeof(CLASS_6019),coll_para_save)== 1) {
					print6019(TransFangAn);
				}else {
//						fprintf(stderr,"任务ID=%d 无任务配置单元",taskid);
				}
			}
		}else if(argc==5) {
			sscanf(argv[4],"%04x",&tmp[0]);
			planno = tmp[0];
			fprintf(stderr,"planno=%d\n",planno);
			memset(&TransFangAn,0,sizeof(CLASS_6019));
			if(readCoverClass(0x6019,planno,&TransFangAn,sizeof(CLASS_6019),coll_para_save)==1) {
				print6019(TransFangAn);
			}else {
				fprintf(stderr,"无任务配置单元");
			}
		}
	}
}

void print_601d(CLASS_601D	 reportplan)
{
	int j=0;
	fprintf(stderr,"\n\n[1]上报方案编号 [2]上报通道 [3]上报响应超时时间 [4]最大上报次数 [5]上报内容 {[5.1]类型(0:OAD,1:RecordData) [5.2]数据 [5.3]RSD}");
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
	fprintf(stderr,"\n\n[1]任务ID [2]执行状态<0:未执行 1:执行中 2:已执行> [3]任务执行开始时间 [4]任务执行结束时间 [5]采集总数量 [6]采集成功数量 [7]已发送报文条数 [8]已接收报文条数 \n");
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

void Task6099(int argc, char *argv[])
{
	taskFailInfo_s tfs = {};
	int i=0;
	int taskNum = 0;

	if(argc == 4) {
		if(strcmp("pro",argv[2])==0) {
			if(readCoverClass(0x6099, 0, &tfs, sizeof(taskFailInfo_s), para_vari_save) == 1) {
				taskNum = sizeof(taskFailInfo_s)/sizeof(rptInfo_s);
				taskNum /= 2;
				DEBUG_TIME_LINE("taskNum: %d", taskNum);
				for(i=0;i<taskNum;i++) {
					fprintf(stderr, "\ntaskID<%02d>:\t%04d-%02d-%02d %02d-%02d-%02d,\t%04d-%02d-%02d %02d-%02d-%02d",
							tfs.rptList[i][0].taskId,
							tfs.rptList[i][0].startTime.Year, tfs.rptList[i][0].startTime.Month, tfs.rptList[i][0].startTime.Day,
							tfs.rptList[i][0].startTime.Hour, tfs.rptList[i][0].startTime.Minute, tfs.rptList[i][0].startTime.Sec,
							tfs.rptList[i][1].startTime.Year, tfs.rptList[i][1].startTime.Month, tfs.rptList[i][1].startTime.Day,
							tfs.rptList[i][1].startTime.Hour, tfs.rptList[i][1].startTime.Minute, tfs.rptList[i][1].startTime.Sec);
				}
				fprintf(stderr, "\n");
			} else {
				DEBUG_TIME_LINE("6099文件打开失败");
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
			case 0x6002:
				Search6002(argc,argv);
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
			case 0x6019:
				Trans6019(argc,argv);
				break;
			case 0x601d:
				Report601d(argc,argv);
				break;
			case 0x6035:
				Task6035(argc,argv);
				break;
			case 0x6099:
				Task6099(argc,argv);
				break;
			default:
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


int findFangAnIndex(int code)
{
	int i=0;
	for(i=0;i<20;i++)
	{
		if (fangAn6015[i].sernum == code)
		{
			return i;
		}
	}
	return -1;
}
/*
 *根据6015中的MS 和电表地址 和端口好判断此电表是否需要抄读
 *0-不抄 1-抄
 */
INT8U checkMeterType(MY_MS mst,INT8U usrType,TSA usrAddr)
{

	INT16U ms_num=0;
	INT16U collIndex = 0;

	switch(mst.mstype)
	{
		case 0:
			return 0;
			break;
		case 1:
			return 1;
			break;
		case 2:
			ms_num = (mst.ms.userType[0]<<8) | mst.ms.userType[1];
			for(collIndex=0;collIndex < ms_num;collIndex++)
			{
				if(mst.ms.userType[collIndex+2] == usrType)
				{
					return 1;
				}
			}
			break;
		case 3:
			ms_num = (mst.ms.userAddr[0].addr[0]<<8)|mst.ms.userAddr[0].addr[1];
			if(ms_num > COLLCLASS_MAXNUM) fprintf(stderr,"配置序号 超过限值 %d ,error !!!!!!",COLLCLASS_MAXNUM);
			for(collIndex = 0;collIndex < ms_num;collIndex++)
			{
				if(memcmp(&mst.ms.userAddr[collIndex+1],&usrAddr,sizeof(TSA)) == 0)
				{
					return 1;
				}
			}
			break;
		case 5:
			for(collIndex = 0;collIndex < COLLCLASS_MAXNUM;collIndex++)
			{
				if(mst.ms.type[collIndex].type != interface)
				{
					INT16S typeBegin = -1;
					INT16S typeEnd = -1;
					if(mst.ms.type[collIndex].begin[0] == dtunsigned)
					{
						typeBegin = mst.ms.type[collIndex].begin[1];
					}
					if(mst.ms.type[collIndex].end[0] == dtunsigned)
					{
						typeEnd = mst.ms.type[collIndex].end[1];
					}
					if((usrType > typeBegin)&&(usrType < typeEnd))
					{
						return 1;
					}
					else if(mst.ms.type[collIndex].type == close_open)
					{
						if(usrType == typeBegin)
						{
							return 1;
						}
					}
					else if(mst.ms.type[collIndex].type == open_close)
					{
						if(usrType == typeEnd)
						{
							return 1;
						}
					}
					else if(mst.ms.type[collIndex].type == close_close)
					{
						if((usrType == typeBegin)||(usrType == typeEnd))
						{
							return 1;
						}
					}

				}
			}

			break;
		default :
				return 0;

	}

	return 0;
}
int CheckType(INT8U usrtype,INT8U fanganno,TSA tsa)
{
	int fangAnIndex = 0,needflg=0;
	fangAnIndex = findFangAnIndex(fanganno);//查被抄电表当前任务的采集方案编号，在6015中的索引
	if (fangAnIndex >=0 )
	{
		needflg = checkMeterType(fangAn6015[fangAnIndex].mst, usrtype ,tsa);//查被抄电表的用户类型 是否满足6015中的用户类型条件
	}
	if (needflg == 1)
	{
		return 1;
	}else
	{
		return 0;
	}
}
void PrintTaskInfo3(TASK_INFO *task,INT8U usrtype)
{
	time_t nowt = time(NULL);
	int i=0,j=0,numindex=0,flag=0;
	for(i=0;i<task->task_n;i++)
	{
		flag = CheckType(usrtype,task->task_list[i].fangan.No,task->tsa);
		if (flag==1){
			fprintf(stderr,"\n当前时间( %ld )   本任务下次执行时间  ( %ld )",nowt, task->task_list[i].beginTime  );
			for(j=0;j<task->task_list[i].fangan.item_n;j++)
			{
				numindex++;
				switch(task->task_list[i].fangan.cjtype)
				{
				case 0:
					fprintf(stderr,"\n%02d| %04x-%02x%02x - %04x-%02x%02x .%02d任务 执行频率%d[%d] %d级 方案%d ，类型%d  采集当前数据 | %d-%d-%d %d:%d:%d OK=%d cov %02x%02x%02x%02x",
							numindex,
							task->task_list[i].fangan.items[j].oad1.OI,task->task_list[i].fangan.items[j].oad1.attflg,task->task_list[i].fangan.items[j].oad1.attrindex,
							task->task_list[i].fangan.items[j].oad2.OI,task->task_list[i].fangan.items[j].oad2.attflg,task->task_list[i].fangan.items[j].oad2.attrindex,
							task->task_list[i].taskId,
							task->task_list[i].ti.interval,task->task_list[i].ti.units,
							task->task_list[i].leve,
							task->task_list[i].fangan.No,
							task->task_list[i].fangan.type,
							task->task_list[i].begin.year.data,task->task_list[i].begin.month.data,task->task_list[i].begin.day.data,
							task->task_list[i].begin.hour.data,task->task_list[i].begin.min.data,task->task_list[i].begin.sec.data,
							task->task_list[i].fangan.items[j].sucessflg,
							task->task_list[i].fangan.items[j].item07[3],task->task_list[i].fangan.items[j].item07[2],
							task->task_list[i].fangan.items[j].item07[1],task->task_list[i].fangan.items[j].item07[0]
							);
					break;
				case 1:
					fprintf(stderr,"\n%02d| %04x-%02x%02x - %04x-%02x%02x .%02d任务 执行频率%d[%d] %d级 方案%d ，类型%d  采集上%d次 | %d-%d-%d %d:%d:%d OK=%d cov %02x%02x%02x%02x",
							numindex,
							task->task_list[i].fangan.items[j].oad1.OI,task->task_list[i].fangan.items[j].oad1.attflg,task->task_list[i].fangan.items[j].oad1.attrindex,
							task->task_list[i].fangan.items[j].oad2.OI,task->task_list[i].fangan.items[j].oad2.attflg,task->task_list[i].fangan.items[j].oad2.attrindex,
							task->task_list[i].taskId,
							task->task_list[i].ti.interval,task->task_list[i].ti.units,
							task->task_list[i].leve,
							task->task_list[i].fangan.No,task->task_list[i].fangan.type,
							task->task_list[i].fangan.N,
							task->task_list[i].begin.year.data,task->task_list[i].begin.month.data,task->task_list[i].begin.day.data,
							task->task_list[i].begin.hour.data,task->task_list[i].begin.min.data,task->task_list[i].begin.sec.data,
							task->task_list[i].fangan.items[j].sucessflg,
							task->task_list[i].fangan.items[j].item07[3],task->task_list[i].fangan.items[j].item07[2],
							task->task_list[i].fangan.items[j].item07[1],task->task_list[i].fangan.items[j].item07[0]
							);
					break;
				case 2:
					fprintf(stderr,"\n%02d| %04x-%02x%02x - %04x-%02x%02x .%02d任务 执行频率%d[%d] %d级 方案%d ，类型%d  按冻结时标采集 | %d-%d-%d %d:%d:%d OK=%d cov %02x%02x%02x%02x",
							numindex,
							task->task_list[i].fangan.items[j].oad1.OI,task->task_list[i].fangan.items[j].oad1.attflg,task->task_list[i].fangan.items[j].oad1.attrindex,
							task->task_list[i].fangan.items[j].oad2.OI,task->task_list[i].fangan.items[j].oad2.attflg,task->task_list[i].fangan.items[j].oad2.attrindex,
							task->task_list[i].taskId,
							task->task_list[i].ti.interval,task->task_list[i].ti.units,
							task->task_list[i].leve,
							task->task_list[i].fangan.No,task->task_list[i].fangan.type,
							task->task_list[i].begin.year.data,task->task_list[i].begin.month.data,task->task_list[i].begin.day.data,
							task->task_list[i].begin.hour.data,task->task_list[i].begin.min.data,task->task_list[i].begin.sec.data,
							task->task_list[i].fangan.items[j].sucessflg,
							task->task_list[i].fangan.items[j].item07[3],task->task_list[i].fangan.items[j].item07[2],
							task->task_list[i].fangan.items[j].item07[1],task->task_list[i].fangan.items[j].item07[0]
							);
					break;
				case 3:
					fprintf(stderr,"\n%02d| %04x-%02x%02x - %04x-%02x%02x .%02d任务 执行频率%d[%d] %d级 方案%d ，类型%d  间隔%d (%d) | %d-%d-%d %d:%d:%d OK=%d cov %02x%02x%02x%02x",
							numindex,
							task->task_list[i].fangan.items[j].oad1.OI,task->task_list[i].fangan.items[j].oad1.attflg,task->task_list[i].fangan.items[j].oad1.attrindex,
							task->task_list[i].fangan.items[j].oad2.OI,task->task_list[i].fangan.items[j].oad2.attflg,task->task_list[i].fangan.items[j].oad2.attrindex,
							task->task_list[i].taskId,
							task->task_list[i].ti.interval,task->task_list[i].ti.units,
							task->task_list[i].leve,
							task->task_list[i].fangan.No,task->task_list[i].fangan.type,
							task->task_list[i].fangan.ti.interval,
							task->task_list[i].fangan.ti.units,
							task->task_list[i].begin.year.data,task->task_list[i].begin.month.data,task->task_list[i].begin.day.data,
							task->task_list[i].begin.hour.data,task->task_list[i].begin.min.data,task->task_list[i].begin.sec.data,
							task->task_list[i].fangan.items[j].sucessflg,
							task->task_list[i].fangan.items[j].item07[3],task->task_list[i].fangan.items[j].item07[2],
							task->task_list[i].fangan.items[j].item07[1],task->task_list[i].fangan.items[j].item07[0]
							);
					break;
				default:
					fprintf(stderr,"\n%02d| %04x-%02x%02x - %04x-%02x%02x .%02d任务 执行频率%d[%d] %d级 方案%d ，类型%d 未知采集类型 | %d-%d-%d %d:%d:%d OK=%d cov %02x%02x%02x%02x",
							numindex,
							task->task_list[i].fangan.items[j].oad1.OI,task->task_list[i].fangan.items[j].oad1.attflg,task->task_list[i].fangan.items[j].oad1.attrindex,
							task->task_list[i].fangan.items[j].oad2.OI,task->task_list[i].fangan.items[j].oad2.attflg,task->task_list[i].fangan.items[j].oad2.attrindex,
							task->task_list[i].taskId,
							task->task_list[i].ti.interval,task->task_list[i].ti.units,
							task->task_list[i].leve,
							task->task_list[i].fangan.No,task->task_list[i].fangan.type,
							task->task_list[i].begin.year.data,task->task_list[i].begin.month.data,task->task_list[i].begin.day.data,
							task->task_list[i].begin.hour.data,task->task_list[i].begin.min.data,task->task_list[i].begin.sec.data,
							task->task_list[i].fangan.items[j].sucessflg,
							task->task_list[i].fangan.items[j].item07[3],task->task_list[i].fangan.items[j].item07[2],
							task->task_list[i].fangan.items[j].item07[1],task->task_list[i].fangan.items[j].item07[0]
							);
				}
			}
		}
	}
}
void PrintTaskInfo2(TASK_INFO *task)
{
	time_t nowt = time(NULL);
	int i=0,j=0,numindex=0;
	for(i=0;i<task->task_n;i++)
	{
		fprintf(stderr,"\n当前时间( %ld )   本任务下次执行时间  ( %ld )",nowt, task->task_list[i].beginTime  );
		for(j=0;j<task->task_list[i].fangan.item_n;j++)
		{
			numindex++;
			switch(task->task_list[i].fangan.cjtype)
			{
			case 0:
				fprintf(stderr,"\n%02d| %04x-%02x%02x - %04x-%02x%02x .%02d任务 执行频率%d[%d] %d级 方案%d ，类型%d  采集当前数据 | %d-%d-%d %d:%d:%d OK=%d cov %02x%02x%02x%02x",
						numindex,
						task->task_list[i].fangan.items[j].oad1.OI,task->task_list[i].fangan.items[j].oad1.attflg,task->task_list[i].fangan.items[j].oad1.attrindex,
						task->task_list[i].fangan.items[j].oad2.OI,task->task_list[i].fangan.items[j].oad2.attflg,task->task_list[i].fangan.items[j].oad2.attrindex,
						task->task_list[i].taskId,
						task->task_list[i].ti.interval,task->task_list[i].ti.units,
						task->task_list[i].leve,
						task->task_list[i].fangan.No,
						task->task_list[i].fangan.type,
						task->task_list[i].begin.year.data,task->task_list[i].begin.month.data,task->task_list[i].begin.day.data,
						task->task_list[i].begin.hour.data,task->task_list[i].begin.min.data,task->task_list[i].begin.sec.data,
						task->task_list[i].fangan.items[j].sucessflg,
						task->task_list[i].fangan.items[j].item07[3],task->task_list[i].fangan.items[j].item07[2],
						task->task_list[i].fangan.items[j].item07[1],task->task_list[i].fangan.items[j].item07[0]
						);
				break;
			case 1:
				fprintf(stderr,"\n%02d| %04x-%02x%02x - %04x-%02x%02x .%02d任务 执行频率%d[%d] %d级 方案%d ，类型%d  采集上%d次 | %d-%d-%d %d:%d:%d OK=%d cov %02x%02x%02x%02x",
						numindex,
						task->task_list[i].fangan.items[j].oad1.OI,task->task_list[i].fangan.items[j].oad1.attflg,task->task_list[i].fangan.items[j].oad1.attrindex,
						task->task_list[i].fangan.items[j].oad2.OI,task->task_list[i].fangan.items[j].oad2.attflg,task->task_list[i].fangan.items[j].oad2.attrindex,
						task->task_list[i].taskId,
						task->task_list[i].ti.interval,task->task_list[i].ti.units,
						task->task_list[i].leve,
						task->task_list[i].fangan.No,task->task_list[i].fangan.type,
						task->task_list[i].fangan.N,
						task->task_list[i].begin.year.data,task->task_list[i].begin.month.data,task->task_list[i].begin.day.data,
						task->task_list[i].begin.hour.data,task->task_list[i].begin.min.data,task->task_list[i].begin.sec.data,
						task->task_list[i].fangan.items[j].sucessflg,
						task->task_list[i].fangan.items[j].item07[3],task->task_list[i].fangan.items[j].item07[2],
						task->task_list[i].fangan.items[j].item07[1],task->task_list[i].fangan.items[j].item07[0]
						);
				break;
			case 2:
				fprintf(stderr,"\n%02d| %04x-%02x%02x - %04x-%02x%02x .%02d任务 执行频率%d[%d] %d级 方案%d ，类型%d  按冻结时标采集 | %d-%d-%d %d:%d:%d OK=%d cov %02x%02x%02x%02x",
						numindex,
						task->task_list[i].fangan.items[j].oad1.OI,task->task_list[i].fangan.items[j].oad1.attflg,task->task_list[i].fangan.items[j].oad1.attrindex,
						task->task_list[i].fangan.items[j].oad2.OI,task->task_list[i].fangan.items[j].oad2.attflg,task->task_list[i].fangan.items[j].oad2.attrindex,
						task->task_list[i].taskId,
						task->task_list[i].ti.interval,task->task_list[i].ti.units,
						task->task_list[i].leve,
						task->task_list[i].fangan.No,task->task_list[i].fangan.type,
						task->task_list[i].begin.year.data,task->task_list[i].begin.month.data,task->task_list[i].begin.day.data,
						task->task_list[i].begin.hour.data,task->task_list[i].begin.min.data,task->task_list[i].begin.sec.data,
						task->task_list[i].fangan.items[j].sucessflg,
						task->task_list[i].fangan.items[j].item07[3],task->task_list[i].fangan.items[j].item07[2],
						task->task_list[i].fangan.items[j].item07[1],task->task_list[i].fangan.items[j].item07[0]
						);
				break;
			case 3:
				fprintf(stderr,"\n%02d| %04x-%02x%02x - %04x-%02x%02x .%02d任务 执行频率%d[%d] %d级 方案%d ，类型%d  间隔%d (%d) | %d-%d-%d %d:%d:%d OK=%d cov %02x%02x%02x%02x",
						numindex,
						task->task_list[i].fangan.items[j].oad1.OI,task->task_list[i].fangan.items[j].oad1.attflg,task->task_list[i].fangan.items[j].oad1.attrindex,
						task->task_list[i].fangan.items[j].oad2.OI,task->task_list[i].fangan.items[j].oad2.attflg,task->task_list[i].fangan.items[j].oad2.attrindex,
						task->task_list[i].taskId,
						task->task_list[i].ti.interval,task->task_list[i].ti.units,
						task->task_list[i].leve,
						task->task_list[i].fangan.No,task->task_list[i].fangan.type,
						task->task_list[i].fangan.ti.interval,
						task->task_list[i].fangan.ti.units,
						task->task_list[i].begin.year.data,task->task_list[i].begin.month.data,task->task_list[i].begin.day.data,
						task->task_list[i].begin.hour.data,task->task_list[i].begin.min.data,task->task_list[i].begin.sec.data,
						task->task_list[i].fangan.items[j].sucessflg,
						task->task_list[i].fangan.items[j].item07[3],task->task_list[i].fangan.items[j].item07[2],
						task->task_list[i].fangan.items[j].item07[1],task->task_list[i].fangan.items[j].item07[0]
						);
				break;
			default:
				fprintf(stderr,"\n%02d| %04x-%02x%02x - %04x-%02x%02x .%02d任务 执行频率%d[%d] %d级 方案%d ，类型%d 未知采集类型 | %d-%d-%d %d:%d:%d OK=%d cov %02x%02x%02x%02x",
						numindex,
						task->task_list[i].fangan.items[j].oad1.OI,task->task_list[i].fangan.items[j].oad1.attflg,task->task_list[i].fangan.items[j].oad1.attrindex,
						task->task_list[i].fangan.items[j].oad2.OI,task->task_list[i].fangan.items[j].oad2.attflg,task->task_list[i].fangan.items[j].oad2.attrindex,
						task->task_list[i].taskId,
						task->task_list[i].ti.interval,task->task_list[i].ti.units,
						task->task_list[i].leve,
						task->task_list[i].fangan.No,task->task_list[i].fangan.type,
						task->task_list[i].begin.year.data,task->task_list[i].begin.month.data,task->task_list[i].begin.day.data,
						task->task_list[i].begin.hour.data,task->task_list[i].begin.min.data,task->task_list[i].begin.sec.data,
						task->task_list[i].fangan.items[j].sucessflg,
						task->task_list[i].fangan.items[j].item07[3],task->task_list[i].fangan.items[j].item07[2],
						task->task_list[i].fangan.items[j].item07[1],task->task_list[i].fangan.items[j].item07[0]
						);
			}
		}
	}
}
/*初始化 全部 普通采集方案6015数组  （CLASS_6015 task6015[20] 为请求抄读时提供参数支持）*/
void task_init6015(CLASS_6015 *fangAn6015p)
{
	int i=0,j=0,taskid;
	for(i=0;i<256;i++)
	{
		taskid = i;
		readCoverClass(0x6015, taskid, (void *)&fangAn6015p[j], sizeof(CLASS_6015), coll_para_save);
		j++;
		if(j>=20)
			break;
	}
}
void showPlcMeterstatus(int argc, char *argv[])
{
	TASK_INFO taskinfo;
	CLASS_6001	 meter={};
	int i=0,ret=0;
	int record_num = getFileRecordNum(0x6000);
	TSA tsa;
	int addrtmp[8];
	memset(tsa.addr,0,sizeof(tsa.addr));
	memset(fangAn6015,0,sizeof(fangAn6015));
	task_init6015(fangAn6015);
	if (argc==3)
	{
		sscanf(argv[2],"%02x%02x%02x%02x%02x%02x%02x%02x",&addrtmp[0],&addrtmp[1],&addrtmp[2],&addrtmp[3],&addrtmp[4],&addrtmp[5],&addrtmp[6],&addrtmp[7]);
		tsa.addr[0] = addrtmp[0];
		tsa.addr[1] = addrtmp[1];
		tsa.addr[2] = addrtmp[2];
		tsa.addr[3] = addrtmp[3];
		tsa.addr[4] = addrtmp[4];
		tsa.addr[5] = addrtmp[5];
		tsa.addr[6] = addrtmp[6];
		tsa.addr[7] = addrtmp[7];
		for(i=0;i<record_num;i++)
		{
			if(readParaClass(0x6000,&meter,i)==1)
			{
				if (meter.sernum!=0 && meter.sernum!=0xffff && meter.basicinfo.port.OI==0xf209 && memcmp(tsa.addr,meter.basicinfo.addr.addr,TSA_LEN)==0)
				{
					fprintf(stderr,"\n电表序号 %d   ,规约 %d    ,用户类型 %d [%02x H]",meter.sernum,meter.basicinfo.protocol,meter.basicinfo.usrtype,meter.basicinfo.usrtype);
					ret = readParaClass(0x8888, &taskinfo, meter.sernum);
					if (ret == 1 )
					{
						PrintTaskInfo2(&taskinfo);
					}else
					{
						fprintf(stderr,"\n读失败");
					}
				}
			}
		}
	}else
	{
		int counter = 1;
		for(i=0;i<record_num;i++)
		{
			if(readParaClass(0x6000,&meter,i)==1)
			{
				if (meter.sernum!=0 && meter.sernum!=0xffff && meter.basicinfo.port.OI==0xf209)
				{
					ret = readParaClass(0x8888, &taskinfo, meter.sernum);
					if (ret == 1 )
					{
						fprintf(stderr,"\n【 %d 】TSA: %02x%02x%02x%02x%02x%02x%02x%02x",counter++,
								taskinfo.tsa.addr[0],taskinfo.tsa.addr[1],taskinfo.tsa.addr[2],taskinfo.tsa.addr[3],
								taskinfo.tsa.addr[4],taskinfo.tsa.addr[5],taskinfo.tsa.addr[6],taskinfo.tsa.addr[7]);
//						PrintTaskInfo2(&taskinfo);
						PrintTaskInfo3(&taskinfo,meter.basicinfo.usrtype);
						fprintf(stderr,"\n");
					}
				}
			}
		}
	}
	return;
}
