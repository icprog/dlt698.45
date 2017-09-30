/*********************************
 * 实现集中器在非轮显状态时,
 * 与用户的交互信息
 *********************************/

#include <mqueue.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <basedef.h>
#include "comm.h"
#include "lcd_menu.h"
#include "lcd_ctrl.h"
#include "lcdprt_jzq.h"
#include "mutils.h"
#include "gui.h"
#include "show_ctrl.h"
#include "../libMq/libmmq.h"
#include "../libBase/PublicFunction.h"

#pragma message("\n\n************************************\n CCTT_I__Compiling............\n************************************\n")
extern ProgramInfo* p_JProgramInfo ;

/*
 * menu中的元素<!"必须"!>按照如下规则排列:
 * 		如果一个菜单A是菜单B的子菜单, 那么A必须在B的后面,
 * 且A与B之间或者为B的其他子菜单,  或者没有其他项.
 * */
Menu menu[]={//必须是一级菜单，然后二级菜单。。。。
	//一级菜单
	//level,    name,   		fun, 				ispasswd			pthis,
{{level0,"     ",		NULL, 				MENU_NOPASSWD},		NULL},
	{{level1,"测量点数据显示", 	NULL, 				MENU_NOPASSWD},		NULL},
		//二级菜单 测量点数据显示子菜单
		{{level2,"1.实时数据",	menu_showclddata, 	MENU_NOPASSWD},		NULL},
		{{level2,"2.日 数 据", 	menu_showdaydata, 	MENU_NOPASSWD},		NULL},//0
		{{level2,"3.月 数 据", 	menu_showmonthdata, MENU_NOPASSWD},		NULL},//0
	{{level1,"参数设置与查看", 	NULL, 				MENU_NOPASSWD},		NULL},
		//二级菜单 参数设置与查看子菜单
		{{level2,"1.通信通道设置", 	NULL, 				MENU_ISPASSWD_EDITMODE},	NULL},
			////三级菜单 通信通道设置子菜单
			{{level3,"1.通信方式设置", 	NULL, 		MENU_NOPASSWD},		NULL},
				{{level4,"1.以太网通信方式",   menu_set_nettx,        MENU_NOPASSWD},     NULL},//1111
				{{level4,"2.无线通信方式",     menu_set_wlantx,       MENU_NOPASSWD },    NULL},//1111
			{{level3,"2.短信中心", 	menu_jzqtelephone, 	MENU_NOPASSWD},		NULL},//111
			{{level3,"3.主站通信参数", 	NULL, 		MENU_NOPASSWD},		NULL},//1
				{{level4,"1.以太网通信参数",	menu_netmaster,		MENU_NOPASSWD}},//1111
				{{level4,"2.无线通信参数",		menu_wlanmaster,	MENU_NOPASSWD}},//1111
			{{level3,"4.本地以太网配置", 	menu_eth0para, 		MENU_NOPASSWD},		NULL},//11
		{{level2,"2.电表参数设置", 	NULL, 	MENU_ISPASSWD_EDITMODE},	NULL},
			{{level3,"1.修改测量点", 		menu_jzqsetmeter,	MENU_ISPASSWD},	NULL},//11
			{{level3,"2.添加测量点", 		menu_jzqaddmeter,	MENU_ISPASSWD},	NULL},//11
			{{level3,"3.删除测量点", 		menu_jzqdelmeter,	MENU_ISPASSWD},	NULL},//11
		{{level2,"3.集中器时间设置",	menu_jzqtime, 		MENU_NOPASSWD},		NULL},//11
		{{level2,"4.界面密码设置",		menu_setpasswd, 	MENU_NOPASSWD},		NULL},//11
		{{level2,"5.集中器地址设置", 		jzq_id_edit, 				MENU_ISPASSWD_EDITMODE},		NULL},//111
	{{level1,"终端管理与维护", 	NULL, 				MENU_NOPASSWD},		NULL},
		//二级菜单 终端管理与维护子菜单
		{{level2,"1.终端版本", 	menu_jzqstatus, 	MENU_NOPASSWD},		NULL},//11
		{{level2,"2.终端数据", 	NULL, 				MENU_NOPASSWD},		NULL},
			{{level3,"1.遥信状态", 	menu_yxstatus, 		MENU_NOPASSWD},		NULL},
			{{level3,"2.时钟电池", 	menu_rtcpower, 		MENU_NOPASSWD},		NULL},//0
//			////三级菜单 集中器数据子菜单
		{{level2,"3.终端管理", 	NULL, 				MENU_NOPASSWD},		NULL},
			////三级菜单 终端管理子菜单
			{{level3,"1.终端重启", 	menu_jzqreboot, 	MENU_ISPASSWD},		NULL},//111
			{{level3,"2.数据初始化", 	menu_initjzqdata, 	MENU_ISPASSWD},		NULL},
			{{level3,"3.事件初始化", 	menu_initjzqevent, 	MENU_ISPASSWD},		NULL},
			{{level3,"4.需量初始化", 	menu_initjzqdemand, 	MENU_ISPASSWD},		NULL},
			{{level3,"5.恢复出厂设置",menu_FactoryReset, 	MENU_ISPASSWD},		NULL},
		{{level2,"4.现场调试", 	NULL, 				MENU_NOPASSWD},		NULL},
		////三级菜单 现场调试子菜单
			{{level3,"1.本地IP设置",	menu_termip, 		MENU_NOPASSWD},		NULL},//111
			{{level3,"2.GPRSIP查看",	menu_gprsip, 		MENU_NOPASSWD},		NULL},//111
			{{level3,"3.液晶对比度", 	menu_lcdcontrast, 	MENU_NOPASSWD},		NULL},
			{{level3,"4.交采芯片信息",menu_ac_info,		MENU_NOPASSWD},		NULL},
			{{level3,"5.规约切换",menu_ProtocolChange,		MENU_NOPASSWD},NULL},
		{{level2,"5.页面设置", 	menu_pagesetup, 				MENU_NOPASSWD},		NULL},
		{{level2,"6.手动抄表", 	NULL, 				MENU_NOPASSWD},		NULL},
			{{level3,"1.根据表序号抄表", menu_readmeterbycldno, 	MENU_NOPASSWD},	NULL},
			{{level3,"2.根据表地址抄表",menu_readmeterbycldaddr,MENU_NOPASSWD},	NULL},
		{{level2,"7.485II设置", 	 menu_set485II , MENU_NOPASSWD},		NULL},
		{{level2,"8.载波管理",	NULL, 				MENU_NOPASSWD},		NULL},
//		/////三级菜单 载波抄表子菜单
			{{level3,"1.重新抄表", 	menu_zb_begin, 		MENU_NOPASSWD},		NULL},
			{{level3,"2.暂停抄表",	menu_zb_stop,		MENU_NOPASSWD},		NULL},
			{{level3,"3.恢复抄表",	menu_zb_resume,		MENU_NOPASSWD},		NULL},
};//测量点数据显示

#define GUI_MSG_MAXLEN 4096
extern LunXian_t LunXian[LunPageNum];
extern Proxy_Msg* p_Proxy_Msg_Data;

extern INT8U g_chgOI4510;
extern INT8U g_chgOI4001;

TS Tcurr_tm_his;
int g_PressKey_old;//用于液晶点抄 半途退出


int getMenuSize_jzq(){
	return sizeof(menu)/sizeof(Menu);
}

void show_realdata(int pindex, LcdDataItem *item, int itemcount){
	int pageno=0;
	CLASS_6001 meter ;
	memset(&meter,0,sizeof(CLASS_6001));
	if(pindex <=0)
		return;
	readParaClass(0x6000,&meter,pindex);
	if(meter.basicinfo.port.OI == PORT_JC){
		PressKey = NOKEY;
		while(g_LcdPoll_Flag==LCD_NOTPOLL){
			switch(PressKey)
			{
			case LEFT:
			case UP:
				pageno--;
				if(pageno<0)
					pageno = JCPageNum-1;
				break;
			case RIGHT:
			case DOWN:
				pageno++;
				if(pageno>JCPageNum-1)
					pageno = 0;
				break;
			case ESC:
				return;
			}
			gui_clrrect(rect_Client);
			ShowJCData(p_JProgramInfo,pageno, item, itemcount, 2);
			PressKey = NOKEY;
			delay(200);
		}
	}
	else
	{
		gui_clrrect(rect_Client);
		ShowCLDDataPage(item, itemcount, 2);
		PressKey = NOKEY;
		while(g_LcdPoll_Flag==LCD_NOTPOLL){
			if(PressKey==ESC)
				return;
			PressKey = NOKEY;
			delay(300);
		}
	}
	return;
}
int message_send_data(mmq_head mq_h,mqd_t mqd_mr,unsigned prio_t,INT8U *sendbuf)
{
	int cnt;
	cnt=mmq_put(mqd_mr, 1, mq_h,(INT8S *)sendbuf,prio_t);
	if(cnt >=0){
//		printf("\n send message to vmain program ok!\n");
		return 1;
	}else{
		return 0;
	}
}

mqd_t createMsg(INT8S *mq_name, INT8U flg){
	struct mq_attr attr;
	mqd_t mqd = 0;
	mqd = mmq_open(mq_name, &attr, flg);//O_RDONLY);
	//dbg_prt("\nmqd=%d\n",mqd);
	if(mqd<0){
//		dbg_prt( "\nGUI error mmq_open[%d]:%s error:%s  msg_num=%ld/%ld[%ld] msg_flg=%ld",
//				mqd, mq_name, strerror(errno), attr.mq_curmsgs,attr.mq_maxmsg,attr.mq_msgsize, attr.mq_flags);
		return -1;
	}
	return mqd;
}
void colseMsg(mqd_t mqd){
	if(mqd>0)
		mmq_close(mqd);
}
//发消息  返回<0 失败  返回>0 消息句柄 成功
int sendMsg(mqd_t mqd, INT32U cmd, INT8S *sendbuf, INT32U bufsiz)
{
	int ret;
	mmq_head mqh;
	mqh.pid = cjgui;
	mqh.cmd = cmd;
	mqh.bufsiz = bufsiz;
	ret = mmq_put(mqd, 1, mqh, sendbuf, 0);
	if (ret<0){
//		dbg_prt( "\nGUI error mmq_put:%d error:%s ret=%d",
//				mqd, strerror(errno),ret);
		return -2;
	}
//	else
//		DEBUG_TIME_LINE( "\n GUI ---->zb ret=%d",ret);
	return ret;
}

int recvMsg(mqd_t mqd, mmq_head *mqh, INT8U *recvbuf, int timeout)
{
	int mmq_ret=0;
	mmq_ret = mmq_get(mqd, timeout, mqh, recvbuf);
	if(mmq_ret<0){
//		dbg_prt("\n GUI error mmq_get:%d error:%s ret=%d ",
//				(int)mqd, strerror(errno), mmq_ret);
		return mmq_ret;
	}
	return mmq_ret;
}

//int build_dataflg(int pindex, int pdid[], int pdid_size, RealDataFlagInfo *realdataflag){
//	int i, count=0;
//	if(pindex <=0)
//		return 0;
//	for(i=0; i<pdid_size; i++){
//		realdataflag[i].mNo = ParaAll->f10.para_mp[pindex - 1].MPNo;
//		realdataflag[i].id = pdid[i];
//		count++;
//	}
//	return count;
//}

//手动抄表

int requestdata(int cldno, INT8S *req_mq_name, int cmd, \
		int arr_did[], int arr_size, int arr_idata, int arr_iend, int timeout, INT8U *msgbuf)
{
//#ifndef FB_SIM
//	INT8U msgbuf_tmp[GUI_MSG_MAXLEN];
//	g_PressKey_old = NOKEY;
//	Point pos;
//	RealDataFlagInfo realdataflag[100];
//	memset(realdataflag, 0, 100*sizeof(RealDataFlagInfo));
//	char str[50];
//	INT32S		dataflg_count=0, time_count=0;
//	mmq_head	mq_h;
//	mqd_t mqd, mqd_gui;
//	memset(&mq_h, 0, sizeof(mmq_head));
//	if(arr_did!=NULL && arr_size!=0){
//		if(cldno==0){//液晶提示无此测量点
//			dbg_prt("\n cldno==0 error!");
//			return 0;
//		}
//		realdataflag[0].idata = arr_idata;
//		realdataflag[0].iend = arr_iend;
//		dataflg_count = build_dataflg(cldno, arr_did, arr_size, realdataflag);
//	}
//	g_curcldno = cldno;
//	mqd =  createMsg(req_mq_name, O_WRONLY);
////TODO:创建消息队列
////	mqd_gui = createMsg((INT8S*)GUI_REV_MQ, O_RDONLY);
//	//dbg_prt( "\n GUI requestdata:req_mq_name=%s mqd_request=%d mqd_gui=%d", req_mq_name, mqd,mqd_gui);
//	if(mqd<0){
//		dbg_prt( "\nGUI createMsg %s  error!!2  mqd=%d",req_mq_name, mqd);
//		return 0;
//	}
//	if(mqd_gui<0){
//		dbg_prt( "\nGUI createMsg %s  error!!2  mqd=%d",GUI_REV_MQ, mqd_gui);
//		return 0;
//	}
//	if(arr_did!=NULL && arr_size!=0){//正常的测量点
//		while(recvMsg(mqd_gui, &mq_h, msgbuf_tmp, 1)<=0){
//			dbg_prt( "\nGUI Message has existed in recv-queqe!!!");
//			break;
//		}
//		memset(&mq_h, 0, sizeof(mmq_head));
//		//正向有功总电能
//		memcpy(msgbuf, &realdataflag, dataflg_count*sizeof(RealDataFlagInfo));
//		if(sendMsg(mqd, cmd, (INT8S*)msgbuf, dataflg_count*sizeof(RealDataFlagInfo))<0){
//			dbg_prt( "\nGUI sendmsg %s  error!!2  mqd=%d",req_mq_name, mqd);
//			colseMsg(mqd);
//			colseMsg(mqd_gui);
//			return 0;
//		}
//	}else{//交采
//		memset(msgbuf, 0, GUI_MSG_MAXLEN);
//		if(sendMsg(mqd, cmd, (INT8S *)msgbuf, 200)<0){
//			colseMsg(mqd);
//			colseMsg(mqd_gui);
//			dbg_prt( "\n GUI rev_mq:mq_open_ret=%d  error!!1\n",mqd);
//			return 0;
//		}
//		timeout = 0;//请求交采的超时时间为0,不显示"正在读取数据...%d" 字样
//	}
//	memset(msgbuf, 0, GUI_MSG_MAXLEN);
//	PressKey = NOKEY;
//	while(1){
//		if(PressKey!=NOKEY)
//			g_PressKey_old = PressKey;
//		delay(300);
//		time_count++;
//		if(timeout>2){
//			if(time_count>timeout)//60秒超时
//				break;
//			gui_clrrect(rect_Client);
//			memset(str, 0, 50);
//			sprintf(str, "正在读取数据...%d", time_count);
//			//sprintf(str, "正在读取数据...");
//			gui_setpos(&pos, rect_Client.left+5*FONTSIZE, rect_Client.top+8*FONTSIZE);
//			gui_textshow(str, pos, LCD_NOREV);
//		}else{
//			if(time_count>1)
//				break;
//		}
//		dbg_prt( "\n GUI cmd=%d time_count=%d",cmd, time_count);
//		if(recvMsg(mqd_gui, &mq_h, msgbuf, 1)>0){
//			if(mq_h.cmd == (INT32U)cmd){
//				dbg_prt("  OK");
//				break;
//			}
//		}
//		if(PressKey!=NOKEY)
//			g_PressKey_old = PressKey;
//		if(g_PressKey_old == ESC)
//			break;
//		//PressKey = NOKEY;
//	}
//	colseMsg(mqd);
//	colseMsg(mqd_gui);
//	g_curcldno = 1;
//	return mq_h.bufsiz;
//#endif
	return 0;
}

int requestDataSingle(INT32U cldno, INT8S *req_mq_name, INT32U cmd, \
		INT32U arr_did,INT32U arr_idata, INT32U arr_iend, INT32U timeout, INT8U *msgbuf)
{//此函数用来处理单独的数据项
//#ifndef FB_SIM
//	INT8U msgbuf_tmp[GUI_MSG_MAXLEN];
//	g_PressKey_old = NOKEY;
//	Point pos;
//	RealDataFlagInfo realdataflag;//发给485进程的点抄结构体
//	char str[50];
//	INT32S	time_count=0;
//	mmq_head	mq_h;
//	mqd_t mqd, mqd_gui;
//
//	memset(&mq_h, 0, sizeof(mmq_head));
//	memset(&realdataflag,0,sizeof(RealDataFlagInfo));
//	bzero(msgbuf_tmp,sizeof(GUI_MSG_MAXLEN));
//
//	if(cldno <= 1 || req_mq_name == NULL || arr_did > 3000 || msgbuf == NULL)
//	{//
//		dbg_prt("\npara error");
//		return 0;
//	}
//	realdataflag.idata = arr_idata;
//	realdataflag.iend = arr_iend;
//	realdataflag.mNo = cldno;
//	realdataflag.id = arr_did;
//
//	realdataflag.startflag = 1;
//	realdataflag.msg_index = 0;
//	realdataflag.msg_num = 1;
//	realdataflag.group_index = 0;
//
//	g_curcldno = cldno;//用于液晶顶部状态栏测量点显示
//	if((mqd =  createMsg(req_mq_name, O_WRONLY)) < 0)
//	{
//		dbg_prt( "\nGUI createMsg %s  error!!2  mqd=%d",req_mq_name, mqd);
//		return 0;
//	}
//	if((mqd_gui = createMsg((INT8S*)GUI_REV_MQ, O_RDONLY)) < 0)
//	{
//		dbg_prt( "\nGUI createMsg %s  error!!2  mqd=%d",GUI_REV_MQ, mqd_gui);
//		return 0;
//	}
//	while(recvMsg(mqd_gui, &mq_h, msgbuf_tmp, 1) > 0)
//	{
//		dbg_prt( "\nGUI Message has existed in recv-queqe!!!");
//		//break;
//	}
//	memset(&mq_h, 0, sizeof(mmq_head));
//	memcpy(msgbuf, &realdataflag, sizeof(RealDataFlagInfo));
//	if(sendMsg(mqd, cmd, (INT8S*)msgbuf, sizeof(RealDataFlagInfo))<0)
//	{
//		dbg_prt( "\nGUI sendmsg %s  error!!2  mqd=%d",req_mq_name, mqd);
//		colseMsg(mqd);
//		colseMsg(mqd_gui);
//		return 0;
//	}
//	memset(msgbuf, 0, GUI_MSG_MAXLEN);
//	PressKey = NOKEY;
//	while(1)
//	{
//		if(PressKey!=NOKEY)
//			g_PressKey_old = PressKey;
//			delay(600);
//			time_count++;
//			if(timeout > 2)
//			{
//				if(time_count > timeout)//60秒超时
//					break;
//				gui_clrrect(rect_Client);
//				memset(str, 0, 50);
//				sprintf(str, "正在读取数据...%d", time_count);
//				gui_setpos(&pos, rect_Client.left+5*FONTSIZE, rect_Client.top+8*FONTSIZE);
//				gui_textshow(str, pos, LCD_NOREV);
//			}
//			else
//			{
//				if(time_count>1)
//					break;
//			}
//			dbg_prt( "\n GUI cmd=%d time_count=%d",cmd, time_count);
//			if(recvMsg(mqd_gui, &mq_h, msgbuf, 1) > 0){
//				if(mq_h.cmd == (INT32U)cmd)
//				{
//					dbg_prt("OK");
//					break;
//				}
//			}
//			if(PressKey!=NOKEY)
//				g_PressKey_old = PressKey;
//			if(g_PressKey_old == ESC)
//				break;
//	}
//	colseMsg(mqd);
//	colseMsg(mqd_gui);
//	g_curcldno = 1;
//	return mq_h.bufsiz;//返回实际接收的报文长度
//#endif
	return 0;
}

int requestDataBlock(CLASS_6001* cldno, INT8S *req_mq_name, INT32U cmd, int msg_num,INT32U timeout, INT8U *msgbuf)
{
	INT8U result = 0;
	INT32U msgPos,retMsgSize,msgNum;
	msgPos = retMsgSize = msgNum = 0;
	INT8U msgbuf_tmp[GUI_MSG_MAXLEN];
	g_PressKey_old = NOKEY;
	Point pos;
	Proxy_Msg msg_real;//发给抄表进程的点抄结构体
	char str[50];
	INT32S time_count=0;
	mmq_head	mq_h;
	mqd_t mqd;
	memset(&mq_h, 0, sizeof(mmq_head));
	memset(&msg_real, 0,sizeof(Proxy_Msg));
	bzero(msgbuf_tmp,sizeof(msgbuf_tmp));
	fprintf(stderr,"\n----------addr0=%d-----------req_mq_name =%p   msgbuf=%p\n",cldno->basicinfo.addr.addr[0],req_mq_name ,msgbuf);
	if(cldno->basicinfo.addr.addr[0] <= 0 || req_mq_name == NULL || msgbuf == NULL)
	{
		fprintf(stderr,"\n-----------return ");
		return 0;
	}


	if((mqd =  createMsg(req_mq_name, O_WRONLY)) <0)
	{
		fprintf(stderr,"\nreqmqname = %s",req_mq_name);
		return 0;
	}
	memset(&mq_h, 0, sizeof(mmq_head));
	memcpy(msg_real.addr.addr,cldno->basicinfo.addr.addr,cldno->basicinfo.addr.addr[0]+1);
	memcpy(&msg_real.port ,&cldno->basicinfo.port,sizeof(cldno->basicinfo.port));
	msg_real.baud = cldno->basicinfo.baud;
	msg_real.protocol = cldno->basicinfo.protocol;
	msg_real.oi = 0x0010;//TODO:698正向有功电能示值标识


	if(sendMsg(mqd,PROXY,(INT8S*)&msg_real,sizeof(msg_real)) < 0)//TODO:发送消息pid
	{
		fprintf(stderr,"\n液晶消息发送失败");
		colseMsg(mqd);
		return 0;
	}
	fprintf(stderr,"\n00000000000000000000000000\n");
	memset(msgbuf_tmp, 0, sizeof(msgbuf_tmp));
	PressKey = NOKEY;
	time_t nowtime = time(NULL);
	time_t waittime = time(NULL);
	while(1)
	{
		if(PressKey!=NOKEY)
			g_PressKey_old = PressKey;
		delay(600);
		nowtime = time(NULL);
		time_count = nowtime-waittime;
//		time_count++;
		if(timeout > 2)
		{
			if(time_count>timeout || msgNum >= msg_num){//60秒超时
				result = 0;
				break;
			}
			gui_clrrect(rect_Client);
			memset(str, 0, 50);
			sprintf(str, "正在读取数据...%d", time_count);
			gui_setpos(&pos, rect_Client.left+5*FONTSIZE, rect_Client.top+8*FONTSIZE);
			gui_textshow(str, pos, LCD_NOREV);
		}
		else
		{
			if(time_count>1){
				result = 0;
				break;
			}
		}
		if(1 == p_Proxy_Msg_Data->done_flag)
		{
			//数据写入全局变量成功，跳出
			DEBUG_TIME_LINE("\ngui: -------------cur p_Proxy_Msg_Data->done_flag = %d\n",p_Proxy_Msg_Data->done_flag);
			result = 1;
			break;
		}
		if(PressKey!=NOKEY)
			g_PressKey_old = PressKey;
	}
	colseMsg(mqd);
	return result;
}

