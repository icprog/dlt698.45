#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include "ParaDef.h"
#include "AccessFun.h"
#include "StdDataType.h"
#include "dlt698def.h"
#include "Objectdef.h"

/*
命令结果∷=enum
{
  文件传输进度 0~99%				（0~99），
  传输或执行操作成功				（100），
  正在建立连接（扩展传输）			（101），
  正在远程登录（扩展传输）			（102），
  正在执行文件					（103），
  文件或目录不存在					（104），
  操作不允许（创建/删除/读写/执行）	（105），
  文件传输中断					（106），
  文件校验失败					（107），
  文件转发失败					（108），
  文件代收失败					（109），
  建立连接失败（扩展传输）			（110），
  远程登录失败（扩展传输）			（111），
  存储空间不足					（112），
  复位后默认值					（255）
}
*/

//每次只能同时升级一个文件
static unsigned char checksum;
static unsigned short blocksize;
static unsigned char file_path[256];

int createFile(const char* path, int length, unsigned char crc, unsigned short bs) {
    FILE* fp = NULL;
    //文件不能太长
    if (length > 5 * 1024 * 1024) {
        return 112;
    }

    checksum  = crc;
    blocksize = bs;

    //文件夹不存在,创建文件夹
    if (access("/nand/UpFiles", F_OK) != 0) {
        system("mkdir /nand/UpFiles");
    }
    memcpy(file_path, path, sizeof(file_path));

    //打开文件，并写入空值对文件进行填充
    fp = fopen((const char*)path, "w+");
    if (fp != NULL) {
        INT8U fills[1024];
        fseek(fp, 0L, SEEK_SET);
        int len = length;
        while (len > 0) {
            int num = fwrite(fills, (len > 1024) ? 1024 : len, 1, fp);
            //			printf("create File size:%d-%d-%d\n", num, errno, length);
            len -= (len > 1024) ? 1024 : len;
        }
        fclose(fp);

    } else {
        return 105;
    }

    return 0;
}

int CheckFileSum(void) {
    FILE* fp = NULL;

    fp = fopen((const char*)file_path, "r+");
    if (fp == NULL) {
        return 107;
    }

    //获取文件长度
    struct stat mstats;
    stat(file_path, &mstats);
    fprintf(stderr, "文件校验，长度(%d)", mstats.st_size);

    char buf[1024];
    int file_length        = mstats.st_size;
    unsigned char local_cs = 0x00;

    while (file_length > 0) {
        int current_length = (file_length > 1024) ? 1024 : file_length;
        fread(buf, current_length, 1, fp);
        for (int i = 0; i < current_length; i++) {
            local_cs += buf[i];
            local_cs %= 256;
        }
        file_length -= current_length;
    }
    fclose(fp);

    fprintf(stderr, "文件校验结果，应为(%02x)，实为(%02x)", checksum, local_cs);

    return 100;
}

int appendFile(int shift, int length, unsigned char* buf) {
    FILE* fp = NULL;

    //文件不存在
    if (access(file_path, F_OK) != 0) {
        return 104;
    }

    fp = fopen((const char*)file_path, "r+");
    if (fp == NULL) {
        return 105;
    }

    fseek(fp, shift * blocksize, SEEK_SET);
    fwrite(buf, length, 1, fp);
    fclose(fp);

    //获取文件长度
    struct stat mstats;
    stat(file_path, &mstats);
    int res = (int)((shift * blocksize + length) * 100.0) / mstats.st_size;

    if (shift * blocksize + length != mstats.st_size) {
        return res;
    }

    return CheckFileSum();
    // return 107;
}

int GetFileState(RESULT_NORMAL* response) {
    FILE* fp = fopen((const char*)file_path, "r+");
    if (fp == NULL) {
        fprintf(stderr, "尝试获取文件状态，但文件不存在...\n");
        return 107;
    }

    //获取文件长度
    struct stat mstats;
    stat(file_path, &mstats);
    fprintf(stderr, "获取文件状态，长度(%d)", mstats.st_size);

    fclose(fp);

    int blocks = mstats.st_size / blocksize;
    if (mstats.st_size % blocksize > 0) {
        blocks += 1;
    }
    int counts = blocks / 8;
    int last   = blocks % 8;

    response->data[0] = 0x04;
    response->data[1] = 0x82;
    response->data[2] = ((blocks)&0xff00) >> 8;
    response->data[3] = ((blocks)&0x00ff);

    for (int i = 0; i < counts; i++) {
        response->data[i + 4] = 0xff;
    }

    response->data[counts + 4] = 0x00;

    for (int i = 0; i < last; i++) {
        response->data[counts + 4 + i] |= (0x01 << (7 - i));
        printf("%02x\n", (0x01 << (7 - i)));
    }
    response->datalen += counts + 1 + 4;

    char order[256];
    memset(order, 0x00, sizeof(order));

    sprintf(order, "mv %s /nand/UpFiles/update.sh", file_path);
    system(order);


    system("echo \"reboot\" >> /nand/UpFiles/reboot");

    return 0;
}