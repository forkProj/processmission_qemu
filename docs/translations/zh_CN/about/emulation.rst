模拟
========

QEMU 的 Tiny Code Generator（TCG）可以在任意受支持的宿主平台上模拟多种 CPU 架构。
根据客户机架构，支持 :ref:`System Emulation` 和 :ref:`User Mode Emulation`。

.. list-table:: 支持模拟的客户机架构
  :widths: 30 10 10 50
  :header-rows: 1

  * - 架构（QEMU 名称）
    - 系统模式
    - 用户模式
    - 说明
  * - Alpha
    - 是
    - 是
    - DEC 开发的早期 64 位 RISC 指令集
  * - Arm (arm, aarch64)
    - :ref:`是 <ARM-System-emulator>`
    - 是
    - 支持广泛特性，详见 :ref:`Arm Emulation`
  * - AVR
    - :ref:`是 <AVR-System-emulator>`
    - 否
    - 常用于创客项目的 8 位微控制器
  * - Hexagon
    - 否
    - 是
    - Qualcomm 的 DSP 系列
  * - PA-RISC (hppa)
    - 是
    - 是
    - HP 早期小型机使用的 RISC 系统
  * - x86 (i386, x86_64)
    - :ref:`是 <QEMU-PC-System-emulator>`
    - 是
    - 广泛使用的桌面 PC 架构，支持 32 位和 64 位
  * - LoongArch
    - 是
    - 是
    - 中国开发的、类似 MIPS 的 64 位 RISC 架构
  * - m68k
    - :ref:`是 <ColdFire-System-emulator>`
    - 是
    - Motorola 68000 的变体及 ColdFire
  * - MCS-51 系列 (mcs51, mcs251)
    - :ref:`是 <MCS51-System-emulator>`
    - 否
    - 8 位微控制器及其 MCS-251 扩展
  * - Microblaze
    - 是
    - 是
    - Xilinx 的 RISC 软核
  * - MIPS (mips*)
    - :ref:`是 <MIPS-System-emulator>`
    - 是
    - 起源于斯坦福大学的经典 RISC 架构
  * - OpenRISC
    - :ref:`是 <OpenRISC-System-emulator>`
    - 是
    - OpenRISC 社区开发的开源 RISC 架构
  * - Power (ppc, ppc64)
    - :ref:`是 <PowerPC-System-emulator>`
    - 是
    - 目前由 IBM 管理的通用 RISC 架构
  * - RISC-V
    - :ref:`是 <RISC-V-System-emulator>`
    - 是
    - RISC-V International 维护的开放标准 RISC 指令集
  * - RX
    - :ref:`是 <RX-System-emulator>`
    - 否
    - Renesas 开发的 32 位微控制器
  * - s390x
    - :ref:`是 <s390x-System-emulator>`
    - 是
    - IBM System Z 大型机的 64 位 CPU
  * - sh4
    - 是
    - 是
    - Hitachi 开发的 32 位 RISC 嵌入式 CPU
  * - SPARC (sparc, sparc64)
    - :ref:`是 <Sparc32-System-emulator>`
    - 是
    - 最初由 Sun Microsystems 开发的 RISC 指令集
  * - Tricore
    - 是
    - 否
    - Infineon 开发的 32 位 RISC、微控制器和 DSP 架构
  * - Xtensa
    - :ref:`是 <Xtensa-System-emulator>`
    - 是
    - 现由 Cadence 拥有的可配置 32 位软核

.. _Semihosting:

半主机（Semihosting）
---------------------

半主机是由架构设计方定义的功能，允许程序与调试主机交互。
真实硬件上，通常由直接连接开发板的在线仿真器（ICE）提供。
QEMU 可以将半主机调用传给宿主系统，也可以通过 ``gdbstub`` 传递。

在尚未启用完整操作系统时，半主机有助于启动和调试底层代码。
它也允许通常没有完整 libc 的嵌入式微控制器代码，
作为裸机代码在 QEMU 用户态模拟中运行。
半主机还适合编写测试：许多编译器套件及 QEMU 自身都用它退出测试并报告成功状态。

只有 TCG 模拟支持半主机，因为触发调用的指令通常属于保留指令，
大多数虚拟机监控器会将其捕获为异常。

.. warning::
   半主机从机制上绕过客户机与宿主之间的隔离，因此程序可能损坏宿主系统。
   某些调用，例如 ``SYS_READC``，可能无限期阻塞执行。
   只应对可信代码启用半主机。

重定向
~~~~~~~~

调试时，可通过 :ref:`gdbstub <GDB usage>` 将半主机调用重定向到本地或远程 GDB。
半主机控制台输出配置为 ``chardev``，因此可以像其他字符设备一样，
重定向到文件、管道或 socket。

支持的目标
~~~~~~~~~~

多数目标的半主机实现相似，仅触发调用的指令编码和保存参数的寄存器略有差异。
通常提供简单的类 POSIX API，用于读写文件、访问控制台等基本交互。

特定目标的完整 ABI 和调用集合，请参阅对应架构的半主机规范。

.. note::
   QEMU 将全部文件访问实现为 ``O_BINARY`` 模式。
   因此，无论程序指定文本还是二进制模式，输入输出都不会转换行结束符。
   原因是 GDB 半主机支持不区分这两种模式，自动处理行结束符容易造成困惑。

