/*
 * cjdev.c
 *
 *  Created on: Feb 9, 2017
 *      Author: ava
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "StdDataType.h"
#include "Shmem.h"
#include "EventObject.h"
#include "PublicFunction.h"
#include "AccessFun.h"
#include "Objectdef.h"
#include "ParaDef.h"
#include "Shmem.h"
#include "main.h"

void printF203()
{
	static CLASS_f203	oif203={};
	readCoverClass(0xf203,0,&oif203,sizeof(CLASS_f203),para_vari_save);
	fprintf(stderr,"[F203]开关量输入\n");
	fprintf(stderr,"逻辑名 %s\n",oif203.class22.logic_name);
	fprintf(stderr,"设备对象数量：%d\n",oif203.class22.device_num);
	fprintf(stderr,"属性2：ST=%d_%d_%d_%d %d_%d_%d_%d\n",oif203.statearri.stateunit[0].ST,oif203.statearri.stateunit[1].ST,oif203.statearri.stateunit[2].ST,oif203.statearri.stateunit[3].ST,oif203.statearri.stateunit[4].ST,oif203.statearri.stateunit[5].ST,oif203.statearri.stateunit[6].ST,oif203.statearri.stateunit[7].ST);
	fprintf(stderr,"属性2：CD=%d_%d_%d_%d %d_%d_%d_%d\n\n",oif203.statearri.stateunit[0].CD,oif203.statearri.stateunit[1].CD,oif203.statearri.stateunit[2].CD,oif203.statearri.stateunit[3].CD,oif203.statearri.stateunit[4].CD,oif203.statearri.stateunit[5].CD,oif203.statearri.stateunit[6].CD,oif203.statearri.stateunit[7].CD);
	fprintf(stderr,"属性4：接入标志=%02x\n",oif203.state4.StateAcessFlag);
	fprintf(stderr,"属性4：属性标志=%02x\n",oif203.state4.StatePropFlag);
}

//void SetF101(int argc, char *argv[])
//{
//	CLASS_F101  f101={};
//	readCoverClass(0xf101,0,&f101,sizeof(CLASS_F101),para_vari_save);
//
//}

INT8U workModel;					//工作模式 enum{混合模式(0),客户机模式(1),服务器模式(2)},
INT8U onlineType;					//在线方式 enum{永久在线(0),被动激活(1)}
INT8U connectType;					//连接方式 enum{TCP(0),UDP(1)}
INT8U appConnectType;				//连接应用方式 enum{主备模式(0),多连接模式(1)}
INT8U apn[OCTET_STRING_LEN];		//apn
INT8U userName[OCTET_STRING_LEN];	//用户名称
INT8U passWord[OCTET_STRING_LEN];	//密码
INT8U proxyIp[OCTET_STRING_LEN];	//代理服务器地址
INT16U proxyPort;					//代理端口
INT8U timeoutRtry;					//超时时间，重发次数
INT8U heartBeat;					//心跳周期秒

void Init_4500(){
	CLASS25 obj;
	memset(&obj,0,sizeof(obj));
	obj.commconfig.workModel = 1;
	obj.commconfig.onlineType = 0;
	obj.commconfig.connectType = 0;
	obj.commconfig.appConnectType = 0;
	memcpy(obj.commconfig.apn, "cmcc", 4);
	memcpy(obj.commconfig.userName, "user", 4);
	memcpy(obj.commconfig.passWord, "user", 4);
	memcpy(obj.commconfig.proxyIp, "0.0.0.0", 7);
	obj.commconfig.proxyPort = 0;
	obj.commconfig.timeoutRtry = 3;
	obj.commconfig.heartBeat = 300;
	memcpy(obj.master.ip, "192.168.0.97", sizeof("192.168.0.97"));
	obj.master.port = 5200;

	saveCoverClass(0x4500,0,(void *)&obj,sizeof(CLASS25),para_init_save);
}

void inoutdev_process(int argc, char *argv[])
{
	int 	tmp=0;
	OI_698	oi=0;

	if(argc>=2) {	//dev pro
		if(strcmp(argv[1],"dev")==0) {
			if(strcmp(argv[2],"pro")==0) {
				sscanf(argv[3],"%04x",&tmp);
				oi = tmp;
				switch(oi) {
				case 0xf203:
					printF203();
					break;
				}
			}
			if(strcmp(argv[2],"init")==0) {
				sscanf(argv[3],"%04x",&tmp);
				oi = tmp;
				switch(oi) {
				case 0x4500:
					Init_4500();
					break;
				}
			}
		}
	}
}
