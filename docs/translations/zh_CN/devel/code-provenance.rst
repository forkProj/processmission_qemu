.. _code-provenance:

代码来源
========

证明提交补丁的来源
~~~~~~~~~~~~~~~~~~

QEMU 社区要求所有贡献者证明提交补丁的来源，
也就是确认自己在法律上有权向项目贡献这些内容。

做法很简单，在每个 Git 提交末尾添加一行::

   Signed-off-by: YOUR NAME <YOUR@EMAIL>


这表示补丁作者按
`开发者来源证明 <https://developercertificate.org>`__ 的以下条款贡献：

.. _dco:

  开发者来源证明 1.1

  向本项目作出贡献，即证明：

  (a) 贡献全部或部分由我创作，我有权按文件中注明的开源许可证提交；或者

  (b) 贡献基于已有工作。尽我所知，该工作采用适当的开源许可证，
      我有权依据该许可证，按文件所注明的相同许可证提交修改后的工作，
      无论修改全部或部分由我创作；除非我已获准采用其他许可证提交；或者

  (c) 贡献由其他人直接提供给我，此人已按 (a)、(b) 或 (c) 作出证明，
      且我没有修改它。

  (d) 我理解并同意，本项目及贡献是公开的，贡献记录会永久保存，
      包括我随贡献提交的全部个人信息和签署信息，
      并可按本项目或相关开源许可证重新分发。

Signed-off-by 名称无需是法定姓名、出生姓名或证件姓名。
它是你在社区中选择使用的身份，但不得匿名，也不得虚假表示身份。

通常要求至少一条 Signed-off-by 的姓名和邮箱匹配提交 Author 字段。
可以通过多个邮箱订阅或贡献，但同一提交混用多个地址只会造成混淆。

发送邮件的人即使不是作者，也应添加自己的 Signed-off-by，以满足 DCO (c)。

多人创作
~~~~~~~~

补丁由多人共同贡献很常见，通常每位参与创作的人都应有 Signed-off-by。
边界情况包括：

  * 次要作者贡献极其简单，可以认为不受版权保护，此时无需其签署。

    最常见于审查者提供简短修复代码。
    除非建议代码异常庞大，否则不必添加审查者 Signed-off-by，
    但非简单建议通常通过 Suggested-by 致谢。

  * 两位贡献者受雇于同一单位，雇主要求转让版权。

    此时签署可视为确认已获得版权持有者雇主许可。
    但仍建议每人签署，因为部分国家不允许雇员向雇主转让版权，
    也能覆盖工作时间之外的投入。

多条 Signed-off-by 必须严格按创作先后排列，从最早到最新。

其他提交标签
~~~~~~~~~~~~

除必需 Signed-off-by 外，开发中常见标签包括：

 * **Reviewed-by**：社区成员审查后认为补丁可接受，应回复此标签。
   子系统维护者即使同时添加自己的 Signed-off-by，也应在审查后添加它。

 * **Acked-by**：维护者认可触及自己子系统的补丁，但打算由另一维护者合入并发送请求时使用。
   跨多个子系统时，它只代表本人负责范围的审查；完整审查应使用 Reviewed-by。

 * **Tested-by**：以某种方式完成功能测试后，通过回复提供。

 * **Reported-by**：问题经邮件列表等非问题跟踪系统的渠道报告时，
   修复补丁应以此致谢。GitLab 问题报告则只需附问题链接。

 * **Suggested-by**：审查者或其他第三方提供非简单修改建议时，用于致谢。

子系统维护者要求
~~~~~~~~~~~~~~~~

接受贡献者补丁时，除常规审查外，还应确认存在适当 Signed-off-by。

将补丁加入子系统树时，维护者必须添加自己的签署，表示已完成上述确认。
这不能替代希望添加的 Reviewed-by。

若补丁进入树后又作修改，应记录维护者贡献，
通常在自己签署前加入说明::

    Signed-off-by: Cory Contributor <cory.contributor@example.com>
    [Comment rephrased for clarity]
    Signed-off-by: Mary Maintainer <mary.maintainer@mycorp.test>




添加 ``Signed-off-by`` 的工具
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

多种工具可自动添加标签，避免反复手输。

Git 命令
^^^^^^^^

创建或修改提交时，git commit -s 会按配置的作者信息追加签署。

git format-patch -s 可仅在生成邮件中追加，不修改本地提交。
若要修改分支全部本地提交，可以::

  git rebase master -x 'git commit --amend --no-edit -s'


Emacs
^^^^^^^^

在 $HOME/.emacs.d/abbrev_defs 添加：

.. code:: elisp

  (define-abbrev-table 'global-abbrev-table
    '(
      ("8rev" "Reviewed-by: YOUR NAME <your@email.addr>" nil 1)
      ("8ack" "Acked-by: YOUR NAME <your@email.addr>" nil 1)
      ("8test" "Tested-by: YOUR NAME <your@email.addr>" nil 1)
      ("8sob" "Signed-off-by: YOUR NAME <your@email.addr>" nil 1)
     ))


