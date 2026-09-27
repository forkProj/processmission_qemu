===============
Virtio 设备迁移
===============

Copyright 2015 IBM Corp.

This work is licensed under the terms of the GNU GPL, version 2 or later.  See
the COPYING file in the top-level directory.

保存和恢复 virtio 设备状态较为复杂，原因有几方面：

- 状态分散在多个部分：

  - virtio 核心：功能、队列数等通用字段。

  - virtio 传输（pci、ccw 等）：各代理设备及传输专用状态，
    例如 MSI-X 向量、指示器。

  - virtio 设备（net、blk 等）：各设备类型及其状态，例如 MAC 地址、请求队列。

- 大多数字段通过流接口保存，后来又增加子节以支持跨版本迁移。

本文记录当前流程，并说明需要注意的问题。

保存状态流程
============

::

  virtio core               virtio transport          virtio device
  -----------               ----------------          -------------

                                                      save() function registered
                                                      via VMState wrapper on
                                                      device class
  virtio_save()                                       <----------
               ------>      save_config()
                            - save proxy device
                            - save transport-specific
                              device fields
  - save common device
    fields
  - save common virtqueue
    fields
               ------>      save_queue()
                            - save transport-specific
                              virtqueue fields
               ------>                               save_device()
                                                     - save device-specific
                                                       fields
  - save subsections
    - device endianness,
      if changed from
      default endianness
    - 64 bit features, if
      any high feature bit
      is set
    - virtio-1 virtqueue
      fields, if VERSION_1
      is set

加载状态流程
============

::

  virtio core               virtio transport          virtio device
  -----------               ----------------          -------------

                                                      load() function registered
                                                      via VMState wrapper on
                                                      device class
  virtio_load()                                       <----------
               ------>      load_config()
                            - load proxy device
                            - load transport-specific
                              device fields
  - load common device
    fields
  - load common virtqueue
    fields
               ------>      load_queue()
                            - load transport-specific
                              virtqueue fields
  - notify guest
               ------>                               load_device()
                                                     - load device-specific
                                                       fields
  - load subsections
    - device endianness
    - 64 bit features
    - virtio-1 virtqueue
      fields
  - sanitize endianness
  - sanitize features
  - virtqueue index sanity
    check
                                                     - feature-dependent setup

这种设计的影响
==============

加载状态时必须谨慎：核心在子节加载之前调用 load_device()。
因此，依赖子节传输信息的代码，例如依赖功能位的代码，
必须在设备 load() 中、virtio_load() 返回之后调用。

为保持兼容，迁移状态的扩展应通过向核心添加子节完成。
如果扩展传输或设备专用状态，核心需从新子节调用对应回调。
