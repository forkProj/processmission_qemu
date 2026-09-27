===================================
pSeries 系列板（``pseries``）
===================================

Linux on Power 架构参考文档（[LoPAR]_）描述的 Power 机器半虚拟化环境称为 pSeries，也称为 sPAPR、System p 客户机，或简称 Power Linux 客户机（尽管它也能运行 AIX 等其他操作系统）。

虽然 pSeries 被设计为客户机环境，但它也可以充当虚拟机监控器操作系统，从而提供嵌套虚拟化能力。

支持的设备
=================

 * 多代 Power 处理器的多处理器支持：
   - POWER7、POWER7+
   - POWER8
   - POWER9
   - Power10
   - Power11
   - 也支持 POWER5+，配合正确的内核和用户空间即可工作
 * 中断控制器
    - XICS（POWER8）
    - XIVE（以下处理器支持：）
        - POWER9
        - Power10
        - Power11
 * vPHB PCIe 主机桥。
 * vscsi 和 vnet 设备，兼容由 VIOS 管理 LPAR 的 PowerVM 虚拟机监控器提供的同类设备。
 * 基于 virtio 的设备。
 * PCIe 设备直通。

缺少的设备
===============

 * SPICE 支持。

固件
========

QEMU 的 pSeries 平台附带两种固件：

`SLOF <https://gitlab.com/slof/slof>`_ （Slimline Open Firmware）是 `IEEE 1275-1994 启动（初始化配置）固件标准：核心要求与实践
<https://standards.ieee.org/standard/1275-1994.html>`_ 的实现。

SLOF 执行总线扫描、PCI 资源分配，并提供从块设备和网络启动的客户端接口。

QEMU 包含 SLOF 预构建镜像，需要更新版本时会更新它。

VOF（Virtual Open Firmware）是与 ``-machine pseries,x-vof=on`` 配合使用的极简固件。启用后，固件作为轻量适配层，由 QEMU 实现 IEEE 1275 Open Firmware 接口的部分功能。

VOF 没有设备驱动，不进行 PCI 资源分配，而依靠 ``-kernel`` 配合足够新的 Linux 内核（v5.4+）完成 PCI 资源分配。它非常适合与 petitboot 配合使用。

通过 ``-kernel`` 启动时支持以下组合：

+-------------------+-------------------+------------------+
| 内核              | pseries,x-vof=off | pseries,x-vof=on |
+===================+===================+==================+
| vmlinux BE        |     ✓             |     ✓            |
+-------------------+-------------------+------------------+
| vmlinux LE        |     ✓             |     ✓            |
+-------------------+-------------------+------------------+
| zImage.pseries BE |     ✓¹            |     ✓¹           |
+-------------------+-------------------+------------------+
| zImage.pseries LE |     ✓             |     ✓            |
+-------------------+-------------------+------------------+

¹ 必须设置 kernel-addr=0

构建说明
================

.. code-block:: bash

  ./configure --target-list=ppc64-softmmu && make


运行说明
====================

运行 QEMU 时使用以下选项，可选择 pSeries 机器类型：

.. code-block:: bash

  qemu-system-ppc64 -M pseries <other QEMU arguments>


sPAPR 设备
=============

sPAPR 规范定义了一组半虚拟化设备，QEMU 的 pSeries 机器也支持这些设备，可通过 ``-device`` 选项实例化：

* ``spapr-vlan``：虚拟网络接口。
* ``spapr-vscsi``：虚拟 SCSI 磁盘接口。
* ``spapr-rng``：向客户机传递随机数生成器数据的伪设备（详情参见 `H_RANDOM 超级调用功能
  <https://wiki.qemu.org/Features/HRandomHypercall>`_）。
* ``spapr-vty``：虚拟电传终端。
* ``spapr-pci-host-bridge``：PCI 主机桥。
* ``tpm-spapr``：可信平台模块（TPM）。
* ``spapr-tpm-proxy``：TPM 代理。

这些设备兼容历史上运行 IBM PowerVM 虚拟机监控器及 LPAR 时可用的设备。

