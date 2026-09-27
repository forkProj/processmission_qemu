=======================
QEMU SystemTap 跟踪工具
=======================

用法概要
--------

**qemu-trace-stap** [*GLOBAL-OPTIONS*] *COMMAND* [*COMMAND-OPTIONS*] *ARGS*...

说明
--------

``qemu-trace-stap`` 方便使用 SystemTap 跟踪 QEMU 模拟器的执行。

它是 ``stap`` 的包装程序，因此需要先安装 SystemTap 运行时环境。

选项
--------

.. program:: qemu-trace-stap

无论执行哪条命令，都可以使用以下全局选项：

.. option:: --verbose, -v

  显示命令执行的详细信息。

支持以下命令：

.. option:: list BINARY PATTERN...

  列出 *BINARY* 提供的、匹配 *PATTERN* 的全部探针名称。

  如果 *BINARY* 不是绝对路径，则在 ``$PATH`` 列出的目录中查找。

  *PATTERN* 是用于过滤结果的普通字符串，可以包含 ``*`` 通配符，
  以便一次匹配多个探针，而不必逐个列出。
  可以指定多个 *PATTERN*，列出匹配任意模式的探针。
  未指定模式时，列出全部可用探针。

  例如，列出 |qemu_system| 中的全部探针：

  .. parsed-literal::

    $ qemu-trace-stap list |qemu_system|


  对不在 ``$PATH`` 中的二进制文件，只列出 QEMU 密码子系统相关探针：

  .. parsed-literal::

    $ qemu-trace-stap list /opt/qemu/|version|/bin/|qemu_system| 'qcrypto*'


.. option:: run OPTIONS BINARY PATTERN...

  运行跟踪会话。每当执行 *BINARY* 的进程触发匹配 *PATTERN* 的探针时，
  打印格式化输出。

  如果 *BINARY* 不是绝对路径，则在 ``$PATH`` 列出的目录中查找。

  *PATTERN* 是匹配 *LIST* 命令所显示探针名称的普通字符串，
  可以包含 ``*`` 通配符，以便一次匹配多个探针。
  可以指定多个模式，监控全部匹配探针。
  至少必须提供一个模式，因为 stap 无法在不溢出跟踪缓冲区的情况下，
  同时跟踪全部已知 QEMU 探针。

  该命令不必与 QEMU 进程同步启动。
  除非限定特定进程，否则它会匹配所有当前运行和之后启动的进程中的探针。

  此命令的专用选项如下：

  .. program:: qemu-trace-stap-run

  .. option:: --pid=PID, -p PID

    仅跟踪 *PID* 指定的进程。

  例如，监控所有执行 ``$PATH`` 中 |qemu_system| 的进程，
  显示全部 I/O 相关探针：

  .. parsed-literal::

    $ qemu-trace-stap run |qemu_system| 'qio*'


  只监控 PID 为 1732 的 QEMU 进程：

  .. parsed-literal::

    $ qemu-trace-stap run --pid=1732 |qemu_system| 'qio*'


  监控使用 ``$PATH`` 之外另一个二进制文件的 QEMU 进程，
  并显示跟踪环境设置的详细信息：

  .. parsed-literal::

    $ qemu-trace-stap -v run /opt/qemu/|version|/bin/|qemu_system| 'qio*'


另请参阅
--------

:manpage:`qemu(1)`, :manpage:`stap(1)`

..
  Copyright (C) 2019 Red Hat, Inc.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
