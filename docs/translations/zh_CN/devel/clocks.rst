在 QEMU 中建模时钟树
==============================

什么是时钟？
----------------

时钟是为建模 QEMU 中时钟分发而开发的 QOM 对象。
可以描述平台的时钟分发，并检测时钟树配置错误，
例如 PLL 配置错误、时钟源选择错误或时钟被禁用。

对象为 Clock，QOM 名称是 ``clock``，C 宏为 ``TYPE_CLOCK``。

时钟通常用于建模设备输入和输出，创建方式类似 GPIO。
不同设备的输入和输出可以相互连接。

这种情况下 Clock 是 Device 的子对象，但这不是强制要求。
时钟可以独立于设备，例如在任何设备之外创建时钟，表示整机主时钟源。

时钟示例::

    +---------+      +----------------------+   +--------------+
    | 时钟1   |      |       设备 B         |   |   设备 C     |
    |         |      | +-------+  +-------+ |   | +-------+    |
    |         |>>-+-->>|时钟2  |  |时钟3  |>>--->>|时钟6  |    |
    +---------+   |  | | (输入)|  | (出)  | |   | | (输入)|    |
                  |  | +-------+  +-------+ |   | +-------+    |
                  |  |            +-------+ |   +--------------+
                  |  |            |时钟4  |>>
                  |  |            | (出)  | |   +--------------+
                  |  |            +-------+ |   |   设备 D     |
                  |  |            +-------+ |   | +-------+    |
                  |  |            |时钟5  |>>--->>|时钟7  |    |
                  |  |            | (出)  | |   | | (输入)|    |
                  |  |            +-------+ |   | +-------+    |
                  |  +----------------------+   |              |
                  |                             | +-------+    |
                  +----------------------------->>|时钟8  |    |
                                                | | (输入)|    |
                                                | +-------+    |
                                                +--------------+


时钟定义位于 ``include/hw/core/clock.h``，
设备相关函数位于 ``include/hw/core/qdev-clock.h``。

时钟状态
---------------

时钟状态即周期，以整数保存，单位为 2 :sup:`-32` ns。
特殊值 0 表示时钟不活动或被门控关闭。
时钟不建模信号本身的引脚翻转，也不建模占空比等属性。

输入和输出时钟都包含此状态，因此可随时查询当前周期。
更新时钟时，新值立即传播到时钟树中全部连接的时钟。

为便于使用，各状态读写函数都提供带单位后缀的辅助版本：

- ``_ns``：以纳秒处理周期。
- ``_hz``：以赫兹处理频率。

周期 0 转换为频率 0，反之亦然。0 始终表示时钟已禁用。

添加新时钟
------------------

必须在 Device 实例的 init 方法中添加时钟。

使用 ``qdev_init_clock_in()`` 添加输入时钟，
参数包括名称、回调、回调的不透明参数，以及应触发回调的事件掩码，详见后文。
输出时钟更简单，只需名称。典型用法::

    qdev_init_clock_in(DEVICE(dev), "clk_in", clk_in_callback, dev, ClockUpdate);
    qdev_init_clock_out(DEVICE(dev), "clk_out");


两个函数都返回创建的 Clock 指针，应保存在设备状态结构中以供后续使用。
QOM 引用机制会自动删除这些对象。

也可以用静态数组描述时钟输入和输出，再将数组传给 ``qdev_init_clocks()`` 初始化。
效果等同于对每项分别调用 ``qdev_init_clock_in/out()``。
``include/hw/core/qdev-clock.h`` 提供辅助宏，便于构造数组。
以下为设备创建一个输入时钟和一个输出时钟：

.. code-block:: c

    /* device structure containing pointers to the clock objects */
    typedef struct MyDeviceState {
        DeviceState parent_obj;
        Clock *clk_in;
        Clock *clk_out;
    } MyDeviceState;

    /*
     * callback for the input clock (see "Callback on input clock
     * change" section below for more information).
     */
    static void clk_in_callback(void *opaque, ClockEvent event);

    /*
     * static array describing clocks:
     * + a clock input named "clk_in", whose pointer is stored in
     *   the clk_in field of a MyDeviceState structure with callback
     *   clk_in_callback.
     * + a clock output named "clk_out" whose pointer is stored in
     *   the clk_out field of a MyDeviceState structure.
     */
    static const ClockPortInitArray mydev_clocks = {
        QDEV_CLOCK_IN(MyDeviceState, clk_in, clk_in_callback, ClockUpdate),
        QDEV_CLOCK_OUT(MyDeviceState, clk_out),
        QDEV_CLOCK_END
    };

    /* device initialization function */
    static void mydev_init(Object *obj)
    {
        /* cast to MyDeviceState */
        MyDeviceState *mydev = MYDEVICE(obj);
        /* create and fill the pointer fields in the MyDeviceState */
        qdev_init_clocks(mydev, mydev_clocks);
        [...]
    }


