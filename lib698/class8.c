//
// Created by 周立海 on 2017/4/21.
//

#include "class8.h"
#include "PublicFunction.h"

int class8001_act127(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    asyslog(LOG_WARNING, "投入保电\n");
    return 0;
}

int class8001_act128(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    asyslog(LOG_WARNING, "解除保电\n");
    return 0;
}

int class8001_act129(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    asyslog(LOG_WARNING, "解除自动保电\n");
    return 0;
}


int class8001_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    switch (attr_act) {
        case 127:
            class8001_act127(1, attr_act, data, act_ret);
            break;
        case 128:
            class8001_act128(1, attr_act, data, act_ret);
            break;
        case 129:
            class8001_act129(1, attr_act, data, act_ret);
            break;
    }
}


INT64U getLongValue(INT8U *data) {
    INT64U v = 0x00;
    for (int i = 0; i < 8; ++i) {
        v += data[i + 1];
        if (i + 1 == 8) {
            break;
        }
        v = v << 8;
    }
    return v;
}

int class8100_set(int index, OAD oad, INT8U *data, INT8U *DAR) {
    CLASS_8100 c8100;
    if (data[0] != 0x14) {
        return 0;
    }

    c8100.v = getLongValue(data);
    asyslog(LOG_WARNING, "设置终端安保定值(%lld)", c8100.v);

    return 0;
}


int class8101_set(int index, OAD oad, INT8U *data, INT8U *DAR) {
    CLASS_8101 c8101;
    if (data[0] != 0x01 || data[1] != 0x0C) {
        return 0;
    }

    for (int i = 0; i < 12; ++i) {
        c8101.time[i] = data[i * 2 + 3];
        printf("%02x\n", c8101.time[i]);
    }
    return 0;
}

int class8102_set(int index, OAD oad, INT8U *data, INT8U *DAR) {
    CLASS_8102 c8102;
    if (data[0] != 0x01 || data[1] != 0x08) {
        return 0;
    }

    for (int i = 0; i < 8; ++i) {
        c8102.time[i] = data[i * 2 + 3];
        printf("%02x\n", c8102.time[i]);
    }
    return 0;
}


int class8103_act3(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    CLASS_8103 c8103;

    if (data[0] != 0x02 || data[1] != 0x06) {
        return 0;
    }

    if (data[2] != 0x50) {
        return 0;
    }

    c8103.list[0].index = data[3] * 256 + data[4];
    c8103.list[0].sign = data[7];

    if (data[8] != 0x02 || data[9] != 0x09) {
        return 0;
    }

    c8103.list[0].v1.n = data[12];
    c8103.list[0].v1.t1 = getLongValue(&data[13]);
    c8103.list[0].v1.t2 = getLongValue(&data[22]);
    c8103.list[0].v1.t3 = getLongValue(&data[31]);
    c8103.list[0].v1.t4 = getLongValue(&data[40]);
    c8103.list[0].v1.t5 = getLongValue(&data[49]);
    c8103.list[0].v1.t6 = getLongValue(&data[58]);
    c8103.list[0].v1.t7 = getLongValue(&data[67]);
    c8103.list[0].v1.t8 = getLongValue(&data[76]);

    if (data[85] != 0x02 || data[86] != 0x09) {
        return 0;
    }

    c8103.list[0].v2.n = data[89];
    c8103.list[0].v2.t1 = getLongValue(&data[90]);
    c8103.list[0].v2.t2 = getLongValue(&data[99]);
    c8103.list[0].v2.t3 = getLongValue(&data[108]);
    c8103.list[0].v2.t4 = getLongValue(&data[117]);
    c8103.list[0].v2.t5 = getLongValue(&data[126]);
    c8103.list[0].v2.t6 = getLongValue(&data[135]);
    c8103.list[0].v2.t7 = getLongValue(&data[144]);
    c8103.list[0].v2.t8 = getLongValue(&data[153]);

    if (data[162] != 0x02 || data[163] != 0x09) {
        return 0;
    }

    c8103.list[0].v2.n = data[166];
    c8103.list[0].v2.t1 = getLongValue(&data[175]);
    c8103.list[0].v2.t2 = getLongValue(&data[184]);
    c8103.list[0].v2.t3 = getLongValue(&data[193]);
    c8103.list[0].v2.t4 = getLongValue(&data[202]);
    c8103.list[0].v2.t5 = getLongValue(&data[211]);
    c8103.list[0].v2.t6 = getLongValue(&data[220]);
    c8103.list[0].v2.t7 = getLongValue(&data[229]);
    c8103.list[0].v2.t8 = getLongValue(&data[238]);

    c8103.list[0].para = data[248];
    printf("c8103 act 3\n");
    return 0;
}

