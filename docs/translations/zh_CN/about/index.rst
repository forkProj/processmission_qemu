---------
关于 QEMU
---------

QEMU 是通用、开源的机器模拟器和虚拟化工具。

QEMU 有多种使用方式。最常见的是 :ref:`System Emulation`，
通过模拟整台机器，包括 CPU、内存和设备，运行客户机操作系统。
此模式下，CPU 可以完全通过软件模拟，也可以与 KVM、Xen 或 Hypervisor.Framework
等虚拟机监控器配合，让客户机直接在宿主 CPU 上运行。

另一种方式是 :ref:`User Mode Emulation`，
在一种 CPU 上运行面向另一种 CPU 编译的进程。此模式始终模拟 CPU。

QEMU 还提供若干独立的 :ref:`命令行工具 <Tools>`，
例如用于创建、转换和修改磁盘镜像的 ``qemu-img``。

.. toctree::
   :maxdepth: 2

   build-platforms
   emulation
   deprecated
   removed-features
   license