int requestdata_ACS(int cmd, LcdDataItem *item){//请求交采值和电能量
//	INT8U msgbuf[GUI_MSG_MAXLEN];
//	CurrentData_SAVE  tmprealdatainfo;
//	int size=0, mq_cnt=0, i, cldno=1;
//	cldno = gui_GetJCMP();
//	size = requestdata(cldno,(INT8S*)ACS_REALDATA_MQ, cmd, NULL,0,0,0,2,msgbuf);
//	if(size>0){
//		mq_cnt = size/sizeof(CurrentData_SAVE);
//		for(i=0; i<mq_cnt; i++){
//			memcpy(&tmprealdatainfo, &msgbuf[i*sizeof(CurrentData_SAVE)],sizeof(CurrentData_SAVE));
//			item[i].index = i;
//			item[i].dataflg_id = tmprealdatainfo.id_d;
////			if(tmprealdatainfo.id_d==36||tmprealdatainfo.id_d==37||tmprealdatainfo.id_d==38)
////			dbg_prt("\n gui tmprealdatainfo.id_d=%d:%f",
////					tmprealdatainfo.id_d, bcd2double(tmprealdatainfo.data,sizeof(tmprealdatainfo.data),2,positive));
//			memcpy(item[i].val, tmprealdatainfo.data, LcdDataItem_VALLEN);
//		}
//	}
//	return mq_cnt;
	return 0;
}

int requestdata_485_ZB(int cldno, INT8U *mq_name, int arr_did[], int arr_size, LcdDataItem *item){
//	INT8U msgbuf[GUI_MSG_MAXLEN];
//	RealData_DianChao tmprealdatainfo;
////	Pname_DMCol_Strt *pdm;//did数据项id+name数据项名称
//	int size=0, mq_cnt=0, i=0, j;
//	unsigned int k;
//	for(j = 0; j < arr_size; j++)
//	{
//		size = requestdata(cldno,(INT8S*)mq_name, READPOINT, &arr_did[j], 1,  j, arr_size-1, 60, msgbuf);
//		if(size>0){
//			mq_cnt += size/sizeof(RealData_DianChao);
//			memcpy(&tmprealdatainfo, &msgbuf[0],sizeof(RealData_DianChao));
//			if(tmprealdatainfo.id_mp == cldno){
//				DEBUG_TIME_LINE("\n Dianchao:");
//				for(k = 0; k < sizeof(tmprealdatainfo.data); k++)
//					DEBUG_TIME_LINE(" %02x",tmprealdatainfo.data[k]);
//				DEBUG_TIME_LINE("\n");
//				for(i=0; i<5; i++){
//					item[i].index = i;
//					item[i].dataflg_id = arr_did[j] + i;
//					//dbg_prt("tmprealdatainfo.realhead.id = %d\n",tmprealdatainfo.id_d+i);
//					memcpy(item[i].val, &tmprealdatainfo.data[7*i], LcdDataItem_VALLEN);
//				}
//			}
//		}
//		if(g_PressKey_old == ESC)
//			break;
//		g_PressKey_old = NOKEY;
//	}
//	return mq_cnt*5;
	return 0;
}
//向消息队列mq_name请求数据，返回的数据格式化为LcdDataItem结构体
//消息队列返回的是字符流，但是该字符六可以格式化为通信进程之间约定的共用结构体RealDataInfo
//再将共用结构体RealDataInfo转化为液晶显示专用的结构体LcdDataItem
//返回LcdDataItem结构体的个数

int requestdata_485_ZB_Block(CLASS_6001* cldno, INT8U *mq_name, int msg_num, LcdDataItem *item){
	//向某个进程的消息队列发送消息，请求测量点号为cldno的数据项，这些数据项存放在arr_did数组中，数组的大小为arr_size
	INT8U msgbuf[GUI_MSG_MAXLEN];
	INT8U result = 0;
	int mq_cnt=5;
	memset(msgbuf,0,sizeof(msgbuf));
//	bzero(&msg_real,sizeof(Proxy_Msg));
	fprintf(stderr,"\n==================================\n");
	result = requestDataBlock(cldno,(INT8S*)mq_name,PROXY,msg_num,60,msgbuf);
//	DEBUG_TIME_LINE("\ngui: -------------cur rev msg from 485 result = %d\n",result);
	if(result > 0)
	{
		item[0].index = 1;
		item[0].dataflg_id = PosPt_All_Id;
		memcpy(item[0].val,p_Proxy_Msg_Data->realdata.data_All,sizeof(p_Proxy_Msg_Data->realdata.data_All));

		item[1].index = 1;
		item[1].dataflg_id = PosPt_Rate1_Id;
		memcpy(item[1].val,p_Proxy_Msg_Data->realdata.Rate1_Data,sizeof(p_Proxy_Msg_Data->realdata.Rate1_Data));

		item[2].index = 1;
		item[2].dataflg_id = PosPt_Rate2_Id;
		memcpy(item[2].val,p_Proxy_Msg_Data->realdata.Rate2_Data,sizeof(p_Proxy_Msg_Data->realdata.Rate2_Data));

		item[3].index = 1;
		item[3].dataflg_id = PosPt_Rate3_Id;
		memcpy(item[3].val,p_Proxy_Msg_Data->realdata.Rate3_Data,sizeof(p_Proxy_Msg_Data->realdata.Rate3_Data));

		item[4].index = 1;
		item[4].dataflg_id = PosPt_Rate4_Id;
		memcpy(item[4].val,p_Proxy_Msg_Data->realdata.Rate4_Data,sizeof(p_Proxy_Msg_Data->realdata.Rate4_Data));
	}
	return mq_cnt;
}

int requestdata_485_ZB_Single(int cldno, INT8U *mq_name, int arr_did[], int arr_size, LcdDataItem *item){
	//向某个进程的消息队列发送消息，请求测量点号为cldno的数据项，这些数据项存放在arr_did数组中，数组的大小为arr_size
//	INT8U msgbuf[GUI_MSG_MAXLEN];
//	RealDataInfo tmprealdatainfo;
//	int size=0, mq_cnt=0, j;
//	unsigned int k;
//	bzero(msgbuf,sizeof(msgbuf));
//	for(j = 0; j < arr_size; j++)
//	{
//		size = requestDataSingle(cldno, (INT8S*)mq_name, READPOINT, arr_did[j], j, arr_size-1, 15, msgbuf);
//		dbg_prt("\nsize = %d",size);
//		if(size>0)
//		{
//			mq_cnt += size/sizeof(RealDataInfo);
//			dbg_prt("\nmq_cnt = %d",mq_cnt);
//			memcpy(&tmprealdatainfo, &msgbuf[0],sizeof(RealDataInfo));
//			if(tmprealdatainfo.realhead.mNo == cldno)
//			{
//				DEBUG_TIME_LINE("\n Dianchao:");
//				for(k = 0; k < sizeof(tmprealdatainfo.data); k++)
//					DEBUG_TIME_LINE(" %02x",tmprealdatainfo.data[k]);
//				DEBUG_TIME_LINE("\n");
//				item[j].index = j;
//				item[j].dataflg_id = arr_did[j];
//				dbg_prt("\nitem[%d].dataflg_id = %d",j,item[j].dataflg_id);
//				memcpy(item[j].val, tmprealdatainfo.data, LcdDataItem_VALLEN);
//			}
//		}
//		if(g_PressKey_old == ESC)
//			break;
//		g_PressKey_old = NOKEY;
//	}
//	return mq_cnt;
	return 0;
}

void show_realdatabycld(void * pindex){
	CLASS_6001* cur_pindex = NULL;
	cur_pindex = (CLASS_6001*)pindex;
	int mqcount=0;
	LcdDataItem item[10];//存储的所有数据项
	if(cur_pindex == NULL)
		return;
	memset(item, 0, 10*sizeof(LcdDataItem));
	if(cur_pindex->basicinfo.port.OI == PORT_485 ||cur_pindex->basicinfo.port.OI ==  PORT_ZB){
		mqcount = requestdata_485_ZB_Block(cur_pindex,(INT8U*)PROXY_485_MQ_NAME,5, item);
	}
	show_realdata(cur_pindex->sernum, item, mqcount);
	memset(p_Proxy_Msg_Data,0,sizeof(Proxy_Msg));
}

//TODO:该函数里循环中使用readParaClass（），会对系统带来负担，后期需要在参数中包含有效测量点个数等，方便使用
//TODO:该函数中不要开辟内存，复制测量点信息，用到时再开辟。
int gui_mp_compose(CLASS_6001 **ppgui_mp){
	CLASS_6001 *gui_mp=NULL;
	int i,effe_num = 0,cld_num=0;//cld_num从文件中读出当前个数，其中可能包括无效测量点，需过滤得到effe_num有效测量点个数
	CLASS_6001	 meter={};
	CLASS11		coll={};
	if(readInterClass(0x6000,&coll)==0){
		return -1;
	}
	cld_num = getFileRecordNum(0x6000);//获取文件测量点单元个数
	for(i=0;i<cld_num;i++)//循环查询有效测量点个数
	{
		if(readParaClass(0x6000,&meter,i))
			if(meter.sernum != 0 && meter.sernum != 0xFFFF)
			{
				effe_num++;
			}
	}

	//DEBUG_TIME_LINE("\n------cld_num = %d----effe_num = %d-----\n",cld_num,effe_num);
	if(effe_num<=0) return 0;
	gui_mp = (CLASS_6001*)malloc(effe_num*sizeof(CLASS_6001));
	if(gui_mp==NULL)  return -1;
	memset(gui_mp,0,effe_num*sizeof(CLASS_6001));
	*ppgui_mp = gui_mp;
	effe_num=0;
	for(i=0;i<cld_num;i++)
	{
		if(readParaClass(0x6000,&meter,i))
		{
			if(meter.sernum != 0 && meter.sernum != 0xFFFF)
			{
				if(meter.basicinfo.addr.addr[0]>TSA_LEN)//TSA地址长度异常
				{
					continue;
				}
				memcpy(&gui_mp[effe_num],&meter,sizeof(CLASS_6001));
				effe_num++;
				fprintf(stderr,"%d\n",gui_mp[i].sernum);
			}
		}
	}
	return effe_num;
}
void gui_mp_free(CLASS_6001 *gui_mp){
	if(gui_mp==NULL)
		return;
	free(gui_mp);
}
//获取当前0x6000中测量点序号，寻找最大的序号，返回最大序号加1，用以添加测量点使用
int gui_mp_getsernum()
{
	int cld_num=0,i=0,ret=0;
	CLASS_6001	 meter={};
	cld_num = getFileRecordNum(0x6000);//获取文件测量点单元个数
	for(i=0;i<cld_num;i++)//循环查询有效测量点个数
	{
		if(readParaClass(0x6000,&meter,i))
			if(meter.sernum != 0 && meter.sernum != 0xFFFF)
				if(ret<meter.sernum)
					ret = meter.sernum;
	}
	if(ret ==0 || ret == 1) return 2;
	return ret+1;
}
void showallmeter(void (*pfun)(void* mp_info))
{
	CLASS_6001* gui_mp = NULL;
	int cur_mp=1, begin_mp=1, i=0, presskey_ok_acs=NOKEY;
	Point pos;
	char first_flg=0, str_cld[50], addr[20];
	Rect rect;
	int mp_max=0;
	mp_max = gui_mp_compose(&gui_mp);
	if(mp_max<=0){
		msgbox_label((char *)"未配置测量点", CTRL_BUTTON_OK);
		return;
	}
	//TODO: add acs node
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		switch(PressKey)
		{
		case LEFT:
			if(mp_max>PAGE_COLNUM-1){
				if(begin_mp!=1 ){
					begin_mp -= PAGE_COLNUM-1;
					if(begin_mp<=0)
						begin_mp = 1;
					cur_mp = begin_mp;
				}else
					cur_mp = begin_mp = mp_max - (PAGE_COLNUM-1);
			}
			break;
		case UP:
			cur_mp--;
			if(cur_mp<=0 && mp_max>PAGE_COLNUM){
				cur_mp = mp_max;
				begin_mp = mp_max - PAGE_COLNUM + 2;
			}else if(cur_mp<=0 && mp_max<=PAGE_COLNUM){
				cur_mp = mp_max;
			}else if(cur_mp<=begin_mp)
				begin_mp = cur_mp;
			break;
		case RIGHT:
			if((begin_mp+PAGE_COLNUM-1)>=mp_max){
				cur_mp = begin_mp = 1;
			}else{
				begin_mp += PAGE_COLNUM-1;
				cur_mp = begin_mp;
			}
			break;
		case DOWN:
			cur_mp++;
			if(cur_mp>mp_max)
				begin_mp = cur_mp = 1;
			else if(cur_mp>begin_mp+PAGE_COLNUM-2)
				begin_mp++;
			break;
		case OK:
			pfun((void *)(gui_mp+cur_mp-1));
//			gui_mp_free(gui_mp);
//			mp_max = gui_mp_compose(&gui_mp);
//			if(mp_max<=0){
//				msgbox_label((char *)"未配置测量点", CTRL_BUTTON_OK);
//				return;
//			}
			break;
		case ESC:
			gui_mp_free(gui_mp);
			return;
		default:
			break;
		}
		if(PressKey!=NOKEY||first_flg==0||presskey_ok_acs==OK){
			int addr_len = 0;
			presskey_ok_acs = NOKEY;
			first_flg = 1;
			gui_clrrect(rect_Client);
			pos.x = rect_Client.left;
			pos.y = rect_Client.top+1;
			gui_textshow((char *)" 配置序号     表地址", pos, LCD_NOREV);
			for(i=begin_mp; i<begin_mp +PAGE_COLNUM-1; i++){
				if(i>mp_max)
					continue;
				memset(str_cld, 0, 50);
				memset(addr, 0, sizeof(addr));
				if((gui_mp+i-1)->sernum == 0) continue;

				addr_len = (gui_mp+i-1)->basicinfo.addr.addr[1]+1;
				bcd2str(&(gui_mp+i-1)->basicinfo.addr.addr[2],(INT8U*)addr,addr_len,sizeof(addr),positive);
				sprintf(str_cld, " %04d    %s",(gui_mp+i-1)->sernum, addr);
				pos.x = rect_Client.left;
				pos.y = rect_Client.top + (i-begin_mp+1)*FONTSIZE*2 + 2;
				gui_textshow(str_cld, pos, LCD_NOREV);
				if(i==cur_mp){
					memset(&rect, 0, sizeof(Rect));
					rect = gui_getstrrect((unsigned char*)str_cld, pos);//获得字符串区域
					gui_reverserect(rect);
				}
			}
		}
		PressKey = NOKEY;
		delay(50);
	}
	gui_mp_free(gui_mp);
}

/*
 * 显示测量点当天数据
 * cld为"测量点"的拼音头字母
*/
void menu_showclddata()
{
	showallmeter(show_realdatabycld);
}
//name 数据项名称 dataid 数据项ID len 小数点后的有效位数   位置：pos_x pos_y
//flag 1代表有数据，正常显示，0代表无数据，有xx标示
void dataitem_showvalue( char *idname,float fval,int len, int pos_x, int pos_y,char flag){
	char str[100];
	LcdDataItem lcd_data;
	memset(&lcd_data, 0, sizeof(LcdDataItem));
	memset(str, 0, 100);
	if(flag == 1)
	{
		if(len==2)
			sprintf(str,"%s % 9.2f kWh", idname, fval);
		else
			sprintf(str,"%s % 8.4f kWh", idname, fval);
	}
	else
		sprintf(str,"%s xxxxxx.xx kWh", idname);
	lcd_data.pos.x = pos_x;
	lcd_data.pos.y = pos_y;
	gui_textshow(str, lcd_data.pos, LCD_NOREV);
}
//填充csds，查询日/月冻结日期和冻结项
//FLAG =1 日冻结  =2 月冻结
void MonthAndDayFillCsds(CSD_ARRAYTYPE *csds,INT8U flag)
{
	csds->num = 0x02;//2个查询项
	csds->csd[0].type=0x00;//oad对象属性描述符
	csds->csd[0].csd.oad.OI=0x6041;//采集成功时间
	csds->csd[0].csd.oad.attflg=0x02;
	csds->csd[0].csd.oad.attrindex = 0x00;
	csds->csd[1].type=0x01;
	csds->csd[1].csd.road.num=0x01;
	if(flag == 1)
		csds->csd[1].csd.road.oad.OI = 0x5004;//日冻结对象
	else
		csds->csd[1].csd.road.oad.OI = 0x5006;//日冻结对象
	csds->csd[1].csd.road.oad.attflg = 0x02;
	csds->csd[1].csd.road.oad.attrindex = 0x00;
	csds->csd[1].csd.road.oads[0].OI = 0x0010;//正向有功电能量
	csds->csd[1].csd.road.oads[0].attflg = 0x02;
	csds->csd[1].csd.road.oads[0].attrindex = 0x00;
}
INT8S dealDateAndEnergy(INT8U *databuf,TS *cj_date,INT32U *element)
{
	INT8U ret = 1;
	INT16U i=0,j=0;;
	INT16U index=0;
	for(i=0;i<2;i++)//处理2个项
	{
		if(databuf[index] == 8)//采集成功时间
		{
			cj_date->Year = 	databuf[index+2]<<8 | databuf[index+3];//掠过一个date_time_s的标志字节
			cj_date->Month = databuf[index+4];
			cj_date->Day = databuf[index+5];
			cj_date->Hour = databuf[index+6];
			cj_date->Minute = databuf[index+7];
			cj_date->Sec = databuf[index+8];
			index+=9;
		}
		else if (databuf[index] == 27)//总及4费率
		{
			if(databuf[index+1] == 0x01 && databuf[index+2] == 0x05 )
			{
				index+=4;//略过double long unsign 标示
				for(j=0;j<5;j++)
				{
					bcd2int32u(&databuf[index],4,positive,element+j);
					index+=5;//多加一个直接是double long unsign标示
				}
			}
			else
			{
				ret = -1;
				break;
			}
		}
		else
		{
			ret = -2;
			break;
		}
	}
	return ret;
}
//flag = 1 日冻结 flag = 2 月冻结
void ReadAndShow_DayMonthData(void *mp_info,INT8U dayOrMonth){
	INT8U str[100];
	TS cj_date;
	memset(&cj_date,0,sizeof(TS));
	Point pos;
	INT16S retBuf=0;
	INT8U flag = 0;
	INT32U energyElement[5];//总及4费率
	INT8U databuf[200];//读取文件后返回的数据，包括采集成功时间和正向有功电能
	memset(databuf,0,200);
	CLASS_6001 *mp_datainfo = (CLASS_6001 *)mp_info;
	CSD_ARRAYTYPE csds;
	memset(&csds,0,sizeof(CSD_ARRAYTYPE));
	MonthAndDayFillCsds(&csds,dayOrMonth);
	//根据采集时间/测量点地址，获取冻结具体数据
	retBuf = GUI_GetFreezeData(csds,mp_datainfo->basicinfo.addr,Tcurr_tm_his,databuf);
	gui_clrrect(rect_Client);
	gui_setpos(&pos, rect_Client.left+6*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char*)"正向有功电能示值", pos, LCD_NOREV);
	if(retBuf>0 ) //注意，此处的retBuf 是GUI_GetFreezeData返回值，如果返回正确，再用dealDateAndEnergy处理数据
		retBuf = dealDateAndEnergy(databuf,&cj_date,energyElement);
	if(retBuf>0 ) flag = 1;//为0时显示xx
	dataitem_showvalue((char*)"正向有功总:", (float)(energyElement[0]/100),2,0, rect_Client.top + FONTSIZE*4,flag);
	dataitem_showvalue( (char*)"正向有功尖:",  (float)(energyElement[1]/100),2, 0, rect_Client.top + FONTSIZE*6+3,flag);
	dataitem_showvalue((char*)"正向有功峰:",  (float)(energyElement[2]/100), 2,0, rect_Client.top + FONTSIZE*8+6,flag);
	dataitem_showvalue( (char*)"正向有功平:",  (float)(energyElement[3]/100),2, 0, rect_Client.top + FONTSIZE*10+9,flag);
	dataitem_showvalue( (char*)"正向有功谷:",  (float)(energyElement[4]/100),2, 0, rect_Client.top + FONTSIZE*12+12,flag);

	memset(str, 0, 100);
	if(cj_date.Year==0|| cj_date.Month==0||cj_date.Day==0)
		sprintf((char*)str,"抄表时间 00/00/00 00:00");
	else
		sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
			cj_date.Year-2000, cj_date.Month, cj_date.Day, cj_date.Hour, cj_date.Minute);
	gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+18*FONTSIZE);
	gui_textshow((char*)str, pos, LCD_NOREV);
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		if(PressKey==ESC)
			break;
		PressKey = NOKEY;
		delay(300);
	}
}
void show_day_djdata(void *mp_info)
{
	ReadAndShow_DayMonthData(mp_info,1);//=1日冻结
}
void show_mon_djdata(void *mp_info)
{
	ReadAndShow_DayMonthData(mp_info,2);//=2日冻结
}

void menu_showdaydata(){
	int year=0, month=0, day=0;
	char s_date[20],str[5],oprmode_old=0;
	memset(s_date, 0, 20);
	TS curr_tm, curr_tm_his;
	TSGet(&curr_tm);
	tminc(&curr_tm, day_units, -1);
	sprintf(s_date, "%04d%02d%02d",curr_tm.Year,curr_tm.Month, curr_tm.Day);
	int msgbox_ret=0;
	oprmode_old = get_oprmode();
	set_oprmode(OPRMODE_MODIFY);
	msgbox_ret = msgbox_inputjzqtime(s_date, strlen(s_date));
	set_oprmode(oprmode_old);
	if(msgbox_ret == ACK){
		memset(str, 0, 5);
		memcpy(str, &s_date[0], 4);
		year = atoi(str);
		memset(str, 0, 5);
		memcpy(str, &s_date[4], 2);
		month = atoi(str);
		memset(str, 0, 5);
		memcpy(str, &s_date[6], 2);
		day = atoi(str);
		tmass(&curr_tm_his,year,month,day,0,0,0);
		memcpy(&Tcurr_tm_his,&curr_tm_his,sizeof(curr_tm_his));
		showallmeter(show_day_djdata);
	}
}

