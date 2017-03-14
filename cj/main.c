/*
 * main.c
 *
 *  Created on: Jan 5, 2017
 *
 *      Author: ava
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include <mqueue.h>
#include <semaphore.h>
#include <termios.h>
#include "Shmem.h"
#include "StdDataType.h"
#include "main.h"
#include "AccessFun.h"

#include "dlt698.h"
#include "filebase.h"
ProgramInfo* JProgramInfo = NULL;

static char* usage_set = "\n--------------------参数设置----------------------------\n"
                         "		 【主站IP端口设置】cj ip XXX.XXX.XXX.XXX:port XXX.XXX.XXX.XXX:port 	\n"
                         "		 【主站apn设置】cj apn cmnet		\n"
                         "		 【cdma电信用户名密码设置】cj usr-pwd 　user  password		\n"
                         "		 【通信地址】cj id <addr>	如：地址为123456  :cj id 12 34 56	\n"
                         "-------------------------------------------------------\n\n";

static char* usage_para = "\n--------------------参变量类对象----------------------------\n"
                          "[电气设备] "
                          "		 【参数读取】cj para pro 4300 		\n"
                          "		 【数据初始化】cj para method 4300 3		\n"
                          "-------------------------------------------------------\n\n";

static char* usage_inoutdev = "\n-------------------A.12　输入输出设备类对象----------------------------\n"
                              "\n---------文件传输类对象 	ESAM接口类对象 	输入输出设备类对象 	显示类对象--------\n"
                              "【开关量输入】cj dev pro f203 				\n"
                              "[初始化通信参数]  cj dev init <oi> :	例如：初始化通信参数  cj dev init 4500 	\n"
                              "【安全模式参数读取】cj dev pro f101 			\n"
                              "【安全模式参数设置,0:不启用，1:启用】cj dev set f101 <0/1>		\n"
                              "-------------------------------------------------------\n\n";

static char* usage_coll =
"\n--------------------采集监控类对象----------------------------\n"
"[清除配置]cj coll clear <oi>	\n"
"[删除一个配置单元]cj coll delete <oi> <id>  	id=【1..255】	\n"
"[采集档案配置表读取]cj coll pro 6000	\n"
"[增加一个采集档案配置表]查看帮助：cj coll add 6000	\n"
"[任务配置单元] cj coll pro 6013 <任务号> [读取任务配置单元]\n"
"			  cj coll pro 6013 任务ID 执行频率 方案类型 方案编号 开始时间 结束时间 延时 执行优先级 状态 运行时段 起始小时:起始分钟 结束小时:结束分钟\n"
"             cj coll pro 6013 1 1-5 1 1 2016-11-11 0:0:0 2099-9-9 9:9:9 1-2 2 1 0 0:0-23:59\n"
"[普通采集方案] cj coll pro 6015 <采集方案号>\n"
"[事件采集方案] cj coll pro 6017 <方案编号>\n"
"[上报方案] 	  cj coll pro 601d <方案编号>\n"
"[采集任务监控] cj coll pro 6035 <采集方案号>\n"
"-------------------------------------------------------\n\n";
static char* usage_event = "--------------------事件类对象----------------------------\n"
                           "[初始化事件参数]  cj event init <oi> :例如：初始化采集终端初始化事件  cj event init 0x3100/0全部 	\n"
                           "[复位事件]  cj event reset <oi> :例如：复位采集终端初始化事件  cj event reset 0x3100 	\n"
                           "[读取事件属性] cj event pro <oi> :例如：读取采集终端初始化事件属性 cj event pro 0x3100 	\n"
                           "[设置Class7]  cj event pro <oi> 当前记录数 最大记录数 上报标识 有效标识 关联对象个数 关联对象OAD[1-10]	\n"
                           "	[设置采集终端初始化事件] cj event pro 3100 1 16 1 1 0 \n"
                           "	[设置终端状态量变位事件] cj event pro 3104 1 16 1 1 5 201E-4200 F203-4201 F203-4202 F203-4203 F203-4204 F203-4205\n"
                           "[读取事件记录] cj event record <oi> 0（所有）/n（记录n）:例如：读取采集终端初始化事件记录 cj event record 0x3100 0（所有）/1(记录1)"
                           "[设置电能表开盖事件有效/无效] cj event enable 301B 1/0 1有效 0无效 \n"
                           "[设置停上电事件有效/无效] cj event enable 3106 1/0 1有效 0无效 \n"
                           "[设置对时事件有效/无效] cj event enable 3114 1/0 1有效 0无效 \n"
                           "-------------------------------------------------------\n\n";

static char* usage_acs = "--------------------终端交采计量校表及维护命令----------------------------\n"
                         "acs acreg   <Pa Pb Pc Qa Qb Qc Ua Ub Uc Ia Ib Ic >   [同时校正三相系数，输入值为单相标准值]\n\n"
                         "		 [三相四交采校表:准备工作：标准源输入220V,3A,角度=60“C(0.5L 感性)]\n"
                         "         例如输入：cj acs acreg 330.00 330.00 330.00 572.00 572.00 572.00 220.0 220.0 220.0 3 3 3\n"
                         "         [参数输入标准源显示值，可输入浮点数。]\n"
                         "		<Pa 0 Pc Qa 0 Qc Uab 0 Uca Ia 0 Ic >\n"
                         "		 [三相三交采校表:准备工作：标准源输入100V,3A,角度=1“C(1L 感性)]\n"
                         "         例如输入：cj acs acreg 259.8076 0 259.8076 150 0 -150 100 0 100 3 0 -3\n"
                         "acs acphase   <Pa Pb Pc Qa Qb Qc Ua Ub Uc Ia Ib Ic >    [小电流相位校正：同时校正三相系数，输入值为单相标准值]\n\n"
                         "		 [ATT7022E 交采校表:准备工作：标准源输入220V,0.3A,角度=60“C(0.5L 感性)]\n"
                         "         例如输入：cj acs acphase 165.00 165.00 165.00 285.79 285.79 285.79 220.0 220.0 220.0 220.0 1.5 1.5 1.5\n"
                         "         [参数输入标准源显示值，可输入浮点数。]\n"
                         "		 [ATT7022E-D 交采校表:准备工作：标准源输入220V,1.5A,角度=60“C(0.5L 感性)]\n"
                         "         例如输入：cj acs acphase 165 165 165 286 286 286 220 220 220 1.5 1.5 1.5\n"
                         "         [参数输入标准源显示值，可输入浮点数。]\n"
                         "		[三相三交采校表:准备工作：标准源输入100V,1.5A,角度=1“C(1L 感性)]\n"
                         "         例如输入：cj acs acphase 129.9 0 129.9 75 0 -75 100 0 100 1.5 0 1.5\n"
                         "acs acphase0  <Pa Pb Pc Qa Qb Qc Ua Ub Uc Ia Ib Ic >  [(7022E-d型芯片支持)电流相位校正：同时校正三相系数，输入值为单相标准值]\n\n"
                         "		 [交采校表:准备工作：标准源输入220V,0.15A,角度=60“C(0.5L 感性)]\n"
                         "         例如输入：cj acs acphase0 16.5 16.5 16.5 28.6 28.6 28.6 220 220 220 0.15 0.15 0.15\n"
                         "         [参数输入标准源显示值，可输入浮点数。]\n"
                         "		[三相三交采校表:准备工作：标准源输入100V,0.3A,角度=1“C(1L 感性)]\n"
                         "         例如输入：cj acs acphase0 25.9 0 25.9 15 0 -15 100 0 100 0.3 0 0.3\n"
                         "acs acregclean [清除（交采）校表系数]\n"
                         "acs acregdata	[读校表系数]\n"
                         "acs acdata  [打印测量点1（交采）实时数据]\n"
                         "acs ace  	 [测量点1（交采）电能示值数据]\n"
                         "acs acrndata	[读RN8209校表系数]\n"
                         "acs checku <U> [rn8209交采电压校正：输入标准源显示值]\n"
                         "        例如输入：cj acs checku 220.00 \n"
                         "        [参数输入标准源显示值，可输入浮点数。]\n"
                         "-------------------------------------------------------\n\n";

void prthelp() {
    fprintf(stderr, "Usage: ./cj (维护功能)  ");
    fprintf(stderr, "help	 [help] ");
    fprintf(stderr, "%s", usage_acs);
    fprintf(stderr, "%s", usage_set);
    fprintf(stderr, "%s", usage_para);
    fprintf(stderr, "%s", usage_event);
    fprintf(stderr, "%s", usage_coll);
    fprintf(stderr, "%s", usage_inoutdev);
}
void dog_feed() {
    INT32S fd = -1;
    INT32S tm = 888888;
    system("pkill cjmain");
    sleep(1);
    if ((fd = open(DEV_WATCHDOG, O_RDWR | O_NDELAY)) == -1) {
        fprintf(stderr, "\n\r open /dev/watchdog error!!!");
        return;
    }
    write(fd, &tm, sizeof(int));
    close(fd);
    system("pkill cjcomm");
    system("pkill cjdeal");
    system("pkill gsmMuxd");
}

INT8U Getcurrno(INT16U* currno, INT16U maxno) {
    fprintf(stderr, "[event]currno=%d maxno=%d \n", *currno, maxno);
    maxno = 0 ? 15 : maxno;
    if (*currno > maxno)
        *currno = 1;
    return 1;
}

INT16U FixHeadUnit(INT8U *headbuf,INT8U *fixlen) {
	static INT8U head_oad[4][4]={{0x20,0x2a,0x02,0x00},{0x60,0x40,0x02,0x00},{0x60,0x41,0x02,0x00},{0x60,0x42,0x02,0x00}};
	static INT8U head_oad_len[4]={0x0012,0x0008,0x0008,0x0008};
	int	  i=0,index=0;
	HEAD_UNIT	unit[4]={};
	*fixlen = 0;
	for(i=0;i<4;i++) {
		memset(&unit[i].oad_m,0,sizeof(OAD));
		unit[i].oad_r.OI = head_oad[i][0];
		unit[i].oad_r.OI = (unit[i].oad_r.OI<<8) | head_oad[i][1];
		unit[i].oad_r.attflg = head_oad[i][2];
		unit[i].len = head_oad_len[i];
		*fixlen += head_oad_len[i];
	}
	if(headbuf!=NULL) {
		memcpy(headbuf,unit,sizeof(unit));
		index += sizeof(unit);
	}
	return index;
}

INT16U CalcHeadRcsdUnitNum(CSD_ARRAYTYPE csds) {
	INT16U headunit_num=4;		//FixHeadUnit 固定TSA+3个时标的HEAD_UNIT长度
	int i=0;
	if(csds.num>MY_CSD_NUM) {
		fprintf(stderr,"rcsd 个数超过限值 %d!!!!!!!!!!\n",MY_CSD_NUM);
		csds.num = MY_CSD_NUM;
	}
	for(i=0;i<csds.num;i++)
	{
		if(csds.csd[i].type != 0 && csds.csd[i].type != 1)
			continue;
		if(csds.csd[i].type == 1)
			headunit_num += csds.csd[i].csd.road.num+1;//加上本身
		else
			headunit_num++;
	}
	if(headunit_num == 4)
		return 0;
	else
		return headunit_num;
}
/*
 * 计算某个OI的数据长度，指针对抄表数据 todo 先写个简单的，以后完善 而且没有考虑费率
 * attr_flg:0 全部属性 非0 一个属性  例如20000200 则为全部属性 20000201则为一个属性
 * OI_TYPE.cfg格式定义：
 * 对象标识OI-数据类型描述-数据长度-接口类IC-单位换算
 * 2000-12-002-03-11 解析：
 * 2000：OI 电压
 * 12：long-unsigned
 * 002:1个数据长度
 * 03：接口类IC 变量类参数
 * 11：换算：-1   如果12：表示换算：-2,  02：表示换算：+2
 */
