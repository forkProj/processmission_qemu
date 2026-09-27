Hyper-V 优化功能
======================


说明
-----------

在某些情况下，用软件实现硬件接口较慢，因此 KVM 实现了自己的半虚拟化接口。这对 Linux 很有效，因为客户机对此类功能的支持会与功能本身同时加入。然而，为专有操作系统（尤其是 Microsoft Windows）添加这些接口的支持可能非常困难，甚至无法做到。

x86 上的 KVM 为 Windows 客户机实现了 Hyper-V 优化功能（Enlightenments）。这些功能使 Windows 和 Hyper-V 客户机认为自己运行在兼容 Hyper-V 的虚拟机监控器之上，并使用 Hyper-V 专用功能。


配置
-----

KVM 和 QEMU 默认都不启用任何 Hyper-V 优化功能。在 QEMU 中，可以通过 CPU 标志逐项启用，例如：

.. parsed-literal::

  |qemu_system| --enable-kvm --cpu host,hv_relaxed,hv_vpindex,hv_time, ...


有些优化功能之间存在依赖关系，QEMU 应当检查所提供的配置是否合理。

只要启用了任意一组 Hyper-V 优化功能，QEMU 就会将虚拟机监控器标识（CPUID 0x40000000..0x4000000A）更改为 Hyper-V。KVM 标识和功能保留在叶 0x40000100..0x40000101 中。


现有优化功能
-----------------------

``hv-relaxed``
  此功能告知客户机操作系统，它正运行在虚拟机监控器上，应禁用看门狗超时。已知某些 Windows 版本仅看到 'hypervisor' CPU 标志就会这样做。

``hv-vapic``
  向客户机提供所谓的 VP Assist 页 MSR，使其能够更高效地使用 APIC。特别是，此优化功能支持半虚拟化的（无需退出的）EOI 处理。

``hv-spinlocks`` = xxx
  启用半虚拟化自旋锁。参数指定在通知虚拟机监控器之前应尝试获取自旋锁多少次。特殊值 0xffffffff 表示“从不通知”。

``hv-vpindex``
  向客户机提供 HV_X64_MSR_VP_INDEX（0x40000002）MSR，其中包含虚拟处理器索引信息。此优化功能适合与 hv-synic、hv-stimer 以及其他要求客户机知道其虚拟处理器索引的优化功能结合使用（例如，需要在超级调用中传入 VP 索引时）。

``hv-runtime``
  向客户机提供 HV_X64_MSR_VP_RUNTIME（0x40000010）MSR。该 MSR 以 100ns 为单位记录虚拟处理器运行时间，使客户机操作系统能够了解有多少时间被“占用”（虚拟 CPU 被抢占以执行其他工作时）。

``hv-crash``
  向客户机提供 HV_X64_MSR_CRASH_P0..HV_X64_MSR_CRASH_P5（0x40000100..0x40000105）和 HV_X64_MSR_CRASH_CTL（0x40000105）MSR。客户机在崩溃时写入这些 MSR，HV_X64_MSR_CRASH_P0..HV_X64_MSR_CRASH_P5 包含额外的崩溃信息。这些信息会输出到 QEMU 日志，并通过 QAPI 提供。
  注意：与真正的 Hyper-V 不同，写入 HV_X64_MSR_CRASH_CTL 会通过 ``KVM_SYSTEM_EVENT_CRASH`` 触发 ``qemu_system_guest_panicked()``，最终操作取决于 ``-action panic=...`` 策略（默认值为 ``shutdown``）。默认操作实际上会阻止 Windows 生成崩溃转储。

``hv-time``
  启用两种客户机可用的 Hyper-V 专用时钟源：基于 MSR 的 Hyper-V 时钟源（HV_X64_MSR_TIME_REF_COUNT，0x40000020），以及参考 TSC 页（通过 MSR HV_X64_MSR_REFERENCE_TSC，0x40000021 启用）。两者都以客户机为单位，参考 TSC 页时钟源允许无需退出即可读取时间戳。使用此优化功能可显著加速所有与时间戳相关的操作。

