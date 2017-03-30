/*
 * lcdprt.c
 *
 *  Created on: 2013-8-16
 *      Author: yd
 */
#include <assert.h>
#include "lcdprt.h"
#include "mutils.h"
INT8U DongJie_FileName[100];

struct erc_t ERCNAME[EVENTCOUNT] =
{
{ "没有事件" },
{ "数据初始化和版本变更记录" },//1
{ "参数丢失记录" },//2
{ "参数变更记录" },//3
{ "状态量变位记录" },//4
{ "遥控跳闸记录" },//5
{ "功控跳闸记录" },//6
{ "电控跳闸记录" },//7
{ "电能表参数变更" },//8
{ "电流回路异常" },//9
{ "电压回路异常" },//10
{ "相序异常" },//11
{ "电能表时间超差" },//12
{ "电表故障信息" },//13
{ "终端停/上电事件" },//14
{ "谐波越限告警" },//15
{ "直流模拟量越限" },//16
{ "电压/电流不平衡越限" },//17
{ "电容器投/切自锁记录" },//18
{ "购电参数设置记录" },//19
{ "消息认证错误记录" },//20
{ "终端故障记录" },//21
{ "有功总电能量差动越限" },//22
{ "电控告警事件" },//23
{ "电压越限记录" },//24
{ "电流越限记录" },//25
{ "视在功率越限记录" },//26
{ "电能表示度下降" },//27
{ "电能量超差" },//28
{ "电能表飞走" },//29
{ "电能表停走" },//30
{ "终端485抄表失败" },//31
{ "通信流量超门限" },//32
{ "电表运行状态字变位" },//33
{ "电流互感器异常" },//34
{ "发现未知电能表" },//35
{ "控制开关状态量变位" },//36
{ "电能表开盖事件" },//37
{ "电能表开端钮盒事件" },//38
{ "补抄失败事件" },//39
{ "磁场异常事件" },//40
{ "对时事件" },//41
{ "备用" },//42
{ "备用" },//43
{ "备用" },//44
{ "电池失压事件" },//45
{ "备用" },//46
{ "备用" },//47
{ "备用" },//48
{ "备用" },//49
{ "备用" },//50
{ "备用" },//51
{ "备用" },//52
{ "备用" },//53
{ "备用" },//54
{ "备用" },//55
{ "备用" },//56
{ "备用" },//57
{ "备用" },//58
{ "备用" },//59
{ "备用" },//60
};

//根据dm.cfg 获得 数据项的后缀
int get_dataflg_suffix(int did, char *name){

return 0;
}

//检测是否有效的测量点地址  addr[12] 返回 1 有效 返回0 无效
int isValidMeterAddr_12(char *addr){
	int valid=1,i;
	if(memcmp((void*)addr,(void*)"ffffffffffff",12)==0)
		valid = 0;
	if(memcmp((void*)addr,(void*)"FFFFFFFFFFFF",12)==0)
		valid = 0;
	for(i=0; i<12; i++){
		if(addr[i]<'0'|| addr[i]>'9'){
			valid = 0;
			break;
		}
	}
	return valid;
}
static int OprMode;// 查看模式 设置模式  0 取消 1 设置模式 2 查看模式
int get_oprmode(){
	return OprMode;
}
void set_oprmode(int mode){
	OprMode = mode;
}


double lcd_A02toDouble(short A2){
	double ret=0;
	int integer,exp=0, positive=0;
//	dbg_prt("\n A2=%x", A2);
	integer = ((A2>>8)&0x0f)*100 + ((A2>>4)&0x0f)*10 + (A2&0x0f);
	positive = (A2>>12) & 0x01;
	exp = (A2>>13)&0x07;
	if(positive==0)
		ret = integer*pow(10,(4-exp));
	else
		ret = 0 - integer*pow(10,(4-exp));
//	dbg_prt("\n ret = %f", ret);
	return ret;
}

double lcd_A02toDouble_decbits(INT16S A2, INT8U decbits){
	double ret=0;
//	dbg_prt("\n A2=%x", A2);
	ret = A2*pow(10, decbits);
//	dbg_prt("\n ret = %f", ret);
	return ret;
}

int lcd_A03toint(int A3, int *G){
	int ret=0;
	int i;
	for(i=0; i<7; i++)
		ret += ((A3>>(i*4))&0x0f)*pow(10,i);
	*G = ((A3>>30)&0x01);
	return ret;
}
int lcd_A03toint_c(INT8U *data, int *G){
	int ret=0;
	ret = (data[0]&0x0f)+((data[0]&0xf0)>>4)*10;
	ret += (data[1]&0x0f)*100+((data[1]&0xf0)>>4)*1000;
	ret += (data[2]&0x0f)*10000+((data[2]&0xf0)>>4)*100000;
	ret += (data[3]&0x0f)*1000000;
	*G = ((data[3]>>6)&0x01);
	return ret;
}
char lcd_A04tochar(char A4, int *S){
	char ret=0;
	ret = ((A4>>4)&0x07)*10 + (A4&0x0f);
	*S = ((A4>>7)&0x01);
	return ret;
}
void lcd_A15toDate(INT8U* A15,INT8U* year,INT8U* month,INT8U* day,INT8U* hour,INT8U* min)
{
	*year = ((A15[4]&0xf0)>>4)*10 + (A15[4]&0x0f) ;
	*month = ((A15[3]&0xf0)>>4)*10 + (A15[3]&0x0f);
	*day  =  ((A15[2]&0xf0)>>4)*10 + (A15[2]&0x0f);
	*hour = ((A15[1]&0xf0)>>4)*10 + (A15[1]&0x0f);
	*min  = ((A15[0]&0xf0)>>4)*10 + (A15[0]&0x0f);
}
void lcd_A19toDate(char* A19, char* hour, char* min){
	*hour = ((A19[1]&0xf0)>>4)*10 + (A19[1]&0x0f);
	*min  = ((A19[0]&0xf0)>>4)*10 + (A19[0]&0x0f);
}
void lcd_A20toDate(char* A20,char* year,char* month,char* day)
{
	*year = ((A20[2]&0xf0)>>4)*10 + (A20[2]&0x0f) ;
	*month = ((A20[1]&0xf0)>>4)*10 + (A20[1]&0x0f);
	*day  = ((A20[0]&0xf0)>>4)*10 + (A20[0]&0x0f);
}
//是否有效的测量点 index  测量点序号
//TODO:判断有效测量点
int gui_isValidCld(int index){
	INT8U Flag_MPValid=0;
	if(index>0)
//	if (ParaAll->f10.para_mp[index-1].MPNo > 0)
//	{
////		fprintf(stderr,"cldno = %d	",ParaAll->f10.para_mp[index-1].MPNo);
//		Flag_MPValid = ParaAll->f150.Flag_MPValid[ParaAll->f10.para_mp[index-1].MPNo-1];
////		fprintf(stderr,"youxiao = %d \n",Flag_MPValid);
//	}
	return Flag_MPValid;
}

