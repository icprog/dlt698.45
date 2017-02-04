

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

/*
 * 根据测量点串口参数是否改变
 * 返回值：<=0：串口打开失败
 * 		  >0： 串口打开句柄
 * */
INT32S open_com_para_chg(INT8U port,INT32U baud,INT32S oldcomfd)
{
	INT32S newfd = 0;
	static INT8U lastport = 0;
	static INT32U lastbaud = 0;

	if ((lastbaud==baud) &&((lastport==port)))
	{
		return oldcomfd;
	}
	else
	{

	}
	if(oldcomfd>0)
	{
		CloseCom(oldcomfd);
		sleep(1);
	}
	newfd = OpenCom(port,baud,(unsigned char *)"even",1,8);

	return newfd;
}

/*
 * 根据6013任务配置单元去文件里查找对应的6015普通采集方案
 * 输入 st6013
 * 输出 st6015
*/
INT8U use6013find6015(INT16U taskID, CLASS_6015* st6015)
{
	INT8U result = 0;
	st6015->sernum = 1;
	st6015->deepsize = 0x100;
	st6015->cjtype = 1;
	st6015->csd[0].type = 1;
	st6015->csd[0].rcsd[0].oad.OI = 0x5004;
	st6015->csd[0].rcsd[0].oad.attflg = 2;
	st6015->csd[0].rcsd[0].oad.attrindex = 0;
	st6015->csd[0].rcsd[0].oad.OI = 0x2021;
	st6015->csd[0].rcsd[0].oad.attflg = 2;
	st6015->csd[0].rcsd[0].oad.attrindex = 0;
	st6015->csd[0].rcsd[1].oad.OI = 0x0010;
	st6015->csd[0].rcsd[1].oad.attflg = 2;
	st6015->csd[0].rcsd[1].oad.attrindex = 0;
	st6015->csd[0].rcsd[2].oad.OI = 0x0020;
	st6015->csd[0].rcsd[2].oad.attflg = 2;
	st6015->csd[0].rcsd[2].oad.attrindex = 0;
	st6015->ms.allmeter_null = 1;//所有电表
	st6015->savetimeflag = 4;
	return result;

}
/*
 * 485口接收处理,判断完整帧
 * 输入参数：delayms，超时等待时间，单位：毫秒
 * 输出；*str：接收缓冲区
 * 返回：>0：完整报文；=0:接收长度为0；-1：乱码，无完整报文
 */
INT16S ReceDataFrom485(INT32S fd,INT16U delayms,INT8U *str)
{
	INT8U 	TmprevBuf[256];//接收报文临时缓冲区
	INT8U 	prtstr[50];
	INT16U  len_Total=0,len,rec_step,rec_head,rec_tail,DataLen,i,j;

	if(fd<=2)
		return -1;

	memset(TmprevBuf,0,256);
	rec_head=rec_tail= rec_step = DataLen =0;
	fprintf(stderr, "delayms=%d, 111111111111111\n", delayms);
	usleep(delayms*1000);

	for(j=0;j<15;j++)
	{
		usleep(20000);	//20ms
		len = read(fd,TmprevBuf,256);

		if(len>0){
			len_Total+=len;
			if (len_Total > 256)
			{
				fprintf(stderr, "len_Total=%d, xxxxxxxxxxx\n", len_Total);
				return -1;
			}
			for(i=0;i<len;i++)
			{
				str[rec_head++]=TmprevBuf[i];
			}

			memset(prtstr, 0, sizeof(prtstr));
			sprintf((char *)prtstr, "485(%d)_R(%d):", 1, len);
//			dbg_prtbuff((char *)prtstr, TmprevBuf, len, "%02x", " ", "\n");

		}
		switch (rec_step) {
		case 0:
			if (rec_tail < rec_head)
			{
				for(i=rec_tail; i<rec_head; i++)
				{
					if(str[i] == 0x68)
					{//ma:判断第一个字符是否为0x68
						rec_step = 1;
						rec_tail = i;
						break;
					}else
						rec_tail++;
				}
			}
			break;
		case 1:
			if((rec_head - rec_tail)>=10)
			{
				if(str[rec_tail]==0x68 && str[rec_tail+7]==0x68 )
				{
					DataLen=str[rec_tail+9];//获取报文数据块长度
					rec_step = 2;
					break;
				}else
					rec_tail++;
			}
			break;
		case 2:
			if((rec_head - rec_tail)>=(DataLen+2))
			{
				if (str[rec_tail+9 +DataLen+2] == 0x16) {
//					DbPrt1("R:",(char *)str, rec_head, NULL);
					return rec_head;
				}
			}
			break;
		default:
			break;
		}
	}
	if (len_Total>0)
		return -1;
	else
		return 0;
}
/**
 * 485口发送
 */
