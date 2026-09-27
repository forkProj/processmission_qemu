.. _atomics-ref:

=================
QEMU 中的原子操作
=================

CPU 执行相互独立的内存操作时，实际顺序近乎任意，
这会给 CPU 间交互带来问题，也包括 QEMU 与客户机的交互。
多线程程序通过各种机制，要求编译器和 CPU 将顺序限制为符合程序员预期的形式。

最基本的机制是锁。QEMU 使用互斥锁、条件变量和信号量，它们应是默认同步方式。
其他方式难得多，但确实经常有必要；
尤其性能关键部分需要内存屏障和原子操作等底层并发机制。
并发内存访问语义遵循 C11 内存模型。

qemu/atomic.h 封装 C11 原子操作，提供更好的可移植性和更简洁语法，宏分为三类：

- 编译器屏障：barrier()。

- 弱原子访问及手动内存屏障：qatomic_read()、qatomic_set()、smp_rmb()、
  smp_wmb()、smp_mb()、smp_mb_acquire()、smp_mb_release()、
  smp_read_barrier_depends()、smp_mb__before_rmw()、smp_mb__after_rmw()。

- 顺序一致的原子访问：其他所有操作。

通常应将这些操作封装为易用数据结构，例如无锁单链表操作
QSLIST_INSERT_HEAD_ATOMIC、QSLIST_MOVE_ATOMIC，
或 RCU、QemuEvent、QemuLockCnt 等同步原语。
直接使用原子操作和内存屏障应限于线程间标志检查，并提供充分说明。



编译器内存屏障
==============

barrier() 阻止编译器将其两侧的内存访问移到另一侧。
它不直接影响 CPU，CPU 仍可自行重排。

它主要用于 qemu/atomic.h 内部。
某些架构的 CPU 顺序保证足够强，只限制编译器优化就能确保执行顺序，
此时较强屏障会退化为单纯的编译器屏障。

编写可能被信号处理器中断的代码时，barrier() 也很有用。


顺序一致的原子访问
==================

qemu/atomic.h 的多数操作保证顺序一致性：
任何执行的结果，都等同于所有处理器的操作按某种统一顺序执行，
且各处理器自身操作在该顺序中遵循其程序顺序。

提供以下原子读—改—写操作::

    void qatomic_inc(ptr)
    void qatomic_dec(ptr)
    void qatomic_add(ptr, val)
    void qatomic_sub(ptr, val)
    void qatomic_and(ptr, val)
    void qatomic_or(ptr, val)

    typeof(*ptr) qatomic_fetch_inc(ptr)
    typeof(*ptr) qatomic_fetch_dec(ptr)
    typeof(*ptr) qatomic_fetch_add(ptr, val)
    typeof(*ptr) qatomic_fetch_sub(ptr, val)
    typeof(*ptr) qatomic_fetch_and(ptr, val)
    typeof(*ptr) qatomic_fetch_or(ptr, val)
    typeof(*ptr) qatomic_fetch_xor(ptr, val)
    typeof(*ptr) qatomic_fetch_inc_nonzero(ptr)
    typeof(*ptr) qatomic_xchg(ptr, val)
    typeof(*ptr) qatomic_cmpxchg(ptr, old, new)


这些操作都返回 ``*ptr`` 的旧值。
它们具有多态性，适用于宽度不超过指针的任意类型。

对应的以下操作返回新值::

    typeof(*ptr) qatomic_inc_fetch(ptr)
    typeof(*ptr) qatomic_dec_fetch(ptr)
    typeof(*ptr) qatomic_add_fetch(ptr, val)
    typeof(*ptr) qatomic_sub_fetch(ptr, val)
    typeof(*ptr) qatomic_and_fetch(ptr, val)
    typeof(*ptr) qatomic_or_fetch(ptr, val)
    typeof(*ptr) qatomic_xor_fetch(ptr, val)


还提供 qatomic_set 后接 smp_mb 的优化快捷方式::

    void         qatomic_set_mb(ptr, val)



弱原子访问与手动内存屏障
========================

相较顺序一致访问，使用较弱一致性模型编程复杂得多。
此时只能依赖以下保证：

- 原子访问不会造成数据竞争及由此引发的未定义行为。
  普通访问若与其他访问并发，且至少一方为写，就会发生数据竞争。
  为此，编译器不会消除原子访问、凭空增加访问或执行类似优化。

- 对系统其他组件而言，acquire 操作先于程序中其后的所有 LOAD 或 STORE 发生。

- 对系统其他组件而言，release 操作后于程序中其前的所有 LOAD 或 STORE 发生。