void menu_showmonthdata(){
	int year=0, month=0;
	char s_date[20], str[5],oprmode_old=0;
	memset(s_date, 0, 20);
	TS curr_tm, curr_tm_his;
	TSGet(&curr_tm);
	tminc(&curr_tm, MONTH, -1);
	sprintf(s_date, "%04d%02d", curr_tm.Year, curr_tm.Month);
	char msgbox_ret=0;
	oprmode_old = get_oprmode();
	set_oprmode(OPRMODE_MODIFY);
	msgbox_ret = msgbox_inputjzqtime(s_date, strlen(s_date));
	set_oprmode(oprmode_old);
	if(msgbox_ret == ACK){
		memset(str, 0, 5);
		memcpy(str, &s_date[0], 4);
		year = atoi(str);
		memset(str, 0, 5);
		memcpy(str, &s_date[4], 2);
		month = atoi(str);
		tmass(&curr_tm_his,year,month,1,0,0,0);//月冻结都是放在月初第一天，此处将日期置1
		memcpy(&Tcurr_tm_his,&curr_tm_his,sizeof(curr_tm_his));
		showallmeter(show_mon_djdata);
	}
}
//返回控件的输入值 就是把form.key[i].c组合成一个字符串
void eidt_gettext(Edit *edit, char *text)
{
	int i;
	for(i=0; i<edit->form.c_num; i++)
		text[i] = edit->form.key[i].c;
}
//数组下标转实际的端口号
void index2port(int index,OAD *oad){
	if(index==0 || index == 1 )//485I或4852
		oad->OI = PORT_485;
	else if(index==2)//PORT_ZB
		oad->OI = PORT_ZB;
	else if(index==3)//JC
		oad->OI = PORT_JC;
	oad->attflg = 2;
	if(index == 1)//4852
		oad->attrindex = 2;
	else
		oad->attrindex = 1;
}
//实际的端口号转数组下标
int port2index(OAD oad){
	int index=0;
	if(oad.OI==PORT_ZB)
		index = 2;
	else if(oad.OI==PORT_JC)
		index = 3;
	else if(oad.OI == PORT_485)
	{
		if(oad.attrindex == 1)
			index = 0;
		else
			index = 1;
	}
	return index;
}
static void setmp_cbtext_jiaoyan(char cb_text[][TEXTLEN_Y]){
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "无校验", strlen("无校验"));
	memcpy(cb_text[1], "奇校验", strlen("奇校验"));
	memcpy(cb_text[2], "偶校验", strlen("偶校验"));
}
static void setmp_cbtext_databit(char cb_text[][TEXTLEN_Y]){
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "5位", strlen("5位"));
	memcpy(cb_text[1], "6位", strlen("6位"));
	memcpy(cb_text[2], "7位", strlen("7位"));
	memcpy(cb_text[3], "8位", strlen("8位"));
}
static void setmp_cbtext_stopbit(char cb_text[][TEXTLEN_Y]){
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "1位", strlen("1位"));
	memcpy(cb_text[1], "2位", strlen("2位"));
}
static void setmp_cbtext_flowctrl(char cb_text[][TEXTLEN_Y]){
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "无流控", strlen("无流控"));
	memcpy(cb_text[1], "硬件", strlen("硬件"));
	memcpy(cb_text[2], "软件", strlen("软件"));
}

static void setmp_cbtext_func(char cb_text[][TEXTLEN_Y]){
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "上行通信", strlen("上行通信"));
	memcpy(cb_text[1], "抄表", strlen("抄表"));
	memcpy(cb_text[2], "级联", strlen("级联"));
	memcpy(cb_text[3], "停用", strlen("停用"));
}

static void setmp_cbtext_port(char cb_text[][TEXTLEN_Y]){
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "RS485-1", strlen("RS485-1"));
	memcpy(cb_text[1], "RS485-2", strlen("RS485-2"));
	memcpy(cb_text[2], "载波口", strlen("载波口"));
	memcpy(cb_text[3], "交采口", strlen("交采口"));
}

static void setmp_cbtext_guiyue(char cb_text[][TEXTLEN_Y]){
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0],"未知",strlen("未知"));
	memcpy(cb_text[1], "DL/T645-1997", strlen("DL/T645-1997"));
	memcpy(cb_text[2], "DL/T645-2007", strlen("DL/T645-2007"));
	memcpy(cb_text[3], "DL/T698.45", strlen("DL/T698.45"));
	memcpy(cb_text[4], "CJ/T188-2004", strlen("CJ/T188-2004"));
}

static void setmp_cbtext_baud(char cb_text[][TEXTLEN_Y]){
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "300", strlen("300"));
	memcpy(cb_text[1], "600", strlen("600"));
	memcpy(cb_text[2], "1200", strlen("1200"));
	memcpy(cb_text[3], "2400", strlen("2400"));
	memcpy(cb_text[4], "4800", strlen("4800"));
	memcpy(cb_text[5], "7200", strlen("7200"));
	memcpy(cb_text[6], "9600", strlen("9600"));
	memcpy(cb_text[7], "19200", strlen("19200"));
	memcpy(cb_text[8], "38400", strlen("38400"));
	memcpy(cb_text[9], "57600", strlen("57600"));
	memcpy(cb_text[10], "115200", strlen("115200"));
	memcpy(cb_text[11],"自适应",strlen("自适应"));
}

static void setmp_cbtext_con_type(char(*cb_text)[TEXTLEN_Y])
{
	memset(cb_text,0,TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0],"未知",strlen("未知"));
	memcpy(cb_text[1],"单相",strlen("单相"));
	memcpy(cb_text[2],"三相三线",strlen("三相三线"));
	memcpy(cb_text[3],"三相四线",strlen("三相四线"));
}

static int addmp_showlabel(struct list *head, struct list *node){
	int ret=0;
	//int npos=0;
	//配置序号、表地址、端口、规约、通信速率、费率个数、接线方式、采集器地址
	Point label_pos;
	label_pos.x = rect_Client.left;
	label_pos.y = rect_Client.top;
	label_pos.y += ROW_INTERVAL;
	gui_textshow((char *)"序  号:", label_pos, LCD_NOREV);
	label_pos.x = rect_Client.left;
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"表地址:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"端  口:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"规  约:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"波特率:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"费率个数:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"接线方式:", label_pos, LCD_NOREV);
	//npos = list_getListIndex(head, node);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
//#ifdef CCTT_I
	gui_textshow((char *)"采集器:", label_pos, LCD_NOREV);
//#endif
	if (NULL != p_JProgramInfo) {
		if (SPTF3 == p_JProgramInfo->cfg_para.device) {
			gui_textshow((char *)"局编号:", label_pos, LCD_NOREV);
		}
	}
	return ret;
}

static int setf201_showlabel(){
	int ret=0;
	Point label_pos;
	label_pos.x = rect_Client.left;
	label_pos.y = rect_Client.top;
	label_pos.y += ROW_INTERVAL;
	gui_textshow((char *)"波特率:", label_pos, LCD_NOREV);
	label_pos.x = rect_Client.left;
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"校验位:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"数据位:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"停止位:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"流  控:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"功  能:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	return ret;
}

static int setmp_showlabel(struct list *head, struct list *node){
	int ret=0;
	Point label_pos;
	label_pos.x = rect_Client.left;
	label_pos.y = rect_Client.top;
	label_pos.y += ROW_INTERVAL;
	gui_textshow((char *)"表  号:", label_pos, LCD_NOREV);
	label_pos.x = rect_Client.left;
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"表地址:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"端  口:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"规  约:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"波特率:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"费  率:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"接线类型:", label_pos, LCD_NOREV);
	//npos = list_getListIndex(head, node);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
//#ifdef CCTT_I
	gui_textshow((char *)"采集器:", label_pos, LCD_NOREV);
//#endif
	if (NULL != p_JProgramInfo) {
		if (SPTF3 == p_JProgramInfo->cfg_para.device) {
			gui_textshow((char *)"局编号:", label_pos, LCD_NOREV);
		}
	}
	return ret;
}

