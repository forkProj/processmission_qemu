QEMU Python 工具
================

此目录包含 QEMU 项目用于构建、配置和测试的 Python 工具。
先按命名空间 ``qemu``，再按包组织，例如 ``qemu/machine``、``qemu/utils``。

这些工具和库默认安装到 configure 阶段创建的 Python 虚拟环境中，
见 qemu.git/pythondeps.toml 的 tooling 组，构建系统执行的 Python 脚本都可以使用。
手动运行脚本时，建议使用构建目录中的 run 脚本，以便访问这些库。

总体结构
--------

pip 通过 ``setup.py`` 将工具安装到当前环境，``setup.cfg`` 提供打包配置。
通常使用以下方式之一：

1. ``pip3 install .`` 安装到当前环境。
   虚拟环境内会安装到该环境；否则尝试全局安装，**不推荐** 这样做。

2. ``pip3 install --user .`` 安装到用户本地 Python 包目录。
   虚拟环境内此操作会失败，应使用前一种方式。

加入 ``--editable`` 或 ``-e`` 后采用可编辑模式，安装一个指向源码树的转发入口，
因此安装后的包始终反映源码的最新版本。configure 阶段采用此模式。

使用 .[devel] 替代 .，还会安装测试所需软件包。
它们不是运行时依赖，单纯使用这些库不需要安装。

``make develop`` 会安装全部测试依赖，并将 QEMU 以可编辑模式安装到当前环境，
是 ``pip3 install -e .[devel]`` 的快捷方式。

更多说明见 `使用 pip 和虚拟环境安装软件包
<https://packaging.python.org/guides/installing-using-pip-and-virtual-environments/>`_。

不安装就使用这些包
------------------

不再推荐绕过虚拟环境安装直接使用，但某些场景下仍可能可行。

qemu.qmp 现在托管于 qemu.git 之外，树内的 qemu.machine 依赖它。
可以单独安装 qemu.qmp，再直接使用其他包；
但未来新增依赖时，跳过安装可能导致脚本失效。

如仍需这样做，可选择：

1. 将源码目录加入 PYTHONPATH，例如 ``~/src/qemu/python``。
   参见 https://docs.python.org/3/using/cmdline.html#envvar-PYTHONPATH 。

2. 在 Python 脚本中，导入 qemu 命名空间前通过 ``sys.path`` 添加搜索路径。
   参见 https://docs.python.org/3/library/sys.html#sys.path 。

两种方式的明显缺点是通常会妨碍静态分析工具定位和分析导入的代码。

安装包通常还会提供可执行控制台脚本，使 qmp-shell 等可以通过 $PATH 访问。
不安装时可以这样调用：

``> PYTHONPATH=~/src/qemu/python python3 -m qemu.qmp.qmp_shell``

**强烈建议直接使用 configure 阶段创建的虚拟环境。**
完成 configure 后，调用构建目录中的 run 脚本即可：

``> $builddir/run qmp-shell``

脚本名与 Python 模块路径的映射见 ``setup.cfg``。
可用控制台脚本如下：

* ``qemu-ga-client``
* ``qmp-shell``
* ``qmp-shell-wrap``
* ``qmp-tui``：基于 urwid 的异步 QMP 原型界面。
* ``qom``
* ``qom-fuse``：需要安装 fusepy。
* ``qom-get``
* ``qom-list``
* ``qom-set``
* ``qom-tree``

目录中的文件
------------

- ``qemu/``：Python qemu 命名空间包源码。
- ``tests/``：包测试目录。
- ``Makefile``：常见测试和安装命令，使用 ``make help`` 查看目标。
- ``MANIFEST.in``：供 setuptools 读取，指定源码分发包需要额外包含的文件。
- ``PACKAGE.rst``：在 PyPI.org 显示的 README。
- ``README.rst``：当前文档。
- ``VERSION``：符合 PEP 440 的包版本，由 ``setup.cfg`` 引用。
- ``setup.cfg``：setuptools 打包配置。
- ``setup.py``：pip 使用的 setuptools 安装入口，见前文。
