/*
 * PublicFunction.h
 *
 *  Created on: Sep 11, 2015
 *      Author: fzh
 */

#ifndef PUBLICFUNCTION_H_
#define PUBLICFUNCTION_H_

#include <signal.h>
#include <semaphore.h>
#include <syslog.h>
#include "Shmem.h"
#include "StdDataType.h"
#include "ParaDef.h"

#define HEX_TO_BCD(x) (((x)/0x0A)*0x10+((x)%0x0A))
#define BCD_TO_HEX(x) (((x)/0x10)*0x0A+((x)%0x10))
#define ASCII_TO_HEX(c) (((c) >='0' && (c) <='9')?((c)-'0'):(((c)>='A'&&(c)<='F')?((c)-'A'+10):(((c)>='a'&&c<='f')?((c)-'a'+10):0)))
#define isDigit(c) ((unsigned) ((c)-'0') < 10)
#define isHex(c) (((unsigned) ((c)-'0') < 10) || ((unsigned) ((c)-'A') < 6) || ((unsigned) ((c)-'a') < 6) )
#define isDelim(c) (c == ' ' || c == '\n' || c == 't' || c == '\r')
#define HEX_TO_ASCII(x) ((x<=0x09)?(x+0x30):(x+0x37))

#define U8_TO_ASCII_H(x) HEX_TO_ASCII(((x)&0x0F))
#define U8_TO_ASCII_L(x) HEX_TO_ASCII(((x)&0x0F))
#define BIT_N(byte, n) (((byte)>>(n))&0x01)
#define	NELEM(array)	(sizeof(array)/sizeof(array[0]))

#define	FILE_LINE		__FILE__,__FUNCTION__,__LINE__

#define	DEBUG_BUFF(data, dataSize)		debugBuf(FILE_LINE, data, dataSize)
#define	DEBUG_TIME_LINE(format, ...)	debugToStderr(FILE_LINE, format, ##__VA_ARGS__)
#define 	DEBUG_TO_FILE(fname, format, ...)	debugToFile(fname, FILE_LINE, format, ##__VA_ARGS__)

extern void Setsig(struct sigaction* psa, void (*pfun)(ProjectInfo* proinfo));
/*
 * 共享内存操作函数
 * */
extern void* CreateShMem(char* shname, int memsize, void* pmem);
extern void* OpenShMem(char* shname, int memsize, void* pmem);
extern void shmm_unregister(char* shname, int memsize);
extern INT32S prog_find_pid_by_name(INT8S* ProcName, INT32S* foundpid);
/*
 * 串口操作函数
 * */
extern int OpenCom(int port, int baud, unsigned char* par, unsigned char stopb, unsigned char bits);
extern void CloseCom(int ComPort);

/*
 * 时间转换函数
 * */
extern int TItoSec(TI ti);
extern void TSGet(TS* ts);
extern time_t tmtotime_t(TS ptm);
extern time_t TimeBCDTotime_t(DateTimeBCD timeBCD);
extern void TimeBCDToTs(DateTimeBCD timeBCD,TS* outTs);
extern void TsToTimeBCD(TS inTs,DateTimeBCD* outTimeBCD);
extern void time_tToTS(time_t inTimet,TS *outts);
extern DateTimeBCD timet_bcd(time_t t);
extern void setsystime(DateTimeBCD datetime);
extern INT8U TScompare(TS ts1, TS ts2);
extern void DataTimeGet(DateTimeBCD* ts);
extern INT8S tminc(TS* tmi, Time_Units unit, INT32S val);

/* BCD码转int32u
 *参数：bcd为bcd码头指针，len为bcd码长度，order为positive正序/inverted反序，dint转换结果
 * 返回:0：成功；-1：asc为空；-2：en为0；-3：order有误
 * 例如:0x12 0x34 -> 1234
 * */
extern INT8S bcd2int32u(INT8U* bcd, INT8U len, ORDER order, INT32U* dint);
extern INT32S int32u2bcd(INT32U dint32, INT8U* bcd, ORDER order);
INT32S asc2bcd(INT8U* asc, INT32U len, INT8U* bcd,ORDER order);
extern void myBCDtoASC1(char val, char dest[2]);
/*
 * gpio操作函数
 */
extern INT8S gpio_readbyte(char* devpath);
extern INT32S gpio_readint(char* devpath);
extern INT32S gpio_writebyte(char* devpath, INT8S data);
extern INT32S gpio_writebytes(char* devpath, INT8S* vals, INT32S valnum);

extern BOOLEAN pwr_has();
extern INT8U pwr_down_byVolt(INT8U valid, INT32U volt, INT16U limit);
extern INT8U pwr_on_byVolt(INT8U valid, INT32U volt, INT16U limit);
extern BOOLEAN bettery_getV(FP32* clock_bt, FP32* tmnl_bt);
extern BOOLEAN bettery_getV_II(FP32* clock_bt);

extern INT8S getSpiAnalogState();
/*
 * 信号量操作函数
 * */
extern sem_t* nsem_open(const char* name);
extern sem_t* create_named_sem(const char* name, int flag);
extern sem_t* open_named_sem(const char* name);
extern void close_named_sem(const char* name);

/*
 * 数据从大到小排序 arr数组 len长度
 */
extern INT8U getarryb2s(INT32S* arr, INT8U len);

/*
 * 日志处理函数
 */
void asyslog(int priority, const char* fmt, ...);
void bufsyslog(const INT8U* buf, const char* title, int head, int tail, int len);
//国网送检时，增加了packet.log打印gprs和485_1的报文
void PacketBufToFile(INT8U type,char *prefix, char *buf, int len, char *suffix);

/*
 * 数据处理
 * */
extern INT8U getBase_DataTypeLen(Base_DataType dataType,INT8U data);
extern INT8S reversebuff(INT8U* buff,INT32U len,INT8U* invbuff);


extern void debug(const char* file, const char* func, INT32U line, const char *fmt, ...);
extern void debugBuf(const char* file, const char* func, INT32U line, INT8U* buf, INT32U bufSize);
extern void debugToPlcFile(const char* file, const char* func, INT32U line, const char *fmt, ...);
extern void debugToStderr(const char* file, const char* func, INT32U line, const char *fmt, ...);
extern void debugToFile(const char* fname, const char* file, const char* func, INT32U line, const char *fmt,...);
extern void readFrm(char* str,  INT8U* buf, INT32U* bufSize);

//读取设备配置信息
extern int ReadDeviceConfig(ConfigPara	*cfg_para);

/* 地区获取
 * 输入值　zone 区分大小写，与device.cfg的zone地区配置一致
 * 返回值　=０:满足该地区要求　 =1: 非该地区
 * */
extern int getZone(char *zone);

/*
 * 写/nor/rc.d/ip.sh脚本
 * */
extern void writeIpSh(INT8U *ip,INT8U *netmask);

/* 修改ID生成备份文件
 * */
extern void writeIdFile(CLASS_4001_4002_4003 classtmp);
/*
 * 参数初始化
 * */
extern void InitClass4016();	//当前套日时段表
extern void InitClass4300();	//电气设备信息
//extern void InitClass4510(INT16U heartBeat,MASTER_STATION_INFO master_info,NETCONFIG net_ip); //以太网通信模块1
extern void InitClass6000();	//采集配置单元
extern void InitClassf203();	//开关量输入
extern void InitClassByZone(INT8U type);	//根据地区进行相应初始化

#endif /* PUBLICFUNCTION_H_ */
