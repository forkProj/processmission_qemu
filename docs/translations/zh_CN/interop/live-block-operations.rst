..
    Copyright (C) 2017 Red Hat Inc.

    This work is licensed under the terms of the GNU GPL, version 2 or
    later.  See the COPYING file in the top-level directory.

.. _Live Block Operations:

============================================================
在线块设备操作
============================================================

截至 QEMU 2.9，块层支持四类主要在线作业：stream、commit、mirror 和 backup。
它们可操作磁盘镜像链：将后备文件数据在线复制到覆盖层；
将覆盖层合入后备文件以缩短长链；将含活动磁盘的镜像链在线同步到另一目标；
以及创建块设备的时间点备份和增量备份。
以下介绍这些 QMP 原语，并以非穷尽示例说明用法。

.. note::
    源码 qapi/block-core.json 包含这些 QMP 原语的权威 QAPI 模式文档。

.. todo (kashyapc):: 集成 Sphinx 后移除 contents 指令。

.. contents::

磁盘后备链表示法
------------------------------------------------

简单镜像链，可在线用 blockdev-snapshot-sync，或离线用 qemu-img 创建::

                   (运行中 QEMU)
                        |
                        .
                        V

            [A] <----- [B]

    (后备文件)        (覆盖层)

箭头表示 A 是 B 的后备文件。运行中的 QEMU 写 B，因此 B 也称活动层。

描述链中文件常用两类术语：

(1) 方向术语 base 与 top：上例 A 为 base，B 为 top。
    QAPI 模式 block-core.json 使用此术语。

(2) 关系术语 backing file 与 overlay：A 为后备文件，B 为覆盖层。

   本文采用关系术语。

.. important::
    覆盖层通常可用任何支持后备文件的格式，QCOW2 是首选，也是本文所用格式。


在线块 QMP 原语概览
------------------------------------------------------------

四类操作如下：

(1) block-stream：在线将后备文件数据复制到覆盖层。

    .. note:: 完成后注意：

                (a) QEMU 重写后备链，移除对已复制且冗余后备文件的引用。
                (b) 不删除文件本身，须由用户显式丢弃。
                (c) 文件仍有效，可基于它创建更多覆盖层，详情见后文。

(2) block-commit：在线将覆盖层数据合入后备文件，可选择从链中移除覆盖层。
    自 QEMU 2.0 起支持活动提交，即将当前活动层合入基础镜像。

    .. note:: 完成后同样注意：

                (a) QEMU 重写链，移除已提交到后备文件的冗余覆盖层引用。
                (b) 不删除文件本身，需手动删除。
                (c) 与 stream 不同，中间镜像会失效，不能再基于它们创建覆盖层。
                    详情见后文。

(3) drive-mirror 和 blockdev-mirror：将运行中的磁盘同步到另一镜像。

(4) blockdev-backup 和已弃用的 drive-backup：将块设备在线复制为目标时间点备份。


.. _`Interacting with a QEMU instance`:

与 QEMU 实例交互
------------------------------------------------

以下命令启动 QEMU，并通过 UNIX 套接字提供 QMP：

.. parsed-literal::

  $ |qemu_system| -display none -no-user-config -nodefaults \\
    -m 512 -blockdev \\
    node-name=node-A,driver=qcow2,file.driver=file,file.node-name=file,file.filename=./a.qcow2 \\
    -device virtio-blk,drive=node-A,id=virtio0 \\
    -monitor stdio -qmp unix:/tmp/qmp-sock,server=on,wait=off

-blockdev 自 QEMU 2.9 可用。node-name 将 a.qcow2 命名为 node-A，
比直接指定文件路径更清晰。后续通过 blockdev-snapshot-sync 或 blockdev-add
创建的每个链节点也分配 node-name，操作时尽量使用节点名。
截至 2.9，block-commit 尚不接受 node-name，是例外。

使用源码 qemu/scripts/qmp 中的 qmp-shell 与实例交互，它接受 QMP 键值参数。
以下调用还打印完整原始 JSON，供后续示例参考::

    $ ./qmp-shell -v -p /tmp/qmp-sock
    (QEMU)

.. note::
    重复命令首次同时展示 qmp-shell 与 JSON，后续仅展示 qmp-shell，省略等效 JSON。


