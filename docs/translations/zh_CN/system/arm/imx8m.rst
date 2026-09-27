NXP i.MX 8M Plus 和 i.MX 8M Mini 评估套件（``imx8mp-evk``、``imx8mm-evk``）
==================================================================================

``imx8mp-evk`` 和 ``imx8mm-evk`` 机器模拟基于 i.MX 8M Plus 和 i.MX 8M Mini SoC 的相应评估套件。

支持的设备
-----------------

``imx8mp-evk`` 和 ``imx8mm-evk`` 机器实现了以下设备：

 * 最多四个 Cortex-A53 核心
 * 通用中断控制器（GICv3）
 * 四个 UART
 * 三个 USDHC 存储控制器
 * 一个 Designware PCI Express 控制器
 * 一个以太网控制器
 * 两个 Designware USB 3 控制器
 * 两个 FlexCAN 3 CAN 控制器（仅 ``imx8mp-evk``）
 * 五个 GPIO 控制器
 * 六个 I2C 控制器
 * 三个 SPI 控制器
 * 三个看门狗
 * 六个通用定时器
 * 安全非易失性存储（SNVS），包含 RTC
 * 时钟树

启动选项
------------

``imx8mp-evk`` 和 ``imx8mm-evk`` 机器可以使用标准 ``-kernel`` 功能直接启动 Linux 内核。

直接启动 Linux 内核
''''''''''''''''''''''''

在此机器上运行完整 Linux 系统的最简单方法，可能是用 Buildroot 生成镜像。撰写本文时测试了 2024.11.1 版本，流程分为两步。首先，在 Buildroot 源代码树顶层目录运行以下命令。

对于 i.MX 8M Plus EVK：

.. code-block:: bash

  $ make freescale_imx8mpevk_defconfig
  $ make


对于 i.MX 8M Mini EVK：

.. code-block:: bash

  $ make freescale_imx8mmevk_defconfig
  $ make


成功完成后会出现 ``output/image`` 子目录。进入该目录，将 SD 卡镜像大小调整为二的幂：

.. code-block:: bash

  $ qemu-img resize sdcard.img 256M


准备完成后，即可按以下方式启动机器。

对于 i.MX 8M Plus EVK：

.. code-block:: bash

  $ qemu-system-aarch64 -M imx8mp-evk \
      -display none -serial null -serial stdio \
      -kernel Image \
      -dtb imx8mp-evk.dtb \
      -append "root=/dev/mmcblk2p2" \
      -drive file=sdcard.img,if=sd,bus=2,format=raw,id=mmcblk2


对于 i.MX 8M Mini EVK：

.. code-block:: bash

  $ qemu-system-aarch64 -M imx8mm-evk -smp 4 -m 2G \
      -display none -serial null -serial stdio \
      -kernel Image \
      -dtb imx8mm-evk.dtb \
      -append "root=/dev/mmcblk2p2" \
      -drive file=sdcard.img,if=sd,bus=2,format=raw,id=mmcblk2


KVM 加速
----------------

在命令行中添加 ``-accel kvm``，即可启用 KVM 硬件辅助加速。虽然这会显著提升性能，但需要注意以下限制：

* ``imx8mp-evk`` 和 ``imx8mm-evk`` 机器不属于 :doc:`QEMU 安全策略 </system/security>` 中的“虚拟化使用场景”。这意味着无论使用 TCG 还是 KVM，都不应信任它能够隔离恶意客户机。如果不信任客户机，并依赖 QEMU 作为安全边界，应选择 ``virt`` 等其他机器。
* 将使用与主机相同的 CPU 类型，而不是 Cortex-A53。这是 KVM 的限制，对严格依赖 Cortex-A53 的客户机可能不适用。
* 不提供 EL2 和 EL3 异常级别，这也是 KVM 的限制。直接内核启动应可工作，但运行 U-Boot、TF-A 等不会成功。
