sPAPR 的 XIVE（pseries 机器）
=======================================

POWER9 处理器引入一种新的中断控制器架构，称为 XIVE，即“外部中断虚拟化引擎”
（eXternal Interrupt Virtualization Engine）。它支持更多中断源，并提供虚拟化功能，
使硬件无需 hypervisor 协助即可将中断直接投递给虚拟处理器。

使用 POWER9 处理器的 QEMU ``pseries`` 机器（符合 PAPR）可以运行在两种中断模式下：

- *传统兼容模式*

  hypervisor 提供与 PAPR+ 2.7 版相同的接口和类似的功能。这是默认模式。

  在 QEMU 中也称为 *XICS*。

- *XIVE 原生利用模式*

  hypervisor 提供新的接口来管理 XIVE 控制结构，并通过 MMIO 页面提供直接的
  中断管理控制。

机器可使用哪些中断模式，在客户端架构支持（Client Architecture Support）协商
过程中与客户机操作系统协商确定。两种模式互斥。

两种中断模式共用同一 IRQ 编号空间。其布局见下文。

CAS 协商
---------------

QEMU 通过设备树属性 ``ibm,arch-vec-5-platform-support`` 的第 23 字节公布支持的
中断模式；操作系统对 XIVE 的选择由 ``ibm,architecture-vec-5`` 属性的第 23 字节表示。

机器支持的中断模式取决于 CPU 类型（XIVE 要求 POWER9），也取决于可在命令行设置的
机器属性 ``ic-mode``。其可取值为 ``xics``、``xive`` 和默认的 ``dual``。
``dual`` 表示同时支持 XICS **和** XIVE；如果客户机操作系统支持 XIVE，便选择 XIVE。

选定的中断模式在机器复位期间完成重新配置后激活。

KVM 协商
---------------

客户机在 KVM 下启动时，也会协商主机内核与 QEMU 的能力。KVM 是否向 QEMU 公布 XIVE
能力，取决于主机内核版本。

不过，机器可用的中断模式不应依赖主机的 XIVE KVM 能力。对于不支持 XIVE KVM 的旧版
内核，QEMU 回退使用仿真的 XIVE 设备；对于较新的内核（>=5.2），则使用 KVM XIVE 设备。

KVM 嵌套客户机，即运行于 L1 hypervisor 下的虚拟机（pSeries 上的 KVM），不支持
XIVE 原生利用模式。在这种情况下，hypervisor 不公布该 KVM 能力，QEMU 使用仿真的
XIVE 设备，与旧版 KVM 相同。

用户还可通过机器选项 ``kernel_irqchip`` 控制是否使用 KVM 设备。


KVM 支持 XIVE
~~~~~~~~~~~~~~~~~~~

对于支持 XIVE 的客户机操作系统，在支持 XIVE KVM 的主机内核上，最终中断模式如下：

==============  =============  =============  ================
ic-mode                            kernel_irqchip
--------------  ----------------------------------------------
/               allowed        off            on
                (默认)
==============  =============  =============  ================
dual (默认)     XIVE KVM       XIVE 仿真      XIVE KVM
xive            XIVE KVM       XIVE 仿真      XIVE KVM
xics            XICS KVM       XICS 仿真      XICS KVM
==============  =============  =============  ================

对于不支持 XIVE 的传统客户机操作系统，最终中断模式如下：

==============  =============  =============  ================
ic-mode                            kernel_irqchip
--------------  ----------------------------------------------
/               allowed        off            on
                (默认)
==============  =============  =============  ================
dual (默认)     XICS KVM       XICS 仿真      XICS KVM
xive            QEMU 错误 (3)  QEMU 错误 (3)  QEMU 错误 (3)
xics            XICS KVM       XICS 仿真      XICS KVM
==============  =============  =============  ================

(3) QEMU 在 CAS 阶段失败，并报错 ``Guest requested unavailable interrupt
    mode (XICS), either don't set the ic-mode machine property or try
    ic-mode=xics or ic-mode=dual``


KVM 不支持 XIVE
~~~~~~~~~~~~~~~~~~~~~~

对于支持 XIVE 的客户机操作系统，在不支持 XIVE KVM 的主机内核上，最终中断模式如下：

==============  =============  =============  ================
ic-mode                            kernel_irqchip
--------------  ----------------------------------------------
/               allowed        off            on
                (默认)
==============  =============  =============  ================
dual (默认)     XIVE 仿真 (1)  XIVE 仿真      QEMU 错误  (2)
xive            XIVE 仿真 (1)  XIVE 仿真      QEMU 错误  (2)
xics            XICS KVM       XICS 仿真      XICS KVM
==============  =============  =============  ================

(1) QEMU 发出警告 ``warning: kernel_irqchip requested but unavailable:
    IRQ_XIVE capability must be present for KVM``。
    在某些情况下（旧版主机内核或 KVM 嵌套客户机），复位时销毁设备可能引发
    QEMU/KVM 不兼容。QEMU 失败并报错
    ``KVM is incompatible with ic-mode=dual,kernel-irqchip=on``。
(2) QEMU 失败并报错 ``kernel_irqchip requested but unavailable:
    IRQ_XIVE capability must be present for KVM``。


对于不支持 XIVE 的传统客户机操作系统，最终中断模式如下：

==============  =============  =============  ================
ic-mode                            kernel_irqchip
--------------  ----------------------------------------------
/               allowed        off            on
                (默认)
==============  =============  =============  ================
dual (默认)     QEMU 错误 (4)  XICS 仿真      QEMU 错误 (4)
xive            QEMU 错误 (3)  QEMU 错误 (3)  QEMU 错误 (3)
xics            XICS KVM       XICS 仿真      XICS KVM
==============  =============  =============  ================

