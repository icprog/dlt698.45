//list  双向循环链表
#include "comm.h"
#include "list.h"

//初始化链表或者节点，使此节点的所有指针指向本身
void list_init(struct list *list)
{
	list->next = NULL;
	list->prev = NULL;
	list->parent = NULL;
	list->child = NULL;
}
/*
 void list_insert(struct list *head, struct list *new, char flag))
 {
 switch(flag)
 {
 case FLAG_AFTER:
 {
 new->next = head->next;
 new->prev = head;
 head->next->prev = new;
 head->next = new;
 break;
 case FLAG_BEFORE:
 new->prev = head->prev;
 new->next = head;
 head->prev->next = new;
 head->prev = new;
 break;
 default:
 break;
 }
 }
 }
 */

//在head和第一个节点之间加入一个节点
void list_add_head(struct list *head, struct list *add)
{
	head->next->prev = add;
	add->next = head->next;
	add->prev = head;
	head->next = add;
}

//在head的链表尾加入一个节点
void list_add_tail(struct list *head, struct list *add)
{
	while (head->next != NULL)
		head = head->next;
	head->next = add;
	add->prev = head;
}

//删除本节点
void list_del(struct list *entry)
{
	if (entry->next != NULL && entry->prev != NULL) {
		entry->prev->next = entry->next;
		entry->next->prev = entry->prev;
		list_init(entry);
	} else if (entry->next == NULL && entry->prev != NULL) {
		entry->prev->next = NULL;
		list_init(entry);
	}
}

//判断链表是否为空    返回： 空为1 非空为0
int list_empty(struct list *head)
{
	return head->next == NULL;
}

struct list *list_getnext(struct list *node)
{
	return node = node->next;
}

struct list *list_getprev(struct list *node)
{
	return node = node->prev;
}

struct list *list_getparent(struct list *node)
{
	return node = node->parent;
}
struct list *list_getchild(struct list *node)
{
	return node = node->child;
}
//获得最后一个节点
struct list *list_getlast(struct list *node)
{
	struct list *pos = node;
	while (pos->next != NULL)
		pos = pos->next;
	return pos;
}
//获得头节点
struct list *list_getfirst(struct list *node)
{
	struct list *pos = node;
	while (pos->prev != NULL)
		pos = pos->prev;
	return pos;
}
//打印链表
void list_print(struct list *node)
{
	struct list *pos = node;
	fprintf(stderr, "\n head=%x", (int) node);
	while (pos->next != NULL) {
		pos = pos->next;
		fprintf(stderr, "\n  pos=%x prev=%x next=%x", (int) pos,
				(int) pos->prev, (int) pos->next);
	}
	return;
}
//--------------------------------------
//在链表head中定位到node的前第num个节点，如果到达链表头则指向第一个节点
struct list *list_getPrevNumNode(struct list *head, struct list *node, int num)
{
	int i = 0;
	if (node == NULL)
		return NULL;
	while (node->prev != NULL) {
		i++;
		if (i > num)
			break;
		node = node->prev;
		if (node == head) {
			node = head->next;
			break;
		}
	}
	return node;
}
//在链表中定位到node的后第num个节点，如果到达链表尾则指向最后一个节点
struct list *list_getNextNumNode(struct list *head, struct list *node, int num)
{
	int i = 0;
	if (node == NULL)
		return NULL;
	while (node->next != NULL) {
		i++;
		if (i > num)
			break;
		node = node->next;
	}
	return node;
}
//获得链表节点总数(不包括子节点，只是兄弟节点)
int list_getListNum(struct list *head)
{
	int count = 0;
	if (head == NULL)
		return 0;
	while (head->next != NULL) {
		head = head->next;
		count++;
	}
	return count;
}

int list_getListIndex(struct list *head, struct list *node)
{
	struct list *pos = head;
	int index = 0;
	if (node == NULL)
		return 0;
	while (pos->next != NULL) {
		pos = pos->next;
		index++;
		if (pos == node)
			break;
	}
	return index;
}

//判断node在不在从start起num个节点之间 在第一个节点之前返回1 在中间返回2在第二个节点之后返回3
int listbetween(struct list *node, struct list *start, int num)
{
	int node_pos = 0, start_pos = 0;
	int ret = 0;
	node_pos = list_getListIndex(list_getfirst(node), node);
	start_pos = list_getListIndex(list_getfirst(node), start);
	if (node_pos < start_pos)
		ret = BEFORE;
	else if (node_pos > start_pos + num)
		ret = AFTER;
	else
		ret = MIDDLE;
	return ret;
}
