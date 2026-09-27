.. SPDX-License-Identifier: GPL-2.0-or-later

QEMU COLO 容错
=========================

本文概述 COLO 的设计及用法。

背景
----------

虚拟机复制是广为人知的技术，用软件实现与应用无关的硬件容错，也称为“不停机服务”。

COLO（COarse-grained LOck-stepping，粗粒度锁步）是一种高可用方案。主虚拟机（PVM）和备虚拟机（SVM）并行运行，接收相同的客户端请求，并行生成响应。如果 PVM 和 SVM 的响应数据包相同，就立即放行；否则按需执行虚拟机检查点。

架构
------------

COLO 架构如下图所示，由一对联网物理节点组成：主节点运行 PVM，备节点运行 SVM，以维护 PVM 的有效副本。PVM 和 SVM 并行执行，按应用语义为客户端请求生成响应数据包。

来自客户端或外部网络的入站数据包由主节点接收，再转发到备节点，使 PVM 和 SVM 收到相同请求。

COLO 接收 PVM 和 SVM 的出站数据包，并在允许输出发送给客户端之前比较它们。

只要 SVM 对全部客户端请求生成相同响应，就视为 PVM 的有效副本。一旦检测到 PVM 和 SVM 的输出存在差异，COLO 就暂停发送出站数据包，直到成功将 PVM 状态同步到 SVM。

概览::

      Primary Node                                                            Secondary Node
    +------------+  +-----------------------+       +------------------------+  +------------+
    |            |  |       HeartBeat       +<----->+       HeartBeat        |  |            |
    | Primary VM |  +-----------+-----------+       +-----------+------------+  |Secondary VM|
    |            |              |                               |               |            |
    |            |  +-----------|-----------+       +-----------|------------+  |            |
    |            |  |QEMU   +---v----+      |       |QEMU  +----v---+        |  |            |
    |            |  |       |Failover|      |       |      |Failover|        |  |            |
    |            |  |       +--------+      |       |      +--------+        |  |            |
    |            |  |   +---------------+   |       |   +---------------+    |  |            |
    |            |  |   | VM Checkpoint +-------------->+ VM Checkpoint |    |  |            |
    |            |  |   +---------------+   |       |   +---------------+    |  |            |
    |Requests<--------------------------\ /-----------------\ /--------------------->Requests|
    |            |  |                   ^ ^ |       |       | |              |  |            |
    |Responses+---------------------\ /-|-|------------\ /-------------------------+Responses|
    |            |  |               | | | | |       |  | |  | |              |  |            |
    |            |  | +-----------+ | | | | |       |  | |  | | +----------+ |  |            |
    |            |  | | COLO disk | | | | | |       |  | |  | | | COLO disk| |  |            |
    |            |  | |   Manager +---------------------------->| Manager  | |  |            |
    |            |  | ++----------+ v v | | |       |  | v  v | +---------++ |  |            |
    |            |  |  |+-----------+-+-+-++|       | ++-+--+-+---------+ |  |  |            |
    |            |  |  ||   COLO Proxy     ||       | |   COLO Proxy    | |  |  |            |
    |            |  |  || (compare packet  ||       | |(adjust sequence | |  |  |            |
    |            |  |  ||and mirror packet)||       | |    and ACK)     | |  |  |            |
    |            |  |  |+------------+---+-+|       | +-----------------+ |  |  |            |
    +------------+  +-----------------------+       +------------------------+  +------------+
    +------------+     |             |   |                                |     +------------+
    | VM Monitor |     |             |   |                                |     | VM Monitor |
    +------------+     |             |   |                                |     +------------+
    +---------------------------------------+       +----------------------------------------+
    |   Kernel         |             |   |  |       |   Kernel            |                  |
    +---------------------------------------+       +----------------------------------------+
                       |             |   |                                |
        +--------------v+  +---------v---+--+       +------------------+ +v-------------+
        |   Storage     |  |External Network|       | External Network | |   Storage    |
        +---------------+  +----------------+       +------------------+ +--------------+

组件
^^^^^^^^^^