.. list-table:: 支持半主机的客户机架构
  :widths: 10 10 80
  :header-rows: 1

  * - 架构
    - 模式
    - 规范
  * - Arm
    - 系统模式和用户模式
    - https://github.com/ARM-software/abi-aa/blob/main/semihosting/semihosting.rst
  * - m68k
    - 系统模式
    - https://sourceware.org/git/?p=newlib-cygwin.git;a=blob;f=libgloss/m68k/m68k-semi.txt;hb=HEAD
  * - MIPS
    - 系统模式
    - Unified Hosting Interface (MD01069)
  * - RISC-V
    - 系统模式和用户模式
    - https://github.com/riscv-non-isa/riscv-semihosting/blob/main/riscv-semihosting.adoc
  * - Xtensa
    - 系统模式
    - Tensilica ISS SIMCALL

.. _tcg-plugins:

TCG 插件
--------

TCG 插件让用户利用模拟器对客户机的完整控制进行实验。
插件可以订阅翻译和执行过程中的事件，并在事件发生时接收回调。
TCG 插件只能被动观察，不能修改系统状态，
但可以精确到单条指令，甚至订阅全部加载和存储操作。

编写方法见开发者手册的 :ref:`插件编写 <TCG Plugins>`。

用法
~~~~~~~~

支持 TCG 的 QEMU 二进制默认启用插件。早期版本需要显式配置：::

  configure --enable-plugins


构建后，可以加载多个插件，并分别提供参数：::

  $QEMU $OTHER_QEMU_ARGS \
      -plugin contrib/plugins/libhowvec.so,inline=on,count=hint \
      -plugin contrib/plugins/libhotblocks.so


参数由各插件定义，用于调整行为。
上例要求 howvec 使用内联操作，对 hint 指令按类型统计和细分。

Linux 用户态模拟也会读取 ``QEMU_PLUGIN`` 环境变量：::

  QEMU_PLUGIN="file=contrib/plugins/libhowvec.so,inline=on,count=hint" $QEMU


QEMU 插件避免直接写入 stdin/stderr，而使用 API 提供的日志接口，
见 ``qemu_plugin_outs``。若要显示输出，可以增加：::

  $QEMU $OTHER_QEMU_ARGS \
    -d plugin \
    -plugin contrib/plugins/libhowvec.so,inline=on,count=hint


示例插件
~~~~~~~~

QEMU 自带多个插件，也欢迎向上游贡献自己的插件，放入 ``contrib/plugins``。
``tests/tcg/plugins`` 中还提供一些基本插件，用于 ``make check-tcg`` 测试 API，
它们也适合进行基本分析。

空插件
........

``tests/tcg/plugins/empty.c``

纯粹用于测量插件系统自身开销，不执行插桩。

基本块
........

``tests/tcg/plugins/bb.c``

一个简单插件，在每次基本块执行时进行粗粒度统计。
默认在运行结束后显示结果：::

  $ qemu-aarch64 -plugin tests/plugin/libbb.so \
      -d plugin ./tests/tcg/aarch64-linux-user/sha1
  SHA1=15dd99a1991e0b3826fede3deffc1feba42278e6
  bb's: 2277338, insns: 158483046


可通过以下参数调整行为：

.. list-table:: 基本块插件参数
  :widths: 20 80
  :header-rows: 1

  * - 选项
    - 说明
  * - inline=true|false
    - 使用更快的内联单计数器加法
  * - idle=true|false
    - 每当客户机 vCPU 空闲时输出当前执行统计

基本块向量
..........

``contrib/plugins/bbv.c``

bbv 生成可用于 `SimPoint <https://cseweb.ucsd.edu/~calder/simpoint/>`__ 分析的基本块向量。

.. list-table:: 基本块向量参数
  :widths: 20 80
  :header-rows: 1

  * - 选项
    - 说明
  * - interval=N
    - 以指令数指定生成向量的间隔，默认 N = 100000000
  * - outfile=PATH
    - 输出文件路径，追加 ``.N.bb`` 后缀，其中 N 为 vCPU 索引

示例：::

  $ qemu-aarch64 \
    -plugin contrib/plugins/libbbv.so,interval=100,outfile=sha1 \
    tests/tcg/aarch64-linux-user/sha1
  SHA1=15dd99a1991e0b3826fede3deffc1feba42278e6
  $ du sha1.0.bb
  23128   sha1.0.bb


指令
........

``tests/tcg/plugins/insn.c``

基本的指令级插桩，可统计每个核心或线程执行的指令数：::

  $ qemu-aarch64 -plugin tests/plugin/libinsn.so \
      -d plugin ./tests/tcg/aarch64-linux-user/threadcount
  Created 10 threads
  Done
  cpu 0 insns: 46765
  cpu 1 insns: 3694
  cpu 2 insns: 3694
  cpu 3 insns: 2994
  cpu 4 insns: 1497
  cpu 5 insns: 1497
  cpu 6 insns: 1497
  cpu 7 insns: 1497
  total insns: 63135


可通过以下参数调整行为：

.. list-table:: 指令插件参数
  :widths: 20 80
  :header-rows: 1

  * - 选项
    - 说明
  * - inline=true|false
    - 使用更快的内联单计数器加法
  * - sizes=true|false
    - 汇总运行期间的指令长度
  * - match=<string>
    - 仅对匹配字符串前缀的指令插桩

