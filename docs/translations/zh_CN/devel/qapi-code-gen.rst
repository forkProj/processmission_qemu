====================
使用 QAPI 代码生成器
====================

..
   Copyright IBM Corp. 2011
   Copyright (C) 2012-2016 Red Hat, Inc.

   This work is licensed under the terms of the GNU GPL, version 2 or
   later.  See the COPYING file in the top-level directory.

.. _qapi:

简介
========

QAPI 是 QEMU 内部的原生 C API，为内外部使用者提供管理功能。
对外部用户或进程，通过基于 JSON 的线上格式暴露：
QMP 用于控制 QEMU，QEMU 客户机代理（QGA）用于与客户机通信。
下文将 QMP 或 QGA 连接上的内容统称为“客户端 JSON 协议”。

我们从 QAPI Schema 生成 C 代码，在客户端协议和原生 API 之间转换。
本文介绍 Schema 语言、它与协议及 C 的映射，
并说明如何维护客户端 JSON 协议兼容性。


QAPI Schema 语言
================

Schema 定义客户端协议的命令、事件及所用类型，允许前向引用。

也可包含未被命令或事件使用的额外类型，借助生成的 C 代码供内部使用。

类型包括简单类型（int、str 等内置类型和枚举）、数组、
复杂类型（结构体和联合体），以及在其他类型间选择的 alternate。


Schema 语法
-----------

语法大体基于 `JSON <http://www.ietf.org/rfc/rfc8259.txt>`_，区别如下：

* 注释从字符串之外的 # 开始，持续到行尾。
* 字符串使用单引号，而非双引号。
* 字符串只允许可打印 ASCII，转义序列只允许 ``\\``。
* 不支持数字和 null。

第二层语法定义哪些 JSON 文本序列构成有效 QAPI Schema。
这里使用类似 EBNF 的记法：

* 产生式为 ``non-terminal = expression``。
* 连接：A B 先匹配 A，再匹配 B。
* 选择：A | B 匹配 A 或 B。
* 重复：A... 匹配零次或多次 A。
* 重复：A, ... 匹配零次或多次以逗号分隔的 A。
* 分组：( A ) 匹配 A。
* JSON 结构字符 ``{ } [ ] : ,`` 是终结符。
* JSON 字面名 false 和 true 是终结符。
* 单引号字符串是终结符，匹配该 JSON 字符串，但去掉开头的 ``*``。
* JSON 对象成员名以 ``*`` 开头时，成员可选。
* STRING 是终结符，匹配任意 JSON 字符串。
* BOOL 是终结符，匹配 JSON false 或 true。
* 除 STRING 外的全大写单词是非终结符。

除非特别说明，对象内成员顺序无关。

Schema 由一系列顶层表达式组成::

    SCHEMA = TOP-LEVEL-EXPR...


顶层表达式均为 JSON 对象。代码和文档按 Schema 定义顺序生成，
生成代码的顺序不应影响行为。

顶层表达式是指令或定义::

    TOP-LEVEL-EXPR = DIRECTIVE | DEFINITION


有两类指令和六类定义::

    DIRECTIVE = INCLUDE | PRAGMA
    DEFINITION = ENUM | STRUCT | UNION | ALTERNATE | COMMAND | EVENT


下文详细介绍。


内置类型
--------

预定义类型及其 C 映射如下：

  ============= ============== ============================================
  模式          C              JSON
  ============= ============== ============================================
  ``str``       ``char *``     任意 UTF-8 JSON 字符串
  ``number``    ``double``     任意 JSON 数字
  ``int``       ``int64_t``    无小数部分的 JSON 数字
                               且数值可由对应 C 整数类型表示
  ``int8``      ``int8_t``     同上
  ``int16``     ``int16_t``    同上
  ``int32``     ``int32_t``    同上
  ``int64``     ``int64_t``    同上
  ``uint8``     ``uint8_t``    同上
  ``uint16``    ``uint16_t``   同上
  ``uint32``    ``uint32_t``   同上
  ``uint64``    ``uint64_t``   同上
  ``size``      ``uint64_t``   类似 uint64_t，但
                               StringInputVisitor 接受大小后缀
  ``bool``      ``bool``       JSON true 或 false
  ``null``      ``QNull *``    JSON ``null``
  ``any``       ``QObject *``  任意 JSON 值
  ``QType``     ``QType``      匹配 QType 枚举值的 JSON 字符串
  ============= ============== ============================================


包含指令
--------

语法::

    INCLUDE = { 'include': STRING }


可以通过 include 将 Schema 模块化::

 { 'include': 'path/to/file.json' }


指令递归求值，路径相对于包含它的文件。
同一文件多次包含具有幂等性。

风格上应让每个文件自包含，但当前并不禁止被包含文件前向引用
仅由外层文件引入的类型。未来解析器可能更严格，拒绝不完整的包含文件。

.. _pragma:

Pragma 指令
-----------

语法::

    PRAGMA = { 'pragma': {
                   '*doc-required': BOOL,
                   '*command-name-exceptions': [ STRING, ... ],
                   '*command-returns-exceptions': [ STRING, ... ],
                   '*documentation-exceptions': [ STRING, ... ],
                   '*member-name-exceptions': [ STRING, ... ] } }


pragma 控制可选生成器行为。
当前作用域是整个 Schema，在不同部分设置不同值无法正常工作。

'doc-required' 接收布尔值，为 true 时要求提供文档，默认 false。

'command-name-exceptions' 列出允许名称使用下划线而非连字符的命令，默认空。

'command-returns-exceptions' 列出允许违反返回类型规则的命令，默认空。

'documentation-exceptions' 列出成员或参数无需文档的类型、命令和事件，默认空。

'member-name-exceptions' 列出允许成员名使用大写及下划线的类型，默认空。

.. _ENUM-VALUE:

枚举类型
--------

语法::

    ENUM = { 'enum': STRING,
             'data': [ ENUM-VALUE, ... ],
             '*prefix': STRING,
             '*if': COND,
             '*features': FEATURES }
    ENUM-VALUE = STRING
               | { 'name': STRING,
                   '*if': COND,
                   '*features': FEATURES }


'enum' 指定类型名。
'data' 数组每项定义一个值，STRING 是 :code:`{ 'name': STRING }` 的简写，
名称必须互不相同。

例如::

 { 'enum': 'MyEnum', 'data': [ 'value1', 'value2', 'value3' ] }


允许空枚举，但通常没有用途。

线上以字符串名称表示枚举值，C 中使用枚举常量 PREFIX_NAME。
PREFIX 来自类型名，NAME 来自值名。
上例 MyEnum 转为 MY_ENUM，value1 转为 VALUE1，最终为 MY_ENUM_VALUE1。
可选 prefix 覆盖前缀，但很少需要，应克制使用。

生成的枚举值按 Schema 顺序为 0 到 N-1，N 是值数量，
并额外生成值为 N 的 PREFIX__MAX。

枚举能满足需要时，不要使用字符串或整数类型代替。

可选 if 指定条件，详见 `配置 Schema <#configuring-the-schema>`_。
可选 features 指定功能，详见 `功能 <#features>`_。


.. _TYPE-REF:

类型引用与数组类型
------------------

语法::

    TYPE-REF = STRING | ARRAY-TYPE
    ARRAY-TYPE = [ STRING ]


