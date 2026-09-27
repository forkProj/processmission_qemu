================
RAPL MSR 支持
================

RAPL（Running Average Power Limit，运行平均功率限制）接口公布各电源域
（例如 CPU 封装、DRAM 等）的累计能耗。

能耗通过 MSR（模型特定寄存器）报告，例如 CPU 封装电源域的
MSR_PKG_ENERGY_STATUS。这些 MSR 是 64 位寄存器，以微焦耳表示累计能耗。

借助 KVM 的 `MSR 过滤 <msr-filter-patch_>`__ 功能，MSR 不再全部由 KVM
处理。其中一些现在可由用户空间（QEMU）处理；创建虚拟机时会向 KVM 提供
一份 MSR 列表，访问这些 MSR 时就会退出到用户空间。

.. _msr-filter-patch: https://patchwork.kernel.org/project/kvm/patch/20200916202951.23760-7-graf@amazon.com/

目前涉及以下 MSR：

.. code:: C

    #define MSR_RAPL_POWER_UNIT             0x00000606
    #define MSR_PKG_POWER_LIMIT             0x00000610
    #define MSR_PKG_ENERGY_STATUS           0x00000611
    #define MSR_PKG_POWER_INFO              0x00000614

``*_POWER_UNIT``、``*_POWER_LIMIT``、``*_POWER INFO`` 属于 RAPL 规范，
用于指定封装的功率限制、提供参数范围（最小功率、最大功率等），以及提供
将能量计数器换算成功率所需的乘数信息。这些 MSR 在开始时通过读取主机 CPU
的 MSR 填充一次，在客户机请求时原样返回。

MSR_PKG_ENERGY_STATUS 是计数器，表示自寄存器上次清零以来消耗的能量总量。
将它乘以上述 UNIT 即可得到以微焦耳为单位的能量值。此计数器始终递增，
增长速度随封装的能耗而变化。计数器预期会在某个时刻溢出。

同一封装内的各个核心读取 MSR_PKG_ENERGY_STATUS（即 "rdmsr 0x611"）时，
得到的值相同。该值表示整个封装的能耗。无论哪个核心读取，结果都相同；
属于 PKG-0 的核心无法取得 PKG-1 的值，反之亦然。

实现概述
-------------------------

为更新虚拟 MSR 的值，会创建一个 QEMU 线程。该线程基本上是一个无限循环，
执行以下操作：

1. 获取所有 QEMU 线程的时间指标快照（在用户空间和系统空间被调度运行的时间）。

2. 获取 QEMU 线程所在全部封装的实际 MSR_PKG_ENERGY_STATUS 计数器快照。

3. 休眠 1 秒。在此期间，vcpu 和其他非 vcpu 线程执行各自工作，能量计数器随之增长。

4. 重复第 2 和第 3 步，计算每项指标的差值，得到各 QEMU 线程被调度运行的时间
   *以及* 休眠期间各封装消耗的能量。

5. 区分 vcpu 线程与非 vcpu 线程。

6. 获取虚拟机拓扑，以确定各 vCPU 运行在哪个虚拟封装上。

7. 将非 vcpu 线程消耗的总能量除以 vcpu 线程数，使每个 vcpu 线程均摊
   QEMU 工作线程消耗的能量。

8. 计算每个 vcpu 线程消耗能量的比例。

9. 计算各虚拟封装的能量。

10. 更新每个虚拟封装的虚拟 MSR。同一封装内的每个 vCPU 在访问 MSR 时
    都返回相同的值。

11. 返回第 1 步继续循环。

比例计算
-----------------

在 Linux 中，每个进程都有对应的执行时间。调度器以时钟滴答为单位划分时间。
每秒的时钟滴答数可通过 sysconf 系统调用查询，典型值为 100。因此，一个核心
每秒最多可运行进程 100 个滴答。如果封装有 4 个核心，那么在 1 秒内，
该封装所有核心上最多可调度 400 个滴答。

`/proc/[pid]/stat <stat_>`__ 是 procfs 文件，可提供进程 ID 为 [pid] 的进程
执行时间。它给出进程在用户空间（utime）和内核空间（stime）被调度的滴答数。

.. _stat: https://man7.org/linux/man-pages/man5/proc.5.html

读取线程的这些指标，即可计算封装执行该线程的时间占比。

例如：

一个 4 核封装在每核每秒 100 个滴答的条件下，每秒最多可调度 400 个滴答。
如果某个线程在该封装上 1 秒内被调度了 100 个滴答，就意味着该线程占用了
整个封装 1/4 的调度时间。因此，该线程在这整整 1 秒内于此封装上消耗的能量，
为封装总能耗的 1/4。

用法
-----

目前，此功能仅适用于已加载 RAPL 驱动且可通过 sysfs 访问该驱动的 Intel CPU。
否则 QEMU 会在启动时失败。

通过 -accel kvm,rapl=true,rapl-helper-socket=/path/sock.sock 启用此功能。

套接字路径必须与 :program:`qemu-vmsr-helper` 监听的路径相同。

qemu-vmsr-helper
----------------

qemu-vmsr-helper 的工作方式与 qemu-pr-helper 非常相似。不过它并非用于持久预留，
而是应对 CVE-2020-8694 导致用户无法访问 rapl msr 属性的问题。

启用了 RAPL MSR 支持的 QEMU 进程与 qemu-vmsr-helper 之间通过套接字通信。
contrib/systemd/qemu-vmsr-helper.(service/socket) 提供 systemd 服务和套接字激活配置。

systemd 套接字使用 600 权限，与 contrib/systemd/qemu-pr-helper.socket 一样。
libvirt 可以通过 SCM_RIGHTS 传递套接字，也可以修改权限（例如在 Debian 系统上
使用 660 和 root:kvm）。如有需要，libvirt 也可启动独立的辅助进程。
具体策略由用户决定。

更多详情请参阅 qemu-pr-helper 文档或手册页。

当前限制
-------------------

- 仅适用于 Intel 主机 CPU，因为 AMD CPU 使用不同的 MSR 地址。

- 目前只报告封装电源域（MSR_PKG_ENERGY_STATUS）。
