.. _migration:

========
迁移框架
========

QEMU 可以保存和加载客户机状态，二者互为逆操作。
保存时记录各设备状态，恢复时则将状态加载回各设备。

为此，两次启动通常必须使用相同参数，
即恢复目标应具有相同设备。某些要求可以适当放宽，但这里先假定配置完全一致。

具备保存和恢复能力后，就能实现迁移：在一台机器上运行，再移动到另一台机器。

在线迁移进一步允许客户机在状态传输期间继续运行。
这很重要，因为 RAM 等状态可能很大，传输需要较长时间。
只有发送最后一部分状态时才暂停客户机，典型停顿为几百毫秒以内，但受多种因素影响。

.. contents::

传输方式
========

迁移数据通常是可经任意传输方式发送的字节流。

- tcp：使用 TCP socket。
- unix：使用 Unix socket。
- exec：通过某个进程的标准输入输出传输。
- fd：使用传给 QEMU 的文件描述符，不关心该描述符如何打开。
- file：通过路径指定文件。支持偏移，使管理程序可以在文件开头添加自己的元数据。
  注意，迁移结束时 QEMU 不会刷新缓存的文件数据和元数据。

  也可以使用已打开的文件，通过 fdset 传入一组描述符，详见 QMP add-fd。
  这样管理程序能够控制文件打开过程。
  启用 multifd 时有严格要求：

    - fdset 必须包含两个彼此不是 dup 副本的文件描述符。
    - 使用 direct-io 时，必须恰有一个描述符设置 O_DIRECT。
    - 源端以 WRONLY 打开，目标端以 RDONLY 打开。

- rdma：由 RDMA 硬件传输页面，显著减少 CPU 负担。
  内部实现有所不同，但除 RAM 迁移代码外基本不可见。

这些方式共用设备状态保存和恢复基础设施，也与 savevm/loadvm 共用。

公共基础设施
============

承载迁移流的文件、socket 和描述符抽象为 QEMUFile，见 migration/qemu-file.h。
多数情况下，它连接到某个 QIOChannel 子类，见 io/。

保存单个设备状态
================

多数设备通过一次调用保存状态，属于非迭代设备。
它们的数据在预复制末尾、CPU 暂停后发送。
RAM 或大表等包含大量数据的设备则采用迭代方式，见下文。

设备开发建议
------------

- 状态应反映被模拟的设备，而非当前实现细节，
  使未来改变实现时仍保持流兼容。也可能需要保存寄存器不可见的内部状态。

- 保存时可能遍历并检查设备，发现内部损坏或客户机异常操作等问题。
  应谨慎使用断言或直接终止，因为用户会认为迁移破坏了此前看似正常的虚拟机。
  应记录原因，并考虑将设备置为错误状态，让虚拟机其余部分继续运行。

- 迁移可能恰逢客户机重新配置设备、重启、关机或设备等待外部 I/O。
  强烈建议这些情况下仍能迁移，因为云平台可能自动迁移管理员不能直接控制的虚拟机。

- 必须失败时，要记录足够信息说明原因。

- 目标端应将迁入流视为不可信。检查缓冲区偏移等，防止越界；
  流损坏时应拒绝迁入。现有代码对此的检查程度并不一致。

- 注意可能依赖迁移版本的状态或行为。
  例如 PCI capability 顺序必须保持不变；
  改变默认行为后，原本罕见、由 subsection 处理的情况可能变得常见。

- 迁出不能改变或破坏源端状态。
  超时、管理层取消或目标宿主故障都很常见，此时客户机会在源端恢复。
  即使 QEMU 层报告成功，只要目标尚未开始执行，管理层仍可合法撤销迁移。

- 总线和设备应支持实例化时显式指定地址，管理工具也应使用它。
  例如 USB 热插入必须指定端口及地址，
  否则根据命令行顺序产生的隐式排序可能在目标端不同，将状态加载到错误设备。

VMState
--------

多数设备数据可通过 VMSTATE 宏描述，主要定义在 include/migration/vmstate.h。

以下示例来自 hw/input/pckbd.c：

