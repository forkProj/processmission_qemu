.. _LoongArch-System-emulator:

LoongArch 系统模拟器
-------------------------

QEMU 通过 ``qemu-system-loongarch64`` 模拟 LoongArch 64 位系统，
目前仅支持 ``virt`` 机器类型。

使用 KVM 加速时，可模拟 la464 CPU 模型。使用 TCG 默认 CPU 模型时，模拟
la464 功能的子集，足以运行面向 la464 构建的发行版。

开发板专用文档
============================

.. toctree::
   loongarch/virt
