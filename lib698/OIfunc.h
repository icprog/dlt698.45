/*
 * OIfunc.h
 *
 *  Created on: Jun 3, 2017
 *      Author: lhl
 */

#ifndef OIFUNC_H_
#define OIFUNC_H_

#include "Objectdef.h"
#include "Shmem.h"

extern int Set_4000(INT8U *data,INT8U *DAR);
/*----------------------变量类----------------------*/
extern INT8U Get_213x(OAD oad,INT8U *sourcebuf,INT8U *buf,int *len);
extern INT8U Get_2200(OI_698 oi, INT8U* sourcebuf, INT8U* buf, int* len);
extern INT8U Get_2203(OI_698 oi, INT8U* sourcebuf, INT8U* buf, int* len);
extern INT8U Get_2204(OI_698 oi, INT8U* sourcebuf, INT8U* buf, int* len);

/*----------------------参变量类----------------------*/
extern int Get_4000(OAD oad,INT8U *data);

/*----------------------采集监控类----------------------*/
extern int Get_6001(INT8U type,INT16U seqnum, INT8U* data);
extern int Get_6013(INT8U type,INT8U taskid,INT8U *data);
extern int Get_6015(INT8U type,INT8U seqnum, INT8U* data);
extern int Get_6017(INT8U type,INT8U seqnum,INT8U *data);
extern int Get_6035(INT8U type,INT8U seqnum, INT8U* data);
extern int Get_601D(INT8U type,INT8U seqnum,INT8U *data);

#endif /* OIFUNC_H_ */
