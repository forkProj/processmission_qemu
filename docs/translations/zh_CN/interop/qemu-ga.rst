.. _qemu-ga:

QEMU 客户机代理
========================================

概要
------------------------

**qemu-ga** [*OPTIONS*]

说明
------------------------

QEMU 客户机代理是运行在虚拟机内部的守护进程，允许虚拟机管理器主机执行操作，例如：

- 获取客户机信息。
- 设置系统时间。
- 读写文件。
- 同步并冻结文件系统。
- 挂起客户机。
- 重新配置客户机本地处理器。
- 设置用户密码。
- 等等。

启动时先读取系统配置，默认 |CONFDIR|\ ``/qemu-ga.conf``，
再解析命令行中的其余选项。同一键以最后出现的值为准，但列表会累加，格式见下文。

定义允许 RPC 列表时，默认禁止全部 RPC，只放行列表内项目。

定义禁止 RPC 列表时，默认允许全部 RPC，只禁止列表内项目。

两种列表都定义时，先默认禁止全部，再应用允许列表，最后应用禁止列表。

文件系统冻结期间，无论通用配置如何，仅允许指定安全集合中的 RPC。

选项
------------------------

.. program:: qemu-ga

.. option:: -c, --config=PATH

  配置文件路径，默认 |CONFDIR|\ ``/qemu-ga.conf``，可由 QGA_CONF 环境变量覆盖。

.. option:: -m, --method=METHOD

  传输方式：unix-listen、virtio-serial、isa-serial 或 vsock-listen，默认 virtio-serial。

.. option:: -p, --path=PATH

  设备或套接字路径。virtio-serial 默认 /dev/virtio-ports/org.qemu.guest_agent.0，
  isa-serial 默认 /dev/ttyS0。vsock-listen 地址格式为 ``<cid>:<port>``。

.. option:: -l, --logfile=PATH

  日志文件路径，默认 stderr。

.. option:: -f, --pidfile=PATH

  PID 文件，默认 /var/run/qemu-ga.pid。

.. option:: -F, --fsfreeze-hook=PATH

  启用 fsfreeze 钩子。可选参数指定冻结／解冻时运行的脚本，
  分别传入 freeze 或 thaw。默认 |CONFDIR|\ ``/fsfreeze-hook``。
  带参数使用 -F 时，选项后不能加空格，例如 ``-F/var/run/fsfreezehook.sh``。

.. option:: -t, --statedir=PATH

  保存状态信息的目录，仅支持绝对路径，默认 /var/run。

.. option:: -v, --verbose

  记录额外调试信息。

.. option:: -V, --version

  打印版本并退出。

.. option:: -d, --daemon

  启动后转入后台，脱离终端。

.. option:: -b, --block-rpcs=LIST

  逗号分隔的禁用 RPC 列表，不含空格。``--block-rpcs=help`` 列出可用 RPC。

.. option:: -a, --allow-rpcs=LIST

  逗号分隔的启用 RPC 列表，不含空格。``--allow-rpcs=help`` 列出可用 RPC。

.. option:: -D, --dump-conf

  以兼容 qemu-ga.conf 的格式输出配置并退出。

.. option:: -h, --help

  显示帮助并退出。

文件
------------------------

qemu-ga.conf 遵循 Desktop Entry Specification，由若干组键值对和其间注释组成。

::

    # qemu-ga configuration sample
    [general]
    daemonize = 0
    pidfile = /var/run/qemu-ga.pid
    verbose = 0
    method = virtio-serial
    path = /dev/virtio-ports/org.qemu.guest_agent.0
    statedir = /var/run

键对应命令行选项：

=============  ====================
键             类型
=============  ====================
daemon         布尔值
method         字符串
path           字符串
logfile        字符串
pidfile        字符串
fsfreeze-hook  字符串
statedir       字符串
verbose        布尔值
block-rpcs     字符串列表
allow-rpcs     字符串列表
=============  ====================

另见
------------------------

:manpage:`qemu(1)`
