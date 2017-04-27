/*
 * ObjectAction.c
 *
 *  Created on: Nov 12, 2016
 *      Author: ava
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "ParaDef.h"
#include "AccessFun.h"
#include "StdDataType.h"
#include "dlt698def.h"
#include "dlt698.h"
#include "Objectdef.h"
#include "event.h"
#include "secure.h"
#include "basedef.h"
#include "class8.h"
#include "class23.h"

extern INT8U Reset_add();

extern void FrameTail(INT8U *buf, int index, int hcsi);

extern int FrameHead(CSINFO *csinfo, INT8U *buf);

extern INT8S (*pSendfun)(int fd, INT8U *sndbuf, INT16U sndlen);

extern void Get698_event(OAD oad, ProgramInfo *prginfo_event);

extern int comfd;
extern ProgramInfo *memp;
extern PIID piid_g;
extern INT8U securetype;
extern INT8U broadcast;

INT16U getMytypeSize(INT8U first) {
    if (first == 0xAA) {
        return (sizeof(DATA_TYPE));
    }
    if (first == 0x55) {
        return (sizeof(CSD_ARRAYTYPE));
    }
    if (first == 0xCC) {
        return (12);
    }
    if (first == 0x22) {
        return (sizeof(MASTER_STATION_INFO_LIST));
    }
    if (first == 0x88) {
        return (sizeof(TSA_ARRAYTYPE));
    }
    if (first == 0x77)
        return (sizeof(ARRAY_ROAD));
    return 0;
}

int doReponse(int server, int reponse, CSINFO *csinfo, int datalen, INT8U *data, INT8U *buf) {
    int index = 0, hcsi = 0;
    int apduplace = 0;

    fprintf(stderr, "\nbroadcast = %02x\n", broadcast);

    if (broadcast == 0x03)
        return 0;
    csinfo->dir = 1;
    csinfo->prm = 0;

    index = FrameHead(csinfo, buf);
    hcsi = index;
    index = index + 2;
    apduplace = index;

    buf[index] = server;
    index++;
    buf[index] = reponse;
    index++;
    buf[index] = piid_g.data;
    index++;

    memcpy(&buf[index], data, datalen);
    index = index + datalen;
    //buf[index++] = 0;	//操作返回数据
    buf[index++] = 0;    //跟随上报信息域 	FollowReport
    buf[index++] = 0;    //时间标签		TimeTag

    if (securetype != 0)//安全等级类型不为0，代表是通过安全传输下发报文，上行报文需要以不低于请求的安全级别回复
    {
        apduplace += composeSecurityResponse(&buf[apduplace], index - apduplace);
        index = apduplace;
    }
    FrameTail(buf, index, hcsi);

    if (pSendfun != NULL)
        pSendfun(comfd, buf, index + 3);
    return (index + 3);
}

void get_BasicUnit(INT8U *source, INT16U *sourceindex, INT8U *dest, INT16U *destindex) {
    INT8U choicetype;
    INT8U size = 0;
    INT8U i = 0;
    INT8U strnum = 0;
    INT16U source_sumindex = 0, dest_sumindex = 0, csdsize = 0;

    INT8U type = source[0];

    fprintf(stderr, "\ntype = %02x  sourceindex=%d ", type, *sourceindex);
    A_FPRINTF("dest[0] :%02X\n", dest[0]);
    dest_sumindex = getMytypeSize(dest[0]);
    A_FPRINTF("dest[0] :%02X\n", dest[0]);
    if (dest_sumindex > 0) {
        dest[0] = type;
        dest = dest + 1;
        fprintf(stderr, "\n遇到变长结构体 目标地址跳转 %d 字节", dest_sumindex);
    }
    switch (type) {
        case 0x00:
            dest[0] = 0;//Data类型 0x00为NULL
            break;
        case 0x01:    //array
            strnum = source[1];
            dest[0] = strnum;        //数组类型第一个字节为长度
            dest = dest + 1;
            fprintf(stderr, "\n数组个数-%d", strnum);
            size = 1;
            if (dest_sumindex > 0) {
                csdsize = dest_sumindex;//csdsize 保存特殊类型数组尺寸
                dest_sumindex = 0;
            }
            break;
        case 0x02: //struct
            strnum = source[1];
            fprintf(stderr, "\n		结构体 %d  元素", strnum);
            size = 1;
            break;
        case 0x03: //bool
            dest[0] = source[1];
            fprintf(stderr, "\n		bool %d  元素", source[1]);
            size = 1;
            if (dest_sumindex == 0)
                dest_sumindex = 1;
            break;
        case 0x04: //bit-string
            size = 2;
            dest[0] = source[2];  // TODO: 此处默认8个bit   source[1] : 长度字节
            fprintf(stderr, "\n		bit-string %d ", source[2]);
            if (dest_sumindex == 0)
                dest_sumindex = 1;
            break;
        case 0x06: //double-long-unsigned
            size = 4;
            dest[0] = source[4];
            dest[1] = source[3];
            dest[2] = source[2];
            dest[3] = source[1];
            if (dest_sumindex == 0)
                dest_sumindex = size;
            break;
        case 0x09://octet-string
            size = source[1];
            memcpy(dest, &source[1], size + 1);// memcpy(dest,&source[2],size); 第一个字节放置实际数据长度
            if (dest_sumindex == 0)
                dest_sumindex = OCTET_STRING_LEN;
            size = size + 1;
            break;
        case 0x0a:    //visible-string
            size = source[1];// 长度
            memcpy(dest, &source[1], size + 1);
            if (dest_sumindex == 0)
                dest_sumindex = VISIBLE_STRING_LEN;
            size += 1;//加1 ：    1长度
            break;
        case 0x11://unsigned
            size = 1;
            memcpy(dest, &source[1], size);
            if (dest_sumindex == 0)
                dest_sumindex = size;
            fprintf(stderr, "\n		unsigned %02x", source[1]);
            break;
        case 0x12://long unsigned
            size = 2;
            dest[0] = source[2];
            dest[1] = source[1];
            fprintf(stderr, "\n		long %02x %02x", source[2], source[1]);
            if (dest_sumindex == 0)
                dest_sumindex = size;
            break;
        case 0x16://enum
            size = 1;
            memcpy(dest, &source[1], size);
            fprintf(stderr, "\n		enum data=%d\n", dest[0]);
            if (dest_sumindex == 0)
                dest_sumindex = size;
            break;
        case 0x1b://time
            size = 3;
            dest[0] = source[1];
            dest[1] = source[2];
            dest[2] = source[3];
            fprintf(stderr, "\n		time %02x %02x %02x ", dest[0], dest[1], dest[2]);
            if (dest_sumindex == 0)
                dest_sumindex = size;
            break;
        case 0x1c://DateTimeBCD
            dest[1] = source[1];//年
            dest[0] = source[2];
            dest[2] = source[3];//月
            dest[3] = source[4];//日
            dest[4] = source[5];//时
            dest[5] = source[6];//分
            dest[6] = source[7];//秒
            size = 7;
            if (dest_sumindex == 0)
                dest_sumindex = size;
            break;
        case 0x55://TSA
            size = source[1];
            memcpy(dest, &source[1], size + 2);// 0 表示长度为 1字节    1表示 长度为2字节 ....  将TSA长度拷贝到地址缓存中
            if (dest_sumindex == 0)
                dest_sumindex = TSA_LEN;
            size = size + 1;
            fprintf(stderr, "TSA %d %02x %02x %02x %02x %02x %02x\n", dest[0], dest[1], dest[2], dest[3], dest[4],
                    dest[5], dest[6]);
            break;
        case 0x5c://MS
            size = 1;
            choicetype = source[1];
            switch (choicetype) {
                case 0:
                case 1:
                    dest[0] = source[1];  //0表示 没有电表  1表示 全部电表
                    fprintf(stderr, "\n		MS:Choice =%02x ", source[1]);
                    size = 1;
                    break;
                case 2:
                    break;
                case 3:
                    break;
                case 4:
                    break;
            }
            if (dest_sumindex == 0)
                dest_sumindex = sizeof(MY_MS);
            fprintf(stderr, "\n		目标地址跳转 %d 字节 ", dest_sumindex);
            break;
        case 0x51://OAD
            size = 4;
            dest[0] = source[2];
            dest[1] = source[1];
            dest[2] = source[3];
            dest[3] = source[4];
            if (dest_sumindex == 0)
                dest_sumindex = size;
            break;
        case 0x54://TI
            dest[0] = source[1];//单位
            dest[2] = source[2];//long unsigned数值
            dest[1] = source[3];
            size = 3;
            if (dest_sumindex == 0)
                dest_sumindex = 3;
            break;
        case 0x5B://CSD
            choicetype = source[1];//01
            if (choicetype == 1) {//road
                dest[0] = choicetype;
                dest[1] = source[3];
                dest[2] = source[2];
                dest[3] = source[4];
                dest[4] = source[5];
                int numm = source[6];//SEQUENCE 0F OAD 数量
                dest[5] = (INT8U) numm;
                fprintf(stderr, "\nnumm=%d", numm);
                for (int k = 0; k < numm; k++) {
                    dest[6 + k * 4 + 0] = source[7 + k * 4 + 1];
                    dest[6 + k * 4 + 1] = source[7 + k * 4 + 0];
                    dest[6 + k * 4 + 2] = source[7 + k * 4 + 2];
                    dest[6 + k * 4 + 3] = source[7 + k * 4 + 3];
                }
                size = 1 + 4 + 1 + numm * 4;// 1:choicetype  4:oad  1:num
            } else {//oad  6字节
                dest[0] = choicetype;
                dest[1] = source[3];
                dest[2] = source[2];
                dest[3] = source[4];
                dest[4] = source[5];
                size = 4 + 1;// 1： choicetype占用1个字节
                fprintf(stderr, "\n%02x %02x %02x %02x ", dest[1], dest[2], dest[3], dest[4]);
            }
            if (dest_sumindex == 0)
                dest_sumindex = sizeof(MY_CSD);
            break;
    }
    source_sumindex = size + 1;// 1：类型占用一个字节
    fprintf(stderr, "\n源缓冲区跳 %d字节 ", source_sumindex);

    for (i = 0; i < strnum; i++) {
        fprintf(stderr, "\n----------i=%d  dest 向前移动 %d", i, dest_sumindex);
        get_BasicUnit(source + source_sumindex, sourceindex, dest + dest_sumindex, destindex);     //no
        source_sumindex += *sourceindex;
        dest_sumindex += *destindex;
    }
    if (i == strnum) {
        if (csdsize > 0) {
            fprintf(stderr, "\n循环结束 csdsize=%d  dest_sumindex=%d", csdsize, dest_sumindex);
            dest_sumindex = csdsize;
        }
    }
    *sourceindex = source_sumindex;
    *destindex = dest_sumindex;
}


void AddBatchMeterInfo(INT8U *data, INT8U type, Action_result *act_ret) {
    CLASS_6001 meter = {};
    int k = 0, saveflg = 0, index = 0;
    INT8U *dealdata = NULL;
    INT8U addnum = 0;// = data[1];

    if (type == 127) {
        dealdata = data;
        addnum = 1;
    } else if (type == 128) {
        dealdata = &data[2];
        addnum = data[1];
    } else
        return;
    fprintf(stderr, "\n测量点数量 %d", addnum);

    for (k = 0; k < addnum; k++) {
        memset(&meter, 0, sizeof(meter));
        index = index + 2;//struct
        index += getLongUnsigned(&dealdata[index], (INT8U *) &meter.sernum);
        index = index + 2;//struct
        index += getOctetstring(1, &dealdata[index], (INT8U *) &meter.basicinfo.addr);
        index += getEnum(1, &dealdata[index], &meter.basicinfo.baud);
        index += getEnum(1, &dealdata[index], &meter.basicinfo.protocol);
        index += getOAD(1, &dealdata[index], &meter.basicinfo.port);
        index += getOctetstring(1, &dealdata[index], (INT8U *) &meter.basicinfo.pwd);
        index += getUnsigned(&dealdata[index], &meter.basicinfo.ratenum);
        index += getUnsigned(&dealdata[index], &meter.basicinfo.usrtype);
        index += getUnsigned(&dealdata[index], &meter.basicinfo.connectype);
        index += getLongUnsigned(&dealdata[index], (INT8U *) &meter.basicinfo.ratedU);
        index += getLongUnsigned(&dealdata[index], (INT8U *) &meter.basicinfo.ratedI);
        index = index + 2;//struct
        index += getOctetstring(1, &dealdata[index], (INT8U *) &meter.extinfo.cjq_addr);
        index += getOctetstring(1, &dealdata[index], (INT8U *) &meter.extinfo.asset_code);
        index += getLongUnsigned(&dealdata[index], (INT8U *) &meter.extinfo.pt);
        index += getLongUnsigned(&dealdata[index], (INT8U *) &meter.extinfo.ct);
        INT8U arraysize = 0;
        index += getArray(&dealdata[index], &arraysize);
        int w = 0;
        for (w = 0; w < arraysize; w++) {
            index = index + 2;//struct
            getOAD(1, &dealdata[index], &meter.aninfo.oad);
        }
        saveflg = saveParaClass(0x6000, (unsigned char *) &meter, meter.sernum);
        if (saveflg != 0) {
            fprintf(stderr, "\n采集档案配置 %d 保存失败", meter.sernum);
            act_ret->DAR = refuse_rw;
        }else {
        	fprintf(stderr, "\n采集档案配置 %d 保存成功", meter.sernum);
        }
        fprintf(stderr, "\n........meter.sernum=%d,addr=%02x-%02x-%02x%02x%02x%02x%02x%02x,baud=%d,protocol=%d", meter.sernum,
                meter.basicinfo.addr.addr[0], meter.basicinfo.addr.addr[1], meter.basicinfo.addr.addr[2],meter.basicinfo.addr.addr[3],
                meter.basicinfo.addr.addr[4], meter.basicinfo.addr.addr[5], meter.basicinfo.addr.addr[6],meter.basicinfo.addr.addr[7],
                meter.basicinfo.baud,meter.basicinfo.protocol);
        fprintf(stderr,
                "\n........[OAD]OI=%04x,attflg=%d,attrindex=%d\n pwd=%02x%02x%02x%02x%02x%02x,ratenum=%d,usrtype=%d,connectype=%d,ratedU=%d,rateI=%d",
                meter.basicinfo.port.OI, meter.basicinfo.port.attflg, meter.basicinfo.port.attrindex,
                meter.basicinfo.pwd[0], meter.basicinfo.pwd[1], meter.basicinfo.pwd[2],
                meter.basicinfo.pwd[3], meter.basicinfo.pwd[4], meter.basicinfo.pwd[5],
                meter.basicinfo.ratenum, meter.basicinfo.usrtype, meter.basicinfo.connectype, meter.basicinfo.ratedU,
                meter.basicinfo.ratedI);
        fprintf(stderr, "\n........[ext]addr=%02x%02x%02x%02x%02x%02x,asset_code=%02x%02x%02x%02x%02x%02x,pt=%d ct=%d",
                meter.extinfo.cjq_addr.addr[0], meter.extinfo.cjq_addr.addr[1], meter.extinfo.cjq_addr.addr[2],
                meter.extinfo.cjq_addr.addr[3], meter.extinfo.cjq_addr.addr[4], meter.extinfo.cjq_addr.addr[5],
                meter.extinfo.asset_code[0], meter.extinfo.asset_code[1], meter.extinfo.asset_code[2],
                meter.extinfo.asset_code[3],
                meter.extinfo.asset_code[4], meter.extinfo.asset_code[5], meter.extinfo.pt, meter.extinfo.ct);
    }
    act_ret->datalen = index;
    act_ret->DAR = success;
}

void AddCjiFangAnInfo(INT8U *data, Action_result *act_ret) {
    act_ret->DAR = success;

    INT8U *buf;
    CLASS_6015 fangAn = {};
    int k = 0;
    INT8U addnum = data[1];        //data[0] = apdu[7]
    INT8U *dealdata = NULL;
    int index = 0;
    fprintf(stderr, "\nsizeof fangAn=%d", sizeof(fangAn));
    fprintf(stderr, "\n添加个数 %d", addnum);
    dealdata = &data[2];
    fprintf(stderr, "=======%02x %02x %02x %02x %02x %02x", dealdata[0], dealdata[1], dealdata[2], dealdata[3],
            dealdata[4], dealdata[5]);
    for (k = 0; k < addnum; k++) {
        memset(&fangAn, 0, sizeof(fangAn));
        index += getStructure(&dealdata[index], NULL);
        index += getUnsigned(&dealdata[index], (INT8U *) &fangAn.sernum);
        fprintf(stderr, "fangan sernum =%d ,index=%d\n", fangAn.sernum, index);
        index += getLongUnsigned(&dealdata[index], (INT8U *) &fangAn.deepsize);
        index = index + 2;//struct
        index += getUnsigned(&dealdata[index], (INT8U *) &fangAn.cjtype);
        fprintf(stderr, "cjtype=%d\n", fangAn.cjtype);
        switch (fangAn.cjtype) {
            case 0:
            case 2:
                fangAn.data.type = 0;        //NULL
                index++;
                break;
            case 1:
                fangAn.data.type = 0x11;    // unsigned
                index += getUnsigned(&dealdata[index], (INT8U *) &fangAn.data.data);
                break;
            case 3:
                fangAn.data.type = 0x54;    // TI
                index += getTI(1, &dealdata[index], (TI *) &fangAn.data.data);
                fprintf(stderr, "\n方案类型：%02x  data=%02x-%02x-%02x\n", fangAn.data.type, fangAn.data.data[0],
                        fangAn.data.data[1], fangAn.data.data[2]);
                break;
            default:
                return;
        }
        INT8U arraysize = 0;
        index += getArray(&dealdata[index], &arraysize);
        fangAn.csds.num = arraysize;
        fprintf(stderr, "fangAn.csds.num=%d\n", fangAn.csds.num);
        int w = 0;
        for (w = 0; w < arraysize; w++) {
            index += getCSD(1, &dealdata[index], (MY_CSD *) &fangAn.csds.csd[w]);
        }
        fprintf(stderr, "\n%02x %02x %02x %02x %02x %02x %02x ",
                dealdata[index], dealdata[index + 1], dealdata[index + 2], dealdata[index + 3], dealdata[index + 4],
                dealdata[index + 5], dealdata[index + 6]);
        index += getMS(1, &dealdata[index], &fangAn.mst);
        index += getEnum(1, &dealdata[index], &fangAn.savetimeflag);
        fprintf(stderr, "\n方案号 ：%d ", fangAn.sernum);
        fprintf(stderr, "\n存储深度 ：%d ", fangAn.deepsize);
        fprintf(stderr, "\n采集类型 ：%d ", fangAn.cjtype);
        fprintf(stderr, "\n采集内容(data) 类型：%02x  data=%d", fangAn.data.type, fangAn.data.data[0]);
        buf = (INT8U *) &fangAn.csds.flag;
        fprintf(stderr, "\ncsd:");
        INT8U type = 0;
        for (int i = 0; i < fangAn.csds.num; i++) {
            type = fangAn.csds.csd[i].type;
            if (type == 0) {
                fprintf(stderr, "\nOAD");
                fprintf(stderr, "\n%04x %02x %02x", fangAn.csds.csd[i].csd.oad.OI, fangAn.csds.csd[i].csd.oad.attflg,
                        fangAn.csds.csd[i].csd.oad.attrindex);
            } else if (type == 1) {
                fprintf(stderr, "\nROAD");
                fprintf(stderr, "\n		OAD-%04x %02x %02x", fangAn.csds.csd[i].csd.road.oad.OI,
                        fangAn.csds.csd[i].csd.road.oad.attflg, fangAn.csds.csd[i].csd.road.oad.attrindex);
                for (w = 0; w < fangAn.csds.csd[i].csd.road.num; w++) {
                    if (fangAn.csds.csd[i].csd.road.oads[w].OI != 0xeeee)
                        fprintf(stderr, "\n		OAD-%04x %02x %02x", fangAn.csds.csd[i].csd.road.oads[w].OI,
                                fangAn.csds.csd[i].csd.road.oads[w].attflg,
                                fangAn.csds.csd[0].csd.road.oads[w].attrindex);
                }
            }
        }
        fprintf(stderr, "\n电能表集合MS ：类型 %d (0:无表   1:全部   2:一组用户   3:一组用户地址   4:一组配置序号   )", fangAn.mst.mstype);
        fprintf(stderr, "\n存储时标选择 ： %d (1:任务开始时间  2：相对当日0点0分  3:相对上日23点59分  4:相对上日0点0分  5:相对当月1日0点0分)",
                fangAn.savetimeflag);
        fprintf(stderr, "\n");

        act_ret->DAR = saveCoverClass(0x6015, fangAn.sernum, &fangAn, sizeof(fangAn), coll_para_save);
    }
    act_ret->datalen = index + 2;    //2 array + num
}

void AddEventCjiFangAnInfo(INT8U *data, Action_result *act_ret) {
    CLASS_6017 eventFangAn = {};
    int i = 0, k = 0;
    INT8U addnum = 0;
    int index = 0;

    index += getArray(&data[index], &addnum);
    index += getStructure(&data[index], NULL);
    fprintf(stderr, "\n添加个数 %d", addnum);
    for (k = 0; k < addnum; k++) {
        memset(&eventFangAn, 0, sizeof(eventFangAn));
        index += getUnsigned(&data[index], (INT8U *) &eventFangAn.sernum);
        if (data[index] == dtstructure) {        //勘误增加了采集方式类型，浙江测试还未修改，故判断
            index += getStructure(&data[index], NULL);
            index += getUnsigned(&data[index], (INT8U *) &eventFangAn.collstyle.colltype);
        }
        index += getArray(&data[index], (INT8U *) &eventFangAn.collstyle.roads.num);
        for (i = 0; i < eventFangAn.collstyle.roads.num; i++)
            index += getROAD(&data[index], &eventFangAn.collstyle.roads.road[i]);
        index += getMS(1, &data[index], &eventFangAn.ms);
        index += getBool(&data[index], &eventFangAn.ifreport);
        index += getLongUnsigned(&data[index], (INT8U *) &eventFangAn.deepsize);
        act_ret->DAR = saveCoverClass(0x6017, eventFangAn.sernum, &eventFangAn, sizeof(eventFangAn), coll_para_save);
    }
    act_ret->datalen = index;

//	index += getArray(&data[index],&addnum);
//	index += getStructure(&data[index],NULL);
//	fprintf(stderr,"\n添加个数 %d",addnum);
//	for(k=0; k<addnum; k++)
//	{
//		memset(&eventFangAn,0,sizeof(eventFangAn));
//		index += getUnsigned(&data[index],(INT8U *)&eventFangAn.sernum);
//		index += getArray(&data[index],(INT8U *)&eventFangAn.roads.num);
//		for(i=0;i<eventFangAn.roads.num;i++)
//			index += getROAD(&data[index],&eventFangAn.roads.road[i]);
//		index += getMS(1,&data[index],&eventFangAn.ms);
//		index += getBool(&data[index],&eventFangAn.ifreport);
//		index += getLongUnsigned(&data[index],(INT8U *)&eventFangAn.deepsize);
//		act_ret->DAR = saveCoverClass(0x6017,eventFangAn.sernum,&eventFangAn,sizeof(eventFangAn),coll_para_save);
//	}
//	act_ret->datalen = index;
}

void AddTaskInfo(INT8U *data, Action_result *act_ret) {
    act_ret->DAR = success;
    CLASS_6013 task = {};
    int k = 0, index = 0;
    INT8U *dealdata = NULL;
    INT8U addnum = data[1];
    fprintf(stderr, "\nsizeof task=%d", sizeof(task));
    fprintf(stderr, "\n添加个数 %d", addnum);
    dealdata = &data[2];
    for (k = 0; k < addnum; k++) {
        memset(&task, 0, sizeof(task));
        index = index + 2;//struct
        index += getUnsigned(&dealdata[index], (INT8U *) &task.taskID);
        index += getTI(1, &dealdata[index], &task.interval);
        index += getEnum(1, &dealdata[index], (INT8U *) &task.cjtype);
        index += getUnsigned(&dealdata[index], (INT8U *) &task.sernum);
        index += getDateTimeS(1, &dealdata[index], (INT8U *) &task.startime);
        index += getDateTimeS(1, &dealdata[index], (INT8U *) &task.endtime);
        index += getTI(1, &dealdata[index], &task.delay);
        index += getEnum(1, &dealdata[index], &task.runprio);
        index += getEnum(1, &dealdata[index], &task.state);
        index += getLongUnsigned(&dealdata[index], (INT8U *) &task.befscript);
        index += getLongUnsigned(&dealdata[index], (INT8U *) &task.aftscript);
        index = index + 2;//struct
        index += getEnum(1, &dealdata[index], &task.runtime.type);
        INT8U arraysize = 0;
        index += getArray(&dealdata[index], &arraysize);
        task.runtime.num = arraysize;
        int w = 0;
        for (w = 0; w < arraysize; w++) {
            index = index + 2;//struct
            index += getUnsigned(&dealdata[index], (INT8U *) &task.runtime.runtime[w].beginHour);
            index += getUnsigned(&dealdata[index], (INT8U *) &task.runtime.runtime[w].beginMin);
            index += getUnsigned(&dealdata[index], (INT8U *) &task.runtime.runtime[w].endHour);
            index += getUnsigned(&dealdata[index], (INT8U *) &task.runtime.runtime[w].endMin);
        }
        fprintf(stderr, "\n任务 ID=%d", task.taskID);
        fprintf(stderr, "\n执行频率 单位=%d   value=%d", task.interval.units, task.interval.interval);
        fprintf(stderr, "\n方案类型 =%d", task.cjtype);
        fprintf(stderr, "\n方案序号 =%d", task.sernum);
        fprintf(stderr, "\n开始时间 =%d年 %d月 %d日 %d时 %d分 %d秒 ", task.startime.year.data, task.startime.month.data,
                task.startime.day.data, task.startime.hour.data, task.startime.min.data, task.startime.sec.data);
        fprintf(stderr, "\n结束时间 =%d年 %d月 %d日 %d时 %d分 %d秒 ", task.endtime.year.data, task.endtime.month.data,
                task.endtime.day.data, task.endtime.hour.data, task.endtime.min.data, task.endtime.sec.data);
        fprintf(stderr, "\n优先级别 =%d", task.runprio);
        fprintf(stderr, "\n任务状态 =%d", task.state);
        fprintf(stderr, "\n运行时段类型 =%02x", task.runtime.type);
        fprintf(stderr, "\n开始  %d时 %d分  ", task.runtime.runtime[0].beginHour, task.runtime.runtime[0].beginMin);
        fprintf(stderr, "\n结束  %d时 %d分  ", task.runtime.runtime[0].endHour, task.runtime.runtime[0].endMin);

        act_ret->DAR = saveCoverClass(0x6013, task.taskID, &task, sizeof(task), coll_para_save);
    }
    act_ret->datalen = index + 2;        //2:array
}

void Set_CSD(INT8U *data) {

}

void CjiFangAnInfo(INT16U attr_act, INT8U *data, Action_result *act_ret) {
    switch (attr_act) {
        case 127:    //方法 127:Add (array 普通采集方案)
            fprintf(stderr, "\n添加普通采集方案");
            AddCjiFangAnInfo(data, act_ret);
            break;
        case 128:    //方法 128:Delete(array 方案编号)
//			DeleteCjFangAn(data[1]);
            break;
        case 129:    //方法 129:Clear( )
            fprintf(stderr, "\n清空普通采集方案");
            clearClass(0x6015);            //普通采集方案放置在6015目录下
            break;
        case 130:    //方法 130:Set_CSD(方案编号,array CSD)
            Set_CSD(data);
            break;
    }
}

void EventCjFangAnInfo(INT16U attr_act, INT8U *data, Action_result *act_ret) {
    switch (attr_act) {
        case 127:    //方法 127:Add(array 事件采集方案)
            fprintf(stderr, "\n添加事件采集方案");
            AddEventCjiFangAnInfo(data, act_ret);
            break;
        case 128:    //方法 128:Delete(array 方案编号)
            //		DeleteEventCjFangAn(data[1]);
            break;
        case 129:    //方法 129:Clear( )
            fprintf(stderr, "\n清空事件采集方案");
            clearClass(0x6016);
            break;
        case 130:    //方法 130:Set_CSD(方案编号,array CSD)
            //		UpdateReportFlag(data);
            break;
    }
}

void print_601d(CLASS_601D reportplan) {
    int j = 0;
    fprintf(stderr, "\n[1]上报方案编号 [2]上报通道 [3]上报响应超时时间 [4]最大上报次数 [5]上报内容 {[5.1]类型(0:OAD,1:RecordData) [5.2]数据 [5.3]RSD}");
    fprintf(stderr, "\n[1]上报方案编号:%d \n", reportplan.reportnum);
    fprintf(stderr, "[2]OAD[%d] ", reportplan.chann_oad.num);
    for (j = 0; j < reportplan.chann_oad.num; j++) {
        fprintf(stderr, "%04x-%02x%02x ", reportplan.chann_oad.oadarr[j].OI, reportplan.chann_oad.oadarr[j].attflg,
                reportplan.chann_oad.oadarr[j].attrindex);
    }
    fprintf(stderr, " [3]TI %d-%d ", reportplan.timeout.units, reportplan.timeout.interval);
    fprintf(stderr, " [4]%d ", reportplan.maxreportnum);
    fprintf(stderr, " [5.1]%d ", reportplan.reportdata.type);
    if (reportplan.reportdata.type == 0) {
        fprintf(stderr, " [5.2]OAD:%04x-%02x%02x ", reportplan.reportdata.data.oad.OI,
                reportplan.reportdata.data.oad.attflg, reportplan.reportdata.data.oad.attrindex);
    } else {
        fprintf(stderr, " [5.2]OAD:%04x-%02x%02x ", reportplan.reportdata.data.oad.OI,
                reportplan.reportdata.data.oad.attflg, reportplan.reportdata.data.oad.attrindex);
        print_rcsd(reportplan.reportdata.data.recorddata.csds);
        fprintf(stderr, " [5.4]");
        print_rsd(reportplan.reportdata.data.recorddata.selectType, reportplan.reportdata.data.recorddata.rsd);
    }
    fprintf(stderr, "\n\n");
}

/* 601d :上报方案
 * */
