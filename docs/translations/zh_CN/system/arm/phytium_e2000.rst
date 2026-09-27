.. SPDX-License-Identifier: GPL-2.0-or-later

飞腾 E2000Q 机器（``phytium-pi``、``phytium-e2000-come``）
================================================================

概述
--------

QEMU 模拟两款基于异构飞腾 E2000Q SoC 的板：

.. list-table:: E2000Q 机器变体
   :header-rows: 1
   :widths: 20 20 20 20 20

   * - 机器
     - 固件介质
     - Linux 存储
     - SDK DTB
     - 直接启动内核
   * - ``phytium-pi``
     - SD0
     - SD0
     - ``phytiumpi_firefly.dtb``
     - ``Image.gz``
   * - ``phytium-e2000-come``
     - QSPI0
     - AHCI1 SATA
     - ``e2000q-come-board.dtb``
     - ``Image``

每种板级机器都嵌入一个 ``phytium-e2000-soc`` QOM 子对象。SoC 拥有 CPU、中断控制器和集成外设控制器。机器拥有 RAM 和启动配置，并将板级 SD 卡、QSPI 闪存和 SATA 磁盘连接到 SoC 提供的总线。

两种机器都面向飞腾 Buildroot SDK 生成的 Linux 和固件镜像。飞腾派没有板级 SATA 启动磁盘。COMe 板在 QSPI0 上连接 GD25Q128 SPI NOR 闪存，在 AHCI1 上连接 SATA 磁盘。

飞腾固件栈在概念上分为三层：片上飞腾启动 ROM（PBR）建立信任根并准备早期交接；处理器基础固件（PBF）初始化处理器；系统固件（SFW）提供后续固件服务和引导加载程序。QEMU 不执行内部 ROM，而是重建 PBR 交接状态，从板所选固件介质的字节零处读取 ``fip-all.bin`` 镜像，在 EL3 启动 PBF 载荷，再由 PBF 经 SFW 各阶段交接到同一镜像中的 U-Boot。

SFW 流程加载 BL31，其常驻 EL3 运行时服务处理 U-Boot 和 Linux 发出的飞腾私有 SMC。在固件启动路径中，QEMU 不拦截这些调用。

启动选择由固定板级接线决定。飞腾派通过 PBR 交接报告 SD0，COMe 则报告 QSPI0。固件可能向所选后端保存环境或 DDR 训练数据，因此应使用可写镜像或 ``-snapshot``。

机器不生成设备树。使用 ``-kernel`` 直接启动 Linux 时，必须通过 ``-dtb`` 提供上表所列对应 SDK DTB。稍后由固件启动 Linux 时，同样需要在客户机可见的启动环境中提供匹配的板级 DTB。

PBR 设备的职责
--------------------

固件启动由 ``phytium-e2000-pbr`` 设备实现。板级机器配置其 E2000 SoC 子对象时，选择启动介质和后端。SoC 创建 QSPI 和 MCI 控制器，映射 PBR 状态窗口、启动 SRAM 和 IACC，并将 PBR 连接到 CPU 拓扑。机器和 SoC 本身都不解析或暂存固件。``boot-mode`` 属于 PBR 设备，不作为用户可设置的机器属性提供。

PBR 设备仅读取选定的板级介质，验证外层镜像范围、嵌入的 TF-A FIP、主 MPIDR 和平台参数记录。随后，将计算出的完整镜像范围放置在 IACC ``0x38000000``，将 PBF/BL1 重定位到 ``0xf8c40000``，构造启动 SRAM 交接数据，并释放镜像指定的主 CPU。

公开 PBF 接口为 PLL、PCIe、DDR 和 COMMON 服务参数定义了通用 16 字节头，包含魔数、版本、大小和保留字。E2000 固件配置规定了其私有容器和交接位置：

* PLL：FIP 偏移 ``0xf5000`` 至 SCP SRAM ``0x32a10c00``，魔数 ``0x54460020``
* DDR/MCU（内存控制器单元）：FIP 偏移 ``0xf5300`` 至 SCP SRAM ``0x32a10d00``，魔数 ``0x54460024``
* PCIe：FIP 偏移 ``0xf5100`` 至 SCP SRAM ``0x32a10e00``，魔数 ``0x54460021``
* COMMON：FIP 偏移 ``0xf5200`` 至 SCP SRAM ``0x32a10f00``，魔数 ``0x54460013``

在此 PBF 术语中，MCU 表示 Memory Controller Unit（内存控制器单元）。该记录提供 DDR 控制器配置、DIMM/SPD 数据和训练参数。

