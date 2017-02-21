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
				if(*sernum == 0 || *sernum == 0xffff) {		//当前位置不存在相关序号记录，进行添加动作
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
	case 0x6001:
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
	fprintf(stderr,"未找到OI=%04x的相关信息配置内容！！！\n",oi);
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
	case calc_voltage_save: //电压合格率
		makeSubDir(CALCDIR);
		sprintf(fname,"%s/%04x.par",CALCDIR,oi);
		break;
	}
//	fprintf(stderr,"getFileName fname=%s\n",fname);
}

/*
 * =0 : 文件存在
 * =-1：文件错误
 * */
int readFileName(OI_698 oi,INT16U seqno,INT16U type,char *fname)
{
	int		ret=0;
	char dirname[FILENAMELEN]={};
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
		break;
	case acs_energy_save:
		sprintf(fname,"%s/energy.par",_ACSDIR_);
		break;
	case para_init_save:	//参数初始化
		sprintf(fname,"%s/%04x.par",INITDIR,oi);
		break;
	}
	ret = access(fname,F_OK);
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
			else ret = 0;
		}
		fclose(fp);
	} else
	{
		ret = 0;
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
		fprintf(stderr,"创建文件\n");
	}else {
		fp = fopen((char*) FileName, "r+");
		fprintf(stderr,"替换文件\n");
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
	INT16U  sizenew=0,offset=0;
	void 	*blockdata1=NULL;
	void  	*blockdata2=NULL;
	struct 	stat info1={},info2={};		//文件信息stat包含头文件/sys/stat.h
	char	fname2[FILENAMELEN]={};
	INT16U  *readcrc1=NULL;//=(INT16U *)((INT8U*)blockdata+size-4);
	INT16U  *readcrc2=NULL;
	INT16U  ret=0;

//	fprintf(stderr,"\n read file :%s\n",fname);
	if(fname==NULL || strlen(fname)<=4 || size<=2) 	return 0;

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
	if((*readcrc1 == *readcrc2) && (ret1==1) && (ret2==1))  {		//两个文件校验正确，并且校验码相等，返回 1
//		fprintf(stderr,"正确\n");
//		syslog(LOG_NOTICE," %s 校验正确 ",fname);
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
//		syslog(LOG_NOTICE," %s 返回数据 ",fname);
	}else {
//		fprintf(stderr,"\n 读取失败！！\n");
		syslog(LOG_NOTICE," %s 读取失败! ",fname);
	}
	free(blockdata1);
	free(blockdata2);
//	fprintf(stderr,"ret=%d\n",ret);
	return ret;					//异常情况，程序返回0，参数初始默认值，产生ERC2参数丢失事件
}

// 数据块数据保存文件
// 输入参数：fname:主文件名，blockdata：主文件块缓冲区，size:主文件尺寸，index:文件的存储索引位置
// 返回值：=1：文件保存成功，=0，文件保存失败，此时建议产生ERC2参数丢失事件通知主站异常
INT8U save_block_file(char *fname,void *blockdata,int size,int headsize,int index)
{
	int		i=0,ret=0;
	int		sizenew=0,offset=0;
	INT16U	readcrc=0;

	if(fname==NULL) 	  return 0;

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
	return ret;
}


/////////////////////////////////////////////////////////
sem_t * InitSem()
{
//	return NULL;
	int			val=0;
	sem_t * 	sem_parasave=NULL;	//参数文件存储信号量
	//打开信号量
	sem_parasave = open_named_sem(SEMNAME_PARA_SAVE);
	if(sem_parasave!=NULL) {
		sem_wait(sem_parasave);
	}
//	sem_getvalue(sem_parasave, &val);
//	fprintf(stderr,"\nprocess The sem = %s value = %d sem_parasave=%p\n",SEMNAME_PARA_SAVE, val,sem_parasave);
	return sem_parasave;
}

