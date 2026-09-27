.. _testing:

QEMU 测试
=========

QEMU 的测试基础设施覆盖单元测试、子系统测试，直到完整功能测试，因此较为复杂。
在源码或构建目录运行 ``make check-help``，可查看概览。

大多数测试也集成到 Meson，可直接从构建目录运行。例如：::

  [./pyvenv/bin/]meson test --suite qemu:softfloat


只运行 softfloat 测试。

自动化测试使用框架提供的通用函数或类编写，
由框架执行并报告成功或失败 [1]_。

通常分三部分：

1. 初始化参数，包括输入和预期结果。
2. 调用待测代码。
3. 将实际结果与预期结果比较，匹配则成功，否则失败。

下文介绍各测试组。

使用 make check 测试
--------------------

make check 系列包含大多数 C 测试。通常运行：

.. code::

  make check


包括 QAPI schema、单元测试、QTest 和部分 iotests，各子类将在下文说明。

测试前最好先构建 QEMU 程序。
部分测试要求可执行文件已存在，找不到时可能显示难以理解的错误。

超时设置较为宽松，通常不会触发。
但在较慢宿主或启用 Clang AddressSanitizer 等较慢配置下，
可以通过 TIMEOUT_MULTIPLIER 全局放大超时。例如：

.. code::

 TIMEOUT_MULTIPLIER=3 make check


将默认超时乘以三。设为 0 则完全禁用超时。

.. _unit-tests:

单元测试
~~~~~~~~

单元测试独立验证接口、数据结构或功能等软件组件，发现组件边界内的错误。
它针对最小软件单元，关注内部处理逻辑和数据结构，
用例应覆盖计算错误、比较错误及不当控制流 [2]_。

通过 ``make check-unit`` 运行。
通常是链接个别 QEMU 目标文件、调用导出函数的简单 C 测试。

编写新代码时，尤其是状态较少、依赖较少的工具模块，可考虑增加单元测试：

1. 创建源文件，例如 ``tests/unit/foo-test.c``。

2. 引入模块 API 头文件，验证接口行为，并使用 GLib 测试框架组织代码。
   通常可以参考并修改已有测试。

3. 加入 ``tests/unit/meson.build`` 的 tests 字典。
   值为需要额外链接的源文件和依赖。
   简单测试只需加入：::

     {
       ...
       'foo-test': [],
       ...
     }


单元测试通常不需要环境变量，直接调用或在 GDB 中运行往往最方便。
但 make 与手动运行仍可能因 ``MALLOC_PERTURB_`` 环境变量和 gtester 选项而表现不同。
前者影响内存回收，更容易暴露无效指针。
必要时运行：

.. code::

  make check-unit V=1


复制实际命令行，再手动执行。

QTest
~~~~~~~~

QTest 用于测试设备模拟，还可以通过专用协议控制虚拟时钟步进等行为。
详见 :doc:`qtest`。

运行方式：

.. code::

   make check-qtest


编写可移植测试
~~~~~~~~~~~~~~

单元测试和 QTest 可在 POSIX 与 Windows 宿主运行，应注意：

* 必要时使用 GLib 可移植 API，例如 g_setenv()、g_mkdtemp()、g_mkdir()。
* 不要将临时目录写死为 /tmp，使用 g_get_tmp_dir()。
* Windows 对标准输入输出及空设备的表示不同。
  Linux 的 /dev/fd/2、/dev/null 在 Windows 应使用 2、nul。
  Windows 上 I/O 重定向不工作，应避免依赖 2>nul。
* blkdebug 配置和镜像应使用相对路径，避免 Windows 绝对路径中的冒号干扰解析。
* 额外 QEMU 命令行使用双引号，Windows 不会在传参时去掉单引号。
* Windows 默认以文本模式打开文件，而 POSIX 对文本和二进制无此区分。
  写入后需要逐字比较时，应为 fopen() 模式加入 b，或为 open() 加入 O_BINARY。
* 仅支持 POSIX 或 Linux 的用例应使用适当条件编译；
  整个套件无法在 Windows 运行时，在 meson.build 禁用构建。

.. _qapi-tests:

QAPI schema 测试
~~~~~~~~~~~~~~~~

此类测试向 QMP 使用的 QAPI 解析器输入预定义内容，再与参考输出比较。

