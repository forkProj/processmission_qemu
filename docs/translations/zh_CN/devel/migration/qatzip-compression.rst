===========
QATzip 压缩
===========
网络带宽有限时，QATzip 可借助 Intel QuickAssist Technology（QAT）硬件
加速压缩和解压，节省大量主机 CPU 资源。

以下测试使用 8 个 multifd 通道和 10 Gbps 网络。
结果表明，相比 zstd，QATzip 显著节省发送端 CPU，并缩短迁移时间。
相比不压缩，它大幅提高以每秒页面数衡量的脏页处理能力，同时缩短总迁移时间。

::

   虚拟机配置：16 vCPU，64G 内存
   虚拟机负载：所有 vCPU 空闲，54G 内存填充 Silesia 数据。
   QAT 设备：4
   |-----------|--------|---------|----------|----------|------|------|
   |8 通道     |总      |停机     |吞吐量    |每秒      | 发送 | 接收 |
   |           |时间(ms)|时间(ms) |(mbps)    |页数      | cpu %| cpu% |
   |-----------|--------|---------|----------|----------|------|------|
   |qatzip     |   16630|       28|     10467|   2940235|   160|   360|
   |-----------|--------|---------|----------|----------|------|------|
   |zstd       |   20165|       24|      8579|   2391465|   810|   340|
   |-----------|--------|---------|----------|----------|------|------|
   |none       |   46063|       40|     10848|    330240|    45|    85|
   |-----------|--------|---------|----------|----------|------|------|



QATzip 压缩框架
===============

QATzip 是基于 Intel QuickAssist Technology 的用户空间库，
提供扩展的加速压缩和解压服务。

更多介绍见 `QATzip 简介
<https://github.com/intel/QATzip?tab=readme-ov-file#introductionl>`_。

::

  +----------------+
  | MultiFd 线程   |
  +-------+--------+
          |
          | 压缩/解压
  +-------+--------+
  | QATzip 库      |
  +-------+--------+
          |
  +-------+--------+
  |  QAT 库        |
  +-------+--------+
          |         用户空间
  --------+---------------------
          |         内核空间
   +------+-------+
   |  QAT 驱动    |
   +------+-------+
          |
   +------+-------+
   | QAT 设备     |
   +--------------+



安装 QATzip
-----------

部分 Linux 发行版已集成 QATzip 软件包，可直接安装。
例如 Ubuntu Server 24.04 LTS 可使用以下命令：

.. code-block:: shell

   #apt search qatzip
   libqatzip-dev/noble 1.2.0-0ubuntu3 amd64
     Intel QuickAssist user space library development files

   libqatzip3/noble 1.2.0-0ubuntu3 amd64
     Intel QuickAssist user space library

   qatzip/noble,now 1.2.0-0ubuntu3 amd64 [installed]
     Compression user-space tool for Intel QuickAssist Technology

   #sudo apt install libqatzip-dev libqatzip3 qatzip


如果系统没有相应软件包，可以从源码构建安装，见
`QATzip 源码安装
<https://github.com/intel/QATzip?tab=readme-ov-file#build-intel-quickassist-technology-driver>`_。

部署 QAT 硬件
-------------

QAT 支持通过物理功能（PF）或虚拟功能（VF）部署，
可以按实际需求配置迁移所用资源。详情见
`Intel QuickAssist Technology 文档 <https://intel.github.io/quickassist/index.html>`_。

硬件概述见
`Intel QuickAssist Technology 概览
<https://www.intel.com/content/www/us/en/architecture-and-technology/intel-quick-assist-technology-overview.html>`_。

如何使用 QATzip 压缩
====================

1 - 安装 QATzip 库。

2 - 构建 QEMU 时使用 ``--enable-qatzip``。

  例如 configure --target-list=x86_64-softmmu --enable-kvm ``--enable-qatzip``

3 - 设置 ``migrate_set_parameter multifd-compression qatzip``。

4 - 设置 ``migrate_set_parameter multifd-qatzip-level comp_level``。
comp_level 默认为 1，支持 1～9。

QAT 内存需求
============

需要为 QAT 内存管理预留系统内存，以便分配 DMA 内存。
预留量取决于迁移使用的设备数量和 multifd 通道数。

内存用量与 QAT 配置有关，计算方法见
`QAT 内存驱动查询
<https://intel.github.io/quickassist/PG/infrastructure_debugability.html?highlight=memory>`_。

.. list-table:: 使用一个 PF 迁移的示例
  :header-rows: 1

  * - 通道数
    - 发送端内存用量
    - 接收端内存用量
  * - 2
    - 10M
    - 10M
  * - 4
    - 12M
    - 14M
  * - 8
    - 16M
    - 20M

如何选择 QATzip 与 QPL
======================
从第四代 Intel Xeon 可扩展处理器 Sapphire Rapids（SPR）开始，
支持 QAT、IAA 等多种内置加速器。
QAT 用于加速 QATzip，IAA 用于加速 QPL。

建议如下：

1 - 如果热迁移受网络带宽限制，且 QAT 硬件资源多于 IAA，
使用 QATzip 可节省大量压缩所需的主机 CPU 资源。

2 - 如果系统不支持共享虚拟内存（SVM），使用 QATzip，
因为缺少 SVM 支持时 QPL 性能不佳。

3 - 其他场景优先使用 QPL。
