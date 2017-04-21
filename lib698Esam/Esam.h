/*
 * Esam.h
 *
 * Created on: 2013-6-26
 * Author: Administrator
 */

#ifndef ESAM_H_
#define ESAM_H_

#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include "../include/StdDataType.h"
#define DEBUG_ESAM_COMM

#define MARK_ESAM 0x55
#define RevHeadLen_ESAM   4

//---------------------ESAM ERR ARRAY
#define ERR_ESAM_WRTBUF_OVERLEN -1  //写入缓冲长度过长,2000字节限制
#define ERR_ESAM_WAIT_TIMEOUT -2    //数据等待超时，没有检测到0x55,对于错误数据或报文也返回该值
#define ERR_ESAM_SW1SW2_ALLZERO -3  //接收数据状态字全零 异常
#define ERR_ESAM_RETDATA_OVERLEN -4 // ESAM返回的LEN代表的长度数据过长，后续数据没有接收；
#define ERR_ESAM_SENDDATA_LRCERR -5 //发送数据LRC校验错误
#define ERR_ESAM_REVDATA_LRCERR -6  //接收数据LRC校验错误
#define ERR_ESAM_SPI_OPENERR -7     // ESAM SPI设备打开失败
#define ERR_ESAM_TRANSPARA_ERR -8  //传入参数错误
//----SW1SW2错误字错误
#define ERRNUM_SW1SW2_ESAM 21

#define ERR_ESAM_INCNOSUPP -9     //INS 不支持
#define ERR_ESAM_CLANOSUPP -10         //CLA 不支持
#define ERR_ESAM_P2P2_ERR -11           //  P1P2 不正确
#define ERR_ESAM_LC_LENGTH_ERR -12          // LC 长度错误
#define ERR_ESAM_MEM_ERR -13 //存储器故障

#define ERR_ESAM_COUNTER_ERR -14           //计数器不正确
#define ERR_ESAM_RANDOM_ERR -15        //  随机数无效
#define ERR_ESAM_OUT_AUTH_ERR -16       //外部认证错误
#define ERR_ESAM_DATA_SERIAL_ERR -17    //数据包序号错
#define ERR_ESAM_TIMER_OUT -18 // TIMER 超时

#define ERR_ESAM_ORDER_STRUCT_ERR -19            // 命令与文件结构不兼容
#define ERR_ESAM_COMMIT_UNBULID -20     //会话未建立
#define ERR_ESAM_USECONDITION_ERR -21           //使用条件不满足
#define ERR_ESAM_COUNTERR -22           //计算错误
#define ERR_ESAM_MAC_ERR -23     //MAC 校验错

#define ERR_ESAM_DATAAREA_ERR -24  //不正确的数据域
#define ERR_ESAM_FUNCTION_ERR -25    //功能不支持
#define ERR_ESAM_FILE_NOFIND -26            //文件未找到
#define ERR_ESAM_LACK_MEM -27          //无足够的文件存储空间
#define ERR_ESAM_NOFIND_KEY -28          //密钥未找到
#define ERR_ESAM_LRC_CHECK_ERR -29           //LRC 校验错误

#define ERR_ESAM_UNKNOWN -100 // ESAM未知错误

#define ERR_ESAM_GetSerialNum -101      //获取ESAM芯片序列号错误
#define ERR_ESAM_GetCountOffLine -102   //获取离线计数器错误
#define ERR_ESAM_GetChipStatusInfo -103 //获取芯片状态信息错误
#define ERR_ESAM_GetCASerialNum -104    //获取证书系列号错误
#define ERR_ESAM_GetPrivateKey -105     //获取密钥版本错误

#define ERR_ESAM_GetRandomNum_8Byte -106  //获取8字节随机数错误
#define ERR_ESAM_GetRandomNum_16Byte -107 //获取16字节随机数错误
#define ERR_ESAM_GetRandomNum_TYPE -108   //随机数类型错误
#define ERR_ESAM_ShareFile_ReadLen -109   //终端共享文件偏置和数据长度异常，或两个的和
#define ERR_ESAM_RevLEN -110              // ESAM返回数据的长度
#define ERR_ESAM_CertiType -111           //证书类型或切换状态类型不存在，只有00、01两种
#define ERR_ESAM_AFN -112                 // MAC校验和组广播中，AFN类型错误或不存在
#define MeterNum_Type_P2 -113             //身份认证时，电表表号类型错误
#define ERR_ESAM_INTEREXE_ERR -114  //ESAM驱动函数内部错误

/*esam发送数据帧结构：55 CLA INS P1 P2 Len1 Len2 DATA LRC1
 *55为发送命令结构的命令头；
（2）CLA是命令类别；
（3）INS是命令类别中的指令代码；
（4）P1、P2是一个完成指令代码的参考符号；
（5）Len1 Len2是后续DATA的长度，不包含LRC1，由两字节表示；
（6）DATA是由T-ESAM来处理的数据输入；
（7）LRC1是发送数据的校验值，计算方法见SPI通信流程
 */

