
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>
#include "sys/reboot.h"
#include <wait.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "read485.h"
#include "dlt698def.h"
#include "cjdeal.h"

#include <stdarg.h>
#include <sys/stat.h>

extern ProgramInfo* JProgramInfo;
extern INT8U poweroffon_state;
extern MeterPower MeterPowerInfo[POWEROFFON_NUM];
#ifdef TESTDEF
INT8U flag07_0CF177[4] =  {0x00,0xff,0x00,0x00};//当前组合有功总电能示值
INT8U flag07_0CF33[4] =   {0x00,0xff,0x01,0x00};//当前正向有功总电能示值
INT8U flag07_0CF33_1[4] =   {0x01,0xff,0x01,0x00};//当前正向有功总电能示值1
INT8U flag07_0CF34[4] =   {0x00,0xff,0x02,0x00};//当前反向有功总电能示值
INT8U flag07_0CF34_1[4] =   {0x01,0xff,0x02,0x00};//当前反向有功总电能示值1
INT8U flag07_0CZHWG1[4] =  {0x00,0x01,0x30,0x00};//当前组合无功1
INT8U flag07_0CZHWG2[4] =  {0x00,0x01,0x40,0x00};//当前组合无功2
INT8U flag07_0C1XXWG[4] =  {0x00,0xff,0x50,0x00};//第一象限无功
INT8U flag07_0C2XXWG[4] =  {0x00,0xff,0x60,0x00};//第二象限无功
INT8U flag07_0C3XXWG[4] =  {0x00,0xff,0x70,0x00};//第三象限无功
INT8U flag07_0C4XXWG[4] =  {0x00,0xff,0x80,0x00};//第四象限无功
INT8U flag07_0CF25_1[4] = {0x00,0xff,0x01,0x02};//当前电压
INT8U flag07_0CF25_1_A[4] = {0x00,0x01,0x01,0x02};//当前A相电压
INT8U flag07_0CF25_2[4] = {0x00,0xff,0x02,0x02};//当前电流
INT8U flag07_0CF25_2_A[4] = {0x00,0x01,0x02,0x02};//当前A相电流
INT8U flag07_0CF25_3[4] = {0x00,0xff,0x03,0x02};//当前有功功率
INT8U flag07_0CF25_4[4] = {0x00,0xff,0x04,0x02};//当前无功功率
INT8U freezeflag07_1[4] = {0x01,0x00,0x06,0x05};//上一次日冻结时标
INT8U freezeflag07_2[4] = {0x01,0x01,0x06,0x05};//上一次日冻结正向有功总电能示值
INT8U freezeflag07_3[4] = {0x01,0x02,0x06,0x05};//上一次日冻结反向有功总电能示值
INT8U flag07_date[4] 	= {0x01,0x01,0x00,0x04};//电能表日历时钟-日期
INT8U flag07_time[4]	= {0x02,0x01,0x00,0x04};//电能表日历时钟-时间
INT8U flag07_tingshangdian[4] = {0x01,0x00,0x11,0x03};//上一次停上电记录
#endif


void DbgPrintToFile1(INT8U comport,const char *format,...)
{
	char str[50];
	char fname[100];
	char tmpcmd[256];
	time_t cur_time;
	struct tm cur_tm;
	FILE *fp = NULL;

	memset(fname,0,sizeof(fname));
	sprintf(fname,"/nand/vs485_%d.log",comport);
	memset(str,0,50);
	cur_time=time(NULL);
	localtime_r(&cur_time,&cur_tm);
	sprintf(str, "\n[%04d-%02d-%02d %02d:%02d:%02d]",
			cur_tm.tm_year+1900, cur_tm.tm_mon+1, cur_tm.tm_mday,
			cur_tm.tm_hour, cur_tm.tm_min, cur_tm.tm_sec);

	fp = fopen(fname, "a+");//内容存入文件
	if (fp != NULL)
	{
		va_list ap;
	    va_start(ap,format);
		vfprintf(fp,str,ap);
	    if(fp)
			vfprintf(fp,format,ap);
	    va_end(ap);
	    fflush(fp);
		fclose(fp);
	}

	struct stat fileInfo;
	stat(fname, &fileInfo);
	if (fileInfo.st_size>3000*1000)//超过300K
	{
		memset(tmpcmd,0,sizeof(tmpcmd));
		sprintf(tmpcmd,"cp %s %s.0",fname,fname);
		system(tmpcmd);
		sleep(3);
		memset(tmpcmd,0,sizeof(tmpcmd));
		sprintf(tmpcmd,"rm %s",fname);
		system(tmpcmd);
	}
}

void myBCDtoASC1(char val, char dest[2])
{
	int i=0;
	char c[2];
	c[0]=0; c[1]=0;
	c[0] = (val>>4) & 0x0f;
	c[1] = val & 0x0f;
	for(i=0; i<2; i++)
	{
		//if(c[i]>=0 && c[i]<=9)
		if(c[i]<=9)
			dest[i] = c[i] + '0';
		if(c[i]==10)
			dest[i] = 'a';
		if(c[i]==11)
			dest[i] = 'b';
		if(c[i]==12)
			dest[i] = 'c';
		if(c[i]==13)
			dest[i] = 'd';
		if(c[i]==14)
			dest[i] = 'e';
		if(c[i]==15)
			dest[i] = 'f';
	}
}


void DbPrt1(INT8U comport,char *prefix, char *buf, int len, char *suffix)
{
	//return ;
	char str[50], tmpbuf[2048], c[2], c1[2], c2[2];
	int i=0;
	memset(c, 0, 2);
	memset(str, 0, 50);
	memset(tmpbuf, 0, 2048);

	int count= 0;
	int prtlen=0;
	int k=0;
	while(1)
	{
		memset(c, 0, 2);
		memset(str, 0, 50);
		memset(tmpbuf, 0, 2048);
		if(len<=512)
		{
			prtlen = len;
		}else
		{
			if(k<len/512)
			{
				k++;
				prtlen = 512;
			}else
			{
				prtlen = len%512;
			}
		}
		if(prefix!=NULL)
		{
			sprintf(str, "%s[%d] ", prefix,prtlen);
			strcat(tmpbuf, str);
		}
		for(i=0; i<prtlen; i++)
		{
			memset(c, 0, 2);
			memset(c1, 0, 2);
			memset(c2, 0, 2);
			myBCDtoASC1(buf[i+count], c);

			c1[0] = c[0];
			c2[0] = c[1];
			strcat(tmpbuf, c1);
			strcat(tmpbuf, c2);
			strcat(tmpbuf, " ");
		}
		if(suffix!=NULL)
			strcat(tmpbuf, suffix);
		DbgPrintToFile1(comport,tmpbuf);
		count += prtlen;
		if(count>=len)
			break;
	}
}


typedef enum {
	coll_bps = 1,
	coll_protocol,
	coll_wiretype,
	task_ti,
	task_cjtype,
	task_prio,
	task_status,
	task_runtime,
	coll_mode,
	ms_type,
	savetime_sel,
} OBJ_ENUM;
char *getenum(int type, int val) {
	char name1[128] = { };
	char *name = NULL;

	name = name1;
	memset(name1, 0, sizeof(name1));
//	fprintf(stderr,"val=%d ,type=%d\n",val,type);
	switch (type) {
	case coll_bps:
		if (val == bps300)
			strcpy(name, "300");
		if (val == bps600)
			strcpy(name, "600");
		if (val == bps1200)
			strcpy(name, "1200");
		if (val == bps2400)
			strcpy(name, "2400");
		if (val == bps4800)
			strcpy(name, "4800");
		if (val == bps7200)
			strcpy(name, "7200");
		if (val == bps9600)
			strcpy(name, "9600");
		if (val == bps19200)
			strcpy(name, "19200");
		if (val == bps38400)
			strcpy(name, "38400");
		if (val == bps57600)
			strcpy(name, "57600");
		if (val == bps115200)
			strcpy(name, "115200");
		if (val == autoa)
			strcpy(name, "自适应");
		break;
	case coll_protocol:
		if (val == 0)
			strcpy(name, "未知");
		if (val == 1)
			strcpy(name, "DL/T645-1997");
		if (val == 2)
			strcpy(name, "DL/T645-2007");
		if (val == 3)
			strcpy(name, "DL/T698.45");
		if (val == 4)
			strcpy(name, "CJ/T18802004");
		break;
	case coll_wiretype:
		if (val == 0)
			strcpy(name, "未知");
		if (val == 1)
			strcpy(name, "单相");
		if (val == 2)
			strcpy(name, "三相三线");
		if (val == 3)
			strcpy(name, "三相四线");
		break;
	case task_ti:
		if (val == 0)
			strcpy(name, "秒");
		if (val == 1)
			strcpy(name, "分");
		if (val == 2)
			strcpy(name, "时");
		if (val == 3)
			strcpy(name, "日");
		if (val == 4)
			strcpy(name, "月");
		if (val == 5)
			strcpy(name, "年");
		break;
	case task_cjtype:
		if (val == 1)
			strcpy(name, "普通采集方案");
		if (val == 2)
			strcpy(name, "事件采集方案");
		if (val == 3)
			strcpy(name, "透明方案");
		if (val == 4)
			strcpy(name, "上报方案");
		if (val == 5)
			strcpy(name, "脚本方案");
		break;
	case task_prio:
		if (val == 1)
			strcpy(name, "首要");
		if (val == 2)
			strcpy(name, "必要");
		if (val == 3)
			strcpy(name, "需要");
		if (val == 4)
			strcpy(name, "可能");
		break;
	case task_status:
		if (val == 1)
			strcpy(name, "正常");
		if (val == 2)
			strcpy(name, "停用");
		break;
	case task_runtime:
		if (val == 0)
			strcpy(name, "前闭后开");
		if (val == 1)
			strcpy(name, "前开后闭");
		if (val == 2)
			strcpy(name, "前闭后闭");
		if (val == 3)
			strcpy(name, "前开后开");
		break;
	}
//	fprintf(stderr,"get name=%s\n",name);
	return name;
}
INT32U getMeterBaud(INT8U bps)
{
	if (bps == bps1200)
		return 1200;
	if (bps == bps4800)
		return 4800;
	if (bps == bps7200)
		return 7200;
	if (bps == bps9600)
		return 9600;
	if (bps == bps115200)
		return 115200;

	return 2400;
}
void print6013(CLASS_6013 class6013) {
	fprintf(stderr, "\n----------------------------------");
	fprintf(stderr, "【6013】任务配置单元: 任务ID--%04x\n", class6013.taskID);
	fprintf(stderr,
			"[1]执行频率 [2]方案类型 [3]方案编号 [4]开始时间 [5]结束时间 [6]延时 [7]执行优先级 [8]状态 [9]开始前脚本id [10]开始后脚本id [11]运行时段【起始HH:MM 结束HH:MM】\n");
	fprintf(stderr, "[1]%s-%d ", getenum(task_ti, class6013.interval.units),
			class6013.interval.interval);
	fprintf(stderr, "[2]%s  [3]%d   ", getenum(task_cjtype, class6013.cjtype),
			class6013.sernum);
	fprintf(stderr, "[4]%d-%d-%d %d:%d:%d ", class6013.startime.year.data,
			class6013.startime.month.data, class6013.startime.day.data,
			class6013.startime.hour.data, class6013.startime.min.data,
			class6013.startime.sec.data);
	fprintf(stderr, "[5]%d-%d-%d %d:%d:%d ", class6013.endtime.year.data,
			class6013.endtime.month.data, class6013.endtime.day.data,
			class6013.endtime.hour.data, class6013.endtime.min.data,
			class6013.endtime.sec.data);
	fprintf(stderr, "[6]%s-%d ", getenum(task_ti, class6013.delay.units),
			class6013.delay.interval);
	fprintf(stderr, "[7]%s  ", getenum(task_prio, class6013.runprio));
	fprintf(stderr, "[8]%s  [9]%d  [10]%d ",
			getenum(task_status, class6013.state), class6013.befscript,
			class6013.aftscript);

	fprintf(stderr, "\n");
}
void printMY_CSD(MY_CSD prtMyCSD) {
	fprintf(stderr, "\n printMY_CSD---------start-------\n");
	INT8U type, w = 0;
	type = prtMyCSD.type;
	if (type == 0) {
		fprintf(stderr, "OAD%04x-%02x%02x ", prtMyCSD.csd.oad.OI,
				prtMyCSD.csd.oad.attflg, prtMyCSD.csd.oad.attrindex);
	} else if (type == 1) {
		fprintf(stderr, "ROAD%04x-%02x%02x ", prtMyCSD.csd.road.oad.OI,
				prtMyCSD.csd.road.oad.attflg, prtMyCSD.csd.road.oad.attrindex);
		if (prtMyCSD.csd.road.num >= 16) {
			fprintf(stderr, "csd overvalue 16 error\n");
			return;
		}
//			fprintf(stderr,"csds.num=%d\n",class6015.csds.num);
		for (w = 0; w < prtMyCSD.csd.road.num; w++) {
			fprintf(stderr, "<..%d>%04x-%02x%02x ", w,
					prtMyCSD.csd.road.oads[w].OI,
					prtMyCSD.csd.road.oads[w].attflg,
					prtMyCSD.csd.road.oads[w].attrindex);
		}

	}
	fprintf(stderr, "\n printMY_CSD---------end-------\n");
}
void print6015(CLASS_6015 class6015) {
	INT8U i = 0;

	fprintf(stderr,
			"[1]方案编号 [2]存储深度 [3]采集类型 [4]采集内容 [5]OAD-ROAD [6]MS [7]存储时标\n");
	fprintf(stderr, "[6015]普通采集方案:[1]方案号: %d  \n", class6015.sernum);
	fprintf(stderr, "     [2]%d  [3]%s ", class6015.deepsize,
			getenum(coll_mode, class6015.cjtype));
	switch (class6015.cjtype) {
	case 0: // NULL
		fprintf(stderr, "[4]%02x ", class6015.data.data[0]);
		break;
	case 1:	//unsigned
		fprintf(stderr, "[4]%02x ", class6015.data.data[0]);
		break;
	case 2:	// NULL
		fprintf(stderr, "[4]%02x ", class6015.data.data[0]);
		break;
	case 3:	//TI
		fprintf(stderr, "[4]%s-%d ", getenum(task_ti, class6015.data.data[0]),
				((class6015.data.data[2] << 8) | class6015.data.data[1]));
		break;
	case 4:	//RetryMetering
		fprintf(stderr, "[4]%s-%d %d\n",
				getenum(task_ti, class6015.data.data[0]),
				((class6015.data.data[2] << 8) | class6015.data.data[1]),
				((class6015.data.data[4] << 8) | class6015.data.data[3]));
		break;
	}
	if (class6015.csds.num >= 10) {
		fprintf(stderr, "csd overvalue 10 error\n");
		return;
	}
	fprintf(stderr, "[5]");
	for (i = 0; i < class6015.csds.num; i++) {
		printMY_CSD(class6015.csds.csd[i]);
	}
	fprintf(stderr, "[6]%s ", getenum(ms_type, class6015.mst.mstype));
	fprintf(stderr, "[7]%s ", getenum(savetime_sel, class6015.savetimeflag));
	fprintf(stderr, "\n");

}
/*
 * 根据测量点串口参数是否改变
 * 返回值：<=0：串口打开失败
 * 		  >0： 串口打开句柄
 * */
