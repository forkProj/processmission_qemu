虚拟机间平坦共享内存设备
----------------------------------

ivshmem-flat 面向没有 PCI 总线、无法使用传统 PCI ivshmem 的机器，例如
Cortex-M MCU。控制和状态寄存器直接映射到内存，只需很小的驱动即可与其他
虚拟机交互，适合 Zephyr 等运行在受限目标上的 RTOS。

它同样支持硬件中断通知和共享内存，可与传统 ivshmem 协作。例如，使用 PCI
ivshmem 的 AArch64 Linux 客户机，可以与使用平坦设备的 Arm Zephyr 客户机通信。

不支持 memdev 选项，详见 ivshmem.rst。它依赖服务器创建并分发共享内存文件
描述符及用于通知对端的 eventfd，因此创建设备时服务器必须运行。

中断是可选的。未指定输入 IRQ 时，通知功能禁用并显示警告，既不能通知其他
虚拟机，也不能接收通知；共享内存区域始终存在。

INTRMASK、INTRSTATUS、IVPOSITION、DOORBELL 的偏移和功能遵循 ivshmem 规范，
与 PCI 设备完全一致，见 ./specs/ivshmem-spec.txt。
