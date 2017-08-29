/*
 * filebase.c
 *
 *  Created on: Jan 18, 2017
 *      Author: ava
 */

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
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "filebase.h"
#include "PublicFunction.h"
#include "AccessFun.h"
#include "StdDataType.h"
#include "Objectdef.h"
#include "EventObject.h"
#include "ParaDef.h"

extern CLASS_INFO	info;

/**************************************/
//函数功能：建立子目录
/**************************************/
void makeSubDir(char *DirName)
{
	DIR *dir=NULL;
	dir = opendir(DirName);
	if(dir==NULL) {
		mkdir(DirName,0777);
	}
	else
	{
		closedir(dir);
	}
}

/*
 * 返回值： 0：写公用接口类失败，1：写成功
 * */
INT8U	writeInterClass(char *file_name,void *dest,int size)
{
	FILE 	*fp=NULL;
	int		num=0;
	char	fname2[FILENAMELEN]={};

	if(access(file_name,F_OK)!=0)  //文件不存在
	{
		fp = fopen((char*)file_name, "w+");
	}else {
		fp = fopen((char*)file_name, "r+");
	}
	if (fp != NULL) {
		fseek(fp, 0L, SEEK_SET);
//		fprintf(stderr,"write size=%d\n",size);
		num = fwrite(dest,size,1,fp);
		fclose(fp);
	}
	memset(fname2,0,sizeof(fname2));
	strncpy(fname2,file_name,strlen(file_name)-4);
	strcat(fname2,".bak");
	if(access(fname2,F_OK)!=0)  //文件不存在
	{
		fp = fopen((char*)fname2, "w+");
	}else {
		fp = fopen((char*)fname2, "r+");
	}
	if (fp != NULL) {
		fseek(fp, 0L, SEEK_SET);
//		fprintf(stderr,"write size=%d\n",size);
		num = fwrite(dest,size,1,fp);
		fclose(fp);
	}
	return num;
};

int WriteClass11(OI_698 oi,INT16U seqnum,INT8U method)
{
	void 	*unitdata=NULL;
	CLASS11		class11={};
	CLASS_INFO	tmpinfo={};
	int		ret=0;
	INT16U 	*sernum=NULL;

	if(getclassinfo(oi,&tmpinfo)==-1) {
		return -1;
	}

	unitdata = malloc(tmpinfo.unit_len);
	memset(unitdata,0,tmpinfo.unit_len);
	if(unitdata==NULL) {
		free(unitdata);
		syslog(LOG_NOTICE," oi = %04x malloc error",oi);
		return -2;
	}

	if(readInterClass(oi,&class11)==0){			//文件不存在，初始化类
		strncpy((char *)&class11.logic_name,tmpinfo.logic_name,sizeof(class11.logic_name));
		class11.curr_num = 0;
		class11.max_num = MAX_POINT_NUM;
	}
	if(seqnum>0 && seqnum<=MAX_POINT_NUM) {
		switch(method) {
		case AddUpdate:
			ret = block_file_sync(tmpinfo.file_name,unitdata,tmpinfo.unit_len,tmpinfo.interface_len,seqnum);
			if(ret==1) {
				sernum = (INT16U *)((INT8U*)unitdata+tmpinfo.index_site);
	//			fprintf(stderr,"================WriteClass11:index=%d  sernum=%d,site=%d\n",index,*sernum,tmpinfo.index_site);
				if(*sernum != 0 && *sernum != 0xffff) {		//当前位置不存在相关序号记录，进行添加动作
					class11.curr_num++;
					fprintf(stderr,"添加操作 当前元素个数=%d\n",class11.curr_num);
				}
			}
			break;
		case Delete:
			if(class11.curr_num) {
				class11.curr_num--;
				fprintf(stderr,"删除操作 当前元素个数=%d\n",class11.curr_num);
			}
			break;
		case Clear:
			class11.curr_num=0;
			break;
		}
	}
	writeInterClass((char *)tmpinfo.file_name,&class11,sizeof(CLASS11));
	free(unitdata);
	return 1;
}
/*
 * 组织接口类函数的属性值
 * */
