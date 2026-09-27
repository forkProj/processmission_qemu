Rocker 网络交换机寄存器编程指南
************************************************

..
   Copyright (c) Scott Feldman <sfeldma@gmail.com>
   Copyright (c) Neil Horman <nhorman@tuxdriver.com>
   Version 0.11, 12/29/2014

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

简介
============

概述
--------

本文描述 Rocker 交换机设备的硬件／软件接口，面向操作系统驱动及设备仿真软件的开发者。

记号与约定
-------------------------

* 寄存器描述中的 [n:m] 表示从位 n 到位 m 的范围，包含两端。
* 前缀 0x 表示十六进制数。
* 前缀 0b 表示二进制数。
* RSVD 或 Reserved 表示该位或字段保留供将来使用。
* 除非另有说明，字段宽度以字节为单位。
* 寄存器属性为 (R) 只读、(R/W) 读写、(W) 只写或 (COR) 读清零。
* 网络字节序的 TLV 值以 (N) 标识。


PCI 配置寄存器
===========================

PCI 配置空间
-----------------------

每个交换机实例注册为一个 PCI 设备，其 PCI 配置空间如下：

::

    偏移    宽度    说明                    值
    ---------------------------------------------
    0x0     2       厂商 ID                 0x1b36
    0x2     2       设备 ID                 0x0006
    0x4     4       命令／状态
    0x8     1       修订版本 ID             0x01
    0x9     3       类代码                  0x2800
    0xC     1       缓存行大小
    0xD     1       延迟计时器
    0xE     1       头部类型
    0xF     1       内建自检
    0x10    4       基地址低部
    0x14    4       基地址高部
    0x18-28         保留
    0x2C    2       子系统厂商 ID           *
    0x2E    2       子系统 ID               *
    0x30-38         保留
    0x3C    1       中断线
    0x3D    1       中断引脚                0x00
    0x3E    1       最小授权时间            0x00
    0x3D    1       最大延迟                0x00
    0x40    1       TRDY 超时
    0x41    1       重试计数
    0x42    2       保留

    * 由子系统实现分配

内存映射寄存器空间
============================

设备有两个内存映射 BAR。BAR0 映射设备寄存器空间，大小为 0x2000。
BAR1 映射 MSI-X 向量表和 PBA 表，大小也为 0x2000，可容纳 256 个 MSI-X 向量。

所有寄存器均为 4 或 8 字节长。假定主机软件通过一次 4 字节访问操作访问 4 字节寄存器，
通过两次 4 字节访问或一次 8 字节访问操作访问 8 字节寄存器。使用两次 4 字节访问时，
必须先访问低 4 字节，再访问高 4 字节。

BAR0 设备寄存器空间布局如下：

::

    偏移            说明
    ------------------------------------------------------
    0x0000-0x000f    用于发现驱动异常行为的伪寄存器。
                    写入无效果，读取返回 0xDEADBABE。
    0x0010-0x00ff    测试寄存器
    0x0300-0x03ff    通用寄存器
    0x1000-0x1fff    描述符控制

寄存器空间中的空洞均为保留区域。写入保留寄存器无效果，读取保留寄存器返回 0。

所有寄存器均未启用写合并等特殊机制。

BAR1 MSI-X 寄存器空间布局如下：

::

    偏移            说明
    ------------------------------------------------------
    0x0000-0x0fff    MSI-X 向量表（共 256 个向量）
    0x1000-0x1fff    MSI-X PBA 表

中断、DMA 与字节序
===============================

PCI 中断
--------------

设备仅支持 MSI-X 中断。BAR1 内存映射区域包含 MSI-X 向量表和 PBA 表，
最多支持 256 个 MSI-X 向量。

向量分配如下：

::

    向量            说明
    -----------------------------------------------------
    0               命令描述符环完成
    1               事件描述符环完成
    2               测试操作完成
    3               RSVD
    4-255           Tx 和 Rx 描述符环完成
                      Tx 向量为偶数
                      Rx 向量为奇数

MSI-X 向量表条目大小为 16 字节：

::

    字段            偏移    宽度    说明
    -------------------------------------------------------------
    lower_addr      0x0     4       [31:2] 消息地址[31:2]
                                    [1:0] 保留（要求按 4 字节对齐）
    upper_addr      0x4     4       [31:19] 保留
                                    [14:0] 消息地址[46:32]
    data            0x8     4       消息数据[31:0]
    control         0xc     4       [31:1] 保留
                                    [0] 屏蔽（0 = 启用，1 = 屏蔽）

软件应在启用任何端口或向命令环发送任何命令之前，安装中断服务程序（ISR）。

DMA 操作
--------------

DMA 操作用于 CPU 收发数据包的 DMA，以及命令和事件处理。命令处理包括统计计数器和
表的转储、表项插入／删除等。事件处理为设备产生的事件提供异步通知方式。
每种 DMA 操作都有一组控制寄存器，用于管理描述符环。描述符环从主机可用于 DMA 的
连续内存中分配，寄存器指定环的基地址、大小及当前头尾索引。软件始终写入头索引，
硬件始终写入尾索引。

