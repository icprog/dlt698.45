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
		num=fread(dest,class_info[infoi].interface_len,1,fp);
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
	ret = save_block_file((char *)class_info[infoi].file_name,blockdata,class_info[infoi].unit_len,class_info[infoi].interface_len,seqnum);
	if(class_info[infoi].interface_len!=0) {		//该存储单元内部包含的类的公共属性
		WriteInterfaceClass(oi,seqnum,AddUpdate);
	}
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
	int		ret = refuse_rw;
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
INT8U datafile_write(char *FileName, void *source, int size, int offset)
{
	FILE *fp=NULL;
	int	  fd=0;
	INT8U res=0;
	int num=0;
	INT8U	*blockdata=NULL;

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
	free(blockdata);
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
void getEveFileName(OI_698 eve_oi,char *fname)
{
	char dirname[FILENAMELEN]={};
	if (fname==NULL)
		return ;
	memset(fname,0,FILENAMELEN);
	sprintf(dirname,"%s",EVEDATA);
	makeSubDir(dirname);
	sprintf(fname,"%s/%04x.dat",EVEDATA,eve_oi);
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
//				fprintf(stderr,"\nfind oad %04x%02x%02x-%04x%02x%02x:offset:%d\n",
//						item_road.oad[i].oad_m.OI,item_road.oad[i].oad_m.attflg,item_road.oad[i].oad_m.attrindex,
//						item_road.oad[i].oad_r.OI,item_road.oad[i].oad_r.attflg,item_road.oad[i].oad_r.attrindex,
//						datapos);
				oad_offset[i].offset = datapos;
				oad_offset[i].len = head_unit[j].len;
			}
			else {
				datapos += head_unit[j].len;
//				fprintf(stderr,"datapos = %d\n",datapos);
			}
		}
	}
}
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
		asyslog(LOG_INFO,"GetTaskData: class6013.cjtype =%d  class6013.state =%d\n",class6013.cjtype,class6013.state);
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
			asyslog(LOG_INFO,"任务开始结束时间：%d:%d--%d:%d\n",tasknor_info->starthour,tasknor_info->startmin,tasknor_info->endhour,tasknor_info->endmin);
			asyslog(LOG_INFO,"\n---@@@---任务%d执行次数%d\n",taskid,tasknor_info->runtime);

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
	char lnf[5];
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

		memset(lnf,0x00,5);
		memcpy(lnf,&ln[0],4);

		if(strtoul(lnf,NULL,16) != oi)
			continue;

		memset(lnf,0x00,5);
		memcpy(lnf,&ln[8],3);
		oi_len = strtoul(lnf,NULL,16)+1;

		memset(lnf,0x00,5);
		memcpy(lnf,&ln[12],2);
		ic_type = strtol(lnf,NULL,10);
		break;
	}
	fclose(fp);
	fprintf(stderr,"\noi_len=%d ic_type=%d\n",oi_len,ic_type);
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

	print_rcsd(csds);
	if(csds.num > MY_CSD_NUM)//超了
		csds.num = MY_CSD_NUM;
	asyslog(LOG_INFO,"csds.num=%d\n",csds.num);
	for(i=0;i<csds.num;i++)
	{
//		asyslog(LOG_INFO,"csds.csd[%d].type=%d\n",i,csds.csd[i].type);
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
			item_road->oad[item_road->oadmr_num].oad_num = 0;//oad类型写为0
			item_road->oadmr_num++;
//			asyslog(LOG_INFO,"0000:item_road->oadmr_num=%d\n",item_road->oadmr_num);

			break;
		case 1:
			if(csds.csd[i].csd.road.num > ROAD_OADS_NUM)
				csds.csd[i].csd.road.num = ROAD_OADS_NUM;
			item_road->oad[item_road->oadmr_num].oad_num = csds.csd[i].csd.road.num;//road类型的从oad个数写为实际从oad个数
			for(j=0;j<csds.csd[i].csd.road.num;j++)
			{
				memcpy(&item_road->oad[item_road->oadmr_num].oad_m,&csds.csd[i].csd.road.oad,sizeof(OAD));
				memcpy(&item_road->oad[item_road->oadmr_num].oad_r,&csds.csd[i].csd.road.oads[j],sizeof(OAD));
				item_road->oadmr_num++;
			}
//			asyslog(LOG_INFO,"11111:item_road->oadmr_num=%d\n",item_road->oadmr_num);

			break;
		default:break;
		}
	}
