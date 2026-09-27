=============
zoned-storage
=============

分区块设备（ZBD）将 LBA 空间划分为称为 zone 的块区域，其大小大于单个 LBA。
它们可以只允许顺序写入，从而降低 SSD 写放大，并可能提高吞吐量和容量。
更多信息见：

https://zonedstorage.io/docs/introduction/zoned-storage

1. 分区存储的块层 API
---------------------
QEMU 块层支持三种模型：

- BLK_Z_HM：主机管理模型仅允许对 zone 顺序写入，
  支持主机管理设备 zone 所需的 ZBD 专用 I/O 命令。
- BLK_Z_HA：主机感知模型允许 zone 内随机写入，因此向后兼容普通块设备。
- BLK_Z_NONE：非分区模型，不支持 zone。
  包括普通设备和驱动器管理的 ZBD，不支持 ZBD 专用 I/O 命令。

块设备信息保存在 BlockDriverState 中。
块层处理 I/O 时持续访问 BlockLimits 结构（BlockDriverState::bl）。
BlockBackend 有一个指向 BlockDriverState 图根部的指针，
例如 file-posix 上叠加 raw 格式。
分区存储信息可从叶子 BlockDriverState 一直向上传播到 BlockBackend。
如果 file-posix 模型设为 BLK_Z_HM，块驱动就声明支持主机分区设备。

块层 API 支持这些设备所需命令，包括报告 zone、四种 zone 操作和 zone append。

2. 模拟分区存储控制器
---------------------
当 BlockBackend 的 BlockLimits 模型报告分区存储设备时，
virtio-blk 模拟器或 qemu-io-cmds.c 工具等使用方，
可通过块层 API 执行分区存储模拟或测试。

例如，通过 qemu-io 测试 null_blk 设备的 zone_report::

  $ path/to/qemu-io --image-opts -n driver=host_device,filename=/dev/nullb0 -c "zrp offset nr_zones"

通过 virtio-blk 暴露主机分区块设备，可使用包含 -device 的命令行::

  -blockdev node-name=drive0,driver=host_device,filename=/dev/nullb0,cache.direct=on \
  -device virtio-blk-pci,drive=drive0

也可只使用 -drive 参数::

  -driver driver=host_device,file=/dev/nullb0,if=virtio,cache.direct=on

QEMU 还可通过以下方式支持分区存储：

(1) virtio-scsi：--device scsi-block 可直通 SCSI ZBC 设备，
    将 ZBC 或 ZAC 硬盘连接到 QEMU。
(2) PCI 直通：NVMe ZNS 模拟可用于测试，但尚不能直通主机分区设备。
    要将 NVMe ZNS 设备交给客户机，可用 VFIO PCI 直通整个 NVMe PCI 适配器。
    同样，也可直通硬盘 HBA，让 QEMU 访问其连接的全部硬盘。