INT32S open_com_para_chg(INT8U port, INT32U baud, INT32S oldcomfd) {
	INT32S newfd = 0;
	static INT8U lastport = 0;
	static INT32U lastbaud = 0;

	if ((oldcomfd>0)&&(lastbaud == baud) && (lastport == port))
	{
		return oldcomfd;
	}
	if (oldcomfd > 0) {
		CloseCom(oldcomfd);
		sleep(1);
	}

	if (port==1)
		port = 2;
	else if (port==2)
		port = 1;

	fprintf(stderr,"\n open_com_para_chg port = %d baud = %d newfd = %d",port,baud, newfd);

	newfd = OpenCom(port, baud, (unsigned char *) "even", 1, 8);

	lastport = port;
	lastbaud = baud;

	return newfd;
}
INT8S getComfdBy6001(INT8U baud,INT8U port)
{

	INT8S result = -1;
	INT32U baudrate = getMeterBaud(baud);
	fprintf(stderr,"\n\n baud = %d port = %d baudrate = %d comfd4851 = %d comfd4852 = %d\n\n",baud,port,baudrate,comfd4851,comfd4852);
	if (port == S4851)
	{
		comfd4851 = open_com_para_chg(S4851, baudrate, comfd4851);
		if (comfd4851 <= 0)
		{
			fprintf(stderr, "打开S4851串口失败\n");
			return result;
		}
	}
	else if (port == S4852)
	{
		comfd4852 = open_com_para_chg(S4852, baudrate, comfd4852);
		if (comfd4852 <= 0)
		{
			fprintf(stderr, "打开S4852串口失败\n");
			return result;
		}
	}
	else
	{
		return result;
	}
	return 1;
}
/*
 * 根据6013任务配置单元去文件里查找对应的6015普通采集方案
 * 输入 st6013
 * 输出 st6015
 */
INT8U use6013find6015(INT16U fanganID, CLASS_6015* st6015) {
	INT8U result = 0;
	OI_698 oi = 0x6015;
	if (readCoverClass(oi, fanganID, st6015, sizeof(CLASS_6015), coll_para_save)
			== 1) {
		//此函数不光是打印还要根据下发的档案初始化 冻结数据 07 标识
		print6015(*st6015);
	}
	return result;

}
void printbuff(const char* prefix, INT8U* buff, INT32U len, const char* format,
		const char* space, const char* surfix) {
	if (prefix != NULL )
		fprintf(stderr, "%s", prefix);
	if (buff != NULL && len > 0) {
		INT32U i = 0;
		for (i = 0; i < len; i++) {
			fprintf(stderr, (const char*) format, *(buff + i));
			if (space != NULL )
				fprintf(stderr, "%s", space);
		}
	}
	if (surfix != NULL )
		fprintf(stderr, "%s", surfix);
}
/*
 * 485口接收处理,判断完整帧
 * 输入参数：delayms，超时等待时间，单位：毫秒
 * 输出；*str：接收缓冲区
 * 返回：>0：完整报文；=0:接收长度为0；-1：乱码，无完整报文
 */
INT16S ReceDataFrom485(METER_PROTOCOL meterPro,INT8U port485, INT16U delayms, INT8U *str) {
	INT8U TmprevBuf[256];	//接收报文临时缓冲区
	INT8U prtstr[50];
	INT16U len_Total = 0, len, rec_step, rec_head, rec_tail, DataLen, i, j;
	INT32S fd = comfd4851;
	if(port485 == S4852)
	{
		fd = comfd4852;
	}
	if (fd <= 2)
		return -1;

	memset(TmprevBuf, 0, 256);
	rec_head = rec_tail = rec_step = DataLen = 0;
	//fprintf(stderr, "\n ReceDataFrom485 delayms=%d\n", delayms);
	usleep(delayms * 1000);

	for (j = 0; j < 15; j++) {
		usleep(20000);	//20ms
		len = read(fd, TmprevBuf, 256);

		if (len > 0) {
			len_Total += len;
			if (len_Total > 256) {
				fprintf(stderr, "len_Total=%d, xxxxxxxxxxx\n", len_Total);
				return -1;
			}
			for (i = 0; i < len; i++) {
				str[rec_head++] = TmprevBuf[i];
			}

			memset(prtstr, 0, sizeof(prtstr));
			sprintf((char *) prtstr, "485(%d)_R(%d):", 1, len);

			printbuff((char *) prtstr, TmprevBuf, len, "%02x", " ", "\n");
		}
		switch (rec_step) {
		case 0:
			if (rec_tail < rec_head) {
				for (i = rec_tail; i < rec_head; i++) {
					if (str[i] == 0x68) {	//ma:判断第一个字符是否为0x68
						rec_step = 1;
						rec_tail = i;
						break;
					} else
						rec_tail++;
				}
			}
			break;
		case 1:
			{
				if(meterPro == DLT_645_07)
				{
					if ((rec_head - rec_tail) >= 10)
					{
						if (str[rec_tail] == 0x68 && str[rec_tail + 7] == 0x68)
						{
							DataLen = str[rec_tail + 9];	//获取报文数据块长度
							rec_step = 2;
							break;
						} else
							rec_tail++;
					}
				}
				if(meterPro == DLT_698)
				{
					if ((rec_head - rec_tail) >= 3)
					{
							if (str[rec_tail] == 0x68)
							{
								DataLen = (str[rec_tail + 2]<<8);
								DataLen += str[rec_tail + 1];	//获取报文数据块长度
								rec_step = 2;
								break;
							} else
								rec_tail++;
					}
				}
			}


			break;
		case 2:
			{
				if(meterPro == DLT_645_07)
				{
					if ((rec_head - rec_tail) >= (DataLen + 2))
					{
						if (str[rec_tail + 9 + DataLen + 2] == 0x16) {
							DbPrt1(port485,"R:",(char *)str, rec_head, NULL);
							return rec_head;
						}
					}
				}
				if(meterPro == DLT_698)
				{
					//fprintf(stderr,"\n\n rec_head = %d rec_tail = %d DataLen = %d\n\n",rec_head,rec_tail,DataLen);
					//fprintf(stderr,"str[rec_tail + DataLen +1] = %02x",str[rec_tail + DataLen +1]);
					if ((rec_head - rec_tail) >= (DataLen + 1))
					{
							if (str[rec_tail + DataLen +1] == 0x16) {
								DbPrt1(port485,"R:",(char *)str, rec_head, NULL);
								return rec_head;
							}
					}
				}
			}
			break;
		default:
			break;
		}
	}
	if (len_Total > 0)
		return -1;
	else
		return 0;
}

/**
 * 485口发送
 */
