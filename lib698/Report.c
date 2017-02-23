/*
 * Report.c
 *
 *  Created on: 2017-2-23
 *      Author: wzm
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "AccessFun.h"
#include "StdDataType.h"
#include "Objectdef.h"
#include "dlt698def.h"
#include "dlt698.h"
#include "PublicFunction.h"
#include "secure.h"
#include "event.h"
extern int FrameHead(CSINFO *csinfo,INT8U *buf);
extern void FrameTail(INT8U *buf,int index,int hcsi);
extern INT8U Get_Event(OAD oad,INT8U eventno,INT8U** Getbuf,int *Getlen,ProgramInfo* prginfo_event);
INT8S (*pSendfun_report)(int fd,INT8U* sndbuf,INT16U sndlen);

INT8U Report_Event(CommBlock *com,Reportevent report_event){

	int apduplace =0;
	int index=0, hcsi=0;
	pSendfun_report = com->p_send;
	CSINFO csinfo;
	csinfo.dir = 1;
	csinfo.prm = 0;
	csinfo.funcode = 0x03;
	csinfo.gframeflg = 0;
	csinfo.sa_type = 0;
	INT8U sa_len=com->serveraddr[0]+1;
	memcpy(csinfo.sa,&com->serveraddr[1],sa_len);
	csinfo.ca = com->taskaddr;
    csinfo.sa_length = sa_len;
	INT8U sendbuf_report[256];
	index = FrameHead(&csinfo,sendbuf_report);
	hcsi = index;
	index = index + 2;

	apduplace = index;		//记录APDU 起始位置
	sendbuf_report[index++] = REPORT_NOTIFICATION;
	sendbuf_report[index++] = REPORTNOTIFICATIONLIST;
	sendbuf_report[index++] = 0B10000000;	//	piid
	sendbuf_report[index++] = 1; //个数
	sendbuf_report[index++] = dtoad;//OAD数据
	OAD oad;
	oad.OI=report_event.oi;
	oad.attflg=2;
	oad.attrindex=0;
	sendbuf_report[index++] = oad.OI&0x00ff;
	sendbuf_report[index++] = ((oad.OI>>8)&0x00ff);
	sendbuf_report[index++] = oad.attflg;
	sendbuf_report[index++] = oad.attrindex;

	INT8U *data=NULL;
	int datalen=0;
	if(Get_Event(oad,1,&data,&datalen,(ProgramInfo*)com->shmem) == 1){
		if(data!=NULL && datalen>0 && datalen<256){
			memcpy(&sendbuf_report[index],data,datalen);
			sendbuf_report[index++] = 1; //data
		}else
			sendbuf_report[index++] = 0; //data
	}
	if (data!=NULL)
		free(data);
	sendbuf_report[index++] = 0;	//跟随上报信息域 	FollowReport
	sendbuf_report[index++] = 0;	//时间标签		TimeTag
	if(com->securetype!=0)//安全等级类型不为0，代表是通过安全传输下发报文，上行报文需要以不低于请求的安全级别回复
	{
		apduplace += composeSecurityResponse(&sendbuf_report[apduplace],index-apduplace);
		index=apduplace;
	}
	FrameTail(sendbuf_report,index,hcsi);
	if(pSendfun_report!=NULL)
		pSendfun_report(com->phy_connect_fd,sendbuf_report,index+3);
	return (index+3);
}