数据保存在 ``tests/qapi-schema``，每个用例有四个同名主体文件：

  * ``${casename}.json``：解析器输入。
  * ``${casename}.out``：预期标准输出。
  * ``${casename}.err``：预期标准错误。
  * ``${casename}.exit``：预期退出码。

修改解析器、修复缺陷或调整语法时，应考虑添加用例：

1. 添加上述四个文件，例如：

  ``$EDITOR tests/qapi-schema/foo.{json,out,err,exit}``。

2. 加入 ``tests/Makefile.include``，例如：

  ``qapi-schema += foo.json``

设置 QEMU_TEST_REGENERATE 后运行测试，可以按最新生成器更新参考输出：

.. code::

   QEMU_TEST_REGENERATE=1 make check-qapi-schema


作者必须审阅变化，确认符合预期，再将参考输出与生成器修改放入同一提交。

.. _tracetool-tests:

Tracetool 测试
~~~~~~~~~~~~~~

此类测试验证不同跟踪后端及源码格式生成的探针定义。
运行 tracetool 处理示例 trace-events，并与已知正确输出比较：

.. code::

  make check-tracetool


参考文件位于 tests/tracetool。
有意修改后端或格式输出时，需要更新参考，可设置 QEMU_TEST_REGENERATE=1：

.. code::

   QEMU_TEST_REGENERATE=1 make check-tracetool


作者必须先审阅确认，再与生成器修改一起提交。

check-block
~~~~~~~~~~~

可通过多种 make 目标，为不同格式和协议（下文统称驱动）运行块 I/O 测试。

默认 make check 或 make check-block 只运行 qcow2 的 auto 组测试。

SPEED 可扩展范围。SPEED=slow 运行 qcow2 和 raw 的全部测试；
SPEED=thorough 运行 luks、nbd、parallels、qcow2、qed、raw、vdi、vhdx、vmdk、vpc
的所有可用测试。

各驱动也有独立的 ``make check-block-$DRIVER``，
无需设置 SPEED，即运行该驱动的全部测试。

框架细节见下文 QEMU iotests 一节。

.. _qemu-iotests:

QEMU iotests
------------

``tests/qemu-iotests`` 广泛用于块层功能测试，层次高于 make check 中的测试，
约 99% 代码为 Bash 或 Python。成功与否由参考输出比较决定，用例以数字命名。

先构建 QEMU，再进入构建目录下的 tests/qemu-iotests，
运行带适当参数的 ./check。

默认使用 raw 格式和 file 协议，执行除不支持用例外的全部测试。
可以覆盖格式和协议：

.. code::

  # test with qcow2 format
  ./check -qcow2
  # or test a different protocol
  ./check -nbd


也可显式列出测试编号：

.. code::

  # run selected cases with qcow2 format
  ./check -qcow2 001 030 153


-c 选择缓存模式，有助于发现特定模式下的缺陷。
更多选项见 ``./check -h``。

编写新用例
~~~~~~~~~~

修改块层时应考虑增加测试，iotest 通常是合适选择。
已有大量用例，扩展现有测试可能比新建更方便。
从数百个测试中寻找相关用例没有完全可靠的方法，可以尝试 git grep。

通常一个用例由可执行脚本及参考输出组成，例如 055 和 055.out。
脚本向 stdout、stderr 输出内容。

少数用例在 cache=none 下输出不同时，增加 .out.nocache。
不同镜像格式输出不同时，则使用 178.out.qcow2、178.out.raw 等文件。

没有严格规定脚本写法，通常复制并修改现有用例。常见方式包括：

* Bash：使用测试环境变量，并加载 common.* 辅助库。
* Python unittest：导入 iotests，继承 iotests.QMPTestCase，调用 iotests.main。
  缺点是输出较少，调试相对困难。
* 普通 Python：导入 iotests 启动 QEMU 和使用工具，但不继承 QMPTestCase，
  因而不通过 unittest 执行，结合前两种方式。

Bash 和 Python 对调用、交互 QEMU 的支持相近，可按偏好选择。
使用 Python 时强烈建议兼容 Python 3。

两种框架都有创建和清理测试镜像的辅助函数。
无需真实 I/O 或协议特性时，通常使用 null-co:// 伪驱动更方便，无需创建或删除文件。
尽量避免 /dev/null、/dev/zero 等系统级设备和文件，否则必须考虑锁冲突。
例如，其他宿主应用已锁定文件可能导致测试失败。
确实需要这些设备时，可考虑 locking=off。

