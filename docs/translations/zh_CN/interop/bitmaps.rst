..
   Copyright 2019 John Snow <jsnow@redhat.com> and Red Hat, Inc.
   All rights reserved.

   This file is licensed via The FreeBSD Documentation License, the full
   text of which is included at the end of this document.

============================================================
脏位图与增量备份
============================================================

脏位图是在内存中跟踪块设备写入的对象，可配合多种块作业，实现增量或差异备份。

本文解释概念和机制，并完整介绍当前操作 API，希望说明为什么使用、是什么及如何使用。

面向为管理应用添加 QEMU 备份功能的开发者，以及直接通过 QMP 管理 QEMU 的高级用户。

.. contents::

概述
----------------------------------------

位图是位向量，每个 1 表示对应块设备的一段内容已修改，即脏。
跟踪区段大小称为粒度。例如粒度 64K 时，一个 1 表示整个 64K 区域可能发生变化，
即使只改动了一个字节。

粒度更小，跟踪更准确，但计算开销和位图体积更大；
粒度更大，位图更小，但备份范围不够精确。

位图字节大小计算如下：
    ``size`` = ceil(ceil(``image_size`` / ``granularity``) / 8)

例如 2TiB 镜像、64KiB 粒度：
    ``size`` = ((2147483648K / 64K) / 8)
         = 4194304B = 4MiB.

增量备份通过位图决定应复制哪些文件区段。
位图默认不启用，必须显式添加才开始记录。

可随时创建并附加到存储图任意块节点，
但通常附加在客户机存储设备模型连接的根节点最有意义。

也就是说，跟踪客户机磁盘写入通常最有用；
理论上也可附加到其他节点，跟踪 qcow2 元数据变化等，但不在本文范围。

QEMU 可通过 qcow2 将位图持久保存到磁盘，
这种保存或加载的位图称为持久位图，其他称为临时位图。

两种位图均支持热迁移，包括任意镜像格式的临时位图和 qcow2 持久位图。

.. _Supported Image Formats:

支持的镜像格式
----------------------------------------

qcow2 支持下文所有功能。

只有关闭时将位图写盘的持久化功能严格要求 qcow2。
无需持久化时，其余功能均可用于任意镜像格式。

例如 raw 镜像可使用脏位图，但退出时位图变化会丢失。

.. warning:: 临时位图不会在 QEMU 退出时保存！只有 qcow2 支持持久位图。

脏位图名称
----------------------------------------

API 需要引用位图，所有 API 创建管理的位图都有用户在创建时指定的可读名称。

- 名称在节点内唯一，不同节点可同名，因此通过 (node, name) 二元组定位。
- 用户创建的名称不能为空字符串。
- 临时位图支持 JSON Unicode 名称，实际无长度限制；QMP 可能限制消息小于 64MiB。
- 持久格式可有自己的名称和命名空间限制。目前仅 qcow2 支持，详情见 :doc:`qcow2`：

   - 名称长度为 1 到 1023 字节。
   - 同一 qcow2 文件内不得同名。

- QEMU 内部有时使用匿名位图，查询不可见，外部 API 不可操作，永不持久化或迁移。

位图状态
----------------------------------------

可用 `query-block <qemu-qmp-ref.html#command-QMP-block-core.query-block>`_ 查询，
结果通过 `BlockDirtyInfo <qemu-qmp-ref.html#object-QMP-block-core.BlockDirtyInfo>`_ 提供。

该结构含名称、粒度、脏字节数，以及布尔状态：

- recording：正在记录写入。
- busy：正被操作使用。
- persistent：持久位图。
- inconsistent：已损坏，不可使用。

备份使用位图或从迁移加载时会出现 +busy，禁止删除、清空或其他修改。
下文许多命令都会拒绝操作此类位图。

+inconsistent 同样禁止几乎所有操作，仅允许 block-dirty-bitmap-remove。

还有已弃用的 DirtyBitmapStatus 类型 status 字段，历史上有五种可见状态：

   #. Frozen：正被操作使用，不可删除、重命名、重置等。

      现在对应 +busy。

   #. Disabled：不记录新写入。

      现在对应 -recording -busy。

   #. Active：记录新写入。

      现在对应 +recording -busy。

   #. Locked：正被操作使用且不可变，与 Frozen 的区别主要是实现细节。

      现在对应 +busy。

   #. Inconsistent：持久位图未正确写盘，无法继续使用，留在内存仅用于表示失败。

      现在对应 +inconsistent。

旧状态已由布尔标志替代，不应继续使用。
Frozen 与 Locked 的内部实现差异不应影响外部用户。

