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
		fprintf(stderr,"tmp = %x \n",tmp);
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
			fseek(fp,begitoffset+A_TSAblock+TSA_LEN+1,0);
		}else
		{
			fseek(fp,begitoffset+TSA_LEN+1,0);
			break;
		}
	}
	return findok;
}
void record_prt(int recordnum,int indexn,HEAD_UNIT *length,FILE *fp)
{
	int	nonullflag=0;
	int k=0,i=0,j=0;
	int	nullbuf[50]={};
	INT8U buf[50]={};

	memset(&nullbuf,0,sizeof(nullbuf));
	for(k=0;k<recordnum;k++)
	{
		nonullflag=0;
//		fprintf(stderr,"\nunit_pos:%d\n",ftell(fp));
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
//void recordinfo_prt(int recordnum,int recordsize,FILE *fp)
//{
//	int i=0,j=0,k=0,oknum=0;
//	int	nullbuf[50]={};
//	INT8U *blockbuf = NULL;
//	INT32U blocksize = recordnum*recordsize;
//
//	blockbuf = malloc(blocksize);
//	memset(&nullbuf,0,sizeof(nullbuf));
////	fprintf(stderr,"序号:");
////	for(i=0;i<recordnum;i++)
////		fprintf(stderr,"%02d ",i+1);
//	while(!feof(fp))
//	{
//		oknum = 0;
//		for(k=0;k<4;k++)
//		{
//			for(i=0;i<recordnum/4;i++)
//			{
//				memset(buf,0,recordsize);
//				if (fread(buf,recordsize,1,fp)>0)
//				{
//					if(i==0)
//					{
//						fprintf(stderr,"TSA:");
//						for(j=0;j<buf[1];j++)
//							fprintf(stderr,"%02x ",buf[j+2]);
//						fprintf(stderr,":");
//						for(j=0;j<10;j++)
//							fprintf(stderr,"%02d ",j+1);
//						fprintf(stderr,"\n                      00:");
//					}
//					if(memcmp(&buf[18],nullbuf,8)!=0)
//					{
//						oknum++;
//						fprintf(stderr," * ");
//					}
//					else
//						fprintf(stderr," - ");
//				}
//				else
//				{
//					if(buf != NULL)
//						free(buf);
//					return;
//				}
//			}
//			fprintf(stderr,"\n数据点数%d\n",oknum);
//		}
//	}
//	if(buf != NULL)
//		free(buf);
//	return ;
//}
void recordinfo_prt(int recordnum,int recordsize,FILE *fp)
{
	int i=0,j=0,k=0,oknum=0;
	int	nullbuf[50]={};
	INT8U *blockbuf = NULL;
	INT32U blocksize = recordsize*recordnum;

	blockbuf = malloc(blocksize);
	memset(&nullbuf,0,sizeof(nullbuf));
//	fprintf(stderr,"序号:");
//	for(i=0;i<recordnum;i++)
//		fprintf(stderr,"%02d ",i+1);

	while(!feof(fp))
	{
		memset(blockbuf,0,blocksize);
		if (fread(blockbuf,blocksize,1,fp)>0)
		{
			fprintf(stderr,"\n   测量点地址 TSA:");
			for(i=0;i<blockbuf[1];i++)
				fprintf(stderr,"%02x ",blockbuf[i+2]);
			fprintf(stderr,"\n");
			oknum=0;
			fprintf(stderr,"  ");
			for(j=0;j<24;j++)
				fprintf(stderr," %02d",j);
			fprintf(stderr,"\n");
			for(k=0;k<4;k++)
			{
				fprintf(stderr,"%02d",k*15);
				for(j=0;j<recordnum/4;j++)
				{
					if(blockbuf[(4*j+k)*recordsize+18]==0)//时标不为空
						fprintf(stderr,"   ");
					else
					{
						oknum++;
						fprintf(stderr," o ");
					}
				}
				fprintf(stderr,"\n");
			}
			fprintf(stderr,"   记录总数:%d  成功总数:%d\n\n",recordnum,oknum);
		}
		else
			break;
	}
	if(blockbuf != NULL)
		free(blockbuf);
}
void recordoadinfo_prt(int recordnum,int unitnum,int recordsize,HEAD_UNIT0 *length,FILE *fp)
{
	int i=0,j=0,k=0,oknum=0;
	int	nullbuf[50]={};
	INT8U *blockbuf = NULL;
	INT32U blocksize = recordsize*recordnum;
	int offset = 0;

	blockbuf = malloc(blocksize);
	memset(&nullbuf,0,sizeof(nullbuf));
//	fprintf(stderr,"序号:");
//	for(i=0;i<recordnum;i++)
//		fprintf(stderr,"%02d ",i+1);

	while(!feof(fp))
	{
		memset(blockbuf,0,blocksize);
		if (fread(blockbuf,blocksize,1,fp)>0)
		{
			offset = 0;
			fprintf(stderr,"\n   测量点地址 TSA:---------------------------------------------------------------");
			for(i=0;i<blockbuf[1];i++)
				fprintf(stderr,"%02x ",blockbuf[i+2]);
			fprintf(stderr,"\n");
			for(i=0;i<unitnum;i++)
			{
				if((length[i].oad_r.OI==0x202a) || (length[i].oad_r.OI==0x6040) || (length[i].oad_r.OI==0x6041) || (length[i].oad_r.OI==0x6042))//找第一个不是固定格式的数据项
				{
					offset += length[i].len;
					continue;
				}
				fprintf(stderr,"   %04x%02x%02x-%04x%02x%02x\n",length[i].oad_m.OI,length[i].oad_m.attflg,length[i].oad_m.attrindex,
						length[i].oad_r.OI,length[i].oad_r.attflg,length[i].oad_r.attrindex);
				oknum=0;
				fprintf(stderr,"  ");
				for(j=0;j<24;j++)
					fprintf(stderr," %02d",j);
				fprintf(stderr,"\n");
				for(k=0;k<4;k++)
				{
					fprintf(stderr,"%02d",k*15);
					for(j=0;j<recordnum/4;j++)
					{
						if(blockbuf[offset+(4*j+k)*recordsize]==0)
							fprintf(stderr,"   ");
						else
						{
							oknum++;
							fprintf(stderr," o ");
						}
					}
					fprintf(stderr,"\n");
				}
				offset += length[i].len;
				fprintf(stderr,"   记录总数:%d  成功总数:%d\n\n",recordnum,oknum);
			}
		}
		else
			break;
	}
	if(blockbuf != NULL)
		free(blockbuf);
}


void setm2g(int argc, char* argv[])
{
	int m2g = 0;
	if(argc == 2){
		readCoverClass(0x4521, 0, &m2g, sizeof(int), para_vari_save);
		if(m2g == 666){
			fprintf(stderr, "当前模式为2G锁定模式\n");
		}else{
			fprintf(stderr, "当前模式为4G优先模式\n");
		}
	}else {
		int setm = 4;
		setm = atoi(argv[2]);
		if(setm == 4){
			m2g = 0;
			saveCoverClass(0x4521, 0, &m2g, sizeof(int), para_vari_save);
			fprintf(stderr, "设置为为4G优先模式\n");
		}else if (setm == 2){
			m2g = 666;
			saveCoverClass(0x4521, 0, &m2g, sizeof(int), para_vari_save);
			fprintf(stderr, "设置为为2G锁定模式\n");
		}
	}
}

void analyTaskData(int argc, char* argv[])
{
	int TSA_D[20]={};
	char *filename= argv[2];
	FILE *fp=NULL;
	int i=0;
	int tsanum=0 , head_len=0,haveTsa =0;
	HEADFIXED_INFO taskhead_info;
	HEAD_UNIT headoad_unit[FILEOADMAXNUM];


	if (filename!=NULL)
	{
		if(argc>3)
		{
			tsanum = argc -3;
			haveTsa = tsanum;   //人工输入的TSA目标字节数
			fprintf(stderr,"\n\n\n【 目标地址(%d): ",tsanum);
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
//			head_len = readfile_int(fp);
			fread(&taskhead_info,sizeof(HEADFIXED_INFO),1,fp);
			fread(headoad_unit,taskhead_info.oadnum*sizeof(HEAD_UNIT),1,fp);
			head_len = sizeof(HEADFIXED_INFO)+taskhead_info.oadnum*sizeof(HEAD_UNIT);


			fprintf(stderr,"\n文件头长度:%d (字节) 对象oad总数:%d 记录长度:%d 每日记录次数:%d 记录间隔:%d(秒)\n",
					head_len,taskhead_info.oadnum,taskhead_info.reclen,taskhead_info.seqnum,taskhead_info.seqsec);


			if (findtsa(fp,TSA_D,taskhead_info.reclen*taskhead_info.seqnum)==1)
			{
				fprintf(stderr,"\n\n\n>>>>查找到相关的TSA数据(%d)\n",ftell(fp));
				//打印TSA的全部记录
				record_prt(taskhead_info.seqnum,taskhead_info.oadnum,headoad_unit,fp);
			}
			else
				fprintf(stderr,"\n\n\n>>>>未查找到相关的TSA数据\n\n\n");
			fclose(fp);
		}
	}
	return ;
}
INT16U getTaskDataTsaNum(INT8U taskID)
{
	TS ts_tmp;
	TSGet(&ts_tmp);
	char	fname[128]={};
	getTaskFileName(taskID,ts_tmp,fname);//得到要抄读的文件名称
	fprintf(stderr,"\n打开文件名%s\n",fname);
	INT8U tmp=0,buf[20]={};
	int begitoffset =0 ;

	int indexn=0,A_record=0,A_TSAblock=0;
	HEAD_UNIT0 length[20];
	int tsaNum =0 , head_len=0,unitnum=0;


	FILE *fp=NULL;
	fp = fopen(fname,"r");
	if(fp==NULL)
		return 0;
	fprintf(stderr,"\n\n\n--------------------------------------------------------");
	head_len = readfile_int(fp);
	fprintf(stderr,"\n文件头长度 %d (字节)",head_len);

	A_TSAblock = readfile_int(fp);
	memset(&length,0,sizeof(length));
	unitnum = (head_len )/sizeof(HEAD_UNIT0);

	//打印文件头结构
	A_record = head_prt(unitnum,length,&indexn,fp);

	fprintf(stderr,"\nA_TSAblock = %d\n",A_TSAblock);
	for(;;)
	{
		begitoffset = ftell(fp);
		if (fread(&tmp,1,1,fp)<=0)
		{
			fprintf(stderr,"1111111");
			return tsaNum;
		}
		if(tmp!=0X55)
		{
			fprintf(stderr,"2222222");
			return tsaNum;
		}
		fread(&tmp,1,1,fp);
		fread(&buf,tmp,1,fp);

		tsaNum++;
		fseek(fp,begitoffset+A_TSAblock,0);

	}

	return tsaNum;

}
void analyTaskInfo(int argc, char* argv[])
{
	char *filename= argv[2];
	FILE *fp=NULL;
	int indexn=0,A_record=0,A_TSAblock=0;
	HEAD_UNIT0 length[20];
	int head_len=0,recordnum =0, unitnum=0;

	if (filename!=NULL)
	{
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

//			fprintf(stderr,"\n\n\n>>>>查找到相关的TSA数据(%d)\n",ftell(fp));
			//打印TSA的全部记录
//			fprintf(stderr,"\n文件指针位置%d\n",ftell(fp));
			recordinfo_prt(recordnum,A_record,fp);
			fclose(fp);
		}
	}
	return ;
}
void analyTaskOADInfo(int argc, char* argv[])
{
	char *filename= argv[2];
	FILE *fp=NULL;
	int indexn=0,A_record=0,A_TSAblock=0;
	HEAD_UNIT0 length[20];
	int head_len=0,recordnum =0, unitnum=0;

	if (filename!=NULL)
	{
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

//			fprintf(stderr,"\n\n\n>>>>查找到相关的TSA数据(%d)\n",ftell(fp));
			//打印TSA的全部记录
//			fprintf(stderr,"\n文件指针位置%d\n",ftell(fp));
			recordoadinfo_prt(recordnum,unitnum,A_record,length,fp);
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

