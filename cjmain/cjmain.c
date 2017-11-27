#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>
#include <sys/reboot.h>
#include <wait.h>
#include <errno.h>
#include <mqueue.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

#include "ParaDef.h"
#include "PublicFunction.h"
#include "AccessFun.h"
#include "cjmain.h"
#include "basedef.h"
#include "att7022e.h"
#include "../lib698/dlt698.h"
#include "../libMq/libmmq.h"
#include "../include/version.h"

static ProgramInfo *JProgramInfo = NULL;
static int mmqFds[MAX_MMQ_SIZE] = { };

const static mmq_attribute mmq_register[] =
		{ { cjcomm, PROXY_485_MQ_NAME,MAXSIZ_PROXY_NET, MAXNUM_PROXY_NET },
		  { cjdeal, PROXY_NET_MQ_NAME,MAXSIZ_PROXY_485, MAXNUM_PROXY_485 },
		  { cjdeal, TASKID_485_1_MQ_NAME,MAXSIZ_TASKID_QUEUE, MAXNUM_TASKID_QUEUE },
		  { cjdeal, TASKID_485_2_MQ_NAME, MAXSIZ_TASKID_QUEUE, MAXNUM_TASKID_QUEUE },
		  { cjdeal, TASKID_plc_MQ_NAME, MAXSIZ_TASKID_QUEUE, MAXNUM_TASKID_QUEUE } };

#define	LED_LIGHT	1//点亮led
#define	LED_CLOSE	0//关闭led

#define	PWR_SHUT_CNT_ZJ		90//集中器连续断电的计数值, 超过这个计数值就认为是彻底断电了
#define	PWR_SHUT_CNT_SD			60//山东省计量要求1分钟后必须灭掉全部灯

INT8U	g_powerState = 0;//交流电是否断电, 1-上电状态; 0-断电状态
/*
 * 国网要求：
 * 运行灯：红色，灯常亮表示终端主CPU正常运行，但未和主站建立连接，灯亮1s灭1s交替闪烁表示终端正常运行且和主站建立连接；
 *
 * 浙江要求：
 * 运行灯：一直常亮
 * */
void Runled(int state) {
	static INT8S run_staus = 0;
	if (getZone("GW") == 0) {
		if (JProgramInfo->dev_info.connect_ok == 1) {
			gpio_writebyte((char *) DEV_LED_RUN, run_staus);
			if (run_staus == 1)
				run_staus = 0;
			else
				run_staus = 1;
		} else {
			gpio_writebyte((char *) DEV_LED_RUN, state);
		}
	} else {
		//浙江要求运行灯常亮
		gpio_writebyte((char *) DEV_LED_RUN, state);
	}
}

void setRunLED(INT8S state)
{
	INT8S swch = ((PWR_ON == g_powerState) ? state : LED_CLOSE);
	Runled(swch);
}

void shutAllLed()
{
	gpio_writebyte((char *) DEV_LED_RUN, (INT8S)0);
	gpio_writebyte((char *) DEV_LED_ONLINE, (INT8S)0);
	gpio_writebyte((char *) DEV_LED_CSQ_RED, (INT8S)0);
	gpio_writebyte((char *) DEV_LED_CSQ_GREEN, (INT8S)0);
	gpio_writebyte((char *) DEV_LED_RMT_RED, (INT8S)0);
	gpio_writebyte((char *) DEV_LED_RMT_GRN, (INT8S)0);
}

void SyncRtc(void) {
	static int MinOld = 0;
	TS ts;
	TSGet(&ts);

	if (MinOld != ts.Minute) {
		//每20分钟更新一次RTC时钟
		MinOld = ts.Minute;

		if (ts.Minute % 20 == 0) {
			asyslog(LOG_INFO, "开始更新TRTC时钟...[%d:%d]", ts.Hour, ts.Minute);
			system("hwclock -s &");
		}
	}
}

void Watchdog(int count) //硬件看门狗
{
	int fd = -1;

	if (count < 2 || count > 120) {
		count = 5;
	}

	if ((fd = open(DEV_WATCHDOG, O_RDWR | O_NDELAY)) == -1) {
		asyslog(LOG_ERR, "打开硬件狗设备失败，原因(%d)", errno);
	} else {
		write(fd, &count, sizeof(int));
		close(fd);
	}
	return;
}

