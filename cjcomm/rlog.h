#include <syslog.h>

#include "PublicFunction.h"

void asyslog(int priority, const char* fmt, ...);
void bufsyslog(const INT8U* buf, int head, int tail, int len);
