SiFive HiFive Unleashed（``sifive_u``）
=======================================

SiFive HiFive Unleashed 开发板是一款采用 Freedom U540 多核 RISC-V 处理器的高端 RISC-V 开发板。

支持的设备
-----------------

``sifive_u`` 机器支持以下设备：

* 一个 E51 / E31 核心
* 最多四个 U54 / U34 核心
* 核心本地中断器（CLINT）
* 平台级中断控制器（PLIC）
* 电源、复位、时钟和中断（PRCI）
* L2 松耦合集成内存（L2-LIM）
* DDR 内存控制器
* 两个 UART
* 一个 GEM 以太网控制器
* 一个 GPIO 控制器
* 一个存有序列号的一次性可编程（OTP）存储器
* 一个 DMA 控制器
* 两个 QSPI 控制器
* 一个 ISSI 25WP256 闪存
* 一张工作在 SPI 模式的 SD 卡
* PWM0 和 PWM1

请注意，真实 HiFive Unleashed 板固定采用一个 E51 核心和四个 U54 核心的组合，RISC-V 核心以 64 位模式启动。在 QEMU 中，可以创建具有一个 E51 核心和最多四个 U54 核心的机器。也可以创建具有相同外设的 32 位变体，仅将 RISC-V 核心替换为 32 位核心（E31 和 U34），以帮助测试 32 位客户机软件。

硬件配置信息
----------------------------------

如果未指定 ``-dtb`` 选项，``sifive_u`` 机器会自动生成设备树二进制文件（“dtb”）并传给客户机。其中提供系统中各种设备的地址、中断线和其他配置信息。客户机软件应从生成的 DTB 中发现设备，而不是使用真实硬件的 DTB，因为部分设备未被 QEMU 建模，尝试访问它们可能导致意外行为。

如果用户希望提供自己的 DTB，可以使用 ``-dtb`` 选项。这些 DTB 应满足以下要求：

* /cpus 节点应至少包含一个 E51 子节点，且子节点数量应与 QEMU 的 ``-smp`` 选项匹配
* /memory 的 reg 大小应与 QEMU 通过 ``-m`` 选择的 ram_size 匹配
* 与 OpenSBI BIOS 镜像配合使用时，应包含一个 compatible 字符串为 "riscv,clint0" 的 CLINT 设备节点

启动选项
------------

``sifive_u`` 机器可使用标准 -kernel 功能加载 Linux 内核、VxWorks 内核、修改后的 U-Boot 引导加载程序（S 模式）或 ELF 可执行文件，并以默认 OpenSBI 固件镜像作为 -bios。它也支持通过标准 -bios 功能启动未经修改的 U-Boot 引导加载程序。

机器专用选项
------------------------

支持以下机器专用选项：

- serial=nnn

  板的序列号。未给出时，使用默认序列号 1。

  SiFive 将 16 KiB OTP 内存的前 1 KiB 保留用于内部用途。目前仅在偏移 0xfc 处存储板的序列号。U-Boot 从 OTP 内存读取序列号，并用它生成唯一 MAC 地址，写入片上 GEM 以太网控制器。如果创建多个 QEMU ``sifive_u`` 机器并连接到同一子网，它们都会具有相同的 MAC 地址，导致网络无法使用。此时，用户应在创建不同 ``sifive_u`` 机器时为 serial= 指定不同值。

- start-in-flash

  给出此选项时，QEMU 的 ROM 代码直接跳转到 QSPI 内存映射闪存。否则，QEMU 根据 msel= 的值跳转到 DRAM 或 L2LIM。未给出时，默认直接从 DRAM 启动。

- msel=[6|11]

  模式选择（MSEL[3:0]）引脚值，用于控制启动来源。

  FU540 SoC 支持从多个来源启动，由芯片上的模式选择引脚控制。通常，启动流程在执行用户提供的程序前会经过若干阶段，一般包括：

  1. 第零阶段引导加载程序（ZSBL），位于片上掩膜 ROM，由 QEMU 提供。注意，QEMU 实现的 ROM 代码与真实硬件中写入的代码不同。QEMU 的版本经过简化，但提供与硬件相同的功能。
  2. 第一阶段引导加载程序（FSBL），启动 PLL 和 DDR 内存，即 U-Boot SPL。
  3. 第二阶段引导加载程序（SSBL），按需进一步初始化其他外设，即与 OpenSBI fw_dynamic 固件镜像组合的 U-Boot proper。

  msel=6 表示 FSBL 和 SSBL 都位于 QSPI 闪存；msel=11 表示它们都位于 SD 卡。

运行 Linux 内核
--------------------

撰写本文时测试了 Linux 主线 v5.10 发布版。要构建可由 ``sifive_u`` 机器以 64 位模式启动的 Linux 主线内核，只需使用 defconfig 配置内核：

