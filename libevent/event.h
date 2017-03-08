#ifndef EVENT_H_
#define EVENT_H_

#include <time.h>
#include "StdDataType.h"
#include "EventObject.h"
#include "Shmem.h"
#include "Objectdef.h"

//标准记录单元结构体元素数量索引位置及值
#define STANDARD_NUM_INDEX 1           //事件记录单元元素数量索引
#define STANDARD_NUM 5                 //事件记录单元元素数量
#define STANDARD_NO_INDEX 2            //事件记录单元事件序号索引
#define STANDARD_HAPPENTIME_INDEX 7    //事件记录单元发生时间索引
#define STANDARD_ENDTIME_INDEX 15      //事件记录单元结束时间索引
#define STANDARD_SOURCE_INDEX 23       //事件记录单元事件发生源索引

#define DATA_FF 0xFF


#define ERC3106PATH "/nand/erc3106.bat"
//表正向有功
typedef struct
{
   TSA tsa;
   INT32U data;
   TS ts;
}Curr_Data;

//事件发生源枚举
typedef enum {
	s_null=0, //NULL 0
    s_tsa=85, //TSA 85
    s_oad=81, //OAD 81
    s_usigned=17, //unsigned 17
    s_enum=22, //enum 22
    s_oi=80 //OI 80
}Source_Typ;

#define POWER_START 0 //上电初始状态
#define POWER_ON 1 //上电状态
#define POWER_OFF 2 //停电状态
#define POWER_OFF_VALIDE 3 //停电事件有效
#define POWER_OFF_INVALIDE 4 //停电事件无效


/*
 * 根据参数读取事件记录文件
 * oi:事件oi eventno:0最新n某条 Getbuf空指针地址，动态分配 Getlen返回长度 prginfo_event共享内存
 */
extern INT8U Get_Event(OAD oad,INT8U eventno,INT8U** Getbuf,int *Getlen,ProgramInfo* prginfo_event);
/*
 * GETREQUESTRECORD selector9/10 获取事件记录中某一数据
 */
extern INT8U Getevent_Record_Selector(RESULT_RECORD *record_para,ProgramInfo* prginfo_event);
/*
 * 端初始化事件1 可以698规约解析actionrequest 调用该接口，data为OAD prginfo_event共享内存
 */