另一种创建方式是直接调用 ``object_new(TYPE_CLOCK)``，
此时它不属于设备的输入或输出。
完整 QOM 层级建立后，应调用 ``clock_setup_canonical_path()``。

新建时钟周期为 0，即禁用，可通过 ``clock_set_ns()`` 或 ``clock_set_hz()`` 修改。

如果时钟周期固定且永不改变，例如开发板主时钟源，设置后就无需其他操作。
连接时钟时，此值会传播到其他时钟，设备在首次复位时读取正确值。

时钟回调
---------------

可通过以下方式设置回调：

 * 作为 ``qdev_init_clock_in()`` 参数传入。
 * 作为 ``QDEV_CLOCK_IN()`` 参数，初始化传给 ``qdev_init_clocks()`` 的数组。
 * 直接调用 ``clock_set_callback()``。

回调类型必须为：

.. code-block:: c

   typedef void ClockCallback(void *opaque, ClockEvent event);


``opaque`` 是此前传给 ``qdev_init_clock_in()`` 或 ``clock_set_callback()`` 的指针；
使用 ``qdev_init_clocks()`` 时则为 ``dev`` 设备指针。

``event`` 表示调用原因。
注册时指定关注的 ClockEvent 值掩码，只有这些事件会触发回调。

当前支持：

 * ``ClockPreUpdate``：输入时钟周期即将更新时调用。
   若设备操作需要知道旧周期，此事件很有用。
   回调期间 ``clock_get()``、``clock_ticks_to_ns()`` 等 API 使用旧周期。
 * ``ClockUpdate``：输入时钟周期改变后调用。
   回调期间 ``clock_ticks_to_ns()`` 等 API 使用新周期。

一个时钟只能有一个回调，不能为不同事件注册不同函数。
必须让同一回调监听所有关注事件，再通过 event 区分。

从设备获取时钟
-------------------------------

``qdev_get_clock_in()`` 和 ``dev_get_clock_out()``
可用于获取设备输入或输出时钟。例如：

.. code-block:: c

   Clock *clk = qdev_get_clock_in(DEVICE(mydev), "clk_in");


或者：

.. code-block:: c

   Clock *clk = qdev_get_clock_out(DEVICE(mydev), "clk_out");


连接两个时钟
------------------------------

使用 ``clock_set_source()`` 连接两个时钟。
``clock_set_source(clk2, clk1);`` 使 clk2 跟随 clk1 的周期变化，
每次更新 clk1 时也更新 clk2。

连接不同设备的时钟时，优先用 ``qdev_connect_clock_in()`` 设置输入时钟源。
例如将 devB 的 clk2 输入连接到 devA 的 clk1 输出：

.. code-block:: c

    qdev_connect_clock_in(devB, "clk2", qdev_get_clock_out(devA, "clk1"))


这里使用了 qdev_get_clock_out()，但任意时钟都能驱动输入时钟，包括另一个输入时钟。
下图展示一些连接方式；一个时钟也可以驱动多个其他时钟。

::

  +------------+  +--------------------------------------------------+
  |  设备 A    |  |                   设备 B                         |
  |            |  |               +---------------------+            |
  |            |  |               |       设备 C        |            |
  |  +-------+ |  | +-------+     | +-------+ +-------+ |  +-------+ |
  |  |时钟1  |>>-->>|时钟2  |>>+-->>|时钟3  | |时钟5  |>>>>|时钟6  |>>
  |  | (出)  | |  | | (输入)|  |  | | (输入)| | (出)  | |  | (出)  | |
  |  +-------+ |  | +-------+  |  | +-------+ +-------+ |  +-------+ |
  +------------+  |            |  +---------------------+            |
                  |            |                                     |
                  |            |  +--------------+                   |
                  |            |  |   设备 D     |                   |
                  |            |  | +-------+    |                   |
                  |            +-->>|时钟4  |    |                   |
                  |               | | (输入)|    |                   |
                  |               | +-------+    |                   |
                  |               +--------------+                   |
                  +--------------------------------------------------+


