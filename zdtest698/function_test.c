#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include "function_test.h"
#include "md5.h"
#include "lcd_test.h"
#include "gpio.h"

static CLASS_f203 g_oif203 = { };
INT8U g_save_changed = 0;
ConfigPara	g_cfg_para = {};
int PressKey;

//去除字符串尾的空格
char* rtrim(char* str)
{
	int n = strlen(str) - 1;
	if (n < 1)
		return str;
	while (n > 0) {
		if (*(str + n) == ' ') {
			*(str + n) = 0;
			n--;
		} else
			break;
	}
	return str;
}

char *revstr(char* str)
{
	char tmp = 0;
	int i;
	int n = strlen(str) - 1;
	if (n < 1)
		return str;
	for (i = 0; i < n / 2 + 1; i++) {
		tmp = *(str + i);
		*(str + i) = *(str + n - i);
		*(str + n - i) = tmp;
	}
	return str;
}
//去除字符串头尾的空格
char *rltrim(char* str)
{
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
	int strsize = 0, succ = 0, row = 0;
	fp = fopen(filename, "r");
	if (fp == NULL)
		return 0;
	while (!feof(fp)) {
		row++;
		if (row > 500) //为了防止程序在while里死循环
			break;
		memset(stmp, 0, 500);
		if (fgets(stmp, 500, fp) != NULL) //读取一行
		{
			if (memcmp(stmp, "//测试结束", strlen("//测试结束")) == 0) //到了文件尾
				break;
			if (stmp[0] == '/' && stmp[1] == '/')
				continue;
			if (stmp[0] == 10 || stmp[0] == 13) //换行 回车
				continue;
			strsize = strlen(stmp); //计算字符串的长度，不包括(\0)
			if (stmp[strsize - 1] == 0x0A) { //如果最后一个字符是(\n)
				stmp[strsize - 1] = '\0'; //将最后一个字符改为(\0)
			}
			pstr = NULL;
			pstr = strtok(stmp, "="); //以"="为分割符分成两个字符串
			if (pstr == NULL) //如果该行不包含“=”
				continue;
			rltrim(pstr); //去除字符串首尾的空格
			if (strcmp(pstr, name) == 0) { //找到了该字符串
				pstr = strtok(NULL, "="); //取值
				if (pstr == NULL) { //如果值为空
					succ = 0; //设置返回标志为0
					break;
				}
				rltrim(pstr); //去除字符串头尾的空格
				memcpy(ret, pstr, strlen(pstr));
				succ = 1;
				break;
			}
		}
	}
	fclose(fp);
	return succ;
}

void lcdprt_result(char *buf, char ok)
{
	char str[100], tmp_str[100];
	memset(str, 0, 100);
	memcpy(str, buf, strlen(buf));
	int count = 0, i, str_len = 0;
	memset(tmp_str, 0, 100);
	u2g(str, strlen(str), tmp_str, 100);			//utf8一个汉字3个字节 gbk一个汉字2个字节
	str_len = strlen(tmp_str);
	count = 15 - str_len;
	if (count <= 0)
		count = 10;
	for (i = 0; i < count; i++)
		strcat(str, " ");
	if (ok == OK)
		strcat(str, "OK");
	else if (ok == ERROR)
		strcat(str, "ERROR");
	else if (ok == UNKNOWN)
		strcat(str, "UNKNOWN");

	Gui_Point.y += ROWSIZE;

	lcd_disp(str, Gui_Point.x, Gui_Point.y);
}

void Esam_test()
{
	char cmd[] = "/nand/bin/cj esam 1>/nand/esam.log 2>>/nand/esam.log";
	char s[8192] = { 0 };
	FILE* fp = NULL;

	DEBUG_TO_FILE(1, TEST_LOG_FILE, "ESAM 测试开始");
	system(cmd);
	if ((fp = fopen("/nand/esam.log", "r")) == NULL) {
		lcdprt_result("ESAM", ERROR);
		DEBUG_TO_FILE(1, TEST_LOG_FILE, "结论: ESAM  ERROR");
	} else {
		if (fread(s, sizeof(char), sizeof(s), fp) == 0) {
			lcdprt_result("ESAM", ERROR);
			DEBUG_TO_FILE(1, TEST_LOG_FILE, "结论: ESAM  ERROR");
		} else {
			DEBUG_TO_FILE(1, TEST_LOG_FILE, "%s", s);
			if (strstr(s, "OK") == NULL) {
				lcdprt_result("ESAM", ERROR);
				DEBUG_TO_FILE(1, TEST_LOG_FILE, "结论: ESAM  ERROR");
			} else {
				lcdprt_result("ESAM", OK);
				DEBUG_TO_FILE(1, TEST_LOG_FILE, "结论: ESAM  OK");
			}
		}
	}
	fclose(fp);
	DEBUG_TO_FILE(1, TEST_LOG_FILE, "ESAM 测试结束");
	return;
}

