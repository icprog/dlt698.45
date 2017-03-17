/*
 * cjdev.c
 *
 *  Created on: Feb 9, 2017
 *      Author: ava
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "StdDataType.h"
#include "Shmem.h"
#include "EventObject.h"
#include "PublicFunction.h"
#include "AccessFun.h"
#include "Objectdef.h"
#include "ParaDef.h"
#include "Shmem.h"
#include "main.h"
void write_apn(char* apn) {
    FILE* fp;
    fp = fopen("/etc/ppp/gprs-connect-chat", "w");
    if (fp == NULL) {
        return;
    }
    fprintf(fp, "TIMEOUT        5\n");
    fprintf(fp, "ABORT        \'\\nBUSY\\r\'\n");
    fprintf(fp, "ABORT        \'\\nNO ANSWER\\r\'\n");
    fprintf(fp, "ABORT        \'\\nRINGING\\r\\n\\r\\nRINGING\\r\'\n");
    fprintf(fp, "ABORT        \'\\nNO CARRIER\\r\'\n");
    fprintf(fp, "\'\' \\rAT\n");
    fprintf(fp, "TIMEOUT        5\n");
    fprintf(fp, "\'OK-+++\\c-OK\'    ATH\n");
    fprintf(fp, "TIMEOUT        20\n");
    fprintf(fp, "OK        AT+CREG?\n");
    fprintf(fp, "TIMEOUT        10\n");
    fprintf(fp, "OK        AT+CGATT=1\n");
    fprintf(fp, "TIMEOUT        300\n");
    fprintf(fp, "OK        AT+CGATT?\n");
    fprintf(fp, "OK        AT+CFUN=1\n");
    fprintf(fp, "TIMEOUT        5\n");
    fprintf(fp, "OK        AT+CPIN?\n");
    fprintf(fp, "TIMEOUT        5\n");
    fprintf(fp, "OK        AT+CSQ\n");
    fprintf(fp, "TIMEOUT        5\n");
    fprintf(fp, "OK        AT+CGDCONT=1,\"IP\",\"%s\"\n", apn);
    fprintf(fp, "OK        ATDT*99***1#\n");
    fprintf(fp, "CONNECT \'\'\n");
    fclose(fp);
    fp = NULL;
}

void write_userpwd(unsigned char* user, unsigned char* pwd, unsigned char* apn) {
    FILE* fp = NULL;
    fp       = fopen("/etc/ppp/chap-secrets", "w");
    fprintf(fp, "\"%s\" * \"%s\" *", user, pwd);
    fclose(fp);

    fp = fopen("/etc/ppp/pap-secrets", "w");
    fprintf(fp, "\"%s\" * \"%s\" *", user, pwd);
    fclose(fp);

    fp = fopen("/etc/ppp/peers/cdma2000", "w");
    fprintf(fp, "/dev/mux0\n");
    fprintf(fp, "115200\n");
    fprintf(fp, "modem\n");
    fprintf(fp, "debug\n");
    fprintf(fp, "nodetach\n");
    fprintf(fp, "usepeerdns\n");
    fprintf(fp, "noipdefault\n");
    fprintf(fp, "defaultroute\n");
    fprintf(fp, "user \"%s\"\n", user);
    fprintf(fp, "0.0.0.0:0.0.0.0\n");
    fprintf(fp, "ipcp-accept-local\n");
    fprintf(fp, "connect 'chat -s -v -f /etc/ppp/cdma2000-connect-chat'\n");
    fprintf(fp, "disconnect \"chat -s -v -f /etc/ppp/gprs-disconnect-chat\"\n");
    fclose(fp);

    fp = fopen("/etc/ppp/cdma2000-connect-chat", "w");
    fprintf(fp, "TIMEOUT        5\n");
    fprintf(fp, "ABORT        \"DELAYED\"\n");
    fprintf(fp, "ABORT        \"BUSY\"\n");
    fprintf(fp, "ABORT        \"ERROR\"\n");
    fprintf(fp, "ABORT        \"NO DIALTONE\"\n");
    fprintf(fp, "ABORT        \"NO CARRIER\"\n");
    fprintf(fp, "'' AT\n");
    fprintf(fp, "TIMEOUT        5\n");
    fprintf(fp, "'OK-+++\c-OK' ATZ\n");
    fprintf(fp, "TIMEOUT        20\n");
    fprintf(fp, "OK        AT+CREG?\n");
    fprintf(fp, "TIMEOUT        10\n");
    fprintf(fp, "OK        AT$MYNETCON=0,\"USERPWD\",\"%s,%s\"\n", user, pwd);
    fprintf(fp, "TIMEOUT        10\n");
    fprintf(fp, "OK        AT$MYNETCON=0,\"APN\",\"%s\"\n", apn);
    fprintf(fp, "TIMEOUT        10\n");
    fprintf(fp, "OK        AT$MYNETCON=0,\"AUTH\",1\n");
    fprintf(fp, "TIMEOUT        10\n");
    fprintf(fp, "OK ATDT#777\n");
    fprintf(fp, "CONNECT ''\n");
    fclose(fp);
}

void printF203() {
    CLASS_f203 oif203 = {};
    readCoverClass(0xf203, 0, &oif203, sizeof(CLASS_f203), para_vari_save);
    fprintf(stderr, "[F203]开关量输入\n");
    fprintf(stderr, "逻辑名 %s\n", oif203.class22.logic_name);
    fprintf(stderr, "设备对象数量：%d\n", oif203.class22.device_num);
    fprintf(stderr, "属性2：ST=%d_%d_%d_%d %d_%d_%d_%d\n", oif203.statearri.stateunit[0].ST, oif203.statearri.stateunit[1].ST, oif203.statearri.stateunit[2].ST,
            oif203.statearri.stateunit[3].ST, oif203.statearri.stateunit[4].ST, oif203.statearri.stateunit[5].ST, oif203.statearri.stateunit[6].ST,
            oif203.statearri.stateunit[7].ST);
    fprintf(stderr, "属性2：CD=%d_%d_%d_%d %d_%d_%d_%d\n\n", oif203.statearri.stateunit[0].CD, oif203.statearri.stateunit[1].CD,
            oif203.statearri.stateunit[2].CD, oif203.statearri.stateunit[3].CD, oif203.statearri.stateunit[4].CD, oif203.statearri.stateunit[5].CD,
            oif203.statearri.stateunit[6].CD, oif203.statearri.stateunit[7].CD);
    fprintf(stderr, "属性4：接入标志=%02x\n", oif203.state4.StateAcessFlag);
    fprintf(stderr, "属性4：属性标志=%02x\n", oif203.state4.StatePropFlag);
}

void printF101() {
    CLASS_F101 f101 = {};
    int i           = 0;
    readCoverClass(0xf101, 0, &f101, sizeof(CLASS_F101), para_vari_save);
    fprintf(stderr, "[F101]安全模式参数\n");
    if (f101.active == 0) {
        fprintf(stderr, "属性2:安全模式选择：不启用安全模式参数\n");
    } else if (f101.active == 1) {
        fprintf(stderr, "属性2:安全模式选择：启用安全模式参数\n");
    } else {
        fprintf(stderr, "属性2:安全模式选择[0,1]：读取无效值：%d\n", f101.active);
    }
    fprintf(stderr, "安全模式参数个数：%d\n", f101.modelnum);
    for (i = 0; i < f101.modelnum; i++) {
        fprintf(stderr, "OI=%04x 安全模式=%d\n", f101.modelpara[i].oi, f101.modelpara[i].model);
    }
}

void SetF101(int argc, char* argv[]) {
    CLASS_F101 f101 = {};
    int tmp         = 0;
    if (strcmp(argv[2], "init") == 0) {
        memset(&f101, 0, sizeof(CLASS_F101));
        f101.active = 1; //初始化启用
        saveCoverClass(0xf101, 0, &f101, sizeof(CLASS_F101), para_init_save);
    }
    if (strcmp(argv[2], "set") == 0) {
        memset(&f101, 0, sizeof(CLASS_F101));
        readCoverClass(0xf101, 0, &f101, sizeof(CLASS_F101), para_vari_save);
        sscanf(argv[4], "%d", &tmp);
        f101.active = tmp;
        saveCoverClass(0xf101, 0, &f101, sizeof(CLASS_F101), para_vari_save);
    }
}
void getipnum(MASTER_STATION_INFO* info, char* argv) {
    int ipnum1 = 0, ipnum2 = 0, ipnum3 = 0, ipnum4 = 0, port1;
    sscanf((const char*)argv, "%d.%d.%d.%d:%d", &ipnum1, &ipnum2, &ipnum3, &ipnum4, &port1);
    info[0].port  = port1;
    info[0].ip[1] = (INT8U)ipnum1;
    info[0].ip[2] = (INT8U)ipnum2;
    info[0].ip[3] = (INT8U)ipnum3;
    info[0].ip[4] = (INT8U)ipnum4;
}

void SetUsrPwd(int argc, char* argv[]) {
    CLASS25 class4500;
    memset(&class4500, 0x00, sizeof(class4500));

    if (argc == 5) {
        write_userpwd(argv[2], argv[3], argv[4]);
        readCoverClass(0x4500, 0, &class4500, sizeof(CLASS25), para_vari_save);
        class4500.commconfig.userName[0] = strlen(argv[2]);
        memcpy(&class4500.commconfig.userName[1], argv[2], strlen(argv[2]));
        class4500.commconfig.passWord[0] = strlen(argv[3]);
        memcpy(&class4500.commconfig.passWord[1], argv[3], strlen(argv[3]));
        saveCoverClass(0x4500, 0, &class4500, sizeof(CLASS25), para_vari_save);
    } else {
        readCoverClass(0x4500, 0, &class4500, sizeof(CLASS25), para_vari_save);
        fprintf(stderr, "用户名：%s\t密码：%s\n", &class4500.commconfig.userName[1], &class4500.commconfig.passWord[1]);
    }
}

void SetIPort(int argc, char* argv[]) {
    CLASS25 class4500;
    MASTER_STATION_INFO_LIST master;

    memset(&master, 0, sizeof(MASTER_STATION_INFO_LIST));
    memset(&class4500, 0, sizeof(CLASS25));
    readCoverClass(0x4500, 0, &class4500, sizeof(CLASS25), para_vari_save);
    fprintf(stderr, "\n先读出 主IP %d.%d.%d.%d :%d\n", class4500.master.master[0].ip[1], class4500.master.master[0].ip[2], class4500.master.master[0].ip[3],
            class4500.master.master[0].ip[4], class4500.master.master[0].port);
    fprintf(stderr, "\n先读出 备IP %d.%d.%d.%d :%d\n", class4500.master.master[1].ip[1], class4500.master.master[1].ip[2], class4500.master.master[1].ip[3],
            class4500.master.master[1].ip[4], class4500.master.master[1].port);
    int i   = 0;
    int num = argc - 2;
    if (num > 0 && num < 4) {
        master.masternum = num;
        for (i = 0; i < num; i++) {
            getipnum(&master.master[i], argv[2 + i]);
        }
        memcpy(&class4500.master, &master, sizeof(MASTER_STATION_INFO_LIST));
        fprintf(stderr, "\n存储前 主IP %d.%d.%d.%d :%d\n", class4500.master.master[0].ip[1], class4500.master.master[0].ip[2], class4500.master.master[0].ip[3],
                class4500.master.master[0].ip[4], class4500.master.master[0].port);
        fprintf(stderr, "\n存储前 备IP %d.%d.%d.%d :%d\n", class4500.master.master[1].ip[1], class4500.master.master[1].ip[2], class4500.master.master[1].ip[3],
                class4500.master.master[1].ip[4], class4500.master.master[1].port);
        saveCoverClass(0x4500, 0, &class4500, sizeof(CLASS25), para_vari_save);
    }
}

void SetNetIPort(int argc, char* argv[]) {
    CLASS26 class4510;
    MASTER_STATION_INFO_LIST master;

    memset(&master, 0, sizeof(MASTER_STATION_INFO_LIST));
    memset(&class4510, 0, sizeof(CLASS26));
    readCoverClass(0x4510, 0, &class4510, sizeof(CLASS26), para_vari_save);
    fprintf(stderr, "\n先读出 主IP %d.%d.%d.%d :%d\n", class4510.master.master[0].ip[1], class4510.master.master[0].ip[2], class4510.master.master[0].ip[3],
            class4510.master.master[0].ip[4], class4510.master.master[0].port);
    fprintf(stderr, "\n先读出 备IP %d.%d.%d.%d :%d\n", class4510.master.master[1].ip[1], class4510.master.master[1].ip[2], class4510.master.master[1].ip[3],
            class4510.master.master[1].ip[4], class4510.master.master[1].port);
    int i   = 0;
    int num = argc - 2;
    if (num > 0 && num < 4) {
        master.masternum = num;
        for (i = 0; i < num; i++) {
            getipnum(&master.master[i], argv[2 + i]);
        }
        memcpy(&class4510.master, &master, sizeof(MASTER_STATION_INFO_LIST));
        fprintf(stderr, "\n存储前 主IP %d.%d.%d.%d :%d\n", class4510.master.master[0].ip[1], class4510.master.master[0].ip[2], class4510.master.master[0].ip[3],
                class4510.master.master[0].ip[4], class4510.master.master[0].port);
        fprintf(stderr, "\n存储前 备IP %d.%d.%d.%d :%d\n", class4510.master.master[1].ip[1], class4510.master.master[1].ip[2], class4510.master.master[1].ip[3],
                class4510.master.master[1].ip[4], class4510.master.master[1].port);
        saveCoverClass(0x4510, 0, &class4510, sizeof(CLASS26), para_vari_save);
    }
}

void SetID(int argc, char* argv[]) {
    CLASS_4001_4002_4003 classtmp = {};
    char idbuf[VISIBLE_STRING_LEN - 1];
    long int len = 0, id = 0, i = 0;
    char a;
    int tmpval = 0;

    memset(&classtmp, 0, sizeof(CLASS_4001_4002_4003));
    //
    if (argc > 2) {
        memset(idbuf, 0, sizeof(idbuf));
        len                     = argc - 2;
        classtmp.curstom_num[0] = len;
        for (i = 0; i < len; i++) {
            sscanf(argv[2 + i], "%02x", &tmpval);
            classtmp.curstom_num[i + 1] = tmpval;
        }
        for (i = 0; i < 16; i++) {
            fprintf(stderr, "%02x ", classtmp.curstom_num[i]);
        }
        saveCoverClass(0x4001, 0, &classtmp, sizeof(CLASS_4001_4002_4003), para_vari_save);
    } else {
        readCoverClass(0x4001, 0, &classtmp, sizeof(CLASS_4001_4002_4003), para_vari_save);
        fprintf(stderr, "\n通信地址[%d]:", classtmp.curstom_num[0]);
        for (i = 0; i < classtmp.curstom_num[0]; i++) {
            fprintf(stderr, "%02x ", classtmp.curstom_num[i + 1]);
        }
    }
}

void SetHEART(int argc, char* argv[]) {
	CLASS25 class4500={};
	memset(&class4500,0,sizeof(CLASS25));
	readCoverClass(0x4500, 0, &class4500, sizeof(CLASS25), para_vari_save);
	int tmpval=0;//分钟
    if (argc > 2) {
        sscanf(argv[2], "%d", &tmpval);
        class4500.commconfig.heartBeat=tmpval;
        saveCoverClass(0x4500, 0, &class4500, sizeof(CLASS25), para_vari_save);
        ProgramInfo* JProgramInfo=NULL;
        JProgramInfo = OpenShMem("ProgramInfo",sizeof(ProgramInfo),NULL);
        JProgramInfo->oi_changed.oi4500++;
    } else {
        fprintf(stderr, "\n心跳周期:%d s \n",class4500.commconfig.heartBeat);
    }
}


void SetApn(int argc, char* argv[]) {
    CLASS25 class4500;
    COMM_CONFIG_1 config1;
    char apnbuf[VISIBLE_STRING_LEN - 1];
    memset(&config1, 0, sizeof(COMM_CONFIG_1));
    memset(&class4500, 0, sizeof(CLASS25));
    readCoverClass(0x4500, 0, &class4500, sizeof(CLASS25), para_vari_save);
    fprintf(stderr, "\n先读出 APN : %s\n", &class4500.commconfig.apn[1]);
    if (argc > 2) {
        memset(apnbuf, 0, sizeof(apnbuf));
        if (sscanf(argv[2], "%s", apnbuf)) {
            memcpy(&class4500.commconfig.apn[1], apnbuf, sizeof(apnbuf));
            fprintf(stderr, "\n存储前 APN : %s\n", &class4500.commconfig.apn[1]);
            saveCoverClass(0x4500, 0, &class4500, sizeof(CLASS25), para_vari_save);
            write_apn((char*)&class4500.commconfig.apn[1]);
        }
    }
}
void Init_4500() {
    CLASS25 obj;
    memset(&obj, 0, sizeof(obj));
    obj.commconfig.workModel      = 1;
    obj.commconfig.onlineType     = 0;
    obj.commconfig.connectType    = 0;
    obj.commconfig.appConnectType = 0;
    memcpy(&obj.commconfig.apn[1], "cmcc", 4);
    memcpy(&obj.commconfig.userName[1], "user", 4);
    memcpy(&obj.commconfig.passWord[1], "user", 4);
    memcpy(&obj.commconfig.proxyIp[1], "0.0.0.0", 7);
    obj.commconfig.proxyPort   = 0;
    obj.commconfig.timeoutRtry = 3;
    obj.commconfig.heartBeat   = 60;
    memcpy(&obj.master.master[0].ip[1], "192.168.0.97", sizeof("192.168.0.97"));
    obj.master.master[0].port = 5022;

    saveCoverClass(0x4500, 0, (void*)&obj, sizeof(CLASS25), para_init_save);
}

void inoutdev_process(int argc, char* argv[]) {
    int tmp   = 0;
    OI_698 oi = 0;

    if (argc >= 2) { // dev pro
        if (strcmp(argv[1], "dev") == 0) {
            sscanf(argv[3], "%04x", &tmp);
            oi = tmp;
            if (strcmp(argv[2], "pro") == 0) {
                switch (oi) {
                    case 0xf203:
                        printF203();
                        break;
                    case 0xf101:
                        printF101();
                }
            } else {
                switch (oi) {
                    case 0xf101:
                        SetF101(argc, argv);
                        break;
                }
            }
            if (strcmp(argv[2], "init") == 0) {
                sscanf(argv[3], "%04x", &tmp);
                oi = tmp;
                switch (oi) {
                    case 0x4500:
                        Init_4500();
                        break;
                }
            }
        }
    }
}
void cjfangAn() {
    CLASS_6017 obj6017;
}
void SetF201(int argc, char* argv[]) {
    CLASS_f201 oif201 = {};

    if (argc != 6 || (atoi(argv[2]) < 0 && atoi(argv[2]) > 255) || (atoi(argv[3]) < 0 && atoi(argv[3]) > 2) || (atoi(argv[4]) > 5 && atoi(argv[4]) > 8) ||
        (atoi(argv[5]) < 0 && atoi(argv[5]) > 2)) {
        fprintf(stderr, "使用方法，括号内的数字是需要设置的参数\n");

        fprintf(stderr, "波特率：bps300(0),bps600(1),bps1200(2),bps2400(3),bps4800(4),bps7200(5),\nbps9600(6),bps19200(7),bps38400(8),bps57600(9),bps115200(10)"
                        ",auto(255)\n");
        fprintf(stderr, "校验方式：none(0), odd(1), even(2)\n");
        fprintf(stderr, "数据位：d5(5), d6(6), d7(7),d8(8)\n");
        fprintf(stderr, "停止位：stop0(0), stop1(1), stop2(2)\n");

        if (readCoverClass(0xf201, 0, &oif201, sizeof(CLASS_f201), para_vari_save) > 0) {
            fprintf(stderr, "当前：波特率(%d)，校验方式(%d)，数据位(%d)，停止位(%d)\n", oif201.devpara.baud, oif201.devpara.verify, oif201.devpara.databits,
                    oif201.devpara.stopbits);
        }
        return;
    }

    oif201.devpara.baud     = atoi(argv[2]);
    oif201.devpara.verify   = atoi(argv[3]);
    oif201.devpara.databits = atoi(argv[4]);
    oif201.devpara.stopbits = atoi(argv[5]);
    saveCoverClass(0xf201, 0, &oif201, sizeof(CLASS_f201), para_vari_save);
}

void SetF202(int argc, char* argv[]) {
    CLASS_f202 oif202 = {};

    if (argc != 6 || (atoi(argv[2]) < 0 && atoi(argv[2]) > 255) || (atoi(argv[3]) < 0 && atoi(argv[3]) > 2) || (atoi(argv[4]) > 5 && atoi(argv[4]) > 8) ||
        (atoi(argv[5]) < 0 && atoi(argv[5]) > 2)) {
        fprintf(stderr, "使用方法，括号内的数字是需要设置的参数\n");

        fprintf(stderr, "波特率：bps300(0),bps600(1),bps1200(2),bps2400(3),bps4800(4),bps7200(5),\nbps9600(6),bps19200(7),bps38400(8),bps57600(9),bps115200(10)"
                        ",auto(255)\n");
        fprintf(stderr, "校验方式：none(0), odd(1), even(2)\n");
        fprintf(stderr, "数据位：d5(5), d6(6), d7(7),d8(8)\n");
        fprintf(stderr, "停止位:stop0(0), stop1(1), stop2(2)\n");

        if (readCoverClass(0xf202, 0, &oif202, sizeof(CLASS_f202), para_vari_save) > 0) {
            fprintf(stderr, "当前：波特率(%d)，校验方式(%d)，数据位(%d)，停止位(%d)\n", oif202.devpara.baud, oif202.devpara.verify, oif202.devpara.databits,
                    oif202.devpara.stopbits);
        }
        return;
    }

    oif202.devpara.baud     = atoi(argv[2]);
    oif202.devpara.verify   = atoi(argv[3]);
    oif202.devpara.databits = atoi(argv[4]);
    oif202.devpara.stopbits = atoi(argv[5]);
    saveCoverClass(0xf202, 0, &oif202, sizeof(CLASS_f202), para_vari_save);
}
