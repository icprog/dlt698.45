#ifndef STDDATATYPE_H_
#define STDDATATYPE_H_

#include "ParaDef.h"
//#pragma pack(1)				//结构体一个字节对齐
typedef unsigned char  BOOLEAN;
typedef unsigned char  INT8U;                    /* Unsigned  8 bit quantity                           */
typedef signed   char  INT8S;                    /* Signed    8 bit quantity                           */
typedef unsigned short INT16U;                   /* Unsigned 16 bit quantity                           */
typedef signed   short INT16S;                   /* Signed   16 bit quantity                           */
typedef unsigned int   INT32U;                   /* Unsigned 32 bit quantity                           */
typedef signed   int   INT32S;                   /* Signed   32 bit quantity                           */
typedef unsigned long long     	INT64U;          /* Unsigned 64 bit quantity   						   */
typedef signed long long  		INT64S;          /* Unsigned 64 bit quantity                           */
typedef float          FP32;                     /* Single precision floating point                    */
typedef double         FP64;                     /* Double precision floating point                    */

#define TRUE 1
#define FALSE !TRUE

//698.45扩展数据类型-------------------------------------
typedef unsigned char MAC_698;	//数据安全MAC
typedef unsigned char RN_698;	//随机数
typedef unsigned short OI_698;	//对象标识

//////////////////////////////////////////////////////////////////
/*
 * 			枚举型参数定义区
 * */
//顺序
typedef enum { positive,/*正序*/inverted //倒序
} ORDER;

typedef enum{					//该子程序运行状态
	None=0,						//标志无程序运行
	NeedStart,					//该程序需要运行
	NeedKill,					//该程序需要停止运行
	NowRun,						//目前该程序正在运行
	NeedStop					//进程需要停止
}ProjectState;					//程序运行状态

/*
 * 			CJ/T698.45 规约枚举类型定义区
 * */
typedef enum {
	build_connection/*建立连接*/,
	heart_beat,
	close_connection/*断开连接*/
}Link_Request_type;	/*连接请求类型*/

/*698基本数据类型*/
typedef enum {
  dtnull=0,dtarray=1,dtstructure=2,dtbool=3,dtbitstring=4,dtdoublelong=5,dtdoublelongunsigned=6,
  dtoctetstring=9,dtvisiblestring=0x0a,dtutf8string=0x0c,dtinteger=0x0f,dtlong=0x10,dtunsigned=0x11,
  dtlongunsigned=0x12,dtlong64=0x14,dtlong64unsigned=0x15,dtenum=0x16,dtfloat32=0x17,dtfloat64=0x18,
  dtdatetime=0x19,dtdate=0x1a,dttime=0x1b,dtdatetimes=0x1c,dtoi=0x50,dtoad=0x51,dtroad=0x52,dtomd=0x53,
  dtti=0x54,dttsa=0x55,dtmac=0x56,dtrn=0x57,dtregion=0x58,dtscalerunit=0x59,dtrsd=0x5a,dtcsd=0x5b,dtms=0x5c,
  dtsid=0x5d,dtsidmac=0x5e,dtcomdcb=0x5f,dtrcsd=0x60
}Base_DataType;

typedef enum {
	bps300,bps600,bps1200,bps2400,bps4800,bps7200,bps9600,bps19200,bps38400,bps57600,bps115200,autoa
}Baud_Rate;	/*波特率*/
typedef enum {
	none,odd,even
}Verify_Type;	/*校验类型*/
typedef enum {
	d0,d1,d2,d3,d4,d5,d6,d7,d8
}DataBits;	/*数据位*/
typedef enum {
	stop0,stop1,stop2
}StopBits;	/*停止位*/
typedef enum {
	no,hard,soft
}FlowControl;	/*流控*/

typedef enum {
	sec_units = 0,minute_units = 1,hour_units = 2,day_units = 3,month_units = 4,year_units = 5
}Time_Units;	/*时间单位*/

typedef enum {
	close_open, open_close, close_close, open_open,interface
}Region_Type;	/*区间类型*/
typedef enum {
	null,a,mo,wk,d,h,min,s,o,C,huobi,m,ms,m3_0,m3_a,m3h1,m3h2,m3d1,m3d2,l,kg,N,Nm,
	Unit_P,Unit_bar,Unit_J,Unit_Jh,Unit_W,Unit_kW,Unit_VA,Unit_kVA,Unit_var,
	Unit_Kvar,Unit_kWh,Unit_kVAh,Unit_Kvarh,Unit_A,Unit_C,
	Unit_V,Unit_Vm,Unit_F,Unit_O,Unit_Om2,Unit_Wb,Unit_T,Unit_Am,
	Unit_H,Unit_Hz,Unit_lWh,Unit_impkWh,Unit_impvarh,
	VAh,baifenbi,byte,dBm,yuankWh,Ah
}Physical_Units;/*物理单位*/

