.. _Glossary:

--------
术语表
--------

本节简要解释 QEMU 开发者使用的缩写和术语。

加速器（Accelerator）
---------------------

用于加速客户机指令执行的特定 API。可以基于硬件，
通过宿主操作系统的虚拟化 API（kvm、hvf、whpx、mshv 等）实现，
也可以基于软件（tcg）。参见 `支持的加速器 <Accelerators>`。

开发板（Board）
---------------

:ref:`machine` 的另一种称呼。

块（Block）
-----------

块驱动提供可用的 `磁盘格式和前端 <block-drivers>`，
块设备使用它们实现虚拟机磁盘，参见 `选项页面的块设备部分 <sec_005finvocation>`。

CFI
--------

控制流完整性是一种加固技术，通过检测执行中的意外跳转，防止针对 QEMU 的漏洞利用。
QEMU `积极支持 <cfi>` 在编译时启用 CFI。

设备（Device）
--------------

在 QEMU 中，设备是客户机可见的硬件，例如 UART、PCI 控制器、PCI 卡、VGA 控制器等。

QEMU 可以模拟 CPU 及所有与之交互的硬件，包括 `许多设备 <device-emulation>`。
即使使用硬件加速器运行虚拟机，QEMU 仍负责通过软件模拟设备。

EDK2
--------

EDK2，也称 `TianoCore <https://www.tianocore.org/>`_，是 UEFI 标准的开源实现。
使用 UEFI 固件启动的 QEMU 虚拟机通常采用 EDK2。

gdbstub
--------

QEMU 实现了 `GDB 服务器 <GDB usage>`，
允许 GDB 连接并调试运行中的虚拟机或用户态程序，
从而调试 QEMU 内执行的客户机代码。

glib2
--------

`GLib2 <https://docs.gtk.org/glib/>`_ 是 QEMU 代码中最重要的基础库之一，
提供数据结构、宏、字符串及线程工具，以及跨操作系统的可移植函数。
构建 QEMU 必须具备此库。

客户机代理（Guest agent）
-------------------------

`QEMU 客户机代理 <qemu-ga>` 是运行在虚拟机内部的守护进程，
提供多种服务，帮助 QEMU 与客户机交互。

.. _guest:

客户机（Guest）
---------------

客户机指被模拟的虚拟机架构，另见 :ref:`host`。

有时也称 :ref:`target` 架构，但 target 一词可能存在歧义。

.. _host:

宿主（Host）
------------

宿主指 QEMU 自身运行的原生架构，另见 :ref:`guest`。

虚拟机监控器（Hypervisor）
--------------------------

正式定义上，虚拟机监控器是用于管理虚拟机的程序或 API。
QEMU 是虚拟化工具，会与多种虚拟机监控器交互。

在 QEMU 语境中，通常指宿主操作系统提供的、用于执行虚拟机的 API。
Linux 可选择 KVM、Xen 或 MSHV；macOS 提供 HVF；Windows 提供 WHPX；
NetBSD 提供 NVMM。

.. _machine:

机器（Machine）
---------------

QEMU 系统模拟为多种硬件建模。机器模型（有时称开发板模型）描述完整虚拟系统，
包括 RAM、一个或多个 CPU，以及各种设备。
可以通过 qemu-system 的 ``-machine`` 选择，现有模型见 `机器列表 <system-targets-ref>`。

迁移（Migration）
-----------------

QEMU 可以在不同宿主系统之间保存并恢复虚拟机执行状态，
由 :ref:`迁移框架 <migration>` 提供支持。

NBD
--------

`QEMU 网络块设备服务器 <qemu-nbd>` 可用于挂载和访问 QEMU 镜像，
提供类似 loop 设备的功能。

邮件列表（Mailing List）
------------------------

`邮件列表 <https://wiki.qemu.org/Contribute/MailingLists>`_ 是开发协作的主要场所。
修改以补丁系列发布，所有开发者都可以审阅并反馈。

报告问题时，最好使用 `GitLab 问题跟踪器 <https://gitlab.com/qemu-project/qemu/-/issues>`_。

.. _softmmu:

MMU / softmmu
-------------