不过，这些设备最初的规范面向另一个虚拟机监控器和非 Linux 客户机，因此在可能时，应改用对应 virtio 设备（例如 virtio-net、virtio-blk/scsi 和 virtio-rng），因为在 QEMU 环境中，它们很可能为 Linux 客户机提供更好性能。

QEMU 的 pSeries 机器始终实例化以下设备：

* NVRAM 设备（``spapr-nvram``）。
* 虚拟电传终端（``spapr-vty``）。
* PCI 主机桥（``spapr-pci-host-bridge``）。

因此，除非使用 QEMU 的 ``-nodefaults`` 命令行选项，否则无需手动添加这些设备。

对于默认 ``spapr-nvram`` 设备，要使其内容持久保存，需要在启动 QEMU 时指定 PFLASH 设备：可以使用 ``-drive if=pflash,file=<filename>,format=raw`` 设置默认 PFLASH 设备，也可以指定带 ID 的设备（``-drive if=none,file=<filename>,format=raw,id=pfid``），再通过 ``-global spapr-nvram.drive=pfid`` 将该 ID 传给 NVRAM 设备。

sPAPR 规范
-------------------

sPAPR 标准的主要文档来源是 [LoPAR]_ 文档。QEMU 文档中也包含针对 QEMU 规范实现的说明：

.. toctree::
   :maxdepth: 1

   ../../specs/ppc-spapr-hotplug.rst
   ../../specs/ppc-spapr-hcalls.rst
   ../../specs/ppc-spapr-numa.rst
   ../../specs/ppc-spapr-uv-hcalls.rst
   ../../specs/ppc-spapr-xive.rst

在 KVM-PR 和 KVM-HV 内核模块间切换
=====================================================

目前，Power 上有两种 KVM 实现：``kvm_hv.ko`` 和 ``kvm_pr.ko``。

如果主机同时支持两种 KVM 模式，且两个 KVM 内核模块均已加载，可以通过 ``kvm-type`` 参数切换模式：

* 使用 ``qemu-system-ppc64 -M pseries,accel=kvm,kvm-type=PR`` 选择 ``kvm_pr.ko`` 内核模块。
* 使用 ``qemu-system-ppc64 -M pseries,accel=kvm,kvm-type=HV`` 选择 ``kvm_hv.ko``。

KVM-PR
------

KVM-PR 使用 PPC CPU 的所谓 **PR**\ oblem 状态运行客户机，即虚拟机在用户模式运行，所有特权指令都会陷入，必须由主机模拟。这意味着可以在 pSeries 客户机（或 PowerVM LPAR）内运行 KVM-PR，这也是它的起源：历史上（POWER7 之前）Power 处理器无法在虚拟机监控器模式运行 Linux，此功能仅限 IBM 专有的 PowerVM 虚拟机监控器。

由于所有特权指令都会陷入，大量使用特权指令的客户机在 KVM-PR 上运行较慢。另一方面，正因为如此，此内核模块几乎可以在所有 PPC 硬件上运行，并模拟许多客户机 CPU。它甚至可以运行其他 PowerPC 客户机，例如模拟的 PowerMac。

由于 KVM-PR 可在 pSeries 客户机内运行，因此它也能提供嵌套虚拟化能力（即在客户机内运行客户机）。

需要注意，由于 KVM-HV 的执行性能更好，过去几年的维护工作更集中于 KVM-HV，而 KVM-PR 的维护很少。

要在 POWER9 处理器上运行 KVM-PR 客户机，需要在启动 QEMU 时使用 ``kernel_irqchip=off`` 命令行选项。

KVM-HV
------

KVM-HV 使用较新 Power 处理器的虚拟机监控器模式，可直接访问裸机硬件。虽然 POWER7 已具备此能力，但 IBM 直到 POWER8 才正式支持。

最初，KVM-HV 只能在 PowerNV 平台（即 Power 裸机）上使用。虽然它运行在 PowerNV 上，但只能启动 pSeries 客户机。由于 pSeries 客户机无法访问 Power CPU 的虚拟机监控器模式，原本无法在客户机中运行 KVM-HV。这一限制已被解除，现在也可以在 pSeries 客户机内部运行 KVM-HV，从而支持 KVM-HV 嵌套虚拟化。

