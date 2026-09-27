如何编写监视器命令
==================

本文逐步介绍如何使用 QAPI 框架编写新的 QMP 命令，以及如何编写 HMP 命令。

本文不讨论 QMP 协议层细节，也不深入 QAPI 框架实现。

QAPI 框架的深入介绍见 :doc:`qapi-code-gen`。QMP 协议见 :doc:`/interop/qmp-spec`。

新命令可以只在 QMP 中实现。新的 HMP 命令应基于 QMP 实现。典型的 HMP 命令封装对应的 QMP 命令，也可以组合 QMP 基础操作，构建方便使用的 HMP 命令。长期目标是让所有现有 HMP 命令都遵循这种方式，将 HMP 与 QEMU 内部实现完全隔离。对于以往仅在 HMP 中实现的命令，进一步指导见 `编写返回非结构化文本的调试辅助命令 <#writing-a-debugging-aid-returning-unstructured-text>`_ 一节。

概述
--------

一般而言，编写新 QMP 命令需要以下步骤。

1. 在适当的 QAPI 模式模块中定义命令及所需类型。

2. 编写 QMP 命令本身，它是一个普通 C 函数。最好由某个 QEMU 子系统导出，也可以添加到 monitor/qmp-cmds.c 文件中。

3. 此时即可通过 QMP 协议测试命令。

4. 编写对应的 HMP 命令。这一步不是必需的，只有在 HMP 中提供该功能确实合理时才应实施。HMP 命令通过 QMP 命令实现。

以下各节演示上述步骤，从最简单的示例逐步深入。


测试
--------

以下各节示例均使用这里介绍的相同测试环境。

首先，如下启动 QEMU::

 # qemu-system-TARGET [...] \
     -chardev socket,id=chrqmp0,port=4444,host=localhost,server=on \
     -object monitor-qmp,chardev=chrqmp0,pretty=on,id=qmp0


然后在另一个终端中执行::

 $ telnet localhost 4444
 Trying 127.0.0.1...
 Connected to localhost.
 Escape character is '^]'.
 {
     "QMP": {
         "version": {
             "qemu": {
                 "micro": 50,
                 "minor": 2,
                 "major": 8
             },
             "package": ...
         },
         "capabilities": [
             "oob"
         ]
     }
 }


以上输出是 QMP 服务器表示连接成功。服务器此时处于能力协商模式。输入以下内容进入命令模式::

 { "execute": "qmp_capabilities" }


服务器应回复::

 {
     "return": {
     }
 }


这在 QMP 中表示“上一条命令执行成功，没有返回任何数据”。现在即可输入以下各节说明的 QMP 示例命令。


编写简单命令：hello-world
-------------------------

这是能编写的最简单的 QMP 命令。通常这类命令会在 QEMU 中执行有意义的操作，但这里仅向标准输出打印 "Hello, world"。

命令名为 "hello-world"，不接受参数，也不返回数据。

第一步是在适当的 QAPI 模式模块中定义命令。选择 qapi/misc.json 模块，在底部添加以下一行::

 ##
 # @hello-world:
 #
 # Since: 9.0
 ##
 { 'command': 'hello-world' }


"command" 关键字定义新的 QMP 命令，指示 QAPI 生成原型，以及封送和解封送协议数据所需的代码。

下一步编写 "hello-world" 的实现。如前所述，命令最好位于 QEMU 子系统中，但 "hello-world" 不属于任何子系统，因此将其实现放到 monitor/qmp-cmds.c 中::

 void qmp_hello_world(Error **errp)
 {
     printf("Hello, world!\n");
 }


有几点需要注意：

1. QMP 命令实现函数必须以 ``qmp_`` 为前缀。
2. qmp_hello_world() 返回 void，与命令不返回任何数据的定义一致。
3. 它接受 ``Error **`` 参数，这是必需的。后面会介绍如何返回错误以及接受更多参数。如果命令不返回错误，就不应修改 Error 参数。
4. 不需要添加函数原型，QAPI 会自动生成。
5. 不鼓励 QMP 命令向终端打印内容；这里这样做，是因为这是演示 QMP 命令最简单的方式。

至此已完成。构建 QEMU，按照“测试”一节的方法运行，然后输入以下 QMP 命令::

 { "execute": "hello-world" }


检查运行 QEMU 的终端中是否出现 "Hello, world" 字符串。如果没有，说明某个环节出了问题。


参数
~~~~~~~~

为 "hello-world" 命令添加参数。

首先，将模式文件中的命令定义改为如下内容::

 ##
 # @hello-world:
 #
 # @message: message to be printed (default: "Hello, world!")
 #
 # @times: how many times to print the message (default: 1)
 #
 # Since: 9.0
 ##
 { 'command': 'hello-world',
   'data': { '*message': 'str', '*times': 'int' } }