void USB_test()
{
	DEBUG_TO_FILE(1, TEST_LOG_FILE, "USB 测试开始");
	lcdprt_result("USB", OK);
	DEBUG_TO_FILE(1, TEST_LOG_FILE, "结论: USB  OK");
	DEBUG_TO_FILE(1, TEST_LOG_FILE, "USB 测试结束");
	return;
}

int SendStrTo485(unsigned char *str, unsigned short Len, int Port)
{
	ssize_t re = 0;
	DEBUG_TO_FILE(1, TEST_LOG_FILE, "len: %d", Len);
	re = write(Port, str, Len);
	return re;
}

int ASCToBCD(unsigned char * ASC, int Len, unsigned char *Ret)
{
	int i, j, k;
	if (ASC != NULL) {
		for (i = 0; i < Len; i++) {
			if (ASC[i] <= '9' && ASC[i] >= '0')
				ASC[i] = ASC[i] - '0';
			else if (ASC[i] <= 'f' && ASC[i] >= 'a') {
				ASC[i] = ASC[i] - 'W';
			} else if (ASC[i] <= 'F' && ASC[i] >= 'A') {
				ASC[i] = ASC[i] - '7';
			}
		}
		if (Len % 2 == 0) {
			memset(Ret, 0, Len / 2 + 1);
			for (j = 0, k = 0; j < Len / 2; j++) {
				*(Ret + j) = (ASC[k] << 4) | ASC[k + 1];
				k++;
				k++;
			}
		}
	}
	return 0;
}


int byteNCmp(char* s, char* t, int n)
{
	int i=0;

	if(NULL == s || NULL == t)
		return ERROR;

	for(i=0;i<n;i++,s++,t++) {
		if(s[i] != t[i])
			return ERROR;
	}

	return OK;
}

int readFromCom(int port, int timeout)
{
	unsigned char sendbuf[] = { 0x68, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68,
			0x11, 0x04, 0x33, 0x32, 0x34, 0x33, 0xb2, 0x16 };

	int portNo4851 = 0;
	int portNo4852 = 0;

	int fd4851 = 0, fd4852 = 0;
	int sendFd = 0, readFd = 0;

	int j = 0, len = 0, recv_count = 0;
	char s_port[10] = { 0 };
	unsigned char recvbuf[512] = { 0 };

	if (readcfg(JZQTEST_PARA_NAME, "485I_port", s_port) == 1)
		portNo4851 = atoi(s_port);

	if (readcfg(JZQTEST_PARA_NAME, "485II_port", s_port) == 1)
		portNo4852 = atoi(s_port);

	fd4851 = OpenCom(portNo4851, 2400, (unsigned char *) "even", 1, 8);
	fd4852 = OpenCom(portNo4852, 2400, (unsigned char *) "even", 1, 8);

	if (fd4851 <= 0) {
		DEBUG_TO_FILE(1, TEST_LOG_FILE, "打开4851串口失败！！！");
		return 0;
	}

	if (fd4852 <= 0) {
		DEBUG_TO_FILE(1, TEST_LOG_FILE, "打开4852串口失败！！！");
		return 0;
	}

	DEBUG_TO_FILE(1, TEST_LOG_FILE, "send=%d port=%d", sizeof(sendbuf), port);
	for (j = 0; j < 16; j++) {
		DEBUG_TO_FILE(0, TEST_LOG_FILE, " %02x", sendbuf[j]);
	}
	DEBUG_TO_FILE(0, TEST_LOG_FILE, "\n");

	if (port == S4851) {
		sendFd = fd4851;
		readFd = fd4852;
	} else if (port == S4852) {
		sendFd = fd4852;
		readFd = fd4851;
	}

	SendStrTo485(sendbuf, sizeof(sendbuf), sendFd);
	while (1) {	//todo:  485-1发送的数据, 485-2读到后在发送回485-1, 才认为485-1的读写通道是好的, 否则出问题了.   485-2同理
		recv_count++;
		if (recv_count > timeout)
			break;
		len = 0;
		memset(recvbuf, 0, 512);
		len = ReceiveFrom485_test(recvbuf, readFd);
		DEBUG_TO_FILE(1, TEST_LOG_FILE, "len: %d", len);
		if (len == sizeof(sendbuf)) {
			DEBUG_TO_FILE(1, TEST_LOG_FILE, "\nReceiveFrom485  recv=%d", len);
			for (j = 0; j < len; j++) {
				DEBUG_TO_FILE(1, TEST_LOG_FILE, " %02x", recvbuf[j]);
			}
			DEBUG_TO_FILE(1, TEST_LOG_FILE, "\n");

			if(byteNCmp(sendbuf, recvbuf, len) != OK) {
				close(fd4851);
				close(fd4852);
				return ERROR;
			}
			break;
		}
		delay(200);
	}

	SendStrTo485(recvbuf, len, readFd);
	while (1) {	//读到的数据再发回去
		recv_count++;
		if (recv_count > timeout)
			break;
		len = 0;
		memset(recvbuf, 0, sizeof(recvbuf));
		len = ReceiveFrom485_test(recvbuf, sendFd);
		DEBUG_TO_FILE(1, TEST_LOG_FILE, "len: %d", len);
		if (len == sizeof(sendbuf)) {
			DEBUG_TO_FILE(1, TEST_LOG_FILE, "\nReceiveFrom485  recv=%d", len);
			for (j = 0; j < len; j++) {
				DEBUG_TO_FILE(1, TEST_LOG_FILE, " %02x", recvbuf[j]);
			}
			DEBUG_TO_FILE(1, TEST_LOG_FILE, "\n");

			if(byteNCmp(sendbuf, recvbuf, len) != OK) {
				close(fd4851);
				close(fd4852);
				return ERROR;
			}
			break;
		}
		delay(200);
	}

	close(fd4851);
	close(fd4852);
	return len;
}

