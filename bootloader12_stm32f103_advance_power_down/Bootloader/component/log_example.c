/**
 * @file log_example.c
 * @brief 日志控制宏使用示例
 * @note  这个文件演示了如何使用LOG_ENABLE宏来控制log_printf的输出
 */

/* 方法1：在编译时通过编译器参数控制
 * 在Makefile或项目设置中添加：
 * -DLOG_ENABLE=0  (禁用日志)
 * -DLOG_ENABLE=1  (启用日志)
 */

/* 方法2：在包含头文件前定义宏 */
#define LOG_ENABLE 1  /* 在这里设置为0可以禁用本文件的所有日志 */

#include "retarget_rtt.h"
#include <stdio.h>

/**
 * @brief 示例函数：演示日志输出
 */
void demo_logging(void)
{
    int test_value = 42;
    
    /* 这些log_printf调用会根据LOG_ENABLE的值决定是否输出 */
    log_printf("系统启动完成\n");
    log_printf("测试数值: %d\n", test_value);
    log_printf("错误代码: 0x%08X\n", 0x12345678);
    
    /* 普通的printf不受LOG_ENABLE影响，总是会输出 */
    printf("这条消息总是会显示\n");
}

/**
 * @brief 条件编译示例：在不同模式下使用不同的日志级别
 */
void demo_conditional_logging(void)
{
#if LOG_ENABLE
    log_printf("调试模式：详细信息输出\n");
    log_printf("内存使用情况检查\n");
    log_printf("性能计时开始\n");
#endif
    
    /* 核心功能代码（无论LOG_ENABLE如何设置都会执行） */
    // 实际的业务逻辑...
    
#if LOG_ENABLE
    log_printf("性能计时结束\n");
#endif
}

/* 使用说明：
 * 
 * 1. 开发阶段：设置 LOG_ENABLE = 1
 *    - 所有 log_printf 会输出调试信息
 *    - 帮助定位问题和观察程序运行状态
 * 
 * 2. 发布阶段：设置 LOG_ENABLE = 0  
 *    - 所有 log_printf 被替换为空操作 ((void)0)
 *    - 不占用运行时间，不产生任何输出
 *    - 代码体积更小，运行更快
 * 
 * 3. 灵活控制：
 *    - 可以在不同的.c文件中设置不同的LOG_ENABLE值
 *    - 可以通过编译器参数全局控制
 *    - 可以定义多个级别的日志宏（如LOG_ERROR, LOG_DEBUG等）
 */ 