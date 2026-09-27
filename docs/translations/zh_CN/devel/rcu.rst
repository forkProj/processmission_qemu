使用 RCU（读—复制—更新）同步
============================

RCU 是保护以读取为主的数据结构的同步机制。
读端效率高、扩展性好，且无等待，因此可使读取路径非常快。

RCU 支持单写者与多读者并发，通常还需其他机制。
写端一般用锁串行化更新，也可把更新限制在单一任务中。
QEMU 常用的锁是 iothread mutex，也称大 QEMU 锁（BQL）；
限制到单任务则可通过 bottom half API 实现。

RCU 本质是“等待完成”：读端标记临界区，更新端在继续执行或异步调用回调前，
等待所有当前正在运行的临界区结束。

关键是只等待当前临界区。等待开始后才进入的临界区，虽然与更新者并发，
却不会延长等待。这使 RCU 比读写锁更容易扩展。
整个系统只需一个 RCU 机制实例，就可以保护任意数量的对象，
无需担心机制本身的竞争或死锁等问题。

其基本思想是将更新拆成移除和回收两阶段。
移除阶段确保后续读者无法再取得旧数据引用。
移除完成后新进入的临界区不能访问旧数据，因此无需等待它们。
只要此前的临界区全部结束，就不再有读者持有旧数据结构引用，
此时可以安全回收，例如释放内存或减少引用计数。

示意如下::

        thread 1                  thread 2                  thread 3
    -------------------    ------------------------    -------------------
    enter RCU crit.sec.
           |                finish removal phase
           |                begin wait
           |                      |                    enter RCU crit.sec.
    exit RCU crit.sec             |                           |
                            complete wait                     |
                            begin reclamation phase           |
                                                       exit RCU crit.sec.



线程 2 开始回收时，线程 3 仍处于临界区。
这是安全的，因为线程 3 进入时，旧版本数据结构已经不可访问。

RCU API
--------

核心 API 很精简：

``void rcu_read_lock(void);``
        读者通知回收方，自己正进入 RCU 读端临界区。

``void rcu_read_unlock(void);``
        读者通知回收方，自己正退出 RCU 读端临界区。
        读端临界区可以嵌套，也可以相互重叠。

``void synchronize_rcu(void);``
        阻塞至所有线程中此前已存在的 RCU 读端临界区全部结束，
        标志移除阶段结束、回收阶段开始。

        执行 synchronize_rcu 时，允许另一次更新到来。
        因此更新者最好先释放持有的锁，再调用该函数。
        若无法做到，例如更新者受 BQL 保护，可使用 call_rcu。

``void call_rcu1(struct rcu_head * head, void (*func)(struct rcu_head *head));``
        所有线程此前已存在的读端临界区结束后，调用 func(head)。
        这标志移除阶段结束，func 异步负责回收阶段。

        foo 结构需要包含 rcu_head，例如::

            struct foo {
                struct rcu_head rcu;
                int a;
                char b;
                long c;
            };


        回收函数便可取得 struct foo 地址并释放::

            call_rcu1(&foo.rcu, foo_reclaim);

            void foo_reclaim(struct rcu_head *rp)
            {
                struct foo *fp = container_of(rp, struct foo, rcu);
                g_free(fp);
            }


        通常通过 call_rcu 或 g_free_rcu 宏使用 call_rcu1，
        它们处理 rcu_head 是结构首字段的常见情况。

``void call_rcu(T *p, void (*func)(T *p), field-name);``
        struct rcu_head 是首字段时，可用此宏代替 call_rcu1。

``void g_free_rcu(T *p, field-name);``
        call_rcu 的特例，回调为 g_free。
        前面 call_rcu1 的示例可简写为::

            g_free_rcu(&foo, rcu);


``typeof(*p) qatomic_rcu_read(p);``
        类似 qatomic_load_acquire()，但对调用代码作出额外假设，允许更优化的实现。

        它假定一个临界区内读取多个共享数据时，
        这些读取要么具有数据依赖，要么不需要顺序保证。
        RCU 通常符合这一点：读端沿一个或多个会在更新时变化的指针遍历，
        直到目标数据结构，再读取其中内容。

        除非其他同步机制已阻止并发写入，否则读端临界区必须通过 qatomic_rcu_read() 读取。

        此外，应沿单一方向遍历，方向与更新者初始化数据结构的方向相反。

``void qatomic_rcu_set(p, typeof(*p) v);``
        类似 qatomic_store_release()，同样通过对调用代码的假设获得优化空间。

        如果更新者从不修改已对读者可见的数据项字段，此函数就足以与读者同步。
        初始化 RCU 保护结构的新副本时正是如此：
        确保在 qatomic_rcu_set() 发布前完成 ``*p`` 的初始化。
        遵守该规则时，写入顺序与读端遍历顺序相反，或只有一次更新，
        因而无需其他同步机制协调访问。

线程使用 RCU 前必须使用以下 API：

``void rcu_register_thread(void);``
        将线程标记为参与 RCU。
        线程必须定期报告静止点，可手动报告，
        也可通过 QemuCond、QemuSemaphore、QemuEvent API 报告。

``void rcu_unregister_thread(void);``
        将线程标记为不再参与 RCU。
        此后手动或通过上述 API 报告静止点也不会有问题。

这些 API 相对较重，且不得嵌套。

便捷宏
--------

以下两个宏在作用域结束时自动释放读锁：

``RCU_READ_LOCK_GUARD()``
         获取锁，并在所在代码块结束时释放。

``WITH_RCU_READ_LOCK_GUARD()  { code }``
         放在代码块开头，保护块内代码。