.. code-block:: bash

  $ export ARCH=riscv
  $ export CROSS_COMPILE=riscv64-linux-
  $ make defconfig
  $ make


在 QEMU 中使用 ``sifive_u`` 机器启动新构建的 Linux 内核：

.. code-block:: bash

  $ qemu-system-riscv64 -M sifive_u -smp 5 -m 2G \
      -display none -serial stdio \
      -kernel arch/riscv/boot/Image \
      -initrd /path/to/rootfs.ext4 \
      -append "root=/dev/ram"


也可以使用自定义 DTB 启动机器，在 Linux 内核的 fu540-c000.dtsi 中插入一个 CLINT 节点，

.. code-block:: none

    clint: clint@2000000 {
        compatible = "riscv,clint0";
        interrupts-extended = <&cpu0_intc 3 &cpu0_intc 7
                               &cpu1_intc 3 &cpu1_intc 7
                               &cpu2_intc 3 &cpu2_intc 7
                               &cpu3_intc 3 &cpu3_intc 7
                               &cpu4_intc 3 &cpu4_intc 7>;
        reg = <0x00 0x2000000 0x00 0x10000>;
    };


并使用以下命令行选项：

.. code-block:: bash

  $ qemu-system-riscv64 -M sifive_u -smp 5 -m 8G \
      -display none -serial stdio \
      -kernel arch/riscv/boot/Image \
      -dtb arch/riscv/boot/dts/sifive/hifive-unleashed-a00.dtb \
      -initrd /path/to/rootfs.ext4 \
      -append "root=/dev/ram"


要构建可由 ``sifive_u`` 机器以 32 位模式启动的 Linux 主线内核，请使用 rv32_defconfig 配置。Linux v5.10 内核需要一个补丁来修复 32 位启动问题。

.. code-block:: bash

  $ export ARCH=riscv
  $ export CROSS_COMPILE=riscv64-linux-
  $ curl https://patchwork.kernel.org/project/linux-riscv/patch/20201219001356.2887782-1-atish.patra@wdc.com/mbox/ > riscv.patch
  $ git am riscv.patch
  $ make rv32_defconfig
  $ make


将上述命令行中的 ``qemu-system-riscv64`` 替换为 ``qemu-system-riscv32``，即可启动 32 位 Linux 内核。必须使用包含 32 位应用的根文件系统镜像，内核才能启动到用户空间。

运行 VxWorks 内核
----------------------

撰写本文时测试了 VxWorks 7 SR0650 发布版。要构建可由 ``sifive_u`` 机器启动的 64 位 VxWorks 主线内核，只需按照 BSP 文档说明，基于 sifive_generic BSP 创建 VxWorks 源代码构建项目，再创建 VxWorks 镜像项目以生成可启动的 VxWorks 镜像。

VxWorks SDK 还提供适用于 HiFive Unleashed 板的预构建 64 位 VxWorks 7 镜像，可用于测试。SDK 下载说明如下：

.. code-block:: bash

  $ wget https://labs.windriver.com/downloads/wrsdk-vxworks7-sifive-hifive-1.01.tar.bz2
  $ tar xvf wrsdk-vxworks7-sifive-hifive-1.01.tar.bz2
  $ ls bsps/sifive_generic_1_0_0_0/uboot/uVxWorks


在 QEMU 中使用 ``sifive_u`` 机器启动 VxWorks 内核：

.. code-block:: bash

  $ qemu-system-riscv64 -M sifive_u -smp 5 -m 2G \
      -display none -serial stdio \
      -nic tap,ifname=tap0,script=no,downscript=no \
      -kernel /path/to/vxWorks \
      -append "gem(0,0)host:vxWorks h=192.168.200.1 e=192.168.200.2:ffffff00 u=target pw=vxTarget f=0x01"


也可以在 ``sifive_u`` 机器上测试 32 位 VxWorks。创建 32 位项目构建 32 位 VxWorks 镜像，再使用 ``qemu-system-riscv32`` 和完全相同的命令行选项。

运行 U-Boot
--------------

撰写本文时测试了 U-Boot 主线 v2024.01 发布版。要构建可由 ``sifive_u`` 机器启动的 U-Boot 主线引导加载程序，请使用 sifive_unleashed_defconfig，命令与上述 Linux 构建命令类似：

.. code-block:: bash

  $ export CROSS_COMPILE=riscv64-linux-
  $ export OPENSBI=/path/to/opensbi-riscv64-generic-fw_dynamic.bin
  $ make sifive_unleashed_defconfig


将在构建树中得到 spl/u-boot-spl.bin 和 u-boot.itb 文件。

要使用 ``sifive_u`` 机器启动 U-Boot，请准备已正确分区并填入正确内容的 SPI 闪存镜像或 SD 卡镜像。可使用 genimage_ 生成这些镜像。

128 MiB SD 卡镜像的配置文件示例：