INT16U CalcOILen(OAD oad, INT8U rate) {
	FILE *fp;
	char ln[60];
	char lnf[4];
	INT16U oi_len=0,oi_tmp = 0;
	INT8U ic_type = 1;

	//TODO:  MET_RATE 替换成6000档案的电表费率个数rate
	if(oad.OI>=0x0000 && oad.OI<0x2000)		//接口IC的1,2类，每个数据长度固定为4个字节
	{
		if(oad.attrindex == 0){	//全部属性
			oi_len += 2;			//数组+元素个数
			oi_len += 5*(MET_RATE+1);	//5:数据类型描述+数据,(MET_RATE+1):总及4费率
			return oi_len;
		}
		else
			return (4+1);	//4:数据长度+1个字节数据类型
	}
	fp = fopen("/nor/config/OI_TYPE.cfg","r");
	if(fp == NULL)
	{
		fprintf(stderr,"\nOI_TYPE.cfg do not exist,hard error!!\n");
		return 0;
	}
	while(1)
	{
		memset(ln,0x00,60);
		fscanf(fp,"%s",ln);
		if(strncmp(ln,"begin",5) == 0) continue;
		if(strncmp(ln,"end",3) == 0) break;
		if(strncmp(ln,"//",2) == 0) continue;

		memset(lnf,0x00,4);
		memcpy(lnf,&ln[0],4);


		oi_tmp = strtol(lnf,NULL,16);
//		if(strtol(lnf,NULL,16) != oad.OI)
		fprintf(stderr,"\n------oi_tmp=%04x--%s\n",oi_tmp,ln);
		if(oi_tmp != oad.OI)
			continue;
		memset(lnf,0x00,4);
		memcpy(lnf,&ln[8],3);
		oi_len = strtol(lnf,NULL,10)+1;		//返回长度+1个字节数据类型描述
		memset(lnf,0x00,4);
		memcpy(lnf,&ln[12],2);
		ic_type = strtol(lnf,NULL,10);
		fprintf(stderr,"oi=%04x ,oi_len=%d,ic_type=%d",oad.OI,oi_len,ic_type);
		break;
	}
	fclose(fp);
	if(oi_len != 0 && ic_type != 0)
	{
		switch(ic_type)
		{
		case 3:	//分相变量接口类
			if(oad.attrindex == 0)
				oi_len = oi_len*3+1+1;//三相			+1：数组， +1：元素个数
			break;
		case 4://功率接口类
			if(oad.attrindex == 0)
				oi_len = oi_len*4+1+1;//总及分项
			break;
		default:
			break;
		}
	}
	fprintf(stderr,"return oi_len=%d\n",oi_len);
	return oi_len;
}

