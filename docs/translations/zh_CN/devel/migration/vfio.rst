=============
VFIO 设备迁移
=============

虚拟机迁移需要保存客户机在源主机上运行的每个设备的状态，并在目标主机恢复这些状态。本文详细介绍 QEMU 如何保存和恢复 VFIO 设备。

VFIO 设备迁移包括两个阶段：可选的预复制阶段，以及停止并复制阶段。预复制阶段采用迭代方式，能够处理需要传输大量数据的 VFIO 设备。在迁移的迭代预复制阶段，客户机可在 VFIO 设备状态传输到目标端时继续运行，有助于减少虚拟机总停机时间。VFIO 设备通过在 VFIO_DEVICE_FEATURE_MIGRATION ioctl 中报告 VFIO_MIGRATION_PRE_COPY 标志，声明支持预复制。

支持预复制时，可以启用 "switchover-ack" 迁移能力，进一步缩短停机时间。VFIO 迁移 uAPI 将“初始字节”定义为预复制数据流的一部分，并建议在停止源虚拟机之前发送这些初始字节，并在目标端加载。启用此迁移能力可以保证这一点，因此可能进一步减少停机时间。

例如，mlx5 设备的初始字节包含元数据，用于目标端耗时较长的资源预分配。即使初始字节数据量小、发送很快，在目标端加载也可能耗时较长。Switchover-ack 保证这种预分配不会发生在停机期间。

初始字节原先被定义为单调递减，但某些情况下，需要在预复制过程中传输新的初始字节块，例如设备重新配置后。VFIO_PRECOPY_INFO_REINIT 功能解决了这个问题：支持该功能时，可以报告新的初始字节值，不受先前报告值的限制。此时会请求新的切换 ACK，确保在切换之前，新初始字节已在目标端加载。

为支持多个设备之间可能存在 P2P 事务的迁移，VFIO 迁移 uAPI 定义了一个中间的 P2P 静止状态。在此状态下，设备不能发起 P2P DMA 事务，但可以响应传入的事务。此外，设备进入该状态时，保证所有未完成的 P2P 事务均已结束。

所有支持 P2P 迁移的设备都先转换到 P2P 静止状态，然后才停止或启动。由于无法对所有设备同时原子地执行启动和停止，这样可以保证迁移过程中 P2P 操作的安全。

因此，只有所有设备都支持 P2P 迁移时，才允许迁移多个 VFIO 设备。迁移单个 VFIO 设备则不受是否支持 P2P 迁移的限制。

VFIO 设备迁移 UAPI 的详细说明见头文件 linux-headers/linux/vfio.h 中 ``vfio_device_mig_state`` 结构的注释。

VFIO 为迭代方式实现了以下设备钩子：

* ``save_setup`` 函数在源端设置迁移。

* ``load_setup`` 函数将目标端 VFIO 设备设为 _RESUMING 状态。

* ``save_query_pending`` 函数报告厂商驱动尚需为 VFIO 设备保存的数据量。

* ``is_active_iterate`` 函数指示，只有 VFIO 设备处于预复制状态时，``save_live_iterate`` 才处于活动状态。

* ``save_live_iterate`` 函数在迭代预复制阶段从厂商驱动读取 VFIO 设备数据。

* ``switchover_start`` 函数在 multifd 模式下启动线程，重组 multifd 接收的数据，并按顺序加载到设备中。在非 multifd 模式下，此函数不执行任何操作。

* ``save_state`` 函数在非 multifd 模式下保存设备配置空间（如果存在）。在 multifd 模式下，它只发出一个占位 EOS 标记。

* ``save_complete`` 函数将 VFIO 设备设为 _STOP_COPY 状态，并迭代复制 VFIO 设备数据，直到厂商驱动表示没有剩余数据。在 multifd 模式下，它只发出一个占位 EOS 标记。