void AddReportInfo(INT8U *data, Action_result *act_ret) {
    CLASS_601D reportplan = {};
    int k = 0, j = 0;
    INT8U addnum = 0, strunum = 0;
//	INT8U roadnum=0;
    int index = 0;
//	INT16U source_sumindex=0,source_index=0,dest_sumindex=0,dest_index=0;

    index += getArray(&data[index], &addnum);
    fprintf(stderr, "\n添加个数 %d", addnum);
    for (k = 0; k < addnum; k++) {
        memset(&reportplan, 0, sizeof(CLASS_601D));
        index += getStructure(&data[index], &strunum);
        index += getUnsigned(&data[index], &reportplan.reportnum);
        index += getArray(&data[index], &reportplan.chann_oad.num);
        for (j = 0; j < reportplan.chann_oad.num; j++) {
            index += getOAD(1, &data[index], &reportplan.chann_oad.oadarr[j]);
        }
        index += getTI(1, &data[index], &reportplan.timeout);
        index += getUnsigned(&data[index], &reportplan.maxreportnum);
        index += getStructure(&data[index], &strunum);
        index += getUnsigned(&data[index], &reportplan.reportdata.type);
        switch (reportplan.reportdata.type) {
            case 0:    //OAD
                index += getOAD(1, &data[index], &reportplan.reportdata.data.oad);
                break;
            case 1://RecordData
                index += getStructure(&data[index], &strunum);
                fprintf(stderr, "RecordData strnum=%d\n", strunum);
                index += getOAD(1, &data[index], &reportplan.reportdata.data.recorddata.oad);
                index += get_BasicRCSD(1, &data[index], &reportplan.reportdata.data.recorddata.csds);
                index += get_BasicRSD(1, &data[index], (INT8U *) &reportplan.reportdata.data.recorddata.rsd,
                                      &reportplan.reportdata.data.recorddata.selectType);
                break;
        }
        print_601d(reportplan);
        act_ret->DAR = saveCoverClass(0x601d, reportplan.reportnum, &reportplan, sizeof(CLASS_601D), coll_para_save);
    }
    fprintf(stderr, "601d  return index=%d\n", index);
    act_ret->datalen = index;
}

