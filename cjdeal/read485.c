
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

extern ProgramInfo* JProgramInfo;

#ifdef TESTDEF
INT8U flag07_0CF177[4] =  {0x00,0xff,0x00,0x00};//当前组合有功总电能示值
INT8U flag07_0CF33[4] =   {0x00,0xff,0x01,0x00};//当前正向有功总电能示值
INT8U flag07_0CF34[4] =   {0x00,0xff,0x02,0x00};//当前反向有功总电能示值
INT8U flag07_0CZHWG1[4] =  {0x00,0x01,0x30,0x00};//当前组合无功1
INT8U flag07_0CZHWG2[4] =  {0x00,0x01,0x40,0x00};//当前组合无功2
INT8U flag07_0C1XXWG[4] =  {0x00,0xff,0x50,0x00};//第一象限无功
INT8U flag07_0C2XXWG[4] =  {0x00,0xff,0x60,0x00};//第二象限无功
INT8U flag07_0C3XXWG[4] =  {0x00,0xff,0x70,0x00};//第三象限无功
INT8U flag07_0C4XXWG[4] =  {0x00,0xff,0x80,0x00};//第四象限无功
INT8U flag07_0CF25_1[4] = {0x00,0xff,0x01,0x02};//当前电压
INT8U flag07_0CF25_2[4] = {0x00,0xff,0x02,0x02};//当前电流
INT8U freezeflag07_1[4] = {0x01,0x00,0x06,0x05};//上一次日冻结时标
INT8U freezeflag07_2[4] = {0x01,0x01,0x06,0x05};//上一次日冻结正向有功总电能示值
INT8U freezeflag07_3[4] = {0x01,0x02,0x06,0x05};//上一次日冻结反向有功总电能示值
INT8U flag07_date[4] 	= {0x01,0x01,0x00,0x04};//电能表日历时钟-日期
INT8U flag07_time[4]	= {0x02,0x01,0x00,0x04};//电能表日历时钟-时间
#endif

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
#ifdef TESTDEF
		if (class6015.csds.csd[i].type == 1) {
					memcpy(&testArray[0].flag698.road, &class6015.csds.csd[i].csd.road,
							sizeof(ROAD));

					memcpy(testArray[0].flag07.DI_1[0], freezeflag07_1, 4);
					memcpy(testArray[0].flag07.DI_1[1], freezeflag07_2, 4);
					memcpy(testArray[0].flag07.DI_1[2], freezeflag07_3, 4);
					testArray[0].flag07.dinum = 1;
				}

		if (class6015.csds.csd[i].type == 0) {
			fprintf(stderr,"\n 2-----class6015.csds.csd[i].csd.oad = %04x",class6015.csds.csd[i].csd.oad.OI);
			if(class6015.csds.csd[i].csd.oad.OI== 0x0010)
			{
				memcpy(&testArray[1].flag698.oad, &class6015.csds.csd[i].csd.oad,sizeof(OAD));
				memcpy(testArray[1].flag07.DI_1[0], flag07_0CF33, 4);
				testArray[1].flag07.dinum = 1;
			}
			if(class6015.csds.csd[i].csd.oad.OI== 0x2000)
			{
				memcpy(&testArray[2].flag698.oad, &class6015.csds.csd[i].csd.oad,sizeof(OAD));
				memcpy(testArray[2].flag07.DI_1[0], flag07_0CF25_1, 4);
				testArray[2].flag07.dinum = 1;
			}
			if(class6015.csds.csd[i].csd.oad.OI== 0x2001)
			{
				memcpy(&testArray[3].flag698.oad, &class6015.csds.csd[i].csd.oad,sizeof(OAD));
				memcpy(testArray[3].flag07.DI_1[0], flag07_0CF25_2, 4);
				testArray[3].flag07.dinum = 1;
			}
#if 0
			INT8U flag07_date[4] = { 0x01, 0x01, 0x00, 0x04 };//电能表日历时钟-日期
			memcpy(testArray[0].flag07.DI_1[3], flag07_date, 4);
			INT8U flag07_time[4] = { 0x02, 0x01, 0x00, 0x04 };//电能表日历时钟-时间
			memcpy(testArray[0].flag07.DI_1[4], flag07_time, 4);
#endif
		}