DMA_DESC_COMP_ERR 的最高位用于标记硬件已完成描述符处理。软件将描述符提交到环中时
清除此位，硬件完成描述符后置位。

描述符环大小必须为 2 的幂，范围为 2 至 64K 个条目。描述符环基地址必须按 8 字节对齐，
描述符必须在环中紧密排列。每个环中的各描述符也必须按 8 字节边界对齐。
每个描述符环具有以下寄存器：

::

    DMA_DESC_xxx_BASE_ADDR, 偏移 0x1000 + (x * 32), 64 位, (R/W)
    DMA_DESC_xxx_SIZE, 偏移 0x1008 + (x * 32), 32 位, (R/W)
    DMA_DESC_xxx_HEAD, 偏移 0x100c + (x * 32), 32 位, (R/W)
    DMA_DESC_xxx_TAIL, 偏移 0x1010 + (x * 32), 32 位, (R)
    DMA_DESC_xxx_CTRL, 偏移 0x1014 + (x * 32), 32 位, (W)
    DMA_DESC_xxx_CREDITS, 偏移 0x1018 + (x * 32), 32 位, (R/W)
    DMA_DESC_xxx_RSVD1, 偏移 0x101c + (x * 32), 32 位, (R/W)

其中 x 为描述符环索引：

::

    索引            环
    --------------------
    0               CMD
    1               EVENT
    2               TX（端口 0）
    3               RX（端口 0）
    4               TX（端口 1）
    5               RX（端口 1）
    .
    .
    .
    124             TX（端口 61）
    125             RX（端口 61）
    126             保留
    127             保留

写入 BASE_ADDR 或 SIZE 会将 HEAD 和 TAIL 复位为零。HEAD 不能写到 TAIL 之后，
否则会绕环覆盖。HEAD == TAIL 表示空环；HEAD 位于 TAIL 前一个位置时表示满环。
HEAD 和 TAIL 均递增，并按环大小取模回绕。

CTRL 寄存器位：

::

    位      名称            说明
    ------------------------------------------------------------------------
    [0]     CTRL_RESET      复位描述符环
    [1:31]  保留

所有描述符类型共有一些字段：

::

    字段                    宽度    说明
    -------------------------------------------------------------------
    DMA_DESC_BUF_ADDR       8       描述符载荷的物理地址，按 8 字节对齐
    DMA_DESC_COOKIE         8       用于匹配完成通知的描述符 cookie，最高位保留
    DMA_DESC_BUF_SIZE       2       描述符载荷大小，以字节为单位
    DMA_DESC_TLV_SIZE       2       描述符载荷中用于 TLV 的总大小，以字节为单位。
                                    必须 <= DMA_DESC_BUF_SIZE。
    DMA_DESC_COMP_ERR       2       关联描述符载荷的完成状态。新描述符的最高位
                                    清零，硬件在条目完成时翻转此位。

为支持前向和后向兼容，描述符及完成载荷采用 TLV 格式。字段以 Type=字段名、
Length=字段长度、Value=字段值的形式打包。软件忽略交换机填入的未知字段；
交换机同样忽略软件填入的未知字段。

描述符载荷缓冲区按 8 字节对齐，TLV 也按 8 字节对齐。TLV 内的值同样按 8 字节对齐。
紧密打包、总长 8 字节的 TLV 头如下：

::

    字段    宽度    说明
    -----------------------------
    type    4       TLV 类型
    len     2       TLV 值长度
    pad     2       保留

描述符和 TLV 的对齐要求旨在避免软件发生非对齐访问异常。注意，每个 TLV 的载荷
也按 8 字节对齐。

图 1 展示一个包含两个 TLV 的描述符缓冲区示例：

::

                    <------- 8 字节  ------->

    8字节   +––––+  +–––––––––––+–––––+–––––+                     +–+
    对齐            |   type    | len | pad |    TLV#1 头           |
                    +–––––––––––+–––––+–––––+    (len=22)           |
                    |                       |                       |
                    |  value                |    TVL#1 值           |
                    |                       |    (填充以实现8字节   |
                    |                 +–––––+     对齐)             |
                    |                 |/////|                       |
     8字节  +––––+  +–––––––––––+–––––––––––+                       |
     对齐           |   type    | len | pad |    TLV#2 头     DESC_BUF_SIZE
                    +–––––+–––––+–––––+–––––+    (len=2)            |
                    |value|/////////////////|    TLV#2 值           |
                    +–––––+/////////////////|                       |
                    |///////////////////////|                       |
                    |///////////////////////|                       |
                    |///////////////////////|                       |
                    |////////未用  /////////|                       |
                    |////////空间 //////////|                       |
                    |///////////////////////|                       |
                    |///////////////////////|                       |
                    |///////////////////////|                       |
                    +–––––––––––––––––––––––+                     +–+

                                  图 1

TLV 可以嵌套在 NEST 类型的 TLV 中。

中断信用值
^^^^^^^^^^^^^^^^^

