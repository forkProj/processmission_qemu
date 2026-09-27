=================
QEMU 构建系统架构
=================

本文帮助开发者理解 QEMU 构建系统架构。
与 GNU Autotools 项目类似，构建分为两阶段：先运行 configure 确定本地环境，
再运行 make 构建。但相似之处基本仅限于此，不应直接套用 Autotools 经验。

通常有两种构建布局：

 - 产物完全放在源码树之外::

     cd ../
     mkdir build
     cd build
     ../qemu/configure
     make


 - 产物放在源码树的子目录::

     mkdir build
     cd build
     ../configure
     make


实际构建主要由 Meson 完成，因此产物不能直接放入源码目录本身。


阶段 1：configure
=================

configure 有五项任务：

 - 检测主机架构。

 - 列出要构建的模拟器目标；该列表也影响固件和测试的构建范围。

 - 查找构建可执行文件、固件和测试所需的本机及交叉编译器，
   将结果写为 Makefile 片段 config-host.mak，或 Meson 机器文件 config-meson.cross。

 - 创建供构建期间全部 Python 代码使用的虚拟环境，必要时从 PyPI 安装软件包。

 - 在虚拟环境运行 Meson，执行模拟器构建的实际配置。

configure 自动识别存在同名 Meson 选项的命令行参数，
将命令行中的连字符转换为下划线。

几乎所有需要修改构建系统的开发者只需关注 Meson，可以跳过本节余下内容。


修改 ``configure``
------------------

configure 是使用 #!/bin/sh 的 shell 脚本，必须兼容任意 POSIX shell。
应避免 Bash 专用语法，以免破坏默认 shell 不是 Bash 的平台。

脚本提供以下函数，帮助编写可移植代码，并在不同架构和系统间保持一致行为：

``error_exit $MESSAGE $MORE...``
   向标准错误输出 MESSAGE 和后续 MORE 内容，再以非零状态退出。

``has $COMMAND``
   检查当前环境是否存在该命令，包括 shell 内置或可执行程序，成功返回 0。
   Meson 中对应 find_program()。

``probe_target_compiler $TARGET``
  检测指定 QEMU 目标的交叉编译器和工具，例如 $CPU-softmmu、
  $CPU-linux-user、$CPU-bsd-user。
  找到可用编译器时返回成功，并将 target_cc、target_ar 等设为非空。

``write_target_makefile``
  向标准输出写入 Makefile 片段，将最近一次 probe_target_compiler 的结果
  暴露为 CC、AR、LD 等常规变量。


除检测主机平台和确认编译器可用等基本检查外，configure 通常不测试编译器选项。
基本检查使用以下辅助函数：

``compile_object $CFLAGS``
   使用系统 C 编译器和指定 CFLAGS 编译测试程序。
   程序必须已写入 $TMPC 文件。

``compile_prog $CFLAGS $LDFLAGS``
   用系统编译器和 CFLAGS 编译，再用系统链接器和 LDFLAGS 链接测试程序。
   程序必须已写入 $TMPC。

``check_define $NAME``
   检查系统 C 编译器是否定义指定宏。

``do_compiler $CC $ARGS...``
   使用指定参数运行 C 编译器，不采用 --extra-cflags 等选项传入的标志，
   因而可用于交叉编译器检查。
   不过多数此类检查在 make 阶段执行，
   例如 pc-bios/option-rom/Makefile 的 cc-option 宏。

``write_c_skeleton``
   向 $TMPC 指定的临时文件写入最小 C main() 函数。


Python 虚拟环境与构建过程
-------------------------

configure 的重要步骤是在配置阶段创建 Python 虚拟环境。
解释器按优先级来自 --python、环境变量 $PYTHON，最后是系统 PATH。
虚拟环境位于构建树的 pyvenv，确保构建期间运行 Python 的方式一致。

此时还会向选定解释器查询构建依赖。
构建过程不会在 PATH 搜索 meson 或 sphinx-build 二进制文件，
也没有 --meson、--sphinx-build 之类选项。
这避免 PATH 上的程序使用不同 Python 环境。
但也可能让用户困惑：依赖已经安装，configure 却找不到。
此时通常是装到了其他解释器的 site-packages，或使用了错误的 pip。

