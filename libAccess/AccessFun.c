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

/*
 * 脉冲计量接口数据清除
 * */
void clearClass12Data(CLASS12 *class12)
{
	class12->p = 0;
	class12->q = 0;
	class12->pluse_count = 0;
	memset(&class12->day_pos_p, 0, sizeof(class12->day_pos_p));
	memset(&class12->mon_pos_p, 0, sizeof(class12->mon_pos_p));
	memset(&class12->day_nag_p, 0, sizeof(class12->day_nag_p));
	memset(&class12->mon_nag_p, 0, sizeof(class12->mon_nag_p));

	memset(&class12->day_pos_q, 0, sizeof(class12->day_pos_p));
	memset(&class12->mon_pos_q, 0, sizeof(class12->mon_pos_p));
	memset(&class12->day_nag_q, 0, sizeof(class12->day_nag_p));
	memset(&class12->mon_nag_q, 0, sizeof(class12->mon_nag_p));

	memset(&class12->val_pos_p, 0, sizeof(class12->val_pos_p));
	memset(&class12->val_nag_p, 0, sizeof(class12->val_nag_p));
	memset(&class12->val_pos_q, 0, sizeof(class12->val_pos_q));
	memset(&class12->val_nag_q, 0, sizeof(class12->val_nag_q));
}

/*
 * 总加组类数据清除
 * */
void clearClass23Data(CLASS23 *class23)
{
	INT8U i=0;
	for(i=0;i<MAX_AL_UNIT;i++) {
		memset(class23->allist[i].curP,0,sizeof(class23->allist[i].curP));
		memset(class23->allist[i].curQ,0,sizeof(class23->allist[i].curQ));
	}
	class23->p = 0;
	class23->q = 0;
	class23->TaveP = 0;
	class23->TaveQ = 0;
	class23->DayPALL = 0;
	memset(&class23->DayP, 0, sizeof(class23->DayP));
	class23->DayQALL = 0;
	memset(&class23->DayQ, 0, sizeof(class23->DayQ));
	class23->MonthPALL = 0;
	memset(class23->MonthP, 0, sizeof(class23->MonthP));
	class23->MonthQALL = 0;
	memset(class23->MonthQ, 0, sizeof(class23->MonthQ));
	class23->remains = 0;
	class23->DownFreeze = 0;
}

/*
 * 清除总加组及脉冲计量
 * */
