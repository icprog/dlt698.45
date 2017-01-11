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
 INT16U secureEsamCheck(INT8U* apdu,INT16S appLen,INT8U* retData);

#endif /* SECURE_H_ */
