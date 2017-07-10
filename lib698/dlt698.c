
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "AccessFun.h"
#include "EventObject.h"
#include "dlt698.h"
#include "dlt698def.h"
#include "secure.h"
#include "Esam.h"
#include "ParaDef.h"
#include "Shmem.h"
#include "PublicFunction.h"
#define LIB698_VER 	1
#define TESTDEF
extern int doObjectAction();
//extern int doActionReponse(int reponse,CSINFO *csinfo,PIID piid,OMD omd,int dar,INT8U *data,INT8U *buf);
extern int getRequestRecord(OAD oad,INT8U *data,CSINFO *csinfo,INT8U *sendbuf);
extern int getRequestRecordList(INT8U *data,CSINFO *csinfo,INT8U *sendbuf);
extern int getRequestNormal(OAD oad,INT8U *data,CSINFO *csinfo,INT8U *sendbuf);
extern int getRequestNormalList(INT8U *data,CSINFO *csinfo,INT8U *sendbuf);
extern int getRequestNext(INT8U *data,CSINFO *csinfo,INT8U *sendbuf);
extern int doReponse(int server,int reponse,CSINFO *csinfo,int datalen,INT8U *data,INT8U *buf);
extern INT16U setRequestNormal(INT8U *data,OAD oad,INT8U *DAR,CSINFO *csinfo,INT8U *buf);
extern int setRequestNormalList(INT8U *Object,CSINFO *csinfo,INT8U *buf);
extern int setThenGetRequestNormalList(INT8U *data,CSINFO *csinfo,INT8U *buf);
extern int Proxy_GetRequestlist(INT8U *data,CSINFO *csinfo,INT8U *sendbuf,INT8U piid);
extern int Proxy_GetRequestRecord(INT8U *data,CSINFO *csinfo,INT8U *sendbuf,INT8U piid);
extern int Proxy_TransCommandRequest(INT8U *data,CSINFO *csinfo,INT8U *sendbuf,INT8U piid);
extern unsigned short tryfcs16(unsigned char *cp, int  len);
extern INT32S secureConnectRequest(SignatureSecurity* securityInfo ,SecurityData* RetInfo);
INT8S (*pSendfun)(int fd,INT8U* sndbuf,INT16U sndlen);
extern void  Get698_event(OAD oad,ProgramInfo* prginfo_event);
int comfd = 0;
INT8U ClientPiid=0;
INT8U TmpDataBuf[MAXSIZ_FAM];
INT8U TmpDataBufList[MAXSIZ_FAM*2];
ProgramInfo *memp;
LINK_Response *linkResponse_p;	// 预连接（登录、心跳）的确认，存储在com控制块中
CONNECT_Response *myAppVar_p;	// 集中器支持参数（应用层会话参数）
CONNECT_Response *AppVar_p;		// 集中器协商后参数（应用层会话参数）
INT8U securetype;  //安全等级类型  01明文，02明文+MAC 03密文  04密文+MAC
INT8U secureRN[20];//安全认证随机数，主站下发，终端回复时需用到，esam计算使用
static INT8U	client_addr=0;
PIID piid_g={};
TimeTag		Response_timetag;		//响应的时间标签值
INT8U broadcast=0;
/**************************************
 * 函数功能：DL/T698.45 状态机
 * 参数含义：
 **************************************/
int StateProcess(CommBlock* nst, int delay_num)
{
	int length=0,i=0;
	int ret = 1;
	switch(nst->deal_step)
	{
		case 0:		 // 找到第一个 0x68
			while(nst->RHead!=nst->RTail)
			{
				if (nst->RecBuf[nst->RTail]== 0x68)
				{
					nst->deal_step = 1;
					fprintf(stderr,"\n找到 68");
					ret = 0;//需要继续
					break;
				}else {
					nst->RTail = (nst->RTail + 1)% FRAMELEN;
				}
			}
			break;
		case 1:	   //从rev_tail2开始 跨长度字节找 0x16
			if(((nst->RHead-nst->RTail+FRAMELEN)%FRAMELEN) >= 3)
			{
//				fprintf(stderr,"\nNetRevBuf[*rev_tail] = %02x  %02x\n ",NetRevBuf[(*rev_tail+ 2)%FRAMELEN],NetRevBuf[(*rev_tail+ 1)%FRAMELEN]);
//				fprintf(stderr,"\nRevBuf[%d]=%02x  %02x  %02x",
//						nst->RTail,
//						nst->RecBuf[(nst->RTail+ 1 + FRAMELEN )%FRAMELEN],
//						nst->RecBuf[(nst->RTail+ 2 + FRAMELEN )%FRAMELEN]);

				length = (nst->RecBuf[(nst->RTail+ 2 + FRAMELEN )%FRAMELEN] << 8) + nst->RecBuf[(nst->RTail+ 1 + FRAMELEN)%FRAMELEN];
				fprintf(stderr,"\n---length =%d  ",length);

				length = (length) & 0x03FFF;//bit15 bit14 保留
				fprintf(stderr,"\nlength = %d \n",length);
				if (length <= 0)//长度异常
				{
					nst->RTail = (nst->RTail + 1)% FRAMELEN;
					nst->deal_step = 0;
					ret = 0;//需要继续
					break;
				}else {
//					fprintf(stderr,"*rev_head = %d *rev_tail = %d ",*rev_head,*rev_tail);
					if((nst->RHead-nst->RTail+FRAMELEN)%FRAMELEN >= (length+2))//长度length为除 68和16 以外的字节数
					{
						fprintf(stderr,"\n找16");
						if(nst->RecBuf[ (nst->RTail + length + 1 + FRAMELEN)% FRAMELEN ]== 0x16)
						{
							fprintf(stderr,"\n找到16!!!!!!!");
							nst->rev_delay = 0;
//							fprintf(stderr,"RRR=[%d]\n",length+2);
							for(i=0;i<(length+2);i++)
							{
								nst->DealBuf[i] = nst->RecBuf[nst->RTail];
//								fprintf(stderr,"%02x ",dealbuf[i]);
								nst->RTail = (nst->RTail + 1) % FRAMELEN;
							}
							nst->deal_step = 0;//进入下一步
							return (length+2);
						}
						else
						{
							fprintf(stderr,"\n1、 delay =%d  ",nst->rev_delay);
							if (nst->rev_delay < delay_num)
							{
								(nst->rev_delay)++;
								ret = 0;//需要继续
								break;
							}else
							{
								fprintf(stderr,"\n1、超时  Tail 移动 ！！");
								nst->rev_delay = 0;
								nst->RTail = (nst->RTail +1 )% FRAMELEN;
								nst->deal_step = 0;//返回第一步
								ret = 0;//需要继续
							}
						}
					}else
					{
							fprintf(stderr,"\n2、 delay =%d  ",nst->rev_delay);
							if (nst->rev_delay < delay_num)
							{
								(nst->rev_delay)++;
								ret = 0;//需要继续
								break;
							}else
							{
								fprintf(stderr,"\n2、超时  Tail 移动 ！！");
								nst->rev_delay = 0;
								nst->RTail = (nst->RTail +1 )% FRAMELEN;
								nst->deal_step = 0;
								ret = 0;//需要继续
							}
					}
				}
			}
			break;
		default :
			break;
	}
	return ret;
}
/**********************************************************
 * 检测下行帧服务器地址是否与本终端匹配
 */
