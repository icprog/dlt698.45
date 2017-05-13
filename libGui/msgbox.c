/*
 * msgbox.c
 *
 *  Created on: 2013-6-8
 *      Author: yd
 */
#include "lcd_ctrl.h"
#include "gui.h"
#include "lcdprt.h"
////pos左上角坐标   len 对话框长度   width 对话框宽度
//void msgbox_init(Form *box, Point pos, int len, int width)
//{
//	box->pos.x = pos.x;
//	box->pos.y = pos.y;
//	box->rect = getrect(pos, len, width);
//}

void msgbox_showtext(Text_t *ttext){
	int i;
	for(i=0; i<MSGBOX_TEXTSIZE; i++)
	{
		if(ttext[i].pos.x==0 && ttext[i].pos.y==0)
			continue;
		if(ttext[i].text==NULL)
			continue;
		gui_textshow((char*)ttext[i].text, ttext[i].pos, LCD_NOREV);
	}
}
//对话框 form：控件链表指针  form_size:有几个控件 ttext:显示的控件标签  type:对话框的类型（编辑框或者显示） msgbox_ret[INPUTKEYNUM] 对话框返回值 OK或CANCEL
void msgbox(Rect form_rect, struct list *head, Text_t *ttext, MsgBoxRet *msgbox_ret)
{
	int j=0, index=0;
	INT8U lcd_buf[LCM_X*LCM_Y];
	char first_flg=0;//str[INPUTKEYNUM]; //str编辑框初始的内容
	Rect rect;
	struct list *cur_node=NULL, *tmpnode=NULL;
	Form msgbox, *cur_form=NULL, *tmpform=NULL;
	memcpy(&msgbox.rect, &form_rect, sizeof(Rect));
	msgbox.node.child = head;

	memset(lcd_buf, 0, LCM_X*LCM_Y);
	lcdregion_save(lcd_buf, msgbox.rect);
	gui_clrrect(msgbox.rect);
	//初始化所有控件
	tmpnode = head;
	while(tmpnode->next!=NULL){
		tmpnode = tmpnode->next;
		tmpform = list_entry(tmpnode, Form, node);
		if(tmpform->focus == FOCUS){
			cur_form = tmpform;
			cur_node = &tmpform->node;//msgbox默认焦点控件
		}
	}
//	list_print(msgbox.node.child);
	set_time_show_flag(1);
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		switch(PressKey)
		{
		case LEFT:
		case UP:
			cur_form->focus = NOFOCUS;
			cur_node=list_getprev(cur_node);
			if(cur_node==msgbox.node.child)
				cur_node = list_getlast(msgbox.node.child);
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
		case OK://处理返回值
//			dbg_prt("\n  msgbox mode=%d", get_oprmode());
			cur_form = list_entry(cur_node, Form, node);
			if(cur_form->CtlType==CTRL_BUTTON_OK||
					cur_form->CtlType==CTRL_BUTTON_CANCEL){
				cur_form->pfun_process(cur_form);
				msgbox_ret->btn_ret = cur_form->key[0].c;
				if(cur_form->CtlType==CTRL_BUTTON_OK){//确定
					index = 0;
					tmpnode = head;
					while(tmpnode->next!=NULL){
						tmpnode = tmpnode->next;
						tmpform = list_entry(tmpnode, Form, node);
						j=0;
						if(tmpform->ret_flg==1){
							while(j<tmpform->c_num){
								msgbox_ret->s_ret[index++] = tmpform->key[j].c;
								j++;
							}
						}
					}
					lcdregion_restore(lcd_buf, msgbox.rect);
					set_time_show_flag(1);
					return;
				}else if(cur_form->CtlType==CTRL_BUTTON_CANCEL){//取消
					lcdregion_restore(lcd_buf, msgbox.rect);
					set_time_show_flag(1);
					return;
				}
			}else if(get_oprmode()==OPRMODE_MODIFY){
				cur_form->pfun_process(cur_form);
				msgbox_ret->btn_ret = cur_form->key[0].c;
				if(cur_form->pfun_process_ret==OK){
					tmpform = NULL;
					tmpnode = head;
					while(tmpnode->next!=NULL){
						tmpnode = tmpnode->next;
						tmpform = list_entry(tmpnode, Form, node);
						if(tmpform->CtlType==CTRL_BUTTON_OK){
							cur_form->focus = NOFOCUS;
							cur_form = tmpform;
							cur_node = &tmpform->node;
						}
					}
				}else if(cur_form->pfun_process_ret==ESC){
					tmpform = NULL;
					tmpnode = head;
					while(tmpnode->next!=NULL){
						tmpnode = tmpnode->next;
						tmpform = list_entry(tmpnode, Form, node);
						if(tmpform->CtlType==CTRL_BUTTON_CANCEL){
							cur_form->focus = NOFOCUS;
							cur_form = tmpform;
							cur_node = &tmpform->node;
						}
					}
				}
			}
			break;
		case ESC:
			msgbox_ret->btn_ret = 0;
			return;
		}
		if(PressKey!=NOKEY || first_flg==0){
			gui_clrrect(msgbox.rect);
			gui_rectangle(gui_changerect(msgbox.rect, -1));
			gui_rectangle(gui_changerect(msgbox.rect, -2));
			tmpnode = msgbox.node.child;
//			list_print(msgbox.node.child);
			while(tmpnode->next!=NULL){
				tmpnode = tmpnode->next;
				tmpform = list_entry(tmpnode, Form, node);
				tmpform->pfun_show(tmpform);
			}
			if(ttext!=NULL)
				msgbox_showtext(ttext);//显示text
			//显示当前控件
			cur_form = list_entry(cur_node, Form, node);//根据链表节点找到控件指针
			cur_form->focus = FOCUS;
			memcpy(&rect, &cur_form->rect, sizeof(Rect));
			//选中该控件时微调各个控件的位置
			if(cur_form->CtlType==CTRL_EDIT){
				rect.top += 2;
				rect.bottom +=4;
				gui_rectangle(gui_changerect(rect, 3));
			}else if(cur_form->CtlType==CTRL_BUTTON_OK||cur_form->CtlType==CTRL_BUTTON_CANCEL){
				gui_reverserect(gui_changerect(rect, 2));//反显按钮
				gui_rectangle(gui_changerect(rect, 2));
			}else  if(cur_form->CtlType==CTRL_COMBOX){
				gui_rectangle(gui_changerect(gui_moverect(rect, DOWN, 4), 6));
			}else if(cur_form->CtlType==CTRL_EDITTIME){
				rect.bottom += 7;
				rect.left += 1;
				rect.right -= 1;
				gui_rectangle(gui_changerect(rect, 2));
			}else if(cur_form->CtlType==CTRL_EDITIP){
				rect.top += 3;
				rect.bottom +=4;
				gui_rectangle(gui_changerect(rect, 4));
			}
			set_time_show_flag(1);
//			dbg_prt("\n------PressKey=%d------------4",PressKey);
			first_flg = 1;
		}
		PressKey = NOKEY;
		delay(50);
	}
}