``match`` 会显示基本统计，例如距离上一次执行已运行多少条指令：::

   $ qemu-aarch64 -plugin tests/plugin/libinsn.so,match=bl \
       -d plugin ./tests/tcg/aarch64-linux-user/sha512-vector
   ...
   0x40069c, 'bl #0x4002b0', 10 hits, 1093 match hits, Δ+1257 since last match, 98 avg insns/match
   0x4006ac, 'bl #0x403690', 10 hits, 1094 match hits, Δ+47 since last match, 98 avg insns/match
   0x4037fc, 'bl #0x4002b0', 18 hits, 1095 match hits, Δ+22 since last match, 98 avg insns/match
   0x400720, 'bl #0x403690', 10 hits, 1096 match hits, Δ+58 since last match, 98 avg insns/match
   0x4037fc, 'bl #0x4002b0', 19 hits, 1097 match hits, Δ+22 since last match, 98 avg insns/match
   0x400730, 'bl #0x403690', 10 hits, 1098 match hits, Δ+33 since last match, 98 avg insns/match
   0x4037ac, 'bl #0x4002b0', 12 hits, 1099 match hits, Δ+20 since last match, 98 avg insns/match
   ...


更详细的执行跟踪选项见 ``execlog`` 插件。

内存
........

``tests/tcg/plugins/mem.c``

基本的指令级内存插桩：::

  $ qemu-aarch64 -plugin tests/plugin/libmem.so,inline=true \
      -d plugin ./tests/tcg/aarch64-linux-user/sha1
  SHA1=15dd99a1991e0b3826fede3deffc1feba42278e6
  inline mem accesses: 79525013


可通过以下参数调整行为：

.. list-table:: 内存插件参数
  :widths: 20 80
  :header-rows: 1

  * - 选项
    - 说明
  * - inline=true|false
    - 使用更快的内联单计数器加法
  * - callback=true|false
    - 每次内存插桩都使用回调
  * - hwaddr=true|false
    - 统计 I/O 访问，仅用于系统模拟

系统调用
........

``tests/tcg/plugins/syscall.c``

基本系统调用跟踪插件，仅适用于用户态。
默认在运行结束时汇总系统调用统计：::

  $ qemu-aarch64 -plugin tests/plugin/libsyscall \
      -d plugin ./tests/tcg/aarch64-linux-user/threadcount
  Created 10 threads
  Done
  syscall no.  calls  errors
  226          12     0
  99           11     11
  115          11     0
  222          11     0
  93           10     0
  220          10     0
  233          10     0
  215          8      0
  214          4      0
  134          2      0
  64           2      0
  96           1      0
  94           1      0
  80           1      0
  261          1      0
  78           1      0
  160          1      0
  135          1      0


可通过以下参数调整行为：

.. list-table:: 系统调用插件参数
  :widths: 20 80
  :header-rows: 1

  * - 选项
    - 说明
  * - print=true|false
    - 打印每个系统调用的调用次数
  * - log_writes=true|false
    - 以 hexdump 格式记录每次 write 系统调用的缓冲区

测试内联操作
............

``tests/plugins/inline.c``

此插件测试全部内联操作、条件回调和 scoreboard，并按 CPU 输出事件摘要。

热点基本块
..........

``contrib/plugins/hotblocks.c``

hotblocks 用于查找程序中的热点执行路径。
程序结束后，会按排序列出基本块的起始 PC、翻译次数、指令数和执行次数。
最适合 linux-user，因为系统模拟中不同程序的块换入换出系统内存时，往往会重新翻译。

示例：::

  $ qemu-aarch64 \
    -plugin contrib/plugins/libhotblocks.so -d plugin \
    ./tests/tcg/aarch64-linux-user/sha1
  SHA1=15dd99a1991e0b3826fede3deffc1feba42278e6
  collected 903 entries in the hash table
  pc, tcount, icount, ecount
  0x0000000041ed10, 1, 5, 66087
  0x000000004002b0, 1, 4, 66087
  ...


可通过以下参数调整行为：

.. list-table:: 热点基本块插件参数
  :widths: 20 80
  :header-rows: 1

  * - 选项
    - 说明
  * - inline=true|false
    - 使用更快的内联单计数器加法
  * - limit=N
    - 打印的块数，默认 N = 20，0 表示不限制

热点页面
........

``contrib/plugins/hotpages.c``

类似 hotblocks，但跟踪内存访问：::

  $ qemu-aarch64 \
    -plugin contrib/plugins/libhotpages.so -d plugin \
    ./tests/tcg/aarch64-linux-user/sha1
  SHA1=15dd99a1991e0b3826fede3deffc1feba42278e6
  Addr, RCPUs, Reads, WCPUs, Writes
  0x000055007fe000, 0x0001, 31747952, 0x0001, 8835161
  0x000055007ff000, 0x0001, 29001054, 0x0001, 8780625
  0x00005500800000, 0x0001, 687465, 0x0001, 335857
  0x0000000048b000, 0x0001, 130594, 0x0001, 355
  0x0000000048a000, 0x0001, 1826, 0x0001, 11


可使用以下参数配置：

.. list-table:: 热点页面参数
  :widths: 20 80
  :header-rows: 1

  * - 选项
    - 说明
  * - sortby=reads|writes|address
    - 按读取次数、写入次数或地址排序；默认按读写次数总和排序
  * - io=on
    - 跟踪 I/O 地址，仅适用于全系统模拟，默认关闭
  * - pagesize=N
    - 页面大小，默认 N = 4096

指令分布
........

``contrib/plugins/howvec.c``

这是一个指令分类器，可以统计不同指令类型，并用选项细化统计范围。
为 ``count`` 指定某类指令，可以对其作完整细分。
例如，查看所有系统寄存器访问：::

  $ qemu-system-aarch64 $(QEMU_ARGS) \
    -append "root=/dev/sda2 systemd.unit=benchmark.service" \
    -smp 4 -plugin ./contrib/plugins/libhowvec.so,count=sreg -d plugin


