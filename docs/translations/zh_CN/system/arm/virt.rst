.. _arm-virt:

'virt' 通用虚拟平台（``virt``）
==========================================

``virt`` 板是专为虚拟机设计的平台，不对应任何真实硬件。如果只想运行 Linux 等客户机，而不需要重现某款真实硬件的特殊行为和限制，建议使用此板类型。

这是一个“版本化”的板级模型。因此，除了 ``virt`` 机器类型本身（在不同 QEMU 版本间可能包含改进、错误修复和其他小变化）之外，还提供保证与旧版 QEMU 行为相同的版本，以支持跨 QEMU 版本迁移虚拟机。例如，``virt-5.0`` 机器类型的行为与 QEMU 5.0 发布版的 ``virt`` 机器相同，5.0 发布版的 ``virt-5.0`` 与 5.1 发布版的 ``virt-5.0`` 之间应当可以迁移。对于不带版本的 ``virt`` 机器类型，不保证不同 QEMU 发布版之间可以迁移。

使用 ``-cpu max`` 时，不保证虚拟机可迁移，因为支持的功能可能在不同 QEMU 版本间变化。为确保虚拟机可迁移，建议改用其他 CPU 模型。

支持的设备
"""""""""""""""""

virt 板支持：

- PCI/PCIe 设备
- CXL 固定内存窗口、根桥和设备
- 闪存
- 用于非安全世界的一个或两个 PL011 UART
- 一个 RTC
- 允许客户机从 QEMU 获取数据的 fw_cfg 设备
- 一个 PL061 GPIO 控制器
- 可选的全机 SMMUv3 IOMMU
- 用户可创建的 SMMUv3 设备（示例见下文）
- 可选的 SBSA 通用看门狗定时器（见下文）
- 可热插拔的 DIMM
- 可热插拔的 NVDIMM
- MSI 控制器（GICv2m 或 ITS）
  - 使用 GICv3 时，如果平台提供 ITS，则默认选择 ITS。
  - 使用 GICv2 时，默认提供 GICv2m。
  - GICv3 平台不提供 ITS 时，默认提供 GICv2m。
- 32 个 virtio-mmio 传输设备
- 在 aarch64 硬件上使用 KVM 加速器运行客户机
- 大容量 RAM（至少 255GB，使用 highmem 时可以更多）
- 大量 CPU（使用 GICv3 和 highmem 时最多 512 个）
- 如果 CPU 具有 TrustZone，还提供仅供安全世界使用的设备：

  - 第二个 PL011 UART
  - 第二个 PL061 GPIO 控制器，提供用于触发系统复位或系统关机的 GPIO 线
  - 安全闪存
  - 16MB 安全 RAM

只有在显式配置后端（例如通过第二个 -serial 命令行选项）且未启用 TrustZone 模拟时，才会存在第二个非安全 UART。

支持的客户机 CPU 类型：

- ``cortex-a7`` （32 位）
- ``cortex-a15`` （32 位；默认值）
- ``cortex-a35`` （64 位）
- ``cortex-a53`` （64 位）
- ``cortex-a55`` （64 位）
- ``cortex-a57`` （64 位）
- ``cortex-a72`` （64 位）
- ``cortex-a76`` （64 位）
- ``cortex-a710`` （64 位）
- ``a64fx`` （64 位）
- ``host`` （仅适用于 KVM 和 HVF）
- ``neoverse-n1`` （64 位）
- ``neoverse-v1`` （64 位）
- ``neoverse-n2`` （64 位）
- ``max`` （对于 KVM 和 HVF 与 ``host`` 相同；对于 TCG 则提供尽可能完善的模拟）

注意，默认类型是 ``cortex-a15``，因此 AArch64 客户机必须指定 CPU 类型。

另请注意，传入 ``max`` CPU（即 ``-cpu max``）并不会启用给定 ``virt`` 机器的所有 CPU 功能。如果某项 CPU 架构功能同时要求 CPU 本身和系统其他部分提供支持（例如 MTE），则默认可能不启用它，而是需要通过机器选项启用。

例如，要支持 MTE，除了通过 ``-cpu`` 选项选择支持 MTE 的 CPU（例如 ``max``）之外，还必须使用 ``-machine virt,mte=on`` 启用 MTE 支持。

请参阅下方机器专用选项，或传入 ``help`` 子选项查看指定机器的选项，例如 ``-machine virt-9.0,help``。

此机器支持图形输出，但与 x86 PC 机器类型不同，它默认不启用显示设备：应从 "-device help" 的 Display devices 部分选择设备。建议使用 ``virtio-gpu-pci``，这是唯一能够在 KVM 下正常工作的选项。还可能需要确保客户机内核配置了对应支持，见下文。

机器专用选项
""""""""""""""""""""""""

支持以下机器专用选项：

secure
  设为 ``on``/``off``，启用或禁用实现 Arm 安全扩展（TrustZone）的客户机 CPU 模拟。默认值为 ``off``。

virtualization
  设为 ``on``/``off``，启用或禁用实现 Arm 虚拟化扩展的客户机 CPU 模拟。默认值为 ``off``。

mte
  设为 ``on``/``off``，启用或禁用实现 Arm 内存标记扩展的客户机 CPU 模拟。默认值为 ``off``。

highmem
  设为 ``on``/``off``，启用或禁用将设备和 RAM 放置在超出 32 位的物理地址空间中。对于晚于 ``virt-2.12`` 的机器类型，当 CPU 支持大于 32 位的地址空间时（即 64 位 CPU，或具有大物理地址扩展 LPAE 功能的 32 位 CPU），默认值为 ``on``。如果要在实现 LPAE 的 CPU 类型上启动未启用 ``CONFIG_LPAE`` 的 32 位内核，必须手动将其设为 ``off``；否则，PCI 控制器等部分设备将无法访问。

compact-highmem
  设为 ``on``/``off``，启用或禁用高内存区域的紧凑布局。对于晚于 ``virt-7.2`` 的机器类型，默认值为 ``on``。

highmem-redists
  设为 ``on``/``off``，启用或禁用 GICv3 或 GICv4 重分发器的高内存区域。默认值为 ``on``。设为 ``off`` 会限制使用 GICv3 或 GICv4 时的最大 CPU 数量。

highmem-ecam
  设为 ``on``/``off``，启用或禁用 PCI ECAM 的高内存区域。对于晚于 ``virt-3.0`` 的机器类型，默认值为 ``on``。

highmem-mmio
  设为 ``on``/``off``，启用或禁用 PCI MMIO 的高内存区域。默认值为 ``on``。

highmem-mmio-size
  设置 PCI MMIO 高内存区域的大小。必须为 2 的幂，且大于或等于默认大小（512G）。

gic-version
  指定提供的通用中断控制器（GIC）版本。有效值如下：

  ``2``
    GICv2。注意，此选项将 CPU 数量限制为 8。
  ``3``
    GICv3。最多允许 512 个 CPU。
  ``4``
    GICv4。要求 ``virtualization`` 为 ``on``；最多允许 317 个 CPU。
  ``x-5``
    GICv5（实验性）。这是基于 GICv5 架构规范 EAC 发布版实现的实验性 GICv5 模拟。实验性意味着：

    - 当规范最终版发布且 QEMU 实现它后，客户机可见的行为可能变化
    - 尚未实现迁移支持
    - GICv5 仅通过 DTB 暴露给客户机，不通过 ACPI 表暴露
    - 向客户机暴露中断控制器的方式，以及启用它的命令行语法，可能变化

    当前实现仅支持 EL1 客户机（不支持 EL2、EL3 或 Realm），且未实现 ITS（不支持 MSI）。

    注意，GICv5 是 Armv9 功能，因此启用它会自动禁用除 EL0（用户空间）之外所有异常级别的 AArch32 支持。
  ``host``
    使用 KVM 时，采用主机提供的相同 GIC 版本。
  ``max``
    使用当前可用的最佳 GIC 版本（使用 KVM 时与主机相同；使用 TCG 时，当前在 ``virtualization`` 为 ``off`` 时采用 ``3``，为 ``on`` 时采用 ``4``，但未来可能变化）。

msi
  指定提供的 MSI 和 MSI-X 控制器（GIC）。有效值如下：

  ``auto``
    使用可用的最佳 MSI-X 控制器。支持 ITS 时使用 ITS，否则使用 GICv2m。
  ``gicv2m``
    GICv2m。通常与 GICv2 配合使用，也可与更新的 GIC 配合使用。
  ``its``
    GICv3 ITS。在受支持的加速器上使用 GICv3 或 GICv4 时，这是默认选项。
  ``off``
    禁用 MSI/MSI-X 中断支持。

its
  设为 ``on``/``off``，控制是否实例化 ITS。此选项已弃用，请改用 ``msi``。

iommu
  设置为客户机创建的 IOMMU 类型。有效值如下：

  ``none``
    不创建 IOMMU（默认值）。
  ``smmuv3``
    创建全机 SMMUv3。

default-bus-bypass-iommu
  设为 ``on``/``off``，为默认根总线启用或禁用 `bypass_iommu
  <https://gitlab.com/qemu-project/qemu/-/blob/master/docs/bypass-iommu.txt>`_。

ras
  设为 ``on``/``off``，启用或禁用通过 ACPI 和客户机外部中止异常向客户机报告主机内存错误。默认值为 off。

acpi
  设为 ``on``/``off``/``auto``，启用或禁用 ACPI。

cxl
  设为 ``on``/``off``，启用或禁用 CXL。更多详情见 :doc:`../devices/cxl`。默认值为 off。

cxl-fmw
  CXL 固定内存窗口数组，描述到目标 CXL 主机桥的固定地址路由。参见 :doc:`../devices/cxl`。

dtb-randomness
  设为 ``on``/``off``，控制是否通过客户机 DTB 的 rng-seed 和 kaslr-seed 节点（位于 "/chosen" 和 "/secure-chosen" 中）传递随机种子，供随机数生成器和地址空间随机化等功能使用。默认值为 ``on``。如果可信启动链会验证传入的 DTB，应禁用此选项，因为它会使 DTB 不具有确定性。此时，如果固件需要种子，应自行生成并传递。

dtb-kaslr-seed
  dtb-randomness 的已弃用同义选项。

virtio-mmio-transports
  设置创建的 virtio-mmio 传输数量（0 至 32，默认值为 32）。未使用的传输不会造成问题，但如有需要，可以用此属性避免向客户机暴露它们。

x-oem-id
  设置字符串（最多 6 字节），覆盖 ACPI 表头中 OEMID 字段的默认值。

x-oem-table-id
  设置字符串（最多 8 字节），覆盖 ACPI 表头中 OEM Table ID 字段的默认值。

SMMU 配置
""""""""""""""""""

全机 SMMUv3 IOMMU
  设置机器专用选项 ``iommu=smmuv3``，会使 QEMU 创建一个全机 SMMUv3 实例，作用于 PCIe 拓扑中的所有设备。

  关于选择性绕过设备的信息，请参阅 ``docs/bypass-iommu.txt``。

用户可创建的 SMMUv3 设备
  可以使用 ``-device arm-smmuv3`` 选项创建多个用户定义的 SMMUv3 设备，每个设备关联一个独立的 PCIe 根复合体。仅在未使用全机 SMMUv3（``iommu=smmuv3``）选项时才允许这样做。每个 ``arm-smmuv3`` 设备使用 ``primary-bus`` 子选项指定其关联的 PCIe 根复合体。

  当希望复现主机配置时，此模型很有用：主机上的每个 NUMA 节点通常具有自己的 SMMU，因此这种方式使虚拟机拓扑更接近主机硬件布局。

  示例::

      -device arm-smmuv3,primary-bus=pcie.0,id=smmuv3.0
      ...
      -device pxb-pcie,id=pcie.1,numa_node=1
      -device arm-smmuv3,primary-bus=pcie.1,id=smmuv3.1


  *加速的 SMMUv3（嵌套地址转换）*

  ``accel=on`` 选项为 vfio-pci 直通设备启用硬件加速的嵌套地址转换。在此模式下，客户机 SMMU 驱动配置自己的 Stage-1 页表，主机 SMMUv3 则在硬件中同时处理 Stage-1（客户机）和 Stage-2（主机）转换。主机 SMMUv3 必须支持嵌套转换。此模式要求 iommufd 后端，且仅支持通过 ACPI 启动（不支持设备树）。

  当 ``accel=on`` 时，除非显式设置，QEMU 会根据主机 SMMUv3 的能力自动推导 ``ril``、``ats``、``oas``、``ssidsize`` 和 ``cmdqv`` 子选项的值。

  示例::

      -device arm-smmuv3,primary-bus=pcie.0,id=smmuv3.0,accel=on


  *加速的 SMMUv3 命令队列（Tegra241 CMDQV）*

  ``cmdqv`` 子选项在支持的主机上启用 NVIDIA Tegra241 命令队列虚拟化（CMDQV）。启用 CMDQV 后，每个加速的 ``arm-smmuv3`` 设备获得专用硬件命令队列，客户机直接向真实硬件发出 SMMU 失效命令，绕过 QEMU，提高大量发出失效命令的工作负载的吞吐量。不使用此功能时，每条失效命令都会陷入 QEMU。

SBSA 通用看门狗
"""""""""""""""""""""