//TODO: 各个接口类能否做成统一？
void WriteInterfaceClass(OI_698 oi,INT16U seqnum,INT8U method)
{
	switch(oi) {
	case 0x6000:
		WriteClass11(oi,seqnum,method);
		break;
	}
}

/*
 * 覆盖存储（数据文件直接存储）
 */
INT8U writeCoverFile(char *fname, void *dataunit,int len)
{
	int fd=0,ret=0;
	FILE *fp=NULL;

	fp = fopen(fname, "w");
	if(fp!=NULL)
	{
		fprintf(stderr,"\nfilewrite over %s",fname);
		ret = fwrite(dataunit,len,1,fp);
		fd = fileno(fp);
		fsync(fd);
		fclose(fp);
	}
	return ret;
}

/*
 * 覆盖文件（数据）整块读取
 */
int readCoverFile(char *fname, void *dataunit,int len)
{
	FILE 	*fp=NULL;
	int		num = 0;

	fp = fopen(fname, "r");
	if (fp != NULL) {
		num=fread(dataunit,1 ,len,fp);
		fclose(fp);
	}
	return num;
}

///////////////////////////////////////////////////////////////////////////////
/*
 * 根据oi参数，查找相应的class_info的结构体数据
 * */
INT16S getclassinfo(OI_698 oi,CLASS_INFO *classinfo)
{
	INT16S i=0;
	for(i=0; i < sizeof(class_info)/sizeof(CLASS_INFO);i++)
	{
//		fprintf(stderr,"%d:classinfo %04x  oi=%04x\n",i,class_info[i].oi,oi);
		if(class_info[i].oi == oi) {
			strncpy(classinfo->logic_name,class_info[i].logic_name,sizeof(class_info[i].logic_name));
			strncpy(classinfo->file_name,class_info[i].file_name,sizeof(class_info[i].file_name));
			classinfo->interface_len = class_info[i].interface_len;
			classinfo->unit_len = class_info[i].unit_len;
			classinfo->index_site = class_info[i].index_site;
			return i;
		}
	}
//	fprintf(stderr,"未找到OI=%04x的相关信息配置内容！！！\n",oi);
	return -1;
}

/*
 * 根据oi参数，查找相应的Variable_Class的结构体数据
 * */
int getvarioffset(OI_698 oi,int coll_seqnum,int *offset,int *blklen)
{
	int i=0;

	for(i=0; i < sizeof(vari_data)/sizeof(Variable_Class);i++)
	{
		if(vari_data[i].oi == oi) {
			*blklen = VARI_LEN;
			*offset = vari_data[i].offset*VARI_LEN;
			return 1;
		}
	}
	*offset=0;
	for(i=0; i < sizeof(vari_tj_data)/sizeof(Variable_TJ_Class);i++)
	{
		*offset += vari_tj_data[i].datalen;
		if(vari_tj_data[i].oi == oi) {
			*blklen = vari_tj_data[i].datalen;
			return 2;
		}
	}
	return -1;
}

/*
char filename[256][256];
int len = 0;
int trave_dir(char* path, int depth)
{
    DIR *d; //声明一个句柄
    struct dirent *file; //readdir函数的返回值就存放在这个结构体中
    struct stat sb;

    if(!(d = opendir(path)))
    {
        printf("error opendir %s!!!/n",path);
        return -1;
    }
    while((file = readdir(d)) != NULL)
    {
        //把当前目录.，上一级目录..及隐藏文件都去掉，避免死循环遍历目录
        if(strncmp(file->d_name, ".", 1) == 0)
            continue;
        strcpy(filename[len++], file->d_name); //保存遍历到的文件名
        //判断该文件是否是目录，及是否已搜索了三层，这里我定义只搜索了三层目录，太深就不搜了，省得搜出太多文件
        if(lstat(file->d_name, &sb) >= 0 && S_ISDIR(sb.st_mode) && depth <= 3)
        {
            trave_dir(file->d_name, depth + 1);
        }
    }
    closedir(d);
    return 0;
}
*/

