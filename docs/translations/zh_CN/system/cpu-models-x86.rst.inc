x86 主机的 KVM CPU 模型配置建议
============================================================

以下建议帮助配置 x86 主机 CPU 模型，目标是在尽量提高性能的同时，保护客户机
免受各种 CPU 硬件缺陷影响，并可选支持异构 CPU 主机之间的热迁移。

QEMU / KVM 的两种 CPU 配置方式
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

(1) **主机直通**

    将主机 CPU 的功能、型号、步进原样传给客户机。但虚拟化无法支持的功能
    仍可能由 KVM 过滤。这种模式不能安全热迁移，因为 libvirt/QEMU 无法
    保证跨主机暴露一致 CPU。无需热迁移时，推荐采用此方式。

(2) **命名模型**

    QEMU 提供一组预定义 CPU 模型，通常对应 Intel、AMD 的特定硬件代际。
    这样使客户机与主机 CPU 适度隔离，增加不同硬件间热迁移的灵活性。

两种方式均可额外启用或移除单项功能，修改客户机默认看到的特性。

libvirt 还支持第三种“主机模型”方式：自动选择接近主机的命名模型，再增加
功能，尽量逼近实际 CPU。它不像直通那样保证家族、步进等精确一致，但保留
多数直通优势，同时保证热迁移安全。

CPU 模型的 ABI 兼容级别
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

x86_64 定义了多个 `ABI compatibility levels`_。传统系统和工具链多面向最初
的基线 ABI，未来可能更多面向较新 ABI。下表列出各 QEMU 模型满足的兼容级别，
仅包含长期稳定版本，例如 Haswell-v4。此外还有许多别名，会按机器类型解析
到不同模型版本。

.. _ABI compatibility levels: https://gitlab.com/x86-psABIs/x86-64-ABI/

.. csv-table:: x86-64 ABI 兼容级别
   :file: cpu-models-x86-abi.csv
   :widths: 40,15,15,15,15
   :header-rows: 1

Intel x86 主机的首选 CPU 模型
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

推荐选择与主机 CPU 代际一致的模型。如果部署中混有不同型号且需要热迁移，
选择所有目标主机均兼容的最新模型。

``DiamondRapids``
    Intel Xeon 处理器（DiamondRapids，2026）

    不含 SMT，但支持 module（双核模块 DCM）和 die（核心构建块 CBB）拓扑层。
    每线程拥有 L1 指令/数据缓存，每模块拥有 L2，每 die 拥有 L3，
    可用 smp-cache 模拟：

        ::

            -machine smp-cache.0.cache=l1d,smp-cache.0.topology=thread,\
                     smp-cache.1.cache=l1i,smp-cache.1.topology=thread,\
                     smp-cache.2.cache=l2,smp-cache.2.topology=module,\
                     smp-cache.3.cache=l3,smp-cache.3.topology=die


``ClearwaterForest``
    Intel Xeon 处理器（ClearwaterForest，2025）

``SierraForest``, ``SierraForest-v2``
    Intel Xeon 处理器（SierraForest，2024）。v2 使用步进 3 缓解 GDS 和 RFDS。

``GraniteRapids``, ``GraniteRapids-v2``
    Intel Xeon 处理器（GraniteRapids，2024）

``Cascadelake-Server``, ``Cascadelake-Server-noTSX``
    Intel Xeon 处理器（Cascade Lake，2019），仅步进 6 或 7。
    *步进 5 的 Cascade Lake Xeon 易受 MDS 变体攻击*。

``Skylake-Server``, ``Skylake-Server-IBRS``, ``Skylake-Server-IBRS-noTSX``
    Intel Xeon 处理器（Skylake，2016）

``Skylake-Client``, ``Skylake-Client-IBRS``, ``Skylake-Client-noTSX-IBRS}``
    Intel Core 处理器（Skylake，2015）

``Broadwell``, ``Broadwell-IBRS``, ``Broadwell-noTSX``, ``Broadwell-noTSX-IBRS``
    Intel Core 处理器（Broadwell，2014）

``Haswell``, ``Haswell-IBRS``, ``Haswell-noTSX``, ``Haswell-noTSX-IBRS``
    Intel Core 处理器（Haswell，2013）

``IvyBridge``, ``IvyBridge-IBR``
    Intel Xeon E3-12xx v2（Ivy Bridge，2012）

``SandyBridge``, ``SandyBridge-IBRS``
    Intel Xeon E312xx（Sandy Bridge，2011）

``Westmere``, ``Westmere-IBRS``
    Westmere E56xx/L56xx/X56xx（Nehalem-C，2010）

``Nehalem``, ``Nehalem-IBRS``
    Intel Core i7 9xx（Nehalem 类 Core i7，2008）

``Penryn``
    Intel Core 2 Duo P9xxx（Penryn 类 Core 2，2007）

