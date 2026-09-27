.. _submitting-a-patch:

提交补丁
========

QEMU 欢迎修复缺陷、添加功能或改进文档的贡献。不过，我们收到的补丁很多，因此制定了一些提交指南。遵循这些指南，可以减轻贡献审阅工作，也能让你的修改更快获得接受并提交。

本页看起来很长。如果你只是想快速提交一次性的修复，我们至少要求：

.. list-table:: 补丁最低检查清单
   :widths: 35 65
   :header-rows: 1

   * - 检查项
     - 原因
   * - 补丁包含 ``Signed-off-by: Your Name <author@email>``
     - 声明你在法律上有权贡献代码。参见 :ref:`patch_emails_must_include_a_signed_off_by_line`
   * - 以补丁邮件发送至 ``qemu-devel@nongnu.org``
     - 项目采用基于邮件列表的工作流程。参见 :ref:`submitting_your_patches`
   * - 准备好回应审阅意见
     - 未通过审阅的代码不会合并。参见 :ref:`participating_in_code_review`

发帖不要求订阅列表（列表要求回复所有人，保留抄送，使未订阅的人也能参与自己发起的讨论），不过订阅后可能更容易从其他邮件中汲取好想法。如果订阅，请做好接收大量邮件的准备，通常每周超过一千封。列表实行审核；某个邮件地址第一次发帖时，无论是否订阅，都可能需要等待审核员批准该地址，因此会有延迟。

贡献越大，或越打算长期参与，本页其余内容就越重要。阅读下面的目录即可了解基本要求。可以把目录当作索引，阅读自己有疑问的部分。

.. contents:: 目录

.. _writing_your_patches:

编写补丁
--------

.. _use_the_qemu_coding_style:

采用 QEMU 编码风格
~~~~~~~~~~~~~~~~~~

提交前可以运行 *scripts/checkpatch.pl <patchfile>*，检查是否符合编码规范。不过，``checkpatch.pl`` 并非万无一失，特别是在涉及 C 预处理器宏时；还需要自行判断。另请参阅：

-  :ref:`coding-style`
-  `在提交时自动运行 checkpatch <https://blog.vmsplice.net/2011/03/how-to-automatically-run-checkpatchpl.html>`__

.. _base_patches_against_current_git_master:

基于当前 git master 编写补丁
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

提交基于 QEMU 已发布版本的补丁没有意义，因为开发已经继续推进，这种补丁甚至可能无法应用到 master。我们只把选定的缺陷修复应用到发布分支，而且必须先进入 master，再进行回移。

补丁也可以基于尚未进入 git master 分支的其他在研工作。为了帮助 `patchew <http://patchew.org/QEMU/>`__ 等持续集成工具，应在说明信中 `添加标签 <https://lists.gnu.org/archive/html/qemu-devel/2017-08/msg01288.html>`__ 行 ``Based-on: $MESSAGE_ID``，明确指出系列的依赖关系。

.. _split_up_long_patches:

拆分长补丁
~~~~~~~~~~

把较长的补丁拆成由逻辑代码修改组成的补丁系列。每项修改都应能成功编译和运行。例如，不要在第一个补丁中把文件加入 makefile，到第二个补丁才添加文件本身。（这条规则使人们日后能使用 `git bisect <http://git-scm.com/docs/git-bisect>`__ 等工具，而不会在提交历史中遇到因无关原因导致 QEMU 无法工作的节点。）文档应放在前面，而非最后，这样读者可以先独立评估文档，再核实代码是否符合文档。提交说明中出现“另外……”时，往往意味着适合拆成多个补丁。有关合理拆分补丁和编写良好提交说明的更多建议，请参阅 `OpenStack 的建议 <https://wiki.openstack.org/wiki/GitCommitMessages>`__。

.. _make_code_motion_patches_easy_to_review:

让代码移动补丁便于审阅
~~~~~~~~~~~~~~~~~~~~~~

