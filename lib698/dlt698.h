

#ifndef DLT698_H_
#define DLT698_H_
#include "Objectdef.h"

extern int getTsas(MY_MS ms, INT8U** tsas);
extern int doObjectAction(OAD oad, INT8U* data, Action_result* result);
extern int StateProcess(CommBlock* nst, int delay_num);

extern int ProcessData(CommBlock *com);
extern int Link_Request(LINK_Request request,INT8U *addr,INT8U *buf);
extern void testframe(INT8U *apdu,int len);
extern INT8U Report_Event(CommBlock *com,Reportevent report_event);
extern INT16U composeAutoReport(INT8U* SendApdu,INT16U length);
extern INT16U  composeAutoTask(AutoTaskStrap* list);

/*----------------------抄表相关*************************/
extern INT16S composeProtocol698_GetRequest(INT8U*, CLASS_6015, TSA);
// OAD转换为报文
extern INT8U OADtoBuff(OAD fromOAD, INT8U* buff);
extern INT8U analyzeProtocol698(INT8U* Rcvbuf, INT8U* resultCount, INT16S recvLen, INT8U* apduDataStartIndex, INT16S* dataLen);
extern void ProxyListResponse(PROXY_GETLIST* list, CommBlock* com);
int createFile(const char* path, int length, unsigned char crc, unsigned short bs);
int appendFile(int shift, int length, unsigned char* buf);

/*规约类型打印
 * */
extern INT8U prtstat(int flg);
extern void printMS(MY_MS ms);
extern void print_road(ROAD road);
extern void print_rcsd(CSD_ARRAYTYPE csds);
extern void print_rsd(INT8U choice, RSD rsd);
/*----------------------接口类及对象实例的基本数据类型组帧----------------------*/
extern int create_OAD(INT8U* data, OAD oad);
extern int create_array(INT8U* data, INT8U numm);
extern int create_struct(INT8U* data, INT8U numm);
extern int file_bool(INT8U* data, INT8U value);
extern int fill_bit_string8(INT8U* data, INT8U bits);
extern int fill_double_long_unsigned(INT8U* data, INT32U value);
extern int fill_octet_string(INT8U* data, char* value, INT8U len);
extern int fill_visible_string(INT8U* data, char* value, INT8U len);
extern int fill_integer(INT8U* data, INT8U value);
extern int fill_unsigned(INT8U* data, INT8U value);
extern int fill_long_unsigned(INT8U* data, INT16U value);
extern int fill_enum(INT8U* data, INT8U value);
extern int fill_time(INT8U* data, INT8U* value);
extern int fill_date_time_s(INT8U* data, DateTimeBCD* time);
extern int fill_TI(INT8U* data, TI ti);
extern int fill_CSD(INT8U type, INT8U* data, MY_CSD csd);
extern int fill_TSA(INT8U* data, INT8U* value, INT8U len);
extern int fill_RCSD(INT8U type, INT8U* data, CSD_ARRAYTYPE csds);
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
extern int get_BasicRCSD(INT8U type, INT8U* source, CSD_ARRAYTYPE* csds);         // 0x60
extern int get_Data(INT8U* source, INT8U* dest);
/*----------------------具体OI类组帧函数----------------------*/
/*----------------------统计相关数据----------------------*/
extern INT8U Get_2200(OI_698 oi, INT8U* sourcebuf, INT8U* buf, int* len);
extern INT8U Get_2203(OI_698 oi, INT8U* sourcebuf, INT8U* buf, int* len);
extern INT8U Get_2204(OI_698 oi, INT8U* sourcebuf, INT8U* buf, int* len);
/*----------------------参变量类----------------------*/
extern int Get_6001(INT8U seqnum, INT8U* data);
extern int Get_6013(INT8U taskid, INT8U* data);
extern int Get_6015(INT8U seqnum, INT8U* data);
extern int Get_6035(INT8U seqnum, INT8U* data);
#endif