COLO 架构图包含多个组件，功能如下。

心跳
~~~~~~~~~

在主、备节点上运行，定期检查平台可用性。如果主节点遭遇硬件故障停机，心跳会停止响应，备节点一旦确认心跳丢失，就会触发故障转移。

COLO 磁盘管理器
~~~~~~~~~~~~~~~~~

主虚拟机向镜像写入数据时，COLO 磁盘管理器捕获这些数据并发送到备虚拟机，确保备虚拟机镜像内容与主虚拟机一致。更多详情请参阅 docs/block-replication.txt。

检查点/故障转移控制器
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

修改保存和恢复流程，实现持续迁移，确保备端虚拟机状态始终与主端一致。

COLO 代理
~~~~~~~~~~

将数据包递送到主、备端，比较两端响应，再按规则决定是否启动检查点。更多信息请参阅 docs/colo-proxy.txt。

注意：
心跳尚未实现，因此需要使用 'x-colo-lost-heartbeat' 命令触发故障转移流程。

COLO 运行状态
^^^^^^^^^^^^^^^^^^^^^

概览::

    +-----------------+
    |                 |
    |    Start COLO   |
    |                 |
    +--------+--------+
             |
             |  Main qmp command:
             |  migrate-set-capabilities with x-colo
             |  migrate
             |
             v
    +--------+--------+
    |                 |
    |  COLO running   |
    |                 |
    +--------+--------+
             |
             |  Main qmp command:
             |  x-colo-lost-heartbeat
             |  or
             |  some error happened
             v
    +--------+--------+
    |                 |  send qmp event:
    |  COLO failover  |  COLO_EXIT
    |                 |
    +-----------------+



COLO 使用 QMP 命令切换和报告运行状态。图中只显示主要 QMP 命令，详情见测试流程。

测试流程
--------------

环境设置
^^^^^^^^

这里为测试在同一主机上运行两个实例。如果希望在两台主机上运行，请更改 IP 地址。初始状态下，``127.0.0.1`` 为主主机，``127.0.0.2`` 为备主机。

COLO 在备端使用客户机 RAM 大小两倍的内存。两台主机上的 QEMU 版本应相同。

启动 QEMU
^^^^^^^^^^^^

**1. 主端**：
首先，需要将 ``$imagefolder/primary.qcow2`` 复制到所有主机。这里无需更改 IP，因为 ``0.0.0.0`` 监听所有接口。使用 ``127.0.0.1`` 地址的字符设备回环连接到本地 QEMU 实例::

    # imagefolder="/mnt/vms/colo-test-primary"

    # qemu-system-x86_64 -enable-kvm -cpu qemu64,kvmclock=on -m 512 -smp 1 -qmp stdio \
       -device piix3-usb-uhci -device usb-tablet -name primary \
       -netdev tap,id=hn0,vhost=off,helper=/usr/lib/qemu/qemu-bridge-helper \
       -device rtl8139,id=e0,netdev=hn0 \
       -chardev socket,id=mirror0,host=0.0.0.0,port=9003,server=on,wait=off \
       -chardev socket,id=compare1,host=0.0.0.0,port=9004,server=on,wait=on \
       -chardev socket,id=compare0,host=127.0.0.1,port=9001,server=on,wait=off \
       -chardev socket,id=compare0-0,host=127.0.0.1,port=9001 \
       -chardev socket,id=compare_out,host=127.0.0.1,port=9005,server=on,wait=off \
       -chardev socket,id=compare_out0,host=127.0.0.1,port=9005 \
       -object filter-mirror,id=m0,netdev=hn0,queue=tx,outdev=mirror0 \
       -object filter-redirector,netdev=hn0,id=redire0,queue=rx,indev=compare_out \
       -object filter-redirector,netdev=hn0,id=redire1,queue=rx,outdev=compare0 \
       -object iothread,id=iothread1 \
       -object colo-compare,id=comp0,primary_in=compare0-0,secondary_in=compare1,\
    outdev=compare_out0,iothread=iothread1 \
       -drive if=ide,id=colo-disk0,driver=quorum,read-pattern=fifo,vote-threshold=1,\
    children.0.file.filename=$imagefolder/primary.qcow2,children.0.driver=qcow2 -S