int class8103_act6(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    int oi = 0x00;
    if (data[0] != 0x50) {
        return 0;
    }

    oi = data[1] * 256 + data[2];
    asyslog(LOG_WARNING, "时段功控-控制投入[%04x]", oi);
    return 0;
}

int class8103_act7(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    int oi = 0x00;
    if (data[0] != 0x50) {
        return 0;
    }

    oi = data[1] * 256 + data[2];
    asyslog(LOG_WARNING, "时段功控-控制解除[%04x]", oi);
    return 0;
}

int class8103_act127(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    int oi = 0x00;
    INT8U sign = 0x00;
    INT8U numb = 0x00;

    if (data[0] != 0x02 || data[1] != 0x02) {
        return 0;
    }

    oi = data[3] * 256 + data[4];

    if (data[5] != 0x02 || data[6] != 0x02) {
        return 0;
    }

    sign = data[9];
    numb = data[11];

    asyslog(LOG_WARNING, "控制方案切换[8103-127],%04x-%02d-%02d", oi, sign, numb);

    return 0;
}

int class8103_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    switch (attr_act) {
        case 3:
            class8103_act3(1, attr_act, data, act_ret);
            break;
        case 6:
            class8103_act6(1, attr_act, data, act_ret);
            break;
        case 7:
            class8103_act7(1, attr_act, data, act_ret);
            break;
        case 127:
            class8103_act127(1, attr_act, data, act_ret);
            break;
    }
}


int class8104_act3(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    asyslog(LOG_WARNING, "厂休功控-添加控制单元");
    return 0;
}

int class8104_act6(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    int oi = 0x00;
    if (data[0] != 0x50) {
        return 0;
    }

    oi = data[1] * 256 + data[2];
    asyslog(LOG_WARNING, "厂休功控-控制投入[%04x]", oi);
    return 0;
}

int class8104_act7(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    int oi = 0x00;
    if (data[0] != 0x50) {
        return 0;
    }

    oi = data[1] * 256 + data[2];
    asyslog(LOG_WARNING, "厂休功控-控制解除[%04x]", oi);
    return 0;
}

int class8104_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    switch (attr_act) {
        case 3:
            class8104_act3(1, attr_act, data, act_ret);
            break;
        case 6:
            class8104_act6(1, attr_act, data, act_ret);
            break;
        case 7:
            class8104_act7(1, attr_act, data, act_ret);
            break;
    }
}


int class8105_act3(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    asyslog(LOG_WARNING, "营业报停-添加控制单元");
    return 0;
}

int class8105_act6(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    int oi = 0x00;
    if (data[0] != 0x50) {
        return 0;
    }

    oi = data[1] * 256 + data[2];
    asyslog(LOG_WARNING, "营业报停-控制投入[%04x]", oi);
    return 0;
}

int class8105_act7(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    int oi = 0x00;
    if (data[0] != 0x50) {
        return 0;
    }

    oi = data[1] * 256 + data[2];
    asyslog(LOG_WARNING, "营业报停-控制解除[%04x]", oi);
    return 0;
}

int class8105_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    switch (attr_act) {
        case 3:
            class8105_act3(1, attr_act, data, act_ret);
            break;
        case 6:
            class8105_act6(1, attr_act, data, act_ret);
            break;
        case 7:
            class8105_act7(1, attr_act, data, act_ret);
            break;
    }
}

int class8106_act3(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    asyslog(LOG_WARNING, "功率下浮-添加控制单元");
    return 0;
}

