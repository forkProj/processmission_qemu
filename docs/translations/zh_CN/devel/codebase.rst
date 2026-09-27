========
代码库
========

本节介绍 QEMU 的各个组成部分及代码库的组织方式。
除了简要说明，也提供指向相关文档和源码的链接。

以下两个视频是了解代码库的良好入口：

- `QEMU 开发入门：总体概览 <https://www.youtube.com/watch?v=OCBLTMKLGAk>`_
- `QEMU 开发入门：系统模式 <https://www.youtube.com/watch?v=jrZ56K3Sl_k>`_

子系统
--------

完整子系统及关联文件列表见
`MAINTAINERS <https://gitlab.com/qemu-project/qemu/-/blob/master/MAINTAINERS>`_。

主要子系统包括：

- `加速器<Accelerators>`
- 块设备和 `磁盘镜像<disk images>` 支持
- `持续集成<ci>` 和 `测试<testing>`
- `设备<device-emulation>` 与开发板模型
- `文档<documentation-root>`
- `GDB 支持<GDB usage>`
- :ref:`迁移<migration>`
- `监控器<QEMU monitor>`
- :ref:`QOM（QEMU 对象模型）<qom>`
- `系统模式<System emulation>`
- :ref:`TCG（微型代码生成器）<tcg>`
- `用户模式<user-mode>` （`Linux<linux-user-mode>` 和 `BSD<bsd-user-mode>`）
- 用户界面

更多子系统文档见 :ref:`internal-subsystem`。

目录导览
--------

下面简要介绍源码顶层各目录的内容。

目录名链接指向 GitLab 仓库中的对应目录。
其他链接指向已有的更详细子系统文档。
并非所有子系统都有文档，部分情况下只能直接阅读源码。

* `accel <https://gitlab.com/qemu-project/qemu/-/tree/master/accel>`_:
  QEMU 各种 `加速器 <Accelerators>` （TCG、KVM、hvf、whpx、xen、nvmm、mshv）
  相关基础设施和架构无关代码，包含需要各
  `目标架构 <https://gitlab.com/qemu-project/qemu/-/tree/master/target>`_ 实现的操作接口。
* `audio <https://gitlab.com/qemu-project/qemu/-/tree/master/audio>`_:
  主机音频支持。
* `authz <https://gitlab.com/qemu-project/qemu/-/tree/master/authz>`_:
  `QEMU 授权框架<client authorization>`。
* `backends <https://gitlab.com/qemu-project/qemu/-/tree/master/backends>`_:
  访问主机资源的各种后端，例如随机数生成、内存支持和密码学功能。
* `block <https://gitlab.com/qemu-project/qemu/-/tree/master/block>`_:
  块设备和 `镜像格式<disk images>` 实现。
* `bsd-user <https://gitlab.com/qemu-project/qemu/-/tree/master/bsd-user>`_:
  `BSD 用户模式<bsd-user-mode>`。
* build:
  默认构建输出位置，也可以要求构建系统使用其他任意位置。
* `chardev <https://gitlab.com/qemu-project/qemu/-/tree/master/chardev>`_:
  字符设备使用的各种后端。
* `common-user <https://gitlab.com/qemu-project/qemu/-/tree/master/common-user>`_:
  处理系统调用期间信号的用户模式汇编代码。
* `configs <https://gitlab.com/qemu-project/qemu/-/tree/master/configs>`_:
  定义 QEMU 构建配置的 Makefile。
* `contrib <https://gitlab.com/qemu-project/qemu/-/tree/master/contrib>`_:
  社区贡献的设备、插件和工具。
* `crypto <https://gitlab.com/qemu-project/qemu/-/tree/master/crypto>`_:
  QEMU 使用的密码学算法。
* `disas <https://gitlab.com/qemu-project/qemu/-/tree/master/disas>`_:
  目标架构代码使用的反汇编函数。
* `docs <https://gitlab.com/qemu-project/qemu/-/tree/master/docs>`_:
  QEMU 文档。
* `dump <https://gitlab.com/qemu-project/qemu/-/tree/master/dump>`_:
  转储运行中虚拟机内存的代码。
* `ebpf <https://gitlab.com/qemu-project/qemu/-/tree/master/ebpf>`_:
  eBPF 程序支持，`virtio-net RSS<ebpf-rss>` 使用它。
* `fpu <https://gitlab.com/qemu-project/qemu/-/tree/master/fpu>`_:
  浮点软件模拟。
* `fsdev <https://gitlab.com/qemu-project/qemu/-/tree/master/fsdev>`_:
  `VirtFS <https://www.linux-kvm.org/page/VirtFS>`_ 支持。
