/*
 * sms.c
 *
 *  Created on: Sep 1, 2017
 *      Author: z
 */

#include <stdarg.h>

#include "db.h"
#include "sms.h"
#include "cjcomm.h"
#include "atBase.h"
#include "helper.h"


// 可打印字符串转换为字节数据
// 如："C8329BFD0E01" --> {0xC8, 0x32, 0x9B, 0xFD, 0x0E, 0x01}
// pSrc: 源字符串指针
// pDst: 目标数据指针
// nSrcLength: 源字符串长度
// 返回: 目标数据长度
int gsmString2Bytes(const char *pSrc, unsigned char *pDst, int nSrcLength) {
    int i;
    //	fprintf(stderr,"gsmString2Bytes:nSrcLength=%d,pSrc=%s\n",nSrcLength,pSrc);
    for (i = 0; i < nSrcLength; i += 2) {
        // 输出高4位
        if (*pSrc >= '0' && *pSrc <= '9') {
            *pDst = (*pSrc - '0') << 4;
        } else {
            *pDst = (*pSrc - 'A' + 10) << 4;
        }
        //       fprintf(stderr,"pSrc=%c,pDsth=%02x\n",*pSrc,*pDst);
        pSrc++;

        // 输出低4位
        if (*pSrc >= '0' && *pSrc <= '9') {
            *pDst |= *pSrc - '0';
        } else {
            *pDst |= *pSrc - 'A' + 10;
        }
        //       fprintf(stderr,"pSrc=%d,pDstl=%02x\n",*pSrc,*pDst);
        pSrc++;
        pDst++;
    }
    // 返回目标数据长度
    return nSrcLength / 2;
}

// 两两颠倒的字符串转换为正常顺序的字符串
// 如："683158812764F8" --> "8613851872468"a
// pSrc: 源字符串指针
// pDst: 目标字符串指针
// nSrcLength: 源字符串长度
// 返回: 目标字符串长度
int gsmSerializeNumbers(const char *pSrc, char *pDst, int nSrcLength) {
    int nDstLength; // 目标字符串长度
    //    char ch;          // 用于保存一个字符
    int i;

    // 复制串长度
    nDstLength = nSrcLength;

    //    fprintf(stderr,"pSrc=%s\n",pSrc);
    // 两两颠倒
    for (i = 0; i < nSrcLength; i++) {
        if (i % 2 == 0) {
            pDst[i + 1] = pSrc[i];
        } else {
            pDst[i - 1] = pSrc[i];
        }
    }
    //    fprintf(stderr,"pDst=%s\n",pDst);
    // 最后的字符是'F'吗？
    if (pDst[i - 1] == 'F') {
        //	    fprintf(stderr,"pDst[%d]=%c\n",i-1,pDst[i-1]);
        pDst[i - 1] = '\0'; // 输出字符串加个结束符
        nDstLength--;       // 目标字符串长度减1
    }
    // 返回目标字符串长度
    return nDstLength;
}

// 7-bit解码
// pSrc: 源编码串指针
// pDst: 目标字符串指针
// nSrcLength: 源编码串长度
// 返回: 目标字符串长度
int gsmDecode7bit(const unsigned char *pSrc, char *pDst, int nSrcLength) {
    int nSrc;            // 源字符串的计数值
    int nDst;            // 目标解码串的计数值
    int nByte;           // 当前正在处理的组内字节的序号，范围是0-6
    unsigned char nLeft; // 上一字节残余的数据

    // 计数值初始化
    nSrc = 0;
    nDst = 0;

    // 组内字节序号和残余数据初始化
    nByte = 0;
    nLeft = 0;

    // 将源数据每7个字节分为一组，解压缩成8个字节
    // 循环该处理过程，直至源数据被处理完
    // 如果分组不到7字节，也能正确处理
    while (nSrc < nSrcLength) {
        //    	fprintf(stderr,"pSrc=%02x\n",*pSrc);
        // 将源字节右边部分与残余数据相加，去掉最高位，得到一个目标解码字节
        *pDst = ((*pSrc << nByte) | nLeft) & 0x7f;
        //        fprintf(stderr,"pDst=%02x\n",*pDst);
        // 将该字节剩下的左边部分，作为残余数据保存起来
        nLeft = *pSrc >> (7 - nByte);
        // 修改目标串的指针和计数值
        pDst++;
        nDst++;
        // 修改字节计数值
        nByte++;
        // 到了一组的最后一个字节
        if (nByte == 7) {
            // 额外得到一个目标解码字节
            *pDst = nLeft;
            // 修改目标串的指针和计数值
            pDst++;
            nDst++;
            // 组内字节序号和残余数据初始化
            nByte = 0;
            nLeft = 0;
        }
        // 修改源串的指针和计数值
        pSrc++;
        nSrc++;
    }
    *pDst = '\0';
    // 返回目标串长度
    return nDst;
}