void SendDataTo485(INT8U port485, INT8U *sendbuf, INT16U sendlen) {

	INT32S fd = comfd4851;
	if(port485 == S4852)
	{
		fd = comfd4852;
	}
	ssize_t slen;

	INT8U str[50];
	memset(str, 0, 50);
	sprintf((char *) str, "485(%d)_S(%d):", port485, sendlen);
	printbuff((char *) str, sendbuf, sendlen, "%02x", " ", "\n");

	fprintf(stderr,"\n port485 = %d fd = %d \n",port485,fd);
	slen = write(fd, sendbuf, sendlen);
	if (slen < 0)
		fprintf(stderr, "slen=%d,send err!\n", slen);
	DbPrt1(port485,"S:", (char *) sendbuf, sendlen, NULL);
}
//根据TSA从文件中找出6001
INT8U get6001ObjByTSA(TSA addr,CLASS_6001* targetMeter)
{
	INT8U ret = 0;

	int fileIndex = 0;
	int recordnum = 0;
	INT16U oi = 0x6000;
	recordnum = getFileRecordNum(oi);
	if (recordnum == -1) {
		fprintf(stderr, "未找到OI=%04x的相关信息配置内容！！！\n", 6000);
		return ret;
	} else if (recordnum == -2) {
		fprintf(stderr, "采集档案表不是整数，检查文件完整性！！！\n");
		return ret;
	}
	INT8U isMeterExist = 0;
	for(fileIndex = 0;fileIndex < recordnum;fileIndex++)
	{
		if(readParaClass(oi,targetMeter,fileIndex)==1)
		{
			if(targetMeter->sernum!=0 && targetMeter->sernum!=0xffff)
			{
				fprintf(stderr,"\n addr.addr = %02x%02x%02x%02x%02x%02x%02x%02x",
						addr.addr[0],addr.addr[1],addr.addr[2],addr.addr[3],addr.addr[4],addr.addr[5],addr.addr[6],addr.addr[7]);
				fprintf(stderr,"\ntargetMeter.addr = %02x%02x%02x%02x%02x%02x%02x%02x",
						targetMeter->basicinfo.addr.addr[0],targetMeter->basicinfo.addr.addr[1],targetMeter->basicinfo.addr.addr[2],
						targetMeter->basicinfo.addr.addr[3],targetMeter->basicinfo.addr.addr[4],targetMeter->basicinfo.addr.addr[5],
						targetMeter->basicinfo.addr.addr[6],targetMeter->basicinfo.addr.addr[7]);
				if(memcmp(addr.addr,targetMeter->basicinfo.addr.addr,addr.addr[0])==0)
				{
					isMeterExist = 1;
					ret = 1;
					break;
				}
			}
		}
	}

	return ret;
}

INT8U is485OAD(OAD portOAD,INT8U port485)
{
	port485 += 1;//浙江测试 485 1口下的是 F201_02_02 485 2口下的是 F201_02_03
	fprintf(stderr,"\n portOAD.OI = %04x portOAD.attflg = %d  portOAD.attrindex = %d port485 = %d \ n"
			,portOAD.OI,portOAD.attflg,portOAD.attrindex,port485);
	if ((portOAD.OI != 0xF201) || (portOAD.attflg != 0x02)
			|| (portOAD.attrindex != port485)) {
		return 0;
	}
	return 1;
}
INT8U checkEvent(CLASS_6001 meter,FORMAT07 resultData07,INT16U taskID)
{
	INT8U ret = 0;

	if(memcmp(flag07_0CF33,resultData07.DI,4)==0)
	{
		fprintf(stderr,"\n\n&&&&&&&&&&&checkEvent taskID = %d  mter.sernum = %d\n\n",taskID,meter.sernum);
		ret = Event_310B(meter.basicinfo.addr,taskID,resultData07.Data,resultData07.Length,JProgramInfo);

		ret = Event_310C(meter.basicinfo.addr,taskID,resultData07.Data,resultData07.Length,JProgramInfo,meter);

		ret = Event_310D(meter.basicinfo.addr,taskID,resultData07.Data,resultData07.Length,JProgramInfo,meter);

		ret = Event_310E(meter.basicinfo.addr,taskID,resultData07.Data,resultData07.Length,JProgramInfo);
	}
	if((memcmp(flag07_date,resultData07.DI,4)==0)||(memcmp(flag07_time,resultData07.DI,4)==0))
	{
		ret = Event_3105(meter.basicinfo.addr,taskID,resultData07.Data,resultData07.Length,JProgramInfo);
	}
	return ret;
}

//根据07 DI 返回数据类型dataType 数组大小size 信息
INT8U getASNInfo(FORMAT07* DI07,Base_DataType* dataType)
{
	fprintf(stderr, "\n getASNInfo DI07 = %02x%02x%02x%02x",DI07->DI[3],DI07->DI[2],DI07->DI[1],DI07->DI[0]);


	INT8U unitNum = 1;
	//电压
	if(memcmp(flag07_0CF25_1,DI07->DI,4) == 0)
	{
		*dataType = dtlongunsigned;
		unitNum = 3;
	}
	//A相电压
	if(memcmp(flag07_0CF25_1_A,DI07->DI,4) == 0)
	{
		*dataType = dtlongunsigned;
		unitNum = 1;
	}
	if(memcmp(flag07_0CF25_2,DI07->DI,4) == 0)
	{
		*dataType = dtdoublelong;
		unitNum = 3;
		INT8U f25_2_buff[12] = {0};
		memcpy(&f25_2_buff[1],&DI07->Data[0],3);
		memcpy(&f25_2_buff[5],&DI07->Data[3],3);
		memcpy(&f25_2_buff[9],&DI07->Data[6],3);
		memcpy(&DI07->Data[0],&f25_2_buff[0],12);
		DI07->Length += 3;
	}
	if(memcmp(flag07_0CF25_2_A,DI07->DI,4) == 0)
	{
		*dataType = dtdoublelong;
		unitNum = 1;
		INT8U f25_2_buff[4] = {0};
		memcpy(&f25_2_buff[1],&DI07->Data[0],3);
		memcpy(&DI07->Data[0],&f25_2_buff[0],4);
		DI07->Length += 1;
	}
	if((memcmp(flag07_0CF25_3,DI07->DI,4) == 0)||(memcmp(flag07_0CF25_4,DI07->DI,4) == 0))
	{
		*dataType = dtdoublelong;
		unitNum = 4;
		INT8U f25_3_buff[16] = {0};
		memcpy(&f25_3_buff[1],&DI07->Data[0],3);
		memcpy(&f25_3_buff[5],&DI07->Data[3],3);
		memcpy(&f25_3_buff[9],&DI07->Data[6],3);
		memcpy(&f25_3_buff[13],&DI07->Data[9],3);
		memcpy(&DI07->Data[0],&f25_3_buff[0],16);
		DI07->Length += 4;
	}

	//冻结时标数据07为5个字节 698为7个 需要特殊处理
	if(memcmp(freezeflag07_1,DI07->DI,4) == 0)
	{
		*dataType = dtdatetimes;
		DI07->Length += 2;
		fprintf(stderr,"\n 1-----getASNInfo dataType = %d",*dataType);
	}

	if((memcmp(flag07_0CF25_3,DI07->DI,4) == 0) || (memcmp(flag07_0CF25_4,DI07->DI,4) == 0))
	{
		*dataType = dtdoublelong;
		unitNum = 4;
	}

	if((memcmp(freezeflag07_2,DI07->DI,4) == 0)||(memcmp(freezeflag07_3,DI07->DI,4) == 0)
			||(memcmp(flag07_0CF33,DI07->DI,4) == 0)||(memcmp(flag07_0CF34,DI07->DI,4) == 0)
			||(memcmp(flag07_0CF177,DI07->DI,4) == 0))
	{
		*dataType = dtdoublelongunsigned;
		unitNum = 5;
		fprintf(stderr,"\n 2-----getASNInfo dataType = %d",*dataType);
	}

	return unitNum;
}

//把抄回来的07数据转换成698格式存储
INT16U data07Tobuff698(FORMAT07 Data07,INT8U* dataContent)
{
	INT16U index;

	INT16U len = 0;
	Base_DataType dataType = dtnull;
	INT8U unitSize = 0;
	INT8U unitNum = getASNInfo(&Data07,&dataType);
	INT16U dataLen07 = Data07.Length-4;
	#ifdef TESTDEF
		fprintf(stderr, "正常应答！  DI07 = %02x%02x%02x%02x datalen = %d data=",
				Data07.DI[3], Data07.DI[2], Data07.DI[1], Data07.DI[0],dataLen07);
		for(index = 0;index < dataLen07;index++)
		{
			fprintf(stderr," %02x",Data07.Data[index]);
		}
	#endif

	unitSize = dataLen07/unitNum;
	fprintf(stderr,"dataLen07 = %d unitNum = %d unitSize = %d",dataLen07,unitNum,unitSize);
	if((dataLen07%unitNum)!=0)
	{
		return len;
	}
	if(unitNum > 1)
	{
		dataContent[len++] = dtarray;
		dataContent[len++] = unitNum;
		fprintf(stderr,"\n 1---- dataContent[%d] = %d",len,unitNum);
	}

	for(index = 0;index < unitNum;index++)
	{
		INT16U dataIndex = unitSize*index;
		dataContent[len++] = dataType;
		fprintf(stderr,"\n 2---index = %d dataIndex = %d dataContent[%d] = %d",index,dataIndex,len,dataType);
		memcpy(&dataContent[len],&Data07.Data[dataIndex],unitSize);
		len += unitSize;
	}
#ifdef TESTDEF
	fprintf(stderr, "\n\n\n ###############data07Tobuff698[%d] = ",len);
	for(index = 0;index < len;index++)
	{
		fprintf(stderr," %02x",dataContent[index]);
	}
	fprintf(stderr, "###############\n\n\n");
#endif
	return len;
}
INT16S request698_07DataSingle(FORMAT07* format07, INT8U* SendBuf,INT16S SendLen,
		CLASS_6035* st6035,INT8U* dataContent,CLASS_6001 meter,INT8U por485)
{
	BOOLEAN nextFlag = 0;
	INT8S recsta = 0;
	INT16S RecvLen = 0;
	INT8U RecvBuff[256];
	INT16S buffLen = -1;
	memset(&RecvBuff[0], 0, 256);

	SendDataTo485(por485, SendBuf, SendLen);
	st6035->sendMsgNum++;
	RecvLen = ReceDataFrom485(DLT_645_07,por485, 500, RecvBuff);
	if (RecvLen > 0)
	{
		buffLen = 0;
		st6035->rcvMsgNum++;
		recsta = analyzeProtocol07(format07, RecvBuff, RecvLen, &nextFlag);
		if (recsta == 0)
		{
			//把07数据格式化放到dataContent
			buffLen = data07Tobuff698(*format07,dataContent);
			//检查是否是事件关联数据标识
			checkEvent(meter,*format07,st6035->taskID);
		} else
		{

			if (recsta == -1) {
				fprintf(stderr, "电表异常应答，无数据项  %02x%02x%02x%02x！！！\n",
						format07->DI[3], format07->DI[2], format07->DI[1],
						format07->DI[0]);
			} else if (recsta == -2) {
				fprintf(stderr, "电表异常应答，未知错误！ Err=%02x\n", format07->Err);
			} else if (recsta == -3) {
				fprintf(stderr, "其他功能！\n");
			} else if (recsta == -4) {
				fprintf(stderr, "校验错误！\n");
			}
		}
	}
	fprintf(stderr,"request698_07DataSingle buffLen = %d",buffLen);
	return buffLen;
}
INT16S request698_07Data(INT8U* DI07,INT8U* dataContent,CLASS_6001 meter,CLASS_6035* st6035,INT8U port485)
{
	fprintf(stderr, "\n\n -----------request698_07Data-----------\n\n");
	INT16S retLen = -1;
	INT16S SendLen = 0;
	INT8U SendBuff[256];
	INT8U subindex = 0;
	INT8U invalidDI[4] = { 0 };
	FORMAT07 Data07;
	memset(&SendBuff[0], 0, 256);
	memset(&Data07, 0, sizeof(FORMAT07));

	if (memcmp(invalidDI, DI07, 4) == 0)
	{
		fprintf(stderr,"\n 无效的数据标识");
		return retLen;
	}
	fprintf(stderr, "\n meterAddr len = %d addr = %02x%02x%02x%02x%02x%02x%02x%02x",
			meter.basicinfo.addr.addr[0], meter.basicinfo.addr.addr[1], meter.basicinfo.addr.addr[2],
			meter.basicinfo.addr.addr[3], meter.basicinfo.addr.addr[4], meter.basicinfo.addr.addr[5],
			meter.basicinfo.addr.addr[6],meter.basicinfo.addr.addr[7],meter.basicinfo.addr.addr[8]);
	fprintf(stderr, "\nDI = %02x%02x%02x%02x\n",DI07[0],DI07[1],DI07[2],DI07[3]);



	Data07.Ctrl = CTRL_Read_07;
	if(meter.basicinfo.addr.addr[1] > 5)
	{
		meter.basicinfo.addr.addr[1] = 5;
		fprintf(stderr,"request698_07Data 电表地址长度大于6");
	}

	INT8U startIndex = 5 - meter.basicinfo.addr.addr[1];
	memcpy(&Data07.Addr[startIndex], &meter.basicinfo.addr.addr[2], (meter.basicinfo.addr.addr[1]+1));
	memcpy(&Data07.DI, DI07, 4);

	DbgPrintToFile1(port485,"07测量点 = %02x%02x%02x%02x%02x%02x  DI = %02x%02x%02x%02x\n",
			Data07.Addr[0],Data07.Addr[1],Data07.Addr[2],Data07.Addr[3],Data07.Addr[4],Data07.Addr[5],
			DI07[0],DI07[1],DI07[2],DI07[3]);

	SendLen = composeProtocol07(&Data07, SendBuff);
	if (SendLen < 0)
	{
		fprintf(stderr, "request698_07DataList1");
		return retLen;
	}
	subindex = 0;
	while(subindex < 3)
	{
		retLen =
				request698_07DataSingle(&Data07,SendBuff,SendLen,st6035,dataContent,meter,port485);

		if(retLen >= 0)
		{
			return retLen;
		}
		subindex++;
	}
	fprintf(stderr,"\n request698_07Data retLen = %d",retLen);
	return retLen;
}
/*
 * 698 OAD 和 645 07规约 数据标识转换
 * dir:0-通过698OAD找64507DI 1-通过64507DI找698OAD
 * */