分类统计后会显示排序列表：::

  Instruction Classes:
  Class:   UDEF                   not counted
  Class:   SVE                    (68 hits)
  Class:   PCrel addr             (47789483 hits)
  Class:   Add/Sub (imm)          (192817388 hits)
  Class:   Logical (imm)          (93852565 hits)
  Class:   Move Wide (imm)        (76398116 hits)
  Class:   Bitfield               (44706084 hits)
  Class:   Extract                (5499257 hits)
  Class:   Cond Branch (imm)      (147202932 hits)
  Class:   Exception Gen          (193581 hits)
  Class:     NOP                  not counted
  Class:   Hints                  (6652291 hits)
  Class:   Barriers               (8001661 hits)
  Class:   PSTATE                 (1801695 hits)
  Class:   System Insn            (6385349 hits)
  Class:   System Reg             counted individually
  Class:   Branch (reg)           (69497127 hits)
  Class:   Branch (imm)           (84393665 hits)
  Class:   Cmp & Branch           (110929659 hits)
  Class:   Tst & Branch           (44681442 hits)
  Class:   AdvSimd ldstmult       (736 hits)
  Class:   ldst excl              (9098783 hits)
  Class:   Load Reg (lit)         (87189424 hits)
  Class:   ldst noalloc pair      (3264433 hits)
  Class:   ldst pair              (412526434 hits)
  Class:   ldst reg (imm)         (314734576 hits)
  Class: Loads & Stores           (2117774 hits)
  Class: Data Proc Reg            (223519077 hits)
  Class: Scalar FP                (31657954 hits)
  Individual Instructions:
  Instr: mrs x0, sp_el0           (2682661 hits)  (op=0xd5384100/  System Reg)
  Instr: mrs x1, tpidr_el2        (1789339 hits)  (op=0xd53cd041/  System Reg)
  Instr: mrs x2, tpidr_el2        (1513494 hits)  (op=0xd53cd042/  System Reg)
  Instr: mrs x0, tpidr_el2        (1490823 hits)  (op=0xd53cd040/  System Reg)
  Instr: mrs x1, sp_el0           (933793 hits)   (op=0xd5384101/  System Reg)
  Instr: mrs x2, sp_el0           (699516 hits)   (op=0xd5384102/  System Reg)
  Instr: mrs x4, tpidr_el2        (528437 hits)   (op=0xd53cd044/  System Reg)
  Instr: mrs x30, ttbr1_el1       (480776 hits)   (op=0xd538203e/  System Reg)
  Instr: msr ttbr1_el1, x30       (480713 hits)   (op=0xd518203e/  System Reg)
  Instr: msr vbar_el1, x30        (480671 hits)   (op=0xd518c01e/  System Reg)
  ...


目前，类别参数的简写需要查看插件源码，
具体是 InsnClassExecCount 表中的 ``*opt`` 参数。

锁步执行
........

``contrib/plugins/lockstep.c``

此调试工具用于定位 TCG 代码生成作出细微修改后，执行从何时何处开始分歧。
它并不精确，引入异步事件后结果可能不稳定。
虽然 -icount 可以使执行流具备确定性，但不保证翻译序列完全相同。
常见原因是服务 GUI 的定时器触发，导致基本块提前结束。
不过，它有时仍能指出大致分歧位置。
插件只需要一个参数：两个实例通信所用的 socket 路径：::

  $ qemu-system-sparc -monitor none -parallel none \
    -net none -M SS-20 -m 256 -kernel day11/zImage.elf \
    -plugin ./contrib/plugins/liblockstep.so,sockpath=lockstep-sparc.sock \
    -d plugin,nochain


最终会报告：::

  qemu-system-sparc: warning: nic lance.0 has no peer
  @ 0x000000ffd06678 vs 0x000000ffd001e0 (2/1 since last)
  @ 0x000000ffd07d9c vs 0x000000ffd06678 (3/1 since last)
  Δ insn_count @ 0x000000ffd07d9c (809900609) vs 0x000000ffd06678 (809900612)
    previously @ 0x000000ffd06678/10 (809900609 insns)
    previously @ 0x000000ffd001e0/4 (809900599 insns)
    previously @ 0x000000ffd080ac/2 (809900595 insns)
    previously @ 0x000000ffd08098/5 (809900593 insns)
    previously @ 0x000000ffd080c0/1 (809900588 insns)



硬件访问分析
............

``contrib/plugins/hwprofile.c``

hwprofile 仅用于系统模拟，可以查看各硬件被访问的频率。
支持以下选项：

.. list-table:: 硬件访问分析参数
  :widths: 20 80
  :header-rows: 1

  * - 选项
    - 说明
  * - track=[read|write]
    - 默认同时跟踪读写；此选项可限制为一种访问类型
  * - source
    - 按发起访问的客户机 PC 详细分类，与 pattern 不兼容。示例输出：::

        cirrus-low-memory @ 0xfffffd00000a0000
         pc:fffffc0000005cdc, 1, 256
         pc:fffffc0000005ce8, 1, 256
         pc:fffffc0000005cec, 1, 256


  * - pattern
    - 改为按硬件区域内偏移分类，有助于发现设备最常用的寄存器。示例输出：::

        pci0-conf @ 0xfffffd01fe000000
          off:00000004, 1, 1
          off:00000010, 1, 3
          off:00000014, 1, 3
          off:00000018, 1, 2
          off:0000001c, 1, 2
          off:00000020, 1, 2
          ...



