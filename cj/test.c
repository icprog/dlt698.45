/*
 * esam.c
 *
 *  Created on: Mar 30, 2017
 *      Author: ava
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <semaphore.h>
#include <fcntl.h>

#include "StdDataType.h"
#include "EventObject.h"
#include "PublicFunction.h"
#include "AccessFun.h"
#include "filebase.h"
#include "Objectdef.h"
#include "ParaDef.h"
#include "Shmem.h"
#include "main.h"
extern ProgramInfo* JProgramInfo ;

TSA tsas[]={
		{0x07,0x05,0x00,0x00,0x00,0x00,0x00,0x01},
		{0x07,0x05,0x00,0x00,0x00,0x00,0x00,0x02},
		{0x07,0x05,0x00,0x00,0x00,0x00,0x00,0x03},
		{0x07,0x05,0x00,0x00,0x00,0x00,0x00,0x04},
		{0x07,0x05,0x00,0x00,0x00,0x00,0x00,0x05},
};

void Test(int argc, char *argv[])
{
	if (strcmp("plcmode", argv[1]) == 0) {
        JProgramInfo = OpenShMem("ProgramInfo", sizeof(ProgramInfo), NULL);
        JProgramInfo->dev_info.PLC_ModeTest = atoi(argv[2]);
        if( JProgramInfo->dev_info.PLC_ModeTest == 1) {
        	fprintf(stderr,"模拟设置集中器主导\n");
        }else  if( JProgramInfo->dev_info.PLC_ModeTest == 2) {
        	fprintf(stderr,"模拟设置路由主导\n");
        }else  {
        	JProgramInfo->dev_info.PLC_ModeTest = 0;
        	fprintf(stderr,"设置载波工作主导无效\n");
        }
        shmm_unregister("ProgramInfo", sizeof(ProgramInfo));
	}
    if (strcmp("trydel", argv[1]) == 0) {
    	deloutofdatafile();
    }
    if (strcmp("savetest", argv[1]) == 0) {
        DateTimeBCD dt;
        PassRate_U passu[3];
        OAD oad;
        int val = 1, i = 0;
        INT8U buf[256];
        int buflen = 0;

        if (argc == 6) {
            dt.year.data = atoi(argv[2]);
            dt.month.data = atoi(argv[3]);
            dt.day.data = atoi(argv[4]);
            val = atoi(argv[5]);
        }
        dt.hour.data = 0;
        dt.min.data = 0;
        dt.sec.data = 0;
        fprintf(stderr, "write time %04d-%02d-%02d %02d:%02d:%02d,val=%d\n", dt.year.data, dt.month.data, dt.day.data,
                dt.hour.data, dt.min.data, dt.sec.data, val);

        for (i = 0; i < 3; i++) {
            oad.OI = 0x2131 + i;
            oad.attflg = 0x02;
            oad.attrindex = 0x01;
            passu[i].monitorTime = val * (i + 1);
            passu[i].passRate = val * (i + 1) + 1;
            passu[i].overRate = val * (i + 1) + 2;
            passu[i].upLimitTime = val * (i + 1) + 3;
            passu[i].downLimitTime = val * (i + 1) + 4;
            saveFreezeRecord(0x5004, oad, dt, sizeof(PassRate_U), (INT8U * ) & passu[i]);
        }
        return EXIT_SUCCESS;
    }

	if (strcmp("report", argv[1]) == 0) {
		JProgramInfo = OpenShMem("ProgramInfo", sizeof(ProgramInfo), NULL);
		int retaskid = 64;
		TS ts_rep1, ts_rep2;
		if (argc >= 3) {
			if (argc == 10) {
				retaskid = atoi(argv[2]);
				TSGet(&ts_rep1);
				ts_rep1.Year = atoi(argv[3]);
				ts_rep1.Month = atoi(argv[4]);
				ts_rep1.Day = atoi(argv[5]);
				ts_rep1.Hour = atoi(argv[6]);
				ts_rep1.Minute = atoi(argv[7]);
				memcpy(&ts_rep2, &ts_rep1, sizeof(TS));
				ts_rep2.Hour = atoi(argv[8]);
				ts_rep2.Minute = atoi(argv[9]);
			} else {
				fprintf(stderr,
						"\n参数不够，格式如下:cj report 64 2017 6 6 10 30 11 30 **上报任务17-6-6 10:30到11:30这个点的数据上报\n");
				return;
			}
		}
		fprintf(stderr, "report taskid=%d\n", retaskid);
		supplementRpt(ts_rep1, ts_rep2, retaskid, &JProgramInfo->cfg_para.extpara[0]);
		shmm_unregister("ProgramInfo", sizeof(ProgramInfo));
		return EXIT_SUCCESS;
	}

    if (strcmp("ms", argv[1]) == 0) {
		 int taskid = 64;
		 int ret = 0;
		 if (argc >= 3) {
			 taskid = atoi(argv[2]);
		 }
		 fprintf(stderr, "taskid=%d\n", taskid);
		 CLASS_601D class601d = {};
		 if (readCoverClass(0x601D, taskid, &class601d, sizeof(CLASS_601D), coll_para_save) == 1) {
			 ret = GetReportData(class601d);
		 }
		 return EXIT_SUCCESS;
    }
    if (strcmp("gettsas", argv[1]) == 0) {
    	CLASS_6001  *tsa_group = NULL;
//    	TSA *tsa_group = NULL;
    	MY_MS	ms;
    	int		tsa_num=0;
    	int		i=0,j=0;
		int 	seqNum = 1;

    	if(argc >= 3) {
    		ms.mstype = atoi(argv[2]);
    	}else {
    		ms.mstype = 5;
    	}
		for(i=0;i<COLLCLASS_MAXNUM;i++) {
			ms.ms.type[i].type = interface;//初始化interface, 用来获取有效的区间长度，Region_Type type;//type = interface 无效
		}
		fprintf(stderr,"test  ms.mstype = %d\n",ms.mstype);
    	switch(ms.mstype) {
    	case 2:
    		seqNum = 2;
    		ms.ms.userType[0]= (seqNum >> 8);
    		ms.ms.userType[1]= seqNum & 0xff;
    		for(i=0;i<seqNum;i++) {
    			ms.ms.userType[i+2] = 3+i;
    		}
    		for(i=0;i<(seqNum+2);i++) {
    			fprintf(stderr,"userType[%d]=%d\n",i,ms.ms.userType[i]);
    		}
    		break;
    	case 3:
    		seqNum = 5;
    		ms.ms.userAddr[0].addr[0] = (seqNum >> 8);
    		ms.ms.userAddr[0].addr[1] = seqNum & 0xff;
    		fprintf(stderr,"seqNum = %d\n",seqNum);
    		for(i=0;i<seqNum;i++) {
    			memcpy(&ms.ms.userAddr[i+1].addr,&tsas[i],sizeof(TSA));
    		}
    		for(i=0;i<(seqNum+1);i++) {
    			printTSA(ms.ms.userAddr[i]);
    		}
    		break;
    	case 4:
    		seqNum = 4;
    		ms.ms.configSerial[0] = seqNum;
    		ms.ms.configSerial[1] = 12;
    		ms.ms.configSerial[2] = 99;
    		ms.ms.configSerial[3] = 111;
    		ms.ms.configSerial[4] = 290;
    		break;
    	case 5:
    		ms.ms.type[0].type = close_open;
    		ms.ms.type[0].begin[0] = dtunsigned;
    		ms.ms.type[0].begin[1] = 31;
    		ms.ms.type[0].end[0] = dtunsigned;
    		ms.ms.type[0].end[1] = 43;
    		break;
    	case 7:
    		ms.ms.serial[0].type = close_open;
    		seqNum = 22;
    		ms.ms.serial[0].begin[0] = dtlongunsigned;
    		ms.ms.serial[0].begin[1] = (seqNum >> 8) & 0xff;
    		ms.ms.serial[0].begin[2] = seqNum & 0xff;
    		seqNum = 291;
    		ms.ms.serial[0].end[0] = dtlongunsigned;
    		ms.ms.serial[0].end[1] = (seqNum >> 8) & 0xff;
    		ms.ms.serial[0].end[2] = seqNum & 0xff;
    		break;
    	}
//    	tsa_num = getTsas(ms,(INT8U **)&tsa_group);


    	tsa_num = getOI6001(ms,(INT8U **)&tsa_group);

//    	fprintf(stderr,"get 需要上报的：tsa_num=%d,tsa_group=%p\n",tsa_num,tsa_group);
//    	for(i=0;i<tsa_num;i++) {
//    		fprintf(stderr,"\nTSA%d: %d-",i,tsa_group[i].addr[0]);
//    		for(j=0;j<tsa_group[i].addr[0];j++) {
//    			fprintf(stderr,"-%02x",tsa_group[i].addr[j+1]);
//    		}
//    	}
    	fprintf(stderr,"\n\ntsa_num = %d\n",tsa_num);
    	for(i=0;i<tsa_num;i++) {
    		printTSA(tsa_group[i]);
    		fprintf(stderr,"用户类型:%d\n",tsa_group[i].basicinfo.usrtype);
    	}
    	if(tsa_group != NULL)
    		free(tsa_group);

    	return EXIT_SUCCESS;
    }
}
