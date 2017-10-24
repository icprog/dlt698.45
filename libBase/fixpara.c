/*
 * fixpara.c
 *
 *  Created on: Feb 10, 2017
 *      Author: ava
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>
#include "sys/reboot.h"
#include <wait.h>
#include <errno.h>

#include "ParaDef.h"
#include "PublicFunction.h"
#include "AccessFun.h"
#include "Objectdef.h"
#include "basedef.h"

typedef struct {
    INT8U apn[VISIBLE_STRING_LEN];      // apn
    INT8U userName[VISIBLE_STRING_LEN]; //用户名称
    INT8U passWord[VISIBLE_STRING_LEN]; //密码
    INT8U proxyIp[OCTET_STRING_LEN];    //代理服务器地址
}GprsPara;

#define  IP_LEN		4			//参数ip类长度
									//厂商代码　　软件版本　软件日期　　硬件版本　硬件日期  扩展信息
static VERINFO verinfo          = { "QDGK", "V1.1", "171008", "1.10", "160328", "00000000" }; // 4300 版本信息
									//湖南需要双协议,软件版本要求为SXY8（双协议8） ，1376.1（软件版本为SXY1）
static VERINFO verinfo_HuNan    = { "QDGK", "SXY8", "170928", "1.10", "160328", "00000000" }; // 4300 版本信息
static VERINFO verinfo_ShanDong  = { "QDGK", "V1.1", "170917", "1.10", "160328", "00000000" }; // 4300 版本信息

static DateTimeBCD product_date = { { 2016 }, { 04 }, { 6 }, { 0 }, { 0 }, { 0 } };   // 4300 生产日期
static char protcol[]           = "DL/T 698.45";                                      // 4300 支持规约类型

static MASTER_STATION_INFO null_info = {};   //备用地址不设置传入
///湖南　Ｉ型
static MASTER_STATION_INFO	master_info_HuNan = {{10,223,31,200},4000};			//IP				端口号
static GprsPara 	gprs_para_HuNan = {"dl.vpdn.hn","cs@dl.vpdn.hn","hn123456",""};		//apn ,userName,passWord,proxyIp
static MASTER_STATION_INFO	master_info_HuNan_4510 = {{192,168,127,127},9027};			//net IP	端口号
static NETCONFIG 	IP_HuNan={1,{192,168,0,10},{255,255,255,0},{192,168,0,1},{},{}};	//网络配置

///浙江　ＩＩ型
static MASTER_STATION_INFO	master_info_ZheJiang = {{10,137,253,7},9006};		//IP				端口号
static GprsPara 	gprs_para_ZheJiang = {"ZJDL.ZJ","card.ZJ","card",""};		//apn ,userName,passWord,proxyIp
static MASTER_STATION_INFO	master_info_ZheJiang_4510 = {{10,137,253,7},9006};			//主net IP	端口号
static MASTER_STATION_INFO	bak_info_ZheJiang_4510 = {{10,137,253,7},9005};			//备net IP	端口号
static NETCONFIG 	IP_ZheJiang={1,{192,168,0,4},{255,255,255,0},{192,168,0,1},{},{}};	//网络配置

///国网送检
static MASTER_STATION_INFO	master_info_GW = {{192,168,127,127},9027};			//IP				端口号
static GprsPara 	gprs_para_GW = {"CMNET","CARD","CARD",""};		//apn ,userName,passWord,proxyIp
static MASTER_STATION_INFO	master_info_GW_4510 = {{192,168,127,127},9027};			//net IP	端口号
static NETCONFIG 	IP_GW={1,{192,168,127,244},{255,255,255,0},{192,168,127,1},{},{}};	//网络配置


void InitClass4500(INT16U heartBeat,MASTER_STATION_INFO master_info,MASTER_STATION_INFO bak_info,GprsPara gprs_para)
{
	CLASS25 class4500 = {};
    memset(&class4500, 0, sizeof(CLASS25));

    class4500.commconfig.workModel  = 1; //客户机模式
    class4500.commconfig.onlineType = 0;	//永久在线
    class4500.commconfig.connectType = 0;	//连接方式:TCP
    class4500.commconfig.appConnectType = 0;	//连接应用方式:主备模式
    class4500.commconfig.listenPortnum = 0;		//监听端口 NULL
    class4500.commconfig.proxyPort = 0;			//代理端口 NULL
    class4500.commconfig.timeoutRtry = (30<<2) | (3 & 0x03);	// bit7~bit2:超时时间 30秒, bit1~bit0:重发次数:3次
    class4500.commconfig.heartBeat  = heartBeat; 	//心跳周期 60s
    memcpy(&class4500.commconfig.apn[1],&gprs_para.apn,strlen((char *)gprs_para.apn));
    class4500.commconfig.apn[0] = strlen((char *)gprs_para.apn);
    memcpy(&class4500.commconfig.userName[1],&gprs_para.userName,strlen((char *)gprs_para.userName));
    class4500.commconfig.userName[0] = strlen((char *)gprs_para.userName);
    memcpy(&class4500.commconfig.passWord[1],&gprs_para.passWord,strlen((char *)(gprs_para.passWord)));
    class4500.commconfig.passWord[0] = strlen((char *)(gprs_para.passWord));
    class4500.master.masternum = 0;
    if(memcmp(&master_info,&null_info,sizeof(MASTER_STATION_INFO))!=0) {
		class4500.master.master[0].ip[0] = strlen((char *)master_info.ip);
		if(class4500.master.master[0].ip[0]<4) 	class4500.master.master[0].ip[0] = 4;
		memcpy(&class4500.master.master[0].ip[1],&master_info.ip,class4500.master.master[0].ip[0]);
		class4500.master.master[0].port = master_info.port;
		class4500.master.masternum++;
    }
    if(memcmp(&bak_info,&null_info,sizeof(MASTER_STATION_INFO))!=0) {
    	class4500.master.master[1].ip[0] = IP_LEN;
		memcpy(&class4500.master.master[1].ip[1],&bak_info.ip,class4500.master.master[1].ip[0]);
		class4500.master.master[1].port = bak_info.port;
		class4500.master.masternum++;
	    fprintf(stderr, "ssss备IP %d.%d.%d.%d:%d\n", class4500.master.master[1].ip[1],
	            class4500.master.master[1].ip[2], class4500.master.master[1].ip[3],
	            class4500.master.master[1].ip[4], class4500.master.master[1].port);
    }
    syslog(LOG_NOTICE, "\nInitClass4500主IP %d.%d.%d.%d:%d  ", class4500.master.master[0].ip[1],
            class4500.master.master[0].ip[2], class4500.master.master[0].ip[3],
            class4500.master.master[0].ip[4], class4500.master.master[0].port);
    fprintf(stderr, "备IP %d.%d.%d.%d:%d\n", class4500.master.master[1].ip[1],
            class4500.master.master[1].ip[2], class4500.master.master[1].ip[3],
            class4500.master.master[1].ip[4], class4500.master.master[1].port);
    saveCoverClass(0x4500, 0, &class4500, sizeof(CLASS25), para_vari_save);
}


void InitClass4510(INT16U heartBeat,MASTER_STATION_INFO master_info,NETCONFIG net_ip) //以太网通信模块1
{
    CLASS26 oi4510 = {};
    int ret        = 0;
    int i=0;

    memset(&oi4510, 0, sizeof(CLASS26));
    ret = readCoverClass(0x4510, 0, (void*)&oi4510, sizeof(CLASS26), para_vari_save);
    if (ret != 1) {
        fprintf(stderr, "\n初始化以太网通信模块1：4510\n");
        oi4510.master.masternum         = 2;		//主站通信参数配置
        for(i=0;i<oi4510.master.masternum;i++) {
        	oi4510.master.master[i].ip[0] = IP_LEN;
        	memcpy(&oi4510.master.master[i].ip[1],&master_info.ip,strlen((char *)master_info.ip));
        	oi4510.master.master[i].port = master_info.port;
        }
        oi4510.IP.ipConfigType = net_ip.ipConfigType;		//IP配置方式:静态
        oi4510.IP.ip[0] = IP_LEN;//strlen((char *)net_ip.ip);
        memcpy(&oi4510.IP.ip[1],&net_ip.ip[0],oi4510.IP.ip[0]);
    	oi4510.IP.subnet_mask[0] = IP_LEN;
    	memcpy(&oi4510.IP.subnet_mask[1],&net_ip.subnet_mask,oi4510.IP.subnet_mask[0]);
       	oi4510.IP.gateway[0] = IP_LEN;
    	memcpy(&oi4510.IP.gateway[1],&net_ip.gateway,strlen((char *)net_ip.gateway));

        //修改ip.sh文件
        writeIpSh(oi4510.IP.ip,oi4510.IP.subnet_mask);

        oi4510.commconfig.workModel     = 1; 	//客户机模式
        oi4510.commconfig.connectType   = 0;	//连接方式:TCP
        oi4510.commconfig.heartBeat     = heartBeat; 	// 60s
        saveCoverClass(0x4510, 0, &oi4510, sizeof(CLASS26), para_vari_save);
    }
}

/*
 * 初始化当前套日时段表
 * */