/*
 * 检测电池掉电后，pwrdelay（秒）延时关闭电池，关闭集中器
 * */
void PowerOffToClose(INT8U pwrdelay) {
	static INT8U cnt_pwroff = 0;

	if (pwr_has() == FALSE) {
		fprintf(stderr, "\n底板电源已关闭，设备关闭倒计时：%d s..........", cnt_pwroff);
		cnt_pwroff++;
		if (cnt_pwroff == pwrdelay) {
			fprintf(stderr, "\n设备关闭.....");
			gpio_writebyte((char *) DEV_BAT_SWITCH, (INT8S) 0);
		}
	} else
		cnt_pwroff = 0;
}

/*
 * 检测交采掉电后，delay个计数后, 重启集中器
 * 这个需求是浙江现场的工程人员提出的
 * 假如集中器程序因为某种原因, 某些进程异常
 * 退出, 现场维护人员需要对集中器重启.
 */
void rebootWhenPwrDown(INT8U delay) {
    static INT8U cnt_pwroff = 0;
    int i = 0;

    if (pwr_down_byVolt(JProgramInfo->ACSRealData.Available,
			JProgramInfo->ACSRealData.Ua,
			JProgramInfo->event_obj.Event3106_obj.poweroff_para_obj.screen_para_obj.happen_voltage_limit) == 1) {
        cnt_pwroff++;

        if (cnt_pwroff > delay) {
            if(PWR_ON == g_powerState) {
    			system("cj stop");
    			g_powerState = PWR_DOWN;
    			for (i=0;i<5;i++) {
    				shutAllLed();
    				usleep(100);
    			}
            }
           	syslog(LOG_NOTICE,"检测到II型掉电(%d)\n",delay);
//        	sleep(3);
//         	system("reboot");
        }
    } else {
        cnt_pwroff = 0;
        g_powerState = PWR_ON;
    }
}

//读取系统配置文件
int ReadSystemInfo() {
	int ProgsNum = 1;

	char Strbuf[256] = { };

	//初始化共享内存参数
	for (int i = 0; i < PROJECTCOUNT; i++) {
		memset((void *) &JProgramInfo->Projects[i], 0, sizeof(ProjectInfo));
	}

	memset(Strbuf, 0x00, sizeof(Strbuf));
	sprintf(Strbuf, "%s/systema.cfg", _CFGDIR_);

	FILE *fp = fopen(Strbuf, "r");
	memset(Strbuf, 0x00, sizeof(Strbuf));

	if (fp == NULL) {
		asyslog(LOG_ERR, "配置文件无法打开，或不存在，文件名(%s)，原因(%d)", Strbuf, errno);
		return -1;
	}

	int max = 0;

	while (fgets(Strbuf, sizeof(Strbuf), fp)) {
		int index = 0, foo = 0;

		//获取序号
		if (sscanf(Strbuf, "%d=%s %s %s %s", &index,
				JProgramInfo->Projects[0].ProjectName,
				JProgramInfo->Projects[0].argv[1],
				JProgramInfo->Projects[0].argv[2],
				JProgramInfo->Projects[0].argv[3]) < 1) {
			continue;
		}

		if (index > max) {
			max = index;
		}

		//获取参数
		if (sscanf(Strbuf, "%d=%s %s %s %s", &foo,
				JProgramInfo->Projects[index].ProjectName,
				JProgramInfo->Projects[index].argv[1],
				JProgramInfo->Projects[index].argv[2],
				JProgramInfo->Projects[index].argv[3]) < 1) {
			continue;
		}

		if (index < 0 || index > PROJECTCOUNT
				|| strlen(JProgramInfo->Projects[index].ProjectName) < 2) {
			asyslog(LOG_WARNING, "发现不合规项目，文件名(%s)，序号(%d)",
					JProgramInfo->Projects[index].ProjectName, index);
			continue;
		}

		JProgramInfo->Projects[index].ProjectState = NeedStart;
		JProgramInfo->Projects[index].WaitTimes = 0;
		sprintf(JProgramInfo->Projects[index].argv[0], "%d", index);

		memset(Strbuf, 0x00, sizeof(Strbuf));
		asyslog(LOG_WARNING, "需要启动程序[%s][%s][%d]",
				JProgramInfo->Projects[index].ProjectName,
				JProgramInfo->Projects[index].argv[0], index);
		ProgsNum++;
	}

	fclose(fp);

	asyslog(LOG_INFO, "读取配置文件，共计(%d)个程序需要启动...\n", ProgsNum);
	return max + 1;
}

