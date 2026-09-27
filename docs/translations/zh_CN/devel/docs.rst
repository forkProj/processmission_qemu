
=========
QEMU 文档
=========

QEMU 文档使用 reStructuredText 编写，通过 Sphinx 生成。
HTML 手册和 man 手册页由同一套文档源码生成。

hxtool 和 .hx 文件
------------------

QEMU 命令行选项和人类监控协议（HMP）命令的文档写在 .hx 后缀文件中。
这些文件有两种处理方式：

 * scripts/hxtool 从中生成 C 头文件，供 QEMU 包含，例如处理 --help 输出。
 * docs/sphinx/hxtool.py 中的 Sphinx 扩展生成 rST 内容，纳入 HTML 或 man 文档。

.hx 语法很简单，整体上交替包含用于 C 输出的 C 代码，
以及用于文档输出的 rST 文本。
系统识别少数特殊指令，均使用大写，且必须出现在行首。

``HXCOMM`` 是注释标记。整行，包括标记后的任意文本，
都会丢弃，既不进入 C 输出，也不进入文档。

``SRST`` 开始 reStructuredText 段，后续行原样进入文档，不进入 C 输出。
另一种形式 ``SRST()`` 用于定义可从其他 rST 文档引用的标签。
标签形式为 ``<DOCNAME-HXFILE-LABEL>``：DOCNAME 是顶层 rST 文件名，
HXFILE 是去掉 .hx 扩展名后的文件名，LABEL 是 SRST() 中提供的文本。
例如 ``<system/invocation-qemu-options-initrd>``。

``ERST`` 结束 SRST 开始的文档段，切回 C 代码段。

``DEFHEADING()`` 定义同时出现在 --help 和文档中的标题，应位于 C 代码块。
括号中有字符串时，将其作为标题；字符串为空时，在 --help 中产生空行，
在 rST 输出中忽略。

``ARCHHEADING()`` 是 DEFHEADING() 的变体，
只有指定客户机架构已编译进 QEMU 时才生成标题。
新文档应避免使用它。

在 C 代码段中，应查看文件顶部注释了解预期用法，因为各文件不同。
例如 qemu-options.hx 使用 DEF() 宏定义选项及其 --help 文本；
hmp-commands.hx 的 C 代码段则是 HMPCommand 结构体数组的元素，
定义各监控命令的名称、行为和帮助文本。

在 qemu-options.hx 中，不要在文档段显式定义 reStructuredText 标签。
该文件被包含到两个不同的 Sphinx 文档中，部分 Sphinx 版本会报告重复标签。
应使用上述 SRST() 指令生成无歧义的标签。
