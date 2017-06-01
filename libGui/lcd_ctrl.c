/*
 * lcd_ctrl.c  控件
 *
 *  Created on: 2013-5-14
 *      Author: yd
 */
#include "lcd_ctrl.h"
Key KeyBoard[KEYBOARD_LEN][KEYBOARD_WIDTH];
Key KeyBoadrd_old[INPUTKEYNUM];

void initHexAF(Point start_pos, char c, int index_x, int index_y){
	KeyBoard[index_x][index_y].c = c;
	KeyBoard[index_x][index_y].pos.x = start_pos.x + index_y*(FONTSIZE+2);
	KeyBoard[index_x][index_y].pos.y = start_pos.y + index_x*FONTSIZE*2;
	KeyBoard[index_x][index_y].index.index_x = index_x;
	KeyBoard[index_x][index_y].index.index_y = index_y;
}
void initkeyboard(Point start_pos, char keyboard_style)
{
	int i, j, k=0, len=0, width=0;
	if(keyboard_style==KEYBOARD_ASC){
		len = KEYBOARD_LEN;
		width = KEYBOARD_WIDTH;
		//初始化数字
		for(i=0; i<len; i++){
			for(j=0; j<3; j++){
				KeyBoard[i][j].c = 48+k;
				k++;
				KeyBoard[i][j].pos.x = start_pos.x + j*(FONTSIZE+2);
				KeyBoard[i][j].pos.y = start_pos.y + i*(FONTSIZE)*2;
				KeyBoard[i][j].index.index_x = i;
				KeyBoard[i][j].index.index_y = j;
			}
		}
		KeyBoard[3][1].c = '.';
		//初始化字母
		KeyBoard[3][2].c = '-';
		k = 0;
		for(i=0; i<len; i++){
			for(j=3; j<width; j++){
				KeyBoard[i][j].c = 65+k;
				k++;
				KeyBoard[i][j].pos.x = start_pos.x + j*(FONTSIZE+2);
				KeyBoard[i][j].pos.y = start_pos.y + i*(FONTSIZE)*2;
				KeyBoard[i][j].index.index_x = i;
				KeyBoard[i][j].index.index_y = j;
				if(KeyBoard[i][j].c>90)
					KeyBoard[i][j].c = 0;
			}
		}
	}else if(keyboard_style==KEYBOARD_DEC){
		len = 	KEYBOARD_LEN;
		width = 3;
		//初始化数字
		for(i=0; i<len; i++){
			for(j=0; j<3; j++){
				KeyBoard[i][j].c = 48+k;
				k++;
				KeyBoard[i][j].pos.x = start_pos.x + j*(FONTSIZE+2);
				KeyBoard[i][j].pos.y = start_pos.y + i*(FONTSIZE)*2;
				KeyBoard[i][j].index.index_x = i;
				KeyBoard[i][j].index.index_y = j;
			}
		}
		KeyBoard[3][1].c = 0;
		KeyBoard[3][2].c = 0;
	}else{
		len = 	KEYBOARD_LEN;
		width = 5;
		//初始化数字
		for(i=0; i<len; i++){
			for(j=0; j<3; j++){
				KeyBoard[i][j].c = 48+k;
				k++;
				KeyBoard[i][j].pos.x = start_pos.x + j*(FONTSIZE+2);
				KeyBoard[i][j].pos.y = start_pos.y + i*(FONTSIZE)*2;
				KeyBoard[i][j].index.index_x = i;
				KeyBoard[i][j].index.index_y = j;
			}
		}
		KeyBoard[3][1].c = 0;
		KeyBoard[3][2].c = 0;
		//初始化字母
		initHexAF(start_pos, 'A', 0, 3);
		initHexAF(start_pos, 'B', 0, 4);
		initHexAF(start_pos, 'C', 1, 3);
		initHexAF(start_pos, 'D', 1, 4);
		initHexAF(start_pos, 'E', 2, 3);
		initHexAF(start_pos, 'F', 2, 4);
	}
}
//获得键盘大小
Point keyboard_getsize(char keyboard_style){
	Point size;
	if(keyboard_style==KEYBOARD_ASC){
		size.x = KEYBOARD_LEN;
		size.y = KEYBOARD_WIDTH;
	}else if(keyboard_style==KEYBOARD_HEX){
		size.x = KEYBOARD_LEN;
		size.y = 5;
	}else{
		size.x = KEYBOARD_LEN;
		size.y = 3;
	}
	return size;
}
//显示键盘
void showkeyboard(Key key[][KEYBOARD_WIDTH], char keyboard_style)
{
	Rect rect;
	int i, j, len=0, width=0;
	Point size;
	size = keyboard_getsize(keyboard_style);
	len = size.x;
	width = size.y;
	rect.left = KeyBoard[0][0].pos.x;
	rect.top = KeyBoard[0][0].pos.y;
	rect.right = KeyBoard[0][0].pos.x + width*(FONTSIZE+2);
	rect.bottom = KeyBoard[0][0].pos.y + len*(FONTSIZE)*2;
	gui_clrrect(gui_changerect(rect, 2));
	gui_rectangle(gui_changerect(rect, 2));

	for(i=0; i<len; i++){//4行10列
		for(j=0; j<width; j++){
			gui_charshow(key[i][j].c, key[i][j].pos, LCD_NOREV);
		}
	}
}

