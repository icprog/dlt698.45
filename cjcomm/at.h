#ifndef AT_H_
#define AT_H_

#define RES_LENGTH 128 //接受字符的最大长度
#define TEL_LEN 20     //电话号码存贮长度

// 用户信息编码方式
#define GSM_7BIT 0
#define GSM_8BIT 4
#define GSM_UCS2 8

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

void AT_POWOFF();
void CreateATWorker(void);
void SetOnline(void);
void SetOffline(void);
int ATMYSOCKETLED(unsigned char step, int ComPort);
int gpofun(char* devname, int data);

#endif
