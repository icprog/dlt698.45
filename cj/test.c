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
void bu_report(TS ts1,TS ts2,INT8U retaskid,INT8U *saveflg)
{
	char filename[60];
	FILE *tk_fp = NULL,*fr_fp=NULL;
	INT8U onefrmbuf[2000],recordbuf[2000],tmpnull[8],seqnumindex=0,taskid=0;
	INT16U taskseq = 0,taskseqend = 0,headsize = 0,unitnum = 0,tsa_num=0,recordnum=0,recordlen=0,framesum=0,blocksize = 0,i=0,j=0;//上报的序号
	INT32U indexn=0,offsetTsa = 0,recordoffset=0;
	TSA *tsa_group = NULL;
	HEAD_UNIT *headunit = NULL;//文件头
	OAD_INDEX oad_offset[100],oad_offset_can[100];//oad索引
	ROAD_ITEM item_road;
	memset(&item_road,0,sizeof(item_road));
	CLASS_601D class601d = {};

	*saveflg = 0;
	memset(filename,0x00,sizeof(filename));
	sprintf(filename,"/nand/reportdata");
	if(access(filename,F_OK)==0)
	{
		fprintf(stderr,"\n文件%s存在，退出！！！\n",filename);
		if(tk_fp!=NULL)
			fclose(tk_fp);
		if(fr_fp!=NULL)
			fclose(fr_fp);
		if(tsa_group != NULL)
			free(tsa_group);
		if(headunit!=NULL)
			free(headunit);
		return;
	}
	fr_fp = fopen(filename,"a+");
	if(fr_fp == NULL)
	{
		system("rm /nand/reportdata");
		if(tk_fp!=NULL)
			fclose(tk_fp);
		if(fr_fp!=NULL)
			fclose(fr_fp);
		if(tsa_group != NULL)
			free(tsa_group);
		if(headunit!=NULL)
			free(headunit);
		fprintf(stderr,"\n打开存储帧文件失败\n");
		return;
	}

	if (readCoverClass(0x601D, retaskid, &class601d, sizeof(CLASS_601D), coll_para_save) != 1) {
		system("rm /nand/reportdata");
		if(tk_fp!=NULL)
			fclose(tk_fp);
		if(fr_fp!=NULL)
			fclose(fr_fp);
		if(tsa_group != NULL)
			free(tsa_group);
		if(headunit!=NULL)
			free(headunit);
		fprintf(stderr,"\n获取任务%d的采集方案失败\n",retaskid);
		return;
	}

//	extendcsds(class601d.reportdata.data.recorddata.csds,&item_road);
//	memset(&item_road,0,sizeof(item_road));
	if((taskid = GetTaskidFromCSDs(class601d.reportdata.data.recorddata.csds,&item_road)) == 0) {//暂时不支持招测的不在一个采集方案
		system("rm /nand/reportdata");
		fprintf(stderr,"GetTaskData: taskid=%d\n",taskid);
		if(tk_fp!=NULL)
			fclose(tk_fp);
		if(fr_fp!=NULL)
			fclose(fr_fp);
		if(tsa_group != NULL)
			free(tsa_group);
		if(headunit!=NULL)
			free(headunit);
		return;
	}

	memset(filename,0x00,sizeof(filename));
	sprintf(filename,"/nand/task/%03d/%04d%02d%02d.dat",taskid,ts1.Year,ts1.Month,ts1.Day);
	taskseq = ts1.Hour*4+ts1.Minute/15;
	taskseqend = ts2.Hour*4+ts2.Minute/15;
	if(taskseqend < taskseq) {
		fprintf(stderr,"设置结束时间超前开始时间,参数错误,退出");
		system("rm /nand/reportdata");
		if(tk_fp!=NULL)
			fclose(tk_fp);
		if(fr_fp!=NULL)
			fclose(fr_fp);
		if(tsa_group != NULL)
			free(tsa_group);
		if(headunit!=NULL)
			free(headunit);
		return;
	}
	fprintf(stderr,"\n任务%d 查找文件%s 序列号%d-%d\n",taskid,filename,taskseq,taskseqend);
	if(access(filename,F_OK)!=0)
	{
		system("rm /nand/reportdata");
		fprintf(stderr,"\n任务数据文件%s不存在！\n",filename);
		if(tk_fp!=NULL)
			fclose(tk_fp);
		if(fr_fp!=NULL)
			fclose(fr_fp);
		if(tsa_group != NULL)
			free(tsa_group);
		if(headunit!=NULL)
			free(headunit);
		return;
	}
	tk_fp = fopen(filename,"r");
	if(tk_fp == NULL)
	{
		system("rm /nand/reportdata");
		fprintf(stderr,"\n打开任务数据文件%s失败！\n",filename);
		if(tk_fp!=NULL)
			fclose(tk_fp);
		if(fr_fp!=NULL)
			fclose(fr_fp);
		if(tsa_group != NULL)
			free(tsa_group);
		if(headunit!=NULL)
			free(headunit);
		return;
	}
	unitnum = GetTaskHead(tk_fp,&headsize,&blocksize,&headunit);//得到任务文件头

	memset(oad_offset,0x00,sizeof(oad_offset));
	GetOADPosofUnit(item_road,headunit,unitnum,oad_offset);//得到每一个oad在块数据中的偏移

	tsa_num = getTsas(class601d.reportdata.data.recorddata.rsd.selec10.meters,(INT8U **)&tsa_group);
	//-----------------------------------------------------------------------------------------------------------组帧
	memset(onefrmbuf,0,sizeof(onefrmbuf));
	//初始化分帧头
	indexn = 2;
	indexn += initFrameHead(&onefrmbuf[indexn],class601d.reportdata.data.oad,class601d.reportdata.data.recorddata.rsd,10,class601d.reportdata.data.recorddata.csds,&seqnumindex);

	if(tsa_num == 0) {
		fprintf(stderr,"未找到符合条件的TSA数据\n");
		if(tk_fp!=NULL)
			fclose(tk_fp);
		if(fr_fp!=NULL)
			fclose(fr_fp);
		if(tsa_group != NULL)
			free(tsa_group);
		if(headunit!=NULL)
			free(headunit);
		return;
	}
	for(i =0; i< tsa_num; i++)
	{
		offsetTsa = findTsa(tsa_group[i],tk_fp,headsize,blocksize);//计算tsa偏移
		if(offsetTsa == 0) {
			fprintf(stderr,"task未找到数据,i=%d\n",i);
			indexn += fillTsaNullData(&onefrmbuf[indexn],tsa_group[i],item_road);
			recordnum++;
			continue;
		}
		recordlen = blocksize/96;//计算每条记录的字节数
		for(j=taskseq;j<=taskseqend;j++)
		{
			fprintf(stderr,"\nrecordlen = %d\n",recordlen);
			recordoffset = findrecord(offsetTsa,recordlen,j);//计算record偏移

			memset(recordbuf,0x00,sizeof(recordbuf));
			//6\读出一行数据到临时缓存
			fseek(tk_fp,recordoffset,SEEK_SET);
			fread(recordbuf,recordlen,1,tk_fp);

			memset(tmpnull,0x00,8);
			if(memcmp(&recordbuf[18],tmpnull,8)==0)//本条记录为空
			{
				fprintf(stderr,"\n本条记录号%d为空\n",j);
				continue;
			}
			printRecordBytes(recordbuf,recordlen);
			//7\根据csds挑选数据，组织存储缓存
			memcpy(oad_offset_can,oad_offset,sizeof(oad_offset));
			indexn += collectData(&onefrmbuf[indexn],recordbuf,oad_offset_can,item_road);
			recordnum++;
			asyslog(LOG_INFO,"recordnum=%d  seqnumindex=%d\n",recordnum,seqnumindex);

			if (indexn>=1024-100)
	//			if (indexn>=900)
			{
				framesum++;
				//8 存储1帧
				intToBuf((indexn-2),onefrmbuf);		//帧长度保存帧的数据长度
				onefrmbuf[seqnumindex] = recordnum;
				saveOneFrame(onefrmbuf,indexn,fr_fp);
				indexn = 2;
				indexn += initFrameHead(&onefrmbuf[indexn],class601d.reportdata.data.oad,class601d.reportdata.data.recorddata.rsd,10,class601d.reportdata.data.recorddata.csds,&seqnumindex);
				recordnum = 0;
			}
		}
	}
	if(framesum==0) {
		framesum = 1; //一帧
		fprintf(stderr,"\n indexn = %d saveOneFrame  seqnumindex=%d,  recordnum=%d!!!!!!!!!!!!!!!!\n",indexn,seqnumindex,recordnum);
//		asyslog(LOG_INFO,"任务数据文件组帧:indexn = %d , seqnumindex=%d,  recordnum=%d\n",indexn,seqnumindex,recordnum);
		intToBuf((indexn-2),onefrmbuf);
		onefrmbuf[seqnumindex] = recordnum;
		saveOneFrame(onefrmbuf,indexn,fr_fp);
	}
	else {
		if(recordnum != 0)
		{
			framesum++;
			fprintf(stderr,"\n last frm indexn = %d saveOneFrame  seqnumindex=%d,  recordnum=%d!!!!!!!!!!!!!!!!\n",indexn,seqnumindex,recordnum);
			asyslog(LOG_INFO,"任务数据文件组帧:indexn = %d , seqnumindex=%d,  recordnum=%d\n",indexn,seqnumindex,recordnum);
			intToBuf((indexn-2),onefrmbuf);
			onefrmbuf[seqnumindex] = recordnum;
			saveOneFrame(onefrmbuf,indexn,fr_fp);
		}
	}
	fprintf(stderr,"\n帧总数:%d\n",framesum);
	*saveflg = 1;
	if(tk_fp!=NULL)
		fclose(tk_fp);
	if(fr_fp!=NULL)
		fclose(fr_fp);
	if(tsa_group != NULL)
		free(tsa_group);
	if(headunit!=NULL)
		free(headunit);
}
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
    if (strcmp("report", argv[1]) == 0) {
         JProgramInfo = OpenShMem("ProgramInfo", sizeof(ProgramInfo), NULL);
		 int retaskid = 64;
		 TS ts_rep1,ts_rep2;
		 if (argc >= 3) {
			 if(argc == 10)
			 {
				 retaskid = atoi(argv[2]);
				 TSGet(&ts_rep1);
				 ts_rep1.Year = atoi(argv[3]);
				 ts_rep1.Month = atoi(argv[4]);
				 ts_rep1.Day = atoi(argv[5]);
				 ts_rep1.Hour = atoi(argv[6]);
				 ts_rep1.Minute = atoi(argv[7]);
				 memcpy(&ts_rep2,&ts_rep1,sizeof(TS));
				 ts_rep2.Hour = atoi(argv[8]);
				 ts_rep2.Minute = atoi(argv[9]);
			 }
			 else {
 				 fprintf(stderr,"\n参数不够，格式如下:cj report 64 2017 6 6 10 30 11 30 **上报任务17-6-6 10:30到11:30这个点的数据上报\n");
 				 return;
			 }
		 }
		 fprintf(stderr, "retaskid=%d\n", retaskid);
		 bu_report(ts_rep1,ts_rep2,retaskid,&JProgramInfo->cfg_para.extpara[0]);
//		 tk_fp = opendatafile(taskid,recinfo,taskinfoflg);
//		 CLASS_601D class601d = {};
//		 if (readCoverClass(0x601D, taskid, &class601d, sizeof(CLASS_601D), coll_para_save) == 1) {
//			 ret = GetReportData(class601d);
//		 }
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
    	TSA *tsa_group = NULL;
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
    		ms.ms.type[0].begin[1] = 96;
    		ms.ms.type[0].end[0] = dtunsigned;
    		ms.ms.type[0].end[1] = 111;
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
    	tsa_num = getTsas(ms,(INT8U **)&tsa_group);
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
    	}
    	if(tsa_group != NULL)
    		free(tsa_group);

    	return EXIT_SUCCESS;
    }
}