//	asyslog(LOG_INFO,"任务下发的主-从OI配置：oadmr_num=%d\n",item_road->oadmr_num);
//	for(i=0;i<item_road->oadmr_num;i++){
//		asyslog(LOG_INFO,"[%d] %04x_%04x\n",i,item_road->oad[i].oad_m.OI,item_road->oad[i].oad_r.OI);
//	}
	memset(&class6013,0,sizeof(CLASS_6013));
	memset(&class6015,0,sizeof(CLASS_6015));
	for(i=0;i<256;i++)//先比较有没有跟现成采集方案匹配的，有直接返回taskid，没有返回0
	{
		if(readCoverClass(0x6013,i+1,&class6013,sizeof(class6013),coll_para_save) == 1)
		{
			if(class6013.cjtype != 1 || class6013.state != 1)//过滤掉不是普通采集方案的
				continue;
			if(readCoverClass(0x6015,class6013.sernum,&class6015,sizeof(CLASS_6015),coll_para_save) == 1)
			{
				asyslog(LOG_INFO,"查找任务号 %d，方案序号：%d class6015.csds.num=%d",i+1,class6013.sernum,class6015.csds.num);
				for(j=0;j<class6015.csds.num;j++)
				{
					for(mm=0;mm<item_road->oadmr_num;mm++)
					{
						switch(class6015.csds.csd[j].type)
						{
						case 0:
//							  asyslog(LOG_INFO,"mm=%d,oad_r  =%04x_%02x%02x \n",mm,item_road->oad[mm].oad_r.OI,
//											item_road->oad[mm].oad_r.attflg,item_road->oad[mm].oad_r.attrindex);
//							  asyslog(LOG_INFO,"jj=%d,csd.oad=%04x_%02x%02x \n",j,class6015.csds.csd[j].csd.oad.OI,
//											class6015.csds.csd[j].csd.oad.attflg,class6015.csds.csd[j].csd.oad.attrindex);
							if(item_road->oad[mm].oad_m.OI == 0x0000)//都为oad类型
							{
								if(memcmp(&item_road->oad[mm].oad_r,&class6015.csds.csd[j].csd.oad,sizeof(OAD))==0){
									item_road->oad[mm].taskid = i+1;
//									asyslog(LOG_INFO,"0000:item_road->oad[%d].taskid=%d\n",mm,item_road->oad[mm].taskid);
								}
							}
							break;
						case 1:
//							  asyslog(LOG_INFO,"11111 mm=%d,oad_r  =%04x_%02x%02x \n",mm,item_road->oad[mm].oad_r.OI,
//											item_road->oad[mm].oad_r.attflg,item_road->oad[mm].oad_r.attrindex);
//							  asyslog(LOG_INFO,"11111 jj=%d,csd.oad=%04x_%02x%02x \n",j,class6015.csds.csd[j].csd.oad.OI,
//											class6015.csds.csd[j].csd.oad.attflg,class6015.csds.csd[j].csd.oad.attrindex);
							if(memcmp(&item_road->oad[mm].oad_m,&class6015.csds.csd[j].csd.road.oad,sizeof(OAD))==0)//
							{
								for(nn=0;nn<class6015.csds.csd[j].csd.road.num;nn++)
								{
									if(memcmp(&item_road->oad[mm].oad_r,&class6015.csds.csd[j].csd.road.oads[nn],sizeof(OAD))==0){
										item_road->oad[mm].taskid = i+1;
//										asyslog(LOG_INFO,"1111:item_road->oad[%d].taskid=%d\n",mm,item_road->oad[mm].taskid);
									}
								}
							}
							break;
						default:break;
						}
					}
				}
//				asyslog(LOG_INFO,"item_road->oadmr_num=%d\n",item_road->oadmr_num);
//				for(mm=0;mm<item_road->oadmr_num;mm++) {
//					asyslog(LOG_INFO,"taskid[%d]=%d\n",mm,item_road->oad[mm].taskid);
//				}
				for(mm=0;mm<(item_road->oadmr_num);mm++)
				{
//					asyslog(LOG_INFO,"taskno=%d ,item_road->oad[%d].taskid=%d\n",taskno,mm,item_road->oad[mm].taskid);
					if(taskno != 0 && taskno != item_road->oad[mm].taskid)
					{
						taskno = 0;
						asyslog(LOG_INFO,"break taskno=%d\n",taskno);
						break;
					}
					if(item_road->oad[mm].taskid != 0)
						taskno = item_road->oad[mm].taskid;
//					asyslog(LOG_INFO,"i=%d ,taskno=%d\n",mm,taskno);
				}
				if(taskno != 0)
				{
					asyslog(LOG_INFO,"return  ,taskno=%d\n",taskno);
					return taskno;
				}
			}
		}
	}