int CheckSerAddr(unsigned char* buf ,INT8U *addr)
{
	INT8U checkByte=0;
	INT8U Check_Hb=0,Check_Lb=0,My_Hb=0,My_Lb=0;
	INT8U sa_length=0,mycslen=0;
	INT8U logicID=0;
	INT8U tmp[OCTET_STRING_LEN];
	int i=0;
	INT8U cstype = 0;
	if(buf[0]==0x68 )
	{
		sa_length 	= (buf[4]& 0x0f) + 1; 		/*服务器地址长度 0,1,，，15 表示 1,2,，，16*/
		logicID = (buf[4]& 0x30)>>4;			/*逻辑地址*/
		cstype = (buf[4]& 0xc0) >> 6;
		mycslen = addr[0];
		if(mycslen > OCTET_STRING_LEN)
			mycslen = OCTET_STRING_LEN;

		fprintf(stderr,"\n下行报文逻辑地址 %d",logicID);\
		if (logicID!=0 && logicID!=1)
		{
			fprintf(stderr,"\n逻辑地址非0并且非1 ");
			return 0;
		}
		fprintf(stderr,"\n本终端地址(%d字节): ",mycslen);
		for(i=0;i<mycslen;i++)
		{
			tmp[i] = addr[mycslen-i];
			fprintf(stderr,"%02x ",tmp[i]);
		}

		fprintf(stderr,"\n本帧 地址(%d字节)： ",sa_length);
		for(i=0;i<sa_length;i++)
			fprintf(stderr,"%02x ",buf[5+i]);

		switch (cstype)
		{
			case 0://单地址类型，需要判断是否与本服务器地址匹配
				fprintf(stderr,"\n单地址，地址字节数 %d",sa_length);
				if (mycslen!=sa_length)
				{
					fprintf(stderr,"\n单地址，长度不符合 ");
					return 0;
				}else
				{
					for(i=0;i<mycslen;i++)
					{
						fprintf(stderr,"\n本终端 addr[%d]=%02x buf[%d]=%02x",i,tmp[i],5+i,buf[5+i]);
						if (tmp[i]!=buf[5+i])
						{
							fprintf(stderr,"\n单地址招测报文与本终端不符合!!");
							return 0;
						}
					}
				}
				break;
			case 1:
				fprintf(stderr,"\n通配地址，地址字节数 %d",sa_length);
				for(i=0;i<mycslen;i++)
				{
					fprintf(stderr,"\n本终端 addr[%d]=%02x buf[%d]=%02x",i,tmp[i],5+i,buf[5+i]);
					if (tmp[i]!= buf[5+i])
					{
						Check_Hb = buf[5+i] & 0xF0;
						Check_Lb = buf[5+i] & 0x0F;
						My_Hb = tmp[i] & 0xF0;
						My_Lb = tmp[i] & 0x0F;
						if (Check_Hb != 0xA0  &&  Check_Hb != My_Hb)//低4位不是通配符  而且还不相等
						{
							fprintf(stderr,"\n不符合");
							return 0;
						}
						if (Check_Lb != 0x0A  &&  Check_Lb != My_Lb)//低4位不是通配符  而且还不相等
						{
							fprintf(stderr,"\n不符合");
							return 0;
						}
						continue;
					}
				}
				break;
			case 2:
				fprintf(stderr,"\n组地址，地址字节数 %d",sa_length);
				break;
			case 3:
				fprintf(stderr,"\n广播地址，地址字节数 %d",sa_length);
				break;
		}
	}
	return 1;
}
/**********************************************************
 * 处理帧头
 * 	获取：1、帧长度       2、控制码      3、服务器地址    4、客户机地址
 * 	校验帧头部分
 */
int CheckHead(unsigned char* buf ,CSINFO *csinfo)
{
	//TODO
	//添加逻辑地址的比对, 如果不是针对本集中器的报文, 则舍弃
	INT8U sa_length=0;
	INT16U	cs16=0;//程序计算的校验码
	INT16U	fcs16=0;//从帧中取出的校验码
	ctlUN ctl;
	lengthUN frameLen;

	ctl.u8b = 0;
	frameLen.u16b = 0;
	if(buf[0]==0x68  && csinfo!=NULL)
	{
		memcpy(&frameLen, &buf[1], 2);
		memcpy(&ctl, &buf[3], 1);
		sa_length 	= (buf[4]& 0x0f) + 1; 		/*服务器地址长度 0,1,，，15 表示 1,2,，，16*/
		cs16 = tryfcs16(&buf[1], sa_length + 5);//小端存储 低位在左 高位在右

		memcpy(&fcs16, &buf[5+ sa_length + 1], 2);//小端机器用法, 不可移植到大端机器
		if(fcs16 != cs16) {
			fprintf(stderr, "\n帧头校验错误!\n");
			return 0;
		}

		DEBUG_TIME_LINE("\nframe length: %d", frameLen.length.len);
		DEBUG_TIME_LINE("direction: %s", ctl.ctl.dir?"send by server":"send by client");
		DEBUG_TIME_LINE("prm: %s", ctl.ctl.prm?"send by client":"send by server");
		DEBUG_TIME_LINE("divS: %s", ctl.ctl.divS?"part of APDU":"Whole APDU");
		switch (ctl.ctl.func) {
		case 1:
			fprintf(stderr,"登录, 心跳, 退出登录\n");
			break;
		case 3:
			fprintf(stderr,"应用连接管理及数据交换服务\n");
			break;
		default:
			fprintf(stderr,"功能码未定义\n");
			break;
		}
		csinfo->frame_length = frameLen.length.len;//帧长度
		csinfo->funcode		= ctl.ctl.func;//功能码
		csinfo->dir			= ctl.ctl.dir;
		csinfo->prm			= ctl.ctl.prm;
		csinfo->gframeflg	= ctl.ctl.divS;
		csinfo->sa_type		= (buf[4]& 0xc0) >> 6;	/*0:单地址   1：通配地址   2：组地址   3：广播地址*/
		memcpy(csinfo->sa, &buf[5], sa_length);		/*服务器地址*/
		csinfo->ca			= buf[5+ sa_length]; 			/*客户机地址*/
		csinfo->sa_length	= sa_length;
		fprintf(stderr,"\n地址类型 %d",csinfo->sa_type);
		return 1;
	}
	return 0;
}
/******************************************************
 * 校验帧头部分
 * length:需要校验的字节长度  = 帧长度 - 起始符和结束符两个字节
 ******************************************************/
int CheckTail(unsigned char * buf,INT16U length)
{
	INT16U cs16=0;
	INT16U fcs16=0;

	if( buf[0]==0x68 ) {
		cs16 = tryfcs16(&buf[1], length-2);
		memcpy(&fcs16, &buf[length - 1], 2);
		if (cs16 == fcs16) {
			return 1;
		} else {
			fprintf(stderr,"\n帧尾校验错误!");
		}
	}
	return 0;
}
INT8U CtrlWord(CSINFO* csinfo)
{
	ctlUN ctl;

	fprintf(stderr,"csinfo.dir=%d,prm=%d",csinfo->dir,csinfo->prm);
	if(csinfo == NULL)
		return 0;

	ctl.u8b = 0;
	ctl.ctl.dir = csinfo->dir;//如果接收报文来自客户机, 则应给出服务器的应答
	ctl.ctl.prm = csinfo->prm;//直接使用接收报文的启动标志
	ctl.ctl.divS = csinfo->gframeflg;
	ctl.ctl.func = csinfo->funcode;
	fprintf(stderr,"ctl.u8b=%02x",ctl.u8b);
	return ctl.u8b;
}
void FrameTail(INT8U *buf,int index,int hcsi)
{
	INT8U b1=0,b2=0;
	INT16U cs16=0;
	INT16U length = (index+1) & 0x3fff;

	memcpy(&buf[1],&length,2);

	cs16 = tryfcs16(&buf[1], hcsi-1);
	b1 = (cs16 & 0x00ff);
	b2 = ((cs16 >> 8) & 0x00ff);
	buf[hcsi] =b1;
	buf[hcsi+1]	=b2;

	cs16 = tryfcs16(&buf[1], index-1);
	b1 = (cs16 & 0x00ff);
	b2 = ((cs16 >> 8) & 0x00ff);
	buf[index] = b1;
	buf[index+1] = b2;
	buf[index+2] = 0x16;
	return;
}

int broadServerAddr(INT8U *sa,int sa_len)
{
	int i=0;

	if(sa == NULL)
		return 0;

	for(i=0;i<sa_len;i++)
		if(sa[i]!=0xAA)
			return 0;

	return 1;
}

int FrameHead(CSINFO *csinfo,INT8U *buf)
{
	CLASS_4001_4002_4003 sa = {};
	int i=0,j=0;
	saUN saTypeLen;

	buf[i++]= 0x68;//起始码
	buf[i++]= 0;	//长度
	buf[i++]= 0;
	buf[i++]= CtrlWord(csinfo);
	buf[i++]= (csinfo->sa_type<<6) | (0<<4) | ((csinfo->sa_length-1) & 0xf);

	//集中器与浙江汉普台体测试，台体下发广播地址,应答终端的通信地址上报，
	//其他情况如电表下发广播对时命令时，是不能进行修改服务器端地址的
	//只有客户机启动的下行广播报文才响应
	if(broadServerAddr(csinfo->sa,csinfo->sa_length)==1 &&\
			csinfo->dir==0 &&\
			csinfo->prm==1) {
		memset(&sa, 0, sizeof(CLASS_4001_4002_4003));
		readCoverClass(0x4001, 0, &sa, sizeof(CLASS_4001_4002_4003), para_vari_save);
		saTypeLen.u8b = 0;//初始化saTypeLen各位域为0
		saTypeLen.sa.saType = 0;//应该应答单地址
		saTypeLen.sa.logicAddr = 0;//逻辑地址
		saTypeLen.sa.saLen = sa.curstom_num[0]-1;//终端地址长度
		buf[i-1] = saTypeLen.u8b;//重新设置终端地址的长度字
		for(j=0;j<sa.curstom_num[0];j++) {
			buf[i++] = sa.curstom_num[sa.curstom_num[0]-j];
		}
	}else {
		memcpy(&buf[i],csinfo->sa,csinfo->sa_length);
		i = i + csinfo->sa_length;
	}

	buf[i++]=csinfo->ca;
	return i;
}

