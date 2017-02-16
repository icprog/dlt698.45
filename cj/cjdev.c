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
	CLASS_f203	oif203={};
	readCoverClass(0xf203,0,&oif203,sizeof(CLASS_f203),para_vari_save);
	fprintf(stderr,"[F203]开关量输入\n");
	fprintf(stderr,"逻辑名 %s\n",oif203.class22.logic_name);
	fprintf(stderr,"设备对象数量：%d\n",oif203.class22.device_num);
	fprintf(stderr,"属性2：ST=%d_%d_%d_%d %d_%d_%d_%d\n",oif203.statearri.stateunit[0].ST,oif203.statearri.stateunit[1].ST,oif203.statearri.stateunit[2].ST,oif203.statearri.stateunit[3].ST,oif203.statearri.stateunit[4].ST,oif203.statearri.stateunit[5].ST,oif203.statearri.stateunit[6].ST,oif203.statearri.stateunit[7].ST);
	fprintf(stderr,"属性2：CD=%d_%d_%d_%d %d_%d_%d_%d\n\n",oif203.statearri.stateunit[0].CD,oif203.statearri.stateunit[1].CD,oif203.statearri.stateunit[2].CD,oif203.statearri.stateunit[3].CD,oif203.statearri.stateunit[4].CD,oif203.statearri.stateunit[5].CD,oif203.statearri.stateunit[6].CD,oif203.statearri.stateunit[7].CD);
	fprintf(stderr,"属性4：接入标志=%02x\n",oif203.state4.StateAcessFlag);
	fprintf(stderr,"属性4：属性标志=%02x\n",oif203.state4.StatePropFlag);
}

void printF101()
{
	CLASS_F101  f101={};
	int		i=0;
	readCoverClass(0xf101,0,&f101,sizeof(CLASS_F101),para_vari_save);
	fprintf(stderr,"[F101]安全模式参数\n");
	if(f101.active==0) {
		fprintf(stderr,"属性2:安全模式选择：不启用安全模式参数\n");
	}else 	if(f101.active==1) {
		fprintf(stderr,"属性2:安全模式选择：启用安全模式参数\n");
	}else {
		fprintf(stderr,"属性2:安全模式选择[0,1]：读取无效值：%d\n",f101.active);
	}
	fprintf(stderr,"安全模式参数个数：%d\n",f101.modelnum);
	for(i=0;i<f101.modelnum;i++) {
		fprintf(stderr,"OI=%04x 安全模式=%d\n",f101.modelpara[i].oi,f101.modelpara[i].model);
	}
}

void SetF101(int argc, char *argv[])
{
	CLASS_F101  f101={};
	int		tmp=0;
	if(strcmp(argv[2],"init")==0) {
		memset(&f101,0,sizeof(CLASS_F101));
		f101.active = 1;		//初始化启用
		saveCoverClass(0xf101,0,&f101,sizeof(CLASS_F101),para_init_save);
	}
	if(strcmp(argv[2],"set")==0) {
		memset(&f101,0,sizeof(CLASS_F101));
		readCoverClass(0xf101,0,&f101,sizeof(CLASS_F101),para_vari_save);
		sscanf(argv[4],"%d",&tmp);
		f101.active = tmp;
		saveCoverClass(0xf101,0,&f101,sizeof(CLASS_F101),para_vari_save);
	}
}

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
	memcpy(obj.master[0].ip, "192.168.0.97", sizeof("192.168.0.97"));
	obj.master[0].port = 5022;

	saveCoverClass(0x4500,0,(void *)&obj,sizeof(CLASS25),para_init_save);
}

void inoutdev_process(int argc, char *argv[])
{
	int 	tmp=0;
	OI_698	oi=0;

	if(argc>=2) {	//dev pro
		if(strcmp(argv[1],"dev")==0) {
			sscanf(argv[3],"%04x",&tmp);
			oi = tmp;
			if(strcmp(argv[2],"pro")==0) {
				switch(oi) {
				case 0xf203:
					printF203();
					break;
				case 0xf101:
					printF101();
				}
			}else {
				switch(oi) {
				case 0xf101:
					SetF101(argc,argv);
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
