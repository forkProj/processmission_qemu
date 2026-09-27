PowerNV 系列板（``powernv8``、``powernv9``、``powernv10``、``powernv11``）
================================================================================

PowerNV（Non-Virtualized，非虚拟化）是使用 OPAL 固件的“裸机”平台，在 IBM 和 OpenPOWER 系统上运行 Linux。它可以作为运行 KVM 客户机的虚拟机监控器操作系统，也可以作为普通主机操作系统。

QEMU 的 PowerNV 机器尝试在 skiboot 固件层面模拟 PowerNV 系统；skiboot 加载操作系统并提供部分运行时服务。Power 系统还有一层更底层的固件 HostBoot，用于执行 DRAM 训练等底层系统初始化。目前这些不在 QEMU 的模拟范围内。

支持的设备
-----------------

 * POWER8、POWER9、Power10 和 Power11 的多处理器支持。
 * XSCOM，用于配置小芯片的串行通信边带总线。
 * 简单 LPC 控制器。
 * 处理器服务接口（PSI）控制器。
 * 中断控制器：XICS（POWER8）、XIVE（POWER9）和 XIVE2（Power10 与 Power11）。
 * POWER8 PHB3 PCIe 主机桥和 POWER9 PHB4 PCIe 主机桥。
 * 简单 OCC，用于电源管理任务的片上微控制器。
 * 处理 BMC 通信的 iBT 设备，可使用 QEMU 提供的内部 BMC 模拟器，也可使用 Aspeed QEMU 机器等外部 BMC。
 * 包含各固件分区的 PNOR。

缺少的设备
---------------

尚缺少许多功能，包括：

 * I2C 控制器（尚待合并）。
 * NPU/NPU2/NPU3 控制器。
 * PCIe 主机桥控制器的 EEH 支持。
 * NX 控制器。
 * VAS 控制器。
 * chipTOD（日历时钟）。
 * 自启动引擎（SBE）。
 * FSI 总线。

固件
--------

用于 OpenPower 系统的 OPAL 固件（OpenPower 抽象层）包含运行时服务 ``skiboot``，以及引导加载内核和 initramfs ``skiroot``。源码位于 `GitHub 上的 OpenPOWER 账号 <https://github.com/open-power>`_。

``skiboot`` 和 ``skiroot`` 的预构建镜像可从 `OpenPOWER <https://github.com/open-power/op-build/releases/>`__ 网站获取。

QEMU 包含 ``skiboot`` 预构建镜像，当模型需要更新版本时会更新它。

当前加速支持情况
---------------------------

Linux Power 主机通过 kvm-hv 和 kvm-pr 模块提供 KVM 加速。kvm-hv 遵循 PAPR，不兼容 powernv。理论上 kvm-pr 可作为有效加速选项，但目前尚不支持。

为避免用户尝试 accel=kvm 时遇到信息不足的错误，powernv 机器会明确报告不支持 KVM。如果未来 kvm-pr（或其他 KVM 方案）可用作此机器的 KVM 加速器，可以重新考虑这一限制。

启动选项
------------

以下是一个使用单个 e1000e 网卡的简单配置：

.. code-block:: bash

  $ qemu-system-ppc64 -m 2G -machine powernv9 -smp 2,cores=2,threads=1 \
  -accel tcg,thread=single \
  -device e1000e,netdev=net0,mac=C0:FF:EE:00:00:02,bus=pcie.0,addr=0x0 \
  -netdev user,id=net0,hostfwd=::20022-:22,hostname=pnv \
  -kernel ./zImage.epapr  \
  -initrd ./rootfs.cpio.xz \
  -nographic


再添加 SATA 磁盘：

.. code-block:: bash

  -device ich9-ahci,id=sata0,bus=pcie.1,addr=0x0 \
  -drive file=./ubuntu-ppc64le.qcow2,if=none,id=drive0,format=qcow2,cache=none \
  -device ide-hd,bus=sata0.0,unit=0,drive=drive0,id=ide,bootindex=1 \


复杂 PCIe 配置
~~~~~~~~~~~~~~~~~~~~~~~~~~

每个芯片（POWER9）定义六个 PHB，但为兼容 libvirt，不提供默认 PCI 布局。可使用如下命令行选项，在任意可用 PCIe 插槽上添加 PCI 设备：

.. code-block:: bash

  -device e1000e,netdev=net0,mac=C0:FF:EE:00:00:02,bus=pcie.0,addr=0x0
  -netdev bridge,id=net0,helper=/usr/libexec/qemu-bridge-helper,br=virbr0,id=hostnet0

  -device megasas,id=scsi0,bus=pcie.0,addr=0x0
  -drive file=./ubuntu-ppc64le.qcow2,if=none,id=drive-scsi0-0-0-0,format=qcow2,cache=none
  -device scsi-hd,bus=scsi0.0,channel=0,scsi-id=0,lun=0,drive=drive-scsi0-0-0-0,id=scsi0-0-0-0,bootindex=2


