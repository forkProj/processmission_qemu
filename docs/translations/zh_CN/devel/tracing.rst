.. _tracing:

========
跟踪
========

简介
========

本文介绍 QEMU 跟踪基础设施，以及如何用于调试、性能分析和观察执行。

快速入门
========

启用 memory_region_ops_read 和 memory_region_ops_write 事件::

    $ qemu --trace "memory_region_ops_*" ...
    ...
    719585@1608130130.441188:memory_region_ops_read cpu 0 mr 0x562fdfbb3820 addr 0x3cc value 0x67 size 1
    719585@1608130130.441190:memory_region_ops_write cpu 0 mr 0x562fdfbd2f00 addr 0x3d4 value 0x70e size 2


输出来自 log 后端。未显式指定 ./configure --enable-trace-backends=BACKENDS 时，
默认启用该后端。

重复 --trace 可指定多个模式::

    $ qemu --trace "kvm_*" --trace "virtio_*" ...


经常使用的模式可以保存在文件中，避免过长命令行::

    $ echo "memory_region_ops_*" >/tmp/events
    $ echo "kvm_*" >>/tmp/events
    $ qemu --trace events=/tmp/events ...


跟踪事件
========

子目录设置
----------

源码各目录可在本地 trace-events 声明事件。
所有含该文件的目录都必须列入顶层 meson.build 的 trace_events_subdirs。
构建时，tracetool 处理各文件并生成代码。

各文件还会合并成 trace-events-all，安装到 /usr/share/qemu，
供 simpletrace.py 后续分析 simpletrace 格式日志。

构建期间在 <builddir>/trace/ 自动生成：

 - trace-<subdir>.c：事件状态声明。
 - trace-<subdir>.h：事件枚举和探针函数。
 - trace-dtrace-<subdir>.h：DTrace 探针规格。
 - trace-dtrace-<subdir>.dtrace：DTrace 探针辅助声明。
 - trace-dtrace-<subdir>.o：由 dtrace 生成的二进制提供者。
 - trace-ust-<subdir>.h：UST 探针辅助声明。

subdir 是把路径斜杠替换为下划线后的名称。
例如 accel/kvm 变为 accel_kvm，对应 trace-accel_kvm.c。

源码不直接包含构建目录的生成文件，而包含本目录的 trace.h，不带路径前缀。
例如 io/channel-buffer.c::

  #include "trace.h"


io/trace.h 必须手动创建，包含对应生成头文件::

  $ (echo '/* SPDX-License-Identifier: GPL-2.0-or-later */' ; echo '#include "trace/trace-io.h"')  >io/trace.h


虽然可以包含其他目录的 trace.h，但通常不推荐。
应优先在实际使用事件的目录声明。
唯一例外是顶层 trace-events 中的共享事件。
顶层生成文件使用 trace/trace-root 前缀，避免与当前目录 trace.h 混淆。

使用跟踪事件
------------

在源码中直接调用::

    #include "trace.h"  /* needed for trace event prototype */
    
    void *qemu_vmalloc(size_t size)
    {
        void *ptr;
        size_t align = QEMU_VMALLOC_ALIGN;
     
        if (size < align) {
            align = getpagesize();
        }
        ptr = qemu_memalign(align, size);
        trace_qemu_vmalloc(size, ptr);
        return ptr;
    }


声明跟踪事件
------------

tracetool 生成供所有事件使用方包含的 trace.h。
由于包含它的源码很多，应尽量少引入类型和其他头文件，
保持命名空间整洁，降低编译时间和依赖。

类型选择规则：

 * 固定大小使用 stdint.h 类型。偏移和客户机地址通常使用 uint32_t 或 uint64_t。
   避免随主机 32/64 位变化的基本类型，防止截断或构建失败。

 * 结构指针和数组使用 ``void *``。
   trace.h 无法包含全部用户结构声明，因此需要通用指针。

 * 其他值使用符号属性适当的 char、int、long 等基本标量。

 * 避免 float 和 double，因为 SystemTap 不支持。
   通常可舍入为整数；若需保留小数，先乘以 1000 等比例因子。

格式字符串必须匹配类型，int64_t 和 uint64_t 分别使用 PRId64 和 PRIu64，
确保 32/64 位平台可移植性。
字符串末尾不得包含换行，由后端适配日志行尾。

声明依次包含事件名、参数和格式字符串。例如::

    qemu_vmalloc(size_t size, void *ptr) "size %zu ptr %p"
    qemu_vfree(void *ptr) "ptr %p"



添加新事件的建议
----------------

1. 跟踪状态变化，例如启动、停止、分配和释放。
   这些位置有助于理解系统执行。
2. 跟踪客户机操作，例如读取设备寄存器等 I/O，帮助理解客户机交互。
3. 使用关联字段提供上下文，例如记录 malloc 返回及 free 接收的指针，
   便于匹配分配和释放。缺少上下文的事件用处有限。
4. 按所在函数命名事件，同一函数多个事件时，末尾加唯一标识。

通用接口与监控器命令
====================

trace/control.h 提供后端无关接口，可用代码查询和控制事件状态。

部分后端未实现其中某些功能，此时 QEMU 只输出警告。
哪些函数依赖后端，见该头文件。

也可通过监控器命令查询和修改：

* ``info trace-events``
  查看可用事件及状态，1 表示启用，0 表示禁用。