PBR 设备验证每个精确的 E2000 魔数、声明大小、源范围，以及 ``0x100`` 字节目标槽位的边界。它复制声明的记录，不解释服务专用载荷，并将目标槽位剩余部分清零。参数版本属于各自 PBF 服务，不用于选择 PBR 交接或 TF-A 对象布局。特别是，QEMU 不生成 DDR/SPD 数据，不改写 DDR/MCU 记录版本，也不强制设置 DDR 训练控制。

PBF 内嵌 TF-A v2.3 FIP 和 memmap I/O 驱动，以及平台 I/O 策略表。PBR 设备根据它们之间的 TF-A 数据结构关系定位对象，构造注册和 ``dev_open`` 本应生成的启动 SRAM 状态。这允许不同 PBF 构建独立于记录版本重定位各自 I/O 对象，包括两个携带第 4 版 DDR/MCU 参数记录的构建。

其余部分交接细节属于厂商私有 PBR/PBF 约定，而非已公布的 TF-A 或硬件 ABI。特别是，固定启动 SRAM 参数图和 FIP 驱动运行时状态的位置，是根据 2 GiB、4 GiB 飞腾派 SDK 固件样本以及 COMe SDK 固件样本中的早期 PBF 访问和回调反汇编重建的。实现在相关代码旁记录这些证据边界，并拒绝外围 TF-A 结构不匹配的固件。

设备还负责此状态的复位、迁移和清理。状态寄存器以及固件、主 CPU 状态具有显式 VMState；设备拥有的启动 SRAM 和 IACC RAM 区域则作为 RAMBlock 迁移。复位会重新装载不可变输入镜像，恢复交接状态和主入口。这是 ROM 约定的行为模型，不是片上 PBR 的 Arm 指令级实现。

CPU 拓扑
------------

E2000Q 为异构架构。QEMU 模拟 SDK Linux 设备树描述的三个非均匀 CPU 集群：

* 集群 0 包含一个 FTC664 核心，MPIDR 亲和性为 ``0x0``
* 集群 1 包含一个 FTC664 核心，MPIDR 亲和性为 ``0x100``
* 集群 2 包含两个 FTC310 核心，MPIDR 亲和性为 ``0x200`` 和 ``0x201``

对应的 QEMU CPU 槽位顺序为 ``0x0``、``0x100``、``0x200``、``0x201``。因此，选择主亲和性 ``0x200`` 的固件镜像会释放 QEMU CPU 索引 2。

全部四个槽位都实例化由 SoC 拥有的 Cortex-A72 TCG CPU 对象。在实例化每个对象前，SoC 应用其 FTC310 或 FTC664 标识，以及固件使用的 E2000 系统寄存器。因此，``-cpu`` 选项只接受 Cortex-A72 执行基础，不能替换异构槽位分配。

飞腾 MIDR 值、架构指令功能字段、AArch32 浮点功能字段以及 VIPT/PIPT 指令缓存策略，与物理 E2000Q 板上观察到的值一致。这是架构兼容性模型，不是性能或缓存容量模型。

支持的设备
-----------------

两种机器目前均支持：

* 三个集群中的两个 FTC664 和两个 FTC310 AArch64 CPU 槽位
* 从 ``0x80000000`` 开始的 RAM，默认 2 GiB，最大 8 GiB
* 厂商固件栈使用的、由 PBR 拥有的启动 SRAM 和 IACC RAM
* 带 ITS 的 GICv3 中断控制器
* 七个 PL011 UART
* 兼容 DesignWare 的 I2C 控制器
* E2000 硬件随机数生成器
* 两个 USB 3.0 xHCI 主机控制器
* 两个单端口 sysbus AHCI 控制器
* 两个 SD/MMC 控制器
* GPEX PCIe 主机桥，通过 ITS 支持 MSI，通过 Arm SMMUv3 提供第一级 DMA 地址转换；网络设备等 PCIe 端点必须通过 ``-device`` 显式添加
* 带直接映射读取窗口的 QSPI0 控制器
* 厂商固件所需的 PBR、DDR 和 MHU/SCMI 兼容行为，以及 Linux 使用的 SCMI Base 协议

板级连接区别如下：

* 飞腾派使用 SD0 存放固件和根文件系统，没有连接 QSPI 闪存或 SATA 启动磁盘
* COMe 使用 QSPI0 上的 GD25Q128 存放固件，使用 AHCI1 SATA 磁盘运行 Linux

固件启动
-------------

不支持通过 ``-bios`` 或 pflash 驱动器提供固件。请按下文使用板上固定的 SD0 或 QSPI0 介质。

飞腾派 SD 启动
~~~~~~~~~~~~~~~~~~

从飞腾维护的 Buildroot 发布版构建完整 SD 镜像。无需单独执行 ``make phytium_defconfig``，因为 ``merge_config.sh`` 会直接使用基础 defconfig：

