/*
 * ctrl.c
 *
 *  Created on: Sep 7, 2017
 *      Author: lhl
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "AccessFun.h"
#include "StdDataType.h"
#include "Objectdef.h"
#include "main.h"
#include "dlt698.h"
#include "dlt645.h"
#include "PublicFunction.h"
#include "crtl_base.h"

extern ProgramInfo* JProgramInfo ;

typedef union {//control code
	INT16U u16b;//convenient to set value to 0
	struct {//only for little endian mathine!
		INT8U bak	: 6;	//备用
		INT8U lun1_state: 1;//轮次1-状态
		INT8U lun1_red	: 1;//轮次1-红灯
		INT8U lun1_green: 1;//轮次1-绿灯
		INT8U lun2_state: 1;//轮次2-状态
		INT8U lun2_red	: 1;//轮次2-红灯
		INT8U lun2_green: 1;//轮次2-绿灯
		INT8U gongk_led		: 1;//功控灯
		INT8U diank_led		: 1;//电控灯
		INT8U alm_state		: 1;//告警状态
		INT8U baodian_led	: 1;//报警灯
	} ctrl;
} ctrlUN;
void ctrlTest(int argc, char *argv[])
{
	int roundno = 0;
	int cmd = 0;
   	JProgramInfo = OpenShMem("ProgramInfo", sizeof(ProgramInfo), NULL);
	if(argc < 5){
		fprintf(stderr, "参数不足\n");
	}else {
		roundno = atoi(argv[3]);
		cmd = atoi(argv[4]);
	}
	fprintf(stderr,"轮次【%d】 状态[%d]\n",roundno,cmd);
	if(roundno == 1 && cmd==0)  {
		fprintf(stderr, "一轮 分闸\n");
		JProgramInfo->ctrls.control[0] = 0xEEFFEFEF;
		JProgramInfo->ctrls.control[1] = 0xEEFFEFEF;
		JProgramInfo->ctrls.control[2] = 0xEEFFEFEF;
	}else if(roundno == 1 && cmd == 1) {
		fprintf(stderr, "一轮 合闸\n");
		JProgramInfo->ctrls.control[0] = 0xCCAACACA;
		JProgramInfo->ctrls.control[1] = 0xCCAACACA;
		JProgramInfo->ctrls.control[2] = 0xCCAACACA;
	}else if(roundno == 2 && cmd==0)  {
		fprintf(stderr, "二轮 分闸\n");
		JProgramInfo->ctrls.control[0] = 0x55552525;
		JProgramInfo->ctrls.control[1] = 0x55552525;
		JProgramInfo->ctrls.control[2] = 0x55552525;
	}else if(roundno == 2 && cmd == 1) {
		fprintf(stderr, "二轮 合闸\n");
		JProgramInfo->ctrls.control[0] = 0xCCCC2C2C;
		JProgramInfo->ctrls.control[1] = 0xCCCC2C2C;
		JProgramInfo->ctrls.control[2] = 0xCCCC2C2C;
	}else {
		fprintf(stderr, "非法参数\n");
	}
	shmm_unregister("ProgramInfo", sizeof(ProgramInfo));
}

void ctrl_comm(ctrlUN 	ctrlunit)
{
	CheckModelState();
	InitCtrlModel();
	int fd = OpenSerialPort();
	fprintf(stderr,"ctrlunit = %d",ctrlunit.u16b);
	SetCtrl_CMD(fd, ctrlunit.ctrl.lun1_state, ctrlunit.ctrl.lun1_red, ctrlunit.ctrl.lun1_green,
				ctrlunit.ctrl.lun2_state, ctrlunit.ctrl.lun2_red, ctrlunit.ctrl.lun2_green,
				ctrlunit.ctrl.gongk_led, ctrlunit.ctrl.diank_led, ctrlunit.ctrl.alm_state, ctrlunit.ctrl.baodian_led);
	close(fd);
}
void ctrlType(int argc, char *argv[])
{
	int	type=0;
	ctrlUN 	ctrlunit;
	if(argc == 4) {
		type = atoi(argv[3]);
		if(type == 1) {	//功控
			ctrlunit.ctrl.lun1_state = 1;
			ctrlunit.ctrl.lun1_red = 0;
			ctrlunit.ctrl.lun1_green = 1;
			ctrlunit.ctrl.lun2_state = 1;
			ctrlunit.ctrl.lun2_red = 0;
			ctrlunit.ctrl.lun2_green = 1;
			ctrlunit.ctrl.gongk_led = 1;
			ctrlunit.ctrl.diank_led = 0;
			ctrlunit.ctrl.baodian_led = 0;
			ctrlunit.ctrl.alm_state = 0;
		}else  if(type == 2){ //电控
			ctrlunit.ctrl.lun1_state = 1;
			ctrlunit.ctrl.lun1_red = 0;
			ctrlunit.ctrl.lun1_green = 1;
			ctrlunit.ctrl.lun2_state = 1;
			ctrlunit.ctrl.lun2_red = 0;
			ctrlunit.ctrl.lun2_green = 1;
			ctrlunit.ctrl.gongk_led = 0;
			ctrlunit.ctrl.diank_led = 1;
			ctrlunit.ctrl.baodian_led = 0;
			ctrlunit.ctrl.alm_state = 0;
		}
		if(type==1 || type==2) {
			ctrl_comm(ctrlunit);
		}
	}
}

void ctrlAlarm(int argc, char *argv[])
{
	int	type=0;
	ctrlUN 	ctrlunit;

	if(argc == 4) {
		type = atoi(argv[3]);
		if(type == 1) {	//告警投入
			ctrlunit.ctrl.lun1_state = 1;
			ctrlunit.ctrl.lun1_red = 0;
			ctrlunit.ctrl.lun1_green = 1;
			ctrlunit.ctrl.lun2_state = 1;
			ctrlunit.ctrl.lun2_red = 0;
			ctrlunit.ctrl.lun2_green = 1;
			ctrlunit.ctrl.gongk_led = 0;
			ctrlunit.ctrl.diank_led = 0;
			ctrlunit.ctrl.baodian_led = 0;
			ctrlunit.ctrl.alm_state = 1;
		}else  if(type == 0){ //告警解除
			ctrlunit.ctrl.lun1_state = 1;
			ctrlunit.ctrl.lun1_red = 0;
			ctrlunit.ctrl.lun1_green = 1;
			ctrlunit.ctrl.lun2_state = 1;
			ctrlunit.ctrl.lun2_red = 0;
			ctrlunit.ctrl.lun2_green = 1;
			ctrlunit.ctrl.gongk_led = 0;
			ctrlunit.ctrl.diank_led = 0;
			ctrlunit.ctrl.baodian_led = 0;
			ctrlunit.ctrl.alm_state = 0;
		}
		if(type==1 || type==0) {
			ctrl_comm(ctrlunit);
		}
	}
}

void ctrlKeepelec(int argc, char *argv[])
{
	int	type=0;
	ctrlUN 	ctrlunit;

	if(argc == 4) {
		type = atoi(argv[3]);
		if(type == 1) {	//保电投入
			ctrlunit.ctrl.lun1_state = 1;
			ctrlunit.ctrl.lun1_red = 0;
			ctrlunit.ctrl.lun1_green = 1;
			ctrlunit.ctrl.lun2_state = 1;
			ctrlunit.ctrl.lun2_red = 0;
			ctrlunit.ctrl.lun2_green = 1;
			ctrlunit.ctrl.gongk_led = 0;
			ctrlunit.ctrl.diank_led = 0;
			ctrlunit.ctrl.baodian_led = 1;
			ctrlunit.ctrl.alm_state = 0;
		}else  if(type == 0){ //保电解除
			ctrlunit.ctrl.lun1_state = 1;
			ctrlunit.ctrl.lun1_red = 0;
			ctrlunit.ctrl.lun1_green = 1;
			ctrlunit.ctrl.lun2_state = 1;
			ctrlunit.ctrl.lun2_red = 0;
			ctrlunit.ctrl.lun2_green = 1;
			ctrlunit.ctrl.gongk_led = 0;
			ctrlunit.ctrl.diank_led = 0;
			ctrlunit.ctrl.baodian_led = 0;
			ctrlunit.ctrl.alm_state = 0;
		}
		if(type==1 || type==0) {
			ctrl_comm(ctrlunit);
		}
	}
}

void guiTest(int argc, char *argv[])
{
	int i;
	fprintf(stderr,"\n guitest\n");
	if(argc == 3)
	{
		 i = atoi(argv[2]);
		 fprintf(stderr,"\n---------- i =%d  =\n",i);
		 CLASS_8103 c8103;
		 memset(&c8103,0,sizeof(CLASS_8103));
		 readCoverClass(0x8103, 0, &c8103, sizeof(CLASS_8103),para_vari_save);
		 fprintf(stderr,"\ntime1:%05d time2:%05d",(int)c8103.list[i].v1.t1,(int)c8103.list[i].v1.t2);
		 fprintf(stderr,"\ntime3:%05d time4:%05d",(int)c8103.list[i].v1.t3,(int)c8103.list[i].v1.t4);
		 fprintf(stderr,"\ntime5:%05d time6:%05d",(int)c8103.list[i].v1.t5,(int)c8103.list[i].v1.t6);
		 fprintf(stderr,"\ntime7:%05d time8:%05d",(int)c8103.list[i].v1.t7,(int)c8103.list[i].v1.t8);

	}
}

void breezeTest(int argc, char *argv[])
{
	fprintf(stderr,"【蜂鸣器】蜂鸣投入 cj buzzer 1 <蜂鸣投入>  cj buzzer 0 <蜂鸣解除>\n");
	if(argc == 3) {
		if(strcmp("0",argv[2])==0) {
			gpio_writebyte((INT8S *)DEV_ALARM_BUZZER,0x00);
		}else if(strcmp("1",argv[2])==0){
			gpio_writebyte((INT8S *)DEV_ALARM_BUZZER,0x01);
		}
	}
}

void ctrl_process(int argc, char *argv[])
{
	ctrlUN 	ctrlunit={};
	int fd = 0;
	unsigned int pluse_tmp[2] = { };

	if(argc>2) {
		if(strcmp(argv[1],"ctrl")==0) {
			if(strcmp(argv[2],"round")==0) {
				ctrlTest(argc,argv);
			}else if(strcmp(argv[2],"type")==0) {
				ctrlType(argc,argv);
			}else if(strcmp(argv[2],"alarm")==0) {
				ctrlAlarm(argc,argv);
			}else if(strcmp(argv[2],"keepelec")==0) {
				ctrlKeepelec(argc,argv);
			}else if(strcmp(argv[2],"clear")==0) {
				ctrlunit.u16b = 0;
				ctrl_comm(ctrlunit);
			}else if(strcmp(argv[2],"pulse")==0) {
				for(;;) {
					if ((fd = open(DEV_PULSE, O_RDWR | O_NDELAY)) >= 0) {
						read(fd, &pluse_tmp, 2 * sizeof(unsigned int));
						close(fd);
					}
					fprintf(stderr,"Pulse1 = %d		Pulse2 = %d\n",pluse_tmp[0],pluse_tmp[1]);
					sleep(1);
				}
			}
		}
	}
}

void pluseTest(OI_698 oi,CLASS12	class12)
{
	INT8U	i=0;
	fprintf(stderr,"att2:通信地址");
	for(i=0;i<(class12.addr[0]+1);i++) {
		fprintf(stderr,"%02x ",class12.addr[i]);
	}
	fprintf(stderr,"\n");
	fprintf(stderr,"att3-4:互感器倍率PT=%d，CT=%d\n",class12.pt,class12.ct);
	fprintf(stderr,"att5  :有功功率=%d\n",class12.p);
	fprintf(stderr,"att6  :无功功率=%d\n",class12.q);
	fprintf(stderr,"att7  :当日正向有功电量=%d-%d-%d-%d\n",class12.day_pos_p[0],class12.day_pos_p[1],class12.day_pos_p[2],class12.day_pos_p[3]);
	fprintf(stderr,"att8  :当月正向有功电量=%d-%d-%d-%d\n",class12.mon_pos_p[0],class12.mon_pos_p[1],class12.mon_pos_p[2],class12.mon_pos_p[3]);
	fprintf(stderr,"att9  :当日反向有功电量=%d-%d-%d-%d\n",class12.day_nag_p[0],class12.day_nag_p[1],class12.day_nag_p[2],class12.day_nag_p[3]);
	fprintf(stderr,"att10 :当月反向有功电量=%d-%d-%d-%d\n",class12.mon_nag_p[0],class12.mon_nag_p[1],class12.mon_nag_p[2],class12.mon_nag_p[3]);
	fprintf(stderr,"att11 :当日正向无功电量=%d-%d-%d-%d\n",class12.day_pos_q[0],class12.day_pos_q[1],class12.day_pos_q[2],class12.day_pos_q[3]);
	fprintf(stderr,"att12 :当月正向无功电量=%d-%d-%d-%d\n",class12.mon_pos_q[0],class12.mon_pos_q[1],class12.mon_pos_q[2],class12.mon_pos_q[3]);
	fprintf(stderr,"att13 :当日反向无功电量=%d-%d-%d-%d\n",class12.day_nag_q[0],class12.day_nag_q[1],class12.day_nag_q[2],class12.day_nag_q[3]);
	fprintf(stderr,"att14 :当月反向无功电量=%d-%d-%d-%d\n",class12.mon_nag_q[0],class12.mon_nag_q[1],class12.mon_nag_q[2],class12.mon_nag_q[3]);
	fprintf(stderr,"att15 :正向有功电能示值=%d-%d-%d-%d\n",class12.val_pos_p[0],class12.val_pos_p[1],class12.val_pos_p[2],class12.val_pos_p[3]);
	fprintf(stderr,"att16 :反向有功电能示值=%d-%d-%d-%d\n",class12.val_nag_p[0],class12.val_nag_p[1],class12.val_nag_p[2],class12.val_nag_p[3]);
	fprintf(stderr,"att17 :正向无功电能示值=%d-%d-%d-%d\n",class12.val_pos_q[0],class12.val_pos_q[1],class12.val_pos_q[2],class12.val_pos_q[3]);
	fprintf(stderr,"att18 :反向无功电能示值=%d-%d-%d-%d\n",class12.val_nag_q[0],class12.val_nag_q[1],class12.val_nag_q[2],class12.val_nag_q[3]);
	fprintf(stderr,"pulse_count = %d\n",class12.pluse_count);
}

void sumgroupTest(OI_698 oi,CLASS23	class23)
{
	INT8U	unit = 0;
	INT8U	i=0;
	//总加组配置单元个数：有TSA认为有效
	unit = 0;
	for(i=0;i<MAX_AL_UNIT;i++) {
		printTSA(class23.allist[i].tsa);
		if(class23.allist[i].tsa.addr[0]!=0) {
			unit++;
		}
//		else break;
	}

	fprintf(stderr,"总加组配置单元总个数 = %d \n",unit);
	for(i=0;i<unit;i++) {
		fprintf(stderr,"\n\n--------总加组配置单元  %d \n",i);
		fprintf(stderr,"参与总加的分路通信地址");
		printTSA(class23.allist[i].tsa);
		fprintf(stderr,"总加标志  %d {正向（0），反向（1）}\n",class23.allist[i].al_flag);
		fprintf(stderr,"运算符标志  %d  {加（0），减（1）}\n",class23.allist[i].cal_flag);
		fprintf(stderr,"当前有功=%lld-%lld-%lld-%lld-%lld\n",class23.allist[i].curP[0],
				class23.allist[i].curP[1],class23.allist[i].curP[2],class23.allist[i].curP[3],class23.allist[i].curP[4]);
		fprintf(stderr,"当前无功=%lld-%lld-%lld-%lld-%lld\n",class23.allist[i].curQ[0],
				class23.allist[i].curQ[1],class23.allist[i].curQ[2],class23.allist[i].curQ[3],class23.allist[i].curQ[4]);
			}

	fprintf(stderr,"att7:总加日有功电量=%lld-%lld-%lld-%lld-%lld\n",class23.DayPALL,
			class23.DayP[0],class23.DayP[1],class23.DayP[2],class23.DayP[3]);
	fprintf(stderr,"att8:总加日无功电量=%lld-%lld-%lld-%lld-%lld\n",class23.DayQALL,
			class23.DayQ[0],class23.DayQ[1],class23.DayQ[2],class23.DayQ[3]);
	fprintf(stderr,"att9:总加月有功电量=%lld-%lld-%lld-%lld-%lld\n",class23.MonthPALL,
			class23.MonthP[0],class23.MonthP[1],class23.MonthP[2],class23.MonthP[3]);
	fprintf(stderr,"att10:总加月无功电量=%lld-%lld-%lld-%lld-%lld\n",class23.MonthQALL,
			class23.MonthQ[0],class23.MonthQ[1],class23.MonthQ[2],class23.MonthQ[3]);
	fprintf(stderr,"att11:总加剩余电量=%lld\n",class23.remains);
	fprintf(stderr,"att12:当前功率下浮控后总加有功功率冻结值=%lld\n",class23.DownFreeze);
}

void sum_process(int argc, char *argv[])
{
	int 	tmp=0;
	OI_698	oi=0;
	INT8U	index=0;
	CLASS23	class23={};
	CLASS12	class12={};
	if(argc>2) {
//	   	JProgramInfo = OpenShMem("ProgramInfo", sizeof(ProgramInfo), NULL);
		if(strcmp(argv[1],"sum")==0) {
			if(strcmp(argv[2],"pro")==0) {
				sscanf(argv[3],"%04x",&tmp);
				oi = tmp;
				index = oi-0x2301;
				index = rangeJudge("总加组",index,0,(MAXNUM_SUMGROUP-1));
				if(index == -1) {
					fprintf(stderr,"总加组OI【%04x】输入错误\n",oi);
					return;
				}
				readCoverClass(oi, 0, &class23,	sizeof(CLASS23), para_vari_save);
				sumgroupTest(oi,class23);
//				sumgroupTest(oi,JProgramInfo->class23[index]);
			}
		}
		if(strcmp(argv[1],"pulse")==0) {
			if(strcmp(argv[2],"pro")==0) {
				sscanf(argv[3],"%04x",&tmp);
				oi = tmp;
				index = oi-0x2401;
				index = rangeJudge("脉冲",index,0,(MAX_PULSE_NUM-1));
				if(index == -1) {
					fprintf(stderr,"脉冲计量OI【%04x】输入错误\n",oi);
					return;
				}
				readCoverClass(oi, 0, &class12,	sizeof(CLASS12), para_vari_save);
				pluseTest(oi,class12);
			}
		}
		if(strcmp(argv[1],"ctrlp")==0) {
			if(strcmp(argv[2],"pro")==0) {
				sscanf(argv[3],"%04x",&tmp);
				oi = tmp;
				if(oi==0x8102) {
					CLASS_8102 c8102={};
					memset(&c8102, 0x00, sizeof(CLASS_8102));
					readCoverClass(0x8102, 0, (void *) &c8102, sizeof(CLASS_8102),para_vari_save);
					fprintf(stderr,"c8102.time_num = %d\n",c8102.time_num);
					int i = 0;
					for (i = 0; i < c8102.time_num; i++) {
						fprintf(stderr,"%02x\n", c8102.time[i]);
					}
				}
				if(oi==0x8103) {
					CLASS_8103 c8103={};
					JProgramInfo = OpenShMem("ProgramInfo", sizeof(ProgramInfo), NULL);
					memset(&c8103, 0x00, sizeof(CLASS_8103));
//					memcpy(&c8103,&JProgramInfo->ctrls.c8103,sizeof(CLASS_8103));
					readCoverClass(oi, 0, (void *) &c8103, sizeof(CLASS_8103),para_vari_save);
					int i=0;
					for(i=0;i<MAX_AL_UNIT;i++) {
						fprintf(stderr,"\n-------i=%d------------\n",i);
						fprintf(stderr,"OI = %04x\n",c8103.list[i].index);
						fprintf(stderr,"sign = %02x\n",c8103.list[i].sign);
						fprintf(stderr,"V1 = %d %lld %lld %lld %lld %lld %lld %lld %lld \n",c8103.list[i].v1.n,c8103.list[i].v1.t1,c8103.list[i].v1.t2,
								c8103.list[i].v1.t3,c8103.list[i].v1.t4,c8103.list[i].v1.t5,c8103.list[i].v1.t6,c8103.list[i].v1.t7,c8103.list[i].v1.t8);
						fprintf(stderr,"V2 = %d %lld %lld %lld %lld %lld %lld %lld %lld \n",c8103.list[i].v2.n,c8103.list[i].v2.t1,c8103.list[i].v2.t2,
								c8103.list[i].v2.t3,c8103.list[i].v2.t4,c8103.list[i].v2.t5,c8103.list[i].v2.t6,c8103.list[i].v2.t7,c8103.list[i].v2.t8);
						fprintf(stderr,"V3 = %d %lld %lld %lld %lld %lld %lld %lld %lld \n",c8103.list[i].v3.n,c8103.list[i].v3.t1,c8103.list[i].v3.t2,
								c8103.list[i].v3.t3,c8103.list[i].v3.t4,c8103.list[i].v3.t5,c8103.list[i].v3.t6,c8103.list[i].v3.t7,c8103.list[i].v3.t8);
						fprintf(stderr,"para = %d\n",c8103.list[i].para);
						fprintf(stderr,"控制投入 OI=%x  状态=%d\n",JProgramInfo->ctrls.c8103.enable[i].name,JProgramInfo->ctrls.c8103.enable[i].state);
						fprintf(stderr,"控制输出 OI=%x  状态=%02x\n",JProgramInfo->ctrls.c8103.output[i].name,JProgramInfo->ctrls.c8103.output[i].state);
						fprintf(stderr,"告警状态 OI=%x  状态=%d\n",JProgramInfo->ctrls.c8103.overflow[i].name,JProgramInfo->ctrls.c8103.overflow[i].state);
					}

				}
				if(oi==0x8104) {
					CLASS_8104 c8104={};
					memset(&c8104, 0x00, sizeof(CLASS_8104));
					readCoverClass(oi, 0, (void *) &c8104, sizeof(CLASS_8104),para_vari_save);
					int i=0;
					for(i=0;i<MAX_AL_UNIT;i++) {
						fprintf(stderr,"\n-------i=%d------------\n",i);
						fprintf(stderr,"OI = %04x\n",c8104.list[i].index);
						fprintf(stderr,"厂休控定值 =%lld\n",c8104.list[i].v);
						printDataTimeS("限电起始时间",c8104.list[i].start);
						fprintf(stderr,"限电延续时间 =%d\n",c8104.list[i].sustain);
						fprintf(stderr,"每周限电日 =0x%x\n",c8104.list[i].noDay);
						fprintf(stderr,"控制投入 OI=%x  状态=%d\n",c8104.enable[i].name,c8104.enable[i].state);
						fprintf(stderr,"控制输出 OI=%x  状态=%02x\n",c8104.output[i].name,c8104.output[i].state);
						fprintf(stderr,"告警状态 OI=%x  状态=%d\n",c8104.overflow[i].name,c8104.overflow[i].state);
					}
				}
				if(oi==0x8105) {
					CLASS_8105 c8105={};
					memset(&c8105, 0x00, sizeof(CLASS_8105));
					readCoverClass(oi, 0, (void *) &c8105, sizeof(CLASS_8105),para_vari_save);
					int i=0;
					for(i=0;i<MAX_AL_UNIT;i++) {
						fprintf(stderr,"\n-------i=%d------------\n",i);
						fprintf(stderr,"OI = %04x\n",c8105.list[i].index);
						printDataTimeS("报停起止时间",c8105.list[i].start);
						printDataTimeS("报停结束时间",c8105.list[i].end);
						fprintf(stderr,"功率定值 =%lld\n",c8105.list[i].v);
						fprintf(stderr,"控制投入 OI=%x  状态=%d\n",c8105.enable[i].name,c8105.enable[i].state);
						fprintf(stderr,"控制输出 OI=%x  状态=%02x\n",c8105.output[i].name,c8105.output[i].state);
						fprintf(stderr,"告警状态 OI=%x  状态=%d\n",c8105.overflow[i].name,c8105.overflow[i].state);
					}
				}
				if(oi==0x8107) {
					CLASS_8107 c8107={};
					memset(&c8107, 0x00, sizeof(CLASS_8107));
					readCoverClass(oi, 0, (void *) &c8107, sizeof(CLASS_8107),para_vari_save);
					int i=0;
					for(i=0;i<MAX_AL_UNIT;i++) {
						fprintf(stderr, "\n\n购电-控制单元[%d]\n[对象%04x]\n [购电单号%d]\n [追加刷新标识%d]\n [购电类型%d]\n-[购电量%lld]\n-[报警门限%lld]\n-[跳闸门限%lld]\n-[购电控模式%d]\n",
								i,c8107.list[i].index, c8107.list[i].no,
								c8107.list[i].add_refresh, c8107.list[i].type,
								c8107.list[i].v, c8107.list[i].alarm, c8107.list[i].ctrl, c8107.list[i].mode);
						fprintf(stderr,"控制投入 OI=%x  状态=%d\n",c8107.enable[i].name,c8107.enable[i].state);
						fprintf(stderr,"控制输出 OI=%x  状态=%02x\n",c8107.output[i].name,c8107.output[i].state);
						fprintf(stderr,"告警状态 OI=%x  状态=%d\n",c8107.overflow[i].name,c8107.overflow[i].state);
					}
				}
				if(oi==0x8108) {
					CLASS_8108 c8108={};
					memset(&c8108, 0x00, sizeof(CLASS_8108));
					readCoverClass(oi, 0, (void *) &c8108, sizeof(CLASS_8108),para_vari_save);
					int i=0;
					for(i=0;i<MAX_AL_UNIT;i++) {
						fprintf(stderr, "\n\n月电-控制单元[%d]\n[对象%04x]\n [定值%ld]\n [限值系数%d]\n [浮动系数%d]\n",
								i,c8108.list[i].index, c8108.list[i].v,c8108.list[i].para,c8108.list[i].flex);
						fprintf(stderr,"控制投入 OI=%x  状态=%d\n",c8108.enable[i].name,c8108.enable[i].state);
						fprintf(stderr,"控制输出 OI=%x  状态=%02x\n",c8108.output[i].name,c8108.output[i].state);
						fprintf(stderr,"告警状态 OI=%x  状态=%d\n",c8108.overflow[i].name,c8108.overflow[i].state);

					}
				}
			}
		}
//		shmm_unregister("ProgramInfo", sizeof(ProgramInfo));
	}
}