``hv-synic``
  启用 Hyper-V 合成中断控制器，它是本地 APIC 的扩展。启用后，此优化功能为客户机提供额外的通信设施：SynIC 消息和事件。这是实现 VMBus 设备的前提（QEMU 尚未实现）。此外，启用 Hyper-V 合成定时器也需要此功能。SynIC 通过 MSR HV_X64_MSR_SCONTROL..HV_X64_MSR_EOM（0x40000080..0x40000084）以及 HV_X64_MSR_SINT0..HV_X64_MSR_SINT15（0x40000090..0x4000009F）控制。

  依赖：``hv-vpindex``

``hv-stimer``
  启用 Hyper-V 合成定时器。每个虚拟 CPU 有四个合成定时器，通过 HV_X64_MSR_STIMER0_CONFIG..HV_X64_MSR_STIMER3_COUNT（0x400000B0..0x400000B7）MSR 控制。这些定时器可以工作在单次或周期模式。已知某些 Windows 版本在未提供此优化功能时会转而大量使用 HPET（HPET 不可用时甚至使用 RTC）；这可能导致显著的 CPU 消耗，即使虚拟 CPU 处于空闲状态也是如此。

  依赖：``hv-vpindex``、``hv-synic``、``hv-time``

``hv-tlbflush``
  启用半虚拟化 TLB 远程失效机制。在 x86 架构上，远程 TLB 刷新过程需要发送 IPI，并等待其他 CPU 执行本地 TLB 刷新。在虚拟化环境中，某些虚拟 CPU 在调用时可能根本未被调度，也可能无需刷新（或者可将刷新推迟到虚拟 CPU 被调度时）。hv-tlbflush 优化功能通过虚拟机监控器实现 TLB 远程失效，从而支持此优化。

  依赖：``hv-vpindex``

``hv-ipi``
  启用半虚拟化 IPI 发送机制。HvCallSendSyntheticClusterIpi 超级调用可同时以超过 64 个虚拟 CPU 为目标，而通过 APIC 实现相同操作需要多次访问（也就需要多次退出到虚拟机监控器）。

  依赖：``hv-vpindex``

``hv-vendor-id`` = xxx
  将 CPUID 0x40000000.EBX-EDX 中的 Hyper-V 标识从默认的 "Microsoft Hv" 改为指定值。参数不得超过 12 个字符。按照规范，客户机不应使用此信息，目前未知是否有 Windows 版本会因此采取不同的行为。
  注意：hv-vendor-id 不是优化功能，因此单独指定它而未指定其他优化功能时，不会启用 Hyper-V 标识。

``hv-reset``
  向客户机提供 HV_X64_MSR_RESET（0x40000003）MSR，允许其通过写入该 MSR 复位自身。即使启用此 MSR，它也不是 Windows 执行系统重启的推荐方式，因此可能不会被使用。

``hv-frequencies``
  提供 HV_X64_MSR_TSC_FREQUENCY（0x40000022）和 HV_X64_MSR_APIC_FREQUENCY（0x40000023），使客户机无需测量即可获得其 TSC/APIC 频率。

``hv-reenlightenment``
  此优化功能专用于嵌套场景，面向运行在 KVM 上的 Hyper-V 客户机。启用后，它提供 HV_X64_MSR_REENLIGHTENMENT_CONTROL（0x40000106）、HV_X64_MSR_TSC_EMULATION_CONTROL（0x40000107）和 HV_X64_MSR_TSC_EMULATION_STATUS（0x40000108）MSR，使客户机在 TSC 频率变化时（仅在迁移时发生）获得通知，并在准备好切换到新频率之前继续使用旧频率（通过虚拟机监控器模拟）。将其与 ``hv-frequencies`` 结合，可以使运行在 KVM 上的 Hyper-V 为自身的客户机提供稳定的时钟源（参考 TSC 页）。

  注意，KVM 尚未完全支持重新优化通知，也不会在迁移后模拟 TSC 访问，因此还必须指定 'tsc-frequency=' CPU 选项才能成功迁移。目标主机必须具有相同的 TSC 频率，或支持 TSC 缩放 CPU 功能。

  建议：``hv-frequencies``

