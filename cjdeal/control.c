/*
 * control.c
 *
 *  Created on: 2017-3-8
 *      Author: xxh
 *  functional description:功能描述
 *  	主要完成III型专变终端的控制功能；
 *  	1、
 */
#include "control.h"
/*
 * 控制本单元要用的到的变量，先定义到这里，因为没有共享内存，所以在control.h定义的一些698规约的变量暂时当成
 * 共享内存来用，或是以后在文件中读取；
 * */

INT64S test_baoandingzhi;

/**/
CTR_OBJECT_CLASS	ctr_object_class;
SUMGROUP_INTERFACE_CLASS_23	 sumgroup_interface_class_23[MAXNUM_SUMGROUP];
CONTROL_VAR Outpar;	//输出参数变量
CONTROL_VAR Outpar_old;	////输出参数变量
/*函数声明*/
void* ControlFun();
/*打开控制模块串口*/
void OpenSerialPort() {
	char *parity = "even";
	serial_fd = OpenCom(5, 19200, (unsigned char*) parity, 1, 8);
	if (serial_fd <= 0) {
		fprintf(stderr, "Open Serial Port Failed-%d!\r\n", serial_fd);
	}
}
/*
 *检测模块是否已插入
 */
int CheckModelState() {
	//读取模块状态，查询模块是否已经插入
	char* ZB_STATE = "/dev/gpiZB_STATE";
	if (1 == gpio_readbyte((char*) ZB_STATE)) {
		perror("State of Ctrl Module is 1!\r\n");
		return 0;
	}
	return 1;
}
/*初始化控制模块
 */
int InitCtrlModel() {

	if (CheckModelState() == 0)
		return 0;

	//复位控制模块，低电平有效
	char* ZB_RST = "/dev/gpoZAIBO_RST";
	if (gpio_writebyte((char*) ZB_RST, 0) < 0) {
		perror("Ctrl Module RST ERR!\r\n");
		return 0;
	}

	if (gpio_writebyte((char*) ZB_RST, 1) < 0) {
		perror("Ctrl Module RST ERR!\r\n");
		return 0;
	}

	//控制模块使能，高电平有效
	char* CTL_EN = "/dev/gpoCTL_EN";
	if (gpio_writebyte((char*) CTL_EN, 1) < 0) {
		perror("Ctrl Module CTLEN ERR!\r\n");
		return 0;
	}

	//复位后，至少需要60多毫秒，控制模块才能正常使用
	usleep(80000);
	fprintf(stderr,"\nvctrl 初始化模块完成");
	return 1;
}
void init_crt_para(CTR_OBJECT_CLASS *ctr_object_class)
{
	int i,j;
	for(i=0;i<MAXNUM_SUMGROUP;i++)
	{
		for(j=0;j<48;j++)
		{
//			ctr_object_class->ShiDuankong[i].gk_shiduan[j].begin =j*30;
//			ctr_object_class->ShiDuankong[i].gk_shiduan[j].end= j*30+30;
		}
	}

//	if((par->Yaokong.Lunci[0].valid & VALID) != VALID ) par->Yaokong.Lunci[0].state = HE;  //0c f6中，遥控取反，此处需初始化
//	if((par->Yaokong.Lunci[1].valid & VALID) != VALID ) par->Yaokong.Lunci[1].state = HE;
//	memset(&Outpar_old,0xff,sizeof(Outpar_old));
}
void init_CtrlState(CONTROL_VAR *out)
{
	int i,j,tou[2];
	tou[0] = tou[1]=0;
	out->lun1_state  = HE;			//取消动作
	out->lun1_red 	 = RED_CLOSE;	//熄灭
	if (tou[0]==1)
		out->lun1_green  = GREEN_LIGHT;
	else
		out->lun1_green  = GREEN_CLOSE;	//熄灭

	out->lun2_state  = HE;			//取消动作
	out->lun2_red 	 = RED_CLOSE;	//熄灭

	if (tou[1]==1)
		out->lun2_green  = GREEN_LIGHT;
	else
		out->lun2_green  = GREEN_CLOSE;	//熄灭

	out->gongk_led 	 = RED_CLOSE;	//功控灯熄灭
	out->diank_led 	 = RED_CLOSE;	//电控灯熄灭
	out->alm_state	 = 0x01;	//告警取消
//	gpio_writebyte((INT8S *)DEV_ALARM_BUZZER,0x00);///dev/gpoBUZZER
	out->baodian_led = RED_CLOSE;	//保电灯熄灭
}
/*获取控制参数*/
void GetPar_general()
{
	int i=0;
	//功控轮次设定
	for(i=0;i<MAXNUM_SUMGROUP;i++)
	{
		sumgroup_interface_class_23[i].gongkong_lunci = 1;
	}

	//保安定值
	test_baoandingzhi = ctr_object_class.baoan_value_8100/10;
	//功率控制的功率计算滑差时间
	for(i=0;i<MAXNUM_SUMGROUP;i++)
	{
		sumgroup_interface_class_23[i].period=1;
	}
	//电控轮次设定
	for(i=0;i<MAXNUM_SUMGROUP;i++)
	{
		sumgroup_interface_class_23[i].diankong_lunci=1;
	}
	//功控轮次告警时间
}
void GetPar_shiduan()
{

}
void GetPar_changxiu()
{

}
void Getpar_xiafu()
{

}
void Getpar_yingye()
{}
void Getpar_goudian()
{}
void Getpar_yuedian()
{}
void Getpar_cuifei()
{}
//获得参数状态-----接口留好没有进行编程
void GetPara(CTR_OBJECT_CLASS *ctr_object_class)
{
//	GetPara(CTRPARA* par,para_all* fnpar)

}
/*
 * //5个状态灯变化函数(报警添加进入参数下发报警)
 * 根据当前功控电控遥控状态标识，确定输出5种状态灯输出具体状态。
 * 除保电外，原则上不允许直接修改输出状态，需要通过该函数判断各个控制状态位，来确定输出状态。
 */
