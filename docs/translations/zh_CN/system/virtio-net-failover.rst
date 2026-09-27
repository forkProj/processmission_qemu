========================================
QEMU virtio-net 备用设备（net_failover）
========================================

本文介绍用于创建 net_failover 设备对的 virtio-net 备用功能。

设备对由 vfio-pci 和 virtio-net 组成。迁移前移除 vfio，流量转到 virtio-net；
目的端再插入另一个 vfio-pci 接管数据通路。客户机 net_failover 模块将 MAC
相同的网络设备配对。

快速硬件网卡称为主设备，virtio-net 称为备用设备。

限制
------------

目前主设备必须是 PCIe 网卡，未来增强支持后可能放宽 PCIe 限制。用户必须
确保主、备设备不在同一 PCIe 插槽。

使用场景
--------

此功能让直通高速网卡也能方便迁移：迁移期间退回 virtio-net，类似无需客户机
配置的简化 bond。热迁移时 QEMU 通过 PCIe 热插拔处理器移除主设备，流量
转到备用；目的端自动重新插入主设备，net_failover 再将其登记为主设备。

用法
-----

主设备可以热插入，也可在启动配置中提供。

.. code-block:: shell

  -device virtio-net-pci,netdev=hostnet1,id=net1,mac=52:54:00:6f:55:cc,bus=root2,failover=on


failover=on 启用 VIRTIO_NET_F_STANDBY。

.. code-block:: shell

  -device vfio-pci,host=5e:00.2,id=hostdev0,bus=root1,failover_pair_id=net1


failover_pair_id 引用备用 virtio-net 的 ID，仅用于 QEMU 内配对。
客户机 net_failover 仍根据相同 MAC 匹配。

热插拔
-------

主备均可通过监视器热插入。若先插入 virtio-net，会警告找不到主设备。

迁移
---------

该功能增加 wait-unplug 迁移状态。配置中存在故障切换主设备时，迁移进入此
状态，等待客户机完成设备移除，再进入 active。若备用设备已协商该功能位，
目的端会自动热插入主设备。
