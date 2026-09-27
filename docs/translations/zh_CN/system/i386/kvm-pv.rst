KVM 半虚拟化功能
============================

说明
-----------

在某些情况下，用软件实现硬件接口较慢，因此 ``KVM`` 实现了自己的半虚拟化接口。

配置
-----

``KVM`` 半虚拟化功能以 CPU 标志表示。启用 ``KVM`` 加速时，任意 CPU 模型默认都会启用以下功能：

- ``kvmclock``
- ``kvm-nopiodelay``
- ``kvm-asyncpf``
- ``kvm-steal-time``
- ``kvm-pv-eoi``
- ``kvmclock-stable-bit``

在使用拆分 irqchip 的 x2apic 模式下（例如 "-machine ...,kernel-irqchip=split -cpu ...,x2apic"），默认启用 ``kvm-msi-ext-dest-id`` 功能。

注意：使用 ``host`` CPU 模型时，QEMU 会向客户机透传所有支持的 ``KVM`` 半虚拟化功能。

现有功能
-----------------

``kvmclock``
  向客户机提供 ``KVM`` 专用的半虚拟化时钟源。自 Linux v2.6.26 起支持。

``kvm-nopiodelay``
  客户机无需在 PIO 操作中执行延迟。自 Linux v2.6.26 起支持。

``kvm-mmu``
  此功能已弃用。

``kvm-asyncpf``
  启用异步缺页机制。自 Linux v2.6.38 起支持。注意：自 Linux v5.10 起，此功能已弃用，``KVM`` 不再启用它。请改用 ``kvm-asyncpf-int``。

``kvm-steal-time``
  启用被占用时间（客户机 vCPU 未运行的时间）统计。自 Linux v3.1 起支持。

``kvm-pv-eoi``
  启用半虚拟化的中断结束通知。自 Linux v3.10 起支持。

``kvm-pv-unhalt``
  启用半虚拟化自旋锁支持。自 Linux v3.12 起支持。

``kvm-pv-tlb-flush``
  启用半虚拟化 TLB 刷新机制。自 Linux v4.16 起支持。

``kvm-pv-ipi``
  启用半虚拟化 IPI 机制。自 Linux v4.19 起支持。

``kvm-poll-control``
  允许客户机控制 HLT 时的主机端轮询。自 Linux v5.10 起支持。

``kvm-pv-sched-yield``
  启用半虚拟化的调度让出功能。自 Linux v5.10 起支持。

``kvm-asyncpf-int``
  启用基于中断的异步缺页机制。自 Linux v5.10 起支持。

``kvm-msi-ext-dest-id``
  支持外部中断的“扩展目标 ID”。此功能允许在不进行 IRQ 重映射的情况下使用最多 32768 个 CPU（但可能存在其他限制，使特定配置支持的 vCPU 数更少）。自 Linux v5.10 起支持。

``kvmclock-stable-bit``
  告知客户机：客户机可见的 TSC 值可完全信任，可用于 kvmclock 计算，且预期不会发生跳变。自 Linux v2.6.35 起支持。

补充功能
----------------------

``kvm-pv-enforce-cpuid``
  将受支持的半虚拟化功能集合限制为已暴露的功能。注意，默认情况下，即使客户机可见的 CPUID 未声明某些功能，``KVM`` 仍允许客户机使用当前支持的所有半虚拟化功能。自 Linux v5.10 起支持。


相关链接
------------

更多详情请参阅 Linux 中的 Documentation/virt/kvm。
