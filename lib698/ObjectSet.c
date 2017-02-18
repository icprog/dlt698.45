/*
 * ObjectSet.c
 *
 *  Created on: Nov 12, 2016
 *      Author: ava
 */

#include <string.h>
#include <stdio.h>
#include <malloc.h>

#include "AccessFun.h"
#include "StdDataType.h"
#include "Objectdef.h"
#include "EventObject.h"
#include "PublicFunction.h"

extern void get_BasicUnit(INT8U *source,INT16U *sourceindex,INT8U *dest,INT16U *destindex);
extern ProgramInfo *memp;

INT8U prtstat(int flg)
{
	if (flg == 1) {
		fprintf(stderr,"\n保存成功");
		return success;
	}else {
		fprintf(stderr,"\n保存失败");
		return refuse_rw;
	}
}

/*参数文件修改，改变共享内存的标记值，通知相关进程，参数有改变
 * */
void setOIChange(OI_698 oi)
{
	switch(oi) {
	case 0x3100:  	memp->oi_changed.oi3100++; 	break;
	case 0x3101:  	memp->oi_changed.oi3101++; 	break;
	case 0x3014:	memp->oi_changed.oi3104++; 	break;
	case 0x3105:	memp->oi_changed.oi3105++; 	break;
	case 0x3106:	memp->oi_changed.oi3106++; 	break;
	case 0x3107:	memp->oi_changed.oi3107++; 	break;
	case 0x3108:	memp->oi_changed.oi3108++; 	break;
	case 0x3109: 	memp->oi_changed.oi3109++; 	break;
	case 0x310A:	memp->oi_changed.oi310A++; 	break;
	case 0x310B:	memp->oi_changed.oi310B++; 	break;
	case 0x310C:	memp->oi_changed.oi310C++; 	break;
	case 0x310D:	memp->oi_changed.oi310D++; 	break;
	case 0x310E:	memp->oi_changed.oi310E++; 	break;
	case 0x310F:	memp->oi_changed.oi310F++; 	break;
	case 0x3110:	memp->oi_changed.oi3110++;	break;
	case 0x3111:	memp->oi_changed.oi3111++;	break;
	case 0x3112:	memp->oi_changed.oi3112++;  break;
	case 0x3114:	memp->oi_changed.oi3114++; 	break;
	case 0x3115:	memp->oi_changed.oi3105++; 	break;
	case 0x3116:	memp->oi_changed.oi3116++;	break;
	case 0x3117:	memp->oi_changed.oi3117++;	break;
	case 0x3118:	memp->oi_changed.oi3118++;	break;
	case 0x3119:	memp->oi_changed.oi3119++;	break;
	case 0x311A:	memp->oi_changed.oi311A++;	break;
	case 0x311B:	memp->oi_changed.oi311B++;	break;
	case 0x311C:	memp->oi_changed.oi311C++;	break;
	case 0x3200:	memp->oi_changed.oi3200++;	break;
	case 0x3201:	memp->oi_changed.oi3201++;	break;
	case 0x3202:	memp->oi_changed.oi3202++;	break;
	case 0x3203:	memp->oi_changed.oi3203++;	break;
	case 0x4016:	memp->oi_changed.oi4016++;	break;
	case 0xf203:	memp->oi_changed.oiF203++;  break;
	}
}

INT16U set310d(OAD oad,INT8U *data,INT8U *DAR)
{
	Event310D_Object tmp310d={};
	INT16U  source_index=0,dest_index=0;
	int 	saveflg=0;

	saveflg = readCoverClass(oad.OI,0,&tmp310d,sizeof(Event310D_Object),event_para_save);
	fprintf(stderr,"\n[310d]电能表飞走事件 阈值=%d 任务号=%d\n",tmp310d.poweroffset_obj.power_offset,tmp310d.poweroffset_obj.task_no);
	get_BasicUnit(data,&source_index,(INT8U *)&tmp310d.poweroffset_obj,&dest_index);
	fprintf(stderr,"\n：属性6 阈值=%d 任务号=%d",tmp310d.poweroffset_obj.power_offset,tmp310d.poweroffset_obj.task_no);
	saveflg = saveCoverClass(oad.OI,0,&tmp310d,sizeof(Event310D_Object),event_para_save);
	*DAR = prtstat(saveflg);
	return source_index;
}

