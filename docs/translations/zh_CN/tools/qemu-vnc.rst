.. _qemu-vnc:

====================
QEMU 独立 VNC 服务器
====================

用法概要
--------

**qemu-vnc** [*OPTION*]...

说明
--------

``qemu-vnc`` 是独立的 VNC 服务器，通过 D-Bus 显示接口（:ref:`dbus-display`）
连接正在运行的 QEMU 实例。它通过 VNC 协议提供客户机显示、输入、音频、
剪贴板及串口控制台字符设备，使 VNC 客户端能够与虚拟机交互，
而无需 QEMU 自身绑定 VNC socket。

选项
--------

.. program:: qemu-vnc

.. option:: -h, --help

  显示帮助并退出。

.. option:: -V, --version

  打印版本信息并退出。

.. option:: -a ADDRESS, --dbus-address=ADDRESS

  要连接的 D-Bus 地址。未指定时，连接会话总线。

.. option:: -p FD, --dbus-p2p-fd=FD

  继承的 Unix socket 文件描述符，用于与 QEMU 建立点对点 D-Bus 连接。
  与 ``--dbus-address`` 和 ``--bus-name`` 互斥。

.. option:: -n NAME, --bus-name=NAME

  要连接的 QEMU 实例的 D-Bus 总线名称，默认为 ``org.qemu``。
  如果指定自定义 ``--dbus-address`` 而未给出总线名称，则使用点对点 D-Bus。

.. option:: -W, --wait

  等待 D-Bus 总线名称出现。仅适用于总线连接。

.. option:: --password

  要求客户端进行 VNC 密码认证。
  密码通过 D-Bus 的 ``SetPassword`` 方法在运行时设置，详见 :doc:`/interop/dbus-vnc`。
  设置密码之前，客户端无法连接。

  如果存在 systemd 凭据密码，此选项会被忽略，
  因为这种情况下已经通过 ``password-secret`` 启用密码认证。

.. option:: -l ADDR, --vnc-addr=ADDR

  VNC 监听地址，格式与 QEMU 的 ``-vnc`` 相同。
  默认为 ``localhost:0``，即 TCP 端口 5900。

.. option:: -w ADDR, --websocket=ADDR

  在指定地址启用 WebSocket 传输。
  *ADDR* 可以是端口号，也可以是 *address:port*。

.. option:: -O OBJDEF, --object=OBJDEF

  创建 QEMU 用户可创建对象。*OBJDEF* 使用与 ``-object`` 相同的 key=value 语法。
  可以重复指定，例如用于创建 ``--tls-authz`` 引用的授权对象。

.. option:: -t DIR, --tls-creds=DIR

  包含 TLS x509 凭据的目录，即 ``ca-cert.pem``、``server-cert.pem`` 和
  ``server-key.pem``。指定后，客户端必须通过 TLS 连接。

.. option:: --tls-authz=ID

  此前通过 ``--object`` 创建的 ``QAuthZ`` 对象 ID，用于 TLS 客户端证书授权。
  指定后，TLS 凭据采用 ``verify-peer=yes``，客户端必须提供有效证书。
  TLS 握手后，会使用授权对象检查客户端证书的可分辨名称（Distinguished Name）。
  此选项要求同时设置 ``--tls-creds``。

.. option:: --sasl

  要求客户端通过 SASL 向 VNC 服务器认证。
  具体认证方法由系统或用户针对 qemu 服务的 SASL 配置控制，
  通常位于 ``/etc/sasl2/qemu.conf``。
  以非特权用户运行 QEMU 时，可以用 ``SASL_CONF_PATH`` 环境变量指定其他配置位置。
  虽然部分 SASL 方法（例如 GSSAPI）也提供数据加密，
  仍建议始终配合 tls 和 x509 设置，启用 SSL 及服务器证书，
  通过加密数据保护认证凭据。详见系统模拟用户指南的 :ref:`VNC security`。