extern INT8U Event_3100(INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 终端版本变更事件 可直接调用，维护命令修改版本或者升级后可调用 prginfo_event共享内存
 */
extern INT8U Event_3101(INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 状态量变位事件 可直接调用 data为前后得ST CD，（1-4路）8个字节即可 prginfo_event共享内存
 */
extern INT8U Event_3104(INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 电能表时钟超差事件 tsa事件发生源 电表时钟 prginfo_event共享内存 taskno采集方案号
 */
extern INT8U Event_3105(TSA tsa, INT8U taskno,INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 终端停/上电事件5-停电事件-放在交采模块 prginfo_event共享内存
 */
extern INT8U Event_3106(ProgramInfo* prginfo_event,MeterPower *MeterPowerInfo,INT8U *state);
/*
 * 分析交采数据，产生对应的配置事件。
 */
extern INT8U Event_AnalyseACS(INT8U* data,INT8U len);
/*
 * 终端直流模拟量越上限事件6 data为直流模拟量 字节高到低 prginfo_event共享内存
 */
extern INT8U Event_3107(INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 终端直流模拟量越下限事件7 data为直流模拟量 字节高到低 prginfo_event共享内存
 */
extern INT8U Event_3108(INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 终端消息认证错误事件8 data事件发生前安全认证密码(不含数据类型) len为长度 prginfo_event共享内存
 */
extern INT8U Event_3109(INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 设备故障事件 errtype：0,1,2,3,4,5 prginfo_event共享内存
 */
extern INT8U Event_310A(MachineError_type errtype,ProgramInfo* prginfo_event);
/*
 * 电能表示度下降事件10 前台两次电能值对比是否超过设定值 prginfo_event共享内存 taskno采集方案号
 */
extern INT8U Event_310B(TSA tsa,  INT8U taskno,INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 电能量超差事件11 前台两次电能值以及测量点额定电压、电流 prginfo_event共享内存 meter当前测量点信息 taskno采集方案号
 */
extern INT8U Event_310C(TSA tsa, INT8U taskno, INT8U* data,INT8U len,ProgramInfo* prginfo_event,CLASS_6001 meter);
/*
 * 电能表飞走事件12 前台两次电能值以及测量点额定电压、电流 prginfo_event共享内存 meter当前测量点信息 taskno采集方案号
 */
extern INT8U Event_310D(TSA tsa, INT8U taskno,INT8U* data,INT8U len,ProgramInfo* prginfo_event,CLASS_6001 meter);
/*
 * 电能表停走事件 前台两次电能值是否相同以及时间差是否超过设定值 prginfo_event共享内存 taskno采集方案号
 */
extern INT8U Event_310E(TSA tsa, INT8U taskno,INT8U* data,INT8U len,ProgramInfo* prginfo_event) ;
/*
 * 抄表失败事件 抄表可自行判断是否抄表失败，可直接调用该接口 prginfo_event共享内存
 */
extern INT8U Event_310F(TSA tsa, INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 月通信流量超限事件 data为当月已经发生流量 字节由高到低 prginfo_event共享内存
 */
extern INT8U Event_3110(INT32U data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 发现未知电能表事件 抄表搜表可以判断出表信息，直接可调用该接口，默认data为整个电能表信息 prginfo_event共享内存
 */
extern INT8U Event_3111(TSA tsa, INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 跨台区电能表事件17 抄表搜表可以判断出表信息，直接可调用该接口，默认data为整个垮台区电能表信息 prginfo_event共享内存
 */
extern INT8U Event_3112(TSA tsa, INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 电能表在网状态切换事件24 怎么判断TODO？ data为电能表地址TSA及在网状态bool prginfo_event共享内存
 */
extern INT8U Event_311A(TSA tsa, INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 终端对电表校时记录 抄表是否可以自行判断是较时？可直接调用该接口 data为较时前电表时间及误差 prginfo_event共享内存
 */
extern INT8U Event_311B(TSA tsa, INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 电能表数据变更监控记录 抄表可自行判断，直接调用该函数。 prginfo_event共享内存
 */
extern INT8U Event_311C(TSA tsa, INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 终端对时事件 此接口在698规约库调用，data为对时前时间 date-time-s格式 7个字节 prginfo_event共享内存
 */
extern INT8U Event_3114(DateTimeBCD data,ProgramInfo* prginfo_event);
/*
 * 遥控跳闸记录 prginfo_event共享内存
 */
extern INT8U Event_3115(INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 有功总电能量差动越限事件记录 prginfo_event共享内存
 */
extern INT8U Event_3116(INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 输出回路接入状态变位事件记录 prginfo_event共享内存
 */
extern INT8U Event_3117(INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 终端编程记录 data为多个OAD集合，第一个字节为数量，后面是多个4个字节得OAD prginfo_event共享内存
 */
extern INT8U Event_3118(INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 电能表开盖事件 DATA记录，len长度
 */
INT8U Event_301B(INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 终端电流回路异常事件23,II型集中器没有电流，暂时不处理,type为0,1 短路、开路 prginfo_event共享内存
 */
extern INT8U Event_3119(INT8U type, INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 功控跳闸记录 data为事件源OI+事件发生后2分钟功率long64+控制对象OI+跳闸轮次bit-string(SIZE(8))+功控定值long64+跳闸前总有加有功功率23012300
 *  prginfo_event共享内存
 */
extern INT8U Event_3200(INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * prginfo_event共享内存
 * 电控跳闸记录 data为事件源OI+控制对象OI+跳闸轮次bit-string(SIZE(8))+电控定值long64+跳闸发生时总有加有功电量23014900array
 */
extern INT8U Event_3201(INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 购电参数设置记录29  prginfo_event共享内存
 */
extern INT8U Event_3202(INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 电控告警事件记录  data为事件源OI+控制对象OI+电控定值long64  prginfo_event共享内存
 */
extern INT8U Event_3203(INT8U* data,INT8U len,ProgramInfo* prginfo_event);
/*
 * 698guiyue规约库判断初始化事件、终端对时事件
 */
extern void  Get698_event(OAD oad,ProgramInfo* prginfo_event);
#endif