INT8S OADMap07DI(OI_698 roadOI,OAD sourceOAD, C601F_07Flag* obj601F_07Flag) {
	fprintf(stderr, "\n CSDMap07DI--------start--------\n");
	fprintf(stderr, "\n roadOI = %04x sourceOAD = %04x%02x%02x\n",roadOI,sourceOAD.OI,sourceOAD.attflg,sourceOAD.attrindex);


	INT8S result = 0;
	INT8U index;
	for (index = 0; index < map07DI_698OAD_NUM; index++)
	{
#if 0
		fprintf(stderr,"\n map07DI_698OAD[index].roadOI = %04x OAD = %04x%02x%02x",map07DI_698OAD[index].roadOI
				,map07DI_698OAD[index].flag698.OI,map07DI_698OAD[index].flag698.attflg,map07DI_698OAD[index].flag698.attrindex);
#endif
		if((memcmp(&roadOI,&map07DI_698OAD[index].roadOI,sizeof(OI_698))==0)
				&&(memcmp(&sourceOAD.OI,&map07DI_698OAD[index].flag698.OI,sizeof(OI_698))==0))
		{

			memcpy(obj601F_07Flag, &map07DI_698OAD[index].flag07, sizeof(C601F_07Flag));
			if(sourceOAD.attflg != 0x02)
			{
				obj601F_07Flag->DI_1[0][1] = sourceOAD.attrindex;
			}
			return 1;
		}
	}

	fprintf(stderr, "\n CSDMap07DI--------没有对应的07数据项--------\n");
	return result;
}

INT8U getSinglegOADDataUnit(INT8U* oadData)
{
	INT8U length = 0;
	INT8U dataUnitLen = 0;
	switch(oadData[length])
	{
		case dtarray:
		{
			length++;
			INT8U arrayNum = oadData[length++];
			fprintf(stderr,"\n arrayNum = %d\n",arrayNum);
			INT8U arrayNumIndex = 0;
			for(arrayNumIndex = 0;arrayNumIndex < arrayNum;arrayNumIndex++)
			{
				dataUnitLen = getBase_DataTypeLen(oadData[length++]);
				length += dataUnitLen;
			}
		}
		break;
		default:
		{
			dataUnitLen = getBase_DataTypeLen(oadData[length++]);
			length += dataUnitLen;
		}
	}
	return length;
}
/*
 * 解析单个OAD数据
 * */

INT16U parseSingleOADData(INT8U isProxyResponse,INT8U* oadData,INT8U* dataContent,INT8U* dataIndex)
{
	fprintf(stderr,"\n --------------------------parseSingleOADData-------------------------\n");
	INT8U dataLen = 0;
	INT16U length = 0;
	fprintf(stderr,"\n OAD %02x %02x %02x %02x\n",oadData[length],oadData[length+1],oadData[length+2],oadData[length+3]);
	if(isProxyResponse == 1)
	{
		memcpy(&dataContent[dataLen],&oadData[length],4);
		dataLen += 4;
	}
	OI_698 rcvOI = (oadData[length] << 8) + oadData[length+1];
	INT16U oiDataLen = CalcOIDataLen(rcvOI,oadData[length+3]);
	length += 4;
	fprintf(stderr,"\n rcvOI = %04x  len = %d\n",rcvOI,oiDataLen);
	if(oiDataLen <= 0)
	{
		fprintf(stderr,"\n 未在OI_TYPE.cfg找到对应OI");
	}
	if(oadData[length++] == 0)//0------ 没有数据  1--数据
	{
		fprintf(stderr,"\n 回复数据无效：DAR = %d\n",oadData[length+5]);
		if(isProxyResponse == 1)
		{
			dataContent[dataLen++] = 0;
		}
		else
		{
			memset(dataContent,0,oiDataLen);
			dataLen = oiDataLen;
		}
		length += 1; // 00 + DAR 两个字节

	}
	else
	{
		if(isProxyResponse == 1)
		{
			dataContent[dataLen++] = 0x01;
		}
		INT16U startIndex = 0;
		startIndex = length;
		INT8U singledataLen = getSinglegOADDataUnit(&oadData[length]);
		length += singledataLen;
		memcpy(&dataContent[dataLen],&oadData[startIndex],singledataLen);
		dataLen += singledataLen;
		//长度不够 CalcOIDataLen 后面补0
		if((singledataLen < oiDataLen)&&(isProxyResponse == 0))
		{
			memset(&dataContent[dataLen],0,oiDataLen-singledataLen);
			dataLen = dataLen + oiDataLen - singledataLen;
		}

		fprintf(stderr,"\n dataLen = %d\n",dataLen);
	}
	*dataIndex = dataLen;
#ifdef TESTDEF
	INT16U prtIndex =0;
	fprintf(stderr,"parseSingleOADData dataContent[%d] = ",dataLen);
	for(prtIndex = 0;prtIndex < dataLen;prtIndex++)
	{
		fprintf(stderr,"%02x ",dataContent[prtIndex]);
	}
#endif

	return length;
}
/*
 * 解析单个ROAD数据
 * 数据为空按road格式补0
 * */

INT16U parseSingleROADData(ROAD road,INT8U* oadData,INT8U* dataContent,INT8U* dataIndex)
{
	fprintf(stderr,"\n --------------------------parseSingle---ROAD----Data-------------------------\n");
	INT16U length = 0;
	INT16U dataLen = 0;
	INT16U startIndex = 0;
	INT8U prtIndex = 0;
	INT8U oadbuff[4];
	OAD_DATA oadListContent[ROAD_OADS_NUM];

	fprintf(stderr,"\n receive ROAD %02x %02x %02x %02x\n",oadData[length],oadData[length+1],oadData[length+2],oadData[length+3]);
	OADtoBuff(road.oad,oadbuff);
	if(memcmp(oadbuff,oadData,4)!=0)
	{
		fprintf(stderr,"\n request ROAD %02x %02x %02x %02x\n",oadbuff[length],oadbuff[length+1],oadbuff[length+2],oadbuff[length+3]);
		return length;
	}
	length += 4;
	INT8U rcvCSDnum = oadData[length++];//csd数量
	fprintf(stderr,"\n rcvCSDnum = %d",rcvCSDnum);
	if((rcvCSDnum > ROAD_OADS_NUM)||(rcvCSDnum == 0))
	{
		return length;
	}
	INT8U csdIndex;
	fprintf(stderr,"\n 收到回复的OAD ：");
	for(csdIndex = 0;csdIndex < rcvCSDnum;csdIndex++)
	{
		length ++;
		memcpy(oadListContent[csdIndex].oad,&oadData[length],4);
		fprintf(stderr," /%d-%02x%02x%02x%02x ",csdIndex,oadData[length],oadData[length+1],oadData[length+2],oadData[length+3]);
		length += 4;
	}
	if(oadData[length++] == 0)// 1-数据  0-错误
	{
		return length;
	}
	INT8U recordNum = oadData[length++];
	fprintf(stderr,"\nrecordNum = %d \n",recordNum);
	if(recordNum == 0)
	{
		return length;
	}
	//解析收到的ROAD数据
	INT8U recordIndex;
	for(recordIndex = 0;recordIndex<recordNum;recordIndex++)
	{
		for(csdIndex = 0;csdIndex < rcvCSDnum;csdIndex++)
		{
			startIndex = length;
			INT8U singledataLen = getSinglegOADDataUnit(&oadData[length]);
			length += singledataLen;
			oadListContent[csdIndex].datalen = singledataLen;
			memcpy(oadListContent[csdIndex].data,&oadData[startIndex],singledataLen);
			fprintf(stderr,"\n--------receive content -------\n");
			fprintf(stderr,"\nOAD = %02x%02x%02x%02x",oadListContent[csdIndex].oad[0],oadListContent[csdIndex].oad[1],oadListContent[csdIndex].oad[2],oadListContent[csdIndex].oad[3]);
			fprintf(stderr,"\ndatalen = %d",singledataLen);
			fprintf(stderr,"\ncontent = ");

			for(prtIndex = 0;prtIndex < singledataLen;prtIndex++)
			{
				fprintf(stderr,"%02x ",oadListContent[csdIndex].data[prtIndex]);
			}

		}
	}
	//按存储格式填充ROAD数据
	for(csdIndex = 0;csdIndex < road.num;csdIndex++)
	{
		INT16U oiDataLen = CalcOIDataLen(road.oads[csdIndex].OI,road.oads[csdIndex].attrindex);
		memset(&dataContent[dataLen],0,oiDataLen);
		memset(oadbuff,0,4);
		OADtoBuff(road.oads[csdIndex],oadbuff);
		INT8U subIndex=0;
		for(subIndex=0;subIndex<rcvCSDnum;subIndex++)
		{
			if(memcmp(oadbuff,oadListContent[subIndex].oad,4)==0)
			{
				memcpy(&dataContent[dataLen],oadListContent[subIndex].data,oadListContent[subIndex].datalen);
			}

		}
		dataLen += oiDataLen;
	}

	*dataIndex = dataLen;
	return dataLen;
}
/*
 *解析698抄表返回的报文
 *isProxyResponse = 1 说明是代理读取返回的  dataContent需要直接返回给主战
 *isProxyResponse = 0 正常抄表返回的 dataContent用来存储数据  二者格式不一样
 * */
