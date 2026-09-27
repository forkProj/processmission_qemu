.. _checkfunctional-ref:

使用 Python 进行功能测试
========================

``tests/functional`` 包含 Python 功能测试。
它们通常是较高层测试，可能访问外部资源，并与各种客户机操作系统交互。

测试应采用 Python `unittest`_ 风格，通过标准输入输出使用 TAP 协议。
``tests/functional/qemu_test`` 提供 QemuBaseTest、QemuUserTest、QemuSystemTest
等类及辅助函数，帮助满足这些要求，例如替换 stdout 对象，将普通测试输出重定向到 stderr。

如果不使用 QemuBaseTest 派生类，或测试会启动子进程，
必须确保 stdio 不出现与 TAP 不兼容的输出。
可以为每行加上 "# "，将其标为 TAP 注释，
也可以捕获子进程 stdout，或将其重定向到 stderr。

基于 ``qemu_test.QemuSystemTest`` 的测试可以方便地：

 * 定制 ``self.vm`` 这一 QEMUMachine 实例的命令行参数。

 * 与 QEMU 监视器交互，发送 QMP 命令并检查结果。

 * 通过便利的控制台设备与客户机系统交互，验证命令行参数或 QMP 命令的效果。

 * 下载并缓存远程数据，例如固件和内核镜像。

运行测试
--------

执行以下命令运行功能测试：

.. code::

  make check-functional


也可只运行特定目标的测试，例如 x86_64：

.. code::

  make check-functional-x86_64


若不使用 Meson 测试运行器，可以通过环境变量指定模拟器程序，然后直接执行测试文件。

假设当前目录是源码根目录，构建目录为 ./build：::

  $ export QEMU_TEST_QEMU_BINARY=qemu-system-x86_64


运行文件中的全部测试：::

  $ ./build/run tests/functional/x86_64/test_virtio_version.py


运行某个测试类：::

  $ ./build/run tests/functional/x86_64/test_virtio_version.py VirtioVersionCheck


运行单个测试：::

  $ ./build/run tests/functional/x86_64/test_virtio_version.py VirtioVersionCheck.test_modern_only_devs


也支持按名称过滤：::

  $ ./build/run tests/functional/x86_64/test_virtio_version.py -k modern


框架会自动清理测试产生的临时文件。
调试失败时，可设置 ``QEMU_TEST_KEEP_SCRATCH=1`` 保留它们。
下次不设置该变量运行测试时，这些文件会被删除。

日志
--------

每个测试的日志保存在构建目录的以下子目录：::

 <builddir>/tests/functional/<arch>/<fileid>.<classid>.<testname>/


通常有三个文件：

* ``base.log``：测试代码通过 self.log.info()、self.log.debug() 等写入的通用日志。
* ``console.log``：客户机串口控制台输出。
* ``default.log``：QEMU 输出。如果测试为虚拟机选择了其他标识符，
  例如同时运行多个虚拟机，此文件名称也会不同。

编写测试入门
------------

``tests/functional/qemu_test`` 提供 qemu_test 模块及 QemuSystemTest 类。
简单示例如下：

.. code::

  #!/usr/bin/env python3

  from qemu_test import QemuSystemTest

  class Version(QemuSystemTest):

      def test_qmp_human_info_version(self):
          self.vm.launch()
          res = self.vm.cmd('human-monitor-command',
                            command_line='info version')
          self.assertRegex(res, r'^(\d+\.\d+\.\d)')

  if __name__ == '__main__':
      QemuSystemTest.main()


脚本开头提供 shebang、赋予可执行权限并调用 QemuSystemTest.main() 后，
可脱离运行器独立执行。
通过测试运行器执行时，main() 也负责按正确方式调用测试，
例如提供 Meson 要求的 TAP 输出。

``qemu_test.QemuSystemTest`` 基类
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

此类具有以下特点。

首先，它为每个测试准备 ``self.vm`` QEMUMachine 实例。
由于许多测试需要调整命令行，调用 ``self.vm.launch()`` 启动实例由测试作者负责。

基类也支持多个实例。``self.get_vm()`` 返回 QEMUMachine，
接受创建实例所需的参数，以及可选 name。
通过 name 可以识别特定机器，并在不同测试方法中重复获取同一实例。
示例如下：

.. code::

  from qemu_test import QemuSystemTest

  class MultipleMachines(QemuSystemTest):
      def test_multiple_machines(self):
          first_machine = self.get_vm()
          second_machine = self.get_vm()
          self.get_vm(name='third_machine').launch()

          first_machine.launch()
          second_machine.launch()

          first_res = first_machine.cmd(
              'human-monitor-command',
              command_line='info version')

          second_res = second_machine.cmd(
              'human-monitor-command',
              command_line='info version')

          third_res = self.get_vm(name='third_machine').cmd(
              'human-monitor-command',
              command_line='info version')

          self.assertEqual(first_res, second_res, third_res)


测试清理阶段，QemuSystemTest 负责关闭全部实例。

QEMUMachine
-----------

此 API 已广泛用于 Python iotests、device-crash-test 及其他脚本。
它包装 QEMU 程序执行，提供：

 * 设置 QEMU 命令行参数的能力。

 * 已就绪的 QMP 连接和接口，可发送命令、检查结果及异步事件。

 * 更简洁、直观地设置常用参数的便利方法。

