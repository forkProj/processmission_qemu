Compute Express Link（CXL）
===========================
从单主机视角看，CXL 是连接加速器和内存设备的互连标准。本文关注模拟主机
软件或功能模拟内部可见的部分，省略真实硬件和一般介绍中大量涉及的电气及
协议细节。这里只考虑单主机静态配置，也不讨论结构管理。

CXL 与 PCI Express 共用许多概念和基础设施。主桥包含根端口，可直接连接
CXL 或 PCI 端点，也可通过交换机连接。许多控制和能力通过 PCIe 接口暴露。
这种接口及代码复用也体现在 QEMU 中，大多数 CXL 元素基于对应 PCIe 设备。

CXL 设备支持：

* 多数传统 PCIe 接口

  - 配置空间
  - 用于寄存器和邮箱的 BAR 内存访问
  - MSI/MSI-X
  - AER
  - DOE 邮箱
  - IDE
  - 其他多种 PCIe 接口

* 内存操作

  - 等同访问 DRAM/NVDIMM，主机对普通内存支持的访问和功能也应适用于 CXL。

* 缓存操作。QEMU 不模拟一致性协议，因此通常无关；相关模拟属于设备专用
  行为，不在本文范围。

CXL 2.0 设备类型
--------------------
端点通常分三类。

**类型 1：** 一致性缓存主机内存，例如密码加速器。还可能有通过 PCI BAR
读写等方式访问的私有设备内存。

**类型 2：** 一致性缓存主机内存，并提供主机管理设备内存（HDM），其一致性
协议由主机管理。此主题较复杂，详见 CXL 2.0 规范。

**类型 3 内存设备：** 向主机提供额外 HDM，可为易失或持久内存。通过主机、
主桥、交换机上行口及端点中的 HDM 解码器，拓扑可在多个类型 3 设备间交错。

QEMU 模拟范围
------------------------------
重点是 CXL 2.0 及以后。早期功能较少，许多控制接口由实现或设备决定，通用
模拟困难：主机专用固件负责设置，端点作为根复合体集成端点呈现给操作系统。
2.0 更接近 PCIe，并完整定义拓扑发现机制。

CXL 系统组件
----------------------
系统由主机和若干标准组件组成，系统软件可按 2.0 规范发现其控制和能力。

CXL 固定内存窗口（CFMW）
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CFMW 是路由到指定主桥的一段主机物理地址空间。在通用软件初始化时，已经
具有确定的交错配置和服务质量限流组（QTG）。系统软件据此决定如何配置可用
内存设备间的交错。信息以 CXL Early Discovery Table（CEDT）ACPI 表中的
CFMWS 结构提供。

注意：QEMU 目前仅支持 QTG 0。

CXL 主桥（CXL HB）
~~~~~~~~~~~~~~~~~~~~~~~~
主桥类似 PCIe，但有规范定义的 CHBCR 组件寄存器。其 MMIO 位置通过 CEDT
中的 CHBS 结构报告给软件。接口与其他层级中放在 PCI BAR 的组件寄存器一致。

提供的接口包括：

* 配置 HDM 解码器，将特定 HPA 范围路由到服务该地址的设备所在端口。
  可以映射到单个根端口，也可跨多个根端口。

CXL 根端口（CXL RP）
~~~~~~~~~~~~~~~~~~~~~~~
作用与 PCIe 根端口相同，配置空间增加 CXL 专用 DVSEC，并通过 PCI BAR
访问相关组件寄存器。

CXL 交换机
~~~~~~~~~~
这里只考虑单个虚拟层级的简单交换机。复杂设备对某个主机可见的形态通常相同，
主机通常不感知复杂重路由或设备池，只看到热添加和热移除。

结构类似 PCIe，包含一个上行口、内部 PCI 总线和多个下行口。

上下行口都有专用 DVSEC 和 BAR 组件寄存器。上行口提供 HDM 解码配置，将
传入内存访问路由到正确下行口。

创建方式与 PCI 交换机类似：创建 cxl-upstream，再在内部总线上添加多个
cxl-downstream。

类型 3 内存设备
~~~~~~~~~~~~~~~~~~~~~~~~~~~
此类设备使用 PCI 类代码，供通用操作系统驱动支持。端点的 HDM 解码器不负责
路由，而负责把传入主机物理地址 HPA 转成设备物理地址 DPA。

CXL 内存交错
---------------------
以完整配置拓扑中的一次读取说明各组件关系。除 CFMW 外，所有组件都由系统
软件配置。软件从 CFMW 分配适当范围，再像普通 RAM 一样通过内存配置暴露。

