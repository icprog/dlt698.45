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
typedef struct
{
	TSA tsa;			//目标地址
	INT16U onetimeout;	//一个服务器的超时时间
	INT16U num;			//oad的个数
	OAD oads[10];		//num个对象描述
}GETOBJS;
typedef struct
{
	INT16U timeout;	//代理超时时间
	INT16U num;		//个数
	GETOBJS objs[10];//代理请求列表
	INT16U len;
	INT8U *data;
}PROXY_GETLIST;

int getProxylist(INT8U *data,PROXY_GETLIST *getlist)
{
	int i=0,k=0, iindex=0;
	INT8U num=0,oadnum=0;
	num = data[iindex++];

	getlist->num = num;
	for(i=0;i<num;i++)
	{
		num = data[iindex];
		memcpy(&getlist->objs[i].tsa,&data[iindex],num);
		iindex++;
		getlist->objs[i].onetimeout = data[iindex]<<8 + data[iindex+1];
		iindex = iindex + 2;
		oadnum = data[iindex++];
		for(k=0; k<oadnum; k++)
		{
			getlist->objs[i].oads[k].OI = data[iindex]<<8 | data[iindex+1];
			getlist->objs[i].oads[k].attflg = data[iindex+2];
			getlist->objs[i].oads[k].attrindex = data[iindex+3];
			iindex = iindex + 4;
		}
	}
	return iindex;
}
int doProxyGetResponselist(PROXY_GETLIST *list)
{
	INT16U timeout = list->timeout;
	do{

	}while(timeout--);
	return 1;
}
int Proxy_GetRequestlist(INT8U *data,CSINFO *csinfo,INT8U *sendbuf)
{
	INT16U timeout=0 ;
	int i=0;
	PROXY_GETLIST getlist;
	timeout = data[0]>>8 | data[1];
	getlist.timeout = timeout;
//	memset(TmpDataBuf,0,sizeof(TmpDataBuf));
	getProxylist(&data[2],&getlist);
	//	SMode_OADListGetClass
//	doProxyGetlist(&getlist);
	doProxyGetResponselist(&getlist);
//	BuildFrame_GetResponse(GET_REQUEST_NORMAL,csinfo,response,sendbuf);
//	securetype = 0;		//清除安全等级标识

	return 1;
}