INT8U which_protocol(char protocol)
{
	INT8U result = 0;
	switch(protocol)
	{
	case unknown:
		result = 1;
		break;
	case Dl97:
	case Dl07:
		result = 2;
		break;
	case Dl698:
	case Cj188:
		result = 3;
		break;
	default:
		result = 0;
		break;
	}
	return result;
}
int getindexdataindex(int datanum)
{
	if(datanum>=5 && datanum<=8)
		return (datanum-5);
	return(8-5);
}
int getindexstopindex(int datanum)
{
	if(datanum==1 || datanum==2)
	{
		return (datanum-1);
	}
	return(0);
}
void menu_set485II()
{
	CLASS_f201 f201[3];
	int tmp=0;
	char first_flg=0;

	if(readCoverClass(0xf201,0,f201,sizeof(CLASS_f201)*3,para_vari_save)==-1) {
	//无参数文件，默认初始化上行通道  9600-even-8-1
		f201[1].devpara.baud = 6;
		f201[1].devpara.databits = 8;
		f201[1].devpara.stopbits = 1;
		f201[1].devpara.flow = 0;
		f201[1].devpara.verify = 2;
		f201[1].devfunc = 0;	//上行通道
	}
	Combox baudctl; //波特率
	Combox flowctl;	//流控 			无0，硬件1，软件2
	Combox checkbit;//校验位 			无0，奇1 ，偶2
	Combox databit;	//数据位 			5 ，6 ，7 ，8
	Combox stopbit;	//停止位			1 ，2
	Combox funcode;	//功能配置		上行通信0 ，抄表1 ， 系联2 ，停用3
	struct list *cur_node=NULL, *tmpnode=NULL;
	Rect rect;
	Form *cur_form=NULL, client;//client 液晶显示客户区
	char cb_text[TEXTLEN_X][TEXTLEN_Y];//用于存放combox的元素
	char str[INPUTKEYNUM]={};
	fprintf(stderr,"\n----------------------------------1");
	memset(&baudctl, 0, sizeof(Edit));
	memset(&flowctl, 0, sizeof(Edit));
	memset(&checkbit, 0, sizeof(Edit));
	memset(&databit, 0, sizeof(Edit));
	memset(&stopbit, 0, sizeof(Edit));
	memset(&funcode, 0, sizeof(Edit));
	memset(&client, 0, sizeof(Form));
	client.node.child = (struct list*)malloc(sizeof(struct list));
	if(client.node.child==NULL){
		g_curcldno = 1;//上状态栏显示
		return;
	}
	fprintf(stderr,"\n----------------------------------2");
	memset(client.node.child, 0, sizeof(struct list));
	gui_clrrect(rect_Client);
	Point pos,pos_index;
	pos.x = rect_Client.left+FONTSIZE*7.5;
	pos.y = rect_Client.top;
	memcpy(&pos_index,&pos,sizeof(Point));
	//------------------------------------------------------------
	setmp_cbtext_baud(cb_text);
//	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	pos.y +=  ROW_INTERVAL;
	combox_init(&baudctl, f201[1].devpara.baud, cb_text, pos, 0,client.node.child);		//通信速率   F201
	//------------------------------------------
	setmp_cbtext_jiaoyan(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	if (f201[1].devpara.verify < 0 || f201[1].devpara.verify > 2)
		f201[1].devpara.verify = 2;
	combox_init(&checkbit, f201[1].devpara.verify, cb_text, pos, 0,client.node.child);	//校验位    F201
	//------------------------------------------
	setmp_cbtext_databit(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	if (f201[1].devpara.databits < 5 || f201[1].devpara.databits > 8)
		f201[1].devpara.databits = 8;
	combox_init(&databit, getindexdataindex(f201[1].devpara.databits), cb_text, pos, 0,client.node.child);		//数据位    F201
	//------------------------------------------
	setmp_cbtext_stopbit(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	if (f201[1].devpara.stopbits < 1 || f201[1].devpara.stopbits > 2)
		f201[1].devpara.stopbits = 1;
	combox_init(&stopbit,getindexstopindex(f201[1].devpara.stopbits), cb_text, pos, 0,client.node.child);		//停止位    F201
	//------------------------------------------
	setmp_cbtext_flowctrl(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	if (f201[1].devpara.flow < 0 || f201[1].devpara.flow > 2)
		f201[1].devpara.flow = 0;
	combox_init(&flowctl, f201[1].devpara.flow, cb_text, pos, 0,client.node.child);		//流控    F201
	//------------------------------------------
	setmp_cbtext_func(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	if (f201[1].devfunc < 0 || f201[1].devfunc > 3)
		f201[1].devfunc = 1;
	combox_init(&funcode, f201[1].devfunc, cb_text, pos, 0,client.node.child);		//端口功能   F201
	//------------------------------------------
	setf201_showlabel();//显示各个控件的标签
	cur_node = &baudctl.form.node;
	cur_form = &baudctl.form;
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
			cur_form->pfun_process(cur_form);
			if(cur_form->pfun_process_ret==OK){
				if(msgbox_label((char *)"保存参数?终端重启", CTRL_BUTTON_OK)==ACK){
					fprintf(stderr,"\n保存");
					f201[1].devpara.baud = baudctl.cur_index;			//波特率
					f201[1].devpara.flow = flowctl.cur_index;			//流控 			无0，硬件1，软件2
					f201[1].devpara.verify = checkbit.cur_index;		//校验位 			无0，奇1 ，偶2
					f201[1].devpara.databits = databit.cur_index + 5;	//数据位 			5 ，6 ，7 ，8
					f201[1].devpara.stopbits = stopbit.cur_index + 1;	//停止位			1 ，2
					f201[1].devfunc = funcode.cur_index;				//功能配置		上行通信0 ，抄表1 ， 系联2 ，停用3
					syslog(LOG_NOTICE,"485_II para change,reboot");
					saveCoverClass(0xf201, 0, &f201, sizeof(CLASS_f201)*3, para_vari_save);
					sleep(2);
					system((const char *) "reboot");
				}
				g_curcldno = 1;
			}
			break;
		case ESC:
			if(client.node.child!=NULL)
				free(client.node.child);
			return;
		}
		if(PressKey!=NOKEY||first_flg==0){
			memcpy(&rect, &rect_Client, sizeof(Rect));
			rect.left = rect_Client.left + FONTSIZE*8 - 8;
			gui_clrrect(rect);
			tmpnode = client.node.child;
			tmp = setf201_showlabel();
			memset(str, 0, INPUTKEYNUM);
			sprintf(str,"%04d", g_curcldno);
			gui_textshow(str,pos_index,LCD_NOREV);
			while(tmpnode->next!=NULL){
				tmpnode = tmpnode->next;
				cur_form = list_entry(tmpnode, Form, node);
				if(tmp==1){
					if(list_getListIndex(client.node.child, tmpnode)>=list_getListIndex(client.node.child, cur_node))
						cur_form->pfun_show(cur_form);
				}else
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
}

void setmeterpara(void *pindex)
{
	CLASS_6001 meter;
	CLASS_6001* cur_pindex = NULL;
	cur_pindex = (CLASS_6001*)pindex;
	int tmp=0,addr_len = 0;
	char first_flg=0;
	struct list *cur_node=NULL, *tmpnode=NULL;
	Form *cur_form=NULL, client;//client 液晶显示客户区
	char cb_text[TEXTLEN_X][TEXTLEN_Y];//用于存放combox的元素
	char str[INPUTKEYNUM];
	INT8U bak_addr[20];
	INT8U sever_addr[20];
	char chg_addr[20];
	Rect rect;
	int rate_num_o = 0;
	//配置序号、表地址、端口、规约、通信速率、费率个数、接线方式、采集器地址
	Edit  edit_cldaddr, edit_rate,edit_usr_type,edit_cjqaddr;
	Combox cb_port,cb_protocol,cb_baud,cb_con_method;

	memset(&edit_cldaddr, 0, sizeof(Edit));
	memset(&cb_port, 0, sizeof(Combox));
	memset(&cb_protocol, 0, sizeof(Combox));
	memset(&cb_baud, 0, sizeof(Combox));
	memset(&edit_rate, 0, sizeof(Edit));
	memset(&edit_usr_type, 0, sizeof(Edit));
	memset(&cb_con_method, 0, sizeof(Combox));
	memset(&edit_cjqaddr, 0, sizeof(Edit));

	memcpy(&meter,cur_pindex,sizeof(CLASS_6001));
	if(cur_pindex == NULL)
		 return;
	g_curcldno = cur_pindex->sernum;//cldno;
	memset(&client, 0, sizeof(Form));
	client.node.child = (struct list*)malloc(sizeof(struct list));
	if(client.node.child==NULL){
		g_curcldno = 1;//上状态栏显示
		return;
	}
	memset(client.node.child, 0, sizeof(struct list));
	gui_clrrect(rect_Client);
	Point pos,pos_index;
	pos.x = rect_Client.left+FONTSIZE*7.5;
	pos.y = rect_Client.top;
	//------------------------------------------------------------
	pos.y += ROW_INTERVAL;
	memcpy(&pos_index,&pos,sizeof(Point));
	memset(str, 0, INPUTKEYNUM);
	sprintf(str,"%04d", cur_pindex->sernum);
	gui_textshow(str,pos,LCD_NOREV);
	//edit_init(&edit_cldno, str, 4, pos, 0, 0, client.node.child,KEYBOARD_DEC);//配置序号
	//------------------------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
//	memcpy(pos_addr,pos,sizeof(Point));
	memset(str, 0, INPUTKEYNUM);
	memset(sever_addr,0,sizeof(sever_addr));
	addr_len = cur_pindex->basicinfo.addr.addr[1];
	bcd2str(&cur_pindex->basicinfo.addr.addr[2],(INT8U*)sever_addr,addr_len+1,sizeof(str),positive);
	sscanf((char *)sever_addr,"%[0-9]",str);
	edit_init(&edit_cldaddr, str, 16, pos, 0, 0, client.node.child,KEYBOARD_DEC);//表地址
	//------------------------------------------
	setmp_cbtext_port(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	combox_init(&cb_port, port2index(cur_pindex->basicinfo.port), cb_text, pos, 0,client.node.child);//端口
	//------------------------------------------
	setmp_cbtext_guiyue(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	combox_init(&cb_protocol, cur_pindex->basicinfo.protocol, cb_text, pos, 0,client.node.child);//规约
	//------------------------------------------
	setmp_cbtext_baud(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	combox_init(&cb_baud, cur_pindex->basicinfo.baud, cb_text, pos, 0,client.node.child);//通信速率
	//------------------------------------------
//	pos.x = rect_Client.left+FONTSIZE*7.5; //用于分平显示
//	pos.y = rect_Client.top;
//	pos.y += ROW_INTERVAL;
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	sprintf(str,"%d",cur_pindex->basicinfo.ratenum);
	edit_init(&edit_rate, str, 1, pos, 0, 0, client.node.child,KEYBOARD_DEC);//费率个数
	//------------------------------------------
	setmp_cbtext_con_type(cb_text);
	pos.x = rect_Client.left+FONTSIZE*8.5;
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	combox_init(&cb_con_method, cur_pindex->basicinfo.connectype, cb_text, pos, 0,client.node.child);//接线方式
	//------------------------------------------
	pos.x = rect_Client.left+FONTSIZE*7.5;
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	memset(sever_addr,0,sizeof(sever_addr));
	addr_len = cur_pindex->extinfo.cjq_addr.addr[1]+1;
	bcd2str(&cur_pindex->extinfo.cjq_addr.addr[2],(INT8U*)sever_addr,addr_len,sizeof(str),positive);
	sscanf((char *)sever_addr,"%[0-9]",str);
	edit_init(&edit_cjqaddr, str, addr_len*2, pos, 0, 0, client.node.child,KEYBOARD_DEC);//采集器地址// addr_len*2采集器地址长度

	cur_node = &edit_cldaddr.form.node;
	cur_form = &edit_cldaddr.form;
	setmp_showlabel(client.node.child, cur_node);//显示各个控件的标签
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
					if(msgbox_label((char *)"保存参数?", CTRL_BUTTON_OK)==ACK){
						memset(str,0,sizeof(str));
						memset(chg_addr,0,sizeof(chg_addr));
						memset(sever_addr,0,sizeof(sever_addr));
						memset(meter.basicinfo.addr.addr,0,sizeof(meter.basicinfo.addr.addr));
						eidt_gettext(&edit_cldaddr, str);
						if(cb_protocol.cur_index == Dl97 || cb_protocol.cur_index == Dl07)//97表和07表
						{
							memset(bak_addr,'0',sizeof(bak_addr));
							sscanf(str,"%*[^1-9]%[0-9]",chg_addr);
							if(strlen(chg_addr)<12)
							{
								memcpy(&bak_addr[12-strlen(chg_addr)],chg_addr,strlen(chg_addr));
							}
							else if(strlen(chg_addr)>12)
							{
								memcpy(bak_addr,chg_addr,12);
							}
							bak_addr[12] = '\0';
						}
						else
						{
							memset(bak_addr,0,sizeof(bak_addr));
							strncpy((char*)bak_addr,str,sizeof(bak_addr)-1);
						}
						addr_len = strlen((char*)bak_addr);
						if(str2bcd((INT8U*)bak_addr,sever_addr,addr_len))
						{
							if(addr_len%2)//TODO:表地址为奇数位是否补F
							{
								sever_addr[addr_len/2] |= 0x0F;
								memcpy(&meter.basicinfo.addr.addr[2],sever_addr,addr_len/2 + 1);
								meter.basicinfo.addr.addr[1] = addr_len/2+1;
							}
							else{
								memcpy(&meter.basicinfo.addr.addr[2],sever_addr,addr_len/2);
								meter.basicinfo.addr.addr[1] = addr_len/2;
							}
							meter.basicinfo.addr.addr[0] = meter.basicinfo.addr.addr[1] + 1;
							meter.basicinfo.addr.addr[1] -= 1;
						}

						//配置序号、表地址、端口、规约、通信速率、费率个数、接线方式、采集器地址
						index2port(cb_port.cur_index,&meter.basicinfo.port);//获取端口oad放入meter中
						meter.basicinfo.protocol = cb_protocol.cur_index;
						meter.basicinfo.baud = cb_baud.cur_index;
						eidt_gettext(&edit_rate,str);
						rate_num_o = atoi(str);
						if(rate_num_o > 255)
						{
							rate_num_o = 255;
						}
						meter.basicinfo.ratenum = rate_num_o;
						meter.basicinfo.connectype = cb_con_method.cur_index;
//#ifdef CCTT_I
						memset(str,0,sizeof(str));
						memset(bak_addr,0,sizeof(bak_addr));
						memset(sever_addr,0,sizeof(sever_addr));
						memset(meter.extinfo.cjq_addr.addr,0,sizeof(meter.extinfo.cjq_addr.addr));
						eidt_gettext(&edit_cjqaddr, str);
						strncpy(bak_addr,str,sizeof(bak_addr)-1);
						addr_len = strlen((char*)bak_addr);

						if(str2bcd((INT8U*)bak_addr,sever_addr,addr_len))
						{
							if(addr_len%2)//TODO:表地址为奇数位是否补F
							{
								sever_addr[addr_len/2] |= 0x0F;
								memcpy(&meter.extinfo.cjq_addr.addr[2],sever_addr,addr_len/2 + 1);
								meter.extinfo.cjq_addr.addr[1] = addr_len/2+1;
							}
							else{
								memcpy(&meter.extinfo.cjq_addr.addr[2],sever_addr,addr_len/2);
								meter.extinfo.cjq_addr.addr[1] = addr_len/2;
							}
							meter.extinfo.cjq_addr.addr[0] = meter.extinfo.cjq_addr.addr[1] + 1;
							meter.extinfo.cjq_addr.addr[1] -= 1;
						}
						saveParaClass(0x6000,(void *)&meter,meter.sernum);
					}
					g_curcldno = 1;
				}
			}
			break;
		case ESC:
			if(client.node.child!=NULL)
				free(client.node.child);
			return;
		}
		if(PressKey!=NOKEY||first_flg==0){
			memcpy(&rect, &rect_Client, sizeof(Rect));
			rect.left = rect_Client.left + FONTSIZE*8 - 8;
			gui_clrrect(rect);
			tmpnode = client.node.child;
			tmp = setmp_showlabel(client.node.child, cur_node);
			memset(str, 0, INPUTKEYNUM);
			sprintf(str,"%04d", g_curcldno);
			gui_textshow(str,pos_index,LCD_NOREV);
			while(tmpnode->next!=NULL){
				tmpnode = tmpnode->next;
				cur_form = list_entry(tmpnode, Form, node);
				if(tmp==1){
					if(list_getListIndex(client.node.child, tmpnode)>=list_getListIndex(client.node.child, cur_node))
						cur_form->pfun_show(cur_form);
				}else
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
}

/*
 * 目前处理的添加测量点为自动寻找最小可配置序号
 * 对已存在的配置序号不进行修改
 * */
void addmeter()
{
	CLASS_6001 meter;
	int sernum = 0;
	int tmp=0,addr_len = 0,rate_chg = 0;
	char first_flg=0;
	struct list *cur_node=NULL, *tmpnode=NULL;
	Form *cur_form=NULL, client;//client 液晶显示客户区
	char cb_text[TEXTLEN_X][TEXTLEN_Y];//用于存放combox的元素
	char str[INPUTKEYNUM];
	char bak_addr[20];
	char chg_addr[20];
	INT8U sever_addr[20];
	Rect rect;
	//配置序号、表地址、端口、规约、通信速率、费率个数、接线方式、采集器地址
	Edit edit_cldno,edit_cldaddr,edit_rate, edit_cjqaddr;
	Combox cb_port,cb_protocol,cb_baud,cb_con_method;

	memset(&edit_cldno, 0, sizeof(Edit));
	memset(&meter,0,sizeof(CLASS_6001));
	memset(&edit_cldaddr, 0, sizeof(Edit));
	memset(&cb_port, 0, sizeof(Combox));
	memset(&cb_protocol, 0, sizeof(Combox));
	memset(&cb_baud, 0, sizeof(Combox));
	memset(&edit_rate,0,sizeof(Edit));
	memset(&cb_con_method,0,sizeof(Edit));
	memset(&edit_cjqaddr, 0, sizeof(Edit));

	sernum = gui_mp_getsernum();//获取当前需要添加测量点的序号
	 if(sernum >= MAX_POINT_NUM){
		msgbox_label((char *)"已达上限！", CTRL_BUTTON_CANCEL);
		return;//测量点已达到最大数量，不能进行添加测量点操作
	}
	g_curcldno = sernum;

	memset(&client, 0, sizeof(Form));
	client.node.child = (struct list*)malloc(sizeof(struct list));
	if(client.node.child==NULL){
		g_curcldno = 1;//上状态栏显示
		return;
	}
	memset(client.node.child, 0, sizeof(struct list));
	gui_clrrect(rect_Client);
	//往控件里放入测量点数据 应该在控件init里设置
	Point pos;
	Point pos_index;
	//配置序号、表地址、端口、规约、通信速率、费率个数、接线方式、采集器地址

	pos.x = rect_Client.left+FONTSIZE*7.5;
	pos.y = rect_Client.top;
	pos.y += ROW_INTERVAL;
	memcpy(&pos_index,&pos,sizeof(Point));
	memset(str, 0, INPUTKEYNUM);
	sprintf(str,"%04d", sernum);
	//gui_textshow(str,pos,LCD_NOREV);
	edit_init(&edit_cldno, str, 4, pos, 0, 0, client.node.child,KEYBOARD_DEC);//配置序号
	//------------------------------------------------------------
	pos.x = rect_Client.left+FONTSIZE*7.5;
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	edit_init(&edit_cldaddr, str, 16, pos, 0, 0, client.node.child,KEYBOARD_DEC);//表地址
	//------------------------------------------
	setmp_cbtext_port(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	combox_init(&cb_port, 0, cb_text, pos, 0,client.node.child);//端口
	//------------------------------------------
	setmp_cbtext_guiyue(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	combox_init(&cb_protocol, 2, cb_text, pos, 0,client.node.child);//规约07
	//------------------------------------------
	setmp_cbtext_baud(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	combox_init(&cb_baud, 3, cb_text, pos, 0,client.node.child);//通信速率
	//------------------------------------------
	pos.x = rect_Client.left+FONTSIZE*9.5;
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
//	memcpy(str, cur_pindex->basicinfo.ratenum, 1);
	str[0] = '4';
	edit_init(&edit_rate, str, 1, pos, 0, 0, client.node.child,KEYBOARD_DEC);//费率个数
	//------------------------------------------
	setmp_cbtext_con_type(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	combox_init(&cb_con_method, 1, cb_text, pos, 0,client.node.child);//接线方式
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	edit_init(&edit_cjqaddr, str, 16, pos, 0, 0, client.node.child,KEYBOARD_DEC);//采集器地址
	//------------------------------------------
	cur_node = &edit_cldaddr.form.node;
	cur_form = &edit_cldaddr.form;
	addmp_showlabel(client.node.child, cur_node);//显示各个控件的标签
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
			cur_form->pfun_process(cur_form);
			if(cur_form->pfun_process_ret==OK){
				if(msgbox_label((char *)"保存测量点?", CTRL_BUTTON_OK)==ACK){
					//配置序号、表地址、端口、规约、通信速率、费率个数、接线方式、采集器地址
					memset(str,0,sizeof(str));
					eidt_gettext(&edit_cldno, str);
					meter.sernum = atoi(str);//配置序号

					memset(str,0,sizeof(str));
					memset(chg_addr,0,sizeof(chg_addr));
					memset(sever_addr,0,sizeof(sever_addr));
					memset(meter.basicinfo.addr.addr,0,sizeof(meter.basicinfo.addr.addr));
					eidt_gettext(&edit_cldaddr, str);
					if(cb_protocol.cur_index == Dl97 || cb_protocol.cur_index == Dl07)//97表和07表
					{
						memset(bak_addr,'0',sizeof(bak_addr));
						sscanf(str,"%*[^1-9]%[1-9a-z]",chg_addr);
						if(strlen(chg_addr)<12)
						{
							memcpy(&bak_addr[12-strlen(chg_addr)],chg_addr,strlen(chg_addr));
						}
						else if(strlen(chg_addr)>12)
						{
							memcpy(bak_addr,chg_addr,12);
						}
						bak_addr[12] = '\0';
					}
					else
					{
						memset(bak_addr,0,sizeof(bak_addr));
						strncpy(bak_addr,str,sizeof(bak_addr)-1);
					}
					addr_len = strlen((char*)bak_addr);
//					DEBUG_TIME_LINE("\n------addr_len = %d-------\n",addr_len);
					if(str2bcd((INT8U*)bak_addr,sever_addr,addr_len))
					{
						if(addr_len%2)//TODO:表地址为奇数位是否补F
						{
							sever_addr[addr_len/2] |= 0x0F;
							memcpy(&meter.basicinfo.addr.addr[2],sever_addr,addr_len/2 + 1);
							meter.basicinfo.addr.addr[1] = addr_len/2+1;
						}
						else{
							memcpy(&meter.basicinfo.addr.addr[2],sever_addr,addr_len/2);
							meter.basicinfo.addr.addr[1] = addr_len/2;
						}
						meter.basicinfo.addr.addr[0] = meter.basicinfo.addr.addr[1] + 1;
						meter.basicinfo.addr.addr[1] -= 1;
					}
					index2port(cb_port.cur_index,&meter.basicinfo.port);//获取端口oad放入meter中
					meter.basicinfo.baud = cb_baud.cur_index;
					meter.basicinfo.protocol = cb_protocol.cur_index;

					memset(str,0,sizeof(str));
					eidt_gettext(&edit_rate,str);
					rate_chg= atoi(str);
					if(rate_chg>255){
						rate_chg = 4;
					}
					meter.basicinfo.ratenum = rate_chg;
					meter.basicinfo.connectype = cb_con_method.cur_index;

					memset(str,0,sizeof(str));
					memset(bak_addr,0,sizeof(bak_addr));
					memset(sever_addr,0,sizeof(sever_addr));
					memset(meter.extinfo.cjq_addr.addr,0,sizeof(meter.extinfo.cjq_addr.addr));
					eidt_gettext(&edit_cjqaddr, str);
					strncpy(bak_addr,str,sizeof(bak_addr)-1);
					addr_len = strlen((char*)bak_addr);
					if(str2bcd((INT8U*)bak_addr,sever_addr,addr_len))
					{
						if(addr_len%2)//TODO:表地址为奇数位是否补F
						{
							sever_addr[addr_len/2] |= 0x0F;
							memcpy(&meter.extinfo.cjq_addr.addr[2],sever_addr,addr_len/2 + 1);
							meter.extinfo.cjq_addr.addr[1] = addr_len/2+1;
						}
						else{
							memcpy(&meter.extinfo.cjq_addr.addr[2],sever_addr,addr_len/2);
							meter.extinfo.cjq_addr.addr[1] = addr_len/2;
						}
						meter.extinfo.cjq_addr.addr[0] = meter.extinfo.cjq_addr.addr[1] + 1;
						meter.extinfo.cjq_addr.addr[1] -= 1;
					}
					saveParaClass(0x6000,(void *)&meter,meter.sernum);
				}
				g_curcldno = 1;
			}
			break;
		case ESC:
			if(client.node.child!=NULL)
				free(client.node.child);
			return;
		}
		if(PressKey!=NOKEY||first_flg==0){
			memcpy(&rect, &rect_Client, sizeof(Rect));
			rect.left = rect_Client.left + FONTSIZE*8 - 8;
			gui_clrrect(rect);
			tmpnode = client.node.child;
			tmp = addmp_showlabel(client.node.child, cur_node);
			while(tmpnode->next!=NULL){
				tmpnode = tmpnode->next;
				cur_form = list_entry(tmpnode, Form, node);
				if(tmp==1){
					if(list_getListIndex(client.node.child, tmpnode)>=list_getListIndex(client.node.child, cur_node))
						cur_form->pfun_show(cur_form);
				}else
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
}

void deletemeter(void* pindex)
{
	CLASS_6001* cur_pindex = NULL;
	CLASS_6001* del_pindex = NULL;
	cur_pindex = (CLASS_6001*)pindex;
	char str[20];
	sprintf(str,"删除配置序号%04d?",cur_pindex->sernum);
	if(msgbox_label(str, CTRL_BUTTON_OK)==ACK)
	{
		delClassBySeq(0x6000,del_pindex,cur_pindex->sernum);
		delay(300);
	}
}
//菜单 电表档案设置  160-32=128/12=10
void menu_jzqsetmeter(){
	showallmeter(setmeterpara);	//485-II设置
}
void menu_jzqaddmeter(){
	addmeter();
}
void menu_jzqdelmeter(){
	showallmeter(deletemeter);
}
void menu_jzqtime()
{
	time_t curr_time=time(NULL);
	struct tm curr_tm;
	char s_jzqtime[20], str[5], msgbox_ret=0,oprmode_old=0;
	//int s_len=14;
	memset(s_jzqtime, 0, 20);
	localtime_r(&curr_time, &curr_tm);
	sprintf(s_jzqtime, "%04d%02d%02d%02d%02d%02d", curr_tm.tm_year+1900,curr_tm.tm_mon+1,
					curr_tm.tm_mday, curr_tm.tm_hour, curr_tm.tm_min, curr_tm.tm_sec);
//	dbg_prt("\n s_jzqtime=%s %d", s_jzqtime, (int)curr_time);
	oprmode_old = get_oprmode();
	set_oprmode(OPRMODE_MODIFY);
	msgbox_ret = msgbox_inputjzqtime(s_jzqtime, strlen(s_jzqtime));
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
		g_JZQ_TimeSetUp_flg = 1;
		stime(&curr_time);
		system("hwclock -w");
		delay(1000);
		g_JZQ_TimeSetUp_flg = 0;
	}
}
//设置密码
void menu_setpasswd()
{
	char passwd[7], s_passwd_ret[6],oprmode_old=0;
	int ret;
	int i=0;
	memset(passwd, 0, 7);
	if(readcfg((char*)"/nor/config/lcd.cfg", (char*)"LCD_PASSWORD", passwd)==0){
		for(i=0; i<6; i++)
			passwd[i]='0';
	}
	memset(s_passwd_ret, 0, 6);
	oprmode_old = get_oprmode();
	set_oprmode(OPRMODE_MODIFY);
	ret = msgbox_setpasswd(passwd, 6, s_passwd_ret);
		set_oprmode(oprmode_old);
	if(ret==PASSWD_OK){
		memset(passwd, 0, 7);
		memcpy(passwd, &s_passwd_ret[0], 6);
		writecfg((char*)"/nor/config/lcd.cfg", (char*)"LCD_PASSWORD", passwd);
		msgbox_label((char *)"设置成功！", CTRL_BUTTON_OK);
	}
	else if(ret == PASSWD_ESC)
	{
		return ;
	}
	else
	{
		msgbox_label((char *)"密码错误！", CTRL_BUTTON_OK);
	}
}

void menu_readmeterbycldno(){
	int i,flag= 0,cld_num=0,cldno=0,mqcount=0;
	CLASS_6001	 meter={};
	CLASS11		coll={};
	if(readInterClass(0x6000,&coll)==0){
		msgbox_label("档案异常",2);
		return ;
	}
	cldno = msgbox_inputcldno();//获取用户输入表序号
	if(cldno<=0)
	{
		msgbox_label("表序号输入错误",2);
		return;
	}
	cld_num = getFileRecordNum(0x6000);//获取文件测量点单元个数
	if(cld_num<=0)
	{
		msgbox_label("无表档案",2);
		return;
	}
	for(i=0;i<cld_num;i++)//循环查询有效测量点个数
	{
		if(readParaClass(0x6000,&meter,i))
			if(meter.sernum == cldno)
			{
				flag=1;
				break;
			}
	}
	if(flag == 0 )
	{
		msgbox_label("未找到对应表序号",2);
		return;
	}
	LcdDataItem item[10];//存储的所有数据项
	memset(item, 0, 10*sizeof(LcdDataItem));
		if(meter.basicinfo.port.OI == PORT_485 ||meter.basicinfo.port.OI ==  PORT_ZB){
			mqcount = requestdata_485_ZB_Block(&meter,(INT8U*)PROXY_485_MQ_NAME,5, item);
		}
		show_realdata(meter.sernum, item, mqcount);
		memset(p_Proxy_Msg_Data,0,sizeof(Proxy_Msg));
}
//目前只支持6字节，12位电表
void menu_readmeterbycldaddr()
{
	int i,mqcount=0, ret=0,cld_num=0,flag=0;
	LcdDataItem item[10];//存储的所有数据项
	memset(item, 0, 10*sizeof(LcdDataItem));
	CLASS_6001	 meter={};
		CLASS11		coll={};
		if(readInterClass(0x6000,&coll)==0){
			msgbox_label("档案异常",2);
			return ;
		}
		cld_num = getFileRecordNum(0x6000);//获取文件测量点单元个数
		if(cld_num<=0)
		{
			msgbox_label("无表档案",2);
			return;
		}
	INT8U cldaddr[12];
	memset(cldaddr, 0, 12);
	ret = msgbox_inputcldaddr((char*)cldaddr,12);//返回按键值
	if(ret<=0)
	{
		msgbox_label("表地址输入错误",2);
		return;
	}
	for(i=0;i<cld_num;i++)//循环查询有效测量点个数
	{
		if(readParaClass(0x6000,&meter,i))
		{
			if(memcmp(cldaddr, &meter.basicinfo.addr.addr[2], 6)==0)
			{
				flag=1;
				break;
			}
		}
	}
	if(flag == 0 )
	{
		msgbox_label("未找到对应表地址",2);
		return;
	}
	if(meter.basicinfo.port.OI == PORT_485 ||meter.basicinfo.port.OI ==  PORT_ZB){
		mqcount = requestdata_485_ZB_Block(&meter,(INT8U*)PROXY_485_MQ_NAME,5, item);
		}
		show_realdata(meter.sernum, item, mqcount);
		memset(p_Proxy_Msg_Data,0,sizeof(Proxy_Msg));
}
//获得本地IP
void getlocalip(char *ip){
	int iIP[4];
	FILE *fp;
	INT8U str_0[20],str_1[20],str_2[20],str_3[20];
   //para_F7 p_f7;
	memset(iIP, 0, 4*sizeof(int));
	memset(str_0,0,20);memset(str_1,0,20);memset(str_2,0,20);memset(str_3,0,20);
	fp=fopen("/nor/rc.d/ip.sh", "r");
	if(fp!=NULL){
		fscanf(fp,"%s %s %s %s",str_0,str_1,str_2,str_3);
		fclose(fp);
		sscanf((char*)str_2, "%d.%d.%d.%d",&iIP[0],&iIP[1],&iIP[2],&iIP[3]);
		ip[3] = iIP[3]&0xff;
		ip[2] = iIP[2]&0xff;
		ip[1] = iIP[1]&0xff;
		ip[0] = iIP[0]&0xff;
	}
	return;
}

void menu_jzqreboot()
{
	reboot(LINUX_REBOOT_CMD_RESTART);
}
//1打印开关 2 升级 3 内存使用率
void menu_debug()
{
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		if(PressKey==ESC)
			break;
		PressKey = NOKEY;
		delay(200);
	}
	return;
}
void gui_clearEvent()
{
	//事件类数据清除
	INT8U*	eventbuff=NULL;
	int 	saveflg=0,i=0;
	int		classlen=0;
	Class7_Object	class7={};

	for(i=0; i < sizeof(event_class_len)/sizeof(EVENT_CLASS_INFO);i++)
	{
		if(event_class_len[i].oi) {
			classlen = event_class_len[i].classlen;
			eventbuff = (INT8U *)malloc(classlen);
			if(eventbuff!=NULL) {
				memset(eventbuff,0,classlen);
				DEBUG_TIME_LINE("i=%d, oi=%04x, size=%d\n",i,event_class_len[i].oi,classlen);
				saveflg = readCoverClass(event_class_len[i].oi,0,(INT8U *)eventbuff,classlen,event_para_save);
				DEBUG_TIME_LINE("saveflg=%d oi=%04x\n",saveflg,event_class_len[i].oi);

				if(saveflg) {
					memcpy(&class7,eventbuff,sizeof(Class7_Object));
					DEBUG_TIME_LINE("修改前：i=%d,oi=%x,class7.crrentnum=%d\n",i,event_class_len[i].oi,class7.crrentnum);
					if(class7.crrentnum!=0) {
						class7.crrentnum = 0;			//清除当前记录数
						memcpy(eventbuff,&class7,sizeof(Class7_Object));
						saveflg = saveCoverClass(event_class_len[i].oi,0,eventbuff,classlen,event_para_save);
					}
				}
				free(eventbuff);
				eventbuff=NULL;
			}
		}
	}
	system("rm -rf /nand/event/record");
	system("rm -rf /nand/event/current");
}

void jzq_reset(int type_init){
	switch(type_init){
	case DATA_INIT:
		system("rm -rf /nand/task");
		system("rm -rf /nand/demand");
		gui_clearEvent();
		break;
	case EVENT_INIT:
		gui_clearEvent();
		break;
	case DEMAND_INIT:
		system("rm -rf /nand/demand");
		break;
	case FACTORY_RESET:
		system("rm -rf /nand/para");
		system("rm -rf /nand/event/property");
		InitClass4016();    //当前套日时段表
		InitClass4300();    //电气设备信息
		//InitClass6000();	//初始化交采采集档案
	    InitClassf203();	//开关量输入
		InitClassByZone(0);		//根据地区进行相应初始化	4500,4510参数
		break;
	default :
		break;
	}
}

//初始化集中器数据

void menu_initjzqdata(){
	jzq_reset(DATA_INIT);
}
//初始化集中器参数

void menu_initjzqevent(){
	jzq_reset(EVENT_INIT);
}

void menu_initjzqdemand(){
	jzq_reset(DEMAND_INIT);
}
void menu_FactoryReset()
{
	jzq_reset(FACTORY_RESET);
}
int pagesetup_showlabel(int item_no){
	char str[100];
	Point label_pos;
	int i;
	gui_setpos(&label_pos, rect_Client.left+2*FONTSIZE, rect_Client.top);
	for(i=0; i<LunPageNum/2; i++){
		label_pos.y += FONTSIZE*2 + 2;
		if(label_pos.y > rect_Client.bottom)
			label_pos.y = rect_Client.top;
		memset(str, 0, 100);
		sprintf(str, "%s:",LunXian[i+item_no].label_name);//, i+1+item_no);
		gui_textshow(str, label_pos, LCD_NOREV);
	}
	return 0;
}

void print_lunxun_flg(INT8U *lunxun_flg){
	int i;
	DEBUG_TIME_LINE("\nlunxun_flg=");
	for(i=0; i<4; i++){
		DEBUG_TIME_LINE( " %02x", lunxun_flg[i]);
	}
}
//集中器页面设置
int pagesetup_item(int pageno, INT8U *lunxun_flg){
	int tmp=0, i;
	char first_flg=0, ssetup[10];
	struct list *cur_node=NULL, *tmpnode=NULL;
	Form *cur_form=NULL, client;//client 液晶显示客户区
	char cb_text[TEXTLEN_X][TEXTLEN_Y];//用于存放combox的元素
	Rect rect;
	Combox cb_page_setup[LunPageNum/2];
	memset(cb_page_setup, 0, (LunPageNum/2)*sizeof(Form));
	memset(&client, 0, sizeof(Form));
	client.node.child = (struct list*)malloc(sizeof(struct list));
	if(client.node.child==NULL)
		return 0;
	memset(client.node.child, 0, sizeof(struct list));
	gui_clrrect(rect_Client);
	//往控件里放入测量点数据 应该在控件init里设置
	Point pos;
	gui_setpos(&pos, rect_Client.left+20*FONTSIZE, rect_Client.top);

	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "禁止", strlen("禁止"));
	memcpy(cb_text[1], "使能", strlen("使能"));

	print_lunxun_flg(lunxun_flg);
	for(i=0; i<LunPageNum/2; i++){
		pos.y += FONTSIZE*2 + 2;
		if(pos.y > rect_Client.bottom)
			pos.y = rect_Client.top;
		combox_init(&cb_page_setup[i], (lunxun_flg[pageno/(LunPageNum/2)]>>i)&0x01, cb_text, pos, NORETFLG,client.node.child);
	}
	//--------------------------------------------
	cur_node = &cb_page_setup[0].form.node;
	cur_form = &cb_page_setup[0].form;
	pagesetup_showlabel(pageno);//显示各个控件的标签
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		switch(PressKey)
		{
		case LEFT:
			break;
		case UP:
			cur_form->focus = NOFOCUS;
			cur_node=list_getprev(cur_node);
			if(cur_node==client.node.child)
				cur_node = list_getlast(client.node.child);
			break;
		case RIGHT:
			break;
		case DOWN:
			cur_form->focus = NOFOCUS;
			if(list_getnext(cur_node)==NULL){
				cur_node = list_getfirst(cur_node);
				cur_node = cur_node->next;
			}else
				cur_node = list_getnext(cur_node);
			break;
		case OK:
			cur_form->pfun_process(cur_form);
			DEBUG_TIME_LINE( "\n cur_form->pfun_process_ret=%d",cur_form->pfun_process_ret);
			if(cur_form->pfun_process_ret==OK){
//				cur_node = list_getfirst(cur_node);
				if(msgbox_label((char *)"保存参数?", CTRL_BUTTON_OK)==ACK)
				{
//					dbg_prt("\n msgbox_ret = OK  save para here!!!!");
					print_lunxun_flg(lunxun_flg);
					for(i=0; i<LunPageNum/2; i++){
//						dbg_prt("\n pageno=%d cb_page_setup[%d].cur_index=%d",
//								 pageno, i, cb_page_setup[i].cur_index);
						if(cb_page_setup[i].cur_index==1)
							lunxun_flg[pageno/(LunPageNum/2)] |= (1<<(i%8));
						else
							lunxun_flg[pageno/(LunPageNum/2)] &= (~(1<<(i%8)));
					}
					print_lunxun_flg(lunxun_flg);
					memset(ssetup, 0, 10);
					bcd2asc((INT8U*)lunxun_flg, 4, (INT8U*)ssetup, positive);
					writecfg((char*)"/nor/config/lcd.cfg", (char*)"LCD_LUNXIAN_SETUP", ssetup);
					//	msgbox_label((char *)"保存失败！", CTRL_BUTTON_CANCEL);
				}
				g_curcldno = 1;
			}
			break;
		case ESC:
			if(client.node.child!=NULL)
				free(client.node.child);
			return ESC;
		}
		if(PressKey!=NOKEY||first_flg==0){
			memcpy(&rect, &rect_Client, sizeof(Rect));
			rect.left = rect_Client.left + FONTSIZE*8 - 8;
			gui_clrrect(rect);
			tmpnode = client.node.child;
			tmp = pagesetup_showlabel(pageno);
			while(tmpnode->next!=NULL){
				tmpnode = tmpnode->next;
				cur_form = list_entry(tmpnode, Form, node);
				//dbg_prt("\n ctltype = %d", cur_form->CtlType);
				if(tmp==1){
					if(list_getListIndex(client.node.child, tmpnode)>=list_getListIndex(client.node.child, cur_node))
						cur_form->pfun_show(cur_form);
				}else
					cur_form->pfun_show(cur_form);
			}
			for(i=0; i<LunPageNum/2; i++){
//				dbg_prt("\n 1111pageno=%d cb_page_setup[%d].cur_index=%d",
//						 pageno, i, cb_page_setup[i].cur_index);
				if(cb_page_setup[i].cur_index==1)
					lunxun_flg[pageno/(LunPageNum/2)] |= (1<<(i%8));
				else
					lunxun_flg[pageno/(LunPageNum/2)] &= (~(1<<(i%8)));
			}
			cur_form = list_entry(cur_node, Form, node);//根据链表节点找到控件指针
			cur_form->focus = FOCUS;
			memcpy(&rect, &cur_form->rect, sizeof(Rect));
			rect.left += 2;
			rect.right -= 3;
			rect.top -= 1;
			gui_rectangle(gui_changerect(gui_moverect(rect, DOWN, 4), 4));
			first_flg = 1;
			if(PressKey==LEFT)
				return LEFT;
			if(PressKey==RIGHT){
				return RIGHT;
			}
//			//更新轮显标志
//			for(i=0; i<LunPageNum/2; i++){
//				if(cb_page_setup[i].cur_index==1)
//					lunxun_flg[i/8] |= (1<<(i%8));
//				else
//					lunxun_flg[i/8] &= (~(1<<(i%8)));
//			}
		}
		PressKey = NOKEY;
		delay(100);
	}
	if(client.node.child!=NULL)
		free(client.node.child);
	return 0;
}
void menu_pagesetup(){
	int page_item=0, left_right=0;
	char str[100], ssetup[10];
	INT8U lcd_lunxian_flg[4];//存储轮寻各个数据项是否显示，每位代表一个
	memset(lcd_lunxian_flg, 0, 4);
	memset(str, 0, 100);
	memset(ssetup, 0, 10);
	if(readcfg((char*)"/nor/config/lcd.cfg", (char*)"LCD_LUNXIAN_SETUP", ssetup)==0)
		memset(lcd_lunxian_flg, 0xff, 4);
	else
		asc2bcd((INT8U*)ssetup, 8, (INT8U*)lcd_lunxian_flg, positive);
//	dbg_prt( "\n lcd_lunxian_flg = %x", lcd_lunxian_flg[0]);
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		//gui_clrrect(rect_Client);
		left_right = pagesetup_item(page_item, lcd_lunxian_flg);
		if(left_right==LEFT){
			page_item -= LunPageNum/2;
			if(page_item<0)
				page_item = LunPageNum/2;
		}else if(left_right==RIGHT){
			page_item += LunPageNum/2;
			if(page_item>=LunPageNum)
				page_item = 0;
		}else if(left_right==ESC)
			break;
		delay(300);
	}
}

int settx_showlabel(char type){
	Point label_pos, pos;
	gui_setpos(&pos, rect_Client.left+8*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"通讯方式", pos, LCD_NOREV);
	gui_setpos(&label_pos, rect_Client.left+5.5*FONTSIZE, rect_Client.top);
	label_pos.y += FONTSIZE*3 + ROW_INTERVAL;
	gui_textshow((char *)"重发次数:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"重拨间隔:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"心跳周期:", label_pos, LCD_NOREV);
	if(2 == type)
	{
		label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
		gui_textshow((char *)"在线方式:", label_pos, LCD_NOREV);
	}
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"连接模式:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"工作模式:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"应用方式:", label_pos, LCD_NOREV);
	return 0;
}
/*
 * 以太网通信方式更改
 * */
void menu_set_nettx()
{
	int tmp=0,interval_replay=0;
	INT8U trynum = 0;
	char first_flg=0;
	struct list *cur_node=NULL, *tmpnode=NULL;
	Form *cur_form=NULL, client;//client 液晶显示客户区
	char cb_text[TEXTLEN_X][TEXTLEN_Y];//用于存放combox的元素
	char str[INPUTKEYNUM];
	Rect rect;
	Edit edit_resendnum,edit_redailstep, edit_heartbeat;
	Combox cb_conntype, cb_worktype, cb_appcontype;

	memset(&edit_resendnum, 0, sizeof(Edit));
	memset(&edit_redailstep, 0, sizeof(Edit));
	memset(&edit_heartbeat, 0, sizeof(Edit));
	memset(&cb_conntype, 0, sizeof(Combox));
	memset(&cb_worktype, 0, sizeof(Combox));
	memset(&cb_appcontype, 0, sizeof(Combox));
	memset(&client, 0, sizeof(Form));
	client.node.child = (struct list*)malloc(sizeof(struct list));
	if(client.node.child==NULL){
		g_curcldno = 1;//上状态栏显示
		return;
	}
	memset(client.node.child, 0, sizeof(struct list));
	gui_clrrect(rect_Client);
	//往控件里放入测量点数据 应该在控件init里设置
	Point pos;

	gui_setpos(&pos, rect_Client.left+15*FONTSIZE, rect_Client.top);
	pos.y += FONTSIZE*3 + ROW_INTERVAL;

	if (p_JProgramInfo->oi_changed.oi4510 != g_chgOI4510) {
		g_chgOI4510 = p_JProgramInfo->oi_changed.oi4510;
		readCoverClass(0x4510, 0, (void*)&g_class26_oi4510, sizeof(CLASS26), para_vari_save);
	}

	memset(str, 0, INPUTKEYNUM);
	sprintf(str, "%02d", g_class26_oi4510.commconfig.timeoutRtry&0x03);//重拨次数
	edit_init(&edit_resendnum, str, 2, pos,NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
//	dbg_prt("\n para_f8.Interval_Replay=%d",para_f8.Interval_Replay);
	sprintf(str, "%02d", (g_class26_oi4510.commconfig.timeoutRtry>>2));//超时时间
	edit_init(&edit_redailstep, str, 2, pos,NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	sprintf(str, "%05d", g_class26_oi4510.commconfig.heartBeat);//心跳周期
	edit_init(&edit_heartbeat, str, 5, pos,NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);
	//------------------------------------------
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "TCP", strlen("TCP"));
	memcpy(cb_text[1], "UDP", strlen("UDP"));
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	tmp = g_class26_oi4510.commconfig.connectType;
	combox_init(&cb_conntype, tmp, cb_text, pos, NORETFLG,client.node.child);
	//--------------------------------------------
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "混合模式", strlen("混合模式"));
	memcpy(cb_text[1], "客户模式", strlen("客户模式"));
	memcpy(cb_text[2], "服务模式", strlen("服务模式"));
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	tmp = g_class26_oi4510.commconfig.workModel;
	combox_init(&cb_worktype, tmp, cb_text, pos, NORETFLG,client.node.child);
	//--------------------------------------------
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "主备模式", strlen("主备模式"));
	memcpy(cb_text[1], "多连接模式", strlen("多连接模式"));
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	tmp = g_class26_oi4510.commconfig.appConnectType;
	if(tmp < 0 || tmp > 1){
		tmp = 0;
	}
	combox_init(&cb_appcontype, tmp, cb_text, pos, NORETFLG,client.node.child);
	//--------------------------------------------
	cur_node = &edit_resendnum.form.node;
	cur_form = &edit_resendnum.form;
	settx_showlabel(1);//显示各个控件的标签
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
			DEBUG_TIME_LINE("\n get_oprmode()=%d", get_oprmode());
			if(get_oprmode()==OPRMODE_MODIFY){
				cur_form->pfun_process(cur_form);
				if(cur_form->pfun_process_ret==OK){
					//cur_node = list_getfirst(cur_node);
					if(get_oprmode()==OPRMODE_MODIFY && msgbox_label((char *)"保存参数?", CTRL_BUTTON_OK)==ACK){
						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_resendnum, str);
						trynum = atoi(str);
						DEBUG_TIME_LINE("\n------retrystr = %s,trynum = %d\n",str,trynum);
						if(trynum>3){
							trynum = 3;//重发次数最大为3
						}
						g_class26_oi4510.commconfig.timeoutRtry &= 0xF6;
						g_class26_oi4510.commconfig.timeoutRtry |= trynum;//重发次数
						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_redailstep, str);
						interval_replay = atoi(str);
						if(interval_replay>0x3F){
							interval_replay = 0x3F;//超时时间最大为63s
						}
						g_class26_oi4510.commconfig.timeoutRtry |= (interval_replay<<2);
						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_heartbeat, str);
						interval_replay = atoi(str);
						g_class26_oi4510.commconfig.heartBeat = atoi(str);
						if(g_class26_oi4510.commconfig.heartBeat > 65535)//最大心跳周期65535秒
							g_class26_oi4510.commconfig.heartBeat = 65535;//TODO:心跳周期应设置成INT16U

						g_class26_oi4510.commconfig.connectType = cb_conntype.cur_index;
						g_class26_oi4510.commconfig.workModel = cb_worktype.cur_index;
						g_class26_oi4510.commconfig.appConnectType = cb_appcontype.cur_index;

						//保存时不作参数变更比较，只要用户点击确认，那就重新更新参数
						saveCoverClass(0x4510, 0, (void*)&g_class26_oi4510, sizeof(CLASS26), para_vari_save);
						p_JProgramInfo->oi_changed.oi4510++;//更新oi标志，其他进程检测该标志
					}
					g_curcldno = 1;
				}
			}
			break;
		case ESC:
			if(client.node.child!=NULL)
				free(client.node.child);
			return;
		}
		if(PressKey!=NOKEY||first_flg==0){
			memcpy(&rect, &rect_Client, sizeof(Rect));
			rect.left = rect_Client.left + FONTSIZE*8 - 8;
			gui_clrrect(rect);
			tmpnode = client.node.child;
			tmp = settx_showlabel(1);
			while(tmpnode->next!=NULL){
				tmpnode = tmpnode->next;
				cur_form = list_entry(tmpnode, Form, node);
				if(tmp==1){
					if(list_getListIndex(client.node.child, tmpnode)>=list_getListIndex(client.node.child, cur_node))
						cur_form->pfun_show(cur_form);
				}else
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
	return;
}

/*
 * 无线通信方式更改
 * */
void menu_set_wlantx()
{
	int tmp=0, interval_replay=0;
	INT8U trynum = 0;
	char first_flg=0;
	struct list *cur_node=NULL, *tmpnode=NULL;
	Form *cur_form=NULL, client;//client 液晶显示客户区
	char cb_text[TEXTLEN_X][TEXTLEN_Y];//用于存放combox的元素
	char str[INPUTKEYNUM];
	Rect rect;
	Edit edit_resendnum,edit_redailstep, edit_heartbeat;
	Combox cb_onlinetype, cb_conntype, cb_worktype, cb_appcontype;

	memset(&edit_resendnum, 0, sizeof(Edit));
	memset(&edit_redailstep, 0, sizeof(Edit));
	memset(&edit_heartbeat, 0, sizeof(Edit));
	memset(&cb_onlinetype, 0, sizeof(Combox));
	memset(&cb_conntype, 0, sizeof(Combox));
	memset(&cb_worktype, 0, sizeof(Combox));
	memset(&cb_appcontype, 0, sizeof(Combox));
	memset(&client, 0, sizeof(Form));
	client.node.child = (struct list*)malloc(sizeof(struct list));
	if(client.node.child==NULL){
		g_curcldno = 1;//上状态栏显示
		return;
	}
	memset(client.node.child, 0, sizeof(struct list));
	gui_clrrect(rect_Client);
	//往控件里放入测量点数据 应该在控件init里设置
	Point pos;

	gui_setpos(&pos, rect_Client.left+15*FONTSIZE, rect_Client.top);
	pos.y += FONTSIZE*3 + ROW_INTERVAL;

	if (g_chgOI4500 != p_JProgramInfo->oi_changed.oi4500) {
		g_chgOI4500 = p_JProgramInfo->oi_changed.oi4500;
		readCoverClass(0x4500, 0, (void*)&g_class25_oi4500, sizeof(CLASS25), para_vari_save);
	}

	memset(str, 0, INPUTKEYNUM);
	sprintf(str, "%02d", g_class25_oi4500.commconfig.timeoutRtry&0x03);//重拨次数
	edit_init(&edit_resendnum, str, 2, pos,NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
//	dbg_prt("\n para_f8.Interval_Replay=%d",para_f8.Interval_Replay);
	sprintf(str, "%02d", (g_class25_oi4500.commconfig.timeoutRtry>>2));//超时时间
	edit_init(&edit_redailstep, str, 2, pos,NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	sprintf(str, "%05d", g_class25_oi4500.commconfig.heartBeat);//心跳周期
	edit_init(&edit_heartbeat, str, 5, pos,NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);
	//------------------------------------------
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "永久在线", strlen("永久在线"));
	memcpy(cb_text[1], "被动激活", strlen("被动激活"));
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
//	tmp=para_f8.WorkMode&0x03;
	tmp = g_class25_oi4500.commconfig.onlineType;
	if(tmp<0)
		tmp = 0;
	combox_init(&cb_onlinetype, tmp, cb_text, pos, NORETFLG,client.node.child);
	//------------------------------------------
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "TCP", strlen("TCP"));
	memcpy(cb_text[1], "UDP", strlen("UDP"));
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	tmp = g_class25_oi4500.commconfig.connectType;
	combox_init(&cb_conntype, tmp, cb_text, pos, NORETFLG,client.node.child);
	//--------------------------------------------
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "混合模式", strlen("混合模式"));
	memcpy(cb_text[1], "客户模式", strlen("客户模式"));
	memcpy(cb_text[2], "服务模式", strlen("服务模式"));
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	tmp = g_class25_oi4500.commconfig.workModel;
	combox_init(&cb_worktype, tmp, cb_text, pos, NORETFLG,client.node.child);
	//--------------------------------------------
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "主备模式", strlen("主备模式"));
	memcpy(cb_text[1], "多连接模式", strlen("多连接模式"));
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	tmp = g_class25_oi4500.commconfig.appConnectType;
	combox_init(&cb_appcontype, tmp, cb_text, pos, NORETFLG,client.node.child);
	//--------------------------------------------
	cur_node = &edit_resendnum.form.node;
	cur_form = &edit_resendnum.form;
	settx_showlabel(2);//显示各个控件的标签
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
			DEBUG_TIME_LINE("\n get_oprmode()=%d", get_oprmode());
			if(get_oprmode()==OPRMODE_MODIFY){
				cur_form->pfun_process(cur_form);
				if(cur_form->pfun_process_ret==OK){
					//cur_node = list_getfirst(cur_node);
					if(get_oprmode()==OPRMODE_MODIFY && msgbox_label((char *)"保存参数?", CTRL_BUTTON_OK)==ACK){
						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_resendnum, str);
						trynum = atoi(str);
						if(trynum>3){
							trynum = 3;//重发次数最大为3
						}
						g_class25_oi4500.commconfig.timeoutRtry &= 0xF6;
						g_class25_oi4500.commconfig.timeoutRtry |= trynum;//重发次数
						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_redailstep, str);
						interval_replay = atoi(str);
						if(interval_replay>0x3F){
							interval_replay = 0x3F;//超时时间最大为63s
						}
						g_class25_oi4500.commconfig.timeoutRtry |= (interval_replay<<2);

						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_heartbeat, str);
						interval_replay = atoi(str);
						g_class25_oi4500.commconfig.heartBeat = atoi(str);
						if(g_class25_oi4500.commconfig.heartBeat > 65535)//最大心跳周期65535秒
							g_class25_oi4500.commconfig.heartBeat = 65535;//TODO:心跳周期应设置成INT16U

						g_class25_oi4500.commconfig.onlineType = cb_onlinetype.cur_index;
						g_class25_oi4500.commconfig.connectType = cb_conntype.cur_index;
						g_class25_oi4500.commconfig.workModel = cb_worktype.cur_index;
						g_class25_oi4500.commconfig.appConnectType = cb_appcontype.cur_index;

						saveCoverClass(0x4500, 0, (void*)&g_class25_oi4500, sizeof(CLASS25), para_vari_save);
						p_JProgramInfo->oi_changed.oi4500++;//更新oi标志，其他进程检测该标志
					}
					g_curcldno = 1;
				}
			}
			break;
		case ESC:
			if(client.node.child!=NULL)
				free(client.node.child);
			return;
		}
		if(PressKey!=NOKEY||first_flg==0){
			memcpy(&rect, &rect_Client, sizeof(Rect));
			rect.left = rect_Client.left + FONTSIZE*8 - 8;
			gui_clrrect(rect);
			tmpnode = client.node.child;
			tmp = settx_showlabel(2);
			while(tmpnode->next!=NULL){
				tmpnode = tmpnode->next;
				cur_form = list_entry(tmpnode, Form, node);
				if(tmp==1){
					if(list_getListIndex(client.node.child, tmpnode)>=list_getListIndex(client.node.child, cur_node))
						cur_form->pfun_show(cur_form);
				}else
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
	return;
}

//16字节的ip换成12字节
void paraip_gettext(char *para_ip, char *text)
{
	int i, iip[4];
	char ip[4][4];
	if(para_ip[0]==0)
		return;
	memset(ip, 0, 16);
	sscanf(para_ip,"%d.%d.%d.%d",&iip[0],&iip[1],&iip[2],&iip[3]);
	for(i=0; i<4; i++){
		sprintf(ip[i], "%03d", iip[i]);
		strcat(text, ip[i]);
	}
}
//12字节的ip转换程16字节的ip
void eidtip16_gettext(Edit *edit, char *text)
{
	char textTemp[16];
	int i, j=0;

	char ip1[4],ip2[4],ip3[4],ip4[4];
	memset(textTemp,0,16);
	memset(ip1,0,4);
	memset(ip2,0,4);
	memset(ip3,0,4);
	memset(ip4,0,4);
	for(i = 0; i < 12 ; i++)
	{
		if(i % 3 == 0 && i != 0)
		{
			textTemp[i + j] = ' ';
			j++;
		}
		textTemp[i + j] = edit->form.key[i].c;
	}
	DEBUG_TIME_LINE("textTemp = %s\n",textTemp);
	sscanf(textTemp,"%s %s %s %s",ip1,ip2,ip3,ip4);
	DEBUG_TIME_LINE("ip1 = %s,ip2 = %s,ip3 = %s,ip4 = %s\n",ip1,ip2,ip3,ip4);
	sprintf(text,"%d.%d.%d.%d",atoi(ip1),atoi(ip2),atoi(ip3),atoi(ip4));
	DEBUG_TIME_LINE("text = %s",text);

	return;
}
void eidtip12_gettext(Edit *edit, char *text)
{
	int i;
	for(i=0; i<12; i++)
		text[i] = edit->form.key[i].c;
	return;
}

void edit_gettext(Edit *edit, char *text,char len)
{
	int i;
	for(i = 0;i<len;i++)
	{
		text[i] = edit->form.key[i].c;
	}
	return;
}

int setip_showlabel(char type){
	Point label_pos, pos;
	gui_setpos(&pos, rect_Client.left+7*FONTSIZE, rect_Client.top+FONTSIZE+ROW_INTERVAL*2);
	gui_textshow((char *)"主站IP地址", pos, LCD_NOREV);
	gui_setpos(&label_pos, rect_Client.left+2*FONTSIZE, rect_Client.top);
	label_pos.y += FONTSIZE*5 + ROW_INTERVAL;
	gui_textshow((char *)"主用IP:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"主端口:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"备用IP:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"备端口:", label_pos, LCD_NOREV);
	if(2 == type)
	{
		label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
		gui_textshow((char *)"A P N:", label_pos, LCD_NOREV);
	}
	return 0;
}

void menu_netmaster()
{
	int i;
	int tmp=0;
	char first_flg=0;
	struct list *cur_node=NULL, *tmpnode=NULL;
	Form *cur_form=NULL, client;//client 液晶显示客户区
	char str[INPUTKEYNUM];
	char ip_chg[20];
	int foo[4];
	Rect rect;
	Edit edit_masterip,edit_masterport,edit_salveip,edit_salveport;
	memset(&edit_masterip, 0, sizeof(Edit));
	memset(&edit_masterport, 0, sizeof(Edit));
	memset(&edit_salveip, 0, sizeof(Edit));
	memset(&edit_salveport, 0, sizeof(Edit));

	memset(&client, 0, sizeof(Form));
	client.node.child = (struct list*)malloc(sizeof(struct list));
	if(client.node.child==NULL){
		g_curcldno = 1;//上状态栏显示
		return;
	}
	memset(client.node.child, 0, sizeof(struct list));
	gui_clrrect(rect_Client);
	//往控件里放入测量点数据 应该在控件init里设置
	Point pos;
	gui_setpos(&pos, rect_Client.left+9.5*FONTSIZE, rect_Client.top);

	pos.y += FONTSIZE*5 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);

	if (g_chgOI4510 != p_JProgramInfo->oi_changed.oi4510) {
		g_chgOI4510 = p_JProgramInfo->oi_changed.oi4510;
		readCoverClass(0x4510, 0, (void*)&g_class26_oi4510,sizeof(CLASS26), para_vari_save);
	}
	sprintf(ip_chg,"%d.%d.%d.%d",g_class26_oi4510.master.master[0].ip[1],g_class26_oi4510.master.master[0].ip[2],
	                             g_class26_oi4510.master.master[0].ip[3],g_class26_oi4510.master.master[0].ip[4]);
	paraip_gettext(ip_chg, str);
	editip_init(&edit_masterip, CTRL_EDITIP, str, 12, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//主IP
	//------------------------------------------dbg_prt("\n edit_cldno.node=%x prev=%x next=%x",(int)&edit_cldno.node, (int)edit_cldno.node.prev,(int)edit_cldno.node.next);

	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	sprintf(str, "%05d", g_class26_oi4510.master.master[0].port);
	edit_init(&edit_masterport, str, 5, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//主端口
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	memset(ip_chg,0,sizeof(ip_chg));
	sprintf(ip_chg,"%d.%d.%d.%d",g_class26_oi4510.master.master[1].ip[1],g_class26_oi4510.master.master[1].ip[2],
	                             g_class26_oi4510.master.master[1].ip[3],g_class26_oi4510.master.master[1].ip[4]);
	paraip_gettext(ip_chg, str);
	editip_init(&edit_salveip, CTRL_EDITIP, str, 12, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//备IP
	memset(ip_chg,0,sizeof(ip_chg));
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	sprintf(str, "%05d", g_class26_oi4510.master.master[1].port);
	edit_init(&edit_salveport, str, 5, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//备端口
	//------------------------------------------
	cur_node = &edit_masterip.form.node;
	cur_form = &edit_masterip.form;
	setip_showlabel(1);//显示各个控件的标签
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
//					cur_node = list_getfirst(cur_node);
					if(msgbox_label((char *)"保存参数?", CTRL_BUTTON_OK)==ACK){
						eidtip16_gettext(&edit_masterip, ip_chg);
						memset(foo,0,4);
						sscanf(ip_chg,"%d.%d.%d.%d",&foo[0],&foo[1],&foo[2],&foo[3]);
						for(i = 0;i<4;i++)
						{
							if(foo[i]>255)
							{
								foo[i] = 255;
							}
							g_class26_oi4510.master.master[0].ip[i+1] = foo[i];
						}
//						sscanf(ip_chg,"%d.%d.%d.%d",(int*)&Class26.master.master[0].ip[1],(int*)&Class26.master.master[0].ip[2],
//						                            (int*)&Class26.master.master[0].ip[3],(int*)&Class26.master.master[0].ip[4]);
						memset(str, 0, INPUTKEYNUM);
						memset(ip_chg,0,sizeof(ip_chg));
						eidt_gettext(&edit_masterport, str);
						g_class26_oi4510.master.master[0].port = atoi(str);
						eidtip16_gettext(&edit_salveip, ip_chg);
						memset(foo,0,4);
						sscanf(ip_chg,"%d.%d.%d.%d",&foo[0],&foo[1],&foo[2],&foo[3]);
						for(i = 0;i<4;i++)
						{
							if(foo[i]>255)
							{
								foo[i] = 255;
							}
							g_class26_oi4510.master.master[1].ip[i+1] = foo[i];
						}
//						sscanf(ip_chg,"%d.%d.%d.%d",(int*)&Class26.master.master[1].ip[1],(int*)&Class26.master.master[1].ip[2],
//													(int*)&Class26.master.master[1].ip[3],(int*)&Class26.master.master[1].ip[4]);
						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_salveport, str);
						g_class26_oi4510.master.master[1].port = atoi(str);
						saveCoverClass(0x4510, 0, (void*)&g_class26_oi4510, sizeof(CLASS26), para_vari_save);
						p_JProgramInfo->oi_changed.oi4510++;//更新oi标志，其他进程检测该标志
					}
					g_curcldno = 1;
				}
			}
			break;
		case ESC:
			if(client.node.child!=NULL)
				free(client.node.child);
			return;
		}
		if(PressKey!=NOKEY||first_flg==0){
			memcpy(&rect, &rect_Client, sizeof(Rect));
			rect.left = rect_Client.left + FONTSIZE*8 - 8;
			gui_clrrect(rect);
			tmpnode = client.node.child;
			tmp = setip_showlabel(1);
			while(tmpnode->next!=NULL){
				tmpnode = tmpnode->next;
				cur_form = list_entry(tmpnode, Form, node);
				if(tmp==1){
					if(list_getListIndex(client.node.child, tmpnode)>=list_getListIndex(client.node.child, cur_node))
						cur_form->pfun_show(cur_form);
				}else
					cur_form->pfun_show(cur_form);
			}
			cur_form = list_entry(cur_node, Form, node);//根据链表节点找到控件指针
			cur_form->focus = FOCUS;
//			dbg_prt("\n cur_form:(%d)", cur_form->CtlType);
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
	return;
}

void menu_wlanmaster()
{
	int i;
	int tmp=0;
	char first_flg=0;
	struct list *cur_node=NULL, *tmpnode=NULL;
	Form *cur_form=NULL, client;//client 液晶显示客户区
	char str[INPUTKEYNUM];
	char ip_chg[20];
	int foo[4];
	Rect rect;
	Edit edit_masterip,edit_masterport,edit_salveip,edit_salveport,edit_apn;
	memset(&edit_masterip, 0, sizeof(Edit));
	memset(&edit_masterport, 0, sizeof(Edit));
	memset(&edit_salveip, 0, sizeof(Edit));
	memset(&edit_salveport, 0, sizeof(Edit));
	memset(&edit_apn, 0, sizeof(Edit));
	memset(&client, 0, sizeof(Form));

	client.node.child = (struct list*)malloc(sizeof(struct list));
	if(client.node.child==NULL){
		g_curcldno = 1;//上状态栏显示
		return;
	}
	memset(client.node.child, 0, sizeof(struct list));
	gui_clrrect(rect_Client);
	//往控件里放入测量点数据 应该在控件init里设置
	Point pos;
	gui_setpos(&pos, rect_Client.left+9.5*FONTSIZE, rect_Client.top);

	pos.y += FONTSIZE*5 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);

	if (g_chgOI4500 != p_JProgramInfo->oi_changed.oi4500) {
		g_chgOI4500 = p_JProgramInfo->oi_changed.oi4500;
		readCoverClass(0x4500, 0, (void*)&g_class25_oi4500,sizeof(CLASS25), para_vari_save);
	}

	sprintf(ip_chg,"%d.%d.%d.%d",g_class25_oi4500.master.master[0].ip[1],g_class25_oi4500.master.master[0].ip[2],
	                             g_class25_oi4500.master.master[0].ip[3],g_class25_oi4500.master.master[0].ip[4]);
	paraip_gettext(ip_chg, str);
	editip_init(&edit_masterip, CTRL_EDITIP, str, 12, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//主IP
	//------------------------------------------dbg_prt("\n edit_cldno.node=%x prev=%x next=%x",(int)&edit_cldno.node, (int)edit_cldno.node.prev,(int)edit_cldno.node.next);

	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	sprintf(str, "%05d", g_class25_oi4500.master.master[0].port);
	edit_init(&edit_masterport, str, 5, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//主端口
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	memset(ip_chg,0,sizeof(ip_chg));
	sprintf(ip_chg,"%d.%d.%d.%d",g_class25_oi4500.master.master[1].ip[1],g_class25_oi4500.master.master[1].ip[2],
	                             g_class25_oi4500.master.master[1].ip[3],g_class25_oi4500.master.master[1].ip[4]);
	paraip_gettext(ip_chg, str);
	editip_init(&edit_salveip, CTRL_EDITIP, str, 12, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//备IP
	memset(ip_chg,0,sizeof(ip_chg));
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	sprintf(str, "%05d", g_class25_oi4500.master.master[1].port);
	edit_init(&edit_salveport, str, 5, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//备端口
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, ' ', INPUTKEYNUM);
//	if(strlen((char*)ParaAll->f3.APN)!=0)
		strcpy(str,(char*) &g_class25_oi4500.commconfig.apn[1]);
//	else
//		memcpy(str, "        ", 8);
	edit_init(&edit_apn, str, 16, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_ASC);//apn
	//------------------------------------------
	cur_node = &edit_masterip.form.node;
	cur_form = &edit_masterip.form;
	setip_showlabel(2);//显示各个控件的标签
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
//					cur_node = list_getfirst(cur_node);
					if(msgbox_label((char *)"保存参数?", CTRL_BUTTON_OK)==ACK){
						eidtip16_gettext(&edit_masterip, ip_chg);
						memset(foo,0,4);
						sscanf(ip_chg,"%d.%d.%d.%d",&foo[0],&foo[1],&foo[2],&foo[3]);
						for(i = 0;i<4;i++)
						{
							if(foo[i]>255)
							{
								foo[i] = 255;
							}
							g_class25_oi4500.master.master[0].ip[i+1] = foo[i];
						}
//						sscanf(ip_chg,"%d.%d.%d.%d",(int*)&Class25.master.master[0].ip[1],(int*)&Class25.master.master[0].ip[2],
//								(int*)&Class25.master.master[0].ip[3],(int*)&Class25.master.master[0].ip[4]);
						memset(str, 0, INPUTKEYNUM);
						memset(ip_chg,0,sizeof(ip_chg));
						eidt_gettext(&edit_masterport, str);
						g_class25_oi4500.master.master[0].port = atoi(str);
						eidtip16_gettext(&edit_salveip, ip_chg);
						memset(foo,0,4);
						sscanf(ip_chg,"%d.%d.%d.%d",&foo[0],&foo[1],&foo[2],&foo[3]);
						for(i = 0;i<4;i++)
						{
							if(foo[i]>255)
							{
								foo[i] = 255;
							}
							g_class25_oi4500.master.master[1].ip[i+1] = foo[i];
						}
//						sscanf(ip_chg,"%d.%d.%d.%d",(int*)&Class25.master.master[1].ip[1],(int*)&Class25.master.master[1].ip[2],
//													(int*)&Class25.master.master[1].ip[3],(int*)&Class25.master.master[1].ip[4]);
						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_salveport, str);
						g_class25_oi4500.master.master[1].port = atoi(str);
						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_apn, str);
						strcpy((char*)&g_class25_oi4500.commconfig.apn[1], str);
						g_class25_oi4500.commconfig.apn[0] = (INT8U)(strlen(str));
//						DEBUG_TIME_LINE("\nthe len of apn %s is %d\n",&Class25.commconfig.apn[1],Class25.commconfig.apn[0]);
						write_apn((char*)&g_class25_oi4500.commconfig.apn[1]);
						saveCoverClass(0x4500, 0, (void*)&g_class25_oi4500, sizeof(CLASS25), para_vari_save);
						p_JProgramInfo->oi_changed.oi4500++;//更新oi标志，其他进程检测该标志
					}
					g_curcldno = 1;
				}
			}
			break;
		case ESC:
			if(client.node.child!=NULL)
				free(client.node.child);
			return;
		}
		if(PressKey!=NOKEY||first_flg==0){
			memcpy(&rect, &rect_Client, sizeof(Rect));
			rect.left = rect_Client.left + FONTSIZE*8 - 8;
			gui_clrrect(rect);
			tmpnode = client.node.child;
			tmp = setip_showlabel(2);
			while(tmpnode->next!=NULL){
				tmpnode = tmpnode->next;
				cur_form = list_entry(tmpnode, Form, node);
				if(tmp==1){
					if(list_getListIndex(client.node.child, tmpnode)>=list_getListIndex(client.node.child, cur_node))
						cur_form->pfun_show(cur_form);
				}else
					cur_form->pfun_show(cur_form);
			}
			cur_form = list_entry(cur_node, Form, node);//根据链表节点找到控件指针
			cur_form->focus = FOCUS;
//			dbg_prt("\n cur_form:(%d)", cur_form->CtlType);
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
	return;
}
//IP配置、终端IP、子网掩码、网关地址、侦听端口、PPPoE用户名、PPPoE密码
int seteth0_showlabel(){
	Point label_pos, pos;
	gui_setpos(&pos, rect_Client.left+8*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"以太网参数", pos, LCD_NOREV);
	gui_setpos(&label_pos, rect_Client.left+FONTSIZE, rect_Client.top);
	label_pos.y += FONTSIZE*3 + ROW_INTERVAL;
	gui_textshow((char *)" IP 配置:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"终端 I P:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"子网掩码:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"网关地址:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"侦听端口:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"PPPoE账号:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"PPPoE密码:", label_pos, LCD_NOREV);
	return 0;
}