基本 QMP 用法
----------------------------------------

位图主要通过 QMP 操作。协议见 :doc:`qmp-spec`，完整命令参考见 :doc:`qemu-qmp-ref`。

支持的命令
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

六个主要管理命令：

- block-dirty-bitmap-add
- block-dirty-bitmap-remove
- block-dirty-bitmap-clear
- block-dirty-bitmap-disable
- block-dirty-bitmap-enable
- block-dirty-bitmap-merge

另有相关查询：

- query-block

.. _`Creation: block-dirty-bitmap-add`:

创建：block-dirty-bitmap-add
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

`block-dirty-bitmap-add <qemu-qmp-ref.html#command-QMP-block-core.block-dirty-bitmap-add>`_：

创建跟踪指定节点写入的新位图，可设置粒度、持久化及记录状态。

.. admonition:: 示例

 创建启用记录的持久位图：

 .. code-block:: QMP

  -> { "execute": "block-dirty-bitmap-add",
       "arguments": {
         "node": "drive0",
         "name": "bitmap0",
         "persistent": true,
       }
     }

  <- { "return": {} }

- 默认粒度尽量匹配关联驱动器的簇大小，并限制在 4KiB–64KiB。
  当前 qcow2 默认 64KiB。

.. admonition:: 示例

 创建禁用记录、按 32KiB 跟踪的临时位图：

 .. code-block:: QMP

  -> { "execute": "block-dirty-bitmap-add",
       "arguments": {
         "node": "drive0",
         "name": "bitmap1",
         "granularity": 32768,
         "disabled": true
       }
     }

  <- { "return": {} }

删除：block-dirty-bitmap-remove
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

`block-dirty-bitmap-remove <qemu-qmp-ref.html#command-QMP-block-core.block-dirty-bitmap-remove>`_：

删除位图，+busy 时不可删除。

- 不影响同节点其他位图，也不影响从该位图或节点创建的已有备份。
- 名称仅在节点内唯一，因此必须指定节点／驱动器名。
- 删除持久位图也会从 qcow2 文件移除。

.. admonition:: 示例

 从 drive0 移除 bitmap0：

 .. code-block:: QMP

  -> { "execute": "block-dirty-bitmap-remove",
       "arguments": {
         "node": "drive0",
         "name": "bitmap0"
       }
     }

  <- { "return": {} }

重置：block-dirty-bitmap-clear
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

`block-dirty-bitmap-clear <qemu-qmp-ref.html#command-QMP-block-core.block-dirty-bitmap-clear>`_：

清除全部脏位，+busy 时不可清除。

- 从空位图创建增量备份不会复制数据，相当于没有变化。

.. admonition:: 示例

 清空 drive0 的 bitmap0：

 .. code-block:: QMP

  -> { "execute": "block-dirty-bitmap-clear",
       "arguments": {
         "node": "drive0",
         "name": "bitmap0"
       }
     }

  <- { "return": {} }

启用：block-dirty-bitmap-enable
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

`block-dirty-bitmap-enable <qemu-qmp-ref.html#command-QMP-block-core.block-dirty-bitmap-enable>`_：

将 recording 设为 true，开始记录写入。+busy 时不可启用。

- 未另行配置时，创建后默认启用。
- 已启用的持久位图加载时保留 +recording 状态。

.. admonition:: 示例

 为 drive0 的 bitmap0 设置 +recording：

 .. code-block:: QMP

  -> { "execute": "block-dirty-bitmap-enable",
       "arguments": {
         "node": "drive0",
         "name": "bitmap0"
       }
     }

  <- { "return": {} }

禁用：block-dirty-bitmap-disable
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

`block-dirty-bitmap-disable <qemu-qmp-ref.html#command-QMP-block-core.block-dirty-bitmap-disable>`_：

将 recording 设为 false，忽略后续写入。+busy 时不可禁用。

.. warning::

  存在风险：QEMU 不会因节点存在禁用位图而阻止写入，
  也不会在写入发生时把禁用位图标为 +inconsistent。
  从这样的位图创建的备份无法重建一致镜像。

- 禁用可用于检查特定时段修改了哪些扇区，或显式管理差异备份窗口。
- 禁用的持久位图加载时保留 -recording。

.. admonition:: 示例

 为 drive0 的 bitmap0 设置 -recording：

 .. code-block:: QMP

  -> { "execute": "block-dirty-bitmap-disable",
       "arguments": {
         "node": "drive0",
         "name": "bitmap0"
       }
     }

  <- { "return": {} }

