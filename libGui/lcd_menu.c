/*
 * menu.c  显示菜单
 * */
#include "lcd_menu.h"
#include "gui.h"
#include "lcd_ctrl.h"
#include "lcdprt.h"
#include "../include/ParaDef.h"
#include "show_ctrl.h"
extern char g_LcdPoll_Keypress;

int my_min(int a, int b){
	if(a<b)
		return a;
	else 
		return b;
}
//根据item_id获得menu.pthis 即这个menuitem所对应的链表节点（struct list）地址
void *GetMenuItembyID(Menu* menu,  int menu_count, int item_id)
{
	if(menu==NULL) return NULL;
	return menu[item_id].pthis;
}
MenuList *getMenuListItembyList(struct list* list_node)
{
	MenuList *pitem=NULL;
	pitem = list_entry(list_node, MenuList, node);//通过结构体的node元素来获取结构体的地址
	return pitem;
}
//根据menu菜单数组生成一个菜单链表节点， 同时把节点的node地址传给menu->pthis
//以便以后根据menu数组就可以查找到菜单链表节点地址
MenuList *MakeMenuListItem(Menu *menu)//(ElemType *pdata)
{
	MenuList *pitem=NULL;
	if(menu==NULL)	return NULL;
	pitem =(MenuList *)malloc(sizeof(MenuList));
	if(pitem!=NULL){
		memset(pitem, 0, sizeof(MenuList));
		memcpy(&pitem->data, &menu->data, sizeof(ElemType)); 
		//dbg_prt("\n &pitem->node==%x",(unsigned int)&pitem->node);
		menu->pthis = &pitem->node;
	}
	return pitem;
}
//判断menu是否是一个目录 currmenu_index第几个菜单项 从0开始记
int isCata(Menu *menu, int menu_count, int currmenu_index){
	int iscata=0;//1 是目录 0 不是
	if(currmenu_index+1>=menu_count)
		return iscata;
	//下一个菜单项的目录级别大于当前菜单项的目录级别则判定为此菜单项有子菜单，
	//比如说当前菜单项的level是1，如果下一个菜单项的
	//level是2，那么则说明该菜单项包含子菜单项
	if(menu[currmenu_index+1].data.level>menu[currmenu_index].data.level)
		iscata = 1;
	return iscata;
}

struct list* menu_getparent(Menu *menu, int menu_count, int currmenu_index){
	int i, currmenu_level=menu[currmenu_index].data.level;//获取当前菜单级别
	struct list *list_parent=NULL;
	if(currmenu_index<=0)
		return NULL;
	for(i=currmenu_index; i>=0; i--){//往前找，找到第一个级别比它小的就是它的父节点
		if(menu[i].data.level<currmenu_level){
			list_parent = (struct list*)GetMenuItembyID(menu, menu_count, i);
			break;
		}
	}
	return list_parent;
}
//根据菜单数组（一维数组）组合成一个菜单链表（多维） 程序退出释放内存
MenuList *ComposeDList(Menu *menu,  int menu_count)
{
	int i=0, level;
	struct list *list_parent=NULL;
	MenuList *pitem=NULL, *pmenulist_head=NULL;
//	fprintf(stderr,"\n-----\n");
	DEBUG_TIME_LINE("menu_count= %d", menu_count);
	for(level=0; level<=level6; level++){
		for(i=0; i<menu_count; i++)
		{
			if(menu[i].data.level!=level)
				continue;
			pitem = MakeMenuListItem(&menu[i]);//生成一个节点

			if(pitem==NULL)
				return NULL;
			list_parent = (struct list*)menu_getparent(menu, menu_count, i);//获得链表中父节点指针
			if(pitem->node.child==NULL && isCata(menu, menu_count, i)==1){//含有子菜单项的菜单项第一次初始化
				pitem->node.child = (struct list *)malloc(sizeof(struct list));
				list_init(pitem->node.child);
				if(list_parent==NULL){//顶层菜单的头节点  只有顶层菜单的头节点没有父节点
//					fprintf(stderr,"\nhead0=0x%x", (unsigned int)pitem);
					pmenulist_head = pitem;
					continue;
				}
			}
			//如果是同一级别的菜单项则在链表尾端插入这个节点
			list_add_tail(list_parent->child, &pitem->node);
			pitem->node.parent = list_parent;
//			fprintf(stderr,"\n Level %d: [%d]name=%s node=0x%x prev=0x%x list_parent=0x%x pitem->node.child=0x%x",
//					level, i, pitem->data.name,
//					(unsigned int)&pitem->node,
//					(unsigned int)pitem->node.prev,
//					(unsigned int)list_parent,
//					(unsigned int)pitem->node.child);
		}
	}
	return pmenulist_head;
}
//每次搜索只删除一个节点的子菜单
void deletemenu(MenuList *pmenulist_head){
	struct list *head=NULL, *node=NULL, *tmpnode=NULL, *parent=NULL;
	MenuList *menunode=NULL;
	head = pmenulist_head->node.child;
	char child_flg=0;
	while(1){
		node = head;
		if(pmenulist_head->node.child==NULL){
			if(pmenulist_head!=NULL){
				//dbg_prt("\n del head0");
				free(pmenulist_head);
				pmenulist_head = NULL;
			}
			//dbg_prt("\n pmenulist_head->node.child==NULL");
			break;
		}
		child_flg = 0;
		while(node->next!=NULL){//在链表中找子菜单找到就进去
			node = node->next;
			if(node->child!=NULL){
				child_flg = 1;
				head = node->child;
			}
		}
		if(child_flg==0){//在链表中没有子菜单就删掉所有的菜单项
			node = list_getlast(node);
			do{
				parent = node->parent;
				menunode = getMenuListItembyList(node);
				//dbg_prt( "\n  del node=%x", (int)node);
				tmpnode = node->prev;
				list_del(node);
				if(menunode!=NULL) {
					free(menunode);
					menunode = NULL;
				}
				node = tmpnode;
			}while(node->prev!=NULL);
			// 释放内存
			if(head!=NULL){
				//dbg_prt( "\ndel head=%x", (int)head);
				free(head);
				head = NULL;
				parent->child = NULL;
			}
			head = pmenulist_head->node.child;//重新赋值 进行一轮搜索
		}
	}
	return;
}
//初始化菜单
void initmenu()
{
	rect_TopStatus.left = 0;
	rect_TopStatus.top  = 0;
	rect_TopStatus.right = LCM_X;
	rect_TopStatus.bottom = FONTSIZE_8*2;
	
	rect_Client.left = 0;
	rect_Client.top  = FONTSIZE_8*2 + 1;
	rect_Client.right = LCM_X;
	rect_Client.bottom = LCM_Y - FONTSIZE_8*2 - 1;

	rect_BottomStatus.left = 0;
	rect_BottomStatus.top  = LCM_Y - FONTSIZE_8*2;
	rect_BottomStatus.right = LCM_X;
	rect_BottomStatus.bottom = LCM_Y;
}

