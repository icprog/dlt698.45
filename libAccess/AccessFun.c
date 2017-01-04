#include <stdio.h>
#include "errno.h"
#include <time.h>
#include <dirent.h>
#include <ctype.h>
#include <strings.h>
#include <termios.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <fcntl.h>
#include <dirent.h>
#include <syslog.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "AccessFun.h"
#include "StdDataType.h"
#include "ObjectAction.h"

#define LIB_ACCESS_VER 0x0001
#define MAX_POINT_NUM 1200
#define MP_INFO_DIR "/nand/para/mp_info.par"

typedef struct//集合接口类
{
	INT8U logic_name[OCTET_STRING_LEN];//逻辑名
	INT16U curr_num;//当前元素个数
	INT16U max_num;//最大元素个数
}COLL_CLASS;
//typedef struct
//{
//	TSA addr;			//通信地址
//	INT8U baud;			//波特率
//	INT8U protocol;		//规约类型
//	OAD port;			//端口
//	INT8U pwd[OCTET_STRING_LEN];	//通信密码
//	INT8U ratenum;		//费率个数
//	INT8U usrtype;		//用户类型
//	INT8U connectype;	//接线方式
//	INT16U ratedU;		//额定电压
//	INT16U ratedI;		//额定电流
//}BASIC_OBJECT;
//typedef struct
//{
//	TSA cjq_addr;		//采集器地址
//	INT8U asset_code[OCTET_STRING_LEN];		//资产号
//	INT16U pt;
//	INT16U ct;
//}EXTEND_OBJECT;
//typedef struct
//{
//	OAD oad;
//	INT8U data[OCTET_STRING_LEN];
//}ANNEX_OBJECT;
//
//typedef struct//集合
//{
//	INT8U name[50];
//	INT16U sernum;
//	BASIC_OBJECT basicinfo;
//	EXTEND_OBJECT extinfo;
//	ANNEX_OBJECT aninfo;
//}CLASS_6001;//采集档案配置表对象




/***************************************
 * 当前数据存储函数
 * mtype:	表计类型
 * id:		数据标识
 * data:	数据
 * len:		数据长度
 * 返回值: 	1
 ***************************************/
unsigned short SetMCurrentData(int mtype,int id,unsigned char* data,int len)
{
	return 0;
}
/***************************************
 * 历史数据存储函数
 * mtype:	表计类型
 * id:		数据标识
 * year,month,day 数据日期
 * data:	数据
 * len:		数据长度
 * 返回值: 	1
 ***************************************/
unsigned short SetMHisData(int mtype,int id,int year,int month,int day,unsigned char* data,int len)
{
	return 0;
}
/***************************************
 * 表计参数存储函数
 * mtype:	表计类型
 * id:		数据标识
 * data:	数据
 * len:		数据长度
 * 返回值: 	1
 ***************************************/
unsigned short SetMPara(int mtype,int id,unsigned char* data,int len)
{
	return 0;
}


/***************************************
 * 当前数据访问函数
 * mtype:	表计类型
 * id:		数据标识
 * data:	数据
 * len:		数据长度
 * 返回值: 	1
 ***************************************/
unsigned short GetMCurrentData(int mtype,int id,unsigned char* data,int len)
{
	return 0;
}
/***************************************
 * 历史数据访问函数
 * mtype:	表计类型
 * id:		数据标识
 * year,month,day 数据日期
 * data:	数据
 * len:		数据长度
 * 返回值: 	1
 ***************************************/
unsigned short GetMHisData(int mtype,int id,int year,int month,int day,unsigned char* data,int len)
{
	return 0;
}
/***************************************
 * 表计参数访问函数
 * mtype:	表计类型
 * id:		数据标识
 * data:	数据
 * len:		数据长度
 * 返回值: 	1
 ***************************************/
unsigned short GetMPara(int mtype,int id,unsigned char* data,int len)
{
	return 0;
}
/***************************************
 * 表计参数存储测量点函数
 * id:		数据标识
 * data:	数据
 * 返回值: 	4:r+打开配置文件失败 3: w+打开配置文件失败 2: 1:长度不对 0：添加测量点成功
 ***************************************/