合并与复制：block-dirty-bitmap-merge
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

`block-dirty-bitmap-merge <qemu-qmp-ref.html#command-QMP-block-core.block-dirty-bitmap-merge>`_：

将一个或多个源位图合并到目标。任意源为脏的区段，目标都标脏。

- 将多个源合为一个目标，目标包含所有源脏区段的并集。
- 不会自动创建不存在的目标，可预先创建空位图。
- 合并前不清空目标，连续合并会累积脏区段。
- 失败保证目标不变。源或目标忙、粒度不同等情况可失败。
- 只能合并同节点位图，命令只有一个 node 参数。
- 单个源合入空目标即可实现复制。

.. admonition:: 示例

 将 drive0 的 bitmap0 合入 new_bitmap。后者原为空时即复制。

 .. code-block:: QMP

  -> { "execute": "block-dirty-bitmap-merge",
       "arguments": {
         "node": "drive0",
         "target": "new_bitmap",
         "bitmaps": [ "bitmap0" ]
       }
     }

  <- { "return": {} }

查询：query-block
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

`query-block <qemu-qmp-ref.html#command-QMP-block-core.query-block>`_：

并非专用于位图，但会返回客户机设备根节点所附位图的信息。

- inconsistent 为 false 时不出现，仅为 true、表示问题时出现。

.. admonition:: 示例

 查询块子系统。以下 JSON 删去无关键，只突出位图部分，
 显示 drive0 根节点上的 bitmap0。

 .. code-block:: QMP

  -> {
       "execute": "query-block",
       "arguments": {}
     }

  <- {
       "return": [ {
         "dirty-bitmaps": [ {
           "status": "active",
           "count": 0,
           "busy": false,
           "name": "bitmap0",
           "persistent": false,
           "recording": true,
           "granularity": 65536
         } ],
         "device": "drive0",
       } ]
     }

位图持久化
----------------------------------------

如 `Supported Image Formats`_ 所述，QEMU 可将位图保存到 qcow2。
按 `Creation: block-dirty-bitmap-add`_ 的方法，传 persistent: true 即可。

加载时自动读入内存，关闭时自动写盘，使用基本透明。

但 QEMU 若未能正常关闭文件，下次加载时会标为 +inconsistent，
认为不安全，只允许 block-dirty-bitmap-remove。

丢失位图不会使已有备份失效，但该备份链无法再创建后续备份。

事务
----------------------------------------

QMP 事务允许同时提交多个命令，保证它们整体原子成功或失败。
下文展示位图与事务的配合。

详情见 `transaction <qemu-qmp-ref.html#command-QMP-transaction.transaction>`_。

.. _Justification:

使用理由
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

位图通常随时可改，但某些操作只有直接配合其他命令才合理。
虚拟机暂停时可确保命令间没有客户机写入；运行时，单独命令之间可能发生写入，难以保证。

例如仅使用单独命令：

#. 以暂停状态启动虚拟机。
#. 为 drive0 创建完整备份。
#. 创建 drive0 位图，确信期间没有写入。
#. 恢复虚拟机。
#. 稍后用 bitmap0 创建增量备份。

此时位图与完整备份同步，后续增量正确对应完整备份。

若虚拟机已运行一段时间，才决定开始增量备份，则上述办法不够实用。
此时希望：

#. 启动并运行虚拟机。
#. 通过单个事务：

   - 创建 bitmap0。
   - 创建 drive0 完整备份。

#. 稍后用 bitmap0 创建增量备份。

.. note:: 若 bitmap0 早于完整备份创建，仍可生成增量，
          但会额外复制备份开始前已修改的区段。
          事务收紧关键时间点，减少冗余，也确保不遗漏区段。

支持的位图事务
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- block-dirty-bitmap-add
- block-dirty-bitmap-clear
- block-dirty-bitmap-enable
- block-dirty-bitmap-disable
- block-dirty-bitmap-merge

用法与相应 QMP 命令相同，具体示例见下文。

增量备份：推送模式
------------------------------------------------

增量备份是部分磁盘镜像，在基础镜像上与其他部分镜像组合，
即可重建增量备份发起时刻的完整备份。

推送模式指 QEMU 将修改块推送到目标。使用
`blockdev-backup <qemu-qmp-ref.html#command-QMP-block-core.blockdev-backup>`_
创建完整和增量备份。

该命令是后台作业，有独立查询管理 API，见
`后台作业 <qemu-qmp-ref.html#background-jobs>`_。

示例：新的增量备份锚点
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

