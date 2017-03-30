//lcd_prt.c 菜单功能实现
//#include "../include/stdafx.h"
#include <mqueue.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "comm.h"
#include "lcd_menu.h"
#include "lcd_ctrl.h"
#include "lcdprt_jzq.h"
#include "usertype.h"
#include "../libMq/libmmq.h"
#include "mutils.h"
#include "gui.h"
#include "show_ctrl.h"
//extern ProgramInfo* p_JProgramInfo;
//extern void set_time_show_flag(INT8U value);
//#include "libhd.h"
#pragma message("\n\n************************************\n CCTT_I__Compiling............\n************************************\n")

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
			{{level3,"4.以太网参数配置", 	menu_eth0para, 		MENU_NOPASSWD},		NULL},//11
//			{{level3,"5.虚拟专网参数", 	menu_Virpara, 		MENU_NOPASSWD},		NULL},//-1
		{{level2,"2.电表参数设置", 	NULL, 	MENU_ISPASSWD_EDITMODE},	NULL},
			{{level3,"1.修改测量点", 		menu_jzqsetmeter,	MENU_ISPASSWD},	NULL},//11
			{{level3,"2.添加测量点", 		menu_jzqaddmeter,	MENU_ISPASSWD},	NULL},//11
			{{level3,"3.删除测量点", 		menu_jzqdelmeter,	MENU_ISPASSWD},	NULL},//11
		{{level2,"3.集中器时间设置",	menu_jzqtime, 		MENU_NOPASSWD},		NULL},//11
		{{level2,"4.界面密码设置",		menu_setpasswd, 	MENU_NOPASSWD},		NULL},//11
		{{level2,"5.集中器地址设置", 		jzq_id_edit, 				MENU_ISPASSWD_EDITMODE},		NULL},//111
//			////三级菜单 集中器编号子菜单
//			{{level3,"1.十进制", 		menu_jzqzddr10, 	MENU_NOPASSWD},		NULL},
//			{{level3,"2.十六进制", 	menu_jzqzddr16, 	MENU_NOPASSWD},		NULL},
	{{level1,"终端管理与维护", 	NULL, 				MENU_NOPASSWD},		NULL},
		//二级菜单 终端管理与维护子菜单
		{{level2,"1.终端版本", 	menu_jzqstatus, 	MENU_NOPASSWD},		NULL},//11
//		{{level2,"2.终端数据", 	NULL, 				MENU_NOPASSWD},		NULL},
//			////三级菜单 集中器数据子菜单
//			{{level3,"1.遥信状态", 	menu_yxstatus, 		MENU_NOPASSWD},		NULL},
//			{{level3,"2.事件信息", 	NULL,		 		MENU_NOPASSWD},		NULL},
//				///////四级菜单 事件信息子菜单
//				{{level4,"1.重要事件", 	menu_impsoe, 		MENU_NOPASSWD},		NULL},
//				{{level4,"2.一般事件",	menu_norsoe,		MENU_NOPASSWD},		NULL},
		{{level2,"2.终端管理", 	NULL, 				MENU_NOPASSWD},		NULL},
			////三级菜单 终端管理子菜单
			{{level3,"1.终端重启", 	menu_jzqreboot, 	MENU_ISPASSWD},		NULL},//111
			{{level3,"2.数据初始化", 	menu_initjzqdata, 	MENU_ISPASSWD},		NULL},
			{{level3,"3.事件初始化", 	menu_initjzqevent, 	MENU_ISPASSWD},		NULL},
			{{level3,"4.需量初始化", 	menu_initjzqdemand, 	MENU_ISPASSWD},		NULL},
		{{level2,"3.现场调试", 	NULL, 				MENU_NOPASSWD},		NULL},
		////三级菜单 现场调试子菜单
			{{level3,"1.本地IP设置",	menu_termip, 		MENU_NOPASSWD},		NULL},//111
			{{level3,"2.GPRSIP查看",	menu_gprsip, 		MENU_NOPASSWD},		NULL},//111
#if 1//(defined(NINGXIA)||defined(SHANDONG))
//			{{level3,"3.抄表结果查看",menu_readmeter_info,		MENU_NOPASSWD},		NULL},
			{{level3,"3.液晶对比度", 	menu_lcdcontrast, 	MENU_NOPASSWD},		NULL},
//			{{level3,"5.485-2设置",menu_485func_change,		MENU_NOPASSWD},		NULL},
//			{{level3,"6.时钟电池", 	menu_rtcpower, 		MENU_NOPASSWD},		NULL},//0
//			{{level3,"7.载波模块信息",	menu_zb_info,		MENU_NOPASSWD},		NULL},//0
//			{{level3,"8.GPRS模块信息",menu_gprs_info,		MENU_NOPASSWD},		NULL},//0
			{{level3,"4.交采芯片信息",menu_ac_info,		MENU_NOPASSWD},		NULL},
			{{level3,"5.配置设置",menu_TorF_info,		MENU_NOPASSWD},		NULL},
#else
			{{level3,"3.抄表结果查看",menu_readmeter_info,		MENU_NOPASSWD},		NULL},
			{{level3,"4.液晶对比度", 	menu_lcdcontrast, 	MENU_NOPASSWD},		NULL},
			{{level3,"5.时钟电池", 	menu_rtcpower, 		MENU_NOPASSWD},		NULL},
			{{level3,"6.载波模块信息",	menu_zb_info,		MENU_NOPASSWD},		NULL},
			{{level3,"7.GPRS模块信息",menu_gprs_info,		MENU_NOPASSWD},		NULL},
			{{level3,"8.交采芯片信息",menu_ac_info,		MENU_NOPASSWD},		NULL},
			{{level3,"9.配置设置",menu_TorF_info,		MENU_NOPASSWD},		NULL},
#endif
//		{{level2,"5.页面设置", 	menu_pagesetup, 	MENU_NOPASSWD},		NULL},
//		{{level2,"6.手动抄表", 	NULL, 				MENU_NOPASSWD},		NULL},
//			//////三级菜单 手动抄表子菜单
//			{{level3,"1.根据表号抄表", menu_readmeterbycldno, 	MENU_NOPASSWD},	NULL},
//			{{level3,"2.根据表地址抄表",menu_readmeterbycldaddr,MENU_NOPASSWD},	NULL},
//		{{level2,"7.载波管理",	NULL, 				MENU_NOPASSWD},		NULL},
//		/////三级菜单 载波抄表子菜单
//			{{level3,"1.重新抄表", 	menu_zb_begin, 		MENU_NOPASSWD},		NULL},
//			{{level3,"2.暂停抄表",	menu_zb_stop,		MENU_NOPASSWD},		NULL},
//			{{level3,"3.恢复抄表",	menu_zb_resume,		MENU_NOPASSWD},		NULL},
//		{{level2,"8.波特率设置",	NULL, 				MENU_NOPASSWD},		NULL},
//			{{level3,"1.红外口波特率", 	menu_vifr_set, 		MENU_NOPASSWD},		NULL},
//			{{level3,"2.RS232波特率", 	menu_rs232_set, 		MENU_NOPASSWD},		NULL},
#ifdef ZHEJIANG
		{{level2,"9.手动搜表", 	menu_manualsearch, MENU_NOPASSWD},		NULL},
#endif
};//测量点数据显示
//#else//江苏开始
//Menu menu[]={//必须是一级菜单，然后二级菜单。。。。
//	//一级菜单
//	//level,    name,   		fun, 				ispasswd			pthis,
//{{level0,"     ",		NULL, 				MENU_NOPASSWD},		NULL},
//	{{level1,"测量点数据显示", 	NULL, 				MENU_NOPASSWD},		NULL},
//		//二级菜单 测量点数据显示子菜单
//		{{level2,"1.实时数据",	menu_showclddata, 	MENU_NOPASSWD},		NULL},
//		{{level2,"2.日 数 据", 	menu_showdaydata, 	MENU_NOPASSWD},		NULL},
//		{{level2,"3.月 数 据", 	menu_showmonthdata, MENU_NOPASSWD},		NULL},
//	{{level1,"参数设置与查看", 	NULL, 				MENU_NOPASSWD},		NULL},
//		//二级菜单 参数设置与查看子菜单
//		{{level2,"1.通信通道设置", 	NULL, 				MENU_ISPASSWD_EDITMODE},	NULL},
//			////三级菜单 通信通道设置子菜单
//			{{level3,"1.通信方式", 	menu_settx, 		MENU_NOPASSWD},		NULL},
//			{{level3,"2.短信中心", 	menu_jzqtelephone, 	MENU_NOPASSWD},		NULL},
//			{{level3,"3.主站IP地址", 	menu_masterapn, 	MENU_NOPASSWD},		NULL},
//			{{level3,"4.以太网参数", 	menu_eth0para_js, 		MENU_NOPASSWD},		NULL},
//		{{level2,"2.电表参数设置", 	menu_jzqsetmeter, 	MENU_ISPASSWD_EDITMODE},	NULL},
//		{{level2,"3.集中器时间设置",	menu_jzqtime_js, 		MENU_NOPASSWD},		NULL},
//		{{level2,"4.界面密码设置",		menu_setpasswd, 	MENU_NOPASSWD},		NULL},
//		{{level2,"5.集中器编号", 		NULL, 	MENU_NOPASSWD},		NULL},
//			{{level3,"1.十进制", 		menu_jzqzddr10, 	MENU_NOPASSWD},		NULL},
//			{{level3,"2.十六进制", 	menu_jzqzddr16, 	MENU_NOPASSWD},		NULL},
//	{{level1,"终端管理与维护", 	NULL, 				MENU_NOPASSWD},		NULL},
//		//二级菜单 终端管理与维护子菜单
//		{{level2,"1.终端信息", 	menu_jzqstatus, 	MENU_NOPASSWD},		NULL},
//		{{level2,"2.终端数据", 	NULL, 				MENU_NOPASSWD},		NULL},
//			////三级菜单 集中器数据子菜单
//			{{level3,"1.遥信状态", 	menu_yxstatus_js, 		MENU_NOPASSWD},		NULL},
//			{{level3,"2.事件信息", 	NULL,		 		MENU_NOPASSWD},		NULL},
//				///////四级菜单 事件信息子菜单
//				{{level4,"1.重要事件", 	menu_impsoe, 		MENU_NOPASSWD},		NULL},
//				{{level4,"2.一般事件",	menu_norsoe,		MENU_NOPASSWD},		NULL},
//		{{level2,"3.终端管理", 	NULL, 				MENU_NOPASSWD},		NULL},
//				////三级菜单 终端管理子菜单
//			{{level3,"1.重新抄表", 	menu_zb_begin, 		MENU_NOPASSWD},		NULL},
//			{{level3,"2.暂停抄表",	menu_zb_stop,		MENU_NOPASSWD},		NULL},
//			{{level3,"3.恢复抄表",	menu_zb_resume,		MENU_NOPASSWD},		NULL},
//			{{level3,"4.液晶对比度", 	menu_lcdcontrast, 	MENU_NOPASSWD},		NULL},
//			{{level3,"5.激活连接",	Term_StartConn,		MENU_NOPASSWD},		NULL},
//			{{level3,"6.断开连接",	Term_StopConn,		MENU_NOPASSWD},		NULL},
//			{{level3,"7.节点维护",	menu_manualsearch,		MENU_NOPASSWD},		NULL},
//			{{level3,"8.USB功能",	NULL,		MENU_NOPASSWD},		NULL},
//				{{level4,"1.程序升级",	USB_UpdateSoft,		MENU_NOPASSWD},		NULL},
//				{{level4,"2.数据拷贝",	USB_DataCopy,		MENU_NOPASSWD},		NULL},
//		{{level2,"4.现场调试", 	NULL, 				MENU_NOPASSWD},		NULL},
//		////三级菜单 现场调试子菜单
//			{{level3,"1.终端重启", 	menu_jzqreboot, 	MENU_ISPASSWD},		NULL},
//			{{level3,"2.数据初始化", 	menu_initjzqdata, 	MENU_ISPASSWD},		NULL},
//			{{level3,"3.参数初始化", 	menu_initjzqpara, 	MENU_ISPASSWD},		NULL},
//			{{level3,"4.本地IP设置",	menu_termip, 		MENU_NOPASSWD},		NULL},
//			{{level3,"5.GPRSIP查看",	menu_gprsip, 		MENU_NOPASSWD},		NULL},
//			{{level3,"6.抄表结果查看",menu_readmeter_info,		MENU_NOPASSWD},		NULL},
//			{{level3,"7.时钟电池", 	menu_rtcpower, 		MENU_NOPASSWD},		NULL},
//			{{level3,"8.配置设置",menu_TorF_info,		MENU_NOPASSWD},		NULL},
//		{{level2,"5.页面设置", 	menu_pagesetup, 	MENU_NOPASSWD},		NULL},
//		{{level2,"6.手动抄表", 	NULL, 				MENU_NOPASSWD},		NULL},
//		//////三级菜单 手动抄表子菜单
//			{{level3,"1.根据表号抄表", menu_readmeterbycldno, 	MENU_NOPASSWD},	NULL},
//			{{level3,"2.根据表地址抄表",menu_readmeterbycldaddr,MENU_NOPASSWD},	NULL},
//		{{level2,"7.波特率设置",	NULL, 				MENU_NOPASSWD},		NULL},
//			{{level3,"1.红外口波特率", 	menu_vifr_set, 		MENU_NOPASSWD},		NULL},
//			{{level3,"2.RS232波特率", 	menu_rs232_set, 		MENU_NOPASSWD},		NULL},
//};
//#endif  //江苏结束
//#endif
#define GUI_MSG_MAXLEN 4096
extern LunXian_t LunXian[LunPageNum];
extern Proxy_Msg* p_Proxy_Msg_Data;
extern ProgramInfo* p_JProgramInfo;
TS Tcurr_tm_his;
#ifdef JIANGSU
extern INT16U show_offtime;
#endif
int g_PressKey_old;//用于液晶点抄 半途退出
//#ifdef CCTT_I
int getMenuSize(){
	return sizeof(menu)/sizeof(Menu);
}
//#endif

