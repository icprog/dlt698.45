/*
 * v645.c
 *
 *  Created on: 2014-2-28
 *      Author: Administrator
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include "StdDataType.h"
#include "Shmem.h"
#include "PublicFunction.h"
#include "def645.h"

#define READ_BUF_SIZE 256
extern INT32S comfd;
extern INT8U addr[6];//表地址，低在前，高在后
extern void dealProcess();
extern void acs_process();

ProgramInfo *JProgramInfo = NULL;

int ProIndex = 0;

//处理现场
void QuitProcess() {
    if (comfd > 0) close(comfd);
    shmm_unregister("ProgramInfo", sizeof(ProgramInfo));
    fprintf(stderr, "\n\r cj645 quit xxx\n\r");
    exit(0);
}

int InitPro(ProgramInfo **prginfo, int argc, char *argv[]) {
        *prginfo = OpenShMem("ProgramInfo", sizeof(ProgramInfo), NULL);
        ProIndex = atoi(argv[1]);
        fprintf(stderr, "\n%s start", (*prginfo)->Projects[ProIndex].ProjectName);
        (*prginfo)->Projects[ProIndex].ProjectID = getpid();//保存当前进程的进程号
        fprintf(stderr, "ProjectID[%d]=%d\n", ProIndex, (*prginfo)->Projects[ProIndex].ProjectID);
        return 1;
 }

INT32S prog_getcmdline(long pid,INT8U* cmdl)
{
  if(cmdl == NULL)
		return -1;
  char n_file[128];
  char buff[128];
  int file;
  char c;
  int i=0;
  memset(n_file,0,128);
  memset(buff,0,128);
  sprintf(n_file,"/proc/%ld/cmdline",pid);
  file = open(n_file, O_RDONLY);
//  fprintf(stderr,"\n");
//  fgets(buff,128,file);
  while(read(file,&c,1)!=0 && i<128)
  {
	  if(c !=0)
		  buff[i++]=c;
	  else
		  buff[i++]=' ';
//	  if(i-1>=0)
//	  fprintf(stderr,"%d|",buff[i-1]);
  }
  close(file);
//  fprintf(stderr,"\nbuff1:%s/i=%d",buff,i);
  i--;
  while(buff[i] == ' ' && i>0)
  {
	  buff[i--]=0;
	  if(i>=0 && buff[i]!=' ')
		  break;
  }
//  fprintf(stderr,"\nbuff2:%s/",buff);
  memcpy(cmdl,buff,strlen(buff));
  return strlen(buff);
}

/*通过带参数的命令
 * */
long prog_getsyspid_bycmd(char* cmdline)
{
   DIR *dir;
   struct dirent *next;
   long pid=-1;

   ///proc中包括当前的进程信息,读取该目录
   dir = opendir("/proc");
   if (!dir)
   {
	   printf("cannot open /proc");
   }

   //遍历
   while ((next = readdir(dir)) != NULL)
   {
	   INT8U buffer[READ_BUF_SIZE];
	   int len_buff=0;

	   if (strcmp(next->d_name, ".") == 0)
	  	   continue;
	   /* Must skip ".." since that is outside /proc */
	   if (strcmp(next->d_name, "..") == 0)
		   continue;
	   /* If it isn't a number, we don't want it */
	   if (!isdigit(*next->d_name))
		   continue;
	   if (atoi(next->d_name) == 1)//process init
		   continue;
	   if (strcmp(next->d_name, "telnetd") == 0)
		   continue;
	   if (next->d_name[0]=='[')
	  	   continue;
	   memset(buffer,0,READ_BUF_SIZE);
	   len_buff =prog_getcmdline(atoi(next->d_name),buffer);
//	   fprintf(stderr,"\nbuffer:%s,len_buff=%d",buffer,len_buff);
//	   if (len_buff>0 && strcmp((const char*)buffer, (const char*)cmdline) == 0)
//	   {
//		   pid = strtol(next->d_name, NULL, 0);
//		   break;
//	   }
//	   if (len_buff>0 && strncmp((const char*)buffer, (const char*)cmdline,len_buff) == 0)
	   if (len_buff>0 && strncmp((const char*)buffer,
			   (const char*)cmdline,strlen((const char*)cmdline)) == 0)
	   {
//		   fprintf(stderr,"\n[prog_getsyspid_bycmd]:dname=%s,cmdline=%s",next->d_name,buffer);
		   pid = strtol(next->d_name, NULL, 0);
		   break;
	   }
   }
   closedir(dir);
   return pid;
}