如果系列需要移动大块代码，可以运用一些技巧让重构更容易审阅。拆分系列，把语义修改（甚至函数重命名）与纯代码移动放在不同补丁中。执行一次 ``git config diff.renames true;`` 和 ``git config diff.algorithm patience`` 即可完成配置（参见 `git-config <http://git-scm.com/docs/git-config>`__）。'diff.renames' 属性使文件重命名补丁以更紧凑的形式表示，只关注重命名前后的差异，而不是把整个旧文件显示为删除、新文件显示为插入。同时，'diff.algorithm' 属性适用于从一个文件抽取不连续部分到新文件的情况：只要抽取部分在补丁前后保持相同顺序，就能减少把原文件中无关的 ``}`` 行当作修改块分界所造成的差异噪声。

理想情况下，可执行以下命令审阅代码移动补丁::

    git format-patch --stdout -1 > patch;
    diff -u <(sed -n 's/^-//p' patch) <(sed -n 's/^\+//p' patch)


从而专注于少数不属于整块代码移动的修改。

.. _dont_include_irrelevant_changes:

不要包含无关修改
~~~~~~~~~~~~~~~~

尤其不要对补丁原本不涉及的代码进行格式、编码风格或空白修改。（可以修复所修改行附近几行的风格问题。）如果认为某段代码确实需要重新缩进或大规模风格修正，请提交一个不改变语义的独立补丁，不要将其与缺陷修复放在同一补丁中。

对于 QEMU 中修改频率较低区域的小补丁，可以考虑采用 :ref:`trivial-patches` 流程。

.. _write_a_meaningful_commit_message:

编写有意义的提交说明
~~~~~~~~~~~~~~~~~~~~

提交说明应具有实际意义，能够独立充当历史记录，解释修改为何必要或有用。

QEMU 遵循常见的 git 提交说明规范：第一行（会成为邮件主题）采用“子系统: 单行修改摘要”的形式。摘要是否以大写字母开头可自行决定，但我们希望末尾不要加句号。可以查看 ``git shortlog -30``，了解主题行示例。随后是空行、更详细的补丁描述、另一个空行以及 Signed-off-by: 行。提交说明每行不要超过 76 个字符，以便在 80 列终端窗口中使用 "git show" 时仍能良好显示。

提交说明正文适合解释修改的重要性。不要加入“这是修复此缺陷的一个建议”之类的评论（可放到邮件的 ``---`` 行下面，使其不进入最终提交说明）。确保正文能独立阅读，即使读者的邮件客户端把主题显示在较远的位置也没有影响（例如，以“……以便”开头、接续主题的正文就比较难理解）。

如果补丁修复仓库中已有的某次提交，请在提交说明的补丁描述之后、"Signed-off-by:" 行之前，添加一行 ``Fixes: <at-least-12-digits-of-SHA-commit-id> ("Fixed commit subject")``，其中提交 ID 至少包含 12 位 SHA，括号内为被修复提交的主题。

如果补丁修复 gitlab 缺陷跟踪器中的问题，还请添加 ``Resolves: <URL-of-the-bug>`` 行。当带有 "Resolves:" 关键字的提交合入项目 master 分支时，Gitlab 可以自动关闭问题。如果补丁解决其他公共缺陷跟踪器中的问题，也可以使用 ``Buglink: <URL-of-the-bug>`` 行提供引用。

示例::

 Fixes: 14055ce53c2d ("s390x/tcg: avoid overflows in time2tod/tod2time")
 Resolves: https://gitlab.com/qemu-project/qemu/-/issues/42
 Buglink: https://bugs.launchpad.net/qemu/+bug/1804323``


提交说明中还会使用 "Message-Id:"、"Tested-by:"、"Acked-by:"、"Reported-by:"、"Suggested-by:" 等标签。在 ``git log`` 中搜索这些关键字可查看用法示例。

.. _test_your_patches:

测试补丁
~~~~~~~~

虽然 QEMU 使用多种 :ref:`ci` 服务尝试测试发送到列表的补丁，但提前确认补丁能够编译和正常运行仍能节省所有人的时间。由于 QEMU 项目很大，默认配置不会在推送分支时于 GitLab 创建测试流水线。有关如何控制测试运行，参见 :ref:`CI 变量文档<ci_var>`；不过，提交系列前最好仍通过完整构建确认补丁有效，尤其是修改可能意外影响你平时不测试的其他代码区域时。有关可用测试的详情，请参见 :ref:`testing`。