void show_realdata(int pindex, LcdDataItem *item, int itemcount){
	int pageno=0;
	CLASS_6001 meter;
	bzero(&meter,sizeof(CLASS_6001));
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
#ifdef SHANGHAI
	else if(ParaAll->f10.para_mp[pindex-1].Protocol==21)  //上海表
	{
		gui_clrrect(rect_Client);
		ShowCLDDataPage_SH(item, itemcount, 2,ParaAll->f10.para_mp[pindex-1].RateNum);
		PressKey = NOKEY;
		while(g_LcdPoll_Flag==LCD_NOTPOLL){
			if(PressKey==ESC)
				return;
			PressKey = NOKEY;
			delay(300);
		}
	}
#endif
	else
	{
		gui_clrrect(rect_Client);
		ShowCLDDataPage(item, itemcount, 2);
		set_time_show_flag(1);
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

void setpara(void *para_f, int pn, int fn, int para_f_len){//发送消息给vmain
//#ifndef FB_SIM
//	gdm_3761 gdm_3761_tmp;
//	mqd_t 	mqfd_ask;
//	gdm_3761_tmp.afn = 0x04;
//	gdm_3761_tmp.dunum = 1;
//	gdm_3761_tmp.du[0].pn = pn;
//	gdm_3761_tmp.du[0].fn = fn;
//	gdm_3761_tmp.du[0].datalen = para_f_len;
//	memcpy(gdm_3761_tmp.du[0].data, para_f, para_f_len);
//	int ret;
//	struct mq_attr attr_main;
//	mqfd_ask = mmq_open((INT8S *)COM_VMAIN_MQ,&attr_main,O_WRONLY);
//	mmq_head mq_h;
//	mq_h.pid = vgui;
//	mq_h.cmd = SETPARA;
//	mq_h.bufsiz = sizeof(gdm_3761_tmp);
//	ret = message_send_data(mq_h,mqfd_ask,0,(INT8U *)&gdm_3761_tmp);
//	if(ret >=1){
//		//dbg_prt("\n vd ip：id parameter share memory success!\n");
//		mmq_close(mqfd_ask);
//	}
//#endif
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
//		fprintf(stderr, "\n GUI ---->zb ret=%d",ret);
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
//          set_time_show_flag(1);
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
//              set_time_show_flag(1);
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
	if(cldno->basicinfo.addr.addr[0] <= 0 || req_mq_name == NULL || msgbuf == NULL)
	{
		return 0;
	}

	if((mqd =  createMsg(req_mq_name, O_WRONLY)) <0)
	{
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
		colseMsg(mqd);
		return 0;
	}
	memset(msgbuf_tmp, 0, sizeof(msgbuf_tmp));
	PressKey = NOKEY;
	while(1)
	{
		if(PressKey!=NOKEY)
			g_PressKey_old = PressKey;
		delay(600);
		time_count++;
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
            set_time_show_flag(1);
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
			fprintf(stderr,"\ngui: -------------cur p_Proxy_Msg_Data->done_flag = %d\n",p_Proxy_Msg_Data->done_flag);
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
//				fprintf(stderr,"\n Dianchao:");
//				for(k = 0; k < sizeof(tmprealdatainfo.data); k++)
//					fprintf(stderr," %02x",tmprealdatainfo.data[k]);
//				fprintf(stderr,"\n");
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
	bzero(msgbuf,sizeof(msgbuf));
//	bzero(&msg_real,sizeof(Proxy_Msg));
	result = requestDataBlock(cldno,(INT8S*)mq_name,PROXY,msg_num,40,msgbuf);
//	fprintf(stderr,"\ngui: -------------cur rev msg from 485 result = %d\n",result);
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
//				fprintf(stderr,"\n Dianchao:");
//				for(k = 0; k < sizeof(tmprealdatainfo.data); k++)
//					fprintf(stderr," %02x",tmprealdatainfo.data[k]);
//				fprintf(stderr,"\n");
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

void show_realdatabycld(int pindex){
	CLASS_6001* cur_pindex = NULL;
	cur_pindex = (CLASS_6001*)pindex;
	int mqcount=0;

	LcdDataItem item[100];//存储的所有数据项
	if(cur_pindex == NULL)
		return;
	memset(item, 0, 100*sizeof(LcdDataItem));
	if(cur_pindex->basicinfo.port.OI == PORT_485){
		mqcount = requestdata_485_ZB_Block(cur_pindex,(INT8U*)PROXY_485_MQ_NAME,5, item);
	}
	show_realdata(cur_pindex->sernum, item, mqcount);
}

int gui_mp_compose(CLASS_6001 **ppgui_mp){
	CLASS_6001 *gui_mp=NULL;
	int i,j = 0,cld_max=0,cur_num;
	CLASS_6001	 meter={};
	CLASS11		coll={};
	if(readInterClass(0x6000,&coll)==0){
		return -1;
	}
	cur_num = coll.curr_num;
	cld_max = getFileRecordNum(0x6000);//获取文件测量点单元个数
	fprintf(stderr,"\n------coll.curr_num = %d----cld_max = %d-----\n",coll.curr_num,cld_max);
	if(cur_num>0){
		*ppgui_mp = (CLASS_6001*)malloc(cur_num*sizeof(CLASS_6001));
		memset(*ppgui_mp,0,cur_num*sizeof(CLASS_6001));
		gui_mp = *ppgui_mp;
		if(gui_mp==NULL){
			return -1;
		}
	}
	else return 0;
	for(i=0;i<cld_max;i++)
	{
		if(readParaClass(0x6000,&meter,i))
		{
			if(meter.sernum != 0 && meter.sernum != 0xFFFF)
			{
				if(meter.basicinfo.addr.addr[0]>TSA_LEN)//TSA地址长度异常
				{
					continue;
				}
				memcpy(&gui_mp[j],&meter,sizeof(CLASS_6001));
				j++;
			}
		}
	}
	return cur_num;
}
void gui_mp_free(CLASS_6001 *gui_mp){
	if(gui_mp==NULL)
		return;
	free(gui_mp);
}

#ifdef SHANGHAI
//cldaddr为表号后6位
void showselectmeter(char *cldaddr,char *box_addr)
{
	Gui_MP_t *gui_mp=NULL;
	int cur_cldno=1, begin_cldno=1, i=0, presskey_ok_acs=NOKEY;
	Point pos;
	char first_flg=0, str_cld[50], addr[13];
	Rect rect;
	int cld_max=0;
	cld_max = gui_mp_select(&gui_mp,box_addr);
	if(cld_max<=0){
		msgbox_label((char *)"未搜素到测量点", CTRL_BUTTON_OK);
		return;
	}
//	dbg_prt("\n 测量点总数为：%d",cld_max);
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		switch(PressKey)
		{
		case LEFT:
			if(cld_max>PAGE_COLNUM-1){
				if(begin_cldno!=1 ){
					begin_cldno -= PAGE_COLNUM-1;
					if(begin_cldno<=0)
						begin_cldno = 1;
					cur_cldno = begin_cldno;
				}else
					cur_cldno = begin_cldno = cld_max - (PAGE_COLNUM-1);
			}
			break;
		case UP:
			cur_cldno--;
			if(cur_cldno<=0 && cld_max>PAGE_COLNUM){
				cur_cldno = cld_max;
				begin_cldno = cld_max - PAGE_COLNUM + 2;
			}else if(cur_cldno<=0 && cld_max<=PAGE_COLNUM){
				cur_cldno = cld_max;
			}else if(cur_cldno<=begin_cldno)
				begin_cldno = cur_cldno;
			break;
		case RIGHT:
			if((begin_cldno+PAGE_COLNUM-1)>=cld_max){
				cur_cldno = begin_cldno = 1;
			}else{
				begin_cldno += PAGE_COLNUM-1;
				cur_cldno = begin_cldno;
			}
			break;
		case DOWN:
			cur_cldno++;
			if(cur_cldno>cld_max)
				begin_cldno = cur_cldno = 1;
			else if(cur_cldno>begin_cldno+PAGE_COLNUM-2)
				begin_cldno++;
			break;
		case OK:
			memcpy(cldaddr,(gui_mp+cur_cldno-1)->cldaddr,12);
			gui_mp_free(gui_mp);
			return;
		case ESC:
			gui_mp_free(gui_mp);
			return;
		default:
			break;
		}
		if(PressKey!=NOKEY||first_flg==0||presskey_ok_acs==OK){
			presskey_ok_acs = NOKEY;
			first_flg = 1;
			gui_clrrect(rect_Client);
			pos.x = rect_Client.left;
			pos.y = rect_Client.top+1;
			gui_textshow((char *)"测量点号        表地址", pos, LCD_NOREV);
			for(i=begin_cldno; i<begin_cldno+PAGE_COLNUM-1; i++){
				if(i>cld_max)
					continue;
				memset(str_cld, 0, 50);
				memset(addr, 0, 13);
				memcpy(addr, (gui_mp+i-1)->cldaddr, 12);
				sprintf(str_cld, "  %04d       %s",(gui_mp+i-1)->mpno, addr);
//				dbg_prt("\n [%d]str_cld=%s iindex=%d mpno=%d", i-1, str_cld,(gui_mp+i-1)->iidnex,(gui_mp+i-1)->mpno);
				pos.x = rect_Client.left;
				pos.y = rect_Client.top + (i-begin_cldno+1)*FONTSIZE*2 + 2;
				gui_textshow(str_cld, pos, LCD_NOREV);
				if(i==cur_cldno){
					memset(&rect, 0, sizeof(Rect));
					rect = gui_getstrrect((unsigned char*)str_cld, pos);//获得字符串区域
					gui_reverserect(rect);
				}
			}
			set_time_show_flag(1);
		}
		PressKey = NOKEY;
		delay(50);
	}
	gui_mp_free(gui_mp);
}

int gui_mp_select(Gui_MP_t **ppgui_mp,char *cldaddr){
	Gui_MP_t *gui_mp=NULL;
	int i, num=0, cld_max=0;
	for(i=0; i<MP_MAXNUM;i++){
		if(gui_isValidCld(i+1)>=1)
		{
			if(memcmp(cldaddr, &ParaAll->f10.para_mp[i].addr[6], 6)==0)//从第7位开始比较（比较后6位）
				num++;
		}
	}
	cld_max = num;
	if(cld_max>0){
		gui_mp = (Gui_MP_t*)malloc(cld_max*sizeof(Gui_MP_t));
		*ppgui_mp = gui_mp;
		if(gui_mp==NULL){
//			dbg_prt("\n malloc gui_mp error!!!");
			return -1;
		}
	}
	num = 0;
	for(i=0; i<MP_MAXNUM;i++){
		if(gui_isValidCld(i+1)>=1){
			if(memcmp(cldaddr, &ParaAll->f10.para_mp[i].addr[6], 6)==0)
			{
				(gui_mp+num)->iidnex = ParaAll->f10.para_mp[i].Index;
				(gui_mp+num)->mpno = ParaAll->f10.para_mp[i].MPNo;
				memcpy((gui_mp+num)->cldaddr, ParaAll->f10.para_mp[i].addr, 12);
//				dbg_prt("\n lcd_num =%d iindex=%d mpno=%d (gui_mp+num)->cldaddr=%s",
//						num,(gui_mp+num)->iidnex, (gui_mp+num)->mpno, (gui_mp+num)->cldaddr);
				num++;
			}
		}
	}
	return cld_max;
}
#endif

void showallmeter(void (*pfun)(int cldno))
{
	CLASS_6001* gui_mp = NULL;
//	Gui_MP_t *gui_mp=NULL;
	int cur_cldno=1, begin_cldno=1, i=0, presskey_ok_acs=NOKEY;
	Point pos;
	char first_flg=0, str_cld[50], addr[20];
	Rect rect;
	int cld_max=0;
	cld_max = gui_mp_compose(&gui_mp);
	if(cld_max<=0){
		msgbox_label((char *)"未配置测量点", CTRL_BUTTON_OK);
		return;
	}

	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		switch(PressKey)
		{
		case LEFT:
			if(cld_max>PAGE_COLNUM-1){
				if(begin_cldno!=1 ){
					begin_cldno -= PAGE_COLNUM-1;
					if(begin_cldno<=0)
						begin_cldno = 1;
					cur_cldno = begin_cldno;
				}else
					cur_cldno = begin_cldno = cld_max - (PAGE_COLNUM-1);
			}
			break;
		case UP:
			cur_cldno--;
			if(cur_cldno<=0 && cld_max>PAGE_COLNUM){
				cur_cldno = cld_max;
				begin_cldno = cld_max - PAGE_COLNUM + 2;
			}else if(cur_cldno<=0 && cld_max<=PAGE_COLNUM){
				cur_cldno = cld_max;
			}else if(cur_cldno<=begin_cldno)
				begin_cldno = cur_cldno;
			break;
		case RIGHT:
			if((begin_cldno+PAGE_COLNUM-1)>=cld_max){
				cur_cldno = begin_cldno = 1;
			}else{
				begin_cldno += PAGE_COLNUM-1;
				cur_cldno = begin_cldno;
			}
			break;
		case DOWN:
			cur_cldno++;
			if(cur_cldno>cld_max)
				begin_cldno = cur_cldno = 1;
			else if(cur_cldno>begin_cldno+PAGE_COLNUM-2)
				begin_cldno++;
			break;
		case OK:
			//TODO:交采请求
//			if(ParaAll->f10.para_mp[(gui_mp+cur_cldno-1)->iidnex-1].Port == PORT_JC)
//				presskey_ok_acs = OK;
//			getDayFilePath((gui_mp+cur_cldno-1)->mpno,Tcurr_tm_his.Year,Tcurr_tm_his.Month,Tcurr_tm_his.Day,(INT8U*)DongJie_FileName,100);
			pfun((int)(gui_mp+cur_cldno-1));
			gui_mp_free(gui_mp);
			cld_max = gui_mp_compose(&gui_mp);
			if(cld_max<=0){
				msgbox_label((char *)"未配置测量点", CTRL_BUTTON_OK);
				return;
			}
//			fprintf(stderr,"\n 测量点总数为：%d",cld_max);
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
			gui_textshow((char *)"配置序号        表地址", pos, LCD_NOREV);
			for(i=0; i<PAGE_COLNUM-1; i++){
				if(i>cld_max)
					continue;
				memset(str_cld, 0, 50);
				memset(addr, 0, sizeof(addr));
//				memcpy(addr, &(gui_mp+i-1)->basicinfo.addr.addr[2], (gui_mp+i-1)->basicinfo.addr.addr[1]+1);
				if((gui_mp+i)->sernum == 0) continue;

				addr_len = (gui_mp+i)->basicinfo.addr.addr[1]+1;

				bcd2str(&(gui_mp+i)->basicinfo.addr.addr[2],(INT8U*)addr,addr_len,sizeof(addr),positive);
				sprintf(str_cld, "  %04d       %s",(gui_mp+i)->sernum, addr);
//				fprintf(stderr,"\n-------index = %d--addr = %s-----\n",(gui_mp+i)->sernum, addr);
				pos.x = rect_Client.left;
				pos.y = rect_Client.top + (i+1)*FONTSIZE*2 + 2;
				gui_textshow(str_cld, pos, LCD_NOREV);
				if(i+1==cur_cldno){
					memset(&rect, 0, sizeof(Rect));
					rect = gui_getstrrect((unsigned char*)str_cld, pos);//获得字符串区域
					gui_reverserect(rect);
				}
			}
			set_time_show_flag(1);
		}
		PressKey = NOKEY;
		delay(50);
	}
	gui_mp_free(gui_mp);
}
//显示测量点当天数据
void menu_showclddata()
{
#ifdef JIANGSU
	showallmeter(show_realdatabycld_js);
	g_curcldno=1;
#else
	showallmeter(show_realdatabycld);
#endif
}
/***********************************************
* 函数说明：查询当前数据，冻结数据，曲线数据
* 输入参数：FileName 要查询文件名
* 		  source   要查询的结构体指针
* 		  size     结构体大小
* 		  flag     结构体类型 1，实时；2，冻结
* 返回值：1 成功 0失败
************************************************/

int read_filedata(char* FileName, int point, int did, INT8U flag, void *source)
{
//	FILE 	*fp=NULL;
//	int		fd,i,num=0, size=0, find_flg=0;
//	int		recodenum=0;
//	struct 	stat  info;
//	dbg_prt("\n FileName = %s", FileName);
//	if(access(FileName, 0)==0){
//		fp = fopen((const char*)FileName, "r");
//		if(fp != NULL){
//			fd = fileno(fp);
//			if(fstat(fd, &info)==-1){
//				recodenum = 0;
//				dbg_prt("\n fstat error:%d %s", errno, strerror(errno));
//				if(fp!=NULL){
//					fclose(fp);
//					fp = NULL;
//				}
//				return -1;
//			}
//			if(flag == CURR_DATA)
//			{
//				CurrentData_SAVE *cd; //_s原  _o目的
//				cd =(CurrentData_SAVE *) source;
//				size = sizeof(CurrentData_SAVE);
//				recodenum = info.st_size / size;
//				find_flg = 0;
//				//文件记录个数
//				for(i=0;i<recodenum;i++){
//					memset(cd,0,sizeof(CurrentData_SAVE));
//					num=fread(cd, size, 1, fp);
//					if(num == 1){
//						if((cd->id_d == did)&&(cd->id_mp == point)){
//							find_flg = 1;
//							if(fp!=NULL){
//								fclose(fp);
//								fp = NULL;
//							}
//							return 1;
//						}
//					}
//				}
//				if(find_flg==0){
//					if(fp!=NULL){
//						fclose(fp);
//						fp = NULL;
//					}
//				}
//			}
//			if(flag == DONGJIE_DATA)
//			{
//				FreezeData_SAVE *fd;
//				fd =(FreezeData_SAVE*) source;
//				size = sizeof(FreezeData_SAVE);
//				recodenum = info.st_size / size;				//文件记录个数
//				find_flg = 0;
//				dbg_prt("\n recodenum=%d info.st_size=%d\n",recodenum,(int)info.st_size);
//				for(i=0;i<recodenum;i++)
//				{
//					memset(fd,0,sizeof(FreezeData_SAVE));
//					num=fread(fd,size,1,fp);
//					if(num == 1){
//						if((fd->id_d == did)&&(fd->id_mp == point)){
//							find_flg = 1;
//							if(fp!=NULL){
//								fclose(fp);
//								fp = NULL;
//							}
//							return 1;
//						}
//					}
//				}
//				if(find_flg==0){
//					if(fp!=NULL){
//						fclose(fp);
//						fp = NULL;
//					}
//				}
//			}
//		}else
//			dbg_prt("\n fopen error:%d %s", errno, strerror(errno));
//	}else{
//		dbg_prt("\n access error:%d %s", errno, strerror(errno));
//		return -2;
//	}
	return 0;
}
int read_filedata_curve(char* FileName, int point, int did, int hour, int minute, void *source)
{
//	FILE 	*fp=NULL;
//	int		fd,i,num=0, size=0, find_flg=0;
//	int		recodenum=0;
//	struct 	stat  info;
//	CurveData_SAVE *cs;
//	cs =(CurveData_SAVE *) source;
//	dbg_prt("\n FileName = %s point=%d did=%d hour:minute=%02d:%02d", FileName,point,did,hour,minute);
//	if(access(FileName, 0)==0){
//		fp = fopen((const char*)FileName, "r");
//		if(fp != NULL){
//			fd = fileno(fp);
//			if(fstat(fd, &info)==-1){
//				recodenum = 0;
//				dbg_prt("\n fstat error:%d %s", errno, strerror(errno));
//				if(fp!=NULL){
//					fclose(fp);
//					fp = NULL;
//				}
//				return -1;
//			}
//			size = sizeof(CurveData_SAVE);
//			recodenum = info.st_size / size;				//文件记录个数
//			find_flg = 0;
//			for(i=0;i<recodenum;i++){
//				memset(cs,0,sizeof(CurveData_SAVE));
//				num=fread(cs, size, 1, fp);
//				if(num == 1){
//					dbg_prt("\n point=%d did=%d hour:minute=%02d:%02d cs->id_mp=%d cs->id_d=%d hour:minute=%02d:%02d",
//							point,did,hour,minute,cs->id_mp,cs->id_d,cs->tm_collect.Hour,cs->tm_collect.Minute);
//					if((cs->id_d == did)&&(cs->id_mp == point)&&
//						(cs->tm_collect.Hour==hour)&&(cs->tm_collect.Minute==minute)){
//						dbg_prt( "     find OK");
//						find_flg = 1;
//						if(fp!=NULL){
//							fclose(fp);
//							fp = NULL;
//						}
//						return 1;
//					}
//				}
//			}
//			if(find_flg==0){
//				if(fp!=NULL){
//					fclose(fp);
//					fp = NULL;
//				}
//			}
//		}else
//			dbg_prt("\n fopen error:%d %s", errno, strerror(errno));
//	}else{
//		dbg_prt("\n access error:%d %s", errno, strerror(errno));
//		return -2;
//	}
	return 0;
}
//name 数据项名称 dataid 数据项ID len 小数点后的有效位数   位置：pos_x pos_y
void dataitem_showvalue(INT8U *filename, int cldno, char *idname,
								int dataid, int len, int pos_x, int pos_y){
	char str[100];
	LcdDataItem lcd_data;
	FreezeData_SAVE fd_s;
	float fval;
	memset(&lcd_data, 0, sizeof(LcdDataItem));
	memset(str, 0, 100);
	if(read_filedata((char*)filename, cldno, dataid, DONGJIE_DATA, (void*)&fd_s)==1){
		memcpy(lcd_data.val, fd_s.data, LcdDataItem_VALLEN);
		fval = bcd2double((INT8U*)lcd_data.val, LcdDataItem_VALLEN,2, positive);
//		dbg_prt( "\n fval=%f", fval);
		if(len==2)
			sprintf(str,"%s % 9.2f kWh", idname, fval);
		else
			sprintf(str,"%s % 8.4f kWh", idname, fval);
	}else
		sprintf(str,"%s xxxxxx.xx kWh", idname);
	lcd_data.pos.x = pos_x;
	lcd_data.pos.y = pos_y;
	gui_textshow(str, lcd_data.pos, LCD_NOREV);
}
void show_day_djdata(int cldno){
	char str[100];
	TS cj_date;
	FreezeData_SAVE fd_s;
	Point pos;
	memset(&cj_date, 0, sizeof(TS));
	gui_clrrect(rect_Client);
	gui_setpos(&pos, rect_Client.left+6*FONTSIZE, rect_Client.top+FONTSIZE);
	gui_textshow((char*)"正向有功电能示值", pos, LCD_NOREV);
					// 测量点 DID 位数 位置
	dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功总:", 824, 2, 0, rect_Client.top + FONTSIZE*4);
#ifdef SHANGHAI
	if(ParaAll->f10.para_mp[cldno-1].Protocol == 21)
	{
		if(ParaAll->f10.para_mp[cldno-1].RateNum == 2)
		{
			dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功平:", 825, 2, 0, rect_Client.top + FONTSIZE*6+3);
			dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功谷:", 826, 2, 0, rect_Client.top + FONTSIZE*8+6);
		}
		else
		{
			dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功尖:", 825, 2, 0, rect_Client.top + FONTSIZE*6+3);
			dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功峰:", 827, 2, 0, rect_Client.top + FONTSIZE*8+6);
			dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功平:", 826, 2, 0, rect_Client.top + FONTSIZE*10+9);
			dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功谷:", 828, 2, 0, rect_Client.top + FONTSIZE*12+12);
		}
	}
	else
	{
		dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功尖:", 825, 2, 0, rect_Client.top + FONTSIZE*6+3);
		dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功峰:", 826, 2, 0, rect_Client.top + FONTSIZE*8+6);
		dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功平:", 827, 2, 0, rect_Client.top + FONTSIZE*10+9);
		dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功谷:", 828, 2, 0, rect_Client.top + FONTSIZE*12+12);
	}
#else
	dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功尖:", 825, 2, 0, rect_Client.top + FONTSIZE*6+3);
	dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功峰:", 826, 2, 0, rect_Client.top + FONTSIZE*8+6);
	dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功平:", 827, 2, 0, rect_Client.top + FONTSIZE*10+9);
	dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功谷:", 828, 2, 0, rect_Client.top + FONTSIZE*12+12);
#endif
	if(read_filedata((char*)DongJie_FileName, cldno, 824, DONGJIE_DATA, (void*)&fd_s)==1)
		memcpy(&cj_date, &fd_s.tm_collect, sizeof(TS));
	memset(str, 0, 100);
	if(cj_date.Year==0|| cj_date.Month==0||cj_date.Day==0)
		sprintf((char*)str,"抄表时间 00/00/00 00:00");
	else
		sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
			cj_date.Year-2000, cj_date.Month, cj_date.Day, cj_date.Hour, cj_date.Minute);
	gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+18*FONTSIZE);
	gui_textshow(str, pos, LCD_NOREV);
    set_time_show_flag(1);
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		if(PressKey==ESC)
			break;
		PressKey = NOKEY;
		delay(300);
	}
}
void show_mon_djdata(int cldno){
//	char str[100];
//	TmS cj_date;
//	FreezeData_SAVE fd_s;
//	Point pos;
//	memset(&cj_date, 0, sizeof(TmS));
//	gui_clrrect(rect_Client);
//	gui_setpos(&pos, rect_Client.left+6*FONTSIZE, rect_Client.top+FONTSIZE);
//	gui_textshow((char*)"正向有功电能示值", pos, LCD_NOREV);
//					// 测量点 DID 位数 位置
//	dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功总:", 924, 2, 0, rect_Client.top + FONTSIZE*4);
//#ifdef SHANGHAI
//	if(ParaAll->f10.para_mp[cldno-1].Protocol == 21)
//	{
//		if(ParaAll->f10.para_mp[cldno-1].RateNum == 2)
//		{
//			dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功平:", 925, 2, 0, rect_Client.top + FONTSIZE*6+3);
//			dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功谷:", 926, 2, 0, rect_Client.top + FONTSIZE*8+6);
//		}
//		else
//		{
//			dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功尖:", 925, 2, 0, rect_Client.top + FONTSIZE*6+3);
//			dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功峰:", 927, 2, 0, rect_Client.top + FONTSIZE*8+6);
//			dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功平:", 926, 2, 0, rect_Client.top + FONTSIZE*10+9);
//			dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功谷:", 928, 2, 0, rect_Client.top + FONTSIZE*12+12);
//		}
//	}
//	else
//	{
//		dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功尖:", 925, 2, 0, rect_Client.top + FONTSIZE*6+3);
//		dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功峰:", 926, 2, 0, rect_Client.top + FONTSIZE*8+6);
//		dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功平:", 927, 2, 0, rect_Client.top + FONTSIZE*10+9);
//		dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功谷:", 928, 2, 0, rect_Client.top + FONTSIZE*12+12);
//	}
//#else
//	dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功尖:", 925, 2, 0, rect_Client.top + FONTSIZE*6+3);
//	dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功峰:", 926, 2, 0, rect_Client.top + FONTSIZE*8+6);
//	dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功平:", 927, 2, 0, rect_Client.top + FONTSIZE*10+9);
//	dataitem_showvalue(DongJie_FileName,cldno, (char*)"正向有功谷:", 928, 2, 0, rect_Client.top + FONTSIZE*12+12);
//#endif
//	if(read_filedata((char*)DongJie_FileName, cldno, 924, DONGJIE_DATA, (void*)&fd_s)==1)
//		memcpy(&cj_date, &fd_s.tm_collect, sizeof(TmS));
//	memset(str, 0, 100);
//	if(cj_date.Year==0|| cj_date.Month==0||cj_date.Day==0)
//		sprintf((char*)str,"抄表时间 00/00/00 00:00");
//	else
//		sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
//			cj_date.Year-2000, cj_date.Month, cj_date.Day, cj_date.Hour, cj_date.Minute);
//	gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+18*FONTSIZE);
//	gui_textshow(str, pos, LCD_NOREV);
//  set_time_show_flag(1);
//	PressKey = NOKEY;
//	while(g_LcdPoll_Flag==LCD_NOTPOLL){
//		if(PressKey==ESC)
//			break;
//		PressKey = NOKEY;
//		delay(300);
//	}
}
void menu_showdaydata(){
	int year=0, month=0, day=0;
	char s_date[20],str[5],old_date[20];
	memset(s_date, 0, 20);
	TS curr_tm, curr_tm_his;
	TSGet(&curr_tm);
	tminc(&curr_tm, day_units, -1);
	sprintf(s_date, "%04d%02d%02d",curr_tm.Year,curr_tm.Month, curr_tm.Day);
	sprintf(old_date, "%04d%02d%02d",curr_tm.Year,curr_tm.Month, curr_tm.Day);
//	dbg_prt("\n s_jzqtime=%s", s_date);
#ifdef JIANGSU
	int edit_ret=0;
//	fprintf(stderr,"s_date %s \n",s_date);
	edit_ret = editctrl_time(s_date, strlen(s_date));
	if(edit_ret>0)
	{
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
		memset(s_date, 0, 20);
		memset(DongJie_FileName, 0, 100);
		memcpy(&Tcurr_tm_his,&curr_tm_his,sizeof(curr_tm_his));
		showallmeter(show_daydata_JS);
		g_curcldno=1;
	}
#else
	int msgbox_ret=0;
	msgbox_ret = msgbox_inputjzqtime(s_date, strlen(s_date));
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
		//tminc(&curr_tm_his, DAY, -1);
		memset(s_date, 0, 20);
//		sprintf(s_date, "%04d%02d%02d",curr_tm_his.Year,curr_tm_his.Month, curr_tm_his.Day);
		memset(DongJie_FileName, 0, 100);
//		sprintf((char*)DongJie_FileName,"%s/%s.dat",SAVE_PATH_DAY,s_date);
//		dbg_prt("\n DongJie_FileName = %s", DongJie_FileName);
		memcpy(&Tcurr_tm_his,&curr_tm_his,sizeof(curr_tm_his));
		showallmeter(show_day_djdata);
	}
#endif
}

void menu_showmonthdata(){
//	int year=0, month=0;
//	char s_date[20], str[5];
//	memset(s_date, 0, 20);
//	TmS curr_tm, curr_tm_his;
//	tmget(&curr_tm);
//	tminc(&curr_tm, MONTH, -1);
//	sprintf(s_date, "%04d%02d", curr_tm.Year, curr_tm.Month);
//	dbg_prt("\n s_jzqtime=%s", s_date);
//#ifdef JIANGSU
//	int edit_ret=0;
//	edit_ret = editctrl_time(s_date, strlen(s_date));
//	if(edit_ret>0)
//	{
//		memset(str, 0, 5);
//		memcpy(str, &s_date[0], 4);
//		year = atoi(str);
//		memset(str, 0, 5);
//		memcpy(str, &s_date[4], 2);
//		month = atoi(str);
//		tmass(&curr_tm_his,year,month,0,0,0,0);
//		memset(s_date, 0, 20);
//		sprintf(s_date, "%04d%02d", curr_tm_his.Year, curr_tm_his.Month);
//		memset(DongJie_FileName, 0, 100);
//		sprintf((char*)DongJie_FileName, "%s/%s.dat", SAVE_PATH_MONTH, s_date);
//		dbg_prt("\n DongJie_FileName = %s", DongJie_FileName);
//		memcpy(&Tcurr_tm_his,&curr_tm_his,sizeof(curr_tm_his));
//		showallmeter(show_monthdata_JS);
//		g_curcldno=1;
//	}
//#else
//	char msgbox_ret=0;
//	msgbox_ret = msgbox_inputjzqtime(s_date, strlen(s_date));
//		if(msgbox_ret == ACK){
//			memset(str, 0, 5);
//			memcpy(str, &s_date[0], 4);
//			year = atoi(str);
//			memset(str, 0, 5);
//			memcpy(str, &s_date[4], 2);
//			month = atoi(str);
//			//memset(&curr_tm_his,0,sizeof(curr_tm_his));
//			tmass(&curr_tm_his,year,month,0,0,0,0);
//			memset(s_date, 0, 20);
//			sprintf(s_date, "%04d%02d", curr_tm_his.Year, curr_tm_his.Month);
//			memset(DongJie_FileName, 0, 100);
//			sprintf((char*)DongJie_FileName, "%s/%s.dat", SAVE_PATH_MONTH, s_date);
//			dbg_prt("\n DongJie_FileName = %s", DongJie_FileName);
//			memcpy(&Tcurr_tm_his,&curr_tm_his,sizeof(curr_tm_his));
//			showallmeter(show_mon_djdata);
//		}
//#endif
}
//返回控件的输入值 就是把form.key[i].c组合成一个字符串
void eidt_gettext(Edit *edit, char *text)
{
	int i;
	for(i=0; i<edit->form.c_num; i++)
		text[i] = edit->form.key[i].c;
}
//数组下标转实际的端口号
int index2port(int index){
	int port=0;
	if(index==0)//485I
		port = PORT_485;
	else if(index==1)//PORT_ZB
		port = PORT_ZB;
	else if(index==2)//JC
		port = PORT_JC;
	return port;
}
//实际的端口号转数组下标
int port2index(int port){
	int index=0;
	if(port==PORT_485)
		index = 0;
	else if(port==PORT_ZB)
		index = 1;
	else if(port==PORT_JC)
		index = 2;
	return index;
}
static void setmp_cbtext_port(char cb_text[][TEXTLEN_Y]){
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "RS485", strlen("RS485"));
	memcpy(cb_text[1], "载波口", strlen("载波口"));
	memcpy(cb_text[2], "交采口", strlen("交采口"));
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