若选定解释器可用某个包，configure 会生成小脚本，从虚拟环境调用它 [#distlib]_。
否则也可选择通过 pip 安装依赖，来源可以是 python/wheels 中的 wheel，或从 PyPI 下载。
--disable-download 可禁用下载；无论如何，只有显式启用了某个选项
（目前仅 --enable-docs）但缺少依赖时才会下载。

.. [#distlib] 脚本依据包元数据中的 console_script 入口点生成，与 pip 安装机制相同。
              当前所有场景其实都可用 python -m，生成脚本略显繁复；
              但它更能适应未来变化，也使 pyvenv/bin 内容更直观。
              可移植性也不是问题，Python Packaging Authority 提供 distlib.scripts 完成此任务。

所需包版本保存在 pythondeps.toml。
它使用 QEMU 自定义格式，文件顶部有说明，应当易于理解。
版本要求应允许使用 QEMU 支持发行版中打包的版本。

下载依赖时，则采用同一文件列出的“已知正常”版本。
此时 pythondeps.toml 类似 Cargo、Poetry 等依赖管理系统的锁文件。


随源码提供的 Python 包
----------------------

构建必需、但并非所有受支持发行版都提供的 Python 包，会随 QEMU 源码分发。
目前只有 Meson，因为 Ubuntu 22.04 和 openSUSE Leap 中的版本过旧。

添加或更新 wheel 时，修改并重跑 python/scripts/vendor.py。
脚本内嵌并验证源码包的 SHA-256 哈希，pypi.org 可方便地查询该值。


阶段 2：Meson
=============

Meson 描述以下内容的构建和安装：

1) 可执行文件，包括：

   - 工具：qemu-img、qemu-nbd、客户机代理 qemu-ga 等；
   - 系统模拟器：qemu-system-$ARCH；
   - 用户空间模拟器：qemu-$ARCH；
   - 单元测试。

2) 文档。

3) ROM，包括随发行包提供的二进制文件，以及按 configure 指示交叉编译的固件。

4) 图标、桌面文件等其他数据。

除 contrib/ 中已知在部分平台（例如 32 位或大端）无法构建的程序外，
默认构建全部可执行文件。测试目前也默认构建，但未来可能变化。

源码高度模块化，以尽量少的重复编译构建这些组件。
meson.build 使用 sourceset，将源码组织为由系统库和配置符号控制的规则。
源码集分为四组：

子系统源码集：
  工具和模拟器共用的子系统通常有独立源码集，
  例如块设备的 block_ss、字符设备的 chardev_ss，随后转换为静态库::

    libchardev = static_library('chardev', chardev_ss.sources(),
                                build_by_default: false)

    chardev = declare_dependency(objects: libchardev.extract_all_objects(recursive: false),
                                 dependencies: chardev_ss.dependencies())


与目标无关的模拟器源码集：
  通用辅助代码只编译一次，生成对象文件供所有需要的程序链接，
  包括错误处理、标准数据结构、平台可移植性封装等。

  common_ss 链接到全部模拟器，system_ss 仅链接系统模拟器，
  user_ss 仅链接用户模式模拟器。

与目标相关的模拟器源码集：
  包含 CPU 模拟、部分设备模拟和大量衔接代码，
  有时需要为每个目标分别编译。相关文件加入 specific_ss。

  每个模拟器还包含 hw/ 和 target/ 子目录中的源码。
  具体子目录由 ``configs/targets/*.mak`` 的 TARGET_BASE_ARCH，
  或缺省时的 TARGET_ARCH 决定。

  hw/ 各子目录向 hw_arch 字典添加一个源码集，例如::

    arm_ss = ss.source_set()
    arm_ss.add(files('boot.c'), fdt)
    ...
    hw_arch += {'arm': arm_ss}


  只用于系统模拟器。

  target/ 各子目录则分别向 target_arch 和 target_system_arch 添加源码集，
  前者供全部模拟器使用，后者仅供系统模拟器。例如::

    arm_ss = ss.source_set()
    arm_system_ss = ss.source_set()
    ...
    target_arch += {'arm': arm_ss}
    target_system_arch += {'arm': arm_system_ss}


模块源码集：
  modules 字典保存目标无关模块，target_modules 保存目标相关模块。
  禁用模块时，前者源码集加入 system_ss，后者加入 specific_ss。

  两个字典都采用嵌套结构：每个子目录建立自己的字典，再加入顶层字典。例如::

    hw_display_modules = {}
    qxl_ss = ss.source_set()
    ...
    hw_display_modules += { 'qxl': qxl_ss }
    modules += { 'hw-display': hw_display_modules }


辅助源码集：
  全部程序链接 libqemuutil.a，由多个源码集构建。
  多数保存生成代码，一般需要关注的是 util_ss 和 stub_ss。

  两者分开主要为了说明用途。util_ss 包含通用工具文件。
  某些文件仅在部分程序中需要钩子，也可能依赖并非所有程序完整实现的函数。
  stub_ss 提供空桩，只有缺少真实实现时才链接。
  可将这些桩理解为弱符号概念的可移植实现。


以下文件共同决定各模拟器链接哪些源码：

``configs/devices/*.mak``
  控制各系统模拟目标包含的开发板和设备，只包含配置变量列表，例如::

    include arm-softmmu.mak
    CONFIG_XLNX_ZYNQMP_ARM=y
    CONFIG_XLNX_VERSAL=y


``*/Kconfig``
  与设备配置一起处理，描述功能、子系统和设备模型之间的依赖，详见 :ref:`kconfig`。