int readmeter(int meter, int port, int timeout)
{
	char flg = 0;
	int ComPort = 0, i = 0, j = 0, load_len = 0;
	int len = 0, recv_count = 0;
	char s_port[10], Check = 0;
	unsigned char sendbuf[200] = { 0x68, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x68, 0x11, 0x04, 0x33, 0x32, 0x34, 0x33, 0xb2, 0x16 };
	unsigned char recvbuf[512];
	memset(s_port, 0, 10);
	char s_jcaddr[20];
	char bcd_jcaddr[20];
	memset(s_jcaddr, 0, 20);
	memset(bcd_jcaddr, 0, 20);

	if (port == S4851) {
		if (readcfg(JZQTEST_PARA_NAME, "485I_port", s_port) == 1)
			port = atoi(s_port);

		if (readcfg(JZQTEST_PARA_NAME, "485I_meter_addr", s_jcaddr) == 1)
			ASCToBCD((unsigned char*) s_jcaddr, 12,
					(unsigned char*) bcd_jcaddr);
	} else if (port == S4852) {
		if (readcfg(JZQTEST_PARA_NAME, "485II_port", s_port) == 1)
			port = atoi(s_port);

		if (readcfg(JZQTEST_PARA_NAME, "485II_meter_addr", s_jcaddr) == 1)
			ASCToBCD((unsigned char*) s_jcaddr, 12,
					(unsigned char*) bcd_jcaddr);
	}
	DEBUG_TO_FILE(1, TEST_LOG_FILE, "\n 表地址=%02x%02x%02x%02x%02x%02x",
			bcd_jcaddr[5], bcd_jcaddr[4], bcd_jcaddr[3], bcd_jcaddr[2],
			bcd_jcaddr[1], bcd_jcaddr[0]);
	memcpy(&sendbuf[1], bcd_jcaddr, 6);
	sendbuf[1] = bcd_jcaddr[5];
	sendbuf[2] = bcd_jcaddr[4];
	sendbuf[3] = bcd_jcaddr[3];
	sendbuf[4] = bcd_jcaddr[2];
	sendbuf[5] = bcd_jcaddr[1];
	sendbuf[6] = bcd_jcaddr[0];
	Check = 0x00;
	for (j = 0; j < 14; j++)
		Check = Check + sendbuf[j];
	DEBUG_TO_FILE(1, TEST_LOG_FILE, "\n sendbuf check=%02x", Check);
	sendbuf[14] = Check;

	ComPort = OpenCom(port, 2400, (unsigned char *) "even", 1, 8);
	if (ComPort <= 0) {
		DEBUG_TO_FILE(1, TEST_LOG_FILE, "打开串口失败！！！");
		return 0;
	}
	DEBUG_TO_FILE(1, TEST_LOG_FILE, "\n send=%d port=%d", 16, port);
	for (j = 0; j < 16; j++) {
		DEBUG_TO_FILE(0, TEST_LOG_FILE, " %02x", sendbuf[j]);
	}
	DEBUG_TO_FILE(0, TEST_LOG_FILE, "\n");
	SendStrTo485(sendbuf, 16, ComPort);
	while (1) {
		recv_count++;
		if (recv_count > timeout)
			break;
		memset(recvbuf, 0, 512);
		len = ReceiveFrom485_test(recvbuf, ComPort);
		if (len > 8) {
			DEBUG_TO_FILE(1, TEST_LOG_FILE, "\nReceiveFrom485_test recv=%d", len);
			for (j = 0; j < len; j++) {
				DEBUG_TO_FILE(1, TEST_LOG_FILE, " %02x", recvbuf[j]);
			}
			DEBUG_TO_FILE(1, TEST_LOG_FILE, "\n");
			flg = 1;
			break;
		}
	}

	if (flg == 1) {
		for (i = 0; i < len; i++) {
			if (recvbuf[i] == 0x68)
				break;
		}
		load_len = recvbuf[i + 9];
		Check = 0x00;
		for (j = 0; j < (load_len + 10); j++) {
			Check = Check + recvbuf[i + j];
		}
		if (Check != recvbuf[i + load_len + 10]) {
			DEBUG_TO_FILE(1, TEST_LOG_FILE, "\n check=%d recvbuf_check=%d", Check,
					recvbuf[i + load_len + 10]);
			close(ComPort);
			return ERROR;
		}
	}

	if (ComPort > 0)
		close(ComPort);

	return OK;
}

