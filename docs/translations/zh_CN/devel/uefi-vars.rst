=========
UEFI 变量
=========

客户机 UEFI 变量管理
====================

QEMU 客户机传统的 UEFI 变量存储方式尽量贴近物理硬件：
提供 pflash 作为存储，由客户机管理变量和闪存。

安全启动要求保护 UEFI 变量存储，禁止操作系统直接访问，所有更新请求必须通过检查。
固件的至少一部分必须在高于操作系统的权限级别运行，才能强制执行这些限制。
x86 上通过 QEMU 和 KVM 的系统管理模式（SMM）实现，与物理硬件一致。
只有运行于 SMM 的特权代码才允许访问闪存。

与 SMM 固件通信时，先将请求序列化到共享缓冲区，再通过 SMI 陷入 SMM。
SMM 代码处理请求，将回复写入同一缓冲区，然后返回。

主机 UEFI 变量服务
==================

特权代码也可以在主机而非客户机内运行，并复用序列化协议。
与主机通过虚拟设备通信，主要配置共享缓冲区位置和大小，
然后陷入主机处理请求。

``uefi-vars`` 实现该 UEFI 虚拟设备，提供 ``uefi-vars-x64``
和 ``uefi-vars-sysbus`` 两种形式。
它重新实现所需处理器，具体为 ``EfiSmmVariableProtocol``
和 ``VarCheckPolicyLibMmiHandler``，还处理 ``EfiEndOfDxeEventGroup``、
``EfiEventReadyToBoot`` 和 ``EfiEventExitBootServices`` 事件。

优点是固件无需特殊权限级别来保护自身：x64 上不依赖 SMM 模拟，
可从 Linux 内核去掉大量复杂 SMM 模拟代码（CONFIG_KVM_SMM=n）。
也能在 ARM 上支持安全启动，而无需在 KVM 实现安全世界（EL3）模拟。

缺点是新增设备扩大了主机攻击面，
并因在 QEMU 中重新实现部分 edk2 功能而产生代码重复。

在 x86_64 上使用
----------------

.. code::

   qemu-system-x86_64 \
      -device uefi-vars-x64,jsonfile=/path/to/vars.json

在 aarch64 上使用
-----------------

.. code::

   qemu-system-aarch64 -M virt \
      -device uefi-vars-sysbus,jsonfile=/path/to/vars.json