可以使用 ``-device sbsa-gwdt`` 向 virt 机器添加 SBSA 通用看门狗定时器（GWDT）。它仅受 virt 机器支持；该机器通过专用插入处理函数连接静态分配的 MMIO 区域和 IRQ。

有两种模式可用：

原生模式（默认）
  看门狗通过 ACPI GTDT 表和 FDT 描述，使用系统计数器频率。示例::

    -device sbsa-gwdt


WDAT 模式
  看门狗通过 ACPI WDAT 表描述（没有 FDT 节点），使用 1 kHz 定时器频率。WDAT 和 GTDT 看门狗条目互斥。示例::

    -device sbsa-gwdt,wdat=on


Linux 客户机内核配置
""""""""""""""""""""""""""""""""

Linux arm 和 arm64 内核的 'defconfig' 应当包含适用于 virtio 和 PCI 控制器的设备驱动；但某些旧版内核，尤其是 32 位 Arm 内核，默认没有启用所有所需功能。如果没有看到预期的 PCI 设备，请检查客户机配置中是否包含::

  CONFIG_PCI=y
  CONFIG_VIRTIO_PCI=y
  CONFIG_PCI_HOST_GENERIC=y


如果要使用 ``virtio-gpu-pci`` 图形设备，还需要::

  CONFIG_DRM=y
  CONFIG_DRM_VIRTIO_GPU=y


用于裸机编程的硬件配置信息
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

``virt`` 板自动生成设备树二进制文件（“dtb”）并将其传递给客户机，其中包含系统中各设备的地址、中断线和其他配置信息。客户机代码可以依赖并硬编码以下地址：

- 闪存起始地址为 0x0000_0000

- RAM 起始地址为 0x4000_0000

关于设备位置的其他所有信息都可能在不同 QEMU 版本间变化，因此客户机代码必须查询 DTB。

QEMU 支持 ``virt`` 的两种客户机镜像启动方式，客户机代码查找 DTB 二进制文件的方法有所不同：

- 对于使用 Linux 内核启动协议的客户机（即通过 QEMU ``-kernel`` 选项传入的任何非 ELF 文件），DTB 地址通过寄存器传递（32 位客户机使用 ``r2``，64 位客户机使用 ``x0``）。

- 对于以“裸机”方式启动的客户机（其他任意启动方式），DTB 位于 RAM 起始位置（0x4000_0000）。
