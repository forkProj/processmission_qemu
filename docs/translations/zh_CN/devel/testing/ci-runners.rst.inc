自定义运行器上的作业
====================

除了前述各种 CI 系统的作业，实际合并前还有一些额外作业。
它们使用相同的 GitLab CI 服务和框架，但依赖额外机器，
而非 GitLab 提供的共享运行器。

GitLab CI 架构允许在不同机器上部署称为 gitlab-runner 的代理，
负责执行推送分支等事件创建的作业。
这里把机器及其正确配置的 gitlab-runner 合称为“自定义运行器”。

相关作业定义位于::

  .gitlab-ci.d/custom-runners.yml

自定义运行器需要自行提供机器。
当前 QEMU GitLab CI 部署的机器及维护者名单见
`QEMU Wiki <https://wiki.qemu.org/AdminContacts>`__。

机器设置方法
------------

Linux 系统的大部分设置可以通过两个 Ansible playbook 自动完成。
在 scripts/ci/setup 下创建 inventory 文件，例如::

  fully.qualified.domain
  other.machine.hostname

可能需要直接在清单文件中设置变量。
常见需求是要求 Ansible 在这些主机上使用 Python 3，例如::

  fully.qualified.domain ansible_python_interpreter=/usr/bin/python3
  other.machine.hostname ansible_python_interpreter=/usr/bin/python3

构建环境
~~~~~~~~

``scripts/ci/setup/$DISTRO/build-environment.yml``
为机器配置构建和运行 QEMU 测试所需的环境，
安装各种必需软件包，并顺带执行总体软件包更新。

已经成功测试的最低 Ansible 版本为 2.8.0，playbook 内也包含版本检查。
运行方式::

  cd scripts/ci/setup
  ansible-playbook -i inventory $DISTRO/build-environment.yml

多数任务需要超级用户权限，例如 root 身份或通过 sudo 获取权限。
必要时请参阅 ansible-playbook 的 --become、--become-method、
--become-user 和 --ask-become-pass 等选项。

gitlab-runner 设置和注册
~~~~~~~~~~~~~~~~~~~~~~~~

每台运行作业的机器都需要安装 gitlab-runner。
机器通过注册令牌与 GitLab 项目关联。
在 GitLab 网页界面按以下路径查找仓库或项目的注册令牌：

 * Settings，左侧竖向工具栏底部的齿轮图标；
 * CI/CD；
 * Runners，点击 Expand；
 * 在 Set up a specific Runner manually 下查找 And this registration token 的值。

将 scripts/ci/setup/vars.yml.template 复制为 scripts/ci/setup/vars.yml，
再将 gitlab_runner_registration_token 设置为此前获取的值。

运行 playbook::

  cd scripts/ci/setup
  ansible-playbook -i inventory gitlab-runner.yml

注册后，需要在 GitLab 界面配置运行器标签，也可调整其他设置。路径为：

 * Settings，齿轮图标；
 * CI/CD；
 * Runners，点击 Expand；
 * Runners activated for this project；
 * 点击锁形图标旁的编辑图标。

标签用于将特定作业分配给相应类型的运行器，非常重要。
应复核自动创建的标签是否与操作系统和架构一致。
例如 Ubuntu 20.04 aarch64 应设置为::

  ubuntu_20.04,aarch64

因为 .gitlab-ci.d/custom-runners.yml 中的作业定义会包含::

  ubuntu-20.04-aarch64-all:
   tags:
   - ubuntu_20.04
   - aarch64

另外建议：

 * 将 Maximum job timeout 增加到例如 ``2h``；
 * 填写更清楚的 Description。
