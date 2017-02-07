/*
 * event.c
 *
 *  Created on: Jan 12, 2017
 *      Author: ava
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "StdDataType.h"
#include "EventObject.h"
#include "AccessFun.h"
#include "ParaDef.h"
//property


void printClass7(Class7_Object class7)
{
	int i=0;
	fprintf(stderr,"【Class7】逻辑名: %04x\n",class7.oi);
	fprintf(stderr,"当前记录数  最大记录数  上报标识  有效标识  关联对象属性【OAD】\n");
	fprintf(stderr,"%d           %d           %d           %d \n",class7.crrentnum,class7.maxnum,class7.reportflag,class7.enableflag);
	for(i=0;i<CLASS7_OAD_NUM;i++) {
		fprintf(stderr,"%04X_%02X%02X ",class7.oadarr[i].OI,class7.oadarr[i].attflg,class7.oadarr[i].attrindex);
	}
	fprintf(stderr,"\n");
}

void printClass310d()
{
	Event310D_Object Event310d; //电能表飞走事件12
	int i=0;

	readCoverClass(0x310d,0,&Event310d,sizeof(Event310D_Object),event_para_save);
	fprintf(stderr,"【Event310D】电能表飞走事件: %04x\n",Event310d.event_obj.oi);
	fprintf(stderr,"当前记录数  最大记录数  上报标识  有效标识  关联对象属性【OAD】\n");
	fprintf(stderr,"%d           %d           %d           %d \n",
			Event310d.event_obj.crrentnum,Event310d.event_obj.maxnum,Event310d.event_obj.reportflag,Event310d.event_obj.enableflag);
	for(i=0;i<CLASS7_OAD_NUM;i++) {
		fprintf(stderr,"%04X_%02X%02X ",Event310d.event_obj.oadarr[i].OI,Event310d.event_obj.oadarr[i].attflg,Event310d.event_obj.oadarr[i].attrindex);
	}
	fprintf(stderr,"\n阈值=%d(%)  关联采集任务号=%d\n",Event310d.poweroffset_obj.power_offset,Event310d.poweroffset_obj.task_no);
	fprintf(stderr,"\n");
}

void event_process(int argc, char *argv[])
{
	OI_698	oi=0;
	int 	tmp[20]={};
	Class7_Object	class7={};
	int		i = 0,ret = 0;

	if(argc>=4) {	//event att 3100
//		fprintf(stderr,"argv=%s",argv[3]);
		sscanf(argv[3],"%04x",&tmp[0]);
		oi = tmp[0];
		if(strcmp("reset",argv[2])==0) {
			ret = resetClass(oi);
			fprintf(stderr,"复位出错=%d",ret);
		}
		if(strcmp("pro",argv[2])==0) {
			if(argc==4) {
				switch(oi) {
				case 0x310d:
					printClass310d();
					break;
				case 0x3100:
					memset(&class7,0,sizeof(Class7_Object));
					readCoverClass(oi,0,&class7,sizeof(Class7_Object),event_para_save);
					printClass7(class7);
					break;
				}
			}else {
				switch(oi) {
				case 0x3100:	//终端初始化事件1
				case 0x3104:	//终端状态量变位事件3
					memset(&class7,0,sizeof(Class7_Object));
					class7.oi = oi;
					sscanf(argv[4],"%d",&tmp[1]);
					class7.crrentnum = tmp[1];
					sscanf(argv[5],"%d",&tmp[1]);
					class7.maxnum = tmp[1];
					sscanf(argv[6],"%d",&tmp[1]);
					class7.reportflag = tmp[1];
					sscanf(argv[7],"%d",&tmp[1]);
					class7.enableflag = tmp[1];
					sscanf(argv[8],"%d",&tmp[0]);
					for(i=0;i<tmp[0];i++) {
						fprintf(stderr,"argv[%d]=%s",i+9,argv[9+i]);
						sscanf(argv[i+9],"%04X-%02X%02X",&tmp[1],&tmp[2],&tmp[3]);
						class7.oadarr[i].OI = tmp[1];
						class7.oadarr[i].attflg = tmp[2];
						class7.oadarr[i].attrindex = tmp[3];
						fprintf(stderr," %04X-%02X%02X\n",class7.oadarr[i].OI,class7.oadarr[i].attflg,class7.oadarr[i].attrindex);
					}
//					fprintf(stderr," crrentnum=%d maxnum=%d reportflag=%d enableflag=%d\n",class7.crrentnum,class7.maxnum,class7.reportflag,class7.enableflag);
//					fprintf(stderr,"class7size=%d\n",sizeof(Class7_Object));
					saveCoverClass(class7.oi,0,&class7,sizeof(Class7_Object),event_para_save);
					break;
				}
			}
		}
	}
}