void SendDataTo485(INT32S fd,INT8U *sendbuf,INT16U sendlen)
{
	ssize_t slen;

	INT8U str[50];
	memset(str, 0, 50);
	sprintf((char *)str, "485(%d)_S(%d):", 1, sendlen);
//	dbg_prtbuff((char *)str, sendbuf, sendlen, "%02x", " ", "\n");

	slen = write(fd,sendbuf,sendlen);
	if(slen<0)	fprintf(stderr, "slen=%d,send err!\n",slen);
//	DbPrt1("S:", (char *) sendbuf, sendlen, NULL);
}

INT8S deal6015_698(CLASS_6015 st6015,BasicInfo6001 to6001)
{
	INT8S result = -1;
	INT16S sendLen = 0;
	INT16S recvLen = 0;
	INT8U sendbuff[BUFFSIZE];
	INT8U recvbuff[BUFFSIZE];
	memset(sendbuff,0,BUFFSIZE);
	memset(recvbuff,0,BUFFSIZE);

	sendLen = composeProtocol698_GetRequest(sendbuff,st6015,to6001.addr);
	SendDataTo485(comfd4851, sendbuff, sendLen);
	recvLen = ReceDataFrom485(comfd4851, 500, recvbuff);



	return result;
}
INT8S deal6015_07(CLASS_6015 st6015,BasicInfo6001 to6001)
{
	INT8S result = 0;

	return result;
}
/*
 * 抄读1个测量点
*/
INT8U deal6015_singlemeter(CLASS_6015 st6015,BasicInfo6001 obj6001)
{
	INT8S ret = 0;
	//打开串口
	comfd4851 = open_com_para_chg(obj6001.port,obj6001.baud,comfd4851);
	if(comfd4851<=0)
	{
		fprintf(stderr,"打开串口失败\n");
		return ret;
	}
	switch(obj6001.protocol)
	{
		case DLT_645_07:
			ret = deal6015_07(to6015,obj6001);
			break;
		default:
			ret = deal6015_698(to6015,obj6001);
	}
	return ret;
}
/*
 * 从文件里读取LIST6001SIZE个测量点
 * */
INT8U readList6001FromFile(BasicInfo6001* list6001,INT16U groupIndex,int recordnum)
{
	INT16U		oi = 0x6001;
	INT8U result = 0;
	INT8U mIndex = 0;
	int endIndex;
	CLASS_6001	 meter={};
	if(((groupIndex+1)*LIST6001SIZE) > recordnum)
	{
		endIndex = recordnum;
	}
	else
	{
		endIndex = (groupIndex+1)*LIST6001SIZE;
	}


	for(mIndex = groupIndex*LIST6001SIZE;mIndex < endIndex;mIndex++)
	{
		if(readParaClass(oi,&meter,mIndex)==1)
		{
			if(meter.sernum!=0 && meter.sernum!=0xffff)
			{
				if(meter.basicinfo.port.OI == 0xF201)
				{
					fprintf(stderr,"\n序号:%d ",meter.sernum);

					list6001[mIndex%LIST6001SIZE].port = S4851;
					list6001[mIndex%LIST6001SIZE].baud = meter.basicinfo.baud;
					list6001[mIndex%LIST6001SIZE].protocol = meter.basicinfo.protocol;
					memcpy(&list6001[mIndex%LIST6001SIZE].addr,&meter.basicinfo.addr,sizeof(TSA));
					memcpy(&list6001[mIndex%LIST6001SIZE].addr,&meter.basicinfo.addr,sizeof(TSA));
				}
				else
				{
					fprintf(stderr,"非485测量点 %04X",meter.basicinfo.port.OI);
				}
			}
		}
	}

	return result;
}

/*
 * 处理一个普通采集方案
 * */
