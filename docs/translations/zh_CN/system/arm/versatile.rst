Arm Versatile 板（``versatileab``、``versatilepb``）
=======================================================

模拟的 Arm Versatile 底板包含以下设备：

-  ARM926E、ARM1136 或 Cortex-A8 CPU

-  PL190 向量中断控制器

-  四个 PL011 UART

-  SMC 91c111 以太网适配器

-  PL110 LCD 控制器

-  带 PS/2 键盘和鼠标的 PL050 KMI

-  PCI 主机桥。注意，模拟的 PCI 桥仅提供对 PCI 内存空间的访问，不提供对 PCI I/O 空间的访问。这意味着某些设备（例如 ne2k_pci 网卡）无法使用，而其他设备（例如 rtl8139 网卡）只有在客户机驱动使用内存映射控制寄存器时才能使用。

-  PCI OHCI USB 控制器

-  带硬盘和 CD-ROM 设备的 LSI53C895A PCI SCSI 主机总线适配器

-  带 SD 卡的 PL181 多媒体卡接口

启动 Linux 内核
----------------------

使用 ``versatile_defconfig`` 构建当前 Linux 内核，应足以启动运行。现在建议采用树外构建（构建多个不同目标时也很有用）。下例中，$BLD 指向构建目录，$SRC 指向 Linux 源代码树根目录。如果从源代码树根目录运行，可省略 $SRC。

.. code-block:: bash

  $ make O=$BLD -C $SRC ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- versatile_defconfig
  $ make O=$BLD -C $SRC ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-


如果要从 SCSI 接口启动，可能需要启用一些额外模块::

  CONFIG_PCI=y
  CONFIG_PCI_VERSATILE=y
  CONFIG_SCSI=y
  CONFIG_SCSI_SYM53C8XX_2=y


随后可以使用如下命令行启动：

.. code-block:: bash

  $ qemu-system-arm -machine type=versatilepb \
      -serial mon:stdio \
      -drive if=scsi,driver=file,filename=debian-buster-armel-rootfs.ext4 \
      -kernel zImage \
      -dtb versatile-pb.dtb  \
      -append "console=ttyAMA0 ro root=/dev/sda"
