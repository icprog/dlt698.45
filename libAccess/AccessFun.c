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
#include "PublicFunction.h"
#include "dlt698.h"

#define 	LIB_ACCESS_VER 			0x0001

CLASS_INFO	info={};

void clearData()
{
	//冻结类数据清除
	system("rm -rf /nand/task");
}

void clearEvent()
{
	//事件类数据清除
	INT8U*	eventbuff=NULL;
	int 	saveflg=0,i=0;
	int		classlen=0;
	Class7_Object	class7={};

	for(i=0; i < sizeof(event_class_len)/sizeof(EVENT_CLASS_INFO);i++)
	{
		if(event_class_len[i].oi) {
			classlen = event_class_len[i].classlen;
			eventbuff = (INT8U *)malloc(classlen);
			if(eventbuff!=NULL) {
				memset(eventbuff,0,classlen);
				fprintf(stderr,"i=%d, oi=%04x, size=%d\n",i,event_class_len[i].oi,classlen);
				saveflg = 0;
				saveflg = readCoverClass(event_class_len[i].oi,0,(INT8U *)eventbuff,classlen,event_para_save);
				fprintf(stderr,"saveflg=%d oi=%04x\n",saveflg,event_class_len[i].oi);
//				int		j=0;
//				INT8U	val;
//				for(j=0;j<classlen;j++) {
//					val = (INT8U )eventbuff[j];
//					fprintf(stderr,"%02x ",val);
//				}
//				fprintf(stderr,"\n");
				if(saveflg) {
					memcpy(&class7,eventbuff,sizeof(Class7_Object));
					fprintf(stderr,"修改前：i=%d,oi=%x,class7.crrentnum=%d\n",i,event_class_len[i].oi,class7.crrentnum);
					if(class7.crrentnum!=0) {
						class7.crrentnum = 0;			//清除当前记录数
						memcpy(eventbuff,&class7,sizeof(Class7_Object));
						saveflg = saveCoverClass(event_class_len[i].oi,0,eventbuff,classlen,event_para_save);
					}
				}
				free(eventbuff);
				eventbuff=NULL;
			}
		}
	}
	system("rm -rf /nand/event/record");
	system("rm -rf /nand/event/current");
}

void clearDemand()
{
	//需量类数据清除
	system("rm -rf /nand/demand");
}


/*
 * 数据区初始化接口函数
 * 返回值 =0: 删除成功
 * =-1：删除失败
 * */
int dataInit(INT16U attr)
{
    struct timeval start={}, end={};
    long  interval=0;
	fprintf(stderr,"[4300]设备参数 属性：%d\n",attr);

 	gettimeofday(&start, NULL);
 	switch(attr) {
	case 3://数据初始化
		clearData();
		clearEvent();
		clearDemand();
		break;
	case 5://事件初始化
		clearEvent();
		break;
	case 6://需量初始化
		clearDemand();
		break;
	}

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
	sem_t   *sem_save=NULL;

	infoi = getclassinfo(oi,&info);
	if(infoi == -1) {
		return -1;
	}
	sem_save = InitSem();
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
	CloseSem(sem_save);
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
	INT8U	oiA1=0;
	sem_t   *sem_save=NULL;

	sem_save = InitSem();

	infoi = getclassinfo(oi,&info);
	if(infoi==-1) {
		memset(cmd,0,sizeof(cmd));
		oiA1 = (oi & 0xf000) >> 12;
		switch(oiA1) {
		case 3:			//事件类
			sprintf(cmd,"rm -rf /%s/%04x",EVENT_PORP,oi);
			break;
		case 4:			//参变量类
		case 6:			//采集监控类
			sprintf(cmd,"rm -rf %s/%04x/",PARADIR,oi);
			break;
		}
		system(cmd);
		CloseSem(sem_save);
		return 1;
	}
	memset(fname2,0,sizeof(fname2));
	strncpy(fname2,class_info[infoi].file_name,strlen(class_info[infoi].file_name)-4);
	strcat(fname2,".bak");

	ret = unlink(class_info[infoi].file_name);
	ret = unlink(fname2);
	CloseSem(sem_save);
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

	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"rm -rf %s/%04x/%d.par",PARADIR,oi,id);
	system(cmd);
	sprintf(cmd,"rm -rf %s/%04x/%d.bak",PARADIR,oi,id);
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
	INT16S	infoi=-1;
	sem_t   *sem_save=NULL;

	infoi = getclassinfo(oi,&info);
	if(infoi == -1) {
		return -1;
	}
	sem_save = InitSem();
	if(class_info[infoi].interface_len!=0) {		//该存储单元内部包含的类的公共属性
		WriteInterfaceClass(oi,seqnum,AddUpdate);
	}
	ret = save_block_file((char *)class_info[infoi].file_name,blockdata,class_info[infoi].unit_len,class_info[infoi].interface_len,seqnum);
	CloseSem(sem_save);
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
	INT16S	infoi=-1;
	sem_t   *sem_save=NULL;

	infoi = getclassinfo(oi,&info);
	if(infoi==-1) {
		fprintf(stderr,"infoi=%d\n",infoi);
		return -1;
	}
	sem_save = InitSem();
	ret = block_file_sync((char *)class_info[infoi].file_name,blockdata,class_info[infoi].unit_len,class_info[infoi].interface_len,seqnum);
	CloseSem(sem_save);
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
	sem_t   *sem_save=NULL;

	sem_save = InitSem();
	memset(fname,0,sizeof(fname));
	getFileName(oi,seqno,type,fname);
	switch(type) {
	case event_para_save:
	case para_vari_save:
	case coll_para_save:
	case acs_coef_save:
	case acs_energy_save:
	case para_init_save:
	case calc_voltage_save:
		fprintf(stderr,"saveClass file=%s ",fname);
		ret = save_block_file(fname,blockdata,savelen,0,0);
		break;
	case event_record_save:
	case event_current_save:
		writeCoverFile(fname,blockdata,savelen);
		break;
	}
	CloseSem(sem_save);
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
//	int		readlen = 0;
	sem_t   *sem_save=NULL;
//	void 	*blockdata1=NULL;

	sem_save = InitSem();
	memset(fname,0,sizeof(fname));
	switch(type) {
	case event_para_save:
	case para_vari_save:
	case coll_para_save:
	case acs_coef_save:
	case acs_energy_save:
		ret = readFileName(oi,seqno,type,fname);
		if(ret==0) {		//文件存在
//			fprintf(stderr,"readClass %s filelen=%d\n",fname,datalen);
			ret = block_file_sync(fname,blockdata,datalen,0,0);
//			fprintf(stderr,"ret=%d\n",ret);
		}else  {		//无配置文件，读取系统初始化参数
			memset(fname,0,sizeof(fname));
			ret = readFileName(oi,seqno,para_init_save,fname);
//			fprintf(stderr,"read /nor/init的参数文件：  Class %s filelen=%d\n",fname,datalen);
			if(ret==0) {	//文件存在
				ret = block_file_sync(fname,blockdata,datalen,0,0);
			}
		}
		break;
	case para_init_save:
		ret = readFileName(oi,seqno,type,fname);
		fprintf(stderr,"readClass %s filelen=%d\n",fname,datalen);
		if(ret==0) {
			ret = block_file_sync(fname,blockdata,datalen,0,0);
		}
	break;
	case event_record_save:
	case event_current_save:
		ret = readFileName(oi,seqno,type,fname);
		if(ret==0) {
			ret = readCoverFile(fname,blockdata,datalen);
		}
		break;
	}
	//信号量post，注意正常退出
	CloseSem(sem_save);
	return ret;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * 参变量数据存储及读取接口
 * oi: 需要存储OI值
 * blockdata:  需要存储数据, 存储格式为:　有效长度 Data
 * datalen :   需要存储数据长度,不能超过64个字节
 * =-1 ：存储失败
 * */
int saveVariData(OI_698 oi,int coll_seqnum,void *blockdata,int datalen)
{
	int 	ret=-1;
	int		offset=-1,type=-1;
	FILE 	*fp=NULL;
	int	  	fd=0;
	int		blklen=0;
	char	*wbuf=NULL;
	char	filename[FILENAMELEN];
	sem_t   *sem_save=NULL;

	if(blockdata==NULL) {
		fprintf(stderr,"存储数据为空，不可保存\n");
		return -1;
	}
	type = getvarioffset(oi,coll_seqnum,&offset,&blklen);
	fprintf(stderr,"offset=%d ,blklen=%d, type=%d\n",offset,blklen,type);
	if(type == -1) {
		fprintf(stderr,"没有相关OI=%04x的存储信息，不可保存!!!\n",oi);
		return -1;
	}
	if(datalen>=blklen) {
		fprintf(stderr,"存储信息[%d]大于等于限定大小[%d]字节，不可保存!!!\n",datalen,blklen);
		return -1;
	}
	sem_save = InitSem();
	makeSubDir(VARI_DIR);
	memset(&filename,0,sizeof(filename));
	switch(type) {
	case 1:
		memcpy(filename,VARI_DATA,sizeof(VARI_DATA));
		break;
	case 2:
		memcpy(filename,VARI_DATA_TJ,sizeof(VARI_DATA_TJ));
		break;
	}
	if(access(filename,F_OK)!=0)
	{
		fp = fopen(filename, "w+");
		fprintf(stderr,"创建文件 %s\n",filename);
	}else {
		fp = fopen(filename, "r+");
		fprintf(stderr,"替换文件 %s\n",filename);
	}
	if (fp != NULL) {
		if(wbuf==NULL) {
			wbuf = malloc(blklen);
			memset(wbuf,0,blklen);
			wbuf[0] = datalen;
			memcpy(wbuf+1,blockdata,datalen);
			fprintf(stderr,"set to %d, datalen=%d ",offset,datalen);
			fseek(fp, offset, SEEK_SET);
			//fwrite(&datalen,sizeof(int),1,fp);			//数据有效长度
			ret = fwrite(wbuf,blklen,1,fp);			//数据内容
			fd = fileno(fp);
			fsync(fd);
			if(wbuf!=NULL) {
				free(wbuf);
			}
		}else ret = -1;
		fclose(fp);
	} else {
		ret = 0;
	}
	CloseSem(sem_save);
	return ret;
}

/*
 *　　读取数据值
 *　　　  oi: 需要读取的oi值的所有属性值
 *　　　  coll_seqnum: 采集任务中TSA对应配置序号，从０开始
 *　　　　　blockdata:返回数据
 *　　　　　len:　blockdata空间大小，需要申请blockdata申请空间大小为：oad个数×VARI_LEN
 *　　　函数返回值：数据长度 =-1,读取失败
 * */
