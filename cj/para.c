/*
 * para.c
 *
 *  Created on: Jan 5, 2017
 *      Author: admin
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "AccessFun.h"
#include "StdDataType.h"
#include "Objectdef.h"

void para_process(int argc, char *argv[])
{
	int 	tmp=0;
	OI_698	oi=0;
	int 	method=0;

	if(argc==5) {	//para
		if(strcmp(argv[1],"para")==0) {
			if(strcmp(argv[2],"method")==0) {
				sscanf(argv[3],"%04x",&tmp);
				oi = tmp;
				sscanf(argv[4],"%d",&tmp);
				method = tmp;
				switch(oi) {
				case 0x4300:
					switch(method) {
					case 3:		//数据区初始化
					case 5:		//事件初始化
					case 6:		//需量初始化
						dataInit(method);
						break;
					}
					break;
				}
			}
		}
	}
}