int FrameTimeTag(TimeTag *tag,INT8U *buf)
{
	int	i=0;
	buf[i++] = tag->flag;
	if(tag->flag==1) {		//时间标签有效
		buf[i++] = (tag->sendTimeTag.year.data >> 8) & 0xff;
		buf[i++] = tag->sendTimeTag.year.data & 0xff;
		buf[i++] = tag->sendTimeTag.month.data & 0xff;
		buf[i++] = tag->sendTimeTag.day.data & 0xff;
		buf[i++] = tag->sendTimeTag.hour.data & 0xff;
		buf[i++] = tag->sendTimeTag.min.data & 0xff;
		buf[i++] = tag->sendTimeTag.sec.data & 0xff;
		buf[i++] = tag->ti.units;
		buf[i++] = (tag->ti.interval >> 8) & 0xff;
		buf[i++] = tag->ti.interval & 0xff;
	}
	return i;
}
/**********************************************************************
 *	服务器向远方客户机提出
 *	1.登录
 *	2.心跳
 *	3.退出登录
 */
int Link_Request(LINK_Request request,INT8U *addr,INT8U *buf)
{
	int index=0, hcsi=0,i=0;
	CSINFO csinfo={};
	csinfo.dir = 1;		//服务器发出
	csinfo.prm = 0; 	//服务器发出
	csinfo.funcode = 1; //链路管理
	csinfo.sa_type = 0 ;//单地址
	csinfo.sa_length = addr[0];//sizeof(addr)-1;//服务器地址长度

	//服务器地址
	fprintf(stderr,"sa_length = %d \n",csinfo.sa_length);
	if(csinfo.sa_length<OCTET_STRING_LEN) {
		for(i=0;i<csinfo.sa_length;i++) {
			csinfo.sa[i] = addr[csinfo.sa_length-i];
		}
	}else {
		fprintf(stderr,"SA 长度超过定义长度，不合理！！！\n");
	}
	//客户端地址
	csinfo.ca = client_addr;

	index = FrameHead(&csinfo,buf) ; //	2：hcs  hcs
	hcsi = index;
	index = index + 2;
//	fprintf(stderr,"\n link request type i=%d",index);
	buf[index++] = 1;//LINK_Request

	memcpy(&buf[index],&request,sizeof(LINK_Request));
	index = index + sizeof(LINK_Request);
//	fprintf(stderr,"\n add link request  i=%d",index);
//	fprintf(stderr,"\n  LINK_Request = %d",sizeof(LINK_Request));
	FrameTail(buf,index,hcsi);
	return (index + 3);			//3: cs cs 16
}

int Link_Response(INT8U *apdu)
{
	LINK_Response linkresponse={};
	memcpy(&linkresponse,&apdu[1],sizeof(LINK_Response));
	linkresponse.request_time.year = ((linkresponse.request_time.year<<8)&0xff00) | ((linkresponse.request_time.year>>8)&0x00ff);
	linkresponse.response_time.year = ((linkresponse.response_time.year<<8)&0xff00) | ((linkresponse.response_time.year>>8)&0x00ff);
	linkresponse.reached_time.year = ((linkresponse.reached_time.year<<8)&0xff00) | ((linkresponse.reached_time.year>>8)&0x00ff);
	fprintf(stderr,"\nLinkResponse: \nPIID = %02x \nResult = %02x ",linkresponse.piid.data,linkresponse.result);
	fprintf(stderr,"\nrequest time: %d-%d-%d %d:%d:%d",linkresponse.request_time.year,linkresponse.request_time.month,linkresponse.request_time.day_of_month,linkresponse.request_time.hour,linkresponse.request_time.minute,linkresponse.request_time.second);
	fprintf(stderr,"\nresponse time: %d-%d-%d  %d:%d:%d",linkresponse.response_time.year,linkresponse.response_time.month,linkresponse.response_time.day_of_month,linkresponse.response_time.hour,linkresponse.response_time.minute,linkresponse.response_time.second);
	fprintf(stderr,"\nreached time: %d-%d-%d  %d:%d:%d",linkresponse.reached_time.year,linkresponse.reached_time.month,linkresponse.reached_time.day_of_month,linkresponse.reached_time.hour,linkresponse.reached_time.minute,linkresponse.reached_time.second);
	memcpy(linkResponse_p,&linkresponse,sizeof(LINK_Response));
	return 1;
}
/**********************************************************************
 * 1.	LINK.response 服务,本服务由客户机应用进程调用,用于向服务器应用进程响应预连接请求。
 * 					  主站（客户机）应答集中器（服务器）预连接请求
 */
int dealClientResponse(INT8U *apdu,CSINFO *csinfo)
{

	INT8U apduType = 0;

	apduType = apdu[0];

	switch(apduType)
	{
		case LINK_RESPONSE:
			client_addr = csinfo->ca;		//预连接后，获取客户端地址
			Link_Response( apdu );//预连接响应
			break;
		case REPORT_RESPONSE:
			break;
	}
	return apduType;
}
int long_unsigned(INT8U *value,INT8U *buf)
{
	value[0]= buf[1];
	value[1]= buf[0];
	return 2;
}

void GetconnetRequest(CONNECT_Request *request,INT8U *apdu)
{
	int index=0, bytenum=0;
	request->piid.data = apdu[index++];
	memcpy((INT8U*)&request->expect_app_ver,&apdu[index],2);
	index = index + 2;
	memcpy((INT8U*)&request->ProtocolConformance,&apdu[index],sizeof(request->ProtocolConformance));
	index += sizeof(request->ProtocolConformance);
	memcpy((INT8U*)&request->FunctionConformance,&apdu[index],sizeof(request->FunctionConformance));
	index += sizeof(request->FunctionConformance);
	index += long_unsigned((INT8U *)&request->client_send_size,&apdu[index]);
	index += long_unsigned((INT8U *)&request->client_recv_size,&apdu[index]);
	request->client_recv_maxWindow = apdu[index++];
	index += long_unsigned((INT8U *)&request->client_deal_maxApdu,&apdu[index]);
	request->expect_connect_timeout = (apdu[index]<<24) + (apdu[index+1]<<16) + (apdu[index+2]<<8) + (apdu[index+3]);
	index = index + 4;
	request->connecttype = apdu[index++];
	switch(request->connecttype )
	{
		case 0://公共连接
			break;
		case 1://一般密码
			break;
		case 2://对称加密
			break;
		default://数字签名
			bytenum = apdu[index];
			if (bytenum<=40)
			{
				memcpy((INT8U*)&request->info.sigsecur.encrypted_code2,&apdu[index],bytenum+1);//长度拷贝到目的缓冲区第0个字节
				index = index + bytenum +1;
				bytenum = apdu[index];
				if (bytenum<70)
					memcpy((INT8U*)&request->info.sigsecur.signature,&apdu[index],bytenum+1);
			}
			break;
	}
//	fprintf(stderr,"\nPIID=%02x",request->piid.data);
//	fprintf(stderr,"\n期望的应用缯协议版本 = %x",request->expect_app_ver.data);
//	fprintf(stderr,"\n期望的协议一致性块 =");
//	for(index=0;index<8;index++)
//		fprintf(stderr,"%02x ",request->ProtocolConformance[index]);
//	fprintf(stderr,"\n期望的功能一致性块 =");
//	for(index=0;index<16;index++)
//		fprintf(stderr,"%02x ",request->FunctionConformance[index]);
//
//	fprintf(stderr,"\n客户机发送最大字节 %d",request->client_send_size);
//	fprintf(stderr,"\n客户机接收最大字节 %d",request->client_recv_size);
//	fprintf(stderr,"\n客户机最大窗口 %d",request->client_recv_maxWindow);
//	fprintf(stderr,"\n客户机最大可处理APDU %d",request->client_deal_maxApdu);
//	fprintf(stderr,"\n期望应用连接超时时间 %d",request->expect_connect_timeout);
//	fprintf(stderr,"\n应用连接类型=%d",request->connecttype);
//	fprintf(stderr,"\n密文2");
//	for(index=0;index<request->info.sigsecur.encrypted_code2[0];index++)
//	{
//		if (index %10==0) fprintf(stderr,"\n");
//		fprintf(stderr,"%02x ",request->info.sigsecur.encrypted_code2[index+1]);
//	}
//	fprintf(stderr,"\n客户机签名2");
//	for(index=0;index<request->info.sigsecur.signature[0];index++)
//	{
//		if (index %10==0) fprintf(stderr,"\n");
//		fprintf(stderr,"%02x ",request->info.sigsecur.signature[index+1]);
//	}
}
void varconsult(CONNECT_Response *response ,CONNECT_Request *request,CONNECT_Response *myvar)
{
	int i =0;

	//具体参照宣贯资料P10
	//商定的应用层协议版本号: 为服务器支持的协议版本号
	memcpy(&response->server_factory_version, &myvar->server_factory_version,sizeof(FactoryVersion));
	//商定的协议一致性块：客户机请求的协议一致性块与服务器支持的协议一致性块按位与的结果
	for(i=0;i<8;i++)
		response->ProtocolConformance[i] = myvar->ProtocolConformance[i] & request->ProtocolConformance[i];
	//商定的功能一致性块：客户机请求的功能一致性块与服务器支持的功能一致性块按位与的结果
	for(i=0;i<16;i++)
		response->FunctionConformance[i] = myvar->FunctionConformance[i] & request->FunctionConformance[i];
	//商定的应用层协议版本号:为服务器支持的协议版本号
	memcpy(&response->app_version,&request->expect_app_ver,sizeof(request->expect_app_ver));

	//服务器发送帧最大尺寸：取服务器支持发送帧最大尺寸与客户机请求接收最大尺寸的小值
	if (myvar->server_send_size < request->client_recv_size)
		response->server_send_size = myvar->server_send_size;
	else
		response->server_send_size = request->client_recv_size;

	//服务器接收帧最大尺寸:取服务器支持的接收帧最大尺寸与客户机请求发送帧最大尺寸的小值
	if (myvar->server_recv_size < request->client_send_size)
		response->server_recv_size = myvar->server_recv_size;
	else
		response->server_recv_size = request->client_send_size;

	//服务器接收帧最大窗口尺寸:为服务器支持的最大窗口尺寸
	//if (myvar->server_recv_maxWindow < request->client_recv_maxWindow)
		response->server_recv_maxWindow = myvar->server_recv_maxWindow;
	//else
	//	response->server_recv_maxWindow = request->client_recv_maxWindow;

	//服务器最大可处理APDU尺寸:服务器支持的最大可处理apdu尺寸
	//if (myvar->server_deal_maxApdu < request->client_deal_maxApdu)//
		response->server_deal_maxApdu = myvar->server_deal_maxApdu;
	//else
	//	response->server_deal_maxApdu = request->client_deal_maxApdu;

	//商定的应用连接超时时间：取客户机请求应用连接超时时间和服务器支持的最大超时时间两者的小值
	if(myvar->expect_connect_timeout < request->expect_connect_timeout)
		response->expect_connect_timeout = myvar->expect_connect_timeout;
	else
		response->expect_connect_timeout = request->expect_connect_timeout;

	response->info.result = allow;
//	response->info.addinfo  //在发送前从 ESAM取值
}

