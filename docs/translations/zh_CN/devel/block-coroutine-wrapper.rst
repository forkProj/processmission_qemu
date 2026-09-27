=======================
block-coroutine-wrapper
=======================

QEMU 块层（见 block/*）的许多函数只能在协程上下文调用，
通常以 coroutine_fn 标记。但有时需要从非协程上下文调用它们：
先启动协程，在其中运行目标函数，再通过 BDRV_POLL_WHILE() 循环等待结束。
运行协程需要一个只有 void* 参数的函数。
因此，为每个需要非协程接口的 coroutine_fn 函数，都要定义结构体封装参数，
另写函数解包并调用原函数，最后定义与原函数参数列表相同的接口函数，
负责打包参数、创建并运行协程，以及在 BDRV_POLL_WHILE() 中等待。
手写这些封装很繁琐，因此提供脚本自动生成。

用法
========

假设已定义 ``coroutine_fn`` 函数 ``bdrv_co_foo(<some args>)``，
需要名为 ``bdrv_foo(<same args>)`` 的非协程接口，可以用脚本生成。
触发生成的方法如下：

1. 在某处（例如 block/coroutines.h）提供带 ``co_wrapper`` 标记的
   bdrv_foo 声明：

.. code-block:: c

    int co_wrapper bdrv_foo(<some args>);

2. 将声明传给 block-coroutine-wrapper 脚本。
   在 block/meson.build 的 block_gen_c 目标定义中，
   将含声明的 .h 或 .c 文件加入 ``input: files(...)`` 列表。

这样就完成了。构建期间，协程封装会生成到 ``<BUILD_DIR>/block/block-gen.c``。

相关位置
========

1. 脚本位于 scripts/block-coroutine-wrapper.py。

2. 私有 co_wrapper 声明通常放在 block/coroutines.h，
   公共声明放在 include/block/block.h。

3. 生成的协程封装所用核心 API 位于 block/block-gen.h，
   该头文件本身不是生成文件。
