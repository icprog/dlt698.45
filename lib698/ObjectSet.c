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
#include "event.h"
#include "dlt698.h"
#include "dlt698def.h"

extern int doReponse(int server,int reponse,CSINFO *csinfo,int datalen,INT8U *data,INT8U *buf);
extern int doGetnormal(INT8U seqOfNum,RESULT_NORMAL *response);
extern ProgramInfo *memp;
extern INT8U TmpDataBuf[MAXSIZ_FAM];
extern INT8U TmpDataBufList[MAXSIZ_FAM*2];

//INT8U prtstat(int flg)
//{
//	if (flg == 1) {
//		fprintf(stderr,"\n保存成功");
//		return success;
//	}else {
//		fprintf(stderr,"\n保存失败");
//		return refuse_rw;
//	}
//}
INT16U set300F(OAD oad,INT8U *data,INT8U *DAR)
{
	Event300F_Object tmp300f={};
	int		index = 0;

	readCoverClass(oad.OI,0,&tmp300f,sizeof(Event300F_Object),event_para_save);
	index += getStructure(&data[index],NULL);
	index += getUnsigned(&data[index],(INT8U *)&tmp300f.offset);
	fprintf(stderr,"\n：300F: 判定延时 =%d\n",tmp300f.offset);
	*DAR = saveCoverClass(oad.OI,0,&tmp300f,sizeof(Event300F_Object),event_para_save);
	return index;
}

INT16U set3105(OAD oad,INT8U *data,INT8U *DAR)  //属性6
{
	Event3105_Object tmp3105={};
	int		index = 0;

	readCoverClass(oad.OI,0,&tmp3105,sizeof(Event3105_Object),event_para_save);
	fprintf(stderr,"\n[3105]电能表时钟超差事件 阈值=%d 任务号=%d\n",tmp3105.mto_obj.over_threshold,tmp3105.mto_obj.task_no);
	index += getStructure(&data[index],NULL);
	index += getLongUnsigned(&data[index],(INT8U *)&tmp3105.mto_obj.over_threshold);
	index += getUnsigned(&data[index],(INT8U *)&tmp3105.mto_obj.task_no);
	fprintf(stderr,"\n：属性6 阈值=%d 任务号=%d\n",tmp3105.mto_obj.over_threshold,tmp3105.mto_obj.task_no);
	*DAR = saveCoverClass(oad.OI,0,&tmp3105,sizeof(Event3105_Object),event_para_save);
	return index;
}

INT16U set3106(OAD oad,INT8U *data,INT8U *DAR)
{
	int i=0;
	Event3106_Object tmpobj={};
	int index=0;
	memset(&tmpobj,0,sizeof(Event3106_Object));
	readCoverClass(oad.OI,0,&tmpobj,sizeof(Event3106_Object),event_para_save);
	if(oad.attrindex == 0x00)
		index += getStructure(&data[index],NULL);
	if(oad.attrindex != 0x02){
		index += getStructure(&data[index],NULL);

		index += getBitString(1,&data[index],&tmpobj.poweroff_para_obj.collect_para_obj.collect_flag);//00
		index += getUnsigned(&data[index],&tmpobj.poweroff_para_obj.collect_para_obj.time_space);//04
		index += getUnsigned(&data[index],&tmpobj.poweroff_para_obj.collect_para_obj.time_threshold);//04
		INT8U arraysize =0;
		index += getArray(&data[index],&arraysize);
		tmpobj.poweroff_para_obj.collect_para_obj.tsaarr.num = arraysize;
		for(i=0;i<arraysize;i++)
		{
			index += getOctetstring(1,&data[index],tmpobj.poweroff_para_obj.collect_para_obj.tsaarr.meter_tas[i].addr);
		}
	}
	if(oad.attrindex != 0x01){
		index += getStructure(&data[index],NULL);
		index += getLongUnsigned(&data[index],(INT8U *)&tmpobj.poweroff_para_obj.screen_para_obj.mintime_space);
		index += getLongUnsigned(&data[index],(INT8U *)&tmpobj.poweroff_para_obj.screen_para_obj.maxtime_space);
		index += getLongUnsigned(&data[index],(INT8U *)&tmpobj.poweroff_para_obj.screen_para_obj.startstoptime_offset);
		index += getLongUnsigned(&data[index],(INT8U *)&tmpobj.poweroff_para_obj.screen_para_obj.sectortime_offset);
		index += getLongUnsigned(&data[index],(INT8U *)&tmpobj.poweroff_para_obj.screen_para_obj.happen_voltage_limit);
		index += getLongUnsigned(&data[index],(INT8U *)&tmpobj.poweroff_para_obj.screen_para_obj.recover_voltage_limit);
	}
	*DAR = saveCoverClass(oad.OI,0,&tmpobj,sizeof(Event3106_Object),event_para_save);
	return index;
}


INT16U set310c(OAD oad,INT8U *data,INT8U *DAR)	 //超差  属性6
{
	Event310C_Object tmp310c={};
	int	index=0;

	readCoverClass(oad.OI,0,&tmp310c,sizeof(tmp310c),event_para_save);
	fprintf(stderr,"\n[310c]阈值=%x",tmp310c.poweroffset_obj.power_offset);
	index += getStructure(&data[index],NULL);
	index += getDouble(&data[index],(INT8U *)&tmp310c.poweroffset_obj.power_offset);
	fprintf(stderr,"data[index]=%02x %02x \n",data[index],data[index+1]);
	index += getUnsigned(&data[index],(INT8U *)&tmp310c.poweroffset_obj.task_no);
	fprintf(stderr,"\n电能量超差事件：属性6 阈值=%x",tmp310c.poweroffset_obj.power_offset);
	*DAR = saveCoverClass(oad.OI,0,&tmp310c,sizeof(tmp310c),event_para_save);
	return index;
}

