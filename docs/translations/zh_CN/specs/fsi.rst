============================================================
IBM 灵活服务接口（FSI）
============================================================

QEMU 的 FSI 仿真实现 ASPEED SoC、FSI 主从设备与末端引擎之间的硬件接口。

FSI 是点对点双线接口，最远支持 4 米。IBM 服务器多年来通过它，
将 IBM 灵活支持处理器（FSP）连接到 CPU 和 IBM ASIC。

FSI 让服务处理器访问主机 POWER 处理器内部总线，进行配置或调试。
它在 POWER 处理器中存在已久，也带来一些历史负担，包括与 ASPEED SoC 的集成方式。

从 POWER 处理器向外看，实现涉及以下基本组件，更多信息见 `FSI specification`_：

1. 通用 FRU 访问宏（CFAM）：包含多种引擎的地址空间，
   驱动对 POWER 芯片内部和外部总线的访问，例如 SBEFIFO 和 I2C 主控制器。
   引擎挂接在本地总线 LBUS 上，由 CFAM 配置块描述。

2. FSI 从设备：是发给该设备的 FSI 符号在总线上的终点。
   从设备可以级联，其配置寄存器位于所连接 CFAM 的地址空间。

3. FSI 主设备：位于 BMC 等平台服务处理器中的控制器，向 POWER 芯片发起 CFAM 引擎访问。
   在硬件层面，FSI 是基于位的协议，支持同步和 DMA 驱动的引擎访问。

4. 片上外设总线 OPB：通常位于 POWER 处理器的低速总线。
   因 FSI 主 IP 与 OPB 紧密集成，也出现在 ASPEED SoC 中，
   主要体现为 CFAM 地址直接通过 MMIO 映射到 OPB 地址空间子区域。

5. APB 到 OPB 桥：让 AST2600 的 Arm 核访问 OPB。
   硬件限制使 OPB 不能直接映射到 APB，所有访问都经桥间接完成。

LBUS 建模用于维持 qdev 总线层级，并利用对象模型自动生成 CFAM 配置块。
配置块按引擎连接到 LBUS 的顺序呈现。引擎实现应继承 LBusDevice，
并将 LBusDeviceClass 的 config 成员设为匹配引擎类型。

CFAM 设计很灵活，例如可同时由多个 FSI 链路驱动。
当前模型并未覆盖全部能力，假定每个 CFAM 只连接一个 FSI 从设备，
因此 CFAM 继承 FSI 从设备。

FSI 符号和线路协议完全未建模。由于 CFAM 映射到 OPB 地址空间，
实现基本功能无需模拟这些内容；模型直接把 CFAM 内存区映射到 OPB 内存区。

以下命令启动内置 FSI 模型的 rainier-bmc，没有模型专用参数。
该机器详情见 :doc:`../../system/arm/aspeed`。

.. code-block:: console

  qemu-system-arm -M rainier-bmc -nographic \
  -kernel fitImage-linux.bin \
  -dtb aspeed-bmc-ibm-rainier.dtb \
  -initrd obmc-phosphor-initramfs.rootfs.cpio.xz \
  -drive file=obmc-phosphor-image.rootfs.wic.qcow2,if=sd,index=2 \
  -append "rootwait console=ttyS4,115200n8 root=PARTLABEL=rofs-a"

QEMU 设备树中的实现如下：

.. code-block:: console

  (qemu) info qtree
  bus: main-system-bus
    type System
    ...
    dev: aspeed.apb2opb, id ""
      gpio-out "sysbus-irq" 1
      mmio 000000001e79b000/0000000000001000
      bus: opb.1
        type opb
        dev: fsi.master, id ""
          bus: fsi.bus.1
            type fsi.bus
            dev: cfam.config, id ""
            dev: cfam, id ""
              bus: lbus.1
                type lbus
                dev: scratchpad, id ""
                  address = 0 (0x0)
      bus: opb.0
        type opb
        dev: fsi.master, id ""
          bus: fsi.bus.0
            type fsi.bus
            dev: cfam.config, id ""
            dev: cfam, id ""
              bus: lbus.0
                type lbus
                dev: scratchpad, id ""
                  address = 0 (0x0)

pdbg 是从 BMC 调试主机 POWER 处理器的简单应用，详情见 `pdbg source repository`_。

.. code-block:: console

  root@p10bmc:~# pdbg -a getcfam 0x0
  p0: 0x0 = 0xc0022d15

.. _FSI specification:
   https://openpowerfoundation.org/specifications/fsi/

.. _pdbg source repository:
   https://github.com/open-power/pdbg