字符串表示同名类型。
包含一个字符串的单元素数组表示该类型的数组，
例如 ``['int']`` 表示 int 数组。


结构体类型
----------

语法::

    STRUCT = { 'struct': STRING,
               'data': MEMBERS,
               '*base': STRING,
               '*if': COND,
               '*features': FEATURES }
    MEMBERS = { MEMBER, ... }
    MEMBER = STRING : TYPE-REF
           | STRING : { 'type': TYPE-REF,
                        '*if': COND,
                        '*features': FEATURES }


'struct' 指定类型名，'data' 对象中的各 MEMBER 定义结构成员。

.. _MEMBERS:

MEMBER 的 STRING 名由可选 ``*`` 前缀及成员名组成，前缀存在表示可选。
其值定义属性，尤其是类型。
TYPE-REF_ 形式是 :code:`{ 'type': TYPE-REF }` 的简写。

例如::

 { 'struct': 'MyType',
   'data': { 'member1': 'str', 'member2': ['int'], '*member3': 'str' } }


Schema 结构体对应 C struct 和 JSON 对象，C 成员按 Schema 顺序生成。

可选 base 指定另一个结构体，将其成员包含进当前类型，且位于 C 结构开头。

例如::

 { 'struct': 'BlockdevOptionsGenericFormat',
   'data': { 'file': 'str' } }
 { 'struct': 'BlockdevOptionsGenericCOWFormat',
   'base': 'BlockdevOptionsGenericFormat',
   'data': { '*backing': 'str' } }


线上 BlockdevOptionsGenericCOWFormat 对象可以这样同时使用两个成员::

 { "file": "/some/place/my-image",
   "backing": "/some/place/my-backing-file" }


可选 if 指定条件，详见 `配置 Schema <#configuring-the-schema>`_。
可选 features 指定功能，详见 `功能 <#features>`_。


联合体类型
----------

语法::

    UNION = { 'union': STRING,
              'base': ( MEMBERS | STRING ),
              'discriminator': STRING,
              'data': BRANCHES,
              '*if': COND,
              '*features': FEATURES }
    BRANCHES = { BRANCH, ... }
    BRANCH = STRING : TYPE-REF
           | STRING : { 'type': TYPE-REF, '*if': COND }


'union' 指定类型名。
'base' 定义公共成员：若为 MEMBERS_ 对象，按结构体 data 的方式定义；
若为字符串，则指定某个结构体，其成员成为公共成员。

'discriminator' 必须是基础结构中一个非可选、枚举类型的成员名。
其值按名称选择分支；若没有对应分支，则视为空分支。

'data' 对象中每个 BRANCH 定义一个分支，至少必须有一个。
BRANCH 的 STRING 名必须是判别枚举的一项值。
其值定义分支属性，尤其是类型，类型必须为结构体。
TYPE-REF_ 是 :code:`{ 'type': TYPE-REF }` 的简写。

线上表示为同时包含基础公共成员及选中分支成员的对象，
两组成员名称不得重叠。

例如::

 { 'enum': 'BlockdevDriver', 'data': [ 'file', 'qcow2' ] }
 { 'union': 'BlockdevOptions',
   'base': { 'driver': 'BlockdevDriver', '*read-only': 'bool' },
   'discriminator': 'driver',
   'data': { 'file': 'BlockdevOptionsFile',
             'qcow2': 'BlockdevOptionsQcow2' } }


得到以下 JSON 对象::

 { "driver": "file", "read-only": true,
   "filename": "/some/place/my-image" }
 { "driver": "qcow2", "read-only": false,
   "backing": "/some/place/my-image", "lazy-refcounts": true }


分支顺序无需与枚举值顺序一致，也不必覆盖全部枚举值。
生成的 C 类型是结构体：先按 Schema 顺序放公共成员，
再放一个联合体，其各成员为各分支结构体。

可选 if 指定条件，详见 `配置 Schema <#configuring-the-schema>`_。
可选 features 指定功能，详见 `功能 <#features>`_。


Alternate 类型
--------------

语法::

    ALTERNATE = { 'alternate': STRING,
                  'data': ALTERNATIVES,
                  '*if': COND,
                  '*features': FEATURES }
    ALTERNATIVES = { ALTERNATIVE, ... }
    ALTERNATIVE = STRING : STRING
                | STRING : { 'type': STRING, '*if': COND }


'alternate' 指定类型名。
'data' 每个 ALTERNATIVE 定义一个分支，至少有一个。
STRING 名为分支名，值定义属性和类型，
字符串形式是 :code:`{ 'type': STRING }` 的简写。

例如::

 { 'alternate': 'BlockdevRef',
   'data': { 'definition': 'BlockdevOptions',
             'reference': 'str' } }


alternate 类似联合体，但线上没有判别字段，而是根据值推断分支。
它只能在具有不同线上表示的类型间选择。

bool 分支接受 true/false，数值内置类型接受 JSON 数字，
str 或枚举接受字符串，null 接受 JSON null，
结构体或联合体等复杂类型接受 JSON 对象。

上例允许以下两种对象::

 { "file": "my_existing_block_device_id" }
 { "file": { "driver": "file",
             "read-only": false,
             "filename": "/tmp/mydisk.qcow2" } }


可选 if 指定条件，详见 `配置 Schema <#configuring-the-schema>`_。
可选 features 指定功能，详见 `功能 <#features>`_。


命令
--------

语法::

    COMMAND = { 'command': STRING,
                (
                '*data': ( MEMBERS | STRING ),
                |
                'data': STRING,
                'boxed': true,
                )
                '*returns': TYPE-REF,
                '*success-response': false,
                '*gen': false,
                '*allow-oob': true,
                '*allow-preconfig': true,
                '*coroutine': true,
                '*if': COND,
                '*features': FEATURES }


'command' 指定命令名。
'data' 定义参数，默认为空 MEMBERS_ 对象。
若为 MEMBERS_，按结构体 data 定义成员的方式定义参数；
若为字符串，则指定复杂类型，其成员作为参数。
联合体类型要求 ``'boxed': true``。

'returns' 定义返回类型，默认空结构体。
通常必须是复杂类型或其数组。
返回其他类型时，命令必须列入 commands-returns-exceptions pragma，
但这样将来扩展返回信息会更困难，强烈不建议新命令使用。

错误响应不在 Schema 中指定，错误条件应通过注释记录。

线上 execute 或 exec-oob 的值是命令名，
arguments 必须符合参数定义，成功响应的 return 必须符合返回类型。

命令示例::

 { 'command': 'my-first-command',
   'data': { 'arg1': 'str', '*arg2': 'str' } }
 { 'struct': 'MyType', 'data': { '*value': 'str' } }
 { 'command': 'my-second-command',
   'returns': [ 'MyType' ] }


可验证以下客户端协议事务::

 => { "execute": "my-first-command",
      "arguments": { "arg1": "hello" } }
 <= { "return": { } }
 => { "execute": "my-second-command" }
 <= { "return": [ { "value": "one" }, { } ] }


生成器输出命令实现函数的 C 原型，函数本身需手写，
示例见 `命令生成代码 <#code-generated-for-commands>`_。

函数返回定义的返回类型。
未指定 boxed 时，按 Schema 顺序逐个接收参数；
否则接收复杂参数类型对应的 C 结构。
两种情况都额外接收 ``Error **``。

