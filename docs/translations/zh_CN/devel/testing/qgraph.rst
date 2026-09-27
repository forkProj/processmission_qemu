.. _qgraph:

QTest 驱动框架
==============

普通 libqos 测试必须自行选择正确的机器和设备启动 QEMU。
因此，每个测试往往固定于某种配置，限制了覆盖范围。

例如，x86_64 和 Arm 开发板都支持 sdhci，通用测试应覆盖所有支持它的机器和驱动。
仅使用 libqos 时，需要手动处理每种组合并构造命令行。

这也会带来兼容性问题：设备或驱动的命令行名称一变，所有相关测试都需要调整。

qgraph 将驱动、机器和测试组织为图。
测试只需声明它要测试的设备类型，即接口，
框架负责选择机器和设备、构造命令行，覆盖支持的设备类型和机器架构。

上例接口为 sdhci，测试只需将自身节点连接到该接口。
如果某个驱动的命令行变化，只需修改对应驱动节点。

QGraph 概念
-----------

节点表示机器、驱动、测试等对象，边表示 CONSUMES、PRODUCES、CONTAINS 关系。

节点
~~~~~~~~

有四种节点：

- **QNODE_MACHINE**：例如 arm/raspi2b。
- **QNODE_DRIVER**：例如 generic-sdhci。
- **QNODE_INTERFACE**：例如 sdhci，抽象全部 sdhci 驱动。
  不需要显式创建，某节点消费或提供它时自动实例化。
  接口是一个结构体，为同类设备的不同驱动提供统一抽象，
  向消费接口的节点提供 API，由提供接口的驱动实现。
- **QNODE_TEST**：例如 sdhci-test，消费接口并测试其功能。

注意：

- QNODE_MACHINE：机器结构体必须包含 QGuestAllocator，
  并实现 get_driver()，通过 memory 接口返回分配器；未设置时也可返回 NULL。
- QNODE_DRIVER：名称必须唯一。
  机器及将被其他节点消费的节点名称必须匹配 QEMU 驱动名称，否则无法发现。

边
~~~~~~~~

节点 X 与 Y 之间可以有以下关系：

- ``X CONSUMES Y``：Y 可以接入 X。
- ``X PRODUCES Y``：X 提供接口 Y。
- ``X CONTAINS Y``：Y 是 X 的组成部分。

执行步骤
~~~~~~~~

框架基本流程如下：

- 各机器、驱动和测试文件创建自己的节点及边。
- 启动 QEMU，查询可用机器和设备；只有机器和被消费的节点与 QEMU 设备一一对应。
- 从可用机器出发，深度优先搜索测试。
- 找到测试后，重新遍历路径，分配各驱动，并将最终接口传给测试。
- 执行测试。
- 清理不再需要的对象，继续发现其他路径。

不同 QEMU 程序只提供部分驱动和机器，只有从它们可达的测试才会运行。

命令行
~~~~~~~~

命令行由节点名称，以及创建边时用户提供的可选参数组成。

参数分三类：

- ``in node``：根据节点名自动生成，例如机器使用 ``-M <machine>``，
  设备使用 ``-device <device>``。
- ``after node``：创建边时，通过 QOSGraphEdgeOptions 的
  after_cmd_line 和 extra_edge_opts 添加。
  extra_edge_opts 是目标节点的属性，框架自动在其前加逗号；
  after_cmd_line 用于增加设备而非属性，因此前面自动加空格。
- ``before node``：通过 QOSGraphEdgeOptions 的 before_cmd_line 添加，
  放在边所指目标节点之前，适合 -fdsev、-netdev 等无法用节点表示的命令。

边上的参数总会加入，但并非所有节点名称都参与构造命令行。
被包含或被提供的节点已由 QEMU 创建，因此只使用与消费关系相关的节点。
QMP 属性为 ``{ "abstract" : true }`` 的节点也不会生成命令行，
因为它们不是可直接添加的实际设备。

示例：::

    QOSGraphEdgeOptions opts = {
        .before_cmd_line = "-drive id=drv0,if=none,file=null-co://,"
                           "file.read-zeroes=on,format=raw",
        .after_cmd_line = "-device scsi-hd,bus=vs0.0,drive=drv0",

        opts.extra_device_opts = "id=vs0";
    };

    qos_node_create_driver("virtio-scsi-device",
                            virtio_scsi_device_create);
    qos_node_consumes("virtio-scsi-device", "virtio-bus", &opts);


