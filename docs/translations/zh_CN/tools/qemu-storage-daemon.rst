.. _storage-daemon:

=================
QEMU 存储守护进程
=================

用法概要
--------

**qemu-storage-daemon** [options]

说明
--------

``qemu-storage-daemon`` 在长期运行的进程中提供 QEMU、``qemu-img`` 和
``qemu-nbd`` 的磁盘镜像功能，通过 QMP 控制，无需运行虚拟机。
它可以导出镜像、执行块作业及其他磁盘操作，
由 QMP 监视器控制，并通过命令行提供初始配置。

它提供 QEMU 的以下功能子集：

* 块节点
* 块作业
* 块导出
* 限流组
* 字符设备
* 密码与机密数据
* QMP
* IOThreads

可通过 QEMU Monitor Protocol（QMP）连接发送命令。
命令说明见 :manpage:`qemu-storage-daemon-qmp-ref(7)`。

进程持续运行，直到收到 QMP ``quit`` 命令，或 SIGINT、SIGHUP、SIGTERM 信号。

**警告：** 不要修改正在被虚拟机或其他进程使用的镜像，否则可能损坏镜像。
查询被其他进程修改的镜像时，也可能遇到不一致状态。

选项
--------

.. program:: qemu-storage-daemon

标准选项：

.. option:: -h, --help

  显示帮助并退出。

.. option:: -V, --version

  显示版本信息并退出。

.. option:: -T, --trace [[enable=]PATTERN][,events=FILE][,file=FILE]

  .. include:: ../qemu-option-trace.rst.inc

.. option:: --blockdev BLOCKDEVDEF

  定义块节点。节点属性见 :manpage:`qemu(1)`，
  驱动专用参数见 :manpage:`qemu-block-drivers(7)`。

.. option:: --chardev CHARDEVDEF

  定义字符设备，属性说明见 :manpage:`qemu(1)`。
  常见用法是配置 Unix 域 socket：::

  --chardev socket,id=char1,path=/var/run/qsd-qmp.sock,server=on,wait=off


.. option:: --export [type=]nbd,id=<id>,node-name=<node-name>[,name=<export-name>][,writable=on|off][,bitmap=<name>]
  --export [type=]vhost-user-blk,id=<id>,node-name=<node-name>,addr.type=unix,addr.path=<socket-path>[,writable=on|off][,logical-block-size=<block-size>][,num-queues=<num-queues>]
  --export [type=]vhost-user-blk,id=<id>,node-name=<node-name>,addr.type=fd,addr.str=<fd>[,writable=on|off][,logical-block-size=<block-size>][,num-queues=<num-queues>]
  --export [type=]fuse,id=<id>,node-name=<node-name>,mountpoint=<file>[,growable=on|off][,writable=on|off][,allow-other=on|off|auto]
  --export [type=]vduse-blk,id=<id>,node-name=<node-name>,name=<vduse-name>[,writable=on|off][,num-queues=<num-queues>][,queue-size=<queue-size>][,logical-block-size=<block-size>][,serial=<serial-number>]

  定义块导出。``node-name`` 指定要导出的块节点。
  ``writable`` 决定是否允许修改数据的写请求，默认为 off。

  ``nbd`` 类型需要 ``--nbd-server``，详见下文。
  ``name`` 是 NBD 导出名称，未指定时使用 ``node-name``。
  ``bitmap`` 是从该块节点可达的脏位图名称。
  NBD 客户端可以使用 NBD_OPT_SET_META_CONTEXT，
  指定元数据上下文 "qemu:dirty-bitmap:BITMAP" 来检查位图。

  ``vhost-user-blk`` 类型指定用于接受连接的 vhost-user socket 地址。
  支持 Unix 域 socket 的 ``addr.type=unix,addr.path=<socket-path>``，
  以及文件描述符传递的 ``addr.type=fd,addr.str=<fd>``。
  ``logical-block-size`` 设置逻辑块大小，单位为字节，默认 512；
  ``num-queues`` 设置 virtqueue 数量，默认 1。

  ``fuse`` 类型指定用于导出块节点的挂载点，该挂载点必须是普通文件。
  文件本身不变，只在导出生效期间呈现为块节点的内容。
  这类似于在目录上挂载文件系统：原目录内容不变，挂载期间显示另一组内容。
  因此，导出生效前已打开该文件的应用仍会看到原内容。
  设置 ``growable`` 后，写入超出导出文件末尾的位置会相应扩展块节点。
  ``allow-other`` 控制运行进程的用户之外的其他用户是否可以访问导出。
  非 root 用户启用此选项，需要在全局 fuse.conf 中启用 user_allow_other。
  默认值 auto 会先尝试启用，出错时再退回禁用状态。

  ``vduse-blk`` 类型使用 ``name`` 创建 VDUSE 设备，名称必须在整个宿主上唯一。
  ``num-queues`` 设置 virtqueue 数量，默认 1。
  ``queue-size`` 设置 virtqueue 描述符表大小，默认 256。

  创建的 VDUSE 设备还必须通过 iproute2 项目的 vdpa(8) 命令加入 vDPA 总线：::

  # vdpa dev add name <id> mgmtdev vduse

  之后可用以下命令从 vDPA 总线移除设备：::

  # vdpa dev del <id>

  关于通过 virtio_vdpa.ko 将 vDPA 设备接入宿主，
  或通过 vhost_vdpa.ko 接入客户机，详见
  https://vdpa-dev.gitlab.io/ 。

  VDUSE 的更多说明见
  https://docs.kernel.org/userspace-api/vduse.html 。