``hv-evmcs``
  此优化功能专用于嵌套场景，面向运行在 KVM 上的 Hyper-V 客户机。启用后，它为客户机提供 Enlightened VMCS 第 1 版功能，在 L0（KVM）与 L1（Hyper-V）虚拟机监控器之间实现半虚拟化协议，加速 L2 退出到虚拟机监控器的过程。此功能仅适用于 Intel。

  注意：启用 hv-evmcs 时，某些虚拟化功能（例如 Posted Interrupts）会被禁用。可以分别测量启用和禁用此功能时嵌套工作负载的表现，以判断启用它是否有益。

  依赖：``hv-vapic``

``hv-stimer-direct``
  Hyper-V 规范允许合成定时器以两种模式工作：“经典”模式将到期事件作为 SynIC 消息递送，“直接”模式则通过普通中断递送事件。已知嵌套 Hyper-V 只能在直接模式下使用合成定时器，因此需要启用 ``hv-stimer-direct``。

  依赖：``hv-vpindex``、``hv-synic``、``hv-time``、``hv-stimer``

``hv-avic`` (``hv-apicv``)
  此优化功能允许在启用硬件 APICv/AVIC 的同时使用 Hyper-V SynIC。通常，Hyper-V SynIC 会禁用这些硬件功能，并建议客户机使用半虚拟化 AutoEOI 功能。
  注意：在旧硬件（不支持 APICv/AVIC）上启用此功能可能降低客户机性能。

``hv-no-nonarch-coresharing`` = on/off/auto
  此优化功能告知客户机操作系统：除非虚拟处理器被报告为同一核心的 SMT 兄弟线程，否则它们永远不会共享物理核心。Windows 和 Hyper-V 客户机需要此信息，才能正确缓解与 SMT 相关的 CPU 漏洞。

  当选项设为 'auto' 时，QEMU 仅在 KVM 报告不可能发生非架构核心共享时启用此功能，这意味着主机不支持超线程或已将其完全禁用。此设置还会阻止迁移，因为目标主机的 SMT 设置可能不同。当选项设为 'on' 时，QEMU 总会启用此功能，而不考虑主机配置。为保证客户机安全，只能在暴露正确 vCPU 拓扑并实施 vCPU 绑定的情况下使用此设置。

``hv-version-id-build``, ``hv-version-id-major``, ``hv-version-id-minor``, ``hv-version-id-spack``, ``hv-version-id-sbranch``, ``hv-version-id-snumber``
  将 CPUID 0x40000002.EAX-EDX 中的 Hyper-V 版本标识从默认值（WS2016）改为指定值。

  - ``hv-version-id-build`` 设置 'Build Number'（32 位）
  - ``hv-version-id-major`` 设置 'Major Version'（16 位）
  - ``hv-version-id-minor`` 设置 'Minor Version'（16 位）
  - ``hv-version-id-spack`` 设置 'Service Pack'（32 位）
  - ``hv-version-id-sbranch`` 设置 'Service Branch'（8 位）
  - ``hv-version-id-snumber`` 设置 'Service Number'（24 位）

  注意：hv-version-id-* 不是优化功能，因此未指定任何其他优化功能时，它们不会启用 Hyper-V 标识。

``hv-syndbg``
  启用 Hyper-V 合成调试器接口。这是一种供 Windows 内核调试器发送数据包的专用接口，用来代替串口或网络传输。
  启用后，此优化功能为客户机提供额外的通信设施：SynDbg 消息。
  Windows 内核调试器使用这种新通信方式替代通过串口或网络发送数据包，与其他通信通道相比可显著提升性能。
  此优化功能需要 VMBus 设备（-device vmbus-bridge,irq=15）。

  依赖：``hv-relaxed``、``hv_time``、``hv-vapic``、``hv-vpindex``、``hv-synic``、``hv-runtime``、``hv-stimer``

``hv-emsr-bitmap``
  此优化功能专用于嵌套场景，面向运行在 KVM 上的 Hyper-V 客户机。启用后，L0（KVM）和 L1（Hyper-V）虚拟机监控器可以协作，避免在 VM 退出时不必要地更新 L2 MSR-Bitmap。虽然 VMX（Intel）和 SVM（AMD）都支持该协议，但 VMX 实现还要求同时启用 Enlightened VMCS（``hv-evmcs``）功能。

  建议：``hv-evmcs`` （Intel）

``hv-xmm-input``
  Hyper-V 规范允许使用 XMM 寄存器为某些超级调用传递参数（“XMM 快速超级调用输入”）。使用此功能后，KVM 可以避免读取客户机内存，从而更快地处理超级调用。