//测试完485后删除所有有效的测量点 质检要求
//删除所有测量点 交采除外
void delallmeter()
{
	int i = 1;
	char jcMeterNo[5] = { 0 };			//交采测量点号
	int jcMeterNoInt = 0;

	if (readcfg(JZQTEST_PARA_NAME, "jcMeterNo", jcMeterNo) == 1)
		jcMeterNoInt = atoi(jcMeterNo);

//	for (i = 1; i < 255; i++) {			//todo: 用cj 命令速度太慢, 需要重写
//		if (i != jcMeterNoInt) {
//			sprintf(cmd, "cj coll delete 6000  %d", i);
//			system(cmd);
//		}
//	}

//	if(readParaClass(0x6000,&meter,seqno)==1) {
//		if(seqno == meter.sernum) {
//			meter.sernum = 0;
//			ret = saveParaClass(0x6000,&meter,seqno);
//	        setOIChange_CJ(0x6000);
//			if(ret==1)
//				fprintf(stderr,"删除 序号 %d 成功, ret=%d\n",seqno,ret);
//		}
//	}
}

void RS485I_test()
{
	int ret = 0, cldno = 2;
	char s_cldno[5] = { 0 };
	char readMeterChar[3] = { 0 };
	int readMeterInt = 0;

	DEBUG_TO_FILE(1, TEST_LOG_FILE, "485I 测试开始");
	delay(1000);
	if (readcfg(JZQTEST_PARA_NAME, "485I_cldno", s_cldno) == 1)
		cldno = atoi(s_cldno);

	if (readcfg("RS485_read_meter", "485I_cldno", readMeterChar) == 1)
		readMeterInt = atoi(readMeterChar);

	if (readMeterInt == 0)
		ret = readFromCom( S4851, 3);
	else if (readMeterInt == 1)
		ret = readmeter(cldno, S4851, 3);

	if (ret == OK) {
		DEBUG_TO_FILE(1, TEST_LOG_FILE, "\n结论: RS485I   OK");
		lcdprt_result("RS485I", OK);
	} else {
		DEBUG_TO_FILE(1, TEST_LOG_FILE, "\n结论: RS485I   ERROR");
		lcdprt_result("RS485I", ERROR);
	}

	delallmeter();
	DEBUG_TO_FILE(1, TEST_LOG_FILE, "485I 测试结束");
}

void RS485II_test()
{
	int ret = 0, cldno = 3;
	char s_cldno[5] = { 0 };
	char readMeterChar[3] = { 0 };
	int readMeterInt = 0;

	DEBUG_TO_FILE(1, TEST_LOG_FILE, "485I 测试开始");
	delay(1000);
	if (readcfg(JZQTEST_PARA_NAME, "485II_cldno", s_cldno) == 1)
		cldno = atoi(s_cldno);

	if (readcfg("RS485_read_meter", "485I_cldno", readMeterChar) == 1)
		readMeterInt = atoi(readMeterChar);

	if (readMeterInt == 0)
		ret = readFromCom( S4852, 3);
	else if (readMeterInt == 1)
		ret = readmeter(cldno, S4852, 3);

	if (ret == OK) {
		DEBUG_TO_FILE(1, TEST_LOG_FILE, "\n结论: RS485II   OK");
		lcdprt_result("RS485II", OK);
	} else {
		DEBUG_TO_FILE(1, TEST_LOG_FILE, "\n结论: RS485II   ERROR");
		lcdprt_result("RS485II", ERROR);
	}
	DEBUG_TO_FILE(1, TEST_LOG_FILE, "485II 测试结束");
	//测试完485后删除所有有效的测量点 质检要求
	delallmeter();
}

