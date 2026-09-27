QEMU 与 ACPI BIOS NVDIMM 接口
============================================================

QEMU 通过 ACPI 支持 NVDIMM。本文描述其基本概念及 QEMU 与 ACPI BIOS 的接口。

NVDIMM ACPI 背景
----------------------------------------

ACPI 6.0 引入 NVDIMM，在 _SB 作用域定义根设备，_HID 为 ACPI0012。
对每个已存在或平台计划支持的 NVDIMM，固件在根设备下暴露 ACPI 命名空间设备。

子设备的 _ADR 对应 NFIT 设备句柄。根设备和各 NVDIMM 可具有 _DSM，
提供具体实现专用的额外功能。

以下 ACPI 6.0 示例的平台含一个 NVDIMM::

  Scope (\_SB){
     Device (NVDR) // Root device
     {
        Name (_HID, "ACPI0012")
        Method (_STA) {...}
        Method (_FIT) {...}
        Method (_DSM, ...) {...}
        Device (NVD)
        {
           Name(_ADR, h) //where h is NFIT Device Handle for this NVDIMM
           Method (_DSM, ...) {...}
        }
     }
  }

根设备与 NVDIMM 都支持的方法
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

_DSM（设备专用方法）
   允许设备提供驱动使用的专用控制功能。
   NVDIMM DSM 规范见 http://pmem.io/documents/NVDIMM_DSM_Interface_Example.pdf。

   参数：

   Arg0
     包含 UUID 的 16 字节 Buffer。
   Arg1
     包含修订 ID 的 4 字节 Integer。
   Arg2
     包含函数索引的 4 字节 Integer。
   Arg3
     包含 UUID、修订 ID 和函数索引所指定函数参数的 Package。

   返回值：

   函数索引为 0 时，返回包含函数索引位域的 Buffer。
   否则类型和值由 DSM 规范中相应 UUID、修订 ID 和函数索引定义。

NVDIMM 根设备的方法
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

_FIT（固件接口表）
   求值为 Buffer，数据格式为一系列 NFIT 类型结构。

   参数：无。

   返回：包含 NFIT 类型结构条目列表的 Buffer。

   结构详情见 ACPI 6.0 第 5.2.25 节，NVDIMM Firmware Interface Table（NFIT）。

QEMU NVDIMM 实现
----------------------------------------

QEMU 使用从 0x0a18 开始的 4 字节 I/O 端口及一个 RAM 内存页实现 NVDIMM ACPI。

内存：
   通过 BIOS 链接／加载功能要求 BIOS 分配一页内存，
   将地址动态修补到 ACPI 中名为 MEMA 的 int32 对象。

   该 RAM 页在 _DSM 与 QEMU 之间传数据。
   ACPI 持有控制权时拥有此页，写入输入数据；否则归 QEMU，仿真 _DSM 并写入输出。

   ACPI 写入的输入布局，偏移相对页起始：

   [0x0 - 0x3]
      4 字节 NVDIMM 设备句柄，完全由 QEMU 内部定义。
      [1, 0xFFFF] 表示 NVDIMM，其他值另作保留用途。

      保留句柄：

      - 0：名为 NVDR 的根设备。
      - 0x10000：在根设备调用的 QEMU 内部 DSM 函数。

   [0x4 - 0x7]
      4 字节修订 ID，即 _DSM 的 Arg1。

   [0x8 - 0xB]
      4 字节函数索引，即 Arg2。

   [0xC - 0xFFF]
      4084 字节 Arg3。

   QEMU 输出布局：

   [0x0 - 0x3]
      4 字节结果长度。

   [0x4 - 0xFFF]
      4092 字节 DSM 结果。

I/O 端口 0x0a18 - 0xa1b：
   ACPI 将 BIOS 分配的页地址写入此端口，QEMU 随即接管并填入结果。

   写入访问：

   [0x0a18 - 0xa1b]
      4 字节 BIOS 分配的内存页地址。

_DSM 处理流程图
----------------------------------------

MEMA 表示 BIOS 分配的内存页地址。

::

 +----------------------+      +-----------------------+
 |    1. OSPM           |      |    2. OSPM            |
 | save _DSM input data |      |  write "MEMA" to      | Exit to QEMU
 | to the page          +----->|  IO port 0x0a18       +------------+
 | indicated by "MEMA"  |      |                       |            |
 +----------------------+      +-----------------------+            |
                                                                    |
                                                                    v
 +--------------------+       +-----------+      +------------------+--------+
 |      5 QEMU        |       | 4 QEMU    |      |        3. QEMU            |
 | write _DSM result  |       |  emulate  |      | get _DSM input data from  |
 | to the page        +<------+ _DSM      +<-----+ the page indicated by the |
 |                    |       |           |      | value from the IO port    |
 +--------+-----------+       +-----------+      +---------------------------+
          |
          | Enter Guest
          |
          v
 +--------------------------+      +--------------+
 |     6 OSPM               |      |   7 OSPM     |
 | result size is returned  |      |  _DSM return |
 | by reading  DSM          +----->+              |
 | result from the page     |      |              |
 +--------------------------+      +--------------+

NVDIMM 热插拔
----------------------------------------

ACPI BIOS 的 GPE.4 处理程序专门通知操作系统 NVDIMM 热添加事件。

仅供 QEMU 内部使用的 _DSM 函数
------------------------------------------------------------

读取 FIT
^^^^^^^^^^^^^^^^^^^^^^^^

_FIT 通过 _DSM 每次获取一页 QEMU NFIT 结构数据，拼接后作为 _FIT 结果返回。

输入参数：

Arg0
  UUID，设为 648B9CF2-CDA1-4312-8AD9-49C4AF32BD62。
Arg1
  修订 ID，设为 1。
Arg2
  函数索引 0x1。
Arg3
  包含以下布局 Buffer 的 Package：

  .. list-table::
     :header-rows: 1

     * - 字段
       - 长度
       - 偏移
       - 描述
     * - offset
       - 4
       - 0
       - 要读取的 QEMU NFIT 结构数据偏移。

DSM 内存页输出布局：

.. list-table::
   :header-rows: 1

   * - 字段
     - 长度
     - 偏移
     - 描述
   * - length
     - 4
     - 0
     - 返回数据总长度，包含此头部。
   * - status
     - 4
     - 4
     - 状态码：0x0 成功；0x100 表示 _FIT 尚未完成读取时 NFIT 更新导致错误；
       其他值遵循 DSM Spec Rev1 第 3 章。
   * - fit data
     - 可变
     - 8
     - FIT 数据，仅 status 为 0 时存在。

OSPM 自己维护 FIT 偏移。当前偏移加上返回的 fit data 大小，即下一次读取偏移。
全部读完后返回零大小数据。

若返回 0x100，OSPM 应从偏移 0 重新开始读取 FIT。