/************************************
 * 函数说明：获取参数文件对象配置单元的个数
 * 返回值：
 * >=0:  单元个数
 * -1:  未查找到OI类数据
 * -2:	文件记录不完整
 *************************************/
long getFileLen(char *filename)
{
	long 		filesize=0;

	fprintf(stderr,"filename=%s\n",filename);
    FILE* fp = fopen(filename, "rb" );
    if(fp==NULL){
//        fprintf(stderr,"ERROR: Open file %s failed.\n", filename);
        return -1;
    }
    fseek( fp, 0L, SEEK_END );
    filesize=ftell(fp);
    fclose(fp);
    return filesize;
}

/************************************
 * 函数说明：获取参数文件对象配置单元的个数
 * 返回值：
 * >=0:  单元个数
 * -1:  未查找到OI类数据
 * -2:	文件记录不完整
 *************************************/
long getFileRecordNum(OI_698 oi)
{
	int			blknum=0,sizenew=0;
	CLASS_INFO	info={};
	long 		filesize=0;

	if(getclassinfo(oi,&info)==-1) {
		return -1;
	}

	if(info.unit_len%4==0)	sizenew = info.unit_len+2;
	else sizenew = info.unit_len+(4-info.unit_len%4)+2;

    FILE* fp = fopen(info.file_name, "rb" );
    if(fp==NULL){
        fprintf(stderr,"ERROR: Open file %s failed.\n", info.file_name);
        return 0;
    }
    fseek( fp, 0L, SEEK_END );
    filesize=ftell(fp);
    fclose(fp);

	blknum = (filesize-info.interface_len)/sizenew;

	if((filesize-info.interface_len)%sizenew!=0 ){
		fprintf(stderr,"采集档案表不是整数，检查文件完整性！！！ %ld-%d=%d\n",filesize,info.interface_len,sizenew);
		return -2;
	}
    return blknum;
}

/**************************************
*  函数功能：根据OI及类型获取存储文件路径及文件名字
*  ！！！！！注意： 函数返回是文件的实际长度，参数文件中包含CRC校验字节
* 				 如果是参数文件请慎用此函数
**************************************/
void getFileName(OI_698 oi,INT16U seqno,INT16U type,char *fname)
{
	char dirname[FILENAMELEN]={};
	if (fname==NULL)
		return ;
	memset(fname,0,FILENAMELEN);
	switch(type) {
	case event_para_save:
		makeSubDir(EVENTDIR);
		makeSubDir(EVENT_PORP);
		sprintf(dirname,"%s/%04x",EVENT_PORP,oi);
		makeSubDir(dirname);
		sprintf(fname,"%s/%04x/%04x.par",EVENT_PORP,oi,oi);
		break;
	case event_record_save:
		makeSubDir(EVENTDIR);
		makeSubDir(EVENT_REC);
		sprintf(dirname,"%s/%04x",EVENT_REC,oi);
		makeSubDir(dirname);
		sprintf(fname,"%s/%04x/%d.dat",EVENT_REC,oi,seqno);
		break;
	case event_current_save:
		makeSubDir(EVENTDIR);
		makeSubDir(EVENT_CURR);
		sprintf(dirname,"%s/%04x",EVENT_CURR,oi);
		makeSubDir(dirname);
		sprintf(fname,"%s/%04x/%d.dat",EVENT_CURR,oi,seqno);
		break;
	case para_vari_save:
		makeSubDir(PARADIR);
		sprintf(fname,"%s/%04x.par",PARADIR,oi);
		break;
	case coll_para_save:
		makeSubDir(PARADIR);
		sprintf(fname,"%s/%04x/",PARADIR,oi);
		makeSubDir(fname);
		sprintf(fname,"%s/%04x/%d.par",PARADIR,oi,seqno);
		break;
	case acs_coef_save:
		makeSubDir(_ACSDIR_);
		sprintf(fname,"%s/accoe.par",_ACSDIR_);
		break;
	case acs_energy_save:
		makeSubDir(_ACSDIR_);
		sprintf(fname,"%s/energy.par",_ACSDIR_);
		break;
	case para_init_save:	//参数初始化
		makeSubDir(INITDIR);
		sprintf(fname,"%s/%04x.par",INITDIR,oi);
		break;
	}
	fprintf(stderr,"getFileName fname=%s\n",fname);
}

