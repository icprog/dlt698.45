/*
 * ObjectSet.c
 *
 *  Created on: Nov 12, 2016
 *      Author: ava
 */

#include <string.h>
#include <stdio.h>

#include "AccessFun.h"
#include "StdDataType.h"
#include "Objectdef.h"
int setRequestNormal(INT8U *Object,CSINFO *csinfo,INT8U *buf)
{
	int bytes=0;
	OAD oad={};//对象属性描述符
	memcpy(&oad , Object,4);
//	bytes = setOneObject(&oad,csinfo,buf);
	return bytes;
}
int setRequestNormalList(INT8U *Object,CSINFO *csinfo,INT8U *buf)
{
	int stepsize=1 , i=0 , bytes=0, objbytes=0;
	INT8U objectnum = Object[0];

	for(i=0 ; i< objectnum ; i++)
	{
		objbytes = setRequestNormal(Object+stepsize,csinfo,buf);
		if (objbytes >0)
			stepsize = stepsize + objbytes;
		else
			break;
	}
	return bytes;
}



