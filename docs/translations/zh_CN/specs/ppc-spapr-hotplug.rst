=============================
sPAPR 动态重配置
=============================

sPAPR 或 pSeries 客户机使用动态重配置机制，处理 PCI 卡等动态“物理”资源，或内存、
CPU 和“物理”主机桥等“逻辑”／半虚拟化资源的热插拔。这些资源通常由主机／hypervisor
管理，并作为虚拟化资源提供给客户机。《Linux on Power Architecture Reference》
（[LoPAR]_）第 13 节详细描述了动态重配置。本文概述其中与 QEMU 实现相关的内容。

动态重配置连接器
==================================

为管理这些资源的热插拔，使用一种称为动态资源连接器（DRC）的固件抽象，
将特定动态资源分配给客户机，并为客户机提供接口，以配置或移除与之关联的资源。

DRC 的设备树描述
===============================

启动时分配给客户机的每个 DRC，通过一组四个 Open Firmware 设备树数组属性描述其
名称、索引、电源域和类型。根据 DRC 所管理的资源类型，可能存在多组这样的数组，
分别位于设备树的不同路径下。

某些情况下，DRC 本身也由动态资源提供，例如管理热插入 PHB 上 PCI 插槽的 DRC。
此时，这些数组会通过 :ref:`guest-host-interface` 中所述的热插入资源设备树获取接口
一并取得。

数组属性如下。数组中的每个条目／元素，描述 ``ibm,drc-indexes`` 对应位置元素
所标识的 DRC：

``ibm,drc-names``
-----------------

  前 4 字节：大端（BE）编码的整数，表示条目数。

  每个条目：以字节数组编码、以 NULL 结尾的 ``<name>`` 字符串。

    逻辑／虚拟资源的 ``<name>`` 值定义于《Linux on Power Architecture Reference》
    （[LoPAR]_）第 13.5.2.4 节，基本形式是资源类型，后接一个空格，再接一个在该类型
    所有资源中唯一的数值。

    PCI 或 VIO 设备等“物理”资源的 ``<name>`` 值定义为“位置代码”：从机箱到设备具体
    插槽的各级外围设备的“位置标签”，用连字符连接而成。这将资源映射到机箱中的
    物理位置，便于调试。对于 QEMU，此映射并不那么重要，因此我们分配符合命名规范的
    位置代码，但仅使用插槽自身的位置标签，以简化实现。位置标签命名约定详见
    [LoPAR]_ 第 12.3.1.5 节。在这里，PCI/VIO 设备插槽采用 ``C<n>``，其中 ``<n>``
    在所有 PCI/VIO 设备插槽中唯一。

``ibm,drc-indexes``
-------------------

  前 4 字节：BE 编码的整数，表示条目数。

  每个 4 字节条目：BE 编码的 ``<index>`` 整数，在机器的所有 DRC 中唯一。

    ``<index>`` 可任意指定，但 QEMU 尽量沿用 pHyp（PowerVM 的 hypervisor 部分）
    为 pSeries 客户机分配索引时使用的约定：

      ``bit[31:28]``：``<type>`` 的整数编码，其中 ``<type>`` 为：

        ``1`` 表示 CPU 资源。

        ``2`` 表示 PHB 资源。

        ``3`` 表示 VIO 资源。

        ``4`` 表示 PCI 资源。

        ``8`` 表示内存资源。

      ``bit[27:0]``：``<id>`` 的整数编码，其中 ``<id>`` 在指定类型的所有资源中唯一。

``ibm,drc-power-domains``
-------------------------

  前 4 字节：BE 编码的整数，表示条目数。

  每个 4 字节条目：32 位 BE 编码的 ``<index>`` 整数，指定资源将被分配到的电源域。
  在 QEMU 中，所有资源均关联到“在线插入”域，假定电源由系统自动管理。
  此域的整数值是特殊值 ``-1``。


``ibm,drc-types``
-----------------

  前 4 字节：BE 编码的整数，表示条目数。

  每个条目：以字节数组编码、以 NULL 结尾的 ``<type>`` 字符串。
  ``<type>`` 按如下方式分配：

    "CPU" 表示 CPU。

    "PHB" 表示物理主机桥。

    "SLOT" 表示 VIO 插槽。

    "28" 表示 PCI 插槽。

    "MEM" 表示内存资源。

.. _guest-host-interface:

管理动态资源的客户机到主机接口
=================================================

每个 DRC 都有一个全局唯一的 DRC 索引。客户机通过一组 RTAS 调用，配置和管理与
特定 DRC 关联的资源；这些调用通过 DRC 索引引用各个 DRC。这可以看作客户机到主机的
接口。

