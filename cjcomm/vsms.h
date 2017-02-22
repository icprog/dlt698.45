#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <net/if.h>
#include <signal.h>
#include <pthread.h>
#include <netinet/tcp.h>
#include <mcheck.h>

#include <termios.h>
#include <mqueue.h>

#include "StdDataType.h"
#include "PublicFunction.h"

/*放置公用变量*/
#ifndef jSmscom_H
#define jSmscom_H

#define ZERO 0
#define RES_LENGTH 128 //接受字符的最大长度
#define TEL_LEN 20     //	电话号码存贮长度

// 用户信息编码方式
#define GSM_7BIT 0
#define GSM_8BIT 4
#define GSM_UCS2 8

//串口缓冲区设置
#define BUF_LEN 8192 //	ttyS7接收缓冲区长度

#define SdPrint(...) fprintf(stderr, __VA_ARGS__);

int put1_str_CRLF_Wait(int cmd, char* str, int count1);
void put1_str_CRLF(char* str);
void RxHandle(int cmd);
void dealinit();
void deal_vsms(int msmport);

//模块接收短信信息结构体
#define RXMSG_NUM 40
#define MSGNO_LEN 10
struct RXMSG {
    INT8U rxmsgflag; //	收到短消息标志
    int rxmsgno;     //	收到的短消息序号
};

// PDU短消息参数结构，编码/解码共用
// 其中，字符串以0结尾
typedef struct {
    char SCA[16];     // 短消息服务中心号码(SMSC地址)
    char TPA[16];     // 目标号码或回复号码(TP-DA或TP-RA)
    char TP_PID;      // 用户信息协议标识(TP-PID)
    char TP_DCS;      // 用户信息编码方式(TP-DCS)
    char TP_SCTS[16]; // 服务时间戳字符串(TP_SCTS), 接收时用到
    char TP_UD[161];  // 原始用户信息(编码前或解码后的TP-UD)
    char index;       // 短消息序号，在读取时用到
} SM_PARAM;

#define CR 0x0D
#define LF 0x0A
#define CRLF "\r\n"
#define ZERO 0

#define UNIONCODE "86" //	国家代码
#define MSG_MAXLEN 70  //	短信报文最大长度,该值必须小于TXBUF_LEN

// m37i模块接收命令标识码
#define RX_NULL 0x00000000
#define RX_OK 0x00000001
#define RX_ERROR 0x00000002
#define RX_RING 0x00000004
#define RX_ATE0 0x00000008
#define RX_ATE1 0x00000010
#define RX_CPBR 0x00000020
#define RX_CRLF 0x00000040
#define RX_CMGS 0x00000080
#define RX_AT_ 0x00000100
#define RX_CMTI 0x00000200
#define RX_CMS_ERROR 0x00000400
#define RX_NOCARRIER 0x00000800
#define RX_CMGR 0x00001000
#define RX_CMGR1 0x00002000
#define RX_CMGL 0x00004000
#define RX_CMGL1 0x00008000
#define RX_GREAT 0x00010000
#define RX_CSQ 0x00020000
#define RX_QINISTAT 0x00080000
#define RX_CREG 0x00100000

// m37i模块接收AT命令
#define OK "OK"
#define OK1 "OK\r"
#define Ok1234 "Ok!"
#define RXATE0 "ATE0"
#define RXATE1 "ATE1"
#define CPBR "+CPBR:"
#define CSQ "+CSQ: "
#define ERROR "ERROR"
#define CMGS "+CMGS: "
#define CMGL "+CMGL: "
#define AT_ "AT"
#define IP "+IP:"
#define CMTI "+CMTI: \"SM\","
#define CMTI1 "+CMTI: \"MT\"," // m37i
#define CMGR "+CMGR:"
#define CMGR1 "+CMGR: 0,,0"
#define CMS_ERROR "+CMS ERROR"
#define CSQ "+CSQ: "
#define GREAT ">"
#define RXQINISTAT "+QINISTAT: "
#define RXCREG "+CREG: "

// m37i模块发送AT命令标识码
#define TXCOMMAND_ATE0 0X01
#define TXCOMMAND_CMGF 0X02
#define TXCOMMAND_CPBS 0X03
#define TXCOMMAND_CPBR 0X04
#define TXCOMMAND_CSCS 0X05
#define TXCOMMAND_CNMI 0X06
#define TXCOMMAND_CSQ 0x07
#define TXCOMMAND_CPBW 0x08
#define TXCOMMAND_QINISTAT 0x09
#define TXCOMMAND_CPMS 0x10
#define TXCOMMAND_CREG 0x11

#define TXCOMMAND_SBV 0x21
#define TXCOMMAND_SISI 0x22
#define TXCOMMAND_CMGS 0x23
#define TXCOMMAND_CMGL 0X24
#define TXCOMMAND_CMGD 0X25
#define TXCOMMAND_CMGR 0X26
#define TXCOMMAND_CMGS_D 0x27

//手机短信编码方式都是PDU传输格式，当选择TEXT文本方式接收时，会显示文本信息，但是汉字可能会有乱码
// m37i模块发送AT命令
#define ATE1 "ATE1\r\n"
#define ATE0 "ATE0\r\n" //关闭回显
#define AT_CPBS "AT+CPBS=\"SM\"\r\n"
#define AT_CPBR "AT+CPBR=1\r\n"
#define AT_CPBW "AT+CPBW=1,"
#define AT_CSCS "AT+CSCS=\"UCS2\"\r\n" // PDU:UCS2 ,TEXT:GSM, CMGF=0设置成PDU时，这个CSCS，UCS2，GSM参数没有影响。
//#define	AT_CNMI				"AT+CNMI=3,1,0,0,1\r\n"
#define AT_CNMI "AT+CNMI=2,1,0,0,0\r\n"
#define AT_CMGF "AT+CMGF=0\r\n" // 0:PDU,1:TEXT
#define AT_CMGS "AT+CMGS="
#define AT_CMGL_READ "AT+CMGL=0\r\n" //读取未读过的SMS
#define AT_CMGL_ALL "AT+CMGL=4\r\n"  // AT+CMGL=4 读取全部 SMS 消息
#define AT_CMEE "AT+CMEE=2\r\n"      // AT+CMEE=2,显示错误
#define AT_CMGR "AT+CMGR="
#define AT_CMGD "AT+CMGD="
#define AT_CSQ "AT+CSQ\r\n"
#define AT_SBV "AT^SBV\r\n"
#define AT_CGATT "AT+CGATT=1,1\r\n"
#define AT_QINISTAT "AT+QINISTAT\r\n" //查询SIM卡初始化状态，返回值为３时，表示初始化成功,sim卡可操作
#define AT_CSCA "AT+CSCA?\r\n" //查询短信息服务中心号码。该号码由网络运营商提供，作为出厂设置保存在
                               // sim卡中。在发送短信息之前最好确认号码是否正确。
#define AT_CPMS "AT+CPMS=\"MT\",\"MT\",\"MT\"" //设置短信息读、写、收等操作的存储器,SM存储在SIM卡中
#define AT_CREG "AT+CREG?" //查询模块是否注册上ＧＳＭ网络，+CREG:0,1或CREG:0,5表示模块注册上GSM网络

#endif /*jSmscom_H*/