/*
 * =0 : 文件存在
 * =-1：文件错误
 * */
int readFileName(OI_698 oi,INT16U seqno,INT16U type,char *fname)
{
	int		ret=0;
	if (fname==NULL)
		return -1;
	memset(fname,0,FILENAMELEN);
	switch(type) {
	case event_para_save:
		sprintf(fname,"%s/%04x/%04x.par",EVENT_PORP,oi,oi);
		break;
	case event_record_save:
		sprintf(fname,"%s/%04x/%d.dat",EVENT_REC,oi,seqno);
		break;
	case event_current_save:
		sprintf(fname,"%s/%04x/%d.dat",EVENT_CURR,oi,seqno);
		break;
	case para_vari_save:
		sprintf(fname,"%s/%04x.par",PARADIR,oi);
		break;
	case coll_para_save:
		sprintf(fname,"%s/%04x/%d.par",PARADIR,oi,seqno);
		break;
	case acs_coef_save:
		sprintf(fname,"%s/accoe.par",_ACSDIR_);
		if(access(fname,F_OK)!=0) {		//文件不存在，查找原3761规约下的参数文件
			sprintf(fname,"%s/accoe.par",_CFGDIR_);
		}
		break;
	case acs_energy_save:
		sprintf(fname,"%s/energy.par",_ACSDIR_);
		break;
	case para_init_save:	//参数初始化
		sprintf(fname,"%s/%04x.par",INITDIR,oi);
		break;
	}
	ret = access(fname,F_OK);
	if(ret!=0 && oi==0x4001){	 //通信地址，数据区初始化恢复通信地址
		memset(fname,0,FILENAMELEN);
		sprintf(fname,"%s/%04x.par",INITDIR,oi);
		ret = access(fname,F_OK);
	}
	return ret;
}

///////////////////////////////////////////////////////////////////////////
INT16U crc(INT16U Data)
{
	unsigned char n=0;
	INT16U Parity=0;

	Parity=Data;
	for(n=0;n<8;n++)
	{
		if ((Parity&0x1)==0x1)
		{
			Parity=Parity>>1;
			Parity=Parity^CODE;
		}
		else {Parity=Parity>>1;}
	}
	return(Parity);
}
// 写数据结构体crc16校验到结构体第一个成员crc16
// 输入参数：source:文件内容，size:文件尺寸
// 返回值：     crc：校验值
INT16U  make_parity(void *source,int size)
{
	int 	m=0;
	INT16U			Parity=0xffff;
	unsigned char   *buf = (unsigned char *)source;
//	 fprintf(stderr,"------------size=%d\n", size);
    for (m=0; m<(size-2); m++){
//    	if(m%32==0)fprintf(stderr,"\n");
//    	fprintf(stderr,"%02x ",buf[m]);
		Parity=Parity^buf[m];
		Parity=crc(Parity);
//		fprintf(stderr," %04x ",Parity);
	}
//    fprintf(stderr,"\n计算校验=%04x\n",Parity);
	return Parity;
}

/////////////////////////////////////////////////////////////////////////////////
//为了兼容376.1校表文件的存储，存储目录变更为/nor/config/accoef.par
/**************************************
 * 函数功能：写数据结构体crc16校验到结构体第一个成员crc16
 * 输入参数：source:文件内容，size:文件尺寸
 * 返回值：  crc：校验值
 **************************************/
INT16U make_parity_accoef(void *source, int size)
{
	int m;
	INT16U Parity = 0xffff;
	unsigned char *buf = (unsigned char *) source;

	for (m = 2; m < size; m++)
	{
		Parity = Parity ^ buf[m];
		Parity = crc(Parity);
	}
	//  fprintf(stderr,"计算校验=%04x\n",Parity);
	return Parity;
}

