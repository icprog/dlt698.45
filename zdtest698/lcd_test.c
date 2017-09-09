/*
 * LCD test program
 * useage:
 * lcd_test /dev/fb0 1
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "font8x16.h"
#include "zdtest698.h"
/*
 * Ioctl definitions
 */

/* Use 'L' as magic number, means LCD */
#define LCD_IOC_MAGIC  'L'
/* Please use a different 8-bit number in your code */
#define _USERDIR_ 				"/nand/bin"
#define LCD_IOCRESET    		_IO(LCD_IOC_MAGIC, 0)
#define LCD_IOC_UPDATE 			_IOW(LCD_IOC_MAGIC,  1,int)
#define LCD_IOC_BACKLIGHT		_IOW(LCD_IOC_MAGIC,  2,int)
#define LCD_IOC_AC_POWER		_IOW(LCD_IOC_MAGIC,  3,int)
#define LCD_IOC_CONTRAST		_IOW(LCD_IOC_MAGIC,  4,int)
#define LCD_IOC_STAT			_IOR(LCD_IOC_MAGIC,  5,int)
#define LCD_IOC_MAXNR 5

#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
				__LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)


#define LCM_X    160
#define LCM_Y    160
unsigned char Txmode_flag;
char HzBuf[32];
unsigned char HzDat[267615];//262144
unsigned char HzDat_12[198576];//8274*24
extern const ASCII_lib UsedAsc[101];
extern int u2g(char *inbuf,int inlen,char *outbuf,int outlen);

char LcdBuf[160*160];

void ReadHzkBuff_12()
{
	int err;
	int hdrfp;
	INT8U TempBuf[60];
	memset(TempBuf, 0, 60);
	sprintf((char *) TempBuf, "%s/12", _USERDIR_);
	hdrfp = open((char *) TempBuf, O_RDONLY);
	if(hdrfp<=0)
	{
		sprintf((char *) TempBuf, "%s/12", "/nor/bin");
		hdrfp = open((char *) TempBuf, O_RDONLY);
	}
	if (hdrfp == -1){
		fprintf(stderr,"han zi ku error!12\n");
		exit(1);
	}
	err = read(hdrfp, HzDat_12, 198576);
	if(err<=0)
		fprintf(stderr,"\n Read font 12 error!!!err=%d",err);
	close(hdrfp);
}
void ReadHzkBuff()
{
	int err;
	int hdrfp;
	char TempBuf[60];
	memset(TempBuf, 0, 60);
	sprintf((char *) TempBuf, "%s/16", "/nand/bin");
	hdrfp = open((char *) TempBuf, O_RDONLY);
	if (hdrfp == -1) {
		exit(1);
	}
	err = read(hdrfp, HzDat, 267615);
	close(hdrfp);
}

static INT8U FontSize;
INT8U getFontSize(){
	return FontSize;
}
void setFontSize(INT8U size){
	if(size!=12)
		size = 16;
	FontSize = size;
	return;
}
//画点
void gui_pixel_color(GUI_Point_t pt, unsigned char color)
{
	unsigned int x = pt.x;
	unsigned int y = pt.y;
	//LcdBuf[y*160+x] = 0xff;
	LcdBuf[y*160+x]= (color==0) ?0 : 0xff;
	return;
}
void gui_textshow(char *str, GUI_Point_t pos, char rev_flg)
{
	unsigned char st[64];
	unsigned int i=0, m, Len;
	unsigned int j, k, l, offset, yp, xp;
	unsigned long int rec_offset;
	unsigned long int b1, b2;
	GUI_Point_t pixel;
	unsigned int tmp=0;//坐标转换
	tmp = pos.x;
	pos.x = pos.y;
	pos.y = tmp;
	Len = strlen((char *) str);
	for (l = 0; l < Len; l++)
		st[l] = str[l];
	yp = pos.y;

	if(getFontSize()==16)
	{
		while (i < Len) {

			offset = 0xffff;
			if (st[i] < 0x80) {
				for(j=0; j<sizeof(UsedAsc)/17; j++){
					if (st[i] == UsedAsc[j].AnsiCode) {
						offset = j;
						break;
					}
				}
				if (offset == 0xffff) {
					i++;
					continue;
				}
				xp = pos.x;
				for (m = 0; m < 16; m++) {
					for (k = 0; k < 8; k++){
						pixel.x = yp + k;
						pixel.y = xp;
						if (rev_flg)
							gui_pixel_color(pixel, (((UsedAsc[offset].Buf[m] >> (7-k)) & 0x01) ^ 0x01));
						else
							gui_pixel_color(pixel, (UsedAsc[offset].Buf[m] >> (7-k)) & 0x01);
					}
					xp = (xp + 1) % LCM_Y;
				}
				yp = (yp + 8) % LCM_X;
				i++;
			} else {
				b1 = st[i];
				b2 = st[i + 1];
				b1 -= 0xa0;//区码
				b2 -= 0xa0;//位码
				rec_offset = (94* ( b1-1)+(b2-1))*32L;//- 0xb040;
				memcpy((void *)&HzBuf[0],(void *)&HzDat[rec_offset], 32);
				k=0;
				xp = pos.x;
				for(m=0;m<16;m++)
				{
					for(k=0;k<8;k++){
						pixel.x = yp + k;
						pixel.y = xp;
						if (rev_flg)
							gui_pixel_color(pixel, (((HzBuf[m*2]>>(7-k))&0x01)^0x01));
						else
							gui_pixel_color(pixel, (HzBuf[m*2]>>(7-k))&0x01);
					}
					for(k=0;k<8;k++){
						pixel.x = yp + k + 8;
						pixel.y = xp;
						if (rev_flg)
							gui_pixel_color(pixel, ((HzBuf[m*2+1]>>(7-k))&0x01)^0x01);
						else
							gui_pixel_color(pixel, (HzBuf[m*2+1]>>(7-k))&0x01);
					}
					xp=(xp+1)%LCM_Y;
				}
				yp=(yp+16)%LCM_X;
				i+=2;
			}
		}
	}else{
		while (i < Len) {
			if (st[i] < 0x80) {
				b1 = st[i];
				b1 -= 0x20;//区码
				rec_offset = b1 * 24;
				memcpy((void *)&HzBuf[0],(void *)&HzDat_12[rec_offset], 24);
				xp = pos.x;
				k = 0;
				for (m = 0; m < 12; m++) {
					for (k = 0; k < 6; k++) {
						pixel.x = yp + k;
						pixel.y = xp;
						if(rev_flg)
							gui_pixel_color(pixel, ((HzBuf[m*2]>>(7-k))&0x01)^0x01);
						else
							gui_pixel_color(pixel, (HzBuf[m*2]>>(7-k))&0x01);
					}
					xp = (xp + 1) % LCM_Y;
				}
				yp = (yp + 6) % LCM_X;
				i++;
			} else {
				b1 = st[i];
				b2 = st[i + 1];
				b1 -= 0xa0;//区码
				b2 -= 0xa0;//位码
				rec_offset = (94* ( b1-1)+(b2-1))*24L;//- 0xb040;
				rec_offset += 96*24;
				memcpy((void *)&HzBuf[0],(void *)&HzDat_12[rec_offset], 24);
				k=0;
				xp = pos.x;
				for(m=0;m<12;m++){
					for(k=0;k<8;k++){
						pixel.x = yp + k;
						pixel.y = xp;
						if(rev_flg)
							gui_pixel_color(pixel, ((HzBuf[m*2]>>(7-k))&0x01)^0x01);
						else
							gui_pixel_color(pixel, (HzBuf[m*2]>>(7-k))&0x01);
					}
					for(k=0;k<4;k++){
						pixel.x = yp + k + 8;
						pixel.y = xp;
						if(rev_flg)
							gui_pixel_color(pixel, ((HzBuf[m*2+1]>>(7-k))&0x01)^0x01);
						else
							gui_pixel_color(pixel, (HzBuf[m*2+1]>>(7-k))&0x01);
					}
					xp=(xp+1)%LCM_Y;
				}
				yp=(yp+12)%LCM_X;
				i+=2;
			}
		}
	}
	return;
}