用于描述符环完成通知的 MSI-X 向量采用信用机制，提高设备、PCIe 总线、操作系统及
驱动的效率。每个描述符环都有一个信用计数，表示等待驱动处理的未完成描述符数量。
设备标记描述符完成时增加信用计数；驱动处理这些待处理描述符时，将信用值返还设备。
设备由此获知驱动进度，并决定何时触发下一次中断，或是否触发中断。当信用计数为零，
且首次向驱动提交描述符时，触发一次中断。中断触发后即被禁用（自动屏蔽*）。
驱动响应中断，处理描述符，并通过 PIO 写入该描述符环的返还信用值。如果驱动返还
全部信用值（即已跟上设备进度，没有待处理工作），则解除中断屏蔽，但不触发中断。
如果只返还部分信用值，中断仍保持屏蔽，但设备会产生中断，通知驱动还有待处理工作。

（* 此屏蔽与 MSI-X 中断屏蔽寄存器无关。）

字节序
----------

设备寄存器固定为小端（LE）。驱动访问设备寄存器时，应在主机字节序与 LE 之间转换。

描述符采用 LE。描述符缓冲区中的 TLV，其 type 和 length 字段为 LE，value 字段则
根据上下文采用 LE 或网络字节序。包含网络数据包数据的 TLV 值采用网络字节序。
用于与网络数据包数据比较的字段或掩码，其 TLV 值也采用网络字节序。例如，流匹配字段
（及掩码）采用网络字节序，因为它们直接逐字节匹配网络数据包数据。所有非网络数据包
的 TLV 多字节值采用 LE。

网络字节序的 TLV 值以 (N) 标识。


测试寄存器
==============

Rocker 有多个测试寄存器，用于排查寄存器访问、中断生成和 DMA 操作的问题：

::

    TEST_REG, 偏移 0x0010, 32 位 (R/W)
    TEST_REG64, 偏移 0x0018, 64 位 (R/W)
    TEST_IRQ, 偏移 0x0020, 32 位 (R/W)
    TEST_DMA_ADDR, 偏移 0x0028, 64 位 (R/W)
    TEST_DMA_SIZE, 偏移 0x0030, 32 位 (R/W)
    TEST_DMA_CTRL, 偏移 0x0034, 32 位 (R/W)

读取 TEST_REG 和 TEST_REG64 时，返回值为上一次写入该寄存器值的两倍。
32 位和 64 位版本分别用于测试主机的 32 位和 64 位访问。

可将向量写入 TEST_IRQ，设备将产生该向量的中断。

测试基本 DMA 操作时，分配主机可用于 DMA 的缓冲区，将缓冲区地址写入 TEST_DMA_ADDR，
大小写入 TEST_DMA_SIZE。然后写入 TEST_DMA_CTRL 操作缓冲区内容。
TEST_DMA_CTRL 操作如下：

::

    操作                    值      说明
    -----------------------------------------------------------
    TEST_DMA_CTRL_CLEAR     1       清空缓冲区
    TEST_DMA_CTRL_FILL      2       用 0x96 填充缓冲区各字节
    TEST_DMA_CTRL_INVERT    4       对缓冲区各字节取反

应测试多种缓冲区地址和大小，确认不存在地址边界问题。尤其应测试起始于奇数
8 字节边界和／或跨越多个 PAGE 大小的缓冲区。


端口
=====

物理端口与逻辑端口
------------------------------------

交换机最多支持 62 个物理（前面板）端口。寄存器 PORT_PHYS_COUNT 返回实际可用的
物理端口数量：

::

    PORT_PHYS_COUNT, 偏移 0x0304, 32 位, (R)

除前面板端口外，交换机还支持用于隧道的逻辑端口。

前面板端口和逻辑隧道端口映射到统一的 32 位端口空间。特殊 CPU 端口分配为端口 0，
前面板端口映射为端口 1-62，特殊回环端口分配为端口 63，逻辑隧道端口分配为
端口 0x0001000-0x0001ffff。端口分配概括如下：

::

    端口                    映射
    -------------------------------------------------------
    0                       CPU 端口（供主机 CPU 收发数据包）
    1-62                    前面板物理端口
    63                      回环端口
    64-0x0000ffff            RSVD
    0x00010000-0x0001ffff     逻辑隧道端口
    0x00020000-0xffffffff     RSVD

物理端口模式
------------------

交换机前面板端口以某种模式运行。目前唯一的模式是 OF-DPA。OF-DPA[1] 模式基于
Broadcom Corporation 的 OpenFlow Data Plane Abstraction（OF-DPA）抽象交换机规范
1.0 版。前面板端口模式的设置和获取方法见下文端口设置。

端口设置
-------------

所有前面板端口的链路状态可通过 PORT_PHYS_LINK_STATUS 获取：

::

    PORT_PHYS_LINK_STATUS, 偏移 0x0310, 64 位, (R)

    值为端口位图。位 0 和 63 始终读为 0。位 1-62 对应前面板各端口，
    链路启用时读为 1，链路断开时读为 0。

前面板端口的其他属性可通过 DMA CMD 描述符获取：

