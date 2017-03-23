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

#include "ParaDef.h"
#include "PublicFunction.h"
#include "AccessFun.h"
#include "cjmain.h"
#include "../lib698/dlt698.h"
#include "../libMq/libmmq.h"

static ProgramInfo* JProgramInfo = NULL;
static int mmqFds[MAX_MMQ_SIZE];

const static mmq_attribute mmq_register[] = {
    { cjcomm, PROXY_485_MQ_NAME, MAXSIZ_PROXY_NET, MAXNUM_PROXY_NET },
    { cjdeal, PROXY_NET_MQ_NAME, MAXSIZ_PROXY_485, MAXNUM_PROXY_485 },
    { cjdeal, TASKID_485_1_MQ_NAME, MAXSIZ_TASKID_QUEUE, MAXNUM_TASKID_QUEUE },
    { cjdeal, TASKID_485_2_MQ_NAME, MAXSIZ_TASKID_QUEUE, MAXNUM_TASKID_QUEUE }
};

void Runled(int state) {
    gpio_writebyte((char*)DEV_LED_RUN, state);
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
            system("hwclock -s");
        }
    }
}

void Watchdog(int count) //硬件看门狗
{
    int fd = -1;

    if (count < 2 || count > 20) {
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

//读取系统配置文件
int ReadSystemInfo() {
    int ProgsNum = 1;

    char Strbuf[256] = {};

    //初始化共享内存参数
    for (int i = 0; i < PROJECTCOUNT; i++) {
        memset((void*)&JProgramInfo->Projects[i], 0, sizeof(ProjectInfo));
    }

    memset(Strbuf, 0x00, sizeof(Strbuf));
    sprintf(Strbuf, "%s/systema.cfg", _CFGDIR_);

    FILE* fp = fopen(Strbuf, "r");
    memset(Strbuf, 0x00, sizeof(Strbuf));

    if (fp == NULL) {
        asyslog(LOG_ERR, "配置文件无法打开，或不存在，文件名(%s)，原因(%d)", Strbuf, errno);
        return -1;
    }

    while (fgets(Strbuf, sizeof(Strbuf), fp)) {
        int index = 0, foo = 0;

        //获取序号
        if (sscanf(Strbuf, "%d=%s %s %s %s", &index, JProgramInfo->Projects[0].ProjectName, JProgramInfo->Projects[0].argv[1],
                   JProgramInfo->Projects[0].argv[2], JProgramInfo->Projects[0].argv[3]) < 1) {
            continue;
        }

        //获取参数
        if (sscanf(Strbuf, "%d=%s %s %s %s", &foo, JProgramInfo->Projects[index].ProjectName, JProgramInfo->Projects[index].argv[1],
                   JProgramInfo->Projects[index].argv[2], JProgramInfo->Projects[index].argv[3]) < 1) {
            continue;
        }

        if (index < 0 || index > PROJECTCOUNT || strlen(JProgramInfo->Projects[index].ProjectName) < 2) {
            asyslog(LOG_WARNING, "发现不合规项目，文件名(%s)，序号(%d)", JProgramInfo->Projects[index].ProjectName, index);
            continue;
        }

        JProgramInfo->Projects[index].ProjectState = NeedStart;
        JProgramInfo->Projects[index].WaitTimes    = 0;
        sprintf(JProgramInfo->Projects[index].argv[0], "%d", index);

        memset(Strbuf, 0x00, sizeof(Strbuf));
        asyslog(LOG_WARNING, "需要启动程序[%s][%s][%d]", JProgramInfo->Projects[index].ProjectName, JProgramInfo->Projects[index].argv[0], index);
        ProgsNum++;
    }

    fclose(fp);

    asyslog(LOG_INFO, "读取配置文件，共计(%d)个程序需要启动...\n", ProgsNum);
    return ProgsNum;
}

void Createmq() {
    for (int i = 0; i < MAX_MMQ_SIZE; i++) {
        mmqFds[i] = 0;
    }

    for (int i = 0; i < sizeof(mmq_register) / sizeof(mmq_attribute); i++) {
        struct mq_attr attr;
        attr.mq_maxmsg  = mmq_register[i].maxnum;
        attr.mq_msgsize = mmq_register[i].maxsiz;

        if ((mmqFds[i] = mmq_create((INT8S*)mmq_register[i].name, &attr, O_RDONLY)) < 0) {
            asyslog(LOG_ERR, "消息队列创建失败，重建系统限制...");

            struct rlimit limit = { RLIM_INFINITY, RLIM_INFINITY };
            if (getrlimit(RLIMIT_MSGQUEUE, &limit) != 0) {
                asyslog(LOG_ERR, "重建系统限制时，获取系统参数失败...");
            } else {
                asyslog(LOG_ERR, "重建系统限制，系统参数[%d-%d]", limit.rlim_cur, limit.rlim_max);
                limit.rlim_cur = 1024000;
                limit.rlim_max = 1024000;
                setrlimit(RLIMIT_MSGQUEUE, &limit);
            }
            usleep(100 * 1000);
        }
        asyslog(LOG_ERR, "创建消息队列，消息号[%d]", mmqFds[i]);
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
        shm_unlink((const char*)"ProgramInfo");
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

    if (access("/dos/cjgwn", 0) == 0) {
        asyslog(LOG_INFO, "检测有升级目录，开始执行升级脚本...");
        system("chmod 777 /dos/cjgwn/update.sh");
        system("/dos/cjgwn/update.sh &");
        UpStates = 1;
    }
    return;
}

void CreateSem() {
    int val;
    sem_t* sem;

    //此设置决定集中器电池工作，并保证在下电情况下，长按向下按键唤醒功能
    gpio_writebyte(DEV_BAT_SWITCH, (INT8S)1);

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
void InitSharedMem(int argc, char* argv[]) {
    JProgramInfo = (ProgramInfo*)CreateShMem("ProgramInfo", sizeof(ProgramInfo), NULL);
    asyslog(LOG_ERR, "打开共享内存，地址[%d]，大小[%d]", JProgramInfo, sizeof(ProgramInfo));

    InitClass4300();

    readCoverClass(0x3100, 0, &JProgramInfo->event_obj.Event3100_obj, sizeof(JProgramInfo->event_obj.Event3100_obj), event_para_save);
    readCoverClass(0x3101, 0, &JProgramInfo->event_obj.Event3101_obj, sizeof(JProgramInfo->event_obj.Event3101_obj), event_para_save);
    readCoverClass(0x3104, 0, &JProgramInfo->event_obj.Event3104_obj, sizeof(JProgramInfo->event_obj.Event3104_obj), event_para_save);
    readCoverClass(0x3105, 0, &JProgramInfo->event_obj.Event3105_obj, sizeof(JProgramInfo->event_obj.Event3105_obj), event_para_save);
    readCoverClass(0x3106, 0, &JProgramInfo->event_obj.Event3106_obj, sizeof(JProgramInfo->event_obj.Event3106_obj), event_para_save);
    readCoverClass(0x3107, 0, &JProgramInfo->event_obj.Event3107_obj, sizeof(JProgramInfo->event_obj.Event3107_obj), event_para_save);
    readCoverClass(0x3108, 0, &JProgramInfo->event_obj.Event3108_obj, sizeof(JProgramInfo->event_obj.Event3108_obj), event_para_save);
    readCoverClass(0x3109, 0, &JProgramInfo->event_obj.Event3109_obj, sizeof(JProgramInfo->event_obj.Event3109_obj), event_para_save);
    readCoverClass(0x310A, 0, &JProgramInfo->event_obj.Event310A_obj, sizeof(JProgramInfo->event_obj.Event310A_obj), event_para_save);
    readCoverClass(0x310B, 0, &JProgramInfo->event_obj.Event310B_obj, sizeof(JProgramInfo->event_obj.Event310B_obj), event_para_save);
    readCoverClass(0x310C, 0, &JProgramInfo->event_obj.Event310C_obj, sizeof(JProgramInfo->event_obj.Event310C_obj), event_para_save);
    readCoverClass(0x310D, 0, &JProgramInfo->event_obj.Event310D_obj, sizeof(JProgramInfo->event_obj.Event310D_obj), event_para_save);
    readCoverClass(0x310E, 0, &JProgramInfo->event_obj.Event310E_obj, sizeof(JProgramInfo->event_obj.Event310E_obj), event_para_save);
    readCoverClass(0x310F, 0, &JProgramInfo->event_obj.Event310F_obj, sizeof(JProgramInfo->event_obj.Event310F_obj), event_para_save);
    readCoverClass(0x3110, 0, &JProgramInfo->event_obj.Event3110_obj, sizeof(JProgramInfo->event_obj.Event3110_obj), event_para_save);
    readCoverClass(0x3111, 0, &JProgramInfo->event_obj.Event3111_obj, sizeof(JProgramInfo->event_obj.Event3111_obj), event_para_save);
    readCoverClass(0x3112, 0, &JProgramInfo->event_obj.Event3112_obj, sizeof(JProgramInfo->event_obj.Event3112_obj), event_para_save);
    readCoverClass(0x3114, 0, &JProgramInfo->event_obj.Event3114_obj, sizeof(JProgramInfo->event_obj.Event3114_obj), event_para_save);
    readCoverClass(0x3115, 0, &JProgramInfo->event_obj.Event3115_obj, sizeof(JProgramInfo->event_obj.Event3115_obj), event_para_save);
    readCoverClass(0x3116, 0, &JProgramInfo->event_obj.Event3116_obj, sizeof(JProgramInfo->event_obj.Event3116_obj), event_para_save);
    readCoverClass(0x3117, 0, &JProgramInfo->event_obj.Event3117_obj, sizeof(JProgramInfo->event_obj.Event3117_obj), event_para_save);
    readCoverClass(0x3118, 0, &JProgramInfo->event_obj.Event3118_obj, sizeof(JProgramInfo->event_obj.Event3118_obj), event_para_save);
    readCoverClass(0x3119, 0, &JProgramInfo->event_obj.Event3119_obj, sizeof(JProgramInfo->event_obj.Event3119_obj), event_para_save);
    readCoverClass(0x311A, 0, &JProgramInfo->event_obj.Event311A_obj, sizeof(JProgramInfo->event_obj.Event311A_obj), event_para_save);
    readCoverClass(0x311B, 0, &JProgramInfo->event_obj.Event311B_obj, sizeof(JProgramInfo->event_obj.Event311B_obj), event_para_save);
    readCoverClass(0x311C, 0, &JProgramInfo->event_obj.Event311C_obj, sizeof(JProgramInfo->event_obj.Event311C_obj), event_para_save);
    readCoverClass(0x3200, 0, &JProgramInfo->event_obj.Event3200_obj, sizeof(JProgramInfo->event_obj.Event3200_obj), event_para_save);
    readCoverClass(0x3201, 0, &JProgramInfo->event_obj.Event3201_obj, sizeof(JProgramInfo->event_obj.Event3201_obj), event_para_save);
    readCoverClass(0x3202, 0, &JProgramInfo->event_obj.Event3202_obj, sizeof(JProgramInfo->event_obj.Event3202_obj), event_para_save);
    readCoverClass(0x3203, 0, &JProgramInfo->event_obj.Event3203_obj, sizeof(JProgramInfo->event_obj.Event3203_obj), event_para_save);
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
        asyslog(LOG_WARNING, "停止进程，ID(%d)", proinfo.ProjectID);
        kill(proinfo.ProjectID, SIGTERM);
        return 1;
    }
    return 0;
}

//激活外部程序
int ProjectExecute(ProjectInfo proinfo, int i) {
    pid_t pid = LAPI_Fork2();

    if (pid == -1) {
        asyslog(LOG_WARNING, "程序启动失败[%s],原因(%d)", (char*)proinfo.ProjectName, errno);
        return -1;
    }

    if (pid == 0) {
        //子进程代码运行部分
        asyslog(LOG_WARNING, "程序启动[%s %s %s %s %s]", (char*)proinfo.ProjectName, proinfo.argv[0], proinfo.argv[1], proinfo.argv[2], proinfo.argv[3]);
        execlp((char*)proinfo.ProjectName, (char*)proinfo.ProjectName, proinfo.argv[0], proinfo.argv[1], proinfo.argv[2], proinfo.argv[3], NULL);
        return 0;
    }
    return 1;
}

//检查外部程序是否正常运行,是否超时
void ProjectCheck(ProjectInfo* proinfo) {
    if (strlen((char*)proinfo->ProjectName) < 2) {
        return;
    }
    if (proinfo->ProjectState == NowRun) {
        if (proinfo->WaitTimes > PRO_WAIT_COUNT) {
            proinfo->ProjectState = NeedKill;
            proinfo->WaitTimes    = 0;
        }
    }
}

void checkProgsState(int ProgsNum) {
    ProjectInfo* pis = JProgramInfo->Projects;

    for (int i = 1; i < ProgsNum; i++) {
        ProjectCheck(&JProgramInfo->Projects[i]);
        switch (JProgramInfo->Projects[i].ProjectState) {
            case NeedKill:
                asyslog(LOG_WARNING, "检测到程序异常，名称[%s] PID[%d-%d]", pis[i].ProjectName, pis[i].ProjectID, i);
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
                asyslog(LOG_WARNING, "检测到程序需要关闭，名称[%s] PID[%d-%d]", pis[i].ProjectName, pis[i].ProjectID, i);
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
    static int state      = 0;
    static int oldtime    = 0;
    static int first_flag = 1;
    static int DevResetNum;

    if (first_flag == 1) {
        first_flag  = 0;
        DevResetNum = JProgramInfo->oi_changed.reset;
    }

    switch (state) {
        case 0:
            if (DevResetNum == JProgramInfo->oi_changed.reset) {
                return;
            } else {
                state   = 1;
                oldtime = time(NULL);
            }
            break;
        case 1:
            if (abs(time(NULL) - oldtime) >= 5) {
                system("reboot");
            }
            break;
    }
    asyslog(LOG_WARNING, "检测到设备需要复位");

    for (int i = 1; i < PROJECTCOUNT; i++) {
        if (JProgramInfo->Projects[i].ProjectID > 0) {
            JProgramInfo->Projects[i].ProjectState = NeedStop;
        }
    }
    DevResetNum = JProgramInfo->oi_changed.reset;
    return (time(NULL));
}

int main(int argc, char* argv[]) {
    int ProgsNum = 0;

    //检查是否已经有程序在运行
    pid_t pids[128];
    if (prog_find_pid_by_name((INT8S*)argv[0], pids) > 1) {
        asyslog(LOG_ERR, "CJMAIN进程仍在运行,进程号[%d]，程序退出...", pids[0]);
        return EXIT_SUCCESS;
    }

    Createmq();
    CreateSem();
    InitSharedMem(argc, argv);

    if (argc >= 2 && strncmp("all", argv[1], 3) == 0) {
        ProgsNum = ReadSystemInfo();
    }

    while (1) {
        sleep(1);

        //喂狗
        Watchdog(5);

        //点亮运行灯
        Runled(1);

        //每20分钟校时
        SyncRtc();

        //检车程序运行状态
        checkProgsState(ProgsNum);

        //检查程序更新
        Checkupdate();

        //检查设备是否需要重启
        checkDevReset();
    }

    exit(1);
}