还生成参数编组函数：从输入 QDict 提取参数，调用用户函数，
成功时从返回值构造输出 QObject，供 QMP 监控器核心使用。

极少数情况下，QAPI 无法为协议命令表达类型安全的表示。
此时需设置 gen 为 false，禁止生成编组函数，改为手写。例如::

 { 'command': 'netdev_add',
   'data': {'type': 'str', 'id': 'str'},
   'gen': false }


请避免新增依赖此方式的命令，优先使用类型安全联合体。

Schema 通常描述需要响应的同步交互。
但有些命令成功时会改变状态，无法再返回成功响应，失败时仍返回错误对象。
此时设置可选 success-response 为 false，目前只有 QGA 使用。

allow-oob 声明是否支持带外执行，默认 false。例如::

 { 'command': 'migrate_recover',
   'data': { 'uri': 'str' }, 'allow-oob': true }


带外执行语法和语义见 :doc:`/interop/qmp-spec`。
支持带外的命令也可在带内执行。

带内处理器运行在主线程，持有 BQL；
带外处理器运行在专用监控器 I/O 线程，不持有 BQL。

支持 OOB 的处理器必须满足：

- 很快结束；
- 不调用可能阻塞的系统调用；
- 不访问后复制热迁移启用 userfaultfd 时可能阻塞的客户机 RAM；
- 只获取快速锁，即它取得的所有锁所保护的临界区，也必须符合上述要求。

锁限制也约束共享状态访问：这类访问需要同步，
但 OOB 命令不能取得 BQL 或其他慢锁。
拿不准时，不要实现 OOB 支持。

allow-preconfig 声明机器构建前能否调用命令，默认 false。例如::

 { 'enum': 'QMPCapability',
   'data': [ 'oob' ] }
 { 'command': 'qmp_capabilities',
   'data': { '*enable': [ 'QMPCapability' ] },
   'allow-preconfig': true }


只有使用 --preconfig 启动时，机器构建前才提供 QMP。

coroutine 告诉分派器处理器是否可安全地在协程中运行，默认 false。
为 true 时在协程上下文调用，可在等待 I/O 完成等外部事件时让出执行，
避免阻塞客户机和其他后台工作。

协程安全与线程安全一样难证明。常见问题包括：

- qemu_coroutine_yield() 不会跨让出过程持有 BQL，
  原先假定原子执行的操作需谨慎防范全局状态变化。

- AIO_WAIT_WHILE() 等嵌套事件循环在协程中容易死锁，
  应改为让出协程，并在条件变为 false 时重新进入。

处理器可以假定处于协程上下文，因此其他调用者也必须提供该上下文。
尤其 HMP 调用这种处理器时，必须在 hmp-commands.hx 标记 .coroutine = true。

同时指定 ``'coroutine': true`` 和 ``'allow-oob': true`` 是错误。
目前没有两者组合的使用场景，也尚不清楚应采用何种语义。

可选 if 指定条件，详见 `配置 Schema <#configuring-the-schema>`_。
可选 features 指定功能，详见 `功能 <#features>`_。


事件
--------

语法::

    EVENT = { 'event': STRING,
              (
              '*data': ( MEMBERS | STRING ),
              |
              'data': STRING,
              'boxed': true,
              )
              '*if': COND,
              '*features': FEATURES }


'event' 指定线上事件名称。
'data' 定义事件专用数据，默认为空 MEMBERS_ 对象。
若为对象，按结构体 data 的方式定义成员；
若为字符串，则指定某个复杂类型，以其成员作为事件数据。
联合体要求 ``'boxed': true``。

例如::

 { 'event': 'EVENT_C',
   'data': { '*a': 'int', 'b': 'str' } }


得到 JSON 对象::

 { "event": "EVENT_C",
   "data": { "b": "test string" },
   "timestamp": { "seconds": 1267020223, "microseconds": 435656 } }


生成器输出发送事件的函数。
没有 boxed 时按 Schema 顺序逐个接收事件数据，
否则接收对应复杂类型的 C 结构。
示例见 `事件生成代码 <#code-generated-for-events>`_。

可选 if 指定条件，详见 `配置 Schema <#configuring-the-schema>`_。
可选 features 指定功能，详见 `功能 <#features>`_。


.. _FEATURE:

功能
--------

语法::

    FEATURES = [ FEATURE, ... ]
    FEATURE = STRING
            | { 'name': STRING, '*if': COND }


QEMU 有时以兼容方式改变行为，却不改变 QMP 语法，
通常是允许原先报错的值或操作。客户端仍可能需要知道该扩展是否存在。

因此可为定义、枚举值和结构成员指定功能列表。
各项可以为 ``{ 'name': STRING, '*if': COND }``，
也可以是 ``{ 'name': STRING }`` 的字符串简写。

可选 if 指定条件，详见 `配置 Schema <#configuring-the-schema>`_。

例如::

 { 'struct': 'TestType',
   'data': { 'number': 'int' },
   'features': [ 'allow-negative-numbers' ] }


功能字符串通过内省暴露给客户端，见
`客户端 JSON 协议内省 <#client-json-protocol-introspection>`_。
预期每个字符串表明当前 QEMU 构建具有某种行为。


特殊功能
~~~~~~~~

deprecated 将命令、事件、枚举值或结构成员标记为弃用，当前不支持其他位置。
这些接口可按 QEMU 弃用政策在未来移除。

unstable 将同类实体标记为不稳定，当前同样不支持其他位置。
未来可移除或以不兼容方式修改这些接口。


命名规则与保留名称
------------------

名称必须以字母开头，只包含 ASCII 字母、数字、连字符和下划线。
例外是枚举值可用数字开头，下游扩展名则以下划线开头，
见 `下游扩展 <#downstream-extensions>`_。

``q_`` 前缀保留给生成器，用于转换类似 C 关键字或其他有问题的 QMP 名称。
例如 default 成员在 C 中变为 q_default。

类型、命令和事件共用命名空间。
因此用户类型通常使用 CamelCase，内置类型使用小写。

以 List 结尾的类型名保留给生成器，用于数组类型。

命令名、成员名和功能名应全部小写，以连字符分词。
部分旧命令和复杂类型使用下划线，扩展它们时应优先保持一致，
而非机械地避开下划线。

事件名全部大写，以下划线分词。

成员名 u，以及 ``has-``、``has_`` 前缀保留给生成器，
用于联合体和可选成员跟踪。

x- 前缀过去表示实验性，现在已由 unstable 特殊功能替代。

command-name-exceptions 和 member-name-exceptions pragma 可豁免规则，
但强烈不建议新代码使用。详情见 `Pragma 指令 <#pragma-directives>`_。


下游扩展
--------

对外可见的名称，例如客户端协议中的名称，必须谨慎管理。
以 ``__RFQDN_`` 为前缀的名称保留给控制该反向完全限定域名的下游。
RFQDN 只能包含 ASCII 字母、数字、连字符和点。

例如 Red Hat 控制 redhat.com，因此可添加 ``__com.redhat_drive-mirror`` 命令。


配置 Schema
-----------

语法::

    COND = STRING
         | { 'all': [ COND, ... ] }
         | { 'any': [ COND, ... ] }
         | { 'not': COND }


全部定义都接受可选 if，值必须是字符串，
或只有 all、any、not 其中一个成员的对象。

