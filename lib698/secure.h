/*
 * secure.h
 *
 *  Created on: 2017-1-11
 *      Author: gk
 */

#ifndef SECURE_H_
#define SECURE_H_
#include "../include/StdDataType.h"

INT8S UnitParse(INT8U* source,INT8U* dest,INT8U type);
 INT16S secureGetAppDataUnit(INT8U* apdu);
 INT16S secureEsamCheck(INT32S fd,INT8U* apdu,INT8U* retData);
 INT32S secureResponseData(INT8U* RN,INT8U* apdu);
 INT32S secureDecryptDataDeal(INT32S fd,INT8U* apdu,INT8U* secureType,INT8U* MAC);
 INT32S secureEncryptDataDeal(INT32S fd,INT8U* secureType,INT8U* apdu,INT8U* retData);
 INT16U getEsamAttribute(OAD oad,INT8U *retBuff);
#endif /* SECURE_H_ */