- release 与 acquire 之间建立 synchronizes-with 关系，详情见 :ref:`acqrel`。

使用此模型时，通过以下操作访问变量：

- qatomic_read()、qatomic_set() 防止编译器消除访问或凭空增加访问，
  但不为加载和存储施加顺序；编译器和处理器均可重排。

- qatomic_load_acquire() 保证此 LOAD 先于其后的所有 LOAD 和 STORE。
  原本位于它前面的操作仍可移到它之后。

- qatomic_store_release() 保证此 STORE 后于其前的所有 LOAD 和 STORE。
  原本位于它后面的操作仍可移到它之前。

也可通过屏障宏限制顺序：smp_rmb()、smp_wmb()、smp_mb()、
smp_mb_acquire()、smp_mb_release()、smp_read_barrier_depends()。

这些屏障控制共享内存引用顺序，共六种：

- smp_rmb() 保证屏障前全部 LOAD，在其他系统组件看来先于屏障后全部 LOAD。
  它为加载建立偏序，但不要求影响存储。

- smp_wmb() 保证屏障前全部 STORE 先于屏障后全部 STORE。
  它为存储建立偏序，但不要求影响加载。

- smp_mb_acquire() 保证屏障前全部 LOAD 先于屏障后全部 LOAD 或 STORE。

- smp_mb_release() 保证屏障后全部 STORE 后于屏障前全部 LOAD 或 STORE。

- smp_mb() 保证屏障前全部 LOAD 和 STORE 先于屏障后全部 LOAD 和 STORE。
  它同时为读写建立偏序，强于单独的读屏障或写屏障，
  包含 acquire 和 release 屏障保证，
  还阻止屏障前 STORE 与屏障后 LOAD 相互越过。

