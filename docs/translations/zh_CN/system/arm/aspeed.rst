Aspeed 系列板（``anacapa-bmc``、``ast2500-evb``、``ast2600-evb``、``bletchley-bmc``、``fuji-bmc``、``gb200nvl-bmc``、``fby35-bmc``、``g220a-bmc``、``palmetto-bmc``、``quanta-q71l-bmc``、``rainier-bmc``、``romulus-bmc``、``supermicrox11-bmc``、``supermicrox11spi-bmc``、``tiogapass-bmc``、``witherspoon-bmc``、``yosemitev2-bmc``）
===============================================================================================================================================================================================================================================================================================================================================

QEMU 的 Aspeed 机器模拟各种 OpenPOWER 系统和 Aspeed 评估板的 BMC。它们基于不同版本的 Aspeed SoC：AST2400 集成 ARM926EJ-S CPU（400MHz），AST2500 采用 ARM1176JZS CPU（800MHz），AST2600 采用双核 Arm Cortex-A7 CPU（1.2GHz）。

SoC 提供 RAM、千兆以太网、USB、SD/MMC、USB、SPI、I2C 等。

基于 AST2400 SoC 的机器：

- ``palmetto-bmc``         OpenPOWER Palmetto POWER8 BMC
- ``quanta-q71l-bmc``      OpenBMC Quanta BMC
- ``supermicrox11-bmc``    Supermicro X11 BMC（ARM926EJ-S）
- ``supermicrox11spi-bmc`` Supermicro X11 SPI BMC（ARM1176）

基于 AST2500 SoC 的机器：

- ``ast2500-evb``          Aspeed AST2500 评估板
- ``romulus-bmc``          OpenPOWER Romulus POWER9 BMC
- ``witherspoon-bmc``      OpenPOWER Witherspoon POWER9 BMC
- ``g220a-bmc``            字节跳动 G220A BMC
- ``yosemitev2-bmc``       Facebook YosemiteV2 BMC
- ``tiogapass-bmc``        Facebook Tiogapass BMC

基于 AST2600 SoC 的机器：

- ``ast2600-evb``          Aspeed AST2600 评估板（Cortex-A7）
- ``rainier-bmc``          IBM Rainier POWER10 BMC
- ``fuji-bmc``             Facebook Fuji BMC
- ``bletchley-bmc``        Facebook Bletchley BMC
- ``fby35-bmc``            Facebook fby35 BMC
- ``anacapa-bmc``          Facebook Anacapa BMC
- ``gb200nvl-bmc``         Nvidia GB200nvl BMC

支持的设备
-----------------

 * SMP（用于 AST2600 Cortex-A7）
 * 中断控制器（VIC）
 * 定时器控制器
 * RTC 控制器
 * I2C 控制器，包括 AST2600 的新寄存器接口
 * 系统控制单元（SCU）
 * SRAM 映射
 * X-DMA 控制器（基本接口）
 * 静态内存控制器（SMC 或 FMC），仅支持 SPI 闪存
 * SPI 内存控制器
 * USB 2.0 控制器
 * SD/MMC 存储控制器
 * SDRAM 控制器（用于基本设置和训练的虚拟接口）
 * 看门狗控制器
 * GPIO 控制器（仅主模式）
 * UART
 * 以太网控制器
 * 前面板 LED（I2C 总线上的 PCA9552）
 * LPC 外设控制器（支持部分子设备）
 * 哈希/密码引擎（HACE），仅支持哈希；待实现 HMAC 和 RSA
 * ADC
 * 安全启动控制器（AST2600）
 * eMMC 启动控制器（虚拟实现）
 * PECI 控制器（最小实现）
 * I3C 控制器
 * 内部桥控制器（SLI 虚拟实现）


缺少的设备
---------------

 * 协处理器支持
 * PWM 和风扇控制器
 * 从模式 GPIO 控制器
 * Super I/O 控制器
 * PCI-Express 1 控制器
 * 图形显示控制器
 * MCTP 控制器
 * 邮箱控制器
 * 虚拟 UART
 * eSPI 控制器

