/*
 * OIsetfunc.h
 *
 *  Created on: Sep 18, 2017
 *      Author: lhl
 */

#ifndef OISETFUNC_H_
#define OISETFUNC_H_

#include "Objectdef.h"
#include "Shmem.h"

extern INT16U set300F(OAD oad,INT8U *data,INT8U *DAR);
extern INT16U set3105(OAD oad,INT8U *data,INT8U *DAR);
extern INT16U set3106(OAD oad,INT8U *data,INT8U *DAR);
extern INT16U set310c(OAD oad,INT8U *data,INT8U *DAR);
extern INT16U set310d(OAD oad,INT8U *data,INT8U *DAR);		//电能表飞走  属性6
extern INT16U set310e(OAD oad,INT8U *data,INT8U *DAR);		//电能表停走	属性6;
extern INT16U set310f(OAD oad,INT8U *data,INT8U *DAR);		//终端抄表失败  属性6
extern INT16U set3110(OAD oad,INT8U *data,INT8U *DAR);		//月通信流量超限  属性6
extern INT16U set311c(OAD oad,INT8U *data,INT8U *DAR);		//电能表数据变更监控记录


extern int Set_4000_att2(INT8U *data,INT8U *DAR);
extern INT16U set4000(OAD oad,INT8U *data,INT8U *DAR);
extern INT16U set4001_4002_4003(OAD oad,INT8U *data,INT8U *DAR);	//通信地址，表号，客户编号
extern INT16U set4004(OAD oad,INT8U *data,INT8U *DAR);
extern INT16U set4005(OAD oad,INT8U *data,INT8U *DAR);
extern INT16U set4006(OAD oad,INT8U *data,INT8U *DAR);
extern INT16U set4007(OAD oad,INT8U *data,INT8U *DAR);
extern INT16U set400c(OAD oad,INT8U *data,INT8U *DAR);
extern INT16U set4014(OAD oad,INT8U *data,INT8U *DAR);
extern INT16U set4016(OAD oad,INT8U *data,INT8U *DAR);
extern INT16U set4018(OAD oad,INT8U *data,INT8U *DAR);
extern INT16U set4024(OAD oad,INT8U *data,INT8U *DAR);
extern INT16U set4030(OAD oad,INT8U *data,INT8U *DAR);
extern INT16U set4103(OAD oad,INT8U *data,INT8U *DAR);

extern INT16U set4202(OAD oad,INT8U *data,INT8U *DAR);
extern INT16U set4204(OAD oad,INT8U *data,INT8U *DAR);
extern INT16U set4300(OAD oad,INT8U *data,INT8U *DAR);
extern INT16U set4400(OAD oad,INT8U *data,INT8U *DAR);
extern INT16U set4500(OAD oad,INT8U *data,INT8U *DAR);
extern INT16U set4510(OAD oad,INT8U *data,INT8U *DAR);

extern INT16U set6002(OAD oad,INT8U *data,INT8U *DAR);

extern int setf101(OAD oad,INT8U *data,INT8U *DAR);

extern int setf200(OI_698 oi,INT8U *data,INT8U *DAR);
extern int setf201(OI_698 oi,INT8U *data,INT8U *DAR);
extern int setf202(OI_698 oi,INT8U *data,INT8U *DAR);
extern int setf203(OAD oad,INT8U *data,INT8U *DAR);
extern int setf205(OAD oad,INT8U *data,INT8U *DAR);
extern int setf206(OAD oad,INT8U *data,INT8U *DAR);
extern int setf209(OAD setoad,INT8U *data,INT8U *DAR);

extern int f205_act127(OAD oad,INT8U *data,INT8U *DAR);
#endif /* OISETFUNC_H_ */
