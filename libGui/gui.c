/*lcd.c 画图函数
 * */
#include "gui.h"
#include "font8x16.h"

unsigned char LcdBuf[LCM_X*LCM_Y]; //液晶显存

volatile char g_LcdPoll_Flag; //液晶是否在轮显中
Rect rect_TopStatus, rect_BottomStatus, rect_Client;//液晶划分的3个区域
//字库缓冲区
char HzBuf[32];

unsigned char HzDat[267615];//262144
unsigned char HzDat_12[198576];//8274*24

volatile INT8U time_show_flag = 0;
INT8U ret_show_time_flag()
{
	return time_show_flag;
}
void set_time_show_flag(INT8U value)
{
	time_show_flag = value;
}

void ReadHzkBuff_16()
{
	int hdrfp;
	unsigned char TempBuf[60];
	memset(TempBuf, 0, 60);
	sprintf((char *) TempBuf, "%s/16", _USERDIR_);
	hdrfp = open((char *) TempBuf, O_RDONLY);
	if(hdrfp<=0)
	{
		sprintf((char *) TempBuf, "%s/16", "/nor/bin");
		hdrfp = open((char *) TempBuf, O_RDONLY);
	}
	if (hdrfp == -1) {
		fprintf(stderr,"han zi ku error!16\n");
		exit(1);
	}
	read(hdrfp, HzDat, 267615);
	close(hdrfp);
}
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
//画点
void gui_pixel_color(Point pt, unsigned char color)
{
	unsigned int x = pt.x;
	unsigned int y = pt.y;
	//LcdBuf[y*160+x] = 0xff;
	LcdBuf[y*160+x]= (color==0) ?0 : 0xff;
	return;
}
//画点
void gui_pixel(Point pt)
{
	unsigned int x = pt.x;
	unsigned int y = pt.y;
	LcdBuf[y*160+x] = 0xff;
	return;
}
//画横线
void gui_hline(Point start, int x_pos)
{
	int i=0,tmp=0;
	Point pt;
	pt.y = start.y;
	if(x_pos<start.x){
		tmp = start.x;
		start.x = x_pos;
		x_pos = tmp;
	}
	for(i=start.x; i<x_pos; i++){
		pt.x = i;
		gui_pixel(pt);
	}
}
//画竖线
void gui_vline(Point start, int y_pos)
{
	int i=0,tmp=0;
	Point pt;
	pt.x = start.x;
	if(y_pos<start.y){
		tmp = start.y;
		start.y = y_pos;
		y_pos = tmp;
	}
	for(i=start.y; i<y_pos; i++){
		pt.y = i;
		gui_pixel(pt);

	}
}
//画斜线 45度角 oblique line
void gui_oline(Point start, Point end)
{
	unsigned int len=0, i=0;
	Point pt;
	pt.x = start.x;
	pt.y = start.y;	
	len = abs((long)(end.x-start.x));
	for(i=0; i<len; i++)
	{
		if(end.x>start.x)
			pt.x++;
		else
			pt.x--;
		if(end.y>start.y)
			pt.y++;
		else
			pt.y--;
		if(pt.x<=0) pt.x = 0;
		if(pt.x>=LCM_X) pt.x = LCM_X-1;
		if(pt.y<=0) pt.y = 0;
		if(pt.y>=LCM_Y) pt.y = LCM_Y-1;
		gui_pixel(pt);
	}
}
//画矩形
void gui_rectangle(Rect rect)
{
	Point pt1, pt2, pt3, pt4;
	pt1.x = rect.left;
	pt1.y = rect.top;
	pt2.x = rect.right;
	pt2.y = rect.top;
	pt3.x = rect.right;
	pt3.y = rect.bottom;
	pt4.x = rect.left;
	pt4.y = rect.bottom;
	gui_hline(pt1, pt2.x); //pt1-->pt2
	gui_vline(pt2, pt3.y); //pt2-->pt3
	gui_hline(pt3, pt4.x); //pt3-->pt4
	gui_vline(pt4, pt1.y); //pt4-->pt1
	set_time_show_flag(1);//TODO:new
}

