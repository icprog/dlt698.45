

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
	static INT8U headeve_oad[4][4]={{0x20,0x2a,0x02,0x00},{0x20,0x22,0x02,0x00},{0x20,0x1e,0x02,0x00},{0x20,0x20,0x02,0x00}};
	static INT8U headeve_oad_len[4]={0x0012,0x0005,0x0008,0x0008};
	int	  i=0,index=0;
	HEAD_UNIT	unit[4]={};
	*fixlen = 0;
	if(road_eve == NULL)
	{
		for(i=0;i<4;i++) {
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
		for(i=0;i<4;i++) {
			if(i==0)
				memset(&unit[i].oad_m,0,sizeof(OAD));
			else
				memcpy(&unit[i].oad_m,&road_eve->oad.OI,sizeof(OAD));
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
			len_tmp = CalcOIDataLen(csds.csd[i].csd.oad.OI,csds.csd[i].csd.oad.attrindex);//多一个数据类型
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
				if(road_eve != NULL)//事件只有一个ROAD
				{
					if(csds.csd[i].csd.road.oads[j].OI == 0x201e || csds.csd[i].csd.road.oads[j].OI == 0x2020 ||//固定格式内存储，此处不再统计
							csds.csd[i].csd.road.oads[j].OI == 0x201a || csds.csd[i].csd.road.oads[j].OI == 0x2022)
						continue;
				}
				if(csds.csd[i].csd.road.oads[j].OI == 0xeeee)
					break;
				len_tmp = CalcOIDataLen(csds.csd[i].csd.road.oads[j].OI,csds.csd[i].csd.road.oads[j].attrindex);//多一个数据类型
				pindex += getOneUnit(&headbuf[pindex],csds.csd[i].csd.road.oad,csds.csd[i].csd.road.oads[j],len_tmp);
				*unitlen += len_tmp;
				(*unitnum)++;
			}
		}
	}
	*unitlen=freq*(fixlen+*unitlen);//一个单元存储TSA共用,在结构最前面，每个单元都有3个时标和数据，预留出合适大小，以能存下一个TSA所有数据
	asyslog(LOG_WARNING, "cjsave 存储文件头 fixlen=%d,*unitlen=%d,freq=%d",fixlen,*unitlen,freq);
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
int GetOADPos(FILE *fp,INT16U headlen,OAD oadm,OAD oadr)
{
	HEAD_UNIT headunit[100];
	int i=0,pos=0;
	INT8U headbuf[1024];
	INT16U unitnum;
	memset(headbuf,0x00,512);
	memset(headunit,0x00,100*sizeof(HEAD_UNIT));
	rewind(fp);

	fread(headbuf,headlen,1,fp);

	memcpy(headunit,&headbuf[4],headlen-4);

	unitnum = (headlen-4)/(sizeof(HEAD_UNIT));
	if(unitnum == 0)
		return -1;
	for(i=0;i<unitnum;i++)
	{
		if(memcmp(&headunit[i].oad_m,&oadm,sizeof(OAD)) == 0 &&
				memcmp(&headunit[i].oad_r,&oadr,sizeof(OAD)) == 0)
			return pos;
		pos += headunit[i].len;
	}
	return -1;
}

/*
 * 存储普通采集方案数据，
 * 数据格式：文件头结构：标注文件TSA和时标及csd格式，开始4个字节为文件头长度和每个数据单元长度 数据结构：存储各数据项值
 * 分测量点存储，一个TSA的全部数据放到一起，例如24个点的曲线数据，则按采集个数编号放入到一个位置,减少索引时间
 * 返回1，表示发生事件
 * ts_cc 要存储到哪一天
 */
