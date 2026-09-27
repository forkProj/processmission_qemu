.. _riscv-aia:

RISC-V 机器的 AIA 支持
================================================

RISC-V 的 virt 机器为 TCG 和 KVM 加速器实现 AIA（高级中断架构）。

主要支持两种模式：

- *aia=aplic*：添加一个或多个 APLIC（高级平台级中断控制器）。
- *aia=aplic-imsic*：添加一个或多个 APLIC，并为每个 CPU 添加 IMSIC（传入 MSI 控制器）。

从用户角度看，各模式的行为不因加速器而变。
从开发者角度看，加速器设置决定哪些部分在用户空间仿真，哪些由内核 irqchip 仿真。

TCG 下，全部控制器均在用户空间仿真，包括机器模式（m-mode）的 APLIC 和适用时的 IMSIC。

KVM 下：

- 不提供 m-mode，因此无论选择何种 AIA 模式，都不仿真 m-mode APLIC 或 IMSIC。
- aia=aplic 时，由用户空间仿真 s-mode APLIC。
- aia=aplic-imsic 有两种情况。不额外指定 KVM 选项时，用户空间不仿真 APLIC/IMSIC，
  虚拟机使用内核提供的控制器。若通过 ``-accel kvm,kernel-irqchip=split``
  选择拆分模式，则仿真 s-mode APLIC，并使用 irqchip 的 s-mode IMSIC。

下表概述 AIA 与加速器选项如何决定用户空间仿真内容：

.. list-table:: AIA 与加速器选项对控制器仿真的影响
   :widths: 25 25 25 25 25 25 25
   :header-rows: 1

   * - 加速器
     - 加速器属性
     - AIA 类型
     - APLIC m-mode
     - IMSIC m-mode
     - APLIC s-mode
     - IMSIC s-mode
   * - tcg
     - ---
     - aplic
     - 仿真
     - 不适用
     - 仿真
     - 不适用
   * - tcg
     - ---
     - aplic-imsic
     - 仿真
     - 仿真
     - 仿真
     - 仿真
   * - kvm
     - ---
     - aplic
     - 不适用
     - 不适用
     - 仿真
     - 不适用
   * - kvm
     - none
     - aplic-imsic
     - 不适用
     - 不适用
     - 内核中
     - 内核中
   * - kvm
     - irqchip=split
     - aplic-imsic
     - 不适用
     - 不适用
     - 仿真
     - 内核中


KVM 加速器选项 riscv-aia
----------------------------------------

KVM 的 riscv-aia 属性与 aia=aplic-imsic 共同决定主机 KVM 如何提供内核 s-mode IMSIC。
kernel-irqchip 不影响 riscv-aia，因为其任何可用设置都使用内核 s-mode IMSIC。
riscv-aia 也不影响 APLIC 的 m-mode/s-mode 或 IMSIC 的 m-mode 设置。

.. list-table:: riscv-aia 对内核 s-mode IMSIC 提供方式的影响
   :widths: 25 25 25 25
   :header-rows: 1

   * - 加速器
     - KVM riscv-aia
     - AIA 类型
     - IMSIC s-mode
   * - kvm
     - none
     - aplic-imsic
     - 内核中，默认 auto
   * - kvm
     - auto
     - aplic-imsic
     - 内核中；可用时硬件加速，否则仿真
   * - kvm
     - hwaccel
     - aplic-imsic
     - 内核中，使用客户机硬件的 IMSIC 控制器
   * - kvm
     - emul
     - aplic-imsic
     - 内核中，由 KVM 仿真 IMSIC