INT16U set310d(OAD oad,INT8U *data,INT8U *DAR)	//电能表飞走  属性6
{
	Event310D_Object tmp310d={};
	int		index=0;

	readCoverClass(oad.OI,0,&tmp310d,sizeof(Event310D_Object),event_para_save);
	fprintf(stderr,"\n[310d]电能表飞走事件 阈值=%d 任务号=%d\n",tmp310d.poweroffset_obj.power_offset,tmp310d.poweroffset_obj.task_no);
	index += getStructure(&data[index],NULL);
	index += getDouble(&data[index],(INT8U *)&tmp310d.poweroffset_obj.power_offset);
	index += getUnsigned(&data[index],(INT8U *)&tmp310d.poweroffset_obj.task_no);
	fprintf(stderr,"\n：属性6 阈值=%d 任务号=%d",tmp310d.poweroffset_obj.power_offset,tmp310d.poweroffset_obj.task_no);
	*DAR = saveCoverClass(oad.OI,0,&tmp310d,sizeof(Event310D_Object),event_para_save);
	return index;
}

INT16U set310e(OAD oad,INT8U *data,INT8U *DAR)	//电能表停走	属性6
{
	Event310E_Object tmp310e={};
	int		index=0;

	readCoverClass(oad.OI,0,&tmp310e,sizeof(tmp310e),event_para_save);
	fprintf(stderr,"\ntmp310e 阈值=%d 单位=%d",tmp310e.powerstoppara_obj.power_offset.interval,tmp310e.powerstoppara_obj.power_offset.units);
	index += getStructure(&data[index],NULL);
	index += getTI(1,&data[index],&tmp310e.powerstoppara_obj.power_offset);
	index += getUnsigned(&data[index],(INT8U *)&tmp310e.powerstoppara_obj.task_no);
	fprintf(stderr,"\n电能表停走事件：属性6 阈值=%d 单位=%d",tmp310e.powerstoppara_obj.power_offset.interval,tmp310e.powerstoppara_obj.power_offset.units);
	*DAR = saveCoverClass(oad.OI,0,&tmp310e,sizeof(tmp310e),event_para_save);
	return index;
}

INT16U set310f(OAD oad,INT8U *data,INT8U *DAR)		//终端抄表失败  属性6
{
	Event310F_Object tmp310f={};
	int		index=0;

	readCoverClass(oad.OI,0,&tmp310f,sizeof(tmp310f),event_para_save);
	index += getStructure(&data[index],NULL);
	index += getUnsigned(&data[index],(INT8U *)&tmp310f.collectfail_obj.retry_nums);
	index += getUnsigned(&data[index],(INT8U *)&tmp310f.collectfail_obj.task_no);
	fprintf(stderr,"\n终端抄表失败事件：属性6 重试轮次=%d ",tmp310f.collectfail_obj.retry_nums);
	*DAR = saveCoverClass(oad.OI,0,&tmp310f,sizeof(tmp310f),event_para_save);
	return index;
}

INT16U set3110(OAD oad,INT8U *data,INT8U *DAR)		//月通信流量超限  属性6
{
	int		index=0;
	Event3110_Object tmpobj={};

	readCoverClass(oad.OI,0,&tmpobj,sizeof(tmpobj),event_para_save);
	index += getStructure(&data[index],NULL);
	index += getDouble(&data[index],(INT8U *)&tmpobj.Monthtrans_obj.month_offset);
	fprintf(stderr,"\n月通信流量限值事件：属性6　通信流量限值=%d ",tmpobj.Monthtrans_obj.month_offset);
	*DAR = saveCoverClass(oad.OI,0,&tmpobj,sizeof(tmpobj),event_para_save);
	return index;
}

INT16U set4000(OAD oad,INT8U *data,INT8U *DAR)
{
	DateTimeBCD datetime={};
	CLASS_4000	class_tmp={};
	int		index=0;

	*DAR = success;
	switch(oad.attflg) {
	case 2:
		DataTimeGet(&datetime);
		index += getDateTimeS(1,&data[index],(INT8U *)&datetime);
		setsystime(datetime);
		break;
	case 3://校时模式
		readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4000),para_vari_save);
		index += getEnum(1,&data[index],&class_tmp.type);
		*DAR = saveCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4000),para_vari_save);
		break;
	case 4:		//精准校时模式
		readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4000),para_vari_save);
		index += getStructure(&data[index],NULL);
		index += getUnsigned(&data[index],&class_tmp.hearbeatnum);
		index += getUnsigned(&data[index],&class_tmp.tichu_max);
		index += getUnsigned(&data[index],&class_tmp.tichu_min);
		index += getUnsigned(&data[index],&class_tmp.delay);
		index += getUnsigned(&data[index],&class_tmp.num_min);
		*DAR = saveCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4000),para_vari_save);
		break;
	}
	return index;
}

INT16U set4001_4002_4003(OAD oad,INT8U *data,INT8U *DAR)	//通信地址，表号，客户编号
{
	int datalen=0;
	int	index=0;
	int i=0;
	CLASS_4001_4002_4003	class_addr={};

	memset(&class_addr,0,sizeof(CLASS_4001_4002_4003));
	readCoverClass(oad.OI,0,&class_addr,sizeof(CLASS_4001_4002_4003),para_vari_save);
	memset(&class_addr.curstom_num,0,sizeof(class_addr.curstom_num));
	if (oad.attflg == 2 )
	{
		index += getOctetstring(1,&data[index],(INT8U *)&class_addr.curstom_num);
		datalen = class_addr.curstom_num[0]+1;
		fprintf(stderr,"\n设置 datalen=%d",datalen);
		for(i=0;i<datalen;i++) {
			fprintf(stderr,"%02x ",class_addr.curstom_num[i]);
		}
		*DAR = saveCoverClass(oad.OI,0,&class_addr,sizeof(CLASS_4001_4002_4003),para_vari_save);
	}
	return index;
}

