

#ifndef DLT698_H_
#define DLT698_H_
#include "Objectdef.h"
#include "Shmem.h"

#define CURVE_INFO_STARTINDEX 10

extern CONNECT_Response *AppVar_p;

extern ProgramInfo *getShareAddr(void);//获取共享内存的地址

extern unsigned short tryfcs16(unsigned char *cp, int  len);
extern int doObjectAction(OAD oad, INT8U* data, Action_result* result);
extern int StateProcess(CommBlock* nst, int delay_num);
extern int ProcessData(CommBlock* com);
extern int Link_Request(LINK_Request request, INT8U* addr, INT8U* buf);
extern void testframe(INT8U* apdu, int len);
extern INT8U Report_Event(CommBlock* com, INT8U *oiarr, INT8U report_type);
extern INT16U composeAutoReport(INT8U* SendApdu, INT16U length);
extern INT16U composeAutoTask(AutoTaskStrap* list);
extern int callAutoReport(char *filename,INT8U reportChoice,CommBlock* com, INT8U ifecho);
extern int callEventAutoReport(CommBlock* com,INT8U *eventbuf,int datalen);
extern int callNotificationReport(CommBlock* com,INT8U *plcbuf,OAD portOAD,int datalen);
extern int GetReportData(CLASS_601D report);
extern int doGetnormal(INT8U seqOfNum,RESULT_NORMAL *response);

extern INT8U Reset_add();
extern void FrameTail(INT8U *buf, int index, int hcsi);
extern int FrameHead(CSINFO *csinfo, INT8U *buf);
extern INT8S (*pSendfun)(int fd, INT8U *sndbuf, INT16U sndlen);
extern void Get698_event(OAD oad, ProgramInfo *prginfo_event);
extern INT16S composeSecurityResponse(INT8U* SendApdu,INT16U Length);
extern int CheckHead(unsigned char* buf ,CSINFO *csinfo);
extern int CheckTail(unsigned char * buf,INT16U length);
extern int fillcsinfo(CSINFO *csinfo,INT8U *addr,INT8U clientaddr);
/*----------------------抄表相关*************************/
extern INT16S composeProtocol698_GetRequest(INT8U*, CLASS_6015, TSA);
extern INT16S composeProtocol698_SetRequest(INT8U* ,RESULT_NORMAL,TSA);
extern INT16U composeProtocol698_GetRequestRecord(PROXY_GETLIST *getlist,INT8U *sendbuf);//代理record
INT16S composeProtocol698_GetRequest_RN(INT8U* sendBuf, CLASS_6015 obj6015,TSA meterAddr);
extern INT16S composeProtocol698_SetActionRequest(INT8U* sendBuf,INT8U type,ACTION_SET_OBJ setOBJ);
extern INT16S composeProtocol698_SetActionThenGetRequest(INT8U* sendBuf,INT8U type,DO_Then_GET dogetOBJ);
extern TS mylookback(time_t times,TI ti,INT8U n);
extern time_t calcnexttime(TI ti, DateTimeBCD datetime,TI ti_delay);
// OAD转换为报文
//extern INT8U OADtoBuff(OAD fromOAD, INT8U* buff);
extern INT8U analyzeProtocol698(INT8U* Rcvbuf, INT8U* resultCount, INT16S recvLen, INT8U* apduDataStartIndex, INT16S* dataLen);
INT8U analyzeProtocol698_RN(INT8U* Rcvbuf, INT8U* resultCount, INT16S recvLen,INT8U* apduDataStartIndex, INT16S* dataLen);
extern void ProxyListResponse(PROXY_GETLIST* list, CommBlock* com);
int createFile(const char* path, int length, unsigned char crc, unsigned short bs);
int appendFile(int shift, int length, unsigned char* buf);

/*规约类型打印
 * */
