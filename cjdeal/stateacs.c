/*
 * stateacs.c
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
#include "stateacs.h"

void stateacs_thread()
{
  while(1){

  }
  pthread_detach(pthread_self());
  pthread_exit(&thread_stateacs);
}

void stateacs_proccess()
{
	pthread_attr_init(&stateacs_attr_t);
	pthread_attr_setstacksize(&stateacs_attr_t,2048*1024);
	pthread_attr_setdetachstate(&stateacs_attr_t,PTHREAD_CREATE_DETACHED);
	while ((thread_stateacs_id=pthread_create(&thread_stateacs, &stateacs_attr_t, (void*)stateacs_thread, NULL)) != 0)
	{
		sleep(1);
	}
}
