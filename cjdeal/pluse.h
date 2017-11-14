/*
 * pluse.h
 *
 *  Created on: Aug 16, 2017
 *      Author: z
 */

#ifndef PLUSE_H_
#define PLUSE_H_

#include "Shmem.h"
#include "Objectdef.h"
#include "AccessFun.h"

typedef struct {
	int pDD[2];
	int pPQ[2];
	int pNow[2];

	int step[2]; //功率计算步骤
	time_t last_time;  //功率计算周期

	CLASS12 *class12;
} PluseUnit;

int pluseInitUnit(PluseUnit * pu, ProgramInfo* JProgramInfo);
void pluseRefreshUnit(PluseUnit * pu);

#endif /* PLUSE_H_ */
