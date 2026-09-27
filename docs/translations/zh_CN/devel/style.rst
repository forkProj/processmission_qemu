.. _coding-style:

=============
QEMU 编码风格
=============

.. contents:: 目录

提交前请用 scripts/checkpatch.pl 检查补丁。

格式与风格
**********

仓库的 .editorconfig 可帮助编辑器采用正确设置，
详情见 `<https://editorconfig.org/>`_。

空白
========

任何编码风格最重要的当然都是空白。
老程序员连鼻梁上的眼镜都找不到，却能在十五秒差距之外分辨制表符和八个空格。
无数争论都曾围绕此事展开。

QEMU 使用四个空格缩进，绝不使用制表符，
只有语法已无法挽回地要求制表符的 Makefile 例外。
空格优于制表符，因为：

* 只有一种表达空白的方式，减少歧义和错误。
* 不再纠结“制表符缩进、空格对齐”。
* 制表符把代码推得太靠右，使屏幕严重失衡。
* 编辑器未正确配置八列制表位时，会显示错误。
* 补丁中制表符显示不好，几乎每行都可能产生偏一列的问题。
* 这是 QEMU 编码风格。

不得保留行尾空白。

多行缩进
--------

以下位置需要缩进：

* if/else；
* while/for；
* 函数定义和调用。

长行折行后，后续行必须适当缩进。
if/else、while/for 的后续行与首行左括号之后对齐。例如：