int getxiaoleibycb(int cb_index, int daleihao){
	int i, item_count=0, index=0;
	for(i=0; i<16; i++){
		if(tXiaoLei[daleihao][i].index==0 && i!=0)
			continue;
		if(cb_index == item_count){
			index = i;
			break;
		}
		item_count++;
	}
	return tXiaoLei[daleihao][index].index;
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
#ifdef SPTF_III
	gui_textshow((char *)"局编号:", label_pos, LCD_NOREV);
#endif
	set_time_show_flag(1);
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
#ifdef SPTF_III
	gui_textshow((char *)"局编号:", label_pos, LCD_NOREV);
#endif
	set_time_show_flag(1);
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

void setmeterpara(int pindex)
{
	CLASS_6001 meter;
	CLASS_6001* cur_pindex = NULL;
	cur_pindex = (CLASS_6001*)pindex;
	int tmp=0, f10_flg=1,addr_len = 0;
//	para_1mp para_f10;
#ifdef SPTF_III
	para_F29 para_f29;
#endif
	int form_cldno=0;//通过控件修改的测量点地址
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
	Edit edit_cldno, edit_cldaddr, edit_rate,edit_usr_type,edit_cjqaddr;
	Combox cb_port,cb_protocol,cb_baud,cb_con_method;

	memset(&edit_cldno, 0, sizeof(Edit));
	memset(&edit_cldaddr, 0, sizeof(Edit));
	memset(&cb_port, 0, sizeof(Combox));
	memset(&cb_protocol, 0, sizeof(Combox));
	memset(&cb_baud, 0, sizeof(Combox));
	memset(&edit_rate, 0, sizeof(Edit));
	memset(&edit_usr_type, 0, sizeof(Edit));
	memset(&cb_con_method, 0, sizeof(Combox));
	memset(&edit_cjqaddr, 0, sizeof(Edit));

	memcpy(&meter,cur_pindex,sizeof(CLASS_6001));
	if(pindex<1)
		 return;
	g_curcldno = pindex;//cldno;
	memset(&client, 0, sizeof(Form));
	client.node.child = (struct list*)malloc(sizeof(struct list));
	if(client.node.child==NULL){
		g_curcldno = 1;//上状态栏显示
		return;
	}
	memset(client.node.child, 0, sizeof(struct list));
	gui_clrrect(rect_Client);
	Point pos;
	pos.x = rect_Client.left+FONTSIZE*7.5;
	pos.y = rect_Client.top;
	//------------------------------------------------------------
	pos.y += ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	sprintf(str,"%04d", cur_pindex->sernum);
	edit_init(&edit_cldno, str, 4, pos, 0, 0, client.node.child,KEYBOARD_DEC);//配置序号
	//------------------------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
//	memcpy(pos_addr,pos,sizeof(Point));
	memset(str, 0, INPUTKEYNUM);
	memset(sever_addr,0,sizeof(sever_addr));
	addr_len = cur_pindex->basicinfo.addr.addr[1];
	bcd2str(&cur_pindex->basicinfo.addr.addr[2],(INT8U*)sever_addr,addr_len,sizeof(str),positive);
	sscanf(sever_addr,"%[0-9]",str);
	edit_init(&edit_cldaddr, str, 16, pos, 0, 0, client.node.child,KEYBOARD_DEC);//表地址
	//------------------------------------------
	setmp_cbtext_port(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	combox_init(&cb_port, port2index(cur_pindex->basicinfo.port.OI), cb_text, pos, 0,client.node.child);//端口
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
	edit_init(&edit_rate, str, 3, pos, 0, 0, client.node.child,KEYBOARD_DEC);//费率个数
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
//#ifdef CCTT_I
	addr_len = cur_pindex->extinfo.cjq_addr.addr[1]+1;
	bcd2str(&cur_pindex->extinfo.cjq_addr.addr[2],(INT8U*)sever_addr,addr_len,sizeof(str),positive);
	sscanf(sever_addr,"%[0-9]",str);
//	memcpy(str, &cur_pindex->extinfo.cjq_addr.addr[2], cur_pindex->extinfo.cjq_addr.addr[1]+1);
	edit_init(&edit_cjqaddr, str, 16, pos, 0, 0, client.node.child,KEYBOARD_DEC);//采集器地址
//#endif
#ifdef SPTF_III
	if(ParaAll->f10.para_mp[pindex-1].MPNo >0)
	{
		memcpy(str, ParaAll->f29s.f29[ParaAll->f10.para_mp[pindex-1].MPNo-1].MeterDisplayNo, 12);
	}
	edit_init(&edit_cjqaddr, str, 12, pos, 0, 0, client.node.child,KEYBOARD_ASC);//局编号
#endif
	//------------------------------------------
	cur_node = &edit_cldno.form.node;
	cur_form = &edit_cldno.form;
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

						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_cldno, str);
						form_cldno = atoi(str);
						meter.sernum = form_cldno;

						if(form_cldno>2048||form_cldno<=0)
							f10_flg = 0;

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
							meter.basicinfo.addr.addr[1] = meter.basicinfo.addr.addr[1] - 1;
						}

						//配置序号、表地址、端口、规约、通信速率、费率个数、接线方式、采集器地址
						meter.basicinfo.port.OI = cb_port.cur_index;
						meter.basicinfo.port.attflg = 2;
						meter.basicinfo.port.attrindex = 1;

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
						sscanf(str,"%*[^0-9]%[0-9]",bak_addr);
//						strncpy((char*)bak_addr,str,sizeof(bak_addr)-1);
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
							meter.extinfo.cjq_addr.addr[1] = meter.extinfo.cjq_addr.addr[1] - 1;
						}
//#endif
#ifdef SPTF_III
						if(ParaAll->f10.para_mp[pindex-1].MPNo >0)
							memcpy(&para_f29, &ParaAll->f29s.f29[ParaAll->f10.para_mp[pindex-1].MPNo-1], sizeof(para_F29));
						eidt_gettext(&edit_cjqaddr, (char*)para_f29.MeterDisplayNo);
						setpara(&para_f29, ParaAll->f10.para_mp[pindex-1].MPNo, 29, sizeof(para_F29));
#endif
						if(f10_flg==1){
//							fprintf(stderr,"\n-----begin---save------\n");
							saveParaClass(0x6000,(unsigned char*)&meter,meter.sernum);
						}else
							msgbox_label((char *)"保存失败！", CTRL_BUTTON_CANCEL);
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
			set_time_show_flag(1);
			first_flg = 1;
		}
		PressKey = NOKEY;
		delay(100);
	}
	if(client.node.child!=NULL)
		free(client.node.child);
}

/*
 * 目前处理的添加测量点为自动寻找最小可配置序号，对已存在的配置序号不进行修改
 * */
void addmeter()
{
	CLASS_6001 meter;
	CLASS11 coll = {};
	int i = 0,blknum = 0,free_space = 0;
	int tmp=0, f10_flg=1,addr_len = 0,rate_chg = 0;
//	para_1mp para_f10;
#ifdef SPTF_III
	para_F29 para_f29;
#endif
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
	Edit edit_cldaddr,edit_rate, edit_cjqaddr;
	Combox cb_port,cb_protocol,cb_baud,cb_con_method;

	memset(&meter,0,sizeof(CLASS_6001));
	memset(&edit_cldaddr, 0, sizeof(Edit));
	memset(&cb_port, 0, sizeof(Combox));
	memset(&cb_protocol, 0, sizeof(Combox));
	memset(&cb_baud, 0, sizeof(Combox));
	memset(&edit_rate,0,sizeof(Edit));
	memset(&cb_con_method,0,sizeof(Edit));
	memset(&edit_cjqaddr, 0, sizeof(Edit));

	if(readInterClass(0x6000,&coll)==0)
	{
		saveParaClass(0x6000,&meter,meter.sernum);
//		return;//打开文件失败
	}
	blknum = getFileRecordNum(0x6000);
	for(i=1;i<blknum+1;i++)
	{
		readParaClass(0x6000,&meter,i);
		if(meter.sernum == 0)
		{
			free_space = i;//可添加的最小配置序号
			break;
		}
		meter.sernum =0;
	}
	if(free_space == 0)
	{
		if(blknum == 0){
			free_space = 1;
		}
		else if(blknum >= MAX_POINT_NUM){
			msgbox_label((char *)"已达上限！", CTRL_BUTTON_CANCEL);
			return;//测量点已达到最大数量，不能进行添加测量点操作
		}
	}
	g_curcldno = free_space;

	memset(&client, 0, sizeof(Form));
	client.node.child = (struct list*)malloc(sizeof(struct list));
	if(client.node.child==NULL){
		g_curcldno = 1;//上状态栏显示
		return;
	}
	memset(client.node.child, 0, sizeof(struct list));
	gui_clrrect(rect_Client);
//    set_time_show_flag(1);
	//往控件里放入测量点数据 应该在控件init里设置
	Point pos;
	Point pos_index;
	//配置序号、表地址、端口、规约、通信速率、费率个数、接线方式、采集器地址
	pos.x = rect_Client.left+FONTSIZE*7.5;
	pos.y = rect_Client.top;
	pos.y += ROW_INTERVAL;
	memcpy(&pos_index,&pos,sizeof(Point));
	memset(str, 0, INPUTKEYNUM);
	sprintf(str,"%04d", free_space);
	gui_textshow(str,pos,LCD_NOREV);
//	edit_init(&edit_indexno, str, 4, pos, 0, 0, client.node.child,KEYBOARD_DEC);//配置序号
	//------------------------------------------------------------
	pos.x = rect_Client.left+FONTSIZE*7.5;
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	edit_init(&edit_cldaddr, str, 16, pos, 0, 0, client.node.child,KEYBOARD_HEX);//表地址
	//------------------------------------------
	setmp_cbtext_port(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	combox_init(&cb_port, 0, cb_text, pos, 0,client.node.child);//端口
	//------------------------------------------
	setmp_cbtext_guiyue(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	combox_init(&cb_protocol, 0, cb_text, pos, 0,client.node.child);//规约
	//------------------------------------------
	setmp_cbtext_baud(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	combox_init(&cb_baud, 11, cb_text, pos, 0,client.node.child);//通信速率
	//------------------------------------------
	pos.x = rect_Client.left+FONTSIZE*9.5;
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
//	memcpy(str, cur_pindex->basicinfo.ratenum, 1);
	edit_init(&edit_rate, str, 3, pos, 0, 0, client.node.child,KEYBOARD_DEC);//费率个数
	//------------------------------------------
	setmp_cbtext_con_type(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	combox_init(&cb_con_method, 0, cb_text, pos, 0,client.node.child);//接线方式
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
//#ifdef CCTT_I
	edit_init(&edit_cjqaddr, str, 16, pos, 0, 0, client.node.child,KEYBOARD_HEX);//采集器地址
//#endif
#ifdef SPTF_III
	if(ParaAll->f10.para_mp[pindex-1].MPNo >0)
	{
	memcpy(str, ParaAll->f29s.f29[ParaAll->f10.para_mp[pindex-1].MPNo-1].MeterDisplayNo, 12);
	dbg_prt( "\n para_f29.MeterDisplayNo=%s---0",
			ParaAll->f29s.f29[ParaAll->f10.para_mp[pindex-1].MPNo-1].MeterDisplayNo);
	}
	edit_init(&edit_cjqaddr, str, 12, pos, 0, 0, client.node.child,KEYBOARD_ASC);//局编号
#endif
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
					meter.sernum = free_space;//配置序号

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
//					fprintf(stderr,"\n------addr_len = %d-------\n",addr_len);
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
						meter.basicinfo.addr.addr[1] = meter.basicinfo.addr.addr[1] - 1;
					}

					meter.basicinfo.port.OI = cb_port.cur_index;
					meter.basicinfo.port.attflg = 2;
					meter.basicinfo.port.attrindex = 1;

					meter.basicinfo.protocol = cb_protocol.cur_index;

					memset(str,0,sizeof(str));
					eidt_gettext(&edit_rate,str);
					rate_chg= atoi(str);
					if(rate_chg>255){
						rate_chg = 255;
					}
					meter.basicinfo.ratenum = rate_chg;

					meter.basicinfo.connectype = cb_con_method.cur_index;
//	#ifdef CCTT_I
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
						meter.extinfo.cjq_addr.addr[1] = meter.extinfo.cjq_addr.addr[1] - 1;
					}
//	#endif
	#ifdef SPTF_III
					if(ParaAll->f10.para_mp[pindex-1].MPNo >0)
						memcpy(&para_f29, &ParaAll->f29s.f29[ParaAll->f10.para_mp[pindex-1].MPNo-1], sizeof(para_F29));
					eidt_gettext(&edit_cjqaddr, (char*)para_f29.MeterDisplayNo);
					dbg_prt( "\n para_f29.MeterDisplayNo=%s---1", para_f29.MeterDisplayNo);
					setpara(&para_f29, ParaAll->f10.para_mp[pindex].MPNo, 29, sizeof(para_F29));
	#endif
					if(f10_flg==1){
//						fprintf(stderr,"\n-----begin---save------\n");
						saveParaClass(0x6000,(unsigned char*)&meter,meter.sernum);
					}else
						msgbox_label((char *)"保存失败！", CTRL_BUTTON_CANCEL);
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
			memset(str, 0, INPUTKEYNUM);
			sprintf(str,"%04d", free_space);
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
            set_time_show_flag(1);
			first_flg = 1;
		}
		PressKey = NOKEY;
		delay(100);
	}
	if(client.node.child!=NULL)
		free(client.node.child);
}

void deletemeter(int pindex)
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
#ifdef JIANGSU
	fprintf(stderr,"OprMode == %d \n",get_oprmode());
	if(get_oprmode() == OPRMODE_LOOK)
		showallmeter(querymeterpara_js);
	else if (get_oprmode() == OPRMODE_MODIFY)
		showallmeter(setmeterpara_js);
#else
	showallmeter(setmeterpara);
#endif
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
#ifdef JIANGSU
	ret = msgbox_password_js(passwd, 6,s_passwd_ret);
#else
	ret = msgbox_setpasswd(passwd, 6, s_passwd_ret);
#endif
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
#ifndef JIANGSU
		msgbox_label((char *)"密码错误！", CTRL_BUTTON_OK);
#endif
	}
}

INT16U Search_Index(INT16U pn)
{
//	INT16U Index = 0;
//	INT16U i = 0;
//	for(i=0;i<MP_MAXNUM;i++)
//	{
//		if(pn == shmm_getpara()->f10.para_mp[i].MPNo)
//		{
//			Index = shmm_getpara()->f10.para_mp[i].Index;
//			return Index;
//		}
//	}
	return 0;
}

void menu_readmeterbycldno(){
//	int mqcount=0;
//	LcdDataItem item[100];//存储的所有数据项
//	memset(item, 0, 100*sizeof(LcdDataItem));
//	int cldno=0;
//	cldno = msgbox_inputcldno();
//	int pindex = Search_Index(cldno);
//	fprintf(stderr,"pindex = %d\n",pindex);
//	if(pindex>1 && gui_isValidCld(pindex)>0){
//		fprintf(stderr,"\n gui_isValidCld(cldno)=%d",gui_isValidCld(pindex));
//	}
//	else
//	{
//		if(cldno != -1)
//			msgbox_label((char*)"无此测量点", CTRL_BUTTON_OK);
//		return;
//	}
//	dbg_prt( "\n cldno=%d", cldno);
//	if(cldno>0){
//		if(memcmp(ParaAll->f10.para_mp[pindex-1].addr, "000000000000", 12)!=0)//判断表地址的有效性
//		{
//			if(ParaAll->f10.para_mp[pindex-1].Port==PORT_ZB)
//				mqcount = requestdata_485_ZB_Block(ParaAll->f10.para_mp[pindex-1].MPNo, (INT8U*)PLC_REV_GUI_MQ,5, 3117, item);
//			else if(ParaAll->f10.para_mp[pindex-1].Port == PORT_485I)
//				mqcount = requestdata_485_ZB_Block(ParaAll->f10.para_mp[pindex-1].MPNo, (INT8U*)S485_1_REV_GUI_MQ,5, 3117, item);
//			else if(ParaAll->f10.para_mp[pindex-1].Port == PORT_485II)
//				mqcount = requestdata_485_ZB_Block(ParaAll->f10.para_mp[pindex-1].MPNo, (INT8U*)S485_2_REV_GUI_MQ,5, 3117, item);
//		}
//		show_realdata(pindex, item,mqcount);
//	}
}

void menu_readmeterbycldaddr()
{
//	int mqcount=0, ret=0;
//	LcdDataItem item[100];//存储的所有数据项
//	memset(item, 0, 100*sizeof(LcdDataItem));
//	int pindex=0;
//	INT8U cldaddr[12];
//	memset(cldaddr, 0, 12);
//#ifdef SHANGHAI
//	ret = msgbox_inputcldaddr((char*)cldaddr,6);
//#else
//	ret = msgbox_inputcldaddr((char*)cldaddr,12);//返回按键值
//#endif
//	fprintf(stderr,"ret = %d\n",ret);
//	if(ret!=0)
//		pindex =  gui_isValidCldAddr(cldaddr);
//	fprintf(stderr,"pindex = %d\n",pindex);
//	if(pindex>0&&ret!=0){
//		if(ParaAll->f10.para_mp[pindex-1].Port==PORT_ZB)
//			//mqcount = requestdata_485_ZB(ParaAll->f10.para_mp[pindex-1].MPNo, (INT8U*)PLC_REV_GUI_MQ, arr_did, 1, item);
//			mqcount = requestdata_485_ZB_Block(ParaAll->f10.para_mp[pindex-1].MPNo, (INT8U*)PLC_REV_GUI_MQ,5, 3117, item);
//		else if(ParaAll->f10.para_mp[pindex-1].Port == PORT_485I)
//			mqcount = requestdata_485_ZB_Block(ParaAll->f10.para_mp[pindex-1].MPNo, (INT8U*)S485_1_REV_GUI_MQ,5, 3117, item);
//			//mqcount = requestdata_485_ZB(ParaAll->f10.para_mp[pindex-1].MPNo, (INT8U*)S485_1_REV_GUI_MQ, arr_did, 1, item);
//		else if(ParaAll->f10.para_mp[pindex-1].Port == PORT_485II)
//			mqcount = requestdata_485_ZB_Block(ParaAll->f10.para_mp[pindex-1].MPNo, (INT8U*)S485_2_REV_GUI_MQ,5, 3117, item);
//			//mqcount = requestdata_485_ZB(ParaAll->f10.para_mp[pindex-1].MPNo, (INT8U*)S485_2_REV_GUI_MQ, arr_did, 1, item);
//		if(mqcount!=0)
//			show_realdata(pindex, item,mqcount);
//	}
//	else if((pindex==0&&ret!=0) && ret != CAN)
//		msgbox_label((char*)"无此测量点", CTRL_BUTTON_OK);
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
				fprintf(stderr,"i=%d, oi=%04x, size=%d\n",i,event_class_len[i].oi,classlen);
				saveflg = 0;
				saveflg = readCoverClass(event_class_len[i].oi,0,(INT8U *)eventbuff,classlen,event_para_save);
				fprintf(stderr,"saveflg=%d oi=%04x\n",saveflg,event_class_len[i].oi);
//				int		j=0;
//				INT8U	val;
//				for(j=0;j<classlen;j++) {
//					val = (INT8U )eventbuff[j];
//					fprintf(stderr,"%02x ",val);
//				}
//				fprintf(stderr,"\n");
				if(saveflg) {
					memcpy(&class7,eventbuff,sizeof(Class7_Object));
					fprintf(stderr,"修改前：i=%d,oi=%x,class7.crrentnum=%d\n",i,event_class_len[i].oi,class7.crrentnum);
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
	default :
		break;
	}
}
//初始化集中器数据

void menu_initjzqdata(){
	jzq_reset(DATA_INIT);
}
#ifdef HUBEI
void menu_initjzqparaf3()//恢复出厂设置
{
	jzq_reset(F3_PARA_INIT);
}
#endif
//初始化集中器参数

void menu_initjzqevent(){
	jzq_reset(EVENT_INIT);
}