INT16S deal698RequestResponse(INT8U isProxyResponse,INT8U getResponseType,INT16U apdudataLen,INT8U csdNum,INT8U* apdudata,INT8U* dataContent,CSD_ARRAYTYPE csds)
{
	INT16U apdudataIndex =0;
	INT16S dataContentIndex =0;
	INT8U dataContentLen =0;//按存储格式填充的数据长度--无效数据填0
	INT8U oaddataLen = 0;//报文中OAD+数据的长度
#ifdef TESTDEF
	fprintf(stderr,"deal698RequestResponse Buf[%d] = \n",apdudataLen);
	INT16U prtIndex =0;
	for(prtIndex = 0;prtIndex < apdudataLen;prtIndex++)
	{
		fprintf(stderr,"%02x ",apdudata[prtIndex]);
		if((prtIndex+1)%20 ==0)
		{
			fprintf(stderr,"\n");
		}
	}
#endif

	switch(getResponseType)
	{
	case GET_REQUEST_NORMAL:
		{
			oaddataLen = parseSingleOADData(isProxyResponse,&apdudata[apdudataIndex],dataContent,&dataContentLen);
			dataContentIndex = dataContentLen;
			fprintf(stderr,"\n dataContentIndex = %d dataContentLen = %d \n",dataContentIndex,dataContentLen);
		}
		break;

		case GET_REQUEST_NORMAL_LIST:
		{
			INT8U oadIndex;
			for(oadIndex=0;oadIndex < csdNum;oadIndex++)
			{
				fprintf(stderr,"\n apdudataIndex = %d dataContentIndex = %d dataContentLen = %d \n",apdudataIndex,dataContentIndex,dataContentLen);
				oaddataLen = parseSingleOADData(isProxyResponse,&apdudata[apdudataIndex],&dataContent[dataContentIndex],&dataContentLen);
				dataContentIndex += dataContentLen;
				apdudataIndex += oaddataLen;
				if(apdudataIndex > apdudataLen)
				{
					fprintf(stderr,"\n apdudataIndex > apdudataLen \n");
					break;
				}
			}
		}
		break;
		case GET_REQUEST_RECORD:
		{
			oaddataLen = parseSingleROADData(csds.csd[0].csd.road,&apdudata[apdudataIndex],&dataContent[dataContentIndex],&dataContentLen);
			dataContentIndex = dataContentLen;
			fprintf(stderr,"\n dataContentIndex = %d dataContentLen = %d \n",dataContentIndex,dataContentLen);
		}

		break;
	}

	return dataContentIndex;
}
INT16U dealProxy_698(CLASS_6001 obj6001,GETOBJS obj07,INT8U* dataContent,INT8U port485)
{
	DbgPrintToFile1(port485," 处理698测量点 代理消息 dealProxy_698 obj07.num = %d",obj07.num);
	fprintf(stderr," 处理698测量点 代理消息 dealProxy_698 obj07.num = %d",obj07.num);
	CLASS_6015 st6015;
	memset(&st6015,0,sizeof(CLASS_6015));
	st6015.cjtype = TYPE_NULL;
	st6015.csds.num = obj07.num;
	INT8U csdIndex;
	for(csdIndex = 0;csdIndex < st6015.csds.num;csdIndex++)
	{
		st6015.csds.csd[csdIndex].type = 0;
		memcpy(&st6015.csds.csd[csdIndex].csd.oad,&obj07.oads[csdIndex],sizeof(OAD));
		DbgPrintToFile1(port485," OAD[%d] = %04x%02x%02x",csdIndex,st6015.csds.csd[csdIndex].csd.oad.OI,
				st6015.csds.csd[csdIndex].csd.oad.attflg,st6015.csds.csd[csdIndex].csd.oad.attrindex);
	}

	INT16S sendLen = 0;
	INT16S recvLen = 0;
	INT16U retdataLen = 0;
	INT8U sendbuff[BUFFSIZE];
	INT8U recvbuff[BUFFSIZE];

	memset(sendbuff, 0, BUFFSIZE);

	sendLen = composeProtocol698_GetRequest(sendbuff, st6015, obj6001.basicinfo.addr);
	if(sendLen < 0)
	{
		fprintf(stderr,"deal6015_698  sendLen < 0");
		return retdataLen;
	}

	INT8U subindex = 0;
	while(subindex < 3)
	{
		memset(recvbuff, 0, BUFFSIZE);
		SendDataTo485(port485, sendbuff, sendLen);

		recvLen = ReceDataFrom485(DLT_698,port485, 500, recvbuff);
		//fprintf(stderr,"\n\n recvLen = %d \n",recvLen);
		if(recvLen > 0)
		{
			INT8U csdNum = 0;
			INT16S dataLen = recvLen;
			INT8U apduDataStartIndex = 0;
			INT8U getResponseType = analyzeProtocol698(recvbuff,&csdNum,recvLen,&apduDataStartIndex,&dataLen);
			fprintf(stderr,"\n dealProxy_698 getResponseType = %d  csdNum = %d dataLen = %d \n",getResponseType,csdNum,dataLen);
			if((getResponseType == GET_REQUEST_NORMAL_LIST)||(getResponseType == GET_REQUEST_NORMAL))
			{
				retdataLen = deal698RequestResponse(1,getResponseType,dataLen,csdNum,&recvbuff[apduDataStartIndex],dataContent,st6015.csds);
				break;
			}
			else
			{
				retdataLen = 0;
			}
		}
		subindex++;
	}

	return retdataLen;
}
INT16U dealProxy_645_07(GETOBJS obj07,INT8U* dataContent,INT8U port485)
{
	INT16U singledataLen = -1;
	INT16U dataLen = 0;
	INT16U dataFlagPos = 0;
	CLASS_6001 meter = {};
	CLASS_6035 inValid6035 = {};

	INT8U oadIndex;
	INT8U diIndex;
	for(oadIndex = 0;oadIndex < obj07.num;oadIndex++)
	{
		DbgPrintToFile1(port485," OAD[%d] = %04x%02x%02x",oadIndex,obj07.oads[oadIndex].OI,obj07.oads[oadIndex].attflg,obj07.oads[oadIndex].attrindex);
		OADtoBuff(obj07.oads[oadIndex],&dataContent[dataLen]);
		dataLen += sizeof(OAD);

		C601F_07Flag obj601F_07Flag;
		memset(&obj601F_07Flag,0,sizeof(C601F_07Flag));

		if(OADMap07DI(0x0000,obj07.oads[oadIndex],&obj601F_07Flag)!=1)
		{
			DbgPrintToFile1(1,"找不到%04x%02x%02x对应07数据项",obj07.oads[oadIndex].OI,obj07.oads[oadIndex].attflg,obj07.oads[oadIndex].attrindex);
			fprintf(stderr,"\n 找不到%04x%02x%02x对应07数据项",obj07.oads[oadIndex].OI,obj07.oads[oadIndex].attflg,obj07.oads[oadIndex].attrindex);
			continue;
		}
		memcpy(meter.basicinfo.addr.addr,obj07.tsa.addr,sizeof(TSA));
		dataFlagPos = dataLen;
		dataContent[dataLen++] = 0x01;//默认有数据

		for(diIndex=0;diIndex<obj601F_07Flag.dinum;diIndex++)
		{
			singledataLen = request698_07Data(obj601F_07Flag.DI_1[diIndex],&dataContent[dataLen],meter,&inValid6035,port485);
			if(singledataLen >= 0)
			{
				dataLen += singledataLen;
			}
		}
		if(dataLen == (dataFlagPos+1))
		{
			dataContent[dataFlagPos] = 0x00;//没有数据
		}
	}
	fprintf(stderr,"\n 处理07测量点代理返回 dealProxy_645_07 dataLen = %d",dataLen);
	return dataLen;
}
INT8S dealProxy(PROXY_GETLIST* getlist,INT8U port485)
{
#ifdef TESTDEF1
	getlist->num = 1;
	getlist->objs[0].num = 2;
	getlist->objs[0].oads[0].OI = 0x0010;
	getlist->objs[0].oads[0].attflg = 0x20;
	getlist->objs[0].oads[0].attrindex = 0x00;
	getlist->objs[0].oads[1].OI = 0x0020;
	getlist->objs[0].oads[1].attflg = 0x20;
	getlist->objs[0].oads[1].attrindex = 0x00;
	getlist->objs[0].tsa.addr[0] = 0x07;
	getlist->objs[0].tsa.addr[1] = 0x05;
	getlist->objs[0].tsa.addr[2] = 0x11;
	getlist->objs[0].tsa.addr[3] = 0x11;
	getlist->objs[0].tsa.addr[4] = 0x11;
	getlist->objs[0].tsa.addr[5] = 0x11;
	getlist->objs[0].tsa.addr[6] = 0x11;
	getlist->objs[0].tsa.addr[7] = 0x11;
#endif
	INT8S result = -1;
#ifdef TESTDEF
	//判断代理是否已经超时
	time_t nowtime = time(NULL);
	fprintf(stderr,"\n\n getlist->timeout = %d",getlist->timeout);
	if(nowtime > (getlist->timeout + getlist->timeold))
	{
		fprintf(stderr,"\n 代理请求超时");
		getlist->status = 3;
		return result;
	}
#endif
	fprintf(stderr,"\n dealProxy--------1 objs num = %d :",getlist->num);
	DbgPrintToFile1(port485,"dealProxy--------1 objs num = %d :", getlist->num);

	INT8U index;
	INT16U totalLen = 0;
	INT16U singleLen = 0;
	getlist->data[totalLen++] = getlist->num;
	for(index = 0;index < getlist->num;index++)
	{
		CLASS_6001 obj6001 = {};
		INT8U addlen = getlist->objs[index].tsa.addr[0]+1;
		memcpy(&getlist->data[totalLen],&getlist->objs[index].tsa.addr[0],addlen);
		totalLen += addlen;
		DbgPrintToFile1(port485,"dealProxy--------1 addr:%02x%02x%02x%02x%02x%02x%02x%02x",
				getlist->objs[index].tsa.addr[0],getlist->objs[index].tsa.addr[1],getlist->objs[index].tsa.addr[2]
				,getlist->objs[index].tsa.addr[3],getlist->objs[index].tsa.addr[4],getlist->objs[index].tsa.addr[5]
				,getlist->objs[index].tsa.addr[6],getlist->objs[index].tsa.addr[7]);
		//通过表地址找 6001
		if(get6001ObjByTSA(getlist->objs[index].tsa,&obj6001) != 1 )
		{
			fprintf(stderr," dealProxy--------2 未找到相应6001");
			DbgPrintToFile1(port485,"dealProxy--------2 未找到相应6001");
			getlist->data[totalLen++] = 0;//没有数据
			continue;
		}
		INT8U portUse = obj6001.basicinfo.port.attrindex -1;
		DbgPrintToFile1(portUse," portUse = %d",portUse);
		if(getComfdBy6001(obj6001.basicinfo.baud,portUse) != 1)
		{
			fprintf(stderr," dealProxy--------4");
			getlist->data[totalLen++] = 0;//没有数据
			continue;
		}
		getlist->data[totalLen++] = getlist->objs[index].num;
		fprintf(stderr,"\n OAD num = %d",getlist->objs[index].num);
		DbgPrintToFile1(portUse," OAD num = %d",getlist->objs[index].num);

		switch(obj6001.basicinfo.protocol)
		{
		case DLT_645_07:
			singleLen = dealProxy_645_07(getlist->objs[index],&getlist->data[totalLen],portUse);
			break;
		default:
			singleLen = dealProxy_698(obj6001,getlist->objs[index],&getlist->data[totalLen],portUse);
		}
		DbgPrintToFile1(portUse," singleLen = %d",singleLen);
		if(singleLen > 0)
		{
			totalLen += singleLen;
		}

		getlist->datalen = totalLen;

		DbPrt1(portUse,"发送代理消息",(char *)getlist->data, totalLen, NULL);
#ifdef TESTDEF
		fprintf(stderr,"\n\ndealProxy 代理返回报文 长度：%d :",totalLen);
		INT16U tIndex;
		for(tIndex = 0;tIndex < totalLen;tIndex++)
		{
			fprintf(stderr,"%02x ",getlist->data[tIndex]);
			if((tIndex+1)%20 ==0)
			{
				fprintf(stderr,"\n");
			}
		}
#endif
		mqs_send((INT8S *)PROXY_NET_MQ_NAME,1,ProxySetResponseList,(INT8U *)getlist,sizeof(PROXY_GETLIST));
		fprintf(stderr,"\n代理消息已经发出\n\n");

	}


	return result;
}
INT8S DealRecvMsg(mmq_head mq_h, INT8U *rev_485_buf,INT8U port485)
{
	INT8S result = -1;
	switch(mq_h.cmd)
	{
		case 1://代理
		{
			PROXY_GETLIST * getlist;
			getlist = (PROXY_GETLIST*)rev_485_buf;
			//fprintf(stderr,"\n---- PROXY_GETLIST size = %d",sizeof(PROXY_GETLIST));
			dealProxy(getlist,port485);
		}
		break;
		default:
		{
			DbgPrintToFile1(port485,"485收到未知消息  cmd=%d!!!---------------", mq_h.cmd);
		}
			return result;
	}
	return result;
}
INT8S readMeterPowerInfo()
{
	fprintf(stderr,"\n\n上电抄读停上电事件 readMeterPowerInfo");
	INT8S result = -1;

	INT8U meterIndex;

	for(meterIndex = 0; meterIndex < POWEROFFON_NUM; meterIndex++)
	{
		fprintf(stderr,"\nMeterPowerInfo[%d] ARRD = %02x%02x%02x%02x%02x%02x%02x%02x  ERC3106State = %d \n",meterIndex,
				MeterPowerInfo[meterIndex].tsa.addr[0],MeterPowerInfo[meterIndex].tsa.addr[1],MeterPowerInfo[meterIndex].tsa.addr[2],
				MeterPowerInfo[meterIndex].tsa.addr[3],MeterPowerInfo[meterIndex].tsa.addr[4],MeterPowerInfo[meterIndex].tsa.addr[5],
				MeterPowerInfo[meterIndex].tsa.addr[6],MeterPowerInfo[meterIndex].tsa.addr[7],
				MeterPowerInfo[meterIndex].ERC3106State);
		if(MeterPowerInfo[meterIndex].ERC3106State==1)
		{
			CLASS_6001 obj6001;
			//通过表地址找 6001
			if(get6001ObjByTSA(MeterPowerInfo[meterIndex].tsa,&obj6001) != 1 )
			{
				fprintf(stderr," readMeterPowerInfo-------1 未找到相应6001");
				continue;
			}


			if(getComfdBy6001(obj6001.basicinfo.baud,obj6001.basicinfo.port.attrindex-1) != 1)
			{
				fprintf(stderr," readMeterPowerInfo--------2");
				continue;
			}
			INT8U port485 = obj6001.basicinfo.port.attrindex-1;

			CLASS_6035 invalidst6035;
			INT8U dataContent[100];
			memset(dataContent,0,100);

			switch(obj6001.basicinfo.protocol)
			{
			case DLT_645_07:
				{
					 request698_07Data(flag07_tingshangdian,dataContent,obj6001,&invalidst6035,port485);
				}
				break;
			default:
				{
					CLASS_6015 st6015;
					memset(&st6015,0,sizeof(CLASS_6015));
					st6015.cjtype = TYPE_NULL;
					st6015.csds.num = 1;
					st6015.csds.csd[0].type = 0;
					st6015.csds.csd[0].csd.oad.OI = 0x3106;
					st6015.csds.csd[0].csd.oad.attflg = 02;
					st6015.csds.csd[0].csd.oad.attrindex = 01;
					INT16S sendLen = 0;
					INT16S recvLen = 0;

					INT8U sendbuff[BUFFSIZE];
					INT8U recvbuff[BUFFSIZE];

					memset(sendbuff, 0, BUFFSIZE);

					sendLen = composeProtocol698_GetRequest(sendbuff, st6015, obj6001.basicinfo.addr);
					if(sendLen < 0)
					{
						fprintf(stderr,"deal6015_698  sendLen < 0");
						continue;
					}

					INT8U subindex = 0;
					while(subindex < 3)
					{
						memset(recvbuff, 0, BUFFSIZE);
						SendDataTo485(port485, sendbuff, sendLen);
						recvLen = ReceDataFrom485(DLT_698,port485, 500, recvbuff);
						fprintf(stderr,"\n\n recvLen = %d \n",recvLen);
						if(recvLen > 0)
						{

							INT8U csdNum = 0;
							INT16S dataLen = recvLen;
							INT8U apduDataStartIndex = 0;
							INT8U getResponseType = analyzeProtocol698(recvbuff,&csdNum,recvLen,&apduDataStartIndex,&dataLen);
							fprintf(stderr,"\n getResponseType = %d  csdNum = %d dataLen = %d \n",getResponseType,csdNum,dataLen);
							if(getResponseType > 0)
							{
								deal698RequestResponse(0,getResponseType,dataLen,csdNum,&recvbuff[apduDataStartIndex],dataContent,st6015.csds);
								break;
							}

						}
						subindex++;
					}
				}

			}
		}
	}
	return result;
}
//处理代理等实时请求
INT8S dealRealTimeRequst(INT8U port485)
{
	while(readState)
	{
		DbgPrintToFile1(port485,"\n 另一个线程正在处理消息 dealRealTimeRequst \n");
		sleep(1);
	}
	fprintf(stderr,"\n poweroffon_state = %d",poweroffon_state);
	//先抄读停上电事件
	if(poweroffon_state == 1)
	{
		readState = 1;
		DbgPrintToFile1(port485,"poweroffon_state=%d!!!---------------",poweroffon_state);
		readMeterPowerInfo();
		poweroffon_state =2;
		readState = 0;
	}

	INT8S result = -1;
	if(mqd_485_main<0)
	{
		fprintf(stderr,"S485_1_REV_MAIN_MQ:mq_open_ret=%d\n",mqd_485_main);
		return result;
	}
	INT8U  rev_485_buf[2048];
	INT32S ret;

	while(1)
	{
		mmq_head mq_h;
		ret = mmq_get(mqd_485_main, 1, &mq_h, rev_485_buf);
#ifdef TESTDEF1
		ret = 1;
		mq_h.cmd = 1;
#endif
		if (ret>0)
		{
			readState = 1;
			fprintf(stderr, "\n\n-----------------vs485_main recvMsg!!!    cmd=%d!!!---------------\n", mq_h.cmd);
			DbgPrintToFile1(port485,"485收到消息    cmd=%d!!!---------------", mq_h.cmd);
			DealRecvMsg(mq_h, rev_485_buf,port485);
			DbgPrintToFile1(port485,"485处理消息结束   cmd=%d!!!---------------", mq_h.cmd);
			readState = 0;
			continue;
		}
		else
		{
			break;
		}
		usleep(1000*1000);
	}

	return result;
}

