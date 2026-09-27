===========================
在 VFIO 中使用 IOMMUFD 后端
===========================

（本文中的 backend、container 和 BE 含义相同。）

引入 iommufd 后，Linux 内核为用户态驱动提供通用接口，
将直通设备的 DMA 映射传递到内核。
旧接口以组为中心，新接口则以设备为中心，依赖设备 fd 和 iommufd。

为同时支持两种接口，QEMU VFIO 引入基础容器，
抽象旧容器和 iommufd 容器的公共部分，让通用 VFIO 代码可使用任一实现。

基础容器实现 memory_listener、地址空间管理等公共功能，
派生容器实现各自专用回调。
每种容器用自己的方式建立安全上下文和 DMA 管理接口，结构如下。

::

                      VFIO                           AddressSpace/Memory
      +-------+  +----------+  +-----+  +-----+
      |  pci  |  | platform |  |  ap |  | ccw |
      +---+---+  +----+-----+  +--+--+  +--+--+     +----------------------+
          |           |           |        |        |   AddressSpace       |
          |           |           |        |        +------------+---------+
      +---V-----------V-----------V--------V----+               /
      |           VFIOAddressSpace              | <------------+
      |                  |                      |  MemoryListener
      |        VFIOContainerBase 列表           |
      +-------+----------------------------+----+
              |                            |
              |                            |
      +-------V------+            +--------V----------+
      |   iommufd    |            |    旧版 vfio      |
      |  容器        |            |     容器          |
      +-------+------+            +--------+----------+
              |                            |
              | /dev/iommu                 | /dev/vfio/vfio
              | /dev/vfio/devices/vfioX    | /dev/vfio/$group_id
  用户空间    |                            |
  ============+============================+===========================
  内核态      |  设备 fd                   |
              +---------------+            | 组/容器 fd
              | (BIND_IOMMUFD |            | (SET_CONTAINER/SET_IOMMU)
              |  ATTACH_IOAS) |            | 设备 fd
              |               |            |
              |       +-------V------------V-----------------+
      iommufd |       |                vfio                  |
  (map/unmap  |       +---------+--------------------+-------+
  ioas_copy)  |                 |                    | map/unmap
              |                 |                    |
       +------V------+    +-----V------+      +------V--------+
       | iommfd 核心 |    |  设备      |      |  vfio iommu   |
       +-------------+    +------------+      +---------------+

* 安全上下文设置

  - iommufd 后端：使用设备 fd 和 iommufd，通过 bind_iommufd、attach_ioas 设置。
  - 旧 VFIO 后端：使用组 fd 和容器 fd，通过 set_container、set_iommu 设置。

* 设备访问

  - iommufd 后端：通过 ``/dev/vfio/devices/vfioX`` 打开设备 fd。
  - 旧 VFIO 后端：通过组 fd 的 ioctl 获取设备 fd。

* DMA 映射流程

  1. VFIOAddressSpace 通过 MemoryListener 接收 MemoryRegion 添加或删除通知。
  2. VFIO 通过容器后端执行 DMA 映射或解除映射。
     * iommufd 后端使用 iommufd。
     * 旧 VFIO 后端使用容器 fd。

配置示例
========

步骤 1：配置主机设备
--------------------

与使用旧 VFIO 容器时完全相同。

步骤 2：配置 QEMU
-----------------

新 iommufd 对象抽象与 /dev/iommu 的交互，通过 CONFIG_IOMMUFD 编译启用。

任何需要使用 /dev/iommu 的 QEMU 设备，例如 VFIO 设备，
都必须关联 iommufd 对象。新增可选 iommufd 属性用于传入该对象。
以 vfio-pci 为例：

.. code-block:: bash

    -object iommufd,id=iommufd0
    -device vfio-pci,host=0000:02:00.0,iommufd=iommufd0

管理层也可在外部打开 /dev/iommu 和 VFIO 字符设备，再传入 fd。
fd 可用名称字符串或数字表示，例如：

.. code-block:: bash

    -object iommufd,id=iommufd0,fd=22
    -device vfio-pci,iommufd=iommufd0,fd=23

不传 fd 属性时，由 QEMU 打开。

如果未给 vfio-pci 传入 iommufd 对象，就不使用 iommufd，而采用旧 VFIO 容器：

.. code-block:: bash

    -device vfio-pci,host=0000:02:00.0

支持的平台
==========

目前支持 x86、Arm 和 s390x。

注意事项
========

P2P DMA
--------

IOMMUFD 尚不支持映射硬件 PCI BAR，因此不支持 PCI P2P DMA。
直通 PCI 设备出现以下警告并非缺陷。

.. code-block:: none

    qemu-system-x86_64: warning: IOMMU_IOAS_MAP failed: Bad address, PCI BAR?
    qemu-system-x86_64: vfio_container_dma_map(0x560cb6cb1620, 0xe000000021000, 0x3000, 0x7f32ed55c000) = -14 (Bad address)

mdev 的 FD 传递
---------------

vfio-pci 根据 sysfsdev 判断后端是否为 mdev。
传入 FD 时无法得知，mdev 会被当成真实 PCI 设备。
若此时为 mdev 启用 RAM 丢弃，会出现以下错误。

.. code-block:: none

    qemu-system-x86_64: -device vfio-pci,iommufd=iommufd0,x-balloon-allowed=on,fd=9: vfio VFIO_FD9: x-balloon-allowed only potentially compatible with mdev devices

vfio-ap 和 vfio-ccw 没有此问题，因为后端始终是 mdev，且强制启用 RAM 丢弃。

与启用 fsts=on 的 intel_iommu 配合
----------------------------------

intel_iommu 配置 fsts=on 时，只支持 IOMMUFD 后端。
旧容器的 VFIO 设备会报错：

.. code-block:: none

    qemu-system-x86_64: -device vfio-pci,host=0000:02:00.0: vfio 0000:02:00.0: Failed to set vIOMMU: Need IOMMUFD backend when fsts=on

不支持 PCI 桥下的 VFIO 设备，必要时应使用 PCIe 桥，否则报错：

.. code-block:: none

    qemu-system-x86_64: -device vfio-pci,host=0000:02:00.0,bus=bridge1,iommufd=iommufd0: vfio 0000:02:00.0: Failed to set vIOMMU: Host device downstream to a PCI bridge is unsupported when fsts=on

如果主机 IOMMU 存在 ERRATA_772415_SPR17，
不支持以 intel_iommu=on,sm_off 运行客户机，
也不支持通过 kexec 或重启从 sm_on 切换到 sm_off。
客户机不需要可扩展模式时，可按如下方式关闭：

.. code-block:: bash

    -device intel-iommu,scalable-mode=off