//passwd 要比对的密码   len 密码长度
int msgbox_passwd(char *passwd, int len) {
	int passwd_ok=0;
//---------------------------
	Rect msgbox_rect;
	MsgBoxRet msgbox_ret;
	struct list *head=NULL;
	char str[10];
	Point msgbox_pos;
	memset(str,'0',10);
	Edit edit1;
	Button btn_ok, btn_esc;
	memset(&edit1, 0, sizeof(Edit));
	memset(&btn_ok, 0, sizeof(Button));
	memset(&btn_esc, 0, sizeof(Button));
	memset(&msgbox_ret, 0, sizeof(MsgBoxRet));
	gui_setpos(&msgbox_pos, (LCM_X-13*FONTSIZE)/2, (LCM_Y-10*FONTSIZE)/2);
	head = (struct list*)malloc(sizeof(struct list));
	if(head==NULL) 	return 0;
	list_init(head);
	Point editctrl_pos = {msgbox_pos.x+FONTSIZE*4-4, msgbox_pos.y+2*FONTSIZE};  //位置
	edit_init(&edit1, str, 6, editctrl_pos, FRAME, RETFLG, head,KEYBOARD_ASC);	//控件初始化
	Point button_ok_pos = {msgbox_pos.x+1*FONTSIZE, msgbox_pos.y+5*FONTSIZE};
	button_init(&btn_ok,(char *)"确定", button_ok_pos, CTRL_BUTTON_OK, head);
	Point button_esc_pos = {msgbox_pos.x+8*FONTSIZE, msgbox_pos.y+FONTSIZE*5};
	button_init(&btn_esc,(char *)"取消", button_esc_pos, CTRL_BUTTON_CANCEL, head);
	btn_ok.form.focus = FOCUS;
	gui_setrect(&msgbox_rect, msgbox_pos.x, msgbox_pos.y, msgbox_pos.x+13*FONTSIZE, msgbox_pos.y+8*FONTSIZE);
	msgbox(msgbox_rect, head, NULL, &msgbox_ret);
	if(head!=NULL)
		free(head);
//------------------------------------------------
	if(len>INPUTKEYNUM)
		return PASSWD_ERR;
	if(msgbox_ret.btn_ret==ACK){
		if(memcmp(passwd, msgbox_ret.s_ret, len)==0)
			passwd_ok = PASSWD_OK;//密码正确
		else
			passwd_ok = PASSWD_ERR;//密码错误
	}else
		passwd_ok = PASSWD_ERR;//密码错误
	return passwd_ok;
}
int msgbox_setpasswd(char *passwd, int len, char *s_passwd_ret)
{
	Text_t ttext[MSGBOX_TEXTSIZE];
	Rect msgbox_rect;
	MsgBoxRet msgbox_ret;
	struct list *head=NULL;
	char str[10];
	Point msgbox_pos;
	memset(str, 0, 10);
	gui_setpos(&msgbox_pos, (LCM_X-17*FONTSIZE)/2, (LCM_Y-14*FONTSIZE)/2);
	Edit edit1, edit2;
	Button btn_ok, btn_esc;
	memset(&edit1, 0, sizeof(Edit));
	memset(&edit2, 0, sizeof(Edit));
	memset(&btn_ok, 0, sizeof(Button));
	memset(&btn_esc, 0, sizeof(Button));
	memset(&msgbox_ret, 0, sizeof(MsgBoxRet));
	head = (struct list*)malloc(sizeof(struct list));
	if(head==NULL) 	return 0;
	list_init(head);
	Point editctrl_oldpasswd_pos = {msgbox_pos.x+FONTSIZE*9, msgbox_pos.y+1*FONTSIZE};  //位置
	edit_init(&edit1, str, 6, editctrl_oldpasswd_pos, FRAME, RETFLG, head,KEYBOARD_ASC);	//控件初始化
	Point editctrl_pos = {msgbox_pos.x+FONTSIZE*9, msgbox_pos.y+4*FONTSIZE};  //位置
	edit_init(&edit2, str, 6, editctrl_pos, FRAME, RETFLG, head,KEYBOARD_ASC);	//控件初始化
	Point button_ok_pos = {msgbox_pos.x+3*FONTSIZE, msgbox_pos.y+7*FONTSIZE};
	button_init(&btn_ok,(char *)"确定", button_ok_pos, CTRL_BUTTON_OK, head);
	Point button_esc_pos = {msgbox_pos.x+10*FONTSIZE, msgbox_pos.y+FONTSIZE*7};
	button_init(&btn_esc,(char *)"取消", button_esc_pos, CTRL_BUTTON_CANCEL, head);
	btn_esc.form.focus = FOCUS;

	memset(ttext, 0, MSGBOX_TEXTSIZE*sizeof(Text_t));
	gui_setpos(&ttext[0].pos, msgbox_pos.x+FONTSIZE, msgbox_pos.y+FONTSIZE);
	memcpy(ttext[0].text, (char*)"旧密码:", strlen("旧密码:"));
	gui_setpos(&ttext[1].pos, msgbox_pos.x+FONTSIZE, msgbox_pos.y+4*FONTSIZE);
	memcpy(ttext[1].text, (char*)"新密码:", strlen("新密码:"));
	gui_setrect(&msgbox_rect, msgbox_pos.x, msgbox_pos.y, msgbox_pos.x+17*FONTSIZE, msgbox_pos.y+10*FONTSIZE);
	msgbox(msgbox_rect, head, ttext, &msgbox_ret);
	if(head!=NULL)
		free(head);
	//------------------------------------------------
	int passwd_ok=0;
	if(msgbox_ret.btn_ret==ACK){
		if(memcmp(passwd, msgbox_ret.s_ret, len)==0){
			passwd_ok = PASSWD_OK;//密码正确
			memcpy(s_passwd_ret, &msgbox_ret.s_ret[len], len);//返回新密码
		}else
			passwd_ok = PASSWD_ERR;//密码错误
	}else
		passwd_ok = PASSWD_ESC;//取消设置
//	dbg_prt("\n ret passwd=%d %d %d %d %d %d ",
//			msgbox_ret.s_ret[6],msgbox_ret.s_ret[7],msgbox_ret.s_ret[8],
//			msgbox_ret.s_ret[9],msgbox_ret.s_ret[10],msgbox_ret.s_ret[11]);
	return passwd_ok;
}

