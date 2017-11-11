//
// Created by 周立海 on 2017/4/24.
//

#ifndef INC_698_CTRL_H
#define INC_698_CTRL_H

typedef union { //control code
    INT16U u16b; //convenient to set value to 0
    struct { //only for little endian mathine!
        INT8U lun1_state : 1; //轮次1-状态
        INT8U lun1_red : 1; //轮次1-红灯
        INT8U lun1_green : 1; //轮次1-绿灯
        INT8U lun2_state : 1; //轮次2-状态
        INT8U lun2_red : 1; //轮次2-红灯
        INT8U lun2_green : 1; //轮次2-绿灯
        INT8U gongk_led : 1; //功控灯
        INT8U diank_led : 1; //电控灯
        INT8U alm_state : 1; //告警状态
        INT8U baodian_led : 1; //报警灯
        INT8U bak : 6; //备用
    } ctrl;
} ctrlUN;

int ctrlMain(void * arg);

#endif //INC_698_CTRL_H