此外，最好在补丁中增加测试套件内容，防止后续修改破坏新功能，或添加能暴露系列其余部分所修复缺陷的测试。把测试和修复放在不同提交中，审阅者就能通过变基先把测试放在前面，证明它确实能发现问题，然后再放到系列末尾，避免二分查找落到已知有问题的状态。

.. _submitting_your_patches:

提交补丁
--------

QEMU 项目通过公开邮件流程审阅和合并补丁。因此，所有 QEMU 贡献都必须 **以补丁形式发送** 到 qemu-devel `邮件列表 <https://wiki.qemu.org/Contribute/MailingLists>`__。不要把补丁贡献贴到缺陷跟踪器、论坛，或托管在外部后仅提供链接。（我们也有其他邮件列表，但所有补丁必须发到 qemu-devel，可以同时抄送其他列表。） ``git send-email`` （`逐步配置指南 <https://git-send-email.io/>`__ 和 `提示与技巧 <https://elixir.bootlin.com/linux/latest/source/Documentation/process/email-clients.rst>`__）最适合避免补丁在发送时被破坏；第一次投稿实在没有其他办法时，也可以使用附件。

.. _use_b4:

使用 B4
~~~~~~~~

Linux 内核开发使用的 `b4`_ 工具也可用于 QEMU 开发。大多数发行版和 PyPi 都提供此工具。QEMU 源码树根目录包含 ``b4`` 项目配置文件 ``.b4-config``。

准备补丁系列的工作流程示例：

1. 从干净的 ``master`` 分支检出开始。
2. 使用 ``b4 prep -n descriptive-name`` 创建具有主题分支名称的新系列。
   ``b4`` 会创建 ``b4/descriptive-name`` 分支并切换到该分支。
3. 提交修改，并遵循本页关于提交说明等方面的指南。
4. 使用 ``b4 prep --edit-cover`` 编写说明清晰的说明信。
5. 使用 ``b4 prep --auto-to-cc`` 添加维护者和审阅者抄送。可以编辑说明信来修改 Cc: 和 To: 收件人。
6. 使用 ``b4 prep --check`` 检查补丁。
7. 可使用 ``b4 send --dry-run`` 审阅补丁，该命令会把原始补丁打印到标准输出。

发送补丁时，可以：

- 配置 ``git-send-email`` 并使用 ``b4 send``，或
- 使用 ``b4 send -o OUTPUT_DIR`` 把补丁导出为文件，再手动发送。

详情请参阅 `b4 文档 <b4 documentation_>`_。

.. _b4 documentation: https://b4.docs.kernel.org/
.. _b4: https://github.com/mricon/b4/

.. _use_git_publish:

使用 git-publish
~~~~~~~~~~~~~~~~

如果已经配置 git send-email，可以直接使用 `git-publish <https://github.com/stefanha/git-publish>`__ 发送系列。

::

    $ git checkout master -b my-feature
    $ # work on new commits, add your 'Signed-off-by' lines to each
    $ git publish
    $ ... more work, rebase on master, ...
    $ git publish # will send a v2


每次发送系列时，git-publish 都会创建格式为 ``<branchname>-v<version>`` 的本地标签，记录该补丁系列。

发送补丁邮件时，'git publish' 会读取 'scripts/get_maintainers.pl' 的输出，并自动抄送其中列出的受影响代码维护者。通常应接受建议的抄送列表，但有时适合缩减范围（例如某些大规模全树清理），或加入其他感兴趣的人。

.. _if_you_cannot_send_patch_emails:

无法发送补丁邮件时
~~~~~~~~~~~~~~~~~~

极少数情况下，可能无法发送格式正确的补丁邮件。可以按照以下步骤使用 `sourcehut <https://sourcehut.org/>`__ 向 QEMU 邮件列表发送补丁：

#. 注册或登录账户。
#. 在 `meta \| keys <https://meta.sr.ht/keys>`__ 中添加 SSH 公钥。
#. 使用 **git push git@git.sr.ht:~USERNAME/qemu HEAD** 发布 git 分支。
#. 在 https://git.sr.ht/~USERNAME/qemu/send-email 使用基于网页的 ``git-send-email`` 界面向 QEMU 邮件列表发送补丁。