void ReportInfo(INT16U attr_act, INT8U *data, Action_result *act_ret) {
    switch (attr_act) {
        case 127:    //方法 127:Add(array 上报方案)
            fprintf(stderr, "\nAdd 上报方案");
            AddReportInfo(data, act_ret);
            break;
        case 128:    //方法 128:Delete(array 方案编号)

            break;
        case 129:    //方法 129:Clear( )
            fprintf(stderr, "\n清空上报方案集");
            clearClass(0x601D);
            break;
    }
}

void TaskInfo(INT16U attr_act, INT8U *data, Action_result *act_ret) {
    switch (attr_act) {
        case 127://方法 127:Add (任务配置单元)
            AddTaskInfo(data, act_ret);
            break;
        case 128://方法 128:Delete(array任务 ID )

            deleteClass(0x6013, 1);
            break;
        case 129://方法 129:Clear()
            fprintf(stderr, "\n清空采集任务配置表");
            clearClass(0x6013);        //任务配置单元存放在/nand/para/6013目录
            break;
    }
}

void TerminalInfo(INT16U attr_act, INT8U *data) {
    switch (attr_act) {
        case 1://设备复位
            Reset_add();
            fprintf(stderr, "\n4300 设备复位！");
            break;
        case 3://数据初始化
        case 5://事件初始化
        case 6://需量初始化
            dataInit(attr_act);
            //Event_3100(NULL,0,memp);//初始化，产生事件
            Reset_add();            //国网台体测试,数据初始化认为是复位操作
            fprintf(stderr, "\n终端数据初始化!");
            break;
        case 151://湖南切换到3761规约程序转换主站通信参数
            fprintf(stderr, "\nhunan change 3761 protocol f151\n");
            if (save_protocol_3761_tx_para(data))//写文件成功
            {
                system((const char *) "mv /nor/rc.d/rc.local /nor/rc.d/698_rc.local");
                sleep(1);
                system((const char *) "mv /nor/rc.d/3761_rc.local /nor/rc.d/rc.local");
                sleep(1);
                system((const char *) "chmod 777 /nor/rc.d/rc.local");
                sleep(1);
                if (access("/nor/rc.d/rc.local", F_OK) != 0 || access("/nor/rc.d/rc.local", X_OK) != 0) {
                    if (write_3761_rc_local()) {
                        sleep(1);
                        system((const char *) "chmod 777 /nor/rc.d/rc.local");
                        sleep(1);
                    }
                    system((const char *) "reboot");                    //TODO:写文件成功切换rc.local
                } else {
                    system((const char *) "reboot");                    //TODO:写文件成功切换rc.local
                }
            }
            break;
    }
}

