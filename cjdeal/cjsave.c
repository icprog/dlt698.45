

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>
#include "sys/reboot.h"
#include <wait.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "cjsave.h"
/*
 * 计算总共有几个oad，来确定给文件头流出多少空间
 * Task文件头格式：
 * 2个字节：文件头长度
 * 2个字节：一个TSA长度
 * 文件头：n个HEAD_UNIT结构
 *       1.  202a-0200:服务器地址
 *       2.  6040-0200:采集启动时标
 *       3.  6041-0200：采集成功时标
 *       4.  6042-0200：采集存储时标
 *       5...n  任务CSD描述
 */
INT16U FixHeadUnit(INT8U *headbuf,INT8U *fixlen,ROAD *road_eve)
{
	static INT8U head_oad[4][4]={{0x20,0x2a,0x02,0x00},{0x60,0x40,0x02,0x00},{0x60,0x41,0x02,0x00},{0x60,0x42,0x02,0x00}};
	static INT8U head_oad_len[4]={0x0012,0x0008,0x0008,0x0008};
	static INT8U headeve_oad[3][4]={{0x20,0x2a,0x02,0x00},{0x20,0x1e,0x02,0x00},{0x20,0x20,0x02,0x00}};
	static INT8U headeve_oad_len[3]={0x0012,0x0008,0x0008};
	int	  i=0,circnt=0,index=0;
	HEAD_UNIT	unit[4]={};
	*fixlen = 0;
	if(road_eve == NULL)
	{
		for(i=0;i<circnt;i++) {
			memset(&unit[i].oad_m,0,sizeof(OAD));
			unit[i].oad_r.OI = head_oad[i][0];
			unit[i].oad_r.OI = (unit[i].oad_r.OI<<8) | head_oad[i][1];
			unit[i].oad_r.attflg = head_oad[i][2];
			unit[i].len = head_oad_len[i];
			*fixlen += head_oad_len[i];
		}
	}
	else
	{
		for(i=0;i<circnt;i++) {
			memset(&unit[i].oad_m,0,sizeof(OAD));
			unit[i].oad_r.OI = headeve_oad[i][0];
			unit[i].oad_r.OI = (unit[i].oad_r.OI<<8) | headeve_oad[i][1];
			unit[i].oad_r.attflg = headeve_oad[i][2];
			unit[i].len = headeve_oad_len[i];
			*fixlen += headeve_oad_len[i];
		}
	}
	if(headbuf!=NULL) {
		memcpy(headbuf,unit,sizeof(unit));
		index += sizeof(unit);
	}
	return index;
}

/*
 * 根据招测的RCSD计算招测的OI个数
 * */
