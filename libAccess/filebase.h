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
		{0x6000,sizeof(COLL_CLASS_11),sizeof(CLASS_6001),OCTET_STRING_LEN,"6000","/nand/para/table6000.par"},		//采集档案配置表
		{0x6000,sizeof(COLL_CLASS_11),sizeof(CLASS_6001),OCTET_STRING_LEN,"6000","/nand/para/table6000.par"},		//交采
//		{0x6013,0,sizeof(CLASS_6013),0,"6013","/nand/para/table6013.par"},		//任务配置单元
};


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
extern long getFileRecordNum(OI_698 oi);
extern long getFileLen(char *filename);
extern INT16S getclassinfo(OI_698 oi,CLASS_INFO *classinfo);

extern INT8U block_file_sync(char *fname,void *blockdata,int size,int headsize,int index);
extern INT8U save_block_file(char *fname,void *blockdata,int size,int headsize,int index);

#endif /* FILEBASE_H_ */
