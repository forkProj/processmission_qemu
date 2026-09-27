========
模糊测试
========

本文介绍 QEMU 虚拟设备的模糊测试基础设施，以及如何添加新的模糊测试器。

基础
--------

模糊测试将输入传给入口或目标函数，跟踪输入触发的代码覆盖情况，
据此变异输入并反复测试。

QEMU 使用 libFuzzer。与 AFL 等工具不同，它在同一进程内运行，
因此开发者必须确保每轮之间重置状态。

构建模糊测试器
--------------

安装较新的 Clang，并将以下命令中的程序名替换为实际版本。
ASan 和 UBSan 可选，但有助于可靠检测越界、释放后使用、重复释放等问题：::

    CC=clang-8 CXX=clang++-8 /path/to/configure \
        --enable-fuzzing --enable-asan --enable-ubsan


模糊测试目标的构建方式与系统目标类似：::

    make qemu-fuzz-i386


将生成 ``./qemu-fuzz-i386``。

首个选项为 ``--fuzz-target=FUZZ_NAME``。
不带参数运行 qemu-fuzz-i386，可列出全部测试器。

例如：::

    ./qemu-fuzz-i386 --fuzz-target=virtio-scsi-fuzz


libFuzzer 解析所有不以双连字符开头的参数，传入 ``-help=1`` 可查看说明。

之后等待测试器发现潜在崩溃即可。

实用 libFuzzer 选项
-------------------

通过 -help=1 可查看全部参数，以下尤其有用：

* ``CORPUS_DIR/``：将目录作为最后一个参数。
  libFuzzer 在其中保存有价值的输入，下次从已有语料继续测试。
  可指定多个目录，读取全部目录的输入，但只向第一个写入新输入。

* ``-max_len=4096``：生成输入的最大字节长度。

* ``-close_fd_mask={1,2,3}``：关闭标准输出、标准错误或两者，
  适合产生大量调试、错误或串口输出的目标。

* ``-jobs=4 -workers=4``：在 4 个工作进程中并行运行 4 个任务。
  只指定 -jobs=N 时，工具自动启动不超过可用 CPU 核数一半的工作进程。
  按机器能力调整，并指定 CORPUS_DIR，使并行测试器能共享有价值输入。

* ``-use_value_profile=1``：为每次比较计算
  ``(caller_pc&4095) | (popcnt(Arg1 ^ Arg2) << 12)``，写入覆盖表。
  适合包含魔数常量的目标。若 Arg1 来自输入、Arg2 是魔数，
  每当二者汉明距离减小，就把输入加入语料。

* ``-shrink=1``：尝试缩小语料元素，根据目标不同，可能改善覆盖效率。

确切行为取决于构建时所用 Clang 和 libFuzzer 的版本。

生成覆盖率报告
--------------

