Parallels 磁盘格式
========================================

..
   Copyright (c) 2015-2017, Virtuozzo, Inc.
   Authors:
        2015 Denis Lunev <den@openvz.org>
        2015 Vladimir Sementsov-Ogievskiy <vsementsov@virtuozzo.com>
        2016-2017 Klim Kireev <klim.kireev@virtuozzo.com>
        2016-2017 Edgar Kaziakhmedov <edgar.kaziakhmedov@virtuozzo.com>

   This work is licensed under the terms of the GNU GPL, version 2 or later.
   See the COPYING file in the top-level directory.

本文包含足以让 QEMU 正确处理 Parallels 磁盘的最小规范。
Parallels Cloud Server 和 Desktop 可添加未规定的内部 XML 节点，但不影响功能。
还可使用辅助 Snapshot.xml 保存可选快照信息，不影响打开、读取、写入。
QEMU 等软件不得使用本文未描述的字段或 Snapshot.xml，必须原样保留。

磁盘由快照集合及记录所有文件和快照信息的描述文件两部分组成。

定义
----------------------------------------

快照
  某个时间点捕获的内容记录，可保存当前状态，具有 UUID 和父 UUID。

快照镜像
  表示该快照相对某个早期快照差异的覆盖层。

覆盖层
  保存两个捕获状态之间不同扇区的镜像。

根镜像
  没有父级的快照镜像，是快照树根。

存储
  虚拟磁盘子范围的后备存储。一个磁盘含多个存储时称为拆分镜像，
  各存储覆盖特定地址空间并有独立根镜像。本文不考虑也不支持拆分镜像。
  每个存储含磁盘参数与镜像列表，始终含根镜像，也可含覆盖层。
  根镜像可为可扩展 Parallels 或普通格式，覆盖层必须可扩展。

描述文件
  DiskDescriptor.xml 保存磁盘参数、快照与存储信息。

顶层快照
  实际状态相对先前快照的覆盖层。
  它供客户机当前写入，并非经典意义的静态快照。

扇区
  512 字节数据块。

描述文件
----------------------------------------

全部信息置于单一 Parallels_disk_image 元素。
仅有 Version 属性，必须为 1.0。

DiskDescriptor.xml 模式::

 <Parallels_disk_image Version="1.0">
    <Disk_Parameters>
        ...
    </Disk_Parameters>
    <StorageData>
        ...
    </StorageData>
    <Snapshots>
        ...
    </Snapshots>
 </Parallels_disk_image>

``Disk_Parameters`` 元素
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

描述虚拟磁盘物理布局及通用设置，**必须** 包含：

* Disk_size：磁盘扇区数，即预期大小。
* Cylinders：柱面数。
* Heads：磁头数。
* Sectors：每柱面扇区数，扇区大小 512 字节。
  Heads、Sectors、Cylinders 的乘积 **必须** 等于 Disk_size。
* Padding：必须为 0。Parallels Cloud Server/Desktop 可能使用 1，
  但本规范不涵盖，QEMU 等软件不应打开或创建此类磁盘。

``StorageData`` 元素
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

描述根镜像及全部快照镜像，由 Storage 子元素组成::

 <StorageData>
    <Storage>
        ...
    </Storage>
 </StorageData>

Storage 包含：

* Start：起始扇区，非拆分存储为 0。
* End：末扇区之后的编号，非拆分存储等于 Disk_size。
* Blocksize：簇大小，即每簇扇区数。磁盘内每个 Compressed 镜像必须具有此簇大小。
  可扩展镜像的簇大小记录于头部 tracks，见 :doc:`parallels`。
* 多个 Image 子元素。

每个 Image 包含：

* GUID：带大括号的 UUID，例如 ``{12345678-9abc-def1-2345-6789abcdef12}.``
  Snapshots 用它引用镜像，见下文。
* Type：镜像类型。

  * Plain：raw 文件。
  * Compressed：可扩展磁盘。

* File：镜像路径，可相对 DiskDescriptor.xml，也可绝对路径。

``Snapshots`` 元素
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

描述快照树内的关系，包含一组 Shot::

 <Snapshots>
    <TopGUID> ... </TopGUID> /* Optional child element */
    <Shot>
        ...
    </Shot>
    <Shot>
        ...
    </Shot>
    ...
 </Snapshots>

每个 Shot 包含：

* GUID：镜像 GUID。
* ParentGUID：父快照镜像 GUID。

可通过 ParentGUID 从子级向父级遍历。根快照的父 GUID 为
``{00000000-0000-0000-0000-000000000000}``，只能有一个根。

顶层快照可由 Snapshots 的 TopGUID 子元素指定，
或使用预定义 ``{5fbaabe3-6958-40ff-92a7-860e329aab41}``。
若定义 TopGUID，预定义值就按普通 GUID 处理。
除顶层外，所有快照镜像应只读打开。

另有预定义 BackupID = {704718e1-2314-44c8-9087-d78ed36b0f4e}，
原软件及部分第三方软件用于备份。
QEMU 等可正常操作此 GUID 的镜像，但不建议为新磁盘使用它，顶层快照不得用此 GUID。