void ShowMenuItem(Point pos, MenuList *menuitem)
{
	gui_textshow(menuitem->data.name, pos, LCD_NOREV);
}
//显示 启始位置begin，显示几个，当前位置current
void ShowMenuItems(struct list *begin, int count, struct list *current)
{
	int i=0, fontsize=12;
	MenuList *menuitem;
	Point pos, cur_pos;
	char name[50];
	fontsize = getFontSize();
	setFontSize(16);
	memset(&cur_pos, 0, sizeof(Point));
	menuitem = getMenuListItembyList(list_getparent(begin));//获得父菜单项结构体指针
	memset(name, 0, 50);
	if(menuitem->data.name[0]<0x80)//将菜单项名称作为子菜单项的标题
		memcpy(name, &menuitem->data.name[2], strlen(menuitem->data.name)-2);
	else
		memcpy(name, &menuitem->data.name[0], strlen(menuitem->data.name));
	gui_setpos(&pos, (LCM_X-strlen(name)*8)/2, rect_Client.top+4);
	gui_textshow(name, pos, LCD_NOREV);//显示标题

	menuitem = getMenuListItembyList(begin);//获取当前需要显示的菜单链表的第一个菜单项指针
	pos.x = (LCM_X-strlen(menuitem->data.name)*8)/2;
	for(i=0; i<count; i++)
	{//从begin开始显示5个菜单项
		pos.y = rect_Client.top + i * 2 * 9 + i + 28;
		menuitem = getMenuListItembyList(begin);
		ShowMenuItem(pos, menuitem);
		if(begin == current)
			memcpy(&cur_pos, &pos, sizeof(Point));
		begin = list_getnext(begin);
	}
	menuitem = getMenuListItembyList(current);
	gui_textshow(menuitem->data.name, cur_pos, LCD_REV);
	setFontSize(fontsize);
}

