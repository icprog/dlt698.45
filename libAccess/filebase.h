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
		{0x8888,sizeof(TASK_INFO),sizeof(TASK_INFO),0,"8888","/nand/para/plcrecord.par"},		//采集档案配置表
};

/*　　变量类对象的数据存储文件结构
 * 	　每个对象标识OI占用64个字节
 * 	　根据vari_data表中定义各个OI在文件中存放的偏移位置offsit，用于数据的定位查找
 * 	　每个oi按照规约内容存放: 类型＋数据
 * */
typedef struct
{
	OI_698		oi;				//对象标识OI
	int			offset;			//文件中编译位置 ，数据长度固定为VARI_LEN
}Variable_Class;

const static  Variable_Class pulse_data[]={
		{0x0010,0},	//正向有功电能
		{0x0020,1},	//反向有功电能
		{0x0030,2},	//正向无功电能/组合无功1电能
		{0x0040,3},	//反向无功电能/组合无功2电能
};

const static  Variable_Class energy_data[]= {
		{0x0000,0}, {0x0010,1}, {0x0011,2}, {0x0012,3}, {0x0013,4}, {0x0020,5}, {0x0021,6}, {0x0022,7}, {0x0023,8}, {0x0030,9},
		{0x0031,10},{0x0032,11},{0x0033,12},{0x0040,13},{0x0041,14},{0x0042,15},{0x0043,16},{0x0050,17},{0x0051,18},{0x0052,19},
		{0x0053,20},{0x0060,21},{0x0061,22},{0x0062,23},{0x0063,24},{0x0070,25},{0x0071,26},{0x0072,27},{0x0073,28},{0x0080,29},
		{0x0081,30},{0x0082,31},{0x0083,32},{0x0090,33},{0x0091,34},{0x0092,35},{0x0093,36},{0x00A0,37},{0x00A1,38},{0x00A2,39},
		{0x00A3,40},{0x0110,41},{0x0111,42},{0x0112,43},{0x0113,44},{0x0120,45},{0x0121,46},{0x0122,47},{0x0123,48},{0x0210,49},
		{0x0211,50},{0x0212,51},{0x0213,52},{0x0220,53},{0x0221,54},{0x0222,55},{0x0223,56},{0x0300,57},{0x0301,58},{0x0302,59},
		{0x0303,60},{0x0400,61},{0x0401,62},{0x0402,63},{0x0403,64},{0x0500,65},{0x0501,66},{0x0502,67},{0x0503,68}
};

const static  Variable_Class demand_data[]= {
		{0x1010,0}, {0x1011,1}, {0x1012,2}, {0x1013,3}, {0x1020,4}, {0x1021,5}, {0x1022,6}, {0x1023,7}, {0x1030,8}, {0x1031,9},
		{0x1032,10},{0x1033,11},{0x1040,12},{0x1041,13},{0x1042,14},{0x1043,15},{0x1050,16},{0x1051,17},{0x1052,18},{0x1053,19},
		{0x1060,20},{0x1061,21},{0x1062,22},{0x1063,23},{0x1070,24},{0x1071,25},{0x1072,26},{0x1073,27},{0x1080,28},{0x1081,29},
		{0x1082,30},{0x1083,31},{0x1090,32},{0x1091,33},{0x1092,34},{0x1093,35},{0x10A0,36},{0x10A1,37},{0x10A2,38},{0x10A3,39},
		{0x1110,40},{0x1111,41},{0x1112,42},{0x1113,43},{0x1120,44},{0x1121,45},{0x1122,46},{0x1123,47},{0x1130,48},{0x1131,49},
		{0x1132,50},{0x1133,51},{0x1140,52},{0x1141,53},{0x1142,54},{0x1143,55},{0x1150,56},{0x1151,57},{0x1152,58},{0x1153,59},
		{0x1160,60},{0x1161,61},{0x1162,62},{0x1163,63},{0x1170,64},{0x1171,65},{0x1172,66},{0x1173,67},{0x1180,68},{0x1181,69},
		{0x1182,70},{0x1183,71},{0x1190,72},{0x1191,73},{0x1192,74},{0x1193,75},{0x11A0,76},{0x11A1,77},{0x11A2,78},{0x11A3,79}
};

const static  Variable_Class vari_data[] ={
		{0x2000,0}, {0x2001,1}, {0x2002,2}, {0x2003,3}, {0x2004,4}, {0x2005,5}, {0x2006,6}, {0x2007,7}, {0x2008,8}, {0x2009,9},
		{0x200A,10},{0x200B,11},{0x200C,12},{0x200D,13},{0x200E,14},{0x200F,15},{0x2010,16},{0x2011,17},{0x2012,18},{0x2013,19},
		{0x2014,20},{0x2017,21},{0x2018,22},{0x2019,23},{0x201A,24},{0x201B,25},{0x201C,26},{0x2026,27},{0x2027,28},{0x2028,29},
		{0x2029,30},{0x202A,31},{0x202C,32},{0x202D,33},{0x202E,34},{0x2031,35},{0x2032,36},{0x2040,37},{0x2041,38},{0x2031,39},
		{0x2032,40},{0x2033,41},{0x2200,42},{0x2203,43},{0x2204,44},
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
		{0x2140,3,128},//日最大有功功率及发生时间		64个
		{0x2141,4,128},//月最大有功功率及发生时间
//		{0x2100,176},//分钟区间统计	1440点*12个字节＝17280		文件太大，考虑单独放置
		{0x2101,5,288},//小时区间统计	24*12=288
};


/*
 * 该文件定义的函数为libAccess接口库内部使用函数。不是对外接口
 * */
extern INT8U fu_read_accoef(char *FileName, void *source,INT32U size);	//为了兼容3761的计量芯片系数内容
extern INT8U file_write_accoef(char *FileName, void *source, int size);	//为了兼容3761的计量芯片系数内容
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
