.. _replay:

..
    Copyright (c) 2010-2022 Institute for System Programming
                        of the Russian Academy of Sciences.

    This work is licensed under the terms of the GNU GPL, version 2 or later.
    See the COPYING file in the top-level directory.

记录与重放
=============

记录/重放用于确定性地复现 QEMU 执行。记录阶段保存非确定事件日志，以后可
在任意位置无限次重放。也支持检查点，以更快回退到指定时刻。重放读取日志，
复现外部输入、硬件时钟和中断等全部非确定事件。

功能包括：

 * 确定性重放整个系统，包括全部内存、设备状态、时钟和虚拟机屏幕。
 * 保存日志，供不同机器多次重放。
 * 支持 i386、x86_64、Arm、AArch64、RISC-V、MIPS、MIPS64、S390X、Alpha、
   PowerPC、PowerPC64、M68000、Microblaze、OpenRISC、SPARC 和 Xtensa。
 * 确定性重放键鼠、串口和网络的全部操作。

使用方法：

 * 先记录：

    .. parsed-literal::
        |qemu_system| \\
        -icount shift=auto,rr=record,rrfile=replay.bin \\
        -drive file=disk.qcow2,if=none,snapshot,id=img-direct \\
        -drive driver=blkreplay,if=none,image=img-direct,id=img-blkreplay \\
        -device ide-hd,drive=img-blkreplay \\
        -netdev user,id=net1 -device rtl8139,netdev=net1 \\
        -object filter-replay,id=replay,netdev=net1


 * 再重放：

    .. parsed-literal::
        |qemu_system| \\
        -icount shift=auto,rr=replay,rrfile=replay.bin \\
        -drive file=disk.qcow2,if=none,snapshot,id=img-direct \\
        -drive driver=blkreplay,if=none,image=img-direct,id=img-blkreplay \\
        -device ide-hd,drive=img-blkreplay \\
        -netdev user,id=net1 -device rtl8139,netdev=net1 \\
        -object filter-replay,id=replay,netdev=net1


   唯一区别是 rr 从 record 改为 replay。
 * 记录模式不实际修改磁盘，变更写入临时覆盖层。每个块设备都应使用
   blkreplay 驱动，见 :ref:`block-label`。
 * 不用网络时指定 -net none，因为 QEMU 默认添加网卡；需要网络时必须
   显式配置重放过滤器，见 :ref:`network-label`。
 * 启用音频或串口设备后，其交互自动记录和重放。

核心原理
---------

系统保存并重放非确定事件，例如键盘输入，同时模拟确定事件，例如读取磁盘
或虚拟机内存。只保存非确定事件，可以缩小日志并提高速度。

日志记录键鼠输入、网络包、音频输入、串口输入和硬件时钟。时钟值来自主机，
因此也非确定。模拟硬件输入、客户机内存、软件中断和指令执行不写入日志，
因为它们可从初始状态确定性模拟。

指令计数
--------------------

必须运行于 icount 模式。它原本用于在没有外部输入时实现确定执行。
记录/重放通过 -icount 启用，使与用户或网络交互的机器也能确定执行。

.. _block-label:

块设备
-------------

模块在块驱动栈顶层拦截 bdrv 协程调用。配置如下：

.. parsed-literal::
    -drive file=disk.qcow2,if=none,snapshot,id=img-direct
    -drive driver=blkreplay,if=none,image=img-direct,id=img-blkreplay
    -device ide-hd,drive=img-blkreplay


blkreplay 应放在镜像与虚拟磁盘控制器之间，使所有磁盘请求都能记录和重放。

.. _snapshotting-label:

快照
------------

重放期间可创建快照，之后恢复所需状态。每个状态都关联重放场景中的时间点，
恢复后从该位置继续。

可通过 icount 的 rrsnapshot 指定默认起始快照：

.. parsed-literal::
    -icount shift=auto,rr=record,rrfile=replay.bin,rrsnapshot=snapshot_name


记录开始时创建，重放开始时恢复，也可在重放途中加载以回退。

移除磁盘的 snapshot 标志，才能将快照保存到指定覆盖层或原镜像，而非临时层。

.. parsed-literal::
    -drive file=disk.ovl,if=none,id=img-direct
    -drive driver=blkreplay,if=none,image=img-direct,id=img-blkreplay
    -device ide-hd,drive=img-blkreplay


监视器 savevm <name> 创建额外快照，loadvm <name> 恢复。为保护原镜像，
应使用链接到原镜像的覆盖层，所有新快照包括起始快照都存于其中。

无磁盘虚拟机需要添加一个“孤立”qcow2，专门保存快照，例如：

.. parsed-literal::
    |qemu_system| \\
      -icount shift=auto,rr=replay,rrfile=record.bin,rrsnapshot=init \\
      -net none -drive file=empty.qcow2,if=none,id=rr


empty.qcow2 不连接任何虚拟块设备，只用于保存虚拟机快照。

.. _network-label:

网络设备
---------------

通过网络过滤器记录和重放，每个后端必须有独立实例：

.. parsed-literal::
    -netdev user,id=net1 -device rtl8139,netdev=net1
    -object filter-replay,id=replay,netdev=net1


记录时把所有外部传入包写入日志；重放时从日志注入网卡，并禁用与网络后端
的全部交互。

音频设备
-------------

音频自动记录和重放，两次命令行必须指定完全相同的音频硬件，例如：

.. parsed-literal::
    -audio pa,model=ac97


串口
------------

串口输入自动记录和重放，端口数量必须相同，但后端可以不同。例如记录用
-serial stdio，重放用 -serial null。

反向调试
-----------------

反向调试允许逆向“执行”。GDB 远程协议支持反向单步和反向继续，前者回到
上一条指令，后者寻找过去最近的断点。

记录日志可实现反向调试。QEMU 并不真正倒序执行，而是加载快照，再向前重放
到所需位置或断点。

支持以下 GDB 命令：

 - reverse-stepi（rsi）：后退一条指令
 - reverse-continue（rc）：寻找过去最近的断点

反向单步加载最近快照，并重放到所需指令。

反向继续可能多次检查快照之间的执行，每轮流程为：

 #. 加载快照
 #. 重放并检查断点
 #. 若遇到断点或观察点

    * 再次加载快照
    * 重放到所需断点

 #. 否则

    * 选择更早快照，从第一步继续

因此至少需要一个快照。可移除块设备 snapshot 选项，并在记录和重放命令
都加入 rrsnapshot。详情见 :ref:`snapshotting-label`。

未使用 rrsnapshot 时，会在临时覆盖层创建 start_debugging 快照，允许
反向调试，但快照仅在当前会话内存在。