::

    获取 PORT_SETTINGS 描述符：

            字段            宽度    说明
            ----------------------------------------------
            PORT_SETTINGS   2       CMD_GET
            PPORT           4       物理端口编号

    获取 PORT_SETTINGS 的完成结果：

            字段            宽度    说明
            ----------------------------------------------
            PPORT           4       物理端口编号
            SPEED           4       当前端口接口速率，单位 Mbps
            DUPLEX          1       1 = 全双工，0 = 半双工
            AUTONEG         1       1 = 启用，0 = 禁用
            MACADDR         6       端口 MAC 地址
            MODE            1       0 = OF-DPA
            LEARNING        1       端口 MAC 地址学习
                                      1 = 启用
                                      0 = 禁用
            PHYS_NAME       <var>   物理端口名称（字符串）

    设置 PORT_SETTINGS 描述符：

            字段            宽度    说明
            ----------------------------------------------
            PORT_SETTINGS   2       CMD_SET
            PPORT           4       物理端口编号
            SPEED           4       端口接口速率，单位 Mbps
            DUPLEX          1       1 = 全双工，0 = 半双工
            AUTONEG         1       1 = 启用，0 = 禁用
            MACADDR         6       端口 MAC 地址
            MODE            1       0 = OF-DPA

端口启用
-----------

前面板端口最初处于禁用状态，意味着端口入站和出站数据包均被丢弃。
使用 PORT_PHYS_ENABLE 启用或禁用端口：

::

    PORT_PHYS_ENABLE: 偏移 0x0318, 64 位, (R/W)

    值为前 64 个端口的位图。忽略位 0 和 63，它们始终读为 0。
    写入 1 启用端口；写入 0 禁用端口。默认值为 0。

交换机控制
==============

本节介绍影响整个交换机的寄存器设置。

控制
-------

此寄存器用于交换机的底层控制：

::

    CONTROL: 偏移 0x0300, 32 位, (W)

    位      名称            说明
    ------------------------------------------------------------------------
    [0]     CONTROL_RESET   置位时，设备执行复位
    [1:31]  保留

交换机 ID
---------

交换机具有 SWITCH_ID，供软件唯一标识该交换机：

::

    SWITCH_ID: 偏移 0x0320, 64 位, (R)

    此值对交换机软件是不透明的，不隐含任何特殊编码。

事件
======

设备产生的非 I/O 异步事件通过事件环通知主机。事件的 TLV 结构如下：

::

    字段            宽度    说明
    ---------------------------------------------------
    TYPE            4       事件类型，取以下值之一：
                              1: LINK_CHANGED
                              2: MAC_VLAN_SEEN
    INFO            <nest>  事件信息（详情见下文）

链路变化事件
------------------

物理端口链路状态发生变化时，生成此事件：

::

    字段            宽度    说明
    ---------------------------------------------------
    INFO            <nest>
      PPORT         4       物理端口
      LINKUP        1       链路状态：
                              0: 断开
                              1: 启用

发现 MAC VLAN 事件
-------------------

当数据包进入端口，而设备尚不认识其源 MAC/VLAN 时，设备生成此事件。驱动响应事件，
应将该端口上的 MAC/VLAN 安装到设备的桥接表中。安装后，该端口上的 MAC/VLAN
即为已知，此事件不再生成。

::

    字段            宽度    说明
    ---------------------------------------------------
    INFO            <nest>
      PPORT         4       物理端口
      MAC           6       MAC 地址
      VLAN          2       VLAN ID

CPU 数据包处理
=====================

发往主机 CPU 以作进一步处理的入站数据包，通过 DMA RX 环投递。同样，主机 CPU
产生且需要从交换机端口发出的数据包，由软件通过 DMA TX 环调度。

Tx 数据包处理
--------------------

软件通过 DMA TX 环调度要从交换机端口发出的数据包。TX 描述符缓冲区描述数据包在
主机可用于 DMA 的内存中的位置与大小、目标端口，以及所需硬件卸载功能
（例如 L3 载荷校验和卸载）。软件随后推进描述符头索引，通知硬件有新的 Tx 工作。
硬件响应后，通过 DMA 读取直至头索引的 Tx 描述符，再通过 DMA 读取描述符缓冲区和
数据包数据，执行卸载功能，最后将数据包成帧发送到线路（网络）上。数据包处理完成后，
硬件将状态写回描述符，通知软件 Tx 已完成，可以释放承载数据包的软件资源（例如 skb）。

图 2 展示一个用单个 Tx 描述符排队、包含 3 个片段的数据包示例。每个片段使用一个 TLV：

::

                                               包片段 1
                                               +–––––––+  +–+
                                           +–––+       |    |
                             描述符区      |   |       |    |
                            +––––––––+     |   |       |    |
            Tx 环       +–––+        +–––––+   |       |    |
          +–––––––––+   |   |  TLVs  |         +–––––––+    |
          |         +–––+   +––––––––+         包片段 2     |
          | desc 0  |       |        +–––––+   +–––––––+    |
          +–––––––––+       |  TLVs  |     +–––+       |    |
    head+–+         |       +––––––––+         |       |    |
          | desc 1  |       |        +–––––+   +–––––––+    |pkt
          +–––––––––+       |  TLVs  |     |                |
          |         |       +––––––––+     |   包片段 3     |
          |         |                      |   +–––––––+    |
          +–––––––––+                      +–––+       |    |
          |         |                          |       |    |
          |         |                          |       |    |
          +–––––––––+                          |       |    |
          |         |                          |       |    |
          |         |                          |       |    |
          +–––––––––+                          |       |    |
          |         |                          +–––––––+  +–+
          |         |
          +–––––––––+

                            图 2.