void menu_initjzqdemand(){
	jzq_reset(DEMAND_INIT);
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
	fprintf(stderr,"\nlunxun_flg=");
	for(i=0; i<4; i++){
		fprintf(stderr, " %02x", lunxun_flg[i]);
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
	set_time_show_flag(1);
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
			fprintf(stderr, "\n cur_form->pfun_process_ret=%d",cur_form->pfun_process_ret);
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
			set_time_show_flag(1);
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
	set_time_show_flag(1);
	return 0;
}
/*
 * 以太网通信方式更改
 * */
void menu_set_nettx()
{
	CLASS26 Class26;
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
	memset(&Class26,0,sizeof(CLASS26));
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
    set_time_show_flag(1);
	Point pos;

	gui_setpos(&pos, rect_Client.left+15*FONTSIZE, rect_Client.top);
	pos.y += FONTSIZE*3 + ROW_INTERVAL;

	readCoverClass(0x4510, 0, (void*)&Class26, sizeof(CLASS26), para_vari_save);
	memset(str, 0, INPUTKEYNUM);
	sprintf(str, "%02d", Class26.commconfig.timeoutRtry&0x03);//重拨次数
	edit_init(&edit_resendnum, str, 2, pos,NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
//	dbg_prt("\n para_f8.Interval_Replay=%d",para_f8.Interval_Replay);
	sprintf(str, "%02d", (Class26.commconfig.timeoutRtry>>2));//超时时间
	edit_init(&edit_redailstep, str, 2, pos,NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	sprintf(str, "%05d", Class26.commconfig.heartBeat);//心跳周期
	edit_init(&edit_heartbeat, str, 5, pos,NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);
	//------------------------------------------
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "TCP", strlen("TCP"));
	memcpy(cb_text[1], "UDP", strlen("UDP"));
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	tmp = Class26.commconfig.connectType;
	combox_init(&cb_conntype, tmp, cb_text, pos, NORETFLG,client.node.child);
	//--------------------------------------------
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "混合模式", strlen("混合模式"));
	memcpy(cb_text[1], "客户模式", strlen("客户模式"));
	memcpy(cb_text[2], "服务模式", strlen("服务模式"));
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	tmp = Class26.commconfig.workModel;
	combox_init(&cb_worktype, tmp, cb_text, pos, NORETFLG,client.node.child);
	//--------------------------------------------
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "主备模式", strlen("主备模式"));
	memcpy(cb_text[1], "多连接模式", strlen("多连接模式"));
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	tmp = Class26.commconfig.appConnectType;
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
			fprintf(stderr,"\n get_oprmode()=%d", get_oprmode());
			if(get_oprmode()==OPRMODE_MODIFY){
				cur_form->pfun_process(cur_form);
				if(cur_form->pfun_process_ret==OK){
					//cur_node = list_getfirst(cur_node);
					if(get_oprmode()==OPRMODE_MODIFY && msgbox_label((char *)"保存参数?", CTRL_BUTTON_OK)==ACK){
						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_resendnum, str);
						trynum = atoi(str);
						fprintf(stderr,"\n------retrystr = %s,trynum = %d\n",str,trynum);
						if(trynum>3){
							trynum = 3;//重发次数最大为3
						}
						Class26.commconfig.timeoutRtry &= 0xF6;
						Class26.commconfig.timeoutRtry |= trynum;//重发次数
						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_redailstep, str);
						interval_replay = atoi(str);
						if(interval_replay>0x3F){
							interval_replay = 0x3F;//超时时间最大为63s
						}
						Class26.commconfig.timeoutRtry |= (interval_replay<<2);
						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_heartbeat, str);
						interval_replay = atoi(str);
						Class26.commconfig.heartBeat = atoi(str);
						if(Class26.commconfig.heartBeat > 65535)//最大心跳周期65535秒
							Class26.commconfig.heartBeat = 65535;//TODO:心跳周期应设置成INT16U

						Class26.commconfig.connectType = cb_conntype.cur_index;
						Class26.commconfig.workModel = cb_worktype.cur_index;

						Class26.commconfig.appConnectType = cb_appcontype.cur_index;

						saveCoverClass(0x4510, 0, (void*)&Class26, sizeof(CLASS26), para_vari_save);
//TODO:给cjcomm发消息
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
            set_time_show_flag(1);
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
	CLASS25 Class25;
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
	memset(&Class25,0,sizeof(CLASS25));
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
    set_time_show_flag(1);
	Point pos;

	gui_setpos(&pos, rect_Client.left+15*FONTSIZE, rect_Client.top);
	pos.y += FONTSIZE*3 + ROW_INTERVAL;

	readCoverClass(0x4500, 0, (void*)&Class25, sizeof(CLASS25), para_vari_save);
	memset(str, 0, INPUTKEYNUM);
	sprintf(str, "%02d", Class25.commconfig.timeoutRtry&0x03);//重拨次数
	edit_init(&edit_resendnum, str, 2, pos,NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
//	dbg_prt("\n para_f8.Interval_Replay=%d",para_f8.Interval_Replay);
	sprintf(str, "%02d", (Class25.commconfig.timeoutRtry>>2));//超时时间
	edit_init(&edit_redailstep, str, 2, pos,NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	sprintf(str, "%05d", Class25.commconfig.heartBeat);//心跳周期
	edit_init(&edit_heartbeat, str, 5, pos,NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);
	//------------------------------------------
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "永久在线", strlen("永久在线"));
	memcpy(cb_text[1], "被动激活", strlen("被动激活"));
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
//	tmp=para_f8.WorkMode&0x03;
	tmp = Class25.commconfig.onlineType;
	if(tmp<0)
		tmp = 0;
	combox_init(&cb_onlinetype, tmp, cb_text, pos, NORETFLG,client.node.child);
	//------------------------------------------
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "TCP", strlen("TCP"));
	memcpy(cb_text[1], "UDP", strlen("UDP"));
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	tmp = Class25.commconfig.connectType;
	combox_init(&cb_conntype, tmp, cb_text, pos, NORETFLG,client.node.child);
	//--------------------------------------------
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "混合模式", strlen("混合模式"));
	memcpy(cb_text[1], "客户模式", strlen("客户模式"));
	memcpy(cb_text[2], "服务模式", strlen("服务模式"));
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	tmp = Class25.commconfig.workModel;
	combox_init(&cb_worktype, tmp, cb_text, pos, NORETFLG,client.node.child);
	//--------------------------------------------
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "主备模式", strlen("主备模式"));
	memcpy(cb_text[1], "多连接模式", strlen("多连接模式"));
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	tmp = Class25.commconfig.appConnectType;
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
			fprintf(stderr,"\n get_oprmode()=%d", get_oprmode());
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
						Class25.commconfig.timeoutRtry &= 0xF6;
						Class25.commconfig.timeoutRtry |= trynum;//重发次数
						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_redailstep, str);
						interval_replay = atoi(str);
						if(interval_replay>0x3F){
							interval_replay = 0x3F;//超时时间最大为63s
						}
						Class25.commconfig.timeoutRtry |= (interval_replay<<2);

						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_heartbeat, str);
						interval_replay = atoi(str);
						Class25.commconfig.heartBeat = atoi(str);
						if(Class25.commconfig.heartBeat > 65535)//最大心跳周期65535秒
							Class25.commconfig.heartBeat = 65535;//TODO:心跳周期应设置成INT16U

						Class25.commconfig.onlineType = cb_onlinetype.cur_index;

						Class25.commconfig.connectType = cb_conntype.cur_index;
						Class25.commconfig.workModel = cb_worktype.cur_index;

						Class25.commconfig.appConnectType = cb_appcontype.cur_index;

						saveCoverClass(0x4500, 0, (void*)&Class25, sizeof(CLASS25), para_vari_save);
//TODO:给cjcomm发消息
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
            set_time_show_flag(1);
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
	fprintf(stderr,"textTemp = %s\n",textTemp);
	sscanf(textTemp,"%s %s %s %s",ip1,ip2,ip3,ip4);
	fprintf(stderr,"ip1 = %s,ip2 = %s,ip3 = %s,ip4 = %s\n",ip1,ip2,ip3,ip4);
	sprintf(text,"%d.%d.%d.%d",atoi(ip1),atoi(ip2),atoi(ip3),atoi(ip4));
	fprintf(stderr,"text = %s",text);

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
	set_time_show_flag(1);
	return 0;
}

void menu_netmaster()
{
	int i;
	int tmp=0;
	CLASS26 Class26;
	char first_flg=0;
	struct list *cur_node=NULL, *tmpnode=NULL;
	Form *cur_form=NULL, client;//client 液晶显示客户区
	char str[INPUTKEYNUM];
	char ip_chg[20];
	int foo[4];
	Rect rect;
	Edit edit_masterip,edit_masterport,edit_salveip,edit_salveport;
	memset(&Class26,0,sizeof(CLASS26));
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
    set_time_show_flag(1);//显示标志置位
	Point pos;
	gui_setpos(&pos, rect_Client.left+9.5*FONTSIZE, rect_Client.top);

	pos.y += FONTSIZE*5 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	readCoverClass(0x4500, 0, (void*)&Class26, sizeof(CLASS26), para_vari_save);
	sprintf(ip_chg,"%d.%d.%d.%d",Class26.master.master[0].ip[1],Class26.master.master[0].ip[2],
	                             Class26.master.master[0].ip[3],Class26.master.master[0].ip[4]);
	paraip_gettext(ip_chg, str);
	editip_init(&edit_masterip, CTRL_EDITIP, str, 12, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//主IP
	//------------------------------------------dbg_prt("\n edit_cldno.node=%x prev=%x next=%x",(int)&edit_cldno.node, (int)edit_cldno.node.prev,(int)edit_cldno.node.next);

	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	sprintf(str, "%05d", Class26.master.master[0].port);
	edit_init(&edit_masterport, str, 5, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//主端口
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	memset(ip_chg,0,sizeof(ip_chg));
	sprintf(ip_chg,"%d.%d.%d.%d",Class26.master.master[1].ip[1],Class26.master.master[1].ip[2],
	                             Class26.master.master[1].ip[3],Class26.master.master[1].ip[4]);
	paraip_gettext(ip_chg, str);
	editip_init(&edit_salveip, CTRL_EDITIP, str, 12, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//备IP
	memset(ip_chg,0,sizeof(ip_chg));
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	sprintf(str, "%05d", Class26.master.master[1].port);
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
							Class26.master.master[0].ip[i+1] = foo[i];
						}
//						sscanf(ip_chg,"%d.%d.%d.%d",(int*)&Class26.master.master[0].ip[1],(int*)&Class26.master.master[0].ip[2],
//						                            (int*)&Class26.master.master[0].ip[3],(int*)&Class26.master.master[0].ip[4]);
						memset(str, 0, INPUTKEYNUM);
						memset(ip_chg,0,sizeof(ip_chg));
						eidt_gettext(&edit_masterport, str);
						Class26.master.master[0].port = atoi(str);
						eidtip16_gettext(&edit_salveip, ip_chg);
						memset(foo,0,4);
						sscanf(ip_chg,"%d.%d.%d.%d",&foo[0],&foo[1],&foo[2],&foo[3]);
						for(i = 0;i<4;i++)
						{
							if(foo[i]>255)
							{
								foo[i] = 255;
							}
							Class26.master.master[1].ip[i+1] = foo[i];
						}
//						sscanf(ip_chg,"%d.%d.%d.%d",(int*)&Class26.master.master[1].ip[1],(int*)&Class26.master.master[1].ip[2],
//													(int*)&Class26.master.master[1].ip[3],(int*)&Class26.master.master[1].ip[4]);
						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_salveport, str);
						Class26.master.master[1].port = atoi(str);
//TODO:将以太网主站通讯参数写入文件，并且给cjcomm发送消息
						saveCoverClass(0x4500, 0, (void*)&Class26, sizeof(CLASS26), para_vari_save);
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
            set_time_show_flag(1);//置显示标志
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
	CLASS25 Class25;
	char first_flg=0;
	struct list *cur_node=NULL, *tmpnode=NULL;
	Form *cur_form=NULL, client;//client 液晶显示客户区
	char str[INPUTKEYNUM];
	char ip_chg[20];
	int foo[4];
	Rect rect;
	Edit edit_masterip,edit_masterport,edit_salveip,edit_salveport,edit_apn;
	memset(&Class25,0,sizeof(CLASS25));
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
    set_time_show_flag(1);//显示标志置位
	Point pos;
	gui_setpos(&pos, rect_Client.left+9.5*FONTSIZE, rect_Client.top);

	pos.y += FONTSIZE*5 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);

	readCoverClass(0x4500, 0, (void*)&Class25, sizeof(CLASS25), para_vari_save);
	sprintf(ip_chg,"%d.%d.%d.%d",Class25.master.master[0].ip[1],Class25.master.master[0].ip[2],
	                             Class25.master.master[0].ip[3],Class25.master.master[0].ip[4]);
	paraip_gettext(ip_chg, str);
	editip_init(&edit_masterip, CTRL_EDITIP, str, 12, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//主IP
	//------------------------------------------dbg_prt("\n edit_cldno.node=%x prev=%x next=%x",(int)&edit_cldno.node, (int)edit_cldno.node.prev,(int)edit_cldno.node.next);

	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	sprintf(str, "%05d", Class25.master.master[0].port);
	edit_init(&edit_masterport, str, 5, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//主端口
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	memset(ip_chg,0,sizeof(ip_chg));
	sprintf(ip_chg,"%d.%d.%d.%d",Class25.master.master[1].ip[1],Class25.master.master[1].ip[2],
	                             Class25.master.master[1].ip[3],Class25.master.master[1].ip[4]);
	paraip_gettext(ip_chg, str);
	editip_init(&edit_salveip, CTRL_EDITIP, str, 12, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//备IP
	memset(ip_chg,0,sizeof(ip_chg));
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	sprintf(str, "%05d", Class25.master.master[1].port);
	edit_init(&edit_salveport, str, 5, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//备端口
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, ' ', INPUTKEYNUM);
//	if(strlen((char*)ParaAll->f3.APN)!=0)
		strcpy(str,(char*) &Class25.commconfig.apn[1]);
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
							Class25.master.master[0].ip[i+1] = foo[i];
						}
//						sscanf(ip_chg,"%d.%d.%d.%d",(int*)&Class25.master.master[0].ip[1],(int*)&Class25.master.master[0].ip[2],
//								(int*)&Class25.master.master[0].ip[3],(int*)&Class25.master.master[0].ip[4]);
						memset(str, 0, INPUTKEYNUM);
						memset(ip_chg,0,sizeof(ip_chg));
						eidt_gettext(&edit_masterport, str);
						Class25.master.master[0].port = atoi(str);
						eidtip16_gettext(&edit_salveip, ip_chg);
						memset(foo,0,4);
						sscanf(ip_chg,"%d.%d.%d.%d",&foo[0],&foo[1],&foo[2],&foo[3]);
						for(i = 0;i<4;i++)
						{
							if(foo[i]>255)
							{
								foo[i] = 255;
							}
							Class25.master.master[1].ip[i+1] = foo[i];
						}
//						sscanf(ip_chg,"%d.%d.%d.%d",(int*)&Class25.master.master[1].ip[1],(int*)&Class25.master.master[1].ip[2],
//													(int*)&Class25.master.master[1].ip[3],(int*)&Class25.master.master[1].ip[4]);
						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_salveport, str);
						Class25.master.master[1].port = atoi(str);
						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_apn, str);
						strcpy((char*)&Class25.commconfig.apn[1], str);
						Class25.commconfig.apn[0] = (INT8U)(strlen(str));
//						fprintf(stderr,"\nthe len of apn %s is %d\n",&Class25.commconfig.apn[1],Class25.commconfig.apn[0]);
//TODO:将GPRS主站通讯参数写入文件，并且给cjcomm发送消息
						write_apn((char*)&Class25.commconfig.apn[1]);
						saveCoverClass(0x4500, 0, (void*)&Class25, sizeof(CLASS25), para_vari_save);
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
            set_time_show_flag(1);//置显示标志
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
	CLASS26 Class26;
	char first_flg=0;
	struct list *cur_node=NULL, *tmpnode=NULL;
	Form *cur_form=NULL, client;//client 液晶显示客户区
	char str[INPUTKEYNUM];
	Rect rect;
	Edit edit_termip, edit_netmask, edit_gateway, edit_listenport,edit_pppoe_name,edit_pppoe_pwd;
	Combox cb_IP_config_mode;
	memset(&Class26,0,sizeof(CLASS26));
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
    set_time_show_flag(1);
	Point pos;
	readCoverClass(0x4510, 0, (void*)&Class26, sizeof(CLASS26), para_vari_save);

	gui_setpos(&pos, rect_Client.left+10.5*FONTSIZE, rect_Client.top);
	//IP配置、终端IP、子网掩码、网关地址、侦听端口、PPPoE用户名、PPPoE密码
	//------------------------------------------
	pos.y += FONTSIZE*3 + ROW_INTERVAL;
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "DHCP", strlen("DHCP"));
	memcpy(cb_text[1], "静态", strlen("静态"));
	memcpy(cb_text[2], "PPPoE", strlen("PPPoE"));
	tmp = Class26.IP.ipConfigType;//DHCP：0，静态：1,PPPoE：2
	if(tmp<0)
		tmp = 1;
	combox_init(&cb_IP_config_mode, tmp, cb_text, pos, NORETFLG,client.node.child);
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	ip2asc(&Class26.IP.ip[1], str);
	editip_init(&edit_termip, CTRL_EDITIP, str, 12, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//主IP
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	ip2asc(&Class26.IP.subnet_mask[1], str);
	editip_init(&edit_netmask, CTRL_EDITIP, str, 12, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//掩码
	//-----------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	ip2asc(&Class26.IP.gateway[1], str);
	editip_init(&edit_gateway, CTRL_EDITIP, str, 12, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//网关
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	sprintf(str, "%05d", Class26.commconfig.listenPort[0]);
	edit_init(&edit_listenport, str, 5, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//监听端口列表1
	//-------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	memcpy(str,&Class26.IP.username_pppoe[1],Class26.IP.username_pppoe[0]);
	edit_init(&edit_pppoe_name, str, 15, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//PPPoE用户名
	//-------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	memcpy(str,&Class26.IP.password_pppoe[1],Class26.IP.password_pppoe[0]);
	edit_init(&edit_pppoe_pwd, str, 15, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//PPPoE密码
	//-------------------------------------------
	cur_node = &cb_IP_config_mode.form.node;
	cur_form = &edit_termip.form;
	seteth0_showlabel();//显示各个控件的标签
	set_time_show_flag(1);
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

						Class26.IP.ipConfigType = cb_IP_config_mode.cur_index;

						memset(sip, 0, 16);
						eidtip12_gettext(&edit_termip, (char*)sip);//取得编辑框中的ip值
						if(ip_strtobyte(sip, &Class26.IP.ip[1])==0)
							input_valid = 0;
						else{
							memset(cmd, 0, 50);
							sprintf(cmd, "ifconfig eth0 %d.%d.%d.%d up", Class26.IP.ip[1],Class26.IP.ip[2],
									Class26.IP.ip[3],Class26.IP.ip[4]);
							system(cmd);
							memset(cmd1, 0, 100);
							sprintf(cmd1, "echo %s > /nor/rc.d/ip.sh", cmd);
							system(cmd1);
						}

						memset(sip, 0, 16);
						eidtip12_gettext(&edit_netmask, (char*)sip);
						if(ip_strtobyte(sip, &Class26.IP.subnet_mask[1])==0)
							input_valid = 0;

						memset(sip, 0, 16);
						eidtip12_gettext(&edit_gateway, (char*)sip);
						if(ip_strtobyte(sip, &Class26.IP.gateway[1])==0)
							input_valid = 0;

						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_listenport, str);
						Class26.commconfig.listenPort[0] = atoi(str);

						memset(str,0,INPUTKEYNUM);
						eidt_gettext(&edit_pppoe_name, str);
						memcpy(&Class26.IP.username_pppoe[1],str,strlen(str));
						Class26.IP.username_pppoe[0] = strlen(str);

						memset(str,0,INPUTKEYNUM);
						eidt_gettext(&edit_pppoe_pwd,str);
						memcpy(&Class26.IP.password_pppoe[1],str,strlen(str));
						Class26.IP.password_pppoe[0] = strlen(str);

						if(input_valid==1)
							saveCoverClass(0x4510, 0, (void*)&Class26, sizeof(CLASS26), para_vari_save);
						//TODO:给cjcomm发消息
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
            set_time_show_flag(1);
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
	set_time_show_flag(1);
	return 0;
}

void menu_jzqtelephone()
{
	int tmp=0;
	CLASS25 Class25;
	char first_flg=0;
	struct list *cur_node=NULL, *tmpnode=NULL;
	Form *cur_form=NULL, client;//client 液晶显示客户区
	char str[INPUTKEYNUM];
	Rect rect;
	Edit edit_phonenum, edit_msgcenter, edit_user, edit_mima;
	memset(&Class25,0,sizeof(CLASS25));
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
    set_time_show_flag(1);
	Point pos;
	pos.x = rect_Client.left+FONTSIZE*9.5;
	pos.y = rect_Client.top;

	pos.y += FONTSIZE*5 + ROW_INTERVAL;

	readCoverClass(0x4500, 0, (void*)&Class25, sizeof(CLASS25), para_vari_save);
	memset(str, 0, INPUTKEYNUM);
	memcpy(str,Class25.sms.master[0],VISIBLE_STRING_LEN);//主站号码

	edit_init(&edit_phonenum, str, 16, pos,NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	memcpy(str,Class25.sms.center,VISIBLE_STRING_LEN);//短信中心号码

	edit_init(&edit_msgcenter, str, 16, pos,NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);
	//-------------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, ' ', INPUTKEYNUM);
	//if(para_f7.Len_UsrName==0){
	if(Class25.commconfig.userName[0]==0){
		memcpy(str, "USERNAME", strlen("USERNAME"));
	}else
		memcpy(str,&Class25.commconfig.userName[1],VISIBLE_STRING_LEN);
	edit_init(&edit_user, str, 16, pos,NOFRAME, NORETFLG, client.node.child,KEYBOARD_ASC);
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, ' ', INPUTKEYNUM);
	//if(para_f7.Len_Pwd==0){
	if(Class25.commconfig.passWord[0]==0){
		memcpy(str, "000000", strlen("000000"));
	}else
		memcpy(str,&Class25.commconfig.passWord[1],VISIBLE_STRING_LEN);
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
						memcpy(Class25.sms.master[0],str,strlen(str));

						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_msgcenter, str);
						memcpy(Class25.sms.center,str,strlen(str));

						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_user, str);
						memcpy(&Class25.commconfig.userName[1],str,strlen(str));
						Class25.commconfig.userName[0] = strlen(str);

						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_mima, str);
						memcpy(&Class25.commconfig.passWord[1],str,strlen(str));
						Class25.commconfig.passWord[0] = strlen(str);
						write_userpwd(&Class25.commconfig.userName[1],&Class25.commconfig.passWord[1],
								      &Class25.commconfig.apn[1]);
						saveCoverClass(0x4500, 0, (void*)&Class25, sizeof(CLASS25), para_vari_save);
//TODO:给cjcomm发消息
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
            set_time_show_flag(1);
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
	set_time_show_flag(1);
	return 0;
}

void jzq_id_edit()
{
	int i = 0;
	CLASS_4001_4002_4003 Class_id;
	INT8U str[50];
	INT8U sever_addr[20];
	INT8U addr_len = 0;
	char s_jzqdizhi10[30],oprmode_old=0;
	memset(&Class_id,0,sizeof(CLASS_4001_4002_4003));
	memset(sever_addr,0,sizeof(sever_addr));
	memset(s_jzqdizhi10, 0, sizeof(s_jzqdizhi10));
	readCoverClass(0x4001, 0, (void*)&Class_id, sizeof(CLASS_4001_4002_4003), para_vari_save);
	memset(str,0,sizeof(str));

	bcd2str(&Class_id.curstom_num[1],(INT8U*)str,Class_id.curstom_num[0],sizeof(str),positive);

	sscanf((char*)str,"%[0-9]",s_jzqdizhi10);

	oprmode_old = get_oprmode();
	set_oprmode(OPRMODE_MODIFY);
#ifdef JIANGSU
	if(msgbox_jzqaddr_js(s_jzqdizhi10, KEYBOARD_DEC)==ACK){
#else
	if(msgbox_jzqaddr_10(s_jzqdizhi10, 16)==ACK){
#endif
		memset(str, 0, sizeof(str));
		sscanf(s_jzqdizhi10,"%*[^0-9]%[0-9]",str);

		addr_len = strlen((char*)str);

		if(addr_len==0){
			str2bcd(str,sever_addr,sizeof(sever_addr));
			memcpy(&Class_id.curstom_num[1],sever_addr,strlen((char*)sever_addr));
			if(addr_len%2)
			{
				Class_id.curstom_num[addr_len/2+1] |= 0x0F;
				Class_id.curstom_num[0] = addr_len/2+1;
			}
			else{
				Class_id.curstom_num[0] = addr_len/2;
			}

			saveCoverClass(0x4001, 0, (void*)&Class_id, sizeof(CLASS_4001_4002_4003), para_vari_save);
			msgbox_label((char *)"设置成功！", CTRL_BUTTON_OK);
		}else
			msgbox_label((char *)"设置失败！", CTRL_BUTTON_OK);
	}
	set_oprmode(oprmode_old);
}

void show_jzq_ver()
{
    char	str[128];
    Point pos;
    CLASS19 oi4300;

    memset(str,0,sizeof(str));
    memset(&oi4300,0,sizeof(CLASS19));
    readCoverClass(0x4300,0,&oi4300,sizeof(CLASS19),para_vari_save);
    gui_clrrect(rect_Client);
    gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+FONTSIZE);
    gui_textshow((char*)"终端信息", pos, LCD_NOREV);

    pos.x = rect_Client.left + FONTSIZE*5;
    pos.y += FONTSIZE*3;
    memcpy(str,&oi4300.name[1],oi4300.name[0]);
    sprintf(str,"厂商代码:%c%c%c%c",oi4300.info.factoryCode[0],oi4300.info.factoryCode[1],
    								oi4300.info.factoryCode[2],oi4300.info.factoryCode[3]);
    gui_textshow(str, pos, LCD_NOREV);

    memset(str,0,sizeof(str));
    pos.y += FONTSIZE*2+2;
    sprintf(str,"软件版本:%c%c%c%c",oi4300.info.softVer[0],oi4300.info.softVer[1],
    								oi4300.info.softVer[2],oi4300.info.softVer[3]);
    gui_textshow(str, pos, LCD_NOREV);

    memset(str,0,sizeof(str));
    pos.y += FONTSIZE*2+2;
    sprintf(str,"软件版本日期:%c%c%c%c%c%c",oi4300.info.softDate[0],oi4300.info.softDate[1],oi4300.info.softDate[2],
    									   oi4300.info.softDate[3],oi4300.info.softDate[4],oi4300.info.softDate[5]);
    gui_textshow(str, pos, LCD_NOREV);

    memset(str,0,sizeof(str));
    pos.y += FONTSIZE*2+2;
    sprintf(str,"硬件版本:%c%c%c%c",oi4300.info.hardVer[0],oi4300.info.hardVer[1],
    								oi4300.info.hardVer[2],oi4300.info.hardVer[3]);
    gui_textshow(str, pos, LCD_NOREV);

    memset(str,0,sizeof(str));
    pos.y += FONTSIZE*2+2;
    sprintf(str,"硬件版本日期:%c%c%c%c%c%c",oi4300.info.hardDate[0],oi4300.info.hardDate[1],oi4300.info.hardDate[2],
    										oi4300.info.hardDate[3],oi4300.info.hardDate[4],oi4300.info.hardDate[5]);
    gui_textshow(str, pos, LCD_NOREV);

    memset(str,0,sizeof(str));
    pos.y += FONTSIZE*2+2;
    sprintf(str,"生产日期:%d-%d-%d %d:%d:%d",oi4300.date_Product.year.data,oi4300.date_Product.month.data,oi4300.date_Product.day.data,
    										oi4300.date_Product.hour.data,oi4300.date_Product.min.data,oi4300.date_Product.sec.data);
    gui_textshow(str, pos, LCD_NOREV);

    set_time_show_flag(1);
}

void menu_jzqstatus_showused(char* name, int used, Point pos){
	char str[50];
	memset(str, 0, 50);
	sprintf(str, "%s:%d%%", name,used);
	gui_textshow(str, pos, LCD_NOREV);
	set_time_show_flag(1);
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
    set_time_show_flag(1);
	return;
}

void show_jzq_ip(){
	char ip[4];
    char tmpstr[128];
    char str[16];
    memset(str,0,16);
    Point pos;
    gui_clrrect(rect_Client);
    gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+ROW_INTERVAL );
#ifdef HUBEI
    gui_textshow((char*)"主站IP", pos, LCD_NOREV);
	memset(ip, 0, 4);
	getlocalip(ip);
	gui_setpos(&pos, rect_Client.left+FONTSIZE, rect_Client.top+2*FONTSIZE + 2*ROW_INTERVAL);
	sprintf((char *)tmpstr,"IP :%s",(char *)ParaAll->f3.IP_MS);
#else
	gui_textshow((char*)"本地IP", pos, LCD_NOREV);
	memset(ip, 0, 4);
	getlocalip(ip);
	gui_setpos(&pos, rect_Client.left+FONTSIZE, rect_Client.top+2*FONTSIZE + 2*ROW_INTERVAL);
	sprintf((char *)tmpstr,"IP : %03d.%03d.%03d.%03d",ip[0],ip[1],ip[2],ip[3]);
#endif
	gui_textshow((char*)tmpstr, pos, LCD_NOREV);
	//MAC地址
	FILE *fpmac = NULL;
	INT8U TempBuf[60];
	char	str1[16],str2[16],str3[16],str4[16];
	char	mac[16];
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
#ifdef HUBEI
	memset(TempBuf, 0, 60);
	memset(tmpstr,0,sizeof(tmpstr));
	gui_setpos(&pos, rect_Client.left+FONTSIZE, rect_Client.top+6*FONTSIZE + 4*ROW_INTERVAL);
	if(strlen((char*)ParaAll->f3.APN)!=0)
			memcpy(TempBuf, ParaAll->f3.APN, strlen((char*)ParaAll->f3.APN));
		else
			memcpy(TempBuf, "CMNET", strlen("CMNET"));
	sprintf((char *)tmpstr,"APN: %s",TempBuf);
	gui_textshow((char*)tmpstr, pos, LCD_NOREV);
	memset(tmpstr,0,sizeof(tmpstr));
	gui_setpos(&pos, rect_Client.left+FONTSIZE, rect_Client.top+8*FONTSIZE + 5*ROW_INTERVAL);
	sprintf((char *)tmpstr,"端口:%d",ParaAll->f3.Port_MS);
	gui_textshow((char*)tmpstr, pos, LCD_NOREV);
	memset(tmpstr,0,sizeof(tmpstr));
	gui_setpos(&pos, rect_Client.left+FONTSIZE, rect_Client.top+10*FONTSIZE + 6*ROW_INTERVAL);
	sprintf((char *)tmpstr,"心跳周期(分钟):%d",ParaAll->f1.HeartInterval);
	gui_textshow((char*)tmpstr, pos, LCD_NOREV);
	memset(TempBuf, 0, 60);
	memset(tmpstr,0,sizeof(tmpstr));
	gui_setpos(&pos, rect_Client.left+FONTSIZE, rect_Client.top+12*FONTSIZE + 7*ROW_INTERVAL);
	if(strlen((char*)ParaAll->f16.UsrName_Virtual)!=0)
		memcpy(TempBuf, ParaAll->f16.UsrName_Virtual, strlen((char*)ParaAll->f16.UsrName_Virtual));
	else
		memcpy(TempBuf, "username", strlen("username"));
	sprintf((char *)tmpstr,"虚拟专网:%s",TempBuf);
	gui_textshow((char*)tmpstr, pos, LCD_NOREV);
	memset(TempBuf, 0, 60);
	memset(tmpstr,0,sizeof(tmpstr));
	gui_setpos(&pos, rect_Client.left+FONTSIZE, rect_Client.top+14*FONTSIZE + 8*ROW_INTERVAL);
	if(strlen((char*)ParaAll->f16.Pwd_Virtual)!=0)
		memcpy(TempBuf, ParaAll->f16.Pwd_Virtual, strlen((char*)ParaAll->f16.Pwd_Virtual));
	else
		memcpy(TempBuf, "passwd", strlen("passwd"));
	sprintf((char *)tmpstr,"密码:%s",TempBuf);
	gui_textshow((char*)tmpstr, pos, LCD_NOREV);
#endif
    set_time_show_flag(1);
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
#ifdef JIANGSU
		case 1:
			show_jzq_ip();
			break;
		case 2:
			show_jzqused();
			break;
#else
		case 1:
			show_jzqused();
			break;
		case 2:
			show_jzq_ip();
			break;
#endif

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
	get_inet_ip(ETH,ip);
	sscanf(ip,"%d.%d.%d.%d",&iIP[0],&iIP[1],&iIP[2],&iIP[3]);
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
	}
}

