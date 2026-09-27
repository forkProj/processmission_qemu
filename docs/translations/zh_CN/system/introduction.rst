简介
============

.. note::
   QEMU 支持多种目标 CPU 架构，以及各架构的多种机器。某种架构或机器适用的
   选项、属性和命令行，未必适用于其他模型。本手册示例都针对所展示的架构
   和机器类型。有疑问时，请查阅具体目标的文档。

.. _Accelerators:

虚拟化加速器
---------------------------

系统模拟提供机器的虚拟模型，包括 CPU、内存和设备，用于运行客户机操作系统。
它支持多种称为“加速器”的虚拟机监控程序，以及能模拟多种 CPU 的 JIT，
即 Tiny Code Generator（TCG）。

.. list-table:: 支持的加速器
  :header-rows: 1

  * - 加速器
    - 主机操作系统
    - 主机架构
  * - KVM
    - Linux
    - Arm、MIPS、PPC、RISC-V、s390x、x86
  * - Xen
    - Linux（作为 dom0）
    - Arm、x86
  * - MSHV
    - Linux（作为 dom0）
    - x86
  * - Hypervisor Framework（hvf）
    - MacOS
    - x86、Arm
  * - Windows Hypervisor Platform（whpx）
    - Windows
    - Arm、x86
  * - NetBSD Virtual Machine Monitor（nvmm）
    - NetBSD
    - x86
  * - Tiny Code Generator（tcg）
    - Linux、其他 POSIX 系统、Windows、MacOS
    - Arm、x86、Loongarch64、MIPS、PPC、s390x、Sparc64

功能概览
----------------

系统模拟提供丰富设备模型，包括专为高效虚拟化设计的 VirtIO 设备。部分设备
可通过 vhost-user（VirtIO）或 :ref:`Multi-process QEMU` 卸载到主进程之外。
平台支持时也可直通设备，消除模拟开销。详见 :ref:`device-emulation`。

功能完整的 :ref:`块层<Live Block Operations>` 可构造复杂、多层堆叠的存储
拓扑，支持重定向、网络、快照和迁移。

灵活的 chardev 系统支持通过标准输入输出、文件、Unix 套接字和 TCP 处理
字符设备 I/O。

管理接口包括按行交互的 :ref:`HMP<QEMU monitor>`，支持动态增删设备和检查
系统状态；:ref:`QMP<QMP Ref>` 则是定义明确、版本化、面向程序的 API，
供其他工具创建、控制和管理虚拟机。基于 `libvirt <https://libvirt.org>`_
的 `Virt Manager <https://virt-manager.org/>`_ 等高级工具使用此接口。

常见加速器支持通过 :ref:`gdbstub<GDB usage>` 连接 GDB，调试系统软件镜像。

运行
-------

QEMU API 丰富且复杂，可能不易掌握。有些架构只给磁盘镜像就能启动，但这些
示例依赖大量默认值，未必适合现代系统。

非 x86 架构支持多种机器，命令行通常更明确地定义机器和启动行为。
:ref:`system-targets-ref` 中常有示例。

项目并不反对直接用命令行启动，但也有许多项目提供更友好的体验。基于 libvirt
的工具可以探测功能，创建适合现有硬件的现代虚拟机。

QEMU 命令行的一般形式为：

.. parsed-literal::

  $ |qemu_system| [machine opts] \\
                  [cpu opts] \\
                  [accelerator opts] \\
                  [device opts] \\
                  [backend opts] \\
                  [interface opts] \\
                  [boot opts]


多数选项支持帮助，例如：

.. parsed-literal::

   $ |qemu_system| -M help


列出当前 QEMU 二进制支持的机器。help 也可作为其他选项参数，例如：

.. parsed-literal::

  $ |qemu_system| -device scsi-hd,help


列出控制 scsi-hd 行为的额外参数及其默认值。

.. list-table:: 选项概览
  :header-rows: 1
  :widths: 10, 90

  * - 选项
    -
  * - 机器
    - 定义机器类型、内存大小等。
  * - CPU
    - 定义 vCPU 类型、数量和拓扑。多数加速器提供 host 选项，直接传递主机
      CPU 配置，不过滤功能。
  * - 加速器
    - 取决于所用监控程序。默认 TCG 是纯模拟，要利用硬件虚拟化必须指定加速器。
  * - 设备
    - 机器类型未默认提供的额外设备。
  * - 后端
    - QEMU 如何处理客户机数据，例如块设备如何存储、网卡如何连接网络、
      串口如何连接外部。
  * - 接口
    - 如何显示、管理、控制或调试系统。
  * - 启动
    - 通过固件还是直接加载内核启动。

下面先定义用于 AArch64 客户机的通用 virt 平台，启用虚拟化，以便在模拟的
客户机中使用 KVM。为内置 pflash 命名，以便稍后覆盖默认配置。

.. code::

 $ qemu-system-aarch64 \
    -machine type=virt,virtualization=on,pflash0=rom,pflash1=efivars \
    -m 4096 \


接着用 max 选择 QEMU 能模拟的全部 Arm 功能，定义 4 个 vCPU，并显式指定
本来就是默认值的 TCG。

.. code::

 -cpu max \
 -smp 4 \
 -accel tcg \


virt 没有默认网络和存储设备，需要自行添加，并用 ID 为随后连接后端做准备。

.. code::

 -device virtio-net-pci,netdev=unet \
 -device virtio-scsi-pci \
 -device scsi-hd,drive=hd \


为网卡连接用户模式网络。外部无法直接访问该网络，因此把本机 2222 端口
转发到客户机 SSH 端口。

.. code::

 -netdev user,id=unet,hostfwd=tcp::2222-:22 \


将客户机块设备连接到预留的 LVM 分区。

.. code::

 -blockdev driver=raw,node-name=hd,file.driver=host_device,file.filename=/dev/lvm-disk/debian-bullseye-arm64 \


将 :ref:`QEMU monitor` 与串口输出复用，通过
:ref:`keys in the character backend multiplexer` 切换。没有默认图形设备，
且可完全在终端工作，因此禁用显示。

.. code::

 -serial mon:stdio \
 -display none \


最后覆盖默认固件，提供持久保存 EFI 配置的存储。固件负责找到磁盘、启动 grub，
最终运行系统。

.. code::

 -blockdev node-name=rom,driver=file,filename=(pwd)/pc-bios/edk2-aarch64-code.fd,read-only=true \
 -blockdev node-name=efivars,driver=file,filename=$HOME/images/qemu-arm64-efivars
