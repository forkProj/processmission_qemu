Banana Pi BPI-M2U（``bpim2u``）
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Banana Pi BPI-M2 Ultra 是基于 Allwinner A40i/R40/V40 SoC 的四核迷你单板计算机，具有 2GB RAM、8GB eMMC，以及板载 WiFi 和蓝牙。接口方面，BPI-M2 Ultra 提供两个 USB A 2.0 端口、一个 USB OTG 端口、一个 HDMI 端口、一个音频插孔、一个直流电源端口和一个 SATA 端口。

支持的设备
"""""""""""""""""

Banana Pi M2U 机器支持以下设备：

 * SMP（四核 Cortex-A7）
 * 通用中断控制器配置
 * SRAM 映射
 * SDRAM 控制器
 * 定时器设备（复用 Allwinner A10 实现）
 * UART
 * SD/MMC 存储控制器
 * EMAC 以太网
 * GMAC 以太网
 * 时钟控制单元
 * SATA
 * TWI（I2C）
 * USB 2.0
 * 硬件看门狗

限制
"""""""""""

目前，Banana Pi M2U *不* 支持以下功能：

- 通过 HDMI、GPU 和/或显示引擎输出图形
- 音频输出
- 实时时钟

完整的未实现 I/O 设备列表还可参见 Allwinner R40 SoC 模块 ``./hw/arm/allwinner-r40.c`` 中的 'unimplemented' 数组。

启动选项
""""""""""""

Banana Pi M2U 机器可使用标准 -kernel 功能加载 Linux 内核或 ELF 可执行文件并启动。此外，它还可以模拟实际 Allwinner R40 SoC 中的 BootROM，从通过 qemu-system-arm 的 -sd 参数指定的 SD 卡加载引导加载程序。

运行主线 Linux
""""""""""""""""""""""

要构建可由 Banana Pi M2U 机器启动的 Linux 主线内核，只需使用 sunxi_defconfig 配置内核：

.. code-block:: bash

  $ ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- make mrproper
  $ ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- make sunxi_defconfig


在 QEMU 中使用 Banana Pi M2U 机器启动新构建的 Linux 内核：

.. code-block:: bash

  $ qemu-system-arm -M bpim2u -nographic \
      -kernel /path/to/linux/arch/arm/boot/zImage \
      -append 'console=ttyS0,115200' \
      -dtb /path/to/linux/arch/arm/boot/dts/sun8i-r40-bananapi-m2-ultra.dtb


Banana Pi M2U 镜像
""""""""""""""""""""

注意，主线内核不包含根文件系统。可以使用 Buildroot 的 bananapi_m2_ultra_defconfig 构建自己的镜像。更多信息请参见 https://buildroot.org。

另一种方式是运行适用于 Banana Pi M2U 的 OpenWrt 镜像，可从以下地址下载：

   https://downloads.openwrt.org/releases/22.03.3/targets/sunxi/cortexa7/

将镜像用作 SD 卡时，必须将大小调整为二的幂。可用 ``qemu-img`` 命令完成。为避免数据丢失，建议仅扩大镜像，而不要将其缩小到二的幂。例如，准备下载的 Armbian 镜像时，先解压，再将大小增至 1GB：

.. code-block:: bash

  $ qemu-img resize \
    openwrt-22.03.3-sunxi-cortexa7-sinovoip_bananapi-m2-ultra-ext4-sdcard.img \
    1G


除了通过 -kernel 提供自定义 Linux 内核外，也可以让 Banana Pi M2U 机器从 SD 卡加载引导加载程序，就像真实板通过 BootROM 所做的那样。只需通过 -sd 参数传入选定镜像，并移除 -kernel、-append、-dbt 和 -initrd 参数：

.. code-block:: bash

  $ qemu-system-arm -M bpim2u -nic user -nographic \
    -sd openwrt-22.03.3-sunxi-cortexa7-sinovoip_bananapi-m2-ultra-ext4-sdcard.img


运行 U-Boot
""""""""""""""

可以使用 Bananapi_M2_Ultra_defconfig 配置和构建主线 U-Boot，命令与上述 Linux 命令类似。注意，开发或测试时，建议在 U-Boot 中选择以下配置：

  Device Tree Control > Provider for DTB for DT Control > Embedded DTB

Allwinner R40 的 BootROM 从 SD 卡偏移 8KiB 处加载 U-Boot。创建可启动磁盘镜像：

.. code-block:: bash

  $ dd if=/dev/zero of=sd.img bs=32M count=1
  $ dd if=u-boot-sunxi-with-spl.bin of=sd.img bs=1k seek=8 conv=notrunc


然后启动它。

.. code-block:: bash

  $ qemu-system-arm -M bpim2u -nographic -sd sd.img


Banana Pi M2U 功能测试
""""""""""""""""""""""""""""""

Banana Pi M2U 机器包含若干功能测试。要运行全部测试，请从源码构建 QEMU，然后执行以下命令：

.. code-block:: bash

  $ cd qemu-build-dir
  $ QEMU_TEST_ALLOW_LARGE_STORAGE=1 \
    pyvenv/bin/meson test --suite thorough func-arm-arm_bpim2u
