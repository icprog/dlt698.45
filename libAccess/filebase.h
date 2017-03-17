/*
 * filebase.h
 *
 *  Created on: Jan 18, 2017
 *      Author: ava
 */

#ifndef FILEBASE_H_
#define FILEBASE_H_

#include "StdDataType.h"
#include "Objectdef.h"


#define 	CODE 					0xa001
#define 	FILENAMELEN				128			//文件名字最大长度
#define 	FILEEXTLEN				5			//文件扩展名最大长度


typedef enum{
	AddUpdate=1,		//添加及更新
	Delete,			//删除
	Clear,			//清空
}Method;



typedef struct
{
   INT16U 		oi;  							//对象标识OI
   INT16U		interface_len;					//接口类头文件长度
   INT16U		unit_len;						//配置单元长度
   INT16U		index_site;						//配置序号索引位置
   char			logic_name[OCTET_STRING_LEN];	//对象逻辑名字
   char			file_name[FILENAMELEN];			//对象保存名字
}CLASS_INFO;

const static CLASS_INFO  class_info[] ={
		{0x6000,sizeof(CLASS11),sizeof(CLASS_6001),OCTET_STRING_LEN,"6000","/nand/para/table6000.par"},		//采集档案配置表
};

/*　　变量类对象的数据存储文件结构
 * 	　每个对象标识OI占用64个字节
 * 	　根据vari_data表中定义各个OI在文件中存放的偏移位置offsit，用于数据的定位查找
 * 	　每个oi按照规约内容存放: 类型＋数据
 * */

typedef struct
{
	OI_698		oi;				//对象标识OI
	int			offset;			//文件中编译位置
}Variable_Class;

const static  Variable_Class vari_data[] ={
		{0x2000,0}, {0x2001,1}, {0x2002,2}, {0x2003,3}, {0x2004,4}, {0x2005,5}, {0x2006,6}, {0x2007,7}, {0x2008,8}, {0x2009,9},
		{0x200A,10},{0x200B,11},{0x200C,12},{0x200D,13},{0x200E,14},{0x200F,15},{0x2010,16},{0x2011,17},{0x2012,18},{0x2013,19},
		{0x2014,20},{0x2017,21},{0x2018,22},{0x2019,23},{0x201A,24},{0x201B,25},{0x201C,26},{0x2026,27},{0x2027,28},{0x2028,29},
		{0x2029,30},{0x202A,31},{0x202C,32},{0x202D,33},{0x202E,34},{0x2031,35},{0x2032,36},{0x2040,37},{0x2041,38},{0x2031,39},
		{0x2032,40},{0x2033,41},{0x2140,42},{0x2141,43},{0x2200,44},{0x2203,45},{0x2204,46},
};

typedef struct
{
	OI_698		oi;				//对象标识OI
	int			offset;			//文件中编译位置
	int			datalen;		//每个类数据长度
}Variable_TJ_Class;
/*1个TSA的数据偏移字节数，
 * */
#define TJ_TSA_LEN		608		//1个TSA占用的字节数
const static  Variable_TJ_Class vari_tj_data[] ={
		{0x2131,0,64},//当月A相电压合格率			64个字节
		{0x2132,1,64},//当月B相电压合格率
		{0x2133,2,64},//当月C相电压合格率
		{0x2140,3,64},//日最大有功功率及发生时间		64个
		{0x2141,4,64},//月最大有功功率及发生时间
//		{0x2100,176},//分钟区间统计	1440点*12个字节＝17280		文件太大，考虑单独放置
		{0x2101,5,288},//小时区间统计	24*12=288
};

typedef struct{
	OAD   oad_m;	//主OAD		oad 不存在主OAD，填0 ，road  存在主oad，如50040200
	OAD   oad_r;	//关联OAD
	INT16U len;		//oad 的数据长度，包括类型描述
}HEAD_UNIT;

typedef struct{
	OAD    oad_m;
	OAD    oad_r;
	int    offset;
	INT16U len;
}OAD_INDEX;//oad索引
/*
 * 该文件定义的函数为libAccess接口库内部使用函数。不是对外接口
 * */
extern INT8U writeInterClass(char *file_name,void *dest,int size);
extern void WriteInterfaceClass(OI_698 oi,INT16U seqnum,INT8U method);
extern int WriteClass11(OI_698 oi,INT16U seqnum,INT8U method);
extern INT8U writeCoverFile(char *fname, void *dataunit,int len);
extern int readCoverFile(char *fname, void *dataunit,int len);

extern void makeEventDir(OI_698 oi);
extern void makeSubDir(char *DirName);

extern void getFileName(OI_698 oi,INT16U seqno,INT16U type,char *fname);
extern int  readFileName(OI_698 oi,INT16U seqno,INT16U type,char *fname);
extern long getFileRecordNum(OI_698 oi);
extern long getFileLen(char *filename);
extern INT16S getclassinfo(OI_698 oi,CLASS_INFO *classinfo);

extern INT8U block_file_sync(char *fname,void *blockdata,int size,int headsize,int index);
extern INT8U save_block_file(char *fname,void *blockdata,int size,int headsize,int index);

extern int getvarioffset(OI_698 oi,int coll_seqnum,int *offset,int *blklen);

extern sem_t * InitSem();
extern void CloseSem(sem_t * sem_parasave);

#endif /* FILEBASE_H_ */
