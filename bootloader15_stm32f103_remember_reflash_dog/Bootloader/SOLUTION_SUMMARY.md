# Bootloader 跳转 APP 问题解决方案

## 问题总结

**现象:** Bootloader 能跳转 Keil 编译的 APP，但跳转 GCC 编译的 APP 时进入 HardFault

**根本原因:** 在 Bootloader 中设置 `SCB->VTOR` 的时机问题
- 设置 VTOR 后，CPU 立即开始使用 APP 的向量表
- 在跳转前的短暂时间窗口内，调试器或系统中断被触发
- 中断发生时使用 APP 向量表查找处理函数，但执行上下文仍在 Bootloader 中
- 导致访问无效地址，触发总线故障 → HardFault

## 故障寄存器分析

```
hfsr = 1073741824 (0x40000000) → bit30 (FORCED) = 1：其他故障被强制转为 HardFault
cfsr = 33280 (0x8200) → bit15 (BFARVALID) = 1, bit9 (PRECISERR) = 1：精确总线故障
bfar = 536936452 (0x20010004) → 故障地址在栈区域
vtor = 0 → 向量表仍指向 0x00000000 (Bootloader区域)
```

## 解决方案

**方案1: 让 APP 自己设置 VTOR（推荐）**

在 Bootloader 的 `IAP_JumpToApp()` 函数中注释掉：
```c
// SCB->VTOR = AppAddr;
// __DSB(); __ISB();
```

让 APP 的 `SystemInit()` 函数自己设置 VTOR：
```c
// 在 APP 的 system_stm32f1xx.c 中已有：
SCB->VTOR = VECT_TAB_BASE_ADDRESS | VECT_TAB_OFFSET;
```

**优点:**
- 避免了中断时机冲突
- APP 在正确的执行上下文中设置向量表
- 代码更简洁，责任分离明确

## 最终优化后的跳转流程

```c
static void IAP_JumpToApp(uint32_t app_addr)
{   
    // 1. 验证APP有效性
    if (!IAP_IsValidApp(app_addr)) {
        while (1);  // APP无效，停止跳转
    }
    
    // 2. 关闭全局中断
    __disable_irq();

    // 3. 彻底关闭SysTick和NVIC
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;
    for (uint32_t i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }   

    // 4. 等待中断清除完成
    __DSB();
    __ISB();

    // 5. 获取APP栈顶和复位向量
    uint32_t stack_top = *(uint32_t *)app_addr;
    uint32_t reset_handler = *(uint32_t *)(app_addr + 4);

    // 6. 设置主堆栈指针MSP
    __set_MSP(stack_top);

    // 7. 切换为特权级模式
    __set_CONTROL(0);

    // 8. 不设置VTOR，让APP自己设置

    // 9. 跳转至APP的Reset_Handler
    typedef void (*pFunction)(void);
    pFunction AppEntry = (pFunction)reset_handler;
    AppEntry();
    
    while (1);  // 理论上不会执行到这里
}
```

## 为什么 Keil 编译的 APP 可以工作

可能的原因：
1. **启动序列差异:** Keil 的启动文件可能有不同的中断处理方式
2. **编译优化差异:** 导致时间窗口不同
3. **向量表格式差异:** Keil 和 GCC 生成的向量表可能有细微差别
4. **调试器交互差异:** 不同编译器与调试器的交互方式可能不同

## 最佳实践建议

1. **责任分离:** Bootloader 负责跳转，APP 负责自己的初始化
2. **最小化时间窗口:** 在 Bootloader 中尽量减少设置向量表后的操作
3. **彻底清理环境:** 确保所有中断和外设状态完全清理
4. **添加有效性检查:** 验证 APP 向量表的合法性
5. **使用内存屏障:** 确保关键操作的时序正确

## 总结

这个问题是一个典型的**中断时机冲突**问题。通过让 APP 自己设置向量表，我们避免了在 Bootloader 中可能出现的中断冲突，实现了稳定可靠的跳转。

这种解决方案不仅修复了当前问题，还提高了代码的健壮性和可维护性。
