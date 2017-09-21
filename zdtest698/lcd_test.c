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
#include "PublicFunction.h"
#include "font8x16.h"
#include "lcd_test.h"
#include "gpio.h"

extern int u2g(char *inbuf, int inlen, char *outbuf, int outlen);

extern const ASCII_lib UsedAsc[101];

unsigned char Txmode_flag;
char HzBuf[32];
unsigned char HzDat[267615]; //262144
unsigned char HzDat_12[198576]; //8274*24
static INT8U FontSize;
char LcdBuf[160 * 160];
Point_s Gui_Point = {};

void ReadHzkBuff_12()
{
	int err;
	int hdrfp;
	INT8U TempBuf[60];
	memset(TempBuf, 0, 60);
	sprintf((char *) TempBuf, "%s/12", _USERDIR_);
	hdrfp = open((char *) TempBuf, O_RDONLY);
	if (hdrfp <= 0) {
		sprintf((char *) TempBuf, "%s/12", "/nor/bin");
		hdrfp = open((char *) TempBuf, O_RDONLY);
	}
	if (hdrfp == -1) {
		fprintf(stderr, "han zi ku error!12\n");
		exit(1);
	}
	err = read(hdrfp, HzDat_12, 198576);
	if (err <= 0)
		fprintf(stderr, "\n Read font 12 error!!!err=%d", err);
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

INT8U getFontSize()
{
	return FontSize;
}

void setFontSize(INT8U size)
{
	FontSize = size;
	return;
}

//画点
void gui_pixel_color(Point_s pt, unsigned char color)
{
	unsigned int x = pt.x;
	unsigned int y = pt.y;
	//LcdBuf[y*160+x] = 0xff;
	LcdBuf[y * 160 + x] = (color == 0) ? 0 : 0xff;
	return;
}

//显示字符串  rev_flg 反选标志  1 反选  0 不反选
void gui_textshow(char *str, Point_s pos, char rev_flg)
{
	unsigned char st[256];
	unsigned int i=0, m, Len;
	unsigned int j, k, l, offset, yp, xp;
	unsigned long int rec_offset;
	unsigned long int b1, b2;
	Point_s pixel;
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
	Point_s pos;
	int fd = 0;
	if ((fd = open("/dev/fb0", O_RDWR | O_NDELAY)) == -1) {
		fprintf ( stderr, "open fb0 failed\n");
	}
	memset(showstr, 0, 100);
	u2g(str, strlen(str), showstr, 100);
	if (strcmp("clear", showstr) == 0)
		memset(LcdBuf, 0x00, 160 * 160);
	else {
		read(fd, LcdBuf, 160 * 160);
		pos.x = x;
		pos.y = y;
		setFontSize(12);
		gui_textshow(showstr, pos, 0);
	}
	write(fd, LcdBuf, 160 * 160);
	close(fd);
}

void openlight()
{
	int fd = 0;
	int on = 1;
	if ((fd = open("/dev/gpoLCD_LIGHT", O_RDWR | O_NDELAY)) > 0) {
		write(fd, &on, sizeof(int));
		close(fd);
	}
}

void closelight()
{
	int fd = 0;
	int on = 0;
	if ((fd = open("/dev/gpoLCD_LIGHT", O_RDWR | O_NDELAY)) > 0) {
		write(fd, &on, sizeof(int));
		close(fd);
	}
}

//置位byte的第bit_offset位值为bit
void setbit(int *val, INT32S bit, INT8U bit_offset)
{
	if (bit_offset < 0 || bit_offset > 31) {
		fprintf(stderr, "\n bit_offset > 31");
		return;
	}
	if (bit == 1) { //置1
		*val |= (1 << bit_offset);
	} else if (bit == 0) { //置0
		*val &= (~(1 << bit_offset));
	} else
		DEBUG_TIME_LINE( "keyval: %d", bit);
	return;
}

int getkey()
{
	int keyval = 0, keypress = 0;

	gpio_readint((char*) "/dev/gpiKEY_L", &keyval); //左
	setbit(&keypress, keyval, OFFSET_L);
	gpio_readint((char*) "/dev/gpiKEY_R", &keyval); //右
	setbit(&keypress, keyval, OFFSET_R);
	gpio_readint((char*) "/dev/gpiKEY_U", &keyval); //上
	setbit(&keypress, keyval, OFFSET_U);
	gpio_readint((char*) "/dev/gpiKEY_D", &keyval); //下
	setbit(&keypress, keyval, OFFSET_D);
	gpio_readint((char*) "/dev/gpiKEY_ENT", &keyval); //确定
	setbit(&keypress, keyval, OFFSET_O);
	gpio_readint((char*) "/dev/gpiKEY_ESC", &keyval); //取消
	setbit(&keypress, keyval, OFFSET_E);
//	gpio_readint((char*) "/dev/gpiPROG_KEY", &keyval); //编程键
//	DEBUG_TIME_LINE("keyval: %d", keyval);
//	setbit(&keypress, keyval, OFFSET_K);
	return keypress;
}