void CloseSem(sem_t * sem_parasave)
{
//	return;
	int  val=0;
	if(sem_parasave!=NULL) {
		sem_post(sem_parasave);
//			sem_getvalue(sem_parasave, &val);
//			fprintf(stderr,"closesem: val=%d   sem_parasave=%p\n",val,sem_parasave);
		sem_parasave = NULL;
		sem_close(sem_parasave);
	}
}
/*
 * 计算某个普通采集方案一天需要存几次，是否与采集任务里面的执行频率有关？
 */
INT16U CalcFreq(CLASS_6015 class6015)
{
	INT16U freq=0;
	INT8U  unit=0;
	INT16U value=0;
	switch(class6015.data.type)
	{
	case 0://采集当前数据
	case 1://采集上第n次
	case 2://按冻结时标采集
		freq = 1;
		break;
	case 3://按时标间隔采集
		unit = class6015.data.data[0];
		value = (class6015.data.data[1]<<8) + class6015.data.data[2];
		break;
	}
	if(class6015.data.type == 3)
	{
		switch(unit)
		{
		case 0:
			freq = 24*60*60/value;
			break;
		case 1:
			freq = 24*60/value;
			break;
		case 2:
			freq = 24/value;
			break;
		case 3:
		case 4:
		case 5:
			freq = 1;
			break;
		}
	}
	return freq;
}
void getTaskFileName(INT8U taskid,TS ts,char *fname)
{
	char dirname[FILENAMELEN]={};
	if (fname==NULL)
		return ;
	memset(fname,0,FILENAMELEN);
	sprintf(dirname,"%s",TASKDATA);
	makeSubDir(dirname);
	sprintf(dirname,"%s/%03d",TASKDATA,taskid);
	makeSubDir(dirname);
	sprintf(fname,"%s/%03d/%04d%02d%02d.dat",TASKDATA,taskid,ts.Year,ts.Month,ts.Day);
//	fprintf(stderr,"getFileName fname=%s\n",fname);
}
INT16U GetFileOadLen(INT8U units,INT8U tens)//个位十位转化为一个INT16U
{
	INT16U total = 0;
	total = tens;
	return (total<<8)+units;
}
/*
 * 返回位置和长度
 */
int GetPosofOAD(INT8U *file_buf,OAD oad_master,OAD oad_relate,HEAD_UNIT *head_unit,INT8U unitnum,INT8U *tmpbuf)
{
	int i=0,j=0,datapos=0,curlen=0,valid_cnt = 0;
	OAD oad_hm,oad_hr;
	fprintf(stderr,"\n--GetPosofOAD--unitnum=%d\n",unitnum);
	memset(&oad_hm,0xee,sizeof(OAD));
	memset(&oad_hr,0xee,sizeof(OAD));
	for(i=0;i<unitnum;i++)
	{
		curlen = GetFileOadLen(head_unit[i].num[1],head_unit[i].num[0]);
		fprintf(stderr,"\n%02x %02x\n",head_unit[i].num[1],head_unit[i].num[0]);
		fprintf(stderr,"\n-head_unit[%d].type=%d-GetPosofOAD--curlen=%d,valid_cnt=%d\n",i,head_unit[i].type,curlen,valid_cnt);
		if(head_unit[i].type == 1)
		{
			memcpy(&oad_hr,&head_unit[i].oad,sizeof(OAD));
			memset(&oad_hm,0xee,sizeof(OAD));
			valid_cnt = curlen;
			continue;
		}
		if(valid_cnt==0)//关联oad失效
			memset(&oad_hr,0xee,sizeof(OAD));
		if(valid_cnt>0)
			valid_cnt--;//road包括cnt个oad，在非零前，关联oad有效
		if(head_unit[i].type == 0)
		{
			memcpy(&oad_hm,&head_unit[i].oad,sizeof(OAD));
		}
		fprintf(stderr,"\nhead master oad:%04x%02x%02x\n",oad_hm.OI,oad_hm.attflg,oad_hm.attrindex);
		fprintf(stderr,"\nhead relate oad:%04x%02x%02x\n",oad_hr.OI,oad_hr.attflg,oad_hr.attrindex);
		fprintf(stderr,"\nbaow master oad:%04x%02x%02x\n",oad_master.OI,oad_master.attflg,oad_master.attrindex);
		fprintf(stderr,"\nbaow relate oad:%04x%02x%02x\n",oad_relate.OI,oad_relate.attflg,oad_relate.attrindex);
		if(memcmp(&oad_hm,&oad_master,sizeof(OAD))==0 && memcmp(&oad_hr,&oad_relate,sizeof(OAD))==0)
		{
			fprintf(stderr,"\n-----oad equal!\n");
			memcpy(tmpbuf,&file_buf[datapos],curlen);
			fprintf(stderr,"\n文件中的数据:");
			for(j=0;j<400;j++)
			{
				if(j%20==0)
					fprintf(stderr,"\n");
				fprintf(stderr," %02x",file_buf[j]);
			}
			fprintf(stderr,"\n");
			fprintf(stderr,"\n文件中取出的数据(%d)datapos=%d:",curlen,datapos);
			for(j=0;j<curlen;j++)
				fprintf(stderr," %02x",tmpbuf[j]);
			fprintf(stderr,"\n");
			return curlen;
		}
		else
			fprintf(stderr,"\n-----oad not equal!\n");
		if(head_unit[i].type == 0)
			datapos += curlen;

		if(valid_cnt==0)
		{
			memset(&oad_hr,0xee,sizeof(OAD));
		}

	}
	return 0;
}
/*
 * 读取抄表数据，读取某个测量点某任务某天一整块数据，放在内存里，根据需要提取数据,并根据csd组报文
 */