BOOLEAN pgpl_isExist(INT32S pid_sys)
{
   BOOLEAN ret = FALSE;
   INT8S path[128];
   FILE* fp = NULL;
   if(pid_sys > 0)
   {
		memset(path,0,128);
		sprintf((char*)path,"/proc/%d/status",pid_sys);
		fp = fopen((const char*)path,"r");
//	    fprintf(stderr,"\npath=%s,fp=%d",path,fp);
		if(fp == NULL)
		{
		   ret = FALSE;
		}
		else
		{
			char aline[128];
			memset(aline,0,128);

			while(fgets(aline,128,fp) != NULL)
			{
				if(strncmp(aline,"State:",6) == 0  )
				{
					if(aline[7]=='Z')//防止僵死进程
						kill(pid_sys,SIGKILL);
					break;
				}
				memset(aline,0,128);
			}
		   fclose(fp);
		   fp = NULL;
		   ret = TRUE;
		}
   }
   return ret;
}

long check_cjcomm()
{
	long pid_sys = prog_getsyspid_bycmd("cjcomm 2");
	if(pid_sys > 0)
	{
		fprintf(stderr,"cjcomm 2 pid_sys=%d\n",pid_sys);
	}
	return pid_sys;
}
/*
 * 485口通信测试,两个串口互相发送接收
 * II型集中器, port1 = 1, port2 = 4 ,485_I 与 485_III 互相抄读
 *
 * */
int  vs485_test(int port1,int port2)
{
//	int Test_485_result = 1;
//    INT8U msg[256];
//    INT8U res[256];
//    int lens =0, wlen=0;
//
//
//    int comfd1 = OpenCom(port1, 9600, (INT8U *) "even", 1, 8);
//    int comfd2 = OpenCom(port2, 9600, (INT8U *) "even", 1, 8);
//
//    for (int i = 0; i < 256; ++i) {
//        msg[i] = i;
//    }
//    wlen = write(comfd1, msg, 178);//177
//    sleep(1);
//    lens = read(comfd2, res, 178);
//    printf("write %d 收到数据[%d]字节\n", wlen,lens);
//
//    for (int j = 0; j < lens; ++j) {
//        if (msg[j] != res[j]) {
//            Test_485_result = 0;
//            fprintf(stderr,"j=%d %02x_%02x\n",j,msg[j],res[j]);
//            fprintf(stderr,"!!!!!!!!!!!!!!!!!!!!!!j=%d  error\n",j);
//        }
//    }
//
//    memset(msg, 0x00, sizeof(msg));
//    memset(res, 0x00, sizeof(res));
//    msg[0]	= 0x55;
//    write(comfd1, msg,1);
//    sleep(1);
//    lens = read(comfd2, res, 1);
//    fprintf(stderr,"write %02x, read %02x lens=%d\n",msg[0],res[0],lens);
//
//    memset(msg, 0x00, sizeof(msg));
//    memset(res, 0x00, sizeof(res));
//    msg[0]	= 0x22;
//    write(comfd1, msg,1);
//    sleep(1);
//    lens = read(comfd2, res, 1);
//    fprintf(stderr,"write %02x, read %02x lens=%d\n",msg[0],res[0],lens);
//
//
//    sleep(5);
//    memset(msg, 0x00, sizeof(msg));
//    memset(res, 0x00, sizeof(res));
//    msg[0]	= 0xaa;
//    wlen = write(comfd2, msg,1);
//    sleep(1);
//    lens = read(comfd1, res, 2);
//    fprintf(stderr,"com2->com1 write %02x, read %02x_%02x wlen=%d lens=%d\n",msg[0],res[0],res[1],wlen,lens);
//
//    return Test_485_result;

    int Test_485_result = 1;
    int lens =0;

    INT8U msg[256];
    INT8U res[256];

    memset(msg, 0x00, sizeof(msg));
    memset(res, 0x00, sizeof(res));

    int comfd1 = OpenCom(port1, 9600, (INT8U *) "even", 1, 8);
    int comfd2 = OpenCom(port2, 9600, (INT8U *) "even", 1, 8);

    for (int i = 0; i < 256; ++i) {
        msg[i] = i;
    }
    fprintf(stderr,"msg_len = %d  res_len = %d\n",sizeof(msg),sizeof(res));

    write(comfd1, msg, sizeof(msg));
    sleep(1);
    lens = read(comfd2, res, sizeof(res));
    printf("收到数据[%d]字节\n", lens);

    for (int j = 0; j < 256; ++j) {
        if (msg[j] != res[j]) {
            Test_485_result = 0;
            fprintf(stderr,"j=%d %02x_%02x\n",j,msg[j],res[j]);
            fprintf(stderr,"!!!!!!!!!!!!!!!!!!!!!!j=%d  error\n",j);
        }
    }

    fprintf(stderr,"comm2 to comm3 end!\n");

    close(comfd1);
    close(comfd2);

    comfd1 = OpenCom(port1, 9600, (INT8U *) "even", 1, 8);
    comfd2 = OpenCom(port2, 9600, (INT8U *) "even", 1, 8);

    memset(msg, 0x00, sizeof(msg));
    memset(res, 0x00, sizeof(res));

    msg[0]	= 0x55;
    write(comfd1, msg,1);
    sleep(1);
    lens = read(comfd2, res, 2);
    fprintf(stderr,"write 0x55, read %02x lens=%d\n",res[0],lens);

    for (int i = 0; i < 256; ++i) {
        msg[i] = i;
    }
    write(comfd2, msg, sizeof(msg));
    sleep(1);
    read(comfd1, res, sizeof(res));

    for (int j = 0; j < 256; ++j) {

        if (msg[j] != res[j]) {
            Test_485_result = 0;
            fprintf(stderr,"j=%d %02x_%02x    ",j,msg[j],res[j]);
            fprintf(stderr,"!!!!!!!!!!!!!!!!!!!!!!j=%d  error\n",j);
        }
    }

    fprintf(stderr,"comm3 to comm2 end!\n");
    close(comfd1);
    close(comfd2);

    return Test_485_result;
}


