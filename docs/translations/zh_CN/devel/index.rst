----------
开发者信息
----------

本手册的这一部分介绍 QEMU 内部的各个组成部分。
如果希望阅读或修改 QEMU 源码，才需要阅读这些内容。

QEMU 是一个规模庞大且成熟的项目，包含多个复杂子系统，理解起来可能并不容易。
开发文档尚未覆盖所有内容，但希望能够提供足够的信息帮助你入门。
如果有不清楚的地方，请通过 IRC 频道或邮件列表联系社区，
帮助我们为后续开发者完善文档。

所有开发者都应熟悉 :ref:`development_process` 以及社区的协作方式。
请特别留意 :ref:`coding-style` 和 :ref:`submitting-a-patch`，避免常见问题。

如果希望实现新的硬件模型，请阅读 :ref:`qom`，了解 QEMU 对象模型的工作方式。

如果希望增强或添加 CPU 模拟能力，请阅读 :ref:`tcg` 文档，
尤其是 :ref:`tcg_internals` 的概述。

.. toctree::
   :maxdepth: 1

   index-process
   index-build
   testing/index
   index-api
   index-internals
   index-tcg
   codebase
