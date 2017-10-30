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
#include "EventObject.h"
#include "secure.h"
#include "event.h"
extern int FrameHead(CSINFO *csinfo,INT8U *buf);
extern void FrameTail(INT8U *buf,int index,int hcsi);
extern INT8U Get_Event(OAD oad,INT8U eventno,INT8U** Getbuf,int *Getlen,ProgramInfo* prginfo_event);
INT8S (*pSendfun_report)(int fd,INT8U* sndbuf,INT16U sndlen);


INT8U Report_Event(CommBlock *com,INT8U *oiarr,INT8U report_type){

	if(oiarr == NULL)
		return 0;
	int apduplace =0;
	int index=0, hcsi=0,temindex=0,i=0;
	static INT8U	piid=0;
	pSendfun_report = com->p_send;
	OI_698 oi=((oiarr[1]<<8)+oiarr[0]);
	CSINFO csinfo;
	csinfo.dir = 1;
	csinfo.prm = 0;
	csinfo.funcode = 0x03;
	csinfo.gframeflg = 0;
	csinfo.sa_type = 0;
	INT8U sa_len=com->serveraddr[0];
	for(i=0;i<sa_len;i++) {
		csinfo.sa[i] = com->serveraddr[sa_len-i];
	}
	csinfo.ca = com->taskaddr;
    csinfo.sa_length = sa_len;
	INT8U sendbuf_report[300],tem_buf[300];
	index = FrameHead(&csinfo,sendbuf_report);
	hcsi = index;  //两字节校验
	index = index + 2;
	apduplace = index;		//记录APDU 起始位置
	OAD oad;
	oad.OI=oi;
	oad.attflg=2;
	oad.attrindex=0;
	piid++;
	com->report_piid[0] = piid;		//处理无应答重复上报判断报文帧，目前未考虑多通道在线情况
	if(report_type == 1){
		tem_buf[temindex++] = REPORT_NOTIFICATION;
		tem_buf[temindex++] = REPORTNOTIFICATIONLIST;
		tem_buf[temindex++] = piid;//0B10000000;	//	piid
		tem_buf[temindex++] = 1; //个数
		tem_buf[temindex++] = dtoad;//OAD数据

		tem_buf[temindex++] = ((oad.OI>>8)&0x00ff);
		tem_buf[temindex++] = oad.OI&0x00ff;
		tem_buf[temindex++] = oad.attflg;
		tem_buf[temindex++] = oad.attrindex;
	}else{
		tem_buf[temindex++] = REPORT_NOTIFICATION;
	    tem_buf[temindex++] = REPROTNOTIFICATIONRECORDLIST;
	    tem_buf[temindex++] = piid;//0x02;
	    tem_buf[temindex++] = 0x01;
	    tem_buf[temindex++] = ((oad.OI>>8)&0x00ff);
	    tem_buf[temindex++] = oad.OI&0x00ff;
		tem_buf[temindex++] = oad.attflg;
		tem_buf[temindex++] = oad.attrindex;

		INT8U oadnumindex=temindex;
	    tem_buf[temindex++] = 0x00; //OAD数量

		INT8U in=0,_in=0;
		for(in=0;in<sizeof(guanlian_oad)/sizeof(event_guanlian);in++){
            if(guanlian_oad[in].oi == oad.OI){
            	fprintf(stderr,"\\\\num=%d \n",guanlian_oad[in].num);
            	tem_buf[oadnumindex]+=guanlian_oad[in].num;
            	for(_in=0;_in<guanlian_oad[in].num;_in++){
            		tem_buf[temindex++] = 0x00;
            		tem_buf[temindex++] = (guanlian_oad[in].oad[_in].OI>>8)&0x00ff;
					tem_buf[temindex++] = guanlian_oad[in].oad[_in].OI&0x00ff;
					tem_buf[temindex++] = guanlian_oad[in].oad[_in].attflg;
					tem_buf[temindex++] = guanlian_oad[in].oad[_in].attrindex;
            	}
            	break;
            }
		}
	}
	INT8U *data=NULL;
	int datalen=0;
	if(Get_Event(oad,1,&data,&datalen,(ProgramInfo*)com->shmem) == 1){
		if(data!=NULL && datalen>0 && datalen<256){
			tem_buf[temindex++] = 1; //data
			if(report_type == 1){
				memcpy(&tem_buf[temindex],data,datalen);
				temindex +=datalen;
			}else{
				tem_buf[temindex++] = 1; //1个
				memcpy(&tem_buf[temindex],&data[STANDARD_NO_INDEX],datalen-2);//事件序号以后得数据
				temindex +=datalen-2;
			}
		}else
			tem_buf[temindex++] = 0; //data
	}


	if(com->f101.active == 1){
		sendbuf_report[index++]=16; //SECURIGY-REQUEST
		sendbuf_report[index++]=0;  //明文应用数据单元
		tem_buf[temindex++] = 0;	//跟随上报信息域 	FollowReport
		tem_buf[temindex++] = 0;	//时间标签		TimeTag
		INT16U seclen=composeAutoReport(tem_buf,temindex);
		if(seclen>0){
			memcpy(&sendbuf_report[index],tem_buf,seclen);
			index +=seclen;
		}
	}else{
       memcpy(&sendbuf_report[index],tem_buf,temindex);
       index +=temindex;
       sendbuf_report[index++] = 0;	//跟随上报信息域 	FollowReport
       sendbuf_report[index++] = 0;	//时间标签		TimeTag
	}
	FrameTail(sendbuf_report,index,hcsi);
	if(pSendfun_report!=NULL)
		pSendfun_report(com->phy_connect_fd,sendbuf_report,index+3);
	if (data!=NULL)
		free(data);

	return (index+3);
}

/*
 * 国网要求：采集终端互换性测试说明：转发主站直接对电能表的批量抄读数据命令接口
 *
 *  返回 : piid   =-1，错误
 */
int callNotificationReport(CommBlock* com,INT8U *plcbuf,OAD portOAD,int datalen)
{
	fprintf(stderr, "callNotificationReport\n");
	INT8U *sendbuf = com->SendBuf;
	static INT8U  piid=0;
	int 	index=0,hcsi=0,apduplace=0;
	CSINFO csinfo={};

	if ((com==NULL) || (datalen==0))
		return -1;
	if (fillcsinfo(&csinfo,com->serveraddr,com->taskaddr)==0)
		return 0;
	piid++;
	com->report_piid[0] = piid;		//处理无应答重复上报判断报文帧，目前未考虑多通道在线情况
	index = 0;
	index = FrameHead(&csinfo,sendbuf);
	hcsi = index;
	index = index + 2;
	apduplace = index;		//记录APDU 起始位置
	sendbuf[index++] = REPORT_NOTIFICATION;
	sendbuf[index++] = REPORTNOTIFICATIONTRANSDATA;
	sendbuf[index++] = piid;	//PIID
	sendbuf[index++] = (portOAD.OI >> 8) & 0xff;	//数据来源端口号
	sendbuf[index++] = portOAD.OI & 0xff;
	sendbuf[index++] = portOAD.attflg;
	sendbuf[index++] = portOAD.attrindex;
	memcpy(&sendbuf[index],plcbuf,datalen);//将读出的数据拷贝
	index +=datalen;
	sendbuf[index++] = 0;
	sendbuf[index++] = 0;
	FrameTail(sendbuf,index,hcsi);
	if(com->p_send!=NULL)
		com->p_send(com->name, com->phy_connect_fd,sendbuf,index+3);  //+3:crc1,crc2,0x16
	return piid;
}