* ``save_complete_precopy_thread`` 函数在 multifd 模式下提供执行 multifd 设备状态传输的线程处理程序。它将 VFIO 设备设为 _STOP_COPY 状态，迭代读取设备数据并放入 multifd 传输队列，直到厂商驱动表示没有剩余数据。随后保存设备配置空间，并同样排队进行 multifd 传输。在非 multifd 模式下，此线程不执行任何操作。

* ``load_state`` 函数加载上述保存函数生成的配置段和数据段。

* ``load_state_buffer`` 函数加载通过 multifd 通道到达的设备状态和设备配置。仅用于 multifd 模式。

* 保存和加载过程各自的 ``cleanup`` 函数执行所有与迁移有关的清理。


VFIO 迁移代码使用虚拟机状态变化处理程序，在虚拟机从运行变为非运行状态或反向变化时，相应地改变 VFIO 设备状态。

类似地，迁移状态变化处理程序在某些迁移状态变化发生时触发 VFIO 设备状态转换。例如，迁移失败或取消时，VFIO 设备状态会恢复为 _RUNNING。

系统内存脏页跟踪
----------------

内存监听器回调 ``log_global_start`` 和 ``log_global_stop`` 通知 VFIO 脏页跟踪模块启动和停止脏页跟踪。``log_sync`` 内存监听器回调从脏页跟踪模块查询脏页位图，并将 VFIO 设备执行过 DMA 的系统内存页标记为脏页。脏页位图按容器查询。

目前可通过两种方式跟踪脏页：
(1) 设备脏页跟踪：
此方式由设备负责记录并报告自身的 DMA。只有设备具备跟踪自身 DMA 的能力时才能采用。发现设备能力、启动和停止脏页跟踪，以及从设备同步脏位图，都通过 DMA 日志 uAPI 完成。更多 uAPI 信息见头文件 linux-headers/linux/vfio.h 中 ``vfio_device_feature_dma_logging_control`` 和 ``vfio_device_feature_dma_logging_report`` 结构的注释。

(2) VFIO IOMMU 模块：
此方式由 IOMMU 跟踪脏页。不过，目前 IOMMU 尚不支持脏页跟踪。因此，所有页面都会始终标记为脏页；如果设备驱动通过外部 API 固定了页面，则只有这些固定的页面始终标记为脏页。

如果以上两种方式均不受支持，QEMU 会始终将所有页面标记为脏页。

默认情况下，预复制阶段和停止并复制阶段都会跟踪脏页，因此被标记为脏的页面会在两个阶段都复制到目标端。在预复制阶段复制脏页，有助于 QEMU 预测能否满足停机时间容忍范围。如果 QEMU 在预复制阶段不断发现脏页，就能推断在停止并复制阶段也可能存在脏页，并据此预测停机时间。

QEMU 还提供按设备选择退出的选项 ``pre-copy-dirty-page-tracking``，用于禁用预复制阶段的脏位图查询。设为 off 后，所有脏页仅在停止并复制阶段复制到目标端。

启用 vIOMMU 时的系统内存脏页跟踪
--------------------------------

使用 vIOMMU 时，迁移的预复制阶段可能取消某个 I/O 虚拟地址范围的映射。此时，取消映射 ioctl 会返回该范围内的脏页，QEMU 将相应客户机物理页报告为脏页。在停止并复制阶段，使用 IOMMU 通知器获取已映射页面的回调，随后从 VFIO IOMMU 模块提取这些映射范围的脏页位图。如果在使用 vIOMMU 时启用设备脏页跟踪，将阻止实时迁移。

实时迁移期间的状态变化流程
==========================

下图展示同时支持预复制和 P2P 迁移的 VFIO 设备在实时迁移期间的状态变化。不支持这些功能的设备流程类似，只是跳过预复制和 P2P 的相关状态。括号中的值依次表示虚拟机状态、迁移状态和 VFIO 设备状态。

实时迁移保存路径
----------------