注意模式中新增的 'data' 成员。它指定 QAPI 类型为 'str' 的参数 'message'，以及 QAPI 类型为 'int' 的参数 'times'。星号用于将参数标记为可选。

然后更新 monitor/qmp-cmds.c 中的 C 实现::

 void qmp_hello_world(const char *message, bool has_times, int64_t times,
                      Error **errp)
 {
     if (!message) {
         message = "Hello, world";
     }
     if (!has_times) {
         times = 1;
     }

     for (int i = 0; i < times; i++) {
         printf("%s\n", message);
     }
 }


有两个重要细节：

1. 除指针以外的可选参数会配有 ``has_`` 布尔值。提供该可选参数时为真，否则为假。
2. C 实现的函数签名必须遵循模式中由 "data" 成员定义的参数顺序。

现在测试新版 "hello-world" 命令。构建 QEMU，按“测试”一节运行，然后发送两条命令::

 { "execute": "hello-world" }
 {
     "return": {
     }
 }

 { "execute": "hello-world", "arguments": { "message": "We love QEMU" } }
 {
     "return": {
     }
 }


运行 QEMU 的终端中应出现 "Hello, world" 和 "We love QEMU"。如果没有这些字符串，说明出了问题。


错误
~~~~~~~~

QMP 命令应使用 error.h 头文件导出的错误接口。大多数错误通过调用 error_setg() 设置。

假设不允许 "message" 字符串包含单词 "love"。如果包含，就让 "hello-world" 命令返回错误::

 void qmp_hello_world(const char *message, Error **errp)
 {
     if (message) {
         if (strstr(message, "love")) {
             error_setg(errp, "the word 'love' is not allowed");
             return;
         }
         printf("%s\n", message);
     } else {
         printf("Hello, world\n");
     }
 }


error_setg() 的第一个参数是指向 Error 指针的指针，所有 QMP 函数都会收到它。下一个参数是供人阅读的错误描述，采用类似 printf 的自由格式字符串。

测试上述示例：构建 QEMU，按“测试”一节运行，再发送以下命令::

 { "execute": "hello-world", "arguments": { "message": "all you need is love" } }


QMP 服务器应回复::

 {
     "error": {
         "class": "GenericError",
         "desc": "the word 'love' is not allowed"
     }
 }


注意，error_setg() 生成 "GenericError" 类别。通常所有 QMP 错误都应属于此类别，只有两种例外：

 1. 管理应用需要识别特定错误，以进行特殊处理。

 2. 向后兼容性要求。

如果要报告的失败属于上述两种情况之一，请使用 error_set()，并将第二个参数设为 ErrorClass 值。


实现 HMP 命令
~~~~~~~~~~~~~

有了 QMP 命令后，也可以在人类监视器（HMP）中提供它。

引入 QAPI 后，HMP 命令会调用 QMP。大多数 HMP 命令只是简单封装。

以下是 "hello-world" HMP 命令的实现::

 void hmp_hello_world(Monitor *mon, const QDict *qdict)
 {
     const char *message = qdict_get_try_str(qdict, "message");
     Error *err = NULL;

     qmp_hello_world(!!message, message, &err);
     if (hmp_handle_error(mon, err)) {
         return;
     }
 }


将其加入 monitor/hmp-cmds.c，同时把原型加入 include/monitor/hmp.h。

有四点需要注意：

1. 所有 HMP 函数都必须接受 "mon" 和 "qdict" 参数。前者是监视器对象，后者用于将用户输入的参数从监视器传给命令实现。
2. 这里选择不在 HMP 中支持 "times" 参数。
3. hmp_hello_world() 执行错误检查。本例只调用 hmp_handle_error() 向用户打印消息，但也可以进行更多处理，例如根据 qmp_hello_world() 返回的错误采取不同操作。
4. 在调用 QMP 之前，必须将 "err" 变量初始化为 NULL。

最后还需将命令加入 hmp-commands.hx，才能真正供监视器用户使用::

    {
        .name       = "hello-world",
        .args_type  = "message:s?",
        .params     = "hello-world [message]",
        .help       = "Print message to the standard output",
        .cmd        = hmp_hello_world,
    },

 SRST
 ``hello_world`` *message*
   Print message to the standard output
 ERST


测试时，打开用户监视器并执行 "hello-world" 命令。使用 HMP 的 "help" 命令查看其文档也有助于理解。

如何打开用户监视器，请查看 "-monitor" 命令行选项。


编写更复杂的命令
----------------

QMP 命令可以返回 QAPI 支持的任何数据，例如整数、字符串、布尔值、枚举和用户定义类型。

本节关注用户定义类型，其他类型的信息请参阅 QAPI 文档。


