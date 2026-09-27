.. _disk images:

磁盘镜像
-----------

QEMU 支持多种磁盘镜像格式，包括按写入的非空扇区增长的可扩展镜像，
以及压缩和加密镜像。

.. _disk_005fimages_005fquickstart:

快速创建磁盘镜像
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

创建命令为::

   qemu-img create myimage.img mysize

myimage.img 是文件名，mysize 是以 KB 表示的大小。可加 M 后缀表示 MB，
或 G 表示 GB。

更多信息见 qemu-img 调用文档。

.. _disk_005fimages_005fsnapshot_005fmode:

快照模式
~~~~~~~~~~~~~

使用 ``-snapshot`` 时，所有磁盘镜像均视为只读，扇区写入保存在 /tmp 的
临时文件中。不过，可用监视器 commit 命令，或串口控制台 :kbd:`Ctrl+a s`，
强制将内容写回原始镜像。

.. _vm_005fsnapshots:

虚拟机快照
~~~~~~~~~~~~

虚拟机快照记录完整机器，包括 CPU、RAM、设备及所有可写磁盘。至少需要
一个采用 qcow2 的不可移除、可写块设备，通常是第一块虚拟硬盘。

监视器 savevm 创建快照或替换已有快照。除数字 ID 外，也可分配易读名称。

loadvm 恢复，delvm 删除，info snapshots 列出可用快照及信息::

   (qemu) info snapshots
   Snapshot devices: hda
   Snapshot list (from hda):
   ID        TAG                 VM SIZE                DATE       VM CLOCK
   1         start                   41M 2006-08-06 12:38:02   00:00:14.954
   2                                 40M 2006-08-06 12:43:29   00:00:18.633
   3         msys                    40M 2006-08-06 12:44:04   00:00:23.514

快照由虚拟机状态信息及各可写磁盘的快照组成。状态大小由 info snapshots
显示，保存在第一个不可移除、可写 qcow2 设备中；磁盘快照则各自保存在对应
镜像内。为节省空间，各快照共享扇区，因此难以估算单个磁盘快照大小，
info snapshots 不显示它，否则每份快照都需要完整复制所有磁盘。

使用与此不同的 ``-snapshot`` 选项时，仍可创建虚拟机快照，但退出 QEMU
即删除，见 :ref:`disk_005fimages_005fsnapshot_005fmode`。

目前已知限制：

-  无法处理快照之后发生移除或插入的可移动设备。

-  少数设备驱动的支持仍不完整，尤其是 USB，其状态不能正确保存或恢复。

.. _block-drivers:

.. include:: qemu-block-drivers.rst.inc
