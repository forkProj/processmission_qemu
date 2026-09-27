..
    Copyright (C) 2009-2016 Red Hat, Inc.

    This work is licensed under the terms of the GNU GPL, version 2 or
    later. See the COPYING file in the top-level directory.


============================================================
QEMU 机器协议规范
============================================================

QEMU 机器协议（QMP）基于 JSON，供应用在机器层面操作 QEMU。
QEMU 客户机代理（QGA）也使用它，让主机应用与客户机操作系统交互。
本文规定通用格式，命令和数据结构详情见 :doc:`qemu-qmp-ref` 与 :doc:`qemu-ga-ref`。

.. contents::

协议规范
========================================

本节说明协议格式。“服务器”指 QEMU 或 QGA，“客户端”指经 QMP 与其通信的应用。

本文提及 JSON 数据结构时，统一写为：

    json-DATA-STRUCTURE-NAME

DATA-STRUCTURE-NAME 为 `JSON 标准 <http://www.ietf.org/rfc/rfc8259.txt>`_
定义的任意有效结构。

服务器接受 UTF-8 输入，以 ASCII 输出。

为叙述方便，文中 json-object 成员按特定顺序列出，实际可为任意顺序，不能作假设。
相反，除非明确说明，json-array 元素顺序应被视为重要。
对象重复键会产生不可预测结果。

为方便使用，服务器也接受 ``'single-quoted'`` 字符串，替代通常的双引号字符串。
两种输入都支持额外的 ``\'`` 转义单引号；输出只用双引号。

通用定义
----------------------------------------

服务器发送的全部交互都是 json-object，并始终以 CRLF 结束。

未注明可选的对象成员均为必需。

.. _Server Greeting:

服务器问候
----------------------------------------

连接后服务器立即发出问候，表示连接成功并可开始能力协商，见 `Capabilities Negotiation`_。

格式如下::

  { "QMP": { "version": json-object, "capabilities": json-array } }

其中：

- version 为服务器版本，格式同 query-version。
- capabilities 表示基础规范之外的可用能力，数组元素顺序无特别含义。

能力
----------------------------------------

目前支持：

``oob``
  服务器支持带外执行，见 `Out-of-band execution`_。

发送命令
----------------------------------------

命令格式::

  { "execute": json-string, "arguments": json-object, "id": json-value }

或::

  { "exec-oob": json-string, "arguments": json-object, "id": json-value }

其中：

- execute 或 exec-oob 标识待执行命令，后者请求带外执行。
- arguments 传入所需参数；不需要参数时可省略，各命令定义其有效内容。
- id 是关联命令执行的事务标识，可选；提供时会出现在响应中。
  可为任意 json-value，逐命令递增的 json-number 即可。

具体命令见 :doc:`qemu-qmp-ref`。

.. _Out-of-band execution:

带外执行
----------------------------------------

服务器通常逐个读取、执行和回复命令，客户端按发送顺序收到响应。

经 `capabilities negotiation`_ 启用带外执行后，服务器读取到达的命令并入队，
依次执行。带外命令跳过队列立即执行，可能超越此前带内命令，
其响应也可能先于此前带内命令返回。

为匹配响应，客户端必须为带外命令传 id。
接受 oob 能力的客户端，建议对全部命令都传 id。

带内命令发送速度超过执行速度时，服务器会暂停读取，直到队列降至可接受长度。

为确保带外命令可被读取并执行，客户端最多应保留 8 个在途带内命令。

仅少数命令支持带外执行，在 query-qmp-schema 输出中标记为 ``"allow-oob": true``。

命令响应
----------------------------------------

执行结果可能是成功或错误。

命令若含正确的 id，响应也附相同值以匹配请求。
客户端应丢弃 id 未知的响应。

成功
----------------------------------------

成功响应格式::

  { "return": json-value, "id": json-value }

其中：

- return 为各命令定义的返回数据，通常为对象或对象数组，
  有时为数字、字符串或字符串数组；不返回数据时是空对象。
- id 为客户端提供的事务标识，如有。

错误
----------------------------------------

错误响应格式::

  { "error": { "class": json-string, "desc": json-string }, "id": json-value }

其中：

- class 为错误类名，例如 GenericError。
- desc 为供人阅读的错误消息，客户端不应尝试解析。
- id 为客户端提供的事务标识，如有。

注意：错误可能发生在服务器读取 id 之前，此时即使客户端提供了 id，回复也不会包含。

异步事件
----------------------------------------

状态变化可能使服务器在任何时刻单方面向客户端发消息，
但不能插入其他响应中间。这些消息称为异步事件。

格式如下::

  { "event": json-string, "data": json-object,
    "timestamp": { "seconds": json-number, "microseconds": json-number } }

其中：

- event 为事件名称。
- data 为按事件定义的专用数据，可选。
- timestamp 为事件在服务器发生的准确时间，以固定对象表示自 UNIX 纪元
  （1970 年 1 月 1 日）以来的秒和微秒；获取主机时间失败时，两者均为 -1。

具体事件见 :doc:`qemu-qmp-ref`。

部分事件限频为每秒最多一次。同一秒内更多相似事件只保留最后一个，
并延迟发送。相似通常指事件类型相同。

