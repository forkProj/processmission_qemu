=========
QEMU 说明
=========

QEMU 是通用、开源的机器和用户空间模拟器，也是虚拟化工具。

QEMU 可以完全通过软件模拟整台机器，无需硬件虚拟化支持，
并通过动态翻译获得良好性能。
它也能与 Xen、KVM 配合，由 QEMU 提供模拟硬件、虚拟机监控器管理 CPU，
使 CPU 性能接近原生。
直接模拟 CPU 时，可以在不同机器上运行操作系统，
例如在 x86_64 PC 上运行面向 ARMv7 开发板的系统。

QEMU 还提供 Linux 和 BSD 内核接口的用户空间 API 虚拟化，
让针对一种架构 ABI（如 Linux PPC64）编译的程序，
运行在采用另一种 ABI（如 Linux x86_64）的宿主上。
这只涉及 CPU 和系统调用模拟，不涉及硬件设备模拟。

QEMU 面向多种用途。需要完整控制行为和配置的用户可以直接调用，
上层管理工具也可以通过稳定的命令行和监视器 API 集成它。
oVirt、OpenStack 和 virt-manager 等开源应用通常通过 libvirt 间接调用 QEMU。

QEMU 整体采用 GNU 通用公共许可证第 2 版，完整许可信息见 LICENSE。

文档
========

在线文档位于 `<https://www.qemu.org/documentation/>`_。
当前开发版文档位于 `<https://www.qemu.org/docs/master/>`_，
由源码树的 ``docs/`` 使用 `Sphinx <https://www.sphinx-doc.org/en/master/>`_ 构建。

构建
========

QEMU 是跨平台软件，旨在支持现代 Linux、OS-X、通过 Mingw64 的 Win32，
以及多种其他 UNIX 平台。基本构建步骤如下：

.. code-block:: shell

  mkdir build
  cd build
  ../configure
  make


其他信息见 QEMU 网站：

* `<https://wiki.qemu.org/Hosts/Linux>`_
* `<https://wiki.qemu.org/Hosts/Mac>`_
* `<https://wiki.qemu.org/Hosts/W32>`_

提交补丁
========

QEMU 源码使用 Git 维护：

.. code-block:: shell

   git clone https://gitlab.com/qemu-project/qemu.git


常见提交方式是使用 git format-patch 和 git send-email，
将格式化后的补丁发往 qemu-devel@nongnu.org。
所有补丁都必须包含作者的 Signed-off-by，
并遵循开发者指南的 `风格规范 <https://www.qemu.org/docs/master/devel/style.html>`_。

更多信息见：

* `<https://wiki.qemu.org/Contribute/SubmitAPatch>`_
* `<https://wiki.qemu.org/Contribute/TrivialPatches>`_

QEMU 网站也通过版本控制维护：

.. code-block:: shell

  git clone https://gitlab.com/qemu-project/qemu-web.git


* `<https://www.qemu.org/2017/02/04/the-new-qemu-website-is-up/>`_

推荐经常贡献，或需要反复发送补丁系列修订版的用户使用 git-publish，简化上述流程。
它仍需要可用的 git send-email 配置，而且默认不会自动完成所有步骤，
建议先手动走一遍流程。

安装说明见：

* `<https://github.com/stefanha/git-publish>`_

使用流程如下：

.. code-block:: shell

  $ git checkout master -b my-feature
  $ # work on new commits, add your 'Signed-off-by' lines to each
  $ git publish


补丁系列会被发送，并标记为 my-feature-v1，方便以后引用。

发送第 2 版：

.. code-block:: shell

  $ git checkout my-feature # same topic branch
  $ # making changes to the commits (using 'git rebase', for example)
  $ git publish


邮件主题会包含 v2，分支顶端会标记为 my-feature-v2。

报告缺陷
========

QEMU 使用 GitLab issues 跟踪缺陷。
从 QEMU Git 或上游发布源码构建时发现的问题，请报告到：

* `<https://gitlab.com/qemu-project/qemu/-/issues>`_

如果使用操作系统供应商提供的二进制包，优先向供应商的问题跟踪器报告。
确认最新上游代码也受影响时，也可以提交到 GitLab。

更多信息见：

* `<https://wiki.qemu.org/Contribute/ReportABug>`_

变更记录
========

版本历史及发布说明见 `<https://wiki.qemu.org/ChangeLog/>`_，
更详细的信息可以查阅 Git 历史。

联系
========

联系社区的主要方式是电子邮件和 IRC：

* `<mailto:qemu-devel@nongnu.org>`_
* `<https://lists.nongnu.org/mailman/listinfo/qemu-devel>`_
* irc.oftc.net 上的 #qemu

其他联系方式见：

* `<https://wiki.qemu.org/Contribute/StartHere>`_
