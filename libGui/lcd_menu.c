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
	//下一个菜单项的目录级别大于当前菜单项的目录级别则判定为此菜单项有子菜单，比如说当前菜单项的level是1，如果下一个菜单项的
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
			//如果是同一级别的菜单项则在链表尾端插入把这个节点加到链表中
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
				if(menunode!=NULL)
					free(menunode);
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
	set_time_show_flag(1);//TODO:new
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
	while(1)
	{
//		if(thread_run == PTHREAD_STOP)//如果收到终端信号
//			break;
		delay(100);
		if(g_LcdPoll_Flag==LCD_INPOLL)//如果处于轮选状态，则一直等待轮显结束
			return;
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
			case UP:
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
#ifdef HUBEI
							memset(passwd, '1', 6);
#else
							memset(passwd,'0',6);
#endif
#ifdef JIANGSU
						if(get_oprmode()==OPRMODE_MODIFY && msgbox_passwd_js(passwd, 6)==PASSWD_OK)//如果是更改模式并且密码设置正确
#else
						if(get_oprmode()==OPRMODE_MODIFY && msgbox_passwd(passwd, 6)==PASSWD_OK)//如果是更改模式并且密码设置正确
#endif
							passwd_flg = 1;//密码是否正确标致
						else if(get_oprmode()==OPRMODE_LOOK)//如果是查看模式
							passwd_flg = 1;
						else
							passwd_flg = 0;
					}
				}else if(menuitem->data.ispasswd == MENU_ISPASSWD){
					if(readcfg((char*)"/nor/config/lcd.cfg", (char*)"LCD_PASSWORD", passwd)==0)
#ifdef HUBEI
							memset(passwd, '1', 6);
#else
							memset(passwd,'0',6);
#endif
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
			set_time_show_flag(1);//TODO:new
			first_flg = 1;
		}
		PressKey = NOKEY;
		show_ctrl();//显示控制非客户区//TODO:new
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