//从液晶键盘输入一个字符
INT8U gui_keyinput(Point start_pos, Key *curkey, char keyboard_style) {
	char first_flg = 0;
	int len=0, width=0;
	Point size;
	size = keyboard_getsize(keyboard_style);
	len = size.x;
	width = size.y;
	initkeyboard(start_pos, keyboard_style); //初始化键盘
	PressKey = NOKEY;
	while (g_LcdPoll_Flag == LCD_NOTPOLL) {
		switch (PressKey) {
		case UP:
			curkey->index.index_x--;
			if (curkey->index.index_x < 0 || curkey->index.index_x >= 255)
				curkey->index.index_x = len - 1;
			break;
		case DOWN:
			curkey->index.index_x++;
			curkey->index.index_x = (curkey->index.index_x+len)%(len);
			break;
		case LEFT:
			curkey->index.index_y--;
			if (curkey->index.index_y < 0 || curkey->index.index_y >= 255)
				curkey->index.index_y = width - 1;
			break;
		case RIGHT:
			curkey->index.index_y++;
			curkey->index.index_y = (curkey->index.index_y+width)%(width);
			break;
		case OK:
			return curkey->c;
		case ESC:
			return 0;
		default:
			break;
		}
		if (PressKey != NOKEY || first_flg == 0) { //显示反显curkey -->>
			first_flg = 1;
			PressKey = NOKEY;
			showkeyboard(KeyBoard,keyboard_style); //显示键盘
			curkey->c = KeyBoard[curkey->index.index_x][curkey->index.index_y].c;
			memcpy(&curkey->pos, &KeyBoard[curkey->index.index_x][curkey->index.index_y].pos,
					sizeof(Point));
			gui_charshow(curkey->c, curkey->pos, LCD_REV); //--反显选中的字符
//			dbg_prt( "\n gui_keyinput:%d %d ==  %d %d %d[%c]  ",
//					curkey->index.index_x, curkey->index.index_y, curkey->pos.x, curkey->pos.y, curkey->c, curkey->c);
		}
		PressKey = NOKEY;
		delay(100);
	}
	return 0;
}
//此处将FONTSIZE增大2 ，即每个字符左右增加一个点(待定)
Point keyboard_getpos(Edit *edit, int len, int width){
	Point pos;
	pos.x =(edit->form.rect.left+(edit->form.rect.right- edit->form.rect.left)/2)
					- width * (FONTSIZE+2) / 2;
	pos.y = edit->form.pos.y + 2.5 * (FONTSIZE);
	if(pos.x<3)
		pos.x = 6;
	if(pos.x+width*(FONTSIZE+2)>=LCM_X)
		pos.x = LCM_X - width*(FONTSIZE+2) - 6;
	if(pos.y+len*(FONTSIZE)*2>=LCM_Y-34)
		pos.y = pos.y - len*(FONTSIZE)*2 - (FONTSIZE)*3;
	return pos;
}
//在编辑模式下  form:控件  key：编辑框里的字符   curkey:当前字符
INT8U edit_editmode(Edit* edit, Key *curkey, int *press_ok)
{
	Point keyboard_pos;
	char first_flg = 0;
//	char esc_flg=0;
	char ok_flg=0;
	INT8U lcd_buf[LCM_X * LCM_Y], c_input = 0;
	Rect keyboard_rect;
	Key board_key;
	Point size;
	int len=0, width=0;
	size = keyboard_getsize(edit->keyboard_style);
	len = size.x;
	width = size.y;
	keyboard_pos = keyboard_getpos(edit, len, width);//获得键盘位置
	keyboard_rect = getrect(keyboard_pos, (FONTSIZE+2)*width, 2*(FONTSIZE)*len);
//	dbg_prt("\n form->keyboard_style=%d  len =%d width=%d keyboard:%d %d %d %d save",
//			edit->keyboard_style,len, width,keyboard_rect.left,keyboard_rect.top,keyboard_rect.right,keyboard_rect.bottom);
	memset(lcd_buf, 0, LCM_X * LCM_Y);
	lcdregion_save(lcd_buf, gui_changerect(keyboard_rect, 2));
	memcpy(&board_key, curkey, sizeof(Key));
	board_key.c= '0';
	board_key.index.index_x = 0;
	board_key.index.index_y = 0;
	memcpy(&board_key.pos, &keyboard_pos,sizeof(Point));
	PressKey=NOKEY;
	while (g_LcdPoll_Flag == LCD_NOTPOLL) {
		switch(PressKey){
		case UP:
		case DOWN:
			break;
		case RIGHT:
//			esc_flg=0;
			curkey->index.index_x = (curkey->index.index_x+1+edit->form.c_num)%edit->form.c_num;
			break;
		case LEFT:
//			esc_flg=0;
			curkey->index.index_x = (curkey->index.index_x-1+edit->form.c_num)%edit->form.c_num;
			break;
		case OK:
			ok_flg=1;
//			esc_flg=0;
			memcpy(curkey, &edit->form.key[curkey->index.index_x], sizeof(Key));
			gui_charshow(curkey->c, curkey->pos, LCD_REV);
			c_input = gui_keyinput(keyboard_pos, &board_key, edit->keyboard_style); //从键盘输入一个字符--------------------
			if (c_input != 0) {
				edit->form.key[curkey->index.index_x].c = c_input; //输入一个字符后自动跳到下一个字符处
				curkey->index.index_x = (curkey->index.index_x+1+edit->form.c_num)%edit->form.c_num;
			}
			if(PressKey == ESC)
				lcdregion_restore(lcd_buf, gui_changerect(keyboard_rect, 2));
			break;
		case ESC:
//			esc_flg++;
			lcdregion_restore(lcd_buf, gui_changerect(keyboard_rect, 2));
//			if(esc_flg>=2)
//			{
				if(ok_flg!=0)
					{
						*press_ok=OK;//进入过键盘编辑模式，做询问是否保存
						return 2;
					}else{
						*press_ok=ESC;//未进入键盘编辑模式，不进行询问
						return 3;
					}
//			}
			break;
		default:
			break;
		}
		if (PressKey != NOKEY || first_flg == 0) {
			first_flg=1;
			edit_show(edit);
//			if(esc_flg==0)
//			{
				memcpy(curkey, &edit->form.key[curkey->index.index_x], sizeof(Key));
				gui_charshow(curkey->c, curkey->pos, LCD_REV); //反显当前字符
//			}
			if(PressKey == OK )//此处为从gui_keyinput中跳出，如果为OK，则继续进入，不修改键值
				continue;
			PressKey = NOKEY;
		}
		delay(100);
	}
//	dbg_prt("\n len =%d width=%d keyboard:%d %d %d %d restore",
//			len, width,keyboard_rect.left,keyboard_rect.top,keyboard_rect.right,keyboard_rect.bottom);
	lcdregion_restore(lcd_buf, gui_changerect(keyboard_rect, 2));
	return SHOWMODE;
}
//在查看模式下 	form控件    key  键盘输入内容   curkey 当前输入的键值
INT8U edit_showmode(Edit* edit, Key *curkey)
{
	char first_flg = 0;
	PressKey = NOKEY;
	while (g_LcdPoll_Flag == LCD_NOTPOLL) {
		switch (PressKey) {
		case LEFT:
			curkey->index.index_x--;
			if (curkey->index.index_x < 0 || curkey->index.index_x >= 255)
				curkey->index.index_x = edit->form.c_num - 1;
			break;
		case RIGHT:
			curkey->index.index_x++;
			curkey->index.index_x = (curkey->index.index_x + edit->form.c_num)
					% edit->form.c_num;
			break;
		case OK:
			return EDITMODE;
		case ESC:
			edit_show(edit);
			return NORMALMODE;
		}
		if (PressKey == LEFT || PressKey == RIGHT || first_flg == 0) {//反显下一个字符
			edit_show(edit);
			first_flg = 1;
			curkey->c = edit->form.key[curkey->index.index_x].c;
			memcpy(&curkey->pos, &edit->form.key[curkey->index.index_x].pos,
					sizeof(Point));
			gui_charshow(curkey->c, curkey->pos, LCD_REV);
		}
		PressKey = NOKEY;
		delay(100);
	}
	return NORMALMODE;
}
void initkeyboard_old(char keyboard_style)
{
	int i;
	memset(KeyBoadrd_old, 0, INPUTKEYNUM);
	if(keyboard_style==KEYBOARD_ASC){//包括 0-9  A-Z . - 空格
		for(i=0; i<10; i++){
			KeyBoadrd_old[i].c = 0x30 + i;//'0'
			KeyBoadrd_old[i].index.index_x = i;
		}
		for(i=0; i<26; i++){
			KeyBoadrd_old[i+10].c = 0x41 + i;//'A'
			KeyBoadrd_old[i+10].index.index_x = i+10;
		}
		KeyBoadrd_old[36].c = '.';
		KeyBoadrd_old[36].index.index_x = 36;
		KeyBoadrd_old[37].c = '-';
		KeyBoadrd_old[37].index.index_x = 37;
		KeyBoadrd_old[38].c = '@';
		KeyBoadrd_old[38].index.index_x = 38;
		KeyBoadrd_old[39].c = ' ';
		KeyBoadrd_old[39].index.index_x = 39;
	}else if(keyboard_style==KEYBOARD_DEC){//包括 0-9 空格
		for(i=0; i<10; i++){
			KeyBoadrd_old[i].c = 0x30 + i;//'0'
			KeyBoadrd_old[i].index.index_x = i;
		}
		KeyBoadrd_old[i].c = ' ';
		KeyBoadrd_old[i].index.index_x = i;
	}else{//包括 0-9  A-F
		for(i=0; i<10; i++){
			KeyBoadrd_old[i].c = 0x30 + i;//'0'
			KeyBoadrd_old[i].index.index_x = i;
		}
		for(i=0; i<6; i++){
			KeyBoadrd_old[i+10].c = 0x41 + i;//'A'
			KeyBoadrd_old[i+10].index.index_x = i+10;
		}
	}
}
//获得keyboard中index位置
int keyboard_index_old(int index, char keyboard_style){
	if(keyboard_style==KEYBOARD_ASC){//包括 0-9  A-Z . - 空格
		if(index<0)
			index = 39;
		else if(index>39)
			index= 0;
	}else if(keyboard_style==KEYBOARD_DEC){//包括 0-9 空格
		if(index<0)
			index = 10;
		else if(index>10)
			index= 0;
	}else{//包括 0-9  A-F
		if(index<0)
			index = 15;
		else if(index>15)
			index= 0;
	}
	return index;
}
int keyboard_getindex_old(char c, char keyboard_style){
	int i,keyboard_index=0;
	if(keyboard_style==KEYBOARD_ASC){//包括 0-9  A-Z . - 空格
		for(i=0; i<38; i++){
			if(c==KeyBoadrd_old[i].c){
				keyboard_index = i;
				break;
			}
		}
	}else if(keyboard_style==KEYBOARD_DEC){//包括 0-9
		for(i=0; i<10; i++){
			if(c==KeyBoadrd_old[i].c){
				keyboard_index = i;
				break;
			}
		}
	}else{//包括 0-9  A-F
		for(i=0; i<16; i++){
			if(c==KeyBoadrd_old[i].c){
				keyboard_index = i;
				break;
			}
		}
	}
	return keyboard_index;
}
char keyboard_getc_old(int index){
	return KeyBoadrd_old[index].c;
}
INT8U edit_showmode_old(Edit* edit, Key *curkey, int *press_ok)
{
	char first_flg=0;
	int keyboard_index=0;
	initkeyboard_old(edit->keyboard_style);
	curkey->index.index_x = 0;
	PressKey = NOKEY;
	while (g_LcdPoll_Flag == LCD_NOTPOLL) {
		switch (PressKey) {
		case LEFT:
			curkey->index.index_x--;
			if (curkey->index.index_x < 0 || curkey->index.index_x >= 255)
				curkey->index.index_x = edit->form.c_num - 1;
			break;
		case RIGHT:
			curkey->index.index_x++;
			curkey->index.index_x = (curkey->index.index_x + edit->form.c_num)
					% edit->form.c_num;
			break;
		case UP:
			keyboard_index++;
			keyboard_index = keyboard_index_old(keyboard_index, edit->keyboard_style);
			break;
		case DOWN:
			keyboard_index--;
			keyboard_index = keyboard_index_old(keyboard_index, edit->keyboard_style);
			break;
		case OK:
			*press_ok = OK;
			edit_show(edit);
			return NORMALMODE;
		case ESC:
			*press_ok = ESC;
			edit_show(edit);
			return NORMALMODE;
		}
		if (PressKey==LEFT || PressKey==RIGHT || first_flg==0) {//反显下一个字符
			edit_show(edit);
			first_flg = 1;
			curkey->c = edit->form.key[curkey->index.index_x].c;
			memcpy(&curkey->pos, &edit->form.key[curkey->index.index_x].pos,sizeof(Point));
			gui_charshow(curkey->c, curkey->pos, LCD_REV);
			keyboard_index = keyboard_getindex_old(curkey->c, edit->keyboard_style);
		}else if(PressKey==UP || PressKey==DOWN){
			edit->form.key[curkey->index.index_x].c = keyboard_getc_old(keyboard_index);
			curkey->c = edit->form.key[curkey->index.index_x].c;
			gui_charshow(curkey->c, curkey->pos, LCD_REV);
		}
		PressKey = NOKEY;
		delay(100);
	}
	return NORMALMODE;
}
//初始化edit控件 c_num 输入的位数  text edit显示的内容  是否有边框 frame_flg   ret_flg此控件的输入值是否加入到返回值中
//edit_init(&form[0], CTRL_EDIT, str, 6, editctrl_pos, FRAME, RETFLG, head);	//控件初始化
void edit_init(Edit *edit, char *text, int c_num, Point pos, char frame_flg,
		char ret_flg, struct list *parent, char keyboard_style)
{
	int i;
	Rect rect;
	memset(edit, 0, sizeof(Edit));
	edit->form.CtlType = CTRL_EDIT;
	edit->mode = NORMALMODE;
	memcpy(&edit->form.pos, &pos, sizeof(Point));
	//初始化key
	if(text!=NULL){
		for (i = 0; i < INPUTKEYNUM; i++) {
			if (text[0] == 0 )
				edit->form.key[i].c = '0';
			else
				edit->form.key[i].c = text[i];
			edit->form.key[i].index.index_x = i;
			edit->form.key[i].pos.x = edit->form.pos.x + FONTSIZE * i;
			edit->form.key[i].pos.y = edit->form.pos.y;
	//		dbg_prt("\n edit->key[%d].c=%d pos:%d %d", i,
	//				edit->key[i].c,edit->key[i].pos.x,edit->key[i].pos.y);
		}
	}
	edit->keyboard_style = keyboard_style;
	edit->form.ret_flg = ret_flg;
	edit->frame_flg = frame_flg;
	edit->form.c_num = c_num;
	edit->form.rect = getrect(pos, FONTSIZE * c_num, FONTSIZE);
	memcpy(&rect, &edit->form.rect, sizeof(Rect));
	edit->form.pfun_process = edit_process;
	edit->form.pfun_show = edit_show;
	if (parent != NULL){
		edit->form.node.parent = parent;
		list_init(&edit->form.node);
		list_add_tail(parent, &edit->form.node);
	}
}