void Createmq() {
	for (int i = 0; i < MAX_MMQ_SIZE; i++) {
		mmqFds[i] = 0;
	}

	for (int i = 0; i < sizeof(mmq_register) / sizeof(mmq_attribute); i++) {
		struct mq_attr attr;
		attr.mq_maxmsg = mmq_register[i].maxnum;
		attr.mq_msgsize = mmq_register[i].maxsiz;

		if ((mmqFds[i] = mmq_create((INT8S *) mmq_register[i].name, &attr,
				O_RDONLY)) < 0) {
			asyslog(LOG_ERR, "消息队列创建失败，重建系统限制...");

			struct rlimit limit = { RLIM_INFINITY, RLIM_INFINITY };
			if (getrlimit(RLIMIT_MSGQUEUE, &limit) != 0) {
				asyslog(LOG_ERR, "重建系统限制时，获取系统参数失败...");
			} else {
				asyslog(LOG_ERR, "重建系统限制，系统参数[%d-%d]", limit.rlim_cur,
						limit.rlim_max);
				limit.rlim_cur = 1024000;
				limit.rlim_max = 1024000;
				setrlimit(RLIMIT_MSGQUEUE, &limit);
			}
			usleep(100 * 1000);
		}
		asyslog(LOG_INFO, "创建消息队列，消息号[%d]", mmqFds[i]);
	}
}

void MmqClear() {
	for (int i = 0; i < MAX_MMQ_SIZE; i++) {
		if (mmqFds[i] == 0) {
			continue;
		}
		asyslog(LOG_INFO, "关闭消息队列，消息号[%d]", mmqFds[i]);
		mmq_close(mmqFds[i]);
	}
}

void shmm_destroy() {
	if (JProgramInfo != NULL) {
		munmap(JProgramInfo, sizeof(ProgramInfo));
		JProgramInfo = NULL;
		shm_unlink((const char *) "ProgramInfo");
	}
}

//程序入口函数-----------------------------------------------------------------------------------------------------------
//程序退出前处理，杀死其他所有进程 清楚共享内存
void ProjectMainExit(int signo) {
	fprintf(stderr, "\ncjmain exit\n");
	MmqClear();
	close_named_sem(SEMNAME_SPI0_0);
	sem_unlink(SEMNAME_SPI0_0);
	close_named_sem(SEMNAME_PARA_SAVE);
	sem_unlink(SEMNAME_PARA_SAVE);

	shmm_destroy();

	exit(0);
	return;
}

void Checkupdate() {
	static int UpStates = 0;

	if (UpStates == 1) {
		return;
	}

	FILE *fp_org = 0;
	FILE *fp_new = 0;

	if (access("/dos/cjgwn", 0) == 0) {
		Watchdog(60);
		asyslog(LOG_INFO, "检测有升级目录{3}，开始执行升级脚本...");
		system("mkdir /nand/UpFiles");
		system("chmod 777 /dos/cjgwn/index.sh");

		if(getZone("ZheJiang") == 0) {
			asyslog(LOG_INFO, "浙江地区不检测校验码, 直接升级...");
			system("/dos/cjgwn/index.sh");
		}

		fp_org = fopen("/nand/Version.log", "r");
		if (fp_org != NULL) {
			char md5_org[24];
			char md5_new[24];
			memset(md5_new, 0x00, sizeof(md5_new));
			memset(md5_org, 0x00, sizeof(md5_org));
			system("rm /nand/Version.log.new");
			system("md5sum /dos/cjgwn/app/update.sh >> /nand/Version.log.new");
			fp_new = fopen("/nand/Version.log.new", "r");

			if (fp_new != NULL) {
				fread(md5_new, sizeof(md5_new), 1, fp_new);
				fread(md5_org, sizeof(md5_org), 1, fp_org);
				int res = strncmp(md5_new, md5_org, 24);
				if (res != 0) {
					asyslog(LOG_INFO, "版本比对不同，开始升级....");
					system("/dos/cjgwn/index.sh");
				} else {
					asyslog(LOG_INFO, "版本比对相同，不予升级....");
				}
			}
		} else {
			asyslog(LOG_INFO, "找不到原始md5版本，开始升级....");
			system("/dos/cjgwn/index.sh");
		}

		if (access("/nand/UpFiles/update.sh", 0) == 0) {
			asyslog(LOG_INFO, "升级包存在，重新校验原始md5...");
			system("rm /nand/Version.log");
			system("md5sum /nand/UpFiles/update.sh >> /nand/Version.log");
		}

		if (fp_new != NULL) {
			fclose(fp_new);
		}

		if (fp_org != NULL) {
			fclose(fp_org);
		}

		UpStates = 1;
	}
	return;
}

