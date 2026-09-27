.. _riscv-iommu:

RISC-V 机器的 IOMMU 支持
================================================

QEMU 基于 RISC-V IOMMU 1.0 规范 `iommu1.0.0`_ 实现仿真。

包括 PCI 参考设备 riscv-iommu-pci 和平台总线设备 riscv-iommu-sys，
供 QEMU RISC-V 开发板使用。virt 机器兼容两者。

riscv-iommu-pci 参考设备
----------------------------------------

按 `iommu1.0.0`_ 的“Integrating an IOMMU as a PCIe device”一节实现：
PCI 基类 08h、子类 06h、编程接口 00h。

作为参考设备，不实现规范之外的内容，使用 QEMU 的通用默认 PCI ID 1b36:0014。

在 virt 机器中添加：

.. code-block:: bash


  $ qemu-system-riscv64 -M virt -device riscv-iommu-pci,[optional_pci_opts] (...)

这会按照额外 PCI 参数（如总线地址）添加设备。
RISC-V IOMMU 的行为由规范定义，但其操作方式取决于操作系统。

Linux 内核 v6.13 合入 IOMMU 支持，主线内核可使用 QEMU 仿真实现基本 PCIe IOMMU 功能。

截至 v6.17，尚不支持 VFIO 直通等功能。有一个尚未合入的 `VFIO`_ RFC 系列，
可使用 Ventana Micro Systems 公开内核仓库 `ventana-linux`_ 测试 VFIO。

v6.13+ 驱动通过 IOMMU 为系统中所有符合条件的卡创建 IOMMU 组，
不受命令行中设备添加顺序等因素影响。

因此，对当前内核驱动而言，以下命令等效：

.. code-block:: bash


  $ qemu-system-riscv64 \
        -M virt,aia=aplic-imsic,aia-guests=5 \
        -device riscv-iommu-pci,addr=1.0 \
        -device e1000e,netdev=net1 -netdev user,id=net1,net=192.168.0.0/24 \
        -device e1000e,netdev=net2 -netdev user,id=net2,net=192.168.200.0/24 \
        (...)

  $ qemu-system-riscv64 \
        -M virt,aia=aplic-imsic,aia-guests=5 \
        -device e1000e,netdev=net1 -netdev user,id=net1,net=192.168.0.0/24 \
        -device e1000e,netdev=net2 -netdev user,id=net2,net=192.168.200.0/24 \
        -device riscv-iommu-pci,addr=3.0 \
        (...)

两者都会为两个 e1000e 创建 IOMMU 组。

以下选项控制设备能力：

- bus：IOMMU 所用总线。
- ioatc-limit：地址转换缓存大小，默认 2Mb。
- intremap：启用／禁用 MSI。
- ats：启用 ATS。
- off：复位后的转换模式；on 禁用 DMA，off 为 BARE 直通。
- s-stage：启用 s-stage。
- g-stage：启用 g-stage。
- hpm-counters：硬件性能计数器数量，最大和默认均为 31，设 0 禁用 HPM。
- vendor-id/device-id：PCI 设备 ID，默认 1b36:0014（Redhat）。

riscv-iommu-sys 设备
----------------------------------------

以平台总线设备形式实现 RISC-V IOMMU，供 RISC-V 开发板使用。

virt 默认禁用，通过 iommu-sys 机器选项启用：

.. code-block:: bash


  $ qemu-system-riscv64 -M virt,iommu-sys=on (...)

virt 不提供通过 QEMU 命令行配置此设备能力的选项，固定使用以下设置：

- ioatc-limit：默认 2Mb。
- intremap：启用。
- ats：启用。
- off：on，禁用 DMA。
- s-stage：启用。
- g-stage：启用。

.. _iommu1.0.0: https://github.com/riscv-non-isa/riscv-iommu/releases/download/v1.0.0/riscv-iommu.pdf

.. _VFIO: https://lore.kernel.org/linux-riscv/20241114161845.502027-17-ajones@ventanamicro.com/

.. _ventana-linux: https://github.com/ventanamicro/linux/tree/dev-upstream
