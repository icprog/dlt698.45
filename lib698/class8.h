//
// Created by 周立海 on 2017/4/21.
//

#include "../include/StdDataType.h"

#ifndef INC_698_CLASS8_H
#define INC_698_CLASS8_H

/*
 * 遥控
 * */
int class8000_set(OAD oad, INT8U *data, INT8U *DAR);

int class8001_set(OAD oad, INT8U *data, INT8U *DAR);

int class8100_set(int index, OAD oad, INT8U *data, INT8U *DAR);

int class8101_set(OAD oad, INT8U *data, INT8U *DAR);

int class8102_set(OAD oad, INT8U *data, INT8U *DAR);

int class8103_act3(int index, int attr_act, INT8U *data, INT8U *DAR);

int class8104_act3(int index, int attr_act, INT8U *data, INT8U *DAR);

int class8105_act3(int index, int attr_act, INT8U *data, INT8U *DAR);

int class8108_act3(int index, int attr_act, INT8U *data, INT8U *DAR);

int class8107_set(OAD oad, INT8U *data, INT8U *DAR);

int class8000_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret);

int class8001_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret);

int class8002_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret);

int class8003_8004_act_route(OAD oad, INT8U *data, Action_result *act_ret);

int class8103_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret);

int class8104_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret);

int class8105_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret);

int class8106_act_route(OAD oad, INT8U *data, Action_result *act_ret);

int class8107_act_route(OAD oad, INT8U *data, Action_result *act_ret);

int class8108_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret);


#endif //INC_698_CLASS8_H