int ComposeSendBuff(TS *ts,INT8U seletype,INT8U taskid,TSA *tsa_con,INT8U tsa_num,CSD_ARRAYTYPE csds,INT8U *SendBuf)
{
	OAD  oadm,oadr;
	FILE *fp  = NULL;
	INT16U headlen=0,blocklen=0,unitnum=0,freq=0,sendindex=0,retlen=0,schpos=0;
	INT8U *databuf_tmp=NULL;
	INT8U tmpbuf[256];
	INT8U headl[2],blockl[2];
	int i=0,j=0,k=0,m=0;
	CLASS_6015	class6015={};
	CLASS_6013	class6013={};
	HEAD_UNIT *head_unit = NULL;
	TS ts_now;
	TSGet(&ts_now);
	char	fname[FILENAMELEN]={};
	memset(&class6013,0,sizeof(CLASS_6013));
	readCoverClass(0x6013,taskid,&class6013,sizeof(class6013),coll_para_save);
	memset(&class6015,0,sizeof(CLASS_6015));
	readCoverClass(0x6015,class6013.sernum,&class6015,sizeof(CLASS_6015),coll_para_save);
	freq = CalcFreq(class6015);
	////////////////////////////////////////////////////////////////////////////////test
	memset(&class6015,0xee,sizeof(CLASS_6015));
	class6015.csds.num = 1;
	class6015.csds.csd[0].type=1;
	class6015.csds.csd[0].csd.road.oad.OI =0x5004;
	class6015.csds.csd[0].csd.road.oad.attflg = 0x02;
	class6015.csds.csd[0].csd.road.oad.attrindex = 0x00;
	class6015.csds.csd[0].csd.road.num = 3;
	class6015.csds.csd[0].csd.road.oads[0].OI = 0x2021;
	class6015.csds.csd[0].csd.road.oads[0].attflg = 0x02;
	class6015.csds.csd[0].csd.road.oads[0].attrindex = 0x00;
	class6015.csds.csd[0].csd.road.oads[1].OI = 0x0010;
	class6015.csds.csd[0].csd.road.oads[1].attflg = 0x02;
	class6015.csds.csd[0].csd.road.oads[1].attrindex = 0x00;
	class6015.csds.csd[0].csd.road.oads[2].OI = 0x0020;
	class6015.csds.csd[0].csd.road.oads[2].attflg = 0x02;
	class6015.csds.csd[0].csd.road.oads[2].attrindex = 0x00;
	freq = 1;
	taskid=1;
	//////////////////////////////////////////////////////////////////////////////////test

	getTaskFileName(taskid,ts_now,fname);
//	ReadFileHeadLen(fname,&headlen,&unitlen);
//	headbuf = (INT8U *)malloc(headlen);
//	unitnum = (headlen-4)/sizeof(HEAD_UNIT);
//	ReadFileHead(fname,headlen,unitlen,unitnum,headbuf);
//	databuf_tmp = malloc(unitlen);
//	savepos = (int *)malloc(tsa_num*sizeof(int));
//	fprintf(stderr,"\n-------------6--%s\n",fname);
	fp = fopen(fname,"r");
	if(fp == NULL)//文件没内容 组文件头，如果文件已存在，提取文件头信息
	{
		return 0;
	}
	else
	{
//		fprintf(stderr,"\n-------------7\n");
		fread(headl,2,1,fp);
		headlen = headl[0];
		headlen = (headl[0]<<8) + headl[1];
		fread(&blockl,2,1,fp);
//		fprintf(stderr,"\nblocklen=%02x %02x\n",blockl[1],blockl[0]);
		blocklen = blockl[0];
		blocklen = (blockl[0]<<8) + blockl[1];
		unitnum = (headlen-4)/sizeof(HEAD_UNIT);
		databuf_tmp = (INT8U *)malloc(blocklen);

		head_unit = (HEAD_UNIT *)malloc(headlen-4);
		fread(head_unit,headlen-4,1,fp);

		fseek(fp,headlen,SEEK_SET);//跳过文件头
		while(!feof(fp))//找存储结构位置
		{
//			fprintf(stderr,"\n-------------8---blocklen=%d,headlen=%d,tsa_num=%d\n",blocklen,headlen,tsa_num);
			//00 00 00 00 00 00 00 00 07 05 00 00 00 00 00 01
			//00 00 00 00 00 00 00 00 00 07 05 00 00 00 00 00 01
			memset(databuf_tmp,0xee,blocklen);
			if(fread(databuf_tmp,blocklen,1,fp) == 0)
				break;
			for(i=0;i<tsa_num;i++)
			{
				for(m=0;m<freq;m++)
				{
					schpos = m*blocklen/freq;
//					fprintf(stderr,"\n-------------9---i=%d\n",i);
//					fprintf(stderr,"\n1addr1:%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
//							databuf_tmp[schpos+16],databuf_tmp[schpos+15],databuf_tmp[schpos+14],databuf_tmp[schpos+13],
//							databuf_tmp[schpos+12],databuf_tmp[schpos+11],databuf_tmp[schpos+10],databuf_tmp[schpos+9],
//							databuf_tmp[schpos+8],databuf_tmp[schpos+7],databuf_tmp[schpos+6],	databuf_tmp[schpos+5],
//							databuf_tmp[schpos+4],databuf_tmp[schpos+3],databuf_tmp[schpos+2],databuf_tmp[schpos+1]);
//					fprintf(stderr,"\n1addr2:%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
//							tsa_con[i].addr[16],tsa_con[i].addr[15],tsa_con[i].addr[14],tsa_con[i].addr[13],
//							tsa_con[i].addr[12],tsa_con[i].addr[11],tsa_con[i].addr[10],tsa_con[i].addr[9],
//							tsa_con[i].addr[8],tsa_con[i].addr[7],tsa_con[i].addr[6],	tsa_con[i].addr[5],
//							tsa_con[i].addr[4],tsa_con[i].addr[3],tsa_con[i].addr[2],tsa_con[i].addr[1],tsa_con[i].addr[0]);
					if(memcmp(&databuf_tmp[schpos+1],&tsa_con[i].addr[1],16)==0)//找到了存储结构的位置，一个存储结构可能含有unitnum个单元
					{
						fprintf(stderr,"\naddr1:%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
								databuf_tmp[schpos+16],databuf_tmp[schpos+15],databuf_tmp[schpos+14],databuf_tmp[schpos+13],
								databuf_tmp[schpos+12],databuf_tmp[schpos+11],databuf_tmp[schpos+10],databuf_tmp[schpos+9],
								databuf_tmp[schpos+8],databuf_tmp[schpos+7],databuf_tmp[schpos+6],	databuf_tmp[schpos+5],
								databuf_tmp[schpos+4],databuf_tmp[schpos+3],databuf_tmp[schpos+2],databuf_tmp[schpos+1]);
						fprintf(stderr,"\naddr2:%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
								databuf_tmp[schpos+16],databuf_tmp[schpos+15],databuf_tmp[schpos+14],databuf_tmp[schpos+13],
								databuf_tmp[schpos+12],databuf_tmp[schpos+11],databuf_tmp[schpos+10],databuf_tmp[schpos+9],
								databuf_tmp[schpos+8],databuf_tmp[schpos+7],databuf_tmp[schpos+6],	databuf_tmp[schpos+5],
								databuf_tmp[schpos+4],databuf_tmp[schpos+3],databuf_tmp[schpos+2],databuf_tmp[schpos+1]);
//						fprintf(stderr,"\n------------10---find addr\n");
						for(j=0;j<csds.num;j++)
						{
//							fprintf(stderr,"\n-------%d:(type=%d)\n",j,csds.csd[j].type);
							if(csds.csd[j].type != 0 && csds.csd[j].type != 1)
								continue;
							if(csds.csd[j].type == 1)
							{
								SendBuf[sendindex++] = 0x01;//aray
								SendBuf[sendindex++] = csds.csd[j].csd.road.num;
								for(k=0;k<csds.csd[j].csd.road.num;k++)
								{
									memset(tmpbuf,0x00,256);
									memcpy(&oadm,&csds.csd[j].csd.road.oads[k],sizeof(OAD));
									memcpy(&oadr,&csds.csd[j].csd.road.oad,sizeof(OAD));
//									fprintf(stderr,"\nmaster oad:%04x%02x%02x\n",oadm.OI,oadm.attflg,oadm.attrindex);
//									fprintf(stderr,"\nrelate oad:%04x%02x%02x\n",oadr.OI,oadr.attflg,oadr.attrindex);
									if((retlen = GetPosofOAD(&databuf_tmp[schpos],oadm,oadr,head_unit,unitnum,tmpbuf))==0)
										SendBuf[sendindex++] = 0;
									else
									{
//										fprintf(stderr,"\n--type=1--tmpbuf[0]=%02x\n",tmpbuf[0]);
										if(tmpbuf[0]==0)//NULL
											SendBuf[sendindex++] = 0;
										else
										{
											memcpy(&SendBuf[sendindex],tmpbuf,retlen);
											sendindex += retlen;
										}
									}
//									fprintf(stderr,"\n---1----k=%d retlen=%d\n",k,retlen);
								}
							}
							if(csds.csd[j].type == 0)
							{
								if(csds.csd[j].csd.oad.OI == 0x4001)
								{
									SendBuf[sendindex++]=0x55;
									for(k=1;k<17;k++)
									{
//										fprintf(stderr,"\n--databuf_tmp[%d+%d]=%d\n",schpos,k,databuf_tmp[schpos+k]);
										if(databuf_tmp[schpos+k]==0)
											continue;
										memcpy(&SendBuf[sendindex],&databuf_tmp[schpos+k],databuf_tmp[schpos+k]+1);
										sendindex += databuf_tmp[schpos+k]+1;
										break;
									}
									continue;
								}
								memset(tmpbuf,0x00,256);
								memcpy(&oadm,&csds.csd[j].csd.oad,sizeof(OAD));
								memset(&oadr,0xee,sizeof(OAD));
								if((retlen = GetPosofOAD(&databuf_tmp[schpos],oadm,oadr,head_unit,unitnum,tmpbuf))==0)
									SendBuf[sendindex++] = 0;
								else
								{
//									fprintf(stderr,"\n--type=0--tmpbuf[0]=%02x\n",tmpbuf[0]);
									if(tmpbuf[0]==0)//NULL
										SendBuf[sendindex++] = 0;
									else
									{
										memcpy(&SendBuf[sendindex],tmpbuf,retlen);
										sendindex += retlen;
									}
								}
//								fprintf(stderr,"\n---0----k=%d retlen=%d\n",k,retlen);
							}
						}
	//					return sendindex;
						continue;
					}
				}
			}
		}
	}
	return sendindex;
}
