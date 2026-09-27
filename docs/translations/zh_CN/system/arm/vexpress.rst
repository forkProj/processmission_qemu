Arm Versatile Express 板（``vexpress-a9``、``vexpress-a15``）
================================================================

QEMU 建模了 Arm Versatile Express 开发板系列的两个变体：

- ``vexpress-a9`` 模拟 Versatile Express 主板与 CoreTile Express A9x4 子板的组合。
- ``vexpress-a15`` 模拟 Versatile Express 主板与 CoreTile Express A15x2 子板的组合。

注意，此硬件没有 PCI、IDE 或 SCSI，因此唯一可用的存储选项是模拟 SD 卡。

已实现的设备：

- PL041 音频
- PL181 SD 控制器
- PL050 键盘和鼠标
- PL011 UART
- SP804 定时器
- I2C 控制器
- PL031 RTC
- PL111 LCD 显示控制器
- 闪存
- LAN9118 以太网

未实现的设备：

- SP810 系统控制块
- PCI-express
- USB 控制器（Philips ISP1761）
- 本地 DAP ROM
- CoreSight 接口
- PL301 AXI 互连
- SCC
- 系统计数器
- HDLCD 控制器（``vexpress-a15``）
- SP805 看门狗
- PL341 动态内存控制器
- DMA330 DMA 控制器
- PL354 静态内存控制器
- BP147 TrustZone 保护控制器
- TrustZone 地址空间控制器

硬件与 QEMU 模型之间的其他差异：

- 除非通过 ``-smp`` 参数指定其他数量，否则 QEMU 默认创建一个 CPU。
- QEMU 允许通过 ``-m`` 参数指定提供的 RAM 大小。
- QEMU 默认提供的 CPU 既不支持 TrustZone，也不支持虚拟化扩展；如需使用，必须通过 ``-machine secure=on`` 和 ``-machine virtualization=on`` 启用。
- QEMU 提供 4 个 virtio-mmio virtio 传输；在 ``vexpress-a9`` 上起始地址为 ``0x10013000``，在 ``vexpress-a15`` 上起始地址为 ``0x1c130000``，IRQ 从 40 开始。如果命令行提供了 DTB，QEMU 会编辑它，加入适当的条目，向客户机描述这些传输。
- QEMU 目前不支持对地址 0 的内存区域进行动态或静态重映射：它始终映射为第一个闪存组的别名。

启动 Linux 内核
----------------------

使用 ``multi_v7_defconfig`` 构建当前 Linux 内核，应足以启动运行。现在建议采用树外构建（构建多个不同目标时也很有用）。下例中，$BLD 指向构建目录，$SRC 指向 Linux 源代码树根目录。如果从源代码树根目录运行，可省略 $SRC。

.. code-block:: bash

  $ make O=$BLD -C $SRC ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- multi_v7_defconfig
  $ make O=$BLD -C $SRC ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-


通常应通过 SD 卡接口启动根文件系统。根文件系统需要填充到合适的大小。使用适当的 DTB，还可以向 virtio-mmio 总线添加设备。

.. code-block:: bash

  $ qemu-system-arm -cpu cortex-a15 -smp 4 -m 4096 \
      -machine type=vexpress-a15 -serial mon:stdio \
      -drive if=sd,driver=file,filename=armel-rootfs.ext4 \
      -kernel zImage  \
      -dtb vexpress-v2p-ca15-tc1.dtb \
      -append "console=ttyAMA0 root=/dev/mmcblk0 ro"
