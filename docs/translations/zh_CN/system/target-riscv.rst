.. _RISC-V-System-emulator:

RISC-V 系统模拟器
======================

QEMU 支持 32 位和 64 位 RISC-V CPU，分别使用 ``qemu-system-riscv32`` 和
``qemu-system-riscv64``。

QEMU 对 RISC-V 客户机支持较好，提供多种机器。RISC-V 硬件比 x86 更多样：
各公司将 CPU 和不同设备集成进 SoC，再构成机器，即使使用同一 SoC，机器
仍可能存在进一步差异。

多数开发板的 CPU 类型固定并匹配真实硬件，通常无需手动指定，virt 等特殊
情况除外。

选择开发板模型
----------------------

RISC-V 系统模拟必须用 ``-M`` 或 ``--machine`` 指定模型，没有默认值。

不同 RISC-V 系统在基本设计上就有很大差异，为某机器制作的系统或固件
通常完全不能在其他机器运行。习惯 x86 标准 PC 的新用户常感到意外。
内核启动后，多数用户空间软件对硬件细节依赖较少。

如果已有能在硬件运行的系统镜像或内核，先检查 ``-machine help`` 是否列出
对应机器。列出时通常可使用该模型；未列出则镜像几乎肯定无法启动。
也许可以提取文件系统，搭配能够在 QEMU 支持机器上启动的其他内核使用。

如果只想运行 Linux，不在意复现特定硬件的内存小、缺少 PCI 或硬盘等限制，
最好使用 ``virt``。它不对应真实硬件，专为虚拟机设计。需要用适当配置编译
Linux。virt 支持 PCI、virtio、较新 CPU、大容量 RAM 和 64 位 CPU。

开发板专用文档
----------------------------

许多受支持的 RISC-V 开发板目前还没有文档。可运行
``qemu-system-riscv64 --machine help`` 或
``qemu-system-riscv32 --machine help`` 查看完整列表。

..
   此目录按各文件标题的字母顺序排列，
   与按文件名排序的顺序不同。

.. toctree::
   :maxdepth: 1

   riscv/k230-canmv
   riscv/microblaze-v-generic
   riscv/microchip-icicle-kit
   riscv/milkv-duo
   riscv/mips
   riscv/shakti-c
   riscv/sifive_u
   riscv/spacemit-k3
   riscv/tt_atlantis
   riscv/virt
   riscv/xiangshan-kunminghu
   riscv/riscv-server-ref

RISC-V CPU 固件
-------------------

sifive_u 和 virt 支持三种固件启动方式：

* ``-bios default``

未指定 -bios 时的默认行为，自动加载随 QEMU 发布的 OpenSBI，无需额外操作。
用户只需用 -kernel 指定要启动的内核。

* ``-bios none``

不自动加载固件，用户负责加载全部所需镜像。

* ``-bios <file>``

加载指定文件作为固件。

RISC-V CPU 字节序
---------------------

RISC-V ISA 规定取指始终为小端，而数据访问可由 MSTATUS 的 MBE、SBE、UBE
位控制为小端或大端，见特权规范 3.1.6.5 节 Memory Endianness。

QEMU 实现这些位描述的完整数据字节序行为。另外，CPU 对象提供布尔属性
``big-endian``，模拟 MBE、SBE、UBE 硬连为 1 的纯大端硬件。启用后复位时
三个位都设为 1，因此从复位向量开始即使用大端数据模式。这是每 CPU 的
静态硬件配置，不用于运行时切换。

该属性不模拟软件可在运行时切换这些位的混合字节序实现。QEMU 将这些字段
视为由 CPU 配置固定：默认复位为 0，启用 big-endian 时为 1。

可在命令行启用，例如::

    -cpu <cpu>,big-endian=on

目前没有上游 CPU 模型默认采用大端。此属性用于模拟纯大端 RISC-V 变体。
