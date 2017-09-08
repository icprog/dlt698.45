#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/syslog.h>
#include <sys/stat.h>
#include "function_test.h"
#include "zdtest698.h"

#define     UPDATE_FILE		"/nand/update/update.tar"
#define     GZ_FILE			"/nand/update/user.tar.gz"
#define     TAR_FILE		"/nand/update/user.tar"
#define		MD5_FILES		"/nand/update/md5files"
#define 	MD5				"/nand/update/md5"
#define 	DIR_UPDATE		"/nand/update/"
#define 	DIR_BIN			"nand/bin"
#define 	DIR_LIB			"nor/lib"
#define 	DIR_CONFIG		"nor/config"

extern GUI_Point_t Gui_Point;

//去除字符串尾的空格
char* rtrim(char* str)
{
	int n = strlen(str)-1;
	if(n<1)
		return str;
	while(n>0){
		if(*(str+n)==' '){
			*(str+n) = 0;
			n--;
		}else
			break;
	}
	return str;
}
char *revstr(char* str){
	char tmp=0;
	int i;
	int n = strlen(str)-1;
	if(n<1)
		return str;
	for(i=0; i<n/2+1; i++){
		tmp = *(str+i);
		*(str+i) = *(str+n-i);
		*(str+n-i) = tmp;
	}
	return str;
}
//去除字符串头尾的空格
char *rltrim(char* str){
	rtrim(str);
	revstr(str);
	rtrim(str);
	revstr(str);
	return str;
}
//读配置文件  filename：文件名  name:要读取字符串的名称  ret:返回读取的字符串
//成功返回1  失败返回0
int readcfg(char *filename, char *name, char *ret)
{
	FILE *fp;
	char stmp[500];
	char *pstr;
	int strsize=0, succ=0, row=0;
	fp=fopen(filename, "r");
	if(fp == NULL)
		return 0;
	//SdPrint("\n name=%s", name);
	while(!feof(fp))
	{
		row++;
		if(row>50)//为了防止程序在while里死循环
			break;
		memset(stmp, 0, 500);
		if(fgets(stmp, 500, fp) != NULL)//读取一行
		{
			if(memcmp(stmp, "//测试结束", strlen("//测试结束"))==0)//到了文件尾
				break;
			if(stmp[0]=='/' && stmp[1]=='/')
				continue;
			if(stmp[0]==10 || stmp[0]==13)//换行 回车
				continue;
			strsize=strlen(stmp);//计算字符串的长度，不包括(\0)
			if(stmp[strsize-1] == 0x0A){//如果最后一个字符是(\n)
				stmp[strsize-1]='\0';//将最后一个字符改为(\0)
			}
			pstr = NULL;
			pstr = strtok(stmp,"=");//以"="为分割符分成两个字符串
			if(pstr==NULL)//如果该行不包含“=”
				continue;
			rltrim(pstr);//去除字符串首尾的空格
			if(strcmp(pstr, name) == 0){//找到了该字符串
				pstr = strtok(NULL,"=");//取值
				if(pstr==NULL){//如果值为空
					succ=0;//设置返回标志为0
					break;
				}
				rltrim(pstr);//去除字符串头尾的空格
				//strcpy(ret, pstr);//yd
				memcpy(ret, pstr, strlen(pstr));
				succ = 1;
				break;
			}
		}
	}
	fclose(fp);
	return succ;
}
#define OK  		1
#define ERROR  		0
#define UNKNOWN  	-1
extern int u2g(char *inbuf,int inlen,char *outbuf,int outlen);
void lcdprt_result(char *buf, char ok){
	char str[100], tmp_str[100];
	memset(str, 0, 100);
	memcpy(str, buf, strlen(buf));
	int count=0, i, str_len=0;
	memset(tmp_str, 0, 100);
	u2g(str,strlen(str), tmp_str, 100);//utf8一个汉字3个字节 gbk一个汉字2个字节
	str_len = strlen(tmp_str);
	count = 15 - str_len;
	if(count<=0)
		count = 10;
	for(i=0; i<count; i++)
		strcat(str," ");
	if(ok==OK)
		strcat(str,"OK");
	else if(ok==ERROR)
		strcat(str,"ERROR");
	else
		strcat(str,"UNKNOWN");
	Gui_Point.y += ROWSIZE;
	lcd_disp(str, Gui_Point.x, Gui_Point.y);
}
void Esam_test(){
	int ret=0, count=0;
	char port[2];
	PrtTestName("ESAM 测试开始");
	while(ret==0){
		if(readcfg(JZQTEST_PARA_NAME, "esamport", port)==1)
			ret = esam_main(2, (void*)port);
		else
			ret = esam_main(1, NULL);
		count++;
		if(count>2)
			break;
	}
	if(ret==1){
		lcdprt_result("ESAM", OK);
		SdPrint("\n结论: ESAM  OK");
	}else{
		lcdprt_result("ESAM", ERROR);
		SdPrint("\n结论: ESAM  ERROR");
	}
	PrtTestName("ESAM 测试结束");
	return;
}