void CreateSem() {
	int val;
	sem_t *sem;


	//信号量建立
	sem = create_named_sem(SEMNAME_SPI0_0, 1);
	sem_getvalue(sem, &val);
	asyslog(LOG_INFO, "SPI信号量建立，初始值[%d]", val);

	sem = create_named_sem(SEMNAME_PARA_SAVE, 1);
	sem_getvalue(sem, &val);
	asyslog(LOG_INFO, "PARA_SAVE信号量建立，初始值[%d]", val);
}

/*
 * 初始化操作
 * */
void InitSharedMem(int argc, char *argv[]) {
	JProgramInfo = (ProgramInfo *) CreateShMem("ProgramInfo",
			sizeof(ProgramInfo), NULL);
	asyslog(LOG_NOTICE, "打开共享内存，地址[%p]，大小[%d]", JProgramInfo,
			sizeof(ProgramInfo));

	memset(&JProgramInfo->oi_changed,0,sizeof(OI_CHANGE));
	InitClass4016(); //当前套日时段表
	InitClass4300(); //电气设备信息
	InitClassf203(); //开关量输入
	InitClassByZone(1); //根据地区进行相应初始化	4500,4510参数,防止参数丢失,重新生产

	//事件参数初始化
	readCoverClass(0x3100, 0, &JProgramInfo->event_obj.Event3100_obj,
			sizeof(JProgramInfo->event_obj.Event3100_obj), event_para_save);
	readCoverClass(0x3101, 0, &JProgramInfo->event_obj.Event3101_obj,
			sizeof(JProgramInfo->event_obj.Event3101_obj), event_para_save);
	readCoverClass(0x3104, 0, &JProgramInfo->event_obj.Event3104_obj,
			sizeof(JProgramInfo->event_obj.Event3104_obj), event_para_save);
	readCoverClass(0x3105, 0, &JProgramInfo->event_obj.Event3105_obj,
			sizeof(JProgramInfo->event_obj.Event3105_obj), event_para_save);
	readCoverClass(0x3106, 0, &JProgramInfo->event_obj.Event3106_obj,
			sizeof(JProgramInfo->event_obj.Event3106_obj), event_para_save);
	readCoverClass(0x3107, 0, &JProgramInfo->event_obj.Event3107_obj,
			sizeof(JProgramInfo->event_obj.Event3107_obj), event_para_save);
	readCoverClass(0x3108, 0, &JProgramInfo->event_obj.Event3108_obj,
			sizeof(JProgramInfo->event_obj.Event3108_obj), event_para_save);
	readCoverClass(0x3109, 0, &JProgramInfo->event_obj.Event3109_obj,
			sizeof(JProgramInfo->event_obj.Event3109_obj), event_para_save);
	readCoverClass(0x310A, 0, &JProgramInfo->event_obj.Event310A_obj,
			sizeof(JProgramInfo->event_obj.Event310A_obj), event_para_save);
	readCoverClass(0x310B, 0, &JProgramInfo->event_obj.Event310B_obj,
			sizeof(JProgramInfo->event_obj.Event310B_obj), event_para_save);
	readCoverClass(0x310C, 0, &JProgramInfo->event_obj.Event310C_obj,
			sizeof(JProgramInfo->event_obj.Event310C_obj), event_para_save);
	readCoverClass(0x310D, 0, &JProgramInfo->event_obj.Event310D_obj,
			sizeof(JProgramInfo->event_obj.Event310D_obj), event_para_save);
	readCoverClass(0x310E, 0, &JProgramInfo->event_obj.Event310E_obj,
			sizeof(JProgramInfo->event_obj.Event310E_obj), event_para_save);
	readCoverClass(0x310F, 0, &JProgramInfo->event_obj.Event310F_obj,
			sizeof(JProgramInfo->event_obj.Event310F_obj), event_para_save);
	readCoverClass(0x3110, 0, &JProgramInfo->event_obj.Event3110_obj,
			sizeof(JProgramInfo->event_obj.Event3110_obj), event_para_save);
	readCoverClass(0x3111, 0, &JProgramInfo->event_obj.Event3111_obj,
			sizeof(JProgramInfo->event_obj.Event3111_obj), event_para_save);
	readCoverClass(0x3112, 0, &JProgramInfo->event_obj.Event3112_obj,
			sizeof(JProgramInfo->event_obj.Event3112_obj), event_para_save);
	readCoverClass(0x3114, 0, &JProgramInfo->event_obj.Event3114_obj,
			sizeof(JProgramInfo->event_obj.Event3114_obj), event_para_save);
	readCoverClass(0x3115, 0, &JProgramInfo->event_obj.Event3115_obj,
			sizeof(JProgramInfo->event_obj.Event3115_obj), event_para_save);
	readCoverClass(0x3116, 0, &JProgramInfo->event_obj.Event3116_obj,
			sizeof(JProgramInfo->event_obj.Event3116_obj), event_para_save);
	readCoverClass(0x3117, 0, &JProgramInfo->event_obj.Event3117_obj,
			sizeof(JProgramInfo->event_obj.Event3117_obj), event_para_save);
	readCoverClass(0x3118, 0, &JProgramInfo->event_obj.Event3118_obj,
			sizeof(JProgramInfo->event_obj.Event3118_obj), event_para_save);
	readCoverClass(0x3119, 0, &JProgramInfo->event_obj.Event3119_obj,
			sizeof(JProgramInfo->event_obj.Event3119_obj), event_para_save);
	readCoverClass(0x311A, 0, &JProgramInfo->event_obj.Event311A_obj,
			sizeof(JProgramInfo->event_obj.Event311A_obj), event_para_save);
	readCoverClass(0x311B, 0, &JProgramInfo->event_obj.Event311B_obj,
			sizeof(JProgramInfo->event_obj.Event311B_obj), event_para_save);
	readCoverClass(0x311C, 0, &JProgramInfo->event_obj.Event311C_obj,
			sizeof(JProgramInfo->event_obj.Event311C_obj), event_para_save);
	readCoverClass(0x3200, 0, &JProgramInfo->event_obj.Event3200_obj,
			sizeof(JProgramInfo->event_obj.Event3200_obj), event_para_save);
	readCoverClass(0x3201, 0, &JProgramInfo->event_obj.Event3201_obj,
			sizeof(JProgramInfo->event_obj.Event3201_obj), event_para_save);
	readCoverClass(0x3202, 0, &JProgramInfo->event_obj.Event3202_obj,
			sizeof(JProgramInfo->event_obj.Event3202_obj), event_para_save);
	readCoverClass(0x3203, 0, &JProgramInfo->event_obj.Event3203_obj,
			sizeof(JProgramInfo->event_obj.Event3203_obj), event_para_save);
	readCoverClass(0x300F, 0, &JProgramInfo->event_obj.Event300F_obj,
			sizeof(JProgramInfo->event_obj.Event300F_obj), event_para_save);
	readCoverClass(0x3010, 0, &JProgramInfo->event_obj.Event3010_obj,
			sizeof(JProgramInfo->event_obj.Event3010_obj), event_para_save);

	if (0 == JProgramInfo->event_obj.Event3106_obj.poweroff_para_obj.screen_para_obj.recover_voltage_limit) {
		JProgramInfo->event_obj.Event3106_obj.poweroff_para_obj.screen_para_obj.recover_voltage_limit = VOL_ON_THR;
	}

	if (0 == JProgramInfo->event_obj.Event3106_obj.poweroff_para_obj.screen_para_obj.happen_voltage_limit) {
		JProgramInfo->event_obj.Event3106_obj.poweroff_para_obj.screen_para_obj.happen_voltage_limit = VOL_DOWN_THR;
	}
}

