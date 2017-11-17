/*
 * class12.h
 *
 *  Created on: 2017-7-3
 *      Author: zhoulihai
 */

#include "../include/StdDataType.h"
#include "../include/Objectdef.h"

#ifndef CLASS12_H_
#define CLASS12_H_

int class12_set(OAD oad, INT8U *data, INT8U *DAR);
int class12_get(OAD oad, INT8U *sourcebuf, INT8U *buf, int *len);
int class12_router(OAD oad, INT8U *data, int *datalen, INT8U *DAR);

int get_Scaler_Unit(OAD oad,Scaler_Unit *su, INT8U *buf, int *len);
int set_Scaler_Unit(OAD oad,INT8U *data, INT8U *DAR, CLASS12 *memClass12,CLASS23 *memClass23);
#endif /* CLASS12_H_ */
