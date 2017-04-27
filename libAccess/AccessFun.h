
#ifndef ACCESSFUNH_H_
#define ACCESSFUNH_H_

#include "StdDataType.h"
#include "Objectdef.h"
/********************************************************
 * 存储结构：
 * 1、参数类
 * 	  [1] ->/nand/para/oi.par	 			追加的参数文件
 * 	  	  ->/nand/para/oi.bak
 * 	  [2] --> /nand/para/oi/01-255.par		按照序号等存储一组参数
 * 	  	  --> /nand/para/oi/01-255.bak
 * 2、事件类
 *	  [1] ->/nand/event/property/oi/oi.par[oi.bak]		oi属性参数文件
 *	  [2] -->/nand/event/record/oi/01-255.dat		事件记录表
 *	  [3] -->/nand/event/current/oi/01-255.dat		当前值记录表
 *
 ********************************************************/
#define  EVENTDIR			"/nand/event/"
#define	 EVENT_PORP			"/nand/event/property"		//事件属性目录
#define	 EVENT_REC			"/nand/event/record"		//事件记录目录
#define	 EVENT_CURR			"/nand/event/current"		//当前值记录表

#define  USERDIR			"/nand/bin"
#define	 VARI_DIR			"/nand/data"				//变量类数据文件目录
#define	 VARI_DATA				"/nand/data/vari.dat"		//变量类-计量、采集子类数据存储
#define	 VARI_DATA_TJ			"/nand/data/vari_tj.dat"		//变量类－统计子类数据存储
#define	 PARADIR				"/nand/para"				//参变量文件（4000） 采集监控类文件（6000）
//#define	 DEMANDDIR				"/nand/demand"				//需量类数据
//#define	 FREEZEDIR				"/nand/freeze"				//冻结类数据存储
//#define	 CALCDIR				"/nand/calc"				//统计类数据存储
#define	 INITDIR				"/nor/init"					//初始化参数文件
#define	 TASKDATA				"/nand/task"				//任务采集存储文件
#define	 EVEDATA				"/nand/allevent"				//全事件数据存储文件
#define  PROTOCOL_TRANS_PATH    "/nor/ProTransCfg/protocol.cfg" //湖南规约切换通信参数文件

typedef struct
{
	INT8U trans_flg;    //转换标志
	INT8U AreaNo[2];	//行政区码
	INT8U TmnlAddr[2];	//终端地址
	INT8U OOPAddr[6];   //面向对象终端地址
	INT8U main_ip[4];
	INT16U main_port;
	INT8U bak_ip[4];
	INT16U bak_port;
	INT8U APN[20];
	INT8U Len_UsrName;	//用户名长度，数值范围0-20，为0时、表示无用户名，为非0时、表示连接方式需用户名验证
	INT8U UsrName[20];	//用户名
	INT8U Len_Pwd;		//密码长度：0-20，当为0时，表示无密码，当为非0时，表示连接方式需要密码验证
	INT8U Pwd[20];		//密码
}Protocol_Trans;//湖南切换规约，通信参数文件结构体
//文件存储类型
typedef enum
{
	event_para_save=1,			//参数文件存储
	event_record_save=2,		//事件记录表存储
	event_current_save=3,		//当前值记录表存储
	para_vari_save=4,		    //参变量类对象
	coll_para_save=5,			//采集类参数存储
	acs_coef_save=6,			//交采计量芯片系数存储
	acs_energy_save=7,			//交采计量电能量数据存储
	para_init_save=8,			//初始化参数保存文件
}SaveFile_type;

typedef struct {
	INT16U  runtime;  //一天执行次数，日月年冻结和实时数据无效，置位1，由执行频率计算，主要针对负荷曲线，0表示对于这个采集方案任务无效
	INT16U starthour;
	INT16U startmin; //开始执行分钟，主要针对负荷曲线
	INT16U endhour;
	INT16U endmin;   //结束执行分钟，主要针对负荷曲线
	INT32U freq;     //执行频率
	INT8U  KBtype;   //开闭方式 0000 0011前闭后闭 0000 0000前开后开,以此类推
	INT16U memdep;   //存储深度
	CSD_ARRAYTYPE    csds;   //采集方案号
}TASKSET_INFO;
typedef struct {//例如：oad_m为50040200，oad_r为00100200 关联属性oad，没有的oad_m写为00000000
	INT8U taskid;
	OAD oad_m;
	OAD oad_r;
	INT8U oad_num;//oad的写为1，road的写为从oad个数
}OAD_MR;
typedef struct {
	INT16U oadmr_num;//涉及到的road个数，每一个都写成二维OAD_MR，oad类型的关联属性oad_r写为0000
	OAD_MR oad[MY_CSD_NUM*ROAD_OADS_NUM];
}ROAD_ITEM;//将招测csd分解为多个oad
typedef struct {
	int recordno_num;//序列总数
	time_t rec_start;//开始时间秒数
	time_t rec_end;//结束时间秒数
}CURR_RECINFO;//当前记录信息
/*
 * 更改拨号脚本
 * */