生成命令行：
``-drive id=drv0,if=none,file=null-co://, -device virtio-scsi-device,id=vs0 -device scsi-hd,bus=vs0.0,drive=drv0``

排查不可用测试
~~~~~~~~~~~~~~

如果从可用机器没有路径到达某测试，该测试就不可用，不会执行。
原因可能是节点配置错误，也可能是所需机器或设备没有编入 QEMU，或因其他原因不可用。

可以运行以下命令排查：::

  $ QTEST_QEMU_BINARY=build/qemu-system-x86_64 build/tests/qtest/qos-test --verbose
  # ALL QGRAPH EDGES: {
  #   src='virtio-net'
  #      |-> dest='virtio-net-tests/vhost-user/multiqueue' type=2 (node=0x559142109e30)
  #      |-> dest='virtio-net-tests/vhost-user/migrate' type=2 (node=0x559142109d00)
  #   src='virtio-net-pci'
  #      |-> dest='virtio-net' type=1 (node=0x55914210d740)
  #   src='pci-bus'
  #      |-> dest='virtio-net-pci' type=2 (node=0x55914210d880)
  #   src='pci-bus-pc'
  #      |-> dest='pci-bus' type=1 (node=0x559142103f40)
  #   src='i440FX-pcihost'
  #      |-> dest='pci-bus-pc' type=0 (node=0x55914210ac70)
  #   src='x86_64/pc'
  #      |-> dest='i440FX-pcihost' type=0 (node=0x5591421117f0)
  #   src=''
  #      |-> dest='x86_64/pc' type=0 (node=0x559142111600)
  #      |-> dest='arm/raspi2b' type=0 (node=0x559142110740)
  ...
  # }
  # ALL QGRAPH NODES: {
  #   name='virtio-net-tests/announce-self' type=3 cmd_line='(null)' [available]
  #   name='arm/raspi2b' type=0 cmd_line='-M raspi2b ' [UNAVAILABLE]
  ...
  # }


输出中，virtio-net-tests/announce-self 在 ALL QGRAPH NODES 中标为 available，
表示会运行。其 ALL QGRAPH EDGES 路径为
``'' -> 'x86_64/pc' -> 'i440FX-pcihost' -> 'pci-bus-pc' -> 'pci-bus' -> 'virtio-net-pci' -> 'virtio-net'``。
根节点为 ''，深度优先搜索从这里开始。

arm/raspi2b 虽然通过 ``'' -> 'arm/raspi2b'`` 可达，却标为 UNAVAILABLE，
因为查询时 QEMU 没有报告该机器。
本例使用 qemu-system-x86_64，不支持 Arm 机器，因此符合预期。

如果测试意外不可用，先检查 ALL QGRAPH EDGES 中是否存在从根到测试的连接。
不存在则说明驱动或测试代码没有正确建立图；
若存在，再逐个检查路径节点在 ALL QGRAPH NODES 中的可用性。
第一个不可用节点就是原因，通常是程序缺少相应机器或设备支持。

创建驱动及其接口
----------------

继续以 sdhci 为例：

- sdhci-test 测试 sdhci 驱动提供的 read[q,w]、writeq。
- x86_64/pc 和 Arm 都支持 sdhci，本例 Arm 使用 arm-raspi2b。
- QEMU 提供 Arm 的 QSDHCI_MemoryMapped 和 x86_64/pc 的 QSDHCI_PCI，
  两者都实现上述操作。

测试开发者需要：

- 创建 x86_64/pc 机器节点。它使用 PCI 总线，包含 pci-bus-pc 驱动，实际路径为：

  ``x86_64/pc --contains--> 1440FX-pcihost --contains--> pci-bus-pc --produces--> pci-bus``。

  本例不讨论 PCI 接口的具体实现。

- 创建代表 QSDHCI_PCI 的 sdhci-pci 驱动节点。
  它使用 PCI 总线及其 API，因此消费抽象全部 PCI 驱动的通用 pci-bus 接口：

  ``sdhci-pci --consumes--> pci-bus``