如事务 `Justification`_ 所述，可能需要为驱动器创建新的增量链。

此例为 drive0 创建完整备份，并创建空位图，从这一时刻开始记录写入。

目标可通过 `blockdev-add <qemu-qmp-ref.html#command-QMP-block-core.blockdev-add>`_
或 `blockdev-create <qemu-qmp-ref.html#command-QMP-block-core.blockdev-create>`_ 创建。

.. note:: 命令发出后的新写入，即使作业仍运行，也只写本地，不写入备份目标，
          并相应记录在位图中。

.. code-block:: QMP

  -> {
       "execute": "transaction",
       "arguments": {
         "actions": [
           {
             "type": "block-dirty-bitmap-add",
             "data": {
               "node": "drive0",
               "name": "bitmap0"
             }
           },
           {
             "type": "blockdev-backup",
             "data": {
               "device": "drive0",
               "target": "target0",
               "sync": "full"
             }
           }
         ]
       }
     }

  <- { "return": {} }

  <- {
       "timestamp": {
         "seconds": 1555436945,
         "microseconds": 179620
       },
       "data": {
         "status": "created",
         "id": "drive0"
       },
       "event": "JOB_STATUS_CHANGE"
     }

  ...

  <- {
       "timestamp": {...},
       "data": {
         "device": "drive0",
         "type": "backup",
         "speed": 0,
         "len": 68719476736,
         "offset": 68719476736
       },
       "event": "BLOCK_JOB_COMPLETED"
     }

  <- {
       "timestamp": {...},
       "data": {
         "status": "concluded",
         "id": "drive0"
       },
       "event": "JOB_STATUS_CHANGE"
     }

  <- {
       "timestamp": {...},
       "data": {
         "status": "null",
         "id": "drive0"
       },
       "event": "JOB_STATUS_CHANGE"
     }

作业转换与 JOB_STATUS_CHANGE 完整语义不在本文范围，后续示例省略。
上例为简洁也省略了其他若干事件。

.. note:: 后续仅保留 BLOCK_JOB_COMPLETED，除非需要其他事件说明流程差异。

          省略的事件和 JSON 对象用 ``...`` 表示。

示例：重置增量备份锚点
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

若要复用已有位图开始新链，可在创建完整备份的同一事务中清空位图：

.. code-block:: QMP

  -> {
       "execute": "transaction",
       "arguments": {
         "actions": [
         {
           "type": "block-dirty-bitmap-clear",
           "data": {
             "node": "drive0",
             "name": "bitmap0"
           }
         },
         {
           "type": "blockdev-backup",
           "data": {
             "device": "drive0",
             "target": "target0",
             "sync": "full"
           }
         }
       ]
     }
   }

  <- { "return": {} }

  ...

  <- {
       "timestamp": {...},
       "data": {
         "device": "drive0",
         "type": "backup",
         "speed": 0,
         "len": 68719476736,
         "offset": 68719476736
       },
       "event": "BLOCK_JOB_COMPLETED"
     }

  ...

结果与首例相同，只是清空已有位图而非新增。

.. tip:: 两例中 bitmap0 与新完整备份在概念上关联。
         QEMU 不保存此关系，由操作者或管理层记录位图对应哪个备份。

示例：第一次增量备份
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#. 任意方式创建完整备份并同步位图：

   - 上述两种在线备份方法之一。
   - 按 `Justification`_ 暂停虚拟机后使用 QMP。
   - 虚拟机离线时手工复制镜像，再以暂停状态启动，务必在运行前添加新位图。

   假定完成后：

   - 完整备份名为 drive0.full.qcow2。
   - 位图名为 bitmap0，附于 drive0。

#. 创建以完整备份为后备文件的增量目标镜像。

   - 假设新镜像名为 drive0.inc0.qcow2：

   .. code:: bash

       $ qemu-img create -f qcow2 drive0.inc0.qcow2 \
         -b drive0.full.qcow2 -F qcow2

#. 添加目标块节点：

   .. code-block:: QMP

    -> {
         "execute": "blockdev-add",
         "arguments": {
           "node-name": "target0",
           "driver": "qcow2",
           "file": {
             "driver": "file",
             "filename": "drive0.inc0.qcow2"
           }
         }
       }

    <- { "return": {} }

#. 发起增量备份：

   .. code-block:: QMP

    -> {
         "execute": "blockdev-backup",
         "arguments": {
           "device": "drive0",
           "bitmap": "bitmap0",
           "target": "target0",
           "sync": "incremental"
         }
       }

    <- { "return": {} }

    ...

    <- {
         "timestamp": {...},
         "data": {
           "device": "drive0",
           "type": "backup",
           "speed": 0,
           "len": 68719476736,
           "offset": 68719476736
         },
         "event": "BLOCK_JOB_COMPLETED"
       }

    ...