typedef enum {
	success/*0成功*/,
	hdw_disable/*1硬件失效*/,					temporal_disable/*2暂时失效*/,				refuse_rw/*3拒绝读写*/,
	obj_undefine/*4对象未定义*/,				interface_uncomp/*5对象接口类不符合*/,			obj_unexist/*6对象不存在*/,
	type_mismatch/*7类型不匹配*/,				boundry_over/*8越界*/,						dblock_invalid/*9数据块不可用*/,
	framesegment_cancel/*10分帧传输已取消*/,	framesegment_invalid_state/*11不处于分帧传输状态*/,
	wdblock_cancel/*12块写取消*/,				no_wdblock_state/*13不存在块写状态*/,			dblock_invalid_serial/*14数据块序号无效*/,
	pwd_err1/*15密码错未授权*/,				comm_rate_disnablechg/*16通信速率不能更改*/,	year_zonenum_over/*17年时区数超*/,
	day_zonenum_over/*18日时段数超*/,			feilvnum_over/*19费率数超*/,					security_mismatch/*20安全认证不匹配*/,
	recharge_reuse/*21重复充值*/,				esam_verify_fail/*22ESAM 验证失败*/,			security_fail,/*23安全认证失败*/
	customer_code_mismatch/*24客户编号不匹配*/,recharge_counter_err/*25充值次数错误*/,		buypower_over/*26购电超囤积*/,
	addr_exception/*27地址异常*/,				symmetric_decryption_err/*28对称解密错误*/,	unsymmetric_decryption_err/*29非对称解密错误*/,
	signature_err1/*30签名错误*/,				meter_suspend/*31电表挂起*/,					timetag_invalid/*32时间标签无效*/,
	request_overtime/*33请求超时*/,			esam_p1p2_err/*34 ESAM的P1P2不正确*/,			esam_lc_err/*ESAM的LC错误*/,
	other_err1=255/*其他*/
}DAR;	/*数据访问结果*/

typedef enum {
	allow/*允许建立应用连接*/,
	pwd_err2/*密码错误*/,
	symm_decryption_err/*对称解密错误*/,
	asymm_decryption_err/*非对称解密错误*/,
	signature_err2/*签名错误*/,
	protocol_ver_err/*协议版本错误*/,
	other_err2/*其它错误*/
}ConnectResult;	/*应用连接请求认证的结果*/
//6013
typedef enum {
	norm=1/*普通采集方案*/,
	events=2/*事件采集方案*/,
	tran=3/*透明采集方案*/,
	rept=4/*上报方案*/,
	scpt=5/*脚本方案*/
}SCHM_TYPE;//方案类型

typedef enum {
	first=1/*首要*/,
	ness=2/*必要*/,
	need=3/*需要*/,
	poss=4/*可能*/
}RUN_PRIO;//执行优先级

typedef enum {
	E_S=0/*东、南*/,
	W_N=1/*西、北*/,
	OTHERSTATUS=255
}LNG_LAT;//经纬度方位

typedef enum {
	 task_valid=1/*正常*/,
	 task_novalid=2/*停用*/
}TASK_VALID;//任务状态

typedef enum {
	B_K=0/*前闭后开*/,
	K_B=1/*前开后闭*/,
	B_B=2/*前闭后闭*/,
	K_K=3/*前开后开*/
}RUN_TIME_TYPE;//运行时段类型

typedef enum {
	BEFORE_OPR=0/*未执行*/,
	IN_OPR=1/*执行中*/,
	AFTER_OPR=2/*已执行*/,
}TASK_STATE;//任务执行状态

typedef enum {
	MAIN=0,/*主站授时*/
	TERMINAL=1,/*终端精确校时*/
	GPS=2,/*北斗/GPS*/
	OTHER=255/*其它*/
}OI4000_TYPE;//校时模式

typedef enum {
	TYPE_NULL=0/*采集当前数据*/,
	TYPE_LAST=1/*采集上N次*/,
	TYPE_FREEZE=2/*按冻结时标*/,
	TYPE_INTERVAL=3/*按时标间隔*/
}CJTYPE_6015;//任务执行状态

