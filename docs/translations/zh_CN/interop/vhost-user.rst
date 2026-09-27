.. _vhost_user_proto:

========================================
Vhost-user 协议
========================================

..
  Copyright 2014 Virtual Open Systems Sarl.
  Copyright 2019 Intel Corporation
  Licence: This work is licensed under the terms of the GNU GPL,
           version 2 or later. See the COPYING file in the top-level
           directory.

.. contents:: 目录

简介
========================================

此协议补充控制 Linux 内核 vhost 的 ioctl 接口，
实现与同主机用户空间进程共享 virtqueue 所需的控制平面。
通过 UNIX 域套接字通信，在消息辅助数据中共享文件描述符。

协议双方称为前端和后端。前端是共享 virtqueue 的应用，此处为 QEMU；
后端是 virtqueue 的使用者。

当前 QEMU 为前端，外部后端消费 virtio 队列，例如用户空间软件交换机 Snabbswitch，
或处理虚拟磁盘读写的块设备后端。
为方便不同实现互操作，建议遵循 :ref:`后端程序约定 <backend_conventions>`。

套接字层面，前端和后端均可作为连接的客户端，或监听的服务器。

非 Linux 平台支持
------------------------------------------------

最初面向 Linux 开发，如今支持任何提供以下功能的平台：

- 可申请由描述符表示的共享内存，经 UNIX 域套接字传递后由另一进程映射。

- 支持 SCM_RIGHTS 的 AF_UNIX 套接字，双方可交换消息和必要的辅助数据。

- eventfd 或 pipe/pipe2。无 eventfd 时，QEMU 自动回退 pipe2，最后才使用 pipe。
  各描述符通过读写 8 字节值接收或发送事件，该值本身无意义，不应解释。

消息规范
========================================

.. Note:: 所有数值采用机器原生字节序。

消息包含三个头部字段和一个载荷。

+---------+-------+------+---------+
| request | flags | size | payload |
+---------+-------+------+---------+

消息头
------------------------

:request: 32 位请求类型。

:flags: 32 位位域。

- 低 2 位为版本，当前 0x01。
- 位 2 为回复标志，后端每条回复均需设置。
- 位 3 为 need_reply，见 :ref:`VHOST_USER_PROTOCOL_F_REPLY_ACK <reply_ack>`。

:size: 32 位载荷大小。

载荷
------------------------

根据请求类型，payload 可以是下列形式。

单个 64 位整数
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+-----+
| u64 |
+-----+

:u64: 64 位无符号整数。

vring 状态描述
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+-------+-----+
| index | num |
+-------+-----+

:index: 32 位索引。

:num: 32 位数值。

拆分 virtqueue 的描述符索引
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+-------------+---------------------+
| vring index | index in avail ring |
+-------------+---------------------+

:vring index: 对应 virtqueue 的 32 位索引。

:index in avail ring: 32 位值，当前仅低 16 位使用。

  - 位 0–15：后端将处理的下一个 Available Ring 描述符索引，
    持续递增，不按环大小回绕。
  - 位 16–31：保留，置零。

紧凑 virtqueue 的描述符索引
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+-------------+--------------------+
| vring index | descriptor indices |
+-------------+--------------------+

:vring index: 对应 virtqueue 的 32 位索引。

:descriptor indices: 32 位值。

  - 位 0–14：下一个待处理 Available Ring 描述符索引，持续递增，不按环大小回绕。
  - 位 15：驱动端 Available Ring 回绕计数器。
  - 位 16–30：后端放置下一个描述符的 Used Ring 条目索引，持续递增，不按环大小回绕。
  - 位 31：设备端 Used Ring 回绕计数器。

vring 地址描述
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+-------+-------+------------+------+-----------+-----+
| index | flags | descriptor | used | available | log |
+-------+-------+------------+------+-----------+-----+

:index: 32 位 vring 索引。

:flags: 32 位 vring 标志。

:descriptor: 描述符表的 64 位环地址。

:used: 已用环的 64 位环地址。

:available: 可用环的 64 位环地址。

:log: 日志用的 64 位客户机地址。

.. Note::
   协商 VIRTIO_F_IOMMU_PLATFORM 时，环地址是 IOVA。

   否则，协商 VHOST_USER_PROTOCOL_F_GPA_ADDRESSES 时，前端消息使用客户机物理地址，
   但后端回复不受此规则影响。

   两种能力都未协商时，环地址是用户虚拟地址。

.. _memory_region_description:

内存区域描述
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+---------------+------+--------------+-------------+
| guest address | size | user address | mmap offset |
+---------------+------+--------------+-------------+

:guest address: 区域的 64 位客户机地址。

:size: 64 位大小。

:user address: 64 位用户地址。协商 VHOST_USER_PROTOCOL_F_GPA_ADDRESSES 时，
  改为客户机物理地址，必须与 guest address 相同。

:mmap offset: 区域在映射内存中起始处的 64 位偏移。

成功协商 VHOST_USER_PROTOCOL_F_XEN_MMAP 后，末尾增加两个字段。

+---------------+------+--------------+-------------+----------------+-------+
| guest address | size | user address | mmap offset | xen mmap flags | domid |
+---------------+------+--------------+-------------+----------------+-------+

:xen mmap flags: 32 位位域。

- 位 0：Xen 外部内存映射。
- 位 1：Xen 授权内存映射。
- 位 8：区域不能提前映射，后端只能在需要时映射／解除其中范围。
  不应一次映射整个区域，前端可能不允许；应仅映射所需大小，用完解除。

:domid: 32 位 Xen 专用域 ID。

对任意时刻全部活动区域：

- 任意两个区域的 [guest address, guest address + size) 不能重叠。
- 任意两个区域的 [user address, user address + size) 不能重叠。

违反即为前端错误。这确保客户机或用户地址最多对应一个内存位置。
添加重叠区域前必须先移除旧区域。

单个内存区域描述
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+---------+--------+
| padding | region |
+---------+--------+

:padding: 64 位。

:region: 使用 :ref:`内存区域描述 <memory_region_description>`。

多个内存区域描述
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+-------------+---------+---------+-----+---------+
| num regions | padding | region0 | ... | region7 |
+-------------+---------+---------+-----+---------+

:num regions: 32 位区域数量。

:padding: 32 位。

:regions: 包含 8 个 :ref:`内存区域描述 <memory_region_description>`。

日志描述
^^^^^^^^^^^^^^^^^^^^^^^^

+----------+------------+
| log size | log offset |
+----------+------------+

:log size: 日志区域的 64 位大小。

:log offset: 日志起始位置相对所提供描述符开头的 64 位偏移，
             即客户机地址 0 的日志位置。

IOTLB 消息
^^^^^^^^^^^^^^^^^^^^^^^^

+------+------+--------------+-------------------+------+
| iova | size | user address | permissions flags | type |
+------+------+--------------+-------------------+------+

:iova: 客户机配置的 64 位 I/O 虚拟地址。

:size: 64 位大小。

:user address: 64 位用户地址。协商 VHOST_USER_PROTOCOL_F_GPA_ADDRESSES 时为客户机物理地址，
  但 VHOST_USER_BACKEND_IOTLB_MSG 始终使用用户地址。

:permissions flags: 8 位值。
  - 0：无访问权限。
  - 1：可读。
  - 2：可写。
  - 3：可读写。

:type: 8 位消息类型。
  - 1：IOTLB 未命中。
  - 2：更新。
  - 3：失效。
  - 4：访问失败。

Virtio 设备配置空间
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+--------+------+-------+---------+
| offset | size | flags | payload |
+--------+------+-------+---------+

:offset: virtio 配置空间的 32 位偏移。

:size: 32 位访问大小，单位字节。

:flags: 32 位值。
  - 0：用于可写字段的前端消息。
  - 1：用于热迁移的前端消息。

:payload: 长度为 size 的字节数组，包含设备配置空间内容。

Vring 区域描述
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+-----+------+--------+
| u64 | size | offset |
+-----+------+--------+

:u64: 含 vring 索引和标志的 64 位整数。

:size: 64 位区域大小。

:offset: 区域相对所提供描述符起始处的 64 位偏移。