- 创建 arm/raspi2b 机器节点，包含代表 QSDHCI_MemoryMapped 的
  generic-sdhci 内存映射驱动：

  ``arm/raspi2b --contains--> generic-sdhci``

- 创建 sdhci 接口节点，提供全部 sdhci 设备共有的函数。
  接口由平台专用驱动 sdhci-pci 和 generic-sdhci 提供：

  ``sdhci-pci --produces--> sdhci``

  ``generic-sdhci --produces--> sdhci``

- 创建 sdhci-test，消费接口并使用其 API，无需了解具体机器或驱动：

  ``sdhci-test --consumes--> sdhci``

arm-raspi2b 机器的简化实现，来自 ``tests/qtest/libqos/arm-raspi2-machine.c``：::

    #include "qgraph.h"

    struct QRaspi2Machine {
        QOSGraphObject obj;
        QGuestAllocator alloc;
        QSDHCI_MemoryMapped sdhci;
    };

    static void *raspi2_get_driver(void *object, const char *interface)
    {
        QRaspi2Machine *machine = object;
        if (!g_strcmp0(interface, "memory")) {
            return &machine->alloc;
        }

        fprintf(stderr, "%s not present in arm/raspi2b\n", interface);
        g_assert_not_reached();
    }

    static QOSGraphObject *raspi2_get_device(void *obj,
                                                const char *device)
    {
        QRaspi2Machine *machine = obj;
        if (!g_strcmp0(device, "generic-sdhci")) {
            return &machine->sdhci.obj;
        }

        fprintf(stderr, "%s not present in arm/raspi2b\n", device);
        g_assert_not_reached();
    }

    static void *qos_create_machine_arm_raspi2(QTestState *qts)
    {
        QRaspi2Machine *machine = g_new0(QRaspi2Machine, 1);

        alloc_init(&machine->alloc, ...);

        /* Get node(s) contained inside (CONTAINS) */
        machine->obj.get_device = raspi2_get_device;

        /* Get node(s) produced (PRODUCES) */
        machine->obj.get_driver = raspi2_get_driver;

        /* free the object */
        machine->obj.destructor = raspi2_destructor;
        qos_init_sdhci_mm(&machine->sdhci, ...);
        return &machine->obj;
    }

    static void raspi2_register_nodes(void)
    {
        /* arm/raspi2b --contains--> generic-sdhci */
        qos_node_create_machine("arm/raspi2b",
                                 qos_create_machine_arm_raspi2);
        qos_node_contains("arm/raspi2b", "generic-sdhci", NULL);
    }

    libqos_init(raspi2_register_nodes);


x86_64/pc 的简化实现，来自 ``tests/qtest/libqos/x86_64_pc-machine.c``：::

    #include "qgraph.h"

    struct i440FX_pcihost {
        QOSGraphObject obj;
        QPCIBusPC pci;
    };

    struct QX86PCMachine {
        QOSGraphObject obj;
        QGuestAllocator alloc;
        i440FX_pcihost bridge;
    };

    /* i440FX_pcihost */

    static QOSGraphObject *i440FX_host_get_device(void *obj,
                                                const char *device)
    {
        i440FX_pcihost *host = obj;
        if (!g_strcmp0(device, "pci-bus-pc")) {
            return &host->pci.obj;
        }
        fprintf(stderr, "%s not present in i440FX-pcihost\n", device);
        g_assert_not_reached();
    }

    /* x86_64/pc machine */

    static void *pc_get_driver(void *object, const char *interface)
    {
        QX86PCMachine *machine = object;
        if (!g_strcmp0(interface, "memory")) {
            return &machine->alloc;
        }

        fprintf(stderr, "%s not present in x86_64/pc\n", interface);
        g_assert_not_reached();
    }

    static QOSGraphObject *pc_get_device(void *obj, const char *device)
    {
        QX86PCMachine *machine = obj;
        if (!g_strcmp0(device, "i440FX-pcihost")) {
            return &machine->bridge.obj;
        }

        fprintf(stderr, "%s not present in x86_64/pc\n", device);
        g_assert_not_reached();
    }

    static void *qos_create_machine_pc(QTestState *qts)
    {
        QX86PCMachine *machine = g_new0(QX86PCMachine, 1);

        /* Get node(s) contained inside (CONTAINS) */
        machine->obj.get_device = pc_get_device;

        /* Get node(s) produced (PRODUCES) */
        machine->obj.get_driver = pc_get_driver;

        /* free the object */
        machine->obj.destructor = pc_destructor;
        pc_alloc_init(&machine->alloc, qts, ALLOC_NO_FLAGS);

        /* Get node(s) contained inside (CONTAINS) */
        machine->bridge.obj.get_device = i440FX_host_get_device;

        return &machine->obj;
    }

    static void pc_machine_register_nodes(void)
    {
        /* x86_64/pc --contains--> 1440FX-pcihost --contains-->
         * pci-bus-pc [--produces--> pci-bus (in pci.h)] */
        qos_node_create_machine("x86_64/pc", qos_create_machine_pc);
        qos_node_contains("x86_64/pc", "i440FX-pcihost", NULL);

        /* contained drivers don't need a constructor,
         * they will be init by the parent */
        qos_node_create_driver("i440FX-pcihost", NULL);
        qos_node_contains("i440FX-pcihost", "pci-bus-pc", NULL);
    }

    libqos_init(pc_machine_register_nodes);


