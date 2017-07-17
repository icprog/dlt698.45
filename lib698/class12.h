/*
 * class12.h
 *
 *  Created on: 2017-7-3
 *      Author: zhoulihai
 */

#include "../include/StdDataType.h"

#ifndef CLASS12_H_
#define CLASS12_H_

int class12_act3(int index, INT8U* data);
int class12_act4(int index, INT8U* data);

int class12_router(int index, int attr_act, INT8U *data,
		Action_result *act_ret);

int class2401_set(int index, OAD oad, INT8U *data, INT8U *DAR);
int class2401_set_attr_2(int index, OAD oad, INT8U *data, INT8U *DAR);
int class2401_set_attr_3(int index, OAD oad, INT8U *data, INT8U *DAR);

int class12_get(OAD oad, INT8U *sourcebuf, INT8U *buf, int *len);

#endif /* CLASS12_H_ */
