.. _Multi-process QEMU:

多进程 QEMU
==================

本文介绍多进程 QEMU 的配置和使用。设计文档见 docs/devel/multi-process.rst。

1）配置
----------------

启用 KVM 的目标默认启用多进程支持。


2）使用
--------

多进程 QEMU 需要编排程序启动。

启动 mpqemu 的命令行工作方式如下。

* 编排程序：

  - 创建 Unix 套接字对。

  - 启动远程进程，通过命令行传入其中一个套接字。

  - 再启动 QEMU，将另一套接字作为代理设备对象的选项。

* 远程进程：

  - QEMU 通过 remote 机器选项进入远程进程模式。

  - 编排程序创建 remote-object，提供设备详情及设备文件描述符。

  - 其他选项与正常启动带设备的 QEMU 无异。

  - 远程进程命令行示例：

      /usr/bin/qemu-system-x86_64                                        \
      -machine x-remote                                                  \
      -device lsi53c895a,id=lsi0                                         \
      -drive id=drive_image2,file=/build/ol7-nvme-test-1.qcow2           \
      -device scsi-hd,id=drive2,drive=drive_image2,bus=lsi0.0,scsi-id=0  \
      -object x-remote-object,id=robj1,devid=lsi0,fd=4,

* QEMU:

  - 部分 RAM 在 QEMU 与远程进程间共享，因此需要 memory-backend-memfd：

    -object memory-backend-memfd,id=mem,size=2G

  - 远程进程中模拟的每个 PCI 设备，都创建一个 x-pci-proxy-dev。socket
    子选项指定编排程序创建通道的另一端。必须设置 id，且与远程 PCI 设备
    的 id 相同。

  - QEMU 命令行示例：

      -device x-pci-proxy-dev,id=lsi0,socket=3