sdhci，来自 ``tests/qtest/libqos/sdhci.c``：::

    /* Interface node, offers the sdhci API */
    struct QSDHCI {
        uint16_t (*readw)(QSDHCI *s, uint32_t reg);
        uint64_t (*readq)(QSDHCI *s, uint32_t reg);
        void (*writeq)(QSDHCI *s, uint32_t reg, uint64_t val);
        /* other fields */
    };

    /* Memory Mapped implementation of QSDHCI */
    struct QSDHCI_MemoryMapped {
        QOSGraphObject obj;
        QSDHCI sdhci;
        /* other driver-specific fields */
    };

    /* PCI implementation of QSDHCI */
    struct QSDHCI_PCI {
        QOSGraphObject obj;
        QSDHCI sdhci;
        /* other driver-specific fields */
    };

    /* Memory mapped implementation of QSDHCI */

    static void *sdhci_mm_get_driver(void *obj, const char *interface)
    {
        QSDHCI_MemoryMapped *smm = obj;
        if (!g_strcmp0(interface, "sdhci")) {
            return &smm->sdhci;
        }
        fprintf(stderr, "%s not present in generic-sdhci\n", interface);
        g_assert_not_reached();
    }

    void qos_init_sdhci_mm(QSDHCI_MemoryMapped *sdhci, QTestState *qts,
                        uint32_t addr, QSDHCIProperties *common)
    {
        /* Get node contained inside (CONTAINS) */
        sdhci->obj.get_driver = sdhci_mm_get_driver;

        /* SDHCI interface API */
        sdhci->sdhci.readw = sdhci_mm_readw;
        sdhci->sdhci.readq = sdhci_mm_readq;
        sdhci->sdhci.writeq = sdhci_mm_writeq;
        sdhci->qts = qts;
    }

    /* PCI implementation of QSDHCI */

    static void *sdhci_pci_get_driver(void *object,
                                      const char *interface)
    {
        QSDHCI_PCI *spci = object;
        if (!g_strcmp0(interface, "sdhci")) {
            return &spci->sdhci;
        }

        fprintf(stderr, "%s not present in sdhci-pci\n", interface);
        g_assert_not_reached();
    }

    static void *sdhci_pci_create(void *pci_bus,
                                  QGuestAllocator *alloc,
                                  void *addr)
    {
        QSDHCI_PCI *spci = g_new0(QSDHCI_PCI, 1);
        QPCIBus *bus = pci_bus;
        uint64_t barsize;

        qpci_device_init(&spci->dev, bus, addr);

        /* SDHCI interface API */
        spci->sdhci.readw = sdhci_pci_readw;
        spci->sdhci.readq = sdhci_pci_readq;
        spci->sdhci.writeq = sdhci_pci_writeq;

        /* Get node(s) produced (PRODUCES) */
        spci->obj.get_driver = sdhci_pci_get_driver;

        spci->obj.start_hw = sdhci_pci_start_hw;
        spci->obj.destructor = sdhci_destructor;
        return &spci->obj;
    }

    static void qsdhci_register_nodes(void)
    {
        QOSGraphEdgeOptions opts = {
            .extra_device_opts = "addr=04.0",
        };

        /* generic-sdhci */
        /* generic-sdhci --produces--> sdhci */
        qos_node_create_driver("generic-sdhci", NULL);
        qos_node_produces("generic-sdhci", "sdhci");

        /* sdhci-pci */
        /* sdhci-pci --produces--> sdhci
         * sdhci-pci --consumes--> pci-bus */
        qos_node_create_driver("sdhci-pci", sdhci_pci_create);
        qos_node_produces("sdhci-pci", "sdhci");
        qos_node_consumes("sdhci-pci", "pci-bus", &opts);
    }

    libqos_init(qsdhci_register_nodes);


