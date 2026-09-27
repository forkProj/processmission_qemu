虚拟机生成 ID 设备
====================================

..
   Copyright (C) 2016 Red Hat, Inc.
   Copyright (C) 2017 Skyport Systems, Inc.

   This work is licensed under the terms of the GNU GPL, version 2 or later.
   See the COPYING file in the top-level directory.

虚拟机生成 ID（``vmgenid``）设备是一种仿真设备，提供一个 128 位、密码学随机的
整数标识符，称为全局唯一标识符（Globally Unique Identifier，GUID）。

管理应用程序（例如 libvirt）可借此通知客户机操作系统：虚拟机正在以不同配置运行
（例如执行快照，或从模板创建虚拟机）。客户机操作系统察觉这一变化后，就能采取
相应措施，例如将其分布式数据库副本标记为脏、重新初始化随机数生成器等。


要求
------------

以下要求摘自 2012 年 8 月 1 日发布的
`Microsoft 虚拟机生成 ID 规范 <http://go.microsoft.com/fwlink/?LinkId=260709>`_
中“如何在虚拟化平台中实现虚拟机生成 ID 支持”一节。

- **R1a** 生成 ID 必须位于按 8 字节对齐的缓冲区中。

- **R1b** 存储生成 ID 的缓冲区必须位于客户机 RAM、ROM 或设备 MMIO 范围内。

- **R1c** 存储生成 ID 的缓冲区必须与操作系统使用的区域分开。

- **R1d** E820 或 UEFI 内存映射中的 AddressRangeMemory 或 AddressRangeACPI
  条目不得覆盖该缓冲区。

- **R1e** 生成 ID 不得位于可能以禁用缓存方式映射的页框中。
  （换言之，无论生成 ID 位于 RAM、ROM 还是 MMIO 中，都只能映射为可缓存。）

- **R2** 至 **R5** [Microsoft 规范已将这些 AML 要求充分独立列出，此处直接引用。]

- **R6** Hypervisor 必须在 VMGenId 设备的作用域内提供一个
  对该 hypervisor 厂商唯一的 _HID（硬件标识符）对象。


QEMU 实现
-------------------

上述规范未规定由哪个 ACPI 描述符表包含虚拟机生成 ID 设备。其他实现（Hyper-V 和
Xen）将它放在主描述符表（差异化系统描述表，即 DSDT）中。为方便调试和实现，
我们决定将它放在独立的辅助系统描述表（SSDT）中。

以下是从运行中的系统转储的内容::

  # iasl -p ./SSDT -d /sys/firmware/acpi/tables/SSDT

  Intel ACPI Component Architecture
  ASL+ Optimizing Compiler version 20150717-64
  Copyright (c) 2000 - 2015 Intel Corporation

  Reading ACPI table from file /sys/firmware/acpi/tables/SSDT - Length
  00000198 (0x0000C6)
  ACPI: SSDT 0x0000000000000000 0000C6 (v01 BOCHS  VMGENID  00000001 BXPC 00000001)
  Acpi table [SSDT] successfully installed and loaded
  Pass 1 parse of [SSDT]
  Pass 2 parse of [SSDT]
  Parsing Deferred Opcodes (Methods/Buffers/Packages/Regions)

  Parsing completed
  Disassembly completed
  ASL Output:    ./SSDT.dsl - 1631 bytes
  # cat SSDT.dsl
  /*
   * Intel ACPI Component Architecture
   * AML/ASL+ Disassembler version 20150717-64
   * Copyright (c) 2000 - 2015 Intel Corporation
   *
   * Disassembling to symbolic ASL+ operators
   *
   * Disassembly of /sys/firmware/acpi/tables/SSDT, Sun Feb  5 00:19:37 2017
   *
   * Original Table Header:
   *     Signature        "SSDT"
   *     Length           0x000000CA (202)
   *     Revision         0x01
   *     Checksum         0x4B
   *     OEM ID           "BOCHS "
   *     OEM Table ID     "VMGENID"
   *     OEM Revision     0x00000001 (1)
   *     Compiler ID      "BXPC"
   *     Compiler Version 0x00000001 (1)
   */
  DefinitionBlock ("/sys/firmware/acpi/tables/SSDT.aml", "SSDT", 1, "BOCHS ", "VMGENID", 0x00000001)
  {
      Name (VGIA, 0x07FFF000)
      Scope (\_SB)
      {
          Device (VGEN)
          {
              Name (_HID, "QEMUVGID")  // _HID: Hardware ID
              Name (_CID, "VM_Gen_Counter")  // _CID: Compatible ID
              Name (_DDN, "VM_Gen_Counter")  // _DDN: DOS Device Name
              Method (_STA, 0, NotSerialized)  // _STA: Status
              {
                  Local0 = 0x0F
                  If ((VGIA == Zero))
                  {
                      Local0 = Zero
                  }

                  Return (Local0)
              }

              Method (ADDR, 0, NotSerialized)
              {
                  Local0 = Package (0x02) {}
                  Index (Local0, Zero) = (VGIA + 0x28)
                  Index (Local0, One) = Zero
                  Return (Local0)
              }
          }
      }

      Method (\_GPE._E05, 0, NotSerialized)  // _Exx: Edge-Triggered GPE
      {
          Notify (\_SB.VGEN, 0x80) // Status Change
      }
  }


