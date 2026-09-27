CPU 功能
============

QEMU 的 OpenRISC 模拟提供以下内置功能：

- 影子通用寄存器
- 128 项、单路 MMU TLB
- 电源管理（PM）
- 可编程中断控制器（PIC）
- Tick 定时器

默认全部启用，可检查单元存在寄存器 ``UPR`` 和 CPU 配置寄存器 ``CPUCFGR``
确认。
