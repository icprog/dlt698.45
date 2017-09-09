/*
 * iconv.c
 *
 *  Created on: 2014-2-27
 *      Author: yd
 */

/*
iconv函数族有三个函数,原型如下:
(1) iconv_t iconv_open(const char *tocode, const char *fromcode);
此函数说明将要进行哪两种编码的转换,tocode是目标编码,fromcode是原编码,该函数返回一个转换句柄,供以下两个函数使用。
(2) size_t iconv(iconv_t cd,char **inbuf,size_t *inbytesleft,char **outbuf,size_t *outbytesleft);
此函数从inbuf中读取字符,转换后输出到outbuf中,inbytesleft用以记录还未转换的字符数,outbytesleft用以记录输出缓冲的剩余空间。 (3) int iconv_close(iconv_t cd);
此函数用于关闭转换句柄,释放资源。
例子1: 用C语言实现的转换示例程序
*/
/* f.c : 代码转换示例C程序 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/syslog.h>
#include <sys/stat.h>
#include "iconv.h"

#define OUTLEN 255
#define LIBICONV_PLUG
//代码转换:从一种编码转为另一种编码
int code_convert(char *from_charset,char *to_charset,char *inbuf,int inlen,char *outbuf,int outlen)
{
	iconv_t cd;
	char **pin = &inbuf;
	char **pout = &outbuf;

	cd = iconv_open(to_charset,from_charset);
	if (cd==0) return -1;
	memset(outbuf,0,outlen);
	if (iconv(cd,pin,(size_t*)&inlen,pout,(size_t*)&outlen)==-1)
		return -1;
	iconv_close(cd);
	return 0;
}
//UNICODE码转为GB2312码
int u2g(char *inbuf,int inlen,char *outbuf,int outlen)
{
	return code_convert("utf-8","gb2312",inbuf,inlen,outbuf,outlen);
}
//GB2312码转为UNICODE码
int g2u(char *inbuf,size_t inlen,char *outbuf,size_t outlen)
{
	return code_convert("gb2312","utf-8",inbuf,inlen,outbuf,outlen);
}
//
//main()
//{
//	char *in_utf8 = "姝ｅ?ㄥ??瑁?";
//	char *in_gb2312 = "正在安装";
//	char out[OUTLEN];
//
//	//unicode码转为gb2312码
//	rc = u2g(in_utf8,strlen(in_utf8),out,OUTLEN);
//	printf("unicode-->gb2312 out=%sn",out);
//	//gb2312码转为unicode码
//	rc = g2u(in_gb2312,strlen(in_gb2312),out,OUTLEN);
//	printf("gb2312-->unicode out=%sn",out);
//}
