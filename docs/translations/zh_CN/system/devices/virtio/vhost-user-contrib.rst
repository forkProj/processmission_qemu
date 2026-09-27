contrib 中的 vhost-user 守护进程
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

QEMU 的 contrib 提供若干 :ref:`vhost_user` 守护进程，许多在此功能最初
加入时编写。应考虑 rust-vmm 的 `vhost-device repository`_ 等其他实现
是否更适合生产。

.. _vhost-device repository: https://github.com/rust-vmm/vhost-device

.. _vhost_user_block:

vhost-user-block：块设备
===============================

此后端将普通文件或块设备作为简单块设备暴露给客户机。通常更应选择
:ref:`storage-daemon`，它支持多种存储模式及包括 vhost-user 的多个接口。

.. _vhost_user_gpu:

vhost-user-gpu：GPU 设备
===========================

提供半虚拟化 GPU 和显示控制器。若需要最新功能，通常应选内部
:ref:`virtio_gpu`，也可使用 rust-vmm 的 `vhost_device_gpu`_。

.. _vhost_device_gpu: https://github.com/rust-vmm/vhost-device/tree/main/vhost-device-gpu

.. _vhost_user_input:

vhost-user-input：输入模拟
==================================

Virtio input 是用于输入事件的半虚拟化设备。

说明
-----------

该实现与轮询输入设备的守护进程配合，将事件传给客户机。

QEMU 后端位于 contrib/vhost-user-input。

Linux 内核支持
--------------------

客户机内核需要启用 CONFIG_VIRTIO_INPUT。

示例
--------

先启动后端：

::

  host# vhost-user-input --socket-path=input.sock	\
      --evdev-path=/dev/input/event17


QEMU 需要 chardev 套接字通信，并通过
:ref:`共享内存 <shared_memory_object>` 访问 VirtIO 队列。

::

  host# qemu-system								\
      -chardev socket,path=/tmp/input.sock,id=mouse0				\
      -device vhost-user-input-pci,chardev=mouse0				\
      -m 4096 									\
      -object memory-backend-file,id=mem,size=4G,mem-path=/dev/shm,share=on	\
      -numa node,memdev=mem							\
      ...



.. _vhost_user_scsi:

vhost-user-scsi：SCSI 控制器
=================================

将 iSCSI 设备代理到虚拟 SCSI 控制器。

.. _vhost_user_bridge:

vhost-user-bridge：网络桥
==================================

提供 vhost-user 网络后端，即服务器，用作测试真实互联网流量的开发工具。

示例
-------
单个 QEMU 同时运行 slirp 用户网络栈和 vhost-user 客户端时：

先启动桥：

::

  $ vhost-user-bridge -u /tmp/vubr.sock \
                      -l 127.0.0.1:4444 \
                      -r 127.0.0.1:5555


再启动 QEMU：

::

  $ qemu-system-x86_64 \
        -m 4G \
        -object memory-backend-memfd,id=mem0,size=4G,share=on,prealloc=on \
        -numa node,memdev=mem0 \
        -chardev socket,id=char0,path=/tmp/vubr.sock \
        -netdev vhost-user,id=vhost0,chardev=char0,vhostforce=on \
        -device virtio-net-pci,netdev=vhost0 \
        -netdev socket,id=udp0,udp=localhost:4444,localaddr=localhost:5555 \
        -netdev user,id=user0 \
        -netdev hubport,id=hub0,hubid=0,netdev=udp0 \
        -netdev hubport,id=hub1,hubid=0,netdev=user0 \
        ...
