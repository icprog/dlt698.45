
#ifndef ACCESSFUNH_H_
#define ACCESSFUNH_H_

#include "StdDataType.h"

/*
 * 复位
 * */
extern int resetClass(OI_698 oi);
/*
 * 方法：Clean()清空
 * 输入参数：oi对象标识
 * 返回值：=1：配置单元删除成功
 * =-1:  未查找到OI类数据
 */
extern int ClearClass(OI_698 oi);
////////////////////////////////////////////////////////////////////////////////////////
/*		第一类参数文件：文件包含接口类公用属性，配置单元按照配置序号在相应的位置存储，
 *           方法：存储更新（追加，更新），删除，清空
 **/
/*
 * 参数类存储
 * 输入参数：oi对象标识，blockdata：主文件块缓冲区，seqnum:对象配置单元序列号，作为文件位置索引
 * 返回值：=1：文件保存成功，=0，文件保存失败，此时建议产生ERC2参数丢失事件通知主站异常
 * =-1:  未查找到OI类数据
 */
extern int saveParaClass(OI_698 oi,void *blockdata,int seqnum);
/*
 * 根据OI、配置序号读取某条配置单元内容
 * 输入参数：oi对象标识，seqnum:对象配置单元序列号
 * 返回值：
 * =1：文件读取成功，blockdata：配置单元内容
 * =0： 文件读取失败
 * =-1:  未查找到OI类数据
 */
extern int readParaClass(OI_698 oi,void *blockdata,int seqnum);
/*
 * 接口类公共属性读取
 * 输入参数：oi对象标识，dest：接口类公共属性
 * 返回值：
 * =1：文件读取成功   =0：文件读取失败   =-1:  未查找到OI类数据信息
 */
extern int readInterClass(OI_698 oi,void *dest);

/*
 * 通过配置序号删除配置单元
 * 输入参数：oi对象标识，seqnum:要删除的配置序号
 * 返回值：=1：配置单元删除成功
 * =-1:  未查找到OI类数据
 */
extern int delClassBySeq(OI_698 oi,void *blockdata,int seqnum);



//////////////////////////////////////////////////////////////////////////////////////////
/*		第二类文件：事件参数、事件记录、当前记录集存储
 *		该类文件根据OI类型生成相应的目录  /nand/oi/
 *		type: 参数类数据存储
 *
 **/
/*
 * 输入参数：	oi:对象标识，seqno:记录序号，blockdata:存储数据，len：存储长度，
 * 			type：存储类型【	para_save：参数文件存储   event_record_save: 事件记录表存储 	current_record_save :当前值记录表存储】
 * 返回值：=1：文件存储成功
 */
extern int saveEventClass(OI_698 oi,INT16U seqno,void *blockdata,int savelen,int type);

/*
 * 输入参数：	oi:对象标识，seqno:记录序号，blockdata:存储数据，savelen：存储长度，
 * 			type：存储类型【	para_save：参数文件存储   event_record_save: 事件记录表存储 	current_record_save :当前值记录表存储】
 * 返回值：=1：文件存储成功
 * =-1: 文件不存在
 */
extern int readEventClass(OI_698 oi,INT16U seqno,void *blockdata,int type);


//////////////////////////////////////////////////////////////////////////////////////////
/************************************
 * 函数说明：获取参数文件对象配置单元的个数saveEventClass
 * 返回值：
 * >=0:  单元个数
 * -1:  未查找到OI类数据
 * -2:	文件记录不完整
 *************************************/
extern long getFileRecordNum(OI_698 oi);

#endif /* ACCESS_H_ */
