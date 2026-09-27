=====================
QEMU 持久预留辅助程序
=====================

用法概要
--------

**qemu-pr-helper** [*OPTION*]

说明
--------

实现 QEMU 的持久预留辅助服务。

在共享存储环境中，SCSI 持久预留可以将块设备访问限制为特定发起端。
构建虚拟机集群时，虚拟机通常需要发送持久预留 SCSI 命令。
但错误使用这些命令可能破坏存储网络的正常运行，因此操作系统限制非特权程序发送它们。
QEMU 的 SCSI 直通设备 ``scsi-block`` 和 ``scsi-generic`` 支持将客户机请求
转交给外部特权辅助程序。:program:`qemu-pr-helper` 就是该程序，
它创建监听 socket，接受 QEMU 的连接并进行通信。

在这种环境中运行虚拟机时，应将辅助程序作为系统服务启动。
配置 QEMU 连接该 socket 的方法，请参阅手册中关于持久预留管理器的章节。

连接 socket 后，:program:`qemu-pr-helper` 可以放弃 root 权限，
仅保留工作所需的能力。

它也支持 systemd socket 激活协议。
此时 systemd socket 单元应指定 Unix 流式 socket，例如：::

    [Socket]
    ListenStream=/var/run/qemu-pr-helper.sock

选项
--------

.. program:: qemu-pr-helper

.. option:: -d, --daemon

  在后台运行，并创建 PID 文件。

.. option:: -q, --quiet

  减少输出信息。

.. option:: -v, --verbose

  增加输出信息。

.. option:: -f, --pidfile=PATH

  以守护进程运行时使用的 PID 文件。默认在系统运行时状态目录创建，例如
  :file:`/var/run/qemu-pr-helper.pid`.

.. option:: -k, --socket=PATH

  socket 路径。默认在系统运行时状态目录创建，例如
  :file:`/var/run/qemu-pr-helper.sock`.

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
