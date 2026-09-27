使用多个 ``IOThread``
=====================

..
   Copyright (c) 2014-2017 Red Hat Inc.

   This work is licensed under the terms of the GNU GPL, version 2 or later.  See
   the COPYING file in the top-level directory.


本文介绍 IOThread 功能，以及如何编写在 BQL 之外运行的代码。

主循环与 ``IOThread``
---------------------
QEMU 是事件驱动程序，通过事件循环同时处理多项工作。
VNC 服务器和 QMP 监控器在同一事件循环中处理：
循环监视文件描述符，可读时调用回调。

默认事件循环称为主循环，见 main-loop.c。
可通过 ``-object iothread,id=my-iothread`` 创建额外事件循环线程。

补充说明：主循环和 IOThread 都是事件循环，但代码并未完全共享。
概念相似不意味着当前可以互换，有时需要记住这一区别。

为什么 ``IOThread`` 有用
------------------------
IOThread 允许用户控制工作分配。
在多 CPU 主机上，单一主循环会限制扩展性，可将工作分散到多个 IOThread。
配置正确时，这可以改善 I/O 延迟，并减少客户机感知的抖动。

主循环还与 BQL 紧密关联，后者本身也是扩展性瓶颈。
vCPU 线程和主循环通过 BQL 串行执行 QEMU 代码，
因为历史上大量代码并不具备线程安全性，所以需要此互斥锁。

所有 I/O 集中在一个主循环，且全部 vCPU 与主循环争用 BQL，
因此有必要把工作移到 IOThread。

实验性 virtio-blk 数据平面实现的基准测试展示了这些效果：
ftp://public.dhe.ibm.com/linux/pdfs/KVM_Virtualized_IO_Performance_Paper.pdf

.. _how-to-program:

为 ``IOThread`` 编程
--------------------
旧代码与可在 IOThread 运行的新代码，主要区别在于后者显式处理
事件循环对象 AioContext，见 include/qemu/aio.h。
仅支持主循环的代码隐式使用主循环的 AioContext；
支持 IOThread 的代码必须知道自己所用的 AioContext。

AioContext 提供：

 * 文件描述符监视，在 POSIX 主机上包括读、写和错误；
 * 事件通知器，用于线程间信号；
 * 定时器；
 * Bottom Half（BH）延迟回调。

以下旧 API 使用主循环 AioContext：

 * 旧 ``qemu_aio_set_fd_handler()``：监视文件描述符。
 * 旧 ``qemu_aio_set_event_notifier()``：监视事件通知器。
 * 旧 ``timer_new_ms()``：创建定时器。
 * 旧 ``qemu_bh_new()``：创建 BH。
 * 旧 ``qemu_bh_new_guarded()``：创建带设备重入保护的 BH。
 * 旧 ``qemu_aio_wait()``：执行一轮事件循环。

它们隐式操作主循环，不能用于 IOThread 中运行的代码。
IOThread 未持有 BQL，调用这些函数可能崩溃或死锁。

应直接使用 AioContext 函数，见 include/qemu/aio.h：

 * ``aio_set_fd_handler()``：监视文件描述符。
 * ``aio_set_event_notifier()``：监视事件通知器。
 * ``aio_timer_new()``：创建定时器。
 * ``aio_bh_new()``：创建 BH。
 * ``aio_bh_new_guarded()``：创建带设备重入保护的 BH。
 * ``aio_poll()``：执行一轮事件循环。

qemu_bh_new_guarded/aio_bh_new_guarded 接收 MemReentrancyGuard 参数，
用于检查并防止重入问题。
设备相关 BH 的保护对象位于对应 DeviceState，名为 mem_reentrancy_guard。

通过 ``iothread_get_aio_context()`` 从 IOThread 获取 AioContext，
或通过 ``qemu_get_aio_context()`` 获取主循环的对象。
接收 AioContext 参数的代码可在两者中运行，取决于调用者传入哪个实例。

与 ``IOThread`` 同步
--------------------
多个线程访问的变量需要某种同步机制，例如 qemu_mutex_lock()、rcu_read_lock()。

aio_set_fd_handler()、aio_set_event_notifier()、aio_bh_new()、aio_timer_new()
等 AioContext 函数是线程安全的，可用于触发 IOThread 中的活动。

补充说明：跨线程安排函数调用的最佳方式是 ``aio_bh_schedule_oneshot()``。

主循环线程可通过 ``AIO_WAIT_WHILE()`` 同步等待某个条件。

``AioContext`` 与块层
---------------------
AioContext 起源于 QEMU 块层，如今是任意子系统都可使用的通用事件循环。

块层已集成支持，每个 BlockDriverState 通过 bdrv_try_change_aio_context()
和 bdrv_get_aio_context() 关联一个 AioContext，
让块层在正确的上下文处理 I/O。其他子系统也可采用类似方式。

因此块层代码必须预期自己可能运行在 IOThread 中，
避免隐式使用主循环的旧 API，方法见 :ref:`how-to-program`。

监控器代码通常需要确保此前客户机请求全部完成。
块设备位于 IOThread 时，该线程还会通过 ioeventfd 处理客户机请求。
为同时满足这些要求，应将代码放在 bdrv_drained_begin()
和 bdrv_drained_end() 之间，形成“排空区间”。

长时间运行的作业通常以协程形式调度到 BlockDriverState 的 AioContext。
可使用 ``bdrv_add``/``remove_aio_context_notifier``，
或使用 BlockBackend 时采用 ``blk_add``/``remove_aio_context_notifier``，
在 bdrv_try_change_aio_context() 将 BlockDriverState 移到另一上下文时获得通知。
