Xilinx Zynq 板（``xilinx-zynq-a9``）
======================================

Zynq 7000 系列基于 AMD SoC 架构。这些产品在单个器件中集成了功能丰富的双核或单核 Arm Cortex-A9 MPCore 处理系统（PS）以及 AMD 可编程逻辑（PL）。

该 SoC 的文档为 `Zynq 7000 技术参考手册 <https://docs.amd.com/r/en-US/ug585-zynq-7000-SoC-TRM/Zynq-7000-SoC-Technical-Reference-Manual>`__。

QEMU 的 xilinx-zynq-a9 板支持以下设备：

- Arm Cortex-A9 MPCore CPU

  - Cortex-A9 CPU
  - GIC v1 中断控制器
  - 通用定时器
  - 看门狗定时器

- OCM 256KB
- SMC SRAM 64MB
- Zynq SLCR
- 两个 SPI
- QSPI
- UART
- 两个 TTC
- 两个千兆以太网控制器
- 两个 SD 控制器
- XADC
- Arm PrimeCell DMA 控制器
- DDR 内存
- 两个 USB 2.0

运行 Buildroot ZC702 镜像
""""""""""""""""""""""""""""""

Buildroot 提供了 ZC702 defconfig。撰写本文时测试了 Buildroot 2026.05 发布版。在 Buildroot 源代码树中执行：

.. code-block:: bash

  $ make zynq_zc702_defconfig
  $ make


生成的文件位于 ``output/images/``。以下示例使用：

 * ``u-boot.bin``
 * ``uImage``
 * ``sdcard.img``

QEMU 的 SD 卡模型要求镜像大小为二的幂。请复制 Buildroot SD 镜像并调整副本大小，不要修改原始输出：

.. code-block:: bash

  $ cp output/images/sdcard.img sdcard-qemu.img
  $ qemu-img resize -f raw sdcard-qemu.img 128M


要通过 Buildroot 生成的 U-Boot 镜像启动，请使用通用加载器设备将原始 ``u-boot.bin`` 放到其链接地址，并从该地址启动 CPU。``zynq_zc702_defconfig`` U-Boot 镜像是 U-Boot proper，而不是 Zynq 启动 ROM 镜像，其链接地址为 ``0x04000000``：

.. code-block:: bash

  $ qemu-system-arm -M xilinx-zynq-a9 -m 1G \
      -machine boot-mode=sd \
      -display none -serial null -serial mon:stdio \
      -device loader,file=output/images/u-boot.bin,addr=0x04000000,cpu-num=0 \
      -drive file=sdcard-qemu.img,if=sd,format=raw


也支持直接启动生成的 Buildroot Linux 镜像。Zynq ZC702 DTB 由 Buildroot 生成，位于 ``output/images/zynq-zc702.dtb``。内核镜像以 uImage 格式生成：

.. code-block:: bash

  $ qemu-system-arm -M xilinx-zynq-a9 -m 1G \
      -display none -serial null -serial mon:stdio \
      -kernel output/images/uImage \
      -dtb output/images/zynq-zc702.dtb \
      -append "console=ttyPS0,115200 root=/dev/mmcblk0p2 rw rootwait" \
      -drive file=sdcard-qemu.img,if=sd,format=raw


要配置启动模式，在命令行中提供：

.. code-block:: bash

   -machine boot-mode=qspi


支持的值为 ``jtag``、``sd``、``qspi`` 和 ``nor``。
