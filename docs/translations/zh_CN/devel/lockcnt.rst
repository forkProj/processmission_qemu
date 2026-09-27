带锁计数器（``QemuLockCnt``）
=============================

QEMU 常用引用计数跟踪仍被访问、不能释放的数据结构。
例如，以下调用回调的循环并不安全::

    QLIST_FOREACH_SAFE(ioh, &io_handlers, next, pioh) {
        if (ioh->revents & G_IO_OUT) {
            ioh->fd_write(ioh->opaque);
        }
    }


QLIST_FOREACH_SAFE 通过提前保存 next，允许删除当前节点 ioh。
但 ioh->fd_write 实际也可能删除下一个节点。
最简单的规避方法是先标记删除，再在循环中移除::

    QLIST_FOREACH_SAFE(ioh, &io_handlers, next, pioh) {
        if (ioh->deleted) {
            QLIST_REMOVE(ioh, next);
            g_free(ioh);
        } else {
            if (ioh->revents & G_IO_OUT) {
                ioh->fd_write(ioh->opaque);
            }
        }
    }


若循环还必须支持重入，即 fd_write 可能再次调用此循环，就需要计数::

    walking_handlers++;
    QLIST_FOREACH_SAFE(ioh, &io_handlers, next, pioh) {
        if (ioh->deleted) {
            if (walking_handlers == 1) {
                QLIST_REMOVE(ioh, next);
                g_free(ioh);
            }
        } else {
            if (ioh->revents & G_IO_OUT) {
                ioh->fd_write(ioh->opaque);
            }
        }
    }
    walking_handlers--;


也许会想到使用 rcu_read_lock() 和 rcu_read_unlock()，
让 RCU 嵌套计数取代全局 walking_handlers。
引用计数与 RCU 目的相近，但通常互为补充：

- 引用计数粒度细，只作用于单个结构；RCU 延迟所有受保护结构的回收。

- 引用计数支持长期持有引用；RCU 临界区原则上应较短。

- 引用计数常用于非线程安全但可重入的代码，QEMU 使用它比引入线程早很多年。
  RCU 通常保护读者，避免其他线程在并发修改后释放其使用的内存。

- RCU 可由独立线程回收，可能提升性能，也可能不必要地延迟回收。
  引用计数的回收则是确定性的。

本文介绍 QemuLockCnt，它为既需线程安全又需可重入的代码提供引用计数抽象。


``QemuLockCnt`` 概念
--------------------

QemuLockCnt 包含计数器和互斥锁，提供增减计数及加锁、解锁原语。
计数表示当前有多少次数据结构访问，可能来自不同线程，
也可能是同一线程的多次重入。基本规则为：

- 只有计数为零且持有互斥锁时，才可释放受保护数据。

- 计数为零且互斥锁已被持有时，不能开始新访问。

通常互斥锁保护全部写入，而不仅是释放，但某些场景无需如此。

读取可不持锁，前提是读写双方使用与 RCU 相同的宏，
例如 qatomic_rcu_read、qatomic_rcu_set、QLIST_FOREACH_RCU。
读取在锁外进行，赋值或 QLIST_INSERT_HEAD 可能与之并发，
RCU API 保证处理器和编译器遵守必要的内存屏障。

一种简单实现是用互斥锁保护计数，例如::

    // (1)
    qemu_mutex_lock(&walking_handlers_mutex);
    walking_handlers++;
    qemu_mutex_unlock(&walking_handlers_mutex);

    ...

    // (2)
    qemu_mutex_lock(&walking_handlers_mutex);
    if (--walking_handlers == 0) {
        QLIST_FOREACH_SAFE(ioh, &io_handlers, next, pioh) {
            if (ioh->deleted) {
                QLIST_REMOVE(ioh, next);
                g_free(ioh);
            }
        }
    }
    qemu_mutex_unlock(&walking_handlers_mutex);


省略号处不能发生释放。
如果另一线程正在执行临界区 (2)，当前线程无法增加 walking_handlers，
因此无法进入相应代码。
访问期间，其他线程都会看到非零计数，和单线程版本一样。

多个并发访问可能无限延迟清理，即计数始终不归零。
因此这种技术更适合结构并发访问较少的情况。

每次修改计数都需要临界区，容易忘记。
QemuLockCnt 确保计数修改适当加锁，并通过两种方式提高效率：

- 许多操作不需要加锁，例如在计数非零时递增。

