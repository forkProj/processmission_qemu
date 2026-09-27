.. _Network_Emulation:

网络模拟
-----------------

QEMU 可模拟多种网卡，例如 PC 的 PCI 或 ISA 卡，并连接到主机网络后端或
模拟集线器。后端可通过 TAP、非特权用户网络栈连接真实网络，也可通过套接字
连接另一个 QEMU 进程中的客户机。

使用 TAP 接口
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

这是连接真实网络的标准方式。QEMU 在主机添加 tapN 虚拟设备，可像真实
以太网卡一样配置。

Linux 主机
^^^^^^^^^^

发行版通常提供辅助脚本，默认 /etc/qemu-ifup 和 /etc/qemu-ifdown，QEMU
改变网络状态时按需调用。

非特权运行时，可能需正确配置 sudo，使脚本中的网络命令能以 root 执行。

确认主机内核支持 TAP，即 /dev/net/tun 存在。

命令行示例见 :ref:`sec_005finvocation`。

Windows 主机
^^^^^^^^^^^^

Windows 2000/XP 的虚拟以太网驱动名为 TAP-Win32，不包含在标准 QEMU
Windows 版本中，需要单独获取。它属于 OpenVPN，可从 https://openvpn.net/
下载。

使用用户模式网络栈
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

-net user 使用完全用户空间网络，不需 root；未指定 -net 时默认采用。
虚拟网络如下::

        guest (10.0.2.15)  <------>  Firewall/DHCP server <-----> Internet
                              |          (10.0.2.2)
                              |
                              ---->  DNS server (10.0.2.3)
                              |
                              ---->  SMB server (10.0.2.4)


虚拟机相当于位于阻止所有传入连接的防火墙后。可以通过 DHCP 自动配置，
地址从 10.0.2.15 开始分配。

检查网络时，可 ping 10.0.2.2，并确认获得 10.0.2.x 地址。

一般 ICMP 不工作，但 ping 本地路由器 10.0.2.2 应可用。Linux 3.0 及以后
可使用非特权 ICMP ping 套接字访问互联网，管理员需设置 ping_group_range。
例如允许 GID 100（通常是 users）::

   echo 100 100 > /proc/sys/net/ipv4/ping_group_range


使用内置 TFTP 时，路由器也是 TFTP 服务器。

-netdev user,hostfwd=... 可将主机 TCP、UDP 或 Unix 连接重定向到客户机，
例如 X11、telnet 或 SSH。

使用 passt 用户模式网络栈
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

passt_ 可直接替代 SLIRP（-net user），不需要任何特权或能力。它性能更好，
完整支持 IPv6，且作为 QEMU 外部守护进程，安全性更好。

可像 user 后端一样使用 -net passt、-netdev passt、-nic passt，也可手动
启动，再通过 -netdev stream 套接字或 -netdev vhost-user 连接。

后两种方式可启用 passt 后端接口尚未提供的功能，例如迁移。

详情见 `passt(1)`_。

.. _passt: https://passt.top/
.. _passt(1): https://passt.top/builds/latest/web/passt.1.html

使用 passt 后端接口
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

无需自行启动守护进程，QEMU 会代劳，默认使用套接字模式。

.. parsed-literal::
   |qemu_system| [...OPTIONS...] -nic passt

   (qemu) info network
   e1000e.0: index=0,type=nic,model=e1000e,macaddr=52:54:00:12:34:56
    \ #net071: index=0,type=passt,stream,connected to pid 24846


.. parsed-literal::
   |qemu_system| [...OPTIONS...] -net nic -net passt,tcp-ports=10001,udp-ports=10001

   (qemu) info network
   hub 0
    \ hub0port1: #net136: index=0,type=passt,stream,connected to pid 25204
    \ hub0port0: e1000e.0: index=0,type=nic,model=e1000e,macaddr=52:54:00:12:34:56