//	asyslog(LOG_INFO,"item_road->oadmr_num=%d\n",item_road->oadmr_num);
//	for(i=0;i<item_road->oadmr_num;i++) {
//		asyslog(LOG_INFO,"taskid[%d]=%d\n",i,item_road->oad[i].taskid);
//	}
//	for(i=0;i<(item_road->oadmr_num);i++)
//	{
//		asyslog(LOG_INFO,"taskno=%d ,item_road->oad[%d].taskid=%d\n",taskno,i,item_road->oad[i].taskid);
//		if(taskno != 0 && taskno != item_road->oad[i].taskid)
//		{
//			taskno = 0;
//			asyslog(LOG_INFO,"break taskno=%d\n",taskno);
//			break;
//		}
//		taskno = item_road->oad[i].taskid;
//		asyslog(LOG_INFO,"i=%d ,taskno=%d\n",i,taskno);
//	}
//	asyslog(LOG_INFO,"return  ,taskno=%d\n",taskno);
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
FILE* opendatafile(INT8U taskid,CURR_RECINFO recinfo)
{
	FILE *fp = NULL;
	char	fname[FILENAMELEN]={};
	TS ts_rec;
	struct tm *tm_p;
	fprintf(stderr,"\nrecinfo.rec_start = %ld--curr-%ld\n",recinfo.rec_start,time(NULL));
	tm_p = gmtime(&recinfo.rec_start);
	ts_rec.Year = tm_p->tm_year+1900;
	ts_rec.Month = tm_p->tm_mon+1;
	ts_rec.Day = tm_p->tm_mday;
	ts_rec.Hour = 0;
	ts_rec.Minute = 0;
	ts_rec.Sec = 0;

	getTaskFileName(taskid,ts_rec,fname);//得到要抄读的文件名称
	fprintf(stderr,"fname=%s\n",fname);
	asyslog(LOG_INFO,"任务时间到: 组帧frmdata，打开任务文件=%s, taskid=%d\n",fname,taskid);
	fp =fopen(fname,"r");
	return fp;
}
FILE* openevefile(OI_698 eve_oi)
{
	FILE *fp = NULL;
	char	fname[FILENAMELEN]={};
	getEveFileName(eve_oi,fname);//得到要抄读的文件名称
	fprintf(stderr,"fname=%s\n",fname);
	asyslog(LOG_INFO,"任务时间到: 组帧frmdata，打开任务文件=%s,\n",fname);
	fp =fopen(fname,"r");
	return fp;
}
FILE* openFramefile(char *filename)
{
	FILE *fp = NULL;

	if (access(filename,0)==0)
	{
		fp = fopen(filename,"w");
		fclose(fp);
	}
	fp = fopen(filename,"a+");
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

	fprintf(stderr,"\noffset=%d,需要查找 TSA: %d-",offset,tsa.addr[0]);
	for(i=0;i<(tsa.addr[0]+1);i++) {
		fprintf(stderr,"-%02x",tsa.addr[i]);
	}
	while(!feof(fp))//找存储结构位置
	{
		if(fread(tsa_tmp,TSA_LEN + 1,1,fp)==0)
		{
			return 0;
		}
//		fprintf(stderr,"\n任务保存 TSA: ");
//		for(i=0;i<(tsa.addr[0]+2);i++) {
//			fprintf(stderr,"-%02x",tsa_tmp[i]);
//		}
		if(memcmp(&tsa_tmp[1],&tsa.addr[0],tsa.addr[0])==0)
		{
			fprintf(stderr,"\n找到匹配 addr: %d-",tsa.addr[0]);
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
int getrecordno(INT8U starthour,INT8U startmin,int interval,CURR_RECINFO recinfo)
{
	int recordno = 0;
	struct tm *tm_p;
	tm_p = gmtime(&recinfo.rec_start);
	fprintf(stderr,"\ninterval = %d\n",interval);
	recordno = (tm_p->tm_hour*60 + tm_p->tm_min) - (starthour*60 + startmin);
	if(interval!=0) {
		recordno = recordno/(interval/60);
	}else recordno = 1;		//冻结抄读
	fprintf(stderr,"\n当前：%d:%d 任务开始：%d:%d 任务间隔:%d 记录序号%d\n",tm_p->tm_hour,tm_p->tm_min,starthour,startmin,interval,recordno);
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
	indexn +=create_OAD(0,&buf[indexn] ,oad);		//主OAD
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
int collectData(INT8U *databuf,INT8U *srcbuf,OAD_INDEX *oad_offset,ROAD_ITEM item_road)
{
	int i=0,j=0;
	INT8U tmpbuf[256];
	int pindex = 0,retlen=0;

//	fprintf(stderr,"oadmr_num=%d  unitnum=%d \n",item_road.oadmr_num,unitnum);
//	for(i=0;i<item_road.oadmr_num;i++)
	{
		memset(tmpbuf,0x00,256);
		for(j=0;j<item_road.oadmr_num;j++)
		{
			fprintf(stderr,"\n%04x-%04x--%d\n",item_road.oad[j].oad_m.OI,item_road.oad[j].oad_r.OI,item_road.oad[j].oad_num);
			if(item_road.oad[j].oad_num != 0)
			{
				databuf[pindex++] = 0x01;
				databuf[pindex++] = item_road.oad[j].oad_num;
			}
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
					fprintf(stderr,"\nretlen=%d--%02x\n",retlen,oad_offset[j].oad_r.OI);
					retlen = retlen*tmpbuf[1]+2;//2代表一个array类型加一个个数
					fprintf(stderr,"\nretlen=%d--%d\n",retlen,tmpbuf[1]);
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

int fillTsaNullData(INT8U *databuf,TSA tsa,ROAD_ITEM item_road)
{
	int pindex = 0;
	int i=0;

	fprintf(stderr,"item_road.oadmr_num = %d",item_road.oadmr_num);
	for(i=0;i<item_road.oadmr_num;i++) {
		fprintf(stderr,"\nitem_road.oad[i].oad_m.OI = %04x  item_road.oad[i].oad_num = %d\n",item_road.oad[i].oad_m.OI,
				item_road.oad[i].oad_num);
		if(item_road.oad[i].oad_m.OI == 0x0000 && item_road.oad[i].oad_r.OI == 0x202a) {
			databuf[pindex++] = dttsa;
			memcpy(&databuf[pindex],&tsa,(tsa.addr[0]+1));
			pindex += (tsa.addr[0]+1);
		}else {
			if((item_road.oad[i].oad_m.OI == 0x0000) || (item_road.oad[i].oad_num != 0)) {
				databuf[pindex++] = 0;
				fprintf(stderr,"\npindex = %d\n",pindex);
			}
		}
	}
	fprintf(stderr,"fillTsaNullData----index=%d\n",pindex);
	return pindex;
}

INT16S GetTaskHead(FILE *fp,INT16U *head_len,INT16U *tsa_len,HEAD_UNIT **head_unit)
{
	INT8U 	headl[2],blockl[2];
	INT16U 	unitnum=0;

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
 * 跨日时实时更新描述符fp
 */
INT8U updatedatafp(FILE *fp,INT8U recno,INT8U selectype,INT16U interval,CURR_RECINFO recinfo,INT8U taskid)//更新数据文件数据流指针
{
	time_t time_rec=0;
	struct tm *tm_p = NULL;
	INT8U oldday=0,nowday=0;
	char dirname[FILENAMELEN]={};
	char	fname[FILENAMELEN]={};
	fprintf(stderr,"\nrecno=%d\n",recno);
	if(recno < 1)//recno从1开始
		return 0;
	switch(selectype)
	{
	case 5://无需更新数据流
		break;
	case 7://
		time_rec = recinfo.rec_start+interval*(recno-1);//recno从1开始
		tm_p = gmtime(&time_rec);
		time_rec = recinfo.rec_start+interval*recno;
		tm_p = gmtime(&time_rec);
		break;
	case 10:
		time_rec = recinfo.rec_start-interval*(recno-1);//recno从1开始
		tm_p = gmtime(&time_rec);
		oldday=tm_p->tm_mday;
		fprintf(stderr,"\n1111olay=%d\n",oldday);
		time_rec = recinfo.rec_start-interval*recno;
		tm_p = gmtime(&time_rec);
		localtime_r(&time_rec,tm_p);
		nowday=tm_p->tm_mday;
		fprintf(stderr,"\n222nowday=%d\n",nowday);
		break;
	default://不合理
		break;
	}
	if(oldday != nowday)
	{
		if (fname==NULL)
			return 0;
		memset(fname,0,FILENAMELEN);
		sprintf(dirname,"%s",TASKDATA);
		makeSubDir(dirname);
		sprintf(dirname,"%s/%03d",TASKDATA,taskid);
		makeSubDir(dirname);
		sprintf(fname,"%s/%03d/%04d%02d%02d.dat",TASKDATA,taskid,tm_p->tm_year+1900,tm_p->tm_mon+1,tm_p->tm_mday);
		fprintf(stderr,"\n更新文件流：%s\n",fname);
		fp =fopen(fname,"r");
		if(fp != NULL)
			return 2;
		else
			return 0;
	}
	return 1;
}
/*
 * recinfo记录索引信息，用于动态更新读取文件流信息 将找测的selector信息转化为统一的格式
 */
INT8U initrecinfo(CURR_RECINFO *recinfo,TASKSET_INFO tasknor_info,INT8U selectype,RSD select)
{
	time_t time_s;
	struct tm *tm_p;
	switch(selectype)
	{
	case 5://冻结的招测时间要比文件时间提前一天,因此找文件时，要加上一天
		recinfo->recordno_num = tasknor_info.runtime;
		time(&time_s);
		time_s += 86400;//24*60*60; 加上一天的秒数
		tm_p = localtime(&time_s);
		tm_p->tm_year = select.selec5.collect_save.year.data;
		tm_p->tm_mon = select.selec5.collect_save.month.data;
		tm_p->tm_mday = select.selec5.collect_save.year.data;
		tm_p->tm_hour = tasknor_info.starthour;
		tm_p->tm_min = tasknor_info.startmin;
		tm_p->tm_sec = 0;
		recinfo->rec_start = mktime(tm_p);

		time(&time_s);
		time_s += 86400;//24*60*60; 加上一天的秒数
		tm_p = localtime(&time_s);
		tm_p->tm_year = select.selec5.collect_save.year.data;
		tm_p->tm_mon = select.selec5.collect_save.month.data;
		tm_p->tm_mday = select.selec5.collect_save.year.data;
		tm_p->tm_hour = tasknor_info.endhour;
		tm_p->tm_min = tasknor_info.endmin;
		tm_p->tm_sec = 0;
		recinfo->rec_end = mktime(tm_p);
		break;
	case 7://实时数据类
		recinfo->recordno_num = (recinfo->rec_end - recinfo->rec_start)/tasknor_info.freq + 1;
		time(&time_s);
		tm_p = localtime(&time_s);
		tm_p->tm_year = select.selec7.collect_save_star.year.data;
		tm_p->tm_mon = select.selec7.collect_save_star.month.data;
		tm_p->tm_mday = select.selec7.collect_save_star.year.data;
		tm_p->tm_hour = select.selec7.collect_save_star.hour.data;
		tm_p->tm_min = select.selec7.collect_save_star.min.data;
		tm_p->tm_sec = select.selec7.collect_save_star.sec.data;
		recinfo->rec_start = mktime(tm_p);

		time(&time_s);
		tm_p = localtime(&time_s);
		tm_p->tm_year = select.selec7.collect_save_finish.year.data;
		tm_p->tm_mon = select.selec7.collect_save_finish.month.data;
		tm_p->tm_mday = select.selec7.collect_save_finish.year.data;
		tm_p->tm_hour = select.selec7.collect_save_finish.hour.data;
		tm_p->tm_min = select.selec7.collect_save_finish.min.data;
		tm_p->tm_sec = select.selec7.collect_save_finish.sec.data;
		recinfo->rec_end = mktime(tm_p);
		break;
	case 10://主动上报类
		recinfo->recordno_num = select.selec10.recordn;
		fprintf(stderr,"\nselect.selec10.recordn=%d\n",select.selec10.recordn);
		recinfo->rec_start = time(NULL);
		recinfo->rec_end = recinfo->rec_start-recinfo->recordno_num*tasknor_info.freq;
		break;
	default:
		memset(recinfo,0x00,sizeof(CURR_RECINFO));
		break;
	}
	return 0;
}
/*
 * 计算当前索引序号
 */
INT8U getcurecord(INT8U selectype,int *curec,int curecn,int runtime)
{
	int currecord = *curec;
	if(selectype == 7 || selectype == 5)
		currecord = (currecord+curecn)%runtime;
	else if(selectype == 10)
	{
		int cnt=0;
		int daymax = 10;//最多可查找十天的，不用while(1),防止死循环
		while(daymax--)
		{
			if(currecord<curecn)
			{
				if(currecord+cnt*runtime >= curecn)
				{
					*curec = currecord+cnt*runtime - curecn;
					return 1;
				}
				cnt++;
			}
			else
			{
				*curec = currecord-curecn;
				fprintf(stderr,"\n\n");
				return 1;
			}
		}
	}
	return 0;
}
/*
 * 摊平csds
 */
void extendcsds(CSD_ARRAYTYPE csds,ROAD_ITEM *item_road)
{
	int i=0,j=0;
	if(csds.num > MY_CSD_NUM)
		csds.num = MY_CSD_NUM;
	for(i=0;i<csds.num;i++)
	{
		asyslog(LOG_INFO,"csds.csd[%d].type=%d\n",i,csds.csd[i].type);
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
			item_road->oad[item_road->oadmr_num].oad_num = 0;//oad类型写为0
			item_road->oadmr_num++;
			asyslog(LOG_INFO,"0000:item_road->oadmr_num=%d\n",item_road->oadmr_num);

			break;
		case 1:
			if(csds.csd[i].csd.road.num > ROAD_OADS_NUM)
				csds.csd[i].csd.road.num = ROAD_OADS_NUM;
			item_road->oad[item_road->oadmr_num].oad_num = csds.csd[i].csd.road.num;//road类型的从oad个数写为实际从oad个数
			for(j=0;j<csds.csd[i].csd.road.num;j++)
			{
				memcpy(&item_road->oad[item_road->oadmr_num].oad_m,&csds.csd[i].csd.road.oad,sizeof(OAD));
				memcpy(&item_road->oad[item_road->oadmr_num].oad_r,&csds.csd[i].csd.road.oads[j],sizeof(OAD));
				item_road->oadmr_num++;
			}
			asyslog(LOG_INFO,"11111:item_road->oadmr_num=%d\n",item_road->oadmr_num);

			break;
		default:break;
		}
	}
}
///*
// *获得任务数据记录
// */
//int GetTaskData(OAD oad,RSD select, INT8U selectype,CSD_ARRAYTYPE csds)
//{
//	FILE *fp = NULL,*myfp = NULL;
//	INT8U 	taskid=0,recordbuf[1000],onefrmbuf[2000];
//	ROAD_ITEM item_road;
//	CURR_RECINFO recinfo;
//	HEAD_UNIT *headunit = NULL;//文件头
//	OAD_INDEX oad_offset[100];//oad索引
//	TASKSET_INFO tasknor_info;
//	INT16U  blocksize=0,headsize=0;
//	int offsetTsa = 0,recordoffset = 0,unitnum=0,i=0,j=0,indexn=0,recordlen = 0,currecord = 0,firecord = 0,tsa_num=0,framesum=0;
//	INT8U recordnum=0,seqnumindex=0;
//	TSA *tsa_group = NULL;
//	memset(&item_road,0x00,sizeof(ROAD_ITEM));
//
//	if(selectype != 5 && selectype != 7 && selectype != 10)
//		return 0;
//
//	if(csds.num > MY_CSD_NUM)
//		csds.num = MY_CSD_NUM;
//	asyslog(LOG_INFO,"普通任务采集方案！！\n");
//	memset(&item_road,0,sizeof(item_road));
//	if((taskid = GetTaskidFromCSDs(csds,&item_road)) == 0) {//暂时不支持招测的不在一个采集方案
//		asyslog(LOG_INFO,"GetTaskData: taskid=%d\n",taskid);
//		return 0;
//	}
//	if(ReadTaskInfo(taskid,&tasknor_info)!=1)//得到任务信息
//	{
//		asyslog(LOG_INFO,"n得到任务信息失败\n");
//		fprintf(stderr,"\n得到任务信息失败\n");
//		return 0;
//	}
//	fprintf(stderr,"\n得到任务信息成功\n");
//
//	memset(&recinfo,0x00,sizeof(CURR_RECINFO));
//	initrecinfo(&recinfo,tasknor_info,selectype,select);//获得recinfo信息
//	//获得第一个序号
//	currecord = getrecordno(tasknor_info.starthour,tasknor_info.startmin,tasknor_info.freq,recinfo);//freq为执行间隔,单位分钟
//	firecord = currecord;//每次切换表地址，当前记录序号赋值第一次的数值
//	//1\打开数据文件
//	fprintf(stderr,"\n----------1\n");
//	fp = opendatafile(taskid,recinfo);
//	myfp = openFramefile(TASK_FRAME_DATA);
//	if (fp==NULL || myfp==NULL)
//		return 0;
//	asyslog(LOG_INFO,"\n打开文件成功\n");
////	ReadFileHeadLen(fp,&headsize,&blocksize);
////	memset(headunit,0x00,sizeof(headunit));
////	fread(headunit,headsize-4,1,fp);
//	unitnum = GetTaskHead(fp,&headsize,&blocksize,&headunit);
//	fprintf(stderr,"\n----------2\n");
//	for(i=0;i<unitnum;i++)
//		fprintf(stderr,"%04x%02x%02x:%04x%02x%02x:%04x\n",
//				headunit[i].oad_m.OI,headunit[i].oad_m.attflg,headunit[i].oad_m.attrindex,
//				headunit[i].oad_r.OI,headunit[i].oad_r.attflg,headunit[i].oad_r.attrindex,headunit[i].len);
//
//	memset(oad_offset,0x00,sizeof(oad_offset));
//	GetOADPosofUnit(item_road,headunit,unitnum,oad_offset);//得到每一个oad在块数据中的偏移
//	fprintf(stderr,"\n----------4\n");
//	recordlen = blocksize/tasknor_info.runtime;//计算每条记录的字节数
//	fprintf(stderr,"\nrecordlen = %d,freq=%d\n",recordlen,tasknor_info.freq);
////	recordno = getrecordno(tasknor_info.starthour,tasknor_info.startmin,tasknor_info.freq,ts_sele);//计算招测的第一个的序列号
//	fprintf(stderr,"\n-----------------------------------1-----------------------------------------------------------\n");
//	//2\获得全部TSA列表
////	fprintf(stderr,"\nmstype=%d recordno=%d\n",select.selec10.meters.mstype,recordno);
//
//	tsa_num = getTsas(select.selec10.meters,(INT8U **)&tsa_group);
//	fprintf(stderr,"get 需要上报的：tsa_num=%d,tsa_group=%p\n",tsa_num,tsa_group);
//	for(i=0;i<tsa_num;i++) {
//		fprintf(stderr,"\nTSA%d: %d-",i,tsa_group[i].addr[0]);
//		for(j=0;j<tsa_group[i].addr[0];j++) {
//			fprintf(stderr,"-%02x",tsa_group[i].addr[j+1]);
//		}
//	}
//	fprintf(stderr,"\n----------------------------------2------------------------------------------------------------\n");
//	memset(onefrmbuf,0,sizeof(onefrmbuf));
//	//初始化分帧头
//	indexn = 2;
//	indexn += initFrameHead(&onefrmbuf[indexn],oad,select,selectype,csds,&seqnumindex);
//
//	//3\定位TSA , 返回offset
//	for(i =0; i< tsa_num; i++)
//	{
//		currecord = firecord;//每次切换表地址，当前记录序号赋值第一次的数值
//		offsetTsa = findTsa(tsa_group[i],fp,headsize,blocksize);
//		fprintf(stderr,"\n-----offsetTsa = %d\n",offsetTsa);
//		//4\计算当前点
////		currecord = getrecordno(tasknor_info.starthour,tasknor_info.startmin,tasknor_info.freq,recinfo);//freq为执行间隔,单位分钟
////		for(j=0; j<recordn ; j++)
//		asyslog(LOG_INFO,"招测的序列总数%d\n",recinfo.recordno_num);
//		for(j=1; j<=recinfo.recordno_num;j++)		//test
//		{
//			if(updatedatafp(fp,j,selectype,tasknor_info.freq,recinfo,taskid)==2)//更新数据流 事件不需要更新
//				offsetTsa = findTsa(tsa_group[i],fp,headsize,blocksize);
//			if(offsetTsa == 0) {
//				asyslog(LOG_INFO,"task未找到数据,i=%d\n",i);
//				indexn += fillTsaNullData(&onefrmbuf[indexn],tsa_group[i],item_road);
//				recordnum++;
//				continue;
//			}
//			//5\定位指定的点（行）, 返回offset
//			if(getcurecord(selectype,&currecord,j,tasknor_info.runtime) == 0)//招测天数跨度超出10天
//				break;
//			asyslog(LOG_INFO,"\n计算出来的currecord=%d\n",currecord);
//			recordoffset = findrecord(offsetTsa,recordlen,currecord);
//			memset(recordbuf,0x00,sizeof(recordbuf));
//			//6\读出一行数据到临时缓存
//			fseek(fp,recordoffset,SEEK_SET);
//			fread(recordbuf,recordlen,1,fp);
//			printRecordBytes(recordbuf,recordlen);
//			//7\根据csds挑选数据，组织存储缓存
//			indexn += collectData(&onefrmbuf[indexn],recordbuf,oad_offset,item_road);
//			recordnum++;
//			asyslog(LOG_INFO,"recordnum=%d  seqnumindex=%d\n",recordnum,seqnumindex);
//			if (indexn>=1000)
//			{
//				framesum++;
//				//8 存储1帧
//				intToBuf((indexn-2),onefrmbuf);		//帧长度保存帧的数据长度
//				saveOneFrame(onefrmbuf,indexn,myfp);
//				indexn = 2;
//				indexn += initFrameHead(&onefrmbuf[indexn],oad,select,selectype,csds,&seqnumindex);
//				onefrmbuf[seqnumindex] = recordnum;
//				recordnum = 0;
//				break;
//			}
//		}
//	}
//	asyslog(LOG_INFO,"组帧：indexn=%d\n",indexn);
//	for(i=0;i<indexn;i++) {
//		fprintf(stderr,"%02x ",onefrmbuf[i]);
//	}
//
//	if(framesum==0) {
//		fprintf(stderr,"\n indexn = %d saveOneFrame  seqnumindex=%d,  recordnum=%d!!!!!!!!!!!!!!!!\n",indexn,seqnumindex,recordnum);
//		asyslog(LOG_INFO,"任务数据文件组帧:indexn = %d , seqnumindex=%d,  recordnum=%d\n",indexn,seqnumindex,recordnum);
//		intToBuf((indexn-2),onefrmbuf);
//		onefrmbuf[seqnumindex] = recordnum;
//		saveOneFrame(onefrmbuf,indexn,myfp);
//	}
//	if(tsa_group != NULL)
//		free(tsa_group);
//	if(headunit!=NULL){
//		free(headunit);
//	}
//	if(fp != NULL)
//		fclose(fp);
//	if(myfp != NULL)
//		fclose(myfp);
//	return (framesum+1);
//}
/*
 *获得任务数据和事件记录
 */
int GetTaskData(OAD oad,RSD select, INT8U selectype,CSD_ARRAYTYPE csds)
{
	FILE *fp = NULL,*myfp = NULL;
	INT8U 	taskid=0,recordbuf[1000],onefrmbuf[2000];
	ROAD_ITEM item_road;
	CURR_RECINFO recinfo;
	HEAD_UNIT *headunit = NULL;//文件头
	OAD_INDEX oad_offset[100];//oad索引
	TASKSET_INFO tasknor_info;
	INT16U  blocksize=0,headsize=0;
	int offsetTsa = 0,recordoffset = 0,unitnum=0,i=0,j=0,indexn=0,recordlen = 0,currecord = 0,firecord = 0,tsa_num=0,framesum=0;
	INT8U recordnum=0,seqnumindex=0;
	TSA *tsa_group = NULL;
	ROAD road_eve;
	INT8U eveflg=0;
	memset(&item_road,0x00,sizeof(ROAD_ITEM));

	if(selectype != 5 && selectype != 7 && selectype != 10)
		return 0;

	if(csds.num > MY_CSD_NUM)
		csds.num = MY_CSD_NUM;
	for(i=0;i<csds.num;i++)//招测单个事件
	{
		if(csds.csd[i].type != 1)
			continue;
		if(csds.csd[i].csd.road.oad.OI >= 0x3000 && csds.csd[i].csd.road.oad.OI < 0x4000)//事件关联对象
		{
			eveflg = 1;
			memcpy(&road_eve,&csds.csd[i].csd.road,sizeof(ROAD));
			break;
		}
	}
	if(eveflg == 1)
	{
		asyslog(LOG_INFO,"事件任务采集方案！！\n");
		memset(&item_road,0,sizeof(item_road));
		extendcsds(csds,&item_road);
		currecord = 0;//事件只有一个记录
		firecord = 0;
		memset(&recinfo,0x00,sizeof(CURR_RECINFO));
		recinfo.recordno_num = 1;//获得recinfo信息
		tasknor_info.runtime = 1;//写死为一天执行一次,来保证一天存一个记录,实际按间隔执行

		fp = openevefile(road_eve.oad.OI);
	}
	else
	{
		asyslog(LOG_INFO,"普通任务采集方案！！\n");
		memset(&item_road,0,sizeof(item_road));
		if((taskid = GetTaskidFromCSDs(csds,&item_road)) == 0) {//暂时不支持招测的不在一个采集方案
			asyslog(LOG_INFO,"GetTaskData: taskid=%d\n",taskid);
			return 0;
		}
		if(ReadTaskInfo(taskid,&tasknor_info)!=1)//得到任务信息
		{
			asyslog(LOG_INFO,"n得到任务信息失败\n");
			fprintf(stderr,"\n得到任务信息失败\n");
			return 0;
		}
		fprintf(stderr,"\n得到任务信息成功\n");

		memset(&recinfo,0x00,sizeof(CURR_RECINFO));
		initrecinfo(&recinfo,tasknor_info,selectype,select);//获得recinfo信息
		//获得第一个序号
		currecord = getrecordno(tasknor_info.starthour,tasknor_info.startmin,tasknor_info.freq,recinfo);//freq为执行间隔,单位分钟
		firecord = currecord;//每次切换表地址，当前记录序号赋值第一次的数值
		//1\打开数据文件
		fprintf(stderr,"\n----------1\n");
		fp = opendatafile(taskid,recinfo);
	}
	myfp = openFramefile(TASK_FRAME_DATA);
	if (fp==NULL || myfp==NULL)
		return 0;
	asyslog(LOG_INFO,"\n打开文件成功\n");
//	ReadFileHeadLen(fp,&headsize,&blocksize);
//	memset(headunit,0x00,sizeof(headunit));
//	fread(headunit,headsize-4,1,fp);
	unitnum = GetTaskHead(fp,&headsize,&blocksize,&headunit);
	fprintf(stderr,"\n----------2\n");
	for(i=0;i<unitnum;i++)
		fprintf(stderr,"%04x%02x%02x:%04x%02x%02x:%04x\n",
				headunit[i].oad_m.OI,headunit[i].oad_m.attflg,headunit[i].oad_m.attrindex,
				headunit[i].oad_r.OI,headunit[i].oad_r.attflg,headunit[i].oad_r.attrindex,headunit[i].len);

	memset(oad_offset,0x00,sizeof(oad_offset));
	GetOADPosofUnit(item_road,headunit,unitnum,oad_offset);//得到每一个oad在块数据中的偏移
	fprintf(stderr,"\n----------4\n");
	recordlen = blocksize/tasknor_info.runtime;//计算每条记录的字节数
	fprintf(stderr,"\nrecordlen = %d,freq=%d\n",recordlen,tasknor_info.freq);
//	recordno = getrecordno(tasknor_info.starthour,tasknor_info.startmin,tasknor_info.freq,ts_sele);//计算招测的第一个的序列号
	fprintf(stderr,"\n-----------------------------------1-----------------------------------------------------------\n");
	//2\获得全部TSA列表
//	fprintf(stderr,"\nmstype=%d recordno=%d\n",select.selec10.meters.mstype,recordno);

	tsa_num = getTsas(select.selec10.meters,(INT8U **)&tsa_group);
	fprintf(stderr,"get 需要上报的：tsa_num=%d,tsa_group=%p\n",tsa_num,tsa_group);
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
		currecord = firecord;//每次切换表地址，当前记录序号赋值第一次的数值
		offsetTsa = findTsa(tsa_group[i],fp,headsize,blocksize);
		fprintf(stderr,"\n-----offsetTsa = %d\n",offsetTsa);
		//4\计算当前点
//		currecord = getrecordno(tasknor_info.starthour,tasknor_info.startmin,tasknor_info.freq,recinfo);//freq为执行间隔,单位分钟
//		for(j=0; j<recordn ; j++)
		asyslog(LOG_INFO,"招测的序列总数%d\n",recinfo.recordno_num);
		for(j=1; j<=recinfo.recordno_num;j++)		//test
		{
			if(updatedatafp(fp,j,selectype,tasknor_info.freq,recinfo,taskid)==2 && eveflg != 1)//更新数据流 事件不需要更新
				offsetTsa = findTsa(tsa_group[i],fp,headsize,blocksize);
			if(offsetTsa == 0) {
				asyslog(LOG_INFO,"task未找到数据,i=%d\n",i);
				indexn += fillTsaNullData(&onefrmbuf[indexn],tsa_group[i],item_road);
				recordnum++;
				continue;
			}
			//5\定位指定的点（行）, 返回offset
			if(getcurecord(selectype,&currecord,j,tasknor_info.runtime) == 0)//招测天数跨度超出10天
				break;
			asyslog(LOG_INFO,"\n计算出来的currecord=%d\n",currecord);
			recordoffset = findrecord(offsetTsa,recordlen,currecord);
			memset(recordbuf,0x00,sizeof(recordbuf));
			//6\读出一行数据到临时缓存
			fseek(fp,recordoffset,SEEK_SET);
			fread(recordbuf,recordlen,1,fp);
			printRecordBytes(recordbuf,recordlen);
			//7\根据csds挑选数据，组织存储缓存
			indexn += collectData(&onefrmbuf[indexn],recordbuf,oad_offset,item_road);
			recordnum++;
			asyslog(LOG_INFO,"recordnum=%d  seqnumindex=%d\n",recordnum,seqnumindex);
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
	asyslog(LOG_INFO,"组帧：indexn=%d\n",indexn);
	for(i=0;i<indexn;i++) {
		fprintf(stderr,"%02x ",onefrmbuf[i]);
	}

	if(framesum==0) {
		fprintf(stderr,"\n indexn = %d saveOneFrame  seqnumindex=%d,  recordnum=%d!!!!!!!!!!!!!!!!\n",indexn,seqnumindex,recordnum);
		asyslog(LOG_INFO,"任务数据文件组帧:indexn = %d , seqnumindex=%d,  recordnum=%d\n",indexn,seqnumindex,recordnum);
		intToBuf((indexn-2),onefrmbuf);
		onefrmbuf[seqnumindex] = recordnum;
		saveOneFrame(onefrmbuf,indexn,myfp);
	}
	if(tsa_group != NULL)
		free(tsa_group);
	if(headunit!=NULL){
		free(headunit);
	}
	if(fp != NULL)
		fclose(fp);
	if(myfp != NULL)
		fclose(myfp);
	return (framesum+1);
}
int getSelector(OAD oad_h,RSD select, INT8U selectype, CSD_ARRAYTYPE csds, INT8U *data, int *datalen)
{
	int  framesum=0;		//分帧
	asyslog(LOG_INFO,"getSelector: selectype=%d\n",selectype);
	switch(selectype)
	{
	case 5:
		framesum = GetTaskData(oad_h,select,selectype,csds);
		break;
	case 7:
		framesum = GetTaskData(oad_h,select,selectype,csds);//程序里面计算
		break;
	case 10:
		framesum = GetTaskData(oad_h,select,selectype,csds);
		fprintf(stderr,"framesum=%d\n",framesum);
		break;
	default:break;
	}
	return framesum;
}

/*
 * 返回： 当前文件读取偏移位置
 */
long int readFrameDataFile(char *filename,int offset,INT8U *buf,int *datalen)
{
	FILE *fp=NULL;
	int bytelen=0;
	long int	retoffset=0;

	*datalen = 0;
	fp = fopen(filename,"r");
	if (fp!=NULL && buf!=NULL)
	{
		fseek(fp,offset,0);		 			//定位到文件指定偏移位置
		//if (fread(&bytelen,2,1,fp) <=0)	 	//读出数据报文长度
		fread(&bytelen,2,1,fp);
		fprintf(stderr,"bytelen=%d\n",bytelen);
//			return 0;
		if(bytelen>=MAX_APDU_SIZE) {
			return 0;
		}
		if (fread(buf,bytelen,1,fp) <=0 ) 	//按数据报文长度，读出全部字节
			return 0;
		*datalen = bytelen;
		retoffset = ftell(fp);
		fclose(fp);
		return retoffset;		 			//返回当前偏移位置
	}
	return 0;
}
