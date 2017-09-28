//
// Created by 周立海 on 2017/4/21.
//

#include "../include/StdDataType.h"

#ifndef INC_698_CLASS23_H
#define INC_698_CLASS23_H

int class23_selector(int index, int attr_act, INT8U *data,Action_result *act_ret);

int class23_act1(int index);
int class23_act3(int index, INT8U* data);

int class23_get(OAD oad, INT8U *sourcebuf, INT8U *buf, int *len);
int class23_set(OAD oad, INT8U *data, INT8U *DAR);

#endif //INC_698_CLASS23_H
