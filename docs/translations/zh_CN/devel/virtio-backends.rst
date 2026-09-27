..
   Copyright (c) 2022, Linaro Limited
   Written by Alex Bennée

为 QEMU 编写 VirtIO 后端
========================

本文概述开发者编写 QEMU 设备模拟所需的知识，重点是 VirtIO。
前端指客户机内运行的驱动，后端指实现 VirtIO 设备模拟所需的全部工作。
后端可完全在 QEMU 中实现，也可由 QEMU 和内核分担（vhost），
或由 QEMU 配置的独立进程处理（vhost-user）。

VirtIO 传输
-----------

VirtIO 支持多种传输方式。设备配置和操作细节通常相同，
但 QEMU 根据传输方式将其表示为不同设备。
例如 -device virtio-foo 使用 MMIO，-device virtio-foo-pci
则是同类设备使用 PCI 传输。

使用 QEMU 对象模型（QOM）
-------------------------

QEMU 设备通常位于 TYPE_DEVICE 类层级中。
VirtIO 设备应基于派生自该基类的 TYPE_VIRTIO_DEVICE，例如：

.. code:: c

  static const TypeInfo virtio_blk_info = {
      .name = TYPE_VIRTIO_BLK,
      .parent = TYPE_VIRTIO_DEVICE,
      .instance_size = sizeof(VirtIOBlock),
      .instance_init = virtio_blk_instance_init,
      .class_init = virtio_blk_class_init,
  };

也可以建立更丰富的类层级，支持多个设备类型。例如 VirtIO GPU：

.. code:: c

  static const TypeInfo virtio_gpu_base_info = {
      .name = TYPE_VIRTIO_GPU_BASE,
      .parent = TYPE_VIRTIO_DEVICE,
      .instance_size = sizeof(VirtIOGPUBase),
      .class_size = sizeof(VirtIOGPUBaseClass),
      .class_init = virtio_gpu_base_class_init,
      .abstract = true
  };

  static const TypeInfo vhost_user_gpu_info = {
      .name = TYPE_VHOST_USER_GPU,
      .parent = TYPE_VIRTIO_GPU_BASE,
      .instance_size = sizeof(VhostUserGPU),
      .instance_init = vhost_user_gpu_instance_init,
      .instance_finalize = vhost_user_gpu_instance_finalize,
      .class_init = vhost_user_gpu_class_init,
  };

  static const TypeInfo virtio_gpu_info = {
      .name = TYPE_VIRTIO_GPU,
      .parent = TYPE_VIRTIO_GPU_BASE,
      .instance_size = sizeof(VirtIOGPU),
      .class_size = sizeof(VirtIOGPUClass),
      .class_init = virtio_gpu_class_init,
  };

先定义 VirtIO GPU 基类，再派生内部实现和 vhost-user 两个版本。

VirtIOPCIProxy
^^^^^^^^^^^^^^

[AJB：以下是推测，欢迎更了解情况的人补充。]

可能出于 QOM 出现前的历史原因，PCI VirtIO 设备不遵循普通层级。
独立对象基于 VirtIOPCIProxy，而具体 VirtIO 实例由手动创建：