int LAPI_Fork2(void) {
	pid_t pid;

	if (!(pid = fork())) {
		switch (fork()) {
		case 0:
			return 0;
		case -1:
			_exit(errno); /* assumes all errnos are <256 */
			break;
		default:
			_exit(0); //孙子的父亲离开，以便让儿子的父亲回收!!
			break;
		}
	}

	int status = 0;
	if (pid < 0 || waitpid(pid, &status, 0) < 0) //儿子的父亲回收
		return -1;

	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status) == 0) {
			return 1;
		} else {
			errno = WEXITSTATUS(status);
		}
	} else {
		errno = EINTR; /* well, interrupted */
	}

	return -1;
}

int ProjectKill(ProjectInfo proinfo) {
	if (proinfo.ProjectID > 0) {
		kill(proinfo.ProjectID, SIGTERM);
		//cjcomm出现异常，进程无法彻底杀死
		asyslog(LOG_WARNING, "停止进程，ID(%d) ", proinfo.ProjectID);
		return 1;
	}

	if (proinfo.ProjectID == 0) {
		return 1;
	}

	return 0;
}

//激活外部程序
int ProjectExecute(ProjectInfo proinfo, int i) {
	pid_t pid = LAPI_Fork2();

	if (pid == -1) {
		asyslog(LOG_WARNING, "程序启动失败[%s],原因(%d)", (char *) proinfo.ProjectName,
				errno);
		return -1;
	}

	if (pid == 0) {
		//子进程代码运行部分
		asyslog(LOG_WARNING, "程序启动[%s %s %s %s %s]",
				(char *) proinfo.ProjectName, proinfo.argv[0], proinfo.argv[1],
				proinfo.argv[2], proinfo.argv[3]);
		execlp((char *) proinfo.ProjectName, (char *) proinfo.ProjectName,
				proinfo.argv[0], proinfo.argv[1], proinfo.argv[2],
				proinfo.argv[3], NULL);
		return 0;
	}
	return 1;
}

