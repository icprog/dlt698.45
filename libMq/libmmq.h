/*
 * mq_comm.h
 *
 *  Created on: Apr 18, 2013
 *      Author: nl1031
 */

#ifndef MQ_COMM_H_
#define MQ_COMM_H_

#include <mqueue.h>
#include "../include/StdDataType.h"
#include "../include/ParaDef.h"
typedef struct{
	INT16U 	pid;			   //发送方进程id
	INT32U  cmd;               //命令字
	INT32U  bufsiz;				//buf的有效长度
}mmq_head;

typedef enum
{
	 cjcomm,
	 cjdeal
}PROGS_ID;

typedef struct
{
	PROGS_ID pid;    //消息队列服务器端进程号
	INT8U    name[MMQNAMEMAXLEN]; //消息队列名称
	INT32U   maxsiz; //数据缓冲容量最大值
	INT32U   maxnum; //mq最大消息个数
}mmq_attribute;

extern mqd_t mmq_create(INT8S * name,struct mq_attr *attr,INT32S flags);

/*打开一个消息队列
 *输入:name 消息队列名称，flags 读写标识(O_WRONLY or O_RDONLY)；
 *输入:name 输出：attr 消息队列设置属性,
 *返回：-1:打开失败(errno捕捉),-2：name或attr为NULL,-3:flags值不为O_WRONLY or O_RDONLY,-4获取attr失败
 * */
extern mqd_t mmq_open(INT8S * name,struct mq_attr *attr,INT32S flags);


/*从一个消息队列中获取一个消息
 * 输入：fd消息文件描述符,time_out超时时间
 * 输出：msg_head mq头结构,buff消息内容指针
 * 返回成功时收到的消息字节数,-1获取失败(errno捕捉),-2为fd<0；-3为pmsg为NULL，-4无法获取mq_attr,-5为time_out不在0- 3600之间;
 * */
extern INT32S mmq_get(mqd_t fd, INT32U time_out,mmq_head* msg_head, void* buff);


/*往消息队列发送一条消息体
 * 输入：fd消息文件描述符,time_out超时时间,msg_head mq头结构,buff消息内容指针，buff消息内容指针
 * 		msg_head.bufsiz获取消息缓冲区大小（必须不大于队列attr的mq_msgsize，允许零长度）,
 * 		prio一个用于指定消息优先级的非负整数。消息在队列里依据优先级排序，相同优先级的消息新消息放于旧消息后面。
 * 		如果消息队列已经满了(如当前队列消息个数等于队列的 mq_maxmsg 属性)，那么，默认地，
 * 		会一直阻塞到有足够的空间来把当前消息入队，或者在阻塞的时候被一个信号处理器中断。
 * 		如果 O_NONBLOCK 标志在消息队列描述符里有设置，那么调用将直接由 EAGAIN 错误返回。
 * 返回：成功时返回0；-1发送失败(errno捕捉),-2为fd<0；，-3无法获取mq_attr，-4为msg_head.bufsiz大于attr.mq_msgsize
 * */
extern INT32S mmq_put(mqd_t fd,INT32U time_out,mmq_head msg_head, void * buff, INT8U prio);
/*关闭消息队列描述符 mqdes。
*如果调用进程在消息队列 mqdes 绑定了通知请求，那么这个请求被删除，此后其它进程就可以绑定通知请求到此消息队列。
*/
extern INT32S mmq_close(mqd_t fd);

extern INT8S mqs_send(INT8S* mqname,INT16U pid,INT32U cmd,INT8U* buf,INT32U bufsiz);
#endif /* MQ_COMM_H_ */