生成的 C 代码通过 #if 保护，条件转换方式为：

 * STRING 生成 defined(STRING)。
 * { 'all': [COND, ...] } 生成 (COND && ...)。
 * { 'any': [COND, ...] } 生成 (COND || ...)。
 * { 'not': COND } 生成 !COND。

例如条件结构体::

 { 'struct': 'IfStruct', 'data': { 'foo': 'int' },
   'if': { 'all': [ 'CONFIG_FOO', 'HAVE_BAR' ] } }


其生成代码由以下条件保护::

 #if defined(CONFIG_FOO) && defined(HAVE_BAR)
 ... generated code ...
 #endif /* defined(HAVE_BAR) && defined(CONFIG_FOO) */


复杂类型的单个成员也可附条件，需要 MEMBER 的完整形式。
例如 foo 无条件，bar 带条件::

 { 'struct': 'IfStruct',
   'data': { 'foo': 'int',
             'bar': { 'type': 'int', 'if': 'IFCOND'} } }


联合体判别字段不得带条件。

枚举的单个值也可附条件，需要 ENUM-VALUE_ 的完整形式。例如::

 { 'enum': 'IfEnum',
   'data': [ 'foo',
             { 'name' : 'bar', 'if': 'IFCOND' } ] }


功能同样可附条件，需要 FEATURE_ 的完整形式。
例如 allow-negative-numbers 为条件功能的结构体::

 { 'struct': 'TestType',
   'data': { 'number': 'int' },
   'features': [ { 'name': 'allow-negative-numbers',
                   'if': 'IFCOND' } ] }


开发者必须保证任意条件组合下 C 代码都能编译，生成器当前无法检查这一点。

条件也影响内省：只有当前构建满足条件时才显示对应实体。


文档注释
--------

以单独的 ## 行开始和结束的多行注释是文档注释。
若开头形式为::

    ##
    # @SYMBOL:


则描述 SYMBOL 定义，否则是自由格式文档。
详情见 `定义文档 <#definition-documentation>`_。
自由格式文档可添加说明文字和结构组织。


标题和子标题
~~~~~~~~~~~~

自由格式文档不以 @SYMBOL 开头，可以包含任意 rST。
标题使用标准语法::

    ##
    # *************************
    # This is a level 2 heading
    # *************************
    #
    # This is a free-form comment which will go under the
    # top level heading.
    ##

    ##
    # This is a third level heading
    # ==============================
    #
    # Level 4
    # _______
    #
    # Level 5
    # ^^^^^^^
    #
    # Level 6
    # """""""
    ##


一级标题保留给生成文档页面本身，因此正文最高使用二级标题。


文档标记
~~~~~~~~

文档注释可用多数 rST 标记，尤其可通过 :: 字面块保留文本格式::

    # ::
    #
    #   Text of the example, may span
    #   multiple lines


星号开始无序列表::

    # * First item, may span
    #   multiple lines
    # * Second item


也可使用连字符。

十进制数字后接点号，开始编号列表::

    # 1. First item, may span
    #    multiple lines
    # 2. Second item


数字实际取值无关。
两类列表前后都必须有空行，多行条目的后续行需正确缩进，
与首行第一个内容字符对齐。

应使用常规 ****strong****、 *\*emphasized\** 和 ````literal```` 标记。
若要表示单独的星号，需用反斜杠转义。

使用 ```foo``` 引用 Schema 定义并生成链接。
存在歧义时，可用 `QAPI 交叉引用角色 <QAPI-domain-cross-references>` 消除歧义。

@foo 引用当前定义中的成员说明，这是 rST 扩展。
当前显示形式与 ````foo```` 相同，但包含额外语义。

例如::

 ##
 # Some text foo with **bold** and *emphasis*
 #
 # 1. with a list
 # 2. like that
 #
 # And some code:
 #
 # ::
 #
 #   $ echo foo
 #   -> do this
 #   <- get that
 ##


为提高可读性，段落每行不应超过 70 个字符。
句子之间使用两个空格。


定义文档
~~~~~~~~

若提供定义文档，必须紧邻被描述定义之前。
要求文档时，即 pragma_ 的 doc-required 生效时，每个定义都必须有文档。

定义文档先描述所命名实体，可带缩进概要；
然后描述命令／事件参数、结构体／联合体成员、alternate 分支或枚举值；
再描述各功能（如有），最后是可选标签部分。

说明以 '\@name:' 开头，文本按如下方式缩进::

 # @name: Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed
 #     do eiusmod tempor incididunt ut labore et dolore magna aliqua.


.. FIXME The parser accepts these things in almost any order.

.. FIXME union branches should be described, too.

定义首次发布后添加的扩展，附带“(since x.y.z)”注释。

功能说明前需空行，再加一行 Features:，例如::

  #
  # Features:
  #
  # @feature: Description text


标签部分以 Since:、Returns:、Errors: 或 TODO: 开头的段落开始，
到下一部分开始时结束。第二行及后续行必须缩进::

 # TODO: Ut enim ad minim veniam, quis nostrud exercitation ullamco
 #     laboris nisi ut aliquip ex ea commodo consequat.
 #
 #     Duis aute irure dolor in reprehenderit in voluptate velit esse
 #     cillum dolore eu fugiat nulla pariatur.


Returns 和 Errors 只对命令有效，分别描述成功和错误响应。
Errors 应为 rST 列表，各项详细描述一个错误条件，例如::

 # Errors:
 #     - If @device does not exist, DeviceNotFound
 #     - Any other error returns a GenericError.


Since: x.y.z 表示首次引入该定义的版本。
TODO 面向开发者，不会显示给 QMP 用户。
其他部分会排版并允许 rST 标记。

可以通过 ``.. qmp-example::`` 添加 QMP 示例。
最简单形式包含一个 QMP 代码块，接受标准 JSON、服务器方向指示符 -> 与 <-，
以及省略部分。省略通常为 ...，也可在两组 ... 之间加说明文字。

可用 ``:title:`` 选项提供纯文本标题，省略时默认为“Example:”。
简单示例::

  # .. qmp-example::
  #
  #     -> { "execute": "query-name" }
  #     <- { "return": { "name": "Fred" } }


需要在代码块前或块间说明的复杂、多步骤示例，可使用 ``:annotated:``。
此时嵌套 QMP 代码块必须显式采用 :: 语法。

例如::

  # .. qmp-example::
  #    :annotated:
  #    :title: A more complex demonstration
  #
  #    This is a more complex example that can use
  #    ``arbitrary rST syntax`` in its exposition::
  #
  #     -> { "execute": "query-block" }
  #     <- { "return": [
  #             {
  #               "device": "ide0-hd0",
  #               ...
  #             }
  #             ... more ...
  #          ] }
  #
  #    Above, lengthy output has been omitted for brevity.


其他语言高亮可用 ``.. code-block:: lang``，省略语言则不高亮。

完整定义文档示例::

 ##
 # @BlockStats:
 #     Statistics of a virtual block device or a block backing device.
 #
 # @device: If the stats are for a virtual block device, the name
 #     corresponding to the virtual block device.
 #
 # @node-name: The node name of the device.  (Since 2.3)
 #
 # ... more members ...
 #
 # Since: 0.14
 ##
 { 'struct': 'BlockStats',
   'data': {'*device': 'str', '*node-name': 'str',
            ... more members ... } }

 ##
 # @query-blockstats:
 #     Query the @BlockStats for all virtual block devices.
 #
 # @query-nodes: If true, the command will query all the block nodes
 #     ... explain, explain ...
 #     (Since 2.3)
 #
 # Returns: A list of @BlockStats for each virtual block devices.
 #
 # Since: 0.14
 #
 # .. qmp-example::
 #
 #     -> { "execute": "query-blockstats" }
 #     <- {
 #          ...
 #        }
 ##
 { 'command': 'query-blockstats',
   'data': { '*query-nodes': 'bool' },
   'returns': ['BlockStats'] }