void lcd_disp(char *str, int x, int y)
{
	char showstr[100];
	GUI_Point_t pos;
	int fd=0;
	if((fd = open("/dev/fb0", O_RDWR | O_NDELAY)) == -1){
		fprintf(stderr,"open fb0 failed\n");
		FATAL;
	}
	memset(showstr, 0, 100);
	u2g(str,strlen(str), showstr, 100);
	if(strcmp("clear", showstr)==0)
		memset(LcdBuf, 0x00, 160*160);
	else{
		read(fd, LcdBuf, 160*160);
		pos.x = x;
		pos.y = y;
		setFontSize(12);
		gui_textshow(showstr, pos, 0);
	}
	write(fd,LcdBuf,160*160);
	close(fd);
}

void openlight()
{
	int fd=0;
	int on=1;
	if((fd = open("/dev/gpoLCD_LIGHT", O_RDWR | O_NDELAY)) > 0)
	{
		write(fd,&on,sizeof(int));
		close(fd);
	}
}
void closelight()
{
	int fd=0;
	int on=0;
	if((fd = open("/dev/gpoLCD_LIGHT", O_RDWR | O_NDELAY)) > 0)
	{
		write(fd,&on,sizeof(int));
		close(fd);
	}
}

//置位byte的第bit_offset位值为bit
void setbit(int *val, char bit, char bit_offset)
{
	if(bit_offset<0 || bit_offset>31){
		fprintf(stderr,"\n bit_offset > 31");
		return;
	}
	if(bit==1){//置1
		*val |=(1<<bit_offset);
	}else if(bit==0){//置0
		*val &=(~(1<<bit_offset));
	}else
		fprintf(stderr,"\n setbit bit != 1 && bit != 0");
	return;
}
int getkey()
{
	int keyval=0, keypress=0;
	keyval = gpio_read((char*)"/dev/gpiKEY_L");//左
	setbit(&keypress, keyval, OFFSET_L);
	keyval = gpio_read((char*)"/dev/gpiKEY_R");//右
	setbit(&keypress, keyval, OFFSET_R);
	keyval = gpio_read((char*)"/dev/gpiKEY_U");//上
	setbit(&keypress, keyval, OFFSET_U);
	keyval = gpio_read((char*)"/dev/gpiKEY_D");//下
	setbit(&keypress, keyval, OFFSET_D);
	keyval = gpio_read((char*)"/dev/gpiKEY_ENT");//确定
	setbit(&keypress, keyval, OFFSET_O);
	keyval = gpio_read((char*)"/dev/gpiKEY_ESC");//取消
	setbit(&keypress, keyval, OFFSET_E);
	keyval = gpio_read((char*)"/dev/gpiPROG_KEY");//编程键
	setbit(&keypress, keyval, OFFSET_K);
	return keypress;
}