INT16U set4004(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0;
	CLASS_4004 class4004={};
	memset(&class4004,0,sizeof(CLASS_4004));
	readCoverClass(oad.OI,0,&class4004,sizeof(CLASS_4004),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getStructure(&data[index],NULL);
		index += getStructure(&data[index],NULL);
		index += getEnum(1,&data[index],&class4004.jing.fangwei);
		index += getUnsigned(&data[index],&class4004.jing.du);
		index += getUnsigned(&data[index],&class4004.jing.fen);
		index += getUnsigned(&data[index],&class4004.jing.miao);
		index += getStructure(&data[index],NULL);
		index += getEnum(1,&data[index],&class4004.wei.fangwei);
		index += getUnsigned(&data[index],&class4004.wei.du);
		index += getUnsigned(&data[index],&class4004.wei.fen);
		index += getUnsigned(&data[index],&class4004.wei.miao);
		index += getDouble(&data[index],(INT8U *)&class4004.heigh);
		fprintf(stderr,"\n【精度】方位 %d  度 %d  分 %d  秒 %d",class4004.jing.fangwei,class4004.jing.du,class4004.jing.fen,class4004.jing.miao);
		fprintf(stderr,"\n【纬度】方位 %d  度 %d  分 %d  秒 %d",class4004.jing.fangwei,class4004.jing.du,class4004.jing.fen,class4004.jing.miao);
		fprintf(stderr,"\n【高度】%d",class4004.heigh);
		*DAR = saveCoverClass(oad.OI,0,&class4004,sizeof(CLASS_4004),para_vari_save);
	}
	return index;
}

INT16U set4006(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U index=0;
	CLASS_4006 class4006={};
	memset(&class4006,0,sizeof(CLASS_4006));
	readCoverClass(oad.OI,0,&class4006,sizeof(CLASS_4006),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getStructure(&data[index],NULL);
		index += getEnum(1,&data[index],&class4006.clocksource);
		index += getEnum(1,&data[index],&class4006.state);
		fprintf(stderr,"\n【时钟源】%d",class4006.clocksource);
		fprintf(stderr,"\n【状态】 %d",class4006.state);
		*DAR = saveCoverClass(oad.OI,0,&class4006,sizeof(CLASS_4006),para_vari_save);
	}
	return index;
}
INT16U set4007(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0;
//	INT16U source_index=0,dest_index=0;
	CLASS_4007 class4007={};
	memset(&class4007,0,sizeof(CLASS_4007));
	readCoverClass(oad.OI,0,&class4007,sizeof(CLASS_4007),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getStructure(&data[index],NULL);
		index += getUnsigned(&data[index],&class4007.poweon_showtime);
		index += getLongUnsigned(&data[index],(INT8U *)&class4007.lcdlight_time);
		index += getLongUnsigned(&data[index],(INT8U *)&class4007.looklight_time);
		index += getLongUnsigned(&data[index],(INT8U *)&class4007.poweron_maxtime);
		index += getLongUnsigned(&data[index],(INT8U *)&class4007.poweroff_maxtime);
		index += getUnsigned(&data[index],&class4007.energydata_dec);
		index += getUnsigned(&data[index],&class4007.powerdata_dec);

		fprintf(stderr,"\n【上电全显时间】%d",class4007.poweon_showtime);
		fprintf(stderr,"\n【背光点亮时间（按键）】 %d",class4007.lcdlight_time);
		fprintf(stderr,"\n【背光点亮时间(查看)】 %d",class4007.looklight_time);
		fprintf(stderr,"\n【有电按键屏幕驻留时间(查看)】 %d",class4007.poweron_maxtime);
		fprintf(stderr,"\n【无电按键屏幕驻留时间(查看)】 %d",class4007.poweroff_maxtime);
		fprintf(stderr,"\n【显示电能小数位】 %d",class4007.energydata_dec);
		fprintf(stderr,"\n【显示功率小数位】 %d",class4007.powerdata_dec);
		*DAR = saveCoverClass(oad.OI,0,&class4007,sizeof(CLASS_4007),para_vari_save);
	}
	return index;
}

INT16U set4030(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0;
	CLASS_4030 class4030={};
	memset(&class4030,0,sizeof(CLASS_4030));

	readCoverClass(oad.OI,0,&class4030,sizeof(CLASS_4030),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getStructure(&data[index],NULL);
		index += getLongUnsigned(&data[index],(INT8U *)&class4030.uUp_Kaohe);
		index += getLongUnsigned(&data[index],(INT8U *)&class4030.uDown_Kaohe);
		index += getLongUnsigned(&data[index],(INT8U *)&class4030.uUp);
		index += getLongUnsigned(&data[index],(INT8U *)&class4030.uDown);

		fprintf(stderr,"\n【电压考核上限】%d",class4030.uUp_Kaohe);
		fprintf(stderr,"\n【电压考核下限】%d",class4030.uDown_Kaohe);
		fprintf(stderr,"\n【电压合格上限】%d",class4030.uUp);
		fprintf(stderr,"\n【电压合格下限】%d",class4030.uDown);
		*DAR = saveCoverClass(oad.OI,0,&class4030,sizeof(CLASS_4030),para_vari_save);
	}
	return index;
}