* `gdbstub <https://gitlab.com/qemu-project/qemu/-/tree/master/gdbstub>`_:
  `GDB <GDB usage>` 支持。
* `host <https://gitlab.com/qemu-project/qemu/-/tree/master/host>`_:
  各架构专用头文件，包括密码学、原子操作和内存操作。
* `linux-headers <https://gitlab.com/qemu-project/qemu/-/tree/master/linux-headers>`_:
  从 Linux 内核导入的部分头文件，用于 KVM 和用户模式实现。
* `linux-user <https://gitlab.com/qemu-project/qemu/-/tree/master/linux-user>`_:
  `用户模式 <user-mode>` 实现，每个目标架构有独立子目录。
* `.gitlab-ci.d <https://gitlab.com/qemu-project/qemu/-/tree/master/.gitlab-ci.d>`_:
  `持续集成 <ci>` 的 YAML 和脚本。
* `include <https://gitlab.com/qemu-project/qemu/-/tree/master/include>`_:
  各子系统头文件，目录层级对应源码的组织和命名。
* `hw <https://gitlab.com/qemu-project/qemu/-/tree/master/hw>`_:
  `设备 <device-emulation>` 和开发板模拟，按类型、协议或架构划分到对应子目录。
* `io <https://gitlab.com/qemu-project/qemu/-/tree/master/io>`_:
  QEMU 的 `I/O 通道 <https://lists.gnu.org/archive/html/qemu-devel/2015-11/msg04208.html>`_。
* `libdecnumber <https://gitlab.com/qemu-project/qemu/-/tree/master/libdecnumber>`_:
  从 GCC 导入的库，用于十进制算术。
* `migration <https://gitlab.com/qemu-project/qemu/-/tree/master/migration>`__:
  :ref:`迁移框架 <migration>`。
* `monitor <https://gitlab.com/qemu-project/qemu/-/tree/master/monitor>`_:
  `监控器 <QEMU monitor>` 实现，包括 HMP 和 QMP。
* `nbd <https://gitlab.com/qemu-project/qemu/-/tree/master/nbd>`_:
  QEMU NBD（网络块设备）服务器。
* `net <https://gitlab.com/qemu-project/qemu/-/tree/master/net>`_:
  主机网络支持。
* `pc-bios <https://gitlab.com/qemu-project/qemu/-/tree/master/pc-bios>`_:
  预构建固件和启动镜像，无需编译即可在 QEMU 中使用。
* `plugins <https://gitlab.com/qemu-project/qemu/-/tree/master/plugins>`_:
  :ref:`TCG 插件 <tcg-plugins>` 核心实现。
  插件示例位于 `tests <https://gitlab.com/qemu-project/qemu/-/tree/master/tests/tcg/plugins>`__
  和 `contrib <https://gitlab.com/qemu-project/qemu/-/tree/master/contrib/plugins>`__。
* `po <https://gitlab.com/qemu-project/qemu/-/tree/master/po>`_:
  翻译文件。
* `python <https://gitlab.com/qemu-project/qemu/-/tree/master/python>`_:
  构建和测试系统中的 Python 部分。
* `qapi <https://gitlab.com/qemu-project/qemu/-/tree/master/qapi>`_:
  `QAPI <qapi>` 实现。
* `qobject <https://gitlab.com/qemu-project/qemu/-/tree/master/qobject>`_:
  QEMU Object 实现。
* `qga <https://gitlab.com/qemu-project/qemu/-/tree/master/qga>`_:
  QEMU `客户机代理 <qemu-ga>` 实现。
* `qom <https://gitlab.com/qemu-project/qemu/-/tree/master/qom>`_:
  QEMU :ref:`对象模型 <qom>` 实现及相关监控器命令。
* `replay <https://gitlab.com/qemu-project/qemu/-/tree/master/replay>`_:
  QEMU :ref:`记录／重放 <replay>` 实现。
* `roms <https://gitlab.com/qemu-project/qemu/-/tree/master/roms>`_:
  各种固件和 ROM 的源码，需要定制或新版时可以自行编译。
* `rust <https://gitlab.com/qemu-project/qemu/-/tree/master/rust>`_:
  QEMU 的 Rust 集成，包含新定义的接口和使用它们的设备。
* `scripts <https://gitlab.com/qemu-project/qemu/-/tree/master/scripts>`_:
  构建、测试脚本，以及处理 QEMU 代码库和执行跟踪的各种工具。
* `scsi <https://gitlab.com/qemu-project/qemu/-/tree/master/scsi>`_:
  SCSI 设备使用的支持代码。