void clearControlData()
{
	syslog(LOG_NOTICE,"__%s__",__func__);
	CLASS12			class12={};			//脉冲计量
	CLASS23			class23={};			//总加组
	int		ret = 0;
	OI_698	oi=0;

	//清除总加组数据
	for(oi=0x2301;oi<=0x2308;oi++) {
		ret = readCoverClass(oi, 0, &class23, sizeof(CLASS23), para_vari_save);
		if(ret!=-1) {
			clearClass23Data(&class23);
			saveCoverClass(oi, 0, &class23, sizeof(CLASS23), para_vari_save);
		}
	}
	//清除脉冲计量数据
	for(oi=0x2401;oi<=0x2408;oi++) {
		ret = readCoverClass(oi, 0, &class12, sizeof(CLASS12), para_vari_save);
		if(ret!=-1) {
			clearClass12Data(&class12);
			fprintf(stderr,"class12 pulse = %d\n",class12.pluse_count);
			saveCoverClass(oi, 0, &class12, sizeof(CLASS12), para_vari_save);
		}
	}
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
	//删除控制文件数据
	clearControlData();
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
						setOIChange(event_class_len[i].oi);
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
		sleep(3);		//延时保证能争取删除参数并通知进程初始化参数
		InItClass(0);
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
	syslog(LOG_NOTICE,"##### infoi=%d oi=%04x",infoi,oi);
	if(infoi==-1) {
		memset(cmd,0,sizeof(cmd));
		oiA1 = (oi & 0xf000) >> 12;
		syslog(LOG_NOTICE,"##### infoi=%d oi=%04x,oiA1=%d",infoi,oi,oiA1);
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
			return ret;
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
INT16U CalcMinFromZero(INT8U hour,INT8U min)
{
	INT16U minfromzero = 0;
	minfromzero = hour;
	minfromzero = minfromzero*60 +min;
	return minfromzero;
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
void intToBuf(int value,INT8U *buf)
{
	buf[0] = value&0x00ff;
	buf[1] = (value>>8) & 0x0ff;
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
		index += getOctetstring(1,OCTET_STRING_LEN-1,&dealdata[index],&master.master[i].ip[1],&master.master[i].ip[0],NULL);
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
INT8U fillclass23data(OAD oad_m,OAD oad_r,TSA meter,INT8U* data,ProgramInfo* JProgramInfo)
{
	fprintf(stderr,"\n\n ---------------fillclass23data");
	INT8U ret = 0;
	INT8U meterIndex = 0;
	INT8U groupIndex = 0;

	for(groupIndex = 0;groupIndex < 8;groupIndex++)
	{
		for(meterIndex = 0;meterIndex < MAX_AL_UNIT;meterIndex++)
		{
			if(JProgramInfo->class23[groupIndex].allist[meterIndex].tsa.addr[0]==0)
				break;
			if(memcmp(&meter,&JProgramInfo->class23[groupIndex].allist[meterIndex].tsa,(meter.addr[0]+1))==0)
			{
				if(oad_r.attrindex == 0)
				{
					data = &data[2];
					INT8U rateIndex = 0;
					for(rateIndex = 0;rateIndex < MAXVAL_RATENUM+1;rateIndex++)
					{
						INT32U dianliang = (data[rateIndex*5+1]<<24)+(data[rateIndex*5+2]<<16)+(data[rateIndex*5+3]<<8)+data[rateIndex*5+4];
						dianliang = dianliang*100;
						switch(oad_m.OI)
						{
							//日冻结
							case 0x5004:
								if(oad_r.OI == 0x0010)
								{
//									JProgramInfo->class23[groupIndex].allist[meterIndex].freeze[0][rateIndex] = dianliang;
								}
								if(oad_r.OI == 0x0020)
								{
//									JProgramInfo->class23[groupIndex].allist[meterIndex].freeze[1][rateIndex] = dianliang;
								}
								break;
							//月冻结
							case 0x5006:
								if(oad_r.OI == 0x0010)
								{
//									JProgramInfo->class23[groupIndex].allist[meterIndex].freeze[2][rateIndex] = dianliang;
								}
								if(oad_r.OI == 0x0020)
								{
//									JProgramInfo->class23[groupIndex].allist[meterIndex].freeze[3][rateIndex] = dianliang;
								}
								break;
							default:
								if(oad_r.OI == 0x0010)
								{
									JProgramInfo->class23[groupIndex].allist[meterIndex].curP[rateIndex] = dianliang;
								}
								if(oad_r.OI == 0x0020)
								{
									JProgramInfo->class23[groupIndex].allist[meterIndex].curNP[rateIndex] = dianliang;
								}
								if(oad_r.OI == 0x0030)
								{
									JProgramInfo->class23[groupIndex].allist[meterIndex].curQ[rateIndex] = dianliang;
								}
								if(oad_r.OI == 0x0040)
								{
									JProgramInfo->class23[groupIndex].allist[meterIndex].curNQ[rateIndex] = dianliang;
								}
						}
					}
				}
				else if(oad_r.attrindex == 1)
				{
					INT32U dianliang = (data[1]<<24)+(data[2]<<16)+(data[3]<<8)+data[4];
					dianliang = dianliang*100;
					if(oad_r.OI == 0x0010)
					{
						JProgramInfo->class23[groupIndex].allist[meterIndex].curP[0] = dianliang;
					}
					if(oad_r.OI == 0x0020)
					{
						JProgramInfo->class23[groupIndex].allist[meterIndex].curNP[0] = dianliang;
					}
					if(oad_r.OI == 0x0030)
					{
						JProgramInfo->class23[groupIndex].allist[meterIndex].curQ[0] = dianliang;
					}
					if(oad_r.OI == 0x0040)
					{
						JProgramInfo->class23[groupIndex].allist[meterIndex].curNQ[0] = dianliang;
					}
				}
				return 1;
			}
		}
	}
	return ret;
}
///////////////////////////////////////////////////////////////////////////存储相关函数
void prtTSA(TSA tsa)
{
	int i = 0;
	fprintf(stderr,"\nTSA[%02x]:",tsa.addr[0]);
	for(i=0;i<tsa.addr[0];i++)
		fprintf(stderr," %02x",tsa.addr[1+i]);
	fprintf(stderr,"\n");
}
void PRTbuf(INT8U *buf,INT16U buflen)
{
	int i=0;
	for(i=0;i<buflen;i++)
		fprintf(stderr," %02x",buf[i]);
	fprintf(stderr,"\n");
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
//					fprintf(stderr,"\nTSA: %d-",meter.basicinfo.addr.addr[0]);
					for(j=0;j<meter.basicinfo.addr.addr[0];j++) {
//						fprintf(stderr,"-%02x",meter.basicinfo.addr.addr[j+1]);
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
//					fprintf(stderr,"\nms.mstype = %d,tsa_num = %d",ms.mstype,tsa_num);
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
//	fprintf(stderr," tsas  p=%p record_num=%d  tsa_len=%d",*tsas,record_num,tsa_len);
	tsa_num = 0;
	for(i=0;i<record_num;i++) {
		if(readParaClass(0x6000,&meter,i)==1) {
			if(meter.sernum!=0 && meter.sernum!=0xffff) {
				switch(ms.mstype) {
				case 1:	//全部用户地址
					fprintf(stderr,"\nTSA: %d-",meter.basicinfo.addr.addr[0]);
					for(j=0;j<meter.basicinfo.addr.addr[0];j++) {
//						fprintf(stderr,"-%02x",meter.basicinfo.addr.addr[j+1]);
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
//	fprintf(stderr,"\noi=%04x\n",oi);
//	fprintf(stderr,"\n-----case %d\n",((oi&0xf000)>>12));
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
		oi_info->io_unit = 1;//array
		oi_info->mem_unit = 2;//数组
		oi_info->mem_num = 2;
		oi_info->oi_mem[0].mem_len = 4;
		oi_info->oi_mem[0].mem_chg = 14;//-4

		oi_info->oi_mem[1].mem_unit = 28;
		oi_info->oi_mem[1].mem_len = 7;
		oi_info->oi_mem[1].mem_chg = 0;
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

			oi_info->io_unit = 1;//array
			oi_info->mem_unit = 2;//数组
			oi_info->mem_num = 2;

			oi_info->oi_mem[0].mem_unit = 6;
			oi_info->oi_mem[0].mem_len = 4;
			oi_info->oi_mem[0].mem_chg = 14;

			oi_info->oi_mem[1].mem_unit = 28;
			oi_info->oi_mem[1].mem_len = 7;
			oi_info->oi_mem[1].mem_chg = 0;
			break;
			////////////////////////////////////////////////////////ic = 3
		case 0x2001:
			oi_info->ic = 3;
			oi_info->oinum = 4;//三相加零序
			oi_info->io_unit = 1;
			oi_info->oi_mem[0].mem_chg = 13;//-3
			oi_info->oi_mem[0].mem_len = 4;
			oi_info->oi_mem[0].mem_unit = 5;
			break;
		case 0x2000:
		case 0x2002:
		case 0x2003:
			oi_info->ic = 3;
			oi_info->oinum = 3;//三相
			oi_info->io_unit = 1;
			oi_info->oi_mem[0].mem_chg = 11;//-1
			oi_info->oi_mem[0].mem_len = 2;
			oi_info->oi_mem[0].mem_unit = 18;
			break;
		case 0x200b:
		case 0x200c:
			oi_info->ic = 3;
			oi_info->oinum = 3;//三相
			oi_info->io_unit = 1;
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
			oi_info->io_unit = 1;
			oi_info->oi_mem[0].mem_chg = 11;//-1
			oi_info->oi_mem[0].mem_len = 4;
			oi_info->oi_mem[0].mem_unit = 5;
			break;
		case 0x200a:
			oi_info->ic = 4;
			oi_info->oinum = 4;//总及分相
			oi_info->io_unit = 1;
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
			oi_info->mem_num = 1;
			oi_info->oi_mem[0].mem_chg = 0;//
			oi_info->oi_mem[0].mem_len = 3;
			oi_info->oi_mem[0].mem_unit = 4;
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
			oi_info->io_unit = 1;
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
			oi_info->mem_num = 5;

			oi_info->oi_mem[0].mem_unit = 6;
			oi_info->oi_mem[0].mem_len = 4;
			oi_info->oi_mem[0].mem_chg = 0;

			oi_info->oi_mem[1].mem_unit = 18;
			oi_info->oi_mem[1].mem_len = 2;
			oi_info->oi_mem[1].mem_chg = 12;

			oi_info->oi_mem[2].mem_unit = 18;
			oi_info->oi_mem[2].mem_len = 2;
			oi_info->oi_mem[2].mem_chg = 12;

			oi_info->oi_mem[3].mem_unit = 6;
			oi_info->oi_mem[3].mem_len = 4;
			oi_info->oi_mem[3].mem_chg = 0;

			oi_info->oi_mem[4].mem_unit = 6;
			oi_info->oi_mem[4].mem_len = 4;
			oi_info->oi_mem[4].mem_chg = 0;
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
/*
 * 计算某个OI的数据长度，指针对抄表数据 todo 先写个简单的，以后完善 而且没有考虑费率
 * attr_flg:0 全部属性 非0 一个属性  例如20000200 则为全部属性 20000201则为一个属性
 */
INT16U CalcOIDataLen(OAD oad)
{
	INT16U oi_len = 0,len_tmp=0;
	int i=0;
	OI_INFO oi_info;

	if(oad.OI == 0x2001 && oad.attflg == 0x04 && oad.attrindex == 00)
		return 5;

	if(oad.attflg == 4 && oad.OI>=0x0000 && oad.OI<0x1000) {//临时处理高精度
		if(oad.attrindex == 0)
			return 47;//长度4+1个字节数据类型  +2: array 01 + 长度
		else
			return 9;
	}

	GetOIinfo(oad.OI,4,&oi_info);

//	fprintf(stderr,"\n---------------oi_info.io_unit=%d\n",oi_info.io_unit);
	if(oad.attrindex == 0)
	{
		switch(oi_info.io_unit)
		{
		case 1://array 类型加个数
			if(oi_info.mem_unit == 2)
			{
				fprintf(stderr,"\n数组里面包含结构体\n");
				oi_len = 2;
				for(i=0;i<oi_info.mem_num;i++)
					len_tmp += oi_info.oi_mem[i].mem_len+1;
				oi_len += oi_info.oinum*(2+len_tmp);
			}
			else
				oi_len = 2 + oi_info.oinum*(oi_info.oi_mem[0].mem_len+1);
			break;
		case 2://struct
			oi_len = 2;//类型加个数
			for(i=0;i<oi_info.mem_num;i++)
			{
				oi_len += oi_info.oi_mem[i].mem_len+1;
			}
			if(oad.OI == 0x2131 || oad.OI == 0x2132 || oad.OI == 0x2133)
			{
				oi_len *= 2;//02 02   02 01 ...  02 01...
				oi_len += 2;
			}
			break;
		default:
			oi_len = oi_info.oi_mem[0].mem_len+1;
			break;
		}
	}
	else
	{
		fprintf(stderr,"\noi_info.io_unit=%d oi=%04x\n",oi_info.io_unit,oad.OI);
		if(oi_info.io_unit == 2 && (oad.OI == 0x2131 || oad.OI == 0x2132 || oad.OI == 0x2133))
		{
			fprintf(stderr,"\n1oi_info.mem_num=%d\n",oi_info.mem_num);
			oi_len = 2;//类型加个数
			for(i=0;i<oi_info.mem_num;i++)
			{
				oi_len += oi_info.oi_mem[i].mem_len+1;
			}
		}
		else if(oi_info.mem_unit == 2) {//数组里面有结构体
			fprintf(stderr,"\n2oi_info.mem_num=%d\n",oi_info.mem_num);
			oi_len = 2;//类型加个数
			for(i=0;i<oi_info.mem_num;i++)
				oi_len += oi_info.oi_mem[i].mem_len+1;
			} else
				oi_len = oi_info.oi_mem[0].mem_len+1;
	}
//	fprintf(stderr,"\n0x%04x-02%02x计算的长度为%d\n",oad.OI,oad.attrindex,oi_len);
	return oi_len;
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
//	fprintf(stderr,"-------oadmr_num=%d,unitnum=%d\n",item_road.oadmr_num,unitnum);
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
					item_road.oad[i].oad_r.attflg == head_unit[j].oad_r.attflg &&
					item_road.oad[i].oad_r.OI == head_unit[j].oad_r.OI)
			{
//				fprintf(stderr,"\nfind oad %04x%02x%02x-%04x%02x%02x:offset:%d--head:%04x%02x%02x-%04x%02x%02x\n",
//						item_road.oad[i].oad_m.OI,item_road.oad[i].oad_m.attflg,item_road.oad[i].oad_m.attrindex,
//						item_road.oad[i].oad_r.OI,item_road.oad[i].oad_r.attflg,item_road.oad[i].oad_r.attrindex,
//						datapos,
//						head_unit[j].oad_m.OI,head_unit[j].oad_m.attflg,head_unit[j].oad_m.attrindex,
//						head_unit[j].oad_r.OI,head_unit[j].oad_r.attflg,head_unit[j].oad_r.attrindex);
				if(item_road.oad[i].oad_r.attrindex != 0 && head_unit[j].oad_r.attrindex == 0)//招测某一项
				{
					INT16U oadlen = CalcOIDataLen(item_road.oad[i].oad_r);
					oad_offset[i].offset = datapos + (item_road.oad[i].oad_r.attrindex-1)*oadlen +2;
					oad_offset[i].len = oadlen;
//					fprintf(stderr,"\n招测某一项oadlen=%d\n",oadlen);
				}
				else if(item_road.oad[i].oad_r.attrindex == head_unit[j].oad_r.attrindex)
				{
//					fprintf(stderr,"\n招测所有项\n");
					oad_offset[i].offset = datapos;
					oad_offset[i].len = head_unit[j].len;
//					fprintf(stderr,"\n招测所有oadlen=%d\n",oad_offset[i].len);
				}
				else
				{
					fprintf(stderr,"\n招测没有项\n");
					datapos += head_unit[j].len;
				}
			}
			else {
				datapos += head_unit[j].len;
//				fprintf(stderr,"datapos = %d\n",datapos);
			}
		}
	}
}

//int getTItoSec(TI ti)
//{
//	int  sec = 0;
//	switch(ti.units)
//	{
//		case sec_units://秒
//			sec = ti.interval;
//			break;
//		case minute_units://分
//			sec = ti.interval * 60;
//			break;
//		case hour_units://时
//			sec =  ti.interval * 3600;
//			break;
//		default:
//			break;
//	}
////	fprintf(stderr,"get TI(%d-%d) sec=%d\n",ti.units,ti.interval,sec);
//	return sec;
//}
INT32U getTASKruntimes(CLASS_6013 class6013,CLASS_6015 class6015,INT32U *seqsec)//计算任务每天抄读次数
{
	INT32U runtimes=0,seqsecond=0,taskdaysec=0;//频率秒数 任务一天的活跃秒数
	TI ti_tmp;
//	tasknor_info->starthour = class6013.runtime.runtime[0].beginHour;
//	tasknor_info->startmin = class6013.runtime.runtime[0].beginMin;//按照设置一个时段来
//	tasknor_info->endhour = class6013.runtime.runtime[0].endHour;
//	tasknor_info->endmin = class6013.runtime.runtime[0].endMin;//按照设置一个时段来
	taskdaysec = (class6013.runtime.runtime[0].endHour*3600+class6013.runtime.runtime[0].endMin*60) -
			(class6013.runtime.runtime[0].beginHour*3600+class6013.runtime.runtime[0].beginMin*60);
	fprintf(stderr,"\n6013end:%d:%d 6013start:%d:%d\n",
			class6013.runtime.runtime[0].endHour,class6013.runtime.runtime[0].endMin,
			class6013.runtime.runtime[0].beginHour,class6013.runtime.runtime[0].beginMin);
	fprintf(stderr,"\ntaskdaysec=%d\n",taskdaysec);
	if(taskdaysec<=0)
		return 0;//任务设置不合理
	fprintf(stderr,"\n--------------cjtype=%d\n",class6015.cjtype);
	switch(class6015.cjtype)
	{
	case 0://当前数据 存储次数由任务执行频率决定
	case 1:
	case 2:
		seqsecond = TItoSec(class6013.interval);
		if(seqsecond==0)
			runtimes = 1;
		else
			runtimes = taskdaysec/seqsecond+1;
		break;
//	case 1://采集上n次
////		runtimes=class6015.data.data[1];//data[0]应该是类型？
////		break;
////	case 2://按冻结时标采集
//		runtimes = 1;
//		break;
	case 3://按时标间隔采集
		ti_tmp.interval = (class6015.data.data[1]<<8) + class6015.data.data[2];
		ti_tmp.units = class6015.data.data[0];
		seqsecond = TItoSec(ti_tmp);
		if(seqsecond==0)
			runtimes = 1;
		else
			runtimes = taskdaysec/seqsecond+1;
		break;
	case 4://补抄
		break;
	default:
		runtimes = 1;
		break;
	}
	if(seqsecond==0)
		seqsecond = taskdaysec;
	if(runtimes > class6015.deepsize && runtimes == 1)
	{
		runtimes = class6015.deepsize;
		seqsecond = 24*60*60;//1天一次
	}
	*seqsec = seqsecond;
	fprintf(stderr,"\n*seqsec =%d seqsecond =%d\n",*seqsec,seqsecond);
	return runtimes;
}

INT8U get60136015info(INT8U taskid,CLASS_6015 *class6015,CLASS_6013 *class6013)
{
	int i=0;
	memset(class6013,0,sizeof(CLASS_6013));
	memset(class6015,0,sizeof(CLASS_6015));
	if(readCoverClass(0x6013,taskid,class6013,sizeof(CLASS_6013),coll_para_save) == 1)
	{
		if(class6013->cjtype != 1 || class6013->state != 1)//
		{
			fprintf(stderr,"\ncjtype=%d 任务state=%d\n",class6013->cjtype,class6013->state);
			return 0;
		}
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
	{
		fprintf(stderr,"\n读取任务%d失败\n",taskid);
		return 0;
	}
	return 4;//实时或曲线
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
INT8U cmpTSAtype(CLASS_6001 *tsa,CLASS_6015	class6015)
{
	int kk=0;
	if(tsa != NULL)
	{
		fprintf(stderr,"\n  metertype=%d,tsa.usrtype=%d\n",class6015.mst.mstype,tsa[0].basicinfo.usrtype);
		prtTSA(tsa[0].basicinfo.addr);
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
	else
	{
		fprintf(stderr,"\ntsa null\n");
	}
	return 0;
}
//oad在这个任务里，则返回1，否则0
INT8U cmpOADintask(OAD oad_m,OAD oad_r,CLASS_6015 class6015)
{
	int j=0,mm=0,nn=0;
	for(j=0;j<class6015.csds.num;j++)
	{
		switch(class6015.csds.csd[j].type)
		{
		case 0:
			if(oad_m.OI == 0x0000 ||
					(oad_m.OI <= 0x5002 && oad_m.OI >= 0x5000)) 	//深度查找满足的任务oad
			{
				if(memcmp(&oad_r,&class6015.csds.csd[j].csd.oad,sizeof(OAD))==0 ||
						(oad_r.OI == class6015.csds.csd[j].csd.oad.OI &&
								oad_r.attrindex != 0 &&
								class6015.csds.csd[j].csd.oad.attrindex == 0)){
					return 1;
				}
			}
			break;
		case 1:
			if(memcmp(&oad_m,&class6015.csds.csd[j].csd.road.oad,sizeof(OAD))==0)//
			{
				for(nn=0;nn<class6015.csds.csd[j].csd.road.num;nn++)
				{
					fprintf(stderr,"oad_r=%04x%02x%02x %d.oad=%04x%02x%02x\n",
							oad_r.OI,oad_r.attflg,oad_r.attrindex,nn,
							class6015.csds.csd[j].csd.road.oads[nn].OI,class6015.csds.csd[j].csd.road.oads[nn].attflg,class6015.csds.csd[j].csd.road.oads[nn].attrindex);
					if(memcmp(&oad_r,&class6015.csds.csd[j].csd.road.oads[nn],sizeof(OAD))==0 ||
							(oad_r.OI == class6015.csds.csd[j].csd.road.oads[nn].OI &&
								oad_r.attrindex != 0 &&
								class6015.csds.csd[j].csd.road.oads[nn].attrindex == 0)){
						return 1;
					}
				}
			}
			break;
		default:break;
		}
	}
	return 0;
}

/*
 * 根据csds得到任务号
 */
INT8U GetTaskidFromCSDs(ROAD_ITEM item_road,CLASS_6001 *tsa)
{
	CLASS_6015	class6015={};
	CLASS_6013	class6013={};
	int i=0,j=0,mm=0,nn=0;
	INT8U taskno=0,taskid=0;
	INT32U seqsec=0,seqnum=0;

	memset(&class6013,0,sizeof(CLASS_6013));
	memset(&class6015,0,sizeof(CLASS_6015));
	for(i=0;i<256;i++)
	{
		fprintf(stderr,"\n查找任务%d\n",i+1);
		if(readCoverClass(0x6013,i+1,&class6013,sizeof(class6013),coll_para_save) == 1)
		{
			if(class6013.cjtype != 1 || class6013.state != 1)//过滤掉不是普通采集方案的
			{
				fprintf(stderr,"\n非普通方案\n");
				continue;
			}
			if(readCoverClass(0x6015,class6013.sernum,&class6015,sizeof(CLASS_6015),coll_para_save) == 1)
			{
				seqnum = getTASKruntimes(class6013,class6015,&seqsec);
				if(item_road.zc_seqsec != 0 && item_road.zc_seqsec < seqsec)//
				{
					fprintf(stderr,"\nitem_road.zc_seqsec=%d,seqsec=%d\n",item_road.zc_seqsec,seqsec);
					continue;
				}
				if(cmpTSAtype(tsa,class6015)==0)//比对tsa类型，不符和本采集方案的跳过
				{
					fprintf(stderr,"\ntsa不符和\n");
					continue;
				}
				else
					fprintf(stderr,"\ntsa_equ=1,taskid=%d\n",i+1);
				for(j=0;j<class6015.csds.num;j++)
				{
					fprintf(stderr,"\nzc:%04x%02x%02x\n",
						class6015.csds.csd[j].csd.oad.OI,class6015.csds.csd[j].csd.oad.attflg,class6015.csds.csd[j].csd.oad.attrindex);
					for(mm=0;mm<item_road.oadmr_num;mm++)
					{
						switch(class6015.csds.csd[j].type)
						{
						case 0:
							fprintf(stderr,"\nzc:%04x%02x%02x %04x%02x%02x\n",
									item_road.oad[mm].oad_m.OI,item_road.oad[mm].oad_m.attflg,item_road.oad[mm].oad_m.attrindex,
									item_road.oad[mm].oad_r.OI,item_road.oad[mm].oad_r.attflg,item_road.oad[mm].oad_r.attrindex);
							if(item_road.oad[mm].oad_r.OI >= 0x9000)//无效数据
							{
								item_road.oad[mm].taskid = 0;
								continue;
							}
							if(item_road.oad[mm].oad_m.OI == 0x0000 ||
									(item_road.oad[mm].oad_m.OI <= 0x5002 && item_road.oad[mm].oad_m.OI >= 0x5000)) 	//深度查找满足的任务oad
							{
								if(memcmp(&item_road.oad[mm].oad_r,&class6015.csds.csd[j].csd.oad,sizeof(OAD))==0 ||
										(item_road.oad[mm].oad_r.OI == class6015.csds.csd[j].csd.oad.OI &&
												item_road.oad[mm].oad_r.attrindex != 0 &&
												class6015.csds.csd[j].csd.oad.attrindex == 0)){
									item_road.oad[mm].taskid = i+1;
									continue;
								}
							}
							break;
						case 1:
							if(memcmp(&item_road.oad[mm].oad_m,&class6015.csds.csd[j].csd.road.oad,sizeof(OAD))==0)//
							{
								for(nn=0;nn<class6015.csds.csd[j].csd.road.num;nn++)
								{
									fprintf(stderr,"oad_r=%04x%02x%02x %d.oad=%04x%02x%02x\n",
											item_road.oad[mm].oad_r.OI,item_road.oad[mm].oad_r.attflg,item_road.oad[mm].oad_r.attrindex,nn,
											class6015.csds.csd[j].csd.road.oads[nn].OI,class6015.csds.csd[j].csd.road.oads[nn].attflg,class6015.csds.csd[j].csd.road.oads[nn].attrindex);
									if(item_road.oad[mm].oad_r.OI >= 0x9000)//无效数据
									{
										item_road.oad[mm].taskid = 0;
										continue;
									}
									if(memcmp(&item_road.oad[mm].oad_r,&class6015.csds.csd[j].csd.road.oads[nn],sizeof(OAD))==0 ||
											(item_road.oad[mm].oad_r.OI == class6015.csds.csd[j].csd.road.oads[nn].OI &&
												item_road.oad[mm].oad_r.attrindex != 0 &&
												class6015.csds.csd[j].csd.road.oads[nn].attrindex == 0)){
										item_road.oad[mm].taskid = i+1;
										fprintf(stderr,"\n------find \n");
										continue;
									}
								}
							}
							break;
						default:break;
						}
					}
				}
				for(mm=0;mm<(item_road.oadmr_num);mm++)
				{
					fprintf(stderr,"=====0====taskno=%d oad=%04x%02x%02x taskid=%d",
							taskno,item_road.oad[mm].oad_r.OI,item_road.oad[mm].oad_r.attflg,item_road.oad[mm].oad_r.attrindex,
							item_road.oad[mm].taskid);
					if(item_road.oad[mm].oad_r.OI == 0x202a || item_road.oad[mm].oad_r.OI == 0x6040 ||
							item_road.oad[mm].oad_r.OI == 0x6041 || item_road.oad[mm].oad_r.OI == 0x6042 ||
							(item_road.oad[mm].oad_r.OI >= 0x9000 && item_road.oad[mm].oad_r.OI <= 0xf000))
						continue;
					taskno = item_road.oad[mm].taskid;
					if(taskno == 0 || taskno != item_road.oad[mm].taskid)
						break;
				}
				if(taskno != 0)
				{
					asyslog(LOG_INFO,"return  ,taskno=%d\n",taskno);
					taskid = taskno;
					if(class6015.mst.mstype == 1)
						return taskid;
					else
						continue;
					return taskno;
				}
				else
					fprintf(stderr,"\n====1===taskno=%d \n",taskno);
			}
		}
	}
	return taskno;
}
INT16U getrecdata(INT8U *recorddata,TSA tsa,ROAD_ITEM item_road,OAD_INDEX *oad_offset,INT8U *databuf)
{
	INT16U pindex=0,oadlen=0,retlen=0;
	INT8U tmpbuf[256];
	int i=0,j=0;
	OAD oad_tmp;


	fprintf(stderr,"\nitem_road.oadmr_num=%d\n",item_road.oadmr_num);
	memset(tmpbuf,0x00,sizeof(tmpbuf));
	for(i=0;i<item_road.oadmr_num;i++)
	{
//		fprintf(stderr,"\n%d:oi:%04x-%04x::off:%d-len:%d\n",i,item_road.oad[i].oad_r.OI,item_road.oad[i].oad_r.OI,oad_offset[i].offset,oad_offset[i].len);
		if(item_road.oad[i].oad_r.OI == 0x202a)
		{
			databuf[pindex++] = 0x55;
			memcpy(&databuf[pindex],tsa.addr,tsa.addr[0]+1);
			pindex += tsa.addr[0]+1;
			continue;
		}
		if(item_road.oad[i].oad_num != 0)//road格式需要写出oad个数
		{
			databuf[pindex++] = 0x01;
			databuf[pindex++] = item_road.oad[i].oad_num;
		}
		if(oad_offset[i].len == 0)//没找到
		{
			fprintf(stderr,"\n无数据\n");
			databuf[pindex++] = 0;
		}
		else
		{
			memcpy(tmpbuf,&recorddata[oad_offset[i].offset],oad_offset[i].len);
			switch(tmpbuf[0])
			{
			fprintf(stderr,"\noad类型tmpbuf[0]=%d\n",tmpbuf[0]);
			case 0:
				if(getZone("HuNan")==0 && tmpbuf[0] == 0 && item_road.oad[i].oad_r.attrindex == 0)
				{
					OI_INFO oi_info;
					GetOIinfo(item_road.oad[i].oad_r.OI,4,&oi_info);
					fprintf(stderr,"\n地区：湖南  %d\n",oi_info.oinum);
					if(oi_info.oinum>1)//array类型
					{
						databuf[pindex++] = 1;
						databuf[pindex++] = oi_info.oinum;
						for(j=0;j<oi_info.oinum;j++)
							databuf[pindex++] = 0;
						break;
					}
				}
				databuf[pindex++] = 0;
				break;
			case 1://array
//				oadlen = CalcOIDataLen(oad_offset[i].oad_r.OI,oad_offset[i].oad_r.attrindex);
//				fprintf(stderr,"\narray---oadlen=%d\n",oadlen);
////				oadlen = oadlen*tmpbuf[1]+2;//2代表一个array类型加一个个数
//				memcpy(&databuf[pindex],tmpbuf,oadlen);
//				pindex += oadlen;
//				break;
				memcpy(&databuf[pindex],tmpbuf,2);
				pindex += 2;
				oad_tmp = oad_offset[i].oad_r;
				oad_tmp.attrindex = 1;
				retlen = CalcOIDataLen(oad_tmp);
//					fprintf(stderr,"\n元素个数%d\n",tmpbuf[1]);
				for(j=0;j<tmpbuf[1];j++)
				{
//						fprintf(stderr,"\n类型(%d)-%d  长度%d\n",retlen*i+2,tmpbuf[retlen*i+2],retlen);
					if(tmpbuf[retlen*j+2] == 0)
						databuf[pindex++] = 0;
					else
					{
						memcpy(&databuf[pindex],&tmpbuf[j*retlen+2],retlen);
						pindex += retlen;
					}
				}
				break;
//			case 2://struct 暂时不处理
//				break;
			case 0x55:	//TSA
				memcpy(&databuf[pindex],tmpbuf,(tmpbuf[1]+2));
				pindex += (tmpbuf[1]+2);
				break;
			default:
				memcpy(&databuf[pindex],tmpbuf,oad_offset[i].len);
				pindex += oad_offset[i].len;
				break;
			}
		}
//		fprintf(stderr,"\n组数据(%d):",pindex);
//		for(j=0;j<pindex;j++)
//			fprintf(stderr," %02x",databuf[j]);
//		fprintf(stderr,"\n");
	}
//	fprintf(stderr,"\n上报数据:");
//	for(i=0;i<pindex;i++)
//		fprintf(stderr," %02x",databuf[i]);
//	fprintf(stderr,"\n");
	return pindex;
}
int initFrameHead(INT8U *buf,OAD oad,CSD_ARRAYTYPE csds,INT8U *seqnumindex)
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
void saveOneFrame(INT8U *buf,int len,FILE *fp)
{
	int  fd=0,i=0;
	fprintf(stderr,"\n存储帧：");
	for(i=0;i<len;i++)
		fprintf(stderr," %02x",buf[i]);
	if(fp != NULL)
	{
		fwrite(buf,len,1,fp);
		fd = fileno(fp);
		fsync(fd);
	}
	else
		fprintf(stderr,"\nfrmdata 文件NULL\n");
	return ;
}
void saveOneFrame1(INT8U *buf,int len)
{
	int  fd=0,i=0,savecount=0;
	char filename[FILENAMELEN];
	FILE *fp = NULL;

	memset(filename,0x00,FILENAMELEN);

	sprintf(filename,"%s",TASK_FRAME_DATA);
	if (access(filename,0)==0)
	{
		fprintf(stderr,"\n创建文件\n");
		fp = fopen(filename,"w");
		fclose(fp);
	}
	fp = fopen(filename,"a+");
	if(fp != NULL)
	{
//		fprintf(stderr,"\n附加存储(%d):",len);
//		for(i=0;i<len;i++)
//			fprintf(stderr," %02x",buf[i]);
//		fprintf(stderr,"\n");

		savecount = fwrite(buf,len,1,fp);
		fprintf(stderr,"\nsavelen=%d\n",savecount);
		fd = fileno(fp);
		fsync(fd);
		fclose(fp);
	}
	else
		fprintf(stderr,"\n帧文件流 NULL\n");
}
FILE* openFramefile(char *filename,int addflg)
{
	FILE *fp = NULL;

	if (access(filename,0)==0 && addflg == 0)
	{
		fp = fopen(filename,"w");
		fclose(fp);
	}
	fp = fopen(filename,"a+");
	return fp;
}

//fp为可读的文件流
//找到tsa在文件中的位置，以块个数表示 位置记录到没有用的pt上,即tsa_group[0].extinfo.pt
INT16U getTSAblkoffnum(CLASS_6001 *tsa_group,INT16U tsa_num,INT32U blklen,INT16U headlen,FILE *fp)
{
	INT8U tsadata[18];
	INT16U tsa_offnum = 1;
	INT16U tsa_findnum = 0;
	int i =0;
	if(fp == NULL)//文件不存在，返回空
	{
		fprintf(stderr,"\n计算tsapos打开文件失败!!!\n");
		return 0;
	}
	for(i=0;i<tsa_num;i++)
	{
		tsa_group[i].extinfo.pt = 0;
	}
	fseek(fp,headlen,SEEK_SET);
	while(!feof(fp))
	{
//		fprintf(stderr,"\n文件位置%d blklen=%d\n",ftell(fp),blklen);
		if(fread(tsadata,sizeof(TSA)+1,1,fp)==0)//0x55 + tsa
		{
			fprintf(stderr,"\n文件尾\n");
			break;
		}
		for(i=0;i<tsa_num;i++)
		{
//			fprintf(stderr,"\ni=%d\n",i);
//			PRTbuf(&tsadata[1],TSA_LEN);
//			PRTbuf(tsa_group[i].basicinfo.addr.addr,TSA_LEN);
			if(memcmp(&tsadata[1],tsa_group[i].basicinfo.addr.addr,tsadata[1]+1)==0 && tsadata[0] == 0x55)
			{
				tsa_group[i].extinfo.pt = tsa_offnum;
				tsa_findnum++;
				if(tsa_findnum == tsa_num)//找全了
				{
					fprintf(stderr,"\n找全了%d\n",tsa_findnum);
					return tsa_findnum;
				}
				break;
			}
		}
		fseek(fp,blklen,SEEK_CUR);//当前位置偏移一个块,指向下一个块
		tsa_offnum++;
	}
	return tsa_findnum;
}
int getoneTSAblkoffnum(TSA tsa,INT32U blklen,INT16U headlen,FILE *fp)
{
	INT8U tsadata[18];
	INT16U tsa_offnum=0;
	int i=0;
	if(fp == NULL)//文件不存在，返回空
	{
		fprintf(stderr,"\nONE计算tsapos打开文件失败!!!\n");
		return 0;
	}
	fseek(fp,headlen,SEEK_SET);
	while(!feof(fp))
	{
		if(fread(tsadata,sizeof(TSA)+1,1,fp)==0)//0x55 + tsa
		{
			fprintf(stderr,"\n文件尾\n");
			break;
		}
		fprintf(stderr,"\n文件中tsa(%02x)：",tsadata[0]);
		for(i=0;i<tsadata[1]+1;i++)
		{
			fprintf(stderr,"%02x ",tsadata[i+1]);
		}
		fprintf(stderr,"\n查找的tsa：");
		for(i=0;i<tsa.addr[0]+1;i++)
		{
			fprintf(stderr,"%02x ",tsa.addr[i]);
		}
		fprintf(stderr,"\n");
		if(memcmp(&tsadata[1],tsa.addr,tsadata[1]+1)==0 && tsadata[0] == 0x55)
		{
			return tsa_offnum;
		}
		tsa_offnum++;
		fseek(fp,blklen,SEEK_CUR);//当前位置偏移一个块,指向下一个块
	}
	return -1;
}
void prtfiletsa(char *fname)//打印任务文件中存在的测量点
{
	INT8U tsadata[18];
	INT16U tsa_index=0;
	INT32U blklen=0,headlen=0;
	int i =0;
	HEADFIXED_INFO taskhead_info;
	FILE *fp = NULL;
	fprintf(stderr,"\n打开文件%s\n",fname);
	fp = fopen(fname,"r");//读取格式
	if(fp == NULL)//文件不存在，返回空
	{
		fprintf(stderr,"\n打开文件%s失败!!!\n",fname);
		return;
	}
	else
		fread(&taskhead_info,sizeof(HEADFIXED_INFO),1,fp);
	blklen = taskhead_info.reclen*taskhead_info.seqnum;
	headlen = sizeof(HEADFIXED_INFO)+taskhead_info.oadnum*sizeof(HEAD_UNIT);
	fprintf(stderr,"\nreclen=%d,seqnum=%d,seqsec=%d,blklen=%d,oadnum=%d\n",taskhead_info.reclen,taskhead_info.seqnum,taskhead_info.seqsec,blklen,taskhead_info.oadnum);

	fseek(fp,headlen,SEEK_SET);
	while(!feof(fp))
	{
		if(fread(tsadata,sizeof(TSA)+1,1,fp)==0)//0x55 + tsa
			break;
		fprintf(stderr,"\n%d:",tsa_index);
		for(i=0;i<tsadata[1]+1;i++)
			fprintf(stderr,"%02x ",tsadata[1+i]);
		fprintf(stderr,"\n");
		tsa_index++;
		fseek(fp,blklen,SEEK_CUR);//当前位置偏移一个块,指向下一个块
	}
}
int prtTSAdata(char *fname,TSA tsa)
{
	INT8U recorddata[2048],bcd_tmp=0;
	INT16U tsa_offnum=0;
	INT32U blklen=0,headlen=0;
	int i=0,j=0,mm=0,data_index=0;
	FILE *fp=NULL;
	CLASS_6001 meter_tmp;
	HEADFIXED_INFO taskhead_info;
	HEAD_UNIT headoad_unit[FILEOADMAXNUM];

	memcpy(meter_tmp.basicinfo.addr.addr,tsa.addr,TSA_LEN);
	fprintf(stderr,"\n打开文件%s\n",fname);
	fp = fopen(fname,"r");//读取格式
	if(fp == NULL)//文件不存在，返回空
	{
		fprintf(stderr,"\n打开文件%s失败!!!\n",fname);
		return -1;
	}
	else
	{
		fread(&taskhead_info,sizeof(HEADFIXED_INFO),1,fp);
		fread(headoad_unit,taskhead_info.oadnum*sizeof(HEAD_UNIT),1,fp);
	}
	blklen = taskhead_info.reclen*taskhead_info.seqnum;
	headlen = sizeof(HEADFIXED_INFO)+taskhead_info.oadnum*sizeof(HEAD_UNIT);
	fprintf(stderr,"\nreclen=%d,seqnum=%d,seqsec=%d,blklen=%d,oadnum=%d\n",taskhead_info.reclen,taskhead_info.seqnum,taskhead_info.seqsec,blklen,taskhead_info.oadnum);

	if((tsa_offnum=getoneTSAblkoffnum(tsa,blklen,headlen,fp))<0)
	{
		fprintf(stderr,"没有这个测量点");
		return -1;
	}
	fprintf(stderr,"\ntsa_offnum=%d\n",tsa_offnum);
	for(i=0;i<taskhead_info.seqnum;i++)
	{
		data_index = 0;
		fseek(fp,headlen+(blklen+TSA_LEN)*tsa_offnum+TSA_LEN+1+taskhead_info.reclen*i,SEEK_SET);//跳到要抄找的记录的位置
		fread(recorddata,taskhead_info.reclen,1,fp);
		if(recorddata[0]==0)
			continue;
		fprintf(stderr,"\n\nseq=%d:\n",i);
		for(j=0;j<taskhead_info.oadnum;j++)
		{
			fprintf(stderr,"\n%04x%02x%02x-%04x%02x%02x:",
					headoad_unit[j].oad_m.OI,headoad_unit[j].oad_m.attflg,headoad_unit[j].oad_m.attrindex,
					headoad_unit[j].oad_r.OI,headoad_unit[j].oad_r.attflg,headoad_unit[j].oad_r.attrindex);
			for(mm=0;mm<headoad_unit[j].len;mm++)
				fprintf(stderr," %02x",recorddata[data_index++]);
		}
	}
	fprintf(stderr,"\n");
	return 0;
}

time_t tstotime_t(TS ts)
{
	time_t time_s;
	struct tm *tm_p;
	time(&time_s);
	tm_p = localtime(&time_s);
	tm_p->tm_year = ts.Year-1900;
	tm_p->tm_mon = ts.Month-1;
	tm_p->tm_mday = ts.Day;
	tm_p->tm_hour = ts.Hour;
	tm_p->tm_min = ts.Minute;
	tm_p->tm_sec = ts.Sec;
	return mktime(tm_p);
}
void time_ttots(TS *ts,time_t t)
{
    struct tm set;

    localtime_r(&t, &set);
    ts->Year  = set.tm_year + 1900;
    ts->Month = set.tm_mon + 1;
    ts->Day   = set.tm_mday;
    ts->Hour  = set.tm_hour;
    ts->Minute   = set.tm_min;
    ts->Sec   = set.tm_sec;
}

void getFILEts(INT8U frz_type,TS *ts_file)
{
	switch(frz_type)//冻结类存储时间-1个单位时间，如日冻结 16号抄的15号的冻结，则存在15的文件名里
	{
	case 1://日冻结
		tminc(ts_file, day_units, -1);
		break;
	case 2://月冻结
		tminc(ts_file, month_units, -1);
		ts_file->Day = 0;
		break;
	case 3://年冻结
		tminc(ts_file, year_units, -1);
		ts_file->Month = 0;
		ts_file->Day = 0;
		break;
	default://默认当日
		break;
	}
	ts_file->Hour = 0;
	ts_file->Minute = 0;
	ts_file->Sec = 0;
}
int fillTsaNullData(INT8U *databuf,TSA tsa,ROAD_ITEM item_road)
{
	int pindex = 0;
	int i=0;

	for(i=0;i<item_road.oadmr_num;i++) {
//		fprintf(stderr,"\nitem_road.oad[i].oad = %04x-%04x  item_road.oad[i].oad_num = %d\n",
//				item_road.oad[i].oad_m.OI,item_road.oad[i].oad_r.OI,
//				item_road.oad[i].oad_num);
		if(item_road.oad[i].oad_m.OI == 0x0000 && item_road.oad[i].oad_r.OI == 0x202a) {
			databuf[pindex++] = dttsa;
			memcpy(&databuf[pindex],&tsa,(tsa.addr[0]+1));
			pindex += (tsa.addr[0]+1);
		}else {
			if((item_road.oad[i].oad_m.OI == 0x0000) || (item_road.oad[i].oad_num != 0)) {
					databuf[pindex++] = 0;
//				fprintf(stderr,"\npindex = %d\n",pindex);
			}
		}
	}
//	fprintf(stderr,"fillTsaNullData----index=%d\n",pindex);
	return pindex;
}
//此招测类型用于招测的数据在一天,两个传进来的时间必须在同一天
INT16U dealselect5(OAD oad_h,CSD_ARRAYTYPE csds,TS ts_start,TS ts_end,INT32U zc_sec,INT16U tsa_num,CLASS_6001 *tsa_group,INT16U frmmaxsize,int frmadd_flg)
{
	char fname[FILENAMELEN]={};
	INT8U taskid,recorddata[2048],frmdata[2048],seqnumindex=0,frz_type=0;
	INT16U frmnum=0,tsa_findnum=0,recordnum=0,seq_start=0,seq_end=0,indexn=0,day_sec=0;
	INT32U blklen=0,headlen=0,seqnum=0,seqsec=0;
	int i=0,j=0;
	time_t time_end,time_start;
	ROAD_ITEM item_road;
	CLASS_6015 class6015;
	CLASS_6013 class6013;
	HEAD_UNIT headoad_unit[FILEOADMAXNUM];
	FILE *fp=NULL,*frm_fp=NULL;
	HEADFIXED_INFO taskhead_info;
	OAD_INDEX oad_offset[FILEOADMAXNUM];
	TS ts_file;

	if(frmmaxsize <= 512 || frmmaxsize>=1500)
		frmmaxsize = 512;

	memset(frmdata,0,sizeof(frmdata));

	memset(&item_road,0,sizeof(item_road));
	extendcsds(csds,&item_road);
	item_road.zc_seqsec = zc_sec;
	if((taskid = GetTaskidFromCSDs(item_road,tsa_group)) == 0) {//暂时不支持招测的不在一个采集方案
		//处理招测的oad不在一个任务
		asyslog(LOG_INFO,"GetTaskData: taskid=%d\n",taskid);
			return 0;
	}
	//------------------------------------------------------------------------------打开任务文件，并获得任务文件头信息
	if((frz_type = get60136015info(taskid,&class6015,&class6013))==0)//获取任务参数
	{
		fprintf(stderr,"\n获取任务%d配置失败\n",taskid);
		return 0;
	}

	if(ts_start.Year == 0xffff)
	{
		fprintf(stderr,"\n任务%d存储类型为%d\n",taskid,frz_type);
		seqsec = TItoSec(class6013.interval);
		fprintf(stderr,"\n每天存储记录个数seqsec=%d\n",seqsec);
		if(seqsec == 0)
			seqsec = 86400;
		TSGet(&ts_end);
		time_end = tstotime_t(ts_end);
		day_sec = ((ts_end.Hour*3600+ts_end.Minute*60+ts_end.Sec)/seqsec)*seqsec;
		fprintf(stderr,"\nday_sec=%d\n",day_sec);
		ts_end.Hour = 0;
		ts_end.Minute = 0;
		ts_end.Sec = 0;
		time_end = tstotime_t(ts_end) + day_sec - seqsec;//上报上一次
		time_start = time_end - seqsec;
		time_ttots(&ts_start,time_start);
		time_ttots(&ts_end,time_end-1);
	}
	fprintf(stderr,"\n取数时间范围%d-%d-%d %d:%d:%d---%d-%d-%d %d:%d:%d\n",
			ts_start.Year,ts_start.Month,ts_start.Day,ts_start.Hour,ts_start.Minute,ts_start.Sec,
			ts_end.Year,ts_end.Month,ts_end.Day,ts_end.Hour,ts_end.Minute,ts_end.Sec);
	ts_file = ts_start;

	if((class6015.savetimeflag == 3 || class6015.savetimeflag == 4) && frz_type == 1)
		;
	else
		getFILEts(frz_type,&ts_file);
	getTaskFileName(taskid,ts_file,fname);
	fprintf(stderr,"\n打开文件%s\n",fname);
	fp = fopen(fname,"r");//读取格式
	if(fp == NULL)//文件不存在，返回空
	{
		fprintf(stderr,"\n打开文件%s失败!!!\n",fname);
		return 0;
	}
	else
	{
		fread(&taskhead_info,sizeof(HEADFIXED_INFO),1,fp);
		fread(headoad_unit,taskhead_info.oadnum*sizeof(HEAD_UNIT),1,fp);
	}
	seq_start = (ts_start.Hour*60*60+ts_start.Minute*60+ts_start.Sec)/taskhead_info.seqsec;
	seq_end = (ts_end.Hour*60*60+ts_end.Minute*60+ts_end.Sec)/taskhead_info.seqsec+1;
	if(ts_start.Year != ts_end.Year || ts_start.Month != ts_end.Month || ts_start.Day != ts_end.Day)
		return 0;

	blklen = taskhead_info.reclen*taskhead_info.seqnum;
	headlen = sizeof(HEADFIXED_INFO)+taskhead_info.oadnum*sizeof(HEAD_UNIT);
	fprintf(stderr,"\nheadlen=%d,reclen=%d,seqnum=%d,seqsec=%d,blklen=%d,oadnum=%d\n",headlen,taskhead_info.reclen,taskhead_info.seqnum,taskhead_info.seqsec,blklen,taskhead_info.oadnum);
	//------------------------------------------------------------------------------获得招测的TSA在文件中的偏移块数
	tsa_findnum = getTSAblkoffnum(tsa_group,tsa_num,blklen,headlen,fp);
	fprintf(stderr,"\ntsa_offnum=%d\n",tsa_findnum);
	if(tsa_findnum == 0)
		return 0;
	fprintf(stderr,"\n----1\n");
	if(seq_end > taskhead_info.seqnum)
		seq_end = taskhead_info.seqnum;
	if(frz_type != 4)//除了实时数据，其他一天就存一个
	{
		seq_start = 0;
		seq_end = 1;
	}
	fprintf(stderr,"\n招测序号%d--%d\n",seq_start,seq_end);

	//------------------------------------------------------------------------------获得每个招测的oad在一条记录中的偏移
	memset(oad_offset,0x00,sizeof(oad_offset));
	GetOADPosofUnit(item_road,headoad_unit,taskhead_info.oadnum,oad_offset);//得到每一个oad在块数据中的偏移

	//------------------------------------------------------------------------------提取记录并组帧
	frm_fp = openFramefile(TASK_FRAME_DATA,frmadd_flg);
	memset(frmdata,0,sizeof(frmdata));
	//初始化分帧头
	indexn = 2;
	indexn += initFrameHead(&frmdata[indexn],oad_h,csds,&seqnumindex);
//	for(i=0;i<tsa_findnum;i++)
	for(i=0;i<tsa_num;i++)
	{
//		fprintf(stderr,"\ntsa偏移块个数%d\n",tsa_group[i].extinfo.pt);
		if(tsa_group[i].extinfo.pt==0)//没有这个地址的数据
		{
//			indexn += fillTsaNullData(&frmdata[indexn],tsa_group[i].basicinfo.addr,item_road);
//			recordnum++;
			continue;
		}
		for(j=seq_start;j<seq_end;j++)
		{
			memset(recorddata,0x00,sizeof(recorddata));
			fprintf(stderr,"\n序号%d 记录位置%d\n",j,headlen+(blklen+TSA_LEN+1)*tsa_group[i].extinfo.pt+TSA_LEN+1+taskhead_info.reclen*j);
			fseek(fp,headlen+(blklen+TSA_LEN+1)*(tsa_group[i].extinfo.pt-1)+TSA_LEN+1+taskhead_info.reclen*j,SEEK_SET);//跳到要抄找的记录的位置
			fread(recorddata,taskhead_info.reclen,1,fp);
			fprintf(stderr,"\n数据%02x %02x\n",recorddata[0],recorddata[1]);
			if(recorddata[0]==0)
			{
				fprintf(stderr,"\n记录%d为空，不取数据%02x %02x\n",j,recorddata[0],recorddata[1]);
				continue;
			}
			PRTbuf(recorddata,taskhead_info.reclen);
			indexn += getrecdata(recorddata,tsa_group[i].basicinfo.addr,item_road,oad_offset,&frmdata[indexn]);
			recordnum++;
			fprintf(stderr,"\nindexn=%d,frmmaxsize=%d\n",indexn,frmmaxsize);
			if(indexn >= frmmaxsize-100)
			{
				frmnum++;
				intToBuf((indexn-2),frmdata);		//帧长度保存帧的数据长度
				frmdata[seqnumindex] = recordnum;
//				saveOneFrame1(frmdata,indexn);
				saveOneFrame(frmdata,indexn,frm_fp);
				indexn = 2;
				indexn += initFrameHead(&frmdata[indexn],oad_h,csds,&seqnumindex);
				recordnum = 0;
			}
		}
//		break;
	}
	fprintf(stderr,"\nrecordnum=%d\n",recordnum);
	if(frmnum==0) {
		frmnum = 1; //一帧
		fprintf(stderr,"\n indexn = %d saveOneFrame  seqnumindex=%d,  recordnum=%d!!!!!!!!!!!!!!!!\n",indexn,seqnumindex,recordnum);
		asyslog(LOG_INFO,"任务数据文件组帧:indexn = %d , seqnumindex=%d,  recordnum=%d\n",indexn,seqnumindex,recordnum);
		intToBuf((indexn-2),frmdata);
		frmdata[seqnumindex] = recordnum;
//		saveOneFrame1(frmdata,indexn);
		saveOneFrame(frmdata,indexn,frm_fp);
//		indexn += initFrameHead(&frmdata[indexn],oad_h,csds,&seqnumindex);
	}
	else {
		if(recordnum != 0)
		{
			frmnum++;
			fprintf(stderr,"\n last frm indexn = %d saveOneFrame  seqnumindex=%d,  recordnum=%d!!!!!!!!!!!!!!!!\n",indexn,seqnumindex,recordnum);
			asyslog(LOG_INFO,"任务数据文件组帧:indexn = %d , seqnumindex=%d,  recordnum=%d\n",indexn,seqnumindex,recordnum);
			intToBuf((indexn-2),frmdata);
			frmdata[seqnumindex] = recordnum;
//			saveOneFrame1(frmdata,indexn);
			saveOneFrame(frmdata,indexn,frm_fp);
//			indexn += initFrameHead(&frmdata[indexn],oad_h,csds,&seqnumindex);
		}
	}
	if(frm_fp != NULL)
		fclose(frm_fp);
	return frmnum;
}
/*
 * 上报某一时间段数据记录，这一时间段跨日
 */
INT16U dealselect7(OAD oad_h,CSD_ARRAYTYPE csds,CLASS_6001 *tsa_group,TS ts_start,TS ts_end,INT16U frmmaxsize)
{
	char fname[FILENAMELEN]={};
	INT8U taskid=0,recorddata[2048],frmdata[2048],seqnumindex=0;
	INT16U frmnum=0,tsa_num=0,tsa_findnum=0,recordnum=0,indexn=0;
	INT32U blklen=0,headlen=0;
	int i=0,j=0,mm=0;
	TS ts_cur;
	time_t time_start,time_end,time_cur,time_chaoshi;
	INT16U seq_start,seq_end;
	ROAD_ITEM item_road;
	HEAD_UNIT headoad_unit[FILEOADMAXNUM];
	FILE *fp=NULL,*frm_fp=NULL;
	HEADFIXED_INFO taskhead_info;
	OAD_INDEX oad_offset[FILEOADMAXNUM];
	if(frmmaxsize <= 512 || frmmaxsize>=2000)
		frmmaxsize = 512;

	memset(&item_road,0x00,sizeof(ROAD_ITEM));
	memset(frmdata,0,sizeof(frmdata));

	time_start = tstotime_t(ts_start);
	time_end = tstotime_t(ts_end);
	memset(&item_road,0,sizeof(item_road));
	extendcsds(csds,&item_road);
	if((taskid = GetTaskidFromCSDs(item_road,tsa_group)) == 0) {//暂时不支持招测的不在一个采集方案
		asyslog(LOG_INFO,"GetTaskData: taskid=%d\n",taskid);
			return 0;
	}

	frm_fp = openFramefile(TASK_FRAME_DATA,0);
	fprintf(stderr,"\ntsa_num=%d\n",tsa_num);
	time_chaoshi = time(NULL);
	for(i=0;i<tsa_num;i++)
	{
		if(tsa_group[i].extinfo.pt==0)//没有这个地址的数据
		{
//			indexn += fillTsaNullData(&frmdata[indexn],tsa_group[i].basicinfo.addr,item_road);
//			recordnum++;
			continue;
		}
		ts_cur = ts_start;
		time_cur = time_start;
		fprintf(stderr,"\ncur-end:%d--%d\n",time_cur,time_end);
		while(time_cur>time_end)
		{
			if(abs(time(NULL)-time_chaoshi)>25)//进入函数超过25s，则退出本函数   超时前处理
			{
				fprintf(stderr,"\n查找数据超时，返回现有帧\n");
				return frmnum;
			}
			getTaskFileName(taskid,ts_cur,fname);
			fprintf(stderr,"\n打开文件%s\n",fname);
			fp = fopen(fname,"r");//读取格式
			if(fp == NULL)//文件不存在，返回空
			{
				fprintf(stderr,"\n打开文件%s失败!!!\n",fname);
				return 0;
			}
			else
			{
				fread(&taskhead_info,sizeof(HEADFIXED_INFO),1,fp);
				fread(headoad_unit,taskhead_info.oadnum*sizeof(HEAD_UNIT),1,fp);
			}

			blklen = taskhead_info.reclen*taskhead_info.seqnum;
			headlen = sizeof(HEADFIXED_INFO)+taskhead_info.oadnum*sizeof(HEAD_UNIT);
			fprintf(stderr,"\nreclen=%d,seqnum=%d,seqsec=%d,blklen=%d,oadnum=%d\n",taskhead_info.reclen,taskhead_info.seqnum,taskhead_info.seqsec,blklen,taskhead_info.oadnum);
			tsa_findnum = getTSAblkoffnum(&tsa_group[i],1,blklen,headlen,fp);//只比对一个tsa
			fprintf(stderr,"\ntsa_offnum=%d\n",tsa_findnum);
			if(tsa_findnum == 0)
				continue;
//			fprintf(stderr,"\ntsa偏移块个数%d\n",tsa_group[i].extinfo.pt);


			memset(oad_offset,0x00,sizeof(oad_offset));
			GetOADPosofUnit(item_road,headoad_unit,taskhead_info.oadnum,oad_offset);//得到每一个oad在块数据中的偏移


			memset(frmdata,0,sizeof(frmdata));
			//初始化分帧头
			indexn = 2;
			indexn += initFrameHead(&frmdata[indexn],oad_h,csds,&seqnumindex);

			seq_start = (ts_cur.Hour*60*60+ts_cur.Minute*60+ts_cur.Sec)/taskhead_info.seqsec;
			if(ts_cur.Year == ts_end.Year && ts_cur.Month == ts_end.Month && ts_cur.Day == ts_end.Day)
				seq_end = (ts_end.Hour*60*60+ts_end.Minute*60+ts_end.Sec)/taskhead_info.seqsec;
			else
				seq_end = taskhead_info.seqnum;
			if(seq_end > taskhead_info.seqnum)
				seq_end = taskhead_info.seqnum;
			for(j=seq_start;j<seq_end;j++)
			{
				memset(recorddata,0x00,sizeof(recorddata));
				fprintf(stderr,"\n记录%d位置%d\n",j,headlen+(blklen+TSA_LEN+1)*tsa_group[i].extinfo.pt+TSA_LEN+1+taskhead_info.reclen*j);
				fseek(fp,headlen+(blklen+TSA_LEN+1)*(tsa_group[i].extinfo.pt-1)+TSA_LEN+1+taskhead_info.reclen*j,SEEK_SET);//跳到要抄找的记录的位置
				fread(recorddata,taskhead_info.reclen,1,fp);
				PRTbuf(recorddata,taskhead_info.reclen);
				if(recorddata[0]==0)
					continue;
				fprintf(stderr,"\nrecorddata[0] = %d\n",recorddata[0]);
				indexn += getrecdata(recorddata,tsa_group[i].basicinfo.addr,item_road,oad_offset,&frmdata[indexn]);
				recordnum++;
				fprintf(stderr,"\nindexn=%d,frmmaxsize=%d\n",indexn,frmmaxsize);
				fprintf(stderr,"\nfrmdata(%d):",indexn);
				for(mm=0;mm<indexn;mm++)
					fprintf(stderr," %02x",frmdata[mm]);
				fprintf(stderr,"\n");
				if(indexn >= frmmaxsize-100)
				{
					frmnum++;
					intToBuf((indexn-2),frmdata);		//帧长度保存帧的数据长度
					frmdata[seqnumindex] = recordnum;
					saveOneFrame(frmdata,indexn,frm_fp);
					indexn = 2;
					indexn += initFrameHead(&frmdata[indexn],oad_h,csds,&seqnumindex);
					recordnum = 0;
				}
			}
			ts_cur.Hour = 0;
			ts_cur.Minute = 0;
			ts_cur.Sec = 0;
			tminc(&ts_cur,day_units,1);//加上一天,加到当天0点0分0秒
			time_cur = tstotime_t(ts_cur);
		}
	}


	fprintf(stderr,"\nrecordnum=%d\n",recordnum);
	if(frmnum==0) {
		frmnum = 1; //一帧
		fprintf(stderr,"\n indexn = %d saveOneFrame  seqnumindex=%d,  recordnum=%d!!!!!!!!!!!!!!!!\n",indexn,seqnumindex,recordnum);
		asyslog(LOG_INFO,"任务数据文件组帧:indexn = %d , seqnumindex=%d,  recordnum=%d\n",indexn,seqnumindex,recordnum);
		intToBuf((indexn-2),frmdata);
		frmdata[seqnumindex] = recordnum;
		saveOneFrame(frmdata,indexn,frm_fp);
	}
	else {
		if(recordnum != 0)
		{
			frmnum++;
			fprintf(stderr,"\n last frm indexn = %d saveOneFrame  seqnumindex=%d,  recordnum=%d!!!!!!!!!!!!!!!!\n",indexn,seqnumindex,recordnum);
			asyslog(LOG_INFO,"任务数据文件组帧:indexn = %d , seqnumindex=%d,  recordnum=%d\n",indexn,seqnumindex,recordnum);
			intToBuf((indexn-2),frmdata);
			frmdata[seqnumindex] = recordnum;
			saveOneFrame(frmdata,indexn,frm_fp);
		}
	}
	if(frm_fp != NULL)
		fclose(frm_fp);
	return frmnum;
}

void saveNULLfrm(INT8U oad_num,OAD oad_h,CSD_ARRAYTYPE csds)
{
	INT8U frmdata[2048];
	int indexn = 2,i = 0;
	FILE *frm_fp=NULL;
	memset(frmdata,0x00,sizeof(frmdata));

	fprintf(stderr,"\noad_num=%d\n",oad_num);
	frm_fp = openFramefile(TASK_FRAME_DATA,0);
	if(frm_fp !=NULL)
	{
		frmdata[indexn++] = 1;							//SEQUENCE OF A-ResultRecord
		indexn +=create_OAD(0,&frmdata[indexn] ,oad_h);		//主OAD
		frmdata[indexn++] = csds.num;					//RCSD::SEQUENCE OF CSD
		for(i=0; i<csds.num; i++)
		{
			indexn += fill_CSD(0,&frmdata[indexn],csds.csd[i]);		//填充 CSD
		}
		frmdata[indexn++] = 1;	//结果类型 数据
		frmdata[indexn++] = 0;	//sequence of 长度
//		indexn += initFrameHead(&frmdata[indexn],oad_h,csds,&seqnumindex);
		intToBuf((indexn-2),frmdata);
		saveOneFrame(frmdata,indexn,frm_fp);
		fclose(frm_fp);
	}
}
/*
 *上报最新一条数据记录
 *
 */
INT16U dealselect10(OAD oad_h,CSD_ARRAYTYPE csds,INT16U zcseq_num,INT8U tsa_num,CLASS_6001 *tsa_group,INT16U frmmaxsize)
{
	char fname[FILENAMELEN]={};
	INT8U taskid=0,recorddata[2048],frmdata[2048],seqnumindex=0,frz_type=0;
	INT16U frmnum=0,tsa_findnum=0,recordnum=0,indexn=0,curseq=0;
	INT32U blklen=0,headlen=0;
	int i=0,j=0;
	TS ts_zc,ts_tmp,ts_file;
	ROAD_ITEM item_road;
	HEAD_UNIT headoad_unit[FILEOADMAXNUM];
	FILE *fp=NULL,*frm_fp=NULL;
	CLASS_6015 class6015;
	CLASS_6013 class6013;
	HEADFIXED_INFO taskhead_info;
	OAD_INDEX oad_offset[FILEOADMAXNUM];
	fprintf(stderr,"\nfrmmaxsize=%d\n",frmmaxsize);
	if(frmmaxsize <= 512 || frmmaxsize>=2000)
		frmmaxsize = 512;

	TSGet(&ts_zc);//招测的第一个值为当前值
	memset(&item_road,0x00,sizeof(ROAD_ITEM));
	memset(frmdata,0,sizeof(frmdata));

	fprintf(stderr,"\n招测方式     select 10\n");
	extendcsds(csds,&item_road);
	if((taskid = GetTaskidFromCSDs(item_road,tsa_group)) == 0) {//暂时不支持招测的不在一个采集方案
		asyslog(LOG_INFO,"GetTaskData: taskid=%d\n",taskid);
		//初始化分帧头
		fprintf(stderr,"\n存空帧\n");
		saveNULLfrm(item_road.oadmr_num,oad_h,csds);
		return 1;
	}
	if((frz_type = get60136015info(taskid,&class6015,&class6013))==0)//获取任务参数
	{
		fprintf(stderr,"\n获取任务%d配置失败\n",taskid);
		return 0;
	}
	ts_file = ts_zc;
	getFILEts(frz_type,&ts_file);
	getTaskFileName(taskid,ts_file,fname);
	fprintf(stderr,"\n打开文件%s\n",fname);
	fp = fopen(fname,"r");//读取格式
	if(fp == NULL)//文件不存在，返回空
	{
		fprintf(stderr,"\n打开文件%s失败!!!\n",fname);
		return 0;
	}
	else
	{
		fread(&taskhead_info,sizeof(HEADFIXED_INFO),1,fp);
		fread(headoad_unit,taskhead_info.oadnum*sizeof(HEAD_UNIT),1,fp);
	}

	blklen = taskhead_info.reclen*taskhead_info.seqnum;
	headlen = sizeof(HEADFIXED_INFO)+taskhead_info.oadnum*sizeof(HEAD_UNIT);
	fprintf(stderr,"\nsele 10 reclen=%d,seqnum=%d,seqsec=%d,blklen=%d,oadnum=%d\n",taskhead_info.reclen,taskhead_info.seqnum,taskhead_info.seqsec,blklen,taskhead_info.oadnum);

	memset(oad_offset,0x00,sizeof(oad_offset));
	GetOADPosofUnit(item_road,headoad_unit,taskhead_info.oadnum,oad_offset);//得到每一个oad在块数据中的偏移


	//初始化分帧头
	indexn = 2;
	indexn += initFrameHead(&frmdata[indexn],oad_h,csds,&seqnumindex);

	tsa_findnum = getTSAblkoffnum(tsa_group,tsa_num,blklen,headlen,fp);
	if(tsa_findnum == 0)
		return 0;
	frm_fp = openFramefile(TASK_FRAME_DATA,0);
//	if(zcseq_num == 1)//招测最新的一条记录，开启补报模式
	for(i=0;i<tsa_num;i++)
	{
		ts_tmp = ts_zc;
//		fprintf(stderr,"\ntsa偏移块个数%d\n",tsa_group[i].extinfo.pt);
		if(tsa_group[i].extinfo.pt==0)//没有这个地址的数据
		{
			fprintf(stderr,"\n没有这个地址的数据，上报空\n");
//			indexn += fillTsaNullData(&frmdata[indexn],tsa_group[i].basicinfo.addr,item_road);
//			recordnum++;
			continue;
		}
		curseq = (ts_zc.Hour*60*60+ts_zc.Minute*60+ts_zc.Sec)/taskhead_info.seqsec;
		for(j=0;j<zcseq_num;j++)
		{
			memset(recorddata,0x00,sizeof(recorddata));
			fprintf(stderr,"\n记录位置%d\n",headlen+(blklen+TSA_LEN+1)*tsa_group[i].extinfo.pt+TSA_LEN+1+taskhead_info.reclen*j);
			fseek(fp,headlen+(blklen+TSA_LEN+1)*(tsa_group[i].extinfo.pt-1)+TSA_LEN+1+taskhead_info.reclen*curseq,SEEK_SET);//跳到要抄找的记录的位置
			fread(recorddata,taskhead_info.reclen,1,fp);
			PRTbuf(recorddata,taskhead_info.reclen);
			indexn += getrecdata(recorddata,tsa_group[i].basicinfo.addr,item_road,oad_offset,&frmdata[indexn]);
			recordnum++;
			fprintf(stderr,"\nindexn=%d,frmmaxsize=%d\n",indexn,frmmaxsize);
			if(indexn >= frmmaxsize-100)
			{
				frmnum++;
				intToBuf((indexn-2),frmdata);		//帧长度保存帧的数据长度
				frmdata[seqnumindex] = recordnum;
				saveOneFrame(frmdata,indexn,frm_fp);
				indexn = 2;
				indexn += initFrameHead(&frmdata[indexn],oad_h,csds,&seqnumindex);
				recordnum = 0;
			}
			if(curseq == 0 && zcseq_num != 1)//更新fp文件流
			{
				if(fp != NULL)
					fclose(fp);
				curseq = taskhead_info.seqnum;
				tminc(&ts_tmp,day_units,-1);
				getTaskFileName(taskid,ts_tmp,fname);
				fprintf(stderr,"\n打开文件%s\n",fname);
				fp = fopen(fname,"r");//读取格式
				if(fp == NULL)//文件不存在
				{
					fprintf(stderr,"\n打开文件%s失败!!!\n",fname);
					continue;
				}
				else
				{
					fread(&taskhead_info,sizeof(HEADFIXED_INFO),1,fp);
					fread(headoad_unit,taskhead_info.oadnum*sizeof(HEAD_UNIT),1,fp);
				}

				blklen = taskhead_info.reclen*taskhead_info.seqnum;
				headlen = sizeof(HEADFIXED_INFO)+taskhead_info.oadnum*sizeof(HEAD_UNIT);
				fprintf(stderr,"\nreclen=%d,seqnum=%d,seqsec=%d,blklen=%d,oadnum=%d\n",taskhead_info.reclen,taskhead_info.seqnum,taskhead_info.seqsec,blklen,taskhead_info.oadnum);

				memset(oad_offset,0x00,sizeof(oad_offset));
				GetOADPosofUnit(item_road,headoad_unit,taskhead_info.oadnum,oad_offset);//得到每一个oad在块数据中的偏移
				tsa_findnum = getTSAblkoffnum(tsa_group,tsa_num,blklen,headlen,fp);
			}
			curseq--;
		}
	}
	fprintf(stderr,"\nrecordnum=%d\n",recordnum);
	if(frmnum==0) {
		frmnum = 1; //一帧
		fprintf(stderr,"\n indexn = %d saveOneFrame  seqnumindex=%d,  recordnum=%d!!!!!!!!!!!!!!!!\n",indexn,seqnumindex,recordnum);
		asyslog(LOG_INFO,"任务数据文件组帧:indexn = %d , seqnumindex=%d,  recordnum=%d\n",indexn,seqnumindex,recordnum);
		intToBuf((indexn-2),frmdata);
		frmdata[seqnumindex] = recordnum;
		saveOneFrame(frmdata,indexn,frm_fp);
	}
	else {
		if(recordnum != 0)
		{
			frmnum++;
			fprintf(stderr,"\n last frm indexn = %d saveOneFrame  seqnumindex=%d,  recordnum=%d!!!!!!!!!!!!!!!!\n",indexn,seqnumindex,recordnum);
			asyslog(LOG_INFO,"任务数据文件组帧:indexn = %d , seqnumindex=%d,  recordnum=%d\n",indexn,seqnumindex,recordnum);
			intToBuf((indexn-2),frmdata);
			frmdata[seqnumindex] = recordnum;
			saveOneFrame(frmdata,indexn,frm_fp);
		}
	}
	if(frm_fp != NULL)
		fclose(frm_fp);
	return frmnum;
}
//日期在同一天返回1，相同，返回0 不合理返回-1，合理则返回相差的天数
int getdaynum(DateTimeBCD date_start,DateTimeBCD date_end)
{
	time_t time_s,time_start,time_end;
	struct tm *tm_p;

	if(memcmp(&date_start,&date_end,sizeof(TS))==0)
		return 0;
	if(date_start.year.data == date_end.year.data && date_start.month.data == date_end.month.data &&
			date_start.day.data == date_end.day.data)
	{
		if(date_end.hour.data*3600+date_end.min.data*60+date_end.sec.data <
				date_start.hour.data*3600+date_start.min.data*60+date_start.sec.data)
			return -1;
		else
			return 1;
	}
	time(&time_s);
	tm_p = localtime(&time_s);
	tm_p->tm_year = date_start.year.data-1900;
	tm_p->tm_mon = date_start.month.data-1;
	tm_p->tm_mday = date_start.day.data;
	tm_p->tm_hour = 0;
	tm_p->tm_min = 0;
	tm_p->tm_sec = 0;
	time_start = mktime(tm_p);

	time(&time_s);
	tm_p = localtime(&time_s);
	tm_p->tm_year = date_end.year.data-1900;
	tm_p->tm_mon = date_end.month.data-1;
	tm_p->tm_mday = date_end.day.data;
	tm_p->tm_hour = 0;
	tm_p->tm_min = 0;
	tm_p->tm_sec = 0;
	time_end = mktime(tm_p);

	fprintf(stderr,"\n时间秒数 %d %d now:%d %d\n",time_end,time_start,time(NULL),(time_end - time_start +1));
	if(time_end < time_start)
		return -1;
	else
		return (time_end - time_start -1)/86400;
}
INT16U dealevent(OAD oad_h,ROAD road_eve,CSD_ARRAYTYPE csds,INT16U tsa_num,CLASS_6001 *tsa_group,INT16U frmmaxsize)
{
	INT8U recorddata[2048],frmdata[2048],seqnumindex=0;
	INT16U frmnum=0,tsa_findnum=0,recordnum=0,indexn=0,lentmp=0;
	INT32U blklen=0,headlen=0;
	int i=0;
	HEADFIXED_INFO taskhead_info;
	HEAD_UNIT headoad_unit[FILEOADMAXNUM];
	OAD_INDEX oad_offset[FILEOADMAXNUM];
	ROAD_ITEM item_road;
	FILE *fp = NULL,*frm_fp = NULL;
	if(frmmaxsize <= 512 || frmmaxsize>=2000)
		frmmaxsize = 512;

	memset(&item_road,0x00,sizeof(ROAD_ITEM));
	memset(frmdata,0,sizeof(frmdata));
	memset(recorddata,0,sizeof(recorddata));

	extendcsds(csds,&item_road);

	memset(&taskhead_info,0x00,sizeof(HEADFIXED_INFO));
	memset(headoad_unit,0x00,sizeof(headoad_unit));

	fp = openevefile(road_eve.oad.OI);
	if(fp == NULL)//文件不存在，返回空
	{
		fprintf(stderr,"\n打开事件文件失败!!!\n");
		return 0;
	}
	else
	{
		fread(&taskhead_info,sizeof(HEADFIXED_INFO),1,fp);
		fread(headoad_unit,taskhead_info.oadnum*sizeof(HEAD_UNIT),1,fp);
	}

	blklen = taskhead_info.reclen*taskhead_info.seqnum;
	headlen = sizeof(HEADFIXED_INFO)+taskhead_info.oadnum*sizeof(HEAD_UNIT);
	fprintf(stderr,"\nreclen=%d,seqnum=%d,seqsec=%d,blklen=%d,oadnum=%d\n",taskhead_info.reclen,taskhead_info.seqnum,taskhead_info.seqsec,blklen,taskhead_info.oadnum);

	memset(oad_offset,0x00,sizeof(oad_offset));
	GetOADPosofUnit(item_road,headoad_unit,taskhead_info.oadnum,oad_offset);//得到每一个oad在块数据中的偏移


	memset(frmdata,0,sizeof(frmdata));
	//初始化分帧头
	indexn = 2;
	indexn += initFrameHead(&frmdata[indexn],oad_h,csds,&seqnumindex);

	tsa_findnum = getTSAblkoffnum(tsa_group,tsa_num,blklen,headlen,fp);
	fprintf(stderr,"\ntsa_offnum=%d\n",tsa_findnum);
	if(tsa_findnum == 0)
		return 0;
	frm_fp = openFramefile(TASK_FRAME_DATA,0);
//	if(zcseq_num == 1)//招测最新的一条记录，开启补报模式
	for(i=0;i<tsa_findnum;i++)
	{
		memset(recorddata,0x00,sizeof(recorddata));
		fprintf(stderr,"\n块数%d pos:%d len=%d\n",tsa_group[i].extinfo.pt,headlen+(blklen+TSA_LEN+1)*tsa_group[i].extinfo.pt+TSA_LEN+1,taskhead_info.reclen);
		fseek(fp,headlen+(blklen+TSA_LEN+1)*(tsa_group[i].extinfo.pt-1)+TSA_LEN+1,SEEK_SET);//跳到要抄找的记录的位置
		if((lentmp = fread(recorddata,taskhead_info.reclen,1,fp)) == 0)
		{
			fprintf(stderr,"\n查找到文件尾 结束  lentmp=%d\n",lentmp);
			break;
		}
		PRTbuf(recorddata,taskhead_info.reclen);
		indexn += getrecdata(recorddata,tsa_group[i].basicinfo.addr,item_road,oad_offset,&frmdata[indexn]);
		recordnum++;
		fprintf(stderr,"\nindexn=%d,frmmaxsize=%d\n",indexn,frmmaxsize);
		if(indexn >= frmmaxsize-100)
		{
			frmnum++;
			intToBuf((indexn-2),frmdata);		//帧长度保存帧的数据长度
			frmdata[seqnumindex] = recordnum;
			saveOneFrame(frmdata,indexn,frm_fp);
			indexn = 2;
			indexn += initFrameHead(&frmdata[indexn],oad_h,csds,&seqnumindex);
			recordnum = 0;
		}
	}
	fprintf(stderr,"\nrecordnum=%d\n",recordnum);
	if(frmnum==0) {
		frmnum = 1; //一帧
		fprintf(stderr,"\n indexn = %d saveOneFrame  seqnumindex=%d,  recordnum=%d!!!!!!!!!!!!!!!!\n",indexn,seqnumindex,recordnum);
		asyslog(LOG_INFO,"任务数据文件组帧:indexn = %d , seqnumindex=%d,  recordnum=%d\n",indexn,seqnumindex,recordnum);
		intToBuf((indexn-2),frmdata);
		frmdata[seqnumindex] = recordnum;
		saveOneFrame(frmdata,indexn,frm_fp);
	}
	else {
		if(recordnum != 0)
		{
			frmnum++;
			fprintf(stderr,"\n last frm indexn = %d saveOneFrame  seqnumindex=%d,  recordnum=%d!!!!!!!!!!!!!!!!\n",indexn,seqnumindex,recordnum);
			asyslog(LOG_INFO,"任务数据文件组帧:indexn = %d , seqnumindex=%d,  recordnum=%d\n",indexn,seqnumindex,recordnum);
			intToBuf((indexn-2),frmdata);
			frmdata[seqnumindex] = recordnum;
			saveOneFrame(frmdata,indexn,frm_fp);
		}
	}
	if(fp != NULL)
		fclose(fp);
	if(frm_fp != NULL)
		fclose(frm_fp);
	return frmnum;
}
MY_MS getSELEMS(INT8U selectype,RSD select)
{
	MY_MS meters_cur;
	memset(&meters_cur,0x00,sizeof(MY_MS));
	switch(selectype)
	{
	case 0:
		meters_cur.mstype = 1;//全部电表
		break;
	case 4:
		memcpy(&meters_cur,&select.selec4.meters,sizeof(meters_cur));
		break;
	case 5:
		memcpy(&meters_cur,&select.selec5.meters,sizeof(meters_cur));
		break;
	case 6:
		memcpy(&meters_cur,&select.selec6.meters,sizeof(meters_cur));
		break;
	case 7:
		memcpy(&meters_cur,&select.selec7.meters,sizeof(meters_cur));
		break;
	case 8:
		memcpy(&meters_cur,&select.selec8.meters,sizeof(meters_cur));
		break;
	case 10:
		memcpy(&meters_cur,&select.selec10.meters,sizeof(meters_cur));
		break;
	}
	return meters_cur;
}
//1得到招测信心
//2确定taskid  data最多一帧能存入2048个字节
//3提取记录组文件 5 7 10等招测的应该在一个任务文件里
INT16U getSelector(OAD oad_h,RSD select,INT8U selectype,CSD_ARRAYTYPE csds,INT8U *data,int *datalen,INT16U frmmaxsize,int recordnum)
{
	INT16U frmnum = 0,tsa_num=0,zc_sec=0;//zc_sec 0:全天数据 非0：招测间隔
	int day_num = 0,i=0;
	TS ts_start,ts_end,ts_now;
	time_t time_now=0,time_end=0;
	CLASS_6001 *tsa_group = NULL;//本次召测的tsa集合
	MY_MS meters_cur;
	ROAD road_eve;
	memset(&meters_cur,0x00,sizeof(MY_MS));
	memset(&road_eve,0x00,sizeof(ROAD));

	if(frmmaxsize >= 2048 || frmmaxsize < 500)
		frmmaxsize = 1024;
	fprintf(stderr,"\nselectype=%d\n",selectype);
	//一致性
	////////////////////////////////////////////////////////////
	meters_cur = getSELEMS(selectype,select);
	tsa_num = getOI6001(meters_cur,(INT8U **)&tsa_group);

	fprintf(stderr,"\ntsa_num=%d\n",tsa_num);
	if(tsa_num==0)
	{
		if(tsa_group != NULL)
			free(tsa_group);
		return 0;
	}
//	for(i=0;i<tsa_num;i++)
//		prtTSA(tsa_group[i].basicinfo.addr);

	for(i=0;i<csds.num;i++)//招测单个事件
	{
		if(csds.csd[i].type != 1)
			continue;
		if(csds.csd[i].csd.road.oad.OI >= 0x3000 && csds.csd[i].csd.road.oad.OI < 0x4000)//事件关联对象
		{
			selectype = 0xee;
			memcpy(&road_eve,&csds.csd[i].csd.road,sizeof(ROAD));
			fprintf(stderr,"\n事件oi：%04x\n",road_eve.oad.OI);
			break;
		}
	}

	switch(selectype)
	{
	case 0:
		frmnum = dealselect10(oad_h,csds,1,tsa_num,tsa_group,frmmaxsize);//上报当前数据
		break;
	case 4:
	case 5://找某一整天的数据
		TimeBCDToTs(select.selec5.collect_save,&ts_start);//
		ts_end=ts_start;
		ts_start.Hour=0;ts_start.Minute=0;ts_start.Sec=0;
		ts_end.Hour=23;ts_end.Minute=59;ts_end.Sec=59;
		frmnum = dealselect5(oad_h,csds,ts_start,ts_end,zc_sec,tsa_num,tsa_group,frmmaxsize,recordnum);
		break;
//	case 7:
//		day_num = getdaynum(select.selec7.collect_save_star,select.selec7.collect_save_finish);
//		TimeBCDToTs(select.selec7.collect_save_star,&ts_start);//
//		TimeBCDToTs(select.selec7.collect_save_finish,&ts_end);//
//		TSGet(&ts_now);
//		time_now = tstotime_t(ts_now);
//		time_end = tstotime_t(ts_end);
//		fprintf(stderr,"\nday_num=%d time_now=%d time_end=%d\n",day_num,time_now,time_end);
//		if(time_end > time_now)
//			ts_end = ts_now;
//		if(day_num<0)
//			return 0;
//		switch(day_num)
//		{
//		case 0:
//			frmnum = dealselect10(oad_h,select,csds,1,tsa_num,tsa_group,frmmaxsize);//上报当前数据
//			break;
//		case 1:
//			frmnum = dealselect5(oad_h,csds,ts_start,ts_end,select.selec7.meters,tsa_num,tsa_group,frmmaxsize);
//			break;
//		default:
//			frmnum = dealselect7(oad_h,select,csds,tsa_group,ts_start,ts_end,frmmaxsize);//找某一时间段的值
//			break;
//		}
//		break;
	case 6:
	case 7:
	case 8:
//		getSELETS();
		fprintf(stderr,"\nselect=%d\n",selectype);
		fprintf(stderr,"\nsele 6:%d-%d-%d %d:%d:%d\n",select.selec6.collect_star.year.data,
				select.selec6.collect_star.month.data,
				select.selec6.collect_star.day.data,
				select.selec6.collect_star.hour.data,
				select.selec6.collect_star.min.data,
				select.selec6.collect_star.sec.data);
		fprintf(stderr,"\nsele 6:%d-%d-%d %d:%d:%d\n",select.selec6.collect_finish.year.data,
				select.selec6.collect_finish.month.data,
				select.selec6.collect_finish.day.data,
				select.selec6.collect_finish.hour.data,
				select.selec6.collect_finish.min.data,
				select.selec6.collect_finish.sec.data);

		fprintf(stderr,"\nsele 7:%d-%d-%d %d:%d:%d\n",select.selec7.collect_save_star.year.data,
				select.selec7.collect_save_star.month.data,
				select.selec7.collect_save_star.day.data,
				select.selec7.collect_save_star.hour.data,
				select.selec7.collect_save_star.min.data,
				select.selec7.collect_save_star.sec.data);
		fprintf(stderr,"\nsele 7:%d-%d-%d %d:%d:%d\n",select.selec7.collect_save_finish.year.data,
				select.selec7.collect_save_finish.month.data,
				select.selec7.collect_save_finish.day.data,
				select.selec7.collect_save_finish.hour.data,
				select.selec7.collect_save_finish.min.data,
				select.selec7.collect_save_finish.sec.data);

		fprintf(stderr,"\nsele 8:%d-%d-%d %d:%d:%d\n",select.selec8.collect_succ_star.year.data,
				select.selec8.collect_succ_star.month.data,
				select.selec8.collect_succ_star.day.data,
				select.selec8.collect_succ_star.hour.data,
				select.selec8.collect_succ_star.min.data,
				select.selec8.collect_succ_star.sec.data);
		fprintf(stderr,"\nsele 8:%d-%d-%d %d:%d:%d\n",select.selec8.collect_succ_finish.year.data,
				select.selec8.collect_succ_finish.month.data,
				select.selec8.collect_succ_finish.day.data,
				select.selec8.collect_succ_finish.hour.data,
				select.selec8.collect_succ_finish.min.data,
				select.selec8.collect_succ_finish.sec.data);
		if(select.selec8.collect_succ_star.year.data == 0xffff)//开始时间和结束时间置为无效0xff
		{
			fprintf(stderr,"\n默认上报区段\n");
			memset(&ts_start,0xff,sizeof(TS));
			memset(&ts_end,0xff,sizeof(TS));
			day_num = 1;
		}
		else
		{
			day_num = getdaynum(select.selec8.collect_succ_star,select.selec8.collect_succ_finish);
			TimeBCDToTs(select.selec8.collect_succ_star,&ts_start);//
			TimeBCDToTs(select.selec8.collect_succ_finish,&ts_end);//
			TSGet(&ts_now);
			time_now = tstotime_t(ts_now);
			time_end = tstotime_t(ts_end);
			fprintf(stderr,"\nday_num=%d time_now=%d time_end=%d\n",day_num,time_now,time_end);
			if(time_end > time_now)
				ts_end = ts_now;
			if(day_num<0)
			return 0;
		}
		switch(day_num)
		{
		case 0:
			frmnum = dealselect10(oad_h,csds,1,tsa_num,tsa_group,frmmaxsize);//上报当前数据
			break;
		case 1:
			zc_sec = TItoSec(select.selec8.ti);
			fprintf(stderr,"\n招测间隔zc_sec=%d\n",zc_sec);
			frmnum = dealselect5(oad_h,csds,ts_start,ts_end,zc_sec,tsa_num,tsa_group,frmmaxsize,0);
			break;
		default:
			frmnum = dealselect7(oad_h,csds,tsa_group,ts_start,ts_end,frmmaxsize);//找某一时间段的值
			break;
		}
		break;
	case 0xee://事件
		frmnum = dealevent(oad_h,road_eve,csds,tsa_num,tsa_group,frmmaxsize);
//		dealevent(OAD oad_h,CSD_ARRAYTYPE csds,INT16U tsa_num,CLASS_6001 *tsa_group,INT16U frmmaxsize)
			break;
	case 10://找最新的n条记录
		frmnum = dealselect10(oad_h,csds,select.selec10.recordn,tsa_num,tsa_group,frmmaxsize);
		break;
	}
	if(tsa_group != NULL)
		free(tsa_group);
	fprintf(stderr,"\n帧总数frmnum=%d\n",frmnum);
	return frmnum;
}
//返回oad的三个时标加oad值
INT16S getTASKoaddata(INT8U taskid,INT8U frz_type,TS ts,TSA tsa,TASK_OADDATA *oaddata)
{
	char fname[FILENAMELEN]={};
	INT8U tsadata[18],recdatabuf[1024];
	INT16U curseq=0;
	INT32U blklen=0,headlen=0;
	int i = 0;
	HEADFIXED_INFO taskhead_info;
	HEAD_UNIT headoad_unit[FILEOADMAXNUM];
	OAD_INDEX oad_offset;
	ROAD_ITEM item_road;
	TS ts_file;
	FILE *fp = NULL;
	ts_file = ts;
	memset(&taskhead_info,0x00,sizeof(HEADFIXED_INFO));
	memset(headoad_unit,0,sizeof(headoad_unit));
	memset(recdatabuf,0x00,sizeof(recdatabuf));
	memset(&item_road,0x00,sizeof(ROAD_ITEM));

	item_road.oadmr_num = 1;
	item_road.oad[0].oad_num = 1;
	item_road.oad[0].oad_m = oaddata->oaddata.oad_m;
	item_road.oad[0].oad_r = oaddata->oaddata.oad_r;

	getFILEts(frz_type,&ts_file);
	getTaskFileName(taskid,ts_file,fname);
	fp = fopen(fname,"r");//读取格式
	if(fp == NULL)//文件不存在，返回空
	{
		fprintf(stderr,"\n打开文件%s失败!!!\n",fname);
		return -1;
	}
	else
	{
		fread(&taskhead_info,sizeof(HEADFIXED_INFO),1,fp);
		fread(headoad_unit,taskhead_info.oadnum*sizeof(HEAD_UNIT),1,fp);
	}
	blklen = taskhead_info.reclen*taskhead_info.seqnum;
	headlen = sizeof(HEADFIXED_INFO)+taskhead_info.oadnum*sizeof(HEAD_UNIT);
	fprintf(stderr,"\nreclen=%d,seqnum=%d,seqsec=%d,blklen=%d,oadnum=%d\n",taskhead_info.reclen,taskhead_info.seqnum,taskhead_info.seqsec,blklen,taskhead_info.oadnum);
	fprintf(stderr,"\nts:%d:%d seqsec=%d\n",ts.Hour,ts.Sec,taskhead_info.seqsec);
	if(taskhead_info.seqsec==0)
		curseq = 0;
	else
		curseq = (ts.Hour*60*60+ts.Minute*60+ts.Sec)/taskhead_info.seqsec;//从0开始,计算要取的记录
	fprintf(stderr,"\ncurseq=%d\n",curseq);

	memset(&oad_offset,0x00,sizeof(OAD_INDEX));
	fprintf(stderr,"\noad扩(%d):\n",item_road.oadmr_num);
	for(i=0;i<item_road.oadmr_num;i++)
	{
		fprintf(stderr," %04x-%04x\n",item_road.oad[i].oad_m.OI,item_road.oad[i].oad_r.OI);
	}

	GetOADPosofUnit(item_road,headoad_unit,taskhead_info.oadnum,&oad_offset);//得到每一个oad在块数据中的偏移
	if(oad_offset.len == 0)
		return 0;

	fseek(fp,headlen,SEEK_SET);//跳过文件头
	while(!feof(fp))//轮寻文件
	{
		fprintf(stderr,"\n---1--%d--%d--%d\n",sizeof(TSA)+1,sizeof(tsadata),ftell(fp));
		if(fread(tsadata,sizeof(TSA)+1,1,fp)==0)//0x55 + tsa
		{
			fprintf(stderr,"\n文件尾\n");
			break;
		}
		else
			fprintf(stderr,"\n---2  \n");
		fprintf(stderr,"\n---tsadata[0]=%d tsadata[1]=%d tsa.addr[0]=%d\n",tsadata[0],tsadata[1],tsa.addr[0]);
		prtTSA(tsa);
		PRTbuf(&tsadata[1],tsadata[1]+1);
		if(memcmp(&tsadata[1],tsa.addr,tsa.addr[0])==0 && tsadata[0] == 0x55)//
		{
			fprintf(stderr,"\n地址匹配\n");
			fseek(fp,taskhead_info.reclen*curseq,SEEK_CUR);//跳到要抄找的记录s的位置
			fread(recdatabuf,taskhead_info.reclen,1,fp);

			memcpy(oaddata->time_save,&recdatabuf[0],sizeof(oaddata->time_save));
			memcpy(oaddata->time_end,&recdatabuf[8],sizeof(oaddata->time_end));
			memcpy(oaddata->time_start,&recdatabuf[16],sizeof(oaddata->time_start));//赋值三个时标
			memcpy(oaddata->oaddata.data,&recdatabuf[oad_offset.offset],oad_offset.len);//赋值oad数据

			oaddata->oaddata.datalen = oad_offset.len;
			PRTbuf(oaddata->oaddata.data,oaddata->oaddata.datalen);

			if(fp != NULL)
				fclose(fp);
			return oad_offset.len;
		}
		else
			fseek(fp,blklen,SEEK_CUR);//地址不匹配
	}
	if(fp != NULL)
		fclose(fp);
	return 0;
}
//得到某一个oad某一时刻的数据数据,返回的数据包括三个时标 成功返回1
INT8U GetOADTaskInfo(TS ts,CLASS_6001 tsa_6001,TASK_OADDATA *oaddata)
{
	INT8U frz_type = 0;
	int i=0;
	CLASS_6015 class6015;
	CLASS_6013 class6013;
	memset(&class6013,0,sizeof(CLASS_6013));
	memset(&class6015,0,sizeof(CLASS_6015));
	for(i=0;i<256;i++)
	{
		if((frz_type=get60136015info(i+1,&class6015,&class6013))==0)//
			continue;
		if(cmpTSAtype(&tsa_6001,class6015)==0)//比对tsa类型，不符和本采集方案的跳过
			continue;
		if(cmpOADintask(oaddata->oaddata.oad_m,oaddata->oaddata.oad_r,class6015)==1)
			return getTASKoaddata(i+1,frz_type,ts,tsa_6001.basicinfo.addr,oaddata);
	}
	return 0;
}

//得到某一个oad某一时刻的数据数据,返回数据长度
INT16S getOADdata(CLASS_6001 tsa_6001,TS ts,TASK_OADDATA *oaddata)
{
	INT8U frz_type = 0;
	int i=0,ret=-1;
	CLASS_6015 class6015;
	CLASS_6013 class6013;
	memset(&class6013,0,sizeof(CLASS_6013));
	memset(&class6015,0,sizeof(CLASS_6015));
	for(i=0;i<256;i++)
	{
		if((frz_type=get60136015info(i+1,&class6015,&class6013))==0)//
			continue;
		if(cmpTSAtype(&tsa_6001,class6015)==0)//比对tsa类型，不符和本采集方案的跳过
			continue;
		if(cmpOADintask(oaddata->oaddata.oad_m,oaddata->oaddata.oad_r,class6015)==1)
		{
			return getTASKoaddata(i+1,frz_type,ts,tsa_6001.basicinfo.addr,oaddata);
		}
	}
	return -1;
}
INT16S GUI_GetFreezeData(INT8U dayormon,CLASS_6001 tsa,TS ts_zc,INT8U *databuf)
{
	TASK_OADDATA oaddata;
	INT16U indexn=0;
	if(dayormon == 1){
		oaddata.oaddata.oad_m.OI = 0x5004;//日冻结对象
		oaddata.oaddata.oad_m.attflg = 0x02;
		oaddata.oaddata.oad_m.attrindex = 0x00;
	} else if(dayormon == 2){
		oaddata.oaddata.oad_m.OI = 0x5006;//月冻结对象
		oaddata.oaddata.oad_m.attflg = 0x02;
		oaddata.oaddata.oad_m.attrindex = 0x00;
	} else
		return 0;
	oaddata.oaddata.oad_r.OI = 0x0010;//正向有功电能量
	oaddata.oaddata.oad_r.attflg = 0x02;
	oaddata.oaddata.oad_r.attrindex = 0x00;
	if(getOADdata(tsa,ts_zc,&oaddata) != -1)
	{
		fprintf(stderr,"\n数据长度%d\n",oaddata.oaddata.datalen);
		databuf[indexn++] = 8;//时标长度
		memcpy(&databuf[indexn],oaddata.time_save,sizeof(oaddata.time_save));
		indexn+=8;
		databuf[indexn++] = oaddata.oaddata.datalen;//时标长度
		memcpy(&databuf[indexn],oaddata.oaddata.data,oaddata.oaddata.datalen);
		indexn+=oaddata.oaddata.datalen;
		PRTbuf(databuf,indexn);
		return 1;
	}
	return -1;
}
INT16U getCBsuctsanum(INT8U taskid,TS ts)
{
	char fname[FILENAMELEN]={};
	INT8U tsadata[18],time_frz[24];//三个时标
	INT8U frz_type = 0;
	INT16U curseq=0,tsa_sucnum=0;
	INT32U blklen=0,headlen=0;
	long int tsa_pos = 0;
	FILE *fp = NULL;
	TS ts_file;

	HEADFIXED_INFO taskhead_info;
	HEAD_UNIT headoad_unit[FILEOADMAXNUM];
	CLASS_6015 class6015;
	CLASS_6013 class6013;

	memset(&class6013,0,sizeof(CLASS_6013));
	memset(&class6015,0,sizeof(CLASS_6015));
	memset(fname,0x00,sizeof(fname));
	memset(&taskhead_info,0x00,sizeof(HEADFIXED_INFO));
	memset(headoad_unit,0,sizeof(headoad_unit));

	if((frz_type=get60136015info(taskid,&class6015,&class6013))==0)//
		return 0;

	ts_file = ts;
	getFILEts(frz_type,&ts_file);
	getTaskFileName(taskid,ts_file,fname);
	fprintf(stderr,"fname=%s\n",fname);

	fp = fopen(fname,"r");//读取格式
	if(fp == NULL)//文件不存在，返回空
	{
		fprintf(stderr,"\n打开文件%s失败!!!\n",fname);
		return 0;
	}
	else
	{
		fread(&taskhead_info,sizeof(HEADFIXED_INFO),1,fp);
		fread(headoad_unit,taskhead_info.oadnum*sizeof(HEAD_UNIT),1,fp);
	}
	blklen = taskhead_info.reclen*taskhead_info.seqnum;
	headlen = sizeof(HEADFIXED_INFO)+taskhead_info.oadnum*sizeof(HEAD_UNIT);
	fprintf(stderr,"\nreclen=%d,seqnum=%d,seqsec=%d,blklen=%d,oadnum=%d\n",taskhead_info.reclen,taskhead_info.seqnum,taskhead_info.seqsec,blklen,taskhead_info.oadnum);
	fprintf(stderr,"\nts:%d:%d seqsec=%d\n",ts.Hour,ts.Sec,taskhead_info.seqsec);
	if(taskhead_info.seqsec==0)
		curseq = 0;
	else
		curseq = (ts.Hour*60*60+ts.Minute*60+ts.Sec)/taskhead_info.seqsec;//从0开始,计算要取的记录
	fprintf(stderr,"\n查找curseq=%d\n",curseq);

	fseek(fp,headlen,SEEK_SET);//跳过文件头
	while(!feof(fp))//轮寻文件
	{
		fprintf(stderr,"\n---1--%d--%d--%d\n",sizeof(TSA)+1,sizeof(tsadata),ftell(fp));
		if(fread(tsadata,sizeof(TSA)+1,1,fp)==0)//0x55 + tsa
		{
			fprintf(stderr,"\n文件尾\n");
			break;
		}
		else
			fprintf(stderr,"\n---2\n");
		tsa_pos = ftell(fp);
		if(tsadata[0] != 0)
		{
			fseek(fp,taskhead_info.reclen*curseq,SEEK_CUR);//
			fread(time_frz,3*sizeof(time_frz),1,fp);
			if(time_frz[0] != 0 && time_frz[8] != 0 && time_frz[16] != 0)//三个时标存在
				tsa_sucnum++;
		}
		fseek(fp,tsa_pos+blklen,SEEK_SET);//
	}
	if(fp != NULL)
		fclose(fp);
	return tsa_sucnum;
}
//INT8U GetOADData(TS ts,CLASS_6001 tsa_6001,TASK_OADDATA *oaddata)
//INT16S GUI_GetFreezeData(CLASS_6001 tsa_6001,TS ts_zc,TASK_OADDATA *oaddata)
//{
//	return GetOADData(tsa_6001,ts_zc,oaddata);
//}
INT8U GetTaskidFromCSDs_Sle0(CSD_ARRAYTYPE csds,ROAD_ITEM *item_road,INT8U findmethod,CLASS_6001 *tsa)
{
	return 0;
}
INT16S GetTaskHead(FILE *fp,INT16U *head_len,INT16U *tsa_len,HEAD_UNIT **head_unit)
{
	return 0;
}

void deloutofdatafile()//删除过期任务数据文件
{
	INT8U frz_type=0;
	INT32U seqnum=0,seqsec=0;
	int i=0,taskday,fileday;
	char dirname[60];
	char 	cmdstr[100];
	DIR *dir;
	struct dirent *ptr;
	struct tm tm_p,tm_f;
	time_t time_s,time_p,time_f;

	CLASS_6015 class6015;
	CLASS_6013 class6013;
	memset(&class6013,0,sizeof(CLASS_6013));
	memset(&class6015,0,sizeof(CLASS_6015));

	time_p = time(NULL);
	localtime_r(&time_p,&tm_p);
	tm_p.tm_min = 0;
	tm_p.tm_sec = 0;
	tm_f = tm_p;
	time_p = mktime(&tm_p);
	for(i=0;i<256;i++)
	{
		memset(dirname,0x00,60);
		sprintf(dirname,"/nand/task/%03d/",i+1);
		if(access(dirname,F_OK)!=0)//文件不存在
		{
			continue;
		}

		if((frz_type=get60136015info(i+1,&class6015,&class6013))==0)//
		{
			fprintf(stderr,"\n任务%d存储类型为%d\n",i+1,frz_type);
			continue;
		}
		fprintf(stderr,"\n任务%d存储类型为%d\n",i+1,frz_type);
		seqnum = getTASKruntimes(class6013,class6015,&seqsec);
		fprintf(stderr,"\n每天存储记录个数seqnum=%d,seqsec=%d\n",seqnum,seqsec);
		if(seqnum == 0)
			return;

		taskday = (class6015.deepsize * seqsec)/86400;
		if((class6015.deepsize * seqsec)%86400 != 0)
			taskday++;
		if (seqsec==0 || class6015.deepsize==0)
		{
			taskday = 12;
			asyslog(LOG_INFO,"\nfreq %d  memdep %d  \n",seqsec,class6015.deepsize);
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
				sprintf(cmdstr,"/nand/task/%03d/%s",i+1,ptr->d_name);
				asyslog(LOG_NOTICE,"删除文件[%s]",cmdstr);
				unlink(cmdstr);
			}
		}
		if (dir!=NULL) 	closedir(dir);
	}
}

#define	FILE_LINE		__FILE__,__FUNCTION__,__LINE__
#define	DEBUG_FUNC_LINE(format, ...)	debugToStderr(FILE_LINE, format, ##__VA_ARGS__)

void debugToStderr(const char* file, const char* func, INT32U line, const char *fmt, ...)
{
	va_list ap;

	fprintf(stderr, "\n[%s][%s()][%d]: ", file, func, line);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

/*
 * 上报某一时间段数据记录，这一时间段跨日
 */
INT16U selectData(OAD oad_h,CSD_ARRAYTYPE csds,CLASS_6001 *tsa_group, INT16U tsa_num, TS ts_start,TS ts_end)
 {
	char fname[FILENAMELEN] = { };
	INT8U taskid, recorddata[2048], frmdata[2048], seqnumindex = 0,
			frz_type = 0;
	INT16U frmnum = 0, tsa_findnum = 0, recordnum = 0, seq_start = 0, seq_end = 0,
			indexn = 0, day_sec = 0, frmmaxsize = 512;
	INT32U blklen = 0, headlen = 0, seqsec = 0, offset = 0;
	int i = 0, j = 0;
	time_t time_end, time_start;
	ROAD_ITEM item_road;
	CLASS_6015 class6015;
	CLASS_6013 class6013;
	HEAD_UNIT headoad_unit[FILEOADMAXNUM];
	FILE *fp = NULL, *frm_fp = NULL;
	HEADFIXED_INFO taskhead_info;
	OAD_INDEX oad_offset[FILEOADMAXNUM];
	TS ts_file;

	memset(frmdata, 0, sizeof(frmdata));

	memset(&item_road, 0, sizeof(item_road));
	extendcsds(csds, &item_road);
	if ((taskid = GetTaskidFromCSDs(item_road, tsa_group)) == 0) {//暂时不支持招测的不在一个采集方案
		//处理招测的oad不在一个任务
		asyslog(LOG_INFO, "GetTaskData: taskid=%d\n", taskid);
		return 0;
	}
	//------------------------------------------------------------------------------打开任务文件，并获得任务文件头信息
	if ((frz_type = get60136015info(taskid, &class6015, &class6013)) == 0)//获取任务参数
			{
		fprintf(stderr, "\n获取任务%d配置失败\n", taskid);
		return 0;
	}

	if (ts_start.Year == 0xffff) {
		fprintf(stderr, "\n任务%d存储类型为%d\n", taskid, frz_type);
		seqsec = TItoSec(class6013.interval);
		fprintf(stderr, "\n每天存储记录个数seqsec=%d\n", seqsec);
		if (seqsec == 0)
			seqsec = 86400;
		TSGet(&ts_end);
		time_end = tstotime_t(ts_end);
		day_sec = ((ts_end.Hour * 3600 + ts_end.Minute * 60 + ts_end.Sec)
				/ seqsec) * seqsec;
		fprintf(stderr, "\nday_sec=%d\n", day_sec);
		ts_end.Hour = 0;
		ts_end.Minute = 0;
		ts_end.Sec = 0;
		time_end = tstotime_t(ts_end) + day_sec - seqsec;		//上报上一次
		time_start = time_end - seqsec;
		time_ttots(&ts_start, time_start);
		time_ttots(&ts_end, time_end - 1);
	}
	fprintf(stderr, "\n取数时间范围%d-%d-%d %d:%d:%d---%d-%d-%d %d:%d:%d\n",
			ts_start.Year, ts_start.Month, ts_start.Day, ts_start.Hour,
			ts_start.Minute, ts_start.Sec, ts_end.Year, ts_end.Month,
			ts_end.Day, ts_end.Hour, ts_end.Minute, ts_end.Sec);
	ts_file = ts_start;

	if ((class6015.savetimeflag == 3 || class6015.savetimeflag == 4)
			&& frz_type == 1)
		;
	else
		getFILEts(frz_type, &ts_file);

	getTaskFileName(taskid, ts_file, fname);
	fprintf(stderr, "\n打开文件%s\n", fname);
	fp = fopen(fname, "r");		//读取格式
	if (fp == NULL)		//文件不存在，返回空
	{
		fprintf(stderr, "\n打开文件%s失败!!!\n", fname);
		return 0;
	} else {
		fread(&taskhead_info, sizeof(HEADFIXED_INFO), 1, fp);
		fread(headoad_unit, taskhead_info.oadnum * sizeof(HEAD_UNIT), 1, fp);
	}
	seq_start = (ts_start.Hour * 60 * 60 + ts_start.Minute * 60 + ts_start.Sec)
			/ taskhead_info.seqsec;
	seq_end = (ts_end.Hour * 60 * 60 + ts_end.Minute * 60 + ts_end.Sec)
			/ taskhead_info.seqsec + 1;
	if (ts_start.Year != ts_end.Year || ts_start.Month != ts_end.Month
			|| ts_start.Day != ts_end.Day)
		return 0;

	blklen = taskhead_info.reclen * taskhead_info.seqnum;
	headlen = sizeof(HEADFIXED_INFO) + taskhead_info.oadnum * sizeof(HEAD_UNIT);
	fprintf(stderr,
			"\nheadlen=%d,reclen=%d,seqnum=%d,seqsec=%d,blklen=%d,oadnum=%d\n",
			headlen, taskhead_info.reclen, taskhead_info.seqnum,
			taskhead_info.seqsec, blklen, taskhead_info.oadnum);
	//------------------------------------------------------------------------------获得招测的TSA在文件中的偏移块数
	tsa_findnum = getTSAblkoffnum(tsa_group, tsa_num, blklen, headlen, fp);
	fprintf(stderr, "\ntsa_offnum=%d\n", tsa_findnum);
	if (tsa_findnum == 0)
		return 0;
	fprintf(stderr, "\n----1\n");
	if (seq_end > taskhead_info.seqnum)
		seq_end = taskhead_info.seqnum;
	fprintf(stderr, "\n招测序号%d--%d\n", seq_start, seq_end);

	//------------------------------------------------------------------------------获得每个招测的oad在一条记录中的偏移
	memset(oad_offset, 0x00, sizeof(oad_offset));
	GetOADPosofUnit(item_road, headoad_unit, taskhead_info.oadnum, oad_offset);	//得到每一个oad在块数据中的偏移

	//------------------------------------------------------------------------------提取记录并组帧
	frm_fp = openFramefile(REPORT_FRAME_DATA,0);
	memset(frmdata, 0, sizeof(frmdata));
	//初始化分帧头
	indexn = 2;
	indexn += initFrameHead(&frmdata[indexn], oad_h, csds, &seqnumindex);
	for (i = 0; i < tsa_num; i++) {
		if (tsa_group[i].extinfo.pt == 0) {//没有这个地址的数据
			continue;
		}
		for (j = seq_start; j < seq_end; j++) {
			memset(recorddata, 0x00, sizeof(recorddata));
			fprintf(stderr, "\n序号%d 记录位置%d\n", j,
					headlen + (blklen + TSA_LEN + 1) * tsa_group[i].extinfo.pt
							+ TSA_LEN + 1 + taskhead_info.reclen * j);
			offset = headlen
					+ (blklen + TSA_LEN + 1)
							* (tsa_group[i].extinfo.pt - 1) + TSA_LEN
					+ 1 + taskhead_info.reclen * j;
			fseek(fp, offset, SEEK_SET);//跳到要抄找的记录的位置
			fread(recorddata, taskhead_info.reclen, 1, fp);
			fprintf(stderr, "\n数据%02x %02x\n", recorddata[0], recorddata[1]);
			if (recorddata[0] == 0) {
				fprintf(stderr, "\n记录%d为空，不取数据%02x %02x\n", j, recorddata[0],
						recorddata[1]);
				continue;
			}
			PRTbuf(recorddata, taskhead_info.reclen);
			indexn += getrecdata(recorddata, tsa_group[i].basicinfo.addr,
					item_road, oad_offset, &frmdata[indexn]);
			recordnum++;
			fprintf(stderr, "\nindexn=%d,frmmaxsize=%d\n", indexn, frmmaxsize);
			if (indexn >= frmmaxsize - 100) {
				frmnum++;
				intToBuf((indexn - 2), frmdata);		//帧长度保存帧的数据长度
				frmdata[seqnumindex] = recordnum;
				saveOneFrame(frmdata, indexn, frm_fp);
				indexn = 2;
				indexn += initFrameHead(&frmdata[indexn], oad_h, csds,
						&seqnumindex);
				recordnum = 0;
			}
		}
	}

	fprintf(stderr, "\nrecordnum=%d\n", recordnum);
	if (frmnum == 0) {
		frmnum = 1; //一帧
		fprintf(stderr,
				"\n indexn = %d saveOneFrame  seqnumindex=%d,  recordnum=%d!!!!!!!!!!!!!!!!\n",
				indexn, seqnumindex, recordnum);
		asyslog(LOG_INFO,
				"任务数据文件组帧:indexn = %d , seqnumindex=%d,  recordnum=%d\n",
				indexn, seqnumindex, recordnum);
		intToBuf((indexn - 2), frmdata);
		frmdata[seqnumindex] = recordnum;
		saveOneFrame(frmdata, indexn, frm_fp);
	} else {
		if (recordnum != 0) {
			frmnum++;
			fprintf(stderr,
					"\n last frm indexn = %d saveOneFrame  seqnumindex=%d,  recordnum=%d!!!!!!!!!!!!!!!!\n",
					indexn, seqnumindex, recordnum);
			asyslog(LOG_INFO,
					"任务数据文件组帧:indexn = %d , seqnumindex=%d,  recordnum=%d\n",
					indexn, seqnumindex, recordnum);
			intToBuf((indexn - 2), frmdata);
			frmdata[seqnumindex] = recordnum;
			saveOneFrame(frmdata, indexn, frm_fp);
		}
	}
	if (frm_fp != NULL)
		fclose(frm_fp);
	return frmnum;
}

void supplementRpt(TS ts1, TS ts2, INT8U retaskid, INT8U *saveflg)
{
	INT16U tsa_num = 0; //上报的序号
	CLASS_601D class601d = {};
	CLASS_6001 *tsa_group = NULL;//本次召测的tsa集合
	MY_MS meters_cur;

	*saveflg = 0;

	if (access("/nand/reportdata", F_OK) == 0) {
		asyslog(LOG_INFO, "文件%s存在，退出！！！", "/nand/reportdata");
		goto Ret;
	}

	if (readCoverClass(0x601D, retaskid, &class601d, sizeof(CLASS_601D),
			coll_para_save) != 1) {
		system("rm /nand/reportdata");
		fprintf(stderr, "\n获取任务%d的采集方案失败\n", retaskid);
		goto Ret;
	}

	if(TScompare(ts1, ts2) == 1) {
		DEBUG_FUNC_LINE("错误: 开始时间大于结束时间, 退出!");
		system("rm /nand/reportdata");
		goto Ret;
	}

	meters_cur = getSELEMS(class601d.reportdata.data.recorddata.selectType,
			class601d.reportdata.data.recorddata.rsd);
	tsa_num = getOI6001(meters_cur,(INT8U **)&tsa_group);

	selectData(class601d.reportdata.data.recorddata.oad,
			class601d.reportdata.data.recorddata.csds,
			tsa_group, tsa_num, ts1, ts2);

	sleep(2);
	*saveflg = 1;

Ret:
	if (tsa_group != NULL)
		free(tsa_group);

	return;
}