以下 x 表示各级解码器的匹配::

  |<------------------SYSTEM PHYSICAL ADDRESS MAP (1)----------------->|
  |    __________   __________________________________   __________    |
  |   |          | |                                  | |          |   |
  |   | CFMW 0   | |  CXL Fixed Memory Window 1       | | CFMW 2   |   |
  |   | HB0 only | |  Configured to interleave memory | | HB1 only |   |
  |   |          | |  memory accesses across HB0/HB1  | |          |   |
  |   |__________| |_____x____________________________| |__________|   |
           |             |                     |             |
           |             |                     |             |
           |             |                     |             |
           |       Interleave Decoder          |             |
           |       Matches this HB             |             |
           \_____________|                     |_____________/
               __________|__________      _____|_______________
              |                     |    |                     |
       (2)    | CXL HB 0            |    | CXL HB 1            |
              | HB IntLv Decoders   |    | HB IntLv Decoders   |
              | PCI/CXL Root Bus 0c |    | PCI/CXL Root Bus 0d |
              |                     |    |                     |
              |___x_________________|    |_____________________|
                  |                |       |               |
                  |                |       |               |
       A HB 0 HDM Decoder          |       |               |
       matches this Port           |       |               |
                  |                |       |               |
       ___________|___   __________|__   __|_________   ___|_________
   (3)|  Root Port 0  | | Root Port 1 | | Root Port 2| | Root Port 3 |
      |  Appears in   | | Appears in  | | Appears in | | Appear in   |
      |  PCI topology | | PCI topology| | PCI topo   | | PCI topo    |
      |  as 0c:00.0   | | as 0c:01.0  | | as de:00.0 | | as de:01.0  |
      |_______________| |_____________| |____________| |_____________|
            |                  |               |              |
            |                  |               |              |
       _____|_________   ______|______   ______|_____   ______|_______
   (4)|     x         | |             | |            | |              |
      | CXL Type3 0   | | CXL Type3 1 | | CXL type3 2| | CLX Type 3 3 |
      |               | |             | |            | |              |
      | PMEM0(Vol LSA)| | PMEM1 (...) | | PMEM2 (...)| | PMEM3 (...)  |
      | Decoder to go | |             | |            | |              |
      | from host PA  | | PCI 0e:00.0 | | PCI df:00.0| | PCI e0:00.0  |
      | to device PA  | |             | |            | |              |
      | PCI as 0d:00.0| |             | |            | |              |
      |_______________| |_____________| |____________| |______________|


说明：

(1) **三个 CFMW** 对应不同系统物理范围。各自具有主桥交错配置：CFMW0
    不交错访问 HB0，CFMW2 不交错访问 HB1，CFMW1 在两主桥间交错。

(2) **两个主桥** 各有两个根端口及可编程 HDM，可路由到单端口或跨端口交错。
    例如 HB0 的 HDM0 把 CFMW0 请求送往 RP0，即 Type3 0 的部分内存；
    HDM1 把 CFMW0 另一地址范围送往 RP1，即 Type3 1；HDM2 将又一段范围
    在 RP0、RP1 间交错，实现两设备部分内存的双路交错。
    HDM3 把 CFMW1 中目标为 HB0 的交错访问送往 RP0，访问 Type3 0 的另一
    部分，作为例如 Type3 0 与 Type3 2 系统级双路交错的一部分。
    HDM4 再把 HB0 从 CFMW1 收到的交错请求分到 RP0、RP1，访问两设备更多
    区域，从而在全部四个 Type3 上形成系统级四路交错。
    这些只是该拓扑全部可行配置的代表性子集。

(3) **四个根端口**，本例的类型 3 设备直接连接它们。

(4) **四个类型 3 扩展内存设备**，各有 HDM 解码器，但这里执行的是 HPA 到
    本地 DPA 转换，而不是交错。

