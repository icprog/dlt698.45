#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include "option.h"

#include "PublicFunction.h"
#include "cjcomm.h"

ProgramInfo* JProgramInfo=NULL;

void clearcount(int index)
{
	JProgramInfo->Projects[index].WaitTimes = 0;
}

void QuitProcess(ProjectInfo *proinfo)
{
	proinfo->ProjectID=0;
    fprintf(stderr,"\n退出：%s %d",proinfo->ProjectName,proinfo->ProjectID);
	exit(0);
}
/*********************************************************
 * 进程初始化
 *********************************************************/
int InitPro(ProgramInfo** prginfo, int argc, char *argv[])
{
	if (argc >= 2)
	{
		*prginfo = OpenShMem("ProgramInfo",sizeof(ProgramInfo),NULL);
		(*prginfo)->Index = atoi(argv[1]);
		fprintf(stderr,"\n%s start",(*prginfo)->Projects[(*prginfo)->Index].ProjectName);
		(*prginfo)->Projects[(*prginfo)->Index].ProjectID=getpid();//保存当前进程的进程号
		return 1;
	}
	return 0;
}

void connect_socket(INT8U *server,INT16U serverPort,int *fd){
	char *optval="eth0";
	int    result=0;
	int sock=-1;
    struct sockaddr_in    addr;
    struct timeval timeo;
    unsigned long ul1 = 0;//非阻塞

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) <0)
	{
		return ;
	}
	setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, optval, 4);
	result = ioctl(sock, FIONBIO, (unsigned long*)&ul1);
	if(result< 0)
	{
    	close(sock);
		return ;
	}
	timeo.tv_sec  = 1;
	timeo.tv_usec = 0;
	result = setsockopt(sock,SOL_SOCKET,SO_SNDTIMEO|SO_RCVTIMEO,(char *)&timeo.tv_sec,sizeof(struct timeval));
	if (result < 0)
	{
		close(sock);
		return ;
	}
    bzero(&addr,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_port=htons(serverPort);
    addr.sin_addr.s_addr=inet_addr((char *)server);

    if(connect(sock,(struct sockaddr*)&addr,sizeof(addr))<0)
    {
    	fprintf(stderr,"\n\r socket connect errs [%s]",strerror(errno) );
    	close(sock);
    	sleep(2);
    	return ;
	}
	if (sock>0)
		fprintf(stderr,"\nClient Connection Ok! socket %d\n",sock);
	*fd = sock;
}

INT8S SendPro(int fd,INT8U * buf,INT16U len)
{
	int i=0;
	if(buf == NULL || len == 0)
		return 0;
	fprintf(stderr,"\nNET:S(%d)=",len);
	for(i=0;i<len;i++){
		fprintf(stderr,"%02x ",buf[i]);
	}
	for(i=0;i<len;i++){
	    write(fd,&buf[i],1);
	    usleep(2000);
	}
	return len;
}
void RecvPro(int fd,INT8U* buf,int *head)
{
	INT8U TmpBuf[256]={};
	int len=0,i=0,total=0,revcount=0;

	ioctl(fd,FIONREAD,&revcount);

	len = read(fd,TmpBuf,revcount);
	//len = recv(fd,TmpBuf,revcount,0);
	if(len>0)
	{
		fprintf(stderr,"\nNET:R(%d)=",len);
		for(i=0;i<len;i++) {
			buf[*head]=TmpBuf[i];
			fprintf(stderr,"%02x ",TmpBuf[i]);
			*head = (*head + 1) % BUFLEN;
		}
		total =total + len;
	}
}

void WriteLinkRequest(INT8U link_type,INT16U heartbeat,LINK_Request *link_req)
{
	TS ts={};
	TSGet(&ts);
	link_req->type = link_type;
	link_req->piid_acd.data = 0;
	link_req->time.year = ((ts.Year<<8)&0xff00) | ((ts.Year>>8)&0xff);		//apdu 先高后低
	link_req->time.month = ts.Month;
	link_req->time.day_of_month = ts.Day;
	link_req->time.day_of_week = ts.Week;
	link_req->time.hour = ts.Hour;
	link_req->time.minute = ts.Minute;
	link_req->time.second = ts.Sec;
	link_req->time.milliseconds = 0;
	link_req->heartbeat = ((heartbeat<<8)&0xff00) | ((heartbeat>>8)&0xff);
}

void Comm_task(CommBlock *compara)
{
	int sendlen=0;
	static time_t oldtime=0;
	TS ts={};
	INT16U	heartbeat = 60;

	time_t newtime = time(NULL);
	if (abs(newtime-oldtime)>heartbeat)
	{
		TSGet(&ts);
		oldtime = newtime;
		if (compara->phy_connect_fd < 0 || compara->testcounter >=2)
		{
			fprintf(stderr,"phy_connect_fd = %d ,compara->testcounter = %d\n",compara->phy_connect_fd ,compara->testcounter);
			initComPara(compara);
			return;
		}else if (compara->phy_connect_fd >=0 &&  compara->linkstate == close_connection )//物理通道建立完成后，如果请求状态为close，则需要建立连接
		{
			WriteLinkRequest(build_connection,heartbeat,&compara->link_request);
			fprintf(stderr,"link %d-%0d-%d %d:%d:%d",compara->link_request.time.year,compara->link_request.time.month,compara->link_request.time.day_of_month,compara->link_request.time.hour,compara->link_request.time.minute,compara->link_request.time.second);
			sendlen = Link_Request(compara->link_request,compara->serveraddr,compara->SendBuf);
		}else
		{
			WriteLinkRequest(heart_beat,heartbeat,&compara->link_request);
			sendlen = Link_Request(compara->link_request,compara->serveraddr,compara->SendBuf);
		}

		compara->p_send(compara->phy_connect_fd,compara->SendBuf,sendlen);
		compara->testcounter++;
	}
}