调试用例
~~~~~~~~

check 的以下选项有助于排查失败：

* ``-gdb``：用 gdbserver 包装每次 QEMU 调用，并等待 GDB 连接。
  gdbserver 参数来自 GDB_OPTIONS；为空时默认监听 localhost:12345。
  可使用 ``gdb -iex "target remote $addr"`` 连接。
  未指定 -gdb 时，GDB_OPTIONS 即使设置也会被忽略。

* ``-valgrind``：附加 Valgrind。发现警告时打印并保存到
  ``$TEST_DIR/<valgrind_pid>.valgrind``。
  最终命令为 ``valgrind --log-file=$TEST_DIR/<valgrind_pid>.valgrind --error-exitcode=99 $QEMU ...``。

* ``-d``：增加日志详细程度，例如显示 QMP 命令和应答。

* ``-p``：把 QEMU 标准输出和标准错误重定向到测试输出，
  而非保存到 ``$TEST_DIR/qemu-machine-<random_string>`` 的日志。

用例分组
~~~~~~~~

测试可以属于一个或多个组，通过源码注释定义。
惯例是在 shebang 后的第二行列出：

.. code::

  #!/usr/bin/env python3
  # group: auto quick
  #
  ...


也可以创建 tests/qemu-iotests/group.local。
该文件只供下游使用，不应进入上游，可定义下游分组或临时禁用测试：

.. code::

  # groups for some company downstream process
  #
  # ci - tests to run on build
  # down - our downstream tests, not for upstream
  #
  # Format of each line is:
  # TEST_NAME TEST_GROUP [TEST_GROUP ]...

  013 ci
  210 disabled
  215 disabled
  our-ugly-workaround-test down ci


以下名称具有特殊意义：

- quick：应在几秒内完成。

- auto：用于 make check，应在各种环境中都能运行：
  支持非 x86 等所有 QEMU 程序、各种配置；可选功能未编入时可跳过，但不应失败；
  至少支持 qcow2，适应各宿主文件系统及 nobody、root 等用户；
  不应占用过多内存和磁盘，否则容易使 CI 失败。

- disabled：禁用，check 会忽略。

.. _container-ref:

基于容器的测试
--------------

简介
~~~~~~~~

QEMU 容器框架使用公开镜像，在预定义、易于获得的 Linux 环境中构建和测试，
扩展发行版、工具链和库版本的覆盖范围。
最初面向 Docker，现在也支持 Podman。
虽然不少目标和脚本仍以 docker 命名，系统会自动使用已配置的运行时。

容器也用于扩展 TCG 测试的构建，详见 :ref:`checktcg-ref`。

Docker 前提条件
~~~~~~~~~~~~~~~

用系统包管理器安装 Docker，启动服务，并确保有运行权限。
通常使用免密码 sudo docker 或 root，例如：

.. code::

  $ sudo yum install docker
  $ # or `apt-get install docker` for Ubuntu, etc.
  $ sudo systemctl start docker
  $ sudo docker ps


最后应输出空表，表示准备就绪。

也可以将用户加入 docker 组，并使该组可访问默认的 /var/run/docker.sock：

.. code::

  $ sudo groupadd docker
  $ sudo usermod $USER -a -G docker
  $ sudo chown :docker /var/run/docker.sock


上述任一方式都使用户能通过绑定挂载或其他特权操作控制整个宿主，
因此只应在开发机上这样配置。

Podman 前提条件
~~~~~~~~~~~~~~~

用系统包管理器安装 Podman：

.. code::

  $ sudo dnf install podman
  $ podman ps


最后应输出空表。

快速入门
~~~~~~~~

在源码树运行 make docker-help 查看帮助。
无需先配置或构建 QEMU：configure 和 make 都会在容器中，按目标定义的参数执行：

.. code::

  make docker-test-build@debian


这会自动下载并初始化 debian 镜像，创建容器，运行 test-build 作业。

镜像仓库
~~~~~~~~

项目使用 GitLab 上的 ``registry.gitlab.com/qemu-project/qemu``，自动拉取预构建层，
避免多个开发者重复构建，对发行版仓库造成额外负担。
可通过 NOCACHE 覆盖：

.. code::

   make docker-image-debian-arm64-cross NOCACHE=1