示例磁盘镜像链
----------------------------------------

后续使用以下链，必要时重复列出::

    [A] <-- [B] <-- [C] <-- [D]

A 为原始基础镜像，B、C 为中间覆盖层，D 为活动层，QEMU 正向其写入。
一般规律是 QEMU 始终指向链的最右镜像。

通过 qmp-shell 调用 blockdev-snapshot-sync 创建，以下展示 B 及原始 JSON::

    (QEMU) blockdev-snapshot-sync node-name=node-A snapshot-file=b.qcow2 snapshot-node-name=node-B format=qcow2
    {
        "execute": "blockdev-snapshot-sync",
        "arguments": {
            "node-name": "node-A",
            "snapshot-file": "b.qcow2",
            "format": "qcow2",
            "snapshot-node-name": "node-B"
        }
    }

node-A 是 QEMU 内部引用 A 的名称，B 以它为后备文件。

继续创建 C、D，省略 JSON::

    (QEMU) blockdev-snapshot-sync node-name=node-B snapshot-file=c.qcow2 snapshot-node-name=node-C format=qcow2
    (QEMU) blockdev-snapshot-sync node-name=node-C snapshot-file=d.qcow2 snapshot-node-name=node-D format=qcow2


时间点与文件名的区别
------------------------------------------------

在链中::

    [A] <-- [B] <-- [C] <-- [D]

有三个历史时间点和一个活动层：

- 时间点 1：创建 B 时的客户机状态在 A。
- 时间点 2：创建 C 时在 A+B。
- 时间点 3：创建 D 时在 A+B+C。
- 当前活动状态在 A+B+C+D。

命名时注意：

- 按创建时间命名文件可能误导，因为该时间点的客户机数据不在该文件中。
- 应将文件视为相对后备文件的增量。


在线块流式复制：``block-stream``
------------------------------------------------------------

将后备文件数据在线复制到覆盖层。

从原始链开始::

    [A] <-- [B] <-- [C] <-- [D]

可用多种方式缩短，以下并非全部情况。

.. _`Case-1`:

(1) 客户机运行期间，将 A、B、C 全部合入活动层 D。
    结果为包含全部内容、供 QEMU 写入的独立 D::

        [D]

.. _`Case-2`:

(2) 仅将 B、C 合入 D，再将 D 的后备指针改为 A。结果::

        [A] <-- [D]

.. _`Case-3`:

(3) 中间流式复制，自 QEMU 2.8 可用。
    从四文件原链将 B 复制到 C，完成后可选丢弃 B，C 改指向 A。
    QEMU 仍写 D，结果::

        [A] <-- [C] <-- [D]


``block-stream`` 的 QMP 调用
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

`Case-1`_ 中，node-D 为活动镜像，默认复制全部后备文件、将整链扁平化。
qmp-shell 及 JSON::

    (QEMU) block-stream device=node-D job-id=job0
    {
        "execute": "block-stream",
        "arguments": {
            "device": "node-D",
            "job-id": "job0"
        }
    }

`Case-2`_ 将 B、C 合入 D，D 改用 A 为后备::

    (QEMU) block-stream device=node-D base-node=node-A job-id=job0

`Case-3`_ 将 B 合入 C，C 改用 A 为后备::

    (QEMU) block-stream device=node-C base-node=node-A job-id=job0

可通过以下命令监视进度::

    (QEMU) query-block-jobs
    {
        "execute": "query-block-jobs",
        "arguments": {}
    }

完成后发 BLOCK_JOB_COMPLETED。
中间覆盖层仍有效，可丢弃，也可保留并继续创建覆盖层。
block-stream 作业可随时重新启动。


在线块提交：``block-commit``
------------------------------------------------------------

将覆盖层数据在线合入后备文件，方向与 stream 相反。
自 2.0 支持活动提交，可将 QEMU 正在写入的最右活动层合入基础镜像。

从原始链重新开始，QEMU 写 D::

    [A] <-- [B] <-- [C] <-- [D]

可按以下方式缩短：

.. _`block-commit_Case-1`:

(1) 仅将 B 提交到 A，C 改指向 A::

        [A] <-- [C] <-- [D]