选择 QEMU 程序
^^^^^^^^^^^^^^

self.vm 使用的程序主要取决于 qemu_bin 实例属性。
测试未显式设置时，默认取 QEMU_TEST_QEMU_BINARY 环境变量。

调试挂起的 QEMU
^^^^^^^^^^^^^^^

测试异常时，可能需要调试停滞的 QEMU 进程。
虽然主 QMP socket 由 QEMUMachine 占用，
仍可将 QEMU_TEST_QMP_BACKDOOR 设为一个 Unix socket 路径，请求额外的 QMP 监视器。
随后使用 qmp-shell 连接，检查当前状态。

属性参考
--------

QemuBaseTest
^^^^^^^^^^^^

以下属性适用于所有 qemu_test.QemuBaseTest 实例。

arch
""""

QEMU 程序的目标架构。

测试也可以按需要使用它，例如选择启动虚拟机所用内核或磁盘镜像的架构。

qemu_bin
""""""""

保存 QEMU_TEST_QEMU_BINARY 环境变量的值。

QemuUserTest
^^^^^^^^^^^^

用于通过用户态模拟器运行可执行程序。

QemuSystemTest
^^^^^^^^^^^^^^

用于通过 qemu-system-* 程序运行测试。

vm
""

QEMUMachine 实例，初始根据 qemu_bin 配置。

cpu
"""

测试创建的全部 QEMUMachine 实例使用的 CPU 型号。

machine
"""""""

测试创建的全部实例使用的机器类型。
通过 QemuSystemTest.set_machine() 设置时，可以自动检查该机器是否编入 QEMU，
未编入时跳过测试。

资源处理
--------

许多功能测试从网络下载 Linux 内核、initrd、固件等资源，给框架带来额外要求。

首先，部分用户的网络受限，因此默认 make check 不应运行这些测试。
需要下载的测试只应加入 meson.build 的 thorough 模式；
不需要下载的功能测试可以加入 quick。
make check 只运行 quick 功能测试及其他套件的快速测试。
单独运行 make check-functional 时，thorough 测试也会执行。
要与其他测试一起运行全部功能测试，可使用：::

  make -j$(nproc) check SPEED=thorough


其次，下载耗时取决于网络带宽，不应占用测试运行的超时预算。
因此框架在运行前预先下载资源，通过 qemu_test.Asset 实现缓存。
在测试类中声明 URL 和 SHA256，例如：::

    from qemu_test import Asset

    ASSET_somename = Asset(
        ('https://www.qemu.org/assets/images/qemu_head_200.png'),
        '34b74cad46ea28a2966c1d04e102510daf1fd73e6582b6b74523940d5da029dd')


测试函数中获取缓存文件名：::

    def test_function(self):
        file_path = self.ASSET_somename.fetch()


make check-functional 会自动预缓存，但 make check-functional-<target> 等不会。
只下载、不运行测试时，使用：::

    make precache-functional


默认缓存目录为 ``~/.cache/qemu/download``，可通过 QEMU_TEST_CACHE_DIR 修改。

设置 QEMU_TEST_REFRESH_CACHE 可强制重新下载，即使缓存仍有效。

跳过测试
--------

框架基于 Python unittest，因此可使用常见装饰器，按条件跳过测试，
例如缺少程序，或运行环境是 CI。
更多信息见：

  https://docs.python.org/3/library/unittest.html#skipping-tests-and-expected-failures

虽然跳过条件常因测试而异，但一些常见场景形成了通过环境变量启停测试的惯例。

常用变量如下。

QEMU_TEST_ALLOW_LARGE_STORAGE
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

需要获取或生成大型资源的测试，只有设置 QEMU_TEST_ALLOW_LARGE_STORAGE=1 才运行。

大型的定义并不严格，通常指解压后至少占用 1 GB 磁盘空间。

QEMU_TEST_ALLOW_UNTRUSTED_CODE
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

有些测试启动的内核或固件被认为不适合在开发者工作站安全运行，因此默认跳过。
通常指源码或构建过程不公开的二进制内容，但判断标准并不完全固定。

设置 QEMU_TEST_ALLOW_UNTRUSTED_CODE=1 后才允许运行这类测试。

QEMU_TEST_FLAKY_TESTS
^^^^^^^^^^^^^^^^^^^^^

不稳定的测试默认关闭，包括无法在 GitLab CI 中稳定运行的测试。
它们往往暴露真实问题，只是 CI 的限制使这些问题比开发机上更容易出现。
遇到类似情况时，应报告缺陷，并按下例标记测试：

.. code::

  # See https://gitlab.com/qemu-project/qemu/-/issues/nnnn
  @skipUnless(os.getenv('QEMU_TEST_FLAKY_TESTS'), 'Test is unstable on GitLab')
  def test(self):
      do_something()


测试不应永久处于这种状态，必须修复，或最终移除。

QEMU_TEST_ALLOW_SLOW
^^^^^^^^^^^^^^^^^^^^

运行时间很长的测试，可能在启用调试选项构建 QEMU 等情况下超时。
为避免默认超时，也节省普通测试的 CPU 时间，
只有设置 QEMU_TEST_ALLOW_SLOW 才运行这些测试。

.. _unittest: https://docs.python.org/3/library/unittest.html
