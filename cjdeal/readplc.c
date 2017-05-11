/*
 * readplc.c
 *
 *  Created on: 2017-1-4
 *      Author: wzm
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>
#include "sys/reboot.h"
#include <wait.h>
#include <errno.h>
#include <unistd.h>
#include "time.h"
#include <sys/time.h>
#include "lib3762.h"
#include "readplc.h"
#include "cjdeal.h"
#include "dlt645.h"
#include "dlt698.h"
extern ProgramInfo* JProgramInfo;
extern int SaveOADData(INT8U taskid,OAD oad_m,OAD oad_r,INT8U *databuf,int datalen,TS ts_res);
extern INT16U data07Tobuff698(FORMAT07 Data07,INT8U* dataContent);
extern INT8S analyzeProtocol07(FORMAT07* format07, INT8U* recvBuf, const INT16U recvLen, BOOLEAN *nextFlag);
extern INT8S OADMap07DI(OI_698 roadOI,OAD sourceOAD, C601F_07Flag* obj601F_07Flag);
extern void DbgPrintToFile1(INT8U comport,const char *format,...);
extern void DbPrt1(INT8U comport,char *prefix, char *buf, int len, char *suffix);
extern  INT16U data07Tobuff698(FORMAT07 Data07,INT8U* dataContent);
extern mqd_t mqd_zb_task;
extern CJCOMM_PROXY cjcommProxy;
extern GUI_PROXY cjguiProxy;
extern Proxy_Msg* p_Proxy_Msg_Data;//液晶给抄表发送代理处理结构体，指向由guictrl.c配置的全局变量
extern TASK_CFG list6013[TASK6012_MAX];
//-----------------------------------------------------------------------------------------------------------------------------------------------------
void SendDataToCom(int fd, INT8U *sendbuf, INT16U sendlen)
{
	int i=0;
	ssize_t slen;
	slen = write(fd, sendbuf, sendlen);
//	fprintf(stderr,"\nzb_send(%d): ",sendlen);
//	for(i=0;i<sendlen; i++)
//		fprintf(stderr,"%02x ",sendbuf[i]);
	DbPrt1(31,"发:", (char *) sendbuf, slen, NULL);
}
int RecvDataFromCom(int fd,INT8U* buf,int* head)
{
	int len,i;
	INT8U TmpBuf[ZBBUFSIZE];
	memset(TmpBuf,0,ZBBUFSIZE);
	if (fd < 0 ) return 0;
	len = read(fd,TmpBuf,ZBBUFSIZE);
	if (len>0)
	{
		fprintf(stderr,"\nzb_recv(%d): ",len);
	}

	for(i=0;i<len;i++)
	{
		buf[*head]=TmpBuf[i];
		fprintf(stderr,"%02x ",TmpBuf[i]);
		*head = (*head + 1) % ZBBUFSIZE;
	}
	return len;
}

int StateProcessZb(unsigned char *str,INT8U* Buf )
{
	int i;
	INT16U tmptail=0;

	switch (rec_step)
	{
	case 0:
		while (RecvTail != RecvHead)
		{
			if(Buf[RecvTail] == 0x68)
			{
				rec_step = 1;
				break;
			}else {
				RecvTail = (RecvTail+1)%ZBBUFSIZE;
			}
		}
		break;
	case 1:
		if(((RecvHead - RecvTail+ZBBUFSIZE)%ZBBUFSIZE)>=3)
		{
			tmptail=RecvTail;
			tmptail = (tmptail+1)%ZBBUFSIZE;
			DataLen = Buf[tmptail];
			tmptail = (tmptail+1)%ZBBUFSIZE;
			DataLen |= (Buf[tmptail]<<8);
			if (DataLen !=0)
			{
				rec_step = 2;
				oldtime1 = time(NULL);
			}else
			{
				RecvTail = (RecvTail+1)%ZBBUFSIZE;
				rec_step = 0;
			}
			break;
		}
		break;
	case 2:
		if(((RecvHead - RecvTail+ZBBUFSIZE)%ZBBUFSIZE)>=DataLen)
		{
			if (Buf[(RecvTail+DataLen-1)%ZBBUFSIZE] == 0x16)
			{
				//fprintf(stderr,"\nReceiveFromCarr OK: len = %d\n",DataLen);
				for(i=0; i<DataLen; i++)
				{
					str[i] = Buf[RecvTail];
					RecvTail = (RecvTail+1)%ZBBUFSIZE;
				}
				rec_step = 0;
				DbPrt1(31,"收:", (char *) str, DataLen, NULL);
				return DataLen;
			}else {
				RecvTail = (RecvTail+1)%ZBBUFSIZE;
				rec_step = 0;
			}
		}else
		{
			newtime1 = time(NULL);
			if ((newtime1-oldtime1)> 2)
			{
				RecvTail = (RecvTail+1)%ZBBUFSIZE;
				rec_step = 0;
			}
		}
		break;
	default:
		break;
	}
	return (0);
}
/*
 * n: bit位  n(0..7)
 */
INT8U judgebit(INT8U Byte,int n)
{
	if (n <= 7)
	{
		INT8U tmp = (Byte >> n) & 0x01;
		if (tmp == 1)
			return 1;
	}
	return 0;
}
TS DateBCD2Ts(DateTimeBCD timebcd)
{
	TS ts;
	ts.Year = timebcd.year.data;
	ts.Month = timebcd.month.data;
	ts.Day = timebcd.day.data;
	ts.Hour = timebcd.hour.data;
	ts.Minute = timebcd.min.data;
	ts.Sec = timebcd.sec.data;
	ts.Week = 1;
	fprintf(stderr,"\n111111 ts.Year %d-%d-%d %d:%d:%d",ts.Year,ts.Month,ts.Day,ts.Hour,ts.Minute,ts.Sec);

	return ts;
}

