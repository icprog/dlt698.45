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
static unsigned char checksumtmp;
static unsigned char file_path[256];


int createFile(const char * path, int length, unsigned char crc, unsigned short bs){
	FILE 	*fp=NULL;
	//文件不能太长
	if(length > 5 * 1024 * 1024){
		return 112;
	}
	checksum = crc;
	checksumtmp = 0x00;
	blocksize = bs;

	//文件夹不存在,创建文件夹
	if(access("/nand/UpFiles", F_OK)!=0){
		system("mkdir /nand/UpFiles");
	}
	memcpy(file_path, path, sizeof(file_path));

	//打开文件，并写入空值对文件进行填充
	fp = fopen((const char*)path, "w+");
	if (fp != NULL) {
		INT8U fills[1024];
		fseek(fp, 0L, SEEK_SET);
		int len = length;
		while(len >0){
			int num = fwrite(fills,(len>1024)?1024:len,1,fp);
//			printf("create File size:%d-%d-%d\n", num, errno, length);
			len -= (len>1024)?1024:len;
		}
		fclose(fp);

	}
	else{
		return 105;
	}

	return 0;
}


int appendFile(int shift, int length, unsigned char *buf){
	FILE 	*fp=NULL;

	//文件不存在
	if(access(file_path, F_OK)!=0){
		return 104;
	}

	//打开文件，并写入空值对文件进行填充
	fp = fopen((const char*)file_path, "r+");
	if (fp != NULL) {
		fseek(fp, shift*blocksize, SEEK_SET);
		fwrite(buf,length,1,fp);
		fclose(fp);
		for(int j = 0; j < length; j++){
			checksumtmp += buf[j];
		}
		struct stat mstats;
		stat(file_path, &mstats);
		int res = (int)((shift*blocksize + length)*100.0)/mstats.st_size;
		if(res == 100){
			printf("checksumtmp = %d-%d\n", checksumtmp, checksum);
			if(checksumtmp == checksum){
				return 100;
			}
			else{
				return 107;
			}
		}
		else
		{
			return res;
		}
	}
	else{
		return 105;
	}

	return 0;
}

