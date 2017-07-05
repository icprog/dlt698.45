 /*
 * main.c
 *
 *  Created on: Jan 5, 2017
 *
 *      Author: ava
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include <mqueue.h>
#include <semaphore.h>
#include <termios.h>
#include "Shmem.h"
#include "StdDataType.h"
#include "main.h"
#include "AccessFun.h"
#include "PublicFunction.h"

#include "dlt698.h"
#include "filebase.h"

ProgramInfo *JProgramInfo = NULL;

static char *usage_set = "\n--------------------参数设置及基本维护命令----------------------------\n"
        "		 【公网通信模块：主站IP端口设置】cj ip XXX.XXX.XXX.XXX:port XXX.XXX.XXX.XXX:port 	\n"
        "		 【以太网通信参数：主站IP端口设置】cj net-ip XXX.XXX.XXX.XXX:port XXX.XXX.XXX.XXX:port 	\n"
        "		 【设置gprs和以太网的工作模式(0:混合模式 1:客户端模式 2:服务器模式)】 cj online-mode 1 1 \n"
        "		 【主站apn设置】cj apn cmnet		\n"
        "		 【cdma电信用户名密码设置】cj usr-pwd 　user  password	apn	\n"
		"		 【主站通信状态查询】cj cm	\n"
        "		 【通信地址】cj id <addr>	如：地址为123456  :cj id 12 34 56	\n"
        "		 【停程序】cj dog[停程序并且清狗] 或者 cj stop[清狗]		\n"
		"		  [设置维护485端口参数] cj rs485	\n"
		"		  [设置红外ifr端口参数] cj ifr	\n"
		"		  [显示遥信状态值] cj yx\n"
		"		  [查询软件版本和软件日期，方便远程查询集中器版本信息] cj ver\n"
		"		  [基本信息配置查询] cj check\n"
        "[读取心跳] cj heart       "
        "[设置心跳] cj heart 60 s\n"
		"【初始化】cj InIt 3 [数据区初始化]	\n　　　　　　cj InIt 5 [事件初始化]\n　　　　　　cj InIt 6 [需量初始化]\n　　　　　　cj InIt 4 [恢复出厂参数]\n"
        "[ESAM 测试，测试写到/nand/esam.log] 测试模式1[20M通信1次]：cj esam\n"
        "            测试模式2[speed M通信1次，speed范围可从1到25]：cj esam speed\n"
        "            测试模式3[speed M通信n次，speed范围可从1到25]：cj esam speed n\n"
        "            测试模式4[speed1 M到 speed2 通信n次，speed范围从1到25]：cj esam speed1 speed2 n\n"
        "-------------------------------------------------------\n\n";
static char *usage_data = "\n--------------------数据维护命令----------------------------\n"
        "		 【任务数据读取】cj taskdata <文件名>		\n"
		"		 【曲线数据点采集率显示】cj taskinfo <文件名>		\n"
		"		 【曲线OAD采集率显示】cj oadinfo <文件名>		\n"
        "		 【冻结数据读取】cj freezedata 冻结OI 关联OI	\n"
		"		 【曲线数据补送】cj report 64 2017 6 6 10 30 11 30 **上报任务17-6-6 10:30到11:30这个点的数据上报	\n"
        "-------------------------------------------------------\n\n";
static char *usage_vari = "\n--------------------变量类对象----------------------------\n"
        "		 【供电时间】cj vari 2203		\n"
        "		 【复位次数】cj vari 2204		\n"
        "-------------------------------------------------------\n\n";
static char *usage_event = "--------------------事件类对象----------------------------\n"
        "[初始化事件参数]  cj event init <oi> :例如：初始化采集终端初始化事件  cj event init 0x3100/0全部 	\n"
        "[复位事件]  cj event reset <oi> :例如：复位采集终端初始化事件  cj event reset 0x3100 	\n"
        "[读取事件属性] cj event pro <oi> :例如：读取采集终端初始化事件属性 cj event pro 0x3100 	\n"
        "[设置Class7]  cj event pro <oi> 当前记录数 最大记录数 上报标识 有效标识 关联对象个数 关联对象OAD[1-10]	\n"
        "	[设置采集终端初始化事件] cj event pro 3100 1 16 1 1 0 \n"
        "	[设置终端状态量变位事件] cj event pro 3104 1 16 1 1 5 201E-4200 F203-4201 F203-4202 F203-4203 F203-4204 F203-4205\n"
        "[读取事件记录] cj event record <oi> 0（所有）/n（记录n）:例如：读取采集终端初始化事件记录 cj event record 0x3100 0（所有）/1(记录1)"
        "[读取事件有效/无效] cj event enable\n"
        "[设置电能表开盖事件有效/无效] cj event enable 301B 1/0 1有效 0无效 \n"
        "[设置停上电事件有效/无效] cj event enable 3106 1/0 1有效 0无效 \n"
        "[设置对时事件有效/无效] cj event enable 3114 1/0 1有效 0无效 \n"
        "-------------------------------------------------------\n\n";
static char *usage_para = "\n--------------------参变量类对象----------------------------\n"
        "[电气设备] "
        "		 【时钟参数】cj para pro 4000		\n"
        "		 【终端广播校时】cj para pro 4204		\n"
       "		 【设备管理基本参数读取】cj para pro 4300 		\n"
        "		 【数据初始化】cj para method 4300 3		\n"
        "		 【恢复出厂参数】cj para method 4300 4		\n"
        "		 【事件初始化】cj para method 4300 5		\n"
        "		 【需量初始化】cj para method 4300 6		\n"
		"		 【无线公网通信接口类】cj para pro 4500 		\n"
		"		 【以太网通信接口类】cj para pro 4510 		\n"
        "-------------------------------------------------------\n\n";
static char *usage_coll =
        "\n--------------------采集监控类对象----------------------------\n"
                "[清除配置]cj coll clear <oi>	\n"
                "[删除一个配置单元]cj coll delete <oi> <id>  	id=【1..255】	\n"
                "[采集档案配置表读取]cj coll pro 6000	\n"
                "[增加一个采集档案配置表 ]查看帮助：cj coll add 6000	\n"
        		"[增加任务配置单元]cj coll pro 6013 任务ID 执行频率 方案类型 方案编号 开始时间 结束时间 延时 执行优先级 状态 运行时段 起始小时:起始分钟 结束小时:结束分钟\n"
        		"             cj coll pro 6013 1 1-5 1 1 2016-11-11 0:0:0 2099-9-9 9:9:9 1-2 2 1 0 0:0-23:59\n"
                "[任务配置单元] cj coll pro 6013 <任务号> [读取任务配置单元]\n"
                "[普通采集方案] cj coll pro 6015 <采集方案号>\n"
                "[事件采集方案] cj coll pro 6017 <方案编号>\n"
				"[透明方案] 	cj coll pro 6019 <方案编号>\n"
                "[上报方案] 	  cj coll pro 601d <方案编号>\n"
                "[采集任务监控] cj coll pro 6035 <采集方案号>\n"
                "-------------------------------------------------------\n\n";
static char *usage_inoutdev = "\n-------------------A.12　输入输出设备类对象----------------------------\n"
        "\n---------文件传输类对象 	ESAM接口类对象 	输入输出设备类对象 	显示类对象--------\n"
        "【开关量输入】cj dev pro f203 				\n"
        "[初始化通信参数]  cj dev init <oi> :	例如：初始化通信参数  cj dev init 4500 	\n"
        "【安全模式参数读取】cj dev pro f101 			\n"
        "【安全模式参数设置,0:不启用，1:启用】cj dev set f101 <0/1>		\n"
        "-------------------------------------------------------\n\n";
static char *usage_acs = "--------------------终端交采计量校表及维护命令----------------------------\n"
        "acs acreg   <Pa Pb Pc Qa Qb Qc Ua Ub Uc Ia Ib Ic >   [同时校正三相系数，输入值为单相标准值]\n\n"
        "		 [三相四交采校表:准备工作：标准源输入220V,3A,角度=60“C(0.5L 感性)]\n"
        "         例如输入：cj acs acreg 330.00 330.00 330.00 571.577 571.577 571.577 220.0 220.0 220.0 3 3 3\n"
        "         [参数输入标准源显示值，可输入浮点数。]\n"
        "		<Pa 0 Pc Qa 0 Qc Uab 0 Uca Ia 0 Ic >\n"
        "		 [三相三交采校表:准备工作：标准源输入100V,3A,角度=1“C(1L 感性)]\n"
        "         例如输入：cj acs acreg 259.8076 0 259.8076 150 0 -150 100 0 100 3 0 -3\n"
        "acs acphase   <Pa Pb Pc Qa Qb Qc Ua Ub Uc Ia Ib Ic >    [小电流相位校正：同时校正三相系数，输入值为单相标准值]\n\n"
        "		 [ATT7022E 交采校表:准备工作：标准源输入220V,0.3A,角度=60“C(0.5L 感性)]\n"
        "         例如输入：cj acs acphase 165.00 165.00 165.00 285.79 285.79 285.79 220.0 220.0 220.0 220.0 1.5 1.5 1.5\n"
        "         [参数输入标准源显示值，可输入浮点数。]\n"
        "		 [ATT7022E-D 交采校表:准备工作：标准源输入220V,1.5A,角度=60“C(0.5L 感性)]\n"
        "         例如输入：cj acs acphase 165 165 165 286 286 286 220 220 220 1.5 1.5 1.5\n"
        "         [参数输入标准源显示值，可输入浮点数。]\n"
        "		[三相三交采校表:准备工作：标准源输入100V,1.5A,角度=1“C(1L 感性)]\n"
        "         例如输入：cj acs acphase 129.9 0 129.9 75 0 -75 100 0 100 1.5 0 1.5\n"
        "acs acphase0  <Pa Pb Pc Qa Qb Qc Ua Ub Uc Ia Ib Ic >  [(7022E-d型芯片支持)电流相位校正：同时校正三相系数，输入值为单相标准值]\n\n"
        "		 [交采校表:准备工作：标准源输入220V,0.15A,角度=60“C(0.5L 感性)]\n"
        "         例如输入：cj acs acphase0 16.5 16.5 16.5 28.58 28.58 28.58 220 220 220 0.15 0.15 0.15\n"
        "         [参数输入标准源显示值，可输入浮点数。]\n"
        "		[三相三交采校表:准备工作：标准源输入100V,0.3A,角度=1“C(1L 感性)]\n"
        "         例如输入：cj acs acphase0 25.9 0 25.9 15 0 -15 100 0 100 0.3 0 0.3\n"
        "acs acregclean [清除（交采）校表系数]\n"
        "acs acregdata	[读校表系数]\n"
        "acs acdata  [打印测量点1（交采）实时数据]\n"
        "acs ace  	 [测量点1（交采）电能示值数据]\n"
        "acs acrndata	[读RN8209校表系数]\n"
        "acs checku <U> [rn8209交采电压校正：输入标准源显示值]\n"
        "        例如输入：cj acs checku 220.00 \n"
        "        [参数输入标准源显示值，可输入浮点数。]\n"
        "-------------------------------------------------------\n\n";

void prthelp() {
    fprintf(stderr, "Usage: ./cj (维护功能)  ");
    fprintf(stderr, "help	 [help] ");
    fprintf(stderr, "%s", usage_acs);
    fprintf(stderr, "%s", usage_set);
    fprintf(stderr, "%s", usage_data);
    fprintf(stderr, "%s", usage_vari);
    fprintf(stderr, "%s", usage_event);
    fprintf(stderr, "%s", usage_para);
    fprintf(stderr, "%s", usage_coll);
    fprintf(stderr, "%s", usage_inoutdev);
}


void dog_feed(char *argv) {
    INT32S fd = -1;
    INT32S tm = 3600;

    if (strcmp("dog", argv) == 0 ) {
		system("pkill cjmain");
		sleep(1);
    }
    if ((fd = open(DEV_WATCHDOG, O_RDWR | O_NDELAY)) == -1) {
        fprintf(stderr, "\n\r open /dev/watchdog error!!!");
        return;
    }
    write(fd, &tm, sizeof(int));
    close(fd);
    if (strcmp("dog", argv) == 0 ) {
		system("pkill cjmain");
		sleep(1);
		system("pkill cjcomm");
		system("pkill cjdeal");
		system("pkill gsmMuxd");
    }
}

//心跳报文
static INT8U checkbuf[]={0x68,0x1e,0x00,0x43,0x05,0x05,0x00,0x00,0x00,0x00,0x00,0x01,0x63,0x71,0x01,
						0x00,0x01,0x02,0x58,0x07,0xe1,0x05,0x10,0x02,0x00,0x15,0x19,0x00,0x00,0xcc,0x45,0x16};

//
static INT8U getversion[]={0x68,0x17,0x00,0x43,0x05,0x01,0x00,0x00,0x00,0x00,0x00,0x10,0x26,0xf6,0x05,0x01,0x00,
						0x43,0x00,0x03,0x00,0x00,0x46,0x58,0x16};

void set_4400()
{
  CLASS_4400 class={};
  class.num=1;
  class.authority[0].OI=0x4000;
  class.authority[0].one_authority.met_num=1;
  class.authority[0].one_authority.method[0].id=1;
  class.authority[0].one_authority.method[0].visit_authority=1;
  class.authority[0].one_authority.pro_num=1;
  class.authority[0].one_authority.property[0].id=1;
  class.authority[0].one_authority.property[0].visit_authority=1;
  saveCoverClass(0x4400,0,&class,sizeof(CLASS_4400),para_vari_save);
}
int main(int argc, char *argv[]) {
	// set_4400();
	 //return 0;
    if (argc < 2) {
        prthelp();
        return EXIT_SUCCESS;
    }

    //生产检测本地状态灯，使用485_II口发送报文，台体485_II与485_III短接，cjcomm的维护口485III会返回请求的数据
    //在台体检测的python脚本运行时候会调用cj checkled命令,来实现维护口通信,收到报文本地灯会闪烁
    if (strcmp("checkled", argv[1]) == 0) {
    	int port = 1;
    	if(argc==3) {
    		port = atoi(argv[2]);
    	}
    	fprintf(stderr,"port=%d\n",port);
    	int i=0;
    	int comfd1;
    	for(i=0;i<3;i++) {
    		comfd1 = OpenCom(port, 9600, (INT8U *) "even", 1, 8);
			write(comfd1, checkbuf, sizeof(checkbuf));
			usleep(500 * 1000);
    	}
    	close(comfd1);
        return EXIT_SUCCESS;
    }
    if (strcmp("sertest", argv[1]) == 0) {
    	int port = 4;
    	INT8U buf[256]={};
    	int comfd1;
    	int i=0,ret=0;

    	if(argc==3) {
    		port = atoi(argv[2]);
    	}
    	fprintf(stderr,"port=%d\n",port);
		comfd1 = OpenCom(port, 9600, (INT8U *) "even", 1, 8);
		write(comfd1, getversion, sizeof(getversion));
		usleep(500 * 1000);
		ret = read(comfd1, buf, 256);
		for(i=0;i<10;i++) {
			sleep(1);
			fprintf(stderr,"R[%d]=",ret);
			for(i=0;i<ret;i++) {
				fprintf(stderr,"%02x ",buf[i]);
			}
		}
    	close(comfd1);
        return EXIT_SUCCESS;
    }

    if ((strcmp("savetest", argv[1]) == 0) || (strcmp("report", argv[1]) == 0)
    		|| (strcmp("ms", argv[1]) == 0) || (strcmp("gettsas", argv[1]) == 0) || (strcmp("trydel", argv[1]) == 0)) {
    	Test(argc, argv);
    	return EXIT_SUCCESS;
    }

    if (strcmp("ip", argv[1]) == 0) {
        SetIPort(argc, argv);
        return EXIT_SUCCESS;
    }

    if (strcmp("net-ip", argv[1]) == 0) {
        SetNetIPort(argc, argv);
        return EXIT_SUCCESS;
    }

    if (strcmp("apn", argv[1]) == 0) {
        SetApn(argc, argv);
        return EXIT_SUCCESS;
    }
    if (strcmp("id", argv[1]) == 0) {
        SetID(argc, argv);
        return EXIT_SUCCESS;
    }

    if (strcmp("heart", argv[1]) == 0) {
        SetHEART(argc, argv);
        return EXIT_SUCCESS;
    }

    if (strcmp("esam", argv[1]) == 0) {
        EsamTest(argc, argv);
        return EXIT_SUCCESS;
    }

    if (strcmp("dog", argv[1]) == 0 || strcmp("stop", argv[1]) == 0) {
        dog_feed(argv[1]);
        return EXIT_SUCCESS;
    }

    if (strcmp("help", argv[1]) == 0) {
        prthelp();
        return EXIT_SUCCESS;
    }
    if (strcmp("vari", argv[1]) == 0) {
        fprintf(stderr, "%s", usage_vari);
        vari_process(argc, argv);
        return EXIT_SUCCESS;
    }
    if (strcmp("event", argv[1]) == 0) {
        fprintf(stderr, "%s", usage_event);
        event_process(argc, argv);
        return EXIT_SUCCESS;
    }
    if (strcmp("para", argv[1]) == 0) {
        fprintf(stderr, "%s", usage_para);
        para_process(argc, argv);
        return EXIT_SUCCESS;
    }
    //查询软件版本和软件日期，方便远程查询集中器版本信息
    if (strcmp("ver", argv[1]) == 0) {
        get_softver();
        return EXIT_SUCCESS;
    }

    if (strcmp("InIt", argv[1]) == 0) {
    	fprintf(stderr,"　　　　　　cj InIt 3 [数据区初始化]	\n　　　　　　cj InIt 5 [事件初始化]\n　　　　　　cj InIt 6 [需量初始化]\n　　　　　　cj InIt 4 [恢复出厂参数]\n");
        InIt_Process(argc, argv);
        return EXIT_SUCCESS;
    }
    if (strcmp("coll", argv[1]) == 0) {
        fprintf(stderr, "%s", usage_coll);
        coll_process(argc, argv);
        return EXIT_SUCCESS;
    }
    if (strcmp("dev", argv[1]) == 0) {
        fprintf(stderr, "%s", usage_inoutdev);
        inoutdev_process(argc, argv);
        return EXIT_SUCCESS;
    }

    if (strcmp("online-mode", argv[1]) == 0) {
        setOnlineMode(argc, argv);
        return EXIT_SUCCESS;
    }

    if (strcmp("usr-pwd", argv[1]) == 0) {
        SetUsrPwd(argc, argv);
        return EXIT_SUCCESS;
    }

    if (strcmp("acs", argv[1]) == 0) {
        fprintf(stderr, "%s", usage_acs);
        acs_process(argc, argv);
        return EXIT_SUCCESS;
    }
    if (strcmp("test", argv[1]) == 0) {
        fprintf(stderr, "\n自组报文\n");
        cjframe(argc, argv);
        return EXIT_SUCCESS;
    }
//    if (strcmp("cjread", argv[1]) == 0) {
//        fprintf(stderr, "\n查看任务抄表数据\n");
//        cjread(argc, argv);
//        return EXIT_SUCCESS;
//    }
    if (strcmp("rs485", argv[1]) == 0) {
        fprintf(stderr, "\n设置维护485端口参数\n");
        SetF201(argc, argv);
        return EXIT_SUCCESS;
    }
    if (strcmp("ifr", argv[1]) == 0) {
        fprintf(stderr, "\n设置维护红外端口参数\n");
        SetF202(argc, argv);
        return EXIT_SUCCESS;
    }
    if (strcmp("taskdata", argv[1]) == 0) {
        analyTaskData(argc, argv);
        return EXIT_SUCCESS;
    }
    if (strcmp("taskinfo", argv[1]) == 0) {
        analyTaskInfo(argc, argv);
        return EXIT_SUCCESS;
    }
    if (strcmp("oadinfo", argv[1]) == 0) {
        analyTaskOADInfo(argc, argv);
        return EXIT_SUCCESS;
    }
    if (strcmp("freezedata", argv[1]) == 0) {
        analyFreezeData(argc, argv);
        return EXIT_SUCCESS;
    }

    if (strcmp("cs", argv[1]) == 0) {
        getFrmCS(argc, argv);
        return EXIT_SUCCESS;
    }

    if (strcmp("fcs", argv[1]) == 0) {
        getFrmFCS(argc, argv);
        return EXIT_SUCCESS;
    }

    if (strcmp("bettery", argv[1]) == 0) {
        float v1=0,v2=0;
        ConfigPara	cfg_para={};
        ReadDeviceConfig(&cfg_para);
        if(cfg_para.device == 2) {
			bettery_getV_II(&v1);
			fprintf(stderr, "II型集中器：时钟电池电压: %f\n", v1);
        }else {
			bettery_getV(&v1,&v2);
			fprintf(stderr, "时钟电池电压: %f,　设备电池电压: %f\n", v1,v2);
        }
        return EXIT_SUCCESS;
    }

    if (strcmp("bt", argv[1]) == 0) {
        float v1=0,v2=0;
        ConfigPara	cfg_para={};
        ReadDeviceConfig(&cfg_para);
        if(cfg_para.device == 2) {
            bettery_getV_II(&v1);
            fprintf(stderr, "II型集中器：时钟电池电压: %f\n", v1);
        }else {
            bettery_getV(&v1,&v2);
            fprintf(stderr, "时钟电池电压: %f,　设备电池电压: %f\n", v1,v2);
        }

        float th = atof(argv[2]);
        if(v1 > th) {
            fprintf(stderr, "时钟电池电压正常\n");
        }else{
            fprintf(stderr, "时钟电池电压异常%f\n", th);
        }

        return EXIT_SUCCESS;
    }

    if (strcmp("cm", argv[1]) == 0) {
        showStatus();
        return EXIT_SUCCESS;
    }

    if (strcmp("check", argv[1]) == 0) {
        showCheckPara();
        return EXIT_SUCCESS;
    }

    if(strcmp("plc",argv[1])==0)
    {
    	showPlcMeterstatus(argc, argv);
    	return EXIT_SUCCESS;
    }
    if(strcmp("time",argv[1])==0  && argc==3)
    {
    	DateTimeBCD bcdtime;
    	time_t tmpt;
        sscanf(argv[2],"%ld",&tmpt);
        bcdtime = timet_bcd(tmpt);
        fprintf(stderr,"\n%d-%d-%d %d:%d:%d\n",bcdtime.year.data,bcdtime.month.data,bcdtime.day.data,bcdtime.hour.data,bcdtime.min.data,bcdtime.sec.data);
    	return EXIT_SUCCESS;
    }


    if (strcmp("yx", argv[1]) == 0) {
        for(;;){
            CLASS_f203 oif203 = {};
            readCoverClass(0xf203, 0, &oif203, sizeof(CLASS_f203), para_vari_save);
            fprintf(stderr, "[F203]开关量输入\n");
            fprintf(stderr, "属性2：ST=%d_%d_%d_%d %d_%d_%d_%d\n",
            		oif203.statearri.stateunit[0].ST, oif203.statearri.stateunit[1].ST,
            		oif203.statearri.stateunit[2].ST, oif203.statearri.stateunit[3].ST,
            		oif203.statearri.stateunit[4].ST, oif203.statearri.stateunit[5].ST,
                    oif203.statearri.stateunit[6].ST, oif203.statearri.stateunit[7].ST);
            usleep(500000);
        }
        return EXIT_SUCCESS;
    }

    prthelp();
    return EXIT_SUCCESS;
}

/*参数文件修改，改变共享内存的标记值，通知相关进程，参数有改变
 * */