//检查外部程序是否正常运行,是否超时
void ProjectCheck(ProjectInfo *proinfo) {
	if (strlen((char *) proinfo->ProjectName) < 2) {
		return;
	}
	if (proinfo->ProjectState == NowRun) {
		if (proinfo->WaitTimes > PRO_WAIT_COUNT) {
			proinfo->ProjectState = NeedKill;
			proinfo->WaitTimes = 0;
		}
	}
}

void checkProgsState(int ProgsNum) {

	ProjectInfo *pis = JProgramInfo->Projects;

	for (int i = 1; i < ProgsNum; i++) {

		ProjectCheck(&JProgramInfo->Projects[i]);
		switch (JProgramInfo->Projects[i].ProjectState) {
		case NeedKill:
			asyslog(LOG_WARNING, "检测到程序异常，名称[%s] PID[%d-%d]",
					pis[i].ProjectName, pis[i].ProjectID, i);
			if(strncmp(pis[i].ProjectName,"cjcomm",6)==0) {
				JProgramInfo->dev_info.jzq_login = 0;
				asyslog(LOG_WARNING, "cjcomm退出，清除登陆类型[%d]\n",JProgramInfo->dev_info.jzq_login);
			}
			if (ProjectKill(JProgramInfo->Projects[i]) == 1) {
				asyslog(LOG_WARNING, "程序已经关闭，正在重启...");
				JProgramInfo->Projects[i].ProjectState = NeedStart;
			}
			break;
		case NeedStart:
			if (ProjectExecute(JProgramInfo->Projects[i], i) != -1) {
				JProgramInfo->Projects[i].ProjectState = NowRun;
			}
			break;
		case NowRun:
			break;
		case NeedStop:
			asyslog(LOG_WARNING, "检测到程序需要关闭，名称[%s] PID[%d-%d]",
					pis[i].ProjectName, pis[i].ProjectID, i);
			if (ProjectKill(JProgramInfo->Projects[i]) == 1) {
				asyslog(LOG_WARNING, "程序已经关闭,无需重启...");
				JProgramInfo->Projects[i].ProjectState = NowRun;
			}
			break;
		}
		JProgramInfo->Projects[i].WaitTimes++;
	}
}