在途 I/O 描述
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+-----------+-------------+------------+------------+
| mmap size | mmap offset | num queues | queue size |
+-----------+-------------+------------+------------+

:mmap size: 跟踪在途 I/O 的区域大小，64 位。

:mmap offset: 相对所提供描述符开头的区域偏移，64 位。

:num queues: 16 位 virtqueue 数量。

:queue size: 16 位 virtqueue 大小。

VhostUserShared
^^^^^^^^^^^^^^^^^^^^^^^^

+------+
| UUID |
+------+

:UUID: 16 字节 UUID，前三部分（一个 32 位值、两个 16 位值）以大端保存。

设备状态传输参数
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+--------------------+-----------------+
| transfer direction | migration phase |
+--------------------+-----------------+

:transfer direction: 32 位方向枚举。

  - 0：保存，从后端传至前端，发生于迁移源端。
  - 1：加载，从前端传至后端，发生于迁移目标端。

:migration phase: 描述客户机和设备状态的 32 位枚举。

  - 0：停止，处于内存映射区域已传输、尚未切换到目标端的阶段。
    客户机停止，设备挂起，见 :ref:`挂起设备状态 <suspended_device_state>`。

  未来可能添加更多阶段，例如设备运行时的迭代迁移。

MMAP 请求
^^^^^^^^^^^^^^^^^^^^^^^^

+-------+---------+-----------+------------+-----+-------+
| shmid | padding | fd_offset | shm_offset | len | flags |
+-------+---------+-----------+------------+-----+-------+

:shmid: 8 位共享内存区域标识。

:fd_offset: 区域相对所提供描述符开头的 64 位偏移。

:shm_offset: 相对指定共享内存区域开头的 64 位偏移。

:len: 待映射内存的 64 位大小。

:flags: 64 位值。

  - 0：只读映射。
  - 1：读写映射。

VIRTIO 共享内存区域配置
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+-------------+---------+------------+----+--------------+
| num regions | padding | mem size 0 | .. | mem size 255 |
+-------------+---------+------------+----+--------------+

:num regions: 32 位区域数量。

:padding: 32 位。

:mem size: 256 个 64 位字段构成的数组，各项为对应共享内存区域大小。
           num regions 指定非零大小的有效区域数，数组索引对应 shmid。

C 结构
------------------------

QEMU 的消息实现结构如下：

.. code:: c

  typedef struct VhostUserMsg {
      VhostUserRequest request;
      uint32_t flags;
      uint32_t size;
      union {
          uint64_t u64;
          struct vhost_vring_state state;
          struct vhost_vring_addr addr;
          VhostUserMemory memory;
          VhostUserMemRegMsg mem_reg;
          VhostUserLog log;
          struct vhost_iotlb_msg iotlb;
          VhostUserConfig config;
          VhostUserCryptoSession session;
          VhostUserVringArea area;
          VhostUserInflight inflight;
          VhostUserShared object;
          VhostUserTransferDeviceState transfer_state;
          VhostUserMMap mmap;
          VhostUserShMemConfig shmem;
      };
  } QEMU_PACKED VhostUserMsg;
.. _Communication:

通信
========================================

协议基于 Linux 内核已有 vhost 实现，大多数套接字消息都有对应的内核 ioctl。

前端发请求，后端发回复。多数请求无需回复，下列例外：

* VHOST_USER_GET_FEATURES
* VHOST_USER_GET_PROTOCOL_FEATURES
* VHOST_USER_GET_VRING_BASE
* VHOST_USER_SET_LOG_BASE，启用 VHOST_USER_PROTOCOL_F_LOG_SHMFD 时
* VHOST_USER_GET_INFLIGHT_FD，启用 VHOST_USER_PROTOCOL_F_INFLIGHT_SHMFD 时

.. seealso::

   :ref:`VHOST_USER_PROTOCOL_F_REPLY_ACK <reply_ack>`
       关于 VHOST_USER_PROTOCOL_F_REPLY_ACK 协议扩展。

前端在以下消息的辅助数据中传递描述符：

* VHOST_USER_ADD_MEM_REG
* VHOST_USER_SET_MEM_TABLE
* VHOST_USER_SET_LOG_BASE，启用 VHOST_USER_PROTOCOL_F_LOG_SHMFD 时
* VHOST_USER_SET_LOG_FD
* VHOST_USER_SET_VRING_KICK
* VHOST_USER_SET_VRING_CALL
* VHOST_USER_SET_VRING_ERR
* VHOST_USER_SET_BACKEND_REQ_FD，旧名 VHOST_USER_SET_SLAVE_REQ_FD
* VHOST_USER_SET_INFLIGHT_FD，启用 VHOST_USER_PROTOCOL_F_INFLIGHT_SHMFD 时
* VHOST_USER_SET_DEVICE_STATE_FD

辅助数据必须附着在从消息头第一字节开始发送的 sendmsg 或等效操作上。
后端因而可预期，仅在读取消息头的第一次 recvmsg 中接收描述符。

前端不能发送完整消息或收到错误回复时，会关闭连接，可选实现重连。

后端检测到功能不兼容等错误时也可关闭连接，但仅应在异常情况下如此。

扩展受协议功能位控制，确保双方完全向后兼容。
旧后端不支持协议能力协商，因此专门分配一个功能位::

  #define VHOST_USER_F_PROTOCOL_FEATURES 30

它复用了 `VIRTIO 1.1 6.3 传统接口保留功能位
<https://docs.oasis-open.org/virtio/virtio/v1.1/cs01/virtio-v1.1-cs01.html#x1-4130003>`_
中的 UNUSED 位 30。VIRTIO 设备不通告此位，驱动也无法协商。

借助该保留位，以向后兼容方式加入协议功能协商，
不了解协商机制的旧前后端仍可正常工作。

环状态
----------------------------------------

环有两组独立状态：启动／停止，启用／禁用。

* 停止时，无论启用与否后端都不得处理环，但仍需跟踪启用状态，以便启动后生效。

* 已启动但禁用时，应处理环而不产生副作用。
  例如网络设备不应提供新 RX 包，但应处理并丢弃 TX 包。

* 已启动且启用时，正常处理并执行所有请求。

初始均为停止且禁用。VHOST_USER_SET_VRING_KICK 启动环，
或协商 VHOST_USER_PROTOCOL_F_INBAND_NOTIFICATIONS 后用 VHOST_USER_VRING_KICK 启动；VHOST_USER_GET_VRING_BASE 停止环。
停止后再次收到相同启动消息时，后端恢复处理。

旧规范曾规定，必须在 VHOST_USER_SET_VRING_KICK 指定描述符收到 kick，即可读，
或收到协商后的带内 VHOST_USER_VRING_KICK，才启动环。
广泛使用的实现并未遵循，而且该规则使不依赖 kick 描述符的轮询后端更复杂。

为兼容旧的“kick 才启动”后端，前端 **应** 在 VHOST_USER_SET_VRING_KICK 后注入一次 kick，
确保后端处理已有请求；后端 **不应** 依赖后续 kick。

通过 VHOST_USER_SET_VRING_ENABLE 启用或禁用环。

此外，收到未设置 VHOST_USER_F_PROTOCOL_FEATURES 的 VHOST_USER_SET_FEATURES 后，
后端必须立即启用全部环。

处理环时，无论环是否启用，后端都必须支持在线修改部分配置。

.. _suspended_device_state:

挂起设备状态
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

全部 vring 停止时，设备处于挂起状态。除不处理任何环外，必须：

* 不写任何客户机内存区域。
* 不向客户机发通知。
* 不向前端发消息。
* 仍处理并回复前端消息。

多队列支持
----------------------------------------

许多设备 virtqueue 数量固定，前端无需与后端通信便已知道数量。

部分设备由后端按主机资源或实现细节选择队列数上限，称为多队列设备。

多队列支持让后端通告最大队列数，是协议扩展，后端必须先实现功能协商。
仅设置 VHOST_USER_PROTOCOL_F_MQ（位 0）才支持。

前端用 VHOST_USER_GET_QUEUE_NUM 查询上限，请求数量超出时应停止。

全部队列共用连接，前端在消息中以唯一索引识别各队列。