int appConnectResponse(INT8U *apdu,CSINFO *csinfo,INT8U *buf)
{
	int index=1, hcsi=0,bytenum=0;
	CONNECT_Request request={};
	CONNECT_Response response={};

	/*
	 * 解析主站应用连接请求的参数，在request结构体
	 */
	GetconnetRequest(&request,&apdu[1]);

	/* test:
	 * 临时将应用会话参数 附值成下发的 主站会话参数，测试
	 */

	//---------------------------------------------------------------------------------
	/*
	 * 协商过程：
	 * 把主站下发的会话参数 与 集中器本身的会话参数进行比对，选择小的，应答给主站
	 * response:协商后的结果（应答给主站）
	 * 		request :主站协商请求的内容
	 * 		myAppVar_p :集中器本身的参数
	 */
	memset(&response,0,sizeof(response));
	response.piid_acd = request.piid;
	varconsult(&response,&request,myAppVar_p);
	/*
	 *存储应用会话参数结构
	 */
	memcpy((INT8U *)AppVar_p,&response,sizeof(response));

	/*
	 * 根据 response 组织响应报文
	 */
	//return 1;
	csinfo->dir = 1;
	csinfo->prm = 1;
	index = FrameHead(csinfo,buf);
	hcsi = index;
	index = index + 2;
	buf[index++] = CONNECT_RESPONSE;
	buf[index++] = response.piid_acd.data;
	memcpy(&buf[index],response.server_factory_version.factorycode,4);
	index = index +4;
	memcpy(&buf[index],response.server_factory_version.software_ver,4);
	index = index +4;
	memcpy(&buf[index],response.server_factory_version.software_date,6);
	index = index +6;
	memcpy(&buf[index],response.server_factory_version.hardware_ver,4);
	index = index +4;
	memcpy(&buf[index],response.server_factory_version.hardware_date,6);
	index = index +6;
	memcpy(&buf[index],response.server_factory_version.additioninfo,8);
		index = index +8;
	memcpy(&buf[index],&response.app_version,2);
	index = index +2;
	memcpy(&buf[index],response.ProtocolConformance,8);
	index = index +8;
	memcpy(&buf[index],response.FunctionConformance,16);
	index = index +16;
	buf[index++] = (response.server_send_size & 0xFF00)>>8;
	buf[index++] = response.server_send_size & 0x00FF;
	buf[index++] = (response.server_recv_size & 0xFF00)>>8;
	buf[index++] = response.server_recv_size & 0x00FF;
	buf[index++] = response.server_recv_maxWindow;
	buf[index++] = (response.server_deal_maxApdu & 0xFF00) >>8;
	buf[index++] = response.server_deal_maxApdu &0x00FF;
	buf[index++] = (response.expect_connect_timeout & 0xFF000000) >> 24 ;
	buf[index++] = (response.expect_connect_timeout & 0x00FF0000) >> 16 ;
	buf[index++] = (response.expect_connect_timeout & 0x0000FF00) >> 8 ;
	buf[index++] =  response.expect_connect_timeout & 0x000000FF;


	INT32S ret = 0;
	if (request.connecttype == 3)
	{
		ret = secureConnectRequest(&request.info.sigsecur,&response.info.addinfo);
		if( ret > 0 )
		{
			buf[index++] = response.info.result;
			buf[index++] =0x01;
			bytenum = response.info.addinfo.server_rn[0] +1;
			memcpy(&buf[index],response.info.addinfo.server_rn,bytenum);
			index = index + bytenum;
			bytenum = response.info.addinfo.server_signInfo[0]+1;
			memcpy(&buf[index],response.info.addinfo.server_signInfo,bytenum);
			index = index + bytenum;
			buf[index++] =0;
			buf[index++] =0;
		}else
		{
			buf[index++] = 4;
			buf[index++] = 0;
		}
	}else
	{
		buf[index++] = 0;
		buf[index++] = 0;
	}

	FrameTail(buf,index,hcsi);

	if(pSendfun!=NULL && csinfo->sa_type!=2 && csinfo->sa_type!=3)//组地址或广播地址不需要应答
		pSendfun(comfd,buf,index+3);
	return (index+3);
}

int doSetAttribute(INT8U *apdu,CSINFO *csinfo,INT8U *buf)
{
	INT8U  DAR=success;
	int	 index=0;
	INT8U setType = apdu[1];
	OAD oad={};
	INT8U *data=NULL;
	piid_g.data = apdu[2];
	if (csinfo->sa_type == 2 || csinfo->sa_type == 3)
	{
		fprintf(stderr,"\n组地址或广播地址，不响应设置服务");
		return 0;
	}
	switch(setType)
	{
		case SET_REQUEST_NORMAL:
			memset(TmpDataBuf,0,sizeof(TmpDataBuf));
			getOAD(0,&apdu[3],&oad,NULL);
			data = &apdu[7];					//Data
			setRequestNormal(data,oad,&DAR,NULL,buf);
			fprintf(stderr,"setRequestNormal dar = %d\n",DAR);
			index += create_OAD(0,&TmpDataBuf[index],oad);
			TmpDataBuf[index++] = (INT8U)DAR;
			doReponse(SET_RESPONSE,SET_REQUEST_NORMAL,csinfo,index,TmpDataBuf,buf);
			if(DAR==0) {	//sucess
				Get698_event(oad,memp);
			}
			break;
		case SET_REQUEST_NORMAL_LIST:
			setRequestNormalList(&apdu[3],csinfo,buf);
			break;
		case SET_THENGET_REQUEST_NORMAL_LIST:
			setThenGetRequestNormalList(&apdu[3],csinfo,buf);
			break;
	}

	return 1;
}