void menu_eth0para(){
	char cmd[50],cmd1[100], input_valid=1;
	INT8U  sip[16];
	char cb_text[TEXTLEN_X][TEXTLEN_Y];//用于存放combox的元素
	int tmp=0;
	char first_flg=0;
	struct list *cur_node=NULL, *tmpnode=NULL;
	Form *cur_form=NULL, client;//client 液晶显示客户区
	char str[INPUTKEYNUM];
	Rect rect;
	Edit edit_termip, edit_netmask, edit_gateway, edit_listenport,edit_pppoe_name,edit_pppoe_pwd;
	Combox cb_IP_config_mode;

	memset(&edit_termip, 0, sizeof(Edit));
	memset(&edit_netmask, 0, sizeof(Edit));
	memset(&edit_gateway, 0, sizeof(Edit));
	memset(&edit_listenport, 0, sizeof(Edit));
	memset(&client, 0, sizeof(Form));
	client.node.child = (struct list*)malloc(sizeof(struct list));
	if(client.node.child==NULL){
		g_curcldno = 1;//上状态栏显示
		return;
	}
	memset(client.node.child, 0, sizeof(struct list));
	gui_clrrect(rect_Client);
	//往控件里放入测量点数据 应该在控件init里设置
	Point pos;

	if (g_chgOI4510 != p_JProgramInfo->oi_changed.oi4510) {
		g_chgOI4510 = p_JProgramInfo->oi_changed.oi4510;
		readCoverClass(0x4510, 0, (void*)&g_class26_oi4510,sizeof(CLASS26), para_vari_save);
	}
	gui_setpos(&pos, rect_Client.left+10.5*FONTSIZE, rect_Client.top);
	//IP配置、终端IP、子网掩码、网关地址、侦听端口、PPPoE用户名、PPPoE密码
	//------------------------------------------
	pos.y += FONTSIZE*3 + ROW_INTERVAL;
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "DHCP", strlen("DHCP"));
	memcpy(cb_text[1], "静态", strlen("静态"));
	memcpy(cb_text[2], "PPPoE", strlen("PPPoE"));
	tmp = g_class26_oi4510.IP.ipConfigType;//DHCP：0，静态：1,PPPoE：2
	if(tmp<0)
		tmp = 1;
	combox_init(&cb_IP_config_mode, tmp, cb_text, pos, NORETFLG,client.node.child);
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	ip2asc(&g_class26_oi4510.IP.ip[1], str);
	editip_init(&edit_termip, CTRL_EDITIP, str, 12, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//主IP
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	ip2asc(&g_class26_oi4510.IP.subnet_mask[1], str);
	editip_init(&edit_netmask, CTRL_EDITIP, str, 12, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//掩码
	//-----------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	ip2asc(&g_class26_oi4510.IP.gateway[1], str);
	editip_init(&edit_gateway, CTRL_EDITIP, str, 12, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//网关
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	sprintf(str, "%05d", g_class26_oi4510.commconfig.listenPort[0]);
	edit_init(&edit_listenport, str, 5, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//监听端口列表1
	//-------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	memcpy(str,&g_class26_oi4510.IP.username_pppoe[1],g_class26_oi4510.IP.username_pppoe[0]);
	edit_init(&edit_pppoe_name, str, 15, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//PPPoE用户名
	//-------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	memcpy(str,&g_class26_oi4510.IP.password_pppoe[1],g_class26_oi4510.IP.password_pppoe[0]);
	edit_init(&edit_pppoe_pwd, str, 15, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//PPPoE密码
	//-------------------------------------------
	cur_node = &cb_IP_config_mode.form.node;
	cur_form = &edit_termip.form;
	seteth0_showlabel();//显示各个控件的标签
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
			if(get_oprmode()==OPRMODE_MODIFY){//如果用户选择了设置模式
				cur_form->pfun_process(cur_form);
				if(cur_form->pfun_process_ret==OK){//按下了确认键
//					cur_node = list_getfirst(cur_node);
					if(msgbox_label((char *)"保存参数?", CTRL_BUTTON_OK)==ACK){

						g_class26_oi4510.IP.ipConfigType = cb_IP_config_mode.cur_index;

						memset(sip, 0, 16);
						eidtip12_gettext(&edit_termip, (char*)sip);//取得编辑框中的ip值
						if(ip_strtobyte(sip, &g_class26_oi4510.IP.ip[1])==0)
							input_valid = 0;
						else{
							memset(cmd, 0, 50);
							sprintf(cmd, "ifconfig eth0 %d.%d.%d.%d up", g_class26_oi4510.IP.ip[1],g_class26_oi4510.IP.ip[2],
									g_class26_oi4510.IP.ip[3],g_class26_oi4510.IP.ip[4]);
							system(cmd);
							memset(cmd1, 0, 100);
							sprintf(cmd1, "echo %s > /nor/rc.d/ip.sh", cmd);
							system(cmd1);
						}

						memset(sip, 0, 16);
						eidtip12_gettext(&edit_netmask, (char*)sip);
						if(ip_strtobyte(sip, &g_class26_oi4510.IP.subnet_mask[1])==0)
							input_valid = 0;

						memset(sip, 0, 16);
						eidtip12_gettext(&edit_gateway, (char*)sip);
						if(ip_strtobyte(sip, &g_class26_oi4510.IP.gateway[1])==0)
							input_valid = 0;

						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_listenport, str);
						g_class26_oi4510.commconfig.listenPort[0] = atoi(str);

						memset(str,0,INPUTKEYNUM);
						eidt_gettext(&edit_pppoe_name, str);
						memcpy(&g_class26_oi4510.IP.username_pppoe[1],str,strlen(str));
						g_class26_oi4510.IP.username_pppoe[0] = strlen(str);

						memset(str,0,INPUTKEYNUM);
						eidt_gettext(&edit_pppoe_pwd,str);
						memcpy(&g_class26_oi4510.IP.password_pppoe[1],str,strlen(str));
						g_class26_oi4510.IP.password_pppoe[0] = strlen(str);

						if(input_valid==1)
						{
							saveCoverClass(0x4510, 0, (void*)&g_class26_oi4510, sizeof(CLASS26), para_vari_save);
							p_JProgramInfo->oi_changed.oi4510++;//更新oi标志，其他进程检测该标志
						}
						else
							msgbox_label((char *)"保存失败！", CTRL_BUTTON_OK);
					}
					g_curcldno = 1;
				}
			}
			break;
		case ESC:
			if(client.node.child!=NULL)
				free(client.node.child);
			return;
		}
		if(PressKey!=NOKEY||first_flg==0){
			memcpy(&rect, &rect_Client, sizeof(Rect));
			rect.left = rect_Client.left + FONTSIZE*8 - 8;
			gui_clrrect(rect);
			tmpnode = client.node.child;
			tmp = seteth0_showlabel();
			while(tmpnode->next!=NULL){
				tmpnode = tmpnode->next;
				cur_form = list_entry(tmpnode, Form, node);
				if(tmp==1){
					if(list_getListIndex(client.node.child, tmpnode)>=list_getListIndex(client.node.child, cur_node))
						cur_form->pfun_show(cur_form);
				}else
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
	return;
}

int setphone_showlabel(){
	Point label_pos, pos;
	gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+FONTSIZE+ROW_INTERVAL*2);
	gui_textshow((char *)"短信中心", pos, LCD_NOREV);
	gui_setpos(&label_pos, rect_Client.left, rect_Client.top);
	label_pos.y += FONTSIZE*5 + ROW_INTERVAL;
	gui_textshow((char *)"主站号码:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"短信中心:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"用 户 名:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"密    码:", label_pos, LCD_NOREV);
	return 0;
}

void menu_jzqtelephone()
{
	int tmp=0;
	char first_flg=0;
	struct list *cur_node=NULL, *tmpnode=NULL;
	Form *cur_form=NULL, client;//client 液晶显示客户区
	char str[INPUTKEYNUM];
	Rect rect;
	Edit edit_phonenum, edit_msgcenter, edit_user, edit_mima;

	memset(&edit_phonenum, 0, sizeof(Edit));
	memset(&edit_msgcenter, 0, sizeof(Edit));
	memset(&edit_user, 0, sizeof(Edit));
	memset(&edit_mima, 0, sizeof(Edit));
	memset(&client, 0, sizeof(Form));
	client.node.child = (struct list*)malloc(sizeof(struct list));
	if(client.node.child==NULL){
		g_curcldno = 1;//上状态栏显示
		return;
	}
	memset(client.node.child, 0, sizeof(struct list));
	gui_clrrect(rect_Client);
	Point pos;
	pos.x = rect_Client.left+FONTSIZE*9.5;
	pos.y = rect_Client.top;

	pos.y += FONTSIZE*5 + ROW_INTERVAL;

	if (g_chgOI4500 != p_JProgramInfo->oi_changed.oi4500) {
		g_chgOI4500 = p_JProgramInfo->oi_changed.oi4500;
		g_chgOI4500 = p_JProgramInfo->oi_changed.oi4500;
		readCoverClass(0x4500, 0, (void*)&g_class25_oi4500, sizeof(CLASS25), para_vari_save);
	}

	memset(str, 0, INPUTKEYNUM);
	memcpy(str,g_class25_oi4500.sms.master[0],VISIBLE_STRING_LEN);//主站号码

	edit_init(&edit_phonenum, str, 16, pos,NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	memcpy(str,g_class25_oi4500.sms.center,VISIBLE_STRING_LEN);//短信中心号码

	edit_init(&edit_msgcenter, str, 16, pos,NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);
	//-------------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, ' ', INPUTKEYNUM);
	//if(para_f7.Len_UsrName==0){
	if(g_class25_oi4500.commconfig.userName[0]==0){
		memcpy(str, "USERNAME", strlen("USERNAME"));
	}else
		memcpy(str,&g_class25_oi4500.commconfig.userName[1],VISIBLE_STRING_LEN);
	edit_init(&edit_user, str, 16, pos,NOFRAME, NORETFLG, client.node.child,KEYBOARD_ASC);
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, ' ', INPUTKEYNUM);
	//if(para_f7.Len_Pwd==0){
	if(g_class25_oi4500.commconfig.passWord[0]==0){
		memcpy(str, "000000", strlen("000000"));
	}else
		memcpy(str,&g_class25_oi4500.commconfig.passWord[1],VISIBLE_STRING_LEN);
	edit_init(&edit_mima, str, 16, pos,NOFRAME, NORETFLG, client.node.child,KEYBOARD_ASC);
	//------------------------------------------
	cur_node = &edit_phonenum.form.node;
	cur_form = &edit_phonenum.form;
	setphone_showlabel();//显示各个控件的标签
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
//					cur_node = list_getfirst(cur_node);
					if(msgbox_label((char *)"保存参数?", CTRL_BUTTON_OK)==ACK){
						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_phonenum, str);
						memcpy(g_class25_oi4500.sms.master[0],str,strlen(str));

						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_msgcenter, str);
						memcpy(g_class25_oi4500.sms.center,str,strlen(str));

						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_user, str);
						memcpy(&g_class25_oi4500.commconfig.userName[1],str,strlen(str));
						g_class25_oi4500.commconfig.userName[0] = strlen(str);

						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_mima, str);
						memcpy(&g_class25_oi4500.commconfig.passWord[1],str,strlen(str));
						g_class25_oi4500.commconfig.passWord[0] = strlen(str);
						write_userpwd(&g_class25_oi4500.commconfig.userName[1],&g_class25_oi4500.commconfig.passWord[1],
								      &g_class25_oi4500.commconfig.apn[1]);
						saveCoverClass(0x4500, 0, (void*)&g_class25_oi4500, sizeof(CLASS25), para_vari_save);
						p_JProgramInfo->oi_changed.oi4500++;//更新oi标志，其他进程检测该标志
					}
					g_curcldno = 1;
				}
			}
			break;
		case ESC:
			if(client.node.child!=NULL)
				free(client.node.child);
			return;
		}
		if(PressKey!=NOKEY||first_flg==0){
			memcpy(&rect, &rect_Client, sizeof(Rect));
			rect.left = rect_Client.left + FONTSIZE*8 - 8;
			gui_clrrect(rect);
			tmpnode = client.node.child;
			tmp = setphone_showlabel();
			while(tmpnode->next!=NULL){
				tmpnode = tmpnode->next;
				cur_form = list_entry(tmpnode, Form, node);
				if(tmp==1){
					if(list_getListIndex(client.node.child, tmpnode)>=list_getListIndex(client.node.child, cur_node))
						cur_form->pfun_show(cur_form);
				}else
					cur_form->pfun_show(cur_form);
			}
			cur_form = list_entry(cur_node, Form, node);//根据链表节点找到控件指针
			cur_form->focus = FOCUS;
//			dbg_prt("\n cur_form:(%d)", cur_form->CtlType);
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
	return;
}
//地址类型、逻辑地址、终端地址
int setid_showlabel(){
	Point label_pos, pos;
	gui_setpos(&pos, rect_Client.left+8*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char *)"终端地址配置:", pos, LCD_NOREV);
	gui_setpos(&label_pos, rect_Client.left+FONTSIZE, rect_Client.top+FONTSIZE*3 + ROW_INTERVAL);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"地址类型:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"逻辑地址:", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"终端地址:", label_pos, LCD_NOREV);
	return 0;
}