由于 KVM-HV 可以访问特权指令，大量使用这些指令的客户机能比 KVM-PR 快得多。另一方面，此方式要求客户机 CPU 与主机 CPU 类型相同，例如不能使用 KVM-HV 为客户机指定嵌入式 PPC CPU。不过，至少还可以让客户机运行在针对前代 CPU 的向后兼容模式，例如向 QEMU 传入 ``-cpu POWER8,compat=power7``，即可在 POWER8 主机上运行 POWER7 客户机。

模块支持
===============

如前文所述，各模块可运行在不同环境中。下表列出每个模块支持的环境。只要环境受支持，KVM-PR 或 KVM-HV 都可以嵌套运行。表中未列出的组合不可用。

+--------------+------------+------+-------------------+----------+--------+
| 平台         | 主机类型   | 位数 | 页表格式          | KVM-HV   | KVM-PR |
+==============+============+======+===================+==========+========+
| PowerNV      | 裸机       | 32   | hash              | 否       | 是     |
|              |            |      +-------------------+----------+--------+
|              |            |      | radix             | 不适用   | 不适用 |
|              |            +------+-------------------+----------+--------+
|              |            | 64   | hash              | 是       | 是     |
|              |            |      +-------------------+----------+--------+
|              |            |      | radix             | 是       | 否     |
+--------------+------------+------+-------------------+----------+--------+
| pSeries [1]_ | PowerNV    | 32   | hash              | 否       | 是     |
|              |            |      +-------------------+----------+--------+
|              |            |      | radix             | 不适用   | 不适用 |
|              |            +------+-------------------+----------+--------+
|              |            | 64   | hash              | 否       | 是     |
|              |            |      +-------------------+----------+--------+
|              |            |      | radix             | yes [2]_ | 否     |
|              +------------+------+-------------------+----------+--------+
|              | PowerVM    | 32   | hash              | 否       | 是     |
|              |            |      +-------------------+----------+--------+
|              |            |      | radix             | 不适用   | 不适用 |
|              |            +------+-------------------+----------+--------+
|              |            | 64   | hash              | 否       | 是     |
|              |            |      +-------------------+----------+--------+
|              |            |      | radix [3]_        | 否       | 是     |
+--------------+------------+------+-------------------+----------+--------+

.. [1] 在 POWER9 DD2.1 处理器上，主机与客户机的页表格式必须相同。

.. [2] KVM-HV 无法在 POWER8 机器上嵌套运行。

.. [3] 在 Power10 机器上引入。


.. _power-papr-protected-execution-facility-pef:

POWER（PAPR）受保护执行设施（PEF）
-----------------------------------------------

受保护执行设施（PEF）也称为安全客户机支持，是 IBM POWER9 和 POWER10 处理器提供的功能。

安装了包含 Ultravisor 的合适固件后，它会为 CPU 增加额外的内存保护模式。Ultravisor 管理一组虚拟机监控器无法访问的安全内存。

在 QEMU 中启用此功能后，客户机可通过 ultracall 进入“安全模式”，将大部分内存转移到安全内存中，使被攻陷的虚拟机监控器无法窃取其内容。

启动
^^^^^^^^^

启动允许进入 PEF 安全模式的客户机::

  $ qemu-system-ppc64 \
      -object pef-guest,id=pef0 \
      -machine confidential-guest-support=pef0 \
      ...


实时迁移
^^^^^^^^^^^^^^

PEF 客户机尚未实现实时迁移。为保持一致性，只要启用 PEF 功能，QEMU 目前就会阻止迁移，无论客户机实际上是否已进入安全模式。


维护者联系方式
==============================

Cédric Le Goater <clg@kaod.org>

Daniel Henrique Barboza <danielhb413@gmail.com>

.. [LoPAR] `Linux on Power 架构参考文档（LoPAR）第 2.9 次修订
   <https://openpowerfoundation.org/wp-content/uploads/2020/07/LoPAR-20200812.pdf>`_。
