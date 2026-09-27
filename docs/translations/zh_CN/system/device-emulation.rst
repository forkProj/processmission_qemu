.. _device-emulation:

设备模拟
----------------

QEMU 能模拟大量设备，从网卡、USB 等外设到集成的片上系统（SoC）。配置
这些设备时容易混淆，理解 QEMU 使用的术语会很有帮助。

常用术语
~~~~~~~~~~~~

设备前端
================

设备前端决定客户机看到怎样的设备，类型应与客户机操作系统预期的硬件一致。
所有设备都可通过 ``--device`` 指定。``--device help`` 列出所有已知设备，
``--device foo,help`` 列出该设备额外的配置选项。

前端通常配有后端，描述模拟过程中如何使用主机资源。

设备总线
============

大多数设备连接到某种总线。机器模型 ``-M foo`` 会自动创建一组总线。通常
可以推断设备所连总线，例如 PCI 设备会自动分配到首条 PCI 总线的下一个
空闲地址。复杂配置也可以通过 ``bus=ID`` 和 ``addr=N`` 显式指定。

部分设备会添加新总线供其他设备连接，例如 PCI SCSI 主控制器。假设设备链为：

  --device foo,bus=pci.0,addr=0,id=foo
  --device bar,bus=foo.0,addr=1,id=baz

这表示 ID 为 baz 的 bar 设备位于第一条 foo 总线 foo.0 的地址 1，而提供
该总线的 foo 设备连接到第一条 PCI 总线 pci.0。


设备后端
===============

后端决定 QEMU 如何处理模拟设备的数据，配置通常随设备类别不同而变化。
串行设备用 ``--chardev`` 将数据重定向到文件、套接字或其他系统。存储设备
用 ``--blockdev`` 指定块处理方式，例如保存到 qcow2，或访问主机原始磁盘
分区。后端有时可以堆叠，以实现快照等功能。

后端选择通常对客户机透明，但后端无法支持某功能时，该功能可能不会向客户机报告。

设备直通
===================

设备直通让客户机实际访问底层硬件，例如暴露主机上的单个 USB 设备，或将
PCI 插槽中的显卡专供客户机使用。


模拟设备
~~~~~~~~~~~~~~~~

.. toctree::
   :maxdepth: 1

   devices/virtio/index.rst

   devices/can.rst
   devices/canokey.rst
   devices/ccid.rst
   devices/cxl.rst
   devices/emmc.rst
   devices/igb.rst
   devices/ivshmem-flat.rst
   devices/ivshmem.rst
   devices/keyboard.rst
   devices/net.rst
   devices/nvme.rst
   devices/scsi/index.rst
   devices/usb-u2f.rst
   devices/usb.rst
   devices/vfio-user.rst
