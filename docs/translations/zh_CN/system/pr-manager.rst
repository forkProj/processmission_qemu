===============================
持久预留管理器
===============================

SCSI 持久预留可在共享存储中将块设备访问限制为指定发起方。虚拟机集群通常
需要发送持久预留 SCSI 命令，但操作系统限制非特权程序发送它们，因为误用
可能干扰存储网络的正常运行。

因此，QEMU 的 SCSI 直通设备 ``scsi-block`` 和 ``scsi-generic`` （均仅支持
Linux）可将持久预留委托给独立的“持久预留管理器”对象。仅 PERSISTENT RESERVE
OUT 和 PERSISTENT RESERVE IN 交给它，其他命令仍由 QEMU 正常处理。

-----------------------------------------
定义持久预留管理器
-----------------------------------------

管理器是 QOM 类 pr-manager 的某个子类实例。

目前只有 ``pr-manager-helper``，通过 Unix 套接字将命令转发到外部特权
辅助程序。辅助程序只允许对 QEMU 已持有文件描述符的设备发送预留命令，
因此 QEMU 必须同时有权访问套接字和设备，才能操作持久预留。

该对象只有一个字符串属性 ``path``，指定辅助程序 Unix 套接字路径。以下
定义对象并将其连接到 SCSI 直通设备::

      $ qemu-system-x86_64
          -device virtio-scsi \
          -object pr-manager-helper,id=helper0,path=/var/run/qemu-pr-helper.sock
          -drive if=none,id=hd,driver=raw,file.filename=/dev/sdb,file.pr-manager=helper0
          -device scsi-block,drive=hd

也可使用 ``-blockdev``::

      $ qemu-system-x86_64
          -device virtio-scsi \
          -object pr-manager-helper,id=helper0,path=/var/run/qemu-pr-helper.sock
          -blockdev node-name=hd,driver=raw,file.driver=host_device,file.filename=/dev/sdb,file.pr-manager=helper0
          -device scsi-block,drive=hd

还需确保 :command:`qemu-pr-helper` 正在运行，且使用与 QEMU 命令行相同
的套接字文件名。详情见该工具的文档或手册页。

---------------------------------------------
多路径设备与持久预留
---------------------------------------------

正确支持多路径持久预留，需要与多路径守护进程通信，使新发现或重新上线的
路径能登记并应用预留。如果构建时系统有 libmpathpersist，qemu-pr-helper
即可提供此功能。

截至 2017 年 8 月，必须在 multipath.conf 指定预留密钥，multipathd 才会
检查新发现或恢复路径的持久预留。属性可放在 defaults 或 multipaths，例如::

    multipaths {
        multipath {
            wwid   XXXXXXXXXXXXXXXX
            alias      yellow
            reservation_key  0x123abc
        }
    }

将 :program:`qemu-pr-helper` 链接到 libmpathpersist，不影响它处理普通 SCSI
设备。
