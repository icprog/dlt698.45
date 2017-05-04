/*
 * state.c
 *
 *  Created on: Feb 8, 2017
 *      Author: ava
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "AccessFun.h"
#include "StdDataType.h"
#include "Objectdef.h"
#include "PublicFunction.h"
#include "event.h"

static CLASS_f203	oif203={};

typedef enum {STATE1=1,STATE2=2,STATE3=3,STATE4=4,PLUSE1=5,PLUSE2=6} DEV_STATE_PULSE;

INT32S state_get(DEV_STATE_PULSE road)
{
	INT32S	staval=-1;
	unsigned int pluse_tmp[2]={};
	int fd  = 0;

	switch(road)
	{
	case STATE1:
		staval = gpio_readbyte(DEV_STATE1);
		break;
	case STATE2:
		staval = gpio_readbyte(DEV_STATE2);
		break;
	case STATE3:
		staval = gpio_readbyte(DEV_STATE3);
		break;
	case STATE4:
		staval = gpio_readbyte(DEV_STATE4);
		break;
	case PLUSE1:
	    if ((fd = open(DEV_PULSE, O_RDWR | O_NDELAY)) >= 0) {
	        read(fd, &pluse_tmp, 2*sizeof(unsigned int));
	        close(fd);
	        return pluse_tmp[0];
	    } else {
	    	syslog(LOG_ERR,"%s %s fd=%d(PLUSE1)\n",__func__,DEV_PULSE,fd);
	        return -1;
	    }
		break;
	case PLUSE2:
	    if ((fd = open(DEV_PULSE, O_RDWR | O_NDELAY)) >= 0) {
	        read(fd, &pluse_tmp, 2*sizeof(unsigned int));
	        close(fd);
	        return pluse_tmp[1];
	    } else {
	    	syslog(LOG_ERR,"%s %s fd=%d(PLUSE2)\n",__func__,DEV_PULSE,fd);
	        return -1;
	    }
		break;
	}
	return staval;
}

void read_oif203_para()
{
	int	readret = 0;
	memset(&oif203,0,sizeof(oif203));
	readret = readCoverClass(0xf203,0,&oif203,sizeof(CLASS_f203),para_vari_save);
	fprintf(stderr,"f203属性4：接入标志=%02x\n",oif203.state4.StateAcessFlag);
	fprintf(stderr,"f203属性4：属性标志=%02x\n",oif203.state4.StatePropFlag);
}

//检测F203参数是否变化
BOOLEAN oi_f203_changed(INT8U save_changed)
{
	INT8U i=0;
	static INT8U changed = 0xff;
	if(changed != 0 && changed != save_changed)
	{
		read_oif203_para();
		for(i=0; i < STATE_MAXNUM;i++)
		{
			if(((oif203.state4.StateAcessFlag>>(STATE_MAXNUM-1-i))&0x01) ==1)
			{
				if(((oif203.state4.StatePropFlag>>(STATE_MAXNUM-1-i))&0x01) ==0 )	//常闭节点
					oif203.statearri.stateunit[i].ST = 1;
				else
					oif203.statearri.stateunit[i].ST = 0;
			}else {
				oif203.statearri.stateunit[i].ST = 0;
			}
			oif203.statearri.stateunit[i].CD = 0;		//参数变化后，清除状态变位标志
		}
		//将此时遥信状态保存，规约招测
		saveCoverClass(0xf203,0,&oif203,sizeof(CLASS_f203),para_vari_save);
		changed = save_changed;
		fprintf(stderr,"CD=%d-%d-%d-%d-%d-%d-%d-%d\n",
				oif203.statearri.stateunit[0].CD,oif203.statearri.stateunit[1].CD,oif203.statearri.stateunit[2].CD,oif203.statearri.stateunit[3].CD,
				oif203.statearri.stateunit[4].CD,oif203.statearri.stateunit[5].CD,oif203.statearri.stateunit[6].CD,oif203.statearri.stateunit[7].CD);
		fprintf(stderr,"ST=%d-%d-%d-%d-%d-%d-%d-%d\n",
				oif203.statearri.stateunit[0].ST,oif203.statearri.stateunit[1].ST,oif203.statearri.stateunit[2].ST,oif203.statearri.stateunit[3].ST,
				oif203.statearri.stateunit[4].ST,oif203.statearri.stateunit[5].ST,oif203.statearri.stateunit[6].ST,oif203.statearri.stateunit[7].ST);
		return TRUE;
	}
	return FALSE;
}


/*
 * 开关量采集
 * 返回 =1：有状态变位
 *     =0 :无变位
 * */