.. code-block:: shell

   $ git clone https://gitee.com/phytium_embedded/phytium-linux-buildroot.git
   $ cd phytium-linux-buildroot
   $ git checkout phytium-linux-buildroot_v2.4

   $ ./support/kconfig/merge_config.sh \
       configs/phytium_defconfig \
       configs/phytiumpi_sdcard.config
   $ make


生成的完整镜像为 ``output/images/sdcard.img``。SD 卡配置片段选择厂商 4 GiB 固件，使用 Buildroot 内核和飞腾派 DTB 构建 ``fitImage``，再将它们与 ``rootfs.ext2`` 一起打包。应直接使用此镜像，不要手动组装各组件。

厂商生成的 SD 布局在根文件系统之外保留前 64 MiB：

.. list-table:: 飞腾派 SD 镜像布局
   :header-rows: 1
   :widths: 25 20 55

   * - 起始位置
     - 保留大小
     - 内容
   * - ``0x00000000``
     - 4 MiB
     - ``fip-all.bin`` 固件区域
   * - ``0x00400000``
     - 60 MiB
     - U-Boot FIT 镜像
   * - ``0x04000000``
     - 镜像剩余部分
     - 第一个分区，包含 ext4 根文件系统

默认飞腾派 SDK U-Boot 环境从 SD 块 ``0x2000`` （字节偏移 4 MiB）读取 FIT，并使用 ``root=/dev/mmcblk0p1`` 启动。因此，第一个分区必须从字节偏移 64 MiB 开始。DOS 分区表可以替换 FIP 的第零扇区，与厂商镜像配方一致；模型使用的 PBR 数据位于固件区域内较后的固定偏移。

SDK v2.4 镜像配方将根分区声明为 16 GiB，即使生成的 ``sdcard.img`` 更短。因此，Linux 会报告 ``mmcblk0p1`` 超出设备末尾，并截短所报告的分区大小。这一警告是预期行为，不会阻止其中的 ext4 文件系统挂载。

UART1 承载 U-Boot 和 Linux 控制台。以下命令无需注入 U-Boot 命令，即可启动完整原始 SD 镜像。``-snapshot`` 在固件写入环境或 DDR 训练数据时保持源镜像不变。

以下示例使用 Buildroot 生成的镜像::

  IMAGES=/path/to/buildroot/output/images


.. code-block:: shell

   $ qemu-system-aarch64 \
       -machine phytium-pi \
       -smp 4 -m 4G \
       -display none -monitor none \
       -serial file:pbr-uart0.log \
       -serial stdio \
       -nic user \
       -snapshot \
       -drive file="$IMAGES/sdcard.img",if=sd,index=0,format=raw


显式的 4 GiB RAM 大小与 ``phytiumpi_sdcard.config`` 选定的固件一致。采用第 4 版 DDR/MCU 参数记录的已验证流程会完成 PBF 和 DDR 初始化，进入 OP-TEE 和 U-Boot，从 SD 读取 FIT，在全部四个模拟 CPU 上启动 Linux，将 SD0 识别为 ``mmcblk0``，挂载分区 1，并在没有串口或监视器输入的情况下到达登录提示符。
SDK 根文件系统使用普通 ``getty``，不自动登录，因此进入 shell 需要交互式 UART1 字符设备。

COMe 从 QSPI 到 SATA 启动
~~~~~~~~~~~~~~~~~~~~~~~~~

如果具有匹配的 E2000Q ``fip-all.bin``，这是首选 COMe 启动流程。FIP 容器必须从原始 QSPI 镜像的字节零开始。

从飞腾维护的 Buildroot 发布版构建标准 SATA 磁盘镜像：

.. code-block:: shell

   $ git clone https://gitee.com/phytium_embedded/phytium-linux-buildroot.git
   $ cd phytium-linux-buildroot
   $ git checkout phytium-linux-buildroot_v2.4

   $ make phytium_defconfig
   $ make


生成的 ``$SDK/disk.img`` 具有 GPT 分区表。第一个分区是 400 MiB FAT 文件系统，包含 ``Image``、DTB 和 GRUB；第二个分区包含 ext4 根文件系统。直接将镜像连接到 AHCI1，无需在主机侧重新分区或复制文件。

创建处于已擦除状态的临时 16 MiB GD25Q128 镜像，并将 FIP 复制到字节零：

.. code-block:: shell

   $ export FIP=/path/to/sdk/fip-all.bin
   $ export QSPI=/path/to/e2000q-gd25q128.bin

   $ dd if=/dev/zero bs=1M count=16 | tr '\000' '\377' > "$QSPI"
   $ dd if="$FIP" of="$QSPI" conv=notrunc


