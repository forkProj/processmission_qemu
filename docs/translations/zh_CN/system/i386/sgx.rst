软件防护扩展（SGX）
===============================

概述
--------

Intel 软件防护扩展（SGX）是一组用于内存访问的指令和机制，为敏感应用和数据提供安全访问。SGX 允许应用将其特定地址空间用作 *飞地*，即一个受保护的区域，即使存在具有特权的恶意软件也能保证机密性和完整性。任何不在飞地内的软件，包括特权软件，都无法访问飞地内存区域。

虚拟 SGX
-----------

SGX 功能通过 SGX CPUID 暴露给客户机。对于大多数 SGX CPUID，可以向客户机报告与主机相同的 CPUID 信息。报告相同的 CPUID 使客户机能够使用 SGX 的全部能力，而 KVM 无需模拟这些信息。

客户机的 EPC 基址和大小由 QEMU 确定；在为客户机初始化 SGX 之前，KVM 需要 QEMU 向其通知这些信息。

虚拟 EPC
~~~~~~~~~~~

默认情况下，QEMU 不会向虚拟机分配 EPC，也就是说，要在虚拟机中完整启用 SGX，必须显式为其分配 EPC。与 hugetlbfs 等其他专用内存类型类似，EPC 以内存后端的形式提供。

SGX EPC 通过 CPUID 枚举，也就是说，EPC“设备”必须先于 vCPU 本身实例化，而 vCPU 的实例化远早于通用设备的解析和实例化。由于这一限制，EPC 不需要 -maxmem，因为 EPC 不被视为冷插拔或热插拔内存。

QEMU 不会人为限制向客户机提供的 EPC 区段数量，例如它允许创建 64 个 1M EPC 区段。注意，某些内核可能无法识别全部 EPC 区段，例如 Linux SGX 驱动仅支持 8 个 EPC 区段，这是硬编码的限制。

以下 QEMU 参数片段创建两个 EPC 区段，为虚拟机预分配 64M，并额外映射但不分配 28M::

 -object memory-backend-epc,id=mem1,size=64M,prealloc=on \
 -object memory-backend-epc,id=mem2,size=28M \
 -M sgx-epc.0.memdev=mem1,sgx-epc.1.memdev=mem2


注意：

与物理 EPC 相比，虚拟 EPC 的大小和位置受到的限制要少得多。由于物理 EPC 通过范围寄存器保护，其大小必须为二的幂（不过软件看到的只是完整 EPC 的子集，例如 92M 或 128M），且 EPC 必须自然对齐。KVM SGX 的虚拟 EPC 完全是软件构造，只要求大小和位置按页对齐。QEMU 强制要求 EPC 大小为 4k 的倍数，并确保其基址按 4k 对齐。为简化实现，EPC 始终位于客户机物理地址空间的 4g 以上。

迁移
~~~~~~~~~

QEMU/KVM 不会阻止 SGX 虚拟机的实时迁移，尽管从硬件角度看，SGX 并不支持实时迁移，因为 EPC 和 SGX 密钥层次结构都绑定于物理平台。不过，如果客户机软件栈能够在突然丢失 EPC 后重新创建飞地，并且客户机飞地能够检测到 SGX 密钥变化并妥善处理，则仍可以支持实时迁移。例如，ERESUME 因 #PF.SGX 失败时，客户机软件可以检测到此情况并重新创建飞地；当飞地无法解封来自外部的敏感信息时，也可以检测该错误，并重新向其提供敏感信息。

CPUID
~~~~~

由于依赖众多，QEMU 目前没有任何内置 CPU 配置将 SGX 列为支持的功能。要向客户机提供 SGX（以及 SGX 启动控制），必须使用 ``-cpu host`` 透传主机 CPU 模型，或者在使用内置 CPU 模型时显式启用 SGX，例如使用 ``-cpu <model>,+sgx`` 或 ``-cpu <model>,+sgx,+sgxlc``。