INT16U set4103(OAD oad,INT8U *data,INT8U *DAR)
{
	int i=0;//,bytenum=0;
	int	index=0;
	CLASS_4103 class4103={};

	memset(&class4103,0,sizeof(CLASS_4103));
	fprintf(stderr,"\n==========%d",oad.attflg);
	readCoverClass(oad.OI,0,&class4103,sizeof(CLASS_4103),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getVisibleString(&data[index],(INT8U *)&class4103.assetcode);
		fprintf(stderr,"\n【资产管理编码】%d :",class4103.assetcode[0]);
		for(i=0;i<class4103.assetcode[0];i++)
			fprintf(stderr,"%02x ",class4103.assetcode[i+1]);
		fprintf(stderr,"\n");
		*DAR = saveCoverClass(oad.OI,0,&class4103,sizeof(CLASS_4103),para_vari_save);
	}
	return index;
}

INT16U set4204(OAD oad,INT8U *data,INT8U *DAR)
{
	int	index=0;
	CLASS_4204 class4204={};
	memset(&class4204,0,sizeof(CLASS_4204));
	fprintf(stderr,"\n==========%d",oad.attflg);
	readCoverClass(oad.OI,0,&class4204,sizeof(CLASS_4204),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getStructure(&data[index],NULL);
		index += getTime(1,&data[index],(INT8U *)&class4204.startime);
		index += getBool(&data[index],(INT8U *)&class4204.enable);
		fprintf(stderr,"\n【终端广播校时,属性2】:");
		fprintf(stderr,"\ntime : %d %d %d",class4204.startime[0],class4204.startime[1],class4204.startime[2]);
		fprintf(stderr,"\nenable: %d",class4204.enable);
		fprintf(stderr,"\n");
		*DAR = saveCoverClass(oad.OI,0,&class4204,sizeof(CLASS_4204),para_vari_save);
	}else if(oad.attflg == 3)
	{
		index += getStructure(&data[index],NULL);
		index += getUnsigned(&data[index],(INT8U *)&class4204.upleve);
		index += getTime(1,&data[index],(INT8U *)&class4204.startime1);
		index += getBool(&data[index],(INT8U *)&class4204.enable1);
		fprintf(stderr,"\n【终端广播校时，属性3】:");
		fprintf(stderr,"\ntime : %d %d %d ",class4204.startime1[0],class4204.startime1[1],class4204.startime1[2]);
		fprintf(stderr,"\nenable: %d",class4204.enable1);
		fprintf(stderr,"\n误差 = %d",class4204.upleve);
		fprintf(stderr,"\n");
		*DAR = saveCoverClass(oad.OI,0,&class4204,sizeof(CLASS_4204),para_vari_save);
	}
	return index;
}
INT16U set4300(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U index=0;
	CLASS19		class4300={};

	readCoverClass(oad.OI,0,&class4300,sizeof(CLASS19),para_vari_save);
	fprintf(stderr,"oi =%04x,attflg=%d\n",oad.OI,oad.attflg);
	switch(oad.attflg) {
	case 7:	//允许跟随上报
		index += getBool(data,&class4300.follow_report);
		fprintf(stderr,"\n允许跟随上报 : %d",class4300.follow_report);
		*DAR = saveCoverClass(oad.OI,0,&class4300,sizeof(CLASS19),para_vari_save);
		break;
	case 8:	//允许\禁止终端主动上报
		index += getBool(data,&class4300.active_report);
		fprintf(stderr,"\n终端主动上报 : %d",class4300.active_report);
		*DAR = saveCoverClass(oad.OI,0,&class4300,sizeof(CLASS19),para_vari_save);
		break;
	case 9:	//允许与主站通话
		index += getBool(data,&class4300.talk_master);
		fprintf(stderr,"\n允许与主站通话 : %d",class4300.talk_master);
		*DAR = saveCoverClass(oad.OI,0,&class4300,sizeof(CLASS19),para_vari_save);
		break;
	}
	return index;
}

void print4500(CLASS25 class4500)
{
	int  i=0,j=0;
	fprintf(stderr,"\n-------2:通信配置----------");
	fprintf(stderr,"\n【工作模式】%d",class4500.commconfig.workModel);
	fprintf(stderr,"\n【在线方式】%d",class4500.commconfig.onlineType);
	fprintf(stderr,"\n【连接方式】%d",class4500.commconfig.connectType);
	fprintf(stderr,"\n【连接应用方式】%d",class4500.commconfig.appConnectType);
	fprintf(stderr,"\n【侦听端口1】%04x %d",class4500.commconfig.listenPort[0],class4500.commconfig.listenPort[0]);
	fprintf(stderr,"\n【侦听端口2】%04x %d",class4500.commconfig.listenPort[1],class4500.commconfig.listenPort[1]);
	fprintf(stderr,"\n【APN】 %s",&class4500.commconfig.apn[1]);
	fprintf(stderr,"\n【用户名】 %s",&class4500.commconfig.userName[1]);
	fprintf(stderr,"\n【密码】 %s",&class4500.commconfig.passWord[1]);
	fprintf(stderr,"\n【代理服务器地址】 %d.%d.%d.%d ",class4500.commconfig.proxyIp[1],class4500.commconfig.proxyIp[2],class4500.commconfig.proxyIp[3],class4500.commconfig.proxyIp[4]);
	fprintf(stderr,"\n【代理服务器端口】 %d",class4500.commconfig.proxyPort);
	fprintf(stderr,"\n【超时时间和重发次数】 %02x",class4500.commconfig.timeoutRtry);
	fprintf(stderr,"\n【心跳周期】 %d\n",class4500.commconfig.heartBeat);
	fprintf(stderr,"\n-------3:主站通信参数表----------");
	fprintf(stderr,"\n【主站IP】%d.%d.%d.%d",class4500.master.master[0].ip[1],class4500.master.master[0].ip[2],class4500.master.master[0].ip[3],class4500.master.master[0].ip[4]);
	fprintf(stderr,"\n【端口号】 %d  \n",class4500.master.master[0].port);
	fprintf(stderr,"\n存储前 主站IP %d.%d.%d.%d :%d\n",class4500.master.master[0].ip[1],class4500.master.master[0].ip[2],
			class4500.master.master[0].ip[3],class4500.master.master[0].ip[4],class4500.master.master[0].port);
	fprintf(stderr,"\n-------4:通信参数表----------");
	fprintf(stderr,"\n【短信中心号码】 %s ",&class4500.sms.center[1]);
	for(i=0;i<(class4500.sms.center[0]+1);i++) {
		fprintf(stderr,"%02x ",class4500.sms.center[i]);
	}
	fprintf(stderr,"\n【主站号码】 %d ",class4500.sms.masternum);
	for(i=0;i<class4500.sms.masternum;i++) {
		fprintf(stderr," \n%s ",class4500.sms.master[i]);
		for(j=0;j<(class4500.sms.masternum+1);j++) {
			fprintf(stderr,"%02x ",class4500.sms.master[i][j]);
		}
	}
	fprintf(stderr,"\n【短信通知号码】 %d ",class4500.sms.destnum);
	for(i=0;i<class4500.sms.destnum;i++) {
		fprintf(stderr," \n%s ",class4500.sms.dest[i]);
		for(j=0;j<(class4500.sms.destnum+1);j++) {
			fprintf(stderr,"%02x ",class4500.sms.dest[i][j]);
		}
	}
}