上例中 Device A 更新 Clock 1 后，Clock 2、Clock 3 和 Clock 4 都获得新周期。

时钟连接后，不能断开或更改连接关系。

时钟乘数与除数设置
-------------------------------------

默认情况下，连接的子时钟与其源（父）时钟周期相同。
Clock API 内置周期乘除机制，可让子时钟采用不同周期。
调用 ``clock_set_mul_div()`` 指定乘数和除数后，
全部子时钟的周期为 ``parent_period * multiplier / divider``。
例如父时钟为 8 MHz，乘数设为 2、除数设为 3，则子时钟为 12 MHz。

运行时可修改乘除数，用于建模客户机可编程倍频或分频的时钟控制器。

与 clock_set() 类似，clock_set_mul_div() 修改状态后返回 true，
即乘数、除数或两者发生变化时返回 true。

它不会自动调用 ``clock_propagate()``。
运行时修改乘除数后，必须自行调用该函数传播变化。

未连接的输入时钟
------------------------

新建输入时钟被禁用，周期为 0，直到更新周期前都视为禁用。
一直未连接的时钟会保持初始值 0。
若不希望如此，应在设备实例初始化时调用 clock_set()、clock_set_ns()
或 clock_set_hz()。例如：

.. code-block:: c

    clk = qdev_init_clock_in(DEVICE(dev), "clk-in", clk_in_callback,
                             dev, ClockUpdate);
    /* set initial value to 10ns / 100MHz */
    clock_set_ns(clk, 10);


要强制要求开发板代码连接时钟，可在设备 realize 方法中检查 ``clock_has_source()``：

.. code-block:: c

   if (!clock_has_source(s->clk)) {
       error_setg(errp, "MyDevice: clk input must be connected");
       return;
   }


这只检查是否建立连接；连接的输出仍可能被禁用或尚未配置，周期仍为 0。
应通过时钟回调得知周期变化。

获取时钟频率或周期
-------------------------------

使用 ``clock_get()`` 或 ``clock_get_hz()`` 获取当前状态。

clock_get() 返回完整精度的内部周期，以无符号 64 位整数表示，单位为 2^-32 纳秒。
很多情况下 clock_ticks_to_ns() 更方便，见下文到期时间计算。

clock_get_hz() 返回向下取整的频率，因此存在舍入误差，用于计算时应谨慎。

也可为频率变化注册回调。
以下假设 clock_callback 已注册为 ClockUpdate 回调：

.. code-block:: c

    void clock_callback(void *opaque, ClockEvent event) {
        MyDeviceState *s = (MyDeviceState *) opaque;
        /*
         * 'opaque' is the argument passed to qdev_init_clock_in();
         * usually this will be the device state pointer.
         */

        /* do something with the new period */
        fprintf(stdout, "device new period is %" PRIu64 "* 2^-32 ns\n",
                        clock_get(dev->my_clk_input));
    }


若仅为向人显示频率，例如调试，应使用 ``clock_display_freq()``，
它返回“33.3 MHz”之类的格式化字符串，使用后调用者必须用 g_free() 释放。

QTest 也可通过 QMP 访问 QOM 属性 ``qtest-clock-period`` 获取周期。
此属性仅在 qtest 加速器下存在，普通 QEMU 运行时不可用。

计算到期时间
----------------------------

常见需求是计算时钟经过 N 个节拍需要多久，用于设置定时器到期时间。
``clock_ticks_to_ns()`` 接收无符号 64 位节拍数，返回所需纳秒数。

不要简单地把“纳秒周期”乘以节拍数，因为周期可能不是整数纳秒，
累积误差可能很大。