extern void write_apn(char* apn);
extern void write_userpwd(unsigned char* user, unsigned char* pwd, unsigned char* apn);
/*
 * 方法：Clean()清空
 * 输入参数：oi对象标识
 * 返回值：=1：配置单元删除成功
 * =-1:  未查找到OI类数据
 */
extern int clearClass(OI_698 oi);

/*
 * 方法：Delete() 删除一个配置单元
 * 输入参数：oi对象标识，id:索引
 * 返回值：=0：配置单元删除成功
 * =-1:  删除错误
 */
extern int deleteClass(OI_698 oi,INT8U id);

/*
 * 数据区初始化接口函数
 * 返回值 =0: 删除成功
 * =-1：删除失败
 * */
extern int dataInit(INT16U attr);

////////////////////////////////////////////////////////////////////////////////////////
/*		第一类参数文件：文件包含接口类公用属性，配置单元按照配置序号在相应的位置存储，
 *           方法：存储更新（追加，更新），删除，清空
 **/
/*
 * 参数类存储
 * 输入参数：oi对象标识，blockdata：主文件块缓冲区，seqnum:对象配置单元序列号，作为文件位置索引
 * 返回值：=1：文件保存成功，=0，文件保存失败，此时建议产生ERC2参数丢失事件通知主站异常
 * =-1:  未查找到OI类数据
 */
extern int saveParaClass(OI_698 oi,void *blockdata,int seqnum);
/*
 * 根据OI、配置序号读取某条配置单元内容
 * 输入参数：oi对象标识，seqnum:对象配置单元序列号
 * 返回值：
 * =1：文件读取成功，blockdata：配置单元内容
 * =0： 文件读取失败
 * =-1:  未查找到OI类数据
 */
extern int readParaClass(OI_698 oi,void *blockdata,int seqnum);
/*
 * 接口类公共属性读取
 * 输入参数：oi对象标识，dest：接口类公共属性
 * 返回值：
 * =1：文件读取成功   =0：文件读取失败   =-1:  未查找到OI类数据信息
 */
extern int readInterClass(OI_698 oi,void *dest);

/*
 * 通过配置序号删除配置单元
 * 输入参数：oi对象标识，seqnum:要删除的配置序号
 * 返回值：=1：配置单元删除成功
 * =-1:  未查找到OI类数据
 */
extern int delClassBySeq(OI_698 oi,void *blockdata,int seqnum);

//////////////////////////////////////////////////////////////////////////////////////////
/*		第三类文件：采用覆盖方式，根据每个oi的序号存储多个文件
 * 				事件参数、事件记录、当前记录集存储
 *		该类文件根据OI类型生成相应的目录  /nand/event/
 *		type: 参数类数据存储
 *
 **/
/*
 * 输入参数：	oi:对象标识，seqno:记录序号，blockdata:存储数据，len：存储长度，
 * 			type：存储类型【	根据宏定义SaveFile_type】
 * 返回值：=0：文件存储成功
 */
extern int saveCoverClass(OI_698 oi,INT16U seqno,void *blockdata,int savelen,int type);

/*
 * 输入参数：	oi:对象标识，seqno:记录序号，
 * 			type：存储类型【	根据宏定义SaveFile_type 】
 * 返回值：相关对象标识的类的存储文件长度
 * =-1: 无效数据
 * ！！！！！注意： 函数返回是文件的实际长度，参数文件中包含CRC校验字节
 * 				 如果是参数文件请慎用此函数
 */
extern int getClassFileLen(OI_698 oi,INT16U seqno,int type);
/*
 * 输入参数：	oi:对象标识，seqno:记录序号，blockdata:存储数据，datalen：存储长度，
 * 			type：存储类型【	根据宏定义SaveFile_type】
 * 返回值：=1：文件存储成功
 * =-1: 文件不存在
 */
