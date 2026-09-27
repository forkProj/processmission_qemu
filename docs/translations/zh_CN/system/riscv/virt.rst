'virt' 通用虚拟平台（``virt``）
==========================================

``virt`` 板是不对应任何真实硬件的平台，专为虚拟机设计。如果只想运行 Linux 等客户机，而不需要重现某款真实硬件的特殊行为和限制，建议使用此板类型。

支持的设备
-----------------

``virt`` 机器支持以下设备：

* 最多 512 个通用 RV32GC/RV64GC 核心，可带可选扩展
* 核心本地中断器（CLINT）
* 平台级中断控制器（PLIC）
* CFI 并行 NOR 闪存
* 一个兼容 NS16550 的 UART
* 一个 Google Goldfish RTC
* 一个 SiFive Test 设备
* 八个 virtio-mmio 传输设备
* 一个通用 PCIe 主机桥
* 允许客户机从 QEMU 获取数据的 fw_cfg 设备

默认 CPU 已启用虚拟机监控器扩展，因此可以直接使用具有该扩展的虚拟机，无需显式声明。

硬件配置信息
----------------------------------

如果未指定 ``-dtb`` 选项，``virt`` 机器会自动生成设备树二进制文件（“dtb”）并传给客户机。其中提供系统中各种设备的地址、中断线和其他配置信息。客户机软件应从生成的 DTB 中发现设备。

如果用户希望提供自己的 DTB，可以使用 ``-dtb`` 选项。这些 DTB 应满足以下要求：

* /cpus 节点的子节点数量应与 QEMU 的 ``-smp`` 选项匹配
* /memory 的 reg 大小应与 QEMU 通过 ``-m`` 选择的 ram_size 匹配
* 与 OpenSBI BIOS 镜像配合使用时，应包含 compatible 字符串为 "riscv,clint0" 的 CLINT 设备节点

启动选项
------------

``virt`` 机器可以使用标准 -kernel 功能加载 Linux 内核、VxWorks 内核或 S 模式 U-Boot 引导加载程序，并以默认 OpenSBI 固件镜像作为 -bios。它还支持推荐的 RISC-V 启动流程：使用标准 -bios 功能，由 U-Boot SPL（M 模式）加载 OpenSBI fw_dynamic 固件和 U-Boot proper（S 模式）。

使用闪存设备
-------------------

默认情况下，第一个闪存设备（pflash0）应包含 S 模式固件代码。它可以配置为只读，第二个闪存设备（pflash1）则可用于存储配置数据。

例如，启动 edk2 的命令如下：

.. code-block:: bash

  $ qemu-system-riscv64 \
     -blockdev node-name=pflash0,driver=file,read-only=on,filename=<edk2_code> \
     -blockdev node-name=pflash1,driver=file,filename=<edk2_vars> \
     -M virt,pflash0=pflash0,pflash1=pflash1 \
     ... other args ....


仅对于 TCG 客户机，还可以额外传入 ``-bios none``，从第一个闪存设备（pflash0）启动 M 模式固件，例如：

.. code-block:: bash

  $ qemu-system-riscv64 \
     -bios none \
     -blockdev node-name=pflash0,driver=file,read-only=on,filename=<m_mode_code> \
     -M virt,pflash0=pflash0 \
     ... other args ....


用于 pflash 的固件镜像必须恰好为 32 MiB。

riscv-iommu 支持
-------------------

此板支持通过以下命令行使用 riscv-iommu-pci 设备：

.. code-block:: bash

  $ qemu-system-riscv64 -M virt -device riscv-iommu-pci (...)


它还支持 riscv-iommu-sys 平台设备：

.. code-block:: bash

  $ qemu-system-riscv64 -M virt,iommu-sys=on (...)


关于 RISC-V IOMMU 支持的工作方式，请参阅 :ref:`riscv-iommu`。

机器专用选项
------------------------

支持以下机器专用选项：

- aclint=[on|off]

  此选项为 "on" 时，模拟 ACLINT 设备而不是 SiFive CLINT。未指定时视为 "off"。此选项仅适用于 TCG 加速器。

- acpi=[on|off|auto]

  此选项为 "on"（默认值）时，生成 ACPI 表，并以固件表 etc/acpi/rsdp 和 etc/acpi/tables 的形式提供。

- aia=[none|aplic|aplic-imsic]

  此选项允许选择 AIA（高级中断架构）规范定义的中断控制器。"aia=aplic" 选择 APLIC（高级平台级中断控制器）处理有线中断；"aia=aplic-imsic" 选择 APLIC 和 IMSIC（入站消息信号中断控制器），同时处理有线中断和 MSI。未指定时视为 "none"，即选择 SiFive PLIC 处理有线中断。

  此选项也与 '-accel kvm' 交互。在 KVM 下使用 "aia=aplic-imsic" 时，可以通过 "-accel kvm,kernel-irqchip=split" 将内核 irqchip 设为拆分模式。此时，``virt`` 机器会模拟 APLIC 控制器，而不使用 irqchip 中的 APLIC 控制器。关于全部可用 AIA 模式的更多详情，请参阅 :ref:`riscv-aia`。