前端通过 VHOST_USER_SET_VRING_ENABLE 启用队列。vhost-user-net 历史上会自动启用第一对队列。

后端始终应实现 VHOST_USER_PROTOCOL_F_MQ，即使队列数固定，因为实现简单且可提供一定自省能力。

固定队列设备的前端不得依赖 VHOST_USER_PROTOCOL_F_MQ，只有真正多队列设备才要求此功能。

迁移
----------------------------------------

热迁移期间，前端可能需跟踪后端对映射内存的修改，并在日志标记脏页。
遵循该日志要求后，可声明 VHOST_F_LOG_ALL。

为启动或停止数据／已用环写日志，前端可分别通过 VHOST_USER_SET_FEATURES 的 VHOST_F_LOG_ALL，
及 VHOST_USER_SET_VRING_ADDR 的 VHOST_VRING_F_LOG，将相应标志设为 1 或 0。

descriptor 指向内存的全部修改都应标记；
若环设置 VHOST_VRING_F_LOG，used 环修改也必须标记。

脏页大小::

  #define VHOST_LOG_PAGE 0x1000

后端支持 VHOST_USER_PROTOCOL_F_LOG_SHMFD 时，VHOST_USER_SET_LOG_BASE 的辅助数据提供日志内存描述符。

VhostUserMsg 提供日志大小，需覆盖全部已知客户机地址。
日志从描述符指定偏移开始，覆盖地址 0 到最大客户机区域。
标记 addr 所在页的伪代码::

  page = addr / VHOST_LOG_PAGE
  log[page / 8] |= 1 << page % 8

addr 为客户机物理地址。

日志可能并发修改，必须使用原子操作。

记录 used 环修改时，应以 log_guest_addr 计算偏移：
used 环第一字节的写入记录在相对日志开头的该偏移处。
此值可能超出有效客户机物理范围，无需由 VhostUserMemory 覆盖，
但环最后字节对应的位偏移必须位于 VhostUserLog 大小以内。

可选 VHOST_USER_SET_LOG_FD 消息在辅助数据中传 eventfd，通知前端日志被修改。

源端迁移完成后停止环，见 :ref:`挂起设备状态 <suspended_device_state>`。
重新启动前不得继续更新。

后拷贝迁移中，后端在全部内存接收完成前启动，必须避免访问尚未到达的页。
后端打开 userfaultfd 并注册内存，再传给前端。
前端处理访问缺页请求，页可用时执行 WAKE ioctl 唤醒阻塞的后端。
通过 VHOST_USER_PROTOCOL_F_PAGEFAULT 表示支持。

.. _migrating_backend_state:

迁移后端状态
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

设备状态从源后端传至目标后端，目标随后透明恢复运行，
无需驱动在 VIRTIO 层重新初始化。迁移失败时，源端也可透明恢复，等待再次尝试。

前端通常连接含驱动的虚拟机，已有依实现而定的状态迁移机制。
VHOST_USER_PROTOCOL_F_DEVICE_STATE 让前端将后端状态纳入该传输，
后端无需自建机制，虚拟机及 vhost-user 设备的完整状态可以保存在单一数据流中。

源端后端向前端传状态，目标端方向相反。
通道通过 VHOST_USER_SET_DEVICE_STATE_FD 协商，含两个参数：

* 方向：源端保存，从后端到前端；目标端加载，从前端到后端。

* 阶段：目前仅支持映射内存已传输、尚未切换目标的阶段，
  双方设备均 :ref:`挂起 <suspended_device_state>`。
  未来可能支持运行时迭代迁移。

通道由实现决定，但必须像管道：写端写出全部数据后关闭，表示结束；
读端必须读到 EOF 并处理全部数据。

* 保存时，源后端写、源前端读，源前端再通过自有机制传到目标前端。

* 加载时，目标前端写、目标后端读，后端反序列化内部状态，
  使驱动能在 VIRTIO 层无缝继续。

无缝意味着迁移对客户机驱动透明，不重新初始化，而是像未迁移一样继续使用设备。
但目标 vhost-user 前端仍按通常连接流程重建 vhost 状态：
内存映射、kick/call 描述符、协议功能，以及与源相同的 vring 基础索引。
对含未用缓冲区的 vring，前端也可写 kick 描述符或发送已协商的 VHOST_USER_VRING_KICK，
因为驱动可能已在源端 kick，不会再次发送。

双方前端看到描述符关闭、全部数据传完后，发送 VHOST_USER_CHECK_DEVICE_STATE，
确认后端也成功传输和处理。后端在结果确定后回复。

内存访问
----------------------------------------

前端通过 VHOST_USER_SET_MEM_TABLE 发送区域列表，每区有客户机地址和用户地址两个基址。

成功协商 VHOST_USER_PROTOCOL_F_CONFIGURE_MEM_SLOTS 后，也可用 VHOST_USER_ADD_MEM_REG 添加、VHOST_USER_REM_MEM_REG 移除区域。

客户机地址是客户机物理地址；用户地址是任意不透明值，通常为客户端进程用户空间地址。

消息用这些地址引用共享内存位置，映射如下。

用户地址映射到包含该地址的 vhost 区域。

未协商 VIRTIO_F_IOMMU_PLATFORM 时：

* 客户机地址映射到包含它的区域。

已协商时：

* 客户机地址也称 IOVA，经 IOTLB 转为用户地址。
* 不使用内存区域的 guest address。

IOMMU 支持
----------------------------------------

协商 VIRTIO_F_IOMMU_PLATFORM 后，前端以 VHOST_USER_IOTLB_MSG 发送更新与失效，
载荷为 struct vhost_iotlb_msg。
更新需填类型 2、IOVA、大小、用户虚拟地址、权限，
地址及大小必须位于 VHOST_USER_SET_MEM_TABLE 设置的区域内。
失效需填类型 3、IOVA、大小。后端成功回复零载荷值，失败非零。

后端通过 :ref:`反向通道 <backend_communication>` 发送 VHOST_USER_BACKEND_IOTLB_MSG，
载荷同样为 vhost_iotlb_msg，报告未命中和访问失败。
未命中填类型 1、IOVA、权限；访问失败填类型 4、IOVA、权限。
可借助 reply-ack 同步：已协商且后端请求回复时，前端在操作完成后确认。
未命中处理完成，意味着前端发了包含所需地址与权限的更新，
或因 IOVA／权限无效而未发送任何更新。

前端无需主动发送更新，后端会为需要访问的区域发未命中消息。

.. _backend_communication:

后端通信
----------------------------------------

后端声明 VHOST_USER_PROTOCOL_F_BACKEND_REQ 时，提供可选反向请求通道。

描述符通过 VHOST_USER_SET_BACKEND_REQ_FD 辅助数据传入。

后端经此通道发送 VHOST_USER_BACKEND_*。

协商 VHOST_USER_PROTOCOL_F_BACKEND_SEND_FD 后，也可通过辅助数据向前端发送描述符，每条最多 8 个。

.. _inflight_io_tracking:

在途 I/O 跟踪
----------------------------------------

为支持重启或崩溃后重连，后端可能需重新提交在途 I/O。
顺序处理时，可从拆分队列描述符表或紧凑队列描述符环直接获取。
乱序处理则不行：可用环或描述符环中保存信息的条目可能被新条目覆盖。
因此后端需分配额外缓冲区保存信息，并与前端共享以持久保留。
通过 VHOST_USER_GET_INFLIGHT_FD、VHOST_USER_SET_INFLIGHT_FD 传递，格式如下：

+---------------+---------------+-----+---------------+
| queue0 region | queue1 region | ... | queueN region |
+---------------+---------------+-----+---------------+

N 为可用 virtqueue 数，后端从 VhostUserInflight 的 num queues 获取。

拆分队列的区域可实现为：

