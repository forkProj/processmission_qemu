========================================
QEMU 的 PCI ID
========================================

Red Hat, Inc. 将其设备 ID 范围的一部分提供给 QEMU 的虚拟设备使用。
供应商 ID 为 1af4（原 Qumranet ID）和 1b36。

如需为设备分配 ID，请联系 Gerd Hoffmann <kraxel@redhat.com>。

1af4 供应商 ID
------------------------

1000 到 10ff 的设备 ID 用于 virtio-pci，如下所示。
此分配独立于 virtio 规范维护的 virtio 设备 ID。

1af4:1000
  网络设备，传统接口。
1af4:1001
  块设备，传统接口。
1af4:1002
  气球设备，传统接口。
1af4:1003
  控制台设备，传统接口。
1af4:1004
  SCSI 主机总线适配器，传统接口。
1af4:1005
  熵生成设备，传统接口。
1af4:1009
  9p 文件系统设备，传统接口。
1af4:1012
  vsock 设备，用于错误兼容。

1af4:1040 到 1af4:10ef
  现代 virtio 设备范围。PCI 设备 ID 为 virtio 设备 ID 加 0x1040。
  virtio ID 定义于 virtio 规范。Linux 内核的 ``linux/virtio_ids.h``
  定义全部 ID，QEMU 在 ``include/standard-headers/`` 保存副本。

1af4:10f0 到 1a4f:10ff
  可免注册用于实验。代码离开测试实验室时，例如请求合入上游或发布发行版／产品，
  必须获取正式 ID，避免冲突。

1af4:1100
  用作 QEMU 仿真的现有硬件设备的 PCI 子系统 ID。

1af4:1110
  ivshmem 设备，见 :doc:`ivshmem-spec`。

所有其他设备 ID 均保留。

1b36 供应商 ID
------------------------

0000 到 00ff 用于 QEMU 专用、非 virtio 的 PCI 设备：

1b36:0001
  PCI-PCI 桥。
1b36:0002
  PCI 串口（16550A）适配器，见 :doc:`pci-serial`。
1b36:0003
  PCI 双端口 16550A 适配器，见 :doc:`pci-serial`。
1b36:0004
  PCI 四端口 16550A 适配器，见 :doc:`pci-serial`。
1b36:0005
  PCI 测试设备，见 :doc:`pci-testdev`。
1b36:0006
  PCI Rocker 以太网交换机。
1b36:0007
  PCI SD 卡主控制器接口（SDHCI）。
1b36:0008
  PCIe 主桥。
1b36:0009
  PCI 扩展桥，``-device pxb``。
1b36:000a
  PCI-PCI 桥，多席位。
1b36:000b
  PCIe 扩展桥，``-device pxb-pcie``。
1b36:000c
  PCIe 根端口，``-device pcie-root-port``。
1b36:000d
  PCI xHCI USB 主机适配器。
1b36:000e
  PCIe 到 PCI 桥，``-device pcie-pci-bridge``。
1b36:000f
  mdpy，mdev 示例设备，``linux/samples/vfio-mdev/mdpy.c``。
1b36:0010
  PCIe NVMe，``-device nvme``。
1b36:0011
  PCI PVPanic，``-device pvpanic-pci``。
1b36:0012
  PCI ACPI ERST，``-device acpi-erst``。
1b36:0013
  PCI UFS，``-device ufs``。
1b36:0014
  PCI RISC-V IOMMU。

这些设备均记录于 :doc:`index`。

0100 设备 ID 用于 QXL 显卡。