.. code:: c

  static const VMStateDescription vmstate_kbd = {
      .name = "pckbd",
      .version_id = 3,
      .minimum_version_id = 3,
      .fields = (const VMStateField[]) {
          VMSTATE_UINT8(write_cmd, KBDState),
          VMSTATE_UINT8(status, KBDState),
          VMSTATE_UINT8(mode, KBDState),
          VMSTATE_UINT8(pending, KBDState),
          VMSTATE_END_OF_LIST()
      }
  };


状态名称为 pckbd，version_id 为 3，KBDState 中保存四个 uint8_t 字段。
通过以下函数之一注册描述：第一个为每次注册生成不同的 instance_id，
已有每实例唯一 ID 时使用第二个。

.. code:: c

    vmstate_register_any(NULL, &vmstate_kbd, s);
    vmstate_register(NULL, instance_id, &vmstate_kbd, s);


基于 qdev 的设备可以在类初始化函数中注册：

.. code:: c

    dc->vmsd = &vmstate_kbd_isa;


VMState 宏保证设备数据采用可移植格式，通常为大端，
并在编译期对结构体字段类型进行部分检查。

可以嵌套其他描述保存子结构（``VMSTATE_STRUCT_``）、数组（``VMSTATE_ARRAY_``）
及变长数组（``VMSTATE_VARRAY_``），也有用于特殊情况的其他宏。

线上格式仍十分原始，例如 VMSTATE_UINT32 就是四字节大端整数。
未来可能采用更结构化的格式。

旧式方式
--------

所有现有用户迁移到 VMState 后将移除此方式，但转换旧代码可能复杂，因此进度不确定。

设备分别注册保存和加载函数：

.. code:: c

  int register_savevm_live(const char *idstr,
                           int instance_id,
                           int version_id,
                           SaveVMHandlers *ops,
                           void *opaque);


ops 中包含 save_state 和 load_state。
load_state 接收 version_id，判断输入格式；
save_state 不接收版本，因为始终保存最新格式。

由于 VMState 仍保存原始字节，很多情况下可通过精心设计的描述，
精确匹配旧代码的字节布局，替换旧实现。

修改迁移数据结构
----------------

设备状态以字段序列保存和加载。
修复缺陷或增加功能时，可能需要保存更多或不同信息，
必须使用适当方法避免破坏兼容性。
QEMU 通常尽量保证从版本 n 到 n+1 的前向迁移，也有用户依赖向后兼容。

子节
--------

最常见变化是新增数据，例如支持新设备形态，或补上之前漏迁移的状态。
通常应使用 subsection。

子节类似设备 VMState，但具有一个布尔函数，判断是否需要发送。
返回 false 时不发送。子节使用唯一名称供接收端查找。

目标端遇到不认识的子节会使迁移失败。
全部认识则能加载成功，但不会检查某子节是否必然存在，
因此较新 QEMU 可以在谨慎处理缺省值的前提下，加载旧版没有发送该子节的流。

如果新数据只在罕见情况下需要，可以按条件发送，
多数情况下仍能迁往旧 QEMU。对关键数据这很合适；
有时更希望缺少数据也能迁移，则可以把子节关联到设备属性，再关联到带版本号的机器类型。

子节的 pre_load、post_load 仅在该子节被加载时调用。

外层 post_load() 在全部子节加载后调用，因为新增子节可能改变它使用的值。
可结合标志和外层 pre_load、post_load，判断是否加载过子节，缺失时采用默认行为。

示例：