INT8U fu_read_accoef(char *FileName, void *source, INT32U size)
{
	FILE *fp = NULL;
	int num, ret = 0;
	INT16U *readcrc = (INT16U *) source;

	fprintf(stderr,"FileName=%s\n",FileName);
	fp = fopen((const char*)FileName, "r");
	if (fp != NULL )
	{
		num = fread(source, size, 1, fp);
//		fprintf(stderr,"read.num=%d,size=%d,reccrc=%04x\n",num,size,*readcrc);
		if (num == 1)
		{			//读取了size字节数据
//			fprintf(stderr,"make_parity = %04x\n",make_parity_accoef(source, size));
			if (make_parity_accoef(source, size) == *readcrc)
			{
				ret = 1;
			} else
				ret = 0;
		}
		fclose(fp);
	} else
	{
		ret = 0;
	}
//	fprintf(stderr,"ret=%d\n",ret);
	return ret;
}

/**************************************
 * 函数功能：数据保存到指定文件,并进行CRC16校验
 * 输入参数：FileName:文件名
 * 输出：source:文件内容，size:文件尺寸
 * 返回值：  =1，文件保存成功，=0，文件保存失败
 **************************************/
INT8U file_write_accoef(char *FileName, void *source, int size)
{
	FILE *fp = NULL;
	int fd;
	INT8U res;
	int num = 0;
	INT16U *readcrc = (INT16U *) source;
	*readcrc = make_parity_accoef(source, size);			//计算crc16校验
	fp = fopen((char*) FileName, "w");
	if (fp != NULL )
	{
		fseek(fp, 0, SEEK_SET);
		num = fwrite(source, size, 1, fp);
//		fprintf(stderr,"fwrite.num=%d,FileName=%s,size=%d\n",num,FileName,size);
		if (num == 1)
		{
			res = 1;
		} else
			res = 0;
		fd = fileno(fp);
		fsync(fd);
		fclose(fp);
	} else
	{
//		fprintf(stderr, "%s saved error\n\r", FileName);
		res = 0;
	}
	return res;
}
/////////////////////////////////////////////////////////////////////////

// 读取数据到指定缓冲区,并进行CRC16校验
// 结构体数据定义要求
// 		1:使用#pragma pack(1)限制结构体对齐尺寸为1个字节
// 		2:结构体第一个成员为INT16U crc；保存crc校验位
// 输入参数：FileName:文件名，size:文件尺寸
// 输出：	source:文件内容
// 返回值：     =1，校验正确，=0，校验错误
INT8U file_read(char *FileName, void *source, int size,int offset,INT16U *retcrc)
{
	FILE 	*fp=NULL;
	int 	num=0,ret=0;
	INT16U  readcrc=0;//=(INT16U *)((INT8U*)source+size-2);
//	int		i;
//	INT8U  *val;

//	fprintf(stderr,"read FileName=%s\n",FileName);
	fp = fopen(FileName, "r");
	if (fp != NULL) {
		fseek(fp, offset, SEEK_SET);
		num=fread(source,1 ,size-2,fp);
		fread(&readcrc,1,2,fp);
//		fprintf(stderr,"read.num=%d,size=%d,reccrc=%04x\n",num,size,readcrc);
//		for(i=0;i<(size-2);i++) {
//			val = (INT8U *)source+i;
//			fprintf(stderr,"%02x ",*val);
//		}
		if(num==(size-2)) {			//读取了size字节数据
			INT16U crc= make_parity(source,size);
//			fprintf(stderr,"\n计算 crc =%04x\n",crc);
			if(crc==readcrc)  {
//				fprintf(stderr,"read ok\n");
				*retcrc = readcrc;
				ret = 1;
			}
			else {
//				syslog(LOG_ERR,"__%s__,校验错误:crc=%d__readcrc=%d",__func__,crc,readcrc);
				ret = 0;
			}
		}else {
			syslog(LOG_ERR,"__%s__,read num=%d,size=%d",__func__,num,size);
		}
		fclose(fp);
	} else
	{
		ret = 0;
		syslog(LOG_ERR,"__%s__,文件打开失败【%s】,ret=%d",__func__,FileName,ret);
//		fprintf(stderr, "%s read error\n\r", FileName);
	}
	return ret;
}

