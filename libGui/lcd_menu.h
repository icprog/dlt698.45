#ifndef LCD_MENU_H_
#define LCD_MENU_H_

#include "list.h"
#include "comm.h"
#include "lcdprt.h"
//#include "menu.h"
//结构体定义------------------

#define level0 	0
#define level1 	1
#define level2 	2
#define level3 	3
#define level4 	4
#define level5 	5
#define level6 	6

//结构体定义------------------
typedef void(*pFUN)();
typedef struct  {
	int	 level;//菜单项的目录级别
	char name[50];
	pFUN fun;
	char ispasswd;
}ElemType; 
//
typedef struct//菜单链表
{
	struct list node;
	ElemType data;
}MenuList;

typedef struct{//菜单数组
	ElemType data;
	void *pthis;			//存放此链表节点地址
}Menu;

typedef struct
{
	char *str_data;
	INT8U id_no;
}projno;
//#define MENUITEMCOUNT 20 //菜单项总数 (菜单总数+head0)

#define MENU_PAGEITEM_COUNT 6 //6液晶一屏显示几行菜单
void pass_mpdata(INT8U flag,INT16U data_id,INT16U mq_cont,LcdDataItem *item);
void show_xuliang(INT8U data_cont,LcdDataItem *item,int did);
void show_mpdata(INT8U data_genus,INT8U data_cont,LcdDataItem *item,projno *str_name);
int msgbox_jzqtimeset(char *s_jzqtime, int len);
INT8U getmpport(INT16U index);
void menu_jzqtime_js();
extern MenuList *ComposeDList(Menu *menu, int menu_count);
extern void initmenu();
extern void ShowItself(struct list *head);
extern void showmain();
extern int my_min(int a, int b);
extern void deletemenu(MenuList *pmenulist_head);

extern int getMenuSize_jzq();
extern int getMenuSize_fk();
#endif

//------------------------------------function-----------------------