在 QAPI 中建模数据
~~~~~~~~~~~~~~~~~~

对于希望保持稳定并长期支持的 QMP 命令，返回数据必须使用细粒度 QAPI 类型显式建模。一般原则是，QMP 命令的调用方不应需要解析某个返回字段的内容。如果某字段看起来需要解析，就应将其拆分为对应不同数据项的独立字段。供机器使用、而非仅供人工操作的新 QMP 命令，通常都应遵循此要求。

不过，有些 QMP 命令仅用于给操作人员提供临时调试辅助。虽然可能返回大量格式化数据，但并不预期机器需要解析结果。为这些数据定义细粒度 QAPI 类型的成本，可能超过潜在收益。此时允许命令返回包含格式化数据的简单字符串，但必须将命令标记为不稳定。这表示不保证该命令长期稳定，将来可能改变，并且未遵循 QAPI 设计最佳实践。例如，QMP 命令 "x-query-registers" 就采用这种方式，返回特定架构 CPU 状态的格式化转储。数据格式因 QEMU 目标而异，也可能随时间变化，机器只能将其视为不透明字符串。示例见 `编写返回非结构化文本的调试辅助命令 <#writing-a-debugging-aid-returning-unstructured-text>`_ 一节。

用户定义类型
~~~~~~~~~~~~

本例编写 query-option-roms 命令，返回加载到选项 ROM 空间中的 ROM 信息。更多信息请查看 "-option-rom" 命令行选项。

对于每个选项 ROM，要返回两项信息：ROM 镜像文件名，以及 bootindex（如果有）。为此需要创建新的 QAPI 类型，如下所示::

 ##
 # @OptionRomInfo:
 #
 # @filename: option ROM image file name
 #
 # @bootindex: option ROM's bootindex
 #
 # Since: 9.0
 ##
 { 'struct': 'OptionRomInfo',
   'data': { 'filename': 'str', '*bootindex': 'int' } }


"struct" 关键字定义新的 QAPI 类型，其 "data" 成员包含类型的成员。本例中的成员是 "filename" 和 "bootindex"，后者可选。

接着定义 query-option-roms 命令::

 ##
 # @query-option-roms:
 #
 # Query information on ROMs loaded into the option ROM space.
 #
 # Returns: OptionRomInfo
 #
 # Since: 9.0
 ##
 { 'command': 'query-option-roms',
   'returns': ['OptionRomInfo'] }


注意 "returns" 关键字，顾名思义，它定义命令返回的数据。

``['OptionRomInfo']`` 语法表示“返回 OptionRomInfo 列表”。

现在实现 qmp_query_option_roms() 函数，将以下代码加入 monitor/qmp-cmds.c::

 OptionRomInfoList *qmp_query_option_roms(Error **errp)
 {
     OptionRomInfoList *info_list = NULL;
     OptionRomInfoList **tailp = &info_list;
     OptionRomInfo *info;

     for (int i = 0; i < nb_option_roms; i++) {
         info = g_malloc0(sizeof(*info));
         info->filename = g_strdup(option_rom[i].name);
         info->has_bootindex = option_rom[i].bootindex >= 0;
         if (info->has_bootindex) {
             info->bootindex = option_rom[i].bootindex;
         }
         QAPI_LIST_APPEND(tailp, info);
     }

     return info_list;
 }


有几点需要注意：

1. OptionRomInfo 类型由 QAPI 框架自动生成，其成员对应模式文件中的类型定义。
2. 同样会生成 OptionRomInfoList 类型，它是单向链表。
3. 按模式文件的定义，函数返回 OptionRomInfoList，不接受参数（所有 QMP 函数必需的 "errp" 除外）。
4. 返回的对象动态分配。
5. 所有字符串都动态分配，因为 QAPI 还会生成释放类型的函数，而它无法区分动态分配和静态分配的字符串。
6. 还记得 "bootindex" 是可选的吗？作为非指针的可选成员，它带有 'has_bootindex' 成员，需要由实现设置，如上所示。

现在测试新命令。构建 QEMU，按“测试”一节运行，然后尝试::

 { "execute": "query-option-rom" }
 {
     "return": [
         {
             "filename": "kvmvapic.bin"
         }
     ]
 }



HMP 命令
~~~~~~~~

以下是 query-option-roms 对应的 HMP 命令::

 void hmp_info_option_roms(Monitor *mon, const QDict *qdict)
 {
     Error *err = NULL;
     OptionRomInfoList *info_list, *tail;
     OptionRomInfo *info;

     info_list = qmp_query_option_roms(&err);
     if (hmp_handle_error(mon, err)) {
         return;
     }

     for (tail = info_list; tail; tail = tail->next) {
         info = tail->value;
         monitor_printf(mon, "%s", info->filename);
         if (info->has_bootindex) {
             monitor_printf(mon, " %" PRId64, info->bootindex);
         }
         monitor_printf(mon, "\n");
     }

     qapi_free_OptionRomInfoList(info_list);
 }