INT8U getOneUnit(INT8U* headbuf, OAD oad_m, OAD oad_r, INT16U len) {
    HEAD_UNIT headunit = {};

    memcpy(&headunit.oad_m, &oad_m, sizeof(OAD));
    memcpy(&headunit.oad_r, &oad_r, sizeof(OAD));
    headunit.len = len;
    memcpy(headbuf, &headunit, sizeof(HEAD_UNIT));
    return (sizeof(HEAD_UNIT));
}

INT16S GetTaskHead(FILE* fp, INT16U* head_len, INT16U* tsa_len, HEAD_UNIT** head_unit) {
    INT8U headl[2], blockl[2];
    INT16U unitnum = 0, i = 0;

    fread(headl, 2, 1, fp);
    *head_len = headl[0];
    *head_len = (headl[0] << 8) + headl[1];
    fprintf(stderr, "\n----headlen=%d\n", *head_len);
    if (*head_len <= 4)
        return -1;
    fread(&blockl, 2, 1, fp);
    *tsa_len = blockl[0];
    *tsa_len = (blockl[0] << 8) + blockl[1];
    fprintf(stderr, "\n----blocklen=%d\n", *tsa_len);
    unitnum = (*head_len - 4) / sizeof(HEAD_UNIT);
    fprintf(stderr, "\n----blocklen=%d unitnum=%d\n", *tsa_len, unitnum);
    if (*head_unit == NULL)
        *head_unit = malloc(*head_len);
    fprintf(stderr, "get  %p", *head_unit);
    fread(*head_unit, *head_len - 4, 1, fp);
    fprintf(stderr, "\nhead_unit:len(%d):\n", unitnum);
    return unitnum;
}
/*
 * 结构为四个字节长度+TSA(0x00+40010200+2个字节长度)+3*时标+CSD
 * unitlen_z长度为此任务当日需要抄的全部数据长度，以此将一个测量点一天的数据放在一个地方
 */
