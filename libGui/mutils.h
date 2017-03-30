/*
 * mutils.h
 *
 *  Created on: 2017-3-3
 *      Author: prayer
 */

#ifndef MUTILS_H_
#define MUTILS_H_
#include "../include/StdDataType.h"
#define MAXMPNUM_DATAFILE          50//一个数据文件保存的最大测量点数据个数
extern void write_apn(char* apn);
extern void write_userpwd(unsigned char* user, unsigned char* pwd, unsigned char* apn);
extern INT8S bcd2str(INT8U* bcd,INT8U* str,INT8U bcd_len,INT8U str_size,ORDER order);
extern INT8S str2bcd(INT8U* str,INT8U* bcd,INT8U bcd_len);
extern INT32S asc2bcd(INT8U* asc, INT32U len, INT8U* bcd,ORDER order);
extern INT32S bcd2asc(INT8U* bcd, INT32U len, INT8U* asc,ORDER order);
extern FP64 bcd2double(INT8U *bcd, INT8U len,INT16U decbytes,ORDER order);
extern INT8S bcd2int32u(INT8U *bcd, INT8U len,ORDER order,INT32U* dint);
extern INT16S getDayFilePath(INT16U MpNo,INT16U year,INT8U month,INT8U day,INT8U* fpath,INT16U path_len);
void tmass(TS* ts,INT16U year,INT8U mon,INT8U day,INT8U hour,INT8U min,INT8U sec);
/*TmS转time_t整形事件
 * */
#endif /* MUTILS_H_ */