INT16U deal6015_698(CLASS_6015 st6015, CLASS_6001 to6001,CLASS_6035* st6035,INT8U* dataContent,INT8U port485)
{
	dealRealTimeRequst(port485);
	fprintf(stderr, "\n deal6015_698-------------------  meter = %d\n", to6001.sernum);
	DbgPrintToFile1(port485," deal6015_698-------------------  meter = %d", to6001.sernum);
	INT8U getResponseType = 0;
	INT16S retLen = 0;
	INT16S sendLen = 0;
	INT16S recvLen = 0;
	INT8U subindex = 0;
	INT8U sendbuff[BUFFSIZE];
	INT8U recvbuff[BUFFSIZE];

	memset(sendbuff, 0, BUFFSIZE);

	sendLen = composeProtocol698_GetRequest(sendbuff, st6015, to6001.basicinfo.addr);
	if(sendLen < 0)
	{
		fprintf(stderr,"deal6015_698  sendLen < 0");
		return retLen;
	}

	subindex = 0;
	while(subindex < 3)
	{
		memset(recvbuff, 0, BUFFSIZE);
		SendDataTo485(port485, sendbuff, sendLen);
		st6035->sendMsgNum++;
		recvLen = ReceDataFrom485(DLT_698,port485, 500, recvbuff);
		fprintf(stderr,"\n\n recvLen = %d \n",recvLen);
		if(recvLen > 0)
		{
			st6035->rcvMsgNum++;
			INT8U csdNum = 0;
			INT16S dataLen = recvLen;
			INT8U apduDataStartIndex = 0;
			getResponseType = analyzeProtocol698(recvbuff,&csdNum,recvLen,&apduDataStartIndex,&dataLen);
			fprintf(stderr,"\n getResponseType = %d  csdNum = %d dataLen = %d \n",getResponseType,csdNum,dataLen);
			if(getResponseType > 0)
			{
				retLen = deal698RequestResponse(0,getResponseType,dataLen,csdNum,&recvbuff[apduDataStartIndex],dataContent,st6015.csds);
				break;
			}

		}
		subindex++;
	}

	fprintf(stderr, "\n deal6015_698-------------------  retLen = %d\n", retLen);
	return retLen;
}
/*
 * DI07List[10][4]是一个CSD对应的07数据标识列表
 * dataContent里保存一个任务抄上来的所有数据，不带数据标识
 * */
