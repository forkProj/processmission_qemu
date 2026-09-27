QEMU Python 工具
================

此包提供 QEMU 项目用于构建、配置和测试的工具，不是完整 SDK，接口可能随时变化。

用法
--------

``qemu.qmp`` 提供与 QMP 服务器通信的库。
``qemu.machine`` 提供启动和管理 QEMU 进程的基本功能。
更多信息请查阅各包文档：``>>> help(qemu.qmp)``、``>>> help(qemu.machine)``。

参与贡献
--------

此包作为 QEMU 源码树的一部分，由 John Snow <jsnow@redhat.com> 维护。
欢迎贡献，流程遵循 `QEMU 补丁提交流程
<https://wiki.qemu.org/Contribute/SubmitAPatch>`_，向开发邮件列表发送补丁。

John 维护 `GitLab 暂存分支 <https://gitlab.com/jsnow/qemu/-/tree/python>`_，
另有 `官方 GitLab 镜像 <https://gitlab.com/qemu-project/qemu>`_。

请在 `QEMU 问题跟踪器 <https://gitlab.com/qemu-project/qemu/-/issues>`_
报告缺陷，并在报告中标记 ``@jsnow``。

代码质量分析所需的可选依赖可通过 devel 依赖组安装：``pip install qemu[devel]``。

``make develop`` 可用一条命令将包以可编辑模式安装到当前环境，并安装测试依赖。

``make check`` 运行现有测试。