#define REQUEST_GPRSIP 1

void menu_gprsip(){
	CLASS25 Class25;
	char s_gprsip[20];
	Point pos;
	memset(&Class25,0,sizeof(CLASS25));
	memset(s_gprsip, 0, 20);
	readCoverClass(0x4500, 0, (void*)&Class25, sizeof(CLASS25), para_vari_save);
	gui_clrrect(rect_Client);
	gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+8*FONTSIZE);
	memset(s_gprsip, 0, 20);
	sprintf(s_gprsip, "GPRS IP:");
	gui_textshow(s_gprsip, pos, LCD_NOREV);
	memset(s_gprsip, 0, 20);
	sprintf(s_gprsip,"%d.%d.%d.%d",Class25.pppip[1],Class25.pppip[2],Class25.pppip[3],Class25.pppip[4]);
	pos.x = rect_Client.left + FONTSIZE*4;
	pos.y += FONTSIZE*3;
	gui_textshow(s_gprsip, pos, LCD_NOREV);
	set_time_show_flag(1);
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
//	char str[100];
//	memset(str, 0, 100);
//	sprintf(str, "时钟电池:%2.1fV", shmm_getdevstat()->Clock_Batt_Voltage);
//	msgbox_label((char*)str, CTRL_BUTTON_OK);
//	return;
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
//			set_time_show_flag(1);
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
//          set_time_show_flag(1);
//		}
//		PressKey = NOKEY;
//	}
//	return;
}

void ZB_Manage(int cmd){
//	mqd_t mqd;
//	INT8U sendBuf[2];
//	memset(sendBuf, 0, 2);
//	sendBuf[0] = cmd;
//	mqd =  createMsg((INT8S*)PLC_REV_GUI_MQ, O_WRONLY);
//	if(mqd<0)
//		return;
//	if(sendMsg(mqd, CTRLCMD, (INT8S*)sendBuf, 1)<0){
//		colseMsg(mqd);
//		dbg_prt( "\n PLC_REV_GUI_MQ:mq_open_ret=%d  error!!!",mqd);
//		return;
//	}
//	colseMsg(mqd);
}
void menu_zb_begin(){
	int ret;
	ret = msgbox_label((char*)"重启载波抄表?", CTRL_BUTTON_OK);
	if(ret==ACK){
		fprintf(stderr, "\n 液晶重启载波抄表");
		ZB_Manage(3);
	}
}
void menu_zb_stop(){
	int ret;
	ret = msgbox_label((char*)"暂停载波抄表?", CTRL_BUTTON_OK);
	if(ret==ACK){
		fprintf(stderr, "\n 液晶暂停载波抄表");
		ZB_Manage(1);
	}
}
void menu_zb_resume(){
	int ret;
	ret = msgbox_label((char*)"恢复载波抄表?", CTRL_BUTTON_OK);
	if(ret==ACK){
		fprintf(stderr, "\n 液晶恢复载波抄表");
		ZB_Manage(2);
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
#ifdef SHANGHAI
			memcpy(cb_text[0], "2400", strlen("2400"));
			memcpy(cb_text[1], "1200", strlen("1200"));
#else
			memcpy(cb_text[0], "1200", strlen("1200"));
//			memcpy(cb_text[1], "2400", strlen("2400"));

#endif
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
        fprintf(stderr,"设置成功");
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
    	fprintf(stderr,"设置成功");
    }
    return;
}
void menu_zb_info(){
//    char	str[50];
//    Point pos;
//    gui_clrrect(rect_Client);
//    fprintf(stderr, "\n ------载波模块信息-----------");
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
//    set_time_show_flag(1);
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
////    fprintf(stderr, "\n ------GPRS模块信息-----------");
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
//    set_time_show_flag(1);
//	PressKey = NOKEY;
//	while(g_LcdPoll_Flag==LCD_NOTPOLL){
//		delay(300);
//		if(PressKey==ESC)
//			return;
//		PressKey = NOKEY;
//	}
}
#ifdef HUBEI
void menu_Virpara()
{
	int tmp=0;
	para_F16 para_f16;
	char first_flg=0;
	struct list *cur_node=NULL, *tmpnode=NULL;
	Form *cur_form=NULL, client;//client 液晶显示客户区
	Point pos,lab_pos;
	char str[INPUTKEYNUM];
	Rect rect;
	Edit deit_UsrName_Virtual,edit_Pwd_Virtual;
	memset(&deit_UsrName_Virtual, 0, sizeof(Edit));
	memset(&edit_Pwd_Virtual, 0, sizeof(Edit));
	memset(&client, 0, sizeof(Form));
	client.node.child = (struct list*)malloc(sizeof(struct list));
	if(client.node.child==NULL){
		g_curcldno = 1;//上状态栏显示
		return;
	}
	memset(client.node.child, 0, sizeof(struct list));
	gui_clrrect(rect_Client);
	gui_setpos(&pos, rect_Client.left+FONTSIZE, rect_Client.top+FONTSIZE*2+ROW_INTERVAL);
	gui_textshow((char *)"用户名:", pos, LCD_NOREV);

	//------------------------------------------
	pos.y = rect_Client.top + FONTSIZE*4 + ROW_INTERVAL*2;
	memset(str, ' ', INPUTKEYNUM);
	if(strlen((char*)ParaAll->f16.UsrName_Virtual)!=0)
		memcpy(str, ParaAll->f16.UsrName_Virtual, strlen((char*)ParaAll->f16.UsrName_Virtual));
	else
		memcpy(str, "username", strlen("username"));
	edit_init(&deit_UsrName_Virtual, str, 24, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_ASC);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"密码: ", pos, LCD_NOREV);
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, ' ', INPUTKEYNUM);
	if(strlen((char*)ParaAll->f16.Pwd_Virtual)!=0)
		memcpy(str, ParaAll->f16.Pwd_Virtual, strlen((char*)ParaAll->f16.Pwd_Virtual));
	else
		memcpy(str, "passwd", strlen("passwd"));
	edit_init(&edit_Pwd_Virtual, str, 24, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_ASC);
	//------------------------------------------
	cur_node = &deit_UsrName_Virtual.form.node;
	cur_form = &deit_UsrName_Virtual.form;
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
						dbg_prt("\n msgbox_ret = OK  save para here!!!!");
						memcpy(&para_f16, &ParaAll->f16, sizeof(para_F16));
						memset(str,0,INPUTKEYNUM);
						memset(para_f16.UsrName_Virtual, 0, 32);
						eidt_gettext(&deit_UsrName_Virtual, str);
						memcpy(para_f16.UsrName_Virtual,str,32);
						memset(para_f16.Pwd_Virtual, 0, 32);
						memset(str,0,INPUTKEYNUM);
						eidt_gettext(&edit_Pwd_Virtual, str);
						memcpy(para_f16.Pwd_Virtual,str,32);
						setpara(&para_f16, 0, 16, sizeof(para_F16));
					}else
						dbg_prt("\n msgbox_ret = CANCEL  do not save***");
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
			rect.left = rect_Client.left ;
			gui_clrrect(rect);
			gui_setpos(&pos, rect_Client.left+FONTSIZE, rect_Client.top+FONTSIZE*2+ROW_INTERVAL);
			gui_textshow((char *)"用户名:", pos, LCD_NOREV);
			gui_setpos(&pos, rect_Client.left+FONTSIZE, rect_Client.top+FONTSIZE*6+ROW_INTERVAL*3);
			gui_textshow((char *)"密码: ", pos, LCD_NOREV);
			tmpnode = client.node.child;
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
			dbg_prt("\n cur_form:(%d)", cur_form->CtlType);
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
void menu_login_stat(){
	 char	str[50];
	 Point pos;
	int gprs_status = GPRS_CHECKMODEM;
	char stat_str[100];
	memset(stat_str, 0, 100);
	INT16U csq;
	csq = shmm_getdevstat()->Gprs_csq;
	if(shmm_getdevstat()->gprs_status==1)
		gprs_status = GPRS_CHECKMODEM;//AT OK
	else if(shmm_getdevstat()->gprs_status==2)
		gprs_status = GPRS_GETVER;//GWR OK
	else if(shmm_getdevstat()->gprs_status==3)
		gprs_status = GPRS_SIMOK;//CPIN OK
	else if(shmm_getdevstat()->gprs_status==4){
		gprs_status = GPRS_CREGOK;//CREG OK
		if(shmm_getdevstat()->jzq_login==1)
			gprs_status = GPRS_ONLINE;
		else if(shmm_getdevstat()->connect_ok==1)
			gprs_status = GPRS_CONNECTING;
		else if(shmm_getdevstat()->pppd_status==1)
			gprs_status = GPRS_DIALING;
	}else if(shmm_getdevstat()->gprs_status==0)
		gprs_status = GPRS_MODEM_INIT;//init
	switch(gprs_status)
	{
	case GPRS_MODEM_INIT:
		sprintf(stat_str, "%s", "GPRS:初始化通信模块");
		break;
	case GPRS_CHECKMODEM:
		sprintf(stat_str, "%s", "GPRS:检测AT命令成功");
		break;
	case GPRS_GETVER:
		sprintf(stat_str, "%s", "GPRS:获取模块版本信息");
		break;
	case GPRS_SIMOK:
		sprintf(stat_str, "%s", "GPRS:检测到SIM卡");
		break;
	case GPRS_CREGOK:
		sprintf(stat_str, "%s%d", "GPRS:注册网络,信号强度为",shmm_getdevstat()->Gprs_csq);
		break;
	case GPRS_DIALING:
		sprintf(stat_str, "%s", "GPRS:拨号中...");
		break;
	case GPRS_CONNECTING:
		sprintf(stat_str, "%s", "GPRS:连接到主站...");
		break;
	case GPRS_ONLINE:
		sprintf(stat_str, "%s", "GPRS:终端在线");
		break;
	}

	 gui_clrrect(rect_Client);
	 fprintf(stderr, "\n ------登录状态-----------");
	 gui_setpos(&pos, rect_Client.left+8*FONTSIZE, rect_Client.top+2*FONTSIZE);
	 memset(str,0, 50);
	 gui_textshow((char*)"终端登录状态", pos, LCD_NOREV);
	 pos.x = rect_Client.left + FONTSIZE*5;
	 pos.y += FONTSIZE*3;
	 memset(str,0, 50);
	 sprintf(str,"信号强度:%d", shmm_getdevstat()->Gprs_csq);
	 gui_textshow(str, pos, LCD_NOREV);
	 pos.x = rect_Client.left + FONTSIZE*5;
	 pos.y += FONTSIZE*3;
	 memset(str,0, 50);
	 sprintf(str,"登录状态");
	 gui_textshow(str, pos, LCD_NOREV);
	 pos.x = rect_Client.left + FONTSIZE*5;
	 pos.y += FONTSIZE*3;
	 sprintf(str,"%s", stat_str);
	 gui_textshow(str, pos, LCD_NOREV);
	 set_time_show_flag(1);
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		delay(300);
		if(PressKey==ESC)
			return;
		PressKey = NOKEY;
	}
}

#endif

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
    acChipType = p_JProgramInfo->ac_chip_type;
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
    acWireType = p_JProgramInfo->WireType;
    if(0x1200 == acWireType)
       	sprintf(substr,"三相三线");
    else if(0x0600 == acWireType)
    	sprintf(substr,"三相四线");
    else
    	sprintf(substr,"未知接线方式");
    sprintf(str,"接线方式:%s", substr);
    gui_textshow(str, pos, LCD_NOREV);
    set_time_show_flag(1);
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		delay(300);
		if(PressKey==ESC)
			return;
		PressKey = NOKEY;
	}
}

void menu_TorF_info()
{
//	 INT8U ret = 0;
//	 if((access((const char*)FILE_OPTIONS_FILED_PATH,W_OK) ==0)&&(access((const char*)FILE_OPTIONS_TEST_PATH,W_OK) ==0))
//	 {
//		 msgbox_label((char*)"请确认option文件", CTRL_BUTTON_OK);
//		 return;
//	 }
//	 if(access((const char*)FILE_OPTIONS_FILED_PATH,W_OK) ==0)
//	  {
//		ret = msgbox_label((char*)"当前状态T,切换?", CTRL_BUTTON_OK);
//		if(ret==ACK){
//			if(access((const char*)FILE_OPTIONS_PATH,W_OK) ==0)
//			  {
//				  rename(FILE_OPTIONS_PATH,FILE_OPTIONS_TEST_PATH);
//				  rename(FILE_OPTIONS_FILED_PATH,FILE_OPTIONS_PATH);
//				  printf("\n已切换到现场配置..");
//			  }
//		}
//	  }
//	  else if(access((const char*)FILE_OPTIONS_TEST_PATH,W_OK) ==0)
//	  {
//		  ret = msgbox_label((char*)"当前状态F,切换?", CTRL_BUTTON_OK);
//		  if(ret==ACK){
//			  if(access((const char*)FILE_OPTIONS_PATH,W_OK) ==0)
//			  {
//				  rename(FILE_OPTIONS_PATH,FILE_OPTIONS_FILED_PATH);
//				  rename(FILE_OPTIONS_TEST_PATH,FILE_OPTIONS_PATH);
//				  printf("\n已切换到测试配置..");
//			  }
//		  }
//	  }
//	  else
//	  {
//		  msgbox_label((char*)"请确认option文件", CTRL_BUTTON_OK);
//	  }
//	 return;
}
#if 1//(defined(NINGXIA)||defined(SHANDONG))

void menu_485func_change()
{
//	INT8U firstFlag = 1;
//	mmq_head	mq_h;
//	mqd_t mqd;
//	gdm_chg_cfgfile protswitch;
//	INT8U ret;
//	char str[100];
//	memset(str, 0, 100);
//	while(1)
//	{
//		if(firstFlag == 0)
//			usleep(3000*1000);
//		firstFlag = 0;
//		if(prog_getsyspid_bycmd((INT8S *)"vs485 2") > 0)
//		{
//			sprintf(str, "当前:%s","抄表口");
//			protswitch.how = 2;
//		}
//		else if(prog_getsyspid_bycmd((INT8S *)"vcom 2") > 0)
//		{
//			sprintf(str, "当前:%s","维护口");
//			protswitch.how = 1;
//		}
//		else
//			sprintf(str, "当前:%s","unkown");
//		ret = menu_485func_show((char*)str, CTRL_BUTTON_OK);//按键触发返回，没有按键时一直显示当前的状态
//		if(ret == 0)
//			return;
//		else if(ret == ACK)
//		{
//			fprintf(stderr,"ack\n");
//			if(protswitch.how == 1)
//			{
//				protswitch.how = 2;
//			}
//			else if(protswitch.how == 2)
//			{
//				protswitch.how = 1;
//			}
//			memset(&mq_h, 0, sizeof(mmq_head));
//			if((mqd =  createMsg((INT8S *)COM_VMAIN_MQ, O_WRONLY)) <0)
//			{//创建液晶发送消息队列
//				dbg_prt( "\nGUI createMsg %s  error!!2  mqd=%d",COM_VMAIN_MQ, mqd);
//				return;
//			}
//			fprintf(stderr,"creat message success\n");
//			if(sendMsg(mqd, MAIN_CHG_CFGFILE, (INT8S*)&protswitch.how,sizeof(protswitch.how)) < 0)
//			{
//				dbg_prt( "\nGUI sendmsg %s  error!!2  mqd=%d",COM_VMAIN_MQ, mqd);
//				colseMsg(mqd);
//				return;
//			}
//			fprintf(stderr,"send message success\n");
//		}
//	}
//	return;
}
#endif

