Microchip PolarFire SoC Icicle Kit（``microchip-icicle-kit``）
==============================================================

Microchip PolarFire SoC Icicle Kit 集成一个 PolarFire SoC，具有一个 SiFive E51 核心、四个 U54 核心、大量片上外设以及一个 FPGA。

关于 Microchip PolarFire SoC 的更多详情，请参见：
https://www.microchip.com/en-us/products/fpgas-and-plds/system-on-chip-fpgas/polarfire-soc-fpgas

Icicle Kit 板的信息可在此处找到：
https://www.microchip.com/en-us/development-tool/mpfs-icicle-kit-es

支持的设备
-----------------

``microchip-icicle-kit`` 机器支持以下设备：

* 一个 E51 核心
* 四个 U54 核心
* 核心级中断器（CLINT）
* 平台级中断控制器（PLIC）
* 松耦合集成内存（L2-LIM）
* L2 缓存控制器（L2CC）
* DDR 内存控制器
* 系统控制器和系统服务邮箱
* 五个 MMUART
* 一个 DMA 控制器
* 两个 GEM 以太网控制器
* 一个 SDHC 存储控制器
* 一个实时时钟

机器具有固定的 2 GiB RAM，拒绝其他内存大小。机器始终提供全部五个 hart，拒绝其他 CPU 数量。

启动选项
------------

``microchip-icicle-kit`` 机器提供一些运行固件（BIOS）或内核镜像的选项。QEMU 按下表选择固件：

============= ======= ======================================
-bios         -kernel 固件
============= ======= ======================================
none          N       错误
none          Y       内核镜像
NULL, default N       hss.bin
NULL, default Y       opensbi-riscv64-generic-fw_dynamic.bin
other         任意    BIOS 镜像
============= ======= ======================================

直接内核启动
------------------

使用 ``-kernel`` 选项直接运行内核镜像。请求直接内核启动时，可以通过 ``-dtb`` 选项指定设备树二进制文件。与其他 QEMU 机器不同，此机器不会为内核生成设备树，必须由用户提供。用户提供的 DTB 应满足以下要求：

* ``/cpus`` 节点应至少包含一个 E51 子节点，且子节点数量应与 QEMU 的 ``-smp`` 选项匹配。

* ``/memory`` 的 reg 大小应与 QEMU 通过 ``-m`` 选项选择的 RAM 大小匹配。

* 应包含一个 compatible 字符串为 "riscv,clint0" 或 "sifive,clint0" 的 CLINT 设备节点。其 ``interrupts-extended`` 属性必须描述每个 hart（包括 E51 hart 0）的 M 模式软件中断（3）和定时器中断（7）。

例如，五 hart Icicle Kit 的 CLINT 节点可以描述为：

.. code-block:: none

    clint: clint@2000000 {
        compatible = "sifive,fu540-c000-clint", "sifive,clint0";
        reg = <0x0 0x2000000 0x0 0xc000>;
        interrupts-extended = <&cpu0_intc 3>, <&cpu0_intc 7>,
                              <&cpu1_intc 3>, <&cpu1_intc 7>,
                              <&cpu2_intc 3>, <&cpu2_intc 7>,
                              <&cpu3_intc 3>, <&cpu3_intc 7>,
                              <&cpu4_intc 3>, <&cpu4_intc 7>;
    };


如果同一个 DTB 用于初始化通用 OpenSBI 平台，不要将 hart 0 的中断号替换为 ``0xffffffff``。OpenSBI 根据这些中断元组推导第一个 hart 和 CLINT 寄存器槽数量；屏蔽 hart 0 会使逐 hart 的 MSIP 和 MTIMECMP 寄存器映射偏移。

未指定 ``-bios`` 或将其设为 ``default`` 时，会使用 ``generic`` 平台的 OpenSBI ``fw_dynamic`` BIOS 镜像，直接启动 U-Boot 或操作系统内核等 S 模式载荷。

例如，以下命令展示如何从 U-Boot 主线 v2021.07 构建适用于 Microchip Icicle Kit 板的 U-Boot 镜像：

.. code-block:: bash

  $ export CROSS_COMPILE=riscv64-linux-
  $ make microchip_mpfs_icicle_defconfig


随后可以通过以下命令启动机器：

.. code-block:: bash

  $ qemu-system-riscv64 -M microchip-icicle-kit -smp 5 -m 2G \
      -sd path/to/sdcard.img \
      -nic user,model=cadence_gem \
      -nic tap,ifname=tap,model=cadence_gem,script=no \
      -display none -serial stdio \
      -kernel path/to/u-boot/build/dir/u-boot.bin \
      -dtb path/to/u-boot/build/dir/u-boot.dtb


注意事项：

