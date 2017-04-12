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
#include "lib3762.h"
#include "readplc.h"
#include "cjdeal.h"
#include "dlt645.h"
extern ProgramInfo* JProgramInfo;
extern INT8U map07DI_698OAD_NUM;
extern CLASS_601F map07DI_698OAD[NUM_07DI_698OAD];
extern INT8S OADMap07DI(OI_698 roadOI,OAD sourceOAD, C601F_07Flag* obj601F_07Flag);
extern void DbgPrintToFile1(INT8U comport,const char *format,...);
extern void DbPrt1(INT8U comport,char *prefix, char *buf, int len, char *suffix);
extern mqd_t mqd_zb_task;
//-----------------------------------------------------------------------------------------------------------------------------------------------------
void SendDataToCom(int fd, INT8U *sendbuf, INT16U sendlen)
{
	int i=0;
	ssize_t slen;
	slen = write(fd, sendbuf, sendlen);
	fprintf(stderr,"\nzb_send(%d): ",sendlen);
	for(i=0;i<sendlen; i++)
		fprintf(stderr,"%02x ",sendbuf[i]);
	DbPrt1(31,"S:", (char *) sendbuf, slen, NULL);
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
				DbPrt1(31,"S:", (char *) str, DataLen, NULL);
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
			fprintf(stderr,"\n------get %p",p);
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
		memcpy(&(*head)->tsa.addr[2] , addr,6);
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
		memcpy(&(new->tsa.addr[2]) , addr,6);//TODO: TSA长度未赋值
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
			if (runtime_p->comfd >0)
				CloseCom( runtime_p->comfd );
			runtime_p->comfd = OpenCom(5, 9600,(unsigned char*)"even",1,8);
			runtime_p->initflag = 0;
			clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
			if (tsa_count==0)
			{
				step_init = 0;
				return NONE_PROCE;
			}
			step_init = 1;
			break;

		case 1://读取载波信息
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

	time_t nowtime = time(NULL);
	switch(step_cmpslave)
	{
		case 0://读取载波从节点数量
			if ((nowtime  - runtime_p->send_start_time > 20) &&
				runtime_p->format_Up.afn != 0x10 && runtime_p->format_Up.fn!= 1)
			{
				DbgPrintToFile1(31,"\n读从节点数量");
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
				runtime_p->send_start_time = nowtime ;
				sendlen = AFN10_F1(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
			}else if(runtime_p->format_Up.afn == 0x10 && runtime_p->format_Up.fn == 1)
			{//返回从节点数量
				slavenum = runtime_p->format_Up.afn10_f1_up.Num ;
				DbgPrintToFile1(31,"\n载波模块中从节点 %d 个\n",slavenum);
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
				step_cmpslave = 1;
				index = 1;
			}
			break;

		case 1://读取全部载波从节点
			if ((nowtime  - runtime_p->send_start_time > 20) &&
				runtime_p->format_Up.afn != 0x10 && runtime_p->format_Up.fn!= 2)
			{
				DbgPrintToFile1(31,"\n读 从节点信息 %d ",index);
				sendlen = AFN10_F2(&runtime_p->format_Down,runtime_p->sendbuf,index,26);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime;
				index += 26;
			}else if (runtime_p->format_Up.afn == 0x10 && runtime_p->format_Up.fn == 2)
			{
				int replyn = runtime_p->format_Up.afn10_f2_up.ReplyNum;
				DbgPrintToFile1(31,"\n返回从节点数量 %d ",replyn);
				for(i=0; i<replyn; i++)
				{
					addTsaList(&tsa_zb_head,runtime_p->format_Up.afn10_f2_up.SlavePoint[i].Addr);
				}
				clearvar(runtime_p);//376.2上行内容容器清空，发送计时归零
				if (index>=slavenum)
				{
					DbgPrintToFile1(31,"\n读取结束 读%d 个  实际 %d 个",index,slavenum);
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
						DbgPrintToFile1(31,"\n删除完成!");
						step_cmpslave = 3;
						clearvar(runtime_p);
						currtsa = tsa_head;	//删除完成 ,开始第 3 步
						break;
					}else
					{
						nodetmp.tsa = getNextTsa(&currtsa); //从载波模块的档案中取出一个 tsa
						findflg = findTsaInList(tsa_head,&nodetmp);
						DbgPrintToFile1(31,"\n节点  tsatmp (%d): %02x %02x %02x %02x %02x %02x %02x %02x",findflg,nodetmp.tsa.addr[0],nodetmp.tsa.addr[1],nodetmp.tsa.addr[2],nodetmp.tsa.addr[3],nodetmp.tsa.addr[4],nodetmp.tsa.addr[5],nodetmp.tsa.addr[6],nodetmp.tsa.addr[7]);
						if (findflg==0)
						{
							DbgPrintToFile1(31,"删除\n");
							sendlen = AFN11_F2(&runtime_p->format_Down,runtime_p->sendbuf, &nodetmp.tsa.addr[2]);//在载波模块中删除一个表地址，addr[0]=7 addr[1]=5固定
							SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
							runtime_p->send_start_time = nowtime;
							break;
						}else
							DbgPrintToFile1(31,"不需要删除");
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
						DbgPrintToFile1(31,"\n添加节点完成");
						step_cmpslave = 0;
						clearvar(runtime_p);
						return INIT_MASTERADDR;
					}else
					{
						nodetmp.tsa = getNextTsa(&currtsa);	//从档案中取一个tsa
						findflg = findTsaInList(tsa_zb_head,&nodetmp);
						DbgPrintToFile1(31,"\n节点  tsatmp (%d): %02x %02x %02x %02x %02x %02x %02x %02x",findflg,nodetmp.tsa.addr[0],nodetmp.tsa.addr[1],nodetmp.tsa.addr[2],nodetmp.tsa.addr[3],nodetmp.tsa.addr[4],nodetmp.tsa.addr[5],nodetmp.tsa.addr[6],nodetmp.tsa.addr[7]);
						if (findflg == 0)
						{
							DbgPrintToFile1(31,"添加\n");
							sendlen = AFN11_F1(&runtime_p->format_Down,runtime_p->sendbuf, &nodetmp.tsa.addr[2]);//在载波模块中添加一个TSA
							SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
							runtime_p->send_start_time = nowtime;
							break;
						}else
							DbgPrintToFile1(31,"不需要添加");
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
	memcpy(&tsa->addr[2],addr,6);
}
void Echo_Frame(RUNTIME_PLC *runtime_p,INT8U *framebuf,int len)
{
	int sendlen = 0,flag = 1;
	if (len>0)
		flag = 2;
	else
		fprintf(stderr,"\n切表！");
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
	DbgPrintToFile1(31,"find_07item=%d",find_07item);
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
int buildMeterFrame(INT8U *buf,struct Tsa_Node *desnode,CJ_FANGAN fangAn)
{
	INT8U type = 0;
	FORMAT07 Data07;
	int sendlen = 10 ,itemindex = -1 ,readcounter = 0;

	if (desnode == NULL)
		return 0;

	readcounter = desnode->readnum;
	type = desnode->protocol;
	itemindex = findFirstZeroFlg(desnode, fangAn.item_n);//fangAn.item_n);
	if (itemindex >= 0 && readcounter < 2)//全部数据最多抄读2次
	{
		DbgPrintToFile1(31,"规约 %d 当前抄读第 %d 数据项 curr_i指到 %d 【OAD1 %04x-%02x %02x    OAD2 %04x-%02x %02x】第 %d 次抄读",type,itemindex,desnode->curr_i,
				fangAn.items[itemindex].oad1.OI,fangAn.items[itemindex].oad1.attflg,fangAn.items[itemindex].oad1.attrindex,
				fangAn.items[itemindex].oad2.OI,fangAn.items[itemindex].oad2.attflg,fangAn.items[itemindex].oad2.attrindex,desnode->readnum+1);

		memset(buf,0,BUFSIZE645);
		switch (type)
		{
			case DLT645_07:
				Format07(&Data07,fangAn.items[itemindex].oad1,fangAn.items[itemindex].oad2,desnode->tsa);
				sendlen = composeProtocol07(&Data07, buf);
				if (sendlen>0)
				{
					DbPrt1(31,"645:", (char *) buf, sendlen, NULL);
					DbgPrintToFile1(31,"sendlen = %d",sendlen);
					return sendlen;
				}
				break;
			case DLT698:
				return 20;
		}
	}
	return 0;
}
int saveFrameData(FORMAT3762 upframe)
{
	return 1;
}
int checkAllData(struct Tsa_Node* head,struct Tsa_Node desnode)
{
	return 1;
}
int doTask(RUNTIME_PLC *runtime_p)
{
	static int step_cj = 0, beginwork=0;
	static struct Tsa_Node *currtsa;
	struct Tsa_Node *nodetmp;
	TSA tsatmp;
	int sendlen=0;

	time_t nowtime = time(NULL);
	switch( step_cj )
	{
		case 0://重启抄表
			if ( nowtime - runtime_p->send_start_time > 20)
			{
				DbgPrintToFile1(31,"\n重启抄表");
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime ;
				sendlen = AFN12_F1(&runtime_p->format_Down,runtime_p->sendbuf);
				SendDataToCom(runtime_p->comfd, runtime_p->sendbuf,sendlen );
			}else if(runtime_p->format_Up.afn == 0x00 && runtime_p->format_Up.fn == 1)
			{//确认
				clearvar(runtime_p);
				step_cj = 1;
				beginwork = 0;
			}
			break;

		case 1://开始抄表
			if (runtime_p->format_Up.afn == 0x14 && runtime_p->format_Up.fn == 1)//收到请求抄读命令
			{
				beginwork = 1;//收到第一个请求抄读开始
				Addr_TSA(runtime_p->format_Up.afn14_f1_up.SlavePointAddr,&tsatmp);
//				DbgPrintToFile1(31,"请求抄表地址【%02x-%02x-%2x%2x%2x%2x%2x%2x】",tsatmp.addr[0],tsatmp.addr[1],tsatmp.addr[2],tsatmp.addr[3],tsatmp.addr[4],tsatmp.addr[5],tsatmp.addr[6],tsatmp.addr[7]);//TODO 抄表组织报文
				sendlen = 0;
				nodetmp = NULL;
				nodetmp = getNodeByTSA(tsa_head,tsatmp) ;
				if( nodetmp != NULL )
				{
					sendlen = buildMeterFrame(buf645,nodetmp,runtime_p->fangAn);
				}
				Echo_Frame( runtime_p,buf645,sendlen);//内部根据sendlen判断抄表 / 切表
				runtime_p->send_start_time = nowtime;
			}else if ( runtime_p->format_Up.afn == 0x06 && runtime_p->format_Up.fn == 2)//收到返回抄表数据
			{
				DbgPrintToFile1(31,"收数据");
				Addr_TSA(runtime_p->format_Up.afn14_f1_up.SlavePointAddr,&tsatmp);
				saveFrameData(runtime_p->format_Up);
				AFN00_F01( &runtime_p->format_Down,runtime_p->sendbuf );//确认
				clearvar(runtime_p);
				runtime_p->send_start_time = nowtime;
			}else if ( nowtime - runtime_p->send_start_time > 300  && beginwork==1)
			{
				fprintf(stderr,"\n---------超过300秒未收到请求抄读 复位----------");
				clearvar(runtime_p);
				beginwork = 0;
				step_cj = 0;
				return DATE_CHANGE;
			}
			break;
	}
	return TASK_PROCESS;
}
int doProxy(RUNTIME_PLC *runtime_p)
{
	return DATA_REAL;
}
int doSerch(RUNTIME_PLC *runtime_p)
{
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
		fprintf(stderr,"\nafn=%02x   fn=%d 返回",runtime_p->format_Up.afn ,runtime_p->format_Up.fn);
	}
	return;
}
int getFangAn(INT8U type,INT8U sernum,CJ_FANGAN *fangAn)
{
	switch(type)
	{
		case norm:
			return (readCoverClass(0x6015, sernum, (void *)&fangAn->to6015, sizeof(CLASS_6015), coll_para_save));
		case events:
			return (readCoverClass(0x6017, sernum, (void *)&fangAn->to6017, sizeof(CLASS_6017), coll_para_save));
	}
	return 0;
}

void print_road(ROAD road)
{
	int w=0;

	fprintf(stderr,"ROAD:%04x-%02x%02x ",road.oad.OI,road.oad.attflg,road.oad.attrindex);
	fprintf(stderr,"ROAD:%04x-%02x%02x ",road.oad.OI,road.oad.attflg,road.oad.attrindex);
	if(road.num >= 16) {
		fprintf(stderr,"csd overvalue 16 error\n");
		return;
	}
	for(w=0;w<road.num;w++)
	{
		fprintf(stderr,"<关联OAD..%d>%04x-%02x%02x ",w,road.oads[w].OI,road.oads[w].attflg,road.oads[w].attrindex);
	}
	fprintf(stderr,"\n");
}

void print_rcsd(CSD_ARRAYTYPE csds)
{
	int i=0;
	for(i=0; i<csds.num;i++)
	{
		if (csds.csd[i].type==0)
		{
			fprintf(stderr,"<%d>OAD%04x-%02x%02x ",i,csds.csd[i].csd.oad.OI,csds.csd[i].csd.oad.attflg,csds.csd[i].csd.oad.attrindex);
		}else if (csds.csd[i].type==1)
		{
			fprintf(stderr,"<%d> ",i);
			print_road(csds.csd[i].csd.road);
		}
	}
}
void PrintItems(CJ_FANGAN fangAn)
{
	int i=0;
	for(i=0;i<fangAn.item_n;i++)
	{
		DbgPrintToFile1(31,"\nOAD1 %04x-%02x%02x  OAD2 %04x-%02x%02x",
				fangAn.items[i].oad1.OI,fangAn.items[i].oad1.attflg,fangAn.items[i].oad1.attrindex,
				fangAn.items[i].oad2.OI,fangAn.items[i].oad2.attflg,fangAn.items[i].oad2.attrindex);
	}

}
void PrintTaskInfo(RUNTIME_PLC *runtime_p)
{
	fprintf(stderr,"\n--------------------------------------------------------\n\n");
	fprintf(stderr,"当前任务序号 %d  ",runtime_p->taskno);
	if(runtime_p->fangAn.type == norm)
	{
		DbgPrintToFile1(31,"\n普通采集方案 【 %d 】",runtime_p->taskno);
		DbgPrintToFile1(31,"\n采集类型 %d",runtime_p->fangAn.to6015.cjtype);
		DbgPrintToFile1(31,"\n电表集合 %d",runtime_p->fangAn.to6015.mst.mstype);
		print_rcsd(runtime_p->fangAn.to6015.csds);
	}
	if(runtime_p->fangAn.type == events)
	{
		DbgPrintToFile1(31,"\n事件采集方案 【 %d 】",runtime_p->taskno);
		DbgPrintToFile1(31,"\n电表集合 %d",runtime_p->fangAn.to6017.ms.mstype);
		DbgPrintToFile1(31,"\n是否主动上送 %d",runtime_p->fangAn.to6017.ifreport);
//		print_rcsd((CSD_ARRAYTYPE)runtime_p->fangAn.to6017.roads);
	}
	fprintf(stderr,"\n\n--------------------------------------------------------\n");
}
void Array_OAD_Items(CJ_FANGAN *fangAn)
{
	int i=0,j=0,num=0, oadtype=0;

	if(fangAn->type == norm)
	{
		DbgPrintToFile1(31,"普通采集方案  csds.num %d",fangAn->to6015.csds.num);
		for(i=0;i<fangAn->to6015.csds.num;i++)//全部CSD循环
		{
			oadtype = fangAn->to6015.csds.csd[i].type;
			if(oadtype == 1)//ROAD
			{
				for(j=0;j<fangAn->to6015.csds.csd[i].csd.road.num;j++)
				{
					fangAn->items[num].oad1 = fangAn->to6015.csds.csd[i].csd.road.oad;
					fangAn->items[num].oad2 = fangAn->to6015.csds.csd[i].csd.road.oads[j];
					num++;
					if (num >= FANGAN_ITEM_MAX ) return;
				}
			}else			//OAD
			{
				fangAn->items[num].oad1.OI = 0;
				fangAn->items[num].oad2 = fangAn->to6015.csds.csd[i].csd.oad;
				num++;
				if (num >= FANGAN_ITEM_MAX ) return;
			}
		}
		fprintf(stderr,"\n普通采集方案 %d [csd] ",fangAn->to6015.csds.num);
	}
	if(fangAn->type == events)
	{
		DbgPrintToFile1(31,"事件方案6017 road num=%d",fangAn->to6017.collstyle.roads.num);
		for(i=0; i< fangAn->to6017.collstyle.roads.num; i++)
		{
			DbgPrintToFile1(31,"事件方案6017 roads %d 的 num=%d",i,fangAn->to6017.collstyle.roads.road[i].num);
			for(j=0;j<fangAn->to6017.collstyle.roads.road[i].num;j++)
			{
				fangAn->items[num].oad1 = fangAn->to6017.collstyle.roads.road[i].oad;
				fangAn->items[num].oad2 = fangAn->to6017.collstyle.roads.road[i].oads[j];
				num++;
				if (num >= FANGAN_ITEM_MAX ) return;
			}
		}
		fprintf(stderr,"\n事件采集方案 %d [ROAD]",fangAn->to6017.collstyle.roads.num);
	}
	fangAn->item_n = num;

	if (num>0)
	{
		DbgPrintToFile1(31,"-----------全部数据项 %d 个-----------",num);
		for(i=0;i<num;i++)
		{
			DbgPrintToFile1(31,"[ %d ] %04x - %04x ",i,fangAn->items[i].oad1.OI,fangAn->items[i].oad2.OI);
		}
		fprintf(stderr,"\n\n\n");
	}
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
		fprintf(stderr,"\n状态切换到初始化");
		runtime_p->initflag = 1;
		runtime_p->state_bak = runtime_p->tasktype;
		state = DATE_CHANGE;
		return state;
	}

	//TODO 代理判断

	if (state == NONE_PROCE)
	{
		taskindex = getTaskIndex(31);
		if ( taskindex > -1 )
		{
			runtime_p->fangAn.type = list6013[taskindex].basicInfo.cjtype;//任务类型
			runtime_p->taskno = list6013[taskindex].basicInfo.sernum;//任务编号
			runtime_p->result6035.taskID = runtime_p->taskno ;
			DbgPrintToFile1(31,"收到任务消息 任务序号%d  【任务编号: %d 】  【类型: %d 】",taskindex,runtime_p->taskno,runtime_p->fangAn.type);
			int readflg = getFangAn(runtime_p->fangAn.type,runtime_p->taskno,&runtime_p->fangAn) ;
			if (readflg ==1)//读取方案
			{
				runtime_p->result6035.taskState = IN_OPR;
				DataTimeGet(&runtime_p->result6035.starttime);
				state = TASK_PROCESS;
				Array_OAD_Items(&runtime_p->fangAn);
				initlist(tsa_head);
				PrintTaskInfo(runtime_p);
				PrintItems(runtime_p->fangAn);
			}
		}
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
				state = doTask(&runtimevar);							//按任务抄表	  ( 1、根据方案类型和编号号确定抄表报文  )
				break;
			default :
				sleep(1);
				break;
		}

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