.. option:: --monitor MONITORDEF

  定义 QMP 监视器，属性说明见 :manpage:`qemu(1)`。
  常见用法是在字符设备 ``char1`` 上配置监视器：::

  --monitor chardev=char1


.. option:: --nbd-server addr.type=inet,addr.host=<host>,addr.port=<port>[,tls-creds=<id>][,tls-authz=<id>][,max-connections=<n>]
  --nbd-server addr.type=unix,addr.path=<path>[,tls-creds=<id>][,tls-authz=<id>][,max-connections=<n>]
  --nbd-server addr.type=fd,addr.str=<fd>[,tls-creds=<id>][,tls-authz=<id>][,max-connections=<n>]

  为 NBD 导出定义服务器，支持 TCP 和 Unix 域 socket。
  可以通过文件描述符传递提供监听 socket，见下文示例。
  TLS 加密可以通过 ``--object`` 的 tls-creds-* 和 authz-* 机密对象配置，见下文。

  在 Unix 域 socket ``/var/run/qsd-nbd.sock`` 上配置 NBD 服务器：::

  --nbd-server addr.type=unix,addr.path=/var/run/qsd-nbd.sock


.. option:: --object help
  --object <type>,help
  --object <type>[,<property>=<value>...]

  定义 QEMU 用户可创建对象。使用 ``help`` 列出对象类型，
  使用 ``<type>,help`` 列出对象属性。属性说明见 :manpage:`qemu(1)`。

.. option:: --pidfile PATH

  守护进程写入 PID 的文件路径。
  脚本可以据此发送信号停止进程：::

    $ kill -SIGTERM $(<path/to/qsd.pid)


  该文件会被加锁，因此同一个 PID 文件路径只能对应一个运行实例。
  守护进程退出时会删除 PID 文件。

  PID 文件在字符设备、导出和 NBD 服务器创建完成后，接受连接之前写入。
  文件写入表示守护进程已成功启动，客户端可以开始连接。

.. option:: --daemonize

  使进程在后台以守护进程方式运行。
  启动完成（即 PID 文件已写入，或已到应写入的阶段）或失败后，父进程退出。
  父进程退出码表示子进程是否成功启动。

示例
--------

启动带有 ``qmp.sock`` 监视器 socket 的守护进程，供客户端执行 QMP 命令：::

  $ qemu-storage-daemon \
      --chardev socket,path=qmp.sock,server=on,wait=off,id=char1 \
      --monitor chardev=char1


从 Python 启动守护进程，通过文件描述符传递提供 QMP socket，
避免忙等监视器就绪：::

  #!/usr/bin/env python3
  import subprocess
  import socket

  sock_path = '/var/run/qmp.sock'

  with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as listen_sock:
      listen_sock.bind(sock_path)
      listen_sock.listen()

      fd = listen_sock.fileno()

      subprocess.Popen(
          ['qemu-storage-daemon',
           '--chardev', f'socket,fd={fd},server=on,id=char1',
           '--monitor', 'chardev=char1'],
          pass_fds=[fd],
      )

  # listen_sock was automatically closed when leaving the 'with' statement
  # body. If the daemon process terminated early then the following connect()
  # will fail with "Connection refused" because no process has the listen
  # socket open anymore. Launch errors can be detected this way.

  qmp_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
  qmp_sock.connect(sock_path)
  ...QMP interaction...


同样的 socket 创建方式也适用于 ``--nbd-server addr.type=fd,addr.str=<fd>``
和 ``--export type=vhost-user-blk,addr.type=fd,addr.str=<fd>``。

通过 NBD Unix 域 socket ``nbd.sock`` 导出 raw 镜像 ``disk.img``：::

  $ qemu-storage-daemon \
      --blockdev driver=file,node-name=disk,filename=disk.img \
      --nbd-server addr.type=unix,addr.path=nbd.sock \
      --export type=nbd,id=export,node-name=disk,writable=on


通过 Unix 域 socket ``vhost-user-blk.sock``，
将 qcow2 镜像 ``disk.qcow2`` 导出为 vhost-user-blk 设备：::

  $ qemu-storage-daemon \
      --blockdev driver=file,node-name=file,filename=disk.qcow2 \
      --blockdev driver=qcow2,node-name=qcow2,file=file \
      --export type=vhost-user-blk,id=export,addr.type=unix,addr.path=vhost-user-blk.sock,node-name=qcow2


通过 FUSE 将 ``disk.qcow2`` 导出到文件自身的位置，
使其显示为 raw 镜像：::

  $ qemu-storage-daemon \
      --blockdev driver=file,node-name=file,filename=disk.qcow2 \
      --blockdev driver=qcow2,node-name=qcow2,file=file \
      --export type=fuse,id=export,node-name=qcow2,mountpoint=disk.qcow2,writable=on


另请参阅
--------

:manpage:`qemu(1)`, :manpage:`qemu-block-drivers(7)`, :manpage:`qemu-storage-daemon-qmp-ref(7)`