镜像
~~~~~~~~

debian 等镜像的 Dockerfile 位于 tests/docker/dockerfiles/，
例如 debian.docker。make docker-help 列出全部可用镜像。

可在 .docker 旁添加 .pre 脚本，在镜像构建前于构建上下文目录执行，
主要用于宿主侧准备，例如配置 binfmt_misc，使基于 qemu-user 的交叉构建容器可用。

过去多数 Dockerfile 手写，导致不同容器的软件包集合不一致。
今后项目希望使用 libvirt-ci 提供的 lcitool 自动生成：

  https://gitlab.com/libvirt/libvirt-ci

libvirt-ci 包含 lcitool，以及多个第三方项目到发行版软件包名称的映射。
工具根据 tests/lcitool/projects/qemu.yml 中的依赖，
确定各发行版需安装的软件包，生成一致的构建环境。

添加构建依赖
^^^^^^^^^^^^

新增 QEMU 构建依赖时，应更新 tests/lcitool/projects/qemu.yml，确保 CI 环境具备它。

如果 libvirt-ci 已知该依赖：

 * 修改 tests/lcitool/projects/qemu.yml，加入依赖。
 * 运行 make lcitool-refresh，重新生成相关环境清单。

如果尚未知，需要先向 libvirt-ci 贡献映射：

 * 在 GitLab fork libvirt-ci。
 * 在 lcitool/facts/mappings.yml 添加依赖，尽量列出各系统的原生包名，
   运行 ``python -m pytest --regenerate-output`` 并检查结果。
 * 将映射和重新生成的测试文件一起提交，发起合并请求，
   说明这是 QEMU 需要的新构建依赖。
 * CI 会尝试在全部支持系统安装新软件包，验证映射。
 * 合入后，回到 QEMU，更新 tests/lcitool/libvirt-ci 子模块到包含该修改的提交，
   再添加依赖并运行 make lcitool-refresh。
 * 尽可能为多个发行版触发 GitLab 容器生成流水线，确认没有明显问题。
   操作见 `CI 文档 <https://www.qemu.org/docs/master/devel/ci.html>`__。

默认 Python 过旧且已终止支持的企业发行版，采用适配较新 Python 的单独映射，
位于 tests/lcitool/mappings.yml。
除非新增 Python 库或工具，通常无需修改此文件。

添加操作系统发行版
^^^^^^^^^^^^^^^^^^

libvirt-ci 可能尚不支持目标发行版，添加之前应先讨论：

 * 向 qemu-devel 发邮件，抄送 MAINTAINERS 中 Build and test automation 的负责人。
   CI 计算资源有限，需要权衡新增发行版的成本和收益。
 * 在 https://gitlab.com/libvirt/libvirt-ci/-/issues 提交问题，附上邮件归档链接，
   避免重复工作，也便于获得维护者建议。

达成一致后：

 * 在 GitLab fork libvirt-ci。
 * 在 lcitool/facts/targets/ 添加元数据。
   如果使用尚不支持的包格式，可能还需修改代码，可向维护者咨询。
 * 在 lcitool/facts/mappings.yml 为新系统添加尽量完整的包名映射，
   运行 ``python -m pytest --regenerate-output`` 并检查。
 * 将 lcitool/facts 和生成的测试修改一起提交并发起合并请求，说明 QEMU 的需求。
 * CI 会在各支持系统尝试安装软件包，验证映射。
 * 合入后，回到 QEMU，更新 libvirt-ci 子模块到包含映射的提交。

测试
~~~~~~~~

各测试覆盖不同的 QEMU 构建和运行配置。
Docker 测试是 tests/docker 下名为 test-* 的可执行脚本，通常为 shell，
基于 common.rc 提供的源码定位和构建辅助函数。

完整列表见 make docker-help。

排查 Docker 测试失败
~~~~~~~~~~~~~~~~~~~~

收到 CI、维护者或自己的失败报告后：

1. 用报告中的命令在本地复现，例如 ``make docker-test-mingw@fedora-win64-cross J=8``。
2. 加入 V=1，查看详细输出。
3. 再加入 DEBUG=1，在测试开始前进入容器 shell。
   可以手动构建和运行，或按 :kbd:`Ctrl+d` 继续。
4. 继续后会执行相同构建和测试，若再次失败，将返回 shell，便于调试。

选项
~~~~~~~~

