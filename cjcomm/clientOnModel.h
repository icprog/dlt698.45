#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <pthread.h>

#include "PublicFunction.h"
#include "dlt698def.h"
#include "cjcomm.h"
#include "at.h"
#include "ae.h"
#include "../include/Shmem.h"

typedef struct {
    INT8U buf[2048];
    INT16U len;
} Block;

typedef struct {
    Block send[16];
    Block recv;
    INT16U head;
    INT16U tail;
} NetObject;