int msgbox_label(char *text, int ctrl_focus){
	int offset_x=0, text_len=0;
	Text_t ttext[MSGBOX_TEXTSIZE];
	Rect msgbox_rect;
	MsgBoxRet msgbox_ret;
	struct list *head=NULL;
	Point msgbox_pos;
	Button btn_ok, btn_esc;
	memset(&btn_ok, 0, sizeof(Button));
	memset(&btn_esc, 0, sizeof(Button));
	memset(&msgbox_ret, 0, sizeof(MsgBoxRet));
	gui_setpos(&msgbox_pos, (LCM_X-13*FONTSIZE)/2, (LCM_Y-10*FONTSIZE)/2);
	head = (struct list*)malloc(sizeof(struct list));
	if(head==NULL) 	return 0;
	text_len = strlen(text);
	if(text_len>=12){
		offset_x = text_len - 12;
	}
	list_init(head);
	Point button_ok_pos = {msgbox_pos.x + FONTSIZE + offset_x/2*FONTSIZE, msgbox_pos.y+5*FONTSIZE};
	button_init(&btn_ok,(char *)"确定", button_ok_pos, CTRL_BUTTON_OK, head);
	Point button_esc_pos = {msgbox_pos.x+8*FONTSIZE+offset_x/2*FONTSIZE, msgbox_pos.y+5*FONTSIZE};
	button_init(&btn_esc,(char *)"取消", button_esc_pos, CTRL_BUTTON_CANCEL, head);
	if(ctrl_focus==CTRL_BUTTON_OK)
		btn_ok.form.focus = FOCUS;
	else
		btn_esc.form.focus = FOCUS;
	memset(ttext, 0, MSGBOX_TEXTSIZE*sizeof(Text_t));
	gui_setpos(&ttext[0].pos, msgbox_pos.x+FONTSIZE, msgbox_pos.y+FONTSIZE+4);
	//memcpy(ttext[0].text, (char*)"无此测量点!", strlen("无此测量点!"));
	memcpy(ttext[0].text, text, strlen(text));
	gui_setrect(&msgbox_rect, msgbox_pos.x, msgbox_pos.y, msgbox_pos.x+14*FONTSIZE+offset_x*FONTSIZE, msgbox_pos.y+8*FONTSIZE);
	msgbox(msgbox_rect, head, ttext, &msgbox_ret);
	if(head!=NULL)
		free(head);
	return msgbox_ret.btn_ret;
}
//显示当前的485-2规约，定义一个功能切换按钮，按下按钮之后进行功能切换并实时刷新
int menu_485func_show(char *text,INT8U ctrl_focus)
{
	int offset_x=0, text_len=0;
	Text_t ttext[MSGBOX_TEXTSIZE];
	Rect msgbox_rect;
	MsgBoxRet msgbox_ret;
	struct list *head=NULL;
	Point msgbox_pos;
	Button btn_switch;
	memset(&btn_switch, 0, sizeof(Button));
	memset(&msgbox_ret, 0, sizeof(MsgBoxRet));
	gui_setpos(&msgbox_pos, (LCM_X-13*FONTSIZE)/2, (LCM_Y-10*FONTSIZE)/2);
	head = (struct list*)malloc(sizeof(struct list));
	if(head==NULL) 	return 0;
	text_len = strlen(text);
	if(text_len>=12){
		offset_x = text_len - 12;
	}
	list_init(head);
	Point button_ok_pos = {msgbox_pos.x + FONTSIZE + offset_x/2*FONTSIZE + 20, msgbox_pos.y+5*FONTSIZE};
	button_init(&btn_switch,(char *)"切换", button_ok_pos, CTRL_BUTTON_OK, head);

	if(ctrl_focus==CTRL_BUTTON_OK)
		btn_switch.form.focus = FOCUS;

	memset(ttext, 0, MSGBOX_TEXTSIZE*sizeof(Text_t));
	gui_setpos(&ttext[0].pos, msgbox_pos.x+FONTSIZE, msgbox_pos.y+FONTSIZE+4);

	memcpy(ttext[0].text, text, strlen(text));
	gui_setrect(&msgbox_rect, msgbox_pos.x, msgbox_pos.y, msgbox_pos.x+14*FONTSIZE+offset_x*FONTSIZE, msgbox_pos.y+8*FONTSIZE);
	msgbox(msgbox_rect, head, ttext, &msgbox_ret);//为什么按下确定键之后对话框消失了
	if(head!=NULL)
		free(head);
	return msgbox_ret.btn_ret;
}
//通过测量点号抄表
int msgbox_inputcldno(){
	Rect msgbox_rect;
	int cldno=0;
	MsgBoxRet msgbox_ret;
	struct list *head=NULL;
	char str[10],oprmode_old;
	Point msgbox_pos;
	memset(str, 0, 10);
	Edit edit1;
	Button btn_ok, btn_esc;
	memset(&edit1, 0, sizeof(Edit));
	memset(&btn_ok, 0, sizeof(Button));
	memset(&btn_esc, 0, sizeof(Button));
	memset(&msgbox_ret, 0, sizeof(MsgBoxRet));
	gui_setpos(&msgbox_pos, (LCM_X-13*FONTSIZE)/2, (LCM_Y-10*FONTSIZE)/2);
	head = (struct list*)malloc(sizeof(struct list));
	if(head==NULL) 	return 0;
	list_init(head);
	Point editctrl_pos = {msgbox_pos.x+FONTSIZE*4, msgbox_pos.y+2*FONTSIZE};  //位置
	edit_init(&edit1, str, 4, editctrl_pos, FRAME, RETFLG, head,KEYBOARD_DEC);	//控件初始化
	Point button_ok_pos = {msgbox_pos.x+1*FONTSIZE, msgbox_pos.y+5*FONTSIZE};
	button_init(&btn_ok,(char *)"确定", button_ok_pos, CTRL_BUTTON_OK, head);
	Point button_esc_pos = {msgbox_pos.x+8*FONTSIZE, msgbox_pos.y+FONTSIZE*5};
	button_init(&btn_esc,(char *)"取消", button_esc_pos, CTRL_BUTTON_CANCEL, head);
	btn_esc.form.focus = FOCUS;
	gui_setrect(&msgbox_rect, msgbox_pos.x, msgbox_pos.y, msgbox_pos.x+13*FONTSIZE, msgbox_pos.y+8*FONTSIZE);
	oprmode_old = get_oprmode();
	set_oprmode(OPRMODE_MODIFY);
	msgbox(msgbox_rect, head, NULL, &msgbox_ret);
	set_oprmode(oprmode_old);
	if(head!=NULL)
		free(head);
	if(msgbox_ret.btn_ret==ACK){
		cldno = atoi((char*)msgbox_ret.s_ret);
//		dbg_prt("\n s_cldno[%02x][%02x][%02x][%02x] cldno=%d",
//				msgbox_ret.s_ret[3],msgbox_ret.s_ret[2],msgbox_ret.s_ret[1],msgbox_ret.s_ret[0],cldno);
	}else
		cldno = -1;
	return cldno;
}
//通过测量点地址抄表
//num 输入几位地址  上海手动抄表输入6位
int msgbox_inputcldaddr(char *cldaddr,int num){
	Rect msgbox_rect;
	MsgBoxRet msgbox_ret;
	struct list *head=NULL;
	char str[10],oprmode_old;
	Point msgbox_pos;
	memset(str, 0, 10);
	Edit edit1;
	Button btn_ok, btn_esc;
	memset(&edit1, 0, sizeof(Edit));
	memset(&btn_ok, 0, sizeof(Button));
	memset(&btn_esc, 0, sizeof(Button));
	memset(&msgbox_ret, 0, sizeof(MsgBoxRet));
	gui_setpos(&msgbox_pos, (LCM_X-15*FONTSIZE)/2, (LCM_Y-10*FONTSIZE)/2);
	head = (struct list*)malloc(sizeof(struct list));
	if(head==NULL) 	return 0;
	list_init(head);
	Point editctrl_pos = {msgbox_pos.x+FONTSIZE*2, msgbox_pos.y+2*FONTSIZE};  //位置
	edit_init(&edit1, str, num, editctrl_pos, FRAME, RETFLG, head,KEYBOARD_DEC);	//控件初始化
	Point button_ok_pos = {msgbox_pos.x+2*FONTSIZE, msgbox_pos.y+5*FONTSIZE};
	button_init(&btn_ok,(char *)"确定", button_ok_pos, CTRL_BUTTON_OK, head);
	Point button_esc_pos = {msgbox_pos.x+10*FONTSIZE, msgbox_pos.y+FONTSIZE*5};
	button_init(&btn_esc,(char *)"取消", button_esc_pos, CTRL_BUTTON_CANCEL, head);
	btn_esc.form.focus = FOCUS;
	gui_setrect(&msgbox_rect, msgbox_pos.x, msgbox_pos.y, msgbox_pos.x+16*FONTSIZE, msgbox_pos.y+8*FONTSIZE);
	oprmode_old = get_oprmode();
	set_oprmode(OPRMODE_MODIFY);
	msgbox(msgbox_rect, head, NULL, &msgbox_ret);
	set_oprmode(oprmode_old);
	if(head!=NULL)
		free(head);
	if(msgbox_ret.btn_ret==ACK){
//		asc2bcd((INT8U*)msgbox_ret.s_ret, 12, (INT8U*)cldaddr, inverted);
//		dbg_prt("\n s_cldnoaddr=[%02x%02x%02x%02x%02x%02x] ",
//				cldaddr[5],cldaddr[4],cldaddr[3],cldaddr[2],cldaddr[1],cldaddr[0]);
		memcpy(cldaddr, msgbox_ret.s_ret, 12);
//		dbg_prt("\n input addr = %s",cldaddr);
	}
	return msgbox_ret.btn_ret;
}
//2013-08-22 12:23:34   年月日时分秒：len=14 年月日：len=8  年月：len=6
int msgbox_inputjzqtime(char *s_jzqtime, int len)
{
	Rect msgbox_rect;
	MsgBoxRet msgbox_ret;
	struct list *head=NULL;
	char str[10];
	Point msgbox_pos;
	memset(str, 0, 10);
	Edit edit1;
	Button btn_ok, btn_esc;
	memset(&edit1, 0, sizeof(Edit));
	memset(&btn_ok, 0, sizeof(Button));
	memset(&btn_esc, 0, sizeof(Button));
	memset(&msgbox_ret, 0, sizeof(MsgBoxRet));
	gui_setpos(&msgbox_pos, (LCM_X-20*FONTSIZE)/2, (LCM_Y-10*FONTSIZE)/2);
	head = (struct list*)malloc(sizeof(struct list));
	if(head==NULL) 	return 0;
	list_init(head);
	Point editctrl_time_pos;
	if(len==6)
		editctrl_time_pos.x = msgbox_pos.x+FONTSIZE*6;
	else if(len==8)
		editctrl_time_pos.x = msgbox_pos.x+FONTSIZE*4;
	else
		editctrl_time_pos.x = msgbox_pos.x+FONTSIZE;
	editctrl_time_pos.y = msgbox_pos.y+2*FONTSIZE;
	memcpy(&msgbox_ret.s_ret[0], s_jzqtime, len);
	edittime_init(&edit1, CTRL_EDITTIME, &msgbox_ret.s_ret[0], len, editctrl_time_pos, NOFRAME, RETFLG, head);	//控件初始化
	Point button_ok_pos = {msgbox_pos.x+4*FONTSIZE, msgbox_pos.y+5*FONTSIZE};
	button_init(&btn_ok,(char *)"确定", button_ok_pos, CTRL_BUTTON_OK, head);
	Point button_esc_pos = {msgbox_pos.x+11*FONTSIZE, msgbox_pos.y+FONTSIZE*5};
	button_init(&btn_esc,(char *)"取消", button_esc_pos, CTRL_BUTTON_CANCEL, head);
	btn_esc.form.focus = FOCUS;//默认按键
	if(len==6)
		gui_setrect(&msgbox_rect, msgbox_pos.x, msgbox_pos.y, msgbox_pos.x+19*FONTSIZE, msgbox_pos.y+8*FONTSIZE);
	else if(len==8)
		gui_setrect(&msgbox_rect, msgbox_pos.x, msgbox_pos.y, msgbox_pos.x+20*FONTSIZE, msgbox_pos.y+8*FONTSIZE);
	else
		gui_setrect(&msgbox_rect, msgbox_pos.x, msgbox_pos.y, msgbox_pos.x+21*FONTSIZE, msgbox_pos.y+8*FONTSIZE);
	msgbox(msgbox_rect, head, NULL, &msgbox_ret);
	if(head!=NULL)
		free(head);
	memcpy(s_jzqtime, msgbox_ret.s_ret, len);
	return msgbox_ret.btn_ret;
}
int msgbox_jzqaddr_10(char *s_jzqdizhi10, int len){
	Text_t ttext[MSGBOX_TEXTSIZE];
	Rect msgbox_rect;
	MsgBoxRet msgbox_ret;
	struct list *head=NULL;
	char str[10];
	Point msgbox_pos;
	memset(str, 0, 10);
	Edit edit1, edit2;
	Button btn_ok, btn_esc;
	memset(&edit1, 0, sizeof(Edit));
	memset(&edit2, 0, sizeof(Edit));
	memset(&btn_ok, 0, sizeof(Button));
	memset(&btn_esc, 0, sizeof(Button));
	memset(&msgbox_ret, 0, sizeof(MsgBoxRet));
	gui_setpos(&msgbox_pos, (LCM_X-18*FONTSIZE)/2, (LCM_Y-14*FONTSIZE)/2);
	head = (struct list*)malloc(sizeof(struct list));
	if(head==NULL) 	return 0;
	list_init(head);
	Point editctrl_pos = {msgbox_pos.x+FONTSIZE, msgbox_pos.y+4*FONTSIZE};  //位置
	edit_init(&edit1, &s_jzqdizhi10[0], len, editctrl_pos, FRAME, RETFLG, head,KEYBOARD_DEC);	//控件初始化 区号
//	Point editctrl_pos1 = {msgbox_pos.x+FONTSIZE*11, msgbox_pos.y+3*FONTSIZE+6};  //位置
//	edit_init(&edit2, &s_jzqdizhi10[4], len+1, editctrl_pos1, FRAME, RETFLG, head,KEYBOARD_DEC);	//控件初始化 逻辑地址
	Point button_ok_pos = {msgbox_pos.x+3*FONTSIZE, msgbox_pos.y+8*FONTSIZE};
	button_init(&btn_ok,(char *)"确定", button_ok_pos, CTRL_BUTTON_OK, head);
	Point button_esc_pos = {msgbox_pos.x+10*FONTSIZE, msgbox_pos.y+FONTSIZE*8};
	button_init(&btn_esc,(char *)"取消", button_esc_pos, CTRL_BUTTON_CANCEL, head);
	btn_esc.form.focus = FOCUS;

	memset(ttext, 0, MSGBOX_TEXTSIZE*sizeof(Text_t));
	gui_setpos(&ttext[0].pos, msgbox_pos.x+FONTSIZE, msgbox_pos.y+FONTSIZE);
	memcpy(ttext[0].text, (char*)"终端地址:", strlen("终端地址:"));
//	gui_setpos(&ttext[1].pos, msgbox_pos.x+FONTSIZE, msgbox_pos.y+3*FONTSIZE+6);
//	memcpy(ttext[1].text, (char*)"逻辑地址:", strlen("逻辑地址:"));
	gui_setrect(&msgbox_rect, msgbox_pos.x, msgbox_pos.y, msgbox_pos.x+18*FONTSIZE, msgbox_pos.y+12*FONTSIZE);
	msgbox(msgbox_rect, head, ttext, &msgbox_ret);
	if(head!=NULL)
		free(head);

	memcpy(s_jzqdizhi10, msgbox_ret.s_ret, len*2+1);
//	dbg_prt("\n intput dizhi10: ");
//	for(i=0; i<len; i++)
//		dbg_prt("%d ", s_jzqdizhi10[i]);
//	dbg_prt("\n");
	return msgbox_ret.btn_ret;
}
//int msgbox_jzqaddr_10(char *s_jzqdizhi10, int len){
//	int i;
//	Text_t ttext[MSGBOX_TEXTSIZE];
//	char cb_text[TEXTLEN_X][TEXTLEN_Y];//用于存放combox的元素
//	Combox cb_type_addr;
//	Rect msgbox_rect;
//	MsgBoxRet msgbox_ret;
//	struct list *head=NULL;
//	char str[10];
//	Point pos;
//	Point msgbox_pos;
//	memset(str, 0, 10);
//	Edit edit1, edit2;
//	Button btn_ok, btn_esc;
//	memset(&edit1, 0, sizeof(Edit));
//	memset(&edit2, 0, sizeof(Edit));
//	memset(&btn_ok, 0, sizeof(Button));
//	memset(&btn_esc, 0, sizeof(Button));
//	memset(&msgbox_ret, 0, sizeof(MsgBoxRet));
//	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
//	gui_setpos(&pos, rect_Client.left+15*FONTSIZE, rect_Client.top);
//
//	gui_setpos(&msgbox_pos, (LCM_X-18*FONTSIZE)/2, (LCM_Y-14*FONTSIZE)/2);
//	head = (struct list*)malloc(sizeof(struct list));
//	if(head==NULL) 	return 0;
//	list_init(head);
//
//	Point editctrl_pos = {msgbox_pos.x+FONTSIZE*13, msgbox_pos.y+1*FONTSIZE};  //位置
//	edit_init(&edit1, &s_jzqdizhi10[0], len, editctrl_pos, FRAME, RETFLG, head,KEYBOARD_DEC);	//控件初始化 区号
//	Point editctrl_pos1 = {msgbox_pos.x+FONTSIZE*13, msgbox_pos.y+3*FONTSIZE+6};  //位置
//	edit_init(&edit2, &s_jzqdizhi10[4], len+1, editctrl_pos1, FRAME, RETFLG, head,KEYBOARD_DEC);	//控件初始化 逻辑地址
//	Point button_ok_pos = {msgbox_pos.x+3*FONTSIZE, msgbox_pos.y+7*FONTSIZE};
//	button_init(&btn_ok,(char *)"确定", button_ok_pos, CTRL_BUTTON_OK, head);
//	Point button_esc_pos = {msgbox_pos.x+10*FONTSIZE, msgbox_pos.y+FONTSIZE*7};
//	button_init(&btn_esc,(char *)"取消", button_esc_pos, CTRL_BUTTON_CANCEL, head);
//	btn_esc.form.focus = FOCUS;
//
//	memset(ttext, 0, MSGBOX_TEXTSIZE*sizeof(Text_t));
//	gui_setpos(&ttext[0].pos, msgbox_pos.x+FONTSIZE, msgbox_pos.y+FONTSIZE);
//	memcpy(ttext[0].text, (char*)"服务器地址:", strlen("服务器地址:"));
//	gui_setpos(&ttext[1].pos, msgbox_pos.x+FONTSIZE, msgbox_pos.y+3*FONTSIZE+6);
//	memcpy(ttext[1].text, (char*)"客户机地址:", strlen("客户机地址:"));
//	gui_setrect(&msgbox_rect, msgbox_pos.x, msgbox_pos.y, msgbox_pos.x+18*FONTSIZE, msgbox_pos.y+10*FONTSIZE);
//	setid_showlabel();
//	msgbox(msgbox_rect, head, ttext, &msgbox_ret);
//	if(head!=NULL)
//		free(head);
//
//	memcpy(s_jzqdizhi10, msgbox_ret.s_ret, len*2+1);
////	dbg_prt("\n intput dizhi10: ");
//	for(i=0; i<len; i++)
////		dbg_prt("%d ", s_jzqdizhi10[i]);
////	dbg_prt("\n");
//	return msgbox_ret.btn_ret;
//}
//int msgbox_jzqaddr_16(char *s_jzqdizhi16, int len){
//	int i;
//	Text_t ttext[MSGBOX_TEXTSIZE];
//	Rect msgbox_rect;
//	MsgBoxRet msgbox_ret;
//	struct list *head=NULL;
//	char str[10];
//	Point msgbox_pos;
//	memset(str, 0, 10);
//	Edit edit1, edit2;
//	Button btn_ok, btn_esc;
//	memset(&edit1, 0, sizeof(Edit));
//	memset(&edit2, 0, sizeof(Edit));
//	memset(&btn_ok, 0, sizeof(Button));
//	memset(&btn_esc, 0, sizeof(Button));
//	memset(&msgbox_ret, 0, sizeof(MsgBoxRet));
//	gui_setpos(&msgbox_pos, (LCM_X-18*FONTSIZE)/2, (LCM_Y-14*FONTSIZE)/2);
//	head = (struct list*)malloc(sizeof(struct list));
//	if(head==NULL) 	return 0;
//	list_init(head);
//	Point editctrl_pos = {msgbox_pos.x+FONTSIZE*11, msgbox_pos.y+1*FONTSIZE};  //位置
//	edit_init(&edit1, &s_jzqdizhi16[0], len, editctrl_pos, FRAME, RETFLG, head,KEYBOARD_DEC);	//控件初始化
//	Point editctrl_pos1 = {msgbox_pos.x+FONTSIZE*11, msgbox_pos.y+3*FONTSIZE+6};  //位置
//	edit_init(&edit2, &s_jzqdizhi16[4], len, editctrl_pos1, FRAME, RETFLG, head,KEYBOARD_HEX);	//控件初始化
//	Point button_ok_pos = {msgbox_pos.x+3*FONTSIZE, msgbox_pos.y+7*FONTSIZE};
//	button_init(&btn_ok,(char *)"确定", button_ok_pos, CTRL_BUTTON_OK, head);
//	Point button_esc_pos = {msgbox_pos.x+10*FONTSIZE, msgbox_pos.y+FONTSIZE*7};
//	button_init(&btn_esc,(char *)"取消", button_esc_pos, CTRL_BUTTON_CANCEL, head);
//	btn_esc.form.focus = FOCUS;
//	memset(ttext, 0, MSGBOX_TEXTSIZE*sizeof(Text_t));
//	gui_setpos(&ttext[0].pos, msgbox_pos.x+FONTSIZE, msgbox_pos.y+FONTSIZE);
//	memcpy(ttext[0].text, (char*)"行政区号:", strlen("行政区号:"));
//	gui_setpos(&ttext[1].pos, msgbox_pos.x+FONTSIZE, msgbox_pos.y+3*FONTSIZE+6);
//	memcpy(ttext[1].text, (char*)"逻辑地址:", strlen("逻辑地址:"));
//	gui_setrect(&msgbox_rect, msgbox_pos.x, msgbox_pos.y, msgbox_pos.x+18*FONTSIZE, msgbox_pos.y+10*FONTSIZE);
//	msgbox(msgbox_rect, head, ttext, &msgbox_ret);
//	if(head!=NULL)
//		free(head);
//	memcpy(s_jzqdizhi16, msgbox_ret.s_ret, len*2);
////	dbg_prt("\n intput dizhi16: ");
//	for(i=0; i<2*len; i++)
////		dbg_prt("%d ", s_jzqdizhi16[i]);
////	dbg_prt("\n");
//	return msgbox_ret.btn_ret;
//}

