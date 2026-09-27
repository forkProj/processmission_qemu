.. SPDX-License-Identifier: GPL-2.0-or-later

=========
vfio-user
=========

QEMU 包含 vfio-user 客户端。该规范允许在 QEMU 外的用户空间实现 PCI 设备，
这与 vhost-user 类似（见 :doc:`virtio/vhost-user`），但不限于 virtio，
可模拟任意 PCI 设备。vfio 由主机内核处理，而 vfio-user 虽实现相似，
完全在用户空间运行。

例如，SPDK 提供虚拟 PCI NVMe 控制器。在 QEMU 与 SPDK 间建立 vfio-user
Unix 套接字后，虚拟机即可向 SPDK 进程发送 NVMe I/O。

假定服务器已在 /tmp/vfio-user.sock 监听，可如下配置设备：

.. code-block:: console

  --device '{"driver": "vfio-user-pci","socket": {"path": "/tmp/vfio-user.sock", "type": "unix"}}'

更多信息见 `libvfio-user <https://github.com/nutanix/libvfio-user/>`_。
