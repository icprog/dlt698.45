/*
 * data.c
 *
 *  Created on: Apr 8, 2017
 *      Author: ava
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
//void ReadNorData(TS ts,INT8U taskid,INT8U *tsa)
//{
//	FILE *fp  = NULL;
//	INT16U headlen=0,unitlen=0,unitnum=0,readlen=0;
//	INT8U *databuf_tmp=NULL;
//	INT8U *headbuf=NULL;
//	int i=0,j=0;
//	int savepos=0;
//	TS ts_now;
//	TSGet(&ts_now);
//	char	fname[128]={};
//	TASKSET_INFO tasknor_info;
//	if(ReadTaskInfo(taskid,&tasknor_info)!=1)
//		return;
//	getTaskFileName(taskid,ts_now,fname);
//	fp = fopen(fname,"r");
//	if(fp == NULL)//文件没内容 组文件头，如果文件已存在，提取文件头信息
//	{
//		fprintf(stderr,"\nopen file %s fail\n",fname);
//		return;
//	}
//	ReadFileHeadLen(fp,&headlen,&unitlen);
//	headbuf = (INT8U *)malloc(headlen);
//	unitnum = (headlen-4)/sizeof(HEAD_UNIT);
//	ReadFileHead(fp,headlen,unitlen,unitnum,headbuf);
//	databuf_tmp = malloc(unitlen);
//	fseek(fp,headlen,SEEK_SET);//跳过文件头
//	while(!feof(fp))//找存储结构位置
//	{
//		//00 00 00 00 00 00 00 00 07 05 00 00 00 00 00 01
//		//00 00 00 00 00 00 00 00 00 07 05 00 00 00 00 00 01
//		readlen = fread(databuf_tmp,unitlen,1,fp);
//		fprintf(stderr,"\n----读出来的长度readlen=%d,块大小unitlen=%d\n",readlen,unitlen);
//		if(readlen == 0)
//			break;
//		fprintf(stderr,"\n");
//		fprintf(stderr,"\n传进来的TSA：");
//		for(i=0;i<TSA_LEN;i++)
//			fprintf(stderr," %02x",tsa[i]);
//		fprintf(stderr,"\n文件里的TSA：");
//		for(i=0;i<TSA_LEN;i++)
//			fprintf(stderr," %02x",databuf_tmp[i+1]);
//		if(memcmp(&databuf_tmp[1],&tsa[0],17)==0)//找到了存储结构的位置，一个存储结构可能含有unitnum个单元
//		{
//			savepos=ftell(fp)-unitlen;//对应的放到对应的位置
//			break;
//		}
//	}
//	fprintf(stderr,"\n----(pos:%d):\n",savepos);
//	if(savepos==0)//没找到的不打印
//		return;
//	fprintf(stderr,"\n要求的tsa:");
//	for(j=0;j<17;j++)
//		fprintf(stderr,"%02x ",tsa[j]);
//	fprintf(stderr,"\n文件中的数据(pos:%d):",savepos);
//	fseek(fp,savepos,SEEK_SET);//
//	fread(databuf_tmp,unitlen,1,fp);
////		for(j=0;j<unitlen;j++)
////		{
////			fprintf(stderr,"%02x ",databuf_tmp[j]);
////		}
//	fprintf(stderr,"\n---存储单元数：%d 每个单元长度:%d\n",tasknor_info.runtime,unitlen/tasknor_info.runtime);
//	fprintf(stderr,"\n文件%s存储的数据(%d)：",fname,unitlen);
//	fprintf(stderr,"\n");
//
//	for(i=0;i<unitlen;i++)
//	{
//		if(i%(unitlen/tasknor_info.runtime) == 0)
//			fprintf(stderr,"\n%d:",i/(unitlen/tasknor_info.runtime));
//
//		fprintf(stderr," %02x",databuf_tmp[i]);
//	}
//
//}
//void cjread(int argc, char *argv[])
//{
//	INT8U tsa[17];
//	INT8U taskid=0;
//	TS ts_now;
//	int len=0,i=0,tmp=0;
//	fprintf(stderr,"\nargc =%d",argc);
//	if (argc>3)
//	{
//		len = argc - 3;//cj test 05 02 23 34 54 67
//		sscanf(argv[2],"%d",&tmp);
//		if(tmp >= 256)
//			return;
//		if (len>18)
//			return;
//		taskid = tmp;
//		memset(tsa,0,17);
//		for(i=0;i<len;i++)
//		{
//			sscanf(argv[i+3],"%02x",&tmp);
//			tsa[i] = (INT8U)tmp;
//		}
//		TSGet(&ts_now);
//		ReadNorData(ts_now,taskid,tsa);
//	}
//}
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
		fprintf(stderr,"\nunit_pos:%d\n",ftell(fp));
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
				fprintf(stderr,"\n\n\n>>>>查找到相关的TSA数据(%d)\n",ftell(fp));
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

void printBuf(INT8U* buf, INT32U len)
{
	INT32U i;
	for(i=0; i<len; i++)
		fprintf(stderr, "%02X ", buf[i]);

	fprintf(stderr, "\n");
}

void getFrmCS(int argc, char* argv[])
{
	INT8U buf[4096] = {0};
	INT32U bufLen = sizeof(buf);

	readFrm(argv[2], buf, &bufLen);
	printBuf( buf, bufLen);
	fprintf(stderr, "%02X\n", getCS645(buf, bufLen));
}

void getFrmFCS(int argc, char* argv[])
{
	INT8U buf[4096] = {0};
	INT32U bufLen = sizeof(buf);
	INT16U fcs = 0;

	readFrm(argv[2], buf, &bufLen);
	printBuf( buf, bufLen);
	fcs = tryfcs16(buf, bufLen);
	fprintf(stderr, "%02X %02X\n", (INT8U)fcs, (INT8U)(fcs>>8));
}

void analyFreezeData(int argc, char* argv[])
{
	OI_698 freezeOI,relateOI;
	int		tmp=0;
	DateTimeBCD datetime;
	int	 datalen,currRecordNum,maxRecordNum,i=0,j=0,ret=0;
	INT8U data[1024];
	OAD	 	oad;

	if(argc==4) {
		sscanf(argv[2],"%04x",&tmp);
		freezeOI = tmp;
		sscanf(argv[3],"%04x",&tmp);
		relateOI = tmp;
		fprintf(stderr,"freezeOI=%04x relateOI=%04x\n",freezeOI,relateOI);
		memset(&datetime,0,sizeof(DateTimeBCD));
		readFreezeRecordNum(freezeOI,relateOI,&currRecordNum,&maxRecordNum);
		fprintf(stderr,"File=%s/%04x-%04x.dat  currRecord=%d,maxRecord=%d\n",VARI_DIR,freezeOI,relateOI,currRecordNum,maxRecordNum);
		oad.OI = relateOI;
		oad.attflg = 0;
		oad.attrindex = 0;
		for(i=0;i<maxRecordNum;i++) {
			memset(data,0,sizeof(data));
			datalen = 0;
			ret = readFreezeRecordByNum(freezeOI,oad,i,&datetime,&datalen,(INT8U *)data);
			if(ret==1) {
				fprintf(stderr,"序号:%d  冻结时间:%04d-%02d-%02d %02d:%02d:%02d ",i,datetime.year.data,datetime.month.data,datetime.day.data,
						datetime.hour.data,datetime.min.data,datetime.sec.data);
				fprintf(stderr,"数据【%d】 ",datalen);
				for(j=0;j<datalen;j++) {
					fprintf(stderr,"%02x ",data[j]);
				}
				fprintf(stderr,"\n");
			}
		}
	}
	if(argc==7) {
		sscanf(argv[2],"%04x",&tmp);
		freezeOI = tmp;
		sscanf(argv[3],"%04x",&tmp);
		relateOI = tmp;
		datetime.year.data = atoi(argv[4]);
		datetime.month.data = atoi(argv[5]);
		datetime.day.data = atoi(argv[6]);
		datetime.hour.data = 0;
		datetime.min.data = 0;
		datetime.sec.data = 0;
		oad.OI = relateOI;
		oad.attflg = 0;
		oad.attrindex = 0;
		fprintf(stderr,"find freezeOI=%04x relateOI=%04x\n",freezeOI,relateOI);
		ret = readFreezeRecordByTime(freezeOI,oad,datetime,&datalen,(INT8U *)data);
		if(ret==1) {
			fprintf(stderr,"冻结时间:%04d-%02d-%02d %02d:%02d:%02d ",datetime.year.data,datetime.month.data,datetime.day.data,
					datetime.hour.data,datetime.min.data,datetime.sec.data);
			fprintf(stderr,"数据【%d】 ",datalen);
			for(j=0;j<datalen;j++) {
				fprintf(stderr,"%02x ",data[j]);
			}
			fprintf(stderr,"\n");
		}
	}

	return ;
}