设计细节：
---------------

要求 R1a 至 R1e 规定，存储虚拟机生成 ID 的内存必须由客户机固件分配并持有，
这里的固件是 BIOS 或 UEFI。不过，为使这一机制发挥作用，QEMU 必须能够在运行时
修改该内存的内容，尤其是在启动备份或快照映像时。为此，QEMU 必须知道所分配的地址。

选用的内存共享机制是可写的 fw_cfg 数据块。这些数据对象对 QEMU 和客户机均可见，
并可作为顺序文件寻址。

关于 fw_cfg 的更多信息，请参阅 :doc:`fw_cfg`。

这里使用两个 fw_cfg 数据块：

``/etc/vmgenid_guid``

- 包含实际的虚拟机生成 ID GUID
- 对客户机只读

``/etc/vmgenid_addr``

- 包含已下载的 vmgenid 数据块的地址
- 客户机可写


QEMU 在启动时向客户机发送以下命令：

1. 为 vmgenid_guid fw_cfg 数据块分配内存。
2. 将 vmgenid_guid 的地址写入 SSDT（即上述 iasl 转储中的 VGIA ACPI 变量）。
   注意，此修改不会传播回 QEMU。
3. 通过 fw_cfg DMA 接口，将 vmgenid_guid 的地址写回 QEMU 的 vmgenid_addr 副本。

完成第 3 步后，QEMU 即可随时更新 vmgenid_guid 的内容。

由于需要修改 GUID 时 BIOS 或 UEFI 未必会运行，VGIA 的值通过 VMState 机制保存。

规范明确要求，GUID 的任何变化都要触发 ACPI 通知。规范未指定具体使用哪个处理程序，
因此 vmgenid 设备使用第一个尚未占用的处理程序：``\_GPE._E05``。


字节序注意事项：
---------------------------

尽管 Microsoft 文档未作规定，这里假定设备应使用小端格式。

通过命令行或监视器传入的所有 GUID 均按大端处理。
通过监视器显示的 GUID 值也采用大端格式。


GUID 存储格式：
--------------------

为实现 OVMF 的“SDT 头部探测抑制器”，vmgenid_guid fw_cfg 数据块的内容并非只有
一个 128 位 GUID。它还包含大量填充，用于对齐并填满一个内存页，如下图所示::

  +----------------------------------+
  | SSDT：OEM Table ID = VMGENID     |
  +----------------------------------+
  | ...                              |       页首
  | VGIA 双字对象     ---------------|-----> +---------------------------+
  | ...                              |       | 固件所分配数组：          |
  | 引用 VGIA 的 _STA 方法           |       | "etc/vmgenid_guid"        |
  | ...                              |       +---------------------------+
  | 引用 VGIA 的 ADDR 方法           |       |  0: OVMF SDT 头部探测     |
  | ...                              |       |     抑制器                |
  +----------------------------------+       | 36: 填充以实现 8 字节     |
                                             |     对齐                  |
                                             | 40: GUID                  |
                                             | 56: 填充至页大小          |
                                             +---------------------------+
                                             页尾


设备用法：
-------------

该设备具有一个属性，只能通过命令行设置：

``guid``
  设置 GUID 的值。特殊值 ``auto`` 指示 QEMU 生成新的随机 GUID。

例如::

  QEMU  -device vmgenid,guid="324e6eaf-d1d1-4bf6-bf41-b9bb6c91fb87"
  QEMU  -device vmgenid,guid=auto

可通过 QMP/HMP 查询该属性::

  (QEMU) query-vm-generation-id
  {"return": {"guid": "324e6eaf-d1d1-4bf6-bf41-b9bb6c91fb87"}}

QMP/HMP 接口有意不提供设置该参数的功能。目前没有已知场景需要在 QEMU 运行后
修改 GUID，而增加此能力会大幅提升复杂性。