UART1 承载交互式 U-Boot 控制台。UART0 写入独立日志，以保留早期固件输出。``-snapshot`` 在固件写入训练数据、环境或根文件系统时保持输入镜像不变：

.. code-block:: shell

   $ qemu-system-aarch64 \
       -machine phytium-e2000-come \
       -smp 4 -m 2G \
       -display none -monitor none \
       -serial file:pbr-uart0.log \
       -serial stdio \
       -nic user \
       -snapshot \
       -drive file="$QSPI",if=mtd,index=0,format=raw \
       -drive file="$IMAGES/disk.img",if=ide,index=0,format=raw


使用 SDK 固件时，已验证流程保留第 5 版 DDR/MCU 参数记录，识别 2 GiB DDR4/X16，完成软件训练、数据 BIST 和地址 BIST，检测到 GD25Q128，报告 ``boot media is qspi!``，并通过 AHCI1 将 SATA 磁盘枚举为 ``scsi 0``。

U-Boot 默认环境可能与 Buildroot ``disk.img`` 布局不匹配。在自动启动倒计时期间按任意键进入 ``E2000#`` 提示符，然后输入以下命令，从 FAT 分区加载原始 arm64 内核和 COMe DTB，在没有 initrd 的情况下启动：

.. code-block:: shell

   setenv bootargs 'console=ttyAMA1,115200 root=/dev/sda2 rootwait rw cma=256M'
   fatload scsi 0:1 0x90100000 Image
   fatload scsi 0:1 0x90000000 e2000q-come-board.dtb
   booti 0x90100000 - 0x90000000


已验证流程在全部四个模拟 CPU 上启动 Linux，挂载 ``sda2``，并在 UART1 上到达 Buildroot 登录提示符。

直接启动 Linux 的回退方式
--------------------------

只有当 Buildroot SDK 未提供匹配的 EDK2 或 U-Boot 源码，且没有可用的厂商 FIP 流程时，才使用 QEMU 的直接 ``-kernel`` 接口作为回退。此路径绕过 PBR、PBF、SFW 和 U-Boot，要求使用概述中列出的板级 SDK 内核和匹配 DTB；省略 ``-dtb`` 会报错。

不支持通过 ``-kernel`` 直接启动厂商 U-Boot 二进制文件，因为这会绕过 BL31，使引导加载程序缺少通常由常驻 EL3 固件提供的飞腾私有 SMC 服务。

以下命令使用 SDK 内核和 initramfs 启动，无需磁盘镜像。

飞腾派直接启动
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: shell

   $ qemu-system-aarch64 \
       -machine phytium-pi \
       -smp 4 \
       -display none -monitor none \
       -serial null \
       -serial stdio \
       -nic user \
       -kernel "$IMAGES/Image.gz" \
       -dtb "$IMAGES/phytiumpi_firefly.dtb" \
       -initrd "$IMAGES/rootfs.cpio.gz" \
       -append 'console=ttyAMA1,115200 earlycon=pl011,mmio32,0x2800d000 rdinit=/init'


COMe 直接启动
~~~~~~~~~~~~~~~~

.. code-block:: shell

   $ qemu-system-aarch64 \
       -machine phytium-e2000-come \
       -smp 4 \
       -display none -monitor none \
       -serial null \
       -serial stdio \
       -nic user \
       -kernel "$IMAGES/Image" \
       -dtb "$IMAGES/e2000q-come-board.dtb" \
       -initrd "$IMAGES/rootfs.cpio.gz" \
       -append 'console=ttyAMA1,115200 earlycon=pl011,mmio32,0x2800d000 rdinit=/init'


QEMU 对提供的 SDK DTB 应用普通 Arm 直接启动修正，但不会合成替代硬件描述。

已知限制
-----------------

这些机器是针对已测试固件和直接 Linux 启动路径的功能模型，不是物理开发板的完整模型。

* QEMU 模拟已测试 ``fip-all.bin`` 所需的 PBR 交接行为，不模拟 ROM 指令执行、身份验证或完整片上信任根。
* 启动 SRAM 对象图和 SFW 回调地址仅适用于已检查的固件样本；这些样本携带第 4 或第 5 版 DDR/MCU 参数记录。
* 不模拟 FTC310 和 FTC664 的缓存容量及微架构性能。所配置核心变体之间，仅经过证据确认的架构标识和指令缓存策略不同。
* 不支持通过 ``-kernel`` 运行独立厂商 U-Boot；应使用匹配的 ``fip-all.bin``，使 BL31 提供平台 SMC 服务。
* 非启动关键外设仍未实现，因此预期会出现客户机探测失败。飞腾派 ES8336 音频编解码器未建模。
* Linux SCMI 传输仅实现 Base 协议，未实现 Performance 和 Sensor 协议，因此 SCMI CPU 频率和温度接口不可用。
