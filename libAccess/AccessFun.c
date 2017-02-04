#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <ctype.h>
#include <dirent.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "AccessFun.h"
#include "filebase.h"
#include "StdDataType.h"
#include "Objectdef.h"
#include "EventObject.h"
#include "ParaDef.h"


#define 	LIB_ACCESS_VER 			0x0001

CLASS_INFO	info={};

/*
 * 数据区初始化接口函数
 * 返回值 =0: 删除成功
 * =-1：删除失败
 * */
int dataInit()
{
    struct timeval start={}, end={};
    long  interval=0;

    fprintf(stderr,"数据区初始化\n");
	gettimeofday(&start, NULL);
	//事件类数据清除
	system("rm -rf /nand/event/record");
	system("rm -rf /nand/event/current");

	gettimeofday(&end, NULL);
	interval = 1000000*(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec);
    fprintf(stderr,"dataInit interval = %f(ms)\n", interval/1000.0);
	return 0;
}

/*
 * 通过配置序号删除配置单元
 * 输入参数：oi对象标识，seqnum:要删除的配置序号
 * 返回值：=1：配置单元删除成功
 * =-1:  未查找到OI类数据
 */
int delClassBySeq(OI_698 oi,void *blockdata,int seqnum)
{
	int 	ret=-1;
	INT16S	infoi=-1;

	infoi = getclassinfo(oi,&info);
	if(infoi == -1) {
		return -1;
	}
	if(class_info[infoi].interface_len!=0) {		//该存储单元内部包含的类的公共属性
		if(seqnum>0)
			WriteInterfaceClass(oi,seqnum,Delete);
	}
	if(blockdata==NULL) {
		blockdata = malloc(class_info[infoi].unit_len);
		if(blockdata!=NULL) {
			memset(blockdata,0,class_info[infoi].unit_len);
		}
	}
	ret = save_block_file((char *)class_info[infoi].file_name,blockdata,class_info[infoi].unit_len,class_info[infoi].interface_len,seqnum);
	free(blockdata);
	return ret;
}

/*
 * 方法：Reset()复位
 * 输入参数：oi对象标识
 * 返回值：=0：配置单元删除成功
 * =-1:  删除错误
 */
int resetClass(OI_698 oi)
{
	int		ret = -1;
	char	cmd[64]={};

	memset(cmd,0,sizeof(cmd));
	if(oi>=0x3000 && oi<=0x3fff) {
		sprintf(cmd,"rm -rf /%s/%04x",EVENT_PORP,oi);
		system(cmd);
		ret = 0;
	}
	return ret;
}

/*
 * 方法：Clean()清空
 * 输入参数：oi对象标识
 * 返回值：=0：配置单元删除成功
 * =-1:  删除错误
 */
int clearClass(OI_698 oi)
{
	INT16S	infoi=-1;
	int		ret = -1;
	char	fname2[FILENAMELEN]={};
	char	cmd[FILENAMELEN]={};

	infoi = getclassinfo(oi,&info);
	if(infoi==-1) {
		sprintf(cmd,"rm -rf /nand/para/%04x/",oi);
		system(cmd);
		return 1;
	}
	memset(fname2,0,sizeof(fname2));
	strncpy(fname2,class_info[infoi].file_name,strlen(class_info[infoi].file_name)-4);
	strcat(fname2,".bak");

	ret = unlink(class_info[infoi].file_name);
	ret = unlink(fname2);
	return ret;
}

/*
 * 方法：Delete() 删除一个配置单元
 * 输入参数：oi对象标识，id:索引
 * 返回值：=0：配置单元删除成功
 * =-1:  删除错误
 */
int deleteClass(OI_698 oi,INT8U id)
{
	char	cmd[FILENAMELEN]={};

	sprintf(cmd,"rm -rf /nand/para/%04x/%02d.par",oi,id);
	system(cmd);
	sprintf(cmd,"rm -rf /nand/para/%04x/%02d.bak",oi,id);
	system(cmd);
	return 1;
}

/*
 * 接口类公共属性读取
 * 输入参数：oi对象标识，blockdata：主文件块缓冲区，size:主文件尺寸，index:文件的存储索引位置
 * 返回值：
 * =1：文件读取成功   =0：文件读取失败   =-1:  未查找到OI类数据信息
 */
