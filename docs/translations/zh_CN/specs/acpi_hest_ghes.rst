APEI 表生成与 CPER 记录
============================================================

..
   Copyright (c) 2020 HUAWEI TECHNOLOGIES CO., LTD.

   This work is licensed under the terms of the GNU GPL, version 2 or later.
   See the COPYING file in the top-level directory.

设计细节
--------------

::

         etc/acpi/tables                           etc/hardware_errors
      ====================                   ===============================
  + +--------------------------+            +----------------------------+
  | | HEST                     | +--------->|    error_block_address1    |------+
  | +--------------------------+ |          +----------------------------+      |
  | | GHES1                    | | +------->|    error_block_address2    |------+-+
  | +--------------------------+ | |        +----------------------------+      | |
  | | .................        | | |        |      ..............        |      | |
  | | error_status_address-----+-+ |        -----------------------------+      | |
  | | .................        |   |   +--->|    error_block_addressN    |------+-+---+
  | | read_ack_register--------+-+ |   |    +----------------------------+      | |   |
  | | read_ack_preserve        | +-+---+--->|     read_ack_register1     |      | |   |
  | | read_ack_write           |   |   |    +----------------------------+      | |   |
  + +--------------------------+   | +-+--->|     read_ack_register2     |      | |   |
  | | GHES2                    |   | | |    +----------------------------+      | |   |
  + +--------------------------+   | | |    |       .............        |      | |   |
  | | .................        |   | | |    +----------------------------+      | |   |
  | | error_status_address-----+---+ | | +->|     read_ack_registerN     |      | |   |
  | | .................        |     | | |  +----------------------------+      | |   |
  | | read_ack_register--------+-----+ | |  |通用错误状态块             1|<-----+ |   |
  | | read_ack_preserve        |       | |  |-+------------------------+-+        |   |
  | | read_ack_write           |       | |  | |          CPER          | |        |   |
  + +--------------------------|       | |  | |          CPER          | |        |   |
  | | ...............          |       | |  | |          ....          | |        |   |
  + +--------------------------+       | |  | |          CPER          | |        |   |
  | | GHESN                    |       | |  |-+------------------------+-|        |   |
  + +--------------------------+       | |  |通用错误状态块             2|<-------+   |
  | | .................        |       | |  |-+------------------------+-+            |
  | | error_status_address-----+-------+ |  | |           CPER         | |            |
  | | .................        |         |  | |           CPER         | |            |
  | | read_ack_register--------+---------+  | |           ....         | |            |
  | | read_ack_preserve        |            | |           CPER         | |            |
  | | read_ack_write           |            +-+------------------------+-+            |
  + +--------------------------+            |         ..........         |            |
                                            |----------------------------+            |
                                            |通用错误状态块             N |<----------+
                                            |-+-------------------------+-+
                                            | |          CPER           | |
                                            | |          CPER           | |
                                            | |          ....           | |
                                            | |          CPER           | |
                                            +-+-------------------------+-+


(1) QEMU 生成 ACPI HEST 表，放入现有 etc/acpi/tables fw_cfg 数据块。
    各错误源采用不同通知类型。

(2) 引入新的 etc/hardware_errors fw_cfg 数据块，也由 QEMU 填充，
    其中包含地址寄存器表和错误状态数据块表。

(3) 地址寄存器表包含 N 个 Error Block Address 和 N 个 Read Ack Register，
    每项 8 字节。错误状态数据块表包含 N 项，每项大小由源码的
    ACPI_GHES_MAX_RAW_DATA_LENGTH 定义，当前为 1024 字节。
    因而总大小为 (N * 8 * 2 + N * ACPI_GHES_MAX_RAW_DATA_LENGTH) 字节，
    N 为硬件错误源种类数。

(4) QEMU 为固件生成 ACPI 链接／加载脚本。
    固件为 etc/acpi/tables 和 etc/hardware_errors 预分配内存，并复制数据块内容。

(5) QEMU 生成 N 条 ADD_POINTER，修补 HEST 的 error_status_address，
    使其指向 etc/hardware_errors 中相应的地址寄存器。

(6) QEMU 生成 N 条 ADD_POINTER，修补 HEST 的 read_ack_register，
    使其指向 etc/hardware_errors 中相应的 read_ack_register。

(7) QEMU 为固件生成 N 条 ADD_POINTER，修补 error_block_address，
    使其指向 etc/hardware_errors 中对应的错误状态数据块。

(8) QEMU 定义第三个、只写的 fw_cfg 数据块，保存错误块偏移、读确认寄存器和
    CPER 记录的存储位置。

    QEMU 9.2 及以前位于 etc/hardware_errors_addr，
    保存 etc/hardware_errors 起始处的客户机物理地址。

    新版本位于 etc/acpi_table_hest_addr，指向 HEST 表的客户机物理地址。

    固件可用这些 fw_cfg 文件向 QEMU 返回客户机侧分配地址，每项 8 字节。
    QEMU 生成一条 WRITE_POINTER，固件在对应 fw_cfg 文件中写回
    etc/hardware_errors 或 HEST 表的起始地址。

(9) QEMU 收到内核 SIGBUS 后，将 CPER 写入客户机内存中对应的错误状态数据块，
    再注入平台专用中断作为必要通知；Arm virt 机器使用同步外部中止。

(10) 客户机内核处理此虚拟硬件通知，APEI 驱动读取 CPER 错误并采取相应措施。

(11) SIGBUS 触发时，kvm_arch_on_sigbus_vcpu() 通过 SEA 通知报告 RAS 错误。
