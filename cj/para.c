/*
 * para.c
 *
 *  Created on: Jan 5, 2017
 *      Author: admin
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "AccessFun.h"
#include "StdDataType.h"
#include "Objectdef.h"
#include "main.h"
#include "version.h"
#include "Shmem.h"
#include "PublicFunction.h"

extern ProgramInfo *JProgramInfo;
//typedef struct
//{
//	char name[OCTET_STRING_LEN];		//逻辑名
//	char devdesc[VISIBLE_STRING_LEN];		//设备描述符
//	VERINFO info;						//版本信息
//	DateTimeBCD date_Product;			//生产日期
//	OI_698 ois[10];						//子设备列表
//	char  protcol[OCTET_STRING_LEN];	//支持的规约列表
//	INT8U follow_report;				//是否允许跟随上报
//	INT8U active_report;				//是否允许主动上报
//	INT8U talk_master;					//是否允许与主站通话
//} CLASS19;					//设备管理接口类

void printstring(char *pro,INT8U *str)
{
	fprintf(stderr,"\n[%s]:长度(%d)------------%s",pro,str[0],&str[1]);
}

void printoctetstr(char *pro,INT8U *str)
{
	int	i=0;
	fprintf(stderr,"\n[%s](%d):",pro,str[0]);
	for(i=0;i<str[0];i++) {
		fprintf(stderr,"%d ",str[i+1]);
	}
}

void print4000()
{
	CLASS_4000 oi4000={};

	memset(&oi4000,0,sizeof(CLASS_4000));
	fprintf(stderr,"校时参数[4000]\n");
	readCoverClass(0x4000,0,&oi4000,sizeof(CLASS_4000),para_vari_save);
	fprintf(stderr,"校时模式：%d  [主站授时（0），终端精确校时（1），北斗/GPS（2），其他（255）]\n",oi4000.type);
	fprintf(stderr,"日期时间-精准校时模式：\n");
	fprintf(stderr,"最近心跳时间总个数：%d\n",oi4000.hearbeatnum);
	fprintf(stderr,"最大值剔除个数：%d\n",oi4000.tichu_max);
	fprintf(stderr,"最小值剔除个数：%d\n",oi4000.tichu_min);
	fprintf(stderr,"通讯延时阀值：%d\n",oi4000.delay);
	fprintf(stderr,"最小有效个数：%d\n",oi4000.num_min);
}

void print4030()
{
	CLASS_4030 oi4030={};

	memset(&oi4030,0,sizeof(CLASS_4030));
	fprintf(stderr,"电压合格率参数[4030]\n");
	readCoverClass(0x4030,0,&oi4030,sizeof(CLASS_4030),para_vari_save);
	fprintf(stderr,"电压考核上限：%d\n",oi4030.uUp_Kaohe);
	fprintf(stderr,"电压考核下限：%d\n",oi4030.uDown_Kaohe);
	fprintf(stderr,"电压合格上限：%d\n",oi4030.uUp);
	fprintf(stderr,"电压合格下限：%d\n",oi4030.uDown);
}

void print4204()
{
	CLASS_4204 oi4204={};

	memset(&oi4204,0,sizeof(CLASS_4204));
	fprintf(stderr,"广播校时参数[4204]\n");
	readCoverClass(0x4204,0,&oi4204,sizeof(CLASS_4204),para_vari_save);
	fprintf(stderr,"终端广播校时开始时间：%d:%d:%d\n",oi4204.startime[0],oi4204.startime[1],oi4204.startime[2]);
	fprintf(stderr,"是否启用：%d\n",oi4204.enable);

	fprintf(stderr,"单地址终端广播校时开始时间：%d:%d:%d\n",oi4204.startime1[0],oi4204.startime1[1],oi4204.startime1[2]);
	fprintf(stderr,"时钟误差阀值：%d\n",oi4204.upleve);
	fprintf(stderr,"是否启用：%d\n",oi4204.enable1);
}
void print4300()
{
	CLASS19  oi4300={};
	fprintf(stderr,"设备接口类[4300]\n");
	readCoverClass(0x4300,0,&oi4300,sizeof(CLASS19),para_vari_save);
	fprintf(stderr,"\n1.逻辑名：%s",oi4300.name);
	fprintf(stderr,"\n2.设备描述符：%s",oi4300.devdesc);
	fprintf(stderr,"\n3.版本信息:");
	fprintf(stderr,"\n	厂商代码 %c%c%c%c",oi4300.info.factoryCode[0],oi4300.info.factoryCode[1],oi4300.info.factoryCode[2],oi4300.info.factoryCode[3]);
	fprintf(stderr,"\n	软件版本 %c%c%c%c",oi4300.info.softVer[0],oi4300.info.softVer[1],oi4300.info.softVer[2],oi4300.info.softVer[3]);
	fprintf(stderr,"\n	软件版本日期 %c%c%c%c%c%c",oi4300.info.softDate[0],oi4300.info.softDate[1],oi4300.info.softDate[2],oi4300.info.softDate[3],oi4300.info.softDate[4],oi4300.info.softDate[5]);
	fprintf(stderr,"\n	硬件版本 %c%c%c%c",oi4300.info.hardVer[0],oi4300.info.hardVer[1],oi4300.info.hardVer[2],oi4300.info.hardVer[3]);
	fprintf(stderr,"\n	硬件版本日期 %c%c%c%c%c%c",oi4300.info.hardDate[0],oi4300.info.hardDate[1],oi4300.info.hardDate[2],oi4300.info.hardDate[3],oi4300.info.hardDate[4],oi4300.info.hardDate[5]);
	fprintf(stderr,"\n	规约列表 %s\n",oi4300.protcol);
	fprintf(stderr,"\n4.生产日期 %d-%d-%d %d:%d:%d",oi4300.date_Product.year.data,oi4300.date_Product.month.data,oi4300.date_Product.day.data,oi4300.date_Product.hour.data,oi4300.date_Product.min.data,oi4300.date_Product.sec.data);
	fprintf(stderr,"\n5.子设备列表");
	fprintf(stderr,"\n6.支持的规约列表:%s",oi4300.protcol);
	fprintf(stderr,"\n7.是否允许跟随上报:%d",oi4300.follow_report);
	fprintf(stderr,"\n8.是否允许主动上报:%d",oi4300.active_report);
	fprintf(stderr,"\n9.是否允许与主站通话:%d",oi4300.talk_master);
	fprintf(stderr,"\n");
}

void print4500()
{
	CLASS25  oi4500={};
	int		i=0;

	fprintf(stderr,"***************无线公网通信接口类[4500]***************\n");
	readCoverClass(0x4500,0,&oi4500,sizeof(CLASS25),para_vari_save);
	fprintf(stderr,"\n---------------------------------------");
	fprintf(stderr,"\n属性2.通信配置");
	fprintf(stderr,"\n工作模式{混合模式（0），客户机模式（1），服务器模式（2）}------------%d",oi4500.commconfig.workModel);
	fprintf(stderr,"\n在线方式{永久在线（0），被动激活（1）}------------%d",oi4500.commconfig.onlineType);
	fprintf(stderr,"\n连接方式{TCP（0），UDP（1）}------------%d",oi4500.commconfig.connectType);
	fprintf(stderr,"\n连接应用方式{主备模式（0），多连接模式（1）}------------%d",oi4500.commconfig.appConnectType);
	fprintf(stderr,"\n侦听端口列表:个数[%d]",oi4500.commconfig.listenPortnum);
	for(i=0;i<oi4500.commconfig.listenPortnum;i++) {
		fprintf(stderr,"%d ",oi4500.commconfig.listenPort[i]);
	}
	printstring("APN",oi4500.commconfig.apn);
	printstring("用户名",oi4500.commconfig.userName);
	printstring("密码",oi4500.commconfig.passWord);
	printoctetstr("代理服务器地址:个数",&oi4500.commconfig.proxyIp[0]);

	fprintf(stderr,"\n代理端口------------[%d]",oi4500.commconfig.proxyPort);
	fprintf(stderr,"\n超时时间{秒}------------[%d] 重发次数------------[%d]",(oi4500.commconfig.timeoutRtry>>2)&0xff,oi4500.commconfig.timeoutRtry&0x03);
	fprintf(stderr,"\n心跳周期{秒}------------[%d]",oi4500.commconfig.heartBeat);
	fprintf(stderr,"\n---------------------------------------");
	fprintf(stderr,"\n属性3.主站通信参数表");
	fprintf(stderr,"\n主站通信参数(%d)",oi4500.master.masternum);
	for(i=0;i<oi4500.master.masternum;i++) {
		printoctetstr("		IP地址:长度",oi4500.master.master[i].ip);
		fprintf(stderr,"	端口:%d",oi4500.master.master[i].port);
	}
	fprintf(stderr,"\n");
	fprintf(stderr,"\n属性5.版本信息");
	fprintf(stderr,"\n	厂商代码 %c%c%c%c",oi4500.info.factoryCode[0],oi4500.info.factoryCode[1],oi4500.info.factoryCode[2],oi4500.info.factoryCode[3]);
	fprintf(stderr,"\n	软件版本 %c%c%c%c",oi4500.info.softVer[0],oi4500.info.softVer[1],oi4500.info.softVer[2],oi4500.info.softVer[3]);
	fprintf(stderr,"\n	软件版本日期 %c%c%c%c%c%c",oi4500.info.softDate[0],oi4500.info.softDate[1],oi4500.info.softDate[2],oi4500.info.softDate[3],oi4500.info.softDate[4],oi4500.info.softDate[5]);
	fprintf(stderr,"\n	硬件版本 %c%c%c%c",oi4500.info.hardVer[0],oi4500.info.hardVer[1],oi4500.info.hardVer[2],oi4500.info.hardVer[3]);
	fprintf(stderr,"\n	硬件版本日期 %c%c%c%c%c%c",oi4500.info.hardDate[0],oi4500.info.hardDate[1],oi4500.info.hardDate[2],oi4500.info.hardDate[3],oi4500.info.hardDate[4],oi4500.info.hardDate[5]);
	fprintf(stderr,"\n	厂家扩展信息 %c%c%c%c%c%c%c%c",oi4500.info.factoryExpInfo[0],oi4500.info.factoryExpInfo[1],oi4500.info.factoryExpInfo[2],
													oi4500.info.factoryExpInfo[3],oi4500.info.factoryExpInfo[4],oi4500.info.factoryExpInfo[5],
													oi4500.info.factoryExpInfo[6],oi4500.info.factoryExpInfo[7]);
	fprintf(stderr,"\n");

	fprintf(stderr,"\n属性6.支持规约列表");
	for(i=0;i<oi4500.protocolnum;i++) {
		fprintf(stderr,"\n[%d]%s",oi4500.protcol[i][0],&oi4500.protcol[i][1]);
		fprintf(stderr,"\n");
	}

	fprintf(stderr,"\n属性7.SIM卡的ICCID");
	fprintf(stderr,"\n(%d)%s",oi4500.ccid[0],&oi4500.ccid[1]);
	fprintf(stderr,"\n");

	fprintf(stderr,"\n");
	fprintf(stderr,"\n属性8.IMSI");
	fprintf(stderr,"\n(%d)%s",oi4500.imsi[0],&oi4500.imsi[1]);
	fprintf(stderr,"\n");

	fprintf(stderr,"\n");
	fprintf(stderr,"\n属性9.信号强度");
	fprintf(stderr,"\n%d",oi4500.signalStrength);
	fprintf(stderr,"\n");

	fprintf(stderr,"\n");
	fprintf(stderr,"\n属性10.SIM卡号码");
	fprintf(stderr,"\n(%d)%s",oi4500.simkard[0],&oi4500.simkard[1]);
	fprintf(stderr,"\n");

	fprintf(stderr,"\n");
	fprintf(stderr,"\n属性11.拨号IP");
	fprintf(stderr,"\n%d.%d.%d.%d",oi4500.pppip[1],oi4500.pppip[2],oi4500.pppip[3],oi4500.pppip[4]);
	fprintf(stderr,"\n");
}

void print4510()
{
	CLASS26  oi4510={};
	int		i=0;

	fprintf(stderr,"***************以太网通信接口类[4510]***************\n");
	readCoverClass(0x4510,0,&oi4510,sizeof(CLASS26),para_vari_save);
	fprintf(stderr,"\n---------------------------------------");
	fprintf(stderr,"\n属性2.通信配置");
	fprintf(stderr,"\n工作模式{混合模式（0），客户机模式（1），服务器模式（2）}------------%d",oi4510.commconfig.workModel);
	fprintf(stderr,"\n连接方式{TCP（0），UDP（1）}------------%d",oi4510.commconfig.connectType);
	fprintf(stderr,"\n连接应用方式{主备模式（0），多连接模式（1）}------------%d",oi4510.commconfig.appConnectType);
	fprintf(stderr,"\n侦听端口列表:个数[%d]",oi4510.commconfig.listenPortnum);
	for(i=0;i<oi4510.commconfig.listenPortnum;i++) {
		fprintf(stderr,"%d ",oi4510.commconfig.listenPort[i]);
	}
	printoctetstr("代理服务器地址:个数",&oi4510.commconfig.proxyIp[0]);
	fprintf(stderr,"\n代理端口------------[%d]",oi4510.commconfig.proxyPort);
	fprintf(stderr,"\n超时时间{秒}------------[%d] 重发次数------------[%d]",(oi4510.commconfig.timeoutRtry>>2)&0xff,oi4510.commconfig.timeoutRtry&0x03);
	fprintf(stderr,"\n心跳周期{秒}------------[%d]",oi4510.commconfig.heartBeat);
	fprintf(stderr,"\n---------------------------------------");
	fprintf(stderr,"\n属性3.主站通信参数表");
	fprintf(stderr,"\n主站通信参数(%d)",oi4510.master.masternum);
	for(i=0;i<oi4510.master.masternum;i++) {
		printoctetstr("		IP地址:长度",oi4510.master.master[i].ip);
		fprintf(stderr,"	端口:%d",oi4510.master.master[i].port);
	}
	fprintf(stderr,"\n");
}

void get_softver()
{
	CLASS19  oi4300={};
	readCoverClass(0x4300,0,&oi4300,sizeof(CLASS19),para_vari_save);
	fprintf(stderr,"softver:%c%c%c%c",oi4300.info.softVer[0],oi4300.info.softVer[1],oi4300.info.softVer[2],oi4300.info.softVer[3]);
	fprintf(stderr,"	softdate:%c%c%c%c%c%c",oi4300.info.softDate[0],oi4300.info.softDate[1],oi4300.info.softDate[2],oi4300.info.softDate[3],oi4300.info.softDate[4],oi4300.info.softDate[5]);
	fprintf(stderr,"\n");
}

void InIt_Process(int argc, char *argv[])
{
	int 	tmp=0;
	int 	method=0;
	JProgramInfo = OpenShMem("ProgramInfo", sizeof(ProgramInfo), NULL);
	if(argc==3) {	//para
		sscanf(argv[2],"%d",&tmp);
		method = tmp;
		switch(method) {
		case 3:		//数据区初始化
		case 5:		//事件初始化
		case 6:		//需量初始化
			memDataInit(1,JProgramInfo);		//共享内存中数据的初始化
			dataInit(method);
			break;
		case 4:		//恢复出厂参数
        	//清除总表计量电量
			memDataInit(0,JProgramInfo);		//共享内存中数据的初始化
        	clearEnergy();
			paraInit(0,NULL);
			JProgramInfo->oi_changed.ctrlinit = 0x55;
			break;
		}
	}
	shmm_unregister("ProgramInfo", sizeof(ProgramInfo));
}

void para_process(int argc, char *argv[])
{
	int 	tmp=0;
	OI_698	oi=0;
	int 	method=0;

	if(argc==5) {	//para
		if(strcmp(argv[1],"para")==0) {
			if(strcmp(argv[2],"method")==0) {
				sscanf(argv[3],"%04x",&tmp);
				oi = tmp;
				sscanf(argv[4],"%d",&tmp);
				method = tmp;
				switch(oi) {
				case 0x4300:
					switch(method) {
					case 3:		//数据区初始化
					case 5:		//事件初始化
					case 6:		//需量初始化
						dataInit(method);
						break;
					case 4:		//恢复出厂参数
						clearEnergy();
						paraInit(0,NULL);
						break;
					}
					break;
				}
			}
		}
	}
	if(argc==4) {	//para
		if(strcmp(argv[2],"pro")==0) {
			sscanf(argv[3],"%04x",&tmp);
			oi = tmp;
			switch(oi) {
			case 0x4030:
				print4030();
				break;
			case 0x4300:
				print4300();
				break;
			case 0x4000:
				print4000();
				break;
			case 0x4204:
				print4204();
				break;
			case 0x4500:
				print4500();
				break;
			case 0x4510:
				print4510();
				break;
			}
		}
	}
}

void showCheckPara()
{
	ConfigPara	cfg_para={};
	fprintf(stderr,"\n======================================================\n");
	fprintf(stderr,"===================详细通信参数=========================\n");
	print4500();
	print4510();
	fprintf(stderr,"\n======================================================\n");

	fprintf(stderr,"\n======================================================\n");
	fprintf(stderr,"===================基本运行参数=========================\n");
	ReadDeviceConfig(&cfg_para);
	fprintf(stderr,"\n终端类型:%d",cfg_para.device);
	fprintf(stderr,"\n设备地区:%s",cfg_para.zone);
	fprintf(stderr,"\n------------------------------------------------------\n");
	fprintf(stderr,"------------------------------------------------------\n");
	system("cj id");
	fprintf(stderr,"\n------------------------------------------------------\n");
	system("cj ip");
	fprintf(stderr,"------------------------------------------------------\n");
	system("cj apn");
	fprintf(stderr,"------------------------------------------------------\n");
	system("cj usr-pwd");
	fprintf(stderr,"------------------------------------------------------\n");
	system("cj online-mode");
	fprintf(stderr,"------------------------------------------------------\n");
	system("cj net-ip");
	fprintf(stderr,"------------------------------------------------------\n");
	system("cj heart");
	fprintf(stderr,"\n======================================================\n");
	fprintf(stderr,"\n======================================================\n");
	fprintf(stderr,"===================程序基本版本信息========================\n");
	print4300();
	fprintf(stderr,"\n------------------------------------------------------\n");
	fprintf(stderr,"GIT VERSION : %d\n",GL_VERSION);
	fprintf(stderr,"------------------------------------------------------\n");
	system("md5sum /nand/bin/cjmain");
	system("md5sum /nand/bin/cjdeal");
	system("md5sum /nand/bin/cjcomm");
	system("md5sum /nand/bin/cj");
	system("md5sum /nor/lib/lib376.2.so");
	system("md5sum /nor/lib/lib698.so");
	system("md5sum /nor/lib/lib698Esam.so");
	system("md5sum /nor/lib/libAccess.so");
	system("md5sum /nor/lib/libBase.so");
	system("md5sum /nor/lib/libDlt645.so");
	system("md5sum /nor/lib/libGui.so");
	system("md5sum /nor/lib/libMq.so");
	system("md5sum /nor/lib/libevent.so");
	system("md5sum /nor/lib/libzlib.so");
	system("md5sum /nor/config/07DI_698OAD.cfg");
	system("md5sum /nor/config/OI_TYPE.cfg");
	system("md5sum /nor/config/device.cfg");
	system("md5sum /nor/config/systema.cfg");
	system("md5sum /nor/bin/mux.sh");
	system("md5sum /nor/bin/gsmMuxd");
	fprintf(stderr,"------------------------------------------------------\n");
	fprintf(stderr,"\n======================================================\n");

}
