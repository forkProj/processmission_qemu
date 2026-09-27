Boundary Devices SABRE Lite（``sabrelite``）
============================================

Boundary Devices SABRE Lite i.MX6 开发板是一个低成本开发平台，采用性能强劲的 Freescale / NXP Semiconductor i.MX 6 Quad 应用处理器。

支持的设备
-----------------

SABRE Lite 机器支持以下设备：

 * 最多四个 Cortex-A9 核心
 * 通用中断控制器
 * 一个时钟控制器模块
 * 一个系统复位控制器
 * 五个 UART
 * 两个 EPIC 定时器
 * 一个 GPT 定时器
 * 两个看门狗定时器
 * 一个 FEC 以太网控制器
 * 三个 I2C 控制器
 * 七个 GPIO 控制器
 * 四个 SDHC 存储控制器
 * 四个 USB 2.0 主机控制器
 * 五个 ECSPI 控制器
 * 两个 FlexCAN CAN 控制器
 * 一个 SST 25VF016B 闪存

请注意，上述列表是 QEMU SABRE Lite 机器可支持设备的完整超集。在常规用法中，表示真实 SABRE Lite 板的设备树二进制文件仅向客户机软件提供其中部分设备。

启动选项
------------

SABRE Lite 机器可使用标准 -kernel 功能加载 Linux 内核、U-Boot 引导加载程序或 ELF 可执行文件并启动。

运行 Linux 内核
--------------------

撰写本文时测试了 Linux 主线 v5.10 发布版。要构建可由 SABRE Lite 机器启动的 Linux 主线内核，只需使用 imx_v6_v7_defconfig 配置内核：

.. code-block:: bash

  $ export ARCH=arm
  $ export CROSS_COMPILE=arm-linux-gnueabihf-
  $ make imx_v6_v7_defconfig
  $ make


在 QEMU 中使用 SABRE Lite 机器启动新构建的 Linux 内核：

.. code-block:: bash

  $ qemu-system-arm -M sabrelite -smp 4 -m 1G \
      -display none -serial null -serial stdio \
      -kernel arch/arm/boot/zImage \
      -dtb arch/arm/boot/dts/imx6q-sabrelite.dtb \
      -initrd /path/to/rootfs.ext4 \
      -append "root=/dev/ram"


运行 U-Boot
--------------

撰写本文时测试了 U-Boot 主线 v2020.10 发布版。要构建可由 SABRE Lite 机器启动的 U-Boot 主线引导加载程序，请使用 mx6qsabrelite_defconfig，命令与上述 Linux 构建命令类似：

.. code-block:: bash

  $ export CROSS_COMPILE=arm-linux-gnueabihf-
  $ make mx6qsabrelite_defconfig


注意，需要通过以下命令调整设置：

.. code-block:: bash

  $ make menuconfig


然后在 U-Boot 中手动选择以下配置：

  Device Tree Control > Provider of DTB for DT Control > Embedded DTB

要使用 SABRE Lite 机器启动 U-Boot，请将 u-boot 二进制文件传给 -kernel 参数，并提供包含根文件系统的 SD 卡镜像：

.. code-block:: bash

  $ qemu-system-arm -M sabrelite -smp 4 -m 1G \
      -display none -serial null -serial stdio \
      -kernel u-boot


以下示例通过 DHCP 启动 Linux 内核，并使用 SD 卡上的根文件系统。这需要为 QEMU 添加一些命令行参数：

.. code-block:: none

  -nic user,tftp=/path/to/kernel/zImage \
  -drive file=sdcard.img,id=rootfs -device sd-card,drive=rootfs


内置 TFTP 服务器的目录还应包含 SABRE Lite 板的设备树二进制文件。示例 SD 卡镜像只有一个分区，其中填入了根文件系统。可以据实际情况调整内核的 "root=" 启动参数。

U-Boot 启动后，在其命令 shell 中输入以下命令启动 Linux 内核：

.. code-block:: none

  => setenv ethaddr 00:11:22:33:44:55
  => setenv bootfile zImage
  => dhcp
  => tftpboot 14000000 imx6q-sabrelite.dtb
  => setenv bootargs root=/dev/mmcblk3p1
  => bootz 12000000 - 14000000