//设置ip port  前12个字节代表ip 后4个字节代表port
int msgbox_masterip(char *s_ipport, int len){
	Text_t ttext[MSGBOX_TEXTSIZE];
	Rect msgbox_rect;
	MsgBoxRet msgbox_ret;
	struct list *head=NULL;
	char str[10];
	Point msgbox_pos;
	memset(str, 0, 10);
	Edit edit1, edit2;
	Button btn_ok, btn_esc;
	memset(&edit1, 0, sizeof(Edit));
	memset(&edit2, 0, sizeof(Edit));
	memset(&btn_ok, 0, sizeof(Button));
	memset(&btn_esc, 0, sizeof(Button));
	memset(&msgbox_ret, 0, sizeof(MsgBoxRet));
	gui_setpos(&msgbox_pos, (LCM_X-17*FONTSIZE)/2, (LCM_Y-14*FONTSIZE)/2);
	head = (struct list*)malloc(sizeof(struct list));
	if(head==NULL) 	return 0;
	list_init(head);
	Point editctrl_ip_pos = {msgbox_pos.x+FONTSIZE*2, msgbox_pos.y+3*FONTSIZE+2};  //位置
	editip_init(&edit1, CTRL_EDITIP, &msgbox_ret.s_ret[0], 12, editctrl_ip_pos, NOFRAME, RETFLG, head,KEYBOARD_DEC);	//控件初始化
	Point editctrl_port_pos = {msgbox_pos.x+FONTSIZE*9, msgbox_pos.y+5*FONTSIZE+6};  //位置
	edit_init(&edit2, str, 4, editctrl_port_pos, NOFRAME, RETFLG,head,KEYBOARD_DEC);	//控件初始化//控件初始化

	Point button_ok_pos = {msgbox_pos.x+3*FONTSIZE, msgbox_pos.y+9*FONTSIZE};
	button_init(&btn_ok,(char *)"确定", button_ok_pos, CTRL_BUTTON_OK, head);
	Point button_esc_pos = {msgbox_pos.x+11*FONTSIZE, msgbox_pos.y+FONTSIZE*9};
	button_init(&btn_esc,(char *)"取消", button_esc_pos, CTRL_BUTTON_CANCEL, head);
	btn_esc.form.focus = FOCUS;
	memset(ttext, 0, MSGBOX_TEXTSIZE*sizeof(Text_t));
	gui_setpos(&ttext[0].pos, msgbox_pos.x+FONTSIZE, msgbox_pos.y+FONTSIZE);
	memcpy(ttext[0].text, (char*)"主站IP:", strlen("主站IP:"));
	gui_setpos(&ttext[1].pos, msgbox_pos.x+FONTSIZE, msgbox_pos.y+5*FONTSIZE+6);
	memcpy(ttext[1].text, (char*)"端口号:", strlen("端口号:"));
	gui_setrect(&msgbox_rect, msgbox_pos.x, msgbox_pos.y, msgbox_pos.x+19*FONTSIZE, msgbox_pos.y+13*FONTSIZE);
	msgbox(msgbox_rect, head, ttext, &msgbox_ret);
	if(head!=NULL)
		free(head);
	memcpy(s_ipport, msgbox_ret.s_ret, len);
//	dbg_prt("\n s_jzqip:[%s]", s_ipport);
	return msgbox_ret.btn_ret;
}

