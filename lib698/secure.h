/*
 * secure.h
 *
 *  Created on: 2017-1-11
 *      Author: gk
 */

#ifndef SECURE_H_
#define SECURE_H_
#include "../include/StdDataType.h"

INT32S UnitParse(INT8U* source,INT8U* dest,INT8U type);
// INT16S secureGetAppDataUnit(INT8U* apdu);
 //INT16S secureEsamCheck(INT32S fd,INT8U* apdu,INT8U* retData);
 INT32S secureResponseData(INT8U* RN,INT8U* apdu);
 INT32S secureDecryptDataDeal(INT8U* apdu);
 INT32S secureCheckDataSidMac( INT8U* apdu, INT16U appLen);
 INT32S secureEncryptDataDeal(INT8U* apdu,INT8U* retData);
 INT16U getEsamAttribute(OAD oad,INT8U *retBuff);
 INT32S esamMethodKeyUpdate(INT8U *Data2);
 INT32S esamMethodCcieSession(INT8U *Data2);
 INT32S compose_EnData( INT8U* SendApdu,INT16U Length);
 INT32S compose_DataAndMac(INT8U* SendApdu,INT16U Length);
 INT32S compose_EnDataAndMac(INT8U* SendApdu,INT16U Length);
 INT32S esamMeterGetRN(INT8U *RN);
 //INT32S esamMeterDataParse(Esam_MAC_RN_NO* InfoData, INT8U *apdu,INT8U* Rbuf);

#endif /* SECURE_H_ */