**2. 备端**：
活动镜像和隐藏镜像只需创建一次，大小应与 ``primary.qcow2`` 相同。同样，除 ``$primary_ip`` 变量外，这里无需更改任何 IP::

    # imagefolder="/mnt/vms/colo-test-secondary"
    # primary_ip=127.0.0.1

    # qemu-img create -f qcow2 $imagefolder/secondary-active.qcow2 10G

    # qemu-img create -f qcow2 $imagefolder/secondary-hidden.qcow2 10G

    # qemu-system-x86_64 -enable-kvm -cpu qemu64,kvmclock=on -m 512 -smp 1 -qmp stdio \
       -device piix3-usb-uhci -device usb-tablet -name secondary \
       -netdev tap,id=hn0,vhost=off,helper=/usr/lib/qemu/qemu-bridge-helper \
       -device rtl8139,id=e0,netdev=hn0 \
       -chardev socket,id=red0,host=$primary_ip,port=9003,reconnect-ms=1000 \
       -chardev socket,id=red1,host=$primary_ip,port=9004,reconnect-ms=1000 \
       -object filter-redirector,id=f1,netdev=hn0,queue=tx,indev=red0 \
       -object filter-redirector,id=f2,netdev=hn0,queue=rx,outdev=red1 \
       -object filter-rewriter,id=rew0,netdev=hn0,queue=all \
       -drive if=none,id=parent0,file.filename=$imagefolder/primary.qcow2,driver=qcow2 \
       -drive if=none,id=childs0,driver=replication,mode=secondary,file.driver=qcow2,\
    top-id=colo-disk0,file.file.filename=$imagefolder/secondary-active.qcow2,\
    file.backing.driver=qcow2,file.backing.file.filename=$imagefolder/secondary-hidden.qcow2,\
    file.backing.backing=parent0 \
       -drive if=ide,id=colo-disk0,driver=quorum,read-pattern=fifo,vote-threshold=1,\
    children.0=childs0 \
       -incoming tcp:0.0.0.0:9998



**3.** 在备虚拟机的 QEMU 监视器中执行命令::

    {"execute":"qmp_capabilities"}
    {"execute": "migrate-set-capabilities", "arguments": {"capabilities": [ {"capability": "return-path", "state": true }, {"capability": "x-colo", "state": true } ] } }
    {"execute": "nbd-server-start", "arguments": {"addr": {"type": "inet", "data": {"host": "0.0.0.0", "port": "9999"} } } }
    {"execute": "nbd-server-add", "arguments": {"device": "parent0", "writable": true } }


注意：
  a. 必须先运行 QMP 命令 ``nbd-server-start`` 和 ``nbd-server-add``，再在主端 QEMU 中运行 QMP 的 migrate 命令。
  b. 活动磁盘、隐藏磁盘和 NBD 目标的长度应相同。
  c. 最好将活动磁盘和隐藏磁盘放在 RAM 磁盘中。故障转移时，它们会合并到父磁盘。

**4.** 在主虚拟机的 QEMU 监视器中执行命令::

    {"execute":"qmp_capabilities"}
    {"execute": "blockdev-add", "arguments": {"driver": "nbd", "node-name": "nbd0", "server": {"type": "inet", "host": "127.0.0.2", "port": "9999"}, "export": "parent0", "detect-zeroes": "on"} }
    {"execute": "x-blockdev-change", "arguments":{"parent": "colo-disk0", "node": "nbd0" } }
    {"execute": "migrate-set-capabilities", "arguments": {"capabilities": [ {"capability": "return-path", "state": true }, {"capability": "x-colo", "state": true } ] } }
    {"execute": "migrate", "arguments": {"uri": "tcp:127.0.0.2:9998" } }


注意：
  a. 每个主磁盘只能有一个 NBD 客户端。
  b. 必须先在备端 QEMU 运行 QMP 命令行，再运行这里的 QMP 命令行。

