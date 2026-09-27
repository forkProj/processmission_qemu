.. SPDX-License-Identifier: GPL-2.0-or-later

飞腾 Pomelo 板（``phytium-pomelo``）
==========================================

``phytium-pomelo`` 机器模拟上游 U-Boot 描述的 D2000 Pomelo 板。``phytium-d2000`` 是同一机器的别名。

参考 U-Boot 源码树固定为提交 ``211de43d0f954a00a490220c1aac9db298287c40``。

模型遵循上游 U-Boot 的 ``pomelo_defconfig`` 和 ``phytium-pomelo.dts`` 约定，目前提供：

* 八个兼容 FTC663 的 AArch64 CPU 槽位，分为四个双核集群；
* 带 ITS 的 GICv3，以及 D2000 的分发器和重分发器地址布局；
* 位于 ``0x28001000`` 的 PL011 UART；
* 位于 ``0x40000000``、带 Pomelo PCIe 窗口的通用 PCIe ECAM 主机；
* 位于 D2000 GMAC 地址的两个兼容 DWMAC4 的以太网控制器；
* 低地址外设映射占位区域，以及分离的低地址和高地址 RAM 窗口。

直接启动 Linux
-----------------

未提供 ``-dtb`` 时，机器可以生成最小 D2000 设备树：

.. code-block:: console

   $ qemu-system-aarch64 \
       -machine phytium-pomelo \
       -smp 8 -m 3G \
       -kernel Image \
       -initrd initrd \
       -append 'console=ttyAMA0,115200 root=/dev/vda2 rw' \
       -drive if=none,id=root,file=openeuler.img,format=raw \
       -device virtio-blk-pci,drive=root \
       -display none


可以通过 ``-bios`` 提供上游 Pomelo U-Boot 二进制文件。它被加载到上游 ``CONFIG_TEXT_BASE`` （``0x180000``），并获得该 U-Boot 移植所需的最小 D2000 SMCCC 初始化服务。完整的飞腾 PBF/BL31 固件链属于后续独立模块；机器不解析厂商私有 PBF 镜像格式。

D2000 SDCI 和厂商 PBF 适配器有意不采用 E2000 模型表示。只有当其上游 U-Boot/Linux 接口可以作为稳定的测试约定时，才会添加它们。
