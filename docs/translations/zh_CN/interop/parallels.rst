Parallels 可扩展镜像文件格式
============================================================

..
   Copyright (c) 2015 Denis Lunev
   Copyright (c) 2015 Vladimir Sementsov-Ogievskiy

   This work is licensed under the terms of the GNU GPL, version 2 or later.
   See the COPYING file in the top-level directory.


Parallels 可扩展镜像由三个连续部分组成：

* 文件头。
* BAT。
* 数据区。

全部数值采用小端字节序。


定义
----------------------------------------

扇区
  512 字节数据块。

簇
  大小由头部指定。当前默认 1MiB，即 2048 扇区。
  旧版本曾使用 63 扇区、256KB 和 252KB 簇。

BAT
  块分配表，包含客户机到主机 I/O 数据地址转换信息。

文件头
----------------------------------------

位于镜像开头，字段如下::

 Bytes:
   0 - 15:    magic
              必须为 "WithoutFreeSpace" 或 "WithouFreSpacExt"。

  16 - 19:    version
              必须为 2。

  20 - 23:    heads
              客户机磁盘几何参数。

  24 - 27:    cylinders
              客户机磁盘几何参数。

  28 - 31:    tracks
              簇大小，单位扇区。

  32 - 35:    nb_bat_entries
              磁盘大小，单位簇，即 BAT 大小。

  36 - 43:    nb_sectors
              磁盘大小，单位扇区。

              WithoutFreeSpace 仅使用低 4 字节，高 4 字节必须清零。
              WithouFreSpacExt 没有此限制。

  44 - 47:    in_use
              软件以读写模式打开时设为 0x746F6E59，关闭时设为 0x312e3276。

              零表示不支持下述格式扩展的旧软件打开了镜像。
              不允许其他值。

  48 - 51:    data_off
              文件起点到数据区起点的偏移，单位扇区。

              WithoutFreeSpace：
              - 为零时，按 BAT 末尾加填充计算，确保扇区对齐。
              - 非零时应按扇区对齐，但新镜像建议按簇对齐。

              WithouFreSpacExt：必须非零且按簇对齐。

  52 - 55:    flags
              各种标志。

              Bit 0: 空镜像位，设置时应视镜像为空。
              Bits 1-31: 未使用。

  56 - 63:    ext_off
              格式扩展偏移，单位扇区，从文件起点到格式扩展簇起点。
              必须满足下述 BAT 簇偏移的相同要求。

BAT
----------------------------------------

紧随头部，是连续的 32 位无符号小端整数数组，总大小 ``(bat_entries * 4)`` 字节。

每项是从文件开头到对应簇的偏移。
WithouFreSpacExt 以簇为单位，WithoutFreeSpace 以扇区为单位。

条目为零表示簇未分配，应视为全零。

簇偏移必须：

- 不小于数据偏移，即 header.data_off 或前述计算值。
- 小于预期文件大小。
- 在全部 BAT 条目中唯一。
- ``(cluster offset - data offset)`` 按簇大小对齐。

数据区
----------------------------------------

从数据偏移到文件末尾，表示连续簇数组。
多数由 BAT 分配，部分可由头部 ext_off 或扩展分配。
后两者分配的簇也必须满足 BAT 条目的同样要求。


格式扩展
----------------------------------------

一簇大小的区域，提供额外格式功能，由 ext_off 指向。格式如下::

   0 -  7:    magic
              必须为 0xAB234CEF23DCEA87。

   8 - 23:    m_CheckSum
              除前 24 字节之外，整个头扩展簇的 MD5。

随后是功能区段，即扩展，最后必须为 End of features。

每个区段格式::

   0 -  7:    magic
              功能标识：
              0x0000000000000000 - 功能结束
              0x20385FAE252CB34A - 脏位图

   8 - 15:    flags
              扩展外部标志：

              Bit 0: NECESSARY
                     因未知魔数或错误而无法加载时，不得修改文件。
                     未设置且加载出错时，应丢弃此扩展。

              Bit 1: TRANSIT
                     未知扩展设置此位时，应原样保留。

              NECESSARY 与 TRANSIT 都未设置时，应丢弃扩展。

  16 - 19:    data_size
              后续功能数据的字节大小。

  20 - 23:    unused32
              将头对齐到 8 字节边界。

  variable:   data (data_size bytes)

随后填充到下一个 8 字节边界，再开始下一扩展。

最后必须是全部字段为 0 的 End of features。


脏位图功能
----------------------------------------

提供在镜像中存储脏位图的能力，数据区字段如下::

   0 -  7:    size
              位图覆盖大小，应等于磁盘扇区数。

   8 - 23:    id
              备份一致性检查标识。

  24 - 27:    granularity
              位图粒度，单位扇区，即一位对应的扇区数，必须是 2 的幂。

  28 - 31:    l1_size
              位图 L1 表条目数。

  variable:   L1 偏移表 (l1_table)，大小：8 * l1_size 字节。

位图存于 Parallels 镜像内的一组簇，簇偏移保存在扩展指定的 L1 表中。
每项为 64 位整数。

给定位图数据字节偏移，对应项为::

    l1_table[offset / cluster_size]

项为 0，表示对应簇位图全部为 0。

项为 1，表示对应簇位图全部为 1。

其他值表示簇偏移，单位 512 字节扇区。
给定位图字节偏移，文件字节偏移为::

    offset = l1_table[offset / cluster_size] * 512 + (offset % cluster_size)
