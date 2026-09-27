Tenstorrent Atlantis（``tt-atlantis``）
=======================================

Tenstorrent Atlantis 平台由 Tenstorrent 与 CoreLab Technology 合作开发，基于 Atlantis SoC，包含 Ascalon-X CPU 以及来自 Tenstorrent 和 CoreLab Technology 的其他 IP。

Tenstorrent Ascalon-X 是一款符合 RVA23 的高性能 64 位 RISC-V CPU。

tt-atlantis QEMU 模型功能
-------------------------------

* 8 核 Ascalon-X CPU 集群
* 符合 RISC-V 的高级中断架构
* 兼容 16550A 的 UART

已知限制
-----------------

QEMU 的 tt-atlantis 机器尚未模拟真实平台上的全部设备。特别是：

* 没有 PCI 主机桥，因此无法连接 virtio-pci 设备。需要块存储的启动方式必须通过 ``-initrd`` 使用 initramfs。
* DesignWare UART 使用 QEMU 兼容 ns16550 的 ``serial_mm`` 设备建模；超出该寄存器集合的 DesignWare 专用寄存器返回 0。

支持的软件
------------------

Tenstorrent Ascalon CPU 不采用专有或非标准扩展，因此通常与现有软件具有良好兼容性。QEMU 的 tt-atlantis 机器可以使用默认配置运行上游 OpenSBI 和 Linux。

开发板硬件需要固件执行一些特定于实现的设置。这部分正在开发中，未来可能成为 tt-atlantis 机器的必需项或可选项。