void State_Led()
{

}
//比对控制变量的状态，根据状态返回不同的值
int cmpvar()
{
//	CONTROL_VAR var0,CONTROL_VAR var1
}
void createdthread()
{
	pthread_attr_t att;
	pthread_attr_init(&att);
	pthread_attr_setstacksize(&att,2048*1024);
	pthread_attr_setdetachstate(&att,PTHREAD_CREATE_DETACHED);
//	thread_ctrl = pthread_create(&thread_ctrl_id, &att, ControlFun, NULL);
//	if(thread_ctrl !=0)
//		printf("消息处理线程创建失败 ！！！！！！\n");

//	while ((thread_ctrl = pthread_create(&thread_ctrl_id,&att, Control_Procedure_Dispose,NULL)) != 0)
//	{
//		sleep(1);
//	}
	pthread_detach(thread_ctrl_id);
}
void Filldata(INT8U zongjiano)
{
}
/*函数说明：保电判断

 * */
void BaoDian_Control_Decide()
{
}
void BaoDianInit()
{
//	INT8U ret;
//	Outpar.lun1_state = HE;
//	Outpar.lun2_state = HE;
//	Outpar.lun1_green = GREEN_LIGHT;
//	Outpar.lun2_green = GREEN_LIGHT;
//	Outpar.lun1_red = RED_CLOSE;
//	Outpar.lun2_red = RED_CLOSE;
//	Outpar.gongk_led = RED_CLOSE;
//	Outpar.diank_led = RED_CLOSE;
//	Outpar.alm_state = 0x01;  //1为断开
//	gpio_writebyte((INT8S *)DEV_ALARM_BUZZER,0x00);
//	Outpar.baodian_led = RED_LIGHT;
//	ret = cmpvar(Outpar,Outpar_old);
//	if (ret != 0)
//	{
//		ControlOut(&Outpar);
//		memcpy(&Outpar_old,&Outpar,sizeof(Outpar));
//	}
//	fprintf(stderr,"进入保电状态\n");
}
/*-------------------------------------------------
 *  保电,mnbh
 *  返回值：0表示不保电，1表示保电
 */
