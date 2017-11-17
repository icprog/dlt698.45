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
/*
 * 保电
 * */
int class8001_set(OAD oad, INT8U *data, INT8U *DAR);
/*
 * 一般中文信息，重要中文信息
 * */
int class8003_8004_set(OAD oad, INT8U *data, INT8U *DAR);
/*
 * 终端保安定值
 */
int class8100_set(OAD oad, INT8U *data, INT8U *DAR);
/*
 * 终端时控时段
 * */
int class8101_set(OAD oad, INT8U *data, INT8U *DAR);
/*
 * 功控告警时间
 * */
int class8102_set(OAD oad, INT8U *data, INT8U *DAR);
/*
 * 时段功控
 * */
int class8103_set(OAD oad, INT8U *data, INT8U *DAR);
/*
 * 厂休控
 * */
int class8104_set(OAD oad, INT8U *data, INT8U *DAR);
/*
 * 营业报停控
 * */
int class8105_set(OAD oad, INT8U *data, INT8U *DAR);
/*
 * 购电控
 * */
int class8107_set(OAD oad, INT8U *data, INT8U *DAR);
/*
 * 月电控
 * */
int class8108_set(OAD oad, INT8U *data, INT8U *DAR);


int class8000_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret);

int class8001_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret);

int class8002_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret);

int class8003_8004_act_route(OAD oad, INT8U *data, Action_result *act_ret);

/*
 * 时段功控
 * */
int class8103_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret);
int class8103_act3(int index, int attr_act, INT8U *data, INT8U *DAR);

/*
 * 厂休控
 * */
int class8104_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret);
int class8104_act3(int index, int attr_act, INT8U *data, INT8U *DAR);
/*
 * 营业报停控
 * */
int class8105_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret);
int class8105_act3(int index, int attr_act, INT8U *data, INT8U *DAR);

/*
 * 当前功率下浮控
 * */
int class8106_act_route(OAD oad, INT8U *data, Action_result *act_ret);
int class8106_unit(int attr_act, INT8U *data, CLASS_8106 *shmc8106, INT8U *DAR);

/*
 * 购电控
 * */
int class8107_act_route(OAD oad, INT8U *data, Action_result *act_ret);
int set_OI810c(INT8U service,INT8U *data,BUY_CTRL *oi810c,INT8U *DAR);

/*
 * 月电控
 * */
int class8108_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret);
int class8108_act3(int index, int attr_act, INT8U *data, INT8U *DAR);


#endif //INC_698_CLASS8_H