Tx 描述符缓冲区的 TLV 如下：

::

    字段                    宽度    说明
    ---------------------------------------------------------------------
    PPORT                   4       目标物理端口编号
    TX_OFFLOAD              1       硬件卸载模式：
                                      0: 不卸载
                                      1: 插入 IP 校验和（仅 ipv4）
                                      2: 插入 TCP/UDP 校验和
                                      3: 计算 L3 校验和并插入到校验和偏移
                                         (TX_L3_CSUM_OFF)。采用 16 位 1 的补码（反码）
                                         校验和值。IPv4 伪头部和 IP 校验和
                                         已由操作系统计算并插入。
                                      4: TSO（TCP 分段卸载）
    TX_L3_CSUM_OFF          2       L3 校验和卸载模式下，L3 头部中校验和
                                    字段相对于数据包起点的偏移
    TX_TSO_MSS              2       TSO 卸载模式下的最大分段大小，单位字节
    TX_TSO_HDR_LEN          2       TSO 卸载模式下，以太网、IP 及 TCP/UDP
                                    头部长度，包括 IP 和 TCP 选项
    TX_FRAGS                <array> 数据包片段
      TX_FRAG               <nest>  数据包片段
        TX_FRAG_ADDR        8       数据包片段的 DMA 地址
        TX_FRAG_LEN         2       数据包片段长度

完成时描述符可能返回的状态码如下：

::

    DESC_COMP_ERR   原因
    --------------------------------------------------------------------
    0               成功
    -ROCKER_ENXIO   描述符缓冲区或数据包片段的地址／数据读取错误
    -ROCKER_EINVAL  pport 无效，或 TSO／校验和卸载错误
    -ROCKER_ENOMEM  没有内存用于内部暂存 tx 片段

Rx 数据包处理
--------------------

从交换机端口进入、未由交换机转发而是交给主机 CPU 进一步处理的数据包，通过 DMA RX
环投递。Rx 描述符缓冲区由软件分配并放入环中。硬件将数据包数据填入 Rx 描述符缓冲区，
写入完成状态，并通知软件新数据包已就绪。由于无法预先知道 Rx 数据包大小，Rx 描述符
缓冲区必须按最坏情况下的数据包大小分配。单个 Rx 描述符在一个 RX_FRAG 中包含完整的
Rx 数据包数据。其他 Rx TLV 描述对数据包执行的硬件卸载，例如校验和验证。

Rx 描述符缓冲区的 TLV 如下：

::

    字段            宽度    说明
    ---------------------------------------------------
    PPORT           4       源物理端口编号
    RX_FLAGS        2       数据包解析标志：
                              (1 << 0): IPv4 数据包
                              (1 << 1): IPv6 数据包
                              (1 << 2): 已计算校验和
                              (1 << 3): IPv4 校验和正确
                              (1 << 4): IP 分片
                              (1 << 5): TCP 数据包
                              (1 << 6): UDP 数据包
                              (1 << 7): TCP/UDP 校验和正确
                              (1 << 8): 卸载转发
    RX_CSUM         2       计算出的 IP 校验和：
                              IPv4: IP 载荷校验和
                              IPv6: 头部和载荷校验和
                            （仅在 RX_FLAGS 的校验和已计算标志置位时有效）
    RX_FRAG_ADDR    8       数据包片段的 DMA 地址
    RX_FRAG_MAX_LEN 2       数据包片段的最大长度
    RX_FRAG_LEN     2       接收后数据包片段的实际长度

Offload forward RX_FLAG 表示设备已经转发数据包，因此主机 CPU 不应再次转发。

完成时描述符可能返回的状态码如下：

::

    DESC_COMP_ERR   原因
    --------------------------------------------------------------------
    0               成功
    -ROCKER_ENXIO   描述符缓冲区的地址／数据读取错误
    -ROCKER_ENOMEM  没有内存用于内部暂存描述符缓冲区
    -ROCKER_EMSGSIZE Rx 描述符缓冲区不足以容纳数据包数据 TLV 及其他 TLV。

OF-DPA 模式
===========

OF-DPA 模式允许交换机将流数据包处理功能卸载到硬件。OpenFlow 控制器与安装在交换机上
的 OpenFlow 代理通信。OpenFlow 代理直接或间接地与 Rocker 交换机驱动通信，驱动再将
OF-DPA 定义的流功能编程到交换机硬件中。框图如下：

::

    +–––––––––––––––----–––+
    |        OF            |
    |  远程控制器          |
    +––––––––+––----–––––––+
             |
             |
    +––––––––+–––––––––+
    |       OF         |
    |   本地代理       |
    +––––––––––––––––––+
    |                  |
    |   Rocker 驱动    |
    +––––––––––––––––––+
        <本规范>
    +––––––––––––––––––+
    |                  |
    |   Rocker 交换机  |
    +––––––––––––––––––+