标记常见陷阱
~~~~~~~~~~~~

列表与段落之间必须有空行，否则列表可能不被识别，输出混乱。
正确示例::

 # An event's state is modified if:
 #
 # - its name matches the @name pattern, and
 # - if @vcpu is given, the event has the "vcpu" property.


没有空行时会被当作单个段落。

缩进很重要。错误示例::

 # @none: None (no memory side cache in this proximity domain,
 #              or cache associativity unknown)
 #     (since 5.0)


最后一行错误地减少了缩进。第二行及后续行应对齐::

 # @none: None (no memory side cache in this proximity domain,
 #     or cache associativity unknown)
 #     (since 5.0)


部分标签区分大小写，以冒号结尾，且仅在空行之后识别。正确示例::

 #
 # Since: 7.1


以下错误示例都会被当作普通段落::

 # since: 7.1

 # Since 7.1

 # Since : 7.1


成员说明同样需要冒号。正确示例::

 # @interface-id: Interface ID


以下错误示例都会被当作普通段落::

 # @interface-id   Interface ID

 # @interface-id : Interface ID


目前不会对缺少文档的成员报错，而是在生成文档中显示“Not documented”。
增加更多无文档成员前，请慎重考虑。

修改文档注释后，务必检查生成结果符合预期。


客户端 JSON 协议内省
====================

客户端通常需要知道服务器 QEMU 究竟支持什么。
QMP 通过 query-qmp-schema 提供内省，QGA 当前不支持。

虽然不同 QEMU 版本应保持线上协议兼容，但无法同样保证内省结果稳定。
例如一个版本的非变体可选结构成员，后续可能重构为某变体中的必需成员；
开放字符串类型可能变为有限枚举；特定类型可能变成在原类型和另一类型间选择的 alternate。

query-qmp-schema 返回 SchemaInfo 对象数组，共同描述 Schema 定义的线上 ABI。
返回顺序不作保证，客户端必须在整个数组中按名称搜索。
但类型、命令和事件名称保证不冲突。

SchemaInfo 无法反映 QMP 的全部规则和限制。
它用于发现接口有哪些内容，不是接口规范；规范仍是 QAPI Schema。
要理解正确用法，必须阅读 Schema。

query-qmp-schema 和 SchemaInfo 本身也定义在 Schema 中。
这里仅概述机制，详细内容以 Schema 为准。

SchemaInfo 的公共成员为 name、meta-type、features，
并根据 meta-type 带有不同变体成员。
每个对象描述一个命令、事件或某类类型。
命令和事件的名称与 Schema 相同。

命令、事件名属于线上 ABI，类型名不属于。
因此类型的 SchemaInfo 使用自动生成、没有语义的名称。
为便于阅读，本节示例改用有意义的类型名。

可选 features 以 JSON 字符串数组暴露功能名。
查看类型时，应从使用它的命令或事件出发，按名称跟随引用。
无法这样到达的 Schema 定义不会输出。

命令的 meta-type 为 command，变体成员为 arg-type、ret-type 和 allow-oob。
客户端 execute 的 arguments 必须符合 arg-type 指定的对象类型，
成功响应的 return 符合 ret-type。
allow-oob 为 true 表示支持带外，默认 false。

无参数时 arg-type 指向无成员对象；无返回数据时 ret-type 同样指向无成员对象。
例如 query-qmp-schema 的 SchemaInfo::

 { "name": "query-qmp-schema", "meta-type": "command",
   "arg-type": "q_empty", "ret-type": "SchemaInfoList" }

   Type "q_empty" is an automatic object type without members, and type
   "SchemaInfoList" is the array of SchemaInfo type.


事件的 meta-type 为 event，变体成员为 arg-type。
线上事件 data 符合 arg-type 指定的对象类型。
不携带额外数据时，arg-type 指向无成员对象，线上可能没有 data。

Schema 中 data 为 MEMBERS 对象的每个命令或事件，都会隐式定义一个对象类型。
例如 `事件 <#events>`_ 一节的 EVENT_C::

    { "name": "EVENT_C", "meta-type": "event",
      "arg-type": "q_obj-EVENT_C-arg" }

    Type "q_obj-EVENT_C-arg" is an implicitly defined object type with
    the two members from the event's definition.


结构体和联合体的 meta-type 为 object，带 members；
联合体还带 tag 和 variants。

members 是描述公共成员的 JSON 数组。
每项含 name、type，以及可选 features 和 default。
存在 default 表示成员可选，目前值只能是 null，其他值保留供扩展。
数组顺序不作保证，判断某成员是否支持时必须搜索全部内容。

例如 `结构体类型 <#struct-types>`_ 的 MyType::

    { "name": "MyType", "meta-type": "object",
      "members": [
          { "name": "member1", "type": "str" },
          { "name": "member2", "type": "int" },
          { "name": "member3", "type": "str", "default": null } ] }


features 用字符串数组暴露功能名。
例如 `功能 <#features>`_ 一节的 TestType::

    { "name": "TestType", "meta-type": "object",
      "members": [
          { "name": "number", "type": "int" } ],
      "features": ["allow-negative-numbers"] }


tag 是作为类型标签的公共成员名。
variants 是描述变体成员的数组，每项包含 case 和 type：
case 指适用的标签值，type 指提供对应变体成员的对象类型。
顺序不作保证，也不保证与 tag 枚举顺序相同。

例如 `联合体类型 <#union-types>`_ 的 BlockdevOptions::

    { "name": "BlockdevOptions", "meta-type": "object",
      "members": [
          { "name": "driver", "type": "BlockdevDriver" },
          { "name": "read-only", "type": "bool", "default": null } ],
      "tag": "driver",
      "variants": [
          { "case": "file", "type": "BlockdevOptionsFile" },
          { "case": "qcow2", "type": "BlockdevOptionsQcow2" } ] }


基础类型会展开，其成员直接包含在 members 数组中。

alternate 的 meta-type 为 alternate，members 数组的每项只有命名类型的 type 成员。
值必须恰好符合其中一个成员类型，成员顺序不作保证。

例如 `Alternate 类型 <#alternate-types>`_ 的 BlockdevRef::

    { "name": "BlockdevRef", "meta-type": "alternate",
      "members": [
          { "type": "BlockdevOptions" },
          { "type": "str" } ] }


数组的 meta-type 为 array，element-type 指定元素类型。
数组类型隐式定义，名称为方便可能类似元素名，
但客户端应读取 element-type，不能解析 name 作推断。

例如 ['str']::

    { "name": "[str]", "meta-type": "array",
      "element-type": "str" }


枚举的 meta-type 为 enum，members 数组描述各取值。
每项包含 name 及可选的功能字符串数组 features。
顺序不作保证，判断是否支持某值时需搜索整个数组。

例如 `枚举类型 <#enumeration-types>`_ 的 MyEnum::

    { "name": "MyEnum", "meta-type": "enum",
      "members": [
        { "name": "value1" },
        { "name": "value2" },
        { "name": "value3" }
      ] }