void edit_show(void* form)
{
	int i;
	Rect rect;
	Edit *edit = (Edit*)form;
	memcpy(&rect, &edit->form.rect, sizeof(Rect));
	rect.top += 2;
	rect.bottom += 4;
	if(edit->frame_flg==1)
		gui_rectangle(gui_changerect(rect, 4));
	for (i = 0; i < edit->form.c_num; i++)
		gui_charshow(edit->form.key[i].c, edit->form.key[i].pos, LCD_NOREV);
}
//显示编辑框
void edit_process(void* form)
{
	Edit *edit=(Edit *)form;
	int press_ok=NOKEY;
	Key curkey;
	memset(&curkey,0,sizeof(Key));
	if (edit->form.focus == FOCUS) {
		if (PressKey != OK)
			return;
		while (g_LcdPoll_Flag == LCD_NOTPOLL) {
			if (PressKey == OK) {
				edit->mode = SHOWMODE;
				while (edit->mode != NORMALMODE) {
					switch (edit->mode) {
					case EDITMODE:
						edit->mode = edit_editmode(edit, &curkey,&press_ok);
						break;
					case SHOWMODE:
						edit->mode = edit_showmode_old(edit, &curkey, &press_ok);
						break;
					}
					edit->form.pfun_process_ret = press_ok;
					if(press_ok==OK)
						break;
				}
				break;
			} else if (PressKey == ESC) {
				edit->mode = NORMALMODE;
				break;
			}
			delay(20);
		}
	}
}