INT8U deal6015(CLASS_6015 st6015)
{
	INT8U result = 0;
	BasicInfo6001 list6001[LIST6001SIZE];
	int recordnum = 0;
	//全部电表
	if(st6015.ms.allmeter_null == 1)
	{
		INT16U		oi = 0x6001;
		recordnum = getFileRecordNum(oi);
		if(recordnum == -1)
		{
			fprintf(stderr,"未找到OI=%04x的相关信息配置内容！！！\n",6000);
			return result;
		}else if(recordnum == -2)
		{
			fprintf(stderr,"采集档案表不是整数，检查文件完整性！！！\n");
			return result;
		}

		/*
		 * 根据st6015.csd 和 list6001抄表
		 * */
		INT16U groupNum = (recordnum/LIST6001SIZE) + 1;
		INT16U groupindex;
		INT8U mpIndex;
		for(groupindex = 0;groupindex < groupNum;groupindex++)
		{
			memset(list6001,0,sizeof(list6001));
			result = readList6001FromFile(list6001,groupindex,recordnum);
			for(mpIndex = 0;mpIndex < LIST6001SIZE;mpIndex++)
			{
				deal6015_singlemeter(to6015,list6001[mpIndex]);
			}
		}


	}
	return result;
}

//void get_request(INT8U *cjtype,INT8U oad_num,INT8U piid,INT8U *oad,INT8U TP)
//{
//	int i=0;
//	INT16U sendindex=0;
//	INT8U sendbuf[512];
//
//	//报文头以后组 todo
//	sendbuf[sendindex++] = 0x05;
//	switch (cjtype[0])
//	{
//	case 0://实时数据
//		if(oad_num > 1)
//			sendbuf[sendindex++] = 0x01;//normal
//		else
//			sendbuf[sendindex++] = 0x02;//normallist
//		sendbuf[sendindex++] = (piid+1)%256;
//		if(oad_num > 1)
//			sendbuf[sendindex++] = oad_num;//piid后面为个数
//		for(i=0;i<oad_num;i++)
//		{
//			memcpy(&sendbuf[sendindex],&oad[i][0],4);//oad为二位数组
//			sendindex += 4;
//		}
//		break;
//	case 1://日冻结 采集上第N次
//		sendbuf[sendindex++] = 0x03;//record
//		sendbuf[sendindex++] = (piid+1)%256;
//		memcpy(&sendbuf[sendindex],&oad[i][0],4);//oad为二位数组,第0个为冻结类型
//		sendindex += 4;
//		sendbuf[sendindex++] = 0x01;//选择方法为RCSD
//		break;
//	default:break;
//	}
//	sendbuf[sendindex++] = TP;
//}


