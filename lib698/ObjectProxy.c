/*
 * ObjectProxy.c
 *
 *  Created on: Nov 12, 2016
 *      Author: ava
 */

#include <string.h>
#include <stdio.h>
#include <malloc.h>

#include "AccessFun.h"
#include "StdDataType.h"
#include "Objectdef.h"
#include "PublicFunction.h"
#include "dlt698.h"
#include "dlt698def.h"
#include "../libMq/libmmq.h"

extern ProgramInfo *memp;
extern void getoad(INT8U *data,OAD *oad);
extern int FrameHead(CSINFO *csinfo,INT8U *buf);
extern void FrameTail(INT8U *buf,int index,int hcsi);

void ProxyListResponse(PROXY_GETLIST *list,CommBlock *com)
{
	if (com==NULL || list==NULL)
		return;
	CSINFO csinfo;
	INT8U *sendbuf = com->SendBuf;
	int index=0, hcsi=0,datalen=0 ,apduplace =0;

	memcpy(&csinfo,&list->csinfo,sizeof(CSINFO));
	csinfo.dir = 1;
	csinfo.prm = 1;

	index = FrameHead(&csinfo,sendbuf);
	hcsi = index;
	index = index + 2;

	apduplace = index;		//记录APDU 起始位置
	sendbuf[index++] = PROXY_RESPONSE;
	sendbuf[index++] = list->proxytype;
	sendbuf[index++] = list->piid;
	datalen = list->datalen ;
	if (datalen > 512)
		datalen =512;
	memcpy(&sendbuf[index],list->data,datalen);
	index = index + datalen;
	sendbuf[index++] = 0;
	sendbuf[index++] = 0;
	FrameTail(sendbuf,index,hcsi);
	if(com->p_send!=NULL)
		com->p_send(com->phy_connect_fd,sendbuf,index+3);
}

int getProxylist(INT8U *data,PROXY_GETLIST *getlist)
{
	int i=0,k=0, iindex=0;
	INT8U num=0,oadnum=0;
	INT16U timeout=0;
	OAD oadtmp;

	getlist->num = data[iindex++];// sequence of 代理
	fprintf(stderr,"\n---%d",getlist->num);
	for(i=0;i<getlist->num;i++)
	{
		num = data[iindex];
		if (num>sizeof(getlist->proxy_obj.objs[i].tsa))
			num = sizeof(getlist->proxy_obj.objs[i].tsa);
		memcpy(&getlist->proxy_obj.objs[i].tsa,&data[iindex],num+1);
		iindex = iindex + num +1;
		timeout = data[iindex];
		getlist->proxy_obj.objs[i].onetimeout = timeout<<8 |data[iindex+1];
		iindex = iindex + 2;
		oadnum = data[iindex++];
		getlist->proxy_obj.objs[i].num = oadnum;
		for(k=0; k<oadnum; k++)
		{
			getOAD(0,&data[iindex],&oadtmp,NULL);
			memcpy(&getlist->proxy_obj.objs[i].oads[k],&oadtmp,sizeof(oadtmp));
			iindex = iindex + 4;
		}
	}
	return iindex;
}
int Proxy_GetRequestlist(INT8U *data,CSINFO *csinfo,INT8U *sendbuf,INT8U piid)
{
	INT16U timeout=0 ;
	int i=0,j=0;
	PROXY_GETLIST getlist;
	INT8S	ret=0;

	timeout = data[0] ;
	timeout = timeout <<8 | data[1];

	getlist.timeout = timeout;
	getlist.piid = piid;
	getlist.proxytype = ProxyGetRequestList;
	getProxylist(&data[2],&getlist);
	fprintf(stderr,"\nProxy_GetRequestlist, timeout =%d  代理的对象属性读取数量 %d",timeout,getlist.num);
	for(i=0;i<getlist.num;i++)
	{
		if (getlist.num>10) break;
		fprintf(stderr,"\n第%d组代理对象",i);
		for(j=0; j<getlist.proxy_obj.objs[i].num; j++)
		{
			fprintf(stderr,"\n%04x %02x %02x",getlist.proxy_obj.objs[i].oads[j].OI,getlist.proxy_obj.objs[i].oads[j].attflg,getlist.proxy_obj.objs[i].oads[j].attrindex);
		}
	}
	//写入文件，等待转发			规约中只负责解析代理的内容，并追加写入到代理文件 /nand/proxy_list
	getlist.timeold = time(NULL);
	memcpy(&getlist.csinfo,csinfo,sizeof(CSINFO));

	ret= mqs_send((INT8S *)PROXY_485_MQ_NAME,1,ProxyGetResponseList,(INT8U *)&getlist,sizeof(PROXY_GETLIST));
	fprintf(stderr,"\n代理消息已经发出,ret=%d ,getlist_len=%d\n\n",ret,sizeof(PROXY_GETLIST));
	return 1;
}

