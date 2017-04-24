//
// Created by 周立海 on 2017/4/21.
//

#include "../include/StdDataType.h"

#ifndef INC_698_CLASS8_H
#define INC_698_CLASS8_H

int class8100_set(int index, OAD oad, INT8U *data, INT8U *DAR);

int class8101_set(int index, OAD oad, INT8U *data, INT8U *DAR);

int class8102_set(int index, OAD oad, INT8U *data, INT8U *DAR);

int class8103_act3(int index, int attr_act, INT8U *data, Action_result *act_ret);

int class8103_act127(int index, int attr_act, INT8U *data, Action_result *act_ret);

#endif //INC_698_CLASS8_H