要参与流处理功能，端口必须在交换机初始化期间配置为 OF-DPA 模式。

OF-DPA 流表接口
---------------------------

提供添加、修改、删除流表条目及获取统计信息的命令。命令通过 DMA CMD 描述符环发出。
定义了以下命令：

::

    CMD_ADD:                向流表添加条目
    CMD_MOD:                修改流表条目
    CMD_DEL:                从流表删除条目
    CMD_GET_STATS:          获取流条目的统计信息

添加和修改命令的 TLV 如下：

::

    字段                    宽度    说明
    ----------------------------------------------------
    OF_DPA_CMD              2       CMD_[ADD|MOD]
    OF_DPA_TBL              2       流表 ID
                                      0: 入口端口
                                      10: vlan
                                      20: 终结 mac
                                      30: 单播路由
                                      40: 组播路由
                                      50: 桥接
                                      60: ACL 策略
    OF_DPA_PRIORITY         4       流优先级
    OF_DPA_HARDTIME         4       流的硬超时
    OF_DPA_IDLETIME         4       流的空闲超时
    OF_DPA_COOKIE           8       Cookie

根据流表 ID，还需要以下额外 TLV：

表 ID 0：入口端口：

::

    字段                    宽度    说明
    ----------------------------------------------------
    OF_DPA_IN_PPORT         4       入口物理端口编号
    OF_DPA_GOTO_TBL         2       跳转表 ID；零表示丢弃

表 ID 10：vlan：

::

    字段                    宽度    说明
    ----------------------------------------------------
    OF_DPA_IN_PPORT         4       入口物理端口编号
    OF_DPA_VLAN_ID          2 (N)   vlan ID
    OF_DPA_VLAN_ID_MASK     2 (N)   vlan ID 掩码
    OF_DPA_GOTO_TBL         2       跳转表 ID；零表示丢弃
    OF_DPA_NEW_VLAN_ID      2 (N)   新 vlan ID

表 ID 20：终结 mac：

::

    字段                    宽度    说明
    ----------------------------------------------------
    OF_DPA_IN_PPORT         4       入口物理端口编号
    OF_DPA_IN_PPORT_MASK    4       入口物理端口编号掩码
    OF_DPA_ETHERTYPE        2 (N)   必须为 0x0800 或 0x86dd
    OF_DPA_DST_MAC          6 (N)   目标 MAC
    OF_DPA_DST_MAC_MASK     6 (N)   目标 MAC 掩码
    OF_DPA_VLAN_ID          2 (N)   vlan ID
    OF_DPA_VLAN_ID_MASK     2 (N)   vlan ID 掩码
    OF_DPA_GOTO_TBL         2       仅接受单播或组播路由表 ID
    OF_DPA_OUT_PPORT        2       如指定，必须为 controller，否则置零

表 ID 30：单播路由：

::

    字段                    宽度    说明
    ----------------------------------------------------
    OF_DPA_ETHERTYPE        2 (N)   必须为 0x0800 或 0x86dd
    OF_DPA_DST_IP           4 (N)   目标 IPv4 地址，必须为单播地址
    OF_DPA_DST_IP_MASK      4 (N)   IP 掩码，必须为前缀掩码
    OF_DPA_DST_IPV6         16 (N)  目标 IPv6 地址，必须为单播地址
    OF_DPA_DST_IPV6_MASK    16 (N)  IPv6 掩码，必须为前缀掩码
    OF_DPA_GOTO_TBL         2       跳转表 ID；零表示丢弃
    OF_DPA_GROUP_ID         4       GROUP 动作的数据必须为 L3 单播组条目

表 ID 40：组播路由：

::

    字段                    宽度    说明
    ----------------------------------------------------
    OF_DPA_ETHERTYPE        2 (N)   必须为 0x0800 或 0x86dd
    OF_DPA_VLAN_ID          2 (N)   vlan ID
    OF_DPA_SRC_IP           4 (N)   源 IPv4，可选。可包含 IPv4 地址，
                                    不使用时必须完全屏蔽
    OF_DPA_SRC_IP_MASK      4 (N)   IP 掩码
    OF_DPA_DST_IP           4 (N)   目标 IPv4 地址，必须为组播地址
    OF_DPA_SRC_IPV6         16 (N)  源 IPv6 地址，可选。可包含 IPv6 地址，
                                    不使用时必须完全屏蔽
    OF_DPA_SRC_IPV6_MASK    16 (N)  IPv6 掩码
    OF_DPA_DST_IPV6         16 (N)  目标 IPv6 地址，必须为组播地址
                                    必须为组播地址
    OF_DPA_GOTO_TBL         2       跳转表 ID；零表示丢弃
    OF_DPA_GROUP_ID         4       GROUP 动作的数据必须为 L3 组播组条目

表 ID 50：桥接：

