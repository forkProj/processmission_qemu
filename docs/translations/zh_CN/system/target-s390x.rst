.. _s390x-System-emulator:

s390x 系统模拟器
---------------------

QEMU 通过 ``qemu-system-s390x`` 模拟 z/Architecture，特别是 64 位 s390x
系统。仅支持 ``s390-ccw-virtio`` 机器类型，并提供版本化以处理兼容性。

使用 KVM 加速时，可模拟到主机所属代际的 CPU。使用 TCG 默认 CPU 模型时，
模拟 z13 功能的子集，足以运行面向 z13 构建的发行版。

设备支持
==============

QEMU 不模拟 LPAR 或 z/VM 下的大多数传统设备，可用设备主要是 virtio，尤其
是 virtio-ccw。也支持通过 vfio-pci、vfio-ccw 或 vfio-ap 直通主机设备。

.. toctree::
   s390x/vfio-ap
   s390x/css
   s390x/3270
   s390x/vfio-ccw
   s390x/pcidevices

架构功能
======================

.. toctree::
   s390x/bootdevices
   s390x/protvirt
   s390x/cpu-topology