.. code:: c

  typedef struct DescStateSplit {
      /* Indicate whether this descriptor is inflight or not.
       * Only available for head-descriptor. */
      uint8_t inflight;

      /* Padding */
      uint8_t padding[5];

      /* Maintain a list for the last batch of used descriptors.
       * Only available when batching is used for submitting */
      uint16_t next;

      /* Used to preserve the order of fetching available descriptors.
       * Only available for head-descriptor. */
      uint64_t counter;
  } DescStateSplit;

  typedef struct QueueRegionSplit {
      /* The feature flags of this region. Now it's initialized to 0. */
      uint64_t features;

      /* The version of this region. It's 1 currently.
       * Zero value indicates an uninitialized buffer */
      uint16_t version;

      /* The size of DescStateSplit array. It's equal to the virtqueue size.
       * The back-end could get it from queue size field of VhostUserInflight. */
      uint16_t desc_num;

      /* The head of list that track the last batch of used descriptors. */
      uint16_t last_batch_head;

      /* Store the idx value of used ring */
      uint16_t used_idx;

      /* Used to track the state of each descriptor in descriptor table */
      DescStateSplit desc[];
  } QueueRegionSplit;

为跟踪在途 I/O，应按以下方式处理区域。

接收驱动可用缓冲区时：

#. 从可用环获取下一个头描述符索引 i。
#. 将 desc[i].counter 设为全局计数器。
#. 全局计数器加 1。
#. 将 desc[i].inflight 设为 1。

向驱动提交已用缓冲区时：

1. 获取对应已用头描述符索引 i。
2. 将 desc[i].next 设为 last_batch_head。
3. 将 last_batch_head 设为 i。
#. 支持批处理时可重复步骤 1、2、3。
#. 已用环 idx 增加本批大小。
#. 将本批每个 DescStateSplit 的 inflight 清零。
#. 将 used_idx 设为已用环 idx。

重连时：

#. 若 used_idx 不等于已用环 idx，表示上一批 inflight 可能不正确：

   a. 用已用环 idx 减 used_idx，得到上一批条目数。
   #. 从 last_batch_head 开始，将上一批链表各项 inflight 清零。
   #. 将 used_idx 设为已用环 idx。

#. 按 counter 顺序重新提交在途 DescStateSplit。

紧凑队列的区域可实现为：

.. code:: c

  typedef struct DescStatePacked {
      /* Indicate whether this descriptor is inflight or not.
       * Only available for head-descriptor. */
      uint8_t inflight;

      /* Padding */
      uint8_t padding;

      /* Link to the next free entry */
      uint16_t next;

      /* Link to the last entry of descriptor list.
       * Only available for head-descriptor. */
      uint16_t last;

      /* The length of descriptor list.
       * Only available for head-descriptor. */
      uint16_t num;

      /* Used to preserve the order of fetching available descriptors.
       * Only available for head-descriptor. */
      uint64_t counter;

      /* The buffer id */
      uint16_t id;

      /* The descriptor flags */
      uint16_t flags;

      /* The buffer length */
      uint32_t len;

      /* The buffer address */
      uint64_t addr;
  } DescStatePacked;

  typedef struct QueueRegionPacked {
      /* The feature flags of this region. Now it's initialized to 0. */
      uint64_t features;

      /* The version of this region. It's 1 currently.
       * Zero value indicates an uninitialized buffer */
      uint16_t version;

      /* The size of DescStatePacked array. It's equal to the virtqueue size.
       * The back-end could get it from queue size field of VhostUserInflight. */
      uint16_t desc_num;

      /* The head of free DescStatePacked entry list */
      uint16_t free_head;

      /* The old head of free DescStatePacked entry list */
      uint16_t old_free_head;

      /* The used index of descriptor ring */
      uint16_t used_idx;

      /* The old used index of descriptor ring */
      uint16_t old_used_idx;

      /* Device ring wrap counter */
      uint8_t used_wrap_counter;

      /* The old device ring wrap counter */
      uint8_t old_used_wrap_counter;

      /* Padding */
      uint8_t padding[7];

      /* Used to track the state of each descriptor fetched from descriptor ring */
      DescStatePacked desc[];
  } QueueRegionPacked;

为跟踪在途 I/O，按以下步骤处理。

接收驱动可用缓冲区时：

#. 从描述符环获取下一个可用条目 d。

#. 若 d 为头描述符：

   a. desc[old_free_head].num 设为 0。
   #. desc[old_free_head].counter 设为全局计数器。
   #. 全局计数器加 1。
   #. desc[old_free_head].inflight 设为 1。

#. 若 d 为最后描述符，desc[old_free_head].last 设为 free_head。
#. desc[old_free_head].num 加 1。
#. desc[free_head] 的 addr、len、flags、id 分别设为 d.addr、d.len、d.flags、d.id。
#. free_head 设为 desc[free_head].next。
#. 若 d 为最后描述符，old_free_head 设为 free_head。

向驱动提交已用缓冲区时：

1. 从描述符环获取对应已用头描述符 d。
2. 获取对应 DescStatePacked 条目 e。
3. desc[e.last].next 设为 free_head。
4. free_head 设为 e 的索引。
#. 可批处理时重复步骤 1–4。
#. used_idx 增加本批大小，必要时更新 used_wrap_counter。
#. 更新 d.flags。
#. 本批每个头 DescStatePacked 的 inflight 清零。
#. old_free_head、old_used_idx、old_used_wrap_counter 分别设为
   free_head、used_idx、used_wrap_counter。

重连时：

#. 若 used_idx 不等于 old_used_idx，表示上一批 inflight 可能不正确：

   a. 通过 old_used_idx 获取下一个描述符环条目 d。
   #. 用 old_used_wrap_counter 计算可用标志。
   #. 若 d.flags 不等于计算值，表示崩溃前后端已向驱动提交缓冲区，必须提交未完成更新。
      将 old_free_head、old_used_idx、old_used_wrap_counter 分别设为
      free_head、used_idx、used_wrap_counter。

#. 将 free_head、used_idx、used_wrap_counter 分别恢复为对应 ``old_`` 值，回滚未完成更新。
#. 空闲链表每个 DescStatePacked 的 inflight 清零。
#. 按 counter 顺序重新提交在途条目。

带内通知
----------------------------------------

模拟等少数场景需要通过带内消息，而非异步 eventfd 传递 kick、call 及可选 error 信号。
可协商 VHOST_USER_PROTOCOL_F_INBAND_NOTIFICATIONS。

过多套接字消息会阻塞发送应用，除绝对必要外不建议使用。
同时必须协商 VHOST_USER_PROTOCOL_F_BACKEND_REQ 和 VHOST_USER_PROTOCOL_F_REPLY_ACK。
前者提供反向通道，后者需与带内通知配合，等待消息处理完成，
既避免后续阻塞，也保证正确处理，至少模拟场景如此。
只设置带内通知而缺少其余两项属于错误；后端没有其他报错方式，
应在收到这样的 VHOST_USER_SET_PROTOCOL_FEATURES 后关闭连接。

协议功能
----------------------------------------

.. code:: c

  #define VHOST_USER_PROTOCOL_F_MQ                       0
  #define VHOST_USER_PROTOCOL_F_LOG_SHMFD                1
  #define VHOST_USER_PROTOCOL_F_RARP                     2
  #define VHOST_USER_PROTOCOL_F_REPLY_ACK                3
  #define VHOST_USER_PROTOCOL_F_MTU                      4
  #define VHOST_USER_PROTOCOL_F_BACKEND_REQ              5
  #define VHOST_USER_PROTOCOL_F_CROSS_ENDIAN             6
  #define VHOST_USER_PROTOCOL_F_CRYPTO_SESSION           7
  #define VHOST_USER_PROTOCOL_F_PAGEFAULT                8
  #define VHOST_USER_PROTOCOL_F_CONFIG                   9
  #define VHOST_USER_PROTOCOL_F_BACKEND_SEND_FD         10
  #define VHOST_USER_PROTOCOL_F_HOST_NOTIFIER           11
  #define VHOST_USER_PROTOCOL_F_INFLIGHT_SHMFD          12
  #define VHOST_USER_PROTOCOL_F_RESET_DEVICE            13
  #define VHOST_USER_PROTOCOL_F_INBAND_NOTIFICATIONS    14
  #define VHOST_USER_PROTOCOL_F_CONFIGURE_MEM_SLOTS     15
  #define VHOST_USER_PROTOCOL_F_STATUS                  16
  #define VHOST_USER_PROTOCOL_F_XEN_MMAP                17
  #define VHOST_USER_PROTOCOL_F_SHARED_OBJECT           18
  #define VHOST_USER_PROTOCOL_F_DEVICE_STATE            19
  #define VHOST_USER_PROTOCOL_F_GET_VRING_BASE_INFLIGHT 20
  #define VHOST_USER_PROTOCOL_F_GPA_ADDRESSES           21
  #define VHOST_USER_PROTOCOL_F_SHMEM_MAP               22