int msgbox_jzqip(char *s_ipport, int len){
	Text_t ttext[MSGBOX_TEXTSIZE];
	Rect msgbox_rect;
	MsgBoxRet msgbox_ret;
	struct list *head=NULL;
	char str[10],oprmode_old;
	Point msgbox_pos;
	memset(str, 0, 10);
	Edit edit1;
	Button btn_ok, btn_esc;
	memset(&edit1, 0, sizeof(Edit));
	memset(&btn_ok, 0, sizeof(Button));
	memset(&btn_esc, 0, sizeof(Button));
	memset(&msgbox_ret, 0, sizeof(MsgBoxRet));
	gui_setpos(&msgbox_pos, (LCM_X-17*FONTSIZE)/2, (LCM_Y-14*FONTSIZE)/2);
	head = (struct list*)malloc(sizeof(struct list));
	if(head==NULL) 	return 0;
	list_init(head);

	Point editctrl_ip_pos = {msgbox_pos.x+FONTSIZE*1, msgbox_pos.y+3*FONTSIZE+2};  //位置 ip
	editip_init(&edit1, CTRL_EDITIP, s_ipport, 12, editctrl_ip_pos, NOFRAME, RETFLG, head,KEYBOARD_DEC);	//控件初始化

	Point button_ok_pos = {msgbox_pos.x+4*FONTSIZE, msgbox_pos.y+7*FONTSIZE};
	button_init(&btn_ok,(char *)"确定", button_ok_pos, CTRL_BUTTON_OK, head);
	Point button_esc_pos = {msgbox_pos.x+11*FONTSIZE, msgbox_pos.y+FONTSIZE*7};
	button_init(&btn_esc,(char *)"取消", button_esc_pos, CTRL_BUTTON_CANCEL, head);
	btn_esc.form.focus = FOCUS;
	memset(ttext, 0, MSGBOX_TEXTSIZE*sizeof(Text_t));
	gui_setpos(&ttext[0].pos, msgbox_pos.x+FONTSIZE, msgbox_pos.y+FONTSIZE);
	memcpy(ttext[0].text, (char*)"本地IP:", strlen("本地IP:"));
	gui_setrect(&msgbox_rect, msgbox_pos.x, msgbox_pos.y, msgbox_pos.x+17*FONTSIZE, msgbox_pos.y+11*FONTSIZE);
	oprmode_old = get_oprmode();
	set_oprmode(OPRMODE_MODIFY);
	msgbox(msgbox_rect, head, ttext, &msgbox_ret);
	set_oprmode(oprmode_old);
	if(head!=NULL)
		free(head);
	memcpy(s_ipport, msgbox_ret.s_ret, len);
//	dbg_prt("\n s_jzqip:[%s]", s_ipport);
	return msgbox_ret.btn_ret;
}
int _msgbox_soubiao(int *index, Text_t *ttext, MsgBoxRet *msgbox_ret)
{
	Rect msgbox_rect;
//	MsgBoxRet msgbox_ret;
	struct list *head=NULL;
	char str[10];
	Point msgbox_pos;
	char cb_text[TEXTLEN_X][TEXTLEN_Y];//用于存放combox的元素
	memset(str, 0, 10);
	Combox combox1;
	Button btn_ok, btn_esc;
	memset(&combox1, 0, sizeof(Combox));
	memset(&btn_ok, 0, sizeof(Button));
	memset(&btn_esc, 0, sizeof(Button));
	memset(msgbox_ret, 0, sizeof(MsgBoxRet));
	gui_setpos(&msgbox_pos, (LCM_X-15*FONTSIZE)/2, (LCM_Y-10*FONTSIZE)/2);
	head = (struct list*)malloc(sizeof(struct list));
	if(head==NULL) 	return 0;
	list_init(head);
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "使能", strlen("使能"));
	memcpy(cb_text[1], "关闭", strlen("关闭"));
	Point cb_pos = {msgbox_pos.x+FONTSIZE*11, msgbox_pos.y+2*FONTSIZE};  //位置
	combox_init(&combox1, 0, cb_text, cb_pos, RETFLG, head);
	Point button_ok_pos = {msgbox_pos.x+3*FONTSIZE, msgbox_pos.y+5*FONTSIZE};
	button_init(&btn_ok,(char *)"确定", button_ok_pos, CTRL_BUTTON_OK, head);
	Point button_esc_pos = {msgbox_pos.x+10*FONTSIZE, msgbox_pos.y+FONTSIZE*5};
	button_init(&btn_esc,(char *)"取消", button_esc_pos, CTRL_BUTTON_CANCEL, head);
	btn_esc.form.focus = FOCUS;

	ttext[0].pos.x += msgbox_pos.x;
	ttext[0].pos.y += msgbox_pos.y;
	gui_setrect(&msgbox_rect, msgbox_pos.x, msgbox_pos.y, msgbox_pos.x+17*FONTSIZE, msgbox_pos.y+8*FONTSIZE);
	msgbox(msgbox_rect, head, ttext, msgbox_ret);
	*index = msgbox_ret->s_ret[0];
	if(head!=NULL)
		free(head);
	return msgbox_ret->btn_ret;
}
int msgbox_soubiao(int *index, MsgBoxRet *msgbox_ret){
	Text_t ttext[MSGBOX_TEXTSIZE];
	memset(ttext, 0, MSGBOX_TEXTSIZE*sizeof(Text_t));
	gui_setpos(&ttext[0].pos, FONTSIZE, 2*FONTSIZE);
	memcpy(ttext[0].text, (char*)"手动搜表:", strlen("手动搜表:"));
	return _msgbox_soubiao(index, ttext, msgbox_ret);
}
int msgbox_heart(char *s_heart, int len){
	Text_t ttext[MSGBOX_TEXTSIZE];
	Rect msgbox_rect;
	MsgBoxRet msgbox_ret;
	struct list *head=NULL;
	char str[10];
	Point msgbox_pos;
	memset(str, 0, 10);
	Edit edit1;
	Button btn_ok, btn_esc;
	memset(&edit1, 0, sizeof(Edit));
	memset(&btn_ok, 0, sizeof(Button));
	memset(&btn_esc, 0, sizeof(Button));
	memset(&msgbox_ret, 0, sizeof(MsgBoxRet));
	gui_setpos(&msgbox_pos, (LCM_X-18*FONTSIZE)/2, (LCM_Y-10*FONTSIZE)/2);
	head = (struct list*)malloc(sizeof(struct list));
	if(head==NULL) 	return 0;
	list_init(head);
	Point editctrl_pos = {msgbox_pos.x+FONTSIZE*10.5, msgbox_pos.y+2*FONTSIZE};  //位置
	edit_init(&edit1, str, 2, editctrl_pos, FRAME, RETFLG, head,KEYBOARD_DEC);	//控件初始化
	Point button_ok_pos = {msgbox_pos.x+3*FONTSIZE, msgbox_pos.y+5*FONTSIZE};
	button_init(&btn_ok,(char *)"确定", button_ok_pos, CTRL_BUTTON_OK, head);
	Point button_esc_pos = {msgbox_pos.x+11*FONTSIZE, msgbox_pos.y+FONTSIZE*5};
	button_init(&btn_esc,(char *)"取消", button_esc_pos, CTRL_BUTTON_CANCEL, head);
	btn_esc.form.focus = FOCUS;
	memset(ttext, 0, MSGBOX_TEXTSIZE*sizeof(Text_t));
	gui_setpos(&ttext[0].pos, msgbox_pos.x+FONTSIZE, msgbox_pos.y+2*FONTSIZE);
	memcpy(ttext[0].text, (char*)"心跳周期:", strlen("心跳周期:"));
	gui_setpos(&ttext[1].pos, msgbox_pos.x+13*FONTSIZE+4, msgbox_pos.y+2*FONTSIZE);
	memcpy(ttext[1].text, (char*)"分钟", strlen("分钟"));
	gui_setrect(&msgbox_rect, msgbox_pos.x, msgbox_pos.y, msgbox_pos.x+18*FONTSIZE, msgbox_pos.y+8*FONTSIZE);
	msgbox(msgbox_rect, head, ttext, &msgbox_ret);
	if(head!=NULL)
		free(head);
	memcpy(s_heart, msgbox_ret.s_ret, 2);
//	dbg_prt("\n s_heart:[%s]", s_heart);
	return msgbox_ret.btn_ret;
}

