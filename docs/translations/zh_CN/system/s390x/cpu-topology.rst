.. _cpu-topology-s390x:

s390x 上的 CPU 拓扑
=====================

从 QEMU 8.2 开始，s390x CPU 拓扑最多提供 3 层拓扑容器：drawer、book 和 socket。它们组成树状层次结构。

socket 容器包含一个或多个 CPU 条目。每个 CPU 条目由一个位图和三个 CPU 属性组成：

- CPU 类型
- entitlement（资源配额等级）
- dedication（专用性）

位图中每个置位的位对应一个具有相同属性的 vCPU 的 core-id。

本文介绍 S390 CPU 拓扑的一般信息、启用方式，并解释新的 CPU 属性。关于如何修改 S390 CPU 拓扑以及监控极化变化，请参阅 ``docs/devel/s390-cpu-topology.rst``。

前提条件
-------------

目前，要使用 CPU 拓扑，需要选择 KVM 加速器。有关加速器以及选择方式的更多详情，请参阅 :ref:`Accelerators`。

s390x 主机需要使用 Linux v6.0 或更新内核，其中提供 ``KVM_CAP_S390_CPU_TOPOLOGY`` 能力，允许 QEMU 通过 STFLE 位 11 向虚拟机通告 CPU 拓扑设施。

启用 CPU 拓扑
---------------------

目前，只有 "host" CPU 模型默认启用 CPU 拓扑。

要在其他 CPU 模型中启用 CPU 拓扑，请将 CPU 标志 ``ctop`` 设为 ``on``，例如：

.. code-block:: bash

   -cpu gen16b,ctop=on


默认禁用拓扑，可以在不添加新标志的情况下支持新旧 QEMU 之间的迁移。

默认拓扑用法
----------------------

可以通过 QEMU 命令行中的 ``-smp`` 或 ``-device`` 参数指定 CPU 拓扑。

另请注意，从 7.2 开始，拓扑不再支持线程，``-smp`` 命令行参数只接受 ``threads=1``。

如果没有为 ``-smp`` 指定容器属性（drawers、books、sockets），则这些容器的数量均为 1。

因此，以下两个选项将产生相同的拓扑：

.. code-block:: bash

    -smp cpus=5,drawer=1,books=1,sockets=8,cores=4,maxcpus=32


以及

.. code-block:: bash

    -smp cpus=5,sockets=8,cores=4,maxcpus=32


通过 ``-smp`` 参数定义 CPU 时，会根据 core-id 将 CPU 加入拓扑，以计算其位置：从 drawer-0、book-0、socket-0 的位置 0 上的 core-0 开始，填满 socket-0 的所有 CPU 后，再填充 book-0 的 socket-1，依此类推，直到最后一个 drawer 的最后一个 book 的最后一个 socket。

通过 ``-device`` 参数定义 CPU 时，树状拓扑属性必须全部定义或全部不定义。

.. code-block:: bash

    -device gen16b-s390x-cpu,drawer-id=1,book-id=1,socket-id=2,core-id=1


或者

.. code-block:: bash

    -device gen16b-s390x-cpu,core-id=1,dedicated=true


如果 ``-device`` 参数未指定任何树属性（drawer、book、sockets），则与通过 ``-smp`` 参数定义的所有 CPU 一样，只需根据 core-id 将 CPU 加入拓扑，即可设置拓扑树属性。

QEMU 不会尝试解决冲突。如果 ``-device`` 参数显式或隐式定义的 CPU 拓扑与 ``-smp`` 参数隐式定义的 CPU 冲突，将报告错误。

如果 ``-device`` 参数没有定义拓扑修饰属性，则采用以下默认值：

- dedicated：``false``
- entitlement：``medium``


热插拔
++++++++

可以使用 HMP 的 device_add 命令插入新 CPU，例如：

.. code-block:: bash

  (qemu) device_add gen16b-s390x-cpu,core-id=9


CPU 的位置按上文所述从 core-id 推导。

当然，也可以完整定义拓扑：

.. code-block:: bash

    (qemu) device_add gen16b-s390x-cpu,drawer-id=1,book-id=1,socket-id=2,core-id=1