(2) 将 B、C 提交到 A，D 改指向 A::

        [A] <-- [D]

.. _`block-commit_Case-3`:

(3) 将 B、C 和活动 D 提交到 A，形成单一镜像::

        [A]

(4) 仅将 C 提交到 B::

        [A] <-- [B] <-- [D]

(5) 将 C 和活动 D 提交到 B::

        [A] <-- [B]


``block-commit`` 的 QMP 调用
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:ref:`情况 1 <block-commit_Case-1>` 仅将 B 合入 A::

    (QEMU) block-commit device=node-D base=a.qcow2 top=b.qcow2 job-id=job0
    {
        "execute": "block-commit",
        "arguments": {
            "device": "node-D",
            "job-id": "job0",
            "top": "b.qcow2",
            "base": "a.qcow2"
        }
    }

完成后发 BLOCK_JOB_COMPLETED，无需更多操作。
C 的后备改为 A，原四镜像链变为::

    [A] <-- [C] <-- [D]

.. note::
    中间 B 失效，不能再基于它创建覆盖层。

    stream 后的中间镜像仍代表旧时间点，因此可能有效；
    commit 后则不再代表任何时间点，在任何上下文都无效。

:ref:`情况 3 <block-commit_Case-3>`，即活动提交，是两阶段操作。
首先将活动和中间覆盖层复制到基础后备文件；
然后通过 block-job-complete 将该文件设为活动镜像。
也可用 block-job-cancel 取消，但必须谨慎。

同步结束发 BLOCK_JOB_READY，此时可用 block-job-complete 正常完成。
发出完成命令前，提交仍保持活动。

以下流程将::

    [A] <-- [B] <-- [C] <-- [D]

转为::

    [A]

即将 B、C、D 全部提交到 A，此后 QEMU 向 A 写入。

启动活动提交::

    (QEMU) block-commit device=node-D base=a.qcow2 top=d.qcow2 job-id=job0
    {
        "execute": "block-commit",
        "arguments": {
            "device": "node-D",
            "job-id": "job0",
            "top": "d.qcow2",
            "base": "a.qcow2"
        }
    }

同步完成发 BLOCK_JOB_READY。

可选查询活动作业，ready: true 表明可以完成::

    (QEMU) query-block-jobs
    {
        "execute": "query-block-jobs",
        "arguments": {}
    }
    {
        "return": [
            {
                "busy": false,
                "type": "commit",
                "len": 1376256,
                "paused": false,
                "ready": true,
                "io-status": "ok",
                "offset": 1376256,
                "device": "job0",
                "speed": 0
            }
        ]
    }

正常完成提交作业::

    (QEMU) block-job-complete device=job0
    {
        "execute": "block-job-complete",
        "arguments": {
            "device": "job0"
        }
    }
    {
        "return": {}
    }

完成后发 BLOCK_JOB_COMPLETED。

.. note::
    为简洁，省略前述情况 2、4、5 的调用。


在线磁盘同步：``drive-mirror`` 与 ``blockdev-mirror``
--------------------------------------------------------------------------------

将运行中镜像链的全部或部分同步到目标。

给定熟悉的链::

    [A] <-- [B] <-- [C] <-- [D]

drive-mirror 及较新的等效 blockdev-mirror，可将整链复制到单个目标 E，目标可在另一主机。

.. note::

    同步完成前取消 mirror，block-job-cancel 发 BLOCK_JOB_CANCELLED；
    已通过 BLOCK_JOB_READY 表示同步完成后再取消，则发 BLOCK_JOB_COMPLETED。

    除 mirror 外，只有活动 block-commit 发 READY。
    stream、非活动 commit 和 backup 均自动结束。

READY 之后有两种操作：

(1) block-job-cancel 完成并发 COMPLETED 后，得到整链或仅顶层（sync=top）的时间点副本。
    时间点为触发取消之时，常用于非共享存储热迁移。

(2) block-job-complete 完成并发 COMPLETED 后，将客户机设备转向目标 E，后续写入均到 E。

同步模式决定复制链的哪部分，目前四种：

(1) full：全部链内容。
(2) top：仅最上层镜像。
(3) none：仅此后新写入。

    .. note:: backup 的 none 行为不同。
              通常 backup 一方面在客户机覆盖数据前先将旧数据复制到备份，
              另一方面后台从头到尾复制全镜像。sync=none 仅执行前一部分。