void InitClass4016() {
//    CLASS_4016 class4016 = {}; //当前套日时段表
//    int readret          = 0;
//    INT8U i              = 0;
//
//    memset(&class4016, 0, sizeof(CLASS_4016));
//    readret = readCoverClass(0x4016, 0, &class4016, sizeof(CLASS_4016), para_vari_save);
//    if (readret != 1) {
//    	fprintf(stderr, "\n初始化当前套日时段表：4016\n");
//        class4016.day_num = MAX_PERIOD_RATE / 2;
//        for (i = 0; i < class4016.day_num; i++) {
//            class4016.Period_Rate[i].hour   = i;
//            class4016.Period_Rate[i].min    = 0;
//            class4016.Period_Rate[i].rateno = (i % 4) + 1;
//        }
//        saveCoverClass(0x4016, 0, &class4016, sizeof(CLASS_4016), para_vari_save);
//    }
}

void InitClass4300() //电气设备信息
{
    CLASS19 oi4300 = {};
    int ret        = 0;
    VERINFO		run_version;

	if(getZone("HuNan")==0) {
		memcpy(&run_version,&verinfo_HuNan,sizeof(VERINFO));
	}if(getZone("ShanDong")==0) {
		memcpy(&run_version,&verinfo_ShanDong,sizeof(VERINFO));
	}else {
		memcpy(&run_version,&verinfo,sizeof(VERINFO));
	}

    memset(&oi4300, 0, sizeof(CLASS19));
    ret = readCoverClass(0x4300, 0, &oi4300, sizeof(CLASS19), para_vari_save);
    if ((ret != 1) || (memcmp(&oi4300.info, &run_version, sizeof(VERINFO)) != 0)
    				|| memcmp(&oi4300.date_Product, &product_date, sizeof(DateTimeBCD)) != 0
    				|| memcmp(&oi4300.protcol, protcol, sizeof(protcol)) != 0) {
        fprintf(stderr, "\n初始化电气设备信息：4300\n");
        memcpy(&oi4300.info, &run_version, sizeof(VERINFO));
        memcpy(&oi4300.date_Product, &product_date, sizeof(DateTimeBCD));
        memcpy(&oi4300.protcol, protcol, sizeof(oi4300.protcol));
        oi4300.active_report = 1;	//运行主动上报
        saveCoverClass(0x4300, 0, &oi4300, sizeof(CLASS19), para_vari_save);
    }
    fprintf(stderr, "\n厂商代码 %c%c%c%c", oi4300.info.factoryCode[0], oi4300.info.factoryCode[1], oi4300.info.factoryCode[2], oi4300.info.factoryCode[3]);
    fprintf(stderr, "\n软件版本 %c%c%c%c", oi4300.info.softVer[0], oi4300.info.softVer[1], oi4300.info.softVer[2], oi4300.info.softVer[3]);
    fprintf(stderr, "\n软件版本日期 %c%c%c%c%c%c", oi4300.info.softDate[0], oi4300.info.softDate[1], oi4300.info.softDate[2], oi4300.info.softDate[3],
            oi4300.info.softDate[4], oi4300.info.softDate[5]);
    fprintf(stderr, "\n硬件版本 %c%c%c%c", oi4300.info.hardVer[0], oi4300.info.hardVer[1], oi4300.info.hardVer[2], oi4300.info.hardVer[3]);
    fprintf(stderr, "\n硬件版本日期 %c%c%c%c%c%c", oi4300.info.hardDate[0], oi4300.info.hardDate[1], oi4300.info.hardDate[2], oi4300.info.hardDate[3],
            oi4300.info.hardDate[4], oi4300.info.hardDate[5]);
    fprintf(stderr, "\n规约列表 %s", oi4300.protcol);
    fprintf(stderr, "\n生产日期 %d-%d-%d\n", oi4300.date_Product.year.data, oi4300.date_Product.month.data, oi4300.date_Product.day.data);
    fprintf(stderr, "\n主动上报 %d\n",oi4300.active_report);
}