void USB_test(){
	PrtTestName("USB 测试开始");
	lcdprt_result("USB", OK);
	SdPrint("\n结论: USB  OK");
	PrtTestName("USB 测试结束");
	return;
}

extern int ReceiveFrom485_test(unsigned char *str, int ComPort);
int SendStrTo485(unsigned char *str,unsigned short Len,int Port)
{
	ssize_t re=0;
	re=write(Port,str,Len);
	return re;
}

int ASCToBCD(unsigned char * ASC, int Len, unsigned char *Ret) {
	int i, j, k;
	if (ASC != NULL) {
		for (i = 0; i < Len; i++) {
			if (ASC[i] <= '9' && ASC[i] >= '0')
				ASC[i] = ASC[i] - '0';
			else if(ASC[i] <= 'f' && ASC[i] >= 'a')
			{
				ASC[i] = ASC[i] - 'W';
			}
			else if(ASC[i] <= 'F' && ASC[i] >= 'A')
			{
				ASC[i] = ASC[i] - '7';
			}
		}
		if (Len % 2 == 0) {
			//unsigned char *Ret=new char[Len/2+1];
			memset(Ret, 0, Len / 2 + 1);
			for (j = 0, k = 0; j < Len / 2; j++) {
				*(Ret + j) = (ASC[k] << 4) | ASC[k + 1];
				k++;
				k++;
			}
			//return Ret;
		}
	}
	return 0;
}

int readmeter(int meter, int port, int timeout){
	char flg=0;
	int ComPort=0, i=0, j=0, load_len=0;
	int len=0, recv_count=0;
	char s_port[10], Check=0;
	unsigned char sendbuf[200]={0x68, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x11, 0x04, 0x33, 0x32, 0x34, 0x33, 0xb2, 0x16};
	unsigned char recvbuf[512];
	memset(s_port, 0, 10);
	char s_jcaddr[20];
	char bcd_jcaddr[20];
	memset(s_jcaddr, 0, 20);
	memset(bcd_jcaddr, 0, 20);
	if(readcfg(JZQTEST_PARA_NAME, "jcaddr", s_jcaddr)==1){
		ASCToBCD((unsigned char*)s_jcaddr, 12, (unsigned char*)bcd_jcaddr);
	}
	SdPrint("\n 交采地址=%02x%02x%02x%02x%02x%02x",
			bcd_jcaddr[5], bcd_jcaddr[4], bcd_jcaddr[3],
			bcd_jcaddr[2], bcd_jcaddr[1], bcd_jcaddr[0]);
	memcpy(&sendbuf[1], bcd_jcaddr, 6);
	sendbuf[1] = bcd_jcaddr[5];
	sendbuf[2] = bcd_jcaddr[4];
	sendbuf[3] = bcd_jcaddr[3];
	sendbuf[4] = bcd_jcaddr[2];
	sendbuf[5] = bcd_jcaddr[1];
	sendbuf[6] = bcd_jcaddr[0];
	Check = 0x00;
	for(j=0;j<14;j++)
		Check=Check+sendbuf[j];
	SdPrint("\n sendbuf check=%02x", Check);
	sendbuf[14] = Check;
//	if(JParamInfo3761->group2.f10[meter-1].port==2){
	if(port==2){
		port = 6;
		if(readcfg(JZQTEST_PARA_NAME, "485I_port", s_port)==1)
			port = atoi(s_port);
	}else{
		port = 2;
		if(readcfg(JZQTEST_PARA_NAME, "485II_port", s_port)==1)
			port = atoi(s_port);
	}
	ComPort = OpenCom(port, 2400, (unsigned char *)"even", 1, 8);
	if(ComPort<=0){
		SdPrint("打开串口失败！！！");
		return 0;
	}
	SdPrint("\nReceiveFrom485_test send=%d port=%d",16, port);
	for(j=0; j<16; j++){
		SdPrint(" %02x", sendbuf[j]);
	}
	SdPrint("\n");
	SendStrTo485(sendbuf, 16, ComPort);
	while(1){
		recv_count++;
		if(recv_count>timeout)
			break;
		memset(recvbuf, 0, 512);
		len = ReceiveFrom485_test(recvbuf, ComPort);
		if(len>8){
			SdPrint("\nReceiveFrom485_test recv=%d",len);
			for(j=0; j<len; j++){
				SdPrint(" %02x", recvbuf[j]);
			}
			SdPrint("\n");
			flg = 1;
			break;
		}
	}
	if(flg==1){
		for(i=0;i<len;i++)
		{
			if(recvbuf[i]==0x68)
				break;
		}
		load_len=recvbuf[i+9];
		Check=0x00;
		for(j=0;j<(load_len+10);j++)
		{
			Check=Check+recvbuf[i+j];
//			SdPrint("%x ",recvbuf[i+j]);	//接收
		}
		if(Check!=recvbuf[i+load_len+10]){
			SdPrint("\n check=%d recvbuf_check=%d",Check, recvbuf[i+load_len+10]);
			return 0;
		}
	}
	if(ComPort>0)
		close(ComPort);
	return len;
}

