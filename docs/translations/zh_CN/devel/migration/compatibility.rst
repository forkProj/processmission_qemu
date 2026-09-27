向后兼容性
==========

向后兼容性如何工作
------------------

执行迁移时，会有两个 QEMU 进程：源端和目标端。它们可能使用相同版本，也可能使用不同版本。版本相同时比较简单，版本不同时则比较困难。

有两个不同的概念，名称却很相似，有时容易混淆：

- QEMU 版本
- 机器类型版本

先看一个实际例子，使用以下版本：

- qemu-system-x86_64（v5.2），下文称为 qemu-5.2。
- qemu-system-x86_64（v5.1），下文称为 qemu-5.1。

相关的是各版本定义的“最新”机器类型：

- pc-q35-5.2（qemu-5.2 中的最新类型），下文称为 pc-5.2。
- pc-q35-5.1（qemu-5.1 中的最新类型），下文称为 pc-5.1。

首先，只有源端和目标端采用相同机器类型，迁移才应能够正常进行。两端的 QEMU 硬件配置也需要一致。后端配置的大多数方面可以自由改变，少数例外是后端特性会影响前端设备暴露的特性，不过这与本节无关。

下面列出可能的组合。先从最简单的情况开始，即源端和目标端 QEMU 相同：

1 - qemu-5.2 -M pc-5.2  -> 迁移到 -> qemu-5.2 -M pc-5.2

  这是最新 QEMU 配合最新机器类型，必须能够正常工作，否则就是缺陷。

2 - qemu-5.1 -M pc-5.1  -> 迁移到 -> qemu-5.1 -M pc-5.1

  与上一个情况完全相同，只是版本为 5.1，同样没有特别之处。

这些情况最简单，本节不再进一步讨论。

现在看更有意思的情况：两端使用相同 QEMU 版本（qemu-5.2），但采用的机器类型不是该版本最新的 pc-5.2，而是旧 QEMU 版本的机器类型，这里为 pc-5.1。

3 - qemu-5.2 -M pc-5.1  -> 迁移到 -> qemu-5.2 -M pc-5.1

  需要采用 pc-5.1 的定义，以及设备在 5.1 中的配置。不过，这应该比较容易，因为两端的 QEMU 相同，对 pc-5.1 机器的理解也完全一致。

4 - qemu-5.1 -M pc-5.2  -> 迁移到 -> qemu-5.1 -M pc-5.2

  此组合不可能存在，因为 qemu-5.1 不认识 pc-5.2 机器类型，因此无需担心。

接下来才是有意思的情况：两个 QEMU 进程版本不同。请注意，机器类型必须为 pc-5.1，因为 qemu-5.1 不认识 pc-5.2。因此可能的情况如下：

5 - qemu-5.2 -M pc-5.1  -> 迁移到 -> qemu-5.1 -M pc-5.1

  这种迁移称为从新版到旧版。开发 5.2 时，必须注意不能破坏向 qemu-5.1 的迁移。无法更新 qemu-5.1，使其理解 qemu-5.2 决定采用的所有修改，因此相关修改必须在 qemu-5.2 一侧完成。

6 - qemu-5.1 -M pc-5.1  -> 迁移到 -> qemu-5.2 -M pc-5.1

  这种迁移称为从旧版到新版。必须确保能够接收来自 qemu-5.1 的迁移，问题与上一种情况类似。

如果 qemu-5.1 与 qemu-5.2 完全相同，就不会存在兼容性问题。但创建 qemu-5.2 的目的，正是获得新功能、新设备、新默认值等。

如果设备新增功能，或某个默认值发生变化，在不同 QEMU 版本之间迁移时就会遇到问题。

因此，需要一种方式告诉 qemu-5.2：使用 pc-5.1 机器类型时，**不要** 使用新功能，这样才能迁移到真正的 qemu-5.1。

从 qemu-5.1 迁移到 qemu-5.2 时也有对应要求：qemu-5.2 必须预期不会收到新功能的数据，因为 qemu-5.1 不知道该功能。

