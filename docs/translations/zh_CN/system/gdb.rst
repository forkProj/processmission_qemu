.. _GDB usage:

GDB 用法
---------

QEMU 通过 GDB 远程连接接口 gdbstub 支持调试客户机，类似真实硬件的 JTAG。
可以启停虚拟机、检查寄存器和内存、设置断点和观察点。

启动时添加 -s 和 -S：前者监听 TCP 1234，后者等待 GDB 指令才开始运行。
要指定其他端口或传输方式，使用 -gdb dev 替代 -s，见 `使用 Unix 套接字`_。

.. parsed-literal::

   |qemu_system| -s -S -kernel bzImage -drive file=rootdisk.img,format=raw -append "root=/dev/sda"


QEMU 启动后静默等待连接。

对 vmlinux 启动 GDB::

   > gdb vmlinux


连接 QEMU::

   (gdb) target remote localhost:1234


随后正常使用，例如输入 c 启动内核::

   (gdb) c


系统代码调试技巧：

1. info reg 显示全部 CPU 寄存器。
2. x/10i $eip 显示 PC 处的代码。
3. set architecture i8086 用于显示 16 位代码，再以 x/10i $cs*16+$eip
   查看 PC 位置。

断点与观察点支持
=================================

GDB 始终可以尝试向可写内存插入断点，但其他功能取决于加速器。TCG 系统模拟
声明支持无限硬件断点和观察点；其他加速器取决于是否实现，见 AccelGdbConfig。

TCG 用户模式无法跟踪全部内存访问，因此不支持观察点。

代码重定位
===============

现代内核的 ASLR 等功能可能重定位代码，造成调试混淆。需要更新 GDB 对内存
位置的认识，或更简单地在启动时禁用 ASLR。

在系统模拟中调试用户空间
========================================

技术上可以调试系统镜像中的用户程序，但内核抢占和用户/内核模式切换会使
跟踪困难。除非专门调查两者交互，否则更适合在客户机内运行 GDB，或通过
端口暴露 gdbserver。

调试多核机器
============================

GDB 使用两层结构表示并行执行：多个 inferior，每个包含多个线程。QEMU 将
每个 CPU 集群暴露为一个 inferior，集群内各 CPU 为线程。多数机器 CPU 相同，
只有一个集群；异构机器可能多个。例如 sifive_u 的第一集群只有一个 E51，
第二集群有四个 U54，分别映射到第一个 inferior 的唯一线程和第二个的四线程。

连接时自动关联第一个 inferior，可用 info thread 查看该集群 CPU，并用
普通线程命令切换。

多集群需要显式连接其他 inferior。首先使用 extended-remote，而非 remote::

    (gdb) target extended-remote localhost:1234


连接后只有第一集群，需为其他集群创建并附加 inferior::

  (gdb) add-inferior
  Added inferior 2
  (gdb) inferior 2
  [Switching to inferior 2 [<null>] (<noexec>)]
  (gdb) attach 2
  Attaching to process 2
  warning: No executable has been specified and target does not support
  determining executable automatically.  Try using the "file" command.
  0x00000000 in ?? ()


完成后，info threads 显示全部已附加集群::

  (gdb) info threads
    Id   Target Id         Frame
    1.1  Thread 1.1 (cortex-m33-arm-cpu cpu [running]) 0x00000000 in ?? ()
  * 2.1  Thread 2.2 (cortex-m33-arm-cpu cpu [halted ]) 0x00000000 in ?? ()


通常还应启用 schedule-multiple，使 continue 恢复所有 CPU，而不只是当前集群::

  (gdb) set schedule-multiple on


使用 Unix 套接字
==================

操作系统支持时，可用 Unix 套接字替代 TCP。适合并行测试或无法预知空闲端口
的自动化测试。

先创建 chardev，再让 gdbserver 使用它：

.. parsed-literal::

   |qemu_system| -chardev socket,path=/tmp/gdb-socket,server=on,wait=off,id=gdb0 -gdb chardev:gdb0 -S ...


照常启动 GDB，但通过套接字路径连接::

   (gdb) target remote /tmp/gdb-socket


需要 GDB 9.0 或更新版本。

高级调试选项
==========================

修改单步行为
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

默认单步禁用 IRQ 和定时器服务，因为 GDB 预期越过当前指令。若启用中断，
单步可能跳到中断或异常向量，而不是执行当前指令，导致在真正执行之前多次
命中同一断点。少数场景需要单步进入中断，因此提供三个查询和设置命令：

``maintenance packet qqemu.sstepbits``
   显示控制单步的掩码位，例如：

   ::

      (gdb) maintenance packet qqemu.sstepbits
      sending: "qqemu.sstepbits"
      received: "ENABLE=1,NOIRQ=2,NOTIMER=4"


``maintenance packet qqemu.sstep``
   显示当前掩码，例如：

   ::

      (gdb) maintenance packet qqemu.sstep
      sending: "qqemu.sstep"
      received: "0x7"


``maintenance packet Qqemu.sstep=HEX_VALUE``
   修改掩码。若要启用 IRQ 但不启用定时器：

   ::

      (gdb) maintenance packet Qqemu.sstep=0x5
      sending: "qemu.sstep=0x5"
      received: "OK"


检查物理内存
^^^^^^^^^^^^^^^^^^^^^^^^^

gdbstub 还支持切换 GDB 使用的内存视图。默认显示当前进程内存，遵循虚拟
地址转换。

需要检查或修改物理内存时，可以切换到物理模式。

查询和设置命令如下：

``maintenance packet qqemu.PhyMemMode``
    返回 0 或 1，1 表示当前为物理内存模式。

``maintenance packet Qqemu.PhyMemMode:1``
    切换到物理内存模式。

``maintenance packet Qqemu.PhyMemMode:0``
    恢复普通内存模式。

安全注意事项
=======================

连接 GDB 套接字即可在客户机执行任意代码。TCG 不构成安全边界，因此也意味着
可以在主机执行任意代码。调试 qemu-user 时，还能直接下载 QEMU 可读的任意
主机文件。

该套接字没有认证、授权或加密保护。用户必须确保仅授权客户端能连接，例如
采用权限正确的 Unix 套接字，或只在潜在攻击者无法访问的接口监听 TCP。