执行日志
........

``contrib/plugins/execlog.c``

execlog 跟踪执行的指令及内存访问，可用于调试和安全分析，但会产生大量输出。

插件的默认使用参数如下：::

  $ qemu-system-arm $(QEMU_ARGS) \
    -plugin ./contrib/plugins/libexeclog.so -d plugin


生成的执行跟踪遵循以下结构：::

  # vCPU, vAddr, opcode, disassembly[, load/store, memory addr, device]...
  0, 0xa12, 0xf8012400, "movs r4, #0"
  0, 0xa14, 0xf87f42b4, "cmp r4, r6"
  0, 0xa16, 0xd206, "bhs #0xa26"
  0, 0xa18, 0xfff94803, "ldr r0, [pc, #0xc]", load, 0x00010a28, RAM
  0, 0xa1a, 0xf989f000, "bl #0xd30"
  0, 0xd30, 0xfff9b510, "push {r4, lr}", store, 0x20003ee0, RAM, store, 0x20003ee4, RAM
  0, 0xd32, 0xf9893014, "adds r0, #0x14"
  0, 0xd34, 0xf9c8f000, "bl #0x10c8"
  0, 0x10c8, 0xfff96c43, "ldr r3, [r0, #0x44]", load, 0x200000e4, RAM


要获得反汇编信息，QEMU 必须配置 Capstone 支持。

使用 ``ifilter`` 或 ``afilter``，可以只跟踪特定指令或地址。
必要时可以叠加参数：::

  $ qemu-system-arm $(QEMU_ARGS) \
    -plugin ./contrib/plugins/libexeclog.so,ifilter=st1w,afilter=0x40001808 -d plugin


插件也可以在寄存器值改变时输出寄存器。
通过多个 ``reg`` 选项指定名称，也可以使用 glob 通配符：::

  $ qemu-system-arm $(QEMU_ARGS) \
    -plugin ./contrib/plugins/libexeclog.so,reg=\*_el2,reg=sp -d plugin


每增加一个待检查寄存器，都会明显降低速度。
可以使用 rdisas，只对反汇编中提及这些寄存器的指令插桩，减少检查次数。
此方法并非完全可靠，因为某些指令会隐式修改寄存器；
可以用 ifilter 捕获这类情况：::

  $ qemu-system-arm $(QEMU_ARGS) \
    -plugin ./contrib/plugins/libexeclog.so,ifilter=msr,ifilter=blr,reg=x30,reg=\*_el1,rdisas=on


缓存建模
........

``contrib/plugins/cache.c``

此插件在运行工作集时，测量指定 L1 缓存配置的性能，
还可为每个核心模拟统一的 L2 缓存：::

  $ qemu-x86_64 -plugin ./contrib/plugins/libcache.so \
      -d plugin -D cache.log ./tests/tcg/x86_64-linux-user/float_convs


输出如下：::

    core #, data accesses, data misses, dmiss rate, insn accesses, insn misses, imiss rate
    0       996695         508             0.0510%  2642799        18617           0.7044%

    address, data misses, instruction
    0x424f1e (_int_malloc), 109, movq %rax, 8(%rcx)
    0x41f395 (_IO_default_xsputn), 49, movb %dl, (%rdi, %rax)
    0x42584d (ptmalloc_init.part.0), 33, movaps %xmm0, (%rax)
    0x454d48 (__tunables_init), 20, cmpb $0, (%r8)
    ...

    address, fetch misses, instruction
    0x4160a0 (__vfprintf_internal), 744, movl $1, %ebx
    0x41f0a0 (_IO_setb), 744, endbr64
    0x415882 (__vfprintf_internal), 744, movq %r12, %rdi
    0x4268a0 (__malloc), 696, andq $0xfffffffffffffff0, %rax
    ...


以下参数均为可选：

.. list-table:: 缓存建模参数
  :widths: 20 80
  :header-rows: 1

  * - 选项
    - 说明
  * - limit=N
    - 输出导致指令和数据缓存抖动最严重的前 N 条指令，包含地址、未命中次数和反汇编；默认 32
  * - icachesize=N
      iblksize=B
      iassoc=A
    - 分别设置指令缓存大小、块大小和相联度；默认 N = 16384、B = 64、A = 8
  * - dcachesize=N
    - 数据缓存大小，默认 16834
  * - dblksize=B
    - 数据缓存块大小，默认 64
  * - dassoc=A
    - 数据缓存相联度，默认 8
  * - evict=POLICY
    - 设置替换策略，可选 ``lru``、``fifo`` 和 ``rand``，同时用于指令和数据缓存；默认 ``lru``
  * - cores=N
    - 为 N 个核心分别维护指令和数据缓存；linux-user 默认 1，全系统模拟默认客户机核心数
  * - l2=on
    - 使用默认配置模拟统一 L2，保存指令和数据块；大小 2 MB、16 路相联、块大小 64 B
  * - l2cachesize=N
    - L2 大小，默认 2097152（2 MB），隐含 ``l2=on``
  * - l2blksize=B
    - L2 块大小，默认 64，隐含 ``l2=on``
  * - l2assoc=A
    - L2 相联度，默认 16，隐含 ``l2=on``

按触发条件停止
..............

``contrib/plugins/stoptrigger.c``

stoptrigger 可以设置停止模拟的触发条件，
适合研究时运行代码并精确停止，了解执行流经过了哪些位置。

支持两种触发条件：指定指令数，或指定地址。可以同时设置多个条件。