extern int readCoverClass(OI_698 oi,INT16U seqno,void *blockdata,int datalen,int type);


//////////////////////////////////////////////////////////////////////////////////////////

/************************************
 * 函数说明：获取参数文件对象配置单元的个数
 * 返回值：
 * >=0:  单元个数
 * -1:  未查找到OI类数据
 * -2:	文件记录不完整
 *************************************/
extern long getFileRecordNum(OI_698 oi);

//////////////////////////////////////////////////////////////////////////////////////
//extern int write2_ProxyRequestList(PROXY_GETLIST *list);
//
//extern int read_ProxyRequestList(PROXY_GETLIST *list);
//////////////////////////////////////////////////////////////////////////////////////
///////////////变量数据类存储
/*
 * 变量数据存储及读取接口
 * oi: 需要存储OI值
 * blockdata:  需要存储数据, 存储格式为:　有效长度 + OAD + Data
 * datalen :   需要存储数据长度,不能超过64个字节
 * =-1 ：存储失败
 * */
extern int saveVariData(OI_698 oi,int coll_seqnum,void *blockdata,int datalen);

/*
 *　　读取数据值
 *　　　  oi: 需要读取的oi值的所有属性值
 *　　　　　blockdata:返回数据
 *　　　　　len:　blockdata空间大小，需要申请blockdata申请空间大小为：oad个数×VARI_LEN
 *　　　函数返回值：数据长度 =-1,读取失败
 * */
extern int  readVariData(OI_698 oi,int coll_seqnum,void *blockdata,int len);
//////////////////////////////////////////////////////////////////////////////////////
///////////////冻结类数据存储，目前针对统计数据
/*
 * 冻结数据记录单元存储
 * 每条记录数据内容固定64个字节：格式  OAD + 冻结时间 + Data
 * 返回 = 1： 写成功
 *     = 0： 失败
 * */
extern int	saveFreezeRecord(OI_698 freezeOI,OAD oad,DateTimeBCD datetime,int len,INT8U *data);
/*
 * 读取：冻结数据记录单元的最大数及当前记录数
 * 返回 currRecordNum：当前记录数
 * 		MaxRecordNum：冻结深度
 * */
extern int readFreezeRecordNum(OI_698 freezeOI,OI_698 relateOI,int *currRecordNum,int *MaxRecordNum);
/*
 * 冻结数据记录单元读取
 *     根据冻结记录序号
 * */
extern int readFreezeRecordByNum(OI_698 freezeOI,OAD oad,int RecordNum,DateTimeBCD *datetime,int *datalen,INT8U *data);
/*
 * 冻结数据记录单元读取
 *     根据冻结时标读取记录
 * */
extern int	readFreezeRecordByTime(OI_698 freezeOI,OAD oad,DateTimeBCD datetime,int *datalen,INT8U *data);

//////////////////////////////////////////////////////////////////////////////////////


///////////////数据文件存储

extern int getSelector(OAD oad_h,RSD select, INT8U selectype, CSD_ARRAYTYPE csds, INT8U *data, int *datalen);

extern long int readFrameDataFile(char *filename,int offset,INT8U *buf,int *datalen);
//extern void ReadFileHeadLen(FILE *fp,int *headlen,int *blocklen);
extern INT8U ReadTaskInfo(INT8U taskid,TASKSET_INFO *tasknor_info);
extern void getTaskFileName(INT8U taskid,TS ts,char *fname);
extern void getEveFileName(OI_698 eve_oi,char *fname);
extern INT8U datafile_write(char *FileName, void *source, int size, int offset);
extern INT8U datafile_read(char *FileName, void *source, int size, int offset);
extern INT16U CalcOIDataLen(OI_698 oi,INT8U attr_flg);
extern FILE* openFramefile(char *filename);
extern void saveOneFrame(INT8U *buf,int len,FILE *fp);
/*
 * 获取湖南规约切换通信参数
 * */
extern INT8U get_protocol_3761_tx_para();
/*
 * 湖南规约切换通信参数保存
 * */
extern int save_protocol_3761_tx_para(INT8U* dealdata);
/*
 * 重写3761 规约程序rc.local
 * */
extern INT8U write_3761_rc_local();
#endif /* ACCESS_H_ */