启动选项
------------

Aspeed 机器可以使用 ``-kernel`` 和 ``-dtb`` 选项加载 Linux 内核启动，也可以从固件启动。镜像可从 OpenBMC Jenkins 下载：

   https://jenkins.openbmc.org/job/ci-openbmc/lastSuccessfulBuild/

也可直接从 OpenBMC GitHub 发布仓库下载：

   https://github.com/openbmc/openbmc/releases

或从 ASPEED 派生的 OpenBMC GitHub 发布仓库下载：

   https://github.com/AspeedTech-BMC/openbmc/releases

从内核镜像启动
^^^^^^^^^^^^^^^^^^^^^^^^^^^

直接从 Linux 构建树启动内核：

.. code-block:: bash

  $ qemu-system-arm -M ast2600-evb -nographic \
        -kernel arch/arm/boot/zImage \
        -dtb arch/arm/boot/dts/aspeed-ast2600-evb.dtb \
        -initrd rootfs.cpio


从闪存镜像启动
^^^^^^^^^^^^^^^^^^^^^^^^^^^

从闪存镜像启动时，Aspeed 专用的机器选项为：

 * ``execute-in-place`` 通过 FMC 控制器加载指令，模拟从 CE0 闪存设备启动，而不是简单地从 RAM 启动。这会稍慢一些。

 * ``fmc-model`` 更改默认 FMC 闪存型号。固件必须支持该芯片型号才能启动。

 * ``spi-model`` 更改默认 SPI 闪存型号。

使用 MTD 驱动器从闪存镜像启动机器：

.. code-block:: bash

  $ qemu-system-arm -M romulus-bmc -nic user \
	-drive file=obmc-phosphor-image-romulus.static.mtd,format=raw,if=mtd -nographic


要使用其他闪存型号，例如为 ``ast2500-evb`` 机器使用不同的 FMC 芯片和更大的 64M SPI 闪存，运行：

.. code-block:: bash

  -M ast2500-evb,fmc-model=mx25l25635e,spi-model=mx66u51235f


如果需要更灵活地定义闪存设备，使用不同型号或定义全部闪存设备（最多八个），可以使用 QEMU 的 ``-nodefaults`` 选项避免创建默认闪存设备。

随后应从命令行创建闪存设备，并连接到块设备：

.. code-block:: bash

  $ qemu-system-arm -M ast2600-evb \
        -blockdev node-name=fmc0,driver=file,filename=/path/to/fmc0.img \
	-device mx66u51235f,bus=ssi.0,cs=0x0,drive=fmc0 \
	-blockdev node-name=fmc1,driver=file,filename=/path/to/fmc1.img \
	-device mx66u51235f,bus=ssi.0,cs=0x1,drive=fmc1 \
	-blockdev node-name=spi1,driver=file,filename=/path/to/spi1.img \
	-device mx66u51235f,cs=0x0,bus=ssi.1,drive=spi1 \
	-nographic -nodefaults


此时，机器从 FMC0 设备取指启动。启动较慢，但更接近硬件行为。使用机器选项 ``execute-in-place`` 具有类似效果。

从 eMMC 镜像启动
^^^^^^^^^^^^^^^^^^^^^^^^^^

从 eMMC 镜像启动时，Aspeed 专用的机器选项为：

 * ``boot-emmc`` 设置或取消从 eMMC 启动（AST2600）。

只有 ``ast2600-evb`` 和 ``rainier-emmc`` 机器支持从 eMMC 设备启动。此时，机器假定 eMMC 镜像包含特殊启动分区。这类镜像可按以下方式构建：

.. code-block:: bash

   $ dd if=/dev/zero of=mmc-bootarea.img count=2 bs=1M
   $ dd if=u-boot-spl.bin of=mmc-bootarea.img conv=notrunc
   $ dd if=u-boot.bin of=mmc-bootarea.img conv=notrunc count=64 bs=1K
   $ cat mmc-bootarea.img obmc-phosphor-image.wic > mmc.img
   $ truncate --size 16GB mmc.img