INT16U set4500(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0,i=0;
	CLASS25 class4500={};
	memset(&class4500,0,sizeof(CLASS25));

	readCoverClass(oad.OI,0,&class4500,sizeof(CLASS25),para_vari_save);
	fprintf(stderr,"\n先读出 主站IP %d.%d.%d.%d :%d\n",class4500.master.master[0].ip[1],class4500.master.master[0].ip[2],
			class4500.master.master[0].ip[3],class4500.master.master[0].ip[4],class4500.master.master[0].port);

	switch(oad.attflg) {
	case 2:	//通信配置
		index += getStructure(&data[index],NULL);
		index += getEnum(1,&data[index],(INT8U *)&class4500.commconfig.workModel);
		index += getEnum(1,&data[index],(INT8U *)&class4500.commconfig.onlineType);
		index += getEnum(1,&data[index],(INT8U *)&class4500.commconfig.connectType);
		index += getEnum(1,&data[index],(INT8U *)&class4500.commconfig.appConnectType);
		index += getArray(&data[index],(INT8U *)&class4500.commconfig.listenPortnum);
		if(class4500.commconfig.listenPortnum>5) {
			fprintf(stderr,"!!!!!!!!!越限 listenPortnum=%d\n",class4500.commconfig.listenPortnum);
			class4500.commconfig.listenPortnum = 5;
		}
		for(i=0;i<class4500.commconfig.listenPortnum;i++) {
			index += getLongUnsigned(&data[index],(INT8U *)&class4500.commconfig.listenPort[i]);
		}
		index += getVisibleString(&data[index],class4500.commconfig.apn);
		index += getVisibleString(&data[index],class4500.commconfig.userName);
		index += getVisibleString(&data[index],class4500.commconfig.passWord);
		index += getOctetstring(1,&data[index],class4500.commconfig.proxyIp);
		index += getLongUnsigned(&data[index],(INT8U *)&class4500.commconfig.proxyPort);
		index += getOctetstring(1,&data[index],(INT8U *)&class4500.commconfig.timeoutRtry);
		index += getLongUnsigned(&data[index],(INT8U *)&class4500.commconfig.heartBeat);
		break;
	case 3:		//主站通信参数表
		index += getArray(&data[index],(INT8U *)&class4500.master.masternum);
		if(class4500.master.masternum>4) {
			fprintf(stderr,"!!!!!!!!!越限 master.masternum=%d\n",class4500.master.masternum);
			class4500.master.masternum = 4;
		}
		for(i=0;i<class4500.master.masternum;i++) {
			index += getStructure(&data[index],NULL);
			index += getOctetstring(1,&data[index],class4500.master.master[i].ip);
			index += getLongUnsigned(&data[index],(INT8U *)&class4500.master.master[i].port);
		}
		break;
	case 4:		//短信通信参数表
		index += getStructure(&data[index],NULL);
		fprintf(stderr,"struct index=%d\n",index);
		index += getVisibleString(&data[index],(INT8U *)&class4500.sms.center);
		fprintf(stderr,"center index=%d %02x %02x\n",index,data[index],data[index+1]);
		index += getArray(&data[index],(INT8U *)&class4500.sms.masternum);
		if(class4500.sms.masternum>4) {
			fprintf(stderr,"!!!!!!!!!越限 sms.masternum=%d\n",class4500.sms.masternum);
			class4500.sms.masternum = 4;
		}
		for(i=0;i<class4500.sms.masternum;i++)
			index += getVisibleString(&data[index],(INT8U *)&class4500.sms.master[i]);
		index += getArray(&data[index],(INT8U *)&class4500.sms.destnum);
		if(class4500.sms.destnum>4) {
			fprintf(stderr,"!!!!!!!!!越限 sms.destnum=%d\n",class4500.sms.destnum);
			class4500.sms.destnum = 4;
		}
		for(i=0;i<class4500.sms.destnum;i++)
			index += getVisibleString(&data[index],(INT8U *)&class4500.sms.dest[i]);
		break;
	case 5:		//版本信息
		break;
	case 6:		//支持规约列表
		break;
	case 7:		//SIM卡CCID
		break;
	case 8:		//IMSI
		break;
	case 9:		//信号强度
		break;
	case 10:	//SIM卡号码
		break;
	case 11:	//拨号IP
		break;

	}
	print4500(class4500);
	*DAR = saveCoverClass(oad.OI,0,&class4500,sizeof(CLASS25),para_vari_save);
	return index;
}