* `semihosting <https://gitlab.com/qemu-project/qemu/-/tree/master/semihosting>`_:
  QEMU `半主机 <Semihosting>` 实现。
* `stats <https://gitlab.com/qemu-project/qemu/-/tree/master/stats>`_:
  `监控器 <QEMU monitor>` 的统计命令实现。
* `storage-daemon <https://gitlab.com/qemu-project/qemu/-/tree/master/storage-daemon>`_:
  QEMU `存储守护进程 <storage-daemon>` 实现。
* `stubs <https://gitlab.com/qemu-project/qemu/-/tree/master/stubs>`_:
  用于特定配置构建的各种桩函数，即空函数。
* `subprojects <https://gitlab.com/qemu-project/qemu/-/tree/master/subprojects>`_:
  构建系统使用的 QEMU 子模块。
* `system <https://gitlab.com/qemu-project/qemu/-/tree/master/system>`_:
  QEMU `系统模式 <System emulation>` 实现，包括 CPU、MMU 和启动支持。
* `target <https://gitlab.com/qemu-project/qemu/-/tree/master/target>`_:
  全部受支持目标架构的代码，每个架构一个子目录，其中包含各加速器的架构专用实现。
* `tcg <https://gitlab.com/qemu-project/qemu/-/tree/master/tcg>`_:
  :ref:`TCG <tcg>` 相关代码，每个受支持主机架构一个子目录。
* `tests <https://gitlab.com/qemu-project/qemu/-/tree/master/tests>`_:
  QEMU `测试 <testing>` 套件。

  - `data <https://gitlab.com/qemu-project/qemu/-/tree/master/tests/data>`_:
    各种测试数据。
  - `decode <https://gitlab.com/qemu-project/qemu/-/tree/master/tests/decode>`_:
    :ref:`decodetree <decodetree>` 实现的测试套件。
  - `docker <https://gitlab.com/qemu-project/qemu/-/tree/master/tests/docker>`_:
    创建 `持续集成 <ci>` 所用 `容器 <container-ref>` 的代码和脚本。
  - `fp <https://gitlab.com/qemu-project/qemu/-/tree/master/tests/fp>`_:
    QEMU 软件浮点实现的测试套件。
  - `functional <https://gitlab.com/qemu-project/qemu/-/tree/master/tests/functional>`_:
    `功能测试 <checkfunctional-ref>`，完整启动虚拟机。
  - `lcitool <https://gitlab.com/qemu-project/qemu/-/tree/master/tests/lcitool>`_:
    为 CI 容器生成 Dockerfile。
  - `migration <https://gitlab.com/qemu-project/qemu/-/tree/master/tests/migration>`_:
    :ref:`迁移框架 <migration>` 的测试脚本和数据。
  - `multiboot <https://gitlab.com/qemu-project/qemu/-/tree/master/tests/multiboot>`_:
    测试 x86_64/i386 的 multiboot 功能。
  - `qapi-schema <https://gitlab.com/qemu-project/qemu/-/tree/master/tests/qapi-schema>`_:
    `QAPI <qapi-tests>` 的测试脚本和数据。
  - `qemu-iotests <https://gitlab.com/qemu-project/qemu/-/tree/master/tests/qemu-iotests>`_:
    `磁盘镜像和块层测试 <qemu-iotests>`。
  - `qtest <https://gitlab.com/qemu-project/qemu/-/tree/master/tests/qtest>`_:
    `设备模拟测试 <qtest>`。
  - `tcg <https://gitlab.com/qemu-project/qemu/-/tree/master/tests/tcg>`__:
    `TCG 相关测试 <checktcg-ref>`，包含按架构划分的子目录及跨架构测试。
  - `tsan <https://gitlab.com/qemu-project/qemu/-/tree/master/tests/tsan>`_:
    线程检测器的 `抑制规则 <tsan-suppressions>`。
  - `uefi-test-tools <https://gitlab.com/qemu-project/qemu/-/tree/master/tests/uefi-test-tools>`_:
    UEFI 支持测试工具。
  - `unit <https://gitlab.com/qemu-project/qemu/-/tree/master/tests/unit>`_:
    QEMU `单元测试 <unit-tests>`。
* `trace <https://gitlab.com/qemu-project/qemu/-/tree/master/trace>`_:
  :ref:`跟踪框架 <tracing>`，输出执行期间各种事件的信息。
* `ui <https://gitlab.com/qemu-project/qemu/-/tree/master/ui>`_:
  QEMU 用户界面。
* `util <https://gitlab.com/qemu-project/qemu/-/tree/master/util>`_:
  供 QEMU 其他部分使用的辅助代码。
