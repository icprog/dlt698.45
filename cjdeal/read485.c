
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
#if 1//test
		if (class6015.csds.csd[i].type == 0) {
			memcpy(&testArray[0].flag698.oad, &class6015.csds.csd[i].csd.oad,
					sizeof(OAD));
			INT8U flag07_0CF33[4] = { 0x00, 0x01, 0xff, 0x00 };
			memcpy(testArray[0].flag07.DI_1[0], flag07_0CF33, 4);
			INT8U flag07_0CF25_1[4] = { 0x02, 0x01, 0xff, 0x00 };
			memcpy(testArray[0].flag07.DI_1[1], flag07_0CF25_1, 4);
			INT8U flag07_0CF25_2[4] = { 0x02, 0x02, 0xff, 0x00 };
			memcpy(testArray[0].flag07.DI_1[2], flag07_0CF25_2, 4);

		}
		if (class6015.csds.csd[i].type == 1) {
			memcpy(&testArray[1].flag698.road, &class6015.csds.csd[i].csd.road,
					sizeof(ROAD));
			INT8U freezeflag07[4] = { 0x05, 0x06, 0x01, 0x01 };
			memcpy(testArray[1].flag07.DI_1[4], freezeflag07, 4);
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
	newfd = OpenCom(port, baud, (unsigned char *) "even", 1, 8);

	lastport = port;
	lastbaud = baud;
	fprintf(stderr, "open_com_para_chg newfd = %d", newfd);
	return newfd;
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
//			dbg_prtbuff((char *)prtstr, TmprevBuf, len, "%02x", " ", "\n");

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

INT8S deal6015_698(CLASS_6015 st6015, BasicInfo6001 to6001) {
	fprintf(stderr, "\n deal6015_698  meter = %d", to6001.sernum);
	INT8S result = -1;
	INT16S sendLen = 0;
	INT16S recvLen = 0;
	INT8U sendbuff[BUFFSIZE];
	INT8U recvbuff[BUFFSIZE];
	memset(sendbuff, 0, BUFFSIZE);
	memset(recvbuff, 0, BUFFSIZE);

	sendLen = composeProtocol698_GetRequest(sendbuff, st6015, to6001.addr);
	SendDataTo485(comfd4851, sendbuff, sendLen);
	recvLen = ReceDataFrom485(comfd4851, 500, recvbuff);

	return result;
}

INT16U request698_07DataList(INT8U DI07List[10][4], TSA meterAddr,INT8U* dataContent)
{
	INT16U dataLen = 0;
	INT8S recsta = 0;
	BOOLEAN nextFlag = 0;
	FORMAT07 Data07;
	INT16S SendLen = 0, RecvLen = 0;
	INT8U DATA[1024];	//暂存正常抄读的数据
	INT16U DataLen = 0;	//暂存正常抄读的数据长度

	INT8U SendBuff[256], RecvBuff[256];

	INT8U invalidDI[4] = { 0 };
	INT8U index;

	for (index = 0; index < 10; index++) {
		if (memcmp(invalidDI, DI07List[index], 4) == 0) {
			continue;
		}
		fprintf(stderr, "\n meterAddr len = %d addr = %02x%02x%02x%02x%02x%02x",
				meterAddr.addr[0], meterAddr.addr[1], meterAddr.addr[2],
				meterAddr.addr[3], meterAddr.addr[4], meterAddr.addr[5],
				meterAddr.addr[5]);
		fprintf(stderr, "\n request698_07DataList DI[%d] = %02x%02x%02x%02x\n",
				index, DI07List[index][0], DI07List[index][1],
				DI07List[index][2], DI07List[index][3]);
		memset(DATA, 0, sizeof(DATA));
		memset(&SendBuff[0], 0, 256);
		memset(&RecvBuff[0], 0, 256);
		memset(&Data07, 0, sizeof(FORMAT07));

		Data07.Ctrl = CTRL_Read_07;
		memcpy(&Data07.Addr, &meterAddr.addr[1], meterAddr.addr[0]);
		memcpy(&Data07.DI, DI07List[index], 4);
		SendLen = composeProtocol07(&Data07, SendBuff);
		if (SendLen < 0) {
			fprintf(stderr, "request698_07DataList1");
			continue;
		}
		SendDataTo485(comfd4851, SendBuff, SendLen);
		RecvLen = ReceDataFrom485(comfd4851, 500, RecvBuff);
		if (RecvLen > 0) {
			recsta = analyzeProtocol07(&Data07, RecvBuff, RecvLen, &nextFlag);
			if (recsta == 0) {
				fprintf(stderr, "正常应答！  DI07 = %02x%02x%02x%02x\n",
						Data07.DI[3], Data07.DI[2], Data07.DI[1], Data07.DI[0]);
				if (Data07.SEQ == 0)	//无帧序号
						{
				} else {
					INT8U tmpdata[256];
					memcpy(tmpdata, DATA, DataLen);
					memcpy(&tmpdata[DataLen], Data07.Data, Data07.Length - 5);

					Data07.Length = DataLen + Data07.Length - 1;//累加之前几帧报文的数据长度
					memcpy(Data07.Data, tmpdata, Data07.Length);

				}
				recsta = 0;
			} else {
				if (recsta == -1) {
					fprintf(stderr, "电表异常应答，无数据项  %02x%02x%02x%02x！！！\n",
							Data07.DI[3], Data07.DI[2], Data07.DI[1],
							Data07.DI[0]);
				} else if (recsta == -2) {
					fprintf(stderr, "电表异常应答，未知错误！ Err=%02x\n", Data07.Err);
				} else if (recsta == -3) {
					fprintf(stderr, "其他功能！\n");
				} else if (recsta == -4) {
					fprintf(stderr, "校验错误！\n");
				}
			}
		}
	}

	return dataLen;
}
/*
 * 698 OAD 和 645 07规约 数据标识转换
 * dir:0-通过698OAD找64507DI 1-通过64507DI找698OAD
 * */
INT8S CSDMap07DI(INT8U dir, MY_CSD* strCAD, INT8U strDIList[10][4]) {
	fprintf(stderr, "\n CSDMap07DI--------start--------\n");
	printMY_CSD(*strCAD);

	INT8S result = 0;
	INT8U index;
	if (dir == 0) {
		for (index = 0; index < 5; index++) {
			if (strCAD->type == 0) {
				if (memcmp(&strCAD->csd.oad, &testArray[index].flag698.oad,
						sizeof(OAD)) == 0) {
					fprintf(stderr, "\n find CSDMap07DI OAD");
					memcpy(strDIList, testArray[index].flag07.DI_1, 40);
					return 1;
				}
			}
			if (strCAD->type == 1) {
				if (memcmp(&strCAD->csd.road, &testArray[index].flag698.road,
						sizeof(ROAD)) == 0) {
					fprintf(stderr, "\n find CSDMap07DI ROAD");
					memcpy(strDIList, testArray[index].flag07.DI_1, 40);
					return 1;
				}
			}

		}
	} else {

	}
	fprintf(stderr, "\n CSDMap07DI--------end--------\n");
	return result;
}
INT8S deal6015_07(CLASS_6015 st6015, BasicInfo6001 to6001) {
	INT8S result = 0;
	fprintf(stderr,
			"\n deal6015_07  meter = %d st6015.sernum = %d st6015.csds.num = %d",
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

	INT8U DIList[10][4];
	memset(DIList, 0, 40);
	INT8U dataContent[200] = {0};
	INT8U dataIndex = 0;
	INT16U dataLen = 0;
	for (dataIndex = 0; dataIndex < st6015.csds.num; dataIndex++) {
		if (CSDMap07DI(0, &st6015.csds.csd[dataIndex], DIList) == 1) {
			dataLen = request698_07DataList(DIList, to6001.addr,dataContent);
		} else {
			fprintf(stderr, "request698_07Data:1");
			continue;
		}
	}

	return result;
}
/*
 * 抄读1个测量点
 */
INT8U deal6015_singlemeter(CLASS_6015 st6015, BasicInfo6001 obj6001) {
	INT8S ret = 0;
	//打开串口
	if (obj6001.port == S4851) {
		comfd4851 = open_com_para_chg(obj6001.port, obj6001.baud, comfd4851);
		if (comfd4851 <= 0) {
			fprintf(stderr, "打开S4851串口失败\n");
			return ret;
		}
	}
	if (obj6001.port == S4852) {
		comfd4852 = open_com_para_chg(obj6001.port, obj6001.baud, comfd4852);
		if (comfd4852 <= 0) {
			fprintf(stderr, "打开S4852串口失败\n");
			return ret;
		}
	}

	switch (obj6001.protocol) {
#if 0
	case DLT_645_07:
	ret = deal6015_07(to6015,obj6001);
	break;
	default:
	ret = deal6015_698(to6015,obj6001);
#endif
	default:
		ret = deal6015_07(st6015, obj6001);
	}
	return ret;
}
/*
 *根据6015中的MS 和电表地址 和端口好判断此电表是否需要抄读
 *0-不抄 1-抄
 */
INT8U checkMeterType(MY_MS mst, INT8U port485, TSA meterAddr, OAD portOAD) {
	if ((portOAD.OI != 0xF201) || (portOAD.attflg != 0x02)
			|| (portOAD.attrindex != port485)) {
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
INT8U readList6001FromFile(BasicInfo6001* list6001, INT16U groupIndex,
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
					list6001[mIndex % LIST6001SIZE].sernum = meter.sernum;
					list6001[mIndex % LIST6001SIZE].port = port485;
					list6001[mIndex % LIST6001SIZE].baud = meter.basicinfo.baud;
					list6001[mIndex % LIST6001SIZE].protocol =
							meter.basicinfo.protocol;
					memcpy(&list6001[mIndex % LIST6001SIZE].addr,
							&meter.basicinfo.addr, sizeof(TSA));
					fprintf(stderr, "\n -------readList6001FromFile-------");
					fprintf(stderr, "\n序号:%d %02x%02x%02x%02x%02x%02x%02x%02x ",
							meter.sernum,
							list6001[mIndex % LIST6001SIZE].addr.addr[0],
							list6001[mIndex % LIST6001SIZE].addr.addr[1],
							list6001[mIndex % LIST6001SIZE].addr.addr[2],
							list6001[mIndex % LIST6001SIZE].addr.addr[3],
							list6001[mIndex % LIST6001SIZE].addr.addr[4],
							list6001[mIndex % LIST6001SIZE].addr.addr[5],
							list6001[mIndex % LIST6001SIZE].addr.addr[6],
							list6001[mIndex % LIST6001SIZE].addr.addr[7]);

				} else {
					fprintf(stderr, "\n序号:%d 非485测量点 %04X", meter.sernum,
							meter.basicinfo.port.OI);
				}
			}
		}
	}

	return result;
}

