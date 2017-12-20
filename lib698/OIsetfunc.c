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
extern int getDouble(INT8U *source,INT8U *dest);
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
			index += getTSA(1,&data[index],tmpobj.poweroff_para_obj.collect_para_obj.tsaarr.meter_tas[i].addr,DAR);
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
		index += getOctetstring(1,OCTET_STRING_LEN-1,&data[index],(INT8U *)&class_addr.curstom_num[1],&class_addr.curstom_num[0],DAR);
		if(*DAR == success)
		{
			datalen = class_addr.curstom_num[0]+1;
			fprintf(stderr,"\n设置 datalen=%d",datalen);
			for(i=0;i<datalen;i++) {
				fprintf(stderr,"%02x ",class_addr.curstom_num[i]);
			}
			*DAR = saveCoverClass(oad.OI,0,&class_addr,sizeof(CLASS_4001_4002_4003),para_vari_save);
			*DAR = saveCoverClass(oad.OI,0,&class_addr,sizeof(CLASS_4001_4002_4003), para_init_save);
			writeIdFile(class_addr);//写入ID备份文件
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
			INT8U len=getOctetstring(1,OCTET_STRING_LEN-1,&data[index],&class4005.addr[i][1],&class4005.addr[i][0],DAR);
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
INT16U set4008_4009_400A_400B(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0;
	CLASS_4008_4009_400A_400B class4008_4009_400A_400B={};
	memset(&class4008_4009_400A_400B,0,sizeof(CLASS_4008_4009_400A_400B));
	readCoverClass(oad.OI,0,&class4008_4009_400A_400B,sizeof(CLASS_4008_4009_400A_400B),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getDateTimeS(1,data,(INT8U *)&class4008_4009_400A_400B.datetime_s,DAR);
		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class4008_4009_400A_400B,sizeof(CLASS_4008_4009_400A_400B),para_vari_save);
		}
	}
	return index;
}
INT16U set400c(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0;
	int ret=0;
	CLASS_400C class400c={};

	memset(&class400c,0,sizeof(CLASS_400C));
	readCoverClass(oad.OI,0,&class400c,sizeof(CLASS_400C),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getStructure(&data[index],NULL,DAR);
		index += getUnsigned(&data[index],&class400c.year_zone,DAR);
		ret = rangeJudge("年时区数",class400c.year_zone,0,14);
		if(ret == -1) *DAR = boundry_over;
		index += getUnsigned(&data[index],&class400c.day_interval,DAR);
		ret = rangeJudge("日时段表数",class400c.day_interval,0,8);
		if(ret == -1) *DAR = boundry_over;
		index += getUnsigned(&data[index],&class400c.day_change,DAR);
		ret = rangeJudge("日时段数",class400c.day_change,0,14);
		if(ret == -1) *DAR = boundry_over;
		index += getUnsigned(&data[index],&class400c.rate,DAR);
		ret = rangeJudge("费率数",class400c.rate,0,63);
		if(ret == -1) *DAR = boundry_over;
		index += getUnsigned(&data[index],&class400c.public_holiday,DAR);
		ret = rangeJudge("公共假日数",class400c.public_holiday,0,254);
		if(ret == -1) *DAR = boundry_over;
		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class400c,sizeof(CLASS_400C),para_vari_save);
		}
	}
	return index;
}
INT16U set400D_400E_400F_4010(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0;
	CLASS_400D_400E_400F_4010 class400D_400E_400F_4010={};

	memset(&class400D_400E_400F_4010,0,sizeof(CLASS_400D_400E_400F_4010));
	readCoverClass(oad.OI,0,&class400D_400E_400F_4010,sizeof(CLASS_400D_400E_400F_4010),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getUnsigned(&data[index],&class400D_400E_400F_4010.num,DAR);
		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class400D_400E_400F_4010,sizeof(CLASS_400D_400E_400F_4010),para_vari_save);
		}
	}
	return index;
}
INT16U set4011(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0;
	int i=0;
	CLASS_4011 class4011={};

	memset(&class4011,0,sizeof(class4011));
	readCoverClass(oad.OI,0,&class4011,sizeof(CLASS_4014),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getArray(&data[index],&class4011.holidaynum,DAR);
		class4011.holidaynum = limitJudge("当前公共假日数",MAX_PUBLIC_HOLIDAY_NUM,class4011.holidaynum);
		for(i=0;i<class4011.holidaynum;i++) {
			index += getStructure(&data[index],NULL,DAR);
			index += getDateTimeS(1,data,(INT8U *)&class4011.holiday[i].datetime,DAR);
			index += getUnsigned(&data[index],&class4011.holiday[i].tableno,DAR);
		}
		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class4011,sizeof(CLASS_4011),para_vari_save);
		}
	}
	return index;
}
INT16U set4012(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U index=0;
	CLASS_4012	class4012={};
	memset(&class4012,0,sizeof(CLASS_4012));
	readCoverClass(oad.OI,0,&class4012,sizeof(CLASS_4012),para_vari_save);
	if ( oad.attflg == 2 )
	{
		index += getBitString(1,&data[index],(INT8U *)&class4012.restdayflag);
//		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class4012,sizeof(CLASS_4012),para_vari_save);
//		}
	}
	return index;
}
INT16U set4013(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U index=0;
	CLASS_4013	class4013={};
	memset(&class4013,0,sizeof(CLASS_4013));
	readCoverClass(oad.OI,0,&class4013,sizeof(CLASS_4013),para_vari_save);
	if ( oad.attflg == 2 )
	{
		index += getUnsigned(&data[index],&class4013.tableno,DAR);
		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class4013,sizeof(CLASS_4013),para_vari_save);
		}
	}
	return index;
}
INT16U set4014(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0;
	int i=0;
	CLASS_4014 class4014={};

	memset(&class4014,0,sizeof(CLASS_4014));
	readCoverClass(oad.OI,0,&class4014,sizeof(CLASS_4014),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getArray(&data[index],&class4014.zonenum,DAR);
		class4014.zonenum = limitJudge("当前套时区数",MAX_PERIOD_RATE,class4014.zonenum);
		for(i=0;i<class4014.zonenum;i++) {
			index += getStructure(&data[index],NULL,DAR);
			index += getUnsigned(&data[index],&class4014.time_zone[i].month,DAR);
			index += getUnsigned(&data[index],&class4014.time_zone[i].day,DAR);
			index += getUnsigned(&data[index],&class4014.time_zone[i].tableno,DAR);
		}
		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class4014,sizeof(CLASS_4014),para_vari_save);
		}
	}
	return index;
}