.. code:: c

  static bool ide_drive_pio_state_needed(void *opaque)
  {
      IDEState *s = opaque;

      return ((s->status & DRQ_STAT) != 0)
          || (s->bus->error_status & BM_STATUS_PIO_RETRY);
  }

  const VMStateDescription vmstate_ide_drive_pio_state = {
      .name = "ide_drive/pio_state",
      .version_id = 1,
      .minimum_version_id = 1,
      .pre_save = ide_drive_pio_pre_save,
      .post_load = ide_drive_pio_post_load,
      .needed = ide_drive_pio_state_needed,
      .fields = (const VMStateField[]) {
          VMSTATE_INT32(req_nb_sectors, IDEState),
          VMSTATE_VARRAY_INT32(io_buffer, IDEState, io_buffer_total_len, 1,
                               vmstate_info_uint8, uint8_t),
          VMSTATE_INT32(cur_io_buffer_offset, IDEState),
          VMSTATE_INT32(cur_io_buffer_len, IDEState),
          VMSTATE_UINT8(end_transfer_fn_idx, IDEState),
          VMSTATE_INT32(elementary_transfer_size, IDEState),
          VMSTATE_INT32(packet_transfer_size, IDEState),
          VMSTATE_END_OF_LIST()
      }
  };

  const VMStateDescription vmstate_ide_drive = {
      .name = "ide_drive",
      .version_id = 3,
      .minimum_version_id = 0,
      .post_load = ide_drive_post_load,
      .fields = (const VMStateField[]) {
          .... several fields ....
          VMSTATE_END_OF_LIST()
      },
      .subsections = (const VMStateDescription * const []) {
          &vmstate_ide_drive_pio_state,
          NULL
      }
  };


这是 PIO 状态子节，仅在 PIO 操作进行中需要保存，
由 ide_drive_pio_state_needed() 检查。
未启用 DRQ_STAT 时，这些字段内容无意义，无需发送。

将子节关联到属性
----------------

用属性作为发送条件，尤其结合版本化机器类型时，可以保留向后迁移兼容性。

例如：

   a) 用 DEFINE_PROP_BOOL 添加 support-foo 等属性，默认 true。
   b) 在前一版本的 ``hw_compat_`` 中加入将其设为 false 的条目。
   c) 编写静态布尔函数 support_foo 检查属性。
   d) 添加子节，将 .needed 指向该函数。
   e) 必要时在外层 pre_load 为 foo 设置默认值，供子节缺失时使用。

这样，使用旧机器类型时不生成该子节，旧 QEMU 就能接受流。

不再发送已有字段
----------------

某些 VMState 成员可能不再需要，但：

  - 直接删除会破坏兼容性。
  - 设为版本相关并提高版本号会破坏向后兼容。

通常最好保留占位字段。

确实必须移除时：

  a) 像新增子节一样添加属性、兼容设置和条件函数。
  b) 将宏替换为 _TEST 版本，例如：

   ``VMSTATE_UINT32(foo, barstruct)``

   改为：

   ``VMSTATE_UINT32_TEST(foo, barstruct, pre_version_baz)``

   将来不再关心古老版本时才能完全移除。
   为保持向后兼容，必须用目标能理解的数据填充结构。

两端条件判断只要不同，就会启用不同字段，将数据加载到错误位置，
因此这种条件字段非常脆弱。

版本
--------

版本号用于重大、不兼容的设备迁移变化，使用它会破坏向后迁移兼容性。
多数变化可以通过子节或 _TEST 宏实现，避免此问题。

每个版本对应一组字段。save_state 始终保存最新版本，
load_state 有时能够读取旧版本。

两个版本字段为：

- version_id：该设备 VMState 支持的最高版本。
- minimum_version_id：支持的最低版本。

VMState 可以读取这一区间内的版本。

许多 VMSTATE 宏有 _V 形式，用于版本相关字段，例如：

.. code:: c

   VMSTATE_UINT16_V(ip_id, Slirp, 2),


只在版本 2 及以后加载该字段。

保存始终使用 version_id，因此更旧的 QEMU 无法加载该节。

状态整理回调
------------

有时不能直接保存结构体，需要先准备正确值。
例如 KVM：保存 CPU 前要让 KVM 将状态复制给 QEMU；
加载后则要将从 QEMUFile 读到的 CPU 状态交回 KVM。

描述中可提供以下回调：

- ``int (*pre_load)(void *opaque);``

  加载设备前调用。

- ``int (*post_load)(void *opaque, int version_id);``

  加载设备后调用。

- ``int (*pre_save)(void *opaque);``

  保存设备前调用。

- ``void (*post_save)(void *opaque);``

  保存后调用，即使保存失败也会调用，除非 pre_save 本身返回错误。

还提供带 errp 的变体：

- ``bool (*pre_load_errp)(void *opaque, Error **errp);``

  加载前调用。