//数据保存到指定文件,并进行CRC16校验
// Filename（文件名.扩展名格式）文件名长度限制FILENAMELEN（128），
//                    文件扩展名制定长度必须为3个字符
// 输入参数：FileName:文件名，source:文件内容，size:文件尺寸
// 返回值：     =1，文件保存成功，=0，文件保存失败
INT8U file_write(char *FileName, void *source, int size, int offset)
{
	FILE *fp=NULL;
	int	  fd=0;
	INT8U res=0;
	int num=0;
	INT8U	*blockdata=NULL;
	INT16U	readcrc=0;

//	fprintf(stderr,"\nwrite begin size=%d", size);
	blockdata = malloc(size);
//	fprintf(stderr,"\nwrite sourceaddr=%p,blockdata=%p\n", source,blockdata);
	if(blockdata!=NULL) {
//		fprintf(stderr,"write memcpy blockdata\n");
		memset(blockdata,0,sizeof(size));
		memcpy(blockdata,source,size-2);
	} else {
		return 0;//error
	}
//	fprintf(stderr,"\nwrite sourceaddr=%p\n", source);
	readcrc = make_parity(source,size);			//计算crc16校验
//	int i=0;
//	for(i=0;i<size;i++){
//		fprintf(stderr,"%02x ",blockdata[i]);
//	}
//	fprintf(stderr,"\nwrite FileName %s source crc=%04x\n",FileName, readcrc);

	memcpy(blockdata+size-2,&readcrc,2);
	if(access(FileName,F_OK)!=0)
	{
		fp = fopen((char*) FileName, "w+");
//		fprintf(stderr,"创建文件\n");
	}else {
		fp = fopen((char*) FileName, "r+");
//		fprintf(stderr,"替换文件\n");
	}
	if (fp != NULL) {
		fseek(fp, offset, SEEK_SET);

		num = fwrite(blockdata, size,1,fp);
//		fprintf(stderr,"write index=%d,size=%d num=%d\n",offset,size,num);
		fd = fileno(fp);
		fsync(fd);
		fclose(fp);
//		syslog(LOG_NOTICE,"fwrite.num=%d,FileName=%s,size=%d\n",num,FileName,size);
//		fprintf(stderr,"fwrite.num=%d,FileName=%s,size=%d\n",num,FileName,size);
		if(num == 1) {
//			fprintf(stderr,"num=%d\n",num);
			res = 1;
		}else res = 0;
//		syslog(LOG_NOTICE,"fwrite %s end!\n",FileName);
	} else {
//		fprintf(stderr, "%s saved error\n\r", FileName);
		res = 0;
	}
//	fprintf(stderr, "free %p\n", blockdata);
	free(blockdata);//add by nl1031
	return res;
}