INT16U set4016(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0;
	int i=0,j=0;
	CLASS_4016 class4016={};

	memset(&class4016,0,sizeof(CLASS_4016));
	readCoverClass(oad.OI,0,&class4016,sizeof(CLASS_4016),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getArray(&data[index],&class4016.day_num,DAR);
		class4016.day_num = limitJudge("日时段表",MAX_PERIOD_RATE,class4016.day_num);
		fprintf(stderr,"day_num = %d\n",class4016.day_num);
		for(i=0;i<class4016.day_num;i++) {
			index += getArray(&data[index],&class4016.zone_num,DAR);
			fprintf(stderr,"zone_num = %d\n",class4016.zone_num);
			class4016.zone_num = limitJudge("时段",MAX_PERIOD_RATE,class4016.zone_num);
			for(j=0;j<class4016.zone_num;j++) {
				index += getStructure(&data[index],NULL,DAR);
				index += getUnsigned(&data[index],&class4016.Period_Rate[i][j].hour,DAR);
				index += getUnsigned(&data[index],&class4016.Period_Rate[i][j].min,DAR);
				index += getUnsigned(&data[index],&class4016.Period_Rate[i][j].rateno,DAR);
			}
		}
		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class4016,sizeof(CLASS_4016),para_vari_save);
		}
	}
	return index;
}
INT16U set401A(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0;
	int i=0;
	CLASS_401A class401a={};
	CLASS_400D_400E_400F_4010	class400d={};

	memset(&class401a,0,sizeof(CLASS_401A));
	memset(&class400d,0,sizeof(CLASS_400D_400E_400F_4010));
	readCoverClass(oad.OI,0,&class401a,sizeof(CLASS_401A),para_vari_save);
	readCoverClass(0x400D,0,&class400d,sizeof(CLASS_401A),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getStructure(&data[index],NULL,DAR);

		for(i=0;i<class400d.num;i++) {
			index += getDouble(&data[index],(INT8U *)&class401a.ladder_value[i]);
		}
		for(i=0;i<class400d.num;i++) {
			index += getDouble(&data[index],(INT8U *)&class401a.ladder_price[i]);
		}
		for(i=0;i<class400d.num;i++) {
			index += getStructure(&data[index],NULL,DAR);
			index += getUnsigned(&data[index],&class401a.account_day[i].month,DAR);
			index += getUnsigned(&data[index],&class401a.account_day[i].day,DAR);
			index += getUnsigned(&data[index],&class401a.account_day[i].hour,DAR);
		}

		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class401a,sizeof(CLASS_401A),para_vari_save);
		}
	}
	return index;
}
INT16U set401C(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0;
	CLASS_401C_401D class401c={};

	memset(&class401c,0,sizeof(CLASS_401C_401D));
	readCoverClass(oad.OI,0,&class401c,sizeof(CLASS_401C_401D),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getDouble(&data[index],(INT8U *)&class401c.k);
//		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class401c,sizeof(CLASS_401C_401D),para_vari_save);
//		}
	}
	return index;
}
INT16U set401E(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0;
	CLASS_401E class401e={};

	memset(&class401e,0,sizeof(CLASS_401E));
	readCoverClass(oad.OI,0,&class401e,sizeof(CLASS_401E),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getStructure(&data[index],NULL,DAR);
		index += getDouble(&data[index],(INT8U *)&class401e.alarm_amount_limit_1);
		index += getDouble(&data[index],(INT8U *)&class401e.alarm_amount_limit_2);

		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class401e,sizeof(CLASS_401E),para_vari_save);
		}
	}
	return index;
}
INT16U set401F(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0;
	CLASS_401F class401f={};

	memset(&class401f,0,sizeof(CLASS_401F));
	readCoverClass(oad.OI,0,&class401f,sizeof(CLASS_401F),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getStructure(&data[index],NULL,DAR);
		index += getDouble(&data[index],(INT8U *)&class401f.overdraft_amount_limit);
		index += getDouble(&data[index],(INT8U *)&class401f.hoarding_amount_limit);
		index += getDouble(&data[index],(INT8U *)&class401f.switchin_amount_limit);

		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class401f,sizeof(CLASS_401F),para_vari_save);
		}
	}
	return index;
}
INT16U set4020(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0;
	CLASS_4020 class4020={};

	memset(&class4020,0,sizeof(CLASS_401E));
	readCoverClass(oad.OI,0,&class4020,sizeof(CLASS_401E),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getStructure(&data[index],NULL,DAR);
		index += getDouble(&data[index],(INT8U *)&class4020.alarm_electricity_limit_1);
		index += getDouble(&data[index],(INT8U *)&class4020.alarm_electricity_limit_2);

		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class4020,sizeof(CLASS_401E),para_vari_save);
		}
	}
	return index;
}
INT16U set4021(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0;
	CLASS_4021 class4021={};

	memset(&class4021,0,sizeof(CLASS_4021));
	readCoverClass(oad.OI,0,&class4021,sizeof(CLASS_4021),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getStructure(&data[index],NULL,DAR);
		index += getDouble(&data[index],(INT8U *)&class4021.hoarding_electricity_limit);
		index += getDouble(&data[index],(INT8U *)&class4021.overdraft_electricity_limit);
		index += getDouble(&data[index],(INT8U *)&class4021.switchin_electricity_limit);

		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class4021,sizeof(CLASS_4021),para_vari_save);
		}
	}
	return index;
}
INT16U set4022(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U index=0;
	CLASS_4022	class4022={};
	memset(&class4022,0,sizeof(CLASS_4022));
	readCoverClass(oad.OI,0,&class4022,sizeof(CLASS_4022),para_vari_save);
	if ( oad.attflg == 2 )
	{
		index += getBitString(1,&data[index],(INT8U *)&class4022.card_flag);
//		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class4022,sizeof(CLASS_4022),para_vari_save);
//		}
	}
	return index;
}
INT16U set4023(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0;
	CLASS_4023 class4023={};

	memset(&class4023,0,sizeof(CLASS_4023));
	readCoverClass(oad.OI,0,&class4023,sizeof(CLASS_4023),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getLongUnsigned(&data[index],(INT8U *)&class4023.effective_duration);
//		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class4023,sizeof(CLASS_4023),para_vari_save);
//		}
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

INT16U set4100_4101_4102(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0;
	CLASS_4100_4101_4102 class4100_4101_4102={};

	memset(&class4100_4101_4102,0,sizeof(CLASS_4100_4101_4102));
	readCoverClass(oad.OI,0,&class4100_4101_4102,sizeof(CLASS_4100_4101_4102),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getUnsigned(&data[index],&class4100_4101_4102.data,DAR);
		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class4100_4101_4102,sizeof(CLASS_4100_4101_4102),para_vari_save);
		}
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
		index += getVisibleString(1,40,&data[index],(INT8U *)&class4103.assetcode,DAR);
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
INT16U set4104(OAD oad,INT8U *data,INT8U *DAR)
{
	int	index=0;
	CLASS_4104 class4104={};

	memset(&class4104,0,sizeof(CLASS_4104));
	readCoverClass(oad.OI,0,&class4104,sizeof(CLASS_4104),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getVisibleString(1,VISIBLE_STRING_LEN,&data[index],(INT8U *)&class4104.ratedU,DAR);
		if(*DAR!=success)
			return 0;
		*DAR = saveCoverClass(oad.OI,0,&class4104,sizeof(CLASS_4104),para_vari_save);
	}
	return index;
}
INT16U set4105(OAD oad,INT8U *data,INT8U *DAR)
{
	int	index=0;
	CLASS_4105 class4105={};

	memset(&class4105,0,sizeof(CLASS_4105));
	readCoverClass(oad.OI,0,&class4105,sizeof(CLASS_4105),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getVisibleString(1,VISIBLE_STRING_LEN,&data[index],(INT8U *)&class4105.ratedI,DAR);
		if(*DAR!=success)
			return 0;
		*DAR = saveCoverClass(oad.OI,0,&class4105,sizeof(CLASS_4105),para_vari_save);
	}
	return index;
}
INT16U set4106(OAD oad,INT8U *data,INT8U *DAR)
{
	int	index=0;
	CLASS_4106 class4106={};

	memset(&class4106,0,sizeof(CLASS_4106));
	readCoverClass(oad.OI,0,&class4106,sizeof(CLASS_4106),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getVisibleString(1,VISIBLE_STRING_LEN,&data[index],(INT8U *)&class4106.maxI,DAR);
		if(*DAR!=success)
			return 0;
		*DAR = saveCoverClass(oad.OI,0,&class4106,sizeof(CLASS_4106),para_vari_save);
	}
	return index;
}
INT16U set4107_4108(OAD oad,INT8U *data,INT8U *DAR)
{
	int	index=0;
	CLASS_4107_4108 class4107_4108={};

	memset(&class4107_4108,0,sizeof(CLASS_4107_4108));
	readCoverClass(oad.OI,0,&class4107_4108,sizeof(CLASS_4107_4108),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getVisibleString(1,VISIBLE_STRING_LEN,&data[index],(INT8U *)&class4107_4108.accuracy_class,DAR);
		if(*DAR!=success)
			return 0;
		*DAR = saveCoverClass(oad.OI,0,&class4107_4108,sizeof(CLASS_4107_4108),para_vari_save);
	}
	return index;
}
INT16U set4109_410A(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0;
	CLASS_4109_410A class4109_410A={};

	memset(&class4109_410A,0,sizeof(CLASS_4109_410A));
	readCoverClass(oad.OI,0,&class4109_410A,sizeof(CLASS_4109_410A),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getDouble(&data[index],(INT8U *)&class4109_410A.constant);
		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class4109_410A,sizeof(CLASS_4109_410A),para_vari_save);
		}
	}
	return index;
}
INT16U set410B(OAD oad,INT8U *data,INT8U *DAR)
{
	int	index=0;
	CLASS_410B class410B={};

	memset(&class410B,0,sizeof(CLASS_410B));
	readCoverClass(oad.OI,0,&class410B,sizeof(CLASS_410B),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getVisibleString(1,VISIBLE_STRING_LEN,&data[index],(INT8U *)&class410B.meter_type,DAR);
		if(*DAR!=success)
			return 0;
		*DAR = saveCoverClass(oad.OI,0,&class410B,sizeof(CLASS_410B),para_vari_save);
	}
	return index;
}
INT16U set410C_410D_410E_410F(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0;
	CLASS_410C_410D_410E_410F class410C_410D_410E_410F={};
	memset(&class410C_410D_410E_410F,0,sizeof(CLASS_410C_410D_410E_410F));

	readCoverClass(oad.OI,0,&class410C_410D_410E_410F,sizeof(CLASS_410C_410D_410E_410F),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getStructure(&data[index],NULL,DAR);
		index += getLong(&data[index],&class410C_410D_410E_410F.ratio_A,DAR);
		index += getLong(&data[index],&class410C_410D_410E_410F.ratio_B,DAR);
		index += getLong(&data[index],&class410C_410D_410E_410F.ratio_C,DAR);
		if(*DAR==success) {
			*DAR = saveCoverClass(oad.OI,0,&class410C_410D_410E_410F,sizeof(CLASS_410C_410D_410E_410F),para_vari_save);
		}
	}
	return index;
}
INT16U set4110(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U index=0;
	CLASS_4110	class4110={};
	memset(&class4110,0,sizeof(CLASS_4110));
	readCoverClass(oad.OI,0,&class4110,sizeof(CLASS_4110),para_vari_save);
	if ( oad.attflg == 2 )
	{
		index += getBitString(1,&data[index],(INT8U *)&class4110.meter_running_character);
//		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class4110,sizeof(CLASS_4110),para_vari_save);
//		}
	}
	return index;
}
INT16U set4111(OAD oad,INT8U *data,INT8U *DAR)
{
	int	index=0;
	CLASS_4111 class4111={};

	memset(&class4111,0,sizeof(CLASS_4111));
	readCoverClass(oad.OI,0,&class4111,sizeof(CLASS_4111),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getVisibleString(1,VISIBLE_STRING_LEN,&data[index],(INT8U *)&class4111.soft_recordnumber,DAR);
		if(*DAR!=success)
			return 0;
		*DAR = saveCoverClass(oad.OI,0,&class4111,sizeof(CLASS_4111),para_vari_save);
	}
	return index;
}
INT16U set4112_4113_4114(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U index=0;
	CLASS_4112_4113_4114	class4112_4113_4114={};
	memset(&class4112_4113_4114,0,sizeof(CLASS_4112_4113_4114));
	readCoverClass(oad.OI,0,&class4112_4113_4114,sizeof(CLASS_4112_4113_4114),para_vari_save);
	if ( oad.attflg == 2 )
	{
		index += getBitString(1,&data[index],(INT8U *)&class4112_4113_4114.group_character);
//		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class4112_4113_4114,sizeof(CLASS_4112_4113_4114),para_vari_save);
//		}
	}
	return index;
}
INT16U set4116(OAD oad,INT8U *data,INT8U *DAR)
{
	int	index=0,i=0;;
	CLASS_4116 class4116={};
	memset(&class4116,0,sizeof(CLASS_4116));
	readCoverClass(oad.OI,0,&class4116,sizeof(CLASS_4116),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getArray(&data[index],(INT8U *)&class4116.day_num,DAR);
		for(i=0;i<class4116.day_num;i++)
		{
			index += getStructure(&data[index],NULL,DAR);
			index += getUnsigned(&data[index],(INT8U *)&class4116.accountdate[i].day,DAR);
			index += getUnsigned(&data[index],(INT8U *)&class4116.accountdate[i].hour,DAR);
			if(*DAR !=success)
				break;
		}
		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class4116,sizeof(CLASS_4116),para_vari_save);
		}
	}
	return index;
}
//
INT16U set4117(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U index=0;
	CLASS_4117	class4117={};
	memset(&class4117,0,sizeof(CLASS_4117));
	readCoverClass(oad.OI,0,&class4117,sizeof(CLASS_4117),para_vari_save);
	if ( oad.attflg == 2 )
	{
		index += getTI(1,&data[index],&class4117.freezeperiod);
//		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class4117,sizeof(CLASS_4117),para_vari_save);
//		}
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
				index += getTSA(1,&data[index],(INT8U *)&class4202.tsa[i][0],DAR);
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
		index += getInteger(&data[index],&class4204.upleve,DAR);
		index += getTime(1,&data[index],(INT8U *)&class4204.startime1,DAR);
		index += getBool(&data[index],(INT8U *)&class4204.enable1,DAR);
		fprintf(stderr,"\n【终端广播校时，属性3】:");
		fprintf(stderr,"\ntime : %d %d %d ",class4204.startime1[0],class4204.startime1[1],class4204.startime1[2]);
		fprintf(stderr,"\nenable: %d",class4204.enable1);
		fprintf(stderr,"\n误差 = %d",class4204.upleve);
		fprintf(stderr,"\n");
		if(*DAR == success) {
			*DAR = saveCoverClass(oad.OI,0,&class4204,sizeof(CLASS_4204),para_vari_save);
		}
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

INT16U set4401(OAD oad,INT8U *data,INT8U *DAR)
{
	int	index=0;
	CLASS_4401 class4401={};

	memset(&class4401,0,sizeof(CLASS_4401));
	readCoverClass(oad.OI,0,&class4401,sizeof(CLASS_4401),para_vari_save);
	if (oad.attflg == 2 )
	{
		index += getVisibleString(1,VISIBLE_STRING_LEN,&data[index],(INT8U *)&class4401.connect_pwd,DAR);
		if(*DAR!=success)
			return 0;
		*DAR = saveCoverClass(oad.OI,0,&class4401,sizeof(CLASS_4401),para_vari_save);
	}
	return index;
}

INT16U set4500(OAD oad,INT8U *data,INT8U *DAR)
{
	int index=0,i=0;
	CLASS25 class4500={};
	memset(&class4500,0,sizeof(CLASS25));

	readCoverClass(oad.OI,0,&class4500,sizeof(CLASS25),para_vari_save);
	fprintf(stderr,"\n先读出 主站IP %d.%d.%d.%d :%d\n",class4500.master.master[0].ip[1],class4500.master.master[0].ip[2],
			class4500.master.master[0].ip[3],class4500.master.master[0].ip[4],class4500.master.master[0].port);
//	syslog(LOG_NOTICE,"\n读出 主站IP %d.%d.%d.%d :%d [oad=%d]\n",class4500.master.master[0].ip[1],class4500.master.master[0].ip[2],
//			class4500.master.master[0].ip[3],class4500.master.master[0].ip[4],class4500.master.master[0].port,oad.attflg);

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
		index += getVisibleString(1,VISIBLE_STRING_LEN,&data[index],class4500.commconfig.apn,DAR);
		if(*DAR!=success) return 0;
		index += getVisibleString(1,VISIBLE_STRING_LEN,&data[index],class4500.commconfig.userName,DAR);
		if(*DAR!=success) return 0;
		index += getVisibleString(1,VISIBLE_STRING_LEN,&data[index],class4500.commconfig.passWord,DAR);
		if(*DAR!=success) return 0;
		index += getOctetstring(1,OCTET_STRING_LEN-1,&data[index],&class4500.commconfig.proxyIp[1],&class4500.commconfig.proxyIp[0],DAR);
		index += getLongUnsigned(&data[index],(INT8U *)&class4500.commconfig.proxyPort);
		index += getUnsigned(&data[index],(INT8U *)&class4500.commconfig.timeoutRtry,DAR);	//勘误更改
		index += getLongUnsigned(&data[index],(INT8U *)&class4500.commconfig.heartBeat);
		if(index>=sizeof(class4500.commconfig)) {
			*DAR = refuse_rw;
			return index;
		}
	    write_apn((char *) &class4500.commconfig.apn[1]);
		usleep(100*1000);
		write_userpwd(&class4500.commconfig.userName[1],&class4500.commconfig.passWord[1],&class4500.commconfig.apn[1]);
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
			index += getOctetstring(1,OCTET_STRING_LEN-1,&data[index],&class4500.master.master[i].ip[1],&class4500.master.master[i].ip[0],DAR);
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
		index += getVisibleString(1,VISIBLE_STRING_LEN,&data[index],(INT8U *)&class4500.sms.center,DAR);
		if(*DAR!=success)
			return 0;
		fprintf(stderr,"center index=%d %02x %02x\n",index,data[index],data[index+1]);
		index += getArray(&data[index],(INT8U *)&class4500.sms.masternum,DAR);
		if(class4500.sms.masternum>4) {
			fprintf(stderr,"!!!!!!!!!越限 sms.masternum=%d\n",class4500.sms.masternum);
			class4500.sms.masternum = 4;
		}
		for(i=0;i<class4500.sms.masternum;i++){
			index += getVisibleString(1,VISIBLE_STRING_LEN,&data[index],(INT8U *)&class4500.sms.master[i],DAR);
			if(*DAR!=success) return 0;
		}
		index += getArray(&data[index],(INT8U *)&class4500.sms.destnum,DAR);
		if(class4500.sms.destnum>4) {
			fprintf(stderr,"!!!!!!!!!越限 sms.destnum=%d\n",class4500.sms.destnum);
			class4500.sms.destnum = 4;
		}
		for(i=0;i<class4500.sms.destnum;i++){
			index += getVisibleString(1,VISIBLE_STRING_LEN,&data[index],(INT8U *)&class4500.sms.dest[i],DAR);
			if(*DAR!=success) return 0;
		}
		if(index>=sizeof(class4500.sms)) {
			*DAR = refuse_rw;
			return index;
		}
		break;

	case 10:
		index += getVisibleString(1,VISIBLE_STRING_LEN,&data[index],&class4500.simkard[0],DAR);
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
	syslog(LOG_NOTICE,"\n主站IP %d.%d.%d.%d :%d [oad=%d]\n",class4500.master.master[0].ip[1],class4500.master.master[0].ip[2],
			class4500.master.master[0].ip[3],class4500.master.master[0].ip[4],class4500.master.master[0].port,oad.attflg);

	Chg4500_reboot_HN(*DAR,class4500.master.master[0].ip,class4500.master.master[0].port,&memp->oi_changed.reset);
	return index;
}

INT16U set4018(OAD oad,INT8U *data,INT8U *DAR)
{
	int tmp = 0;
	int arraynum=0,index=0,i=0;
	CLASS_4018 class4018;
	memset(&class4018,0,sizeof(CLASS_4018 ));

	readCoverClass(0x4018,0,&class4018,sizeof(CLASS_4018 ),para_vari_save);
	switch(oad.attflg) {
		case 2:
			index += getArray(&data[index],(INT8U *)&arraynum,DAR);
			if (arraynum>32)
				arraynum = 32;
			fprintf(stderr,"\n\nset4018 当前套费率电价 属性2 下发个数 = %d",arraynum);
			class4018.num = arraynum;
			for(i=0; i<arraynum; i++) {
//				fprintf(stderr,"\ndata[%d] = %02x %02x %02x %02x %02x ",index,data[index],data[index+1],data[index+2],data[index+3],data[index+4]);
				index += getDouble(&data[index],(INT8U *)&class4018.feilv_price[i]);
				tmp = class4018.feilv_price[i];
				fprintf(stderr,"\n i=%d  - %d",i,tmp);
			}
			if(index>=sizeof(CLASS_4018 )) {
				*DAR = refuse_rw;
				return index;
			}
			break;
	}
	*DAR = saveCoverClass(0x4018,0,&class4018,sizeof(CLASS_4018),para_vari_save);

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
		index += getOctetstring(1,OCTET_STRING_LEN-1,&data[index],&class4510.commconfig.proxyIp[1],&class4510.commconfig.proxyIp[0],DAR);
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
			index += getOctetstring(1,OCTET_STRING_LEN-1,&data[index],&class4510.master.master[i].ip[1],&class4510.master.master[i].ip[0],DAR);
			index += getLongUnsigned(&data[index],(INT8U *)&class4510.master.master[i].port);
		}
		break;
	case 4:
		index += getStructure(&data[index],NULL,DAR);
		index += getEnum(1,&data[index],(INT8U *)&class4510.IP.ipConfigType);
		index += getOctetstring(1,OCTET_STRING_LEN-1,&data[index],&class4510.IP.ip[1],&class4510.IP.ip[0],DAR);
		index += getOctetstring(1,OCTET_STRING_LEN-1,&data[index],&class4510.IP.subnet_mask[1],&class4510.IP.subnet_mask[0],DAR);
		index += getOctetstring(1,OCTET_STRING_LEN-1,&data[index],&class4510.IP.gateway[1],&class4510.IP.gateway[0],DAR);
		index += getVisibleString(1,VISIBLE_STRING_LEN,&data[index],class4510.IP.username_pppoe,DAR);
		index += getVisibleString(1,VISIBLE_STRING_LEN,&data[index],class4510.IP.password_pppoe,DAR);
		writeIpSh(class4510.IP.ip,class4510.IP.subnet_mask);
		break;
	}
	*DAR = saveCoverClass(oad.OI,0,&class4510,sizeof(CLASS26),para_vari_save);

	return index;
}