int  readVariData(OI_698 oi,int coll_seqnum,void *blockdata,int len)
{
	FILE 	*fp=NULL;
	int 	offset=-1,typelen=-1,retlen=-1,readlen=0;
	sem_t   *sem_save=NULL;
	int		blklen=0;
	char	*rbuf=NULL;

	if(len > VARI_LEN) {
		fprintf(stderr,"读取数据长度[%d]大于申请返回数据空间[%d]，返回失败!!!\n",len,VARI_LEN);
		return -1;
	}
	if(blockdata==NULL) {
		fprintf(stderr,"数据空间为空，返回失败!!!\n");
		return -1;
	}
	memset(blockdata,0,len);
	sem_save = InitSem();

	retlen = 0;
	typelen = getvarioffset(oi,coll_seqnum,&offset,&blklen);
	switch(typelen) {
	case -1:
		retlen = -1;
		break;
	case 1:
		fp = fopen(VARI_DATA, "r");
		break;
	case 2:
		fp = fopen(VARI_DATA_TJ, "r");
		break;
	}
	if (fp != NULL) {
		fseek(fp, offset, SEEK_SET);
		memset(blockdata,0,len);
		if(rbuf==NULL) {
			rbuf = malloc(blklen);
			memset(rbuf,0,blklen);
			readlen=fread(rbuf,blklen,1,fp);	//读一个块数据
			if(readlen==1) {
				fprintf(stderr,"rbuf[0]=%d\n",rbuf[0]);
				if(rbuf[0]==0) {
					retlen = 0;
				}else {
					memcpy((char *)blockdata+retlen,&rbuf[1],len);	//第一个字节为有效长度
					retlen+=len;
				}
			}
			if(rbuf!=NULL) {
				fprintf(stderr,"free rbuf\n");
				free(rbuf);
			}
		}
		fclose(fp);
	}
	fprintf(stderr,"retlen=%d\n",retlen);
	CloseSem(sem_save);
	return retlen;
}
/*
 *　　读取数据值
 *　　　  oi: 需要读取的oi值的所有属性值
 *　　　  oadnum: 需要读取oad个数
 *　　　　　blockdata:返回数据
 *　　　　　len:　blockdata空间大小，需要申请blockdata申请空间大小为：oad个数×VARI_LEN
 *　　　函数返回值：数据长度 =-1,读取失败
 * */
//int  readVariData(OI_698 *oi,int oadnum,void *blockdata,int len)
//{
//	FILE 	*fp=NULL;
//	int 	i=0,offset=-1,retlen=-1,readlen=0;
//	sem_t   *sem_save=NULL;
//	INT8U	tmpbuf[VARI_LEN]={};
//
//	if(len > oadnum*VARI_LEN) {
//		fprintf(stderr,"读取数据长度[%d]大于申请返回数据空间[%d]，返回失败!!!\n",len,oadnum*VARI_LEN);
//		return -1;
//	}
//	if(blockdata==NULL) {
//		fprintf(stderr,"数据空间为空，返回失败!!!\n");
//		return -1;
//	}
//	memset(blockdata,0,len);
//	sem_save = InitSem();
//	fp = fopen(VARI_DATA, "r");
//	if (fp != NULL) {
//		retlen = 0;
//		if(oadnum==1) {		//只读取一个，返回实际数据长度
//			offset = getvarioffset(oi[0]);
//			fprintf(stderr,"oi = %04x, offset=%d site=%d\n",oi[0],offset,offset*VARI_LEN);
//			if(offset!=-1) {
//				fseek(fp, offset*VARI_LEN, SEEK_SET);
//				memset(blockdata,0,len);
//				readlen=fread(tmpbuf,VARI_LEN,1,fp);	//读一个块数据
//				fprintf(stderr,"readlen=%d\n",readlen);
//				if(readlen==1) {
//					fprintf(stderr,"tmpbuf[0]=%d\n",tmpbuf[0]);
//					if(tmpbuf[0]==0) {
//						retlen = 0;
//					}else {
//						memcpy((char *)blockdata+retlen,&tmpbuf[1],len);	//第一个字节为有效长度
//						retlen+=len;
//					}
//				}
//			}else retlen=-1;
//		}else {		//目前没有多个读取情况，功能未测试
//			retlen = 0;
//			for(i=0; i < oadnum;i++) {
//				offset = getvarioffset(oi[i]);
//				if(offset!=-1) {
//					fseek(fp, offset*VARI_LEN, SEEK_SET);
//					memset(tmpbuf,0,sizeof(tmpbuf));
//					readlen=fread(tmpbuf,VARI_LEN,1,fp);	//读一个块数据
//					if(readlen==1) {
//						memcpy((char *)blockdata+retlen,&tmpbuf,VARI_LEN);	//第一个字节为有效长度
//						retlen+=tmpbuf[0];
//					}else {
//						memset(blockdata+retlen,0,len);
//						retlen+=VARI_LEN;
//					}
//				}
//			}
//		}
//		fclose(fp);
//	}else
//	{
//		retlen = -1;
//	}
//	fprintf(stderr,"retlen=%d\n",retlen);
//	CloseSem(sem_save);
//	return retlen;
//}


///////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * 计算某个普通采集方案一天需要存几次，是否与采集任务里面的执行频率有关？
 */
