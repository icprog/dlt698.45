#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/resource.h>
#include "libmmq.h"


mqd_t mmq_create(INT8S * name,struct mq_attr* attr,INT32S flags)
{
	mqd_t fd;
	if (name == NULL || attr == NULL) return -2;
	if (flags != O_RDONLY && flags != O_WRONLY) return -3;
	mq_unlink((const char*)name);
	attr->mq_msgsize += sizeof(mmq_head);
	fd = mq_open((const char*)name , flags | O_CREAT|O_EXCL, 0666, attr);

	return fd;
}

mqd_t mmq_open(INT8S * name,struct mq_attr *attr,INT32S flags)
{
	mqd_t fd;
	if (name == NULL || attr == NULL) return -2;
	if (flags != O_RDONLY && flags != O_WRONLY) return -3;
	fd = mq_open((const char*)name, flags);
	if (fd != -1) {
		if (mq_getattr(fd, attr) == -1) {
			return -4;
		}
	} else {
		return -1;
	}

	return fd;
}

INT32S mmq_get(mqd_t fd, INT32U time_out,mmq_head* msg_head, void* buff)
{
	struct timespec tm;
	int cnt;
	unsigned prio = 0;
	if(fd <0) return -2;
	if (buff == NULL)
		return -3;
	struct mq_attr attr_mq;
	if (mq_getattr(fd, &attr_mq) == -1) {
			return -4;
	}

	if (time_out > 3600 || time_out <0)
		return -5;
	INT8S* pmsg = (INT8S*)malloc(attr_mq.mq_msgsize);
	//fprintf(stderr,"\n mmq_get ---attr_mq.mq_msgsize = %ld\n",attr_mq.mq_msgsize);
	//fprintf(stderr,"\n pmsg = %p\n",pmsg);
	memset(pmsg,0,attr_mq.mq_msgsize);
	if (time_out == 0) {
		cnt = mq_receive(fd,(char*) pmsg, attr_mq.mq_msgsize+1, &prio);
	} else {
//		tm.tv_nsec = 10;
//		time(&tm.tv_sec);
//		tm.tv_sec += time_out;
		tm.tv_nsec = 0;
		time(&tm.tv_sec);
		tm.tv_nsec += 1000*1000*time_out*100;
		cnt = mq_timedreceive(fd,(char*) pmsg, attr_mq.mq_msgsize+1,&prio, &tm);
	}
#if 0
	#endif
	if(cnt >0)
	{
		memcpy(msg_head,pmsg,sizeof(mmq_head));
		memcpy(buff,&pmsg[sizeof(mmq_head)],msg_head->bufsiz);

		fprintf(stderr,"\n\n***********cnt = %d",cnt);
		fprintf(stderr,"\n[libmmq]:pid:%d,cmd=%ld,bufsize = %ld  curmsgs=%d,maxmsg=%ld ",
									msg_head->pid,msg_head->cmd,msg_head->bufsiz,attr_mq.mq_curmsgs,attr_mq.mq_maxmsg);

	}
	if(pmsg != NULL)
	{
	free(pmsg);
	pmsg = NULL;
	}
	return cnt;
}

INT32S mmq_put(mqd_t fd,INT32U time_out,mmq_head msg_head, void * buff, INT8U prio)
{
	int cnt;
	int tmp_prio;
	if (fd < 0)
	{
		return -2;
	}
	struct mq_attr attr_mq;
	if(mq_getattr(fd,&attr_mq) == -1)
	{
		fprintf(stderr,"\n[libmmq]:pid:%d,cmd=%d,mq_getattr failed:%s",
				msg_head.pid,msg_head.cmd,strerror(errno));
		return -3;
	}
	if((attr_mq.mq_curmsgs+1) >= attr_mq.mq_maxmsg)
	{
		fprintf(stderr,"\n消息队列满了　[libmmq]:pid:%d,cmd=%d,mq_curmsgs = %ld mq_maxmsg = %ld",
						msg_head.pid,msg_head.cmd,attr_mq.mq_curmsgs,attr_mq.mq_maxmsg);
		return -5;
	}
	//fprintf(stderr,"\n bufsiz=%d + sizeof(mmq_head)=%d         mq_msgsize=%d",msg_head.bufsiz,sizeof(mmq_head),attr_mq.mq_msgsize);
	if (msg_head.bufsiz+sizeof(mmq_head) >= (INT32U)attr_mq.mq_msgsize)
	{
		fprintf(stderr,"\n[libmmq]:pid:%d,cmd=%d,msg_head.bufsiz+sizeof(mmq_head) >= (INT32U)attr_mq.mq_msgsize",
				msg_head.pid,msg_head.cmd);
		return -4;
	}
	tmp_prio = prio & 0x1f;
	INT8S* pmsg = (INT8S*)malloc(msg_head.bufsiz+sizeof(mmq_head));
	memcpy(pmsg,(void*)&msg_head,sizeof(mmq_head));
	if(buff !=NULL && msg_head.bufsiz >0)
		memcpy((void*)(pmsg+sizeof(mmq_head)),buff,msg_head.bufsiz);
	if(time_out  == 0)
	{
		cnt = mq_send(fd, (char *)pmsg, msg_head.bufsiz+sizeof(mmq_head), tmp_prio);
		if(cnt  == -1)
		{
			fprintf(stderr,"\n[libmmq]:pid:%d,cmd=%d,mq_send failed:%s(curmsgs=%ld,maxmsg=%ld)",
					msg_head.pid,msg_head.cmd,strerror(errno),attr_mq.mq_curmsgs,attr_mq.mq_maxmsg);
		}
	}
	else
	{
		struct timespec tm;
		tm.tv_nsec = 10;
		time(&tm.tv_sec);
		tm.tv_sec += time_out;
		cnt = mq_timedsend(fd, (char *)pmsg, msg_head.bufsiz+sizeof(mmq_head), tmp_prio,&tm);
		if(cnt  == -1)
		{
			fprintf(stderr,"\n[libmmq]:pid:%d,cmd=%d,mq_send failed:%s(curmsgs=%ld,maxmsg=%ld)",
								msg_head.pid,msg_head.cmd,strerror(errno),attr_mq.mq_curmsgs,attr_mq.mq_maxmsg);
		}
	}
	if(pmsg != NULL)
	{
		free(pmsg);
		pmsg = NULL;
	}
	return cnt;
}

INT32S mmq_close(mqd_t fd)
{
	return mq_close(fd);
}

INT8S mqs_send(INT8S* mqname,INT16U pid,INT32U cmd,INT8U* buf,INT32U bufsiz)
{
	mmq_head head;
	mqd_t mqd;
	struct mq_attr attr;
	mqd = mmq_open((INT8S*)mqname , &attr, O_WRONLY);

	fprintf(stderr,"\nmax =%ld   curr =%ld   flags=%ld",attr.mq_maxmsg,attr.mq_curmsgs,attr.mq_flags);

	if(mqd <0)
	{
		fprintf(stderr,"\nmmq_open %s failed!",mqname);
		return -1;
	}
	head.pid = pid;
	head.cmd = cmd;
	head.bufsiz = bufsiz;
	if(mmq_put(mqd,3,head,buf,0) <0)
	{
		fprintf(stderr,"\nmmq_put %s failed!",mqname);
		mmq_close(mqd);
		return -2;
	}
	fprintf(stderr,"\nmq(%s)=%d,mq_curmsgs=%ld,mq_maxmsg=%ld",mqname,mqd,attr.mq_curmsgs,attr.mq_maxmsg);
	mmq_close(mqd);
	return 0;
}