typedef enum {
	Terminal_File=0,		//终端文件          （0），
	Online_Module_File=1,	//本地通信模块文件  （1），
	Remote_Module_File=2,	//远程通信模块文件  （2），
	Collector_File=3,	 	//采集器文件        （3），
	Sub_node_Module_File=4,	//从节点通信模块文件（4），
	OtherFile=255,			//其它文件          (255)
}FILE_Type;	/*文件类型*/

//////////////////////////////////////////////////////////////////
//typedef struct
//{
//	INT8U type;
//	INT8U value;
//}BOOL_TYPE
//typedef struct
//{
//	INT8U type;
//	INT8U value[16];
//}BITSTRING_TYPE


/////////////////////////////////////////////////////////////////
typedef struct
{
	INT8U type;
	INT8U data[50];
}DATA_TYPE;

typedef struct
{
	INT16U Year;    //year;
    INT8U  Month;   //month;
    INT8U  Day;     //day;
    INT8U  Hour;    //hour;
    INT8U  Minute;  //minute;
    INT8U  Sec;     //second;
    INT8U  Week;
}TS;


typedef union {//frame's length
	INT16U u16b;//convenient to set value to 0
	struct {//only for little endian frame!
		INT16U len	: 14;//length
		INT16U rev	: 2;//reserve
	} length;
} lengthUN;

typedef union {//control code
	INT8U u8b;//convenient to set value to 0
	struct {//only for little endian mathine!
		INT8U func		: 3;//function code
		INT8U rev		: 2;//reserve
		INT8U divS		: 1;//dived frame flag
		INT8U prm		: 1;//promote flag
		INT8U dir		: 1;//direction flag
	} ctl;
} ctlUN;

typedef struct
{
	INT8U sa_type;		//服务器地址类型	0:单地址   1:通配地址   2：组地址   3：广播地址
	INT8U sa_length;	//服务器地址长度   0～15 表示 1～16
	INT8U funcode;		//功能码标识		1：链路管理 3：用户数据
	INT8U dir;			//方向指示		0：客户机发出  1：服务器发出
	INT8U prm;			//启动标识		1：客户机发起  0：服务器发起
	INT8U gframeflg;	//分帧			1：表示APDU片段  0：完整APDU
	INT8U sa[OCTET_STRING_LEN];		//服务器地址		服务器地址
	INT8U ca;			//客户机地址		0:不关注客户机地址
	INT16U frame_length;//帧长度
}CSINFO;

typedef struct
{
	INT8U data;
}ComBCD2;
typedef struct
{
	INT16U data;
}ComBCD4;
typedef struct
{
	INT8U data1;
	INT8U data2;
	INT8U data3;
}ComBCD6;
typedef struct
{
	INT8U data1;
	INT8U data2;
	INT8U data3;
	INT8U data4;
}ComBCD8;//053283069933
typedef struct//12：30       1：30    //  18305356699  /     85     // 128
{
	ComBCD4 year;
	ComBCD2 month;
	ComBCD2 day;
	ComBCD2 hour;
	ComBCD2 min;
	ComBCD2 sec;
}DateTimeBCD;
typedef struct
{
	DateTimeBCD datetime;
	INT16U msec;
}DateTimeBCD_H;

typedef struct
{
	ComBCD4 year;
	ComBCD2 month;
	ComBCD2 day;
	ComBCD2 hour;
	ComBCD2 min;
}DateTimeBCD_S;

typedef struct
{
	ComBCD4 year;
	ComBCD2 month;
	ComBCD2 day;
}DateBCD;
typedef struct
{
	ComBCD2 hour;
	ComBCD2 min;
	ComBCD2 sec;
}TimeBCD;
typedef struct
{
	INT8U data;
}PIID;

typedef struct
{
	INT8U 	DAR;		//返回 DAR结果
	int		datalen;	//返回 数据帧长度字节
}Action_result;

typedef struct
{
	INT8U data;
}PIID_ACD;
typedef struct
{
	OI_698 OI;
	INT8U attflg;		/*属性标识及其特征	取值 0...31  0:全部属性*/
	INT8U attrindex;	/*属性内元素索引 unsigned(1...255)*/
}OAD;
typedef struct
{
	OAD oad;
	INT8U num;
	OAD oads[ROAD_OADS_NUM];
}ROAD;					/*记录型对象属性描述符*/