void gui_setrect(Rect *rect, int left, int top, int right, int bottom){
	if(rect==NULL)
		return;
	rect->left = left;
	rect->top = top;
	rect->right = right;
	rect->bottom = bottom;
}
void gui_setpos(Point *pos, int x, int y){
	if(pos==NULL)
		return;
	pos->x = x;
	pos->y = y;
}
//根据字符串的位置获得字符串区域
Rect gui_getstrrect(unsigned char *str, Point pos)
{
	Rect rect;
	int len = strlen((char*)str);
	if(len<0)
		len = 0;
	rect.left = pos.x;
	rect.top = pos.y;
	rect.right = pos.x + len*FONTSIZE;
	rect.bottom = pos.y + 2*FONTSIZE;
	return rect;
}
Rect gui_getcrect(unsigned char c, Point pos)//获得字符区域
{
	Rect rect;
	rect.left = pos.x;
	rect.top = pos.y;
	rect.right = pos.x + FONTSIZE;
	rect.bottom = pos.y + 2*FONTSIZE;
	return rect;
}
Rect gui_moverect(Rect rect, int direction, int offset)
{
	if((direction & UP) !=0){
		rect.top -= offset;
		rect.bottom -= offset;
	}
	if((direction & LEFT) !=0){
		rect.left -= offset;
		rect.right -= offset;
	}
	if((direction & RIGHT) !=0){
		rect.left += offset;
		rect.right += offset;
	}
	if((direction & DOWN) !=0){
		rect.top += offset;
		rect.bottom += offset;
	}
	return rect;
}
//清除某个区域
void gui_clrrect(Rect rect)
{
	int i=0,j=0;
	for(j=rect.top; j<rect.bottom; j++){
		for(i=rect.left; i<rect.right; i++)
			LcdBuf[j*160+i] = 0;
	}
}
//反选某个区域
void gui_reverserect(Rect rect)
{
	int i=0,j=0;
	for(j=rect.top; j<rect.bottom; j++){
		for(i=rect.left; i<rect.right; i++){
			if(LcdBuf[j*160+i]==0)
				LcdBuf[j*160+i] = 0xff;
			else if(LcdBuf[j*160+i]==0xff)
				LcdBuf[j*160+i] = 0;
		}
			//LcdBuf[j*160+i] = (~LcdBuf[j*160+i]);
	}
}
//反选字符串
void gui_textshowreverse(unsigned char *str, Point pos)
{
	Rect rect;
	rect = gui_getstrrect(str, pos);
	gui_reverserect(rect);
	set_time_show_flag(1);//TODO:new
}
//放大或缩小区域
Rect gui_changerect(Rect rect, int size)
{
	rect.top -= size;
	rect.left -= size;
	rect.right += size;
	rect.bottom += size;
	return rect;

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
//void GUI_DispStringAt(unsigned char *format, unsigned int y, unsigned int x)
//显示字符串  rev_flg 反选标志  1 反选  0 不反选
void gui_textshow(char *str, Point pos, char rev_flg)
{
	unsigned char st[64];
	unsigned int i=0, m, Len;
	unsigned int j, k, l, offset, yp, xp;
	unsigned long int rec_offset;
	unsigned long int b1, b2;
	Point pixel;
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
void gui_textshow_16(char *str, Point pos, char rev_flg)
{
	unsigned char st[64];
	unsigned int i=0, m, Len;
	unsigned int j, k, l, offset, yp, xp;
	unsigned long int rec_offset;
	unsigned long int b1, b2;
	Point pixel;
	unsigned int tmp=0;//坐标转换
	tmp = pos.x;
	pos.x = pos.y;
	pos.y = tmp;
	Len = strlen((char *) str);
	for (l = 0; l < Len; l++)
		st[l] = str[l];
	yp = pos.y;

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
	return;
}
//显示单个字符
void gui_charshow(char c, Point pos, char rev_flag)
{
	char str[2];
	memset(str, 0, 2);
	str[0] = c;
	gui_textshow(str, pos, rev_flag);
	return; 
}
//-------------------------------------------------------------------//
//根据起始点point(左上角)、长和宽构造一个区域
Rect getrect(Point pos, int len, int width)
{
	Rect rect;
	rect.left = pos.x;
	rect.top = pos.y;
	rect.right = pos.x + len;
	rect.bottom = pos.y + width;
	return rect;
}
//保存rect对应的液晶缓存buf
void lcdregion_save(unsigned char *buf, Rect rect)
{
	int i=0, j=0;
	if(rect.bottom >= LCM_Y)
		rect.bottom = LCM_Y-1;
	if(rect.right >= LCM_X)
		rect.bottom = LCM_X-1;
	for(j=rect.top; j<=rect.bottom; j++){
		for(i=rect.left; i<=rect.right; i++)
			buf[j*160+i] = LcdBuf[j*160+i];
	}
}
void lcdregion_restore(unsigned char *buf, Rect rect)
{
	int i=0, j=0;
	if(rect.bottom >= LCM_Y)
		rect.bottom = LCM_Y-1;
	if(rect.right >= LCM_X)
		rect.bottom = LCM_X-1;
	for(j=rect.top; j<=rect.bottom; j++){
		for(i=rect.left; i<=rect.right; i++)
			LcdBuf[j*160+i] = buf[j*160+i];
	}
}
char* skip_unwanted(char *ptr)
{
	int i = 0;
	static char buf[256];
	while (*ptr != '\0') {
		if (*ptr == '\t' || *ptr == '\n') {
			ptr++;
			continue;
		}
		if (*ptr == '#')
			break;
		buf[i++] = *ptr;
		ptr++;
	}
	buf[i] = 0;
	return (buf);
}
/*****************************************************
 * 函数说明:解析配置文件的具体参数
 ****************************************************/
int parse_options(char *buf,int type)
{
	char 	*str,*str1;
	static char buf1[256];
	INT8U	i,j;
	int baud = 0;
	str = strstr(buf, "end");
	if (str != NULL) {
		return 100;
	}
	if(type == 1)
		str = strstr(buf,"Ifr");
	else
		str = strstr(buf,"Com");
	if(str!=NULL) {
		str = index(buf, '=');
		if (str != NULL) {
			str1 = str;
			str++;
			if (*str != '\0') {
				for(j=0;j<4;j++) {
					str1++;
					i=0;
					while ((*str1 != ' ') && (*str1 != '\0')) {
						buf1[i++] = *str1;
						str1++;
					}
					buf1[i] = 0;
					switch(j) {
						case 0:	baud = strtol(buf1, NULL, 10);  break;
//						case 1:	strncpy((char *)ComParaIfr.Parity,buf1,12);	break;
//						case 2:	ComParaIfr.Bits = strtol(buf1, NULL, 10);  break;
//						case 3:	ComParaIfr.Stopb = strtol(buf1, NULL, 10);  break;
					}
				}
				return (baud);
			}
		}
		return (0);
	}
	return (0);
}

/*****************************************************
 * 函数说明:读串口参数配置文件(/nor/config/ttyS.cfg)
 ****************************************************/
int parse_ttyS_file(int type)
{
	FILE *fp;
	char line[256];
	char TempBuf[64];
	int  baud=0;
	char *ptr;

	sprintf((char *) TempBuf, "%s/ttyS.cfg", _CFGDIR_);
	fp = fopen(TempBuf, "r");
	if (fp == NULL) {			//无配置文件,初始化串口默认值
		return (-1);
//		ComParaIfr.Baud=2400;
//		strncpy((char *)ComParaIfr.Parity,"even",sizeof(ComParaIfr.Parity));
//		ComParaIfr.Bits=8;
//		ComParaIfr.Stopb=1;
	}else {
		while (fgets(line, 256, fp) != NULL) {
			ptr = skip_unwanted(line);
			baud = parse_options(ptr,type);
			if(baud >0)
				break;
    	}
	 	fclose(fp);
		return baud;
	}
}
void para_write(int type,int baud)
{
	FILE *fp;
	char line[256];
	char TempBuf[64];
	int ifrbaud=0,combaud=0,ret=0;
	char *ifrindex,*comindex;
	char str_ifrbaud[4],str_combaud[4];
	sprintf((char *) TempBuf, "%s/ttyS.cfg", _CFGDIR_);
	ret = access((char *)TempBuf, F_OK);
	if (ret != 0) {
		fp = fopen((char*) TempBuf, "w+");		//无配置文件,初始化串口默认值
		if(type == 1) //ifr
			combaud = 9600;
		if(type == 2)
			ifrbaud = 2400;
	}else {
		fp = fopen((char*) TempBuf, "r+");
		while (fgets(line, 256, fp) != NULL)
		{
			if(strstr(line,"Ifr=") != NULL)
			{
				memset(str_ifrbaud,0,4);
				ifrindex = strstr(line,"Ifr=");
				memcpy(str_ifrbaud,ifrindex+4,4);
				ifrbaud = strtol(str_ifrbaud,NULL,10);
				memset(str_ifrbaud,0,4);
			}
			if(strstr(line,"Com=") != NULL)
			{
				memset(str_combaud,0,4);
				comindex = strstr(line,"Com=");
				memcpy(str_combaud,comindex+4,4);
				combaud = strtol(str_combaud,NULL,10);
			}
    	}
		fclose(fp);
		fp = fopen((char*) TempBuf, "w+");
	}
	if(type == 1) //ifr
	{
		fprintf(fp,"Ifr=%d even 8 1 \r\n",baud);
		fprintf(fp,"Com=%d even 8 1 ",combaud);
	}
	if(type == 2)
	{
		fprintf(fp,"Ifr=%d even 8 1 \r\n",ifrbaud);
		fprintf(fp,"Com=%d even 8 1 ",baud);
	}
	 	fclose(fp);
}
