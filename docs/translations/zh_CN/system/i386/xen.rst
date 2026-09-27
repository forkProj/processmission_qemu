Xen HVM 客户机支持
=====================


说明
-----------

KVM 支持承载 Xen 客户机，拦截 Xen 超级调用和事件通道（Xen PV 中断）递送。这使得预期运行在 Xen 下的客户机也可以在 Linux/KVM 上的 QEMU 中运行。

Xen 支持强制要求使用拆分 irqchip。

配置
-----

通过设置 KVM 加速器的 ``xen-version`` 属性启用 Xen 模式，例如 Xen 4.17：

.. parsed-literal::

  |qemu_system| --accel kvm,xen-version=0x40011,kernel-irqchip=split


此外，可以通过 ``xen-vapic`` CPU 标志向客户机通告虚拟 APIC 支持：

.. parsed-literal::

  |qemu_system| --accel kvm,xen-version=0x40011,kernel-irqchip=split --cpu host,+xen-vapic


启用 Xen 支持后，QEMU 会将虚拟机监控器标识（CPUID 0x40000000..0x4000000A）更改为 Xen。KVM 标识和功能不会向 Xen 客户机通告。如果同时启用了 Hyper-V，Xen 标识会移至叶 0x40000100..0x4000010A。

属性
----------

KVM 加速器对象提供以下属性：

``xen-version``
  此属性以 ``XENVER_version`` 格式包含 Xen 版本，高 16 位为主版本，低 16 位为次版本。设置此属性即可启用 Xen 客户机支持。如果指定 Xen 4.5 或更高版本，会填充 Xen CPUID 中的 HVM 叶。Xen 4.6 在 CPUID 中启用 vCPU ID，4.17 则向客户机通告 vCPU upcall 向量支持。

``xen-evtchn-max-pirq``
  Xen PIRQ 表示模拟的物理中断，可以是 GSI 或 MSI；它可路由到事件通道，而不是模拟的 I/O APIC 或本地 APIC。默认情况下，QEMU 只允许 256 个 PIRQ，以最大程度兼容 32 位 MSI，否则 PIRQ 编号的高位需要放在 MSI 消息的高 64 位中。对于 PCI 设备很多（且没有设备受限于 32 位寻址）的客户机，可以考虑增大此值。

``xen-gnttab-max-frames``
  Xen 授权表是 Xen 客户机向 PV 后端（磁盘、网络等）授予内存访问权的方式。QEMU 仅支持条目大小为 8 字节的 v1 授权表，因此每个授权表页（每个帧）可引用客户机内存的 512 个页面。默认帧数为 64，允许 PV 后端通过同时生效的授权访问 32768 个客户机内存页。对于 PV 设备很多且吞吐量高的客户机，可以考虑增大此值。

Xen 半虚拟化设备
-----------------------

Xen 客户机会自动启用 Xen PCI 平台设备。它允许客户机拔除所有模拟设备，转而使用半虚拟化块设备和网络驱动。

这些半虚拟化 Xen 块设备、网络设备（以及控制台设备）可通过命令行创建，也可热插拔，或同时使用两种方式。

要提供 Xen 控制台设备，先定义字符设备，再定义一个 ``xen-console`` 类型设备连接到它。例如，与便捷的 ``-serial mon:stdio`` 选项等效的 Xen 控制台配置为：

.. parsed-literal::
   -chardev stdio,mux=on,id=char0,signal=off \\
   -object monitor-hmp,chardev=char0,id=hmp0 \\
   -device xen-console,chardev=char0


Xen 网络设备为 ``xen-net-device``，它会成为模拟 Xen 客户机的默认网卡模型，因此仅使用 QEMU 提供的默认网卡就应能自动工作，并向客户机呈现一个 Xen 网络设备。

磁盘可通过 '``-drive file=${GUEST_IMAGE},if=xen``' 配置，在客户机中从 ``xvda`` 起依次出现。

在 Xen 下，启动磁盘通常同时通过 IDE 模拟和 PV 块设备提供。客户机引导加载程序通常使用 IDE 加载客户机内核，随后内核拔除 IDE，继续使用 Xen PV 块设备。

可以用以下方式实现此配置：

.. parsed-literal::

  |qemu_system| --accel kvm,xen-version=0x40011,kernel-irqchip=split \\
       -drive file=${GUEST_IMAGE},if=xen \\
       -drive file=${GUEST_IMAGE},file.locking=off,if=ide


也可以使用 VirtIO 设备；对于 Linux 客户机，可能需要在命令行中添加 '``xen_emul_unplug=never``'，防止其拔除这些设备。

启动 Xen PV 客户机
---------------------

可以使用 Xen PV shim 启动 PV 客户机内核。Xen PV shim 是 Xen 自身的一个版本，设计为运行在 Xen HVM 客户机中，并且仅为一个客户机提供内存管理服务。

通过 ``-kernel`` 提供 Xen 二进制文件，通过 ``-initrd`` 镜像提供客户机内核本身（或 PV Grub 镜像）；这里的 ``-initrd`` 实际上只是第一个 multiboot“模块”。例如：

.. parsed-literal::

  |qemu_system| --accel kvm,xen-version=0x40011,kernel-irqchip=split \\
       -chardev stdio,id=char0 -device xen-console,chardev=char0 \\
       -display none  -m 1G  -kernel xen -initrd bzImage \\
       -append "pv-shim console=xen,pv -- console=hvc0 root=/dev/xvda1" \\
       -drive file=${GUEST_IMAGE},if=xen


Xen 镜像必须使用 ``CONFIG_XEN_GUEST`` 和 ``CONFIG_PV_SHIM`` 选项构建。截至 Xen 4.17，Xen 的 PV shim 模式不支持使用串口；必须提供 Xen 控制台，否则会触发 panic。

上例在 Xen 命令行的分隔符（" ``--`` "）之后提供客户机内核命令行，但没有为客户机内核提供实际的 initramfs；后者需要列为第二个 multiboot 模块。更复杂的用法请参阅 ``-initrd`` 选项的命令行 :ref:`文档 <system/invocation-qemu-options-initrd>`。

主机操作系统要求
--------------------

KVM 加速器的最低限度 Xen 支持要求主机运行 Linux v5.12 或更高版本。后续版本加入了优化：Linux v5.17 增加了通过 Xen PIRQ 机制递送中断的加速，Linux v5.19 加速了 Xen PV 定时器和处理器间中断（IPI）。
