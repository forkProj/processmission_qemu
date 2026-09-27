==================
头部分离的 LUKS 卷
==================

简介
========

本文概述 LUKS 头部分离卷的设计和用法。

背景
========

LUKS 格式允许将头部与载荷存放在不同卷中，
可扩展 QEMU LUKS 驱动以支持这种场景。

普通 LUKS 卷布局如下：

::

         +-----------------------------------------------+
         |         |                |                    |
 磁盘    | 头部    |  密钥材料      |  磁盘载荷数据      |
         |         |                |                    |
         +-----------------------------------------------+

使用分离头部时，需要两个磁盘：

::

         +--------------------------+
 磁盘1   |   头部    | 密钥材料     |
         +--------------------------+
         +---------------------+
 磁盘2   |  磁盘载荷数据       |
         +---------------------+

这样做有多种好处：

 * 隐蔽性：disk2 没有头部，无法被识别为 LUKS 卷。
 * 控制：限制 disk1 的访问后，即使取得 disk2 也不能解锁。
   例如磁盘位于 NFS，但希望限制哪些主机可启动虚拟机，
   就可以动态向指定主机提供头部访问权限。
 * 灵活性：应用数据卷大小已确定，不便为加密而调整容量时，
   可独立保存头部，继续以原卷存放载荷。
 * 恢复：头部一个比特损坏就可能使全部载荷不可访问。
   可以方便地备份头部；主头部损坏时，指向分离头部备份仍可解锁数据。

架构
========

以 qcow2 加密为例，分离头部 LUKS 卷的架构如下图。

根节点有 file 和 header 两个子节点。
磁盘载荷保存在 file 节点，LUKS 头部和密钥材料位于 header 节点。

::

                       +-----------------------------+
  根节点               |          foo[luks]          |
                       +-----------------------------+
                          |                       |
                     file |                header |
                          |                       |
               +---------------------+    +------------------+
  子节点       |payload-format[qcow2]|    |header-format[raw]|
               +---------------------+    +------------------+
                          |                       |
                     file |                 file  |
                          |                       |
               +----------------------+  +---------------------+
  子节点       |payload-protocol[file]|  |header-protocol[file]|
               +----------------------+  +---------------------+
                          |                       |
                          |                       |
                          |                       |
                     主机存储                主机存储

用法
========

使用 qemu-img 创建头部分离的 LUKS 磁盘
--------------------------------------

Shell 命令行::

  # qemu-img create --object secret,id=sec0,data=abc123 -f luks \
    -o cipher-alg=aes-256,cipher-mode=xts -o key-secret=sec0 \
    -o detached-header=true test-header.img
  # qemu-img create -f qcow2 test-payload.qcow2 200G
  # qemu-img info 'json:{"driver":"luks","file":{"filename": \
    "test-payload.img"},"header":{"filename":"test-header.img"}}'

设置虚拟机的头部分离 LUKS 卷
----------------------------

QEMU 命令行::

  # qemu-system-x86_64 ... \
    -object '{"qom-type":"secret","id":"libvirt-3-format-secret", \
    "data":"abc123"}' \
    -blockdev '{"driver":"file","filename":"/path/to/test-header.img", \
    "node-name":"libvirt-1-storage"}' \
    -blockdev '{"node-name":"libvirt-1-format","read-only":false, \
    "driver":"raw","file":"libvirt-1-storage"}' \
    -blockdev '{"driver":"file","filename":"/path/to/test-payload.qcow2", \
    "node-name":"libvirt-2-storage"}' \
    -blockdev '{"node-name":"libvirt-2-format","read-only":false, \
    "driver":"qcow2","file":"libvirt-2-storage"}' \
    -blockdev '{"node-name":"libvirt-3-format","driver":"luks", \
    "file":"libvirt-2-format","header":"libvirt-1-format","key-secret": \
    "libvirt-3-format-secret"}' \
    -device '{"driver":"virtio-blk-pci","bus":XXX,"addr":YYY,"drive": \
    "libvirt-3-format","id":"virtio-disk1"}'

向虚拟机添加头部分离的 LUKS 卷
------------------------------

1. 用 object-add 添加解密上述 LUKS 头部密文所需的秘密对象::

    # virsh qemu-monitor-command vm '{"execute":"object-add", \
      "arguments":{"qom-type":"secret", "id": \
      "libvirt-4-format-secret", "data":"abc123"}}'

2. 添加 LUKS 头部的协议节点::

    # virsh qemu-monitor-command vm '{"execute":"blockdev-add", \
      "arguments":{"node-name":"libvirt-1-storage", "driver":"file", \
      "filename": "/path/to/test-header.img" }}'

3. 添加 LUKS 头部的 raw 驱动节点::

    # virsh qemu-monitor-command vm '{"execute":"blockdev-add", \
      "arguments":{"node-name":"libvirt-1-format", "driver":"raw", \
      "file":"libvirt-1-storage"}}'

4. 添加磁盘载荷镜像的协议节点::

    # virsh qemu-monitor-command vm '{"execute":"blockdev-add", \
      "arguments":{"node-name":"libvirt-2-storage", "driver":"file", \
      "filename":"/path/to/test-payload.qcow2"}}'

5. 添加磁盘载荷的 qcow2 格式节点::

    # virsh qemu-monitor-command vm '{"execute":"blockdev-add", \
      "arguments":{"node-name":"libvirt-2-format", "driver":"qcow2", \
      "file":"libvirt-2-storage"}}'

6. 添加 luks 格式节点，通过 header 字段将 qcow2 磁盘与 LUKS 头部关联::

    # virsh qemu-monitor-command vm '{"execute":"blockdev-add", \
      "arguments":{"node-name":"libvirt-3-format", "driver":"luks", \
      "file":"libvirt-2-format", "header":"libvirt-1-format", \
      "key-secret":"libvirt-2-format-secret"}}'

7. 最后热插入 virtio-blk 设备::

    # virsh qemu-monitor-command vm '{"execute":"device_add", \
      "arguments": {"driver":"virtio-blk-pci", \
      "drive": "libvirt-3-format", "id":"virtio-disk2"}}

待办
========

1. 支持虚拟机内共享分离的 LUKS 头部。
