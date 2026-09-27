.. _ARM-System-emulator:

Arm 系统模拟器
-------------------

QEMU 可模拟 32 位和 64 位 Arm CPU。64 位机器使用 ``qemu-system-aarch64``；
32 位机器可使用它或 ``qemu-system-arm``。通常适用于后者的命令行，在前者
上具有相同行为。

QEMU 对 Arm 客户机的支持较好，能模拟近五十种机器。数量众多是因为 Arm
硬件远比 x86 多样：不同公司把 CPU 和不同设备集成进 SoC，再组成机器，
即使采用同一 SoC，机器仍可能有很大差异。五十种开发板也只覆盖生态的一小部分。

64 位 Arm 情况类似，只是目前实现的机器更少。

除常见的、带 MMU 且可运行 Linux 的 A-profile CPU 外，也支持 Cortex-M0、
Cortex-M4、Cortex-M33 等用于高度嵌入式开发板的 M-profile 微控制器。
多数开发板 CPU 类型与真实硬件一致并固定，通常无需手动指定，virt 等除外。

选择开发板模型
======================

Arm 系统模拟必须用 ``-M`` 或 ``--machine`` 指定开发板，没有默认模型。

Arm 系统的差异涉及基本设计，因此为一台机器制作的操作系统或固件镜像，
通常完全不能用于另一台。习惯标准 PC 式 x86 系统的新用户常对此感到意外。
内核启动后，多数用户空间软件对硬件细节的依赖则小得多。

如果已有能在硬件运行的系统镜像或内核，先检查 ``-machine help`` 是否列出
对应机器。列出时通常可使用该模型；未列出则镜像几乎肯定无法启动。
也许可以提取文件系统，搭配能够在 QEMU 支持机器上启动的其他内核使用。

如果只想运行 Linux，不在意复现特定硬件的内存小、缺少 PCI 或硬盘等限制，
最好使用 ``virt``。它不对应真实硬件，专为虚拟机设计。需要用适当配置编译
Linux。virt 支持 PCI、virtio、较新 CPU、大容量 RAM 和 64 位 CPU。

开发板专用文档
============================

..
   此目录按各文件标题的字母顺序排列，
   与按文件名排序的顺序不同。

.. toctree::
   :maxdepth: 1

   arm/max78000
   arm/ax650x-pyramid
   arm/integratorcp
   arm/mps2
   arm/musca
   arm/realview
   arm/sbsa
   arm/versatile
   arm/vexpress
   arm/aspeed
   arm/bananapi_m2u.rst
   arm/b-l475e-iot01a.rst
   arm/sabrelite
   arm/digic
   arm/cubieboard
   arm/emcraft-sf2
   arm/exynos
   arm/musicpal
   arm/kzm
   arm/nrf
   arm/nuvoton
   arm/s32k5
   arm/imx25-pdk
   arm/mcimx6ul-evk
   arm/mcimx7d-sabre
   arm/imx8m
   arm/orangepi
   arm/phytium_e2000
   arm/phytium_pomelo
   arm/raspi
   arm/rk3588
   arm/collie
   arm/sx1
   arm/stellaris
   arm/stm32
   arm/stm32g474
   arm/virt
   arm/vmapple
   arm/xenpvh
   arm/xlnx-versal-virt
   arm/xlnx-zynq
   arm/xlnx-zcu102

模拟 CPU 架构支持
=================================

.. toctree::
   arm/emulation

Arm CPU 功能
================

.. toctree::
   arm/cpu-features
