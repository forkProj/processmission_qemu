使用 blkdebug 注入块 I/O 错误
=============================

..
   Copyright (C) 2014-2015 Red Hat Inc

   This work is licensed under the terms of the GNU GPL, version 2 or later.  See
   the COPYING file in the top-level directory.

``blkdebug`` 是基于规则的块驱动错误注入引擎，
可用于测试 ENOSPC（空间不足）、EIO 等错误处理路径。

本文概述其主要功能。

背景
--------

块驱动有许多处理 I/O 错误的代码路径。
镜像格式尤其复杂：分配簇或更新表时发生的元数据 I/O 错误，
可能出现在请求处理的中途，需要谨慎处理才能保持文件一致。

错误注入让测试在指定位置触发 I/O 错误，从而验证各条错误路径。

规则
--------

blkdebug 接受一组规则，决定何时让 I/O 请求失败。

每个请求都会与规则比较，匹配后执行相应动作。

规则可以保存在配置文件中，采用与 QEMU -readconfig 相同的类 INI 格式，
每个节定义一条规则。

以下配置定义一条规则：::

  $ cat blkdebug.conf
  [inject-error]
  event = "read_aio"
  errno = "28"


它使所有异步读请求以 ENOSPC（28）失败。
errno 数值取决于宿主，Linux 的取值见 ``/usr/include/asm-generic/errno-base.h``。

按以下方式运行 QEMU：::

  $ qemu-system-x86_64
        -drive if=none,cache=none,file=blkdebug:blkdebug.conf:test.img,id=drive0 \
        -device virtio-blk-pci,drive=drive0,id=virtio-blk-pci0


规则支持以下属性：

``event``
  要匹配的操作类型，例如 read_aio、write_aio、flush_to_os、flush_to_disk，详见 `事件`_。

``state``
  可选。引擎必须处于该状态才能匹配，详见 `状态转换`_。

``errno``
  匹配时返回的 errno 数值。POSIX 没有标准化这些数字，因此取决于宿主。

``sector``
  可选。请求必须覆盖指定扇区才能匹配。

``once``
  可选，默认 off。仅对首次匹配请求执行动作。

``immediately``
  可选，默认 off。返回空 BlockAIOCB 指针，不带 errno 失败，
  用于测试 BlockAIOCB 失败且不调用 BlockCompletionFunc 的路径。

事件
--------

块驱动会报告即将发出的 I/O 类型，便于规则精确匹配。
例如，qcow2 访问 L1 表时通知 blkdebug，
规则便能只匹配 L1 访问，而非其他元数据或客户机数据请求。

核心事件如下：

``read_aio``
  读取客户机数据。

``write_aio``
  写入客户机数据。

``flush_to_os``
  写出尚未持久化到操作系统的块驱动状态，例如缓存的元数据。

``flush_to_disk``
  刷新宿主块设备的磁盘缓存。

完整事件列表见 ``qapi/block-core.json:BlkdebugEvent``。
有时需要搜索块驱动源码，理解特定事件的含义。

状态转换
--------

有时需要从较长请求序列中匹配特定一次操作，例如：::

  write_aio
  flush_to_disk
  write_aio


若只匹配第二次 write_aio，就需要状态转换。

引擎内部维护整数 state，初始为 1。
外部无法直接观察它，但规则可以读写它，实现更强的匹配能力。

规则可以以当前状态为条件，并切换到新状态。

state 属性非零时，必须与当前状态相同才会匹配。

例如，匹配第二次 write_aio：::

  [set-state]
  event = "write_aio"
  state = "1"
  new_state = "2"

  [inject-error]
  event = "write_aio"
  state = "2"
  errno = "5"


第一次请求匹配 set-state，将状态从 1 改为 2。
之后该规则不再匹配，因为它要求状态 1。
下一次 write_aio 则匹配 inject-error，注入 EIO（5）。

状态转换规则支持：

``event``
  要匹配的操作类型，例如 read_aio、write_aio、flush_to_os、flush_to_disk，详见 `事件`_。

``state``
  可选。匹配所需的当前状态。

``new_state``
  要切换到的状态。

挂起和恢复
----------

测试某些块驱动路径，需要让并发请求按特定顺序执行。
断点功能可以在 blkdebug 事件处暂停请求，稍后恢复，
从而在多个在途请求之间实现确定性顺序。

断点关联用户定义的 tag 字符串，用于稍后识别并恢复请求。

详见 qemu-io(1) 的 break、resume、remove_break 和 wait_break 命令。