void jzq_id_edit()
{
	INT8U str[50] = {0};
	INT8U sever_addr[OCTET_STRING_LEN-1];//去掉首部第一个代表长度字节
	INT8U addr_len = 0;
	char idtmp[30] = {0}, oprmode_old=0;

	memset(sever_addr,0,sizeof(sever_addr));
	memset(idtmp, 0, sizeof(idtmp));
	memset(str,0,sizeof(str));

	if (g_chgOI4001 != p_JProgramInfo->oi_changed.oi4001) {
		g_chgOI4500 = p_JProgramInfo->oi_changed.oi4001;
		readCoverClass(0x4001, 0, (void*)&g_Class4001_4002_4003, 	sizeof(CLASS_4001_4002_4003), para_vari_save);
	}

	bcd2str(&g_Class4001_4002_4003.curstom_num[1],\
			(INT8U*)str,g_Class4001_4002_4003.curstom_num[0],\
			sizeof(str),positive);
	sscanf((char*)str,"%[0-9]",idtmp);
	DEBUG_TIME_LINE("terminal id: %s\n", str);

	oprmode_old = get_oprmode();
	set_oprmode(OPRMODE_MODIFY);
	if(msgbox_jzqaddr_10(idtmp, 16)==ACK){
		memset(str, 0, sizeof(str));
		if(idtmp[0] == ' ')
			sscanf(idtmp,"%*[^0-9]%[0-9]",str);//为空时剔除空字节
		else
			sscanf(idtmp,"%[0-9]",str);

		addr_len = strlen((char*)str);
		if(addr_len!=0){
			str2bcd(str,sever_addr,sizeof(sever_addr));
			memcpy(&g_Class4001_4002_4003.curstom_num[1],sever_addr,OCTET_STRING_LEN-1);
			if(addr_len%2)
			{
				g_Class4001_4002_4003.curstom_num[addr_len/2+1] |= 0x0F;
				g_Class4001_4002_4003.curstom_num[0] = addr_len/2+1;
			}
			else{
				g_Class4001_4002_4003.curstom_num[0] = addr_len/2;
			}
			syslog(LOG_NOTICE,"chg 4001_addr[(%02x)%02x_%02x_%02x_%02x_%02x_%02x]",g_Class4001_4002_4003.curstom_num[0],
					g_Class4001_4002_4003.curstom_num[1],g_Class4001_4002_4003.curstom_num[2],g_Class4001_4002_4003.curstom_num[3],
					g_Class4001_4002_4003.curstom_num[4],g_Class4001_4002_4003.curstom_num[5],g_Class4001_4002_4003.curstom_num[6]);
			saveCoverClass(0x4001, 0, (void*)&g_Class4001_4002_4003, sizeof(CLASS_4001_4002_4003), para_vari_save);
			saveCoverClass(0x4001, 0, (void*)&g_Class4001_4002_4003, sizeof(CLASS_4001_4002_4003), para_init_save);
			p_JProgramInfo->oi_changed.oi4001++;
			msgbox_label((char *)"设置成功！", CTRL_BUTTON_OK);
		}else
			msgbox_label((char *)"设置失败！", CTRL_BUTTON_OK);
	}
	set_oprmode(oprmode_old);
}