int bcd2int(char *bcd){
	int ret=0;
	if(bcd2int32u((INT8U*)bcd, 1, positive, (INT32U *)&ret)==0)
		return ret;
	else
		return 0;
}
//是否有效的测量点
//TODO:判断有效测量点
int gui_isValidCldAddr(INT8U *addr){
	int i, index=0;
//	for(i=0; i<MP_MAXNUM;i++){
//		if(gui_isValidCld(i+1)==0)
//			continue;
//		if(memcmp(addr, ParaAll->f10.para_mp[i].addr, 12)==0){
//			index = ParaAll->f10.para_mp[i].Index;
//			break;
//		}
//	}
	return index;//0:无效
}
//int gui_isValidCldAddr(INT8U *addr){
//	int i, iindex=0, mpno=0;
//	for(i=0; i<MP_MAXNUM;i++){
//		if(gui_isValidCld(i+1)==0)
//			continue;
//		if(memcmp(addr, ParaAll->f10.para_mp[i].addr, 12)==0){
//			iindex = i;
//			mpno = ParaAll->f10.para_mp[iindex].MPNo;
//			break;
//		}
//	}
////	return mpno;
//	return iindex;//0:无效
//}
//获得有效测量点总数
//TODO:获得有效测量点总数
int gui_GetCldNum(){
	int i, num=0;
//	for(i=0; i<MP_MAXNUM;i++){
//		if(gui_isValidCld(i+1)>=1)
//			num++;
//	}
	return num;
}
//获得交采测量点
//TODO:获得交采测量点
int gui_GetJCMP(){
	int i, jccld=0;
//	for(i=0; i<MP_MAXNUM;i++){
//		if(gui_isValidCld(i+1)==1&&ParaAll->f10.para_mp[i].Protocol==PROTOCOL_JC){
//			jccld = i+1;
//			break;
//		}
//	}
	return jccld;
}
//字符串(12字节)转ip(4字节)
int ip_strtobyte(INT8U *sip, INT8U *ip_ret){
	char ret_flg=1;
	int i, ipret[4];
	INT8U iptmp[4];
	memset(ipret, 0, 4*sizeof(int));
	for(i=0; i<4; i++){
		memset(iptmp,0,4);
		memcpy(iptmp, &sip[i*3], 3);
		ipret[i] = atoi((char*)iptmp);
		if(ipret[i]>0xff)
			ret_flg = 0;
		ip_ret[i] = ipret[i]&0xff;
		//dbg_prt("\n sip[%d*3]=%s ipret[i]=%d ip_ret[i]=%d", i,iptmp,ipret[i],ip_ret[i]);
	}
	return ret_flg;
}
//ip 待转换的ip  sip转换后的字符串
void ip2asc(INT8U *ip, char* sip){
	sprintf(&sip[0], "%03d", ip[0]);
	sprintf(&sip[3], "%03d", ip[1]);
	sprintf(&sip[6], "%03d", ip[2]);
	sprintf(&sip[9], "%03d", ip[3]);
}
/*
**
** 静态数组实现堆栈程序 a_stack.c ，数组长度由#define确定
*/
#define STACK_SIZE 10 /* 堆栈最大容纳元素数量 */
/*
** 存储堆栈中的数组和一个指向堆栈顶部元素的指针
*/
static STACK_TYPE stack[STACK_SIZE];
static int top_element = -1;
/* push */
int push(STACK_TYPE *value){
    if(is_full()) /* 压入堆栈之前先判断是否堆栈已满*/
    	return 0;
    top_element += 1;
    memcpy(&stack[top_element], value, sizeof(STACK_TYPE));
    return 1;
}
/* pop */
int pop(void){
    if(is_empty())
    	return 0;
    memset(&stack[top_element], 0, sizeof(STACK_TYPE));
    top_element -= 1;
    return 1;
}
/* top */
int top(STACK_TYPE *value){
    if(is_empty())
    	return 0;
    memcpy(value, &stack[top_element], sizeof(STACK_TYPE));
    return 1;
}
/* is_empty */
int is_empty(void){
    return top_element == -1;
}
/* is_full */
int is_full(void){
    return top_element == STACK_SIZE - 1;
}