- 部分平台可将锁状态和计数保存在单个字中，
  快速路径成本不超过简单原子计数，见 :doc:`atomics`。
  并发访问预期很少时，这尤其有帮助。


释放与写入共用互斥锁仍有少量低效情况。
例如计数为零且锁被持有时不能开始访问，
即使持锁的是原则上无需阻止读取的写操作。
不过只要满足以下任一条件，通常就不是问题：

- 可能并发访问，但很少发生；

- 写入很少；

- 写入频繁，但这类写操作，例如追加链表，临界区非常短。

例如，QEMU 用 QemuLockCnt 管理 AioContext 的 BH 和文件描述符处理器列表。
文件描述符列表修改较少；创建 BH 较频繁，可能位于快速路径，
但几乎不与 BH 列表遍历并发，且关键路径只有三个指令：两次赋值和 smp_wmb()。


``QemuLockCnt`` API
-------------------

.. kernel-doc:: include/qemu/lockcnt.h


``QemuLockCnt`` 用法
--------------------

本节介绍典型使用模式。

在 qemu_lockcnt_lock 与 qemu_lockcnt_unlock 之间将变量设为非 NULL::

    qemu_lockcnt_lock(&xyz_lockcnt);
    if (!xyz) {
        new_xyz = g_new(XYZ, 1);
        ...
        qatomic_rcu_set(&xyz, new_xyz);
    }
    qemu_lockcnt_unlock(&xyz_lockcnt);


在 qemu_lockcnt_inc 与 qemu_lockcnt_dec 之间访问其值::

    qemu_lockcnt_inc(&xyz_lockcnt);
    if (xyz) {
        XYZ *p = qatomic_rcu_read(&xyz);
        ...
        /* Accesses can now be done through "p".  */
    }
    qemu_lockcnt_dec(&xyz_lockcnt);


释放也可使用 lock/unlock，但必须额外确保计数为零，即没有并发访问。
因为 qemu_lockcnt_inc 会获取 QemuLockCnt 的锁，释放期间计数不能变成非零。
释放代码如下::

    qemu_lockcnt_lock(&xyz_lockcnt);
    if (!qemu_lockcnt_count(&xyz_lockcnt)) {
        g_free(xyz);
        xyz = NULL;
    }
    qemu_lockcnt_unlock(&xyz_lockcnt);


如果访问后立即释放对象，可把递减、加锁和计数检查合并::

    qemu_lockcnt_inc(&xyz_lockcnt);
    if (xyz) {
        XYZ *p = qatomic_rcu_read(&xyz);
        ...
        /* Accesses can now be done through "p".  */
    }
    if (qemu_lockcnt_dec_and_lock(&xyz_lockcnt)) {
        g_free(xyz);
        xyz = NULL;
        qemu_lockcnt_unlock(&xyz_lockcnt);
    }


也可用于访问链表::

    qemu_lockcnt_inc(&io_handlers_lockcnt);
    QLIST_FOREACH_RCU(ioh, &io_handlers, pioh) {
        if (ioh->revents & G_IO_OUT) {
            ioh->fd_write(ioh->opaque);
        }
    }

    if (qemu_lockcnt_dec_and_lock(&io_handlers_lockcnt)) {
        QLIST_FOREACH_SAFE(ioh, &io_handlers, next, pioh) {
            if (ioh->deleted) {
                QLIST_REMOVE(ioh, next);
                g_free(ioh);
            }
        }
        qemu_lockcnt_unlock(&io_handlers_lockcnt);
    }


这里仍使用 RCU 原语，因为遍历期间可能插入新项。
QLIST_FOREACH_RCU 确保处理器和编译器遵守相应内存屏障。

另一种模式使用 qemu_lockcnt_dec_if_lock::

    qemu_lockcnt_inc(&io_handlers_lockcnt);
    QLIST_FOREACH_SAFE_RCU(ioh, &io_handlers, next, pioh) {
        if (ioh->deleted) {
            if (qemu_lockcnt_dec_if_lock(&io_handlers_lockcnt)) {
                QLIST_REMOVE(ioh, next);
                g_free(ioh);
                qemu_lockcnt_inc_and_unlock(&io_handlers_lockcnt);
            }
        } else {
            if (ioh->revents & G_IO_OUT) {
                ioh->fd_write(ioh->opaque);
            }
        }
    }
    qemu_lockcnt_dec(&io_handlers_lockcnt);


此时可用 qemu_lockcnt_dec，而非 qemu_lockcnt_dec_and_lock，
因为计数从 1 降至 0 时没有特殊操作。