//删除所有测量点 交采除外
void delallmeter(){
	int j=0;
	for (j = 1; j < 6000; j++){
		delay(300);
	}
}

void RS485I_test(){
	PrtTestName("485I 测试开始");
	int ret=0, cldno=2;
	char s_cldno[5];
	memset(s_cldno, 0, 5);
	system("pkill jRead485");
	delay(2000);

	if(readcfg(JZQTEST_PARA_NAME, "485I_cldno", s_cldno)==1)
		cldno = atoi(s_cldno);

	ret = readmeter(cldno, 2, 7);
	if(ret>0){
		SdPrint("\n结论: RS485I   OK");
		lcdprt_result("RS485I", OK);
	}else{
		SdPrint("\n结论: RS485I   ERROR");
		lcdprt_result("RS485I", ERROR);
	}
	PrtTestName("485I 测试结束");
	//测试完485后删除所有有效的测量点 质检要求
	delallmeter();
	return;
}
void RS485II_test(){
	PrtTestName("485II 测试开始");
	int ret=0, cldno=3;
	char buf[100], s_cldno[5];
	memset(buf, 0xee, 100);
	memset(s_cldno, 0, 5);
	system("pkill jRead485");
	delay(2000);
	//system("jSet meter 3-1 3 3 30 2400 0 9 5 1 4");
	//delay(2000);
	if(readcfg(JZQTEST_PARA_NAME, "485II_cldno", s_cldno)==1)
		cldno = atoi(s_cldno);
//	SdPrint("\n cldno:%d addr:%02x%02x%02x%02x%02x%02x", cldno,
//			JParamInfo3761->group2.f10[cldno-1].Address[5],
//			JParamInfo3761->group2.f10[cldno-1].Address[4],
//			JParamInfo3761->group2.f10[cldno-1].Address[3],
//			JParamInfo3761->group2.f10[cldno-1].Address[2],
//			JParamInfo3761->group2.f10[cldno-1].Address[1],
//			JParamInfo3761->group2.f10[cldno-1].Address[0]);
	ret = readmeter(cldno, 3, 7);
	if(ret>0){
		SdPrint("\n结论: RS485II   OK");
		lcdprt_result("RS485II",   OK);
	}else{
		SdPrint("\n结论: RS485II   ERROR");
		lcdprt_result("RS485II",   ERROR);
	}
	PrtTestName("485II 测试结束");
	//测试完485后删除所有有效的测量点 质检要求
	delallmeter();
	//system("jSet meter 3-0");
	return;
}