void PrintTaskInfo2(TASK_INFO *task)
{
	struct tm ctm;
	int i=0,j=0,numindex=0;
	time_t t;
	for(i=0;i<task->task_n;i++)
	{
		for(j=0;j<task->task_list[i].fangan.item_n;j++)
		{
			numindex++;
			t = task->task_list[i].beginTime;
			localtime_r(&t, &ctm);
			DbgPrintToFile1(31," %02d | %04x-%02x%02x - %04x-%02x%02x 【任务%d ， 优先级%d    方案%d ， 类型%d】  开始 %d-%d-%d %d:%d:%d  结束 %d-%d-%d %d:%d:%d  OK=%d  item %02x%02x%02x%02x",numindex,
					task->task_list[i].fangan.items[j].oad1.OI,task->task_list[i].fangan.items[j].oad1.attflg,task->task_list[i].fangan.items[j].oad1.attrindex,
					task->task_list[i].fangan.items[j].oad2.OI,task->task_list[i].fangan.items[j].oad2.attflg,task->task_list[i].fangan.items[j].oad2.attrindex,
					task->task_list[i].taskId,task->task_list[i].leve,task->task_list[i].fangan.No,task->task_list[i].fangan.type,
					task->task_list[i].begin.year.data,task->task_list[i].begin.month.data,task->task_list[i].begin.day.data,
					task->task_list[i].begin.hour.data,task->task_list[i].begin.min.data,task->task_list[i].begin.sec.data,
					task->task_list[i].end.year.data,task->task_list[i].end.month.data,task->task_list[i].end.day.data,
					task->task_list[i].end.hour.data,task->task_list[i].end.min.data,task->task_list[i].end.sec.data,
					task->task_list[i].fangan.items[j].sucessflg,
					task->task_list[i].fangan.items[j].item07[3],task->task_list[i].fangan.items[j].item07[2],
					task->task_list[i].fangan.items[j].item07[1],task->task_list[i].fangan.items[j].item07[0]
					);
		}
	}
}
int Array_OAD_Items(CJ_FANGAN *fangAn)
{
	int i=0,j=0,num=0, oadtype=0;
	CLASS_6015 Class6015;
	CLASS_6017 Class6017;

	if(fangAn->type == norm)
	{
		readCoverClass(0x6015, fangAn->No, (void *)&Class6015, sizeof(CLASS_6015), coll_para_save);
		DbgPrintToFile1(31,"普通采集方案  csds.num %d",Class6015.csds.num);
		for(i=0;i<Class6015.csds.num;i++)//全部CSD循环
		{
			oadtype = Class6015.csds.csd[i].type;
			if(oadtype == 1)//ROAD
			{
				for(j=0;j<Class6015.csds.csd[i].csd.road.num;j++)
				{
					fangAn->items[num].oad1 = Class6015.csds.csd[i].csd.road.oad;
					fangAn->items[num].oad2 = Class6015.csds.csd[i].csd.road.oads[j];
					num++;
					if (num >= FANGAN_ITEM_MAX ) return num;
				}
			}else			//OAD
			{
				fangAn->items[num].oad1.OI = 0;
				fangAn->items[num].oad2 = Class6015.csds.csd[i].csd.oad;
				num++;
				if (num >= FANGAN_ITEM_MAX ) return num;
			}
		}
		fprintf(stderr,"\n普通采集方案 %d [csd] ",Class6015.csds.num);
	}
	if(fangAn->type == events)
	{
		readCoverClass(0x6017, fangAn->No, (void *)&Class6017, sizeof(CLASS_6017), coll_para_save);
		DbgPrintToFile1(31,"事件方案6017 road num=%d",Class6017.collstyle.roads.num);
		for(i=0; i< Class6017.collstyle.roads.num; i++)
		{
			DbgPrintToFile1(31,"事件方案6017 roads %d 的 num=%d",i,Class6017.collstyle.roads.road[i].num);
			for(j=0;j<Class6017.collstyle.roads.road[i].num;j++)
			{
				fangAn->items[num].oad1 = Class6017.collstyle.roads.road[i].oad;
				fangAn->items[num].oad2 = Class6017.collstyle.roads.road[i].oads[j];
				num++;
				if (num >= FANGAN_ITEM_MAX ) return num;
			}
		}
		fprintf(stderr,"\n事件采集方案 %d [ROAD]",Class6017.collstyle.roads.num);
	}
	fangAn->item_n = num;

//	if (num>0)
//	{
//		DbgPrintToFile1(31,"-----------全部数据项 %d 个-----------",num);
//		for(i=0;i<num;i++)
//		{
//			DbgPrintToFile1(31,"[ %d ] %04x - %04x ",i,fangAn->items[i].oad1.OI,fangAn->items[i].oad2.OI);
//		}
//		fprintf(stderr,"\n\n\n");
//	}
	return num;
}
int task_leve(INT8U leve,TASK_UNIT *taskunit)
{
	int i=0,t=0;
	INT8U type=0 ,serNo=0;
	for(i=0;i<TASK6012_MAX;i++)
	{
		if (list6013[i].basicInfo.runprio == leve)
		{
			taskunit[t].taskId = list6013[i].basicInfo.taskID;
			taskunit[t].leve = list6013[i].basicInfo.runprio;
			taskunit[t].beginTime = calcnexttime(list6013[i].basicInfo.interval,list6013[i].basicInfo.startime,list6013[i].basicInfo.delay);;
			taskunit[t].endTime = tmtotime_t( DateBCD2Ts(list6013[i].basicInfo.endtime ));
			taskunit[t].begin = list6013[i].basicInfo.startime;
			taskunit[t].end = list6013[i].basicInfo.endtime;
			type = list6013[i].basicInfo.cjtype;
			serNo = list6013[i].basicInfo.sernum;//方案序号
			memset(&taskunit[t].fangan,0,sizeof(CJ_FANGAN));
			taskunit[t].fangan.No = serNo;
			taskunit[t].fangan.type = type;
			taskunit[t].fangan.item_n = Array_OAD_Items(&taskunit[t].fangan);
			t++;
		}
	}
	return t;
}

int initTaskData(TASK_INFO *task)
{
	int num =0;
	if (task==NULL)
		return 0;

	memset(task,0,sizeof(TASK_INFO));
	num += task_leve(1,&task->task_list[num]);
	num += task_leve(2,&task->task_list[num]);
	num += task_leve(3,&task->task_list[num]);
	num += task_leve(4,&task->task_list[num]);
	task->task_n = num;
	return 1;
}
int initTsaList(struct Tsa_Node **head)
{
	int i=0, record_num=0 ,n=0;
	CLASS_6001	 meter={};
	struct Tsa_Node *p=NULL;
	struct Tsa_Node *tmp=NULL;

	record_num = getFileRecordNum(0x6000);
	for(i=0;i<record_num;i++)
	{
		if(readParaClass(0x6000,&meter,i)==1)
		{
			if (meter.sernum!=0 && meter.sernum!=0xffff && meter.basicinfo.port.OI==0xf209)
			{
				tmp = (struct Tsa_Node *)malloc(sizeof(struct Tsa_Node));
				memcpy(&tmp->tsa , &meter.basicinfo.addr,sizeof(TSA));
				tmp->protocol = meter.basicinfo.protocol;
				tmp->usrtype = meter.basicinfo.usrtype;
				tmp->readnum = 0;
				tmp->tsa_index = meter.sernum;
				memset(tmp->flag,0,sizeof(tmp->flag));
				if (tmp!=NULL)
				{
					if (n==0)
					{
						p  = tmp;
						*head = p;
					}else
					{
						p->next = tmp;
						p = p->next;
					}
					n++;
				}
			}
		}
	}
	if (p!=NULL)
	{
		p->next = NULL;
	}
	return n;
}


void reset_ZB()
{
	gpio_writebyte((char *)"/dev/gpoZAIBO_RST", 0);
	usleep(500*1000);
//	sleep(5);
	gpio_writebyte((char *)"/dev/gpoZAIBO_RST", 1);
	sleep(10);

}
void tsa_print(struct Tsa_Node *head,int num)
{
	int j = 0,i=0;
	struct Tsa_Node *tmp = NULL;
	tmp = head;
	fprintf(stderr,"\ntmp = %p",tmp);
	while(tmp!=NULL)
	{
		fprintf(stderr,"\nTSA%d: %d-",i,tmp->tsa.addr[0]);
		for(j=0;j<tmp->tsa.addr[0];j++)
		{
			fprintf(stderr,"-%02x",tmp->tsa.addr[j+1]);
		}
		fprintf(stderr,"\nFLAG: %02x %02x %02x %02x %02x %02x %02x %02x\n",tmp->flag[0],tmp->flag[1],tmp->flag[2],tmp->flag[3],tmp->flag[4],tmp->flag[5],tmp->flag[6],tmp->flag[7]);
		fprintf(stderr,"\nProtocol = %d  curr_i=%d\n\n",tmp->protocol,tmp->curr_i);
		tmp = tmp->next;
		i++;
	}
}
TSA getNextTsa(struct Tsa_Node **p)
{
	TSA tsatmp;
	memset(&tsatmp,0,sizeof(TSA));
	if (*p!=NULL)
	{
		tsatmp = (*p)->tsa;
		*p = (*p)->next;
	}
	return tsatmp;
}
//在档案中查找指定TSA
int findTsaInList(struct Tsa_Node *head,struct Tsa_Node *desnode)
{
	struct Tsa_Node *p=NULL;
	p = head;
	while(p!=NULL && desnode!=NULL)
	{
		if(memcmp(&(p->tsa.addr[2]),&(desnode->tsa.addr[2]), desnode->tsa.addr[1]+1)==0)
		{
			desnode->readnum = p->readnum;
			desnode->protocol = p->protocol;
			desnode->usrtype = p->usrtype;
			return 1;
		}
		else
			p = p->next;
	}
	return 0;
}
struct Tsa_Node *getNodeByTSA(struct Tsa_Node *head,TSA tsa)
{
	struct Tsa_Node *p=NULL;
	p = head;
	while(p!=NULL )
	{
		if(memcmp(&(p->tsa.addr[2]),&tsa.addr[2], tsa.addr[1]+1)==0)
		{
			return p;
		}
		else
			p = p->next;
	}
	return NULL;
}
void freeList(struct Tsa_Node *head)
{
	struct Tsa_Node *tmp;
	while(head!=NULL)
	{
		tmp = head;
		head = head->next;
		free(tmp);
	}
	tmp = NULL;
	return;
}
/**********************************************************************
 * 将表地址添加到链表尾   addr为标准6字节表地址 非TSA格式，赋值时将TSA格式补充完整
 *********************************************************************/
