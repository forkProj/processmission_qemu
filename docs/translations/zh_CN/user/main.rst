.. _user-mode:

QEMU 用户空间模拟器
===================

支持的操作系统
--------------

用户空间模拟支持以下操作系统：

-  Linux，称为 qemu-linux-user。

-  BSD，称为 qemu-bsd-user。

功能
--------

QEMU 用户空间模拟具有以下主要功能。

系统调用转换
~~~~~~~~~~~~

系统调用是用户空间与内核之间的主要接口。
通常，各内核版本提供相同的系统调用，因此 QEMU 包含通用的系统调用转换器。
它负责调整字节序、32/64 位参数大小，再调用等效的宿主系统调用。

QEMU 也可以用类似方式调整设备专用的 ``ioctl()`` 调用。

POSIX 信号处理
~~~~~~~~~~~~~~

QEMU 可以将宿主发出的所有信号（例如 ``SIGALRM``）转发给正在运行的程序，
也能根据虚拟 CPU 异常生成信号，例如程序执行除零操作时生成 ``SIGFPE``。

QEMU 依赖宿主内核模拟大多数信号相关系统调用，例如信号掩码。
在 Linux 上，QEMU 同时支持普通信号和实时信号。

线程
~~~~~~~~

在 Linux 上，QEMU 可以模拟 ``clone`` 系统调用，
为每个被模拟线程创建真实的宿主线程，并为其分配独立的虚拟 CPU。
但 QEMU 依赖系统 libc 代为调用 ``clone``，因此只接受 libc 所用的标志。
具体而言，不支持影响命名空间的标志，例如容器运行时使用的标志。
不过，QEMU 用户态进程本身仍可以在容器内运行。

QEMU 尽力正确模拟原子操作，但宿主与客户机内存模型的差异，
仍可能影响对内存模型作出假设的软件。

QEMU 在设计上允许最终模拟自身。虽然实际用途不大，
但这是展示模拟器能力的一项重要测试。

.. _linux-user-mode:

Linux 用户空间模拟器
--------------------

命令行选项
~~~~~~~~~~

::

   qemu-i386 [-h] [-d] [-L path] [-s size] [-cpu model] [-g endpoint] [-B offset] [-R size] program [arguments...]


``-h``
   打印帮助。

``-L path``
   设置 x86 ELF 解释器路径前缀，默认为 /usr/local/qemu-i386。

``-s size``
   设置 x86 栈大小，单位为字节，默认为 524288。

``-cpu model``
   选择 CPU 型号；使用 -cpu help 查看列表及额外特性的选择方式。

``-E var=value``
   将环境变量 var 设为 value。

``-U var``
   从环境中删除 var。

``-B offset``
   将客户机地址偏移指定字节数。当客户机应用需要的地址区域已被宿主保留时，
   此选项很有用。目前仅部分宿主支持。

``-R size``
   预分配指定大小的客户机虚拟地址空间，单位为字节。
   指定大小时可以使用 "G"、"M" 和 "k" 后缀。

调试选项：

``-d item1,...``
   启用指定项目的日志，使用 '-d help' 查看日志项目列表。

``-g endpoint``
   在端口（例如 ``1234``）或 Unix socket（例如 ``/tmp/qemu.sock``）上等待 GDB 连接。

   如果 Unix socket 路径包含一个 ``%d`` 占位符，例如 ``/tmp/qemu-%d.sock``，
   它会替换为模拟器的 PID。
   通过 ``QEMU_GDB`` 环境变量向多进程应用传递此选项时，这很有用。

   如果端点地址后加上 ``,suspend=n``，例如 ``1234,suspend=n``，
   被模拟程序会立即启动，无需等待连接；之后仍可随时连接。

``-one-insn-per-tb``
   每个翻译块只执行一条客户机指令。
   这会显著降低模拟速度，但在分析 ``-d`` 选项生成的日志等场景下可能有用。

环境变量：

QEMU_STRACE
   类似 strace，打印系统调用及其参数。
   真正的 strace 无法工作，因为用户空间模拟器尚未实现 ptrace。
   此功能目前仍不完整。没有专用参数格式的系统调用一律打印六个参数的信息；
   许多标志类参数没有解码器，只显示数字。

