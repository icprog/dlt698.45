#ifndef BASEDEF_H_
#define BASEDEF_H_

#define	A_OUT						fprintf(stderr, "[%s][%s][%d]", __FILE__, __FUNCTION__, __LINE__);
#define	A_FPRINTF(fmt, arg...)		A_OUT fprintf(stderr, fmt, ##arg)



/*-------------------集中器类型宏：以下互斥打开——-------*/
//#define CCTT_I        //I型集中器代码开关
#define CCTT_II     //II型集中器代码开关
//#define SPTF_III   //III型专变代码开关

#endif//BASEDEF_H_