int doGetAttribute(INT8U *apdu,CSINFO *csinfo,INT8U *sendbuf)
{
//	PIID piid={};
	INT8U getType = apdu[1];
	OAD oad={};
	INT8U *data=NULL;
	piid_g.data = apdu[2];
	fprintf(stderr,"\n- get type = %d PIID=%02x",getType,piid_g.data);

	switch(getType)
	{
		case GET_REQUEST_NORMAL:
			getOAD(0,&apdu[3],&oad,NULL);
			data = &apdu[7];					//*重新定位数据指针地址*/
			getRequestNormal(oad,data,csinfo,sendbuf);
			break;
		case GET_REQUEST_NORMAL_LIST:
			data = &apdu[3];
			getRequestNormalList(data,csinfo,sendbuf);
			break;
		case GET_REQUEST_RECORD:
			getOAD(0,&apdu[3],&oad,NULL);
			data = &apdu[7];
			getRequestRecord(oad,data,csinfo,sendbuf);
			break;
		case GET_REQUEST_RECORD_LIST:
			data = &apdu[3];
			getRequestRecordList(data,csinfo,sendbuf);
			break;
		case GET_REQUEST_RECORD_NEXT:
			data = &apdu[3];
			getRequestNext(data,csinfo,sendbuf);
			break;

	}
	return 1;
}

int doProxyRequest(INT8U *apdu,CSINFO *csinfo,INT8U *sendbuf)
{
	PIID piid={};
	INT8U getType = apdu[1];
	INT8U *data=NULL;

	piid_g.data = apdu[2];
	data = &apdu[3];
	fprintf(stderr,"\n代理 PIID %02x   ",piid.data);

	switch(getType)
	{
		case ProxyGetRequestList:
			fprintf(stderr,"\n====ProxyGetRequestList======\n");
			Proxy_GetRequestlist(data,csinfo,sendbuf,piid_g.data);
			break;
		case ProxyGetRequestRecord:
			fprintf(stderr,"\n====ProxyGetRequestRecord======\n");
			Proxy_GetRequestRecord(data,csinfo,sendbuf,piid_g.data);
			break;
		case ProxySetRequestList:

			break;
		case ProxySetThenGetRequestList:

			break;
		case ProxyActionRequestList:

			break;
		case ProxyActionThenGetRequestList:

			break;
		case ProxyTransCommandRequest:
			Proxy_TransCommandRequest(data,csinfo,sendbuf,piid_g.data);
			break;
	}
	return 1;
}

int doActionRequest(INT8U *apdu,CSINFO *csinfo,INT8U *buf)
{
	int	 seqnum = 0,i=0;
	int	 index = 0,apdu_index=0;
	int	 choice_index = 0;
	OAD  oad={};
	INT8U *data=NULL;
	INT8U request_choice = apdu[apdu_index+1];		//ACTION-Request
	piid_g.data = apdu[apdu_index+2];				//PIID
	Action_result	act_ret={};
	INT8U get_delay = 0;		//延时读取时间
	RESULT_NORMAL response={};

	fprintf(stderr,"\n-------- request choice = %d",request_choice);
	switch(request_choice)
	{
		case ACTIONREQUEST:
			memset(TmpDataBuf,0,sizeof(TmpDataBuf));
			index = 0;
			oad.OI= (apdu[apdu_index+3]<<8) | apdu[apdu_index+4];
			oad.attflg = apdu[apdu_index+5];
			oad.attrindex = apdu[apdu_index+6];
			data = &apdu[apdu_index+7];					//Data
			doObjectAction(oad,data,&act_ret);
			index += create_OAD(0,&TmpDataBuf[index],oad);
			TmpDataBuf[index++] = act_ret.DAR;
//			if(act_ret.DAR == success) {
				TmpDataBuf[index++] = 0;	//数据为空
//			}
			doReponse(ACTION_RESPONSE,ActionResponseNormal,csinfo,index,TmpDataBuf,buf);
			Get698_event(oad,memp);
			break;
		case ACTIONREQUEST_LIST:
			index = 0;
			apdu_index = 3;
			data = &apdu[apdu_index];					//Data
			seqnum = apdu[apdu_index++];				//3
			fprintf(stderr,"seqnum = %d\n",seqnum);
			TmpDataBuf[index++] = seqnum;
			for(i=0;i<seqnum;i++) {
				act_ret.DAR = success;	//每个OAD先默认成功
				oad.OI= (apdu[apdu_index]<<8) | apdu[apdu_index+1];
				apdu_index+=2;
				oad.attflg = apdu[apdu_index++];
				oad.attrindex = apdu[apdu_index++];
				index += create_OAD(0,&TmpDataBuf[index],oad);
				doObjectAction(oad,&apdu[apdu_index],&act_ret);
				TmpDataBuf[index++] = act_ret.DAR;
//				if(act_ret.DAR == success) {
					TmpDataBuf[index++] = 0;		//数据为空
//				}
				apdu_index += act_ret.datalen;
			}
			doReponse(ACTION_RESPONSE,ActionResponseNormalList,csinfo,index,TmpDataBuf,buf);
			break;
		case ACTIONTHENGET_REQUEST_NORMAL_LIST:
			index = 0;
			apdu_index = 3;
			data = &apdu[apdu_index];					//Data
			seqnum = apdu[apdu_index++];				//3
			fprintf(stderr,"ACTIONTHENGET_REQUEST_NORMAL_LIST  seqnum = %d\n",seqnum);
			TmpDataBuf[index++] = seqnum;
			for(i=0;i<seqnum;i++) {
				act_ret.DAR = success;	//每个OAD先默认成功
				oad.OI= (apdu[apdu_index]<<8) | apdu[apdu_index+1];
				apdu_index+=2;
				oad.attflg = apdu[apdu_index++];
				oad.attrindex = apdu[apdu_index++];
				index += create_OAD(0,&TmpDataBuf[index],oad);
				doObjectAction(oad,&apdu[apdu_index],&act_ret);
				TmpDataBuf[index++] = act_ret.DAR;
//				if(act_ret.DAR == success) {
					TmpDataBuf[index++] = 0;		//数据为空
//				}
				apdu_index += act_ret.datalen;
				//read
				oad.OI= (apdu[apdu_index]<<8) | apdu[apdu_index+1];
				apdu_index+=2;
				oad.attflg = apdu[apdu_index++];
				oad.attrindex = apdu[apdu_index++];
				get_delay = apdu[apdu_index++];						//延时读取时间
				if(get_delay>=0 && get_delay<=5) 	sleep(get_delay);
				fprintf(stderr,"read oad=%04x_%02x%02x,get_delay=%d\n",oad.OI,oad.attflg,oad.attrindex,get_delay);
				//A-ReusltNormal.OAD
				index += create_OAD(0,&TmpDataBuf[index],oad);
				memcpy(&response.oad,&oad,sizeof(response.oad));
				choice_index = index;
				index += 1;		//空出Get-Result::CHOICE的 DAR，Data的位置
				response.datalen = 0;
				response.data = &TmpDataBuf[index];	//将reposnse获取的data指向Data数据
				doGetnormal(0,&response);
				int j=0;
				fprintf(stderr,"datalen=%d\n",response.datalen);
				for(j=0;j<response.datalen;j++) {
					fprintf(stderr,"%02x ",response.data[j]);
				}
				if(response.datalen>0) {
					//A-ReusltNormal.Get-Result
					TmpDataBuf[choice_index] = 1;		//Get-Result::Data
					index = index + response.datalen;
				}else {
					TmpDataBuf[choice_index++] = 0;//错误
					TmpDataBuf[choice_index++] = response.dar;
					index = choice_index;
				}
			}
			doReponse(ACTION_RESPONSE,ActionThenGetResponseNormalList,csinfo,index,TmpDataBuf,buf);
			break;
	}
	return 1;
}
/**********************************************************************
 * 安全传输Esam校验
 *decryptData--明文信息 encryptData--密文数据集
 *输入：retData--esam验证后返回信息(需要在该函数外层开辟空间) MAC明文加MAC时，需要保存全局MAC
 *输入：CSINFO用来在组地址或广播地址时对安全标示验证（esam芯片手册：5.3第三部分）
 *输出：retData长度
 **********************************************************************/