//初始化combox控件  text 二维数组combox显示内容  default_index默认内容的索引
void combox_init(Combox *combox, int default_index, char (*text)[TEXTLEN_Y],
		Point pos, char ret_flg, struct list *parent)
{
	int i = 0, maxlen = 0, len = 0;
	memset(combox, 0, sizeof(Combox));
	combox->form.CtlType = CTRL_COMBOX;
	combox->mode = NORMALMODE;
	memset(combox->form.key, 0, INPUTKEYNUM);
	memcpy(&combox->form.pos, &pos, sizeof(Point));
	while (text[i][0] != 0) { //找出最长的字符串
		len = strlen(text[i]);
		if (len > maxlen)
			maxlen = len;
		i++;
		if (i >= TEXTLEN_X)
			break;
	}
	if (maxlen > 0)
		combox->form.rect = getrect(pos, FONTSIZE * maxlen, FONTSIZE);
	memcpy(combox->text, text, TEXTLEN_X*TEXTLEN_Y);
	combox->form.ret_flg = ret_flg;
	combox->cur_index = default_index;
	combox->form.pfun_process = combox_process;
	combox->form.pfun_show = combox_show;
	combox->form.c_num = 1;
	if (parent != NULL){
		//dbg_prt("\n parent!=NULL");
		combox->form.node.parent = parent;
		list_init(&combox->form.node);
		list_add_tail(parent, &combox->form.node);
	}
}
void combox_show(void* form)
{
	Combox *combox=(Combox*)form;
	gui_textshow(combox->text[combox->cur_index], combox->form.pos, LCD_NOREV);
}
void combox_process(void* form)
{
	Combox *combox=(Combox*)form;
	INT8U first_flg = 0;
	Rect rect;
	int count = 0, i = 0, index = 0, len =0, maxlen=0;
	for (i = 0; i < TEXTLEN_X; i++) {
		if (combox->text[i][0] != 0){
			len = strlen(combox->text[i]);
			if (len > maxlen)
				maxlen = len;
			count++;
		}
	}
	if (count == 0)
		return;
	PressKey = NOKEY;
	index = combox->cur_index;
//	dbg_prt("\n maxlen=%d", maxlen);
	while (g_LcdPoll_Flag == LCD_NOTPOLL) {
		switch (PressKey) {
		case UP:
			index--;
			if (index < 0)
				index = count-1;
			break;
		case DOWN:
			index++;
			if (index >= count)
				index = 0;
			break;
		case ESC:
			break;
		}
		if (PressKey == UP ||PressKey == OK || PressKey == DOWN || PressKey == ESC || first_flg == 0) {
			memset(&rect, 0, sizeof(Rect));
			rect = getrect(combox->form.pos, maxlen*FONTSIZE, 2*FONTSIZE);
			gui_clrrect(rect);
			combox->cur_index = index;
			combox->form.key[0].c = index;
			if(PressKey != ESC || first_flg == 0){
				gui_textshow(&combox->text[index][0], combox->form.pos, LCD_REV);
				if(PressKey==OK){
					combox->form.pfun_process_ret = OK;
//					fprintf(stderr,"\n combox_process--------------------------OK");
					return;
				}
			}else{
				gui_textshow(&combox->text[index][0], combox->form.pos, LCD_NOREV);
				combox->form.pfun_process_ret = ESC;
//				fprintf(stderr,"\n combox_process--------------------------ESC");
				return;
			}
			first_flg = 1;
		}
		PressKey = NOKEY;
		delay(50);
	}
}