``configs/targets/*.mak``
  主要定义各模拟器 ``*-config-target.h`` 中的符号 [#cfgtarget]_。
  TARGET_ARCH 和 TARGET_BASE_ARCH 还用于选择编入各目标的 hw/ 和 target/ 子目录。

.. [#cfgtarget] 编译目标专用源码集时，由 qemu/osdep.h 包含此头文件。

除非添加全新目标，或为特定系统／用户空间目标启用新设备和硬件，通常无需修改这些文件。


添加检查
--------

编译器检查可以很简单，例如::

  config_host_data.set('HAVE_BTRFS_H', cc.has_header('linux/btrfs.h'))


添加新依赖等复杂任务通常包括：

 - 在 meson_options.txt 添加构建选项；
 - 编写实际功能检测代码；
 - 将功能状态加入 config-host.h；
 - 在配置完成后的摘要中输出状态。

以 SDL2_Image 检测为例，meson_options.txt 包含::

  option('sdl_image', type : 'feature', value : 'auto',
         description: 'SDL Image support for icons')


除非命令行显式给出非 auto 值，否则只有依赖确实会被使用时才应检测::

  sdl_image = not_found
  if not get_option('sdl_image').auto() or have_system
    sdl_image = dependency('SDL2_image', required: get_option('sdl_image'),
                           method: 'pkg-config')
  endif


这可避免用户模式静态构建等情况下的警告，
因为系统模拟器所用多数库不支持静态链接。

其他配套代码通常很简单::

  # Create config-host.h (if applicable)
  config_host_data.set('CONFIG_SDL_IMAGE', sdl_image.found())

  # Summary
  summary_info += {'SDL image support': sdl_image.found()}


configure 解析新选项需要最新的 scripts/meson-buildoptions.sh，
make update-buildoptions 或直接 make 会自动更新它。


辅助脚本
--------

Meson 调用 Python 脚本有一个特殊约定：
若首行为 #! /usr/bin/env python3，且文件不可执行，
find_program() 会安排使用运行 Meson 的同一解释器执行脚本。
这是最常用和首选方式，因为它自动遵守 configure 的 --python= 设置。

非 Python 脚本应使用 #! /usr/bin/env ...，并设置可执行权限。

需要可执行权限的 Python 脚本，例如开发者可能直接运行的
测试脚本 tests/qapi-schema/test-qapi.py，
应通过 meson.build 中的 python 变量调用。例如::

  test('QAPI schema regression tests', python,
       args: files('test-qapi.py'),
       env: test_env, suite: ['qapi-schema', 'qapi-frontend'])


这样才能遵守 --python=，因为指定解释器可能不是 PATH 中第一个 python3。

Meson 运行时，依赖已在虚拟环境中准备好，
应通过 configure 放入 pyvenv 的脚本调用。
一种方式是使用 find_program::

  sphinx_build = find_program(
       fs.parent(python.full_path()) / 'sphinx-build',
       required: get_option('docs'))



阶段 3：Make
============

接下来运行 make。QEMU 要求 GNU Make，部分主机上其名称为 gmake。

Meson 输出 build.ninja，供 Ninja 使用。
但 QEMU 还需构建固件和 tests/tcg，它们需要不同交叉编译器。
顶层 Makefile 封装 Ninja 及这些较小构建系统，
并在 configure 改变后重新运行它。
除了调用这些子 Makefile，构建整体上基本不递归。

无论测试是否由 Meson 定义，Makefile 都通过传统的 make check 伪目标运行。
基准测试使用 make bench；unit 等 Meson 测试套件可用 make check-unit；
make check-tcg 为全部目标构建并运行非 Meson 测试。

也可直接用 ninja 构建模拟器，用 pyvenv/bin/meson test 运行 Meson 测试。
主要区别是 make 需要 -jN 才能启用并行构建或测试。

实用 Make 目标
--------------

``help``
  输出常用构建目标帮助。

``print-VAR``
  输出变量 VAR 的值，便于调试构建系统。


构建系统的重要文件
==================

静态定义文件
------------

以下文件在源码树中静态定义，包含构建规则，
其行为还受后文所列动态文件影响。

``Makefile``
  make 构建全部 QEMU 组件的入口，默认 all 目标构建所有组件。

``*/meson.build``
  根目录文件是 Meson 入口，协调全部可执行文件的配置和构建。
  子目录规则分布在源码树其他 meson.build 中。