void softver_test(){
	int i;
	int proj_ver=0;
	char flag=1;
	char kernelver_std[100], cmd[100],*kernelver=NULL;
	char softver[30] = {0};

	PrtTestName("SoftVer 测试开始");
	memset(softver, 0, 30);
	if(readcfg(JZQTEST_PARA_NAME, "softver", softver)==1){

	}else{
		SdPrint("\n softver UNKNOWN");
		lcdprt_result("softver", UNKNOWN);
		flag = 0;
	}

	for(i=0; i < PROJECTCOUNT; i++) {
		if(JProgramInfo->Projects[i].ProjectID!=0) {
			if(memcmp(JProgramInfo->Projects[i].ProjectName, "jLcdTask", strlen("jLcdTask"))==0)
				continue;
			memset(softver, 0, 30);
			proj_ver = 0;
			if(readcfg(JZQTEST_PARA_NAME, (char*)JProgramInfo->Projects[i].ProjectName, softver)==1){
				proj_ver = atoi(softver);

				if(proj_ver){
					SdPrint("\n %s OK", JProgramInfo->Projects[i].ProjectName);
				}else{
//					SdPrint("\n %s ERROR ver=%d okver=%d",
//							JProgramInfo->Projects[i].ProjectName,
//							JProgramInfo->Projects[i].version,
//							proj_ver);
					lcdprt_result((char*)JProgramInfo->Projects[i].ProjectName, ERROR);
					flag = 0;
				}
			}else{
				lcdprt_result((char*)JProgramInfo->Projects[i].ProjectName, UNKNOWN);
				SdPrint("%s UNKNOWN", JProgramInfo->Projects[i].ProjectName);
				flag = 0;
			}
		}
	}

	memset(kernelver_std, 0, 100);
	memset(cmd, 0, 100);
	if(readcfg(JZQTEST_PARA_NAME, "Kernel_Ver", kernelver_std)==1){
		setenv("Kernel_Ver", kernelver_std, 0);
		system("chmod +x $PWD/kernel_check.sh && $PWD/kernel_check.sh");
		kernelver = getenv("Kernel_Ver");
		SdPrint("\n kernelver=%s kernelver_std=%s", kernelver, kernelver_std);
		if (kernelver!=NULL){
			SdPrint("\n Kernel_Ver OK");
//			lcdprt_result("Kernel_Ver", OK);
		}else{
			SdPrint("\n Kernel_Ver ERROR");
			lcdprt_result("Kernel_Ver", ERROR);
			flag = 0;
		}
	}else{
		SdPrint("\n Kernel_Ver UNKNOWN");
		lcdprt_result("Kernel_Ver", UNKNOWN);
		flag = 0;
	}
	//---------------------------------
	if(flag==1){
		SdPrint("\n结论: 软件版本 OK");
		lcdprt_result("软件版本",  OK);
	}else{
		SdPrint("\n结论: 软件版本 ERROR");
		lcdprt_result("软件版本", ERROR);
	}
	PrtTestName("SoftVer 测试结束");
}
/*
 * # ping 193.168.18.173 -q -c 3 -W 2 > ping.log
# cat ping.log
PING 193.168.18.173 (193.168.18.173): 56 data bytes

--- 193.168.18.173 ping statistics ---
3 packets transmitted, 3 packets received, 0% packet loss
round-trip min/avg/max = 1.028/1.150/1.243 ms
# ping 193.168.18.174 -q -c 3 -W 2 > ping.log
# cat ping.log
PING 193.168.18.174 (193.168.18.174): 56 data bytes

--- 193.168.18.174 ping statistics ---
3 packets transmitted, 0 packets received, 100% packet loss
#
 */