void FileTransMothod(INT16U attr_act, INT8U *data) {
    INT8U name[128];
    INT8U sub_name[128];
    INT8U version[128];
    INT8U path[256];

    memset(name, 0x00, sizeof(name));
    memset(sub_name, 0x00, sizeof(sub_name));
    memset(version, 0x00, sizeof(version));
    memset(path, 0, sizeof(path));
    INT32U file_length = 0;
    INT16U block_length = 0;
    INT8U crc = 0;
    INT16U block_start = 0;

    switch (attr_act) {
        case 7://启动传输
            //开始解析固定的信息
            if (data[2] == 0x02 && data[3] == 0x05) {
                int data_index = 4;
                if (data[data_index] != 0x0a) {
                    fprintf(stderr, "未能找到文件名\n");
                    goto err;
                }
                data_index++;
                if (data[data_index] >= 128) {
                    fprintf(stderr, "文件名过长\n");
                    goto err;
                }
                memcpy(name, &data[data_index + 1], data[data_index]);
                data_index += data[data_index] + 1;

                if (data[data_index] != 0x0a) {
                    fprintf(stderr, "未能找到文件扩展名\n");
                    goto err;
                }
                data_index++;
                if (data[data_index] >= 128) {
                    fprintf(stderr, "文件扩展名过长\n");
                    goto err;
                }
                memcpy(sub_name, &data[data_index + 1], data[data_index]);
                data_index += data[data_index] + 1;

                if (data[data_index] != 0x06) {
                    fprintf(stderr, "未能找到文件长度\n");
                    goto err;
                }
                data_index++;
                file_length += data[data_index++];
                file_length <<= 8;
                file_length += data[data_index++];
                file_length <<= 8;
                file_length += data[data_index++];
                file_length <<= 8;
                file_length += data[data_index++];
                data_index += 3;

                if (data[data_index] != 0x0a) {
                    fprintf(stderr, "未能找到文件版本信息\n");
                    goto err;
                }
                data_index++;
                if (data[data_index] >= 128) {
                    fprintf(stderr, "文件版本信息过长\n");
                    goto err;
                }
                memcpy(version, &data[data_index + 1], data[data_index]);
                data_index += data[data_index] + 1;

                if (data[data_index] != 0x12) {
                    fprintf(stderr, "未能找到文件传输块大小\n");
                    goto err;
                }
                data_index++;
                block_length += data[data_index++];
                block_length <<= 8;
                block_length += data[data_index++];

                data_index += 2;
                if (data[data_index] != 0x16 || data[data_index + 1] != 0x00) {
                    fprintf(stderr, "无法找到文件校验方式或者文件校验方式不是CRC{%d}\n", data[data_index + 1]);
                    goto err;
                }
                data_index += 2;
                if (data[data_index] != 0x09) {
                    fprintf(stderr, "无法找到文件校验的crc\n");
                    goto err;
                }
                data_index++;
                crc = data[data_index];
            } else if (data[2] == 0x02 && data[3] == 0x06) {
                int data_index = 4;
                if (data[data_index] != 0x0a) {
                    fprintf(stderr, "无法找到源文件\n");
                    goto err;
                }
                data_index++;
                data_index += data[data_index] + 1;
                if (data[data_index] != 0x0a) {
                    fprintf(stderr, "无法找到目标文件\n");
                    goto err;
                }
                data_index++;
                memcpy(name, &data[data_index + 1], data[data_index]);
                data_index += data[data_index] + 1;
                if (data[data_index] != 0x06) {
                    fprintf(stderr, "未能找到文件长度\n");
                    goto err;
                }
                data_index++;
                file_length += data[data_index++];
                file_length <<= 8;
                file_length += data[data_index++];
                file_length <<= 8;
                file_length += data[data_index++];
                file_length <<= 8;
                file_length += data[data_index++];
                data_index += 3;
                if (data[data_index] != 0x0a) {
                    fprintf(stderr, "未能找到版本信息\n");
                    goto err;
                }
                data_index++;
                data_index += data[data_index] + 1 + 2;

                if (data[data_index] != 0x12) {
                    fprintf(stderr, "未能找到分段长度信息\n");
                    goto err;
                }
                data_index++;
                block_length += data[data_index++];
                block_length <<= 8;
                block_length += data[data_index++];
            } else {
                goto err;
            }

            snprintf(path, sizeof(path), "/nand/UpFiles/%s.%s.temp", name, sub_name);

            fprintf(stderr, "启动传输 文件名:%s,文件长度:%d,文件校验%02x,块长度%d\n", path, file_length, crc, block_length);
            createFile(path, file_length, crc, block_length);
            break;
        case 8://写文件
            if (data[0] == 0x02 && data[1] == 0x02) {
                int data_index = 2;
                INT16U block_index = 0;
                INT32U block_sub_len = 0;
                if (data[data_index] != 0x12) {
                    fprintf(stderr, "未能找到分段序号\n");
                    goto err;
                }
                data_index++;
                block_index += data[data_index++];
                block_index <<= 8;
                block_index += data[data_index++];

                if (data[data_index] != 0x09) {
                    fprintf(stderr, "未能找到分段长度\n");
                    goto err;
                }
                data_index++;
                if (data[data_index] > 128) {
                    int length_len = data[data_index] - 128;
                    if (length_len > 2) {
                        fprintf(stderr, "分片过长\n");
                        goto err;
                    }
                    data_index++;
                    for (int i = 0; i < length_len; i++) {
                        block_sub_len += data[data_index++];
                        block_sub_len <<= 8;
                    }
                    block_sub_len >>= 8;
                } else {
                    block_sub_len = data[data_index++];
                }
                block_start = data_index;
                fprintf(stderr, "写入文件 分段序号%d,分段长度%d\n", block_index, block_sub_len);
                appendFile(block_index, block_sub_len, &data[block_start]);
            } else {
                goto err;
            }
            break;
        case 9://读文件
            fprintf(stderr, "读取文件\n");
            break;
    }

    err:
    return;
}

