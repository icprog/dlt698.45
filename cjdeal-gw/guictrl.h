/*
 * guictrl.h
 *
 *  Created on: 2017-1-4
 *      Author: wzm
 */
#ifndef GUICTRL_H_
#define GUICTRL_H_
#include "../include/ParaDef.h"
#include "../include/StdDataType.h"
#include "../include/ParaDef.h"
#include "../include/att7022e.h"
#include "../libBase/PublicFunction.h"
#include "../libMq/libmmq.h"
#include "../libGui/gui.h"
#include "../libGui/lcd_menu.h"
#include "../libGui/lcm.h"
#include "../libGui/lcd_ctrl.h"
#include "../libGui/show_ctrl.h"
#include "../libGui/lcdprt_jzq.h"
#include "../libGui/comm.h"
#include "../include/Shmem.h"
//#include "../libGui/comm.h"
extern ProgramInfo* JProgramInfo;
#define time_wait_ms(a) usleep((a)*1000)
pthread_attr_t guictrl_attr_t, guishow_attr_t,ctrlmain_attr_t, ctrlsave_attr_t;
int thread_guictrl_id, thread_guishow_id,thread_ctrlsave_id,thread_ctrlmain_id;        //液晶、控制（I型、专变）
pthread_t thread_guictrl, thread_guishow,thread_ctrlsave,thread_ctrlmain;
extern void guictrl_proccess();
#endif /* EVENTCALC_H_ */
