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
	int index=0, hcsi=0,temindex=0;
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
	INT8U sendbuf_report[300],tem_buf[100];
	index = FrameHead(&csinfo,sendbuf_report);
	hcsi = index;  //两字节校验
	index = index + 2;
	apduplace = index;		//记录APDU 起始位置
	tem_buf[temindex++] = REPORT_NOTIFICATION;
	tem_buf[temindex++] = REPORTNOTIFICATIONLIST;
	tem_buf[temindex++] = 0B10000000;	//	piid
	tem_buf[temindex++] = 1; //个数
	tem_buf[temindex++] = dtoad;//OAD数据
	OAD oad;
	oad.OI=report_event.oi;
	oad.attflg=2;
	oad.attrindex=0;
	tem_buf[temindex++] = oad.OI&0x00ff;
	tem_buf[temindex++] = ((oad.OI>>8)&0x00ff);
	tem_buf[temindex++] = oad.attflg;
	tem_buf[temindex++] = oad.attrindex;

	INT8U *data=NULL;
	int datalen=0;
	if(Get_Event(oad,1,&data,&datalen,(ProgramInfo*)com->shmem) == 1){
		if(data!=NULL && datalen>0 && datalen<256){
			tem_buf[temindex++] = 1; //data
			memcpy(&tem_buf[temindex],data,datalen);
			temindex +=datalen;
		}else
			tem_buf[temindex++] = 0; //data
	}
	if (data!=NULL)
		free(data);

	if(com->f101.active == 1){
		sendbuf_report[index++]=16; //SECURIGY-REQUEST
		sendbuf_report[index++]=0;  //明文应用数据单元
		INT16U seclen=composeAutoReport(tem_buf,temindex);
		if(seclen>0){
			memcpy(&sendbuf_report[index],tem_buf,seclen);
			index +=seclen;
		}
	}else{
       memcpy(&sendbuf_report[index],tem_buf,temindex);
       index +=temindex;
	}
	sendbuf_report[index++] = 0;	//跟随上报信息域 	FollowReport
	sendbuf_report[index++] = 0;	//时间标签		TimeTag
	FrameTail(sendbuf_report,index,hcsi);
	if(pSendfun_report!=NULL)
		pSendfun_report(com->phy_connect_fd,sendbuf_report,index+3);
	return (index+3);
}