示例
++++++++

在以下机器中，我们定义 8 个 socket，每个包含 4 个核心。

.. code-block:: bash

  $ qemu-system-s390x -accel kvm -m 2G \
    -cpu gen16b,ctop=on \
    -smp cpus=5,sockets=8,cores=4,maxcpus=32 \
    -device host-s390x-cpu,core-id=14 \


可以像之前一样使用 HMP 的 device_add 命令插入新 CPU：

.. code-block:: bash

  (qemu) device_add gen16b-s390x-cpu,core-id=9


core-id 定义核心在拓扑中的位置，从 socket 0 中的 core 0 开始，直到 maxcpus。

在上例中：

* ``-smp`` 命令行向客户机提供 5 个 CPU，其 core-id 为 0、1、2、3、4。由于每个 socket 有 4 个核心，socket 0 中向客户机提供 4 个 CPU，core-id 为 0、1、2、3。最后一个 CPU 的 core-id 为 4，位于 socket 1。

* ``-device`` 命令行提供的 ID 为 14 的核心位于 socket 3，core-id 为 14。

* QMP 的 ``device_add`` 命令提供的 ID 为 9 的核心位于 socket 2，core-id 为 9。


极化、资源配额和专用性
----------------------------------------

极化
++++++++++++

极化影响共享主机的 CPU 如何在客户机之间使用和分配。客户机通过 PTF 指令确定极化方式。

极化定义了两种 CPU 供给模型：水平和垂直。

水平极化是启动时以及子系统复位后的默认模型。采用水平极化时，所有 vCPU 获得的资源供给应大致相等。

垂直极化模型中的 vCPU 并不平等，但总体上可能有更多资源可用。客户机可以使用主机提供的 vCPU 配额信息优化内核线程调度。

子系统复位会使配置中的所有 vCPU 恢复水平极化。

资源配额
+++++++++++

垂直极化规定，客户机的 vCPU 可以获得不同的真实 CPU 资源供给：

- 垂直高配额的 vCPU 获得真实 CPU 资源供给的 100%。

- 垂直中配额的 vCPU 与其他 vCPU 共享真实 CPU。

- 垂直低配额的 vCPU 仅在其他 vCPU 都不需要时，才能获得真实 CPU 资源供给。

如果垂直高配额的 vCPU 没有使用真实 CPU，则未使用的“空余”资源可以调度给中配额或低配额的其他 vCPU。

vCPU 可以设置为“专用”，此时该 vCPU 完全专属于一个真实 CPU。

dedicated 位表示 vCPU 对真实 CPU 的亲和性，而 entitlement 表示使用上的共享性或独占性。

在命令行中定义拓扑
-----------------------------------------

可以完全使用 -device cpu 语句定义拓扑，唯一例外是 CPU 0，必须通过 -smp 参数定义。

例如，下例将核心 1、2、3 放在 drawer 1、book 1、socket 2，将核心 0、9、14 放在 drawer 0、book 0、socket 0，且不定义 entitlement 或 dedication。核心 4 放在其默认位置 socket 1（因为每个 socket 有 4 个核心），并定义为专用且具有垂直高配额。

.. code-block:: bash

  $ qemu-system-s390x -accel kvm -m 2G \
    -cpu gen16b,ctop=on \
    -smp cpus=1,sockets=8,cores=4,maxcpus=32 \
    \
    -device gen16b-s390x-cpu,drawer-id=1,book-id=1,socket-id=2,core-id=1 \
    -device gen16b-s390x-cpu,drawer-id=1,book-id=1,socket-id=2,core-id=2 \
    -device gen16b-s390x-cpu,drawer-id=1,book-id=1,socket-id=2,core-id=3 \
    \
    -device gen16b-s390x-cpu,drawer-id=0,book-id=0,socket-id=0,core-id=9 \
    -device gen16b-s390x-cpu,drawer-id=0,book-id=0,socket-id=0,core-id=14 \
    \
    -device gen16b-s390x-cpu,core-id=4,dedicated=on,entitlement=high


为 CPU 4 定义的配额，只有在客户机通过 PTF 指令成功启用垂直极化后才会使用。
