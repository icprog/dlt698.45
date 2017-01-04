#include "msgr.h"
#include "handle.h"
#include "mtypes.h"
#include "libmmq.h"
#include "libgdw3761.h"

#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

INT8U transread_point(char* revbuf, TransDataInfo* allrtransinfo, INT8U address_A3) {
    TransDataInfo* transreal_info = (TransDataInfo*)revbuf;
    fprintf(stderr, "[vMsgr][transread_point]transidata=%d,transiend=%d\n", transreal_info->idata, transreal_info->iend);
    if (transreal_info->idata == 0) {
        if (allrtransinfo != NULL) {
            free(allrtransinfo);
            allrtransinfo = NULL;
        }
        allrtransinfo = malloc((transreal_info->iend + 1) * sizeof(TransDataInfo));
    }
    if (allrtransinfo != NULL) {
        memcpy(allrtransinfo + transreal_info->idata, transreal_info, sizeof(TransDataInfo));

        if (transreal_info->idata == transreal_info->iend) {
            gdw3761_retransfermsgto3761(allrtransinfo, transreal_info->iend + 1, address_A3);
        }
    }
    return 0;
}

int report_master(char* rv_msg, INT8U address_A3) {
    Erc_send* erc_send;
    erc_send = (Erc_send*)rv_msg;

    gdw3761_setCallback(netWrite);
    if (erc_send->Buf[0] == 14 && shmm_getpara()->f9.Flag_EvtEffect[13] == 1) {
        if (shmm_getpara()->f9.Flag_EvtImp[13] == 1)
            gdw3761_autoreport_class3(1, address_A3);
        else
            gdw3761_autoreport_class3(2, address_A3);
    } else {
        gdw3761_autoreport_class3(1, address_A3);
    }

    if (erc_send->Buf[0] == 14) {
        ERC14 _erc14;
        memset(&_erc14, 0, sizeof(ERC14));
        memcpy(&_erc14, erc_send->Buf, sizeof(ERC14));
        TmS Ting_time;
        TmS Shang_time;
        Ting_time.Year   = _erc14.Ting_Time.BCD05;
        Ting_time.Month  = _erc14.Ting_Time.BCD04;
        Ting_time.Day    = _erc14.Ting_Time.BCD03;
        Ting_time.Hour   = _erc14.Ting_Time.BCD02;
        Ting_time.Minute = _erc14.Ting_Time.BCD01;

        Shang_time.Year   = _erc14.Shang_Time.BCD05;
        Shang_time.Month  = _erc14.Shang_Time.BCD04;
        Shang_time.Day    = _erc14.Shang_Time.BCD03;
        Shang_time.Hour   = _erc14.Shang_Time.BCD02;
        Shang_time.Minute = _erc14.Shang_Time.BCD01;

        if ((_erc14.Shang_Time.BCD01 == 0xee && _erc14.Shang_Time.BCD02 == 0xee && _erc14.Shang_Time.BCD03 == 0xee &&
             _erc14.Shang_Time.BCD04 == 0xee && _erc14.Shang_Time.BCD05 == 0xee) ||
            (tmcmp(&Ting_time, &Shang_time)) > 0) {
        }
    }
    return 0;
}

void mmqRead(struct aeEventLoop* eventLoop, int fd, void* clientData, int mask) {
	mmqAgent* mmq = (mmqAgent*) clientData;
	mNetworker* net = getNetStruct();

	mmq_head msg_head;
	char buf[MAXSIZ_IFR_REQ];

	memset(buf, 0, sizeof(buf));
	memset(&msg_head, 0, sizeof(mmq_head));
	int cnt = mmq_get(mmq->fd, 0, &msg_head, (INT8S*) buf);
	if (cnt != -1 && (msg_head.pid > 0 && msg_head.pid < vd + 1)) {
		fprintf(stderr, "[vMsgr]消息队列接受消息长度[%d]\n", msg_head.bufsiz);

		switch (msg_head.cmd) {
		case REPORT:
			//允许终端主动通话 且允许主动上报 ==0x0101s
			if (shmm_getdevstat()->controls.autoreport) {
				report_master((char*) buf, net->address_A3);
			}
		case READPOINT:
			read_point(buf, &mmq->allrealinfo);
			break;
		case TRANSDATA:
			transread_point(buf, mmq->allrtransinfo, 0);
			break;
		}
	}
}

INT8S all_tomsg(gdm_type mqtype, INT8U* buf, INT16U len, INT8S* name) {
    INT8S ret = 0;
    struct mq_attr attr;
    mqd_t currmack;
    mmq_head msg_head;
    msg_head.pid    = vnet;
    msg_head.cmd    = mqtype;
    msg_head.bufsiz = len;
    currmack        = mmq_open((INT8S*)name, &attr, O_WRONLY);
    if (currmack != -1) {
        if (mmq_put(currmack, 3, msg_head, (INT8S*)buf, 0) < 0) {
            fprintf(stderr, "[vMsgr][Ifr]消息队列发送失败(%d %s)\n", errno, strerror(errno));
            ret = -1;
        } else {
            ret = 1;
        }
    } else {
        ret = -2;
    }
    mq_close(currmack);
    return ret;
}

INT8S setpara_tomsg(gdm_type mqtype, INT8U* buf, INT16U len) {
    return all_tomsg(mqtype, buf, len, COM_VMAIN_MQ);
}

/****************************************
 * 函数说明：发送交采消息回调函数
 ****************************************/
INT8S sendacs_tomsg(gdm_type mqtype, INT8U* buf, INT16U len) {
    return all_tomsg(mqtype, buf, len, ACS_REALDATA_MQ);
}

/****************************************
 * 函数说明：点抄发送s485-1消息回调函数
 ****************************************/
INT8S read4851_tomsg(gdm_type mqtype, INT8U* buf, INT16U len) {
    return all_tomsg(mqtype, buf, len, S485_1_REV_IFR_MQ);
}

/****************************************
 * 函数说明：点抄发送s485-2消息回调函数
 ****************************************/
INT8S read4852_tomsg(gdm_type mqtype, INT8U* buf, INT16U len) {
    return all_tomsg(mqtype, buf, len, S485_2_REV_IFR_MQ);
}

/****************************************
 * 函数说明：点抄发送统计calc消息回调函数
 ****************************************/
INT8S readcalc_tomsg(gdm_type mqtype, INT8U* buf, INT16U len) {
    return all_tomsg(mqtype, buf, len, CALC_REV_MQ);
}

/****************************************
 * 函数说明：点抄发送载波消息回调函数
 ****************************************/
INT8S readplc_tomsg(gdm_type mqtype, INT8U* buf, INT16U len) {
    return all_tomsg(mqtype, buf, len, PLC_REV_IFR_MQ);
}

/****************************************
 * 函数说明：控制发送控制消息回调函数
 ****************************************/
INT8S setctrl_tomsg(gdm_type mqtype, INT8U* buf, INT16U len) {
    return all_tomsg(mqtype, buf, len, CTRL_REV_MQ);
}

/****************************************
 * 函数说明：复位发送存储回调函数
 ****************************************/
INT8S reset_tomsg(gdm_type mqtype, INT8U* buf, INT16U len) {
    return all_tomsg(mqtype, buf, len, SAVE_NORMAL_MQ);
}
/****************************************
 * 函数说明：发送事件回调函数
 ****************************************/
INT8S vevent_tomsg(gdm_type mqtype, INT8U* buf, INT16U len) {
    return all_tomsg(mqtype, buf, len, SAVE_NORMAL_MQ);
}