INT8U state_check(BOOLEAN changed,INT8U devicetype)
{
	INT8U	staret=0;
	INT8U 	i =0;
	INT8U 	bit_state[STATE_MAXNUM]={};
	INT8S	readstate[STATE_MAXNUM]={};	//读取开关量状态

	memset(bit_state,0,sizeof(bit_state));
	for(i=0; i < STATE_MAXNUM; i++)
	{
		if(((oif203.state4.StateAcessFlag >>(STATE_MAXNUM-1-i))&0x01) ==1) {
			if(i>=0 && i<=3) {		//YX1-YX4
				readstate[i] = state_get(i+1);
				if(readstate[i]!=-1) {
					bit_state[i] = (~(readstate[i])) & 0x01;
				}else {
					bit_state[i] = bit_state[0];			//II型无设备，台体测试测试1-4路状态
				}
			}else if(i==4) {		//门节点
				if(devicetype==1) {		//I型集中器
					readstate[i] = getSpiAnalogState();
					if(readstate[i]!=-1) {
						bit_state[i] = ((~(readstate[i]>>5))&0x01);
					}
				}
			}
			if(((oif203.state4.StatePropFlag>>(STATE_MAXNUM-1-i))&0x01)==0){	//常闭
				bit_state[i] = (~bit_state[i])&0x01;
			}
		}
	}
	for(i=0; i < STATE_MAXNUM; i++)
	{
		if((changed == FALSE) && (bit_state[i] != oif203.statearri.stateunit[i].ST)) {
			oif203.statearri.stateunit[i].ST = bit_state[i];
			oif203.statearri.stateunit[i].CD = 1;
			staret = 1;
			fprintf(stderr,"RD=%d_%d_%d_%d %d_%d_%d_%d\n",bit_state[0],bit_state[1],bit_state[2],bit_state[3],bit_state[4],bit_state[5],bit_state[6],bit_state[7]);
			fprintf(stderr,"ST=%d_%d_%d_%d %d_%d_%d_%d\n",oif203.statearri.stateunit[0].ST,oif203.statearri.stateunit[1].ST,oif203.statearri.stateunit[2].ST,oif203.statearri.stateunit[3].ST,oif203.statearri.stateunit[4].ST,oif203.statearri.stateunit[5].ST,oif203.statearri.stateunit[6].ST,oif203.statearri.stateunit[7].ST);
			fprintf(stderr,"CD=%d_%d_%d_%d %d_%d_%d_%d\n\n",oif203.statearri.stateunit[0].CD,oif203.statearri.stateunit[1].CD,oif203.statearri.stateunit[2].CD,oif203.statearri.stateunit[3].CD,oif203.statearri.stateunit[4].CD,oif203.statearri.stateunit[5].CD,oif203.statearri.stateunit[6].CD,oif203.statearri.stateunit[7].CD);
		}
	}
	return staret;
}

void getStateEvent(ProgramInfo* prginfo)
{
	INT8U	data[STATE_MAXNUM]={};
	INT8U	i=0;

	for(i=0; i < STATE_MAXNUM/2; i++){
		data[i*2+0]=oif203.statearri.stateunit[i].ST;
		data[i*2+1]=oif203.statearri.stateunit[i].CD;
	}
	Event_3104(data,STATE_MAXNUM,prginfo);
}

/*
 * 开关量状态处理
 * */
void DealState(ProgramInfo* prginfo)
{
	BOOLEAN changed = FALSE;
	INT8U	stachg = 0;
	changed = oi_f203_changed(prginfo->oi_changed.oiF203);
	stachg = state_check(changed,prginfo->cfg_para.device);
	if(stachg==1) {
		getStateEvent(prginfo);
		saveCoverClass(0xf203,0,&oif203,sizeof(CLASS_f203),para_vari_save);
	}
}
