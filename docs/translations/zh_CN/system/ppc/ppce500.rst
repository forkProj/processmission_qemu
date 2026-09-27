ppce500 通用平台（``ppce500``）
======================================

PPC 版 QEMU 支持专为模拟和虚拟化设计的特殊 ``ppce500`` 机器。

支持的设备
-----------------

``ppce500`` 机器支持以下设备：

* PowerPC e500 系列核心（e500v2/e500mc/e5500/e6500）
* 配置、控制和状态寄存器（CCSR）
* 支持 MSI 的多核可编程中断控制器（MPIC）
* 一个 16550A UART 设备
* 一个 Freescale MPC8xxx I2C 控制器
* 一个通过 I2C 连接的 Pericom pt7c4338 RTC
* 一个 Freescale MPC8xxx GPIO 控制器
* 通过一个 GPIO 引脚提供的关机功能
* 一个 Freescale MPC8xxx PCI 主机控制器
* 通过 PCI 总线连接的 VirtIO 设备
* 一个 Freescale 增强型安全数字主机控制器（eSDHC）
* 一个 Freescale 增强型三速以太网控制器（eTSEC）

硬件配置信息
----------------------------------

如果未指定 ``-dtb`` 选项，``ppce500`` 机器会自动生成设备树二进制文件（“dtb”）并传给客户机，其中提供系统中各种设备的地址、中断线和其他配置信息。

如果用户希望提供自己的 DTB，可以使用 ``-dtb`` 选项。这些 DTB 应满足以下要求：

* /cpus 节点下的子节点数量应与 QEMU 的 ``-smp`` 选项匹配
* /memory 的 reg 大小应与 QEMU 通过 ``-m`` 选择的 ram_size 匹配

``qemu-system-ppc`` 和 ``qemu-system-ppc64`` 都提供以下 32 位 PowerPC CPU 的模拟：

* e500v2
* e500mc

此外，``qemu-system-ppc64`` 还支持以下 64 位 PowerPC CPU：

* e5500
* e6500

可以通过 ``-cpu`` 命令行选项指定 CPU 类型。如果未指定，会创建使用 e500v2 核心的机器。以下示例创建基于 e6500 的机器：

.. code-block:: bash

  $ qemu-system-ppc64 -nographic -M ppce500 -cpu e6500


启动选项
------------

``ppce500`` 机器可以使用标准 -kernel 功能启动，加载操作系统内核（例如 Linux）或 U-Boot 固件等载荷。

省略 -bios 时，使用默认的 pc-bios/u-boot.e500 固件镜像作为 BIOS。QEMU 根据下表选择要执行的载荷：

===== ========== =======
-bios -kernel    载荷
===== ========== =======
N     N          u-boot
N     Y          kernel
Y     任意       u-boot
===== ========== =======

同时指定 -bios 和 -kernel 时，QEMU 加载 U-Boot，随后 U-Boot 通过内置 "bootm" 命令自动加载 -kernel 参数指定的内核镜像，因此这种情况下需要传统 uImage 格式。

运行 Linux 内核
--------------------

撰写本文时测试了 Linux 主线 v5.11 发布版。要构建能够由 ``ppce500`` 机器以 64 位模式启动的 Linux 主线内核，只需使用以下 defconfig 配置内核：

.. code-block:: bash

  $ export ARCH=powerpc
  $ export CROSS_COMPILE=powerpc-linux-
  $ make corenet64_smp_defconfig
  $ make menuconfig


然后手动选择以下配置：

  Platform support > Freescale Book-E Machine Type > QEMU generic e500 platform

在 QEMU 中使用 ``ppce500`` 机器启动新构建的 Linux 内核：

.. code-block:: bash

  $ qemu-system-ppc64 -M ppce500 -cpu e5500 -smp 4 -m 2G \
      -display none -serial stdio \
      -kernel vmlinux \
      -initrd /path/to/rootfs.cpio \
      -append "root=/dev/ram"


要构建能够由 ``ppce500`` 机器以 32 位模式启动的 Linux 主线内核，采用相同的 64 位配置步骤，但 defconfig 文件应使用 corenet32_smp_defconfig。

启动 32 位 Linux 内核：

.. code-block:: bash

  $ qemu-system-ppc64 -M ppce500 -cpu e500mc -smp 4 -m 2G \
      -display none -serial stdio \
      -kernel vmlinux \
      -initrd /path/to/rootfs.cpio \
      -append "root=/dev/ram"


运行 U-Boot
--------------

撰写本文时测试了 U-Boot 主线 v2021.07 发布版。要构建能够由 ``ppce500`` 机器启动的 U-Boot 主线引导加载程序，请使用 qemu-ppce500_defconfig，命令与上述 Linux 构建命令类似：

.. code-block:: bash

  $ export CROSS_COMPILE=powerpc-linux-
  $ make qemu-ppce500_defconfig


将在构建树中得到 u-boot 文件。

U-Boot 启动时，如果使用 ``-cpu e6500``，会看到以下内容：

.. code-block:: none

  CPU:   Unknown, Version: 0.0, (0x00000000)
  Core:  e6500, Version: 2.0, (0x80400020)


这是因为我们只向 QEMU 指定了核心名称，它没有能够代表实际集成该核心的 SoC 的有效 SVR 值。可以指定 QEMU 内置支持的真实 SoC 设备，但这些 SoC 都是基于 e500v2 的 MPC85xx 系列，因此无法测试为 P4080（e500mc）、P5020（e5500）和 T2080（e6500）构建的内容。

网络
----------

默认情况下，一个标准 VirtIO PCI 网络设备作为以太网接口连接到 PCI 地址 0.1.0，但可以将其切换为 e1000 网卡：

.. code-block:: bash

  $ qemu-system-ppc64 -M ppce500 -smp 4 -m 2G \
                      -display none -serial stdio \
                      -bios u-boot \
                      -nic tap,ifname=tap0,script=no,downscript=no,model=e1000


如果向 QEMU 传入 “-device eTSEC”，``ppce500`` 机器还可以动态实例化 eTSEC 设备：

.. code-block:: bash

  -netdev tap,ifname=tap0,script=no,downscript=no,id=net0 -device eTSEC,netdev=net0


闪存驱动器上的根文件系统
-------------------------------

根文件系统除了放在 RAM 磁盘上，也可以放在 CFI 闪存中。给定一个大小必须为二的幂的 ext2 镜像，可按以下方式使用：

.. code-block:: bash

  $ qemu-system-ppc64 -M ppce500 -cpu e500mc -smp 4 -m 2G \
      -display none -serial stdio \
      -kernel vmlinux \
      -drive if=pflash,file=/path/to/rootfs.ext2,format=raw \
      -append "rootwait root=/dev/mtdblock0"


根文件系统也可以放在模拟 SD 卡中，其大小同样必须为二的幂：

.. code-block:: bash

  $ qemu-system-ppc64 -M ppce500 -cpu e500mc -smp 4 -m 2G \
      -display none -serial stdio \
      -kernel vmlinux \
      -device sd-card,drive=mydrive \
      -drive id=mydrive,if=none,file=/path/to/rootfs.ext2,format=raw \
      -append "rootwait root=/dev/mmcblk0"