**5.** 完成上述步骤后，每当更改 PVM，SVM 都会同步。可以执行 ``{ "execute": "migrate-set-parameters" , "arguments":{ "x-checkpoint-delay": 2000 } }`` 更改空闲检查点周期。

故障转移测试
^^^^^^^^^^^^^

可以终止其中一台虚拟机，并在存活的虚拟机上执行故障转移：

如果终止了备端，遵循“主端故障转移”。之后若希望恢复复制，遵循“主端恢复复制”。

如果终止了主端，遵循“备端故障转移”。之后若希望恢复复制，遵循“备端恢复复制”。

主端故障转移
~~~~~~~~~~~~~~~~

备端已停止，在主端恢复运行::

    {"execute": "x-blockdev-change", "arguments":{ "parent": "colo-disk0", "child": "children.1"} }
    {"execute": "blockdev-del", "arguments": {"node-name": "nbd0"} }
    {"execute": "object-del", "arguments":{ "id": "comp0" } }
    {"execute": "object-del", "arguments":{ "id": "iothread1" } }
    {"execute": "object-del", "arguments":{ "id": "m0" } }
    {"execute": "object-del", "arguments":{ "id": "redire0" } }
    {"execute": "object-del", "arguments":{ "id": "redire1" } }
    {"execute": "x-colo-lost-heartbeat" }


备端故障转移
~~~~~~~~~~~~~~~~~~

主端已停止，在备端恢复运行，并准备成为新主端::

    {"execute": "nbd-server-stop"}
    {"execute": "x-colo-lost-heartbeat"}

    {"execute": "object-del", "arguments":{ "id": "f2" } }
    {"execute": "object-del", "arguments":{ "id": "f1" } }
    {"execute": "chardev-remove", "arguments":{ "id": "red1" } }
    {"execute": "chardev-remove", "arguments":{ "id": "red0" } }

    {"execute": "chardev-add", "arguments":{ "id": "mirror0", "backend": {"type": "socket", "data": {"addr": { "type": "inet", "data": { "host": "0.0.0.0", "port": "9003" } }, "server": true } } } }
    {"execute": "chardev-add", "arguments":{ "id": "compare1", "backend": {"type": "socket", "data": {"addr": { "type": "inet", "data": { "host": "0.0.0.0", "port": "9004" } }, "server": true } } } }
    {"execute": "chardev-add", "arguments":{ "id": "compare0", "backend": {"type": "socket", "data": {"addr": { "type": "inet", "data": { "host": "127.0.0.1", "port": "9001" } }, "server": true } } } }
    {"execute": "chardev-add", "arguments":{ "id": "compare0-0", "backend": {"type": "socket", "data": {"addr": { "type": "inet", "data": { "host": "127.0.0.1", "port": "9001" } }, "server": false } } } }
    {"execute": "chardev-add", "arguments":{ "id": "compare_out", "backend": {"type": "socket", "data": {"addr": { "type": "inet", "data": { "host": "127.0.0.1", "port": "9005" } }, "server": true } } } }
    {"execute": "chardev-add", "arguments":{ "id": "compare_out0", "backend": {"type": "socket", "data": {"addr": { "type": "inet", "data": { "host": "127.0.0.1", "port": "9005" } }, "server": false } } } }


主端恢复复制
~~~~~~~~~~~~~~~~~~~~~~~~~~

新备端启动后恢复复制。

启动新备端（上述步骤 2 和 3），然后在主端执行::

    {"execute": "drive-mirror", "arguments":{ "device": "colo-disk0", "job-id": "resync", "target": "nbd://127.0.0.2:9999/parent0", "mode": "existing", "format": "raw", "sync": "full"} }