周期很长且节拍数很大时，结果理论上可能超出 64 位。
为避免溢出，clock_ticks_to_ns() 会把结果饱和到 INT64_MAX，
这是 QEMUTimer API 接受的最大有效值。
INT64_MAX 纳秒接近 300 年，因此更晚的到期时间可视为永不发生。
调用者通常不应特殊处理饱和结果，而应直接把定时器设置到这个遥远的未来。
当然，若还要对返回值继续计算，而不只是传给 timer_mod_ns() 等 QEMUTimer 函数，
就需要防止后续计算溢出。

获取节拍数
---------------------

需要计算某段时间内节拍数时，使用 ``clock_ns_to_ticks()``。
它处理非整数纳秒周期，避免舍入误差。
时钟停止，即周期为 0 时返回 0。
若输入导致节拍数溢出 64 位，例如极短周期和极长时长，
结果会截断，相当于 64 位输出回绕。

修改时钟周期
-----------------------

设备可用 ``clock_update()``、``clock_update_ns()`` 或 ``clock_update_hz()``
修改输出，并触发全部已连接输入更新。

例如，初始化时创建 clkout 输出，并在设备状态中保存指针：

.. code-block:: c

   dev->clkout = qdev_init_clock_out(DEVICE(dev), "clkout");


之后除下述情形外，可以随时修改：

.. code-block:: c

   clock_update_hz(dev->clkout, 1000 * 1000 * 1000); /* 1GHz */


更新可能通过已连接时钟和回调触发副作用，因此必须持有 QEMU I/O 锁。

同理，只有允许对其他对象产生副作用时才能更新。因此以下阶段禁止更新：

* 迁移期间；
* 复位的 enter 阶段。

``clock_update[_ns|_hz]()`` 等价于使用相同参数调用 ``clock_set[_ns|_hz]()``，
再调用 clock_propagate()。
因此可将状态设置与副作用触发分离，便于复用处理设备复位和迁移的代码。

时钟别名
---------------

有时需要转发或继承其他设备的时钟。
例如组合设备时，希望直接暴露子设备时钟而不干预它。
``qdev_alias_clock()`` 提供此功能，也可用不同名称暴露，适用于输入和输出。

例如 B 是 A 的子设备，device_a_instance_init() 可以这样做：

.. code-block:: c

    void device_a_instance_init(Object *obj)
    {
        AState *A = DEVICE_A(obj);
        BState *B;
        /* create object B as child of A */
        [...]
        qdev_alias_clock(B, "clk", A, "b_clk");
        /*
         * Now A has a clock "b_clk" which is an alias to
         * the clock "clk" of its child B.
         */
    }


函数不返回时钟对象，新时钟方向与原时钟相同，只增加指向已有时钟的链接。
上例中 B 仍是唯一可以操作此时钟的对象，A 不得修改周期或设置回调。
输入时钟的情况如下::

    +--------------------------+
    |        设备 A            |
    |         +--------------+ |
    |         |   设备 B     | |
    |         | +-------+    | |
    >>"b_clk">>>| "clk" |    | |
    |  (输入) | |  (入) |    | |
    |         | +-------+    | |
    |         +--------------+ |
    +--------------------------+


迁移
---------

时钟状态不自动迁移，各设备必须自行处理。别名时钟不得迁移。

要确保正确恢复状态，有两种方式。

第一种是在设备 vmstate 描述中添加条目，使用 ``VMSTATE_CLOCK`` 宏。
通常用于输入时钟，例如：

.. code-block:: c

    MyDeviceState {
        DeviceState parent_obj;
        [...] /* some fields */
        Clock *clk;
    };

    VMStateDescription my_device_vmstate = {
        .name = "my_device",
        .fields = (const VMStateField[]) {
            [...], /* other migrated fields */
            VMSTATE_CLOCK(clk, MyDeviceState),
            VMSTATE_END_OF_LIST()
        }
    };


第二种是根据已有信息恢复，适合根据设备状态恢复输出时钟。
可在 post_load() 迁移回调中使用 ``clock_set[_ns|_hz]()``。

为现有设备加入时钟支持时，应注意迁移兼容性，直接添加 VMSTATE_CLOCK() 会破坏兼容。
可以将其放入带适当 needed 函数的 vmstate 子节，
并在 pre_load() 中用 clock_set() 设置默认值，
以供源虚拟机未发送时钟状态时使用。

整个迁移过程中不得使用 ``clock_update[_ns|_hz]()`` 或 clock_propagate()，
否则会对状态尚不确定的其他设备触发副作用。
