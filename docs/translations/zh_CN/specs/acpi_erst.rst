ACPI ERST 设备
========================================

ACPI ERST 设备支持错误记录序列化表（ERST），
将错误记录保存到持久存储，供后续查阅或调试。

ACPI 规范 [1] 的 ACPI Platform Error Interfaces（APEI）章节，
尤其是 Error Serialization 小节，定义持久化存储错误记录的方法。

记录格式定义于 UEFI 规范 [2] 附录 N，Common Platform Error Record。

ACPI 允许直接向操作系统／客户机暴露非易失 RAM 的 NVRAM 模式，
见 GET_ERROR_LOG_ADDRESS_RANGE_ATTRIBUTES，但此设备实现非 NVRAM 模式。
多数 BIOS 也采用该模式，因为闪存内容更新需要编程操作。
撰写本文时，Linux 也仅支持非 NVRAM 模式。


背景与动机
----------------------------------------

Linux 用持久存储文件系统 pstore 在 panic 和关机时记录 dmesg 尾部等信息。
pstore 独立于 kdump，且先于它运行。主机或客户机根文件系统位于 NFS/iSCSI，
而网络软硬件故障导致 kdump 失败等情况下，pstore 可能保留事后调试信息。

pstore 常见后端为 ACPI ERST 和 UEFI。多数 BIOS 实现 ERST，但并非所有客户机都用 UEFI。
QEMU 支持 ERST 后，它就能像裸机一样成为虚拟机可用的 pstore 后端。

该支持提供一致的内核 panic 信息采集方法，适用于资源受限 microvm 到大型客户机，
尤其适用于没有 UEFI 运行时服务的直接启动环境。

Microsoft Windows 在 ERST 可用时，也用它保存部分崩溃信息 [3]。


配置与使用
----------------------------------------

创建 memory-backend-file 对象和 acpi-erst 设备，例如：

 qemu ...
 -object memory-backend-file,id=erstnvram,mem-path=acpi-erst.backing,size=0x10000,share=on \
 -device acpi-erst,memdev=erstnvram

正常工作要求后备对象具有以下参数：

 - id：用于将内存对象关联到 acpi-erst。
 - size：ERST 后备存储大小，必需。
 - mem-path：后备存储文件位置，必需。
 - share：必须 share=on，确保更新写入文件。

设备参数：

 - memdev：memory-backend-file 的对象 ID。
 - record_size：后备存储中记录或槽的大小，必须为不小于 4096（PAGE_SIZE）的 2 的幂。


PCI 接口
----------------------------------------

ERST 是带两个 BAR 的 PCI 设备，分别访问编程寄存器与记录交换缓冲区。

BAR0 包含 64 位 ACTION 和 VALUE 寄存器。
全部 ERST 操作及副作用都在写 ACTION 时发生，所需数据必须先写 VALUE。
读取 VALUE 仅返回其内容，该内容可能已被此前 ACTION 更新。

BAR1 包含 8KiB 记录交换缓冲区，即实现所支持的最大记录大小。


后备存储格式
----------------------------------------

存储分为固定 8KiB 的槽，每槽一个记录。槽不必全部占用，也不要求连续占用。
清除特定记录会产生空闲槽。

槽 0 保存存储头，标识 ERST 内容并支持高效访问。
根据总大小，头部可能占更多槽。8KiB 头可容纳 1021 条记录，
因此 8MiB（8KiB * 1024）存储需再用一个槽作头部。
此时槽 0、1 共同组成头部，记录从槽 2 开始。

以下示例存储小于 8MiB，大小为 8KiB 整数倍，含 N 个记录槽。
其中两条 CPER 记录已占用，其余空闲。

::

 槽     记录
        <------------------ 8KiB -------------------->
        +--------------------------------------------+
    0   | 存储头                                     |
        +--------------------------------------------+
    1   | 空闲／可用                                 |
        +--------------------------------------------+
    2   | CPER                                       |
        +--------------------------------------------+
    3   | CPER                                       |
        +--------------------------------------------+
  ...   |                                            |
        +--------------------------------------------+
    N   | 空闲／可用                                 |
        +--------------------------------------------+

存储头包含基本信息和 CPER record_id 数组，以高效访问记录。

头部全部字段均为小端。

::

  +--------------------------------------------+
  | magic                                      | 0x0000
  +--------------------------------------------+
  | record_offset        | record_size         | 0x0008
  +--------------------------------------------+
  | record_count         | reserved | version  | 0x0010
  +--------------------------------------------+
  | record_id[0]                               | 0x0018
  +--------------------------------------------+
  | record_id[1]                               | 0x0020
  +--------------------------------------------+
  | record_id[...]                             |
  +--------------------------------------------+
  | record_id[N]                               | 0x1FF8
  +--------------------------------------------+

magic 值为 0x524F545354535245。

record_size 为 0x2000，即 8KiB。

record_offset 指向数组第一项 record_id，即 0x0018。

version 为 0x0100，表示第一版本。

record_count 为后备存储中的有效记录数。

record_id 数组保存对应槽内 CPER 的 64 位记录标识。
其在数组中的位置就是对应记录的槽索引。

例如存储小于 8MiB 时，槽 0 为头部，record_id[0] 永不包含有效 CPER ID。
槽 1 才是第一个可用槽，因此 record_id_[1] 可能包含 CPER。

全 0 或全 1 的 record_id 表示无效记录，即该槽可用。


参考资料
----------------------------------------

[1] “Advanced Configuration and Power Interface Specification”，4.0，2009 年 6 月。

[2] “Unified Extensible Firmware Interface Specification”，2.1，2008 年 10 月。

[3] “Windows Hardware Error Architecture”，尤其是“Error Record Persistence Mechanism”。