INT16U set4510(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0,i=0;
	CLASS26 class4510={};
	memset(&class4510,0,sizeof(CLASS26));

	readCoverClass(oad.OI,0,&class4510,sizeof(CLASS26),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getStructure(&data[index],NULL);
		index += getEnum(1,&data[index],(INT8U *)&class4510.commconfig.workModel);
		index += getEnum(1,&data[index],(INT8U *)&class4510.commconfig.connectType);
		index += getEnum(1,&data[index],(INT8U *)&class4510.commconfig.appConnectType);
		index += getArray(&data[index],(INT8U *)&class4510.commconfig.listenPortnum);
		if(class4510.commconfig.listenPortnum>5) {
			fprintf(stderr,"!!!!!!!!!越限 listenPortnum=%d\n",class4510.commconfig.listenPortnum);
			class4510.commconfig.listenPortnum = 5;
		}
		for(i=0;i<class4510.commconfig.listenPortnum;i++) {
			index += getLongUnsigned(&data[index],(INT8U *)&class4510.commconfig.listenPort[i]);
		}
		index += getOctetstring(1,&data[index],class4510.commconfig.proxyIp);
		index += getLongUnsigned(&data[index],(INT8U *)&class4510.commconfig.proxyPort);
		index += getBitString(1,&data[index],(INT8U *)&class4510.commconfig.timeoutRtry);
		index += getLongUnsigned(&data[index],(INT8U *)&class4510.commconfig.heartBeat);
		fprintf(stderr,"\n【工作模式】%d",class4510.commconfig.workModel);
		fprintf(stderr,"\n【连接方式】%d",class4510.commconfig.connectType);
		fprintf(stderr,"\n【连接应用方式】%d",class4510.commconfig.appConnectType);
		fprintf(stderr,"\n【侦听端口总数】%d",class4510.commconfig.listenPortnum);
		fprintf(stderr,"\n【侦听端口1】%04x %d",class4510.commconfig.listenPort[0],class4510.commconfig.listenPort[0]);
		fprintf(stderr,"\n【代理服务器地址】 %d.%d.%d.%d ",class4510.commconfig.proxyIp[1],class4510.commconfig.proxyIp[2],class4510.commconfig.proxyIp[3],class4510.commconfig.proxyIp[4]);
		fprintf(stderr,"\n【代理服务器端口】 %d",class4510.commconfig.proxyPort);
		fprintf(stderr,"\n【超时时间和重发次数】 %02x",class4510.commconfig.timeoutRtry);
		fprintf(stderr,"\n【心跳周期】 %d\n",class4510.commconfig.heartBeat);
	}
	*DAR = saveCoverClass(oad.OI,0,&class4510,sizeof(CLASS26),para_vari_save);

	return index;
}

INT16U setf203(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U index=0;
	CLASS_f203	f203={};
	memset(&f203,0,sizeof(CLASS_f203));
	readCoverClass(0xf203,0,&f203,sizeof(CLASS_f203),para_vari_save);
	if ( oad.attflg == 4 )//配置参数
	{
		index += getStructure(&data[index],NULL);
		index += getBitString(1,&data[index],(INT8U *)&f203.state4.StateAcessFlag);
		index += getBitString(1,&data[index],(INT8U *)&f203.state4.StatePropFlag);
		*DAR = saveCoverClass(0xf203,0,&f203,sizeof(CLASS_f203),para_vari_save);
		fprintf(stderr,"\n状态量配置参数 : 接入标志 %02x  属性标志 %02x \n",f203.state4.StateAcessFlag,f203.state4.StatePropFlag);
	}
	return index;
}