带交换机的拓扑示例::

  |<------------------SYSTEM PHYSICAL ADDRESS MAP (1)----------------->|
  |    __________   __________________________________   __________    |
  |   |          | |                                  | |          |   |
  |   | CFMW 0   | |  CXL Fixed Memory Window 1       | | CFMW 2   |   |
  |   | HB0 only | |  Configured to interleave memory | | HB1 only |   |
  |   |          | |  memory accesses across HB0/HB1  | |          |   |
  |   |____x_____| |__________________________________| |__________|   |
           |             |                     |             |
           |             |                     |             |
           |             |                     |
  Interleave Decoder     |                     |             |
   Matches this HB       |                     |             |
           \_____________|                     |_____________/
               __________|__________      _____|_______________
              |                     |    |                     |
              | CXL HB 0            |    | CXL HB 1            |
              | HB IntLv Decoders   |    | HB IntLv Decoders   |
              | PCI/CXL Root Bus 0c |    | PCI/CXL Root Bus 0d |
              |                     |    |                     |
              |___x_________________|    |_____________________|
                  |              |          |               |
                  |
       A HB 0 HDM Decoder
       matches this Port
       ___________|___
      |  Root Port 0  |
      |  Appears in   |
      |  PCI topology |
      |  as 0c:00.0   |
      |___________x___|
                  |
                  |
                  \_____________________
                                        |
                                        |
            ---------------------------------------------------
           |    Switch 0  USP as PCI 0d:00.0                   |
           |    USP has HDM decoder which direct traffic to    |
           |    appropriate downstream port                    |
           |    Switch BUS appears as 0e                       |
           |x__________________________________________________|
            |                  |               |              |
            |                  |               |              |
       _____|_________   ______|______   ______|_____   ______|_______
   (4)|     x         | |             | |            | |              |
      | CXL Type3 0   | | CXL Type3 1 | | CXL type3 2| | CLX Type 3 3 |
      |               | |             | |            | |              |
      | PMEM0(Vol LSA)| | PMEM1 (...) | | PMEM2 (...)| | PMEM3 (...)  |
      | Decoder to go | |             | |            | |              |
      | from host PA  | | PCI 10:00.0 | | PCI 11:00.0| | PCI 12:00.0  |
      | to device PA  | |             | |            | |              |
      | PCI as 0f:00.0| |             | |            | |              |
      |_______________| |_____________| |____________| |______________|


命令行示例
---------------------
单个直连类型 3 持久内存设备::

  qemu-system-x86_64 -M q35,cxl=on -m 4G,maxmem=8G,slots=8 -smp 4 \
  ...
  -object memory-backend-file,id=cxl-mem1,share=on,mem-path=/tmp/cxltest.raw,size=256M \
  -object memory-backend-file,id=cxl-lsa1,share=on,mem-path=/tmp/lsa.raw,size=256M \
  -device pxb-cxl,bus_nr=12,bus=pcie.0,id=cxl.1 \
  -device cxl-rp,port=0,bus=cxl.1,id=root_port13,chassis=0,slot=2 \
  -device cxl-type3,bus=root_port13,persistent-memdev=cxl-mem1,lsa=cxl-lsa1,id=cxl-pmem0,sn=0x1 \
  -M cxl-fmw.0.targets.0=cxl.1,cxl-fmw.0.size=4G


单个直连类型 3 易失内存设备::

  qemu-system-x86_64 -M q35,cxl=on -m 4G,maxmem=8G,slots=8 -smp 4 \
  ...
  -object memory-backend-ram,id=vmem0,share=on,size=256M \
  -device pxb-cxl,bus_nr=12,bus=pcie.0,id=cxl.1 \
  -device cxl-rp,port=0,bus=cxl.1,id=root_port13,chassis=0,slot=2 \
  -device cxl-type3,bus=root_port13,volatile-memdev=vmem0,id=cxl-vmem0 \
  -M cxl-fmw.0.targets.0=cxl.1,cxl-fmw.0.size=4G


同一易失配置也可带 LSA 区域::

  qemu-system-x86_64 -M q35,cxl=on -m 4G,maxmem=8G,slots=8 -smp 4 \
  ...
  -object memory-backend-ram,id=vmem0,share=on,size=256M \
  -object memory-backend-file,id=cxl-lsa0,share=on,mem-path=/tmp/lsa.raw,size=256M \
  -device pxb-cxl,bus_nr=12,bus=pcie.0,id=cxl.1 \
  -device cxl-rp,port=0,bus=cxl.1,id=root_port13,chassis=0,slot=2 \
  -device cxl-type3,bus=root_port13,volatile-memdev=vmem0,lsa=cxl-lsa0,id=cxl-vmem0 \
  -M cxl-fmw.0.targets.0=cxl.1,cxl-fmw.0.size=4G