void menu_readmeter_info()
{
//	int i=0,m=0,fail_num=0,total_all=0,total_all_suc=0;
//	INT16U totalnum_succ_RdM;
//	INT16U n_grp = MP_MAXNUM/MAXMPNUM_DATAFILE;
//	INT16U fail_mpindex[MP_MAXNUM];
//	memset(fail_mpindex,0,sizeof(fail_mpindex));
//	TmS cbdate;
//	tmget(&cbdate);
//	tminc(&cbdate,DAY,-1);
//	int meter_total[6];
//	memset(meter_total,0,sizeof(meter_total));
//	char	str[128];
//	Point pos;
//	meter_total[0] = shmm_getdevstat()->info_RdM[1].totalnum_RdM;  //485-1总抄表块数
//	meter_total[1] = shmm_getdevstat()->info_RdM[2].totalnum_RdM; ////485-2总抄表块数
//	meter_total[2] = shmm_getdevstat()->info_RdM[30].totalnum_RdM;//载波总抄表块数
//	total_all = meter_total[0] + meter_total[1] + meter_total[2];
//	for(i=0; i< MAXNUM_READMETER_ROAD; i++)
//	{
//		if(shmm_getdevstat()->info_RdM[i].is_valid == 1)
//		{
//			INT8U cindex=0;
//			totalnum_succ_RdM=0;
//			char DataFileName[50];
//			for(cindex=0;cindex<=n_grp;cindex++)
//			{
//				memset(DataFileName,0,50);
//				sprintf((char*)DataFileName,"%s/%04d%02d%02d_%d.dat",SAVE_PATH_DAY,cbdate.Year,cbdate.Month,cbdate.Day,cindex);
//				if(access((char*)DataFileName,F_OK)!=0)
//				{
//					continue;
//				}
//				FreezeData_SAVE dayfreez_data;
//				for(m=cindex*MAXMPNUM_DATAFILE-1;m<(cindex+1)*MAXMPNUM_DATAFILE-1;m++)
//				{
//					if(m<0)
//						continue;
//					if((shmm_getpara()->f10.para_mp[m].MPNo>0)&&(shmm_getpara()->f10.para_mp[m].Port==(i+PortBegNum)))
//					{
//						memset(&dayfreez_data,0,sizeof(FreezeData_SAVE));
//						dayfreez_data.id_d=824;
//						dayfreez_data.id_mp=shmm_getpara()->f10.para_mp[m].MPNo;
//						dayfreez_data.tm_collect.Year=cbdate.Year;
//						dayfreez_data.tm_collect.Month=cbdate.Month;
//						dayfreez_data.tm_collect.Day=cbdate.Day;
//						dayfreez_data.tm_collect.Hour=0;
//						dayfreez_data.tm_collect.Minute=0;
//						if(fu_query((INT8S*)DataFileName,(void*)&dayfreez_data,sizeof(FreezeData_SAVE),2)>=0)
//						{
//							totalnum_succ_RdM++;
//						}
//						else
//						{
//							fail_mpindex[fail_num++] = m;
//						}
//					}
//				}
//			}
//			if(i == 1) meter_total[3] = totalnum_succ_RdM; //485-1成功块数
//			if(i == 2) meter_total[4] = totalnum_succ_RdM;//485-2成功块数
//			if(i == 30) meter_total[5] = totalnum_succ_RdM;//载波成功块数
//		}
//	}
//	for(i = 0;i<fail_num;i++)
//		fprintf(stderr,"failmp == %d\n",shmm_getpara()->f10.para_mp[i].MPNo);
//	total_all_suc = meter_total[3] + meter_total[4] + meter_total[5];
//	//if(total_all_suc > 1024)  total_all_suc = 0; //超限，归零
//	gui_clrrect(rect_Client);
//	gui_setpos(&pos, rect_Client.left+10*FONTSIZE, rect_Client.top+FONTSIZE);
//	gui_textshow((char*)"抄表信息", pos, LCD_NOREV);
//	pos.x = rect_Client.left + FONTSIZE*2;
//	pos.y += FONTSIZE*3;
//	sprintf(str,"总数:%d  成功数:%d",total_all,total_all_suc);
//	gui_textshow(str, pos, LCD_NOREV);
//	pos.y += FONTSIZE*3+1;
//	sprintf(str,"485-1总数:%d  成功:%d",meter_total[0],meter_total[3]);
//	gui_textshow(str, pos, LCD_NOREV);
//	pos.y += FONTSIZE*3+1;
//	sprintf(str,"485-2总数:%0d  成功:%d",meter_total[1],meter_total[4]);
//	gui_textshow(str, pos, LCD_NOREV);
//	pos.y += FONTSIZE*3+1;
//	sprintf(str,"载波表总数:%d  成功:%d",meter_total[2],meter_total[5]);
//	gui_textshow(str, pos, LCD_NOREV);
//	pos.y += FONTSIZE*3+1;
//	sprintf(str,"查看详细信息，请按确认键");
//	gui_textshow(str, pos, LCD_NOREV);
//  set_show_time_flag(1);
//	while(g_LcdPoll_Flag==LCD_NOTPOLL){
//		PressKey = NOKEY;
//		delay(300);
//		if(PressKey==ESC)
//			return;
//		if(PressKey==OK)
//		{
//			if(fail_num == 0) continue;
//			menu_fail_meterinfo(fail_mpindex,fail_num);
//			return;
//		}
//	}
}

void menu_fail_meterinfo(INT16U *fail_mpindex,INT16U fail_num)
{
//	Gui_MP_t *fail_mp = NULL;
//	int cur_cldno=1, begin_cldno=1, i=0, presskey_ok_acs=NOKEY;
//	Point pos;
//	char first_flg=0, str_cld[50], addr[13];
//	Rect rect;
//	if(fail_num > MP_MAXNUM){
//		msgbox_label((char *)"统计错误", CTRL_BUTTON_OK);
//		return;
//	}
//	fail_mp = (Gui_MP_t*)malloc(fail_num*sizeof(Gui_MP_t));
//	for(i = 0;i<fail_num;i++)
//	{
//		if(*(fail_mpindex+i) != 0 )
//		{
//			fail_mp[i].iidnex = shmm_getpara()->f10.para_mp[*(fail_mpindex+i)].Index;
//			fail_mp[i].mpno = shmm_getpara()->f10.para_mp[*(fail_mpindex+i)].MPNo;
//			memcpy(fail_mp[i].cldaddr,shmm_getpara()->f10.para_mp[*(fail_mpindex+i)].addr,12);
//		}
//	}
//	dbg_prt("\n 抄读失败测量点总数：%d",fail_num);
//	PressKey = NOKEY;
//	while(g_LcdPoll_Flag==LCD_NOTPOLL){
//		switch(PressKey)
//		{
//		case LEFT:
//			if(fail_num>PAGE_COLNUM-1){
//				if(begin_cldno!=1 ){
//					begin_cldno -= PAGE_COLNUM-1;
//					if(begin_cldno<=0)
//						begin_cldno = 1;
//					cur_cldno = begin_cldno;
//				}else
//					cur_cldno = begin_cldno = fail_num - (PAGE_COLNUM-1);
//			}
//			break;
//		case UP:
//			cur_cldno--;
//			if(cur_cldno<=0 && fail_num>PAGE_COLNUM){
//				cur_cldno = fail_num;
//				begin_cldno = fail_num - PAGE_COLNUM + 2;
//			}else if(cur_cldno<=0 && fail_num<=PAGE_COLNUM){
//				cur_cldno = fail_num;
//			}else if(cur_cldno<=begin_cldno)
//				begin_cldno = cur_cldno;
//			break;
//		case RIGHT:
//			if((begin_cldno+PAGE_COLNUM-1)>=fail_num){
//				cur_cldno = begin_cldno = 1;
//			}else{
//				begin_cldno += PAGE_COLNUM-1;
//				cur_cldno = begin_cldno;
//			}
//			break;
//		case DOWN:
//			cur_cldno++;
//			if(cur_cldno>fail_num)
//				begin_cldno = cur_cldno = 1;
//			else if(cur_cldno>begin_cldno+PAGE_COLNUM-2)
//				begin_cldno++;
//			break;
//		case OK:
//			break;
//		case ESC:
//			gui_mp_free(fail_mp);
//			return;
//		default:
//			break;
//		}
//		if(PressKey!=NOKEY||first_flg==0||presskey_ok_acs==OK){
//			presskey_ok_acs = NOKEY;
//			first_flg = 1;
//			gui_clrrect(rect_Client);
//			pos.x = rect_Client.left;
//			pos.y = rect_Client.top+1;
//			gui_textshow((char *)"测量点号        表地址", pos, LCD_NOREV);
//			for(i=begin_cldno; i<begin_cldno+PAGE_COLNUM-1; i++){
//				if(i>fail_num)
//					continue;
//				memset(str_cld, 0, 50);
//				memset(addr, 0, 13);
//				memcpy(addr, (fail_mp+i-1)->cldaddr, 12);
//				sprintf(str_cld, "  %04d       %s",(fail_mp+i-1)->mpno, addr);
//				pos.x = rect_Client.left;
//				pos.y = rect_Client.top + (i-begin_cldno+1)*FONTSIZE*2 + 2;
//				gui_textshow(str_cld, pos, LCD_NOREV);
//				if(i==cur_cldno){
//					memset(&rect, 0, sizeof(Rect));
//					rect = gui_getstrrect((unsigned char*)str_cld, pos);//获得字符串区域
//					gui_reverserect(rect);
//				}
//			}
//          set_time_show_flag(1);
//		}
//		PressKey = NOKEY;
//		delay(50);
//	}
}

void menu_yxstatus_js()
{
//	Point pos;
//	char str[100];
//	char str_ch[3];
//	memset(str_ch,0,3);
//	INT8U yx1_attrib,yx2_attrib;
//	PressKey=NOKEY;
//	while(g_LcdPoll_Flag==LCD_NOTPOLL)
//	{
//		yx1_attrib = ParaAll->f12.StatePropFlag & 0x01;//遥信属性
//		yx2_attrib = (ParaAll->f12.StatePropFlag & 0x02)>>1;//遥信属性
//		gui_clrrect(rect_Client);
//		pos.x = rect_Client.left+FONTSIZE*6;
//		pos.y = rect_Client.top + ROW_INTERVAL + FONTSIZE;
//		setFontSize(16);
//		gui_textshow((char *)"遥信状态", pos, LCD_NOREV);
//		setFontSize(12);
//		pos.x = rect_Client.left+FONTSIZE*2;
//		pos.y +=  FONTSIZE*3.5;
//		gui_textshow((char *)"开 关 号:    1     2", pos, LCD_NOREV);
//		pos.y +=  FONTSIZE*3.5;
//		sprintf((char*)str, "开关状态:   %s    %s",
//						shmm_getdevstat()->YxStat&0x01?"合":"分",
//						shmm_getdevstat()->YxStat&0x02?"合":"分"	);
//		gui_textshow(str, pos, LCD_NOREV);
//		pos.y +=  FONTSIZE*3.5;
//		gui_textshow((char *)"是否变位:", pos, LCD_NOREV);
//		//	//0x14 0x1b空心 0x25 0x7d实心//空心为变位状态
//		pos.x += 12*FONTSIZE;
//		if((shmm_getdevstat()->YxChange&0x01) == 0x01)
//		{
//			str_ch[0] = 0x25;
//			str_ch[1] = 0x7d;
//		}
//		else
//		{
//			str_ch[0] = 0x14;
//			str_ch[1] = 0x1b;
//		}
//		gui_textshow_16(str_ch, pos, LCD_NOREV);
//		pos.x += 6*FONTSIZE;
//		if((shmm_getdevstat()->YxChange&0x02) == 0x02)
//		{
//			str_ch[0] = 0x25;
//			str_ch[1] = 0x7d;
//		}
//		else
//		{
//			str_ch[0] = 0x14;
//			str_ch[1] = 0x1b;
//		}
//		gui_textshow_16(str_ch, pos, LCD_NOREV);
//		pos.y +=  FONTSIZE*3.5;
//		pos.x = rect_Client.left+FONTSIZE*2;
//		sprintf((char*)str, "开关属性:  %s  %s",
//						yx1_attrib?"动合":"动断",yx2_attrib?"动合":"动断");
//		gui_textshow(str, pos, LCD_NOREV);
//      set_time_show_flag(1);
//		if(PressKey==ESC){
//			break;
//		}
//		delay(100);
//	}
}

void menu_yxstatus(){
//	Point pos;
//	INT8U str[100];
//	INT8U yx1_attrib,yx2_attrib;
//#ifdef CCTT_I
//	INT8U yx3_attrib,yx4_attrib,yx5_attrib;//集中器3、4路遥信和第5路门节点
//#endif
//
//#ifdef SPTF_III
//	INT8U yx3_attrib;//专变门节点
//#endif
//
//	while(g_LcdPoll_Flag==LCD_NOTPOLL){
//		if(PressKey==ESC){
//			break;
//		}
//		delay(100);
//
//#ifdef SPTF_III
//		yx3_attrib = (ParaAll->f12.StatePropFlag & 0x04)>>2;//专变门节点属性
//#endif
//		fprintf(stderr,"shuxing = %d\n",ParaAll->f12.StatePropFlag);
//		fprintf(stderr,"jieru = %d\n",shmm_getdevstat()->YxStat);
//		yx1_attrib = ParaAll->f12.StatePropFlag & 0x01;//遥信属性
//		yx2_attrib = (ParaAll->f12.StatePropFlag & 0x02)>>1;//遥信属性
//#ifdef CCTT_I
//		//集中器四路遥信
//		yx3_attrib = (ParaAll->f12.StatePropFlag & 0x04)>>2;//遥信属性
//		//fprintf(stderr,"%c \n",yx3_attrib);
//		yx4_attrib = (ParaAll->f12.StatePropFlag & 0x08)>>3;//遥信属性
//
//		yx5_attrib = (ParaAll->f12.StatePropFlag & 0x10)>>4;//遥信属性
//#endif
//		gui_clrrect(rect_Client);
//		gui_setpos(&pos, rect_Client.left+6*FONTSIZE, rect_Client.top+FONTSIZE);
//		gui_textshow((char*)"当前开关量状态", pos, LCD_NOREV);
//		memset(str, 0, 100);
//		pos.x = rect_Client.left + FONTSIZE*9;
//		pos.y += FONTSIZE*3;
//		gui_textshow((char*)"状态  变位  属性", pos, LCD_NOREV);
//		pos.x = rect_Client.left + FONTSIZE;
//		pos.y += FONTSIZE*3-2;
//		memset(str, 0, 100);
//		sprintf((char*)str, "开关量1: %s    %s   %s",
//				shmm_getdevstat()->YxStat&0x01?"合":"分",
//				shmm_getdevstat()->YxChange&0x01?"是":"否",
//				yx1_attrib?"动合":"动断");
//		gui_textshow((char*)str, pos, LCD_NOREV);
//		pos.y += FONTSIZE*3-2;
//		memset(str, 0, 100);
//		sprintf((char*)str, "开关量2: %s    %s   %s",
//				shmm_getdevstat()->YxStat&0x02?"合":"分",
//				shmm_getdevstat()->YxChange&0x02?"是":"否",
//				yx2_attrib?"动合":"动断");
//		gui_textshow((char*)str, pos, LCD_NOREV);
//#ifdef CCTT_I
//		pos.x = rect_Client.left + FONTSIZE;
//		pos.y += FONTSIZE*3-2;
//		memset(str, 0, 100);
//		sprintf((char*)str, "开关量3: %s    %s   %s",
//				shmm_getdevstat()->YxStat&0x04?"合":"分",
//				shmm_getdevstat()->YxChange&0x04?"是":"否",
//				yx3_attrib?"动合":"动断");
//		gui_textshow((char*)str, pos, LCD_NOREV);
//		pos.y += FONTSIZE*3-2;
//		memset(str, 0, 100);
//		sprintf((char*)str, "开关量4: %s    %s   %s",
//				shmm_getdevstat()->YxStat&0x08?"合":"分",
//				shmm_getdevstat()->YxChange&0x08?"是":"否",
//				yx4_attrib?"动合":"动断");
//		gui_textshow((char*)str, pos, LCD_NOREV);
//		pos.y += FONTSIZE*3-2;
//		memset(str, 0, 100);
//		sprintf((char*)str, "门节点1: %s    %s   %s",
//				shmm_getdevstat()->YxStat&0x10?"合":"分",
//				shmm_getdevstat()->YxChange&0x10?"是":"否",
//				yx5_attrib?"动合":"动断");
//		gui_textshow((char*)str, pos, LCD_NOREV);
//#endif
//#ifdef SPTF_III
//		pos.y += FONTSIZE*3-2;
//		memset(str, 0, 100);
//		sprintf((char*)str, "门节点1: %s    %s   %s",
//			shmm_getdevstat()->YxStat&0x04?"合":"分",
//			shmm_getdevstat()->YxChange&0x04?"是":"否",
//			yx3_attrib?"动合":"动断");
//		gui_textshow((char*)str, pos, LCD_NOREV);
//		pos.y += FONTSIZE*3-2;
//		memset(str, 0, 100);
//		sprintf((char*)str, "脉冲1计数：	%d",shmm_getpubdata()->pulse_calc_by_vstate.collects[0].count);
//		gui_textshow((char*)str, pos, LCD_NOREV);
//		pos.y += FONTSIZE*3-2;
//		memset(str, 0, 100);
//		sprintf((char*)str, "脉冲2计数：	%d",shmm_getpubdata()->pulse_calc_by_vstate.collects[1].count);
//		gui_textshow((char*)str, pos, LCD_NOREV);
//#endif
//      set_time_show_flag(1);
//		PressKey = NOKEY;
//		delay(300);
//	}
//	return;
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
#ifdef JIANGSU
static void setmp_cbtext_isvalid(char cb_text[][TEXTLEN_Y]){
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "否", strlen("否"));
	memcpy(cb_text[1], "是", strlen("是"));
}
void show_realdatabycld_js(int pindex)
{
	if(pindex <=0)
		return;
	LcdDataItem item[10];
	memset(item,0,sizeof(LcdDataItem)*10);
	int data_did[16][2]={{3117,5},{3122,5},{3137,5},{3142,5},{3127,5},
			{3147,5},{3152,5},{3132,5},{3043,4},{3036,3},{3039,3},{3024,4},{3028,4},{3032,4},{3157,3162},{3177,3182}};
	INT8U cur_did=0;
	INT8U mp_port=0;
	INT16U i;
	INT16U cur_index=0;//当前总的有效电表块数序号
	INT16U cld_max=0;
	INT8U first_flg=0;
	INT8U mqcount=0;
	Gui_MP_t *gui_mpmax=NULL;
	cld_max = gui_mp_compose(&gui_mpmax);
	PressKey = NOKEY;
	int PressKey_ok=0;
//	cur_index=pindex;
	for(i=0;i<cld_max;i++)
	{
		cur_index++;
//		fprintf(stderr,"iindex =%d\n",(gui_mpmax+i)->iidnex);
//		fprintf(stderr,"cur_index 1111 = %d\n",cur_index);
		if((gui_mpmax+i)->iidnex == pindex)
			break;
	}
//	fprintf(stderr,"cur_index 1111 = %d\n",cur_index);
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		switch(PressKey){
		case LEFT:
			cur_index++;
			if(cur_index>cld_max) cur_index=1;
			PressKey_ok=PressKey;
			break;
		case RIGHT:
			cur_index--;
			if(cur_index>cld_max) cur_index=1;
			if(cur_index<=0)cur_index=cld_max;
			PressKey_ok=PressKey;
			break;
		case UP:
			cur_did++;
			if(cur_did>15) cur_did=0;
			PressKey_ok=PressKey;
			break;
		case DOWN:
			cur_did--;
			if(cur_did<0 || cur_did>=16) cur_did=15;
			PressKey_ok=PressKey;
			break;
		case OK:
			PressKey_ok=NOKEY;
			break;
		case ESC:
		{
			gui_mp_free(gui_mpmax);
			return;//如果转换到轮显状态此处如何处理？？？？
		}
		default:
			break;
		}
		if(PressKey_ok!=NOKEY||first_flg==0)
		{
			PressKey_ok=0;
			first_flg=1;
			if(cur_index>cld_max || cur_index<0) cur_index=1;
			mp_port = getmpport((gui_mpmax+cur_index-1)->iidnex);
			if(mp_port == PORT_JC && cur_did == 15)
			{
				if(PressKey == UP) cur_did = 1;
				if(PressKey == DOWN) cur_did = 14;
			}
			if(mp_port == 0)
				break;
			g_curcldno=(gui_mpmax+cur_index-1)->mpno;
			if(mp_port !=PORT_JC)
			{
				if(cur_did == 14 || cur_did == 15)//需量需要抄读2个数据项，单独处理,只能这样了，我也没办法
				{
					if(mp_port == PORT_ZB)
					{
						mqcount = requestdata_485_ZB_Block((gui_mpmax+cur_index-1)->mpno, (INT8U*)PLC_REV_GUI_MQ,5,data_did[cur_did][0], item);
						mqcount += requestdata_485_ZB_Block((gui_mpmax+cur_index-1)->mpno, (INT8U*)PLC_REV_GUI_MQ,5,data_did[cur_did][1], &item[5]);
					}
					else if(mp_port == PORT_485I)
					{
						mqcount = requestdata_485_ZB_Block((gui_mpmax+cur_index-1)->mpno,(INT8U*)S485_1_REV_GUI_MQ, 5,data_did[cur_did][0], item);
						mqcount += requestdata_485_ZB_Block((gui_mpmax+cur_index-1)->mpno,(INT8U*)S485_1_REV_GUI_MQ, 5,data_did[cur_did][1], &item[5]);
					}
					else if(mp_port == PORT_485II)
					{
						mqcount = requestdata_485_ZB_Block((gui_mpmax+cur_index-1)->mpno,(INT8U*)S485_2_REV_GUI_MQ, 5,data_did[cur_did][0], item);
						mqcount += requestdata_485_ZB_Block((gui_mpmax+cur_index-1)->mpno,(INT8U*)S485_2_REV_GUI_MQ, 5,data_did[cur_did][1], &item[5]);
					}
					if(cur_did == 14)
						show_xuliang(mqcount,item,157);
					if(cur_did == 15)
						show_xuliang(mqcount,item,177);
				}
				else
				{
					if(mp_port == PORT_ZB)
						mqcount = requestdata_485_ZB_Block((gui_mpmax+cur_index-1)->mpno, (INT8U*)PLC_REV_GUI_MQ,data_did[cur_did][1], data_did[cur_did][0], item);
					else if(mp_port == PORT_485I)
						mqcount = requestdata_485_ZB_Block((gui_mpmax+cur_index-1)->mpno,(INT8U*)S485_1_REV_GUI_MQ,data_did[cur_did][1], data_did[cur_did][0], item);
					else if(mp_port == PORT_485II)
						mqcount = requestdata_485_ZB_Block((gui_mpmax+cur_index-1)->mpno,(INT8U*)S485_2_REV_GUI_MQ,data_did[cur_did][1], data_did[cur_did][0], item);
					pass_mpdata(1,data_did[cur_did][0],mqcount,item);
				}
			}
			else if(mp_port == PORT_JC)
			{
				gui_clrrect(rect_Client);//清除客户区
				ShowJCData(cur_did, item, data_did[cur_did][1], 2);
			}
			PressKey=NOKEY;
		}
		delay(100);
	}
	gui_mp_free(gui_mpmax);//跳入轮显后释放？？？
}
void show_daydata_JS(int cldno){//江苏专用
	char str[100];
	TmS cj_date;
	FreezeData_SAVE fd_s;
	Point pos;
	INT8U cur_did=1;
	INT16U i;
	INT16U cur_index=0;//当前总的有效电表块数序号
	INT16U cld_max=0;
	INT8U first_flg=0;
	Gui_MP_t *gui_mpmax=NULL;
	cld_max = gui_mp_compose(&gui_mpmax);
	int PressKey_ok=0;
	memset(&cj_date, 0, sizeof(TmS));
	if(cldno <=0)
	return;
	for(i=0;i<cld_max;i++)
	{
		cur_index++;
		if((gui_mpmax+i)->iidnex == cldno)
			break;
	}
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		switch(PressKey){
		case LEFT:
			cur_index++;
			if(cur_index>cld_max) cur_index=1;
			PressKey_ok=PressKey;
			break;
		case RIGHT:
			cur_index--;
			if(cur_index>cld_max) cur_index=1;
			if(cur_index<=0)cur_index=cld_max;
			PressKey_ok=PressKey;
			break;
		case UP:
			cur_did++;
			if(cur_did>4) cur_did=1;
			PressKey_ok=PressKey;
			break;
		case DOWN:
			cur_did--;
			if(cur_did<1)  cur_did=4;
			if(cur_did>4) cur_did=1;
			PressKey_ok=PressKey;
			break;
		case OK:
			PressKey_ok=NOKEY;
			break;
		case ESC:
		{
			gui_mp_free(gui_mpmax);
			return;//如果转换到轮显状态此处如何处理？？？？
		}
		default:
			break;
		}
		if(PressKey_ok!=NOKEY||first_flg==0)
		{
			setFontSize(16);
			PressKey=NOKEY;
			PressKey_ok=0;
			first_flg=1;
			if(cur_index>cld_max || cur_index<0) cur_index=1;
			gui_clrrect(rect_Client);
			gui_setpos(&pos, rect_Client.left+8*FONTSIZE, rect_Client.top+FONTSIZE);
			g_curcldno=(gui_mpmax+cur_index-1)->mpno;
			setFontSize(16);
			gui_textshow((char*)"日数据", pos, LCD_NOREV);
			setFontSize(12);
			fprintf(stderr,"cur_did = %d cur_index = %d mpno = %d \n",cur_did,cur_index,(gui_mpmax+cur_index-1)->mpno);
			if(cur_did == 1)//正向有功
			{
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"正向有功总:", 824, 2, 0, rect_Client.top + FONTSIZE*4);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"正向有功尖:", 825, 2, 0, rect_Client.top + FONTSIZE*6+3);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"正向有功峰:", 826, 2, 0, rect_Client.top + FONTSIZE*8+6);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"正向有功平:", 827, 2, 0, rect_Client.top + FONTSIZE*10+9);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"正向有功谷:", 828, 2, 0, rect_Client.top + FONTSIZE*12+12);
			}
			else if(cur_did == 2)
			{
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"反向有功总:", 844, 2, 0, rect_Client.top + FONTSIZE*4);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"反向有功尖:", 945, 2, 0, rect_Client.top + FONTSIZE*6+3);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"反向有功峰:", 946, 2, 0, rect_Client.top + FONTSIZE*8+6);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"反向有功平:", 947, 2, 0, rect_Client.top + FONTSIZE*10+9);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"反向有功谷:", 948, 2, 0, rect_Client.top + FONTSIZE*12+12);
			}
			else if(cur_did == 3)
			{
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"正向需量总:", 864, 4, 0, rect_Client.top + FONTSIZE*4);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"正向需量尖:", 865, 4, 0, rect_Client.top + FONTSIZE*6+3);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"正向需量峰:", 866, 4, 0, rect_Client.top + FONTSIZE*8+6);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"正向需量平:", 867, 4, 0, rect_Client.top + FONTSIZE*10+9);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"正向需量谷:", 868, 4, 0, rect_Client.top + FONTSIZE*12+12);
			}
			else
			{
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"反向需量总:", 884, 4, 0, rect_Client.top + FONTSIZE*4);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"反向需量尖:", 885, 4, 0, rect_Client.top + FONTSIZE*6+3);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"反向需量峰:", 886, 4, 0, rect_Client.top + FONTSIZE*8+6);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"反向需量平:", 887, 4, 0, rect_Client.top + FONTSIZE*10+9);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"反向需量谷:", 888, 4, 0, rect_Client.top + FONTSIZE*12+12);
			}
			if(read_filedata((char*)DongJie_FileName, cldno, 824, DONGJIE_DATA, (void*)&fd_s)==1)
				memcpy(&cj_date, &fd_s.tm_collect, sizeof(TmS));
			memset(str, 0, 100);
			if(cj_date.Year==0|| cj_date.Month==0||cj_date.Day==0)
				sprintf((char*)str,"抄表时间 00/00/00 00:00");
			else
				sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
					cj_date.Year-2000, cj_date.Month, cj_date.Day, cj_date.Hour, cj_date.Minute);
			gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+18*FONTSIZE);
			gui_textshow(str, pos, LCD_NOREV);
			set_time_show_flag(1);