::

    字段                    宽度    说明
    ----------------------------------------------------
    OF_DPA_VLAN_ID          2 (N)   vlan ID
    OF_DPA_TUNNEL_ID        4       隧道 ID
    OF_DPA_DST_MAC          6 (N)   目标 MAC
    OF_DPA_DST_MAC_MASK     6 (N)   目标 MAC 掩码
    OF_DPA_GOTO_TBL         2       跳转表 ID；零表示丢弃
    OF_DPA_GROUP_ID         4       GROUP 动作的数据必须按需选用 L2 接口、
                                    L2 组播、L2 泛洪或 L2 覆盖组条目
    OF_DPA_TUNNEL_LPORT     4       单播租户桥接流指定隧道逻辑端口 ID
    OF_DPA_OUT_PPORT        2       OUTPUT 动作的数据，仅允许 CONTROLLER，
                                    否则置为 0

表 ID 60：ACL 策略：

::

    字段                    宽度    说明
    ----------------------------------------------------
    OF_DPA_IN_PPORT         4       入口物理端口编号
    OF_DPA_IN_PPORT_MASK    4       入口物理端口编号掩码
    OF_DPA_ETHERTYPE        2 (N)   以太网类型
    OF_DPA_VLAN_ID          2 (N)   vlan ID
    OF_DPA_VLAN_ID_MASK     2 (N)   vlan ID 掩码
    OF_DPA_VLAN_PCP         2 (N)   vlan 优先级代码点
    OF_DPA_VLAN_PCP_MASK    2 (N)   vlan 优先级代码点掩码
    OF_DPA_SRC_MAC          6 (N)   源 MAC
    OF_DPA_SRC_MAC_MASK     6 (N)   源 MAC 掩码
    OF_DPA_DST_MAC          6 (N)   目标 MAC
    OF_DPA_DST_MAC_MASK     6 (N)   目标 MAC 掩码
    OF_DPA_TUNNEL_ID        4       隧道 ID
    OF_DPA_SRC_IP           4 (N)   源 IPv4，可选。可包含 IPv4 地址，
                                    不使用时必须完全屏蔽
    OF_DPA_SRC_IP_MASK      4 (N)   IP 掩码
    OF_DPA_DST_IP           4 (N)   目标 IPv4 地址，必须为组播地址
    OF_DPA_DST_IP_MASK      4 (N)   IP 掩码
    OF_DPA_SRC_IPV6         16 (N)  源 IPv6 地址，可选。可包含 IPv6 地址，
                                    不使用时必须完全屏蔽
    OF_DPA_SRC_IPV6_MASK    16 (N)  IPv6 掩码
    OF_DPA_DST_IPV6         16 (N)  目标 IPv6 地址，必须为组播地址
    OF_DPA_DST_IPV6_MASK    16 (N)  IPv6 掩码
    OF_DPA_SRC_ARP_IP       4 (N)   ARP 载荷中的源 IPv4 地址，
                                    仅在 ethertype == 0x0806 时使用。
    OF_DPA_SRC_ARP_IP_MASK  4 (N)   IP 掩码
    OF_DPA_IP_PROTO         1       IP 协议
    OF_DPA_IP_PROTO_MASK    1       IP 协议掩码
    OF_DPA_IP_DSCP          1       DSCP
    OF_DPA_IP_DSCP_MASK     1       DSCP 掩码
    OF_DPA_IP_ECN           1       ECN
    OF_DPA_IP_ECN_MASK      1       ECN 掩码
    OF_DPA_L4_SRC_PORT      2 (N)   L4 源端口，仅用于 TCP、UDP 或 SCTP
    OF_DPA_L4_SRC_PORT_MASK 2 (N)   L4 源端口掩码
    OF_DPA_L4_DST_PORT      2 (N)   L4 源端口，仅用于 TCP、UDP 或 SCTP
    OF_DPA_L4_DST_PORT_MASK 2 (N)   L4 源端口掩码
    OF_DPA_ICMP_TYPE        1       ICMP 类型，仅在 IP 协议为 1 时使用
    OF_DPA_ICMP_TYPE_MASK   1       ICMP 类型掩码
    OF_DPA_ICMP_CODE        1       ICMP 代码
    OF_DPA_ICMP_CODE_MASK   1       ICMP 代码掩码
    OF_DPA_IPV6_LABEL       4 (N)   IPv6 流标签
    OF_DPA_IPV6_LABEL_MASK  4 (N)   IPv6 流标签掩码
    OF_DPA_GROUP_ID         4       GROUP 动作的数据
    OF_DPA_QUEUE_ID_ACTION  1       写入队列 ID
    OF_DPA_NEW_QUEUE_ID     1       队列 ID
    OF_DPA_VLAN_PCP_ACTION  1       写入 VLAN 优先级
    OF_DPA_NEW_VLAN_PCP     1       VLAN 优先级
    OF_DPA_IP_DSCP_ACTION   1       写入 DSCP
    OF_DPA_NEW_IP_DSCP      1       新 DSCP
    OF_DPA_TUNNEL_LPORT     4       限定为有效的隧道逻辑端口，否则置为 0。
    OF_DPA_OUT_PPORT        2       OUTPUT 动作的数据，仅允许 CONTROLLER，
                                    否则置为 0
    OF_DPA_CLEAR_ACTIONS    4       若为 1，则丢弃匹配流的数据包
                                    （忽略所有其他指令）

