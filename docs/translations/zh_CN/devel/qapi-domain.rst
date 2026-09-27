==============
Sphinx QAPI 域
==============

位于 docs/sphinx/qapi_domain.py 的 QAPI 域扩展 Sphinx 的
`rST 语法 <https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html>`_。
它类似 Sphinx 自带的
`Python 域 <https://www.sphinx-doc.org/en/master/usage/domains/python.html>`_，
但专门提供标注和描述 QAPI 定义的指令与角色。

`域 <https://www.sphinx-doc.org/en/master/usage/domains/index.html>`_
为 Sphinx 提供一组专用 rST 指令和交叉引用角色，使其理解某种语言的文档标记。
QAPI 域本身只解析手写标记；
`自动文档 <https://www.sphinx-doc.org/en/master/usage/extensions/autodoc.html>`_
功能由 docs/sphinx/qapidoc.py 中的 Transmogrifier 提供。

通常不要求开发者或文档作者直接编写或阅读这些特殊形式。
但调试时了解语法很有帮助，本文也可作为理解扩展实现的指南。
多数形式无需直接用于文档编写，但交叉引用语法对其他 rST 文档，
以及丰富 QAPIDoc 块正文很有用。


概念
========

QAPI 域不读取 Schema，也不从已有代码生成文档，只定义描述对象的 rST 语法。
例如 Python 域提供 ``:py:func:``，但读取 Python 并生成这些标记的是 autodoc。
QAPI 同理：qapidoc.py 读取 Schema 并生成 rST，
qapi_domain.py 解释特殊语法，并提供 Sphinx 内部 API。

