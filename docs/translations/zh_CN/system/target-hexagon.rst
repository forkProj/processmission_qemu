.. SPDX-License-Identifier: GPL-2.0-or-later

.. _Hexagon-System-emulator:

Hexagon 系统模拟器
-----------------------

使用 ``qemu-system-hexagon`` 模拟 32 位 Hexagon 机器。

Hexagon 机器
================

Hexagon DSP 适用于多种功能，通常作为较大型片上系统（SoC）中的 DSP 子系统。

典型子系统如下图所示。DSP 解除复位前，指令先载入 DDR；随后通过 EVB/复位
向量从 DDR 获取最初的指令。

真实系统通常由 TBU/SMMU 仲裁 AXI 访问，但 QEMU 无需模拟这一部分。

Hexagon DSP 内核采用同时多线程（SMT），最多具有 8 个硬件线程。

.. admonition:: 示意图

 .. code:: text

              AHB (local) bus                     AXI (global) bus
                    │                                 │
                    │                                 │
       ┌─────────┐  │       ┌─────────────────┐       │
       │ L2VIC   ├──┤       │                 │       │
       │         ├──┼───────►                 ├───────┤
       └─────▲───┘  │       │   Hexagon DSP   │       │
             │      │       │                 │       │        ┌─────┐
             │      │       │    N threads    │       │        │ DDR │
             │      ├───────┤                 │       │        │     │
        ┌────┴──┐   │       │                 │       ├────────┤     │
        │QTimer ├───┤       │                 │       │        │     │
        │       │   │       │                 │       │        │     │
        └───────┘   │       │   ┌─────────┐   │       │        │     │
                    │       │  ┌─────────┐│   │       │        │     │
        ┌───────┐   │       │  │  HVX xM ││   │       │        │     │
        │QDSP6SS├───┤       │  │         │┘   │       │        │     │
        └───────┘   │       │  └─────────┘    │       │        └─────┘
                    │       │                 │       │
        ┌───────┐   │       └─────────────────┘       │
        │  CSR  ├───┤
        └───────┘   │   ┌──────┐   ┌───────────┐
                    │   │ TCM  │   │   VTCM    │
                        │      │   │           │
                        └──────┘   │           │
                                   │           │
                                   │           │
                                   │           │
                                   └───────────┘


组件
----------
除 l2vic 和 HVX 外，下列组件尚未在 QEMU 实现。

* L2VIC：二级向量中断控制器，支持 1024 个边沿或电平触发输入。内核 ISA
  的 ``VID``、``VID1`` 系统寄存器可直接读取 L2VIC。
* QTimer：基于 ARMSSE 的可编程定时器，中断接到 L2VIC。系统寄存器
  ``TIMER``、``UTIMER`` 可直接读取 QTimer。
* QDSP6SS：整个 SoC 可访问的 DSP 子系统功能，包括 NMI、看门狗、复位等。
* CSR：配置/状态寄存器。
* TCM：DSP 独占的紧耦合内存，可在与 DDR 隔离或某些引导模式下使用。
* VTCM：DSP 独占的向量紧耦合内存，由部分 HVX 指令访问。
* HVX：支持 64 和 128 字节向量寄存器的向量协处理器。QEMU 未实现 64 字节模式。

引导
-------------
Hexagon 系统通常无法访问块设备，因此 QEMU 的典型用法是将二进制或 ELF
载入内存，再从指定入口执行::

    $ qemu-system-hexagon -kernel ./prog -append 'arg1 arg2'


半主机
-----------
Hexagon 支持类似其他架构的半主机接口，通过 ``trap0`` 指令调用，使客户机
软件访问主机控制台和文件系统。QEMU Hexagon 尚未实现半主机支持。

Hexagon 功能
================
.. toctree::
   hexagon/emulation
   hexagon/cdsp
