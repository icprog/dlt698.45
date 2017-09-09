#ifndef ESAN_H_
#define ESAN_H_
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <time.h>
#include <stdint.h>
#include <signal.h>
#include <semaphore.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <math.h>
#include <termios.h>
#include "stdio.h"
#include "string.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include "PublicFunction.h"

#define ESAM_TIMEOUT   5
#define SPI1   0
#define LCmax              (INT8U)0xFF//(u8)20
#define SETUP_LENGTH       (INT8U)20
#define HIST_LENGTH        (INT8U)20
#define BUFLEN 512
#define  KEY_NUM               12
FILE *fp;

#define MYBUFLEN 			2048
#endif