INT16U set310c(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U source_index=0,dest_index=0;
	Event310C_Object tmp310c={};
	int saveflg=0;

	readCoverClass(oad.OI,0,&tmp310c,sizeof(tmp310c),event_para_save);
	fprintf(stderr,"\n[310c]阈值=%x",tmp310c.poweroffset_obj.power_offset);
	get_BasicUnit(data,&source_index,(INT8U *)&tmp310c.poweroffset_obj.power_offset,&dest_index);
	fprintf(stderr,"\n电能量超差事件：属性6 阈值=%x",tmp310c.poweroffset_obj.power_offset);
	saveflg = saveCoverClass(oad.OI,0,&tmp310c,sizeof(tmp310c),event_para_save);
	*DAR = prtstat(saveflg);
	return source_index;
}

INT16U set310e(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U source_index=0,dest_index=0;
	Event310E_Object tmp310e={};
	int saveflg=0;

	readCoverClass(oad.OI,0,&tmp310e,sizeof(tmp310e),event_para_save);
	fprintf(stderr,"\ntmp310e 阈值=%d 单位=%d",tmp310e.powerstoppara_obj.power_offset.interval,tmp310e.powerstoppara_obj.power_offset.units);
	get_BasicUnit(data,&source_index,(INT8U *)&tmp310e.powerstoppara_obj.power_offset,&dest_index);
	fprintf(stderr,"\n电能表停走事件：属性6 阈值=%d 单位=%d",tmp310e.powerstoppara_obj.power_offset.interval,tmp310e.powerstoppara_obj.power_offset.units);
	saveflg = saveCoverClass(oad.OI,0,&tmp310e,sizeof(tmp310e),event_para_save);
	*DAR = prtstat(saveflg);
	return source_index;
}

INT16U set310f(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U source_index=0,dest_index=0;
	Event310F_Object tmp310f={};
	int saveflg = 0;

	readCoverClass(oad.OI,0,&tmp310f,sizeof(tmp310f),event_para_save);
	get_BasicUnit(data,&source_index,(INT8U *)&tmp310f.collectfail_obj.retry_nums,&dest_index);
	fprintf(stderr,"\n终端抄表失败事件：属性6 重试轮次=%d ",tmp310f.collectfail_obj.retry_nums);
	saveflg = saveCoverClass(oad.OI,0,&tmp310f,sizeof(tmp310f),event_para_save);
	*DAR = prtstat(saveflg);
	return source_index;
}

INT16U set4000(INT8U attflg,INT8U index,INT8U *data)
{
	INT16U source_index=0,dest_index=0;
	DateTimeBCD datetime;

	if ( attflg == 2 )
	{
		DataTimeGet(&datetime);
		Event_3114(datetime,memp);//对时，产生事件
		get_BasicUnit(data,&source_index,(INT8U *)&datetime,&dest_index);
		setsystime(datetime);
	}
	return source_index;
}