int SaveNorData(INT8U taskid,ROAD *road_eve,INT8U *databuf,int datalen,TS ts_cc)//存储事件时指针road_eve定义为NULL
{
	FILE *fp;	CSD_ARRAYTYPE csds;
	char	fname[FILENAMELEN]={};
	char    cmdname[FILENAMELEN]={};
	INT8U *databuf_tmp=NULL,eveflg=0,taskinfoflg=0;
	int savepos=0,currpos=0,i=0;
	INT16U headlen=0,unitlen=0,unitnum=0,unitseq=0,runtime=0;//runtime执行次数
	TASKSET_INFO tasknor_info;
//	HEAD_UNIT *headunit = NULL;//文件头
	memset(&csds,0x00,sizeof(ROAD));
	//TS ts_cc;
	//TSGet(&ts_cc);
//	csds.num = 1;
//	csds.csd[0].type = 1;//road
	if(road_eve == NULL)//不是事件
	{
		fprintf(stderr,"SaveNorData==========\n");
		if((taskinfoflg = ReadTaskInfo(taskid,&tasknor_info))==0)
			return 0;
		runtime = tasknor_info.runtime;
		memcpy(&csds,&tasknor_info.csds,sizeof(CSD_ARRAYTYPE));//
		fprintf(stderr,"taskinfoflg=%d==========\n",taskinfoflg);
		if(taskinfoflg == 2)//月冻结
		{
			ts_cc.Day = 0;
			ts_cc.Hour = 0;
			ts_cc.Minute = 0;
			ts_cc.Sec = 0;
			asyslog(LOG_WARNING, "月冻结存储:%d",ts_cc.Month);
		}
		getTaskFileName(taskid,ts_cc,fname);
	}
	else
	{
		//1 初始化csds
		runtime = 1;//只存上一次 记录
		memcpy(&csds.csd[0].csd.road,road_eve,sizeof(ROAD));//
		csds.num = 1;
		getEveFileName(road_eve->oad.OI,fname);//创建eve文件
	}
//	asyslog(LOG_WARNING, "filename=%s",fname);
	fp = fopen(fname,"r");
	if(fp == NULL)//文件没内容 组文件头，如果文件已存在，提取文件头信息
	{
		asyslog(LOG_WARNING, "file：%s不存在",fname);
		CreateSaveHead(fname,road_eve,csds,&headlen,&unitlen,&unitnum,runtime,1);//写文件头信息并返回
		if(unitlen == 0)
		{
			asyslog(LOG_WARNING, "cjsave 存储文件头%s headlen=%d unitlen=%d unitnum=%d runtime=%d",fname,headlen,unitlen,unitnum,runtime);
			return 0;
		}
		asyslog(LOG_WARNING, "cjsave 存储文件头%s headlen=%d unitlen=%d unitnum=%d runtime=%d",fname,headlen,unitlen,unitnum,runtime);
		databuf_tmp = malloc(unitlen);
		savepos=0;
	}
	else
	{
//		asyslog(LOG_WARNING, "file：%s存在",fname);
		ReadFileHeadLen(fp,&headlen,&unitlen);
//		unitnum = GetTaskHead(fp,&headlen,&unitlen,&headunit);
//		for(i=0;i<unitnum;i++)
//		{

		if(unitlen==0)
		{
			asyslog(LOG_WARNING, "cjsave 存储文件头%s headlen=%d unitlen=%d unitnum=%d runtime=%d",fname,headlen,unitlen,unitnum,runtime);
			return 0;
		}
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
					if(memcmp(&databuf_tmp[18],&databuf[18],4)==0)//比对事件记录序号是否相同，相同事件未发生，不保存
					{
						if(fp != NULL)
							fclose(fp);
						if(databuf_tmp != NULL)
							free(databuf_tmp);
						fprintf(stderr,"\n--事件OI%02x未发生，不存!!\n",road_eve->oad.OI);
						return 0;//事件未发生，不存
					}
					else
					{
						//发送消息通知主动上报
						eveflg = 1;
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
		eveflg = 1;//第一次存储这个事件，需要主动上报
		if(currpos == 0)//第一个存储的
			savepos=headlen;
		else
			savepos=currpos;
		memset(databuf_tmp,0x00,unitlen);
		for(i=0;i<runtime;i++)
			memcpy(&databuf_tmp[unitlen*i/runtime],databuf,18);//每个小单元地址附上
	}
	unitseq = (ts_cc.Hour*60*60+ts_cc.Minute*60+ts_cc.Sec)/((24*60*60)/runtime)+1;
//	asyslog(LOG_NOTICE,"ts: %d:%d:%d",ts_cc.Hour,ts_cc.Minute,ts_cc.Sec);
	asyslog(LOG_NOTICE,"存储序号: unitseq=%d runtime=%d  %d--%d",unitseq,runtime,(ts_cc.Hour*60*60+ts_cc.Minute*60+ts_cc.Sec),((24*60*60)/runtime));
	if(unitseq > runtime || datalen != unitlen/runtime)
	{
		if(datalen != unitlen/runtime)//长度不对
		{
			sprintf(cmdname,"rm -rf %s",fname);
			system(cmdname);
			asyslog(LOG_NOTICE,"数据长度不对，删除文件%s,执行%s",fname,cmdname);
		}
		if(road_eve == NULL)
			asyslog(LOG_NOTICE,"数据长度不对，不存: datalen=%d,need=%d",datalen,unitlen/runtime);
		else
			asyslog(LOG_NOTICE,"事件长度不对，不存: datalen=%d,need=%d",datalen,unitlen/runtime);
//		return 0;//长度不对
		eveflg = 0;
	}
	else
	{
		memcpy(&databuf_tmp[unitlen*(unitseq-1)/runtime],databuf,datalen);
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
	return eveflg;
}
/*
 * 存储oad数据,给定数据，接口函数自己计算数据长度，不够字节自己补0
 * databuf前18个字节为55+地址，后面为数据
 */
int SaveOADData(INT8U taskid,OAD oad_m,OAD oad_r,INT8U *databuf,int datalen,TS ts_res)
{
		FILE *fp;
		CSD_ARRAYTYPE csds;
		char	fname[FILENAMELEN]={};
		INT8U *databuf_tmp=NULL,eveflg=0, firOIflg=0;
		int savepos=0, currpos=0, i=0, oadoffset=0, oadlen=0, taskinfoflg=0;
		INT16U headlen=0,unitlen=0,unitnum=0,unitseq=0,runtime=0;//runtime执行次数
		TASKSET_INFO tasknor_info;
		DateTimeBCD datetime;
		TS ts_now,ts_cc;
		ts_cc = ts_res;
		TSGet(&ts_now);
		memset(&csds,0x00,sizeof(ROAD));
	//	csds.num = 1;
	//	csds.csd[0].type = 1;//road

		fprintf(stderr,"SaveOADData==========\n");
		if((taskinfoflg = ReadTaskInfo(taskid,&tasknor_info))==0)
			return 0;
		fprintf(stderr,"\ntaskinfoflg=%d\n",taskinfoflg);
//		if(ReadTaskInfo(taskid,&tasknor_info)==0)
//			return 0;
		runtime = tasknor_info.runtime;
		memcpy(&csds,&tasknor_info.csds,sizeof(CSD_ARRAYTYPE));//
		if(taskinfoflg == 2)//月冻结
		{
			ts_cc.Day = 0;
			ts_cc.Hour = 0;
			ts_cc.Minute = 0;
			ts_cc.Sec = 0;
			asyslog(LOG_WARNING, "月冻结存储:%d",ts_cc.Month);
		}
		getTaskFileName(taskid,ts_cc,fname);

		ts_cc= ts_res;//重新赋值，减一天用于日冻结存储时标赋值
		tminc(&ts_cc,day_units,-1);

		fp = fopen(fname,"r");
		if(fp == NULL)//文件没内容 组文件头，如果文件已存在，提取文件头信息
		{
			CreateSaveHead(fname,NULL,csds,&headlen,&unitlen,&unitnum,runtime,1);//写文件头信息并返回
			asyslog(LOG_WARNING, "cjsave 存储文件头%s headlen=%d unitlen=%d unitnum=%d runtime=%d datalen=%d",fname,headlen,unitlen,unitnum,runtime,datalen);
			databuf_tmp = malloc(unitlen);
			savepos=0;
			fp = fopen(fname,"r");
			if(fp == NULL)
			{
				asyslog(LOG_WARNING, "file-%s is NULL",fname);
				return 0;
			}
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
				memcpy(&databuf_tmp[unitlen*i/runtime],databuf,18);//每个小单元地址附上
		}
		unitseq = (ts_res.Hour*60*60+ts_res.Minute*60+ts_res.Sec)/((24*60*60)/runtime)+1;
		asyslog(LOG_NOTICE,"ts: %d:%d:%d",ts_res.Hour,ts_res.Minute,ts_res.Sec);
		asyslog(LOG_NOTICE,"存储序号: unitseq=%d unitlen=%d runtime=%d  %d--%d",unitseq,unitlen,runtime,(ts_res.Hour*60*60+ts_res.Minute*60+ts_res.Sec),((24*60*60)/runtime));
		if(unitseq > runtime)
		{
			asyslog(LOG_NOTICE,"不符和unitseq=%d runtime=%d",unitseq, runtime);
			if(fp!=NULL)
				fclose(fp);
			if(databuf_tmp != NULL)
				free(databuf_tmp);
			return 0;//出错了，序列号超过了总长度
		}
		asyslog(LOG_NOTICE,"计算oadoffset");

		oadoffset = GetOADPos(fp,headlen,oad_m,oad_r);
		asyslog(LOG_NOTICE,"计算oadoffset=%d::%d ts: %d:%d:%d",oadoffset,ts_res.Hour,ts_res.Minute,ts_res.Sec,unitlen/runtime);
		if(oadoffset < 0 || oadoffset>=unitlen/runtime)//没找到//计算的有问题
		{
			if(fp!=NULL)
				fclose(fp);
			if(databuf_tmp != NULL)
				free(databuf_tmp);
			return 0;
		}

		asyslog(LOG_NOTICE,"oadoffset=%d合理::%d",oadoffset,unitlen*(unitseq-1)/runtime+oadoffset);
		memcpy(&databuf_tmp[unitlen*(unitseq-1)/runtime+oadoffset],&databuf[18],datalen-18);//赋值到应该赋值的oad位置

		memset(&datetime,0x00,sizeof(DateTimeBCD));
		asyslog(LOG_NOTICE,"文件里开始时标%d:%02x %02x%02x %02x %02x %02x %02x %02x"
				,sizeof(DateTimeBCD),
				databuf_tmp[18],databuf_tmp[19],databuf_tmp[20],databuf_tmp[21],
				databuf_tmp[22],databuf_tmp[23],databuf_tmp[24],databuf_tmp[25]);
		if(memcmp(&databuf_tmp[18],&datetime,sizeof(DateTimeBCD))==0)//开始时间为空
		{
			firOIflg = 1;
		}
		datetime.year.data = ((ts_now.Year&0xff00)>>8) + ((ts_now.Year&0x00ff)<<8);
		datetime.month.data = ts_now.Month;
		datetime.day.data = ts_now.Day;
		datetime.hour.data = ts_now.Hour;
		datetime.min.data = ts_now.Minute;
		datetime.sec.data = ts_now.Sec;
		asyslog(LOG_NOTICE,"当前赋值时标%d:%04x %02x %02x %02x %02x %02x"
				,sizeof(DateTimeBCD),
				datetime.year.data,datetime.month.data,datetime.day.data,datetime.hour.data,
				datetime.min.data,datetime.sec.data);
		if(firOIflg == 1)
		{
			databuf_tmp[unitlen*(unitseq-1)/runtime+18] = 0x1c;
			memcpy(&databuf_tmp[unitlen*(unitseq-1)/runtime+19],&datetime,7);//赋值抄表开始时间
		}
		databuf_tmp[unitlen*(unitseq-1)/runtime+18+8] = 0x1c;
		memcpy(&databuf_tmp[unitlen*(unitseq-1)/runtime+19+8],&datetime,7);//赋值抄表成功时间

		if(taskinfoflg == 2)//月冻结----------------------------------------//赋值抄表存储时间
		{
			fprintf(stderr,"\n月冻结\n");
			datetime.year.data = ((ts_res.Year&0xff00)>>8) + ((ts_res.Year&0x00ff)<<8);
			datetime.month.data = ts_res.Month;
			datetime.day.data = 0;
			datetime.hour.data = 0;
			datetime.min.data = 0;
			datetime.sec.data = 0;
		}
		else
			if(taskinfoflg == 1)//日冻结
			{
				fprintf(stderr,"\n日冻结tasknor_info.save_timetype)=%d\n",tasknor_info.save_timetype);
				switch(tasknor_info.save_timetype)
				{
				case 2://相对当日零点零分
					datetime.year.data = ((ts_res.Year&0xff00)>>8) + ((ts_res.Year&0x00ff)<<8);
					datetime.month.data = ts_res.Month;
					datetime.day.data = ts_res.Day;
					datetime.hour.data = 0;
					datetime.min.data = 0;
					datetime.sec.data = 0;
					break;
				case 3://相对上日33点59分
					datetime.year.data = ((ts_cc.Year&0xff00)>>8) + ((ts_cc.Year&0x00ff)<<8);
					datetime.month.data = ts_cc.Month;
					datetime.day.data = ts_cc.Day;
					datetime.hour.data = 23;
					datetime.min.data = 59;
					datetime.sec.data = 0;
					break;
				case 4://相对上日零点零分
					datetime.year.data = ((ts_cc.Year&0xff00)>>8) + ((ts_cc.Year&0x00ff)<<8);
					datetime.month.data = ts_cc.Month;
					datetime.day.data = ts_cc.Day;
					datetime.hour.data = 0;
					datetime.min.data = 0;
					datetime.sec.data = 0;
					break;
				default:break;
				}
			}
			else if(taskinfoflg == 4)//曲线
			{
				int jiange = 0;
				if(tasknor_info.taskfreq >= 60 && tasknor_info.taskfreq < 3600)//分钟冻结
				{
					jiange = tasknor_info.taskfreq/60;
					fprintf(stderr,"\n分钟冻结--jiange=%d,ts_res.Minute=%d\n",jiange,ts_res.Minute);
					datetime.min.data = (ts_res.Minute/jiange)*jiange;//算成整分钟数
					datetime.sec.data = 0;
				}
				else if(tasknor_info.taskfreq >= 3600)//小时冻结
				{
					fprintf(stderr,"\n小时冻结\n");
					jiange = tasknor_info.taskfreq/3600;
					datetime.hour.data = (ts_res.Hour/jiange)*jiange;//算成整小时数
					datetime.min.data = 0;
					datetime.sec.data = 0;
				}
			}

		asyslog(LOG_NOTICE,"当前赋值存储时标%d:%04x %02x %02x %02x %02x %02x"
				,sizeof(DateTimeBCD),
				datetime.year.data,datetime.month.data,datetime.day.data,datetime.hour.data,
				datetime.min.data,datetime.sec.data);
		databuf_tmp[unitlen*(unitseq-1)/runtime+18+16] = 0x1c;
		memcpy(&databuf_tmp[unitlen*(unitseq-1)/runtime+19+16],&datetime,7);//赋值抄表存储时间

		asyslog(LOG_NOTICE,"oadoffset=%d合理",oadoffset);
		oadlen = CalcOIDataLen(oad_r.OI,oad_r.attrindex);
		asyslog(LOG_NOTICE,"计算长度%04x-%02x-%02x:len=%d",oad_r.OI,oad_r.attflg,oad_r.attrindex,oadlen);
		if(datalen != oadlen+TSA_LEN+1)
		{
			asyslog(LOG_NOTICE,"计算的长度不对%d::%d",datalen, oadlen+TSA_LEN+1);
			if(fp!=NULL)
				fclose(fp);
			if(databuf_tmp != NULL)
				free(databuf_tmp);
			return 0;//长度不对
		}
		else
			datafile_write(fname, databuf_tmp, unitlen, savepos);
		if(fp!=NULL)
			fclose(fp);
		if(databuf_tmp != NULL)
			free(databuf_tmp);
		return eveflg;
}
INT8S get6035ByTaskID(INT16U taskID,CLASS_6035* class6035)
{
	memset(class6035,0,sizeof(CLASS_6035));
	class6035->taskID = taskID;
	CLASS_6035 tmp6035;
	memset(&tmp6035,0,sizeof(CLASS_6035));
	INT16U i;
	for(i=0;i<=255;i++)
	{
		if(readCoverClass(0x6035,i,&tmp6035,sizeof(CLASS_6035),coll_para_save)== 1)
		{
			if(tmp6035.taskID == taskID)
			{
				memcpy(class6035,&tmp6035,sizeof(CLASS_6035));
				return 1;
			}
		}
	}

#if 0
	//任务初始化新建6035
	CLASS_6035 result6035;	//采集任务监控单元
	memset(&result6035,0,sizeof(CLASS_6035));
	result6035.taskState = BEFORE_OPR;
	result6035.taskID = taskID;

	CLASS_6001	 meter={};
	int	blknum=0;
	blknum = getFileRecordNum(0x6000);
	int meterIndex = 0;
	for(meterIndex=0;meterIndex<blknum;meterIndex++)
	{
		if(readParaClass(oi,&meter,meterIndex)==1)
		{
			if(meter.sernum!=0 && meter.sernum!=0xffff)
			{
				if (checkMeterType(st6015.mst, meter.basicinfo.usrtype,meter.basicinfo.addr))
				{
					result6035.totalMSNum++;
				}

			}
		}
	}
#endif

	class6035->taskState = BEFORE_OPR;
	saveCoverClass(0x6035, class6035->taskID, class6035,sizeof(CLASS_6035), coll_para_save);
	return -1;
}