void show_jzq_ver()
{
    char	str[128] = {0};
    Point 	pos = {0};

    memset(str,0,sizeof(str));

	if (g_chgOI4300 != p_JProgramInfo->oi_changed.oi4300) {
		g_chgOI4300 = p_JProgramInfo->oi_changed.oi4300;
		g_chgOI4300 = p_JProgramInfo->oi_changed.oi4300;
		readCoverClass(0x4300, 0, (void*)&g_class19_oi4300, sizeof(CLASS19), para_vari_save);
	}

    gui_clrrect(rect_Client);
    gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+FONTSIZE);
    gui_textshow((char*)"终端信息", pos, LCD_NOREV);

    pos.x = rect_Client.left + FONTSIZE*3;
    pos.y += FONTSIZE*3;
    memcpy(str,&g_class19_oi4300.name[1],g_class19_oi4300.name[0]);
    sprintf(str,"厂商代码:%c%c%c%c",g_class19_oi4300.info.factoryCode[0],g_class19_oi4300.info.factoryCode[1],
    								g_class19_oi4300.info.factoryCode[2],g_class19_oi4300.info.factoryCode[3]);
    gui_textshow(str, pos, LCD_NOREV);

    memset(str,0,sizeof(str));
    pos.y += FONTSIZE*2+2;
    sprintf(str,"软件版本:%c%c%c%c",g_class19_oi4300.info.softVer[0],g_class19_oi4300.info.softVer[1],
    								g_class19_oi4300.info.softVer[2],g_class19_oi4300.info.softVer[3]);
    gui_textshow(str, pos, LCD_NOREV);

    memset(str,0,sizeof(str));
    pos.y += FONTSIZE*2+2;
    sprintf(str,"软件版本日期:%c%c%c%c%c%c",g_class19_oi4300.info.softDate[0],g_class19_oi4300.info.softDate[1],g_class19_oi4300.info.softDate[2],
    									   g_class19_oi4300.info.softDate[3],g_class19_oi4300.info.softDate[4],g_class19_oi4300.info.softDate[5]);
    gui_textshow(str, pos, LCD_NOREV);

    memset(str,0,sizeof(str));
    pos.y += FONTSIZE*2+2;
    sprintf(str,"硬件版本:%c%c%c%c",g_class19_oi4300.info.hardVer[0],g_class19_oi4300.info.hardVer[1],
    								g_class19_oi4300.info.hardVer[2],g_class19_oi4300.info.hardVer[3]);
    gui_textshow(str, pos, LCD_NOREV);

    memset(str,0,sizeof(str));
    pos.y += FONTSIZE*2+2;
    sprintf(str,"硬件版本日期:%c%c%c%c%c%c",g_class19_oi4300.info.hardDate[0],g_class19_oi4300.info.hardDate[1],g_class19_oi4300.info.hardDate[2],
    										g_class19_oi4300.info.hardDate[3],g_class19_oi4300.info.hardDate[4],g_class19_oi4300.info.hardDate[5]);
    gui_textshow(str, pos, LCD_NOREV);

    memset(str,0,sizeof(str));
    pos.y += FONTSIZE*2+2;
    sprintf(str,"生产日期:%d-%d-%d ",g_class19_oi4300.date_Product.year.data,g_class19_oi4300.date_Product.month.data,g_class19_oi4300.date_Product.day.data);
    gui_textshow(str, pos, LCD_NOREV);

    memset(str,0,sizeof(str));
    pos.y += FONTSIZE*2+2;
    sprintf(str,"规约协议:698 ");
    gui_textshow(str, pos, LCD_NOREV);
}

void menu_jzqstatus_showused(char* name, int used, Point pos){
	char str[50];
	memset(str, 0, 50);
	sprintf(str, "%s:%d%%", name,used);
	gui_textshow(str, pos, LCD_NOREV);
}

void show_jzqused(){
	int memused, cpuload, nandused, norused;
	Point pos;
	norused = getNorInfo();
	nandused = getNandInfo();
	memused = getMemInfo();
	cpuload = getCpuInfo();
	gui_clrrect(rect_Client);
	gui_setpos(&pos, rect_Client.left+8*FONTSIZE, rect_Client.top+2*FONTSIZE);
	gui_textshow((char*)"终端使用率", pos, LCD_NOREV);
	gui_setpos(&pos, rect_Client.left+5*FONTSIZE, rect_Client.top+6*FONTSIZE);
	menu_jzqstatus_showused((char*)"系统盘使用率", norused, pos);
	pos.y += 3*FONTSIZE;
	menu_jzqstatus_showused((char*)"数据盘使用率", nandused, pos);
	pos.y += 3*FONTSIZE;
	menu_jzqstatus_showused((char*)"内存使用率", memused, pos);
	pos.y += 3*FONTSIZE;
	menu_jzqstatus_showused((char*)"CPU使用率", cpuload, pos);
	return;
}

void show_jzq_ip(){
	char ip[4] = {0};
    char tmpstr[128] = {0};
    char str[16] = {0};
    memset(str,0,16);
    Point pos = {0};
    gui_clrrect(rect_Client);
    gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+ROW_INTERVAL );
	gui_textshow((char*)"本地IP", pos, LCD_NOREV);
	memset(ip, 0, 4);
	getlocalip(ip);
	gui_setpos(&pos, rect_Client.left+FONTSIZE, rect_Client.top+2*FONTSIZE + 2*ROW_INTERVAL);
	sprintf((char *)tmpstr,"IP : %03d.%03d.%03d.%03d",ip[0],ip[1],ip[2],ip[3]);
	gui_textshow((char*)tmpstr, pos, LCD_NOREV);
	//MAC地址
	FILE *fpmac = NULL;
	INT8U TempBuf[60] = {0};
	char	str1[16] = {0}, str2[16] = {0}, str3[16] = {0}, str4[16] = {0};
	char	mac[16] = {0};
	memset(TempBuf, 0, 60);
	memset(tmpstr,0,sizeof(tmpstr));
	sprintf((char *) TempBuf, "/etc/rc.d/mac.sh");
	fpmac = fopen((char *) TempBuf, "r");
	if (fpmac!=NULL){
		fscanf(fpmac,"%s %s %s %s %s",str1,str2,str3,str4,mac);
		fclose(fpmac);
	}
	sprintf((char *)tmpstr,"MAC: %C%C:%C%C:%C%C:%C%C:%C%C:%C%C",mac[0],mac[1],mac[2],mac[3],
			mac[4],mac[5],mac[6],mac[7],mac[8],mac[9],mac[10],mac[11]);
	gui_setpos(&pos, rect_Client.left+FONTSIZE, rect_Client.top+4*FONTSIZE + 3*ROW_INTERVAL );
	gui_textshow((char*)tmpstr, pos, LCD_NOREV);
	return;
}

void menu_jzqstatus(){
	int pageno=0;
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		switch(PressKey){
		case LEFT:
		case UP:
			pageno--;
			if(pageno<0)
				pageno = 2;
			break;
		case RIGHT:
		case DOWN:
			pageno++;
			if(pageno>2)
				pageno = 0;
			break;
		case ESC:
			return;
		}
		switch(pageno) {
		case 0:
			show_jzq_ver();
			break;
		case 1:
			show_jzqused();
			break;
		case 2:
			show_jzq_ip();
			break;

		}
		PressKey = NOKEY;
		delay(200);
	}
	return;
}
/*
 * 获取以太网IP地址
 * */
char get_inet_ip(char* eth,char* ip_addr)
{
	int sock_fd;
	struct sockaddr_in my_addr;
	struct ifreq ifr;
	if((sock_fd = socket(PF_INET,SOCK_DGRAM,0)) == -1)
	{
		perror("socket open error");
		return 0;
	}
	strncpy(ifr.ifr_name,eth,IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ-1] = '\0';
	if(ioctl(sock_fd,SIOCGIFADDR,&ifr) < 0)
	{
		perror("NO such device");
		return 0;
	}
	memcpy(&my_addr,&ifr.ifr_addr,sizeof(my_addr));
	strcpy(ip_addr,inet_ntoa(my_addr.sin_addr));
	close(sock_fd);
	return 1;
}

void menu_termip(){
	char ip[20];
	char ETH[5] = "eth0";
	char no_point_ip[20];
	char cmd[50], cmd1[100], s_ip[4][4];
	int iIP[4];

	memset(iIP,0,sizeof(iIP));
	memset(ip,0,sizeof(ip));
	if(get_inet_ip(ETH,ip)==1) {
		sscanf(ip,"%d.%d.%d.%d",&iIP[0],&iIP[1],&iIP[2],&iIP[3]);
		if(iIP[0]>255 || iIP[1]>255 || iIP[2]>255 || iIP[3]>255) {
			syslog(LOG_ERR,"ip error[%s],insmod macb.ko",ip);
			msgbox_label((char *)"IP获取失败，重新获取", CTRL_BUTTON_OK);
			system("rmmod /lib/macb.ko");
			sleep(2);
			system("insmod /lib/macb.ko");
			sleep(1);
			system("/etc/rc.d/ip.sh");
		}else {
			sprintf(no_point_ip,"%03d%03d%03d%03d",iIP[0],iIP[1],iIP[2],iIP[3]);
			if(msgbox_jzqip(no_point_ip, 12)==ACK){
				//设置ip
				memset(s_ip, 0, 4*4);
				memcpy(&s_ip[0][0], &no_point_ip[0], 3);
				memcpy(&s_ip[1][0], &no_point_ip[3], 3);
				memcpy(&s_ip[2][0], &no_point_ip[6], 3);
				memcpy(&s_ip[3][0], &no_point_ip[9], 3);
				memset(cmd, 0, 50);
				sprintf(cmd, "ifconfig eth0 %d.%d.%d.%d up", atoi(&s_ip[0][0]),atoi(&s_ip[1][0]),
						atoi(&s_ip[2][0]),atoi(&s_ip[3][0]));
				system(cmd);
				memset(cmd1, 0, 100);
				sprintf(cmd1, "echo %s > /nor/rc.d/ip.sh", cmd);
				system(cmd1);
				//修改4510的参数配置
			}
		}
	}else {
		syslog(LOG_ERR,"get ip error[%s],insmod macb.ko and run ip.sh",ip);
		msgbox_label((char *)"IP失败，重新获取", CTRL_BUTTON_OK);
		system("rmmod /lib/macb.ko");
		sleep(2);
		system("insmod /lib/macb.ko");
		sleep(1);
		system("/etc/rc.d/ip.sh");
	}
}

#define REQUEST_GPRSIP 1

void menu_gprsip(){
	char s_gprsip[20];
	Point pos;
	memset(s_gprsip, 0, 20);

	if (g_chgOI4500 != p_JProgramInfo->oi_changed.oi4500) {
		g_chgOI4500 = p_JProgramInfo->oi_changed.oi4500;
		g_chgOI4500 = p_JProgramInfo->oi_changed.oi4500;
		readCoverClass(0x4500, 0, (void*)&g_class25_oi4500, sizeof(CLASS25), para_vari_save);
	}

	gui_clrrect(rect_Client);
	gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+8*FONTSIZE);
	memset(s_gprsip, 0, 20);
	sprintf(s_gprsip, "GPRS IP:");
	gui_textshow(s_gprsip, pos, LCD_NOREV);
	memset(s_gprsip, 0, 20);
	sprintf(s_gprsip,"%d.%d.%d.%d",g_class25_oi4500.pppip[1],g_class25_oi4500.pppip[2],g_class25_oi4500.pppip[3],g_class25_oi4500.pppip[4]);
	pos.x = rect_Client.left + FONTSIZE*4;
	pos.y += FONTSIZE*3;
	gui_textshow(s_gprsip, pos, LCD_NOREV);
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		if(PressKey==ESC)
			break;
		PressKey = NOKEY;
		delay(300);
	}
}
void menu_lcdcontrast(){
	modify_lcd_contrast();
	return;
}

void menu_rtcpower(){
    float v1=0,v2=0;
	bettery_getV(&v1,&v2);
	char str[15];
	sprintf(str,"时钟:%.1fV",v1);
	msgbox_label((char*)str, CTRL_BUTTON_OK);
	return;
}

void menu_impsoe(){
//	INT8U first_flg=0;
//	char tmpstr[50];
//	INT8U soetype;
//	int curpoint1;//重要
//	Point pos;
//	curpoint1 = shmm_getdevstat()->erc.EC1-1;
//	if(curpoint1<0)
//		curpoint1 = 0;
//	PressKey = NOKEY;
//	while(g_LcdPoll_Flag==LCD_NOTPOLL)
//	{
//		delay(300);
//		switch(PressKey)
//		{
//		case LEFT:
//		case UP:
//			if(shmm_getdevstat()->erc.EC1>0){
//				curpoint1--;
//				if(curpoint1<0)
//					curpoint1 = 0;
//			}
//			break;
//		case RIGHT:
//		case DOWN:
//			if(shmm_getdevstat()->erc.EC1>0){
//				curpoint1++;
//				if(curpoint1>shmm_getdevstat()->erc.EC1-1)
//					curpoint1 = shmm_getdevstat()->erc.EC1-1;
//			}
//			break;
//		case ESC:
//			return;
//		}
//		if(PressKey!=NOKEY||first_flg==0){
//			gui_clrrect(rect_Client);
//			memset(tmpstr,0,50);
//			sprintf((char *)tmpstr,"重要事件信息");
//			gui_setpos(&pos, rect_Client.left+7*FONTSIZE, rect_Client.top+2*FONTSIZE);
//			gui_textshow(tmpstr, pos, LCD_NOREV);
//			soetype =  shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Buff[0];//重要事件类型
//			if (soetype > EVENTCOUNT-1)
//				soetype = EVENTCOUNT-1;//未知事件
//			gui_setpos(&pos, rect_Client.left, rect_Client.top+5*FONTSIZE);
//			gui_textshow((char*)"事件类型:", pos, LCD_NOREV);
//			pos.x = rect_Client.left+4*FONTSIZE;
//			pos.y += FONTSIZE*3;
//			gui_textshow((char *)ERCNAME[soetype].Ercname, pos, LCD_NOREV);
//			pos.x = rect_Client.left;
//			pos.y += FONTSIZE*3;
//			gui_textshow((char*)"发生时间:", pos, LCD_NOREV);
//			memset(tmpstr,0,50);
//			sprintf((char *)tmpstr,"%02x年%02x月%02x日%02x时%02x分",
//					shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err1.Occur_Time.BCD05,
//					shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err1.Occur_Time.BCD04,
//					shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err1.Occur_Time.BCD03,
//					shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err1.Occur_Time.BCD02,
//					shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err1.Occur_Time.BCD01);
//			if(shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Buff[0]==14){
//				memset(tmpstr,0,50);
//				if(shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err14.Ting_Time.BCD05==0xee){//停上电事件 如果停电时间是ee的话 就取上电时间
//					sprintf((char *)tmpstr,"%02x年%02x月%02x日%02x时%02x分",
//						shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err14.Shang_Time.BCD05,
//						shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err14.Shang_Time.BCD04,
//						shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err14.Shang_Time.BCD03,
//						shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err14.Shang_Time.BCD02,
//						shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err14.Shang_Time.BCD01);
//				}
//				else if(shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err14.Shang_Time.BCD05==0xee){//停上电事件 如果上电时间是ee的话 就取掉电时间
//					sprintf((char *)tmpstr,"%02x年%02x月%02x日%02x时%02x分",
//						shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err14.Ting_Time.BCD05,
//						shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err14.Ting_Time.BCD04,
//						shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err14.Ting_Time.BCD03,
//						shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err14.Ting_Time.BCD02,
//						shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err14.Ting_Time.BCD01);
//				}
//				else
//				{
//					sprintf((char *)tmpstr,"%02x年%02x月%02x日%02x时%02x分",
//					shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err14.Shang_Time.BCD05,
//					shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err14.Shang_Time.BCD04,
//					shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err14.Shang_Time.BCD03,
//					shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err14.Shang_Time.BCD02,
//					shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err14.Shang_Time.BCD01);
//				}
//			}
//			else if (shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Buff[0]==41)
//			{
//				memset(tmpstr,0,50);
//				sprintf((char *)tmpstr,"%02x年%02x月%02x日%02x时%02x分",
//					shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err41.New_time.BCDYear,
//					shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err41.New_time.BCDMon & 0x1f,
//					shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err41.New_time.BCDDay,
//					shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err41.New_time.BCDHour,
//					shmm_getdevstat()->erc.ImpEvent[curpoint1+1].Err41.New_time.BCDMin);
//			}
//			pos.x = rect_Client.left+4*FONTSIZE;
//			pos.y += FONTSIZE*3;
//			gui_textshow(tmpstr, pos, LCD_NOREV);
//			memset(tmpstr,0,50);
//			if(shmm_getdevstat()->erc.EC1==0)
//				sprintf((char*)tmpstr, "事件计数: %d",0);
//			else
//				sprintf((char*)tmpstr, "事件计数: %d",curpoint1+1);
//			pos.x = rect_Client.left;
//			pos.y += FONTSIZE*3;
//			gui_textshow(tmpstr, pos, LCD_NOREV);
//		}
//		PressKey = NOKEY;
//	}
//	return;
}

void menu_norsoe(){
//	INT8U first_flg=0;
//	char tmpstr[50];
//	INT8U soetype;
//	int curpoint1;
//	Point pos;
//	curpoint1 = shmm_getdevstat()->erc.EC2-1;
//	if(curpoint1<0)
//		curpoint1 = 0;
//	PressKey = NOKEY;
//	while(g_LcdPoll_Flag==LCD_NOTPOLL)
//	{
//		delay(300);
//		switch(PressKey)
//		{
//		case LEFT:
//		case UP:
//			if(shmm_getdevstat()->erc.EC2>0){
//				curpoint1--;
//				if(curpoint1<0)
//					curpoint1 = 0;
//			}
//			break;
//		case RIGHT:
//		case DOWN:
//			if(shmm_getdevstat()->erc.EC2>0){
//				curpoint1++;
//				if(curpoint1>shmm_getdevstat()->erc.EC2-1)
//					curpoint1 = shmm_getdevstat()->erc.EC2-1;
//			}
//			break;
//		case ESC:
//			return;
//		}
//		if(PressKey!=NOKEY||first_flg==0){
//			gui_clrrect(rect_Client);
//			memset(tmpstr,0,50);
//			sprintf((char *)tmpstr,"一般事件信息");
//			gui_setpos(&pos, rect_Client.left+7*FONTSIZE, rect_Client.top+2*FONTSIZE);
//			gui_textshow(tmpstr, pos, LCD_NOREV);
//			soetype =  shmm_getdevstat()->erc.NorEvent[curpoint1+1].Buff[0];//重要事件类型
//			if (soetype > EVENTCOUNT-1)
//				soetype = EVENTCOUNT-1;//未知事件
//			pos.x = rect_Client.left;
//			pos.y = rect_Client.top + FONTSIZE*5;
//			gui_textshow((char*)"事件类型:", pos, LCD_NOREV);
//			pos.x = rect_Client.left+4*FONTSIZE;
//			pos.y += FONTSIZE*3;
//			gui_textshow((char *)ERCNAME[soetype].Ercname, pos, LCD_NOREV);
//			pos.x = rect_Client.left;
//			pos.y += FONTSIZE*3;
//			gui_textshow((char*)"发生时间:", pos, LCD_NOREV);
//			memset(tmpstr,0,50);
//			sprintf((char *)tmpstr,"%02x年%02x月%02x日%02x时%02x分",
//					shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err1.Occur_Time.BCD05,
//					shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err1.Occur_Time.BCD04,
//					shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err1.Occur_Time.BCD03,
//					shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err1.Occur_Time.BCD02,
//					shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err1.Occur_Time.BCD01);
//			if(shmm_getdevstat()->erc.NorEvent[curpoint1+1].Buff[0]==14){//停上电事件 如果停电时间是ee的话 就取上电时间
//				memset(tmpstr,0,50);
//				if(shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err14.Ting_Time.BCD05==0xee){
//					sprintf((char *)tmpstr,"%02x年%02x月%02x日%02x时%02x分",
//						shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err14.Shang_Time.BCD05,
//						shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err14.Shang_Time.BCD04,
//						shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err14.Shang_Time.BCD03,
//						shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err14.Shang_Time.BCD02,
//						shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err14.Shang_Time.BCD01);
//				}
//				else if(shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err14.Shang_Time.BCD05==0xee){//停上电事件 如果停电时间是ee的话 就取上电时间
//					sprintf((char *)tmpstr,"%02x年%02x月%02x日%02x时%02x分",
//						shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err14.Ting_Time.BCD05,
//						shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err14.Ting_Time.BCD04,
//						shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err14.Ting_Time.BCD03,
//						shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err14.Ting_Time.BCD02,
//						shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err14.Ting_Time.BCD01);
//				}
//				else
//				{
//					sprintf((char *)tmpstr,"%02x年%02x月%02x日%02x时%02x分",
//					shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err14.Shang_Time.BCD05,
//					shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err14.Shang_Time.BCD04,
//					shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err14.Shang_Time.BCD03,
//					shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err14.Shang_Time.BCD02,
//					shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err14.Shang_Time.BCD01);
//				}
//			}
//			else if (shmm_getdevstat()->erc.NorEvent[curpoint1+1].Buff[0]==41)
//			{
//				memset(tmpstr,0,50);
//				sprintf((char *)tmpstr,"%02x年%02x月%02x日%02x时%02x分",
//					shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err41.New_time.BCDYear,
//					shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err41.New_time.BCDMon & 0x1f,
//					shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err41.New_time.BCDDay,
//					shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err41.New_time.BCDHour,
//					shmm_getdevstat()->erc.NorEvent[curpoint1+1].Err41.New_time.BCDMin);
//			}
//			pos.x = rect_Client.left+4*FONTSIZE;
//			pos.y += FONTSIZE*3;
//			gui_textshow(tmpstr, pos, LCD_NOREV);
//			memset(tmpstr,0,50);
//			if(shmm_getdevstat()->erc.EC2==0)
//				sprintf((char*)tmpstr, "事件计数: %d",0);
//			else
//				sprintf((char*)tmpstr, "事件计数: %d",curpoint1+1);
//			pos.x = rect_Client.left;
//			pos.y += FONTSIZE*3;
//			gui_textshow(tmpstr, pos, LCD_NOREV);
//		}
//		PressKey = NOKEY;
//	}
//	return;
}

