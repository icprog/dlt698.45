//
// Created by 周立海 on 2017/4/21.
//

#include "class23.h"
#include "PublicFunction.h"

int class23_selector(int index, int attr_act,INT8U *data, Action_result *act_ret)
{
    switch (attr_act) {
        case 1:
            class23_act1(index);
            break;
        case 3:
            class23_act3(index, data);
            break;
    }
    return 0;
}

int class23_act1(int index)
{
    asyslog(LOG_WARNING, "清除所有配置单元(%d)", index);
    return 0;
}

int class23_act3(int index, INT8U* data)
{
    CLASS23 class23;
    if(data[0] != 0x02 || data[1] != 0x03 || data[2] != 0x55){
        return 0;
    }

    int tsa_len = data[3];
    int data_index = 4;

    if(tsa_len > 17)
    {
        return 0;
    }
    class23.allist[0].tsa.addr[0] = tsa_len+1;
    class23.allist[0].tsa.addr[1] = tsa_len-1;

    for (int i = 0; i < tsa_len; ++i) {
        class23.allist[0].tsa.addr[2+i] = data[data_index];
        data_index++;
    }

    if(data[data_index] != 0x16 || data[data_index+2] != 0x16){
        return 0;
    }

    class23.allist[0].al_flag = data[data_index+1];
    class23.allist[0].cal_flag = data[data_index+3];

    asyslog(LOG_WARNING, "添加一个配置单元(%d)", index);

    return 0;
}

int class23_set(int index, OAD oad, INT8U *data,INT8U *DAR)
{
    CLASS23 class23;
    asyslog(LOG_WARNING, "修改总加组属性(%d)", oad.attflg);

    switch (oad.attflg) {
        case 13:
            if (data[0] != 0x17){
                return 0;
            }
            class23.aveCircle = data[1];
            break;
        case 14:
            if(data[0] != 0x04){
                return 0;
            }
            class23.pConfig = data[1];
            break;
        case 15:
            if(data[0] != 0x04){
                return 0;
            }
            class23.eConfig = data[1];
            break;
    }
    return 0;
}