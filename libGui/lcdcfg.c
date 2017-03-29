/*
 * lcdcfg.c  液晶配置文件
 *
 *  Created on: 2013-7-9
 *      Author: yd
 */

#include "comm.h"
#include "lcm.h"
#include "gui.h"
int Contrast_alue;
int FB_Handle;
int lcm_open(){
	FB_Handle=-1;
	FB_Handle = open("/dev/fb0", O_RDWR | O_NDELAY);
	return FB_Handle;
}
void lcm_close(){
	if(FB_Handle>=0)
		close(FB_Handle);
}
void lcm_write()
{
	if(FB_Handle>=0)
		write(FB_Handle,LcdBuf,160*160);
}
//void lcm_write()
//{
//	int fd=-1;
//	if((fd = open("/dev/fb0", O_RDWR | O_NDELAY)) > 0)
//	{
//		write(fd,LcdBuf,160*160);
//		close(fd);
//	}
//}

/*文件格式
//液晶亮度
LCD_BRIGHTNESS=1789
//液晶密码
LCD_PASSWORD=000000
*/
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
	char stmp[100];
	char *pstr;
	int strsize=0, succ=0, row=0;
	fp=fopen(filename, "r");
	if(fp == NULL)
		return 0;
	//dbg_prt("\n name=%s", name);
	while(!feof(fp))
	{
		row++;
		if(row>500)//为了防止程序在while里死循环
			break;
		memset(stmp, 0, 100);
		if(fgets(stmp, 100, fp) != NULL)//读取一行
		{
			if(memcmp(stmp, "//end", strlen("//end"))==0)//到了文件尾
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
//往文件里写入一行 如果没有的话则添加一行
int writecfg(char *filename, char *name, char *value)
{
	FILE *fp;
	char stmp[100],sline[100];
	char *pstr;
	int strsize=0, position=0, succ=0, row=0;
	fp=fopen(filename, "r+");
	if(fp == NULL){
		fp=fopen(filename, "w");
		if(fp==NULL){
			return succ;
		}
	}
//	dbg_prt("\n lcd.cfg :");
	while(!feof(fp))
	{
		row++;
		if(row>30)//为了防止程序在while里死循环
			break;
		memset(stmp, 0, 100);
		position = ftell(fp);
		if(fgets(stmp, 200, fp) != NULL)
		{
//			dbg_prt("\n line %d:%s", row, stmp);
			if(memcmp(stmp, "//end", strlen("//end"))==0)
				break;
			if(stmp[0]=='/' && stmp[1]=='/')
				continue;
			strsize=strlen(stmp);
			if(stmp[strsize-1] == 0x0A){
				stmp[strsize-1]='\0';
				if(strsize==1)
					continue;
			}
			pstr = strtok(stmp,"=");
			rltrim(pstr);
			if(strcmp(pstr, name) == 0){
				memset(sline,0,100);
				sprintf(sline, "%s=%s", pstr, value);
				fseek(fp, position, SEEK_SET);
				fputs(sline,fp);
				succ = 1;
				break;
			}
		}
	}
	if(succ==0){
		memset(sline,0,100);
		sprintf(sline, "\n%s=%s",name, value);
		fseek(fp, 0, SEEK_END);
		if(fputs(sline,fp)>=0)
			succ = 2;
		else
			succ = 0;
	}
	fclose(fp);
	return succ;
}

//LCD_IOC_AC_POWER
void lcm_ac_power()
{
	ioctl(FB_Handle,LCD_IOC_AC_POWER,&Contrast_alue);
}

void initliangdu()
{
//	int fdyj=-1;
	char ret[20];
	memset(ret, 0, 20);
	Contrast_alue = 0;
	if(readcfg((char*)"/nor/config/lcd.cfg", (char*)"LCD_BRIGHTNESS", ret)==1)
		Contrast_alue = atoi(ret);
	if(Contrast_alue>0){
		if( Contrast_alue<YJ_BOTTOMLIMIT1 || (Contrast_alue>YJ_TOPLIMIT1 && Contrast_alue<YJ_BOTTOMLIMIT2) )
			Contrast_alue=YJ_DEFAULTLIMIT1;
		if( Contrast_alue>YJ_TOPLIMIT2 )
			Contrast_alue=YJ_DEFAULTLIMIT2;
	}else
		Contrast_alue=YJ_DEFAULTLIMIT2;
//	if((fdyj = open("/dev/fb0", O_RDWR | O_NDELAY)) >0)
		ioctl(FB_Handle,LCD_IOC_CONTRAST,&Contrast_alue);
//	close(fdyj);
}

void modify_lcd_contrast(){
	int contrast_value_old=0;
	char first_flg=0, showstr[30], ret[10];
	Point pos;
	initliangdu();
	contrast_value_old = Contrast_alue;
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		switch(PressKey){
		case UP:
			Contrast_alue = Contrast_alue + 2;
			if( contrast_value_old<=YJ_TOPLIMIT1 && Contrast_alue>=YJ_TOPLIMIT1 )
				Contrast_alue=YJ_BOTTOMLIMIT2;
			if( contrast_value_old<=YJ_TOPLIMIT2 && Contrast_alue>=YJ_TOPLIMIT2 )
				Contrast_alue=YJ_BOTTOMLIMIT1;
//			dbg_prt("\nup Contrast_alue = %d\n", Contrast_alue);
			contrast_value_old = Contrast_alue;
			break;
		case DOWN:
			Contrast_alue = Contrast_alue - 2;
			if( contrast_value_old>=YJ_BOTTOMLIMIT2 && Contrast_alue<=YJ_BOTTOMLIMIT2 )
				Contrast_alue=YJ_TOPLIMIT1;
			if( contrast_value_old>=YJ_BOTTOMLIMIT1 && Contrast_alue<=YJ_BOTTOMLIMIT1 )
				Contrast_alue=YJ_TOPLIMIT2;
//			dbg_prt("\ndown Contrast_alue = %d\n", Contrast_alue);
			contrast_value_old = Contrast_alue;
			break;
		case ESC:
			return;
		}
		if(PressKey==UP || PressKey==DOWN || first_flg==0){
			first_flg = 1;
			gui_clrrect(rect_Client);
			sprintf(showstr,"液晶对比度: %d",Contrast_alue);
			gui_setpos(&pos, rect_Client.left+FONTSIZE*4, rect_Client.top+FONTSIZE*9);
			gui_textshow((char *)showstr, pos, LCD_NOREV);
			set_time_show_flag(1);//TODO:new
		//	if((fdyj = open("/dev/fb0", O_RDWR | O_NDELAY)) >0){
				ioctl(FB_Handle,LCD_IOC_CONTRAST,&Contrast_alue);
		//		close(fdyj);
		//	}
			memset(ret, 0, 10);
			sprintf(ret,"%d",Contrast_alue);
			writecfg((char*)"/nor/config/lcd.cfg", (char*)"LCD_BRIGHTNESS", ret);
		}
		PressKey = NOKEY;
		delay(200);
	}
	return;
}