//			fprintf(stderr,"cur_did2222 = %d\n",cur_did);
//			fprintf(stderr,"mpno = %d  mpindex=%d\n",(gui_mpmax+cur_index-1)->mpno,cur_index);
		}
		delay(100);
	}
	gui_mp_free(gui_mpmax);//跳入轮显后释放？？？
}
void show_monthdata_JS(int cldno){//江苏专用
	char str[100];
	TmS cj_date;
	FreezeData_SAVE fd_s;
	Point pos;
	INT8U cur_did=1;
	INT16U i;
	INT16U cur_index=0;//当前总的有效电表块数序号
	INT16U cld_max=0;
	INT8U first_flg=0;
	Gui_MP_t *gui_mpmax=NULL;
	cld_max = gui_mp_compose(&gui_mpmax);
	int PressKey_ok=0;
	memset(&cj_date, 0, sizeof(TmS));
	if(cldno <=0)
	return;
	for(i=0;i<cld_max;i++)
	{
		cur_index++;
		if((gui_mpmax+i)->iidnex == cldno)
			break;
	}
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		switch(PressKey){
		case LEFT:
			cur_index++;
			if(cur_index>cld_max) cur_index=1;
			PressKey_ok=PressKey;
			break;
		case RIGHT:
			cur_index--;
			if(cur_index>cld_max) cur_index=1;
			if(cur_index<=0)cur_index=cld_max;
			PressKey_ok=PressKey;
			break;
		case UP:
			cur_did++;
			if(cur_did>4) cur_did=1;
			PressKey_ok=PressKey;
			break;
		case DOWN:
			cur_did--;
			if(cur_did<1)  cur_did=4;
			if(cur_did>4) cur_did=1;
			PressKey_ok=PressKey;
			break;
		case OK:
			PressKey_ok=NOKEY;
			break;
		case ESC:
		{
			gui_mp_free(gui_mpmax);
			return;//如果转换到轮显状态此处如何处理？？？？
		}
		default:
			break;
		}
		if(PressKey_ok!=NOKEY||first_flg==0)
		{
			PressKey=NOKEY;
			PressKey_ok=0;
			first_flg=1;
			if(cur_index>cld_max || cur_index<0) cur_index=1;
			gui_clrrect(rect_Client);
			gui_setpos(&pos, rect_Client.left+8*FONTSIZE, rect_Client.top+FONTSIZE);
			g_curcldno=(gui_mpmax+cur_index-1)->mpno;
			setFontSize(16);
			gui_textshow((char*)"月数据", pos, LCD_NOREV);
			setFontSize(12);
			if(cur_did == 1)//正向有功
			{
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"正向有功总:", 924, 2, 0, rect_Client.top + FONTSIZE*4);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"正向有功尖:", 925, 2, 0, rect_Client.top + FONTSIZE*6+3);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"正向有功峰:", 926, 2, 0, rect_Client.top + FONTSIZE*8+6);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"正向有功平:", 927, 2, 0, rect_Client.top + FONTSIZE*10+9);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"正向有功谷:", 928, 2, 0, rect_Client.top + FONTSIZE*12+12);
			}
			else if(cur_did == 2)
			{
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"反向有功总:", 944, 2, 0, rect_Client.top + FONTSIZE*4);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"反向有功尖:", 945, 2, 0, rect_Client.top + FONTSIZE*6+3);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"反向有功峰:", 946, 2, 0, rect_Client.top + FONTSIZE*8+6);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"反向有功平:", 947, 2, 0, rect_Client.top + FONTSIZE*10+9);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"反向有功谷:", 948, 2, 0, rect_Client.top + FONTSIZE*12+12);
			}
			else if(cur_did == 3)
			{
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"正向需量总:", 964, 4, 0, rect_Client.top + FONTSIZE*4);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"正向需量尖:", 965, 4, 0, rect_Client.top + FONTSIZE*6+3);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"正向需量峰:", 966, 4, 0, rect_Client.top + FONTSIZE*8+6);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"正向需量平:", 967, 4, 0, rect_Client.top + FONTSIZE*10+9);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"正向需量谷:", 968, 4, 0, rect_Client.top + FONTSIZE*12+12);
			}
			else
			{
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"反向需量总:", 984, 4, 0, rect_Client.top + FONTSIZE*4);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"反向需量尖:", 985, 4, 0, rect_Client.top + FONTSIZE*6+3);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"反向需量峰:", 986, 4, 0, rect_Client.top + FONTSIZE*8+6);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"反向需量平:", 987, 4, 0, rect_Client.top + FONTSIZE*10+9);
				dataitem_showvalue(DongJie_FileName,(gui_mpmax+cur_index-1)->mpno, (char*)"反向需量谷:", 988, 4, 0, rect_Client.top + FONTSIZE*12+12);
			}
			if(read_filedata((char*)DongJie_FileName, cldno, 924, DONGJIE_DATA, (void*)&fd_s)==1)
				memcpy(&cj_date, &fd_s.tm_collect, sizeof(TmS));
			memset(str, 0, 100);
			if(cj_date.Year==0|| cj_date.Month==0||cj_date.Day==0)
				sprintf((char*)str,"抄表时间 00/00/00 00:00");
			else
				sprintf((char*)str,"抄表时间 %02d/%02d/%02d %02d:%02d",
					cj_date.Year-2000, cj_date.Month, cj_date.Day, cj_date.Hour, cj_date.Minute);
			gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+18*FONTSIZE);
			gui_textshow(str, pos, LCD_NOREV);
			set_time_show_flag(1);