INT16S doSecurityRequest(INT8U* apdu,CSINFO *csinfo)//
{
	if(apdu[0]!=0x10) return -1;//非安全传输，不处理
	if(apdu[1] !=0x00 && apdu[1] != 0x01) return -2 ;   //明文应用数据单元
	 INT16S retLen=0;
	 if(apdu[1]==0x00)//明文应用数据处理
	 {
		 retLen = secureDecryptDataDeal(apdu);//传入安全等级
		 apdu=&apdu[2];
	 }
	 else if(apdu[1]==0x01)//密文应用数据处理
	 {
		 retLen = secureEncryptDataDeal(apdu,apdu,csinfo);
	 }
	 fprintf(stderr,"doSecurityRequest retlen = %d\n",retLen);
	 return retLen;
}
//组织SecurityResponse上行报文
//length上行报文应用层数据长度，SecurityType下行报文等级（之前解析下行报文得出的值）
//规约要求：所有应答的安全级别不能低于请求的安全级别。此处，使用和下发报文相同安全级别回复
//返回：SendApdu中存储新的加密数据（应用数据单元和数据验证信息,包括明文/密文的开始第一个标示字节）
INT16S composeSecurityResponse(INT8U* SendApdu,INT16U Length)
{
	 INT32S ret=0;
	 fprintf(stderr,"composeSecurityResponse securetype = %d\n",securetype);
	 do
	 {
		 if(Length>0)
		 {
			 if(securetype == 0x02)//明文+mac
				 ret = compose_DataAndMac(SendApdu,Length);
			 else if(securetype == 0x03)//密文
				 ret = compose_EnData(SendApdu,Length);
			 else if(securetype == 0x04)//密文+mac
				 ret = compose_EnDataAndMac(SendApdu,Length);
			 else
				 break;
		 }
		 if(ret>0)//esam校验正常，返回
		 {
			 return ret;
		 }
	 }
	 while(0);
	 //以上都正常返回了，走到这就就很抱歉了
	 //走到这里说明esam验证出现错误，回复DAR异常错误
	 SendApdu[0]=0x90;
	 SendApdu[1]=0x02;//DAR
	 SendApdu[2]=0x16;//22ESAM校验错误
	 SendApdu[3]=0x00;//mac optional
	 return 4;
}
//组织主动上报报文安全加密（上送主站报文）
//明文发送到ESAM芯片，返回12字节RN和4字节MAC共16字节
//上报报文是明文+RN_MAC类型
//传入的SendApdu后，在该buff后面添加RN_MAC
//SendApdu第一个字节是0x00，代表明文应用数据单元，此处从第二个字节开始计算mac和rn
INT16U composeAutoReport(INT8U* SendApdu,INT16U length)
{
	if(length<=0) return 0;
	 INT16S retLen=0;
	 INT16S index=0;
	 INT8U Len[3];
	 memset(Len,0,3);
	 INT8U RN[12];
	 memset(RN,0,12);
	 INT8U MAC[4];
	 memset(MAC,0,4);
	 INT8U tmpApdu[2048];
	 memset(tmpApdu,0,2048);
	 memcpy(tmpApdu,SendApdu,length);
	 retLen = Esam_ReportEncrypt(&SendApdu[0],length,RN,MAC);
	 if(retLen<=0) return 0;
	 retLen = GetLengthByte(length,Len);
	 if(retLen<=0) return  0;
	 memcpy(SendApdu,Len,retLen);
	 index += retLen;
	 memcpy(&SendApdu[index],tmpApdu,length);
	 index+=length;
 	 SendApdu[index++]=0x02;//数据验证信息类型RN_MAC
	 SendApdu[index++]=0x0C;//随机数长度
	 memcpy(&SendApdu[index],RN,12);//12个随机数，固定大小
	 index += 12;
	 SendApdu[index++]=0x04;//mac长度
	 memcpy(&SendApdu[index],MAC,4);//MAC,固定大小
	 index+=4;
	 return  index;
}
/**********************************************************************
 *  终端主动上报后,解析主站回复数据SECURITY-response， apdu[0]=144;apdu[1]应用数据单元
 * 主动上报当前资料应用环境和流程是  明文+RN_MAC ----返回  明文+MAC
 * 上行中终端明文进入esam生成RN和MAC，主站校验，返回明文和MAC，终端根据上行的RN和主站返回的MAC校验
 * 注意RN需要终端主动上报后本地保存(全局变量)
 **********************************************************************/
INT16S parseSecurityResponse(INT8U* RN,INT8U* apdu)//apdu负责传入和传出数据，一人全包
{
	if(apdu[1]==0x00 )//明文处理
	{
	     INT32S retLen = secureResponseData(RN,apdu);
	     return retLen;
	}
	else if(apdu[1]==0x01)//密文
	{
		return -1;
	}
	else if( apdu[1]==0x02)//异常
	{
		printf("parseSecurityResponse receive err flag DAR ,NO=%d",apdu[2]);
		return apdu[2];
	}
	else
		return -2;//无效应用数据单元标示
}


//OAD转换为报文
INT8U OADtoBuff(OAD fromOAD,INT8U* buff)
{
	memcpy(&buff[0],&fromOAD,sizeof(OAD));
	INT8U tmp = buff[0];
	buff[0] = buff[1];
	buff[1] = tmp;
	return sizeof(OAD);
}