void checkDevReset() {
	static int state = 0;
	static int oldtime = 0;
	static int first_flag = 1;
	static int DevResetNum = 0;

	if (first_flag == 1) {
		first_flag = 0;
		DevResetNum = JProgramInfo->oi_changed.reset;
	}

	switch (state) {
	case 0:
		if (DevResetNum == JProgramInfo->oi_changed.reset) {
			return;
		} else {
			state = 1;
			oldtime = time(NULL);
		}
		break;
	case 1:
		if (abs(time(NULL) - oldtime) >= 10) { //掉电前电量处理
			system("reboot");
		}
		break;
	}
	asyslog(LOG_WARNING, "检测到设备需要复位");
	sleep(3);
	for (int i = 1; i < PROJECTCOUNT; i++) {
		if (JProgramInfo->Projects[i].ProjectID > 0) {
			JProgramInfo->Projects[i].ProjectState = NeedStop;
		}
	}
	DevResetNum = JProgramInfo->oi_changed.reset;
}

void checkRebootFile() {
	static int count = 0;

	if (count == 0) {
		FILE *fp = fopen("/nand/UpFiles/reboot", "r+");
		if (fp != NULL) {
			fclose(fp);
			system("rm /nand/UpFiles/reboot");
		}
		count++;
	}

	FILE *fp = fopen("/nand/UpFiles/reboot", "r+");
	if (fp != NULL) {
		fclose(fp);
		count++;
		if (count > 10) {
			system("reboot");
		}
	}
}

void CheckOnLineStatue() {
	static int reboot_count = 0;
	if (getZone("GW") == 0) {
		return;
	}
	if (JProgramInfo->dev_info.jzq_login == 0) {
		reboot_count++;
		if(reboot_count == 1800) {
			asyslog(LOG_ERR, "<异常>检查到设备30分钟不在线...");
		}
		if(reboot_count == 3600) {
			asyslog(LOG_ERR, "<异常>检查到设备1小时不在线...");
		}
		if (reboot_count > 2 * 3600) {
			asyslog(LOG_ERR, "<异常>检查到设备2小时不在线，重新启动设备...");
			sleep(1);
			system("reboot");
		}
	} else {
		reboot_count = 0;
	}
}

/**********************
 * 从文件中同步 ID
 * */
void sync_Id_fromFile()
{
	int i=0;
	CLASS_4001_4002_4003 id_public,id_698;
	if (readIdFile(&id_public)==1)
	{
		fprintf(stderr,"\n同步ID  读取同步文件中 :");
		for(i=0;i<OCTET_STRING_LEN;i++)
			fprintf(stderr," %02x",id_public.curstom_num[i]);

		readCoverClass(0x4001, 0, (void*)&id_698, 	sizeof(CLASS_4001_4002_4003), para_vari_save);
		fprintf(stderr,"\n同步ID  698配置文件中 :");
		for(i=0;i<OCTET_STRING_LEN;i++)
			fprintf(stderr," %02x",id_698.curstom_num[i]);

		if(memcmp(id_698.curstom_num,id_public.curstom_num,16)!=0)
		{
			fprintf(stderr,"\n不一样！需要同步到698中");
			memcpy(id_698.curstom_num,id_public.curstom_num,16);
			saveCoverClass(0x4001, 0, &id_698, sizeof(CLASS_4001_4002_4003), para_vari_save);
			sync();
			readCoverClass(0x4001, 0, (void*)&id_698, 	sizeof(CLASS_4001_4002_4003), para_vari_save);
			fprintf(stderr,"\n同步ID  重新读取698配置文件中 :");
			for(i=0;i<OCTET_STRING_LEN;i++)
				fprintf(stderr," %02x",id_698.curstom_num[i]);
		}
	}
}

