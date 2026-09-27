Nuvoton iBMC 板（``kudo-bmc``、``mori-bmc``、``npcm750-evb``、``quanta-gbs-bmc``、``quanta-gsj``、``npcm845-evb``）
======================================================================================================================

`Nuvoton iBMC`_ 是基于 Arm 的 SoC 系列，设计用于各类服务器的基板管理控制器（BMC）。目前有两个系列：NPCM7XX 和 NPCM8XX。NPCM7XX 具有一个或两个 Arm Cortex-A9 CPU 核心，NPCM8XX 具有四个 Arm Cortex-A35 CPU 核心。两个系列都包含面向企业级或数据中心/超大规模应用的不同外设组合。

.. _Nuvoton iBMC: https://www.nuvoton.com/products/cloud-computing/ibmc/

NPCM750 SoC 具有两个 Cortex-A9 核心，面向企业级市场。以下机器基于此芯片：

- ``npcm750-evb``       Nuvoton NPCM750 评估板

NPCM730 SoC 具有两个 Cortex-A9 核心，面向数据中心和超大规模应用。以下机器基于此芯片：

- ``quanta-gbs-bmc``    Quanta GBS 服务器 BMC
- ``quanta-gsj``        Quanta GSJ 服务器 BMC
- ``kudo-bmc``          Fii USA Kudo 服务器 BMC
- ``mori-bmc``          Fii USA Mori 服务器 BMC

另有 NPCM710 和 NPCM705 两款 SoC，分别是 NPCM750 和 NPCM730 的单核变体，目前 QEMU 不支持。

NPCM8xx 是 NPCM7xx 的后继 SoC，具有四个 Cortex-A35 核心。以下机器基于此芯片：

- ``npcm845-evb``       Nuvoton NPCM845 评估板

支持的设备
-----------------

 * SMP（双核 Cortex-A9）
 * Cortex-A9MPCore 内置外设：SCU、GIC、全局定时器、私有定时器和看门狗
 * SRAM、ROM 和 DRAM 映射
 * 系统全局控制寄存器（GCR）
 * 时钟和复位控制器（CLK）
 * 定时器控制器（TIM）
 * 串口（基于 16550）
 * DDR4 内存控制器（用虚拟接口指示内存训练完成）
 * OTP 控制器（无保护功能）
 * 闪存接口单元（FIU，无保护功能）
 * 随机数生成器（RNG）
 * USB 主机（USBH）
 * GPIO 控制器
 * 模数转换器（ADC）
 * 脉宽调制（PWM）
 * SMBus 控制器（SMBF）
 * 以太网控制器（EMC）
 * 转速计
 * 外设 SPI 控制器（PSPI）

缺少的设备
---------------

 * LPC/eSPI 主机到 BMC 接口，包括：

   * 键盘和鼠标控制器接口（KBCI）
   * 键盘控制器风格（KCS）通道
   * BIOS POST 代码 FIFO
   * 系统唤醒控制（SWC）
   * 共享内存（SHM）
   * eSPI 从接口
   * 块传输接口（仅 8XX）
   * 虚拟 UART（仅 8XX）

 * 以太网控制器（GMAC）
 * USB 设备（USBD）
 * SD/MMC 主机
 * PECI 接口
 * PCI 和 PCIe 根复合体及桥
 * VDM 和 MCTP 支持
 * 串行 I/O 扩展
 * LPC/eSPI 主机
 * 协处理器
 * 图形
 * 视频采集
 * 编码压缩引擎
 * 安全功能
 * I3C 总线（仅 8XX）
 * 温度传感器接口（仅 8XX）
 * 虚拟 UART（仅 8XX）
 * 闪存监视器（仅 8XX）
 * JTAG 主机（仅 8XX）

启动选项
------------

Nuvoton 机器可从 OpenBMC 固件镜像启动，也可使用 ``-kernel`` 选项直接进入内核。OpenBMC 机器名称不一定与 QEMU 机器名称相同。请查看 OpenBMC 支持的机器列表和 Jenkins，以获取当前可用源码及预构建镜像。

根据以下文档，QEMU Nuvoton 机器已知的 OpenBMC（v2.18.0）目标名称为：

   https://github.com/openbmc/openbmc/blob/2.18.0/meta-phosphor/docs/supported-machines.md

.. list-table::
   :header-rows: 1

   * - QEMU 机器
     - OpenBMC 机器
   * - ``npcm750-evb``
     - ``evb-npcm750``
   * - ``npcm845-evb``
     - ``evb-npcm845``
   * - ``quanta-gbs-bmc``
     - ``gbs``
   * - ``kudo-bmc``
     - ``kudo``
   * - ``mori-bmc``
     - ``mori``

截至 2026 年 6 月，最新 OpenBMC 发布版 ``2.18.0`` 已不再列出 ``gsj`` 机器。要为 QEMU 的 ``quanta-gsj`` 机器构建镜像，请使用仍包含 ``meta-quanta/meta-gsj`` 的旧版 OpenBMC。``2.14.0`` 发布版包含 ``gsj`` 机器：

   https://github.com/openbmc/openbmc/tree/2.14.0/meta-quanta/meta-gsj

Jenkins 上可能提供部分 QEMU Nuvoton 机器的预构建 OpenBMC 镜像：

   https://jenkins.openbmc.org/

要在 Jenkins 查找预构建 MTD 镜像，请从 Jenkins 首页打开 ``latest-master`` 任务。选择 ``target`` 与 OpenBMC 机器名称匹配的矩阵配置，例如 ``label=docker-builder,target=gbs``，再打开其最近一次成功构建的产物。MTD 镜像通常位于 ``openbmc/build/tmp/deploy/images/<machine>/`` 下，文件名为 ``obmc-phosphor-image-<machine>-<timestamp>.static.mtd``。如果 Jenkins 未列出匹配目标，或构建产物不包含 MTD 镜像，则该机器当前没有可用的预构建 MTD 镜像。

固件镜像应作为 MTD 驱动器连接。例如：

.. code-block:: bash

  $ qemu-system-arm -machine quanta-gbs-bmc -nographic \
      -drive file=obmc-phosphor-image-gbs-xxxxxx.static.mtd,if=mtd,bus=0,unit=0,format=raw


测试镜像的默认 root 密码通常为 ``0penBmc``。

对于 Jenkins 未提供预构建镜像的其他机器，请按照 OpenBMC 构建文档从源码构建镜像。