void FreezeAtti(OAD oad, Relate_Object one_obj) {
    int i = 0, j = 0;
    FreezeObject FreeObj = {};

    memset(&FreeObj, 0, sizeof(FreezeObject));
    readCoverClass(oad.OI, 0, &FreeObj, sizeof(FreezeObject), para_vari_save);
    switch (oad.attflg) {
        case 5:        //删除
            for (i = 0; i < FreeObj.RelateNum; i++) {
                if (memcmp(&FreeObj.RelateObj[i].oad, &one_obj.oad, sizeof(OAD)) == 0) {
                    if (FreeObj.RelateNum) FreeObj.RelateNum = FreeObj.RelateNum - 1;
                    for (j = i; j < FreeObj.RelateNum; j++) {        //删除匹配的OAD
                        memcpy(&FreeObj.RelateObj[j], &FreeObj.RelateObj[j + 1], sizeof(Relate_Object));
                    }
                    break;
                }
            }
            break;
        case 7:        //批量添加冻结对象属性
            for (i = 0; i < FreeObj.RelateNum; i++) {
                if (memcmp(&FreeObj.RelateObj[i].oad, &one_obj.oad, sizeof(OAD)) == 0) {        //已有，替换
                    memcpy(&FreeObj.RelateObj[i], &one_obj, sizeof(Relate_Object));
                    break;
                }
            }
            if (i == FreeObj.RelateNum) {        //增加
                memcpy(&FreeObj.RelateObj[i], &one_obj, sizeof(Relate_Object));
                FreeObj.RelateNum = FreeObj.RelateNum + 1;
            }
            break;
    }
    fprintf(stderr, "冻结 %04x，关联个数=%d\n", oad.OI, FreeObj.RelateNum);
    for (i = 0; i < FreeObj.RelateNum; i++) {
        fprintf(stderr, "OAD=%04x-%02x-%02x,priod=%d,depth=%d\n", FreeObj.RelateObj[i].oad.OI,
                FreeObj.RelateObj[i].oad.attflg,
                FreeObj.RelateObj[i].oad.attrindex, FreeObj.RelateObj[i].freezePriod, FreeObj.RelateObj[i].saveDepth);
    }
    saveCoverClass(oad.OI, 0, &FreeObj, sizeof(FreezeObject), para_vari_save);
}