void CreateSaveHead(char* fname, CSD_ARRAYTYPE csds, INT16U* headlen, INT16U* unitlen, INT16U* unitnum, INT16U freq, INT8U wrflg) {
    INT16U pindex = 0, len_tmp = 0, csd_unitnum = 0;
    int i = 0, j = 0;
    INT8U* headbuf = NULL;
    OAD oad_m      = {};

    if (csds.num == 0xee || csds.num == 0)
        return;
    csd_unitnum = CalcHeadRcsdUnitNum(csds);
    fprintf(stderr, "\n---csd_unitnum = %d\n", csd_unitnum);
    if (headbuf == NULL) {
        *headlen = csd_unitnum * sizeof(HEAD_UNIT) + 4; // 4:文件头长度+TSA块长度
        headbuf  = (INT8U*)malloc(*headlen);
    }
    headbuf[pindex++] = (*headlen & 0xff00) >> 8; //文件头长度
    headbuf[pindex++] = (*headlen & 0x00ff);
    headbuf[pindex++] = 0x00;
    headbuf[pindex++] = 0x00; //长度
    int framlen=0;
    pindex += FixHeadUnit(&headbuf[pindex],&framlen);
    if (csds.num > MY_CSD_NUM) //超了
        csds.num = MY_CSD_NUM;
    for (i = 0; i < csds.num; i++) {
        if (csds.csd[i].type == 0xee)
            break;
        if (csds.csd[i].type != 0 && csds.csd[i].type != 1)
            continue;
        if (csds.csd[i].type == 0) // OAD
        {
            fprintf(stderr, "\n-0--csds.csd[i].csd.oad.OI = %04x\n", csds.csd[i].csd.oad.OI);
            len_tmp = CalcOILen(csds.csd[i].csd.oad, 4); //多一个数据类型
            fprintf(stderr, "\nlen_tmp=%d\n", len_tmp);
            memset(&oad_m, 0, sizeof(OAD));
            pindex += getOneUnit(&headbuf[pindex], oad_m, csds.csd[i].csd.oad, len_tmp);
            (*unitnum)++;
            fprintf(stderr, "\n-1-unitlen=%d\n", *unitlen);
        } else if (csds.csd[i].type == 1) // ROAD
        {
            if (csds.csd[i].csd.road.num == 0xee)
                continue;
            if (csds.csd[i].csd.road.num > ROAD_OADS_NUM) //超了
                csds.csd[i].csd.road.num = ROAD_OADS_NUM;
            for (j = 0; j < csds.csd[i].csd.road.num; j++) {
                fprintf(stderr, "\n-2--csds.csd[i].csd.oad.OI = %04x\n", csds.csd[i].csd.road.oads[j].OI);
                if (csds.csd[i].csd.road.oads[j].OI == 0xeeee)
                    break;
                len_tmp = CalcOILen(csds.csd[i].csd.road.oads[j], 4); //多一个数据类型
                fprintf(stderr, "\n--2-len_tmp=%d\n", len_tmp);
                pindex += getOneUnit(&headbuf[pindex], csds.csd[i].csd.road.oad, csds.csd[i].csd.road.oads[j], len_tmp);
                (*unitnum)++;
            }
        }
    }
    *unitlen = pindex - 4; //每个单元长度 = 文件头长度-4
    fprintf(stderr, "\n-2-unitlen=%d\n", *unitlen);
    *unitlen = freq * (42 + *unitlen); //一个单元存储TSA共用,在结构最前面，每个单元都有3个时标和数据，预留出合适大小，以能存下一个TSA所有数据
    fprintf(stderr, "\n-3-unitlen=%d\n", *unitlen);
    headbuf[2] = (*unitlen & 0xff00) >> 8; //数据单元长度
    headbuf[3] = *unitlen & 0x00ff;
    fprintf(stderr, "\nhead(%d)::", pindex);
    for (i = 0; i < pindex; i++) {
        fprintf(stderr, " %02x", headbuf[i]);
    }
    fprintf(stderr, "\n");
    if (wrflg == 1) {
        datafile_write(fname, headbuf, *headlen, 0);
    }
    if (headbuf) {
        free(headbuf);
    }
}