#endif

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

	if ((lastbaud == baud) && ((lastport == port))) {
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

	//fprintf(stderr,"\n open_com_para_chg port = %d baud = %d newfd = %d",port,baud, newfd);

	newfd = OpenCom(port, baud, (unsigned char *) "even", 1, 8);

	lastport = port;
	lastbaud = baud;

	return newfd;
}
INT8S getComfdBy6001(INT8U baud,INT8U port)
{
	INT8S result = -1;
	INT32U baudrate = getMeterBaud(baud);

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
INT16S ReceDataFrom485(INT32S fd, INT16U delayms, INT8U *str) {
	INT8U TmprevBuf[256];	//接收报文临时缓冲区
	INT8U prtstr[50];
	INT16U len_Total = 0, len, rec_step, rec_head, rec_tail, DataLen, i, j;

	if (fd <= 2)
		return -1;

	memset(TmprevBuf, 0, 256);
	rec_head = rec_tail = rec_step = DataLen = 0;
	fprintf(stderr, "delayms=%d, 111111111111111\n", delayms);
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
			if ((rec_head - rec_tail) >= 10) {
				if (str[rec_tail] == 0x68 && str[rec_tail + 7] == 0x68) {
					DataLen = str[rec_tail + 9];	//获取报文数据块长度
					rec_step = 2;
					break;
				} else
					rec_tail++;
			}
			break;
		case 2:
			if ((rec_head - rec_tail) >= (DataLen + 2)) {
				if (str[rec_tail + 9 + DataLen + 2] == 0x16) {
//					DbPrt1("R:",(char *)str, rec_head, NULL);
					return rec_head;
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
void SendDataTo485(INT32S fd, INT8U *sendbuf, INT16U sendlen) {
	ssize_t slen;

	INT8U str[50];
	memset(str, 0, 50);
	sprintf((char *) str, "485(%d)_S(%d):", 1, sendlen);
	printbuff((char *) str, sendbuf, sendlen, "%02x", " ", "\n");

	slen = write(fd, sendbuf, sendlen);
	if (slen < 0)
		fprintf(stderr, "slen=%d,send err!\n", slen);
//	DbPrt1("S:", (char *) sendbuf, sendlen, NULL);
}
//根据TSA从文件中找出6001
INT8U get6001Obj2TSA(TSA addr,CLASS_6001* targetMeter)
{
	INT8U ret = 0;
	TSA targetMeterAddr;
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
#ifdef TESTDEF1
				if(memcmp(targetMeterAddr.addr,targetMeter->basicinfo.addr.addr,sizeof(TSA))==0)
#endif
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
		ret = Event_310B(meter.basicinfo.addr,taskID,resultData07.Data,resultData07.Length,JProgramInfo);

		ret = Event_310C(meter.basicinfo.addr,taskID,resultData07.Data,resultData07.Length,JProgramInfo,meter);

		ret = Event_310D(meter.basicinfo.addr,taskID,resultData07.Data,resultData07.Length,JProgramInfo,meter);

		ret = Event_310B(meter.basicinfo.addr,taskID,resultData07.Data,resultData07.Length,JProgramInfo);
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
	if(memcmp(flag07_0CF25_1,DI07->DI,4) == 0)
	{
		*dataType = dtlongunsigned;
		unitNum = 3;
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
	//冻结时标数据07为5个字节 698为7个 需要特殊处理
	if(memcmp(freezeflag07_1,DI07->DI,4) == 0)
	{
		*dataType = dtdatetimes;
		DI07->Length += 2;
		fprintf(stderr,"\n 1-----getASNInfo dataType = %d",*dataType);
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
		CLASS_6035* st6035,INT8U* dataContent,CLASS_6001 meter)
{
	BOOLEAN nextFlag = 0;
	INT8S recsta = 0;
	INT16S RecvLen = 0;
	INT8U RecvBuff[256];
	INT16S buffLen = -1;
	memset(&RecvBuff[0], 0, 256);
	SendDataTo485(comfd4851, SendBuf, SendLen);
	st6035->sendMsgNum++;
	RecvLen = ReceDataFrom485(comfd4851, 500, RecvBuff);
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
INT16S request698_07Data(INT8U* DI07,INT8U* dataContent,CLASS_6001 meter,CLASS_6035* st6035)
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
	fprintf(stderr, "\n meterAddr len = %d addr = %02x%02x%02x%02x%02x%02x",
			meter.basicinfo.addr.addr[0], meter.basicinfo.addr.addr[1], meter.basicinfo.addr.addr[2],
			meter.basicinfo.addr.addr[3], meter.basicinfo.addr.addr[4], meter.basicinfo.addr.addr[5],
			meter.basicinfo.addr.addr[6]);
	fprintf(stderr, "\nDI = %02x%02x%02x%02x\n",DI07[0],DI07[1],DI07[2],DI07[3]);

	Data07.Ctrl = CTRL_Read_07;
	if(meter.basicinfo.addr.addr[0] > 6)
	{
		meter.basicinfo.addr.addr[0] = 6;
		fprintf(stderr,"request698_07Data 电表地址长度大于6");
	}

	memcpy(&Data07.Addr, &meter.basicinfo.addr.addr[1], meter.basicinfo.addr.addr[0]);
	memcpy(&Data07.DI, DI07, 4);

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
				request698_07DataSingle(&Data07,SendBuff,SendLen,st6035,dataContent,meter);

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
INT8S CSDMap07DI(MY_CSD strCAD, C601F_07Flag* obj601F_07Flag) {
	fprintf(stderr, "\n CSDMap07DI--------start--------\n");
	printMY_CSD(strCAD);

	INT8S result = 0;
	INT8U index;

	for (index = 0; index < TESTARRAYNUM; index++) {
		if (strCAD.type == 0) {
			if (memcmp(&strCAD.csd.oad, &testArray[index].flag698.oad,
					sizeof(OAD)) == 0) {
				fprintf(stderr, "\n find CSDMap07DI OAD");
				memcpy(obj601F_07Flag, &testArray[index].flag07, sizeof(C601F_07Flag));
				return 1;
			}
		}
		if (strCAD.type == 1) {
			if (memcmp(&strCAD.csd.road, &testArray[index].flag698.road,
					sizeof(ROAD)) == 0) {
				fprintf(stderr, "\n find CSDMap07DI ROAD");
				memcpy(obj601F_07Flag, &testArray[index].flag07, sizeof(C601F_07Flag));
				return 1;
			}
		}

	}

	fprintf(stderr, "\n CSDMap07DI--------end--------\n");
	return result;
}

INT16U dealProxy_645_07(GETOBJS obj07,INT8U* dataContent)
{
	INT16U singledataLen = -1;
	INT16U dataLen = 0;
	INT16U dataFlagPos = 0;
	CLASS_6001 meter = {};
	CLASS_6035 inValid6035 = {};

	INT8U oadIndex;
	INT8U diIndex;
	dataContent[dataLen++] = obj07.num;
	for(oadIndex = 0;oadIndex < obj07.num;oadIndex++)
	{
		OADtoBuff(obj07.oads[oadIndex],&dataContent[dataLen]);
		dataLen += sizeof(OAD);

		C601F_07Flag obj601F_07Flag;
		memset(&obj601F_07Flag,0,sizeof(C601F_07Flag));
		MY_CSD strCAD;
		strCAD.type = 0;
		memcpy(&strCAD.csd.oad,&obj07.oads[oadIndex],sizeof(OAD));

		if(CSDMap07DI(strCAD,&obj601F_07Flag)!=1)
		{
			fprintf(stderr,"\n 找不到%04x%02x%02x对应07数据项",strCAD.csd.oad.OI,strCAD.csd.oad.attflg,strCAD.csd.oad.attrindex);
			continue;
		}
		memcpy(meter.basicinfo.addr.addr,obj07.tsa.addr,sizeof(TSA));
		dataFlagPos = dataLen;
		dataContent[dataLen++] = 0x01;//默认有数据

		for(diIndex=0;diIndex<obj601F_07Flag.dinum;diIndex++)
		{
			singledataLen = request698_07Data(obj601F_07Flag.DI_1[diIndex],&dataContent[dataLen],meter,&inValid6035);
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
	fprintf(stderr,"\n dealProxy_645_07 dataLen = %d",dataLen);
	return dataLen;
}
INT8S dealProxy(PROXY_GETLIST* getlist,INT8U port485)
{
	INT8S result = -1;
	//判断代理是否已经超时
	time_t nowtime = time(NULL);
	fprintf(stderr,"\n\n getlist->timeout = %d",getlist->timeout);
	if(nowtime > (getlist->timeout + getlist->timeold))
	{
		fprintf(stderr,"\n 代理请求超时");
		getlist->status = 3;
		return result;
	}
	fprintf(stderr,"\n dealProxy--------1 addr objs num = %d :",getlist->num);
	INT8U index;
	INT16U totalLen = 0;
	INT16U singleLen = 0;
	getlist->data[totalLen++] = getlist->num;
	for(index = 0;index < getlist->num;index++)
	{
		CLASS_6001 obj6001 = {};
		memcpy(&getlist->data[totalLen],getlist->objs[index].tsa.addr,sizeof(TSA));
		totalLen += sizeof(TSA);

		//通过表地址找 6001
		if(get6001Obj2TSA(getlist->objs[index].tsa,&obj6001) != 1 )
		{
			fprintf(stderr," dealProxy--------2 未找到相应6001");
			getlist->data[totalLen++] = 0;//没有数据
			continue;
		}
		if(is485OAD(obj6001.basicinfo.port,port485) == 0)
		{
			fprintf(stderr," dealProxy--------3 非本端口测量点");
			getlist->data[totalLen++] = 0;//没有数据
			continue;
		}
		if(getComfdBy6001(obj6001.basicinfo.port.attrindex,port485) != 1)
		{
			fprintf(stderr," dealProxy--------4");
			getlist->data[totalLen++] = 0;//没有数据
			continue;
		}
		getlist->data[totalLen++] = getlist->objs[index].num;
#ifdef TESTDEF1
		if(obj6001.basicinfo.protocol == DLT_645_07)
#endif
		{
			singleLen = dealProxy_645_07(getlist->objs[index],&getlist->data[totalLen]);
			if(singleLen > 0)
			{
				totalLen += singleLen;
			}
		}
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
			return result;
	}
	return result;
}
//处理代理等实时请求
INT8S dealRealTimeRequst(INT8U port485)
{

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

		if (ret>0)
		{
			//备份当前comfd4851 comfd4852，代理执行完后重新赋值
			INT32S comfd_BK = -1;
			if(port485 == S4851)
			{
				comfd_BK = comfd4851;
			}
			if(port485 == S4852)
			{
				comfd_BK = comfd4852;
			}

			fprintf(stderr, "\n\n-----------------vs485_main recvMsg!!!    cmd=%d!!!---------------\n", mq_h.cmd);
			DealRecvMsg(mq_h, rev_485_buf,port485);

			if(port485 == S4851)
			{
				comfd4851 = comfd_BK;
			}
			if(port485 == S4852)
			{
				comfd4851 = comfd_BK;
			}
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
INT16U deal6015_698(CLASS_6015 st6015, CLASS_6001 to6001,CLASS_6035* st6035,INT8U port485)
{
	dealRealTimeRequst(port485);
	fprintf(stderr, "\n deal6015_698  meter = %d", to6001.sernum);
	INT16U retLen = 0;
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
#if TSETDEF1
	subindex = 0;
	while(subindex < 3)
	{
		memset(recvbuff, 0, BUFFSIZE);
		SendDataTo485(comfd4851, sendbuff, sendLen);
		st6035->sendMsgNum++;
		recvLen = ReceDataFrom485(comfd4851, 500, recvbuff);
		if(recvLen > 0)
		{
			st6035->rcvMsgNum++;
			recsta = analyzeProtocol698(format07, RecvBuff, RecvLen, &nextFlag);
		}

		subindex++;
	}
#endif


	return retLen;
}
/*
 * DI07List[10][4]是一个CSD对应的07数据标识列表
 * dataContent里保存一个任务抄上来的所有数据，不带数据标识
 * */
INT16U request698_07DataList(	C601F_07Flag obj601F_07Flag, CLASS_6001 meter,INT8U* dataContent,CLASS_6035* st6035,INT8U port485)
{
	INT16U DataLen = 0;	//暂存正常抄读的数据长度
	INT8U index;
	INT16S singleBuffLen = 0;
	INT8U isSuccess = 1;
	fprintf(stderr,"\n\n-------request698_07DataList obj601F_07Flag.dinum = %d",obj601F_07Flag.dinum);
	for (index = 0; index < obj601F_07Flag.dinum; index++)
	{
		dealRealTimeRequst(port485);
		singleBuffLen = request698_07Data(obj601F_07Flag.DI_1[index],&dataContent[DataLen],meter,st6035);

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

INT16U deal6015_07(CLASS_6015 st6015, CLASS_6001 to6001,CLASS_6035* st6035,INT8U* dataContent,INT8U port485) {
	INT16U totaldataLen =0;
	INT16U datalen = 0;
	fprintf(stderr,
			"\n\n-------start------------ deal6015_07  meter = %d st6015.sernum = %d st6015.csds.num = %d---------",
			to6001.sernum, st6015.sernum, st6015.csds.num);
	switch (st6015.cjtype) {
	case TYPE_NULL:/*采集当前数据--实时*/
	{
		fprintf(stderr, "\n deal6015_07 采集当前数据--实时");
	}
		break;
	case TYPE_LAST:/*采集上N次*/
	{
		fprintf(stderr, "\n deal6015_07 采集上N次数据--冻结");
	}
		break;
	case TYPE_FREEZE:/*按冻结时标*/
		break;
	case TYPE_INTERVAL:/*按时标间隔---曲线*/
		break;
	}

	C601F_07Flag obj601F_07Flag;
	memset(&obj601F_07Flag,0,sizeof(C601F_07Flag));

	INT8U dataIndex = 0;

	for (dataIndex = 0; dataIndex < st6015.csds.num; dataIndex++) {
		if (CSDMap07DI(st6015.csds.csd[dataIndex], &obj601F_07Flag) == 1) {
			datalen = request698_07DataList(obj601F_07Flag, to6001,&dataContent[totaldataLen],st6035,port485);
			totaldataLen += datalen;
			fprintf(stderr,"\n deal6015_07 totaldataLen = %d,datalen=%d",totaldataLen,datalen);
			if(totaldataLen >= DATA_CONTENT_LEN)
			{
				fprintf(stderr,"dataContent 长度不够");
				fprintf(stderr,"deal6015_07 datalen = %d totaldataLen = %d",datalen,totaldataLen);
				break;
			}
		} else {
			fprintf(stderr, "request698_07Data:1");
			continue;
		}
	}
	fprintf(stderr,
			"\n\n**********end************ deal6015_07  meter = %d st6015.sernum = %d st6015.csds.num = %d---------",
			to6001.sernum, st6015.sernum, st6015.csds.num);
	return totaldataLen;
}
/*
 * 抄读1个测量点
 */
INT16U deal6015_singlemeter(CLASS_6015 st6015, CLASS_6001 obj6001,CLASS_6035* st6035,INT8U* dataContent,INT8U port485) {
	INT16U ret = 0;
	if(getComfdBy6001(obj6001.basicinfo.baud,obj6001.basicinfo.port.attrindex)!=1)
	{
		return ret;
	}

	switch (obj6001.basicinfo.protocol) {
#ifndef TESTDEF
	case DLT_645_07:
		ret = deal6015_07(st6015, obj6001,st6035,dataContent,port485);
	break;
	default:
		ret = deal6015_698(st6015,obj6001,st6035,port485);
#else
	case DLT_645_07:
	case DLT_698:
	ret = deal6015_07(st6015, obj6001,st6035,dataContent,port485);
#endif
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
		fprintf(stderr,"\n checkMeterType 非485");
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

	for (mIndex = groupIndex * LIST6001SIZE; mIndex < endIndex; mIndex++) {
		if (readParaClass(oi, &meter, mIndex) == 1) {

			if (meter.sernum != 0 && meter.sernum != 0xffff) {
				if (checkMeterType(mst, port485, meter.basicinfo.addr,
						meter.basicinfo.port)) {
						memcpy(&list6001[mIndex],&meter,sizeof(CLASS_6001));
				} else {
					fprintf(stderr, "\n序号:%d 非485测量点 %04X", meter.sernum,
							meter.basicinfo.port.OI);
				}
			}
		}
	}

	return result;
}
INT16U compose6012Buff(DateTimeBCD startTime,TSA meterAddr,INT16U dataLen,INT8U* dataContent)
{
	fprintf(stderr,"\n 存储数据  compose6012Buff--------------");
	INT16U index;
	INT16U bufflen = 0;
	DateTimeBCD endTime;
	DataTimeGet(&endTime);
	INT8U buff6012[DATA_CONTENT_LEN];
	memset(buff6012,0,DATA_CONTENT_LEN);
	buff6012[bufflen++] = 0x55;
	memcpy(&buff6012[bufflen],meterAddr.addr,sizeof(TSA));//采集通信地址
	bufflen += sizeof(TSA);

	buff6012[bufflen++] = dtdatetimes;
	memcpy(&buff6012[bufflen],&startTime,sizeof(DateTimeBCD));//采集启动时标
	bufflen += sizeof(DateTimeBCD);

	buff6012[bufflen++] = dtdatetimes;
	memcpy(&buff6012[bufflen],&endTime,sizeof(DateTimeBCD));//采集成功时标
	bufflen += sizeof(DateTimeBCD);

	buff6012[bufflen++] = dtdatetimes;
	memcpy(&buff6012[bufflen],&endTime,sizeof(DateTimeBCD));//采集存储时标
	bufflen += sizeof(DateTimeBCD);

	memcpy(&buff6012[bufflen],dataContent,dataLen);
	bufflen += dataLen;

	memset(dataContent,0,DATA_CONTENT_LEN);
	memcpy(dataContent,buff6012,bufflen);
	fprintf(stderr,"\n\n buff6012[%d]:",bufflen);
	for(index = 0;index < bufflen;index++)
	{
		fprintf(stderr," %02x",buff6012[index]);
		if((index+1)%20 == 0)
		{
			fprintf(stderr,"\n");
		}
	}
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
		dealRealTimeRequst(port485);
		for (mpIndex = 0; mpIndex < LIST6001SIZE; mpIndex++)
		{
			if (list6001[mpIndex].sernum > 0)
			{
				st6035->totalMSNum++;
				fprintf(stderr,"\n\n 任务号:%d  方案号:%d deal6015 测量点 = %d-----",st6035->taskID,st6015.sernum,list6001[mpIndex].sernum);
				INT8U dataContent[DATA_CONTENT_LEN];
				memset(dataContent,0,DATA_CONTENT_LEN);
				INT16U dataLen = 0;
				DateTimeBCD startTime;

				DataTimeGet(&startTime);
				dataLen = deal6015_singlemeter(st6015, list6001[mpIndex],st6035,dataContent,port485);

				if(dataLen > 0)
				{
					int bufflen = compose6012Buff(startTime,list6001[mpIndex].basicinfo.addr,dataLen,dataContent);
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

INT8U time_in_shiduan(TASK_RUN_TIME str_runtime) {
	TS ts_now;
	TSGet(&ts_now);

	INT16U min_start, min_end, now_min;	//距离0点0分
	now_min = ts_now.Hour * 60 + ts_now.Minute;
	INT8U timePartIndex = 0;
	for (timePartIndex = 0; timePartIndex < str_runtime.num; timePartIndex++)
	{
		min_start = str_runtime.runtime[timePartIndex].beginHour * 60
				+ str_runtime.runtime[timePartIndex].beginMin;
		min_end = str_runtime.runtime[timePartIndex].endHour * 60
				+ str_runtime.runtime[timePartIndex].endMin;
		if (min_start <= min_end) {
			if ((now_min > min_start) && (now_min < min_end)) {
				return 1;
			} else if (((str_runtime.type & 0x01) == 0x01)
					&& (now_min == min_end)) {
				return 1;
			} else if (((str_runtime.type & 0x03) == 0x01)
					&& (now_min == min_start)) {
				return 1;
			}
		}
	}
	return 0;
}
//时间在任务开始结束时间段内 0:任务开始 1：任务不执行
INT8U time_in_task(CLASS_6013 from6012_curr) {
	struct tm tm_start;
	struct tm tm_end;
	struct tm tm_curr;
	if (from6012_curr.startime.year.data < 1900
			|| from6012_curr.startime.month.data < 1
			|| from6012_curr.endtime.year.data < 1900
			|| from6012_curr.endtime.month.data < 1) {
		fprintf(stderr, "\n time_in_task - 1");
		return 1;	//无效，任务不执行
	}

	memset(&tm_start, 0x00, sizeof(struct tm));
	tm_start.tm_year = from6012_curr.startime.year.data - 1900;
	tm_start.tm_mon = from6012_curr.startime.month.data - 1;
	tm_start.tm_mday = from6012_curr.startime.day.data;
	tm_start.tm_hour = from6012_curr.startime.hour.data;
	tm_start.tm_min = from6012_curr.startime.min.data;
	tm_start.tm_sec = from6012_curr.startime.sec.data;

	memset(&tm_end, 0x00, sizeof(struct tm));
	tm_end.tm_year = from6012_curr.endtime.year.data - 1900;
	tm_end.tm_mon = from6012_curr.endtime.month.data - 1;
	tm_end.tm_mday = from6012_curr.endtime.day.data;
	tm_end.tm_hour = from6012_curr.endtime.hour.data;
	tm_end.tm_min = from6012_curr.endtime.min.data;
	tm_end.tm_sec = from6012_curr.endtime.sec.data;

	time_t curr_time_t = time(NULL );
	localtime_r(&curr_time_t, &tm_curr);
#if 0
	fprintf(stderr,"\n start year = %d mon = %d day = %d hour=%d  min=%d",
			tm_start.tm_year,tm_start.tm_mon,tm_start.tm_mday,tm_start.tm_hour,tm_start.tm_min);
	fprintf(stderr,"\n end year = %d mon = %d day = %d hour=%d  min=%d",
			tm_end.tm_year,tm_end.tm_mon,tm_end.tm_mday,tm_end.tm_hour,tm_end.tm_min);
	fprintf(stderr,"\n curr year = %d mon = %d day = %d hour=%d  min=%d",
			tm_curr.tm_year,tm_curr.tm_mon,tm_curr.tm_mday,tm_curr.tm_hour,tm_curr.tm_min);
#endif
	if ((tm_curr.tm_year >= tm_start.tm_year)
			&& (tm_curr.tm_year <= tm_end.tm_year)) {
		if (tm_start.tm_year == tm_end.tm_year) {
			tm_start.tm_year = 0;
			tm_end.tm_year = 0;
			tm_curr.tm_year = 0;
			time_t currsec = mktime(&tm_curr);
			time_t startsec = mktime(&tm_start);
			time_t endsec = mktime(&tm_end);
			if ((currsec >= startsec) && (currsec <= endsec)) {
				return 0;
			} else {
				return 1;
			}
		} else {
			return 0;
		}
	} else {
		return 1;
	}
	return 0;
}

/*
 * 过滤掉
 * 状态不对
 * 时段不符合
 * 要求的任务
 *
 * */
INT8U filterInvalidTask(INT16U taskIndex) {

	if (list6013[taskIndex].basicInfo.taskID == 0) {
		fprintf(stderr, "\n filterInvalidTask - 1");
		return 0;
	}
	if (list6013[taskIndex].basicInfo.state == task_novalid)	//任务无效
			{
		fprintf(stderr, "\n filterInvalidTask - 2");
		return 0;
	}
	if (time_in_task(list6013[taskIndex].basicInfo) == 1)	//不在任务执行时段内
	{
		fprintf(stderr, "\n filterInvalidTask - 3");
		return 0;
	}
	if (time_in_shiduan(list6013[taskIndex].basicInfo.runtime) == 1)	//在抄表时段内
	{
		return 1;
	}
	return 0;
}

/*
 * 计算下一次抄读该任务的时间
 *
 * */
void getTaskNextTime(INT16U taskIndex) {
	TSGet(&list6013[taskIndex].ts_next);
	fprintf(stderr,"\n getTaskNextTime 任务ID = %d",list6013[taskIndex].basicInfo.taskID);
	fprintf(stderr,"\n 本次抄表时间 %04d-%02d-%02d %02d:%02d:%02d",
			list6013[taskIndex].ts_next.Year,list6013[taskIndex].ts_next.Month,list6013[taskIndex].ts_next.Day,
			list6013[taskIndex].ts_next.Hour,list6013[taskIndex].ts_next.Minute,list6013[taskIndex].ts_next.Sec);
	tminc(&list6013[taskIndex].ts_next,
			list6013[taskIndex].basicInfo.interval.units,
			list6013[taskIndex].basicInfo.interval.interval);
	fprintf(stderr,"\n 下次抄表时间 %04d-%02d-%02d %02d:%02d:%02d",
				list6013[taskIndex].ts_next.Year,list6013[taskIndex].ts_next.Month,list6013[taskIndex].ts_next.Day,
				list6013[taskIndex].ts_next.Hour,list6013[taskIndex].ts_next.Minute,list6013[taskIndex].ts_next.Sec);

}
/*
 * 比较当前时间应该先抄读哪一个任务
 * 比较权重 优先级 >  采集类型（年>月>日>分） > run_flg
 * 返回
 * ：0-优先级一样
 * ：1-taskIndex1先执行
 * ：2-taskIndex2先执行
 * */
INT8U cmpTaskPrio(INT16U taskIndex1, INT16U taskIndex2) {

	if (list6013[taskIndex1].basicInfo.runprio
			> list6013[taskIndex2].basicInfo.runprio) {
		return 1;
	} else if (list6013[taskIndex1].basicInfo.runprio
			< list6013[taskIndex2].basicInfo.runprio) {
		return 2;
	} else if (list6013[taskIndex1].basicInfo.interval.units
			> list6013[taskIndex2].basicInfo.interval.units) {
		return 1;
	} else if (list6013[taskIndex1].basicInfo.interval.units
			< list6013[taskIndex2].basicInfo.interval.units) {
		return 2;
	} else if (list6013[taskIndex1].run_flg > list6013[taskIndex2].run_flg) {
		return 1;
	} else if (list6013[taskIndex1].run_flg < list6013[taskIndex2].run_flg) {
		return 2;
	}
	return 0;
}
//查找下一个执行的任务
INT16S getNextTastIndexIndex() {
	INT16S taskIndex = -1;
	INT16U tIndex = 0;

	for (tIndex = 0; tIndex < TASK6012_MAX; tIndex++)
	{

		if (list6013[tIndex].basicInfo.taskID == 0) {
			continue;
		}
		fprintf(stderr, "\n ---------list6013[%d].basicInfo.taskID = %d ",
				tIndex, list6013[tIndex].basicInfo.taskID);
		//run_flg > 0说明应该抄读还没有抄
		if (list6013[tIndex].run_flg > 0) {
			fprintf(stderr, "\n  getNextTastIndexIndex-2222");
			list6013[tIndex].run_flg++;
		} else {
			//过滤任务无效或者不再抄表时段内的
			if (filterInvalidTask(tIndex) == 0) {
				fprintf(stderr, "\n  getNextTastIndexIndex-3333");
				continue;
			}
			TS tsNow = { };
			TSGet(&tsNow);
			if (TScompare(tsNow, list6013[tIndex].ts_next) == 1) {
				list6013[tIndex].run_flg = 1;
				fprintf(stderr, "\n  getNextTastIndexIndex-4444");
			}
		}

		if (taskIndex == -1)
		{
			if(list6013[tIndex].run_flg > 0)
			{
				fprintf(stderr, "\n  getNextTastIndexIndex-5555");
				taskIndex = tIndex;
			}
			continue;
		}

		if (cmpTaskPrio(taskIndex, tIndex) == 2) {
			fprintf(stderr, "\n  getNextTastIndexIndex-6666");
			taskIndex = tIndex;
			continue;
		}
	}
	return taskIndex;
}

/*
 * 从文件里把所有的任务单元读上来
 * */
INT8U init6013ListFrom6012File() {
	//list6013  初始化下一次抄表时间
	TS ts_now;
	TSGet(&ts_now);

	fprintf(stderr, "\n -------------init6013ListFrom6012File---------------");
	INT8U result = 0;
	memset(list6013, 0, TASK6012_MAX * sizeof(TASK_CFG));
	INT16U tIndex = 0;
	OI_698 oi = 0x6013;
	CLASS_6013 class6013 = { };
	for (tIndex = 0; tIndex < TASK6012_MAX; tIndex++) {
		if (readCoverClass(oi, tIndex, &class6013, sizeof(CLASS_6013),
				coll_para_save) == 1) {
			memcpy(&list6013[tIndex].basicInfo, &class6013, sizeof(CLASS_6013));
			list6013[tIndex].ts_next.Year = ts_now.Year;
			list6013[tIndex].ts_next.Month = ts_now.Month;
			list6013[tIndex].ts_next.Day = ts_now.Day;
			list6013[tIndex].ts_next.Hour = ts_now.Hour;
			list6013[tIndex].ts_next.Minute = ts_now.Minute;
			print6013(list6013[tIndex].basicInfo);
		}
	}

	return result;
}

void read485_thread(void* i485port) {
	INT8U port = *(INT8U*) i485port;
	fprintf(stderr, "\n port = %d", port);
	comfd4851 = -1;
	INT8U ret = 0;
	INT16S tastIndexIndex = -1;

	while (1) {

		dealRealTimeRequst(port);
		tastIndexIndex = getNextTastIndexIndex();

		if (tastIndexIndex > -1) {
			fprintf(stderr, "\n\n\n\n\n*************-read485_thread tastIndexIndex = %d taskID = %d*****************\n",
					tastIndexIndex, list6013[tastIndexIndex].basicInfo.taskID);
			//计算下一次抄读此任务的时间
			getTaskNextTime(tastIndexIndex);

			CLASS_6035 result6035;	//采集任务监控单元
			memset(&result6035, 0xee, sizeof(CLASS_6035));
			result6035.taskID = list6013[tastIndexIndex].basicInfo.taskID;
			result6035.taskState = IN_OPR;
			memcpy(&result6035.starttime,&list6013[tastIndexIndex].basicInfo.startime,sizeof(DateTimeBCD));
			memcpy(&result6035.endtime,&list6013[tastIndexIndex].basicInfo.endtime,sizeof(DateTimeBCD));

			CLASS_6015 to6015;	//采集方案集
			memset(&to6015, 0, sizeof(CLASS_6015));
			switch (list6013[tastIndexIndex].basicInfo.cjtype) {
			case norm:/*普通采集方案*/
			{
				ret = use6013find6015(list6013[tastIndexIndex].basicInfo.sernum,
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
			list6013[tastIndexIndex].run_flg = 0;
			saveCoverClass(0x6035, result6035.taskID, &result6035,
					sizeof(CLASS_6035), coll_para_save);
		} else {
			//fprintf(stderr, "\n 当前无任务可执行");
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
void initTestArray()
{
	//当前正向有功总电能示值
	memset(testArray,0,sizeof(CLASS_601F)*20);

	testArray[5].flag698.oad.OI = 0x0010;
	testArray[5].flag698.oad.attflg = 0x02;
	testArray[5].flag698.oad.attrindex = 0x00;
	memcpy(testArray[5].flag07.DI_1[0], flag07_0CF33, 4);
	testArray[5].flag07.dinum = 1;

	//当前反向有功总电能示值
	testArray[6].flag698.oad.OI = 0x0020;
	testArray[6].flag698.oad.attflg = 0x02;
	testArray[6].flag698.oad.attrindex = 0x00;
	memcpy(testArray[6].flag07.DI_1[0], flag07_0CF34, 4);
	testArray[6].flag07.dinum = 1;

	//当前组合有功总电能示值
	testArray[7].flag698.oad.OI = 0x0000;
	testArray[7].flag698.oad.attflg = 0x02;
	testArray[7].flag698.oad.attrindex = 0x00;
	memcpy(testArray[7].flag07.DI_1[0], flag07_0CF177, 4);
	testArray[7].flag07.dinum = 1;

	//当前组合无功1
	testArray[8].flag698.oad.OI = 0x0030;
	testArray[8].flag698.oad.attflg = 0x02;
	testArray[8].flag698.oad.attrindex = 0x01;
	memcpy(testArray[8].flag07.DI_1[0], flag07_0CZHWG1, 4);
	testArray[8].flag07.dinum = 1;

	//当前组合无功2
	testArray[9].flag698.oad.OI = 0x0040;
	testArray[9].flag698.oad.attflg = 0x02;
	testArray[9].flag698.oad.attrindex = 0x01;
	memcpy(testArray[9].flag07.DI_1[0], flag07_0CZHWG2, 4);
	testArray[9].flag07.dinum = 1;

	//第一象限无功
	testArray[10].flag698.oad.OI = 0x0050;
	testArray[10].flag698.oad.attflg = 0x02;
	testArray[10].flag698.oad.attrindex = 0x00;
	memcpy(testArray[10].flag07.DI_1[0], flag07_0C1XXWG, 4);
	testArray[10].flag07.dinum = 1;

	//第二象限无功
	testArray[11].flag698.oad.OI = 0x0060;
	testArray[11].flag698.oad.attflg = 0x02;
	testArray[11].flag698.oad.attrindex = 0x00;
	memcpy(testArray[11].flag07.DI_1[0], flag07_0C2XXWG, 4);
	testArray[11].flag07.dinum = 1;

	//第三象限无功
	testArray[12].flag698.oad.OI = 0x0070;
	testArray[12].flag698.oad.attflg = 0x02;
	testArray[12].flag698.oad.attrindex = 0x00;
	memcpy(testArray[12].flag07.DI_1[0], flag07_0C3XXWG, 4);
	testArray[12].flag07.dinum = 1;

	//第四象限无功
	testArray[13].flag698.oad.OI = 0x0080;
	testArray[13].flag698.oad.attflg = 0x02;
	testArray[13].flag698.oad.attrindex = 0x00;
	memcpy(testArray[13].flag07.DI_1[0], flag07_0C4XXWG, 4);
	testArray[13].flag07.dinum = 1;

}
void read485_proccess() {
	//读取所有任务文件
	init6013ListFrom6012File();
#ifdef TESTDEF
	initTestArray();
#endif
	INT8U i485port1 = 1;
	pthread_attr_init(&read485_attr_t);
	pthread_attr_setstacksize(&read485_attr_t, 2048 * 1024);
	pthread_attr_setdetachstate(&read485_attr_t, PTHREAD_CREATE_DETACHED);
	while ((thread_read4851_id = pthread_create(&thread_read4851,&read485_attr_t, (void*) read485_thread, &i485port1)) != 0)
	{
		sleep(1);
	}
#if 0
	INT8U i485port2 = 2;
	while ((thread_read4852_id=pthread_create(&thread_read4852, &read485_attr_t, (void*)read485_thread, &i485port2)) != 0)
	{
		sleep(1);
	}
#endif

}

