.. SPDX-License-Identifier: GPL-2.0-or-later

iommu-testdev：裸机测试用 IOMMU 设备
================================================================

概述
------------------------

``iommu-testdev`` 是仅供测试的最小 PCI 设备，
用于在无需固件或客户机操作系统的情况下测试 ARM SMMUv3 等 IOMMU 地址转换。
测试可填入已知转换表并触发 DMA，使请求经过 IOMMU 转换路径。
它 **不是** 忠实的 PCIe 端点，应视为 QEMU 内部测试工具。

主要功能
------------------------

* **裸机 IOMMU 测试**：无需客户机内核或固件。
* **可配置 DMA 属性**：通过 MMIO 配置地址空间。
* **确定性验证**：按已知模式先写后读，自动核对结果。

状态
------------------------

* 位置：``hw/misc/iommu-testdev.c``
* 头文件：``include/hw/misc/iommu-testdev.h``
* 构建开关：``CONFIG_IOMMU_TESTDEV``

设备接口
------------------------

设备提供一个 PCI BAR0，含以下 32 位 MMIO 寄存器：

* ``ITD_REG_DMA_TRIGGERING`` （0x00）：读取触发 DMA，并消费已准备请求。
* ``ITD_REG_DMA_GVA_LO`` （0x04）：DMA IOVA 的 [31:0] 位。
* ``ITD_REG_DMA_GVA_HI`` （0x08）：DMA IOVA 的 [63:32] 位。
* ``ITD_REG_DMA_GPA_LO`` （0x1C）：回读验证所用 GPA 的 [31:0] 位。
* ``ITD_REG_DMA_GPA_HI`` （0x20）：回读验证所用 GPA 的 [63:32] 位。
* ``ITD_REG_DMA_LEN`` （0x0C）：DMA 传输长度。
* ``ITD_REG_DMA_RESULT`` （0x10）：DMA 结果，0 成功，0xffffffff 空闲，0xfffffffe 已准备。
* ``ITD_REG_DMA_DBELL`` （0x14）：写 1 准备 DMA，写 0 取消准备。
  准备仅标记请求并设置 BUSY，不锁存或检查参数；为 qtest 提供显式控制点，
  也为异步执行或锁存留出空间。
* ``ITD_REG_DMA_ATTRS`` （0x18）：映射 MemTxAttrs 部分字段的 DMA 属性：

  - 位 0：secure，1 为 Secure，0 为 Non-Secure。
  - 位 [2:1]：ArmSecuritySpace，0 为 Secure，1 为 Non-Secure。
  - 位 3：space_valid，1 表示 space 有效，0 则忽略并默认 Non-Secure。
    仅设置 space_valid 才使用 MemTxAttrs 的 space。
    Secure/Non-Secure 情况下 secure 与 space 必须一致，否则返回 ITD_DMA_ERR_BAD_ATTRS。
    其他位保留，未来测试需要额外属性时可接入。

转换设置流程
----------------------------------------

iommu-testdev 不自行构建 SMMU、AMD-Vi 或 RISC-V IOMMU 结构。
架构专用构造完全在 qtest/libqos 辅助函数中完成：
将页表及架构数据结构填入客户机内存，直接配置仿真 IOMMU 寄存器。
SMMUv3 示例见 ``tests/qtest/libqos/qos-smmuv3.c`` 的
``qsmmu_setup_and_enable_translation()``。

DMA 操作流程
----------------------------------------

准备语义：

* 向 DMA_DBELL 写 bit0=1，将请求标记为已准备，并把 DMA_RESULT 设为 BUSY。
  不锁存 GVA/LEN/ATTRS，读取 DMA_TRIGGERING 时才采样。
* 写 bit0=0 取消准备，并将 DMA_RESULT 设为 IDLE。
* 读取 DMA_TRIGGERING 消费请求并清除准备状态，即使发生错误也如此。

流程拆分为以下步骤，便于时序控制与调试。
qtest 可以分别测试并断言 NOT_ARMED、BAD_LEN、TX/RD 失败、数据不匹配等路径，
而非将所有副作用隐藏在单一步骤中：

1. 测试配置 IOMMU 转换表。
2. 配置 DMA IOVA（GVA_LO/HI）、回读 GPA、长度和属性。
3. 向 DMA_DBELL 写 1 准备操作。
4. 读取 DMA_TRIGGERING 执行 DMA。
5. 轮询 DMA_RESULT：

   - 0x00000000：成功。
   - 0xFFFFFFFE：已准备，等待触发。DMA 同步运行，触发读取结束后看不到 BUSY。
   - 0xDEAD0006：属性错误，S/NS 的 secure 与 space 不一致。
   - 0xDEAD000X：其他错误码。

设备以已知模式 0x12345678 先写后读，自动验证数据完整性。

运行 qtest
----------------------------------------

SMMUv3 测试套件使用此设备，覆盖多种转换模式::

    cd build
    QTEST_QEMU_BINARY=./qemu-system-aarch64 \\
        ./tests/qtest/iommu-smmuv3-test --tap -k

覆盖以下场景：

* 仅阶段 1 转换。
* 仅阶段 2 转换。
* 嵌套转换，即阶段 1 加阶段 2。

实例化
------------------------

设备默认不接入任何开发板，测试通过命令行实例化::

    -device iommu-testdev

使用 SMMUv3 的 ARM 平台::

    -M virt,iommu=smmuv3 -device iommu-testdev

IOMMU 位于同一 PCI 根复合体 pci.0 时，设备自动置于其后。
其他 PCI 拓扑需显式指定总线。

限制
------------------------

* 不提供真实 PCIe 枚举、MSI/MSI-X 或中断处理。
* 不支持 ATS/PRI。
* 除 DMA 测试模式外没有实际设备功能。
* 仅供测试，不适合生产或逼真机器仿真。
* Secure/Root/Realm 地址空间支持取决于架构，并由 space_valid 控制。
* 回读使用配置的 GPA，经系统内存访问，避免再次经过 IOMMU。

另见
------------------------

* ``tests/qtest/iommu-smmuv3-test.c``：SMMUv3 测试套件。
* ``tests/qtest/libqos/qos-smmuv3.{c,h}``：SMMUv3 测试库。
* SMMUv3 仿真：``hw/arm/smmu*``。
