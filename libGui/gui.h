#ifndef LCD_H_
#define LCD_H_
#include "list.h"
#include "comm.h"
#include "font8x16.h"
#include "../include/StdDataType.h"
//液晶点坐标
typedef struct {
	int x;
	int y;
}Point;
//液晶区域
typedef struct{ 
    int left;
    int top;
    int right;
    int bottom;
}Rect;

extern Rect rect_TopStatus, rect_BottomStatus, rect_Client;//液晶划分的3个区域

//字库缓冲区
extern char HzBuf[32];
extern unsigned char HzDat[267615];//262144
extern unsigned char HzDat_12[198576];//8274*24

//缓冲区刷新标志处理
extern volatile INT8U show_time_flag;
extern INT8U ret_show_time_flag();
extern void set_time_show_flag(INT8U value);

void gui_setrect(Rect *rect, int left, int top, int right, int bottom);
void gui_setpos(Point *pos, int x, int y);

void gui_pixel(Point pt); //画点
void gui_pixel_color(Point pt, unsigned char color); //专为汉字画点
void gui_hline(Point start, int x_pos);//画横线
void gui_vline(Point start, int y_pos);//画竖线
void gui_oline(Point start, Point end);			//画45度斜线
void gui_rectangle(Rect rect);					//画矩形
Rect gui_getstrrect(unsigned char *str, Point pos);//获得字符串区域
Rect gui_getcrect(unsigned char c, Point pos);//获得字符区域
Rect gui_moverect(Rect rect, int direction, int offset);//平移区域
void gui_clrrect(Rect rect);					//清除某个区域
void gui_reverserect(Rect rect);					//反显区域
void gui_textshowreverse(unsigned char *str, Point pos);//反显字符串
Rect gui_changerect(Rect rect, int size);		//放大或缩小区域
void gui_textshow(char *str, Point pos, char rev_flg);		//显示字符串
void gui_textshow_16(char *str, Point pos, char rev_flg);		//显示字符串
void gui_charshow(char c, Point pos, char rev_flag);			//显示字符
void initkeyboard(Point start_pos,char keyboard_style);				//初始化液晶键盘

Rect getrect(Point pos, int len, int width);	//根据左点和长宽获得区域
void lcdregion_save(unsigned char *buf, Rect rect);//保存一块区域
void lcdregion_restore(unsigned char *buf, Rect rect);//恢复此块区域

extern INT8U getFontSize();
extern void setFontSize(INT8U size);
void ReadHzkBuff_16();
void ReadHzkBuff_12();

int readcfg(char *filename, char *name, char *ret);
int writecfg(char *filename, char *name, char *value);

char* skip_unwanted(char *ptr);
int parse_options(char *buf,int type);
int parse_ttyS_file(int type);
void para_write(int type,int baud);

#endif