启动 ``rainier-emmc`` 机器：

.. code-block:: bash

   $ qemu-system-arm -M rainier-bmc \
         -drive file=mmc.img,format=raw,if=sd,index=2 \
         -nographic


可设置或取消 ``boot-emmc`` 选项，改变机器默认启动模式，即 SPI 或 eMMC。这可用于让默认从 SPI 启动的 ``ast2600-evb`` 从 eMMC 启动，或让默认从 eMMC 启动的 ``rainier-bmc`` 从闪存启动。

例如，以下使用 ``boot-emmc=false`` 使 ``rainier-bmc`` 从闪存设备启动，同时使用 eMMC 镜像：

.. code-block:: bash

   $ qemu-system-arm -M rainier-bmc,boot-emmc=false \
        -drive file=flash.img,format=raw,if=mtd \
        -drive file=mmc.img,format=raw,if=sd,index=2 \
        -nographic


需要注意，此时 eMMC 设备不能包含启动分区，否则机器无法访问其内容。此限制源于使用 ``-drive`` 接口。

理想情况下，应能够直接在命令行中定义 eMMC 设备及其后端，例如：

.. code-block:: bash

   -blockdev node-name=emmc0,driver=file,filename=mmc.img \
   -device emmc,bus=sdhci-bus.2,drive=emmc0,boot-partition-size=1048576,boot-config=8


截至 QEMU 10.0 尚不支持此方式，需要重构 sdhci 总线模型。

其他启动选项
^^^^^^^^^^^^^^^^^^^^^

其他 Aspeed 专用机器选项包括：

 * ``bmc-console`` 更改默认控制台设备。大多数机器使用 ``UART5`` 作为启动控制台，在 Linux 下映射为 ``/dev/ttyS4``，但并非总是如此。

要更改启动控制台，使用 ``UART3`` （Linux 下的 ``/dev/ttyS2``），运行：

.. code-block:: bash

  -M ast2500-evb,bmc-console=uart3


OTP 选项
^^^^^^^^^^

AST2600 和 AST1030 使用相同的一次性可编程（OTP）内存模块，用于配置、密钥存储和用户可编程数据存储。该模块由安全启动控制器（SBC）管理。以下选项可按需指定或省略。

  * 指定选项时，使用预先生成的配置文件作为 OTP 内存存储。

  * 省略选项时，使用内部内存缓冲区存储 OTP 数据。

.. code-block:: bash

  -blockdev driver=file,filename=otpmem.img,node-name=otp \
  -global aspeed-otp.drive=otp \


可以使用以下 Bash 命令生成 OTP 内存的默认配置文件：

.. code-block:: bash

  if [ ! -f otpmem.img ]; then
    for i in $(seq 1 2048); do
      printf '\x00\x00\x00\x00\xff\xff\xff\xff'
    done > otpmem.img
  fi


Aspeed 2700 系列板（``ast2700-evb``、``ast2700fc``）
==========================================================

QEMU 的 Aspeed 机器模拟 Aspeed 评估板的 BMC，基于不同版本的 Aspeed SoC：AST2700 具有四核 Arm Cortex-A35 64 位 CPU（1.6GHz）。

SoC 提供 RAM、千兆以太网、USB、SD/MMC、USB、SPI、I2C 等。

基于 AST2700 SoC 的机器：

- ``ast2700-evb``          Aspeed AST2700 评估板（Cortex-A35）
- ``ast2700fc``            Aspeed AST2700 评估板（Cortex-A35 + Cortex-M4）

