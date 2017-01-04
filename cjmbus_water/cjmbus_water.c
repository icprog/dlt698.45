#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include "cjmbus_water.h"


void QuitProcess(ProjectInfo *proinfo)
{
	proinfo->ProjectID=0;
    fprintf(stderr,"\n退出：%s %d",proinfo->ProjectName,proinfo->ProjectID);
	exit(0);
}
/*********************************************************
 * 进程初始化
 *********************************************************/
int InitPro(ProgramInfo** prginfo, int argc, char *argv[])
{
	if (argc >= 2)
	{
		*prginfo = OpenShMem("ProgramInfo",sizeof(ProgramInfo),NULL);
		ProIndex = atoi(argv[1]);
		Port = atoi(argv[2]);
		fprintf(stderr,"\n%s %d run...use ttyS%d",(*prginfo)->Projects[ProIndex].ProjectName,ProIndex,Port);
		(*prginfo)->Projects[ProIndex].ProjectID=getpid();//保存当前进程的进程号
		return 1;
	}
	return 0;
}
int SendPro(int fd,INT8U * buf,INT16U len)
{
	int i=0;
	if(buf == NULL || len == 0)
		return 0;
	fprintf(stderr,"\nMBUS:S(%d)=",len);
	for(i=0;i<len;i++){
		fprintf(stderr,"%02x ",buf[i]);
	}
	for(i=0;i<len;i++){
	    write(fd,&buf[i],1);
	    usleep(2000);
	}
	return 1;
}
int RecvPro(int fd,INT8U* buf,INT32U *head)
{
	INT8U TmpBuf[256]={};
	int len=0,i=0,total=0;
	len = read(fd,TmpBuf,255);
	if(len>0)
	{
		for(i=0;i<len;i++) {
			buf[*head]=TmpBuf[i];
//			if (i%8==0)
//				fprintf(stderr,"\n		");
//			fprintf(stderr,"buf[%d]=%02x ",*head,TmpBuf[i]);
			*head = (*head + 1) % BUFLEN;
		}
		total =total + len;
	}
	return total;
}
int IfTimeout(time_t value,int limite)
{
	if( abs(time(NULL)-value) >= limite)
		return 1;
	return 0;
}
/********************************************************
 * 初始化记录文件：
 * 根据数据的类型datatype: 0当前数据   1日冻结数据   2抄表日数据   3上一日补抄    4上二日补抄  5事件相关
 * 根据：
 * 		表的类型、数据类型 产生抄表记录文件
 *
 *____nand/record/mbus1-0.rec________________
 *____nand/record/mbus1-1.rec________________
 *      		D1   D2   D3  ...  Dn
 * Meter1(0,0)| y    y    n   	   y
 * Meter1(5,1)| y    n    y        y
 * Meter1(5,1)| y    n    y        y
 * ...		  |
 * Meter1(3,2)| y    y    y        n
 * ___________________________________________
 */
