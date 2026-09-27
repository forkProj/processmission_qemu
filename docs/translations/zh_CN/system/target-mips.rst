.. _MIPS-System-emulator:

MIPS 系统模拟器
--------------------

四个程序覆盖两种字节序的 32 位和 64 位 MIPS：qemu-system-mips、
qemu-system-mipsel、qemu-system-mips64 和 qemu-system-mips64el。
可模拟五种机器类型：

-  MIPS Malta 原型板 \"malta\"

-  ACER Pica \"pica61\". 该机器需要 64 位模拟器。

-  MIPS Magnum R4000 机器 \"magnum\". 该机器需要 64 位模拟器。

Malta 支持以下设备：

-  带 MIPS 24Kf CPU 和 Galileo 系统控制器的核心板

-  PIIX4 PCI/USB/SMbus 控制器

-  Multi-I/O 芯片的串行设备

-  PCI 网卡（PCnet32 等）

-  Malta FPGA 串行设备

-  Cirrus（默认）或其他 PCI VGA 显卡

Boston 支持以下设备：

-  带 PCIe 根端口和 UART 的 Xilinx FPGA

-  连接 I/O 外设的 Intel EG20T PCH，但仅模拟 SATA 总线

ACER Pica 支持：

-  MIPS R4000 CPU

-  PC 风格 IRQ 和 DMA 控制器

-  PC 键盘

-  IDE 控制器

MIPS Magnum R4000 支持：

-  MIPS R4000 CPU

-  PC 风格 IRQ 控制器

-  PC 键盘

-  SCSI 控制器

-  G364 帧缓冲

Fuloong 2E 支持：

-  Loongson 2E CPU

-  作为北桥的 Bonito64 系统控制器

-  作为南桥的 VT82C686 芯片组

-  RTL8139D 网卡芯片

Loongson-3 虚拟平台支持：

-  Loongson 3A CPU

-  LIOINTC 中断控制器

-  GPEX 和 virtio 外设

.. include:: cpu-models-mips.rst.inc

.. _nanoMIPS-System-emulator:

nanoMIPS 系统模拟器
~~~~~~~~~~~~~~~~~~~~~~~~

qemu-system-mipsel 也支持模拟小端 32 位 nanoMIPS：

-  nanoMIPS I7200 CPU

以下是 nanoMIPS 用法示例：

从以下位置下载 ``<disk_image_file>``：
https://mipsdistros.mips.com/LinuxDistro/nanomips/buildroot/index.html.

从以下位置下载 ``<kernel_image_file>``：
https://mipsdistros.mips.com/LinuxDistro/nanomips/kernels/v4.15.18-432-gb2eb9a8b07a1-20180627102142/index.html.

启动带 nanoMIPS I7200 CPU 的 Malta 模拟::

   qemu-system-mipsel -cpu I7200 -kernel <kernel_image_file> \
       -M malta -serial stdio -m <memory_size> -drive file=<disk_image_file>,format=raw \
       -append "mem=256m@0x0 rw console=ttyS0 vga=cirrus vesa=0x111 root=/dev/sda"