INT16U set4001_4002_4003(OAD oad,INT8U *data)
{
	int datalen ;
	INT16U source_index=0,dest_index=0;
	CLASS_4001_4002_4003	class_addr={};
	int i=0;
	memset(&class_addr,0,sizeof(CLASS_4001_4002_4003));
	readCoverClass(oad.OI,0,&class_addr,sizeof(CLASS_4001_4002_4003),para_vari_save);

	if (oad.attflg == 2 )
	{
		get_BasicUnit(data,&source_index,(INT8U *)&class_addr.curstom_num,&dest_index);
		datalen = class_addr.curstom_num[0]+1;
		fprintf(stderr,"\n设置 datalen=%d",datalen);
		for(i=0;i<datalen;i++) {
			fprintf(stderr,"%02x ",class_addr.curstom_num[i]);
		}
		saveCoverClass(oad.OI,0,&class_addr,sizeof(CLASS_4001_4002_4003),para_vari_save);
	}
	return source_index;
}
INT16U set4004(OAD oad,INT8U *data)
{
	INT16U source_index=0,dest_index=0;
	CLASS_4004 class4004;
	memset(&class4004,0,sizeof(CLASS_4004));
	readCoverClass(oad.OI,0,&class4004,sizeof(CLASS_4004),para_vari_save);
	if (oad.attflg == 2 )
	{
		get_BasicUnit(data,&source_index,(INT8U *)&class4004.jing,&dest_index);
		fprintf(stderr,"\n【精度】方位 %d  度 %d  分 %d  秒 %d",class4004.jing.fangwei,class4004.jing.du,class4004.jing.fen,class4004.jing.miao);
		fprintf(stderr,"\n【纬度】方位 %d  度 %d  分 %d  秒 %d",class4004.jing.fangwei,class4004.jing.du,class4004.jing.fen,class4004.jing.miao);
		fprintf(stderr,"\n【高度】%d",class4004.heigh);
		saveCoverClass(oad.OI,0,&class4004,sizeof(CLASS_4004),para_vari_save);
	}
	return 0;
}
INT16U set4006(OAD oad,INT8U *data)
{
	INT16U source_index=0,dest_index=0;
	CLASS_4006 class4006;
	memset(&class4006,0,sizeof(CLASS_4006));
	readCoverClass(oad.OI,0,&class4006,sizeof(CLASS_4006),para_vari_save);
	if (oad.attflg == 2 )
	{
		get_BasicUnit(data,&source_index,(INT8U *)&class4006,&dest_index);
		fprintf(stderr,"\n【时钟源】%d",class4006.clocksource);
		fprintf(stderr,"\n【状态】 %d",class4006.state);
		saveCoverClass(oad.OI,0,&class4006,sizeof(CLASS_4006),para_vari_save);
	}
	return 0;
}
INT16U set4007(OAD oad,INT8U *data)
{
	INT16U source_index=0,dest_index=0;
	CLASS_4007 class4007;
	memset(&class4007,0,sizeof(CLASS_4007));
	readCoverClass(oad.OI,0,&class4007,sizeof(CLASS_4007),para_vari_save);
	if (oad.attflg == 2 )
	{
		get_BasicUnit(data,&source_index,(INT8U *)&class4007,&dest_index);
		fprintf(stderr,"\n【上电全显时间】%d",class4007.poweon_showtime);
		fprintf(stderr,"\n【背光点亮时间（按键）】 %d",class4007.lcdlight_time);
		fprintf(stderr,"\n【背光点亮时间(查看)】 %d",class4007.looklight_time);
		fprintf(stderr,"\n【有电按键屏幕驻留时间(查看)】 %d",class4007.poweron_maxtime);
		fprintf(stderr,"\n【无电按键屏幕驻留时间(查看)】 %d",class4007.poweroff_maxtime);
		fprintf(stderr,"\n【显示电能小数位】 %d",class4007.energydata_dec);
		fprintf(stderr,"\n【显示功率小数位】 %d",class4007.powerdata_dec);
		saveCoverClass(oad.OI,0,&class4007,sizeof(CLASS_4007),para_vari_save);
	}
	return 0;
}
INT16U set4300(INT8U attflg,INT8U index,INT8U *data)
{
	INT16U source_index=0,dest_index=0;
	if ( attflg == 8 )//允许\禁止终端主动上报
	{
		INT8U autoReport=0;
		get_BasicUnit(data,&source_index,&autoReport,&dest_index);
		fprintf(stderr,"\n终端主动上报 : %d\n",autoReport);
	}
	return source_index;
}
INT16U set4500(OAD oad,INT8U *data)
{
	INT16U source_index=0,dest_index=0;
	CLASS25 class4500;
	memset(&class4500,0,sizeof(CLASS25));
	readCoverClass(oad.OI,0,&class4500,sizeof(CLASS25),para_vari_save);
	fprintf(stderr,"\n先读出 主站IP %d.%d.%d.%d :%d\n",class4500.master.master[0].ip[1],class4500.master.master[0].ip[2],
			class4500.master.master[0].ip[3],class4500.master.master[0].ip[4],class4500.master.master[0].port);

	if (oad.attflg == 2 )
	{
		COMM_CONFIG_1 config;
		memset(&config,0,sizeof(config));
		config.listenPortnum = 0xCC;//数组
		get_BasicUnit(data,&source_index,(INT8U *)&config.workModel,&dest_index);
		fprintf(stderr,"\n【工作模式】%d",config.workModel);
		fprintf(stderr,"\n【在线方式】%d",config.onlineType);
		fprintf(stderr,"\n【连接方式】%d",config.connectType);
		fprintf(stderr,"\n【连接应用方式】%d",config.appConnectType);
		fprintf(stderr,"\n【侦听端口1】%04x %d",config.listenPort[0],config.listenPort[0]);
		fprintf(stderr,"\n【侦听端口2】%04x %d",config.listenPort[1],config.listenPort[1]);
		fprintf(stderr,"\n【APN】 %s",&config.apn[1]);
		fprintf(stderr,"\n【用户名】 %s",&config.userName[1]);
		fprintf(stderr,"\n【密码】 %s",&config.passWord[1]);
		fprintf(stderr,"\n【代理服务器地址】 %d.%d.%d.%d ",config.proxyIp[1],config.proxyIp[2],config.proxyIp[3],config.proxyIp[4]);
		fprintf(stderr,"\n【代理服务器端口】 %d",config.proxyPort);
		fprintf(stderr,"\n【超时时间和重发次数】 %02x",config.timeoutRtry);
		fprintf(stderr,"\n【心跳周期】 %d\n",config.heartBeat);
		memcpy(&class4500.commconfig,&config,sizeof(COMM_CONFIG_1));
		saveCoverClass(oad.OI,0,&class4500,sizeof(CLASS25),para_vari_save);
	}
	if (oad.attflg == 3)
	{
		MASTER_STATION_INFO_LIST  master;
		memset(&master,0,sizeof(master));
		master.masternum = 0x22;
		get_BasicUnit(data,&source_index,(INT8U *)&master.masternum,&dest_index);
		fprintf(stderr,"\n【主站IP】%d.%d.%d.%d",master.master[0].ip[1],master.master[0].ip[2],master.master[0].ip[3],master.master[0].ip[4]);
		fprintf(stderr,"\n【端口号】 %d  \n",master.master[0].port);
		memcpy(&class4500.master,&master,sizeof(MASTER_STATION_INFO_LIST));
		fprintf(stderr,"\n存储前 主站IP %d.%d.%d.%d :%d\n",class4500.master.master[0].ip[1],class4500.master.master[0].ip[2],
				class4500.master.master[0].ip[3],class4500.master.master[0].ip[4],class4500.master.master[0].port);

		saveCoverClass(oad.OI,0,&class4500,sizeof(CLASS25),para_vari_save);
	}
	memp->oi_changed.oi4500++;
	return 1;
}
INT16U set4103(OAD oad,INT8U *data)
{
	int i=0,bytenum=0;
	INT16U source_index=0,dest_index=0;
	CLASS_4103 class4103;
	memset(&class4103,0,sizeof(CLASS_4103));
	fprintf(stderr,"\n==========%d",oad.attflg);
	readCoverClass(oad.OI,0,&class4103,sizeof(CLASS_4103),para_vari_save);
	if (oad.attflg == 2 )
	{
		bytenum = data[1]+1; // data[0]表示类型，data[1]表示长度，字符串长度加 长度字节本身
		if (bytenum>sizeof(class4103.assetcode))
			bytenum = sizeof(class4103.assetcode);
		memcpy(class4103.assetcode,&data[1],bytenum);
		fprintf(stderr,"\n【资产管理编码】%d :",bytenum);
		for(i=0;i<bytenum;i++)
			fprintf(stderr,"%02x ",class4103.assetcode[i]);
		fprintf(stderr,"\n");
		saveCoverClass(oad.OI,0,&class4103,sizeof(CLASS_4103),para_vari_save);
	}
	return 0;
}
INT16U set4204(OAD oad,INT8U *data)
{
	INT16U source_index=0,dest_index=0;
	CLASS_4204 class4204;
	memset(&class4204,0,sizeof(CLASS_4204));
	fprintf(stderr,"\n==========%d",oad.attflg);
	readCoverClass(oad.OI,0,&class4204,sizeof(CLASS_4204),para_vari_save);
	if (oad.attflg == 2 )
	{
		get_BasicUnit(data,&source_index,(INT8U *)&class4204.startime,&dest_index);
		fprintf(stderr,"\n【终端广播校时,属性2】:");
		fprintf(stderr,"\ntime : %02x %02x %02x",class4204.startime[0],class4204.startime[1],class4204.startime[2]);
		fprintf(stderr,"\nenable: %d",class4204.enable);
		fprintf(stderr,"\n");
		saveCoverClass(oad.OI,0,&class4204,sizeof(CLASS_4204),para_vari_save);
	}else if(oad.attflg == 3)
	{
		get_BasicUnit(data,&source_index,(INT8U *)&class4204.upleve,&dest_index);
		fprintf(stderr,"\n【终端广播校时，属性3】:");
		fprintf(stderr,"\ntime : %02x %02x %02x ",class4204.startime1[0],class4204.startime1[1],class4204.startime1[2]);
		fprintf(stderr,"\nenable: %d",class4204.enable1);
		fprintf(stderr,"\n误差 = %d",class4204.upleve);
		fprintf(stderr,"\n");
		saveCoverClass(oad.OI,0,&class4204,sizeof(CLASS_4204),para_vari_save);
	}
	return 0;
}
INT16U setf203(INT8U attflg,INT8U index,INT8U *data)
{
	INT16U source_index=0,dest_index=0;
	CLASS_f203	f203;
	memset(&f203,0,sizeof(CLASS_f203));
	readCoverClass(0xf203,0,&f203,sizeof(CLASS_f203),para_vari_save);
	if ( attflg == 4 )//配置参数
	{
		get_BasicUnit(data,&source_index,(INT8U*)&f203.state4,&dest_index);
		saveCoverClass(0xf203,0,&f203,sizeof(CLASS_f203),para_vari_save);
		fprintf(stderr,"\n状态量配置参数 : 接入标志 %02x  属性标志 %02x \n",f203.state4.StateAcessFlag,f203.state4.StatePropFlag);
	}
	return source_index;
}
INT16U setf101(INT8U attflg,INT8U index,INT8U *data)
{
	INT16U source_index=0,dest_index=0;
	CLASS_F101	f101;
	memset(&f101,0,sizeof(CLASS_F101));
	readCoverClass(0xf101,0,&f101,sizeof(CLASS_F101),para_vari_save);
	if ( attflg == 2 )//配置参数
	{
		get_BasicUnit(data,&source_index,(INT8U*)&f101.active,&dest_index);
		saveCoverClass(0xf101,0,&f101,sizeof(CLASS_F101),para_vari_save);
		fprintf(stderr,"\n安全模式选择 : %02x \n",f101.active);
	}
	return source_index;
}
///////////////////////////////////////////////////////////////////////////////
/*
 * 根据oi参数，查找相应的class_info的结构体数据
 * */