void addTsaList(struct Tsa_Node **head,INT8U *addr)
{
	struct Tsa_Node *tmp = *head;
	struct Tsa_Node *new;
	tsa_zb_count = 0;
	if (*head==NULL)
	{
		*head = (struct Tsa_Node *)malloc(sizeof(struct Tsa_Node));
		(*head)->tsa.addr[0] = 7;
		(*head)->tsa.addr[1] = 5;
//		memcpy(&(*head)->tsa.addr[2] , addr,6);
		(*head)->tsa.addr[2] = addr[5];
		(*head)->tsa.addr[3] = addr[4];
		(*head)->tsa.addr[4] = addr[3];
		(*head)->tsa.addr[5] = addr[2];
		(*head)->tsa.addr[6] = addr[1];
		(*head)->tsa.addr[7] = addr[0];

		(*head)->next = NULL;
		tsa_zb_count = 1;
	}else
	{
		while(tmp->next != NULL)
		{
			tmp = tmp->next;
			tsa_zb_count++;
		}
		new = (struct Tsa_Node *)malloc(sizeof(struct Tsa_Node));
		new->tsa.addr[0] = 7;
		new->tsa.addr[1] = 5;
//		memcpy(&(new->tsa.addr[2]) , addr,6);//TODO: TSA长度未赋值
		new->tsa.addr[2] = addr[5];
		new->tsa.addr[3] = addr[4];
		new->tsa.addr[4] = addr[3];
		new->tsa.addr[5] = addr[2];
		new->tsa.addr[6] = addr[1];
		new->tsa.addr[7] = addr[0];
		new->next = NULL;
		tmp->next = new;
		tsa_zb_count++;
	}
	return;
}
void printModelinfo(AFN03_F10_UP info)
{
	DbgPrintToFile1(31,"硬件复位");
	DbgPrintToFile1(31,"\n\n--------------------------------\n\n厂商信息:%c%c%c%c \nDate:%d-%d-%d \nVersion:%d%d",
			info.ModuleInfo.VendorCode[1],
			info.ModuleInfo.VendorCode[0],
			info.ModuleInfo.ChipCode[1],
			info.ModuleInfo.ChipCode[0],
			info.ModuleInfo.VersionYear,
			info.ModuleInfo.VersionMonth,
			info.ModuleInfo.VersionDay,
			info.ModuleInfo.Version[1],
			info.ModuleInfo.Version[0]);
	DbgPrintToFile1(31,"\n主节点地址:%02x%02x%02x%02x%02x%02x",
			info.MasterPointAddr[5],
			info.MasterPointAddr[4],
			info.MasterPointAddr[3],
			info.MasterPointAddr[2],
			info.MasterPointAddr[1],
			info.MasterPointAddr[0]);
	DbgPrintToFile1(31,"\nMonitorOverTime=%d 秒", info.MonitorOverTime);
	DbgPrintToFile1(31,"\nReadMode=%02x\n--------------------------------\n\n", info.ReadMode);

}
void clearvar(RUNTIME_PLC *runtime_p)
{
	runtime_p->send_start_time = 0;
	memset(&runtime_p->format_Up,0,sizeof(runtime_p->format_Up));
}
int doInit(RUNTIME_PLC *runtime_p)
{
	static int step_init = 0;
	int sendlen=0;
	time_t nowtime = time(NULL);
	if (runtime_p->initflag == 1)
		step_init = 0;
	switch(step_init )
	{
		case 0://初始化
			DbgPrintToFile1(31,"硬件复位");
			freeList(tsa_head);
			freeList(tsa_zb_head);
			tsa_head = NULL;
			tsa_zb_head = NULL;
			reset_ZB();
			tsa_count = initTsaList(&tsa_head);
			tsa_print(tsa_head,tsa_count);
			initTaskData(&taskinfo);
			system("rm /nand/para/plcrecord.par  /nand/para/plcrecord.bak");
			if (runtime_p->comfd >0)
				CloseCom( runtime_p->comfd );
			runtime_p->comfd = OpenCom(5, 9600,(unsigned char*)"even",1,8);
			runtime_p->initflag = 0;
			clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
			fprintf(stderr,"\n-----------tsacount=%d",tsa_count);
			if (tsa_count <= 0)
			{
				DbgPrintToFile1(31,"无载波测量点");
				step_init = 0;
				return NONE_PROCE;
			}
			step_init = 1;
			break;

		case 1://读取载波信息
			fprintf(stderr,"\n读取信息状态 nowtime = %ld  runtime_p->send_start_time =%ld",nowtime,runtime_p->send_start_time );
			fprintf(stderr,"\nruntime_p->format_Up.afn= %02x  runtime_p->format_Up.fn=%d",runtime_p->format_Up.afn,runtime_p->format_Up.fn);
			if ((nowtime  - runtime_p->send_start_time > 20) &&
				runtime_p->format_Up.afn != 0x03 && runtime_p->format_Up.fn!= 10)
			{
				fprintf(stderr,"\n读取载波信息");
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
				runtime_p->send_start_time = nowtime ;
				sendlen = AFN03_F10(&runtime_p->format_Down,runtime_p->sendbuf);//查询载波模块信息
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
			}else if (runtime_p->format_Up.afn == 0x03 && runtime_p->format_Up.fn == 10)
			{//返回载波信息
				fprintf(stderr,"\n返回载波信息");
				memcpy(&module_info,&runtime_p->format_Up.afn03_f10_up,sizeof(module_info));
				printModelinfo(module_info);
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
				step_init = 0;
				return SLAVE_COMP;
			}
			else if  (runtime_p->send_start_time !=0 && (nowtime  - runtime_p->send_start_time)>10)
			{//超时
				fprintf(stderr,"\n读取载波信息超时");
				step_init = 0;
			}
			break;
	}
	return DATE_CHANGE;
}
int doSetMasterAddr(RUNTIME_PLC *runtime_p)
{
	INT8U masteraddr[6] = {0x01,0x02,0x03,0x04,0x05,0x06};
	int sendlen=0;
	static int step_MasterAddr = 0;
	time_t nowtime = time(NULL);
	switch(step_MasterAddr )
	{
		case 0:
			if (nowtime  - runtime_p->send_start_time > 20)
			{
				DbgPrintToFile1(31,"\n设置主节点地址 ");
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
				memcpy(runtime_p->masteraddr,masteraddr,6);
				sendlen = AFN05_F1(&runtime_p->format_Down,runtime_p->sendbuf,masteraddr);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				runtime_p->send_start_time = nowtime;
			}else if (runtime_p->format_Up.afn==0x00 && (runtime_p->format_Up.fn==1))
			{
				clearvar(runtime_p);
				DbgPrintToFile1(31,"\n设置主节点完成");

				return NONE_PROCE;
			}
			break;
	}
	return INIT_MASTERADDR ;
}
int doCompSlaveMeter(RUNTIME_PLC *runtime_p)
{
	static int step_cmpslave = 0;
	static int slavenum = 0;
	static int index=0;
	static struct Tsa_Node *currtsa;//=tsa_zb_head;
	struct Tsa_Node nodetmp;
	int i=0, sendlen=0, findflg=0;
	INT8U addrtmp[6]={};
	time_t nowtime = time(NULL);
	switch(step_cmpslave)
	{
		case 0://读取载波从节点数量
			if ((nowtime  - runtime_p->send_start_time > 20) &&
				runtime_p->format_Up.afn != 0x10 && runtime_p->format_Up.fn!= 1)
			{
				DbgPrintToFile1(31,"读从节点数量");
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
				runtime_p->send_start_time = nowtime ;
				sendlen = AFN10_F1(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
			}else if(runtime_p->format_Up.afn == 0x10 && runtime_p->format_Up.fn == 1)
			{//返回从节点数量
				slavenum = runtime_p->format_Up.afn10_f1_up.Num ;
				DbgPrintToFile1(31,"载波模块中从节点 %d 个",slavenum);
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
				step_cmpslave = 1;
				index = 1;
			}
			break;

		case 1://读取全部载波从节点
			if ((nowtime  - runtime_p->send_start_time > 20) &&
				runtime_p->format_Up.afn != 0x10 && runtime_p->format_Up.fn!= 2)
			{
				DbgPrintToFile1(31,"读从节点信息 %d ",index);
				sendlen = AFN10_F2(&runtime_p->format_Down,runtime_p->sendbuf,index,26);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime;
				index += 26;
			}else if (runtime_p->format_Up.afn == 0x10 && runtime_p->format_Up.fn == 2)
			{
				int replyn = runtime_p->format_Up.afn10_f2_up.ReplyNum;
				DbgPrintToFile1(31,"返回从节点数量 %d ",replyn);
				for(i=0; i<replyn; i++)
				{
					addTsaList(&tsa_zb_head,runtime_p->format_Up.afn10_f2_up.SlavePoint[i].Addr);
				}
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
				if (index >= slavenum)
				{
					DbgPrintToFile1(31,"读取结束 读%d 个  实际 %d 个",index,slavenum);
					tsa_print(tsa_zb_head,slavenum);
					step_cmpslave = 2;//读取结束
					index = 0;
					currtsa = tsa_zb_head;
				}
			}
			break;

		case 2://删除多余节点
			if (nowtime  - runtime_p->send_start_time > 10)
			{
				for(;;)
				{
					if (currtsa == NULL)
					{
						DbgPrintToFile1(31,"删除判断完成!");
						step_cmpslave = 3;
						clearvar(runtime_p);
						currtsa = tsa_head;	//删除完成 ,开始第 3 步
						break;
					}else
					{
						nodetmp.tsa = getNextTsa(&currtsa); //从载波模块的档案中取出一个 tsa
						findflg = findTsaInList(tsa_head,&nodetmp);
						if (findflg==0)
						{
							DbgPrintToFile1(31,"节点  tsatmp (%d): %02x %02x %02x %02x %02x %02x %02x %02x  删除",findflg,nodetmp.tsa.addr[0],nodetmp.tsa.addr[1],nodetmp.tsa.addr[2],nodetmp.tsa.addr[3],nodetmp.tsa.addr[4],nodetmp.tsa.addr[5],nodetmp.tsa.addr[6],nodetmp.tsa.addr[7]);
							addrtmp[5] = nodetmp.tsa.addr[2];
							addrtmp[4] = nodetmp.tsa.addr[3];
							addrtmp[3] = nodetmp.tsa.addr[4];
							addrtmp[2] = nodetmp.tsa.addr[5];
							addrtmp[1] = nodetmp.tsa.addr[6];
							addrtmp[0] = nodetmp.tsa.addr[7];
							sendlen = AFN11_F2(&runtime_p->format_Down,runtime_p->sendbuf, addrtmp);//&nodetmp.tsa.addr[2]);//在载波模块中删除一个表地址，addr[0]=7 addr[1]=5固定
							SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
							runtime_p->send_start_time = nowtime;
							break;
						}
					}
				}
			}else if (runtime_p->format_Up.afn==0x00 && (runtime_p->format_Up.fn==1))
			{
				clearvar(runtime_p);
			}
			break;

		case 3://添加节点
			if (nowtime  - runtime_p->send_start_time > 10)
			{
				for(;;)
				{
					if (currtsa == NULL)
					{
						DbgPrintToFile1(31,"添加判断完成");
						step_cmpslave = 0;
						clearvar(runtime_p);
						return INIT_MASTERADDR;
					}else
					{
						nodetmp.tsa = getNextTsa(&currtsa);	//从档案中取一个tsa
						findflg = findTsaInList(tsa_zb_head,&nodetmp);
						if (findflg == 0)
						{
							DbgPrintToFile1(31,"节点  tsatmp (%d): %02x %02x %02x %02x %02x %02x %02x %02x  添加",findflg,nodetmp.tsa.addr[0],nodetmp.tsa.addr[1],nodetmp.tsa.addr[2],nodetmp.tsa.addr[3],nodetmp.tsa.addr[4],nodetmp.tsa.addr[5],nodetmp.tsa.addr[6],nodetmp.tsa.addr[7]);
							addrtmp[5] = nodetmp.tsa.addr[2];
							addrtmp[4] = nodetmp.tsa.addr[3];
							addrtmp[3] = nodetmp.tsa.addr[4];
							addrtmp[2] = nodetmp.tsa.addr[5];
							addrtmp[1] = nodetmp.tsa.addr[6];
							addrtmp[0] = nodetmp.tsa.addr[7];
							sendlen = AFN11_F1(&runtime_p->format_Down,runtime_p->sendbuf, addrtmp);//&nodetmp.tsa.addr[2]);//在载波模块中添加一个TSA
							SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
							runtime_p->send_start_time = nowtime;
							break;
						}
					}
				}
			}else if (runtime_p->format_Up.afn==0x00 && (runtime_p->format_Up.fn==1))
			{
				clearvar(runtime_p);
			}
	}
	return SLAVE_COMP ;
}
void Addr_TSA(INT8U *addr,TSA *tsa)
{
	tsa->addr[0] = 7;
	tsa->addr[1] = 5;
	tsa->addr[2] = addr[5];
	tsa->addr[3] = addr[4];
	tsa->addr[4] = addr[3];
	tsa->addr[5] = addr[2];
	tsa->addr[6] = addr[1];
	tsa->addr[7] = addr[0];


//	memcpy(&tsa->addr[2],addr,6);
}
void Echo_Frame(RUNTIME_PLC *runtime_p,INT8U *framebuf,int len)
{
	int sendlen = 0,flag = 0;
	if ( len > 1 )
	{
		flag = 2;
		fprintf(stderr,"\n可以抄读！");
	}
	else if ( len == 1)
	{
		flag = 0;
		fprintf(stderr,"\n失败切表！");
	}else
	{
		flag = 1;
		fprintf(stderr,"\n抄读成功！");
	}
	memcpy(runtime_p->format_Down.addr.SourceAddr,runtime_p->masteraddr,6);
	sendlen = AFN14_F1(&runtime_p->format_Down,runtime_p->sendbuf,runtime_p->format_Up.afn14_f1_up.SlavePointAddr, flag, 0, len, framebuf);
	SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
	clearvar(runtime_p);
}
/***********************************************************************
 *  从index指向的bit开始，找到第一个为0的位 返回该位标示的数据项索引
 *  然后，index指针向前移动1
 *  desnode位当前电表信息节点地址，itemnum：当前采集方案数据项总数
 *  返回值： -1  标识bit返回到第0位
 **********************************************************************/
int findFirstZeroFlg(struct Tsa_Node *desnode,int itemnum)
{

	int byten =0, biti =0 ,index=0;
	do
	{
		index = desnode->curr_i;			//当前需要抄读的数据项索引  62   7, >>6
		byten = index / 8;
		biti = index % 8;

		desnode->curr_i = (desnode->curr_i + 1) % itemnum;
		if (desnode->curr_i==0)
			desnode->readnum++;
		if (((desnode->flag[byten] >> biti) & 0x01) == 0 )
		{
			fprintf(stderr,"\n第 %d 数据项 ,需要抄",index);
			return index;
		}
	}while(index!=itemnum-1);
	return -1;
}
int Format07(FORMAT07 *Data07,OAD oad1,OAD oad2,TSA tsa)
{
	INT8U startIndex =0;
	int find_07item = 0;
	C601F_07Flag obj601F_07Flag;

	memset(Data07, 0, sizeof(FORMAT07));
	find_07item = OADMap07DI(oad1.OI,oad2,&obj601F_07Flag) ;
	DbgPrintToFile1(31,"find_07item=%d   %04x %04x",find_07item,oad1.OI,oad2.OI);
	if (find_07item == 1)
	{
		Data07->Ctrl = 0x11;
		startIndex = 5 - tsa.addr[1];
		memcpy(&Data07->Addr[startIndex], &tsa.addr[2], (tsa.addr[1]+1));
		memcpy(Data07->DI, &obj601F_07Flag.DI_1[0], 4);
		return 1;
	}
	return 0;
}
int buildProxyFrame(RUNTIME_PLC *runtime_p,struct Tsa_Node *desnode,Proxy_Msg pMsg)
{
	int sendlen = 0;
	INT8U type = 0;
	FORMAT07 Data07;
	OAD requestOAD1;
	OAD requestOAD2;
	INT8U addrtmp[6];
	type = desnode->protocol;
	switch (type)
	{
		case DLT645_07:
			requestOAD1.OI = 0;
			requestOAD2.OI = pMsg.oi;
			requestOAD2.attflg = 0x02;
			requestOAD2.attrindex = 0x00;

			Format07(&Data07,requestOAD1,requestOAD2,desnode->tsa);
			memset(buf645,0,BUFSIZE645);
			sendlen = composeProtocol07(&Data07, buf645);
			DbgPrintToFile1(31,"sendlen=%d",sendlen);
			DbPrt1(31,"645:", (char *) buf645, sendlen, NULL);
			if (sendlen>0)
			{
				memcpy(runtime_p->format_Down.addr.SourceAddr,runtime_p->masteraddr,6);
				addrtmp[5] = desnode->tsa.addr[2];
				addrtmp[4] = desnode->tsa.addr[3];
				addrtmp[3] = desnode->tsa.addr[4];
				addrtmp[2] = desnode->tsa.addr[5];
				addrtmp[1] = desnode->tsa.addr[6];
				addrtmp[0] = desnode->tsa.addr[7];
				return (AFN13_F1(&runtime_p->format_Down,runtime_p->sendbuf,addrtmp, 2, 0, buf645, sendlen));
			}
			break;
		case DLT698:
			return 20;
	}

	return 0;
}
int saveProxyData(FORMAT3762 format_3762_Up)
{
	INT8U buf645[255];
	int len645=0;
	INT8U nextFlag=0;
	FORMAT07 frame07;
	INT8U dataContent[50];
	memset(dataContent,0,sizeof(dataContent));

	if (format_3762_Up.afn13_f1_up.MsgLength > 0)
	{
		len645 = format_3762_Up.afn13_f1_up.MsgLength;
		memcpy(buf645, format_3762_Up.afn13_f1_up.MsgContent, len645);
		if (analyzeProtocol07(&frame07, buf645, len645, &nextFlag) == 0)
		{
			if(data07Tobuff698(frame07,dataContent) > 0)
			{
				TSGet(&p_Proxy_Msg_Data->realdata.tm_collect);
				memcpy(p_Proxy_Msg_Data->realdata.data_All,&dataContent[3],4);
				memcpy(p_Proxy_Msg_Data->realdata.Rate1_Data,&dataContent[8],4);
				memcpy(p_Proxy_Msg_Data->realdata.Rate2_Data,&dataContent[13],4);
				memcpy(p_Proxy_Msg_Data->realdata.Rate3_Data,&dataContent[18],4);
				memcpy(p_Proxy_Msg_Data->realdata.Rate4_Data,&dataContent[23],4);
			}
			else
			{
				memset(&p_Proxy_Msg_Data->realdata,0xee,sizeof(RealDataInfo));
			}
			p_Proxy_Msg_Data->done_flag = 1;
		}
	}
	return 0;
}
DATA_ITEM checkMeterData(TASK_INFO *meterinfo,int *taski,int *itemi)
{
	int i=0,j=0;
	time_t nowt = time(NULL);
	DATA_ITEM item;

	memset(&item,0,sizeof(DATA_ITEM));
	DbgPrintToFile1(31,"检查  %02x%02x%02x%02x%02x%02x%02x%02x%02x  index=%d 任务数 %d   数据相个数 %d",
			meterinfo->tsa.addr[0],meterinfo->tsa.addr[1],meterinfo->tsa.addr[2],
			meterinfo->tsa.addr[3],meterinfo->tsa.addr[4],meterinfo->tsa.addr[5],
			meterinfo->tsa.addr[6],meterinfo->tsa.addr[7],meterinfo->tsa.addr[8],
			meterinfo->tsa_index,meterinfo->task_n,meterinfo->task_list[i].fangan.item_n);

	for(i=0; i< meterinfo->task_n; i++)
	{
//		if (nowt >= meterinfo->task_list[i].beginTime )
		{
			 for(j = 0; j<meterinfo->task_list[i].fangan.item_n; j++)
			 {
				 if ( meterinfo->task_list[i].fangan.items[j].sucessflg==0)
				 {
					 item.oad1 = meterinfo->task_list[i].fangan.items[j].oad1;
					 item.oad2 = meterinfo->task_list[i].fangan.items[j].oad2;
					 meterinfo->task_list[i].fangan.items[j].sucessflg = 1;
					 *taski = i;
					 *itemi = j;
					 return item;
				 }
			 }
		}
	}
	return item;
}
int createMeterFrame(struct Tsa_Node *desnode,DATA_ITEM item,INT8U *buf,INT8U *item07)
{
	INT8U type = 0;
	FORMAT07 Data07;
	int sendlen = 0 ,itemindex = -1 ,readcounter = 0;

	if (desnode == NULL)
		return 0;
	type = desnode->protocol;
	memset(buf,0,BUFSIZE645);
	switch (type)
	{
		case DLT645_07:
			Format07(&Data07,item.oad1,item.oad2,desnode->tsa);
			DbgPrintToFile1(31,"当前抄读 【OAD1 %04x-%02x %02x    OAD2 %04x-%02x %02x】%02x%02x%02x%02x ",
						item.oad1.OI,item.oad1.attflg,item.oad1.attrindex,item.oad2.OI,item.oad2.attflg,item.oad2.attrindex,
						Data07.DI[3],Data07.DI[2],Data07.DI[1],Data07.DI[0]);

			sendlen = composeProtocol07(&Data07, buf);
			if (sendlen>0)
			{
				memcpy(item07,Data07.DI,4);// 保存07规约数据项
//					DbPrt1(31,"645:", (char *) buf, sendlen, NULL);
				return sendlen;
			}
			break;
		case DLT698:
			return 20;
	}
	return 0;
}
int ifTsaValid(TSA tsa)
{
	if(tsa.addr[0]!=0 || tsa.addr[1]!=0 || tsa.addr[2]!=0|| tsa.addr[3]!=0|| tsa.addr[4]!=0|| tsa.addr[5]!=0
			|| tsa.addr[6]!=0|| tsa.addr[7]!=0|| tsa.addr[8]!=0|| tsa.addr[9]!=0|| tsa.addr[10]!=0|| tsa.addr[11]!=0
			|| tsa.addr[12]!=0|| tsa.addr[13]!=0|| tsa.addr[14]!=0|| tsa.addr[15]!=0|| tsa.addr[16]!=0)
	{
		return 1;
	}
	return 0;
}
void zeroitemflag(TASK_INFO *task)
{
	int t=0,j=0;
	for(t=0;t<task->task_n;t++)
	{
		for(j=0;j<task->task_list[t].fangan.item_n;j++)
		{
			task->task_list[t].fangan.items[j].sucessflg = 0;
		}
	}
}
int processMeter(INT8U *buf,struct Tsa_Node *desnode)
{
	static int first = 1;
	DATA_ITEM  tmpitem;
	int   sendlen=0;
	//记录单元信息是不是这次要抄的电表的  Y：继续，N：读取本表记录信息到内存，继续
	if (first ==1 )
	{
		taskinfo.tsa = desnode->tsa;
		taskinfo.tsa_index = desnode->tsa_index;
		first = 0;
	}
	DbgPrintToFile1(31,"内存- %02x%02x%02x%02x%02x%02x%02x%02x%02x  index=%d",
			taskinfo.tsa.addr[0],taskinfo.tsa.addr[1],taskinfo.tsa.addr[2],
			taskinfo.tsa.addr[3],taskinfo.tsa.addr[4],taskinfo.tsa.addr[5],
			taskinfo.tsa.addr[6],taskinfo.tsa.addr[7],taskinfo.tsa.addr[8],taskinfo.tsa_index);
	DbgPrintToFile1(31,"请求- %02x%02x%02x%02x%02x%02x%02x%02x%02x  index=%d",
			desnode->tsa.addr[0],desnode->tsa.addr[1],desnode->tsa.addr[2],
			desnode->tsa.addr[3],desnode->tsa.addr[4],desnode->tsa.addr[5],
			desnode->tsa.addr[6],desnode->tsa.addr[7],desnode->tsa.addr[8],desnode->tsa_index);

	if (memcmp(taskinfo.tsa.addr,desnode->tsa.addr,TSA_LEN)!=0 )
	{
		if (ifTsaValid(taskinfo.tsa)==1)//判断为有效TSA
		{
			DbgPrintToFile1(31,"保存 %02x%02x%02x%02x%02x%02x%02x%02x%02x  index=%d",
				taskinfo.tsa.addr[0],taskinfo.tsa.addr[1],taskinfo.tsa.addr[2]
				,taskinfo.tsa.addr[3],taskinfo.tsa.addr[4],taskinfo.tsa.addr[5]
			    ,taskinfo.tsa.addr[6],taskinfo.tsa.addr[7],taskinfo.tsa.addr[8],taskinfo.tsa_index);
			saveParaClass(0x8888, &taskinfo,taskinfo.tsa_index);
		}
		DbgPrintToFile1(31,"读取 desnode->tsa_index=%d",desnode->tsa_index);
		int ret = readParaClass(0x8888, &taskinfo, desnode->tsa_index) ;
		DbgPrintToFile1(31,"读取 ret =%d",ret);
		if (ret != 1 )//读出 desnode->tsa的记录
		{// 返回 0 成功   1 失败
			DbgPrintToFile1(31,"读取失败");
			taskinfo.tsa = desnode->tsa;
			taskinfo.tsa_index = desnode->tsa_index;
			zeroitemflag(&taskinfo);
		}else
		{
			DbgPrintToFile1(31,"读取成功  %02x%02x%02x%02x%02x%02x%02x%02x%02x  index=%d",
						taskinfo.tsa.addr[0],taskinfo.tsa.addr[1],taskinfo.tsa.addr[2],
						taskinfo.tsa.addr[3],taskinfo.tsa.addr[4],taskinfo.tsa.addr[5],
						taskinfo.tsa.addr[6],taskinfo.tsa.addr[7],taskinfo.tsa.addr[8],taskinfo.tsa_index);
		}
	}
	//检查当前电表需要抄读哪个数据项
	int taski=0, itemi=0;//返回 tmpitem指示的具体任务索引 ，itemi指示的具体数据项索引
	INT8U item07[4]={0,0,0,0};
	tmpitem = checkMeterData(&taskinfo,&taski,&itemi);
	if (tmpitem.oad1.OI !=0 || tmpitem.oad2.OI !=0 )
	{	//组织抄读报文
		sendlen = createMeterFrame(desnode, tmpitem, buf, item07);
		taskinfo.task_list[taski].fangan.items[itemi].item07[0] = item07[0];
		taskinfo.task_list[taski].fangan.items[itemi].item07[1] = item07[1];
		taskinfo.task_list[taski].fangan.items[itemi].item07[2] = item07[2];
		taskinfo.task_list[taski].fangan.items[itemi].item07[3] = item07[3];
		taskinfo.now_taski = taski;
		taskinfo.now_itemi = itemi;
		taskinfo.task_list[taski].fangan.item_i = itemi;
		PrintTaskInfo2(&taskinfo);
	}else
	{
		//切表
		DbgPrintToFile1(31,"切表");
		sendlen = 1;
	}
	return sendlen;
}

int buildMeterFrame(INT8U *buf,struct Tsa_Node *desnode,CJ_FANGAN fangAn)
{
	INT8U type = 0;
	FORMAT07 Data07;
	int sendlen = 0 ,itemindex = -1 ,readcounter = 0;

	if (desnode == NULL)
		return 0;

	readcounter = desnode->readnum;
	type = desnode->protocol;
	itemindex = findFirstZeroFlg(desnode, fangAn.item_n);//fangAn.item_n);
	memset(buf,0,BUFSIZE645);
	if (itemindex >= 0 && readcounter < 2)//全部数据最多抄读2次
	{
		DbgPrintToFile1(31,"当前抄读第 %d 数据项 curr_i指到 %d 【OAD1 %04x-%02x %02x    OAD2 %04x-%02x %02x】第 %d 次抄读",itemindex,desnode->curr_i,
				fangAn.items[itemindex].oad1.OI,fangAn.items[itemindex].oad1.attflg,fangAn.items[itemindex].oad1.attrindex,
				fangAn.items[itemindex].oad2.OI,fangAn.items[itemindex].oad2.attflg,fangAn.items[itemindex].oad2.attrindex,desnode->readnum+1);
		switch (type)
		{
			case DLT645_07:
				Format07(&Data07,fangAn.items[itemindex].oad1,fangAn.items[itemindex].oad2,desnode->tsa);
				sendlen = composeProtocol07(&Data07, buf);
				if (sendlen>0)
				{
//					DbPrt1(31,"645:", (char *) buf, sendlen, NULL);
					return sendlen;
				}
				break;
			case DLT698:
				return 20;
		}
	}
	return 0;
}
void addTimeLable(TASK_INFO *tskinfo,int taski,int itemi)
{
	INT8U index = tskinfo->task_list[taski].fangan.item_i;//当前抄到方案数据项第几数据项目
	if (index == 0)
	{
		//保存开始时间
	}else if (index == tskinfo->task_list[taski].fangan.item_n)
	{
		//保存完成时间
	}
}
int saveTaskData(FORMAT3762 format_3762_Up,INT8U taskid)
{
	struct Tsa_Node *nodetmp = NULL;
	INT8U buf645[255];
	int len645=0;
	INT8U nextFlag=0;
	TSA tsatmp;
	FORMAT07 frame07;
	INT8U dataContent[50];
	memset(dataContent,0,sizeof(dataContent));

	if (format_3762_Up.afn06_f2_up.MsgLength > 0)
	{
		len645 = format_3762_Up.afn06_f2_up.MsgLength;
		memcpy(buf645, format_3762_Up.afn06_f2_up.MsgContent, len645);
		if (analyzeProtocol07(&frame07, buf645, len645, &nextFlag) == 0)
		{
			Addr_TSA(frame07.Addr,&tsatmp);
			DbgPrintToFile1(31,"内存- %02x%02x%02x%02x%02x%02x%02x%02x%02x  index=%d",
					taskinfo.tsa.addr[0],taskinfo.tsa.addr[1],taskinfo.tsa.addr[2],
					taskinfo.tsa.addr[3],taskinfo.tsa.addr[4],taskinfo.tsa.addr[5],
					taskinfo.tsa.addr[6],taskinfo.tsa.addr[7],taskinfo.tsa.addr[8],taskinfo.tsa_index);
			DbgPrintToFile1(31,"上数- %02x%02x%02x%02x%02x%02x%02x%02x%02x",
					tsatmp.addr[0],tsatmp.addr[1],tsatmp.addr[2],
					tsatmp.addr[3],tsatmp.addr[4],tsatmp.addr[5],
					tsatmp.addr[6],tsatmp.addr[7],tsatmp.addr[8]);
			if (memcmp(taskinfo.tsa.addr,tsatmp.addr,8) == 0 )
			{//是当前抄读TSA 数据
				TS ts;
				INT8U tmp[4]={0,0,0,0};
				INT8U alldata[100]={};
				int taski = taskinfo.now_taski;
				int itemi = taskinfo.now_itemi;
				TSGet(&ts);
				memcpy(tmp,taskinfo.task_list[taski].fangan.items[itemi].item07,4);
				DbgPrintToFile1(31,"回码数据项 %02x%02x%02x%02x",frame07.DI[0],frame07.DI[1],frame07.DI[2],frame07.DI[3]);
				DbgPrintToFile1(31,"抄读数据项 %02x%02x%02x%02x",tmp[0],tmp[1],tmp[2],tmp[3]);
				if (memcmp(tmp,frame07.DI,4) == 0)//抄读项 与 回码数据项相同
				{
					int len698 = data07Tobuff698(frame07,dataContent);
					if(len698 > 0)
					{
						alldata[0] = 0x55;
						memcpy(&alldata[1],taskinfo.tsa.addr,17);
						memcpy(&alldata[18],dataContent,len698);
						len698 = len698 + 18;
						DbPrt1(31,"存储:", (char *) alldata, len698, NULL);
						SaveOADData(taskinfo.task_list[taski].taskId,
								taskinfo.task_list[taski].fangan.items[itemi].oad1,
								taskinfo.task_list[taski].fangan.items[itemi].oad2,
								alldata,len698,
								ts);
					}
				}
			}
		}
	}
//	{
//		nodetmp = getNodeByTSA(tsa_head,tsatmp);
//		readParaClass(0x9999, &tmpmter, nodetmp->tsa_index);// 返回 0 成功   1 失败
//	}
	return 1;
}
int saveSerchMeter(FORMAT3762 format_3762_Up)
{
	if (format_3762_Up.afn06_f4_up.DeviceType == 1)
	{
		DbgPrintToFile1(31,"\n搜到设备-电表");
	}
	else
	{
		DbgPrintToFile1(31,"\n搜到设备-采集器");
	}
	return 1;
}

int checkAllData(struct Tsa_Node* head,struct Tsa_Node desnode)
{
	return 1;
}
int doTask(RUNTIME_PLC *runtime_p)
{
	static int step_cj = 0, beginwork=0;
	struct Tsa_Node *nodetmp;
	TSA tsatmp;
	int sendlen=0;

	time_t nowtime = time(NULL);
	if (runtime_p->redo == 1)
	{
		fprintf(stderr,"\n--------redo 重启抄表");
		step_cj = 0;
	}
	else if (runtime_p->redo == 2)
	{
		fprintf(stderr,"\n--------redo 恢复抄表");
		step_cj = 1;
	}

	switch( step_cj )
	{
		case 0://重启抄表
			if ( nowtime - runtime_p->send_start_time > 20)
			{
				fprintf(stderr,"\n--------do 重启抄表");
				DbgPrintToFile1(31,"\n重启抄表");
				clearvar(runtime_p);
				runtime_p->redo = 0;
				runtime_p->send_start_time = nowtime ;
				sendlen = AFN12_F1(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
			}else if(runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1)
			{//确认
				clearvar(runtime_p);
				step_cj = 2;
				beginwork = 0;
			}
			break;
		case 1://恢复抄读
			if ( nowtime - runtime_p->send_start_time > 20)
			{
				fprintf(stderr,"\n--------do 恢复抄表");
				DbgPrintToFile1(31,"\n恢复抄表");
				runtime_p->redo = 0;
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime ;
				sendlen = AFN12_F3(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
			}else if(runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1)
			{//确认
				clearvar(runtime_p);
				step_cj = 2;
				beginwork = 0;
			}
			break;
		case 2://开始抄表
			if (runtime_p->format_Up.afn == 0x14 && runtime_p->format_Up.fn == 1)//收到请求抄读命令
			{
				beginwork = 1;//收到第一个请求抄读开始
				Addr_TSA(runtime_p->format_Up.afn14_f1_up.SlavePointAddr,&tsatmp);
				DbgPrintToFile1(31,"\n ");
				DbgPrintToFile1(31,"请求地址【%02x-%02x-%02x%02x%02x%02x%02x%02x】",tsatmp.addr[0],tsatmp.addr[1],tsatmp.addr[2],tsatmp.addr[3],tsatmp.addr[4],tsatmp.addr[5],tsatmp.addr[6],tsatmp.addr[7]);//TODO 抄表组织报文
				sendlen = 0;
				nodetmp = NULL;
				nodetmp = getNodeByTSA(tsa_head,tsatmp);
				if( nodetmp != NULL )
				{
					sendlen = processMeter(buf645,nodetmp);
				}
				Echo_Frame( runtime_p,buf645,sendlen);//内部根据sendlen判断抄表 / 切表
				runtime_p->send_start_time = nowtime;
			}else if ( runtime_p->format_Up.afn == 0x06 && runtime_p->format_Up.fn == 2)//收到返回抄表数据
			{
				DbgPrintToFile1(31,"收数据");
				saveTaskData(runtime_p->format_Up, runtime_p->taskno);
				sendlen = AFN00_F01( &runtime_p->format_Down,runtime_p->sendbuf );//确认
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime;
			}
			break;
	}
	return TASK_PROCESS;
}
int doProxy(RUNTIME_PLC *runtime_p)
{
	static int step_cj = 0, beginwork=0;
	static struct Tsa_Node *currtsa;
	struct Tsa_Node *nodetmp;
	TSA tsatmp;
	int sendlen=0;

	time_t nowtime = time(NULL);
	switch( step_cj )
	{
		case 0://暂停抄表
			if ( nowtime - runtime_p->send_start_time > 20)
			{
				DbgPrintToFile1(31,"暂停抄表");
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime ;
				sendlen = AFN12_F2(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
			}else if(runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1)
			{//确认
				clearvar(runtime_p);
				step_cj = 1;
				beginwork = 0;
			}
			break;

		case 1://开始监控载波从节点
			if (beginwork==0  &&
				judgebit(cjguiProxy.isInUse,2)==1 &&
				cjguiProxy.strProxyMsg.port.OI == 0xf209 )
			{//发送点抄
				DbgPrintToFile1(31,"dealGuiRead 处理液晶点抄 :%02x%02x%02x%02x%02x%02x%02x%02x 波特率=%d protocol=%d 端口号=%04x%02x%02x 规约类型=%d 数据标识=%04x"
						,cjguiProxy.strProxyMsg.addr.addr[0],cjguiProxy.strProxyMsg.addr.addr[1],cjguiProxy.strProxyMsg.addr.addr[2],cjguiProxy.strProxyMsg.addr.addr[3]
						,cjguiProxy.strProxyMsg.addr.addr[4],cjguiProxy.strProxyMsg.addr.addr[5],cjguiProxy.strProxyMsg.addr.addr[6],cjguiProxy.strProxyMsg.addr.addr[7]
						,cjguiProxy.strProxyMsg.baud,cjguiProxy.strProxyMsg.protocol,cjguiProxy.strProxyMsg.port.OI,cjguiProxy.strProxyMsg.port.attflg,cjguiProxy.strProxyMsg.port.attrindex
						,cjguiProxy.strProxyMsg.protocol,cjguiProxy.strProxyMsg.oi);
				beginwork = 1;
				nodetmp = NULL;
				nodetmp = getNodeByTSA(tsa_head,cjguiProxy.strProxyMsg.addr) ;
				clearvar(runtime_p);
				if( nodetmp != NULL )
				{
					DbgPrintToFile1(31,"发送点抄报文");
					memcpy(runtime_p->format_Down.addr.SourceAddr,runtime_p->masteraddr,6);
					sendlen = buildProxyFrame(runtime_p,nodetmp,cjguiProxy.strProxyMsg);
					SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				}
				runtime_p->send_start_time = nowtime;
			}
			else if ((runtime_p->format_Up.afn == 0x13 && runtime_p->format_Up.fn == 1 ))
			{//收到应答数据，或超时10秒，
				cjguiProxy.isInUse = cjguiProxy.isInUse & 0b11111011;
				beginwork = 0;
				saveProxyData(runtime_p->format_Up);
				memset(&runtime_p->format_Up,0,sizeof(runtime_p->format_Up));
				DbgPrintToFile1(31,"收到点抄数据");
			}else if ((nowtime - runtime_p->send_start_time > 20  ) && beginwork==1)
			{
				DbgPrintToFile1(31,"单次点抄超时");
				cjguiProxy.isInUse = cjguiProxy.isInUse & 0b11111011;
				beginwork = 0;
			}
			else if( nowtime - runtime_p->send_start_time > 100  )
			{//100秒等待
				DbgPrintToFile1(31,"100秒超时");
				clearvar(runtime_p);
				beginwork = 0;
				step_cj = 2;
			}
			break;
		case 2:
			if (runtime_p->state_bak == TASK_PROCESS )
			{
				if ( nowtime - runtime_p->send_start_time > 20)
				{
					DbgPrintToFile1(31,"恢复抄表");
					clearvar(runtime_p);
					runtime_p->send_start_time = nowtime ;
					sendlen = AFN12_F3(&runtime_p->format_Down,runtime_p->sendbuf);
					SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
					memset(&runtime_p->format_Up,0,sizeof(runtime_p->format_Up));
				}else if(runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1)
				{
					clearvar(runtime_p);
					step_cj = 0;
					beginwork = 0;
					DbgPrintToFile1(31,"返回到状态%d",runtime_p->state_bak);
					return(runtime_p->state_bak);
				}
			}else
			{
				clearvar(runtime_p);
				step_cj = 0;
				beginwork = 0;
				DbgPrintToFile1(31,"返回到状态%d",runtime_p->state_bak);
				return(runtime_p->state_bak);
			}
			break;
	}
	return DATA_REAL;
}
int startSearch(FORMAT3762 *down,INT8U *sendbuf)
{
	FORMAT07 format07_Down;
	INT8U ModuleFlag=0, Ctrl=0x02,searchTime=10, sendLen645=0;
	format07_Down.Ctrl = 0x13;//启动搜表
	format07_Down.SearchTime[0] = searchTime%256;
	format07_Down.SearchTime[1] = searchTime/256;
	memset(buf645,0,BUFSIZE645);
	sendLen645 = composeProtocol07(&format07_Down, buf645);
	int len = AFN05_F3(down, ModuleFlag, Ctrl, buf645, sendLen645,sendbuf);
	return (len);
}
int doSerch(RUNTIME_PLC *runtime_p)
{
	static int step_cj = 0, beginwork=0;
	static struct Tsa_Node *currtsa;
	struct Tsa_Node *nodetmp;
	TSA tsatmp;
	int sendlen=0;

	time_t nowtime = time(NULL);
	if (runtime_p->nowts.Hour==23)
	{
		step_cj = 0;
		beginwork = 0;
		clearvar(runtime_p);
		DbgPrintToFile1(31,"23点结束搜表，返回状态 %d",runtime_p->state_bak);
	}

	switch( step_cj )
	{
		case 0://暂停抄读
			if ( nowtime - runtime_p->send_start_time > 20)
			{
				DbgPrintToFile1(31,"暂停抄表");
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime ;
				sendlen = AFN12_F2(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
			}else if(runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1)
			{//确认
				clearvar(runtime_p);
				step_cj = 1;
			}
			break;
		case 1://启动广播
			if ( nowtime - runtime_p->send_start_time > 20  && beginwork==0)
			{
				DbgPrintToFile1(31,"启动广播");
				clearvar(runtime_p);
				memcpy(runtime_p->format_Down.addr.SourceAddr,runtime_p->masteraddr,6);
				sendlen = startSearch(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				runtime_p->send_start_time = nowtime ;
			}else if (runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1 && beginwork==0)//收到确认
			{
				DbgPrintToFile1(31,"收到确认");
				runtime_p->send_start_time = nowtime ;
				fprintf(stderr,"\nruntime_p->send_start_time = %ld",runtime_p->send_start_time);
				beginwork = 1;
			}else if (beginwork == 1)
			{
				fprintf(stderr,"\nruntime_p->send_start_time = %ld   nowtime=%ld",runtime_p->send_start_time,nowtime);
				if (nowtime - runtime_p->send_start_time >120 )
				{
					DbgPrintToFile1(31,"等待到时间");
					clearvar(runtime_p);
					step_cj = 2;
				}else
				{
					if ((nowtime-runtime_p->send_start_time) % 10 == 0)
					{
						DbgPrintToFile1(31,"等待120秒... (%ld)",nowtime-runtime_p->send_start_time);
						sleep(1);
					}
				}
			}
			break;
		case 2://激活从节点注册
			if ( nowtime - runtime_p->send_start_time > 20)
			{
				DbgPrintToFile1(31,"激活从节点注册",nowtime);
				beginwork = 0;
				clearvar(runtime_p);
				sendlen = AFN11_F5(&runtime_p->format_Down,runtime_p->sendbuf, 20);//minute
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				runtime_p->send_start_time = nowtime ;
			}else if (runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1)//收到确认
			{
				DbgPrintToFile1(31,"激活确认，进入等待从节点主动注册...",nowtime);
				beginwork = 1;
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime ;
				step_cj = 3;
			}
			break;
		case 3://等待注册
			if ( nowtime - runtime_p->send_start_time < 5 *60)
			{
				if (runtime_p->format_Up.afn == 0x06 && runtime_p->format_Up.fn == 4)
				{
					saveSerchMeter(runtime_p->format_Up);
					sendlen = AFN00_F01( &runtime_p->format_Down,runtime_p->sendbuf );//确认
					SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				}
				if ((nowtime-runtime_p->send_start_time) % 100 == 0)
				{
					DbgPrintToFile1(31,"等待1200秒... (%ld)",nowtime-runtime_p->send_start_time);
					sleep(1);
				}
			}else
			{
				DbgPrintToFile1(31,"搜表结束");
				beginwork =0;
				step_cj = 0;
				clearvar(runtime_p);
				return(runtime_p->state_bak);
			}
			break;
	}
	return METER_SEARCH;
}

int dateJudge(TS *old ,TS *new)
{
	if(old->Day!=new->Day || old->Year!=new->Year || old->Month!=new->Month)
	{
		memcpy(old,new,sizeof(TS));
		return 1;
	}
	else
		return 0;
}
void dealData(int state,RUNTIME_PLC *runtime_p)
{
	int datalen = 0;
	RecvDataFromCom(runtime_p->comfd,runtime_p->recvbuf,&RecvHead);
	datalen = StateProcessZb(runtime_p->dealbuf,runtime_p->recvbuf);
	if (datalen>0)
	{
		analyzeProtocol3762(&runtime_p->format_Up,runtime_p->dealbuf,datalen);
//		fprintf(stderr,"\nafn=%02x   fn=%d 返回",runtime_p->format_Up.afn ,runtime_p->format_Up.fn);
	}
	return;
}
void initlist(struct Tsa_Node *head)
{
	struct Tsa_Node *tmp ;
	while(head!=NULL)
	{
		tmp = head;
		head = head->next;
		tmp->curr_i = 0;
		tmp->readnum = 0;
		memset(tmp->flag ,0 ,8);
	}
	return;
}

int stateJuge(int nowdstate,INT8U my6000,RUNTIME_PLC *runtime_p)
{
	int state = nowdstate;
	int taskindex = 0;

	if ( dateJudge(&runtime_p->oldts,&runtime_p->nowts) == 1 || JProgramInfo->oi_changed.oi6000 != my6000)
	{
		DbgPrintToFile1(31,"\n状态切换到初始化");
		runtime_p->initflag = 1;
		runtime_p->state_bak = runtime_p->state;
		state = DATE_CHANGE;
		runtime_p->state = state;
		runtime_p->redo = 1;  //初始化之后需要重启抄读
		return state;
	}

	if ((runtime_p->nowts.Hour==23 && runtime_p->nowts.Minute==59) || (runtime_p->nowts.Hour==0 && runtime_p->nowts.Minute==0))
		return state;  //23点59分--0点0分之间不进行任务判断（准备跨日初始化）

	if ((runtime_p->nowts.Hour==20 && runtime_p->nowts.Minute==0) &&
		 state!=METER_SEARCH && state!=DATE_CHANGE && state!=SLAVE_COMP  && state!=INIT_MASTERADDR )
	{
		DbgPrintToFile1(31,"\n20点启动搜表");
		runtime_p->state_bak = runtime_p->state;
		clearvar(runtime_p);
		runtime_p->redo = 2;  //搜表后需要恢复抄读
		return METER_SEARCH;
	}

	//-------------------------------------------------------------------------------------------------------------------------
	if ( judgebit(cjguiProxy.isInUse ,2) && cjguiProxy.strProxyMsg.port.OI == 0xf209 &&
		 state!=DATE_CHANGE && state!=DATA_REAL)
	{	//出现液晶点抄载波表标识，并且不在初始化和点抄状态
		DbgPrintToFile1(31,"\n载波收到点抄消息 需要处理 %04x ",cjguiProxy.strProxyMsg.port.OI);
		runtime_p->state_bak = runtime_p->state;
		runtime_p->state = DATA_REAL;
		clearvar(runtime_p);
		runtime_p->redo = 2;	//点抄后需要恢复抄读
		return DATA_REAL;
	}

	if (state == NONE_PROCE && taskinfo.task_n>0)
	{
		state = TASK_PROCESS;
		runtime_p->state = TASK_PROCESS;
	}

	return state;
}
void readplc_thread()
{
	INT8U my6000=0;
	int state = DATE_CHANGE;
	RUNTIME_PLC runtimevar;
	memset(&runtimevar,0,sizeof(RUNTIME_PLC));
	my6000 = JProgramInfo->oi_changed.oi6000 ;
	RecvHead = 0;
	RecvTail = 0;
	initTaskData(&taskinfo);
	PrintTaskInfo2(&taskinfo);
	DbgPrintToFile1(31,"载波线程开始");

	while(1)
	{
		usleep(50000);
		/********************************
		 * 	   状态判断
		********************************/
		TSGet(&runtimevar.nowts);
		state = stateJuge(state,my6000,&runtimevar);

		/********************************
		 * 	   状态流程处理
		********************************/
		switch(state)
		{
			case DATE_CHANGE :
				state = doInit(&runtimevar);					//初始化 		 （ 1、硬件复位 2、模块版本信息查询  ）
				break;
			case SLAVE_COMP :
				state = doCompSlaveMeter(&runtimevar);			//从节点比对    ( 1、测量点比对  )
				break;
			case INIT_MASTERADDR:
				state = doSetMasterAddr(&runtimevar);			//设置主节点地址 ( 1、主节点地址设置  )
				break;
			case DATA_REAL :
				state = doProxy(&runtimevar);					//代理		  ( 1、发送代理抄读报文 2、根据超时限定主动退出代理state  ->> oldstate)
				break;
			case METER_SEARCH :
				state = doSerch(&runtimevar);					//搜表		  ( 1、启动搜表 2、根据超时限定主动退出搜表state )
				break;
			case TASK_PROCESS :
				state = doTask(&runtimevar);					//按任务抄表	  ( 1、根据方案类型和编号号确定抄表报文  )
				break;
			default :
					runtimevar.state = NONE_PROCE;
					sleep(1);
				break;
		}
		runtimevar.state  = state;
		/********************************
		 * 	   接收报文，并处理
		********************************/
		dealData(state,&runtimevar);

	}
	freeList(tsa_head);
	freeList(tsa_zb_head);

	pthread_detach(pthread_self());
	pthread_exit(&thread_readplc);
}

void readplc_proccess()
{
	struct mq_attr attr_zb_task;
	mqd_zb_task = mmq_open((INT8S *)TASKID_plc_MQ_NAME,&attr_zb_task,O_RDONLY);

	pthread_attr_init(&readplc_attr_t);
	pthread_attr_setstacksize(&readplc_attr_t,2048*1024);
	pthread_attr_setdetachstate(&readplc_attr_t,PTHREAD_CREATE_DETACHED);
	while ((thread_readplc_id=pthread_create(&thread_readplc, &readplc_attr_t, (void*)readplc_thread, NULL)) != 0)
	{
		sleep(1);
	}
}