sourcehut 文档可在 `这里 <https://man.sr.ht/git.sr.ht/#sending-patches-upstream>`__ 找到。

.. _cc_the_relevant_maintainer:

抄送相关维护者
~~~~~~~~~~~~~~

把补丁发送到邮件列表，并抄送所修改文件的维护者。查看 MAINTAINERS 文件以确定具体人员。也可以尝试仓库中的 scripts/get_maintainer.pl，了解所修改文件最常见的提交者。

示例::

    ~/src/qemu/scripts/get_maintainer.pl -f hw/ide/core.c


实际上，只需执行一次 ``git config sendemail.cccmd 'scripts/get_maintainer.pl --nogit-fallback'`` 即可自动完成此操作（参见 `git-config <http://git-scm.com/docs/git-config>`__）。

.. _do_not_send_as_an_attachment:

不要使用附件发送
~~~~~~~~~~~~~~~~

请以内联形式发送补丁，便于回复并提出审阅意见。不要把补丁放在附件中。

.. _use_git_format_patch:

使用 ``git format-patch``
~~~~~~~~~~~~~~~~~~~~~~~~~

使用正确的 diff 格式。`git format-patch <http://git-scm.com/docs/git-format-patch>`__ 会生成格式正确的补丁邮件（使用方法见文档）。随后可以编辑说明信，再使用 ``git send-email`` 把文件发到邮件列表。（推荐 `git send-email <http://git-scm.com/docs/git-send-email>`__，因为邮件客户端经常通过折行或修改空白破坏补丁。一些发行版默认安装 git 时不包含 send-email，可能需要安装附加包，例如 Fedora 系统中的 'git-email'。）补丁系列需要说明信，并采用浅层线程结构：系列中的所有补丁都回复说明信，而不是相互回复。单个独立补丁不需要说明信；如果发送说明信，请使用 ``--numbered``，让说明信与补丁具有不同的主题。补丁最好发起新的顶层线程，这样更容易找到，而不会埋在其他已有线程的回复中。

.. _avoid_posting_large_binary_blob:

避免发送大型二进制数据
~~~~~~~~~~~~~~~~~~~~~~

如果向仓库添加了二进制文件，可以考虑使用 ``git format-patch --no-binary`` 生成补丁邮件，并提供 git 仓库链接，供读者获取原始提交。

.. _patch_emails_must_include_a_signed_off_by_line:

补丁邮件必须包含 ``Signed-off-by:`` 行
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

补丁 **必须** 包含 Signed-off-by: 行。这是硬性要求，因为它表示“我在法律上有权贡献这些内容，并同意其进入 QEMU”。完整指南见 :ref:`code-provenance` 文档。


.. _include_a_meaningful_cover_letter:

提供有意义的说明信
~~~~~~~~~~~~~~~~~~

包含多个补丁的系列必须提供说明信，以帮助持续集成；单个独立补丁则可选。说明信解释系列的整体目标，也提供方便的 0/N 邮件，供他人针对整个系列回复。执行一次 ``git config format.coverletter auto`` （参见 `git-config <http://git-scm.com/docs/git-config>`__），即可在需要时生成说明信。

如果审阅者在开始时不知道目标，可能会反对某些只有读到系列末尾才显得合理的早期修改，因为当时缺乏足够上下文。目标不明确还可能增加审阅与修正的轮次，因为审阅者尚未认同思路。如果说明信能解释这些问题，流程会更顺畅，补丁也会更快合并。确保说明信包含整个系列的 diffstat；潜在审阅者知道自己关心哪些文件，需要能方便地判断系列是否涉及这些文件。

.. _use_the_rfc_tag_if_needed:

需要时使用 RFC 标签
~~~~~~~~~~~~~~~~~~~

例如 "[PATCH RFC v2]"。``git format-patch --subject-prefix=RFC`` 可以帮助生成此标签。

"RFC" 意为“征求意见”（Request For Comments），表示你暂不打算把补丁集应用到 master，但仍希望获得审阅。原因包括：

-  补丁依赖尚未接受的内核修改，因此 QEMU 补丁系列必须等待依赖解决，但仍值得审阅。
-  补丁集尚未完成（可能未覆盖全部用例或不支持全部目标），但希望在继续之前尽早审阅重大 API 修改或设计结构。