/*
 * 初始化采集配置单元
 * */
void InitClass6000() {
    CLASS_6001 meter        = {};
    int readret             = 0;
    static INT8U ACS_TSA[8] = { 0x08, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }; //[0]=08,TSA长度 [1]=05,05+1=地址长度，交采地址：000000000001

    readret = readParaClass(0x6000, &meter, 1);
    if (readret != 1) {
        fprintf(stderr, "\n初始化采集配置单元：6000\n");
        memset(&meter, 0, sizeof(CLASS_6001));
        meter.sernum = 1;
        memcpy(meter.basicinfo.addr.addr, ACS_TSA, sizeof(ACS_TSA));
        meter.basicinfo.baud           = 3;
        meter.basicinfo.protocol       = 3;
        meter.basicinfo.port.OI        = 0xF208;
        meter.basicinfo.port.attflg    = 0x02;
        meter.basicinfo.port.attrindex = 0x01;
        meter.basicinfo.ratenum        = MAXVAL_RATENUM;
        //		meter.basicinfo.connectype = getACSConnectype();
        meter.basicinfo.ratedU = 2200;
        meter.basicinfo.ratedI = 1500;
        saveParaClass(0x6000, &meter, meter.sernum);
    }
}

/*
 * 开关量输入
 * */
void InitClassf203()
{
    CLASS_f203 oif203 = {};
    int readret       = 0;
//    ConfigPara	cfgpara = {};

//    cfgpara.device = CCTT2;
//    ReadDeviceConfig(&cfgpara);
    memset(&oif203, 0, sizeof(oif203));
    readret = readCoverClass(0xf203, 0, &oif203, sizeof(CLASS_f203), para_vari_save);
    if (readret != 1) {
//        fprintf(stderr, "初始化开关量输入：【F203】 设备类型:%d\n",cfgpara.device);
        strncpy((char*)&oif203.class22.logic_name, "F203", sizeof(oif203.class22.logic_name));
		oif203.class22.device_num    = 1;
		oif203.statearri.num         = 8;
		oif203.state4.StateAcessFlag = 0xFF; //所有开关量接入标志位均为1(接入)
		oif203.state4.StatePropFlag  = 0xFF; //所有开关量属性标志位均为1(常开)
//        switch(cfgpara.device) {
//        case CCTT1:
//            oif203.class22.device_num    = 1;
//            oif203.statearri.num         = 4;
//            oif203.state4.StateAcessFlag = 0xF0; //第1路状态接入
//            oif203.state4.StatePropFlag  = 0xF0; //第1路状态常开触点
//        	break;
//        case CCTT2:
//            oif203.class22.device_num    = 1;
//            oif203.statearri.num         = 1;
//            oif203.state4.StateAcessFlag = 0x80; //第1路状态接入
//            oif203.state4.StatePropFlag  = 0x80; //第1路状态常开触点
//        	break;
//        }
        saveCoverClass(0xf203, 0, &oif203, sizeof(CLASS_f203), para_vari_save);
    }
}