等待磁盘同步完成，然后执行::

    {"execute": "stop"}
    {"execute": "block-job-cancel", "arguments":{ "device": "resync"} }

    {"execute": "blockdev-add", "arguments": {"driver": "nbd", "node-name": "nbd0", "server": {"type": "inet", "host": "127.0.0.2", "port": "9999"}, "export": "parent0", "detect-zeroes": "on"} }
    {"execute": "x-blockdev-change", "arguments":{ "parent": "colo-disk0", "node": "nbd0" } }

    {"execute": "object-add", "arguments":{ "qom-type": "filter-mirror", "id": "m0", "netdev": "hn0", "queue": "tx", "outdev": "mirror0" } }
    {"execute": "object-add", "arguments":{ "qom-type": "filter-redirector", "id": "redire0", "netdev": "hn0", "queue": "rx", "indev": "compare_out" } }
    {"execute": "object-add", "arguments":{ "qom-type": "filter-redirector", "id": "redire1", "netdev": "hn0", "queue": "rx", "outdev": "compare0" } }
    {"execute": "object-add", "arguments":{ "qom-type": "iothread", "id": "iothread1" } }
    {"execute": "object-add", "arguments":{ "qom-type": "colo-compare", "id": "comp0", "primary_in": "compare0-0", "secondary_in": "compare1", "outdev": "compare_out0", "iothread": "iothread1" } }

    {"execute": "migrate-set-capabilities", "arguments":{ "capabilities": [ {"capability": "x-colo", "state": true } ] } }
    {"execute": "migrate", "arguments":{ "uri": "tcp:127.0.0.2:9998" } }


注意：
如果此主端之前是备端，需要使用 ""insert": "before", "position": "id=rew0"" 选项，将过滤器插入到 filter-rewriter 之前。参见下文。

备端恢复复制
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

成为主端，并在新备端启动后恢复复制。注意，此时 127.0.0.1 是备端，127.0.0.2 是主端。

启动新备端（上述步骤 2 和 3，但 primary_ip=127.0.0.2），然后在原备端执行::

    {"execute": "drive-mirror", "arguments":{ "device": "colo-disk0", "job-id": "resync", "target": "nbd://127.0.0.1:9999/parent0", "mode": "existing", "format": "raw", "sync": "full"} }


等待磁盘同步完成，然后执行::

    {"execute": "stop"}
    {"execute": "block-job-cancel", "arguments":{ "device": "resync" } }

    {"execute": "blockdev-add", "arguments": {"driver": "nbd", "node-name": "nbd0", "server": {"type": "inet", "host": "127.0.0.1", "port": "9999"}, "export": "parent0", "detect-zeroes": "on"} }
    {"execute": "x-blockdev-change", "arguments":{ "parent": "colo-disk0", "node": "nbd0" } }

    {"execute": "object-add", "arguments":{ "qom-type": "filter-mirror", "id": "m0", "insert": "before", "position": "id=rew0", "netdev": "hn0", "queue": "tx", "outdev": "mirror0" } }
    {"execute": "object-add", "arguments":{ "qom-type": "filter-redirector", "id": "redire0", "insert": "before", "position": "id=rew0", "netdev": "hn0", "queue": "rx", "indev": "compare_out" } }
    {"execute": "object-add", "arguments":{ "qom-type": "filter-redirector", "id": "redire1", "insert": "before", "position": "id=rew0", "netdev": "hn0", "queue": "rx", "outdev": "compare0" } }
    {"execute": "object-add", "arguments":{ "qom-type": "iothread", "id": "iothread1" } }
    {"execute": "object-add", "arguments":{ "qom-type": "colo-compare", "id": "comp0", "primary_in": "compare0-0", "secondary_in": "compare1", "outdev": "compare_out0", "iothread": "iothread1" } }

    {"execute": "migrate-set-capabilities", "arguments":{ "capabilities": [ {"capability": "x-colo", "state": true } ] } }
    {"execute": "migrate", "arguments":{ "uri": "tcp:127.0.0.1:9998" } }


| Copyright (c) 2016 Intel Corporation
| Copyright (c) 2016 HUAWEI TECHNOLOGIES CO., LTD.
| Copyright (c) 2016 Fujitsu, Corp.
| Copyright (c) 2026 Lukas Straub <lukasstraub2@web.de>