通常，这相当于请他人审阅一个连提交者也认为不应应用的补丁集，因此最好：

-  谨慎使用。
-  在说明信中明确解释为何是 RFC、希望审阅哪些部分，以及这些内容为何值得审阅者关注。

.. _consider_whether_your_patch_is_applicable_for_stable:

考虑补丁是否适用于稳定版
~~~~~~~~~~~~~~~~~~~~~~~~

如果补丁修复严重问题或回归，可能适用于稳定版。此时可以在补丁中添加 ``Cc: qemu-stable@nongnu.org``，通知稳定版维护者。

有关 QEMU 稳定版流程的详情，参见 :ref:`stable-process`。

.. _participating_in_code_review:

获取已有系列
------------

如果想把已有系列应用到自己的源码树，可以直接使用 `b4`_。

::

    b4 shazam $msg-id


消息 ID 对应已发送到邮件列表的补丁系列。需要从其中任意一个补丁中获取 "Message-Id:" 头字段；使用任意补丁的 ID，b4 都会应用整个系列。

参与代码审阅
------------

所有提交到 QEMU 项目的补丁在被接受前都要经过代码审阅。因此，系列往往需要多轮迭代，才会由 :ref:`维护者<maintainers>` 接收。请准备好阅读邮件回复，并愿意据此采取行动。

由于提交理想的补丁需要学习，维护者通常愿意手动修正首次贡献。不过，要取得最佳效果，应主动回应建议，修改代码或解释当前做法的理由。

维护良好的代码区域可能很快完成审阅，关注较少的区域则可能等待更久。

.. _stay_around_to_fix_problems_raised_in_code_review:

持续参与并修复审阅发现的问题
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

很少有补丁能立即进入 QEMU。开发者经常会发现缺陷、建议更清晰的方案，或指出编码风格问题、提交说明中的拼写错误。你需要回应这些意见，并发送修复问题后的第二版补丁。这需要投入一些时间和精力，但如果不做，修改就无法进入 QEMU。

请记住，维护者没有义务接受补丁。如果有人花时间审阅代码并建议改进，而你既不处理意见，也不给出进一步理由，只是重新发送补丁，那么这些审阅工作就白费了。不能要求他人先合并，再替你修正代码。

回复补丁意见时，**请回复所有人，不要只回复发件人**。让讨论保留在邮件列表中，所有人才能跟进。遵循 :ref:`code_of_conduct` 的精神，保持尊重与合作，避免针对个人发表评论。

.. _pay_attention_to_review_comments:

认真对待审阅意见
~~~~~~~~~~~~~~~~

有人花时间审阅你的工作，应尊重这份付出。反复提交系列而不处理上一轮的全部意见，容易疏远审阅者，导致补丁停滞。审阅者也并非总是正确，因此可以说明原代码为何正确，不必盲目照做。另一方面，如果审阅时有人指出潜在问题，即使代码最终证明没有问题，也往往说明应该改进提交说明或代码注释，解释代码为何正确。

修复审阅提出的问题后，**重新发送整个补丁系列**，不要只发送改动过的单个补丁。这样维护者就能方便地应用修正后的系列，无需手动辨别哪些补丁有效。把新版作为全新的邮件或邮件系列发送，不要作为第一版的后续回复。（这有助于补丁邮件自动处理工具区分 v1 和 v2。）

.. _when_resending_patches_add_a_version_tag:

重发补丁时添加版本标签
~~~~~~~~~~~~~~~~~~~~~~

第一版之后的所有补丁都应包含版本标签，例如 "[PATCH v2]"，以便读者判断是否为最新版。（第一版无需注明 "v1"，只用 [PATCH] 即可。）对于系列，版本号适用于整个系列；即使只修改一个补丁，也应重发整个系列并标记为 "v2"。不要分别跟踪系列中不同补丁的版本。`git format-patch <http://git-scm.com/docs/git-format-patch>`__ 和 `git send-email <http://git-scm.com/docs/git-send-email>`__ 都支持 ``-v2`` 选项，方便完成此操作。每个新版都应发起新的顶层线程，不要埋在旧版的回复中，因为许多审阅者不会深入旧线程寻找新补丁。