.. option:: --sasl-authz=ID

  此前通过 ``--object`` 创建的 ``QAuthZ`` 对象 ID，用于 SASL 用户名授权。
  SASL 认证成功后，会使用该对象检查已认证用户名；检查失败则断开客户端。
  此选项要求同时设置 ``--sasl``。

.. option:: -s POLICY, --share=POLICY

  设置显示共享策略。*POLICY* 为 ``allow-exclusive``、``force-shared`` 或 ``ignore``。

  ``allow-exclusive`` 允许客户端请求独占访问。
  按照 RFB 规范建议，通过断开其他连接实现。
  多个客户端同时连接时，所有客户端都必须请求共享会话，
  即 vncviewer 的 -shared 开关。这是默认策略。

  ``force-shared`` 禁止客户端独占，适合共享桌面，
  避免某人忘记指定 -shared 就断开其他所有人的连接。

  ``ignore`` 完全忽略共享标志，无条件允许所有人连接。
  这不符合 RFB 规范，但属于 QEMU 的传统行为。

.. option:: -C NAME, --vt-chardev=NAME

  要作为 VNC 文本控制台提供的字符设备类型名称。
  可以重复指定，以提供多个字符设备。
  未指定时使用 ``org.qemu.console.serial.0`` 和 ``org.qemu.monitor.hmp.0``。

.. option:: -N, --no-vt

  不将任何字符设备作为文本控制台提供。
  此选项覆盖默认字符设备列表及全部 ``--vt-chardev`` 选项。

.. option:: -k LAYOUT, --keyboard-layout=LAYOUT

  键盘布局，例如 ``en-us``，会传给 VNC 服务器进行键码转换。

.. option:: --lossy

  启用有损压缩，如 gradient、JPEG 等。
  根据客户端编码设置，客户端可能收到有损帧缓冲更新。
  可以显著节省带宽，但会牺牲画质。

.. option:: --non-adaptive

  禁用默认启用的自适应编码。
  自适应编码尝试检测频繁更新的屏幕区域，并使用 JPEG 等有损方式传输这些区域。
  这在播放视频时能有效节省带宽。
  禁用后，Tight 等编码恢复原本的静态行为。

.. option:: -T, --trace [[enable=]PATTERN][,events=FILE][,file=FILE]

  .. include:: ../qemu-option-trace.rst.inc

示例
--------

使用 D-Bus 显示后端启动 QEMU：::

    qemu-system-x86_64 -display dbus ...


然后连接 ``qemu-vnc``：::

    qemu-vnc


VNC 客户端现在可以连接 ``localhost:5900``。

在不同端口监听并启用 TLS：::

    qemu-vnc --vnc-addr localhost:1 --tls-creds /etc/pki/qemu-vnc


要求 TLS 并进行客户端证书授权：::

    qemu-vnc --object authz-list-file,id=auth0,filename=/etc/qemu/vnc.acl,refresh=on \
             --tls-creds /etc/pki/qemu-vnc --tls-authz auth0


启用 TLS 和 SASL 认证：::

    qemu-vnc --tls-creds /etc/pki/qemu-vnc --sasl


VNC 密码认证
------------

有两种方式启用 VNC 密码认证：

1. 使用 ``--password`` 启动 ``qemu-vnc``，
   再通过 D-Bus 的 ``SetPassword`` 在运行时设置密码。
   设置之前，客户端连接会被拒绝。

2. 使用 systemd 凭据。如果设置了 ``CREDENTIALS_DIRECTORY`` 环境变量
   （参见 :manpage:`systemd.exec(5)`），且该目录中有 ``vnc-password`` 文件，
   VNC 服务器会自动将其内容用作密码，此时不需要 ``--password``。

D-Bus 接口
----------

``qemu-vnc`` 提供 D-Bus 接口，用于管理和监控 VNC 连接。
完整参考见 :doc:`/interop/dbus-vnc`。

另请参阅
--------

:manpage:`qemu(1)`,
:doc:`/interop/dbus-display`,
:doc:`/interop/dbus-vnc`,
`RFB 协议 <https://github.com/rfbproto/rfbproto>`_
