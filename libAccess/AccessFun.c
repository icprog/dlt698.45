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

//syslog_info 信息记录标记
//#define		SYS_INFO		1

CLASS_INFO	info={};
void write_apn(char* apn) {
	syslog(LOG_NOTICE,"__%s__",__func__);
    FILE* fp;
    fp = fopen("/etc/ppp/gprs-connect-chat", "w");
    if (fp == NULL) {
        return;
    }
    fprintf(fp, "TIMEOUT        5\n");
    fprintf(fp, "ABORT        \'\\nBUSY\\r\'\n");
    fprintf(fp, "ABORT        \'\\nNO ANSWER\\r\'\n");
    fprintf(fp, "ABORT        \'\\nRINGING\\r\\n\\r\\nRINGING\\r\'\n");
    fprintf(fp, "ABORT        \'\\nNO CARRIER\\r\'\n");
    fprintf(fp, "\'\' \\rAT\n");
    fprintf(fp, "TIMEOUT        5\n");
    fprintf(fp, "\'OK-+++\\c-OK\'    ATH\n");
    fprintf(fp, "TIMEOUT        20\n");
    fprintf(fp, "OK        AT+CREG?\n");
    fprintf(fp, "TIMEOUT        10\n");
    fprintf(fp, "OK        AT+CGATT=1\n");
    fprintf(fp, "TIMEOUT        300\n");
    fprintf(fp, "OK        AT+CGATT?\n");
    fprintf(fp, "OK        AT+CFUN=1\n");
    fprintf(fp, "TIMEOUT        5\n");
    fprintf(fp, "OK        AT+CPIN?\n");
    fprintf(fp, "TIMEOUT        5\n");
    fprintf(fp, "OK        AT+CSQ\n");
    fprintf(fp, "TIMEOUT        5\n");
    fprintf(fp, "OK        AT+CGDCONT=1,\"IP\",\"%s\"\n", apn);
    fprintf(fp, "OK        ATDT*99***1#\n");
    fprintf(fp, "CONNECT \'\'\n");
    fclose(fp);
    fp = NULL;
}

void write_userpwd(unsigned char* user, unsigned char* pwd, unsigned char* apn) {
	syslog(LOG_NOTICE,"__%s__",__func__);
	FILE* fp = NULL;
    fp       = fopen("/etc/ppp/chap-secrets", "w");
    fprintf(fp, "\"%s\" * \"%s\" *", user, pwd);
    fclose(fp);

    fp = fopen("/etc/ppp/pap-secrets", "w");
    fprintf(fp, "\"%s\" * \"%s\" *", user, pwd);
    fclose(fp);

    fp = fopen("/etc/ppp/peers/cdma2000", "w");
    fprintf(fp, "/dev/mux0\n");
    fprintf(fp, "115200\n");
    fprintf(fp, "modem\n");
    fprintf(fp, "debug\n");
    fprintf(fp, "nodetach\n");
    fprintf(fp, "usepeerdns\n");
    fprintf(fp, "noipdefault\n");
    fprintf(fp, "defaultroute\n");
    fprintf(fp, "user \"%s\"\n", user);
    fprintf(fp, "0.0.0.0:0.0.0.0\n");
    fprintf(fp, "ipcp-accept-local\n");
    fprintf(fp, "connect 'chat -s -v -f /etc/ppp/cdma2000-connect-chat'\n");
    fprintf(fp, "disconnect \"chat -s -v -f /etc/ppp/gprs-disconnect-chat\"\n");
    fclose(fp);

    fp = fopen("/etc/ppp/cdma2000-connect-chat", "w");
    fprintf(fp, "TIMEOUT        5\n");
    fprintf(fp, "ABORT        \"DELAYED\"\n");
    fprintf(fp, "ABORT        \"BUSY\"\n");
    fprintf(fp, "ABORT        \"ERROR\"\n");
    fprintf(fp, "ABORT        \"NO DIALTONE\"\n");
    fprintf(fp, "ABORT        \"NO CARRIER\"\n");
    fprintf(fp, "'' AT\n");
    fprintf(fp, "TIMEOUT        5\n");
    fprintf(fp, "'OK-+++\c-OK' ATZ\n");
    fprintf(fp, "TIMEOUT        20\n");
    fprintf(fp, "OK        AT+CREG?\n");
    fprintf(fp, "TIMEOUT        10\n");
    fprintf(fp, "OK        AT$MYNETCON=0,\"USERPWD\",\"%s,%s\"\n", user, pwd);
    fprintf(fp, "TIMEOUT        10\n");
//    fprintf(fp, "OK        AT$MYNETCON=0,\"APN\",\"%s\"\n", apn);
//    fprintf(fp, "TIMEOUT        10\n");
//    fprintf(fp, "OK        AT$MYNETCON=0,\"AUTH\",1\n");
//    fprintf(fp, "TIMEOUT        10\n");
    fprintf(fp, "OK ATDT#777\n");
    fprintf(fp, "CONNECT ''\n");
    fclose(fp);
}

/*
 * 总表(交采)计量电量数据清除
 * */
void clearEnergy()
{
	syslog(LOG_NOTICE,"__%s__",__func__);
	//总表计量电量数据清除
	system("rm -rf /nor/acs/energy.par");
	system("rm -rf /nor/acs/energy.bak");
}

void clearData()
{
	syslog(LOG_NOTICE,"__%s__",__func__);
	//冻结类数据清除
	system("rm -rf /nand/task");
	//统计类数据清除
	system("rm -rf /nand/data");
	//全事件数据清除
	system("rm -rf /nand/allevent");
	//删除6035
	system("rm -rf /nand/para/6035");
	//删除抄表记录状态
	system("rm -rf /nand/para/plcrecord.par");
	system("rm -rf /nand/para/plcrecord.bak");
}

void clearEvent()
{
	//事件类数据清除
	INT8U*	eventbuff=NULL;
	int 	saveflg=0,i=0;
	int		classlen=0;
	Class7_Object	class7={};

	syslog(LOG_NOTICE,"__%s__",__func__);
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
	syslog(LOG_NOTICE,"__%s__",__func__);
	//需量类数据清除
	system("rm -rf /nand/demand");
}

