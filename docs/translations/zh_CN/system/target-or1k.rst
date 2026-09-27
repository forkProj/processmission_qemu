.. _OpenRISC-System-emulator:

OpenRISC 系统模拟器
~~~~~~~~~~~~~~~~~~~~~~~~

QEMU 使用 ``qemu-system-or1k`` 模拟 32 位 OpenRISC CPU。

OpenRISC CPU 通常集成在运行于 FPGA 的 SoC 中，这些 SoC 与 QEMU 支持的
or1ksim（原始 OpenRISC 指令级模拟器）采用相同核心架构，因此无需支持很多
不同开发板就能覆盖其硬件生态。

支持的 CPU 为 or1200，具有 MMU，可运行 Linux。

选择开发板模型
======================

通过 -M 或 --machine 指定开发板，默认机器为 or1k-sim。

可以构建一个能在全部 QEMU OpenRISC 机器上启动的 Linux 内核，只需编入
全部所需驱动。QEMU 会创建描述机器的设备树并传给内核，后者据此自行配置。

不过，用户通常使用面向特定机器的专用固件。

如果已有能在硬件运行的系统镜像或内核，先检查 ``-machine help`` 是否列出
对应机器。列出时通常可使用该模型；未列出则镜像几乎肯定无法启动。
也许可以提取文件系统，搭配能够在 QEMU 支持机器上启动的其他内核使用。

如果只想运行 Linux，不需要复现特定硬件的内存少、缺少 PCI 或硬盘等限制，
最好使用 virt。该平台专为虚拟机设计，不对应真实硬件。需要以适当配置编译
Linux；它支持 PCI、virtio 和大容量 RAM。

开发板专用文档
============================

..
   此目录按各文件标题的字母顺序排列，
   与按文件名排序的顺序不同。

.. toctree::
   :maxdepth: 1

   or1k/or1k-sim
   or1k/virt

模拟 CPU 架构支持
=================================

.. toctree::
   or1k/emulation

OpenRISC CPU 功能
=====================

.. toctree::
   or1k/cpu-features