换言之，qapi_domain.py 添加 ``.. qapi:command::`` 等语法，
qapidoc.py 则将 ``qapi/*.json`` 的文档转换为使用这些指令的 rST。

也可以更简短地说：``:py:`` 对应 ``:qapi:``，就像 autodoc 对应 qapidoc。


信息字段列表
============

`字段列表 <https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html#field-lists>`_
是标准 reStructuredText 语法。
Sphinx `扩展该语法 <https://www.sphinx-doc.org/en/master/usage/domains/python.html#info-field-lists>`_，
为特定字段赋予特殊含义和解析方式，例如添加交叉引用。
QAPI 域用它描述参数、成员、值等内容。

Sphinx 主要通过 Field、GroupedField 和 TypedField 三个类处理字段列表，
选择的类决定行为和格式。

Field：
  * 不分组，每项生成独立部分，不会合并。
  * 可以支持可选参数。
  * 可为参数、正文、两者或都不应用交叉引用角色。

适用于预期最多出现一次的条目，QAPI 用于 Errors 部分。

GroupedField：
  * 分组，相邻多项合并成一部分，内容形成项目符号列表。
  * 必须接收一个参数。
  * 可对参数应用交叉引用，但不能对正文应用。
  * 可以配置为仅一项时取消项目符号列表。
  * 各项呈现为“参数 -- 正文”。

适用于会重复、但无需两个参数的条目，例如只有类型无名称，或只有名称无类型。
QAPI 用于功能、返回值和枚举值。

TypedField：
  * 带类型的分组字段，相邻项合并成项目符号列表。
  * 至少接收一个、最多两个参数，通常为名称和类型。
  * 可对类型或名称应用交叉引用，但不能对正文应用。
  * 可以配置为仅一项时取消项目符号列表。
  * 各项呈现为“名称（类型）-- 正文”。

适用于重复且具有名称、类型、说明的条目。
QAPI 用于参数、备选项和成员。
下文的类型名必须是已有文档、可在 HTML 中引用的有效类型，
或内置 JSON 类型：string、number、int、boolean、null、value、q_empty。


``:feat:``
----------

描述 QAPI 定义附带的功能。

:availability: 可用于 Command、Event、Enum、Object 和 Alternate 指令正文。
:syntax: ``:feat name: Lorem ipsum, dolor sit amet...``
:type: `sphinx.util.docfields.GroupedField
       <https://pydoc.dev/sphinx/latest/sphinx.util.docfields.GroupedField.html?private=1>`_

例如::

   .. qapi:object:: BlockdevOptionsVirtioBlkVhostVdpa
      :since: 7.2
      :ifcond: CONFIG_BLKIO

      Driver specific block device options for the virtio-blk-vhost-vdpa
      backend.

   :memb string path: path to the vhost-vdpa character device.
   :feat fdset: Member ``path`` supports the special "/dev/fdset/N" path
       (since 8.1)



``:arg:``
---------

描述 QAPI 命令参数。

:availability: 仅用于 Command 正文。
:syntax: ``:arg type name: description``
:type: `sphinx.util.docfields.TypedField
       <https://pydoc.dev/sphinx/latest/sphinx.util.docfields.TypedField.html?private=1>`_

例如::

   .. qapi:command:: job-pause
      :since: 3.0

      Pause an active job.

      This command returns immediately after marking the active job for
      pausing.  Pausing an already paused job is an error.

      The job will pause as soon as possible, which means transitioning
      into the PAUSED state if it was RUNNING, or into STANDBY if it was
      READY.  The corresponding JOB_STATUS_CHANGE event will be emitted.

      Cancelling a paused job automatically resumes it.

      :arg string id: The job identifier.



``:error:``
-----------

描述 QAPI 命令的错误条件。

:availability: 仅用于 Command 正文。
:syntax: ``:error: Lorem ipsum dolor sit amet ...``
:type: `sphinx.util.docfields.Field
       <https://pydoc.dev/sphinx/latest/sphinx.util.docfields.Field.html?private=1>`_

错误说明可以是任意 rST。也允许拼作 ``:errors:``，两者完全等价。

例如::

   .. qapi:command:: block-job-set-speed
      :since: 1.1

      Set maximum speed for a background block operation.

      This command can only be issued when there is an active block job.

      Throttling can be disabled by setting the speed to 0.

      :arg string device: The job identifier.  This used to be a device
          name (hence the name of the parameter), but since QEMU 2.7 it
          can have other values.
      :arg int speed: the maximum speed, in bytes per second, or 0 for
          unlimited.  Defaults to 0.
      :error:
          - If no background operation is active on this device,
            DeviceNotActive



``:return:``
------------

描述 QAPI 命令的返回类型和值。

:availability: 仅用于 Command 正文。
:syntax: ``:return type: Lorem ipsum dolor sit amet ...``
:type: `sphinx.util.docfields.GroupedField
       <https://pydoc.dev/sphinx/latest/sphinx.util.docfields.GroupedField.html?private=1>`_

例如::

   .. qapi:command:: query-replay
      :since: 5.2

      Retrieve the record/replay information.  It includes current
      instruction count which may be used for ``replay-break`` and
      ``replay-seek`` commands.

      :return ReplayInfo: record/replay information.

      .. qmp-example::

          -> { "execute": "query-replay" }
          <- { "return": {
                 "mode": "play", "filename": "log.rr", "icount": 220414 }
             }



``:return-nodesc:``
-------------------

描述返回类型，不附带说明。

:availability: 仅用于 Command 正文。
:syntax: ``:return-nodesc: type``
:type: `sphinx.util.docfields.Field
       <https://pydoc.dev/sphinx/latest/sphinx.util.docfields.Field.html?private=1>`_

例如::

   .. qapi:command:: query-replay
      :since: 5.2

      Retrieve the record/replay information.  It includes current
      instruction count which may be used for ``replay-break`` and
      ``replay-seek`` commands.

      :return-nodesc: ReplayInfo

      .. qmp-example::

          -> { "execute": "query-replay" }
          <- { "return": {
                 "mode": "play", "filename": "log.rr", "icount": 220414 }
             }


``:value:``
-----------

描述 QAPI 枚举的一种取值。

:availability: 仅用于 Enum 正文。
:syntax: ``:value name: Lorem ipsum, dolor sit amet ...``
:type: `sphinx.util.docfields.GroupedField
       <https://pydoc.dev/sphinx/latest/sphinx.util.docfields.GroupedField.html?private=1>`_

例如::

   .. qapi:enum:: QapiErrorClass
      :since: 1.2

      QEMU error classes

      :value GenericError: this is used for errors that don't require a specific
          error class.  This should be the default case for most errors
      :value CommandNotFound: the requested command has not been found
      :value DeviceNotActive: a device has failed to be become active
      :value DeviceNotFound: the requested device has not been found
      :value KVMMissingCap: the requested operation can't be fulfilled because a
          required KVM capability is missing



``:alt:``
---------

描述 QAPI alternate 的一种分支。

:availability: 仅用于 Alternate 正文。
:syntax: ``:alt type name: Lorem ipsum, dolor sit amet ...``
:type: `sphinx.util.docfields.TypedField
       <https://pydoc.dev/sphinx/latest/sphinx.util.docfields.TypedField.html?private=1>`_

受 Sphinx 限制，除类型外还必须描述分支名称，
即使名称在 QMP 线上格式中不可见。未来可能解除这一限制。

例如::

   .. qapi:alternate:: StrOrNull
      :since: 2.10

      This is a string value or the explicit lack of a string (null
      pointer in C).  Intended for cases when 'optional absent' already
      has a different meaning.

       :alt string s: the string value
       :alt null n: no string value



``:memb:``
----------

描述 Event 或 Object 的成员。

:availability: 用于 Event 或 Object 正文。
:syntax: ``:memb type name: Lorem ipsum, dolor sit amet ...``
:type: `sphinx.util.docfields.TypedField
       <https://pydoc.dev/sphinx/latest/sphinx.util.docfields.TypedField.html?private=1>`_

本质与 ``:arg:`` 和 ``:alt:`` 相同，但对事件、结构体和联合体使用 Members 标签。

例如::

   .. qapi:event:: JOB_STATUS_CHANGE
      :since: 3.0

      Emitted when a job transitions to a different status.

      :memb string id: The job identifier
      :memb JobStatus status: The new job status



任意字段列表
------------

其他字段名虽然是有效 rST，但 QAPI 指令内部禁止使用，
以防信息字段名称拼写错误。
若需添加无特殊处理的任意字段，必须先加入 docs/conf.py 的允许列表。

例如::

   qapi_allowed_fields = {
       "see also",
   }


之后即可使用::

   .. qapi:command:: x-fake-command

      :see also: Lorem ipsum, dolor sit amet ...


.. _QAPI-domain-cross-references:

交叉引用
========

QAPI 的交叉引用
`角色 <https://www.sphinx-doc.org/en/master/usage/restructuredtext/roles.html>`_
紧密仿照
`Python 交叉引用语法 <https://www.sphinx-doc.org/en/master/usage/domains/python.html#cross-referencing-python-objects>`_。

可以用标准
`any 角色 <https://www.sphinx-doc.org/en/master/usage/referencing.html#role-any>`_
引用 QAPI 定义，例如 ```query-blockstats```。
需要消除歧义时，可以使用显式角色：

* ``:qapi:mod:`block-core```：引用模块，链接到相应文档部分开头。
* ``:qapi:cmd:`query-block```：引用命令。
* ``:qapi:event:`JOB_STATUS_CHANGE```：引用事件。
* ``:qapi:enum:`QapiErrorClass```：引用枚举。
* ``:qapi:obj:`BlockdevOptionsVirtioBlkVhostVdpa```：引用对象，即结构体或联合体。
* ``:qapi:alt:`StrOrNull```：引用 alternate。
* ``:qapi:type:`BlockDirtyInfo```：引用任意 QAPI 类型，不包括模块、命令或事件。
* ``:qapi:any:`block-job-set-speed```：引用任意 QAPI 实体。

信息字段中的类型参数按 ``:qapi:type:`` 方式转换为引用。
下述特殊语法同时适用于字段列表和独立显式引用。


类型修饰
--------

类型名可加方括号表示数组，例如 [typename]，链接只作用于括号内类型。
例如 ``:qapi:type:`[Qcow2BitmapInfoFlags]``` 呈现为：
:qapi:type:`[QMP:Qcow2BitmapInfoFlags]`

字段列表中，可在类型后加 ? 表示可选参数或成员。
呈现形式为“type, Optional”，链接只覆盖类型名。
例如 ``:qapi:type:`BitmapSyncMode?``` 呈现为：
:qapi:type:`QMP:BitmapSyncMode?`


命名空间
--------

类似 `Python 域目标规格语法
<https://www.sphinx-doc.org/en/master/usage/domains/python.html#target-specification>`_，
QAPI 可以指定数据类型的完全限定路径。

* 可显式指定命名空间，例如 ``:qapi:type:`QMP:BitmapSyncMode``。
* 可显式指定模块，``:qapi:type:`QMP:block-core.BitmapSyncMode``` 呈现为：
  :qapi:type:`QMP:block-core.BitmapSyncMode`
* 不希望显示完全限定名时，加波浪号前缀。
  ``:qapi:type:`~QMP:block-core.BitmapSyncMode``` 呈现为：
  :qapi:type:`~QMP:block-core.BitmapSyncMode`


目标解析
--------

无论使用 any 还是显式 ```:qapi:any:`target```，
引用均可省略或提供命名空间、模块信息。

省略时，根据之前的 qapi:namespace、qapi:module 指令推断上下文。
推断值未找到结果时，最后会搜索其他命名空间和模块；
显式提供的值则必须匹配。

因此多数情况下只需很少语法即可有效引用。
但在 QAPI 参考文档之外，若多个 Schema 有同名项目，可能需要额外上下文或命名空间。


自定义链接文字
--------------

与 `标准 Sphinx 引用 <https://www.sphinx-doc.org/en/master/usage/referencing.html#syntax>`_
一样，可用 ``custom text <target>`` 覆盖显示文字。

例如 ``:qapi:cmd:`Merge dirty bitmaps <block-dirty-bitmap-merge>``` 呈现为：
:qapi:cmd:`合并脏位图 <QMP:block-dirty-bitmap-merge>`。


指令
========

QAPI 域提供多个自定义指令描述 QAPI/QMP 实体，采用普通 rST，通用格式为::

  .. qapi:directive:: argument
     :option:
     :another-option: with an argument

     Content body, arbitrary rST is allowed here.



Sphinx 标准选项
---------------

所有 QAPI 指令继承 Sphinx ObjectDescription 的
`标准选项 <https://www.sphinx-doc.org/en/master/usage/domains/index.html#basic-markup>`_。

带连字符的拼写在 Sphinx 7.2 引入。
不带连字符的旧拼写当前仍作为别名保留，未来会移除。

* ``:no-index:`` 和 ``:noindex:``：不加入索引，也不允许交叉引用。
* no-index-entry: 和 ``:noindexentry:``：不加入索引，但允许交叉引用。
* no-contents-entry 和 ``:nocontentsentry:``：不加入目录。
* no-typesetting：创建目录、索引及交叉引用实体，但不显示正文。


QAPI 标准选项
-------------

除 namespace 和 module 外，全部指令支持：

* ``:namespace:`` name：覆盖定义所属命名空间。
* ``:module:`` modname：借鉴 Python 域，覆盖定义所属模块。
* ``:since:`` x.y：描述引入版本，在签名栏显示。
* ``:ifcond:`` CONDITION：描述可用条件，在签名栏下突出显示。
* ``:deprecated:``：在签名栏下标明定义已弃用，应避免使用。
* ``:unstable:``：在签名栏下标明不稳定，不应在生产代码中使用。


qapi:namespace
--------------

标记命名空间开始，不接收正文或选项。
后续 QAPI 指令归属最近的命名空间，影响完全限定名，
允许不同空间定义同名对象。

它还影响未显式指定命名空间的引用解析，使其优先选择当前空间目标。

例如::

   .. qapi:namespace:: QMP



此指令没有可见输出。


qapi:module
-----------

标记模块开始，可带正文，也可省略。
后续指令归属最近模块，影响完全限定名，没有其他影响。

例如::

   .. qapi:module:: block-core

      Welcome to the block-core module!


呈现为：

.. qapi:module:: block-core
   :noindex:

   欢迎来到 block-core 模块！


qapi:command
------------

描述 QMP 命令，可使用任意 Sphinx 或 QAPI 标准选项。
正文可包含 ``:arg:``、 ``:feat:``、 ``:error:``、 ``:return:`` 信息字段。

例如::

  .. qapi:command:: x-fake-command
     :since: 42.0
     :unstable:

     This command is fake, so it can't hurt you!

     :arg int foo: Your favorite number.
     :arg string? bar: Your favorite season.
     :return [string]: A lovely computer-written poem for you.



呈现为：

  .. qapi:command:: x-fake-command
     :noindex:
     :since: 42.0
     :unstable:

     这是虚构命令，不会伤害你！

     :arg int foo: 你最喜欢的数字。
     :arg string? bar: 你最喜欢的季节。
     :return [string]: 计算机为你写的一首可爱小诗。


qapi:event
----------

描述 QMP 事件，可使用任意标准选项，正文可包含 ``:memb:`` 或 ``:feat:``。

例如::

  .. qapi:event:: COMPUTER_IS_RUINED
     :since: 0.1
     :deprecated:

     This event is emitted when your computer is *extremely* ruined.

     :memb string reason: Diagnostics as to what caused your computer to
        be ruined.
     :feat sadness: When present, the diagnostic message will also
        explain how sad the computer is as a result of your wrongdoings.


呈现为：

.. qapi:event:: COMPUTER_IS_RUINED
   :noindex:
   :since: 0.1
   :deprecated:

   计算机严重损坏时发出此事件。

   :memb string reason: 导致计算机损坏的诊断信息。
   :feat sadness: 存在时，诊断信息还会说明计算机因你的错误操作而有多难过。


qapi:enum
---------

描述 QAPI 枚举，可使用任意标准选项，正文可包含 ``:value:`` 或 ``:feat:``。

例如::

  .. qapi:enum:: Mood
     :ifcond: LIB_PERSONALITY

     This enum represents your virtual machine's current mood!

     :value Happy: Your VM is content and well-fed.
     :value Hungry: Your VM needs food.
     :value Melancholic: Your VM is experiencing existential angst.
     :value Petulant: Your VM is throwing a temper tantrum.


呈现为：

.. qapi:enum:: Mood
   :noindex:
   :ifcond: LIB_PERSONALITY

   此枚举表示虚拟机当前的心情！

   :value Happy: 虚拟机很满足，吃得很饱。
   :value Hungry: 虚拟机需要食物。
   :value Melancholic: 虚拟机正在经历存在主义焦虑。
   :value Petulant: 虚拟机正在发脾气。


qapi:object
-----------

描述 QAPI 结构体或联合体，代表 QMP 对象。
可使用任意标准选项，正文可包含 ``:memb:`` 或 ``:feat:``。

例如::

  .. qapi:object:: BigBlobOfStuff

     This object has a bunch of disparate and unrelated things in it.

     :memb int Birthday: Your birthday, represented in seconds since the
                         UNIX epoch.
     :memb [string] Fav-Foods: A list of your favorite foods.
     :memb boolean? Bizarre-Docs: True if the documentation reference
        should be strange.


呈现为：

.. qapi:object:: BigBlobOfStuff
   :noindex:

   此对象包含许多互不相关的内容。

   :memb int Birthday: 你的生日，以 Unix 纪元以来的秒数表示。
   :memb [string] Fav-Foods: 你最喜欢的食物列表。
   :memb boolean? Bizarre-Docs: 若文档参考应当古怪，则为 true。


qapi:alternate
--------------

描述 QAPI alternate，可使用任意标准选项，正文可包含 ``:alt:`` 或 ``:feat:``。

例如::

  .. qapi:alternate:: ErrorCode

     This alternate represents an Error Code from the VM.

     :alt int ec: An error code, like the type you're used to.
     :alt string em: An expletive-laced error message, if your
        computer is feeling particularly cranky and tired of your
        antics.


呈现为：

.. qapi:alternate:: ErrorCode
   :noindex:

   此 alternate 表示虚拟机的错误代码。

   :alt int ec: 一个常规整数错误码。
   :alt string em: 若计算机特别暴躁、厌烦你的折腾，
      则返回夹杂粗话的错误消息。
