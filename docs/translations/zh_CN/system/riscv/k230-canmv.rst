Canaan CanMV-K230 板（``k230-canmv``）
========================================

``k230-canmv`` 机器模拟 Canaan CanMV-K230 板，兼容 Kendryte K230 SDK 的 ``k230_canmv_defconfig`` 构建产物。

K230 是 Canaan Inc. 旗下 Kendryte ® 生产的 AIoT SoC 系列芯片，采用全新的多异构单元加速计算结构。

该芯片包含两个 RISC-V 计算核心和新一代 KPU（知识处理单元）智能计算单元。

更多信息请参见 <https://www.kendryte.com/en/proDetail/230>。

支持的设备
-----------------

``k230-canmv`` 机器支持以下设备：

* 一个 T-Head C908 核心，以及可选的 C908V 大核
* 核心本地中断器（CLINT）
* 平台级中断控制器（PLIC）
* 两个 K230 看门狗定时器
* 五个 UART
* 用于 SDK U-Boot gzip 镜像解压的 GSDMA 和 UGZIP 模块
* PDMA 寄存器块
* 两个兼容 SDHCI 的 K230 DWC MSHC 控制器

启动选项
------------

``k230-canmv`` 机器通过 M 模式 U-Boot 启动 K230 SDK。U-Boot 从连接的 SD 卡镜像读取 Linux 载荷，再按正常 SDK 启动流程启动 OpenSBI/Linux。

QEMU 模拟了 CanMV 可移除卡槽使用的 K230 SDHCI 存储控制器，因此常规 Linux 启动路径不需要手动设置 RAM 加载器。

运行
-------

U-Boot SD 卡启动
~~~~~~~~~~~~~~~~~~~

使用 SDK U-Boot 二进制文件作为机器固件，并通过 ``-drive if=sd`` 连接 K230 SDK SD 卡镜像。第一个 SD 驱动器连接到 SDK CanMV 可移除卡槽 ``sdhci1@91581000``：

.. code-block:: bash

   $ SDK=k230_sdk/output/k230_canmv_defconfig
   $ SDIMG=/path/to/k230-sdk-sdcard.img
   $ qemu-system-riscv64 -machine k230-canmv \
      -bios "$SDK/little/uboot/u-boot" \
      -drive if=sd,file="$SDIMG",format=raw \
      -nographic


对于同时启动 C908V 大核的 SDK 镜像，启用两个虚拟 CPU：

.. code-block:: bash

   $ qemu-system-riscv64 -machine k230-canmv,boot-both-cores=on -smp 2 \
      -bios "$SDK/little/uboot/u-boot" \
      -drive if=sd,file="$SDIMG",format=raw \
      -nographic


可以通过额外的 ``-serial`` 选项连接更多 UART。UART0 用于 U-Boot 和 Linux 控制台，C908V RT-Smart 控制台使用 UART3：

.. code-block:: bash

   $ qemu-system-riscv64 -machine k230-canmv,boot-both-cores=on -smp 2 \
      -bios "$SDK/little/uboot/u-boot" \
      -drive if=sd,file="$SDIMG",format=raw \
      -serial mon:stdio -serial null -serial null -serial pty -serial null \
      -display none