``rtas-set-power-level``
------------------------

设置指定电源域的功率级别。

  ``arg[0]``：标识电源域的整数。

  ``arg[1]``：该域的新功率级别，``0-100``。

  ``output[0]``：状态，成功时为 ``0``。

  ``output[1]``：命令执行后的功率级别。

``rtas-get-power-level``
------------------------

获取指定电源域的功率级别。

  ``arg[0]``：标识电源域的整数。

  ``output[0]``：状态，成功时为 ``0``。

  ``output[1]``：当前功率级别。

``rtas-set-indicator``
----------------------

设置指示器或传感器的状态。

  ``arg[0]``：标识传感器／指示器类型的整数。

  ``arg[1]``：传感器索引。对于与 DR 相关的传感器，通常为 DRC 索引。

  ``arg[2]``：期望的传感器值。

  ``output[0]``：状态，成功时为 ``0``。

本文关注与 DRC 关联的指示器／传感器类型，具体如下：

* ``9001``：``isolation-state``，控制／指示客户机是否能够访问设备。
  支持的传感器值：

    ``0``：``isolate``，使客户机操作系统无法访问设备。

    ``1``：``unisolate``，使客户机操作系统可以使用设备。

* ``9002``：``dr-indicator``，控制与设备关联的“视觉”指示器。
  支持的传感器值：

    ``0``：``inactive``，可以安全移除资源。

    ``1``：``active``，资源正在使用，无法安全移除。

    ``2``：``identify``，用于在交互式热插拔时直观标识插槽。

    ``3``：``action``，多数情况下与 identify 用法相同。

* ``9003``：``allocation-state``，通常仅用于“逻辑”DR 资源，分别在通过
  ``isolation-state->unisolate`` 获取资源之前，或通过 ``isolation-state->isolate``
  释放资源之后，请求分配／取消分配资源。对于“物理”DR（如 PCI 热插拔），资源已隐式
  预分配，不使用此传感器。支持的传感器值：

    ``0``：``unusable``，通知固件／系统可以取消分配或回收资源，将其放回系统资源池。

    ``1``：``usable``，请求分配或预留资源供客户机操作系统使用。

    ``2``：``exchange``，用于在某些情况下分配备用资源以进行故障转移。QEMU 不使用。

    ``3``：``recover``，用于回收先前已分配、但目前未分配给客户机操作系统的资源。
    QEMU 不使用。

``rtas-get-sensor-state:``
--------------------------

用于读取指示器或传感器值。

  ``arg[0]``：标识传感器／指示器类型的整数。

  ``arg[1]``：传感器索引。对于与 DR 相关的传感器，通常为 DRC 索引。

  ``output[0]``：状态，成功时为 0。

对于与 DR 相关的操作，唯一需要关注的传感器为 ``dr-entity-sense``，其类型值为
``9003``，与 ``rtas-set-indicator`` 中的 ``allocation-state`` 相同。
不过，传感器值的语义和编码不同。

``dr-entity-sense`` （``9003``）传感器支持的值：

  ``0``：空。

    对物理资源：DRC／插槽为空。

    对逻辑资源：未使用。

  ``1``：存在。

    对物理资源：DRC／插槽中装有设备／资源。

    对逻辑资源：已为 DRC 分配资源。

  ``2``：不可用。

    对物理资源：未使用。

    对逻辑资源：DRC 尚未分配资源。

  ``3``：交换。

    对物理资源：未使用。

    对逻辑资源：资源可用于交换（见上文 ``allocation-state`` 的传感器语义）。

  ``4``：恢复。

    对物理资源：未使用。

    对逻辑资源：资源可用于恢复（见上文 ``allocation-state`` 的传感器语义）。

``rtas-ibm-configure-connector``
--------------------------------

用于获取与特定 DRC 关联的资源的 OpenFirmware 设备树描述。

  ``arg[0]``：4096 字节工作区缓冲区的客户机物理地址。

  ``arg[1]``：0，或额外的 4096 字节工作区缓冲区地址；仅当先前的 RTAS 响应表示
  需要额外内存时才非零。

  ``output[0]``：状态：

    ``0``：已完成设备树节点的传输。

    ``1``：指示客户机准备下一个设备树同级节点。

    ``2``：指示客户机准备下一个设备树子节点。

    ``3``：指示客户机准备下一个设备树属性。

    ``4``：指示客户机返回设备树父节点。

    ``5``：指示客户机通过 ``arg[1]`` 提供额外工作区缓冲区。

    ``990x``：告知客户机操作耗时过长，应稍后重试。

