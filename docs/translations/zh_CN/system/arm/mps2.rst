Arm MPS2 和 MPS3 板（``mps2-an385``、``mps2-an386``、``mps2-an500``、``mps2-an505``、``mps2-an511``、``mps2-an521``、``mps3-an524``、``mps3-an536``、``mps3-an547``）
=========================================================================================================================================================================

这些板级模型使用 Arm M-profile 或 R-profile CPU。

Arm MPS2、MPS2+ 和 MPS3 开发板都基于 FPGA（2+ 的 FPGA 更大，其他方面与 2 相同；3 的 FPGA 又进一步增大，可支持 4GB RAM，并具有 USB 控制器和 QSPI 闪存）。

由于 CPU 本身和大多数设备都位于 FPGA 中，客户机所见的板级细节很大程度上取决于 FPGA 镜像。

QEMU 模拟以下 FPGA 镜像：

使用 M-profile CPU 的 FPGA 镜像：

``mps2-an385``
  Arm 应用笔记 AN385 描述的 Cortex-M3。
``mps2-an386``
  Arm 应用笔记 AN386 描述的 Cortex-M4。
``mps2-an500``
  Arm 应用笔记 AN500 描述的 Cortex-M7。
``mps2-an505``
  Arm 应用笔记 AN505 描述的 Cortex-M33。
``mps2-an511``
  Arm 应用笔记 AN511 描述的 Cortex-M3 'DesignStart'。
``mps2-an521``
  Arm 应用笔记 AN521 描述的双 Cortex-M33。
``mps3-an524``
  Arm 应用笔记 AN524 描述的 MPS3 上的双 Cortex-M33。
``mps3-an547``
  Arm 应用笔记 AN547 描述的 MPS3 上的 Cortex-M55。

使用 R-profile CPU 的 FPGA 镜像：

``mps3-an536``
  Arm 应用笔记 AN536 描述的 MPS3 上的双 Cortex-R52。

QEMU 与真实硬件的差异：

- 未实现 AN385/AN386 将低 16K 内存重映射到 ZBT SSRAM1 或块 RAM 的功能（QEMU 始终将其映射到 ZBT SSRAM1，相当于 zbt_boot_ctrl 始终为零）。
- 未实现 AN524 将低地址内存重映射到 BRAM 或 QSPI 闪存的功能（QEMU 始终映射到 BRAM，忽略 SCC CFG_REG0 的内存重映射位）。
- QEMU 提供 LAN9118 以太网而非 LAN9220；客户机可见的唯一区别是 LAN9118 不支持校验和卸载。
- QEMU 将 MPS3 板上的 QSPI 闪存仅建模为简单 ROM，而非真实 QSPI 闪存，因此客户机尝试重写闪存会失败。
- QEMU 不模拟 MPS3 板上的 USB 控制器。
- AN536 不支持在运行时通过 SCC CFG_REG0 寄存器控制 CPU 复位和停止。
- AN536 不支持通过 SCC CFG_REG1 寄存器启用或禁用闪存和 ATCM 接口。
- AN536 不支持通过 SCC CFG_REG6 和 CFG_REG7 寄存器配置设置初始向量表基址，也没有提供在启动时指定这些值的机制，因此所有客户机镜像都必须构建为从 TCM 启动（即假定复位时中断向量基址为 0）。
- AN536 默认只创建一个 CPU；这对应于真实 FPGA 镜像通常通过初始 SCC CFG_REG0 寄存器设置，使第二个 Cortex-R52 保持停止状态的运行方式。可以使用 ``-smp 2`` 创建第二个 CPU，此时两个 CPU 都会在启动时立即开始执行。

注意，在 AN536 上，第一个 UART 只能由 CPU0 访问，第二个 UART 只能由 CPU1 访问。两个 CPU 都能访问的第一个共享 UART 是第三个 UART。因此，客户机软件可能构建为使用第一个或第三个 UART；如果在正在查看的 UART 上没有看到输出，请尝试其他 UART。（即使 AN536 机器只启动一个 CPU，因而没有“CPU1 专用 UART”，UART 编号仍保持不变，第三个 UART 仍是第一个共享 UART。）

机器专用选项
""""""""""""""""""""""""

支持以下机器专用选项：

remap
  仅 ``mps3-an524`` 支持。
  设为 ``BRAM``/``QSPI`` 以选择初始内存映射。默认值为 ``BRAM``。