上述示例创建了全部类型的关系：::

  x86_64/pc --contains--> 1440FX-pcihost --contains--> pci-bus-pc
                                                            |
               sdhci-pci --consumes--> pci-bus <--produces--+
                  |
                  +--produces--+
                               |
                               v
                             sdhci
                               ^
                               |
                               +--produces-- +
                                             |
               arm/raspi2b --contains--> generic-sdhci


将 consumes 反向表示为 consumed_by 后：::

  x86_64/pc --contains--> 1440FX-pcihost --contains--> pci-bus-pc
                                                            |
            sdhci-pci <--consumed by-- pci-bus <--produces--+
                |
                +--produces--+
                             |
                             v
                            sdhci
                             ^
                             |
                             +--produces-- +
                                           |
            arm/raspi2b --contains--> generic-sdhci


添加测试
--------

有了上述结构，添加测试很简单。
sdhci-test 来自 ``tests/qtest/sdhci-test.c``：::

    static void check_capab_sdma(QSDHCI *s, bool supported)
    {
        uint64_t capab, capab_sdma;

        capab = s->readq(s, SDHC_CAPAB);
        capab_sdma = FIELD_EX64(capab, SDHC_CAPAB, SDMA);
        g_assert_cmpuint(capab_sdma, ==, supported);
    }

    static void test_registers(void *obj, void *data,
                                QGuestAllocator *alloc)
    {
        QSDHCI *s = obj;

        /* example test */
        check_capab_sdma(s, s->props.capab.sdma);
    }

    static void register_sdhci_test(void)
    {
        /* sdhci-test --consumes--> sdhci */
        qos_add_test("registers", "sdhci", test_registers, NULL);
    }

    libqos_init(register_sdhci_test);


它消费 sdhci 接口，使两种机器都存在通向测试的有效路径。最终图如下：::

  x86_64/pc --contains--> 1440FX-pcihost --contains--> pci-bus-pc
                                                            |
               sdhci-pci --consumes--> pci-bus <--produces--+
                  |
                  +--produces--+
                               |
                               v
                             sdhci <--consumes-- sdhci-test
                               ^
                               |
                               +--produces-- +
                                             |
               arm/raspi2b --contains--> generic-sdhci


将 consumes 反向表示为 consumed_by 后：::

  x86_64/pc --contains--> 1440FX-pcihost --contains--> pci-bus-pc
                                                            |
            sdhci-pci <--consumed by-- pci-bus <--produces--+
                |
                +--produces--+
                             |
                             v
                            sdhci --consumed by--> sdhci-test
                             ^
                             |
                             +--produces-- +
                                           |
            arm/raspi2b --contains--> generic-sdhci


使用 ``QTEST_QEMU_BINARY=./qemu-system-x86_64`` 时，有效路径为：
``/x86_64/pc/1440FX-pcihost/pci-bus-pc/pci-bus/sdhci-pc/sdhci/sdhci-test``。

使用 ``QTEST_QEMU_BINARY=./qemu-system-arm`` 时为：

``/arm/raspi2b/generic-sdhci/sdhci/sdhci-test``

更多示例见 test-qgraph.c。

Qgraph API 参考
---------------

.. kernel-doc:: tests/qtest/libqos/qgraph.h