自完整备份以来修改的块被复制到 drive0.inc0.qcow2。
运行中 bitmap0 标为 +busy。成功后会清空，以遵循每次仅复制新增变化的增量模式。

.. note:: 作业开始后的新写入不复制到目标，备份时间点是开始而非结束。
          这些写入记录在特殊位图中，结束时合回 bitmap0，供下次增量复制。

示例：第二次增量备份
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#. 创建指向前一增量镜像的新目标，例如 drive0.inc1.qcow2。

   .. code:: bash

       $ qemu-img create -f qcow2 drive0.inc1.qcow2 \
         -b drive0.inc0.qcow2 -F qcow2

#. 添加目标块节点：

   .. code-block:: QMP

    -> {
         "execute": "blockdev-add",
         "arguments": {
           "node-name": "target0",
           "driver": "qcow2",
           "file": {
             "driver": "file",
             "filename": "drive0.inc1.qcow2"
           }
         }
       }

    <- { "return": {} }

#. 发起新增量，唯一差异是下方目标镜像改变。

   .. code-block:: QMP

    -> {
         "execute": "blockdev-backup",
         "arguments": {
           "device": "drive0",
           "bitmap": "bitmap0",
           "target": "target0",
           "sync": "incremental"
         }
       }

    <- { "return": {} }

    ...

    <- {
         "timestamp": {...},
         "data": {
           "device": "drive0",
           "type": "backup",
           "speed": 0,
           "len": 68719476736,
           "offset": 68719476736
         },
         "event": "BLOCK_JOB_COMPLETED"
       }

    ...

前一次成功后，bitmap0 已与 drive0.inc0.qcow2 同步。
再次使用它，创建基于上一次备份的新增量，形成三个镜像的链：

.. admonition:: 图示

 .. code:: text

   +-------------------+   +-------------------+   +-------------------+
   | drive0.full.qcow2 |<--| drive0.inc0.qcow2 |<--| drive0.inc1.qcow2 |
   +-------------------+   +-------------------+   +-------------------+

每次增量都将位图重新同步到最新备份，用户可持续消费位图，在已有链上新增备份。

图中 drive0.inc1.qcow2 和 drive0.inc0.qcow2 本身都不完整，
依赖后备链重建完整镜像，依赖最终止于完整备份。

链中每个备份独立且不受后来新增条目修改。
例如 drive0.inc0.qcow2 始终是其发起时刻的有效备份。

示例：不带后备文件的增量推送备份
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

备份最好异地保存，因此通常无法直接链接到链中已有文件。
备份时这不是问题，只需创建目标时不设置后备镜像。

#. 创建不带后备文件的目标。因 QEMU 无法从后备文件推导大小，必须指定基础镜像大小。

   .. code:: bash

       $ qemu-img create -f qcow2 drive0.inc2.qcow2 64G

   .. note:: 也可省略推送命令的 ``mode: "existing"``，让 QEMU 创建无后备文件镜像，
             但会失去对兼容性和预分配等格式选项的控制。

#. 添加目标块节点：

   .. code-block:: QMP

    -> {
         "execute": "blockdev-add",
         "arguments": {
           "node-name": "target0",
           "driver": "qcow2",
           "file": {
             "driver": "file",
             "filename": "drive0.inc2.qcow2"
           }
         }
       }

    <- { "return": {} }

#. 发起增量。除新目标镜像外，与前两例没有区别。

   .. code-block:: QMP

    -> {
         "execute": "blockdev-backup",
         "arguments": {
           "device": "drive0",
           "bitmap": "bitmap0",
           "target": "target0",
           "sync": "incremental"
         }
       }

    <- { "return": {} }

    ...

    <- {
         "timestamp": {...},
         "data": {
           "device": "drive0",
           "type": "backup",
           "speed": 0,
           "len": 68719476736,
           "offset": 68719476736
         },
         "event": "BLOCK_JOB_COMPLETED"
       }

    ...

对用户而言，唯一区别是恢复时需要设置后备镜像：

.. code:: bash

    $ qemu-img rebase drive0.inc2.qcow2 \
      -u -b drive0.inc1.qcow2

这里使用不安全 rebase，只将后备文件设置为尚不存在的文件。

也可用 --image-opts 在运行时手动将整个后备链作为临时属性指定，但超出本文范围。

