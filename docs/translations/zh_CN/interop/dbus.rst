========================
D-Bus
========================

简介
========================================

QEMU 可能同时使用多种辅助进程：

- vhost-user*，如 GPU、virtfs、输入等。
- TPM 或其他设备仿真。
- 用户网络 slirp。
- DHCP/DNS、samba/ftp 等网络服务。
- 压缩、流式传输等后台任务。
- 客户端界面。
- 管理与命令行工具。

多进程可采用更严格的安全规则，也提高模块化程度。

QEMU 自身主要使用 QMP 进行进程间通信，并用 Spice/VNC 远程显示，
而 D-Bus 是 Unix 系统事实上的常用 IPC。
其线路格式适合机器处理，有多语言绑定和多种工具。

总线让辅助进程无需经 QEMU 中转即可相互发现和通信，
拓扑也比网状连接更易理解和调试，但仍须考虑安全。

安全
========================================

QEMU 的 D-Bus 总线应为单个虚拟机私有，
这样同一总线上仅运行共同服务于该虚拟机的协作任务。

D-Bus 协议和标准本身不提供连接建立后强制隔离对等端的机制。
各端可以另外实施基于 UNIX 凭据等的安全规则。

守护进程可按多种元数据属性控制谁能收发消息，但单靠这一点通常不足以保证安全。
D-Bus 方法本身的语义同样关键，各端必须仔细验证来自不同信任级别对等端的信息。

dbus-daemon 策略
----------------------------------------

dbus-daemon 可根据连接进程的 UID/GID 实施策略。
因此建议辅助程序使用不同于 QEMU 的 UID，并设置相应规则。

可按使用场景选择：

- 全部使用相同 UID。

  - 便于开发。
  - 可靠性更好，一部分崩溃不会导致整个虚拟机退出。
  - 除非另加 SELinux 或 AppArmor 等控制，否则不比传统 QEMU 更安全。

- 两个 UID：QEMU 一个，D-Bus 与辅助程序共用另一个。

  - 基于用户的隔离得到适度改善。

- 多个 UID：QEMU、D-Bus 和每个辅助程序各用一个。

  - 基于用户的隔离最强。
  - 管理每个虚拟机所需的独立 UID 更复杂。

例如，只允许 qemu 用户与 qemu-helper 的 org.qemu.Helper1 服务通信，策略可包含：

.. code:: xml

  <policy user="qemu">
     <allow send_destination="org.qemu.Helper1"/>
     <allow receive_sender="org.qemu.Helper1"/>
  </policy>

  <policy user="qemu-helper">
     <allow own="org.qemu.Helper1"/>
  </policy>


dbus-daemon 也能根据来源和目标安全上下文执行 SELinux 检查。
例如，允许 virtiofs_t 向 svirt_t 发消息，却不允许 virtiofs_t 向 virtiofs_t 发消息。

详情见 dbus-daemon 手册页。

指南
========================================

实现新接口时，建议遵循 D-Bus API 设计指南：
https://dbus.freedesktop.org/doc/dbus-api-design.html

org.qemu.* 前缀保留给 QEMU 项目实现和分发的服务。

QEMU 接口
========================================

:doc:`dbus-vmstate`

:doc:`dbus-display`