int class8106_act6(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    int oi = 0x00;
    if (data[0] != 0x50) {
        return 0;
    }

    oi = data[1] * 256 + data[2];
    asyslog(LOG_WARNING, "功率下浮-控制投入[%04x]", oi);
    return 0;
}

int class8106_act7(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    int oi = 0x00;
    if (data[0] != 0x50) {
        return 0;
    }

    oi = data[1] * 256 + data[2];
    asyslog(LOG_WARNING, "功率下浮-控制解除[%04x]", oi);
    return 0;
}

int class8106_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    switch (attr_act) {
        case 3:
            class8106_act3(1, attr_act, data, act_ret);
            break;
        case 6:
            class8106_act6(1, attr_act, data, act_ret);
            break;
        case 7:
            class8106_act7(1, attr_act, data, act_ret);
            break;
    }
}


int class8107_act3(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    int oi = 0x00;
    int id = 0x00;
    INT8U sign = 0x00;
    INT8U type = 0x00;
    INT64U val = 0x00;
    INT64U war_thr = 0x00;
    INT64U ctl_thr = 0x00;
    INT8U mode = 0x00;

    if (data[0] != 0x02 || data[1] != 0x08) {
        return 0;
    }

    oi = data[3] * 256 + data[4];
    id = data[6] * 256 * 256 * 256 + data[7] * 256 * 256 + data[8] * 256 + data[9];

    sign = data[11];
    type = data[13];

    val = getLongValue(&data[14]);
    war_thr = getLongValue(&data[23]);
    ctl_thr = getLongValue(&data[32]);

    mode = data[42];

    asyslog(LOG_WARNING, "购电-添加控制单元[%04x-%d-%d-%d-%lld-%lld-%lld-%d]", oi, id, sign, type, val, war_thr, ctl_thr, mode);
    return 0;
}

int class8107_act6(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    int oi = 0x00;
    if (data[0] != 0x50) {
        return 0;
    }

    oi = data[1] * 256 + data[2];
    asyslog(LOG_WARNING, "购电-控制投入[%04x]", oi);
    return 0;
}

int class8107_act7(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    int oi = 0x00;
    if (data[0] != 0x50) {
        return 0;
    }

    oi = data[1] * 256 + data[2];
    asyslog(LOG_WARNING, "购电-控制解除[%04x]", oi);
    return 0;
}

int class8107_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    switch (attr_act) {
        case 3:
            class8107_act3(1, attr_act, data, act_ret);
            break;
        case 6:
            class8107_act6(1, attr_act, data, act_ret);
            break;
        case 7:
            class8107_act7(1, attr_act, data, act_ret);
            break;
    }
}


int class8108_act3(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    int oi = 0x00;
    INT64U v = 0x00;
    INT8U th = 0x00;
    INT8U fl = 0x00;

    if (data[0] != 0x02 || data[1] != 0x04) {
        return 0;
    }

    oi = data[3] * 256 + data[4];
    v = getLongValue(&data[5]);
    th = data[15];
    fl = data[17];

    asyslog(LOG_WARNING, "月电-添加控制单元[%04x-%lld-%d-%d]", oi, v, th, fl);
    return 0;
}

int class8108_act6(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    int oi = 0x00;
    if (data[0] != 0x50) {
        return 0;
    }

    oi = data[1] * 256 + data[2];
    asyslog(LOG_WARNING, "月电-控制投入[%04x]", oi);
    return 0;
}

int class8108_act7(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    int oi = 0x00;
    if (data[0] != 0x50) {
        return 0;
    }

    oi = data[1] * 256 + data[2];
    asyslog(LOG_WARNING, "月电-控制解除[%04x]", oi);
    return 0;
}

int class8108_act_route(int index, int attr_act, INT8U *data, Action_result *act_ret) {
    switch (attr_act) {
        case 3:
            class8108_act3(1, attr_act, data, act_ret);
            break;
        case 6:
            class8108_act6(1, attr_act, data, act_ret);
            break;
        case 7:
            class8108_act7(1, attr_act, data, act_ret);
            break;
    }
}