通过 CPUID 枚举的所有 SGX 子功能，例如 SGX2、MISCSELECT、ATTRIBUTES 等，都可以通过 CPUID 标志进行限制。注意，强制限制 MISCSELECT、ATTRIBUTES 和 XFRM 需要拦截 ECREATE，因此可能略微降低客户机中的 SGX 性能。所有通过 -cpu 控制的 SGX 子功能都以 "sgx" 为前缀，例如::

  $ qemu-system-x86_64 -cpu help | xargs printf "%s\n" | grep sgx
  sgx
  sgx-debug
  sgx-encls-c
  sgx-enclv
  sgx-exinfo
  sgx-kss
  sgx-mode64
  sgx-provisionkey
  sgx-tokenkey
  sgx1
  sgx2
  sgxlc


以下 QEMU 参数片段透传主机 CPU，但限制对配置密钥和 EINIT 令牌密钥的访问::

 -cpu host,-sgx-provisionkey,-sgx-tokenkey


SGX 子功能无法模拟，也就是说，硬件不存在的子功能无法通过 '-cpu' 强制启用。

虚拟化 SGX 启动控制
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

QEMU 对 SGX 启动控制（LC）的支持是被动的，它不会主动更改 LC 配置。QEMU SGX 允许用户设置或清除 CPUID 标志（以及 fw_cfg 中相关的 IA32_FEATURE_CONTROL MSR 位），并在获取或写入客户机状态时保存或恢复 LE Hash MSR，但不会增加直接修改 LC 配置的新控制项。与硬件行为类似，将 LC 配置锁定为非 Intel 值的工作留给客户机固件。不同于主机 BIOS 的 SGX 启动控制（LC）设置，本设计没有针对 SGX 客户机的特殊 BIOS 设置。即使主机处于锁定模式，也仍然允许创建启用 SGX 的虚拟机。

功能控制
~~~~~~~~~~~~~~~

QEMU SGX 更新 ``etc/msr_feature_control`` fw_cfg 条目，根据各自的 CPUID 支持设置 SGX（位 18）和 SGX LC（位 17）标志。也就是说，只要现有客户机固件支持 fw_cfg.msr_feature_control，它就会自动据此设置 SGX 和 SGX LC。

启动客户机
-----------------

启动 SGX 客户机：

.. parsed-literal::

  |qemu_system_x86| \\
   -cpu host,+sgx-provisionkey \\
   -object memory-backend-epc,id=mem1,size=64M,prealloc=on \\
   -M sgx-epc.0.memdev=mem1,sgx-epc.0.node=0


在客户机中使用 SGX 需要支持 SGX 的内核或操作系统。可在客户机中通过以下命令检查支持情况::

  $ grep sgx /proc/cpuinfo


通过以下命令查看 SGX EPC 信息::

  $ dmesg | grep sgx
  [    0.182807] sgx: EPC section 0x140000000-0x143ffffff
  [    0.183695] sgx: [Firmware Bug]: Unable to map EPC section to online node. Fallback to the NUMA node 0.


启动 SGX NUMA 客户机：

.. parsed-literal::

  |qemu_system_x86| \\
   -cpu host,+sgx-provisionkey \\
   -object memory-backend-ram,size=2G,host-nodes=0,policy=bind,id=node0 \\
   -object memory-backend-epc,id=mem0,size=64M,prealloc=on,host-nodes=0,policy=bind \\
   -numa node,nodeid=0,cpus=0-1,memdev=node0 \\
   -object memory-backend-ram,size=2G,host-nodes=1,policy=bind,id=node1 \\
   -object memory-backend-epc,id=mem1,size=28M,prealloc=on,host-nodes=1,policy=bind \\
   -numa node,nodeid=1,cpus=2-3,memdev=node1 \\
   -M sgx-epc.0.memdev=mem0,sgx-epc.0.node=0,sgx-epc.1.memdev=mem1,sgx-epc.1.node=1


通过以下命令查看 SGX EPC NUMA 信息::

  $ dmesg | grep sgx
  [    0.369937] sgx: EPC section 0x180000000-0x183ffffff
  [    0.370259] sgx: EPC section 0x184000000-0x185bfffff

  $ dmesg | grep SRAT
  [    0.009981] ACPI: SRAT: Node 0 PXM 0 [mem 0x180000000-0x183ffffff]
  [    0.009982] ACPI: SRAT: Node 1 PXM 1 [mem 0x184000000-0x185bfffff]


参考资料
----------

- `SGX 主页 <https://software.intel.com/sgx>`__

- `SGX SDK <https://github.com/intel/linux-sgx.git>`__

- SGX 规范：Intel SDM 第 3 卷