代码覆盖率是评估测试器效果的重要指标。
libFuzzer 的 cov 列显示已覆盖的唯一块或边数。
逐行检查可以使用 Clang 覆盖工具：

 1. 配置语料目录，保存全部有价值输入，见前面的 CORPUS_DIR。
 2. 使用以下选项配置 QEMU：::

    --enable-fuzzing \
    --extra-cflags="-fprofile-instr-generate -fcoverage-mapping"


 3. 重新运行，传入 ``$CORPUS_DIR/*``，执行全部语料后退出。
    工作目录中应生成 default.profraw。
 4. 运行以下命令生成详细 HTML 报告：::

      llvm-profdata merge -output=default.profdata default.profraw
      llvm-cov show ./path/to/qemu-fuzz-i386 -instr-profile=default.profdata \
      --format html -output-dir=/path/to/output/report


添加测试器
----------

增加测试器可以扩展虚拟设备覆盖范围。
源码放在 ``tests/qtest/fuzz/``，并加入该目录的 meson.build。

可使用 qtest 和 libqos 与虚拟设备通信。

1. 创建源文件，例如 ``tests/qtest/fuzz/foo-device-fuzz.c``。

2. 使用 libqtest/libqos API 编写测试，参考已有实现。

3. 加入 ``tests/qtest/fuzz/meson.build``。

测试器可以视为特殊的 qtest 程序：根据 libFuzzer 输入改变 qtest 命令或参数。
输入是一段字节数组及长度，常见做法是循环读取，将其解释为命令、地址或数值列表。

通用测试器
----------

编写专用目标工作量较大，尤其当 libqos 尚无对应驱动时。
许多设备可以用 generic-fuzz，在没有设备专用代码的情况下获得一定程度的测试。

它覆盖 PIO、MMIO 和 DMA 输入空间，至少需要设置两个环境变量：

* ``QEMU_FUZZ_ARGS=``：配置机器及设备的 QEMU 参数。
  例如，在 pc-i440fx 上测试 virtio-net：::

    QEMU_FUZZ_ARGS="-M pc -nodefaults -netdev user,id=user0 \
    -device virtio-net,netdev=user0"


* ``QEMU_FUZZ_OBJECTS=``：空格分隔的匹配字符串，支持 glob。
  与 MemoryRegion 名称及其所有者名称比较，决定哪些区域参与测试。
  对 virtio-net，可使用以下任一种：::

    QEMU_FUZZ_OBJECTS='virtio-net'
    QEMU_FUZZ_OBJECTS='virtio*'
    QEMU_FUZZ_OBJECTS='virtio* pcspk' # Fuzz the virtio devices and the speaker
    QEMU_FUZZ_OBJECTS='*' # Fuzz the whole machine``


监视器的 info mtree 和 info qom-tree 尤其适合查找 MemoryRegion 和 Object 名称。

通常匹配的区域或设备越多，输入空间越大，发现单个设备崩溃输入的概率越小，
因此建议限制为少量区域。

检查变量是否正确配置：::

    ./qemu-fuzz-i386 --fuzz-target=generic-fuzz -runs=0


输出应列出全部匹配区域。

OSS-Fuzz
--------

QEMU 在 `OSS-Fuzz <https://github.com/google/oss-fuzz>`_ 上持续进行模糊测试。
默认会尝试全部目标。由于 generic-fuzz 需要额外环境信息，
我们在 ``tests/qtest/fuzz/generic_fuzz_configs.h`` 预定义配置，每项必须包含：

- ``.name``：配置名称。

- ``.args`` 或 ``.argfunc``：字符串，或返回字符串的函数指针，
  用于设置 QEMU_FUZZ_ARGS。需要动态临时目录或空闲 TCP/UDP 端口时，argfunc 很有用。

- ``.objects``：设置 QEMU_FUZZ_OBJECTS 的字符串。

若要在 OSS-Fuzz 增加设备或配置，提交新的专用测试器或 generic-fuzz 配置即可。

构建细节：

- 环境配置位于 OSS-Fuzz 仓库的
  `Dockerfile <https://github.com/google/oss-fuzz/blob/master/projects/qemu/Dockerfile>`_。

- 构建脚本位于 QEMU 的 ``scripts/oss-fuzz/build.sh``。

构建崩溃复现程序
----------------

发现崩溃后，应尝试生成可在普通 QEMU 构建上运行的独立复现程序，
以排除误报，并方便开发者调试。
以下针对 generic-fuzz 发现的崩溃：

- 确认可以复现：::

    qemu-fuzz-i386 --fuzz-target... ./crash-...


- 收集对应 QTest 输出：::

    QEMU_FUZZ_TIMEOUT=0 QTEST_LOG=1 FUZZ_SERIALIZE_QTEST=1 \
    qemu-fuzz-i386 --fuzz-target... ./crash-... &> /tmp/trace


- 重排并清理跟踪：::

    scripts/oss-fuzz/reorder_fuzzer_qtest_trace.py /tmp/trace > /tmp/reproducer


- 获取启动参数，并提供 QEMU 路径：::

    less /tmp/trace # The args should be logged at the top of this file
    export QEMU_ARGS="-machine ..."
    export QEMU_PATH="path/to/qemu-system"


- 确认在 qemu-system 中仍可复现：::

    $QEMU_PATH $QEMU_ARGS -qtest stdio < /tmp/reproducer


- 从输出中选取能标识崩溃的字符串，例如某行堆栈跟踪：::

    export CRASH_TOKEN="hw/usb/hcd-xhci.c:1865"


- 最小化复现输入：::

    scripts/oss-fuzz/minimize_qtest_trace.py -M1 -M2 \
      /tmp/reproducer /tmp/reproducer-minimized


- 确认最小化后仍会崩溃：::

    $QEMU_PATH $QEMU_ARGS -qtest stdio < /tmp/reproducer-minimized


- 生成便于邮件发送的单行复现命令：::

    ./scripts/oss-fuzz/output_reproducer.py -bash /tmp/reproducer-minimized


- 输出复现该问题的 C 测试源码：::

    ./scripts/oss-fuzz/output_reproducer.py -owner "John Smith <john@smith.com>"\
      -name "test_function_name" /tmp/reproducer-minimized


- 报告缺陷，并向上游提交包含 C 复现用例的补丁。

QEMU 也可以直接从文件读取复现输入，而非标准输入：::

    $QEMU_PATH $QEMU_ARGS -qtest chardev:repro \
      -chardev file,id=repro,path=/dev/null,input-path=/tmp/reproducer


在调试器中运行 QEMU 排查失败时，这很有用。

实现细节与生命周期
------------------

libFuzzer 提供自己的 main()，完成初始化后调用两个入口：

``LLVMFuzzerInitialize``：测试开始前调用，初始化全部必要状态。

``LLVMFuzzerTestOneInput``：每轮调用，处理输入并在结束时重置状态。

具体流程如下。

LLVMFuzzerInitialize 解析以双连字符开头的测试器参数，
以免被 libFuzzer 的 main() 处理；当前参数用于选择测试目标。
随后初始化 qtest 客户端。如果需要 qos，则配置 qgraph 并初始化 QOM/LIBQOS 模块，
再遍历图，确定并保存 QEMU 命令行。

之后调用 vl.c:main 配置客户机。
目标专用钩子可在 main 前后执行 PCI 配置、虚拟机快照等额外准备。

LLVMFuzzerTestOneInput 根据输入调用 qtest/qos，
还必须手动调用 main_loop_wait，确保执行 bottom half，并完成下一轮前所需清理。

同一进程会反复使用，因此每轮结束都需要重置 QEMU 状态，例如重启虚拟机。

  - *优点*：对简单目标直接且快速。

  - *缺点*：某些设备状态无法完全重置。
    如果设备需要先初始化才能测试（QOS 目标常见），每次重启后还需重复初始化。

  - *示例目标*：``i440fx-qtest-reboot-fuzz``。
