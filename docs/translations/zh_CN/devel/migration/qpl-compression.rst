========
QPL 压缩
========
Intel Query Processing Library（Intel ``QPL``）是提供压缩和解压功能的开源库，基于 deflate 压缩算法（RFC 1951）。

``QPL`` 压缩依赖 Intel In-Memory Analytics Accelerator（``IAA``）和共享虚拟内存（``SVM``）技术。这些功能从第四代 Intel Xeon 可扩展处理器开始支持，该处理器代号为 Sapphire Rapids（``SPR``）。

有关 ``QPL`` 的更多介绍，参见 `QPL 简介 <https://intel.github.io/qpl/documentation/introduction_docs/introduction.html>`_。

QPL 压缩框架
============

::

  +----------------+       +------------------+
  | MultiFD 线程   |       |accel-config 工具 |
  +-------+--------+       +--------+---------+
          |                         |
          |                         |
          |压缩/解压                |
  +-------+--------+                | 配置 IAA
  |  QPL 库        |                | 资源
  +-------+---+----+                |
          |   |                     |
          |   +-------------+-------+
          |   打开 IAA      |
          |   设备    +-----+-----+
          |           |idxd 驱动  |
          |           +-----+-----+
          |                 |
          |                 |
          |           +-----+-----+
          +-----------+IAA 设备   |
      提交任务        +-----------+
      经 enqcmd



构建与安装 QPL
--------------

.. code-block:: shell

  $git clone --recursive https://github.com/intel/qpl.git qpl
  $mkdir qpl/build
  $cd qpl/build
  $cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DQPL_LIBRARY_TYPE=SHARED ..
  $sudo cmake --build . --target install


有关 ``QPL`` 安装的更多信息，参见 `QPL 安装 <https://intel.github.io/qpl/documentation/get_started_docs/installation.html>`_。

IAA 设备管理
------------

``IAA`` 设备数量取决于 Xeon 产品型号。在 ``SPR`` 服务器上，最多可有 8 个 ``IAA`` 设备，每个处理器插槽最多 4 个。

默认情况下，所有 ``IAA`` 设备均处于禁用状态，需要用户手动配置并启用。

通过以下命令检查设备数量：

.. code-block:: shell

  #lspci -d 8086:0cfe
  6a:02.0 System peripheral: Intel Corporation Device 0cfe
  6f:02.0 System peripheral: Intel Corporation Device 0cfe
  74:02.0 System peripheral: Intel Corporation Device 0cfe
  79:02.0 System peripheral: Intel Corporation Device 0cfe
  e7:02.0 System peripheral: Intel Corporation Device 0cfe
  ec:02.0 System peripheral: Intel Corporation Device 0cfe
  f1:02.0 System peripheral: Intel Corporation Device 0cfe
  f6:02.0 System peripheral: Intel Corporation Device 0cfe


配置与启用 IAA 设备
^^^^^^^^^^^^^^^^^^^

``accel-config`` 工具用于启用 ``IAA`` 设备，以及配置 ``IAA`` 硬件资源（工作队列和引擎）。一个 ``IAA`` 设备具有 8 个工作队列和 8 个处理引擎，可以通过 ``group`` 属性把多个引擎分配给一个工作队列。

安装 ``accel-config`` 的方法参见 `accel-config 安装 <https://github.com/intel/idxd-config>`_。

以下示例展示如何配置并启用一个 ``IAA`` 设备。

.. code-block:: shell

  #accel-config config-engine iax1/engine1.0 -g 0
  #accel-config config-engine iax1/engine1.1 -g 0
  #accel-config config-engine iax1/engine1.2 -g 0
  #accel-config config-engine iax1/engine1.3 -g 0
  #accel-config config-engine iax1/engine1.4 -g 0
  #accel-config config-engine iax1/engine1.5 -g 0
  #accel-config config-engine iax1/engine1.6 -g 0
  #accel-config config-engine iax1/engine1.7 -g 0
  #accel-config config-wq iax1/wq1.0 -g 0 -s 128 -p 10 -b 1 -t 128 -m shared -y user -n app1 -d user
  #accel-config enable-device iax1
  #accel-config enable-wq iax1/wq1.0


.. note::
   IAX 是 IAA 的早期名称。

- ``IAA`` 设备索引为 1，可使用 ``ls -lh /sys/bus/dsa/devices/iax*`` 命令查询 ``IAA`` 设备索引。

- 在组 0 中配置 8 个引擎和 1 个工作队列，因此提交到该工作队列的所有压缩任务可以由全部引擎同时处理。

- 设置工作队列属性，包括工作模式、工作队列大小等。

- 启用 ``IAA1`` 设备和工作队列 1.0。

.. note::

  将工作队列模式设为共享模式，因为 ``QPL`` 库只支持共享模式。

更详细的配置请参见 `IAA 配置示例 <https://github.com/intel/idxd-config/tree/stable/Documentation/accfg>`_。

IAA 单元测试
^^^^^^^^^^^^

- 在 Xeon 平台启用 ``IAA`` 设备的方法参见 `IAA 用户指南 <https://www.intel.com/content/www/us/en/content-details/780887/intel-in-memory-analytics-accelerator-intel-iaa.html>`_。

- ``IAA`` 设备驱动为 Intel Data Accelerator Driver（idxd），建议 Linux 内核版本至少为 5.18。

