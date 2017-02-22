#include "vsms.h"
#include <string.h>
#include <fcntl.h>

extern int smsPort = -1; //	短信口

static long long RetCode;
static unsigned char StartCMGL = 0; //	发送AT+CMGL=ALL命令标志
static INT8U rxbuf[BUF_LEN];        //	模块短信接收缓冲区
static int rxpt;                    //	接收短信缓冲区指针
static int handlept;                //	接收短信处理指针
static char msgno[10];              //  手机短信序号
static char rec_sms_telno[TEL_LEN]; //  接收到的手机短信号码
static INT8U SmsReceBuf[BUF_LEN];   //  短信接收数据临时变量
static int SmsRHead, SmsRTail;      //	接收报文头指针，尾指针

static struct RXMSG rxmsg[RXMSG_NUM];
static SM_PARAM smpara;

// 8Bit编码
// pSrc: 源字符串指针
// pDst: 目标编码串指针
// nSrcLength: 源字符串长度
// 返回: 目标编码串长度
int gsmEncode8bit(const char* pSrc, unsigned char* pDst, int nSrcLength) {
    memcpy(pDst, pSrc, nSrcLength);
    // 输出字符串加个结束符
    //   *pDst = '\0';
    return nSrcLength;
}

// 可打印字符串转换为字节数据
// 如："C8329BFD0E01" --> {0xC8, 0x32, 0x9B, 0xFD, 0x0E, 0x01}
// pSrc: 源字符串指针
// pDst: 目标数据指针
// nSrcLength: 源字符串长度
// 返回: 目标数据长度
int gsmString2Bytes(const char* pSrc, unsigned char* pDst, int nSrcLength) {
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

// 字节数据转换为可打印字符串
// 如：{0xC8, 0x32, 0x9B, 0xFD, 0x0E, 0x01} --> "C8329BFD0E01"
// pSrc: 源数据指针
// pDst: 目标字符串指针
// nSrcLength: 源数据长度
// 返回: 目标字符串长度
int gsmBytes2String(const unsigned char* pSrc, char* pDst, int nSrcLength) {
    const char tab[] = "0123456789ABCDEF"; // 0x0-0xf的字符查找表
    int i;
    for (i = 0; i < nSrcLength; i++) //
    {
        // 输出低4位
        *pDst++ = tab[*pSrc >> 4];
        // 输出高4位
        *pDst++ = tab[*pSrc & 0x0f];
        pSrc++;
    }
    // 输出字符串加个结束符
    *pDst = '\0';
    // 返回目标字符串长度
    return nSrcLength * 2;
}

// 正常顺序的字符串转换为两两颠倒的字符串，若长度为奇数，补'F'凑成偶数
// 如："8613800535500" --> "683108505305F0"
// pSrc: 源字符串指针
// pDst: 目标字符串指针
// nSrcLength: 源字符串长度
// 返回: 目标字符串长度
int gsmInvertNumbers(const char* pSrc, char* pDst, int nSrcLength) {
    int nDstLength; // 目标字符串长度
    int i;
    char tmpSrc[100];

    // 复制串长度
    nDstLength = nSrcLength;
    memset(tmpSrc, 0, sizeof(tmpSrc));
    strcpy(tmpSrc, pSrc);
    // 源串长度是奇数吗？
    if (nSrcLength & 1) {
        tmpSrc[nSrcLength] = 'F'; // 补'F'
        nDstLength++;
    }
    //    fprintf(stderr,"Src=%s\n",pSrc);
    // 两两颠倒
    for (i = 0; i < nDstLength; i++) //
    {
        if (i % 2 == 0) {
            pDst[i + 1] = tmpSrc[i];
        } else {
            pDst[i - 1] = tmpSrc[i];
        }
    }
    // 输出字符串加个结束符
    pDst[i] = '\0';
    // 返回目标字符串长度
    return nDstLength;
}

int gsmEncodePdu(const SM_PARAM* pSrc, char* pDst) {
    int nLength;            // 内部用的串长度
    int nDstLength = 0;     // 目标PDU串长度
    unsigned char buf[256]; // 内部用的缓冲区

    // TPDU段基本参数、目标地址等
    nLength = strlen(pSrc->TPA); // TP-DA地址字符串的长度
    fprintf(stderr, "nLength=%d\n,TPA=%s\n", nLength, pSrc->TPA);
    buf[0] = 0;                                               //短信中心号码及前面字节去掉，用0x00代替。
    buf[1] = 0x11;                                            // 是发送短信(TP-MTI=01)，TP-VP用相对格式(TP-VPF=10)
    buf[2] = 0;                                               // TP-MR=0
    buf[3] = (char)nLength;                                   // 目标地址数字个数(TP-DA地址字符串真实长度)
    buf[4] = 0;                                               // TODO:0x91;国际格式号码。浙江主站短信号码是9位，955983015，接收短信回来这个字节使用的“0xA1”(国内格式)//浙江报文特殊发送
    nDstLength += gsmBytes2String(buf, &pDst[nDstLength], 5); // 转换4个字节到目标PDU串
    nDstLength += gsmInvertNumbers(pSrc->TPA, &pDst[nDstLength], nLength); // 转换TP-DA到目标PDU串
    fprintf(stderr, "TPA:nDstLength=%d,pDst=%s\n", nDstLength, &pDst[0]);
    // TPDU段协议标识、编码方式、用户信息等
    nLength = strlen(pSrc->TP_UD); // 用户信息字符串的长度
    buf[0]  = pSrc->TP_PID;        // 协议标识(TP-PID)
    buf[1]  = pSrc->TP_DCS;        // 用户信息编码方式(TP-DCS)//0x15配置成和浙江收到短信同样类型
    buf[2]  = 0;                   // 有效期(TP-VP)为5分钟
    if (pSrc->TP_DCS == GSM_8BIT) {
        // 8-bit编码方式
        fprintf(stderr, "Tp_UD=%s,nLength=%d\n", pSrc->TP_UD, nLength);
        //   	buf[3] = gsmEncode8bit(pSrc->TP_UD, &buf[4], nLength);    // 转换TP-DA到目标PDU串
        buf[3] = gsmString2Bytes(pSrc->TP_UD, &buf[4], nLength); // 格式转换
        fprintf(stderr, "nLength=%d,buf[3]=%d\n", nLength, buf[3]);
    } else {
        fprintf(stderr, "PDU编码不是8-bit方式\n");
    }
    nLength = buf[3] + 4;                                           // nLength等于该段数据长度
    nDstLength += gsmBytes2String(buf, &pDst[nDstLength], nLength); // 转换该段数据到目标PDU串
    fprintf(stderr, "PDU:nDstLength=%d,pDst=%s\n", nDstLength, &pDst[0]);
    // 返回目标字符串长度
    return nDstLength;
}

// 两两颠倒的字符串转换为正常顺序的字符串
// 如："683158812764F8" --> "8613851872468"a
// pSrc: 源字符串指针
// pDst: 目标字符串指针
// nSrcLength: 源字符串长度
// 返回: 目标字符串长度
int gsmSerializeNumbers(const char* pSrc, char* pDst, int nSrcLength) {
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
int gsmDecode7bit(const unsigned char* pSrc, char* pDst, int nSrcLength) {
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
int gsmDecodePdu(const char* pSrc, SM_PARAM* pDst) {
    int nDstLength = 0;     // 目标PDU串长度
    unsigned char tmp;      // 内部用的临时字节变量
    unsigned char buf[512]; // 内部用的缓冲区

    // SMSC地址信息段
    gsmString2Bytes(pSrc, &tmp, 2); // 取长度
    fprintf(stderr, "tmp=%d\n", tmp);

    tmp = (tmp - 1) * 2; // SMSC号码串长度
    fprintf(stderr, "tmp1=%d\n", tmp);
    pSrc += 4;                                 // 指针后移
    gsmSerializeNumbers(pSrc, pDst->SCA, tmp); // 转换SMSC号码到目标PDU串
    fprintf(stderr, "SCA=%s,tmp=%d\n", pDst->SCA, tmp);
    pSrc += tmp; // 指针后移
    fprintf(stderr, "pSrc=%s\n", pSrc);
    // TPDU段基本参数、回复地址等
    gsmString2Bytes(pSrc, &tmp, 2); // 取基本参数
    fprintf(stderr, "pSrc=%s,tmp=%d\n", pSrc, tmp);
    pSrc += 2; // 指针后移
               //    if(tmp & 0x80)
    {
        // 包含回复地址，取回复地址信息
        gsmString2Bytes(pSrc, &tmp, 2); // 取长度
        if (tmp & 1)
            tmp += 1;                              // 调整奇偶性
        pSrc += 4;                                 // 指针后移
        gsmSerializeNumbers(pSrc, pDst->TPA, tmp); // 取TP-RA号码
                                                   //        fprintf(stderr,"TPA=%s\n",pDst->TPA);
        pSrc += tmp;                               // 指针后移
    }

    // TPDU段协议标识、编码方式、用户信息等
    gsmString2Bytes(pSrc, (unsigned char*)&pDst->TP_PID, 2); // 取协议标识(TP-PID)
    fprintf(stderr, "TP_PID=%d\n", pDst->TP_PID);
    pSrc += 2;                                               // 指针后移
    gsmString2Bytes(pSrc, (unsigned char*)&pDst->TP_DCS, 2); // 取编码方式(TP-DCS)
    pDst->TP_DCS = pDst->TP_DCS & 0x0c;
    fprintf(stderr, "TP_DCS=%x,接收号码：%s\n", pDst->TP_DCS, pDst->TPA);
    pSrc += 2;                                    // 指针后移
    gsmSerializeNumbers(pSrc, pDst->TP_SCTS, 14); // 服务时间戳字符串(TP_SCTS)
    pSrc += 14;                                   // 指针后移
    fprintf(stderr, "pSrc=%s\n", pSrc);
    gsmString2Bytes(pSrc, &tmp, 2); // 用户信息长度(TP-UDL)
    fprintf(stderr, "tmp_nDstLength=%d\n", tmp);
    pSrc += 2; // 指针后移

    if (pDst->TP_DCS == GSM_7BIT) {
        // 7-bit解码
        //   	fprintf(stderr,"Data:pSrc=%s\n",pSrc);
        nDstLength = gsmString2Bytes(pSrc, buf, tmp & 7 ? (int)tmp * 7 / 4 + 2 : (int)tmp * 7 / 4); // 格式转换
        //  	 nDstLength = gsmString2Bytes(pSrc, buf,4); // 格式转换
        //        fprintf(stderr,"nDstLength=%d,buf=%02x_%02x\n",nDstLength,buf[0],buf[1]);
        if (nDstLength > 161) {
            fprintf(stderr, "7-Bit:错误短信长度>161,返回");
            return 0;
        }
        gsmDecode7bit(buf, pDst->TP_UD, nDstLength); // 转换到TP-DU
        fprintf(stderr, "7-Bit:nDstLength=%d\n", nDstLength);
        fprintf(stderr, "TP_UD=%s\n", pDst->TP_UD);
        //        int i;
        //        for(i=0;i<nDstLength;i++)
        //        	fprintf(stderr,"%d\n",pDst->TP_UD[i]);
        nDstLength = tmp;
    } else if (pDst->TP_DCS == GSM_UCS2) {
        fprintf(stderr, "UCS2编码格式,不处理\n");
        // UCS2解码
        //        nDstLength = gsmDecodeUcs2(buf, pDst->TP_UD, nDstLength);    // 转换到TP-DU
    } else {
        // 8-bit解码
        fprintf(stderr, "数据pSrc=%s\n", pSrc);
        nDstLength = gsmString2Bytes(pSrc, buf, tmp * 2); // 格式转换
        fprintf(stderr, "nDstLength=%d\n", nDstLength);
        int i;
        for (i = 0; i < nDstLength; i++) {
            fprintf(stderr, "%02x ", buf[i]);
        }
        if (nDstLength > 161) {
            fprintf(stderr, "8-Bit:错误短信长度>161,返回");
            return 0;
        }
        nDstLength = memcpy(buf, pDst->TP_UD, nDstLength); // 转换到TP-DU

        fprintf(stderr, "8-Bit:nDstLength=%d\n", nDstLength);
        for (i = 0; i < nDstLength; i++) {
            fprintf(stderr, "%02x ", pDst->TP_UD[i]);
        }
    }

    // 返回目标字符串长度
    return nDstLength;
}

/*************************************************/
/*    m37i模块接收短信处理程序					 */
/*	  参数 *str:接收短信内容						 */
/*		 *telno:接收短信手机号					 */
/*************************************************/
void SmsProcess(char* smsstr, int index) {
    int len = 0, i;
    //	char	temp[MSG_MAXLEN];

    len = gsmDecodePdu(smsstr, &smpara);
    fprintf(stderr, "数据：R(%d)---\n", len);
    for (i = 0; i < len; i++) {
        SmsReceBuf[SmsRHead] = smpara.TP_UD[i];
        fprintf(stderr, "%02x ", SmsReceBuf[SmsRHead]);
        SmsRHead = (SmsRHead + 1) % BUF_LEN;
    }
    fprintf(stderr, "\n---\n");
}

/*************************************************/
/*    m37i模块串口接收处理程序					 */
/*************************************************/
void RxHandle(int cmd) {
    int count      = 0;
    unsigned int i = 0;
    char *str, *str1, *str2, *str3;
    unsigned int lensub, lenstr, pt;
    INT8U buf[BUF_LEN]; //	模块接收处理缓冲区
    INT8U tmp[BUF_LEN];
    int recno;
    int k1, k2, k3; // CMGL临时数据

    lenstr = (rxpt - handlept + BUF_LEN) % BUF_LEN;
    if (!lenstr)
        return;
    lensub = 0;
    pt     = handlept;
    for (i = ZERO; i < lenstr; i++) {
        buf[i] = (char)rxbuf[pt];
        pt     = (pt + 1) % BUF_LEN;
    }

    buf[i] = (char)0x00;
    str    = (char*)buf;
    str1   = (char*)tmp;
    str2   = (char*)tmp;
    str3   = (char*)tmp;
    SdPrint("r=%d,h=%d,lenstr=%d\n", rxpt, handlept, lenstr);
    SdPrint("R:%s \n", str);
    while (lenstr) {
        //		ClearWaitTimes(ProjectNo,JProgramInfo);
        usleep(10000);
        str1 = strstr(str, CRLF);
        if (str == str1) { // 开始字符为CRLF
                           //			SdPrint("rec CRLF\n");
            lensub   = str1 - str + strlen(CRLF);
            handlept = (handlept + lensub) % BUF_LEN;
            lenstr -= lensub;
            str += lensub;
            //			fprintf(stderr,"CRLFstr=%s \n",str);
            if (cmd == TXCOMMAND_CMGS) {
                RetCode |= RX_CRLF;
            }
            continue;
        }

        if ((str[0] == CR) || (str[0] == LF)) { // 开始字符为CR或LF
            SdPrint("rec CR or LF\n");
            handlept = (handlept + 1) % BUF_LEN;
            lenstr--;
            str++;
            continue;
        }

        str1 = strstr(str, RXATE0);
        if (str1 == str) {
            str2 = strstr(str1, CRLF);
            if (str2 == NULL) {
                //				SdPrint("str2=%s",str2);
                return;
            }
            str1 = strstr(str2, OK);
            if (str1 == NULL) {
                //				SdPrint("str1=%s",str1);
                return;
            }
            RetCode |= RX_ATE0;
            lensub = str2 - str + 2;
            lenstr -= lensub;
            str      = str2 + 2;
            handlept = (handlept + lensub) % BUF_LEN;
            continue;
        }

        str1 = strstr(str, RXATE1);
        if (str1 == str) {
            str1 = strstr(str, OK);
            if (str1 == NULL)
                return;

            str2 = strstr(str1, CRLF);
            if (str2 == NULL)
                return;

            RetCode |= RX_ATE1;
            lensub = str2 - str + 2;
            lenstr -= lensub;
            str      = str2 + 2;
            handlept = (handlept + lensub) % BUF_LEN;
            continue;
        }

        str1 = strstr(str, GREAT); //短信息提示符
        if (str1 == str) {
            handlept = (handlept + sizeof(GREAT)) % BUF_LEN;
            lenstr -= sizeof(GREAT);
            str += sizeof(GREAT);
            RetCode |= RX_GREAT;
            continue;
        }

        str1 = strstr(str, ERROR);
        if (str1 == str) {
            str2 = strstr(str1, CRLF);
            if (str2 == NULL)
                return;

            RetCode |= RX_ERROR;
            lensub   = str2 - str + 2;
            handlept = (handlept + lensub) % BUF_LEN;
            lenstr -= lensub;
            str = str2 + 2;
            continue;
        }

        str1 = strstr(str, CPBR); //电话号码本
        if (str == str1) {
            str2 = strstr(str1, OK);
            if (str2 == NULL)
                return;

            str1 = strstr(str, ",\""); //,"号码开始
            str1 += 2;
            str3    = strstr(str1, "\","); //“，号码结束
            str3[0] = 0;
            // TODO:以下两行由于之前09规约在现场没有用到本机的sim卡卡号，暂先不加，后续程序都改完之后，视情况再修改。
            //			strcpy(JConfigInfo->jzqpara.SIMCard,str1);
            //			SdPrint("TEL=%s\n",JConfigInfo->jzqpara.SIMCard);
            lensub   = (str2 - str);
            handlept = (handlept + lensub + 2) % BUF_LEN;
            lenstr -= (lensub + 2);
            str = str2 + strlen(CRLF);
            RetCode |= RX_CPBR;
            continue;
        }

        str1 = strstr(str, CSQ);
        if (str1 == str) {
            str2 = strstr(str, CRLF);
            if (str2 == NULL)
                return;
            str1 += strlen(CSQ);
            if (sscanf(str1, "%d,%d", &k1, &k2) == 2) {
                //				JDataFileInfo->DayRunTj.GprsCSQ = k1;
                //                JParamInfo3761->Gprs_csq = k1;
            }
            lensub   = str2 - str + 2;
            handlept = (handlept + lensub) % BUF_LEN;
            lenstr -= lensub;
            str = str2 + 2;
            RetCode |= RX_CSQ;
            continue;
        }

        //查询sim卡初始化状态
        str1 = strstr(str, RXQINISTAT);
        if (str1 == str) {
            str2 = strstr(str, CRLF);
            if (str2 == NULL)
                return;
            str1 += strlen(RXQINISTAT);
            if (sscanf(str1, "%d", &k1) == 1) {
                if (k1 == 3) {
                    RetCode |= RX_QINISTAT;
                }
            }
            lensub   = str2 - str + 2;
            handlept = (handlept + lensub) % BUF_LEN;
            lenstr -= lensub;
            str = str2 + 2;
            continue;
        }

        str1 = strstr(str, CMGS);
        if (str1 == str) {
            str2 = strstr(str1, CRLF);
            if (str2 == NULL)
                return;

            RetCode |= RX_CMGS;
            lensub = str2 - str + 2;
            lenstr -= lensub;
            str      = str2 + 2;
            handlept = (handlept + lensub) % BUF_LEN;
            continue;
        }

        str1 = strstr(str, RXCREG);
        if (str1 == str) {
            str2 = strstr(str, CRLF);
            if (str2 == NULL)
                return;
            str1 += strlen(RXCREG);
            if (sscanf(str1, "%d,%d", &k1, &k2) == 1) {
                if ((k1 == 0) && (k2 == 1 || k2 == 5)) {
                    RetCode |= RX_CREG;
                }
            }
            lensub   = str2 - str + 2;
            handlept = (handlept + lensub) % BUF_LEN;
            lenstr -= lensub;
            str = str2 + 2;
            continue;
        }

        str1 = strstr(str, AT_);
        if (str1 == str) {
            str1 = strstr(str, CRLF);
            if (str1 == NULL)
                return;

            handlept = rxpt;
            RetCode |= RX_AT_;
            return;
        }

        str1 = strstr(str, CMTI); //	收到短信息
        if (str == str1) {
            str1 = strstr(str, CRLF);
            if (str1 == NULL)
                return;
            //			str[str1-str] = 0x00;		//	str = +CMTI: "MT", nn  -- 其中nn为短信息号
            str3 = str + strlen(CMTI);
            for (i = ZERO; i < RXMSG_NUM; i++) {
                if (rxmsg[i].rxmsgflag)
                    continue;
                if (sscanf(str3, "%d", &k1) == 1) {
                    rxmsg[i].rxmsgno   = k1;
                    rxmsg[i].rxmsgflag = 1;
                }
                break;
            }
            RetCode |= RX_CMTI;
            lenstr -= str1 - str + 2;
            handlept = (handlept + str1 - str + 2) % BUF_LEN;
            str      = str1 + 2;
            continue;
        }

        str1 = strstr(str, CMTI1); //	收到短信息
        if (str == str1) {
            str1 = strstr(str, CRLF);
            if (str1 == NULL)
                return;

            //			str[str1-str] = 0x00;		//	str = +CMTI: "SM", nn  -- 其中nn为短信息号
            str3 = str + strlen(CMTI);
            for (i = ZERO; i < RXMSG_NUM; i++) {
                if (rxmsg[i].rxmsgflag)
                    continue;
                if (sscanf(str3, "%d", &k1) == 1) {
                    rxmsg[i].rxmsgno   = k1;
                    rxmsg[i].rxmsgflag = 1;
                }
                break;
            }
            RetCode |= RX_CMTI;
            lenstr -= str1 - str + 2;
            handlept = (handlept + str1 - str + 2) % BUF_LEN;
            str      = str1 + 2;
            continue;
        }

        str1 = strstr(str, CMGR1);
        if (str1 == str) {
            str2 = strstr(str1, CRLF);
            if (str2 == NULL)
                return;

            RetCode |= RX_CMGR1;
            lensub = (str2 - str + 2);
            lenstr -= lensub;
            str      = str2 + 2;
            handlept = (handlept + lensub) % BUF_LEN;
            continue;
        }
        //适用于PDU格式的CMGR数据接收处理
        str1 = strstr(str, CMGR);
        if (str == str1) { //读短信
            str3 = strstr(str, OK);
            if (str3 == NULL)
                return;
            str2 = strstr(str, CRLF); // str2 -- 第１行+CMGR: 1,,21
            if (str2 == NULL)
                return;
            str2 += 2;
            lensub = str3 - str1 + sizeof(OK);
            if (sscanf(str1, "+CMGR: %d", &recno) == 1) {
                fprintf(stderr, "str2=%s,recno=%d\n", str2, recno);
                SmsProcess(str2, recno);
                //				INT8U xx[] =
                //{0x68,0x6A,0x01,0x6A,0x01,0x68,0x4A,0x32,0x05,0x01,0x00,0x02,0x04,0xF1,0x00,0x00,0x02,0x01,0x02,0x00,0x02,0x00,0x02,0x00,0x62,0x1E,0x59,0x57,0x40,0x09,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x03,0x00,0x7F,0x1E,0x35,0x35,0x38,0x09,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x45,0x32,0x14,0x15,0x00,0xF6,0x16};
                //				static int flag = 0;
                //				if(flag == 0)
                //				{
                //					INT8U xx[] =
                //{0x68,0x32,0x00,0x32,0x00,0x68,0x4b,0x73,0x91,0x58,0x78,0x02,0x0c,0x61,0x00,0x00,0x02,0x00,0x90,0x16};
                //					SendToSMS(xx,sizeof(xx));
                //					flag = 1;
                //				}
            }
            fprintf(stderr, "str3=%s\n", str3);
            //			fprintf(stderr,"smsprocess,str2=%s\n",str2);
            //			fprintf(stderr,"lensub=%d\n",lensub);
            handlept = (handlept + lensub) % BUF_LEN;
            fprintf(stderr, " hand=%d, rx=%d ,lensub=%d ,lenstr=%d ", handlept, rxpt, lensub, lenstr);
            lenstr -= lensub;
            str = str3 + sizeof(OK);
            RetCode |= RX_CMGR;
            fprintf(stderr, " str=%s ", str);
            continue;
        }

        str1 = strstr(str, CMS_ERROR);
        if (str == str1) {
            str2 = strstr(str1, CRLF);
            if (str2 == NULL)
                return;

            lensub   = str2 - str + 2;
            handlept = (handlept + lensub) % BUF_LEN;
            lenstr -= lensub;
            str = str2 + 2;
            RetCode |= RX_CMS_ERROR;
            continue;
        }

        str1 = strstr(str, CMGL); //+CMGL: 1,1,,21,PDU格式前面固定18（0891683108505305F）个字节+返回的数据（21）长度
        if (str1 != NULL) {
            str2 = strstr(str, CRLF);
            if (str2 == NULL)
                return;
            str2 += 2;
            str3 = strstr(str2, CRLF);
            if (str3 == NULL)
                return;
            //			fprintf(stderr,"str=%s\n",str);
            //			fprintf(stderr,"str2=%s\n",str2);
            //			fprintf(stderr,"str3=%s\n",str3);
            if (sscanf(str, "+CMGL: %d,%d,,%d", &k1, &k2, &k3) == 3) {
                //				fprintf(stderr,"k1=%d,k2=%d,k3=%d,ret=%d\n",k1,k2,k3,ssret);
                for (i = ZERO; i < RXMSG_NUM; i++) {
                    if (rxmsg[i].rxmsgflag)
                        continue;
                    rxmsg[i].rxmsgno   = k1;
                    rxmsg[i].rxmsgflag = 1;
                    SdPrint("rxmsg[%d]=%d\n", i, rxmsg[i].rxmsgno);
                    break;
                }
            }
            //			fprintf(stderr,"QQ:lensub=%d,handlept=%d,lenstr=%d\n",lensub,handlept,lenstr);
            lensub   = str3 - str + 2;
            handlept = (handlept + lensub) % BUF_LEN;
            lenstr -= lensub;
            str -= lensub;
            //			fprintf(stderr,"SS:lensub=%d,handlept=%d,lenstr=%d\n",lensub,handlept,lenstr);
            //			fprintf(stderr,"str-lensub=%s\n",str);
            RetCode |= RX_CMGL1;
            continue;
        }

        str1 = NULL;
        str1 = strstr(str, OK);
        if (str1 != NULL) {
            fprintf(stderr, "strstr=%s\n", str1);
            //			str2 = strstr(str,CRLF);
            //			if ( str2 == NULL )	return;
            lensub   = str1 - str + sizeof(OK);
            handlept = (handlept + lensub) % BUF_LEN;
            lenstr -= lensub;
            str = str1 + sizeof(OK);
            RetCode |= RX_OK;
            continue;
        }

        //		str1 = NULL;
        //		str1 = strstr(str,OK1);
        //		if ( str1 !=NULL )	{
        ////			fprintf(stderr,"strstr=%s\n",str1);
        ////			str2 = strstr(str,CRLF);
        ////			if ( str2 == NULL )	return;
        //			lensub = str1 - str + sizeof(OK);
        //			handlept = (handlept + lensub)%BUF_LEN;
        //			lenstr -= lensub;
        //			str = str1 + sizeof(OK);
        //			RetCode |= RX_OK;
        //			continue;
        //		}

        if ((handlept == rxpt) || str == NULL) {
            fprintf(stderr, "处理帧：str=%s,hand=%d,rxpt=%d,退出！\n", str, handlept, rxpt);
            break;
        }
        count++;
        if (count >= 5000) {
            SdPrint("  rec count %d!!! \n", count);
            break;
        }
    } // while(lenstr)
    SdPrint("Ret=%llx\n", RetCode);
}

/*************************************************/
/*    M37i模块串口发送模块初始化数据				 */
/*	  参数	*str：发送字符						 */
/*************************************************/
void put1_str_CRLF(char* str) {
    int len;
    RetCode = 0;
    len     = strlen(str);
    fprintf(stderr, "--------Len=%d,S:%s\n", len, str);
    write(smsPort, str, len);
}

/*************************************************/
/*    M37i模块串口发送等待返回程序				 */
/*	  参数	cmd：AT命令代码						 */
/*			*str: AT命令						 	 */
/*			count1:重复发送AT命令时间，100ms*count1	 */
/*	  返回值：接收正确命令返回1，否则，返回0		 */
/*************************************************/
int put1_str_CRLF_Wait(int cmd, char* str, int count1) {
    int flag = 0;
    int count, num = 0;
    int sendnum = 3;

    while (num < sendnum) {
        put1_str_CRLF(str);
        count = 0;
        while (count < count1) {
            //			ClearWaitTimes(ProjectNo,JProgramInfo);
            usleep(100000); // 100ms
            flag = 0;
            RxHandle(cmd);
            switch (cmd) {
                case TXCOMMAND_ATE0:
                    if ((RetCode & RX_ATE0) || (RetCode & RX_OK))
                        flag = 1;
                    break;
                case TXCOMMAND_CPBR:
                    if ((RetCode & RX_CPBR) || (RetCode & RX_OK)) {
                        flag = 1;
                    }
                    break;
                case TXCOMMAND_CPBS:
                    if (RetCode & RX_OK)
                        flag = 1;
                    break;
                case TXCOMMAND_CSQ:
                    if ((RetCode & RX_OK) || (RetCode & RX_CSQ))
                        flag = 1;
                    break;
                case TXCOMMAND_CMGS:
                    if ((RetCode & RX_GREAT) || (RetCode & RX_CRLF))
                        flag = 1;
                    break;
                case TXCOMMAND_CREG:
                    if ((RetCode & RX_OK) || (RetCode & RX_CRLF))
                        flag = 1;
                    break;
                case TXCOMMAND_QINISTAT:
                    if ((RetCode & RX_QINISTAT) || (RetCode & RX_CRLF))
                        flag = 1;
                    break;
                case TXCOMMAND_CPMS:
                    if ((RetCode & RX_OK) || (RetCode & RX_CRLF))
                        flag = 1;
                    break;
                //			case TXCOMMAND_CMGS_D:
                //				if ( RetCode & RX_OK )					flag = 1;
                //				else if ( (RetCode & RX_CMS_ERROR)||(RetCode & RX_ERROR) )	{
                //					flag = 1;
                //				}
                //				break;
                case TXCOMMAND_CMGL:
                    // if ( (RetCode & RX_CMGL)||(RetCode & RX_CMGL1)||(RetCode & RX_OK))	flag = 1;
                    if (RetCode & RX_OK)
                        flag = 1;
                    break;
                case TXCOMMAND_CMGR:
                    if ((RetCode & RX_CMGR) || (RetCode & RX_CMGR1) || (RetCode & RX_OK))
                        flag = 1;
                    else if ((RetCode & RX_CMS_ERROR) || (RetCode & RX_ERROR)) {
                        flag = 1;
                    }
                    break;
                case TXCOMMAND_CMGD:
                    if (RetCode & RX_OK)
                        flag = 1;
                    else if ((RetCode & RX_CMS_ERROR) || (RetCode & RX_ERROR)) {
                        StartCMGL = 1;
                        flag      = 1;
                    }
                    break;
                case TXCOMMAND_SBV:
                case TXCOMMAND_CSCS:
                case TXCOMMAND_CNMI:
                case TXCOMMAND_CMGF:
                default:
                    if ((RetCode & RX_OK) || (RetCode & RX_ERROR))
                        flag = 1;
                    break;
            } // switch(cmd) end
            if (flag) {
                //				SMSLinkCount = 0;
                return flag;
            }
            count++;
        } // while(count<count1) end
        sleep(1);
        num++;
    } // while(num < sendnum) end
    if (!flag) {
        fprintf(stderr, " >>>>超时无应答 \n");
    }
    //	fprintf(stderr,"flag=%d ",flag);
    return flag;
}

/*****************************************************/
/*    M37i模块发送AT+CMGL="ALL"命令读所有短信		 */
/*****************************************************/
void SendCMGL(int minute) //	AT+CMGL="ALL"
{
    static int lastMinute = 100;
    int i;

    if (StartCMGL || (minute != lastMinute)) {
        lastMinute = minute;
        StartCMGL  = 0;

        while (1) {
            //			ClearWaitTimes(ProjectNo,JProgramInfo);
            if (put1_str_CRLF_Wait(TXCOMMAND_CMGL, AT_CMGL_READ, 600)) {
                //				fprintf(stderr,"TXCOMMAND_CMGL:RetCode=%llx........\n",RetCode);
                if ((RetCode & RX_OK)) {
                    for (i = 0; i < RXMSG_NUM; i++) {
                        if (rxmsg[i].rxmsgflag) {
                            fprintf(stderr, "no read CMGL:%d\n", rxmsg[i].rxmsgno);
                        }
                    }
                    //				fprintf(stderr,"rxpt=%d,handlept=%d,return........\n",rxpt,handlept);
                } else {
                    handlept = rxpt;
                    fprintf(stderr, "CMGL error\n");
                }
            }
            if (put1_str_CRLF_Wait(TXCOMMAND_CMGL, AT_CMGL_ALL, 600)) {
                if ((RetCode & RX_OK)) {
                    for (i = 0; i < RXMSG_NUM; i++) {
                        if (rxmsg[i].rxmsgflag) {
                            fprintf(stderr, "all sms CMGL:%d\n", rxmsg[i].rxmsgno);
                        }
                    }
                    //				fprintf(stderr,"rxpt=%d,handlept=%d,return........\n",rxpt,handlept);
                    return;
                } else {
                    handlept = rxpt;
                    fprintf(stderr, "CMGL error\n");
                    return;
                }
            }
        }
    }
}

/*****************************************************/
/*   m37i模块发送读短信命令							 */
/*****************************************************/
void SendCMGR(int msgno) {
    char tempstr[16];

    sprintf(tempstr, "%s%d\r\n", AT_CMGR, msgno);
    if (put1_str_CRLF_Wait(TXCOMMAND_CMGR, tempstr, 1000)) {
        if (RetCode & RX_CMGR) {
            sprintf(tempstr, "%s%d\r\n", AT_CMGD, msgno);
            if (put1_str_CRLF_Wait(TXCOMMAND_CMGD, tempstr, 1000)) {
                RetCode = 0;
            }
        } else {
            RetCode = 0;
        }
    }
}

void m37i_at_init(int day) {
    static int m37i_init_flag = 1;
    static int lastday;

    if (m37i_init_flag == 1 || (day != lastday)) {
        lastday = day;
        if (put1_str_CRLF_Wait(TXCOMMAND_ATE0, ATE0, 10)) {
            if (put1_str_CRLF_Wait(TXCOMMAND_QINISTAT, AT_QINISTAT, 10)) {
                if (put1_str_CRLF_Wait(TXCOMMAND_CSQ, AT_CSQ, 100)) {
                    //					if(put1_str_CRLF_Wait(TXCOMMAND_CREG,AT_CREG,100)){
                    if (put1_str_CRLF_Wait(TXCOMMAND_CMGF, AT_CMGF, 10)) {
                        if (put1_str_CRLF_Wait(TXCOMMAND_CSCS, AT_CSCS, 10)) {
                            if (put1_str_CRLF_Wait(TXCOMMAND_CNMI, AT_CNMI, 10)) {
                                //								if(put1_str_CRLF_Wait(TXCOMMAND_CPMS,AT_CREG,10)) {
                                m37i_init_flag = 0;
                                //								}
                                //									JProgramInfo->gprsinitflag = 0;
                            }
                        }
                    }
                    //					}
                }
            }
        }
    }
    printf("m37i_init_flag %d\n", m37i_init_flag);
}

void* RecePro(void* param) {
    static unsigned char TempBuf[1024];

	if (smsPort != -1) {
		//			SdPrint("Flag=%d,smsPort=%d ",SMSLinkFlag,smsPort);
		bzero(TempBuf, 1024);
		if (((handlept - rxpt - 256 + BUF_LEN) % BUF_LEN) > 0) { //判断有空
			int smslen = read(smsPort, TempBuf, 256);
			if (smslen > 0) {
				SdPrint("RRRRRRRRRR-%d=", smslen);
				for (int i = 0; i < smslen; i++) {
					rxbuf[rxpt] = TempBuf[i];
					SdPrint("%02x,", rxbuf[rxpt]);
					SdPrint("%c,", rxbuf[rxpt]);
					rxpt = (rxpt + 1) % BUF_LEN;
				}
				//				TempBuf[smslen]=' ';
				SdPrint("rxpt=%d, %s\n", rxpt, TempBuf);
				SdPrint("\n");
			}
		}
	}
}

void dealinit() {
    rxpt     = 0;
    handlept = 0;
    memset(rxbuf, 0, BUF_LEN);
    memset(rxmsg, 0, sizeof(rxmsg));
}

void setPort(int port) {
	smsPort = port;
}

void deal_vsms(int msmport) {
    TS ts;
    fprintf(stderr, "smsPort=%d\n", smsPort);

    if(smsPort > 0){
		TSGet(&ts);
		RxHandle(0);
		m37i_at_init(ts.Day); //(gprs模块初始化通知)一天一次M37i模块短信模式初始化
		SendCMGL(ts.Minute); //定时1分钟，或有短信通知查短信
		//当有短信时，处理短信数据
		for (int i = 0; i < RXMSG_NUM; i++) {
			//			fprintf(stderr,"rxmsg[i].rxmsgflag=%d",rxmsg[i].rxmsgflag);
			if (rxmsg[i].rxmsgflag) {
				rxmsg[i].rxmsgflag = 0;
				SendCMGR(rxmsg[i].rxmsgno);
			}
		}
    }
}