内存管理单元负责将虚拟地址转换为物理地址，并管理内存保护。
QEMU 系统模式称为 softmmu，正是因为它为客户机虚拟机在软件中实现了这些功能，
包括 TLB（地址转换后备缓冲区）。

QEMU 用户态并不实现完整的软件 MMU，
而是通过添加特定偏移量转换虚拟地址，依赖宿主的 MMU 和操作系统。

Monitor / QMP / HMP
-------------------

`QEMU 监视器 <QEMU monitor>` 是与运行中的虚拟机交互的文本接口。

QMP 是 QEMU Monitor Protocol 的缩写，提供基于 JSON 的接口。
HMP 是 Human Monitor Protocol 的缩写，提供一组文本命令，
适合偏好可读命令而非 JSON 的用户。

MTTCG
--------

早期多 CPU 支持使用单线程轮询算法。
后来开发的 `多线程 TCG <mttcg>` 可以利用多个核心加速执行。

插件（Plugins）
---------------

`TCG 插件 <TCG Plugins>` API 用于在系统模式和用户模式下对客户机代码插桩，
目标是提供类似 `DynamoRIO <https://dynamorio.org/>`_ 或
`valgrind <https://valgrind.org/>`_ 的功能。

QEMU 插件的一个重要优势是可以进行与架构无关的插桩。

Patchew
--------

`Patchew <https://patchew.org/QEMU/>`_ 是跟踪邮件列表补丁的网站。

PR
--

补丁系列经审阅并由子系统维护者接受后，会进入 PR（Pull Request，拉取请求）。
项目维护者运行测试后，将其合入 QEMU 主分支。

QEMU 项目目前通常不要求普通开发者直接提交拉取请求。

QCOW2
--------

QEMU Copy On Write 是 QEMU 开发的磁盘格式。
相比 raw 镜像，它具有透明压缩、自动扩展等优势。

推荐使用 qcow2 格式。

QEMU
--------

`QEMU（Quick Emulator） <https://www.qemu.org/>`_ 是通用、开源的机器模拟器和虚拟化工具。

QOM
--------

:ref:`QEMU 对象模型 <qom>` 是面向对象的 API，
用于在 QEMU 代码中定义各种设备和硬件。

记录与重放（Record/replay）
---------------------------

:ref:`记录与重放 <replay>` 使虚拟机执行具有确定性，并能重复再现。

Rust
--------

`Rust <https://www.rust-lang.org/>`_ 是默认提供内存安全的新型编程语言。
QEMU 正在推进将它用于多个子系统。

系统模式（System mode）
-----------------------

QEMU 系统模式提供整台机器的虚拟模型，包括 CPU、内存和模拟设备，
用于运行客户机操作系统。
CPU 可以完全模拟，也可以配合 KVM、Xen 或 Hypervisor.Framework 等虚拟机监控器，
让客户机直接运行在宿主 CPU 上。

此模式也称 :ref:`softmmu <softmmu>`。

.. _target:

目标（Target）
--------------

target 一词可能有歧义。在 QEMU 的大多数场景中，它是 :ref:`guest` 的同义词。
例如，模拟 Arm CPU 的代码位于 ``target/arm/``。
但在 :ref:`TCG 子系统 <tcg>` 中，target 指 QEMU 自身运行的架构，即 :ref:`host`。

TCG
--------

TCG 是 QEMU 的 `Tiny Code Generator <tcg>`，
即用于通过软件模拟客户机 CPU 的即时（JIT）编译器。

它是 QEMU 支持的加速器之一，支持多种客户机和宿主架构。

用户模式（User mode）
---------------------

QEMU 用户模式可以在一种 CPU 上运行面向另一种 CPU 编译的进程。
此模式始终模拟 CPU，并将客户机系统调用转换为宿主内核系统调用。
目前支持 Linux 和 BSD。

VirtIO
--------

VirtIO 是定义和实现低开销虚拟设备的开放标准，规定了一组数据结构和超级调用。
超级调用类似系统调用，但面向虚拟机监控器，此处即 QEMU。
与模拟真实设备相比，VirtIO 通过减少客户机与监控器之间的交互，提高效率。

vhost-user
----------

`Vhost-user <vhost_user>` 是在 QEMU 进程之外实现 VirtIO 设备的接口。