void trimStr(char* s)
{
	int strsize = 0;

	if (NULL == s)
		return;

	rltrim(s);
	strsize = strlen(s);
	if (strsize > 2 || strsize == 2)
		if (s[strsize - 2] == '\r')
			s[strsize - 2] = '\0';

	if (s[strsize - 1] == '\n') {
		s[strsize - 1] = '\0';
	}
}

char* getLastStr(char* s, char delim)
{
	char* p = NULL;

	if (s == NULL)
		return NULL;

	p = s + strlen(s);
	while (*p != delim && p != s)
		p--;

	if (s == p)
		return s;
	else
		return (p + 1);
}

void softver_test()
{			//todo: 把软件校验结果写到log里; 从配置文件读取哪个功能需要测
	int i = 0;
	int rowCnt = 0;
	char softflag = 1;
	char md5Res[50] = { 0 };
	char kernelFlag = 1;
	char kernelver_std[100] = { 0 }, kernelver[100] = { 0 };

	char cmd[100] = { 0 };
	char fileToChk[200] = { 0 };
	char stmp[500] = { 0 };
	char* pstr = NULL;
	int fileCnt = 0;			//要检查md5码的文件数量
	FILE *fp = NULL;

	//---------------------------------
	DEBUG_TO_FILE(1, TEST_LOG_FILE, "软件校验开始");
	//文件MD5校验码
	memset(stmp, 0, 30);
	//读取要校验的文件数量
	if (readcfg(JZQTEST_PARA_NAME, "filecount", stmp) == 1) {
		fileCnt = atoi(stmp);
		DEBUG_TO_FILE(1, TEST_LOG_FILE, "fileCnt: %d", fileCnt);
	}

	if ((fp = fopen(JZQTEST_PARA_NAME, "r")) == NULL) {
		lcdprt_result("md5 config", UNKNOWN);
		softflag = 0;
	} else {
		fseek(fp, 0, SEEK_SET);
		rowCnt = 0;
		while (!feof(fp)) {				//定位filecount标签
			if (rowCnt == 500)
				break;

			if (fgets(stmp, 500, fp) != NULL) {
				trimStr(stmp);
				pstr = NULL;
				pstr = strtok(stmp, "=");
				if (pstr == NULL)
					continue;

				trimStr(pstr);
				if (strcmp(pstr, "filecount") == 0) {				//找到了该检测项
					break;
				}
			}
			rowCnt++;
		}

		for (i = 0; i < fileCnt; i++) {
			bzero(md5Res, sizeof(md5Res));
			bzero(fileToChk, sizeof(fileToChk));

			if (fgets(stmp, sizeof(stmp), fp) != NULL) {
				DEBUG_TO_FILE(1, TEST_LOG_FILE, "%s", stmp);
				pstr = strtok(stmp, "=");   //获取文件名
				rltrim(pstr);
				strcpy(fileToChk, pstr);

				MD5_File(fileToChk, md5Res);   //计算文件的校验码
				trimStr(md5Res);
				DEBUG_TO_FILE(1, TEST_LOG_FILE, "%s md5Res: %s",
						getLastStr(fileToChk, '/'), md5Res);

				pstr = strtok(NULL, "=");   //获取正确的校验码
				trimStr(pstr);
				DEBUG_TO_FILE(1, TEST_LOG_FILE, "md5STD: %s", pstr);

				if (strcmp(pstr, md5Res) == 0) {
					sprintf(stmp, "%s 正确\n", getLastStr(fileToChk, '/'));
					DEBUG_TO_FILE(1, TEST_LOG_FILE, "%s", stmp);
				} else {
					sprintf(stmp, "%s 错误\n", getLastStr(fileToChk, '/'));
					DEBUG_TO_FILE(1, TEST_LOG_FILE, "%s", stmp);
					softflag = 0;
				}
			}
		}
	}
	fclose(fp);
	DEBUG_TO_FILE(1, TEST_LOG_FILE, "软件版本校验结束");

	//内核
	DEBUG_TO_FILE(1, TEST_LOG_FILE, "内核校验开始");
	bzero(cmd, sizeof(cmd));
	bzero(kernelver_std, sizeof(kernelver_std));
	bzero(kernelver, sizeof(kernelver));
	if (readcfg(JZQTEST_PARA_NAME, "Kernel_Ver", kernelver_std) == 1) {
		DEBUG_TO_FILE(1, TEST_LOG_FILE, "kernelver_std: %s, length: %d",
				kernelver_std, strlen(kernelver_std));
		sprintf(cmd, "cat /proc/version | awk '{print $16,$17,$18}'");
		fp = popen(cmd, "r");
		if (NULL == fp) {
			kernelFlag = 0;
		} else {
			if (fread(kernelver, sizeof(char), sizeof(kernelver), fp) == 0) {
				DEBUG_TO_FILE(1, TEST_LOG_FILE, "内核校验错误");
				kernelFlag = 0;
			} else {
				trimStr(kernelver);
				DEBUG_TO_FILE(1, TEST_LOG_FILE, "kernelver: %s, length: %d",
						kernelver, strlen(kernelver));
				if (strcmp(kernelver_std, kernelver) == 0) {
					DEBUG_TO_FILE(1, TEST_LOG_FILE, "内核校验正确");
				} else {
					DEBUG_TO_FILE(1, TEST_LOG_FILE, "内核校验错误");
					kernelFlag = 0;
				}
			}
		}
		pclose(fp);
	} else {
		DEBUG_TO_FILE(1, TEST_LOG_FILE, "Kernel_Ver UNKNOWN");
		kernelFlag = 0;
	}
	sprintf(stmp, "软件%s.内核%s", softflag == 1 ? "OK" : "ERROR",
			kernelFlag == 1 ? "OK" : "ERROR");
	lcdprt_result(stmp, -1);

	DEBUG_TO_FILE(1, TEST_LOG_FILE, "内核校验结束");
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
void RJ45_test()
{
	int i = 0;
	int rowcount = 0;
	char stmp[500];
	int trans_count = 0, receive_count = 0;
	char tmpstr[10][50];

	char remote_ip[50] = { "192.168.0.224" };
	char local_ip[50] = { "192.168.0.10" };
	char cmd[100];

	//ping 193.168.18.173 -q -c 3 -W 2 > ping.log
	DEBUG_TO_FILE(1, TEST_LOG_FILE, "RJ45 测试开始");
	if (readcfg(JZQTEST_PARA_NAME, "local_ip", local_ip) == 1) {
		DEBUG_TO_FILE(1, TEST_LOG_FILE, "local_ip: %s", local_ip);
		bzero(cmd, sizeof(cmd));
		sprintf(cmd, "ifconfig eth0 %s up", local_ip);
		for (i = 0; i < 3; i++) {
			system(cmd);
			delay(200);
		}
	}

	bzero(cmd, sizeof(cmd));
	if (readcfg(JZQTEST_PARA_NAME, "remote_ip", remote_ip) == 1) {
		DEBUG_TO_FILE(1, TEST_LOG_FILE, "remote_ip: %s", remote_ip);
		sprintf(cmd, "ping %s -q -c 5 -W 2 > /nand/ping.log", remote_ip);
		for (i = 0; i < 3; i++) {
			system(cmd);
			delay(200);
		}
	}

	FILE *fp = NULL;
	fp = fopen("/nand/ping.log", "r");
	if (fp == NULL) {
		fprintf(stderr, "\n no ping.log");
		lcdprt_result("RJ45", ERROR);
		return;
	}
	while (!feof(fp)) {
		rowcount++;
		if (rowcount > 300)
			break;
		memset(stmp, 0, 500);
		if (fgets(stmp, 500, fp) != NULL)   //读取一行
		{
			if (memcmp(stmp, "//测试结束", strlen("//测试结束")) == 0)   //到了文件尾
				break;
			if (stmp[0] == '/' && stmp[1] == '/')
				continue;
			if (stmp[0] == 10 || stmp[0] == 13)   //换行 回车
				continue;
			if (strstr(stmp, "packet loss") != NULL) {
				//3 packets transmitted, 3 packets received, 0% packet loss
				sscanf(stmp, "%d %s %s %d %s %s %s %s %s", &trans_count,
						tmpstr[0], tmpstr[1], &receive_count, tmpstr[3],
						tmpstr[4], tmpstr[5], tmpstr[6], tmpstr[7]);
				if (trans_count == receive_count && receive_count > 0) {
					DEBUG_TO_FILE(1, TEST_LOG_FILE, "\n结论: RJ45  OK");
					lcdprt_result("RJ45", OK);
				} else {
					DEBUG_TO_FILE(1, TEST_LOG_FILE, "\n结论: RJ45  ERROR");
					lcdprt_result("RJ45", ERROR);
				}
			}
		}
	}
	DEBUG_TO_FILE(1, TEST_LOG_FILE, "RJ45 测试结束");
}

void battery_test()
{
	char btThd[20] = { 0 };
	float btThdF = 0.0;
	float v1 = 0.0, v2 = 0.0;

	DEBUG_TO_FILE(1, TEST_LOG_FILE, "时钟电压 测试开始");
	bettery_getV(&v1, &v2);
	DEBUG_TO_FILE(1, TEST_LOG_FILE, "v1: %f, v2: %f", v1, v2);
	if (readcfg(JZQTEST_PARA_NAME, "battery", btThd) == 1) {
		btThdF = strtof(btThd, NULL);
		DEBUG_TO_FILE(1, TEST_LOG_FILE, "btThdF: %f", btThdF);
		if (v1 <= btThdF) {
			lcdprt_result((char*) "时钟电压", ERROR);
		} else {
			lcdprt_result((char*) "时钟电压", OK);
		}
	}
}

char state_get(DEV_STATE_PULSE road)
{
	char staval = -1;
	unsigned int pluse_tmp[2] = { };
	int fd = 0;

	switch (road) {
	case STATE1:
		staval = gpio_readbyte(DEV_STATE1);
		break;
	case STATE2:
		staval = gpio_readbyte(DEV_STATE2);
		break;
	case STATE3:
		staval = gpio_readbyte(DEV_STATE3);
		break;
	case STATE4:
		staval = gpio_readbyte(DEV_STATE4);
		break;
	case PLUSE1:
		if ((fd = open(DEV_PULSE, O_RDWR | O_NDELAY)) >= 0) {
			read(fd, &pluse_tmp, 2 * sizeof(unsigned int));
			close(fd);
			return pluse_tmp[0];
		} else {
			syslog(LOG_ERR, "%s %s fd=%d(PLUSE1)\n", __func__, DEV_PULSE, fd);
			return -1;
		}
		break;
	case PLUSE2:
		if ((fd = open(DEV_PULSE, O_RDWR | O_NDELAY)) >= 0) {
			read(fd, &pluse_tmp, 2 * sizeof(unsigned int));
			close(fd);
			return pluse_tmp[1];
		} else {
			syslog(LOG_ERR, "%s %s fd=%d(PLUSE2)\n", __func__, DEV_PULSE, fd);
			return -1;
		}
		break;
	}
	return staval;
}

void state_check(BOOLEAN changed)
{
	INT8U state_num = 1;
	INT8U i = 0;
	INT8U bit_state[STATE_MAXNUM] = { };
	INT8S readstate[STATE_MAXNUM] = { };	//读取开关量状态

	memset(bit_state, 0, sizeof(bit_state));
	if (g_cfg_para.device == CCTT2) {
		state_num = 1;		//防止读取其他无用的yx设备
	} else
		state_num = STATE_MAXNUM;

	for (i = 0; i < state_num; i++) {
		if (((g_oif203.state4.StateAcessFlag >> (STATE_MAXNUM - 1 - i)) & 0x01)
				== 1) {
			if (i >= 0 && i <= 3) {		//YX1-YX4
				readstate[i] = state_get(i + 1);
				if (readstate[i] != -1) {
					bit_state[i] = (~(readstate[i])) & 0x01;
				}
			} else if (i == 4) {		//门节点
				if (g_cfg_para.device == CCTT1) {		//I型集中器
					readstate[i] = getSpiAnalogState();
					if (readstate[i] != -1) {
						bit_state[i] = ((~(readstate[i] >> 5)) & 0x01);
					}
				}
			}
			if (((g_oif203.state4.StatePropFlag >> (STATE_MAXNUM - 1 - i)) & 0x01)
					== 0) {	//常闭
				bit_state[i] = (~bit_state[i]) & 0x01;
			}
		}
	}

	for (i = 0; i < STATE_MAXNUM; i++) {
		if ((changed == FALSE)
				&& (bit_state[i] != g_oif203.statearri.stateunit[i].ST)) {

			g_oif203.statearri.stateunit[i].ST = bit_state[i];
			g_oif203.statearri.stateunit[i].CD = 1;
		}
	}


	fprintf(stderr, "RD=%d_%d_%d_%d %d_%d_%d_%d\n", bit_state[0],
			bit_state[1], bit_state[2], bit_state[3], bit_state[4],
			bit_state[5], bit_state[6], bit_state[7]);
	fprintf(stderr, "ST=%d_%d_%d_%d %d_%d_%d_%d\n",
			g_oif203.statearri.stateunit[0].ST,
			g_oif203.statearri.stateunit[1].ST,
			g_oif203.statearri.stateunit[2].ST,
			g_oif203.statearri.stateunit[3].ST,
			g_oif203.statearri.stateunit[4].ST,
			g_oif203.statearri.stateunit[5].ST,
			g_oif203.statearri.stateunit[6].ST,
			g_oif203.statearri.stateunit[7].ST);
	fprintf(stderr, "CD=%d_%d_%d_%d %d_%d_%d_%d\n\n",
			g_oif203.statearri.stateunit[0].CD,
			g_oif203.statearri.stateunit[1].CD,
			g_oif203.statearri.stateunit[2].CD,
			g_oif203.statearri.stateunit[3].CD,
			g_oif203.statearri.stateunit[4].CD,
			g_oif203.statearri.stateunit[5].CD,
			g_oif203.statearri.stateunit[6].CD,
			g_oif203.statearri.stateunit[7].CD);
}

void dogFeed()
{
    INT32S fd = -1;
    INT32S tm = 3600;

    if ((fd = open(DEV_WATCHDOG, O_RDWR | O_NDELAY)) == -1) {
        fprintf(stderr, "\n\r open /dev/watchdog error!!!");
        return;
    }
    write(fd, &tm, sizeof(int));
    close(fd);
}

void YX_test()
{
	char str[100];
	int originY = Gui_Point.y;
	int originX = Gui_Point.x;

	DEBUG_TO_FILE(1, TEST_LOG_FILE, "遥信 测试开始");

	g_oif203.state4.StateAcessFlag = 0xff;
	g_oif203.state4.StatePropFlag = 0xff;
	memset(&g_oif203.statearri.stateunit,0,sizeof(g_oif203.statearri.stateunit));

	while (1) {
		dogFeed();

		if (PressKey == KEY_ESC) {
			break;
		}
		state_check(FALSE);

		memset(str, 0, 100);
		Gui_Point.x = FONTSIZE;
		Gui_Point.y = originY + FONTSIZE;
		lcd_disp((char*) " 遥信", Gui_Point.x, Gui_Point.y);

		Gui_Point.y += FONTSIZE;
		memset(str, 0, 100);
		sprintf((char*) str, "1: %s",
				g_oif203.statearri.stateunit[0].ST ? "合" : "开");
		lcd_disp(str, Gui_Point.x, Gui_Point.y);

		Gui_Point.x += FONTSIZE * 3;
		memset(str, 0, 100);
		sprintf((char*) str, "2: %s",
				g_oif203.statearri.stateunit[1].ST ? "合" : "开");
		lcd_disp(str, Gui_Point.x, Gui_Point.y);

		Gui_Point.x =  FONTSIZE;
		Gui_Point.y += FONTSIZE;
		memset(str, 0, 100);
		sprintf((char*) str, "3: %s",
				g_oif203.statearri.stateunit[2].ST ? "合" : "开");
		lcd_disp(str, Gui_Point.x, Gui_Point.y);

		Gui_Point.x += FONTSIZE * 3;
		memset(str, 0, 100);
		sprintf((char*) str, "4: %s",
				g_oif203.statearri.stateunit[3].ST ? "合" : "开");
		lcd_disp(str, Gui_Point.x, Gui_Point.y);

		Gui_Point.x = FONTSIZE;
		Gui_Point.y += FONTSIZE;
		memset(str, 0, 100);
		sprintf((char*) str, "门节点: %s",
				g_oif203.statearri.stateunit[4].ST ? "合" : "开");
		lcd_disp(str, Gui_Point.x, Gui_Point.y);


		memset(str, 0, 100);
		Gui_Point.x = FONTSIZE*7;
		Gui_Point.y = originY + FONTSIZE;
		lcd_disp((char*) " 变位", Gui_Point.x, Gui_Point.y);

		Gui_Point.y += FONTSIZE;
		memset(str, 0, 100);
		sprintf((char*) str, "1: %s",
				g_oif203.statearri.stateunit[0].CD ? "是" : "否");
		lcd_disp(str, Gui_Point.x, Gui_Point.y);

		memset(str, 0, 100);
		Gui_Point.x += FONTSIZE*3;
		sprintf((char*) str, "2: %s",
				g_oif203.statearri.stateunit[1].CD ? "是" : "否");
		lcd_disp(str, Gui_Point.x, Gui_Point.y);

		memset(str, 0, 100);
		Gui_Point.x = FONTSIZE*7;
		Gui_Point.y += FONTSIZE;
		sprintf((char*) str, "3: %s",
				g_oif203.statearri.stateunit[2].CD ? "是" : "否");
		lcd_disp(str, Gui_Point.x, Gui_Point.y);

		memset(str, 0, 100);
		Gui_Point.x += FONTSIZE*3;
		sprintf((char*) str, "4: %s",
				g_oif203.statearri.stateunit[3].CD ? "是" : "否");
		lcd_disp(str, Gui_Point.x, Gui_Point.y);

		memset(str, 0, 100);
		Gui_Point.x = FONTSIZE*7;
		Gui_Point.y += FONTSIZE;
		sprintf((char*) str, "门节点: %s",
				g_oif203.statearri.stateunit[4].CD ? "是" : "否");
		lcd_disp(str, Gui_Point.x, Gui_Point.y);

		PressKey = NOKEY;
		delay(200);
	}
	DEBUG_TO_FILE(1, TEST_LOG_FILE, "遥信 测试结束");
}