如何让 QEMU 知道这些设备功能变化？通过 hw/core/machine.c 中的 hw_compat_X_Y 数组。

如果修改默认值，就需要在该数组中恢复旧值。设备初始化时需要检查数组，确定该功能应采用的值。数组中记录的就是属性值。

为设备创建属性，需要使用 ``DEFINE_PROP_*()`` 宏之一。可在 include/hw/core/qdev-properties.h 中查看现有宏。通过这些宏设置属性默认值，最新发布版本就会采用该值。如果旧版需要不同值，可以在 hw_compat_X_Y 数组中调整。

hw_compat_X_Y 是记录数组，每条记录包含：

- name_device
- name_property
- value

看一个实际例子。

qemu-5.2 中的 virtio-blk-device 增加了多队列支持。这是不向后兼容的修改。在 qemu-5.1 中只有一个队列；在 qemu-5.2 中，队列数量等于系统 CPU 数量。

迁移时，如果从有 4 个队列的设备迁移到只有一个队列的设备，就不知道其余 3 个队列的信息该放在哪里，导致迁移失败。

从只有一个队列的 qemu-5.1 迁移到 qemu-5.2 时也存在类似问题：只发送了一个队列的信息，但目标端有 4 个队列，于是有 3 个队列未正确初始化，结果无法预料。

解决方法很简单：让 qemu-5.2 在运行 pc-5.1 时，将 virtio-blk-devices 的队列数量设为 1。

这样就能解决情况 5 和 6。

5 - qemu-5.2 -M pc-5.1  -> 迁移到 -> qemu-5.1 -M pc-5.1

    qemu-5.2 -M pc-5.1 将队列数量设为 1。
    qemu-5.1 -M pc-5.1 预期队列数量为 1。

    正确，迁移可以工作。

6 - qemu-5.1 -M pc-5.1  -> 迁移到 -> qemu-5.2 -M pc-5.1

    qemu-5.1 -M pc-5.1 将队列数量设为 1。
    qemu-5.2 -M pc-5.1 预期队列数量为 1。

    正确，迁移可以工作。

再看另一个有意思的情况，即情况 3：

3 - qemu-5.2 -M pc-5.1  -> 迁移到 -> qemu-5.2 -M pc-5.1

    两端的 QEMU 相同。因此，是否将队列数量设为 1 似乎并不重要，因为两端一样。

    错了！

    想想进行以下连续两次迁移会发生什么：

    A -> 迁移 -> B -> 迁移 -> C

    其中：

    A: qemu-5.1 -M pc-5.1
    B: qemu-5.2 -M pc-5.1
    C: qemu-5.2 -M pc-5.1

    A -> B 属于情况 6，因此队列数量必须为 1。

    B -> C 属于情况 3，看起来不必关心队列数量。但实际上必须关心，因为客户机并非在 qemu-5.2 中启动，而是从 qemu-5.1 迁移而来。因此，为保证安全，使用 pc-5.1 时必须始终采用 1 个队列。

实际是如何实现的？以下提交展示了做法::

  commit 9445e1e15e66c19e42bea942ba810db28052cd05
  Author: Stefan Hajnoczi <stefanha@redhat.com>
  Date:   Tue Aug 18 15:33:47 2020 +0100

  virtio-blk-pci: default num_queues to -smp N