(4) incremental：复制脏位图所描述内容。

.. note::
    incremental 详情见 :doc:`bitmaps`。


``drive-mirror`` 的 QMP 调用
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

将 A 至 D 整链复制到 E。目标不存在时 drive-mirror 会创建::

    (QEMU) drive-mirror device=node-D target=e.qcow2 sync=full job-id=job0
    {
        "execute": "drive-mirror",
        "arguments": {
            "device": "node-D",
            "job-id": "job0",
            "target": "e.qcow2",
            "sync": "full"
        }
    }

sync: full 表示复制整链。

查询会看到 mirror 已 ready，同时 QEMU 发 BLOCK_JOB_READY::

    (QEMU) query-block-jobs
    {
        "execute": "query-block-jobs",
        "arguments": {}
    }
    {
        "return": [
            {
                "busy": false,
                "type": "mirror",
                "len": 21757952,
                "paused": false,
                "ready": true,
                "io-status": "ok",
                "offset": 21757952,
                "device": "job0",
                "speed": 0
            }
        ]
    }

此时如前所述有两种操作：

(a) 结束同步，得到结束时刻的副本，E 包含 A 至 D 整链::

        (QEMU) block-job-cancel device=job0
        {
            "execute": "block-job-cancel",
            "arguments": {
                "device": "job0"
            }
        }

(b) 或完成并将运行中的 QEMU 切换到目标::

        (QEMU) block-job-complete device=job0

两种情况之后，再次 query-block-jobs 都不应有活动作业。

commit 与 mirror 都可丢弃覆盖层，但 commit 修改已有基础镜像，
而 mirror 将链内容填充到新目标。


通过 ``drive-mirror`` 与 NBD 在线迁移存储的 QMP 调用
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

无共享存储的在线迁移，是 drive-mirror 与 QEMU 内置 NBD 服务器的常见用途。
以下简要展示设置流程。

给定链::

    [A] <-- [B] <-- [C] <-- [D]

不复制整链，而仅将最上层活动 D 同步到 TargetDisk。

.. important::
    目标主机必须已通过 cp、rsync 或存储阵列专用命令等方式获得 A、B、C 内容。

这也称浅复制，因为只复制活动层。

.. note::
    为简单起见，此例源和目标都位于 localhost。

假设目标已有 A 至 C 内容，例如 Contents-of-A-B-C.qcow2。
创建以它为后备的目标覆盖层，用于接收源 D 的镜像::

    $ qemu-img create -f qcow2 -b ./Contents-of-A-B-C.qcow2 \
        -F qcow2 ./target-disk.qcow2

源 QEMU 已按 `Interacting with a QEMU instance`_ 启动。
目标启动如下，此处同主机只是简化，也可在其他主机：

.. parsed-literal::

  $ |qemu_system| -display none -no-user-config -nodefaults \\
    -m 512 -blockdev \\
    node-name=node-TargetDisk,driver=qcow2,file.driver=file,file.node-name=file,file.filename=./target-disk.qcow2 \\
    -device virtio-blk,drive=node-TargetDisk,id=virtio0 \\
    -S -monitor stdio -qmp unix:./qmp-sock2,server=on,wait=off \\
    -incoming tcp:localhost:6666

源端链为::

    [A] <-- [B] <-- [C] <-- [D]

目标应已有 A、B、C，所以只复制 D。

(1) 在目标 QEMU 指定主机和端口启动内置 NBD，``::`` 表示本地主机::

        (QEMU) nbd-server-start addr={"type":"inet","data":{"host":"::","port":"49153"}}
        {
            "execute": "nbd-server-start",
            "arguments": {
                "addr": {
                    "data": {
                        "host": "::",
                        "port": "49153"
                    },
                    "type": "inet"
                }
            }
        }

(2) 在目标 QEMU 导出目标镜像::

        (QEMU) nbd-server-add device=node-TargetDisk writable=true
        {
            "execute": "nbd-server-add",
            "arguments": {
                "device": "node-TargetDisk"
            }
        }