//设置apn
int msgbox_apn(char *s_apn){
	Text_t ttext[MSGBOX_TEXTSIZE];
	Rect msgbox_rect;
	MsgBoxRet msgbox_ret;
	Edit edit1;
	Button btn_ok, btn_esc;
	struct list *head=NULL;
	char str[INPUTKEYNUM];
	Point msgbox_pos;
	memset(str, ' ', INPUTKEYNUM);
	//TODO:APN显示
//	if(strlen((char*)ParaAll->f3.APN)!=0)
//		memcpy(str, ParaAll->f3.APN, strlen((char*)ParaAll->f3.APN));
//	else
		memcpy(str, "CMNET", strlen("CMNET"));
	//dbg_prt("\n str=%s str[4-6]=%d %d %d  %d", str,str[4],str[5],str[6],strlen(str));
	memset(&edit1, 0, sizeof(Edit));
	memset(&btn_ok, 0, sizeof(Button));
	memset(&btn_esc, 0, sizeof(Button));
	memset(&msgbox_ret, 0, sizeof(MsgBoxRet));
	gui_setpos(&msgbox_pos, (LCM_X-20*FONTSIZE)/2, (LCM_Y-10*FONTSIZE)/2);
	head = (struct list*)malloc(sizeof(struct list));
	if(head==NULL) 	return 0;
	list_init(head);
	Point editctrl_pos = {msgbox_pos.x+FONTSIZE*2, msgbox_pos.y+3*FONTSIZE};  //位置
	edit_init(&edit1, str, 16, editctrl_pos, FRAME, RETFLG, head,KEYBOARD_ASC);	//控件初始化
	Point button_ok_pos = {msgbox_pos.x+2*FONTSIZE, msgbox_pos.y+6*FONTSIZE};
	button_init(&btn_ok,(char *)"确定", button_ok_pos, CTRL_BUTTON_OK, head);
	Point button_esc_pos = {msgbox_pos.x+10*FONTSIZE, msgbox_pos.y+FONTSIZE*6};
	button_init(&btn_esc,(char *)"取消", button_esc_pos, CTRL_BUTTON_CANCEL, head);
	btn_esc.form.focus = FOCUS;
	memset(ttext, 0, MSGBOX_TEXTSIZE*sizeof(Text_t));
	gui_setpos(&ttext[0].pos, msgbox_pos.x+FONTSIZE, msgbox_pos.y+4);
	memcpy(ttext[0].text, (char*)"APN:", strlen("APN:"));
	gui_setrect(&msgbox_rect, msgbox_pos.x, msgbox_pos.y, msgbox_pos.x+20*FONTSIZE, msgbox_pos.y+9*FONTSIZE);
	msgbox(msgbox_rect, head, ttext, &msgbox_ret);
	if(head!=NULL)
		free(head);
	if(msgbox_ret.btn_ret==ACK){
		memcpy(s_apn, msgbox_ret.s_ret, strlen(msgbox_ret.s_ret));
//		dbg_prt("\n apn=%s",s_apn);
	}
	return msgbox_ret.btn_ret;
}

