.. _submitting-a-pull-request:

提交拉取请求
============

QEMU 欢迎代码贡献，但通常要求以普通补丁邮件发送到邮件列表，详情见
:ref:`submitting-a-patch`。一般只有已有子树维护者需要提交拉取请求，
不过遇到大型补丁序列，也可能要求提交者提供拉取请求。
本页为这些人员介绍建议做法。

一条实用原则是：除非有人要求，否则不要发送拉取请求。

**随拉取请求重新发送补丁**，将补丁邮件作为拉取请求邮件的线程回复。
最简单的方法是用 ``git format-patch --cover-letter`` 生成邮件，
再编辑封面信，加入 ``git request-pull`` 输出的请求信息。

**在标题标签中使用 PULL**，封面信和重发补丁邮件都应如此，
例如给 git format-patch 添加 ``--subject-prefix=PULL``。
这样便于收件人筛选，尤其只抄送整组中的一封邮件时。

**每个补丁必须有你自己的 Signed-off-by: 行**。
如果补丁不是你写的，也必须保留原作者的行。
拉取请求意味着补丁经过你转交，而非直接来自原作者。

**不要忘记添加 Reviewed-by: 和 Acked-by: 行**。
确保复制其他人对请求中补丁给出的审查标签。
通过 `patches 工具 <https://github.com/stefanha/patches>`__
从邮件导入 Git 时会自动包含标签；手动或通过其他方式更新时，
需要自行编辑提交说明。

**不要为未通过审查的代码发送拉取请求**。
拉取请求表示补丁已准备好进入 QEMU，因此必须通过标准代码审查。
尤其在一轮审查后修正了问题，必须按正常流程重发修改后的序列，
通过审查后才能放入拉取请求。
极其简单的修正可能可以顺手处理，但拿不准时不要省略审查。

**发送前进行测试**。
确保所有内容都能构建，包括补丁序列的每一步，并通过 make check。
子树维护者是阻止有问题代码进入 QEMU 的一道防线，应仔细复核细节。

**所有拉取请求必须签名**。
这里的签名指邮件引用的标签必须经过 GPG 签名，
例如通过 'gpg tag -s ...' 创建。
详情见 :ref:`maintainer_keys`。

**非 master 的拉取请求应写明“not for master”，标题标签为“PULL SUBSYSTEM whatever”**。
如果目标是稳定分支或其他子维护者的树，封面信应包含“not for master”，
标题标签使用“PULL SUBSYSTEM s390/block/whatever”，而不是单独的“PULL”。
这样可以从应合入 master 的请求中自动过滤掉它们。

`make-pullreq <https://gitlab.com/pm215/misc-scripts/-/blob/master/make-pullreq>`__
脚本可自动执行部分流程，并提供一些基本检查。
使用前必须编辑脚本，使配置符合本地情况。