``hv-tlbflush-ext``
  允许向 Hyper-V TLB 刷新超级调用（HvFlushVirtualAddressList/HvFlushVirtualAddressListEx）传递扩展的 GVA 范围。

  依赖：``hv-tlbflush``

``hv-tlbflush-direct``
  此优化功能专用于嵌套场景，面向运行在 KVM 上的 Hyper-V 客户机。启用后，L0（KVM）可以直接处理 L2 客户机发出的 TLB 刷新超级调用，无需退出到 L1（Hyper-V）虚拟机监控器。虽然 VMX（Intel）和 SVM（AMD）都支持此功能，但 VMX 实现还要求同时启用 Enlightened VMCS（``hv-evmcs``）功能。

  依赖：``hv-vapic``

  建议：``hv-evmcs`` （Intel）

补充功能
----------------------

``hv-passthrough``
  在某些情况下（例如开发期间），可以让 QEMU 采用“透传”模式，为 Windows 客户机提供 KVM 当前支持的所有优化功能。

  注意：``hv-passthrough`` 标志仅启用 QEMU 已知的优化功能（具有对应的 'hv-' 标志），并将 ``hv-spinlocks`` 和 ``hv-vendor-id`` 的值从 KVM 复制到 QEMU。``hv-passthrough`` 会覆盖命令行中的所有其他 'hv-' 设置。

  注意：``hv-passthrough`` 不启用 ``hv-syndbg``，因为后者在配置不当时可能阻止某些 Windows 客户机启动。如有需要，可额外启用 ``hv-syndbg``。

  注意：``hv-passthrough`` 实际上会阻止迁移，因为源主机与目标主机上启用的优化功能列表可能不同。

``hv-enforce-cpuid``
  默认情况下，只要暴露了 Hyper-V CPUID 接口，KVM 就允许客户机使用当前支持的所有 Hyper-V 优化功能，无论客户机可见的 CPUID 是否声明了这些功能。``hv-enforce-cpuid`` 会改变此行为，仅允许客户机使用已暴露的 Hyper-V 优化功能。

建议
---------------

为了获得 Windows 和 Hyper-V 客户机的最佳性能，除非存在特殊要求（例如迁移到较旧的 QEMU/KVM 版本、模拟特定 Hyper-V 版本等），建议启用当前实现的全部 Hyper-V 优化功能，但以下情况除外：

- ``hv-syndbg``、``hv-passthrough``、``hv-enforce-cpuid`` 不应在生产配置中启用，因为它们用于调试或开发。
- 可以不启用 ``hv-reset``，因为现代 Hyper-V 版本不提供它。
- ``hv-evmcs`` 只能（也应当）在 Intel CPU 上启用。虽然此功能只在嵌套配置（Hyper-V、WSL2）中使用，但为普通 Windows 客户机启用它不应产生负面影响。
- 只有在正确绑定 vCPU、确保不可能发生非架构核心共享时，才可以启用 ``hv-no-nonarch-coresharing``。
- ``hv-vendor-id``、``hv-version-id-build``、``hv-version-id-major``、``hv-version-id-minor``、``hv-version-id-spack``、``hv-version-id-sbranch``、``hv-version-id-snumber`` 可以保持不变；客户机不应因所呈现的 Hyper-V 版本不同而改变行为。
- 只有在虚拟化软件栈的上层通过 QAPI 使用崩溃信息时，才应启用 ``hv-crash``。在默认的 ``-action panic=shutdown`` 策略下，启用此功能实际上会阻止 Windows 在崩溃时创建转储。
- ``hv-reenlightenment`` 只能在支持 TSC 缩放的硬件上使用，或在不需要客户机迁移时使用。
- 主机 CPU 超额分配时（即还有其他被调度的任务或客户机），应将 ``hv-spinlocks`` 设为诸如 0xfff 的值；否则可以保留默认值（0xffffffff）。
- 如果硬件不支持 APIC 虚拟化（Intel APICv、AMD AVIC），则不应启用 ``hv-avic``/``hv-apicv``。

相关链接
------------

Hyper-V 顶层功能规范及其他信息：

- https://github.com/MicrosoftDocs/Virtualization-Documentation
- https://docs.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/tlfs
