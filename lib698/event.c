#include "event.h"
#include "EventObject.h"

static TSA tsa_list;
static tsa_list_num;
#if 0
/*
 * 分析抄表存储的报文，输入任务id和抄读对象地址。
 */
INT8U Event_AnalyseData(INT8U * buf, INT32U id, TSA tsa){
	//存储事件产生所需要的判据
	INT8U data[64];
	memset(data,0,64);


	//寻找已知表号，并对这个产生未知电表事件
	if( Event_FindTsa( tsa)){
		 Event_3111 ( tsa,  data);    //发现未知电能表事件16
		 Event_3112 ( tsa,  data);    //跨台区电能表事件17
		 Event_311A ( tsa,  data); //电能表在网状态切换事件24
	}


	//找正向有功数据
	if(Event_FindOAD(buf, 0x0010, data)){
		 Event_310B ( tsa,  data); //电能表示度下降事件10
		 Event_310C ( tsa,  data); //电能量超差事件11
		 Event_310D ( tsa,  data); //电能表飞走事件12
		 Event_310E ( tsa,  data); //电能表停走事件13
	}

	//找电表校时数据
	if(Event_FindOAD(buf, 0x4000, data)){
		 Event_3105( tsa,  data); //电能表时钟超差事件4
	}

return 1;
}
#endif
/*
 * 输入报文和想要找到的OAD，将结果数据放入data中，返回1为成功，返回0为失败。
 */
INT8U Event_FindOAD(INT8U * buf, OI_698 oad, INT8U * data){

}

/*
 * 寻找已知表号，并对这个产生未知电表事件
 */
INT8U Event_FindTsa(TSA tsa){

}

INT8U Event_3105(TSA tsa, INT8U * data); //电能表时钟超差事件4
INT8U Event_310A (INT8U type, INT8U);    //设备故障记录9
INT8U Event_310B (TSA tsa, INT8U * data); //电能表示度下降事件10
INT8U Event_310C (TSA tsa, INT8U * data); //电能量超差事件11
INT8U Event_310D (TSA tsa, INT8U * data); //电能表飞走事件12
INT8U Event_310E (TSA tsa, INT8U * data); //电能表停走事件13
INT8U Event_310F (TSA tsa, INT8U * data); //终端抄表失败事件14
INT8U Event_3111 (TSA tsa, INT8U * data);    //发现未知电能表事件16
INT8U Event_3112 (TSA tsa, INT8U * data);    //跨台区电能表事件17
INT8U Event_311A (TSA tsa, INT8U * data); //电能表在网状态切换事件24
INT8U Event_311B (TSA tsa, INT8U * data);    //终端对电表校时记录25
INT8U Event_311C (TSA tsa, INT8U * data); //电能表数据变更监控记录26

/*
 * 分析下行报文，产生对应的配置事件。
 */
INT8U Event_AnalyseMsg(INT8U * data);

INT8U Event_3100 (INT8U * data);    //终端初始化事件1
INT8U Event_3109 (INT8U * data);    //终端消息认证错误事件8
INT8U Event_3110 (INT8U * data); //月通信流量超限事件15
INT8U Event_3114 (INT8U * data);    //终端对时事件18
INT8U Event_3202 (INT8U * data);   //购电参数设置记录29


INT8U Event_3202_1 (INT8U * data); //终端停/上电事件5-停电事件-放在交采模块
INT8U Event_3202_2 (INT8U * data); //终端停/上电事件5-上电事件-放在交采模块-发起抄表动作
INT8U Event_3202_3 (INT8U * data); //终端停/上电事件5-判定事件-放在停电抄表模块，判定停电有效性
INT8U Event_3202_clean (INT8U * data); //终端停/上电事件5-放在轻量级轮训模块，用于处理停电事件抄表未回情况

/*
 * 分析交采数据，产生对应的配置事件。
 */

INT8U Event_AnalyseACS(INT8U * data);

INT8U Event_3107 (INT8U * data); //终端直流模拟量越上限事件6
INT8U Event_3108 (INT8U * data); //终端直流模拟量越下限事件7
INT8U Event_3119 (INT8U * data);  //终端电流回路异常事件23