int BaoDianPro(CONTROL_BAODIAN_8001* baodian_8001,TS nowts)
{
	static INT8U firstflg=1;
	static INT16U oldmin;
	if (firstflg==1)
	{
		oldmin = nowts.Minute;
		firstflg = 0;
	}
	if (baodian_8001==NULL) return 0;
	if(baodian_8001->bd_s==0)	return 0;
//	if(((baodian->Valid & 0x81) != 0x81)&& (mempar->f58.NoCommunication_TheMasterStation_Time == 0))
//		return 0;//该参数为０时,永不进入保电状态,该参数单位为小时
//	if ((baodian->Valid & 0x01 )== 0) return 0;
//	if ((baodian->Valid & 0x81) == 0x01) return 1; // 终端掉线保电
//	if (baodian->Delaytime.para_time <= 0) return 1;//无限时保电
//	if (baodian->Delaytime.val_time < baodian->Delaytime.para_time)
//	{
//		if(oldmin != nowts.Minute)
//		{
//			baodian->Delaytime.val_time ++;
//			oldmin =  nowts.Minute;
//			return 1;//保电延时未到期 继续保电
//		}
//
//	}
//	else //保电延时已过，退出本次保电
//	{
//		baodian->Valid = 0;
//		baodian->Delaytime.val_time = 0;
//	}
	return 0;
}



/*控制流程处理
 * 根据控制流程优先级不同，对不同的控制做相应的判断处理*/
void* Control_Procedure_Dispose()
{
	TS ts;
	int i=0;
	INT8U Init_flag = 0;
	time_t tim_now,tim_old;
	while(1)
	{
		GetPara(&ctr_object_class);
		TSGet(&ts);
		if (ts.Sec%10 == 0)  //10秒处理一次
		{
			for( i=0; i<MAXNUM_SUMGROUP; i++ )
				Filldata(i);	//总加组功率缓冲
		}
		//判断是否是主站下发的保电控制
//		if((ctr_object_class.Baodian.bd_s& 0x81) != 0x81)
//		{
//			if(login_flag == 0 && JProgramInfo->jzq_login != 0) //终端上线
//			{
//				//终端在线的情况下，没有设置保电----？
//				ctr_object_class.Baodian.bd_s &= 0x00;
//				tim_now=time(NULL);
////				login_flag = JProgramInfo->jzq_login;
//				if(Init_flag != 0) //非初次上线
//				{
////					BaoDian_recover(&mem->ctrpar);
//				}
//				else    //终端初次上线,不需要保电恢复，Init_flag置1.
//					Init_flag = 0x01;
//			}
//			else if(login_flag != 0 && JProgramInfo->jzq_login == 0)//终端掉线
//			{
//
//			}
//			else if()
//			{
//
//			}
//		}
		//
		if(1)	//7.保电状态判断
		{

		}
		usleep(1000*1000);
	}
	fprintf(stderr,"\n控制线程退出\n");
	pthread_detach(pthread_self());
	pthread_exit(&thread_ctrl_id);
}
void control_proccess()
{
	int ret=0;
	TS ts;
	memset(&ctr_object_class,0,sizeof(CTR_OBJECT_CLASS));
	//打开串口
	OpenSerialPort();
	//初始化控制模块
	InitCtrlModel();
	//获取当前时间
	TSGet(&ts);
	//初始化控制参数		--？此处的共享内存是在哪里定义的？？？？？？？
	init_crt_para(&ctr_object_class);
	//初始化控制状态
	init_CtrlState(&Outpar);
	//记录终端上一个时刻在线状态
	login_flag = 0;
	//获得参数状态
	GetPara(&ctr_object_class);
	//状态灯状态
	State_Led();
	//比对变量参数
	ret = cmpvar();
	if (ret != 0)
	{
		fprintf(stderr,"\nvctrl: 新状态比较返回 %d",ret);
//		ControlOut(&Outpar);
//		memcpy(&Outpar_old,&Outpar,sizeof(Outpar));
	}
	//创建线程
	createdthread();
}
