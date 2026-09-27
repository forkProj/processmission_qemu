.. _qemu-nbd:

=========================
QEMU 磁盘网络块设备服务器
=========================

用法概要
--------

**qemu-nbd** [*OPTION*]... *filename*

**qemu-nbd** -L [*OPTION*]...

**qemu-nbd** -d *dev*

说明
--------

通过 NBD 协议导出 QEMU 磁盘镜像。

其他用途：

- 在 Linux 上，将 /dev/nbdX 块设备绑定到 QEMU 服务器。
- 作为客户端查询远程 NBD 服务器提供的导出。

选项
--------

.. program:: qemu-nbd

*filename* 是磁盘镜像文件名；指定 :option:`--image-opts` 时，则表示一组块驱动选项。

*dev* 是 NBD 设备。

.. option:: --object type,id=ID,...

  创建 *type* 对象类的新实例，以 *ID* 标识。
  支持的属性详见 :manpage:`qemu(1)`。
  常用对象类型包括提供密码或加密密钥的 ``secret``，
  以及为 ``qemu-nbd`` 服务器或客户端提供 TLS 凭据的 ``tls-creds``。

.. option:: -p, --port=PORT

  服务器监听或客户端连接的 TCP 端口，默认为 ``10809``。

.. option:: -o, --offset=OFFSET

  镜像内部的偏移量。

.. option:: -b, --bind=IFACE

  服务器绑定或客户端连接的接口地址，默认为 ``0.0.0.0``。

.. option:: -k, --socket=PATH

  使用路径为 *PATH* 的 Unix socket。

.. option:: --image-opts

  将 *filename* 解释为一组镜像选项，而非普通文件名。
  使用此选项时，不应指定 ``-f``，而应设置 :option:`format=`。

.. option:: -f, --format=FMT

  强制使用格式 *FMT* 的块驱动，不进行自动检测。

.. option:: -r, --read-only

  以只读方式导出磁盘。

.. option:: -A, --allocation-depth

  通过 NBD_OPT_SET_META_CONTEXT 可访问的 ``qemu:allocation-depth``
  元数据上下文提供分配深度信息。

.. option:: -B, --bitmap=NAME

  如果 *filename* 包含名为 *NAME* 的 qcow2 持久位图，
  则通过 NBD_OPT_SET_META_CONTEXT 可访问的 ``qemu:dirty-bitmap:NAME``
  元数据上下文提供该位图。

.. option:: -s, --snapshot

  使用 *filename* 作为外部快照的后备文件，
  创建设置为 ``backing_file=``\ *filename* 的临时文件，将写入重定向到临时文件。

.. option:: -l, --load-snapshot=SNAPSHOT_PARAM

  加载 *filename* 中的内部快照，并以只读设备导出。
  SNAPSHOT_PARAM 格式为 ``snapshot.id=[ID],snapshot.name=[NAME]`` 或 ``[ID_OR_NAME]``。

.. option:: --cache=CACHE

  文件缓存模式。有效值为 ``none``、``writeback`` （默认）、``writethrough``、
  ``directsync`` 和 ``unsafe``。详见模拟器 ``-drive cache=...`` 的说明。

.. option:: -n, --nocache

  等同于 :option:`--cache=none`。

.. option:: --aio=AIO

  设置异步 I/O 模式，可选 ``threads`` （默认）、``native`` （仅 Linux）
  和 ``io_uring`` （Linux 5.1 及以上）。

.. option:: --discard=DISCARD

  控制是忽略 ``discard`` 请求（也称 trim 或 unmap），还是将其传给文件系统。
  *DISCARD* 可取 ``ignore`` （或 ``off``）、``unmap`` （或 ``on``），默认为 ``ignore``。

.. option:: --detect-zeroes=DETECT_ZEROES

  控制是否将操作系统的普通零写入自动转换为驱动专用的优化命令。
  *DETECT_ZEROES* 可取 ``off``、``on`` 或 ``unmap``，默认为 ``off``。
  ``unmap`` 将零写入转换为取消映射操作，仅当 *DISCARD* 也为 ``unmap`` 时可用。

.. option:: -c, --connect=DEV

  将 *filename* 连接到 NBD 设备 *DEV*，仅适用于 Linux。

.. option:: -d, --disconnect

  断开设备 *DEV*，仅适用于 Linux。

.. option:: -e, --shared=NUM

  允许最多 *NUM* 个客户端共享设备，默认为 ``1``，0 表示不限制。