int Proxy_GetRequestRecord(INT8U *data,CSINFO *csinfo,INT8U *sendbuf,INT8U piid)
{
	INT8U num=0;
	INT16U timeout=0 ;
	int iindex=0,rsdlen=0;
	PROXY_GETLIST getlist;
	INT8S	ret=0;
	OAD oadtmp;
	RESULT_RECORD record={};

	timeout = data[0] ;
	timeout = timeout <<8 | data[1];

	getlist.timeout = timeout;
	getlist.piid = piid;
	getlist.proxytype = ProxyGetRequestRecord;
	iindex = 2;
	num = data[iindex];
	if (num>sizeof(TSA))
		num = sizeof(TSA);
	memcpy(&getlist.proxy_obj.record.tsa,&data[iindex],num+1);
	iindex = iindex + num +1;
	getOAD(0,&data[iindex],&oadtmp,NULL);
	memcpy(&getlist.proxy_obj.record.oad,&oadtmp,sizeof(oadtmp));
	iindex = iindex + 4;

	rsdlen = get_BasicRSD(0,&data[iindex],(INT8U *)&record.select,&record.selectType);
	getlist.proxy_obj.record.selectbuf.len  = rsdlen;
	getlist.proxy_obj.record.selectbuf.type = record.selectType;
	if (rsdlen<=512)
		memcpy(getlist.proxy_obj.record.selectbuf.buf,&record.select,rsdlen);
	iindex  += rsdlen;
	iindex  += get_BasicRCSD(0,&data[iindex],&getlist.proxy_obj.record.rcsd.csds);

	//写入文件，等待转发			规约中只负责解析代理的内容，并追加写入到代理文件 /nand/proxy_list
	getlist.timeold = time(NULL);
	memcpy(&getlist.csinfo,csinfo,sizeof(CSINFO));

	ret= mqs_send((INT8S *)PROXY_485_MQ_NAME,1,ProxyGetResponseRecord,(INT8U *)&getlist,sizeof(PROXY_GETLIST));
	fprintf(stderr,"\n代理消息已经发出,ret=%d\n\n",ret);
	return 1;
}
int getProxyDO_Then_Get_list(INT8U *data,DO_Then_GET *doget)
{
	int i=0,k=0, iindex=0;
	INT8U num=0,oadnum=0,num_tsa=0;
	INT16U timeout=0;
	OAD oadtmp;
	INT8U	dar=success;
	num_tsa = data[iindex++];// sequence of 代理
	fprintf(stderr,"\nseqOf_TSA---%d",num_tsa);
	for(i=0;i<num_tsa;i++)//TSA 个数
	{
//		num = data[iindex];
//		if (num>sizeof(doget[i].tsa))
//			num = sizeof(doget[i].tsa);
//		memcpy(&doget[i].tsa,&data[iindex],num+1);
//		iindex = iindex + num +1;
		iindex += getOctetstring(0,&data[iindex],(INT8U *)&doget[i].tsa,&dar);
		timeout = data[iindex];
		doget[i].timeout = timeout<<8 |data[iindex+1];
		iindex = iindex + 2;
		oadnum = data[iindex++];
		doget[i].num = oadnum;
		for(k=0; k<oadnum; k++)
		{
			iindex += getOAD(0,&data[iindex],&oadtmp,NULL);
			memcpy(&doget[i].setoads[k].oad_set,&oadtmp,sizeof(oadtmp));
			doget[i].setoads[k].len = get_Data(&data[iindex],doget[i].setoads[k].data);
			iindex += doget[i].setoads[k].len;
			iindex += getOAD(0,&data[iindex],&oadtmp,NULL);
			memcpy(&doget[i].setoads[k].oad_get,&oadtmp,sizeof(oadtmp));
			doget[i].setoads[k].dealy = data[iindex++];
		}
	}
	printProxyDoThenGet(num_tsa,doget);
	return num_tsa;
}