INT8S ComWrite(int fd, INT8U* buf, INT16U len) {
    return write(fd, buf, len);
}

void initComPara(CommBlock *compara)
{
	INT8U addr[16]={0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	memcpy(compara->serveraddr,addr,16);
	compara->phy_connect_fd = -1;
	compara->testcounter = 0;
	compara->linkstate = close_connection;
	memset(compara->RecBuf,0,sizeof(compara->RecBuf));
	memset(compara->SendBuf,0,sizeof(compara->SendBuf));
	memset(compara->DealBuf,0,sizeof(compara->DealBuf));
	compara->RHead = 0;
	compara->RTail = 0;
	compara->deal_step = 0;
	compara->rev_delay = 20;

	compara->p_send = ComWrite;
	compara->p_recv = NULL;
	compara->p_connet = NULL;
}

void *FirComWorker(void* args){
	CommBlock fir_comstat;
	CommBlock com_comstat;

	initComPara(&fir_comstat);
	initComPara(&com_comstat);

	if ((fir_comstat.phy_connect_fd = OpenCom(3, 2400,(unsigned char *)"none",1,8)) <= 0){
		//wrong
	}

	if ((com_comstat.phy_connect_fd = OpenCom(4, 2400,(unsigned char *)"none",1,8)) <= 0){
		//wrong
	}

	while(1){
		int revcount = 0;
		ioctl(fir_comstat.phy_connect_fd, FIONREAD, &revcount);
		if (revcount > 0) {
			for (int j = 0; j < revcount; j++) {
				read(fir_comstat.phy_connect_fd, &fir_comstat.RecBuf[fir_comstat.RHead], 1);
				fir_comstat.RHead = (fir_comstat.RHead + 1)%BUFLEN;
			}

			int len = StateProcess(&comstat.deal_step,&comstat.rev_delay,10,&comstat.RTail,&comstat.RHead,comstat.RecBuf,comstat.DealBuf);
			if(len > 0)
			{
				int apduType = ProcessData(&comstat);
				switch(apduType)
				{
					case LINK_RESPONSE:
						comstat.linkstate = build_connection;
						comstat.testcounter = 0;
						break;
					default:
						break;
				}
			}
		}

		revcount = 0;
		ioctl(com_comstat.phy_connect_fd, FIONREAD, &revcount);

		if (revcount > 0) {
			for (int j = 0; j < revcount; j++) {
				read(com_comstat.phy_connect_fd, &com_comstat.RecBuf[com_comstat.RHead], 1);
				com_comstat.RHead = (com_comstat.RHead + 1)%BUFLEN;
			}

			int len = StateProcess(&comstat.deal_step,&comstat.rev_delay,10,&comstat.RTail,&comstat.RHead,comstat.RecBuf,comstat.DealBuf);
			if(len > 0)
			{
				int apduType = ProcessData(&comstat);
				switch(apduType)
				{
					case LINK_RESPONSE:
						comstat.linkstate = build_connection;
						comstat.testcounter = 0;
						break;
					default:
						break;
				}
			}
		}
	}

	return NULL;
}

void CreateFirComWorker(void){
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	pthread_t temp_key;
	pthread_create(&temp_key, &attr, FirComWorker, NULL);
}

int main(int argc, char *argv[])
{
	INT8U apduType=0;

	TS ts={};
	int len=0;
	struct sigaction sa={};
	if(InitPro(&JProgramInfo,argc,argv)==0){
		fprintf(stderr,"进程 %s 参数错误",argv[0]);
		return EXIT_FAILURE;
	}
	Setsig(&sa,QuitProcess);

	CreateFirComWorker();
	initComPara(&comstat);

	while(1)
	{
		clearcount(JProgramInfo->Index);
		if (comstat.phy_connect_fd <0)
		{
			initComPara(&comstat);
			comstat.p_connet((INT8U *)"192.168.1.179", 5022, &comstat.phy_connect_fd);
		}else
		{
			TSGet(&ts);
			Comm_task(&comstat);
			comstat.p_recv(comstat.phy_connect_fd, comstat.RecBuf,&comstat.RHead);
			len = StateProcess(&comstat.deal_step,&comstat.rev_delay,10,&comstat.RTail,&comstat.RHead,comstat.RecBuf,comstat.DealBuf);
			if(len > 0)
			{
				apduType = ProcessData(&comstat);
				switch(apduType)
				{
					case LINK_RESPONSE:
						comstat.linkstate = build_connection;
						comstat.testcounter = 0;
						break;
					default:
						break;
				}
			}
		}
		delay(50);
	}
	QuitProcess(&JProgramInfo->Projects[JProgramInfo->Index]);
 	return EXIT_SUCCESS;
}