示例：多驱动器增量备份
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

假设虚拟机有 drive0 和 drive1，希望两份备份对应同一崩溃一致性时间点。

#. 为各驱动器创建空镜像：

   .. code:: bash

    $ qemu-img create -f qcow2 drive0.full.qcow2 64G
    $ qemu-img create -f qcow2 drive1.full.qcow2 64G

#. 添加目标块节点：

   .. code-block:: QMP

    -> {
         "execute": "blockdev-add",
         "arguments": {
           "node-name": "target0",
           "driver": "qcow2",
           "file": {
             "driver": "file",
             "filename": "drive0.full.qcow2"
           }
         }
       }

    <- { "return": {} }

    -> {
         "execute": "blockdev-add",
         "arguments": {
           "node-name": "target1",
           "driver": "qcow2",
           "file": {
             "driver": "file",
             "filename": "drive1.full.qcow2"
           }
         }
       }

    <- { "return": {} }

#. 分别创建完整锚点备份及配套位图：

   .. code-block:: QMP

    -> {
         "execute": "transaction",
         "arguments": {
           "actions": [
             {
               "type": "block-dirty-bitmap-add",
               "data": {
                 "node": "drive0",
                 "name": "bitmap0"
               }
             },
             {
               "type": "block-dirty-bitmap-add",
               "data": {
                 "node": "drive1",
                 "name": "bitmap0"
               }
             },
             {
               "type": "blockdev-backup",
               "data": {
                 "device": "drive0",
                 "target": "target0",
                 "sync": "full"
               }
             },
             {
               "type": "blockdev-backup",
               "data": {
                 "device": "drive1",
                 "target": "target1",
                 "sync": "full"
               }
             }
           ]
         }
       }

    <- { "return": {} }

    ...

    <- {
         "timestamp": {...},
         "data": {
           "device": "drive0",
           "type": "backup",
           "speed": 0,
           "len": 68719476736,
           "offset": 68719476736
         },
         "event": "BLOCK_JOB_COMPLETED"
       }

    ...

    <- {
         "timestamp": {...},
         "data": {
           "device": "drive1",
           "type": "backup",
           "speed": 0,
           "len": 68719476736,
           "offset": 68719476736
         },
         "event": "BLOCK_JOB_COMPLETED"
       }

    ...

#. 稍后为各增量创建目标，分别指向对应完整备份：

   .. code:: bash

     $ qemu-img create -f qcow2 drive0.inc0.qcow2 \
       -b drive0.full.qcow2 -F qcow2
     $ qemu-img create -f qcow2 drive1.inc0.qcow2 \
       -b drive1.full.qcow2 -F qcow2

#. 添加目标块节点：

   .. code-block:: QMP

    -> {
         "execute": "blockdev-add",
         "arguments": {
           "node-name": "target0",
           "driver": "qcow2",
           "file": {
             "driver": "file",
             "filename": "drive0.inc0.qcow2"
           }
         }
       }

    <- { "return": {} }

    -> {
         "execute": "blockdev-add",
         "arguments": {
           "node-name": "target1",
           "driver": "qcow2",
           "file": {
             "driver": "file",
             "filename": "drive1.inc0.qcow2"
           }
         }
       }

    <- { "return": {} }

#. 发起多驱动器增量推送事务：

   .. code-block:: QMP

    -> {
         "execute": "transaction",
         "arguments": {
           "actions": [
             {
               "type": "blockev-backup",
               "data": {
                 "device": "drive0",
                 "bitmap": "bitmap0",
                 "sync": "incremental",
                 "target": "target0"
               }
             },
             {
               "type": "blockdev-backup",
               "data": {
                 "device": "drive1",
                 "bitmap": "bitmap0",
                 "sync": "incremental",
                 "target": "target1"
               }
             },
           ]
         }
       }

    <- { "return": {} }

    ...

    <- {
         "timestamp": {...},
         "data": {
           "device": "drive0",
           "type": "backup",
           "speed": 0,
           "len": 68719476736,
           "offset": 68719476736
         },
         "event": "BLOCK_JOB_COMPLETED"
       }

    ...

    <- {
         "timestamp": {...},
         "data": {
           "device": "drive1",
           "type": "backup",
           "speed": 0,
           "len": 68719476736,
           "offset": 68719476736
         },
         "event": "BLOCK_JOB_COMPLETED"
       }

    ...

推送备份错误与恢复
------------------------------------------------

无论作业由独立 QMP 命令还是事务成功启动，若之后失败，
用户会收到带失败消息的 BLOCK_JOB_COMPLETE，以及 BLOCK_JOB_ERROR。