int msgbox_oprmode(int ctrl_focus){
	Rect msgbox_rect;
	MsgBoxRet msgbox_ret;
	struct list *head=NULL;
	Point msgbox_pos;
	Button btn_ok, btn_esc;
	memset(&btn_ok, 0, sizeof(Button));
	memset(&btn_esc, 0, sizeof(Button));
	memset(&msgbox_ret, 0, sizeof(MsgBoxRet));
	gui_setpos(&msgbox_pos,(LCM_X-rect_Client.left-18*FONTSIZE)/2, (LCM_Y-12*FONTSIZE)/2);
	head = (struct list*)malloc(sizeof(struct list));
	if(head==NULL) 	return 0;
	list_init(head);
	Point button_ok_pos = {msgbox_pos.x + 3*FONTSIZE, msgbox_pos.y+3*FONTSIZE};
	button_init(&btn_ok,(char *)"1.设置模式", button_ok_pos, CTRL_BUTTON_OK, head);
	Point button_esc_pos = {msgbox_pos.x + 3*FONTSIZE, msgbox_pos.y+7*FONTSIZE};
	button_init(&btn_esc,(char *)"2.查看模式", button_esc_pos, CTRL_BUTTON_CANCEL, head);
	if(ctrl_focus==CTRL_BUTTON_OK)
		btn_ok.form.focus = FOCUS;
	else
		btn_esc.form.focus = FOCUS;

	gui_setrect(&msgbox_rect, msgbox_pos.x, msgbox_pos.y, msgbox_pos.x+16*FONTSIZE, msgbox_pos.y+12*FONTSIZE);
	msgbox(msgbox_rect, head, NULL, &msgbox_ret);
	if(head!=NULL)
		free(head);
	if(msgbox_ret.btn_ret==ACK)
		return OPRMODE_MODIFY;
	else if(msgbox_ret.btn_ret==CAN)
		return OPRMODE_LOOK;
	else
		return 0;
}