.. option:: -t, --persistent

  最后一个连接断开后仍不退出。

.. option:: -x, --export-name=NAME

  设置 NBD 卷导出名称，默认为空字符串。

.. option:: -D, --description=DESCRIPTION

  设置便于阅读的 NBD 卷导出描述。

.. option:: --handshake-limit=N

  设置客户端完成握手的超时时间，单位为秒，默认 10；0 表示不限制。

.. option:: -L, --list

  以客户端身份连接远程 NBD 服务器，列出其全部导出的详细信息。
  此选项启用列表模式，与 :option:`--export-name`、:option:`--offset`
  等改变特定导出行为的选项不兼容。

.. option:: --tls-creds=ID

  指定此前通过 :option:`--object` 创建的 TLS 凭据对象 ID，
  为服务器启用强制 TLS 加密；在列表模式下，则为客户端连接提供所需凭据。

.. option:: --tls-hostname=hostname

  验证 TLS 连接收到的 x509 证书时，会将客户端连接所用的主机名与服务器证书比较。
  有时需要覆盖用于检查的名称。例如，通过 localhost 隧道连接远程服务器时，
  应使用此选项指定远程 NBD 服务器实际预期的主机名。
  经 Unix socket 访问 NBD、没有固有主机名时，也可以使用。
  仅允许在通过 :option:`--list` 充当 NBD 客户端时指定。

.. option:: --fork

  派生服务器进程，服务器开始运行后退出父进程。

.. option:: --pid-file=PATH

  将服务器进程 ID 写入指定文件。

.. option:: --tls-authz=ID

  指定此前通过 :option:`--object` 创建的 qauthz 对象 ID，
  根据连接用户的 x509 可分辨名称进行授权。

.. option:: -v, --verbose

  显示额外调试信息。
  如果因 :option:`--fork` 或 :option:`-c` 等选项而进入守护进程模式，
  此选项还会保持原始 *STDERR* 流打开。

.. option:: -h, --help

  显示帮助并退出。

.. option:: -V, --version

  显示版本信息并退出。

.. option:: -T, --trace [[enable=]PATTERN][,events=FILE][,file=FILE]

  .. include:: ../qemu-option-trace.rst.inc

示例
--------

启动监听 10809 端口的服务器，只导出 qcow2 文件中客户机可见的内容，
不启用 TLS，使用默认的空导出名。
此命令为一次性运行，会阻塞到首个成功连接的客户端断开：

::

  qemu-nbd -f qcow2 file.qcow2


启动长期运行的服务器，在 10810 端口启用加密，
允许持有特定 X.509 证书的客户端访问 raw 文件中 1 MB 的区域，导出名为 subset：

::

  qemu-nbd \
    --object tls-creds-x509,id=tls0,endpoint=server,dir=/path/to/qemutls \
    --object 'authz-simple,id=auth0,identity=CN=laptop.example.com,,\
              O=Example Org,,L=London,,ST=London,,C=GB' \
    --tls-creds tls0 --tls-authz auth0 \
    -t -x subset -p 10810 \
    --image-opts driver=raw,offset=1M,size=1M,file.driver=file,file.filename=file.raw


通过 Unix socket 提供客户机镜像的只读副本，允许最多 5 个并发读取者，
并派生持久运行的守护进程：

::

  qemu-nbd --fork --persistent --shared=5 --socket=/path/to/sock \
    --read-only --format=qcow2 file.qcow2


通过 /dev/nbd0 提供 qcow2 镜像中客户机可见的内容，
必要时还会为其中的分区创建 /dev/nbd0p1 等设备，完成后断开。
将 ``qemu-nbd`` 绑定到 /dev/nbd 设备通常需要 root 权限，
也可能需要运行 ``modprobe nbd`` 启用内核 NBD 客户端模块。
*注意：* 不要用此方法挂载不可信客户机镜像中的文件系统。
恶意客户机可能专门构造镜像，以触发内核在分区探测或文件系统挂载时的漏洞。

::

  qemu-nbd -c /dev/nbd0 -f qcow2 file.qcow2
  qemu-nbd -d /dev/nbd0


查询远程服务器在 10809 端口提供的导出详情，并通过 PSK 认证：

::

  qemu-nbd \
    --object tls-creds-psk,id=tls0,dir=/tmp/keys,username=eblake,endpoint=client \
    --tls-creds tls0 -L -b remote.example.com


另请参阅
--------

:manpage:`qemu(1)`, :manpage:`qemu-img(1)`
