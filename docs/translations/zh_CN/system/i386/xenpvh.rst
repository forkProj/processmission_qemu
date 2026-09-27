Xen PVH 机器 (``xenpvh``)
=========================================

Xen 的各类客户机对硬件虚拟化、设备模拟和半虚拟化的依赖程度不同。PVH
像 HVM 一样使用硬件虚拟化，但尽量避免设备模拟，改用直通或半虚拟化设备。

此最小机器用于在模拟 PCIe 控制器上提供半虚拟化 virtio 设备。

支持的设备
-----------------

提供以下设备：

- RAM
- GPEX 主桥
- virtio-pci 设备

设计上仅连接 virtio-pci 设备，但理论上，只要 Xen 和客户机支持，
任何兼容的 PCI 设备模型都可以工作。

运行
-------

Xen 工具通常在需要时自动构造命令并启动 QEMU。若需手动操作，可参考：

.. code-block:: console

    qemu-system-i386 -xen-domid 3 -no-shutdown        \
      -chardev socket,id=libxl-cmd,path=/var/run/xen/qmp-libxl-3,server=on,wait=off \
      -object monitor-qmp,id=qmp0,chardev=libxl-cmd   \
      -chardev socket,id=libxenstat-cmd,path=/var/run/xen/qmp-libxenstat-3,server=on,wait=off \
      -object monitor-qmp,id=qmp1,chardev=libxenstat-cmd \
      -nodefaults                                     \
      -no-user-config                                 \
      -xen-attach -name g0                            \
      -vnc none                                       \
      -display none                                   \
      -device virtio-net-pci,id=nic0,netdev=net0,mac=00:16:3e:5c:81:78 \
      -netdev type=tap,id=net0,ifname=vif3.0-emu,br=xenbr0,script=no,downscript=no \
      -smp 4,maxcpus=4                                \
      -nographic                                      \
      -machine xenpvh,ram-low-base=0,ram-low-size=2147483648,ram-high-base=4294967296,ram-high-size=2147483648,pci-ecam-base=824633720832,pci-ecam-size=268435456,pci-mmio-base=4026531840,pci-mmio-size=33554432,pci-mmio-high-base=824902156288,pci-mmio-high-size=68719476736 \
      -m 4096
