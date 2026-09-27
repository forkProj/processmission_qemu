.. _qtest:

======================
QTest 设备模拟测试框架
======================

.. toctree::

   qgraph

QTest 是用于测试设备模型的框架，还能通过专用 qtest 协议
控制 QEMU 的一些行为，例如步进虚拟时钟。协议详见 :ref:`qtest-protocol`。

运行用例：

.. code::

   make check-qtest


库实现位于 ``tests/qtest/libqtest.c``，API 定义在 ``tests/qtest/libqtest.h``。

新增虚拟硬件时，应考虑新增 QTest；扩展已有设备功能时，也应考虑扩展测试。

libqtest 之上的 libqos 封装了驱动常见工作，例如内存管理及系统总线、设备通信。
许多设备测试使用 libqos，而非直接调用 libqtest。
它还提供 Qgraph API，提高每个测试的覆盖范围，自动配置 QEMU 命令行和设备。
详见 :ref:`qgraph`。

添加用例的步骤如下：

1. 创建测试源文件，必要时可以多个，例如 ``tests/qtest/foo-test.c``。

2. 使用 GLib 和 libqtest/libqos API 编写代码，参考现有测试及库头文件。

3. 在 ``tests/qtest/meson.build`` 注册。
   将可执行文件名加入适当的 ``qtests_*`` 变量。
   每个架构有专用变量，跨架构测试则放入 qtests_generic，例如：::

     qtests_generic = [
       ...
       'foo-test',
       ...
     ]


4. 如果有多个源文件，或需要 qemuutil、qos 之外的链接依赖，
   则加入 qtests 字典。例如，需要 QIO 的测试：::

     {
       ...
       'foo-test': [io],
       ...
     }


排查 QTest 比单元测试稍难，因为它从 QTEST_QEMU_BINARY、QTEST_QEMU_IMG 等环境变量
获取 QEMU 程序名称，而且不易向测试派生的 QEMU 进程附加 GDB。
不过手动调用测试并使用 GDB 仍很简单，可以先从以下输出找到实际命令，再手动运行：

.. code::

  make check-qtest V=1


.. _qtest-protocol:

QTest 协议
----------

.. kernel-doc:: system/qtest.c
   :doc: QTest Protocol

libqtest API 参考
-----------------

.. kernel-doc:: tests/qtest/libqtest.h

QTest 支持的环境变量
--------------------

部分环境变量指定测试使用的产物，主要是 QEMU 二进制文件，或控制测试行为。
构建系统会自动设置，但手动测试时修改它们可能很有用。
除各测试专用变量外，QTest 识别以下变量：

``QTEST_QEMU_BINARY``
    QEMU 程序本身，通常为 qemu-system-<arch>。

``QTEST_QEMU_ARGS``
    QEMU 命令行的额外参数。

``QTEST_QEMU_IMG``
    qemu-img 程序。

``QTEST_QEMU_STORAGE_DAEMON_BINARY``
    qemu-storage-daemon 程序。

``QTEST_STOP``
    在继续执行前，使用 SIGSTOP 暂停 QEMU 进程。

``QTEST_LOG``
    以逗号分隔的详细日志域列表，目前包括：

    ``qmp``
        QMP 命令调用的详细输出。

    ``qtest``
        QTest 操作的详细输出。

    ``test``
        测试自身的详细输出。

    在域名前加 ``-``，表示启用其他所有域的详细日志，同时禁用指定域。
