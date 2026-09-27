.. _ci_var:

自定义 CI/CD 变量
=================

可以通过 CI 环境变量调整 QEMU 的 CI 流水线。

在用户 CI 命名空间中全局设置
----------------------------

变量可在用户 CI 命名空间设置中全局定义。
具体方法见::

  https://docs.gitlab.com/ee/ci/variables/#add-a-cicd-variable-to-a-project

推送分支或标签到用户仓库时手动设置
----------------------------------

推送分支或标签时，可以通过 git-push 命令行参数手动设置变量。
以下设置 QEMU_CI_EXAMPLE_VAR：

.. code::

   git push -o ci.variable="QEMU_CI_EXAMPLE_VAR=value" myrepo mybranch

具体方法见::

  https://docs.gitlab.com/ee/user/project/push_options.html#push-options-for-gitlab-cicd

在 Git 配置中设置别名
---------------------

通过别名可以方便地使用不同 CI 配置推送分支。
例如定义触发 CI 的别名：

.. code::

   git config --local alias.push-ci "push -o ci.variable=QEMU_CI=1"
   git config --local alias.push-ci-now "push -o ci.variable=QEMU_CI=2"

随后运行：

.. code::

   git push-ci

即可创建流水线；或者运行：

.. code::

   git push-ci-now

创建并立即运行流水线。


变量命名和分组
--------------

QEMU CI 配置使用的变量划分为几个命名空间：

 * QEMU_JOB_nnnn：在单个作业或模板中定义，影响 .base_job_template 中的公共规则。

 * QEMU_CI_nnn：贡献者在仓库 CI 设置中定义，或通过 git push 传入，
   控制流水线运行哪些作业。

 * QEMU_CI_CONTAINER_TAG：阶段 1 发布容器时使用的标签，供阶段 2 构建作业使用。
   默认为 latest；多个分支并发运行流水线时，应为各流水线单独覆盖。

 * QEMU_CI_UPSTREAM：视为上游的 GitLab 命名空间，默认为 qemu-project。
   修改 base.yml 规则且需要验证上游上下文行为，而非自己 fork 的行为时，
   贡献者可以覆盖此值。

 * nnn：其他未归入上述类别的杂项变量，
   或因历史原因使用其他名称、尚未完成转换的变量。

维护者控制的作业变量
--------------------

在 CI 配置文件中定义作业时，可以设置以下变量。

QEMU_JOB_OPTIONAL
~~~~~~~~~~~~~~~~~

作业通常应能成功，但为节约有限 CI 资源，默认不运行。
贡献者可在流水线界面中手动启动。

QEMU_JOB_ONLY_FORKS
~~~~~~~~~~~~~~~~~~~

作业结果仅对提交代码前的贡献者有用，不是合并准入流水线的必需部分。

QEMU_JOB_SKIPPED
~~~~~~~~~~~~~~~~

作业通常不能可靠成功，因此目前不适合默认运行。
理想情况下这只是临时标记，直到修复问题或永久移除作业。

QEMU_JOB_PUBLISH
~~~~~~~~~~~~~~~~

作业用于在分支合入上游默认分支后发布内容。

QEMU_JOB_FUNCTIONAL
~~~~~~~~~~~~~~~~~~~

作业运行功能测试套件。

QEMU_JOB_MACOS
~~~~~~~~~~~~~~

作业需要访问 macOS 共享运行器，仅 Premium/Ultimate 等级可用。
这包括开源项目成员，但不包括 fork 仓库。

贡献者控制的运行时变量
----------------------

贡献者可设置以下变量控制作业执行。

QEMU_CI
~~~~~~~~

为节省 CI 额度，贡献者 fork 默认不创建流水线。

设为 1 时创建流水线，但全部作业需要在界面手动启动。

设为 2 时创建流水线并立即运行全部作业，恢复以往的行为。

QEMU_CI_FUNCTIONAL
~~~~~~~~~~~~~~~~~~
默认不自动运行使用功能测试框架的测试，
因为需要下载多个产物，可能造成大量网络流量。
设置此变量后，这些测试会自动运行。

其他杂项变量
------------

这些变量主要控制私有运行器上的作业。

AARCH64_RUNNER_AVAILABLE
~~~~~~~~~~~~~~~~~~~~~~~~
如果可使用某个 aarch64 主机作为 GitLab CI 运行器，
设置此变量可启用需要此类主机的测试。运行器应带 aarch64 标签。

S390X_RUNNER_AVAILABLE
~~~~~~~~~~~~~~~~~~~~~~
如果可使用某个 IBM Z 主机作为 GitLab CI 运行器，
设置此变量可启用需要此类主机的测试。运行器应带 s390x 标签。

PPC64LE_RUNNER_AVAILABLE
~~~~~~~~~~~~~~~~~~~~~~~~
如果可使用某个 PPC64LE 主机作为 GitLab CI 运行器，
设置此变量可启用需要此类主机的测试。运行器应带 ppc64le 标签。

CCACHE_DISABLE
~~~~~~~~~~~~~~
作业默认使用 ccache，通常能缩短编译时间，但会增加存储占用。
若怀疑它反而增加整体执行时间，可设置 CCACHE_DISABLE=1 禁用。
