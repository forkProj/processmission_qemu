'microvm' 虚拟平台（``microvm``）
========================================

``microvm`` 是一种受 ``Firecracker`` 启发、按其机器模型构建的机器类型。

它是一种不支持 ``PCI`` 或 ``ACPI`` 的极简机器类型，专为短生命周期客户机设计。microvm 同时针对启动时间和资源占用进行了优化，因此也为 QEMU 和客户机操作系统的基准测试及优化提供了基线。


支持的设备
-----------------

microvm 机器类型支持以下设备：

- ISA 总线
- i8259 PIC（可选）
- i8254 PIT（可选）
- MC146818 RTC（可选）
- 一个 ISA 串口（可选）
- LAPIC
- IOAPIC（默认使用 kernel-irqchip=split）
- kvmclock（使用 KVM 时）
- fw_cfg
- 最多八个 virtio-mmio 设备（由用户配置）


限制
-----------

目前，microvm *不* 支持以下功能：

- 仅支持 PCI 的设备。
- 任何形式的热插拔。
- 跨 QEMU 版本实时迁移。


使用 microvm 机器类型
------------------------------

机器专用选项
~~~~~~~~~~~~~~~~~~~~~~~~

它支持以下机器专用选项：

- microvm.x-option-roms=bool（设为 off 可禁用选件 ROM 加载）
- microvm.pit=OnOffAuto（启用 i8254 PIT）
- microvm.isa-serial=bool（设为 off 可禁用 ISA 串口实例化）
- microvm.pic=OnOffAuto（启用 i8259 PIC）
- microvm.rtc=OnOffAuto（启用 MC146818 RTC）
- microvm.auto-kernel-cmdline=bool（设为 off 可禁用向内核命令行添加 virtio-mmio 设备）


启动选项
~~~~~~~~~~~~

默认情况下，microvm 使用 ``qboot`` 作为 BIOS，以缩短启动时间，但它也兼容 ``SeaBIOS``。

由于目前没有固件能够从以 ``virtio-mmio`` 为传输方式的块设备启动，因此基于 microvm 的虚拟机需要使用主机端内核运行，并可选择提供 initrd 镜像。


运行基于 microvm 的虚拟机
~~~~~~~~~~~~~~~~~~~~~~~~~~

默认情况下，microvm 以最大兼容性为目标，同时启用传统设备和非传统设备。以下示例未传递任何额外的机器专用选项，创建的虚拟机使用传统 ``ISA serial`` 设备作为控制台::

  $ qemu-system-x86_64 -M microvm \
     -enable-kvm -cpu host -m 512m -smp 2 \
     -kernel vmlinux -append "earlyprintk=ttyS0 console=ttyS0 root=/dev/vda" \
     -nodefaults -no-user-config -nographic \
     -serial stdio \
     -drive id=test,file=test.img,format=raw,if=none \
     -device virtio-blk-device,drive=test \
     -netdev tap,id=tap0,script=no,downscript=no \
     -device virtio-net-device,netdev=tap0


虽然上述示例可以工作，但也可以通过禁用部分传统设备进一步减少资源占用。如果使用 ``KVM``，可以禁用 ``RTC``，让客户机完全依赖 ``kvmclock``。此外，如果主机 CPU 具有 ``TSC_DEADLINE`` 功能，还可以同时禁用 i8259 PIC 和 i8254 PIT（务必确保为客户机模拟的 CPU 也具有此功能）。

以下示例禁用了虚拟机中所有可选的传统功能::

  $ qemu-system-x86_64 \
     -M microvm,x-option-roms=off,pit=off,pic=off,isa-serial=off,rtc=off \
     -enable-kvm -cpu host -m 512m -smp 2 \
     -kernel vmlinux -append "console=hvc0 root=/dev/vda" \
     -nodefaults -no-user-config -nographic \
     -chardev stdio,id=virtiocon0 \
     -device virtio-serial-device \
     -device virtconsole,chardev=virtiocon0 \
     -drive id=test,file=test.img,format=raw,if=none \
     -device virtio-blk-device,drive=test \
     -netdev tap,id=tap0,script=no,downscript=no \
     -device virtio-net-device,netdev=tap0



触发由客户机发起的关机
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

由于 microvm 机器类型仅包含少量系统设备，某些 x86 重启或关机机制，例如向键盘发送按键序列或写入 ACPI 寄存器，在虚拟机中不会产生任何效果。

触发客户机主动关机的推荐方式是产生 ``triple-fault``，这会使虚拟机发起重启。如果命令行还包含 ``-no-reboot`` 参数，QEMU 会检测到此事件并正常终止自身执行。

Linux 支持此机制，但默认只在其他方式均尝试失败后才使用，因此会使重启延迟数秒。可以在内核命令行中添加 ``reboot=t``，指示内核首先尝试三重故障机制。
