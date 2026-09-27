.. SPDX-License-Identifier: GPL-2.0-or-later

=============================
使用 mdev mdpy 测试 VFIO 显示
=============================

.. contents:: 目录

内核提供中介设备示例驱动 mdpy（samples/vfio-mdev/mdpy.c），
通过 VFIO 显示区域接口暴露一个虚拟帧缓冲区。
无需真实 GPU，即可测试包括热插拔在内的 VFIO 显示支持。

内核模块
========

mdpy 依赖 mdev 子系统。请启用、构建并加载模块。

最小配置如下::

    CONFIG_SAMPLE_VFIO_MDEV_MDPY=m
    CONFIG_SAMPLE_VFIO_MDEV_MDPY_FB=m   # guest framebuffer driver

CONFIG_VFIO_MDEV 会自动选中。

验证驱动已成功注册：

.. code-block:: bash

    ls /sys/devices/virtual/mdpy/mdpy/mdev_supported_types/

创建 mdev 实例
==============

不同类型对应不同分辨率，例如 mdpy-vga 为 640x480，
mdpy-xga 为 1024x768，mdpy-hd 为 1920x1080。

每个 mdev 实例由 UUID 标识：

.. code-block:: bash

    uuid=$(uuidgen)
    echo "$uuid" > /sys/devices/virtual/mdpy/mdpy/mdev_supported_types/mdpy-xga/create

之后可这样移除实例：

.. code-block:: bash

    echo 1 > /sys/bus/mdev/devices/$uuid/remove

确保用户有访问 VFIO 组所需的权限，例如 chmod 666 /dev/vfio/16。

启动 QEMU
=========

启动时连接
----------

.. code-block:: bash

    qemu-system-x86_64 -machine q35 -m 1G \
        -device vfio-pci,sysfsdev=/sys/bus/mdev/devices/$uuid,display=on \
        -display gtk,gl=on

通过 HMP 热插拔
---------------

启动 QEMU 时添加 PCIe 根端口，PCIe 热插拔需要它，并启用监控器：

.. code-block:: bash

    qemu-system-x86_64 -machine q35 -m 1G \
        -device pcie-root-port,id=rp0,slot=1 \
        -display gtk,gl=on \
        -monitor stdio

然后在 ``(qemu)`` 提示符执行：

.. code-block:: none

    device_add vfio-pci,sysfsdev=/sys/bus/mdev/devices/<uuid>,display=on,bus=rp0,id=mdpy0

热拔出时执行：

.. code-block:: none

    device_del mdpy0