void Create_Record(INT8U datatype)
{


}
int GetMeterData_Record(struct DataNode* node, INT8U datatype )
{

	return 1;
}
void GetMeter_Info(INT16U MpNo, MeterType* Meter)
{
	INT8U addtmp[7]={0x58,0x12,0x01,0x08,0x16,0x33,0x78};

	memcpy(Meter->Addr,addtmp,sizeof(addtmp));
	Meter->MpType = 0x10;

	return;
}
void PrtEchoInfo(cj188_Frame* frame,int ret)
{
	int k=0;
	switch (ret)
	{
		case ERR_RCVD_LOST:
			fprintf(stderr,"\n错误:接收报文不完整");
			break;
		case ERR_LOST_0x68:
			fprintf(stderr,"\n错误:报文开头0x68丢失");
			break;
		case ERR_LOST_0x16:
			fprintf(stderr,"\n错误:报文结束0x16丢失");
			break;
		case ERR_SUM_ERROR:
			fprintf(stderr,"\n错误:总加和校验失败");
			break;
		case ERR_METERTYPE:
			fprintf(stderr,"\n错误:仪表类型不支持");
			break;
		case ERR_MONTH:
			fprintf(stderr,"\n错误:错误历史月");
			break;
		default:
			{
				fprintf(stderr,"\nADDR:%02x%02x%02x%02x%02x%02x%02x DI:%02x%02x	\nData[%d]:",
						frame->Addr[6],frame->Addr[5],frame->Addr[4],frame->Addr[3],
						frame->Addr[2],frame->Addr[1],frame->Addr[0],
						frame->DI[1],frame->DI[0],frame->Length);
				for(;k<frame->Length;k++)
					fprintf(stderr," %02x",frame->Data[k]);
			}
	}
	fprintf(stderr,"\n\n");
}
void BuildReadMeterFrame(MeterType meter,cj188_Frame *askframe)
{
	INT8U ditmp[2]={0x1f,0x90};//测试用数据项
	INT8U addrtmp[7]={0x58,0x12,0x01,0x08,0x16,0x33,0x78};//测试用表地址

	memcpy(meter.Addr,addrtmp,7);
	meter.MpType = 0x10;

	memcpy(askframe->Addr,meter.Addr,7);				//表地址
	memcpy(askframe->DI,ditmp,2);						//数据项
	askframe->Ctrl = 0x01;								//控制码:读数据
	askframe->MeterType = meter.MpType;					//表类型:冷水水表
	askframe->SER = ++meter_serno;						//序列号
	fprintf(stderr,"\nmeter_serno=%d",meter_serno);

}
INT8U JugeDataType()
{
return 0;
}
int main(int argc, char *argv[])
{
	int framelength=0;
	int ret=0;
	struct DataNode NodeTmp;
	struct DataNode* DataHead;
	MeterType MeterTmp={};
	int	Meter_index=0;
	INT8U	DataType=0;

	cj188_Frame cj188_ask={};
	cj188_Frame cj188_echo={};
	cj188_Para  cj188_para={};

	int comfd=0,len=0;
	struct sigaction sa={};
	if(InitPro(&JProgramInfo,argc,argv)==0)
	{
		fprintf(stderr,"进程 %s 参数错误",argv[0]);
		return EXIT_FAILURE;
	}

	Setsig(&sa,QuitProcess);
	deal_step = 0;
	time_t timeout=0;
	OldType = 1;
	meter_serno = 0;

	comfd = OpenCom(Port,2400,(unsigned char *)"even",1,8);
	while(1){
		JProgramInfo->Projects[ProIndex].WaitTimes = 0;

		/*抄表时段及流程判断*/
		DataType = 0;
		if(DataType!=OldType)
			Meter_index = 0;

		if(DataType== -1)
		{
			sleep(1);
			continue;
		}
		/*数据项组织过程*/
		if( IfTimeout(timeout,10) ==1 )
		{
//			if (GetMeterData_Record(&NodeTmp,DataType)==1)
			{
				GetMeter_Info(2 , &MeterTmp );

				BuildReadMeterFrame(MeterTmp,&cj188_ask);
				framelength = cj188_ComposeFrame(cj188_ask,cj188_para,SendBuf);
				SendPro(comfd,SendBuf,framelength);
			}
			timeout = time(NULL);
		}

		/*接收和处理过程*/
		RecvPro(comfd,RecBuf,&RHead);
		len = cj188_PreProcess(&deal_step,&rev_delay,10,&RTail,&RHead,RecBuf,DealBuf);
		if(len > 0)
		{
			ret = cj188_parse(&cj188_echo,DealBuf,len);
			memset(DealBuf,0,sizeof(DealBuf));
			PrtEchoInfo(&cj188_echo,ret);
			timeout = 0;
			sleep(2);
		}
		delay(50);

	}
	QuitProcess(&JProgramInfo->Projects[ProIndex]);
 	return EXIT_SUCCESS;
}