//初始化button控件
void button_init(Button *btn, char *name, Point pos, char type, struct list *parent)
{
	btn->form.focus = NOFOCUS;
	memset(btn, 0, sizeof(Edit));
	btn->form.CtlType = type;
	memcpy(btn->name, name, strlen((char*) name));
	memcpy(&btn->form.pos, &pos, sizeof(Point));
	btn->form.rect = gui_getstrrect((INT8U*) name, pos);
	//gui_textshow(name, pos, LCD_NOREV);
	if (parent != NULL){
		btn->form.node.parent = parent;
		list_init(&btn->form.node);
		list_add_tail(parent, &btn->form.node);
	}
	btn->form.pfun_process = button_process;
	btn->form.pfun_show = button_show;
}

//显示按钮
void button_show(void* form)
{
	Button *btn=(Button*)form;
	gui_textshow(btn->name, btn->form.pos, LCD_NOREV);
}
void button_process(void* form)
{
	Button *btn=(Button*)form;
	if(btn->form.CtlType==CTRL_BUTTON_OK)
		btn->form.key[0].c = ACK;
	else if(btn->form.CtlType==CTRL_BUTTON_CANCEL)
		btn->form.key[0].c = CAN;
	else
		btn->form.key[0].c = 0;
}

void edittime_show(void* form)
{
	int i;
	Rect rect;
	Point pos;
	Edit *edit=(Edit*)form;
	memcpy(&rect, &edit->form.rect, sizeof(Rect));
	rect.top += 3;
	rect.bottom += 4;
	if(edit->frame_flg==1)
		gui_rectangle(gui_changerect(rect, 4));
	for (i = 0; i < edit->form.c_num; i++)
		gui_charshow(edit->form.key[i].c, edit->form.key[i].pos, LCD_NOREV);
	gui_setpos(&pos, edit->form.pos.x+FONTSIZE*4+2, edit->form.pos.y);
	gui_charshow('-', pos, LCD_NOREV);
	if(edit->form.c_num<=6)
		return;
	gui_setpos(&pos, edit->form.pos.x+FONTSIZE*7+2, edit->form.pos.y);

	gui_charshow('-', pos, LCD_NOREV);
	if(edit->form.c_num<=8)
		return;
	gui_setpos(&pos, edit->form.pos.x+FONTSIZE*10+2, edit->form.pos.y);
	gui_charshow(' ', pos, LCD_NOREV);
	gui_setpos(&pos, edit->form.pos.x+FONTSIZE*13+2, edit->form.pos.y);
	gui_charshow(':', pos, LCD_NOREV);
	gui_setpos(&pos, edit->form.pos.x+FONTSIZE*16+2, edit->form.pos.y);
	gui_charshow(':', pos, LCD_NOREV);
}
//初始化editip控件 c_num 输入的位数  text edit显示的内容  是否有边框 frame_flg
void edittime_init(Edit *edit, char type, char *text, int c_num, Point pos, char frame_flg,
		char ret_flg, struct list *parent)
{
	int i, i_pos=0;
	Rect rect;
	memset(edit, 0, sizeof(Form));
	edit->form.CtlType = type;
	edit->mode = NORMALMODE;
	memcpy(&edit->form.pos, &pos, sizeof(Point));
	if(text==NULL)
		return;
	//初始化key
	for (i = 0; i < INPUTKEYNUM; i++) {
		if (text[0] == 0 )
			edit->form.key[i].c = '0';
		else
			edit->form.key[i].c = text[i];
		edit->form.key[i].index.index_x = i;
		if(i>=4 && i<=5)
			i_pos = i+1;
		else if(i>=6 && i<=7)
			i_pos = i+2;
		else if(i>=8 && i<=9)
			i_pos = i+3;
		else if(i>=10 && i<=11)
			i_pos = i+4;
		else if(i>=12 && i<=13)
			i_pos = i+5;
		else
			i_pos = i;
		edit->form.key[i].pos.x = edit->form.pos.x + FONTSIZE * i_pos + 2;
		edit->form.key[i].pos.y = edit->form.pos.y;
	}
	edit->keyboard_style = KEYBOARD_DEC;
	edit->form.ret_flg = ret_flg;
	edit->frame_flg = frame_flg;
	edit->form.c_num = c_num;
	if(edit->form.c_num==6)
		edit->form.rect = getrect(pos, FONTSIZE * (edit->form.c_num+1)+2, FONTSIZE);
	else if(edit->form.c_num==8)
		edit->form.rect = getrect(pos, FONTSIZE * (edit->form.c_num+2)+2, FONTSIZE);
	else
		edit->form.rect = getrect(pos, FONTSIZE * (edit->form.c_num+5)+1, FONTSIZE);
	memcpy(&rect, &edit->form.rect, sizeof(Rect));
	edit->form.pfun_process = edit_process;
	edit->form.pfun_show = edittime_show;
	if (parent != NULL ){
		edit->form.node.parent = parent;
		list_init(&edit->form.node);
		list_add_tail(parent, &edit->form.node);
	}
}