- 在内核命令行添加 ``"intel_iommu=on,sm_on"`` 参数，以启用 ``SVM`` 功能。

以下是使用 `iaa_test <https://github.com/intel/idxd-config/tree/stable/test>`_ 验证 ``IAA`` 设备驱动和 ``SVM`` 的简单方法：

.. code-block:: shell

  #./test/iaa_test
   [ info] alloc wq 0 shared size 128 addr 0x7f26cebe5000 batch sz 0xfffffffe xfer sz 0x80000000
   [ info] test noop: tflags 0x1 num_desc 1
   [ info] preparing descriptor for noop
   [ info] Submitted all noop jobs
   [ info] verifying task result for 0x16f7e20
   [ info] test with op 0 passed



为迁移分配 IAA 资源
^^^^^^^^^^^^^^^^^^^

迁移没有 ``IAA`` 资源配置参数，``accel-config`` 工具的配置也不能直接指定迁移所使用的 ``IAA`` 资源。

采用 ``QPL`` 压缩方式的 multifd 迁移会使用所有已启用且处于共享模式的工作队列。

.. note::

  默认情况下，访问 IAA 资源需要 ``sudo`` 命令或 ``root`` 权限。管理员可以修改 IAA 设备节点的所有者，使 QEMU 能以指定用户的权限使用 IAA。

  例如：

  #chown -R qemu /dev/iax

共享虚拟内存（SVM）简介
=======================

这项能力使加速器 I/O 设备能够在主机处理器上应用程序的同一虚拟内存空间中运行，也意味着可以使用可换页内存，无需为 DMA 操作固定内存。

采用 ``SVM`` 技术时，用户无需为 ``IAA`` 设备预留内存并执行内存固定操作。``IAA`` 设备可以直接通过进程的虚拟地址访问数据。

有关 ``SVM`` 技术的更多信息，参见 `使用 ENQCMD 的共享虚拟寻址（SVA） <https://docs.kernel.org/next/x86/sva.html>`_。


如何在迁移中使用 QPL 压缩
=========================

1 - 安装 ``QPL`` 库；如果使用 IAA，还需安装 ``accel-config`` 库。

2 - 通过 ``accel-config`` 配置并启用 ``IAA`` 设备和工作队列。

3 - 使用 ``--enable-qpl`` 参数构建 ``QEMU``。

  例如：configure --target-list=x86_64-softmmu --enable-kvm ``--enable-qpl``

4 - 在迁移过程中启用 ``QPL`` 压缩。

  迁移时设置 ``migrate_set_parameter multifd-compression qpl``。``QPL`` 压缩不支持配置压缩级别，只支持一个压缩级别。

QPL 与 ZLIB 的差异
==================

虽然 ``QPL`` 和 ``ZLIB`` 都基于 deflate 压缩算法，而且 ``QPL`` 支持 ``ZLIB`` 的头部和尾部，但迁移中的 ``QPL`` 压缩仍无法与 ``ZLIB`` 完全兼容。

``QPL`` 只支持 4K 历史缓冲区，而 ``ZLIB`` 默认使用 32K。``ZLIB`` 压缩的数据可能无法由 ``QPL`` 正确解压，反之亦然。

``QPL`` 不支持 ``ZLIB`` 流式压缩中的 ``Z_SYNC_FLUSH`` 操作。当前 ``ZLIB`` 实现使用 ``Z_SYNC_FLUSH``，因此每个 ``multifd`` 线程都有一个 ``ZLIB`` 流上下文，所有页面的压缩和解压都基于此流。``QPL`` 无法解压这类数据，反之亦然。

关于 ``Z_SYNC_FLUSH`` 的介绍，请参见 `Zlib 手册 <https://www.zlib.net/manual.html>`_。

最佳实践
========
用户启用 IAA 设备执行 ``QPL`` 压缩时，建议在目标端启动参数中添加 ``-mem-prealloc``。此参数可以避免 I/O 缺页，减少 IAA 压缩和解压的开销。

带 ``-mem-prealloc`` 参数的启动示例：

.. code-block:: shell

   $qemu-system-x86_64 --enable-kvm -cpu host --mem-prealloc ...



以下示例测量未使用 ``-mem-prealloc`` 时目标端的 I/O 缺页。``svm_prq`` 表示 I/O 缺页发生次数和处理时间。

.. code-block:: shell

  #echo 1 > /sys/kernel/debug/iommu/intel/dmar_perf_latency
  #echo 2 > /sys/kernel/debug/iommu/intel/dmar_perf_latency
  #echo 3 > /sys/kernel/debug/iommu/intel/dmar_perf_latency
  #echo 4 > /sys/kernel/debug/iommu/intel/dmar_perf_latency
  #cat /sys/kernel/debug/iommu/intel/dmar_perf_latency
  IOMMU: dmar18 Register Base Address: c87fc000
                  <0.1us   0.1us-1us    1us-10us  10us-100us   100us-1ms    1ms-10ms      >=10ms     min(us)     max(us) average(us)
   inv_iotlb           0         286         123           0           0           0           0           0           1           0
  inv_devtlb           0         276         133           0           0           0           0           0           2           0
     inv_iec           0           0           0           0           0           0           0           0           0           0
     svm_prq           0           0       25206         364         395           0           0           1         556           9