流删除和统计信息获取命令的 TLV 如下：

::

    字段                    宽度    说明
    ---------------------------------------------------
    OF_DPA_CMD              2       CMD_[DEL|GET_STATS]
    OF_DPA_COOKIE           8       Cookie

统计信息获取命令完成后，将以下 TLV 写回描述符缓冲区：

::

    字段                    宽度    说明
    ---------------------------------------------------
    OF_DPA_STAT_DURATION    4       流持续时间
    OF_DPA_STAT_RX_PKTS     8       接收数据包数
    OF_DPA_STAT_TX_PKTS     8       发送数据包数

完成时描述符可能返回的状态码如下：

::

    DESC_COMP_ERR   命令                    原因
    --------------------------------------------------------------------
    0               全部                    成功
    -ROCKER_EFAULT  全部                    头或尾索引超出环范围
    -ROCKER_ENXIO   全部                    描述符缓冲区的地址／数据读取错误
    -ROCKER_EMSGSIZE GET_STATS              命令描述符缓冲区不足以容纳写回 TLV
    -ROCKER_EINVAL  全部                    传入参数无效
    -ROCKER_EEXIST  ADD                     条目已存在
    -ROCKER_ENOSPC  ADD                     流表没有剩余空间
    -ROCKER_ENOENT  MOD|DEL|GET_STATS       cookie 无效

组表接口
---------------------

提供添加、修改、删除组表条目及获取统计信息的命令。命令通过 DMA CMD 描述符环发出。
定义了以下命令：

::

    CMD_ADD:                向组表添加条目
    CMD_MOD:                修改组表条目
    CMD_DEL:                从组表删除条目
    CMD_GET_STATS:          获取组条目的统计信息

添加和修改命令的 TLV 如下：

::

    字段                    宽度    说明
    -----------------------------------------------------------
    FLOW_GROUP_CMD          2       CMD_[ADD|MOD]
    FLOW_GROUP_ID           2       流组 ID
    FLOW_GROUP_TYPE         1       组类型：
                                      0: L2 接口
                                      1: L2 重写
                                      2: L3 单播
                                      3: L2 组播
                                      4: L2 泛洪
                                      5: L3 接口
                                      6: L3 组播
                                      7: L3 ECMP
                                      8: L2 覆盖
    FLOW_VLAN_ID            2       Vlan ID（类型 0、3、4、6）
    FLOW_L2_PORT            2       端口（类型 0）
    FLOW_INDEX              4       索引（除 0 之外的所有类型）
    FLOW_OVERLAY_TYPE       1       覆盖子类型（类型 8）：
                                      0: 泛洪单播隧道
                                      1: 泛洪组播隧道
                                      2: 组播单播隧道
                                      3: 组播组播隧道
    FLOW_GROUP_ACTION               nest
      FLOW_GROUP_ID         2       链中的下一个组 ID（除 0 之外的所有类型）
      FLOW_OUT_PORT         4       出口端口（类型 0、8）
      FLOW_POP_VLAN_TAG     1       剥离外层 VLAN 标签（仅类型 1）
      FLOW_VLAN_ID          2       （类型 1、5）
      FLOW_SRC_MAC          6       （类型 1、2、5）
      FLOW_DST_MAC          6       （类型 1、2）

流删除和统计信息获取命令的 TLV 如下：

::

    字段                    宽度    说明
    -----------------------------------------------------------
    FLOW_GROUP_CMD          2       CMD_[DEL|GET_STATS]
    FLOW_GROUP_ID           2       流组 ID

统计信息获取命令完成后，将以下 TLV 写回描述符缓冲区：

::

    字段                    宽度    说明
    ---------------------------------------------------
    FLOW_GROUP_ID           2       流组 ID
    FLOW_STAT_DURATION      4       流持续时间
    FLOW_STAT_REF_COUNT     4       流引用计数
    FLOW_STAT_BUCKET_COUNT  4       流桶计数

完成时描述符可能返回的状态码如下：

::

    DESC_COMP_ERR   命令                    原因
    --------------------------------------------------------------------
    0               全部                    成功
    -ROCKER_EFAULT  全部                    头或尾索引超出环范围
    -ROCKER_ENXIO   全部                    描述符缓冲区的地址／数据读取错误
    -ROCKER_ENOSPC  GET_STATS               命令描述符缓冲区不足以容纳写回 TLV
    -ROCKER_EINVAL  ADD|MOD                 传入参数无效
    -ROCKER_EEXIST  ADD                     条目已存在
    -ROCKER_ENOSPC  ADD                     流表没有剩余空间
    -ROCKER_ENOENT  MOD|DEL|GET_STATS       组 ID 无效
    -ROCKER_EBUSY   DEL                     组引用计数非零
    -ROCKER_ENODEV  ADD                     下一个组 ID 不存在

参考资料
==========

[1] OpenFlow Data Plane Abstraction（OF-DPA）抽象交换机规范，1.0 版，
Broadcom Corporation，2014 年 2 月 21 日。