- ``bool (*post_load_errp)(void *opaque, int version_id, Error **errp);``

  加载后调用。

- ``bool (*pre_save_errp)(void *opaque, Error **errp);``

  保存前调用。

新实现应优先采用 errp 版本，旧实现逐步转换；全部转换后将移除旧变体。

可参考 hpet.c，它使用前三种回调整理传输状态。

迁移数据与设备内部存储形式不同的时候，VMSTATE_WITH_TMP 可先填充临时结构，
再传递到主结构。

如果设备在初始化以外，例如响应客户机动作时，
调用 memory 或 portio_list API 改变内存布局，通常也需要在 post_load 中重建布局。
例如：

  - memory_region_add_subregion()
  - memory_region_del_subregion()
  - memory_region_set_readonly()
  - memory_region_set_nonvolatile()
  - memory_region_set_enabled()
  - memory_region_set_address()
  - memory_region_set_alias_offset()
  - portio_list_set_address()
  - portio_list_set_enabled()

设备保存和恢复顺序未定义，因此这些回调不得访问或修改其他设备状态，
例如不要在 post_load 中触发 update_irq()。
否则恢复不具确定性，会破坏执行记录与重放。

迭代设备迁移
------------

RAM 或某些平台设备数据量很大，一次性发送会让 CPU 暂停过久，因此采用迭代方式。

它们通常不用 VMState 宏，而直接使用 ``qemu_put_*``、``qemu_get_*`` 读写流，
也有适合高带宽 I/O 的专用版本。

需要提供：

  - save_setup：初始化数据结构，并发送首节信息；RAM 会发送 RAMBlock 列表及大小。
  - load_setup：初始化目标数据结构。
  - save_query_pending：报告还有多少数据需要保存。
  - save_live_iterate：发送一批数据，达到流带宽限制后停止，每次调用生成一节。
  - save_complete：发送包含剩余数据的最后一节。
  - load_state：加载各保存函数产生的节。
  - 保存端和加载端的 cleanup：迁移结束时调用。

这些节的格式通常由设备自行实现，应谨慎设计和解析，使内容容易验证。

设备顺序
--------

有些情况下加载顺序很重要。
例如设备加载时产生中断，而中断控制器之后才加载，可能丢失状态。

机器创建设备的顺序隐式提供了部分保证，但较脆弱。

MigrationPriority 枚举可显式规定顺序，数值越大越先加载。
通过设备顶层 VMStateDescription 的 priority 设置。

流结构
========

流尽量与字长、字节序无关，使不同宿主能够迁移同一虚拟机。

  - 头部

    - 魔数
    - 版本
    - 虚拟机配置节

       - 机器类型
       - 目标页大小位数

  - 节列表，每节包含一个设备或一次迭代保存的数据

    - 节类型
    - 节 ID
    - ID 字符串，仅设备首节
    - 实例 ID，仅设备首节
    - 版本 ID，仅设备首节
    - 设备数据
    - 尾标记

  - EOF 标记
  - 虚拟机描述结构：JSON 格式，仅用于分析内容

设备数据由前述代码生成。
非迭代设备只有一节，迭代设备有首节、末节及若干中间部分。
公共代码对内容完整性的检查很少，主要由设备自行负责。
尾标记可以在接收端多读或少读时提供有限保护。

ID 字符串通常由总线名和设备地址构成，具有唯一性，适合 PCI 及其控制器上的存储设备。
有些设备只有固定单实例，例如 pc-ram。
另一些旧设备或没有总线概念的系统设备，则使用实例 ID 区分同名设备。

返回路径
--------

普通迁移只需单向流，需要双向通信时可创建 return path。
主要用于后复制，也用于迁移结束时向源端返回成功标志。

``qemu_file_get_return_path(QEMUFile* fwdpath)`` 返回对应的 QEMUFile 指针。

  源端

     正向路径：由迁移线程写入。
     返回路径：主线程打开，返回路径线程读取。

  目标端

     正向路径：由主线程读取。
     返回路径：主线程打开，由主线程和后复制线程写入，通过 rp_mutex 保护。
