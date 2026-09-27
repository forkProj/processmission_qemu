Microblaze-V 通用板（``amd-microblaze-v-generic``）
=========================================================

AMD MicroBlaze™ V 处理器是面向 AMD 自适应 SoC 和 FPGA 的 RISC-V 软核处理器 IP。MicroBlaze™ V 处理器基于 32 位（或 64 位）RISC-V 指令集架构（ISA），并包含与经典 MicroBlaze™ V 处理器兼容的接口（即它可在现有 RTL 设计中直接替换经典 MicroBlaze™ 处理器）。更多信息可参见以下文档。

https://docs.amd.com/r/en-US/ug1629-microblaze-v-user-guide/MicroBlaze-V-Architecture

QEMU 中的 MicroBlaze™ V 通用板支持以下设备：

    - timer
    - uartlite
    - uart16550
    - emaclite
    - timer2
    - axi emac
    - axi dma

QEMU 中的 MicroBlaze™ V 核心采用以下配置：

    - RV32I 基础整数指令集
    - "Zicsr" 控制和状态寄存器指令
    - "Zifencei" 指令获取
    - 扩展：m、a、f、c

运行
"""""""

以下命令行示例在 Microblaze-V 通用板上启动主线 U-Boot（xilinx_mbv32_defconfig）。

.. code-block:: bash

   $ qemu-system-riscv32 -M amd-microblaze-v-generic \
     -display none \
     -device loader,addr=0x80000000,file=u-boot-spl.bin,cpu-num=0 \
     -device loader,addr=0x80200000,file=u-boot.img \
     -serial mon:stdio \
     -device loader,addr=0x83000000,file=system.dtb \
     -m 2g