//显示
//参数head为链表的头指针
void ShowItself(struct list *head)
{
	char oprmode=0, oprmode_bak=0;//msgbox的两种模式查看模式/更改模式
	MenuStat_t tmenustat;//保存父菜单链表状态
	struct list *begin=head->next;//一屏的第一个菜单项
	struct list *current=head->next;//一屏中当前选中的菜单项
	struct list *tophead = head;//tophead为堆栈的栈顶指针
	struct list *current_oprmode=NULL;
	int pageitem_count=0;
	MenuList *menuitem;
	char first_flg=0, passwd_flg=0;//密码标识
	char passwd[6];//
	g_curcldno = 1;
	while(1) {

		if(g_LcdPoll_Flag==LCD_INPOLL)//如果处于轮选状态，则一直等待轮显结束
		{
			delay(100);
			return;
		}
		if(g_LcdPoll_Keypress==1)
		{//在轮显模式下进入正常模式 菜单显示顶层菜单 同时当前按键只能算是唤醒液晶
			head = tophead;
			begin=tophead->next;
			current=tophead->next;
			first_flg = 0;
			g_LcdPoll_Keypress=0;
		}else{
			switch(PressKey)
			{
			case LEFT:
			//	DEBUG_TIME_LINE("PressKey LEFT\n");
				break;
			case RIGHT:
			//	DEBUG_TIME_LINE("PressKey RIGHT\n");
				break;
			case UP:
			//	DEBUG_TIME_LINE("PressKey UP\n");
				if(pageitem_count>0){
					current=list_getprev(current);
					if(current==head){//如果当前菜单项是菜单项链表的第一个菜单项
						current = list_getlast(head);//获取菜单项链表的最后一个菜单项指针
						begin = list_getPrevNumNode(head, current, pageitem_count-1);
					}else{
						if(listbetween(current, begin, pageitem_count-1)==BEFORE)
							begin = begin->prev;
					}
				}
				break;
			case DOWN:
			//	DEBUG_TIME_LINE("PressKey DOWN\n");
				if(pageitem_count>0){
					current = list_getnext(current);
					if(current==NULL)
						begin = current = head->next;
					else{
						if(listbetween(current, begin, pageitem_count-1)==AFTER)
							begin = begin->next;
					}
				}
				break;
			case OK:
				//oprmode_bak = get_oprmode();
			//	DEBUG_TIME_LINE("PressKey OK\n");
				menuitem = getMenuListItembyList(current);//获取当前菜单项信息，根据获取的信息判断其是否有密码和子菜单项
				if(menuitem->data.ispasswd == MENU_ISPASSWD_EDITMODE)
				{//如果菜单项需要输入密码
					oprmode = msgbox_oprmode(CTRL_BUTTON_OK);
					if(oprmode!=0){
						oprmode_bak = get_oprmode();
						set_oprmode(oprmode);
						current_oprmode = current;
//						dbg_prt("\n mode=%d", get_oprmode());
						if(readcfg((char*)"/nor/config/lcd.cfg", (char*)"LCD_PASSWORD", passwd)==0)
						memset(passwd,'0',6);
						if(get_oprmode()==OPRMODE_MODIFY && msgbox_passwd(passwd, 6)==PASSWD_OK)//如果是更改模式并且密码设置正确
							passwd_flg = 1;//密码是否正确标致
						else if(get_oprmode()==OPRMODE_LOOK)//如果是查看模式
							passwd_flg = 1;
						else
							passwd_flg = 0;
					}
				}else if(menuitem->data.ispasswd == MENU_ISPASSWD){
					if(readcfg((char*)"/nor/config/lcd.cfg", (char*)"LCD_PASSWORD", passwd)==0)
					memset(passwd,'0',6);
					if(msgbox_passwd(passwd, 6)==PASSWD_OK)//如果是更改模式并且密码设置正确
						passwd_flg = 1;//密码是否正确标致
					else
						passwd_flg = 0;
				}else
					passwd_flg = 1;
				if(passwd_flg == 1)
				{//密码正确
					if(current->child!=NULL)
					{//有子菜单
						DEBUG_TIME_LINE("has child");
						tmenustat.begin = begin;
						tmenustat.current = current;
						tmenustat.head = head;
						push(&tmenustat);
						head = current->child;
						current = begin = head->next;
					}else{
//						dbg_prt("\n mode=%d------", get_oprmode());
						if(menuitem->data.fun!=NULL)
							menuitem->data.fun();
					}
					passwd_flg = 0;
				}
				//set_oprmode(oprmode_bak);
				break;
			case ESC:
		//		DEBUG_TIME_LINE("PressKey ESC\n");
				memset(&tmenustat, 0, sizeof(MenuStat_t));
				if(top(&tmenustat))
				{//利用堆栈来存储菜单显示的页面
					begin = tmenustat.begin;
					current = tmenustat.current;
					head = tmenustat.head;
					pop();//弹出父菜单链表状态
				}
				if(current==current_oprmode){//TODO 测试
					set_oprmode(oprmode_bak);
					oprmode_bak = 0;
					current_oprmode = NULL;
				}
				break;
			default:
				break;
			}
		}
		if(PressKey!=NOKEY || first_flg==0)
		{
			fprintf(stderr,"\n有按键触发\n");
			gui_clrrect(rect_Client);
			/*
			 * 一屏显示5个菜单项
			 * 如果一个菜单项链表的菜单项多于5个则只显示其中的5个；如果少于5个则全部显示
			 * */
			pageitem_count = my_min(list_getListNum(head),MENU_PAGEITEM_COUNT-1);
			ShowMenuItems(begin, pageitem_count, current);
			first_flg = 1;
		}
		PressKey = NOKEY;
		show_ctrl();//显示控制非客户区//TODO:new
		delay(100);
	}
}
//TODO:获取测量点端口号
INT8U getmpport(INT16U index)
{
	INT16U i;
//	for(i=0;i<MP_MAXNUM;i++)
//	{
//		if (ParaAll->f10.para_mp[i].Index == index)
//			return ParaAll->f10.para_mp[i].Port;
//	}
	return 0;
}