内置类型的 SchemaInfo 名称与 Schema 相同，
见 `内置类型 <#built-in-types>`_，但有下述例外。
json-type 成员表示值的线上编码。

例如 str::

    { "name": "str", "meta-type": "builtin", "json-type": "string" }


Schema 支持多个仅 C 映射不同的整数类型，
在 SchemaInfo 看来相同，因此全部映射为一个 int 类型。

类型名并非线上 ABI 的一部分，内置类型也不例外。
客户端应检查 json-type，而不是硬编码类型名。


兼容性考虑
==========

演进 Schema 时，维护客户端协议向后兼容需要谨慎。
本节讨论语法兼容，它是实际兼容的必要条件，但不是充分条件。

客户端发送带参数的命令，接收带返回数据的响应和带数据的事件。

发送方向增加选择性功能是兼容的，例如增加命令、可选参数、枚举值、
联合体或 alternate 分支，把参数类型改为包含原类型的 alternate，
或将必需参数改为可选。不知道新功能的客户端仍能工作。

不兼容修改包括删除命令、参数、枚举值或分支，增加必需参数，
以及将可选参数改为必需。

缺省可选参数的约定行为应保持不变。
适当文档仍允许一定弹性：若 buffer-size 只承诺默认采用合理大小，
实际数值可以改变，因为约定的是合理性，不是精确大小。

接收方向增加功能通常兼容，例如增加事件、返回成员或事件数据成员。
客户端应忽略不认识的内容。

删除不再可达的内容，例如不可能触发的事件、不再发送的可选成员或枚举值，
除了内省外对客户端没有影响。但内省变化可能使客户端困惑，仍需谨慎。

删除返回成员或事件数据成员是不兼容的。

命令 data 或其递归使用类型的修改，需要考虑发送方向；
命令 return、事件 data 或其递归类型的修改，需要考虑接收方向；
同时用于两者的类型，需要兼顾两个方向。

枚举值、复杂类型成员和 alternate 分支可以自由重排。
枚举和 alternate 不影响线上编码，复杂类型可能改变 JSON 对象成员输出顺序，
但协议允许这样做。

类型名线上不可见，因此可以自由重命名。
把成员抽到公共基类等某些重构也不可见。


代码生成
========

qapi-gen.py 从 Schema 生成代码和文档。
配合 QAPI 核心库，完成读取 JSON 命令、将参数解组为 C 类型、
调用 C 实现、把响应转回协议并返回用户，以及命令内省的全部工作。

以下 Schema 定义一个复杂用户类型，和接收其列表、返回其中单个元素的命令。
用户只需实现 qmp_my_command()，其余由生成器完成。

::

    $ cat example-schema.json
    { 'struct': 'UserDefOne',
      'data': { 'integer': 'int', '*string': 'str', '*flag': 'bool' } }

    { 'command': 'my-command',
      'data': { 'arg1': ['UserDefOne'] },
      'returns': 'UserDefOne' }

    { 'event': 'MY_EVENT' }


运行方式::

    $ python scripts/qapi-gen.py --output-dir="qapi-generated" \
    --prefix="example-" example-schema.json


若要更全面了解生成代码，tests/qapi-schema/qapi-schema-tests.json
覆盖更多可接受的 Schema，并在 make check-unit 中编译对应 C 代码。


为 QAPI 类型生成的代码
----------------------

生成以下文件：

 ``$(prefix)qapi-types.h``
     Schema 类型对应的 C 类型。

 ``$(prefix)qapi-types.c``
     上述类型的清理函数。

prefix 是可选命名空间参数，隔离不同 Schema 生成的代码，
使多个 Schema 的输出可以共存而不覆盖。

例如::

    $ cat qapi-generated/example-qapi-types.h
    [Uninteresting stuff omitted...]

    #ifndef EXAMPLE_QAPI_TYPES_H
    #define EXAMPLE_QAPI_TYPES_H

    #include "qapi/qapi-builtin-types.h"

    typedef struct UserDefOne UserDefOne;

    typedef struct UserDefOneList UserDefOneList;

    typedef struct q_obj_my_command_arg q_obj_my_command_arg;

    struct UserDefOne {
        int64_t integer;
        char *string;
        bool has_flag;
        bool flag;
    };

    void qapi_free_UserDefOne(UserDefOne *obj);
    G_DEFINE_AUTOPTR_CLEANUP_FUNC(UserDefOne, qapi_free_UserDefOne)

    struct UserDefOneList {
        UserDefOneList *next;
        UserDefOne *value;
    };

    void qapi_free_UserDefOneList(UserDefOneList *obj);
    G_DEFINE_AUTOPTR_CLEANUP_FUNC(UserDefOneList, qapi_free_UserDefOneList)

    struct q_obj_my_command_arg {
        UserDefOneList *arg1;
    };

    #endif /* EXAMPLE_QAPI_TYPES_H */
    $ cat qapi-generated/example-qapi-types.c
    [Uninteresting stuff omitted...]

    void qapi_free_UserDefOne(UserDefOne *obj)
    {
        Visitor *v;

        if (!obj) {
            return;
        }

        v = qapi_dealloc_visitor_new();
        visit_type_UserDefOne(v, NULL, &obj, NULL);
        visit_free(v);
    }

    void qapi_free_UserDefOneList(UserDefOneList *obj)
    {
        Visitor *v;

        if (!obj) {
            return;
        }

        v = qapi_dealloc_visitor_new();
        visit_type_UserDefOneList(v, NULL, &obj, NULL);
        visit_free(v);
    }

    [Uninteresting stuff omitted...]


对模块化 Schema，见 `包含指令 <#include-directives>`_，
各 SUBDIR/SUBMODULE.json 的代码实际输出到::

 SUBDIR/$(prefix)qapi-types-SUBMODULE.h
 SUBDIR/$(prefix)qapi-types-SUBMODULE.c


使用 --builtins 时还生成：

 ``qapi-builtin-types.h``
     内置类型对应的 C 类型。

 ``qapi-builtin-types.c``
     这些类型的清理函数。


为 QAPI 类型访问生成的代码
--------------------------

访问者函数遍历原生 QAPI C 结构，并与 QObject 等其他格式相互转换。
名称为 visit_type_FOO() 和 visit_type_FOO_members()。

生成：

 ``$(prefix)qapi-visit.c``
     各 C 类型的访问者，自动与 QObject 相互转换，也可释放已有 C 对象内存。

 ``$(prefix)qapi-visit.h``
     上述函数声明。