前端消息类型
----------------------------------------

``VHOST_USER_GET_FEATURES``
  :id: 1
  :等效 ioctl: ``VHOST_GET_FEATURES``
  :请求载荷: 不适用
  :回复载荷: ``u64``

  获取底层 vhost 的功能位掩码。VHOST_USER_F_PROTOCOL_FEATURES 表示后端支持
  VHOST_USER_GET_PROTOCOL_FEATURES 和 VHOST_USER_SET_PROTOCOL_FEATURES。

``VHOST_USER_SET_FEATURES``
  :id: 2
  :等效 ioctl: ``VHOST_SET_FEATURES``
  :请求载荷: ``u64``
  :回复载荷: 不适用

  通过位掩码启用底层功能。VHOST_USER_F_PROTOCOL_FEATURES 表示后端支持
  VHOST_USER_GET_PROTOCOL_FEATURES 和 VHOST_USER_SET_PROTOCOL_FEATURES。

``VHOST_USER_GET_PROTOCOL_FEATURES``
  :id: 15
  :等效 ioctl: ``VHOST_GET_FEATURES``
  :请求载荷: 不适用
  :回复载荷: ``u64``

  获取协议功能掩码。仅在 VHOST_USER_GET_FEATURES 报告 VHOST_USER_F_PROTOCOL_FEATURES 时合法，
  无需由 VHOST_USER_SET_FEATURES 确认。

.. Note::
   报告此功能位的后端，必须在 VHOST_USER_SET_FEATURES 调用之前就支持此消息。

``VHOST_USER_SET_PROTOCOL_FEATURES``
  :id: 16
  :等效 ioctl: ``VHOST_SET_FEATURES``
  :请求载荷: ``u64``
  :回复载荷: 不适用

  启用底层协议功能。

  仅在 VHOST_USER_GET_FEATURES 报告 VHOST_USER_F_PROTOCOL_FEATURES 时合法，
  无需由 VHOST_USER_SET_FEATURES 确认。

.. Note::
   报告此功能位的后端，必须在 VHOST_USER_SET_FEATURES 调用之前就支持此消息。

``VHOST_USER_SET_OWNER``
  :id: 3
  :等效 ioctl: ``VHOST_SET_OWNER``
  :请求载荷: 不适用
  :回复载荷: 不适用

  新连接建立时发送，将发送方标为会话所有者前端。
  后端可将其视为会话开始标志。

``VHOST_USER_RESET_OWNER``
  :id: 4
  :请求载荷: 不适用
  :回复载荷: 不适用

.. admonition:: 已弃用

   不再使用。过去请求禁用全部环，但部分后端还丢弃连接状态，会导致错误。
   建议后端忽略此消息，或仅用它禁用全部环。

``VHOST_USER_SET_MEM_TABLE``
  :id: 5
  :等效 ioctl: ``VHOST_SET_MEM_TABLE``
  :请求载荷: 多个内存区域描述
  :回复载荷: （仅后拷贝） 多个内存区域描述

  设置后端内存映射区域，供转换 vring 地址。
  辅助数据含各区域的描述符数组，数量与顺序必须与区域一致。

  已收到 VHOST_USER_POSTCOPY_LISTEN 时，VHOST_USER_SET_MEM_TABLE 向前端回复映射区域基址。
  后端必须已 mmap()，但尚未访问，也不应产生 userfault 事件。

.. Note::
   此时不设置 NEED_REPLY_MASK。QEMU 随后以空 VHOST_USER_SET_MEM_TABLE 确认映射列表，
   只有收到此确认后，客户机才可访问内存并产生缺页。

``VHOST_USER_SET_LOG_BASE``
  :id: 6
  :等效 ioctl: ``VHOST_SET_LOG_BASE``
  :请求载荷: u64
  :回复载荷: 不适用

  设置日志共享内存。

  后端支持 VHOST_USER_PROTOCOL_F_LOG_SHMFD 时，辅助数据提供日志内存描述符，消息提供大小和偏移。

``VHOST_USER_SET_LOG_FD``
  :id: 7
  :等效 ioctl: ``VHOST_SET_LOG_FD``
  :请求载荷: 不适用
  :回复载荷: 不适用

  设置日志描述符，通过辅助数据传递。

``VHOST_USER_SET_VRING_NUM``
  :id: 8
  :等效 ioctl: ``VHOST_SET_VRING_NUM``
  :请求载荷: vring 状态描述
  :回复载荷: 不适用

  设置队列大小。

``VHOST_USER_SET_VRING_ADDR``
  :id: 9
  :等效 ioctl: ``VHOST_SET_VRING_ADDR``
  :请求载荷: vring 地址描述
  :回复载荷: 不适用

  设置 vring 各组成部分的地址。

``VHOST_USER_SET_VRING_BASE``
  :id: 10
  :等效 ioctl: ``VHOST_SET_VRING_BASE``
  :请求载荷: vring 描述符索引
  :回复载荷: 不适用

  设置 vring 下一个描述符索引。

  * 拆分队列仅设置可用环下一个待处理索引。
    设备应从客户机内存的对应 vring 结构读取已用环下一个索引。
  * 紧凑队列必须同时提供两索引，因为内存中不存在显式值。

  因此载荷随队列类型不同，分别为拆分队列单索引或紧凑队列双索引。

``VHOST_USER_GET_VRING_BASE``
  :id: 11
  :等效 ioctl: ``VHOST_USER_GET_VRING_BASE``
  :请求载荷: vring 状态描述
  :回复载荷: vring 描述符索引

  停止 vring，并返回当前描述符索引。

  * 拆分队列只返回可用环下一个待处理的 16 位索引。
    它可能不同于内存 vring 的可用环索引，后者指向驱动放入新描述符的位置。
    已用环只需下一个放入描述符的索引，该值已在内存 vring 中，因此不包含于消息。
  * 紧凑队列的两个索引均无法从内存直接读取，所以返回设备维护的两者。

  因此载荷随队列类型而异。

  全部 vring 都停止期间，设备处于 :ref:`挂起状态 <suspended_device_state>`。

  请求 num 字段保留，必须为 0。

  默认情况下，后端停止前必须完成该 vring 全部在途 I/O。

  若协商 VHOST_USER_PROTOCOL_F_GET_VRING_BASE_INFLIGHT，可挂起并按
  :ref:`在途 I/O 跟踪 <inflight_io_tracking>` 记录，而非先完成。
  如何挂起由实现决定，通常中止或取消底层 I/O。
  只有同时协商 VHOST_USER_PROTOCOL_F_INFLIGHT_SHMFD 才能协商此功能。

``VHOST_USER_SET_VRING_KICK``
  :id: 12
  :等效 ioctl: ``VHOST_SET_VRING_KICK``
  :请求载荷: ``u64``
  :回复载荷: 不适用

  设置向 vring 添加缓冲区的事件描述符，经辅助数据传递。

  载荷位 0–7 是索引，位 8 表示无有效描述符，
  辅助数据不含描述符时设置，指示使用轮询而非等待 kick。
  协商 VHOST_USER_PROTOCOL_F_INBAND_NOTIFICATIONS 后，VHOST_USER_VRING_KICK 也会启动环，因此此消息并非必需。
  仍可用它设置优先于消息的 eventfd，或启用轮询。

``VHOST_USER_SET_VRING_CALL``
  :id: 13
  :等效 ioctl: ``VHOST_SET_VRING_CALL``
  :请求载荷: ``u64``
  :回复载荷: 不适用

  设置缓冲区使用后发信号的 eventfd，经辅助数据传递。

  位 0–7 为索引，位 8 在没有描述符时设置，指示轮询而非等待 call。
  已协商 VHOST_USER_PROTOCOL_F_INBAND_NOTIFICATIONS 和 VHOST_USER_PROTOCOL_F_BACKEND_REQ 时，可用 VHOST_USER_BACKEND_VRING_CALL，
  本消息非必需，但仍可配置 eventfd 或启用轮询。