unsigned short SaveMPoint(int id,unsigned char* data,int len)
{
	CLASS_6001 meter_info;
	CLASS_6001 meter_file;
	COLL_CLASS coll_info;
	INT8U cld_flg=0;
	INT16U pnum=0,i=0,addnum=0;
	FILE *fp_mp = NULL;
	fprintf(stderr,"\n-------------3---len=%d---%d\n",len,sizeof(CLASS_6001));
	for(i=0;i<len;i++)
	{
		fprintf(stderr," %02x",data[i]);
	}
	fprintf(stderr,"\n");
	if(len%(sizeof(CLASS_6001)) != 0)
	{
		fprintf(stderr,"\n-------------return 1\n");
		return 1;
	}
	pnum = len/(sizeof(CLASS_6001));//本次设置的个数
	fprintf(stderr,"\n-------------4---pnum=%d\n",pnum);
	if(access(MP_INFO_DIR,F_OK)!=0)
	{
		fprintf(stderr,"\n-------------5\n");
		fp_mp = fopen(MP_INFO_DIR,"w+");//创建文件
		if(fp_mp != NULL)
		{
			fprintf(stderr,"\n-------------6\n");
			strcpy((char *)coll_info.logic_name,"采集档案表");
			coll_info.curr_num = 0;
			coll_info.max_num = MAX_POINT_NUM;
//			rewind(fp_mp);
			//定位到最大元素个数成员位置
			fseek(fp_mp,MAX_POINT_NUM*sizeof(CLASS_6001),SEEK_SET);
//			fwrite(s->outbuf, 1, Z_BUFSIZE, s->file)
			fprintf(stderr,"\n--111--coll_info---%s--%d--%d\n",(char *)coll_info.logic_name,
					coll_info.curr_num,coll_info.max_num);
			fwrite(&coll_info,sizeof(INT16U)*2 + OCTET_STRING_LEN,1,fp_mp);//改变结构体，名称和个数放最后
			fclose(fp_mp);
		}
		else
			return 3;
	}
	fprintf(stderr,"\n-------------7\n");
	fp_mp = fopen(MP_INFO_DIR,"r+");//替换模式
	if(fp_mp != NULL)
	{
		for(i=0;i<pnum;i++)
		{
			memset(&meter_info,0x00,sizeof(CLASS_6001));
			memset(&meter_file,0x00,sizeof(CLASS_6001));
			memcpy(&meter_info,(CLASS_6001 *)(data+sizeof(CLASS_6001)*i),sizeof(CLASS_6001));
			fprintf(stderr,"\n--1111--sernum=%04d:%d--%d \n",meter_info.sernum,
					meter_info.basicinfo.baud,meter_info.basicinfo.protocol);
			if(meter_info.sernum > MAX_POINT_NUM || meter_info.sernum == 0)
				continue;//超限或者序号为0，则不添加
			//定位到成员位置
			fseek(fp_mp,(meter_info.sernum-1) * sizeof(CLASS_6001),SEEK_SET);//sernum从1计数
			fread(&meter_file,sizeof(CLASS_6001),1,fp_mp);
			if(meter_file.sernum != 0)
			{
				fprintf(stderr,"\n---替换测量点%d,%d\n",meter_file.sernum,addnum);
				cld_flg = 1;//不是添加，是替换
			}
			else
			{
				fprintf(stderr,"\n---添加测量点%d,%d\n",meter_file.sernum,addnum);
				addnum++;//本次增加测量点个数
			}
			fwrite(&meter_info,sizeof(CLASS_6001),1,fp_mp);
		}

		//定位到测量点计数位置
		fseek(fp_mp,MAX_POINT_NUM * sizeof(CLASS_6001) + OCTET_STRING_LEN,SEEK_SET);//sernum从1计数
		fread(&coll_info.curr_num,sizeof(INT16U)*2,1,fp_mp);
		fprintf(stderr,"\n-1-当前计数和最大值为--%d-%d\n",coll_info.curr_num,coll_info.max_num);
		if(cld_flg==0)
			coll_info.curr_num += addnum;
		fwrite(&coll_info.curr_num,sizeof(INT16U)*2,1,fp_mp);
		fprintf(stderr,"\n-2-当前计数和最大值为--%d-%d\n",coll_info.curr_num,coll_info.max_num);
#if 1
		fread(&coll_info.curr_num,sizeof(INT16U)*2,1,fp_mp);
		fprintf(stderr,"\n-3-当前计数和最大值为--%d-%d\n",coll_info.curr_num,coll_info.max_num);
#endif
		fclose(fp_mp);
	}
	else
		return 4;
	return 0;
}
/***************************************
 * 打印表计参数函数：test
 * mtype:	表计类型
 * id:		数据标识
 * data:	数据
 * len:		数据长度
 * 返回值: 	1
 ***************************************/
unsigned short prtmp_info()
{
	CLASS_6001 meter_file;
	COLL_CLASS coll_info;
	FILE *fp_mp = NULL;
	INT16U i=0;
	fprintf(stderr,"\n\n");
	fp_mp = fopen(MP_INFO_DIR,"r");//只读模式
	if(fp_mp != NULL)
	{
		fseek(fp_mp,MAX_POINT_NUM * sizeof(CLASS_6001) + OCTET_STRING_LEN,SEEK_SET);//sernum从1计数
		fread(&coll_info.curr_num,sizeof(INT16U)*2,1,fp_mp);
		fprintf(stderr,"\n-111-当前计数和最大值为--%d-%d\n",coll_info.curr_num,coll_info.max_num);
		for(i=0;i<MAX_POINT_NUM;i++)
		{
			//定位到成员位置
			memset(&meter_file,0x00,sizeof(CLASS_6001));
			fseek(fp_mp,i * sizeof(CLASS_6001),SEEK_SET);
			fread(&meter_file,sizeof(CLASS_6001),1,fp_mp);
//			if(i%10==0)
//				fprintf(stderr,"\n");
			if(meter_file.sernum !=0)
				fprintf(stderr,"\n%04d:sernum=%04d:%d%d \n",i+1,meter_file.sernum,
						meter_file.basicinfo.baud,meter_file.basicinfo.protocol);
//			else
//				fprintf(stderr," %04d:sernum=%04d",i+1,meter_file.sernum);
		}
	}
	fseek(fp_mp,MAX_POINT_NUM * sizeof(CLASS_6001),SEEK_SET);//sernum从1计数
	fread(&coll_info,sizeof(COLL_CLASS),1,fp_mp);
	fprintf(stderr,"\n--coll_info---%s--%d--%d\n",(char *)coll_info.logic_name,
			coll_info.curr_num,coll_info.max_num);
	fclose(fp_mp);
	return 0;
}
/***************************************
 * 表计参数存储函数
 * mtype:	表计类型
 * id:		数据标识
 * data:	数据
 * len:		数据长度
 * 返回值: 	1
 ***************************************/
unsigned short SaveMPara(int mtype,int id,unsigned char* data,int len)
{
	fprintf(stderr,"\n-------------2---id=%d\n",id);
	if(id==6000)
	{
//		prtmp_info();
		SaveMPoint(id,data,len);
	}
//	prtmp_info();
	return 0;
}
