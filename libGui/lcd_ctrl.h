/*
 * lcd_ctrl.h
 *
 *  Created on: 2013-5-24
 *      Author: Administrator
 */

#ifndef LCD_CTRL_H_
#define LCD_CTRL_H_
#include "comm.h"
#include "gui.h"

#define MSGBOX_TEXTSIZE 20
//液晶键盘的定义
#define KEYBOARD_LEN	4	//竖着长度
#define KEYBOARD_WIDTH	10  //横着长度
#define INPUTKEYNUM 50//edit输入字符串的最大个数
#define TEXTLEN_X 16  //combox显示内容 下标_x
#define TEXTLEN_Y 50  //combox显示内容 下标_y

#define OPRMODE_MODIFY 	1
#define OPRMODE_LOOK 	2
//数组下标
typedef struct{
	unsigned char index_x;
	unsigned char index_y;
}Array_index;
//液晶显示的按键
typedef struct{
	char c;				//对应的字符
	Array_index index;	//字符在键盘中的位置的索引  液晶位置
	Point pos;			//字符在键盘中的位置
}Key;
typedef struct{
	Point pos;
	char text[50];
}Text_t;
extern Key KeyBoard[KEYBOARD_LEN][KEYBOARD_WIDTH];
//键盘样式
// ----------------------
// | 0 1 2 A B C D E F G |
// | 3 4 5 H I J K L M N |
// | 6 7 8 O P Q R S T U |
// | 9 . - V W X Y Z     |
// -----------------------

//控件
//typedef struct _Form{
//	int CtlType; 	// 1: 输入框  2: 按钮； 3:combox
//	char name[50]; 	//控件名称
//	char mode;   	//控件模式  分为正常模式、查看模式、编辑模式
//	char focus; 	//控件焦点
//	Point pos;		//控件位置
//	Rect rect; 		//控件区域
//	Key key[INPUTKEYNUM]; //对于输入框 :控件输入的内容; 对于combox: key[0]表示选中的项目
//	int c_num;		//控件里的字符个数
//	char text[TEXTLEN_X][TEXTLEN_Y];//combox显示的内容
//	unsigned char cur_index;//combox初始化的的内容  默认的
//	char frame_flg;  //是否有边框 1 为有  0 为没有
//	char ret_flg;  //返回值 包括此控件 只限于在msgbox里
//	char keyboard_style; //键盘的类型
//	//函数----
//	void(*pfun_init)(struct _Form *form);
//	void(*pfun_show)(struct _Form *form);
//	void(*pfun_process)(struct _Form *form);
//	struct list node;
//	int pfun_process_ret;//按下确定键的时候 保存数据
//}Form;
typedef struct _Form{
	int CtlType; 	// 1: 输入框  2: 按钮； 3:combox
	Point pos;		//控件位置
	Rect rect; 		//控件区域
	char focus; 	//控件焦点
	int pfun_process_ret;//按下确定键的时候 pfun_process的返回值 保存数据
	char ret_flg;  //返回值 包括此控件 只限于在msgbox里
	Key key[INPUTKEYNUM]; //对于输入框 :控件输入的内容;对于其它的则是
	int c_num;		//控件里的字符个数
//	void(*pfun_init)(void *form);
	void(*pfun_show)(void *form);
	void(*pfun_process)(void *form);
	struct list node;
}Form;
typedef struct _Button{
	struct _Form form;
	char name[50];
}Button;
typedef struct _Edit{
	struct _Form form;
	char keyboard_style; //键盘的类型
	char frame_flg;  //是否有边框 1 为有  0 为没有
	char mode;   	//控件模式  分为正常模式、查看模式、编辑模式
}Edit;
typedef struct _Combox{
	struct _Form form;
	char text[TEXTLEN_X][TEXTLEN_Y];//combox显示的内容
	unsigned char cur_index;//combox初始化的的内容  默认的
	char mode;   	//控件模式  分为正常模式、查看模式、编辑模式
}Combox;

//msgbox的返回值
typedef struct {
	char s_ret[INPUTKEYNUM];
	char btn_ret;
}MsgBoxRet;
void showkeyboard(Key key[][KEYBOARD_WIDTH], char keyboard_style);	//显示液晶键盘

unsigned char gui_keyinput(Point start_pos, Key *curkey, char keyboard_style);	//从液晶键盘上输入一个字符
unsigned char edit_editmode(Edit* edit, Key *curkey,int  *press_ok);//编辑框编辑模式
extern unsigned char edit_showmode(Edit* edit, Key *curkey);//查看模式即非编辑模式
void button_init(Button *btn, char *name, Point pos, char type, struct list *parent);//初始化液晶按钮
//初始化编辑框
void edit_init(Edit *edit, char *text, int c_num, Point pos, char frame_flg, char ret_flg, struct list *parent, char keyboard_style);
void editip_init(Edit *edit, char type, char *text, int c_num, Point pos, char frame_flg, char ret_flg,struct list *parent, char keyboard_style);
void editip_show(void *form);
void edittime_init(Edit *edit, char type, char *text, int c_num, Point pos, char frame_flg,char ret_flg, struct list *parent);
void edittime_show(void *form);

//void msgbox_init(void *form, Point pos, int len, int width);//初始化msgbox
void combox_init(Combox *combox, int default_index, char (*text)[TEXTLEN_Y],Point pos, char ret_flg, struct list *parent);
void combox_show(void *form);
void button_show(void *form);						//显示按钮
void edit_show(void *form);
void combox_process(void *form);
void button_process(void *form);						//显示按钮
void edit_process(void *form);							//显示编辑框
void msgbox(Rect form_rect, struct list *head, Text_t *ttext, MsgBoxRet *msgbox_ret);
int msgbox_passwd(char *passwd, int len);
int msgbox_passwd_js(char *passwd, int len);
int msgbox_setpasswd(char *passwd, int len, char *s_passwd_ret);
int msgbox_label(char *text, int ctrl_focus);
int msgbox_inputcldno();
int msgbox_inputcldaddr(char *cldaddr,int num);
int msgbox_inputjzqtime(char *jzqtime, int len);
int msgbox_jzqtimeset(char *s_jzqtime, int len);
int msgbox_jzqaddr_10(char *s_jzqdizhi10, int len);
int msgbox_jzqaddr_16(char *s_jzqdizhi10, int len);
int msgbox_jzqip(char *s_ip, int len);
int msgbox_masterip(char *s_ip, int len);
int msgbox_apn(char *s_apn);
int msgbox_oprmode(int ctrl_focus);
int msgbox_soubiao(int *index, MsgBoxRet *msgbox_ret);
int msgbox_autosoubiao(int *index);
int msgbox_heart(char *s_heart, int len);
int menu_485func_show(char *text,INT8U ctrl_focus);
extern int get_oprmode();
extern void set_oprmode(int mode);

#ifdef JIANGSU
int editctrl_time(char *s_jzqtime, int len);
int msgbox_password_js(char *passwd, int len, char *s_passwd_ret);
int msgbox_jzqaddr_10_js(char *s_jzqdizhi10, int len);
#endif
#endif /* LCD_CTRL_H_ */
