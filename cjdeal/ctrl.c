//
// Created by 周立海 on 2017/4/24.
//

#include "ctrl.h"

int ctrl_base_test() {
    printf("%d", CheckModelState());
    InitCtrlModel();
    int fd = OpenSerialPort();

    SetCtrl_CMD(fd, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);

    return 0;
}
