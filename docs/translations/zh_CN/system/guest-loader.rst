..
   Copyright (c) 2020, Linaro

客户机加载器
------------

客户机加载器类似 ``generic-loader``，但专门用于加载虚拟机监控程序的客户机。
它可以绕过固件和引导加载程序的繁琐步骤，方便调试监控程序。

它执行两项工作：

  - 将二进制数据块（内核和初始 RAM 磁盘）加载到内存
  - 设置平台 FDT 数据，让监控程序能够找到并启动它们

通常由 grub 等引导加载程序的多重引导功能完成这些工作。典型示例为：

.. parsed-literal::

  |qemu_system| -kernel ~/xen.git/xen/xen \
    -append "dom0_mem=1G,max:1G loglvl=all guest_loglvl=all" \
    -device guest-loader,addr=0x42000000,kernel=Image,bootargs="root=/dev/sda2 ro console=hvc0 earlyprintk=xen" \
    -device guest-loader,addr=0x47000000,initrd=rootfs.cpio

上例用 -kernel 加载 Xen，并用 -append 传入其启动参数。Dom0 客户机载入
相应内存区域。每个数据块在 FDT 中都有 ``/chosen/module@<addr>`` 条目，
记录位置和大小。还可以通过额外参数传递其他信息。

目前使用 FDT 启动且受支持的机器仅为 Arm 和 RISC-V 的 ``virt``。

参数
^^^^^^^^^

完整语法为::

  -device guest-loader,addr=<addr>[,kernel=<file>,[bootargs=<args>]][,initrd=<file>]

``addr=<addr>``
  必需，指定数据块的起始地址。

``kernel|initrd=<file>``
  指定内核或 initrd 文件名。两者都带 "multiboot,module" 兼容字符串，
  并按类型附加 "multiboot,kernel" 或 "multiboot,ramdisk"。

``bootargs=<args>``
  内核数据块的可选字段，通过 ``/chosen/module@<addr>/bootargs`` 节点
  传递命令行。