typedef struct
{
	INT8U 	units;
	INT16U  interval;
}TI;					/*时间间隔数据类型*/
typedef struct
{
	INT8U		effect;		//有效
	INT8U		flag;		//时间标签标记
	DateTimeBCD sendTimeTag;
	TI ti;
}TimeTag;				/*时间标签*/


typedef struct
{
	INT8U addr[TSA_LEN];	// addr[0] :array num  addr[1]: TSA_len(后面数据帧为tsa_len+1) addr[2]...addr[2+TSA_len+1]
}TSA;
typedef struct
{
	Region_Type type;			////type = interface 无效
	INT8U  begin[REGION_NUM];	//[1]:数据类型  [2-n]：实际数据
	INT8U  end[REGION_NUM];
}Region;

typedef union
{
	INT8U nometer_null;						//0
	INT8U allmeter_null;					//1
	INT8U userType[COLLCLASS_MAXNUM];		//2：userType[0][1]:表示后面有效的序列个数,[0]:长度高字节,[1]:长度低字节
	TSA	userAddr[COLLCLASS_MAXNUM];			//3：userAddr[0].addr[0][1]:表示后面的有效TSA个数,同上先高后低
	INT16U configSerial[COLLCLASS_MAXNUM];	//4：configSerial[0]:表示后面有效的序列个数
	Region type[COLLCLASS_MAXNUM];			//5：一组用户类型区间
	Region addr[COLLCLASS_MAXNUM];			//6：一组用户地址区间
	Region serial[COLLCLASS_MAXNUM];		//7：一组配置序号区间
}MS;

typedef struct
{
	INT8U mstype;
	MS ms;
}MY_MS;

typedef struct
{
	INT8U conver;
	Physical_Units units;
}Scaler_Unit;

typedef struct
{
	INT8U sig[4];
	INT8U addition[20];/*(长度(1byte)+信息(n byte))*/
}SID;	/*安全标识*/

typedef struct
{
	SID sid;
	INT8U mac[10];/*正常4byte   (长度(1byte)+信息(n byte))*/
}SID_MAC;	/*安全标识*/


typedef struct
{
	OAD oad;
	DATA_TYPE data;
}Selector1;
typedef struct
{
	OAD oad;
	DATA_TYPE data_from;
	DATA_TYPE data_to;
	DATA_TYPE data_jiange;
}Selector2;
typedef struct
{
	INT8U	sel2_num;
	Selector2 selectors[SELECTOR3_NUM];
}Selector3;
typedef struct
{
	DateTimeBCD collect_star;
	MY_MS meters;
}Selector4;		/*指定电能表集合、指定采集启动时间*/
typedef struct
{
	DateTimeBCD collect_save;
	MY_MS meters;
}Selector5;		/*指定电能表集合、指定采集存储时间*/
typedef struct
{
	DateTimeBCD collect_star;
	DateTimeBCD collect_finish;
	TI ti;
	MY_MS meters;
}Selector6;		/*指定电能表集合、指定采集启动时间区间内连续*/
typedef struct
{
	DateTimeBCD collect_save_star;
	DateTimeBCD collect_save_finish;
	TI ti;
	MY_MS meters;
}Selector7;		/*指定电能表集合、指定采集存储时间区间内连续*/
typedef struct
{
	DateTimeBCD collect_succ_star;
	DateTimeBCD collect_succ_finish;
	TI ti;
	MY_MS meters;
}Selector8;		/*指定电能表集合、指定采集到时间区间内连续间隔值*/
typedef struct
{
	INT8U recordn;
}Selector9;		/*指定选取上第n次记录*/
typedef struct
{
	INT8U recordn;
	MY_MS meters;
}Selector10;	/*指定选取最新的n条记录*/

typedef union
{
	INT8U	null;
	Selector1 selec1;
	Selector2 selec2;
	Selector3 selec3;
	Selector4 selec4;
	Selector5 selec5;
	Selector6 selec6;
	Selector7 selec7;
	Selector8 selec8;
	Selector9 selec9;
	Selector10 selec10;
}RSD;
typedef union
{
	OAD  oad;	/*对象属性描述符*/
	ROAD road;	/*记录型对象属性描述符*/
}CSD;	/*列选择描述符*/
typedef struct
{
	INT8U type;
	CSD csd;
}MY_CSD;
typedef struct
{
	INT8U flag;			//解析时的标记，在应用程序中无意义
	INT8U num;
	MY_CSD csd[MY_CSD_NUM];//CSD
}CSD_ARRAYTYPE;