INT16U setf101(OAD oad,INT8U *data,INT8U *DAR)
{
	int		index = 0,i=0;
	CLASS_F101	f101={};

	memset(&f101,0,sizeof(CLASS_F101));
	readCoverClass(0xf101,0,&f101,sizeof(CLASS_F101),para_vari_save);
	if ( oad.attflg == 2 )//配置参数
	{
		index += getEnum(1,&data[index],(INT8U*)&f101.active);
	}
	if (oad.attflg == 3) //显式安全模式参数
	{
		index += getArray(&data[index],(INT8U*)&f101.modelnum);
		index += getStructure(&data[index],NULL);
		for(i=0;i<f101.modelnum;i++) {
			index += getOI(1,&data[index],f101.modelpara[i].oi);
			index += getLongUnsigned(&data[index],(INT8U *)&f101.modelpara[i].model);
		}
	}
	*DAR = saveCoverClass(0xf101,0,&f101,sizeof(CLASS_F101),para_vari_save);
	fprintf(stderr,"\n安全模式选择 : %02x \n",f101.active);

	return index;
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
	int 	i=0;
	INT16S	classlen=0;
	Class7_Object	class7={};
	INT8U	str[OCTET_STRING_LEN]={};
	int		index = 0;

	classlen = getEventClassLen(oad.OI);
	eventbuff = (INT8U *)malloc(classlen);
	if(eventbuff!=NULL) {
		memset(eventbuff,0,classlen);
	}
	readCoverClass(oad.OI,0,eventbuff,classlen,event_para_save);
//	fprintf(stderr,"\n设置前：clsslen=%d\n",classlen);
//	for(int i=0;i<classlen;i++) {
//		fprintf(stderr,"%02x ",eventbuff[i]);
//		if(i%16==0) fprintf(stderr," \n");
//	}
	memcpy(&class7,eventbuff,sizeof(Class7_Object));
	switch(oad.attflg) {
	case 1:	//逻辑名
		memset(str,0,sizeof(str));
		fprintf(stderr,"\n设置前:class7.logic_name = %s",class7.logic_name);
		index += getOctetstring(1,&data[index],(INT8U *)&class7.logic_name);
		fprintf(stderr,"\n设置后:class7.logic_name = %s",class7.logic_name);
		break;
	case 3:	//关联属性表
		index += getArray(&data[index],&class7.class7_oad.num);
		for(i=0;i<class7.class7_oad.num;i++) {
			index += getOAD(1,&data[index],&class7.class7_oad.oadarr[i]);
		}
		fprintf(stderr,"\n设置:class7.关联属性表num=%d ",class7.class7_oad.num);
		for(i=0;i<class7.class7_oad.num;i++) {
			fprintf(stderr,"\n%04x-%02x%02x",class7.class7_oad.oadarr[i].OI,class7.class7_oad.oadarr[i].attflg,class7.class7_oad.oadarr[i].attrindex);
		}
		break;
	case 4:	//当前记录数
		index += getLongUnsigned(&data[index],(INT8U *)&class7.crrentnum);
		fprintf(stderr,"\n设置:class7.当前记录数 = %d",class7.crrentnum);
		break;
	case 5:	//最大记录数
		index += getLongUnsigned(&data[index],(INT8U *)&class7.maxnum);
		fprintf(stderr,"\n设置:class7.最大记录数 = %d",class7.maxnum);
		break;
	case 8: //上报标识
		index += getEnum(1,&data[index],(INT8U *)&class7.reportflag);
		fprintf(stderr,"\n设置:class7.上报标识 = %d",class7.reportflag);
		break;
	case 9: //有效标识
		index += getEnum(1,&data[index],(INT8U *)&class7.enableflag);
		fprintf(stderr,"\n设置:class7.有效标识 = %d",class7.enableflag);
		break;
	}
	memcpy(eventbuff,&class7,sizeof(Class7_Object));
	*DAR = saveCoverClass(oad.OI,0,eventbuff,classlen,event_para_save);
	free(eventbuff);
	eventbuff=NULL;
	return index;
}

INT16U EventSetAttrib(OAD oad,INT8U *data,INT8U *DAR)
{
	OI_698  oi = oad.OI;
	INT8U   attr = oad.attflg;
	INT16S	classlen=0;
	INT16U	data_index=0;

	fprintf(stderr,"\n事件类对象属性设置  oi=%04x\n",oi);
	classlen = getEventClassLen(oi);
	if(classlen == -1) {
		*DAR = obj_unexist;
		return *DAR;
	}
	switch(attr) {
	case 1:	//逻辑名
	case 3:	//关联属性表
	case 4:	//当前记录数
	case 5:	//最大记录数
	case 8: //上报标识
	case 9: //有效标识
		setClass7attr(oad,data,DAR);
		break;
	case 6:	//配置参数
		switch(oi) {
		case 0x300F:	//电能表电压逆相序事件
			data_index = set300F(oad,data,DAR);
			break;
		case 0x3105:	//电能表时钟超差事件
			data_index = set3105(oad,data,DAR);
			break;
		case 0x3106:	//终端停上电事件
			data_index = set3106(oad,data,DAR);
			break;
		case 0x310c:	//电能量超差事件阈值
			data_index = set310c(oad,data,DAR);
			break;
		case 0x310d:	//电能表飞走事件阈值
			data_index = set310d(oad,data,DAR);
			break;
		case 0x310e:	//电能表停走事件阈值
			data_index = set310e(oad,data,DAR);
			break;
		case 0x310F:	//终端抄表失败事件
			data_index = set310f(oad,data,DAR);
			break;
		case 0x3110:	//月通信流量超限事件阈值
			data_index = set3110(oad,data,DAR);
			break;
		}
		break;
	}
	return data_index;
}

INT16U EnvironmentValue(OAD oad,INT8U *data,INT8U *DAR)
{
	fprintf(stderr,"\n参变量类对象属性设置");
	INT16U	data_index=0;
	switch(oad.OI)
	{
		case 0x4000://日期时间
			data_index = set4000(oad,data,DAR);
			break;
		case 0x4001://通信地址
		case 0x4002://表号
		case 0x4003://客户编号
			data_index = set4001_4002_4003(oad,data,DAR);
			break;
		case 0x4004://设备地理位置
			data_index = set4004(oad,data,DAR);
			break;
		case 0x4005://组地址
			break;
		case 0x4006://时钟源
			data_index = set4006(oad,data,DAR);
			break;
		case 0x4007://LCD参数
			data_index = set4007(oad,data,DAR);
			break;
		case 0x4030://电压合格率参数
			data_index = set4030(oad,data,DAR);
			break;
		case 0x4103://资产管理编码
			data_index = set4103(oad,data,DAR);
			break;
		case 0x4204://终端广播校时
			data_index = set4204(oad,data,DAR);
			break;
		case 0x4300://电气设备
			data_index = set4300(oad,data,DAR);
			break;
		case 0x4500:
			data_index = set4500(oad,data,DAR);
			break;
		case 0x4510://以太网通信接口类
			data_index = set4510(oad,data,DAR);
			break;
	}
	return data_index;
}

INT16U CollParaSet(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U	data_index=0;
	INT16U oi = oad.OI;
//	INT8U attr = oad.attflg;
	fprintf(stderr,"\n采集监控类对象属性设置");
	*DAR = refuse_rw;
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
	return data_index;
}

INT16U DeviceIoSetAttrib(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U	data_index=0;
	fprintf(stderr,"\n输入输出设备类对象属性设置");
	switch(oad.OI)
	{
		case 0xF203:	//开关量输入
			data_index = setf203(oad,data,DAR);
			break;
		case 0xF101:
			data_index = setf101(oad,data,DAR);
			break;
	}
	return data_index;
}

