==============================
使用 biosbits 测试 ACPI/SMBIOS
==============================
********
简介
********

Biosbits 由 Josh Triplett 编写，可从 https://biosbits.org/ 下载，
`源码位于 GitHub <https://github.com/biosbits/bits/tree/master>`__。
它不经过操作系统，直接通过 ACPICA BIOS 解释器执行 ACPI、SMBIOS 表等组件。
ACPICA 是 Intel 编写的免费 C 库，位于 https://acpica.org/ ，已包含在 biosbits 中。
Biosbits 将 Python 与 GRUB 集成，测试可以用 Python 编写，
而无需使用 GRUB 原生的类 Bash 脚本语言。

相比通过操作系统间接发现 BIOS 问题，在物理机或虚拟机中直接测试有多项优势。
操作系统通常会绕过 BIOS 问题，将其隐藏起来；
尽量接近运行中的 BIOS，避免复杂操作系统介入，可以更精确地控制测试内容和方式。
另外，测试 ACPI、SMBIOS 等需要最高硬件特权级，例如 x86 的 ring 0。
普通程序处于 ring 3，而操作系统运行在 ring 0，因此往往必须修改操作系统才能测试，
并非所有场景都允许这样做。
最后，测试框架和用例适合用 Python 等高层语言编写，
而系统及模块通常使用 C 和汇编，用这些低层语言编写测试更繁琐。
这些原因使 biosbits 很适合 BIOS 测试。
开发动机与实际用途见 `2011 年 Plumbers 演讲 <Plumbers_>`__
和 `2012 年 Linux.conf.au 演讲 <Linux.conf.au_>`__。

QEMU 在 `gitlab`_ 维护 biosbits 分支及其依赖子模块，
包含大量修复、较新的 ACPICA，以及运行 QEMU 功能测试所需的修改。
本文作者目前维护该分支，更多介绍见 `作者的 FOSDEM 演讲 <FOSDEM_>`__。

.. _Plumbers: https://blog.linuxplumbersconf.org/2011/ocw/system/presentations/867/original/bits.pdf
.. _Linux.conf.au: https://www.youtube.com/watch?v=36QIepyUuhg
.. _gitlab: https://gitlab.com/qemu-project/biosbits-bits
.. _FOSDEM: https://fosdem.org/2024/schedule/event/fosdem-2024-2262-exercising-qemu-generated-acpi-smbios-tables-using-biosbits-from-within-a-guest-vm-/

************
测试框架说明
************

``tests/functional/test_acpi_bits.py`` 是驱动整个流程的 QEMU 功能测试。

以下介绍相关文件，以 tests/functional 为根目录：

::

   ├── acpi-bits
   │ ├── bits-config
   │ │ └── bits-cfg.txt
   │ ├── bits-tests
   │   ├── smbios.py2
   │   ├── testacpi.py2
   │   └── testcpuid.py2
   ├── test_acpi_bits.py


* ``tests/functional``：

   ``test_acpi_bits.py``：
   主 Python 脚本，生成 biosbits ISO，使用它启动 QEMU 虚拟机，
   收集日志并报告失败。如果要改进日志解析，或修改 QEMU 启动命令行，应编辑此文件。
   一般测试作者无需修改它，除非需要调整日志解析。
   设置 **V=1** 可启用详细模式，失败时输出完整 biosbits 日志及更多信息。
   设置 **BITS_DEBUG=1** 可启用调试模式，显示详细日志并保留临时工作目录，
   便于检查和手动执行具体命令。

   在 QEMU 构建目录中，假设源码位于 ..，执行：
   ::

     $ export PYTHONPATH=../python:../tests/functional
     $ export QEMU_TEST_QEMU_BINARY=$PWD/qemu-system-x86_64
     $ python3 ../tests/functional/test_acpi_bits.py


   这会运行全部 acpi-bits 功能测试，并输出 TAP 格式结果。

   查看 ``tests/functional/x86_64/test_acpi_bits.*/`` 中的日志了解运行情况或排查问题。
   设置 V=1 会在日志中加入更多诊断信息。

* ``tests/functional/acpi-bits/bits-config``：

   保存决定 biosbits 如何运行测试的配置。

   ``bits-config.txt``：
   指定 bits 执行哪些测试或操作，选项说明位于文件内。

* ``tests/functional/acpi-bits/bits-tests``：

   保存将在虚拟机 biosbits 环境中执行的 Python 测试。
   可以向相应文件添加用例，例如在 testacpi.py2 中加入 ACPI 测试，
   并调用 testsuite.add_test() 注册。

   有时需要禁用部分子测试，或引入此目录尚未包含的套件。
   此时请克隆 https://gitlab.com/qemu-project/biosbits-bits/-/tree/qemu-bits 。
   必须使用包含 QEMU 专用改进的 qemu-bits 分支，而不是 bits 分支。
   将需要修改的套件或脚本从源码的 python 目录复制到此处。
   例如，修改 CPUID 测试时，复制
   https://gitlab.com/qemu-project/biosbits-bits/-/blob/qemu-bits/python/testcpuid.py ，
   并将扩展名改为 .py2，然后进行修改。具体步骤为：

       (a) 从 bits 源码复制未修改的测试脚本。
       (b) 添加 SPDX 许可证头。
       (c) 修改测试。

   建议分别提交，以清楚区分原脚本和本项目修改；(a) 与 (b) 有时可以合并。

   框架随后会自动使用修改后的脚本，无需其他改动。
   请检查日志，确认修改已经生效。

   .py2 扩展名表示：

   (a) 脚本基于 Python 2.7，而非 Python 3。
   (b) 脚本在 biosbits 虚拟机内执行，不纳入 QEMU 构建及测试 Python 脚本的维护和依赖解析。
   (c) 避免运行测试时被框架意外加载。

作者：Ani Sinha <anisinha@redhat.com>