void RJ45_test(){
	int rowcount=0;
	char stmp[500];
	int trans_count=0, receive_count=0;
	char tmpstr[10][50];

	char remote_ip[50]={"192.168.0.173"};
	char local_ip[50]={"192.168.0.10"};
	char cmd[100];

	//ping 193.168.18.173 -q -c 3 -W 2 > ping.log
	PrtTestName("RJ45 测试开始");
	if(readcfg(JZQTEST_PARA_NAME, "local_ip", local_ip)==1){
		memset(cmd, 0, 100);
		sprintf(cmd, "ifconfig eth0 %s up", local_ip);
		system(cmd);
	}
	memset(cmd, 0, 100);
	readcfg(JZQTEST_PARA_NAME, "remote_ip", remote_ip);
	SdPrint("\n remote_ip=%s local_ip=%s", remote_ip, local_ip);
	sprintf(cmd, "ping %s -q -c 5 -W 2 > /dev/shm/ping.log", remote_ip);
	system(cmd);

	FILE *fp=NULL;
	fp = fopen("/dev/shm/ping.log", "r");
	if(fp == NULL){
		fprintf(stderr,"\n no ping.log");
		return;
	}
	while(!feof(fp)){
		rowcount ++;
		if(rowcount>300)
			break;
		memset(stmp, 0, 500);
		if(fgets(stmp, 500, fp) != NULL)//读取一行
		{
			if(memcmp(stmp, "//测试结束", strlen("//测试结束"))==0)//到了文件尾
				break;
			if(stmp[0]=='/' && stmp[1]=='/')
				continue;
			if(stmp[0]==10 || stmp[0]==13)//换行 回车
				continue;
			if(strstr(stmp, "packet loss")!=NULL){
//3 packets transmitted, 3 packets received, 0% packet loss
				sscanf(stmp,"%d %s %s %d %s %s %s %s %s",
						&trans_count,tmpstr[0],tmpstr[1],&receive_count,
						tmpstr[3],tmpstr[4],tmpstr[5],tmpstr[6],tmpstr[7]);
				if(trans_count==receive_count && receive_count>0){
					SdPrint("\n结论: RJ45  OK");
					lcdprt_result("RJ45",  OK);
				}else{
					SdPrint("\n结论: RJ45  ERROR");
					lcdprt_result("RJ45",  ERROR);
				}
			}
		}
	}
	PrtTestName("RJ45 测试结束");
}
extern int PressKey;
void YX_test(){
	PrtTestName("遥信 测试开始");
//	time_t curtime, oldtime;
	INT8U str[100];
	INT8U yxstat[5][2];
	memset(yxstat, 0, 10);

	while(1){
		memset(str, 0, 100);
//		sprintf((char*)str, "开关量1: %s    %s   %s",
//				JProgramInfo->currYXSTAT&0x01?"合":"分",
//				JProgramInfo->YxChange&0x01?"是":"否",
//				yx1_attrib?"动合":"动断");
		lcd_disp((char*)str, Gui_Point.x, Gui_Point.y+ROWSIZE);
//		SdPrint("\n%s",str);
		memset(str, 0, 100);
//		sprintf((char*)str, "开关量2: %s    %s   %s",
//				JProgramInfo->currYXSTAT&0x02?"合":"分",
//				JProgramInfo->YxChange&0x02?"是":"否",
//				yx2_attrib?"动合":"动断");
		lcd_disp((char*)str, Gui_Point.x, Gui_Point.y+2*ROWSIZE);
//		SdPrint("\n%s",str);
		memset(str, 0, 100);
//		sprintf((char*)str, "开关量3: %s    %s   %s",
//				JProgramInfo->currYXSTAT&0x04?"合":"分",
//				JProgramInfo->YxChange&0x04?"是":"否",
//				yx3_attrib?"动断":"动合");
		lcd_disp((char*)str, Gui_Point.x, Gui_Point.y+3*ROWSIZE);
		lcd_disp((char*)"     按ESC退出检测", Gui_Point.x, Gui_Point.y+4*ROWSIZE+6);
		ClearWaitTimes_test(ProjectNo,JProgramInfo);
		if(PressKey==ESC1){
			Gui_Point.y += 2*ROWSIZE+2;
			//JProgramInfo->Projects[ProjectNo].WaitTimes =ProjectWaitMaxCount+1;
			closelight();
			break;
		}
//		if(abs(curtime-oldtime)>=timeout)
//			break;
		delay(100);
	}
	PrtTestName("遥信 测试结束");
}