int main(int argc, char *argv[])
{
    struct timeval start={}, end={};
    long  interval=0;
    int ProgsNum = 0;

	//此设置决定集中器电池工作，并保证在下电情况下，长按向下按键唤醒功能
	gpio_writebyte(DEV_BAT_SWITCH, (INT8S) 1);
	//将程序log记录到/nand/log_698目录下，防止nand下文件太多，导致程序启动较慢
	DIR *dir=NULL;
	dir = opendir("/nand/log_698");
	if(dir==NULL) {
		mkdir("/nand/log_698",0777);
	}

	printf("==================version==================\n");
//    printf("VERSION : %d\n", GL_VERSION);
    asyslog(LOG_INFO, "VERSION : %d\n", GL_VERSION);
    printf("==================version==================\n\n\n\n");

    //检查是否已经有程序在运行
    pid_t pids[128] = {0};
    if (prog_find_pid_by_name((INT8S *) argv[0], pids) > 1) {
        asyslog(LOG_ERR, "CJMAIN进程仍在运行,进程号[%d]，程序退出...", pids[0]);
        return EXIT_SUCCESS;
    }

    Createmq();
    CreateSem();
    InitSharedMem(argc, argv);
    ReadDeviceConfig(&JProgramInfo->cfg_para);
    asyslog(LOG_NOTICE, "\n当前运行类型：%d 型终端\n", JProgramInfo->cfg_para.device);
    asyslog(LOG_NOTICE, "\n当前运行地区：%s\n", JProgramInfo->cfg_para.zone);


    if (argc >= 2 && strncmp("all", argv[1], 3) == 0) {
        ProgsNum = ReadSystemInfo();
    }
    if(getZone("HuNan")==0) {
    	get_protocol_3761_tx_para();//湖南获取3761切换通信参数，在初始化其他操作之后进行
       	sync_Id_fromFile();//湖南要求3761和698两套程序设置的逻辑地址通用，因此程序运行起来后需要和公共ID文件同步
    }
    //点亮运行灯，初始化运行状态

//    JProgramInfo->powerState = PWR_ON;

    if(JProgramInfo->cfg_para.device == CCTT2) {
    	if(getZone("ShanDong") == 0 || getZone("ZheJiang") == 0) {
    		g_powerState = PWR_DOWN;
    		setRunLED(LED_CLOSE);
    	} else {
    		g_powerState = PWR_ON;
    		setRunLED(LED_LIGHT);
    	}
    } else {
    	setRunLED(LED_LIGHT);
    	g_powerState = PWR_ON;
    }

    while (1) {
        sleep(1);
		gettimeofday(&start, NULL);

		//喂狗
		if (access("/dos/cjgwn", 0) == 0) {
//			asyslog(LOG_INFO, "检测有升级目录，延长喂狗时间...");
			Watchdog(60);
		} else
			Watchdog(5);

		//每20分钟校时
		SyncRtc();
        if (JProgramInfo->cfg_para.device == CCTT1 || JProgramInfo->cfg_para.device == SPTF3) { //I型集中器，III型专变
            //电池检测掉电关闭设备，原写90s，湖南要求电池供电工作120s
            PowerOffToClose(120);
        } else if(JProgramInfo->cfg_para.device == CCTT2) {
        	if (getZone("ZheJiang") == 0) {
        		rebootWhenPwrDown(PWR_SHUT_CNT_ZJ);
        	} else if (getZone("ShanDong") == 0) {
        		rebootWhenPwrDown(PWR_SHUT_CNT_SD);
        	}
        }

        //点亮运行灯 循环前点亮一次
        setRunLED(1);

		//检车程序运行状态
		checkProgsState(ProgsNum);

		//检查程序更新
		Checkupdate();

		//集中器不在线重启
		CheckOnLineStatue();

		//检查设备是否需要重启
		checkDevReset();

		//检查系统升级文件
		checkRebootFile();

		gettimeofday(&end, NULL);

		interval = 1000000 * (end.tv_sec - start.tv_sec)
				+ (end.tv_usec - start.tv_usec);
		if (interval >= 1000000)
			asyslog(LOG_NOTICE, "main interval = %f(ms)\n", interval / 1000.0);

	}

	exit(1);
}