.. code-block:: c

    if (a == 1 &&
        b == 2) {

    while (a == 1 &&
           b == 2) {


函数有两种形式：

* 相对行首缩进四格；
* 与首行左括号之后对齐。

例如：

.. code-block:: c

    do_something(x, y,
        z);

    do_something(x, y,
                 z);

    do_something(x, do_another(y,
                               z));


行宽
========

应尽量将行限制在 80 个字符。

遇到函数或符号名很长的子系统时可能难以做到。
若在第 80 列换行明显更难读、更别扭，宁可保持一行；
85 字符的一行优于不自然的折行。

即使如此，也不要远超 80。
checkpatch 在 100 字符报警只是防止明显过长，不是建议目标。

理由：

* 有人喜欢在 24 英寸屏幕上铺满 6×4 个 80×24 xterm，并全用 vi。
  最好的惩罚就是让他们继续这么做。
* 合理行宽使代码，尤其补丁更易阅读，80 是传统值。
* 四格缩进让“左边空白太多”的借口不再成立。
* 这是 QEMU 编码风格。

命名
========

变量使用 lower_case_with_underscores，便于输入和阅读。
结构化类型采用 CamelCase，虽然难输入，但更醒目。
枚举和函数类型也采用 CamelCase。
标量类型采用以下划线分隔并以 _t 结尾的形式，类似 POSIX uint64_t。
最后这一约定与 POSIX 相冲突，未来可能改变。

变量命名约定
------------

常见 QEMU 类型有一些短变量名惯例。
架构无关 CPUState 指针通常叫 cs，具体 CPUArchState 指针通常叫 env。
设备模拟中的通用 DeviceState 通常叫 dev。

函数命名约定
------------

标准库或 GLib 的封装函数使用 ``qemu_`` 前缀，提醒读者并非原函数，
例如 qemu_strtol、qemu_mutex_lock。
其他跨代码库广泛使用的工具函数不应带前缀，
例如 pstrcpy、find_first_bit 等位操作。

修改全局模拟器状态的函数也用该前缀，例如 qemu_add_vm_change_state_handler。
若存在明显的子系统专用前缀，应优先使用。

文件或子系统公开、在头文件声明的函数，通常使用一致前缀表明归属，
例如 cputlb.c 的 ``tlb_``，或 cpus.c 的 ``cpu_``。

同一功能有持锁和不持锁两个版本时，预期调用者已持锁的版本通常加 _locked。

兼容性适配函数使用 _compat，通常不直接调用，而经预处理器别名映射到普通名称。
_impl 用于不直接调用、而经宏或内联函数使用的具体实现。

块结构
========

所有缩进语句都必须带花括号，即使只有一条语句。
左括号与控制流语句同一行；右括号与 else 同行，没有 else 时单独一行。例如：

.. code-block:: c

    if (a == 5) {
        printf("a was 5.\n");
    } else if (a == 6) {
        printf("a was 6.\n");
    } else {
        printf("a was something else entirely.\n");
    }


else if 视为单条语句，否则长条件链会不断增加缩进。

函数左括号是例外，出于传统和清晰度，单独一行：

.. code-block:: c

    void a_function(void)
    {
        do_something();
    }


一致的括号风格减少歧义，也避免增删行引发不必要改动。
此外，这就是 QEMU 风格。

声明
========

通常不允许块内声明与语句交错，声明应位于块开头。
为避免意外复用，可在 for 中声明循环变量：

.. code-block:: c

    for (int i = 0; i < ARRAY_SIZE(thing); i++) {
        /* do something loopy */
    }


偶尔允许 #ifdef/#ifndef 内声明作为例外：
如果更清楚，即使外面已有语句，也可在条件块开头声明。
不过通常更适合把整个条件块移到独立函数。

条件语句
========

变量与常量比较相等或不等时，常量放右侧，例如：

.. code-block:: c

    if (a == 1) {
        /* Reads like: "If a equals 1" */
        do_something();
    }


理由：if (1 == a) 这类尤达式条件难读。
好的编译器即使常量在右侧，也能对误把 == 写成 = 发出警告。

注释风格
========

使用传统 C 的 ``/* */``，避免 //。
后者在 C99 合法，这只是风格一致性要求，checkpatch 会提醒。

多行注释左侧应有星号，起止标记各占一行：

.. code-block:: c

    /*
     * like
     * this
     */


与 Linux 内核风格相同。

已有代码有时采用左侧无星号的 GNU 风格或其他形式。
新注释不要仿照；除非本来就要编辑该注释，也不必专门转换。

理由是保持一致，并便于从周围代码中辨识多行注释。

语言用法
********

预处理器
========

可变参数宏
----------

使用类似 C99 的形式：

.. code-block:: c

    #define DPRINTF(fmt, ...)                                       \
        do { printf("IRQ: " fmt, ## __VA_ARGS__); } while (0)


包含指令
--------

包含顺序如下：

.. code-block:: c

    #include "qemu/osdep.h"  /* Always first... */
    #include <...>           /* then system headers... */
    #include "..."           /* and finally QEMU headers. */


qemu/osdep.h 定义影响 stdint.h 等核心系统头行为的宏，必须最先包含，
确保外部库包含系统头时也得到 QEMU 依赖的宏。

不要从头文件包含 osdep.h，因为 .c 已经包含。

头文件通常应自行包含除 osdep.h 外所需的一切。
如有例外，必须在头文件中说明。
若只需要某头文件的 typedef，可以考虑将其移到 qemu/typedefs.h。

禁止循环包含。

用于代码生成的包含文件
----------------------

QEMU 广泛通过宏预处理器实例化相似函数。
不禁止这种做法，但它可能使调试和代码导航更难。
应考虑能否通过方便编译器常量折叠，或用 Python 生成便于 grep 的代码达到同样效果。

模板头文件应使用 .c.inc 或 .h.inc 后缀，明确表示为展开而包含。

C 类型
========

应使用正确类型，以下是一些实用建议。

标量
--------

准备使用 int 或 long 时，很可能存在更适当的类型。
计数变量应使用无符号类型。

主机内存大小通常使用 size_t，只有必要时用 ssize_t。
客户机 RAM 偏移必须用 ram_addr_t，但它只覆盖 RAM，不一定覆盖全部客户机地址空间。

文件大小或有符号文件偏移使用 off_t。
只计数较小数值时用 unsigned int；除少见嵌入式系统外，可假设至少四字节。

要求特定位宽时使用 int32_t、uint32_t、uint64_t 等标准类型。
VMState 字段必须使用这些固定宽度类型。

不要使用 Linux 内部类型 u32、__u32、__le32。

客户机物理地址用 hwaddr，PCI 地址例外，使用 pcibus_t。
ram_addr_t 是 QEMU 内部的中间地址空间，将客户机 RAM 物理地址映射到主机虚拟地址。
客户机内存大小通常可放入它，但不能用它保存真实客户机物理地址。

CPU 虚拟地址有几种类型：
vaddr 最适合目标无关代码，足以容纳任何目标的虚拟地址，大小不随目标变化，且无符号。
target_ulong 大小与目标 CPU 虚拟地址相同，可能为 32 或 64 位，
因此只用于目标专用代码，以及 TLB 等性能关键、按目标编译的核心代码。
对应有符号版本是 target_long。
abi_ulong 用于用户模式目标，大小与目标 ABI 的 void 指针相同。
例如 sparc32plus 在 64 位 CPU 上使用 32 位指针，可能不同于完整虚拟地址宽度。
必须匹配目标 ABI 的结构，对目标 unsigned long 或指针字段应使用该类型。
有符号版本为 abi_long。

这些建议需要结合情况。
调用要求 size_t、pid_t、off_t 等类型的系统接口时，相关变量应匹配接口。
若改用 unsigned int 与相关变量符号属性冲突，
而全面修正会牵涉过大，也可能宁可暂时使用“不理想”的类型。

描述性类型很重要，但不要过度。
如果造成警告或需要强制转换，应重新考虑或寻求帮助。

指针
--------

所有指针应保证 const 正确性。
不用于修改目标存储时，加 const，让读者立即知道它只读。
坚持这一点后，非 const 指针就明确表示会修改目标，
或作为另一可修改指针的别名。

Typedef
--------

typedef 用于消除冗余 struct 关键字，
因为类型用 CamelCase，与其他 snake_case 标识符已有区别。
每个命名结构应有 CamelCase 名称和对应 typedef。

部分编译器不接受重复 typedef，因此只在一个头文件声明。
公共类型可放 include/qemu/typedefs.h。
为方便起见，头文件和原型也可直接使用结构前向声明，
避免重复 typedef，并减少头文件之间的包含。

位域
--------

C 位域可能导致不可移植，尤其 Windows 上
`MSVC 与 GCC 的布局不同 <https://gcc.gnu.org/onlinedocs/gcc/x86-Type-Attributes.html>`_，
或需要考虑字节序时。

因此，紧凑结构和必须精确匹配客户机内存布局的结构禁止使用位域。
已有代码可能使用，但其布局已经仔细确认。

即使布局无关，也建议避免位域，除非能证明有显著易用性收益。
建议改用安全的 include/hw/core/registerfields.h。

C 与 POSIX 保留命名空间
-----------------------

避免下划线加大写、双下划线，以及 _t 后缀。

底层内存管理
============

禁止使用 malloc/free/realloc/calloc/valloc/memalign/posix_memalign，
以及 alloca/g_alloca/g_newa/g_newa0。
应使用 GLib 的 g_malloc/g_malloc0/g_new/g_new0/g_realloc/g_free，
或 QEMU 的 qemu_memalign/qemu_blockalign/qemu_vfree。

g_malloc 分配失败会退出，因此无需像 malloc 那样检查。
启动时通常可以使用，因为失败本来就致命；
但推测性加载大型调试符号表等启动场景，也可能不应因此退出。

避免让客户机通过触发大额分配导致 QEMU 退出。
约 4K 的小额分配失败通常意味着主机过载，允许退出合理。
较大分配若可回退到更小方案，应使用 g_try_new 并检查结果，
例如 SoftMMU TLB 的 tlb_mmu_resize_locked 所做的时间／空间权衡。

分配生命周期只限当前函数，且有多个退出路径时，可用 g_autofree 等改善可读性，
见 :ref:`autofree-ref`。

g_malloc(0) 合法并返回 NULL。

优先 ``g_new(T, n)``，而非 ``g_malloc(sizeof(T) * n)``，因为：

* 检测乘法是否溢出 size_t；
* 返回 T 指针而非 void 指针，编译器能发现更多类型错误。

不过以下声明也可接受：

.. code-block:: c

    T *v = g_malloc(sizeof(*v))


qemu_memalign 或 qemu_blockalign 分配的内存必须用 qemu_vfree 释放，
否则会在 Win32 出现问题。

字符串处理
==========

不要使用 strncpy。它不保证 NUL 结尾，十分危险，
还会将目标剩余字节一直清零到指定长度。
尽可能使用以下类似函数，但注意签名不同：

.. code-block:: c

    void pstrcpy(char *dest, int dest_buf_size, const char *src)


不要使用无法检查缓冲区溢出的 strcat，改用：

.. code-block:: c

    char *pstrcat(char *buf, int buf_size, const char *s)


sprintf、vsprintf 有同样问题，应改用 snprintf、vsnprintf。

QEMU 还提供其他实用字符串函数：

.. code-block:: c

    int strstart(const char *str, const char *val, const char **ptr)
    int stristart(const char *str, const char *val, const char **ptr)


字符处理也有替代 isxyz/toxyz 的宏，例如使用 qemu_isalnum 而非 isalnum。
根据内存管理规则，必须使用 g_strdup/g_strndup，而非普通 strdup/strndup。

Printf 风格函数
===============

新增带格式字符串及后续可变参数的函数时，
必须在原型上使用 GCC printf 属性，
使 -Wformat 和 -Wformat-security 能检查格式与参数数量、类型是否一致。

C 标准及实现定义、未定义行为
============================

QEMU C 代码应符合 C11，最终标准的草案形式可从以下位置下载：

    `<http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1548.pdf>`_

C 规范保留未定义和实现定义行为，为编译器优化提供空间。
通常代码应遵守规范，避免两者；“在我测试的 GCC 上没问题”不是有效理由。
不过部分行为在所有关注的平台上都一致，而严格可移植写法很麻烦，因此允许假定：

* 整数采用二进制补码；
* 有符号整数右移复制符号位，即算术右移。

另外，QEMU 假定编译器不利用 C99/C11 将部分有符号左移视为未定义行为的自由，
相关行为自 GCC 4.0 起记载于其手册。

.. _autofree-ref:

自动释放内存
============

QEMU 必须使用 GCC 或 Clang，因此可以使用 C 扩展，
在栈变量离开作用域时自动调用清理函数。
自动释放内存能简化清理路径，常可减少大量 goto。

GLib2 提供相应函数和宏：

  `<https://developer.gnome.org/glib/stable/glib-Miscellaneous-Macros.html>`_

主要包括：

* g_autofree：离开作用域时调用 g_free()。

* g_autoptr：对结构或对象调用通过 G_DEFINE_AUTOPTR_CLEANUP_FUNC
  定义的清理函数，大多数 GLib 类型和 GObject 均支持。

例如，以下代码：

.. code-block:: c

    int somefunc(void)
    {
        int ret = -1;
        char *foo = g_strdup_printf("foo%", "wibble");
        GList *bar = .....

        if (eek) {
           goto cleanup;
        }

        ret = 0;

      cleanup:
        g_free(foo);
        g_list_free(bar);
        return ret;
    }


可以改写为：

.. code-block:: c

    int somefunc(void)
    {
        g_autofree char *foo = g_strdup_printf("foo%", "wibble");
        g_autoptr (GList) bar = .....

        if (eek) {
           return -1;
        }

        return 0;
    }


虽然通常更简单、更不易泄漏，仍需注意：

* ``g_auto*`` 声明的变量必须初始化，否则清理函数会使用未初始化栈内存。

* 若其中的值需要超出函数生命周期，必须先保存该值，再把原变量置 NULL。
  g_steal_pointer 可以简化操作。

.. code-block:: c

    char *somefunc(void)
    {
        g_autofree char *foo = g_strdup_printf("foo%", "wibble");
        g_autoptr (GList) bar = .....

        if (eek) {
           return NULL;
        }

        return g_steal_pointer(&foo);
    }



QEMU 专用惯用法
***************

QEMU 对象模型声明
=================

QOM 为基础 C 提供对象框架。
实例存储结构或类结构的第一个成员必须是父对象，并与新成员之间留空行。
属性后备字段，即用户选项，与内部状态也宜分开，便于阅读。

实例结构首成员应叫 parent_obj，类结构首成员应叫 parent_class，例如：

.. code-block:: c

    struct MyDeviceState {
        DeviceState parent_obj;

        /* Properties */
        int prop_a;
        char *prop_b;
        /* Other stuff */
        int internal_state;
    };

    struct MyDeviceClass {
        DeviceClass parent_class;

        void (*new_fn1)(void);
        bool (*new_fn2)(CPUState *);
    };


QOM 结构无需手动 typedef，声明宏会自动生成，详情见 :ref:`qom`。

QEMU GUARD 宏
=============

``_GUARD`` 宏简化多退出路径处理。
例如用 QEMU_LOCK_GUARD 加锁，会确保函数退出时释放：

.. code-block:: c

    static int my_critical_function(SomeState *s, void *data)
    {
        QEMU_LOCK_GUARD(&s->lock);
        do_thing1(data);
        if (check_state2(data)) {
            return -1;
        }
        do_thing3(data);
        return 0;
    }


无论怎样退出，s->lock 都会释放。
不使用宏的等效代码必须在每个出口正确调用 qemu_mutex_unlock()：

.. code-block:: c

    static int my_critical_function(SomeState *s, void *data)
    {
        qemu_mutex_lock(&s->lock);
        do_thing1(data);
        if (check_state2(data)) {
            qemu_mutex_unlock(&s->lock);
            return -1;
        }
        do_thing3(data);
        qemu_mutex_unlock(&s->lock);
        return 0;
    }


通常还有 ``WITH_`` 形式，便于包围函数内部代码块：

.. code-block:: c

    WITH_RCU_READ_LOCK_GUARD() {
        QTAILQ_FOREACH_RCU(kid, &bus->children, sibling) {
            err = do_the_thing(kid->child);
            if (err < 0) {
                return err;
            }
        }
    }


错误处理和报告
==============

向用户报告错误
--------------

不要使用 printf()、fprintf() 或 monitor_printf()，
应使用 error-report.h 的 error_report() 或 error_vreport()，
确保在正确位置，即当前监控器或标准错误，以统一格式报告。

额外信息用 error_printf() 等输出。

error_report() 输出当前位置。命令行解析等常见场景会自动跟踪位置，
手动调整则使用 error-report.h 的 ``loc_*()``。

传播错误
--------

错误常不能在发现处直接报告，而需沿调用链传到合适的处理位置，方式有多种。
最灵活的是 Error 对象，用法见 error.h。

采用最简单合适的成功／失败传递方式，遵循常见约定：
非负／-1、非负／-errno、非空／NULL，或 Error 对象。

例如函数成功返回指针，且对调用者只有一种失败方式时，
失败返回 NULL 就足够，比传递 Error 对象简单清楚。
若调用者需要只有函数内部才知道的详细原因，则用 ``Error **`` 设置适当错误。

不要一边返回错误供其他代码处理，一边向用户报告。
应由最终消费错误的位置负责报告。

处理错误
--------

启动配置错误时可以调用 exit()，正常运行时则有问题，监控器命令绝不能 exit()。

客户机可触发的错误，例如未实现的翻译或设备模拟边界情况，
不得通过 exit() 或 abort() 处理，客户机不应有能力终止 QEMU。

&error_fatal 等价于 exit(1)，&error_abort 等价于 abort()。


trace-events 风格
=================

0x 前缀
--------

跟踪事件中，十六进制应带 0x，例如：

.. code-block:: c

    some_trace(unsigned x, uint64_t y) "x 0x%x y 0x" PRIx64


按惯例以十六进制表示、用点、斜杠、冒号或空格分隔的数字组可例外，
例如 PCI 总线 ID：

.. code-block:: c

    another_trace(int cssid, int ssid, int dev_num) "bus id: %x.%x.%04x"


这类数字组也可带 0x。无论如何，应让十六进制含义清楚，例如：

.. code-block:: c

    data_dump(uint8_t c1, uint8_t c2, uint8_t c3) "bytes (in hex): %02x %02x %02x"


没有前缀时，十六进制日志难读，尤其数字中恰好没有字母，或同一行还包含十进制。
允许数字组省略，是因为 %x.%x.%x 等惯例不只 QEMU 使用。
原始字节转储逐字节加 0x 也更难读。

printf 的 # 标志
----------------

不要使用 %#x 等 # 格式标志。

添加前缀有 0x%... 和 %#... 两种方式，为一致应统一使用前者，因为：

* 更常见；
* %# 对零值省略 0x，使输出不一致。
