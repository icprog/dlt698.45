/*
 * rn8209.h
 *
 *  Created on: Jun 26, 2013
 *      Author: xt
 */

#ifndef RN8209_H_
#define RN8209_H_

/*********校表参数和计量控制寄存器********/
#define SYS_CON         0x002    //系统控制寄存器    2
#define EMU_CON         0x012    //计量控制寄存器    2
#define HFConst         0x022    //脉冲频率控制      2
#define PStart          0x032    //有功启动功率      2
#define QStart          0x042    //无功启动功率      2
#define GainPQA         0x052    //通道A功率增益     2
#define GainPQB         0x062    //通道B功率增益     2
#define PhsA            0x071    //通道A相位补偿     1
#define PhsB            0x081    //通道B相位补偿     1
#define QPhsCal         0x092    //无功相位补偿      2
#define P_OFFA          0x0A2    //通道A有功功率偏移校正 2
#define P_OFFB          0x0B2    //通道B有功功率偏移校正 2
#define RP_OFFA         0x0C2    //通道A无功功率偏移校正 2
#define RP_OFFB         0x0D2    //通道B无功功率偏移校正 2
#define I_OFFA          0x0E2    //通道A电流有效值偏移校正   2电压频率
#define I_OFFB          0x0F2    //通道B电流有效值偏移校正   2
#define I_GAINB         0x102    //通道B电流增益            2

/*********计量参数和状态寄存器********/
#define PFCnt           0x202    //快速有功脉冲计数  2
#define QFCnt           0x212    //快速无功脉冲计数  2
#define I_RMS_A         0x223    //通道A电流有效值   3
#define I_RMS_B         0x233    //通道B电流有效值   3
#define U_RMS           0x243    //电压通道有效值    3
#define U_Freq          0x252    //电压频率          2
#define Power_PA        0x264    //通道A有功功率     4
#define Power_PB        0x274    //通道B有功功率     4
#define Power_Q         0x284    //无功功率          4
#define Energy_P        0x293    //有功能量          3
#define Energy_P2       0x2A3    //有功能量 读后清零 3
#define Energy_Q        0x2B3    //无功能量          3
#define Energy_Q2       0x2C3    //无功能量 读后清零 3
#define EMU_Status      0x2D3    //计量状态及校验和  3

/*********中断寄存器********/
#define EMU_IE          0x401    //中断允许寄存器        1
#define EMU_IF          0x411    //中断标志寄存器  读后清零  1
#define EMU_RIF         0x421    //复位中断状态寄存器  读后清零  1

/*********系统状态寄存器********/
#define SYS_Status      0x431    //系统状态                  1
#define SYS_RData       0x444    //上一次SPI读出的数据       4
#define SYS_WData       0x452    //上一次SPI写入的数据       2

#define DeviceID        0x7F3    //RN8209 Device ID，820900h 3

#define CMD_REG         0xEA1    //命令寄存器    1

#define CMD_ENABLE_W        	 0xE5ul    //写使能
#define CMD_DISABLE_W       	 0xDCul    //写禁能
#define CMD_SELECT_A_CHNL        0x5Aul    //选择A作为计量通道
#define CMD_SELECT_B_CHNL        0xA5ul    //选择B作为计量通道

#endif /* RN8209_H_ */