适合四路交错的配置：一个固定窗口在两个主桥间双路交错，每个主桥两个根端口，
类型 3 直接连接，不经过交换机::

  qemu-system-x86_64 -M q35,cxl=on -m 4G,maxmem=8G,slots=8 -smp 4 \
  ...
  -object memory-backend-file,id=cxl-mem1,share=on,mem-path=/tmp/cxltest.raw,size=256M \
  -object memory-backend-file,id=cxl-mem2,share=on,mem-path=/tmp/cxltest2.raw,size=256M \
  -object memory-backend-file,id=cxl-mem3,share=on,mem-path=/tmp/cxltest3.raw,size=256M \
  -object memory-backend-file,id=cxl-mem4,share=on,mem-path=/tmp/cxltest4.raw,size=256M \
  -object memory-backend-file,id=cxl-lsa1,share=on,mem-path=/tmp/lsa.raw,size=256M \
  -object memory-backend-file,id=cxl-lsa2,share=on,mem-path=/tmp/lsa2.raw,size=256M \
  -object memory-backend-file,id=cxl-lsa3,share=on,mem-path=/tmp/lsa3.raw,size=256M \
  -object memory-backend-file,id=cxl-lsa4,share=on,mem-path=/tmp/lsa4.raw,size=256M \
  -device pxb-cxl,bus_nr=12,bus=pcie.0,id=cxl.1 \
  -device pxb-cxl,bus_nr=222,bus=pcie.0,id=cxl.2 \
  -device cxl-rp,port=0,bus=cxl.1,id=root_port13,chassis=0,slot=2 \
  -device cxl-type3,bus=root_port13,persistent-memdev=cxl-mem1,lsa=cxl-lsa1,id=cxl-pmem0,sn=0x1 \
  -device cxl-rp,port=1,bus=cxl.1,id=root_port14,chassis=0,slot=3 \
  -device cxl-type3,bus=root_port14,persistent-memdev=cxl-mem2,lsa=cxl-lsa2,id=cxl-pmem1,sn=0x2 \
  -device cxl-rp,port=0,bus=cxl.2,id=root_port15,chassis=0,slot=5 \
  -device cxl-type3,bus=root_port15,persistent-memdev=cxl-mem3,lsa=cxl-lsa3,id=cxl-pmem2,sn=0x3 \
  -device cxl-rp,port=1,bus=cxl.2,id=root_port16,chassis=0,slot=6 \
  -device cxl-type3,bus=root_port16,persistent-memdev=cxl-mem4,lsa=cxl-lsa4,id=cxl-pmem3,sn=0x4 \
  -M cxl-fmw.0.targets.0=cxl.1,cxl-fmw.0.targets.1=cxl.2,cxl-fmw.0.size=4G,cxl-fmw.0.interleave-granularity=8k


交换机下四个设备，支持单路、双路或四路交错::

  qemu-system-x86_64 -M q35,cxl=on -m 4G,maxmem=8G,slots=8 -smp 4 \
  ...
  -object memory-backend-file,id=cxl-mem0,share=on,mem-path=/tmp/cxltest.raw,size=256M \
  -object memory-backend-file,id=cxl-mem1,share=on,mem-path=/tmp/cxltest1.raw,size=256M \
  -object memory-backend-file,id=cxl-mem2,share=on,mem-path=/tmp/cxltest2.raw,size=256M \
  -object memory-backend-file,id=cxl-mem3,share=on,mem-path=/tmp/cxltest3.raw,size=256M \
  -object memory-backend-file,id=cxl-lsa0,share=on,mem-path=/tmp/lsa0.raw,size=256M \
  -object memory-backend-file,id=cxl-lsa1,share=on,mem-path=/tmp/lsa1.raw,size=256M \
  -object memory-backend-file,id=cxl-lsa2,share=on,mem-path=/tmp/lsa2.raw,size=256M \
  -object memory-backend-file,id=cxl-lsa3,share=on,mem-path=/tmp/lsa3.raw,size=256M \
  -device pxb-cxl,bus_nr=12,bus=pcie.0,id=cxl.1 \
  -device cxl-rp,port=0,bus=cxl.1,id=root_port0,chassis=0,slot=0 \
  -device cxl-rp,port=1,bus=cxl.1,id=root_port1,chassis=0,slot=1 \
  -device cxl-upstream,bus=root_port0,id=us0 \
  -device cxl-downstream,port=0,bus=us0,id=swport0,chassis=0,slot=4 \
  -device cxl-type3,bus=swport0,persistent-memdev=cxl-mem0,lsa=cxl-lsa0,id=cxl-pmem0,sn=0x1 \
  -device cxl-downstream,port=1,bus=us0,id=swport1,chassis=0,slot=5 \
  -device cxl-type3,bus=swport1,persistent-memdev=cxl-mem1,lsa=cxl-lsa1,id=cxl-pmem1,sn=0x2 \
  -device cxl-downstream,port=2,bus=us0,id=swport2,chassis=0,slot=6 \
  -device cxl-type3,bus=swport2,persistent-memdev=cxl-mem2,lsa=cxl-lsa2,id=cxl-pmem2,sn=0x3 \
  -device cxl-downstream,port=3,bus=us0,id=swport3,chassis=0,slot=7 \
  -device cxl-type3,bus=swport3,persistent-memdev=cxl-mem3,lsa=cxl-lsa3,id=cxl-pmem3,sn=0x4 \
  -M cxl-fmw.0.targets.0=cxl.1,cxl-fmw.0.size=4G,cxl-fmw.0.interleave-granularity=4k