INT16U request698_07DataList(C601F_07Flag obj601F_07Flag, CLASS_6001 meter,INT8U* dataContent,CLASS_6035* st6035,INT8U port485)
{
	INT16U DataLen = 0;	//暂存正常抄读的数据长度
	INT8U index;
	INT16S singleBuffLen = 0;
	INT8U isSuccess = 1;
	fprintf(stderr,"\n\n-------request698_07DataList obj601F_07Flag.dinum = %d",obj601F_07Flag.dinum);
	for (index = 0; index < obj601F_07Flag.dinum; index++)
	{
		dealRealTimeRequst(port485);
		singleBuffLen = request698_07Data(obj601F_07Flag.DI_1[index],&dataContent[DataLen],meter,st6035,port485);

		if(singleBuffLen > 0)
		{
			DataLen += singleBuffLen;
		}
		else
		{
			isSuccess = 0;
		}

	}
	if(isSuccess ==1)
	{
		st6035->successMSNum++;
	}
	return DataLen;
}
INT16U request07_singleOAD(OI_698 roadOI,OAD soureOAD,CLASS_6001 to6001,CLASS_6035* st6035,INT8U* dataContent,INT8U port485)
{
	INT16U datalen = 0;
	C601F_07Flag obj601F_07Flag;
	memset(&obj601F_07Flag,0,sizeof(C601F_07Flag));
	//存储要求的固定长度 长度不够后面补0
	INT16U formatLen = CalcOIDataLen(soureOAD.OI,soureOAD.attrindex);
	memset(dataContent,0,formatLen);
	if (OADMap07DI(roadOI,soureOAD, &obj601F_07Flag) == 1)
	{
		memset(dataContent,0,formatLen);

		datalen = request698_07DataList(obj601F_07Flag, to6001,dataContent,st6035,port485);
		fprintf(stderr,"\n deal6015_07 datalen=%d",datalen);

	}
	else
	{
		fprintf(stderr, "request698_07Data:1");
		DbgPrintToFile1(port485,"roadOI = %04x soureOAD=%04x%02x%02x 没有对应的07数据项",roadOI,soureOAD.OI,soureOAD.attflg,soureOAD.attrindex);
	}
	return formatLen;

}
INT16U deal6015_07(CLASS_6015 st6015, CLASS_6001 to6001,CLASS_6035* st6035,INT8U* dataContent,INT8U port485) {
	INT16U totaldataLen =0;
	INT16U datalen = 0;
	fprintf(stderr,
			"\n\n-------start------------ deal6015_07  meter = %d st6015.sernum = %d st6015.csds.num = %d---------",
			to6001.sernum, st6015.sernum, st6015.csds.num);
	DbgPrintToFile1(port485,"-------start------------ deal6015_07  meter = %d st6015.sernum = %d st6015.csds.num = %d---------",
			to6001.sernum, st6015.sernum, st6015.csds.num);
	switch (st6015.cjtype) {
	case TYPE_NULL:/*采集当前数据--实时*/
	{
		DbgPrintToFile1(port485, " deal6015_07 采集当前数据--实时");
		fprintf(stderr, "\n deal6015_07 采集当前数据--实时");
	}
		break;
	case TYPE_LAST:/*采集上N次*/
	{
		DbgPrintToFile1(port485, " deal6015_07 采集上N次数据--冻结");
		fprintf(stderr, "\n deal6015_07 采集上N次数据--冻结");
	}
		break;
	case TYPE_FREEZE:/*按冻结时标*/
	{
		DbgPrintToFile1(port485, " deal6015_07 按冻结时标采集");
	}
		break;
	case TYPE_INTERVAL:/*按时标间隔---曲线*/
		break;
	}

	INT8U dataIndex = 0;

	for (dataIndex = 0; dataIndex < st6015.csds.num; dataIndex++)
	{
		//ROAD
		if(st6015.csds.csd[dataIndex].type == 1)
		{
			INT8U csdIndex;
			for(csdIndex=0;csdIndex<st6015.csds.csd[dataIndex].csd.road.num;csdIndex++)
			{
				datalen = request07_singleOAD(st6015.csds.csd[dataIndex].csd.road.oad.OI,
						st6015.csds.csd[dataIndex].csd.road.oads[csdIndex],to6001,st6035,&dataContent[totaldataLen],port485);
				totaldataLen += datalen;
			}
		}
		else
		{
			datalen = request07_singleOAD(0x0000,st6015.csds.csd[dataIndex].csd.oad,to6001,st6035,&dataContent[totaldataLen],port485);
			totaldataLen += datalen;
		}
	}
	if(totaldataLen >= DATA_CONTENT_LEN)
	{
		fprintf(stderr,"dataContent 长度不够");
		fprintf(stderr,"deal6015_07 datalen = %d totaldataLen = %d",datalen,totaldataLen);
		return totaldataLen;
	}
	fprintf(stderr,
			"\n\n**********end************ deal6015_07  meter = %d st6015.sernum = %d st6015.csds.num = %d---------",
			to6001.sernum, st6015.sernum, st6015.csds.num);
	DbgPrintToFile1(port485,"st6015.csds.csd[%d]",
			to6001.sernum, st6015.sernum, st6015.csds.num);
	return totaldataLen;
}
/*
 * 抄读1个测量点
 */
INT16U deal6015_singlemeter(CLASS_6015 st6015, CLASS_6001 obj6001,CLASS_6035* st6035,INT8U* dataContent,INT8U port485)
{
	INT16U ret = 0;
	if(getComfdBy6001(obj6001.basicinfo.baud,port485)!=1)
	{
		return ret;
	}

	switch (obj6001.basicinfo.protocol) {
	case DLT_645_07:
		ret = deal6015_07(st6015, obj6001,st6035,dataContent,port485);
	break;
	default:
		ret = deal6015_698(st6015,obj6001,st6035,dataContent,port485);
	}

	fprintf(stderr, "\ndeal6015_singlemeter ret = %d\n",ret);
	return ret;
}

/*
 *根据6015中的MS 和电表地址 和端口好判断此电表是否需要抄读
 *0-不抄 1-抄
 */
INT8U checkMeterType(MY_MS mst, INT8U port485, TSA meterAddr, OAD portOAD) {

	if(is485OAD(portOAD,port485)==0)
	{
		fprintf(stderr,"\n checkMeterType 非485 %d 测量点",port485);
		return 0;
	}
	if (mst.mstype == 1) {
		return 1;
	}
	return 1;
}
/*
 * 从文件里读取LIST6001SIZE个测量点
 * */
INT8U readList6001FromFile(CLASS_6001* list6001, INT16U groupIndex,
		int recordnum, MY_MS mst, INT8U port485) {
	INT16U oi = 0x6000;
	INT8U result = 0;
	INT8U mIndex = 0;
	int endIndex;
	CLASS_6001 meter = { };
	if (((groupIndex + 1) * LIST6001SIZE) > recordnum) {
		endIndex = recordnum;
	} else {
		endIndex = (groupIndex + 1) * LIST6001SIZE;
	}
	INT8U dealIndex = 0;
	for (mIndex = groupIndex * LIST6001SIZE; mIndex < endIndex; mIndex++) {

		if (readParaClass(oi, &meter, mIndex) == 1) {

			if (meter.sernum != 0 && meter.sernum != 0xffff) {
				if (checkMeterType(mst, port485, meter.basicinfo.addr,
						meter.basicinfo.port))
				{
						memcpy(&list6001[dealIndex],&meter,sizeof(CLASS_6001));
						INT16U mpNO = list6001[dealIndex].sernum;
						DbgPrintToFile1(port485,"readList6001FromFile list6001[%d] 测量点 = %d-----",dealIndex,mpNO);
						dealIndex++;
				}
				else
				{
				}
			}
		}
	}

	return result;
}
INT16U compose6012Buff(DateTimeBCD startTime,TSA meterAddr,INT16U dataLen,INT8U* dataContent, INT8U port485)
{
	fprintf(stderr,"\n 存储数据  compose6012Buff--------------");
	INT16U index;
	INT16U bufflen = 0;
	DateTimeBCD endTime;
	DataTimeGet(&endTime);
	INT8U buff6012[DATA_CONTENT_LEN];
	memset(buff6012,0,DATA_CONTENT_LEN);

	index = bufflen;
	buff6012[bufflen++] = 0x55;
	memcpy(&buff6012[bufflen],meterAddr.addr,sizeof(TSA));//采集通信地址
	bufflen += sizeof(TSA);

	fprintf(stderr,"\n 采集通信地址：");
	for(;index < bufflen;index++)
	{
		fprintf(stderr," %02x",buff6012[index]);
	}

	INT8U DateTimeBCDLen = sizeof(DateTimeBCD)-1;

	index = bufflen;
	buff6012[bufflen++] = dtdatetimes;
	memcpy(&buff6012[bufflen],&startTime,DateTimeBCDLen);//采集启动时标
	bufflen += DateTimeBCDLen;

	fprintf(stderr,"\n 采集启动时标：");
	fprintf(stderr,"index = %d bufflen = %d",index,bufflen);
	for(;index < bufflen;index++)
	{
		fprintf(stderr," %02x",buff6012[index]);
	}

	index = bufflen;
	buff6012[bufflen++] = dtdatetimes;
	memcpy(&buff6012[bufflen],&endTime,DateTimeBCDLen);//采集成功时标
	bufflen += DateTimeBCDLen;

	fprintf(stderr,"\n 采集成功时标：");
	fprintf(stderr,"index = %d bufflen = %d",index,bufflen);
	for(;index < bufflen;index++)
	{
		fprintf(stderr," %02x",buff6012[index]);
	}

	index = bufflen;
	buff6012[bufflen++] = dtdatetimes;
	memcpy(&buff6012[bufflen],&endTime,DateTimeBCDLen);//采集存储时标
	bufflen += DateTimeBCDLen;

	fprintf(stderr,"\n 采集存储时标：");
	fprintf(stderr,"index = %d bufflen = %d",index,bufflen);
	for(;index < bufflen;index++)
	{
		fprintf(stderr," %02x",buff6012[index]);
	}

	index = bufflen;
	memcpy(&buff6012[bufflen],dataContent,dataLen);
	bufflen += dataLen;

	fprintf(stderr,"\n 采集数据内容：\n");
	for(;index < bufflen;index++)
	{
		fprintf(stderr," %02x",buff6012[index]);
		if((index+1)%20==0)
		{
			fprintf(stderr,"\n");
		}
	}
	memset(dataContent,0,DATA_CONTENT_LEN);
	memcpy(dataContent,buff6012,bufflen);
	fprintf(stderr,"\n\n buff6012[%d]:",bufflen);
	DbPrt1(port485,"存储数据  compose6012Buff:", (char *) dataContent, bufflen, NULL);
	return bufflen;
}
/*
 * 处理一个普通采集方案
 * */
