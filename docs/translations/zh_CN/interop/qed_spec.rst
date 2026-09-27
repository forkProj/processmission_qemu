============================================================
QED 镜像文件格式规范
============================================================

文件格式如下::

 +----------+----------+----------+-----+
 | cluster0 | cluster1 | cluster2 | ... |
 +----------+----------+----------+-----+

第一簇从 header 开始，记录普通簇的起始位置，
使头部可扩展并保存额外镜像信息。普通簇可为数据簇、L2 或 L1 表。
L1/L2 表由一个或多个连续簇组成。

通常文件大小是簇大小整数倍。若不是，最后簇之后的额外信息可能在写入时丢失。
合法附加信息应使用头部与首个普通簇之间的空间。

全部字段为小端。

文件头
----------------------------------------

::

 Header {
    uint32_t magic;               /* QED\0 */

    uint32_t cluster_size;        /* in bytes */
    uint32_t table_size;          /* for L1 and L2 tables, in clusters */
    uint32_t header_size;         /* in clusters */

    uint64_t features;            /* format feature bits */
    uint64_t compat_features;     /* compat feature bits */
    uint64_t autoclear_features;  /* self-resetting feature bits */

    uint64_t l1_table_offset;     /* in bytes */
    uint64_t image_size;          /* total logical image size, in bytes */

    /* if (features & QED_F_BACKING_FILE) */
    uint32_t backing_filename_offset; /* in bytes from start of header */
    uint32_t backing_filename_size;   /* in bytes */
 }

字段说明
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- cluster_size 必须为 [2^12, 2^26] 内的 2 的幂。
- table_size 必须为 [1, 16] 内的 2 的幂。
- header_size 是头部及普通簇前附加信息占用的簇数。
- features、compat_features、autoclear_features 为扩展位图：

  - 设置未知 features 时不得打开。不向后兼容的变更必须使用此类位。
  - 设置未知 compat_features 时可安全打开并忽略，它们表示向后兼容变更。
  - 设置未知 autoclear_features 时，清除未知位后可安全打开。
    这使向后兼容功能可平稳降级，日后由新程序重新启用。
- l1_table_offset 是 L1 第一字节的文件偏移，必须是 cluster_size 的整数倍。
- image_size 为客户机可见块设备大小，必须是 512 字节整数倍。
- backing_filename_offset 与 backing_filename_size 以字节偏移和大小描述字符串，
  不以 NUL 终止，无对齐限制，必须位于前 header_size 个簇内。
  后备文件名可为绝对路径，或相对镜像文件。

功能位
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- QED_F_BACKING_FILE = 0x01：使用后备文件。
- QED_F_NEED_CHECK = 0x02：使用前需要一致性检查。
- QED_F_BACKING_FORMAT_NO_PROBE = 0x04：后备为 raw，不应自动探测格式，
  避免文件碰巧包含魔数时被误判为其他镜像格式。

目前未定义 compat_features 或 autoclear_features 位。

受功能位控制的字段仅在对应位设置时使用，但无论是否设置，始终占据头部空间。

表
----------------------------------------

表将块设备逻辑偏移转换为文件簇偏移。

::

 #define TABLE_NOFFSETS (table_size * cluster_size / sizeof(uint64_t))

 Table {
     uint64_t offsets[TABLE_NOFFSETS];
 }

组织如下::

                    +----------+
                    | L1 表    |
                    +----------+
               ,------'  |  '------.
          +----------+   |    +----------+
          | L2 表    |  ...   | L2 表    |
          +----------+        +----------+
      ,------'  |  '------.
 +----------+   |    +----------+
 |   数据   |  ...   |   数据   |
 +----------+        +----------+

每表由一个或多个连续簇组成，table_size 决定大小。
例如 cluster_size=64 KB、table_size=4 时，表为 256 KB。

逻辑镜像大小不能超过 L1 可覆盖的最大簇总大小：

.. code::

 header.image_size <= TABLE_NOFFSETS * TABLE_NOFFSETS * header.cluster_size

L1、L2、数据簇偏移必须按 header.cluster_size 对齐。以下偏移有特殊含义。

L2 表偏移
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- 0：未分配 L2 表。

数据簇偏移
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- 0：未分配数据簇。
- 1：全零簇，未实际分配簇。

未来扩展可能保存逐偏移信息，最低 12 位为此保留，必须为零。
cluster_size 大于 2^12 时有更多未用位，也应清零。

未分配的 L2 表与数据簇
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

读取未分配区域时访问后备文件，无后备则返回零。
后备可小于镜像，超出后备末尾的未分配区域读取为零。

写入未分配区域会分配新数据簇，如 L2 也未分配则同时分配。
新簇由后备数据（无后备则为零）和正在写入的数据填充。

零数据簇
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

零簇可节省存储全零区域的空间。

读取返回零。

.. note::
    与未分配簇的区别是，零簇阻止读取后备文件内容。

写入零簇会分配新数据簇，用零及待写数据填充。

逻辑偏移转换
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

转换如下::

  table_bits table_bits    cluster_bits
  <--------> <--------> <--------------->
 +----------+----------+-----------------+
 | L1 index | L2 index |     byte offset |
 +----------+----------+-----------------+

       Structure of a logical offset

 offset_mask = ~(cluster_size - 1) # mask for the image file byte offset

 def logical_to_cluster_offset(l1_index, l2_index, byte_offset):
   l2_offset = l1_table[l1_index]
   l2_table = load_table(l2_offset)
   cluster_offset = l2_table[l2_index] & offset_mask
   return cluster_offset + byte_offset

一致性检查
----------------------------------------

本节提供信息性背景，说明 QED_F_NEED_CHECK 功能位。

执行可能因崩溃或断电留下不一致状态的操作前，用此位标记镜像为脏。
脏镜像打开时必须检查，因为元数据可能不一致。

检查以下不变量：

- 每簇必须且只能被引用一次。L1/L2 多次引用同簇属于不一致，无引用则为泄漏。
- 偏移必须位于文件大小内，并按 cluster_size 对齐。
- 表偏移距文件末尾至少为 table_size * cluster_size 字节，保证整表可容纳。

从 l1_table_offset 开始扫描全部 L2。
检查完成且除了泄漏外没有其他错误时，可清除 QED_F_NEED_CHECK 并访问镜像。
