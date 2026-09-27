==================================
Vhost-user-gpu 协议
==================================

..
  Licence: This work is licensed under the terms of the GNU GPL,
           version 2 or later. See the COPYING file in the top-level
           directory.

.. contents:: 目录

简介
============

vhost-user-gpu 将 virtio-gpu 的渲染结果从后端进程共享到 QEMU 等前端。
若将 QEMU 视为显示服务器、后端视为客户端，它类似一个功能受限的显示服务器协议。
通常先设置扫描输出／显示配置，再发刷新事件更新显示，也支持光标形状与位置更新。

协议通过 UNIX 域流套接字传输，用辅助数据共享已打开的 DMABUF 或共享内存描述符。
套接字通常通过 ``VHOST_USER_GPU_SET_SOCKET`` 获取。

后端发送请求，前端发送可选回复。

线路格式
============

除非另有说明，数值均采用机器原生字节序。

请求和回复均包含三个头字段及载荷。

+---------+-------+------+---------+
| request | flags | size | payload |
+---------+-------+------+---------+

消息头
------------

:request: ``u32``，请求类型

:flags: ``u32``，32 位位域：

 - 位 2 是回复标志，每条回复都需设置

:size: ``u32``，载荷大小

载荷类型
-------------

根据请求类型，载荷可为：

VhostUserGpuCursorPos
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+------------+---+---+
| scanout-id | x | y |
+------------+---+---+

:scanout-id: ``u32``，光标所在的扫描输出

:x/y: ``u32``，光标位置

VhostUserGpuCursorUpdate
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+-----+-------+-------+--------+
| pos | hot_x | hot_y | cursor |
+-----+-------+-------+--------+

:pos: ``VhostUserGpuCursorPos``，光标位置

:hot_x/hot_y: ``u32``，光标热点位置

:cursor: ``[u32; 64 * 64]``，64×64 RGBA 光标数据，PIXMAN_a8r8g8b8 格式

VhostUserGpuScanout
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+------------+---+---+
| scanout-id | w | h |
+------------+---+---+

:scanout-id: ``u32``，要设置的扫描输出配置

:w/h: ``u32``，扫描输出宽／高

VhostUserGpuUpdate
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+------------+---+---+---+---+------+
| scanout-id | x | y | w | h | data |
+------------+---+---+---+---+------+

:scanout-id: ``u32``，要更新的扫描输出

:x/y/w/h: ``u32``，更新区域

:data: RGB 数据，PIXMAN_x8r8g8b8 格式

VhostUserGpuDMABUFScanout
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+------------+---+---+---+---+-----+-----+--------+-------+--------+
| scanout-id | x | y | w | h | fdw | fwh | stride | flags | fourcc |
+------------+---+---+---+---+-----+-----+--------+-------+--------+

:scanout-id: ``u32``，要设置的扫描输出配置

:x/y: ``u32``，扫描输出在 DMABUF 内的位置

:w/h: ``u32``，扫描输出宽／高

:fdw/fdh/stride/flags: ``u32``，DMABUF 宽／高／步长／标志

:fourcc: ``i32``，DMABUF fourcc


VhostUserGpuEdidRequest
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+------------+
| scanout-id |
+------------+

:scanout-id: ``u32``，获取 EDID 的扫描输出


VhostUserGpuDMABUFScanout2
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+----------------+----------+
| dmabuf_scanout | modifier |
+----------------+----------+

:dmabuf_scanout: ``VhostUserGpuDMABUFScanout``，按该结构的说明填充。

:modifier: ``u64``，DMABUF 修饰符


C 结构
------------

QEMU 中消息由以下结构实现：

.. code:: c

  typedef struct VhostUserGpuMsg {
      uint32_t request; /* VhostUserGpuRequest */
      uint32_t flags;
      uint32_t size; /* the following payload size */
      union {
          VhostUserGpuCursorPos cursor_pos;
          VhostUserGpuCursorUpdate cursor_update;
          VhostUserGpuScanout scanout;
          VhostUserGpuUpdate update;
          VhostUserGpuDMABUFScanout dmabuf_scanout;
          VhostUserGpuEdidRequest edid_req;
          struct virtio_gpu_resp_edid resp_edid;
          struct virtio_gpu_resp_display_info display_info;
          uint64_t u64;
      } payload;
  } QEMU_PACKED VhostUserGpuMsg;