int main(int argc, char* argv[]) {
    usleep(10);
    if (argc < 2) {
        prthelp();
        return EXIT_SUCCESS;
    }
    if (strcmp("head", argv[1]) == 0) {
        CLASS_6015 class6015;
        int headlen = 0, unitlen = 0, unitnum = 0;

        readCoverClass(0x6015, 2, &class6015, sizeof(CLASS_6015), coll_para_save);
        CreateSaveHead("/nand/task/002/20170310.dat", class6015.csds, &headlen, &unitlen, &unitnum, 0, 1); //写文件头信息并返回
        return EXIT_SUCCESS;
    }
    if (strcmp("ms", argv[1]) == 0) {
    	int taskid=64;
		int ret = 0;
		if(argc>=3) {
			taskid = atoi(argv[2]);
		}
		fprintf(stderr,"taskid=%d\n",taskid);
		CLASS_601D class601d = {};
		if (readCoverClass(0x601D, taskid, &class601d, sizeof(CLASS_601D), coll_para_save) == 1) {
			ret = GetReportData(class601d);
		}
		return EXIT_SUCCESS;
    }

    if (strcmp("ip", argv[1]) == 0) {
        SetIPort(argc, argv);
        return EXIT_SUCCESS;
    }

    if (strcmp("net-ip", argv[1]) == 0) {
        SetNetIPort(argc, argv);
        return EXIT_SUCCESS;
    }

    if (strcmp("apn", argv[1]) == 0) {
        SetApn(argc, argv);
        return EXIT_SUCCESS;
    }
    if (strcmp("id", argv[1]) == 0) {
        SetID(argc, argv);
        return EXIT_SUCCESS;
    }

    if (strcmp("dog", argv[1]) == 0 || strcmp("stop", argv[1]) == 0) {
        dog_feed();
        return EXIT_SUCCESS;
    }
    if (strcmp("help", argv[1]) == 0) {
        prthelp();
        return EXIT_SUCCESS;
    }
    if (strcmp("event", argv[1]) == 0) {
        fprintf(stderr, "%s", usage_event);
        event_process(argc, argv);
        return EXIT_SUCCESS;
    }
    if (strcmp("para", argv[1]) == 0) {
        fprintf(stderr, "%s", usage_para);
        para_process(argc, argv);
        return EXIT_SUCCESS;
    }
    if (strcmp("coll", argv[1]) == 0) {
        fprintf(stderr, "%s", usage_coll);
        coll_process(argc, argv);
        return EXIT_SUCCESS;
    }
    if (strcmp("dev", argv[1]) == 0) {
        fprintf(stderr, "%s", usage_inoutdev);
        inoutdev_process(argc, argv);
        return EXIT_SUCCESS;
    }

    if (strcmp("usr-pwd", argv[1]) == 0) {
        SetUsrPwd(argc, argv);
        return EXIT_SUCCESS;
    }

    if (strcmp("acs", argv[1]) == 0) {
        fprintf(stderr, "%s", usage_acs);
        acs_process(argc, argv);
        return EXIT_SUCCESS;
    }
    if (strcmp("test", argv[1]) == 0) {
        fprintf(stderr, "\n自组报文\n");
        cjframe(argc, argv);
        return EXIT_SUCCESS;
    }
    if (strcmp("cjread", argv[1]) == 0) {
        fprintf(stderr, "\n查看任务抄表数据\n");
        cjread(argc, argv);
        return EXIT_SUCCESS;
    }
    if (strcmp("rs485", argv[1]) == 0) {
        fprintf(stderr, "\n设置维护485端口参数\n");
        SetF201(argc, argv);
        return EXIT_SUCCESS;
    }
    if (strcmp("ifr", argv[1]) == 0) {
        fprintf(stderr, "\n设置维护红外端口参数\n");
        SetF202(argc, argv);
        return EXIT_SUCCESS;
    }
    if (strcmp("taskdata", argv[1]) == 0) {
    	fprintf(stderr, "\n分析任务数据文件内容\n");
    	analyTaskData(argv[2]);
    	return EXIT_SUCCESS;
    }
    prthelp();
    return EXIT_SUCCESS;
}