/*
 * type = 1: 判断参数不存在初始化,上电运行判断参数文件
 * 　　　　　= 0: 初始化参数
 * */
void InitClassByZone(INT8U type)
{
	int ret        = 0;
	CLASS25 class4500 = {};
	INT16U		heartBeat = 60;	//单位秒
	if(type == 1) {
    	ret = readCoverClass(0x4500, 0, (void*)&class4500, sizeof(CLASS25), para_vari_save);
	}else ret = 0;
    if (ret != 1) {
    	if(getZone("ZheJiang")==0) {
    		heartBeat = 300;	//5分钟
			InitClass4500(heartBeat,master_info_ZheJiang,bak_info_ZheJiang_4510,gprs_para_ZheJiang);
			InitClass4510(heartBeat,master_info_ZheJiang_4510,IP_ZheJiang);    //以太网通信模块1
		}else if(getZone("HuNan")==0) {
			heartBeat = 300;	//5分钟
			InitClass4500(heartBeat,master_info_HuNan,null_info,gprs_para_HuNan);
			InitClass4510(heartBeat,master_info_HuNan_4510,IP_HuNan);    //以太网通信模块1
		}else if(getZone("GW")==0) {
			InitClass4500(heartBeat,master_info_GW,master_info_GW,gprs_para_GW);
			InitClass4510(heartBeat,master_info_GW_4510,IP_GW);    //以太网通信模块1
			system("cp /nor/init/table6000.* /nand/para/");
			system("cp -rf /nor/init/6015 /nand/para/");
			system("cp -rf /nor/init/6013 /nand/para/");
		}
    }
}