INT8U deal6015(CLASS_6015 st6015, INT8U port485,CLASS_6035* st6035) {
	INT8U result = 0;
	CLASS_6001 list6001[LIST6001SIZE];

	int recordnum = 0;
	//无电能表
	if (st6015.mst.mstype == 0) {
		return 0;
	}

	INT16U oi = 0x6000;
	recordnum = getFileRecordNum(oi);
	if (recordnum == -1) {
		fprintf(stderr, "未找到OI=%04x的相关信息配置内容！！！\n", 6000);
		return result;
	} else if (recordnum == -2) {
		fprintf(stderr, "采集档案表不是整数，检查文件完整性！！！\n");
		return result;
	}
	fprintf(stderr, "\n deal6015 recordnum = %d ", recordnum);
	/*
	 * 根据st6015.csd 和 list6001抄表
	 * */

	INT16U groupNum = (recordnum / LIST6001SIZE) + 1;
	INT16U groupindex;
	INT8U mpIndex;
	for (groupindex = 0; groupindex < groupNum; groupindex++) {
		memset(list6001, 0, LIST6001SIZE * sizeof(CLASS_6001));
		result = readList6001FromFile(list6001, groupindex, recordnum,
				st6015.mst, port485);
		for (mpIndex = 0; mpIndex < LIST6001SIZE; mpIndex++)
		{
			if (list6001[mpIndex].sernum > 0)
			{
				st6035->totalMSNum++;
				fprintf(stderr,"\n\n 任务号:%d  方案号:%d deal6015 测量点 = %d-----",st6035->taskID,st6015.sernum,list6001[mpIndex].sernum);
				DbgPrintToFile1(port485,"任务号:%d  方案号:%d deal6015 测量点 = %d-----",st6035->taskID,st6015.sernum,list6001[mpIndex].sernum);
				INT8U dataContent[DATA_CONTENT_LEN];
				memset(dataContent,0,DATA_CONTENT_LEN);
				INT16U dataLen = 0;
				DateTimeBCD startTime;

				DataTimeGet(&startTime);
				dataLen = deal6015_singlemeter(st6015, list6001[mpIndex],st6035,dataContent,port485);

				if(dataLen > 0)
				{
					int bufflen = compose6012Buff(startTime,list6001[mpIndex].basicinfo.addr,dataLen,dataContent,port485);
					SaveNorData(st6035->taskID,dataContent,bufflen);
				}
				else
				{
					fprintf(stderr,"\n deal6015:失败");
				}
			}
		}
	}

	return result;
}
INT16S getTaskIndex(INT8U port)
{
	INT16S taskIndex = -1;

	INT8U  rev_485_buf[256];
	INT32S ret;
	mmq_head mq_h;


	switch(port)
	{
		case 2:
		{
			if(mqd_485_2_task < 0)
			{
				fprintf(stderr,"mqd_485_2_task:mq_open_ret=%d\n",mqd_485_2_task);
				return taskIndex;
			}
			ret = mmq_get(mqd_485_2_task, 1, &mq_h, rev_485_buf);
			if(ret > 0)
			{
				fprintf(stderr,"mqd_485_2_task 接受消息 mq_h.cmd = %d rev_485_buf[0] = %d",mq_h.cmd,rev_485_buf[0]);
				if(mq_h.cmd ==1)
				{
					taskIndex  = rev_485_buf[0];
				}
			}
		}
		break;
	case 31:

		break;
		default:
		{
				if(mqd_485_1_task < 0)
				{
					fprintf(stderr,"mqd_485_1_task:mq_open_ret=%d\n",mqd_485_1_task);
					return taskIndex;
				}
				ret = mmq_get(mqd_485_1_task, 1, &mq_h, rev_485_buf);
				if(ret > 0)
				{
					fprintf(stderr,"mqd_485_2_task 接受消息 mq_h.cmd = %d rev_485_buf[0] = %d",mq_h.cmd,rev_485_buf[0]);
					if(mq_h.cmd ==1)
					{
						taskIndex  = rev_485_buf[0];
					}
				}
			}

		}
	return taskIndex;
}
void read485_thread(void* i485port) {
	INT8U port = *(INT8U*) i485port;
	fprintf(stderr, "\n port = %d", port);

	INT8U ret = 0;

#ifdef TESTDEF1
	INT8S result = getComfdBy6001(3,2);
	if(result != 1)
	{
		fprintf(stderr,"\n 打开串口失败");
	}
	INT8U jinchang[25] = {0x68,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x68,0x14,0x0D,
						0x34,0x33,0xFF,0x37,0x35,0x33,0x33,0x33,0x44,0x44,
						0x44,0x44,0x44,0xAA,0x16};
	INT8U chgAddr[18]= {0x68,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0x68,0x15,0x06,
						0x44,0x55,0x66,0x77,0x88,0x99,0x7F,0x16};
	INT16S RecvLen = -1;
	INT8U RecvBuff[256] = {0};

	SendDataTo485(2, jinchang, 25);
	RecvLen = ReceDataFrom485(DLT_698,2, 500, RecvBuff);
	if (RecvLen > 0)
	{
		fprintf(stderr,"\n 进场报文收到回复");
	}
	sleep(5);

	memset(RecvBuff,0,256);
	SendDataTo485(2, chgAddr, 18);
	RecvLen = ReceDataFrom485(DLT_698,2, 500, RecvBuff);
	if (RecvLen > 0)
	{
		fprintf(stderr,"\n 修改地址报文收到回复");
	}
	return;
#endif

	while (1) {

		dealRealTimeRequst(port);
		INT16S taskIndex = getTaskIndex(port);

		if(taskIndex > -1)
		{
			fprintf(stderr,"\n read485_thread ---------port = %d ------ taskIndex = %d \n",port,taskIndex);
			DbgPrintToFile1(port,"******************************************taskIndex = %d 任务开始*******************************",taskIndex);
			CLASS_6035 result6035;	//采集任务监控单元
			memset(&result6035, 0x00, sizeof(CLASS_6035));
			result6035.taskID = list6013[taskIndex].basicInfo.taskID;
			result6035.taskState = IN_OPR;
			DataTimeGet(&result6035.starttime);

			CLASS_6015 to6015;	//采集方案集
			memset(&to6015, 0, sizeof(CLASS_6015));
			switch (list6013[taskIndex].basicInfo.cjtype) {
			case norm:/*普通采集方案*/
			{
				ret = use6013find6015(list6013[taskIndex].basicInfo.sernum,
						&to6015);
				ret = deal6015(to6015, port,&result6035);
			}
				break;
			case events:/*事件采集方案*/
			{

			}
				break;
			case tran:/*透明采集方案*/
			{

			}
				break;
			case rept:/*上报方案*/
			{

			}
				break;
			case scpt:/*脚本方案*/
			{

			}
				break;
			}
			DataTimeGet(&result6035.endtime);
			result6035.taskState = AFTER_OPR;

			saveCoverClass(0x6035, result6035.taskID, &result6035,
					sizeof(CLASS_6035), coll_para_save);

			fprintf(stderr,"\n发送报文数量：%d  接受报文数量：%d",result6035.sendMsgNum,result6035.rcvMsgNum);
			DbgPrintToFile1(port,"****************taskIndex = %d 任务结束 发送报文数量：%d  接受报文数量：%d*******************************",
					taskIndex,result6035.sendMsgNum,result6035.rcvMsgNum);
			//判断485故障事件
			if((result6035.sendMsgNum > 0)&&(result6035.rcvMsgNum<=0))
			{
				Event_310A(c485_err,JProgramInfo);
			}
		}
		else
		{

		}

		sleep(1);
	}

	pthread_detach(pthread_self());
	if (port == 1) {
		fprintf(stderr, "485 1 线程退出");
		pthread_exit(&thread_read4851);

	}

	if (port == 2) {
		fprintf(stderr, "485 1 线程退出");
		pthread_exit(&thread_read4852);

	}

	sleep(1);

}
INT8U initMap07DI_698OAD()
{
	map07DI_698OAD_NUM = 0;
	INT8U linenum = 0;
	char aline[MAXLEN_1LINE];
	OI_698 oi698 = 0xffff;
	OI_698 oadOI = 0xffff;
	INT8U unitnum =0,dataType = 0;


	INT8U dataflag07[DF07_BYTES];
	INT8U dataInfo[DF07_INFO_BYTES];


	memset(&map07DI_698OAD[0], 0, sizeof(CLASS_601F)*NUM_07DI_698OAD);

	FILE* fp = fopen((const char*)CLASS_601F_CFG_FILE,(const char*)"r");
	if(fp == NULL)
	{
		fprintf(stderr,"\nopen CLASS_601F_CFG_FILE failed!");
		return linenum;
	}
	memset(aline,0,MAXLEN_1LINE);

	while(fgets((char*)aline,MAXLEN_1LINE,fp) != NULL)
	{
		if(index((char*)aline,'#') == (char*)&aline[0])
		{
			memset(aline,0,MAXLEN_1LINE);
			continue;
		}

		if (strstr(aline,"[begin-"))
		{
			int tempvalue;
			sscanf((char*)aline,"[begin-%04x]",&tempvalue);
			oi698 = (OI_698)tempvalue;
			memset(aline,0,MAXLEN_1LINE);
			continue;
		}
		if (strstr(aline,"[end-"))
		{
			oi698 = 0xffff;
			memset(aline,0,MAXLEN_1LINE);
			continue;
		}
		if(oi698==0xffff)
		{
			continue;
		}

		fprintf(stderr,"\n oi698 = %04x\n",oi698);


		memset(dataflag07,0xff,DF07_BYTES);
		memset(dataInfo,0,DF07_INFO_BYTES);

		int val[3] = {0};

		fprintf(stderr,"\n------------------------");
		sscanf((const char*)aline,"%04x %s %d %d %s",&val[0],dataflag07,&val[1],&val[2],dataInfo);


		oadOI = (OI_698)val[0];
		unitnum = (INT8U)val[1];
		dataType = (INT8U)val[2];

		INT8U asc07[DF07_BYTES*2];
		memset(asc07,0,DF07_BYTES*2);
		memcpy(asc07,dataflag07,DF07_BYTES*2);

		INT8U buf07[DF07_BYTES];
		asc2bcd(asc07,DF07_BYTES*2,buf07,positive);
		reversebuff(buf07, 4, dataflag07);


		fprintf(stderr,"\n linenum = %d  oadOI=%04x  datatype = %d unitnum= %d	dataflag07 = %02x-%02x-%02x-%02x dataInfo = %s\n"
						,linenum,oadOI,dataType,unitnum,dataflag07[0],dataflag07[1],dataflag07[2],dataflag07[3],dataInfo);

		map07DI_698OAD[linenum].roadOI = oi698;
		map07DI_698OAD[linenum].flag698.OI = oadOI;
		map07DI_698OAD[linenum].flag698.attflg = 0x02;
		map07DI_698OAD[linenum].flag698.attrindex = 0x00;
		map07DI_698OAD[linenum].unitnum = unitnum;
		map07DI_698OAD[linenum].datatype = dataType;
		map07DI_698OAD[linenum].flag07.dinum =1;
		memcpy(map07DI_698OAD[linenum].flag07.DI_1,dataflag07,DF07_BYTES);
		linenum++;
		memset(aline,0,MAXLEN_1LINE);


		if(linenum >= NUM_07DI_698OAD)
		{
			fprintf(stderr,"\n 07DI_698OAD.cfg数据项超过 NUM_07DI_698OAD \n");
			return linenum;
		}

	}
	if (linenum < 0)
	{
		fprintf(stderr,"\nLoad  07DI_698OAD.cfg Error!!!     lineNum=%d\n", linenum);
	}
	else
	{
		fprintf(stderr,"\nLoad  07DI_698OAD.cfg  OK!!!        lineNum=%d\n", linenum);
	}

	return linenum;


}

void read485_proccess() {

	map07DI_698OAD_NUM = initMap07DI_698OAD();

	i485port1 = 1;
	i485port2 = 2;
	comfd4851 = -1;
	comfd4852 = -1;
	readState = 0;

	struct mq_attr attr_485_main;
	mqd_485_main = mmq_open((INT8S *)PROXY_485_MQ_NAME,&attr_485_main,O_RDONLY);

	struct mq_attr attr_485_1_task;
	mqd_485_1_task = mmq_open((INT8S *)TASKID_485_1_MQ_NAME,&attr_485_1_task,O_RDONLY);

	struct mq_attr attr_485_2_task;
	mqd_485_2_task = mmq_open((INT8S *)TASKID_485_2_MQ_NAME,&attr_485_2_task,O_RDONLY);


	pthread_attr_init(&read485_attr_t);
	pthread_attr_setstacksize(&read485_attr_t, 2048 * 1024);
	pthread_attr_setdetachstate(&read485_attr_t, PTHREAD_CREATE_DETACHED);

	while ((thread_read4851_id = pthread_create(&thread_read4851,&read485_attr_t, (void*) read485_thread, &i485port1)) != 0)
	{
		sleep(1);
	}

	while ((thread_read4852_id=pthread_create(&thread_read4852, &read485_attr_t, (void*)read485_thread, &i485port2)) != 0)
	{
		sleep(1);
	}

}