INT16S fillGetRequestAPDU(INT8U* sendBuf,CLASS_6015 obj6015,INT8U requestType)
{
	INT16S length = 0;
	INT8U csdIndex = 0;
	INT8U len = 0;
	if((requestType == GET_REQUEST_NORMAL_LIST)||(requestType == GET_REQUEST_RECORD_LIST))
	{
		sendBuf[length++] = obj6015.csds.num;
	}

	if(obj6015.cjtype == TYPE_NULL)
	{
		for(csdIndex = 0;csdIndex < obj6015.csds.num;csdIndex++)
		{
			/*采集当前数据*/
			if(obj6015.csds.csd[csdIndex].type == 0)//OAD
			{
				len = OADtoBuff(obj6015.csds.csd[csdIndex].csd.oad,&sendBuf[length]);
				length +=len;
			}
			else
			{
				fprintf(stderr,"fillGetRequestAPDU not OAD obj6015.sernum = %d,obj6015.cjtype = %d",
						obj6015.sernum,obj6015.cjtype);
			}

		}
	}
	else
	{
		for(csdIndex = 0;csdIndex < obj6015.csds.num;csdIndex++)
		{

			if(obj6015.csds.csd[csdIndex].type == 1)//ROAD
			{
				len = OADtoBuff(obj6015.csds.csd[csdIndex].csd.road.oad,&sendBuf[length]);
				length +=len;
				/*采集上N次数据*/
				if(obj6015.cjtype == TYPE_LAST)
				{
					// selector 9
					sendBuf[length++] = 0x09;//Selector = 9 选取上n条记录
					sendBuf[length++] = 0x01;//选取上1条记录
				}
				/*按照时标间隔采集*/
				if(obj6015.cjtype == TYPE_INTERVAL)
				{
					// selector ２
					sendBuf[length++] = 0x02;//Selector = 2
					//冻结时标OAD
					sendBuf[length++] = 0x20;
					sendBuf[length++] = 0x21;
					sendBuf[length++] = 0x02;
					sendBuf[length++] = 0x00;

					//开始时间结束时间
					memcpy(&sendBuf[length],&obj6015.data.data[CURVE_INFO_STARTINDEX],16);
					length+=16;
					//时间间隔

					sendBuf[length++] = dtti;
					sendBuf[length++] = obj6015.data.data[0];
					sendBuf[length++] = obj6015.data.data[1];
					sendBuf[length++] = obj6015.data.data[2];

				}
				/*按冻结时标采集*/
				if(obj6015.cjtype == TYPE_FREEZE)
				{
					// selector 9
					sendBuf[length++] = 0x01;//Selector = 1
					//冻结时标OAD
					sendBuf[length++] = 0x20;
					sendBuf[length++] = 0x21;
					sendBuf[length++] = 0x02;
					sendBuf[length++] = 0x00;

					DateTimeBCD timeStamp;
					DataTimeGet(&timeStamp);

					sendBuf[length++] = 0x1c;
					INT16U tmpTime = timeStamp.year.data;
					sendBuf[length++] = (tmpTime>>8)&0x00ff;
					sendBuf[length++] = tmpTime&0x00ff;
					sendBuf[length++] = timeStamp.month.data;
					sendBuf[length++] = timeStamp.day.data;
					sendBuf[length++] = 0x00;
					sendBuf[length++] = 0x00;
					sendBuf[length++] = 0x00;
				}
				sendBuf[length++] = obj6015.csds.csd[csdIndex].csd.road.num;//OAD num

				INT8U oadsIndex;
				for (oadsIndex = 0; oadsIndex < obj6015.csds.csd[csdIndex].csd.road.num; oadsIndex++)
				{

					sendBuf[length++] = 0;//OAD
					len = OADtoBuff(obj6015.csds.csd[csdIndex].csd.road.oads[oadsIndex],&sendBuf[length]);
					length +=len;
				}

			}
			else
			{
				fprintf(stderr,"fillGetRequestAPDU not ROAD obj6015.sernum = %d,obj6015.cjtype = %d",
						obj6015.sernum,obj6015.cjtype);
			}

		}
	}

	return length;

}
//
INT8S getRequestType(INT8U cjtype,INT8U csdcount)
{

	INT8S requestType = -1;

	switch(cjtype)
	{
		case TYPE_NULL:
			{
				if(csdcount == 1)
				{
					requestType = GET_REQUEST_NORMAL;
				}
				if(csdcount > 1)
				{
					requestType = GET_REQUEST_NORMAL_LIST;
				}

				break;
			}

		case TYPE_LAST:
			{
				requestType = GET_REQUEST_RECORD;

				if(csdcount == 1)
				{
					requestType = GET_REQUEST_RECORD;
				}
				if(csdcount > 1)
				{
					requestType = GET_REQUEST_RECORD_LIST;
				}

				break;
			}

		case TYPE_FREEZE:
			requestType = GET_REQUEST_RECORD;
			break;
		case TYPE_INTERVAL:
			requestType = GET_REQUEST_RECORD;
			break;
	}


	return requestType;
}
INT16S composeProtocol698_SetRequest(INT8U* sendBuf,RESULT_NORMAL setData,TSA meterAddr)
{

	INT8U PIID = 0x02;
	int sendLen = 0, hcsi = 0,apdulen = 0;

	CSINFO csinfo={};

	csinfo.dir = 0;		//服务器发出
	csinfo.prm = 1; 	//服务器发出
	csinfo.funcode = 3; //链路管理
	csinfo.sa_type = 0 ;//单地址


	INT8U reverseAddr[OCTET_STRING_LEN]= {0};
	fprintf(stderr," \n\n composeProtocol698_GetRequest  meterAddr : %02x  %02x  %02x%02x%02x%02x%02x%02x%02x\n\n",
			meterAddr.addr[0],meterAddr.addr[1],meterAddr.addr[2],meterAddr.addr[3],meterAddr.addr[4],
			meterAddr.addr[5],meterAddr.addr[6],meterAddr.addr[7],meterAddr.addr[8]);
	csinfo.sa_length = (meterAddr.addr[1]&0x0f) + 1;//sizeof(addr)-1;//服务器地址长度
	///当广播地址时，地址类型=3：广播地址，增加下面的赋值
	csinfo.sa_type = (meterAddr.addr[1] >> 6) & 0x03;		//服务器地址类型
	reversebuff(&meterAddr.addr[2],csinfo.sa_length,reverseAddr);

	fprintf(stderr," \n reverseAddr[%d] = ",csinfo.sa_length);
	INT8U prtIndex=0;
	for(prtIndex = 0;prtIndex < csinfo.sa_length;prtIndex++)
	{
		fprintf(stderr," %02x",reverseAddr[prtIndex]);
	}

	memcpy(csinfo.sa,reverseAddr,csinfo.sa_length);//服务器地址
	csinfo.ca = 0x02;

	fprintf(stderr,"sa_length = %d \n",csinfo.sa_length);
	sendLen = FrameHead(&csinfo,sendBuf) ; //	2：hcs  hcs
	hcsi = sendLen;
	sendLen = sendLen + 2;

	sendBuf[sendLen++] = SET_REQUEST;
	sendBuf[sendLen++] = SET_REQUEST_NORMAL;
	sendBuf[sendLen++] = PIID;
	OADtoBuff(setData.oad,&sendBuf[sendLen]);
	sendLen += 4;
	INT16U dataIndex = 0;
	for(dataIndex = 0;dataIndex < setData.datalen;dataIndex++)
	{
		sendBuf[sendLen++] = setData.data[dataIndex];
	}
	sendBuf[sendLen++] = 0x00;//没有时间标签

	FrameTail(sendBuf,sendLen,hcsi);
	return (sendLen + 3);			//3: cs cs 16


}
INT16S composeProtocol698_GetRequest(INT8U* sendBuf,CLASS_6015 obj6015,TSA meterAddr)
{
	INT8U PIID = 0x02;
	int sendLen = 0, hcsi = 0,apdulen = 0;

	CSINFO csinfo={};

	csinfo.dir = 0;		//服务器发出
	csinfo.prm = 1; 	//服务器发出
	csinfo.funcode = 3; //链路管理
	csinfo.sa_type = 0 ;//单地址


	INT8U reverseAddr[OCTET_STRING_LEN]= {0};
#if 0
	fprintf(stderr," \n\n composeProtocol698_GetRequest  meterAddr : %02x  %02x  %02x%02x%02x%02x%02x%02x%02x\n\n",
			meterAddr.addr[0],meterAddr.addr[1],meterAddr.addr[2],meterAddr.addr[3],meterAddr.addr[4],
			meterAddr.addr[5],meterAddr.addr[6],meterAddr.addr[7],meterAddr.addr[8]);
#endif
	csinfo.sa_length = (meterAddr.addr[1]&0x0f) + 1;//sizeof(addr)-1;//服务器地址长度

	reversebuff(&meterAddr.addr[2],csinfo.sa_length,reverseAddr);
#if 0
	fprintf(stderr," \n reverseAddr[%d] = ",csinfo.sa_length);
	INT8U prtIndex;
	for(prtIndex = 0;prtIndex < csinfo.sa_length;prtIndex++)
	{
		fprintf(stderr," %02x",reverseAddr[prtIndex]);
	}
#endif
	memcpy(csinfo.sa,reverseAddr,csinfo.sa_length);//服务器地址
	csinfo.ca = 0x02;

	//fprintf(stderr,"sa_length = %d \n",csinfo.sa_length);
	sendLen = FrameHead(&csinfo,sendBuf) ; //	2：hcs  hcs
	hcsi = sendLen;
	sendLen = sendLen + 2;

	sendBuf[sendLen++] = GET_REQUEST;

	INT8S requestType = getRequestType(obj6015.cjtype,obj6015.csds.num);
	//fprintf(stderr,"\n composeProtocol698_GetRequest requestType = %d cjtype = %d obj6015.csds.num = %d",requestType,obj6015.cjtype,obj6015.csds.num);
	if(requestType < 0)
	{
		return-1;
	}
	sendBuf[sendLen++] = requestType;
	sendBuf[sendLen++] = PIID;
	apdulen = fillGetRequestAPDU(&sendBuf[sendLen],obj6015,requestType);
	sendLen += apdulen;
	sendBuf[sendLen++] = 0x00;//没有时间标签

	FrameTail(sendBuf,sendLen,hcsi);
	return (sendLen + 3);			//3: cs cs 16

}
INT16U getFECount(INT8U* recvBuf, const INT16U recvLen)//得到待解析报文中前导符FE的个数
{
	INT16U i, count=0;
	for (i=0; i<recvLen; i++)
	{
		if (recvBuf[i] == 0x68)
		{
			count = i;
			break;
		}
	}
	return count;
}
/*
 * 返回值 getType : GET_REQUEST_NORMAL   GET_REQUEST_NORMAL_LIST GET_REQUEST_RECORD GET_REQUEST_RECORD_LIST
 * resultCount ： OAD  或者 ROAD数量
 * recvLen：接受报文字节数 所有报文+ FEFE
 * apduDataStartIndex： 返回数据开始位置
 * dataLen： 数据部分长度
 * */
//解析抄表回复报文
INT8U analyzeProtocol698(INT8U* Rcvbuf,INT8U* resultCount,INT16S recvLen,INT8U* apduDataStartIndex,INT16S* dataLen)
{
	fprintf(stderr,"\nanalyzeProtocol698---\n");
	INT16U count = getFECount(Rcvbuf, recvLen);//得到待解析报文中前导符FE的个数
	Rcvbuf += count;
	INT8U startIndex = count;
	*dataLen = recvLen - count;
	INT8U getType = 0;
	CSINFO csinfo={};
	int hcsok = 0 ,fcsok = 0;
	INT8U *apdu= NULL;

	hcsok = CheckHead( Rcvbuf ,&csinfo);
	fcsok = CheckTail( Rcvbuf ,csinfo.frame_length);
	fprintf(stderr,"\n hcsok = %d fcsok = %d\n",hcsok,fcsok);
	if ((hcsok==1) && (fcsok==1))
	{
		fprintf(stderr,"\nsa_length=%d\n",csinfo.sa_length);
		apdu = &Rcvbuf[csinfo.sa_length+8];
		startIndex += csinfo.sa_length+8;
		*dataLen = *dataLen - (csinfo.sa_length+8);
		fprintf(stderr,"\n apdu: %02x %02x %02x %02x %02x %02x",apdu[0],apdu[1],apdu[2],apdu[3],apdu[4],apdu[5]);

		if(apdu[0] == GET_REQUEST_RESPONSE)
		{
			getType = apdu[1];
			if((getType == GET_REQUEST_NORMAL)||(getType == GET_REQUEST_RECORD))
			{
				*resultCount = 1;
				startIndex += 3;
				*dataLen = *dataLen - (3+3);
			}
			if((getType == GET_REQUEST_NORMAL_LIST)||(getType == GET_REQUEST_RECORD_LIST))
			{
				*resultCount = apdu[3];
				startIndex += 4;
				*dataLen = *dataLen - (4+3);
			}
			*apduDataStartIndex = startIndex;
#if 0
			switch(apdu[1])
			{
				case GET_REQUEST_NORMAL:
					dealResponse_RequestNormal(oad,data,csinfo,sendbuf);
					break;
				case GET_REQUEST_NORMAL_LIST:
					/*重新定位数据指针地址*/
					data = &apdu[3];
					dealResponse_RequestNormalList(data,csinfo,sendbuf);
					break;
				case GET_REQUEST_RECORD:
					dealResponse_RequestRecord(oad,data,csinfo,sendbuf);
					break;
				case GET_REQUEST_RECORD_LIST:
					break;
				case GET_REQUEST_RECORD_NEXT:
					break;

			}
#endif
		}
		else
		{
			fprintf(stderr,"\nanalyzeProtocol698 校验错误");
		}


	}
	return getType;
}

