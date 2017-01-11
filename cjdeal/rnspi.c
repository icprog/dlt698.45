#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>
#include <linux/spi/spidev.h>

#include "rn8209.h"
#include "rnspi.h"
#include "vacs.h"
//以下头文件为校表用，如果把校表放在vd进程中，可不加这些头文件

#define AV_COUNT 10   //校正电表采样次数
#define REG_COUNT 100 //校表系数扩大倍数

static const char* rnspidevname = "/dev/spi0.0";
static ACCoe_SAVE attCoef;  //校表系数结构体
static _RealData realdata;  //交采实时数据
static sem_t* sem_check_fd; //校表信号量
INT32U K_vrms = 8193;     // RN8209校表系数 748600

/*
 * 功能：在/dev/shm目录下创建信号量描述文件，如果已经存在同名的文件，则先删除，然后在创建。
 *
 * 输入：
 * name：为命名信号量的名称。
 * flag：1 或者 0
 *
 * 返回：如果信号量创建成功，则返回信号量句柄
 */

sem_t* create_named_sem(const char* name, int flag) {
    sem_t* fd;
    if (name != NULL) {
        sem_unlink(name);
        fd = sem_open(name, O_CREAT, O_RDWR, flag);
        if (fd != SEM_FAILED)
            return fd;
    }
    return NULL;
}

/*
 * 功能：打开一个命名信号量
 *
 * 输入
 * name：命名信号量文件名
 *
 * 返回
 * 成功：返回信号量句柄
 * 失败：返回空
 */

sem_t* open_named_sem(const char* name) {
    sem_t* fd;
    if (name != NULL) {
        fd = sem_open(name, O_RDWR);
        if (fd != SEM_FAILED)
            return fd;
    }
    return NULL;
}

static void dumpstat_r(const char* name, int fd) {
    static uint8_t mode;
    static uint8_t bits   = 8;
    static uint32_t speed = 400000;
    int ret;

    mode = SPI_MODE_1;
    //	mode = SPI_MODE_2;

    ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
    if (ret == -1)
        fprintf(stderr, "can't set spi mode");

    ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
    if (ret == -1)
        fprintf(stderr, "can't get spi mode");

    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1)
        fprintf(stderr, "can't set bits per word");

    ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    if (ret == -1)
        fprintf(stderr, "can't get bits per word");

    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if (ret == -1)
        fprintf(stderr, "can't set max speed hz");

    ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
    if (ret == -1)
        fprintf(stderr, "can't get max speed hz");
}

INT32S spi_close_r(INT32S fd) {
    close(fd);
    return 1;
}

INT32S spi_init_r(INT32S fd, const char* spipath) {
    if (fd != -1) {
        spi_close_r(fd);
    }

    fd = open((char*)spipath, O_RDWR);
    if (fd < 0)
        fprintf(stderr, "can't open  device %s\n", spipath);
    // if((rn_spi_read(spifp_rn8209, SYS_Status) & 0x01) != 0)//IRQ的状态，复位过
//    {
//        dumpstat_r((char*)spipath, fd);
//        usleep(10000); //交采实时电压数据
//        sleep(1);
//    }

    return fd;
}

int spi_read_r(int fd, INT8U* cbuf, INT16U clen, INT8U* rbuf, int rlen) {
    unsigned char rx[512], i;
    struct spi_ioc_transfer xfer[2];
    unsigned char tx[512];

    memset(tx, 0x00, 512);
    memset(rx, 0x00, 512);
    memset(xfer, 0, sizeof xfer);

    for (i = 0; i < clen; i++) {
        tx[i] = cbuf[i];
    }

    xfer[0].tx_buf = (int)tx; //读取的地址
    // fprintf(stderr,"发送的数据 = %d\n",(int)tx);
    xfer[0].len = clen;
    //	fprintf(stderr,"发送的长度 = %d\n",(int)clen);
    xfer[1].rx_buf = (int)rx;
    xfer[1].len    = rlen;
    ioctl(fd, SPI_IOC_MESSAGE(2), xfer);
    for (i = 0; i < rlen; i++) {
        rbuf[i] = rx[i];
        // fprintf(stderr,"rbuf[%d] = %02x ",i,rbuf[i]);
    }

    return 1;
}

int spi_write_r(int fd, INT8U* buf, int len) {
    INT8U tx[32], rx[32];
    INT8U i;
    struct spi_ioc_transfer xfer[2];

    memset(tx, 0, 32);
    memset(rx, 0, 32);
    memset(xfer, 0, sizeof xfer);

    if (len > 8)
        return ERR_WRTBUF_OVERLEN;
    for (i         = 0; i < len; i++)
        tx[i]      = buf[i];
    xfer[0].tx_buf = (int)tx;
    xfer[0].len    = len;

    xfer[1].rx_buf = (int)rx;
    xfer[1].len    = 0;

    ioctl(fd, SPI_IOC_MESSAGE(2), xfer);

    return len;
}

