#ifndef CJCOMM_H_
#define CJCOMM_H_

#include <termios.h>
#include <errno.h>
#include <wait.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include "StdDataType.h"
#include "PublicFunction.h"
#include "dlt698.h"

INT8S SendPro(int fd, INT8U* buf, INT16U len);
void RecvPro(int fd, INT8U* buf, int* head);
void connect_socket(INT8U* server, INT16U serverPort, int* fd);
void initComPara(CommBlock* compara);
int getOnlineState(void);
void setCCID(INT8U CCID[]);
void setIMSI(INT8U IMSI[]);
void setSINSTR(INT16U SINSTR);
void setPPPIP(INT8U PPPIP[]);
void saveCurrClass25(void);

#endif