之后输入 8rev 等缩写，再按空格或回车，即展开为完整文本。

Vim
^^^^^^^^

在 $HOME/.vimrc 添加::

  iabbrev 8rev Reviewed-by: YOUR NAME <your@email.addr>
  iabbrev 8ack Acked-by: YOUR NAME <your@email.addr>
  iabbrev 8test Tested-by: YOUR NAME <your@email.addr>
  iabbrev 8sob Signed-off-by: YOUR NAME <your@email.addr>


之后输入 8rev 等缩写，再按空格或回车，即展开为完整文本。

接续已放弃的工作
~~~~~~~~~~~~~~~~

有些补丁因各种原因提交后未合并。
数月或数年后，其他贡献者可能决定接续并作额外修改后重发。

基本原则是：

 * 保留原作者 Signed-off-by，继续承认其贡献。
 * 发送审查时说明原补丁来源，例如邮件列表、缺陷跟踪器或作者 Git 仓库。
 * 除原作者签署外，添加新贡献者签署，表明新增工作。
 * 说明各人负责部分，通常在新贡献者签署前加注释::

    Signed-off-by: Some Person <some.person@example.com>
    [Rebased and added support for 'foo']
    Signed-off-by: New Person <new.person@mycorp.test>


情况复杂或有疑问时，请在项目邮件列表寻求建议。

也建议尝试联系原作者，告知希望接手，
因为他们可能仍计划回来继续，或能提供后续工作的建议。

包含生成文件
~~~~~~~~~~~~

提交给 QEMU 的文件通常应仅采用最适合修改的源码形式，
因此代码生成器或编译器输出通常不适合直接贡献。

出于实际需要存在例外：如果无法合理要求构建者执行生成或编译过程，
可以包含生成内容，但必须同时提供相应首选源码。例子包括：

 * 图像：从矢量生成位图时，通常同时提供所需分辨率位图，
   因为光栅化工具或过程的细微变化可能影响质量。
   原始矢量文件必须同时提供。

 * 固件：QEMU 包含多种预编译 ROM，提交时必须同时直接提供源码，
   或通过 Git 子模块引用提供。

 * Dockerfile：大多数由源码树中的标准依赖列表和 libvirt-ci 子模块自动生成。
   保留输出是为了能从干净 Git 检出直接构建容器。

 * eBPF：部分目标发行版尚未广泛提供编译工具，因此 QEMU 包含生成的机器码，
   同时提供对应 C 源码。这是临时例外，直到工具链足够普及。

以上情况都必须在引入文件的提交中说明生成文件存在，并解释理由。

按用户指定输入、用确定算法修改已有代码的工具，通常不视为生成器。
例如 Coccinelle 模式转换、拼写检查器修正文档，以及 sed/awk 变换，
都不算代码生成；但提交说明仍应声明自动处理的使用。

贡献者有时用工具生成初始样板，再填充成最终补丁。
这种输出仍可视为首选源码，因为它是后续人工修改的基础。
只要输出版权和许可清晰，允许使用；尤其注意下文 AI 内容生成器的限制。

使用 AI 生成内容
~~~~~~~~~~~~~~~~

简要政策：

  **QEMU 当前拒绝被认为包含 AI 生成内容，或衍生自此类内容的贡献。
  包括 ChatGPT、Claude、Copilot、Llama 及类似工具。**

  **本政策不限制其他 AI 用途，例如研究 API 或算法、静态分析、调试，
  前提是其输出不包含在贡献中。**

AI 辅助开发日益普遍，为包括 QEMU 在内的软件项目带来困难的法律问题和风险。
尤其需要关注 `大语言模型 <https://en.wikipedia.org/wiki/Large_language_model>`__ 的输出。

社区要求贡献者证明补丁符合 `开发者来源证明（DCO） <dco>`。

为满足 DCO，贡献者必须充分理解提交内容的版权和许可状态。
AI 生成器输出的相关状态并不明确，缺少普遍接受、已经确定的法律基础。

已知训练材料常包含大量受严格版权或许可证限制的内容。
即使全部采用开源许可证，也很可能存在多种条款，未必都兼容 QEMU 的要求。

对于当前常见 AI 生成器，如何满足 DCO (b) 或 (c) 并不清楚。
QEMU 不愿也无法承担不合规的法律风险。

因此，计划向项目提交的补丁不得使用 AI 内容生成器，
已知或怀疑使用 AI 的贡献都会被拒绝。

受影响工具包括 GitHub Copilot、OpenAI ChatGPT、Anthropic Claude、
Meta Code Llama，以及建立在它们之上的代码或内容生成智能体。

随着工具成熟和法律状况明确，政策可能变化。

例外
^^^^^^^^

欢迎讨论政策例外或更普遍的修订。
可联系 qemu-devel，说明有助于 QEMU 且能缓解 DCO 合规问题的工具、模型或使用场景。
讨论确定的例外会列在下方。

例外不免除其他贡献要求。
尤其 Signed-off-by 表示作者对整个补丁负责，
包括任何由 AI 或其他工具生成、辅助完成的部分。