void menu_zb_begin(){
	int ret;
	ret = msgbox_label((char*)"重启载波抄表?", CTRL_BUTTON_OK);
	if(ret==ACK){
		DEBUG_TIME_LINE( "\n 液晶重启载波抄表");
		 p_JProgramInfo->PLC_Ctrl = 0xAA;
	}
}
void menu_zb_stop(){
	int ret;
	ret = msgbox_label((char*)"暂停载波抄表?", CTRL_BUTTON_OK);
	if(ret==ACK){
		DEBUG_TIME_LINE( "\n 液晶暂停载波抄表");
		 p_JProgramInfo->PLC_Ctrl = 0x55;
	}
}
void menu_zb_resume(){
	int ret;
	ret = msgbox_label((char*)"恢复载波抄表?", CTRL_BUTTON_OK);
	if(ret==ACK){
		DEBUG_TIME_LINE( "\n 液晶恢复载波抄表");
		 p_JProgramInfo->PLC_Ctrl = 0x44;
	}
}
int msgbox_pro(int type)
{
	int baud = 0;
    int tmpindex=0;
    int ret_baud=0;
    Combox cb_baud;
	Rect msgbox_rect;
	MsgBoxRet msgbox_ret;
    char cb_text[TEXTLEN_X][TEXTLEN_Y];
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	Text_t ttext[MSGBOX_TEXTSIZE];
	struct list *head=NULL;
	Point msgbox_pos;
	Button btn_ok, btn_esc;
	memset(&btn_ok, 0, sizeof(Button));
	memset(&btn_esc, 0, sizeof(Button));
	memset(&msgbox_ret, 0, sizeof(MsgBoxRet));
	gui_setpos(&msgbox_pos, (LCM_X-20*FONTSIZE)/2, (LCM_Y-10*FONTSIZE)/2);
	head = (struct list*)malloc(sizeof(struct list));
	if(head==NULL) 	return -1;
	list_init(head);
	Point editctrl_pos;
	memset(ttext, 0, sizeof(Text_t));
	editctrl_pos.x = msgbox_pos.x+FONTSIZE;
	editctrl_pos.y = msgbox_pos.y+2*FONTSIZE;
	gui_setpos(&ttext[0].pos, editctrl_pos.x,editctrl_pos.y);
	editctrl_pos.x = msgbox_pos.x+FONTSIZE*13;
	baud = parse_ttyS_file(type);
	if (baud !=0)
	{
		if(type == 1){
			memcpy(cb_text[0], "1200", strlen("1200"));
//			memcpy(cb_text[1], "2400", strlen("2400"));
			if(baud == 1200) tmpindex = 1;
			else tmpindex = 0;
		}
		else{
			memcpy(cb_text[0], "9600", strlen("9600"));
			memcpy(cb_text[1], "4800", strlen("4800"));
			memcpy(cb_text[2], "2400", strlen("2400"));
			memcpy(cb_text[3], "1200", strlen("1200"));
			if(baud == 1200) tmpindex = 3;
			else if(baud == 2400) tmpindex = 2;
			else if(baud == 4800) tmpindex = 1;
			else tmpindex = 0;
		}
	}
	combox_init(&cb_baud, tmpindex, cb_text, editctrl_pos,1,head);//通信速率
	Point button_ok_pos = {msgbox_pos.x+4*FONTSIZE, msgbox_pos.y+5*FONTSIZE};
	button_init(&btn_ok,(char *)"确定", button_ok_pos, CTRL_BUTTON_OK, head);
	Point button_esc_pos = {msgbox_pos.x+11*FONTSIZE, msgbox_pos.y+FONTSIZE*5};
	button_init(&btn_esc,(char *)"取消", button_esc_pos, CTRL_BUTTON_CANCEL, head);
	btn_esc.form.focus = FOCUS;//默认按键
	gui_setrect(&msgbox_rect, msgbox_pos.x, msgbox_pos.y, msgbox_pos.x+19*FONTSIZE, msgbox_pos.y+8*FONTSIZE);
	memcpy(ttext[0].text, (char*)"当前波特率:", strlen("当前波特率:"));
	cb_baud.form.key[0].c = 0x33;
	msgbox(msgbox_rect, head,ttext, &msgbox_ret);
	if(cb_baud.form.key[0].c == 0x33)   //给combox控件做初始话，如果 初始化值未变，则不需要修改，直接返回。
	{
		if(head!=NULL) free(head);
		return -1;
	}
	if(msgbox_ret.btn_ret==ACK){
		if(type == 1){
			if(cb_baud.form.key[0].c == 0) ret_baud = 2400;
			else ret_baud = 1200;
		}
		else{
			if(cb_baud.form.key[0].c == 0) ret_baud = 9600;
			else if(cb_baud.form.key[0].c == 1) ret_baud = 4800;
			else if(cb_baud.form.key[0].c == 2) ret_baud = 2400;
			else ret_baud = 1200;
		}
		msgbox_label((char*)"设置成功", CTRL_BUTTON_OK);
	}
	else ret_baud = -1;
	if(head!=NULL)
		free(head);

	return ret_baud;
}
void menu_vifr_set(){
    int box_ret;
    INT32S pid = 0;
    box_ret = msgbox_pro(1); //1为红外
    if (box_ret == -1) return;
    if (box_ret >0) {
		para_write(1,box_ret);
    }
//    pid= prog_getsyspid((INT8S*)"vifr");
    if(pid > 0){
        kill(pid,SIGKILL);
        DEBUG_TIME_LINE("设置成功");
    }
    return;
}


void menu_rs232_set(){
    int box_ret;
    INT32S pid = 0;
    box_ret = msgbox_pro(2); //1为红外
    if (box_ret == -1) return;
    if (box_ret >0) {
		para_write(2,box_ret);
    }
//    pid= prog_getsyspid((INT8S*)"vcom");
    if(pid > 0){
    	kill(pid,SIGKILL);
    	DEBUG_TIME_LINE("设置成功");
    }
    return;
}
void menu_zb_info(){
//    char	str[50];
//    Point pos;
//    gui_clrrect(rect_Client);
//    DEBUG_TIME_LINE( "\n ------载波模块信息-----------");
//    gui_setpos(&pos, rect_Client.left+8*FONTSIZE, rect_Client.top+2*FONTSIZE);
//    memset(str,0, 50);
//    gui_textshow((char*)"载波模块信息", pos, LCD_NOREV);
//    pos.x = rect_Client.left + FONTSIZE*5;
////    char vendorcode[3];
////    pos.y += FONTSIZE*3;
////    memset(str,0, 50);
////    memset(vendorcode, 0, 3);
////    memcpy(vendorcode, shmm_getdevstat()->ZB_Module.VendorCode, 2);
////    //厂商代码（VendorCode[1~0]：TC=鼎信；ES=东软；LH=力合微；37=中瑞昊天）
////    if(vendorcode[1]=='E'&&vendorcode[1]=='S')
////    	sprintf(str,"模块厂家: 东软");
////	else if (vendorcode[1]=='T'&&vendorcode[1]=='C')
////    	sprintf(str,"模块厂家: 鼎信");
////	else if (vendorcode[1]=='L'&&vendorcode[1]=='H')
////		sprintf(str,"模块厂家: 力合微");
////	else if (vendorcode[1]=='3'&&vendorcode[1]=='7')
////		sprintf(str,"模块厂家: 中瑞昊天");
////	else if (vendorcode[1]=='T'&&vendorcode[1]=='C')
////		sprintf(str,"模块厂家: 福星晓程");
////	else
////		sprintf(str,"模块厂家: 未识别");
////    gui_textshow(str, pos, LCD_NOREV);
//    pos.y += FONTSIZE*3;
//    memset(str,0, 50);
//    sprintf(str,"厂商代码:%c%c",
//    		shmm_getdevstat()->ZB_Module.VendorCode[1],
//    		shmm_getdevstat()->ZB_Module.VendorCode[0]);
//    gui_textshow(str, pos, LCD_NOREV);
//    pos.y += FONTSIZE*3;
//    memset(str,0, 50);
//    sprintf(str,"芯片代码:%c%c",
//    		shmm_getdevstat()->ZB_Module.ChipCode[1],
//    		shmm_getdevstat()->ZB_Module.ChipCode[0]);
//    gui_textshow(str, pos, LCD_NOREV);
//    pos.y += FONTSIZE*3;
//    memset(str,0, 50);
//    sprintf(str,"模块版本:%d%d",
//    		shmm_getdevstat()->ZB_Module.Version[1],
//    		shmm_getdevstat()->ZB_Module.Version[0]);
//    gui_textshow(str, pos, LCD_NOREV);
//    pos.y += FONTSIZE*3;
//    memset(str,0, 50);
//    sprintf(str,"模块日期:%02d-%02d-%02d",shmm_getdevstat()->ZB_Module.VersionYear,
//										shmm_getdevstat()->ZB_Module.VersionMonth,
//										shmm_getdevstat()->ZB_Module.VersionDay);
//    gui_textshow(str, pos, LCD_NOREV);
//	PressKey = NOKEY;
//	while(g_LcdPoll_Flag==LCD_NOTPOLL){
//		delay(300);
//		if(PressKey==ESC)
//			return;
//		PressKey = NOKEY;
//	}
}

void menu_gprs_info(){
//    char str[50], substr[10];
//    Point pos;
//    gui_clrrect(rect_Client);
////    DEBUG_TIME_LINE( "\n ------GPRS模块信息-----------");
//    gui_setpos(&pos, rect_Client.left+8*FONTSIZE, rect_Client.top+3);
//    memset(str,0, 50);
//    gui_textshow((char*)"GPRS模块信息", pos, LCD_NOREV);
//    pos.x = rect_Client.left + FONTSIZE*2;
//    memset(substr, 0, 10);
//    memcpy(substr, shmm_getdevstat()->f9_cfg.FactNo, 4);
////    if((substr[0]=='Q'||substr[0]=='q')&&
////		(substr[1]=='U'||substr[1]=='u')&&
////		(substr[2]=='E'||substr[2]=='e')&&
////		(substr[3]=='C'||substr[3]=='c'))
////    	sprintf(str,"模块厂家: 移远");
////	else if ((substr[0]=='N'||substr[0]=='n')&&
////			(substr[1]=='E'||substr[1]=='e')&&
////			(substr[2]=='0'||substr[2]=='0')&&
////			(substr[3]=='6'||substr[3]=='6'))
////		sprintf(str,"模块厂家: 友方");
////	else
////		sprintf(str,"模块厂家: 未识别");
////    pos.y += FONTSIZE*2+3;
////    gui_textshow(str, pos, LCD_NOREV);
//    memset(str,0, 50);
//    pos.y += FONTSIZE*2+3;
//    sprintf(str,"厂商代号:%s", substr);
//    gui_textshow(str, pos, LCD_NOREV);
//    pos.y += FONTSIZE*2+3;
//    memset(str,0, 50);
//    memset(substr, 0, 10);
//    memcpy(substr, shmm_getdevstat()->f9_cfg.ModuleTypeNo, 8);
//    sprintf(str,"模块型号:%s", substr);
//    gui_textshow(str, pos, LCD_NOREV);
//    pos.y += FONTSIZE*2+3;
//    memset(str,0, 50);
//    memset(substr, 0, 10);
//    memcpy(substr, shmm_getdevstat()->f9_cfg.SoftVer, 4);
//    sprintf(str,"模块版本:%s", substr);
//    gui_textshow(str, pos, LCD_NOREV);
//    pos.y += FONTSIZE*2+3;
//    memset(str,0, 50);
//    sprintf(str, "ICCID:");
//    gui_textshow(str, pos, LCD_NOREV);
//    memset(str,0, 50);
//    pos.y += FONTSIZE*2+3;
//    memcpy(str,shmm_getdevstat()->f9_cfg.ICCIDstr,20);
//    gui_textshow(str, pos, LCD_NOREV);
//    pos.y += FONTSIZE*2+3;
//    memset(str,0, 50);
//    memset(substr, 0, 10);
//    substr[0] = (INT8U)shmm_getdevstat()->Gprs_csq/10 + 48;
//    substr[1] = (INT8U)shmm_getdevstat()->Gprs_csq%10 + 48;
//    sprintf(str,"信号强度:%s",substr);
//    gui_textshow(str, pos, LCD_NOREV);
//	PressKey = NOKEY;
//	while(g_LcdPoll_Flag==LCD_NOTPOLL){
//		delay(300);
//		if(PressKey==ESC)
//			return;
//		PressKey = NOKEY;
//	}
}

void menu_ac_info(){
	INT32U acChipType;
	INT32U acWireType;
    char str[50], substr[30];
    Point pos;
    gui_clrrect(rect_Client);
    gui_setpos(&pos, rect_Client.left+8*FONTSIZE, rect_Client.top+3);
    memset(str,0, 50);
    gui_textshow((char*)"交采芯片信息", pos, LCD_NOREV);
    pos.x = rect_Client.left + FONTSIZE*5;
    memset(substr, 0, 30);
    //==0:	RN8029芯片，III型集中器
    //==1： ATT7022E芯片
    //==0x7022E0:	ATT7022E-D芯片
    acChipType = p_JProgramInfo->dev_info.ac_chip_type;
    if(0 == acChipType)
    	sprintf(substr,"RN8029");
    else if(1 == acChipType)
    	sprintf(substr,"ATT7022E");
    else if(0x7022E0 == acChipType)
    	sprintf(substr,"ATT7022E-D");
    else
    	sprintf(substr,"未知芯片型号");
    pos.y += FONTSIZE*4+3;
    sprintf(str,"芯片型号:%s", substr);
    gui_textshow(str, pos, LCD_NOREV);
    pos.y += FONTSIZE*2+3;
    memset(str,0, 50);
    bzero(substr,30);
    //接线方式，0x1200：三相三，0x0600：三相四
    acWireType = p_JProgramInfo->dev_info.WireType;
    if(0x1200 == acWireType)
       	sprintf(substr,"三相三线");
    else if(0x0600 == acWireType)
    	sprintf(substr,"三相四线");
    else
    	sprintf(substr,"未知接线方式");
    sprintf(str,"接线方式:%s", substr);
    gui_textshow(str, pos, LCD_NOREV);
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		delay(300);
		if(PressKey==ESC)
			return;
		PressKey = NOKEY;
	}
}
//698协议切换到3761协议
void menu_ProtocolChange()
{
	if(getZone("HuNan")!=0) return ;//非湖南地区不使用该功能
	if(msgbox_label((char*)"切换到1376.1?", CTRL_BUTTON_OK) != ACK) return ;
	chg_rc_local_3761();
}
void getPluseCount(unsigned int *pulse) {
	int fd = open("/dev/pulse", O_RDWR);
	read(fd, pulse, sizeof(unsigned int) * 2);
	close(fd);
	fprintf(stderr, "刷新脉冲 %d-%d\n", pulse[0], pulse[1]);
}
void menu_yxstatus_fk(){
	Rect rect;
	Point pos;
	INT8U str[100];
    CLASS_f203 oif203 = {};
    int i=0;
	unsigned int pluse[2] = { 0, 0 };

	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		if(PressKey==ESC){
			break;
		}
		readCoverClass(0xf203, 0, &oif203, sizeof(CLASS_f203), para_vari_save);
		gui_clrrect(rect_Client);
		gui_setpos(&pos, rect_Client.left+6*FONTSIZE, rect_Client.top+FONTSIZE);
		memset(str, 0, 100);
		pos.x = rect_Client.left + FONTSIZE;
		gui_textshow((char*)"  状态  变位  接入  属性", pos, LCD_NOREV);

		memcpy(&rect, &rect_Client, sizeof(Rect));
		memset(str,0,sizeof(str));
		sprintf((char*)str,"%s","  状态  变位  接入  属性");
		rect = gui_getstrrect(str, pos);//获得字符串区域
		gui_reverserect(gui_changerect(rect, 2));//反显按钮

		for(i=0;i<4;i++)
		{
			pos.y += FONTSIZE*3-2;
			memset(str, 0, 100);
			sprintf((char*)str, "%d: %s   %s    %s    %s",i+1,
					oif203.statearri.stateunit[i].ST?"合":"分",
					oif203.statearri.stateunit[i].CD?"是":"否",
					((oif203.state4.StateAcessFlag>>i)&0x01)?"是":"否",
					((oif203.state4.StatePropFlag>>i)&0x01)?"动合":"动断");
			gui_textshow((char*)str, pos, LCD_NOREV);
			fprintf(stderr,"状态 = %d 变位= %d  接入 = %d 属性 = %d \n",oif203.statearri.stateunit[i].ST,oif203.statearri.stateunit[i].CD,oif203.state4.StateAcessFlag,
					oif203.state4.StatePropFlag>>i);
		}

		memset(str, 0, 100);
		pos.y += FONTSIZE*3 ;
		sprintf((char*)str, "门接点 :%s",oif203.statearri.stateunit[4].ST?"合":"分");
		gui_textshow((char*)str, pos, LCD_NOREV);

		getPluseCount(pluse);
		memset(str, 0, 100);
		pos.y += FONTSIZE*3-2;
		sprintf((char*)str, "脉冲_1:%d   脉冲_2:%d",p_JProgramInfo->class12[0].pluse_count  ,p_JProgramInfo->class12[1].pluse_count);
		gui_textshow((char*)str, pos, LCD_NOREV);

		PressKey = NOKEY;
		delay(1000);
	}
	return;
}
void menu_yxstatus(){
	Point pos;
	INT8U str[100];
    CLASS_f203 oif203 = {};
    int i=0;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		if(PressKey==ESC){
			break;
		}
		readCoverClass(0xf203, 0, &oif203, sizeof(CLASS_f203), para_vari_save);
		gui_clrrect(rect_Client);
		gui_setpos(&pos, rect_Client.left+6*FONTSIZE, rect_Client.top+FONTSIZE);
		gui_textshow((char*)"当前开关量状态", pos, LCD_NOREV);
		memset(str, 0, 100);
		pos.x = rect_Client.left + FONTSIZE;
		pos.y += FONTSIZE*3;
		gui_textshow((char*)"  状态  变位  接入  属性", pos, LCD_NOREV);
		for(i=0;i<4;i++)
		{
			pos.y += FONTSIZE*3-2;
			memset(str, 0, 100);
			sprintf((char*)str, "%d: %s   %s    %s    %s",i+1,
					oif203.statearri.stateunit[i].ST?"合":"分",
					oif203.statearri.stateunit[i].CD?"是":"否",
					((oif203.state4.StateAcessFlag>>i)&0x01)?"是":"否",
					((oif203.state4.StatePropFlag>>i)&0x01)?"动合":"动断");
			gui_textshow((char*)str, pos, LCD_NOREV);
			fprintf(stderr,"状态 = %d 变位= %d  接入 = %d 属性 = %d \n",oif203.statearri.stateunit[i].ST,oif203.statearri.stateunit[i].CD,oif203.state4.StateAcessFlag,
					oif203.state4.StatePropFlag>>i);
		}
		memset(str, 0, 100);
		pos.y += FONTSIZE*3 ;
		sprintf((char*)str, "门接点 :%s",oif203.statearri.stateunit[4].ST?"合":"分");
		gui_textshow((char*)str, pos, LCD_NOREV);

		PressKey = NOKEY;
		delay(1000);
	}
	return;
}

void menu_manualsearch(){
//	MsgBoxRet msgbox_ret;
//	memset(&msgbox_ret, 0, sizeof(MsgBoxRet));
//	int index=0;
//	msgbox_soubiao(&index, &msgbox_ret);
//	if(msgbox_ret.btn_ret==ACK)
//	{
//		INT8U cbs=5;
//		mqd_t mqd_vplc, mqd_4851, mqd_4852;
//
//		if((mqd_vplc =  createMsg((INT8S *)PLC_REV_GUI_MQ, O_WRONLY)) <0)
//		{//创建液晶发送消息队列
//			dbg_prt( "\nGUI createMsg %s  error!!2  mqd=%d",PLC_REV_GUI_MQ, mqd_vplc);
//			return;
//		}
//		else
//		{
//			if(sendMsg(mqd_vplc, CTRLCMD, (INT8S*)&cbs, 1) < 0)
//			{//给载波发消息
//				dbg_prt( "\nGUI sendmsg %s  error!!2  mqd=%d",PLC_REV_GUI_MQ, mqd_vplc);
//				colseMsg(mqd_vplc);
//				return;
//			}
//		}
//
//		if((mqd_4851 =  createMsg((INT8S *)S485_1_REV_GUI_MQ, O_WRONLY)) <0)
//		{//创建液晶发送消息队列
//			dbg_prt( "\nGUI createMsg %s  error!!2  mqd=%d",S485_1_REV_GUI_MQ, mqd_4851);
//			return;
//		}
//		else
//		{
//			if(sendMsg(mqd_4851, CTRLCMD, (INT8S*)&cbs, 1) < 0)
//			{//给4851发消息
//				dbg_prt( "\nGUI sendmsg %s  error!!2  mqd=%d",S485_1_REV_GUI_MQ, mqd_vplc);
//				colseMsg(mqd_4851);
//				return;
//			}
//		}
//
//		if((mqd_4852 =  createMsg((INT8S *)S485_2_REV_GUI_MQ, O_WRONLY)) <0)
//		{//创建液晶发送消息队列
//			dbg_prt( "\nGUI createMsg %s  error!!2  mqd=%d",S485_2_REV_GUI_MQ, mqd_4852);
//			return;
//		}
//		else
//		{
//			if(sendMsg(mqd_4852, CTRLCMD, (INT8S*)&cbs, 1) < 0)
//			{//给4852发消息
//				dbg_prt( "\nGUI sendmsg %s  error!!2  mqd=%d",S485_2_REV_GUI_MQ, mqd_vplc);
//				colseMsg(mqd_4852);
//				return;
//			}
//		}
//	}
//	return;
}