例如::

    $ cat qapi-generated/example-qapi-visit.h
    [Uninteresting stuff omitted...]

    #ifndef EXAMPLE_QAPI_VISIT_H
    #define EXAMPLE_QAPI_VISIT_H

    #include "qapi/qapi-builtin-visit.h"
    #include "example-qapi-types.h"


    bool visit_type_UserDefOne_members(Visitor *v, UserDefOne *obj, Error **errp);

    bool visit_type_UserDefOne(Visitor *v, const char *name,
                     UserDefOne **obj, Error **errp);

    bool visit_type_UserDefOneList(Visitor *v, const char *name,
                     UserDefOneList **obj, Error **errp);

    bool visit_type_q_obj_my_command_arg_members(Visitor *v, q_obj_my_command_arg *obj, Error **errp);

    #endif /* EXAMPLE_QAPI_VISIT_H */
    $ cat qapi-generated/example-qapi-visit.c
    [Uninteresting stuff omitted...]

    bool visit_type_UserDefOne_members(Visitor *v, UserDefOne *obj, Error **errp)
    {
        bool has_string = !!obj->string;

        if (!visit_type_int(v, "integer", &obj->integer, errp)) {
            return false;
        }
        if (visit_optional(v, "string", &has_string)) {
            if (!visit_type_str(v, "string", &obj->string, errp)) {
                return false;
            }
        }
        if (visit_optional(v, "flag", &obj->has_flag)) {
            if (!visit_type_bool(v, "flag", &obj->flag, errp)) {
                return false;
            }
        }
        return true;
    }

    bool visit_type_UserDefOne(Visitor *v, const char *name,
                     UserDefOne **obj, Error **errp)
    {
        bool ok = false;

        if (!visit_start_struct(v, name, (void **)obj, sizeof(UserDefOne), errp)) {
            return false;
        }
        if (!*obj) {
            /* incomplete */
            assert(visit_is_dealloc(v));
            ok = true;
            goto out_obj;
        }
        if (!visit_type_UserDefOne_members(v, *obj, errp)) {
            goto out_obj;
        }
        ok = visit_check_struct(v, errp);
    out_obj:
        visit_end_struct(v, (void **)obj);
        if (!ok && visit_is_input(v)) {
            qapi_free_UserDefOne(*obj);
            *obj = NULL;
        }
        return ok;
    }

    bool visit_type_UserDefOneList(Visitor *v, const char *name,
                     UserDefOneList **obj, Error **errp)
    {
        bool ok = false;
        UserDefOneList *tail;
        size_t size = sizeof(**obj);

        if (!visit_start_list(v, name, (GenericList **)obj, size, errp)) {
            return false;
        }

        for (tail = *obj; tail;
             tail = (UserDefOneList *)visit_next_list(v, (GenericList *)tail, size)) {
            if (!visit_type_UserDefOne(v, NULL, &tail->value, errp)) {
                goto out_obj;
            }
        }

        ok = visit_check_list(v, errp);
    out_obj:
        visit_end_list(v, (void **)obj);
        if (!ok && visit_is_input(v)) {
            qapi_free_UserDefOneList(*obj);
            *obj = NULL;
        }
        return ok;
    }

    bool visit_type_q_obj_my_command_arg_members(Visitor *v, q_obj_my_command_arg *obj, Error **errp)
    {
        if (!visit_type_UserDefOneList(v, "arg1", &obj->arg1, errp)) {
            return false;
        }
        return true;
    }

    [Uninteresting stuff omitted...]


模块化 Schema 的各子模块代码实际输出到::

 SUBDIR/$(prefix)qapi-visit-SUBMODULE.h
 SUBDIR/$(prefix)qapi-visit-SUBMODULE.c


使用 --builtins 时还生成：

 ``qapi-builtin-visit.h``
     内置类型访问者函数。

 ``qapi-builtin-visit.c``
     这些访问者函数的声明。


为命令生成的代码
----------------

命令编组和分派代码提供 qmp_marshal_COMMAND()，
并声明用户必须实现的 qmp_COMMAND()。

生成：

 ``$(prefix)qapi-commands.c``
     各 QMP 命令的编组和分派函数。

 ``$(prefix)qapi-commands.h``
     Schema 命令的函数原型。

 ``$(prefix)qapi-commands.trace-events``
     跟踪事件声明，见 :ref:`tracing`。

 ``$(prefix)qapi-init-commands.h``
     命令初始化函数原型。

 ``$(prefix)qapi-init-commands.c``
     命令初始化代码。

例如::

    $ cat qapi-generated/example-qapi-commands.h
    [Uninteresting stuff omitted...]

    #ifndef EXAMPLE_QAPI_COMMANDS_H
    #define EXAMPLE_QAPI_COMMANDS_H

    #include "example-qapi-types.h"

    UserDefOne *qmp_my_command(UserDefOneList *arg1, Error **errp);
    void qmp_marshal_my_command(QDict *args, QObject **ret, Error **errp);

    #endif /* EXAMPLE_QAPI_COMMANDS_H */

    $ cat qapi-generated/example-qapi-commands.trace-events
    # AUTOMATICALLY GENERATED, DO NOT MODIFY

    qmp_enter_my_command(const char *json) "%s"
    qmp_exit_my_command(const char *result, bool succeeded) "%s %d"

    $ cat qapi-generated/example-qapi-commands.c
    [Uninteresting stuff omitted...]

    void qmp_marshal_my_command(QDict *args, QObject **ret, Error **errp)
    {
        Error *err = NULL;
        bool ok = false;
        Visitor *v;
        UserDefOne *retval;
        Visitor *ov;
        q_obj_my_command_arg arg = {0};

        v = qobject_input_visitor_new_qmp(QOBJECT(args));
        if (!visit_start_struct(v, NULL, NULL, 0, errp)) {
            goto out;
        }
        if (visit_type_q_obj_my_command_arg_members(v, &arg, errp)) {
            ok = visit_check_struct(v, errp);
        }
        visit_end_struct(v, NULL);
        if (!ok) {
            goto out;
        }

        if (trace_event_get_state_backends(TRACE_QMP_ENTER_MY_COMMAND)) {
            g_autoptr(GString) req_json = qobject_to_json(QOBJECT(args));

            trace_qmp_enter_my_command(req_json->str);
        }

        retval = qmp_my_command(arg.arg1, &err);
        if (err) {
            trace_qmp_exit_my_command(error_get_pretty(err), false);
            error_propagate(errp, err);
            goto out;
        }

        ov = qobject_output_visitor_new_qmp(ret);
        if (visit_type_UserDefOne(ov, "unused", &retval, errp)) {
            visit_complete(ov, ret);
        }
        visit_free(ov);
        ov = qapi_dealloc_visitor_new();
        visit_type_UserDefOne(ov, "unused", &retval, NULL);
        visit_free(ov);

        if (trace_event_get_state_backends(TRACE_QMP_EXIT_MY_COMMAND)) {
            g_autoptr(GString) ret_json = qobject_to_json(*ret);

            trace_qmp_exit_my_command(ret_json->str, true);
        }

    out:
        visit_free(v);
        v = qapi_dealloc_visitor_new();
        visit_start_struct(v, NULL, NULL, 0, NULL);
        visit_type_q_obj_my_command_arg_members(v, &arg, NULL);
        visit_end_struct(v, NULL);
        visit_free(v);
    }

    [Uninteresting stuff omitted...]
    $ cat qapi-generated/example-qapi-init-commands.h
    [Uninteresting stuff omitted...]
    #ifndef EXAMPLE_QAPI_INIT_COMMANDS_H
    #define EXAMPLE_QAPI_INIT_COMMANDS_H

    #include "qapi/qmp-registry.h"

    void example_qmp_init_marshal(QmpCommandList *cmds);

    #endif /* EXAMPLE_QAPI_INIT_COMMANDS_H */
    $ cat qapi-generated/example-qapi-init-commands.c
    [Uninteresting stuff omitted...]
    void example_qmp_init_marshal(QmpCommandList *cmds)
    {
        QTAILQ_INIT(cmds);

        qmp_register_command(cmds, "my-command",
                             qmp_marshal_my_command, 0, 0);
    }
    [Uninteresting stuff omitted...]


