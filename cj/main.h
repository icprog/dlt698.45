/*
 * main.h
 *
 *  Created on: Feb 10, 2017
 *      Author: lhl
 */

#ifndef MAIN_H_
#define MAIN_H_

extern void setOIChange_CJ(OI_698 oi);
extern void SetApn(int argc, char *argv[]);
extern void SetUsrPwd(int argc, char *argv[]);
extern void setOnlineMode(int argc, char* argv[]);

extern void SetIPort(int argc, char *argv[]);
extern void SetNetIPort(int argc, char* argv[]);
extern void SetID(int argc, char *argv[]);

extern void SetHEART(int argc, char* argv[]);

extern void EsamTest(int argc, char* argv[]);

extern void acs_process(int argc, char *argv[]);

extern void event_process(int argc, char *argv[]);

extern void vari_process(int argc, char *argv[]);
/*
 * 采集监控类文件的读取*/
extern void coll_process(int argc, char *argv[]);

extern void cjframe(int argc, char *argv[]);
extern void cjread(int argc, char *argv[]);
extern void analyTaskData(int argc, char* argv[]);
extern void analyFreezeData(int argc, char* argv[]);
/*
 * 参变量类*/
extern void para_process(int argc, char *argv[]);
extern void InIt_Process(int argc, char *argv[]);
/*
 * 文件传输类对象 ESAM接口类对象 输入输出设备类对象 显示类对象
 * */
extern void inoutdev_process(int argc, char *argv[]);

extern void SetF201(int argc, char* argv[]);
extern void SetF202(int argc, char* argv[]);

extern void getFrmCS(int argc, char* argv[]);
extern void getFrmFCS(int argc, char* argv[]);

void showStatus();

extern void Test(int argc, char *argv[]);

#endif /* MAIN_H_ */

