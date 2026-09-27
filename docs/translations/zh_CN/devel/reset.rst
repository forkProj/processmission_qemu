
==========================
QEMU 复位：Resettable 接口
==========================

QEMU 对象通过 include/hw/core/resettable.h 声明的 Resettable 接口处理复位。

接口允许按树组织对象，使整组对象一致地复位。
各成员无需关心其他对象，尤其无需自行处理谁先复位等顺序问题。
主要实现者是 DeviceClass 和 BusClass。

触发复位
--------

本节介绍使用者控制可复位对象的 API。
所有控制函数都必须在持有 BQL 时调用。

resettable_assert_reset() 使对象进入复位，
resettable_release_reset() 使其退出。
若只需立即复位，不保持复位状态，调用 resettable_reset()。
这些函数接收对象指针和复位类型两个参数。

类型通过 ResetType 枚举表示：

``RESET_TYPE_COLD``
  所有可复位对象均支持冷复位。
  在 QEMU 中，它表示恢复到 QEMU 启动时的初始状态，可能不同于真实硬件冷复位。
  暖复位、总线复位等其他类型可能保留部分状态。

``RESET_TYPE_SNAPSHOT_LOAD``
  加载快照前，将系统置于干净状态的复位，
  对应 SHUTDOWN_CAUSE_SNAPSHOT_LOAD。
  几乎所有设备都应按冷复位处理。
  主要例外是冷复位时需要重新随机初始化某些状态的设备，
  例如随机数种子；加载快照前不得重新初始化这些值。

``RESET_TYPE_WAKEUP``
  机器支持从挂起唤醒，且 MachineClass::wakeup() 中需要复位设备时使用。
  设备可据此区分唤醒复位和其他请求。
  例如唤醒时不能丢失 RAM，virtio-mem 等提供额外 RAM 的设备不得重置该状态，
  而冷复位时可能需要重置。
  但不能据此检测唤醒，因为并非所有机器唤醒时都请求设备复位。

``RESET_TYPE_S390_CPU_NORMAL``
  仅用于 S390 CPU，清除中断、停止处理并清空 TLB，不修改寄存器内容。

``RESET_TYPE_S390_CPU_INITIAL``
  仅用于 S390 CPU，执行 NORMAL 的全部操作，
  还清除 PSW、prefix、FPC、定时器和控制寄存器，不修改 GPR、FPR、ACR。

设备必须把未知 ResetType 当作冷复位，以减少未来添加新类型时需改动的现有代码。

resettable_reset() 等价于先 assert 再 release。
这三个函数可以多次交错调用，因为一个对象可能有多个复位源或控制器。
接口内部处理协调，各控制器无需知道其他控制器存在。
只有全部控制器结束复位后，对象才退出复位。
内部通过进行中复位计数实现，因此每次 assert 必须恰好对应一次 release。

目前不支持迁移处于复位中的设备或总线，
应避免在 assert 之后延迟对应 release。

Resettable 是接口，因此 API 接收普通 Object。
对非可复位对象调用属于编程错误，会触发运行时断言。
多数调用经过基类函数，因此这种错误不太容易发生。

设备和总线还有辅助函数：

- device_cold_reset()
- bus_cold_reset()

它们只是把 Device 或 Bus 转为 Object，再以冷复位类型调用 resettable_reset()。
能使用时，应优先选择这些辅助函数。

设备和总线分别提供接口，是因为复位总线和复位拥有该总线的控制器桥可能语义不同。
例如复位 SCSI 控制器会重置控制器寄存器和整条 SCSI 总线，
只复位 SCSI 总线则不影响控制器自身。


多阶段机制
----------

本节介绍接口内部实现。
多阶段设计使对象和机器无需处理复位顺序问题，将复位拆为三个明确阶段。

复位多个对象，例如模拟启动时复位整机，
先执行所有对象的第一阶段，再执行所有第二阶段，最后执行所有第三阶段。

三个阶段为：

1. **enter**：进入复位时执行，只重置本地状态。
   不得对其他对象产生副作用，例如拉高或拉低 qemu_irq，或读写客户机内存。

2. **hold**：同组全部对象已完成 enter 后执行，此时允许影响其他对象。

3. **exit**：退出复位状态时执行，允许影响其他对象。

内部计数确保仅在需要时执行各阶段。
enter 和 hold 仅在第一次断言复位时执行；
已处于复位时再次 assert 或 reset，不重复执行。
只有最后一次复位操作结束才执行 exit。
对象无需知道有多少控制器，以及多少控制器发起了复位。

支持 DMA 的设备应在 enter 或 hold 取消全部未完成 DMA。
IOMMU 应在 exit 复位，这种顺序确保没有残留 DMA 请求发生故障。


在可复位对象中处理复位
----------------------

本节介绍实现者必须提供的 API 和可用函数，
面向新增或转换可复位类的开发者，例如扩展已有设备或总线。

需要实现的方法
....................

可定义三个方法，也可留空：phases.enter()、phases.hold()、phases.exit()，
分别对应三个阶段。它们都接收对象参数，enter 还接收复位类型。

扩展现有类时，也可能需要扩展这些方法。
可用 resettable_class_set_parent_phases() 保存父类方法。

下面示例为复位期间设置 I/O 的设备实现复位：

