//
// Created by 周立海 on 2017/4/24.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ctrl.h"
#include "crtl_base.h"
#include "../include/Objectdef.h"
#include "../libAccess/AccessFun.h"

static CLASS23 class23[8];

static CLASS_8103 c8103;
static CLASS_8104 c8104;
static CLASS_8105 c8105;
static CLASS_8106 c8106;

static CLASS_8107 c8107;
static CLASS_8108 c8108;


int ctrl_base_test() {
    printf("%d", CheckModelState());
    InitCtrlModel();
    int fd = OpenSerialPort();

    SetCtrl_CMD(fd, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);

    return 0;
}

void refreshSumUp() {
    //

}

int initAll() {
    //读取总加组数据
    for (int i = 0; i < 8; ++i) {
        memset(&class23[i], 0x00, sizeof(class23[i]));
        readCoverClass(0x2301 + i, 0, &class23[i], sizeof(class23[i]), para_vari_save);
    }

    //读取配置参数
    memset(&c8103, 0x00, sizeof(c8103));
    readCoverClass(0x8103, 0, &c8103, sizeof(c8103), para_vari_save);

    memset(&c8104, 0x00, sizeof(c8104));
    readCoverClass(0x8104, 0, &c8104, sizeof(c8104), para_vari_save);

    memset(&c8105, 0x00, sizeof(c8105));
    readCoverClass(0xc8105, 0, &c8105, sizeof(c8105), para_vari_save);

    memset(&c8106, 0x00, sizeof(c8106));
    readCoverClass(0x8106, 0, &c8106, sizeof(c8106), para_vari_save);

    memset(&c8107, 0x00, sizeof(c8107));
    readCoverClass(0x8107, 0, &c8107, sizeof(c8107), para_vari_save);

    memset(&c8108, 0x00, sizeof(c8108));
    readCoverClass(0x8108, 0, &c8108, sizeof(c8108), para_vari_save);

    return 0;
}

int check_c8103(PowerCtrlParam pcp, TS ts) {

    return;
}

int deal8103(TS ts) {
    //判断投入状态
//    if (c8103.index < 0x02301 || c8103.index > 0x2308){
//        return 0;
//    }

    //获取总加组
    int index = -1;
    for (int i = 0; i < MAX_AL_UNIT; ++i) {
//        if (c8103.list[i].index == c8103.index) {
//            index = i;
//            break;
//        }
    }

    if (index == -1) {
        return 0;
    }

    //获取当前时段的有效定值
//    int val = check_c8103(pcp, ts);
//    if (val == -1) {
//        return;
//    }

//    if (class23[index].p > val) {
//        //产生时段功控
//    }

    return 0;
}


int check_c8104(int index, TS ts) {
    //根据ts返回当前厂休定值
    return c8104.list[index].v;
}

int deal8104(TS ts) {
    //获取总加组
    int index = -1;
    for (int i = 0; i < MAX_AL_UNIT; ++i) {
//        if (c8104.list[i].index == c8104.index) {
//            index = i;
//            break;
//        }
    }
    if (index == -1) {
        return 0;
    }

    int val = check_c8104(index, ts);
    if (val == -1) {
        return;
    }

    if (class23[index].p > val) {
        //产生一次越限信号
        return 0x8104;
    }

    return 0;

}

int check_c8105(int index, TS ts) {
    //根据ts返回当前营业报停定值
    return c8105.list[index].v;
}

int deal8105(TS ts) {
    //获取总加组
    int index = -1;
    for (int i = 0; i < MAX_AL_UNIT; ++i) {
//        if (c8105.list[i].index == c8105.index) {
//            index = i;
//            break;
//        }
    }
    if (index == -1) {
        return 0;
    }

    int val = check_c8105(index, ts);
    if (val == -1) {
        return;
    }

    if (class23[index].p > val) {
        //产生一次越限信号
        return 0x8105;
    }

    return 0;
}

int deal8106(TS ts) {
#if 0
    //获取总加组
    int index = -1;
    for (int i = 0; i < MAX_AL_UNIT; ++i) {
        if (c8106.list[i].index == c8106.index) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        return 0;
    }
#endif
}


int deal8107(TS ts) {
    //获取总加组
    int index = -1;
    for (int i = 0; i < MAX_AL_UNIT; ++i) {
//        if (c8107.list[i].index == c8107.index) {
//            index = i;
//            break;
//        }
    }
    if (index == -1) {
        return 0;
    }

    if (c8107.list[index].alarm > class23->remains) {
        //购电控报警
    }

    if (c8107.list[index].ctrl > class23->remains) {
        //购电控跳闸
        if (c8107.list[index].mode == 0) {
            //真正执行跳闸
        }
    }


}

void deal8108(TS ts) {
    //获取总加组
    int index = -1;
    for (int i = 0; i < MAX_AL_UNIT; ++i) {
//        if (c8108.list[i].index == c8108.index) {
//            index = i;
//            break;
//        }
    }
    if (index == -1) {
        return 0;
    }

    if (c8108.list[index].v < class23->MonthP[0]) {
        //产生月电控报警
    }

}

void dealControl() {
    TS ts = {};
    TSGet(&ts);

    //直接跳闸，必须检测
    deal8107(ts);//购电控
    deal8108(ts);//月电控控制

    /*
     * 检测控制有优先级，当高优先级条件产生时，忽略低优先级的配置
     */
    //功率下浮控
    if (deal8106(ts) != 0) {
        return;
    }
    //营业报停控
    if (deal8105(ts) != 0) {
        return;
    }
    //厂休控
    if (deal8104(ts) != 0) {
        return;
    }
    //时段功控
    if (deal8103(ts) != 0) {
        return;
    }

}

int ctrlMain() {

    //初始化参数,搭建8个总加组数据，读取功控、电控参数
    initAll();

    while (1) {
        //更新总加组数据
        refreshSumUp();

        //处理控制逻辑
        dealControl();
    }
}
