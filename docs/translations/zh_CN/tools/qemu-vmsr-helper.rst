===========================
QEMU 虚拟 RAPL MSR 辅助程序
===========================

用法概要
--------

**qemu-vmsr-helper** [*OPTION*]

说明
--------

实现 QEMU 的虚拟 RAPL MSR 辅助服务。

访问 RAPL（Running Average Power Limit，运行平均功率限制）MSR，
可以让 RAPL powercap 驱动提供并监控不同功率域的功耗或累计能耗，
例如 CPU 封装、DRAM，以及硬件支持的其他组件。

这些寄存器需要特权访问（CAP_SYS_RAWIO）。QEMU 可以通过外部辅助程序访问它们。

:program:`qemu-vmsr-helper` 就是该程序，
它创建监听 socket，接受 QEMU 的连接并进行通信。

在这种环境中运行虚拟机时，应将辅助程序作为系统服务启动。
配置 QEMU 连接该 socket 的方法，请参阅手册中的 RAPL MSR 支持章节。

连接 socket 后，:program:`qemu-vmsr-helper` 可以放弃 root 权限，
仅保留工作所需的能力。

它也支持 systemd socket 激活协议。
此时 systemd socket 单元应指定 Unix 流式 socket，例如：::

    [Socket]
    ListenStream=/var/run/qemu-vmsr-helper.sock

选项
--------

.. program:: qemu-vmsr-helper

.. option:: -d, --daemon

  在后台运行，并创建 PID 文件。

.. option:: -q, --quiet

  减少输出信息。

.. option:: -v, --verbose

  增加输出信息。

.. option:: -f, --pidfile=PATH

  以守护进程运行时使用的 PID 文件。默认在系统运行时状态目录创建，例如
  :file:`/var/run/qemu-vmsr-helper.pid`.

.. option:: -k, --socket=PATH

  socket 路径。默认在系统运行时状态目录创建，例如
  :file:`/var/run/qemu-vmsr-helper.sock`.

.. option:: -T, --trace [[enable=]PATTERN][,events=FILE][,file=FILE]

  .. include:: ../qemu-option-trace.rst.inc

.. option:: -u, --user=USER

  降权后使用的用户。

.. option:: -g, --group=GROUP

  降权后使用的组。

.. option:: -h, --help

  显示帮助并退出。

.. option:: -V, --version

  显示版本信息并退出。