DRC 索引编码在第一个工作区缓冲区的前 4 字节中。工作区（``wa``）布局如下，
偏移以 4 字节为单位：

  ``wa[0]``：要获取设备树节点的 DRC 的索引。

  ``wa[1]``：``0`` （硬编码）。

  ``wa[2]``：

    对 next-sibling/next-child 响应：

      表示新节点名称的以空字符结尾的字符串在 ``wa`` 中的偏移。

    对 next-property 响应：

      表示新属性名称的以空字符结尾的字符串在 ``wa`` 中的偏移。

  ``wa[3]``：用于 next-property 响应（其他情况不使用）：

      新属性值的字节长度。

  ``wa[4]``：用于 next-property 响应（其他情况不使用）：

      新属性的值，编码为兼容 OFDT 的字节数组。

热插拔事件
======================

对于大多数 DR 操作，hypervisor 通过 EPOW/check-exception 通知框架发送主机到客户机
的添加／移除事件：主机先发出 check-exception 中断，然后通过客户机为响应中断而
调用的 rtas-check-exception 提供 RTAS 事件日志。PAPR+ v2.7 描述了此框架，QEMU
已经使用它通过 EPOW 事件生成关机请求。

为了支持 DR，该框架已扩展为包含热插拔事件。此前，HMC 等主机级管理工具可直接操纵
客户机中与 DR 相关的用户空间工具，因此不需要此类事件。这种管理方式不适用于 Power
上的 KVM，所以需要扩展通知框架以支持热插拔事件。

这些通过 EPOW 通知的事件格式见下文 :ref:`hot-plug-unplug-event-structure`。
注意，这些事件不是 PAPR+ 规范的正式组成部分，并且已被更新的格式取代；
新格式也在下文 :ref:`hot-plug-unplug-event-structure` 中说明，因此旧格式如今被视为
“传统”格式。两种格式相似，但“现代”格式包含额外字段／标志。本文档使用
``#ifdef GUEST_SUPPORTS_MODERN`` 条件编译标记表示这些部分。

除非客户机通过 ``ibm,client-architecture-support`` hcall，将其
``ibm,architecture-vec-5`` 选项向量结构的字节 5、位 6 置位，以公布对“现代”格式的
支持（见 [LoPAR]_ 第 B.5.2.3 节），否则 QEMU 应假定仅支持“传统”字段／标志。
与“传统”格式事件一样，“现代”格式事件通过 check-exception RTAS 调用提供给客户机，
但使用专用事件源通知客户机。按照 [LoPAR]_ 第 B.5.12.2 节所述的标准格式，在客户机
设备树的 ``/event-sources`` 节点下添加 ``hot-plug-events`` 节点，即可向客户机公布
此事件源。

.. _hot-plug-unplug-event-structure:

热插拔事件结构
===============================

QEMU 中热插拔专用载荷实现如下（所有值均以大端格式编码）：

.. code-block:: c

   struct rtas_event_log_v6_hp {
   #define SECTION_ID_HOTPLUG              0x4850 /* HP */
       struct section_header {
           uint16_t section_id;            /* set to SECTION_ID_HOTPLUG */
           uint16_t section_length;        /* sizeof(rtas_event_log_v6_hp),
                                            * plus the length of the DRC name
                                            * if a DRC name identifier is
                                            * specified for hotplug_identifier
                                            */
           uint8_t section_version;        /* version 1 */
           uint8_t section_subtype;        /* unused */
           uint16_t creator_component_id;  /* unused */
       } hdr;
   #define RTAS_LOG_V6_HP_TYPE_CPU         1
   #define RTAS_LOG_V6_HP_TYPE_MEMORY      2
   #define RTAS_LOG_V6_HP_TYPE_SLOT        3
   #define RTAS_LOG_V6_HP_TYPE_PHB         4
   #define RTAS_LOG_V6_HP_TYPE_PCI         5
       uint8_t hotplug_type;               /* type of resource/device */
   #define RTAS_LOG_V6_HP_ACTION_ADD       1
   #define RTAS_LOG_V6_HP_ACTION_REMOVE    2
       uint8_t hotplug_action;             /* action (add/remove) */
   #define RTAS_LOG_V6_HP_ID_DRC_NAME          1
   #define RTAS_LOG_V6_HP_ID_DRC_INDEX         2
   #define RTAS_LOG_V6_HP_ID_DRC_COUNT         3
   #ifdef GUEST_SUPPORTS_MODERN
   #define RTAS_LOG_V6_HP_ID_DRC_COUNT_INDEXED 4
   #endif
       uint8_t hotplug_identifier;         /* type of the resource identifier,
                                            * which serves as the discriminator
                                            * for the 'drc' union field below
                                            */
   #ifdef GUEST_SUPPORTS_MODERN
       uint8_t capabilities;               /* capability flags, currently unused
                                            * by QEMU
                                            */
   #else
       uint8_t reserved;
   #endif
       union {
           uint32_t index;                 /* DRC index of resource to take action
                                            * on
                                            */
           uint32_t count;                 /* number of DR resources to take
                                            * action on (guest chooses which)
                                            */
   #ifdef GUEST_SUPPORTS_MODERN
           struct {
               uint32_t count;             /* number of DR resources to take
                                            * action on
                                            */
               uint32_t index;             /* DRC index of first resource to take
                                            * action on. guest will take action
                                            * on DRC index <index> through
                                            * DRC index <index + count - 1> in
                                            * sequential order
                                            */
           } count_indexed;
   #endif
           char name[1];                   /* string representing the name of the
                                            * DRC to take action on
                                            */
       } drc;
   } QEMU_PACKED;