全部选项见 make docker 帮助，常用项为：

* ``V=1``：与顶层 make 相同，会传入容器并启用详细输出。
* ``J=$N``：容器内 make 的并行任务数。
  顶层的 -j 不会传入容器，需要单独设置。
* ``DEBUG=1``：启用前述交互调试。

Thread Sanitizer
----------------

TSan 用于检测数据竞争，QEMU 支持用它构建和测试。

更多信息见：

https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual

在 Docker 中使用 TSan
~~~~~~~~~~~~~~~~~~~~~

所有宿主 Docker 镜像均支持 TSan，例如 ubuntu2404。

test-tsan 会启用 TSan 构建，再运行 make check：

.. code::

  make docker-test-tsan@ubuntu2404


警告写入 build/tsan/。
建议使用 DEBUG=1，在容器中启动测试并检查警告。

使用 TSan 构建和测试
~~~~~~~~~~~~~~~~~~~~

也可以在容器外操作，但需要一些额外步骤，容器中通常会自动完成。

Clang 和 GCC 都支持 TSan。
Sanitizer 通常要求全部代码，包括共享库依赖，都以相同方式构建。
否则 GLib 的 GMutex 等同步原语不会被 TSan 正确识别，导致误报。

构建 TSan 版 GLib：

.. code::

   $ git clone --depth=1 --branch=2.81.0 https://github.com/GNOME/glib.git
   $ cd glib
   $ CFLAGS="-O2 -g -fsanitize=thread" meson build
   $ ninja -C build


配置 QEMU：

.. code::

  ../configure --enable-tsan \
               --disable-werror --extra-cflags="-O0"


运行时记得指向 TSan 版 GLib：

.. code::

   $ glib_dir=/path/to/glib
   $ export LD_LIBRARY_PATH=$glib_dir/build/gio:$glib_dir/build/glib:$glib_dir/build/gmodule:$glib_dir/build/gobject:$glib_dir/build/gthread
   # check correct version is used
   $ ldd build/qemu-x86_64 | grep glib
   $ qemu-system-x86_64 ...


运行时行为由 TSAN_OPTIONS 控制，详见：

https://github.com/google/sanitizers/wiki/ThreadSanitizerFlags

例如：

.. code::

  export TSAN_OPTIONS=suppressions=<path to qemu>/tests/tsan/suppressions.tsan \
                      detect_deadlocks=false history_size=7 exitcode=0 \
                      log_path=<build path>/tsan/tsan_warning


exitcode=0 使出现警告时仍继续，不以错误退出，便于跑完后统一检查。
如果希望警告导致错误退出，使用 exitcode=66。

.. _tsan-suppressions:

TSan 警告抑制
~~~~~~~~~~~~~

检测到竞争不一定意味着真实缺陷。
TSan 提供多种抑制机制，但通常应尽可能修正代码、消除竞争，而不是隐藏警告。

相关文件如下。

tests/tsan/suppressions.tsan：运行时抑制规则，注释通常说明原因。
格式见：

https://github.com/google/sanitizers/wiki/ThreadSanitizerSuppressions

tests/tsan/ignore.tsan：用于测试或调试的编译期禁用规则。
通过以下 configure 参数启用：

"--extra-cflags=-fsanitize-blacklist=<src path>/tests/tsan/ignore.tsan"

格式见以下页面的 Blacklist Format：

https://github.com/google/sanitizers/wiki/ThreadSanitizerFlags

TSan 注解
~~~~~~~~~

include/qemu/tsan.h 定义了注解，具体说明见该文件。
注解可以抑制警告，也可以提供更多信息，使 TSan 正确识别数据访问之间的关系。

示例见：

https://github.com/llvm/llvm-project/tree/master/compiler-rt/test/tsan/

可以先阅读 annotate_happens_before.cpp 和 ignore_race.cpp。

完整注解集合见：

https://github.com/llvm/llvm-project/blob/master/compiler-rt/lib/tsan/rtl/tsan_interface_ann.cpp

docker-binfmt-image-debian-% 目标
---------------------------------

可结合 Debian 引导脚本和已配置的 binfmt_misc，
构建多个 Debian 发行版，包括尚未被正式系统支持的实验性移植。
用 Docker 容纳异架构 rootfs，比手动调用 chroot 更简单。

配置 binfmt_misc
~~~~~~~~~~~~~~~~