//时间在任务开始结束时间段内 0:任务开始 1：任务不执行
INT8U time_in_round(CLASS_6013 from6012_curr)
{
	struct tm tm_start;
	struct tm tm_end;
	struct tm tm_curr;
	if(from6012_curr.startime.year.data < 1900 || from6012_curr.startime.month.data < 1 ||
			from6012_curr.endtime.year.data < 1900 || from6012_curr.endtime.month.data < 1)
	{
		fprintf(stderr,"\n time_in_round - 1");
		return 1;//无效，任务不执行
	}

	memset(&tm_start,0x00,sizeof(struct tm));
	tm_start.tm_year = from6012_curr.startime.year.data -1900;
	tm_start.tm_mon = from6012_curr.startime.month.data -1;
	tm_start.tm_mday = from6012_curr.startime.day.data;
	tm_start.tm_hour = from6012_curr.startime.hour.data;
	tm_start.tm_min = from6012_curr.startime.min.data;
	tm_start.tm_sec = from6012_curr.startime.sec.data;



	memset(&tm_end,0x00,sizeof(struct tm));
	tm_end.tm_year = from6012_curr.endtime.year.data -1900;
	tm_end.tm_mon = from6012_curr.endtime.month.data -1;
	tm_end.tm_mday = from6012_curr.endtime.day.data;
	tm_end.tm_hour = from6012_curr.endtime.hour.data;
	tm_end.tm_min = from6012_curr.endtime.min.data;
	tm_end.tm_sec = from6012_curr.endtime.sec.data;


	time_t curr_time_t = time(NULL);
	localtime_r(&curr_time_t, &tm_curr);
#if 0
	fprintf(stderr,"\n start year = %d mon = %d day = %d hour=%d  min=%d",
				tm_start.tm_year,tm_start.tm_mon,tm_start.tm_mday,tm_start.tm_hour,tm_start.tm_min);
	fprintf(stderr,"\n end year = %d mon = %d day = %d hour=%d  min=%d",
			tm_end.tm_year,tm_end.tm_mon,tm_end.tm_mday,tm_end.tm_hour,tm_end.tm_min);
	fprintf(stderr,"\n curr year = %d mon = %d day = %d hour=%d  min=%d",
			tm_curr.tm_year,tm_curr.tm_mon,tm_curr.tm_mday,tm_curr.tm_hour,tm_curr.tm_min);
#endif
	if((tm_curr.tm_year >= tm_start.tm_year)&&(tm_curr.tm_year <= tm_end.tm_year))
	{
		if(tm_start.tm_year == tm_end.tm_year)
		{
			tm_start.tm_year = 0;
			tm_end.tm_year = 0;
			tm_curr.tm_year = 0;
			time_t currsec = mktime(&tm_curr);
			time_t startsec = mktime(&tm_start);
			time_t endsec = mktime(&tm_end);
			if((currsec>=startsec)&&(currsec<=endsec))
			{
				return 0;
			}
			else
			{
				return 1;
			}
		}
		else
		{
			return 0;
		}
	}
	else
	{
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
INT8U filterInvalidTask(INT16U taskIndex)
{
	TS ts_now;
	TSGet(&ts_now);

	INT16U min_start,min_end,now_min;//距离0点0分
	if(list6013[taskIndex].basicInfo.taskID == 0)
	{
		fprintf(stderr,"\n filterInvalidTask - 1");
		return 0;
	}


	if(list6013[taskIndex].basicInfo.state == task_novalid)//任务无效
	{
		fprintf(stderr,"\n filterInvalidTask - 2");
		return 0;
	}

	if(time_in_round(list6013[taskIndex].basicInfo)==1)//不在抄表时段内
	{
		fprintf(stderr,"\n filterInvalidTask - 3");
		return 0;
	}

	now_min = ts_now.Hour * 60 + ts_now.Minute;
	min_start = list6013[taskIndex].basicInfo.runtime.runtime[0] * 60 + list6013[taskIndex].basicInfo.runtime.runtime[1];//前秒数
	min_end = list6013[taskIndex].basicInfo.runtime.runtime[2] * 60 + list6013[taskIndex].basicInfo.runtime.runtime[3];//后秒数

	if(min_start >= min_end)//当日抄表时段无效
	{
		fprintf(stderr,"\n filterInvalidTask - 4");
		return 0;
	}
	if((list6013[taskIndex].basicInfo.runtime.type & 0x01) == 0x01)//后闭
	{
		if(now_min > min_end)
		{
			fprintf(stderr,"\n filterInvalidTask - 5");
			return 0;
		}
	}
	else
	{
		if(now_min >= min_end)
		{
			fprintf(stderr,"\n filterInvalidTask - 6");
			return 0;
		}
	}

	if((list6013[taskIndex].basicInfo.runtime.type & 0x03) == 0x01)//前闭
	{
		if(now_min < min_start)
		{
			fprintf(stderr,"\n filterInvalidTask - 7");
			return 0;
		}
	}
	else
	{
		if(now_min <= min_start)
		{
			fprintf(stderr,"\n filterInvalidTask - 8");
			return 0;
		}
	}
	return 1;
}
/*
 * 根据抄表时间断和抄表间隔
 * 计算当前时间应该抄第几轮
 * runtime 现在默认就一个时段
 *
 * */
int getTaskReadRounds(INT16U taskIndex)
{
	int readRound = 0;
	TS ts_now;
	TSGet(&ts_now);

	switch(list6013[taskIndex].basicInfo.interval.units)//计算本抄表时段唯一标识值
	{
		case year_units:
			if(ts_now.Year != list6013[taskIndex].ts_last.Year)
			{
				readRound += 1;
			}
			break;
		case month_units:
			if(ts_now.Month != list6013[taskIndex].ts_last.Month)
			{
				readRound += 1;
			}
			break;
		case day_units:
			if(ts_now.Day != list6013[taskIndex].ts_last.Day)
			{
				readRound += 1;
			}
			break;
		case hour_units://小时开始，标识不是唯一，不同天会产生循环的数，因此需要加上年月日判断
			//小时与上一次抄表时间的day结合使用,每天重新生成唯一标识
			if(ts_now.Year != list6013[taskIndex].ts_last.Year || ts_now.Month != list6013[taskIndex].ts_last.Month || ts_now.Day != list6013[taskIndex].ts_last.Day)
			{
				readRound = 1;
				list6013[taskIndex].resetFlag = 1;
			}
			else
				readRound = 1 + abs(ts_now.Hour - list6013[taskIndex].basicInfo.runtime.runtime[0])/list6013[taskIndex].basicInfo.interval.interval;
			break;
		case minute_units:
			if(ts_now.Year != list6013[taskIndex].ts_last.Year || ts_now.Month != list6013[taskIndex].ts_last.Month || ts_now.Day != list6013[taskIndex].ts_last.Day)
			{
				readRound = 1;
				list6013[taskIndex].resetFlag = 1;
			}
			else
			{
				INT16U minSpan = abs((ts_now.Hour*60 + ts_now.Minute) - (list6013[taskIndex].basicInfo.runtime.runtime[0]*60 + list6013[taskIndex].basicInfo.runtime.runtime[1]));
				readRound =  1 + minSpan /list6013[taskIndex].basicInfo.interval.interval;
			}
			break;
		case sec_units:
			if(ts_now.Year != list6013[taskIndex].ts_last.Year || ts_now.Month != list6013[taskIndex].ts_last.Month || ts_now.Day != list6013[taskIndex].ts_last.Day)
			{
				readRound = 1;
				list6013[taskIndex].resetFlag = 1;
			}
			else
			{
				INT16U secSpan = abs((ts_now.Hour*3600 + ts_now.Minute*60 + ts_now.Sec) -
									(list6013[taskIndex].basicInfo.runtime.runtime[0]*3600
											+ list6013[taskIndex].basicInfo.runtime.runtime[1]*60));
				readRound =  1 + secSpan /list6013[taskIndex].basicInfo.interval.interval;
			}

			break;
		default:
			break;
	}

	return readRound;
}
/*
 * 比较当前时间应该先抄读哪一个任务
 * 返回
 * ：0-优先级一样
 * ：1-taskIndex1先执行
 * ：2-taskIndex2先执行
 * */
INT8U cmpTaskPrio(INT16U taskIndex1,INT16U taskIndex2)
{
	if(list6013[taskIndex1].basicInfo.interval.units > list6013[taskIndex2].basicInfo.interval.units)
	{
		return 2;
	}
	if(list6013[taskIndex1].basicInfo.interval.units < list6013[taskIndex2].basicInfo.interval.units)
	{
		return 1;
	}
	if((getTaskReadRounds(taskIndex1) - list6013[taskIndex1].last_round) >
	(getTaskReadRounds(taskIndex2) - list6013[taskIndex2].last_round))
	{
		return 1;
	}
	if((getTaskReadRounds(taskIndex1) - list6013[taskIndex1].last_round) <
	(getTaskReadRounds(taskIndex2) - list6013[taskIndex2].last_round))
	{
		return 2;
	}
	return 0;
}
INT16S getNextTastID()
{
	INT16S taskIndex = -1;
	INT16U tIndex = 0;


	for(tIndex=0;tIndex<TASK6012_MAX;tIndex++)
	{
		if(list6013[tIndex].basicInfo.taskID == 0)
			continue;

		fprintf(stderr,"\n -----------------getNextTastID tIndex = %d-----------------------",tIndex);
		//过滤任务无效或者不再抄表时段内的
		if (filterInvalidTask(tIndex)==0)
		{
			fprintf(stderr,"\n filterInvalidTask");
			continue;
		}
		//过滤没到抄表间隔的
		if((list6013[tIndex].last_round == getTaskReadRounds(tIndex))
			&&(list6013[tIndex].resetFlag!=1))
		{
			continue;
		}
		if(taskIndex == -1)
		{
			taskIndex = tIndex;
			continue;
		}
		else if(list6013[taskIndex].basicInfo.runprio < list6013[tIndex].basicInfo.runprio)
		{
			//判断优先级
			taskIndex = tIndex;
			continue;
		}
		else
		{
			if(cmpTaskPrio(taskIndex,tIndex) == 2)
			{
				taskIndex = tIndex;
				continue;
			}
		}

	}

	return list6013[taskIndex].basicInfo.taskID;
}
typedef enum{
	coll_bps=1,
	coll_protocol,
	coll_wiretype,
	task_ti,
	task_cjtype,
	task_prio,
	task_status,
	task_runtime
}OBJ_ENUM;
char *getenum(int type,int val)
{
	char name1[128]={};
	char *name=NULL;

	name = name1;
	memset(name1,0,sizeof(name1));
//	fprintf(stderr,"val=%d ,type=%d\n",val,type);
	switch(type) {
	case coll_bps:
		if(val==bps300)	strcpy(name,"300");
		if(val==bps600)	strcpy(name,"600");
		if(val==bps1200)	strcpy(name,"1200");
		if(val==bps2400)	strcpy(name,"2400");
		if(val==bps4800)	strcpy(name,"4800");
		if(val==bps7200)	strcpy(name,"7200");
		if(val==bps9600)	strcpy(name,"9600");
		if(val==bps19200)	strcpy(name,"19200");
		if(val==bps38400)	strcpy(name,"38400");
		if(val==bps57600)	strcpy(name,"57600");
		if(val==bps115200)	strcpy(name,"115200");
		if(val==autoa)		strcpy(name,"自适应");
		break;
	case coll_protocol:
		if(val==0)	strcpy(name,"未知");
		if(val==1)	strcpy(name,"DL/T645-1997");
		if(val==2)	strcpy(name,"DL/T645-2007");
		if(val==3)	strcpy(name,"DL/T698.45");
		if(val==4)	strcpy(name,"CJ/T18802004");
		break;
	case coll_wiretype:
		if(val==0)	strcpy(name,"未知");
		if(val==1)	strcpy(name,"单相");
		if(val==2)	strcpy(name,"三相三线");
		if(val==3)	strcpy(name,"三相四线");
		break;
	case task_ti:
		if(val==0)	strcpy(name,"秒");
		if(val==1)	strcpy(name,"分");
		if(val==2)	strcpy(name,"时");
		if(val==3)	strcpy(name,"日");
		if(val==4)	strcpy(name,"月");
		if(val==5)	strcpy(name,"年");
		break;
	case task_cjtype:
		if(val==1)	strcpy(name,"普通采集方案");
		if(val==2)	strcpy(name,"事件采集方案");
		if(val==3)	strcpy(name,"透明方案");
		if(val==4)	strcpy(name,"上报方案");
		if(val==5)	strcpy(name,"脚本方案");
		break;
	case task_prio:
		if(val==1)	strcpy(name,"首要");
		if(val==2)	strcpy(name,"必要");
		if(val==3)	strcpy(name,"需要");
		if(val==4)	strcpy(name,"可能");
		break;
	case task_status:
		if(val==1)	strcpy(name,"正常");
		if(val==2)	strcpy(name,"停用");
		break;
	case task_runtime:
		if(val==0)	strcpy(name,"前闭后开");
		if(val==1)	strcpy(name,"前开后闭");
		if(val==2)	strcpy(name,"前闭后闭");
		if(val==3)	strcpy(name,"前开后开");
		break;
	}
//	fprintf(stderr,"get name=%s\n",name);
	return name;
}

void print6013(CLASS_6013 class6013)
{
	fprintf(stderr,"\n----------------------------------");
	fprintf(stderr,"【6013】任务配置单元: 任务ID--%04x\n",class6013.taskID);
	fprintf(stderr,"[1]执行频率 [2]方案类型 [3]方案编号 [4]开始时间 [5]结束时间 [6]延时 [7]执行优先级 [8]状态 [9]开始前脚本id [10]开始后脚本id [11]运行时段【起始HH:MM 结束HH:MM】\n");
	fprintf(stderr,"[1]%s-%d ",getenum(task_ti,class6013.interval.units),class6013.interval.interval);
	fprintf(stderr,"[2]%s  [3]%d   ",getenum(task_cjtype,class6013.cjtype),class6013.sernum);
	fprintf(stderr,"[4]%d-%d-%d %d:%d:%d ",class6013.startime.year.data,class6013.startime.month.data,class6013.startime.day.data,
			class6013.startime.hour.data,class6013.startime.min.data,class6013.startime.sec.data);
	fprintf(stderr,"[5]%d-%d-%d %d:%d:%d ",class6013.endtime.year.data,class6013.endtime.month.data,class6013.endtime.day.data,
			class6013.endtime.hour.data,class6013.endtime.min.data,class6013.endtime.sec.data);
	fprintf(stderr,"[6]%s-%d ",getenum(task_ti,class6013.delay.units),class6013.delay.interval);
	fprintf(stderr,"[7]%s  ",getenum(task_prio,class6013.runprio));
	fprintf(stderr,"[8]%s  [9]%d  [10]%d ",getenum(task_status,class6013.state),class6013.befscript,class6013.aftscript);
	fprintf(stderr,"[11]%s [%d:%d %d:%d] ",getenum(task_runtime,class6013.runtime.type),class6013.runtime.runtime[0],class6013.runtime.runtime[1],class6013.runtime.runtime[2],class6013.runtime.runtime[3]);
	fprintf(stderr,"\n");
}


/*
 * 从文件里把所有的任务单元读上来
 * */
INT8U init6013ListFrom6012File()
{
	//list6013  初始化上一次抄表时间  和 抄表轮次
	TS ts_now;
	TSGet(&ts_now);


	fprintf(stderr,"\n -------------init6013ListFrom6012File---------------");
	INT8U result = 0;
	memset(list6013,0x00,TASK6012_MAX*sizeof(TASK_CFG));

	INT16U tIndex = 0;
	OI_698	oi=0x6013;
	CLASS_6013	class6013={};
	for(tIndex = 0;tIndex < TASK6012_MAX;tIndex++)
	{
		if(readCoverClass(oi,tIndex+1,&class6013,coll_para_save)== 1)
		{
			memcpy(&list6013[tIndex].basicInfo,&class6013,sizeof(CLASS_6013));
			list6013[tIndex].ts_last.Year = ts_now.Year;
			list6013[tIndex].ts_last.Month = ts_now.Month;
			list6013[tIndex].ts_last.Day = ts_now.Day;
			list6013[tIndex].ts_last.Hour = ts_now.Hour;
			list6013[tIndex].ts_last.Minute = ts_now.Minute;
			list6013[tIndex].last_round = 0;
			print6013(list6013[tIndex].basicInfo);
		}
	}

	return result;
}
void read485_thread(void* i485port)
{
	INT8U port = *(INT8U*)i485port;
	fprintf(stderr,"\n port = %d",port);
	comfd4851 = -1;
	INT8U ret = 0;
	INT16S taskID = -1;

	while(1)
	{

		taskID = getNextTastID();

		if(taskID > -1)
		{
			fprintf(stderr,"\n------------------- taskID = %d",taskID);
			CLASS_6035 result6035;//采集任务监控单元
			memset(&result6035,0,sizeof(CLASS_6035));
			result6035.taskID = taskID;
			result6035.taskState = IN_OPR;
			DataTimeGet(&result6035.starttime);

			ret = use6013find6015(taskID,&to6015);
			ret = deal6015(to6015);
			DataTimeGet(&result6035.endtime);
			result6035.taskState = AFTER_OPR;

		}
		else
		{
			fprintf(stderr,"\n 当前无任务可执行");
		}
		sleep(3);
	}

	pthread_detach(pthread_self());
	if(port==1)
	{
		pthread_exit(&thread_read4851);
	}

	if(port==2)
	{
		pthread_exit(&thread_read4852);
	}

	sleep(1);


}
void read485_proccess()
{
	init6013ListFrom6012File();
	fprintf(stderr,"\n init6013ListFrom6012File end");


	INT8U i485port1 = 1;
	//INT8U i485port2 = 2;
	pthread_attr_init(&read485_attr_t);
	pthread_attr_setstacksize(&read485_attr_t,2048*1024);
	pthread_attr_setdetachstate(&read485_attr_t,PTHREAD_CREATE_DETACHED);
	while ((thread_read4851_id=pthread_create(&thread_read4851, &read485_attr_t, (void*)read485_thread, &i485port1)) != 0)
	{
		sleep(1);
	}
#if 0
	while ((thread_read4852_id=pthread_create(&thread_read4852, &read485_attr_t, (void*)read485_thread, &i485port2)) != 0)
	{
		sleep(1);
	}
#endif
}

