AWS Nitro Enclaves
==================

`AWS Nitro Enclaves <https://aws.amazon.com/ec2/nitro/nitro-enclaves/>`_ 是与
EC2 实例并行运行的隔离计算环境。它从父实例划分 CPU 和内存，在 Nitro
Hypervisor 管理的机密虚拟机中启动已签名 EIF 文件。

在启用 enclave 支持的 EC2 上，QEMU 可通过 nitro 加速器和机器类型启动。

前提条件
-------------

* EC2 实例启用 Nitro Enclaves
* 加载 nitro_enclaves 内核模块，提供 /dev/nitro_enclaves
* 用 nitro-enclaves-allocator 向 CPU 池分配内核
* 用同一工具分配大页

快速开始
-----------

从预构建 EIF 启动::

    $ qemu-system-x86_64 -accel nitro,debug-mode=on -M nitro -nographic \
        -smp 2 -m 512M -kernel enclave.eif


从独立内核和 initrd 启动::

    $ qemu-system-x86_64 -accel nitro,debug-mode=on -M nitro -nographic \
        -smp 2 -m 512M -kernel vmlinuz -initrd initrd.cpio \
        -append "console=ttyS0"


Graviton EC2 上可将程序换为 qemu-system-aarch64，命令其余相同。

加速器
-----------

-accel nitro 通过 /dev/nitro_enclaves 创建设备并管理 enclave，负责：

* 创建虚拟机槽位
* 提供大页支持的内存区域
* 添加完整物理核心的 vCPU
* 启动 enclave
* 向 vsock 总线设备通知 CID

选项：

``debug-mode=on|off``
    启用调试模式，Nitro Hypervisor 通过 vsock 端口暴露串口输出，机器模型
    自动连接。此时 PCR 值为零，默认 off。

机器
-------

-M nitro 是与架构无关的最小机器，只提供 enclave 所需内容：

* 基于大页 memfd 的 RAM
* 默认 host 类型的 vCPU
* Nitro vsock 总线，包含：

  - 9000 端口的心跳服务器
  - 串口桥，作为 vsock 客户端，仅调试模式可用

通信仅限 virtio-vsock。启动时分配 CID，可用
``-accel nitro,enclave-cid=<N>`` 请求指定值，0 表示由监控程序选择。
可从 vsock 桥读取实际值::

    (qemu) qom-get /machine/peripheral/nitro-vsock enclave-cid


EIF 镜像格式
^^^^^^^^^^^^^^^^

Nitro 从 EIF 启动。-kernel 文件通过 .eif 魔数识别为 EIF 时，直接载入内存。

普通 bzImage 或 Image 则由机器根据 -kernel、-initrd、-append 动态组装
最小 EIF，无需外部工具即可直接启动内核。

CPU 要求
^^^^^^^^^^^^^^^^

必须使用完整物理核心。超线程系统的 -smp 必须是每核心线程数的倍数，通常为 2。

只能使用已经捐赠到 Nitro CPU 池的核心。可用 allocator，或手动写参数。
例如分配 vCPU 1、2、3::

  $ echo 1,2,3 | sudo tee /sys/module/nitro_enclaves/parameters/ne_cpus


x86-64 的同核超线程编号并不连续，必须成对加入。可用 lstopo、lscpu 查看
实例拓扑。

内存要求
^^^^^^^^^^^^^^^^^^^

内存必须由大页支持，机器自动创建启用大页的 memfd 后端。系统需预留大页，
例如在 PAGE_SIZE 为 4 KiB 的系统预留 1 GiB::

    $ echo 512 | sudo tee /proc/sys/vm/nr_hugepages


模拟 Nitro Enclaves
-----------------------

除原生方式外，也可用 :doc:`i386/nitro-enclave` 的模拟机器，实现 x86 Nitro
设备模型。-M nitro 委托 Nitro Hypervisor 模拟设备，-M nitro-enclave 则
自行实现，因此非 EC2 也能运行。

若需要有效 AWS 证书支持的 NSM 证明，必须用 -M nitro。模拟模型不提供 AWS
签名的证明文档。