.. code-block:: bash

  $ cat genimage_sdcard.cfg
  image sdcard.img {
          size = 128M

          hdimage {
                  gpt = true
          }

          partition u-boot-spl {
                  image = "u-boot-spl.bin"
                  offset = 17K
                  partition-type-uuid = 5B193300-FC78-40CD-8002-E86C45580B47
          }

          partition u-boot {
                  image = "u-boot.itb"
                  offset = 1041K
                  partition-type-uuid = 2E54B353-1271-4842-806F-E436D6AF6985
          }
  }


SPI 闪存镜像的分区偏移略有不同，且大小必须为 32 MiB，以匹配真实板上的 ISSI 25WP256 闪存：

.. code-block:: bash

  $ cat genimage_spi-nor.cfg
  image spi-nor.img {
          size = 32M

          hdimage {
                  gpt = true
          }

          partition u-boot-spl {
                  image = "u-boot-spl.bin"
                  offset = 20K
                  partition-type-uuid = 5B193300-FC78-40CD-8002-E86C45580B47
          }

          partition u-boot {
                  image = "u-boot.itb"
                  offset = 1044K
                  partition-type-uuid = 2E54B353-1271-4842-806F-E436D6AF6985
          }
  }


假设 U-Boot 二进制文件与配置文件位于同一目录，可用以下命令生成镜像：

.. code-block:: bash

  $ genimage --config genimage_<boot_src>.cfg --inputpath .


指定 msel=11，并将 SD 卡镜像传给 QEMU ``sifive_u`` 机器，即可从 SD 卡启动 U-Boot：

.. code-block:: bash

  $ qemu-system-riscv64 -M sifive_u,msel=11 -smp 5 -m 8G \
      -display none -serial stdio \
      -bios /path/to/u-boot-spl.bin \
      -drive file=/path/to/sdcard.img,if=sd


将 msel= 的值改为 6，可从 SPI 闪存启动 U-Boot：

.. code-block:: bash

  $ qemu-system-riscv64 -M sifive_u,msel=6 -smp 5 -m 8G \
      -display none -serial stdio \
      -bios /path/to/u-boot-spl.bin \
      -drive file=/path/to/spi-nor.img,if=mtd


注意，测试 U-Boot 时不使用 QEMU 自动生成的设备树二进制文件，因为 U-Boot 自身嵌入了用于 U-Boot SPL 和 U-Boot proper 的设备树。因此，核心数和内存大小必须与真实硬件一致，即 5 个核心（-smp 5）和 8 GiB 内存（-m 8G）。

上述用法可在 QEMU ``sifive_u`` 机器上直接运行适用于 SiFive HiFive Unleashed 板的上游 U-Boot，使用户能够通过真实场景开发和测试推荐的 RISC-V 启动流程：ZSBL（位于 QEMU 中）将 U-Boot SPL 从 SD 卡或 SPI 闪存加载到 L2LIM，随后 U-Boot SPL 加载 OpenSBI fw_dynamic 固件与 U-Boot proper 的组合载荷镜像。

不过，有时只想快速测试在 QEMU 上启动 U-Boot，而不准备 SPI 闪存或 SD 卡镜像。此时可采用另一种方式，修改 U-Boot 配置以创建 S 模式 U-Boot 镜像：

.. code-block:: bash

  $ export CROSS_COMPILE=riscv64-linux-
  $ make sifive_unleashed_defconfig
  $ ./scripts/config --enable OF_BOARD
  $ ./scripts/config --disable BINMAN_FDT
  $ ./scripts/config --disable SPL
  $ make olddefconfig


这使 U-Boot 使用 QEMU 生成的设备树，并跳过 U-Boot SPL 阶段。

直接启动 64 位 U-Boot S 模式镜像：

.. code-block:: bash

  $ qemu-system-riscv64 -M sifive_u -smp 5 -m 2G \
      -display none -serial stdio \
      -kernel /path/to/u-boot.bin


也可以创建 32 位 U-Boot S 模式镜像。

.. code-block:: bash

  $ export CROSS_COMPILE=riscv64-linux-
  $ make sifive_unleashed_defconfig
  $ ./scripts/config --disable ARCH_RV64I
  $ ./scripts/config --enable ARCH_RV32I
  $ ./scripts/config --set-val TEXT_BASE 0x80400000
  $ ./scripts/config --enable OF_BOARD
  $ ./scripts/config --disable BINMAN_FDT
  $ ./scripts/config --disable SPL
  $ make olddefconfig


使用相同的命令行选项启动 32 位 U-Boot S 模式镜像：

.. code-block:: bash

  $ qemu-system-riscv32 -M sifive_u -smp 5 -m 2G \
      -display none -serial stdio \
      -kernel /path/to/u-boot.bin


.. _genimage: https://github.com/pengutronix/genimage