其他二进制程序
~~~~~~~~~~~~~~

-  用户态（Alpha）

   * ``qemu-alpha``：待补充。

-  用户态（Arm）

   * ``qemu-armeb``：待补充。

   * ``qemu-arm`` 还支持 Arm "Angel" 半主机 ELF 二进制程序，
     对应 arm-elf 和 arm-eabi 的 Newlib/GDB 配置实现，
     以及 arm-uclinux 的 bFLT 格式程序。

-  用户态（ColdFire）

-  用户态（M68K）

   * ``qemu-m68k`` 可以运行使用 BDM（m5xxx-ram-hosted.ld）或
     m68k-sim（sim.ld）系统调用接口的半主机程序，以及 coldfire uClinux bFLT 程序。

   二进制格式会自动检测。

-  用户态（i386）

   * ``qemu-i386``：待补充。
   * ``qemu-x86_64``：待补充。

-  用户态（Microblaze）

   * ``qemu-microblaze``：待补充。

-  用户态（MIPS）

   * ``qemu-mips`` 执行 32 位大端 MIPS 程序（MIPS O32 ABI）。

   * ``qemu-mipsel`` 执行 32 位小端 MIPS 程序（MIPS O32 ABI）。

   * ``qemu-mips64`` 执行 64 位大端 MIPS 程序（MIPS N64 ABI）。

   * ``qemu-mips64el`` 执行 64 位小端 MIPS 程序（MIPS N64 ABI）。

   * ``qemu-mipsn32`` 执行 32 位大端 MIPS 程序（MIPS N32 ABI）。

   * ``qemu-mipsn32el`` 执行 32 位小端 MIPS 程序（MIPS N32 ABI）。

-  用户态（PowerPC）

   * ``qemu-ppc64``：待补充。
   * ``qemu-ppc``：待补充。

-  用户态（SH4）

   * ``qemu-sh4eb``：待补充。
   * ``qemu-sh4``：待补充。

-  用户态（SPARC）

   * ``qemu-sparc`` 可以执行 Sparc32 程序（Sparc32 CPU，32 位 ABI）。

   * ``qemu-sparc32plus`` 可以执行 Sparc32 和 SPARC32PLUS 程序
     （Sparc64 CPU，32 位 ABI）。

   * ``qemu-sparc64`` 可以执行部分 Sparc64 程序（Sparc64 CPU，64 位 ABI），
     以及 SPARC32PLUS 程序（Sparc64 CPU，32 位 ABI）。

.. _bsd-user-mode:

BSD 用户空间模拟器
------------------

BSD 支持状态
~~~~~~~~~~~~

-  在 Sparc64 宿主上模拟 Sparc64 目标：部分简单程序可以运行。

快速入门
~~~~~~~~

要启动 BSD 进程，QEMU 需要进程的可执行文件，以及它使用的全部目标动态库。

-  在 Sparc64 上，可以直接使用本机库尝试启动进程：::

      qemu-sparc64 /bin/ls


命令行选项
~~~~~~~~~~

::

   qemu-sparc64 [-h] [-d] [-L path] [-s size] [-bsd type] program [arguments...]


``-h``
   打印帮助。

``-L path``
   设置库的根路径，默认为 /。

``-s size``
   设置栈大小，单位为字节，默认为 524288。

``-ignore-environment``
   以空环境启动。如果不使用此选项，初始环境是调用者环境的副本。

``-E var=value``
   将环境变量 var 设为 value。

``-U var``
   从环境中删除 var。

``-bsd type``
   设置被模拟的 BSD 操作系统类型。
   有效值为 FreeBSD、NetBSD 和 OpenBSD，默认 OpenBSD。

调试选项：

``-d item1,...``
   启用指定项目的日志，使用 '-d help' 查看日志项目列表。

``-one-insn-per-tb``
   每个翻译块只执行一条客户机指令。
   这会显著降低模拟速度，但在分析 ``-d`` 日志等场景下可能有用。