``VHOST_USER_SET_VRING_ERR``
  :id: 14
  :等效 ioctl: ``VHOST_SET_VRING_ERR``
  :请求载荷: ``u64``
  :回复载荷: 不适用

  设置发生错误时发信号的 eventfd，经辅助数据传递。

  位 0–7 为索引，位 8 在无描述符时设置。
  已协商 VHOST_USER_PROTOCOL_F_INBAND_NOTIFICATIONS 和 VHOST_USER_PROTOCOL_F_BACKEND_REQ 时，可用 VHOST_USER_BACKEND_VRING_ERR，
  但本消息仍可设置优先于消息的 eventfd。

``VHOST_USER_GET_QUEUE_NUM``
  :id: 17
  :等效 ioctl: 不适用
  :请求载荷: 不适用
  :回复载荷: u64

  查询后端支持的队列数。

  仅在 VHOST_USER_GET_PROTOCOL_FEATURES 查询结果设置 VHOST_USER_PROTOCOL_F_MQ 时发送。

``VHOST_USER_SET_VRING_ENABLE``
  :id: 18
  :等效 ioctl: 不适用
  :请求载荷: vring 状态描述
  :回复载荷: 不适用

  通知后端启用或禁用对应 vring。

  仅在已协商 VHOST_USER_F_PROTOCOL_FEATURES 时发送。

``VHOST_USER_SEND_RARP``
  :id: 19
  :等效 ioctl: 不适用
  :请求载荷: ``u64``
  :回复载荷: 不适用

  请求后端广播伪造 VHOST_USER_PROTOCOL_F_RARP，通知不支持 GUEST_ANNOUNCE 的客户机迁移已结束。

  仅在 VHOST_USER_GET_FEATURES 含 VHOST_USER_F_PROTOCOL_FEATURES，
  VHOST_USER_GET_PROTOCOL_FEATURES 含 VHOST_USER_PROTOCOL_F_RARP 时合法。
  载荷前 6 字节是客户机 MAC，供构造并广播 VHOST_USER_PROTOCOL_F_RARP。

``VHOST_USER_NET_SET_MTU``
  :id: 20
  :等效 ioctl: 不适用
  :请求载荷: ``u64``
  :回复载荷: 不适用

  设置向客户机暴露的主机 MTU。

  仅在已协商 VIRTIO_NET_F_MTU，VHOST_USER_GET_FEATURES 含 VHOST_USER_F_PROTOCOL_FEATURES，
  且 VHOST_USER_GET_PROTOCOL_FEATURES 含 VHOST_USER_PROTOCOL_F_NET_MTU 时发送。

  若协商 VHOST_USER_PROTOCOL_F_REPLY_ACK，MTU 有效时回复零，否则非零。

``VHOST_USER_SET_BACKEND_REQ_FD`` (旧名 ``VHOST_USER_SET_SLAVE_REQ_FD``)
  :id: 21
  :等效 ioctl: 不适用
  :请求载荷: 不适用
  :回复载荷: 不适用

  设置后端发起请求的套接字描述符，经辅助数据传递。

  仅在已协商 VHOST_USER_F_PROTOCOL_FEATURES，且 VHOST_USER_GET_PROTOCOL_FEATURES 含
  VHOST_USER_PROTOCOL_F_BACKEND_REQ 时发送。
  协商 VHOST_USER_PROTOCOL_F_REPLY_ACK 时，成功回复零，否则非零。

``VHOST_USER_IOTLB_MSG``
  :id: 22
  :等效 ioctl: 不适用 (等效于 ``VHOST_IOTLB_MSG`` 消息类型)
  :请求载荷: ``struct vhost_iotlb_msg``
  :回复载荷: ``u64``

  以 struct vhost_iotlb_msg 为载荷发送 IOTLB 消息。

  前端用它更新和失效设备 IOTLB 条目。后端必须以 u64 确认，成功零、失败非零。

  仅在成功协商 VIRTIO_F_IOMMU_PLATFORM 时发送。

``VHOST_USER_SET_VRING_ENDIAN``
  :id: 23
  :等效 ioctl: ``VHOST_SET_VRING_ENDIAN``
  :请求载荷: vring 状态描述
  :回复载荷: 不适用

  设置传统设备 VQ 字节序。state.num 为 0 表示小端，为 1 表示大端，其他值无效。

  仅在协商 VHOST_USER_PROTOCOL_F_CROSS_ENDIAN 时发送。后端须支持两种字节序，
  并预期每个 VQ 在配置时、前端启动前收到一次消息。

``VHOST_USER_GET_CONFIG``
  :id: 24
  :等效 ioctl: 不适用
  :请求载荷: virtio 设备配置空间
  :回复载荷: virtio 设备配置空间

  协商 VHOST_USER_PROTOCOL_F_CONFIG 后，前端获取 virtio 配置空间内容。
  后端载荷大小 **必须** 与请求匹配，用零长度表示错误。
  前端可缓存内容以避免重复调用。

``VHOST_USER_SET_CONFIG``
  :id: 25
  :等效 ioctl: 不适用
  :请求载荷: virtio 设备配置空间
  :回复载荷: 不适用

  协商 VHOST_USER_PROTOCOL_F_CONFIG 后，客户机修改配置空间时前端发送，也可在目标主机热迁移时使用。
  后端必须检查 flags；除非设置热迁移位，否则 **不得** 接受对只读配置字段的写入。

``VHOST_USER_CREATE_CRYPTO_SESSION``
  :id: 26
  :等效 ioctl: 不适用
  :请求载荷: 加密会话描述
  :回复载荷: 加密会话描述

  创建加密操作会话。后端必须返回会话 ID，零或正数成功，负数失败。
  仅成功协商 VHOST_USER_PROTOCOL_F_CRYPTO_SESSION 时发送，这是加密设备必需功能。

``VHOST_USER_CLOSE_CRYPTO_SESSION``
  :id: 27
  :等效 ioctl: 不适用
  :请求载荷: ``u64``
  :回复载荷: 不适用

  关闭此前通过 VHOST_USER_CREATE_CRYPTO_SESSION 创建的会话。

  仅成功协商 VHOST_USER_PROTOCOL_F_CRYPTO_SESSION 时发送，这是加密设备必需功能。

``VHOST_USER_POSTCOPY_ADVISE``
  :id: 28
  :请求载荷: 不适用
  :回复载荷: userfault 描述符

  支持 VHOST_USER_PROTOCOL_F_PAGEFAULT 时，前端告知启用后拷贝的迁移正在进行，后端必须打开 userfaultfd。
  注意此时迁移仍处于预拷贝模式。

``VHOST_USER_POSTCOPY_LISTEN``
  :id: 29
  :请求载荷: 不适用
  :回复载荷: 不适用

  前端告知已切换后拷贝，后端必须确保共享内存已注册到 userfaultfd，
  使不存在的页触发缺页。

  总在 VHOST_USER_POSTCOPY_ADVISE 之后发送，因此仅用于支持 VHOST_USER_PROTOCOL_F_PAGEFAULT 的情况。

``VHOST_USER_POSTCOPY_END``
  :id: 30
  :请求载荷: 不适用
  :回复载荷: ``u64``

  告知后拷贝迁移完成，后端必须禁用 userfaultfd。回复仅作确认。

  支持 VHOST_USER_PROTOCOL_F_PAGEFAULT 时，在此前发送 VHOST_USER_POSTCOPY_LISTEN 后，于迁移末尾发送。

  返回值为错误指示，0 成功。

``VHOST_USER_GET_INFLIGHT_FD``
  :id: 31
  :等效 ioctl: 不适用
  :请求载荷: 在途 I/O 描述
  :回复载荷: 不适用

  成功协商 VHOST_USER_PROTOCOL_F_INFLIGHT_SHMFD 后，前端向后端获取用于跟踪在途 I/O 的共享缓冲区。
  虚拟机复位时 QEMU 应重新获取。

