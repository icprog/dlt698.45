//
// Created by 周立海 on 2017/6/24.
//

#include "cjcomm.h"
#include "../libMq/libmmq.h"
#include "../include/StdDataType.h"

#ifndef INC_698_MMQ_H
#define INC_698_MMQ_H

typedef struct {
    mmq_head header;
    char content[MAXSIZ_PROXY_NET];
    int repeat_timeout;
} EventBuf;

#define AUTO_EVENT_BUF_SIZE 5
#define AUTO_EVENT_REPEAT_TIMEOUT 60

#endif //INC_698_MMQ_H