::

                           QEMU 正常运行状态
                           (RUNNING, _NONE, _RUNNING)
                                      |
                     migrate_init 创建 migration_thread
            迁移线程随后调用各设备的 .save_setup()
                          (RUNNING, _SETUP, _PRE_COPY)
                                      |
                         (RUNNING, _ACTIVE, _PRE_COPY)
  设备活动时，通过 .state_pending_{estimate,exact}() 获取 pending_bytes
       若总 pending_bytes >= threshold_size，调用 .save_live_iterate()
                复制预复制阶段的 VFIO 设备数据
      迭代至待传字节总量收敛并低于阈值
                                      |
       迁移完成时，停止 vCPU 和 VFIO 设备
              首先将 VFIO 设备置于 P2P 静止状态
                    (FINISH_MIGRATE, _ACTIVE, _PRE_COPY_P2P)
                                      |
                随后将 VFIO 设备置于 _STOP_COPY 状态
                     (FINISH_MIGRATE, _ACTIVE, _STOP_COPY)
               为每个活动设备调用 .save_complete()
              对 VFIO 设备：非 multifd 模式下迭代调用
                             .save_complete()，直到
                               待传数据为 0
	          multifd 模式下，此迭代改在
	          .save_complete_precopy_thread() 中执行
                                      |
                     (POSTMIGRATE, _COMPLETED, _STOP_COPY)
            迁移线程调度清理下半部并退出
                                      |
                           调用 .save_cleanup()
                        (POSTMIGRATE, _COMPLETED, _STOP)


实时迁移恢复路径
----------------

::

             传入迁移为各设备调用 .load_setup()
                          (RESTORE_VM, _ACTIVE, _STOP)
                                      |
     对每个设备，通过主迁移通道传输的设备段数据
                 由 .load_state() 加载。
     对于 multifd 通道传输的数据，则调用 .load_state_buffer()
                                   加载。
                        (RESTORE_VM, _ACTIVE, _RESUMING)
                                      |
  最后为各设备调用 .load_cleanup()，并启动 vCPU
              首先将 VFIO 设备置于 P2P 静止状态
                        (RUNNING, _ACTIVE, _RUNNING_P2P)
                                      |
                           (RUNNING, _NONE, _RUNNING)


后复制
========

VFIO 设备目前不支持后复制迁移。

Multifd
========

从 QEMU 10.0 开始，可以通过 multifd 通道传输 VFIO 设备的 _STOP_COPY 状态。这有助于减少停机时间，尤其是在有多个 VFIO 设备或设备迁移状态较大的情况下。此外，在这种迁移模式下，将 VFIO 设备设为 _STOP_COPY 状态以及保存其配置空间也会并行执行，即在单独的线程中运行。

multifd VFIO 设备状态传输由 VFIO 设备属性 "x-migration-multifd-transfer" 控制。该属性默认为 AUTO，表示在其他方面满足要求的配置中，尝试通过 multifd 通道传输 VFIO 设备状态。

由于目标 QEMU 需要按顺序加载设备状态缓冲区，因此必须将传入缓冲区排队，直到可以加载到设备中。这意味着，恶意 QEMU 源端理论上可能使目标 QEMU 为这些尚在传输处理中的缓冲区分配无限量的内存。

"x-migration-max-queued-buffers-size" 属性可以限制目标端排队的这些 VFIO 设备状态缓冲区的总大小。

在大多数 VFIO 实时迁移用例中，恶意 QEMU 源端导致目标端 OOM 不被认为是现实威胁，而且合适的值取决于具体环境，因此默认通过将此值设为 UINT64_MAX 来禁用排队缓冲区大小限制。

某些主机平台（例如 ARM64）要求在所有可迭代状态加载完成后，于不可迭代状态加载阶段才加载 VFIO 设备配置。此类同步由 VFIO 设备属性 "x-migration-load-config-after-iter" 控制，其默认设置 AUTO 只在确有要求的平台上执行这一操作。