交换机下四个易失内存 Type3，其中两个使用 HDM-DB 一致性，需要 Flit 模式::

  qemu-system-x86_64 -M q35,cxl=on -m 4G,maxmem=8G,slots=8 -smp 4 \
  ...
  -object memory-backend-ram,id=cxl-mem0,share=on,size=256M \
  -object memory-backend-ram,id=cxl-mem1,share=on,size=256M \
  -object memory-backend-ram,id=cxl-mem2,share=on,size=256M \
  -object memory-backend-ram,id=cxl-mem3,share=on,size=256M \
  -device pxb-cxl,bus_nr=12,bus=pcie.0,id=cxl.1 \
  -device cxl-rp,port=0,bus=cxl.1,id=root_port0,chassis=0,slot=0 \
  -device cxl-rp,port=1,bus=cxl.1,id=root_port1,chassis=0,slot=1 \
  -device cxl-upstream,bus=root_port0,id=us0,x-256b-flit=on \
  -device cxl-downstream,port=0,bus=us0,id=swport0,chassis=0,slot=4 \
  -device cxl-type3,bus=swport0,volatile-memdev=cxl-mem0,id=cxl-mem0,sn=0x1,x-256b-flit=on,hdm-db=on \
  -device cxl-downstream,port=1,bus=us0,id=swport1,chassis=0,slot=5 \
  -device cxl-type3,bus=swport1,volatile-memdev=cxl-mem1,id=cxl-mem1,sn=0x2,x-256b-flit=on,hdm-db=on \
  -device cxl-downstream,port=2,bus=us0,id=swport2,chassis=0,slot=6 \
  -device cxl-type3,bus=swport2,volatile-memdev=cxl-mem2,id=cxl-mem2,sn=0x3 \
  -device cxl-downstream,port=3,bus=us0,id=swport3,chassis=0,slot=7 \
  -device cxl-type3,bus=swport3,volatile-memdev=cxl-mem3,id=cxl-mem3,sn=0x4 \
  -M cxl-fmw.0.targets.0=cxl.1,cxl-fmw.0.size=4G,cxl-fmw.0.interleave-granularity=4k


arm/virt 上单个直连类型 3 易失内存示例::

  qemu-system-aarch64 -M virt,gic-version=3,cxl=on -m 4g,maxmem=8g,slots=4 -cpu max -smp 4 \
  ...
  -object memory-backend-ram,id=vmem0,share=on,size=256M \
  -device pxb-cxl,bus_nr=12,bus=pcie.0,id=cxl.1 \
  -device cxl-rp,port=0,bus=cxl.1,id=root_port13,chassis=0,slot=2 \
  -device cxl-type3,bus=root_port13,volatile-memdev=vmem0,id=cxl-vmem0 \
  -M cxl-fmw.0.targets.0=cxl.1,cxl-fmw.0.size=4G


弃用项
------------

Type3 的 memdev 已弃用，改用 persistent-memdev。为兼容，memdev 默认表示
持久内存，不能与 persistent-memdev 同时使用。

内核配置选项
----------------------------

Linux 5.18 要按本文方式由操作系统管理 CXL 内存，需启用：

* CONFIG_CXL_BUS
* CONFIG_CXL_PCI
* CONFIG_CXL_ACPI
* CONFIG_CXL_PMEM
* CONFIG_CXL_MEM
* CONFIG_CXL_PORT
* CONFIG_CXL_REGION

参考资料
----------

 - 联盟网站及规范：http://www.computeexpresslink.org
 - Compute Express Link（CXL）规范，修订 3.1，2023 年 8 月