默认退出码为 0，也可以用 ``:CODE`` 为各条件指定退出码。

例如，在第 20 条指令处以 41 退出，在 0xd4 处以 0 退出，或在 0xd8 处以 42 退出：::

  $ qemu-system-aarch64 $(QEMU_ARGS) \
    -plugin ./contrib/plugins/libstoptrigger.so,icount=20:41,addr=0xd4,addr=0xd8:42 -d plugin


插件会记录退出原因，例如：::

  0xd4 reached, exiting


限制每秒指令数
..............

此插件限制每秒执行的指令数：::

    # get number of instructions
    $ num_insn=$(./build/qemu-x86_64 -plugin ./build/tests/plugin/libinsn.so -d plugin /bin/true |& grep total | sed -e 's/.*: //')
    # limit speed to execute in 10 seconds
    $ time ./build/qemu-x86_64 -plugin ./build/contrib/plugins/libips.so,ips=$(($num_insn/10)) /bin/true
    real 10.000s



.. list-table:: IPS 参数
  :widths: 20 80
  :header-rows: 1

  * - 选项
    - 说明
  * - ips=N
    - 每个 CPU 每秒最多执行的指令数，达到限制后插件会休眠
  * - ipq=N
    - 每个时间片的指令数，即每执行多少条指令重新计算时间；值越小越精确但效率越低，默认 ips/10

Uftrace
........

``contrib/plugins/uftrace.c``

此插件生成与 `uftrace <https://github.com/namhyung/uftrace>`_ 兼容的二进制跟踪。

支持 aarch64、x64 和 riscv64，可用于用户模式和系统模式，
因此能够跟踪通常不易观察的系统启动过程。

用户模式在执行结束时直接复制 ``/proc/self/maps`` 作为内存映射，
Uftrace 通常能自行获取符号，无需额外步骤。
系统模式默认映射为空，可以通过 ``contrib/plugins/uftrace_symbols.py``
生成映射及相关符号；ELF 二进制文件中必须包含符号。

它通过分析帧指针跟踪调用栈，因此程序及其依赖必须使用
``-fno-omit-frame-pointer -mno-omit-leaf-frame-pointer`` 编译。
2024 年，`Ubuntu 和 Fedora 在 64 位平台重新默认启用了帧指针
<https://www.brendangregg.com/blog/2024-03-17/the-return-of-the-frame-pointers.html>`_。
aarch64 上通常问题较小，因为除叶函数外，帧指针一般属于 ABI 要求。
这适用于用户空间应用，但不一定适用于裸机代码。
参见 `系统构建示例 <uftrace_build_system_example>`，了解如何构建带帧指针的系统。

跟踪超过一分钟的场景时，数据可能很长，难以提取所需部分。
一种简单方法是在跟踪的同时，用 ``qemu-system-aarch64 ... | ts "%s"``
生成带时间戳的输出日志，再用 ``uftrace --time-range=start~end`` 限定分析范围。

性能方面，相比普通 TCG 执行，开销约为 5–15 倍。

.. list-table:: Uftrace 插件参数
  :widths: 20 80
  :header-rows: 1

  * - 选项
    - 说明
  * - trace-privilege-level=[on|off]
    - 为各特权级分别生成跟踪：aarch64 为异常级别与安全状态，riscv64 为特权级，x64 为 Ring

.. list-table:: uftrace_symbols.py 参数
  :widths: 20 80
  :header-rows: 1

  * - 选项
    - 说明
  * - elf_file [elf_file ...]
    - ELF 文件路径，可用 /path/to/file:0xdeadbeef 指定映射偏移
  * - --prefix-symbols
    - 在符号前加上二进制文件名

用户态跟踪示例
++++++++++++++

例如，可以跟踪运行 git 的 QEMU 自身：::

    $ ./build/qemu-aarch64 -plugin \
      build/contrib/plugins/libuftrace.so \
      ./build/qemu-aarch64 /usr/bin/git --help

    # and generate a chrome trace directly
    $ uftrace dump --chrome | gzip > ~/qemu_aarch64_git_help.json.gz


为方便试用，可以下载
`qemu_aarch64_git_help.json.gz
<https://github.com/p-b-o/qemu-assets/raw/refs/heads/master/qemu-uftrace/qemu_aarch64_git_help.json.gz>`_，
在 https://ui.perfetto.dev/ 打开，通过 :kbd:`W`、:kbd:`A`、:kbd:`S`、:kbd:`D` 缩放和平移。
以下是其中部分执行序列：

- 加载程序及其解释器

.. image:: https://github.com/p-b-o/qemu-assets/blob/master/qemu-uftrace/loader_exec.png?raw=true
   :height: 200px

- open 系统调用

.. image:: https://github.com/p-b-o/qemu-assets/blob/master/qemu-uftrace/open_syscall.png?raw=true
   :height: 200px

- 创建翻译块

.. image:: https://github.com/p-b-o/qemu-assets/blob/master/qemu-uftrace/tb_translation.png?raw=true
   :height: 200px

通常直接使用 ``uftrace record`` 更好。
但如果不希望重新编译二进制文件，且其中仍有符号，
通过 qemu-user 跟踪就很方便，因为 ``uftrace record`` 需要插桩。

系统跟踪示例
++++++++++++

按下文步骤生成的完整系统启动跟踪（Chrome trace 格式）可以从
`这里 <https://github.com/p-b-o/qemu-assets/raw/refs/heads/master/qemu-uftrace/aarch64_boot.json.gz>`_
下载，并在 https://ui.perfetto.dev/ 打开。
可查看各特权级执行的代码，通过 :kbd:`W`、:kbd:`A`、:kbd:`S`、:kbd:`D` 缩放和平移。
以下是其中部分序列：