取消时改为收到 BLOCK_JOB_CANCELLED，不会收到 COMPLETE/ERROR 对。

两种失败情况下，位图都不会清空，保留开始时全部脏位及运行期间新增脏位。

因此，位图记录差异的基准时间仍为上次成功增量的发起时刻，
而不是前移到这次失败备份的开始时刻。

解决底层问题后，例如为目标增加空间，可用同一位图重试。

示例：单独失败
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

单个增量推送作业失败时遵循上述规则：

#. 创建目标镜像：

   .. code:: bash

       $ qemu-img create -f qcow2 drive0.inc0.qcow2 \
         -b drive0.full.qcow2 -F qcow2

#. 添加目标块节点：

   .. code-block:: QMP

    -> {
         "execute": "blockdev-add",
         "arguments": {
           "node-name": "target0",
           "driver": "qcow2",
           "file": {
             "driver": "file",
             "filename": "drive0.inc0.qcow2"
           }
         }
       }

    <- { "return": {} }

#. 通过 QMP 尝试增量备份：

   .. code-block:: QMP

    -> {
         "execute": "blockdev-backup",
         "arguments": {
           "device": "drive0",
           "bitmap": "bitmap0",
           "target": "target0",
           "sync": "incremental"
         }
       }

    <- { "return": {} }

#. 收到两个失败事件：

   .. code-block:: QMP

    <- {
         "timestamp": {...},
         "data": {
           "device": "drive0",
           "action": "report",
           "operation": "write"
         },
         "event": "BLOCK_JOB_ERROR"
       }

    <- {
         "timestamp": {...},
         "data": {
           "speed": 0,
           "offset": 0,
           "len": 67108864,
           "error": "No space left on device",
           "device": "drive0",
           "type": "backup"
         },
         "event": "BLOCK_JOB_COMPLETED"
       }

#. 移除目标节点：

   .. code-block:: QMP

    -> {
         "execute": "blockdev-del",
         "arguments": {
           "node-name": "target0",
         }
       }

    <- { "return": {} }

#. 删除失败镜像并重新创建。

   .. code:: bash

       $ rm drive0.inc0.qcow2
       $ qemu-img create -f qcow2 drive0.inc0.qcow2 \
         -b drive0.full.qcow2 -F qcow2

#. 添加目标块节点：

   .. code-block:: QMP

    -> {
         "execute": "blockdev-add",
         "arguments": {
           "node-name": "target0",
           "driver": "qcow2",
           "file": {
             "driver": "file",
             "filename": "drive0.inc0.qcow2"
           }
         }
       }

    <- { "return": {} }

#. 修复底层问题，例如释放备份卷空间后，重试：

   .. code-block:: QMP

    -> {
         "execute": "blockdev-backup",
         "arguments": {
           "device": "drive0",
           "bitmap": "bitmap0",
           "target": "target0",
           "sync": "incremental"
         }
       }

    <- { "return": {} }

#. 收到成功完成确认：

   .. code-block:: QMP

    <- {
         "timestamp": {...},
         "data": {
           "device": "drive0",
           "type": "backup",
           "speed": 0,
           "len": 67108864,
           "offset": 67108864
         },
         "event": "BLOCK_JOB_COMPLETED"
       }

.. _`Example: Partial Transactional Failures`:

示例：事务部分失败
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

`blockdev-backup <qemu-qmp-ref.html#command-QMP-block-core.blockdev-backup>`_
从概念上只是启动作业，因此事务可能成功，而作业稍后失败。
这与直觉中的事务行为可能不同。

含启动命令的事务返回成功后，关联作业仍可能失败。
管理应用必须能处理成功事务之后的部分备份失败。

同一事务指定多个作业时，某个失败默认不会影响其他作业。
成功作业清空其位图，失败作业不清空。
因此，即使部分失败，也不能安全删除已经成功生成的增量备份。

以下两个作业，一个成功、一个失败：

#. 发起同时备份两盘的事务：

   .. code-block:: QMP

    -> {
         "execute": "transaction",
         "arguments": {
           "actions": [
           {
             "type": "blockdev-backup",
             "data": {
               "device": "drive0",
               "bitmap": "bitmap0",
               "sync": "incremental",
               "target": "target0"
             }
           },
           {
             "type": "blockdev-backup",
             "data": {
               "device": "drive1",
               "bitmap": "bitmap0",
               "sync": "incremental",
               "target": "target1"
             }
           }]
         }
       }

#. 收到事务接受、作业启动通知：

   .. code-block:: QMP

    <- { "return": {} }

