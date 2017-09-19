/*
 * OIsetfunc.c
 *
 *  Created on: Sep 18, 2017
 *      Author: ava
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "AccessFun.h"
#include "PublicFunction.h"
#include "OIsetfunc.h"
#include "dlt698.h"

extern ProgramInfo *memp;
/////////////////////////////////////////////////////////////////////////////
INT16U set300F(OAD oad,INT8U *data,INT8U *DAR)
{
	Event300F_Object tmp300f={};
	int		index = 0;

	readCoverClass(oad.OI,0,&tmp300f,sizeof(Event300F_Object),event_para_save);
	index += getStructure(&data[index],NULL,DAR);
	index += getUnsigned(&data[index],(INT8U *)&tmp300f.offset,DAR);
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
	index += getStructure(&data[index],NULL,DAR);
	index += getLongUnsigned(&data[index],(INT8U *)&tmp3105.mto_obj.over_threshold);
	index += getUnsigned(&data[index],(INT8U *)&tmp3105.mto_obj.task_no,DAR);
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
		index += getStructure(&data[index],NULL,DAR);
	if(oad.attrindex != 0x02){
		index += getStructure(&data[index],NULL,DAR);

		index += getBitString(1,&data[index],&tmpobj.poweroff_para_obj.collect_para_obj.collect_flag);//00
		index += getUnsigned(&data[index],&tmpobj.poweroff_para_obj.collect_para_obj.time_space,DAR);//04
		index += getUnsigned(&data[index],&tmpobj.poweroff_para_obj.collect_para_obj.time_threshold,DAR);//04
		INT8U arraysize =0;
		index += getArray(&data[index],&arraysize,DAR);
		tmpobj.poweroff_para_obj.collect_para_obj.tsaarr.num = arraysize;
		for(i=0;i<arraysize;i++)
		{
			index += getOctetstring(1,&data[index],tmpobj.poweroff_para_obj.collect_para_obj.tsaarr.meter_tas[i].addr,DAR);
		}
	}
	if(oad.attrindex != 0x01){
		index += getStructure(&data[index],NULL,DAR);
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
	index += getStructure(&data[index],NULL,DAR);
	index += getDouble(&data[index],(INT8U *)&tmp310c.poweroffset_obj.power_offset);
	fprintf(stderr,"data[index]=%02x %02x \n",data[index],data[index+1]);
	index += getUnsigned(&data[index],(INT8U *)&tmp310c.poweroffset_obj.task_no,DAR);
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
	index += getStructure(&data[index],NULL,DAR);
	index += getDouble(&data[index],(INT8U *)&tmp310d.poweroffset_obj.power_offset);
	index += getUnsigned(&data[index],(INT8U *)&tmp310d.poweroffset_obj.task_no,DAR);
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
	index += getStructure(&data[index],NULL,DAR);
	index += getTI(1,&data[index],&tmp310e.powerstoppara_obj.power_offset);
	index += getUnsigned(&data[index],(INT8U *)&tmp310e.powerstoppara_obj.task_no,DAR);
	fprintf(stderr,"\n电能表停走事件：属性6 阈值=%d 单位=%d",tmp310e.powerstoppara_obj.power_offset.interval,tmp310e.powerstoppara_obj.power_offset.units);
	*DAR = saveCoverClass(oad.OI,0,&tmp310e,sizeof(tmp310e),event_para_save);
	return index;
}

INT16U set310f(OAD oad,INT8U *data,INT8U *DAR)		//终端抄表失败  属性6
{
	Event310F_Object tmp310f={};
	int		index=0;

	readCoverClass(oad.OI,0,&tmp310f,sizeof(tmp310f),event_para_save);
	index += getStructure(&data[index],NULL,DAR);
	index += getUnsigned(&data[index],(INT8U *)&tmp310f.collectfail_obj.retry_nums,DAR);
	index += getUnsigned(&data[index],(INT8U *)&tmp310f.collectfail_obj.task_no,DAR);
	fprintf(stderr,"\n终端抄表失败事件：属性6 重试轮次=%d ",tmp310f.collectfail_obj.retry_nums);
	*DAR = saveCoverClass(oad.OI,0,&tmp310f,sizeof(tmp310f),event_para_save);
	return index;
}

INT16U set3110(OAD oad,INT8U *data,INT8U *DAR)		//月通信流量超限  属性6
{
	int		index=0;
	Event3110_Object tmpobj={};

	readCoverClass(oad.OI,0,&tmpobj,sizeof(tmpobj),event_para_save);
	index += getStructure(&data[index],NULL,DAR);
	index += getDouble(&data[index],(INT8U *)&tmpobj.Monthtrans_obj.month_offset);
	fprintf(stderr,"\n月通信流量限值事件：属性6　通信流量限值=%d ",tmpobj.Monthtrans_obj.month_offset);
	*DAR = saveCoverClass(oad.OI,0,&tmpobj,sizeof(tmpobj),event_para_save);
	return index;
}

INT16U set311c(OAD oad,INT8U *data,INT8U *DAR)		//电能表数据变更监控记录
{
	int		index=0;
	Event311C_Object tmpobj={};

	readCoverClass(oad.OI,0,&tmpobj,sizeof(tmpobj),event_para_save);
	index += getStructure(&data[index],NULL,DAR);
	index += getUnsigned(&data[index],(INT8U *)&tmpobj.task_para.task_no,DAR);
	fprintf(stderr,"\n电能表数据变更监控记录 关联采集任务号=%d ",tmpobj.task_para.task_no);
	*DAR = saveCoverClass(oad.OI,0,&tmpobj,sizeof(tmpobj),event_para_save);
	return index;
}


/////////////////////////////////////////////////////////////////////////////
int Set_4000_att2(INT8U *data,INT8U *DAR)
{
	DateTimeBCD datetime={};
	int		index=0;

//	DataTimeGet(&datetime);
	index += getDateTimeS(1,data,(INT8U *)&datetime,DAR);
	if(*DAR==success) {	//时间合法
		setsystime(datetime);
	}
//	sleep(2);		//延时2秒，确保台体测试过程中，修改时间设置成功
	return index;
}

INT16U set4000(OAD oad,INT8U *data,INT8U *DAR)
{
	CLASS_4000	class_tmp={};
	int		index=0;

	*DAR = success;
	switch(oad.attflg) {
	case 2:
		index += Set_4000_att2(&data[index],DAR);
		break;
	case 3://校时模式
		readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4000),para_vari_save);
		index += getEnum(1,&data[index],&class_tmp.type);
		*DAR = getEnumValid(class_tmp.type,MAIN,GPS,OTHER);
		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4000),para_vari_save);
		}
		break;
	case 4:		//精准校时模式
		readCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4000),para_vari_save);
		index += getStructure(&data[index],NULL,DAR);
		index += getUnsigned(&data[index],&class_tmp.hearbeatnum,DAR);
		index += getUnsigned(&data[index],&class_tmp.tichu_max,DAR);
		index += getUnsigned(&data[index],&class_tmp.tichu_min,DAR);
		index += getUnsigned(&data[index],&class_tmp.delay,DAR);
		index += getUnsigned(&data[index],&class_tmp.num_min,DAR);
		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class_tmp,sizeof(CLASS_4000),para_vari_save);
		}
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
		index += getOctetstring(1,&data[index],(INT8U *)&class_addr.curstom_num,DAR);
		if(*DAR == success)
		{
			datalen = class_addr.curstom_num[0]+1;
			fprintf(stderr,"\n设置 datalen=%d",datalen);
			for(i=0;i<datalen;i++) {
				fprintf(stderr,"%02x ",class_addr.curstom_num[i]);
			}
			*DAR = saveCoverClass(oad.OI,0,&class_addr,sizeof(CLASS_4001_4002_4003),para_vari_save);
			*DAR = saveCoverClass(oad.OI,0,&class_addr,sizeof(CLASS_4001_4002_4003), para_init_save);
		}
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
		index += getStructure(&data[index],NULL,DAR);
		index += getStructure(&data[index],NULL,DAR);

		index += getEnum(1,&data[index],&class4004.jing.fangwei);
		*DAR = getEnumValid(class4004.jing.fangwei,E_S,W_N,OTHERSTATUS);
		if(*DAR == success)
		{
			index += getUnsigned(&data[index],&class4004.jing.du,DAR);
			index += getUnsigned(&data[index],&class4004.jing.fen,DAR);
			index += getUnsigned(&data[index],&class4004.jing.miao,DAR);
			index += getStructure(&data[index],NULL,DAR);
			index += getEnum(1,&data[index],&class4004.wei.fangwei);
			*DAR = getEnumValid(class4004.wei.fangwei,E_S,W_N,OTHERSTATUS);
			if(*DAR == success)
			{
				index += getUnsigned(&data[index],&class4004.wei.du,DAR);
				index += getUnsigned(&data[index],&class4004.wei.fen,DAR);
				index += getUnsigned(&data[index],&class4004.wei.miao,DAR);
				index += getDouble(&data[index],(INT8U *)&class4004.heigh);
				fprintf(stderr,"\n【精度】方位 %d  度 %d  分 %d  秒 %d",class4004.jing.fangwei,class4004.jing.du,class4004.jing.fen,class4004.jing.miao);
				fprintf(stderr,"\n【纬度】方位 %d  度 %d  分 %d  秒 %d",class4004.jing.fangwei,class4004.jing.du,class4004.jing.fen,class4004.jing.miao);
				fprintf(stderr,"\n【高度】%d",class4004.heigh);
				*DAR = saveCoverClass(oad.OI,0,&class4004,sizeof(CLASS_4004),para_vari_save);
			}
		}
	}
	return index;
}
INT16U set4005(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0;
	CLASS_4005 class4005={};
	memset(&class4005,0,sizeof(CLASS_4005));
	readCoverClass(oad.OI,0,&class4005,sizeof(CLASS_4005),para_vari_save);
	if (oad.attflg == 2 )
	{
        INT8U arraysize = 0; //数组个数
        index += getArray(&data[index], &arraysize,DAR);
        if(arraysize>20)
        	arraysize = 20;
        class4005.num = arraysize;
        INT8U i=0;
        fprintf(stderr,"arraysize = %d \n",arraysize);
        for(i=0;i<arraysize;i++)
        {
			INT8U len=getOctetstring(1,&data[index],&class4005.addr[i][0],DAR);
			if(*DAR == success)
			{
				fprintf(stderr,"len=%d \n",len);
				index +=len;
			}
			else
				break;
        }
        int n=0,m=0;
        fprintf(stderr,"class4005.num=%d \n",class4005.num);
        for(n=0;n<class4005.num;n++)
        {
           fprintf(stderr,"NO.%d:",n);
           for(m=0;m<class4005.addr[n][0];m++)
        	   fprintf(stderr,"%02x ",class4005.addr[n][m+1]);
           fprintf(stderr,"\n");
        }
		if(*DAR == success)
			*DAR = saveCoverClass(oad.OI,0,&class4005,sizeof(CLASS_4005),para_vari_save);
	}
	return index;
}

INT16U set4006(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U index=0;
	CLASS_4006 class4006={};
	memset(&class4006,0,sizeof(CLASS_4006));
	readCoverClass(oad.OI,0,&class4006,sizeof(CLASS_4006),para_vari_save);
	switch(oad.attflg) {
	case 2:
		index += getStructure(&data[index],NULL,DAR);
		index += getEnum(1,&data[index],&class4006.clocksource);
		*DAR=getEnumValid(class4006.clocksource,0,4,0xff);
		if(*DAR == success)
		{
			index += getEnum(1,&data[index],&class4006.state);
			*DAR=getEnumValid(class4006.state,0,1,0xff);
			if(*DAR == success)
			{
				fprintf(stderr,"\n【时钟源】%d",class4006.clocksource);
				fprintf(stderr,"\n【状态】 %d",class4006.state);
				*DAR = saveCoverClass(oad.OI,0,&class4006,sizeof(CLASS_4006),para_vari_save);
			}
		}
		break;
	case 127:
	case 128:
		getEnum(0,data,&class4006.state);
		*DAR = saveCoverClass(0x4006,0,&class4006,sizeof(CLASS_4006),para_vari_save);
		break;
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
		index += getStructure(&data[index],NULL,DAR);
		index += getUnsigned(&data[index],&class4007.poweon_showtime,DAR);
		index += getLongUnsigned(&data[index],(INT8U *)&class4007.lcdlight_time);
		index += getLongUnsigned(&data[index],(INT8U *)&class4007.looklight_time);
		index += getLongUnsigned(&data[index],(INT8U *)&class4007.poweron_maxtime);
		index += getLongUnsigned(&data[index],(INT8U *)&class4007.poweroff_maxtime);
		index += getUnsigned(&data[index],&class4007.energydata_dec,DAR);
		index += getUnsigned(&data[index],&class4007.powerdata_dec,DAR);

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

INT16U set4024(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0;
	if (oad.attflg == 2 )
	{
		int state = data[1];
		fprintf(stderr, "剔除状态变更 %d\n", state);
		memp->ctrls.c4024 = state;
		//测试要求，不知是为什么
		memp->ctrls.c8100.v = 5000;
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
		index += getStructure(&data[index],NULL,DAR);
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
		index += getVisibleString(&data[index],(INT8U *)&class4103.assetcode,DAR);
		if(*DAR!=success)
			return 0;
		fprintf(stderr,"\n【资产管理编码】%d :",class4103.assetcode[0]);
		for(i=0;i<class4103.assetcode[0];i++)
			fprintf(stderr,"%02x ",class4103.assetcode[i+1]);
		fprintf(stderr,"\n");
		*DAR = saveCoverClass(oad.OI,0,&class4103,sizeof(CLASS_4103),para_vari_save);
	}
	return index;
}
INT16U set4202(OAD oad,INT8U *data,INT8U *DAR)
{
	int	index=0;
	CLASS_4202 class4202={};
	memset(&class4202,0,sizeof(CLASS_4202));
	fprintf(stderr,"\n==========%d",oad.attflg);
	readCoverClass(oad.OI,0,&class4202,sizeof(CLASS_4202),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getStructure(&data[index],NULL,DAR);
		index += getBool(&data[index],(INT8U *)&class4202.flag,DAR);
		index += getOAD(1,&data[index],&class4202.oad,DAR);
		*DAR = getPortValid(class4202.oad);
		if(*DAR == success)
		{
			index += getLongUnsigned(&data[index],(INT8U *)&class4202.total_timeout);
			index += getLongUnsigned(&data[index],(INT8U *)&class4202.byte_timeout);
			index += getUnsigned(&data[index],(INT8U *)&class4202.resendnum,DAR);
			if(*DAR!=success)
				return 0;
			index += getUnsigned(&data[index],(INT8U *)&class4202.cycle,DAR);
			if(*DAR!=success)
				return 0;
			index += getUnsigned(&data[index],(INT8U *)&class4202.portnum,DAR);
			if(*DAR!=success)
				return 0;
			index += getArray(&data[index],(INT8U *)&class4202.tsanum,DAR);
			int i=0;
			for(i=0;i<class4202.tsanum;i++)
			{
				index += getOctetstring(1,&data[index],(INT8U *)&class4202.tsa[i][0],DAR);
				if(*DAR !=success)
					break;
			}
		   *DAR = saveCoverClass(oad.OI,0,&class4202,sizeof(CLASS_4202),para_vari_save);
		}
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
		index += getStructure(&data[index],NULL,DAR);
		index += getTime(1,&data[index],(INT8U *)&class4204.startime,DAR);
		if(*DAR == success)
		{
			index += getBool(&data[index],(INT8U *)&class4204.enable,DAR);
			fprintf(stderr,"\n【终端广播校时,属性2】:");
			fprintf(stderr,"\ntime : %d %d %d",class4204.startime[0],class4204.startime[1],class4204.startime[2]);
			fprintf(stderr,"\nenable: %d",class4204.enable);
			fprintf(stderr,"\n");
			*DAR = saveCoverClass(oad.OI,0,&class4204,sizeof(CLASS_4204),para_vari_save);
		}
	}else if(oad.attflg == 3)
	{
		index += getStructure(&data[index],NULL,DAR);
		index += getUnsigned(&data[index],(INT8U *)&class4204.upleve,DAR);
		if(*DAR!=success)
			return 0;
		index += getTime(1,&data[index],(INT8U *)&class4204.startime1,DAR);
		if(*DAR!=success)
			return 0;
		index += getBool(&data[index],(INT8U *)&class4204.enable1,DAR);
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
	INT8U arraysize=0,i=0;
	CLASS19		class4300={};

	readCoverClass(oad.OI,0,&class4300,sizeof(CLASS19),para_vari_save);
	fprintf(stderr,"oi =%04x,attflg=%d\n",oad.OI,oad.attflg);

	switch(oad.attflg) {
	case 1:
	case 2: //设备描述符
	case 3: //版本信息
	case 4:
	case 5:
	case 6:
		*DAR=type_mismatch;
		break;
	case 7:	//允许跟随上报
		index += getBool(data,&class4300.follow_report,DAR);
		if(*DAR!=success)
			return 0;
		fprintf(stderr,"\n允许跟随上报 : %d",class4300.follow_report);
		*DAR = saveCoverClass(oad.OI,0,&class4300,sizeof(CLASS19),para_vari_save);
		break;
	case 8:	//允许\禁止终端主动上报
		index += getBool(data,&class4300.active_report,DAR);
		if(*DAR!=success)
			return 0;
		fprintf(stderr,"\n终端主动上报 : %d",class4300.active_report);
		*DAR = saveCoverClass(oad.OI,0,&class4300,sizeof(CLASS19),para_vari_save);
		break;
	case 9:	//允许与主站通话
		index += getBool(data,&class4300.talk_master,DAR);
		if(*DAR!=success)
			return 0;
		fprintf(stderr,"\n允许与主站通话 : %d",class4300.talk_master);
		*DAR = saveCoverClass(oad.OI,0,&class4300,sizeof(CLASS19),para_vari_save);
		break;
	case 10://上报通道
		index +=getArray(&data[index],(INT8U*)&arraysize,DAR);
		if(*DAR!=success)
			return 0;
		for(i=0;i<arraysize;i++)
		{
			index +=getOAD(1,&data[index],&class4300.oads[i],DAR);
			fprintf(stderr,"%d-oad.oi=%04x \n",i,class4300.oads[i].OI);
			if(*DAR!=success)
				return 0;
		}
		*DAR = saveCoverClass(oad.OI,0,&class4300,sizeof(CLASS19),para_vari_save);
		break;
	}
	return index;
}
INT16U set4400(OAD oad,INT8U *data,INT8U *DAR)
{
	switch(oad.attflg) {
	case 1:
	case 2: //设备描述符
	case 3: //版本信息
	case 4:
	case 5:
	case 6:
		*DAR=type_mismatch;
		break;
	}
	return 0;
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
		index += getStructure(&data[index],NULL,DAR);
		index += getEnum(1,&data[index],(INT8U *)&class4500.commconfig.workModel);
		*DAR=getEnumValid(class4500.commconfig.workModel,0,2,255);
		if(*DAR !=success)
			return 0;
		index += getEnum(1,&data[index],(INT8U *)&class4500.commconfig.onlineType);
		*DAR=getEnumValid(class4500.commconfig.onlineType,0,1,255);
		if(*DAR !=success)
			return 0;
		index += getEnum(1,&data[index],(INT8U *)&class4500.commconfig.connectType);
		*DAR=getEnumValid(class4500.commconfig.connectType,0,1,255);
		if(*DAR !=success)
			return 0;
		index += getEnum(1,&data[index],(INT8U *)&class4500.commconfig.appConnectType);
		*DAR=getEnumValid(class4500.commconfig.appConnectType,0,1,255);
		if(*DAR !=success)
			return 0;
		index += getArray(&data[index],(INT8U *)&class4500.commconfig.listenPortnum,DAR);
		if(*DAR !=success)
			return 0;
		if(class4500.commconfig.listenPortnum>5) {
			fprintf(stderr,"!!!!!!!!!越限 listenPortnum=%d\n",class4500.commconfig.listenPortnum);
			class4500.commconfig.listenPortnum = 5;
		}
		for(i=0;i<class4500.commconfig.listenPortnum;i++) {
			index += getLongUnsigned(&data[index],(INT8U *)&class4500.commconfig.listenPort[i]);
		}
		index += getVisibleString(&data[index],class4500.commconfig.apn,DAR);
		if(*DAR!=success) return 0;
		index += getVisibleString(&data[index],class4500.commconfig.userName,DAR);
		if(*DAR!=success) return 0;
		index += getVisibleString(&data[index],class4500.commconfig.passWord,DAR);
		if(*DAR!=success) return 0;
		index += getOctetstring(1,&data[index],class4500.commconfig.proxyIp,DAR);
		index += getLongUnsigned(&data[index],(INT8U *)&class4500.commconfig.proxyPort);
		index += getUnsigned(&data[index],(INT8U *)&class4500.commconfig.timeoutRtry,DAR);	//勘误更改
		index += getLongUnsigned(&data[index],(INT8U *)&class4500.commconfig.heartBeat);
		if(index>=sizeof(class4500.commconfig)) {
			*DAR = refuse_rw;
			return index;
		}
		break;
	case 3:		//主站通信参数表
		index += getArray(&data[index],(INT8U *)&class4500.master.masternum,DAR);
		if(*DAR !=success)
			return 0;
		if(class4500.master.masternum>4) {
			fprintf(stderr,"!!!!!!!!!越限 master.masternum=%d\n",class4500.master.masternum);
			class4500.master.masternum = 4;
		}
		for(i=0;i<class4500.master.masternum;i++) {
			index += getStructure(&data[index],NULL,DAR);
			index += getOctetstring(1,&data[index],class4500.master.master[i].ip,DAR);
			if(*DAR!=success) return 0;
			index += getLongUnsigned(&data[index],(INT8U *)&class4500.master.master[i].port);
		}
		if(index>=sizeof(class4500.master)) {
			*DAR = refuse_rw;
			return index;
		}
		break;
	case 4:		//短信通信参数表
		index += getStructure(&data[index],NULL,DAR);
		fprintf(stderr,"struct index=%d\n",index);
		index += getVisibleString(&data[index],(INT8U *)&class4500.sms.center,DAR);
		if(*DAR!=success)
			return 0;
		fprintf(stderr,"center index=%d %02x %02x\n",index,data[index],data[index+1]);
		index += getArray(&data[index],(INT8U *)&class4500.sms.masternum,DAR);
		if(class4500.sms.masternum>4) {
			fprintf(stderr,"!!!!!!!!!越限 sms.masternum=%d\n",class4500.sms.masternum);
			class4500.sms.masternum = 4;
		}
		for(i=0;i<class4500.sms.masternum;i++){
			index += getVisibleString(&data[index],(INT8U *)&class4500.sms.master[i],DAR);
			if(*DAR!=success) return 0;
		}
		index += getArray(&data[index],(INT8U *)&class4500.sms.destnum,DAR);
		if(class4500.sms.destnum>4) {
			fprintf(stderr,"!!!!!!!!!越限 sms.destnum=%d\n",class4500.sms.destnum);
			class4500.sms.destnum = 4;
		}
		for(i=0;i<class4500.sms.destnum;i++){
			index += getVisibleString(&data[index],(INT8U *)&class4500.sms.dest[i],DAR);
			if(*DAR!=success) return 0;
		}
		if(index>=sizeof(class4500.sms)) {
			*DAR = refuse_rw;
			return index;
		}
		break;

	case 10:
		index += getVisibleString(&data[index],&class4500.simkard[0],DAR);
		if(*DAR!=success) return 0;
		break;
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 11:
		*DAR=type_mismatch;
		return 0;
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
	switch(oad.attflg) {
	case 2:
		index += getStructure(&data[index],NULL,DAR);
		index += getEnum(1,&data[index],(INT8U *)&class4510.commconfig.workModel);
		index += getEnum(1,&data[index],(INT8U *)&class4510.commconfig.connectType);
		index += getEnum(1,&data[index],(INT8U *)&class4510.commconfig.appConnectType);
		index += getArray(&data[index],(INT8U *)&class4510.commconfig.listenPortnum,DAR);
		if(class4510.commconfig.listenPortnum>5) {
			fprintf(stderr,"!!!!!!!!!越限 listenPortnum=%d\n",class4510.commconfig.listenPortnum);
			class4510.commconfig.listenPortnum = 5;
		}
		for(i=0;i<class4510.commconfig.listenPortnum;i++) {
			index += getLongUnsigned(&data[index],(INT8U *)&class4510.commconfig.listenPort[i]);
		}
		index += getOctetstring(1,&data[index],class4510.commconfig.proxyIp,DAR);
		index += getLongUnsigned(&data[index],(INT8U *)&class4510.commconfig.proxyPort);
		index += getUnsigned(&data[index],(INT8U *)&class4510.commconfig.timeoutRtry,DAR);//勘误更改类型
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

	    fprintf(stderr, "\n主IP %d.%d.%d.%d :%d\n", class4510.master.master[0].ip[1], class4510.master.master[0].ip[2],
	            class4510.master.master[0].ip[3],
	            class4510.master.master[0].ip[4], class4510.master.master[0].port);
		if(index>=sizeof(class4510.commconfig)) {
			*DAR = refuse_rw;
			return index;
		}
	break;
	case 3://主站通信参数表
		index += getArray(&data[index],(INT8U *)&class4510.master.masternum,DAR);
		if(class4510.master.masternum>4) {
			class4510.master.masternum = 4;
			syslog(LOG_ERR,"主站设置端口数量%d大于限值%d\n",class4510.master.masternum,4);
		}
		for(i=0;i<class4510.master.masternum;i++) {
			index += getStructure(&data[index],NULL,DAR);
			index += getOctetstring(1,&data[index],class4510.master.master[i].ip,DAR);
			index += getLongUnsigned(&data[index],(INT8U *)&class4510.master.master[i].port);
		}
		break;
	case 4:
		index += getStructure(&data[index],NULL,DAR);
		index += getEnum(1,&data[index],(INT8U *)&class4510.IP.ipConfigType);
		index += getOctetstring(1,&data[index],class4510.IP.ip,DAR);
		index += getOctetstring(1,&data[index],class4510.IP.subnet_mask,DAR);
		index += getOctetstring(1,&data[index],class4510.IP.gateway,DAR);
		index += getVisibleString(&data[index],class4510.IP.username_pppoe,DAR);
		index += getVisibleString(&data[index],class4510.IP.password_pppoe,DAR);
		writeIpSh(class4510.IP.ip,class4510.IP.subnet_mask);
		break;
	}
	*DAR = saveCoverClass(oad.OI,0,&class4510,sizeof(CLASS26),para_vari_save);

	return index;
}



/////////////////////////////////////////////////////////////////////////////
int setf101(OAD oad,INT8U *data,INT8U *DAR)
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
		index += getArray(&data[index],(INT8U*)&f101.modelnum,DAR);
		index += getStructure(&data[index],NULL,DAR);
		for(i=0;i<f101.modelnum;i++) {
			index += getOI(1,&data[index],&f101.modelpara[i].oi);
			index += getLongUnsigned(&data[index],(INT8U *)&f101.modelpara[i].model);
		}
	}
	*DAR = saveCoverClass(0xf101,0,&f101,sizeof(CLASS_F101),para_vari_save);
	fprintf(stderr,"\n安全模式选择 : %02x \n",f101.active);

	return index;
}

int	Set_F200(OI_698 oi,INT8U *data,INT8U *DAR)
{
	int	 index=0;
	CLASS_f201	f201={};
	OAD		oad={};

	readCoverClass(oi,0,&f201,sizeof(CLASS_f201),para_vari_save);
	index += getStructure(&data[index],NULL,DAR);
	index += getOAD(1,&data[index],&oad,DAR);
	index += getCOMDCB(1,&data[index],&f201.devpara,DAR);
	index += getEnum(1,&data[index],&f201.devfunc);
	fprintf(stderr,"DAR=%d  return %d\n",*DAR,index);
	if(*DAR==success) {
		*DAR = saveCoverClass(oi,0,&f201,sizeof(CLASS_f201),para_vari_save);
	}
	return index;
}

int	Set_F201(OI_698 oi,INT8U *data,INT8U *DAR)
{
	INT8U	com = 0;
	int	 index=0;
	CLASS_f201	f201[3]={};
	OAD		oad={};

	readCoverClass(oi,0,&f201,sizeof(f201),para_vari_save);
	index += getStructure(&data[index],NULL,DAR);
	index += getOAD(1,&data[index],&oad,DAR);
	if(oad.attrindex>=1 && oad.attrindex<=3) {
		com = oad.attrindex-1;
	}else {
		*DAR = boundry_over;
	}
	index += getCOMDCB(1,&data[index],&f201[com].devpara,DAR);
	index += getEnum(1,&data[index],&f201[com].devfunc);
//	fprintf(stderr,"com=%d DAR=%d  return %d\n",com,*DAR,index);
	if(*DAR==success) {
		*DAR = saveCoverClass(oi,0,&f201,sizeof(f201),para_vari_save);
	}
	return index;
}

int	Set_F202(OI_698 oi,INT8U *data,INT8U *DAR)
{
	int	 index=0;
	CLASS_f202	f202={};
	OAD		oad={};

	readCoverClass(oi,0,&f202,sizeof(CLASS_f202),para_vari_save);
	index += getStructure(&data[index],NULL,DAR);
	index += getOAD(1,&data[index],&oad,DAR);
	index += getCOMDCB(1,&data[index],&f202.devpara,DAR);
	if(*DAR==success) {
		*DAR = saveCoverClass(oi,0,&f202,sizeof(CLASS_f202),para_vari_save);
	}
	return index;
}

int setf203(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U index=0;
	CLASS_f203	f203={};
	memset(&f203,0,sizeof(CLASS_f203));
	readCoverClass(0xf203,0,&f203,sizeof(CLASS_f203),para_vari_save);
	if ( oad.attflg == 4 )//配置参数
	{
		index += getStructure(&data[index],NULL,DAR);
		index += getBitString(1,&data[index],(INT8U *)&f203.state4.StateAcessFlag);
		index += getBitString(1,&data[index],(INT8U *)&f203.state4.StatePropFlag);
		*DAR = saveCoverClass(0xf203,0,&f203,sizeof(CLASS_f203),para_vari_save);
		fprintf(stderr,"\n状态量配置参数 : 接入标志 %02x  属性标志 %02x \n",f203.state4.StateAcessFlag,f203.state4.StatePropFlag);
	}
	return index;
}

int	Set_F209(OAD setoad,INT8U *data,INT8U *DAR)
{
	int	 index=0;
	CLASS_f209	f209={};

	switch(setoad.attflg) {
	case 2:
		readCoverClass(setoad.OI,0,&f209,sizeof(CLASS_f209),para_vari_save);
		index += getCOMDCB(1,&data[index],&f209.para.devpara,DAR);
		fprintf(stderr,"set baud=%d\n",f209.para.devpara.baud);
		*DAR = saveCoverClass(setoad.OI,0,&f209,sizeof(CLASS_f209),para_vari_save);
		break;
	case 5:
		break;
	case 6:
		break;
	}
	return index;
}