支持的设备
-----------------

 * 中断控制器
 * 定时器控制器
 * RTC 控制器
 * I2C 控制器
 * 系统控制单元（SCU）
 * SRAM 映射
 * X-DMA 控制器（基本接口）
 * 静态内存控制器（SMC 或 FMC），仅支持 SPI 闪存
 * SPI 内存控制器
 * USB 2.0 控制器
 * SD/MMC 存储控制器
 * SDRAM 控制器（用于基本设置和训练的虚拟接口）
 * 看门狗控制器
 * GPIO 控制器（仅主模式）
 * UART
 * 以太网控制器
 * 前面板 LED（I2C 总线上的 PCA9552）
 * LPC 外设控制器（支持部分子设备）
 * 哈希/密码引擎（HACE），仅支持哈希；待实现密码功能
 * ADC
 * eMMC 启动控制器（虚拟实现）
 * PECI 控制器（最小实现）
 * I3C 控制器
 * 内部桥控制器（SLI 虚拟实现）

缺少的设备
---------------

 * PWM 和风扇控制器
 * 从模式 GPIO 控制器
 * Super I/O 控制器
 * PCI-Express 1 控制器
 * 图形显示控制器
 * MCTP 控制器
 * 邮箱控制器
 * 虚拟 UART
 * eSPI 控制器

启动选项
------------

镜像可从 ASPEED 派生的 OpenBMC GitHub 发布仓库下载：

   https://github.com/AspeedTech-BMC/openbmc/releases

启动 ast2700-evb 机器
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

从闪存镜像启动 AST2700 机器。

支持两种使用闪存镜像启动 AST2700 的方式。

使用 ``-device loader`` 手动启动：

这使全部四个 CPU 核心从地址 ``0x430000000`` 开始执行，该地址对应 BL31 镜像加载地址。

.. code-block:: bash

  IMGDIR=ast2700-default

  $ qemu-system-aarch64 -M ast2700-evb \
       -device loader,force-raw=on,addr=0x400000000,file=${IMGDIR}/u-boot.bin \
       -device loader,force-raw=on,addr=0x430000000,file=${IMGDIR}/bl31.bin \
       -device loader,force-raw=on,addr=0x430080000,file=${IMGDIR}/optee/tee-raw.bin \
       -device loader,cpu-num=0,addr=0x430000000 \
       -device loader,cpu-num=1,addr=0x430000000 \
       -device loader,cpu-num=2,addr=0x430000000 \
       -device loader,cpu-num=3,addr=0x430000000 \
       -smp 4 \
       -drive file=${IMGDIR}/image-bmc,format=raw,if=mtd \
       -nographic


使用虚拟启动 ROM（``-bios``）启动：

如果用户未指定 ``-bios option``，QEMU 会尝试从当前工作目录或 QEMU 源码树的 ``pc-bios`` 目录加载默认 vbootrom 镜像 ``ast27x0_bootrom.bin``。

.. code-block:: bash

  $ qemu-system-aarch64 -M ast2700-evb \
      -drive file=image-bmc,format=raw,if=mtd \
      -nographic


``-bios`` 选项允许指定启动时加载的 vbootrom 镜像的自定义路径。下例从 ${HOME} 目录中的指定路径加载镜像。

.. code-block:: bash

  -bios ${HOME}/ast27x0_bootrom.bin


启动 ast2700fc 机器
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

AST2700 具有四个 Cortex-A35 主处理器和两个 Cortex-M4 协处理器。**ast2700-evb** 机器专注于模拟四个 Cortex-A35 主处理器，**ast2700fc** 在此基础上增加两个 Cortex-M4 协处理器支持。

启动 AST2700fc 的步骤：

1. 确保目录中具备以下 AST2700A1 二进制文件：

 * u-boot-nodtb.bin
 * u-boot.dtb
 * bl31.bin
 * optee/tee-raw.bin
 * image-bmc
 * zephyr-aspeed-ssp.elf（SSP 固件，CPU 5）
 * zephyr-aspeed-tsp.elf（TSP 固件，CPU 6）

2. 执行以下命令启动 ``ast2700fc`` 机器：