qemu-binfmt-conf.sh 可以配置 QEMU 用户态程序，自动执行异架构二进制。
虽然脚本尽量支持动态链接 QEMU，复制进容器时静态链接程序的问题更少。
现代内核的 F 标志会在配置时打开程序，避免在 chroot 中重新查找和打开，
通过 --persistent 启用。

调用示例
~~~~~~~~

例如，配置 Debian HPPA 移植构建：::

  make docker-binfmt-image-debian-sid-hppa \
    DEB_TYPE=sid DEB_ARCH=hppa \
    DEB_URL=http://ftp.ports.debian.org/debian-ports/ \
    DEB_KEYRING=/usr/share/keyrings/debian-ports-archive-keyring.gpg \
    EXECUTABLE=(pwd)/qemu-hppa V=1


``DEB_`` 变量供 debian-bootstrap.pre 替换，
在复制 rootfs 进容器前执行第一阶段 debootstrap。
第二阶段在构建时运行，最终镜像标签为 qemu/debian-sid-hppa。

虚拟机测试
----------

此套件通过脚本准备各种客户机镜像，并安装构建 QEMU 所需软件包。
基本用法见 make vm-help。

快速入门
~~~~~~~~

make vm-help 列出目标。运行特定命令即可在镜像中构建，
例如 make vm-build-freebsd 使用 FreeBSD 镜像。
可从源码或构建目录执行；从源码目录执行时无需先 configure。
测试镜像生成在工作目录的 ./tests/vm/ 下。

注意：这些镜像接受公开已知的 RSA 密钥对进行 SSH 登录。
如果担心攻击者控制客户机，继而利用 QEMU 漏洞攻击宿主，不应将其暴露到外部接口。

QEMU 程序
~~~~~~~~~

默认在 PATH 查找 qemu-system-x86_64。
没有该程序或版本低于 2.10 时，测试无法运行，
可设置 ``QEMU=/path/to/qemu-2.10+`` 指定。

qemu-img 路径同样可通过 QEMU_IMG 设置。

Make 并行任务
~~~~~~~~~~~~~

命令行 -j 不会传入虚拟机，使用 J=$X 控制客户机并行任务数。

调试
~~~~~~~~

在 make 命令加入 DEBUG=1 或 V=1，启用交互调试和详细输出。
V=1 会传给客户机内的 make。仍不足时可手动调用脚本。

手动调用
~~~~~~~~

各客户机脚本均可执行，并使用相同选项。
例如 NetBSD 使用 ``$QEMU_SRC/tests/vm/netbsd``：

.. code::

    $ cd $QEMU_SRC/tests/vm

    # To bootstrap the image
    $ ./netbsd --build-image --image /var/tmp/netbsd.img
    <...>

    # To run an arbitrary command in guest (the output will not be echoed unless
    # --debug is added)
    $ ./netbsd --debug --image /var/tmp/netbsd.img uname -a

    # To build QEMU in guest
    $ ./netbsd --debug --image /var/tmp/netbsd.img --build-qemu $QEMU_SRC

    # To get to an interactive shell
    $ ./netbsd --interactive --image /var/tmp/netbsd.img sh


添加客户机
~~~~~~~~~~

可以参考现有脚本。核心步骤是继承 BaseVM，
实现 build_image()，定义 BUILD_SCRIPT，再从 main() 调用 basevm.main()。

* build_image() 通常从预定义 URL 下载模板镜像。
  BaseVM._download_with_cache() 处理缓存和校验和，建议使用。

* 下载后配置用户、SSH 和构建依赖：

  - root 密码设为 BaseVM.ROOT_PASS。
  - 创建 BaseVM.GUEST_USER，密码设为 BaseVM.GUEST_PASS。
  - 启用 SSH 并设为开机启动，
    将 ``$QEMU_SRC/tests/keys/id_rsa.pub`` 加入 root 和普通用户的 authorized_keys。
  - 启用 DHCP 客户端并开机启动，使 virtio-net-pci 自动配置，
    与 QEMU 用户网络 10.0.2.2 通信。
  - 安装解压源码并构建 QEMU 所需的软件包。

* 编写 BUILD_SCRIPT shell 模板：从 raw virtio-blk 设备解压源码压缩包，
  再配置和构建，建议同时运行 make check。

镜像模糊测试
------------