#. 收到首个作业完成：

   .. code-block:: QMP

    <- {
         "timestamp": {...},
         "data": {
           "device": "drive0",
           "type": "backup",
           "speed": 0,
           "len": 67108864,
           "offset": 67108864
         },
         "event": "BLOCK_JOB_COMPLETED"
       }

#. 收到第二个作业失败：

   .. code-block:: QMP

    <- {
         "timestamp": {...},
         "data": {
           "device": "drive1",
           "action": "report",
           "operation": "read"
         },
         "event": "BLOCK_JOB_ERROR"
       }

    ...

    <- {
         "timestamp": {...},
         "data": {
           "speed": 0,
           "offset": 0,
           "len": 67108864,
           "error": "Input/output error",
           "device": "drive1",
           "type": "backup"
         },
         "event": "BLOCK_JOB_COMPLETED"
       }

最终 drive0.inc0.qcow2 有效，必须保留；drive1.inc0.qcow2 不完整，应删除。
若要创建同一时间点覆盖整个虚拟机的增量，需为两盘重新备份，
且 drive0 的新增量必须基于 drive0.inc0.qcow2。

此例只有 drive0 有新增量，没有 drive1，因此最新的虚拟机整体崩溃一致性备份仍为完整备份：

.. code:: text

          [drive0.full.qcow2] <-- [drive0.inc0.qcow2]
          [drive1.full.qcow2]

修复方法是再次对两盘发起增量，得到类似以下的链：

.. code:: text

          [drive0.full.qcow2] <-- [drive0.inc0.qcow2] <-- [drive0.inc1.qcow2]
          [drive1.full.qcow2] <-------------------------- [drive1.inc1.qcow2]

示例：分组完成模式
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

事务启动的作业通常独立成功或失败，也可要求作为一组共同完成或失败。
事务的可选 properties 结构可改变此行为。

completion-mode 可为默认的 individual，或下述 grouped。

grouped 模式中，只有全部准备成功才报告成功；任一失败，其他全部取消。

无论增量作业失败还是取消，其位图都保持原有时间点，与单独失败相同。

以下复用 `Example: Partial Transactional Failures`_ 的多盘场景，但设置 grouped：

#. 发起多盘增量事务：

   .. code-block:: QMP

    -> {
         "execute": "transaction",
         "arguments": {
           "properties": {
             "completion-mode": "grouped"
           },
           "actions": [
           {
             "type": "blockdev-backup",
             "data": {
               "device": "drive0",
               "bitmap": "bitmap0",
               "sync": "incremental",
               "target": "target0"
             }
           },
           {
             "type": "blockdev-backup",
             "data": {
               "device": "drive1",
               "bitmap": "bitmap0",
               "sync": "incremental",
               "target": "target1"
             }
           }]
         }
       }

#. 收到接受与启动通知：

   .. code-block:: QMP

    <- { "return": {} }

#. 收到 drive1 备份失败：

   .. code-block:: QMP

    <- {
         "timestamp": {...},
         "data": {
           "device": "drive1",
           "action": "report",
           "operation": "read"
         },
         "event": "BLOCK_JOB_ERROR"
       }

    <- {
         "timestamp": {...},
         "data": {
           "speed": 0,
           "offset": 0,
           "len": 67108864,
           "error": "Input/output error",
           "device": "drive1",
           "type": "backup"
         },
         "event": "BLOCK_JOB_COMPLETED"
       }

#. 收到 drive0 作业取消：

   .. code-block:: QMP

    <- {
         "timestamp": {...},
         "data": {
           "device": "drive0",
           "type": "backup",
           "speed": 0,
           "len": 67108864,
           "offset": 16777216
         },
         "event": "BLOCK_JOB_CANCELLED"
       }

此例两个作业均因失败中止，两份目标镜像都无用，应删除。

稍后可直接重新发起整个事务。

.. raw:: html

   <!--
   The FreeBSD Documentation License

   Redistribution and use in source (ReST) and 'compiled' forms (SGML, HTML,
   PDF, PostScript, RTF and so forth) with or without modification, are
   permitted provided that the following conditions are met:

   Redistributions of source code (ReST) must retain the above copyright notice,
   this list of conditions and the following disclaimer of this file unmodified.

   Redistributions in compiled form (transformed to other DTDs, converted to
   PDF, PostScript, RTF and other formats) must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS DOCUMENTATION IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS DOCUMENTATION, EVEN IF ADVISED OF
   THE POSSIBILITY OF SUCH DAMAGE.
   -->
