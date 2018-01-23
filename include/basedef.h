#ifndef BASEDEF_H_
#define BASEDEF_H_

#define	A_OUT						fprintf(stderr, "[%s][%s][%d]", __FILE__, __FUNCTION__, __LINE__);
#define	A_FPRINTF(fmt, arg...)		A_OUT fprintf(stderr, fmt, ##arg)


/*
 * 由宏定义判断终端类型
 * 现在已改为读取/nor/config/device.cfg
 * 来决定终端的类型
 */
//#define CCTT_I //I型集中器
//#define CCTT_II//II型集中器
//#define SPTF_III//III型专变
//#define HUNAN_SXY		//湖南双协议698-3761切换

#define CCTT1	1//与配置文件约定的I型集中器编号
#define CCTT2	2//与配置文件约定的II型集中器编号
#define SPTF3	3//与配置文件约定的III型专变编号


#endif//BASEDEF_H_
