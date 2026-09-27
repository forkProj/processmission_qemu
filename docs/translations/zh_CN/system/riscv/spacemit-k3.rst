.. SPDX-License-Identifier: GPL-2.0-or-later

SpacemiT K3 Pico-ITX 板（``k3-pico-itx``）
================================================

``k3-pico-itx`` 机器模拟在 K3 Pico-ITX 板上启动 SpacemiT K3 SDK Linux 内核所需的标准 RISC-V 平台子集。它重点支持八个 X100 应用 hart，不提供 K3 A100 或 IME hart。

机器采用固定的 CPU 拓扑和内存布局，与 Linux 所见的板级配置一致。必须使用默认配置，不得覆盖 CPU 类型、RAM 大小或 SMP 拓扑。

支持的设备
-----------------

``k3-pico-itx`` 机器支持：

* 八个实现 RVA23S64 的 SpacemiT X100 hart，分为两个集群，每个四个；
* 256 位 RISC-V 向量寄存器、Sstc 以及 Smaia/Ssaia 扩展；
* 从 ``0x102000000`` 开始的 2 GiB RAM；
* 从 ``0x100000000`` 开始的 32 MiB 固件窗口；
* 从 ``0xc0800000`` 开始的 512 KiB 片上 SRAM；
* 时基为 24 MHz 的 ACLINT 软件中断和机器定时器块；
* 机器级和监管者级 APLIC 与 IMSIC 中断控制器；
* 位于 ``0xd4017000``、使用中断源 42、兼容 8250 的 UART0；
* 位于 ``0xd4280000``、使用中断源 99 的 SDHCI0 控制器；
* U-Boot 使用的 SD 时钟、复位和启动模式寄存器；
* 位于 ``0xc0f00000``、使用中断源 234 的 T100 IOMMU 标准 RISC-V 寄存器接口。

启动选项
------------

机器既支持直接启动 Linux，也支持通过 U-Boot proper 和 SD 的固件启动路径。两种路径都使用通用 OpenSBI 固件，并要求外部设备树，因为机器会在启动前验证精确的 K3 Linux 拓扑和地址映射。

直接启动 Linux
~~~~~~~~~~~~~~~~~

以下示例使用从 SpacemiT K3 Buildroot SDK v1.0.2 构建的产物：

.. code-block:: bash

   $ qemu-system-riscv64 \
       -machine k3-pico-itx \
       -bios fw_dynamic.bin \
       -kernel Image \
       -initrd k3-qemu-initramfs.cpio.gz \
       -dtb k3-pico-itx-qemu.dtb \
       -append "earlycon=uart8250,mmio32,0xd4017000,115200 \
                console=ttyS0,115200 rdinit=/init" \
       -nographic -no-reboot


机器提供固定的 2 GiB RAM 和 8 hart 拓扑，因此无需 ``-m`` 或 ``-smp`` 选项。直接内核启动需要 OpenSBI；存在 ``-kernel`` 时会拒绝 ``-bios none``。机器使用固定的 ``spacemit-x100`` CPU 模型。

U-Boot 和 SD 启动
~~~~~~~~~~~~~~~~~~

K3 SDK U-Boot proper 可以作为 OpenSBI 的监管者模式下一阶段运行。将原始 SD 镜像作为第一个 SD 驱动器连接；它连接到 SDHCI0，在 U-Boot 中显示为 MMC 设备 0：

.. code-block:: bash

   $ qemu-system-riscv64 \
       -machine k3-pico-itx \
       -bios fw_dynamic.bin \
       -kernel u-boot.bin \
       -dtb k3-pico-itx-qemu-uboot.dtb \
       -drive file=k3-qemu-sd.raw,if=sd,format=raw,snapshot=on \
       -nographic -no-reboot


QEMU 通过 K3 CIU 启动标志报告 SD 启动。随后 U-Boot 初始化 SDHCI0，找到名为 ``bootfs`` 的 GPT 分区，导入 ``env_k3.txt``，并从该 FAT 分区加载 Linux ``Image``、设备树和可选 initramfs。使用 ``snapshot=on`` 可防止固件或客户机修改源镜像。

控制器通告支持 52 MHz 基础时钟、四位 3.3 V 工作模式、SD 高速模式（最高 50 MHz）和 ADMA2，不通告 1.8 V 信号或 UHS 模式。

限制
-----------

此机器模拟的是启动路径子集，而不是完整 K3 硬件。由于未建模 VS 中断文件和虚拟化路径，它禁用了 X100 H 扩展。也未建模 K3 BootROM、bootinfo 解析、U-Boot SPL，以及 LPDDR 初始化和训练；QEMU 提供已初始化的 RAM，并直接启动 OpenSBI。

未实现 A100 和 IME hart、eMMC、UFS、SPI 闪存、PCIe、网络、多媒体加速器、系统电源管理和大多数板级外设。SDHCI0 实现了本文所述 U-Boot 到 Linux 路径所需的寄存器和 DMA 行为，没有实现所有厂商 PHY 调优模式。

IOMMU 提供标准的 4 KiB 寄存器窗口及其单个有线中断。机器不会修改外部设备树；SDK DTS 目前将此节点标记为禁用，因此客户机 DTB 必须先启用它，Linux 才能探测。尚未连接任何请求方：K3 将 PCIe0 至 PCIe2 映射到 IOMMU，但这些主机桥尚未建模，而在 K3 DTS 中 SDHCI0 不是 IOMMU 客户端。

T100 分布式 IOATC、厂商性能事件与过滤器、混合 WSI/MSI 中断行为，以及 PCIe ATS/PRI 路径也未建模。报告的 56 位物理地址大小是 QEMU 平台模型的选择，并非测量得到的 K3 能力值。