通过 goto 跳出受保护代码块也会释放锁。

与 Linux 的区别
---------------

- 可以在 RCU 临界区内等待互斥锁，但不建议这样做。
  用户态很少使用自旋锁；若禁止等待互斥锁，就无法将读端临界区升级为更新者。

- qatomic_rcu_read 和 qatomic_rcu_set 替代 rcu_dereference 和 rcu_assign_pointer，
  接收指向所访问变量的指针。

- call_rcu 是带额外参数的宏，该参数是结构首字段名称，且字段必须为 struct rcu_head。
  回调参数类型必须与第一个参数类型一致。
  call_rcu1 则对应 Linux 的 call_rcu。


RCU 使用模式
------------

许多读写锁模式可以直接转换为 RCU，获得更高扩展性并免于死锁。

通常，只要更新者每次运行都能创建数据结构的新版本，就可以用 RCU。
这一限制看起来很严格，但：

- 更新者不是所有写数据结构的操作，而是所有涉及回收步骤的操作，见下文数组示例。

- 创建新版本有时非常便宜。例如修改单链表的 next 指针，实际上就产生了新版本链表。

下面介绍一些常见用法。


RCU 链表处理
^^^^^^^^^^^^

待补充，目前 QEMU 尚未使用。


RCU 引用计数
^^^^^^^^^^^^

宽限期不能在相应读端临界区仍进行时结束，
因此读端原语可以充当一种受限的引用计数机制。例如::

    rcu_read_lock();
    p = qatomic_rcu_read(&foo);
    /* do something with p. */
    rcu_read_unlock();


临界区确保 p 至少在 rcu_read_unlock() 调用完成前保持有效，
某种意义上相当于取得 p 的引用，并在临界区结束时释放。
写端只需如下代码，并使用适当锁::

    qemu_mutex_lock(&foo_mutex);
    old = foo;
    qatomic_rcu_set(&foo, new);
    qemu_mutex_unlock(&foo_mutex);
    synchronize_rcu();
    free(old);


若处理不能全部在临界区内完成，可与真正的引用计数结合::

    rcu_read_lock();
    p = qatomic_rcu_read(&foo);
    foo_ref(p);
    rcu_read_unlock();
    /* do something with p. */
    foo_unref(p);


写端可以这样做::

    qemu_mutex_lock(&foo_mutex);
    old = foo;
    qatomic_rcu_set(&foo, new);
    qemu_mutex_unlock(&foo_mutex);
    synchronize_rcu();
    foo_unref(old);


或使用 call_rcu::

    qemu_mutex_lock(&foo_mutex);
    old = foo;
    qatomic_rcu_set(&foo, new);
    qemu_mutex_unlock(&foo_mutex);
    call_rcu(foo_unref, old, rcu);


两种情况下写端都只完成移除，最后一个 foo 引用释放时才回收。
由于最后引用可能由读端释放，使用 synchronize_rcu() 成本过高，
因此可以改为 call_rcu()::

     foo_unref(struct foo *p) {
        if (qatomic_fetch_dec(&p->refcount) == 1) {
            call_rcu(foo_destroy, p, rcu);
        }
    }



相同模式也可用读写锁实现::

    read_lock(&foo_rwlock);         write_mutex_lock(&foo_rwlock);
    p = foo;                        p = foo;
    /* do something with p. */      foo = new;
    read_unlock(&foo_rwlock);       free(p);
                                    write_mutex_unlock(&foo_rwlock);
                                    free(p);

    ------------------------------------------------------------------

    read_lock(&foo_rwlock);         write_mutex_lock(&foo_rwlock);
    p = foo;                        old = foo;
    foo_ref(p);                     foo = new;
    read_unlock(&foo_rwlock);       foo_unref(old);
    /* do something with p. */      write_mutex_unlock(&foo_rwlock);
    read_lock(&foo_rwlock);
    foo_unref(p);
    read_unlock(&foo_rwlock);


foo_unref 可以通过 bottom half 等机制，把释放内存移出写端临界区。


RCU 可变长数组
^^^^^^^^^^^^^^

可变长数组可以使用 RCU，只有调整容量时才需较昂贵的同步或 call_rcu。
需要注意：

- 从移除到回收之间，旧数组仍必须可访问；
- 读端不能看到不匹配的数组数据和大小。

第一个问题只需避免 realloc，每次分配新数组并复制旧数据。
如果数组大小和数据指针是大结构中的两个独立成员，就会出现第二个问题::

    struct mystuff {
        ...
        int data_size;
        int data_alloc;
        T   *data;
        ...
    };


应将大小与数组本体放在一起::

    struct arr {
        int size;
        int alloc;
        T   data[];
    };
    struct arr *global_array;

    read side:
        rcu_read_lock();
        struct arr *array = qatomic_rcu_read(&global_array);
        x = i < array->size ? array->data[i] : -1;
        rcu_read_unlock();
        return x;

    write side (running under a lock):
        if (global_array->size == global_array->alloc) {
            /* Creating a new version.  */
            new_array = g_malloc(sizeof(struct arr) +
                                 global_array->alloc * 2 * sizeof(T));
            new_array->size = global_array->size;
            new_array->alloc = global_array->alloc * 2;
            memcpy(new_array->data, global_array->data,
                   global_array->alloc * sizeof(T));

            /* Removal phase.  */
            old_array = global_array;
            qatomic_rcu_set(&global_array, new_array);
            synchronize_rcu();

            /* Reclamation phase.  */
            free(old_array);
        }



参考资料
--------

* `Linux 内核 RCU 文档 <https://docs.kernel.org/RCU/>`__