.. parsed-literal::
   |qemu_system| [...OPTIONS...] -netdev passt,id=netdev0 -device virtio-net,mac=9a:2b:2c:2d:2e:2f,id=virtio0,netdev=netdev0

   (qemu) info network
   virtio0: index=0,type=nic,model=virtio-net-pci,macaddr=9a:2b:2c:2d:2e:2f
    \ netdev0: index=0,type=passt,stream,connected to pid 25428


使用 vhost 接口时，添加 vhost-user=on 并选择 virtio-net：

.. parsed-literal::
   |qemu_system| [...OPTIONS...] -nic passt,model=virtio,vhost-user=on

   (qemu) info network
   virtio-net-pci.0: index=0,type=nic,model=virtio-net-pci,macaddr=52:54:00:12:34:56
    \ #net006: index=0,type=passt,vhost-user,connected to pid 25731


使用套接字 passt 接口
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

启动守护进程::

   passt --socket ~/passt.socket


未提供 --socket 时，会输出可连接路径，如 /tmp/passt_1.socket、
/tmp/passt_2.socket。随后连接 QEMU：

.. parsed-literal::
   |qemu_system| [...OPTIONS...] -device virtio-net-pci,netdev=netdev0 -netdev stream,id=netdev0,server=off,addr.type=unix,addr.path=~/passt.socket


~/passt.socket 是 passt 创建的通信套接字。

使用 vhost 接口
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

以 --vhost-user 启动::

   passt --vhost-user --socket ~/passt.socket


然后连接 QEMU：

.. parsed-literal::
   |qemu_system| [...OPTIONS...] -m $RAMSIZE -chardev socket,id=chr0,path=~/passt.socket -netdev vhost-user,id=netdev0,chardev=chr0 -device virtio-net,netdev=netdev0 -object memory-backend-memfd,id=memfd0,share=on,size=$RAMSIZE -numa node,memdev=memfd0


$RAMSIZE 为虚拟机内存，-m 必须与 memory-backend-memfd 的 size 匹配。

迁移 passt
^^^^^^^^^^^^^^^^^^^

使用 vhost-user 时，passt 可随 QEMU 迁移而不中断网络连接。

它无特权运行，因此依赖 passt-repair 通过 TCP_REPAIR 保存和加载 TCP 状态。
辅助程序需要 CAP_NET_ADMIN 或 root。缺少它时，TCP 连接不会保留。

同一主机迁移客户机示例
________________________________________________

运行前，以 root 为 passt-repair 设置能力::

   setcap cap_net_admin+eip ./passt-repair


启动源端 passt::

   passt --vhost-user --socket ~/passt_src.socket --repair-path ~/passt-repair_src.socket


~/passt-repair_src.socket 用于与辅助程序通信，默认路径为 --socket 后加 .repair。

启动 passt-repair::

   passt-repair ~/passt-repair_src.socket


启动源端 QEMU，并提供监视器以发送迁移命令：

.. parsed-literal::
   |qemu_system| [...OPTIONS...] [...VHOST USER OPTIONS...] -monitor stdio


启动目的端 passt::

   passt --vhost-user --socket ~/passt_dst.socket --repair-path ~/passt-repair_dst.socket


启动目的端 passt-repair::

   passt-repair ~/passt-repair_dst.socket


启动带 -incoming 的 QEMU：

.. parsed-literal::
   |qemu_system| [...OPTIONS...] [...VHOST USER OPTIONS...] -incoming tcp:localhost:4444


在源端监视器启动迁移::

   (qemu) migrate tcp:localhost:4444


每次迁移都需要独立 passt-repair。迁移失败后，重试前也需重新启动它。

集线器
~~~~~~

QEMU 可模拟多个集线器，作为网卡或 TAP 等设备间的虚拟连接。通过
-netdev hubport 或 -nic hubport 连接客户机网卡或主机后端。旧式 -net
也连接到默认 ID 0 集线器，除非用 -net nic,netdev=xxx 指定后端。

连接多个 QEMU 实例的模拟网络
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

-netdev socket、-nic socket 或 -net socket 可建立跨多个实例的网络。
基本示例见 :ref:`sec_005finvocation` 中 -netdev socket 的说明。