// PDU解码，用于接收、阅读短消息
// pSrc: 源PDU串指针
// pDst: 目标PDU参数指针
// 返回: 用户信息串长度
int gsmDecodePdu(const char *pSrc, SM_PARAM *pDst) {
    int nDstLength = 0;     // 目标PDU串长度
    unsigned char tmp;      // 内部用的临时字节变量
    unsigned char buf[512]; // 内部用的缓冲区

    // SMSC地址信息段
    gsmString2Bytes(pSrc, &tmp, 2);
    asyslog(LOG_DEBUG, "SMSC地址信息段长度=%d", tmp);

    tmp = (tmp - 1) * 2;
    asyslog(LOG_DEBUG, "SMSC号码串长度=%d", tmp);
    pSrc += 4;                                 // 指针后移
    gsmSerializeNumbers(pSrc, pDst->SCA, tmp); // 转换SMSC号码到目标PDU串
    pSrc += tmp;                               // 指针后移
    // TPDU段基本参数、回复地址等
    gsmString2Bytes(pSrc, &tmp, 2); // 取基本参数
    pSrc += 2;                      // 指针后移
    // 包含回复地址，取回复地址信息
    gsmString2Bytes(pSrc, &tmp, 2); // 取长度
    if (tmp & 1)
        tmp += 1;                              // 调整奇偶性
    pSrc += 4;                                 // 指针后移
    gsmSerializeNumbers(pSrc, pDst->TPA, tmp); // 取TP-RA号码
    //        fprintf(stderr,"TPA=%s\n",pDst->TPA);
    pSrc += tmp;                               // 指针后移

    // TPDU段协议标识、编码方式、用户信息等
    gsmString2Bytes(pSrc, (unsigned char *) &pDst->TP_PID, 2); // 取协议标识(TP-PID)
    asyslog(LOG_DEBUG, "TP_PID=%d", pDst->TP_PID);
    pSrc += 2;                                               // 指针后移
    gsmString2Bytes(pSrc, (unsigned char *) &pDst->TP_DCS, 2); // 取编码方式(TP-DCS)
    pDst->TP_DCS = pDst->TP_DCS & 0x0c;
    asyslog(LOG_DEBUG, "TP_DCS=%x,接收号码：%s", pDst->TP_DCS, pDst->TPA);
    pSrc += 2;                                    // 指针后移
    gsmSerializeNumbers(pSrc, pDst->TP_SCTS, 14); // 服务时间戳字符串(TP_SCTS)
    pSrc += 14;                                   // 指针后移
    gsmString2Bytes(pSrc, &tmp, 2);               // 用户信息长度(TP-UDL)
    pSrc += 2;                                    // 指针后移

    if (pDst->TP_DCS == GSM_7BIT) {
        // 7-bit解码
        nDstLength = gsmString2Bytes(pSrc, buf, tmp & 7 ? (int) tmp * 7 / 4 + 2 : (int) tmp * 7 / 4); // 格式转换
        if (nDstLength > 161) {
            asyslog(LOG_DEBUG, "7-Bit:错误短信长度>161,返回");
            return 0;
        }
        gsmDecode7bit(buf, pDst->TP_UD, nDstLength); // 转换到TP-DU
        asyslog(LOG_DEBUG, "7-Bit解码数据长度=%d", nDstLength);
        asyslog(LOG_DEBUG, "解码后的数据%s", pDst->TP_UD);
        nDstLength = tmp;
    } else if (pDst->TP_DCS == GSM_UCS2) {
        fprintf(stderr, "UCS2编码格式,不处理");
    } else {
        // 8-bit解码
        nDstLength = gsmString2Bytes(pSrc, buf, tmp * 2); // 格式转换
        asyslog(LOG_DEBUG, "nDstLength=%d", nDstLength);
        if (nDstLength > 161) {
            asyslog(LOG_DEBUG, "8-Bit:错误短信长度>161,返回");
            return 0;
        }
        memcpy(buf, pDst->TP_UD, nDstLength); // 转换到TP-DU
        asyslog(LOG_DEBUG, "8-Bit解码数据长度=%d", nDstLength);
    }

    // 返回目标字符串长度
    return nDstLength;
}

