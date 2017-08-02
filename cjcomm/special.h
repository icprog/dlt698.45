/*
 * special.h
 *
 *  Created on: 2017-7-27
 *      Author: zhoulihai
 */

#include "cjcomm.h"
#include "../include/Shmem.h"

#ifndef SPECIAL_H_
#define SPECIAL_H_

void specialClear();
void specialCheckF101Change();
void specialCheck4500Change();
void specialCheck4510Change();
void specialTransFlow();

void StartSecial(struct aeEventLoop *ep, long long id, void *clientData);

#endif /* SPECIAL_H_ */
