.. SPDX-License-Identifier: GPL-2.0-or-later

可组合 SR-IOV 设备
========================

SR-IOV（单根 I/O 虚拟化）是 PCI Express 设备的可选扩展能力。它允许一个
物理功能（PF）表现为多个虚拟功能（VF），主要用于消除虚拟机 I/O 的软件开销。

一些设备带有预定义 SR-IOV 配置，也可以自行组合。目前只有 virtio-net-pci
支持组合 SR-IOV 设备。

通过向总线添加 virtio-net-pci 功能，可以配置支持 SR-IOV 的 virtio-net。
命令行示例如下：

.. code-block:: shell

    -netdev user,id=n -netdev user,id=o
    -netdev user,id=p -netdev user,id=q
    -device pcie-root-port,id=b
    -device virtio-net-pci,bus=b,addr=0x0.0x3,netdev=q,sriov-pf=f
    -device virtio-net-pci,bus=b,addr=0x0.0x2,netdev=p,sriov-pf=f
    -device virtio-net-pci,bus=b,addr=0x0.0x1,netdev=o,sriov-pf=f
    -device virtio-net-pci,bus=b,addr=0x0.0x0,netdev=n,id=f

VF 通过 ``sriov-pf`` 指定配对 PF。必须先添加全部 VF，再添加 PF。用户需
确保 VF 功能号大于 PF，且功能号间隔一致。PF 和 VF 都支持 ARI，因此最多
可以有 255 个 VF。

客户机中可能还需额外步骤启用 SR-IOV。Linux 用法参阅 [1]_。

.. [1] https://docs.kernel.org/PCI/pci-iov-howto.html
