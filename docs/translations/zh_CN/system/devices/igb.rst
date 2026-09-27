.. SPDX-License-Identifier: GPL-2.0-or-later
.. _igb:

igb
---

igb 是 Intel 千兆以太网控制器系列，QEMU 具体实现 82576，数据手册见 [1]_。

它旨在无需物理硬件即可测试 SR-IOV 网络。

限制
===========

初期使用 Linux Test Project [2]_ 和 Windows HLK [3]_ 测试，后来也使用
DPDK Test Suite [4]_。LTP 命令为：

.. code-block:: shell

  network.sh -6mta


该实现缺少真实硬件的许多功能。使用 DPDK、Linux、Windows 之外的系统，
或测试未覆盖的功能时，可能遇到各种失败。

使用 igb
=========

与其他网卡无异，一般说明见 :ref:`Network_emulation`。

客户机启用 SR-IOV 可能还需额外步骤，Linux 见 [5]_。

开发 igb
==============

igb 是 e1000e 的后继，后者又是 e1000 的后继。三者相似，若修改也适用于
e1000e 或 e1000，请同步应用。

提交前务必测试。QEMU 内测试很少，还应运行可能受影响的应用，确认完整系统
正常工作。

测试 igb
===========

有基础功能 qtest，在构建目录运行：

.. code-block:: shell

  pyvenv/bin/meson test qtest-x86_64/qos-test


ethtool 可测试寄存器、中断等，已自动化为功能测试，在构建目录执行：

.. code:: shell

  pyvenv/bin/meson test --suite thorough func-x86_64-netdev_ethtool


参考资料
==========

.. [1] https://www.intel.com/content/dam/www/public/us/en/documents/datasheets/82576eb-gigabit-ethernet-controller-datasheet.pdf
.. [2] https://github.com/linux-test-project/ltp
.. [3] https://learn.microsoft.com/en-us/windows-hardware/test/hlk/
.. [4] https://doc.dpdk.org/dts/gsg/
.. [5] https://docs.kernel.org/PCI/pci-iov-howto.html
