/*
 * guictrl.c
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
#include "guictrl.h"

void guictrl_thread()
{
  while(1){

  }
  pthread_detach(pthread_self());
  pthread_exit(&thread_guictrl);
}

void guictrl_proccess()
{
	pthread_attr_init(&guictrl_attr_t);
	pthread_attr_setstacksize(&guictrl_attr_t,2048*1024);
	pthread_attr_setdetachstate(&guictrl_attr_t,PTHREAD_CREATE_DETACHED);
	while ((thread_guictrl_id=pthread_create(&thread_guictrl, &guictrl_attr_t, (void*)guictrl_thread, NULL)) != 0)
	{
		sleep(1);
	}
}