以下完整示例在不同 PHB 上配置两个不同的存储控制器，各连接一个磁盘，第二个 PHB 留空：

.. code-block:: bash

  $ qemu-system-ppc64 -m 2G -machine powernv9 -smp 2,cores=2,threads=1 -accel tcg,thread=single \
  -kernel ./zImage.epapr -initrd ./rootfs.cpio.xz -bios ./skiboot.lid \
  \
  -device megasas,id=scsi0,bus=pcie.0,addr=0x0 \
  -drive file=./rhel7-ppc64le.qcow2,if=none,id=drive-scsi0-0-0-0,format=qcow2,cache=none \
  -device scsi-hd,bus=scsi0.0,channel=0,scsi-id=0,lun=0,drive=drive-scsi0-0-0-0,id=scsi0-0-0-0,bootindex=2 \
  \
  -device pcie-pci-bridge,id=bridge1,bus=pcie.1,addr=0x0 \
  \
  -device ich9-ahci,id=sata0,bus=bridge1,addr=0x1 \
  -drive file=./ubuntu-ppc64le.qcow2,if=none,id=drive0,format=qcow2,cache=none \
  -device ide-hd,bus=sata0.0,unit=0,drive=drive0,id=ide,bootindex=1 \
  -device e1000e,netdev=net0,mac=C0:FF:EE:00:00:02,bus=bridge1,addr=0x2 \
  -netdev bridge,helper=/usr/libexec/qemu-bridge-helper,br=virbr0,id=net0 \
  -device nec-usb-xhci,bus=bridge1,addr=0x7 \
  \
  -serial mon:stdio -nographic


也可以使用 VIRTIO 设备：

.. code-block:: bash

  -drive file=./fedora-ppc64le.qcow2,if=none,snapshot=on,id=drive0 \
  -device virtio-blk-pci,drive=drive0,id=blk0,bus=pcie.0 \
  \
  -netdev tap,helper=/usr/lib/qemu/qemu-bridge-helper,br=virbr0,id=netdev0 \
  -device virtio-net-pci,netdev=netdev0,id=net0,bus=pcie.1 \
  \
  -fsdev local,id=fsdev0,path=$HOME,security_model=passthrough \
  -device virtio-9p-pci,fsdev=fsdev0,mount_tag=host,bus=pcie.2


多插槽
~~~~~~~~~~~~~

插槽数根据 CPU 数和核心数推导。``-smp 2,cores=1`` 定义两个插槽、每个一个核心的机器；``-smp 2,cores=2`` 定义一个插槽、包含两个核心的机器；``-smp 8,cores=2`` 则定义四个插槽、每个两个核心。

BMC 配置
~~~~~~~~~~~~~~~~~

OpenPOWER 系统与其 BMC 协商关机和重启。QEMU PowerNV 机器内嵌使用 iBT 接口的 IPMI BMC 模拟器，应能提供相同的电源功能。

如需定义自己的 BMC，使用 ``-nodefaults``，并在命令行中指定：

.. code-block:: bash

  -device ipmi-bmc-sim,id=bmc0 -device isa-ipmi-bt,bmc=bmc0,irq=10


`palmetto-SDR.bin <http://www.kaod.org/qemu/powernv/palmetto-SDR.bin>`__ 和 `palmetto-FRU.bin <http://www.kaod.org/qemu/powernv/palmetto-FRU.bin>`__ 文件分别为 Palmetto BMC 定义传感器数据记录库和现场可更换单元清单，可用于扩展 QEMU BMC 模拟器。

.. code-block:: bash

  -device ipmi-bmc-sim,sdrfile=./palmetto-SDR.bin,fruareasize=256,frudatafile=./palmetto-FRU.bin,id=bmc0 \
  -device isa-ipmi-bt,bmc=bmc0,irq=10


PowerNV 机器也可使用外部 IPMI BMC 设备，连接到充当 BMC 的远程 QEMU 机器，选项如下：

.. code-block:: bash

  -chardev socket,id=ipmi0,host=localhost,port=9002,reconnect-ms=10000 \
  -device ipmi-bmc-extern,id=bmc0,chardev=ipmi0 \
  -device isa-ipmi-bt,bmc=bmc0,irq=10 \
  -nodefaults


NVRAM
~~~~~

使用 MTD 驱动器为机器添加 PNOR，从而获得 NVRAM：

.. code-block:: bash

  -drive file=./witherspoon.pnor,format=raw,if=mtd


如果未提供 MTD 驱动器，powernv 平台会使用 pc-bios/pnv-pnor.bin 中的小型已格式化 PNOR，以只读方式打开并创建默认 PNOR 设备（PNOR 的修改在重启间保留，但不会在不同 QEMU 启动间保留）。如果不使用默认设备，则提供已擦除的 128MB PNOR（由于未格式化，skiboot 可能无法识别）。

维护者联系方式
------------------------------

Cédric Le Goater <clg@kaod.org>