// 数据块文件存储同步（文件保存为实际数据+最后两个字节的CRC校验）
// 输入参数：fname:保存文件名，size:文件尺寸
// 输出：    		blockdata：文件数据缓冲区
// 返回值： =1:文件同步成功,使用blockdata数据源初始化内存
//         =0:文件同步失败，返回错误，参数初始化默认值，建议产生ERC2参数丢失事件
INT8U block_file_sync(char *fname,void *blockdata,int size,int headsize,int index)
{
	INT8U	ret1=0,ret2=0;
	int		sizenew=0;
	long	offset=0;
	void 	*blockdata1=NULL;
	void  	*blockdata2=NULL;
	struct 	stat info1={},info2={};		//文件信息stat包含头文件/sys/stat.h
	char	fname2[FILENAMELEN]={};
	INT16U  *readcrc1=NULL;//=(INT16U *)((INT8U*)blockdata+size-4);
	INT16U  *readcrc2=NULL;
	INT16U  ret=0;

//	fprintf(stderr,"\n read file :%s　size=%d\n",fname,size);
	if(fname==NULL || strlen(fname)<=4 || size<2) {
		syslog(LOG_NOTICE,"strlen(%s)=%d,size=%d",fname,strlen(fname),size);
		return 0;
	}

	//文件默认最后两个字节为CRC16校验，原结构体尺寸如果不是4个字节对齐，进行补齐，加CRC16
	if(size%4==0)	sizenew = size+2;
	else sizenew = size+(4-size%4)+2;

//	fprintf(stderr,"size=%d,sizenew=%d\n",size,sizenew);

	blockdata1 = malloc(sizenew);
	blockdata2 = malloc(sizenew);
	memset(blockdata1,0,sizenew);
	memset(blockdata2,0,sizenew);
//	fprintf(stderr,"\nmalloc p=%p",blockdata2);
	if(blockdata1==NULL || blockdata2==NULL ) {
		if(blockdata1!=NULL)free(blockdata1);
		if(blockdata2!=NULL)free(blockdata2);
		syslog(LOG_NOTICE," %s malloc error",fname);
		return 0;
	}

	readcrc1=(INT16U *)((INT8U*)blockdata1+sizenew-2);
	readcrc2=(INT16U *)((INT8U*)blockdata2+sizenew-2);

	memset(fname2,0,sizeof(fname2));
	strncpy(fname2,fname,strlen(fname)-4);
	strcat(fname2,".bak");

//	fprintf(stderr,"\n------par=%s",fname);
//	fprintf(stderr,"\n------bak=%s",fname2);

	offset = headsize+sizenew*index;
	ret1 = file_read(fname,blockdata1,sizenew,offset,readcrc1);
	ret2 = file_read(fname2,blockdata2,sizenew,offset,readcrc2);
//	fprintf(stderr,"\ncrc1=%04x,crc2=%04x,ret1=%d,ret2=%d\n",*readcrc1,*readcrc2,ret1,ret2);
//	syslog(LOG_NOTICE,"\ncrc1=%04x,crc2=%04x,ret1=%d,ret2=%d\n",*readcrc1,*readcrc2,ret1,ret2);////2
	if((*readcrc1 == *readcrc2) && (ret1==1) && (ret2==1))  {		//两个文件校验正确，并且校验码相等，返回 1
//		fprintf(stderr,"正确\n");
//		syslog(LOG_NOTICE," %s 校验正确 ",fname);			////3
		ret= 1;
	}
	if ((*readcrc1!=*readcrc2) && (ret1==1) && (ret2==1)) {		//两个文件校验正确，但是校验码不等，采用文件保存日期新的数据
//		fprintf(stderr,"校验码不等\n");
		stat(fname,&info1);
		stat(fname2,&info2);
//		fprintf(stderr,"info1=%ld,info2=%ld\n",info1.st_mtim.tv_sec,info2.st_mtim.tv_sec);
//		if(info1.st_mtim.tv_sec >= info2.st_mtim.tv_sec) {			//fname1文件修改时间新,更新fname2备份数据
		//校验码不等，使用fname1文件内容更新fname2
//			syslog(LOG_NOTICE," %s 校验码不等,更新备份文件 ",fname);
//			fprintf(stderr," %s 校验码不等,更新备份文件 ",fname);
			file_write(fname2,blockdata1,sizenew,offset);
			ret= 1;
//		}else {														//fname2文件修改时间新,更新fname1源数据
//			syslog(LOG_NOTICE," %s 校验码不等,更新源文件 ",fname);
//			fprintf(stderr," %s 校验码不等,更新源文件 ",fname);
//			file_write(fname,blockdata2,sizenew);
//			ret= 1;
//		}
	}
	if((ret1==1) &&(ret2==0)) {							//fname1校验正确，fname2校验错误,更新fname2备份文件
//		fprintf(stderr,"备份文件校验错误\n");
		syslog(LOG_NOTICE," %s 备份文件校验错误 ",fname);
		file_write(fname2,blockdata1,sizenew,offset);
		ret= 1;
	}
	if((ret1==0) &&(ret2==1)) {							//fname2校验正确，fname1校验错误,更新fname1源文件
		fprintf(stderr,"主文件校验错误\n");
		syslog(LOG_NOTICE," %s 主文件校验错误 ",fname);
		file_write(fname,blockdata2,sizenew,offset);
		memcpy(blockdata1,blockdata2,sizenew);
		ret= 1;
	}
//	if(ret1==0 && ret2==0){
//		fprintf(stderr,"主文件 备份文件都错误  size=%d\n", sizenew);
//		syslog(LOG_NOTICE," %s 主文件 备份文件都错误 ",fname);
//		file_write(fname2,blockdata1,sizenew,offset);
//		file_write(fname,blockdata1,sizenew,offset);
//		ret = 1;
//	}

	if (ret ==1)
	{
		memcpy(blockdata,blockdata1,size);
//		syslog(LOG_NOTICE," %s 返回数据 ret=%d",fname,ret);///4
	}else {
//		fprintf(stderr,"\n 读取失败！！\n");
//		syslog(LOG_NOTICE," %s 读取失败! ",fname);
	}
	free(blockdata1);
	free(blockdata2);
//	fprintf(stderr,"ret=%d\n",ret);
//	syslog(LOG_NOTICE,"after free  %s 返回数据 ret=%d",fname,ret);////////5
	return ret;					//异常情况，程序返回0，参数初始默认值，产生ERC2参数丢失事件
}