//INT16U CalcFreq(CLASS_6015 class6015)
//{
//	INT16U freq=0;
//	INT8U  unit=0;
//	INT16U value=0;
//	switch(class6015.data.type)
//	{
//	case 0://采集当前数据
//	case 1://采集上第n次
//	case 2://按冻结时标采集
//		freq = 1;
//		break;
//	case 3://按时标间隔采集
//		unit = class6015.data.data[0];
//		value = (class6015.data.data[1]<<8) + class6015.data.data[2];
//		break;
//	}
//	if(class6015.data.type == 3)
//	{
//		switch(unit)
//		{
//		case 0:
//			freq = 24*60*60/value;
//			break;
//		case 1:
//			freq = 24*60/value;
//			break;
//		case 2:
//			freq = 24/value;
//			break;
//		case 3:
//		case 4:
//		case 5:
//			freq = 1;
//			break;
//		}
//	}
//	return freq;
//}
//lyl
INT8U datafile_write(char *FileName, void *source, int size, int offset)
{
	FILE *fp=NULL;
	int	  fd=0;
	INT8U res=0;
	int num=0;
	INT8U	*blockdata=NULL;
//	int i=0;

	blockdata = malloc(size);
	if(blockdata!=NULL) {
		memcpy(blockdata,source,size);
	} else {
		return 0;//error
	}

	if(access(FileName,F_OK)!=0)
	{
		fp = fopen((char*) FileName, "w+");
		fprintf(stderr,"创建文件--%s\n",FileName);
	}else {
		fp = fopen((char*) FileName, "r+");
		fprintf(stderr,"替换文件\n");
	}
	if (fp != NULL) {
		fseek(fp, offset, SEEK_SET);
		num = fwrite(blockdata, size,1,fp);
		fd = fileno(fp);
		fsync(fd);
		fclose(fp);
		if(num == 1) {
			res = 1;
		}else res = 0;
	} else {
		res = 0;
	}
	free(blockdata);//add by nl1031
	return res;
}
INT8U datafile_read(char *FileName, void *source, int size, int offset)
{
	FILE 	*fp=NULL;
	int 	num=0,ret=0;
	fp = fopen(FileName, "r");
	if (fp != NULL) {
		fseek(fp, offset, SEEK_SET);
		num=fread(source,1 ,size,fp);
		if(num==(size)) 			//读取了size字节数据
				ret = 1;
			else ret = 0;
		fclose(fp);
	} else
		ret = 0;
	return ret;
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
 * 得到单元中oad的偏移，确定在这个任务里再进来找，否则会出错,没找到暂时将oad_m和oad_r都置位0xee,表示没找到出错
 */
void GetOADPosofUnit(ROAD_ITEM item_road,HEAD_UNIT *head_unit,INT8U unitnum,OAD_INDEX *oad_offset)
{
	int i=0,j=0,datapos=0;
	fprintf(stderr,"oadmr_num=%d,unitnum=%d\n",item_road.oadmr_num,unitnum);
	for(i=0;i<item_road.oadmr_num;i++)//找不到呢
	{
		datapos=0;
		for(j=0;j<unitnum;j++)
		{
//			fprintf(stderr,"j=%d,len=%d,datapos=%d\n",j,head_unit[j].len,datapos);
			memcpy(&oad_offset[i].oad_m,&item_road.oad[i].oad_m,sizeof(OAD));
			memcpy(&oad_offset[i].oad_r,&item_road.oad[i].oad_r,sizeof(OAD));
			if(memcmp(&item_road.oad[i].oad_m,&head_unit[j].oad_m,sizeof(OAD))==0 &&
					memcmp(&item_road.oad[i].oad_r,&head_unit[j].oad_r,sizeof(OAD))==0)
			{
				fprintf(stderr,"\nfind oad %04x%02x%02x-%04x%02x%02x:offset:%d\n",
						item_road.oad[i].oad_m.OI,item_road.oad[i].oad_m.attflg,item_road.oad[i].oad_m.attrindex,
						item_road.oad[i].oad_r.OI,item_road.oad[i].oad_r.attflg,item_road.oad[i].oad_r.attrindex,
						datapos);
				oad_offset[i].offset = datapos;
				oad_offset[i].len = head_unit[j].len;
			}
			else {
				datapos += head_unit[j].len;
				fprintf(stderr,"datapos = %d\n",datapos);
			}
		}
	}
}
///*
// * 返回位置和长度
// */
//int GetPosofOAD(INT8U *file_buf,OAD oad_master,OAD oad_relate,HEAD_UNIT *head_unit,INT8U unitnum,INT8U *tmpbuf)
//{
//	int i=0,j=0,datapos=0,curlen=0,valid_cnt = 0;
//	OAD oad_hm,oad_hr;
////	fprintf(stderr,"\n--GetPosofOAD--unitnum=%d\n",unitnum);
//	memset(&oad_hm,0xee,sizeof(OAD));
//	memset(&oad_hr,0xee,sizeof(OAD));
//	for(i=0;i<unitnum;i++)
//	{
//		curlen = GetFileOadLen(head_unit[i].len[1],head_unit[i].num[0]);
////		fprintf(stderr,"\n%02x %02x\n",head_unit[i].num[1],head_unit[i].num[0]);
////		fprintf(stderr,"\n-head_unit[%d].type=%d-GetPosofOAD--curlen=%d,valid_cnt=%d\n",i,head_unit[i].type,curlen,valid_cnt);
//		if(head_unit[i].type == 1)
//		{
//			memcpy(&oad_hr,&head_unit[i].oad,sizeof(OAD));
//			memset(&oad_hm,0xee,sizeof(OAD));
//			valid_cnt = curlen;
//			continue;
//		}
//		if(valid_cnt==0)//关联oad失效
//			memset(&oad_hr,0xee,sizeof(OAD));
//		if(valid_cnt>0)
//			valid_cnt--;//road包括cnt个oad，在非零前，关联oad有效
//		if(head_unit[i].type == 0)
//		{
//			memcpy(&oad_hm,&head_unit[i].oad,sizeof(OAD));
//		}
////		fprintf(stderr,"\nhead master oad:%04x%02x%02x\n",oad_hm.OI,oad_hm.attflg,oad_hm.attrindex);
////		fprintf(stderr,"\nhead relate oad:%04x%02x%02x\n",oad_hr.OI,oad_hr.attflg,oad_hr.attrindex);
////		fprintf(stderr,"\nbaow master oad:%04x%02x%02x\n",oad_master.OI,oad_master.attflg,oad_master.attrindex);
////		fprintf(stderr,"\nbaow relate oad:%04x%02x%02x\n",oad_relate.OI,oad_relate.attflg,oad_relate.attrindex);
//		if(memcmp(&oad_hm,&oad_master,sizeof(OAD))==0 && memcmp(&oad_hr,&oad_relate,sizeof(OAD))==0)
//		{
//			fprintf(stderr,"\n-----oad equal!\n");
//			memcpy(tmpbuf,&file_buf[datapos],curlen);
////			fprintf(stderr,"\n文件中的数据:");
////			for(j=0;j<400;j++)
////			{
////				if(j%20==0)
////					fprintf(stderr,"\n");
////				fprintf(stderr," %02x",file_buf[j]);
////			}
//			fprintf(stderr,"\n");
//			fprintf(stderr,"\n文件中取出的数据(%d)datapos=%d:",curlen,datapos);
//			for(j=0;j<curlen;j++)
//				fprintf(stderr," %02x",tmpbuf[j]);
//			fprintf(stderr,"\n");
//			return curlen;
//		}
////		else
////			fprintf(stderr,"\n-----oad not equal!\n");
//		if(head_unit[i].type == 0)
//			datapos += curlen;
//
//		if(valid_cnt==0)
//		{
//			memset(&oad_hr,0xee,sizeof(OAD));
//		}
//
//	}
//	return 0;
//}
INT16U CalcMinFromZero(INT8U hour,INT8U min)
{
	INT16U minfromzero = 0;
	minfromzero = hour;
	minfromzero = minfromzero*60 +min;
	return minfromzero;
}
INT8U CalcKBType(INT8U type)
{
	INT8U ret = 0xee;
	switch(type)
	{
	case 0:
		ret = 0b00000010;//前闭后开
		break;
	case 1:
		ret = 0b00000001;//前开后闭
		break;
	case 2:
		ret = 0b00000011;//前闭后闭
		break;
	case 3:
		ret = 0b00000000;//前开后开
		break;
	default:break;
	}
	return ret;//不合法
}
INT16U CalcFreq(TI runti,CLASS_6015 class6015,INT16U startmin,INT16U endmin,INT16U *sec_freq)//不管开闭
{
	INT16U rate = 0;//倍率
	INT16U sec_unit = 0;
	INT8U  inval_flg = 0;
	if(class6015.cjtype == 3 || class6015.cjtype == 0)//按时标间隔采集
	{
		if(endmin <= startmin || runti.units > 2)
			return 0;//无效设置
		switch(runti.units)
		{
		case 0://秒
			rate = 1;
			if(runti.interval >= 60)//如果就要设置90秒呢
				inval_flg = 1;
			break;
		case 1://分钟
			rate = 60;
			if(runti.interval >= 60)//如果就要设置90分钟呢
				inval_flg = 1;
			break;
		case 2://小时
			rate = 3600;
			if(runti.interval >= 60)//如果就要设置1个半小时呢
				inval_flg = 1;
			break;
		default:
			break;//没有这种情况
		}
		if(inval_flg == 1)//todo
			return 0;
		sec_unit = (runti.interval * rate);
		fprintf(stderr,"\nsec_unit = %d,interval=%d(%d)\n",sec_unit,runti.interval,runti.units);
		*sec_freq = sec_unit;
		fprintf(stderr,"\n---@@@-开始分钟数：%d 结束分钟数：%d 间隔秒数%d 次数:%d---%d\n",startmin,endmin,sec_unit,((endmin-startmin)*60)/sec_unit,((endmin-startmin)*60)/sec_unit+1);
		return ((endmin-startmin)*60)/sec_unit+1;
	}
	return 1;
}
//读取taskid相应的配置结构体，return 1成功，0失败
INT8U ReadTaskInfo(INT8U taskid,TASKSET_INFO *tasknor_info)//读取普通采集方案配置
{
	CLASS_6015	class6015={};
	CLASS_6013	class6013={};
	memset(tasknor_info,0x00,sizeof(TASKSET_INFO));
	memset(&class6013,0,sizeof(CLASS_6013));
	memset(&class6015,0,sizeof(CLASS_6015));
	if(readCoverClass(0x6013,taskid,&class6013,sizeof(class6013),coll_para_save) == 1)
	{
		if(class6013.cjtype != 1 || class6013.state != 1)//
			return 0;
		if(readCoverClass(0x6015,class6013.sernum,&class6015,sizeof(CLASS_6015),coll_para_save) == 1)
		{
			tasknor_info->starthour = class6013.runtime.runtime[0].beginHour;
			tasknor_info->startmin = class6013.runtime.runtime[0].beginMin;//按照设置一个时段来
			tasknor_info->endhour = class6013.runtime.runtime[0].endHour;
			tasknor_info->endmin = class6013.runtime.runtime[0].endMin;//按照设置一个时段来
			fprintf(stderr,"\n任务开始结束时间：%d:%d--%d:%d\n",tasknor_info->starthour,tasknor_info->startmin,tasknor_info->endhour,tasknor_info->endmin);
			tasknor_info->runtime = CalcFreq(class6013.interval,class6015,tasknor_info->starthour*60+tasknor_info->startmin,tasknor_info->endhour*60+tasknor_info->endmin,&tasknor_info->freq);
			fprintf(stderr,"\n---@@@---任务%d执行次数%d\n",taskid,tasknor_info->runtime);
			tasknor_info->KBtype = CalcKBType(class6013.runtime.type);
			fprintf(stderr,"\n---@@@---开闭方式%d\n",tasknor_info->KBtype);
			tasknor_info->memdep = class6015.deepsize;
			fprintf(stderr,"\n---@@@---存储深度%d\n",class6015.deepsize);
			memcpy(&tasknor_info->csds,&class6015.csds,sizeof(CSD_ARRAYTYPE));
			fprintf(stderr,"\n---@@@---返回1\n");
			return 1;
		}
	}
	return 0;
}
typedef struct {
	INT16U onenum;//单元个数
	INT16U onelen;//单元长度
	INT16U frmindex;//发送索引
}FRM_HEAD;
/*
 *databuf前两个字节为本帧长度,datalen为数据长度
 */
void saveonefrm(INT8U *frmbuf,INT16U frmlen)
{
	char fname[60];
	FILE *fp = NULL;
	memset(fname,0x00,60);
	sprintf(fname,"/nand/frm.dat");
	fp = fopen(fname,"a+");//附加形式打开
	if(fp != NULL)
	{

		fwrite(frmbuf,frmlen,1,fp);
	}
	else
		fprintf(stderr,"\nopen file /nand/frm.dat fail!!!!\n");
}
/*
 * unitlen：单元长度 unitnum_file：文件里最后一个单元索引
 */
void savefrm(INT16U unitlen,INT16U unitnum_file,INT8U lastflg,INT8U *databuf,int datalen)
{
	FRM_HEAD frm_head;
	char fname[60];
	memset(fname,0x00,60);
	memset(&frm_head,0x00,sizeof(FRM_HEAD));
	sprintf(fname,"/nand/frm.dat");
	if(unitnum_file == 0)//第一帧，组文件头
	{
		frm_head.onelen = unitlen;
		frm_head.frmindex = 0;
		datafile_write(fname, &frm_head, sizeof(FRM_HEAD), 0);//存储文件头
	}
	if(lastflg == 1)
	{
		frm_head.onenum = unitnum_file;//存储完成写为单元数量,未完成为0
		datafile_write(fname, &frm_head, sizeof(FRM_HEAD), 0);//更改文件头
		return;//只修改分帧完成标志
	}
	fprintf(stderr,"\n存储位置：%d,数据长度%d\n",unitlen*unitnum_file+sizeof(FRM_HEAD),datalen);
	datafile_write(fname, databuf, datalen, unitlen*unitnum_file+sizeof(FRM_HEAD));//存储数据文件
}
/*
 * 计算某个OI的数据长度，指针对抄表数据 todo 先写个简单的，以后完善 而且没有考虑费率
 * attr_flg:0 全部属性 非0 一个属性  例如20000200 则为全部属性 20000201则为一个属性
 */
INT16U CalcOIDataLen(OI_698 oi,INT8U attr_flg)
{
	FILE *fp;
	char ln[60];
	char lnf[4];
	INT16U oi_len=0;
	INT8U ic_type = 1;

	if(oi>=0x0000 && oi<0x2000)
	{
		if(attr_flg == 0)
			return 27;//长度4+1个字节数据类型
		else
			return 5;
	}
//	if(oi == 2140 || oi == 2141)//struct 类型要在原长度基础上+3
//		return (11+3)*(MET_RATE+1)+1+1;
	fp = fopen("/nor/config/OI_TYPE.cfg","r");
	if(fp == NULL)
	{
		fprintf(stderr,"\nOI_TYPE.cfg do not exist,hard error!!\n");
		return 0;
	}
	while(1)
	{
		memset(ln,0x00,60);
		fscanf(fp,"%s",ln);
		if(strncmp(ln,"begin",5) == 0) continue;
		if(strncmp(ln,"end",3) == 0) break;
		if(strncmp(ln,"//",2) == 0) continue;

		memset(lnf,0x00,4);
		memcpy(lnf,&ln[0],4);

		if(strtol(lnf,NULL,16) != oi)
			continue;
		memset(lnf,0x00,4);
		memcpy(lnf,&ln[8],3);
		oi_len = strtol(lnf,NULL,10)+1;//返回长度+1个字节数据类型
		memset(lnf,0x00,4);
		memcpy(lnf,&ln[12],2);
		ic_type = strtol(lnf,NULL,10);
		break;
	}
	fclose(fp);
	if(oi_len != 0 && ic_type != 0)
	{
		switch(ic_type)
		{
		case 1:
		case 2:
			if(attr_flg == 0)
				oi_len = oi_len*(MET_RATE+1)+1+1;//+类型+个数
			break;
		case 3:
			if(attr_flg == 0)
				oi_len = oi_len*3+1+1;//三相
			break;
		case 4:
			if(attr_flg == 0)
				oi_len = oi_len*4+1+1;//总及分项
			break;
		default:
			break;
		}
	}
	return oi_len;
}
///*
// * 读取抄表数据，读取某个测量点某任务某天一整块数据，放在内存里，根据需要提取数据,并根据csd组报文
// */
//int ComposeSendBuff(TS *ts,INT8U seletype,INT8U taskid,TSA *tsa_con,INT8U tsa_num,CSD_ARRAYTYPE csds,INT8U *SendBuf)
//{
//	OAD  oadm,oadr;
//	FILE *fp  = NULL;
//	INT16U headlen=0,blocklen=0,unitnum=0,sendindex=0,retlen=0,schpos=0,unitlen=0,frmunitcnt=0,frmunitnum_old=0,frmflg=0;
//	INT8U *databuf_tmp=NULL;
//	INT8U tmpbuf[256];
//	INT8U headl[2],blockl[2];
//	int i=0,j=0,k=0,m=0;
//	HEAD_UNIT *head_unit = NULL;
//	TASKSET_INFO tasknor_info;
//	TS ts_now;
////	TSGet(&ts_now);
//	char	fname[FILENAMELEN]={};
//	if(ReadTaskInfo(taskid,&tasknor_info)!=1)
//		return 0;
//
//	memcpy(&ts_now,ts,sizeof(TS));
//	getTaskFileName(taskid,ts_now,fname);
//	fp = fopen(fname,"r");
//	if(fp == NULL)//文件没内容 组文件头，如果文件已存在，提取文件头信息
//	{
//		fprintf(stderr,"\n-----file %s not exist\n",fname);
//		return 0;
//	}
//	else
//	{
//		fread(headl,2,1,fp);
//		headlen = headl[0];
//		headlen = (headl[0]<<8) + headl[1];
//		fread(&blockl,2,1,fp);
////		fprintf(stderr,"\nblocklen=%02x %02x\n",blockl[1],blockl[0]);
//		blocklen = blockl[0];
//		blocklen = (blockl[0]<<8) + blockl[1];
//		unitnum = (headlen-4)/sizeof(HEAD_UNIT);
//		databuf_tmp = (INT8U *)malloc(blocklen);
//		if(databuf_tmp == NULL)
//		{
//			fprintf(stderr,"\n分配内存给databuf_tmp失败！\n");
//			return 0;
//		}
//
//		head_unit = (HEAD_UNIT *)malloc(headlen-4);
//		if(head_unit == NULL)
//		{
//			fprintf(stderr,"\n分配内存给head_unit失败！\n");
//			return 0;
//		}
//		fprintf(stderr,"\n-1-headlen=%d\n",headlen);
//		fread(head_unit,headlen-4,1,fp);
//
//		fprintf(stderr,"\n--headlen=%d\n",headlen);
////		fseek(fp,headlen,SEEK_SET);//跳过文件头
//		fprintf(stderr,"\n--seek=%d\n",fseek(fp,headlen,SEEK_SET));
//		while(!feof(fp))//找存储结构位置
//		{
////			fprintf(stderr,"\n-------------8---blocklen=%d,headlen=%d,tsa_num=%d\n",blocklen,headlen,tsa_num);
//			//00 00 00 00 00 00 00 00 07 05 00 00 00 00 00 01
//			//00 00 00 00 00 00 00 00 00 07 05 00 00 00 00 00 01
//			memset(databuf_tmp,0xee,blocklen);
//			if(fread(databuf_tmp,blocklen,1,fp) == 0)
//				break;
//			for(i=0;i<tsa_num;i++)
//			{
//				if(memcmp(&databuf_tmp[schpos+1],&tsa_con[i].addr[0],17)!=0)
//					continue;
////				fprintf(stderr,"\n@@@find addr:\n");
////				fprintf(stderr,"\naddr1:%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
////						databuf_tmp[schpos+16],databuf_tmp[schpos+15],databuf_tmp[schpos+14],databuf_tmp[schpos+13],
////						databuf_tmp[schpos+12],databuf_tmp[schpos+11],databuf_tmp[schpos+10],databuf_tmp[schpos+9],
////						databuf_tmp[schpos+8],databuf_tmp[schpos+7],databuf_tmp[schpos+6],	databuf_tmp[schpos+5],
////						databuf_tmp[schpos+4],databuf_tmp[schpos+3],databuf_tmp[schpos+2],databuf_tmp[schpos+1]);
////				fprintf(stderr,"\n1addr2:%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
////						tsa_con[i].addr[16],tsa_con[i].addr[15],tsa_con[i].addr[14],tsa_con[i].addr[13],
////						tsa_con[i].addr[12],tsa_con[i].addr[11],tsa_con[i].addr[10],tsa_con[i].addr[9],
////						tsa_con[i].addr[8],tsa_con[i].addr[7],tsa_con[i].addr[6],	tsa_con[i].addr[5],
////						tsa_con[i].addr[4],tsa_con[i].addr[3],tsa_con[i].addr[2],tsa_con[i].addr[1],tsa_con[i].addr[0]);
//				for(m=0;m<tasknor_info.runtime;m++)
//				{
//					schpos = m*blocklen/tasknor_info.runtime;
//					for(j=0;j<csds.num;j++)
//					{
////							fprintf(stderr,"\n-------%d:(type=%d)\n",j,csds.csd[j].type);
//						if(csds.csd[j].type != 0 && csds.csd[j].type != 1)
//							continue;
//						if(csds.csd[j].type == 1)
//						{
//							SendBuf[sendindex++] = 0x01;//array
//							SendBuf[sendindex++] = csds.csd[j].csd.road.num;
//							for(k=0;k<csds.csd[j].csd.road.num;k++)
//							{
//								memset(tmpbuf,0x00,256);
//								memcpy(&oadm,&csds.csd[j].csd.road.oads[k],sizeof(OAD));
//								memcpy(&oadr,&csds.csd[j].csd.road.oad,sizeof(OAD));
////									fprintf(stderr,"\nmaster oad:%04x%02x%02x\n",oadm.OI,oadm.attflg,oadm.attrindex);
////									fprintf(stderr,"\nrelate oad:%04x%02x%02x\n",oadr.OI,oadr.attflg,oadr.attrindex);
//								if((retlen = GetPosofOAD(&databuf_tmp[schpos],oadm,oadr,head_unit,unitnum,tmpbuf))==0)
//									SendBuf[sendindex++] = 0;
//								else
//								{
////										fprintf(stderr,"\n--type=1--tmpbuf[0]=%02x\n",tmpbuf[0]);
//									switch(tmpbuf[0])
//									{
//									case 0:
//										SendBuf[sendindex++] = 0;
//										break;
//									case 1://array
//										retlen = CalcOIDataLen(oadm.OI,1);
//										retlen = retlen*tmpbuf[1]+2;//2代表一个array类型加一个个数
//										memcpy(&SendBuf[sendindex],tmpbuf,retlen);
//										sendindex += retlen;
//										break;
//									default:
//										memcpy(&SendBuf[sendindex],tmpbuf,retlen);
//										sendindex += retlen;
//										break;
//									}
////									if(tmpbuf[0]==0)//NULL
////										SendBuf[sendindex++] = 0;
////									else
////									{
////										memcpy(&SendBuf[sendindex],tmpbuf,retlen);
////										sendindex += retlen;
////									}
//								}
////									fprintf(stderr,"\n---1----k=%d retlen=%d\n",k,retlen);
//							}
//						}
//						if(csds.csd[j].type == 0)
//						{
//							if(csds.csd[j].csd.oad.OI == 0x202a)
//							{
//								SendBuf[sendindex++]=0x55;
//								for(k=1;k<17;k++)
//								{
////										fprintf(stderr,"\n--databuf_tmp[%d+%d]=%d\n",schpos,k,databuf_tmp[schpos+k]);
//									if(databuf_tmp[schpos+k]==0)
//										continue;
//									memcpy(&SendBuf[sendindex],&databuf_tmp[schpos+k],databuf_tmp[schpos+k]+1);
//									sendindex += databuf_tmp[schpos+k]+1;
//									break;
//								}
//								continue;
//							}
//							memset(tmpbuf,0x00,256);
//							memcpy(&oadm,&csds.csd[j].csd.oad,sizeof(OAD));
//							memset(&oadr,0xee,sizeof(OAD));
//							if((retlen = GetPosofOAD(&databuf_tmp[schpos],oadm,oadr,head_unit,unitnum,tmpbuf))==0)
//								SendBuf[sendindex++] = 0;
//							else
//							{
////									fprintf(stderr,"\n--type=0--tmpbuf[0]=%02x\n",tmpbuf[0]);
//								switch(tmpbuf[0])
//								{
//								case 0:
//									SendBuf[sendindex++] = 0;
//									break;
//								case 1://array
//									retlen = CalcOIDataLen(oadm.OI,1);
//									retlen = retlen*tmpbuf[1]+2;//2代表一个array类型加一个个数
//									memcpy(&SendBuf[sendindex],tmpbuf,retlen);
//									sendindex += retlen;
//									break;
//								default:
//									memcpy(&SendBuf[sendindex],tmpbuf,retlen);
//									sendindex += retlen;
//									break;
//								}
////								if(tmpbuf[0]==0)//NULL
////									SendBuf[sendindex++] = 0;
////								else
////								{
////									memcpy(&SendBuf[sendindex],tmpbuf,retlen);
////									sendindex += retlen;
////								}
//							}
////								fprintf(stderr,"\n---0----k=%d retlen=%d\n",k,retlen);
//						}
//					}
////						return sendindex;
//					fprintf(stderr,"\n-------sendindex = %d\n",sendindex);
//
//					if(frmunitcnt == 0)
//					{
//						unitlen = sendindex;//每个单元长度相同
//					}
//					frmunitcnt++;
//					if(sendindex > 1000)//1000为分帧最大长度，以后根据参数决定大小
//					{
//						frmflg = 1;//需要分帧
//						fprintf(stderr,"\n---111----unitlen = %d,frmunitnum_old=%d\n",unitlen,frmunitnum_old);
//						savefrm(unitlen,frmunitnum_old,0,SendBuf,sendindex);
//						sendindex = 0;
//						frmunitnum_old = frmunitcnt;
//					}
//					continue;
//				}
//			}
//		}
//	}
//	free(databuf_tmp);
//	free(head_unit);
//	if(frmflg==1)
//	{
//		fprintf(stderr,"\n---分帧了\n");
//		sendindex = 0xffff;//如果分帧了,返回0xffff
//		if(sendindex > 0)
//			savefrm(unitlen,frmunitcnt,0,SendBuf,sendindex);
//		savefrm(unitlen,frmunitcnt,1,SendBuf,sendindex);//修改分帧完成标志
//	}
//	return sendindex;
//}
INT16U GetTSANum()
{
	INT16U i=0,TSA_num=0;
	CLASS_6001 meter = { };
	for (i = 1; i < 1200; i++) {
		if (readParaClass(0x6000, &meter, i) == 1) {

			if (meter.sernum != 0 && meter.sernum != 0xffff) {
				if(meter.basicinfo.port.attrindex == 1 || meter.basicinfo.port.attrindex == 2)
				{
					fprintf(stderr,"\n-0--pointno=%d\n",i);
					TSA_num++;
				}
			}
		}
	}
	return TSA_num;
}
INT16U GetTSACon(MY_MS meters,TSA *tsa_con,INT16U tsa_num)
{
	INT16U i=0,TSA_num=0;
	CLASS_6001 meter = { };
	for (i = 1; i < tsa_num; i++) {
		if (readParaClass(0x6000, &meter, i) == 1) {

			if (meter.sernum != 0 && meter.sernum != 0xffff) {
				if(meter.basicinfo.port.attrindex == 1 || meter.basicinfo.port.attrindex == 2)
				{
					fprintf(stderr,"\n-1--pointno=%d\n",i);
					memcpy(&tsa_con[TSA_num],&meter.basicinfo.addr,sizeof(TSA));
					fprintf(stderr,"\ncpy addr:%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
							tsa_con[TSA_num].addr[16],tsa_con[TSA_num].addr[15],tsa_con[TSA_num].addr[14],tsa_con[TSA_num].addr[13],
							tsa_con[TSA_num].addr[12],tsa_con[TSA_num].addr[11],tsa_con[TSA_num].addr[10],tsa_con[TSA_num].addr[9],
							tsa_con[TSA_num].addr[8],tsa_con[TSA_num].addr[7],tsa_con[TSA_num].addr[6],	tsa_con[TSA_num].addr[5],
							tsa_con[TSA_num].addr[4],tsa_con[TSA_num].addr[3],tsa_con[TSA_num].addr[2],tsa_con[TSA_num].addr[1],tsa_con[TSA_num].addr[0]);
					fprintf(stderr,"\ncpy addr:%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
							meter.basicinfo.addr.addr[16],meter.basicinfo.addr.addr[15],meter.basicinfo.addr.addr[14],meter.basicinfo.addr.addr[13],
							meter.basicinfo.addr.addr[12],meter.basicinfo.addr.addr[11],meter.basicinfo.addr.addr[10],meter.basicinfo.addr.addr[9],
							meter.basicinfo.addr.addr[8],meter.basicinfo.addr.addr[7],meter.basicinfo.addr.addr[6],	meter.basicinfo.addr.addr[5],
							meter.basicinfo.addr.addr[4],meter.basicinfo.addr.addr[3],meter.basicinfo.addr.addr[2],meter.basicinfo.addr.addr[1],meter.basicinfo.addr.addr[0]);
					TSA_num++;
				}
			}
		}
	}
	return TSA_num;
}
INT8U GetTaskidFromCSDs(CSD_ARRAYTYPE csds,ROAD_ITEM *item_road)
{
	CLASS_6015	class6015={};
	CLASS_6013	class6013={};
	int i=0,j=0,mm=0,nn=0;
	INT8U taskno=0;

	if(csds.num > MY_CSD_NUM)//超了
		csds.num = MY_CSD_NUM;
	for(i=0;i<csds.num;i++)
	{
		switch(csds.csd[i].type)
		{
		case 0://OAD类型，第一个oad为0x00000000，第二个oad为OAD
//			if(csds.csd[i].csd.oad.OI == 0x202a || csds.csd[i].csd.oad.OI == 0x6040 ||//时标和地址不统计在内
//					csds.csd[i].csd.oad.OI == 0x6041 || csds.csd[i].csd.oad.OI == 0x6042)
//				break;
			item_road->oad[item_road->oadmr_num].oad_m.OI=0x0000;
			item_road->oad[item_road->oadmr_num].oad_m.attflg=0x00;
			item_road->oad[item_road->oadmr_num].oad_m.attrindex=0x00;
			memcpy(&item_road->oad[item_road->oadmr_num].oad_r,&csds.csd[i].csd.oad,sizeof(OAD));
			item_road->oadmr_num++;
			break;
		case 1:
			if(csds.csd[i].csd.road.num > ROAD_OADS_NUM)
				csds.csd[i].csd.road.num = ROAD_OADS_NUM;
			for(j=0;j<csds.csd[i].csd.road.num;j++)
			{
				memcpy(&item_road->oad[item_road->oadmr_num].oad_m,&csds.csd[i].csd.road.oad,sizeof(OAD));
				memcpy(&item_road->oad[item_road->oadmr_num].oad_r,&csds.csd[i].csd.road.oads[j],sizeof(OAD));
				item_road->oadmr_num++;
			}
			break;
		default:break;
		}
	}
	memset(&class6013,0,sizeof(CLASS_6013));
	memset(&class6015,0,sizeof(CLASS_6015));
	for(i=0;i<256;i++)//先比较有没有跟现成采集方案匹配的，有直接返回taskid，没有返回0
	{
		if(readCoverClass(0x6013,i+1,&class6013,sizeof(class6013),coll_para_save) == 1)
		{
			if(class6013.cjtype != 1 || class6013.state != 1)//
				continue;
			if(readCoverClass(0x6015,class6013.sernum,&class6015,sizeof(CLASS_6015),coll_para_save) == 1)
			{
				for(j=0;j<class6015.csds.num;j++)
				{
					for(mm=0;mm<item_road->oadmr_num;mm++)
					{
						switch(class6015.csds.csd[j].type)
						{
						case 0:
							if(item_road->oad[mm].oad_m.OI == 0x0000)//都为oad类型
							{
								if(memcmp(&item_road->oad[mm].oad_r,&class6015.csds.csd[j].csd.oad,sizeof(OAD))==0)
									item_road->oad[mm].taskid = i+1;
							}
							break;
						case 1:
							if(memcmp(&item_road->oad[mm].oad_m,&class6015.csds.csd[j].csd.road.oad,sizeof(OAD))==0)//
							{
								for(nn=0;nn<class6015.csds.csd[j].csd.road.num;nn++)
								{
									if(memcmp(&item_road->oad[mm].oad_r,&class6015.csds.csd[j].csd.road.oads[nn],sizeof(OAD))==0)
										item_road->oad[mm].taskid = i+1;
								}
							}
							break;
						default:break;
						}
					}
				}
			}
		}
	}
	for(i=0;i<item_road->oadmr_num;i++)
	{
		if(taskno != 0 && taskno != item_road->oad[i].taskid)
		{
			taskno = 0;
			break;
		}
		taskno = item_road->oad[i].taskid;
	}
	return taskno;
}
/*
 * 得到时间区间，某年月日，某点某分到某点某分
 */
typedef struct {
	ComBCD4 year;
	ComBCD2 month;
	ComBCD2 day;
	ComBCD2 start_hour;
	ComBCD2 start_min;
	ComBCD2 end_hour;
	ComBCD2 end_min;
	INT8U last_time;//上几次，针对selector10
}ZC_TIMEINTERVL;//招测时间段
INT8U getTimeInterval(RSD select,INT8U selectype,ZC_TIMEINTERVL *timeinte)
{
	TS ts_now;
	TSGet(&ts_now);
	switch(selectype)
	{
	case 5:
		timeinte->year = select.selec5.collect_save.year;
		timeinte->month = select.selec5.collect_save.month;
		timeinte->day = select.selec5.collect_save.day;
		timeinte->start_hour = select.selec5.collect_save.hour;
		timeinte->start_min = select.selec5.collect_save.min;
		timeinte->end_hour = select.selec5.collect_save.hour;
		timeinte->end_min = select.selec5.collect_save.min;
		break;
	case 7:
		if(select.selec7.collect_save_star.year.data != select.selec7.collect_save_finish.year.data ||
				select.selec7.collect_save_star.month.data != select.selec7.collect_save_finish.month.data ||
				select.selec7.collect_save_star.day.data != select.selec7.collect_save_finish.day.data)
			return 1;//暂时不支持跨日招测
		timeinte->year = select.selec7.collect_save_star.year;
		timeinte->month = select.selec7.collect_save_star.month;
		timeinte->day = select.selec7.collect_save_star.day;
		timeinte->start_hour = select.selec7.collect_save_star.hour;
		timeinte->start_min = select.selec7.collect_save_star.min;
		timeinte->end_hour = select.selec7.collect_save_finish.hour;
		timeinte->end_min = select.selec7.collect_save_finish.min;
		break;
	case 10://上报上几次
		select.selec10.recordn=0;//上几次
		timeinte->year.data = ts_now.Year;
		timeinte->month.data = ts_now.Month;
		timeinte->day.data = ts_now.Day;
		timeinte->start_hour.data = ts_now.Hour;
		timeinte->start_min.data = ts_now.Minute;
		timeinte->end_hour.data = ts_now.Hour;
		timeinte->end_min.data = ts_now.Minute;
		break;
	default:break;
	}
	return 0;
}
INT16U getTSASE4(MS ms,TSA *tsa)
{
	INT16U i=0,TSA_num=0;
	CLASS_6001 meter = { };
	for(i=0;i<ms.configSerial[0];i++)//第0个表示个数
	{
		if (readParaClass(0x6000, &meter, ms.configSerial[i+1]) == 1) {

			if (meter.sernum != 0 && meter.sernum != 0xffff) {
				if(meter.basicinfo.port.attrindex == 1 || meter.basicinfo.port.attrindex == 2)
				{
					memcpy(&tsa[TSA_num],&meter.basicinfo.addr,sizeof(TSA));
					TSA_num++;
				}
			}
		}
	}
	return TSA_num;
}

//INT8U getTSAInterval(RSD select,INT8U selectype,TSA *tsa_group)
//{
//	TS ts_now;
//	TSGet(&ts_now);
//	MY_MS ms_curr;
//	INT16U tsa_num = 0;
//	memset(&ms_curr,0x00,sizeof(MY_MS));
////	INT16U tsa_num = getFileRecordNum(0x6000);
////	if(tsa_num == 0)//没设置测量点
////	{
////		fprintf(stderr,"\n no document!!!\n");
////		return 0;
////	}
////	else
////		fprintf(stderr,"\n document num : %d\n",tsa_num);
//	switch(selectype)
//	{
//	case 5:
//		break;
//	case 7:
//		break;
//	case 10://上报上几次
//		memcpy(&ms_curr,&select.selec10.meters,sizeof(MY_MS));
//		break;
//	default:break;
//	}
//	switch(ms_curr.mstype)
//	{
//	case 0:
//		break;
//	case 1:
//		break;
//	case 2:
//		break;
//	case 3:
//		break;
//	case 4://SEQUENCE OF long-unsigned
//		tsa_num = getTSASE4(ms_curr.ms,tsa_group);
//		break;
//	case 5:
//		break;
//	case 6:
//		break;
//	case 7:
//		break;
//	default:break;
//	}
//	return tsa_num;
//}
FILE* opendatafile(INT8U taskid)
{
	FILE *fp = NULL;
	char	fname[FILENAMELEN]={};
	TS ts_now;
	TSGet(&ts_now);
	getTaskFileName(taskid,ts_now,fname);//得到要抄读的文件名称
	fprintf(stderr,"fname=%s\n",fname);
	fp =fopen(fname,"r");
	return fp;
}
FILE* openFramefile()
{
	FILE *fp = NULL;

	if (access("/nand/frmdata",0)==0)
	{
		fp = fopen("/nand/frmdata","w");
		fclose(fp);
	}
	fp = fopen("/nand/frmdata","a+");
	return fp;
}
void saveOneFrame(INT8U *buf,int len,FILE *fp)
{
	int  fd=0;
	if(fp != NULL)
	{
		fwrite(buf,len,1,fp);
		fd = fileno(fp);
		fsync(fd);
	}
	return ;
}
/*
 * 读取文件头长度和块数据长度
 */
void ReadFileHeadLen(FILE *fp,int *headlen,int *blocklen)
{
	INT16U headlength=0,blocklength=0;
	fread(&headlength,2,1,fp);
	*headlen = ((headlength>>8)+((headlength&0xff)<<8));
	fread(&blocklength,2,1,fp);
	*blocklen = ((blocklength>>8)+((blocklength&0xff)<<8));
}

int findTsa(TSA tsa,FILE *fp,int headsize,int blocksize)
{
	int i=0;

	rewind(fp);
	fseek(fp,headsize,SEEK_CUR);
	INT8U  tsa_tmp[TSA_LEN + 1];
	int offset = headsize;

	fprintf(stderr,"\n目的tsa addr: %d-",tsa.addr[0]);
	for(i=0;i<(tsa.addr[0]+1);i++) {
		fprintf(stderr,"-%02x",tsa.addr[i]);
	}
	while(!feof(fp))//找存储结构位置
	{
		if(fread(tsa_tmp,TSA_LEN + 1,1,fp)==0)
		{
			return 0;
		}
		fprintf(stderr,"\nnow addr: %d-",tsa.addr[0]);
		for(i=0;i<(tsa.addr[0]+1);i++) {
			fprintf(stderr,"-%02x",tsa_tmp[i]);
		}
		if(memcmp(&tsa_tmp[1],&tsa.addr[0],tsa.addr[0])==0)
		{
			fprintf(stderr,"\nfind addr: %d-",tsa.addr[0]);
			for(i=0;i<(tsa.addr[0]+1);i++) {
				fprintf(stderr,"-%02x",tsa.addr[i]);
			}
			break;
		}
		else
			offset += blocksize;
		fseek(fp,offset,SEEK_SET);
	}
	return offset;
}
/*
 * 得到某条记录的偏移
 */
int findrecord(int offsetTsa,int recordlen,int recordno)
{
	int recordoffset=0;
	recordoffset = offsetTsa+recordno*recordlen;
	fprintf(stderr,"\ntsa偏移：%d 查找序号%d：偏移%d\n",offsetTsa,recordno,recordoffset);
	return recordoffset;
}
int getrecordno(INT8U starthour,INT8U startmin,int interval)
{
	TS ts_now;
	int recordno = 0;
	TSGet(&ts_now);
	fprintf(stderr,"\ninterval = %d\n",interval);
	recordno = (ts_now.Hour*60 + ts_now.Minute) - (starthour*60 + startmin);
	if(interval!=0) {
		recordno = recordno/(interval/60);
	}else recordno = 1;		//冻结抄读
	fprintf(stderr,"\n当前：%d:%d 任务开始：%d:%d 任务间隔:%d 记录序号%d\n",ts_now.Hour,ts_now.Minute,starthour,startmin,interval,recordno);
	return recordno;
}
void printRecordBytes(INT8U *data,int datalen)
{
	int i=0;
	fprintf(stderr,"\n(%d):",datalen);
	for(i=0;i<datalen;i++)
	{
		fprintf(stderr," %02x",data[i]);
	}
	fprintf(stderr,"\n");
}
int initFrameHead(INT8U *buf,OAD oad,RSD select,INT8U selectype,CSD_ARRAYTYPE csds,INT8U *seqnumindex)
{
	int indexn=0 ,i=0 ;

	buf[indexn++] = 1;							//SEQUENCE OF A-ResultRecord
	indexn +=create_OAD(&buf[indexn] ,oad);		//主OAD
	buf[indexn++] = csds.num;					//RCSD::SEQUENCE OF CSD
//	fprintf(stderr,"csds.num=%d\n",csds.num);
	for(i=0; i<csds.num; i++)
	{
		indexn += fill_CSD(0,&buf[indexn],csds.csd[i]);		//填充 CSD
	}
	buf[indexn++] = 1;	//结果类型 数据
	*seqnumindex = indexn+2;
	buf[indexn++] = 1;	//sequence of 长度

	fprintf(stderr,"seqnumindex = %d\n",*seqnumindex);
	return indexn;
}
void intToBuf(int value,INT8U *buf)
{
	buf[0] = value&0x00ff;
	buf[1] = (value>>8) & 0x0ff;
}
int collectData(INT8U *databuf,INT8U *srcbuf,OAD_INDEX *oad_offset,int oadnum)
{
	int i=0,j=0;
	INT8U tmpbuf[256];
	int pindex = 0,retlen=0;

//	fprintf(stderr,"oadmr_num=%d  unitnum=%d \n",item_road.oadmr_num,unitnum);
//	for(i=0;i<item_road.oadmr_num;i++)
	{
		memset(tmpbuf,0x00,256);
		for(j=0;j<oadnum;j++)
		{
			fprintf(stderr,"j=%d, len = %d, offset=%d\n",j,oad_offset[j].len,oad_offset[j].offset);
			fprintf(stderr,"oad_m=%04x,oad_r=%04x\n",oad_offset[j].oad_m.OI,oad_offset[j].oad_r.OI);
			if(oad_offset[j].len == 0)//没找到
				databuf[pindex++] = 0;
			else
			{
				memcpy(tmpbuf,&srcbuf[oad_offset[j].offset],oad_offset[j].len);
//				fprintf(stderr,"tmpbuf[0]=%02x\n",tmpbuf[0]);
				switch(tmpbuf[0])
				{
				case 0:
					databuf[pindex++] = 0;
					fprintf(stderr,"000 pindex=%d\n",pindex);
					break;
				case 1://array
					retlen = CalcOIDataLen(oad_offset[j].oad_r.OI,1);
					retlen = retlen*tmpbuf[1]+2;//2代表一个array类型加一个个数
					memcpy(&databuf[pindex],tmpbuf,retlen);
					pindex += retlen;
					fprintf(stderr,"111 pindex=%d\n",pindex);
					break;
				case 0x55:	//TSA
					memcpy(&databuf[pindex],tmpbuf,(tmpbuf[1]+2));
					pindex += (tmpbuf[1]+2);
					fprintf(stderr,"TSA pindex=%d\n",pindex);
					break;
				default:
					memcpy(&databuf[pindex],tmpbuf,oad_offset[j].len);
					pindex += oad_offset[j].len;
					fprintf(stderr,"pindex=%d\n",pindex);
					break;
				}
				int k=0;
				for(k=0;k<pindex;k++) {
					fprintf(stderr,"%02x ",databuf[k]);
				}
				fprintf(stderr,"\n\n");
//				break;		//从unitnum 找到 退出
			}
		}
	}
	return pindex;
}



INT16S GetTaskHead(FILE *fp,INT16U *head_len,INT16U *tsa_len,HEAD_UNIT **head_unit)
{
	INT8U 	headl[2],blockl[2];
	INT16U 	unitnum=0,i=0;

	fread(headl,2,1,fp);
	*head_len = headl[0];
	*head_len = (headl[0]<<8) + headl[1];
	fprintf(stderr,"\n----headlen=%d\n",*head_len);
	if(*head_len<=4)
		return -1;
	fread(&blockl,2,1,fp);
	*tsa_len = blockl[0];
	*tsa_len = (blockl[0]<<8) + blockl[1];
	fprintf(stderr,"\n----blocklen=%d\n",*tsa_len);
	unitnum = (*head_len-4)/sizeof(HEAD_UNIT);
	fprintf(stderr,"\n----blocklen=%d unitnum=%d\n",*tsa_len,unitnum);
	if(*head_unit==NULL)
		*head_unit = malloc(*head_len);
	fprintf(stderr,"get  %p",*head_unit);
	fread(*head_unit,*head_len-4,1,fp);
	fprintf(stderr,"\nhead_unit:len(%d):\n",unitnum);
	return unitnum;
}
/*
 *
 */
int GetTaskData(OAD oad,RSD select, INT8U selectype,CSD_ARRAYTYPE csds,INT8U recordn)
{
	INT8U 	taskid=0,recordbuf[1000],onefrmbuf[2000];
	ROAD_ITEM item_road;
	HEAD_UNIT *headunit = NULL;//文件头
	OAD_INDEX oad_offset[100];//oad索引
	TASKSET_INFO tasknor_info;
	INT16U  blocksize=0,headsize=0;
	int offsetTsa = 0,recordoffset = 0,unitnum=0,i=0,j=0,indexn=0,recordlen = 0,recordno = 0,currecord = 0,tsa_num=0,framesum=0;
	INT8U recordnum=0,seqnumindex=0;
	TSA *tsa_group = NULL;

	if((taskid = GetTaskidFromCSDs(csds,&item_road)) == 0)//暂时不支持招测的不在一个采集方案
		return 0;
	if(ReadTaskInfo(taskid,&tasknor_info)!=1)//得到任务信息
	{
		fprintf(stderr,"\n得到任务信息失败\n");
		return 0;
	}
	fprintf(stderr,"\n得到任务信息成功\n");
	//1\打开数据文件
	FILE *myfp = openFramefile();
	fprintf(stderr,"\n----------1\n");
	FILE *fp = opendatafile(taskid);
	if (fp==NULL || myfp==NULL)
		return 0;
	fprintf(stderr,"\n打开文件成功\n");
//	ReadFileHeadLen(fp,&headsize,&blocksize);
//	memset(headunit,0x00,sizeof(headunit));
//	fread(headunit,headsize-4,1,fp);

	unitnum = GetTaskHead(fp,&headsize,&blocksize,&headunit);
	fprintf(stderr,"\n----------2\n");
	fprintf(stderr,"\n----------3\n");
	for(i=0;i<unitnum;i++)
		fprintf(stderr,"%04x%02x%02x:%04x%02x%02x:%04x\n",
				headunit[i].oad_m.OI,headunit[i].oad_m.attflg,headunit[i].oad_m.attrindex,
				headunit[i].oad_r.OI,headunit[i].oad_r.attflg,headunit[i].oad_r.attrindex,headunit[i].len);

	memset(oad_offset,0x00,sizeof(oad_offset));
	GetOADPosofUnit(item_road,headunit,unitnum,oad_offset);//得到每一个oad在块数据中的偏移
	fprintf(stderr,"\n----------4\n");
	recordlen = blocksize/tasknor_info.runtime;//计算每条记录的字节数
	fprintf(stderr,"\nrecordlen = %d,freq=%d\n",recordlen,tasknor_info.freq);
	recordno = getrecordno(tasknor_info.starthour,tasknor_info.startmin,tasknor_info.freq);


	fprintf(stderr,"\n-----------------------------------1-----------------------------------------------------------\n");
	//2\获得全部TSA列表
	fprintf(stderr,"\nmstype=%d recordno=%d\n",select.selec10.meters.mstype,recordno);

	tsa_num = getTsas(select.selec10.meters,(INT8U **)&tsa_group);
	fprintf(stderr,"get tsa_num=%d,tsa_group=%p\n",tsa_num,tsa_group);
	for(i=0;i<tsa_num;i++) {
		fprintf(stderr,"\nTSA%d: %d-",i,tsa_group[i].addr[0]);
		for(j=0;j<tsa_group[i].addr[0];j++) {
			fprintf(stderr,"-%02x",tsa_group[i].addr[j+1]);
		}
	}
	fprintf(stderr,"\n----------------------------------2------------------------------------------------------------\n");
	memset(onefrmbuf,0,sizeof(onefrmbuf));
	//初始化分帧头
	indexn = 2;
	indexn += initFrameHead(&onefrmbuf[indexn],oad,select,selectype,csds,&seqnumindex);

	//3\定位TSA , 返回offset
	for(i =0; i< tsa_num; i++)
	{
		offsetTsa = findTsa(tsa_group[i],fp,headsize,blocksize);
//		fprintf(stderr,"\n-----offsetTsa = %d\n",offsetTsa);
		if(offsetTsa == 0)
			continue;
		//4\计算当前点
		currecord = getrecordno(tasknor_info.starthour,tasknor_info.startmin,tasknor_info.freq);//freq为执行间隔,单位分钟
//		for(j=0; j<recordn ; j++)
		for(j=1; j<=recordn;j++)		//test
		{
			//5\定位指定的点（行）, 返回offset
			recordoffset = findrecord(offsetTsa,recordlen,currecord-j);//selector10 例如当前在10，上1为10-0=10 上2为10-1=9
			memset(recordbuf,0x00,sizeof(recordbuf));
			//6\读出一行数据到临时缓存
			fseek(fp,recordoffset,SEEK_SET);
			fread(recordbuf,recordlen,1,fp);
			printRecordBytes(recordbuf,recordlen);
			//7\根据csds挑选数据，组织存储缓存
			indexn += collectData(&onefrmbuf[indexn],recordbuf,oad_offset,item_road.oadmr_num);
			recordnum++;
			fprintf(stderr,"recordnum=%d  seqnumindex=%d\n",recordnum,seqnumindex);
			if (indexn>=1000)
			{
				framesum++;
				//8 存储1帧
				intToBuf((indexn-2),onefrmbuf);		//帧长度保存帧的数据长度
				saveOneFrame(onefrmbuf,indexn,myfp);
				indexn = 2;
				indexn += initFrameHead(&onefrmbuf[indexn],oad,select,selectype,csds,&seqnumindex);
				onefrmbuf[seqnumindex] = recordnum;
				recordnum = 0;
				break;
			}
		}
	}
	fprintf(stderr,"组帧：indexn=%d\n",indexn);
	for(i=0;i<indexn;i++) {
		fprintf(stderr,"%02x ",onefrmbuf[i]);
	}

	if(framesum==0) {
		fprintf(stderr,"saveOneFrame  seqnumindex=%d,  recordnum=%d!!!!!!!!!!!!!!!!\n",seqnumindex,recordnum);
		intToBuf((indexn-2),onefrmbuf);
		onefrmbuf[seqnumindex] = recordnum;
		saveOneFrame(onefrmbuf,indexn,myfp);
	}
//	fprintf(stderr,"\n---------------------------------3-------------------------------------------------------------\n");
	if(tsa_group != NULL)
		free(tsa_group);
//	fprintf(stderr,"\n---------------------------------3.5-------------------------------------------------------------\n");
	if(headunit!=NULL){
		free(headunit);
	}
//	fprintf(stderr,"\n---------------------------------4-------------------------------------------------------------\n");
	if(fp != NULL)
		fclose(fp);
	if(myfp != NULL)
		fclose(myfp);
//	fprintf(stderr,"\n---------------------------------5-------------------------------------------------------------\n");
	return 0;
}
//INT8U getaskData(OAD oad_h,INT8U seletype,ZC_TIMEINTERVL timeinte,TSA *tsa_con,INT16U tsa_num,CSD_ARRAYTYPE csds)//,ROAD_ITEM item_road)
//{
//	TASKSET_INFO tasknor_info;
//	ROAD_ITEM item_road;
//	TS ts_file;
//	INT16U headlen=0,blocklen=0,unitnum=0,retlen=0,schpos=0,tsa_cnt=0,tsa_pos=0;//tsa_cnt为一帧报文包含的tsa个数
//	INT16U frmindex=0;//前两个字节为长度
//	INT16U start_seq=0,end_seq=0,circle_max=300;//开始序号和结束序号，有招测的冻结时标决定，如果为selector5，则两个序列号一样
//	INT8U headl[2],blockl[2],tmpbuf[256],onefrmbuf[2000],onefrmhead[200];//onefrmbuf前两个字节为长度
//	INT8U *databuf_tmp=NULL;
//	INT8U taskid=0;
//	HEAD_UNIT *head_unit = NULL;
//	int i=0,j=0;
//	FILE *fp  = NULL;
//	char	fname[FILENAMELEN]={};
//	INT8U tmpbufff[150];
////	fprintf(stderr,"\n--2--tsa_group=%p\n",tsa_con);
//	if((taskid = GetTaskidFromCSDs(csds,&item_road)) == 0)//暂时不支持招测的不在一个采集方案
//		return 0;
//	fprintf(stderr,"\ntaskid=%d GetTaskidFromCSDs(%d):\n",taskid,item_road.oadmr_num);
//	for(i=0;i<item_road.oadmr_num;i++)
//		fprintf(stderr,"%04x%02x%02x-----%04x%02x%02x\n",
//				item_road.oad[i].oad_m.OI,item_road.oad[i].oad_m.attflg,item_road.oad[i].oad_m.attrindex,
//				item_road.oad[i].oad_r.OI,item_road.oad[i].oad_r.attflg,item_road.oad[i].oad_r.attrindex);
//	if(ReadTaskInfo(taskid,&tasknor_info)!=1)//得到任务信息
//	{
//		fprintf(stderr,"\n得到任务信息失败\n");
//		return 0;
//	}
////	fprintf(stderr,"\n--3--tsa_group=%p\n",tsa_con);
//	fprintf(stderr,"\n得到任务信息成功\n");
//	//////////////////////////////////计算每帧的头
//	onefrmhead[frmindex++]=0;
//	onefrmhead[frmindex++]=0;//前两个字节是长度
//	memcpy(&onefrmhead[frmindex],&oad_h,sizeof(OAD));
////	fprintf(stderr,"\n--4--tsa_group=%p\n",tsa_con);
//	frmindex += sizeof(OAD);
//	onefrmhead[frmindex++]=seletype;
//	for(i=0;i<csds.num;i++)
//	{
//		frmindex += fill_CSD(0,&onefrmhead[frmindex],csds.csd[i]);
//	}
////	fprintf(stderr,"\n--5--tsa_group=%p----%d,frmindex=%d\n",tsa_con,sizeof(CSD_ARRAYTYPE),frmindex);
//	tsa_pos = frmindex;
//	onefrmhead[frmindex++]=0;//tsa_cnt
//	///////////////////////////////////
//	ts_file.Year = timeinte.year.data;
//	ts_file.Month = timeinte.month.data;
//	ts_file.Day = timeinte.day.data;
//	ts_file.Hour = timeinte.start_hour.data;
//	ts_file.Minute = timeinte.start_min.data;
//	ts_file.Sec = 0;
//	getTaskFileName(taskid,ts_file,fname);//得到要抄读的文件名称
////	fprintf(stderr,"\n--6--tsa_group=%p\n",tsa_con);
//	fp = fopen(fname,"r");
//	if(fp == NULL)//文件没内容，退出，如果文件已存在，提取文件头信息
//	{
//		fprintf(stderr,"\n-----file %s not exist\n",fname);
//		return 0;
//	}
//	else
//	{
//		fprintf(stderr,"\n-----file %s do exist\n",fname);
//		fread(headl,2,1,fp);
////		fprintf(stderr,"\n--7--tsa_group=%p\n",tsa_con);
//		headlen = headl[0];
//		headlen = (headl[0]<<8) + headl[1];
//		fprintf(stderr,"\n----headlen=%d\n",headlen);
//		fread(&blockl,2,1,fp);
////		fprintf(stderr,"\n--8--tsa_group=%p\n",tsa_con);
////		fprintf(stderr,"\nblocklen=%02x %02x\n",blockl[1],blockl[0]);
//		blocklen = blockl[0];
//		blocklen = (blockl[0]<<8) + blockl[1];
//		fprintf(stderr,"\n----blocklen=%d\n",blocklen);
////		fprintf(stderr,"\n--9--tsa_group=%p\n",tsa_con);
//		unitnum = (headlen-4)/sizeof(HEAD_UNIT);
//		databuf_tmp = (INT8U *)malloc(blocklen);
////		fprintf(stderr,"\n--10--tsa_group=%p\n",tsa_con);
//		if(databuf_tmp == NULL)
//		{
//			fprintf(stderr,"\n分配内存给databuf_tmp失败！\n");
//			return 0;
//		}
////		head_unit = (HEAD_UNIT *)malloc(headlen-4);
////		fprintf(stderr,"\n--11--tsa_group=%p\n",tsa_con);
////		if(head_unit == NULL)
////		{
////			fprintf(stderr,"\n分配内存给head_unit失败！\n");
////			return 0;
////		}
//		fprintf(stderr,"\n----headlen=%d\n",headlen);
////		fread(head_unit,headlen-4,1,fp);
//		fread(tmpbufff,headlen-4,1,fp);
//		head_unit = (HEAD_UNIT *)tmpbufff;
//		fprintf(stderr,"\nhead_unit:num(%d):\n",unitnum);
//		for(i=0;i<(headlen-4)/sizeof(HEAD_UNIT);i++)
//			fprintf(stderr,"%d:%04x%02x%02x--%04x%02x%02x:\n",head_unit[i].len,
//					head_unit[i].oad_m.OI,head_unit[i].oad_m.attflg,head_unit[i].oad_m.attrindex,
//					head_unit[i].oad_r.OI,head_unit[i].oad_r.attflg,head_unit[i].oad_r.attrindex);
//		for(i=0;i<headlen-4;i++)
//			fprintf(stderr,"%02x ",tmpbufff[i]);
//		fprintf(stderr,"\n---------------------------\n");
//		fseek(fp,headlen,SEEK_SET);
////		fprintf(stderr,"\n--12--tsa_group=%p\n",tsa_con);
////		fseek(fp,headlen,SEEK_SET);//跳过文件头
//
//		while(!feof(fp))//找存储结构位置
//		{
//			fprintf(stderr,"\n-------------8---blocklen=%d,headlen=%d,tsa_num=%d\n",blocklen,headlen,tsa_num);
//			//00 00 00 00 00 00 00 00 07 05 00 00 00 00 00 01
//			//00 00 00 00 00 00 00 00 00 07 05 00 00 00 00 00 01
//			fprintf(stderr,"\n--13--tsa_group=%p\n",tsa_con);
//			memset(databuf_tmp,0xee,blocklen);
//			if(fread(databuf_tmp,blocklen,1,fp) == 0)
//				break;
//			fprintf(stderr,"\n-------------9-\n");
//			fprintf(stderr,"\n--14--tsa_group=%p\n",tsa_con);
//			memset(onefrmbuf,0x00,2000);
//			fprintf(stderr,"\n--15--tsa_group=%p\n",tsa_con);
//			for(i=0;i<tsa_num;i++)
//			{
//				fprintf(stderr,"\n-------------i=%d,schpos=%d,%p,%p\n",i,schpos,databuf_tmp,tsa_con);
//				fprintf(stderr,"\nTSA%d: %d-",i,tsa_con[i].addr[0]);
//				for(j=0;j<tsa_con[i].addr[0];j++) {
//					fprintf(stderr,"-%02x",tsa_con[i].addr[j]);
//				}
//				fprintf(stderr,"\n\n");
//				for(j=0;j<tsa_con[i].addr[0];j++) {
//					fprintf(stderr,"-%02x",databuf_tmp[schpos+1+j]);
//				}
//				if(memcmp(&databuf_tmp[schpos+1],&tsa_con[i].addr[0],tsa_con[i].addr[0])!=0)
//				{
//					fprintf(stderr,"\n-------------10-\n");
//					continue;
//				}
//				fprintf(stderr,"\n-------------11-\n");
//				switch(seletype)
//				{
//				case 5:
//				case 7:
//					break;
//				case 10:
//					if( ts_file.Hour*60+ts_file.Minute < tasknor_info.startmin)
//					{
//						fprintf(stderr,"\ngetaskData--------ts_file=%d,task=%d\n",ts_file.Hour*60+ts_file.Minute,tasknor_info.startmin);
//						return 0;//招测不在抄表时间段内，正常情况下不会出现
//					}
//					start_seq = (ts_file.Hour*60+ts_file.Minute-tasknor_info.startmin)/tasknor_info.freq;
//					end_seq = start_seq-timeinte.last_time+1;
//					fprintf(stderr,"\nstart_seq=%d,end_seq=%d\n",start_seq,end_seq);
//					break;
//				default:break;
//				}
//				while(circle_max--)//最多循环300次，防止无限循环，288时为5分钟一个测量点
//				{
//					schpos = start_seq*blocklen/tasknor_info.runtime;
//					if(seletype == 10)
//						start_seq--;
//					else
//						start_seq++;
//					fprintf(stderr,"\n\n");
//					for(j=0;j<item_road.oadmr_num;j++)
//					{
//						memset(tmpbuf,0x00,256);
//						if((retlen = GetPosofOAD(&databuf_tmp[schpos],item_road.oad[j].oad_m,item_road.oad[j].oad_r,head_unit,unitnum,tmpbuf))==0)//得到每一个oad在块数据中的偏移
//							onefrmbuf[frmindex++] = 0;
//						else
//						{
//							switch(tmpbuf[0])
//							{
//							case 0:
//								onefrmbuf[frmindex++] = 0;
//								break;
//							case 1://array
//								retlen = CalcOIDataLen(item_road.oad[j].oad_m.OI,1);
//								retlen = retlen*tmpbuf[1]+2;//2代表一个array类型加一个个数
//								memcpy(&onefrmbuf[frmindex],tmpbuf,retlen);
//								frmindex += retlen;
//								break;
//							default:
//								memcpy(&onefrmbuf[frmindex],tmpbuf,retlen);
//								frmindex += retlen;
//								break;
//							}
//
//						}
//					}
//					tsa_cnt++;
//					if(frmindex >= 1000)
//					{
//						onefrmbuf[0]=frmindex & 0xff;
//						onefrmbuf[1]=(frmindex>>8) & 0xff;//计算长度
//						onefrmbuf[tsa_pos] = tsa_cnt;
//						saveonefrm(onefrmbuf,frmindex);//存1帧
//						//////////////////////////////////计算每帧的头
//						frmindex=0;
//						tsa_cnt=0;
//						memset(onefrmbuf,0x00,2000);
//						onefrmhead[frmindex++]=0;
//						onefrmhead[frmindex++]=0;//前两个字节是长度
//						memcpy(&onefrmhead[frmindex],&oad_h,sizeof(OAD));
//						frmindex += sizeof(OAD);
//						onefrmhead[frmindex++]=seletype;
//						memcpy(&onefrmhead[frmindex],&csds,sizeof(CSD_ARRAYTYPE));
//						tsa_pos = frmindex;
//						onefrmhead[frmindex++]=0;//tsa_cnt
//						///////////////////////////////////
//					}
//
//					if(start_seq <= end_seq && seletype == 10)
//						break;
//				}
//				if(circle_max == 0)
//					fprintf(stderr,"\n循环溢出!\n");
//			}
//		}
//		fprintf(stderr,"\n读取文件结束！！\n");
//	}
//	if(databuf_tmp != NULL)
//		free(databuf_tmp);
//	if(head_unit != NULL)
//		free(head_unit);
//	fprintf(stderr,"\ngetaskdata out\n");
//	return 0;
//}
INT8U getSelector(OAD oad_h,RSD select, INT8U selectype, CSD_ARRAYTYPE csds, INT8U *data, int *datalen)
{
	switch(selectype)
	{
	case 5:
		break;
	case 7:
		break;
	case 10:
		GetTaskData(oad_h,select,selectype,csds,select.selec10.recordn);
		break;
	default:break;
	}
	return 1;
}
///*
// * 根据招测类型组织报文
// * 如果MS选取的测量点过多，不能同时上报，分帧
// */
//INT8U getSelector(RSD select, INT8U selectype, CSD_ARRAYTYPE csds, INT8U *data, int *datalen)
//{
//	TS ts_info[2];//时标选择，根据普通采集方案存储时标选择进行，此处默认为相对当日0点0分 todo
//	INT8U taskid;//,tsa_num=0;
//	TSA *tsa_con = NULL;
//	INT16U tsa_num=0,TSA_num=0;
//	ROAD_ITEM item_road;
//	memset(&item_road,0x00,sizeof(ROAD_ITEM));
//	int i=0;
//	tsa_num = getFileRecordNum(0x6000);
//	tsa_con = malloc(tsa_num*sizeof(TSA));
//	//测试写死
////	taskid = 1;
//	///////////////////////////////////////////////////////////////test
//	fprintf(stderr,"\n-----selectype=%d\n",selectype);
//	switch(selectype)
//	{
//	case 5://例子中招测冻结数据，包括分钟小时日月冻结数据招测方法
//		memcpy(&ts_info[0],&select.selec5.collect_save,sizeof(DateTimeBCD));
//		fprintf(stderr,"\n--招测冻结 ts=%04d-%02d-%02d %02d:%02d\n",ts_info[0].Year,ts_info[0].Month,ts_info[0].Day,ts_info[0].Hour,ts_info[0].Minute);
////		ReadNorData(ts_info,taskid,tsa_con,tsa_num);
////		//////////////////////////////////////////////////////////////////////test
//		TSGet(&ts_info[0]);
////		//////////////////////////////////////////////////////////////////////test
//		TSA_num = GetTSACon(select.selec5.meters,tsa_con,tsa_num);
//		for(i=0;i<TSA_num;i++)
//			fprintf(stderr,"\n1addr3:%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
//					tsa_con[i].addr[16],tsa_con[i].addr[15],tsa_con[i].addr[14],tsa_con[i].addr[13],
//					tsa_con[i].addr[12],tsa_con[i].addr[11],tsa_con[i].addr[10],tsa_con[i].addr[9],
//					tsa_con[i].addr[8],tsa_con[i].addr[7],tsa_con[i].addr[6],	tsa_con[i].addr[5],
//					tsa_con[i].addr[4],tsa_con[i].addr[3],tsa_con[i].addr[2],tsa_con[i].addr[1],tsa_con[i].addr[0]);
//		if((taskid = GetTaskidFromCSDs(csds,&item_road)) != 0)
//			*datalen = ComposeSendBuff(&ts_info[0],selectype,taskid,tsa_con,tsa_num,csds,data);
//		else
//			fprintf(stderr,"\nselector 5招测涉及到多个任务\n");//招测涉及到多个任务
//		break;
//	case 7://例子中招测实时数据方法
//		TSA_num = GetTSACon(select.selec7.meters,tsa_con,tsa_num);
//		for(i=0;i<TSA_num;i++)
//			fprintf(stderr,"\n1addr3:%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
//					tsa_con[i].addr[16],tsa_con[i].addr[15],tsa_con[i].addr[14],tsa_con[i].addr[13],
//					tsa_con[i].addr[12],tsa_con[i].addr[11],tsa_con[i].addr[10],tsa_con[i].addr[9],
//					tsa_con[i].addr[8],tsa_con[i].addr[7],tsa_con[i].addr[6],	tsa_con[i].addr[5],
//					tsa_con[i].addr[4],tsa_con[i].addr[3],tsa_con[i].addr[2],tsa_con[i].addr[1],tsa_con[i].addr[0]);
//		memcpy(&ts_info[0],&select.selec7.collect_save_star,sizeof(DateTimeBCD));
//		memcpy(&ts_info[1],&select.selec7.collect_save_finish,sizeof(DateTimeBCD));
//		fprintf(stderr,"\n--招测实时开始时间 ts=%04d-%02d-%02d %02d:%02d\n",ts_info[0].Year,ts_info[0].Month,ts_info[0].Day,ts_info[0].Hour,ts_info[0].Minute);
//		fprintf(stderr,"\n--招测实时完成时间 ts=%04d-%02d-%02d %02d:%02d\n",ts_info[1].Year,ts_info[1].Month,ts_info[1].Day,ts_info[1].Hour,ts_info[1].Minute);
//		if((taskid = GetTaskidFromCSDs(csds,&item_road)) != 0)
//			*datalen = ComposeSendBuff(&ts_info[0],selectype,taskid,tsa_con,tsa_num,csds,data);
//		else
//			fprintf(stderr,"\nselector 7招测涉及到多个任务\n");//招测涉及到多个任务
//		break;
//	default:
//		break;
//	}
////	fprintf(stderr,"\n报文(%d)：",*datalen);
////	for(i=0;i<*datalen;i++)
////		fprintf(stderr," %02x",data[i]);
//	free(tsa_con);
//	/////////////////////////////////////test
//	if(*datalen > 1000)
//		*datalen = 0;//分帧了
//	////////////////////////////////////test
//	return 0;
//}