- Arm Trusted Firmware 启动序列的前两个阶段（EL3 和 S-EL1）

.. image:: https://github.com/p-b-o/qemu-assets/blob/master/qemu-uftrace/bl3_to_bl1.png?raw=true
   :height: 200px

- U-Boot 初始化，直到代码重定位；重定位后无法继续跟踪

.. image:: https://github.com/p-b-o/qemu-assets/blob/master/qemu-uftrace/uboot.png?raw=true
   :height: 200px

- 内核中的 stat 和 open 系统调用

.. image:: https://github.com/p-b-o/qemu-assets/blob/master/qemu-uftrace/stat.png?raw=true
   :height: 200px

- 定时器中断

.. image:: https://github.com/p-b-o/qemu-assets/blob/master/qemu-uftrace/timer_interrupt.png?raw=true
   :height: 200px

- 关机序列，从内核返回固件，即 NS-EL2 到 EL3

.. image:: https://github.com/p-b-o/qemu-assets/blob/master/qemu-uftrace/poweroff.png?raw=true
   :height: 200px

构建和运行系统示例
++++++++++++++++++

.. _uftrace_build_system_example:

构建一个完整、带帧指针的系统镜像并不简单。

我们提供一种 `简便方法 <https://github.com/p-b-o/qemu-linux-stack>`_，
构建组合 Arm Trusted Firmware、U-Boot、Linux 内核和 Debian 用户空间的 aarch64 系统。
它基于容器，仅使用 ``podman``，并通过 ``qemu-user-static (binfmt)``
确保容易复现，不依赖执行构建的机器。

切换到 `x86_64 <https://github.com/p-b-o/qemu-linux-stack/tree/x86_64>`_ 分支，
可按相同步骤构建由 edk2、Linux 和 Ubuntu 组成的 x64 系统。

切换到 `riscv64 <https://github.com/p-b-o/qemu-linux-stack/tree/riscv64>`_ 分支，
可按相同步骤构建由 OpenSBI、Linux 和 Ubuntu 组成的 riscv64 系统。

构建并运行：::

    # Install dependencies
    $ sudo apt install -y podman qemu-user-static

    $ git clone https://github.com/p-b-o/qemu-linux-stack
    $ cd qemu-linux-stack
    $ ./build.sh

    # system can be started using:
    $ ./run.sh /path/to/qemu-system-aarch64

    # generate a uftrace for execution (collect symbols automatically)
    $ ./trace.sh /path/to/qemu-system-aarch64
    # show output log to read timestamps
    $ cat uftrace.data/exec.log
    # generate final trace (compressed) for perfetto
    $ ./perfetto.sh <start_timestamp> <end_timestamp> ~/trace.gz


手动生成相同跟踪：::

    # run true and poweroff the system
    $ env INIT=true ./run.sh path/to/qemu-system-aarch64 \
      -plugin path/to/contrib/plugins/libuftrace.so,trace-privilege-level=on

    # generate symbols and memory mapping
    $ path/to/contrib/plugins/uftrace_symbols.py \
      --prefix-symbols \
      arm-trusted-firmware/build/qemu/debug/bl1/bl1.elf \
      arm-trusted-firmware/build/qemu/debug/bl2/bl2.elf \
      arm-trusted-firmware/build/qemu/debug/bl31/bl31.elf \
      u-boot/u-boot:0x60000000 \
      linux/vmlinux

    # inspect trace with
    $ uftrace replay


Uftrace 支持过滤跟踪、输出火焰图或 Chrome trace。
后者尤其适合直观观察启动过程：::

    $ uftrace dump --chrome > boot.json
    # Open your browser, and load boot.json on https://ui.perfetto.dev/.


过长的 Chrome trace 不易打开，可以只生成某段执行时间附近的数据：::

    # execute qemu and timestamp output log
    $ env INIT=true ./run.sh path/to/qemu-system-aarch64 \
      -plugin path/to/contrib/plugins/libuftrace.so,trace-privilege-level=on |&
      ts "%s" | tee exec.log

    $ cat exec.log  | grep 'Run /init'
      1753122320 [   11.834391] Run /init as init process
      # init was launched at 1753122320

    # generate trace around init execution (2 seconds):
    $ uftrace dump --chrome --time-range=1753122320~1753122322 > init.json


陷阱计数
........

``contrib/plugins/traps.c``

此插件按 CPU 统计中断（异步事件）、异常（同步事件）及宿主调用（例如半主机）的数量。

动态链接调用
............

``contrib/plugins/dlcall.c``

此插件为 linux-user 客户机提供动态链接函数调用拦截机制：
客户机将调用交给宿主，由插件执行原生代码，替代客户机逐指令模拟。
仅拦截本身即可用于调用跟踪或审计。
另一种用途是借助宿主原生共享库加速，例如通过跳板层，
在模拟环境中运行未修改的 zlib ``minizip``，
但把 ``deflate`` 调用转交宿主原生 libz，从而省去这些库调用的逐指令模拟。

客户机发出一个保留的特殊系统调用，默认编号 4096，可通过 ``syscall_num=N`` 修改。
第一个参数选择透传操作：通过 dlopen/dlclose 打开或关闭宿主库，
通过 dlsym 解析符号，或调用已解析的宿主函数。
插件在宿主执行操作并拦截该系统调用，真实内核不会收到它。

