Orange Pi PC（``orangepi-pc``）
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

迅龙 Orange Pi PC 是基于 Allwinner H3 SoC 的嵌入式计算机，U-Boot 和 Linux 主线均支持。该板配备 1.3GHz 四核 Cortex-A7、1GiB RAM、100Mbit 以太网、USB、SD/MMC、USB、HDMI 以及多种其他 I/O。

支持的设备
"""""""""""""""""

Orange Pi PC 机器支持以下设备：

 * SMP（四核 Cortex-A7）
 * 通用中断控制器配置
 * SRAM 映射
 * SDRAM 控制器
 * 实时时钟
 * 定时器设备（复用 Allwinner A10 实现）
 * UART
 * SD/MMC 存储控制器
 * EMAC 以太网
 * USB 2.0 接口
 * 时钟控制单元
 * 系统控制模块
 * 安全标识设备
 * TWI（I2C）
 * 看门狗定时器

限制
"""""""""""

目前，Orange Pi PC *不* 支持以下功能：

- 通过 HDMI、GPU 和/或显示引擎输出图形
- 音频输出
- 硬件看门狗

完整的未实现 I/O 设备列表，还可参见 Allwinner H3 SoC 模块 ``./hw/arm/allwinner-h3.c`` 中的 'unimplemented' 数组。

启动选项
""""""""""""

Orange Pi PC 机器可使用标准 -kernel 功能加载 Linux 内核或 ELF 可执行文件并启动。此外，还可以模拟实际 Allwinner H3 SoC 中的 BootROM，从通过 qemu-system-arm 的 -sd 参数指定的 SD 卡加载引导加载程序。

机器专用选项
""""""""""""""""""""""""

支持以下机器专用选项：

- allwinner-rtc.base-year=YYYY

  Orange Pi PC 机器自动创建 Allwinner RTC 设备，采用默认基准年份，可通过 'base-year' 属性覆盖。基准年份是 RTC 年份值为零时代表的实际年份。如果目标操作系统驱动使用不同的基准年份，可使用此选项。基准年份最小值为 1900。

- allwinner-sid.identifier=abcd1122-a000-b000-c000-12345678ffff

  客户机可以读取安全标识值。例如，U-Boot 使用它确定唯一 MAC 地址。

上述机器专用选项可以通过 qemu-system-arm 的 '-global' 参数指定，例如：

.. code-block:: bash

  $ qemu-system-arm -M orangepi-pc -sd mycard.img \
       -global allwinner-rtc.base-year=2000


