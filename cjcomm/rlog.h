#include <syslog.h>

#include "PublicFunction.h"

void asyslog(int priority, const char* fmt, ...);
void bufsyslog(const INT8U* buf, const char* title, int head, int tail, int len);