.. warning::

   仅限可信客户机。客户机可以加载任意宿主库，并在 QEMU 宿主进程中执行任意代码。
   此插件不是沙箱，不提供隔离。
   还要求 ``guest_base == 0``，即 qemu-user 默认值，
   且客户机的指针宽度和字节序与宿主相同，
   因为客户机指针会直接作为宿主地址解引用，不进行转换。

插件有意保持 QEMU 侧简单，不了解任何特定库及其调用约定。
真实库的跳板完全由用户空间生成，任何工具链都可以实现该接口。

QEMU 侧只需加载插件：

.. code-block:: shell

   qemu-x86_64 -plugin contrib/plugins/libdlcall.so <guest-program> ...


如果默认调用编号不适合客户机 ABI，可以选择其他编号，
并让用户空间端发出相同编号：

.. code-block:: shell

   qemu-x86_64 -plugin contrib/plugins/libdlcall.so,syscall_num=8192 \
       <guest-program> ...


`Lorelei <https://github.com/rover2024/lorelei>`_ 提供了完整的用户空间实现，
包括客户机及宿主运行时，以及根据库头文件生成跳板的自动化工具链，
使客户机库调用转而在宿主原生库中执行。
它对参数封送、回调和可变参数函数等插件未处理部分的实现，可以作为参考。
支持在 x86_64、aarch64 或 riscv64 宿主上运行 x86_64 客户机。

下面的最小完整示例使用只有一个函数的 ``libhello.so``，分别构建客户机和宿主版本。
客户机版本输出 ``(from the guest)``，宿主版本输出 ``(from the host)``。
未修改的客户机程序 ``main`` 调用 ``hello("World", 7)``，
跳板使同一二进制转而调用宿主版本。源码位于 ``src/``：

.. code-block:: c

   /* src/hello.h */
   void hello(const char *name, int lucky);


.. code-block:: c

   /* src/hello_guest.c */
   #include "hello.h"
   #include <stdio.h>

   void hello(const char *name, int lucky)
   {
       printf("Hello, %s! Your lucky number is %d. (from the guest)\n", name, lucky);
   }


.. code-block:: c

   /* src/hello_host.c */
   #include "hello.h"
   #include <stdio.h>

   void hello(const char *name, int lucky)
   {
       printf("Hello, %s! Your lucky number is %d. (from the host)\n", name, lucky);
   }


.. code-block:: c

   /* src/main.c */
   #include "hello.h"

   int main(void)
   {
       hello("World", 7);
       return 0;
   }


Lorelei 的发行版包含预构建工具链 devkit。下载适合宿主的版本并解压：

.. code-block:: shell

   # ARCH is your host's architecture: x86_64, aarch64 or riscv64. This example uses aarch64.
   # See https://github.com/rover2024/lorelei/releases
   ARCH=aarch64
   VERSION=$(curl -fsSL -o /dev/null -w '%{url_effective}' \
         https://github.com/rover2024/lorelei/releases/latest | sed 's|.*/tag/v||')
   wget "https://github.com/rover2024/lorelei/releases/download/v$VERSION/lorelei-devkit-$ARCH-$VERSION.tar.xz"
   tar -xf lorelei-devkit-$ARCH-$VERSION.tar.xz
   DEVKIT=lorelei-devkit-$ARCH


构建客户机 x86_64 的 ``libhello.so``、宿主架构的 ``libhello.so``，以及客户机程序：

.. code-block:: shell

   mkdir -p build/guest build/host
   $DEVKIT/bin/x86_64-linux-gnu-clang -shared -fPIC src/hello_guest.c -o build/guest/libhello.so
   cc -shared -fPIC src/hello_host.c -o build/host/libhello.so
   $DEVKIT/bin/x86_64-linux-gnu-clang src/main.c -Isrc -Lbuild/guest -lhello -o build/guest/main


在 QEMU 下运行：

.. code-block:: shell

   qemu-x86_64 -L /usr/x86_64-linux-gnu/ -E LD_LIBRARY_PATH=build/guest build/guest/main


输出为：::

   Hello, World! Your lucky number is 7. (from the guest)


根据宿主 ``libhello.so`` 生成跳板。
这会生成替代客户机原库的客户机侧 ``libhello.so``，
以及负责转发到宿主库的宿主侧跳板库：

.. code-block:: shell

   $DEVKIT/bin/LoreMakeThunk.py --name hello --lib build/host/libhello.so \
       --header hello.h -o thunks -- -Isrc


在插件下运行同一个 ``main``，现在调用会进入宿主版本：

.. code-block:: shell

   LD_LIBRARY_PATH=$DEVKIT/lib:build/host \
       qemu-x86_64 -plugin contrib/plugins/libdlcall.so \
       -E LD_LIBRARY_PATH=$DEVKIT/x86_64/lib:thunks/x86_64 \
       -L /usr/x86_64-linux-gnu/ \
       build/guest/main


输出为：::

   Hello, World! Your lucky number is 7. (from the host)


.. list-table:: 动态链接调用参数
  :widths: 20 80
  :header-rows: 1

  * - 选项
    - 说明
  * - syscall_num=N
    - 客户机使用的特殊系统调用编号，默认 4096。该编号不能是客户机 ABI 的真实系统调用，
      也不能在插件接收到它之前被拒绝，因此选择范围同时受两方面约束。

其他模拟功能
------------

系统模拟还可以启用确定性执行，以便进行可重复的记录与重放调试。
详见 :ref:`记录与重放 <replay>`。