``Conroe``
    Intel Celeron_4x0（Conroe/Merom 类 Core 2，2006）

Intel x86 主机的重要 CPU 功能
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

主机支持时，应使用以下重要功能。部分命名模型默认未包含其中一些，因此需要
显式启用。主机直通或主机模型通常包含全部这些功能。

``pcid``
  建议启用，降低 Meltdown（CVE-2017-5754）修复的开销。

  Haswell、Broadwell 和 Skylake 默认包含。

  Westmere、SandyBridge 和 IvyBridge 应显式开启。部分桌面和移动 Westmere
  不支持此功能。

``spec-ctrl``
  启用 Spectre v2（CVE-2017-5715）修复所必需。

  带 -IBRS 后缀的 Intel 模型默认包含。

  不带该后缀的模型必须显式开启。

  主机微代码必须支持，才能用于客户机。

``stibp``
  某些操作系统启用更强的 Spectre v2 修复所必需。

  所有 Intel 模型均需显式开启。

  主机微代码必须支持，才能用于客户机。

``ssbd``
  启用 CVE-2018-3639 修复所必需。

  所有 Intel 模型默认均不包含。

  必须显式开启。

  主机微代码必须支持，才能用于客户机。

``pdpe1gb``
  建议启用，让客户机使用 1GB 页面。

  所有 Intel 模型默认均不包含。

  应显式开启。

  并非所有 CPU 硬件都支持。

``md-clear``
  确认 MDS（CVE-2018-12126、CVE-2018-12127、CVE-2018-12130、CVE-2019-11091）
  修复所必需。

  所有 Intel 模型默认均不包含。

  必须显式开启。

  主机微代码必须支持，才能用于客户机。

``mds-no``
  建议用于告知客户机：主机 *不受* MDS 变体影响，包括 MFBDS CVE-2018-12130、
  MLPDS CVE-2018-12127、MSBDS CVE-2018-12126。

  这是 MSR（模型专用寄存器）功能，而非 CPUID 功能，因此不会出现在主机或
  客户机 /proc/cpuinfo。主机内核用它填充 sysfs 的 MDS 漏洞文件。

  仅当主机 /sys/devices/system/cpu/vulnerabilities/mds 显示 Not affected
  时，才应对虚拟机启用。

``taa-no``
  建议用于告知客户机主机不受 CVE-2019-11135，即 TSX 异步中止（TAA）影响。

  也是 MSR 功能，不出现在 /proc/cpuinfo。

  仅当主机 /sys/devices/system/cpu/vulnerabilities/tsx_async_abort 显示
  Not affected 时启用。

``tsx-ctrl``
  建议告知客户机可以禁用 Intel TSX（事务同步扩展）；若处理器存在漏洞，
  也可用执行内存访问检查的 VERW 指令缓解 TAA。详见 Intel 的
  `MDS 深入分析 <https://software.intel.com/security-software-guidance/insights/deep-dive-intel-analysis-microarchitectural-data-sampling>`_。

  仅当主机启用 TSX，*且* 客户机启用 rtm 标志时，才应暴露此功能。

  禁用 TSX 可避免 KVM 客户机承担缓解 TSX 攻击的代价。

  tsx-ctrl 也是 MSR 功能，不出现在主机或客户机 /proc/cpuinfo。

  可通过两种方式验证客户机确已禁用 TSX：检查其 /proc/cpuinfo 中 *没有*
  rtm，或 tsx_async_abort 漏洞文件显示 ``Mitigation: TSX disabled``。

``bhi-no``
  建议用于告知客户机主机不受 CVE-2022-0001，即分支历史注入（BHI）影响。

  这是 MSR 功能，不出现在 /proc/cpuinfo。

  仅当主机 /sys/devices/system/cpu/vulnerabilities/spectre_v2 显示
  ``BHI: Not affected`` 时启用。

``gds-no``
  建议用于告知客户机主机不受 CVE-2022-40982，即 Gather Data Sampling（GDS）影响。

  这是 MSR 功能，不出现在 /proc/cpuinfo。

  仅当主机 /sys/devices/system/cpu/vulnerabilities/gather_data_sampling
  显示 Not affected 时启用。

``rfds-no``
  建议用于告知客户机主机不受 CVE-2023-28746，即 Register File Data Sampling
  （RFDS）影响。

  这是 MSR 功能，不出现在 /proc/cpuinfo。

  仅当主机 /sys/devices/system/cpu/vulnerabilities/reg_file_data_sampling
  显示 Not affected 时启用。

AMD x86 主机的首选 CPU 模型
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

建议选择与主机代际匹配的模型。异构部署若需要热迁移，应选择所有目标主机
均兼容的最新模型。

``EPYC``, ``EPYC-IBPB``
    AMD EPYC 处理器（2017）

