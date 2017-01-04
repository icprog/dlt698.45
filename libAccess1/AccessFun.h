
#ifndef ACCESSFUNH_H_
#define ACCESSFUNH_H_

extern unsigned short GetMCurrentData(int mtype,int id,unsigned char* data,int len);
extern unsigned short GetMHisData(int mtype,int id,int year,int month,int day,unsigned char* data,int len);
extern unsigned short GetMPara(int mtype,int id,unsigned char* data,int len);
extern unsigned short SetMCurrentData(int mtype,int id,unsigned char* data,int len);
extern unsigned short SetMHisData(int mtype,int id,int year,int month,int day,unsigned char* data,int len);
extern unsigned short SetMPara(int mtype,int id,unsigned char* data,int len);
//extern unsigned short SaveMPara(int mtype,int id,unsigned char* data,int len);
extern void SaveMPara(void);


#endif /* ACCESS_H_ */