INT32S rn_spi_read(int spifp, INT32U addr) {
    INT8U cmd[BUFFLENMAX_SPI];
    INT8U buf[BUFFLENMAX_SPI];
    INT32U i = 0, len_rn8209 = 0;
    INT32S rec;
    int cnt = 0;

    len_rn8209 = addr & 0x0fu;
    cmd[cnt++] = addr >> 4 & 0x7f;
    spi_read_r(spifp, cmd, cnt, buf, len_rn8209);

    rec = 0;
    for (i = 0; i < len_rn8209; i++) {
        rec = (rec << 8) | buf[i];
        // fprintf(stderr,"buf[%d] = %02x ",i,buf[i]);
    }

    return rec;
}

INT32S rn_spi_write(int spifp, INT32U addr, INT8U* buf) {
    INT32U i = 0, len_rn8209 = 0;
    int cnt = 0;
    INT32S num;
    INT8U cmd[BUFFLENMAX_SPI];

    len_rn8209 = addr & 0x0fu;
    if (addr == CMD_REG) {
        cmd[cnt++] = 0xEA; //写特殊命令
    } else
        cmd[cnt++] = addr >> 4 | 0x80;
    for (i = 0; i < len_rn8209; i++) { //写更新校表数据
        cmd[cnt++] = buf[i];
    }
    num = spi_write_r(spifp, cmd, cnt);

    return num;
}

//校验寄存器读出的数值是否正确
INT8S check_regvalue_rn8209(INT32S regvalue) {
    INT32S RRec[128]; // RN8209计量参数寄存器数据

    RRec[SYS_RData >> 4] = rn_spi_read(spifp_rn8209, SYS_RData);
    //	dbg_prt( "校验寄存器值为： %d \n", RRec[SYS_RData>>4]);
    if (regvalue == RRec[SYS_RData >> 4]) {
        return 0;
    } else
        return -1;
}
//换算RN8209计量寄存器值
//输入参数：reg：计量寄存器值
//返回值    ：实时采样值
INT32S trans_regist_rn8209(INT32S reg) {
    INT32S tread = 0;
    if (K_vrms){
        tread = ((FP64)reg * U_COEF) / K_vrms;
    }
    return tread;
}
//获取芯片ID，确定芯片类型
INT8S check_id_rn8209(void) {
    spifp_rn8209 = spi_init_r(spifp_rn8209, rnspidevname);
    if (rn_spi_read(spifp_rn8209, DeviceID) == 0x820900) {
        return 1;
    } else {
        return 0;
    }
}
//初始化RN8209的运行环境
void init_run_env_rn8209(INT32S pid) {

    fprintf(stderr, "读取到的校表系数 = %d\n", K_vrms);
    //初始化spi设备
    spifp_rn8209 = spi_init_r(spifp_rn8209, rnspidevname);

    int sem_id;
	sem_check_fd = create_named_sem(SEMNAME_SPI0_0,1);
	sem_getvalue(sem_check_fd, &sem_id);
	fprintf(stderr, "process %d The sem is %d\n", pid, sem_id);
}

void DealACS(void) {
    INT32S RRec[128]; // RN8209计量参数寄存器数据
    INT32S val = 0;
    static int time_old = 0;
    int time_now = time(NULL);
    if(time_old == time_now){
    	return;
    }
    time_old = time_now;
    sem_wait(sem_check_fd);

	RRec[U_RMS >> 4] = rn_spi_read(spifp_rn8209, U_RMS); //电压通道有效值
	if ((check_regvalue_rn8209(RRec[U_RMS >> 4]) == 0)) {
		val = RRec[U_RMS >> 4];
	}
    sem_post(sem_check_fd);
    realdata.Ua = (realdata.Ua * 4)/5 + (trans_regist_rn8209(val) * 1)/5; //转换，获取电压当前值
    fprintf(stderr, "当前电压值为： %d\n", realdata.Ua);
}

void DealState(void){
	static int step = 0;
	static int time_x = 0;
	static int state_old = 1;

	int fd = 0, state = 0;
    if ((fd = open("/dev/gpiYX1", O_RDWR | O_NDELAY)) >= 0) {
        read(fd, &state, sizeof(int));
        close(fd);
    }

    switch(step){
    case 0:
    	time_x = 0;
    	if(state_old != state){
    		step = 1;
    	}
    	break;
    case 1:
    	if(state_old != state){
    		time_x++;
    		if(time_x > 9){
    			step = 2;
    		}
    	}
    	else
    	{
    		step = 0;
    	}
    	break;
    case 2:
    	printf("发生遥信变位%d\n", state);
    	state_old = state;
    	step = 0;
    	break;
    }
}
