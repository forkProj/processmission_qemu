.. SPDX-License-Identifier: GPL-2.0-or-later

M5Stack AI Pyramid（``ax650x-pyramid``）
========================================

概述
--------

``ax650x-pyramid`` 机器模拟 M5Stack AI Pyramid 中用于 Linux 启动的部分，其运行时设备树将 SoC 标识为 ``axera,ax650x``。模型用于直接启动支持 AX650X 的 Arm64 Linux 内核，不模拟产品中的 AXERA 加速器或固件启动链。

机器采用与目标系统一致的固定拓扑：八个 Cortex-A55 CPU，以及从物理地址 ``0x100000000`` 开始的 2 GiB RAM。拒绝其他 CPU 数量和内存大小。

支持的设备
-----------------

机器提供以下设备和架构服务：

* 八个 Cortex-A55 CPU，MPIDR 为 ``0x000`` 至 ``0x700``；
* 兼容 GIC-400、带虚拟化扩展的 GICv2 中断控制器；
* 运行频率为 24 MHz 的 Arm 通用定时器；
* 使用 SMC 调用通道的 PSCI 1.0；
* 位于 ``0x02016000`` 的 AXERA UART0 控制台，包括 AXERA 8250 驱动所需的扩展寄存器；
* 位于 ``0x28000000`` 的 AX650X SDHCI/eMMC 控制器，包括 Linux 探测期间使用的厂商 PHY 和 eMMC 寄存器；
* 两个兼容 Synopsys DWMAC 4.10a 的以太网控制器，包括 AX650X 时钟/复位适配、MDIO PHY 标识、DMA 和中断路径；
* 用于 PHY 复位的 DesignWare APB GPIO 块；
* GPIO 驱动使用的 AX650X 硬件自旋锁块。

QEMU 生成仅包含上述已实现设备的最小设备树。通过 ``if=sd`` 提供的 eMMC 后端连接到 AX650X 控制器。

直接启动 Linux
-----------------

模型要求使用包含 AXERA UART 和 SDHCI 驱动的未压缩 Arm64 ``Image``。以下示例从 eMMC 镜像的第 12 个分区启动 ext4 根文件系统，该镜像使用目标的固定分区布局。镜像没有 MBR 或 GPT，因此必须提供完整的 ``blkdevparts`` 参数：

.. code-block:: shell

  QEMU=${QEMU:-build/qemu-system-aarch64}
  ASSET_DIR=${ASSET_DIR:-assets/ax650x}
  KERNEL=${KERNEL:-$ASSET_DIR/Image-5.15.73-axera}
  EMMC=${EMMC:-$ASSET_DIR/ax650x-ubuntu-22.04-emmc.raw}

  PARTS='mmcblk0:1536K(uboot),1536K(uboot_bk),1M(env),20M(param)'
  PARTS="$PARTS,6M(logo),1M(dtb),64M(kernel),1M(atf),1M(optee)"
  PARTS="$PARTS,1M(recovery_dtb),74M(recovery),30380032K(rootfs)"
  CMDLINE='console=ttyS0,115200n8 earlycon=uart8250,mmio32,0x2016000'
  CMDLINE="$CMDLINE root=/dev/mmcblk0p12 rootfstype=ext4 rw rootwait"
  CMDLINE="$CMDLINE blkdevparts=$PARTS"
  CMDLINE="$CMDLINE systemd.show_status=yes systemd.log_target=console"

  for input in "$QEMU" "$KERNEL" "$EMMC"; do
      if [ ! -r "$input" ]; then
          echo "missing input: $input" >&2
          exit 1
      fi
  done

  exec "$QEMU" \
      -machine ax650x-pyramid \
      -accel tcg,thread=multi \
      -cpu cortex-a55 \
      -smp 8 \
      -m 2G \
      -kernel "$KERNEL" \
      -append "$CMDLINE" \
      -drive "file=$EMMC,if=sd,format=raw,snapshot=on" \
      -chardev stdio,id=serial0,signal=off \
      -serial chardev:serial0 \
      -display none \
      -monitor none \
      -no-reboot


``snapshot=on`` 保持可复用 eMMC 镜像不变。只有确实需要持久保存客户机写入时，才应移除此选项。

此下游模型已使用 AXERA Linux 5.15.73 内核和 Ubuntu 22.04.5 LTS 运行验证。Linux 枚举全部八个 CPU，通过 64 位 ADMA 探测 eMMC，创建十二个固定分区，并将 ``/dev/mmcblk0p12`` 挂载为可写根文件系统。

已知限制
-----------------

* 仅实现 QEMU 直接内核启动。不加载或执行 BootROM、SPL、U-Boot、Arm Trusted Firmware 和 OP-TEE 镜像。
* 未建模 NPU、VDSP、RISC-V 辅助核心、视频、ISP、显示、音频、USB、PCIe、SATA，以及完整时钟、复位和电源管理树。GPIO 行为仅限于 PHY 复位所需的 DesignWare APB 子集。
* 未建模 DWMAC TSO、PTP/TSN、多队列性能保真度和模拟 PHY 时序。
* UART 扩展窗口实现了探测阶段所需的子集。由于 UART 组件版本寄存器有意选择兼容回退路径，AXERA 驱动可能报告无害的能力不匹配。
* QEMU 直接启动时 CPU 处于 EL2，而目标固件在 EL1 启动 Linux，因此厂商内核在 QEMU 下可能打印非致命的 GICv2 CPU 接口范围警告。
* SDHCI 厂商寄存器组实现了探测、复位和数据传输所需的寄存器。不保证 HS200/HS400 时序精度。

运行测试
-------------

板级 qtest 覆盖 CPU 拓扑、RAM、GICv2、UART MMIO 和 IRQ 行为、SDHCI 厂商寄存器、复位、块 I/O、eMMC IRQ 路由、64 位 ADMA、Auto CMD23、DWMAC 综合配置寄存器、MDIO、GPIO 和复位适配、硬件自旋锁、40 位 DMA 以及以太网 IRQ 路由::

  $ meson test -C build qemu:qtest-aarch64/ax650x-pyramid-test \
      --print-errorlogs


Ubuntu 快速启动功能测试从 `AX650X Ubuntu 22.04 QEMU 资源发布
<https://github.com/processmission/qemu/releases/tag/ax650x-ubuntu-22.04-qemu1>`__ 下载固定版本的内核和压缩 qcow2 镜像。功能测试资源层验证两者的 SHA-256 摘要。测试使用 ``snapshot=on``，并等待 DWMAC 探测、eMMC 分区映射、根文件系统挂载、Ubuntu 就绪标记和串口登录提示符：

.. code-block:: shell

  $ meson test -C build \
        --suite thorough \
        func-aarch64-ax650x_ubuntu \
        --print-errorlogs


由于内核和 Ubuntu 镜像是外部资源，此测试注册在 ``thorough`` 功能测试套件中。只有检查过声明的内容哈希后，才会复用缓存副本。
