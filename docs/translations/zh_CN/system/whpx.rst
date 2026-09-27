Windows Hypervisor Platform
===========================

Windows Hypervisor Platform 是第三方虚拟机监控程序使用 Hyper-V 硬件加速
的 Windows API。

它建立在 Vid 上，后者与 Linux mshv 驱动使用同一组 hypercall。

QEMU 对应后端名为 WHPX，支持 x86_64 和 arm64 Windows 硬件加速。

前提条件
-------------

必须安装 Windows Hypervisor Platform 功能。

安装
^^^^^^^^^^^^
客户端 Windows 使用 Windows 功能（optionalfeatures.exe），服务器版可以
通过服务器管理器的基于功能安装。

也可使用命令行：
``DISM /online /Enable-Feature /FeatureName:HypervisorPlatform /All``

最低系统版本
^^^^^^^^^^^^^^^^^^

x86_64 从 Windows 10 2004 开始经过测试，更早版本可能可用，但未验证。

arm64 最低要求 Windows 11 24H2，并安装 2025 年 4 月可选更新或 5 月安全更新。

更早的 ARM64 24H2 提供预发布 API，QEMU 不支持。

快速开始
-----------

x86_64 使用 WHPX::

    $ qemu-system-x86_64.exe -accel whpx -M pc \
        -smp cores=2 -m 2G -device ich9-usb-ehci1 \
        -device usb-tablet -hda OS.qcow2


arm64 使用 WHPX::

    $ qemu-system-aarch64.exe -accel whpx -M virt \
        -cpu host -smp cores=2 -m 2G \
        -bios edk2-aarch64-code.fd \
        -device ramfb -device nec-usb-xhci \
        -device usb-kbd -device usb-tablet \
        -hda OS.qcow2


arm64 非 Windows 客户机可用 virtio-gpu-pci，相比 ramfb 功能更多，但它不
兼容 Windows UEFI GOP，因为后者要求线性帧缓冲。

加速器选项
--------------------

x86_64
^^^^^^

``-trace whpx_unsupported_msr_access`` 记录对未文档化 MSR 的访问。

``-d invalid_mem`` 跟踪未映射客户机物理地址的访问。

``-accel whpx,ssd=off`` 禁用独立安全域，即客户机进出时的 BTB 刷新。
这显著提高 MMIO 性能，但削弱安全缓解措施。

x86_64 已知问题
----------------------

使用传统 VGA 模式的客户机
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

QEMU 不直接映射帧缓冲内存的 VGA 模式，性能会很差。

可改用较新图形模式，或用 TCG 运行受影响客户机。

用 MMX、SSE、AVX 访问 MMIO 的客户机
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

target/i386/emulate 当前不支持这些指令访问 MMIO 区域。

MMX 会报告 Unimplemented handler，更新的指令则解码失败。

Windows 10 的 PIC 中断
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

使用 Hyper-V 中断控制器时，传统 PIC 中断不能唤醒 HLT 状态客户机。

因此 Windows 10 默认禁用 Hyper-V 中断控制器。可用 -M q35,pic=off 禁用
PIC 并启用它，此配置推荐 UEFI。

正常使用时不应手动设置 -M kernel-irqchip=，该选项仅保留用于调试。

Windows 11 已知问题
^^^^^^^^^^^^^^^^^^^^^^^^^^

arm64
-----

ISA 功能支持
^^^^^^^^^^^^^^^^^^^

目前不支持 SVE 和 SME。
