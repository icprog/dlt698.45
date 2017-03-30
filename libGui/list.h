//list  双向循环链表
#ifndef LIST_H_
#define LIST_H_
#include "comm.h"
#define FLAG_BEFORE 0
#define FLAG_AFTER 1 
struct list
{
	struct list *prev, *next, *parent, *child;
};

typedef  unsigned int size_t;
//#define list_for_each(pos,head) for(pos=head->next; pos!=(head); pos=pos->next)

#define offof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)//获取TYPE类型结构体的MEMBER成员的地址，由于地址是从0开始的，所以这个地址的整型值就是相对与结构体首的偏移字节数


//typeof用于获取表达式的数据类型
//定义一个变量__mptr,它的类型和member相同，const typeof(((type *)0)->member) * __mptr = (ptr)是为了获取实际分配的member的地址
//指针ptr为type类型结构体的member变量
//实际的member的地址减去其在结构体中的偏移得到该结构体的实际分配的地址
#define container_of(ptr, type, member) ({			\
	const typeof(((type *)0)->member) * __mptr = (ptr);	\
	(type *)((char *)__mptr - offof(type, member)); })
	
/*获取某个链表项的地址*/
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)
//  &pos->member != (head);
/*获取链表第pos个链表项的地址*/
#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     pos->member != NULL; 	\
	     pos = list_entry(pos->member.next, typeof(*pos), member))
	     
//初始化链表或者节点，使此节点的所有指针指向本身
void list_init(struct list *list);
void list_add_head(struct list *head, struct list *add);//在head和第一个节点之间加入一个节点
void list_add_tail(struct list *head, struct list *add);//在head的链表尾加入一个节点
void list_del(struct list *entry);//删除本节点
//判断链表是否为空    返回： 空为1 非空为0
 int list_empty(struct list *head);
struct list *list_getnext(struct list *node);
struct list *list_getprev(struct list *node);
struct list *list_getparent(struct list *node);
struct list *list_getchild(struct list *node);
struct list *list_getlast(struct list *node);//获得最后一个节点
struct list *list_getfirst(struct list *node);//获得头节点
void list_print(struct list *node);
//--------------------------------------
//在链表中定位到node的前第num个节点，如果到达链表头则指向第一个节点
struct list *list_getPrevNumNode(struct list *head, struct list *node, int num);
//在链表中定位到node的后第num个节点，如果到达链表尾则指向最后一个节点
struct list *list_getNextNumNode(struct list *head, struct list *node, int num);
int list_getListNum(struct list *head);//获得链表节点总数
int list_getListIndex(struct list *head, struct list *node);//获得节点在链表中的位置
//判断node在不在从start起num个节点之间 在第一个几点之前返回1 在中间返回2在第二个节点之后返回3
int listbetween(struct list *node, struct list *start, int num);
#endif//end of list.h
