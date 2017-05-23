/*
 * v645.c
 *
 *  Created on: 2014-2-28
 *      Author: Administrator
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include "StdDataType.h"
#include "Shmem.h"
#include "PublicFunction.h"

extern INT32S comfd;
extern INT8U addr[6];//表地址，低在前，高在后
extern void dealProcess();
extern void acs_process();

ProgramInfo *JProgramInfo = NULL;

int ProIndex = 0;

//处理现场
void QuitProcess() {
    if (comfd > 0) close(comfd);
    shmm_unregister("ProgramInfo", sizeof(ProgramInfo));
    fprintf(stderr, "\n\r cj645 quit xxx\n\r");
    exit(0);
}

int InitPro(ProgramInfo **prginfo, int argc, char *argv[]) {
        *prginfo = OpenShMem("ProgramInfo", sizeof(ProgramInfo), NULL);
        ProIndex = atoi(argv[1]);
        fprintf(stderr, "\n%s start", (*prginfo)->Projects[ProIndex].ProjectName);
        (*prginfo)->Projects[ProIndex].ProjectID = getpid();//保存当前进程的进程号
        fprintf(stderr, "ProjectID[%d]=%d\n", ProIndex, (*prginfo)->Projects[ProIndex].ProjectID);
        return 1;
 }

/*
 * 485口通信测试,两个串口互相发送接收
 * II型集中器, port1 = 1, port2 = 4 ,485_I 与 485_III 互相抄读
 *
 * */
int  vs485_test(int port1,int port2)
{
    int Test_485_result = 1;

    INT8U msg[256];
    INT8U res[256];

    memset(msg, 0x00, sizeof(msg));
    memset(res, 0x00, sizeof(res));

    int comfd1 = OpenCom(port1, 9600, (INT8U *) "even", 1, 8);
    int comfd2 = OpenCom(port2, 9600, (INT8U *) "even", 1, 8);

    for (int i = 0; i < 256; ++i) {
        msg[i] = i;
    }

    write(comfd1, msg, sizeof(msg));
    sleep(1);
    int lens = read(comfd2, res, sizeof(res));
    printf("收到数据[%d]字节\n", lens);

    for (int j = 0; j < 256; ++j) {
        if (msg[j] != res[j]) {
            Test_485_result = 0;
        }
    }

    memset(msg, 0x00, sizeof(msg));
    memset(res, 0x00, sizeof(res));

    for (int i = 0; i < 256; ++i) {
        msg[i] = i;
    }

    write(comfd2, msg, sizeof(msg));
    sleep(1);
    read(comfd1, res, sizeof(res));

    for (int j = 0; j < 256; ++j) {
        if (msg[j] != res[j]) {
            Test_485_result = 0;
        }
    }

    close(comfd1);
    close(comfd2);

    return Test_485_result;
}

//主程序
int main(int argc, char *argv[])
{
    printf("Checking11...\n");
    int		Test_485_result=0;
    INT8U comport = 2;

    system("rm /nand/check.log");
    Test_485_result = vs485_test(1,4);
    if (Test_485_result == 1) {
        system("echo 485OK >> /nand/check.log");
    }

    system("cj esam 2>> /nand/check.log");

    JProgramInfo = OpenShMem("ProgramInfo", sizeof(ProgramInfo), NULL);
    fprintf(stderr, "\ncj645 start ....\n\r");
    if (JProgramInfo->cfg_para.device == 2) {    //II型集中器
        comport = 2;
    } else {
        comport = 4;
    }

    fprintf(stderr, "open /dev/ttyS%d\n", comport);

    if ((comfd = OpenCom(comport, 2400, (INT8U *) "even", 1, 8)) < 1) {
        fprintf(stderr, "OpenCom645 ERR!!! ........................\n");
    }

    system("cjcomm 2 &");

    acs_process();
    dealProcess();

    usleep(500*1000);

    QuitProcess(0);
    return EXIT_SUCCESS;
}