协议功能
-----------------

.. code:: c

  #define VHOST_USER_GPU_PROTOCOL_F_EDID    0
  #define VHOST_USER_GPU_PROTOCOL_F_DMABUF2 1

通过 ``VHOST_USER_GPU_GET_PROTOCOL_FEATURES`` 和
``VHOST_USER_GPU_SET_PROTOCOL_FEATURES`` 协商新消息与通信变更。

通信
=============

消息类型
-------------

``VHOST_USER_GPU_GET_PROTOCOL_FEATURES``
  :id: 1
  :请求载荷: 不适用
  :回复载荷: ``u64``

  获取支持的协议功能掩码。

``VHOST_USER_GPU_SET_PROTOCOL_FEATURES``
  :id: 2
  :请求载荷: ``u64``
  :回复载荷: 不适用

  通过位掩码启用协议功能。

``VHOST_USER_GPU_GET_DISPLAY_INFO``
  :id: 3
  :请求载荷: 不适用
  :回复载荷: ``struct virtio_gpu_resp_display_info`` （来自 virtio 规范）

  获取首选显示配置。

``VHOST_USER_GPU_CURSOR_POS``
  :id: 4
  :请求载荷: ``VhostUserGpuCursorPos``
  :回复载荷: 不适用

  设置／显示光标位置。

``VHOST_USER_GPU_CURSOR_POS_HIDE``
  :id: 5
  :请求载荷: ``VhostUserGpuCursorPos``
  :回复载荷: 不适用

  设置／隐藏光标。

``VHOST_USER_GPU_CURSOR_UPDATE``
  :id: 6
  :请求载荷: ``VhostUserGpuCursorUpdate``
  :回复载荷: 不适用

  更新光标形状和位置。

``VHOST_USER_GPU_SCANOUT``
  :id: 7
  :请求载荷: ``VhostUserGpuScanout``
  :回复载荷: 不适用

  设置扫描输出分辨率，宽高设为 0 可禁用。

``VHOST_USER_GPU_UPDATE``
  :id: 8
  :请求载荷: ``VhostUserGpuUpdate``
  :回复载荷: 不适用

  更新扫描输出内容，载荷包含图像位数据，应刷新并呈现显示。

``VHOST_USER_GPU_DMABUF_SCANOUT``
  :id: 9
  :请求载荷: ``VhostUserGpuDMABUFScanout``
  :回复载荷: 不适用

  设置扫描输出分辨率／配置，并通过辅助数据共享包含内容的 DMABUF 描述符。
  禁用时宽高设为 0，不传描述符。

``VHOST_USER_GPU_DMABUF_UPDATE``
  :id: 10
  :请求载荷: ``VhostUserGpuUpdate``
  :回复载荷: 空载荷

  应按 ``VhostUserGpuUpdate`` 的更新区域刷新并呈现显示。

  注意：没有数据载荷，因为内容通过 DMABUF 共享，
  必须预先使用 ``VHOST_USER_GPU_DMABUF_SCANOUT`` 设置。

``VHOST_USER_GPU_GET_EDID``
  :id: 11
  :请求载荷: ``struct VhostUserGpuEdidRequest``
  :回复载荷: ``struct virtio_gpu_resp_edid`` （来自 virtio 规范）

  获取指定扫描输出的 EDID，要求支持 ``VHOST_USER_GPU_PROTOCOL_F_EDID``。

``VHOST_USER_GPU_DMABUF_SCANOUT2``
  :id: 12
  :请求载荷: ``VhostUserGpuDMABUFScanout2``
  :回复载荷: 不适用

  与 VHOST_USER_GPU_DMABUF_SCANOUT 相同，但末尾额外传入前者未提供的 DMABUF 修饰符。
  要求支持 ``VHOST_USER_GPU_PROTOCOL_F_DMABUF2``。
