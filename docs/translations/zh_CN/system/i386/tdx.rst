Intel 可信域扩展（TDX）
====================================

Intel 可信域扩展（TDX）是一项 Intel 技术，它扩展了虚拟机扩展（VMX）和多密钥全内存加密（MKTME），引入了一种称为可信域（TD）的新型虚拟机客户机。TD 运行在一种特殊的 CPU 模式下，旨在保护其内存内容和 CPU 状态的机密性，防止包括承载它的虚拟机监控器（VMM）在内的任何其他软件访问，除非 TD 自身明确共享这些内容。

前提条件
-------------

要运行 TD，物理机必须加载并初始化 TDX 模块，KVM 虚拟机监控器必须支持并启用 TDX。满足这些要求后，``KVM_CAP_VM_TYPES`` 会报告支持 ``KVM_X86_TDX_VM``。

可信域虚拟固件（TDVF）
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

可信域虚拟固件（TDVF）提供启动 TD 客户机操作系统所需的 TD 服务。在 TD 启动前，需要将 TDVF 复制到客户机私有内存并对其进行度量。

可以使用 KVM vCPU ioctl ``KVM_TDX_INIT_MEM_REGION`` 将 TDVF 内容填充到其私有内存中。

由于 TDX 不支持只读内存槽，TDVF 无法映射为 pflash 设备，实际以 RAM 的方式工作。使用 "-bios" 选项加载 TDVF。

OVMF 是实现了 TDVF 支持的开源固件。因此，指定并加载 TDVF 的命令行参数为 ``-bios OVMF.fd``。

功能配置
---------------------

与非 TDX 虚拟机不同，TD 的 CPU 功能（通过 CPU 或 MSR 枚举）不完全受 VMM 控制。VMM 只能通过虚拟机作用域的 ``MEMORY_ENCRYPT_OP`` ioctl 的 ``KVM_TDX_INIT_VM`` 命令配置 TD 的部分功能。

可配置功能分为三类：

- 属性：
  - PKS（位 30）控制是否向 TD 提供监管者保护密钥，这决定了相关的 CPUID 位和 CR4 位；
  - PERFMON（位 63）控制是否向 TD 提供 PMU。

- XSAVE 相关功能（XFAM）：
  XFAM 是一个 64 位掩码，格式与 XCR0 或 IA32_XSS MSR 相同。它决定客户机 TD 可使用的扩展功能集合。

- CPUID 功能：
  VMM 只能直接配置部分 CPUID 叶中的部分位。

可配置的功能通过 TDX 能力报告。

TDX 能力
~~~~~~~~~~~~~~~~

虚拟机作用域的 ``MEMORY_ENCRYPT_OP`` ioctl 提供 ``KVM_TDX_CAPABILITIES`` 命令，用于从 KVM 获取 TDX 能力。它返回 ``struct kvm_tdx_capabilities`` 数据结构，说明属性、XFAM 和 CPUID 支持的配置。

TD 属性
~~~~~~~~~~~~~

QEMU 支持通过 "tdx-guest" 对象的 "attributes" 属性直接配置原始的 64 位 TD 属性。注意，用户有责任提供有效值，因为当前 QEMU 或 KVM 可能尚不支持其中的某些位。

QEMU 还支持通过 "tdx-guest" 对象的属性单独配置它所支持的属性位。例如 "sept-ve-disable"（位 28）。

基于 MSR 的功能
~~~~~~~~~~~~~~~~~~

当前 KVM 不支持为 TDX 配置基于 MSR 的功能（例如 MSR_IA32_ARCH_CAPABILITIES）。待 KVM 增加支持后，未来会在 QEMU 中启用此功能。

功能检查
~~~~~~~~~~~~~

QEMU 会检查给定 CPU 模型以及通过 "+featureA/-featureB" 显式调整后所确定的最终（CPU）功能是否受支持。它可能产生如下不支持某功能的警告：

  "warning: host doesn't support requested feature: CPUID.07H:EBX.intel-pt [bit 25]"

如果显式请求禁用固定为 1 的功能，也可能产生如下警告：

  "warning: TDX forcibly sets the feature: CPUID.80000007H:EDX.invtsc [bit 8]"

这是 QEMU 针对 TDX 新增的行为，因为 TDX 存在由 TDX 模块强制启用、VMM 无法禁用的固定为 1 的功能。

启动 TD（TDX 虚拟机）
-----------------------

启动 TD 所必需的命令行选项是 tdx-guest 对象和拆分的 kernel-irqchip，如下所示：

.. parsed-literal::

    |qemu_system_x86| \\
        -accel kvm \\
        -cpu host \\
        -object tdx-guest,id=tdx0 \\
        -machine ...,confidential-guest-support=tdx0 \\
        -bios OVMF.fd \\


限制
------------

 - kernel-irqchip 必须采用 split 模式；

   如果 kernel-irqchip 保持默认的 'auto' 设置，TDX 客户机会默认采用此模式。

 - 私有内存不支持只读；

 - 不支持 SMM：SMM 支持需要操作客户机寄存器状态，而这不被允许；

调试
---------

TD 属性的位 0 是 DEBUG 位，决定 TD 是否运行在域外调试模式。在域外调试模式下，可以通过指定的 SEAMCALL 访问 TD 的 VCPU 状态和私有内存。这需要 KVM 提供调用这些 SEAMCALL 的 API，并对 QEMU 作相应修改。

此功能计划在未来实现。

TD 证明
--------------

在 TD 客户机中，证明流程用于在向客户机提供秘密之前，向其他实体验证 TDX 客户机的可信性。

TD 证明首先在 TD 内调用 TDG.MR.REPORT 获取 REPORT。随后需要由 SGX 报价飞地（QE）将 REPORT 数据转换为可远程验证的 Quote。

由于当前 KVM 尚不支持，QEMU 对 TD 证明的支持也属于未来工作。

实时迁移
--------------

未来工作。

参考资料
----------

- `TDX 主页 <https://www.intel.com/content/www/us/en/developer/articles/technical/intel-trust-domain-extensions.html>`__

- `SGX QE <https://github.com/intel/SGXDataCenterAttestationPrimitives/tree/master/QuoteGeneration>`__
