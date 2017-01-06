
#ifndef ACCESSFUNH_H_
#define ACCESSFUNH_H_

#include "StdDataType.h"


// 数据块数据保存文件
// 输入参数：fname:主文件名，blockdata：主文件块缓冲区，size:主文件尺寸，index:文件的存储索引位置
// 返回值：=1：文件保存成功，=0，文件保存失败，此时建议产生ERC2参数丢失事件通知主站异常
extern INT8U save_block_file(char *fname,void *blockdata,int size,int index);

// 数据块数据读取文件
// 输入参数：fname:保存文件名，size:文件尺寸
// 输出：    		blockdata：文件数据缓冲区
// 返回值： =1:文件同步成功,使用blockdata数据源初始化内存
//         =0:文件同步失败，返回错误，参数初始化默认值，建议产生ERC2参数丢失事件
extern INT8U block_file_sync(char *fname,void *blockdata,int size,int index);

#endif /* ACCESS_H_ */