extern void printProxyDoThenGet(int tsa_num,DO_Then_GET *doget);
extern void printDataTimeS(char *pro,DateTimeBCD datetimes);
extern void printTI(char *pro,TI ti);
extern void printMS(MY_MS ms);
extern void printTSA(TSA tsa);
extern void print_road(ROAD road);
extern void print_rcsd(CSD_ARRAYTYPE csds);
extern void print_rsd(INT8U choice, RSD rsd);
extern void print4500(CLASS25 class4500);
/**/
extern void setOIChange(OI_698 oi);
/*----------------------接口类及对象实例的基本数据类型组帧----------------------*/
extern int fill_timetag(INT8U *data,TimeTag timetag);
extern int create_array(INT8U* data, INT8U numm);					//0x01
extern int create_struct(INT8U* data, INT8U numm);					//0x02
extern int fill_bool(INT8U* data, INT8U value);						//0x03
extern int fill_bit_string(INT8U *data,INT8U size,INT8U *bits);		//0x04
extern int fill_double_long(INT8U *data,INT32S value);				//0x05
extern int fill_double_long_unsigned(INT8U* data, INT32U value);	//0x06
extern int fill_octet_string(INT8U* data, char* value, INT8U len);	//0x09
extern int fill_visible_string(INT8U* data, char* value, INT8U len);//0x0a
extern int fill_integer(INT8U* data, INT8U value);					//0x0f
extern int fill_long(INT8U *data,INT16U value);						//0x10
extern int fill_unsigned(INT8U* data, INT8U value);					//0x11
extern int fill_long_unsigned(INT8U* data, INT16U value);			//0x12
extern int fill_long64(INT8U *data,INT64S value);					//0x14
extern int fill_long64_unsigned(INT8U *data,INT64U value);			//0x15
extern int fill_enum(INT8U* data, INT8U value);						//0x16
extern int fill_time(INT8U* data, INT8U* value);					//0x1b
extern int fill_date_time_s(INT8U* data, DateTimeBCD* time);		//0x1c
extern int fill_OI(INT8U *data,INT8U value);                        //0x50
extern int create_OAD(INT8U type,INT8U* data, OAD oad);				//0x51
extern int fill_ROAD(INT8U type,INT8U *data,ROAD road);				//0x52
extern int fill_TI(INT8U* data, TI ti);								//0x54
extern int fill_TSA(INT8U* data, INT8U* value, INT8U len);			//0x55
extern int fill_Scaler_Unit(INT8U* data, Scaler_Unit su);			//0x59
extern int fill_RSD(INT8U choice,INT8U *data,RSD rsd);				//0x5A
extern int fill_CSD(INT8U type, INT8U* data, MY_CSD csd);			//0x5b
extern int fill_MS(INT8U type,INT8U *data,MY_MS myms);				//0x5C
extern int fill_COMDCB(INT8U type,INT8U *data,COMDCB comdcb);		//0x5F
extern int fill_RCSD(INT8U type, INT8U* data, CSD_ARRAYTYPE csds);	//0x60
extern int fill_Data(INT8U type,INT8U *data,INT8U *value);
/*----------------------接口类及对象实例的数据类型解析----------------------*/
extern int getArray(INT8U *source,INT8U *dest,INT8U *DAR);                                  // 1
extern int getStructure(INT8U *source,INT8U *dest,INT8U *DAR);                              // 2
extern int getBool(INT8U* source, INT8U* dest,INT8U *DAR);                                   // 3
extern int getBitString(INT8U type, INT8U* source, INT8U* dest);                  // 4
extern int getDouble(INT8U* source, INT8U* dest);                                 // 5 6
extern int getOctetstring(INT8U type, INT8U* source, INT8U* tsa,INT8U *DAR);                 // 9 and 0x55
extern int getVisibleString(INT8U *source,INT8U *dest,INT8U *DAR);                          // 0x0A
extern int getInteger(INT8U *source,INT8S *dest,INT8U *DAR);                     // 0x0F
extern int getUnsigned(INT8U *source,INT8U *dest,INT8U *DAR);                     // 0x11
extern int getLongUnsigned(INT8U* source, INT8U* dest);                           // 0x12
extern int getLong64(INT8U *source,INT64U *dest);									//0x14
extern int getEnum(INT8U type, INT8U* source, INT8U* enumvalue);                  // 0x16
extern int getTime(INT8U type,INT8U *source,INT8U *dest,INT8U *DAR);              // 0x1B
extern int getDateTimeS(INT8U type,INT8U *source,INT8U *dest,INT8U *DAR);         // 0x1C
extern int getOI(INT8U type, INT8U* source, OI_698 *oi);                           // 0x50
extern int getOAD(INT8U type,INT8U *source,OAD *oad,INT8U *DAR);                   // 0x51
extern int getROAD(INT8U* source, ROAD* dest);                                    // 0x52
extern int getTI(INT8U type, INT8U* source, TI* ti);                              // 0x54
extern int get_BasicRSD(INT8U type, INT8U* source, INT8U* dest, INT8U* seletype); // 0x5A
extern int getCSD(INT8U type, INT8U* source, MY_CSD* csd);                        // 0X5B
extern int getMS(INT8U type, INT8U* source, MY_MS* ms);                           // 0x5C
extern int getCOMDCB(INT8U type, INT8U* source, COMDCB* comdcb,INT8U *DAR);       // 0x5F
extern int get_BasicRCSD(INT8U type, INT8U* source, CSD_ARRAYTYPE* csds);         // 0x60
extern int get_Data(INT8U* source, INT8U* dest);								//根据类型返回数据的长度
extern int getSel_Data(INT8U type,INT8U *seldata,INT8U *destdata);				//根据selector类型的参数返回读取的数据
/*
 * 根据数据类型返回相应的数据长度
 * */
extern int getDataTypeLen(int dt);
/*----------------------具体OI类组帧函数----------------------*/
/*----------------------统计相关数据----------------------*/
extern INT8U Get_Vacs(RESULT_NORMAL *response,ProgramInfo* prginfo_acs);
extern int  fill_variClass(OAD oad,INT8U getflg,INT8U *sourcebuf,INT8U *destbuf,INT16U *len,ProgramInfo* proginfo);
extern int fill_pulseEnergy(INT8U devicetype,INT8U index,OAD oad,INT8U *destbuf,INT16U *len);
extern int GetFileState(RESULT_NORMAL* response);

/*----------------------规约一致性 数据有效性判断接口----------------------*/
extern INT8U check_date(int year, int month, int day, int hour, int min, int sec);
extern INT8U getEnumValid(INT16U value,INT16U start,INT16U end,INT16U other);
extern INT8U getCOMDCBValid(COMDCB comdcb);
extern void isTimeTagEffect(TimeTag timetag,TimeTag *rec_timetag);
extern INT8U getPortValid(OAD oad);
extern INT8U DataTimeCmp(DateTimeBCD startdt,DateTimeBCD enddt);
extern int limitJudge(char *desc,int limit,int val);
extern int rangeJudge(char *desc,int val,int min,int max);
#endif