/////////////////////////////////////////////////////////////////////////////
INT16U set6002(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U index=0;
	CLASS_6002		class6002={};
	int		i=0;
	memset(&class6002,0,sizeof(CLASS_6002));
	readCoverClass(0x6002,0,&class6002,sizeof(CLASS_6002),para_vari_save);
	switch(oad.attflg) {
	case 2:	//搜表结果

		break;
	case 5://跨台区结果
		break;
	case 6:	//搜表结果记录数
		break;
	case 7:	//跨台区搜表结果记录数
		break;
	case 8:	//搜表
		index += getStructure(&data[index],NULL,DAR);
		index += getBool(&data[index],(INT8U *)&class6002.attr8.enablePeriodFlg,DAR);
		index += getBool(&data[index],(INT8U *)&class6002.attr8.autoUpdateFlg,DAR);
		index += getBool(&data[index],(INT8U *)&class6002.attr8.eventFlg,DAR);
		index += getEnum(1,&data[index],(INT8U *)&class6002.attr8.clearChoice);
		*DAR = saveCoverClass(0x6002,0,&class6002,sizeof(CLASS_6002),para_vari_save);
		break;
	case 9:	//每天周期搜表参数配置
		index += getArray(&data[index],(INT8U *)&class6002.attr9_num,DAR);
		if(*DAR == type_mismatch) {
			fprintf(stderr,"无Array类型\n");
			class6002.attr9_num = 1;
		}
		if(class6002.attr9_num>SERACH_PARA_NUM) {
			class6002.attr9_num = SERACH_PARA_NUM;
			syslog(LOG_ERR,"搜表配置数量%d大于限值%d\n",class6002.attr9_num,SERACH_PARA_NUM);
		}
		for(i=0;i<class6002.attr9_num;i++) {
			index += getStructure(&data[index],NULL,DAR);
			index += getTime(1,&data[index],(INT8U *)&class6002.attr9[i].startTime,DAR);
			if(*DAR!=success)	return 0;	//无效时间，返回
			fprintf(stderr," 开始时间  %d:%d:%d\n",class6002.attr9[i].startTime[0],class6002.attr9[i].startTime[1],class6002.attr9[i].startTime[2]);
			index += getLongUnsigned(&data[index],(INT8U *)&class6002.attr9[i].searchLen);
		}
		*DAR = saveCoverClass(0x6002,0,&class6002,sizeof(CLASS_6002),para_vari_save);
		break;
	case 10://搜表状态

		break;
	}
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

int	setf200(OI_698 oi,INT8U *data,INT8U *DAR)
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

int	setf201(OI_698 oi,INT8U *data,INT8U *DAR)
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

int	setf202(OI_698 oi,INT8U *data,INT8U *DAR)
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


int setf205(OAD oad,INT8U *data,INT8U *DAR)
{
	int	RelayNum = 0,i=0;
	INT16U index=0;
	CLASS_F205	f205={};
	memset(&f205,0,sizeof(CLASS_F205));
	readCoverClass(oad.OI,0,&f205,sizeof(CLASS_F205),para_vari_save);
	if ( oad.attflg == 2 )//继电器单元
	{
		index += getArray(&data[index],(INT8U *)&RelayNum,DAR);
		RelayNum = rangeJudge("继电器单元",RelayNum,0,4);
		if(RelayNum == -1) {
			*DAR = boundry_over;
		}else {
			f205.relaynum = RelayNum;
			for(i=0;i<RelayNum;i++) {
				index += getStructure(&data[index],NULL,DAR);
				index += getVisibleString(1,VISIBLE_STRING_LEN,&data[index],(INT8U *)&f205.unit[i].devdesc,DAR);
				index += getEnum(1,&data[index],(INT8U *)&f205.unit[i].currentState);
				index += getEnum(1,&data[index],(INT8U *)&f205.unit[i].switchAttr);
				index += getEnum(1,&data[index],(INT8U *)&f205.unit[i].wiredState);
				fprintf(stderr,"\n继电器状态 %d %d %d \n",f205.unit[i].currentState,f205.unit[i].switchAttr,f205.unit[i].wiredState);
			}
			if(*DAR == success){
				*DAR = saveCoverClass(oad.OI,0,&f205,sizeof(CLASS_F205),para_vari_save);
			}
		}
	}
	return index;
}

/*
 * 修改开关属性（继电器号、开关属性）
 * */
int f205_act127(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U index = 0;
	CLASS_F205	f205={};
	INT8U  attrindex = 0;
	int		ret = 0;
	OAD		relayoad = {};

	memset(&f205,0,sizeof(CLASS_F205));
	readCoverClass(oad.OI,0,&f205,sizeof(CLASS_F205),para_vari_save);
	if(oad.attflg == 127) {
		index += getStructure(&data[index],NULL,DAR);
		index += getOAD(1,&data[index],&relayoad,DAR);
		attrindex = relayoad.attrindex;
		ret = rangeJudge("继电器号",attrindex,1,4);
		if(ret == -1) {
			*DAR = boundry_over;
		}else {
			attrindex = attrindex-1;
			memcpy(&f205.unit[attrindex].oad,&relayoad,sizeof(OAD));
			index += getEnum(1,&data[index],(INT8U *)&f205.unit[attrindex].switchAttr);
		}
	}
	if(*DAR == success) {
		*DAR = saveCoverClass(oad.OI,0,&f205,sizeof(CLASS_F205),para_vari_save);
	}
	return index;
}

/*
 * 修改多功能端子工作模式
 * */
int f207_act127(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U index = 0;
	CLASS_f207	f207={};
	INT8U  attrindex = 0;
	int		ret = 0;
	OAD		relayoad = {};

	memset(&f207,0,sizeof(CLASS_f207));
	readCoverClass(oad.OI,0,&f207,sizeof(CLASS_f207),para_vari_save);
	if(oad.attflg == 127) {
		index += getStructure(&data[index],NULL,DAR);
		index += getOAD(1,&data[index],&relayoad,DAR);
		attrindex = relayoad.attrindex;
		ret = rangeJudge("多功能端子",attrindex,1,4);
		if(ret == -1) {
			*DAR = boundry_over;
		}else {
			attrindex = attrindex-1;
			memcpy(&f207.oad[attrindex],&relayoad,sizeof(OAD));
			index += getEnum(1,&data[index],(INT8U *)&f207.func[attrindex]);
		}
	}
	if(*DAR == success) {
		*DAR = saveCoverClass(oad.OI,0,&f207,sizeof(CLASS_f207),para_vari_save);
	}
	return index;
}

int setf206(OAD oad,INT8U *data,INT8U *DAR)
{
	INT16U index=0;
	int	i=0;
	CLASS_f206	f206={};
	memset(&f206,0,sizeof(CLASS_f206));
	readCoverClass(oad.OI,0,&f206,sizeof(CLASS_f206),para_vari_save);
	if ( oad.attflg == 2 )//配置参数
	{
		index += getStructure(&data[index],NULL,DAR);
		index += getArray(&data[index],&f206.state_num,DAR);
		f206.state_num = limitJudge("告警输出",10,f206.state_num);
//		fprintf(stderr,"state_num = %d\n",f206.state_num);
//		fprintf(stderr,"f206.state_num = %d\n",f206.state_num);
		for(i=0;i<f206.state_num;i++) {
			index += getEnum(1,&data[index],&f206.alarm_state[i]);
		}
		if(*DAR==success) {
			*DAR = saveCoverClass(oad.OI,0,&f206,sizeof(CLASS_f206),para_vari_save);
		}
	}
	if ( oad.attflg == 4 )//
	{
		index += getArray(&data[index],&f206.time_num,DAR);
		f206.time_num = limitJudge("告警输出",10,f206.time_num);
		fprintf(stderr,"time_num = %d\n",f206.time_num);
		for(i=0;i<f206.time_num;i++) {
			index += getStructure(&data[index],NULL,DAR);
			index += getTime(1,&data[index],(INT8U *)&f206.timev[i].start,DAR);
			index += getTime(1,&data[index],(INT8U *)&f206.timev[i].end,DAR);
		}
		if(*DAR==success) {
			*DAR = saveCoverClass(oad.OI,0,&f206,sizeof(CLASS_f206),para_vari_save);
		}
	}
	return index;
}
int	setf209(OAD setoad,INT8U *data,INT8U *DAR)
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