INT16U CalcHeadRcsdUnitNum(CSD_ARRAYTYPE csds)
{
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
		if(csds.csd[i].type == 0)
			headunit_num++;
		if(csds.csd[i].type == 1)
			headunit_num += csds.csd[i].csd.road.num;
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
INT16U CalcOILen(OAD oad,INT8U rate)
{
	FILE *fp;
	char ln[60];
	char lnf[5];
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

		memset(lnf,0x00,sizeof(lnf));
		memcpy(lnf,&ln[0],4);
		fprintf(stderr," lnf=%s\n",lnf);
		oi_tmp = strtol(lnf,NULL,16);
		if(oi_tmp != oad.OI)
			continue;
		memset(lnf,0x00,sizeof(lnf));
		memcpy(lnf,&ln[8],3);
		oi_len = strtol(lnf,NULL,10)+1;		//返回长度+1个字节数据类型描述
		memset(lnf,0x00,sizeof(lnf));
		memcpy(lnf,&ln[12],2);
		ic_type = strtol(lnf,NULL,10);
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

INT8U getOneUnit(INT8U *headbuf,OAD oad_m,OAD oad_r,INT16U len)
{
	HEAD_UNIT	headunit={};

	memcpy(&headunit.oad_m,&oad_m,sizeof(OAD));
	memcpy(&headunit.oad_r,&oad_r,sizeof(OAD));
	headunit.len = len;
	memcpy(headbuf,&headunit,sizeof(HEAD_UNIT));
	return (sizeof(HEAD_UNIT));
}

/*
 * 结构为四个字节长度+TSA(0x00+40010200+2个字节长度)+3*时标+CSD
 * unitlen_z长度为此任务当日需要抄的全部数据长度，以此将一个测量点一天的数据放在一个地方
 */
void CreateSaveHead(char *fname,ROAD *road_eve,CSD_ARRAYTYPE csds,INT16U *headlen,INT16U *unitlen,INT16U *unitnum,INT16U freq,INT8U wrflg)
{
	INT16U pindex=0,len_tmp=0,csd_unitnum=0;
	int i=0,j=0;
	INT8U *headbuf=NULL,fixlen=0;
	OAD	 oad_m={};
	*unitlen = 0;


	if(csds.num == 0xee || csds.num == 0)
	{
		asyslog(LOG_WARNING, "cjsave :csds.num=%d,return!!!error!!!",csds.num);
		return;
	}
	csd_unitnum = CalcHeadRcsdUnitNum(csds);
	if(headbuf == NULL) {
		*headlen=csd_unitnum*sizeof(HEAD_UNIT)+4;	//4:文件头长度+TSA块长度
		headbuf = (INT8U *)malloc(*headlen);
	}
	headbuf[pindex++] = (*headlen & 0xff00)>>8;//文件头长度
	headbuf[pindex++] = (*headlen & 0x00ff);
	headbuf[pindex++] = 0x00;
	headbuf[pindex++] = 0x00;//长度
	pindex += FixHeadUnit(&headbuf[pindex],&fixlen,road_eve);
	if(csds.num > MY_CSD_NUM)//超了
		csds.num = MY_CSD_NUM;
	for(i=0;i<csds.num;i++)
	{
		if(csds.csd[i].type == 0xee)
			break;
		if(csds.csd[i].type != 0 && csds.csd[i].type != 1)
			continue;
		if(csds.csd[i].type == 0)	//OAD
		{
			len_tmp = CalcOILen(csds.csd[i].csd.oad,4);//多一个数据类型
			memset(&oad_m,0,sizeof(OAD));
			pindex += getOneUnit(&headbuf[pindex],oad_m,csds.csd[i].csd.oad,len_tmp);
			*unitlen += len_tmp;
			(*unitnum)++;
		}else if(csds.csd[i].type == 1)	//ROAD
		{
			if(csds.csd[i].csd.road.num == 0xee)
				continue;
			if(csds.csd[i].csd.road.num > ROAD_OADS_NUM)//超了
				csds.csd[i].csd.road.num = ROAD_OADS_NUM;
			for(j=0;j<csds.csd[i].csd.road.num;j++)
			{
				if(csds.csd[i].csd.road.oads[j].OI == 0xeeee)
					break;
				len_tmp = CalcOILen(csds.csd[i].csd.road.oads[j],4);//多一个数据类型
				pindex += getOneUnit(&headbuf[pindex],csds.csd[i].csd.road.oad,csds.csd[i].csd.road.oads[j],len_tmp);
				*unitlen += len_tmp;
				(*unitnum)++;
			}
		}
	}
	*unitlen=freq*(fixlen+*unitlen);//一个单元存储TSA共用,在结构最前面，每个单元都有3个时标和数据，预留出合适大小，以能存下一个TSA所有数据
	asyslog(LOG_WARNING, "cjsave 存储文件头%s fixlen=%d,*unitlen=%d,freq=%d",fixlen,*unitlen,freq);
	headbuf[2] = (*unitlen & 0xff00) >> 8;//数据单元长度
	headbuf[3] = *unitlen & 0x00ff;
	if(wrflg==1)
	{
		datafile_write(fname, headbuf, *headlen, 0);
	}
	if(headbuf) {
		free(headbuf);
	}
}
/*
 * 读取文件头长度和块数据长度
 */
void ReadFileHeadLen(FILE *fp,INT16U *headlen,INT16U *blocklen)
{
	INT16U headlength=0,blocklength=0;
	fread(&headlength,2,1,fp);
	*headlen = ((headlength>>8)+((headlength&0xff)<<8));
	fread(&blocklength,2,1,fp);
	*blocklen = ((blocklength>>8)+((blocklength&0xff)<<8));
}
void ReadFileHead(char *fname,INT16U headlen,INT16U unitlen,INT16U unitnum,INT8U *headbuf)
{
	FILE *fp = NULL;
	HEAD_UNIT *headunit = NULL;
	int i=0;
	fp = fopen(fname,"r");
	if(fp==NULL)
	{
		fprintf(stderr,"\n%s open error,file not exist!\n",fname);
		return ;
	}
	else
	{
		fread(headbuf,headlen,1,fp);
		headunit = malloc(headlen-4);
		memcpy(headunit,&headbuf[4],headlen-4);
		free(headunit);
		fclose(fp);
	}
}


/*
 * 存储普通采集方案数据，
 * 数据格式：文件头结构：标注文件TSA和时标及csd格式，开始4个字节为文件头长度和每个数据单元长度 数据结构：存储各数据项值
 * 分测量点存储，一个TSA的全部数据放到一起，例如24个点的曲线数据，则按采集个数编号放入到一个位置,减少索引时间
 */
void SaveNorData(INT8U taskid,ROAD *road_eve,INT8U *databuf,int datalen)//存储事件时指针road_eve定义为NULL
{
	TS ts_now;
	FILE *fp;
	CSD_ARRAYTYPE csds;
	char	fname[FILENAMELEN]={};
	INT8U *databuf_tmp=NULL;
	int savepos=0,currpos=0,i=0;
	INT16U headlen=0,unitlen=0,unitnum=0,unitseq=0,runtime=0;//runtime执行次数
	TASKSET_INFO tasknor_info;
	memset(fname,0,sizeof(fname));
	memset(&csds,0x00,sizeof(ROAD));
	csds.num = 1;
	csds.csd[0].type = 1;//road
	if(road_eve == NULL)//不是事件
	{
		TSGet(&ts_now);//用的当前时间，测试用，需要根据具体存储时标选择来定义
		if(ReadTaskInfo(taskid,&tasknor_info)!=1)
			return;
		runtime = tasknor_info.runtime;
		memcpy(&csds,&tasknor_info.csds,sizeof(CSD_ARRAYTYPE));//
		getTaskFileName(taskid,ts_now,fname);
	}
	else
	{
		//1 初始化csds
		runtime = 1;//只存上一次 记录
		memcpy(&csds.csd[0].csd.road,road_eve,sizeof(ROAD));//
		getEveFileName(road_eve->oad.OI,fname);//创建eve文件
	}
	fp = fopen(fname,"r");
	if(fp == NULL)//文件没内容 组文件头，如果文件已存在，提取文件头信息
	{
		CreateSaveHead(fname,road_eve,csds,&headlen,&unitlen,&unitnum,runtime,1);//写文件头信息并返回
		asyslog(LOG_WARNING, "cjsave 存储文件头%s headlen=%d unitlen=%d unitnum=%d runtime=%d",fname,headlen,unitlen,unitnum,runtime);
		databuf_tmp = malloc(unitlen);
		savepos=0;
	}
	else
	{
		ReadFileHeadLen(fp,&headlen,&unitlen);
		databuf_tmp = malloc(unitlen);
		fseek(fp,headlen,SEEK_SET);//跳过文件头
		while(!feof(fp))
		{
			if(fread(databuf_tmp,unitlen,1,fp)==0)
			{
				break;
			}
			if(memcmp(databuf_tmp,databuf,18)==0)//找到了存储结构的位置，一个存储结构可能含有unitnum个单元
			{
				savepos=ftell(fp)-unitlen;
				if(road_eve != NULL)//存储事件
				{
					if(memcmp(&databuf_tmp[18],&databuf[18],16)==0)//比对事件发生时间和结束时间是否相同，相同时间未发生，不保存
					{
						if(fp != NULL)
							fclose(fp);
						if(databuf_tmp != NULL)
							free(databuf_tmp);
						return;//事件未发生，不存
					}
					else
					{
						//发送消息通知主动上报 todo
						fprintf(stderr,"\n--事件OI%02x发生!!\n",road_eve->oad.OI);
					}
				}

				break;
			}
		}
	}
	if(fp != NULL)
		currpos = ftell(fp);
	if(savepos==0)//存储位置为0.说明文件中没找到，则应添加而不是覆盖
	{
		if(currpos == 0)//第一个存储的
			savepos=headlen;
		else
			savepos=currpos;
		memset(databuf_tmp,0x00,unitlen);
		for(i=0;i<runtime;i++)
			memcpy(&databuf_tmp[unitlen*i/runtime],databuf,17);//每个小单元地址附上
	}
	unitseq = (ts_now.Hour*60*60+ts_now.Minute*60+ts_now.Sec)/((24*60*60)/runtime)+1;
	asyslog(LOG_NOTICE,"存储序号: unitseq=%d runtime=%d",unitseq,runtime);
	if(unitseq > runtime)
	{
		if(databuf_tmp != NULL)
			free(databuf_tmp);
		return ;//出错了，序列号超过了总长度
	}
	memcpy(&databuf_tmp[unitlen*(unitseq-1)/runtime],databuf,datalen);
	if(datalen != unitlen/runtime)
	{
		if(databuf_tmp != NULL)
			free(databuf_tmp);
		if(road_eve == NULL)
			asyslog(LOG_NOTICE,"数据长度不对，不存: datalen=%d,need=%d",datalen,unitlen/runtime);
		else
			asyslog(LOG_NOTICE,"事件长度不对，不存: datalen=%d,need=%d",datalen,unitlen/runtime);
		return ;//长度不对
	}
	else
	{
		if(road_eve == NULL)
			asyslog(LOG_NOTICE,"任务数据存储: %s,savepos=%d,unitlen=%d",fname,savepos,unitlen);
		else
			asyslog(LOG_NOTICE,"事件数据存储: %s,savepos=%d,unitlen=%d",fname,savepos,unitlen);
		datafile_write(fname, databuf_tmp, unitlen, savepos);
	}
	if(fp!=NULL)
		fclose(fp);
	if(databuf_tmp != NULL)
		free(databuf_tmp);
}