INT16U setRequestNormal(INT8U *data,OAD oad,INT8U *DAR,CSINFO *csinfo,INT8U *buf)
{
	INT8U oihead = (oad.OI&0xF000) >>12;
	INT16U	data_index=0;
	fprintf(stderr,"\n对象属性设置  【 %04x 】",oad.OI);

	switch(oihead)
	{
		case 0x3:		//事件对象
			data_index = EventSetAttrib(oad,data,DAR);
			break;
		case 0x4:		//参变量对象
			data_index = EnvironmentValue(oad,data,DAR);
			break;
		case 0x6:		//采集监控类对象
			data_index = CollParaSet(oad,data,DAR);
			break;
		case 0xf:		//输入输出设备类对象 + ESAM接口类对象
			data_index = DeviceIoSetAttrib(oad,data,DAR);
	}
	if(*DAR==success) {		//参数文件更改，通知进程
		setOIChange(oad.OI);
	}
	return data_index;
}

int setRequestNormalList(INT8U *data,CSINFO *csinfo,INT8U *buf)
{
	INT8U DAR=success;
	OAD  oad={};
	OAD  event_oad[5]={};
	INT8U oadnum = 0,event_oadnum=0;
	int i=0,listindex=0;
	int sourceindex=0;		//源数据的索引

	oadnum = data[sourceindex++];
	fprintf(stderr,"\nsetRequestNormalList!! OAD_NUM=%d\n",oadnum);
	memset(TmpDataBufList,0,sizeof(TmpDataBufList));
	TmpDataBufList[listindex++] = oadnum;
	memset(&event_oad,0,sizeof(event_oad));
	for(i=0;i<oadnum;i++)
	{
		sourceindex += getOAD(0,&data[sourceindex],&oad);
		if(oad.OI==0x4300 || oad.OI==0x4000) {
			memcpy(&event_oad[event_oadnum],&oad,sizeof(OAD));
			event_oadnum++;
		}
		sourceindex += setRequestNormal(&data[sourceindex],oad,&DAR,NULL,buf);
		listindex += create_OAD(0,&TmpDataBufList[listindex],oad);
		TmpDataBufList[listindex++] = (INT8U)DAR;
	}
	doReponse(SET_RESPONSE,SET_REQUEST_NORMAL_LIST,csinfo,listindex,TmpDataBufList,buf);
	//此处处理防止在设置后未上送应答帧而直接上送事件报文
	for(i=0;i<event_oadnum;i++) {
		Get698_event(event_oad[i],memp);
	}
	return 0;
}


int setThenGetRequestNormalList(INT8U *data,CSINFO *csinfo,INT8U *buf)
{
	INT8U DAR=success;
	OAD  oad={};
	OAD  event_oad[5]={};
	INT8U seqofNum = 0,event_oadnum=0;
	INT8U get_delay = 0;		//延时读取时间
	int i=0,listindex=0;
	int sourceindex=0;		//源数据的索引
	RESULT_NORMAL response={};

	seqofNum = data[sourceindex++];
	fprintf(stderr,"\nsetRequestNormalList!! OAD_NUM=%d\n",seqofNum);
	memset(TmpDataBufList,0,sizeof(TmpDataBufList));
	TmpDataBufList[listindex++] = seqofNum;
	memset(&event_oad,0,sizeof(event_oad));
	for(i=0;i<seqofNum;i++)
	{
		sourceindex += getOAD(0,&data[sourceindex],&oad);		//一个设置的对象属性   OAD
		if(oad.OI==0x4300 || oad.OI==0x4000) {
			memcpy(&event_oad[event_oadnum],&oad,sizeof(OAD));
			event_oadnum++;
		}

		fprintf(stderr,"set 1 sourceindex=%d\n",sourceindex);
		sourceindex += setRequestNormal(&data[sourceindex],oad,&DAR,NULL,buf);	//设置对象数据     Data

		fprintf(stderr,"set end sourceindex=%d\n",sourceindex);

		listindex += create_OAD(0,&TmpDataBufList[listindex],oad);
		TmpDataBufList[listindex++] = (INT8U)DAR;

		sourceindex += getOAD(0,&data[sourceindex],&oad);		//一个读取的对象属性   OAD
		get_delay = data[sourceindex];						//延时读取时间
		sourceindex++;
		//get_delay = 0, 	//按照ProtocolConformance协商结果
		if(get_delay>=0 && get_delay<=5) 	sleep(get_delay);
		memset(TmpDataBuf,0,sizeof(TmpDataBuf));
		memcpy(&response.oad,&oad,sizeof(response.oad));
		response.datalen = 0;
		response.data = TmpDataBuf + 5;		//4 + 1             oad（4字节） + choice(1字节)
		doGetnormal(0,&response);
		listindex += create_OAD(0,&TmpDataBufList[listindex],oad);
		if(response.datalen>0) {
			TmpDataBufList[listindex++] = 1;		//Get-Result∷=CHOICE  Data		国网软件需要，但是报文解析软件不认
			memcpy(&TmpDataBufList[listindex],response.data,response.datalen);
			listindex = listindex + response.datalen;
		}else {
			TmpDataBufList[listindex++] = 0;//错误
			TmpDataBufList[listindex++] = response.dar;
		}
	}
	doReponse(SET_RESPONSE,SET_THENGET_REQUEST_NORMAL_LIST,csinfo,listindex,TmpDataBufList,buf);
	//此处处理防止在设置后未上送应答帧而直接上送事件报文
	for(i=0;i<event_oadnum;i++) {
		Get698_event(event_oad[i],memp);
	}
	return 0;
}