与迁移相关的部分如下::

    @@ -1281,7 +1284,8 @@ static const Property virtio_blk_properties[] = {
     #endif
         DEFINE_PROP_BIT("request-merging", VirtIOBlock, conf.request_merging, 0,
                         true),
    -    DEFINE_PROP_UINT16("num-queues", VirtIOBlock, conf.num_queues, 1),
    +    DEFINE_PROP_UINT16("num-queues", VirtIOBlock, conf.num_queues,
    +                       VIRTIO_BLK_AUTO_NUM_QUEUES),
         DEFINE_PROP_UINT16("queue-size", VirtIOBlock, conf.queue_size, 256),


它修改了 num_queues 的默认值，同时为旧机器类型修正该值，使其保持正确::

    @@ -31,6 +31,7 @@
     GlobalProperty hw_compat_5_1[] = {
         ...
    +    { "virtio-blk-device", "num-queues", "1"},
         ...
     };


两端设备具有不同功能
--------------------

为简化问题，假设两端使用相同 QEMU 二进制程序，但迁移两端某个设备具有不同功能。这可能是因为设备不同，也可能是两端设备的内核驱动提供不同功能，或其他原因。

如何使迁移正常工作？“理论上”很简单：获取源端设备的功能，再获取目标端设备的功能，求出两端功能的交集，然后据此启动 QEMU。

注意，这并不完全是 QEMU 的事情。最重要的是，应由启动 QEMU 的管理应用负责处理。只要 QEMU 配置正确，迁移就能成功。

不过，实际操作很复杂。几乎所有设备都不太支持仅启用部分功能来启动，但有一个重要例外：CPU。

QEMU x86 CPU 模型文档位于：

https://qemu-project.gitlab.io/qemu/system/qemu-cpu-models.html

其中讨论迁移时，建议选择所有 CPU 均支持的最新 CPU 模型。

假设存在以下情况：

主机 A：

设备 X 具有功能 Y。

主机 B：

设备 X 不具有功能 Y。

如果不加处理就从主机 A 迁移到主机 B，会因为目标端加载功能 Y 时发现不存在对应硬件而失败。

这相当于对 CPU 采用以下做法：

主机 A：

$ qemu-system-x86_64 -cpu host

主机 B：

$ qemu-system-x86_64 -cpu host

两台主机的 CPU 功能不同时，这肯定会失败，尤其是主机 B 的功能少于主机 A 时。如果主机 A 的功能少于主机 B，有时可以工作。上一句的关键词是“有时”。

暂且不考虑 CPU 模型，继续使用 -cpu host 示例。假设主机 A 与 B 的 CPU 差异如下：

功能：      'pcid'  'stibp' 'taa-no'
主机 A：       X       X
主机 B：                       X

若希望在两者之间迁移，应这样配置两端 QEMU CPU：

主机 A：

$ qemu-system-x86_64 -cpu host,pcid=off,stibp=off

主机 B：

$ qemu-system-x86_64 -cpu host,taa-no=off

这样就能在两者之间迁移。管理应用或用户负责确保配置正确。一般而言，QEMU 不知道如何检查这类功能。

注意，不建议迁移时使用 -cpu host。此处仅为简化示例而使用它。

其他设备对单个功能的控制能力更差。如果希望在暴露不同功能的主机之间迁移，设备就需要提供配置所用功能的方式。

本节假定迁移两端使用相同 QEMU 二进制程序。如果两端运行不同 QEMU 版本，还必须考虑所有其他差异，示例会更加复杂。

如何缓解向后兼容性错误
----------------------

开发过程中会不断出现破坏旧机器类型迁移的问题，但一经发现就会修复。难点在于，发布版本之后才发现问题时该怎么办。

以下示例说明处理方式。

qemu-8.0 发布后，我们发现 pc-7.2 机器类型的迁移存在问题。

- $ qemu-7.2 -M pc-7.2  ->  qemu-7.2 -M pc-7.2

  迁移正常。

- $ qemu-8.0 -M pc-7.2  ->  qemu-8.0 -M pc-7.2

  迁移正常。

- $ qemu-8.0 -M pc-7.2  ->  qemu-7.2 -M pc-7.2

  迁移失败。

- $ qemu-7.2 -M pc-7.2  ->  qemu-8.0 -M pc-7.2

  迁移失败。

显然，在 qemu-7.2 与 qemu-8.0 之间迁移 pc-7.2 机器类型时出现了问题。错误消息和 git bisect 指向以下提交。

qemu-8.0 中加入了这个提交::

    commit 010746ae1db7f52700cb2e2c46eb94f299cfa0d2
    Author: Jonathan Cameron <Jonathan.Cameron@huawei.com>
    Date:   Thu Mar 2 13:37:02 2023 +0000

    hw/pci/aer: Implement PCI_ERR_UNCOR_MASK register



该提交与本示例相关的部分如下::

    --- a/hw/pci/pcie_aer.c
    +++ b/hw/pci/pcie_aer.c
    @@ -112,6 +112,10 @@ int pcie_aer_init(PCIDevice *dev,

         pci_set_long(dev->w1cmask + offset + PCI_ERR_UNCOR_STATUS,
                      PCI_ERR_UNC_SUPPORTED);
    +    pci_set_long(dev->config + offset + PCI_ERR_UNCOR_MASK,
    +                 PCI_ERR_UNC_MASK_DEFAULT);
    +    pci_set_long(dev->wmask + offset + PCI_ERR_UNCOR_MASK,
    +                 PCI_ERR_UNC_SUPPORTED);

         pci_set_long(dev->config + offset + PCI_ERR_UNCOR_SEVER,
                     PCI_ERR_UNC_SEVERITY_DEFAULT);


补丁改变了为 AER 配置 PCI 空间的方式。但源端和目标端 PCI 空间配置不同时，QEMU 会失败。

以下提交展示了修复方式::

    commit 5ed3dabe57dd9f4c007404345e5f5bf0e347317f
    Author: Leonardo Bras <leobras@redhat.com>
    Date:   Tue May 2 21:27:02 2023 -0300

    hw/pci: Disable PCI_ERR_UNCOR_MASK register for machine type < 8.0

    [...]


QEMU 修复中相关的部分如下。

首先，为设备创建新属性，使其能配置采用旧行为还是新行为::

    diff --git a/hw/pci/pci.c b/hw/pci/pci.c
    index 8a87ccc8b0..5153ad63d6 100644
    --- a/hw/pci/pci.c
    +++ b/hw/pci/pci.c
    @@ -79,6 +79,8 @@ static const Property pci_props[] = {
         DEFINE_PROP_STRING("failover_pair_id", PCIDevice,
                            failover_pair_id),
         DEFINE_PROP_UINT32("acpi-index",  PCIDevice, acpi_index, 0),
    +    DEFINE_PROP_BIT("x-pcie-err-unc-mask", PCIDevice, cap_present,
    +                    QEMU_PCIE_ERR_UNC_MASK_BITNR, true),
     };


注意，对新机器类型会启用该功能。

接着看实际修复。这取决于出现何种兼容性破坏，但本例很简单::

    diff --git a/hw/pci/pcie_aer.c b/hw/pci/pcie_aer.c
    index 103667c368..374d593ead 100644
    --- a/hw/pci/pcie_aer.c
    +++ b/hw/pci/pcie_aer.c
    @@ -112,10 +112,13 @@ int pcie_aer_init(PCIDevice *dev, uint8_t cap_ver,
    uint16_t offset,

         pci_set_long(dev->w1cmask + offset + PCI_ERR_UNCOR_STATUS,
                      PCI_ERR_UNC_SUPPORTED);
    -    pci_set_long(dev->config + offset + PCI_ERR_UNCOR_MASK,
    -                 PCI_ERR_UNC_MASK_DEFAULT);
    -    pci_set_long(dev->wmask + offset + PCI_ERR_UNCOR_MASK,
    -                 PCI_ERR_UNC_SUPPORTED);
    +
    +    if (dev->cap_present & QEMU_PCIE_ERR_UNC_MASK) {
    +        pci_set_long(dev->config + offset + PCI_ERR_UNCOR_MASK,
    +                     PCI_ERR_UNC_MASK_DEFAULT);
    +        pci_set_long(dev->wmask + offset + PCI_ERR_UNCOR_MASK,
    +                     PCI_ERR_UNC_SUPPORTED);
    +    }

         pci_set_long(dev->config + offset + PCI_ERR_UNCOR_SEVER,
                      PCI_ERR_UNC_SEVERITY_DEFAULT);


也就是说，若属性位启用，就按 qemu-8.0 的方式配置；若未设置该属性位，则按 7.2 的方式配置。

现在只需对旧机器类型禁用该功能::

    diff --git a/hw/core/machine.c b/hw/core/machine.c
    index 47a34841a5..07f763eb2e 100644
    --- a/hw/core/machine.c
    +++ b/hw/core/machine.c
    @@ -48,6 +48,7 @@ GlobalProperty hw_compat_7_2[] = {
         { "e1000e", "migrate-timadj", "off" },
         { "virtio-mem", "x-early-migration", "false" },
         { "migration", "x-preempt-pre-7-2", "true" },
    +    { TYPE_PCI_DEVICE, "x-pcie-err-unc-mask", "off" },
     };
     const size_t hw_compat_7_2_len = G_N_ELEMENTS(hw_compat_7_2);


这样，带有此修复的 qemu-8.0.1 发布后，以下所有组合都能按预期工作。

- $ qemu-7.2 -M pc-7.2  ->  qemu-7.2 -M pc-7.2（正常）
- $ qemu-8.0.1 -M pc-7.2  ->  qemu-8.0.1 -M pc-7.2（正常）
- $ qemu-8.0.1 -M pc-7.2  ->  qemu-7.2 -M pc-7.2（正常）
- $ qemu-7.2 -M pc-7.2  ->  qemu-8.0.1 -M pc-7.2（正常）

一切恢复正常了，对吗？

并非如此，现在的组合矩阵大得多。先看简单情况，同版本之间的迁移始终正常：

- $ qemu-7.2 -M pc-7.2  ->  qemu-7.2 -M pc-7.2
- $ qemu-8.0 -M pc-7.2  ->  qemu-8.0 -M pc-7.2
- $ qemu-8.0.1 -M pc-7.2  ->  qemu-8.0.1 -M pc-7.2

接下来是 QEMU 进程版本不同的情况。第一组会失败，而且无能为力，因为两个版本都已发布，无法再改变。

- $ qemu-7.2 -M pc-7.2  ->  qemu-8.0 -M pc-7.2
- $ qemu-8.0 -M pc-7.2  ->  qemu-7.2 -M pc-7.2

下面两种可以正常工作，qemu-8.0.1 中的修改就是为了修复这一问题：

- $ qemu-7.2 -M pc-7.2  ->  qemu-8.0.1 -M pc-7.2
- $ qemu-8.0.1 -M pc-7.2  ->  qemu-7.2 -M pc-7.2

但现在发现，qemu-8.0 既不能迁移到 qemu-7.2，也不能迁移到 qemu-8.0.1。

- $ qemu-8.0 -M pc-7.2  ->  qemu-8.0.1 -M pc-7.2
- $ qemu-8.0.1 -M pc-7.2  ->  qemu-8.0 -M pc-7.2

因此，如果在 qemu-8.0 中启动 pc-7.2 机器，就只能迁移到 qemu-8.0，无法迁移到其他版本。

还能改进吗？

可以。如果知道将进行以下迁移：

- $ qemu-8.0 -M pc-7.2  ->  qemu-8.0.1 -M pc-7.2

就可以这样启动相应设备::

  --device...,x-pci-e-err-unc-mask=on


现在就能接收来自 8.0 的迁移。此后，只要记得为 pc-7.2 启用该属性，就能继续迁移到新机器类型。注意，必须记住这一点，仅知道源端为 qemu-8.0 还不够。考虑以下示例：

$ qemu-8.0 -M pc-7.2 -> qemu-8.0.1 -M pc-7.2 -> qemu-8.2 -M pc-7.2

第二次迁移时，源端已不是 qemu-8.0，但仍存在这个“问题”，并且启用了该属性。需要持续保留此标记或属性，直到机器重新启动。但不能只是普通重启，因为那不会重新加载 QEMU；必须在已修复的 QEMU 上将机器关机后再开机。此后才能使用真正正确的机器配置。