int doReleaseConnect(INT8U *apdu,CSINFO *csinfo,INT8U *sendbuf)
{
	int apduplace =0,index=0, hcsi=0;
	ClientPiid = apdu[1];
	csinfo->dir = 1;
//	csinfo->prm = 0;
	csinfo->prm = 1;
	index = FrameHead(csinfo,sendbuf);
	hcsi = index;
	index = index + 2;

	apduplace = index;		//记录APDU 起始位置
	sendbuf[index++] = RELEASE_RESPONSE;
	sendbuf[index++] = ClientPiid;
	sendbuf[index++] = 0;
	sendbuf[index++] = 0;
	sendbuf[index++] = 0;

	FrameTail(sendbuf,index,hcsi);
	if(pSendfun!=NULL)
		pSendfun(comfd,sendbuf,index+3);
	fprintf(stderr,"\n			断开应用连接 PIID = %x",ClientPiid);
	return 1;
}

/*
 * 获取TimeTag
 * */
void getTimeTag(INT8U *buf,TimeTag *tag)
{
	memset(tag,0,sizeof(TimeTag));
	tag->flag = buf[0];
	tag->sendTimeTag.year.data = (buf[1]<<8) | buf[2];
	tag->sendTimeTag.month.data = buf[3];
	tag->sendTimeTag.day.data = buf[4];
	tag->sendTimeTag.hour.data = buf[5];
	tag->sendTimeTag.min.data = buf[6];
	tag->sendTimeTag.sec.data = buf[7];
	tag->ti.units = buf[8];
	tag->ti.interval = (buf[9]<<8) | buf[10];
}

/**********************************************************************
 * 1.	CONNECT.request 服务,本服务由客户机应用进程调用,用于向远方服务器的应用进程提出建立应用连接请求。
 * 						主站（客户机）请求集中器（客户机）建立应用连接
 */
INT8U dealClientRequest(INT8U *apdu,CSINFO *csinfo,TimeTag timetag,INT8U *sendbuf)
{
	INT16S 	SecurityRe =0;
	INT8U 	apduType = apdu[0];//0x10  [16]

	fprintf(stderr,"\n-------- apduType = %d ",apduType);

	if (apduType == SECURITY_REQUEST)//安全请求的数据类型
	{
		SecurityRe = doSecurityRequest(apdu,csinfo);
		if (SecurityRe <= 0)
		{
			fprintf(stderr,"\n安全请求计算错误!!!");
			return 0;
		}
//		else
//		{
//			fprintf(stderr,"apduType = %d\n",apduType);
//			int i;
//			for( i=0;i<SecurityRe;i++)
//				fprintf(stderr,"%02x ",apdu[i]);
//			fprintf(stderr,"\n");
//		}
		apduType = apdu[0];
		if(SecurityRe > sizeof(TimeTag)) {
			getTimeTag(&apdu[SecurityRe-11],&timetag);	//1:0x68,-2:cs cs(帧校验),-11:时间标签数据
		}
	}
	//判断时间标签是否有效
	isTimeTagEffect(timetag,&Response_timetag);
	switch(apduType)
	{
		case CONNECT_REQUEST:
			appConnectResponse(apdu,csinfo,sendbuf);
			break;
		case GET_REQUEST:
			doGetAttribute(apdu,csinfo,sendbuf);
			break;
		case SET_REQUEST:
			doSetAttribute(apdu,csinfo,sendbuf);
			break;
		case ACTION_REQUEST:
			fprintf(stderr,"\n ACTION_REQUEST");
			doActionRequest(apdu,csinfo,sendbuf);
			break;
		case PROXY_REQUEST:
			fprintf(stderr,"\n PROXY_REQUEST");
			doProxyRequest(apdu,csinfo,sendbuf);
			break;
		case RELEASE_REQUEST:
			doReleaseConnect(apdu,csinfo,sendbuf);
			break;
	}
	return(apduType);
}

void testframe(INT8U *apdu,int len)
{
	int hcsi=0;
	INT8U buf[512]={};
	int i=0;
	buf[i++]= 0x68;//起始码
	buf[i++]= 0;	//长度
	buf[i++]= 0;
	buf[i++]= 0x43;
	buf[i++]= 0x05;
	buf[i++]= 0x05;
	buf[i++]= 0x00;
	buf[i++]= 0x00;
	buf[i++]= 0x00;
	buf[i++]= 0x00;
	buf[i++]= 0x00;
	buf[i++]= 0x01;
	hcsi = i;
	i = i + 2;
	memcpy(&buf[i],apdu,len);
	i = i + len;
	FrameTail(buf,i,hcsi);
	int k=0;
	fprintf(stderr,"\n");
	for(k=0;k<i+3;k++)
		fprintf(stderr,"%02x ",buf[k]);
	fprintf(stderr,"\n----------------------------------------\n");
}

int ProcessData(CommBlock *com)
{
	TimeTag		timetag={};
	CSINFO csinfo={};
	int hcsok = 0 ,fcsok = 0;
	INT8U *apdu= NULL;
	INT8U *Rcvbuf = com->DealBuf;
	INT8U *SendBuf = com->SendBuf;
	linkResponse_p = &com->linkResponse;
	myAppVar_p = &com->myAppVar;
	AppVar_p = &com->AppVar;
	memp = (ProgramInfo*)com->shmem;
	pSendfun = com->p_send;
	comfd = com->phy_connect_fd;
	if (CheckSerAddr(Rcvbuf ,com->serveraddr)==0)
		return 0;
	hcsok = CheckHead( Rcvbuf ,&csinfo);
	com->taskaddr = csinfo.ca;
	broadcast = csinfo.sa_type;
	fcsok = CheckTail( Rcvbuf ,csinfo.frame_length);
	if(csinfo.frame_length > (sizeof(TimeTag)+2)) {
		getTimeTag(&Rcvbuf[1+csinfo.frame_length-2-11],&timetag);	//1:0x68,-2:cs cs(帧校验),-11:时间标签数据
	}
	securetype = 0x00;
	if ((hcsok==1) && (fcsok==1))
	{
		fprintf(stderr,"\nsa_length=%d\n",csinfo.sa_length);
		apdu = &Rcvbuf[csinfo.sa_length+8];
		if (csinfo.dir == 0 && csinfo.prm == 0)		/*客户机对服务器上报的响应	（主站对集中器上报的响应）*/
		{
			return(dealClientResponse(apdu,&csinfo));
		}else if (csinfo.dir==0 && csinfo.prm == 1)	/*客户机发起的请求			（主站对集中器发起的请求）*/
		{
			fprintf(stderr,"\n-------- 客户机发起请求 ");
			return(dealClientRequest(apdu,&csinfo,timetag,SendBuf));
		}else if (csinfo.dir==1 && csinfo.prm == 0)	/*服务器发起的上报			（电表主动上报）*/
		{
			fprintf(stderr,"\n服务器发起的上报			（电表主动上报）");
			//MeterReport();
		}else if (csinfo.dir==1 && csinfo.prm == 1)	/*服务器对客户机请求的响应	（电表应答）*/
		{
			fprintf(stderr,"\n服务器对客户机请求的响应	（电表应答）");
			//MeterEcho();
		}else
		{
			fprintf(stderr,"\n控制码解析错误(传输方向与启动位错误)");
		}
	}
	return 1;
}