- aia-guests=nnn

  为具有 AIA IMSIC 的客户机（即选择了 "aia=aplic-imsic"）模拟的每 hart VS 级 AIA IMSIC 页面数量。未指定时，每 hart VS 级 AIA IMSIC 页面默认数量为 0。

- iommu-sys=[on|off]

  启用 riscv-iommu-sys 平台设备。默认为 'off'。

运行 Linux 内核
--------------------

撰写本文时测试了 Linux 主线 v5.12 发布版。要构建可由 ``virt`` 机器以 64 位模式启动的 Linux 主线内核，只需使用 defconfig 配置内核：

.. code-block:: bash

  $ export ARCH=riscv
  $ export CROSS_COMPILE=riscv64-linux-
  $ make defconfig
  $ make


在 QEMU 中使用 ``virt`` 机器启动新构建的 Linux 内核：

.. code-block:: bash

  $ qemu-system-riscv64 -M virt -smp 4 -m 2G \
      -display none -serial stdio \
      -kernel arch/riscv/boot/Image \
      -initrd /path/to/rootfs.cpio \
      -append "root=/dev/ram"


要构建可由 ``virt`` 机器以 32 位模式启动的 Linux 主线内核，请使用 rv32_defconfig 配置。Linux v5.12 内核需要一个补丁来修复 32 位启动问题。

.. code-block:: bash

  $ export ARCH=riscv
  $ export CROSS_COMPILE=riscv64-linux-
  $ curl https://patchwork.kernel.org/project/linux-riscv/patch/20210627135117.28641-1-bmeng.cn@gmail.com/mbox/ > riscv.patch
  $ git am riscv.patch
  $ make rv32_defconfig
  $ make


将上述命令行中的 ``qemu-system-riscv64`` 替换为 ``qemu-system-riscv32``，即可启动 32 位 Linux 内核。必须使用包含 32 位应用的根文件系统镜像，内核才能启动到用户空间。

运行 U-Boot
--------------

撰写本文时测试了 U-Boot 主线 v2021.04 发布版。要构建可由 ``virt`` 机器启动的 S 模式 U-Boot 引导加载程序，请使用 qemu-riscv64_smode_defconfig，命令与上述 Linux 构建命令类似：

.. code-block:: bash

  $ export CROSS_COMPILE=riscv64-linux-
  $ make qemu-riscv64_smode_defconfig


直接启动 64 位 U-Boot S 模式镜像：

.. code-block:: bash

  $ qemu-system-riscv64 -M virt -smp 4 -m 2G \
      -display none -serial stdio \
      -kernel /path/to/u-boot.bin


要测试启动运行在 M 模式的 U-Boot SPL，并由它加载将 OpenSBI fw_dynamic 固件和 U-Boot proper（S 模式）打包在一起的 FIT 镜像，请使用 riscv64_spl_defconfig 构建 U-Boot 镜像：

.. code-block:: bash

  $ export CROSS_COMPILE=riscv64-linux-
  $ export OPENSBI=/path/to/opensbi-riscv64-generic-fw_dynamic.bin
  $ make qemu-riscv64_spl_defconfig


运行 U-Boot SPL 的最简 QEMU 命令为：

.. code-block:: bash

  $ qemu-system-riscv64 -M virt -smp 4 -m 2G \
      -display none -serial stdio \
      -bios /path/to/u-boot-spl \
      -device loader,file=/path/to/u-boot.itb,addr=0x80200000


要测试 32 位 U-Boot 镜像，请改用 qemu-riscv32_smode_defconfig 和 riscv32_spl_defconfig 构建，并将上述命令行中的 ``qemu-system-riscv64`` 替换为 ``qemu-system-riscv32``，以启动 32 位 U-Boot。

启用 TPM
------------

可按以下步骤将 TPM 设备连接到 virt 板。

首先启动 TPM 模拟器：

.. code-block:: bash

  $ swtpm socket --tpm2 -t -d --tpmstate dir=/tmp/tpm \
        --ctrl type=unixio,path=swtpm-sock


然后使用额外参数启动 QEMU，将 TPM 设备连接到后端：

.. code-block:: bash

  $ qemu-system-riscv64 \
    ... other args .... \
    -chardev socket,id=chrtpm,path=swtpm-sock \
    -tpmdev emulator,id=tpm0,chardev=chrtpm \
    -device tpm-tis-device,tpmdev=tpm0


TPM 设备可在内存树和生成的设备树中看到，客户机软件应能访问它。