INT16S getEventClassLen(OI_698 oi)
{
	INT16U i=0;

	for(i=0; i < sizeof(event_class_len)/sizeof(EVENT_CLASS_INFO);i++)
	{
		if(event_class_len[i].oi == oi) {
			return event_class_len[i].classlen;
		}
	}
	fprintf(stderr,"未找到OI=%04x的相关结构长度！！！\n",oi);
	return -1;
}


INT16U setClass7attr(OAD oad,INT8U *data,INT8U *DAR)
{
	INT8U*	eventbuff=NULL;
	int 	saveflg=0,i=0;
	INT16S	classlen=0;
	INT16U 	source_index=0,dest_index=0;
	Class7_Object	class7={};
	INT8U	str[OCTET_STRING_LEN]={};

	classlen = getEventClassLen(oad.OI);
	eventbuff = (INT8U *)malloc(classlen);
	if(eventbuff!=NULL) {
		memset(eventbuff,0,classlen);
	}
	saveflg = readCoverClass(oad.OI,0,eventbuff,classlen,event_para_save);
//	fprintf(stderr,"\n设置前：clsslen=%d\n",classlen);
//	for(int i=0;i<classlen;i++) {
//		fprintf(stderr,"%02x ",eventbuff[i]);
//		if(i%16==0) fprintf(stderr," \n");
//	}
	memcpy(&class7,eventbuff,sizeof(Class7_Object));
	switch(oad.attflg) {
	case 1:	//逻辑名
		memset(str,0,sizeof(str));
		fprintf(stderr,"\n设置前:class7.oi = %04x",class7.oi);
		fprintf(stderr,"data=%02x %02x %02x %02x %02x\n",data[0],data[1],data[2],data[3],data[4]);
		get_BasicUnit(data,&source_index,(INT8U *)&str[0],&dest_index);
		fprintf(stderr,"str=%02x %02x %02x %02x %02x\n",str[0],str[1],str[2],str[3],str[4]);
		class7.oi = (str[0]<<8) | str[1];
		fprintf(stderr,"\n设置后:class7.oi = %04x",class7.oi);
		break;
	case 3:	//关联属性表
		get_BasicUnit(data,&source_index,(INT8U *)&class7.class7_oad,&dest_index);
		fprintf(stderr,"\n设置:class7.关联属性表num=%d ",class7.class7_oad.num);
		for(i=0;i<class7.class7_oad.num;i++) {
			fprintf(stderr,"\n%04x-%02x%02x",class7.class7_oad.oadarr[i].OI,class7.class7_oad.oadarr[i].attflg,class7.class7_oad.oadarr[i].attrindex);
		}
		break;
	case 4:	//当前记录数
		get_BasicUnit(data,&source_index,(INT8U *)&class7.crrentnum,&dest_index);
		fprintf(stderr,"\n设置:class7.当前记录数 = %d",class7.crrentnum);
		break;
	case 5:	//最大记录数
		get_BasicUnit(data,&source_index,(INT8U *)&class7.maxnum,&dest_index);
		fprintf(stderr,"\n设置:class7.最大记录数 = %d",class7.maxnum);
		break;
	case 8: //上报标识
		get_BasicUnit(data,&source_index,(INT8U *)&class7.reportflag,&dest_index);
		fprintf(stderr,"\n设置:class7.上报标识 = %d",class7.reportflag);
		break;
	case 9: //有效标识
		get_BasicUnit(data,&source_index,(INT8U *)&class7.enableflag,&dest_index);
		fprintf(stderr,"\n设置:class7.有效标识 = %d",class7.enableflag);
		break;
	}
	memcpy(eventbuff,&class7,sizeof(Class7_Object));
	saveflg = saveCoverClass(oad.OI,0,eventbuff,classlen,event_para_save);
	prtstat(saveflg);
	free(eventbuff);
	eventbuff=NULL;
	*DAR = success;
	return source_index;
}