``VHOST_USER_SET_INFLIGHT_FD``
  :id: 32
  :等效 ioctl: 不适用
  :请求载荷: 在途 I/O 描述
  :回复载荷: 不适用

  成功协商 VHOST_USER_PROTOCOL_F_INFLIGHT_SHMFD 后，前端将共享在途缓冲区传回后端，
  让后端在崩溃或重启后恢复在途 I/O。

``VHOST_USER_GPU_SET_SOCKET``
  :id: 33
  :等效 ioctl: 不适用
  :请求载荷: 不适用
  :回复载荷: 不适用

  通过辅助数据设置 GPU 协议套接字。GPU 协议向前端通知渲染状态和更新，
  详情见 vhost-user-gpu.rst。

``VHOST_USER_RESET_DEVICE``
  :id: 34
  :等效 ioctl: 不适用
  :请求载荷: 不适用
  :回复载荷: 不适用

  请求后端禁用全部环，将内部设备状态恢复初始值，准备重新初始化。
  复位全程保留设备所有权。

  仅后端设置 VHOST_USER_PROTOCOL_F_RESET_DEVICE 协议功能时有效。

``VHOST_USER_VRING_KICK``
  :id: 35
  :等效 ioctl: 不适用
  :请求载荷: vring 状态描述
  :回复载荷: 不适用

  协商 VHOST_USER_PROTOCOL_F_INBAND_NOTIFICATIONS 后，前端可用此消息通知 vring 新增缓冲区，
  代替 kick 描述符或后端轮询。

  state.num 保留，必须为 0。

``VHOST_USER_GET_MAX_MEM_SLOTS``
  :id: 36
  :等效 ioctl: 不适用
  :请求载荷: 不适用
  :回复载荷: u64

  协商 VHOST_USER_PROTOCOL_F_CONFIGURE_MEM_SLOTS 后，前端查询可向客户机暴露的最大内存槽数。
  后端以 u64 返回，最终值受目标平台最大 RAM 槽数限制。

``VHOST_USER_ADD_MEM_REG``
  :id: 37
  :等效 ioctl: 不适用
  :请求载荷: 不适用
  :回复载荷: 单个内存区域描述

  成功协商 VHOST_USER_PROTOCOL_F_CONFIGURE_MEM_SLOTS 后，前端发送此消息，载荷为内存区域描述，
  指定后端必须映射的客户机内存。它与 VHOST_USER_REM_MEM_REG 配合设置和更新后端内存表。

  辅助数据必须恰好传入一个用于映射的描述符。

  后拷贝模式（见 VHOST_USER_POSTCOPY_LISTEN）下，后端向前端回复映射基址。
  VHOST_USER_SET_MEM_TABLE 的后拷贝细节也相应适用于此消息。

``VHOST_USER_REM_MEM_REG``
  :id: 38
  :等效 ioctl: 不适用
  :请求载荷: 不适用
  :回复载荷: 单个内存区域描述

  协商 VHOST_USER_PROTOCOL_F_CONFIGURE_MEM_SLOTS 后，前端用区域描述指定后端应解除映射的内存。
  与 VHOST_USER_ADD_MEM_REG 配合设置和更新内存表。

  通过客户机地址、用户地址和大小识别区域，忽略 mmap offset。

  辅助数据 **不应** 传描述符。为兼容已有错误实现，后端 **可以** 接受一个描述符，
  但 **必须** 关闭它，不作其他用途。

``VHOST_USER_SET_STATUS``
  :id: 39
  :等效 ioctl: VHOST_VDPA_SET_STATUS
  :请求载荷: ``u64``
  :回复载荷: 不适用

  协商 VHOST_USER_PROTOCOL_F_STATUS 后，前端通知后端更新后的 Virtio 设备状态。

``VHOST_USER_GET_STATUS``
  :id: 40
  :等效 ioctl: VHOST_VDPA_GET_STATUS
  :请求载荷: 不适用
  :回复载荷: ``u64``

  协商 VHOST_USER_PROTOCOL_F_STATUS 后，前端查询后端的 Virtio 设备状态。

``VHOST_USER_GET_SHARED_OBJECT``
  :id: 41
  :等效 ioctl: 不适用
  :请求载荷: ``struct VhostUserShared``
  :回复载荷: dmabuf 描述符

  协商 VHOST_USER_PROTOCOL_F_SHARED_OBJECT 且 UUID 位于导出者缓存时，前端向对应后端获取指定 UUID 的 dma-buf。
  成功回复附描述符，失败不附。

``VHOST_USER_SET_DEVICE_STATE_FD``
  :id: 42
  :等效 ioctl: 不适用
  :请求载荷: 设备状态传输参数
  :回复载荷: ``u64``

  双方协商迁移时传输后端内部状态的通道，任一方可创建。
  本文不限制通道类型，但创建方必须提供可供对方访问的描述符，满足：

  * 写端可顺序写出完整后端状态，关闭表示结束。
  * 读端可顺序读取完整后端状态，EOF 表示结束。

  管道两端即符合这些要求。

  初始由前端创建通道，并在此消息辅助数据中传入描述符。
  后端可创建另一通道，在回复辅助数据返回描述符；
  此时前端必须丢弃自己的通道，改用后端提供的。

  是否改用后端通道取决于效率。管道两端通常都需复制数据，
  若其他通道能通过零拷贝等方式改善至少一端的处理，就更高效，应优先选择。
  后端能提供此类通道时应使用。

  请求载荷含传输参数，见 :ref:`后端状态迁移 <migrating_backend_state>`。

  返回值同时表示结果和是否返回后端通道：位 0–7 成功为 0、错误非零；
  位 8 为无效描述符标志，未返回描述符时设置。
  未设置时，前端必须使用返回描述符作为本端通道。
  后端不得同时报告错误并返回描述符。

  必须预先协商 VHOST_USER_PROTOCOL_F_DEVICE_STATE。

``VHOST_USER_CHECK_DEVICE_STATE``
  :id: 43
  :等效 ioctl: 不适用
  :请求载荷: 不适用
  :回复载荷: ``u64``

  :ref:`传输后端内部状态 <migrating_backend_state>` 后，检查后端是否完整成功处理。

  返回 0 成功，任意非零值表示错误。

  必须预先协商 VHOST_USER_PROTOCOL_F_DEVICE_STATE。

``VHOST_USER_GET_SHMEM_CONFIG``
  :id: 44
  :等效 ioctl: 不适用
  :请求载荷: 不适用
  :回复载荷: ``struct VhostUserShMemConfig``

  成功协商 VHOST_USER_PROTOCOL_F_SHMEM 后，前端获取 VIRTIO 共享内存区域配置。
  后端回复所需区域数量及各区域大小数组，索引即共享内存 ID。必须遵守：

  * 整个连接生命周期内信息保持有效且不变。
  * 大小必须为 mmap(2) 支持的页大小整数倍。
  * 未使用区域可为 0。

后端消息类型
----------------------------------------

以下消息由后端请求、前端回复。

``VHOST_USER_BACKEND_IOTLB_MSG`` (旧名 ``VHOST_USER_SLAVE_IOTLB_MSG``)
  :id: 1
  :等效 ioctl: 不适用 (等效于 ``VHOST_IOTLB_MSG`` 消息类型)
  :请求载荷: ``struct vhost_iotlb_msg``
  :回复载荷: 不适用

  使用 struct vhost_iotlb_msg 通知 IOTLB 未命中或访问失败。
  已协商 VHOST_USER_PROTOCOL_F_REPLY_ACK 且设置 VHOST_USER_NEED_REPLY 时，
  前端完成操作后必须回复零表示成功，非零表示失败。
  仅成功协商 VIRTIO_F_IOMMU_PLATFORM 时发送。

``VHOST_USER_BACKEND_CONFIG_CHANGE_MSG`` (旧名 ``VHOST_USER_SLAVE_CONFIG_CHANGE_MSG``)
  :id: 2
  :等效 ioctl: 不适用
  :请求载荷: 不适用
  :回复载荷: 不适用

  协商 VHOST_USER_PROTOCOL_F_CONFIG 后，后端通知配置空间变化。支持此功能的主机设备驱动
  可发 VHOST_USER_GET_CONFIG 获取最新内容。
  已协商 VHOST_USER_PROTOCOL_F_REPLY_ACK 且后端设置 VHOST_USER_NEED_REPLY 时，前端必须回复零成功、非零失败。