强制 JSON 解析器回到已知有效状态
----------------------------------------------------------------

不完整或无效输入可能使解析器无法处理后续命令。
客户端应触发词法错误，使其回到已知有效状态。

最干净的方法是发送除 ``\t``、``\r``、``\n`` 之外的 ASCII 控制字符。

但旧 QEMU 可能未将其识别为错误，需要兼容旧版时应发送 0xFF 字节。

QGA 同步
----------------------------------------

客户端通过 virtio-serial 等缺少适当连接语义的传输连接 QGA 时，
QGA 可能仍保留前一个客户端的不完整输入，应按上节方式重置解析器状态。
还可能收到前一客户端未读取的输出。QGA 提供 guest-sync-delimited 帮助跳过，
详情见该命令文档。


QMP 示例
========================================

以下展示真实用法，``->`` 表示客户端发送，``<-`` 表示服务器回复。

.. admonition:: 示例

  服务器问候

  .. code-block:: QMP

    <- { "QMP": {"version": {"qemu": {"micro": 0, "minor": 0, "major": 3},
         "package": "v3.0.0"}, "capabilities": ["oob"] } }

.. admonition:: 示例

  能力协商

  .. code-block:: QMP

    -> { "execute": "qmp_capabilities", "arguments": { "enable": ["oob"] } }
    <- { "return": {}}

.. admonition:: 示例

  简单 stop 执行

  .. code-block:: QMP

    -> { "execute": "stop" }
    <- { "return": {} }

.. admonition:: 示例

  KVM 信息

  .. code-block:: QMP

    -> { "execute": "query-kvm", "id": "example" }
    <- { "return": { "enabled": true, "present": true }, "id": "example"}

.. admonition:: 示例

  解析错误

  .. code-block:: QMP

    -> { "execute": }
    <- { "error": { "class": "GenericError", "desc": "JSON parse error, expecting value" } }

.. admonition:: 示例

  关机事件

  .. code-block:: QMP

    <- { "timestamp": { "seconds": 1258551470, "microseconds": 802384 },
        "event": "POWERDOWN" }

.. admonition:: 示例

  带外执行

  .. code-block:: QMP

    -> { "exec-oob": "migrate-pause", "id": 42 }
    <- { "id": 42,
         "error": { "class": "GenericError",
          "desc": "migrate-pause is currently only supported during postcopy-active state" } }


.. _Capabilities Negotiation:

能力协商
========================================

连接成功后，服务器处于能力协商模式。

此模式只允许 qmp_capabilities，其他命令返回 CommandNotFound，也不发送异步消息。

客户端应通过 qmp_capabilities 启用自身支持、且 `Server Greeting`_ 通告的能力。

该命令无错误返回后，服务器进入命令模式，能力变更生效，
除 qmp_capabilities 外的全部命令均可执行，也开始传递异步消息。

.. _Compatibility Considerations:

兼容性考虑
========================================

任何以不兼容方式修改协议格式的变更或新功能，默认禁用，
并通过 `Server Greeting`_ 的 capabilities 通告，客户端可检查并启用自身支持项。

服务器对参数做类型检查，值类型不符或无法识别客户端传入的键时会报错。
严格检查可发现客户端对模式的错误假设。
客户端可假定这些验证错误会在命令产生任何副作用之前报告。

但客户端不得假定：

- JSON 数组的特定长度。
- JSON 对象的特定大小。未来 QEMU 可添加新键，客户端应能忽略。
- 对象成员或数组元素的特定顺序。
- 命令会产生固定数量的错误。新服务器可为已有命令添加错误。

任何以 x- 开头的命令或成员均视为实验性，未来可移除或不兼容地修改。

服务器保证发送有效 JSON。除此之外，客户端应当谨慎发送、宽容接收。

QMP 下游扩展
========================================

建议下游 **不要** 修改 QMP。管理工具应无需特殊逻辑就支持上下游版本，
而下游扩展天然与此目标冲突。

但有时确实无法避免，上下游双方都需维护长期兼容和互操作。

因此，QMP 将双下划线 __ 开头的对象成员名保留给下游。
上游绝不会在命令、参数、错误、异步事件等使用这些名称。

下游新增名称必须以 __ 开头。为兼容其他下游，强烈建议使用 ``__RFQDN_`` 前缀，
其中 RFQDN 是自己控制的有效反向完全限定域名。
例如 qemu-kvm 专用监视器命令::

  (qemu) __org.linux-kvm_enable_irqchip

下游不得修改 `server greeting`_，除非增加能力；但下文说明为何连这样也不鼓励。

`Compatibility Considerations`_ 同样适用下游。
对于不含下游成员的输入，下游行为必须与上游完全一致，
唯一例外是允许在输出加入下游名称的成员。

因此，只要客户端不发送下游成员，并正确忽略输出中的下游成员，
就不应能区分上下游。

下游修改建议：

1. 可添加新命令。若要扩展已有命令，考虑新增具有新行为的命令。
2. 可添加新异步消息。若要扩展已有消息，考虑新增消息。
3. 新命令可使用新错误。为旧命令添加新错误属于扩展，适用第 1 条。
4. 强烈不建议新能力。能力用于基础协议演进，不希望出现多种相互分歧的基础协议方言。
