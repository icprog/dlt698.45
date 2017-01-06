/*
 * read485.c
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
#include "read485.h"

void read485_thread()
{
  while(1){

  }
  pthread_detach(pthread_self());
  pthread_exit(&thread_read485);
}

void read485_proccess()
{
	pthread_attr_init(&read485_attr_t);
	pthread_attr_setstacksize(&read485_attr_t,2048*1024);
	pthread_attr_setdetachstate(&read485_attr_t,PTHREAD_CREATE_DETACHED);
	while ((thread_read485_id=pthread_create(&thread_read485, &read485_attr_t, (void*)read485_thread, NULL)) != 0)
	{
		sleep(1);
	}
}