int Proxy_DoThenGetRequestList(INT8U *data,CSINFO *csinfo,INT8U *sendbuf,INT8U piid,INT8U type)
{
	INT16U timeout=0 ;
	PROXY_GETLIST getlist;
	INT8S	ret=0;

	timeout = data[0] ;
	timeout = timeout <<8 | data[1];

	getlist.timeout = timeout;
	getlist.piid = piid;
	getlist.proxytype = type;
	getlist.num = getProxyDO_Then_Get_list(&data[2], getlist.proxy_obj.doTsaThenGet);
	getlist.timeold = time(NULL);

	memcpy(&getlist.csinfo,csinfo,sizeof(CSINFO));

	ret= mqs_send((INT8S *)PROXY_485_MQ_NAME,1,type ,(INT8U *)&getlist,sizeof(PROXY_GETLIST));
	fprintf(stderr,"\n代理消息已经发出,ret=%d ,getlist_len=%d\n\n",ret,sizeof(PROXY_GETLIST));

	return 0;
}
int Proxy_DoRequestList(INT8U *data,CSINFO *csinfo,INT8U *sendbuf,INT8U piid,INT8U type)
{
	INT16U timeout=0 ;
	INT8U num=0;
	int i=0,j=0;
	PROXY_GETLIST getlist;
	OAD oadtmp;
	INT8S	ret=0;
	int iindex=0;

	timeout = data[iindex] ;
	timeout = (timeout <<8) | data[iindex+1];
	iindex = iindex + 2;
	getlist.timeout = timeout;
	getlist.piid = piid;
	getlist.proxytype = type;
	getlist.num = data[iindex++];// sequence of 代理
	for(i=0;i<getlist.num;i++)//TSA 个数
	{
		num = data[iindex];
		if (num > sizeof(getlist.proxy_obj.doTsaList[i].tsa))
			num = sizeof(getlist.proxy_obj.doTsaList[i].tsa);
		memcpy(&getlist.proxy_obj.doTsaList[i].tsa,&data[iindex],num+1);
		iindex = iindex + num +1;
		timeout = data[iindex];
		getlist.proxy_obj.doTsaList[i].timeout = timeout<<8 |data[iindex+1];
		iindex = iindex + 2;
		num = data[iindex++];	//OAD 个数
		getlist.proxy_obj.doTsaList[i].num = num;
		for(j=0; j<num; j++)
		{
			iindex += getOAD(0,&data[iindex],&oadtmp,NULL);
			memcpy(&getlist.proxy_obj.doTsaList[i].setobjs[j],&oadtmp,sizeof(oadtmp));
			getlist.proxy_obj.doTsaList[i].setobjs[j].len = get_Data(&data[iindex],getlist.proxy_obj.doTsaList[i].setobjs[j].data);
			iindex += getlist.proxy_obj.doTsaList[i].setobjs[j].len;
		}
	}
	getlist.timeold = time(NULL);
	memcpy(&getlist.csinfo,csinfo,sizeof(CSINFO));

	ret= mqs_send((INT8S *)PROXY_485_MQ_NAME,1,type,(INT8U *)&getlist,sizeof(PROXY_GETLIST));
	fprintf(stderr,"\n代理消息已经发出,ret=%d\n\n",ret);
	return 1;
}
void printcmd(PROXY_GETLIST getlist)
{
	int i=0;
	fprintf(stderr,"proxytype = %d\n",getlist.proxytype);
	fprintf(stderr,"OAD=%04x-%02x-%02x\n",getlist.proxy_obj.transcmd.oad.OI,getlist.proxy_obj.transcmd.oad.attflg,getlist.proxy_obj.transcmd.oad.attrindex);
	fprintf(stderr,"COMDCB:baud=%d,par=%d,datab=%d,stopb=%d,flow=%d\n",getlist.proxy_obj.transcmd.comdcb.baud,getlist.proxy_obj.transcmd.comdcb.verify,
			getlist.proxy_obj.transcmd.comdcb.databits,getlist.proxy_obj.transcmd.comdcb.stopbits,getlist.proxy_obj.transcmd.comdcb.flow);
	fprintf(stderr,"RevTimeOut=%d,ByteTimeOut=%d\n",getlist.proxy_obj.transcmd.revtimeout,getlist.proxy_obj.transcmd.bytetimeout);
	fprintf(stderr,"autoCmdLen=%d\n",getlist.proxy_obj.transcmd.cmdlen);
	for(i=0;i<getlist.proxy_obj.transcmd.cmdlen;i++) {
		fprintf(stderr,"%02x ",getlist.proxy_obj.transcmd.cmdbuf[i]);
	}
}

int Proxy_TransCommandRequest(INT8U *data,CSINFO *csinfo,INT8U *sendbuf,INT8U piid)
{
	PROXY_GETLIST getlist;
	INT8S	ret=0;
	INT16U	index=0;
	INT8U	DAR=success;
	getlist.piid = piid;
	getlist.proxytype = ProxyTransCommandRequest;
	getlist.proxy_obj.transcmd.dar = other_err1;
	index += getOAD(0,&data[index],&getlist.proxy_obj.transcmd.oad,NULL);
	index += getCOMDCB(0,&data[index],&getlist.proxy_obj.transcmd.comdcb,&DAR);
	getlist.proxy_obj.transcmd.revtimeout = (data[index]<<8) | data[index+1];
	index += 2;
	getlist.proxy_obj.transcmd.bytetimeout = (data[index]<<8) | data[index+1];
	index += 2;
	getlist.proxy_obj.transcmd.cmdlen = data[index++];		//默认长度不超过255
	getlist.timeout = getlist.proxy_obj.transcmd.revtimeout;	//代理超时时间，为了在发送代理消息dealProxyAnswer的时候判断

	memcpy(getlist.proxy_obj.transcmd.cmdbuf,&data[index],getlist.proxy_obj.transcmd.cmdlen);
	//写入文件，等待转发			规约中只负责解析代理的内容，并追加写入到代理文件 /nand/proxy_list
	getlist.timeold = time(NULL);
	memcpy(&getlist.csinfo,csinfo,sizeof(CSINFO));

	printcmd(getlist);
	fprintf(stderr,"代理内容：sizeof(getlist)=%d\n",sizeof(PROXY_GETLIST));

	ret= mqs_send((INT8S *)PROXY_485_MQ_NAME,1,ProxyGetResponseList,(INT8U *)&getlist,sizeof(PROXY_GETLIST));
	fprintf(stderr,"\n代理消息已经发出,ret=%d\n\n",ret);
	return 1;
}
