

#ifndef DLT698_H_
#define DLT698_H_
#include "Objectdef.h"
#include "Shmem.h"

#define CURVE_INFO_STARTINDEX 10

extern CONNECT_Response *AppVar_p;

extern unsigned short tryfcs16(unsigned char *cp, int  len);
extern int doObjectAction(OAD oad, INT8U* data, Action_result* result);
extern int StateProcess(CommBlock* nst, int delay_num);

extern int ProcessData(CommBlock* com);
extern int Link_Request(LINK_Request request, INT8U* addr, INT8U* buf);
extern void testframe(INT8U* apdu, int len);
extern INT8U Report_Event(CommBlock* com, INT8U *oiarr, INT8U report_type,INT8U *com_flag);
extern INT16U composeAutoReport(INT8U* SendApdu, INT16U length);
extern INT16U composeAutoTask(AutoTaskStrap* list);
extern int callAutoReport(INT8U reportChoice,CommBlock* com, INT8U ifecho);
extern int callEventAutoReport(CommBlock* com,INT8U *eventbuf,int datalen);
extern int GetReportData(CLASS_601D report);


/*----------------------抄表相关*************************/
extern INT16S composeProtocol698_GetRequest(INT8U*, CLASS_6015, TSA);
extern INT16S composeProtocol698_SetRequest(INT8U* ,RESULT_NORMAL,TSA);
extern TS mylookback(time_t times,TI ti,INT8U n);
extern time_t calcnexttime(TI ti, DateTimeBCD datetime,TI ti_delay);
// OAD转换为报文
extern INT8U OADtoBuff(OAD fromOAD, INT8U* buff);
extern INT8U analyzeProtocol698(INT8U* Rcvbuf, INT8U* resultCount, INT16S recvLen, INT8U* apduDataStartIndex, INT16S* dataLen);
extern void ProxyListResponse(PROXY_GETLIST* list, CommBlock* com);
int createFile(const char* path, int length, unsigned char crc, unsigned short bs);
int appendFile(int shift, int length, unsigned char* buf);

/*规约类型打印
 * */
extern void printDataTimeS(char *pro,DateTimeBCD datetimes);
extern void printTI(char *pro,TI ti);
extern void printMS(MY_MS ms);
extern void printTSA(TSA tsa);
extern void print_road(ROAD road);
extern void print_rcsd(CSD_ARRAYTYPE csds);
extern void print_rsd(INT8U choice, RSD rsd);
/**/
extern int getTItoSec(TI ti);
extern void setOIChange(OI_698 oi);
/*----------------------接口类及对象实例的基本数据类型组帧----------------------*/

extern int create_array(INT8U* data, INT8U numm);			//0x01
extern int create_struct(INT8U* data, INT8U numm);			//0x02
extern int fill_bool(INT8U* data, INT8U value);				//0x03
extern int fill_bit_string8(INT8U* data, INT8U bits);		//0x04
extern int fill_double_long(INT8U *data,INT32S value);		//0x05
extern int fill_double_long_unsigned(INT8U* data, INT32U value);	//0x06
extern int fill_octet_string(INT8U* data, char* value, INT8U len);	//0x09
extern int fill_visible_string(INT8U* data, char* value, INT8U len);//0x0a
extern int fill_integer(INT8U* data, INT8U value);					//0x0f
extern int fill_long(INT8U *data,INT16U value);						//0x10
extern int fill_unsigned(INT8U* data, INT8U value);					//0x11
extern int fill_long_unsigned(INT8U* data, INT16U value);			//0x12
extern int fill_enum(INT8U* data, INT8U value);						//0x16
extern int fill_time(INT8U* data, INT8U* value);					//0x1b
extern int fill_date_time_s(INT8U* data, DateTimeBCD* time);		//0x1c
extern int create_OAD(INT8U type,INT8U* data, OAD oad);				//0x51
extern int fill_ROAD(INT8U type,INT8U *data,ROAD road);				//0x52
extern int fill_TI(INT8U* data, TI ti);								//0x54
extern int fill_TSA(INT8U* data, INT8U* value, INT8U len);			//0x55
extern int fill_RSD(INT8U choice,INT8U *data,RSD rsd);				//0x5A
extern int fill_CSD(INT8U type, INT8U* data, MY_CSD csd);			//0x5b
extern int fill_MS(INT8U type,INT8U *data,MY_MS myms);				//0x5C
extern int fill_RCSD(INT8U type, INT8U* data, CSD_ARRAYTYPE csds);	//0x60
extern int fill_Data(INT8U type,INT8U *data,INT8U *value);
/*----------------------接口类及对象实例的数据类型解析----------------------*/
extern int getArray(INT8U* source, INT8U* dest);                                  // 1
extern int getStructure(INT8U* source, INT8U* dest);                              // 2
extern int getBool(INT8U* source, INT8U* dest);                                   // 3
extern int getBitString(INT8U type, INT8U* source, INT8U* dest);                  // 4
extern int getDouble(INT8U* source, INT8U* dest);                                 // 5 6
extern int getOctetstring(INT8U type, INT8U* source, INT8U* tsa);                 // 9
extern int getVisibleString(INT8U* source, INT8U* dest);                          // 0x0A
extern int getUnsigned(INT8U* source, INT8U* dest);                               // 0x11
extern int getLongUnsigned(INT8U* source, INT8U* dest);                           // 0x12
extern int getEnum(INT8U type, INT8U* source, INT8U* enumvalue);                  // 0x16
extern int getTime(INT8U type, INT8U* source, INT8U* enumvalue);                  // 0x1B
extern int getDateTimeS(INT8U type, INT8U* source, INT8U* dest);                  // 0x1C
extern int getOI(INT8U type, INT8U* source, OI_698 oi);                           // 0x50
extern int getOAD(INT8U type, INT8U* source, OAD* oad);                           // 0x51
extern int getROAD(INT8U* source, ROAD* dest);                                    // 0x52
extern int getTI(INT8U type, INT8U* source, TI* ti);                              // 0x54
extern int get_BasicRSD(INT8U type, INT8U* source, INT8U* dest, INT8U* seletype); // 0x5A
extern int getCSD(INT8U type, INT8U* source, MY_CSD* csd);                        // 0X5B
extern int getMS(INT8U type, INT8U* source, MY_MS* ms);                           // 0x5C
extern int getCOMDCB(INT8U type, INT8U* source, COMDCB* comdcb);                  // 0x5F
extern int get_BasicRCSD(INT8U type, INT8U* source, CSD_ARRAYTYPE* csds);         // 0x60
extern int get_Data(INT8U* source, INT8U* dest);
extern int getSel_Data(INT8U type,INT8U *seldata,INT8U *destdata);				//根据selector类型的参数返回读取的数据
/*
 * 根据数据类型返回相应的数据长度
 * */
extern int getDataTypeLen(int dt);
/*----------------------具体OI类组帧函数----------------------*/
/*----------------------统计相关数据----------------------*/
extern INT8U Get_Vacs(RESULT_NORMAL *response,ProgramInfo* prginfo_acs);


extern int GetFileState(RESULT_NORMAL* response);
#endif