* ``trace-event NAME on|off``
  启用或禁用一个事件，或用通配符选择一组事件。

--trace events=<file> 可从程序开始时启用文件所列事件，每行一个名称。
行首为连字符时则禁用，适合先用通配符启用整组，再关闭噪声较大的单个事件。

监控器命令和事件列表文件都支持通配匹配，可批量切换共同前缀事件。
例如启用 virtio-blk::

    trace-event virtio_blk_* on


跟踪后端
========

tracetool 自动生成繁琐代码，并让事件声明独立于后端，
不会与 LTTng、SystemTap 等紧密绑定。
可通过扩展该脚本增加新后端。

在配置阶段选择后端::

    ./configure --enable-trace-backends=simple,dtrace


支持列表见 ./configure --help 或下文。
启用多个后端时，事件会发送给全部后端。
未显式选择时默认 log。

以下介绍各后端。

Nop
--------

nop 生成空事件函数，编译器可完全消除，不产生性能开销。

无论选择何种后端，具有 disable 属性的事件都使用 nop。

Log
--------

log 直接输出到标准错误，相当于调试 printf。
这是最简单的后端，可与已有 DPRINTF() 代码配合。

-msg timestamp=on|off 控制是否输出线程 ID／时间戳前缀。

Simpletrace
-----------

simple 由独立线程写二进制日志，开销低于 log。
提供 Python API 编写离线分析脚本。
功能可能不如平台专用或第三方后端，但可移植且没有特殊库依赖。

监控器命令
~~~~~~~~~~

* ``trace-file on|off|flush|set <path>``
  启用、禁用、刷新日志文件，或设置文件名。

分析跟踪文件
~~~~~~~~~~~~

simple 的二进制文件可由 simpletrace.py 格式化，
脚本接收 trace-events-all 和二进制日志::

    ./scripts/simpletrace.py trace-events-all trace-12345


必须使用构建该 QEMU 时的同一事件文件，
否则声明可能已变化，输出会不一致。

Ftrace
--------

ftrace 向 ftrace marker 写入数据，事件进入其环形缓冲区，
便于比较 QEMU 与内核跟踪，尤其是使用 KVM 时的 kvm.ko。

使用 KVM 时，先启用相应事件::

   # echo 1 > /sys/kernel/debug/tracing/events/kvm/enable


以 root 运行 QEMU 后，读取日志::

   # cat /sys/kernel/debug/tracing/trace


限制：仅支持 Linux。

Syslog
--------

syslog 使用 POSIX syslog API，打开日志时指定 LOG_DAEMON 和 LOG_PID，
让事件带有来源 QEMU 进程 PID，全部事件采用 LOG_INFO 级别。

注意：syslog 可能合并连续重复事件并限速。

限制：仅支持兼容 POSIX 的系统。

LTTng 用户空间跟踪器
--------------------

ust 使用 LTTng Userspace Tracer 库。
QEMU 没有内置对应监控器命令，应使用 UST 工具列举、启停和转储跟踪。

需要 lttng-tools。当前用户应属于 tracing 组，
或在启动 QEMU 前为该用户手动运行 lttng-sessiond。

运行已插桩的 QEMU 时，可列举全部事件::

    lttng list -u


创建会话::

    lttng create mysession


启用事件::

    lttng enable-event qemu:g_malloc -u


events 可以是逗号分隔列表，也可用 -a 启用全部探针事件。
按需启动和停止::

    lttng start
    lttng stop


查看日志::

    lttng view


销毁会话::

    lttng destroy


之后仍可随时通过 Babeltrace 查看::

    babeltrace $HOME/lttng-traces/mysession-<date>-<time>


SystemTap
---------

dtrace 使用 DTrace SDT 探针，但目前只在 SystemTap 上测试。
检测到支持时，生成带封装探针的 .stp 文件，便于脚本使用。
也可在构建后手动执行，修改探针中的二进制程序名称::

    scripts/tracetool.py --backends=dtrace --format=stap \
                         --binary path/to/qemu-binary \
                         --probe-prefix qemu.system.x86_64 \
                         --group=all \
                         trace-events-all \
                         qemu.stp


只需对某些探针执行 printf 日志时，可使用 qemu-trace-stap 辅助脚本，
用法见其手册页。

跟踪事件属性
============

trace-events-all 的各事件前，可添加以空格分隔的零个或多个属性。

"disable"
---------

某个事件调用极频繁时，即使运行时禁用，也可能产生明显开销。
此时应声明 disable，使其在编译时采用 nop，
普通构建中完全没有性能影响，除非修改事件文件。

如果只有跟踪参数需要较复杂计算，可用 trace_event_get_state_backends()
保护计算，使事件在编译或运行时禁用时都跳过。
编译时禁用的情况下，此检查本身也没有性能影响。

::

    #include "trace.h"  /* needed for trace event prototype */
    
    void *qemu_vmalloc(size_t size)
    {
        void *ptr;
        size_t align = QEMU_VMALLOC_ALIGN;
    
        if (size < align) {
            align = getpagesize();
        }
        ptr = qemu_memalign(align, size);
        if (trace_event_get_state_backends(TRACE_QEMU_VMALLOC)) {
            void *complex;
            /* some complex computations to produce the 'complex' value */
            trace_qemu_vmalloc(size, ptr, complex);
        }
        return ptr;
    }