//TODO: 未读取备份文件的接口类内容进行判断
int	readInterClass(OI_698 oi,void *dest)
{
	FILE 	*fp=NULL;
	int		num = 0;
	INT16S	infoi=-1;
//	CLASS_INFO	info={};

	infoi = getclassinfo(oi,&info);
	if(infoi==-1) {
		return -1;
	}
	fp = fopen(class_info[infoi].file_name, "r");
	if (fp != NULL) {
		num=fread(dest,1 ,class_info[infoi].interface_len,fp);
		fclose(fp);
	}
	return num;
};
/*
 * 参数类存储
 * 输入参数：oi对象标识，blockdata：主文件块缓冲区，seqnum:对象配置单元序列号，作为文件位置索引
 * 返回值：=1：文件保存成功，=0，文件保存失败，此时建议产生ERC2参数丢失事件通知主站异常
 * =-1:  未查找到OI类数据
 */

int saveParaClass(OI_698 oi,void *blockdata,int seqnum)
{
	int 	ret=-1;
	INT16U	infoi=-1;

	infoi = getclassinfo(oi,&info);
	if(infoi == -1) {
		return -1;
	}
	if(class_info[infoi].interface_len!=0) {		//该存储单元内部包含的类的公共属性
		WriteInterfaceClass(oi,seqnum,AddUpdate);
	}
	save_block_file((char *)class_info[infoi].file_name,blockdata,class_info[infoi].unit_len,class_info[infoi].interface_len,seqnum);
	return ret;
}

/*
 * 根据OI、配置序号读取某条配置单元内容
 * 输入参数：oi对象标识，seqnum:对象配置单元序列号
 * 返回值：
 * =1：文件读取成功，blockdata：配置单元内容
 * =0： 文件读取失败
 * =-1:  未查找到OI类数据
 */
int  readParaClass(OI_698 oi,void *blockdata,int seqnum)
{
	int 	ret=-1;
	INT16U	infoi=-1;

	infoi = getclassinfo(oi,&info);
	if(infoi==-1) {
		return -1;
	}
	ret = block_file_sync((char *)class_info[infoi].file_name,blockdata,class_info[infoi].unit_len,class_info[infoi].interface_len,seqnum);
	return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////

/*
 * 输入参数：	oi:对象标识，seqno:记录序号，blockdata:存储数据，savelen：存储长度，
 * 			type：存储类型【	根据宏定义SaveFile_type 】
 * 返回值：=1：文件存储成功
 */
int saveCoverClass(OI_698 oi,INT16U seqno,void *blockdata,int savelen,int type)
{
	int		ret = 0;
	char	fname[FILENAMELEN]={};

	memset(fname,0,sizeof(fname));
	getFileName(oi,seqno,type,fname);
	switch(type) {
	case event_para_save:
	case coll_para_save:
	case acs_coef_save:
	case acs_energy_save:
//		fprintf(stderr,"saveEventClass file=%s ",fname);
		ret = save_block_file(fname,blockdata,savelen,0,0);
		break;
	case event_record_save:
	case event_current_save:
		writeCoverFile(fname,blockdata,savelen);
		break;
	}
	return ret;
}

/*
 * 输入参数：	oi:对象标识，seqno:记录序号，
 * 			type：存储类型【	根据宏定义SaveFile_type 】
 * 返回值：相关对象标识的类的存储文件长度
 * =-1: 无效数据
 */
int getClassFileLen(OI_698 oi,INT16U seqno,int type)
{
	int		filelen = -1;
	char	fname[FILENAMELEN]={};

	memset(fname,0,sizeof(fname));
	getFileName(oi,seqno,type,fname);
	filelen = getFileLen(fname);
	return filelen;
}
/*
 * 输入参数：	oi:对象标识，seqno:记录序号，blockdata:存储数据，savelen：存储长度，
 * 			type：存储类型【	根据宏定义SaveFile_type 】
 * 返回值：=1：文件存储成功
 * =-1: 文件不存在
 */
int readCoverClass(OI_698 oi,INT16U seqno,void *blockdata,int datalen,int type)
{
	int		ret = 0;
	char	fname[FILENAMELEN]={};

	switch(type) {
	case event_para_save:
	case coll_para_save:
	case acs_coef_save:
	case acs_energy_save:
//		fprintf(stderr,"readEventClass %s filelen=%d\n",fname,filelen);
		memset(fname,0,sizeof(fname));
		getFileName(oi,seqno,type,fname);
		if(datalen<=2)	return -1;
		ret = block_file_sync(fname,blockdata,datalen-2,0,0);	//返回数据去掉CRC校验的两个字节
	break;
	case event_record_save:
	case event_current_save:
		memset(fname,0,sizeof(fname));
		getFileName(oi,seqno,type,fname);
		if(datalen==0)	return -1;
		ret = readCoverFile(fname,blockdata,datalen);
		break;
	}
	return ret;
}