//			fprintf(stderr,"cur_did2222 = %d\n",cur_did);
//			fprintf(stderr,"mpno = %d  mpindex=%d\n",(gui_mpmax+cur_index-1)->mpno,cur_index);
		}
		delay(100);
	}
	gui_mp_free(gui_mpmax);//跳入轮显后释放？？？
}
void setmeterpara_js(int pindex)
{
	int daleihao=0,xiaoleihao=0,tmp=0, f10_flg=1,cld_isvalid=0;
	INT8U cur_page=1;//当前页数
	para_1mp para_f10;
	para_F150 para_f150;
#ifdef SPTF_III
	para_F29 para_f29;
#endif
	int form_cldno=0;//通过控件修改的测量点地址
	char first_flg=0;
	struct list *cur1_node=NULL, *tmp1node=NULL;
	struct list *cur2_node=NULL, *tmp2node=NULL;
	Form *cur1_form=NULL, client1;//client 液晶显示客户区page1
	Form *cur2_form=NULL,client2;//液晶显示客户区page2
	char cb_text[TEXTLEN_X][TEXTLEN_Y];//用于存放combox的元素
	char str[INPUTKEYNUM];
	Rect rect;
	int tmpindex,tem_cldvalid;
	//测量点号、表地址、端口、规约、通信速率、大小类号、采集器地址
	Edit edit_cldindex,edit_cldno, edit_cldaddr, edit_cjqaddr;
	Combox cb_port,cb_gueyue,cb_baud,cb_dalei,cb_xiaolei,cb_isvalid;
	memset(&edit_cldno, 0, sizeof(Edit));
	memset(&edit_cldaddr, 0, sizeof(Edit));
	memset(&cb_port, 0, sizeof(Combox));
	memset(&cb_gueyue, 0, sizeof(Combox));
	memset(&cb_baud, 0, sizeof(Combox));
	memset(&cb_dalei, 0, sizeof(Combox));
	memset(&cb_xiaolei, 0, sizeof(Combox));
	memset(&cb_isvalid, 0, sizeof(Combox));
	memset(&edit_cjqaddr, 0, sizeof(Edit));
	fprintf(stderr,"\npindex=%d",pindex);
	if(pindex<1)
		 return;
	g_curcldno = ParaAll->f10.para_mp[pindex-1].MPNo;//cldno;
	memset(&client1, 0, sizeof(Form));
	memset(&client2, 0, sizeof(Form));
	client1.node.child = (struct list*)malloc(sizeof(struct list));
	client2.node.child = (struct list*)malloc(sizeof(struct list));
	if(client1.node.child==NULL || client2.node.child==NULL){
		g_curcldno = 1;//上状态栏显示
		return;
	}
	memset(client1.node.child, 0, sizeof(struct list));
	memset(client2.node.child, 0, sizeof(struct list));
	gui_clrrect(rect_Client);
	//往控件里放入测量点数据 应该在控件init里设置
	Point pos;

	pos.x = rect_Client.left+FONTSIZE*9;
	pos.y = rect_Client.top;
	pos.y += ROW_INTERVAL+FONTSIZE*3;
	memset(str, 0, INPUTKEYNUM);
	sprintf(str,"%04d", ParaAll->f10.para_mp[pindex-1].Index);
	edit_init(&edit_cldindex, str, 4, pos, 0, 0, client1.node.child,KEYBOARD_DEC);//测量点序号
	//------------------------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	sprintf(str,"%04d", ParaAll->f10.para_mp[pindex-1].MPNo);
	edit_init(&edit_cldno, str, 4, pos, 0, 0, client1.node.child,KEYBOARD_DEC);//测量点号
	//------------------------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	memcpy(str, ParaAll->f10.para_mp[pindex-1].addr, 12);
	edit_init(&edit_cldaddr, str, 12, pos, 0, 0, client1.node.child,KEYBOARD_HEX);//表地址
	//------------------------------------------
	setmp_cbtext_baud(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	if(ParaAll->f10.para_mp[pindex-1].BaudRate==1200)
		tmpindex = 0;
	else
		tmpindex = 1;
	combox_init(&cb_baud, tmpindex, cb_text, pos, 0,client1.node.child);//通信速率
	//------------------------------------------
	setmp_cbtext_port(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	combox_init(&cb_port, port2index(ParaAll->f10.para_mp[pindex-1].Port), cb_text, pos, 0,client1.node.child);//端口
	//------------------------------------------
	setmp_cbtext_guiyue(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	if(ParaAll->f10.para_mp[pindex-1].Protocol==PROTOCOL_97)
		tmpindex = 0;
	else if(ParaAll->f10.para_mp[pindex-1].Protocol==PROTOCOL_07)
		tmpindex = 1;
	else if(ParaAll->f10.para_mp[pindex-1].Protocol==PROTOCOL_JC)
		tmpindex = 2;
	else
		tmpindex = 1;
	combox_init(&cb_gueyue, tmpindex, cb_text, pos, 0,client1.node.child);//规约
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	memcpy(str, ParaAll->f10.para_mp[pindex-1].colladdr, 12);
	edit_init(&edit_cjqaddr, str, 12, pos, 0, 0, client1.node.child,KEYBOARD_DEC);//采集器地址
	//------------------------------------------
	setmp_cbtext_dalei(cb_text);
	pos.x = rect_Client.left+FONTSIZE*9; //重新放置控件
	pos.y = rect_Client.top;
	pos.y += ROW_INTERVAL+FONTSIZE*3;
	daleihao = ParaAll->f10.para_mp[pindex-1].TypeofBigUser;
	if(daleihao<0 || daleihao>16)
		daleihao = 0;
	//daleihao = 5;
	combox_init(&cb_dalei, daleihao, cb_text, pos, 0,client2.node.child);//第二页大类
	//------------------------------------------
	setmp_cbtext_xiaolei(daleihao, cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	xiaoleihao = ParaAll->f10.para_mp[pindex-1].TypeOfLitUser;
	if(xiaoleihao<0 || xiaoleihao>16)
		xiaoleihao = 1;
	combox_init(&cb_xiaolei, xiaoleihao, cb_text, pos, 0, client2.node.child);//第二页小类
	//------------------------------------------
	setmp_cbtext_isvalid(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	cld_isvalid = ParaAll->f150.Flag_MPValid[pindex-1];
	if(cld_isvalid<0 || cld_isvalid>1)
		cld_isvalid = 0;
	combox_init(&cb_isvalid, cld_isvalid, cb_text, pos, 0, client2.node.child);//第二页测量点是否投抄
	tem_cldvalid=cb_isvalid.cur_index;
	//------------------------------------------
	cur1_node = &edit_cldindex.form.node;
	cur1_form = &edit_cldindex.form;
	cur2_node = &cb_dalei.form.node;
	cur2_form = &cb_dalei.form;

	setmp_showlabel1(client1.node.child, cur1_node);//显示各个控件的标签(首先显示第一页)
	PressKey = NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		switch(PressKey)
		{
		case LEFT:
			if(cur_page==2)
				cur_page=1;
			fprintf(stderr,"left cur_page == %d\n",cur_page);
			break;
		case UP:
			if(cur_page==1)
			{
				cur1_form->focus = NOFOCUS;
				cur1_node=list_getprev(cur1_node);
				if(cur1_node==client1.node.child)
					cur1_node = list_getlast(client1.node.child);
			}
			else
			{
				cur2_form->focus = NOFOCUS;
				cur2_node=list_getprev(cur2_node);
				if(cur2_node==client2.node.child)
					cur2_node = list_getlast(client2.node.child);
			}
			break;
		case RIGHT:
			if(cur_page==1)
				cur_page=2;
			fprintf(stderr,"right cur_page == %d\n",cur_page);
			break;
		case DOWN:
			if(cur_page==1)
			{
				cur1_form->focus = NOFOCUS;
				if(list_getnext(cur1_node)==NULL){
					cur1_node = list_getfirst(cur1_node);
					cur1_node = cur1_node->next;
				}else
					cur1_node = list_getnext(cur1_node);
			}
			else
			{
				cur2_form->focus = NOFOCUS;
				if(list_getnext(cur2_node)==NULL){
					cur2_node = list_getfirst(cur2_node);
					cur2_node = cur2_node->next;
				}else
					cur2_node = list_getnext(cur2_node);
			}
			break;
		case OK:
			if(get_oprmode()==OPRMODE_MODIFY){
				if(cur_page==2)
				{
					fprintf(stderr,"\npfun_process_ret=%d",cur2_form->pfun_process_ret);
					cur2_form->pfun_process(cur2_form);
						if(cur2_form==&cb_dalei.form){
						setmp_cbtext_xiaolei(cb_dalei.cur_index, cb_text);
						xiaoleihao = 1;
						memcpy(cb_xiaolei.text, cb_text, TEXTLEN_X*TEXTLEN_Y);
						cb_xiaolei.form.pfun_show(&cb_xiaolei);
					}
				}
				else
				{
					fprintf(stderr,"\npfun_process_ret=%d",cur1_form->pfun_process_ret);
					cur1_form->pfun_process(cur1_form);
				}
				if(cur1_form->pfun_process_ret==OK || cur2_form->pfun_process_ret){
					//cur_node = list_getfirst(cur_node);
					if(msgbox_label((char *)"保存参数?", CTRL_BUTTON_OK)==ACK){
//						dbg_prt("\n msgbox_ret = OK  save para here!!!!");
						memcpy(&para_f10, &ParaAll->f10.para_mp[pindex-1], sizeof(para_1mp));//sizeof(para_F10));
						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_cldno, str);
						form_cldno = atoi(str);
						para_f10.MPNo = form_cldno;
						if(form_cldno>2048||form_cldno<=0)
							f10_flg = 0;
						eidt_gettext(&edit_cldaddr, (char*)para_f10.addr);
						para_f10.Port = index2port(cb_port.cur_index);
						if(isValidMeterAddr_12((char*)para_f10.addr)==0)
							f10_flg = 0;

						if(cb_gueyue.cur_index==0)
							para_f10.Protocol = PROTOCOL_97;
						else if(cb_gueyue.cur_index==1)
							para_f10.Protocol = PROTOCOL_07;
						else
							para_f10.Protocol = PROTOCOL_JC;
						if(cb_baud.cur_index==0)
							para_f10.BaudRate = 1200;
						else if(cb_baud.cur_index==1)
							para_f10.BaudRate = 2400;
						para_f10.TypeofBigUser = cb_dalei.cur_index;
						para_f10.TypeOfLitUser = getxiaoleibycb(cb_xiaolei.cur_index,cb_dalei.cur_index);//cb_xiaolei.cur_index;
		#ifdef CCTT_I
						eidt_gettext(&edit_cjqaddr, (char*)para_f10.colladdr);
						if(isValidMeterAddr_12((char*)para_f10.colladdr)==0)
							f10_flg = 0;
		#endif
		#ifdef SPTF_III
						if(ParaAll->f10.para_mp[pindex-1].MPNo >0)
							memcpy(&para_f29, &ParaAll->f29s.f29[ParaAll->f10.para_mp[pindex-1].MPNo-1], sizeof(para_F29));
						eidt_gettext(&edit_cjqaddr, (char*)para_f29.MeterDisplayNo);
//						dbg_prt( "\n para_f29.MeterDisplayNo=%s---1", para_f29.MeterDisplayNo);
						setpara(&para_f29, ParaAll->f10.para_mp[pindex-1].MPNo, 29, sizeof(para_F29));
		#endif
						if(f10_flg==1){
							setpara_f10(&para_f10,10);
							if(tem_cldvalid != cb_isvalid.cur_index)//是否投放有变化
							{
								memset(&para_f150,0,sizeof(para_f150));
								memcpy(&para_f150,&ParaAll->f150,sizeof(para_f150));
								if(cb_isvalid.cur_index==1)
									para_f150.Flag_MPValid[pindex-1]=1;
								else
									para_f150.Flag_MPValid[pindex-1]=0;
								setpara(&para_f150, 0, 150, sizeof(para_F150));
							}
						}else
							msgbox_label((char *)"保存失败！", CTRL_BUTTON_CANCEL);
					}else
//						dbg_prt("\n msgbox_ret = CANCEL  do not save***");
					g_curcldno = 1;
				}
			}
			break;
		case ESC:
			if(client1.node.child!=NULL)
				free(client1.node.child);
			if(client2.node.child!=NULL)
				free(client2.node.child);
			return;
		}
		if(PressKey!=NOKEY||first_flg==0){
			gui_clrrect(rect_Client);
			if(cur_page==1)
			{
				tmp1node = client1.node.child;
				tmp = setmp_showlabel1(client1.node.child, cur1_node);
				while(tmp1node->next!=NULL){
					tmp1node = tmp1node->next;
					cur1_form = list_entry(tmp1node, Form, node);
					if(tmp==1){
						if(list_getListIndex(client1.node.child, tmp1node)>=list_getListIndex(client1.node.child, cur1_node))
							cur1_form->pfun_show(cur1_form);
					}else
						cur1_form->pfun_show(cur1_form);
				}
				cur1_form = list_entry(cur1_node, Form, node);//根据链表节点找到控件指针
				cur1_form->focus = FOCUS;
//				dbg_prt("\n cur_form:(%d)", cur1_form->CtlType);
				memcpy(&rect, &cur1_form->rect, sizeof(Rect));
			}
			else
			{
				tmp2node = client2.node.child;
				tmp = setmp_showlabel2(client2.node.child, cur2_node);
				while(tmp2node->next!=NULL){
					tmp2node = tmp2node->next;
					cur2_form = list_entry(tmp2node, Form, node);
					if(tmp==1){
						if(list_getListIndex(client2.node.child, tmp2node)>=list_getListIndex(client2.node.child, cur2_node))
							cur2_form->pfun_show(cur2_form);
					}else
						cur2_form->pfun_show(cur2_form);
				}
				cur2_form = list_entry(cur2_node, Form, node);//根据链表节点找到控件指针
				cur2_form->focus = FOCUS;
//				dbg_prt("\n cur_form:(%d)", cur2_form->CtlType);
				memcpy(&rect, &cur2_form->rect, sizeof(Rect));
			}
			rect.left += 2;
			rect.right -= 3;
			rect.top -= 1;
			gui_rectangle(gui_changerect(gui_moverect(rect, DOWN, 4), 4));
			first_flg = 1;
		}
		PressKey = NOKEY;
		delay(100);
	}
	if(client1.node.child!=NULL)
		free(client1.node.child);
	if(client2.node.child!=NULL)
		free(client2.node.child);
}
void querymeterpara_js(int pindex)
{
	int daleihao=0,xiaoleihao=0,tmp=0, f10_flg=1;
		para_1mp para_f10;
	#ifdef SPTF_III
		para_F29 para_f29;
	#endif
	int form_cldno=0;//通过控件修改的测量点地址
	char first_flg=0;
	struct list *cur_node=NULL, *tmpnode=NULL;
	Form *cur_form=NULL, client;//client 液晶显示客户区
	char cb_text[TEXTLEN_X][TEXTLEN_Y];//用于存放combox的元素
	char str[INPUTKEYNUM];
	Rect rect;
	int tmpindex;
	//测量点号、表地址、端口、规约、通信速率、大小类号、采集器地址
	Edit edit_cldno, edit_cldaddr, edit_cjqaddr;
	Combox cb_port,cb_gueyue,cb_baud,cb_dalei,cb_xiaolei;
	memset(&edit_cldno, 0, sizeof(Edit));
	memset(&edit_cldaddr, 0, sizeof(Edit));
	memset(&cb_port, 0, sizeof(Combox));
	memset(&cb_gueyue, 0, sizeof(Combox));
	memset(&cb_baud, 0, sizeof(Combox));
	memset(&cb_dalei, 0, sizeof(Combox));
	memset(&cb_xiaolei, 0, sizeof(Combox));
	memset(&edit_cjqaddr, 0, sizeof(Edit));
	fprintf(stderr,"\npindex=%d",pindex);
	if(pindex<1)
		 return;
	g_curcldno = ParaAll->f10.para_mp[pindex-1].MPNo;//cldno;
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
	pos.x = rect_Client.left+FONTSIZE*9;
	pos.y = rect_Client.top+2*FONTSIZE + 2;
//	dbg_prt("\nclient.node.child=%x",(int)client.node.child);
	memset(str, 0, INPUTKEYNUM);
	sprintf(str,"%04d", ParaAll->f10.para_mp[pindex-1].MPNo);
	edit_init(&edit_cldno, str, 4, pos, 0, 0, client.node.child,KEYBOARD_DEC);//测量点号
	//------------------------------------------------------------
	pos.y += FONTSIZE*2 + 2;
	memset(str, 0, INPUTKEYNUM);
	memcpy(str, ParaAll->f10.para_mp[pindex-1].addr, 12);
	edit_init(&edit_cldaddr, str, 12, pos, 0, 0, client.node.child,KEYBOARD_HEX);//表地址
	//------------------------------------------
	setmp_cbtext_baud(cb_text);
	pos.y += FONTSIZE*2 + 2;
	if(ParaAll->f10.para_mp[pindex-1].BaudRate==1200)
		tmpindex = 0;
	else
		tmpindex = 1;
	combox_init(&cb_baud, tmpindex, cb_text, pos, 0,client.node.child);//通信速率
	//-----------------------------------------------
	setmp_cbtext_port(cb_text);
	pos.y += FONTSIZE*2 + 2;
	combox_init(&cb_port, port2index(ParaAll->f10.para_mp[pindex-1].Port), cb_text, pos, 0,client.node.child);//端口
	//------------------------------------------
	setmp_cbtext_guiyue(cb_text);
	pos.y += FONTSIZE*2 + 2;
	if(ParaAll->f10.para_mp[pindex-1].Protocol==PROTOCOL_97)
		tmpindex = 0;
	else if(ParaAll->f10.para_mp[pindex-1].Protocol==PROTOCOL_07)
		tmpindex = 1;
	else if(ParaAll->f10.para_mp[pindex-1].Protocol==PROTOCOL_JC)
		tmpindex = 2;
	else
		tmpindex = 1;
	combox_init(&cb_gueyue, tmpindex, cb_text, pos, 0,client.node.child);//规约
	//------------------------------------------
	pos.y += FONTSIZE*2 + 2;
	memset(str, 0, INPUTKEYNUM);
#ifdef CCTT_I
	memcpy(str, ParaAll->f10.para_mp[pindex-1].colladdr, 12);
	edit_init(&edit_cjqaddr, str, 12, pos, 0, 0, client.node.child,KEYBOARD_DEC);//采集器地址
#endif
#ifdef SPTF_III
	if(ParaAll->f10.para_mp[pindex-1].MPNo >0)
	{
	memcpy(str, ParaAll->f29s.f29[ParaAll->f10.para_mp[pindex-1].MPNo-1].MeterDisplayNo, 12);
//	dbg_prt( "\n para_f29.MeterDisplayNo=%s---0",
//			ParaAll->f29s.f29[ParaAll->f10.para_mp[pindex-1].MPNo-1].MeterDisplayNo);
	}
	edit_init(&edit_cjqaddr, str, 12, pos, 0, 0, client.node.child,KEYBOARD_ASC);//局编号
#endif
	//------------------------------------------
	setmp_cbtext_dalei(cb_text);
	pos.y += FONTSIZE*2 + 2;
	daleihao = ParaAll->f10.para_mp[pindex-1].TypeofBigUser;
	if(daleihao<0 || daleihao>16)
		daleihao = 0;
	//daleihao = 5;
	combox_init(&cb_dalei, daleihao, cb_text, pos, 0,client.node.child);//大类
	//------------------------------------------
	setmp_cbtext_xiaolei(daleihao, cb_text);
	pos.y += FONTSIZE*2 + 2;
	xiaoleihao = ParaAll->f10.para_mp[pindex-1].TypeOfLitUser;
	if(xiaoleihao<0 || xiaoleihao>16)
		xiaoleihao = 1;
	//xiaoleihao = 1;
	combox_init(&cb_xiaolei, xiaoleihao, cb_text, pos, 0, client.node.child);//小类
	//------------------------------------------
	cur_node = &edit_cldno.form.node;
	cur_form = &edit_cldno.form;
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
				fprintf(stderr,"\npfun_process_ret=%d",cur_form->pfun_process_ret);
				cur_form->pfun_process(cur_form);
				if(cur_form==&cb_dalei.form){
					setmp_cbtext_xiaolei(cb_dalei.cur_index, cb_text);
					xiaoleihao = 1;
					memcpy(cb_xiaolei.text, cb_text, TEXTLEN_X*TEXTLEN_Y);
					cb_xiaolei.form.pfun_show(&cb_xiaolei);
				}
				if(cur_form->pfun_process_ret==OK){
					//cur_node = list_getfirst(cur_node);
					if(msgbox_label((char *)"保存参数?", CTRL_BUTTON_OK)==ACK){
//						dbg_prt("\n msgbox_ret = OK  save para here!!!!");
						memcpy(&para_f10, &ParaAll->f10.para_mp[pindex-1], sizeof(para_1mp));//sizeof(para_F10));
						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_cldno, str);
						form_cldno = atoi(str);
						para_f10.MPNo = form_cldno;
						if(form_cldno>2048||form_cldno<=0)
							f10_flg = 0;
						eidt_gettext(&edit_cldaddr, (char*)para_f10.addr);
						para_f10.Port = index2port(cb_port.cur_index);
						if(isValidMeterAddr_12((char*)para_f10.addr)==0)
							f10_flg = 0;

						if(cb_gueyue.cur_index==0)
							para_f10.Protocol = PROTOCOL_97;
						else if(cb_gueyue.cur_index==1)
							para_f10.Protocol = PROTOCOL_07;
						else
							para_f10.Protocol = PROTOCOL_JC;
						if(cb_baud.cur_index==0)
							para_f10.BaudRate = 1200;
						else if(cb_baud.cur_index==1)
							para_f10.BaudRate = 2400;
						para_f10.TypeofBigUser = cb_dalei.cur_index;
						para_f10.TypeOfLitUser = getxiaoleibycb(cb_xiaolei.cur_index,cb_dalei.cur_index);//cb_xiaolei.cur_index;
		#ifdef CCTT_I
						eidt_gettext(&edit_cjqaddr, (char*)para_f10.colladdr);
						if(isValidMeterAddr_12((char*)para_f10.colladdr)==0)
							f10_flg = 0;
		#endif
		#ifdef SPTF_III
						if(ParaAll->f10.para_mp[pindex-1].MPNo >0)
							memcpy(&para_f29, &ParaAll->f29s.f29[ParaAll->f10.para_mp[pindex-1].MPNo-1], sizeof(para_F29));
						eidt_gettext(&edit_cjqaddr, (char*)para_f29.MeterDisplayNo);
//						dbg_prt( "\n para_f29.MeterDisplayNo=%s---1", para_f29.MeterDisplayNo);
						setpara(&para_f29, ParaAll->f10.para_mp[pindex-1].MPNo, 29, sizeof(para_F29));
		#endif
						if(f10_flg==1){
							setpara_f10(&para_f10,10);
						}else
							msgbox_label((char *)"保存失败！", CTRL_BUTTON_CANCEL);
					}else
//						dbg_prt("\n msgbox_ret = CANCEL  do not save***");
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
			tmp = querymp_showlabel_js(client.node.child, cur_node);
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
		//	gui_rectangle(gui_changerect(gui_moverect(rect, DOWN, 4), 4));
			first_flg = 1;
		}
		PressKey = NOKEY;
		delay(100);
	}
	if(client.node.child!=NULL)
		free(client.node.child);
}
int querymp_showlabel_js(struct list *head, struct list *node){
	int ret=0;
	//int npos=0;
	Point label_pos;
	gui_setpos(&label_pos, rect_Client.left+8*FONTSIZE, rect_Client.top);
	gui_textshow((char *)"电能表参数", label_pos, LCD_NOREV);
	label_pos.x = rect_Client.left;
	label_pos.y = rect_Client.top+2*FONTSIZE + 2;
	gui_textshow((char *)"测量点号  ", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + 2;
	gui_textshow((char *)"电表地址  ", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + 2;
	gui_textshow((char *)"通讯速率  ", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + 2;
	gui_textshow((char *)"通讯端口  ", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + 2;
	gui_textshow((char *)"通讯协议  ", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + 2;
#ifdef CCTT_I
	gui_textshow((char *)"采 集 器", label_pos, LCD_NOREV);
#endif
#ifdef SPTF_III
	gui_textshow((char *)"局编号  ", label_pos, LCD_NOREV);
#endif
	label_pos.y += FONTSIZE*2 + 2;
	gui_textshow((char *)"用户大类  ", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + 2;
	gui_textshow((char *)"用户小类  ", label_pos, LCD_NOREV);
	set_time_show_flag(1);
	return ret;
}
void USB_DataCopy()
{
	Point label_pos;
	INT8U ret=0;
	gui_clrrect(rect_Client);
	setFontSize(16);
	gui_setpos(&label_pos, rect_Client.left+8*FONTSIZE, rect_Client.top+6*FONTSIZE);
	gui_textshow((char *)"请插入U盘", label_pos, LCD_NOREV);
	set_time_show_flag(1);
	setFontSize(12);
	PressKey=NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		switch(PressKey)
		{
		case ESC:
			return;
		default:
			break;
		}
		if(access("/dos/gwncp",0) == 0)
		{
			fprintf(stderr,"gwncp ext >>>\n");
			ret=system("cp -rf /nand/* /dos/gwncp/");
			if(ret==0)
			{
				gui_clrrect(rect_Client);
				setFontSize(16);
				gui_setpos(&label_pos, rect_Client.left+8*FONTSIZE, rect_Client.top+6*FONTSIZE);
				gui_textshow((char *)"拷贝完成！", label_pos, LCD_NOREV);
				setFontSize(12);
			}
			else
			{
				gui_clrrect(rect_Client);
				setFontSize(16);
				gui_setpos(&label_pos, rect_Client.left+8*FONTSIZE, rect_Client.top+6*FONTSIZE);
				gui_textshow((char *)"拷贝失败！", label_pos, LCD_NOREV);
				setFontSize(12);
			}
			set_time_show_flag(1);
			sleep(3);
			break;
		}
		delay(100);
	}
}
void USB_UpdateSoft()
{
	Point label_pos;
	gui_clrrect(rect_Client);
	setFontSize(16);
	gui_setpos(&label_pos, rect_Client.left+8*FONTSIZE, rect_Client.top+6*FONTSIZE);
	gui_textshow((char *)"请插入U盘", label_pos, LCD_NOREV);
	set_time_show_flag(1);
	setFontSize(12);
	PressKey=NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		switch(PressKey)
		{
		case ESC:
			return;
		default:
			break;
		}
		delay(100);
	}
}
void Term_StartConn()
{
	Point label_pos;
	gui_clrrect(rect_Client);
	setFontSize(16);
	gui_setpos(&label_pos, rect_Client.left+4*FONTSIZE, rect_Client.top+6*FONTSIZE);
	sleep(2);
	gui_textshow((char *)"激活连接成功!", label_pos, LCD_NOREV);
	set_time_show_flag(1);
	setFontSize(12);
	PressKey=NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		switch(PressKey)
		{
		case ESC:
			return;
		default:
			break;
		}
		delay(100);
	}
}
void Term_StopConn()
{
	Point label_pos;
	gui_clrrect(rect_Client);
	setFontSize(16);
	sleep(2);
	gui_setpos(&label_pos, rect_Client.left+4*FONTSIZE, rect_Client.top+6*FONTSIZE);
	gui_textshow((char *)"断开连接成功!", label_pos, LCD_NOREV);
	set_time_show_flag(1);
	setFontSize(12);
	PressKey=NOKEY;
	while(g_LcdPoll_Flag==LCD_NOTPOLL){
		switch(PressKey)
		{
		case ESC:
			return;
		default:
			break;
		}
		delay(100);
	}
}
int setmp_showlabel1(struct list *head, struct list *node){ //page1
	int ret=0;
	//int npos=0;
	Point label_pos;
	label_pos.x = rect_Client.left + 5*FONTSIZE;
	label_pos.y = rect_Client.top + ROW_INTERVAL;
	setFontSize(16);
	gui_textshow((char *)"电能表参数", label_pos, LCD_NOREV);
	setFontSize(12);
	label_pos.x = rect_Client.left;
	label_pos.y += 3*FONTSIZE;
	gui_textshow((char *)"电表序号 ", label_pos, LCD_NOREV);
	label_pos.x = rect_Client.left;
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"测量点号", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"电表地址", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"通信速率", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"通信端口", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"通信协议", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"采 集 器", label_pos, LCD_NOREV);
	set_time_show_flag(1);
	return ret;
}
int setmp_showlabel2(struct list *head, struct list *node){ //page2
	int ret=0;
	//int npos=0;
	Point label_pos;
	label_pos.x = rect_Client.left + 5*FONTSIZE;
	label_pos.y = rect_Client.top + ROW_INTERVAL;
	setFontSize(16);
	gui_textshow((char *)"电能表参数", label_pos, LCD_NOREV);
	setFontSize(12);
	label_pos.y += 3*FONTSIZE;
	label_pos.x = rect_Client.left;
	gui_textshow((char *)"用户大类", label_pos, LCD_NOREV);
	label_pos.x = rect_Client.left;
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"用户小类", label_pos, LCD_NOREV);
	label_pos.y += FONTSIZE*2 + ROW_INTERVAL;
	gui_textshow((char *)"是否投抄", label_pos, LCD_NOREV);
	set_time_show_flag(1);
	return ret;
}
static void setmp_cbtext_tcpudp(char cb_text[][TEXTLEN_Y]){
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "TCP", strlen("TCP"));
	memcpy(cb_text[1], "UDP", strlen("UDP"));
}
static void setmp_cbtext_termod(char cb_text[][TEXTLEN_Y]){
	memset(cb_text, 0, TEXTLEN_X*TEXTLEN_Y);
	memcpy(cb_text[0], "混合模式", strlen("混合模式"));
	memcpy(cb_text[1], "客户机模式", strlen("客户机模式"));
	memcpy(cb_text[2], "服务器模式", strlen("服务器模式"));
}
void menu_eth0para_js(){
	char cmd[50],cmd1[100], input_valid=1;
	INT8U  sip[16];
	int tmp=0;
	para_F7 para_f7;
	para_F7 para_f7_old;
	para_F8 para_f8;
	char first_flg=0;
	INT8U gway_flag=0;
	INT8U TCPorUDP,TER_MOD;
	struct list *cur_node=NULL, *tmpnode=NULL;
	Form *cur_form=NULL, client;//client 液晶显示客户区
	char str[INPUTKEYNUM];
	Rect rect;
	Edit edit_termip, edit_netmask, edit_gateway, edit_listenport;
	Combox cb_TCPorUDP,cb_termod;
	char cb_text[TEXTLEN_X][TEXTLEN_Y];//用于存放combox的元素
	memset(&edit_termip, 0, sizeof(Edit));
	memset(&edit_netmask, 0, sizeof(Edit));
	memset(&edit_gateway, 0, sizeof(Edit));
	memset(&edit_listenport, 0, sizeof(Edit));
	memset(&cb_TCPorUDP,0,sizeof(cb_TCPorUDP));
	memset(&cb_termod,0,sizeof(cb_termod));
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
	memcpy(&para_f7, &ParaAll->f7, sizeof(para_F7));
	memcpy(&para_f7_old, &ParaAll->f7, sizeof(para_F7));
	memcpy(&para_f8, &ParaAll->f8, sizeof(para_F8));
	gui_setpos(&pos, rect_Client.left+10.5*FONTSIZE, rect_Client.top);
	pos.y += FONTSIZE*5 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	//memset(ip, 0, 4);
	//getlocalip(ip);
	ip2asc((char*)ParaAll->f7.IP_Addr, str);
	editip_init(&edit_termip, CTRL_EDITIP, str, 12, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//主IP
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	ip2asc((char*)ParaAll->f7.IP_SubNet, str);
	editip_init(&edit_netmask, CTRL_EDITIP, str, 12, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//掩码
	//-----------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	ip2asc((char*)ParaAll->f7.IP_GateWay, str);
//	dbg_prt("\n f7.IP_GateWay=%d %d %d %d  str=%s",ParaAll->f7.IP_GateWay[0],ParaAll->f7.IP_GateWay[1],
//			ParaAll->f7.IP_GateWay[2],ParaAll->f7.IP_GateWay[3], str);
	editip_init(&edit_gateway, CTRL_EDITIP, str, 12, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//网关
	//------------------------------------------
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	memset(str, 0, INPUTKEYNUM);
	sprintf(str, "%04d", ParaAll->f7.Port_Listen);
	edit_init(&edit_listenport, str, 4, pos, NOFRAME, NORETFLG, client.node.child,KEYBOARD_DEC);//备端口
	//-------------------------------------------
	setmp_cbtext_tcpudp(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	TCPorUDP= ParaAll->f8.TCPorUDP;
	if(TCPorUDP>1 || TCPorUDP<0)
		TCPorUDP=0;
	combox_init(&cb_TCPorUDP, TCPorUDP, cb_text, pos, 0, client.node.child);//TCPorUDP
	//	------------------------------------------
	setmp_cbtext_termod(cb_text);
	pos.y += FONTSIZE*2 + ROW_INTERVAL;
	TER_MOD=ParaAll->f8.Ter_mod;
	if(TER_MOD<0 ||TER_MOD>3)
		TER_MOD=1;
	combox_init(&cb_termod, TER_MOD, cb_text, pos, 0, client.node.child);//工作模式，混合/服务器/客户端
	//--------------------------------------------
	cur_node = &edit_termip.form.node;
	cur_form = &edit_termip.form;
	seteth0_showlabel();//显示各个控件的标签
	set_time_show_flag(1);
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
//						dbg_prt("\n msgbox_ret = OK  save para here!!!!");
						memcpy(&para_f7, &ParaAll->f7, sizeof(para_F7));//将共享内存f7拷贝到本地作为
						memcpy(&para_f8, &ParaAll->f8, sizeof(para_F8));
						memset(para_f7.IP_Addr, 0, 4);
						memset(sip, 0, 16);
						eidtip12_gettext(&edit_termip, (char*)sip);//取得编辑框中的ip值
						if(ip_strtobyte(sip, para_f7.IP_Addr)==0)
							input_valid = 0;
						else{
						if((para_f7.IP_Addr[0]!=para_f7_old.IP_Addr[0])||(para_f7.IP_Addr[1]!=para_f7_old.IP_Addr[1])||
								(para_f7.IP_Addr[2]!=para_f7_old.IP_Addr[2])||(para_f7.IP_Addr[3]!=para_f7_old.IP_Addr[3]))//ip地址改变，则重新设置网关
							gway_flag=1;
							memset(cmd, 0, 50);
							sprintf(cmd, "ifconfig eth0 %d.%d.%d.%d up", para_f7.IP_Addr[0],para_f7.IP_Addr[1],
									para_f7.IP_Addr[2],para_f7.IP_Addr[3]);
							system(cmd);
							memset(cmd1, 0, 100);
							sprintf(cmd1, "echo %s > /nor/rc.d/ip.sh", cmd);
							system(cmd1);
						}
						memset(para_f7.IP_SubNet, 0, 4);
						memset(sip, 0, 16);
						eidtip12_gettext(&edit_netmask, (char*)sip);
						if(ip_strtobyte(sip, para_f7.IP_SubNet)==0)
							input_valid = 0;
						memset(para_f7.IP_GateWay, 0, 4);
						memset(sip, 0, 16);
						eidtip12_gettext(&edit_gateway, (char*)sip);
						if(ip_strtobyte(sip, para_f7.IP_GateWay)==0)
							input_valid = 0;
						if(gway_flag==1)
						{
							memcpy(para_f7.IP_GateWay,para_f7.IP_Addr,4);
							para_f7.IP_GateWay[3]--;
						}
						memset(&para_f7.Port_Listen, 0, 2);
						memset(str, 0, INPUTKEYNUM);
						eidt_gettext(&edit_listenport, str);
						para_f7.Port_Listen = atoi(str);
						para_f8.TCPorUDP = cb_TCPorUDP.cur_index;
						para_f8.Ter_mod = cb_termod.cur_index;
						if(input_valid==1)
						{
							setpara(&para_f7, 0, 7, sizeof(para_F7));
							setpara(&para_f8, 0, 8, sizeof(para_F8));
							gui_clrrect(rect_Client);
							memset(str, 0, 50);
							sprintf(str, "正在保存修改(等待6秒)...");
							gui_setpos(&pos, rect_Client.left+2*FONTSIZE, rect_Client.top+8*FONTSIZE);
							gui_textshow(str, pos, LCD_NOREV);
							set_time_show_flag(1);
							sleep(6);
						}
						else{
							msgbox_label((char *)"保存失败！", CTRL_BUTTON_OK);
							set_time_show_flag(1);
						}
					}else
//						dbg_prt("\n msgbox_ret = CANCEL  do not save***");
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
			while(tmpnode->next!=NULL){//从开始节点显示到最后
				tmpnode = tmpnode->next;
				cur_form = list_entry(tmpnode, Form, node);
				//dbg_prt("\n ctltype = %d", cur_form->CtlType);
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
			set_time_show_flag(1);
			first_flg = 1;
		}
		PressKey = NOKEY;
		delay(100);
	}
	if(client.node.child!=NULL)
		free(client.node.child);
	return;
}
#endif
