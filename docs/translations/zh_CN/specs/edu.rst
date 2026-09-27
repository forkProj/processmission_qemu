EDU 设备
========================================

..
   Copyright (c) 2014-2015 Jiri Slaby

   This document is licensed under the GPLv2 (or later).

这是用于学习编写内核驱动的教学设备，最初用于 Masaryk 大学的 Linux 内核课程。
学生需要为此虚拟设备编写处理 I/O、中断、DMA 等功能的驱动。

设备行为与 Liberouter 项目 COMBO6 卡上的 PCI 桥十分相似，
PCI 设备 ID 和 PCI 空间都沿用该设备。

命令行选项
----------------------------------------

``-device edu[,dma_mask=mask]``
    dma_mask 使虚拟设备按指定掩码处理 DMA 地址。
    为教学目的，默认仅支持 28 位，即 256 MiB。
    学生应在操作系统驱动中正确设置设备的 dma_mask。

PCI 规范
------------------------

PCI ID：
   ``1234:11e8``

PCI 区域 0：
   1 MB I/O 内存，用户通过此区域与卡通信。

MMIO 区域规范
----------------------------------------

小于 0x80 的地址只允许 ``size == 4`` 访问，其余允许 ``size == 4`` 或 ``size == 8``。

0x00（只读）：标识
            值格式为 ``0xRRrr00edu``，其中 RR 为主版本，rr 为次版本。

0x04（读写）：设备存活检查
            对值执行简单按位取反，即 C 的 ``~`` 运算符。

0x08（读写）：阶乘计算
            读取写入的值，将其阶乘写回此处。
            仅在状态寄存器（0x20）的阶乘位被清除后发生。

0x20（读写）：状态寄存器
            以下值按位或：

            0x01
              正在计算阶乘，只读。
            0x80
              阶乘计算完成后触发中断。

0x24（只读）：中断状态寄存器
            包含触发中断的值，见下方中断触发寄存器。

0x60（只写）：中断触发寄存器
            触发中断。写入值按位或到中断状态寄存器。

0x64（只写）：中断确认寄存器
            清除中断。将写入值对应的位从中断状态中清除。
            ISR 必须执行此操作才能停止生成中断。

0x80（读写）：DMA 源地址
            DMA 从何处读取。

0x88（读写）：DMA 目标地址
            DMA 向何处写入。

0x90（读写）：DMA 传输计数
            DMA 区域大小。

0x98（读写）：DMA 命令寄存器
            以下值按位或：

            0x01
              启动传输。
            0x02
              方向：0 为 RAM 到 EDU，1 为 EDU 到 RAM。
            0x04
              DMA 完成后触发中断 0x100。

IRQ 控制器
----------------------------------------

写入中断触发寄存器会产生 IRQ，该值出现在中断状态寄存器。
必须将该值写入中断确认寄存器，才能撤销中断。

设备支持 INTx 和 MSI，默认使用 INTx。
即使驱动禁用 INTx、仅使用 MSI，IRQ 处理程序结束时仍需更新确认寄存器。

DMA 控制器
----------------------------------------

必须指定源、目标、大小并启动传输。EDU 在偏移 0x40000 提供一个 4096 字节缓冲区，
正确编程后可对该区域执行双向 DMA。

以下示例使用 PCI 地址 addr，在该缓冲区之间往返传输 100 字节：

::

  addr     -> DMA source address
  0x40000  -> DMA destination address
  100      -> DMA transfer count
  1        -> DMA command register
  while (DMA command register & 1)
      ;

::

  0x40000  -> DMA source address
  addr+100 -> DMA destination address
  100      -> DMA transfer count
  3        -> DMA command register
  while (DMA command register & 1)
      ;