* 检查 DTB 中 /chosen 节点的 "stdout-path" 属性，确定串口控制台使用哪个串口。例如，如果控制台设为第二个串口，请改用 "-serial null -serial stdio"。
* 默认 U-Boot 配置使用 CONFIG_OF_SEPARATE，因此 ELF 镜像 ``u-boot`` 不包含 DTB，不能传给 "-kernel"；必须使用包含 DTB 的 ``u-boot.bin``。要使用 ELF 镜像，需要改为 CONFIG_OF_EMBED 或 CONFIG_OF_PRIOR_STAGE。

运行 HSS
-----------

``microchip-icicle-kit`` 机器可以启动 Hart 软件服务（HSS_），随后从 SD 卡加载包含 U-Boot 的 HSS 载荷。以下流程已使用 HSS v2024.06 和 Buildroot 2026.05 测试。

使用 ``mpfs-icicle-kit-es`` 板的默认配置来配置 HSS。QEMU 提供 HSS v2024.06 DDR 初始化和训练流程所使用的软件可见寄存器以及确定性状态；它不模拟 DDR 训练的电气特性。HSS 要求 Microchip SoftConsole 提供的 RISC-V 裸机工具链位于 ``PATH`` 中。构建经过测试的 HSS 版本：

.. code-block:: bash

  $ git clone https://github.com/polarfire-soc/hart-software-services.git
  $ cd hart-software-services
  $ git checkout v2024.06
  $ make BOARD=mpfs-icicle-kit-es defconfig
  $ make -j$(nproc) BOARD=mpfs-icicle-kit-es


HSS 构建同时生成原始封装文件和 eNVM 编程镜像。QEMU 需要完整 eNVM 镜像，包括 Microchip 启动模式编程器添加的 256 字节启动头。将生成的 Intel HEX 文件转换为原始二进制镜像，例如：

.. code-block:: bash

  $ riscv64-unknown-elf-objcopy -I ihex -O binary \
      build/hss-envm-wrapper.mpfs-icicle-kit-es.hex build/hss.bin


不要将 ``build/hss-envm-wrapper.bin`` 直接传给 QEMU。该文件从 eNVM 偏移 0x100 开始，不包含带镜像大小和逐 hart 复位向量的启动头。

使用经过测试的 Buildroot 版本构建 SD 卡镜像：

.. code-block:: bash

  $ git clone https://gitlab.com/buildroot.org/buildroot.git
  $ cd buildroot
  $ git checkout 2026.05
  $ make microchip_mpfs_icicle_defconfig
  $ make


这会生成具有三个 GPT 分区的 ``output/images/sdcard.img``：

* 包含 U-Boot 的 HSS ``payload.bin``。
* 包含 ``boot.scr`` 和内核 FIT 镜像的 FAT 分区。
* ext4 Linux 根文件系统。

QEMU 的 SD 卡模型要求镜像大小为二的幂。制作一个稀疏的 4 GiB 工作副本，并将备份 GPT 移至镜像的新末尾：

.. code-block:: bash

  $ cp --reflink=auto output/images/sdcard.img sdcard.img
  $ truncate -s 4G sdcard.img
  $ sgdisk -e sdcard.img
  $ sgdisk -v sdcard.img


FIT 镜像中的 Icicle Kit 固件设备树描述了 2 GiB RAM，与机器的固定 RAM 大小一致。以下命令保留显式的 ``-m 2G``，将镜像作为 SD 卡连接，并连接板上的两个串口：

.. code-block:: bash

  $ qemu-system-riscv64 \
      -M microchip-icicle-kit -smp 5 -m 2G \
      -bios path/to/hss/build/hss.bin \
      -drive if=sd,file=path/to/sdcard.img,format=raw \
      -display none \
      -serial file:hss.log \
      -serial stdio \
      -no-reboot


HSS 向 MMUART0 写入，命令将其记录在 ``hss.log`` 中。U-Boot 和 Linux 使用 MMUART1，它保持连接到终端。成功启动会依次经过 HSS 载荷加载、U-Boot、Linux 内核，最终到达第三个分区根文件系统的登录提示符。

已知限制
-----------------

* 经过测试的 HSS v2024.06 流程包含两个独立的多 hart 启动竞争问题，截至最新 v2026.04 发布版仍未修复。因此，启动可能在最开始时停滞，也可能在 DDR 训练成功后的 OpenSBI/U-Boot 交接期间停滞。
* SD 卡模型要求原始镜像大小为二的幂。调整镜像大小时，请保持备份 GPT 头位于末尾。
* 机器不生成 Icicle Kit 设备树。固件启动必须在载荷或 FIT 镜像中提供设备树；直接内核启动必须按上文所述使用 ``-dtb``。

.. _HSS: https://github.com/polarfire-soc/hart-software-services