``VHOST_USER_BACKEND_VRING_HOST_NOTIFIER_MSG`` (旧名 ``VHOST_USER_SLAVE_VRING_HOST_NOTIFIER_MSG``)
  :id: 3
  :等效 ioctl: 不适用
  :请求载荷: vring 区域描述
  :回复载荷: 不适用

  设置指定队列的主机通知器。队列索引在区域描述的 u64 中。
  通知器由辅助数据中的描述符（通常是 VFIO 设备描述符）、
  描述中的大小（mmap 大小，须等于主机页大小）和偏移共同定义。
  QEMU 可据此 mmap() 得到内存范围，再映射给虚拟机作为该队列通知 MMIO。
  收到请求后应先注销已有通知器；请求附描述符时再注册新通知器。

  仅成功协商 VHOST_USER_PROTOCOL_F_HOST_NOTIFIER 时发送。

``VHOST_USER_BACKEND_VRING_CALL`` (旧名 ``VHOST_USER_SLAVE_VRING_CALL``)
  :id: 4
  :等效 ioctl: 不适用
  :请求载荷: vring 状态描述
  :回复载荷: 不适用

  成功协商 VHOST_USER_PROTOCOL_F_INBAND_NOTIFICATIONS 后，后端可通知 vring 缓冲区已使用，
  代替 call 描述符或前端轮询。

  state.num 保留，必须为 0。

``VHOST_USER_BACKEND_VRING_ERR`` (旧名 ``VHOST_USER_SLAVE_VRING_ERR``)
  :id: 5
  :等效 ioctl: 不适用
  :请求载荷: vring 状态描述
  :回复载荷: 不适用

  成功协商 VHOST_USER_PROTOCOL_F_INBAND_NOTIFICATIONS 后，后端可通知指定 vring 发生错误，
  代替前端经 VHOST_USER_SET_VRING_ERR 配置的错误描述符。

  state.num 保留，必须为 0。

``VHOST_USER_BACKEND_SHARED_OBJECT_ADD``
  :id: 6
  :等效 ioctl: 不适用
  :请求载荷: ``struct VhostUserShared``
  :回复载荷: 不适用

  成功协商 VHOST_USER_PROTOCOL_F_SHARED_OBJECT 后，后端可将自己作为导出者加入 virtio 共享查找表，
  设备与 UUID 关联。后端负责维护自身导出的 dma-buf 表。
  其他后端导入相同 UUID 时向前端发请求，由前端代理到导出者。
  协商 VHOST_USER_PROTOCOL_F_REPLY_ACK 且设置 VHOST_USER_NEED_REPLY 时，前端必须回复零成功、非零失败。

``VHOST_USER_BACKEND_SHARED_OBJECT_REMOVE``
  :id: 7
  :等效 ioctl: 不适用
  :请求载荷: ``struct VhostUserShared``
  :回复载荷: 不适用

  成功协商 VHOST_USER_PROTOCOL_F_SHARED_OBJECT 后，后端可从 virtio-dmabuf 共享表移除自己。
  仅条目拥有者，即最初添加者有权移除，否则忽略。
  表会移除 UUID 关联设备。协商 VHOST_USER_PROTOCOL_F_REPLY_ACK 且设置 VHOST_USER_NEED_REPLY 时，
  前端必须回复零成功、非零失败。

``VHOST_USER_BACKEND_SHARED_OBJECT_LOOKUP``
  :id: 8
  :等效 ioctl: 不适用
  :请求载荷: ``struct VhostUserShared``
  :回复载荷: dmabuf 描述符和 ``u64``

  成功协商 VHOST_USER_PROTOCOL_F_SHARED_OBJECT 后，后端按 UUID 从 virtio-dmabuf 共享表获取 dma-buf。
  前端成功回复描述符和零，失败返回非零且不传描述符。

``VHOST_USER_BACKEND_SHMEM_MAP``
  :id: 9
  :等效 ioctl: 不适用
  :请求载荷: 描述符和 ``struct VhostUserMMap``
  :回复载荷: 不适用

  成功协商 VHOST_USER_PROTOCOL_F_SHMEM 后，后端通告需在指定 VIRTIO 共享区域创建的新映射。
  前端将给定描述符 mmap 到请求 shmid 对应的区域。

  协商 VHOST_USER_PROTOCOL_F_REPLY_ACK 且设置 VHOST_USER_NEED_REPLY 时，必须回复零成功、非零失败。

  不允许覆盖已有映射，否则请求必须失败。
  范围必须对应有效且空闲的共享区域。映射消耗资源，资源不足也可失败。
  前端会在设备复位时自动解除这些映射。

``VHOST_USER_BACKEND_SHMEM_UNMAP``
  :id: 10
  :等效 ioctl: 不适用
  :请求载荷: ``struct VhostUserMMap``
  :回复载荷: 不适用

  成功协商 VHOST_USER_PROTOCOL_F_SHMEM 后，请求前端解除 shmid 区域中的 shm_offset、len 范围。
  范围必须精确覆盖一个有效映射的整体。

  协商 VHOST_USER_PROTOCOL_F_REPLY_ACK 且设置 VHOST_USER_NEED_REPLY 时，必须回复零成功、非零失败。

.. _reply_ack:

VHOST_USER_PROTOCOL_F_REPLY_ACK
------------------------------------------------------------

原协议只要求部分命令回复，与内核 vhost ioctl 阻塞到后端完成的行为不同。

协商此扩展后，发送方 QEMU 可为任何命令设置 need_reply（位 3）。
后端 **必须** 回复带载荷的 VhostUserMsg 表示成功或失败。
除非消息本来就有明确回复体，否则成功载荷为零，失败非零。

此回复让 QEMU 确定命令结果。当前预期收到错误后终止主 vhost-user 循环；
未来可针对部分请求提高恢复能力。

本来就要求回复的消息，无论协商此功能或设置 need_reply，都不改变行为。
见 Communication_。

.. _backend_conventions:

后端程序约定
========================================

后端提供多种设备和服务，按用途可能需手工配置。
尽可能遵循这些约定，使用户、QEMU 或 libvirt 依赖一致行为，
避免异构配置与管理，方便互操作。

主机安装的每个后端应至少附一个符合 vhost-user.json 模式的 JSON 文件，
说明类型和二进制位置。多个实现匹配时，也定义选择最高优先级后端的规则，
见模式中的 @VhostUserBackend。

后端无法在主机启用所需功能（如 virgl 3D），或初始化失败时，
应尽早启动失败，以非零状态退出，可向 stderr 输出详情。

后端不得自行守护进程化，但可由管理层放入后台，也可能受到系统访问限制。

描述符 0、1、2 存在，具有普通 stdin/stdout/stderr 语义，
可能被管理层重定向到 /dev/null 或日志处理程序。

收到 SIGTERM 后必须尽快干净退出；数秒后管理层可能发送 SIGKILL。

除非明确可选，以下命令行选项均必须实现，行为如下。

--socket-path=PATH

  指定 vhost-user UNIX 域套接字位置，与 --fd 互斥。

--fd=FDNUM

  启动时已有描述符 FDNUM 指向 vhost-user 套接字，与 --socket-path 互斥。

--print-capabilities

  以 JSON 向 stdout 输出能力并成功退出。忽略其他参数，不执行正常功能。
  能力可根据主机动态报告。

JSON 由 vhost-user.json 的 ``@VHostUserBackendCapabilities`` 描述，例如：

.. code:: json

  {
    "type": "foo",
    "features": [
      "feature-a",
      "feature-b"
    ]
  }

vhost-user-input
----------------------------------------

命令行选项：

--evdev-path=PATH

  指定 Linux 输入设备。

  可选。

--no-grab

  不请求输入设备独占访问。

  可选。

vhost-user-gpu
----------------------------------------

命令行选项：

--render-node=PATH

  指定 GPU DRM 渲染节点。

  可选。

--virgl

  启用 virgl 渲染。

  可选。

vhost-user-blk
----------------------------------------

命令行选项：

--blk-file=PATH

  指定块设备或文件路径。

  可选。

--read-only

  启用只读。

  可选。