// 数据块数据保存文件
// 输入参数：fname:主文件名，blockdata：主文件块缓冲区，size:主文件尺寸，index:文件的存储索引位置
// 返回值：=0：文件保存成功，=1，文件保存失败，此时建议产生ERC2参数丢失事件通知主站异常
INT8U save_block_file(char *fname,void *blockdata,int size,int headsize,int index)
{
	int		i=0,ret=0;
	int		sizenew=0,offset=0;
	INT16U	readcrc=0;

	if(fname==NULL) 	  return refuse_rw;

	//文件默认最后两个字节为CRC16校验，原结构体尺寸如果不是4个字节对齐，进行补齐，加CRC16
	if(size%4==0)	sizenew = size+2;
	else sizenew = size+(4-size%4)+2;

//	fprintf(stderr,"write fname=%s,size=%d,sizenew=%d\n",fname,size,sizenew);

	offset = headsize+sizenew*index;
	if(file_write(fname,blockdata,sizenew,offset)==1) {
		for(i=0;i<3;i++) {
//			fprintf(stderr,"read fname=%s,size=%d,sizenew=%d\n",fname,size,sizenew);
			if(file_read(fname,blockdata,sizenew,offset,&readcrc)==1) {						//源文件正确，备份参数文件
//				fprintf(stderr,"保存文件成功，备份文件,crc=%04x\n",readcrc);
				ret = block_file_sync(fname,blockdata,size,headsize,index);			//配置文件同步处理
//				fprintf(stderr,"**********block_file_sync ret=%d\n",ret);
				if(ret==1) 	break;
			}else {
				syslog(LOG_NOTICE,"file_read %s error",fname);
			}
		}
	}else {
		syslog(LOG_NOTICE,"file_write %s error",fname);
	}
	if(ret==1) return success;
	else return refuse_rw;
}


/////////////////////////////////////////////////////////
sem_t * InitSem()
{
	return NULL;
	int			val=0;
	sem_t * 	sem_parasave=NULL;	//参数文件存储信号量
	//打开信号量
	sem_parasave = open_named_sem(SEMNAME_PARA_SAVE);
	if(sem_parasave!=NULL) {
		sem_getvalue(sem_parasave, &val);
		if(val!=1) {
			asyslog(LOG_ERR,"\nprocess The sem = %s value = %d sem_parasave=%p\n",SEMNAME_PARA_SAVE, val,sem_parasave);
		}
		sem_wait(sem_parasave);
	}
	return sem_parasave;
}

void CloseSem(sem_t * sem_parasave)
{
	return;
	int  val=0;
	if(sem_parasave!=NULL) {
		sem_post(sem_parasave);
//			sem_getvalue(sem_parasave, &val);
//			fprintf(stderr,"closesem: val=%d   sem_parasave=%p\n",val,sem_parasave);
		sem_parasave = NULL;
		sem_close(sem_parasave);
	}
}