需要特别注意，hmp_info_option_roms() 调用 qapi_free_OptionRomInfoList()，释放 qmp_query_option_roms() 返回的数据。对于用户定义类型，QAPI 会生成 qapi_free_QAPI_TYPE_NAME() 函数，必须用它释放所定义的类型；列表类型则使用 qapi_free_QAPI_TYPE_NAMEList()（下一节介绍）。如果 QMP 函数返回字符串，应使用 g_free() 释放。

另请注意，hmp_info_option_roms() 执行了错误处理。如果确信 QMP 函数不会返回错误，则并非必须这样做，也可以传入 &error_abort。

另一个重要细节是，HMP 的 "info" 命令应放在 hmp-commands-info.hx 中，而不是 hmp-commands.hx。"info option-roms" 条目如下::

     {
         .name       = "option-roms",
         .args_type  = "",
         .params     = "",
         .help       = "show roms",
         .cmd        = hmp_info_option_roms,
     },
 SRST
 ``info option-roms``
   Show the option ROMs.
 ERST


测试时，运行 QEMU 并在用户监视器中输入 "info option-roms"。


编写返回非结构化文本的调试辅助命令
----------------------------------

如 `在 QAPI 中建模数据 <#modelling-data-in-qapi>`_ 一节所述，预期供机器使用的命令必须采用细粒度 QAPI 数据类型。唯一例外是专门用于调试辅助的命令，此时允许返回非结构化文本，例如查询 QEMU 内部状态某些方面的命令，其结果仅对操作人员有用。

本例考察 qapi/machine.json 中已有的 QMP 命令 ``x-query-roms``。它没有参数，返回 ``HumanReadableText``::

 ##
 # @x-query-roms:
 #
 # Query information on the registered ROMS
 #
 # Features:
 #
 # @unstable: This command is meant for debugging.
 #
 # Returns: registered ROMs
 #
 # Since: 6.2
 ##
 { 'command': 'x-query-roms',
   'returns': 'HumanReadableText',
   'features': [ 'unstable' ] }


``HumanReadableText`` 在 qapi/common.json 中定义，是包含一个字符串成员的结构。所有返回面向人类的非结构化文本的命令都应使用它，并且都应具有 'unstable' 特性。注意，该特性的文档会说明命令为何不稳定。通常使用 ``x-`` 命令名前缀，让用户直观看出其缺乏稳定性。

实现 QMP 命令
~~~~~~~~~~~~~

QMP 实现通常创建 ``GString`` 对象，并向其中打印格式化数据，例如::

 HumanReadableText *qmp_x_query_roms(Error **errp)
 {
     g_autoptr(GString) buf = g_string_new("");
     Rom *rom;

     QTAILQ_FOREACH(rom, &roms, next) {
        g_string_append_printf("%s size=0x%06zx name=\"%s\"\n",
                               memory_region_name(rom->mr),
                               rom->romsize,
                               rom->name);
     }

     return human_readable_text_from_str(buf);
 }


实际实现会输出更多信息，可在 hw/core/loader.c 中查看。


实现 HMP 命令
~~~~~~~~~~~~~

有了 QMP 命令后，即可像前面的示例一样，在人类监视器（HMP）中提供该功能。这些 HMP 实现都很相似，只需调用 QMP 命令，然后打印返回文本或错误消息。以下是 "info roms" HMP 命令的实现::

 void hmp_info_roms(Monitor *mon, const QDict *qdict)
 {
     Error err = NULL;
     g_autoptr(HumanReadableText) info = qmp_x_query_roms(&err);

     if (hmp_handle_error(mon, err)) {
         return;
     }
     monitor_puts(mon, info->human_readable_text);
 }


还必须将函数原型加入 hmp.h。

最后，将命令加入 hmp-commands-info.hx，才能供监视器用户使用::

    {
        .name       = "roms",
        .args_type  = "",
        .params     = "",
        .help       = "show roms",
        .cmd        = hmp_info_roms,
    },


编写调用无参数 QMP 查询命令的 HMP info 处理程序是一种常见情形。为简化实现，已有适用于此场景的通用 HMP info 处理程序。若要通过 HMP 暴露无参数 QMP 查询命令，只需在声明中让 '.cmd_info_hrt' 字段指向 QMP 处理程序，并让 '.cmd' 字段保持 NULL::

    {
        .name         = "roms",
        .args_type    = "",
        .params       = "",
        .help         = "show roms",
        .cmd_info_hrt = qmp_x_query_roms,
    },


实际的 HMP 命令就是这样实现的。
