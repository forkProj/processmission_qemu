================================================
VMWare PVSCSI 设备接口
================================================

..
   Created by Dmitry Fleytman (dmitry@daynix.com), Daynix Computing LTD.

本文基于 Linux 3.0.4 内核 PVSCSI 驱动源码，描述 VMWare PVSCSI 设备接口。

概述
========================================

接口基于虚拟机管理器与虚拟机共享的内存区。
驱动以长度为 PVSCSI_MEM_SPACE_SIZE 的设备 I/O 内存资源获取它。
共享内存由寄存器区和环形队列区组成。
寄存器区用于向虚拟机管理器触发中断和发送设备命令；
环形队列传送虚拟机到管理器的数据描述符和 SCSI 命令，以及反向消息。
数据本身通过虚拟散集 DMA 传输。

PVSCSI 设备寄存器
========================================

寄存器区长一页，即 PVSCSI_MEM_SPACE_COMMAND_NUM_PAGES。
结构由 PVSCSIRegOffset 枚举描述。
寄存器用于发送可带少量附加数据的设备命令、触发设备中断和控制中断屏蔽。

PVSCSI 设备环形队列
========================================

共享内存中有三个环：

请求环（ ``struct PVSCSIRingReqDesc *req_ring`` ）
    操作系统发往设备的请求。

完成环（ ``struct PVSCSIRingCmpDesc *cmp_ring`` ）
    设备请求完成通知。

消息环（ ``struct PVSCSIRingMsgDesc *msg_ring`` ）
    来自设备的消息，可选，客户机可以不配置。

通过控制区 ``struct PVSCSIRingsState *rings_state`` 管理环操作。

PVSCSI 设备到主机的中断
================================================

支持以下类型：

完成中断，即完成环通知：

- ``PVSCSI_INTR_CMPL_0``
- ``PVSCSI_INTR_CMPL_1``

消息中断，即消息环通知：

- ``PVSCSI_INTR_MSG_0``
- ``PVSCSI_INTR_MSG_1``

通过 PVSCSI_REG_OFFSET_INTR_MASK 控制，位为 1 启用，为 0 禁用。

支持传统中断、MSI 和 MSI-X。传统模式通过 PVSCSI_REG_OFFSET_INTR_STATUS
检查到达的中断，向中断状态寄存器写入相应位即确认中断。

PVSCSI 设备操作序列
================================================

启动序列
------------------------

a. 发送 PVSCSI_CMD_ADAPTER_RESET。
b. Windows 驱动在此读取中断状态寄存器。
c. 发送不带附加数据的 PVSCSI_CMD_SETUP_MSG_RING，检查状态；
   返回错误则禁用设备消息。驱动配置已禁用消息时省略此步。
d. 发送 PVSCSI_CMD_SETUP_RINGS，以 struct PVSCSICmdDescSetupRings 提供环配置。
e. 再次发送 PVSCSI_CMD_SETUP_MSG_RING，以 struct PVSCSICmdDescSetupMsgRing 提供配置。
f. 取消完成中断屏蔽；消息启用时也取消消息中断屏蔽。

关闭序列
------------------------

a. 屏蔽中断。
b. 使用 PVSCSI_REG_OFFSET_KICK_NON_RW_IO 刷新请求环。
c. 发送 PVSCSI_CMD_ADAPTER_RESET。

发送请求
------------------------

a. 填充下一个空闲请求描述符。
b. 读写操作使用 PVSCSI_REG_OFFSET_KICK_RW_IO，其他操作使用 PVSCSI_REG_OFFSET_KICK_NON_RW_IO。

中止命令
------------------------

a. 发送 PVSCSI_CMD_ABORT_CMD。

请求完成处理
----------------------------------------

a. 完成中断到达时处理完成环，以及启用时的消息环。
