/*
 * eventcalc.c
 *
 *  Created on: 2017-1-4
 *      Author: wzm
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>
#include "sys/reboot.h"
#include <wait.h>
#include <errno.h>
#include "eventcalc.h"

void eventcalc_thread()
{
  while(1){

  }
  pthread_detach(pthread_self());
  pthread_exit(&thread_eventcalc);
}

void eventcalc_proccess()
{
	pthread_attr_init(&eventcalc_attr_t);
	pthread_attr_setstacksize(&eventcalc_attr_t,2048*1024);
	pthread_attr_setdetachstate(&eventcalc_attr_t,PTHREAD_CREATE_DETACHED);
	while ((thread_eventcalc_id=pthread_create(&thread_eventcalc, &eventcalc_attr_t, (void*)eventcalc_thread, NULL)) != 0)
	{
		sleep(1);
	}
}