typedef struct
{
	CSD_ARRAYTYPE csds;	//csd数组
}RCSD;	/*记录列选择描述符*/
typedef struct{
	INT8U flag;
	INT8U num;
	ROAD road[ARRAY_ROAD_NUM];
}ARRAY_ROAD;

typedef struct
{
	INT8U encrypted_code1;
	INT8U signature;
}SymmetrySecurity;
typedef struct
{
	INT8U encrypted_code2[40];/*密文2,正常32字节(长度(1byte)+信息(32byte))*/
	INT8U signature[70];/*正常64字节,(长度(1byte)+信息(64byte))*/
}SignatureSecurity;
typedef union
{
	INT8U NullSecurity;
	INT8U PasswordSecurity[16];
	SymmetrySecurity  symsecur;
	SignatureSecurity sigsecur;
}ConnectMechanismInfo;
typedef struct
{
	INT8U type;
	ConnectMechanismInfo info;
}ConnectMechanismInfo_type;
typedef struct
{
	RN_698 server_rn[50];			/*服务器随机数*/
	INT8U  server_signInfo[70];		/*服务器签名信息*/
}SecurityData;/*认证附加信息*/

typedef struct
{
	ConnectResult result; 		/*认证结果*/
	SecurityData addinfo;		/*认证附加信息*/
}ConnectResponseInfo;/*应用连接请求认证响应信息*/

typedef struct
{
	INT16U year;
	INT8U month;
	INT8U day_of_month;
	INT8U day_of_week;
	INT8U hour;
	INT8U minute;
	INT8U second;
	INT16U milliseconds;
}date_time;/*日期时间*/
typedef struct
{
	INT16U year;
	INT8U month;
	INT8U day_of_month;
	INT8U day_of_week;
}date;/*日期*/
typedef struct
{
	INT8U hour;
	INT8U minute;
	INT8U second;
	INT16U milliseconds;
}time698;/*时间*/
typedef struct
{
	Baud_Rate baud;
	Verify_Type verify;
	DataBits databits;
	StopBits stopbits;
	FlowControl flow;
}COMDCB;/*串口控制块*/

typedef union {
	INT8U data_null;
//	DataType data_array[10];
//	DataType data_struct[10];
	INT8U data_bool;
//	INT8U data_bit[180];
	INT32U double_long;
	INT32U double_long_unsigned;
	INT8U octet_string[180];
	INT8U visible_string[180];
	INT8U utf8_string[180];
	INT8U bcd[180];
	INT8U integer;
	INT16U data_long;
	INT8U dataunsigned;
	INT16U long_unsigned;
	INT64U long64;
	INT64U long64_unsigned;
	INT8U  data_enum;
	INT32S  float32;
	INT64S float64;
	INT8U data_datetime[10];
	INT8U data_date[5];
	INT8U data_time[3];
	DateTimeBCD datetime;
	DateTimeBCD_H datetime_h;
	DateTimeBCD_S datetime_s;
	DateBCD datebcd;
	TimeBCD timebcd;
	OI_698 OI;
	OAD oad;
	ROAD road;
	TI ti;
	TSA tsa;
	INT8U mac[20];
	INT8U rn[20];
	Region region;
	Scaler_Unit scalerunit;
	RSD rsd;
	CSD csd;
//	MS ms;
	SID sid;
	SID_MAC sidmac;
	COMDCB comdcb;
}DataBuf;
//typedef union {
//	INT8U data_null;
////	DataType data_array[10];
////	DataType data_struct[10];
//	INT8U data_bool;
//	INT8U data_bit[180];
//	INT32U double_long;
//	INT32U double_long_unsigned;
//	INT8U octet_string[180];
//	INT8U visible_string[180];
//	INT8U utf8_string[180];
//	INT8U bcd[180];
//	INT8U integer;
//	INT16U data_long;
//	INT8U dataunsigned;
//	INT16U long_unsigned;
//	INT64U long64;
//	INT64U long64_unsigned;
//	INT8U  data_enum;
//	INT32S  float32;
//	INT64S float64;
//	INT8U data_datetime[10];
//	INT8U data_date[5];
//	INT8U data_time[3];
//	DateTimeBCD datetime;
//	DateTimeBCD_H datetime_h;
//	DateTimeBCD_S datetime_s;
//	DateBCD datebcd;
//	TimeBCD timebcd;
//	OI_698 OI;
//	OAD oad;
//	ROAD road;
//	OMD omd;
//	TI ti;
//	TSA tsa;
//	INT8U mac[20];
//	INT8U rn[20];
//	Region region;
//	Scaler_Unit scalerunit;
//	RSD rsd;
//	CSD csd;
//	MS ms;
//	SID sid;
//	SID_MAC sidmac;
//	COMDCB comdcb;
//}DataType;
//DataType len=1432