.. code-block:: bash

  IMGDIR=ast2700-default

  $ qemu-system-aarch64 -M ast2700fc \
       -device loader,force-raw=on,addr=0x400000000,file=${IMGDIR}/u-boot.bin \
       -device loader,force-raw=on,addr=0x430000000,file=${IMGDIR}/bl31.bin \
       -device loader,force-raw=on,addr=0x430080000,file=${IMGDIR}/optee/tee-raw.bin \
       -device loader,cpu-num=0,addr=0x430000000 \
       -device loader,cpu-num=1,addr=0x430000000 \
       -device loader,cpu-num=2,addr=0x430000000 \
       -device loader,cpu-num=3,addr=0x430000000 \
       -drive file=${IMGDIR}/image-bmc,if=mtd,format=raw \
       -device loader,file=${IMGDIR}/zephyr-aspeed-ssp.elf,cpu-num=4 \
       -device loader,file=${IMGDIR}/zephyr-aspeed-tsp.elf,cpu-num=5 \
       -serial pty -serial pty -serial pty \
       -snapshot \
       -S -nographic


QEMU 启动后，串口设备会自动重定向。输出示例：

.. code-block:: bash

   char device redirected to /dev/pts/55 (label serial0)
   char device redirected to /dev/pts/56 (label serial1)
   char device redirected to /dev/pts/57 (label serial2)


- serial0：四个 Cortex-A35 主处理器的控制台。
- serial1 和 serial2：两个 Cortex-M4 协处理器的控制台。

使用 ``tio`` 或其他终端模拟器连接控制台：

.. code-block:: bash

   $ tio /dev/pts/55
   $ tio /dev/pts/56
   $ tio /dev/pts/57



Aspeed 桥接 IC 和平台信任根处理器系列板（``ast1030-evb``、``ast1040-evb``、``ast1060-evb``）
=======================================================================================================================

QEMU 的 Aspeed 机器模拟各种 Aspeed 评估板的桥接 IC 和平台信任根处理器，基于 Aspeed AST10x0 SoC 系列的不同成员：

- AST1030：桥接 IC
- AST1040：桥接 IC
- AST1060：平台信任根处理器

AST1030 和 AST1060 集成运行在 200 MHz 的 Arm Cortex-M4F CPU。AST1040 集成运行在 400 MHz 的 Arm Cortex-M4F CPU。

SoC 提供 SRAM、SPI、I2C 等。

基于 AST10x0 SoC 的机器：

- ``ast1030-evb``          Aspeed AST1030 评估板（Cortex-M4F）
- ``ast1040-evb``          Aspeed AST1040 评估板（Cortex-M4F）
- ``ast1060-evb``          Aspeed AST1060 评估板（Cortex-M4F）

支持的设备
-----------------

 * SMP（用于 Cortex-M4F）
 * 中断控制器（VIC）
 * 定时器控制器
 * I2C 控制器
 * 系统控制单元（SCU）
 * SRAM 映射
 * 静态内存控制器（SMC 或 FMC），仅支持 SPI 闪存
 * SPI 内存控制器
 * USB 2.0 控制器
 * 看门狗控制器
 * GPIO 控制器（仅主模式）
 * UART
 * LPC 外设控制器（支持部分子设备）
 * 哈希/密码引擎（HACE），仅支持哈希；待实现 HMAC 和 RSA
 * ADC
 * 安全启动控制器
 * PECI 控制器（最小实现）


缺少的设备
---------------

 * PWM 和风扇控制器
 * 从模式 GPIO 控制器
 * 邮箱控制器
 * 虚拟 UART
 * eSPI 控制器
 * I3C 控制器
 * SMBus 过滤控制器
 * QSPI 监控控制器

启动选项
------------

Aspeed 机器可以通过 ``-kernel`` 加载 Zephyr 操作系统启动，也可以从固件启动。镜像可从 ASPEED GitHub 发布仓库下载：

   https://github.com/AspeedTech-BMC/zephyr/releases

直接从 Zephyr 构建树启动内核：

.. code-block:: bash

  $ qemu-system-arm -M ast1030-evb -nographic \
        -kernel zephyr.bin
