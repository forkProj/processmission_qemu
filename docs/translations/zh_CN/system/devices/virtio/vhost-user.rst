.. _vhost_user:

vhost-user 后端
--------------------

vhost-user 让 VirtIO 请求在 QEMU 之外处理，需要以下组成部分。

vhost-user 设备
=================

这些简单的桩设备让客户机能看到 VirtIO，代码大多为样板。各设备通过 chardev
选项引用 --chardev 的 ID，由其套接字连接 vhost-user 守护进程。

各设备都有 virtio-mmio 和 virtio-pci 变体，按平台选择总线。

.. list-table:: vhost-user 设备
  :widths: 20 20 60
  :header-rows: 1

  * - 设备
    - 类型
    - 说明
  * - vhost-user-blk
    - 块存储
    - :ref:`storage-daemon`
  * - vhost-user-fs
    - 文件存储驱动
    - `virtiofsd <https://gitlab.com/virtio-fs/virtiofsd>`_
  * - vhost-user-gpio
    - 将 GPIO 引脚代理到主机
    - `vhost-device-gpio <https://github.com/rust-vmm/vhost-device/tree/main/vhost-device-gpio>`_
  * - vhost-user-gpu
    - GPU 驱动
    - `vhost-device-gpu <https://github.com/rust-vmm/vhost-device/tree/main/vhost-device-gpu>`_ 或 :ref:`vhost_user_gpu`
  * - vhost-user-i2c
    - 将 I2C 设备代理到主机
    - `vhost-device-i2c <https://github.com/rust-vmm/vhost-device/tree/main/vhost-device-i2c>`_
  * - vhost-user-input
    - 通用输入驱动
    - `vhost-device-input <https://github.com/rust-vmm/vhost-device/tree/main/vhost-device-input>`_ 或 :ref:`vhost_user_input`
  * - vhost-user-rng
    - 熵驱动
    - `vhost-device-rng <https://github.com/rust-vmm/vhost-device/tree/main/vhost-device-rng>`_
  * - vhost-user-scmi
    - 系统控制和管理接口
    - `vhost-device-scmi <https://github.com/rust-vmm/vhost-device/tree/main/vhost-device-scmi>`_
  * - vhost-user-snd
    - 音频设备
    - `vhost-device-sound <https://github.com/rust-vmm/vhost-device/tree/main/vhost-device-sound>`_
  * - vhost-user-scsi
    - SCSI 存储
    - :ref:`vhost_user_scsi`
  * - vhost-user-vsock
    - 套接字通信
    - `vhost-device-vsock <https://github.com/rust-vmm/vhost-device/tree/main/vhost-device-vsock>`_
  * - vhost-user-spi
    - 将 SPI 设备代理到主机
    - `vhost-device-spi <https://github.com/rust-vmm/vhost-device/tree/main/vhost-device-spi>`_
  * - vhost-user-rtc
    - 实时时钟
    - `vhost-device-rtc <https://github.com/rust-vmm/vhost-device/tree/main/vhost-device-rtc>`_

列表并不穷尽，任何实现对应设备且符合协议的后端都应可用。

vhost-user-test-device
^^^^^^^^^^^^^^^^^^^^^^

这是供专家开发新后端的通用设备，用户需指定全部参数，包括：

  - 设备 virtio-id
  - 所需 num_vqs 和 vq_size
  - 如需配置空间，指定 config_size

.. note::
  适合开发，但不推荐生产使用。

vhost-user 守护进程
===================

独立进程通过套接字按 :ref:`vhost_user_proto` 与 QEMU 连接。项目可按配置
构建若干守护进程，也可使用任何满足对应设备规范的实现。

.. _shared_memory_object:

共享内存对象
====================

守护进程需访问客户机地址空间中的 VirtIO 队列，通过 memory-backend-file、
memory-backend-memfd 或 memory-backend-shm 实现。协议协商时经套接字传入
可访问对象的文件描述符。

目前共享对象大小必须与 -m 定义的主内存一致。

示例
========

先启动守护进程。

.. parsed-literal::

  $ virtio-foo --socket-path=/var/run/foo.sock $OTHER_ARGS


再启动 QEMU，指定设备、chardev 和内存对象。

.. parsed-literal::

  $ |qemu_system| \\
      -m 4096 \\
      -chardev socket,id=ba1,path=/var/run/foo.sock \\
      -device vhost-user-foo,chardev=ba1,$OTHER_ARGS \\
      -object memory-backend-memfd,id=mem,size=4G,share=on \\
      -numa node,memdev=mem \\
        ...