镜像测试器用于测试格式驱动，目前只支持 qcow2。启动方式：

.. code::

  tests/image-fuzzer/runner.py -c '[["qemu-img", "info", "$test_img"]]' /tmp/test qcow2


修改 -c 也可以测试 qemu-img info 之外的命令。

Python 功能测试
---------------

功能测试关注软件需求，尝试发现功能、接口、行为以及初始化和终止等错误 [3]_。

tests/functional 保存 Python 测试，运行：

.. code::

  make check-functional


详见 :ref:`checkfunctional-ref`。

该测试框架也遵循 TIMEOUT_MULTIPLIER。

.. _checktcg-ref:

使用 make check-tcg 测试
------------------------

check-tcg 对 linux-user 和 softmmu 的 TCG 功能做简单冒烟测试。
构建客户机测试程序需要交叉编译器。
发行版提供时可以直接安装，例如：::

  apt install gcc-aarch64-linux-gnu


configure 会自动检测。有些编译器名称特殊，可以通过对应架构选项显式提示：::

  $(configure) --cross-cc-aarch64=aarch64-cc


若目标需要额外编译参数，还可使用 --cross-cc-cflags-ARCH。

如果当前用户能运行容器，缺少系统编译器时会自动使用容器。
支持构建 QEMU 的架构通常复用相同容器；
此外也有只含最小交叉环境、仅用于构建测试的容器。
某些测试需要受支持 LTS 发行版尚未提供的新编译器功能，可能使用较新的发行版。

详见 :ref:`container-ref`。

运行测试子集
~~~~~~~~~~~~

只构建一个架构的测试：::

  make build-tcg-tests-$TARGET


运行：::

  make run-tcg-tests-$TARGET


加入 V=1 显示具体 QEMU 调用，便于调试。

运行单个测试
~~~~~~~~~~~~

也可直接进入测试构建目录运行，make help 会列出全部目标。
同一个二进制可能用于多个测试，例如：::

  make run-plugin-test-mmap-with-libinline.so


这会使用 libinline.so TCG 插件运行 mmap 测试。
gdbstub 测试也复用这些程序，同时测试 GDB。

TCG 测试依赖
~~~~~~~~~~~~

依赖刻意保持很少：系统模式基本裸机，仅使用最少的 GCC 库；
linux-user 只使用 glibc。原因是交叉编译器配合额外库配置较困难。

其他 TCG 测试
-------------

一些树外套件用于更全面地测试处理器功能。

KVM 单元测试
~~~~~~~~~~~~

虽然设计为在 KVM 下作为客户机系统运行，也可用于 TCG。
它提供最小内核，包含启用 MMU 和通过特殊设备报告结果的钩子：::

  https://git.kernel.org/pub/scm/virt/kvm/kvm-unit-tests.git


Linux Test Project
~~~~~~~~~~~~~~~~~~

LTP 测试 Linux 系统调用接口是否符合文档，并尽量覆盖边界情况，
适合验证 QEMU linux-user：::

  https://linux-test-project.github.io/


GCC gcov 支持
-------------

gcov 通过插桩分析测试覆盖率。
使用 --enable-gcov 配置并构建 QEMU，再正常运行测试。

若只收集单个测试，可先用 make clean-gcda 删除已有覆盖数据。

make coverage-html 生成 HTML 报告，入口为 meson-logs/coveragereport/index.html。

也可以直接对 .gcda 文件运行 gcov，详见其文档。

不稳定测试
----------

同一份代码在不同运行中既成功又失败的测试称为不稳定测试。
常见原因是异步等待、并发和测试顺序依赖 [4]_。

QEMU 通常默认禁用已确认不稳定的测试，设置 QEMU_TEST_FLAKY_TESTS 可启用。

参考文献
--------

.. [1] Sommerville, Ian (2016). Software Engineering. p. 233.
.. [2] Pressman, Roger S. & Maxim, Bruce R. (2020). Software Engineering,
       A Practitioner’s Approach. p. 48, 376, 378, 381.
.. [3] Pressman, Roger S. & Maxim, Bruce R. (2020). Software Engineering,
       A Practitioner’s Approach. p. 388.
.. [4] Luo, Qingzhou, et al. An empirical analysis of flaky tests.
       Proceedings of the 22nd ACM SIGSOFT International Symposium on
       Foundations of Software Engineering. 2014.