模块化 Schema 的各子模块实际输出到::

 SUBDIR/$(prefix)qapi-commands-SUBMODULE.h
 SUBDIR/$(prefix)qapi-commands-SUBMODULE.c



为事件生成的代码
----------------

提供 Schema 事件对应的 qapi_event_send_EVENT()。

生成：

 ``$(prefix)qapi-events.h``
     各事件类型函数原型。

 ``$(prefix)qapi-events.c``
     发送事件的函数实现。

 ``$(prefix)qapi-emit-events.h``
     全部事件名称枚举和公共事件代码声明。

 ``$(prefix)qapi-emit-events.c``
     公共事件代码定义。

例如::

    $ cat qapi-generated/example-qapi-events.h
    [Uninteresting stuff omitted...]

    #ifndef EXAMPLE_QAPI_EVENTS_H
    #define EXAMPLE_QAPI_EVENTS_H

    #include "qapi/util.h"
    #include "example-qapi-types.h"

    void qapi_event_send_my_event(void);

    #endif /* EXAMPLE_QAPI_EVENTS_H */
    $ cat qapi-generated/example-qapi-events.c
    [Uninteresting stuff omitted...]

    void qapi_event_send_my_event(void)
    {
        QDict *qmp;

        qmp = qmp_event_build_dict("MY_EVENT");

        example_qapi_event_emit(EXAMPLE_QAPI_EVENT_MY_EVENT, qmp);

        qobject_unref(qmp);
    }

    [Uninteresting stuff omitted...]
    $ cat qapi-generated/example-qapi-emit-events.h
    [Uninteresting stuff omitted...]

    #ifndef EXAMPLE_QAPI_EMIT_EVENTS_H
    #define EXAMPLE_QAPI_EMIT_EVENTS_H

    #include "qapi/util.h"

    typedef enum example_QAPIEvent {
        EXAMPLE_QAPI_EVENT_MY_EVENT,
        EXAMPLE_QAPI_EVENT__MAX,
    } example_QAPIEvent;

    #define example_QAPIEvent_str(val) \
        qapi_enum_lookup(&example_QAPIEvent_lookup, (val))

    extern const QEnumLookup example_QAPIEvent_lookup;

    void example_qapi_event_emit(example_QAPIEvent event, QDict *qdict);

    #endif /* EXAMPLE_QAPI_EMIT_EVENTS_H */
    $ cat qapi-generated/example-qapi-emit-events.c
    [Uninteresting stuff omitted...]

    const QEnumLookup example_QAPIEvent_lookup = {
        .array = (const char *const[]) {
            [EXAMPLE_QAPI_EVENT_MY_EVENT] = "MY_EVENT",
        },
        .size = EXAMPLE_QAPI_EVENT__MAX
    };

    [Uninteresting stuff omitted...]


模块化 Schema 的各子模块实际输出到::

 SUBDIR/$(prefix)qapi-events-SUBMODULE.h
 SUBDIR/$(prefix)qapi-events-SUBMODULE.c



为内省生成的代码
----------------

生成：

 ``$(prefix)qapi-introspect.c``
     定义保存 Schema JSON 描述的字符串。

 ``$(prefix)qapi-introspect.h``
     声明该字符串。

例如::

    $ cat qapi-generated/example-qapi-introspect.h
    [Uninteresting stuff omitted...]

    #ifndef EXAMPLE_QAPI_INTROSPECT_H
    #define EXAMPLE_QAPI_INTROSPECT_H

    #include "qobject/qlit.h"

    extern const QLitObject example_qmp_schema_qlit;

    #endif /* EXAMPLE_QAPI_INTROSPECT_H */
    $ cat qapi-generated/example-qapi-introspect.c
    [Uninteresting stuff omitted...]

    const QLitObject example_qmp_schema_qlit = QLIT_QLIST(((QLitObject[]) {
        QLIT_QDICT(((QLitDictEntry[]) {
            { "arg-type", QLIT_QSTR("0"), },
            { "meta-type", QLIT_QSTR("command"), },
            { "name", QLIT_QSTR("my-command"), },
            { "ret-type", QLIT_QSTR("1"), },
            {}
        })),
        QLIT_QDICT(((QLitDictEntry[]) {
            { "arg-type", QLIT_QSTR("2"), },
            { "meta-type", QLIT_QSTR("event"), },
            { "name", QLIT_QSTR("MY_EVENT"), },
            {}
        })),
        /* "0" = q_obj_my-command-arg */
        QLIT_QDICT(((QLitDictEntry[]) {
            { "members", QLIT_QLIST(((QLitObject[]) {
                QLIT_QDICT(((QLitDictEntry[]) {
                    { "name", QLIT_QSTR("arg1"), },
                    { "type", QLIT_QSTR("[1]"), },
                    {}
                })),
                {}
            })), },
            { "meta-type", QLIT_QSTR("object"), },
            { "name", QLIT_QSTR("0"), },
            {}
        })),
        /* "1" = UserDefOne */
        QLIT_QDICT(((QLitDictEntry[]) {
            { "members", QLIT_QLIST(((QLitObject[]) {
                QLIT_QDICT(((QLitDictEntry[]) {
                    { "name", QLIT_QSTR("integer"), },
                    { "type", QLIT_QSTR("int"), },
                    {}
                })),
                QLIT_QDICT(((QLitDictEntry[]) {
                    { "default", QLIT_QNULL, },
                    { "name", QLIT_QSTR("string"), },
                    { "type", QLIT_QSTR("str"), },
                    {}
                })),
                QLIT_QDICT(((QLitDictEntry[]) {
                    { "default", QLIT_QNULL, },
                    { "name", QLIT_QSTR("flag"), },
                    { "type", QLIT_QSTR("bool"), },
                    {}
                })),
                {}
            })), },
            { "meta-type", QLIT_QSTR("object"), },
            { "name", QLIT_QSTR("1"), },
            {}
        })),
        /* "2" = q_empty */
        QLIT_QDICT(((QLitDictEntry[]) {
            { "members", QLIT_QLIST(((QLitObject[]) {
                {}
            })), },
            { "meta-type", QLIT_QSTR("object"), },
            { "name", QLIT_QSTR("2"), },
            {}
        })),
        QLIT_QDICT(((QLitDictEntry[]) {
            { "element-type", QLIT_QSTR("1"), },
            { "meta-type", QLIT_QSTR("array"), },
            { "name", QLIT_QSTR("[1]"), },
            {}
        })),
        QLIT_QDICT(((QLitDictEntry[]) {
            { "json-type", QLIT_QSTR("int"), },
            { "meta-type", QLIT_QSTR("builtin"), },
            { "name", QLIT_QSTR("int"), },
            {}
        })),
        QLIT_QDICT(((QLitDictEntry[]) {
            { "json-type", QLIT_QSTR("string"), },
            { "meta-type", QLIT_QSTR("builtin"), },
            { "name", QLIT_QSTR("str"), },
            {}
        })),
        QLIT_QDICT(((QLitDictEntry[]) {
            { "json-type", QLIT_QSTR("boolean"), },
            { "meta-type", QLIT_QSTR("builtin"), },
            { "name", QLIT_QSTR("bool"), },
            {}
        })),
        {}
    }));

    [Uninteresting stuff omitted...]