void FreezeAction(OAD oad, INT8U *data, Action_result *act_ret) {
    int index = 0;
    INT8U SeqOfNum = 0, i = 0;
    Relate_Object one_obj = {};

    switch (oad.attflg) {
        case 5:        //删除一个冻结对象属性
            index += getOAD(1, &data[index], &one_obj.oad);
            fprintf(stderr, "删除 oad=%04x-%02x-%02x\n", oad.OI, oad.attflg, oad.attrindex);
            FreezeAtti(oad, one_obj);
            break;
        case 7:        //批量添加冻结对象属性
            index += getArray(&data[index], &SeqOfNum);
            for (i = 0; i < SeqOfNum; i++) {
                index += getStructure(&data[index], NULL);
                index += getLongUnsigned(&data[index], (INT8U *) &one_obj.freezePriod);
                index += getOAD(1, &data[index], &one_obj.oad);
                index += getLongUnsigned(&data[index], (INT8U *) &one_obj.saveDepth);

                fprintf(stderr, "添加%d：freezeProid=%d,oad=%04x-%02x-%02x,saveDepth=%d\n", i, one_obj.freezePriod,
                        one_obj.oad.OI, one_obj.oad.attflg, one_obj.oad.attrindex, one_obj.saveDepth);
                FreezeAtti(oad, one_obj);
            }
            break;
    }
    act_ret->datalen = index;
    act_ret->DAR = success;
}