.. _include_version_history_in_patchset_revisions:

在补丁集新版中提供版本历史
~~~~~~~~~~~~~~~~~~~~~~~~~~

对于后续版本，应概述相较于旧版的修改，但不要写入提交说明本身。在 git 补丁邮件中，``---`` 行上方的提交说明会在提交时进入 git 日志。这一部分应独立描述当前版本的补丁做了什么，让半年后查看该提交的人也能理解。``---`` 行下方、实际补丁上方的区域（git format-patch 把 diffstat 放在这里）适合放给邮件读者看的备注，也正是“相较于上一版的修改”摘要应放的位置。`git-publish <https://github.com/stefanha/git-publish>`__ 脚本能帮助跨版本维护清晰摘要。`git-backport-diff <https://github.com/codyprime/git-scripts>`__ 脚本则能帮助审阅者专注于版本之间的差异。``b4`` 会自动在说明信中生成版本历史部分，并包含指向 `Lore`_ 上旧版本的链接。

.. _Lore: https://lore.kernel.org/

.. _tips_and_tricks:

提示与技巧
----------

.. _proper_use_of_reviewed_by_tags_can_aid_review:

正确使用 Reviewed-by: 标签有助于审阅
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

审阅大型系列时，审阅者可以回复某些补丁并附上 Reviewed-by 标签，表示认可该补丁本身（有时条件是进行不影响代码内容的轻微清理，例如修复空白）。此时应手动更新提交说明，加入 Reviewed-by 标签，以便下一版中审阅者能看出哪些补丁上一轮已通过。反过来，如果大幅修改已审阅的补丁，应从提交说明中移除 reviewed-by 标签，并列出相较于旧版的修改，使审阅者更容易关注新的变化。

.. _if_your_patch_seems_to_have_been_ignored:

补丁似乎被忽略时
~~~~~~~~~~~~~~~~

如果补丁集没有收到回复，应在一到两周后回复所有人，发送包含 "ping" 的提醒邮件，最好附上补丁在 `patchew <https://patchew.org/QEMU/>`__ 或 `lore.kernel.org <https://lore.kernel.org/qemu-devel/>`__ 上的页面链接。值得再次检查补丁可能被忽略的原因，例如忘记抄送维护者，或没有回应旧版审阅意见而惹人不快。不过，QEMU 中维护较少的区域确实经常会遗漏补丁。如果提醒也无人理会，可再过一周左右重新提醒。作为提交者，你最希望补丁得到应用，因此需要坚持跟进。

.. _is_my_patch_in:

补丁已经合入了吗？
~~~~~~~~~~~~~~~~~~

QEMU 有一些持续集成机器，会尽早尝试发现补丁提交中的问题。`patchew <http://patchew.org/QEMU/>`__ 提供网页界面，跟踪发到列表上的各个线程状态；如果发现补丁问题，可能会自动发邮件通知你。

补丁在列表上得到充分审阅后，该代码区域的维护者会通知列表，说明正在把补丁纳入某个暂存分支。随后维护者定期负责 :ref:`submitting-a-pull-request`，将主题分支汇入 QEMU 主线。通常，除非已经贡献足够多的补丁并成为某个代码区域的维护者，否则无需发送拉取请求。维护者可能进一步修改提交，解决简单合并冲突，或修正审阅中指出的轻微拼写错误，但总会在你的 Signed-off-by 行之外再添加自己的签署，表示提交经过其源码树。偶尔，维护者的拉取请求会遇到较复杂的合并冲突，此时可能请你协助变基和解决问题。从补丁首次获得肯定的审阅，到最终进入 qemu.git，可能需要几周；发布周期的冻结阶段还可能延长这一时间。

.. _return_the_favor:

回馈他人
~~~~~~~~

同行审阅需要所有人都投入一些时间才能运作。如果每个人提交的补丁都多于审阅的补丁，就会出现积压。一个合适的目标是：审阅他人的补丁数量至少与自己提交的数量相当。即使不像维护者那样熟悉代码库，也不必担心；坦言因不熟悉代码而审阅不够深入完全可以。