``Opteron_G5``
    AMD Opteron 63xx 类 CPU（2012）

``Opteron_G4``
    AMD Opteron 62xx 类 CPU（2011）

``Opteron_G3``
    AMD Opteron 23xx（第三代 Opteron，2009）

``Opteron_G2``
    AMD Opteron 22xx（第二代 Opteron，2006）

``Opteron_G1``
    AMD Opteron 240（第一代 Opteron，2004）

AMD x86 主机的重要 CPU 功能
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

主机支持时应使用以下功能。部分或全部命名模型默认未包含，因此需显式启用。
主机直通或主机模型通常包含全部这些功能。

``ibpb``
  启用 Spectre v2（CVE-2017-5715）修复所必需。

  带 -IBPB 后缀的 AMD 模型默认包含。

  其他 AMD 模型必须显式开启。

  主机微代码必须支持，才能用于客户机。

``stibp``
  某些系统启用更强的 Spectre v2 修复所必需。

  所有 AMD 模型均需显式开启。

  主机微代码必须支持，才能用于客户机。

``virt-ssbd``
  启用 CVE-2018-3639 修复所必需。

  所有 AMD 模型默认均不包含。

  必须显式开启。

  即使同时提供 amd-ssbd，也应提供它，以获得最大客户机兼容性。

  某些 QEMU/libvirt 版本在主机模型模式下也必须强制启用，因为这是物理
  主机 CPU 不存在的虚拟功能。

``amd-ssbd``
  启用 CVE-2018-3639 修复所必需。

  所有 AMD 模型默认均不包含。

  必须显式开启。

  性能优于 virt-ssbd，主机支持时应暴露。但部分内核只认识 virt-ssbd，
  因此仍应同时提供后者，确保兼容。

``amd-no-ssb``
  建议用于表示主机不受 CVE-2018-3639 影响。

  所有 AMD 模型默认均不包含。

  未来不受此漏洞影响的 CPU，应通过它告知客户机无需启用缓解措施。
  它与 virt-ssbd、amd-ssbd 互斥。

``pdpe1gb``
  建议允许客户机使用 1GB 页面。

  所有 AMD 模型默认均不包含。

  应显式启用。

  并非所有硬件支持此功能。

默认 x86 CPU 模型
^^^^^^^^^^^^^^^^^^^^^^

默认模型旨在所有主机上运行。如果应用不想在启动前检查主机兼容性，默认模型
保证可用。

但它们会使客户机暴露于多种 CPU 硬件漏洞，因此强烈不推荐。应遵循前文建议
选择更合适配置；无需热迁移时推荐主机直通。

``qemu32``, ``qemu64``
    QEMU 虚拟 CPU 2.5+（32 位和 64 位变体）

未传 -cpu 或 libvirt XML 未提供 <cpu> 时，x86_64 使用 qemu64，i686
使用 qemu32。

其他不推荐的 x86 CPU
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

以下模型兼容多数 AMD 和 Intel 主机，但功能集很有限，无法获得最佳性能，
因此不推荐。

``kvm32``, ``kvm64``
    通用 KVM 处理器（32 位和 64 位）。

    仅为兼容很老的 QEMU 版本而保留的历史模型。

``486``, ``athlon``, ``phenom``, ``coreduo``, ``core2duo``, ``n270``, ``pentium``, ``pentium2``, ``pentium3``
    各种古老 x86 CPU，多数早于硬件辅助虚拟化出现，运行虚拟机通常不再需要。

CPU 模型配置语法
=================================

下面演示 QEMU 和 libvirt 中各类模型及功能配置。

QEMU 命令行
^^^^^^^^^^^^^^^^^

主机直通：

.. parsed-literal::

  |qemu_system| -cpu host


主机直通并定制功能：

.. parsed-literal::

  |qemu_system| -cpu host,vmx=off,...


命名模型：

.. parsed-literal::

  |qemu_system| -cpu Westmere


命名模型并定制功能：

.. parsed-literal::

  |qemu_system| -cpu Westmere,pcid=on,...


libvirt 客户机 XML
^^^^^^^^^^^^^^^^^^

主机直通::

    <cpu mode='host-passthrough'/>


主机直通并定制功能::

    <cpu mode='host-passthrough'>
        <feature name="vmx" policy="disable"/>
        ...
    </cpu>


主机模型::

    <cpu mode='host-model'/>


主机模型并定制功能::

    <cpu mode='host-model'>
        <feature name="vmx" policy="disable"/>
        ...
    </cpu>


命名模型::

    <cpu mode='custom'>
        <model name="Westmere"/>
    </cpu>


命名模型并定制功能::

    <cpu mode='custom'>
        <model name="Westmere"/>
        <feature name="pcid" policy="require"/>
        ...
    </cpu>
