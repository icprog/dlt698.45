#ifndef V_HANDLEH_
#define V_HANDLEH_

#include "ae.h"
#include "msgr.h"
#include "libhd.h"
#include "gdm_para.h"
#include "mmqdef.h"
#include "gdm.h"
#include "shmdef.h"
#include "libgdw3761.h"

#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

INT8S ifrWrite(INT8U* buf, INT16U len);
void ifrRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask);

INT8S comWrite(INT8U* buf, INT16U len);
void comRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask);

void mmqRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask);

INT8S setpara_tomsg(gdm_type mqtype, INT8U* buf, INT16U len);
INT8S sendacs_tomsg(gdm_type mqtype, INT8U* buf, INT16U len);
INT8S read4851_tomsg(gdm_type mqtype, INT8U* buf, INT16U len);
INT8S read4852_tomsg(gdm_type mqtype, INT8U* buf, INT16U len);
INT8S readcalc_tomsg(gdm_type mqtype, INT8U* buf, INT16U len);
INT8S readplc_tomsg(gdm_type mqtype, INT8U* buf, INT16U len);
INT8S setctrl_tomsg(gdm_type mqtype, INT8U* buf, INT16U len);
INT8S reset_tomsg(gdm_type mqtype, INT8U* buf, INT16U len);
INT8S vevent_tomsg(gdm_type mqtype, INT8U* buf, INT16U len);

#endif