void editip_show(void* form)
{
	int i;
	Rect rect;
	Point pos;
	Edit *edit=(Edit*)form;
	memcpy(&rect, &edit->form.rect, sizeof(Rect));
	rect.top += 3;
	rect.bottom += 4;
	if(edit->frame_flg==1)
		gui_rectangle(gui_changerect(rect, 4));
	for (i = 0; i < edit->form.c_num; i++)
		gui_charshow(edit->form.key[i].c, edit->form.key[i].pos, LCD_NOREV);
	gui_setpos(&pos, edit->form.pos.x+FONTSIZE*3, edit->form.pos.y);
	gui_charshow('.', pos, LCD_NOREV);
	gui_setpos(&pos, edit->form.pos.x+FONTSIZE*7, edit->form.pos.y);
	gui_charshow('.', pos, LCD_NOREV);
	gui_setpos(&pos, edit->form.pos.x+FONTSIZE*11, edit->form.pos.y);
	gui_charshow('.', pos, LCD_NOREV);
}
//初始化editip控件 c_num 输入的位数  text edit显示的内容  是否有边框 frame_flg
void editip_init(Edit *edit, char type, char *text, int c_num, Point pos, char frame_flg,
		char ret_flg, struct list *parent, char keyboard_style)
{
	int i, i_pos=0;
	Rect rect;
	memset(edit, 0, sizeof(Edit));
	edit->form.CtlType = type;
	edit->mode = NORMALMODE;
	memcpy(&edit->form.pos, &pos, sizeof(Point));
	if(text==NULL)
		return;
	//初始化key
	for (i = 0; i < INPUTKEYNUM; i++) {
		if (text[0] == 0 )
			edit->form.key[i].c = '0';
		else
			edit->form.key[i].c = text[i];
		edit->form.key[i].index.index_x = i;
		if(i>=3 && i<=5)
			i_pos = i+1;
		else if(i>=6 && i<=8)
			i_pos = i+2;
		else if(i>=9 && i<=11)
			i_pos = i+3;
		else
			i_pos = i;
		edit->form.key[i].pos.x = edit->form.pos.x + FONTSIZE * i_pos;
		edit->form.key[i].pos.y = edit->form.pos.y;
	}
	edit->keyboard_style = keyboard_style;
	edit->form.ret_flg = ret_flg;
	edit->frame_flg = frame_flg;
	edit->form.c_num = 12;
	edit->form.rect = getrect(pos, FONTSIZE * (edit->form.c_num+3), FONTSIZE);
	memcpy(&rect, &edit->form.rect, sizeof(Rect));
	edit->form.pfun_process = edit_process;
	edit->form.pfun_show = editip_show;
	if (parent != NULL ){
		edit->form.node.parent = parent;
		list_init(&edit->form.node);
		list_add_tail(parent, &edit->form.node);
	}
}