void setOIChange_CJ(OI_698 oi)
{
	ProgramInfo *memp = NULL;
	memp = OpenShMem("ProgramInfo", sizeof(ProgramInfo), NULL);

	switch(oi) {
	case 0x300F:	memp->oi_changed.oi300F++;	break;
	case 0x3010:	memp->oi_changed.oi3010++;	break;
	case 0x301B:	memp->oi_changed.oi301B++;	break;
	case 0x3100:  	memp->oi_changed.oi3100++; 	break;
	case 0x3101:  	memp->oi_changed.oi3101++; 	break;
	case 0x3104:	memp->oi_changed.oi3104++; 	break;
	case 0x3105:	memp->oi_changed.oi3105++; 	break;
	case 0x3106:	memp->oi_changed.oi3106++; 	break;
	case 0x3107:	memp->oi_changed.oi3107++; 	break;
	case 0x3108:	memp->oi_changed.oi3108++; 	break;
	case 0x3109: 	memp->oi_changed.oi3109++; 	break;
	case 0x310A:	memp->oi_changed.oi310A++; 	break;
	case 0x310B:	memp->oi_changed.oi310B++; 	break;
	case 0x310C:	memp->oi_changed.oi310C++; 	break;
	case 0x310D:	memp->oi_changed.oi310D++; 	break;
	case 0x310E:	memp->oi_changed.oi310E++; 	break;
	case 0x310F:	memp->oi_changed.oi310F++; 	break;
	case 0x3110:	memp->oi_changed.oi3110++;	break;
	case 0x3111:	memp->oi_changed.oi3111++;	break;
	case 0x3112:	memp->oi_changed.oi3112++;  break;
	case 0x3114:	memp->oi_changed.oi3114++; 	break;
	case 0x3115:	memp->oi_changed.oi3115++; 	break;
	case 0x3116:	memp->oi_changed.oi3116++;	break;
	case 0x3117:	memp->oi_changed.oi3117++;	break;
	case 0x3118:	memp->oi_changed.oi3118++;	break;
	case 0x3119:	memp->oi_changed.oi3119++;	break;
	case 0x311A:	memp->oi_changed.oi311A++;	break;
	case 0x311B:	memp->oi_changed.oi311B++;	break;
	case 0x311C:	memp->oi_changed.oi311C++;	break;
	case 0x3200:	memp->oi_changed.oi3200++;	break;
	case 0x3201:	memp->oi_changed.oi3201++;	break;
	case 0x3202:	memp->oi_changed.oi3202++;	break;
	case 0x3203:	memp->oi_changed.oi3203++;	break;

	case 0x4000:	memp->oi_changed.oi4000++;	break;
	case 0x4001:	memp->oi_changed.oi4001++;	break;
	case 0x4016:	memp->oi_changed.oi4016++;	break;
	case 0x4030:	memp->oi_changed.oi4030++;	break;
	case 0x4204:	memp->oi_changed.oi4204++;	break;
	case 0x4300:	memp->oi_changed.oi4300++;  break;
	case 0x4500:	memp->oi_changed.oi4500++;  break;
	case 0x4510:	memp->oi_changed.oi4510++;  break;

	case 0x6000:	memp->oi_changed.oi6000++;  break;
	case 0x6002:	memp->oi_changed.oi6002++;  break;
	case 0x6012:	memp->oi_changed.oi6012++;  break;
	case 0x6014:	memp->oi_changed.oi6014++;  break;
	case 0x6016:	memp->oi_changed.oi6016++;  break;
	case 0x6018:	memp->oi_changed.oi6018++;  break;
	case 0x601C:	memp->oi_changed.oi601C++;  break;
	case 0x601E:	memp->oi_changed.oi601E++;  break;
	case 0x6051:	memp->oi_changed.oi6051++;  break;

	case 0xf203:   memp->oi_changed.oiF203++;	break;
	case 0xf101:	memp->oi_changed.oiF101++;  break;
	}
	shmm_unregister("ProgramInfo", sizeof(ProgramInfo));
}
