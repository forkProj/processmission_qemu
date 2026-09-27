VirtIO 设备
==============

VirtIO 是为高效模拟和虚拟化设计的半虚拟化设备。除非专门测试某种硬件驱动，
否则推荐虚拟机使用这些模型。

`VirtIO specification`_ 是 OASIS 管理的开放标准，描述客户机驱动如何与
QEMU 设备模型交互。多个操作系统提供 VirtIO 驱动，其中 Linux 支持的设备
类型可能最广。

设备可以完全由 QEMU 实现，也可与内核协作，即 vhost，还可通过
:ref:`vhost user <vhost_user>` 卸载到外部进程。

.. toctree::
   :maxdepth: 1

   virtio-gpu.rst
   virtio-pmem.rst
   virtio-snd.rst
   vhost-user.rst
   vhost-user-contrib.rst

.. _VirtIO specification: https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html