INT8U EventSetAttrib(OAD oad,INT8U *data)
{
	INT8U	DAR=success;
	OI_698  oi = oad.OI;
	INT8U   attr = oad.attflg;
	INT16S	classlen=0;

	fprintf(stderr,"\n事件类对象属性设置");
	classlen = getEventClassLen(oi);
	if(classlen == -1) {
		DAR = obj_unexist;
		return DAR;
	}
	switch(attr) {
	case 1:	//逻辑名
	case 3:	//关联属性表
	case 4:	//当前记录数
	case 5:	//最大记录数
	case 8: //上报标识
	case 9: //有效标识
		setClass7attr(oad,data,&DAR);
		break;
	case 6:	//配置参数
		switch(oi) {
			case 0x310d:
				set310d(oad,data,&DAR);
				break;
			case 0x310c:
				set310c(oad,data,&DAR);
				break;
			case 0x310e:
				set310e(oad,data,&DAR);
				break;
			case 0x310F:
				set310f(oad,data,&DAR);
				break;
		}
		break;
	}
	return DAR;
}

void EnvironmentValue(OAD oad,INT8U *data)
{
	INT16U oi = oad.OI;
	INT8U attr = oad.attflg;
	fprintf(stderr,"\n参变量类对象属性设置");
	switch(oi)
	{
		case 0x4000://日期时间
			set4000(attr,oad.attrindex,data);
			break;
		case 0x4001://通信地址
			//set4001(oad,data);
			break;
		case 0x4002://表号
			break;
		case 0x4003://客户编号
			set4001_4002_4003(oad,data);
			break;
		case 0x4004://设备地理位置
			set4004(oad,data);
			break;
		case 0x4005://组地址
			break;
		case 0x4006://时钟源
			set4006(oad,data);
			break;
		case 0x4007://LCD参数
			set4007(oad,data);
			break;
		case 0x4030://电压合格率参数
			break;
		case 0x4300://
			set4300(attr,oad.attrindex,data);
			break;
		case 0x4103:
			set4103(oad,data);
			break;
		case 0x4204:
			set4204(oad,data);
			break;
		case 0x4500:
			set4500(oad,data);
			break;
	}
}
void CollParaSet(OAD oad,INT8U *data)
{
	INT16U oi = oad.OI;
	INT8U attr = oad.attflg;
	fprintf(stderr,"\n采集监控类对象属性设置");
	switch(oi)
	{
		case 0x6000:	//采集档案配置表
			break;
		case 0x6002:	//搜表
			break;
		case 0x6012:	//任务配置表
			break;
		case 0x6014:	//普通采集方案集
			break;
		case 0x6016:	//事件采集方案
			break;
	}
}
void DeviceIoSetAttrib(OAD oad,INT8U *data)
{
	INT16U oi = oad.OI;
	INT8U attr = oad.attflg;
	fprintf(stderr,"\n输入输出设备类对象属性设置");
	switch(oi)
	{
		case 0xF203:	//开关量输入
			setf203(attr,oad.attrindex,data);
			break;
		case 0xF101:
			setf101(attr,oad.attrindex,data);
			break;
	}
}
int setRequestNormal(INT8U *data,OAD oad,CSINFO *csinfo,INT8U *buf)
{
	INT16U oi = oad.OI;
	INT8U oihead = (oad.OI&0xF000) >>12;
	fprintf(stderr,"\n对象属性设置  【 %04x 】",oi);

	switch(oihead)
	{
		case 0x3:		//事件对象
			EventSetAttrib(oad,data);
			break;
		case 0x4:		//参变量对象
			EnvironmentValue(oad,data);
			break;
		case 0x6:		//采集监控类对象
			CollParaSet(oad,data);
			break;
		case 0xf:		//输入输出设备类对象 + ESAM接口类对象
			DeviceIoSetAttrib(oad,data);
	}
	setOIChange(oi);
	return success;
}
int setRequestNormalList(INT8U *data,OAD oad)
{

	return 0;
}