运行主线 Linux
""""""""""""""""""""""

已知从 4.19 到最新 master 的主线 Linux 内核可以运行。要构建可由 Orange Pi PC 机器启动的 Linux 主线内核，只需使用 sunxi_defconfig 配置：

.. code-block:: bash

  $ ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- make mrproper
  $ ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- make sunxi_defconfig


要使用 USB 存储，需要手动启用对应配置项。启动 kconfig 配置工具：

.. code-block:: bash

  $ ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- make menuconfig


找到以下选项，启用并保存配置：

  Device Drivers > USB support > USB Mass Storage support

构建 Linux 内核：

.. code-block:: bash

  $ ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- make


在 QEMU 中使用 Orange Pi PC 机器启动新构建的 Linux 内核：

.. code-block:: bash

  $ qemu-system-arm -M orangepi-pc -nic user -nographic \
      -kernel /path/to/linux/arch/arm/boot/zImage \
      -append 'console=ttyS0,115200' \
      -dtb /path/to/linux/arch/arm/boot/dts/sun8i-h3-orangepi-pc.dtb


Orange Pi PC 镜像
"""""""""""""""""""

注意，主线内核不包含根文件系统。可以使用官网下载的官方 Orange Pi PC 镜像提供根文件系统：

  http://www.orangepi.org/html/serviceAndSupport/index.html

另一种方式是运行适用于 Orange Pi PC 的 Armbian 镜像，可从以下地址下载：

   https://www.armbian.com/orange-pi-pc/

也可以使用 Buildroot 的 orangepi_pc_defconfig 构建自己的镜像。更多信息请参见 https://buildroot.org。

将镜像用作 SD 卡时，必须将其大小调整为二的幂，可使用 ``qemu-img`` 命令。为避免数据丢失，建议只扩大镜像，而不要缩小到二的幂。例如，准备下载的 Armbian 镜像时，先解压，再将大小增至 1GB：

.. code-block:: bash

  $ qemu-img resize Armbian_19.11.3_Orangepipc_bionic_current_5.3.9.img 1G


可以将选定镜像作为 SD 卡或 USB 大容量存储连接。例如，要从 SD 卡上的 Orange Pi PC Debian 镜像启动，只需添加 -sd 参数，并提供正确的 root= 内核参数：

.. code-block:: bash

  $ qemu-system-arm -M orangepi-pc -nic user -nographic \
      -kernel /path/to/linux/arch/arm/boot/zImage \
      -append 'console=ttyS0,115200 root=/dev/mmcblk0p2' \
      -dtb /path/to/linux/arch/arm/boot/dts/sun8i-h3-orangepi-pc.dtb \
      -sd OrangePi_pc_debian_stretch_server_linux5.3.5_v1.0.img


要将镜像作为 USB 大容量存储设备连接到机器，只需追加以下命令参数：

.. code-block:: bash

  -drive if=none,id=stick,file=myimage.img \
  -device usb-storage,bus=usb-bus.0,drive=stick


除了通过 -kernel 提供自定义 Linux 内核，也可以让 Orange Pi PC 机器从 SD 卡加载引导加载程序，就像真实板通过 BootROM 所做的那样。只需通过 -sd 参数传入选定镜像，并移除 -kernel、-append、-dbt 和 -initrd 参数：

.. code-block:: bash

  $ qemu-system-arm -M orangepi-pc -nic user -nographic \
       -sd Armbian_19.11.3_Orangepipc_buster_current_5.3.9.img


注意，官方 Orange Pi PC 镜像和 Armbian 镜像都会通过 systemd 启动大量用户空间程序。根据主机硬件和操作系统的不同，模拟可能较慢，尤其是同时模拟四个核心时。为减少模拟四核造成的性能下降，可以通过 U-Boot（或 -append）传入以下内核参数：

.. code-block:: bash

  => setenv extraargs 'systemd.default_timeout_start_sec=9000 loglevel=7 nosmp console=ttyS0,115200'


运行 U-Boot
""""""""""""""

可使用 orangepi_pc_defconfig 配置和构建主线 U-Boot，命令与上述 Linux 命令类似。注意，开发或测试时建议在 U-Boot 中选择以下配置：

  Device Tree Control > Provider for DTB for DT Control > Embedded DTB

要使用 Orange Pi PC 机器启动 U-Boot，将 u-boot 二进制文件传给 -kernel 参数：

.. code-block:: bash

  $ qemu-system-arm -M orangepi-pc -nic user -nographic \
      -kernel /path/to/uboot/u-boot -sd disk.img


使用以下 U-Boot 命令从 SD 卡加载并启动 Linux 内核：

.. code-block:: bash

  => setenv bootargs console=ttyS0,115200
  => ext2load mmc 0 0x42000000 zImage
  => ext2load mmc 0 0x43000000 sun8i-h3-orangepi-pc.dtb
  => bootz 0x42000000 - 0x43000000


运行 NetBSD
""""""""""""""

NetBSD 操作系统也支持基于 Allwinner H3 的板，包括 Orange Pi PC。已知 NetBSD 9.0 在 Orange Pi PC 上工作最好，提供具有串口控制台、网络和存储的完整可用系统。对于 Orange Pi PC 机器，从以下地址获取基于 'evbarm-earmv7hf' 的镜像：

  https://archive.netbsd.org/pub/NetBSD-archive/NetBSD-9.0/evbarm-earmv7hf/binary/gzimg/armv7.img.gz

需要手动将 U-Boot 安装到镜像中。按照上一节使用 orangepi_pc_defconfig 构建 U-Boot，然后解压 NetBSD 镜像，并写入包含 SPL 的 U-Boot 二进制文件：

.. code-block:: bash

  $ gunzip armv7.img.gz
  $ dd if=/path/to/u-boot-sunxi-with-spl.bin of=armv7.img bs=1024 seek=8 conv=notrunc


最后，启动机器前必须扩展 SD 镜像，使其大小为二的幂，并避免 NetBSD 内核判定 NetBSD 分区大于模拟 SD 卡：

.. code-block:: bash

  $ qemu-img resize armv7.img 2G


使用以下命令启动机器：

.. code-block:: bash

  $ qemu-system-arm -M orangepi-pc -nic user -nographic \
        -sd armv7.img -global allwinner-rtc.base-year=2000


在 U-Boot 阶段按键中断自动启动流程，并在启动前设置以下环境变量：

.. code-block:: bash

  => setenv bootargs root=ld0a
  => setenv kernel netbsd-GENERIC.ub
  => setenv fdtfile dtb/sun8i-h3-orangepi-pc.dtb
  => setenv bootcmd 'fatload mmc 0:1 ${kernel_addr_r} ${kernel}; fatload mmc 0:1 ${fdt_addr_r} ${fdtfile}; fdt addr ${fdt_addr_r}; bootm ${kernel_addr_r} - ${fdt_addr_r}'


可选择使用 'saveenv' 将环境变量保存到 SD 卡。输入 'boot' 命令即可继续启动 NetBSD。

Orange Pi PC 功能测试
"""""""""""""""""""""""""""""

Orange Pi PC 机器包含若干功能测试。要运行全部测试，从源码构建 QEMU，然后在构建目录执行：

.. code-block:: bash

  $ QEMU_TEST_ALLOW_LARGE_STORAGE=1 \
    pyvenv/bin/meson test --suite thorough func-arm-arm_orangepi