- smp_read_barrier_depends() 是较弱的读屏障。
  多数处理器上，若第二次加载依赖第一次结果，
  例如第一次读出第二次访问的地址，处理器就保证先后顺序。
  因此不同于 smp_rmb() 或 qatomic_load_acquire()，
  它在 Arm、PPC 等弱序架构上也可以只是编译器屏障 [#alpha]_。

  这里必须是数据依赖，而非控制依赖。
  如果第二次加载地址通过条件判断依赖第一次结果，而非直接使用读出的地址，
  就属于控制依赖，需要完整读屏障或更强机制。

.. [#alpha] DEC Alpha 是例外，需要处理器屏障。
   在 x86、s390 等强序架构上，smp_rmb() 和 qatomic_load_acquire()
   也可以仅实现为编译器屏障。

内存屏障及 acquire/release 访问，主要用于一个线程固定写、另一个线程固定读的结构：

    +----------------------------------+----------------------------------+
    | 线程 1                           | 线程 2                           |
    +==================================+==================================+
    | ::                               | ::                               |
    |                                  |                                  |
    |   qatomic_store_release(&a, x);  |   y = qatomic_load_acquire(&b);  |
    |   qatomic_store_release(&b, y);  |   x = qatomic_load_acquire(&a);  |
    +----------------------------------+----------------------------------+

此时可通过下文的“配对”技巧检查正确性。

有时一个线程访问许多彼此无关的变量，
例如除当前线程外，每个变量都只有另外一个线程访问。
此时可把屏障提出循环，例如：

    +------------------------------------------+----------------------------------+
    | 修改前                                   | 修改后                           |
    +==========================================+==================================+
    | ::                                       | ::                               |
    |                                          |                                  |
    |   n = 0;                                 |   n = 0;                         |
    |   for (i = 0; i < 10; i++)               |   for (i = 0; i < 10; i++)       |
    |     n += qatomic_load_acquire(&a[i]);    |     n += qatomic_read(&a[i]);    |
    |                                          |   smp_mb_acquire();              |
    +------------------------------------------+----------------------------------+
    | ::                                       | ::                               |
    |                                          |                                  |
    |                                          |   smp_mb_release();              |
    |   for (i = 0; i < 10; i++)               |   for (i = 0; i < 10; i++)       |
    |     qatomic_store_release(&a[i], false); |     qatomic_set(&a[i], false);   |
    +------------------------------------------+----------------------------------+

拆分循环也可以减少屏障数量：

    +------------------------------------------+----------------------------------+
    | 修改前                                   | 修改后                           |
    +==========================================+==================================+
    | ::                                       | ::                               |
    |                                          |                                  |
    |   n = 0;                                 |     smp_mb_release();            |
    |   for (i = 0; i < 10; i++) {             |     for (i = 0; i < 10; i++)     |
    |     qatomic_store_release(&a[i], false); |       qatomic_set(&a[i], false); |
    |     smp_mb();                            |     smp_mb();                    |
    |     n += qatomic_read(&b[i]);            |     n = 0;                       |
    |   }                                      |     for (i = 0; i < 10; i++)     |
    |                                          |       n += qatomic_read(&b[i]);  |
    +------------------------------------------+----------------------------------+

以下例子还将一个 smp_mb_release() 替换为可能更便宜且更清楚的 smp_wmb()：

    +------------------------------------------+----------------------------------+
    | 修改前                                   | 修改后                           |
    +==========================================+==================================+
    | ::                                       | ::                               |
    |                                          |                                  |
    |                                          |     smp_mb_release();            |
    |   for (i = 0; i < 10; i++) {             |     for (i = 0; i < 10; i++)     |
    |     qatomic_store_release(&a[i], false); |       qatomic_set(&a[i], false); |
    |     qatomic_store_release(&b[i], false); |     smp_wmb();                   |
    |   }                                      |     for (i = 0; i < 10; i++)     |
    |                                          |       qatomic_set(&b[i], false); |
    +------------------------------------------+----------------------------------+


.. _acqrel:

Acquire/release 配对与 synchronizes-with 关系
---------------------------------------------

除 qatomic_set() 和 qatomic_read() 外，原子操作具有 acquire 或 release 语义 [#rmw]_，
产生两种效果：

.. [#rmw] 读—改—写可同时具有两者：acquire 作用于读部分，release 作用于写部分。

- 在线程内部，acquire 排在后续操作之前，release 排在此前操作之后。

- 一个线程的 release 若与另一线程的 acquire 同步，
  顺序约束就从前一个线程传播到后一个线程。
  即前者 release 之前的所有操作，保证 happens-before 后者 acquire 之后的所有操作。

这种语义不限于原子操作，几乎所有高级同步原语也具有 acquire 或 release 语义。例如：

- pthread_mutex_lock 具有 acquire 语义；pthread_mutex_unlock 具有 release 语义，
  并与同一互斥锁的后续 lock 同步。

- pthread_cond_signal 和 pthread_cond_broadcast 具有 release 语义。
  pthread_cond_wait 同时具有 release（与 mutex_lock 同步）
  和 acquire（与 mutex_unlock 及条件变量通知同步）语义。

- pthread_create 具有 release 语义，与新线程启动同步；
  pthread_join 具有 acquire 语义，与线程退出同步。

- qemu_event_set 具有 release 语义，qemu_event_wait 具有 acquire 语义。

以下例子没有原子访问，但线程 2 仍依赖 pthread_exit 的 release
与 pthread_join 的 acquire 之间的同步关系：

      +----------------------+-------------------------------+
      | 线程 1               | 线程 2                        |
      +======================+===============================+
      | ::                   | ::                            |
      |                      |                               |
      |   *a = 1;            |                               |
      |   pthread_exit(a);   |   pthread_join(thread1, &a);  |
      |                      |   x = *a;                     |
      +----------------------+-------------------------------+

线程间同步基本来自 release 与 acquire 的配对。
因此，除 qatomic_set/read 外，原子操作几乎总与相反类别操作配对。
这是一条很实用的判断规则，但在 QEMU 中，配对另一端可能位于客户机驱动中。

smp_read_barrier_depends()、smp_rmb()、smp_mb_acquire()、
qatomic_load_acquire()、qatomic_rcu_read() 都算 acquire。
smp_wmb()、smp_mb_release()、qatomic_store_release()、qatomic_rcu_set() 都算 release。
smp_mb() 兼具两者，可与其他任何原子操作配对。例如：

      +----------------------+------------------------------+
      | 线程 1               | 线程 2                       |
      +======================+==============================+
      | ::                   | ::                           |
      |                      |                              |
      |   qatomic_set(&a, 1);|                              |
      |   smp_wmb();         |                              |
      |   qatomic_set(&b, 2);|   x = qatomic_read(&b);      |
      |                      |   smp_rmb();                 |
      |                      |   y = qatomic_read(&a);      |
      +----------------------+------------------------------+

加载和存储只有访问同一变量时才能配对：
对 x 的 store-release 与对 x 的 load-acquire 同步；
release 屏障则可与任意 acquire 操作同步。
以下展示正确同步：

      +--------------------------------+--------------------------------+
      | 线程 1                         | 线程 2                         |
      +================================+================================+
      | ::                             | ::                             |
      |                                |                                |
      |   qatomic_set(&a, 1);          |                                |
      |   qatomic_store_release(&b, 2);|   x = qatomic_load_acquire(&b);|
      |                                |   y = qatomic_read(&a);        |
      +--------------------------------+--------------------------------+

也可以依赖高级原语的 acquire/release 语义建立同步关系。

写线程和读线程访问变量的顺序相反，这是预期行为：
release 前的存储通常对应 acquire 后的加载，反之亦然。
前面的 pthread_exit/pthread_join 例子也是如此。

以下更复杂的例子包含多次访问和数据依赖屏障，
且在不会发生数据竞争的地方不使用原子访问：

      +----------------------+------------------------------+
      | 线程 1               | 线程 2                       |
      +======================+==============================+
      | ::                   | ::                           |
      |                      |                              |
      |   b[2] = 1;          |                              |
      |   smp_wmb();         |                              |
      |   x->i = 2;          |                              |
      |   smp_wmb();         |                              |
      |   qatomic_set(&a, x);|  x = qatomic_read(&a);       |
      |                      |  smp_read_barrier_depends(); |
      |                      |  y = x->i;                   |
      |                      |  smp_read_barrier_depends(); |
      |                      |  z = b[y];                   |
      +----------------------+------------------------------+

与 Linux 内核原语比较
=====================

Linux 内核原子操作和内存屏障与 QEMU 对应机制的区别包括：

- Linux 原子操作总是作用于 32 位 int，使用封装类型 atomic_t；
  QEMU 原子操作具有多态性，直接使用普通 C 类型。

- Linux 的 atomic_read/set 最初不提供任何保证。
  从 Linux 4.1 起，通过 ACCESS_ONCE 或后来的 READ/WRITE_ONCE 实现 volatile 语义。

  QEMU 的 qatomic_read/set 在编译器支持时使用 C11 relaxed 原子语义，
  否则使用 volatile 语义。两者都阻止某些编译器变换，
  区别是原子访问保证原子性，volatile 不保证。
  后一种情况下，只能基于变量大小不超过机器字且正确对齐的假设，
  期待编译器生成原子访问。

  无论 Linux 还是 QEMU，这些读写操作都不隐含屏障。

- Linux 的原子读—改—写操作分为以下形式：

         ===================== =========================================
         ``atomic_OP``         不返回值
         ``atomic_OP_return``  返回变量新值
         ``atomic_fetch_OP``   返回变量旧值
         ``atomic_cmpxchg``    返回变量旧值
         ===================== =========================================

  QEMU 中第二种名为 atomic_OP_fetch。

- Linux 不同原子读—改—写操作隐含不同的屏障集合。
  QEMU 则全部保证顺序一致性，即程序观察到统一的操作顺序。

- 但根据 QEMU 使用的 C11 内存模型，该顺序不会自动传播到操作两侧的其他内存访问。
  对其他访问而言，该操作只是 load-acquire 后接 store-release。
  此前存储和此后加载仍可重排，发生在读—改—写操作的中间。

  因此，以下代码在 Linux 中正确，在 QEMU 中不正确：

      +----------------------------------+--------------------------------+
      | Linux（正确）                    | QEMU（错误）                   |
      +==================================+================================+
      | ::                               | ::                             |
      |                                  |                                |
      |   a = atomic_fetch_add(&x, 2);   |   a = qatomic_fetch_add(&x, 2);|
      |   b = READ_ONCE(&y);             |   b = qatomic_read(&y);        |
      +----------------------------------+--------------------------------+

  因为处理器或编译器可能将读取 y 移到写入 x 之前。

  需要在写 x 和读 y 之间增加完整内存屏障。
  QEMU 提供 smp_mb__before_rmw() 和 smp_mb__after_rmw()：
  一方面在不需要屏障的处理器上省略它以优化性能，
  另一方面明确表达 C11 内存模型的这个特殊情况：

      +--------------------------------+
      | QEMU（正确）                   |
      +================================+
      | ::                             |
      |                                |
      |   a = qatomic_fetch_add(&x, 2);|
      |   smp_mb__after_rmw();         |
      |   b = qatomic_read(&y);        |
      +--------------------------------+

  常见的只有一个线程写 x 的情况下，也可这样写：

      +--------------------------------+
      | QEMU（正确）                   |
      +================================+
      | ::                             |
      |                                |
      |   a = qatomic_read(&x);        |
      |   qatomic_set_mb(&x, a + 2);   |
      |   b = qatomic_read(&y);        |
      +--------------------------------+

来源
========

- Linux 内核的 ``Documentation/memory-barriers.txt``。