/*esam接收数据的结构为：SW1 SW2 Len1 Len2 DATA LRC2，其中：
（1）SW1 SW2是指令执行完毕后，从设备返回的状态字；
（2）Len1 Len2是后续DATA的长度，不包含LRC2，由两字节表示；
（3）DATA是T-ESAM处理数据完毕后，返回的输出数据；
（4）LRC2是接收数据的校验值，计算方法见SPI通信流程说明
*/

/*LRC1的计算方法：对CLA INS P1 P2 Len1 Len2 DATA数据，每个字节的异或值再取反。
 *LRC2的计算方法：对SW1 SW2 Len1 Len2 DATA数据，每个字节的异或值，再取反。
 */

/*
		SW1  SW2  含义
		90  00  成功
		6D  00  INS 不支持
		6E  00  CLA 不支持
		6A  86  P1P2 不正确
		67  00  LC 长度错误
		65  81  存储器故障

		69  01  计数器不正确
		69  03  随机数无效
		69  04  外部认证错误
		69  05  数据包序号错
		69  07  TIMER 超时

		69  81  命令与文件结构不兼容
		69  82  会话未建立
		69  85  使用条件不满足
		69  88  计算错误
		69  89  MAC 校验错

		6A  80  不正确的数据域
		6A  81  功能不支持
		6A  82  文件未找到
		6A  84  无足够的文件存储空间
		6A  88  密钥未找到
		6A  90  LRC 校验错误
*/
const static INT16S SW1SW2[] = {0x6D00, 0x6E00, 0x6A86, 0x6700, 0x6581,0x6901, 0x6903, 0x6904,
                                  0x6905, 0x6907, 0x6981, 0x6982, 0x6985, 0x6988, 0x6989, 0x6A80, 0x6A81,
                                 0x6A82, 0x6A84, 0x6A88, 0x6A90};
typedef struct
{
	INT8U SingleAddrCounter[4];
	INT8U ReportCounter[4];
	INT8U BroadCastSID[4];
}EsamCurrCounter; //当前计数器
typedef struct
{
	INT8U TerminalCcieVersion;
	INT8U ServerCcieVersion;
}EsamCcieVersion;  //证书版本
typedef struct
{
	INT8U EsamSID[8];    //Esam序列号
	INT8U EsamVID[4];   //Esam版本号
	INT8U SecretKeyVersion[16];  //对称密钥版本
	INT8U SessionTimeHold[4];  //会话时效门限
	INT8U SessionTimeLeft[4];  //会话实效剩余时间
	EsamCurrCounter CurrentCounter; //当前计数器
	EsamCcieVersion  CcieVersion; //证书版本
	INT8U TerminalCcieSID[16];  //终端证书序列号
	INT8U *TerminalCcie; //终端证书
	INT8U ServerCcieSID[16];  //主站证书序列号
	INT8U *ServerCcie;  //主站证书
}EsamInfo;
typedef struct
{
	INT8U DataType;//明文+MAC  0X01  密文  0x02  密文+MAC 0x03
	INT8U MeterNO[20];//第一字节表长度
	INT8U RN[20];//第一字节表长度
	INT8U MAC[10];//第一字节表长度
}Esam_MAC_RN_NO;//抄读电表时，上报报文解析，用于参数传递

INT32S Esam_Init(INT32S fd);
void Esam_Clear(INT32S fd) ;
INT32S Esam_WriteThenRead( INT8U* Tbuf, INT16U Tlen, INT8U* Rbuf);
INT32S _esam_WriteThenRead(INT32S fd, INT8U* Tbuf, INT16U Tlen, INT8U* Rbuf);
void Esam_WriteToChip(INT32S fd, INT8U* Tbuf, INT16U Tlen);
void Esam_ReadFromChip(INT32S fd, INT8U* Rbuf, INT8U Rlen);
INT32S Esam_ErrMessageCheck(INT8U *RBuf);
INT32U CharToINT32U(INT8U *Buf);
INT32S Esam_GetTermiInfo(EsamInfo* esamInfo);
INT32S Esam_GetTermiSingleInfo(INT8U type, INT8U* Rbuf);
INT32S Esam_CreateConnect(SignatureSecurity* securityInfo ,SecurityData* RetInfo) ;
INT32S Esam_SIDTerminalCheck(SID_MAC SidMac,INT8U* Data, INT8U* Rbuf) ;
INT32S Esam_SIDResponseCheck(INT8U P2type, INT8U* Data3,INT16U Length, INT8U* Rbuf);
INT32S Esam_GetTerminalInfo(INT8U *RN,INT8U* Data1,INT16U Length,INT8U* Rbuf) ;
INT32S Esam_SymKeyUpdate(SID_MAC SidMac,INT8U* Data2);
INT32S Esam_CcieSession(SID sid,INT8U* Data2);
INT32S Esam_ReportEncrypt(INT8U* Data1, INT16U Length,INT8U* RN,INT8U* MAC) ;
INT32S Esam_DencryptReport( INT8U* RN,INT8U* MAC,INT8U* Data3, INT8U* Rbuf);
INT32S Esam_GetRN(INT8U* Rbuf);
INT32S Esam_EmeterDataDencrypt( Esam_MAC_RN_NO* InfoData, INT8U *Data2,INT8U* Rbuf);

#endif /* ESAM_H_ */