#ifdef JIANGSU
//data_genus数据分类，实时，日冻结，月冻结
void pass_mpdata(INT8U flag,INT16U data_id,INT16U mq_cont,LcdDataItem *item)
{
	if(data_id == 3117)//正向有功电能示值
	{
		projno str_detal[6] = {{"kWh",2},{"正向有功总",117},{"正向有功尖",118},{"正向有功峰",119},{"正向有功平",120}
		,{"正向有功谷",121}};
		show_mpdata(1,6,item,str_detal);
	}
	else if(data_id == 3122)//正向无功电能示值
	{
		projno str_detal[6] = {{"kVArh",2},{"正向无功总",122},{"正向无功尖",123},{"正向无功峰",124},{"正向无功平",125},
				{"正向无功谷",126}};
		show_mpdata(1,6,item,str_detal);
	}
	else if(data_id == 3137)
	{
		projno str_detal[6] = {{"kWh",2},{"反向有功总",137},{"反向有功尖",138},{"反向有功峰",139},{"反向有功平",140},
				{"反向有功谷",141}};
		show_mpdata(1,6,item,str_detal);
	}
	else if(data_id == 3142)
	{
		projno str_detal[6] = {{"kVArh",2},{"反向无功总",142},{"反向无功尖",143},{"反向无功峰",144},{"反向无功平",145},
				{"反向无功谷",146}};
		show_mpdata(1,6,item,str_detal);
	}
	else if(data_id == 3127)
	{
		projno str_detal[6] = {{"kVArh",2},{"一象限无功总",127},{"一象限无功尖",128},{"一象限无功峰",129},{"一象限无功平",130},
				{"一象限无功谷",131}};
		show_mpdata(1,6,item,str_detal);
	}
	else if(data_id == 3147)
	{
		projno str_detal[6] = {{"kVArh",2},{"二象限无功总",147},{"二象限无功尖",148},{"二象限无功峰",149},{"二象限无功平",150},
				{"二象限无功谷",151}};
		show_mpdata(1,6,item,str_detal);
	}
	else if(data_id == 3152)
	{
		projno str_detal[6] = {{"kVArh",2},{"三象限无功总",152},{"三象限无功尖",153},{"三象限无功峰",154},{"三象限无功平",155},
				{"三象限无功谷",156}};
		show_mpdata(1,6,item,str_detal);
	}
	else if(data_id == 3132)
	{
		projno str_detal[6] = {{"kVArh",2},{"四象限无功总",132},{"四象限无功尖",133},{"四象限无功峰",134},{"四象限无功平",135}
		,{"四象限无功谷",136}};
		show_mpdata(1,6,item,str_detal);
	}
	else if(data_id == 3043)
	{
		projno str_detal[6] = {{"kVA",4},{" 视在功率总",43},{"视在功率A相",44},{"视在功率B相",45},{"视在功率C相",46}};
		show_mpdata(1,5,item,str_detal);
	}
	else if(data_id == 3036)
	{
		projno str_detal[6] = {{"V",1},{"电压A相    ",36},{"电压B相    ",37},{"电压C相    ",38}};
		show_mpdata(1,4,item,str_detal);
	}
	else if(data_id == 3039)
	{
		projno str_detal[6] = {{"A",3},{"电流A相    ",39},{"电流B相    ",40},{"电流C相    ",41}};
		show_mpdata(1,4,item,str_detal);
	}
	else if(data_id == 3024)
	{
		projno str_detal[6] = {{"kW",4},{" 有功功率总",24},{"有功功率A相",25},{"有功功率B相",26},{"有功功率C相",27}};
		show_mpdata(1,5,item,str_detal);
	}
	else if(data_id == 3028)
	{
		projno str_detal[6] = {{"kVar",4},{" 无功功率总",28},{"无功功率A相",29},{"无功功率B相",30},{"无功功率C相",31}};
		show_mpdata(1,5,item,str_detal);
	}
	else if(data_id == 3032)
	{
		projno str_detal[6] = {{" ",4},{" 功率因数总",32},{"功率因数A相",33},{"功率因数B相",34},{"功率因数C相",35}};
		show_mpdata(1,5,item,str_detal);
	}
//	else if(data_id == 3157)
//	{
//		projno str_detal[6] = {{"kW",0},{"正向需量总",157},{"正向需量尖",158},{"正向需量峰",159},{"正向需量平",160}
//		,{"正向需量谷",161}};
//		show_mpdata(1,5,item,str_detal);
//	}
//	else if(data_id == 3177)
//	{
//		projno str_detal[6] = {{"kW",0},{"反向需量总",177},{"反向需量尖",178},{"反向需量峰",179},{"反向需量平",180},
//				{"反向需量谷",181}};
//		show_mpdata(1,5,item,str_detal);
//	}
	else
	{
		fprintf(stderr,"无符合要求的数据项，请确认!!\n");
		return;
	}
}
void show_xuliang(INT8U data_cont,LcdDataItem *item,int did)//江苏实时数据需量显示
{
	Point pos;
	INT8U str[100];
	FP64 dval=0;
	char str_mid[100];
	char *dec_flag="% 10.4f";
	char *nodata=" xxxx.xxxx ";
	char str_unit[3]="kW";
	gui_clrrect(rect_Client);//清除客户区
	gui_setpos(&pos, rect_Client.left+9*FONTSIZE, rect_Client.top+1*FONTSIZE);
	setFontSize(16);
	gui_textshow((char *)"实时数据", pos, LCD_NOREV);
	setFontSize(12);
	memset(str, 0, 100);
	memset(str_mid,0,100);
	if(did == 157)
		sprintf(str_mid,"正向有功需量");
	if(did == 177)
		sprintf(str_mid,"反向有功需量");
	if(get_itemdata1(item, 10,did, &dval, 2)==1)
	{
		if(dval >= 0)
		{
		strcat(str_mid,dec_flag);
		strcat(str_mid,str_unit);//单位
		sprintf((char*)str,str_mid,dval);//str填充完成
		}
		else
		{
			strcat(str_mid,nodata);
			strcat(str_mid,str_unit);
			sprintf((char *)str,str_mid);
		}
	}
	else
	{
		strcat(str_mid,nodata);
		strcat(str_mid,str_unit);
		sprintf((char *)str,str_mid);
	}
	pos.x=rect_Client.left+FONTSIZE;
	pos.y += 6*FONTSIZE+3;
	gui_textshow((char*)str, pos, LCD_NOREV);
	memset(str, 0, 100);
	memset(str_mid,0,100);
	if((item[5].val[4]!= 00) && (item[5].val[4] != 0xef) && (item[5].val[4] != 0xee))
	{
		sprintf((char *)str,"发生时间  %02x/%02x %02x:%02x",item[5].val[3],item[5].val[2],item[5].val[1],item[5].val[0]);
	}
	else
		sprintf((char *)str,"发生时间  xx/xx xx:xx");
	pos.y += 3*FONTSIZE+3;
	gui_textshow((char*)str, pos, LCD_NOREV);
	return;
}
void show_mpdata(INT8U data_genus,INT8U data_cont,LcdDataItem *item,projno *str_name)
{
	Point pos;
	TmS curts;
	INT8U i;
	char str[100];
	char str_mid[100];
	INT8U dec_flag=0;
	char *nodata=" xxxx.xx ";
	char str_unit[6];
	FP64 dval=0;
	memset(str_unit,0,sizeof(char));
	#define OFFSET_Y 3
	gui_clrrect(rect_Client);//清除客户区
	gui_setpos(&pos, rect_Client.left+9*FONTSIZE, rect_Client.top+1*FONTSIZE);
	if(data_genus == 1){//实时数据
		setFontSize(16);
		gui_textshow((char *)"实时数据", pos, LCD_NOREV);
		setFontSize(12);
	}
	else
		return;
	if(data_cont>6) return;
	dec_flag = (INT8U)str_name->id_no;
	strcpy(str_unit,(char *)(str_name->str_data));
	str_name++;
	pos.x=rect_Client.left+2;
	pos.y += 3*FONTSIZE+OFFSET_Y;
//	//首先用strcpy复制过来汉字名称，再用strcat尾部添加数字，再尾部添加单位
	for(i=1;i<data_cont;i++)//从第二个开始
	{
		memset(str_mid,0,sizeof(INT8U));
		strcpy(str_mid,(char *)(str_name->str_data));
		if(get_itemdata1(item, data_cont,(int)(str_name->id_no), &dval, 2)==1)
		{
			if(dval >= 0)
			{
				if(dec_flag==1)
					strcat(str_mid,"% 9.1f");
				else if(dec_flag==2)
					strcat(str_mid,"% 9.2f");
				else if(dec_flag==3)
					strcat(str_mid,"% 9.3f");
				else if(dec_flag==4)
					strcat(str_mid,"% 9.4f");
				else
					strcat(str_mid,"% 9.2f");
				strcat(str_mid,str_unit);//单位
				sprintf((char*)str,str_mid,dval);//str填充完成
			}
			else
			{
				strcat(str_mid,nodata);
				strcat(str_mid,str_unit);
				sprintf((char *)str,str_mid);
			}
		}
		else
		{
			strcat(str_mid,nodata);
			strcat(str_mid,str_unit);
			sprintf((char *)str,str_mid);
		}
		gui_textshow((char*)str, pos, LCD_NOREV);
		pos.y += 2*FONTSIZE+OFFSET_Y;
		str_name++;
	}
	if(data_genus == 1)//实时数据显示抄表时间
	{
		tmget(&curts);
		memset(str, 0, 100);
		if(curts.Year==0|| curts.Month==0||curts.Day==0)
			sprintf((char*)str,"抄表时间 00/00/00 00:00");
		else
		sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
				curts.Year-2000, curts.Month, curts.Day, curts.Hour, curts.Minute);
		gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+19*FONTSIZE);
		gui_textshow((char*)str, pos, LCD_NOREV);
	}
	return;
}
void menu_jzqtime_js()
{
	time_t curr_time=time(NULL);
	struct tm curr_tm;
	char s_jzqtime[20],str[5],msgbox_ret=0,oprmode_old=0;
	//int s_len=14;
	memset(s_jzqtime, 0, 20);
	localtime_r(&curr_time, &curr_tm);
	sprintf(s_jzqtime, "%04d%02d%02d%02d%02d%02d", curr_tm.tm_year+1900,curr_tm.tm_mon+1,
					curr_tm.tm_mday, curr_tm.tm_hour, curr_tm.tm_min, curr_tm.tm_sec);
//	dbg_prt("\n s_jzqtime=%s %d", s_jzqtime, (int)curr_time);
	oprmode_old = get_oprmode();
	set_oprmode(OPRMODE_MODIFY);
	msgbox_ret = msgbox_jzqtimeset(s_jzqtime, strlen(s_jzqtime));
	set_oprmode(oprmode_old);
	memset(str, 0, 5);
	memcpy(str, &s_jzqtime[0], 4);
	curr_tm.tm_year = atoi(str) - 1900;
	memset(str, 0, 5);
	memcpy(str, &s_jzqtime[4], 2);
	curr_tm.tm_mon = atoi(str) - 1;
	memset(str, 0, 5);
	memcpy(str, &s_jzqtime[6], 2);
	curr_tm.tm_mday = atoi(str);
	memset(str, 0, 5);
	memcpy(str, &s_jzqtime[8], 2);
	curr_tm.tm_hour = atoi(str);
	memset(str, 0, 5);
	memcpy(str, &s_jzqtime[10], 2);
	curr_tm.tm_min = atoi(str);
	memset(str, 0, 5);
	memcpy(str, &s_jzqtime[12], 2);
	curr_tm.tm_sec = atoi(str);
	curr_time = mktime(&curr_tm);
	if(msgbox_ret==ACK){
//		dbg_prt( "\n set time %d", (int)curr_time);
		//g_JZQ_TimeSetUp_flg = 1;
		stime(&curr_time);
		system("hwclock -w");
		delay(1000);
		//g_JZQ_TimeSetUp_flg = 0;
	}
}
int msgbox_jzqtimeset(char *s_jzqtime, int len)//江苏专用
{
	char first_flg=0;
	char str[INPUTKEYNUM];
	char str_tmp[4];
	struct list *cur_node=NULL, *tmpnode=NULL;
	Form *cur_form=NULL, client;//client 液晶显示客户区
	Edit edit_year,edit_month,edit_day,edit_hour,edit_min,edit_sec;
	memset(&edit_year, 0, sizeof(Edit));
	memset(&edit_month, 0, sizeof(Edit));
	memset(&edit_day, 0, sizeof(Edit));
	memset(&edit_hour, 0, sizeof(Edit));
	memset(&edit_min, 0, sizeof(Edit));
	memset(&edit_sec, 0, sizeof(Edit));
	client.node.child = (struct list*)malloc(sizeof(struct list));
	if(client.node.child==NULL){
		g_curcldno = 1;//上状态栏显示
		return 0;
	}
	memset(client.node.child, 0, sizeof(struct list));
	Point pos;
	Rect rect;
	pos.x = rect_Client.left+FONTSIZE*5;
	pos.y = rect_Client.top + ROW_INTERVAL + 6*FONTSIZE;
	memset(str, 0, INPUTKEYNUM);
	memcpy(str,s_jzqtime,4);//取年
	edit_init(&edit_year, str, 4, pos, 0, 0, client.node.child,KEYBOARD_DEC);//年
	//------------------------------------------------------------
	pos.x += FONTSIZE*7;
	memset(str, 0, INPUTKEYNUM);
	memcpy(str,(s_jzqtime+4),2);//取月
	edit_init(&edit_month, str, 2, pos, 0, 0, client.node.child,KEYBOARD_DEC);//月
	//------------------------------------------------------------
	pos.x += FONTSIZE*5;
	memset(str, 0, INPUTKEYNUM);
	memcpy(str,(s_jzqtime+6),2);//取日
	edit_init(&edit_day, str, 2, pos, 0, 0, client.node.child,KEYBOARD_DEC);//日
	//------------------------------------------------------------
	pos.x = rect_Client.left+FONTSIZE*7;//小时滞后年2个字节
	pos.y += FONTSIZE*3 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	memcpy(str,(s_jzqtime+8),2);//取小时
	edit_init(&edit_hour, str, 2, pos, 0, 0, client.node.child,KEYBOARD_HEX);//小时
	//------------------------------------------------------------
	pos.x += FONTSIZE*5;
	memset(str, 0, INPUTKEYNUM);
	memcpy(str,(s_jzqtime+10),2);//取分钟
	edit_init(&edit_min, str, 2, pos, 0, 0, client.node.child,KEYBOARD_HEX);//分钟
	//------------------------------------------------------------
	pos.x += FONTSIZE*5;
	memset(str, 0, INPUTKEYNUM);
	memcpy(str,(s_jzqtime+12),2);//取秒
	edit_init(&edit_sec, str, 2, pos, 0, 0, client.node.child,KEYBOARD_HEX);//秒
	//------------------------------------------------------------
	cur_node = &edit_year.form.node;
	cur_form = &edit_year.form;
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		switch(PressKey)
		{
		case LEFT:
		case UP:
			cur_form->focus = NOFOCUS;
			cur_node=list_getprev(cur_node);
			if(cur_node==client.node.child)
				cur_node = list_getlast(client.node.child);
			break;
		case RIGHT:
		case DOWN:
			cur_form->focus = NOFOCUS;
			if(list_getnext(cur_node)==NULL){
				cur_node = list_getfirst(cur_node);
				cur_node = cur_node->next;
			}else
				cur_node = list_getnext(cur_node);
			break;
		case OK:
			if(get_oprmode()==OPRMODE_MODIFY){
				cur_form->pfun_process(cur_form);
				if(cur_form->pfun_process_ret==OK){
					if(msgbox_label((char *)"确定修改时钟?", CTRL_BUTTON_OK)==ACK){
						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_year,str);
						memset(str_tmp, 0, 4);
						eidt_gettext(&edit_month,str_tmp);
						strcat(str,str_tmp);
						memset(str_tmp, 0, 4);
						eidt_gettext(&edit_day,str_tmp);
						strcat(str,str_tmp);
						memset(str_tmp, 0, 4);
						eidt_gettext(&edit_hour,str_tmp);
						strcat(str,str_tmp);
						memset(str_tmp, 0, 4);
						eidt_gettext(&edit_min,str_tmp);
						strcat(str,str_tmp);
						memset(str_tmp, 0, 4);
						eidt_gettext(&edit_sec,str_tmp);
						strcat(str,str_tmp);
						memcpy(s_jzqtime,str,len);
						fprintf(stderr,"s_jzq = %s\n",s_jzqtime);
						fprintf(stderr,"str = %s\n",str);
						return ACK;
					}
				}
			}
			break;
		case ESC:
			if(client.node.child!=NULL)
				free(client.node.child);
			return 0;
		}
		if(PressKey!=NOKEY||first_flg==0){
			gui_clrrect(rect_Client);
			pos.x = rect_Client.left+FONTSIZE*6;
			pos.y = rect_Client.top + ROW_INTERVAL + FONTSIZE;
			setFontSize(16);
			gui_textshow((char *)"终端时钟设置", pos, LCD_NOREV);
			setFontSize(12);
			pos.x = rect_Client.left+FONTSIZE*9+ROW_INTERVAL;
			pos.y = rect_Client.top + ROW_INTERVAL + 6*FONTSIZE;
			gui_textshow((char *)"年", pos, LCD_NOREV);
			pos.x += FONTSIZE*5;
			gui_textshow((char *)"月", pos, LCD_NOREV);
			pos.x += FONTSIZE*5;
			gui_textshow((char *)"日", pos, LCD_NOREV);
			pos.x = rect_Client.left+FONTSIZE*9 + ROW_INTERVAL;
			pos.y += FONTSIZE*3 + ROW_INTERVAL;
			gui_textshow((char *)"时", pos, LCD_NOREV);
			pos.x += FONTSIZE*5;
			gui_textshow((char *)"分", pos, LCD_NOREV);
			pos.x += FONTSIZE*5;
			gui_textshow((char *)"秒", pos, LCD_NOREV);

			tmpnode = client.node.child;
			while(tmpnode->next!=NULL){
				tmpnode = tmpnode->next;
				cur_form = list_entry(tmpnode, Form, node);
				cur_form->pfun_show(cur_form);
			}
			cur_form = list_entry(cur_node, Form, node);//根据链表节点找到控件指针
			cur_form->focus = FOCUS;
			memcpy(&rect, &cur_form->rect, sizeof(Rect));
			rect.left += 2;
			rect.right -= 3;
			rect.top -= 1;
			gui_rectangle(gui_changerect(gui_moverect(rect, DOWN, 4), 4));
			first_flg = 1;
		}
		PressKey = NOKEY;
		delay(100);
	}
	if(client.node.child!=NULL)
		free(client.node.child);
	return 0;
}
#endif