::

    static void mydev_reset_enter(Object *obj, ResetType type)
    {
        MyDevClass *myclass = MYDEV_GET_CLASS(obj);
        MyDevState *mydev = MYDEV(obj);
        /* call parent class enter phase */
        if (myclass->parent_phases.enter) {
            myclass->parent_phases.enter(obj, type);
        }
        /* initialize local state only */
        mydev->var = 0;
    }

    static void mydev_reset_hold(Object *obj, ResetType type)
    {
        MyDevClass *myclass = MYDEV_GET_CLASS(obj);
        MyDevState *mydev = MYDEV(obj);
        /* call parent class hold phase */
        if (myclass->parent_phases.hold) {
            myclass->parent_phases.hold(obj, type);
        }
        /* set an IO */
        qemu_set_irq(mydev->irq, 1);
    }

    static void mydev_reset_exit(Object *obj, ResetType type)
    {
        MyDevClass *myclass = MYDEV_GET_CLASS(obj);
        MyDevState *mydev = MYDEV(obj);
        /* call parent class exit phase */
        if (myclass->parent_phases.exit) {
            myclass->parent_phases.exit(obj, type);
        }
        /* clear an IO */
        qemu_set_irq(mydev->irq, 0);
    }

    typedef struct MyDevClass {
        MyParentClass parent_class;
        /* to store eventual parent reset methods */
        ResettablePhases parent_phases;
    } MyDevClass;

    static void mydev_class_init(ObjectClass *class, const void *data)
    {
        MyDevClass *myclass = MYDEV_CLASS(class);
        ResettableClass *rc = RESETTABLE_CLASS(class);
        resettable_class_set_parent_phases(rc,
                                           mydev_reset_enter,
                                           mydev_reset_hold,
                                           mydev_reset_exit,
                                           &myclass->parent_phases);
    }


上例覆盖全部阶段，也可对不需要覆盖的阶段传 NULL。
例如仅覆盖 enter，保留 hold 和 exit::

    resettable_class_set_parent_phases(rc, mydev_reset_enter, NULL, NULL,
                                       &myclass->parent_phases);


这等价于为 hold 和 exit 提供只调用父类实现的简单方法。

查询复位状态
.......................

resettable_is_in_reset() 在对象当前处于复位时返回 true。

状态从 enter 开始、尚未调用自身或子对象 enter 时进入复位，一直持续到 exit。
enter 和 hold 期间返回复位状态。
exit 传播到子对象之后、调用自身 exit 之前，状态才改变。

如果对象在复位时需要调整行为，可以使用此函数。
例如设备复位期间可能需忽略 IRQ 输入，可在 IRQ 回调开头检查。

支持迁移复位状态之前，不应让对象长期停留在复位。
因此除正在执行复位阶段外，只有同组其他对象的 hold 或 exit
进行了改变 I/O 等外部交互时，此函数才可能返回 true。

设备和总线应优先使用 device_is_in_reset() 和 bus_is_in_reset()。


基类如何处理复位
----------------

只有为 DeviceClass、BusClass 之外的新基类扩展机制，
或维护这两个类内部实现时，才需要了解本节，多数开发者可以跳过。

需要实现的方法
....................

实现接口的类还需提供 get_state() 和 child_foreach()。

Resettable 是接口，没有自己的状态结构，但公共代码需要存储状态。
get_state() 必须返回 ResettableState 指针，
由基类分配，最好直接嵌入对象实例结构。

child_foreach() 为给定对象的每个复位子对象执行回调。
全部子对象也必须可复位，并向回调继续传递复位类型和不透明指针。

DeviceClass 和 BusClass 的 ResettableState 分别位于 DeviceState、BusState。
child_foreach() 遵循总线层级：总线遍历子设备，设备遍历子总线。
因此复位主系统总线即可复位整个机器总线树。

改变复位父节点
............................

基类还需处理复位层级变化。

层级原则上在机器创建期间构建并保持静态，但存在例外。
resettable_change_parent() 可在机器创建完成后设置、更新或移除父节点，
参数包括被移动对象、旧父节点和新父节点，后两者可以不存在。

非复位期间可随时调用；复位期间只能在 hold 调用，enter 或 exit 中调用是错误。
也不应在机器创建期间调用，尽管这样无害：
只要旧、新父节点为 NULL 或均不在复位，函数就不执行操作。

目前有两个使用场景：

1. 设备热插入：将新设备加入运行中的总线。

2. 热切换总线：在总线层级中添加、移动或移除正在运行的设备。
   当前仅用于 raspi 机器切换 SD 卡使用的 sdbus。

复位整个系统
------------

整机复位稍复杂，典型流程如下：

1. 请求方调用 qemu_system_reset_request() 安排复位。
   实际复位在返回后异步发生，因此可安全从设备模型等位置调用。

2. 实际执行由 qemu_system_reset() 完成，通常只有核心系统代码应直接调用。

3. 若当前机器定义 MachineClass::reset，则调用它，该方法必须调用 qemu_devices_reset()；
   否则直接调用 qemu_devices_reset()。

4. qemu_devices_reset() 使用三阶段机制，复位通过 qemu_register_resettable()
   注册的对象，并在 hold 阶段调用 qemu_register_reset() 注册的函数。

5. 最重要的对象是 sysbus，它是 qbus 树根，
   因而总线上的全部设备、子总线及其设备都会递归复位。

6. 不在 qbus 树中的设备不会自动复位。
   最明显的是 CPU，也包括直接继承 TYPE_OBJECT 或 TYPE_DEVICE，
   而非 TYPE_SYS_BUS_DEVICE 等接入总线类型的对象。
   必须通过其他方式安排，例如 qemu_register_resettable() 或 qemu_register_reset()。