(3) 在源 QEMU 调用 drive-mirror，mode=existing 表示同步到目标预建文件，sync=top::

        (QEMU) drive-mirror device=node-D target=nbd:localhost:49153:exportname=node-TargetDisk sync=top mode=existing job-id=job0
        {
            "execute": "drive-mirror",
            "arguments": {
                "device": "node-D",
                "mode": "existing",
                "job-id": "job0",
                "target": "nbd:localhost:49153:exportname=node-TargetDisk",
                "sync": "top"
            }
        }

(4) 源端复制完毕并发 READY 后，用 block-job-cancel 正常结束同步::

        (QEMU) block-job-cancel device=job0
        {
            "execute": "block-job-cancel",
            "arguments": {
                "device": "job0"
            }
        }

(5) 在目标停止 NBD::

        (QEMU) nbd-server-stop
        {
            "execute": "nbd-server-stop",
            "arguments": {}
        }

(6) 在目标通过 cont 恢复 vCPU::

        (QEMU) cont
        {
            "execute": "cont",
            "arguments": {}
        }

.. note::
    libvirt 等高层库可自动完成以上流程，
    但 libvirt 因其他原因不允许同主机迁移到 localhost。


关于 ``blockdev-mirror``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

核心功能等同 drive-mirror，但在 BDS 图的节点层面操作。

目标必须先用 qemu-img 显式创建，再通过 blockdev-add 附加到运行中的 QEMU，
为新目标节点分配名称。

将整链备份到目标的操作顺序例如：

(0) 创建所需深度的 QCOW2 覆盖链。
(1) 用 qemu-img 创建目标，如 e.qcow2。
(2) 运行时通过 blockdev-add 附加目标。
(3) 执行 blockdev-mirror，sync: full 复制整链，观察 BLOCK_JOB_READY。
(4) 可选查询，应该有已准备完成的 mirror 作业。
(5) 正常完成作业，观察 BLOCK_JOB_COMPLETED。
(6) 用 quit 关闭客户机，使缓存刷新。
(7) 用 qemu-img compare 比较链与目标，应显示 Images are identical。


``blockdev-mirror`` 的 QMP 调用
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

给定链::

    [A] <-- [B] <-- [C] <-- [D]

将 A 至 D 整链复制到新目标 E，流程如下。

创建 B、C、D::

    (QEMU) blockdev-snapshot-sync node-name=node-A snapshot-file=b.qcow2 snapshot-node-name=node-B format=qcow2
    (QEMU) blockdev-snapshot-sync node-name=node-B snapshot-file=c.qcow2 snapshot-node-name=node-C format=qcow2
    (QEMU) blockdev-snapshot-sync node-name=node-C snapshot-file=d.qcow2 snapshot-node-name=node-D format=qcow2

创建目标 E::

    $ qemu-img create -f qcow2 e.qcow2 39M

通过 blockdev-add 附加目标::

    (QEMU) blockdev-add driver=qcow2 node-name=node-E file={"driver":"file","filename":"e.qcow2"}
    {
        "execute": "blockdev-add",
        "arguments": {
            "node-name": "node-E",
            "driver": "qcow2",
            "file": {
                "driver": "file",
                "filename": "e.qcow2"
            }
        }
    }

执行 blockdev-mirror，观察 BLOCK_JOB_READY::

    (QEMU) blockdev-mirror device=node-B target=node-E sync=full job-id=job0
    {
        "execute": "blockdev-mirror",
        "arguments": {
            "device": "node-D",
            "job-id": "job0",
            "target": "node-E",
            "sync": "full"
        }
    }

查询作业，应有已 ready 的 mirror::

    (QEMU) query-block-jobs
    {
        "execute": "query-block-jobs",
        "arguments": {}
    }
    {
        "return": [
            {
                "busy": false,
                "type": "mirror",
                "len": 21561344,
                "paused": false,
                "ready": true,
                "io-status": "ok",
                "offset": 21561344,
                "device": "job0",
                "speed": 0
            }
        ]
    }

正常完成，观察 BLOCK_JOB_COMPLETED::

    (QEMU) block-job-complete device=job0
    {
        "execute": "block-job-complete",
        "arguments": {
            "device": "job0"
        }
    }
    {
        "return": {}
    }

用 quit 关闭客户机::

    (QEMU) quit
    {
        "execute": "quit",
        "arguments": {}
    }


