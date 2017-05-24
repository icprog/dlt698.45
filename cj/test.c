/*
 * esam.c
 *
 *  Created on: Mar 30, 2017
 *      Author: ava
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <semaphore.h>
#include <fcntl.h>

#include "StdDataType.h"
#include "EventObject.h"
#include "PublicFunction.h"
#include "AccessFun.h"
#include "Objectdef.h"
#include "ParaDef.h"
#include "Shmem.h"
#include "main.h"


void Test(int argc, char *argv[])
{
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
    	TSA *tsa_group = NULL;
    	MY_MS	ms;
    	int		tsa_num=0;
    	int		i=0,j=0;

    	if(argc >= 3) {
    		ms.mstype = atoi(argv[2]);
    	}else {
    		ms.mstype = 5;
    	}
		for(i=0;i<COLLCLASS_MAXNUM;i++) {
			ms.ms.type[i].type = interface;//初始化interface, 用来获取有效的区间长度，Region_Type type;//type = interface 无效
		}
    	switch(ms.mstype) {
    	case 5:
    		ms.ms.type[0].type = close_open;
    		ms.ms.type[0].begin[0] = dtunsigned;
    		ms.ms.type[0].begin[1] = 96;
    		ms.ms.type[0].end[0] = dtunsigned;
    		ms.ms.type[0].end[1] = 111;
    		break;
    	}
    	tsa_num = getTsas(ms,(INT8U **)&tsa_group);
    	fprintf(stderr,"get 需要上报的：tsa_num=%d,tsa_group=%p\n",tsa_num,tsa_group);
    	for(i=0;i<tsa_num;i++) {
    		fprintf(stderr,"\nTSA%d: %d-",i,tsa_group[i].addr[0]);
    		for(j=0;j<tsa_group[i].addr[0];j++) {
    			fprintf(stderr,"-%02x",tsa_group[i].addr[j+1]);
    		}
    	}
//    	fprintf(stderr,"tsa_num = %d\n",tsa_num);
//    	for(i=0;i<tsa_num;i++) {
//    		printTSA(tsa_group);
//    	}
    	if(tsa_group != NULL)
    		free(tsa_group);

    	return EXIT_SUCCESS;
    }
}