``ibm,lrdr-capacity``
=====================

``ibm,lrdr-capacity`` 是 /rtas 设备树节点中的属性，用于标识客户机的动态重配置能力。
它由 ``<phys>``、``<size>`` 和 ``<maxcpus>`` 三元组组成。

  ``<phys>`` 采用 BE 格式编码，表示以字节为单位的最大地址，因此也表示可分配给
  客户机的最大内存。

  ``<size>`` 采用 BE 格式编码，表示可热插入客户机的内存大小增量。

  ``<maxcpus>`` 是 BE 编码的整数，表示客户机可拥有的最大处理器数。

``pseries`` 客户机使用此属性记录允许的最大 CPU 数。

``ibm,dynamic-reconfiguration-memory``
======================================

``ibm,dynamic-reconfiguration-memory`` 设备树节点表示可动态重配置的逻辑内存块
（LMB）。只有在客户机通过 ``ibm,client-architecture-support`` 调用公布对此节点的
支持时才生成它。不可动态重配置的内存由 ``/memory`` 节点表示。下面介绍此节点中
与 QEMU sPAPR 内存热插拔实现相关的属性。

``ibm,lmb-size``
----------------

此 64 位整数定义每个可动态重配置 LMB 的大小。

``ibm,associativity-lookup-arrays``
-----------------------------------

此属性定义一个查找数组，包含每个 LMB 的 NUMA 关联信息。它是一个按属性格式编码的
数组，以整数 M（关联列表数）开头，后接整数 N（每个关联列表的条目数），最后为
M 个关联列表，每个列表含 N 个整数。

此属性提供的信息与 ``/memory`` 节点的 ``ibm,associativity`` 属性相同。
每个已分配的 LMB 都有一个 0 至 M-1 之间的索引值，用作此表的索引，选择该 LMB
使用的关联列表。各 LMB 的索引值在 ``ibm,dynamic-memory`` 属性中定义。

``ibm,dynamic-memory``
----------------------

此属性描述可动态重配置的内存。它是一个按属性格式编码的数组，包含整数 N
（LMB 数量），后接 N 个 LMB 列表条目。

每个 LMB 列表条目由以下元素组成：

- LMB 起始逻辑地址，编码为 64 位整数，对应 ``/memory`` 节点的 ``reg`` 属性。
- LMB 的 DRC 索引，对应 ``/memory`` 节点的 ``ibm,my-drc-index`` 属性。
- 保留用于扩展的四个字节。
- LMB 的关联列表索引，用于索引前述 ``ibm,associativity-lookup-arrays`` 属性，
  取得该 LMB 应使用的关联列表。
- 一个 32 位标志字。位位置 ``0x00000008`` 的位定义此 LMB 在启动时是否已分配给分区。

``ibm,dynamic-memory-v2``
-------------------------

此属性描述可动态重配置的内存，是一种替代性的较新描述方式。它是一个按属性格式
编码的数组，包含整数 N（LMB 集合条目数），后接 N 个 LMB 集合条目。
每组具有相同属性的连续 LMB 对应一个 LMB 集合条目。

每个 LMB 集合条目由以下元素组成：

- 此条目中连续 LMB 的数量，表示为 32 位整数。
- 集合中第一个 LMB 的逻辑地址，编码为 64 位整数。
- 集合中第一个 LMB 的 DRC 索引。
- 关联列表索引，用于索引前述 ``ibm,associativity-lookup-arrays`` 属性，取得集合中
  所有 LMB 应使用的关联列表。
- 一个适用于集合中所有 LMB 的 32 位标志字。