void checkSms(ATOBJ *ao) {
	int fd = NULL;

    fd = OpenMuxCom(1, 115200, (unsigned char *) "none", 1, 8); // 0

	if(fd < 0){
		return;
	}

    for (int timeout = 0; timeout < 2; timeout++) {
        char Mrecvbuf[512];

        write(fd, "\rAT+CMGL=0\r", 11);
        delay(1000);
        memset(Mrecvbuf, 0, 512);
        RecieveFromComm(Mrecvbuf, 512, fd);

        char cimi[256];
        memset(cimi, 0x00, sizeof(cimi));
        char *position = strstr(Mrecvbuf, "0891683108");
        if (position != Mrecvbuf && position != NULL) {
            if (sscanf(position, "%[0-9|A-F]", cimi) == 1) {
                asyslog(LOG_INFO, "CMGR = %s\n", cimi);
                SM_PARAM smpara;
                int len = gsmDecodePdu(position, &smpara);
                fprintf(stderr, "数据：R(%d)---\n", len);
                for (int i = 0; i < len; i++) {
                    fprintf(stderr, "%02x ", smpara.TP_UD[i]);
                	if(smpara.TP_UD[i] == 0x00){
						smpara.TP_UD[i] = 0x40;
					}
                }
                int pos1 = 0, pos2 = 0;
        		char cmd[128];
                if(strncmp(smpara.TP_UD, "cssdl", strlen("cssdl")) == 0){
                	asyslog(LOG_INFO, "有短信命令接入%c\n", smpara.TP_UD[6]);
                	switch(smpara.TP_UD[6]){
                	case '0':
                		system("reboot");
                		break;
                	case '1':
                		for (int i = len - 1; i >= 0; i--) {
                			if(smpara.TP_UD[i] == ':'){
                				smpara.TP_UD[i] = 0x00;
                				if(pos1 == 0)
                				{
                					pos1 = i + 1;
                					break;
                				}
                			}
                		}
                		memset(cmd, 0x00, sizeof(cmd));
                		sprintf(cmd, "cj ip %s %s", &smpara.TP_UD[8], &smpara.TP_UD[8]);
                		fprintf(stderr, "%s\n", cmd);
                		system(cmd);

                		memset(cmd, 0x00, sizeof(cmd));
                		sprintf(cmd, "cj apn %s", &smpara.TP_UD[pos1]);
                		fprintf(stderr, "%s\n", cmd);
                		system(cmd);
                	    break;
                	case '2':
                		for (int i = len - 1; i >= 0; i--) {
							if(smpara.TP_UD[i] == ':'){
								smpara.TP_UD[i] = 0x00;
								if(pos1 == 0)
								{
									pos1 = i + 1;
									continue;
								}
								if(pos2 == 0)
								{
									pos2 = i + 1;
									break;
								}
							}
						}
                		fprintf(stderr, "++++++++++++++++pos1%d pos2%d\n", pos1, pos2);
                		if(pos1 != 0 && pos2 !=0){
							memset(cmd, 0x00, sizeof(cmd));
							sprintf(cmd, "cj ip %s %s", &smpara.TP_UD[8], &smpara.TP_UD[8]);
							fprintf(stderr, "%s\n", cmd);
							system(cmd);

							memset(cmd, 0x00, sizeof(cmd));
							sprintf(cmd, "cj usr-pwd %s %s cmnet", &smpara.TP_UD[pos2], &smpara.TP_UD[pos1]);
							fprintf(stderr, "%s\n", cmd);
							system(cmd);
                		}
                	    break;
                	}
                }
                fprintf(stderr, "\n---\n");
                break;
            }
        } else {
            asyslog(LOG_NOTICE, "无短信内容，等待下次检查...");
            break;
        }
    }
    if(fd!=NULL) close(fd);//不关闭会不断出现/dev/pts/1的设备
}
