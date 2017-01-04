#include "libgdw3761.h"
#include "handle.h"
#include "mtypes.h"

#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

INT8S ifrWrite(INT8U* buf, INT16U len) {
    mAgent* ao = getIfrStruct();
    write(ao->fd, buf, len);
}

void ifrRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask) {
    mAgent* agentIfr = (mAgent*)clientData;

    int revcount = 0;
    ioctl(agentIfr->fd, FIONREAD, &revcount);

    int len, j = 0;
    for (j = 0; j < revcount; j++) {
        len = read(agentIfr->fd, agentIfr->NetRevBuf + agentIfr->rev_head, 1);
        agentIfr->rev_head += len;
        agentIfr->rev_head %= FrameSize;
    }

    int deallen = 0;

    deallen = gdw3761_preprocess(&agentIfr->step, &agentIfr->rev_delay, 100, &agentIfr->rev_tail, &agentIfr->rev_head,
                                 agentIfr->NetRevBuf, agentIfr->DealBuf);

    if (deallen > 0) {
        gdw3761_setCallback(ifrWrite);
        int ret = gdw3761_parse(agentIfr->DealBuf, deallen, NULL);
        if (ret > 0) {
            fprintf(stderr, "[vMsgr][Ifr]主站响应 [AFN %02x]\n", ret);
        }
    }
}