void MeterInfo(INT16U attr_act, INT8U *data, Action_result *act_ret) {
    int seqnum = 0;
    switch (attr_act) {
        case 127://方法 127:Add (采集档案配置单元)
            AddBatchMeterInfo(data, attr_act, act_ret);
            break;
        case 128://方法 128:AddBatch(array 采集档案配置单元)
            AddBatchMeterInfo(data, attr_act, act_ret);
            break;
        case 129://方法 129:Update(配置序号,基本信息)
            break;
        case 130://方法 130:Update(配置序号,扩展信息,附属信息)
            break;
        case 131://方法 131:Delete(配置序号)
            getLongUnsigned(&data[0], (INT8U *) &seqnum);
            fprintf(stderr, "delete method seqnum=%d (%x)\n", seqnum, seqnum);
            delClassBySeq(0x6000, NULL, seqnum);
            break;
        case 132://方法 132:Delete(基本信息)
            break;
        case 133://方法 133:Delete(通信地址, 端口号)
            break;
        case 134://方法 134:Clear()
            fprintf(stderr, "\n清空采集档案配置表");
            clearClass(0x6000);
            break;
    }
}

int EventMothod(OAD oad, INT8U *data) {
    fprintf(stderr, "\n事件对象方法操作");
    switch (oad.attflg) {
        case 1://复位
            fprintf(stderr, "\n复位");
            clearClass(oad.OI);
            break;
    }
    return 0;
}