//主程序
int main(int argc, char *argv[])
{
    int		Test_485_result=0;
    INT8U 	comport = 2;
    long	cjcomm_pid=0;
    int		i=0;
	struct tm tm_curr;
	char	cmd[64]={};
    system("rm /nand/check.log");
    sleep(1);

    system("echo  出厂功能检测时间 > /nand/check.log");
    system("date >> /nand/check.log");
    fprintf(stderr, "\ncj645 start Checking....\n\r");
    fprintf(stderr,"\n===========================\nstep1:停止进程cjdeal(为了U盘功能检测)\n===========================\n");
    system("pkill cjdeal");
    fprintf(stderr,"\n===========================\nstep2:停止进程cjcomm(为了检测485口)\n===========================\n");
    for(;;) {
		cjcomm_pid = check_cjcomm();
		if(cjcomm_pid>0) {
			syslog(LOG_NOTICE,"cj645检测到cjcomm运行,pid=%ld,正在停止程序!!!!!!!!!!!!\n ",cjcomm_pid);
			memset(&cmd,0,sizeof(cmd));
			sprintf(cmd,"kill -9 %ld",cjcomm_pid);
			fprintf(stderr,"cmd=%s\n",cmd);
			system(cmd);
			sleep(1);
			system("pkill cjdeal");
	    	if(pgpl_isExist(cjcomm_pid)==FALSE) {
	    		fprintf(stderr,"cjcomm 已退出\n");
	    		break;
	    	}
		}
    	usleep(50000);
    }
    sleep(1);
    fprintf(stderr,"\n===========================\nstep3:485 串口互发测试\n===========================\n");
    Test_485_result = vs485_test(1,4);
    if (Test_485_result == 1) {
        system("echo 485OK >> /nand/check.log");
    }
//    else {
//    	 system("echo 485 ERROR!!!!!! >> /nand/check.log");
//    	 fprintf(stderr,"485 ERROR!!!!!!\n");
//    }
    fprintf(stderr,"\n===========================\nstep4:ESAM 功能测试\n===========================\n");
    system("cj esam 2>> /nand/check.log");

    ///////
    JProgramInfo = OpenShMem("ProgramInfo", sizeof(ProgramInfo), NULL);
    for (int j = 0; j < 5; ++j) {
        JProgramInfo->Projects[j].WaitTimes = 0;
    }

    if (JProgramInfo->cfg_para.device == 2) {    //II型集中器
        comport = 2;
    } else {
        comport = 4;
    }

    fprintf(stderr,"\n===========================\nstep5:645脚本通信打开串口 open /dev/ttyS%d\n===========================\n",comport);
    if ((comfd = OpenCom(comport, 2400, (INT8U *) "even", 1, 8)) < 1) {
        fprintf(stderr, "OpenCom645 ERR!!! ........................\n");
    }

    fprintf(stderr,"\n===========================\nstep6:运行cjcomm(为了1.红外测试通信 2.cj checkled发送报文来控制本地灯指示功能)\n===========================\n");
    JProgramInfo->Projects[CjDealIndex].WaitTimes = 0;
    system("cjcomm 2 &");
    for(i=0;i<60;i++) {
    	JProgramInfo->Projects[CjDealIndex].WaitTimes = 0;
		cjcomm_pid = check_cjcomm();
		if(cjcomm_pid>0) {
			syslog(LOG_NOTICE,"cj645调用cjcomm成功运行................,pid=%ld\n ",cjcomm_pid);
			break;
		}
		sleep(1);
    }
    acs_process();		//交采线程,实时计量数据,为了精度检测
    dealProcess();

    usleep(300*1000);

    QuitProcess(0);
    return EXIT_SUCCESS;
}