.. code:: c

  /*
   * virtio-blk-pci: This extends VirtioPCIProxy.
   */
  #define TYPE_VIRTIO_BLK_PCI "virtio-blk-pci-base"
  DECLARE_INSTANCE_CHECKER(VirtIOBlkPCI, VIRTIO_BLK_PCI,
                           TYPE_VIRTIO_BLK_PCI)

  struct VirtIOBlkPCI {
      VirtIOPCIProxy parent_obj;
      VirtIOBlock vdev;
  };

  static const Property virtio_blk_pci_properties[] = {
      DEFINE_PROP_UINT32("class", VirtIOPCIProxy, class_code, 0),
      DEFINE_PROP_BIT("ioeventfd", VirtIOPCIProxy, flags,
                      VIRTIO_PCI_FLAG_USE_IOEVENTFD_BIT, true),
      DEFINE_PROP_UINT32("vectors", VirtIOPCIProxy, nvectors,
                         DEV_NVECTORS_UNSPECIFIED),
  };

  static void virtio_blk_pci_realize(VirtIOPCIProxy *vpci_dev, Error **errp)
  {
      VirtIOBlkPCI *dev = VIRTIO_BLK_PCI(vpci_dev);
      DeviceState *vdev = DEVICE(&dev->vdev);

      ...

      qdev_realize(vdev, BUS(&vpci_dev->bus), errp);
  }

  static void virtio_blk_pci_class_init(ObjectClass *klass, const void *data)
  {
      DeviceClass *dc = DEVICE_CLASS(klass);
      VirtioPCIClass *k = VIRTIO_PCI_CLASS(klass);
      PCIDeviceClass *pcidev_k = PCI_DEVICE_CLASS(klass);

      set_bit(DEVICE_CATEGORY_STORAGE, dc->categories);
      device_class_set_props(dc, virtio_blk_pci_properties);
      k->realize = virtio_blk_pci_realize;
      pcidev_k->vendor_id = PCI_VENDOR_ID_REDHAT_QUMRANET;
      pcidev_k->device_id = PCI_DEVICE_ID_VIRTIO_BLOCK;
      pcidev_k->revision = VIRTIO_PCI_ABI_VERSION;
      pcidev_k->class_id = PCI_CLASS_STORAGE_SCSI;
  }

  static void virtio_blk_pci_instance_init(Object *obj)
  {
      VirtIOBlkPCI *dev = VIRTIO_BLK_PCI(obj);

      virtio_instance_init_common(obj, &dev->vdev, sizeof(dev->vdev),
                                  TYPE_VIRTIO_BLK);
      object_property_add_alias(obj, "bootindex", OBJECT(&dev->vdev),
                                "bootindex");
  }

  static const VirtioPCIDeviceTypeInfo virtio_blk_pci_info = {
      .base_name              = TYPE_VIRTIO_BLK_PCI,
      .generic_name           = "virtio-blk-pci",
      .transitional_name      = "virtio-blk-pci-transitional",
      .non_transitional_name  = "virtio-blk-pci-non-transitional",
      .instance_size = sizeof(VirtIOBlkPCI),
      .instance_init = virtio_blk_pci_instance_init,
      .class_init    = virtio_blk_pci_class_init,
  };

可以看到 instance_init 必须手动创建底层 TYPE_VIRTIO_BLOCK 对象，
并把其中一个属性以别名连接到 PCI 设备。


后端实现
--------

后端可以在以下位置实现：

* QEMU 自身；
* 主机内核，即 vhost；
* 独立进程，即 vhost-user。

vhost_ops 与 TYPE_VHOST_USER_BACKEND
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

vhost 代码有两种实现方式。
需要同时兼容 vhost 和 vhost-user 的代码，多数通过 vhost_dev_init() 创建相应后端，
因此需在主对象结构中包含 struct vhost_dev。

vhost-user 设备还需跟踪 chardev 的初始化，
该设备承载 QEMU 与外部 vhost-user 进程之间的控制套接字。

如果只需要 vhost-user 后端，也可以使用其 QOM 化实现。

.. code:: c

  static void
  vhost_user_gpu_instance_init(Object *obj)
  {
      VhostUserGPU *g = VHOST_USER_GPU(obj);

      g->vhost = VHOST_USER_BACKEND(object_new(TYPE_VHOST_USER_BACKEND));
      object_property_add_alias(obj, "chardev",
                                OBJECT(g->vhost), "chardev");
  }

  static const TypeInfo vhost_user_gpu_info = {
      .name = TYPE_VHOST_USER_GPU,
      .parent = TYPE_VIRTIO_GPU_BASE,
      .instance_size = sizeof(VhostUserGPU),
      .instance_init = vhost_user_gpu_instance_init,
      .instance_finalize = vhost_user_gpu_instance_finalize,
      .class_init = vhost_user_gpu_class_init,
  };

这种方式需要在核心对象结构中添加 struct VhostUserBackend，并手动创建后端。
该子结构同时跟踪连接所需的 vhost_dev 和 CharDev。
应调用 vhost_user_backend_dev_init，而不是 vhost_dev_init，
由它代为完成所需工作。