void paraInit(INT8U oadnum,OAD *oad)
{
	int		i=0;
	if(oadnum == 0) {	//恢复出厂参数
		system("rm -rf /nand/para");
		system("rm -rf /nand/event/property");
		InitClass4016();    //当前套日时段表
		InitClass4300();    //电气设备信息
		//InitClass6000();	//初始化交采采集档案
	    InitClassf203();	//开关量输入
		InitClassByZone(0);		//根据地区进行相应初始化	4500,4510参数
	}else {
		for(i=0;i<oadnum;i++) {

		}
	}
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
//    if(interval>60*1000*1000) {
    	syslog(LOG_ERR,"初始化时间=%f(ms)\n", interval/1000.0);
//    }
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

	syslog(LOG_NOTICE,"__%s__,oi=%04x,seqnum=%d",__func__,oi,seqnum);
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
	if(blockdata!=NULL)		free(blockdata);
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

	syslog(LOG_NOTICE,"__%s__",__func__);
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

	syslog(LOG_NOTICE,"__%s__",__func__);

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
 * 返回值：=0：文件保存成功，=1，文件保存失败，此时建议产生ERC2参数丢失事件通知主站异常
 * =-1:  未查找到OI类数据
 */

int saveParaClass(OI_698 oi,void *blockdata,int seqnum)
{
	int 	ret=-1;
	INT16S	infoi=-1;
	sem_t   *sem_save=NULL;

#ifdef SYS_INFO
	syslog(LOG_NOTICE,"__%s__,oi=%04x,seqnum=%d",__func__,oi,seqnum);
#endif
	infoi = getclassinfo(oi,&info);
	if(infoi == -1) {
		return -1;
	}
	switch(oi){
	case 0x6000:
		if(seqnum>MAX_POINT_NUM) {
			syslog(LOG_ERR,"save oi=%x seqnum=%d 超过限值 %d\n,无法存储！！",oi,seqnum,MAX_POINT_NUM);
		}
		break;
	}
	sem_save = InitSem();
	makeSubDir(PARADIR);
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
#ifdef SYS_INFO
	syslog(LOG_NOTICE,"__%s__,oi=%04x,seqnum=%d",__func__,oi,seqnum);
#endif
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
 * 返回值：=0：文件存储成功
 */
int saveCoverClass(OI_698 oi,INT16U seqno,void *blockdata,int savelen,int type)
{
	int		ret = refuse_rw;
	char	fname[FILENAMELEN]={};
	sem_t   *sem_save=NULL;

#ifdef SYS_INFO
	syslog(LOG_NOTICE,"__%s__,type=%d,oi=%04x,seqno=%d",__func__,type,oi,seqno);
#endif
	sem_save = InitSem();
	memset(fname,0,sizeof(fname));
	getFileName(oi,seqno,type,fname);
	switch(type) {
	case event_para_save:
	case para_vari_save:
	case coll_para_save:
	case acs_energy_save:
	case para_init_save:
		fprintf(stderr,"saveClass file=%s ",fname);
		ret = save_block_file(fname,blockdata,savelen,0,0);
		break;
	case acs_coef_save:
		file_write_accoef(fname,blockdata,savelen);
		//将校表系数文件再测写入/nor/config/accoe.par,防止运行3761程序找不到校表系数文件
		sprintf(fname,"%s/accoe.par",_CFGDIR_);
		file_write_accoef(fname,blockdata,savelen);
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

	memset(fname,0,sizeof(fname));
	ret = readFileName(oi,seqno,type,fname);
	if(ret!=0) {	//文件不存在
		return -1;
	}
#ifdef SYS_INFO
	syslog(LOG_NOTICE,"__%s__,type=%d,oi=%04x,seqno=%d",__func__,type,oi,seqno);
#endif
	sem_save = InitSem();
	switch(type) {
	case event_para_save:
//		ret = readFileName(oi,seqno,type,fname);
//		if(ret==0) {		//文件存在
//			ret = block_file_sync(fname,blockdata,datalen,0,0);
//		}else  {		//无配置文件，读取系统初始化参数
//			memset(fname,0,sizeof(fname));
//			ret = readFileName(oi,seqno,para_init_save,fname);
////			fprintf(stderr,"read /nor/init的参数文件：  Class %s filelen=%d\n",fname,datalen);
//			if(ret==0) {	//文件存在
//				ret = block_file_sync(fname,blockdata,datalen,0,0);
//			}
//		}
//		break;
	case para_vari_save:
	case coll_para_save:
	case acs_energy_save:
//		ret = readFileName(oi,seqno,type,fname);
//		syslog(LOG_NOTICE,"read type=%d,oi=%04x,seqno=%d ret=%d",type,oi,seqno,ret);/////1
		if(ret==0) {		//文件存在
//			fprintf(stderr,"readClass %s filelen=%d,type=%d\n",fname,datalen,type);
			ret = block_file_sync(fname,blockdata,datalen,0,0);
		}
		break;
	case acs_coef_save:
//		ret = readFileName(oi,seqno,type,fname);
		if(ret==0) {		//文件存在
			ret = fu_read_accoef(fname,blockdata,datalen);
		}
		break;
	case para_init_save:
//		ret = readFileName(oi,seqno,type,fname);
		fprintf(stderr,"para_init_save readClass %s filelen=%d\n",fname,datalen);
		if(ret==0) {
			ret = block_file_sync(fname,blockdata,datalen,0,0);
		}
	break;
	case event_record_save:
	case event_current_save:
//		ret = readFileName(oi,seqno,type,fname);
		if(ret==0) {
			ret = readCoverFile(fname,blockdata,datalen);
		}
		break;
	}
	//信号量post，注意正常退出
	CloseSem(sem_save);
//	syslog(LOG_NOTICE,"readCoverClass ret=%d",ret);//////7
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

#ifdef SYS_INFO
	syslog(LOG_NOTICE,"__%s__,oi=%04x,coll_seqnum=%d",__func__,oi,coll_seqnum);
#endif
	if(blockdata==NULL) {
		fprintf(stderr,"存储数据为空，不可保存\n");
		return -1;
	}
	type = getvarioffset(oi,coll_seqnum,&offset,&blklen);
//	fprintf(stderr,"oi=%04x offset=%d ,blklen=%d, type=%d\n",oi,offset,blklen,type);
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
	case 3:
		memcpy(filename,PLUSE_ENERGY_DATA,sizeof(PLUSE_ENERGY_DATA));
		break;
	}
	if(access(filename,F_OK)!=0)
	{
		fp = fopen(filename, "w+");
		fprintf(stderr,"创建文件 %s\n",filename);
	}else {
		fp = fopen(filename, "r+");
//		fprintf(stderr,"替换文件 %s\n",filename);
	}
	if (fp != NULL) {
		if(wbuf==NULL) {
			wbuf = malloc(blklen);
			memset(wbuf,0,blklen);
			wbuf[0] = datalen;
			memcpy(wbuf+1,blockdata,datalen);
//			fprintf(stderr,"set to %d, datalen=%d ",offset,datalen);
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

#ifdef SYS_INFO
	syslog(LOG_NOTICE,"__%s__,oi=%04x,coll_seqnum=%d",__func__,oi,coll_seqnum);
#endif
	if(len > VARI_LEN && oi!= PORT_PLUSE) {
		fprintf(stderr,"读取【oi=%x】数据长度[%d]大于申请返回数据空间[%d]，返回失败!!!\n",oi,len,VARI_LEN);
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
	case 3:
		fp = fopen(PLUSE_ENERGY_DATA, "r");
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
//				fprintf(stderr,"rbuf[0]=%d\n",rbuf[0]);
				if(rbuf[0]==0) {
					retlen = 0;
				}else {
					memcpy((char *)blockdata+retlen,&rbuf[1],len);	//第一个字节为有效长度
					retlen+=len;
				}
			}
			if(rbuf!=NULL) {
//				fprintf(stderr,"free rbuf\n");
				free(rbuf);
			}
		}
		fclose(fp);
	}
	fprintf(stderr,"retlen=%d\n",retlen);
	CloseSem(sem_save);
	return retlen;
}

int getFreezeMaxRecord(OI_698 freezeoi,OI_698 recordoi)
{
	FreezeObject	FreeObj={};
//	OI_698			tmpOI[3]={};
	int		i=0;
	int		maxRecord = 0,tmprec=0;

//	memset(&tmpOI,0,sizeof(tmpOI));
//	if(recordoi == 0x2130) {	//电压合格率
//		tmpOI[0] = 0x2131;		//Ua
//		tmpOI[1] = 0x2132;		//Ub
//		tmpOI[2] = 0x2133;		//Uc
//	}else {
//		tmpOI[0] = recordoi;
//	}
	fprintf(stderr,"getFreezeMaxRecord %04x---%04x\n",freezeoi,recordoi);
	memset(&FreeObj,0,sizeof(FreezeObject));
	readCoverClass(freezeoi,0,&FreeObj,sizeof(FreezeObject),para_vari_save);
	fprintf(stderr,"FreeObj.RelateNum=%d\n",FreeObj.RelateNum);
	for(i=0;i<FreeObj.RelateNum;i++) {
//		if((tmpOI[0]==FreeObj.RelateObj[i].oad.OI) || (tmpOI[1]==FreeObj.RelateObj[i].oad.OI) || (tmpOI[2]==FreeObj.RelateObj[i].oad.OI)) {
		if(recordoi == FreeObj.RelateObj[i].oad.OI) {
			tmprec = FreeObj.RelateObj[i].saveDepth;
			if(tmprec > maxRecord) {
				maxRecord = tmprec;
			}
		}
	}
	fprintf(stderr,"maxRecord=%d\n",maxRecord);
	if(maxRecord == 0) maxRecord = 1;		//防止计算记录数异常
	return maxRecord;
}
////////////////////////////////////////////////////////////////////////////////
/*
 * 冻结数据记录单元存储
 * 每条记录数据内容固定64个字节：格式  OAD + 冻结时间 + Data
 * 返回 = 1： 写成功
 *     = 0： 失败
 * */
int	saveFreezeRecord(OI_698 freezeOI,OAD oad,DateTimeBCD datetime,int len,INT8U *data)
{
	int 	ret = 0;
	int		maxRecord = 0,currRecord = 0;
	int		offset = 0;
	int	 	fd=0;
	FILE 	*fp=NULL;
	char 	filename[128]={};
	int		blklen = 0;
	sem_t   *sem_save=NULL;

#ifdef SYS_INFO
	syslog(LOG_NOTICE,"__%s__,freezeOI=%04x,oad.oi=%04x",__func__,freezeOI,oad.OI);
#endif
	if(len>VARI_LEN) {
		fprintf(stderr,"save %s/%04x-%04x.dat 数据长度[%d]大于限定值[%d],不予保存",VARI_DIR,freezeOI,oad.OI,len,VARI_LEN);
	}
	sem_save = InitSem();
	memset(&filename,0,sizeof(filename));
	makeSubDir(VARI_DIR);
	sprintf(filename,"%s/%04x-%04x.dat",VARI_DIR,freezeOI,oad.OI);
	fprintf(stderr," saveFreezeRecord filename=%s\n",filename);
	if(access(filename,F_OK)!=0)
	{
		fp = fopen((char*) filename, "w+");
		maxRecord = getFreezeMaxRecord(freezeOI,oad.OI);
		currRecord = 0;
	}else {
		fp = fopen((char*) filename, "r+");
		if(fp!=NULL) {
			fseek(fp,0,SEEK_SET);				//定位到文件头
			fread(&currRecord,2,1,fp);				//读出当前记录长度
			fread(&maxRecord,2,1,fp);				//读出最大记录数
		}
	}
	if(fp!=NULL) {
		blklen = VARI_LEN+sizeof(DateTimeBCD);
		offset = currRecord*blklen + 4; 		//+4 ：文件头的当前记录与最大记录
		fseek(fp,offset,SEEK_SET);
		fwrite(&datetime,sizeof(DateTimeBCD),1,fp);
		fwrite(&len,2,1,fp);			//数据有效长度
		ret = fwrite(data,len,1,fp);
		fprintf(stderr,"ret=%d",ret);
		if(maxRecord) {
			currRecord = (currRecord + 1) % maxRecord;
		}
		fseek(fp,0,SEEK_SET);
		fprintf(stderr,"currRecord=%d,maxRecord=%d",currRecord,maxRecord);
		fwrite(&currRecord,2,1,fp);
		fwrite(&maxRecord,2,1,fp);
		fd = fileno(fp);
		fsync(fd);
		fclose(fp);
	}
	CloseSem(sem_save);
	return ret;
}

/*
 * 读取：冻结数据记录单元的最大数及当前记录数
 * 返回 currRecordNum：当前记录数
 * 		MaxRecordNum：冻结深度
 * 	   = 1： 成功
 *     = 0： 失败
 * */
int readFreezeRecordNum(OI_698 freezeOI,OI_698 relateOI,int *currRecordNum,int *MaxRecordNum)
{
	int		ret = 0;
	FILE 	*fp=NULL;
	char 	filename[FILENAMELEN]={};
	int		tmp1=0;
	sem_t   *sem_save=NULL;

#ifdef SYS_INFO
	syslog(LOG_NOTICE,"__%s__,freezeOI=%04x,relateOI=%04x",__func__,freezeOI,relateOI);
#endif
	sem_save = InitSem();
	*currRecordNum = 0;
	*MaxRecordNum = 0;
	memset(&filename,0,sizeof(filename));
	sprintf(filename,"%s/%04x-%04x.dat",VARI_DIR,freezeOI,relateOI);
	fp = fopen((char*)filename, "r");
	if(fp!=NULL) {
		ret = fread(&tmp1,2,1,fp);
		if(ret==1) 	*currRecordNum = tmp1;
		ret = fread(&tmp1,2,1,fp);
		if(ret==1) 	*MaxRecordNum = tmp1;
//		fprintf(stderr,"currRecord=%d,maxRecord=%d\n",*currRecordNum,*MaxRecordNum);
		fclose(fp);
	}
	CloseSem(sem_save);
	return ret;
}
/*
 * 冻结数据记录单元读取
 *     根据冻结记录序号
 * */
int readFreezeRecordByNum(OI_698 freezeOI,OAD oad,int RecordNum,DateTimeBCD *datetime,int *datalen,INT8U *data)
{

	int 	ret = 0;
	long	offset = 0, blklen=0;
	long int filesize=0;
	FILE 	*fp=NULL;
	char 	filename[FILENAMELEN]={};
//	int		i=0;
	sem_t   *sem_save=NULL;

#ifdef SYS_INFO
	syslog(LOG_NOTICE,"__%s__,freezeOI=%04x,oad=%04x,RecordNum=%d",__func__,freezeOI,oad.OI,RecordNum);
#endif
	sem_save = InitSem();

	memset(&filename,0,sizeof(filename));
	sprintf(filename,"%s/%04x-%04x.dat",VARI_DIR,freezeOI,oad.OI);
	fp = fopen((char*) filename, "r");
	if(fp!=NULL && data!=NULL) {
	    fseek( fp, 0L, SEEK_END );
	    filesize=ftell(fp);
		blklen = VARI_LEN+sizeof(DateTimeBCD);
		offset = RecordNum*blklen+4;
		if(offset <= filesize) {			//fseek 设置offset大于文件长度返回值仍然是0，成功，故此处用文件长度进行比较
			ret = fseek(fp,offset,SEEK_SET);
			fread(datetime,sizeof(DateTimeBCD),1,fp);
	//		fprintf(stderr,"%04d-%02d-%02d %02d:%02d:%02d \n",datetime->year.data,datetime->month.data,datetime->day.data,
	//				datetime->hour.data,datetime->min.data,datetime->sec.data);
			fread(datalen,2,1,fp);
			if(*datalen > VARI_LEN) {
				ret = 0;
				syslog(LOG_ERR,"读取数据长度%d 大于限值 %d,失败！！！\n",*datalen,VARI_LEN);
			}else {
 				ret = fread(data,*datalen,1,fp);
			}
//			fprintf(stderr,"datalen=%d ret=%d\n",*datalen,ret);
//			for(i=0;i<*datalen;i++) {
//				fprintf(stderr,"%02x ",data[i]);
//			}
		}
		fclose(fp);
	}
	CloseSem(sem_save);
	return ret;
}
/*
 * 冻结数据记录单元读取
 *     根据冻结时标读取记录
 * */
int	readFreezeRecordByTime(OI_698 freezeOI,OAD oad,DateTimeBCD datetime,int *datalen,INT8U *data)
{
	int 	ret = 0;
	int		num = 0,i=0;
	int		maxRecord = 0,currRecord = 0,blklen=0;
	FILE 	*fp=NULL;
	char 	filename[FILENAMELEN]={};
	DateTimeBCD	RecordTime={};
//	OAD		saveoad={};
	long int filesize=0,offset=0;

	sem_t   *sem_save=NULL;

#ifdef SYS_INFO
	syslog(LOG_NOTICE,"__%s__,freezeOI=%04x,oad=%04x,[%04d-%02d-%02d %02d:%02d:%02d]",__func__,freezeOI,oad.OI,datetime.year.data,datetime.month.data,datetime.day.data,datetime.hour.data,datetime.min.data,datetime.sec.data);
#endif
	sem_save = InitSem();

	memset(&filename,0,sizeof(filename));
	sprintf(filename,"%s/%04x-%04x.dat",VARI_DIR,freezeOI,oad.OI);
	fp = fopen((char*) filename, "r");
	fprintf(stderr,"read filename=%s\n",filename);
	if(fp!=NULL && data!=NULL) {
	    fseek( fp, 0L, SEEK_END );
	    filesize=ftell(fp);
	    fseek(fp, 0L, SEEK_SET);
		num = 0;
		fread(&currRecord,2,1,fp);
		fread(&maxRecord,2,1,fp);
//		fprintf(stderr,"currRecord=%d,maxRecord=%d\n",currRecord,maxRecord);
		blklen = VARI_LEN+sizeof(DateTimeBCD);
		for(i=0;i<maxRecord;i++) {
			fread(&RecordTime,sizeof(DateTimeBCD),1,fp);
//			fprintf(stderr,"RecoTime=%04d-%02d-%02d %02d:%02d:%02d\n",RecordTime.year.data,RecordTime.month.data,RecordTime.day.data,RecordTime.hour.data,RecordTime.min.data,RecordTime.sec.data);
//			fprintf(stderr,"findRead=%04d-%02d-%02d %02d:%02d:%02d\n",datetime.year.data,datetime.month.data,datetime.day.data,datetime.hour.data,datetime.min.data,datetime.sec.data);
//			if(memcmp(&RecordTime,&datetime,sizeof(DateTimeBCD))==0) {	//结构体对齐导致有字节不等，不能用内存拷贝
			if(RecordTime.year.data==datetime.year.data && RecordTime.month.data==datetime.month.data && RecordTime.day.data==datetime.day.data
					&& RecordTime.hour.data==datetime.hour.data && RecordTime.min.data==datetime.min.data && RecordTime.sec.data==datetime.sec.data) {
				fread(datalen,2,1,fp);
				if(*datalen > VARI_LEN) {
					ret = 0;
					syslog(LOG_ERR,"读取数据长度%d 大于限值 %d,失败！！！\n",*datalen,VARI_LEN);
				}else {
					ret = fread(data,(*datalen),1,fp);
				}
//				fprintf(stderr,"datalen=%d\n",*datalen);
//				for(i=0;i<*datalen;i++) {
//					fprintf(stderr,"%02x ",data[i]);
//				}
				break;
			}
			num++;
			offset = blklen*num + 4;
//			fprintf(stderr,"offset=%ld\n",offset);
			if(offset >= filesize) {	//跳转大于文件长度，返回
				fprintf(stderr,"文件尾，未找到记录 offset=%ld,filesize=%ld",offset,filesize);
				break;
			}
			ret = fseek(fp,offset,SEEK_SET);	//文件头跳转
		}
		fclose(fp);
	}
	CloseSem(sem_save);
	return ret;
}

/*
 * 按照冻结关联属性，进行数据存储
 * flag 	0：日冻结，1：月冻结
 * oi，attr		存储的OI及属性
 * savets		存储时标
 * savelen		需要存储数据长度
 * data			数据内容
 * */
void Save_TJ_Freeze(INT8U flag,OI_698 oi,INT16U attr,TS savets,int savelen,INT8U *data)
{
	OAD	 oad={};
	DateTimeBCD datetime={};

	datetime.year.data=savets.Year;
	datetime.month.data=savets.Month;
	if(flag == 0) {
		datetime.day.data=savets.Day;
	}else  if(flag == 1){
		datetime.day.data=1;
	}
	datetime.hour.data=0;
	datetime.min.data=0;
	datetime.sec.data=0;

	oad.OI = oi;
	oad.attflg = (attr>>8) & 0xff;
	oad.attrindex = attr & 0xff;
	if(flag == 0){
		FreezeObject obj_5004={};
		memset(&obj_5004,0,sizeof(FreezeObject));
		readCoverClass(0x5004,0,&obj_5004,sizeof(FreezeObject),para_vari_save);
		INT8U i=0;
		for(i=0;i<obj_5004.RelateNum;i++){
			if(memcmp(&oad,&obj_5004.RelateObj[i].oad,sizeof(OAD))==0){
				int ret=saveFreezeRecord(0x5004,oad,datetime,savelen,data);
			//				  fprintf(stderr,"ret=%d oad=%04x %02x %02x  \n",ret,oad.OI,oad.attflg,oad.attrindex);
			//				  fprintf(stderr,"passu_d[%d]:%d %d %d %d %d \n",j,passu_d[j].monitorTime,passu_d[j].downLimitTime,passu_d[j].overRate,passu_d[j].passRate,passu_d[j].upLimitTime);
//			  memset(&passu_d[j],0,sizeof(PassRate_U));
			}
		}
	}else{
		FreezeObject obj_5006={};
		memset(&obj_5006,0,sizeof(FreezeObject));
		readCoverClass(0x5006,0,&obj_5006,sizeof(FreezeObject),para_vari_save);
		INT8U i=0;
		for(i=0;i<obj_5006.RelateNum;i++){
			if(memcmp(&oad,&obj_5006.RelateObj[i].oad,sizeof(OAD))==0){
				int ret=saveFreezeRecord(0x5006,oad,datetime,savelen,data);
			//				  fprintf(stderr,"ret=%d oad=%04x %02x %02x \n",ret,oad.OI,oad.attflg,oad.attrindex);
			//				  fprintf(stderr,"passu_m[%d]:%d %d %d %d %d \n",j,passu_m[j].monitorTime,passu_m[j].downLimitTime,passu_m[j].overRate,passu_m[j].passRate,passu_m[j].upLimitTime);
			//				  memset(&passu_m[j],0,sizeof(PassRate_U));
			}
		}
	}

}
////////////////////////////////////////////////////////////////////////////////
#if 0
/*
 * 根据条件查找符合用户类型的TSA的值,并返回符合条件的个数
 * tsas: 返回符合条件的TSA
 * 返回0: 未查到,  1: 查找到
 * */
int getUserType(INT8U findtype,INT8U meter_type,TSA meter_tsa,int tsa_num,INT8U **tsas)
{
	if(findtype==meter_type) {
		memcpy(*tsas+(tsa_num*sizeof(TSA)),&meter_tsa.addr,sizeof(TSA));
		return 1;
	}
	return 0;
}

/*
 * 根据条件查找符合用户类型的TSA的值,并返回符合条件的个数
 * tsas: 返回符合条件的TSA
 * 返回0: 未查到,  1: 查找到
 * */
int getUserTSA(TSA findtsa,TSA meter_tsa,int tsa_num,INT8U **tsas)
{
//	int	i=0;
	if(memcmp(&findtsa.addr[0],&meter_tsa.addr,sizeof(TSA))==0) {  //TODO:TSA下发的地址是否按照00：长度，01：TSA长度格式
		memcpy(*tsas+(tsa_num*sizeof(TSA)),&meter_tsa.addr,sizeof(TSA));
		return 1;
	}
	return 0;
}
#endif

/*
 * 根据Region区间类型,返回查找条件
 * */
void getTsaRegion(Region_Type type,int *StartNo,int *EndNo)
{
	switch(type) {
	case close_open://前闭后开
		break;
	case open_close:
		*StartNo = *StartNo+1;
		*EndNo = *EndNo+1;
		break;
	case close_close:
		*EndNo = *EndNo+1;
		break;
	case open_open:
		*StartNo = *StartNo+1;
		break;
	default:
		break;
	}
}

/*
 * 根据ms.type填充OI6001类型tsas ; 返回TS 的数量
 * 注意调用后，释放**tsas的内存
 */
int getOI6001(MY_MS ms,INT8U **tsas)
{
	int  tsa_num = 0;
	int	 record_num = 0;
	int	 tsa_len = 0;
	int	 i=0,j=0,k=0;
	CLASS_6001	 meter={};
	INT8U 	TypeStart[3],TypeEnd[3];
	int		StartNo=0,EndNo=0;

	if(ms.mstype == 0) { //无电能表
		tsa_num = 0;
		return tsa_num;
	}
	record_num = getFileRecordNum(0x6000);
	*tsas = malloc(record_num*sizeof(CLASS_6001));
//	浙江主站招测报文含有两个相同的测量点，例如，集中器共2个测量点，主站招测4个测量点，其中2个是重复的，导致申请内存空间不足（申请两个，放置了4个测量点信息）
//	当配置两个测量点地址相同情况下，招测一组用户地址，查找到两条记录会申请内存不足，所以改为申请最大的测量点数量内存空间
//	switch(ms.mstype) {
//	case 3:	//一组用户地址
//		tsa_len = (ms.ms.userAddr[0].addr[0]<<8) | ms.ms.userAddr[0].addr[1];
//		*tsas = malloc(tsa_len*sizeof(CLASS_6001));
//		break;
//	default:
//		*tsas = malloc(record_num*sizeof(CLASS_6001));
//		break;
//	}
//	fprintf(stderr," tsas  p=%p record_num=%d tsa_len=%d",*tsas,record_num,tsa_len);
	tsa_num = 0;
	for(i=0;i<record_num;i++) {
		if(readParaClass(0x6000,&meter,i)==1) {
			if(meter.sernum!=0 && meter.sernum!=0xffff) {
				switch(ms.mstype) {
				case 1:	//全部用户地址
					fprintf(stderr,"\nTSA: %d-",meter.basicinfo.addr.addr[0]);
					for(j=0;j<meter.basicinfo.addr.addr[0];j++) {
						fprintf(stderr,"-%02x",meter.basicinfo.addr.addr[j+1]);
					}
					memcpy(*tsas+(tsa_num*sizeof(CLASS_6001)),&meter,sizeof(CLASS_6001));
					tsa_num++;
					break;
				case 2:	//一组用户类型
					tsa_len = (ms.ms.userType[0]<<8) | ms.ms.userType[1];
					fprintf(stderr,"\n一组用户类型(%d)",tsa_len);
					for(j=0;j<tsa_len;j++) {
						if(ms.ms.userType[j+2]==meter.basicinfo.usrtype) {
							memcpy(*tsas+(tsa_num*sizeof(CLASS_6001)),&meter,sizeof(CLASS_6001));
							tsa_num++;
						}
//						tsa_num += getUserType(ms.ms.userType[j+2],meter.basicinfo.usrtype,meter.basicinfo.addr,tsa_num,tsas);
					}
					break;
				case 3:	//一组用户地址
					tsa_len = (ms.ms.userAddr[0].addr[0]<<8) | ms.ms.userAddr[0].addr[1];
					fprintf(stderr,"\n一组用户地址(%d)\n\n",tsa_len);
					for(j=0;j<tsa_len;j++) {
						if(memcmp(&ms.ms.userAddr[j+1],&meter.basicinfo.addr,sizeof(TSA))==0) {  //TODO:TSA下发的地址是否按照00：长度，01：TSA长度格式
							memcpy(*tsas+(tsa_num*sizeof(CLASS_6001)),&meter,sizeof(CLASS_6001));
							tsa_num++;
						}
//						tsa_num += getUserTSA(ms.ms.userAddr[j+1],meter.basicinfo.addr,tsa_num,tsas);
					}
					fprintf(stderr,"\nms.mstype = %d,tsa_num = %d",ms.mstype,tsa_num);
					break;
				case 4:	//一组配置序号
					fprintf(stderr,"\n招测序号集(%d)",ms.ms.configSerial[0]);
					for(j=0;j<ms.ms.configSerial[0];j++) {
						fprintf(stderr," %d",ms.ms.configSerial[j+1]);
						if(meter.sernum == ms.ms.configSerial[j+1]) {
							memcpy(*tsas+(tsa_num*sizeof(CLASS_6001)),&meter,sizeof(CLASS_6001));
							tsa_num++;
							break;
						}
					}
					break;
				case 5://一组用户类型区间  无报文,暂时未实现
					for(j=0;j<COLLCLASS_MAXNUM;j++) {
						if(ms.ms.type[j].type!=interface) {	//有效类型

						}
					}
					break;
				case 6://一组用户地址区间
					for(j=0;j<COLLCLASS_MAXNUM;j++) {
						if(ms.ms.addr[j].type!=interface) {	//有效类型
							getTsaRegion(ms.ms.serial[j].type,&StartNo,&EndNo);
							fprintf(stderr,"Start-serial=%d  End-serial=%d\n",StartNo,EndNo);
							for(k=StartNo;k<EndNo;k++) {
								if(memcmp(&ms.ms.addr[j].begin[1],&meter.basicinfo.addr,sizeof(TSA))==0) {  //TODO:TSA下发的地址是否按照00：长度，01：TSA长度格式
									memcpy(*tsas+(tsa_num*sizeof(CLASS_6001)),&meter,sizeof(CLASS_6001));
									tsa_num++;
								}
							}
						}
					}
					break;
				case 7://一组配置序号区间
					for(j=0;j<COLLCLASS_MAXNUM;j++) {
						if(ms.ms.serial[j].type!=interface) {	//有效类型
							fill_Data(ms.ms.serial[j].begin[0],(INT8U *)TypeStart,&ms.ms.serial[j].begin[1]);
							fill_Data(ms.ms.serial[j].end[0],(INT8U *)TypeEnd,&ms.ms.serial[j].end[1]);
							StartNo = (TypeStart[1]<<8) | TypeStart[2];
							EndNo = (TypeEnd[1]<<8) | TypeEnd[2];
							getTsaRegion(ms.ms.serial[j].type,&StartNo,&EndNo);
							fprintf(stderr,"Start-serial=%d  End-serial=%d\n",StartNo,EndNo);
							for(k=StartNo;k<EndNo;k++) {
								if(meter.sernum == k) {
									memcpy(*tsas+(tsa_num*sizeof(CLASS_6001)),&meter,sizeof(CLASS_6001));
									tsa_num++;
									break;
								}
							}
						}
					}
					break;
				}
			}
		}
	}
	fprintf(stderr,"\nms.mstype = %d,tsa_num = %d",ms.mstype,tsa_num);
	return tsa_num;
}

/*
 * 根据ms.type填充tsas ; 返回TS 的数量
 * 注意调用后，释放**tsas的内存
 */
int getTsas(MY_MS ms,INT8U **tsas)
{
	int  tsa_num = 0;
	int	 record_num = 0;
	int	 tsa_len = 0;
	int	 i=0,j=0,k=0;
	CLASS_6001	 meter={};
	INT8U 	TypeStart[3],TypeEnd[3];
	int		StartNo=0,EndNo=0;

	if(ms.mstype == 0) { //无电能表
		tsa_num = 0;
		return tsa_num;
	}
	record_num = getFileRecordNum(0x6000);
	switch(ms.mstype) {
	case 3:	//一组用户地址
		tsa_len = (ms.ms.userAddr[0].addr[0]<<8) | ms.ms.userAddr[0].addr[1];
		*tsas = malloc(tsa_len*sizeof(TSA));
		break;
	default:
		*tsas = malloc(record_num*sizeof(TSA));
		break;
	}
	fprintf(stderr," tsas  p=%p record_num=%d  tsa_len=%d",*tsas,record_num,tsa_len);
	tsa_num = 0;
	for(i=0;i<record_num;i++) {
		if(readParaClass(0x6000,&meter,i)==1) {
			if(meter.sernum!=0 && meter.sernum!=0xffff) {
				switch(ms.mstype) {
				case 1:	//全部用户地址
					fprintf(stderr,"\nTSA: %d-",meter.basicinfo.addr.addr[0]);
					for(j=0;j<meter.basicinfo.addr.addr[0];j++) {
						fprintf(stderr,"-%02x",meter.basicinfo.addr.addr[j+1]);
					}
					memcpy(*tsas+(tsa_num*sizeof(TSA)),&meter.basicinfo.addr,sizeof(TSA));
					tsa_num++;
					break;
				case 2:	//一组用户类型
					tsa_len = (ms.ms.userType[0]<<8) | ms.ms.userType[1];
					fprintf(stderr,"\n一组用户类型(%d)",tsa_len);
					for(j=0;j<tsa_len;j++) {
						if(ms.ms.userType[j+2]==meter.basicinfo.usrtype) {
							memcpy(*tsas+(tsa_num*sizeof(TSA)),&meter.basicinfo.addr,sizeof(TSA));
							tsa_num++;
						}
//						tsa_num += getUserType(ms.ms.userType[j+2],meter.basicinfo.usrtype,meter.basicinfo.addr,tsa_num,tsas);
					}
					break;
				case 3:	//一组用户地址
					tsa_len = (ms.ms.userAddr[0].addr[0]<<8) | ms.ms.userAddr[0].addr[1];
//					fprintf(stderr,"\n一组用户地址(%d)\n\n",tsa_len);
					for(j=0;j<tsa_len;j++) {
						if(memcmp(&ms.ms.userAddr[j+1],&meter.basicinfo.addr,sizeof(TSA))==0) {  //TODO:TSA下发的地址是否按照00：长度，01：TSA长度格式
							memcpy(*tsas+(tsa_num*sizeof(TSA)),&meter.basicinfo.addr,sizeof(TSA));
							tsa_num++;
						}
//						tsa_num += getUserTSA(ms.ms.userAddr[j+1],meter.basicinfo.addr,tsa_num,tsas);
					}
					break;
				case 4:	//一组配置序号
					fprintf(stderr,"\n招测序号集(%d)",ms.ms.configSerial[0]);
					for(j=0;j<ms.ms.configSerial[0];j++) {
						fprintf(stderr," %d",ms.ms.configSerial[j+1]);
						if(meter.sernum == ms.ms.configSerial[j+1]) {
							memcpy(*tsas+(tsa_num*sizeof(TSA)),&meter.basicinfo.addr,sizeof(TSA));
							tsa_num++;
							break;
						}
					}
					break;
				case 5://一组用户类型区间  无报文,暂时未实现
					for(j=0;j<COLLCLASS_MAXNUM;j++) {
						if(ms.ms.type[j].type!=interface) {	//有效类型

						}
					}
					break;
				case 6://一组用户地址区间
					for(j=0;j<COLLCLASS_MAXNUM;j++) {
						if(ms.ms.addr[j].type!=interface) {	//有效类型
							getTsaRegion(ms.ms.serial[j].type,&StartNo,&EndNo);
							fprintf(stderr,"Start-serial=%d  End-serial=%d\n",StartNo,EndNo);
							for(k=StartNo;k<EndNo;k++) {
								if(memcmp(&ms.ms.addr[j].begin[1],&meter.basicinfo.addr,sizeof(TSA))==0) {  //TODO:TSA下发的地址是否按照00：长度，01：TSA长度格式
									memcpy(*tsas+(tsa_num*sizeof(TSA)),&meter.basicinfo.addr,sizeof(TSA));
									tsa_num++;
								}
							}
						}
					}
					break;
				case 7://一组配置序号区间
					for(j=0;j<COLLCLASS_MAXNUM;j++) {
						if(ms.ms.serial[j].type!=interface) {	//有效类型
							fill_Data(ms.ms.serial[j].begin[0],(INT8U *)TypeStart,&ms.ms.serial[j].begin[1]);
							fill_Data(ms.ms.serial[j].end[0],(INT8U *)TypeEnd,&ms.ms.serial[j].end[1]);
							StartNo = (TypeStart[1]<<8) | TypeStart[2];
							EndNo = (TypeEnd[1]<<8) | TypeEnd[2];
							getTsaRegion(ms.ms.serial[j].type,&StartNo,&EndNo);
							fprintf(stderr,"Start-serial=%d  End-serial=%d\n",StartNo,EndNo);
							for(k=StartNo;k<EndNo;k++) {
								if(meter.sernum == k) {
									memcpy(*tsas+(tsa_num*sizeof(TSA)),&meter.basicinfo.addr,sizeof(TSA));
									tsa_num++;
									break;
								}
							}
						}
					}
					break;
				}
			}
		}
	}
	fprintf(stderr,"\nms.mstype = %d,tsa_num = %d",ms.mstype,tsa_num);
	return tsa_num;
}

/*
 * rate表示费率
 */
INT16U GetOIinfo(OI_698 oi,INT8U rate,OI_INFO *oi_info)//得到oi的信息
{
	memset(oi_info,0x00,sizeof(oi_info));

	oi_info->oinum = 1;//默认1
	oi_info->mem_num = 1;//默认1 不是默认值修改
	oi_info->io_unit = 0;//只有array和struct才赋值
	fprintf(stderr,"\noi=%04x\n",oi);
	fprintf(stderr,"\n-----case %d\n",((oi&0xf000)>>12));
	switch((oi&0xf000)>>12)//穷举
	{
	case 0:
		oi_info->ic = 1;
		oi_info->oinum = rate+1;//总加rate个费率
		oi_info->io_unit = 1;
		oi_info->oi_mem[0].mem_len = 4;
		oi_info->oi_mem[0].mem_chg = 12;//-2
		switch(oi)
		{
		case 0x0000:
		case 0x0030:
		case 0x0031:
		case 0x0032:
		case 0x0033:
		case 0x0040:
		case 0x0041:
		case 0x0042:
		case 0x0043:
		case 0x0500:
		case 0x0501:
		case 0x0502:
		case 0x0503:
			oi_info->oi_mem[0].mem_unit = 5;
		break;
		default:
			oi_info->oi_mem[0].mem_unit = 6;
			break;
		}
		break;
	case 1:
		oi_info->ic = 2;
		oi_info->oinum = rate+1;//总加rate个费率
		oi_info->io_unit = 1;
		oi_info->oi_mem[0].mem_len = 4;
		oi_info->oi_mem[0].mem_chg = 14;//-4
		switch(oi)
		{
		case 0x1030:
		case 0x1031:
		case 0x1032:
		case 0x1033:
		case 0x1040:
		case 0x1041:
		case 0x1042:
		case 0x1043:
		case 0x1050:
		case 0x1051:
		case 0x1052:
		case 0x1053:
		case 0x1130:
		case 0x1131:
		case 0x1132:
		case 0x1133:
		case 0x1140:
		case 0x1141:
		case 0x1142:
		case 0x1143:
			oi_info->oi_mem[0].mem_unit = 5;
		break;
		default:
			oi_info->oi_mem[0].mem_unit = 6;
			break;
		}
		break;
	case 2:
		switch(oi)
		{
		///////////////////////////////////////////////////////////ic = 2
		case 0x2140:
		case 0x2141:
			oi_info->ic = 2;
			oi_info->oinum = rate+1;//总加rate个费率
			oi_info->io_unit = 2;//struct
			oi_info->mem_num = 2;

			oi_info->oi_mem[0].mem_unit = 6;
			oi_info->oi_mem[0].mem_len = 4;
			oi_info->oi_mem[0].mem_chg = 14;

			oi_info->oi_mem[1].mem_unit = 30;
			oi_info->oi_mem[1].mem_len = 6;
			oi_info->oi_mem[1].mem_chg = 0;
			break;
			////////////////////////////////////////////////////////ic = 3
		case 0x2001:
			oi_info->ic = 3;
			oi_info->oinum = 4;//三相加零序
			oi_info->oi_mem[0].mem_chg = 13;//-3
			oi_info->oi_mem[0].mem_len = 4;
			oi_info->oi_mem[0].mem_unit = 5;
			break;
		case 0x2000:
		case 0x2002:
		case 0x2003:
			oi_info->ic = 3;
			oi_info->oinum = 3;//三相
			oi_info->oi_mem[0].mem_chg = 11;//-1
			oi_info->oi_mem[0].mem_len = 2;
			oi_info->oi_mem[0].mem_unit = 18;
			break;
		case 0x200b:
		case 0x200c:
			oi_info->ic = 3;
			oi_info->oinum = 3;//三相
			oi_info->oi_mem[0].mem_chg = 12;//-2
			oi_info->oi_mem[0].mem_len = 2;
			oi_info->oi_mem[0].mem_unit = 16;
			break;
			////////////////////////////////////////////////////////ic = 4
		case 0x2004:
		case 0x2005:
		case 0x2006:
		case 0x2007:
		case 0x2008:
		case 0x2009:
			oi_info->ic = 4;
			oi_info->oinum = 4;//总及分相
			oi_info->oi_mem[0].mem_chg = 11;//-1
			oi_info->oi_mem[0].mem_len = 4;
			oi_info->oi_mem[0].mem_unit = 5;
			break;
		case 0x200a:
			oi_info->ic = 4;
			oi_info->oinum = 4;//总及分相
			oi_info->oi_mem[0].mem_chg = 13;//-3
			oi_info->oi_mem[0].mem_len = 2;
			oi_info->oi_mem[0].mem_unit = 16;
			break;
			////////////////////////////////////////////////////////ic = 5 谐波
		case 0x200d:
		case 0x200e:
			//不知到怎么处理 todo
			break;
			////////////////////////////////////////////////////////ic = 6
		case 0x200f:
		case 0x2011:
		case 0x2012:
		case 0x2026:
		case 0x2027:
		case 0x2028:
			oi_info->ic = 6;
			oi_info->oinum = 1;
			oi_info->oi_mem[0].mem_chg = 12;//-2
			oi_info->oi_mem[0].mem_len = 2;
			oi_info->oi_mem[0].mem_unit = 18;
			break;
		case 0x2010:
			oi_info->ic = 6;
			oi_info->oinum = 1;
			oi_info->oi_mem[0].mem_chg = 11;//-1
			oi_info->oi_mem[0].mem_len = 2;
			oi_info->oi_mem[0].mem_unit = 16;
			break;
		case 0x2013:
		case 0x2504:
			oi_info->ic = 6;
			oi_info->oinum = 1;
			oi_info->oi_mem[0].mem_chg = 0;//
			oi_info->oi_mem[0].mem_len = 4;
			oi_info->oi_mem[0].mem_unit = 6;
			break;
		case 0x2014:
			oi_info->ic = 6;
			oi_info->oinum = 7;
			oi_info->io_unit = 1;//array
			oi_info->mem_num = 2;
			oi_info->oi_mem[0].mem_chg = 0;//
			oi_info->oi_mem[0].mem_len = 1;
			oi_info->oi_mem[0].mem_unit = 9;
			break;
		case 0x2015:
			oi_info->ic = 6;
			oi_info->oinum = 1;
			oi_info->oi_mem[0].mem_chg = 0;//
			oi_info->oi_mem[0].mem_len = 64;
			oi_info->oi_mem[0].mem_unit = 4;
			break;
		case 0x2016:
		case 0x2040:
		case 0x2041:
			oi_info->ic = 6;
			oi_info->oinum = 1;
			oi_info->oi_mem[0].mem_chg = 0;//
			oi_info->oi_mem[0].mem_len = 16;
			oi_info->oi_mem[0].mem_unit = 4;
			break;
		case 0x2017:
		case 0x2018:
		case 0x2019:
			oi_info->ic = 6;
			oi_info->oinum = 1;
			oi_info->oi_mem[0].mem_chg = 14;//-4
			oi_info->oi_mem[0].mem_len = 4;
			oi_info->oi_mem[0].mem_unit = 5;
			break;
		case 0x201a:
		case 0x201b:
		case 0x201c:
		case 0x2500:
		case 0x2501:
			oi_info->ic = 6;
			oi_info->oinum = 1;
			oi_info->oi_mem[0].mem_chg = 14;//-4
			oi_info->oi_mem[0].mem_len = 4;
			oi_info->oi_mem[0].mem_unit = 6;
			break;
		case 0x201d:
			oi_info->ic = 6;
			oi_info->oinum = 1;
			oi_info->oi_mem[0].mem_chg = 0;//
			oi_info->oi_mem[0].mem_len = 2;
			oi_info->oi_mem[0].mem_unit = 18;
			break;
		case 0x2029:
			oi_info->ic = 6;
			oi_info->oinum = 4;//总及三相s
			oi_info->oi_mem[0].mem_chg = 12;//-2
			oi_info->oi_mem[0].mem_len = 4;
			oi_info->oi_mem[0].mem_unit = 5;
			break;
		case 0x202d:
		case 0x202e:
		case 0x2031:
		case 0x2032:
		case 0x2502:
		case 0x2503:
			oi_info->ic = 6;
			oi_info->oinum = 1;
			oi_info->oi_mem[0].mem_chg = 12;//-2
			oi_info->oi_mem[0].mem_len = 4;
			oi_info->oi_mem[0].mem_unit = 6;
			break;
		case 0x2130:
		case 0x2131:
		case 0x2132:
		case 0x2133:
			oi_info->ic = 6;
			oi_info->oinum = 1;//
			oi_info->io_unit = 2;//struct
			oi_info->mem_num = 6;

			oi_info->oi_mem[0].mem_unit = 6;
			oi_info->oi_mem[0].mem_len = 4;
			oi_info->oi_mem[0].mem_chg = 0;

			oi_info->oi_mem[1].mem_unit = 6;
			oi_info->oi_mem[1].mem_len = 4;
			oi_info->oi_mem[1].mem_chg = 0;

			oi_info->oi_mem[2].mem_unit = 18;
			oi_info->oi_mem[2].mem_len = 2;
			oi_info->oi_mem[2].mem_chg = 12;

			oi_info->oi_mem[3].mem_unit = 18;
			oi_info->oi_mem[3].mem_len = 2;
			oi_info->oi_mem[3].mem_chg = 12;

			oi_info->oi_mem[4].mem_unit = 6;
			oi_info->oi_mem[4].mem_len = 4;
			oi_info->oi_mem[4].mem_chg = 0;

			oi_info->oi_mem[5].mem_unit = 6;
			oi_info->oi_mem[5].mem_len = 4;
			oi_info->oi_mem[5].mem_chg = 0;
			break;
		case 0x2200:
		case 0x2203:
			oi_info->ic = 6;
			oi_info->oinum = 1;//
			oi_info->io_unit = 2;//struct
			oi_info->mem_num = 2;

			oi_info->oi_mem[0].mem_unit = 6;
			oi_info->oi_mem[0].mem_len = 4;
			oi_info->oi_mem[0].mem_chg = 0;

			oi_info->oi_mem[1].mem_unit = 6;
			oi_info->oi_mem[1].mem_len = 4;
			oi_info->oi_mem[1].mem_chg = 0;
			break;
		case 0x2204:
			oi_info->ic = 6;
			oi_info->oinum = 1;//
			oi_info->io_unit = 2;//struct
			oi_info->mem_num = 2;

			oi_info->oi_mem[0].mem_unit = 18;
			oi_info->oi_mem[0].mem_len = 2;
			oi_info->oi_mem[0].mem_chg = 0;

			oi_info->oi_mem[1].mem_unit = 18;
			oi_info->oi_mem[1].mem_len = 2;
			oi_info->oi_mem[1].mem_chg = 0;
			break;
		case 0x2505:
			oi_info->ic = 6;
			oi_info->oinum = 1;//
			oi_info->io_unit = 2;//struct
			oi_info->mem_num = 2;

			oi_info->oi_mem[0].mem_unit = 6;
			oi_info->oi_mem[0].mem_len = 4;
			oi_info->oi_mem[0].mem_chg = 12;

			oi_info->oi_mem[1].mem_unit = 6;
			oi_info->oi_mem[1].mem_len = 4;
			oi_info->oi_mem[1].mem_chg = 12;
			break;
		case 0x2506:
			oi_info->ic = 6;
			oi_info->oinum = 1;//
			oi_info->io_unit = 2;//struct
			oi_info->mem_num = 2;

			oi_info->oi_mem[0].mem_unit = 22;
			oi_info->oi_mem[0].mem_len = 1;
			oi_info->oi_mem[0].mem_chg = 0;

			oi_info->oi_mem[1].mem_unit = 22;
			oi_info->oi_mem[1].mem_len = 1;
			oi_info->oi_mem[1].mem_chg = 0;
			break;
			////////////////////////////////////////////////////////ic = 7 null
			////////////////////////////////////////////////////////ic = 8
		case 0x202a:
			oi_info->ic = 8;
			oi_info->oinum = 1;
			oi_info->oi_mem[0].mem_chg = 0;//
			oi_info->oi_mem[0].mem_len = 17;
			oi_info->oi_mem[0].mem_unit = 85;//tsa
			break;
		case 0x201e:
		case 0x2020:
		case 0x2021:
			oi_info->ic = 8;
			oi_info->oinum = 1;
			oi_info->oi_mem[0].mem_chg = 0;//
			oi_info->oi_mem[0].mem_len = 7;
			oi_info->oi_mem[0].mem_unit = 28;
			break;
		case 0x2022:
		case 0x2023:
			oi_info->ic = 8;
			oi_info->oinum = 1;
			oi_info->oi_mem[0].mem_chg = 0;//
			oi_info->oi_mem[0].mem_len = 4;
			oi_info->oi_mem[0].mem_unit = 6;
			break;
		case 0x2025:
			oi_info->ic = 8;
			oi_info->oinum = 1;//
			oi_info->io_unit = 2;//struct
			oi_info->mem_num = 2;

			oi_info->oi_mem[0].mem_unit = 6;
			oi_info->oi_mem[0].mem_len = 4;
			oi_info->oi_mem[0].mem_chg = 0;

			oi_info->oi_mem[1].mem_unit = 6;
			oi_info->oi_mem[1].mem_len = 4;
			oi_info->oi_mem[1].mem_chg = 0;
			break;
		case 0x202c:
			oi_info->ic = 8;
			oi_info->oinum = 1;//
			oi_info->io_unit = 2;//struct
			oi_info->mem_num = 2;

			oi_info->oi_mem[0].mem_unit = 6;
			oi_info->oi_mem[0].mem_len = 4;
			oi_info->oi_mem[0].mem_chg = 12;

			oi_info->oi_mem[1].mem_unit = 6;
			oi_info->oi_mem[1].mem_len = 4;
			oi_info->oi_mem[1].mem_chg = 0;
			break;
		}
		break;
	case 3://事件
		break;
	case 4:
		switch(oi)
		{
		case 0x4000:
			oi_info->ic = 8;
			oi_info->oinum = 1;
			oi_info->oi_mem[0].mem_chg = 0;//
			oi_info->oi_mem[0].mem_len = 7;
			oi_info->oi_mem[0].mem_unit = 28;
			break;
		}
		break;
	case 5:
		break;
	case 6:
		switch(oi)
		{
		case 0x6040:
		case 0x6041:
		case 0x6042:
			oi_info->ic = 8;
			oi_info->oinum = 1;
			oi_info->oi_mem[0].mem_chg = 0;//
			oi_info->oi_mem[0].mem_len = 7;
			oi_info->oi_mem[0].mem_unit = 28;
			break;
		}
		break;
	default:break;
	}


	return 1;
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
//		fprintf(stderr,"创建文件--%s\n",FileName);
	}else {
		fp = fopen((char*) FileName, "r+");
//		fprintf(stderr,"替换文件\n");
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
int buf_int(INT8U  *buf)
{
	int value=0;
	value = buf[0];
	value = (value<<8) + buf[1];
	return value;
}
int buf_int2(INT8U  *buf)
{
	int value=0;
	value = buf[1];
	value = (value<<8) + buf[0];
	return value;
}

int readfile_int(FILE *fp)
{
	INT8U buf[2]={};
	int value=0;
	if (fp!=NULL)
	{
		if(fread(buf,2,1,fp)>0)
		{
			//value = buf[0];
			//value = (value<<8) + buf[1];
			value = buf_int(buf);
		}
		close(fp);
	}
	return value;
}

int getOADf(INT8U type,INT8U *source,OAD *oad)		//0x51
{
	if((type == 1) || (type == 0)) {
		oad->OI = source[type+1];
		oad->OI = (oad->OI <<8) | source[type];
		oad->attflg = source[type+2];
		oad->attrindex = source[type+3];
		return (4+type);
	}
	return 0;
}
int head_prt(int unitnum,HEAD_UNIT0 *length,int *indexn,FILE *fp)
{
	INT8U buf[50]={};
	int A_record=0,i=0,j=0;
	OAD oad;

	for(i=0;i<unitnum  ;i++)
	{
		memset(buf,0,50);
		fread(buf,10,1,fp);
		getOADf(0,&buf[0],&oad);
		memcpy(&length[i].oad_m,&oad,sizeof(oad));
		fprintf(stderr,"\n【%02d】  %04x-%02x-%02x   ",i,oad.OI,oad.attflg,oad.attrindex);
		getOADf(0,&buf[4],&oad);
		memcpy(&length[i].oad_r,&oad,sizeof(oad));
		fprintf(stderr,  "%04x-%02x-%02x   ",oad.OI,oad.attflg,oad.attrindex);
		length[i].len = buf_int2(&buf[8]);
		fprintf(stderr," %02d 字节        |   ",length[i].len);
		(*indexn)++;
		for(j=0;j<10;j++)
			fprintf(stderr,"%02x ",buf[j]);
		if (i==3)
			fprintf(stderr,"\n");
		A_record += length[i].len;
	}
	return A_record ;
}
void getTaskFileName(INT8U taskid,TS ts,char *fname)
{
	char dirname[FILENAMELEN]={};
	if (fname==NULL)
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
//	asyslog(LOG_INFO,"-------oadmr_num=%d,unitnum=%d\n",item_road.oadmr_num,unitnum);
	for(i=0;i<item_road.oadmr_num;i++)//找不到呢
	{
		datapos=0;
		for(j=0;j<unitnum;j++)
		{
//			fprintf(stderr,"j=%d,len=%d,datapos=%d\n",j,head_unit[j].len,datapos);
			memcpy(&oad_offset[i].oad_m,&item_road.oad[i].oad_m,sizeof(OAD));
			memcpy(&oad_offset[i].oad_r,&item_road.oad[i].oad_r,sizeof(OAD));
//			if(memcmp(&item_road.oad[i].oad_m,&head_unit[j].oad_m,sizeof(OAD))==0 &&
//					memcmp(&item_road.oad[i].oad_r,&head_unit[j].oad_r,sizeof(OAD))==0)
			if(memcmp(&item_road.oad[i].oad_m,&head_unit[j].oad_m,sizeof(OAD))==0 &&
					item_road.oad[i].oad_r.OI == head_unit[j].oad_r.OI)
			{
//				fprintf(stderr,"\nfind oad %04x%02x%02x-%04x%02x%02x:offset:%d\n",
//						item_road.oad[i].oad_m.OI,item_road.oad[i].oad_m.attflg,item_road.oad[i].oad_m.attrindex,
//						item_road.oad[i].oad_r.OI,item_road.oad[i].oad_r.attflg,item_road.oad[i].oad_r.attrindex,
//						datapos);
				if(item_road.oad[i].oad_r.attrindex != 0 && head_unit[j].oad_r.attrindex == 0)//招测某一项
				{
					INT16U oadlen = CalcOIDataLen(item_road.oad[i].oad_r);
					oad_offset[i].offset = datapos + (item_road.oad[i].oad_r.attrindex-1)*oadlen +2;
					oad_offset[i].len = oadlen;
				}
				else if(item_road.oad[i].oad_r.attrindex == head_unit[j].oad_r.attrindex)
				{
					oad_offset[i].offset = datapos;
					oad_offset[i].len = head_unit[j].len;
				}
				else
					datapos += head_unit[j].len;
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
INT16U CalcFreq(TI runti,CLASS_6015 class6015,INT16U startmin,INT16U endmin,INT32U *sec_freq)//不管开闭
{
	int rate = 0;//倍率
	INT16U sec_unit = 0;
	INT8U  inval_flg = 0;
	asyslog(LOG_INFO,"\n---@@@---class6015.cjtype = %d  data=%d-%d-%d,endmin=%d,startmin=%d\n"
			,class6015.cjtype,class6015.data.data[0],class6015.data.data[1],class6015.data.data[2],endmin,startmin);
	if(class6015.cjtype == 3 || class6015.cjtype == 0 || class6015.cjtype == 1)//按时标间隔采集
	{
		if(class6015.cjtype == 3)//按抄表间隔
		{
			switch(class6015.data.data[0])
			{
			case 0:
				*sec_freq = ((class6015.data.data[1]<<8)+class6015.data.data[2]);
				return ((endmin-startmin)*60)/((class6015.data.data[1]<<8)+class6015.data.data[2])+1;
			case 1:
				asyslog(LOG_INFO,"\n---@@@---按抄表间隔采集,间隔%d\n",((endmin-startmin)*60)/((class6015.data.data[1]<<8)+class6015.data.data[2])*60+1);
				*sec_freq = ((class6015.data.data[1]<<8)+class6015.data.data[2])*60;
				return (endmin-startmin)/((class6015.data.data[1]<<8)+class6015.data.data[2])+1;
			case 2:
				*sec_freq = ((class6015.data.data[1]<<8)+class6015.data.data[2])*3600;
				return ((endmin-startmin)*60)/(((class6015.data.data[1]<<8)+class6015.data.data[2])*3600)+1;
			default:
				break;
			}
			asyslog(LOG_INFO,"\n---@@@---按抄表间隔采集1\n");
			return 1;
		}
		fprintf(stderr,"\n结束分钟数：%d 开始分钟数：%d 单位 %d\n",endmin, startmin, runti.units);
		if(endmin <= startmin || runti.units > 3)
			return 1;//无效设置
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
		case 3://天
			asyslog(LOG_INFO,"\n---@@@---采集类型%d  data:%02x-%02x%02x\n",class6015.cjtype,class6015.data.data[0],class6015.data.data[1],class6015.data.data[2]);
			if(class6015.data.data[1] == 0 && class6015.data.data[2] == 0)
				return 1;
			*sec_freq = 86400;
			return 1;
			break;
		default:
			break;//没有这种情况
		}
		if(inval_flg == 1)//todo
			return 0;
		sec_unit = (runti.interval * rate);
		fprintf(stderr,"\nsec_unit = %d,interval=%d(%d)\n",sec_unit,runti.interval,runti.units);
		if(sec_unit==0)	  sec_unit = 1;		//TODO:该如何赋值
		*sec_freq = sec_unit;
		fprintf(stderr,"\n---@@@-开始分钟数：%d 结束分钟数：%d 间隔秒数%d 次数:%d---%d\n",startmin,endmin,sec_unit,((endmin-startmin)*60)/sec_unit,((endmin-startmin)*60)/sec_unit+1);
		return ((endmin-startmin)*60)/sec_unit+1;
	}
	return 1;
}
#if 0
INT32U freqtosec(TI interval)//ti格式频率转化为秒数,只计算秒分时日，其他返回0
{
	INT32U rate = 0;
	switch(interval.units)
	{
	case 0:
		rate = 1;
		break;
	case 1:
		rate = 60;
		break;
	case 2:
		rate = 3600;
		break;
	case 3:
		rate = 86400;
		break;
	default:
		break;
	}
	return rate*interval.interval;
}
#endif
//读取taskid相应的配置结构体，return 1成功，0失败
INT8U ReadTaskInfo(INT8U taskid,TASKSET_INFO *tasknor_info)//读取普通采集方案配置
{
	int i=0;
	CLASS_6015	class6015={};
	CLASS_6013	class6013={};
	memset(tasknor_info,0x00,sizeof(TASKSET_INFO));
	memset(&class6013,0,sizeof(CLASS_6013));
	memset(&class6015,0,sizeof(CLASS_6015));
	if(readCoverClass(0x6013,taskid,&class6013,sizeof(class6013),coll_para_save) == 1)
	{
#ifdef SYS_INFO
		asyslog(LOG_INFO,"GetTaskData: class6013.cjtype =%d  class6013.state =%d\n",class6013.cjtype,class6013.state);
#endif
		if(class6013.cjtype != 1 || class6013.state != 1)//
			return 0;
		if(readCoverClass(0x6015,class6013.sernum,&class6015,sizeof(CLASS_6015),coll_para_save) == 1)
		{
			tasknor_info->save_timetype = class6015.savetimeflag;
#ifdef SYS_INFO
			asyslog(LOG_INFO,"\n---class6015.cjtype = %d  class6013.sernum = %d \n",class6015.cjtype,class6013.sernum);
#endif
			tasknor_info->taskfreq = TItoSec(class6013.interval);	//替换原来的freqtosec
			fprintf(stderr,"\n任务执行间隔%d\n",tasknor_info->taskfreq);
			tasknor_info->starthour = class6013.runtime.runtime[0].beginHour;
			tasknor_info->startmin = class6013.runtime.runtime[0].beginMin;//按照设置一个时段来
			tasknor_info->endhour = class6013.runtime.runtime[0].endHour;
			tasknor_info->endmin = class6013.runtime.runtime[0].endMin;//按照设置一个时段来
			fprintf(stderr,"\n任务开始结束时间：%d:%d--%d:%d\n",tasknor_info->starthour,tasknor_info->startmin,tasknor_info->endhour,tasknor_info->endmin);
			tasknor_info->runtime = CalcFreq(class6013.interval,class6015,tasknor_info->starthour*60+tasknor_info->startmin,tasknor_info->endhour*60+tasknor_info->endmin,&tasknor_info->freq);
//			tasknor_info->runtime = CalcFreq(class6013.interval,class6015,tasknor_info->starthour*60+tasknor_info->startmin,tasknor_info->endhour*60+tasknor_info->endmin,&tasknor_info->freq);
			if(tasknor_info->runtime == 0)
				return 0;
			fprintf(stderr,"\n---@@@---任务%d执行次数%d 设置的单位%d\n",taskid,tasknor_info->runtime,class6013.interval.units);
			tasknor_info->KBtype = CalcKBType(class6013.runtime.type);
			fprintf(stderr,"\n---@@@---开闭方式%d\n",tasknor_info->KBtype);
			tasknor_info->memdep = class6015.deepsize;
			fprintf(stderr,"\n---@@@---存储深度%d\n",class6015.deepsize);
			memcpy(&tasknor_info->csds,&class6015.csds,sizeof(CSD_ARRAYTYPE));
			for(i=0;i<MY_CSD_NUM;i++)
			{
				switch(tasknor_info->csds.csd[i].csd.road.oad.OI)//union
				{
				case 0x0000://
				case 0x5000:
				case 0x5002:
				case 0x5003://
					break;
				case 0x5004://日冻结
				case 0x5005://结算日
					tasknor_info->freq = 86400;
					return 1;
					break;
				case 0x5006://月冻结
					tasknor_info->freq = 86400;
					return 2;
					break;
				case 0x5007://年冻结
					tasknor_info->freq = 86400;
					return 3;
					break;
				default:break;

				}
			}
//			if(tasknor_info->runtime == 1)//日月年冻结
//			{
//				tasknor_info->freq = 86400;//
//				if(class6013.interval.units == 3)//日冻结
//					return 1;
//				if(class6013.interval.units == 4)//月冻结
//					return 2;
//				if(class6013.interval.units == 5)//年冻结
//					return 3;
//			}
			fprintf(stderr,"\n---@@@---返回4\n");
#ifdef SYS_INFO
			asyslog(LOG_INFO,"任务%d执行次数%d [%d:%d--%d:%d]\n",taskid,tasknor_info->runtime,tasknor_info->starthour,tasknor_info->startmin,tasknor_info->endhour,tasknor_info->endmin);
#endif
			return 4;
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
INT16U CalcOIDataLen(OAD oad)
{
	FILE *fp;
	char ln[60];
	char lnf[5];
	INT16U oi_len=0;
	INT8U ic_type = 1;

	if(oad.OI>=0x0000 && oad.OI<0x1000)
	{
		if(oad.attflg == 2) {
			if(oad.attrindex == 0)
				return 27;//长度4+1个字节数据类型  +2: array 01 + 长度
			else
				return 5;
		}else if(oad.attflg == 4) {
			if(oad.attrindex == 0)
				return 47;//长度4+1个字节数据类型  +2: array 01 + 长度
			else
				return 9;
		}
	}
	if(oad.OI>=0x1000 && oad.OI<0x2000)
	{
		if(oad.attrindex == 0)
			return 15*5+2;
		else
			return 15;
	}
	if(oad.OI == 0x2014)
	{
		if(oad.attrindex == 0)
			return 4*7+2;
		else
			return 4;
	}
	if(oad.OI ==0x202a)
	{
		return 18;
	}
	if(oad.OI==0x2131 || oad.OI==0x2132 || oad.OI==0x2133) {	//电压合格率
		if(oad.attrindex == 0)
			return 23*2+2;
		else
			return 23;
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

		if(strtoul(lnf,NULL,16) != oad.OI)
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
			if(oad.attrindex == 0)
				oi_len = oi_len*(MET_RATE+1)+1+1;//+类型+个数
			break;
		case 3:
			if(oad.attrindex == 0)
				oi_len = oi_len*3+1+1;//三相
			break;
		case 4:
			if(oad.attrindex == 0)
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
/*
 * 根据csds得到任务号
 * findmethod = 1: 按照csds 的下发实际类型查找
 * findmethod = 2: 对与road,只查找相关的oad.解决浙江下发的曲线任务为oad,但是主站主动招测下发road 5002-0200(200a-0201.....),
 * 				   类型不匹配,如果按照oad方式查找,无法查找到实际的任务号
 */
INT8U GetTaskidFromCSDs(CSD_ARRAYTYPE csds,ROAD_ITEM *item_road,INT8U findmethod,CLASS_6001 *tsa)
{
	CLASS_6015	class6015={};
	CLASS_6013	class6013={};
	int i=0,j=0,mm=0,nn=0,kk=0;
	INT8U taskno=0,tsa_equ=0;
	INT16U num_tmp=0;

	print_rcsd(csds);
	if(csds.num > MY_CSD_NUM)//超了
		csds.num = MY_CSD_NUM;
#ifdef SYS_INFO
	asyslog(LOG_INFO,"csds.num=%d\n",csds.num);
#endif
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


#ifdef SYS_INFO
	asyslog(LOG_INFO,"任务下发的主-从OI配置：oadmr_num=%d\n",item_road->oadmr_num);
	for(i=0;i<item_road->oadmr_num;i++){
		asyslog(LOG_INFO,"[%d] %04x_%04x\n",i,item_road->oad[i].oad_m.OI,item_road->oad[i].oad_r.OI);
	}
#endif
	memset(&class6013,0,sizeof(CLASS_6013));
	memset(&class6015,0,sizeof(CLASS_6015));
	for(i=0;i<256;i++)
	{
		tsa_equ = 0;
		if(readCoverClass(0x6013,i+1,&class6013,sizeof(class6013),coll_para_save) == 1)
		{
			if(class6013.cjtype != 1 || class6013.state != 1)//过滤掉不是普通采集方案的
				continue;
			if(readCoverClass(0x6015,class6013.sernum,&class6015,sizeof(CLASS_6015),coll_para_save) == 1)
			{
				if(tsa != NULL)
				{
					fprintf(stderr,"\n  metertype=%d\n",class6015.mst.mstype);
					switch(class6015.mst.mstype)
					{
					case 0:
						return 0;
					case 1:
						tsa_equ = 1;
						break;
					case 2:
					num_tmp = (class6015.mst.ms.userType[0]<<8) | class6015.mst.ms.userType[1];
					fprintf(stderr,"\n类型个数%d\n",num_tmp);
					for(kk=0;kk<num_tmp;kk++)
					{
						if(class6015.mst.ms.userType[kk+2] == tsa[0].basicinfo.usrtype)
							tsa_equ = 1;
					}
						break;
					case 3:
						for(kk=0;kk<COLLCLASS_MAXNUM;kk++)
						{
							if(memcmp(&class6015.mst.ms.userAddr[kk],&tsa[0].basicinfo.addr,sizeof(TSA)) == 0)
								tsa_equ = 1;
						}
						break;
					case 4:
						for(kk=0;kk<COLLCLASS_MAXNUM;kk++)
						{
							if(class6015.mst.ms.configSerial[kk] == tsa[0].sernum)
								tsa_equ = 1;
						}
						break;
					case 5:
						fprintf(stderr,"\n\n------");
						for(kk=0;kk<20;kk++)
							fprintf(stderr," %02x",class6015.mst.ms.type[0].begin[kk]);
						fprintf(stderr,"\n%02x-%02x : %02x \n",class6015.mst.ms.type[0].begin[1],class6015.mst.ms.type[0].end[1],
								tsa[0].basicinfo.usrtype);
						if(tsa[0].basicinfo.usrtype>=class6015.mst.ms.type[0].begin[1] && tsa[0].basicinfo.usrtype<=class6015.mst.ms.type[0].end[1])
							tsa_equ = 1;
						break;
					case 6:break;//暂时不实现
					case 7:break;
					default:
						break;
					}
				}
				if(tsa_equ==0)
					continue;
				else
					fprintf(stderr,"\ntsa_equ=1\n");
#ifdef SYS_INFO
				asyslog(LOG_INFO,"查找任务号 %d，方案序号：%d class6015.csds.num=%d",i+1,class6013.sernum,class6015.csds.num);
#endif
				for(j=0;j<class6015.csds.num;j++)
				{
#ifdef SYS_INFO
					asyslog(LOG_INFO,"jj=%d,csd.oad=%04x_%02x%02x \n",j,class6015.csds.csd[j].csd.oad.OI,
								class6015.csds.csd[j].csd.oad.attflg,class6015.csds.csd[j].csd.oad.attrindex);
#endif
					for(mm=0;mm<item_road->oadmr_num;mm++)
					{
						switch(class6015.csds.csd[j].type)
						{
						case 0:
#ifdef SYS_INFO
							asyslog(LOG_INFO,"mm=%d,oad_r  =%04x_%02x%02x \n",mm,item_road->oad[mm].oad_r.OI,
										item_road->oad[mm].oad_r.attflg,item_road->oad[mm].oad_r.attrindex);
#endif
							//浙江主站下发曲线任务oad,主动招测曲线数据,下发的oad为5002-*, 与任务的id不一致
//							if(findmethod == 2) {		//深度查找满足的任务oad
//						//		if((item_road->oad[mm].oad_m.OI > 0x0000) && (item_road->oad[mm].oad_m.OI < 0x5004))
//								if(item_road->oad[mm].oad_m.OI > 0x0000)
//								{
//									item_road->oad[mm].oad_m.OI = 0x0000;
//									item_road->oad[mm].oad_m.attflg = 0;
//									item_road->oad[mm].oad_m.attrindex = 0;
//									item_road->oad[mm].oad_num = 0;
//								}
//							}
							if(findmethod == 1 && (item_road->oad[mm].oad_m.OI == 0x0000)) 	//深度查找满足的任务oad
							{
								if(memcmp(&item_road->oad[mm].oad_r,&class6015.csds.csd[j].csd.oad,sizeof(OAD))==0 ||
										(item_road->oad[mm].oad_r.OI == class6015.csds.csd[j].csd.oad.OI &&
												item_road->oad[mm].oad_r.attrindex != 0 &&
												class6015.csds.csd[j].csd.oad.attrindex == 0)){
									item_road->oad[mm].taskid = i+1;
#ifdef SYS_INFO
									asyslog(LOG_INFO,"taskid find one %d",i+1);
#endif
									continue;
								}
							}else if(findmethod == 2) {
								if(memcmp(&item_road->oad[mm].oad_r,&class6015.csds.csd[j].csd.oad,sizeof(OAD))==0 ||
										(item_road->oad[mm].oad_r.OI == class6015.csds.csd[j].csd.oad.OI &&
												item_road->oad[mm].oad_r.attrindex != 0 &&
												class6015.csds.csd[j].csd.oad.attrindex == 0)){
									item_road->oad[mm].taskid = i+1;
#ifdef SYS_INFO
									asyslog(LOG_INFO,"taskid find one %d",i+1);
#endif
									continue;
								}
							}
							break;
						case 1:
#ifdef SYS_INFO
							  asyslog(LOG_INFO,"11111 mm=%d,oad_r  =%04x_%02x%02x \n",mm,item_road->oad[mm].oad_r.OI,
											item_road->oad[mm].oad_r.attflg,item_road->oad[mm].oad_r.attrindex);
							  asyslog(LOG_INFO,"11111 jj=%d,csd.oad=%04x_%02x%02x \n",j,class6015.csds.csd[j].csd.oad.OI,
											class6015.csds.csd[j].csd.oad.attflg,class6015.csds.csd[j].csd.oad.attrindex);
#endif
							if(memcmp(&item_road->oad[mm].oad_m,&class6015.csds.csd[j].csd.road.oad,sizeof(OAD))==0)//
							{
								for(nn=0;nn<class6015.csds.csd[j].csd.road.num;nn++)
								{
									fprintf(stderr,"oad_r=%04x%02x%02x %d.oad=%04x%02x%02x\n",
											item_road->oad[mm].oad_r.OI,item_road->oad[mm].oad_r.attflg,item_road->oad[mm].oad_r.attrindex,nn,
											class6015.csds.csd[j].csd.road.oads[nn].OI,class6015.csds.csd[j].csd.road.oads[nn].attflg,class6015.csds.csd[j].csd.road.oads[nn].attrindex);
									if(memcmp(&item_road->oad[mm].oad_r,&class6015.csds.csd[j].csd.road.oads[nn],sizeof(OAD))==0 ||
											(item_road->oad[mm].oad_r.OI == class6015.csds.csd[j].csd.road.oads[nn].OI &&
												item_road->oad[mm].oad_r.attrindex != 0 &&
												class6015.csds.csd[j].csd.road.oads[nn].attrindex == 0)){
										item_road->oad[mm].taskid = i+1;
										fprintf(stderr,"\n------find \n");
//										asyslog(LOG_INFO,"1111:item_road->oad[%d].taskid=%d\n",mm,item_road->oad[mm].taskid);
										continue;
									}
								}
							}
							break;
						default:break;
						}
					}
				}
#ifdef SYS_INFO
				asyslog(LOG_INFO,"item_road->oadmr_num=%d\n",item_road->oadmr_num);
				for(mm=0;mm<item_road->oadmr_num;mm++) {
					asyslog(LOG_INFO,"taskid[%d]=%d\n",mm,item_road->oad[mm].taskid);
				}
#endif
				for(mm=0;mm<(item_road->oadmr_num);mm++)
				{
//					asyslog(LOG_INFO,"taskno=%d ,item_road->oad[%d].taskid=%d\n",taskno,mm,item_road->oad[mm].taskid);
//					if(taskno != 0 && taskno != item_road->oad[mm].taskid)
//					{
//						taskno = 0;
//						asyslog(LOG_INFO,"break taskno=%d\n",taskno);
//						break;
//					}
//					if(item_road->oad[mm].taskid != 0)
//						taskno = item_road->oad[mm].taskid;
					fprintf(stderr,"=========taskno=%d oad=%04x%02x%02x taskid=%d",
							taskno,item_road->oad[mm].oad_r.OI,item_road->oad[mm].oad_r.attflg,item_road->oad[mm].oad_r.attrindex,
							item_road->oad[mm].taskid);
					if(item_road->oad[mm].oad_r.OI == 0x202a || item_road->oad[mm].oad_r.OI == 0x6040 ||
							item_road->oad[mm].oad_r.OI == 0x6041 || item_road->oad[mm].oad_r.OI == 0x6042)
						continue;
					//if(taskno == 0)
					taskno = item_road->oad[mm].taskid;
//					if(taskno >0)
//						break;
					//TODO:这里注释行不行？？？？？？
					if(taskno == 0 || taskno != item_road->oad[mm].taskid)
						break;
//					asyslog(LOG_INFO,"i=%d ,taskno=%d\n",mm,taskno);
				}
				if(taskno != 0)
				{
					asyslog(LOG_INFO,"return  ,taskno=%d\n",taskno);
					return taskno;
				}
				else
					fprintf(stderr,"\n=======taskno=%d \n",taskno);
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
 * 根据csds得到任务号
 * findmethod = 1: 按照csds 的下发实际类型查找
 * findmethod = 2: 对与road,只查找相关的oad.解决浙江下发的曲线任务为oad,但是主站主动招测下发road 5002-0200(200a-0201.....),
 * 				   类型不匹配,如果按照oad方式查找,无法查找到实际的任务号
 */
INT8U GetTaskidFromCSDs_Sle0(CSD_ARRAYTYPE csds,ROAD_ITEM *item_road,INT8U findmethod,CLASS_6001 *tsa)
{
	CLASS_6015	class6015={};
	CLASS_6013	class6013={};
	int i=0,j=0,mm=0,nn=0,kk=0;
	INT8U taskno=0,tsa_equ=0;

	print_rcsd(csds);
	if(csds.num > MY_CSD_NUM)//超了
		csds.num = MY_CSD_NUM;
#ifdef SYS_INFO
	asyslog(LOG_INFO,"csds.num=%d\n",csds.num);
#endif
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


#ifdef SYS_INFO
	asyslog(LOG_INFO,"任务下发的主-从OI配置：oadmr_num=%d\n",item_road->oadmr_num);
	for(i=0;i<item_road->oadmr_num;i++){
		asyslog(LOG_INFO,"[%d] %04x_%04x\n",i,item_road->oad[i].oad_m.OI,item_road->oad[i].oad_r.OI);
	}
#endif
	memset(&class6013,0,sizeof(CLASS_6013));
	memset(&class6015,0,sizeof(CLASS_6015));
	for(i=0;i<256;i++)
	{
		tsa_equ = 0;
		if(readCoverClass(0x6013,i+1,&class6013,sizeof(class6013),coll_para_save) == 1)
		{
			if(class6013.cjtype != 1 || class6013.state != 1)//过滤掉不是普通采集方案的
				continue;
			if(readCoverClass(0x6015,class6013.sernum,&class6015,sizeof(CLASS_6015),coll_para_save) == 1)
			{
				if(tsa != NULL)
				{
					fprintf(stderr,"\n  metertype=%d\n",class6015.mst.mstype);
					switch(class6015.mst.mstype)
					{
					case 0:
						return 0;
					case 1:
						tsa_equ = 1;
						break;
					case 2:
						for(kk=0;kk<COLLCLASS_MAXNUM;kk++)
						{
							if(class6015.mst.ms.userType[kk] == tsa[0].basicinfo.usrtype)
								tsa_equ = 1;
						}
						break;
					case 3:
						for(kk=0;kk<COLLCLASS_MAXNUM;kk++)
						{
							if(memcmp(&class6015.mst.ms.userAddr[kk],&tsa[0].basicinfo.addr,sizeof(TSA)) == 0)
								tsa_equ = 1;
						}
						break;
					case 4:
						for(kk=0;kk<COLLCLASS_MAXNUM;kk++)
						{
							if(class6015.mst.ms.configSerial[kk] == tsa[0].sernum)
								tsa_equ = 1;
						}
						break;
					case 5:
						fprintf(stderr,"\n\n------");
						for(kk=0;kk<20;kk++)
							fprintf(stderr," %02x",class6015.mst.ms.type[0].begin[kk]);
						fprintf(stderr,"\n%02x-%02x : %02x \n",class6015.mst.ms.type[0].begin[1],class6015.mst.ms.type[0].end[1],
								tsa[0].basicinfo.usrtype);
						if(tsa[0].basicinfo.usrtype>=class6015.mst.ms.type[0].begin[1] && tsa[0].basicinfo.usrtype<=class6015.mst.ms.type[0].end[1])
							tsa_equ = 1;
						break;
					case 6:break;//暂时不实现
					case 7:break;
					default:
						break;
					}
				}
				if(tsa_equ==0)
					continue;
				else
					fprintf(stderr,"\ntsa_equ=1\n");
#ifdef SYS_INFO
				asyslog(LOG_INFO,"查找任务号 %d，方案序号：%d class6015.csds.num=%d",i+1,class6013.sernum,class6015.csds.num);
#endif
				for(j=0;j<class6015.csds.num;j++)
				{
#ifdef SYS_INFO
					asyslog(LOG_INFO,"jj=%d,csd.oad=%04x_%02x%02x \n",j,class6015.csds.csd[j].csd.oad.OI,
								class6015.csds.csd[j].csd.oad.attflg,class6015.csds.csd[j].csd.oad.attrindex);
#endif
					for(mm=0;mm<item_road->oadmr_num;mm++)
					{
						switch(class6015.csds.csd[j].type)
						{
						case 0:
#ifdef SYS_INFO
							asyslog(LOG_INFO,"mm=%d,oad_r  =%04x_%02x%02x \n",mm,item_road->oad[mm].oad_r.OI,
										item_road->oad[mm].oad_r.attflg,item_road->oad[mm].oad_r.attrindex);
#endif
							//浙江主站下发曲线任务oad,主动招测曲线数据,下发的oad为5002-*, 与任务的id不一致
//							if(findmethod == 2) {		//深度查找满足的任务oad
//						//		if((item_road->oad[mm].oad_m.OI > 0x0000) && (item_road->oad[mm].oad_m.OI < 0x5004))
//								if(item_road->oad[mm].oad_m.OI > 0x0000)
//								{
//									item_road->oad[mm].oad_m.OI = 0x0000;
//									item_road->oad[mm].oad_m.attflg = 0;
//									item_road->oad[mm].oad_m.attrindex = 0;
//									item_road->oad[mm].oad_num = 0;
//								}
//							}
							if(findmethod == 1 && (item_road->oad[mm].oad_m.OI == 0x0000)) 	//深度查找满足的任务oad
							{
								if(memcmp(&item_road->oad[mm].oad_r,&class6015.csds.csd[j].csd.oad,sizeof(OAD))==0 ||
										(item_road->oad[mm].oad_r.OI == class6015.csds.csd[j].csd.oad.OI &&
												item_road->oad[mm].oad_r.attrindex != 0 &&
												class6015.csds.csd[j].csd.oad.attrindex == 0)){
									item_road->oad[mm].taskid = i+1;
#ifdef SYS_INFO
									asyslog(LOG_INFO,"taskid find one %d",i+1);
#endif
									continue;
								}
							}else if(findmethod == 2) {
								if(memcmp(&item_road->oad[mm].oad_r,&class6015.csds.csd[j].csd.oad,sizeof(OAD))==0 ||
										(item_road->oad[mm].oad_r.OI == class6015.csds.csd[j].csd.oad.OI &&
												item_road->oad[mm].oad_r.attrindex != 0 &&
												class6015.csds.csd[j].csd.oad.attrindex == 0)){
									item_road->oad[mm].taskid = i+1;
#ifdef SYS_INFO
									asyslog(LOG_INFO,"taskid find one %d",i+1);
#endif
									continue;
								}
							}
							break;
						case 1:
#ifdef SYS_INFO
							  asyslog(LOG_INFO,"11111 mm=%d,oad_r  =%04x_%02x%02x \n",mm,item_road->oad[mm].oad_r.OI,
											item_road->oad[mm].oad_r.attflg,item_road->oad[mm].oad_r.attrindex);
							  asyslog(LOG_INFO,"11111 jj=%d,csd.oad=%04x_%02x%02x \n",j,class6015.csds.csd[j].csd.oad.OI,
											class6015.csds.csd[j].csd.oad.attflg,class6015.csds.csd[j].csd.oad.attrindex);
#endif
							if(memcmp(&item_road->oad[mm].oad_m,&class6015.csds.csd[j].csd.road.oad,sizeof(OAD))==0)//
							{
								for(nn=0;nn<class6015.csds.csd[j].csd.road.num;nn++)
								{
									fprintf(stderr,"oad_r=%04x%02x%02x %d.oad=%04x%02x%02x\n",
											item_road->oad[mm].oad_r.OI,item_road->oad[mm].oad_r.attflg,item_road->oad[mm].oad_r.attrindex,nn,
											class6015.csds.csd[j].csd.road.oads[nn].OI,class6015.csds.csd[j].csd.road.oads[nn].attflg,class6015.csds.csd[j].csd.road.oads[nn].attrindex);
									if(memcmp(&item_road->oad[mm].oad_r,&class6015.csds.csd[j].csd.road.oads[nn],sizeof(OAD))==0 ||
											(item_road->oad[mm].oad_r.OI == class6015.csds.csd[j].csd.road.oads[nn].OI &&
												item_road->oad[mm].oad_r.attrindex != 0 &&
												class6015.csds.csd[j].csd.road.oads[nn].attrindex == 0)){
										item_road->oad[mm].taskid = i+1;
										fprintf(stderr,"\n------find \n");
//										asyslog(LOG_INFO,"1111:item_road->oad[%d].taskid=%d\n",mm,item_road->oad[mm].taskid);
										continue;
									}
								}
							}
							break;
						default:break;
						}
					}
				}
#ifdef SYS_INFO
				asyslog(LOG_INFO,"item_road->oadmr_num=%d\n",item_road->oadmr_num);
				for(mm=0;mm<item_road->oadmr_num;mm++) {
					asyslog(LOG_INFO,"taskid[%d]=%d\n",mm,item_road->oad[mm].taskid);
				}
#endif
				for(mm=0;mm<(item_road->oadmr_num);mm++)
				{
//					asyslog(LOG_INFO,"taskno=%d ,item_road->oad[%d].taskid=%d\n",taskno,mm,item_road->oad[mm].taskid);
//					if(taskno != 0 && taskno != item_road->oad[mm].taskid)
//					{
//						taskno = 0;
//						asyslog(LOG_INFO,"break taskno=%d\n",taskno);
//						break;
//					}
//					if(item_road->oad[mm].taskid != 0)
//						taskno = item_road->oad[mm].taskid;
					fprintf(stderr,"=========taskno=%d oad=%04x%02x%02x taskid=%d",
							taskno,item_road->oad[mm].oad_r.OI,item_road->oad[mm].oad_r.attflg,item_road->oad[mm].oad_r.attrindex,
							item_road->oad[mm].taskid);
					if(item_road->oad[mm].oad_r.OI == 0x202a || item_road->oad[mm].oad_r.OI == 0x6040 ||
							item_road->oad[mm].oad_r.OI == 0x6041 || item_road->oad[mm].oad_r.OI == 0x6042)
						continue;
					//if(taskno == 0)
					taskno = item_road->oad[mm].taskid;
					if(taskno >0)
						break;
					//TODO:这里注释行不行？？？？？？
//					if(taskno == 0 || taskno != item_road->oad[mm].taskid)
//						break;
//					asyslog(LOG_INFO,"i=%d ,taskno=%d\n",mm,taskno);
				}
				if(taskno != 0)
				{
					asyslog(LOG_INFO,"return  ,taskno=%d\n",taskno);
					return taskno;
				}
				else
					fprintf(stderr,"\n=======taskno=%d \n",taskno);
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
FILE* opendatafile(INT8U taskid,CURR_RECINFO recinfo,INT8U taskinfoflg)
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

	if(taskinfoflg == 2)
	{
		asyslog(LOG_INFO,"n月冻结招测\n");
		ts_rec.Day = 0;
	}
	getTaskFileName(taskid,ts_rec,fname);//得到要抄读的文件名称
	fprintf(stderr,"fname=%s\n",fname);
	asyslog(LOG_INFO,"组帧frmdata，打开任务文件=%s, taskid=%d\n",fname,taskid);
	fp =fopen(fname,"r");
	return fp;
}
FILE* openevefile(OI_698 eve_oi)
{
	FILE *fp = NULL;
	char	fname[FILENAMELEN]={};
	getEveFileName(eve_oi,fname);//得到要抄读的文件名称
	fprintf(stderr,"fname=%s\n",fname);
	asyslog(LOG_INFO,"组帧frmdata，打开任务文件=%s,\n",fname);
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

/*
 * 在文件fp中查找tsa第一次出现的文件偏移位置
 */
int findTsa(TSA tsa,FILE *fp,int headsize,int blocksize)
{
	int i=0;

	rewind(fp);
	fseek(fp,headsize,SEEK_CUR);
	INT8U  tsa_tmp[TSA_LEN + 1];
	int offset = headsize;

	fprintf(stderr,"\nblocksize = %d offset=%d,需要查找 TSA: %d-",blocksize,offset,tsa.addr[0]);
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
		if(memcmp(&tsa_tmp[1],&tsa.addr[0],(tsa.addr[0]+1))==0)
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
//	fprintf(stderr,"\ntsa偏移：%d 查找序号%d：偏移%d\n",offsetTsa,recordno,recordoffset);
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
	}else recordno = 0;		//冻结抄读
	asyslog(LOG_INFO,"当前：%d:%d 任务开始：%d:%d 任务间隔:%d 记录序号%d",tm_p->tm_hour,tm_p->tm_min,starthour,startmin,interval,recordno);
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
	int i=0,j=0,mm=0;
	INT8U tmpbuf[256];
	int pindex = 0,retlen=0, onelen=0;

//	fprintf(stderr,"oadmr_num=%d  unitnum=%d \n",item_road.oadmr_num,unitnum);
//	for(i=0;i<item_road.oadmr_num;i++)
	{
		memset(tmpbuf,0x00,256);
//		fprintf(stderr,"item_road.oadmr_num = %d\n",item_road.oadmr_num);
		for(j=0;j<item_road.oadmr_num;j++)
		{
//			fprintf(stderr,"\n%04x-%04x--%d\n",item_road.oad[j].oad_m.OI,item_road.oad[j].oad_r.OI,item_road.oad[j].oad_num);
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
				int kk = 0;
				fprintf(stderr,"\ntmpbuf(%d):",oad_offset[j].len);
				for(kk=0;kk<oad_offset[j].len;kk++)
					fprintf(stderr,"tmpbuf[%d]=%02x\n",kk,tmpbuf[kk]);
				switch(tmpbuf[0])
				{
				case 0:
//					fprintf(stderr,"\n---------------tmpbuf[0]=%d\n",tmpbuf[0]);
					if(getZone("HuNan")==0 && tmpbuf[0] == 0 && item_road.oad[j].oad_r.attrindex == 0)
					{
						OI_INFO oi_info;
						GetOIinfo(item_road.oad[j].oad_r.OI,4,&oi_info);
						fprintf(stderr,"\n地区：湖南  %d\n",oi_info.oinum);
						if(oi_info.oinum>1)//array类型
						{
							databuf[pindex++] = 1;
							databuf[pindex++] = oi_info.oinum;
							for(mm=0;mm<oi_info.oinum;mm++)
								databuf[pindex++] = 0;
							break;
						}
					}
					databuf[pindex++] = 0;
//					fprintf(stderr,"000 pindex=%d\n",pindex);
					break;
				case 1://array
					memcpy(&databuf[pindex],tmpbuf,2);
					pindex += 2;
//					retlen = CalcOIDataLen(oad_offset[j].oad_r);
//					fprintf(stderr,"\n元素个数%d\n",tmpbuf[1]);
					retlen = 2;	//array 两个值
					for(i=0;i<tmpbuf[1];i++)
					{
//						fprintf(stderr,"\n类型(%d)-%d  长度%d\n",retlen*i+2,tmpbuf[retlen*i+2],retlen);
						if(tmpbuf[retlen] == 0)
							databuf[pindex++] = 0;
						else
						{
							onelen = getDataTypeLen(tmpbuf[retlen]); //数据实际长度
							onelen += 1;	//+1:数据类型
							memcpy(&databuf[pindex],&tmpbuf[retlen],onelen);
							retlen += onelen;
							pindex += onelen;
						}
					}
					fprintf(stderr,"pindex=%d\n",pindex);
//					retlen = retlen*tmpbuf[1]+2;//2代表一个array类型加一个个数
//					memcpy(&databuf[pindex],tmpbuf,retlen);
//					pindex += retlen;
					break;
//				case 2://struct 暂时不处理
//					break;
				case 0x55:	//TSA
					memcpy(&databuf[pindex],tmpbuf,(tmpbuf[1]+2));
					pindex += (tmpbuf[1]+2);
					break;
				default:
					memcpy(&databuf[pindex],tmpbuf,oad_offset[j].len);
					pindex += oad_offset[j].len;
					break;
				}
//				int k=0;
//				for(k=0;k<pindex;k++) {
//					fprintf(stderr,"%02x ",databuf[k]);
//				}
//				fprintf(stderr,"\n\n");
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

	for(i=0;i<item_road.oadmr_num;i++) {
		fprintf(stderr,"\nitem_road.oad[i].oad_m.OI = %04x  item_road.oad[i].oad_num = %d\n",item_road.oad[i].oad_m.OI,
				item_road.oad[i].oad_num);
		if(item_road.oad[i].oad_m.OI == 0x0000 && item_road.oad[i].oad_r.OI == 0x202a) {
			databuf[pindex++] = dttsa;
			memcpy(&databuf[pindex],&tsa,(tsa.addr[0]+1));
			pindex += (tsa.addr[0]+1);
		}else {
			if((item_road.oad[i].oad_m.OI == 0x0000) || (item_road.oad[i].oad_num != 0)) {
//				if(getZone("HuNan")==0)
//				{
//					DEBUG_TIME_LINE("");
//					fprintf(stderr,"\n地区：湖南\n");
//					OI_INFO oi_info;
//					GetOIinfo(item_road.oad[i].oad_r.OI,4,&oi_info);
//					fprintf(stderr,"oi = %04x,oi_info.io_unit=%d,oi_info.oinum=%d,item_road.oad[i].oad_r.attrindex=%d",
//							item_road.oad[i].oad_r.OI,oi_info.io_unit,oi_info.oinum,item_road.oad[i].oad_r.attrindex);
//					if(oi_info.oinum != 0 && item_road.oad[i].oad_r.attrindex == 0)
//					{
//						switch(oi_info.io_unit)
//						{
//						case 1://array
//							databuf[pindex++] = 1;
//							databuf[pindex++] = oi_info.oinum;
//							memset(&databuf[pindex],0x00,oi_info.oinum);
//							pindex += oi_info.oinum;
//							break;
//						case 2://struct
//							databuf[pindex++] = 2;
//							databuf[pindex++] = oi_info.oinum;
//							memset(&databuf[pindex],0x00,oi_info.oinum);
//							pindex += oi_info.oinum;
//							break;
//						default:
//							databuf[pindex++] = 0;
//							break;
//						}
//					}
//					else {
//						databuf[pindex++] = 0;
//						DEBUG_TIME_LINE("");
//					}
//					DEBUG_TIME_LINE("");
//				}
//				else {
					databuf[pindex++] = 0;
//				}
				fprintf(stderr,"\npindex = %d\n",pindex);
			}
		}
	}
	fprintf(stderr,"fillTsaNullData----index=%d\n",pindex);
	return pindex;
}

/*
 * 文件最前面两个字节是 INT16U类型的,
 * 表示文件头的大小; 紧接着的两个
 * 字节, 也是INT6U类型的, 表示每行数据
 * 的长度; 头部的每个块是一个HEAD_UNIT
 * 类型, 通过文件头部的长度, 可以计算出
 * 表格有多少列.
 * @head_len: 文件头的长度
 * @tsa_len: 每行数据的长度
 * @head_unit: 列描述
 */
INT16S GetTaskHead(FILE *fp,INT16U *head_len,INT16U *tsa_len,HEAD_UNIT **head_unit)
{
	INT8U 	headl[2],blockl[2];
	INT16U 	unitnum=0;

	fread(headl,2,1,fp);
	*head_len = headl[0];
	*head_len = (headl[0]<<8) + headl[1];
	if(*head_len<=4)
		return -1;
	fread(&blockl,2,1,fp);
	*tsa_len = blockl[0];
	*tsa_len = (blockl[0]<<8) + blockl[1];
	unitnum = (*head_len-4)/sizeof(HEAD_UNIT);
	if(*head_unit==NULL)
		*head_unit = malloc(*head_len);
	fread(*head_unit,*head_len-4,1,fp);
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
	case 0://上一条
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
		if(fp != NULL)
		{
			fclose(fp);
		}
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
INT8U initrecinfo(OAD getOAD,CURR_RECINFO *recinfo,TASKSET_INFO tasknor_info,INT8U selectype,RSD select,INT8U freezetype)
{
	time_t time_s,time_tmp,sec_tmp=0;
	struct tm *tm_p;
	time(&time_s);
	tm_p = localtime(&time_s);
	switch(selectype)
	{
	case 0://招测上一条记录
		fprintf(stderr,"\n----------sele 0\n");
		recinfo->recordno_num = 1;
		recinfo->rec_start = time(NULL);//当前时间的秒数
		recinfo->rec_end = time(NULL);
		break;
	case 4:
	case 5:
		recinfo->recordno_num = tasknor_info.runtime;
		//		if(freezetype == 3)//日冻结
		//			time_s += 86400;//24*60*60; 加上一天的秒数
		tm_p->tm_year = select.selec5.collect_save.year.data - 1900;
		tm_p->tm_mon = select.selec5.collect_save.month.data - 1;
		tm_p->tm_mday = select.selec5.collect_save.day.data;//天数
		tm_p->tm_hour = tasknor_info.starthour;
		tm_p->tm_min = tasknor_info.startmin;
		tm_p->tm_sec = 0;
		asyslog(LOG_INFO,"sele5 tasknor_info.save_timetype=%d,freezetype=%d",tasknor_info.save_timetype,freezetype);

		//一致性测试GET_20 sel5,存储类型=2，跨日招测当天的日冻结数据，查找文件，day不能+1
//		if(freezetype == 1 && (tasknor_info.save_timetype == 2 || tasknor_info.save_timetype == 3 || tasknor_info.save_timetype == 4))
		if(freezetype == 1 && (tasknor_info.save_timetype == 3 || tasknor_info.save_timetype == 4))
		{
			asyslog(LOG_INFO,"日冻结招测小时=%d",select.selec7.collect_save_star.hour.data);
			tm_p->tm_mday = tm_p->tm_mday+1;
		}
		recinfo->rec_start = mktime(tm_p);
		/////////////////////////////////////////TEST
		tm_p = localtime(&recinfo->rec_start);
		asyslog(LOG_INFO,"招测的时间%d-%d-%d %d:%d:%d",select.selec7.collect_save_star.year.data,select.selec7.collect_save_star.month.data
				,select.selec7.collect_save_star.day.data,select.selec7.collect_save_star.hour.data
				,select.selec7.collect_save_star.min.data,select.selec7.collect_save_star.sec.data);
		asyslog(LOG_INFO,"重新计算后的时间%d-%d-%d %d:%d:%d",tm_p->tm_year+1900,tm_p->tm_mon+1,tm_p->tm_mday,
				tm_p->tm_hour,tm_p->tm_min,tm_p->tm_sec);
		/////////////////////////////////////////////

//		time(&time_s);
//		if(freezetype == 3)//日冻结
//			time_s += 86400;//24*60*60; 加上一天的秒数
//		tm_p = localtime(&time_s);
		tm_p->tm_year = select.selec5.collect_save.year.data - 1900;
		tm_p->tm_mon = select.selec5.collect_save.month.data - 1;
		tm_p->tm_mday = select.selec5.collect_save.day.data;
		tm_p->tm_hour = tasknor_info.endhour;
		tm_p->tm_min = tasknor_info.endmin;
		tm_p->tm_sec = 0;
		if(freezetype == 1 && (tasknor_info.save_timetype == 3 || tasknor_info.save_timetype == 4))
		{
			asyslog(LOG_INFO,"日冻结招测小时=%d",select.selec7.collect_save_star.hour.data);
			tm_p->tm_mday = tm_p->tm_mday+1;
		}
		recinfo->rec_end = mktime(tm_p);
		break;
	case 7://实时数据类
		asyslog(LOG_INFO,"zc_start:%04d-%02d-%02d %02d:%02d",select.selec7.collect_save_star.year.data,
				select.selec7.collect_save_star.month.data,select.selec7.collect_save_star.day.data
				,select.selec7.collect_save_star.hour.data,select.selec7.collect_save_star.min.data);
		asyslog(LOG_INFO,"zc_end:%04d-%02d-%02d %02d:%02d",select.selec7.collect_save_finish.year.data,
				select.selec7.collect_save_finish.month.data,select.selec7.collect_save_finish.day.data
				,select.selec7.collect_save_finish.hour.data,select.selec7.collect_save_finish.min.data);
		time(&time_s);
		tm_p = localtime(&time_s);
		if(select.selec7.collect_save_star.year.data == 0xffff)//时标默认
		{
			asyslog(LOG_INFO,"时标fffffff，招测时间默认");//距离0点0分整数倍任务执行频率倍秒数
			sec_tmp = ((tm_p->tm_hour*3600+tm_p->tm_min*60+tm_p->tm_sec)/tasknor_info.taskfreq)*tasknor_info.taskfreq;
			tm_p->tm_hour = 0;
			tm_p->tm_min = 0;
			tm_p->tm_sec = 0;
			recinfo->rec_end = mktime(tm_p) + sec_tmp - tasknor_info.taskfreq;//上报上一次
			recinfo->rec_start = recinfo->rec_end - tasknor_info.taskfreq;
			recinfo->recordno_num = tasknor_info.taskfreq/tasknor_info.freq;
		}
		else
		{
			////TODO:  TimeBCDTotime_t()  转换函数可用
			tm_p = localtime(&time_s);
			tm_p->tm_year = select.selec7.collect_save_star.year.data-1900;
			tm_p->tm_mon = select.selec7.collect_save_star.month.data-1;
			tm_p->tm_mday = select.selec7.collect_save_star.day.data;
			tm_p->tm_hour = select.selec7.collect_save_star.hour.data;
			tm_p->tm_min = select.selec7.collect_save_star.min.data;
			tm_p->tm_sec = select.selec7.collect_save_star.sec.data;
			recinfo->rec_start = mktime(tm_p);

			time(&time_s);
			time_tmp = time(NULL);
			tm_p = localtime(&time_s);
			tm_p->tm_year = select.selec7.collect_save_finish.year.data-1900;
			tm_p->tm_mon = select.selec7.collect_save_finish.month.data-1;
			tm_p->tm_mday = select.selec7.collect_save_finish.day.data;
			tm_p->tm_hour = select.selec7.collect_save_finish.hour.data;
			tm_p->tm_min = select.selec7.collect_save_finish.min.data;
			tm_p->tm_sec = select.selec7.collect_save_finish.sec.data;
			recinfo->rec_end = mktime(tm_p);
//			if(time_tmp <= recinfo->rec_end && time_tmp >= recinfo->rec_start)
			if(abs(recinfo->rec_end-recinfo->rec_start) >= 3*24*60*60)//招测跨度大于三天只上报上一次数据
			{
				tm_p = localtime(&time_s);
				asyslog(LOG_INFO,"时标跨度大，招测时间默认");//距离0点0分整数倍任务执行频率倍秒数
				sec_tmp = ((tm_p->tm_hour*3600+tm_p->tm_min*60+tm_p->tm_sec)/tasknor_info.taskfreq)*tasknor_info.taskfreq;
				tm_p->tm_hour = 0;
				tm_p->tm_min = 0;
				tm_p->tm_sec = 0;
				recinfo->rec_end = mktime(tm_p) + sec_tmp - tasknor_info.taskfreq;//上报上一次
				recinfo->rec_start = recinfo->rec_end - tasknor_info.taskfreq;
				recinfo->recordno_num = tasknor_info.taskfreq/tasknor_info.freq;
			} else {
				if(time_tmp <= recinfo->rec_end && time_tmp >= recinfo->rec_start)
				{
					asyslog(LOG_INFO,"--------%d---%d",time_tmp,recinfo->rec_end);
					recinfo->rec_end = time_tmp;
				}
				//一致性测试GET_21:Selector6招测选择区间应该是前闭后开【起始值，结束值），查询记录数不应该+1
				if((recinfo->rec_end - recinfo->rec_start)%tasknor_info.freq ==0) {//测试负荷曲线改得
				recinfo->recordno_num = (recinfo->rec_end - recinfo->rec_start)/tasknor_info.freq;
				}else {
				recinfo->recordno_num = (recinfo->rec_end - recinfo->rec_start)/tasknor_info.freq + 1;
				}
				//一致性测试GET_21:Selector6招测选择区间应该是前闭后开【起始值，结束值），查询记录数不应该+1
				if((recinfo->rec_end - recinfo->rec_start)%tasknor_info.freq ==0) {//测试负荷曲线改得
					recinfo->recordno_num = (recinfo->rec_end - recinfo->rec_start)/tasknor_info.freq;
				}else {
					recinfo->recordno_num = (recinfo->rec_end - recinfo->rec_start)/tasknor_info.freq + 1;
				}
			}
		}
		asyslog(LOG_INFO,"n-----recinfo->recordno_num=%d,recinfo->rec_end=%d,recinfo->rec_start=%d,tasknor_info.freq=%d\n"
				,recinfo->recordno_num,recinfo->rec_end,recinfo->rec_start,tasknor_info.freq);

		break;
	case 10://主动上报类
		//一致性测试GET_25 sel10 OAD=6012_0300,应能正确查找数据
//		if(getOAD.OI==0x6012 && getOAD.attflg==03 && getOAD.attrindex==00) {
//			//只处理了日冻结处理
//			recinfo->recordno_num = select.selec10.recordn;
//			time(&time_s);
//			tm_p = localtime(&time_s);
//			tm_p->tm_hour = 0;
//			tm_p->tm_min = 0;
//			tm_p->tm_sec = 0;
//			fprintf(stderr,"start:%d-%d-%d %d:%d:%d\n",tm_p->tm_year,tm_p->tm_mon,tm_p->tm_mday,tm_p->tm_hour,tm_p->tm_min,tm_p->tm_sec);
//			if(tasknor_info.save_timetype == 2) {//相对当日0点0分
//				recinfo->rec_start = mktime(tm_p);
//				recinfo->rec_end = recinfo->rec_start+tasknor_info.freq;
//			}else {	//未测试
//				recinfo->rec_start = mktime(tm_p)-tasknor_info.freq;
//				recinfo->rec_end = mktime(tm_p);
//			}
//			fprintf(stderr,"end(%ld):%d-%d-%d %d:%d:%d\n",recinfo->rec_end,tm_p->tm_year,tm_p->tm_mon,tm_p->tm_mday,tm_p->tm_hour,tm_p->tm_min,tm_p->tm_sec);
//			fprintf(stderr,"start(%ld) freq=%d\n",recinfo->rec_start,tasknor_info.freq);
//		}else {
			recinfo->recordno_num = select.selec10.recordn;
			fprintf(stderr,"\nselect.selec10.recordn=%d\n",select.selec10.recordn);
			recinfo->rec_start = time(NULL);
			recinfo->rec_end = recinfo->rec_start-recinfo->recordno_num*tasknor_info.freq;
//		}
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
	if(selectype == 0)
		return 1;
	if(selectype == 10)
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
 * 支持招测日冻结
 */
INT16U GetOADFileData(OAD oad_m,OAD oad_r,INT8U taskid,TSA tsa,TS ts_zc,INT8U *databuf)
{
	int unitnum=0, offsetTsa=0, recordoffset=0, recordlen=0, currecord=0;
	INT16U  blocksize=0,headsize=0;
	INT8U recordbuf[1000],frztype=0;
	HEAD_UNIT *headunit = NULL;//文件头
	ROAD_ITEM item_road;
	OAD_INDEX oad_offset;//oad索引
	TASKSET_INFO tasknor_info;
	FILE *fp = NULL;
	char fname[FILENAMELEN]={};
	memset(&item_road,0x00,sizeof(ROAD_ITEM));
	item_road.oadmr_num = 1;
	item_road.oad[0].oad_num = 1;
	memcpy(&item_road.oad[0].oad_m,&oad_m,sizeof(OAD));
	memcpy(&item_road.oad[0].oad_r,&oad_r,sizeof(OAD));

	if((frztype = ReadTaskInfo(taskid,&tasknor_info))==0)//得到任务信息
	{
		asyslog(LOG_INFO,"得到任务信息失败\n");
		fprintf(stderr,"\n得到任务信息失败\n");
		return 0;
	}
	getTaskFileName(taskid,ts_zc,fname);//得到要抄读的文件名称
	fp =fopen(fname,"r");
	if(fp == NULL)
		return 0;

	unitnum = GetTaskHead(fp,&headsize,&blocksize,&headunit);

	memset(&oad_offset,0x00,sizeof(OAD_INDEX));
	GetOADPosofUnit(item_road,headunit,unitnum,&oad_offset);//得到每一个oad在块数据中的偏移

	offsetTsa = findTsa(tsa,fp,headsize,blocksize);

	recordlen = blocksize/tasknor_info.runtime;
	currecord = (ts_zc.Hour*60 + ts_zc.Minute) - (tasknor_info.starthour*60 + tasknor_info.startmin);
	if(tasknor_info.freq!=0) {
		currecord = currecord/(tasknor_info.freq/60);
	}else currecord = 0;		//冻结抄读
	recordoffset = findrecord(offsetTsa,recordlen,currecord);

	fseek(fp,recordoffset,SEEK_SET);
	fread(recordbuf,recordlen,1,fp);
	return collectData(databuf,recordbuf,&oad_offset,item_road);
}

/*
 * 支持液晶招测日月冻结
 * csds存储需要查询的日月冻结数据（抄读成功时间/正向有功电能量等）
 * tsa电表地址
 * ts_zc冻结时间
 * databuf返回存储（此处液晶库中开辟10个指针数组，具体数据存储空间在本函数开辟）
 * item_road存储需要查询的
 */
INT16S GUI_GetFreezeData(CSD_ARRAYTYPE csds,TSA tsa,TS ts_zc,INT8U *databuf)
{
	INT32U unitnum=0, offsetTsa=0, recordoffset=0;
	INT16U  blocksize=0,headsize=0;//blocksize每个块长度（开头3，4字节）headsize头部大小（开头1，2字节）
	ROAD_ITEM item_road;
	memset(&item_road,0,sizeof(ROAD_ITEM));
	HEAD_UNIT *headunit = NULL;//文件头
	OAD_INDEX *oad_offset=NULL;//
	INT8U taskid=0;
	INT16S retBufLen=0;
	FILE *fp = NULL;
	int i,j=0;
	char fname[FILENAMELEN]={};

	if(csds.num>10 || csds.num<=0) return -1;//查询数量不允许超过10

	//根据传入的csds，获取到taskid编号，同时填充item_road
	taskid = GetTaskidFromCSDs(csds,&item_road,1,NULL);
	if(taskid<=0)		return -2;

	getTaskFileName(taskid,ts_zc,fname);//得到要抄读的文件名称
	fp =fopen(fname,"r");
	if(fp == NULL) return -3;
	do//方便跳出，释放资源
	{
		unitnum = GetTaskHead(fp,&headsize,&blocksize,&headunit);//获取文件头由多少个HEAD_UNIT单元组成
		if(unitnum<=0 )
		{
			retBufLen = -4;
			break;
		}
		 oad_offset =(OAD_INDEX *) malloc(unitnum*sizeof(OAD_INDEX));//为每个查询oad准备一个空间存数据
		 memset(oad_offset,0,unitnum*sizeof(OAD_INDEX));

			//获取所查询oad在块中的偏移起始位置和长度,放入oad_offset中
		GetOADPosofUnit(item_road,headunit,unitnum,oad_offset);//得到所需查询oad在文件头中的偏移。该偏移也代表在每个数据块中的偏移

		//获取所查询表号在数据块中的起始偏移位置
		offsetTsa = findTsa(tsa,fp,headsize,blocksize);
		if(offsetTsa<=0)
		{
			retBufLen = -5;
			break;
		}
		//拷贝出所有查询的oad对应的数据到databuf对应的指针数组中
		j=0;
		for(i=0;i<unitnum;i++)
		{
			if((oad_offset+i)->len <= 0) continue;
			fprintf(stderr,"oad_offset .len = %d\n",(oad_offset+i)->len);
			recordoffset = offsetTsa + oad_offset[i].offset;
			databuf[retBufLen++] = (oad_offset+i)->len;
			fseek(fp,recordoffset,SEEK_SET);
			fread(&databuf[retBufLen],(oad_offset+i)->len,1,fp);
			retBufLen+=(oad_offset+i)->len;
			j++;
		}
	}while(0);
	//free close then return
	if(headunit != NULL) free(headunit);
	if(oad_offset!=NULL) free(oad_offset);
	if(retBufLen>=255) return 0;//该函数只作液晶日月冻结，共3个项查询数据，不会大于255
	if(fp!=NULL) fclose(fp);
	 return retBufLen;
}
INT8U get60136015info(INT8U taskid,CLASS_6015 *class6015,CLASS_6013 *class6013)
{
	int i=0;
	memset(class6013,0,sizeof(CLASS_6013));
	memset(class6015,0,sizeof(CLASS_6015));
	if(readCoverClass(0x6013,taskid,class6013,sizeof(CLASS_6013),coll_para_save) == 1)
	{
		if(class6013->cjtype != 1 || class6013->state != 1)//
			return 0;
		if(readCoverClass(0x6015,class6013->sernum,class6015,sizeof(CLASS_6015),coll_para_save) == 1)
		{
			for(i=0;i<MY_CSD_NUM;i++)
			{
				switch(class6015->csds.csd[i].csd.road.oad.OI)//union
				{
				case 0x0000://
				case 0x5000:
				case 0x5002:
				case 0x5003://
					break;
				case 0x5004://日冻结
				case 0x5005://结算日
					return 1;
					break;
				case 0x5006://月冻结
					return 2;
					break;
				case 0x5007://年冻结
					return 3;
					break;
				default:break;
				}
			}
			return 4;//实时
		}
	}
	else
		return 0;
	return 4;//实时或曲线
}
INT8U cmpTSAtype(CLASS_6001 *tsa,CLASS_6015	class6015)
{
	int kk=0;
	if(tsa != NULL)
	{
		fprintf(stderr,"\n  metertype=%d,tsa.usrtype=%d\n",class6015.mst.mstype,tsa[0].basicinfo.usrtype);
//		prtTSA(tsa[0].basicinfo.addr);
		switch(class6015.mst.mstype)
		{
		case 0:
			return 0;
		case 1:
			return 1;
			break;
		case 2:
			for(kk=0;kk<COLLCLASS_MAXNUM;kk++)
			{
				if(class6015.mst.ms.userType[kk] == tsa[0].basicinfo.usrtype)
					return 1;
			}
			break;
		case 3:
			for(kk=0;kk<COLLCLASS_MAXNUM;kk++)
			{
				if(memcmp(&class6015.mst.ms.userAddr[kk],&tsa[0].basicinfo.addr,sizeof(TSA)) == 0)
					return 1;
			}
			break;
		case 4:
			for(kk=0;kk<COLLCLASS_MAXNUM;kk++)
			{
				if(class6015.mst.ms.configSerial[kk] == tsa[0].sernum)
					return 1;
			}
			break;
		case 5:
			fprintf(stderr,"\n\n------");
			for(kk=0;kk<20;kk++)
				fprintf(stderr," %02x",class6015.mst.ms.type[0].begin[kk]);
			fprintf(stderr,"\n%02x-%02x : %02x \n",class6015.mst.ms.type[0].begin[1],class6015.mst.ms.type[0].end[1],
					tsa[0].basicinfo.usrtype);
			if(tsa[0].basicinfo.usrtype>=class6015.mst.ms.type[0].begin[1] && tsa[0].basicinfo.usrtype<=class6015.mst.ms.type[0].end[1])
				return 1;
			break;
		case 6:break;//暂时不实现
		case 7:break;
		default:
			break;
		}
	}
	return 0;
}

/*
 * 要得到的主从oad,为其他进程提供接口
 */
INT16U GetOADData(OAD oad_m,OAD oad_r,TS ts_zc,CLASS_6001 tsa_6001,INT8U *databuf)
{
	CLASS_6015	class6015={};
	CLASS_6013	class6013={};
	INT8U frz_type = 0;
	int i=0,j=0,nn=0;
	memset(&class6013,0,sizeof(CLASS_6013));
	memset(&class6015,0,sizeof(CLASS_6015));
	for(i=0;i<256;i++)
	{
		if((frz_type=get60136015info(i+1,&class6015,&class6013))==0)//
			continue;
		if(cmpTSAtype(&tsa_6001,class6015)==0)//比对tsa类型，不符和本采集方案的跳过
			continue;
//				asyslog(LOG_INFO,"查找任务号 %d，方案序号：%d class6015.csds.num=%d",i+1,class6013.sernum,class6015.csds.num);
		for(j=0;j<class6015.csds.num;j++)
		{
			switch(class6015.csds.csd[j].type)
			{
			case 0:
				if(oad_m.OI == 0x0000)//oad类型
				{
					if(memcmp(&oad_r,&class6015.csds.csd[j].csd.oad,sizeof(OAD))==0){
						return GetOADFileData(oad_m,oad_r,i+1,tsa_6001.basicinfo.addr,ts_zc,databuf);

					}
				}
				break;
			case 1:
				if(memcmp(&oad_m,&class6015.csds.csd[j].csd.road.oad,sizeof(OAD))==0)
				{
					for(nn=0;nn<class6015.csds.csd[j].csd.road.num;nn++)
					{
						if(memcmp(&oad_r,&class6015.csds.csd[j].csd.road.oads[nn],sizeof(OAD))==0){
							return GetOADFileData(oad_m,oad_r,i+1,tsa_6001.basicinfo.addr,ts_zc,databuf);
						}
					}
				}
				break;
			default:break;
			}
		}
	}
	return 0;
}
/*
 *获得任务数据和事件记录
 */
int GetTaskData(OAD oad,RSD select, INT8U selectype,CSD_ARRAYTYPE csds,INT16U frmmaxsize)
{
	TS ts_curr;
	DateTimeBCD CHTimeBCD[2];
	FILE *fp = NULL,*myfp = NULL;
	INT8U 	taskid=0,autoflg=0,recordbuf[1000],onefrmbuf[2000],tmpnull[8];
	ROAD_ITEM item_road;
	ROAD_ITEM item_road_2;
	CURR_RECINFO recinfo;
	HEAD_UNIT *headunit = NULL;//文件头
	OAD_INDEX oad_offset[100],oad_offset_can[100];//oad索引
	TASKSET_INFO tasknor_info;
	INT16U  blocksize=0,headsize=0;
	int offsetTsa = 0,recordoffset = 0,unitnum=0,i=0,j=0,k=0,indexn=0,recordlen = 0,currecord = 0,rec_tmp = 0,firecord = 0,tsa_num=0,framesum=0;
	INT8U recordnum=0,seqnumindex=0,taskinfoflg=0;
//	TSA *tsa_group = NULL;
	CLASS_6001 *tsa_group = NULL;//本次召测的tsa集合
	ROAD road_eve;
	INT8U eveflg=0;
	MY_MS meters_null;
	int		findmethod = 0,mm=0;

#ifdef SYS_INFO
	asyslog(LOG_INFO,"－1－selectype = %d\n",selectype);
#endif
	if((selectype & 0x80) != 0) {//主动上报
		autoflg = 1;
		selectype &= ~0x80;
	}
#ifdef SYS_INFO
	asyslog(LOG_INFO,"－2－selectype = %d －－frmmaxsize = %d\n",selectype,frmmaxsize);
#endif
	memset(&item_road,0x00,sizeof(ROAD_ITEM));
	if(selectype == 8 || selectype == 6)//将selector8和6写成selector7的处理办法
		selectype = 7;
	fprintf(stderr,"\n-----selectype = %d---%d\n",selectype,select.selec8.collect_succ_finish.day.data);

	if(selectype != 0 && selectype != 4 && selectype != 5 && selectype != 7 && selectype != 10)
		return 0;

	if(csds.num > MY_CSD_NUM)
		csds.num = MY_CSD_NUM;
	for(i=0;i<csds.num;i++) {//招测单个事件
		if(csds.csd[i].type != 1)
			continue;
		if(csds.csd[i].csd.road.oad.OI >= 0x3000 && \
			csds.csd[i].csd.road.oad.OI < 0x4000){//事件关联对象
			eveflg = 1;
			memcpy(&road_eve,&csds.csd[i].csd.road,sizeof(ROAD));
			break;
		}
	}

	switch(selectype)
	{
	case 0:
		meters_null.mstype = 1;//全部电表
		tsa_num = getOI6001(meters_null,(INT8U **)&tsa_group);
		break;
	case 4:
	case 5:
		fprintf(stderr,"selec4_5.meters mstype = %d,data=%d-%d\n",select.selec5.meters.mstype,select.selec5.meters.ms.userAddr[0].addr[0],select.selec5.meters.ms.userAddr[0].addr[1]);
		tsa_num = getOI6001(select.selec5.meters,(INT8U **)&tsa_group);
		break;
	case 7:
		tsa_num = getOI6001(select.selec7.meters,(INT8U **)&tsa_group);
		break;
	default:
		tsa_num = getOI6001(select.selec10.meters,(INT8U **)&tsa_group);
		fprintf(stderr,"tsa_num = %d\n",tsa_num);
		break;
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
		findmethod = 1;
		if((taskid = GetTaskidFromCSDs(csds,&item_road,findmethod,tsa_group)) == 0) {//暂时不支持招测的不在一个采集方案
			asyslog(LOG_INFO,"GetTaskData: taskid=%d\n",taskid);
			memset(&item_road,0,sizeof(item_road));
			findmethod = 2;
			if((taskid = GetTaskidFromCSDs(csds,&item_road,findmethod,tsa_group)) == 0) {
				asyslog(LOG_INFO," 只比对oad:GetTaskData: taskid=%d\n",taskid);
				return 0;
			}
		}
		if((taskinfoflg = ReadTaskInfo(taskid,&tasknor_info))==0)//得到任务信息
		{
			asyslog(LOG_INFO,"得到任务信息失败\n");
			fprintf(stderr,"\n得到任务信息失败\n");
			return 0;
		}
		memset(&recinfo,0x00,sizeof(CURR_RECINFO));
//		asyslog(LOG_INFO,"\n----------获得recinfo信息\n");
		initrecinfo(oad,&recinfo,tasknor_info,selectype,select,taskinfoflg);//获得recinfo信息
		fprintf(stderr,"\n----------获得recinfo信息成功\n");
		//获得第一个序号
		currecord = getrecordno(tasknor_info.starthour,tasknor_info.startmin,tasknor_info.freq,recinfo);//freq为执行间隔,单位分钟
		firecord = currecord;//每次切换表地址，当前记录序号赋值第一次的数值
		//1\打开数据文件
		fprintf(stderr,"\n----------1\n");
		fp = opendatafile(taskid,recinfo,taskinfoflg);
	}
	myfp = openFramefile(TASK_FRAME_DATA);
	if (fp==NULL || myfp==NULL)
	{
		if(tsa_group != NULL)
			free(tsa_group);
		if(headunit!=NULL){
			free(headunit);
		}
		if(fp != NULL)
			fclose(fp);
		if(myfp != NULL)
			fclose(myfp);
		asyslog(LOG_INFO,"\n打开文件%s失败\n",TASK_FRAME_DATA);
		return 0;
	}
	asyslog(LOG_INFO,"\n打开文件%s成功\n",TASK_FRAME_DATA);

	unitnum = GetTaskHead(fp,&headsize,&blocksize,&headunit);
	fprintf(stderr,"\n----------2\n");
	for(i=0;i<unitnum;i++)
		fprintf(stderr,"%04x%02x%02x:%04x%02x%02x:%04x\n",
				headunit[i].oad_m.OI,headunit[i].oad_m.attflg,headunit[i].oad_m.attrindex,
				headunit[i].oad_r.OI,headunit[i].oad_r.attflg,headunit[i].oad_r.attrindex,headunit[i].len);

	memset(oad_offset,0x00,sizeof(oad_offset));
	if(findmethod==2) {
		memcpy(&item_road_2,&item_road,sizeof(ROAD_ITEM));
		for(mm=0;mm<item_road_2.oadmr_num;mm++) {
			if(item_road_2.oad[mm].oad_m.OI > 0x0000) {
				item_road_2.oad[mm].oad_m.OI = 0x0000;
				item_road_2.oad[mm].oad_m.attflg = 0;
				item_road_2.oad[mm].oad_m.attrindex = 0;
				item_road_2.oad[mm].oad_num = 0;
			}
		}
		GetOADPosofUnit(item_road_2,headunit,unitnum,oad_offset);//得到每一个oad在块数据中的偏移
	} else {
		GetOADPosofUnit(item_road,headunit,unitnum,oad_offset);//得到每一个oad在块数据中的偏移
	}
	fprintf(stderr,"\n----------4\n");
	if(tasknor_info.runtime!=0) 	//异常处理
		recordlen = blocksize/tasknor_info.runtime;//计算每条记录的字节数
	for(i=0;i<tsa_num;i++) {
		fprintf(stderr,"\nTSA%d: %d-",i,tsa_group[i].basicinfo.addr.addr[0]);
		for(j=0;j<tsa_group[i].basicinfo.addr.addr[0];j++) {
			fprintf(stderr,"-%02x",tsa_group[i].basicinfo.addr.addr[j+1]);
		}
	}
	memset(onefrmbuf,0,sizeof(onefrmbuf));
	//初始化分帧头
	indexn = 2;
	indexn += initFrameHead(&onefrmbuf[indexn],oad,select,selectype,csds,&seqnumindex);

	if(tsa_num == 0) {
		asyslog(LOG_INFO,"未找到符合条件的TSA数据\n");
		if(tsa_group != NULL)
			free(tsa_group);
		if(headunit!=NULL){
			free(headunit);
		}
		if(fp != NULL)
			fclose(fp);
		if(myfp != NULL)
			fclose(myfp);
		return 0;
	}
	//3\定位TSA , 返回offset
	for(i =0; i< tsa_num; i++) {//遍历tsa_group
		currecord = firecord;//每次切换表地址，当前记录序号赋值第一次的数值
		offsetTsa = findTsa(tsa_group[i].basicinfo.addr,fp,headsize,blocksize);

		for(j=1; j<=recinfo.recordno_num;j++) {//test
			if(eveflg != 1 && taskinfoflg == 0) {//事件和日月冻结不更新数据流
				if(updatedatafp(fp,j,selectype,tasknor_info.freq,recinfo,taskid)==2)
					offsetTsa = findTsa(tsa_group[i].basicinfo.addr,fp,headsize,blocksize);
			}
			if(offsetTsa == 0) {
				asyslog(LOG_INFO,"task未找到数据,i=%d\n",i);
				if(autoflg == 1 && tasknor_info.runtime > 1 && recinfo.recordno_num == 1) {//主动上报曲线并且只上报一个点
					fprintf(stderr,"\n曲线没TSA记录,查找上一日的数据\n");
				} else {
					if(getZone("HuNan")==0) {//湖南的，没有数据直接跳过
						continue;
					}
					indexn += fillTsaNullData(&onefrmbuf[indexn],tsa_group[i].basicinfo.addr,item_road);
					recordnum++;
					continue;
				}
			}
			//5\定位指定的点（行）, 返回offset
			if(selectype == 7 || selectype == 5 || selectype == 4)
				rec_tmp = j-1;
			else
				if(getcurecord(selectype,&currecord,j,tasknor_info.runtime) == 0) {//招测天数跨度超出10天
					asyslog(LOG_INFO,"\n招测天数跨度超出10天\n",currecord);
					break;
				} else {
					if(autoflg == 1 && tasknor_info.runtime > 1 && recinfo.recordno_num == 1)//主动上报曲线并且只上报一个点
					{
						fprintf(stderr,"\n---曲线主动上报\n");
						currecord = firecord;
					}
					else
						fprintf(stderr,"\n---非曲线主动上报\n");
				}
#ifdef SYS_INFO
//			asyslog(LOG_INFO,"\n计算出来的currecord=%d\n",currecord+rec_tmp);
#endif
			recordoffset = findrecord(offsetTsa,recordlen,currecord+rec_tmp);
			memset(recordbuf,0x00,sizeof(recordbuf));
			//6\读出一行数据到临时缓存
			if(offsetTsa == 0)
				fprintf(stderr,"\n曲线主动上报tsa空\n");
			else {
				fseek(fp,recordoffset,SEEK_SET);
				fread(recordbuf,recordlen,1,fp);
//				printRecordBytes(recordbuf,recordlen);
			}
			if(autoflg == 1 && tasknor_info.runtime > 1 && recinfo.recordno_num == 1)//主动上报曲线并且只上报一个点
			{
				for(k=0;k<5;k++)//TsToTimeBCD(TS inTs,DateTimeBCD* outTimeBCD)
				{
					memset(tmpnull,0x00,8);
					if(memcmp(&recordbuf[18],tmpnull,8)==0 || offsetTsa == 0) {//本条记录为空或者没有这个tsa
						fprintf(stderr,"\n---曲线主动上报k=%d\n",k);
						fprintf(stderr,"当前   currecord=%d\n",currecord);
						if(currecord == 0)//往前跨一天
						{
							TS ts_tmp;
							char fname[FILENAMELEN]={};
							TSGet(&ts_tmp);
							tminc(&ts_tmp,day_units,-1);
							getTaskFileName(taskid,ts_tmp,fname);//得到要抄读的文件名称
							fprintf(stderr," 往前跨一天 fname=%s\n",fname);
							if(fp != NULL)
								fclose(fp);
							fp =fopen(fname,"r");
							offsetTsa = findTsa(tsa_group[i].basicinfo.addr,fp,headsize,blocksize);
							fprintf(stderr,"\n@@@@@@offsetTsa=%d\n",offsetTsa);
							currecord = tasknor_info.runtime-1;
						}
						else
							currecord--;
						fprintf(stderr,"查找上一个  currecord=%d\n",currecord);
						recordoffset = findrecord(offsetTsa,recordlen,currecord);
						memset(recordbuf,0x00,sizeof(recordbuf));
						//6\读出一行数据到临时缓存
						fseek(fp,recordoffset,SEEK_SET);//再读一次上一条记录上报
						fread(recordbuf,recordlen,1,fp);
						memset(tmpnull,0x00,8);
						if(memcmp(&recordbuf[18],tmpnull,8)==0)//本条记录为空
						{
							fprintf(stderr,"\n本条记录号%d为空\n",currecord);
							continue;
						}
						else
						{
							fprintf(stderr,"\n改时标1\n");
							TSGet(&ts_curr);
							TsToTimeBCD(ts_curr,&CHTimeBCD[0]);
							ts_curr.Minute = ts_curr.Minute/(tasknor_info.freq/15);
							ts_curr.Minute = ts_curr.Minute*(tasknor_info.freq/15);//取整
							ts_curr.Sec = 0;
							TsToTimeBCD(ts_curr,&CHTimeBCD[1]);
							fill_date_time_s(&recordbuf[18],&CHTimeBCD[0]);
							fill_date_time_s(&recordbuf[26],&CHTimeBCD[0]);
							fill_date_time_s(&recordbuf[34],&CHTimeBCD[1]);
							break;
						}
					}
					else//有数据跳出
					{
						if(k==0)//有这个数据,不更改时标
							break;
						fprintf(stderr,"\n改时标2\n");
						TSGet(&ts_curr);
						TsToTimeBCD(ts_curr,&CHTimeBCD[0]);
						ts_curr.Minute = ts_curr.Minute/(tasknor_info.freq/15);
						ts_curr.Minute = ts_curr.Minute*(tasknor_info.freq/15);//取整
						ts_curr.Sec = 0;
						TsToTimeBCD(ts_curr,&CHTimeBCD[1]);
						fill_date_time_s(&recordbuf[18],&CHTimeBCD[0]);
						fill_date_time_s(&recordbuf[26],&CHTimeBCD[0]);
						fill_date_time_s(&recordbuf[34],&CHTimeBCD[1]);
						break;
					}
				}
			}
			memset(tmpnull,0x00,8);
//			printRecordBytes(recordbuf,recordlen);
			if(memcmp(&recordbuf[18],tmpnull,8)==0 && memcmp(&recordbuf[26],tmpnull,8)==0 &&
					memcmp(&recordbuf[34],tmpnull,8)==0)
				continue;

//			printRecordBytes(recordbuf,recordlen);
			//7\根据csds挑选数据，组织存储缓存
			memcpy(oad_offset_can,oad_offset,sizeof(oad_offset));
			indexn += collectData(&onefrmbuf[indexn],recordbuf,oad_offset_can,item_road);
			recordnum++;
#ifdef SYS_INFO
//			asyslog(LOG_INFO,"recordnum=%d  seqnumindex=%d\n",recordnum,seqnumindex);
#endif
			if(frmmaxsize <= 256+100)
				frmmaxsize = 700;
			if (indexn>=frmmaxsize-300) {
				framesum++;
				//8 存储1帧
//				fprintf(stderr,"indexn=%d  recordnum=%d\n",indexn,recordnum);
				intToBuf((indexn-2),onefrmbuf);		//帧长度保存帧的数据长度
				onefrmbuf[seqnumindex] = recordnum;
				saveOneFrame(onefrmbuf,indexn,myfp);
				indexn = 2;
				indexn += initFrameHead(&onefrmbuf[indexn],oad,select,selectype,csds,&seqnumindex);
				recordnum = 0;
			}
		}
	}
#ifdef SYS_INFO
//	asyslog(LOG_INFO,"组帧：indexn=%d\n",indexn);
#endif
//	for(i=0;i<indexn;i++) {
//		fprintf(stderr,"%02x ",onefrmbuf[i]);
//	}
	if(framesum==0) {
		framesum = 1; //一帧
//		fprintf(stderr,"\n indexn = %d saveOneFrame  seqnumindex=%d,  recordnum=%d!!!!!!!!!!!!!!!!\n",indexn,seqnumindex,recordnum);
//		asyslog(LOG_INFO,"任务数据文件组帧:indexn = %d , seqnumindex=%d,  recordnum=%d\n",indexn,seqnumindex,recordnum);
		intToBuf((indexn-2),onefrmbuf);
		onefrmbuf[seqnumindex] = recordnum;
		saveOneFrame(onefrmbuf,indexn,myfp);
	}
	else {
		if(recordnum != 0)
		{
			framesum++;
//			fprintf(stderr,"\n last frm indexn = %d saveOneFrame  seqnumindex=%d,  recordnum=%d!!!!!!!!!!!!!!!!\n",indexn,seqnumindex,recordnum);
//			asyslog(LOG_INFO,"任务数据文件组帧:indexn = %d , seqnumindex=%d,  recordnum=%d\n",indexn,seqnumindex,recordnum);
			intToBuf((indexn-2),onefrmbuf);
			onefrmbuf[seqnumindex] = recordnum;
			saveOneFrame(onefrmbuf,indexn,myfp);
		}
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
	asyslog(LOG_INFO,"--framesum=%d\n",framesum);
	return (framesum );//返回实际帧数
}
/*
 * 文件数据流fp 文件头长度headlen 标准单元长度unitlen
 * 单元号索引unitno_index 索引到的单元号
 */
//void GetDataUnit(FILE *fp,INT16U headlen,INT16U unitlen,INT16U *unitno_index)
//{
//	fseek(fp,headlen+unitlen*unitno_index,SEEK_SET);
//
//}

int getSelector(OAD oad_h,RSD select, INT8U selectype, CSD_ARRAYTYPE csds, INT8U *data, int *datalen,INT16U frmmaxsize)
{
	int  framesum=0;		//分帧
//	asyslog(LOG_INFO,"getSelector: selectype=%d\n",selectype);
	fprintf(stderr,"getSelector: selectype=%d\n",selectype);
//	switch(selectype)
//	{
//	case 0:
//		break;
//	case 5:
//		framesum = GetTaskData(oad_h,select,selectype,csds);
//		break;
//	case 7:
//		framesum = GetTaskData(oad_h,select,selectype,csds);//程序里面计算
//		break;
//	case 10:
		framesum = GetTaskData(oad_h,select,selectype,csds,frmmaxsize);
		fprintf(stderr,"framesum=%d\n",framesum);
//		break;
//	default:break;
//	}
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
		fread(&bytelen,2,1,fp);				//读出数据报文长度
		fprintf(stderr," readFrameDataFile bytelen=%d\n",bytelen);
		if(bytelen>=MAX_APDU_SIZE) {		//防止读取数据溢出
			syslog(LOG_ERR,"read filename=%s bytelen = %d 大于限定值=%d\n",filename,bytelen,MAX_APDU_SIZE);
			return 0;
		}
		if (fread(buf,bytelen,1,fp) <=0 ) {	//按数据报文长度，读出全部字节
			syslog(LOG_NOTICE,"数据读取完毕\n");
			fclose(fp);
			return 0;
		}
		*datalen = bytelen;
		retoffset = ftell(fp);
		fclose(fp);
		return retoffset;		 			//返回当前偏移位置
	}
	return 0;
}
INT8U get_protocol_3761_tx_para()
{
	int fd;
	FILE* fp = NULL;
	INT8U ret = 0;
	CLASS25 class4500;
	CLASS26 class4510;
	CLASS_4001_4002_4003 class4001;
	Protocol_Trans trans_data;
	memset(&class4500,0,sizeof(CLASS25));
	memset(&class4510,0,sizeof(CLASS26));
	memset(&class4001,0,sizeof(CLASS_4001_4002_4003));
	memset(&trans_data,0,sizeof(Protocol_Trans));
	if(access(PROTOCOL_TRANS_PATH,F_OK) != 0)
	{
		return 0;
	}
	fp = fopen(PROTOCOL_TRANS_PATH,"r");
	if(fp == NULL)
	{
		return 0;
	}
	fseek(fp,0,SEEK_SET);
	ret = fread(&trans_data,sizeof(Protocol_Trans),1,fp);

	if(ret == 1)
	{
		if(trans_data.trans_flg == 1){
			//读出参数终端id
			readCoverClass(0x4001,0,&class4001,sizeof(CLASS_4001_4002_4003),para_vari_save);
			memset(class4001.curstom_num,0,sizeof(class4001.curstom_num));
			memcpy(&class4001.curstom_num[1],trans_data.OOPAddr,sizeof(trans_data.OOPAddr));
			class4001.curstom_num[0] = 6;
			saveCoverClass(0x4001, 0, &class4001, sizeof(CLASS_4001_4002_4003), para_vari_save);
			usleep(100*1000);

			//读出gprs通信参数
			readCoverClass(0x4500,0,&class4500,sizeof(CLASS25),para_vari_save);
			memset(&class4500.master,0,sizeof(class4500.master));

			memcpy(&class4500.master.master[0].ip[1],trans_data.main_ip,4);
			class4500.master.master[0].ip[0] = 4;
			class4500.master.master[0].port = trans_data.main_port;

			memcpy(&class4500.master.master[1].ip[1],trans_data.bak_ip,4);
			class4500.master.master[1].ip[0] = 4;
			class4500.master.master[1].port = trans_data.bak_port;

			class4500.master.masternum = 2;

			memset(class4500.commconfig.apn,0,sizeof(class4500.commconfig.apn));
			memcpy(&class4500.commconfig.apn[1],trans_data.APN,sizeof(trans_data.APN));
			class4500.commconfig.apn[0] = strlen((char*)&class4500.commconfig.apn[1]);

			memset(class4500.commconfig.userName,0,sizeof(class4500.commconfig.userName));
			memcpy(&class4500.commconfig.userName[1],trans_data.UsrName,sizeof(trans_data.UsrName));
			class4500.commconfig.userName[0] = strlen((char*)&class4500.commconfig.userName[1]);

			memset(class4500.commconfig.passWord,0,sizeof(class4500.commconfig.passWord));
			memcpy(&class4500.commconfig.passWord[1],trans_data.Pwd,sizeof(trans_data.Pwd));
			class4500.commconfig.passWord[0] = strlen((char*)&class4500.commconfig.passWord[1]);
	        syslog(LOG_NOTICE, "\nget_protocol_3761_tx_para 主IP %d.%d.%d.%d:%d\n", class4500.master.master[0].ip[1], class4500.master.master[0].ip[2],
	        		class4500.master.master[0].ip[3],class4500.master.master[0].ip[4], class4500.master.master[0].port);
			saveCoverClass(0x4500, 0, &class4500, sizeof(CLASS25), para_vari_save);
			usleep(100*1000);

			//写gprs拨号脚本
			write_apn((char*)trans_data.APN);
			usleep(100*1000);
			write_userpwd(trans_data.UsrName,trans_data.Pwd,trans_data.APN);
			usleep(100*1000);

			//读以太网通信参数
			readCoverClass(0x4510,0,&class4510,sizeof(CLASS26),para_vari_save);
			memset(&class4510.master,0,sizeof(class4510.master));
			memcpy(&class4510.master,&class4500.master,sizeof(class4510.master));
			saveCoverClass(0x4510, 0, &class4510, sizeof(CLASS26), para_vari_save);
			//			fseek(fp,0,SEEK_SET);
			trans_data.trans_flg = 0;//清除转换标志
			if(fp!=NULL)
			{
				fclose(fp);
				fp = NULL;
			}
			fp = fopen(PROTOCOL_TRANS_PATH,"w+");
			if(fp != NULL){
				fseek(fp,0,SEEK_SET);
				ret = fwrite(&trans_data,sizeof(Protocol_Trans),1,fp);
//				fprintf(stderr,"\n-------ret = %d--------\n",ret);
				if(ret != 1){
					ret = 0;//写文件失败
				}
				else{
					fflush(fp);
					fd = fileno(fp);
					fsync(fd);
				}
			}
			else ret = 0;
		}
	}

	if(fp != NULL){
		fclose(fp);
		fp = NULL;
	}
	return ret;
}
/*
 * 698规约协议切换至3761，保存通信参数至/nor/ProTransCfg/protocol.cfg
 * */
int save_protocol_3761_tx_para(INT8U* dealdata)
{
	FILE* fp = NULL;
	int fd = 0;
	int ret = 0;
	int index = 0;
	int i = 0;
	INT16U array_num = 0;
	CLASS25 class4500;
	CLASS_4001_4002_4003 class4001;
	Protocol_Trans trans_data;
	MASTER_STATION_INFO_LIST  master;
	union
	{
		INT16U TmnlAddr_int;
		INT8U TmnlAddr_bin[2];
	}TmnlAddr;
	memset(&TmnlAddr,0,sizeof(TmnlAddr));
	memset(&class4500,0,sizeof(CLASS25));
	memset(&class4001,0,sizeof(CLASS_4001_4002_4003));
	memset(&trans_data,0,sizeof(Protocol_Trans));
	memset(&master,0,sizeof(master));

	if(access("/nor/ProTransCfg",F_OK) != 0)
	{
		system("mkdir 755 /nor/ProTransCfg");
	}

	index += getArray(&dealdata[index],(INT8U*)&array_num,NULL);

	if(master.masternum>4) {
		fprintf(stderr,"!!!!!!!!!越限 masternum=%d\n",master.masternum);
		master.masternum = 4;
	}

	for(i=0;i<array_num;i++) {
		index += getStructure(&dealdata[index],(INT8U *)&master.masternum,NULL);
		index += getOctetstring(1,&dealdata[index],master.master[i].ip,NULL);
		index += getLongUnsigned(&dealdata[index],(INT8U *)&master.master[i].port);
	}

	trans_data.trans_flg = 1;

	readCoverClass(0x4001,0,&class4001,sizeof(CLASS_4001_4002_4003),para_vari_save);
	memcpy(trans_data.AreaNo,&class4001.curstom_num[1],2);
	bcd2int32u(&class4001.curstom_num[4], 3, positive, (INT32U*)&TmnlAddr.TmnlAddr_int);

	trans_data.TmnlAddr[0] = TmnlAddr.TmnlAddr_bin[1];
	trans_data.TmnlAddr[1] = TmnlAddr.TmnlAddr_bin[0];

	readCoverClass(0x4500,0,&class4500,sizeof(CLASS25),para_vari_save);

	memcpy(trans_data.main_ip,&master.master[0].ip[1],sizeof(trans_data.main_ip));
	trans_data.main_port = master.master[0].port;

	memcpy(trans_data.bak_ip,&master.master[1].ip[1],sizeof(trans_data.bak_ip));
	trans_data.bak_port = master.master[1].port;

	memcpy(trans_data.APN,&class4500.commconfig.apn[1],sizeof(trans_data.APN));
	memcpy(trans_data.UsrName,&class4500.commconfig.userName[1],sizeof(trans_data.UsrName));
	trans_data.Len_UsrName = class4500.commconfig.userName[0];
	memcpy(trans_data.Pwd,&class4500.commconfig.passWord[1],sizeof(trans_data.Pwd));
	trans_data.Len_Pwd = class4500.commconfig.passWord[0];

	fp = fopen((const char*)PROTOCOL_TRANS_PATH,"w+");
	if(fp != NULL)
	{
		fseek(fp,0,SEEK_SET);
		ret = fwrite(&trans_data,sizeof(Protocol_Trans),1,fp);
		if(ret == 1)
		{
			fd = fileno(fp);
			fsync(fd);
		}
		else ret = 0;
	}
	if(fp != NULL)
	{
		fclose(fp);
		fp = NULL;
	}
	return ret;
}

void chg_rc_local_3761()
{
	syslog(LOG_NOTICE,"进行协议切换过程，修改rc.local启动文件...");
	system((const char *) "cp /nor/rc.d/rc.local /nor/rc.d/698_rc.local");
	sleep(1);
	if(access("/nor/rc.d/3761_rc.local",F_OK)==0) {
		system((const char *) "cp /nor/rc.d/3761_rc.local /nor/rc.d/rc.local");
		sleep(1);
		system((const char *) "chmod 777 /nor/rc.d/rc.local");
		sleep(1);
	}else {
		syslog(LOG_NOTICE,"系统无3761_rc.local备份文件，将重新生成rc.local");
		if (write_3761_rc_local()) {
			sleep(1);
			system((const char *) "chmod 777 /nor/rc.d/rc.local");
			sleep(1);
		}
	}
    if (access("/nor/rc.d/rc.local", F_OK) != 0 || access("/nor/rc.d/rc.local", X_OK) != 0) {
        if (write_3761_rc_local()) {
            sleep(1);
            system((const char *) "chmod 777 /nor/rc.d/rc.local");
            sleep(1);
        }
    }
    system("fsync -d /nor/rc.d/rc.local");
    sleep(1);
    system((const char *) "reboot");		//TODO:写文件成功切换rc.local
}

INT8U write_3761_rc_local()
{
	INT8U ret = 0;
	int fd;
	FILE* fp;
	syslog(LOG_NOTICE,"系统调用协议切换，重新生成rc.local文件");
	fp = fopen("/nor/rc.d/rc.local","w+");
	if(fp == NULL)
	{
		return ret;
	}
	fseek(fp,0,SEEK_SET);
	fprintf(fp,"echo 2048 > /proc/sys/vm/min_free_kbytes\n");
	fprintf(fp,"./etc/rc.d/mac.sh\n");
	fprintf(fp,"./etc/rc.d/ip.sh\n");
	fprintf(fp,"syslogd -l 6 -O /nand/log\n");
	fprintf(fp,"mkfifo /dev/shm/null\n");
	fprintf(fp,"tail -f /dev/shm/null>> /dev/null &\n");
	fprintf(fp,"vinit &\n");
	fprintf(fp,"vmain 2> /dev/shm/null &\n");
	fprintf(fp,"sleep 1\n");
	fprintf(fp,"vupdate &\n");
	fflush(fp);
	fd = fileno(fp);
	fsync(fd);
	if(fp != NULL)
	{
		fclose(fp);
		fp = NULL;
	}
	return 1;
}

void deloutofdatafile()//删除过期任务数据文件
{
	int i=0,taskday,fileday;
	char dirname[60];
	char 	cmdstr[100];
	TASKSET_INFO tasknor_info;
	DIR *dir;
	struct dirent *ptr;
	struct tm tm_p,tm_f;
	time_t time_s,time_p,time_f;

	time_p = time(NULL);
	localtime_r(&time_p,&tm_p);
	tm_p.tm_min = 0;
	tm_p.tm_sec = 0;
	tm_f = tm_p;
	time_p = mktime(&tm_p);
	for(i=0;i<256;i++)
	{
		memset(dirname,0x00,60);
		sprintf(dirname,"/nand/task/%03d/",i);
		if(access(dirname,F_OK)!=0)//文件不存在
			continue;
		if(ReadTaskInfo(i,&tasknor_info)==0)//得到任务信息
		{
			asyslog(LOG_INFO,"没得到任务信息,强制删除目录 %s \n",dirname);
			fprintf(stderr,"\n没得到任务信息,强制删除目录 %s \n",dirname);
			memset(cmdstr,0,100);
			sprintf((char*)cmdstr,"%s %s &", "rm -rf", dirname);
			system(cmdstr);
			continue;
		}

		taskday = (tasknor_info.memdep * tasknor_info.freq)/86400;
		if((tasknor_info.memdep * tasknor_info.freq)%86400 != 0)
			taskday++;
		if (tasknor_info.freq==0 || tasknor_info.memdep==0)
		{
			taskday = 12;
			asyslog(LOG_INFO,"\nfreq %d  memdep %d  \n",tasknor_info.freq,tasknor_info.memdep);
		}
//		fprintf(stderr,"\n\n\n\n\n----------------------------------------------------------\n");
//		asyslog(LOG_INFO,"\n[任务 %d]  执行频率 %d  任务存储深度 %d  任务存储天数 %d\n",i,tasknor_info.freq,tasknor_info.memdep,taskday);
//		fprintf(stderr,"\n[任务 %d]  任务的执行频率 %d  任务存储深度 %d  任务存储天数 %d\n",i,tasknor_info.freq,tasknor_info.memdep,taskday);

		dir = opendir(dirname);
		if(dir  == NULL)
		{
			//asyslog(LOG_INFO,"目录打开错误 %s \n",dirname);
			fprintf(stderr,"\n目录打开错误 %s \n",dirname);
			continue;
		}

		while((ptr = readdir(dir)) != NULL)
		{
	        if (strcmp(ptr->d_name, "..") == 0)
	            continue;
	        if (strcmp(ptr->d_name, ".") == 0)
	            continue;
			sscanf(ptr->d_name,"%04d%02d%02d.dat",&tm_f.tm_year,&tm_f.tm_mon,&tm_f.tm_mday);
			tm_f.tm_year -= 1900;
			tm_f.tm_mon -= 1;
			time_f = mktime(&tm_f);
			if(time_f>=time_p)
				continue;
			fprintf(stderr,"\n------- 查文件 %s     timep=%ld   timef=%ld   ",ptr->d_name, time_p ,time_f);
			fileday = (time_p-time_f)/86400;
			if((time_p-time_f)%86400 != 1)
				fileday++;
			fprintf(stderr,"\n------- 距今已 %d 天",fileday);
			if(fileday > taskday) {
				fprintf(stderr," 需要删除 !");
				sprintf(cmdstr,"/nand/task/%03d/%s",i,ptr->d_name);
				asyslog(LOG_NOTICE,"删除文件[%s]",cmdstr);
				unlink(cmdstr);
			}
		}
		if (dir!=NULL) 	closedir(dir);
	}
}
//得到抄表成功的TSA个数
INT16U getCBsuctsanum(INT8U taskid,TS ts)
{
	char fname[FILENAMELEN]={};
	INT8U taskinfoflg=0,recordbuf[2048];
	INT16U tsa_sucnum=0,seqno=0,recordnum=0;
	int i=0,headlen=0,unitlen=0,reclen=0;
	long int file_endpos= 0,file_idnexpos=0;
	FILE *fp = NULL;
	TS ts_rec;
	TASKSET_INFO tasknor_info;
	memset(&tasknor_info,0x00,sizeof(TASKSET_INFO));

	if((taskinfoflg = ReadTaskInfo(taskid,&tasknor_info))==0)//得到任务信息
	{
		asyslog(LOG_INFO,"得到任务信息失败\n");
		fprintf(stderr,"\n得到任务信息失败\n");
		return 0;
	}
	ts_rec = ts;
	ts_rec.Hour = 0;
	ts_rec.Minute = 0;
	ts_rec.Sec = 0;

	if(taskinfoflg == 2)
	{
		asyslog(LOG_INFO,"n月冻结招测\n");
		ts_rec.Day = 0;
	}
	getTaskFileName(taskid,ts_rec,fname);//得到要抄读的文件名称
	fprintf(stderr,"fname=%s\n",fname);
	seqno = (ts.Hour*60 + ts.Minute) - (tasknor_info.starthour*60 + tasknor_info.startmin);
	if(tasknor_info.freq!=0) {
		seqno = seqno/(tasknor_info.freq/60);
	}else seqno = 0;		//冻结抄读
	fprintf(stderr,"seqno=%d\n",seqno);

	fp = fopen(fname,"r");
	if(fp!=NULL)
	{
		fseek(fp,0,SEEK_END);
		file_endpos = ftell(fp);
		rewind(fp);
		ReadFileHeadLen(fp,&headlen,&unitlen);
		reclen = unitlen/tasknor_info.runtime;
		recordnum = (file_endpos-headlen)/unitlen;
		file_idnexpos = headlen+reclen*seqno;
		fprintf(stderr,"\nrecordnum=%d,reclen=%d\n",recordnum,reclen);
		for(i=0;i<recordnum;i++)
		{
			fseek(fp,file_idnexpos,SEEK_SET);
			if(fread(recordbuf,reclen,1,fp)==0)
			{
				//fprintf(stderr,"\n ## i= %d",i);
				break;
			}
			//地址和三个时标不为0，则认为此条记录不为空
			//fprintf(stderr,"\n ** recordbuf= %02x %02x %02x %02x",recordbuf[0],recordbuf[18],recordbuf[26],recordbuf[34]);
			if(recordbuf[0] != 0 && recordbuf[18] != 0 && recordbuf[26] != 0 && recordbuf[34] != 0)
				tsa_sucnum++;
			file_idnexpos += unitlen;
		}
	}
	if(fp != NULL)
		fclose(fp);
	return tsa_sucnum;
}