``python/scripts/mkvenv.py``
  封装 Python venv 和 distlib.scripts，
  负责创建虚拟环境、生成 pyvenv/bin 脚本，以及调用 pip 安装依赖。

``tests/Makefile.include``
  TCG 等外部测试框架的规则。

``tests/docker/Makefile.include``
  Docker 测试规则。与 tests/Makefile.include 一样直接由顶层包含，
  因此其中的定义影响整个构建系统。

``tests/vm/Makefile.include``
  基于虚拟机的测试规则，同样直接由顶层包含，其中定义影响整个构建系统。

动态生成文件
------------

以下文件在运行时生成，控制 Makefile 行为，
避免像 Autotools 那样由 configure 将 Makefile.in 预处理为 Makefile。

configure 生成：

``run``
  从 Git 检出目录运行命令或脚本。
  将 PATH 指向本地构建程序，激活 Python 虚拟环境，再执行传入命令，例如::

    $ ./build/run ./script/qmp/qmp-shell-wrap qemu-system-x86_64


  将使用本地虚拟环境的 python3 和 site-packages 运行 qmp-shell-wrap，
  并从构建目录启动 QEMU。

``config-host.mak``
  configure 检测主机后，把各工具路径写入此文件，主要供 Makefile、部分供 Meson 使用。

  它也用于依赖检查：configure 修改时间更新时，make 会重新运行 configure。

``config-meson.cross``

  Meson 交叉文件或本机文件，传递工具链路径及其他配置。

``config.status``

  小型 shell 脚本，用首次执行时的环境变量重新调用 configure。
  用于源码变化后重新配置，也可手动查看以检查环境。

``Makefile.prereqs``

  Makefile 依赖集合，确保固件和测试在其所需容器镜像及模拟器之后构建和运行。

``pc-bios/*/config.mak``, ``tests/tcg/config-host.mak``, ``tests/tcg/*/config-target.mak``

  固件和 TCG 测试的构建配置，包括交叉工具链路径。

``pyvenv``

  构建期间全部 Python 代码使用的虚拟环境，
  确保通过 sphinx-build、meson 等运行的代码也使用同一解释器和包。

Meson 生成：

``config-host.h``
  供 C 代码了解整个构建环境属性和已启用功能。

``${TARGET-NAME}-config-devices.mak``
  TARGET-NAME 为系统模拟器名称，Meson 根据 configs/devices 中的文件生成。

``${TARGET-NAME}-config-target.mak``
  TARGET-NAME 为系统或用户模式模拟器名称，Meson 根据 configs/targets 生成。

``$TARGET_NAME-config-target.h``, ``$TARGET_NAME-config-devices.h``
  供 C 代码查询各目标属性和功能。
  通过 configure_file() 从对应 .mak 生成，
  各目标可分别用 CONFIG_TARGET 和 CONFIG_DEVICES 宏包含。

``build.ninja``
  构建规则。


Makefile 生成：

``Makefile.ninja``
  连接到 Ninja 实际构建的 Makefile 包含文件，
  主要列出 Meson 在 build.ninja 中定义的目标。

``Makefile.mtest``
  使 make check 运行 Meson 测试的定义。
  scripts/mtest2make.py 根据 meson introspect --tests 获取的 JSON 测试描述生成规则。