typedef struct
{
	INT8U factorycode[4];
	INT8U software_ver[4];
	INT8U software_date[6];
	INT8U hardware_ver[4];
	INT8U hardware_date[6];
	INT8U additioninfo[8];
}FactoryVersion;
typedef struct
{
	INT8U data[8];	//64位
}ProtocolConformance;

//--------------------------------------------------------------------------------------
typedef union
{
	INT8U request;
	INT8U response;
}LINK_APDU;/*预连接协议数据单元*/

typedef struct
{
	INT8U 	client_request_type;	/*客户机应用层请求类型*/
	TimeTag timeTag;				/*optional*/
}Client_APDU;/*客户机应用层协议数据单元*/
typedef struct
{
	INT8U 	service_response_type;	/*客户机应用层请求类型*/
	INT8U   followReport[100];		/*optional：跟随上报信息域*/
	TimeTag timeTag;				/*optional*/
}Server_APDU;/*服务器应用层协议数据单元*/
typedef struct
{
	PIID_ACD piid_acd;
	INT8U type;
	INT16U heartbeat;
	date_time time;
}LINK_Request;/*预连接请求数据类型*/
typedef struct
{
	PIID piid;
	INT8U result;
	date_time request_time;
	date_time reached_time;
	date_time response_time;
}LINK_Response;/*预连接响应数据类型*/

typedef struct
{
	PIID piid;
	ComBCD4 expect_app_ver;			//期望的应用层协议版本号
	INT8U ProtocolConformance[8];	//期望的协议一致性块
	INT8U FunctionConformance[16];	//期望的功能一致性块
	INT16U client_send_size;		//客户机发送帧最大尺寸
	INT16U client_recv_size;		//客户机接收帧最大尺寸
	INT8U client_recv_maxWindow;	//客户机接收帧最大窗口尺寸
	INT16U client_deal_maxApdu;		//客户机最大可处理APDU尺寸
	INT32U expect_connect_timeout;	//期望的应用连接超时时间
	INT8U connecttype;				//认证机制信息
	ConnectMechanismInfo info;
}CONNECT_Request;
typedef struct
{
	PIID piid_acd;
	FactoryVersion server_factory_version;	//服务器厂商版本信息
	ComBCD4 app_version;
	INT8U ProtocolConformance[8];		//协议一致性协商
	INT8U FunctionConformance[16];		//功能一致性协商
	INT16U server_send_size;			//服务器发送帧最大尺寸
	INT16U server_recv_size;			//服务器接收帧最大尺寸
	INT8U server_recv_maxWindow;		//服务器接收帧最大窗口尺寸
	INT16U server_deal_maxApdu;			//服务器最大可处理ＡＰＤＵ尺寸
	INT32U expect_connect_timeout;		//商定的应用连接超时时间
	ConnectResponseInfo info;
}CONNECT_Response;

typedef struct
{
	PIID piid;
}RELEASE_Request;
typedef struct
{
	PIID_ACD piid_acd;
	INT8U succ;
}RELEASE_Response;
typedef struct
{
	PIID_ACD piid_acd;
	DateTimeBCD connect_build_time;
	DateTimeBCD server_time;
}RELEASE_Notification;

typedef struct
{
	INT8U type;
	PIID piid;
}SET_Request;

typedef struct
{
	INT16U sernum;
	TSA  addr;
	INT8U baud;
	INT8U protocol;
	OAD port;

	INT8U usertype;
	INT8U connectype;
	TSA cjqaddr;
}MeterInfoUnit;

typedef struct{
	OI_698 oi;
	INT8U report_flag; //1需上报 0已上报
	INT16U eventno;
}Reportevent;

typedef struct{
	INT8U event_num;
	Reportevent report_event[15];
}NeedReport_Event;

//消息命令
typedef enum {
	TERMINALEVENT_REPORT = 0,   //0
	METEREVENT_REPORT = 1,   //0
	TERMINALPROXY_RESPONSE = 3,//3
}gdm_type;

#endif