在线磁盘备份：``blockdev-backup`` 与已弃用的 ``drive-backup``
--------------------------------------------------------------------------------

blockdev-backup 及已弃用的 drive-backup 可创建时间点快照。

时间点是命令开始执行时。


``drive-backup`` 的 QMP 调用
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

drive-backup 自 QEMU 6.2 起弃用，未来将移除。

再次从示例链开始::

    [A] <-- [B] <-- [C] <-- [D]

创建包含 A 至 D 内容的目标 E；目标不存在时 drive-backup 会创建::

    (QEMU) drive-backup device=node-D sync=full target=e.qcow2 job-id=job0
    {
        "execute": "drive-backup",
        "arguments": {
            "device": "node-D",
            "job-id": "job0",
            "sync": "full",
            "target": "e.qcow2"
        }
    }

完成后发 BLOCK_JOB_COMPLETED，无需进一步操作。


从 ``drive-backup`` 迁移到 ``blockdev-backup``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

区别在于如何指定目标。blockdev-backup 不接受文件名，而使用已有块节点的 node-name，
节点可通过 blockdev-add 或 blockdev-create 添加。
因此它没有不适用于已有节点的 mode 和 format 参数。细节和示例见下文。


关于 ``blockdev-backup``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

在块驱动状态图 BDS 的节点层面操作。

将整链备份到目标的流程例如：

(0) 创建所需深度的 QCOW2 覆盖链。
(1) 用 qemu-img 创建 e.qcow2 等目标。
(2) 运行时通过 blockdev-add 附加。
(3) 执行 blockdev-backup，sync: full 复制整链，观察 BLOCK_JOB_COMPLETED。
(4) quit 关闭客户机，刷新缓存。
(5) qemu-img compare 比较，应显示 Images are identical。

下一节展示具体调用。

``blockdev-backup`` 的 QMP 调用
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

给定一层覆盖链，B 为 QEMU 正在写入的活动层::

    [A] <-- [B]

将 A、B 全部内容复制到目标 E，步骤如下。

创建覆盖层 B::

    (QEMU) blockdev-snapshot-sync node-name=node-A snapshot-file=b.qcow2 snapshot-node-name=node-B format=qcow2
    {
        "execute": "blockdev-snapshot-sync",
        "arguments": {
            "node-name": "node-A",
            "snapshot-file": "b.qcow2",
            "format": "qcow2",
            "snapshot-node-name": "node-B"
        }
    }

创建备份目标::

    $ qemu-img create -f qcow2 e.qcow2 39M

通过 blockdev-add 附加::

    (QEMU) blockdev-add driver=qcow2 node-name=node-E file={"driver":"file","filename":"e.qcow2"}
    {
        "execute": "blockdev-add",
        "arguments": {
            "node-name": "node-E",
            "driver": "qcow2",
            "file": {
                "driver": "file",
                "filename": "e.qcow2"
            }
        }
    }

调用 blockdev-backup 将 A、B 整链复制到 e.qcow2::

    (QEMU) blockdev-backup device=node-B target=node-E sync=full job-id=job0
    {
        "execute": "blockdev-backup",
        "arguments": {
            "device": "node-B",
            "job-id": "job0",
            "target": "node-E",
            "sync": "full"
        }
    }

成功完成发 BLOCK_JOB_COMPLETED。

查询活动作业，此时应该没有::

    (QEMU) query-block-jobs
    {
        "execute": "query-block-jobs",
        "arguments": {}
    }

关闭客户机::

    (QEMU) quit
    {
            "execute": "quit",
                "arguments": {}
    }
            "return": {}
    }

.. note::
    这一步很重要。遗漏时，qemu-img compare 验证备份完整性会报告
    “Failed to get shared "write" lock on e.qcow2”。

最终 e.qcow2 包含命令发起时 A、B 的时间点备份。

可比较确认内容相同，应看到 Images are identical。
此处假定 QEMU 以 -S 启动，客户机启动时 CPU 尚未运行::

    $ qemu-img compare b.qcow2 e.qcow2
    Warning: Image size mismatch!
    Images are identical.

注意：Image size mismatch 警告符合预期，因为目标 e.qcow2 创建为 39M。