//esam 698处理函数返回0，正常，可以组上行帧。返回负数，异常，组错误帧，同意用0x16
INT32S EsamMothod(INT16U attr_act, INT8U *data) {
    INT32S ret = -1;
    switch (attr_act) {
        case 7://秘钥更新
            ret = esamMethodKeyUpdate(data);
            break;
        case 8://证书更新
        case 9://设置协商时效
            ret = esamMethodCcieSession(data);
            break;
        default:
            ret = -1;
            break;
    }
    return ret;
}

int doObjectAction(OAD oad, INT8U *data, Action_result *act_ret) {
    INT32S errflg = 0;
    INT16U oi = oad.OI;
    INT8U attr_act = oad.attflg;
    INT8U oihead = (oi & 0xF000) >> 12;
    fprintf(stderr, "\n----------  oi =%04x   ", oi);
    switch (oihead) {
        case 3:            //事件类对象方法操作
            EventMothod(oad, data);
            break;
    }
    switch (oi) {
        case 0x4300:    //终端对象
            TerminalInfo(attr_act, data);
            break;
        case 0x5004:    //日冻结
        case 0x5006:    //月冻结
            FreezeAction(oad, data, act_ret);
            break;
        case 0x6000:    //采集档案配置表
            MeterInfo(attr_act, data, act_ret);
            break;
        case 0x6002:    //搜表
            break;
        case 0x6012:    //任务配置表
            TaskInfo(attr_act, data, act_ret);
            break;
        case 0x6014:    //普通采集方案集
            CjiFangAnInfo(attr_act, data, act_ret);
            break;
        case 0x6016:    //事件采集方案
            EventCjFangAnInfo(attr_act, data, act_ret);
            break;
        case 0x6018:    //透明方案集
            break;
        case 0x601C:    //上报方案
            ReportInfo(attr_act, data, act_ret);
            break;
        case 0x601E:    //采集规则库
            break;
        case 0xF001: //文件传输
            FileTransMothod(attr_act, data);
            break;
        case 0xF100:
            errflg = EsamMothod(attr_act, data);
            if (errflg > 0) {
                act_ret->DAR = 0;
//				act_ret->datalen = 1;
            }
            break;
        case 0x2301:
            class23_selector(1, attr_act, data, act_ret);
            break;
        case 0x2302:
            class23_selector(2, attr_act, data, act_ret);
            break;
        case 0x2303:
            class23_selector(3, attr_act, data, act_ret);
            break;
        case 0x2304:
            class23_selector(4, attr_act, data, act_ret);
            break;
        case 0x2305:
            class23_selector(5, attr_act, data, act_ret);
            break;
        case 0x2306:
            class23_selector(6, attr_act, data, act_ret);
            break;
        case 0x2307:
            class23_selector(7, attr_act, data, act_ret);
            break;
        case 0x2308:
            class23_selector(8, attr_act, data, act_ret);
            break;
        case 0x8001:
            class8001_act_route(1, attr_act, data, act_ret);
            break;
        case 0x8103:
            class8103_act_route(1, attr_act, data, act_ret);
            break;
        case 0x8104:
            class8104_act_route(1, attr_act, data, act_ret);
            break;
        case 0x8105:
            class8105_act_route(1, attr_act, data, act_ret);
            break;
        case 0x8106:
            class8106_act_route(1, attr_act, data, act_ret);
            break;
        case 0x8107:
            class8107_act_route(1, attr_act, data, act_ret);
            break;
        case 0x8108:
            class8108_act_route(1, attr_act, data, act_ret);
            break;

    }
    if (oi == 0x4300 && attr_act == 1) {        //设备复位
        memp->oi_changed.reset++;
    }
    if (oi == 0x4300 && attr_act == 3) {        //数据区初始化
        memp->oi_changed.init++;
    }
    setOIChange(oi);
    return success;    //DAR=0，成功	TODO：增加DAR各种错误判断
}
