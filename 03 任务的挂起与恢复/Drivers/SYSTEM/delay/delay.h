/**
 ****************************************************************************************************
 * @file        delay.h
 * @version     V1.0
 * @date        2020-04-17
 * @brief       基于SysTick的延时驱动模块头文件
 *              声明延时模块的初始化函数及微秒/毫秒级延时函数，根据OS支持宏条件重定义HAL库延时函数
 *              与delay.c配套使用，为上层提供统一的延时接口，支持裸机/OS两种运行模式
 ****************************************************************************************************
 * @version log
 * V1.0 20211103
 * 初始版本发布，实现裸机模式下的延时函数声明，以及HAL库延时函数的条件重定义
 ****************************************************************************************************
 */
 
#ifndef __DELAY_H
#define __DELAY_H

#include "./SYSTEM/sys/sys.h"

/**
 * @brief  延时模块初始化函数声明
 * @param  sysclk: 系统核心时钟频率（CPU主频），单位：MHz（例：72代表72MHz）
 * @retval 无
 * @note   需在系统时钟初始化完成后调用，用于配置延时计时的基准系数
 */
void delay_init(uint16_t sysclk);

/**
 * @brief  毫秒级延时函数声明
 * @param  nms: 需要延时的毫秒数，取值范围需参考底层实现，避免超出计数上限
 * @retval 无
 * @note   裸机模式下基于delay_us实现，OS模式下优先调用OS原生非阻塞延时
 */
void delay_ms(uint16_t nms);

/**
 * @brief  微秒级延时函数声明
 * @param  nus: 需要延时的微秒数，取值范围需参考底层实现，避免超出计数上限
 * @retval 无
 * @note   基于SysTick时钟提取法实现，计时精度由系统主频决定
 */
void delay_us(uint32_t nus);

/**
 * @brief  条件编译：重定义HAL库延时函数HAL_Delay
 * @note   1. 仅当未启用操作系统（SYS_SUPPORT_OS为0）时生效
 *         2. HAL库默认HAL_Delay依赖SysTick，与本延时模块冲突，故重定义为调用本模块的delay_ms
 *         3. 启用OS时由delay.c实现该函数，无需在此处声明
 * @param  Delay: 需要延时的毫秒数，与HAL库原函数参数一致
 * @retval 无
 */
#if (!SYS_SUPPORT_OS)
    void HAL_Delay(uint32_t Delay);
#endif

#endif
