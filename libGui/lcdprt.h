/*
 * lcd_prt.h  液晶显示
 *
 *  Created on: 2013-5-19
 *      Author: Administrator
 */

#ifndef LCD_PRT_H_
#define LCD_PRT_H_
#include "gui.h"
#include "show_ctrl.h"

#define LcdDataItem_VALLEN 7  //数据长度 5+2 共7个字节
//每个数据项对应的数值
typedef struct{
	int 	index;    	//菜单上显示第几个
	INT16U	dataflg_id;	//数据项id
	char 	name[50]; 	//数据项名字
	char 	val[LcdDataItem_VALLEN];		//数据值
	char 	suffix[20];	//后缀   kw、kvar
	Point 	pos;		//显示位置 可以不赋值 为以后预留
}LcdDataItem;

struct erc_t{
    char Ercname[50];
};

typedef void (*pfun_lunxian)(ProgramInfo* jprograminfo,LcdDataItem *item, int size, INT8U show_flg);
typedef struct{
	char label_name[30];  //轮显显示的标签
	pfun_lunxian pfun;//轮显的显示函数
	char isshow_flg; //0 不显示  1 显示
}LunXian_t;

#define EVENTCOUNT 61
extern struct erc_t ERCNAME[EVENTCOUNT];

extern Rect rect_TopStatus, rect_BottomStatus, rect_Client;//液晶划分的3个区域
//void showpage(LcdDataItem *item, int item_index, int item_total, Point page_pos);

#define PAGEITEM_COUNT 	4 //每页显示的数据项数量
#define PAGE_COLNUM 	10 //每页显示的行数  指客户区
//lcdpoll.c
#define JC_ITEM_COUNT     100//轮寻数据项个数

#define CURR_DATA 		1
#define DONGJIE_DATA 	2
#define QUXIAN_DATA 	3

#ifdef SPTF_III
#define LunPageNum 	11  //轮寻页数
#else
#define LunPageNum 	11  //轮寻页数
#endif
#define JCPageNum 	11  //轮寻交采页数

#define ROW_INTERVAL 3
#define CMD_GUI_GETREAlDATA 	1 //TODO
#define CMD_GUI_POWERCTRL_REC   2 //TODO

extern INT8U DongJie_FileName[100];
extern int isValidMeterAddr_12(char *addr);
extern void show_djdata(int cldno);//显示冻结数据 正向有功

extern mqd_t createMsg(INT8S *mq_name, INT8U flg);
extern int sendMsg(mqd_t mqd, INT32U cmd, INT8S *sendbuf, INT32U bufsiz);
//extern int recvMsg(mqd_t mqd, mmq_head *mqh, INT8U *recvbuf, int timeout);
extern void getlocalip(char *ip);
extern int bcd2int(char *bcd);
extern int get_oprmode();
extern void set_oprmode(int mode);

extern int get_itemdata1(LcdDataItem *item, int size, int did, FP64 *dval, int decimal);

extern double lcd_A02toDouble(short A2);
extern double lcd_A02toDouble_decbits(INT16S A2, INT8U decbits);
extern int lcd_A03toint(int A3, int *G);
extern int lcd_A03toint_c(INT8U *data, int *G);
extern char lcd_A04tochar(char A4, int *S);
extern void lcd_A15toDate(INT8U* A15,INT8U* year,INT8U* month,INT8U* day,INT8U* hour,INT8U* min);
extern void lcd_A19toDate(char* A19, char* hour, char* min);
extern void lcd_A20toDate(char* A20,char* year,char* month,char* day);

extern int gui_isValidCld(int cldno);
extern int gui_GetCldNum();
extern int gui_GetJCMP();
extern int gui_isValidCldAddr(INT8U *addr);
extern int ip_strtobyte(INT8U *sip, INT8U *ip_ret);
extern void ip2asc(INT8U *ip, char* sip);
typedef struct{
	struct list *head;
	struct list *begin;
	struct list *current;
}MenuStat_t;

#define STACK_TYPE MenuStat_t /* 堆栈所存储的值的数据类型 */
///*
//** 函数原型：create_stack
//** 创建堆栈，参数指定堆栈可以保存多少个元素。
//** 注意：此函数只适用于动态分配数组形式的堆栈。
//*/
//void create_stack(size_t size);
///*
//** 函数原型：destroy_stack
//** 销毁一个堆栈，释放堆栈所适用的内存。
//** 注意：此函数只适用于动态分配数组和链式结构的堆栈。
//*/
//void destroy_stack(void);
/*
** 函数原型：push
** 将一个新值压入堆栈中，参数是被压入的值。
*/
int push(STACK_TYPE *value);

/*
** 函数原型：pop
** 弹出堆栈中栈顶的一个值，并丢弃。
*/
int pop(void);
/*
** 函数原型：top
** 返回堆栈顶部元素的值，但不改变堆栈结构。
*/
int top(STACK_TYPE *value);
/*
** 函数原型：is_empty
** 如果堆栈为空，返回TRUE,否则返回FALSE。
*/
int is_empty(void);
/*
** 函数原型：is_full
** 如果堆栈为满，返回TRUE,否则返回FALSE。
*/
int is_full(void);
#endif /* LCD_PRT_H_ */