/*
 * 处理一个普通采集方案
 * */
INT8U deal6015(CLASS_6015 st6015, INT8U port485) {
	INT8U result = 0;
	BasicInfo6001 list6001[LIST6001SIZE];

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
		memset(list6001, 0, LIST6001SIZE * sizeof(BasicInfo6001));
		result = readList6001FromFile(list6001, groupindex, recordnum,
				st6015.mst, port485);

		for (mpIndex = 0; mpIndex < LIST6001SIZE; mpIndex++) {
			if (list6001[mpIndex].sernum > 0) {
				deal6015_singlemeter(st6015, list6001[mpIndex]);
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
	for (timePartIndex = 0; timePartIndex < 24; timePartIndex++) {
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
	tminc(&list6013[taskIndex].ts_next,
			list6013[taskIndex].basicInfo.interval.units,
			list6013[taskIndex].basicInfo.interval.interval);

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

	for (tIndex = 0; tIndex < TASK6012_MAX; tIndex++) {

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
		if ((taskIndex == -1) && (list6013[tIndex].run_flg > 0)) {
			fprintf(stderr, "\n  getNextTastIndexIndex-5555");
			taskIndex = tIndex;
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

		tastIndexIndex = getNextTastIndexIndex();
		//计算下一次抄读此任务的时间
		getTaskNextTime(tastIndexIndex);
		if (tastIndexIndex > -1) {
			fprintf(stderr, "\n-read485_thread tastIndexIndex = %d taskID = %d",
					tastIndexIndex, list6013[tastIndexIndex].basicInfo.taskID);
			CLASS_6035 result6035;	//采集任务监控单元
			memset(&result6035, 0, sizeof(CLASS_6035));
			CLASS_6015 to6015;	//采集方案集
			memset(&to6015, 0, sizeof(CLASS_6015));
			result6035.taskID = list6013[tastIndexIndex].basicInfo.taskID;
			result6035.taskState = IN_OPR;
			DataTimeGet(&result6035.starttime);
			switch (list6013[tastIndexIndex].basicInfo.cjtype) {
			case norm:/*普通采集方案*/
			{
				ret = use6013find6015(list6013[tastIndexIndex].basicInfo.sernum,
						&to6015);
				ret = deal6015(to6015, port);
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
		} else {
			fprintf(stderr, "\n 当前无任务可执行");
		}
		sleep(10);
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

void read485_proccess() {
	//读取所有任务文件
	init6013ListFrom6012File();

	INT8U i485port1 = 1;

	pthread_attr_init(&read485_attr_t);
	pthread_attr_setstacksize(&read485_attr_t, 2048 * 1024);
	pthread_attr_setdetachstate(&read485_attr_t, PTHREAD_CREATE_DETACHED);
	while ((thread_read4851_id = pthread_create(&thread_read4851,
			&read485_attr_t, (void*) read485_thread, &i485port1)) != 0) {
		sleep(1);
	}
	/*
	 * 	INT8U i485port2 = 2;
	 while ((thread_read4852_id=pthread_create(&thread_read4852, &read485_attr_t, (void*)read485_thread, &i485port2)) != 0)
	 {
	 sleep(1);
	 }
	 */

}