(3) QEMU 在 CAS 阶段失败，并报错 ``Guest requested unavailable interrupt
    mode (XICS), either don't set the ic-mode machine property or try
    ic-mode=xics or ic-mode=dual``。
(4) 复位时销毁设备导致 QEMU/KVM 不兼容。QEMU 失败并报错
    ``KVM is incompatible with ic-mode=dual,kernel-irqchip=on``。


XIVE 设备树属性
---------------------------

选择 *XIVE 原生利用模式* 时，PAPR 中断控制器节点应包含以下属性：

- ``device_type``

  值应为 "power-ivpe"。

- ``compatible``

  值应为 "ibm,power-ivpe"。

- ``reg``

  包含用户级和客户机操作系统级线程中断管理区域（TIMA）的基地址和大小。
  目前只使用客户机操作系统级区域。

- ``ibm,xive-eq-sizes``

  事件队列的大小。每种支持的大小占用一个单元，包含该大小的 log2 值，按升序排列。

- ``ibm,xive-lisn-ranges``

  分配给客户机用于 IPI 的 IRQ 中断编号范围。

根节点还导出：

- ``ibm,plat-res-int-priorities``

  包含 hypervisor 为自身使用而保留的优先级列表。

IRQ 编号空间
----------------

``pseries`` 机器的 IRQ 编号空间为 8K，两种中断模式共用。各范围定义如下：

- ``0x0000 .. 0x0FFF`` 4K 个 CPU IPI（仅在 XIVE 下使用）
- ``0x1000 .. 0x1000`` 1 个 EPOW
- ``0x1001 .. 0x1001`` 1 个 HOTPLUG
- ``0x1002 .. 0x10FF`` 未使用
- ``0x1100 .. 0x11FF`` 256 个 VIO 设备
- ``0x1200 .. 0x127F`` PHB 设备的 32x4 个 LSI
- ``0x1280 .. 0x12FF`` 未使用
- ``0x1300 .. 0x1FFF`` PHB MSI（动态分配）

监视 XIVE
---------------

可以使用监视器命令 ``info pic`` 查询 XIVE 中断控制器的状态。输出分为两部分。

首先，转储每个 CPU 的线程中断上下文寄存器状态：

::

   (qemu) info pic
   CPU[0000]:   QW   NSR CPPR IPB LSMFB ACK# INC AGE PIPR  W2
   CPU[0000]: USER    00   00  00    00   00  00  00   00  00000000
   CPU[0000]:   OS    00   ff  00    00   ff  00  ff   ff  80000400
   CPU[0000]: POOL    00   00  00    00   00  00  00   00  00000000
   CPU[0000]: PHYS    00   00  00    00   00  00  00   ff  00000000
   ...

对于 ``pseries`` 机器，QEMU 充当 hypervisor，因此只有 O/S 和 USER 寄存器环有意义。
``W2`` 包含 vCPU 的 CAM 行，其值设置为 VP 标识符。

随后是汇总 EAS 和 END 配置的路由信息：

::

   ...
   LISN         PQ    EISN     CPU/PRIO EQ
   00000000 MSI --    00000010   0/6    380/16384 @1fe3e0000 ^1 [ 80000010 ... ]
   00000001 MSI --    00000010   1/6    305/16384 @1fc230000 ^1 [ 80000010 ... ]
   00000002 MSI --    00000010   2/6    220/16384 @1fc2f0000 ^1 [ 80000010 ... ]
   00000003 MSI --    00000010   3/6    201/16384 @1fc390000 ^1 [ 80000010 ... ]
   00000004 MSI -Q  M 00000000
   00000005 MSI -Q  M 00000000
   00000006 MSI -Q  M 00000000
   00000007 MSI -Q  M 00000000
   00001000 MSI --    00000012   0/6    380/16384 @1fe3e0000 ^1 [ 80000010 ... ]
   00001001 MSI --    00000013   0/6    380/16384 @1fe3e0000 ^1 [ 80000010 ... ]
   00001100 MSI --    00000100   1/6    305/16384 @1fc230000 ^1 [ 80000010 ... ]
   00001101 MSI -Q  M 00000000
   00001200 LSI -Q  M 00000000
   00001201 LSI -Q  M 00000000
   00001202 LSI -Q  M 00000000
   00001203 LSI -Q  M 00000000
   00001300 MSI --    00000102   1/6    305/16384 @1fc230000 ^1 [ 80000010 ... ]
   00001301 MSI --    00000103   2/6    220/16384 @1fc2f0000 ^1 [ 80000010 ... ]
   00001302 MSI --    00000104   3/6    201/16384 @1fc390000 ^1 [ 80000010 ... ]

中断源信息和配置：

- ``LISN`` 列输出源中断编号（范围为 ``[ 0x0 ... 0x1FFF ]``），以及其类型：
  ``MSI`` 或 ``LSI``。
- ``PQ`` 列反映源的 PQ 位状态：

  - ``--`` 源已准备好接收事件
  - ``P-`` 已发送事件，等待 EOI
  - ``PQ`` 事件已排队
  - ``-Q`` 源已关闭

  ``M`` 表示源在 EAS 层级被 *屏蔽*。

目标配置：

- ``EISN`` 列为将加入操作系统事件队列的事件数据。
- ``CPU/PRIO`` 列是定义服务此源的 CPU 编号和优先级队列的元组。
- ``EQ`` 列输出：

  - 事件队列的当前索引／最大条目数
  - 操作系统事件队列地址
  - 翻转位
  - 最近推入事件队列的条目。
